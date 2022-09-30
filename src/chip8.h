#ifndef _CHIP8_H_
#define _CHIP8_H_

#include <stdint.h>

#define CHIP8_BITPLANE_0   0x1  // layer 1
#define CHIP8_BITPLANE_1   0x2  // layer 2
#define CHIP8_BITPLANE_2   0x4  // layer 3
#define CHIP8_BITPLANE_3   0x8  // layer 4
#define CHIP8_BITPLANE_ALL 0x10 // all layers
#define CHIP8_BITPLANE_BITS 0x4

typedef enum {
    CHIP8_KEY_RELEASED,
    CHIP8_KEY_PRESSED,
} CHIP8_KEYSTATE;

typedef enum {
    CHIP8_KEY_0,
    CHIP8_KEY_1,
    CHIP8_KEY_2,
    CHIP8_KEY_3,
    CHIP8_KEY_4,
    CHIP8_KEY_5,
    CHIP8_KEY_6,
    CHIP8_KEY_7,
    CHIP8_KEY_8,
    CHIP8_KEY_9,
    CHIP8_KEY_A,
    CHIP8_KEY_B,
    CHIP8_KEY_C,
    CHIP8_KEY_D,
    CHIP8_KEY_E,
    CHIP8_KEY_F,
} CHIP8_KEY;

extern const uint32_t CHIP8_init(void);
extern const uint32_t CHIP8_reset(void);
extern void CHIP8_exit(void);

extern const uint32_t CHIP8_memcpy(void *buffer);
extern const uint32_t CHIP8_load_from_path(const char *path);

extern void CHIP8_timer_tick(void);
extern const int32_t CHIP8_cpu_cycle(void);

extern const uint8_t CHIP8_screen_get_pixel(uint8_t x, uint8_t y);
extern const uint8_t CHIP8_screen_get_resolution(uint8_t *width, uint8_t *height);
extern const uint8_t CHIP8_screen_get_update_status(void);

extern void CHIP8_input_set(CHIP8_KEY key, CHIP8_KEYSTATE state);

#endif
