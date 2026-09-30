#include <iostream>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0)
  {
    std::cerr << "failed to create server socket server_fd: " << server_fd << "\n";
    return 1;
  }

  // ip address and port at which we want our server_socket to listen
  struct sockaddr_in address;
  socklen_t sock_len = sizeof(struct sockaddr_in);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8080);

  // binding our server_socket to that ip address and port struct
  if (int ret = bind(server_fd, (struct sockaddr *)&address, sock_len); ret < 0)
  {
    std::cerr << "failed to bind server_fd to ip port: " << ret << "\n";
    return 1;
  }
  std::cout << "one\n";

  // listening to first 3 connections
  if (int ret = listen(server_fd, 2); ret < 0)
  {
    std::cerr << "failed to listen to more requests furture: " << ret << "\n";
    return 1;
  }
  std::cout << "two\n";

  while (true)
  {
    // accept a client connection (blocking)
    int new_socket_fd = accept(server_fd, (sockaddr *)&address, &sock_len);
    if (new_socket_fd <= 0)
    {
      std::cerr << "accept failed: " << new_socket_fd << "\n";
      return 1;
    }
    std::cout << "three\n";

    // send and recv with accepted client socket

    constexpr size_t buffer_size = 1024;
    char buffer[buffer_size] = {0};
    if (int ret = recv(new_socket_fd, buffer, buffer_size, 0); ret < 0)
    {
      std::cerr << "failed to recv: " << ret << "\n";
      return 1;
    }
    std::cout << "received buffer: " << buffer << "\n";
    std::cout << "four\n";

    const char *response = "Message received loud and clear\n";
    if (int ret = send(new_socket_fd, response, strlen(response), 0); ret < 0)
    {
      std::cerr << "failed to write: " << ret << "\n";
      return 1;
    }
    std::cout << "five\n";
    close(new_socket_fd);
  }

  close(server_fd);
  return 0;
}