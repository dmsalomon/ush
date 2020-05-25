
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pty.h>
#include <unistd.h>
#include <termios.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "util.h"

char *argv0 = "ushd";

void sshserve(int);
pid_t hardfork(void);
void shell(void);
int tcpbind(const char *);

int main(int argc, char **argv)
{
    int sfd, cfd;
    struct sockaddr_in peer_addr;
    socklen_t peer_sz = sizeof(peer_addr);

    setbuf(stdout, NULL);

    sfd = tcpbind(DEFPORT);

    while ((cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_sz)) != -1) {
        sshserve(cfd);
    }

    close(sfd);
}

/* serve a client in a child proc */
void sshserve(int cfd)
{
    int mfd, maxfd, n;
    char buf[4096];
    char pts[1000];
    pid_t pid;
    fd_set ifds;
    struct winsize ws;

    pid = hardfork();
    if (pid < 0) {
        die("fork:");
    } else if (pid > 0) {
        close(cfd);
        return;
    }

    /* read the windows size from the client */
    if (read(cfd, &ws, sizeof(ws)) < 0)
        die("read client winsize:");

    /* fork the shell process in a new pty */
    pid = forkpty(&mfd, pts, NULL, &ws);
    if (pid < 0) {
        die("forkpty:");
    } else if (pid == 0) {
        shell();
    }

    /* optionally, disconnect the child from the tty */
    /* setsid(); */

    printf("pts: %s: new\n", pts);

    /* ferry data across the socket */
    maxfd = MAX(mfd, cfd);

    for (;;) {
        FD_ZERO(&ifds);
        FD_SET(mfd, &ifds);
        FD_SET(cfd, &ifds);

        if (select(maxfd+1, &ifds, NULL, NULL, NULL) < 0)
            die("select:");

        if (FD_ISSET(cfd, &ifds)) {
            if ((n = read(cfd, buf, sizeof(buf))) <= 0)
                break;

            if (write(mfd, buf, n) != n)
                die("write to pty:");
        }

        if (FD_ISSET(mfd, &ifds)) {
            if ((n = read(mfd, buf, sizeof(buf))) <= 0)
                break;

            if (write(cfd, buf, n) != n)
                die("write to socket:");
        }
    }

    close(mfd);
    close(cfd);

    if (wait(NULL) < 0)
        die("wait:");

    printf("pts: %s: done\n", pts);
    exit(0);
}

void shell()
{
    if (geteuid() == 0) {
        execlp("/bin/login", "/bin/login", "-p", "--", (char *) NULL);
    } else {
        char *shell = getenv("SHELL");
        if (!shell || !*shell)
            shell = "/bin/sh";
        execlp(shell, shell, (char *) NULL);
    }
    die("execlp:");
}

/* double fork to prevent zombies */
pid_t hardfork()
{
    pid_t pid = fork();

    if (pid > 0) {
        if(wait(NULL) < 0)
            die("wait:");
    } else if (pid == 0) {
        pid = fork();
        if (pid < 0)
            die("fork:");
        else if (pid > 0)
            _exit(0);
    }

    return pid;
}

/*
 * allocate the socket and bind to port
 */
int tcpbind(const char *strport)
{
    int fd;
    uint16_t nport;
    struct sockaddr_in sin;

    if (!(nport = atoport(strport)))
        die("%s: not a valid port number", strport);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(nport);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        die("socket():");

    /* Make the port immediately reusable after termination */
    int reuse = 1;
    if (setsockopt
        (fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
         sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

#ifdef SO_REUSEPORT
    if (setsockopt
        (fd, SOL_SOCKET, SO_REUSEPORT, (const char *)&reuse,
         sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEPORT) failed");
#endif

    if (bind(fd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        die("bind():");

    if (listen(fd, 50) < 0)
        die("listen():");

    return fd;
}
