#ifndef GOMOKU_CONFIG_H
#define GOMOKU_CONFIG_H

#include <stdlib.h>

/* Shared by the C++ server and the independent C MySQL example.
 * Only configuration is validated here; authentication SQL is unchanged.
 */
typedef struct GomokuDbConfig {
    const char* host;
    const char* user;
    const char* password;
    const char* database;
    unsigned short port;
} GomokuDbConfig;

typedef enum GomokuConfigStatus {
    GOMOKU_CONFIG_OK = 0,
    GOMOKU_CONFIG_MISSING_USER,
    GOMOKU_CONFIG_MISSING_PASSWORD,
    GOMOKU_CONFIG_INVALID_PORT
} GomokuConfigStatus;

/* config must point to a valid object. Environment strings are not copied.
 * Missing optional values use local-development defaults; credentials never do.
 */
static inline GomokuConfigStatus gomoku_db_config_from_env(GomokuDbConfig* config) {
    const char* host = getenv("GOMOKU_DB_HOST");
    const char* user = getenv("GOMOKU_DB_USER");
    const char* password = getenv("GOMOKU_DB_PASSWORD");
    const char* database = getenv("GOMOKU_DB_NAME");
    const char* port_text = getenv("GOMOKU_DB_PORT");
    unsigned int port = 3306;

    if (user == NULL || user[0] == '\0') return GOMOKU_CONFIG_MISSING_USER;
    if (password == NULL || password[0] == '\0') return GOMOKU_CONFIG_MISSING_PASSWORD;

    if (port_text != NULL) {
        const char* cursor = port_text;
        port = 0;
        if (*cursor == '\0') return GOMOKU_CONFIG_INVALID_PORT;
        while (*cursor != '\0') {
            unsigned int digit;
            if (*cursor < '0' || *cursor > '9') return GOMOKU_CONFIG_INVALID_PORT;
            digit = (unsigned int)(*cursor - '0');
            if (port > (65535U - digit) / 10U) return GOMOKU_CONFIG_INVALID_PORT;
            port = port * 10U + digit;
            ++cursor;
        }
        if (port == 0) return GOMOKU_CONFIG_INVALID_PORT;
    }

    config->host = (host != NULL && host[0] != '\0') ? host : "127.0.0.1";
    config->user = user;
    config->password = password;
    config->database = (database != NULL && database[0] != '\0') ? database : "gobang";
    config->port = (unsigned short)port;
    return GOMOKU_CONFIG_OK;
}

#endif
