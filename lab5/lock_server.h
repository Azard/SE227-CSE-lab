// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"

class lock_server {

 protected:
  int nacquire;

  typedef lock_protocol::lockid_t lid_t;
  std::map<lid_t, bool> lock_status;
  std::map<lid_t, int> lock_times;
  std::map<lid_t, int> lock_owner;
  pthread_mutex_t mutex;
  pthread_cond_t cond;

 public:
  lock_server();
  ~lock_server() {};
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
};

#endif 







