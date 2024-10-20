#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DEF_S_IP	"127.0.0.1" // default server IP
#define DEF_S_PORT  8080		// default server port

#define BUFFER_SIZE 1024		// buffer size

int send_msg(int socket_fd, char *msg, int line_size);
int receive_msg(int socket_fd, char *msg, int line_size);

int send_file(int sock, const char *filename);
int receive_file(int sock, const char *filename);
