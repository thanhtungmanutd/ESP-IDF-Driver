#include "tcp_server.h"

int main(int argc, char **args) {
  if (argc < 2) 
    return 1;

  uint16_t port = (uint16_t)atoi(args[1]);

  server_init(port);
  server_handle_clients_connection();
  server_deinit();
  return 0;
}