#include "tcp_client.h"

int main(int argc, char **args) {
  if (argc < 2) 
    return 1;

  uint16_t port = (uint16_t)atoi(args[1]);

  client_init(port);
  client_connection_handle();
  client_deinit();
  
  return 0;
}