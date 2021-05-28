# [WIP] 2048.c | Yet another 2048 as a term project

[简体中文](README_CN.md)

This is a <ruby><rb>little game</rb><rt>reinvented wheel</rt></ruby> with pure C as the term project for the course Basics of Programming and Application of my group. Currently only terminal version is offered, and the project is only tested working on Ubuntu 20.04 focal, compiled with clang version 10.

`2048.c` is the main working file of this project, build and run it in a terminal and you should be able to see what has been achieved. `2048ForVS.c` (if any) is made to be compatible with Visual C++ 2008 and above.

**⚠IMPORTANT: THIS PROJECT IS STILL UNDER DEVELOPMENT. Variations for different platforms contain large amounts of dreadful platform-exclusive features. The stability and safety of any code in master branch is not guaranteed.**

### Gameplay:

Move numbers with WASD or arrow keys. Press `q` to quit, `u` to undo and `r` to restart.

### Known bugs:

- [x] 224 merging left produces 8 instead of 44 (remains to be validated)

### Features:

- [x] Basic game logic
- [x] Undo
- [x] Colorize the background of the numbers
- [ ] Adjust the size of the blocks
- [ ] Store game records
  - [ ] Display sorted records
- [ ] GUI

## Credit

[2048.c by mevdschee](https://github.com/mevdschee/2048.c) for the algorithmic ideas and practice of non-canonical terminal input

Anyone offering me advice

## License

[Anti 996 License](LICENSE)