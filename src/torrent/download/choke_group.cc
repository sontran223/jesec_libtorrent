// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#include "config.h"

#include <algorithm>
#include <functional>

#include "choke_group.h"
#include "choke_queue.h"

// TODO: Put resource_manager_entry in a separate file.
#include "resource_manager.h"

#include "download/download_main.h"
#include "torrent/exceptions.h"

namespace torrent {

choke_group::choke_group()
  : m_tracker_mode(TRACKER_MODE_NORMAL)
  , m_down_queue(choke_queue::flag_unchoke_all_new)
  , m_first(NULL)
  , m_last(NULL) {}

uint64_t
choke_group::up_rate() const {
  return std::for_each(
           m_first,
           m_last,
           rak::accumulate((uint64_t)0,
                           std::bind(&Rate::rate,
                                     std::bind(&resource_manager_entry::up_rate,
                                               std::placeholders::_1))))
    .result;
}

uint64_t
choke_group::down_rate() const {
  return std::for_each(m_first,
                       m_last,
                       rak::accumulate(
                         (uint64_t)0,
                         std::bind(&Rate::rate,
                                   std::bind(&resource_manager_entry::down_rate,
                                             std::placeholders::_1))))
    .result;
}

} // namespace torrent
