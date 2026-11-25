#
# PocketSNES for the RetroFW
#
# by pingflood; 2019
#

# Define the applications properties here:

TARGET = pocketsnes/pocketsnes.dge

CC  ?= mipsel-linux-gcc
CXX ?= mipsel-linux-g++
STRIP ?= mipsel-linux-strip

SYSROOT := $(shell $(CC) --print-sysroot)
SDL_CFLAGS := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

INCLUDE = -I src \
		-I sal/linux/include -I sal/include \
		-I src/include \
		-I menu -I src/linux -I src/snes9x \
		-I /home/sean/retrofw-aux/include -I /home/sean/retrofw-aux/lib

CFLAGS =  $(INCLUDE) -DRC_OPTIMIZED -DGCW_ZERO -D__LINUX__ -D__DINGUX__ -DFOREVER_16_BIT -DFOREVER_16_BIT_SOUND -DFOREVER_FORWARD_STEREO -DNO_VOLATILE_SOUND $(SDL_CFLAGS)
CFLAGS += -O3 -fdata-sections -ffunction-sections -fomit-frame-pointer -fno-builtin -fwrapv -fno-strict-aliasing
CFLAGS += -mips32 -march=mips32 -mhard-float -mno-mips16 -mplt -mno-shared -DMIPS_XBURST
CFLAGS += -fno-common -Wno-write-strings -Wno-sign-compare -ffast-math -ftree-vectorize -funswitch-loops
CFLAGS += -DFAST_LSB_WORD_ACCESS
CFLAGS += -fmerge-all-constants
ifeq ($(PGO),generate)
CFLAGS += -fprofile-generate -fprofile-dir=/tmp/profile
endif
ifeq ($(PGO),use)
CFLAGS += -fprofile-use -fprofile-dir=./profile -fprofile-correction -Wno-missing-profile -Wno-coverage-mismatch
endif

CXXFLAGS = -std=gnu++11 $(CFLAGS) -fpermissive -fno-exceptions -fno-rtti -fno-math-errno -fno-threadsafe-statics

LDFLAGS = $(CXXFLAGS) /home/sean/retrofw-aux/lib/libjzaux.a -lpthread -lz -lpng $(SDL_LIBS) -lSDL_image -Wl,--as-needed -Wl,--gc-sections -s

# Find all source files
SOURCE = src/snes9x menu sal/linux sal
SRC_CPP = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.cpp))
SRC_C   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.c))
SRC_S   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.S))
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJ_S   = $(patsubst %.S, %.o, $(SRC_S))
OBJS    = $(OBJ_CPP) $(OBJ_C) $(OBJ_S)

.PHONY : all
all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	$(STRIP) $(TARGET)

ipk: all
	@rm -rf /tmp/.pocketsnes-ipk/ && mkdir -p /tmp/.pocketsnes-ipk/root/home/retrofw/emus/pocketsnes /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
	@cp dist/PocketSNES.dge dist/PocketSNES.man.txt dist/PocketSNES.png dist/backdrop.png /tmp/.pocketsnes-ipk/root/home/retrofw/emus/pocketsnes
	@cp dist/pocketsnes.lnk /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators
	@cp dist/snes.pocketsnes.lnk /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
	# @sed "s/^Version:.*/Version: $$(date +%Y%m%d)/" dist/control > /tmp/.pocketsnes-ipk/control
	@sed "s/^Version:.*/Version: 20190304/" dist/control > /tmp/.pocketsnes-ipk/control
	@cp dist/conffiles /tmp/.pocketsnes-ipk/
	# echo -e "#!/bin/sh\nmkdir -p /home/retrofw/profile/pocketsnes; exit 0" > /tmp/.pocketsnes-ipk/preinst
	# chmod +x /tmp/.gmenu-ipk/postinst /tmp/.pocketsnes-ipk/preinst
	@tar --owner=0 --group=0 -czvf /tmp/.pocketsnes-ipk/control.tar.gz -C /tmp/.pocketsnes-ipk/ control conffiles #preinst
	@tar --owner=0 --group=0 -czvf /tmp/.pocketsnes-ipk/data.tar.gz -C /tmp/.pocketsnes-ipk/root/ .
	@echo 2.0 > /tmp/.pocketsnes-ipk/debian-binary
	@ar r dist/pocketsnes.ipk /tmp/.pocketsnes-ipk/control.tar.gz /tmp/.pocketsnes-ipk/data.tar.gz /tmp/.pocketsnes-ipk/debian-binary

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY : clean

opk: all
	@mksquashfs \
	pocketsnes/default.retrofw.desktop \
	pocketsnes/snes.retrofw.desktop \
	pocketsnes/pocketsnes.dge \
	pocketsnes/pocketsnes.man.txt \
	pocketsnes/pocketsnes.png \
	pocketsnes/backdrop.png \
	pocketsnes/pocketsnes.opk \
	-comp gzip -all-root -noappend -no-exports -no-xattrs

clean :
	rm -f $(OBJS) pocketsnes/pocketsnes.ipk
