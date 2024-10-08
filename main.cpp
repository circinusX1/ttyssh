/*
    From ssh samples and various open source
    By O.Marius Chincisan
 * */


#define LIBSSH_STATIC 1

#include <iostream>
#include "unistd.h"
#include <libssh/libssh.h>  // sudo apt install libssl-dev libssh-dev
#include <stdlib.h>
#include <stdio.h>
#include "comport.h"

static constexpr size_t MAX_BUFFER = 8912;
static constexpr size_t MAX_LOCO = 2014;
static int channel_in_out(ssh_session session, const char* boud);
static int verify_knownhost(ssh_session session);
static bool Echo=false;

int main(int n, char* a[])
{

    // if(getuid()!=0)    {
    //     std::cout << "run as root\n";
    //     exit(1);
    // }

    if(access("./module/ttyssh.ko",0)!=0)
    {
        std::cout << "no driver found\n";
        exit(1);
    }

    if(n != 5)
    {
        std::cout << "run: uart-speed ssh-user ssh-pass ssh-server\n";
        exit(1);
    }
    ssh_session my_ssh_session;
    int rc;
    int port = 22;
    int verbosity = SSH_LOG_PROTOCOL;
    char *password;

    // Open session and set options
    my_ssh_session = ssh_new();
    if (my_ssh_session == NULL)
        exit(-1);

    ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, a[4]);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_USER, a[2]);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(my_ssh_session, SSH_OPTIONS_CIPHERS_C_S,"aes128-ctr"); // if does not connect comment this line

    //ssh_options_set(my_ssh_session, SSH_OPTIONS_PORT, &port);
    // Connect to server
    rc = ssh_connect(my_ssh_session);
    if (rc != SSH_OK)
    {
        fprintf(stderr, "Error: %s\n", ssh_get_error(my_ssh_session)); 
        ssh_free(my_ssh_session);
        exit(-1);
    }

    password = a[3];
    rc = ssh_userauth_password(my_ssh_session, NULL, password);
    if (rc != SSH_AUTH_SUCCESS)
    {
        fprintf(stderr, "Error authenticating with password: %s\n",
                ssh_get_error(my_ssh_session));
        ssh_disconnect(my_ssh_session);
        ssh_free(my_ssh_session);
        exit(-1);
    }
    channel_in_out(my_ssh_session, a[1]);

    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
}

int channel_in_out(ssh_session session, const char* speed)
{
    ssh_channel channel;
    int rc;
    char buffer[256];
    int nbytes;
    fd_set fds;
    int maxfd;
    struct timeval timeout;

    std::cout << "ssh connected to remote\n";

    channel = ssh_channel_new(session);
    if (channel == NULL)
        return SSH_ERROR;

    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK)
    {
        ssh_channel_free(channel);
        return rc;
    }

    rc = ssh_channel_request_pty(channel);
    if (rc != SSH_OK) { std::cout << "PTY failed"; return rc;}

    rc = ssh_channel_change_pty_size(channel, 80, 24);
    if (rc != SSH_OK)  { std::cout << "PTY size failed"; return rc;}

    rc = ssh_channel_request_shell(channel);
    if (rc != SSH_OK)  { std::cout << "PTY shell failed"; return rc;}


    //ssh_channel_poll_timeout(channel,100,0);
    // ssh_channel_set_blocking(channel,false);

    ::system("sudo insmod ./module/ttyssh.ko");
    ::sleep(1);
    char* pu  = getenv("USER");
    std::string cmd = "sudo chown "; cmd += pu; cmd+=":"; cmd += pu;
    ::system((cmd + " /dev/vpoutput0").c_str());
    ::system((cmd + " /dev/vinput0").c_str());


    ::system((std::string("stty /dev/vpoutput0 ")+speed).c_str());
    ::system((std::string("stty /dev/vinputt0 ")+speed).c_str());

    comport     tty;

    if(tty.open_port("/dev/vpoutput0",speed,"8N1") == 0)
    {
        char loco[MAX_LOCO];
        char buffer[MAX_BUFFER];
        std::string accum;
        bool okay = true;
        bool crlf = false;
        ssh_channel in_channels[2], out_channels[2];

        std::cout << "Connect pronterface to: /dev/vpinput0, " << speed << "BPS\n\n";

        while(okay && ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel))
        {
            timeout.tv_sec = 0;
            timeout.tv_usec = 128000;
            in_channels[0] = channel;
            in_channels[1] = NULL;
            out_channels[0]=nullptr;
            out_channels[1]=nullptr;
            FD_ZERO(&fds);
            FD_SET(0, &fds);
            FD_SET(ssh_get_fd(session), &fds);
            maxfd = ssh_get_fd(session) + 1;


            int chars = tty.read_bytes((unsigned char*)loco,sizeof(loco)-1,10);
            if(chars>0)
            {
                loco[chars] = 0;
                ssh_channel_write(channel, loco, chars);
            }

            ssh_select(in_channels, out_channels, maxfd, &fds, &timeout);

            if (out_channels[0] != NULL)
            {
                nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
                if (nbytes < 0) {std::cout << "ssh channel read error \n"; break;}
                if (nbytes > 0)
                {
                    buffer[nbytes] = 0;
                    std::cout << "GOT:" << buffer << "\n";
                    tty.send_bytes((unsigned char*)buffer, strlen(buffer));
                }
                out_channels[0]=nullptr;
            }
            usleep(1000);
        };

        tty.close_port();
        std::cout << "serial closed\n";

    }

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    ::system("sudo rmmod ./module/ttyssh.ko");
    std::cout << "ssh disconnected remote\n";

    return SSH_OK;
}

