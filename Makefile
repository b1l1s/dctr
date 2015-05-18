export PATH	:=	$(DEVKITARM)/bin:$(PATH)

CC=arm-none-eabi-gcc
CP=arm-none-eabi-g++
OC=arm-none-eabi-objcopy 
LD=arm-none-eabi-ld
AM=armips
ROP=python lib/p3ds/gspwn-rop.py
MV=mv -f
RM=rm -rf

LIBNAME=dctr
ELFNAME=$(LIBNAME).elf
BINNAME=$(LIBNAME).bin

SRC_DIR:=src/$(LIBNAME)
OBJ_DIR:=obj/$(LIBNAME)
LIB_DIR:=lib
DEP_DIR:=obj/$(LIBNAME)

LIBS=-lctrff -lctr
LIBPATHS=-L../ctrff/lib -L../ctr/lib
INCPATHS=-I../ctrff/include -I../ctr/include

CFLAGS=-std=gnu99 -Os -g -mword-relocations -fomit-frame-pointer -ffast-math $(INCPATHS)
C9FLAGS=-mcpu=arm946e-s -march=armv5te -mlittle-endian
LDFLAGS=$(LIBS) $(LIBPATHS)
OCFLAGS=--set-section-flags .bss=alloc,load,contents

OBJS:=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
OBJS+=$(patsubst $(SRC_DIR)/%.s, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.s))
OBJS+=$(patsubst $(SRC_DIR)/%.S, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.S))

LAUNCHERBINS=bin/arm9hax.bin bin/arm11hax.bin

OUT_DIR=bin obj/dctr patch

.PHONY: clean

all: Launcher.dat

Launcher.dat: $(LAUNCHERBINS)
	$(ROP) bin/arm11hax.bin $@
	encrypt.bat
	$(MV) Launcher_enc.dat $@

bin/dctr.bin: $(OBJS) ../ctrff/lib/libctrff.a ../ctr/lib/libctr.a | dirs
	$(CC) -nostartfiles --specs=$(LIBNAME).specs $(OBJS) $(LDFLAGS) -o bin/$(ELFNAME)
	$(OC) $(OCFLAGS) -O binary bin/$(ELFNAME) bin/$(BINNAME)

bin/arm11hax.bin: src/launcher/arm11hax.s bin/arm9hax.bin bin/$(BINNAME) | dirs
	$(AM) $<

bin/arm9hax.bin: src/launcher/arm9hax.s | dirs
	$(AM) $<

obj/%.o: src/%.c | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C9FLAGS) $< -o $@

obj/%.o: src/%.s | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C9FLAGS) $< -o $@

obj/%.o: src/%.S | dirs
	@echo Compiling $<
	$(CC) -c $(CFLAGS) $(C9FLAGS) $< -o $@

dirs: ${OUT_DIR}

${OUT_DIR}:
	mkdir -p ${OUT_DIR}

clean:
	rm -rf bin/*.elf bin/*.bin obj/* Launcher.dat
