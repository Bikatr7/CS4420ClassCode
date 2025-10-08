CS 4220 Computer Networks - Project 1
Kaden Bilyeu

I have neither given nor received unauthorized assistance on this work.

Note, I programmed this locally on my Mac

Project Description:
This project implements a TCP client-server file transfer program using sockets in C,
with Stop-and-Wait ARQ for reliable data transmission. The server reads a file in chunks,
adds sequence numbers, and sends frames while waiting for acknowledgments. The client
receives frames, validates sequence numbers, sends ACKs, and handles retransmissions.

How to Build:
1. Ensure you have gcc installed on your system
2. Run 'make all' to build both server and client
3. Alternatively, run 'make server' or 'make client' to build individually

How to Run:
1. On the server machine (where input_file.txt is located):
   ./server

2. On the client machine:
   ./client <server_ip_address>

   Replace <server_ip_address> with the actual IP address of the server machine.

   I personally just used localhost (127.0.0.1)

3. The client will receive the file and save it as client_files/received_file.txt

Important Notes:
- The server reads from input_file.txt in the same directory
- The client saves received files to client_files/received_file.txt
- The program uses Stop-and-Wait ARQ with 2-second timeouts
- All output shows sequence numbers, ACKs, and retransmission events for debugging

Challenges Overcome:
- Implementing proper timeout handling with select() for retransmission logic
- Managing sequence numbers and ACK validation in Stop-and-Wait ARQ
- Ensuring binary file transfer compatibility
- Handling connection establishment and cleanup

Code Structure:
- server.c: TCP server with file reading and Stop-and-Wait ARQ transmission
- client.c: TCP client with ACK handling and duplicate detection
- utils.c/utils.h: Shared utility functions for directory management
- Makefile: Build system with all, server, client, and clean targets
- input_file.txt: Test file for transmission
- client_files/: Directory for received files (auto-managed by both programs)

Shared Utility Functions:
- setup_client_files_dir(): Called by both client and server on startup
  - Removes existing client_files directory if it exists (with all contents)
  - Recreates empty client_files directory
  - Ensures clean state for file reception
