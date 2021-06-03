# [WIP] 2048.c | Yet another 2048 as a term project

[简体中文](README_CN.md)

This is a <ruby><rb>little game</rb><rt>reinvented wheel</rt></ruby> with pure C as the term project for the course Basics of Programming and Application of my group. 

**⚠IMPORTANT: THIS PROJECT IS STILL UNDER DEVELOPMENT. Variations for different platforms contain large amounts of dreadful platform-exclusive features. The stability and safety of any code in master branch is not guaranteed.**

## Console ver. ( `2048.c` and `2048ForVS.c` )

`2048.c` is the main working file of this project, which has been tested on Ubuntu 20.04 focal with clang version 10, build and run it in a terminal and you should be able to see what has been achieved. `2048ForVS.c` (if any) is made to be compatible with Visual C++ 2008 and above.

### Gameplay:

Move numbers with WASD or arrow keys. Press `q` to quit, `u` to undo and `r` to restart.

### Features:

- [X] Basic game logic
- [X] Undo
- [X] Colorize the background of the numbers
- [ ] Adjust the size of the blocks
- [X] Store game records
  - [X] Display sorted records
- [X] GUI

## GUI ver. 

`GUI-Ver` is the working directory of GUI ver. It renders with modified Nuklear and GDI, which is tested on Windows with both MinGW-w64 and Visual C++ 2010 Express. In theory it supports Windows only, but you may easily port it to other desktop environments.

### Compilation guide

#### For Visual C++ 2010 Express (might also for other Visual C++)

- Set Linker -> System -> Subsystem to `Console (/SUBSYSTEM:CONSOLE)`
- Add `msimg32.lib` to Linker -> Input -> Additional Dependencies

#### For MinGW-w64

- Add the parameters when compiling: `-lm -luser32 -lgdi32 -lmsimg32`

## Credit

[2048.c by mevdschee](https://github.com/mevdschee/2048.c) for the algorithmic ideas and practice of non-canonical terminal input

[Nuklear by Immediate Mode UI](https://github.com/immediate-mode-ui/Nuklear/)

Anyone offering me advice

## License

[Anti 996 License](LICENSE)