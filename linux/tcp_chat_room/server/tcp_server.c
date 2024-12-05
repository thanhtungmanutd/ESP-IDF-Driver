#include "tcp_server.h"

#define MAX_CLIENT_CONNECTED      100
#define DATABASE_PATH             "database.txt"

#define ALLOC_DATABASE(dtb)                               \
  dtb.nameList = malloc(100 * sizeof(char*));             \
  dtb.passList = malloc(100 * sizeof(char*));             \
                                                          \
  if ((dtb.nameList == NULL) ||                           \
      (dtb.passList == NULL)) {                           \
    printf("Server: failed to allocate dabatbase\r\n");   \
  }                                                       \
  for (int i = 0; i < 100; i++) {                         \
    dtb.nameList[i] = malloc(256);                        \
    dtb.passList[i] = malloc(256);                        \
    if ((dtb.nameList[i] == NULL) ||                      \
      (dtb.passList[i] == NULL)) {                        \
      printf("Server: failed to allocate dabatbase\r\n"); \
      break;                                              \
    }                                                     \
  }

#define FREE_DATABASE(dtb)                    \
  for (int i = 0; i < 100; i++) {             \
    free(dtb.nameList[i]);                    \
    free(dtb.passList[i]);                    \
  }                                           \
  free(dtb.nameList);                         \
  free(dtb.passList);                         

static int sock_fd = 0;
static int new_sockfd = 0;
static FILE *dtb_ptr = NULL;

static bool is_file_empty(FILE *file);
static void querry_database(database_t *dtb,
                            char **nameList,
                            char **passList);
static response_t checkUserInfo(database_t *dtb,
                                char *userName,
                                char *password,
                                option_t op);

void server_init(uint16_t port) {
  struct sockaddr_in server_addr;

  // create socket server
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    printf("Server: failed to create socket\r\n");
    exit(EXIT_FAILURE);
  }

  // bind socket server
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(port);

  if (bind(sock_fd,
      (struct sockaddr *)&server_addr,
      sizeof(server_addr)) < 0) {
    printf("Server: failed to bind socket\r\n");
    exit(EXIT_FAILURE);
  }

  printf("Chat Server: Server Port Information: %s:%d\r\n",
         inet_ntoa(server_addr.sin_addr), port);

  if (listen(sock_fd, 10) < 0) {
    printf("Server: failed to listen to clients\r\n");
    exit(EXIT_FAILURE);
  }

  // open database file
  dtb_ptr = fopen(DATABASE_PATH, "r+");
  if (dtb_ptr < 0) {
    printf("Server; failed to open databse\r\n");
    exit(EXIT_FAILURE);
  }

  // register Ctrl + C signal to terminate connection
  // signal(SIGINT, signal_handler);
}

static void handle_single_client_connection(struct sockaddr_in *sockaddr) {
  state_t state = receiveClientRequest;
  request_message_t req;
  database_t dtb;

  ALLOC_DATABASE(dtb)
  // querry databse
  querry_database(&dtb, dtb.nameList, dtb.passList);

  while (1) {
    switch (state) {
      case receiveClientRequest:
        if (read(new_sockfd,
            &req,
            sizeof(request_message_t)) < 0) {
          printf("Server: failed to read client message\r\n");
          exit(EXIT_FAILURE);
        }
        if (req.option == Register)
          printf("Chat Server: Incoming registration request at %s:%d\r\n",
                  inet_ntoa(sockaddr->sin_addr),
                  ntohs(sockaddr->sin_port));
        if (req.option == Login)
          printf("Chat Server: Incoming Login request at %s:%d\r\n",
                  inet_ntoa(sockaddr->sin_addr),
                  ntohs(sockaddr->sin_port));
        response_t res = checkUserInfo(&dtb,
                                       req.userName,
                                       req.password,
                                       req.option);
        send(new_sockfd, &res, sizeof(res), 0);
        if (res == SUCCESS) {
          printf("Chat Server: %s joined the Chat Room at %s:%d\r\n",
                 req.userName,
                 inet_ntoa(sockaddr->sin_addr),
                 ntohs(sockaddr->sin_port));
          state = receiveClientChatOption;
        }
        break;

      case receiveClientChatOption:
        int op;
        if (read(new_sockfd, &op, sizeof(op)) < 0) {
          printf("Server: failed to read client message\r\n");
          exit(EXIT_FAILURE);
        }
        if (op == Single) 
          printf("Chat Server: %s selected Single user chat\r\n", req.userName);
        if (op == Multi)
          printf("Chat Server: %s selected Multi user chat\r\n", req.userName);
        if (op == Exit) {
          printf("Chat Server: %s left the Chat Room\r\n", req.userName);
        }
        state = Online;
        break;

      default:
        break;
    }
  }

  // free database
  FREE_DATABASE(dtb)
}

