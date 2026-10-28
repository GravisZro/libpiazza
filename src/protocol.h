// SPDX-License-Identifier: AGPL-3.0-only
//
// Wire types for Piazza's internal HTTP API.
//
// The C++ struct fields are snake_case; the JSON wire format uses Piazza's
// snake_case field names (with the historical exception "History"). JSON
// serialization follows the same ADL `to_json` / `from_json` convention used
// by libqjsonrpc and the example MCP protocol in this repository.
//
// Encoding:  QJsonValue to_json(const T&);  or  operator QJsonValue()
// Decoding:  void from_json(const QJsonValue&, T&);  or  operator=(...)
//
// Forward compatibility: unknown JSON fields are silently dropped on read.
// Optional fields are represented with std::optional<T> or a null QString.

#pragma once

#include "json.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QObject>
#include <QString>

#include <cstdint>
#include <optional>
#include <type_traits>

namespace piazza
{

  Q_NAMESPACE

  // --------------------------------------------------------------------------
  // Endpoints / method names
  // --------------------------------------------------------------------------

  inline const QString logic_api_url = "https://piazza.com/logic/api";
  inline const QString main_api_url  = "https://piazza.com/main/api";
  inline const QString csrf_token_url = "https://piazza.com/main/csrf_token";
  inline const QString login_url = "https://piazza.com/class";
  inline const QString demo_login_url = "https://piazza.com/demo_login";

  inline const QString method_content_get = "content.get";
  inline const QString method_content_create = "content.create";
  inline const QString method_content_update = "content.update";
  inline const QString method_content_answer = "content.answer";
  inline const QString method_content_duplicate = "content.duplicate";
  inline const QString method_content_mark_resolved = "content.mark_resolved";
  inline const QString method_content_pin = "content.pin";
  inline const QString method_content_unpin = "content.unpin";
  inline const QString method_content_delete = "content.delete";
  inline const QString method_content_add_feedback = "content.add_feedback";
  inline const QString method_content_remove_feedback = "content.remove_feedback";
  inline const QString method_network_update = "network.update";
  inline const QString method_network_get_all_users = "network.get_all_users";
  inline const QString method_network_get_users = "network.get_users";
  inline const QString method_network_get_my_feed = "network.get_my_feed";
  inline const QString method_network_filter_feed = "network.filter_feed";
  inline const QString method_network_search = "network.search";
  inline const QString method_network_save_draft = "network.save_draft";
  inline const QString method_network_get_stats = "network.get_stats";
  inline const QString method_user_profile_get_profile = "user_profile.get_profile";
  inline const QString method_user_status = "user.status";
  inline const QString method_memo_get_unread_message_count = "memo.get_unread_message_count";
  inline const QString method_generic_page_event = "generic.page_event";

  // --------------------------------------------------------------------------
  // Enumerations
  // --------------------------------------------------------------------------

  enum class PostType : int
  {
    note,
    question,
    poll
  };
  Q_ENUM_NS(PostType)

  enum class PostStatus : int
  {
    active,
    inactive
  };
  Q_ENUM_NS(PostStatus)

  enum class Editor : int
  {
    rte,
    plain,
    me
  };
  Q_ENUM_NS(Editor)

  enum class ChangeType : int
  {
    create,
    followup,
    feedback,
    i_answer,
    s_answer
  };
  Q_ENUM_NS(ChangeType)

  enum class Visibility : int
  {
    all,
    private_
  };
  Q_ENUM_NS(Visibility)

  enum class EndorserRole : int
  {
    student,
    instructor
  };
  Q_ENUM_NS(EndorserRole)

  // Visibility uses "private" on the wire, which is a C++ keyword.
  [[nodiscard]] QString to_string(Visibility v) noexcept;
  [[nodiscard]] QJsonValue to_json(Visibility v);
  void from_json(const QJsonValue& j, Visibility& v);

  // --------------------------------------------------------------------------
  // Post projections
  // --------------------------------------------------------------------------

  /// The deprecated `data` object attached to posts and children. In current
  /// responses it is always `{ "embed_links": [] }`.
  struct PostData
  {
    QList<QJsonValue> embed_links = {};

