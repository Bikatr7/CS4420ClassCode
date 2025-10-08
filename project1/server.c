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
#include <signal.h>
#include <sys/time.h>
#include "utils.h"

#define PORT 2314
#define BUFFER_SIZE 1024
#define TIMEOUT_SEC 2
#define INPUT_FILE "input_file.txt"

int main() {
    // Setup client_files directory
    setup_client_files_dir();

    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE + 1];
    FILE *file;
    size_t bytes_read;
    int seq_num = 0;
    struct timeval timeout;
    fd_set readfds;
    int retval;

    // TCP socket creation
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Server address setup
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(1);
    }

    // Listen for connections on socket
    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    // Accept connection on socket
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(1);
    }

    printf("Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    file = fopen(INPUT_FILE, "rb");
    if (!file) {
        perror("Failed to open input file");
        close(client_fd);
        close(server_fd);
        exit(1);
    }
    while ((bytes_read = fread(buffer + 1, 1, BUFFER_SIZE, file)) > 0) {
        // Add sequence number to frame or things go not good
        buffer[0] = seq_num;
        int frame_size = bytes_read + 1;

        int retransmit = 1;
        while (retransmit) {
            // Send frame to client
            if (send(client_fd, buffer, frame_size, 0) < 0) {
                perror("Send failed");
                break;
            }
            printf("Sent frame with sequence number: %d (%d bytes)\n", seq_num, (int)bytes_read);

            // Set up timeout for ACK or we will never get an ACK
            FD_ZERO(&readfds);
            FD_SET(client_fd, &readfds);
            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = 0;

            retval = select(client_fd + 1, &readfds, NULL, NULL, &timeout);

            if (retval == -1) {
                perror("Select error");
                break;
            } else if (retval == 0) {
                // Timeout occurred, we will retransmit the frame
                printf("Timeout occurred, retransmitting frame with seq %d\n", seq_num);
                continue;
            } else {
                // Data available, read ACK
                char ack;
                if (recv(client_fd, &ack, 1, 0) <= 0) {
                    perror("Receive ACK failed");
                    break;
                }

                if (ack == seq_num) {
                    printf("Received correct ACK: %d\n", ack);
                    retransmit = 0;  // Move to next frame
                    seq_num = 1 - seq_num;  // Toggle sequence number (0 <-> 1)
                } else {
                    printf("Received incorrect ACK: %d, expected: %d. Retransmitting.\n", ack, seq_num);
                }
            }
        }

        if (retransmit) break;  // If we couldn't send successfully, stop so i can cry
    }

    // Close file and sockets
    fclose(file);
    close(client_fd);
    close(server_fd);

    printf("File transfer complete.\n");
    return 0;
}