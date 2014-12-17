// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"




int lock_client_cache::last_port = 0;

lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);



	status_map.clear();
	mutex_map.clear();
	cond_map.clear();
	revoke_map.clear();
	retry_map.clear();
	pthread_mutex_init(&status_map_mutex, NULL);
	pthread_cond_init(&status_map_cond, NULL);

	tprintf("%s >> init\n", id.c_str());
}






lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
	int r = 0;
	int ret = lock_protocol::OK;


	// exist this entry?
	pthread_mutex_lock(&status_map_mutex);
	if (status_map.find(lid) == status_map.end()) {
		status_map[lid] = NONE;
		mutex_map[lid];
		cond_map[lid];
		pthread_mutex_init(&mutex_map[lid], NULL);
		pthread_cond_init(&cond_map[lid], NULL);
		revoke_map[lid] = false;
		retry_map[lid] = false;
	}
	pthread_mutex_unlock(&status_map_mutex);


	// acquire main
RESTART:
	pthread_mutex_lock(&mutex_map[lid]);
	switch(status_map[lid]) {
		case NONE:
			{
				tprintf("%s >> acquiring >> %lld\n", id.c_str(), lid);
				status_map[lid] = ACQUIRING;
				pthread_mutex_unlock(&mutex_map[lid]);
				
				ret = cl->call(lock_protocol::acquire, lid, id, r);
				
				pthread_mutex_lock(&mutex_map[lid]);
				if (ret == lock_protocol::OK) {
					status_map[lid] = LOCKED;
					tprintf("%s >> locked >> %lld\n", id.c_str(), lid);
					pthread_mutex_unlock(&mutex_map[lid]);
					return ret;
				}
				else if (ret == lock_protocol::RETRY) {
					// retry arrive before this
					while (retry_map[lid] == false)
					{
						pthread_cond_wait(&cond_map[lid], &mutex_map[lid]);	
					}
						retry_map[lid] = false;
						status_map[lid] = NONE;
						pthread_cond_broadcast(&cond_map[lid]);
						pthread_mutex_unlock(&mutex_map[lid]);
						goto RESTART;
				}
			}

		case FREE:
			{
				status_map[lid] = LOCKED;
				pthread_mutex_unlock(&mutex_map[lid]);
				ret = lock_protocol::OK;
				return ret;
			}

		case LOCKED:
		case ACQUIRING:
		case RELEASING:
			{
				pthread_cond_wait(&cond_map[lid], &mutex_map[lid]);
				pthread_mutex_unlock(&mutex_map[lid]);
				goto RESTART;
			}
	}
  return ret;
}





lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
{
	tprintf("%s >> release >> %lld\n", id.c_str(), lid);
  int ret = lock_protocol::OK;
	int r = 0;
	
	// is exist?
	pthread_mutex_lock(&status_map_mutex);
	if (status_map.find(lid) == status_map.end()) {
		tprintf("%s >> Error 04 >> %lld", id.c_str(), lid);
		VERIFY(0);
	}
	pthread_mutex_unlock(&status_map_mutex);


	// release main
	pthread_mutex_lock(&mutex_map[lid]);
	switch(status_map[lid]) {
		case NONE:
		case FREE:
		case ACQUIRING:
		case RELEASING:
			{
				tprintf("%s >> Error 05 >> %lld", id.c_str(), lid);
				VERIFY(0);
			}

		case LOCKED:
			{
				if (revoke_map[lid]) {
					status_map[lid] = RELEASING;
					revoke_map[lid] = false;
					if (lu) {
						lu->dorelease(lid);
					}
					pthread_mutex_unlock(&mutex_map[lid]);
					
					ret = cl->call(lock_protocol::release, lid, id, r);
        
					pthread_mutex_lock(&mutex_map[lid]);
					tprintf("%s >> released >> %lld\n", id.c_str(), lid);
					status_map[lid] = NONE;
				} else {
					status_map[lid] = FREE;
				}
				pthread_cond_broadcast(&cond_map[lid]);
				pthread_mutex_unlock(&mutex_map[lid]);
				return ret;
			}
	}
	return ret;
}






rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, int &)
{
	int r = 0;
  int ret = rlock_protocol::OK;
  
	pthread_mutex_lock(&mutex_map[lid]);
	if (status_map[lid] == FREE) {
		status_map[lid] = RELEASING;
		pthread_mutex_unlock(&mutex_map[lid]);
		
		if (lu) {
				lu->dorelease(lid);
		}

		tprintf("%s >> revoke handler release >> %lld\n", id.c_str(), lid);
		ret = cl->call(lock_protocol::release, lid, id, r);
		tprintf("%s >> revoke handler released >> %lld\n", id.c_str(), lid);
	
		pthread_mutex_lock(&mutex_map[lid]);
		status_map[lid] = NONE;
	}
	else {
		revoke_map[lid] = true;
	}
	pthread_cond_broadcast(&cond_map[lid]);
	pthread_mutex_unlock(&mutex_map[lid]);

	return ret;
}




rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, int &)
{
  int ret = rlock_protocol::OK;
	
	pthread_mutex_lock(&mutex_map[lid]);
	tprintf("%s >> retry handler >> %lld\n", id.c_str(), lid);
	retry_map[lid] = true;
	pthread_cond_broadcast(&cond_map[lid]);
	pthread_mutex_unlock(&mutex_map[lid]);

  return ret;
}



