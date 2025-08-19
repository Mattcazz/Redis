#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

const size_t k_max_msg = 4096;

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

static int32_t read_full(int fd, char *buf, size_t n){
    while(n > 0){
        ssize_t rv  = read(fd, buf, n);
        if (rv <= 0){
            return -1; // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0; 
}

static int32_t write_all (int fd, char *buf, size_t n){
    while(n > 0){
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0){
            return -1; // error 
        }
         assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }

    return 0; 
}


static int32_t one_request(int connfd){
    // 4 bytes header 
    char rbuf[4 + k_max_msg];
    errno = 0; 
    int32_t err = read_full(connfd, rbuf, 4);
    
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err; 
    }

    uint32_t len = 0; 

    memcpy(&len, rbuf, 4); // assume little endian 
    
    if (len > k_max_msg) {
        msg("too long");
        return -1; 
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err){
        msg("read() error");
        return err;
    }

    // do something 
    printf("client says: %.*s\n", len, &rbuf[4]);
    
    // reply using the same protocol 
    const char reply[] = "world";
    char wbuf [4 + sizeof(reply)];
    len = (uint32_t)strlen(reply);
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], &reply, len);
    return write_all(connfd, wbuf, 4+len);

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
    if (rv) { die("listen()");}

    // accept connections 
    while (true){
        // accept 
        struct sockaddr_in client_addr = {};
        socklen_t addrlen = sizeof(client_addr);
        int connfd = accept (fd, (struct sockaddr *)&client_addr, &addrlen);
        if (connfd < 0) {
            continue; // error 
        }

        // nly serves one client connection at once 
        while(true){
            int32_t err = one_request(connfd);
            if (err){
                break; 
            }
        }
        
        close(connfd);
    }

}