#ifndef yfs_client_h
#define yfs_client_h

#include <string>

#include "lock_protocol.h"
#include "lock_client_cache.h"

//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>


class yfs_client {
  extent_client *ec;
  lock_client *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);

 public:
  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  int setattr(inum, size_t);
  int lookup(inum, const char *, bool &, inum &);
  int create(inum, const char *, mode_t, inum &);
  int readdir(inum, std::list<dirent> &);
  int write(inum, size_t, off_t, const char *, size_t &);
  int read(inum, size_t, off_t, std::string &);
  int unlink(inum,const char *);
  int mkdir(inum , const char *, mode_t , inum &);
  
	int symlink(inum parent, const std::string name, const std::string link, inum &ino);
  int readlink(inum ino, std::string &link);
  // int create_link(inum paretn, const char *name, mode_t, inum&);


  bool cp_isfile(inum);
  bool cp_isdir(inum);

  int cp_getfile(inum, fileinfo &);
  int cp_getdir(inum, dirinfo &);

  int cp_setattr(inum, size_t);
  int cp_lookup(inum, const char *, bool &, inum &);
  int cp_create(inum, const char *, mode_t, inum &);
  int cp_readdir(inum, std::list<dirent> &);
  int cp_write(inum, size_t, off_t, const char *, size_t &);
  int cp_read(inum, size_t, off_t, std::string &);
  int cp_unlink(inum,const char *);
  int cp_mkdir(inum , const char *, mode_t , inum &);
  
  int cp_symlink(inum parent, const std::string name, const std::string link, inum &ino);
  int cp_readlink(inum ino, std::string &link);

};

#endif 
