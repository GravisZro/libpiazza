// SPDX-License-Identifier: AGPL-3.0-only
#include "http_transport.h"

#include "error.h"
#include "json.h"
#include "protocol.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <system_error>
#include <utility>

namespace piazza
{

  HttpTransport::HttpTransport(QNetworkAccessManager* nam)
      : HttpTransport(nam, Options{})
  {
  }

  HttpTransport::HttpTransport(QNetworkAccessManager* nam, Options opts)
      : m_nam(nam), m_opts(std::move(opts))
  {
    if(!m_nam)
    {
      throw jsonrpc::Error(jsonrpc::error_code::internal_error,
                           QStringLiteral("HttpTransport: null QNetworkAccessManager"));
    }
  }

  HttpTransport::~HttpTransport() { close(); }

  void HttpTransport::on_message(MessageCallback cb)
  {
    m_on_message = std::move(cb);
  }

  void HttpTransport::on_error(ErrorCallback cb)
  {
    m_on_error = std::move(cb);
  }

  void HttpTransport::on_close(CloseCallback cb)
  {
    m_on_close = std::move(cb);
  }

  void HttpTransport::start(void)
  {
    std::lock_guard<std::mutex> lk(m_mu);
    if(m_running)
      return;
    m_running = true;
    m_closed = false;
  }

  void HttpTransport::close(void)
  {
    QSet<QNetworkReply*> replies;
    {
      std::lock_guard<std::mutex> lk(m_mu);
      if(m_closed)
        return;
      m_closed = true;
      m_running = false;
      replies = m_replies;
    }
    for(QNetworkReply* reply : replies)
    {
      if(reply)
        reply->abort();
    }
  }

  bool HttpTransport::is_open(void) const noexcept
  {
    std::lock_guard<std::mutex> lk(m_mu);
    return m_running && !m_closed;
  }

  QUrl HttpTransport::build_url(const QString& method) const
  {
    QUrl url = m_opts.base_url;
    if(m_opts.append_method_query)
    {
      QUrlQuery query;
      query.addQueryItem(QStringLiteral("method"), method);
      query.addQueryItem(QStringLiteral("aid"), piazza::nonce());
      url.setQuery(query);
    }
    return url;
  }

  QString HttpTransport::csrf_token(void) const
  {
    if(!m_nam)
      return {};
    QNetworkCookieJar* jar = m_nam->cookieJar();
    if(!jar)
      return {};
    const QList<QNetworkCookie> cookies
        = jar->cookiesForUrl(QUrl(QStringLiteral("https://piazza.com")));
    for(const QNetworkCookie& cookie : cookies)
    {
      if(cookie.name() == m_opts.csrf_cookie_name.toUtf8())
        return QString::fromUtf8(cookie.value());
    }
    return {};
  }

  std::error_code HttpTransport::send(QByteArrayView frame)
  {
    if(!is_open())
      return std::make_error_code(std::errc::not_connected);

    QJsonValue frame_value;
    try
    {
      frame_value = jsonrpc::parse_json(frame);
    }
    catch(...)
    {
      return std::make_error_code(std::errc::invalid_argument);
    }

    if(!frame_value.isObject())
      return std::make_error_code(std::errc::invalid_argument);

    const QJsonObject request_object = frame_value.toObject();
    const QString method = request_object.value(QStringLiteral("method")).toString();
    if(method.isEmpty())
      return std::make_error_code(std::errc::invalid_argument);

    const bool has_id = request_object.contains(QStringLiteral("id"));
    const QJsonValue id = request_object.value(QStringLiteral("id"));
    QJsonValue params = request_object.value(QStringLiteral("params"));
    if(params.isUndefined() || params.isNull())
      params = QJsonObject{};

    QNetworkRequest request(build_url(method));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");
    if(!m_opts.user_agent.isEmpty())
      request.setHeader(QNetworkRequest::UserAgentHeader, m_opts.user_agent);

    const QString csrf = csrf_token();
    if(!csrf.isEmpty())
      request.setRawHeader(m_opts.csrf_header_name.toUtf8(), csrf.toUtf8());

    QJsonObject body;
    body.insert(QStringLiteral("method"), method);
    body.insert(QStringLiteral("params"), params);
    const QByteArray payload
        = QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkReply* reply = m_nam->post(request, payload);
    if(!reply)
      return std::make_error_code(std::errc::io_error);

    {
      std::lock_guard<std::mutex> lk(m_mu);
      if(m_closed)
      {
        reply->abort();
        reply->deleteLater();
        return std::make_error_code(std::errc::not_connected);
      }
      m_replies.insert(reply);
    }

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    if(!reply->isFinished())
      loop.exec();

    {
      std::lock_guard<std::mutex> lk(m_mu);
      m_replies.remove(reply);
    }

    const int status
        = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QNetworkReply::NetworkError net_error = reply->error();
    const QString net_error_string = reply->errorString();
    const QByteArray raw = reply->readAll();
    reply->deleteLater();

    QJsonObject response;
    response.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    response.insert(QStringLiteral("id"),
                    has_id ? id : QJsonValue(QJsonValue::Null));

    bool failed = false;
    QString error_message;
    QJsonValue error_data;

    if(net_error != QNetworkReply::NoError)
    {
      failed = true;
      error_message = net_error_string;
    }
    else if(status != 0 && (status < 200 || status >= 300))
    {
      failed = true;
      error_message = QStringLiteral("HTTP %1").arg(status);
      if(!raw.isEmpty())
        error_message += QStringLiteral(": ") + QString::fromUtf8(raw);
    }
    else
    {
      QJsonValue parsed;
      try
      {
        parsed = jsonrpc::parse_json(raw);
      }
      catch(const jsonrpc::Error& e)
      {
        failed = true;
        error_message = QStringLiteral("invalid JSON response: ") + e.message();
      }

      if(!failed)
      {
        if(parsed.isObject())
        {
          const QJsonObject object = parsed.toObject();
          if(const auto it = object.constFind(QStringLiteral("error"));
             it != object.constEnd() && !it->isNull()
             && !(it->isBool() && !it->toBool()))
          {
            failed = true;
            const QJsonValue error = *it;
            if(error.isString())
            {
              error_message = error.toString();
            }
            else if(error.isObject())
            {
              const QJsonObject error_object = error.toObject();
              error_message = error_object.value(QStringLiteral("message"))
                                  .toString(QStringLiteral("Piazza API error"));
              error_data = error;
            }
            else
            {
              error_message = QStringLiteral("Piazza API error");
              error_data = error;
            }
          }
          else if(object.contains(QStringLiteral("result")))
          {
            response.insert(QStringLiteral("result"),
                            object.value(QStringLiteral("result")));
          }
          else
          {
            response.insert(QStringLiteral("result"), parsed);
          }
        }
        else
        {
          response.insert(QStringLiteral("result"), parsed);
        }
      }
    }

    if(failed)
    {
      QJsonObject error_object;
      error_object.insert(QStringLiteral("code"),
                          static_cast<int>(jsonrpc::error_code::internal_error));
      error_object.insert(QStringLiteral("message"), error_message);
      if(!error_data.isNull())
        error_object.insert(QStringLiteral("data"), error_data);
      response.insert(QStringLiteral("error"), error_object);
    }

    if(!has_id)
    {
      if(failed && m_on_error)
        m_on_error(std::make_error_code(std::errc::io_error));
      return failed ? std::make_error_code(std::errc::io_error)
                    : std::error_code{};
    }

    if(m_on_message)
    {
      m_on_message(QJsonDocument(response).toJson(QJsonDocument::Compact));
    }
    return {};
  }

} // namespace piazza
