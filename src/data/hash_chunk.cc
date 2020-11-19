// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#include "config.h"

#include "chunk.h"
#include "chunk_list_node.h"
#include "hash_chunk.h"

namespace torrent {

bool
HashChunk::perform(uint32_t length, bool force) {
  length = std::min(length, remaining());

  if (m_position + length > m_chunk.chunk()->chunk_size())
    throw internal_error(
      "HashChunk::perform(...) received length out of range");

  uint32_t l = force ? length : m_chunk.chunk()->incore_length(m_position);

  bool complete = l == length;

  while (l) {
    Chunk::iterator node = m_chunk.chunk()->at_position(m_position);

    l -= perform_part(node, l);
  }

  return complete;
}

void
HashChunk::advise_willneed(uint32_t length) {
  if (!m_chunk.is_valid())
    throw internal_error("HashChunk::willneed(...) called on an invalid chunk");

  if (m_position + length > m_chunk.chunk()->chunk_size())
    throw internal_error(
      "HashChunk::willneed(...) received length out of range");

  uint32_t pos = m_position;

  while (length) {
    Chunk::iterator itr = m_chunk.chunk()->at_position(pos);

    uint32_t l = std::min(length, remaining_part(itr, pos));

    itr->chunk().advise(pos - itr->position(), l, MemoryChunk::advice_willneed);

    pos += l;
    length -= l;
    ++itr;
  }
}

uint32_t
HashChunk::perform_part(Chunk::iterator itr, uint32_t length) {
  length = std::min(length, remaining_part(itr, m_position));

  m_hash.update(itr->chunk().begin() + m_position - itr->position(), length);
  m_position += length;

  return length;
}

} // namespace torrent
