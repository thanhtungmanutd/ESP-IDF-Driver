#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>

typedef enum {
  Login    = 0,
  Register = 1,
} option_t;

typedef enum {
  sendRequest      = 0,
  selectChatOption = 1,
  Onlined          = 2,
} state_t;

typedef struct {
  char userName[100];
  char password[100];
  option_t option;
} request_message_t;

typedef enum {
  SUCCESS                     = 0,
  ERROR_DUPLICATE_USERNAME    = 1,
  ERROR_USERNAME_NOT_FOUND    = 2,
  ERROR_PASSWORD_NOT_MATCHING = 3,
} response_t;

void client_init(uint16_t port);
void client_connection_handle(void);
void client_deinit(void);
#endif /* _TCP_CLIENT_H_ */