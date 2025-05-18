#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <execinfo.h>

enum LoggerMode {
    OUT_FILE,
    OUT_CONSOLE,
};

typedef struct {
    enum LoggerMode mode;
    bool debug;
    pthread_mutex_t mutex;
    char* filename;
} Logger;

static const char* string_template = "[%s] %-5s: %s\n";

static void thread_safe_print(Logger* current_logger, char* level, char* time_str, char* message){
    pthread_mutex_lock(&current_logger->mutex);
    printf(string_template, time_str, level, message);
    fflush(stdout);
    pthread_mutex_unlock(&current_logger->mutex);
}

static void thread_safe_file_out(Logger *current_logger, char* level, char* time_str, char* message){
    pthread_mutex_lock(&current_logger->mutex);
    FILE* file = fopen(current_logger->filename, "a");
    if(file == NULL){
        return;
    }
    fprintf(file, string_template, time_str, level, message);
    fclose(file);
    pthread_mutex_unlock(&current_logger->mutex);
}

static char* get_datetime(){
    static char buffer[64];
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);
    return strdup(buffer);
}

Logger* create_logger(
    enum LoggerMode mode,
    bool debug,
    char* filename
){
    Logger* tmp_logger = (Logger*)malloc(sizeof(Logger));
    if (tmp_logger == NULL){
        return NULL;
    }
    if (filename == NULL){
        filename = "out.log";
    }
    tmp_logger->mode = mode;
    tmp_logger->debug = debug;
    tmp_logger->filename = filename;
    pthread_mutex_init(&tmp_logger->mutex, NULL);
    return tmp_logger;
}

void free_logger(Logger* current_logger){
    if (current_logger != NULL) {
        pthread_mutex_destroy(&current_logger->mutex);
        free(current_logger);
    }
}

void info(Logger* current_logger, char* message){
    if (current_logger == NULL){
        return;
    }
    char* time_str = get_datetime();
    if (current_logger->mode == OUT_CONSOLE){
        thread_safe_print(
            current_logger,
            "INFO",
            time_str,
            message
        );
    } else {
        thread_safe_file_out(
            current_logger,
            "INFO",
            time_str,
            message
        );
    }
    free(time_str);
}

void debug(Logger* current_logger, char* message){
    if (current_logger == NULL){
        return;
    }
    if (current_logger->debug == false){
        return;
    }
    char* time_str = get_datetime();
    if (current_logger->mode == OUT_CONSOLE){
        thread_safe_print(
            current_logger,
            "DEBUG",
            time_str,
            message
        );
    } else {
        thread_safe_file_out(
            current_logger,
            "DEBUG",
            time_str,
            message
        );
    }
    free(time_str);
}

void warning(Logger* current_logger, char* message){
    if (current_logger == NULL){
        return;
    }
    char* time_str = get_datetime();
    if (current_logger->mode == OUT_CONSOLE){
        thread_safe_print(
            current_logger,
            "WARNING",
            time_str,
            message
        );
    } else {
        thread_safe_file_out(
            current_logger,
            "WARNING",
            time_str,
            message
        );
    }
    free(time_str);
}

void error(Logger* current_logger, char* message) {
    if (current_logger == NULL || message == NULL) return;

    char* time_str = get_datetime();
    if (time_str == NULL) return;

    void* array[10];
    size_t size = backtrace(array, 10);
    char** strings = backtrace_symbols(array, size);

    size_t capacity = 2048;
    char* result_message = malloc(capacity);
    if (result_message == NULL) {
        free(time_str);
        return;
    }

    snprintf(result_message, capacity, "%s\nError: signal %d:\n", message, SIGSEGV);
    size_t len = strlen(result_message);

    if (strings != NULL) {
        for (size_t i = 0; i < size && len < capacity - 100; i++) {
            len += snprintf(result_message + len, capacity - len, "%s\n", strings[i]);
        }
        free(strings);
    } else {
        snprintf(result_message + len, capacity - len, "backtrace_symbols failed\n");
    }

    if (current_logger->mode == OUT_CONSOLE) {
        thread_safe_print(current_logger, "ERROR", time_str, result_message);
    } else {
        thread_safe_file_out(current_logger, "ERROR", time_str, result_message);
    }

    free(result_message);
    free(time_str);
}
