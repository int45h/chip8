# chip8
A small CHIP-8 emulator I wrote in C++ and SDL.
![Alt text](chip8.png?raw=true "CHIP-8 Emu Screenshot")

Features
------------
- Support for most CHIP-8 roms
- Hardware Accelerated 2D rendering ala SDL
- Keyboard-layout agnostic input (ala SDL Scancodes)

Input
------------
The CHIP-8 used a hexadecimal keypad for input, which is mapped to the sets of keys listed below:    
    
    1 | 2 | 3 | C       1 | 2 | 3 | 4
    --+---+---+--       --+---+---+--
    4 | 5 | 6 | D       Q | W | E | R
    --+---+---+--  -->  --+---+---+--
    7 | 8 | 9 | E       A | S | D | F
    --+---+---+--       --+---+---+--
    A | 0 | B | F       Z | X | C | V

NOTE that this is shown with the QWERTY keyboard layout, but will still map the keys to their corresponding positions on a different keyboard layout.

Building
------------
To build, simply enter the 'build' directory and run the command 'make'. 

Dependencies
------------
- SDL 2.0
- Portaudio (Subject for removal in later versions)
- g++ (Compiler)

TO-DO
------------

- Add Super CHIP 48 instructions
- Add audio support
- Add resizable window support
- Add built-in debugger + disassembler
- Add instructions on how to play commonly used ROMS
- Build for Windows + macOS
