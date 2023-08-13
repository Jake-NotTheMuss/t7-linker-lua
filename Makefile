

CC= x86_64-w64-mingw32-gcc

all: linker_lua.dll RemoteLogger.dll

LINKER_LUA_CFLAGS=-ansi -Wall -Wpedantic -ffreestanding -O2

linker_lua.dll: linker_lua.c linker_symbols.def libhksc.a
	$(CC) $(LINKER_LUA_CFLAGS) -save-temps -o $@ -shared linker_lua.c libhksc.a

RemoteLogger.dll: RemoteLogger.c RemoteLogger.h
	$(CC) -ffreestanding -nolibc -o $@ -shared RemoteLogger.c

clean:
	rm linker_lua.dll RemoteLogger.dll

install:
	cp -f linker_lua.dll RemoteLogger.dll "$${TA_GAME_PATH}bin"

.PHONY: all clean install
