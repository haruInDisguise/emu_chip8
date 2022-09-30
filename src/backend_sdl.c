#include "backend.h"
#include "chip8.h"
#include "log.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>

#define WIN_WIDTH 64 * 20
#define WIN_HEIGHT 32 * 20

#define ASSERT_SDL(condition)                                                  \
    do {                                                                       \
        if ((condition)) {                                                     \
            log_error("%s", SDL_GetError());                                   \
            return 1;                                                          \
        }                                                                      \
    } while (0);

static SDL_Window *win = NULL;
static SDL_Renderer *renderer = NULL;

static SDL_Rect rect;

uint32_t CHIP8_backend_init(void) {
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);

    win = SDL_CreateWindow("emu_chip8 - press <ESC> to quit", 0, 0, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_OPENGL);
    ASSERT_SDL(win == NULL);

    renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    ASSERT_SDL(renderer == NULL);

    return 0;
}

void CHIP8_backend_exit(void) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
}

uint32_t CHIP8_backend_render(void) {
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 155, 155, 155, 255);

    uint8_t width, height;
    CHIP8_screen_get_resolution(&width, &height);
    uint32_t pixel_size = WIN_WIDTH / width;

    rect.w = pixel_size;
    rect.h = pixel_size;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (CHIP8_screen_get_pixel(x, y) > 0) {
                rect.x = x * pixel_size;
                rect.y = y * pixel_size;
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    SDL_RenderPresent(renderer);

    return 0;
}

uint32_t CHIP8_backend_handle_events(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            uint8_t keystate = event.key.state == SDL_PRESSED
                                   ? CHIP8_KEY_PRESSED
                                   : CHIP8_KEY_RELEASED;
            switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                return 1;
                break;
            case SDL_SCANCODE_1: // 0x0
                CHIP8_input_set(0x0, keystate);
                break;
            case SDL_SCANCODE_2: // 0x1
                CHIP8_input_set(0x1, keystate);
                break;
            case SDL_SCANCODE_3: // 0x2
                CHIP8_input_set(0x2, keystate);
                break;
            case SDL_SCANCODE_4: // 0x3
                CHIP8_input_set(0x3, keystate);
                break;
            case SDL_SCANCODE_Q: // 0x4
                CHIP8_input_set(0x4, keystate);
                break;
            case SDL_SCANCODE_W: // 0x5
                CHIP8_input_set(0x5, keystate);
                break;
            case SDL_SCANCODE_E: // 0x6
                CHIP8_input_set(0x6, keystate);
                break;
            case SDL_SCANCODE_R: // 0x7
                CHIP8_input_set(0x7, keystate);
                break;
            case SDL_SCANCODE_A: // 0x8
                CHIP8_input_set(0x8, keystate);
                break;
            case SDL_SCANCODE_S: // 0x9
                CHIP8_input_set(0x9, keystate);
                break;
            case SDL_SCANCODE_D: // 0xA
                CHIP8_input_set(0xa, keystate);
                break;
            case SDL_SCANCODE_F: // 0xB
                CHIP8_input_set(0xb, keystate);
                break;
            case SDL_SCANCODE_Z: // 0xC
                CHIP8_input_set(0xc, keystate);
                break;
            case SDL_SCANCODE_X: // 0xD
                CHIP8_input_set(0xd, keystate);
                break;
            case SDL_SCANCODE_C: // 0xE
                CHIP8_input_set(0xe, keystate);
                break;
            case SDL_SCANCODE_V: // 0xF
                CHIP8_input_set(0xf, keystate);
                break;
            default:
                break;
            }
        }
    }

    return 0;
}

