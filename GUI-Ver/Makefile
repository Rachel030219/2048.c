CC := gcc
LINKERFLAG = -lm -luser32 -lgdi32 -lmsimg32 -mwindows
BIN = bin\2048.exe

all: clean generate
	@${CC} 2048.c -o ${BIN} ${LINKERFLAG} && ${BIN}

clean:
	@if exist bin rmdir bin /s /q

generate:
	@mkdir bin 2> nul || exit 0
