// SPDX-License-Identifier: AGPL-3.0-only
#include "rpc.h"

#include "json.h"
#include "http_transport.h"
#include "session.h"

#include <QEventLoop>
#include <QNetworkCookie>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <chrono>
#include <memory>
#include <stdexcept>
#include <utility>

namespace piazza
{

  namespace
  {
    struct HttpResult
    {
      QByteArray body;
      int status = 0;
      QNetworkReply::NetworkError error = QNetworkReply::NoError;
      QString error_string;
    };

    HttpResult wait_for_reply(QNetworkReply* reply)
    {
      HttpResult result;
      if(!reply)
      {
        result.error = QNetworkReply::UnknownNetworkError;
        result.error_string = QStringLiteral("null network reply");
        return result;
      }

      QEventLoop loop;
      QObject::connect(reply, &QNetworkReply::finished, &loop,
                       &QEventLoop::quit);
      if(!reply->isFinished())
        loop.exec();

      result.body = reply->readAll();
      result.status
          = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
      result.error = reply->error();
      result.error_string = reply->errorString();
      reply->deleteLater();
      return result;
    }

    HttpResult http_get(QNetworkAccessManager* nam, const QUrl& url)
    {
      QNetworkRequest request(url);
      request.setHeader(QNetworkRequest::UserAgentHeader,
                        QStringLiteral("libpiazza/0.1 (Qt)"));
      request.setRawHeader("Accept", "application/json");
      return wait_for_reply(nam->get(request));
    }

    HttpResult http_post_form(QNetworkAccessManager* nam, const QUrl& url,
                              const QUrlQuery& form)
    {
      QNetworkRequest request(url);
      request.setHeader(QNetworkRequest::UserAgentHeader,
                        QStringLiteral("libpiazza/0.1 (Qt)"));
      request.setHeader(QNetworkRequest::ContentTypeHeader,
                        QStringLiteral("application/x-www-form-urlencoded"));
      return wait_for_reply(
          nam->post(request, form.toString(QUrl::FullyEncoded).toUtf8()));
    }

    QString parse_csrf_token(const QString& text)
    {
      QString cleaned = text;
      cleaned.remove(QLatin1Char('"'));
      cleaned.remove(QLatin1Char(';'));
      const int equals = cleaned.indexOf(QLatin1Char('='));
      if(equals < 0)
        return {};
      int end = cleaned.indexOf(QLatin1Char('\n'), equals);
      if(end < 0)
        end = cleaned.size();
      return cleaned.mid(equals + 1, end - equals - 1).trimmed();
    }

    QString parse_login_error(const QString& text)
    {
      const int pos = text.indexOf(QStringLiteral("VAR ERROR_MSG"),
                                   Qt::CaseInsensitive);
      if(pos < 0)
        return {};
      int end = text.indexOf(QLatin1Char(';'), pos);
      if(end < 0)
        end = text.size();
      QString message = text.mid(pos, end - pos);
      message.remove(QLatin1Char('"'));
      const int equals = message.indexOf(QLatin1Char('='));
      if(equals < 0)
        return {};
      return message.mid(equals + 1).trimmed();
    }
  } // namespace

  RpcClient::RpcClient(QString network_id)
      : m_network_id(std::move(network_id)),
        m_nam(std::make_unique<QNetworkAccessManager>())
  {
    if(!m_nam->cookieJar())
      m_nam->setCookieJar(new QNetworkCookieJar(m_nam.get()));

    jsonrpc::Session::Options session_options;
    session_options.default_request_timeout = std::chrono::milliseconds(60000);

    HttpTransport::Options logic_options;
    logic_options.base_url = QUrl(logic_api_url);
    logic_options.append_method_query = true;
    m_logic_session = std::make_unique<jsonrpc::Session>(
        std::make_unique<HttpTransport>(m_nam.get(), logic_options),
        session_options);
    m_logic_session->start();

    HttpTransport::Options main_options;
    main_options.base_url = QUrl(main_api_url);
    main_options.append_method_query = false;
    m_main_session = std::make_unique<jsonrpc::Session>(
        std::make_unique<HttpTransport>(m_nam.get(), main_options),
        session_options);
    m_main_session->start();
  }

