// yfs client.  implements FS operations using extent and lock server

#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
using std::cout;
using std::endl;
using std::string;
using std::stringstream;


yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client_cache(lock_dst);
        lc->acquire(1);
  if (ec->put(1, "") != extent_protocol::OK)
      printf("error init root dir\n"); // XYB: init root dir
        lc->release(1);
}


yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    lc->acquire(inum);
    bool ret = cp_isfile(inum);
    lc->release(inum);
    return ret;
}


bool
yfs_client::cp_isfile(inum inum)
{
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is not a file\n", inum);
    return false;
}


bool
yfs_client::isdir(inum inum)
{
    lc->acquire(inum);
    bool ret = cp_isdir(inum);
    lc->release(inum);
    return ret;
}


bool
yfs_client::cp_isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    } 
    printf("isdir: %lld is not a dir\n", inum);
    return false;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    lc->acquire(inum);
    int ret = cp_getfile(inum, fin);
    lc->release(inum);
    return ret;
}



int
yfs_client::cp_getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    lc->acquire(inum);
    int ret = cp_getdir(inum, din);
    lc->release(inum);
    return ret;
}


int
yfs_client::cp_getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)


int
yfs_client::setattr(inum ino, size_t size)
{
    lc->acquire(ino);
    int ret = cp_setattr(ino, size);
    lc->release(ino);
    return ret;
}


// Only support set size of attr
int
yfs_client::cp_setattr(inum ino, size_t size)
{
    cout << "#Azard: In yfs->setattr\n";
    int r = OK;
    /*
     * your lab2 code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    string buf;   
    if (ec->get(ino, buf) != extent_protocol::OK) {
        cout << "#Azard: setattr get data fail.\n";
        r = IOERR;
        return r;
    }
    size_t bufsize = buf.size();
    if(size > bufsize)
        buf.resize(size, '\0'); 
    else 
        buf.resize(size);
    ec->put(ino, buf);
    return r;
}


int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    lc->acquire(parent);
    int ret = cp_create(parent, name, mode, ino_out);
    lc->release(parent);
    return ret;
}

int
yfs_client::cp_create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    cout << "#Azard: In yfs->create\n";
    int r = OK;
    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    cp_lookup(parent, name, found, ino_out);
    if(!found)
    {
        ec->create(extent_protocol::T_FILE, ino_out);
        string buf;
        ec->get(parent,buf);
        if (buf.size() == 0)
            buf.append(string(name) + ',' + filename(ino_out));
        else
            buf.append(',' + string(name) + ',' + filename(ino_out));
        ec->put(parent, buf);
    }
    else
        r = EXIST; 
    return r;
}


int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    lc->acquire(parent);
    int ret = cp_mkdir(parent, name, mode, ino_out);
    lc->release(parent);
    return ret;
}


int
yfs_client::cp_mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    cout << "#Azard: In yfs->mkdir\n";
    int r = OK;
    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    cp_lookup(parent, name, found, ino_out);
    if(!found)
    {
        ec->create(extent_protocol::T_DIR, ino_out);
        string buf;
        ec->get(parent,buf);
        if(buf.size() == 0)
            buf.append(string(name) + ',' + filename(ino_out));
        else
            buf.append(',' + string(name) + ',' + filename(ino_out));
        ec->put(parent, buf);
    }
    else
        r = EXIST; 
    return r;
}


int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    lc->acquire(parent);
    int ret = cp_lookup(parent, name, found, ino_out);
    lc->release(parent);
    return ret;
}


int
yfs_client::cp_lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    cout << "#Azard: In yfs->lookup.\n";
    int r = OK;
    /*
     * your lab2 code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    if(!cp_isdir(parent)) {
        cout << "#Azard: lookup not a dir.\n";
        found = false;
        return r;
    }

    string buf;
    if(ec->get(parent, buf) == extent_protocol::OK) {

        string filename;
        string inodenum;
        bool flag_filename = false;
        bool flag_inodenum = false;
        uint32_t pos;
        uint32_t len_buf = buf.length();
        for (pos = 0; pos < buf.length(); pos++) {
            // split filename    
            if (!flag_filename) {
                if (buf[pos] == ',') {
                    flag_filename = true;
                    pos++;
                }
                else
                    filename += buf[pos];
            }
            // split inodenum
            if (flag_filename && !flag_inodenum) {
                if (buf[pos] == ',')
                    flag_inodenum = true;
                else if (pos == len_buf-1) {
                    flag_inodenum = true;
                    inodenum += buf[pos];
                }
                else
                    inodenum += buf[pos];
            }
            // compare to name
            if (flag_filename && flag_inodenum) {
                if (filename == string(name))
                {
                    cout << "#Azard: lookup find file.\n";
                    found =true;
                    ino_out = n2i(inodenum);
                    return r;
                }
                flag_filename = flag_inodenum = false;
                filename = inodenum = "";
            }
        }
        r = NOENT;
        cout << "#Azard: lookup not find file.\n";
    }

    else {
        cout << "#Azard: lookup get data fail.\n";
        r = IOERR;
    }
    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    lc->acquire(dir);
    int ret = cp_readdir(dir, list);
    lc->release(dir);
    return ret;
}

int
yfs_client::cp_readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;
    cout << "#Azard: In yfs->readdir.\n";
    /*
     * your lab2 code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
     if (!cp_isdir(dir)) {
        cout << "#Azard: readdir read not dir.\n";
        r = IOERR;
        return r;
     }

    string buf;
    if(ec->get(dir, buf) != extent_protocol::OK) {
        cout << "#Azard: readdir get data fail.\n";
        r = IOERR;
        return r;
    }

    dirent* entry = new dirent();
    string filename;
    string inodenum;
    bool flag_filename = false;
    bool flag_inodenum = false;
    uint32_t pos;
    uint32_t len_buf = buf.length();
    for (pos = 0; pos < buf.length(); pos++) {
        // split filename    
        if (!flag_filename) {
            if (buf[pos] == ',') {
                flag_filename = true;
                pos++;
            }
            else
                filename += buf[pos];
        }
        // split inodenum
        if (flag_filename && !flag_inodenum) {
            if (buf[pos] == ',')
                flag_inodenum = true;
            else if (pos == len_buf-1) {
                flag_inodenum = true;
                inodenum += buf[pos];
            }
            else
                inodenum += buf[pos];
        }
        // compare to name
        if (flag_filename && flag_inodenum) {
            entry->name = filename;
            entry->inum = n2i(inodenum);
            list.push_back(*entry);     

            flag_filename = flag_inodenum = false;
            filename = inodenum = "";
        }
    }

    delete entry; 
    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    lc->acquire(ino);
    int ret = cp_read(ino, size, off, data);
    lc->release(ino);
    return ret;
}

int
yfs_client::cp_read(inum ino, size_t size, off_t off, std::string &data)
{
    cout << "#Azard: In yfs->read.\n";
    int r = OK;
    /*
     * your lab2 code goes here.
     * note: read using ec->get().
     */
    string buf;
    if(ec->get(ino, buf) != extent_protocol::OK) {
        cout << "#Azard: read get data fail.\n";
        r = IOERR;
        return r;
    }
    int bufsize = buf.size();
    if(bufsize <= off)
        data = "\0";    
    else {   
        if(bufsize - off <= (int)size)
            data = buf.substr(off, bufsize);    
        else
            data = buf.substr(off,size);
    }
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    lc->acquire(ino);
    int ret = cp_write(ino, size, off, data, bytes_written);
    lc->release(ino);
    return ret;
}

