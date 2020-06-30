#include <stdio.h>     // for fprintf()
#include <unistd.h>    // for close()
#include <sys/epoll.h> // for epoll_create1()
#include <string.h>    // for strncmp
#include <sys/types.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define MAX_EVENTS 5
#define READ_SIZE 10


int createAndListenSocket() {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    printf("socket fd = %d\n", server_fd);
    if (server_fd < 0) {
        exit(1);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(5555);
    if (bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        exit(1);
    }
    listen(server_fd, 5);
    return server_fd;
}
     
int main() {
    int running = 1, event_count, i;
    size_t bytes_read;
    char read_buffer[READ_SIZE + 1];
    struct epoll_event event, events[MAX_EVENTS];
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
      fprintf(stderr, "Failed to create epoll file descriptor\n");
      return 1;
    }
    event.events = EPOLLIN;
    event.data.fd = 0;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, createAndListenSocket(), &event)) {
      fprintf(stderr, "Failed to add file descriptor to epoll\n");
      close(epoll_fd);
      return 1;
    }
    while (running) {
        printf("\nPolling for input...\n");
        event_count = epoll_wait(epoll_fd, events, MAX_EVENTS, 30000);
        printf("%d ready events\n", event_count);
        for (i = 0; i < event_count; i++) {
          printf("Reading file descriptor '%d' -- ", events[i].data.fd);
          bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
          printf("%zd bytes read.\n", bytes_read);
          read_buffer[bytes_read] = '\0';
          printf("Read '%s'\n", read_buffer);
          if (!strncmp(read_buffer, "stop\n", 5)) {
            running = 0;
          }
        }
    }
    if (close(epoll_fd)) {
      fprintf(stderr, "Failed to close epoll file descriptor\n");
      return 1;
    }
    return 0;
}