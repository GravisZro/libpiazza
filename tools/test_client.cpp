// SPDX-License-Identifier: AGPL-3.0-only
//
// Small smoke-test client for the C++ Piazza protocol library.
//
// It authenticates with credentials supplied through the environment, then
// exercises the session, user-status, feed, and post-query paths. The goal is
// to expose protocol/transport failures against the real service without
// embedding credentials in the repository.

#include "protocol.h"
#include "rpc.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QStringList>

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace
{
  QString env_or_empty(const char* name)
  {
    const QByteArray value = qgetenv(name);
    return QString::fromUtf8(value);
  }

  QSet<QString> known_post_keys()
  {
    return {
      "id", "folders", "nr", "data", "created", "bucket_order",
      "bucket_name", "no_answer_followup", "change_log", "history_size",
      "history", "History", "type", "tags", "config", "tag_good",
      "tag_good_arr", "children", "unique_views", "uid", "status", "drafts",
      "request_instructor", "request_instructor_me", "bookmarked",
      "num_favorites", "my_favorite", "is_bookmarked", "is_tag_good",
      "q_edits", "i_edits", "s_edits", "t", "default_anonymity", "my_post",
      "is_pinned"};
  }

  void report_extra_keys(const QJsonObject& post)
  {
    const QSet<QString> known = known_post_keys();
    QStringList extra;
    for(auto it = post.constBegin(); it != post.constEnd(); ++it)
    {
      if(!known.contains(it.key()))
        extra.push_back(it.key());
    }
    extra.sort();
    if(!extra.isEmpty())
    {
      std::cout << "undocumented top-level post keys: "
                << extra.join(", ").toStdString() << "\n";
    }
    else
    {
      std::cout << "no undocumented top-level post keys observed\n";
    }
  }

  int fail(const QString& message, int code = 1)
  {
    std::cerr << "FAIL: " << message.toStdString() << "\n";
    return code;
  }
} // namespace

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  const QString email = env_or_empty("PIAZZA_EMAIL");
  const QString password = env_or_empty("PIAZZA_PASSWORD");
  const QString requested_nid = env_or_empty("PIAZZA_NID");

  if(email.isEmpty() || password.isEmpty())
    return fail("set PIAZZA_EMAIL and PIAZZA_PASSWORD environment variables");

  try
  {
    piazza::RpcClient client;

    std::cout << "logging in...\n";
    client.user_login(email, password);
    std::cout << "login ok\n";

    const QJsonValue status = client.get_user_status();
    const QJsonObject status_obj = status.toObject();
    if(status_obj.isEmpty())
      return fail("user.status returned a non-object/empty result");

    const QJsonArray networks = status_obj.value("networks").toArray();
    if(networks.isEmpty())
      return fail("user.status did not return any networks");

    QString nid = requested_nid;
    if(nid.isEmpty())
      nid = networks.at(0).toObject().value("id").toString();
    if(nid.isEmpty())
      return fail("could not determine a network id");
    client.set_network_id(nid);
    std::cout << "using network " << nid.toStdString() << "\n";

    const QJsonValue feed = client.get_my_feed(3, 0, "updated");
    const QJsonObject feed_obj = feed.toObject();
    const QJsonArray feed_items = feed_obj.value("feed").toArray();
    if(feed_items.isEmpty())
      return fail("network.get_my_feed returned no feed items");
    std::cout << "feed items: " << feed_items.size() << "\n";

    const QJsonValue cid = feed_items.at(0).toObject().value("id");
    if(cid.isUndefined() || cid.isNull())
      return fail("first feed item has no id");
    std::cout << "first cid: " << cid.toString().toStdString() << "\n";

    // Parse through the typed protocol path. This is where a response whose
    // shape differs from the data dictionary will fail.
    const piazza::Post post = client.content_get(cid);
    std::cout << "content.get parsed ok; post id="
              << post.id.toStdString() << "\n";

    const QJsonValue raw = client.request(
        piazza::method_content_get,
        QJsonObject{{"nid", nid}, {"cid", cid}});
    report_extra_keys(raw.toObject());

    const int unread = client.get_unread_message_count();
    std::cout << "memo.get_unread_message_count = " << unread << "\n";

    const QString event_result = client.page_event("homepage.topbar_login_modal");
    std::cout << "generic.page_event = " << event_result.toStdString() << "\n";
  }
  catch(const piazza::AuthenticationError& e)
  {
    return fail(QString("authentication failed: ") + e.what());
  }
  catch(const piazza::RequestError& e)
  {
    return fail(QString("RPC request failed: ") + e.what());
  }
  catch(const std::exception& e)
  {
    return fail(QString("unexpected failure: ") + e.what());
  }

  return 0;
}
