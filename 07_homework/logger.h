#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>

enum LoggerMode {
    OUT_FILE,
    OUT_CONSOLE,
};

typedef struct Logger Logger;

Logger* create_logger(
    enum LoggerMode mode,
    bool debug,
    char* filename
);
void free_logger(Logger* current_logger);
void info(Logger* current_logger, char* message);
void debug(Logger* current_logger, char* message);
void warning(Logger* current_logger, char* message);
void error(Logger* current_logger, char* message);

#endif