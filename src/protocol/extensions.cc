// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#include "torrent/buildinfo.h"

#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <limits>

#include "download/available_list.h"
#include "download/download_main.h"
#include "download/download_wrapper.h"
#include "manager.h"
#include "protocol/extensions.h"
#include "protocol/peer_connection_base.h"
#include "torrent/connection_manager.h"
#include "torrent/download/download_manager.h"
#include "torrent/object_stream.h"
#include "torrent/peer/connection_list.h"
#include "torrent/peer/peer_info.h"

namespace torrent {

template<>
const ExtHandshakeMessage::key_list_type ExtHandshakeMessage::keys = {
  { key_e, "e" },
  { key_m_utMetadata, "m::ut_metadata" },
  { key_m_utPex, "m::ut_pex" },
  { key_metadataSize, "metadata_size" },
  { key_p, "p" },
  { key_reqq, "reqq" },
  { key_v, "v" },
};

template<>
const ExtPEXMessage::key_list_type ExtPEXMessage::keys = {
  { key_pex_added, "added*S" },
};

// DEBUG: Add type info.
template<>
const ExtMetadataMessage::key_list_type ExtMetadataMessage::keys = {
  { key_msgType, "msg_type" },
  { key_piece, "piece" },
  { key_totalSize, "total_size" },
};

struct message_type {
  const char*        key;
  ext_handshake_keys index;
};

const message_type message_keys[] = { { "HANDSHAKE", key_handshake_LAST },
                                      { "ut_pex", key_m_utPex },
                                      { "metadata_size", key_m_utMetadata } };

void
ProtocolExtension::cleanup() {
  //   if (is_default())
  //     return;

  for (int t = HANDSHAKE + 1; t < FIRST_INVALID; t++)
    if (is_local_enabled(t))
      unset_local_enabled(t);
}

void
ProtocolExtension::set_local_enabled(int t) {
  if (is_local_enabled(t))
    return;

  m_flags |= flag_local_enabled_base << t;

  switch (t) {
    case UT_PEX:
      m_download->info()->set_size_pex(m_download->info()->size_pex() + 1);
      break;
    default:
      break;
  }
}

void
ProtocolExtension::unset_local_enabled(int t) {
  if (!is_local_enabled(t))
    return;

  m_flags &= ~(flag_local_enabled_base << t);

  switch (t) {
    case UT_PEX:
      m_download->info()->set_size_pex(m_download->info()->size_pex() - 1);
      break;
    default:
      break;
  }
}

DataBuffer
ProtocolExtension::generate_handshake_message() {
  ExtHandshakeMessage message;

  // Add "e" key if encryption is enabled, set it to 1 if we require
  // encryption for incoming connections, or 0 otherwise.
  if ((manager->connection_manager()->encryption_options() &
       ConnectionManager::encryption_allow_incoming) != 0)
    message[key_e] = (manager->connection_manager()->encryption_options() &
                      ConnectionManager::encryption_require) != 0;

  message[key_p]    = manager->connection_manager()->listen_port();
  message[key_v]    = raw_string::from_c_str("libTorrent " LT_VERSION);
  message[key_reqq] = 2048; // maximum request queue size

  if (!m_download->info()->is_meta_download())
    message[key_metadataSize] = m_download->info()->metadata_size();

  message[key_m_utPex]      = is_local_enabled(UT_PEX) ? UT_PEX : 0;
  message[key_m_utMetadata] = UT_METADATA;

  char            buffer[1024];
  object_buffer_t result =
    static_map_write_bencode_c(object_write_to_buffer,
                               nullptr,
                               std::make_pair(buffer, buffer + sizeof(buffer)),
                               message);

  int   length = result.second - buffer;
  char* copy   = new char[length];
  memcpy(copy, buffer, length);

  return DataBuffer(copy, copy + length);
}

inline DataBuffer
ProtocolExtension::build_bencode(size_t maxLength, const char* format, ...) {
  char* b = new char[maxLength];

  va_list args;
  va_start(args, format);
  unsigned int length = vsnprintf(b, maxLength, format, args);
  va_end(args);

  if (length > maxLength) {
    delete[] b;
    throw internal_error("ProtocolExtension::build_bencode wrote past buffer.");
  }

  return DataBuffer(b, b + length);
}

DataBuffer
ProtocolExtension::generate_toggle_message(MessageType t, bool on) {
  // TODO: Check if we're accepting this message type?

  // Manually create bencoded map { "m" => { message_keys[t] => on ? t : 0 } }
  return build_bencode(32,
                       "d1:md%zu:%si%deee",
                       strlen(message_keys[t].key),
                       message_keys[t].key,
                       on ? t : 0);
}

DataBuffer
ProtocolExtension::generate_ut_pex_message(const PEXList& added,
                                           const PEXList& removed) {
  if (added.empty() && removed.empty())
    return DataBuffer();

  int added_len   = added.size() * 6;
  int removed_len = removed.size() * 6;

  // Manually create bencoded map { "added" => added, "dropped" => dropped }
  auto  buffer_size = 32 + added_len + removed_len;
  char* buffer      = new char[buffer_size];

  auto count = 0;

  count +=
    snprintf(buffer + count, buffer_size - count, "d5:added%d:", added_len);
  if (added_len > 0) {
    assert(added.begin()->c_str() != nullptr);
    std::memcpy(buffer + count, added.begin()->c_str(), added_len);
  }
  count += added_len;

  count +=
    snprintf(buffer + count, buffer_size - count, "7:dropped%d:", removed_len);
  if (removed_len > 0) {
    assert(removed.begin()->c_str() != nullptr);
    std::memcpy(buffer + count, removed.begin()->c_str(), removed_len);
  }
  count += removed_len;

  *(buffer + count) = 'e';
  count += 1;

  if (buffer_size - count < 0)
    throw internal_error(
      "ProtocolExtension::ut_pex_message wrote beyond buffer.");

  return DataBuffer(buffer, buffer + count);
}

void
ProtocolExtension::read_start(int type, uint32_t length, bool skip) {
  if (is_default() || (type >= FIRST_INVALID) || length > (1 << 15))
    throw communication_error("Received invalid extension message.");

  if (m_read != nullptr || (int32_t)length < 0)
    throw internal_error(
      "ProtocolExtension::read_start called in inconsistent state.");

  m_readLeft = length;

  if (skip || !is_local_enabled(type)) {
    m_readType = SKIP_EXTENSION;

  } else {
    m_readType = type;
  }

  // Allocate the buffer even for SKIP_EXTENSION, just to make things
  // simpler.
  m_readPos = m_read = new char[length];
}

bool
ProtocolExtension::read_done() {
  bool result = true;

  try {
    switch (m_readType) {
      case SKIP_EXTENSION:
        break;
      case HANDSHAKE:
        result = parse_handshake();
        break;
      case UT_PEX:
        result = parse_ut_pex();
        break;
      case UT_METADATA:
        result = parse_ut_metadata();
        break;
      default:
        throw internal_error(
          "ProtocolExtension::read_done called with invalid extension type.");
    }

  } catch (bencode_error& e) {
    // Ignore malformed messages.
    // DEBUG:
    //     throw internal_error("ProtocolExtension::read_done '" +
    //     std::string(m_read, std::distance(m_read, m_readPos)) + "'");
  }

  delete[] m_read;
  m_read = nullptr;

  m_readType = FIRST_INVALID;
  m_flags |= flag_received_ext;

  return result;
}

// Called whenever peer enables or disables an extension.
void
ProtocolExtension::peer_toggle_remote(int type, bool active) {
  if (type == UT_PEX) {
    // When ut_pex is enabled, the first peer exchange afterwards needs
    // to be a full message, not delta.
    if (active)
      m_flags |= flag_initial_pex;
  }
}

bool
ProtocolExtension::parse_handshake() {
  ExtHandshakeMessage message;
  static_map_read_bencode(m_read, m_readPos, message);

  for (int t = HANDSHAKE + 1; t < FIRST_INVALID; t++) {
    if (!message[message_keys[t].index].is_value())
      continue;

    uint8_t id = message[message_keys[t].index].as_value();

    set_remote_supported(t);

    if (id != m_idMap[t - 1]) {
      peer_toggle_remote(t, id != 0);

      m_idMap[t - 1] = id;
    }
  }

  // If this is the first handshake, then disable any local extensions
  // not supported by remote.
  if (is_initial_handshake()) {
    for (int t = HANDSHAKE + 1; t < FIRST_INVALID; t++)
      if (!is_remote_supported(t))
        unset_local_enabled(t);
  }

  if (message[key_p].is_value()) {
    uint16_t port = message[key_p].as_value();

    if (port > 0)
      m_peerInfo->set_listen_port(port);
  }

  if (message[key_reqq].is_value())
    m_maxQueueLength = message[key_reqq].as_value();

  if (message[key_metadataSize].is_value())
    m_download->set_metadata_size(message[key_metadataSize].as_value());

  m_flags &= ~flag_initial_handshake;

  return true;
}

bool
ProtocolExtension::parse_ut_pex() {
  // Ignore message if we're still in the handshake (no connection
  // yet), or no peers are present.

  ExtPEXMessage message;
  static_map_read_bencode(m_read, m_readPos, message);

  // TODO: Check if pex is enabled?
  if (!message[key_pex_added].is_raw_string())
    return true;

  raw_string peers = message[key_pex_added].as_raw_string();

  if (peers.empty())
    return true;

  // TODO: Sort the list before adding it.
  AddressList l;
  l.parse_address_compact(peers);
  l.sort();
  l.erase(std::unique(l.begin(), l.end()), l.end());

  m_download->peer_list()->insert_available(&l);

  return true;
}

bool
ProtocolExtension::parse_ut_metadata() {
  ExtMetadataMessage message;

  // Piece data comes after bencoded extension message.
  const char* dataStart = static_map_read_bencode(m_read, m_readPos, message);

  switch (message[key_msgType].as_value()) {
    case 0:
      // Can't process new request while still having data to send.
      if (has_pending_message())
        return false;

      send_metadata_piece(message[key_piece].as_value());
      break;

    case 1:
      if (m_connection == nullptr)
        break;

      m_connection->receive_metadata_piece(
        message[key_piece].as_value(), dataStart, m_readPos - dataStart);
      break;

    case 2:
      if (m_connection == nullptr)
        break;

      m_connection->receive_metadata_piece(
        message[key_piece].as_value(), nullptr, 0);
      break;
  };

  return true;
}

void
ProtocolExtension::send_metadata_piece(size_t piece) {
  // Reject out-of-range piece, or if we don't have the complete metadata yet.
  size_t metadataSize = m_download->info()->metadata_size();
  size_t pieceEnd =
    (metadataSize + metadata_piece_size - 1) >> metadata_piece_shift;

  if (m_download->info()->is_meta_download() || piece >= pieceEnd) {
    // reject: { "msg_type" => 2, "piece" => ... }
    m_pendingType = UT_METADATA;
    m_pending =
      build_bencode(sizeof(size_t) + 36, "d8:msg_typei2e5:piecei%zuee", piece);
    return;
  }

  // These messages will be rare, so we'll just build the
  // metadata here instead of caching it uselessly.
  char* buffer = new char[metadataSize];
  object_write_bencode_c(
    object_write_to_buffer,
    nullptr,
    object_buffer_t(buffer, buffer + metadataSize),
    &(*manager->download_manager()->find(m_download->info()->hash()))
       ->bencode()
       ->get_key("info"));

  // data: { "msg_type" => 1, "piece" => ..., "total_size" => ... } followed by
  // piece data (outside of dictionary)
  size_t length = piece == pieceEnd - 1
                    ? m_download->info()->metadata_size() % metadata_piece_size
                    : metadata_piece_size;
  m_pendingType = UT_METADATA;
  m_pending     = build_bencode((2 * sizeof(size_t)) + length + 120,
                            "d8:msg_typei1e5:piecei%zue10:total_sizei%zuee",
                            piece,
                            metadataSize);

  memcpy(m_pending.end(), buffer + (piece << metadata_piece_shift), length);
  m_pending.set(m_pending.data(), m_pending.end() + length, m_pending.owned());
  delete[] buffer;
}

bool
ProtocolExtension::request_metadata_piece(const Piece* p) {
  if (p->offset() % metadata_piece_size)
    throw internal_error("ProtocolExtension::request_metadata_piece got "
                         "misaligned piece offset.");

  if (has_pending_message())
    return false;

  m_pendingType = UT_METADATA;
  m_pending     = build_bencode(40,
                            "d8:msg_typei0e5:piecei%uee",
                            (unsigned)(p->offset() >> metadata_piece_shift));
  return true;
}

} // namespace torrent
