//Taken from Abhishek Sagar

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include "common.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

/*Server process is running on this port no. Client has to send data to this port no*/
#define SERVER_PORT 2000

#define BUFFER_SIZE 1024

#define CONNECTIONS_COUNT 8

client_data_t test_struct;

result_struct_t res_struct;

char data_buffer[BUFFER_SIZE];

pthread_t threads[CONNECTIONS_COUNT];

int master_sock_fd;

int addr_len;

void* handle_client_connection(void *arg)
{
    request_struct_t *request_struct = (request_struct_t*)arg;

    printf("Server with thread id %ld recvd %d bytes from client %s:%u\n",
            pthread_self(),
            request_struct->sent_recv_bytes,
            inet_ntoa(request_struct->addr.sin_addr),
            ntohs(request_struct->addr.sin_port));

    result_struct_t result;
    char result_str[256];
    strcpy(result_str, request_struct->data.name);
    strcat(result_str, request_struct->data.group);
    strcpy(result.result, result_str);

    /* Server replying back to client now*/
    int sent_recv_bytes = sendto(master_sock_fd, (char *)&result, sizeof(result_struct_t), 0,
                             (struct sockaddr *)&request_struct->addr, sizeof(struct sockaddr));

    printf("Server sent %d bytes in reply to client\n", sent_recv_bytes);
    /*Goto state machine State 3*/
    sleep(10);
}

void setup_udp_server_communication()
{
   master_sock_fd = 0;

   struct sockaddr_in server_addr;

   if ((master_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
   {
       printf("socket creation failed\n");
       exit(1);
   }

   server_addr.sin_family = AF_INET;
   server_addr.sin_port = SERVER_PORT;
   server_addr.sin_addr.s_addr = INADDR_ANY; 

   addr_len = sizeof(struct sockaddr);

   if (bind(master_sock_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
   {
       printf("socket bind failed\n");
       return;
   }

   int current_thread_index = 0;

   while(1)
   {
       struct sockaddr_in client_addr;

       memset(data_buffer, 0, sizeof(data_buffer));

       printf("waits for client request...\n");

       int sent_recv_bytes = recvfrom(master_sock_fd, data_buffer, BUFFER_SIZE - 1, 0,
                                      (struct sockaddr *)&client_addr, &addr_len);

       client_data_t *client_data;
       client_data = (client_data_t *)data_buffer;

       request_struct_t *request_struct;
       request_struct = malloc(sizeof(request_struct_t));
       request_struct->addr = client_addr;
       request_struct->data = *client_data;
       request_struct->sent_recv_bytes = sent_recv_bytes;

       if (pthread_create(&threads[current_thread_index], NULL, handle_client_connection, request_struct))
       {
           printf("error while thread creation...\n");
       }
       else
       {
           current_thread_index %= (current_thread_index + 1) % CONNECTIONS_COUNT;
       }
   }
}

int main(int argc, char **argv){
    setup_udp_server_communication();
    return 0;
}
