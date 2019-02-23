typedef struct client_data{
    char name[256];
    unsigned int age;
    char group[256];
} client_data_t;


typedef struct result_struct_{

    char result[256];

} result_struct_t;

typedef struct request_struct {
    struct client_data data;
    struct sockaddr_in addr;
    int sent_recv_bytes;
} request_struct_t;
