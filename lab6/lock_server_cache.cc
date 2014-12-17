// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_server_cache::lock_server_cache()
{
	pthread_mutex_init(&lock_map_mutex, NULL);
	lock_map.clear();
	mutex_map.clear();
	cond_map.clear();
	owner_map.clear();
	wait_map.clear();
	revoke_map.clear();
}


int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, int &)
{
	tprintf("%s >> acquire >> %lld\n", id.c_str(), lid);
	int r = 0;
  lock_protocol::status ret = lock_protocol::OK;

	// is exist?
	pthread_mutex_lock(&lock_map_mutex);
	nacquire++;
	if (lock_map.find(lid) == lock_map.end()) {
		lock_map[lid] = false;
		mutex_map[lid];
		cond_map[lid];
		owner_map[lid] = "";
		wait_map[lid].erase(id);
		revoke_map[lid] = false;
		pthread_mutex_init(&mutex_map[lid], NULL);
		pthread_cond_init(&cond_map[lid], NULL);
		
	}
	pthread_mutex_unlock(&lock_map_mutex);


	// acquire main
	pthread_mutex_lock(&mutex_map[lid]);
	// Not hold by any client
	if (lock_map[lid] == false) {
		tprintf("%s >> locked >> %lld\n", id.c_str(), lid);
		lock_map[lid] = true;
		owner_map[lid] = id;
		wait_map[lid].erase(id);	// delete from wait set
		revoke_map[lid] = false;
		pthread_mutex_unlock(&mutex_map[lid]);
		ret = lock_protocol::OK;
		return ret;
	}
	// Has hold by client
	else {
		wait_map[lid].insert(id);
		// have send revoke
		if (revoke_map[lid]) {
			ret = lock_protocol::RETRY;
			pthread_mutex_unlock(&mutex_map[lid]);
			tprintf("respond RETRY >> %s >> %lld\n", id.c_str(), lid);
			return ret;
		}
		// send revok to owner
		else {
			revoke_map[lid] = true;
			pthread_mutex_unlock(&mutex_map[lid]);
			std::string target = owner_map[lid];
			handle h(target);
			rpcc *cl = h.safebind();
			if (cl){
				tprintf("send revoke >> %s >> %lld\n", target.c_str(), lid);
			  cl->call(rlock_protocol::revoke, lid, r);
			} else {
				VERIFY(0);
			}
			ret = lock_protocol::RETRY;
			tprintf("respond RETRY >> %s >> %lld\n", id.c_str(), lid);
			return ret;
		}
	}
  return ret;
}




int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, int &r)
{
	tprintf("%s >> release >> %lld\n", id.c_str(), lid);
  lock_protocol::status ret = lock_protocol::OK;
	
	// is exist?
	pthread_mutex_lock(&lock_map_mutex);
	if (lock_map.find(lid) == lock_map.end())
		VERIFY(0);
	pthread_mutex_unlock(&lock_map_mutex);

	
	pthread_mutex_lock(&mutex_map[lid]);
	tprintf("%s >> released >> %lld\n", id.c_str(), lid);
	lock_map[lid] = false;
	owner_map[lid] = "";
	revoke_map[lid] = false;
	
	// send retry to client
	if (wait_map[lid].begin() != wait_map[lid].end()) {
			handle h(*wait_map[lid].begin());
			rpcc *cl = h.safebind();
			if (cl) {
					tprintf("send retry >> %s >> %lld\n", (*wait_map[lid].begin()).c_str(), lid);
					cl->call(rlock_protocol::retry, lid, r);
			} else {
					VERIFY(0);
			}
			// more than 1, send revoke
			if (wait_map[lid].size() > 1) {
					tprintf("send revoke >> %s >> %lld\n", (*wait_map[lid].begin()).c_str(), lid);
					cl->call(rlock_protocol::revoke, lid, r);
			}
	}

	pthread_mutex_unlock(&mutex_map[lid]);
	ret = lock_protocol::OK;
  return ret;
}




lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