  RpcClient::~RpcClient() = default;

  void RpcClient::user_login(const QString& email, const QString& password)
  {
    const HttpResult csrf_result = http_get(m_nam.get(), QUrl(csrf_token_url));
    if(csrf_result.error != QNetworkReply::NoError)
    {
      throw AuthenticationError(
          QStringLiteral("Could not get CSRF token: ")
          + csrf_result.error_string);
    }
    if(csrf_result.status < 200 || csrf_result.status >= 300)
    {
      throw AuthenticationError(
          QStringLiteral("Could not get CSRF token: HTTP %1")
              .arg(csrf_result.status));
    }

    const QString csrf_text = QString::fromUtf8(csrf_result.body);
    if(csrf_text.indexOf(QStringLiteral("CSRF_TOKEN"), 0,
                         Qt::CaseInsensitive)
       < 0)
    {
      throw AuthenticationError(QStringLiteral("Could not get CSRF token"));
    }

    const QString csrf_token = parse_csrf_token(csrf_text);
    if(csrf_token.isEmpty())
    {
      throw AuthenticationError(QStringLiteral("Could not parse CSRF token"));
    }

    QUrlQuery form;
    form.addQueryItem(QStringLiteral("from"), QStringLiteral("/signup"));
    form.addQueryItem(QStringLiteral("email"), email);
    form.addQueryItem(QStringLiteral("password"), password);
    form.addQueryItem(QStringLiteral("remember"), QStringLiteral("on"));
    form.addQueryItem(QStringLiteral("csrf_token"), csrf_token);

    const HttpResult login_result
        = http_post_form(m_nam.get(), QUrl(login_url), form);
    if(login_result.error != QNetworkReply::NoError)
    {
      throw AuthenticationError(
          QStringLiteral("Could not authenticate: ")
          + login_result.error_string);
    }
    if(login_result.status != 200)
    {
      throw AuthenticationError(
          QStringLiteral("Could not authenticate.\nHTTP %1")
              .arg(login_result.status));
    }

    const QString login_text = QString::fromUtf8(login_result.body);
    const QString error_message = parse_login_error(login_text);
    if(!error_message.isEmpty())
    {
      throw AuthenticationError(QStringLiteral("Could not authenticate.\n")
                                + error_message);
    }
  }

  void RpcClient::demo_login(const QString& auth, const QUrl& url)
  {
    const bool has_auth = !auth.isEmpty();
    const bool has_url = url.isValid() && !url.isEmpty();
    if(has_auth == has_url)
    {
      throw std::invalid_argument(
          "demo_login requires exactly one of auth or url");
    }

    QUrl target = url;
    if(!has_url)
    {
      target = QUrl(demo_login_url);
      QUrlQuery query;
      query.addQueryItem(QStringLiteral("nid"), m_network_id);
      query.addQueryItem(QStringLiteral("auth"), auth);
      target.setQuery(query);
    }

    const HttpResult result = http_get(m_nam.get(), target);
    if(result.error != QNetworkReply::NoError)
    {
      throw AuthenticationError(
          QStringLiteral("Could not authenticate: ") + result.error_string);
    }
    if(result.status < 200 || result.status >= 300)
    {
      throw AuthenticationError(
          QStringLiteral("Could not authenticate.\nHTTP %1").arg(result.status));
    }
  }

  QMap<QString, QString> RpcClient::get_cookies(void) const
  {
    QMap<QString, QString> result;
    QNetworkCookieJar* jar = m_nam->cookieJar();
    if(!jar)
      return result;

    const QList<QUrl> urls = {
      QUrl(QStringLiteral("https://piazza.com")),
      QUrl(QStringLiteral("https://piazza.com/class")),
      QUrl(QStringLiteral("https://piazza.com/logic/api")),
    };
    for(const QUrl& url : urls)
    {
      for(const QNetworkCookie& cookie : jar->cookiesForUrl(url))
      {
        result.insert(QString::fromUtf8(cookie.name()),
                      QString::fromUtf8(cookie.value()));
      }
    }
    return result;
  }

