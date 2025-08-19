#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}
/*AF_INET is for IPv4. Use AF_INET6 for IPv6 or dual-stack sockets.
SOCK_STREAM is for TCP. Use SOCK_DGRAM for UDP.
The 3rd argument is 0 and useless for our purposes.*/

static int getSocketHandle() {
    return socket(AF_INET, SOCK_STREAM, 0); 
}

static void do_something(int connfd){
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);

    if (n < 0) {
        msg("read() error");
        return; 
    }

    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}


int main () {
    int fd = getSocketHandle();
    if (fd < 0) {
        die("socket()");
    }

    // Setup options 
    // this is needed for most server applications
    int val = 1; 
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    
    // Binding 
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);        // port
    addr.sin_addr.s_addr = htonl(0);    // wildcard IP 0.0.0.0
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { die("bind()"); }

    // listen 
    rv = listen(fd, SOMAXCONN);
    if (rv) { die("liste()");}

    // accept connections 
    while (true){
        // accept 
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept (fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            continue; // error 
        }
        
        do_something(connfd);
        close(connfd);
    }

}