#include "common.h"

int connect_to_server(const char *SERVER_IP, const int SERVER_PORT) {
    struct sockaddr_in server_addr;
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int pid = getpid();

    if (server_socket_fd < 0) {
        fprintf(stderr, "[%d] ERROR: Socket creation failed.\n", pid);
        exit(EXIT_FAILURE);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "[%d] ERROR: Invalid server IP address.\n", pid);
        exit(EXIT_FAILURE);
    }

    if (connect(server_socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        fprintf(stderr, "[%d] ERROR: Connection to server failed.\n", pid);
        exit(EXIT_FAILURE);
    }

    printf("Server connection successful.\n");
    return server_socket_fd;
}

void *thread_socket2stdout(void *sock_ptr) {
    int socket_fd = *(int *)sock_ptr;
    char buffer[BUFFER_SIZE];

    while (1) {
        if (receive_msg(socket_fd, buffer, BUFFER_SIZE) < 0) {
            return NULL;
        }
        printf("[msg]: %s\n", buffer);
    }
}

void *thread_stdin2socket(void *sock_ptr) {
    int socket_fd = *(int *)sock_ptr;
    char buffer[BUFFER_SIZE];

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

		if (strncmp(buffer, "[DOWNLOAD] ", 11) == 0) {
    		char *filename = buffer + 11;
    		printf("[%d] Download request.\n", getpid(), filename);
    		// send_msg(socket_fd, buffer, BUFFER_SIZE);
    		receive_file(socket_fd, filename);
		} else if (strncmp(buffer, "[UPLOAD] ", 9) == 0) {
    		char *filename = buffer + 9;
    		printf("[%d] Upload request for %s.\n", getpid(), filename);
    		// send_msg(socket_fd, buffer, BUFFER_SIZE);
    		send_file(socket_fd, filename);
        } else {
            if (send_msg(socket_fd, buffer, strlen(buffer)) < 0) {
                return NULL;
            }
        }
    }
    return NULL;
}

int main() {
    pthread_t recv_thread, send_thread, file_thread;
    int server_socket_fd = connect_to_server(DEF_S_IP, DEF_S_PORT);

    pthread_create(&recv_thread, NULL, thread_socket2stdout, &server_socket_fd);
    pthread_create(&send_thread, NULL, thread_stdin2socket, &server_socket_fd);

    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);
    pthread_join(file_thread, NULL);

    close(server_socket_fd);
    return 0;
}
