// SPDX-License-Identifier: AGPL-3.0-only
#include "protocol.h"

#include "error.h"
#include "json.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QRandomGenerator>
#include <QString>

#include <cstdint>
#include <optional>

namespace piazza
{

  // =====================================================================
  // Visibility
  // =====================================================================

  QString to_string(Visibility v) noexcept
  {
    return v == Visibility::private_ ? "private"
                                     : "all";
  }

  QJsonValue to_json(Visibility v) { return QJsonValue(to_string(v)); }

  void from_json(const QJsonValue& j, Visibility& v)
  {
    if(!j.isString())
    {
      throw jsonrpc::Error(jsonrpc::error_code::parse_error,
                           "expected a visibility string");
    }
    const QString s = j.toString();
    if(s == QLatin1String("all"))
    {
      v = Visibility::all;
      return;
    }
    if(s == QLatin1String("private"))
    {
      v = Visibility::private_;
      return;
    }
    throw jsonrpc::Error(jsonrpc::error_code::parse_error,
                         QString("unknown visibility: ") + s);
  }

  // =====================================================================
  // PostData
  // =====================================================================

  PostData::operator QJsonValue() const
  {
    QJsonObject o;
    o.insert("embed_links", jsonrpc::to_json(embed_links));
    return o;
  }

