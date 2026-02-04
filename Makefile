CFLAGS+= -DFXCG50 -O2
LDFLAGS+=-lc -lgint-cg -lc -lgcc -lm -lgint-fxg3a -nostdlib -m4-nofpu -mb -T fxcg50.ld

CFLAGS+=-Wall -g -O2
CELESTE_CC=$(CC)

OUT=ccleste-gint.elf
CELESTE_OBJ=celeste-gint.o
CELESTE_OBJ_DEPS=assets-font.o assets-gfx.o
CFLAGS+=-DCELESTE_P8_FIXEDP
CELESTE_CC=sh-elf-g++
CC=sh-elf-gcc
MAIN=gintmain.c

all: $(OUT)

$(OUT): $(MAIN) $(CELESTE_OBJ) $(CELESTE_OBJ_DEPS) celeste.h sdl20compat.inc.c
	$(CC) $(CFLAGS) $(MAIN) $(CELESTE_OBJ) $(CELESTE_OBJ_DEPS) -o $(OUT) $(LDFLAGS)

$(CELESTE_OBJ): celeste.c celeste.h
	$(CELESTE_CC) $(CFLAGS) -c -o $(CELESTE_OBJ) celeste.c

assets-%.o: data/%.bmp
	fxconv --bopti-image --toolchain=sh-elf --cg $< -o $@ name:assets_$* profile:p4

celeste.g3a: $(OUT)
	sh-elf-objcopy -O binary -R .bss -R .gint_bss $(OUT) $(OUT:.elf=.bin)
	fxgxa --g3a -n "Celeste" --icon-uns "data/icon-cg-uns.png" --icon-sel "data/icon-cg-sel.png" -o $@ $(OUT:.elf=.bin)

clean:
	$(RM) *.g3a *.o *.elf *.bin
