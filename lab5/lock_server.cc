// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
    lock_status.clear();
    lock_times.clear();
    lock_owner.clear();
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  printf("stat request from clt %d\n", clt);
  lock_protocol::status ret = lock_protocol::OK;

  // Not find this lockid
  /*
  if (lock_status.find(lid) == lock_status.end()) {
    ret = lock_protocol::NOENT;
    return ret;
  }
  */

  // lockid not matched client
  /*
  if (lock_owner[lid] != clt) {
    ret = lock_protocol::RPCERR;
    return ret;
  }
  */
  
  pthread_mutex_lock(&mutex);
  r = lock_times[lid];
  pthread_mutex_unlock(&mutex);
  
  return ret;
}



lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
	// Your lab4 code goes here

  printf("client %i, acquire: %lld\n", clt, lid);
  lock_protocol::status ret = lock_protocol::OK;

  if (clt < 0) {
    ret = lock_protocol::RPCERR;
    return ret;
  }

  pthread_mutex_lock(&mutex);
  if (lock_status.find(lid) != lock_status.end()) {
    while (lock_status[lid]) {
      printf("cond wait\n");
      pthread_cond_wait(&cond, &mutex);
    }
  }
  lock_status[lid] = true;
  lock_times[lid]++;
  lock_owner[lid] = clt;
  pthread_mutex_unlock(&mutex);

  return ret;
}



lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
	// Your lab4 code goes here
  
  printf("client %i, release: %lld\n", clt, lid);
  lock_protocol::status ret = lock_protocol::OK;


  pthread_mutex_lock(&mutex);

  // Not find this lockid
  if (lock_status.find(lid) == lock_status.end()) {
    ret = lock_protocol::NOENT;
    pthread_mutex_unlock(&mutex);
    return ret;
  }

  // Not locked
  if (!lock_status[lid]) {
    ret = lock_protocol::NOENT;
    pthread_mutex_unlock(&mutex);
    return ret;
  }

  // lockid not matched client
  if (lock_owner[lid] != clt) {
    ret = lock_protocol::RPCERR;
    pthread_mutex_unlock(&mutex);
    return ret;
  }


  // success to release
  lock_status[lid] = false;
  lock_owner[lid] = -1;
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&mutex);  
  return ret;
}
