#include "chip8.h"
#include "log.h"

#include <assert.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Macros for pretty printing instructions + state. Basically *amazing* print
// debugging
// TODO: Use the 'desc' and 'mode' (print_opt) argument... Also, this whole
// thing is just meh. Print some sort of register table instead?
// clang-format off
#define print_fmt(inst, desc, format, ...) log_emit(LOG_LEVEL_DEBUG, "PC=0x%04x %5s " format, machine->pc, inst, __VA_ARGS__)

#define print_opt_x(inst, desc)    print_fmt(inst, desc, "  x = %-2hu [Vx=%2hu]", x, machine->reg[x])
#define print_opt_n(inst, desc)    print_fmt(inst, desc, "  n = %-2hu", n)
#define print_opt_xy(inst, desc)   print_fmt(inst, desc, "  x = %-2hu [Vx=%2hu]  y = %-2hu [Vy=%2hu]", x, machine->reg[x], y, machine->reg[y])
#define print_opt_xyn(inst, desc)  print_fmt(inst, desc, "  x = %-2hu [Vx=%2hu]  y = %-2hu [Vy=%2hu] n =%2hu", x, machine->reg[x], y, machine->reg[y], n)
#define print_opt_xkk(inst, desc)  print_fmt(inst, desc, "  x = %-2hu [Vx=%2hu] kk = %-3hu", x, machine->reg[x], kk)
#define print_opt_nnn(inst, desc)  print_fmt(inst, desc, "nnn = 0x%-3x", nnn)
#define print_opt_none(inst, desc) print_fmt(inst, desc, "", "")

#define print_opt(inst, desc, format, mode) print_opt_##format(inst, desc)

#define print_debug(format, ...) log_debug(format, __VA_ARGS__)
#define print_info(format, ...) log_info(format, __VA_ARGS__)
#define print_error(format, ...) log_error(format, __VA_ARGS__)
#define print_warn(format, ...) log_warn(format, __VA_ARGS__)
// clang-format on

// -------------

// Get word from memory at 'index'.
#define MEM_GET_WORD(index)                                                    \
    ((machine->mem[(index)] << 8) | machine->mem[(index) + 1])

// Macros for iterating over the bitplane/mask and
// execute code only for actually set planes
#define BITPLANE_ITER_START(selected_bitplane)                                 \
    for (uint32_t bitplane_iter__index = 0;                                    \
         bitplane_iter__index < CHIP8_BITPLANE_BITS; bitplane_iter__index++) { \
        uint32_t bitplane_iter__is_selected =                                  \
            (machine->screen_bitplane >> bitplane_iter__index) & 0x1;          \
        (selected_bitplane) = bitplane_iter__index + 1;                        \
        if (bitplane_iter__is_selected) {

#define BITPLANE_ITER_END                                                      \
    } /* is_selected */                                                        \
    } /* bitmask_iter__index */

// To be used inbetween BITPLANE_ITER_START/END
// XOR the selected bitplane with value
#define BITPLANE_TOGGLE(x, y, value)                                           \
    machine->screen[(y)*CHIP8_SCREEN_BUFFER_WIDTH + (x)] ^=                    \
        ((value) << bitplane_iter__index)

// Set in selected bitplane to value
#define BITPLANE_SET(x, y, value)                                              \
    machine->screen[((y)*CHIP8_SCREEN_BUFFER_WIDTH) + (x)] =                   \
        (machine->screen[y * CHIP8_SCREEN_BUFFER_WIDTH + x] &                  \
         ~(1 << bitplane_iter__index)) |                                       \
        (((value) << bitplane_iter__index))

// Get the value of the selected bitplane
#define BITPLANE_GET(x, y)                                                     \
    (machine->screen[(y)*CHIP8_SCREEN_BUFFER_WIDTH + (x)] >>                   \
     bitplane_iter__index) &                                                   \
        0x01

// -------------

#define CHIP8_MEM_SIZE 1024 * 64
#define CHIP8_MEM_OFFSET 512

