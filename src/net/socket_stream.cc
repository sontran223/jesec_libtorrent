// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#include "net/socket_stream.h"
#include "torrent/utils/error_number.h"

namespace torrent {

std::string
int_to_string(int v) {
  char buf[20];

  std::snprintf(buf, sizeof(buf), "%i", v);

  return buf;
}

uint32_t
SocketStream::read_stream_throws(void* buf, uint32_t length) {
  int r = read_stream(buf, length);

  if (r == 0)
    throw close_connection();

  if (r < 0) {
    if (utils::error_number::current().is_blocked_momentary())
      return 0;
    else if (utils::error_number::current().is_closed())
      throw close_connection();
    else if (utils::error_number::current().is_blocked_prolonged())
      throw blocked_connection();
    else
      throw connection_error(utils::error_number::current().value());
  }

  return r;
}

uint32_t
SocketStream::write_stream_throws(const void* buf, uint32_t length) {
  int r = write_stream(buf, length);

  if (r == 0)
    throw close_connection();

  if (r < 0) {
    if (utils::error_number::current().is_blocked_momentary())
      return 0;
    else if (utils::error_number::current().is_closed())
      throw close_connection();
    else if (utils::error_number::current().is_blocked_prolonged())
      throw blocked_connection();
    else
      throw connection_error(utils::error_number::current().value());
  }

  return r;
}

} // namespace torrent
