
#pragma once
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/time.h>
#include <string>

//////////////////////////////////////////////////////////////////////////////////////////////////
typedef void(*pfncb)(const char*);
typedef void(*pfncbSr)(const char*, bool out);

#define  HANDLE         int
#define  REC_BUFF_SZ    512
//////////////////////////////////////////////////////////////////////////////////////////////////
class comport
{
public:
    comport();
    ~comport();

    int open_port(const char*, const char*, const char *);
    int read_bytes(unsigned char *, int, long to=1);
    int send_bytes(const unsigned char * p, int t, long tp=1);
    int send_char(unsigned char);
    bool connected()const{
        return port_no_>(HANDLE)0;
    }
    void close_port();
    void flush_rx();
    void flush_tx();
    void flush_rx_tx();
    bool is_open();
    static int list_coms(pfncb cb);
    void rt_cb(pfncbSr cb){
        precsend_=cb;
    }

public:
    int              rt_err_=0;
    char             st_mode_[200];
    pfncbSr          precsend_=nullptr;
    struct termios   curtc_et_;
    struct termios   old_tcset_;
    int              port_no_=0;
    char             recbuff_[REC_BUFF_SZ];
    std::string      astring_;

};

/////////////////////////////////////////////////////////////////////////////////////////
inline void msleep(int t)
{
    ::usleep(t*1000);
}

//////////////////////////////////////////////////////////////////////////////////////////
inline time_t tick_count()
{
    struct timeval tv;
    if(gettimeofday(&tv, NULL) !=0)
        return 0;
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
//////////////////////////////////////////////////////////////////////////////////////////


