#include "common.h"

int send_msg(int socket_fd, char *msg, int line_size) {
    if (write(socket_fd, msg, line_size) < 0) {
        fprintf(stderr, "[%d] ERROR: Could not write to socket.\n", getpid());
        return -1;
    }
    return 0;
}

int receive_msg(int socket_fd, char *msg, int line_size) {
    ssize_t n = read(socket_fd, msg, line_size);

    if (n < 0) {
        fprintf(stderr, "[%d] ERROR: Could not read from socket.\n", getpid());
        return -1;
    }
    if (n == 0) {
        fprintf(stderr, "[%d] WARN: Connection closed by peer.\n", getpid());
        return -1;
    }

    msg[n] = '\0';  // Null-terminate the received message
    return 0;
}

int send_file(int sock, const char *filename) {
    int fd, n, total_sent = 0;
    char chunk[BUFFER_SIZE];
    struct stat st;

    // Get the file size and send it first
    if (stat(filename, &st) < 0) {
        fprintf(stderr, "[%d] ERROR: Could not stat file %s.\n", getpid(), filename);
        return -1;
    }

    printf("size of [%s] is: %ld\n", filename, st.st_size);
    snprintf(chunk, sizeof(chunk), "[DOWNLOAD] %ld %s", st.st_size, filename);
    write(sock, chunk, sizeof(chunk)); // Send file size

    // Open the file
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "[%d] ERROR: Could not open file %s.\n", getpid(), filename);
        return -1;
    }

    // Send the file content
    while ((n = read(fd, chunk, sizeof(chunk))) > 0) {
        write(sock, chunk, n); // Send the actual read bytes
        total_sent += n;
    }

    close(fd);
    return total_sent;
}

int receive_file(int sock, const char *filename) {
    int fd, n, total_received = 0;
    char buffer[BUFFER_SIZE];

    // Read the file size from the socket
    n = read(sock, buffer, sizeof(buffer));
    if (n < 0) {
        fprintf(stderr, "[%d] ERROR: Could not read file size from socket.\n", getpid());
        return -1;
    }
    buffer[n] = '\0'; // Null-terminate
    long file_size = atol(buffer);
    printf("[msg]: \"%s\" file size: %ld\n", buffer, file_size);

    // Create and open the file
    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "[%d] ERROR: Could not open file %s for writing.\n", getpid(), filename);
        return -1;
    }

    // Receive the file content
    while (total_received < file_size) {
        n = read(sock, buffer, sizeof(buffer));
        if (n <= 0) {
            fprintf(stderr, "[%d] ERROR: Read error from socket.\n", getpid());
            break; // Handle potential read errors
        }
        write(fd, buffer, n);
        total_received += n;
    }

    close(fd);
    return total_received;
}

/*
int send_file(char buffer[BUFFER_SIZE], int sock, const char *filename) {
	int fd, t, n;
    char chunk[BUFFER_SIZE];
    struct stat st;
    stat(filename, &st);
    printf("size of [%s] is: %ld\n", filename, st.st_size);
    sprintf(chunk, "%ld", st.st_size);
    n = write(sock, chunk, BUFFER_SIZE);
    fd = open(filename, O_RDONLY);
    while (t < st.st_size) {
        read(sock, chunk, BUFFER_SIZE);
        printf("sending: \"%s\"\n", chunk);
        n = write(sock, chunk, BUFFER_SIZE);
        t += n;
    }
    close(fd);
    return 0;
}

int receive_file(char buffer[BUFFER_SIZE], int sock, const char *filename) {
    int n, fd, t = 0, siz;
    write(sock, filename, BUFFER_SIZE);
    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    n = read(sock, buffer, BUFFER_SIZE);
    siz = atoi(buffer);
    printf("server: %s\nfile size: %d\n", buffer, siz);
    while (t < siz) {
        n = read(sock, buffer, BUFFER_SIZE);
        printf("from server (block): \"%s\"\n", buffer);
        write(fd, buffer, BUFFER_SIZE);
        t += n;
    }
    close(fd);
    return 0;
}


/* SEND
	/*
    FILE *file = fopen(filename, "rb");
    char buffer[BUFFER_SIZE];

    if (!file) {
        fprintf(stderr, "ERROR: File \"%s\" not found.\n", filename);
        snprintf(buffer, BUFFER_SIZE, "ERROR: File not found.");
        send_msg(sock, buffer, strlen(buffer));
        return -1;
    }

    printf("SUCCESS: File \"%s\" found.\n", filename);
    snprintf(buffer, BUFFER_SIZE, "SUCCESS");
    send_msg(sock, buffer, strlen(buffer));

    size_t n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
		printf("Sending \"%s\".\n", buffer);
        if (send(sock, buffer, n, 0) < 0) {
            fprintf(stderr, "ERROR: Failed to send file.\n");
            fclose(file);
            return -1;
        }
    }
    // Signal end of file transfer
    snprintf(buffer, BUFFER_SIZE, "EOF");
    send_msg(sock, buffer, strlen(buffer));

    fclose(file);
    printf("File \"%s\" sent successfully.\n", filename);

*/

/* REC
	/*
	char buffer[BUFFER_SIZE];
    ssize_t n;
    FILE *file = fopen(filename, "wb");

	if (!file) {
        fprintf(stderr, "[%d] ERROR: Could not open file \"%s\" for writing.\n", getpid(), filename);
        return -1;
    }

    n = read(sock, buffer, BUFFER_SIZE);
    printf("[error / success msg]: \"%s\"\n", buffer);
    if(!strncmp(buffer, "ERROR", 5)) {
		printf("[%d] ERROR found from remote. cancelling download.\n", getpid());
		return -1;
    }
    else if (!strncmp(buffer, "SUCCESS", 7)) {
		printf("[%d] SUCCESS found from remote.\n", getpid());
    }
    else {
		fprintf(stderr, "[%d] ERROR: Unknown phrase from server.\n", getpid());
		return -1;
    }

	printf("[%d] Accepting file \"%s\"\n", getpid(), filename);

    while ((n = read(sock, buffer, BUFFER_SIZE)) > 0) {
    	printf("[%d] recieved \"%s\".\n", getpid(), buffer);
        if (strncmp(buffer, "ERROR", 5) == 0) {
            fprintf(stderr, "[%d] ERROR from server: \"%s\"\n", getpid(), buffer);
			fclose(file);
            return -1;
        }

        if (strncmp(buffer, "EOF", 3) == 0) {
            break;
        }

        fwrite(buffer, sizeof(char), n, file);
    }

    fclose(file);
    printf("File \"%s\" received successfully.\n", filename);
*/