#define CHIP8_FONTSET_CHAR_SIZE 5
#define CHIP8_FONTSET_CHAR_SIZE_SUPER 10
#define CHIP8_FONTSET_SIZE 16
#define CHIP8_FONTSET_SIZE_SUPER 16
#define CHIP8_FONTSET_OFFSET 0
#define CHIP8_FONTSET_OFFSET_SUPER CHIP8_FONTSET_SIZE *CHIP8_FONTSET_CHAR_SIZE

#define CHIP8_KEYS 16
#define CHIP8_STACK_SIZE 16
#define CHIP8_REGISTERS 16
#define CHIP8_FLAG_REGISTERS 8

// The screen buffer always uses the larges available size
// and restricts its drawing area to the machine->width/height
// values
#define CHIP8_SCREEN_WIDTH 64
#define CHIP8_SCREEN_HEIGHT 32
#define CHIP8_SCREEN_WIDTH_HIRES 128
#define CHIP8_SCREEN_HEIGHT_HIRES 64

#define CHIP8_SCREEN_BUFFER_WIDTH CHIP8_SCREEN_WIDTH_HIRES
#define CHIP8_SCREEN_BUFFER_HEIGHT CHIP8_SCREEN_HEIGHT_HIRES

// TODO: Enable/Disable certain extensions
// Currently only used as tokens in descriptive macros
typedef enum {
    CHIP8_MODE_CH8, // Normal
    CHIP8_MODE_SH8, // Super (SCHIP)
    CHIP8_MODE_C48, // TODO: HP CHIP 48 flag registers
    CHIP8_MODE_XH8, // Xo
} CHIP8_MODE;

typedef enum {
    CHIP8_SCROLL_UP,
    CHIP8_SCROLL_DOWN,
    CHIP8_SCROLL_LEFT,
    CHIP8_SCROLL_RIGHT,
} CHIP8_SCROLL_DIR;

struct {
    uint8_t reg[CHIP8_REGISTERS];

    uint8_t flag_reg[CHIP8_FLAG_REGISTERS];
    uint8_t mem[CHIP8_MEM_SIZE];
    uint8_t keys[CHIP8_KEYS];

    uint8_t screen[CHIP8_SCREEN_BUFFER_HEIGHT * CHIP8_SCREEN_BUFFER_WIDTH];
    uint8_t screen_width;
    uint8_t screen_height;
    uint8_t screen_bitplane;
    uint8_t screen_is_hires;
    uint8_t screen_update_status;

    uint8_t timer_sound;
    uint8_t timer;

    uint16_t stack[CHIP8_STACK_SIZE];
    uint16_t sp;
    uint16_t pc;
    uint16_t index_reg;
} *machine = NULL;

static uint8_t fontset[CHIP8_FONTSET_SIZE * CHIP8_FONTSET_CHAR_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};
// stolen from: https://github.com/wernsey/chip8/blob/master/chip8.c
static uint8_t
    fontset_super[CHIP8_FONTSET_SIZE_SUPER * CHIP8_FONTSET_CHAR_SIZE_SUPER] = {
        /* '0' */ 0x7C, 0x82, 0x82, 0x82, 0x82,
        0x82,           0x82, 0x82, 0x7C, 0x00,
        /* '1' */ 0x08, 0x18, 0x38, 0x08, 0x08,
        0x08,           0x08, 0x08, 0x3C, 0x00,
        /* '2' */ 0x7C, 0x82, 0x02, 0x02, 0x04,
        0x18,           0x20, 0x40, 0xFE, 0x00,
        /* '3' */ 0x7C, 0x82, 0x02, 0x02, 0x3C,
        0x02,           0x02, 0x82, 0x7C, 0x00,
        /* '4' */ 0x84, 0x84, 0x84, 0x84, 0xFE,
        0x04,           0x04, 0x04, 0x04, 0x00,
        /* '5' */ 0xFE, 0x80, 0x80, 0x80, 0xFC,
        0x02,           0x02, 0x82, 0x7C, 0x00,
        /* '6' */ 0x7C, 0x82, 0x80, 0x80, 0xFC,
        0x82,           0x82, 0x82, 0x7C, 0x00,
        /* '7' */ 0xFE, 0x02, 0x04, 0x08, 0x10,
        0x20,           0x20, 0x20, 0x20, 0x00,
        /* '8' */ 0x7C, 0x82, 0x82, 0x82, 0x7C,
        0x82,           0x82, 0x82, 0x7C, 0x00,
        /* '9' */ 0x7C, 0x82, 0x82, 0x82, 0x7E,
        0x02,           0x02, 0x82, 0x7C, 0x00,
        /* 'A' */ 0x10, 0x28, 0x44, 0x82, 0x82,
        0xFE,           0x82, 0x82, 0x82, 0x00,
        /* 'B' */ 0xFC, 0x82, 0x82, 0x82, 0xFC,
        0x82,           0x82, 0x82, 0xFC, 0x00,
        /* 'C' */ 0x7C, 0x82, 0x80, 0x80, 0x80,
        0x80,           0x80, 0x82, 0x7C, 0x00,
        /* 'D' */ 0xFC, 0x82, 0x82, 0x82, 0x82,
        0x82,           0x82, 0x82, 0xFC, 0x00,
        /* 'E' */ 0xFE, 0x80, 0x80, 0x80, 0xF8,
        0x80,           0x80, 0x80, 0xFE, 0x00,
        /* 'F' */ 0xFE, 0x80, 0x80, 0x80, 0xF8,
        0x80,           0x80, 0x80, 0x80, 0x00,
};

