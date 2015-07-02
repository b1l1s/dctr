export PATH	:=	$(DEVKITARM)/bin:$(PATH)

CC=arm-none-eabi-gcc
CP=arm-none-eabi-g++
OC=arm-none-eabi-objcopy 
LD=arm-none-eabi-ld
MV=mv -f
RM=rm -rf

CTR_DIR=../ctr
CTRFF_DIR=../ctrff

LIBNAME=dctr
ELFNAME=$(LIBNAME).elf
BINNAME=$(LIBNAME).bin
DATNAME=$(LIBNAME).dat

SRC_DIR:=src/$(LIBNAME)
OBJ_DIR:=obj/$(LIBNAME)
LIB_DIR:=lib
DEP_DIR:=obj/$(LIBNAME)

LIBS=-lctrff -lctr
LIBPATHS=-L$(CTRFF_DIR)/lib -L$(CTR_DIR)/lib
INCPATHS=-I$(CTRFF_DIR)/include -I$(CTR_DIR)/include

CFLAGS=-std=gnu99 -Os -g -mword-relocations -fomit-frame-pointer -ffast-math $(INCPATHS)
C9FLAGS=-mcpu=arm946e-s -march=armv5te -mlittle-endian
LDFLAGS=$(LIBS) $(LIBPATHS)
OCFLAGS=--set-section-flags .bss=alloc,load,contents

OBJS:=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.c))
OBJS+=$(patsubst $(SRC_DIR)/%.s, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.s))
OBJS+=$(patsubst $(SRC_DIR)/%.S, $(OBJ_DIR)/%.o, $(wildcard $(SRC_DIR)/*.S))

OUT_DIR=bin out obj/dctr

.PHONY: clean

all: out/$(DATNAME)

out/$(DATNAME): bin/$(BINNAME)
	make dir_out=../out name=$(DATNAME) -C CakeHax launcher
	dd if=bin/$(BINNAME) of=out/$(DATNAME) bs=512 seek=256

bin/dctr.bin: $(OBJS) $(CTRFF_DIR)/lib/libctrff.a $(CTR_DIR)/lib/libctr.a | dirs
	$(CC) -nostartfiles --specs=$(LIBNAME).specs $(OBJS) $(LDFLAGS) -o bin/$(ELFNAME)
	$(OC) $(OCFLAGS) -O binary bin/$(ELFNAME) bin/$(BINNAME)

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
	rm -rf bin/*.elf bin/*.bin obj/*
	make dir_out=../out name=dctr.dat -C CakeHax clean
