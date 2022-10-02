#include "chip8.h"
#include "log.h"
#include "backend.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

// TODO: Different hardware might have different clock resolutions...
#define INT_PER_FRAME 50

#define CHIP8_TIMER_RENDER_HZ 60
#define CHIP8_TIMER_RENDER_RATE_NSEC (int)(1000000000 / CHIP8_TIMER_RENDER_HZ)

#define CHIP8_TIMER_TIMER_HZ 60
#define CHIP8_TIMER_TIMER_RATE_NSEC (int)(1000000000 / CHIP8_TIMER_TIMER_HZ)

#define CHIP8_TIMER_CPU_HZ (INT_PER_FRAME * CHIP8_TIMER_RENDER_HZ)
#define CHIP8_TIMER_CPU_RATE_NSEC (int)(1000000000 / CHIP8_TIMER_CPU_HZ)

void log_func_impl(LOG_LEVEL level, char *str) {
    switch (level) {
    case LOG_LEVEL_DEBUG:
        fprintf(stdout, "[\033[34mDEBUG\033[0m] %s\n", str);
        break;
    case LOG_LEVEL_INFO:
        fprintf(stdout, "[\033[32mINFO\033[0m] %s\n", str);
        break;
    case LOG_LEVEL_WARN:
        fprintf(stdout, "[\033[33mWARN\033[0m] %s\n", str);
        break;
    case LOG_LEVEL_ERROR:
        fprintf(stderr, "[\033[31mERROR\033[0m] %s\n", str);
        break;
    default:
        break;
    }
}

uint32_t CHIP8_run(void) {
    struct timespec timer_tick, timer_timer, timer_render, now;

    size_t diff_tick = 0;
    size_t diff_timer = 0;
    size_t diff_render = 0;

    uint8_t is_running = 1;
    uint8_t is_running_chip = 1;

    uint32_t exit_code = 0;

    if (CHIP8_backend_init() == 1) {
        log_error("%s", "Failed to initalize backend");
        exit(1);
    }

    clock_gettime(CLOCK_REALTIME, &timer_tick);
    clock_gettime(CLOCK_REALTIME, &timer_timer);
    clock_gettime(CLOCK_REALTIME, &timer_render);

    while (is_running) {
        clock_gettime(CLOCK_REALTIME, &now);

        diff_tick = now.tv_nsec - timer_tick.tv_nsec;
        if (is_running_chip && diff_tick >= CHIP8_TIMER_CPU_RATE_NSEC) {
            int32_t cpu_status = CHIP8_cpu_cycle();

            switch (cpu_status) {
            case -1: // invalid optcode
                exit(1);
            case 2: // optcode: exit
                is_running = 0;
                break;
            }

            clock_gettime(CLOCK_REALTIME, &timer_tick);
        }

        diff_timer = now.tv_nsec - timer_timer.tv_nsec;
        if (is_running_chip && diff_timer >= CHIP8_TIMER_TIMER_RATE_NSEC) {
            CHIP8_timer_tick();

            clock_gettime(CLOCK_REALTIME, &timer_timer);
        }

        diff_render = now.tv_nsec - timer_render.tv_nsec;
        if (diff_render >= CHIP8_TIMER_RENDER_RATE_NSEC) {
            CHIP8_backend_render();
            is_running = !CHIP8_backend_handle_events();

            clock_gettime(CLOCK_REALTIME, &timer_render);
        }
    }

    CHIP8_backend_exit();

    return exit_code;
}

int main(int argc, char **argv) {
    log_init();
    log_register(LOG_LEVEL_ALL, log_func_impl);

    if(!argv[1]) {
        log_error("%s", "Missing rom path");
        exit(2);
    }

    char *path = argv[1];

    CHIP8_init();

    uint32_t result = CHIP8_load_from_path(path);
    if(result != 0) {
        log_error("Failed to load rom (%s): %s", strerror(errno), path);
        exit(1);
    }

    log_info("Loaded rom from path: %s", path);

    uint32_t exit_code = CHIP8_run();
    CHIP8_exit();

    return exit_code;
}
