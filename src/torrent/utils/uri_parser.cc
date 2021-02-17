// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2005-2011, Jari Sundell <jaris@ifi.uio.no>

#include <cctype>

#include "torrent/utils/string_manip.h"
#include "torrent/utils/uri_parser.h"

namespace torrent::utils {

inline bool
is_unreserved_uri_char(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~';
}

inline bool
is_valid_uri_query_char(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
         c == '~' || c == ':' || c == '&' || c == '=' || c == '/' || c == '%';
}

inline bool
is_unreserved_uri_query_char(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' ||
         c == '~' || c == ':' || c == '=' || c == '/' || c == '%';
}

inline bool
is_not_unreserved_uri_char(char c) {
  return !is_unreserved_uri_char(c);
}

inline bool
is_not_valid_uri_query_char(char c) {
  return !is_valid_uri_query_char(c);
}

inline bool
is_not_unreserved_uri_query_char(char c) {
  return !is_unreserved_uri_query_char(c);
}

template<typename Ftor>
inline std::string::const_iterator
uri_string_copy_until(std::string::const_iterator first,
                      std::string::const_iterator last,
                      std::string&                result,
                      Ftor                        check) {
  std::string::const_iterator next = std::find_if(first, last, check);

  result = std::string(first, next);
  return next;
}

void
uri_parse_throw_error(const char* error_msg, char invalid_char) {
  std::string error_str = std::string(error_msg);
  error_str += utils::value_to_hexchar<1>(invalid_char);
  error_str += utils::value_to_hexchar<0>(invalid_char);

  throw uri_error(error_str);
}

void
uri_parse_str(std::string uri, uri_state& state) {
  if (state.state != uri_state::state_empty)
    throw uri_error("uri_state.state is not uri_state::state_empty");

  state.uri.swap(uri);
  state.state = uri_state::state_invalid;

  std::string::const_iterator first = state.uri.begin();
  std::string::const_iterator last  = state.uri.end();

  // Parse scheme:
  first = uri_string_copy_until(first, last, state.scheme, [](char c) {
    return is_not_unreserved_uri_char(c);
  });

  if (first == last)
    goto uri_parse_success;

  if (*first++ != ':')
    uri_parse_throw_error("could not find ':' after scheme, found character 0x",
                          *--first);

  // Parse resource:
  first = uri_string_copy_until(first, last, state.resource, [](char c) {
    return is_not_unreserved_uri_char(c);
  });

  if (first == last)
    goto uri_parse_success;

  if (*first++ != '?')
    uri_parse_throw_error(
      "could not find '?' after resource, found character 0x", *--first);

  // Parse query:
  first = uri_string_copy_until(first, last, state.query, [](char c) {
    return is_not_valid_uri_query_char(c);
  });

  if (first == last)
    goto uri_parse_success;

  if (*first++ != '#')
    uri_parse_throw_error("could not find '#' after query, found character 0x",
                          *--first);

uri_parse_success:
  state.state = uri_state::state_valid;
  return;
}

void
uri_parse_c_str(const char* uri, uri_state& state) {
  uri_parse_str(std::string(uri), state);
}

// * Letters (A-Z and a-z), numbers (0-9) and the characters
//   '.','-','~' and '_' are left as-is
// * SPACE is encoded as '+' or "%20"
// * All other characters are encoded as %HH hex representation with
//   any non-ASCII characters first encoded as UTF-8 (or other
//   specified encoding)
void
uri_parse_query_str(std::string query, uri_query_state& state) {
  if (state.state != uri_query_state::state_empty)
    throw uri_error(
      "uri_query_state.state is not uri_query_state::state_empty");

  state.query.swap(query);
  state.state = uri_state::state_invalid;

  std::string::const_iterator first = state.query.begin();
  std::string::const_iterator last  = state.query.end();

  while (first != last) {
    std::string element;
    first = uri_string_copy_until(first, last, element, [](char c) {
      return is_not_unreserved_uri_query_char(c);
    });

    if (first != last && *first++ != '&') {
      std::string invalid_hex;
      invalid_hex += utils::value_to_hexchar<1>(*--first);
      invalid_hex += utils::value_to_hexchar<0>(*first);

      throw uri_error("query element contains invalid character 0x" +
                      invalid_hex);
    }

    state.elements.push_back(element);
  }

  state.state = uri_state::state_valid;
}

void
uri_parse_query_str(const char* query, uri_query_state& state) {
  uri_parse_query_str(std::string(query), state);
}

} // namespace torrent
