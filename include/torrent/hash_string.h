// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

// A fixed with char array used to store 20 byte with hashes. This
// should really be replaced with std::array<20>.

#ifndef LIBTORRENT_HASH_STRING_H
#define LIBTORRENT_HASH_STRING_H

#include <cstring>
#include <iterator>
#include <string>
#include <torrent/common.h>

namespace torrent {

class LIBTORRENT_EXPORT HashString {
public:
  typedef char                                  value_type;
  typedef value_type&                           reference;
  typedef const value_type&                     const_reference;
  typedef value_type*                           iterator;
  typedef const value_type*                     const_iterator;
  typedef std::size_t                           size_type;
  typedef std::ptrdiff_t                        difference_type;
  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  static const size_type size_data = 20;

  size_type size() const {
    return size_data;
  }

  iterator begin() {
    return m_data;
  }
  const_iterator begin() const {
    return m_data;
  }

  iterator end() {
    return m_data + size();
  }
  const_iterator end() const {
    return m_data + size();
  }

  reverse_iterator rbegin() {
    return reverse_iterator(end());
  }
  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  reverse_iterator rend() {
    return reverse_iterator(begin());
  }
  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  reference operator[](size_type n) {
    return *(m_data + n);
  }
  const_reference operator[](size_type n) const {
    return *(m_data + n);
  }

  value_type* data() {
    return m_data;
  }
  const value_type* data() const {
    return m_data;
  }

  const value_type* c_str() const {
    return m_data;
  }

  std::string str() const {
    return std::string(m_data, size_data);
  }

  void clear(int v = 0) {
    std::memset(data(), v, size());
  }

  void assign(const value_type* src) {
    std::memcpy(data(), src, size());
  }

  bool equal_to(const char* hash) const {
    return std::memcmp(m_data, hash, size()) == 0;
  }
  bool not_equal_to(const char* hash) const {
    return std::memcmp(m_data, hash, size()) != 0;
  }

  // It is the users responsibility to ensure src.length() >=
  // size_data.
  static const HashString* cast_from(const char* src) {
    return (const HashString*)src;
  }
  static const HashString* cast_from(const std::string& src) {
    return (const HashString*)src.c_str();
  }

  static HashString* cast_from(char* src) {
    return (HashString*)src;
  }

private:
  char m_data[size_data];
};

const char*
hash_string_from_hex_c_str(const char* first,
                           HashString& hash) LIBTORRENT_EXPORT;
char*
hash_string_to_hex(const HashString& hash, char* first) LIBTORRENT_EXPORT;

std::string
hash_string_to_hex_str(const HashString& hash) LIBTORRENT_EXPORT;

inline const char*
hash_string_to_hex_first(const HashString& hash, char* first) {
  hash_string_to_hex(hash, first);
  return first;
}

inline bool
operator==(const HashString& one, const HashString& two) {
  return std::memcmp(one.begin(), two.begin(), HashString::size_data) == 0;
}

inline bool
operator!=(const HashString& one, const HashString& two) {
  return std::memcmp(one.begin(), two.begin(), HashString::size_data) != 0;
}

inline bool
operator<(const HashString& one, const HashString& two) {
  return std::memcmp(one.begin(), two.begin(), HashString::size_data) < 0;
}

inline bool
operator<=(const HashString& one, const HashString& two) {
  return std::memcmp(one.begin(), two.begin(), HashString::size_data) <= 0;
}

} // namespace torrent

#endif