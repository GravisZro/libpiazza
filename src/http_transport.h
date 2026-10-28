// SPDX-License-Identifier: AGPL-3.0-only
//
// HTTP transport for Piazza's JSON-RPC-like API.
//
// libqjsonrpc's Session speaks standard JSON-RPC 2.0 frames. Piazza's wire
// protocol is close, but not identical:
//
//   request:  POST <base>[?method=<method>&aid=<nonce>]
//             body: {"method": "...", "params": {...}}
//   response: {"result": ...}  or  {"error": ...}
//
// This transport adapts both directions. Outbound JSON-RPC requests are
// rewritten into Piazza's request shape; inbound Piazza responses are wrapped
// in a JSON-RPC 2.0 response carrying the original request id. That lets the
// rest of the client use jsonrpc::Session's request/response correlation,
// timeouts, and typed result conversion.
//
// The transport is synchronous from the caller's point of view: send() blocks
// on a local Qt event loop until the HTTP exchange completes, then delivers
// the response through on_message() before returning. Callers must therefore
// have a QCoreApplication instance and should not call send() from a Qt
// network thread that cannot nest event loops.

#pragma once

#include "transport.h"

#include <QByteArray>
#include <QByteArrayView>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QString>
#include <QUrl>

#include <mutex>
#include <system_error>

namespace piazza
{

  class HttpTransport final : public jsonrpc::Transport
  {
  public:
    struct Options
    {
      /// Endpoint used for every request handled by this transport.
      QUrl base_url = QUrl("https://piazza.com/logic/api");
      /// The logic API identifies the method in the query string; the main
      /// API does not. Set false for main-api transports.
      bool append_method_query = true;
      /// Cookie carrying the CSRF token value used in the request header.
      QString csrf_cookie_name = "session_id";
      /// Header name used to send the CSRF token.
      QString csrf_header_name = "CSRF-Token";
      /// Optional User-Agent override.
      QString user_agent = "libpiazza/0.1 (Qt)";
    };

    explicit HttpTransport(QNetworkAccessManager* nam);
    HttpTransport(QNetworkAccessManager* nam, Options opts);
    ~HttpTransport() override;

    HttpTransport(const HttpTransport&) = delete;
    HttpTransport& operator=(const HttpTransport&) = delete;

    void on_message(MessageCallback cb) override;
    void on_error(ErrorCallback cb) override;
    void on_close(CloseCallback cb) override;

    void start(void) override;
    void close(void) override;

    [[nodiscard]] std::error_code send(QByteArrayView frame) override;
    [[nodiscard]] bool is_open(void) const noexcept override;

    [[nodiscard]] const QUrl& base_url(void) const noexcept
    {
      return m_opts.base_url;
    }

  private:
    [[nodiscard]] QUrl build_url(const QString& method) const;
    [[nodiscard]] QString csrf_token(void) const;

    QNetworkAccessManager* m_nam = nullptr;
    Options m_opts;

    bool m_running = false;
    bool m_closed = false;

    MessageCallback m_on_message;
    ErrorCallback m_on_error;
    CloseCallback m_on_close;

    mutable std::mutex m_mu;
    QSet<QNetworkReply*> m_replies;
  };

} // namespace piazza
