// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }

	cache_map.clear();
	pthread_mutex_init(&cache_map_mutex, NULL);
}


extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_map_mutex);
		
		if (cache_map[eid].is_attr_changed) {
			attr = cache_map[eid].metadata;
		}
		else {
			ret = cl->call(extent_protocol::getattr, eid, attr);
			cache_map[eid].metadata = attr;
			cache_map[eid].is_attr_changed = true;
		}
	
	printf("#Lab6: get attr > %lld  < size: %d\n", eid, attr.size);
	pthread_mutex_unlock(&cache_map_mutex);
  return ret;
}



extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &id)
{
  extent_protocol::status ret = extent_protocol::OK;
	ret = cl->call(extent_protocol::create, type, id);
	printf("#Lab6: create > %lld\n", id);
  return ret;
}



extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_map_mutex);
		
		if (cache_map[eid].is_cached) {
			buf = cache_map[eid].data;
			printf("#Lab6: get cache %s > %lld\n", buf.c_str(), eid);
		}
		else {
			ret = cl->call(extent_protocol::get, eid, buf);
			cache_map[eid].data  = buf;
			cache_map[eid].is_cached = true;
			cache_map[eid].is_dirty = false;
			cache_map[eid].is_attr_changed= false;
			printf("#Lab6: get %s > %lld\n", buf.c_str(), eid);
		}

	pthread_mutex_unlock(&cache_map_mutex);
	return ret;
}



extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_map_mutex);
		
		cache_map[eid].is_cached = true;
		cache_map[eid].is_dirty = true;
		if (eid != 1)
		{
			cache_map[eid].is_attr_changed = true;
		}
		else
		{
			if (buf.size() != 0)
				cache_map[eid].is_attr_changed = true;
			else
				cache_map[eid].is_attr_changed = false;
		}
		cache_map[eid].data = buf;
		cache_map[eid].metadata.atime = cache_map[eid].metadata.mtime = cache_map[eid].metadata.ctime = time(NULL);
		cache_map[eid].metadata.size = buf.size();
		printf("#Lab6: put cache %s > %lld\n", buf.c_str(), eid);

	pthread_mutex_unlock(&cache_map_mutex);
  return ret;
}


extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
	pthread_mutex_lock(&cache_map_mutex);

		int a;
		ret = cl->call(extent_protocol::remove, eid, a);
		cache_map.erase(eid);
		printf("#Lab6: remove > %lld\n", eid);
	
	pthread_mutex_unlock(&cache_map_mutex);
  return ret;
}



extent_protocol::status
extent_client::flush(extent_protocol::extentid_t eid)
{
	extent_protocol::status ret = extent_protocol::OK;
	int a;
	pthread_mutex_lock(&cache_map_mutex);

		if (cache_map[eid].is_dirty)
		{
			ret = cl->call(extent_protocol::put, eid, cache_map[eid].data, a);
		}
		cache_map.erase(eid);
		printf("#flush\n");

	pthread_mutex_unlock(&cache_map_mutex);
	return ret;
}

