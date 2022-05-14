// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#include "dht/dht_tracker.h"
#include "torrent/object.h"
#include "torrent/utils/random.h"

namespace torrent {

void
DhtTracker::add_peer(uint32_t addr, uint16_t port) {
  if (port == 0)
    return;

  SocketAddressCompact compact(addr, port);

  unsigned int oldest  = 0;
  uint32_t     minSeen = ~uint32_t();

  // Check if peer exists. If not, find oldest peer.
  for (unsigned int i = 0; i < size(); i++) {
    if (m_peers[i].peer.addr == compact.addr) {
      m_peers[i].peer.port = compact.port;
      m_lastSeen[i]        = cachedTime.seconds();
      return;

    } else if (m_lastSeen[i] < minSeen) {
      minSeen = m_lastSeen[i];
      oldest  = i;
    }
  }

  // If peer doesn't exist, append to list if the table is not full.
  if (size() < max_size) {
    m_peers.push_back(compact);
    m_lastSeen.push_back(cachedTime.seconds());

    // Peer doesn't exist and table is full: replace oldest peer.
  } else {
    m_peers[oldest]    = compact;
    m_lastSeen[oldest] = cachedTime.seconds();
  }
}

// Return compact info as bencoded string (8 bytes per peer) for up to 30 peers,
// returning different peers for each call if there are more.
raw_list
DhtTracker::get_peers(unsigned int maxPeers) {
  if (sizeof(BencodeAddress) != 8)
    throw internal_error("DhtTracker::BencodeAddress is packed incorrectly.");

  auto first = m_peers.begin();
  auto last  = m_peers.end();

  // If we have more than max_peers, randomly return block of peers.
  // The peers in overlapping blocks get picked twice as often, but
  // that's better than returning fewer peers.
  if (m_peers.size() > maxPeers) {
    first += random_uniform_size(0, m_peers.size() - maxPeers - 1);
    last = first + maxPeers;
  }

  return raw_list(first->bencode(),
                  distance(first, last) * sizeof(BencodeAddress));
}

// Remove old announces.
void
DhtTracker::prune(uint32_t maxAge) {
  uint32_t minSeen = cachedTime.seconds() - maxAge;

  for (unsigned int i = 0; i < m_lastSeen.size(); i++)
    if (m_lastSeen[i] < minSeen)
      m_peers[i].peer.port = 0;

  m_peers.erase(std::remove_if(m_peers.begin(),
                               m_peers.end(),
                               std::mem_fn(&BencodeAddress::empty)),
                m_peers.end());
  m_lastSeen.erase(std::remove_if(m_lastSeen.begin(),
                                  m_lastSeen.end(),
                                  [minSeen](uint32_t seen) {
                                    return std::less<uint32_t>()(seen, minSeen);
                                  }),
                   m_lastSeen.end());

  if (m_peers.size() != m_lastSeen.size())
    throw internal_error("DhtTracker::prune did inconsistent peer pruning.");
}

} // namespace torrent