    [[nodiscard]] operator QJsonValue() const;
    PostData& operator=(const QJsonValue&);
  };

  struct Change
  {
    QString anon;                 // "no" or an anonymous alias such as "stud"
    QString uid_a;                // only for anonymous users
    QString uid;
    QJsonValue data;              // only on the initial create change
    QString to;
    std::optional<Visibility> v = {};
    std::optional<ChangeType> type = {};
    QString when;
    QString cid;

    [[nodiscard]] operator QJsonValue() const;
    Change& operator=(const QJsonValue&);
  };

  struct HistoryChange
  {
    QString anon;
    QString uid_a;
    QString uid;
    QString subject;
    QString created;
    QString content;

    [[nodiscard]] operator QJsonValue() const;
    HistoryChange& operator=(const QJsonValue&);
  };

  struct PostConfiguration
  {
    std::optional<Editor> editor = {};
    QString feed_groups;
    std::optional<int> must_read_version = {};
    std::optional<int> has_emails_sent = {};
    std::optional<QJsonObject> seen = {};

    [[nodiscard]] operator QJsonValue() const;
    PostConfiguration& operator=(const QJsonValue&);
  };

  struct Endorsement
  {
    std::optional<EndorserRole> role = {};
    QString name;
    std::optional<QJsonObject> endorser = {};
    std::optional<bool> admin = {};
    QString photo;
    QString id;
    QString photo_url;
    std::optional<bool> published = {};
    std::optional<bool> us = {};
    QString facebook_id;

    [[nodiscard]] operator QJsonValue() const;
    Endorsement& operator=(const QJsonValue&);
  };

  // Feedback is defined before Followup so Followup can contain a list of it.
  // Feedback may itself contain further nested feedback posts.
  struct Feedback
  {
    QString anon;
    std::optional<QList<QString>> folders = {};
    std::optional<PostData> data = {};
    QString subject;
    QString created;
    std::optional<int> bucket_order = {};
    QString bucket_name;
    std::optional<PostType> type = {};
    std::optional<QList<Endorsement>> tag_good = {};
    QString uid;
    QList<Feedback> children = {};
    std::optional<QList<QString>> tag_good_arr = {};
    QString id;
    QString updated;
    std::optional<PostConfiguration> config = {};

    [[nodiscard]] operator QJsonValue() const;
    Feedback& operator=(const QJsonValue&);
  };

  struct Followup
  {
    QString anon;
    std::optional<QList<QString>> folders = {};
    std::optional<PostData> data = {};
    std::optional<int> no_upvotes = {};
    QString subject;
    QString created;
    std::optional<int> bucket_order = {};
    QString bucket_name;
    std::optional<PostType> type = {};
    std::optional<QList<Endorsement>> tag_good = {};
    std::optional<QList<QString>> tag_good_arr = {};
    QString uid;
    QString updated;
    std::optional<PostConfiguration> config = {};
    QList<Feedback> children = {};

    // The dictionary does not name these on followups, but real responses do.
    QString id;
    QString cid;

    [[nodiscard]] operator QJsonValue() const;
    Followup& operator=(const QJsonValue&);
  };

  struct Post
  {
    QString id;                   // required: unique thread hash
    std::optional<QList<QString>> folders = {};
    std::optional<int> nr = {};
    std::optional<PostData> data = {};
    QString created;
    std::optional<int> bucket_order = {};
    QString bucket_name;
    std::optional<int> no_answer_followup = {};
    std::optional<QList<Change>> change_log = {};
    std::optional<int> history_size = {};
    std::optional<QList<HistoryChange>> history = {}; // wire key is "History"
    std::optional<PostType> type = {};
    std::optional<QList<QString>> tags = {};
    std::optional<PostConfiguration> config = {};
    std::optional<QList<Endorsement>> tag_good = {};
    std::optional<QList<QString>> tag_good_arr = {};
    std::optional<QList<Followup>> children = {};
    std::optional<int> unique_views = {};
    QString uid;
    std::optional<PostStatus> status = {};
    std::optional<QJsonObject> drafts = {};
    std::optional<int> request_instructor = {};
    std::optional<bool> request_instructor_me = {};
    std::optional<int> bookmarked = {};
    std::optional<int> num_favorites = {};
    std::optional<bool> my_favorite = {};
    std::optional<bool> is_bookmarked = {};
    std::optional<bool> is_tag_good = {};
    std::optional<QList<QJsonObject>> q_edits = {};
    std::optional<QList<QJsonObject>> i_edits = {};
    std::optional<QList<QJsonObject>> s_edits = {};
    std::optional<int> t = {};
    std::optional<bool> default_anonymity = {};
    std::optional<bool> my_post = {};

