

CC= x86_64-w64-mingw32-gcc

all: linker_lua.dll RemoteLogger.dll

LINKER_LUA_CFLAGS=-ansi -Wall -Wpedantic -ffreestanding -O2 -Ihksc/src

hksc/src/libhksc.a:
	@rm -rf hksc
	@echo -n 'Fetching libhksc... '
	@git clone https://github.com/Jake-NotTheMuss/hksc >/dev/null 2>&1
	@echo Done
	@echo -n 'Building libhksc (this may take a minute)... '
	@cd hksc && ./configure --game=t7 --release >/dev/null 2>&1
	@make -C hksc >/dev/null 2>&1
	@echo Done

linker_lua.dll: linker_lua.c linker_symbols.def hksc/src/libhksc.a
	$(CC) $(LINKER_LUA_CFLAGS) -save-temps -o $@ -shared linker_lua.c \
	hksc/src/libhksc.a

RemoteLogger.dll: RemoteLogger.c RemoteLogger.h
	$(CC) -ffreestanding -nolibc -o $@ -shared RemoteLogger.c

clean:
	rm -f linker_lua.dll RemoteLogger.dll

distclean: clean
	rm -rf hksc

install:
	cp -f linker_lua.dll RemoteLogger.dll "$${TA_GAME_PATH}bin"

uninstall:
	rm -f "$${TA_GAME_PATH}bin/linker_lua.dll" "$${TA_GAME_PATH}bin/RemoteLogger.dll"

.PHONY: all clean distclean install uninstall
