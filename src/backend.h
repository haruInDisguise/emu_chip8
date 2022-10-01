#ifndef _CHIP8_BACKEND_H_
#define _CHIP8_BACKEND_H_

#include <stdint.h>

extern void CHIP8_backend_exit();
extern uint32_t CHIP8_backend_init(void);
extern uint32_t CHIP8_backend_render(void);
extern uint32_t CHIP8_backend_handle_events(void);

#endif
