#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>

void client_set_credentials(const char* username, const char* password);
bool client_start(void);
void client_register_callback(void (*callback)(const char*));
void client_send_message(const char* msg);
void client_stop(void);

#endif
