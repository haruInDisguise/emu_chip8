# CHIP8 Interpreter

This is a simple CHIP8 interpreter that aims for consistent, readable and easily hackable code (<insert other flattering adjectives here>). This project focuses on the (CHIP-XO extension-set), used by the [Octo assembler/interpreter](https://github.com/JohnEarnest/Octo).
It's my first "larger" Project in C, and therefore will certainly leave some things to be desired.

## Implemented

- Support for 4bit bitmaps (for a total of 16 different colors), extending upon Octo, which currently only supports 2bit bitmaps (However, the standart [does mention this addition](https://github.com/JohnEarnest/Octo/blob/gh-pages/docs/XO-ChipSpecification.md#Bitplanes))
- Scrolling the screen (and scrolling individual bitmaps)
- Keyboard input
- Extended memory (64kb)
- High resolution (128 x 64)

## TODO

- Audio
- HP48 Flag registers (optcodes print a warning)

## Requires

- An accessible [SDL2](https://www.libsdl.org/) installation
- Roms can be found... online or [here](https://github.com/kripod/chip8-roms) and (here)[https://github.com/JohnEarnest/Octo/tree/gh-pages/examples]

# How to build, run and customize
Just run `make`. Note that [clang](https://clang.llvm.org/) is used as the default compiler.

Invoke it by typing `./build/emu_chip8 <path to rom here>`. You can set a log level by exporting/setting the `LOG_LEVEL` ENV. Possible values are: `all, debug, info, warn, error, none`. It defaults to `debug`.

The entry point is `src/emu_chip8.c`. It runs the main loop and times all calls to the CHIP8/render backend.
Make used debug mode flags by default.

## References and Resources

- [Guide to making a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#add-super-chip-support): A really well written guide, that aims to explain architecture, rather then code.
- [Cowgod's CHIP8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#8xy6)
- [SCHIP Reference](http://devernay.free.fr/hacks/chip8/schip.txt)
- [XO Reference](http://johnearnest.github.io/Octo/docs/XO-ChipSpecification.html]: A technical reference for the XO Extension set, which is used by the (Octo)[https://github.com/JohnEarnest/Octo]
