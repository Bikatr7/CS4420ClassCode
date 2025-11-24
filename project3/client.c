#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <signal.h>
#include <errno.h>

#define PORT 2314
#define BUFFER_SIZE 4096
#define CERT_FILE "certs/client.crt"
#define KEY_FILE "certs/client.key"
#define CA_CERT_FILE "certs/server.crt"

// Global SSL context
SSL_CTX *ssl_ctx = NULL;

// Signal handler for clean shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down client...\n", sig);
    if (ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
    }
    exit(0);
}

// Initialize OpenSSL
int init_openssl() {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    return 1;
}

// Cleanup OpenSSL
void cleanup_openssl() {
    EVP_cleanup();
}

// Create SSL context with TLS 1.2+ and client certificate
SSL_CTX* create_ssl_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_client_method();  // Only TLS 1.2 and higher
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // Load client certificate for mutual TLS
    if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // Load client private key
    if (SSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // Verify private key matches certificate
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "Private key does not match the certificate public key\n");
        SSL_CTX_free(ctx);
        return NULL;
    }

    // Load server certificate for verification
    if (SSL_CTX_load_verify_locations(ctx, CA_CERT_FILE, NULL) <= 0) {
        fprintf(stderr, "Failed to load server certificate for verification\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // Enable server certificate verification
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    // Set minimum TLS version to 1.2
    if (SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) != 1) {
        fprintf(stderr, "Failed to set minimum TLS version to 1.2\n");
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

// Send HTTP request and receive response
int send_http_request(SSL *ssl) {
    char request[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Create HTTP GET request
    snprintf(request, sizeof(request),
        "GET / HTTP/1.1\r\n"
        "Host: localhost:%d\r\n"
        "User-Agent: SecureHTTPClient/1.0\r\n"
        "Accept: text/plain\r\n"
        "Connection: close\r\n"
        "\r\n", PORT);

    // Send HTTP request
    if (SSL_write(ssl, request, strlen(request)) <= 0) {
        int ssl_err = SSL_get_error(ssl, -1);
        fprintf(stderr, "SSL write error: %d\n", ssl_err);
        ERR_print_errors_fp(stderr);
        return -1;
    }

    printf("Sent HTTP request:\n%s", request);

    // Read HTTP response
    bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        int ssl_err = SSL_get_error(ssl, bytes_read);
        fprintf(stderr, "SSL read error: %d\n", ssl_err);
        ERR_print_errors_fp(stderr);
        return -1;
    }

    buffer[bytes_read] = '\0';
    printf("Received HTTP response:\n%s\n", buffer);

    // Check if response indicates success?
    if (strstr(buffer, "HTTP/1.1 200 OK")) {
        printf("HTTP request successful! Mutual TLS connection verified.\n");
        return 0;
    } else {
        printf("HTTP request failed!\n");
        return -1;
    }
}

int main(int argc, char *argv[]) {
    int sock_fd;
    struct sockaddr_in server_addr;
    SSL *ssl;
    int ret;

    // Check command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
        exit(1);
    }

    // Set up signal handler for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize OpenSSL
    if (!init_openssl()) {
        fprintf(stderr, "Failed to initialize OpenSSL\n");
        exit(1);
    }

    // Create SSL context
    ssl_ctx = create_ssl_context();
    if (!ssl_ctx) {
        fprintf(stderr, "Failed to create SSL context\n");
        cleanup_openssl();
        exit(1);
    }

    printf("SSL context created successfully with TLS 1.2+ and client certificate\n");

    // Create TCP socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
        exit(1);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("Invalid server address");
        close(sock_fd);
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
        exit(1);
    }

    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock_fd);
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
        exit(1);
    }

    printf("Connected to server %s:%d\n", argv[1], PORT);

    // Create SSL connection
    ssl = SSL_new(ssl_ctx);
    if (!ssl) {
        perror("SSL_new failed");
        close(sock_fd);
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
        exit(1);
    }

    // Attach SSL to socket
    SSL_set_fd(ssl, sock_fd);

    // Set hostname for SNI (Server Name Indication)
    SSL_set_tlsext_host_name(ssl, "localhost");

    // Perform SSL handshake (includes mutual authentication)
    printf("Performing SSL handshake with mutual TLS authentication...\n");
    ret = SSL_connect(ssl);
    if (ret <= 0) {
        int ssl_err = SSL_get_error(ssl, ret);
        fprintf(stderr, "SSL_connect failed with error: %d\n", ssl_err);
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sock_fd);
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
        exit(1);
    }

    printf("SSL handshake successful! Mutual TLS authentication completed.\n");

    // Display certificate information
    X509 *server_cert = SSL_get_peer_certificate(ssl);
    if (server_cert) {
        char *server_cert_info = X509_NAME_oneline(X509_get_subject_name(server_cert), NULL, 0);
        printf("Server certificate subject: %s\n", server_cert_info);
        OPENSSL_free(server_cert_info);
        X509_free(server_cert);
    }

    // Send HTTP request and receive response
    ret = send_http_request(ssl);
    if (ret == 0) {
        printf("Secure HTTP transaction completed successfully!\n");
    } else {
        printf("Secure HTTP transaction failed!\n");
    }

    // Clean up SSL connection
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock_fd);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();

    printf("Client shutdown complete\n");
    return (ret == 0) ? 0 : 1;
}
