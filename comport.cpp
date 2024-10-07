
#include <iostream>
#include "comport.h"

/////////////////////////////////////////////////////////////////////////////////////////
comport::comport()
{
}

/////////////////////////////////////////////////////////////////////////////////////////
comport::~comport()
{
    this->close_port();
}

/////////////////////////////////////////////////////////////////////////////////////////
bool comport::is_open()
{
    return port_no_>(HANDLE)0;
}

/////////////////////////////////////////////////////////////////////////////////////////
int comport::open_port(const char* comname, const char* baudrate, const char *mode)
{
    int baudr,status;

    astring_ = comname;
    astring_ +=" ";
    astring_ += baudrate;

    if(port_no_>0)
    {
        return 0;
    }

    switch(::atoi(baudrate))
    {
    case      50 : baudr=B50;
        break;
    case      75 : baudr=B75;
        break;
    case     110 : baudr=B110;
        break;
    case     134 : baudr=B134;
        break;
    case     150 : baudr=B150;
        break;
    case     200 : baudr=B200;
        break;
    case     300 : baudr=B300;
        break;
    case     600 : baudr=B600;
        break;
    case    1200 : baudr=B1200;
        break;
    case    1800 : baudr=B1800;
        break;
    case    2400 : baudr=B2400;
        break;
    case    4800 : baudr=B4800;
        break;
    case    9600 : baudr=B9600;
        break;
    case   19200 : baudr=B19200;
        break;
    case   38400 : baudr=B38400;
        break;
    case   57600 : baudr=B57600;
        break;
    case  115200 : baudr=B115200;
        break;
    case  230400 : baudr=B230400;
        break;
    case  460800 : baudr=B460800;
        break;
    case  500000 : baudr=B500000;
        break;
    case  576000 : baudr=B576000;
        break;
    case  921600 : baudr=B921600;
        break;
    case 1000000 : baudr=B1000000;
        break;
    case 1500000 : baudr=B1500000;
        break;
    case 2000000 : baudr=B2000000;
        break;
    case 2500000 : baudr=B2500000;
        break;
    case 3000000 : baudr=B3000000;
        break;
    case 3500000 : baudr=B3500000;
        break;
    case 4000000 : baudr=B4000000;
        break;
    default      : printf("invalid baudrate\n");
        return(1);
        break;
    }

    int cbits=CS8,
        cpar=0,
        ipar=IGNPAR,
        bstop=0;

    if(strlen(mode) !=3)
    {
        printf("invalid mode \"%s\"\n", mode);
        return(1);
    }

    switch(mode[0])
    {
    case '8': cbits=CS8;
        break;
    case '7': cbits=CS7;
        break;
    case '6': cbits=CS6;
        break;
    case '5': cbits=CS5;
        break;
    default : printf("invalid number of data-bits '%c'\n", mode[0]);
        return(1);
        break;
    }

    switch(mode[1])
    {
    case 'N':
    case 'n': cpar=0;
        ipar=IGNPAR;
        break;
    case 'E':
    case 'e': cpar=PARENB;
        ipar=INPCK;
        break;
    case 'O':
    case 'o': cpar=(PARENB | PARODD);
        ipar=INPCK;
        break;
    default : printf("invalid parity '%c'\n", mode[1]);
        return(1);
        break;
    }

    switch(mode[2])
    {
    case '1': bstop=0;
        break;
    case '2': bstop=CSTOPB;
        break;
    default : printf("invalid number of lib_stop bits '%c'\n", mode[2]);
        return(1);
        break;
    }

    /*
http://pubs.opengroup.org/onlinepubs/7908799/xsh/termios.h.html

http://man7.org/linux/man-pages/man3/termios.3.html
*/

    port_no_=::open(comname, O_RDWR | O_NOCTTY | O_NDELAY);
    if(port_no_==-1)
    {
        std::cerr << ("Unable to open com_port. Reconnect the USB ");
        return(port_no_);
    }

    /* lock access so that another process can't also use the port_no_ */
    if(flock(port_no_, LOCK_EX | LOCK_NB) !=0)
    {
        ::close(port_no_);
        std::cerr << ("Another process has locked the com");
        return(1);
    }

    rt_err_=tcgetattr(port_no_, &old_tcset_);
    if(rt_err_==-1)
    {
        ::close(port_no_);
        flock(port_no_, LOCK_UN);  /* free the port_no_ so that others can use it. */
        perror("unable to read _portsettings ");
        return(1);
    }
    memset(&curtc_et_, 0, sizeof(curtc_et_));  /* clear the new struct */

    curtc_et_.c_cflag=cbits | cpar | bstop | CLOCAL | CREAD;
    curtc_et_.c_iflag=ipar;
    curtc_et_.c_oflag=0;
    curtc_et_.c_lflag=0;
    curtc_et_.c_cc[VMIN]=0;      /* block untill n bytes are received */
    curtc_et_.c_cc[VTIME]=0;     /* block untill a timer expires (n * 100 mSec.) */

    cfsetispeed(&curtc_et_, baudr);
    cfsetospeed(&curtc_et_, baudr);

    rt_err_=tcsetattr(port_no_, TCSANOW, &curtc_et_);
    if(rt_err_==-1)
    {
        tcsetattr(port_no_, TCSANOW, &old_tcset_);

        ::close(port_no_);
        ::flock(port_no_, LOCK_UN);  /* free the port_no_ so that others can use it. */
        port_no_=0;
        perror("unable to adjust _portsettings ");
        return(1);
    }

    /* http://man7.org/linux/man-pages/man4/tty_ioctl.4.html */

    if(ioctl(port_no_, TIOCMGET, &status)==-1)
    {
        tcsetattr(port_no_, TCSANOW, &old_tcset_);
        flock(port_no_, LOCK_UN);  /* free the port_no_ so that others can use it. */
        perror("unable to get _portstatus 1");
        return(1);
    }

    status |=TIOCM_DTR;    /* turn on DTR */
    status |=TIOCM_RTS;    /* turn on RTS */

    if(ioctl(port_no_, TIOCMSET, &status)==-1)
    {
        tcsetattr(port_no_, TCSANOW, &old_tcset_);
        flock(port_no_, LOCK_UN);  /* free the port_no_ so that others can use it. */
        perror("unable to set _portstatus");
        return(1);
    }

    return(0);
}