  PostData& PostData::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    if(const auto it = o.constFind("embed_links");
       it != o.constEnd() && !it->isNull())
    {
      embed_links = jsonrpc::from_json<QList<QJsonValue>>(*it);
    }
    else
    {
      embed_links.clear();
    }
    return *this;
  }

  // =====================================================================
  // Change
  // =====================================================================

  Change::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "anon", anon);
    jsonrpc::put_optional_string(o, "uid_a", uid_a);
    jsonrpc::put_optional_string(o, "uid", uid);
    jsonrpc::put_optional_json(o, "data", data);
    jsonrpc::put_optional_string(o, "to", to);
    if(v.has_value())
      o.insert("v", piazza::to_json(*v));
    jsonrpc::put_optional(o, "type", type);
    jsonrpc::put_optional_string(o, "when", when);
    jsonrpc::put_optional_string(o, "cid", cid);
    return o;
  }

  Change& Change::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "anon", anon);
    jsonrpc::take_optional_string(o, "uid_a", uid_a);
    jsonrpc::take_optional_string(o, "uid", uid);
    jsonrpc::take_optional_json(o, "data", data);
    jsonrpc::take_optional_string(o, "to", to);
    v.reset();
    if(const auto it = o.constFind("v");
       it != o.constEnd() && !it->isNull())
    {
      Visibility tmp;
      piazza::from_json(*it, tmp);
      v = tmp;
    }
    jsonrpc::take_optional<ChangeType>(o, "type", type);
    jsonrpc::take_optional_string(o, "when", when);
    jsonrpc::take_optional_string(o, "cid", cid);
    return *this;
  }

  // =====================================================================
  // HistoryChange
  // =====================================================================

  HistoryChange::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "anon", anon);
    jsonrpc::put_optional_string(o, "uid_a", uid_a);
    jsonrpc::put_optional_string(o, "uid", uid);
    jsonrpc::put_optional_string(o, "subject", subject);
    jsonrpc::put_optional_string(o, "created", created);
    jsonrpc::put_optional_string(o, "content", content);
    return o;
  }

  HistoryChange& HistoryChange::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "anon", anon);
    jsonrpc::take_optional_string(o, "uid_a", uid_a);
    jsonrpc::take_optional_string(o, "uid", uid);
    jsonrpc::take_optional_string(o, "subject", subject);
    jsonrpc::take_optional_string(o, "created", created);
    jsonrpc::take_optional_string(o, "content", content);
    return *this;
  }

  // =====================================================================
  // PostConfiguration
  // =====================================================================

  PostConfiguration::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional(o, "editor", editor);
    jsonrpc::put_optional_string(o, "feed_groups", feed_groups);
    jsonrpc::put_optional(o, "must_read_version", must_read_version);
    jsonrpc::put_optional(o, "has_emails_sent", has_emails_sent);
    jsonrpc::put_optional(o, "seen", seen);
    return o;
  }

  PostConfiguration& PostConfiguration::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional<Editor>(o, "editor", editor);
    jsonrpc::take_optional_string(o, "feed_groups", feed_groups);
    jsonrpc::take_optional<int>(o, "must_read_version", must_read_version);
    jsonrpc::take_optional<int>(o, "has_emails_sent", has_emails_sent);
    jsonrpc::take_optional<QJsonObject>(o, "seen", seen);
    return *this;
  }

  // =====================================================================
  // Endorsement
  // =====================================================================

  Endorsement::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional(o, "role", role);
    jsonrpc::put_optional_string(o, "name", name);
    jsonrpc::put_optional(o, "endorser", endorser);
    jsonrpc::put_optional(o, "admin", admin);
    jsonrpc::put_optional_string(o, "photo", photo);
    jsonrpc::put_optional_string(o, "id", id);
    jsonrpc::put_optional_string(o, "photo_url", photo_url);
    jsonrpc::put_optional(o, "published", published);
    jsonrpc::put_optional(o, "us", us);
    jsonrpc::put_optional_string(o, "facebook_id", facebook_id);
    return o;
  }

  Endorsement& Endorsement::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional<EndorserRole>(o, "role", role);
    jsonrpc::take_optional_string(o, "name", name);
    jsonrpc::take_optional<QJsonObject>(o, "endorser", endorser);
    jsonrpc::take_optional<bool>(o, "admin", admin);
    jsonrpc::take_optional_string(o, "photo", photo);
    jsonrpc::take_optional_string(o, "id", id);
    jsonrpc::take_optional_string(o, "photo_url", photo_url);
    jsonrpc::take_optional<bool>(o, "published", published);
    jsonrpc::take_optional<bool>(o, "us", us);
    jsonrpc::take_optional_string(o, "facebook_id", facebook_id);
    return *this;
  }

  // =====================================================================
  // Feedback
  // =====================================================================

  Feedback::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "anon", anon);
    jsonrpc::put_optional(o, "folders", folders);
    jsonrpc::put_optional(o, "data", data);
    jsonrpc::put_optional_string(o, "subject", subject);
    jsonrpc::put_optional_string(o, "created", created);
    jsonrpc::put_optional(o, "bucket_order", bucket_order);
    jsonrpc::put_optional_string(o, "bucket_name", bucket_name);
    jsonrpc::put_optional(o, "type", type);
    jsonrpc::put_optional(o, "tag_good", tag_good);
    jsonrpc::put_optional_string(o, "uid", uid);
    o.insert("children", jsonrpc::to_json(children));
    jsonrpc::put_optional(o, "tag_good_arr", tag_good_arr);
    jsonrpc::put_optional_string(o, "id", id);
    jsonrpc::put_optional_string(o, "updated", updated);
    jsonrpc::put_optional(o, "config", config);
    return o;
  }

  Feedback& Feedback::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "anon", anon);
    jsonrpc::take_optional<QList<QString>>(o, "folders", folders);
    jsonrpc::take_optional<PostData>(o, "data", data);
    jsonrpc::take_optional_string(o, "subject", subject);
    jsonrpc::take_optional_string(o, "created", created);
    jsonrpc::take_optional<int>(o, "bucket_order", bucket_order);
    jsonrpc::take_optional_string(o, "bucket_name", bucket_name);
    jsonrpc::take_optional<PostType>(o, "type", type);
    jsonrpc::take_optional<QList<Endorsement>>(o, "tag_good", tag_good);
    jsonrpc::take_optional_string(o, "uid", uid);
    if(const auto it = o.constFind("children");
       it != o.constEnd() && !it->isNull())
    {
      children = jsonrpc::from_json<QList<Feedback>>(*it);
    }
    else
    {
      children.clear();
    }
    jsonrpc::take_optional<QList<QString>>(o, "tag_good_arr", tag_good_arr);
    jsonrpc::take_optional_string(o, "id", id);
    jsonrpc::take_optional_string(o, "updated", updated);
    jsonrpc::take_optional<PostConfiguration>(o, "config", config);
    return *this;
  }

  // =====================================================================
  // Followup
  // =====================================================================

  Followup::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "anon", anon);
    jsonrpc::put_optional(o, "folders", folders);
    jsonrpc::put_optional(o, "data", data);
    jsonrpc::put_optional(o, "no_upvotes", no_upvotes);
    jsonrpc::put_optional_string(o, "subject", subject);
    jsonrpc::put_optional_string(o, "created", created);
    jsonrpc::put_optional(o, "bucket_order", bucket_order);
    jsonrpc::put_optional_string(o, "bucket_name", bucket_name);
    jsonrpc::put_optional(o, "type", type);
    jsonrpc::put_optional(o, "tag_good", tag_good);
    jsonrpc::put_optional(o, "tag_good_arr", tag_good_arr);
    jsonrpc::put_optional_string(o, "uid", uid);
    jsonrpc::put_optional_string(o, "updated", updated);
    jsonrpc::put_optional(o, "config", config);
    o.insert("children", jsonrpc::to_json(children));
    jsonrpc::put_optional_string(o, "id", id);
    jsonrpc::put_optional_string(o, "cid", cid);
    return o;
  }

  Followup& Followup::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "anon", anon);
    jsonrpc::take_optional<QList<QString>>(o, "folders", folders);
    jsonrpc::take_optional<PostData>(o, "data", data);
    jsonrpc::take_optional<int>(o, "no_upvotes", no_upvotes);
    jsonrpc::take_optional_string(o, "subject", subject);
    jsonrpc::take_optional_string(o, "created", created);
    jsonrpc::take_optional<int>(o, "bucket_order", bucket_order);
    jsonrpc::take_optional_string(o, "bucket_name", bucket_name);
    jsonrpc::take_optional<PostType>(o, "type", type);
    jsonrpc::take_optional<QList<Endorsement>>(o, "tag_good", tag_good);
    jsonrpc::take_optional<QList<QString>>(o, "tag_good_arr", tag_good_arr);
    jsonrpc::take_optional_string(o, "uid", uid);
    jsonrpc::take_optional_string(o, "updated", updated);
    jsonrpc::take_optional<PostConfiguration>(o, "config", config);
    if(const auto it = o.constFind("children");
       it != o.constEnd() && !it->isNull())
    {
      children = jsonrpc::from_json<QList<Feedback>>(*it);
    }
    else
    {
      children.clear();
    }
    jsonrpc::take_optional_string(o, "id", id);
    jsonrpc::take_optional_string(o, "cid", cid);
    return *this;
  }

  // =====================================================================
  // Post
  // =====================================================================

  Post::operator QJsonValue() const
  {
    QJsonObject o;
    o.insert("id", jsonrpc::to_json(id));
    jsonrpc::put_optional(o, "folders", folders);
    jsonrpc::put_optional(o, "nr", nr);
    jsonrpc::put_optional(o, "data", data);
    jsonrpc::put_optional_string(o, "created", created);
    jsonrpc::put_optional(o, "bucket_order", bucket_order);
    jsonrpc::put_optional_string(o, "bucket_name", bucket_name);
    jsonrpc::put_optional(o, "no_answer_followup", no_answer_followup);
    jsonrpc::put_optional(o, "change_log", change_log);
    jsonrpc::put_optional(o, "history_size", history_size);
    jsonrpc::put_optional(o, "History", history);
    jsonrpc::put_optional(o, "type", type);
    jsonrpc::put_optional(o, "tags", tags);
    jsonrpc::put_optional(o, "config", config);
    jsonrpc::put_optional(o, "tag_good", tag_good);
    jsonrpc::put_optional(o, "tag_good_arr", tag_good_arr);
    jsonrpc::put_optional(o, "children", children);
    jsonrpc::put_optional(o, "unique_views", unique_views);
    jsonrpc::put_optional_string(o, "uid", uid);
    jsonrpc::put_optional(o, "status", status);
    jsonrpc::put_optional(o, "drafts", drafts);
    jsonrpc::put_optional(o, "request_instructor", request_instructor);
    jsonrpc::put_optional(o, "request_instructor_me", request_instructor_me);
    jsonrpc::put_optional(o, "bookmarked", bookmarked);
    jsonrpc::put_optional(o, "num_favorites", num_favorites);
    jsonrpc::put_optional(o, "my_favorite", my_favorite);
    jsonrpc::put_optional(o, "is_bookmarked", is_bookmarked);
    jsonrpc::put_optional(o, "is_tag_good", is_tag_good);
    jsonrpc::put_optional(o, "q_edits", q_edits);
    jsonrpc::put_optional(o, "i_edits", i_edits);
    jsonrpc::put_optional(o, "s_edits", s_edits);
    jsonrpc::put_optional(o, "t", t);
    jsonrpc::put_optional(o, "default_anonymity", default_anonymity);
    jsonrpc::put_optional(o, "my_post", my_post);
    return o;
  }

  Post& Post::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    id = jsonrpc::require<QString>(o, "id");
    jsonrpc::take_optional<QList<QString>>(o, "folders", folders);
    jsonrpc::take_optional<int>(o, "nr", nr);
    jsonrpc::take_optional<PostData>(o, "data", data);
    jsonrpc::take_optional_string(o, "created", created);
    jsonrpc::take_optional<int>(o, "bucket_order", bucket_order);
    jsonrpc::take_optional_string(o, "bucket_name", bucket_name);
    jsonrpc::take_optional<int>(o, "no_answer_followup", no_answer_followup);
    jsonrpc::take_optional<QList<Change>>(o, "change_log", change_log);
    jsonrpc::take_optional<int>(o, "history_size", history_size);
    jsonrpc::take_optional<QList<HistoryChange>>(o, "History", history);
    jsonrpc::take_optional<PostType>(o, "type", type);
    jsonrpc::take_optional<QList<QString>>(o, "tags", tags);
    jsonrpc::take_optional<PostConfiguration>(o, "config", config);
    jsonrpc::take_optional<QList<Endorsement>>(o, "tag_good", tag_good);
    jsonrpc::take_optional<QList<QString>>(o, "tag_good_arr", tag_good_arr);
    jsonrpc::take_optional<QList<Followup>>(o, "children", children);
    jsonrpc::take_optional<int>(o, "unique_views", unique_views);
    jsonrpc::take_optional_string(o, "uid", uid);
    jsonrpc::take_optional<PostStatus>(o, "status", status);
    jsonrpc::take_optional<QJsonObject>(o, "drafts", drafts);
    jsonrpc::take_optional<int>(o, "request_instructor", request_instructor);
    jsonrpc::take_optional<bool>(o, "request_instructor_me",
                                 request_instructor_me);
    jsonrpc::take_optional<int>(o, "bookmarked", bookmarked);
    jsonrpc::take_optional<int>(o, "num_favorites", num_favorites);
    jsonrpc::take_optional<bool>(o, "my_favorite", my_favorite);
    jsonrpc::take_optional<bool>(o, "is_bookmarked", is_bookmarked);
    jsonrpc::take_optional<bool>(o, "is_tag_good", is_tag_good);
    jsonrpc::take_optional<QList<QJsonObject>>(o, "q_edits", q_edits);
    jsonrpc::take_optional<QList<QJsonObject>>(o, "i_edits", i_edits);
    jsonrpc::take_optional<QList<QJsonObject>>(o, "s_edits", s_edits);
    jsonrpc::take_optional<int>(o, "t", t);
    jsonrpc::take_optional<bool>(o, "default_anonymity", default_anonymity);
    jsonrpc::take_optional<bool>(o, "my_post", my_post);
    return *this;
  }

  // =====================================================================
  // Request parameters
  // =====================================================================

  ContentGetParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("cid", cid);
    o.insert("student_view", student_view);
    return o;
  }

  ContentGetParams& ContentGetParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid = o.value("cid");
    student_view = o.value("student_view");
    return *this;
  }

  ContentConfig::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional(o, "bypass_email", bypass_email);
    jsonrpc::put_optional(o, "is_announcement", is_announcement);
    jsonrpc::put_optional_string(o, "feed_groups", feed_groups);
    jsonrpc::put_optional(o, "schedule_later", schedule_later);
    jsonrpc::put_optional(o, "schedule_later_time", schedule_later_time);
    jsonrpc::put_optional(o, "editor", editor);
    jsonrpc::put_optional(o, "ionly", ionly);
    return o;
  }

  ContentConfig& ContentConfig::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional<int>(o, "bypass_email", bypass_email);
    jsonrpc::take_optional<int>(o, "is_announcement", is_announcement);
    jsonrpc::take_optional_string(o, "feed_groups", feed_groups);
    jsonrpc::take_optional<bool>(o, "schedule_later", schedule_later);
    jsonrpc::take_optional<std::int64_t>(o, "schedule_later_time",
                                         schedule_later_time);
    jsonrpc::take_optional<Editor>(o, "editor", editor);
    jsonrpc::take_optional<bool>(o, "ionly", ionly);
    return *this;
  }

  ContentCreateParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    jsonrpc::put_optional_json(o, "cid", cid);
    jsonrpc::put_optional_string(o, "type", type);
    jsonrpc::put_optional_string(o, "subject", subject);
    jsonrpc::put_optional_string(o, "content", content);
    jsonrpc::put_optional_json(o, "folders", folders);
    jsonrpc::put_optional_string(o, "anonymous", anonymous);
    jsonrpc::put_optional(o, "revision", revision);
    jsonrpc::put_optional(o, "config", config);
    jsonrpc::put_optional(o, "prof_override", prof_override);
    jsonrpc::put_optional_string(o, "draftId", draft_id);
    return o;
  }

  ContentCreateParams& ContentCreateParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid = o.value("cid");
    jsonrpc::take_optional_string(o, "type", type);
    jsonrpc::take_optional_string(o, "subject", subject);
    jsonrpc::take_optional_string(o, "content", content);
    folders = o.value("folders");
    jsonrpc::take_optional_string(o, "anonymous", anonymous);
    jsonrpc::take_optional<int>(o, "revision", revision);
    jsonrpc::take_optional<ContentConfig>(o, "config", config);
    jsonrpc::take_optional<bool>(o, "prof_override", prof_override);
    jsonrpc::take_optional_string(o, "draftId", draft_id);
    return *this;
  }

  ContentUpdateParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("cid", cid);
    o.insert("subject", jsonrpc::to_json(subject));
    return o;
  }

  ContentUpdateParams& ContentUpdateParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid = o.value("cid");
    jsonrpc::take_optional_string(o, "subject", subject);
    return *this;
  }

  ContentAnswerParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("cid", cid);
    o.insert("type", jsonrpc::to_json(type));
    o.insert("content", jsonrpc::to_json(content));
    jsonrpc::put_optional(o, "revision", revision);
    o.insert("anonymous", jsonrpc::to_json(anonymous));
    return o;
  }

  ContentAnswerParams& ContentAnswerParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid = o.value("cid");
    jsonrpc::take_optional_string(o, "type", type);
    jsonrpc::take_optional_string(o, "content", content);
    jsonrpc::take_optional<int>(o, "revision", revision);
    jsonrpc::take_optional_string(o, "anonymous", anonymous);
    return *this;
  }

  ContentDuplicateParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("cid_dupe", cid_dupe);
    o.insert("cid_to", cid_to);
    o.insert("msg", jsonrpc::to_json(msg));
    return o;
  }

  ContentDuplicateParams&
  ContentDuplicateParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid_dupe = o.value("cid_dupe");
    cid_to = o.value("cid_to");
    jsonrpc::take_optional_string(o, "msg", msg);
    return *this;
  }

  ContentMarkResolvedParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("cid", cid);
    o.insert("resolved", jsonrpc::to_json(resolved));
    return o;
  }

  ContentMarkResolvedParams&
  ContentMarkResolvedParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid = o.value("cid");
    jsonrpc::take_optional_string(o, "resolved", resolved);
    return *this;
  }

  ContentPinParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("cid", cid);
    return o;
  }

  ContentPinParams& ContentPinParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid = o.value("cid");
    return *this;
  }

  ContentDeleteParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("cid", cid);
    return o;
  }

  ContentDeleteParams& ContentDeleteParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid = o.value("cid");
    return *this;
  }

  ContentFeedbackParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("cid", cid);
    o.insert("type", jsonrpc::to_json(type));
    return o;
  }

  ContentFeedbackParams& ContentFeedbackParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    cid = o.value("cid");
    jsonrpc::take_optional_string(o, "type", type);
    return *this;
  }

  NetworkUpdateParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "id", id);
    o.insert("from", jsonrpc::to_json(from));
    jsonrpc::put_optional(o, "add_students", add_students);
    jsonrpc::put_optional(o, "remove_users", remove_users);
    return o;
  }

  NetworkUpdateParams& NetworkUpdateParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "id", id);
    jsonrpc::take_optional_string(o, "from", from);
    jsonrpc::take_optional<QList<QString>>(o, "add_students", add_students);
    jsonrpc::take_optional<QList<QString>>(o, "remove_users", remove_users);
    return *this;
  }

  NetworkGetAllUsersParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    return o;
  }

  NetworkGetAllUsersParams&
  NetworkGetAllUsersParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    return *this;
  }

  NetworkGetUsersParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("ids", jsonrpc::to_json(ids));
    return o;
  }

  NetworkGetUsersParams& NetworkGetUsersParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    ids = jsonrpc::require<QList<QString>>(o, "ids");
    return *this;
  }

  NetworkGetMyFeedParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("limit", jsonrpc::to_json(limit));
    o.insert("offset", jsonrpc::to_json(offset));
    o.insert("sort", jsonrpc::to_json(sort));
    return o;
  }

  NetworkGetMyFeedParams&
  NetworkGetMyFeedParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    limit = jsonrpc::require<int>(o, "limit");
    offset = jsonrpc::require<int>(o, "offset");
    jsonrpc::take_optional_string(o, "sort", sort);
    return *this;
  }

  NetworkFilterFeedParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("sort", jsonrpc::to_json(sort));
    jsonrpc::put_optional(o, "updated", updated);
    jsonrpc::put_optional(o, "following", following);
    jsonrpc::put_optional(o, "folder", folder);
    jsonrpc::put_optional_string(o, "filter_folder", filter_folder);
    return o;
  }

  NetworkFilterFeedParams&
  NetworkFilterFeedParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    jsonrpc::take_optional_string(o, "sort", sort);
    jsonrpc::take_optional<int>(o, "updated", updated);
    jsonrpc::take_optional<int>(o, "following", following);
    jsonrpc::take_optional<int>(o, "folder", folder);
    jsonrpc::take_optional_string(o, "filter_folder", filter_folder);
    return *this;
  }

  NetworkSearchParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("query", jsonrpc::to_json(query));
    return o;
  }

  NetworkSearchParams& NetworkSearchParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    jsonrpc::take_optional_string(o, "query", query);
    return *this;
  }

  NetworkSaveDraftParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    o.insert("draft", jsonrpc::to_json(draft));
    return o;
  }

  NetworkSaveDraftParams&
  NetworkSaveDraftParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    draft = jsonrpc::require<QJsonObject>(o, "draft");
    return *this;
  }

  NetworkGetStatsParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    return o;
  }

  NetworkGetStatsParams&
  NetworkGetStatsParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    return *this;
  }

  UserProfileParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    return o;
  }

  UserProfileParams& UserProfileParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    return *this;
  }

  UserStatusParams::operator QJsonValue() const
  {
    QJsonObject o;
    jsonrpc::put_optional_string(o, "nid", nid);
    return o;
  }

  UserStatusParams& UserStatusParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "nid", nid);
    return *this;
  }

  GenericPageEventParams::operator QJsonValue() const
  {
    QJsonObject o;
    o.insert("type", jsonrpc::to_json(type));
    return o;
  }

  GenericPageEventParams&
  GenericPageEventParams::operator=(const QJsonValue& j)
  {
    const QJsonObject o = jsonrpc::as_object(j);
    jsonrpc::take_optional_string(o, "type", type);
    return *this;
  }

  // =====================================================================
  // Nonce
  // =====================================================================

  namespace
  {
    constexpr char kBase36Digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

    QString int_to_base(std::uint64_t x, int base)
    {
      if(x == 0)
        return "0";
      QString out;
      while(x != 0)
      {
        out.prepend(QLatin1Char(kBase36Digits[x % static_cast<std::uint64_t>(base)]));
        x /= static_cast<std::uint64_t>(base);
      }
      return out;
    }
  } // namespace

  QString nonce()
  {
    const qint64 ms = QDateTime::currentMSecsSinceEpoch();
    const quint32 suffix = QRandomGenerator::global()->bounded(1679617u);
    return int_to_base(static_cast<std::uint64_t>(ms), 36)
           + int_to_base(suffix, 36);
  }

} // namespace piazza