static inline uint8_t CHIP8_get_rand(void) { return rand() % 255; }

static void CHIP8_screen_draw(const uint8_t reg_x, const uint8_t reg_y,
                              const uint8_t n) {
    uint8_t width, height;
    CHIP8_screen_get_resolution(&width, &height);

    const uint8_t pos_x = machine->reg[reg_x] % width;
    const uint8_t pos_y = machine->reg[reg_y] % height;

    // Reset flag register (i.e. collision detection)
    machine->reg[0x0f] = 0;

    const uint8_t sprite_height = n == 0 ? 16 : n;
    const uint8_t sprite_width = n == 0 ? 16 : 8;

    // Iterate over the bitplanes and proceed to draw
    // matches (i.e. b0111 will draw on bitplane 1, 2 and 3)
    uint8_t selected_bitplane;

    BITPLANE_ITER_START(selected_bitplane);

    // Draw the actual sprite
    for (uint32_t offset_y = 0; offset_y < sprite_height; offset_y++) {
        if (pos_y + offset_y >= height)
            return;

        // Select different graphics data, for individual bitplanes.
        uint16_t bitmask;
        const uint16_t mem_index = machine->index_reg +
            (selected_bitplane - 1) * sprite_height +
            (sprite_width / 8) * offset_y;
        if (n == 0)
            bitmask = MEM_GET_WORD(mem_index);
        else
            bitmask = machine->mem[mem_index];

        for (uint32_t offset_x = 0; offset_x < sprite_width; offset_x++) {
            if (pos_x + offset_x >= width)
                continue;

            const uint8_t bit =
                ((bitmask >> (sprite_width - offset_x - 1)) & 0x1);
            const uint8_t old_value =
                BITPLANE_GET(pos_x + offset_x, pos_y + offset_y);

            BITPLANE_TOGGLE(pos_x + offset_x, pos_y + offset_y, bit);

            if (bit == 1 && old_value > 0)
                machine->reg[0xf] = 1;
        }

    }

    BITPLANE_ITER_END;
}

