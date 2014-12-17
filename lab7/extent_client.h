// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "extent_server.h"

class extent_client {
 private:
  rpcc *cl;
	
	struct Cache {
		std::string data;
		extent_protocol::attr metadata;
		bool is_cached;
		bool is_dirty;
		bool is_attr_changed;
		Cache() {
			is_cached = false;
			is_dirty = false;
			is_attr_changed = false;
		}
	};

	std::map<extent_protocol::extentid_t, Cache> cache_map;
	pthread_mutex_t cache_map_mutex;


 public:
  extent_client(std::string dst);

  extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);
  extent_protocol::status get(extent_protocol::extentid_t eid, 
			                        std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				                          extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status flush(extent_protocol::extentid_t eid);
};

#endif 

