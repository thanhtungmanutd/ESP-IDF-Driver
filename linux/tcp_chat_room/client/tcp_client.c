#include "tcp_client.h"

#define CONNECTION_TIMEOUT            5

static int sock_fd = 0;
static struct sockaddr_in server_addr;

static response_t clientSendRequest(request_message_t *msg);
static response_t clientSelectChatoption(void);

void client_init(uint16_t port) {
  // create socket client
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    printf("Client: failed to create socket\r\n");
    exit(EXIT_FAILURE);
  }

  // connect to server
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port = htons(port);

  // trying to connect to server
  int time = CONNECTION_TIMEOUT;
  while (time--) {
    int status = connect(sock_fd,
                         (struct sockaddr *)&server_addr,
                         sizeof(server_addr));
    if (status == 0)
      break;
    sleep(1);
  }
  if (time < 0) {
    printf("Client: failed to connect to server timeout\r\n");
    exit(EXIT_FAILURE);
  }
}

void client_connection_handle(void) {
  state_t state = sendRequest;
  int option;
  request_message_t msg;
  response_t res;

  printf("INFO: Connected to the Server\r\n\n");
  printf("1. Login\r\n2. Register\r\n3. Exit\r\n\n");
  printf("Enter your Option: ");
  scanf("%d", &option);

  msg.option = (option_t)(option - 1);

  while (1) {
    switch (state) {
      case sendRequest:
        printf("\r\nEnter User Name: ");
        scanf("%s", msg.userName);
        printf("\rEnter Password: ");
        scanf("%s", msg.password);

        // send request
        response_t res = clientSendRequest(&msg);
        if (res == SUCCESS) {
          if (msg.option == Register)
            printf("\nINFO: User Registration Successful\r\n\n");
          if (msg.option == Login) 
            printf("\nINFO: User Login Successful\r\n\n");
          state = selectChatOption;
        } else if (res == ERROR_DUPLICATE_USERNAME) {
          printf("\nINFO: User Registration Failed (Duplicate User Name)\r\n\n");
        }  else if (res == ERROR_USERNAME_NOT_FOUND) {
          printf("\nINFO: User Log In Failed (User Name Not Found)\r\n\n");
        }  else if (res == ERROR_PASSWORD_NOT_MATCHING) {
          printf("\nINFO: User Log In Failed (Password Not Matching)\r\n\n");
        }
        break;
        
      case selectChatOption:
        if (clientSelectChatoption() == SUCCESS) {
          state = Onlined;
        }
        break;
      
      case Onlined:
        break;
      default:
        break;
    }
  }
}

static response_t clientSendRequest(request_message_t *msg) {
  response_t res = SUCCESS;

  if (send(sock_fd, msg, sizeof(request_message_t), 0) < 0) {
    printf("Client: failed to send request message\r\n");
    exit(EXIT_FAILURE);
  }

  if (read(sock_fd, &res, sizeof(res)) < 0) {
    printf("Client: failed to read response message\r\n");
    exit(EXIT_FAILURE);
  }
  return res;
}

static response_t clientSelectChatoption(void) {
  int op;
  response_t res;

  printf("Chat option:\n\r1: Single User Chat\r\n2: Multi User Chat\r\n3: Exit\r\n\n");
  printf("Select what you would like to proceed with: ");
  scanf("%d", &op);


  if (send(sock_fd, &op, sizeof(op), 0) < 0) {
    printf("Client: failed to send chat option message\r\n");
    exit(EXIT_FAILURE);
  }
  if (op == 3) 
    exit(EXIT_SUCCESS);
  if (read(sock_fd, &res, sizeof(res)) < 0) {
    printf("Client: failed to read chat option message\r\n");
    exit(EXIT_FAILURE);
  }

  return res;
}
void client_deinit(void) {
  close(sock_fd);
}