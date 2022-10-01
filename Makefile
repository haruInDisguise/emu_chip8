CC = clang

CFLAGS = -g -O2
CFLAGS_DEBUG = -ggdb -O0 -Wpedantic -Wall -fno-omit-frame-pointer
LDFLAGS = -lSDL2

TARGET = emu_chip8
OUT = build

SRC = src/emu_chip8.c	\
	  src/chip8.c		\
	  src/log.c			\
	  src/backend_sdl.c \

OBJ := $(patsubst %.c,$(OUT)/%.o,$(SRC))

all: build

init:
	git submodule init
	git submodule update

build: $(OBJ)
	@echo "[BUILD] Executing debug build"
	$(CC) $(LDFLAGS) $(OBJ) -o "$(OUT)/$(TARGET)"

clean:
	rm -rf $(OUT)

.PHONY: all run build clean init

# --------------

$(OUT)/%.o: %.c
	mkdir -p ${dir $@}
	$(CC) -c $(CFLAGS_DEBUG) $< -o $@

