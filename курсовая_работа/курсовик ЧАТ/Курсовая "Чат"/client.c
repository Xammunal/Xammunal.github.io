#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 12346
#define BUFFER_SIZE 1024

static int sock = -1;
static pthread_t recv_thread;
static void (*message_callback)(const char*) = NULL;

static char name[64];
static char key[64];

void client_set_credentials(const char* username, const char* password) {
    strncpy(name, username, sizeof(name) - 1);
    strncpy(key, password, sizeof(key) - 1);
    
    printf("[DEBUG] name = %s, key = %s\n", name, key);

}

void client_register_callback(void (*callback)(const char*)) {
    message_callback = callback;
}

static void* recv_loop(void* arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t len = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) break;

        buffer[len] = '\0';

        if (message_callback) {
            message_callback(buffer);
        }
    }
    return NULL;
}

bool client_start(void) {
    struct sockaddr_in server_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        sock = -1;
        return false;
    }

    // Отправка ИМЕНИ и КЛЮЧА по отдельности
    char buffer[BUFFER_SIZE];
    size_t name_len = strlen(name) + 1;
    size_t key_len = strlen(key) + 1;

    memcpy(buffer, name, name_len);
    memcpy(buffer + name_len, key, key_len);

    send(sock, buffer, name_len + key_len, 0);


    pthread_create(&recv_thread, NULL, recv_loop, NULL);

    return true;
}

void client_send_message(const char* msg) {
    if (sock >= 0) {
        send(sock, msg, strlen(msg), 0);
    }
}

void client_stop() {
    if (sock >= 0) {
        close(sock);
        sock = -1;
    }
}
