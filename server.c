#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#define PORT 12346
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10
#define ADMIN_KEY "4415"
#define USER_KEY "1234"

typedef struct Node {
    char symbol;
    struct Node* next;
} Node;

Node* obfuscation_list = NULL;

// Создание списка
void build_obfuscation_list() {
    const char* symbols = "()[]''!.>@#$&*";
    Node* head = NULL;
    Node* current = NULL;

    for (int i = 0; symbols[i] != '\0'; ++i) {
        Node* new_node = malloc(sizeof(Node));
        new_node->symbol = symbols[i];
        new_node->next = NULL;

        if (!head) {
            head = new_node;
        } else {
            current->next = new_node;
        }
        current = new_node;
    }

    obfuscation_list = head;
}

// Получение случайного символа из списка
char get_random_obfuscation_char() {
    if (!obfuscation_list) return '?';

    int len = 0;
    Node* tmp = obfuscation_list;
    while (tmp) {
        len++;
        tmp = tmp->next;
    }

    int target = rand() % len;
    tmp = obfuscation_list;
    while (target-- > 0 && tmp) {
        tmp = tmp->next;
    }

    return tmp ? tmp->symbol : '?';
}

// Шифрование сообщения
void obfuscate_message(char* dst, const char* src) {
    if (!src || !dst) return;  // защита от NULL

    int len = strlen(src);
    
    for (int i = 0; i < len; ++i) {
        dst[i] = get_random_obfuscation_char();
    }
    dst[len] = '\0';  // гарантированное завершение строки
}

void free_obfuscation_list() {
    Node* current = obfuscation_list;
    while (current) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    obfuscation_list = NULL;
}

