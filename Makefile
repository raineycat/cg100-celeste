
# set to 1 to use SDL1.2
SDL_VER?=2
# use gint
USE_GINT?=0

ifeq ($(USE_GINT),1)
	CFLAGS+= -DFXCG50
	LDFLAGS+=-lc -lgint-cg -lc -lgcc -nostdlib -T fxcg50.ld
else
ifeq ($(SDL_VER),2)
	CFLAGS+=`sdl2-config --cflags`
	LDFLAGS+=-lSDL2 -lSDL2_mixer
else
ifeq ($(SDL_VER),1)
	CFLAGS+=`sdl-config --cflags`
	LDFLAGS+=-lSDL -lSDL_mixer
else
	CFLAGS+=$(error "invalid SDL version '$(SDL_VER)'. possible values are '1' and '2'")
endif
endif
endif

CFLAGS+=-Wall -g -O2
CELESTE_CC=$(CC)

ifeq ($(USE_GINT),1)
	OUT=ccleste-gint.elf
	CELESTE_OBJ=celeste-gint.o
	CELESTE_OBJ_DEPS=assets-font.o assets-gfx.o
	CFLAGS+=-DCELESTE_P8_FIXEDP
	CELESTE_CC=sh-elf-g++
	CC=sh-elf-gcc
	MAIN=gintmain.c
else
ifneq ($(USE_FIXEDP),)
	OUT=ccleste-fixedp
	CELESTE_OBJ=celeste-fixedp.o
	CFLAGS+=-DCELESTE_P8_FIXEDP
	CELESTE_CC=$(CXX)
	MAIN=sdl12main.c
else
	OUT=ccleste
	CELESTE_OBJ=celeste.o
	LDFLAGS+=-lm
	MAIN=sdl12main.c
endif
endif

ifneq ($(HACKED_BALLOONS),)
	CFLAGS+=-DCELESTE_P8_HACKED_BALLOONS
endif

all: $(OUT)

$(OUT): $(MAIN) $(CELESTE_OBJ) $(CELESTE_OBJ_DEPS) celeste.h sdl20compat.inc.c
	$(CC) $(CFLAGS) $(MAIN) $(CELESTE_OBJ) $(CELESTE_OBJ_DEPS) -o $(OUT) $(LDFLAGS)

$(CELESTE_OBJ): celeste.c celeste.h
	$(CELESTE_CC) $(CFLAGS) -c -o $(CELESTE_OBJ) celeste.c

assets-%.o: data/%.bmp
	fxconv -b --toolchain=sh-elf --cg $< -o $@ custom-type:celeste-bmp name:assets_$* profile:p4

ifeq ($(USE_GINT),1)
celeste.g3a: $(OUT)
	sh-elf-objcopy -O binary -R .bss -R .gint_bss $(OUT) $(OUT:.elf=.bin)
	mkg3a -n "basic:Celeste" -i "uns:icon-cg-uns.png" -i "sel:icon-cg-sel.png" $(OUT:.elf=.bin) $@
endif

clean:
	$(RM) ccleste ccleste-fixedp ccleste-gint celeste.o celeste-fixedp.o celeste-gint.o
	make -f Makefile.3ds clean
