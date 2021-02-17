#ifndef LIBTORRENT_TORRENT_UTILS_LOG_BUFFER_H
#define LIBTORRENT_TORRENT_UTILS_LOG_BUFFER_H

#include <deque>
#include <functional>
#include <memory>
#include <pthread.h>
#include <string>
#include <utility>

namespace torrent {

struct log_entry {
  log_entry(int32_t t, int32_t grp, std::string msg)
    : timestamp(t)
    , group(grp)
    , message(std::move(msg)) {}

  bool is_older_than(int32_t t) const {
    return timestamp < t;
  }
  bool is_younger_than(int32_t t) const {
    return timestamp > t;
  }
  bool is_younger_or_same(int32_t t) const {
    return timestamp >= t;
  }

  int32_t     timestamp;
  int32_t     group;
  std::string message;
};

class [[gnu::visibility("default")]] log_buffer
  : private std::deque<log_entry> {
public:
  using base_type = std::deque<log_entry>;
  using slot_void = std::function<void()>;

  using base_type::const_iterator;
  using base_type::const_reverse_iterator;
  using base_type::iterator;
  using base_type::reverse_iterator;

  using base_type::back;
  using base_type::begin;
  using base_type::end;
  using base_type::front;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::empty;
  using base_type::size;

  log_buffer() {
    pthread_mutex_init(&m_lock, nullptr);
  }

  unsigned int max_size() const {
    return m_max_size;
  }

  // Always lock before calling any function.
  void lock() {
    pthread_mutex_lock(&m_lock);
  }
  void unlock() {
    pthread_mutex_unlock(&m_lock);
  }

  const_iterator find_older(int32_t older_than);

  void lock_and_set_update_slot(const slot_void& slot) {
    lock();
    m_slot_update = slot;
    unlock();
  }

  void lock_and_push_log(const char* data, size_t length, int group);

private:
  pthread_mutex_t m_lock;
  unsigned int    m_max_size{ 200 };
  slot_void       m_slot_update;
};

using log_buffer_ptr =
  std::unique_ptr<log_buffer, std::function<void(log_buffer*)>>;

[[gnu::visibility("default")]] log_buffer_ptr
log_open_log_buffer(const char* name);

} // namespace torrent

#endif
