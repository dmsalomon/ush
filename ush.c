
#include <termios.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "util.h"

char *argv0 = "ush";

int tcpopen(const char *host, const char *serv);
void clientloop(int client_fd);
void enter_raw_mode(void);
void leave_raw_mode(void);

static struct termios saved_tio;
static int in_raw_mode = 0;

int main(int argc, char **argv)
{
    int fd;
    const char *host = DEFADDR;
    const char *port = DEFPORT;

    if (argc < 1 || argc > 3)
        die("usage: %s [host] [port]", argv[0]);

    if (argc > 1)
        host = argv[1];
    if (argc > 2)
        port = argv[2];

    setbuf(stdout, NULL);

    fd = tcpopen(host, port);

    if (atexit(leave_raw_mode) != 0)
        die("atexit:");
    enter_raw_mode();

    clientloop(fd);
    close(fd);
}

/* main communication routine */
void clientloop(int fd)
{
    int n;
    char buf[4096];
    struct winsize ws;

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
        die("ioctl-TIOCGWINSZ:");

    /* send window size to the server */
    if(write(fd, &ws, sizeof(ws)) != sizeof(ws))
        die("write winsize:");

    fd_set rfds;
    FD_ZERO(&rfds);

    for (;;) {
        FD_SET(0, &rfds);
        FD_SET(fd, &rfds);

        if (select(fd + 1, &rfds, NULL, NULL, NULL) < 0)
            die("select:");

        if (FD_ISSET(0, &rfds)) {
            if ((n = read(0, buf, sizeof(buf))) <= 0)
                break;

            if (n = write(fd, buf, n) != n)
                die("write to server:");
        }

        if (FD_ISSET(fd, &rfds)) {
            if ((n = read(fd, buf, sizeof(buf))) <= 0)
                break;

            if (write(0, buf, n) != n)
                die("write to stdout:");
        }
    }
}

/* connects a TCP socket to the host
 *
 * Host can either be an ip address or
 * a domain name. Port can either be a
 * numeric port, or a service name
 */
int tcpopen(const char *host, const char *port)
{
    struct addrinfo hints, *res = NULL, *rp;
    int fd = -1, e;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_socktype = SOCK_STREAM;

    if ((e = getaddrinfo(host, port, &hints, &res)))
        die("getaddrinfo: %s", gai_strerror(e));

    for (rp = res; rp; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

        if (fd < 0)
            continue;

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) < 0) {
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }

    freeaddrinfo(res);

    if (fd < 0)
        die("could not connect to %s:%s:", host, port);

    return fd;
}

void leave_raw_mode()
{
    if (!in_raw_mode)
        return;

    if (tcsetattr(0, TCSADRAIN, &saved_tio) < 0)
        perror("tcsetattr");
    else
        in_raw_mode = 0;
}

void enter_raw_mode()
{
    struct termios tio;

    if (tcgetattr(0, &tio) == -1)
        die("tcgetattr");

    saved_tio = tio;
    tio.c_iflag |= IGNPAR;
    tio.c_iflag &= ~(ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXANY | IXOFF);
#ifdef IUCLC
    tio.c_iflag &= ~IUCLC;
#endif
    tio.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL);
#ifdef IEXTEN
    tio.c_lflag &= ~IEXTEN;
#endif
    tio.c_oflag &= ~OPOST;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSADRAIN, &tio) == -1)
        die("tcsetattr");
    else
        in_raw_mode = 1;
}