int
yfs_client::cp_write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    cout << "#Azard: In yfs->write.\n";
    int r = OK;
    /*
     * your lab2 code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    string buf;
    if(ec->get(ino, buf) != extent_protocol::OK) {
        cout << "#Azard: write get data fail.\n";
        r = IOERR;
        return r;
    }

    int bufsize = buf.size();
    if(bufsize < off) {
        buf.resize(off,'\0');   
        buf.append(data, size);
    } else {
        if(bufsize < off + (int)size) {
            buf.resize(off);
            buf.append(data,size);
        } else
            buf.replace(off, size, string(data,size));     
    }
    bytes_written = size;
    ec->put(ino, buf);
    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    lc->acquire(parent);
    int ret = cp_unlink(parent, name);
    lc->release(parent);
    return ret;
}

int yfs_client::cp_unlink(inum parent,const char *name)
{
    cout << "#Azard: In yfs->unlink.\n";
    int r = OK;
    /*
     * your lab2 code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
     
    inum inodenum = 0;
    bool found = false;
    r = cp_lookup(parent, name, found, inodenum);
    if (r == IOERR) {
        cout << "#Azard: unlink use lookup have IOERR.\n";
        return r;
    }

    if(found) {
        string buf;
        if(ec->get(parent, buf) != extent_protocol::OK) {
            cout << "#Azard: unlink get data fail.\n";
            r = IOERR;
            return r;
        }
        ec->remove(inodenum);
        size_t begin_pos = buf.find(name);
        buf.replace(begin_pos, strlen(name)+filename(inodenum).size()+2,"");    
        if (buf[buf.length()-1] == ',')
            buf.replace(buf.length()-1, 1, "");
        ec->put(parent, buf);       
    }
    else
        r = NOENT;  
    return r;
}

int yfs_client::symlink(inum parent, const string name, const string link, inum &ino_out)
{
    lc->acquire(parent);
    int ret = cp_symlink(parent, name, link, ino_out);
    lc->release(parent);
    return ret;
}

int yfs_client::cp_symlink(inum parent, const string name, const string link, inum &ino_out)
{

    cout << "#Azard: In yfs->symlink.\n";
    int r = OK;

    bool found = false;
    cp_lookup(parent, name.c_str(), found, ino_out);
    if (found) {
        cout << "#Azard: symlink has same name one.\n";
        r = EXIST;
        return r;
    }

    // create a link file
    ec->create(extent_protocol::T_LINK, ino_out);
    string buf;
    ec->get(parent,buf);
    if (buf.size() == 0)
        buf.append(string(name) + ',' + filename(ino_out));
    else
        buf.append(',' + string(name) + ',' + filename(ino_out));
    ec->put(parent, buf);

    // write thing to link file
    ec->put(ino_out, link);
    return r;
}

int yfs_client::readlink(inum ino, std::string &link)
{
    lc->acquire(ino);
    int ret = cp_readlink(ino, link);
    lc->release(ino);
    return ret;
}


int yfs_client::cp_readlink(inum ino, std::string &link)
{
    cout << "#Azard: In yfs->readlink.\n";
    int r = OK;
    if(ec->get(ino, link) != extent_protocol::OK) {
        cout << "#Azard: read get data fail.\n";
        r = IOERR;
    }
    return r;
}

