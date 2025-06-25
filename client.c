#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <termios.h>

#define PORT 12346
#define BUFFER_SIZE 1024

char name[50];
int sock;
char current_input[BUFFER_SIZE];

// Поток для отправки сообщений
void* send_handler(void* arg) {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // Отключаем канонический режим и эхо
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    int pos = 0;
    char ch;
    printf("Ты: ");
    fflush(stdout);
    while (1) {
        ch = getchar();

        if (ch == '\n') {
            current_input[pos] = '\0';
            if (strcmp(current_input, "exit") == 0) {
                send(sock, current_input, strlen(current_input) + 1, 0);
                printf("\nВыход...\n");
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Возвращаем настройки терминала
                close(sock);
                exit(0);
            }

            send(sock, current_input, strlen(current_input) + 1, 0);
            pos = 0;
            current_input[0] = '\0';
            printf("\nТы: ");
            fflush(stdout);
        } else if (ch == 127 || ch == 8) { // Backspace
            if (pos > 0) {
                pos--;
                current_input[pos] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
        } else if (pos < BUFFER_SIZE - 1) {
            current_input[pos++] = ch;
            current_input[pos] = '\0';
            putchar(ch);
            fflush(stdout);
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Возвращаем настройки терминала
    return NULL;
}

// Поток для получения сообщений
void* recv_handler(void* arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (received <= 0) {
            printf("\nСервер отключился.\n");
            close(sock);
            exit(0);
        }

        // Очистка текущей строки и вывод сообщения
        printf("\r\033[K%s\n", buffer);
        printf("Ты: %s", current_input);
        fflush(stdout);
    }
    return NULL;
}

int main() {
    struct sockaddr_in server_addr;

    printf("Введите ваше имя: ");
    fgets(name, 50, stdin);
    name[strcspn(name, "\n")] = '\0';

    // Создание сокета
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // Подключение
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Отправляем имя серверу
    send(sock, name, strlen(name) + 1, 0);

    // Запросите и отправьте ключ доступа
    char key[50];
    printf("Введите ключ доступа (admin или user): ");
    fflush(stdout);
    fgets(key, sizeof(key), stdin);
    key[strcspn(key, "\n")] = '\0';
    send(sock, key, strlen(key) + 1, 0);

    printf("Подключён к серверу как %s. Пиши сообщения. Введи 'exit' для выхода.\n", name);

    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, send_handler, NULL);
    pthread_create(&recv_thread, NULL, recv_handler, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    return 0;
}
//cd "/Users/ivansalukhov/Desktop/projects vs/c/курсовик ЧАТ/" && gcc client.c -o client && "/Users/ivansalukhov/Desktop/projects vs/c/курсовик ЧАТ/"client