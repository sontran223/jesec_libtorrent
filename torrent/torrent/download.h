#ifndef LIBTORRENT_DOWNLOAD_H
#define LIBTORRENT_DOWNLOAD_H

#include <torrent/common.h>
#include <torrent/entry.h>
#include <torrent/peer.h>

#include <list>
#include <sigc++/signal.h>

namespace torrent {

typedef std::list<Peer> PList;

// == and = operators works.

class Download {
public:
  Download() :        m_ptr(NULL) {}
  Download(void* d) : m_ptr(d) {}

  // Not active atm
  void                 open();
  void                 close();

  void                 start();
  void                 stop();

  // Does not check if it has been removed.
  bool                 is_valid()  { return m_ptr; }

  bool                 is_open();
  bool                 is_active();
  bool                 is_tracker_busy();

  std::string          get_name();
  std::string          get_hash();

  uint64_t             get_bytes_up();
  uint64_t             get_bytes_down();
  uint64_t             get_bytes_done();
  uint64_t             get_bytes_total();

  uint32_t             get_chunks_size();
  uint32_t             get_chunks_done();
  uint32_t             get_chunks_total();

  // Bytes per second.
  uint32_t             get_rate_up();
  uint32_t             get_rate_down();
  
  const unsigned char* get_bitfield_data();
  uint32_t             get_bitfield_size();

  uint32_t             get_peers_min();
  uint32_t             get_peers_max();
  uint32_t             get_peers_connected();
  uint32_t             get_peers_not_connected();

  uint32_t             get_uploads_max();
  
  uint64_t             get_tracker_timeout();
  std::string          get_tracker_msg();

  void                 set_peers_min(uint32_t v);
  void                 set_peers_max(uint32_t v);

  void                 set_uploads_max(uint32_t v);

  void                 set_tracker_timeout(uint64_t v);

  Entry                get_entry(uint32_t i);
  uint32_t             get_entry_size();

  // Call this when you want the modifications of the download priorities
  // in the entries to take effect.
  void                 update_priorities();

  void                 peer_list(PList& pList);
  Peer                 peer_find(const std::string& id);

  void*                get_ptr()          { return m_ptr; }
  void                 set_ptr(void* ptr) { m_ptr = ptr; }

  // Signals:

  typedef sigc::signal0<void>       SignalDownloadDone;
  SignalDownloadDone&               signal_download_done();

  typedef sigc::signal1<void, Peer> SignalPeerConnected;
  SignalPeerConnected&              signal_peer_connected();

  typedef sigc::signal1<void, Peer> SignalPeerDisconnected;
  SignalPeerConnected&              signal_peer_disconnected();

private:
  void*           m_ptr;
};

}

#endif