int verify_knownhost(ssh_session session)
{
    enum ssh_known_hosts_e state;
    unsigned char *hash = NULL;
    ssh_key srv_pubkey = NULL;
    size_t hlen;
    char buf[10];
    char *hexa;
    char *p;
    int cmp;
    int rc;
    rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0) {
        return -1;
    }
    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA1,
                                &hash,
                                &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0) {
        return -1;
    }
    state = ssh_session_is_known_server(session);
    switch (state) {
    case SSH_KNOWN_HOSTS_OK:
        /* OK */
        break;
    case SSH_KNOWN_HOSTS_CHANGED:
        fprintf(stderr, "Host key for server changed: it is now:\n");
        //ssh_print_hexa("Public key hash", hash, hlen);
        fprintf(stderr, "For security reasons, connection will be stopped\n");
        ssh_clean_pubkey_hash(&hash);
        return -1;
    case SSH_KNOWN_HOSTS_OTHER:
        fprintf(stderr, "The host key for this server was not found but an other"
                        "type of key exists.\n");
        fprintf(stderr, "An attacker might change the default server key to"
                        "confuse your client into thinking the key does not exist\n");
        ssh_clean_pubkey_hash(&hash);
        return -1;
    case SSH_KNOWN_HOSTS_NOT_FOUND:
        fprintf(stderr, "Could not find known host file.\n");
        fprintf(stderr, "If you accept the host key here, the file will be"
                        "automatically created.\n");
        /* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */
    case SSH_KNOWN_HOSTS_UNKNOWN:
        hexa = ssh_get_hexa(hash, hlen);
        fprintf(stderr,"The server is unknown. Do you trust the host key?\n");
        fprintf(stderr, "Public key hash: %s\n", hexa);
        ssh_string_free_char(hexa);
        ssh_clean_pubkey_hash(&hash);
        p = fgets(buf, sizeof(buf), stdin);
        if (p == NULL) {
            return -1;
        }
        cmp = strncasecmp(buf, "yes", 3);
        if (cmp != 0) {
            return -1;
        }
        rc = ssh_get_status(session); //ssh_session_update_known_hosts(session);
        if (rc < 0) {
            fprintf(stderr, "Error %s\n", strerror(errno));
            return -1;
        }
        break;
    case SSH_KNOWN_HOSTS_ERROR:
        fprintf(stderr, "Error %s", ssh_get_error(session));
        ssh_clean_pubkey_hash(&hash);
        return -1;
    }
    ssh_clean_pubkey_hash(&hash);
    return 0;
}
