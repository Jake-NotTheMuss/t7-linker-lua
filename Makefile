# Makefile for linker_modtools Lua mod

CC= x86_64-w64-mingw32-gcc

bindir="$${TA_TOOLS_PATH}bin"
csv=$(bindir)/ta_tools_config.csv

LINKER_LUA_CFLAGS=-ansi -Wall -Wpedantic -ffreestanding -O2 -Ihksc/src

all: linker_lua.dll

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

clean:
	rm -f linker_lua.dll

distclean: clean
	rm -rf hksc

install:
	egrep -q '[,[:space:]]linker_lua\.dll($$|[,[:space:]])' $(csv) || \
	echo 'linker,linker_lua.dll' >> $(csv)
	cp -pf linker_lua.dll $(bindir)

uninstall:
	sed -i.bk -e '/[,[:space:]]linker_lua\.dll\($$\|[,[:space:]]\)/d' $(csv)
	rm -f $(bindir)/linker_lua.dll $(csv).bk

.PHONY: all clean distclean install uninstall
