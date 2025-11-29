CS 4220 Computer Networks - Project 3
Kaden Bilyeu, Samuel Aldinger, Enzo Knapp

I have neither given nor received unauthorized assistance on this work.

Note, we were unable to get the project working on blanca, but we got it working fine on redcloud, as well as locally on Mac and Linux

Project Description:
This project implements a secure HTTP client-server application using OpenSSL with mutual
Transport Layer Security (mTLS) authentication. The server supports only TLS 1.2 and higher,
enforces mutual authentication, and serves simple HTTP responses. The client establishes
secure connections with the server using client certificates for mutual authentication.

implemented:
• Mutual TLS (mTLS) authentication - both client and server certificates required
• TLS 1.2+ enforcement - all older TLS versions disabled
• HTTP/1.1 protocol support over secure TLS connection
• Certificate-based authentication with proper validation
• Error handling throughout OpenSSL operations
• Clean connection termination and resource cleanup

Technical Implementation:
- Server: OpenSSL TLS server with certificate loading, mutual authentication enforcement,
  and HTTP request processing
- Client: OpenSSL TLS client with certificate presentation and secure HTTP requests
- Certificates: Self-signed certificates generated for both server and client
- Security: Mutual authentication ensures both parties are verified
- Protocol: HTTP over TLS with proper handshake and session management

How to Build:

LINUX:
1. Install OpenSSL development libraries:
   Ubuntu/Debian:
   sudo apt update
   sudo apt install gcc make libssl-dev openssl

   Fedora/RHEL:
   sudo dnf install gcc make openssl-devel

2. Compile the programs:
   make all

3. Alternative compilation:
   make server    # Build only server
   make client    # Build only client

macOS:
1. Install OpenSSL via Homebrew:
   brew install openssl@3

2. Compile the programs:
   make all

Note: The Makefile automatically detects the OS and uses appropriate paths.
For macOS with non-Homebrew OpenSSL, update the OPENSSL_PREFIX variable
in the Makefile to match your installation path.

How to Run:

Server:
./server

The server will start listening on port 2314 and display status information.

Client:
./client <server_ip>

Replace <server_ip> with the IP address of the machine running the server (e.g., 127.0.0.1 for localhost).

Example session:
Terminal 1:
$ ./server
SSL context created successfully with TLS 1.2+ and mutual authentication
HTTPS server listening on port 2314...

Terminal 2:
$ ./client 127.0.0.1
SSL context created successfully with TLS 1.2+ and client certificate
Connected to server 127.0.0.1:2314
SSL handshake successful! Mutual TLS authentication completed.
Server certificate subject: /C=US/ST=Colorado/L=Colorado Springs/O=UCCS/CN=localhost
Sent HTTP request:
GET / HTTP/1.1
Host: localhost:2314
...

Received HTTP response:
HTTP/1.1 200 OK
Content-Type: text/plain
...

HTTP request successful! Mutual TLS connection verified.


Certificate Generation:

Server certificate:
openssl req -x509 -newkey rsa:2048 -nodes -keyout certs/server.key -out certs/server.crt -days 365 -subj "/C=US/ST=Colorado/L=Colorado Springs/O=UCCS/CN=localhost"

Client certificate:
openssl req -x509 -newkey rsa:2048 -nodes -keyout certs/client.key -out certs/client.crt -days 365 -subj "/C=US/ST=Colorado/L=Colorado Springs/O=UCCS/CN=client"

These certificates are already included in the certs/ directory, but if they are not please generate them.

Summary of things Handled:

1. SSL_CTX Setup:
   - Configured for TLS 1.2+ only
   - Enabled mutual authentication
   - Loaded and checked certificates and keys

2. Mutual TLS:
   - Server checks for client certificate
   - Client sends certificate during handshake
   - Handled certificate verification and errors

3. TLS with HTTP:
   - Used SSL_read/SSL_write, not regular sockets
   - Checked handshake failures and connection errors
   - Managed SSL session cleanup

4. Error Handling:
   - Checked all OpenSSL calls for errors
   - Cleaned up resources on failure
   - Printed useful error messages

6. Platform Support:
   - Works on Linux and macOS
   - Made sure OpenSSL linking is correct


Resources Used:

• OpenSSL Documentation: https://www.openssl.org/docs/
• Beej's Guide to Network Programming (SSL/TLS sections)
• Computer Networks: A Top-Down Approach (Kurose & Ross) - Security chapters
• OpenSSL man pages: man SSL_CTX_new, man SSL_accept, etc.
• Stack Overflow and OpenSSL mailing lists for specific implementation questions
• Linux manual pages for socket and SSL functions

Project Structure:

Root Directory:
- server.c: Server implementation
- client.c: Client implementation
- Makefile: Build configuration (auto-detects Linux/macOS)
- README.txt: This file
- .gitignore: Git ignore file for compiled binaries and logs

certs/:
- server.crt/server.key: Server certificate and private key
- client.crt/client.key: Client certificate and private key
logs/:
- Contains test and runtime log files (ignored by git)

Code Implementation:

server.c:
- OpenSSL initialization and context creation
- Certificate loading and mutual TLS setup
- TCP socket creation and binding
- SSL connection acceptance and handshake
- HTTP request parsing and response generation
- Proper cleanup and error handling

client.c:
- OpenSSL client context creation
- Client certificate loading for mutual TLS
- TCP connection establishment
- SSL handshake with server certificate verification
- HTTP request sending and response processing
- Connection cleanup

Makefile:
- GCC compilation with OpenSSL flags
- Separate targets for server and client
- Clean target for removing binaries

notes:

• Mutual TLS ensures both client and server authenticate each other
• TLS 1.2+ only prevents downgrade attacks
• Certificate validation prevents man-in-the-middle attacks
• Private keys are protected (not world-readable)
• SSL session cleanup prevents resource leaks
• Error handling prevents information disclosure