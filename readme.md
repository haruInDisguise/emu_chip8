# CHIP8 Interpreter

This is a simple CHIP8 interpreter that tries to aim for consistent, readable and modular code (<insert other flattering adjectives here>).
It's my first "larger" Project in C, and therefore will certainly leave some things to be desired.

What is (partially) supported:

- CHIP-8
- SUPER-CHIP
- XO-CHIP

## Implemented

- Drawing Bitplanes (missing render backend)
- Scrolling the screen
- High resolution (128 x 64)
- Keyboard input

## Not implemented

- Audio
- 64k address space
- Scrolling bitplanes
- HP48 Flags registers (optcodes print a warning)

## Requires

- An accessible [SDL2](https://www.libsdl.org/) installation
- Roms can be found... online or [here](https://github.com/kripod/chip8-roms) and (here)[https://github.com/JohnEarnest/Octo/tree/gh-pages/examples]

## References and Resources

- [Guide to making a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#add-super-chip-support): A really well written guide, that aims to explain architecture, rather then code.
- [Cowgod's CHIP8 Technical Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#8xy6)
- [SCHIP Reference](http://devernay.free.fr/hacks/chip8/schip.txt)
- [XO Reference](http://johnearnest.github.io/Octo/docs/XO-ChipSpecification.html]: A technical reference for the XO Extension set, which is used by the [Octo](https://github.com/JohnEarnest/Octo)
