#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>
#include <map>
#include <set>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"

class lock_server_cache {
 private:
  int nacquire;
	
	// lock map
	pthread_mutex_t lock_map_mutex;
	std::map<lock_protocol::lockid_t, bool> lock_map;

	// each lock mutex and cond
	std::map<lock_protocol::lockid_t, pthread_mutex_t> mutex_map;
	std::map<lock_protocol::lockid_t, pthread_cond_t> cond_map;

	// owner map
	std::map<lock_protocol::lockid_t, std::string> owner_map;
	// wait map
	std::map<lock_protocol::lockid_t, std::set<std::string> > wait_map;
	// revoke map
	std::map<lock_protocol::lockid_t, bool> revoke_map;
		
 public:
  lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
