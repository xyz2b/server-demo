#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <zconf.h>
#include <csignal>
#include <string>

#define PORT 8889

void do_service(int fd);

void sighandler(int sig) {
    if (SIGINT == sig) {
        printf("ctrl+c pressed\n");
        exit(-1);
    }
}

void print_client_info(struct sockaddr_in* p) {
    int port = htons(p->sin_port);

    char ip[16];
    memset(ip, 0, sizeof(ip));

    inet_ntop(AF_INET, &(p->sin_addr.s_addr), ip, sizeof(ip));

    printf("client connected: %s(%d)\n", ip, port);
}

int main() {
    // 0.process sigint signal
    if (SIG_ERR != signal(SIGINT, sighandler)) {
        printf("signal SIGINT success\n");
    }

    if (SIG_ERR != signal(SIGPIPE, sighandler)) {
        printf("signal SIGPIPE success\n");
    }


    // 1.create socket
    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (-1 == server_sock) {
        perror("socket failed");
        exit(-1);
    }

    // port reuse
    int opt = 1;
    if (-1 == setsockopt(server_sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(-1);
    }

    if ( -1 == setsockopt(server_sock,  IPPROTO_TCP,  TCP_NODELAY, &opt, sizeof(opt))) {
        perror("setsockopt TCP_NODELAY failed");
        exit(-1);
    }


    // 2.create sockaddr_in, set listen address and port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    // os: small
    // network: big
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // 3. bind
    int status = bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (0 != status) {
        perror("bin failed");
        exit(-1);
    }

    // 4.listen
    status = listen(server_sock, 10);
    if (-1 == status) {
        perror("listen failed");
        exit(-1);
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));

    while (true) {
        printf("wait connect...\n");

        // 5.accept a new client connect, create client socket
        int client_fd = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);

        print_client_info(&client_addr);

        // 6.read or write from client socket
        do_service(client_fd);

        // 7.close client socket
        close(client_fd);
    }

    // 8.close server socket
    close(server_sock);
    return 0;
}

void do_service(int client_fd) {
    // write to client "hello"
    std::string s = "hello\n";

    int writesize = write(client_fd, s.c_str(), s.length());
    printf("write size: %d\n", writesize);
    if (s.length() != writesize) {
        perror("write failed");

        if (errno == EPIPE) {
            printf("client quit(read side quit, cannot write to client)\n");
            exit(-1);
        }
    }

    char buff[1024] = {0};
    int readsize = 0;

    while (1) {
        memset(buff, 0, sizeof(buff));

        readsize = read(client_fd, buff, sizeof(buff));
        if (-1 == readsize) {
            perror("read failed");
            exit(-1);
        } else if (0 == readsize) {
            printf("client quit(write side quit)\n");
            break;
        }

        printf("client say: %s\n", buff);
        if (!strcmp("bye", buff)) {
            break;
        }
    }
}
