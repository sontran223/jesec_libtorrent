// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#ifndef LIBTORRENT_UTILS_INSTRUMENTATION_H
#define LIBTORRENT_UTILS_INSTRUMENTATION_H

#include <algorithm>
#include <array>

#include "torrent/common.h"
#include "torrent/utils/log.h"

namespace torrent {

enum instrumentation_enum {
  INSTRUMENTATION_MEMORY_BITFIELDS,
  INSTRUMENTATION_MEMORY_CHUNK_USAGE,
  INSTRUMENTATION_MEMORY_CHUNK_COUNT,
  INSTRUMENTATION_MEMORY_HASHING_CHUNK_USAGE,
  INSTRUMENTATION_MEMORY_HASHING_CHUNK_COUNT,

  INSTRUMENTATION_MINCORE_INCORE_TOUCHED,
  INSTRUMENTATION_MINCORE_INCORE_NEW,
  INSTRUMENTATION_MINCORE_NOT_INCORE_TOUCHED,
  INSTRUMENTATION_MINCORE_NOT_INCORE_NEW,
  INSTRUMENTATION_MINCORE_INCORE_BREAK,
  INSTRUMENTATION_MINCORE_SYNC_SUCCESS,
  INSTRUMENTATION_MINCORE_SYNC_FAILED,
  INSTRUMENTATION_MINCORE_SYNC_NOT_SYNCED,
  INSTRUMENTATION_MINCORE_SYNC_NOT_DEALLOCATED,
  INSTRUMENTATION_MINCORE_ALLOC_FAILED,
  INSTRUMENTATION_MINCORE_ALLOCATIONS,
  INSTRUMENTATION_MINCORE_DEALLOCATIONS,

  INSTRUMENTATION_POLLING_INTERRUPT_POKE,
  INSTRUMENTATION_POLLING_INTERRUPT_READ_EVENT,

  INSTRUMENTATION_POLLING_DO_POLL,
  INSTRUMENTATION_POLLING_DO_POLL_MAIN,
  INSTRUMENTATION_POLLING_DO_POLL_DISK,
  INSTRUMENTATION_POLLING_DO_POLL_OTHERS,

  INSTRUMENTATION_POLLING_EVENTS,
  INSTRUMENTATION_POLLING_EVENTS_MAIN,
  INSTRUMENTATION_POLLING_EVENTS_DISK,
  INSTRUMENTATION_POLLING_EVENTS_OTHERS,

  INSTRUMENTATION_TRANSFER_REQUESTS_DELEGATED,
  INSTRUMENTATION_TRANSFER_REQUESTS_DOWNLOADING,
  INSTRUMENTATION_TRANSFER_REQUESTS_FINISHED,
  INSTRUMENTATION_TRANSFER_REQUESTS_SKIPPED,
  INSTRUMENTATION_TRANSFER_REQUESTS_UNKNOWN,
  INSTRUMENTATION_TRANSFER_REQUESTS_UNORDERED,
  INSTRUMENTATION_TRANSFER_REQUESTS_QUEUED_ADDED,
  INSTRUMENTATION_TRANSFER_REQUESTS_QUEUED_MOVED,
  INSTRUMENTATION_TRANSFER_REQUESTS_QUEUED_REMOVED,
  INSTRUMENTATION_TRANSFER_REQUESTS_QUEUED_TOTAL,
  INSTRUMENTATION_TRANSFER_REQUESTS_UNORDERED_ADDED,
  INSTRUMENTATION_TRANSFER_REQUESTS_UNORDERED_MOVED,
  INSTRUMENTATION_TRANSFER_REQUESTS_UNORDERED_REMOVED,
  INSTRUMENTATION_TRANSFER_REQUESTS_UNORDERED_TOTAL,
  INSTRUMENTATION_TRANSFER_REQUESTS_STALLED_ADDED,
  INSTRUMENTATION_TRANSFER_REQUESTS_STALLED_MOVED,
  INSTRUMENTATION_TRANSFER_REQUESTS_STALLED_REMOVED,
  INSTRUMENTATION_TRANSFER_REQUESTS_STALLED_TOTAL,
  INSTRUMENTATION_TRANSFER_REQUESTS_CHOKED_ADDED,
  INSTRUMENTATION_TRANSFER_REQUESTS_CHOKED_MOVED,
  INSTRUMENTATION_TRANSFER_REQUESTS_CHOKED_REMOVED,
  INSTRUMENTATION_TRANSFER_REQUESTS_CHOKED_TOTAL,

  INSTRUMENTATION_TRANSFER_PEER_INFO_UNACCOUNTED,

  INSTRUMENTATION_MAX_SIZE
};

extern std::array<int64_t, INSTRUMENTATION_MAX_SIZE> instrumentation_values
  lt_cacheline_aligned;

void
instrumentation_initialize();
void
instrumentation_update(instrumentation_enum type, int64_t change);
void
instrumentation_tick();
void
instrumentation_reset();

//
// Implementation:
//

inline void
instrumentation_initialize() {
  std::fill(
    instrumentation_values.begin(), instrumentation_values.end(), int64_t());
}

inline void
instrumentation_update(instrumentation_enum type, int64_t change) {
#ifdef LT_INSTRUMENTATION
  __sync_add_and_fetch(&instrumentation_values[type], change);
#endif
}

} // namespace torrent

#endif