  void RpcClient::set_cookies(const QMap<QString, QString>& cookies)
  {
    QNetworkCookieJar* jar = m_nam->cookieJar();
    if(!jar)
    {
      jar = new QNetworkCookieJar(m_nam.get());
      m_nam->setCookieJar(jar);
    }

    QList<QNetworkCookie> list;
    for(auto it = cookies.constBegin(); it != cookies.constEnd(); ++it)
    {
      QNetworkCookie cookie(it.key().toUtf8(), it.value().toUtf8());
      cookie.setDomain(QStringLiteral("piazza.com"));
      cookie.setPath(QStringLiteral("/"));
      list.append(cookie);
    }
    if(!list.isEmpty())
    {
      jar->setCookiesFromUrl(list, QUrl(QStringLiteral("https://piazza.com")));
      jar->setCookiesFromUrl(list, QUrl(QStringLiteral("https://piazza.com/class")));
    }
  }

  void RpcClient::ensure_authenticated(void) const
  {
    if(get_cookies().isEmpty())
    {
      throw NotAuthenticatedError(
          QStringLiteral("You must authenticate before making any other "
                         "requests."));
    }
  }

  QJsonValue RpcClient::request(const QString& method, QJsonObject params,
                                ApiType api_type, const QString& nid_key,
                                bool include_nid)
  {
    ensure_authenticated();

    if(include_nid)
    {
      const QJsonValue existing = params.value(nid_key);
      if(existing.isUndefined() || existing.isNull())
      {
        if(!m_network_id.isEmpty())
          params.insert(nid_key, m_network_id);
        else
          params.insert(nid_key, QJsonValue(QJsonValue::Null));
      }
    }

    jsonrpc::Session* session
        = api_type == ApiType::main ? m_main_session.get() : m_logic_session.get();
    try
    {
      return session->send_request(method, params).get();
    }
    catch(const jsonrpc::Error& e)
    {
      throw RequestError(e);
    }
  }

  Post RpcClient::content_get(QJsonValue cid, std::optional<QString> nid)
  {
    ContentGetParams params;
    params.nid = std::move(nid);
    params.cid = std::move(cid);
    const QJsonValue result
        = request(method_content_get, static_cast<QJsonValue>(params).toObject());
    Post post;
    jsonrpc::from_json(result, post);
    return post;
  }