// Scroll the screen horizontally
static void CHIP8_screen_scroll(const int8_t amount,
                                CHIP8_SCROLL_DIR direction) {
    uint8_t width, height;
    CHIP8_screen_get_resolution(&width, &height);

    // Iterate over the bitplanes and proceed to draw
    // matches (i.e. b0111 will draw on bitplane 1, 2 and 3)
    uint8_t selected_bitplane = 1;

    BITPLANE_ITER_START(selected_bitplane);

    // TODO: This feels messy..
    switch (direction) {
    case CHIP8_SCROLL_LEFT:
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                if (x < amount || x + amount >= width)
                    BITPLANE_SET(x, y, 0);
                else
                    BITPLANE_SET(x - amount, y, BITPLANE_GET(x, y));
                BITPLANE_SET(x, y, 0);
            }
        }
        break;
    case CHIP8_SCROLL_RIGHT:
        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = width - 1; x > 0; x--) {
                if (x + amount >= width || x < 0) {
                    BITPLANE_SET(x, y, 0);
                } else {
                    BITPLANE_SET(x + amount, y, BITPLANE_GET(x, y));
                    BITPLANE_SET(x, y, 0);
                }
            }
        }
        break;
    case CHIP8_SCROLL_UP:
        for (uint32_t x = 0; x < width; x++) {
            for (uint32_t y = 0; y < height; y++) {
                if (y < amount || y + amount >= height) {
                    BITPLANE_SET(x, y, 0);
                } else {
                    BITPLANE_SET(x, y - amount, BITPLANE_GET(x, y));
                    BITPLANE_SET(x, y, 0);
                }
            }
        }
        break;
    case CHIP8_SCROLL_DOWN:
        for (uint32_t x = 0; x < width - 1; x++) {
            for (uint32_t y = height - 1; y > 0; y--) {
                if (y + amount >= height || y < 0)
                    BITPLANE_SET(x, y, 0);
                else {
                    BITPLANE_SET(x, y + amount, BITPLANE_GET(x, y));
                    BITPLANE_SET(x, y, 0);
                }
            }
        }
        break;
    }

    BITPLANE_ITER_END;
}

const uint8_t CHIP8_screen_get_update_status(void) {
    return machine->screen_update_status;
}

const uint8_t CHIP8_screen_get_pixel(const uint8_t x, const uint8_t y) {
    return machine->screen[y * CHIP8_SCREEN_BUFFER_WIDTH + x];
}

const uint8_t CHIP8_screen_get_resolution(uint8_t *width, uint8_t *height) {
    if (width == NULL && height == NULL)
        return machine->screen_is_hires;

    if (width != NULL)
        *width = machine->screen_width;
    if (height != NULL)
        *height = machine->screen_height;

    return machine->screen_is_hires;
}

void CHIP8_input_set(const CHIP8_KEY key, const CHIP8_KEYSTATE state) {
    machine->keys[key] = state;
}

const uint32_t CHIP8_reset(void) {
    memset(machine, 0, sizeof(*machine));
    memcpy(machine->mem + CHIP8_FONTSET_OFFSET, fontset, sizeof(fontset));
    memcpy(machine->mem + CHIP8_FONTSET_OFFSET_SUPER, fontset_super,
           sizeof(fontset_super));

    machine->pc = 0x200;
    machine->screen_bitplane = CHIP8_BITPLANE_0;
    machine->screen_width = CHIP8_SCREEN_WIDTH;
    machine->screen_height = CHIP8_SCREEN_HEIGHT;

    return 0;
}

const uint32_t CHIP8_init(void) {
    machine = malloc(sizeof(*machine));

    // If malloc fails, we might aswell abort...
    if (machine == NULL)
        abort();

    srand(time(NULL));
    CHIP8_reset();

    return 0;
}

void CHIP8_exit(void) { free(machine); }

const uint32_t CHIP8_memcpy(void *src) {
    assert(src != NULL);
    void *result = memcpy(machine->mem + CHIP8_MEM_OFFSET, src,
                          CHIP8_MEM_SIZE - CHIP8_MEM_OFFSET - 1);
    assert(result != NULL);

    return 0;
}

const uint32_t CHIP8_load_from_path(const char *path) {
    assert(path != NULL);

    FILE *file = fopen(path, "rb");
    if (file == NULL)
        return 1;

    // TODO: Make sure that the rom fits into memory
    fread(machine->mem + CHIP8_MEM_OFFSET, 1,
          CHIP8_MEM_SIZE - CHIP8_MEM_OFFSET - 1, file);

    if (ferror(file))
        return 1;

    fclose(file);
    return 0;
}

