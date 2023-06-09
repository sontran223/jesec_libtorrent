// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#ifndef LIBTORRENT_DHT_ROUTER_H
#define LIBTORRENT_DHT_ROUTER_H

#include "torrent/dht_manager.h"
#include "torrent/hash_string.h"
#include "torrent/object.h"
#include "torrent/utils/priority_queue_default.h"
#include "torrent/utils/socket_address.h"

#include "dht_hash_map.h"
#include "dht_node.h"
#include "dht_server.h"

namespace torrent {

class DhtBucket;
class DhtTracker;
class TrackerDht;

// Main DHT class, maintains the routing table of known nodes and talks to the
// DhtServer object that handles the actual communication.

class DhtRouter : public DhtNode {
public:
  // How many bytes to return and verify from the 20-byte SHA token.
  static constexpr unsigned int size_token = 8;

  static constexpr unsigned int timeout_bootstrap_retry =
    60; // Retry initial bootstrapping every minute.
  static constexpr unsigned int timeout_update =
    15 * 60; // Regular housekeeping updates every 15 minutes.
  static constexpr unsigned int timeout_bucket_bootstrap =
    15 * 60; // Bootstrap idle buckets after 15 minutes.
  static constexpr unsigned int timeout_remove_node =
    4 * 60 * 60; // Remove unresponsive nodes after 4 hours.
  static constexpr unsigned int timeout_peer_announce =
    30 * 60; // Remove peers which haven't reannounced for 30 minutes.

  // A node ID of all zero.
  static HashString zero_id;

  DhtRouter(const Object& cache, const utils::socket_address* sa);
  ~DhtRouter();

  // Start and stop the router. This starts/stops the UDP server as well.
  void start(int port);
  void stop();

  bool is_active() {
    return m_server.is_active();
  }

  // Find peers for given download and announce ourselves.
  void announce(DownloadInfo* info, TrackerDht* tracker);

  // Cancel any pending transactions related to the given download (or all if
  // NULL).
  void cancel_announce(DownloadInfo* info, const TrackerDht* tracker);

  // Retrieve tracked torrent for the hash.
  // Returns NULL if not tracking the torrent unless create is true.
  DhtTracker* get_tracker(const HashString& hash, bool create);

  // Check if we are interested in inserting a new node of the given ID
  // into our table (i.e. if we have space or bad nodes in the corresponding
  // bucket).
  bool want_node(const HashString& id);

  // Add the given host to the list of potential contacts if we haven't
  // completed the bootstrap process, or contact the given address directly.
  void add_contact(const std::string& host, int port);
  void contact(const utils::socket_address* sa, int port);

  // Retrieve node of given ID in constant time. Return NULL if not found,
  // unless it's our own ID in which case it returns the DhtRouter object.
  DhtNode* get_node(const HashString& id);

  // Search for node with given address in O(n), disregarding the port.
  DhtNode* find_node(const utils::socket_address* sa);

  // Whenever a node queries us, replies, or is confirmed inactive (no reply) or
  // invalid (reply with wrong ID), we need to update its status.
  DhtNode* node_queried(const HashString& id, const utils::socket_address* sa);
  DhtNode* node_replied(const HashString& id, const utils::socket_address* sa);
  DhtNode* node_inactive(const HashString& id, const utils::socket_address* sa);
  void     node_invalid(const HashString& id);

  // Store compact node information (26 bytes) for nodes closest to the
  // given ID in the given buffer, return new buffer end.
  raw_string get_closest_nodes(const HashString& id) {
    return find_bucket(id)->second->full_bucket();
  }

  // Store DHT cache in the given container.
  Object* store_cache(Object* container) const;

  // Create and verify a token. Tokens are valid between 15-30 minutes from
  // creation.
  raw_string make_token(const utils::socket_address* sa, char* buffer);
  bool       token_valid(raw_string token, const utils::socket_address* sa);

  DhtManager::statistics_type get_statistics() const;
  void                        reset_statistics() {
    m_server.reset_statistics();
  }

  void set_upload_throttle(ThrottleList* t) {
    m_server.set_upload_throttle(t);
  }
  void set_download_throttle(ThrottleList* t) {
    m_server.set_download_throttle(t);
  }

private:
  // Hostname and port of potential bootstrap nodes.
  using contact_t = std::pair<std::string, int>;

  // Number of nodes we need to consider the bootstrap process complete.
  static constexpr unsigned int num_bootstrap_complete = 256;

  // Maximum number of potential contacts to keep until bootstrap complete.
  static constexpr unsigned int num_bootstrap_contacts = 1024;

  using DhtBucketList = std::map<const HashString, DhtBucket*>;

  DhtBucketList::iterator find_bucket(const HashString& id);

  bool add_node_to_bucket(DhtNode* node);
  void delete_node(const DhtNodeList::accessor& itr);

  void store_closest_nodes(const HashString& id, DhtBucket* bucket);

  DhtBucketList::iterator split_bucket(const DhtBucketList::iterator& itr,
                                       DhtNode*                       node);

  void bootstrap();
  void bootstrap_bucket(const DhtBucket* bucket);

  void receive_timeout();
  void receive_timeout_bootstrap();

  // buffer needs to hold an SHA1 hash (20 bytes), not just the token (8 bytes)
  char* generate_token(const utils::socket_address* sa,
                       int                          token,
                       char                         buffer[20]);

  utils::priority_item m_taskTimeout;

  DhtServer      m_server;
  DhtNodeList    m_nodes;
  DhtBucketList  m_routingTable;
  DhtTrackerList m_trackers;

  std::deque<contact_t>* m_contacts;

  int m_numRefresh;

  bool m_networkUp;

  // Secret keys used for generating announce tokens.
  int32_t m_curToken;
  int32_t m_prevToken;
};

inline raw_string
DhtRouter::make_token(const utils::socket_address* sa, char* buffer) {
  return raw_string(generate_token(sa, m_curToken, buffer), size_token);
}

} // namespace torrent

#endif
