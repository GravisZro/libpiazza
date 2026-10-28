// SPDX-License-Identifier: AGPL-3.0-only
//
// High-level client for Piazza's internal HTTP API. This mirrors the Python
// piazza-api PiazzaRPC class, but uses jsonrpc::Session + HttpTransport for
// request/response correlation and typed protocol structs for post data.

#pragma once

#include "error.h"
#include "protocol.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QString>
#include <QUrl>

#include <memory>
#include <optional>
#include <stdexcept>

namespace jsonrpc
{
  class Session;
}

namespace piazza
{

  enum class ApiType
  {
    logic,
    main
  };

  class RequestError : public jsonrpc::Error
  {
  public:
    explicit RequestError(const jsonrpc::Error& e)
        : jsonrpc::Error(e.object())
    {
    }
    RequestError(int code, const QString& message,
                 QJsonValue data = QJsonValue(QJsonValue::Null))
        : jsonrpc::Error(code, message, std::move(data))
    {
    }
  };

  class AuthenticationError : public std::runtime_error
  {
  public:
    explicit AuthenticationError(const QString& message)
        : std::runtime_error(message.toStdString())
    {
    }
  };

  class NotAuthenticatedError : public std::runtime_error
  {
  public:
    explicit NotAuthenticatedError(const QString& message)
        : std::runtime_error(message.toStdString())
    {
    }
  };

  class NoNetworkIdError : public std::runtime_error
  {
  public:
    explicit NoNetworkIdError(const QString& message)
        : std::runtime_error(message.toStdString())
    {
    }
  };

  class RpcClient
  {
  public:
    explicit RpcClient(QString network_id = {});
    ~RpcClient();

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;
    RpcClient(RpcClient&&) = delete;
    RpcClient& operator=(RpcClient&&) = delete;

    // ----------------------------------------------------------------------
    // Authentication / session cookies
    // ----------------------------------------------------------------------

    void user_login(const QString& email, const QString& password);
    void demo_login(const QString& auth = {}, const QUrl& url = {});

    [[nodiscard]] QMap<QString, QString> get_cookies(void) const;
    void set_cookies(const QMap<QString, QString>& cookies);

    [[nodiscard]] QString network_id(void) const { return m_network_id; }
    void set_network_id(QString network_id)
    {
      m_network_id = std::move(network_id);
    }

    // ----------------------------------------------------------------------
    // Content API
    // ----------------------------------------------------------------------

    [[nodiscard]] Post content_get(QJsonValue cid,
                                   std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue content_create(const ContentCreateParams& params);
    [[nodiscard]] QJsonValue content_update(const ContentUpdateParams& params);
    [[nodiscard]] QJsonValue
    content_instructor_answer(const ContentAnswerParams& params);
    [[nodiscard]] QJsonValue content_student_answer(QJsonValue cid,
                                                    const QString& content,
                                                    int revision = 1,
                                                    bool anon = false);
    [[nodiscard]] QJsonValue
    content_mark_duplicate(const ContentDuplicateParams& params);
    [[nodiscard]] QJsonValue
    content_mark_resolved(const ContentMarkResolvedParams& params);
    [[nodiscard]] QJsonValue content_pin(const ContentPinParams& params,
                                         bool unpin = false);
    [[nodiscard]] QJsonValue content_delete(const ContentDeleteParams& params);
    [[nodiscard]] QJsonValue
    content_add_feedback(const ContentFeedbackParams& params);
    [[nodiscard]] QJsonValue
    content_remove_feedback(const ContentFeedbackParams& params);

    // ----------------------------------------------------------------------
    // Network / users / feed API
    // ----------------------------------------------------------------------

    [[nodiscard]] QJsonValue add_students(const QList<QString>& student_emails,
                                          std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue get_all_users(std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue get_users(const QList<QString>& user_ids,
                                       std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue remove_users(const QList<QString>& user_ids,
                                          std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue get_my_feed(int limit = 150, int offset = 20,
                                         const QString& sort = QStringLiteral("updated"),
                                         std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue filter_feed(bool updated = false,
                                         bool following = false,
                                         bool folder = false,
                                         const QString& filter_folder = {},
                                         const QString& sort = QStringLiteral("updated"),
                                         std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue search(const QString& query,
                                    std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue save_draft(const QJsonObject& draft,
                                        std::optional<QString> nid = {});
    [[nodiscard]] QJsonValue get_stats(std::optional<QString> nid = {});

    // ----------------------------------------------------------------------
    // User API
    // ----------------------------------------------------------------------

    [[nodiscard]] QJsonValue get_user_profile(void);
    [[nodiscard]] QJsonValue get_user_status(void);

    // ----------------------------------------------------------------------
    // Generic escape hatch
    // ----------------------------------------------------------------------

    [[nodiscard]] QJsonValue request(const QString& method,
                                     QJsonObject params = {},
                                     ApiType api_type = ApiType::logic,
                                     const QString& nid_key = QStringLiteral("nid"));

  private:
    void ensure_authenticated(void) const;

    QString m_network_id;

    std::unique_ptr<QNetworkAccessManager> m_nam;
    std::unique_ptr<jsonrpc::Session> m_logic_session;
    std::unique_ptr<jsonrpc::Session> m_main_session;
  };

} // namespace piazza
