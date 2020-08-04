#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

int setnonblocking(int sockfd) {
   fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);
   return 0;
}

int handleAddConnection(int epfd, int server_fd) {
  struct sockaddr_in their_addr;
  struct epoll_event event;
  int socklen;
  int client_fd;

  client_fd = accept(server_fd, (struct sockaddr *) &their_addr, &socklen);
  setnonblocking(client_fd);
  event.data.fd = client_fd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);
  return 0;
}

int handleEvent(int client_id) {
  int len;
  char buf[READ_SIZE];

  len = recv(client_id, &buf, READ_SIZE, 0);
  printf("%s", buf);
  return 0;
}

int createAndListenSocket() {
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setnonblocking(server_fd);
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
    listen(server_fd, 1);
    return server_fd;
}

void doCycle(int server_fd, int epfd, struct epoll_event *events) {
  printf("\nPolling for input...\n");
  int event_count = epoll_wait(epfd, events, MAX_EVENTS, 6000);
  printf("%d ready events\n", event_count);
  for (int i = 0; i < event_count; i++) {
    printf("Reading file descriptor '%d' -- \n", events[i].data.fd);
    /* if first event is occured */
    if (events[i].data.fd == server_fd) {
      handleAddConnection(epfd, server_fd);
    } else if (events[i].events | EPOLLIN) { /* if next event is occured */
      handleEvent(events[i].data.fd);
    }
  }
}


int main() {
  int epfd = epoll_create1(0);
  struct epoll_event event, events[MAX_EVENTS];
  memset(&event, 0, sizeof(event));
  event.events = EPOLLIN | EPOLLET;
  int server_fd = createAndListenSocket();
  event.data.fd = server_fd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &event)) {
    fprintf(stderr, "Failed to add file descriptor to epoll\n");
    close(epfd);
    return 1;
  }
  for(;;) {
    doCycle(server_fd, epfd, &events[0]);
  }
}