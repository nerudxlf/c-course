#include "logger.h"
#include <stdio.h>
#include <stdbool.h>

int main(void){
    Logger* logger = create_logger(OUT_CONSOLE, false, NULL);
    info(logger, "Hello world");
    warning(logger, "This is a warning");
    debug(logger, "Some debug message");
    error(logger, "Something went wrong");
    free_logger(logger);
    return 0;
}