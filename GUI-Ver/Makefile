CC := gcc
LINKER_FLAG = -lm -luser32 -lgdi32
RELEASE_LINKER_FLAG = $(LINKER_FLAG) -O3 -Wall -Wextra
BIN = bin\2048.exe

all: clean generate
ifeq ($(OS),Windows_NT)
	@echo Generating bin file and trying to run it...
	${CC} 2048.c -o $(BIN) $(LINKER_FLAG)
endif

release: clean generate
ifeq ($(OS),Windows_NT)
	@echo Generating release...
	${CC} 2048.c -o $(BIN) $(RELEASE_LINKER_FLAG)
endif

clean:
ifeq ($(OS),Windows_NT)
	@echo Removing previously generated files...
	@if exist bin rmdir bin /s /q
endif

generate:
ifeq ($(OS),Windows_NT)
	@echo Creating bin folder...
	@mkdir bin 2> nul || exit 0
endif