/////////////////////////////////////////////////////////////////////////////////////////
int comport::read_bytes(unsigned char *buf, int toread, long to)
{
    int     got=0;
    struct  timeval tv;
    fd_set  fds,exx;
    if(this->connected())
    {
        FD_ZERO(&fds);
        FD_ZERO(&exx);
        FD_SET(port_no_, &fds);
        FD_SET(port_no_, &exx);
        tv.tv_sec=0;
        tv.tv_usec=(to * 1000);

        int sel = ::select(port_no_+1, &fds, NULL, &exx, &tv);
        if(sel > 0)
        {
            if( FD_ISSET(port_no_, &fds))
            {
                got=::read(port_no_, buf, toread);
            }
            if( FD_ISSET(port_no_, &exx))
            {
                std::cerr << ("FD_ISSET exception");
                this->close_port();
            }
        }
        if(got>=0){
            buf[got]=0;
            if(precsend_){
                (precsend_)((const char*)buf,false);
            }
        }
        return got;
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////////////////////////
int comport::send_char(unsigned char byte)
{
    int n=::write(port_no_, &byte, 1);
    if(n < 0)
    {
        return 0;
    }

    return(1);
}

/////////////////////////////////////////////////////////////////////////////////////////
int comport::send_bytes(const unsigned char *buf, int toread, time_t to)
{
    int    tries = toread + 1;
    int    exahust = toread + 1;
    int    left = toread;
    int sent=0;
    do{
        int n=::write(port_no_, buf+sent, left);
        if(n < 0)
        {
            if(errno==EAGAIN)
            {
                ::msleep(64);
                if(tries--==0)
                    return -1;
                continue;
            }
            else
            {
                return -1;
            }
        }
        if( n > 0)
        {
            left -=n;
        }
    }while(left>0 && exahust-->0);
    return (left==0) ? 0 : 1/*tout*/;
}

/////////////////////////////////////////////////////////////////////////////////////////
void comport::close_port()
{
    int status;
    if(port_no_>0)
    {
        ::msleep(100);
        this->flush_rx_tx();
        ::msleep(100);
        if(ioctl(port_no_, TIOCMGET, &status)==-1)
        {
            //perror("unable to get _portstatus Closing");
        }
        else
        {
            status &=~TIOCM_DTR;    /* turn off DTR */
            status &=~TIOCM_RTS;    /* turn off RTS */

            if(ioctl(port_no_, TIOCMSET, &status)==-1)
            {
                perror("unable to set _portstatus");
            }
        }

        tcsetattr(port_no_, TCSANOW, &old_tcset_);
        ::close(port_no_);
        flock(port_no_, LOCK_UN);  /* free the port_no_ so that others can use it. */
    }
    port_no_=0;
}

/////////////////////////////////////////////////////////////////////////////////////////
/*
Constant  Description
TIOCM_LE        DSR (data set ready/line enable)
TIOCM_DTR       DTR (data terminal ready)
TIOCM_RTS       RTS (request to uartSend)
TIOCM_ST        Secondary TXD (transmit)
TIOCM_SR        Secondary RXD (waitPrompt)
TIOCM_CTS       CTS (clear to uartSend)
TIOCM_CAR       DCD (data carrier detect)
TIOCM_CD        see TIOCM_CAR
TIOCM_RNG       RNG (ring)
TIOCM_RI        see TIOCM_RNG
TIOCM_DSR       DSR (data set ready)

http://man7.org/linux/man-pages/man4/tty_ioctl.4.html
*/
/////////////////////////////////////////////////////////////////////////////////////////
void comport::flush_rx()
{
    tcflush(port_no_, TCIFLUSH);
}

/////////////////////////////////////////////////////////////////////////////////////////
void comport::flush_tx()
{
    tcflush(port_no_, TCOFLUSH);
}

/////////////////////////////////////////////////////////////////////////////////////////
void comport::flush_rx_tx()
{
    tcflush(port_no_, TCIOFLUSH);
}

/////////////////////////////////////////////////////////////////////////////////////////
int comport::list_coms(pfncb cb)
{
    int ports=0;
    for(int i=0;i<32;i++)
    {
        char file[128];
        ::sprintf(file,"/dev/ttyACM%d", i);
        if(::access(file,0)==0)
        {
            if(cb){
                (cb)(file);
            }
            ++ports;
        }
        ::sprintf(file,"/dev/ttyUSB%d",i);
        if(::access(file,0)==0)
        {
            if(cb){
                (cb)(file);
            }
            ++ports;
        }
    }
    return ports;
}

