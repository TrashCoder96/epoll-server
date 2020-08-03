#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <unistd.h>    // for close()
#include <string.h>    // for strncmp
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define MAX_EVENTS 16
#define READ_SIZE 4096

int createAndListenSocket() {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
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

void doCycle(int server_fd, int epfd, struct epoll_event *events) {
  char read_buffer[READ_SIZE];
  struct epoll_event client_event;
  printf("\nPolling for input...\n");
  int event_count = epoll_wait(epfd, events, MAX_EVENTS, 3000);
  printf("%d ready events\n", event_count);
  for (int i = 0; i < event_count; i++) {
    printf("Reading file descriptor '%d' -- ", events[i].data.fd);
    int client_fd;
    /* if first event is occured */
    if (events[i].data.fd == server_fd) {
      struct sockaddr_in their_addr;
      int socklen;
      client_fd = accept(server_fd, (struct sockaddr*)&their_addr, &socklen);
      memset(&client_event, 0, sizeof(client_event));
      client_event.events = EPOLLIN | EPOLLONESHOT;
      if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_event)) {
        fprintf(stderr, "Failed to add file descriptor to epoll\n");
        close(epfd);
      }
    } else { /* if next event is occured */
      if (events[i].events | EPOLLIN) {
        int bytes_read = read(events[i].data.fd, read_buffer, READ_SIZE);
        printf("bytes_read = %d\n", bytes_read);
      }
    }
  }
}


int main() {
  int epfd = epoll_create1(0);
  struct epoll_event event, events[MAX_EVENTS];
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN;
  int server_fd = createAndListenSocket();
  event.data.fd = server_fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &event)) {
    fprintf(stderr, "Failed to add file descriptor to epoll\n");
    close(epfd);
    return 1;
  }
  while (1) {
    doCycle(server_fd, epfd, &events[0]);
  }
}