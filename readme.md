# CHIP8 Emulator

This is a simple CHIP8 interpreter/emulator that focuses on the CHIP-XO extension-set, used by the [Octo assembler/interpreter](https://github.com/JohnEarnest/Octo).

It's my first "larger" Project in C, and therefore will certainly leave some things to be desired. Feedback is always welcome!

### Features and TODO

- [x] Support for 4bit bitmaps (for a total of 16 different colors), extending upon Octo (However, the standart [does mention this addition](https://github.com/JohnEarnest/Octo/blob/gh-pages/docs/XO-ChipSpecification.md#Bitplanes))
- [x] Keyboard input
- [x] Extended memory (64kb)
- [x] Scrolling (also supports indivdual bitmaps)

- [ ] Basic commandline options (i.e. speed, custom font, colorscheme...)
- [ ] Audio
- [ ] HP48 Flag registers (optcode prints a warning)
- [ ] Add a custom assembler with support for 4bit bitmaps (veeery TODO)

## How to build, run and customize
What is required:

- An accessible [SDL2](https://www.libsdl.org/) installation
- Roms can be found... online or [here](https://github.com/kripod/chip8-roms) and [here](https://github.com/JohnEarnest/Octo/tree/gh-pages/examples)
- This code should work just fine on Windows, however it has only been tested on an Arch Linux installation.

You can set a log level by exporting/setting the `LOG_LEVEL` ENV. Possible values are: `all, debug, info, warn, error, none`. Defaults to `all`.

The interpreter prints each executed instruction and relevant register values to the terminal (using the debug log level). A slow terminal might hinder program execution.

## References and Resources

- [Guide to making a CHIP-8 emulator](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#add-super-chip-support): A really well written guide, that aims to explain architecture, rather then code.
- [CHIP8 Reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#8xy6)
- [Super-CHIP Reference](http://devernay.free.fr/hacks/chip8/schip.txt)
- [CHIP-XO Reference](http://johnearnest.github.io/Octo/docs/XO-ChipSpecification.html): A technical reference for the XO Extension set, which is used by the [Octo](https://github.com/JohnEarnest/Octo)
