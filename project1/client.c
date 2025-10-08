#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "utils.h"

#define PORT 2314 
#define BUFFER_SIZE 1024
#define OUTPUT_FILE "client_files/received_file.txt"

int main(int argc, char *argv[]) {
    // Setup client_files directory
    setup_client_files_dir();

    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE + 1];
    FILE *file;
    int expected_seq = 0;
    ssize_t bytes_received;

    // Check command line arguments or we will never get an IP
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(1);
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock_fd);
        exit(1);
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock_fd);
        exit(1);
    }

    printf("Connected to server %s:%d\n", argv[1], PORT);

    file = fopen(OUTPUT_FILE, "wb");
    if (!file) {
        perror("Failed to open output file");
        close(sock_fd);
        exit(1);
    }

    while (1) {
        bytes_received = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            perror("Receive failed");
            break;
        } else if (bytes_received == 0) {
            printf("Connection closed by server\n");
            break;
        }

        int seq_num = buffer[0];
        int data_size = bytes_received - 1;

        printf("Received frame with sequence number: %d (%d bytes)\n", seq_num, data_size);

        if (seq_num == expected_seq) {
            // Correct sequence number, write data and send ACK
            if (data_size > 0) {
                fwrite(buffer + 1, 1, data_size, file);
            }
            printf("Sending ACK: %d\n", seq_num);

            if (send(sock_fd, &seq_num, 1, 0) < 0) {
                perror("Send ACK failed");
                break;
            }

            expected_seq = 1 - expected_seq;
        } else {
            printf("Received duplicate frame (seq %d, expected %d). Re-sending ACK: %d\n",
                   seq_num, expected_seq, 1 - expected_seq);

            char last_ack = 1 - expected_seq;
            if (send(sock_fd, &last_ack, 1, 0) < 0) {
                perror("Send ACK failed");
                break;
            }
        }
    }
    fclose(file);
    close(sock_fd);

    printf("File reception complete.\n");
    return 0;
}
