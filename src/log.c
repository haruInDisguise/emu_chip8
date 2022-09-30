#include "log.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LOG_BUFFER_SIZE 512

static LOG_LEVEL log_level = LOG_LEVEL_ALL;
static char log_buffer[LOG_BUFFER_SIZE];

static log_func_t *func_debug = NULL;
static log_func_t *func_info = NULL;
static log_func_t *func_warn = NULL;
static log_func_t *func_error = NULL;

void log_init(void) {
    char *level = getenv("LOG_LEVEL");

    if(level == NULL) {
        log_level = LOG_LEVEL_ALL;
        return;
    }

    if(strcmp("all", level) == 0)
        log_level = LOG_LEVEL_ALL;
    else if(strcmp("debug", level) == 0)
        log_level = LOG_LEVEL_DEBUG;
    else if(strcmp("info", level) == 0)
        log_level = LOG_LEVEL_INFO;
    else if(strcmp("warn", level) == 0)
        log_level = LOG_LEVEL_WARN;
    else if(strcmp("error", level) == 0)
        log_level = LOG_LEVEL_ERROR;
    else if(strcmp("none", level) == 0)
        log_level = LOG_LEVEL_NONE;
}

void log_set_level(LOG_LEVEL level) { log_level = level; }

void log_register(LOG_LEVEL level, log_func_t func) {
    assert(func);

    switch (level) {
    case LOG_LEVEL_DEBUG:
        func_debug = func;
        break;
    case LOG_LEVEL_INFO:
        func_info = func;
        break;
    case LOG_LEVEL_WARN:
        func_warn = func;
        break;
    case LOG_LEVEL_ERROR:
        func_error = func;
        break;
    case LOG_LEVEL_ALL:
        func_debug = func;
        func_info = func;
        func_warn = func;
        func_error = func;
        break;
    default:
        fprintf(stderr, "%s: Invalid logging level", __func__);
        abort();
    }
}

void log_emit(LOG_LEVEL level, char *format, ...) {
    assert(format != NULL);

    if (level < log_level)
        return;

    va_list valist;
    va_start(valist, format);

    int result = vsprintf(log_buffer, format, valist);
    assert(result >= 0);

    va_end(valist);

    switch (level) {
    case LOG_LEVEL_DEBUG:
        assert(func_debug != NULL);
        func_debug(LOG_LEVEL_DEBUG, log_buffer);
        break;
    case LOG_LEVEL_INFO:
        assert(func_info != NULL);
        func_info(LOG_LEVEL_INFO, log_buffer);
        break;
    case LOG_LEVEL_WARN:
        assert(func_warn != NULL);
        func_warn(LOG_LEVEL_WARN, log_buffer);
        break;
    case LOG_LEVEL_ERROR:
        assert(func_error != NULL);
        func_error(LOG_LEVEL_ERROR, log_buffer);
        break;
    default:
        fprintf(stderr, "%s: Invalid logging level", __func__);
        abort();
    }
}
