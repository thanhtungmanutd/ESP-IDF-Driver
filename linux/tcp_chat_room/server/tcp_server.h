#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <libxml/tree.h>
#include <libxml/

typedef struct {
  char **nameList;
  char **passList;
} database_t;

typedef enum {
  Login    = 0,
  Register = 1,
} option_t;

typedef enum {
  Single = 1,
  Multi  = 2,
  Exit   = 3,
} chat_option_t;

typedef enum {
  receiveClientRequest    = 0,
  receiveClientChatOption = 1,
  Online                  = 2,
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

void server_init(uint16_t port);
void server_handle_clients_connection(void);
void server_deinit(void);
#endif /* _TCP_SERVER_H_ */