  QJsonValue RpcClient::content_create(const ContentCreateParams& params)
  {
    return request(method_content_create,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::content_update(const ContentUpdateParams& params)
  {
    return request(method_content_update,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::content_instructor_answer(
      const ContentAnswerParams& params)
  {
    return request(method_content_answer,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::content_student_answer(QJsonValue cid,
                                               const QString& content,
                                               int revision, bool anon)
  {
    ContentAnswerParams params;
    params.cid = std::move(cid);
    params.type = QStringLiteral("s_answer");
    params.content = content;
    params.revision = revision;
    params.anonymous = anon ? QStringLiteral("stud") : QStringLiteral("no");
    return request(method_content_answer,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue
  RpcClient::content_mark_duplicate(const ContentDuplicateParams& params)
  {
    return request(method_content_duplicate,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue
  RpcClient::content_mark_resolved(const ContentMarkResolvedParams& params)
  {
    return request(method_content_mark_resolved,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::content_pin(const ContentPinParams& params, bool unpin)
  {
    return request(unpin ? method_content_unpin : method_content_pin,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::content_delete(const ContentDeleteParams& params)
  {
    return request(method_content_delete,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::content_add_feedback(
      const ContentFeedbackParams& params)
  {
    return request(method_content_add_feedback,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::content_remove_feedback(
      const ContentFeedbackParams& params)
  {
    return request(method_content_remove_feedback,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::add_students(const QList<QString>& student_emails,
                                     std::optional<QString> nid)
  {
    NetworkUpdateParams params;
    params.id = std::move(nid);
    params.from = QStringLiteral("ClassSettingsPage");
    params.add_students = student_emails;
    return request(method_network_update,
                   static_cast<QJsonValue>(params).toObject(),
                   ApiType::logic, QStringLiteral("id"));
  }

  QJsonValue RpcClient::get_all_users(std::optional<QString> nid)
  {
    NetworkGetAllUsersParams params;
    params.nid = std::move(nid);
    return request(method_network_get_all_users,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::get_users(const QList<QString>& user_ids,
                                  std::optional<QString> nid)
  {
    NetworkGetUsersParams params;
    params.nid = std::move(nid);
    params.ids = user_ids;
    return request(method_network_get_users,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::remove_users(const QList<QString>& user_ids,
                                     std::optional<QString> nid)
  {
    NetworkUpdateParams params;
    params.id = std::move(nid);
    params.remove_users = user_ids;
    return request(method_network_update,
                   static_cast<QJsonValue>(params).toObject(),
                   ApiType::logic, QStringLiteral("id"));
  }

  QJsonValue RpcClient::get_my_feed(int limit, int offset, const QString& sort,
                                    std::optional<QString> nid)
  {
    NetworkGetMyFeedParams params;
    params.nid = std::move(nid);
    params.limit = limit;
    params.offset = offset;
    params.sort = sort;
    return request(method_network_get_my_feed,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::filter_feed(bool updated, bool following, bool folder,
                                    const QString& filter_folder,
                                    const QString& sort,
                                    std::optional<QString> nid)
  {
    const int selected = static_cast<int>(updated)
                         + static_cast<int>(following)
                         + static_cast<int>(folder);
    if(selected != 1)
    {
      throw std::invalid_argument(
          "filter_feed requires exactly one of updated, following, or folder");
    }
    if(folder && filter_folder.isEmpty())
    {
      throw std::invalid_argument(
          "filter_feed folder=true requires filter_folder");
    }

    NetworkFilterFeedParams params;
    params.nid = std::move(nid);
    params.sort = sort;
    if(updated)
      params.updated = 1;
    else if(following)
      params.following = 1;
    else
    {
      params.folder = 1;
      params.filter_folder = filter_folder;
    }
    return request(method_network_filter_feed,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::search(const QString& query, std::optional<QString> nid)
  {
    NetworkSearchParams params;
    params.nid = std::move(nid);
    params.query = query;
    return request(method_network_search,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::save_draft(const QJsonObject& draft,
                                   std::optional<QString> nid)
  {
    NetworkSaveDraftParams params;
    params.nid = std::move(nid);
    params.draft = draft;
    return request(method_network_save_draft,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::get_stats(std::optional<QString> nid)
  {
    NetworkGetStatsParams params;
    params.nid = std::move(nid);
    return request(method_network_get_stats,
                   static_cast<QJsonValue>(params).toObject(), ApiType::main);
  }

  QJsonValue RpcClient::get_user_profile(void)
  {
    UserProfileParams params;
    return request(method_user_profile_get_profile,
                   static_cast<QJsonValue>(params).toObject());
  }

  QJsonValue RpcClient::get_user_status(void)
  {
    UserStatusParams params;
    return request(method_user_status,
                   static_cast<QJsonValue>(params).toObject());
  }

  int RpcClient::get_unread_message_count(void)
  {
    const QJsonValue result
        = request(method_memo_get_unread_message_count, QJsonObject{},
                  ApiType::logic, QStringLiteral("nid"), false);
    return jsonrpc::from_json<int>(result);
  }

  QString RpcClient::page_event(const QString& type)
  {
    GenericPageEventParams params;
    params.type = type;
    const QJsonValue result
        = request(method_generic_page_event,
                  static_cast<QJsonValue>(params).toObject(), ApiType::logic,
                  QStringLiteral("nid"), false);
    return jsonrpc::from_json<QString>(result);
  }

} // namespace piazza
