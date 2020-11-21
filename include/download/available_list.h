// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#ifndef LIBTORRENT_DOWNLOAD_AVAILABLE_LIST_H
#define LIBTORRENT_DOWNLOAD_AVAILABLE_LIST_H

#include <list>
#include <vector>

#include <rak/socket_address.h>

#include "net/address_list.h"

namespace torrent {

class AvailableList : private std::vector<rak::socket_address> {
public:
  typedef std::vector<rak::socket_address> base_type;
  typedef uint32_t                         size_type;

  using base_type::const_reference;
  using base_type::reference;
  using base_type::value_type;

  using base_type::capacity;
  using base_type::clear;
  using base_type::const_iterator;
  using base_type::empty;
  using base_type::iterator;
  using base_type::reserve;
  using base_type::reverse_iterator;
  using base_type::size;

  using base_type::back;
  using base_type::begin;
  using base_type::end;
  using base_type::pop_back;
  using base_type::rbegin;
  using base_type::rend;

  AvailableList()
    : m_maxSize(1000) {}

  value_type pop_random();

  // Fuzzy size limit.
  size_type max_size() const {
    return m_maxSize;
  }
  void set_max_size(size_type s) {
    m_maxSize = s;
  }

  bool want_more() const {
    return size() <= m_maxSize;
  }

  // This push is somewhat inefficient as it iterates through the
  // whole container to see if the address already exists.
  void push_back(const rak::socket_address* sa);

  void insert(AddressList* l);
  void erase(const rak::socket_address& sa);
  void erase(iterator itr) {
    *itr = back();
    pop_back();
  }

  // A place to temporarily put addresses before re-adding them to the
  // AvailableList.
  AddressList* buffer() {
    return &m_buffer;
  }

private:
  size_type m_maxSize;

  AddressList m_buffer;
};

} // namespace torrent

#endif