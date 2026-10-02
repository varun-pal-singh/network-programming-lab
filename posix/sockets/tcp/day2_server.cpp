#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <thread>

constexpr int32_t BUFFER_SIZE = 256;
constexpr int32_t SEND_Q_SIZE = 2;

int32_t READ_Q_SIZE = 0;

class SocketGuard {
 private:
  int32_t fd;

 public:
  explicit SocketGuard(int32_t f) : fd(f) {};

  SocketGuard(const SocketGuard&) = delete;            // copy constructor disabled
  SocketGuard operator=(const SocketGuard&) = delete;  // copy assignment operator disabled

  ~SocketGuard() {
    if (fd >= 0) close(fd);
  }
};

void handle_client_communication(int32_t client_fd) {
  SocketGuard s_guard(client_fd);

  char recv_buffer[BUFFER_SIZE] = {0};
  char send_buffer[BUFFER_SIZE] = {0};

  int32_t send_len = snprintf(send_buffer, BUFFER_SIZE, "server: hello client_fd: %d\n", client_fd);

  if (int32_t ret = send(client_fd, send_buffer, send_len, 0); ret < 0) {
    printf("Error sending response to client, ret %d\n", ret);
    exit(1);
  }

  while (true) {
    // recv
    memset(recv_buffer, 0, BUFFER_SIZE);
    memset(send_buffer, 0, BUFFER_SIZE);

    int32_t received_bytes = recv(client_fd, recv_buffer, BUFFER_SIZE - 1, 0);

    if (received_bytes < 0) {
      printf("Error in receiving, ret: %d\n", received_bytes);
      READ_Q_SIZE--;
      close(client_fd);
      return;
    } else if (received_bytes == 0) {
      printf("client_fd: %d gracefully shutdown.\n", client_fd);
      break;
    } else if (received_bytes > 0) {
      recv_buffer[received_bytes] = '\0';
    }

    printf("client_fd %d msg: %s\n", client_fd, recv_buffer);

    // send
    send_len = snprintf(send_buffer, BUFFER_SIZE, "server msg received: %s\n", recv_buffer);

    if (int32_t ret = send(client_fd, send_buffer, send_len, 0); ret < 0) {
      printf("Error sending response to client, ret %d\n", ret);
      exit(1);
    }
  }

  READ_Q_SIZE--;
  close(client_fd);
}

int32_t main() {
  // server socket creation
  int32_t server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    printf("server_fd creation failed, server_fd: %d\n", server_fd);
    exit(1);
  }

  int32_t opt = 1;
  if (int32_t ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int32_t));
      ret < 0) {
    printf("can not set option to reuse address, ret: %d\n", ret);
    close(server_fd);
    exit(1);
  }

  if (int32_t ret = setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(int32_t));
      ret < 0) {
    printf("can not set option to reuse port, ret: %d\n", ret);
    close(server_fd);
    exit(1);
  }

  constexpr int32_t port_no = 8080;

  // address to bind to
  sockaddr_in address{};
  socklen_t addr_len = sizeof(sockaddr);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port_no);

  if (int32_t ret = bind(server_fd, (sockaddr*)&address, addr_len); ret < 0) {
    printf("binding failed at address: %d and port: %d\n", address.sin_addr.s_addr,
           address.sin_port);
    exit(1);
  }

  constexpr int32_t listening_queue_size = 2;

  if (int32_t ret = listen(server_fd, listening_queue_size); ret < 0) {
    printf("listening failed, can not listen to more connections, ret: %d\n", ret);
    exit(1);
  }

  while (true) {
    int32_t client_fd = accept(server_fd, (sockaddr*)&address, (socklen_t*)&addr_len);

    if (client_fd < 0) {
      printf("Error in client_fd: %d\n", client_fd);
      exit(1);
    }

    if (READ_Q_SIZE < SEND_Q_SIZE) {
      std::thread t1(handle_client_communication, client_fd);
      t1.detach();
      READ_Q_SIZE++;
    }
  }

  close(server_fd);
  return 0;
}