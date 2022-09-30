#ifndef _CHIP8_BACKEND_H_
#define _CHIP8_BACKEND_H_

#include <stdint.h>

#define CHIP8_BACKEND_PIXEL_MULT 20

void CHIP8_backend_exit();
uint32_t CHIP8_backend_init(void);
uint32_t CHIP8_backend_render(void);
uint32_t CHIP8_backend_handle_events(void);

#endif