    [[nodiscard]] operator QJsonValue() const;
    Post& operator=(const QJsonValue&);
  };

  using PiazzaPost = Post;

  // --------------------------------------------------------------------------
  // Request parameters
  //
  // `nid` is the network/class id. It is optional in these structs so callers
  // can let the RPC client fill in its default network id. On the wire the
  // client always sends the key as null when no network id is known, matching
  // the Python implementation.
  // --------------------------------------------------------------------------

  struct ContentGetParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid;
    QJsonValue student_view = QJsonValue(QJsonValue::Null);

    [[nodiscard]] operator QJsonValue() const;
    ContentGetParams& operator=(const QJsonValue&);
  };

  struct ContentConfig
  {
    std::optional<int> bypass_email = {};
    std::optional<int> is_announcement = {};
    QString feed_groups;
    std::optional<bool> schedule_later = {};
    std::optional<std::int64_t> schedule_later_time = {};
    std::optional<Editor> editor = {};
    std::optional<bool> ionly = {};

    [[nodiscard]] operator QJsonValue() const;
    ContentConfig& operator=(const QJsonValue&);
  };

  struct ContentCreateParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid;
    QString type;
    QString subject;
    QString content;
    QJsonValue folders;
    QString anonymous = "no";
    std::optional<int> revision = {};
    std::optional<ContentConfig> config = {};
    std::optional<bool> prof_override = {};
    QString draft_id;

    [[nodiscard]] operator QJsonValue() const;
    ContentCreateParams& operator=(const QJsonValue&);
  };

  struct ContentUpdateParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid;
    QString subject;

    [[nodiscard]] operator QJsonValue() const;
    ContentUpdateParams& operator=(const QJsonValue&);
  };

  struct ContentAnswerParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid;
    QString type; // "s_answer" or "i_answer"
    QString content;
    std::optional<int> revision = {};
    QString anonymous = "no";

    [[nodiscard]] operator QJsonValue() const;
    ContentAnswerParams& operator=(const QJsonValue&);
  };

  struct ContentDuplicateParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid_dupe;
    QJsonValue cid_to;
    QString msg;

    [[nodiscard]] operator QJsonValue() const;
    ContentDuplicateParams& operator=(const QJsonValue&);
  };

  struct ContentMarkResolvedParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid;
    QString resolved = "true";

    [[nodiscard]] operator QJsonValue() const;
    ContentMarkResolvedParams& operator=(const QJsonValue&);
  };

  struct ContentPinParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid;

    [[nodiscard]] operator QJsonValue() const;
    ContentPinParams& operator=(const QJsonValue&);
  };

  struct ContentDeleteParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid;

    [[nodiscard]] operator QJsonValue() const;
    ContentDeleteParams& operator=(const QJsonValue&);
  };

  struct ContentFeedbackParams
  {
    std::optional<QString> nid = {};
    QJsonValue cid;
    QString type = "tag_good";

    [[nodiscard]] operator QJsonValue() const;
    ContentFeedbackParams& operator=(const QJsonValue&);
  };

  struct NetworkUpdateParams
  {
    std::optional<QString> id = {}; // network.update expects "id", not "nid"
    QString from;
    std::optional<QList<QString>> add_students = {};
    std::optional<QList<QString>> remove_users = {};

    [[nodiscard]] operator QJsonValue() const;
    NetworkUpdateParams& operator=(const QJsonValue&);
  };

  struct NetworkGetAllUsersParams
  {
    std::optional<QString> nid = {};

    [[nodiscard]] operator QJsonValue() const;
    NetworkGetAllUsersParams& operator=(const QJsonValue&);
  };

  struct NetworkGetUsersParams
  {
    std::optional<QString> nid = {};
    QList<QString> ids;

    [[nodiscard]] operator QJsonValue() const;
    NetworkGetUsersParams& operator=(const QJsonValue&);
  };

  struct NetworkGetMyFeedParams
  {
    std::optional<QString> nid = {};
    int limit = 150;
    int offset = 20;
    QString sort = "updated";

    [[nodiscard]] operator QJsonValue() const;
    NetworkGetMyFeedParams& operator=(const QJsonValue&);
  };

  struct NetworkFilterFeedParams
  {
    std::optional<QString> nid = {};
    QString sort = "updated";
    std::optional<int> updated = {};
    std::optional<int> following = {};
    std::optional<int> folder = {};
    QString filter_folder;

    [[nodiscard]] operator QJsonValue() const;
    NetworkFilterFeedParams& operator=(const QJsonValue&);
  };

  struct NetworkSearchParams
  {
    std::optional<QString> nid = {};
    QString query;

    [[nodiscard]] operator QJsonValue() const;
    NetworkSearchParams& operator=(const QJsonValue&);
  };

  struct NetworkSaveDraftParams
  {
    std::optional<QString> nid = {};
    QJsonObject draft;

    [[nodiscard]] operator QJsonValue() const;
    NetworkSaveDraftParams& operator=(const QJsonValue&);
  };

  struct NetworkGetStatsParams
  {
    std::optional<QString> nid = {};

    [[nodiscard]] operator QJsonValue() const;
    NetworkGetStatsParams& operator=(const QJsonValue&);
  };

  struct UserProfileParams
  {
    std::optional<QString> nid = {};

    [[nodiscard]] operator QJsonValue() const;
    UserProfileParams& operator=(const QJsonValue&);
  };

  struct UserStatusParams
  {
    std::optional<QString> nid = {};

    [[nodiscard]] operator QJsonValue() const;
    UserStatusParams& operator=(const QJsonValue&);
  };

  struct GenericPageEventParams
  {
    QString type;

    [[nodiscard]] operator QJsonValue() const;
    GenericPageEventParams& operator=(const QJsonValue&);
  };


  // --------------------------------------------------------------------------
  // ADL decoders
  //
  // json.h's container decoders call from_json(elem, out) unqualified.
  // Defining one constrained template in the type's own namespace makes
  // QList<Post>, QList<Followup>, etc. work without requiring callers to
  // qualify every element conversion. The custom Visibility decoder remains
  // separate because its wire spelling needs explicit parsing.
  // --------------------------------------------------------------------------

  template <typename T, std::enable_if_t<!std::is_enum_v<T>, int> = 0>
    requires requires(T& value, const QJsonValue& j) { value = j; }
  void from_json(const QJsonValue& j, T& v)
  {
    v = j;
  }

  // --------------------------------------------------------------------------
  // to_json template
  //
  // Every protocol type provides `operator QJsonValue()`. This single ADL
  // template makes those types usable by json.h's container encoders
  // (QList<T>, std::optional<T>, ...), which call to_json(element) without
  // knowing about the concrete protocol types.
  // --------------------------------------------------------------------------

  template <typename T, std::enable_if_t<!std::is_enum_v<T>, int> = 0>
    requires requires(const T& value) { static_cast<QJsonValue>(value); }
  QJsonValue to_json(const T& v)
  {
    return static_cast<QJsonValue>(v);
  }

  // --------------------------------------------------------------------------
  // Nonce
  // --------------------------------------------------------------------------

  /// Generate the request nonce used in the `aid` query parameter. This is the
  /// C++ port of piazza-api's nonce module: millisecond timestamp in base 36
  /// followed by a random base-36 suffix.
  [[nodiscard]] QString nonce();

} // namespace piazza
