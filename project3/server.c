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
#define CERT_FILE "certs/server.crt"
#define KEY_FILE "certs/server.key"
#define CA_CERT_FILE "certs/client.crt"

// Global SSL context
SSL_CTX *ssl_ctx = NULL;

// Signal handler for non awful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down server...\n", sig);
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

// Create SSL context with TLS 1.2+ and mutual authentication
SSL_CTX* create_ssl_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = TLS_server_method();  // Only TLS 1.2 and higher so things are ok
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    // Load server certificate
    if (SSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // Load server private key
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

    // Enable mutual TLS authentication
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);

    // Load client certificate for verification
    if (SSL_CTX_load_verify_locations(ctx, CA_CERT_FILE, NULL) <= 0) {
        fprintf(stderr, "Failed to load CA certificate for client verification\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION) != 1) {
        fprintf(stderr, "Failed to set minimum TLS version to 1.2\n");
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

int handle_http_request(SSL *ssl) {
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int bytes_read;

    // Read HTTP request
    bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        int ssl_err = SSL_get_error(ssl, bytes_read);
        fprintf(stderr, "SSL read error: %d\n", ssl_err);
        return -1;
    }

    buffer[bytes_read] = '\0';
    printf("Received HTTP request:\n%s\n", buffer);

    if (strstr(buffer, "GET / HTTP/1.1") || strstr(buffer, "GET / HTTP/1.0")) {
        snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 47\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Hello from secure TLS server! Connection successful.\n");

        if (SSL_write(ssl, response, strlen(response)) <= 0) {
            int ssl_err = SSL_get_error(ssl, -1);
            fprintf(stderr, "SSL write error: %d\n", ssl_err);
            return -1;
        }

        printf("Sent HTTP response successfully\n");
        return 0;
    } else {
        // Bad request response
        snprintf(response, sizeof(response),
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 15\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Bad Request\n");

        SSL_write(ssl, response, strlen(response));
        return -1;
    }
}

// Handle client connection
void handle_client(int client_fd, struct sockaddr_in *client_addr) {
    SSL *ssl;
    int ret;

    printf("Client connected from %s:%d\n",
           inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));

    // Create SSL connection
    ssl = SSL_new(ssl_ctx);
    if (!ssl) {
        perror("SSL_new failed");
        close(client_fd);
        return;
    }

    // Attach SSL to socket
    SSL_set_fd(ssl, client_fd);

    // Perform SSL handshake
    ret = SSL_accept(ssl);
    if (ret <= 0) {
        int ssl_err = SSL_get_error(ssl, ret);
        fprintf(stderr, "SSL_accept failed with error: %d\n", ssl_err);
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(client_fd);
        return;
    }

    printf("SSL handshake successful! Mutual TLS authentication completed.\n");

    // Display certificate information
    X509 *client_cert = SSL_get_peer_certificate(ssl);
    if (client_cert) {
        char *client_cert_info = X509_NAME_oneline(X509_get_subject_name(client_cert), NULL, 0);
        printf("Client certificate subject: %s\n", client_cert_info);
        OPENSSL_free(client_cert_info);
        X509_free(client_cert);
    }

    // Handle HTTP request
    if (handle_http_request(ssl) == 0) {
        printf("HTTP request handled successfully\n");
    } else {
        printf("HTTP request handling failed\n");
    }

    // Clean up SSL connection
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(client_fd);

    printf("Client connection closed\n");
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

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

    printf("SSL context created successfully with TLS 1.2+ and mutual authentication\n");

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
        exit(1);
    }

    // Set up server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
        exit(1);
    }

    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        SSL_CTX_free(ssl_ctx);
        cleanup_openssl();
        exit(1);
    }

    printf("HTTPS server listening on port %d...\n", PORT);
    printf("Server supports TLS 1.2+ with mutual authentication\n");

    // Accept client connections
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                // Interrupted by signal, exit not aweful
                break;
            }
            perror("Accept failed");
            continue;
        }

        handle_client(client_fd, &client_addr);
    }

    // Clean up
    close(server_fd);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();

    printf("Server shutdown complete\n");
    return 0;
}