static response_t checkUserInfo(database_t *dtb,
                                char *userName,
                                char *password,
                                option_t op) {
  char line[256];

  if ((op == Register) && is_file_empty(dtb_ptr)) {
    int numchr = snprintf(line, 256, "%s %s", userName, password);
    int num = fwrite(line, numchr, 1, dtb_ptr);
    return SUCCESS;
  }

  for (int i = 0; i < 100; i++) {
    if (strcmp(userName, dtb->nameList[i]) == 0) {
      if (op == Login) {
        if (strcmp(password, dtb->passList[i]) == 0) 
          return SUCCESS;
        else 
          return ERROR_PASSWORD_NOT_MATCHING;
      }
      if (op == Register) 
        return ERROR_DUPLICATE_USERNAME;
    }
  }
  if (op == Register) {
    fseek(dtb_ptr, 0, SEEK_SET);
    int numchr = snprintf(line, 256, "%s %s\n", userName, password);
    int num = fwrite(line, numchr, 1, dtb_ptr);
    return SUCCESS;
  }
  return ERROR_USERNAME_NOT_FOUND;
}

void server_handle_clients_connection(void) {
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t len;

    // accept connection from client
    new_sockfd = accept(sock_fd,
                        (struct sockaddr *)&client_addr,
                        &len);
    if (new_sockfd < 0) {
      printf("fail to connect to client\r\n");
    } else {
      pid_t child_pid = fork();
      if (child_pid < 0) {
        printf("Server: failed to fork() child process\r\n");
      } else if (child_pid == 0) {
        getpeername(new_sockfd, (struct sockaddr *)&client_addr, &len);
        handle_single_client_connection(&client_addr);
      }
    }
  }
}

static bool is_file_empty(FILE *file) {
  fseek(file, 0, SEEK_SET);
  int c = getc(file);
  if (c == EOF) 
    return true;
  ungetc(c, file);
  return false;
}

static void querry_database(database_t *dtb,
                            char **nameList,
                            char **passList) {
  char line[256] = {0};

  fseek(dtb_ptr, 0, SEEK_SET);
  while (fgets(line, sizeof(line), dtb_ptr)) {
    if (line[strlen(line) - 1] == '\n')
      line[strlen(line) - 1] = '\0';
    char *userName = strtok(line, "|");
    char *userPass = strtok(NULL, "|");
    printf("%s %s\r\n", userName, userPass);
    memcpy(*nameList, userName, strlen(userName));
    memcpy(*passList, userPass, strlen(userPass));
    memset(line, 0, sizeof(line) / sizeof(char));
    nameList++;
    passList++;
  }
}

void server_deinit(void) {
  if ((close(new_sockfd) < 0) || (close(sock_fd) < 0)) {
    printf("Server: failed to close socket\r\n");
    exit(EXIT_FAILURE);
  }

  if (fclose(dtb_ptr) < 0) {
    printf("Server: failed to close database\r\n");
    exit(EXIT_FAILURE);
  }
}