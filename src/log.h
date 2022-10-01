#ifndef _LOG_H_
#define _LOG_H_

typedef enum {
    LOG_LEVEL_ALL = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NONE,
} LOG_LEVEL;

typedef void(log_func_t)(LOG_LEVEL level, char *msg);

extern void log_init(void);

/// Register a callback for <level>
// TODO: This seems to be the most logical/practical implementation... ?
//       Also, this should be it's own library
extern void log_register(LOG_LEVEL level, log_func_t func);

/// Set the desired log level at runtime
extern void log_set_level(LOG_LEVEL level);

/// Invoke the appropiate callback
extern void log_emit(LOG_LEVEL level, char *format, ...);

#define log_debug(format, ...) log_emit(LOG_LEVEL_DEBUG, format, __VA_ARGS__)
#define log_info(format, ...)  log_emit(LOG_LEVEL_INFO, format, __VA_ARGS__)
#define log_warn(format, ...)  log_emit(LOG_LEVEL_WARN, format, __VA_ARGS__)
#define log_error(format, ...) log_emit(LOG_LEVEL_ERROR, format, __VA_ARGS__)

#endif
