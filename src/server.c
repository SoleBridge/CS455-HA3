#include "common.h"

#define MAX_BACKLOG 5
#define MAX_CLIENTS 10

// store client sockets to broadcast messages
int client_sockets[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// starts server. create socket, binds, listens.. get socket ready to accept connections
int server_start() {
    struct sockaddr_in server_addr;
    int pid, server_socket_fd;

	pid = getpid();

    bzero(&server_addr, sizeof(server_addr)); // clear struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(DEF_S_PORT);
    server_addr.sin_addr.s_addr = inet_addr(DEF_S_IP); // = INADDR_ANY;

    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "[%d] ERROR: Socket creation failed.\n", pid);
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &pid, sizeof(int)) < 0) {
        fprintf(stderr, "[%d] ERROR: Socket options set failed.\n", pid = getpid());
        exit(EXIT_FAILURE);
    }

    if ((bind(server_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        fprintf(stderr, "[%d] ERROR: Socket bind failed.\n", pid);
        exit(EXIT_FAILURE);
    }

    if ((listen(server_socket_fd, MAX_BACKLOG)) < 0) {
        fprintf(stderr, "[%d] ERROR: Socket listen failed.\n", pid);
        exit(EXIT_FAILURE);
    }

    printf("[%d] Listening on port %d.\n", pid, DEF_S_PORT);

    return server_socket_fd;
}

void *thread_handle_client(void *client_sock_ptr) {
    int client_socket_fd = *(int *)client_sock_ptr;
    char buffer[BUFFER_SIZE];

    while (1) {
        if (receive_msg(client_socket_fd, buffer, BUFFER_SIZE) < 0) {
            printf("[Server] Client disconnected.\n");
            break;
        }
        printf("[msg]: %s\n", buffer);

        // Handle file upload/download requests here
        if (strncmp(buffer, "[DOWNLOAD] ", 11) == 0) {
    		char *filename = buffer + 11;
    		printf("[%d] Download request for %s.\n", getpid(), filename);
    		send_file(client_socket_fd, filename);
		} else if (strncmp(buffer, "[UPLOAD] ", 9) == 0) {
    		char *filename = buffer + 9;
    		printf("[%d] Upload request for %s.\n", getpid(), filename);
    		receive_file(client_socket_fd, filename);
		}
    }

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == client_socket_fd) {
            client_sockets[i] = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    free(client_sock_ptr);
    return NULL;
}

void *thread_accept_clients(void *server_socket_ptr) {
    int server_socket_fd = *(int *)server_socket_ptr;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        int client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket_fd < 0) {
            fprintf(stderr, "ERROR: Accept client failed.\n");
            continue;
        }

        printf("[%d] Accepted a client: %s:%d.\n", getpid(),
        	inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Add client to the list
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = client_socket_fd;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Create a new thread to handle the client
        pthread_t client_thread;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_socket_fd;
        pthread_create(&client_thread, NULL, thread_handle_client, (void *)new_sock);
        pthread_detach(client_thread); // automatic clean up
    }
    return NULL;
}

void *thread_stdin2all(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

		pthread_mutex_lock(&clients_mutex);
    	for (int i = 0; i < MAX_CLIENTS; i++) {
	        if (client_sockets[i] != 0) {
            	send_msg(client_sockets[i], buffer, strlen(buffer));
        	}
    	}
    	pthread_mutex_unlock(&clients_mutex);
    }
    return NULL;
}

int main(int argc, char** argv, char** env) {
	struct sockaddr_in client_addr;
    int pid, server_socket_fd;
	pthread_t accept_thread, stdin_thread;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    pid = getpid();
	server_socket_fd = server_start();

    pthread_create(&accept_thread, NULL, thread_accept_clients, &server_socket_fd);
    pthread_create(&stdin_thread, NULL, thread_stdin2all, NULL);

	pthread_join(accept_thread, NULL);
    pthread_join(stdin_thread, NULL);

	close(server_socket_fd);

	printf("[%d] Done.\n", pid);
	exit(EXIT_SUCCESS);
}
