#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>

#define MAIN_PORT 8080
#define MAX_NUMBER_OF_PEERS 12
#define BUFFER_SIZE 256
#define GET_PEERS_REQUEST "GET PEERS"
#define SENDING_FINISHED "SENDING FINISHED"
#define PEERS_REFRESH_TIME 10

#define IS_FIRST_PEER 1
#define FIRST_PEER_IP "127.0.0.0"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct peer {
    struct in_addr addr;
};

struct peer* peers[MAX_NUMBER_OF_PEERS];

socklen_t addr_len = sizeof(struct sockaddr);

int number_of_peers = 0;

void add_peer(struct peer *new_peer)
{
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_NUMBER_OF_PEERS; i++)
    {
        if (peers[i] == new_peer) {
            pthread_mutex_unlock(&lock);
            return;
        }
    }

    for (int i = 0; i < MAX_NUMBER_OF_PEERS; i++)
    {
        if (peers[i] == NULL) {
            peers[i] = new_peer;
            number_of_peers++;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

void remove_peer(struct peer *new_peer)
{
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_NUMBER_OF_PEERS; i++)
    {
        if (peers[i] == new_peer) {
            free(peers[i]);
            peers[i] = NULL;
            number_of_peers--;
            break;
        }
    }
    pthread_mutex_unlock(&lock);
}

int find_peer_with_addr(struct in_addr addr) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < MAX_NUMBER_OF_PEERS; i++)
    {
        if (peers[i] == NULL)
            continue;

        if (peers[i]->addr.s_addr == addr.s_addr) {
            pthread_mutex_unlock(&lock);
            return 1;
        }
    }
    pthread_mutex_unlock(&lock);
    return 0;
}

void handle_client(struct peer* server_peer) {
    int client_socket_fd;
    client_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket_fd == -1) {
        printf("Client socket creation failed.\n");
        exit(0);
    }
    else {
        printf("Client socket successfully created.\n");
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr = server_peer->addr;
    server_addr.sin_port = htons(MAIN_PORT);

    if (connect(client_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        char* ip_line = inet_ntoa(server_peer->addr);
        printf("Connection with the server %s failed. Peer removed.\n", ip_line);
        remove_peer(server_peer);
        return;
    }

    while (1) {
        send(client_socket_fd, GET_PEERS_REQUEST, strlen(GET_PEERS_REQUEST) * sizeof(char), 0);
        while (1) {
            char  response_buffer[BUFFER_SIZE];
            recv(client_socket_fd, response_buffer, sizeof(response_buffer), 0);
            if (strncmp(response_buffer, SENDING_FINISHED, sizeof(response_buffer)) == 0) {
                break;
            }
            struct peer* new_peer = malloc(sizeof(struct peer));
            if (inet_aton(response_buffer, &new_peer->addr) == 0) {
                printf("Wrong peer address received.\n");
                exit(0);
            }
            add_peer(new_peer);
        }
        sleep(PEERS_REFRESH_TIME);
    }
}

void handle_requests_from_client(int* client_sock_fd) {
    int sock_fd = *client_sock_fd;
    char request_buffer[BUFFER_SIZE];

    while (1) {
        recv(sock_fd, request_buffer, BUFFER_SIZE, 0);

        if (strncmp(request_buffer, GET_PEERS_REQUEST, sizeof(request_buffer)) == 0) {
            for (int i = 0; i < MAX_NUMBER_OF_PEERS; i++) {
                char *addr_line = inet_ntoa(peers[i]->addr);
                send(sock_fd, addr_line, strlen(addr_line) * sizeof(char), 0);
            }
            send(sock_fd, SENDING_FINISHED, strlen(SENDING_FINISHED) * sizeof(char), 0);
        }
        else {
            printf("Unknown request.\n");
        }
    }
}

void handle_server() {
    int server_sock_fd;

    server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock_fd == -1) {
        printf("Server socket creation failed.\n");
        exit(0);
    }
    else {
        printf("Server socket successfully created.\n");
    }

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(MAIN_PORT);

    if (bind(server_sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        printf("Server socket bind failed.\n");
        exit(0);
    }
    else {
        printf("Server socket successfully bound.\n");
    }

    if ((listen(server_sock_fd, MAX_NUMBER_OF_PEERS)) != 0) {
        printf("Server listening failed.\n");
        exit(0);
    }
    else {
        printf("Server listening...\n");
    }

    int client_sock_fd;
    struct sockaddr_in client_addr;
    while (1) {
        client_sock_fd = accept(server_sock_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_len);

        if (client_sock_fd < 0) {
            printf("Server request accepting failed.\n");
            exit(0);
        }
        else {
            printf("Server accepted connection.\n");
        }

        if (find_peer_with_addr(client_addr.sin_addr)) {
            struct peer* new_peer = malloc(sizeof(struct peer));
            new_peer->addr = client_addr.sin_addr;
            add_peer(new_peer);
            pthread_t thread_id;
            if (pthread_create(&thread_id, NULL, (void *)&handle_client, (void *)new_peer) != 0) {
                printf("Server request handler thread creation failed.\n");
            }
            else {
                printf("Server request handler thread created.\n");
            }
        }

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, (void *)&handle_requests_from_client, (void *)(&client_sock_fd)) != 0) {
            printf("Server request handler thread creation failed.\n");
        }
    }
}

int is_answer_char(char c) {
    if (c == 'y' || c == 'n' || c == 'Y' || c == 'N') {
        return 1;
    }
    return 0;
}

int main() {
    printf("Is first peer? (y, n)\n");

    char is_first_peer;
    struct peer* first_peer = malloc(sizeof(struct peer));

    while (1) {
        scanf("%c", &is_first_peer);

        if (is_first_peer == '\n') {
            continue;
        }

        if (is_answer_char(is_first_peer)) {
            break;
        }

        printf("Error: wrong format of answer.\n");
    }

    if (is_first_peer == 'n' || is_first_peer == 'N') {

        printf("IP address of the first peer:\n");
        char ip_address[BUFFER_SIZE];

        while (1) {
            scanf("%s", ip_address);
            if (inet_aton(ip_address, &first_peer->addr) != 0) {
                break;
            }
            printf("Error: wrong format of ip address.\n");
        }
    }

    pthread_t server_thread_id;
    pthread_create(&server_thread_id, NULL, (void *)&handle_server, NULL);

    if (is_first_peer == 'n' || is_first_peer == 'N') {
        pthread_t client_thread_id;
        pthread_create(&client_thread_id, NULL, (void *)&handle_client, first_peer);
    }

    pthread_join(server_thread_id, NULL);
    return 0;
}