#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
   pid_t child_pid = fork();

   if(child_pid < 0) {
      printf("Failed to create child process\r\n");     
   } if(child_pid == 0) {
      printf("Child process\r\n")
   }

   return 0;
}