void CHIP8_timer_tick() {
    if (machine->timer > 0) {
        machine->timer -= 1;
        print_fmt("TIMER", "Decrease the timer by 1", "value = %d",
                  machine->timer);
    }
    if (machine->timer_sound > 0) {
        machine->timer_sound -= 1;
        print_fmt("STIMER", "Decrease the sound timer by 1", "value = %d",
                  machine->timer);
    }
}

const int32_t CHIP8_cpu_cycle() {
    // TODO: This assumes little endian...
    uint16_t optcode = MEM_GET_WORD(machine->pc);

    // 12-bit address
    uint16_t nnn = optcode & 0x0fff;
    // 8-bit constant
    uint8_t kk = optcode & 0x00ff;
    // 4-bit constant
    uint8_t n = optcode & 0x000f;
    // 4-bit register index (high bits)
    uint8_t y = (optcode >> 4) & 0x000f;
    // 4-bit register index (low bits)
    uint8_t x = (optcode >> 8) & 0x000f;

    // ref: http://devernay.free.fr/hacks/chip8/schip.txt
    // xo:
    // http://johnearnest.github.io/Octo/docs/XO-ChipSpecification.html
    switch (optcode & 0xf000) {
    case 0x0000:
        if (x == 0 && y == 0xc) {
            print_opt("SCRD", "Scroll screen down by n pixels", n,
                    CHIP8_MODE_SC8);
            CHIP8_screen_scroll(n, CHIP8_SCROLL_DOWN);
            machine->screen_update_status = 1;
            machine->pc += 2;
            break;
        } else if (x == 0 && y == 0xd) {
            print_opt("SCRU", "Scroll screen up by n pixels", n,
                    CHIP8_MODE_XC8);
            CHIP8_screen_scroll(n, CHIP8_SCROLL_UP);
            machine->screen_update_status = 1;
            machine->pc += 2;
            break;
        }
        switch (kk) {
        case 0xe0:
            print_opt("CLS", "Clear the screen", none, CHIP8_MODE_CH8);
            memset(machine->screen, 0, sizeof(machine->screen));
            machine->screen_update_status = 1;
            machine->pc += 2;
            break;
        case 0xee:
            print_opt("RET", "Return from subroutine", none, CHIP8_MODE_CH8);
            machine->pc = machine->stack[--(machine->sp)];
            break;
        case 0xfb:
            print_opt("SCRR", "Scroll right by 4 pixels", none, CHIP8_MODE_SC8);
            CHIP8_screen_scroll(4, CHIP8_SCROLL_RIGHT);
            machine->screen_update_status = 1;
            machine->pc += 2;
            break;
        case 0xfc:
            print_opt("SCRL", "Scroll left by 4 pixels", none, CHIP8_MODE_SC8);
            CHIP8_screen_scroll(4, CHIP8_SCROLL_LEFT);
            machine->screen_update_status = 1;
            machine->pc += 2;
            break;
        case 0xfe:
            print_opt("NSUPER", "Disable extended mode", none, CHIP8_MODE_SC8);
            machine->screen_width = CHIP8_SCREEN_WIDTH;
            machine->screen_height = CHIP8_SCREEN_HEIGHT;
            machine->screen_is_hires = 0;
            machine->screen_update_status = 1;
            machine->pc += 2;
            break;
        case 0xfd:
            print_opt("EXIT", "Exit the program", none, CHIP8_MODE_SC8);
            return 2;
            break;
        case 0xff:
            print_opt("SUPER", "Enable extended mode", none, CHIP8_MODE_SC8);
            machine->screen_width = CHIP8_SCREEN_WIDTH_HIRES;
            machine->screen_height = CHIP8_SCREEN_HEIGHT_HIRES;
            machine->screen_is_hires = 1;
            machine->screen_update_status = 1;
            machine->pc += 2;
            break;
        default:
            goto invalid_optcode;
        }
        break;
    case 0x1000:
        print_opt("JP", "Jump to location nnn", nnn, CHIP8_MODE_CH8);
        machine->pc = nnn;
        break;
    case 0x2000:
        print_opt("CALL", "Call subroutine at nnn", nnn, CHIP8_MODE_CH8);
        machine->stack[(machine->sp)++] = machine->pc + 2;
        machine->pc = nnn;
        break;
    case 0x3000:
        print_opt("SE", "Skip next instruction if Vx = kk", xkk,
                  CHIP8_MODE_CH8);
        if (machine->reg[x] == kk)
            if (MEM_GET_WORD(machine->pc + 2) == 0xf000)
                machine->pc += 6;
            else
                machine->pc += 4;
        else
            machine->pc += 2;
        break;
    case 0x4000:
        print_opt("SNE", "Skip next instruction if Vx != kk", xkk,
                  CHIP8_MODE_CH8);
        if (machine->reg[x] != kk)
            if (MEM_GET_WORD(machine->pc + 2) == 0xf000)
                machine->pc += 6;
            else
                machine->pc += 4;
        else
            machine->pc += 2;
        break;
    case 0x5000:
        switch (n) {
        case 0:
            print_opt("SER", "Skip next instruction if Vx = Vy", xy,
                      CHIP8_MODE_CH8);
            if (machine->reg[x] == machine->reg[y])
                if (MEM_GET_WORD(machine->pc + 2) == 0xf000)
                    machine->pc += 6;
                else
                    machine->pc += 4;
            else
                machine->pc += 2;
            break;
        case 2:
            print_opt("SAVER", "Save an inclusive range of registers to memory",
                      xy, CHIP8_MODE_XC8);
            for (uint8_t i = x; i < y; i++)
                machine->mem[machine->index_reg + i] = machine->reg[i];
            machine->pc += 2;
            break;
        case 3:
            print_opt("LOADR",
                      "Load an inclusive range of registers from memory", xy,
                      CHIP8_MODE_XC8);
            for (uint8_t i = x; i < y; i++)
                machine->reg[i] = machine->mem[machine->index_reg + i];
            machine->pc += 2;
            break;
        default:
            goto invalid_optcode;
        }
        break;
    case 0x6000:
        print_opt("LD", "Set Vx = kk", xkk, CHIP8_MODE_CH8);
        machine->reg[x] = kk;
        machine->pc += 2;
        break;
    case 0x7000:
        print_opt("ADD", "Set Vx = Vx + kk", xkk, CHIP8_MODE_CH8);
        machine->reg[x] += kk;
        machine->pc += 2;
        break;
    case 0x8000:
        switch (n) {
        case 0x00:
            print_opt("LDR", "Set Vx = Vy", xy, CHIP8_MODE_CH8);
            machine->reg[x] = machine->reg[y];
            break;
        case 0x01:
            print_opt("OR", "Set Vx = Vx OR Vy", xy, CHIP8_MODE_CH8);
            machine->reg[x] |= machine->reg[y];
            break;
        case 0x02:
            print_opt("AND", "Set Vx = Vx AND Vy", xy, CHIP8_MODE_CH8);
            machine->reg[x] &= machine->reg[y];
            break;
        case 0x03:
            print_opt("XOR", "Set Vx = Vx XOR Vy", xy, CHIP8_MODE_CH8);
            machine->reg[x] ^= machine->reg[y];
            break;
        case 0x04:
            print_opt("ADDR", "Set Vx = Vx + Vy. Set VF on carry", xy,
                      CHIP8_MODE_CH8);
            machine->reg[0xf] =
                ((uint32_t)machine->reg[x] + (uint32_t)machine->reg[y]) > 255
                    ? 1
                    : 0;
            machine->reg[x] += machine->reg[y];
            break;
        case 0x05:
            print_opt("SUBY", "Set Vx = Vx - Vy. Set VF if Vy > Vx", xy,
                      CHIP8_MODE_CH8);
            machine->reg[0xf] = machine->reg[y] > machine->reg[x] ? 1 : 0;
            machine->reg[x] = machine->reg[x] - machine->reg[y];
            break;
        case 0x06:
            print_opt("SHR", "Set Vx = Vx >> 1. Store rightmost bit in VF", x,
                      CHIP8_MODE_CH8);
            machine->reg[0xf] = machine->reg[x] & 0x1;
            machine->reg[x] >>= 1;
            break;
        case 0x07:
            print_opt("SUBX", "Set Vx = Vx - Vy. Set VF if Vx > Vy", xy,
                      CHIP8_MODE_CH8);
            machine->reg[0xf] = machine->reg[x] > machine->reg[y] ? 1 : 0;
            machine->reg[x] = machine->reg[y] - machine->reg[x];
            break;
        case 0x0e:
            print_opt("SHL", "Set Vx = Vx << 1. Store leftmost bit in VF", x,
                      CHIP8_MODE_CH8);
            machine->reg[0x0f] = (machine->reg[x] >> 7) & 0x1;
            machine->reg[x] <<= 1;
            break;
        default:
            goto invalid_optcode;
        }
        machine->pc += 2;
        break;
    case 0x9000:
        if (n == 0) {
            print_opt("SKRNE", "Skip next instruction if Vx != Vy", xy,
                      CHIP8_MODE_CH8);
            if (machine->reg[x] != machine->reg[y])
                if (MEM_GET_WORD(machine->pc + 2) == 0xf000)
                    machine->pc += 6;
                else
                    machine->pc += 4;
            else
                machine->pc += 2;
        } else {
            goto invalid_optcode;
        }
        break;
    case 0xa000:
        print_opt("LDI", "Set I = nnn", nnn, CHIP8_MODE_CH8);
        machine->index_reg = nnn;
        machine->pc += 2;
        break;
    case 0xb000:
        print_opt("JPR", "Jump to location nnn + V0", nnn, CHIP8_MODE_CH8);
        machine->pc = machine->reg[0x0] + nnn;
        break;
    case 0xc000:
        print_opt("RND", "Set Vx = <random byte> AND kk", xkk, CHIP8_MODE_CH8);
        machine->reg[x] = CHIP8_get_rand() & kk;
        machine->pc += 2;
        break;
    case 0xd000:
        if (n == 0) {
            print_opt("DRAW HI",
                      "Draw 16x16 sprite starting at I"
                      "(Vx, Vy), set VF = collision",
                      xyn, CHIP8_MODE_SC8);
            CHIP8_screen_draw(x, y, 0);
            machine->screen_update_status = 1;
        } else {
            print_opt("DRAW",
                      "Draw 8xn sprite starting at I"
                      "(Vx, Vy), set VF = collision",
                      xyn, CHIP8_MODE_CH8);
            CHIP8_screen_draw(x, y, n);
            machine->screen_update_status = 1;
        }
        machine->pc += 2;
        break;
    case 0xe000:
        switch (kk) {
        case 0x9e:
            print_opt("SKP",
                      "Skip next instruction if key of value Vx is pressed", x,
                      CHIP8_MODE_CH8);
            if (machine->keys[machine->reg[x]] == CHIP8_KEY_PRESSED)
                if (MEM_GET_WORD(machine->pc + 2) == 0xf000)
                    machine->pc += 6;
                else
                    machine->pc += 4;
            else
                machine->pc += 2;
            break;
        case 0xa1:
            print_opt("SKNP",
                      "Skip next instruction if key of value Vx is released", x,
                      CHIP8_MODE_CH8);
            if (machine->keys[machine->reg[x]] == CHIP8_KEY_RELEASED)
                if (MEM_GET_WORD(machine->pc + 1) == 0xf000)
                    machine->pc += 8;
                else
                    machine->pc += 4;
            else
                machine->pc += 2;
            break;
        }
        break;
    case 0xf000:
        if (nnn == 0) {
            print_opt("LDI EXT", "Set I to 16bit address", none,
                      CHIP8_MODE_XH8);
            machine->index_reg = MEM_GET_WORD(machine->pc + 2);
            machine->pc += 4;
            break;
        } else if (kk == 1) {
            print_opt("BITPLANE", "Set the bitplane to the value of x", x,
                      CHIP8_MODE_XC8);
            machine->screen_bitplane = x;
            machine->pc += 2;
            break;
        }
        switch (kk) {
        case 0x02:
            print_opt("AUDIO STORE", "Store 16 bytes, starting at I, in the audio buffer", none, CHIP8_MODE_XC8);
            print_warn("%s", "Not implemented");
            machine->pc += 2;
            break;
        case 0x07:
            print_opt("LDT", "Set Vx = <delay timer value>", x, CHIP8_MODE_CH8);
            machine->reg[x] = machine->timer;
            machine->pc += 2;
            break;
        case 0x0A:
            print_opt("LDK", "Wait for a keypress. Store its value in Vx", x,
                      CHIP8_MODE_CH8);
            for (uint8_t i = 0; i < CHIP8_KEYS; i++) {
                if (machine->keys[i] == CHIP8_KEY_PRESSED) {
                    CHIP8_input_set(i, CHIP8_KEY_RELEASED);
                    machine->reg[x] = i;
                    machine->pc += 2;
                }
            }
            break;
        case 0x15:
            print_opt("LDDT", "Set <delay timer> = Vx", x, CHIP8_MODE_CH8);
            machine->timer = machine->reg[x];
            machine->pc += 2;
            break;
        case 0x18:
            print_opt("LDS", "Set <sound timer> = Vx", x, CHIP8_MODE_CH8);
            machine->timer_sound = machine->reg[x];
            machine->pc += 2;
            break;
        case 0x1e:
            print_opt("ADDI", "Set I = I + Vx", x, CHIP8_MODE_CH8);
            machine->index_reg += machine->reg[x];
            machine->pc += 2;
            break;
        case 0x29:
            print_opt("LDF", "Set I = <location of font-sprite in Vx>", x,
                      CHIP8_MODE_CH8);
            machine->index_reg = CHIP8_FONTSET_OFFSET +
                                 CHIP8_FONTSET_CHAR_SIZE * machine->reg[x];
            machine->pc += 2;
            break;
        case 0x30:
            print_opt("LDF HIRES",
                      "Set I = <location of hires font-sprite in Vx>", x,
                      CHIP8_MODE_SC8);
            machine->index_reg =
                CHIP8_FONTSET_OFFSET_SUPER +
                CHIP8_FONTSET_CHAR_SIZE_SUPER * machine->reg[x];
            machine->pc += 2;
            break;
        case 0x33:
            print_opt("BCD",
                      "Store BCD repesentation of Vx in memory "
                      "locations I, I+1, I+2",
                      x, CHIP8_MODE_CH8);
            machine->mem[machine->index_reg] = (machine->reg[x] % 1000) / 100;
            machine->mem[machine->index_reg + 1] = (machine->reg[x] % 100) / 10;
            machine->mem[machine->index_reg + 2] = (machine->reg[x] % 10) / 1;
            machine->pc += 2;
            break;
        case 0x3a:
            print_opt("PITCH", "Set the audio pattern playback rate to 4000*2^((Vx-64)/48)Hz", x, CHIP8_MODE_XC8);
            print_warn("%s", "Not implemented");
            machine->pc += 2;
            break;
        case 0x55:
            print_opt("STORE",
                      "Store registers V0 through Vx into adress I to I + x", x,
                      CHIP8_MODE_CH8);
            for (int i = 0; i < x + 1; i++)
                machine->mem[machine->index_reg + i] = machine->reg[i];
            machine->pc += 2;
            break;
        case 0x65:
            print_opt("READ",
                      "Read registers V0 through Vx from memory "
                      "starting at I",
                      x, CHIP8_MODE_CH8);
            for (int i = 0; i < x + 1; i++)
                machine->reg[i] = machine->mem[machine->index_reg + i];
            machine->pc += 2;
            break;
        case 0x75:
            print_opt("STOREF", "Read V0 to Vx into flag registers (0-7)", x,
                      CHIP8_MODE_SC8);
            log_warn("%s", "Not implemented");
            machine->pc += 2;
            break;
        case 0x85:
            print_opt("READF", "Restore V0 to Vx from flag registers (0-7)", x,
                      CHIP8_MODE_SC8);
            log_warn("%s", "Not implemented");
            machine->pc += 2;
            break;
        default:
            goto invalid_optcode;
        }
        break;
    default:
        goto invalid_optcode;
    }

    return 0;

invalid_optcode:
    print_error("Invalid Optcode: 0x%04x [PC=0x%04x]", optcode, machine->pc);

    return -1;
}
