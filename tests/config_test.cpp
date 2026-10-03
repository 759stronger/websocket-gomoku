#include <cstdlib>
#include <cstring>
#include <iostream>

#if defined(__has_include)
#  if __has_include("gomoku/config.h")
#    include "gomoku/config.h"
#    define GOMOKU_CONFIG_AVAILABLE 1
#  endif
#endif

#ifndef GOMOKU_CONFIG_AVAILABLE
int main() {
    std::cerr << "FAIL: database configuration API is not implemented yet\n";
    return 1;
}
#else
namespace {
int checks = 0;
int failures = 0;

void setEnvironment(const char* name, const char* value) {
#ifdef _WIN32
    if (_putenv_s(name, value ? value : "") != 0) {
        std::cerr << "FAIL: cannot prepare test environment\n";
        std::exit(2);
    }
#else
    const int result = value ? setenv(name, value, 1) : unsetenv(name);
    if (result != 0) {
        std::cerr << "FAIL: cannot prepare test environment\n";
        std::exit(2);
    }
#endif
}

void clearEnvironment() {
    const char* names[] = {
        "GOMOKU_DB_HOST", "GOMOKU_DB_USER", "GOMOKU_DB_PASSWORD",
        "GOMOKU_DB_NAME", "GOMOKU_DB_PORT"
    };
    for (const char* name : names) setEnvironment(name, nullptr);
}

void check(bool condition, const char* description) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << description << '\n';
    }
}

void validCredentials() {
    setEnvironment("GOMOKU_DB_USER", "test-user");
    setEnvironment("GOMOKU_DB_PASSWORD", "test-only-password");
}
}

int main() {
    GomokuDbConfig config = {};
    clearEnvironment();
    check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_MISSING_USER,
          "missing username is rejected");

    setEnvironment("GOMOKU_DB_USER", "test-user");
    check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_MISSING_PASSWORD,
          "missing password is rejected");

    validCredentials();
    check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_OK,
          "valid credentials are accepted");
    check(std::strcmp(config.host, "127.0.0.1") == 0 &&
          std::strcmp(config.database, "gobang") == 0 && config.port == 3306,
          "omitted optional configuration uses safe defaults");
    check(std::strcmp(config.user, "test-user") == 0 &&
          std::strcmp(config.password, "test-only-password") == 0,
          "credentials come from the environment");

    setEnvironment("GOMOKU_DB_HOST", "db.example.invalid");
    setEnvironment("GOMOKU_DB_NAME", "test_schema");
    setEnvironment("GOMOKU_DB_PORT", "5432");
    check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_OK &&
          std::strcmp(config.host, "db.example.invalid") == 0 &&
          std::strcmp(config.database, "test_schema") == 0 && config.port == 5432,
          "explicit optional configuration is retained");

    setEnvironment("GOMOKU_DB_PORT", "1");
    check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_OK && config.port == 1,
          "minimum port is accepted");
    setEnvironment("GOMOKU_DB_PORT", "65535");
    check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_OK && config.port == 65535,
          "maximum port is accepted");

    const char* invalidPorts[] = {
        "0", "65536", "-1", "+3306", "3306x",
        " 3306", "3306 ", "1.5", "999999999999999999999999"
    };
    for (const char* port : invalidPorts) {
        setEnvironment("GOMOKU_DB_PORT", port);
        check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_INVALID_PORT,
              "invalid, signed, padded, fractional or overflowing port is rejected");
    }

    clearEnvironment();
    setEnvironment("GOMOKU_DB_PASSWORD", "test-only-password");
    check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_MISSING_USER,
          "password alone does not satisfy username requirement");
    validCredentials();
    setEnvironment("GOMOKU_DB_PASSWORD", "");
    check(gomoku_db_config_from_env(&config) == GOMOKU_CONFIG_MISSING_PASSWORD,
          "empty password is rejected");

    std::cout << "Configuration checks: " << checks
              << ", failures: " << failures << '\n';
    return failures ? 1 : 0;
}
#endif