typedef struct {
    int socket;
    char name[50];
    int role; // 0 = гость, 1 = пользователь, 2 = админ
    time_t mute_until;
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_client(client_t client) {
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS) {
        clients[client_count++] = client;
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket == client_socket) {
            for (int j = i; j < client_count - 1; ++j) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(char* message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);

    int sender_role = 1; // по умолчанию USER
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket == sender_socket) {
            sender_role = clients[i].role;
            break;
        }
    }

    // Если отправитель гость — заменяем сообщение на шум
    char final_msg[BUFFER_SIZE + 50];
    if (sender_role == 0) {
        obfuscate_message(final_msg, message); // замещаем весь текст
    } else {
        strncpy(final_msg, message, sizeof(final_msg));
    }

    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket != sender_socket) {
            char to_send[BUFFER_SIZE + 50];

            // Если получатель гость — шифруем даже сообщение не-гостя
            if (clients[i].role == 0) {
                obfuscate_message(to_send, final_msg);
                send(clients[i].socket, to_send, strlen(to_send) + 1, 0);
            } else {
                send(clients[i].socket, final_msg, strlen(final_msg) + 1, 0);
            }
        }
    }

    // Логируем только сообщения не от гостей
    if (sender_role != 0) {
        FILE *log_file = fopen("chat.log", "a");
        if (log_file != NULL) {
            fprintf(log_file, "%s\n", message);
            fclose(log_file);
        } else {
            perror("Ошибка открытия chat.log");
        }
    }

    // // Весте с гостями
    // FILE *log_file = fopen("chat.log", "a");
    // if (log_file != NULL) {
    //     if (sender_role != 0) {
    //         fprintf(log_file, "%s\n", message);
    //     } else {
    //         fprintf(log_file, "Зашифрованное сообщение от гостя: %s (оригинал: %s)\n", final_msg, message);
    //     }
    //     fclose(log_file);
    // } else {
    //     perror("Ошибка открытия chat.log");
    // }
    pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    client_t current_client;
    current_client.socket = client_socket;

    // Получаем имя клиента
    recv(client_socket, current_client.name, sizeof(current_client.name), 0);
    // Получаем ключ доступа
    char keybuf[50];
    recv(client_socket, keybuf, sizeof(keybuf), 0);
    
    printf("Получено имя: [%s]\n", current_client.name);
    printf("Получен ключ: [%s]\n", keybuf);

    // Определяем роль
    if (strcmp(keybuf, ADMIN_KEY) == 0) {
        current_client.role = 2;
    } else if (strcmp(keybuf, USER_KEY) == 0) {
        current_client.role = 1;
    } else {
        current_client.role = 0;
    }
    current_client.mute_until = 0;
    add_client(current_client);
    // Логируем подключение с указанием роли
    const char *role_names[] = {"GUEST", "USER", "ADMIN"};
    printf("Клиент присоединился как %s (роль: %s)\n", current_client.name, role_names[current_client.role]);
    // уведомление о присоединении ко всем клиентам
    {
        char joinmsg[BUFFER_SIZE];
        snprintf(joinmsg, sizeof(joinmsg), "%s joined as %s.", current_client.name, role_names[current_client.role]);
        broadcast_message(joinmsg, -1);
    }

    if (current_client.role == 2) {
        const char *cmds = "Admin commands:\n"
                        "/list - list users\n"
                        "/mute <name> - mute user for 10s\n"
                        "/kick <name> - disconnect user\n";
        send(client_socket, (char*)cmds, strlen(cmds) + 1, 0);
    }

    while (1) {
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("%s отключился.\n", current_client.name);
            break;
        }
        // Проверка команд, если клиент — админ
        if (buffer[0] == '/' && current_client.role == 2) {
            // LIST
            if (strncmp(buffer, "/list", 5) == 0) {
                char listbuf[BUFFER_SIZE];
                int offset = snprintf(listbuf, BUFFER_SIZE, "Active users:\n");
                pthread_mutex_lock(&clients_mutex);
                for (int i = 0; i < client_count; ++i) {
                    offset += snprintf(listbuf + offset, BUFFER_SIZE - offset,
                                    "%s (%s)\n",
                                    clients[i].name,
                                    clients[i].role == 2 ? "ADMIN" :
                                    clients[i].role == 1 ? "USER" : "GUEST");
                }
                pthread_mutex_unlock(&clients_mutex);
                send(client_socket, listbuf, strlen(listbuf) + 1, 0);
                continue;
            }
            // KICK
            if (strncmp(buffer, "/kick ", 6) == 0) {
                char target[BUFFER_SIZE] = {0};
                int ti = 0;
                for (int p = 6; buffer[p] && ti < BUFFER_SIZE - 1; ++p) {
                    if (buffer[p] != '<' && buffer[p] != '>' && buffer[p] != ' ') {
                        target[ti++] = buffer[p];
                    }
                }
                target[ti] = '\0';

                int removed_index = -1;
                int victim_sock = -1;
                // Поиск цели под мьютексом
                pthread_mutex_lock(&clients_mutex);
                for (int i = 0; i < client_count; ++i) {
                    if (strcmp(clients[i].name, target) == 0) {
                        if (clients[i].role == 2) {
                            // Нельзя кикать админа
                            send(client_socket, "Cannot kick an admin.", strlen("Cannot kick an admin.")+1, 0);
                            removed_index = -2; // маркер запрета
                        } else {
                            victim_sock = clients[i].socket;
                            removed_index = i;
                        }
                        break;
                    }
                }
                if (removed_index >= 0) {
                    // удаляем клиента из массива
                    for (int j = removed_index; j < client_count - 1; ++j) {
                        clients[j] = clients[j+1];
                    }
                    client_count--;
                }
                pthread_mutex_unlock(&clients_mutex);

                if (removed_index >= 0) {
                    // отключаем клиента вне мьютекса
                    send(victim_sock, "You have been kicked by admin.", strlen("You have been kicked by admin.")+1, 0);
                    shutdown(victim_sock, SHUT_RDWR);
                    close(victim_sock);
                    // уведомляем всех
                    char note[BUFFER_SIZE];
                    snprintf(note, sizeof(note), "%s был исключен.", target);
                    broadcast_message(note, -1);
                    // подтверждаем администратору
                    send(client_socket, "User kicked.", strlen("User kicked.")+1, 0);
                }
                // если removed_index == -2, сообщение уже отправлено
                if (removed_index == -1) {
                    send(client_socket, "User not found.", strlen("User not found.")+1, 0);
                }
                continue;
            }
            // MUTE
            if (strncmp(buffer, "/mute ", 6) == 0) {
                char target[BUFFER_SIZE] = {0};
                int ti = 0;
                for (int p = 6; buffer[p] && ti < BUFFER_SIZE - 1; ++p) {
                    if (buffer[p] != '<' && buffer[p] != '>' && buffer[p] != ' ') {
                        target[ti++] = buffer[p];
                    }
                }
                target[ti] = '\0';

                int found_index = -1;
                // Назначение заглушения под мьютексом
                time_t now = time(NULL);
                pthread_mutex_lock(&clients_mutex);
                for (int i = 0; i < client_count; ++i) {
                    if (strcmp(clients[i].name, target) == 0) {
                        if (clients[i].role == 2) {
                            // Нельзя заглушать админа
                            send(client_socket, "Cannot mute an admin.", strlen("Cannot mute an admin.")+1, 0);
                            found_index = -2;
                        } else {
                            clients[i].mute_until = now + 10;
                            found_index = i;
                        }
                        break;
                    }
                }
                pthread_mutex_unlock(&clients_mutex);

                if (found_index >= 0) {
                    // уведомляем жертву
                    send(clients[found_index].socket,
                        "You have been muted for 10 seconds.", strlen("You have been muted for 10 seconds.")+1, 0);
                    // подтверждаем администратору
                    send(client_socket, "User muted for 10s.", strlen("User muted for 10s.")+1, 0);
                } else if (found_index == -1) {
                    send(client_socket, "User not found.", strlen("User not found.")+1, 0);
                }
                // если found_index == -2, сообщение уже отправлено
                continue;
            }
        }
        // Проверка "mute" для всех, кроме админа
        if (current_client.role != 2) {
            time_t now = time(NULL);
            time_t m_until = 0;
            pthread_mutex_lock(&clients_mutex);
            for (int m = 0; m < client_count; ++m) {
                if (clients[m].socket == client_socket) {
                    m_until = clients[m].mute_until;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            if (now < m_until) {
                send(client_socket, "Вы были заглушены.", strlen("Вы были заглушены.") + 1, 0);
                continue;
            }
        }
        // Обработка выхода
        if (strcmp(buffer, "exit") == 0) {
            printf("%s завершил соединение.\n", current_client.name);
            break;
        }

        char message[BUFFER_SIZE + 50];
        snprintf(message, sizeof(message), "%s: %s", current_client.name, buffer);
        broadcast_message(message, client_socket);
    }

    close(client_socket);
    remove_client(client_socket);
    pthread_exit(NULL);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Создание сокета
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Настройка адреса
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    srand(time(NULL)); // инициализация генератора случайных чисел
    build_obfuscation_list(); // строим список символов

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Сервер слушает на порту %d...\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        pthread_t tid;
        int* pclient = malloc(sizeof(int));
        *pclient = client_fd;
        if (pthread_create(&tid, NULL, handle_client, pclient) != 0) {
            perror("pthread_create failed");
            close(client_fd);
            free(pclient);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_fd);
    free_obfuscation_list();
    return 0;
}

//cd "/Users/ivansalukhov/Desktop/projects vs/c/курсовик ЧАТ/" && gcc server.c -o server && "/Users/ivansalukhov/Desktop/projects vs/c/курсовик ЧАТ/"server