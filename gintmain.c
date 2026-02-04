#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/rtc.h>
#include <gint/clock.h>
#include <gint/defs/attributes.h>
#include <limits.h>
#include <stdint.h>

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;

typedef struct {
	int16_t x, y;
	uint16_t w, h;
} SDL_Rect;

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include "celeste.h"

#define SAVE_FILE "\\\\fls0\\celeste_save.bin"

extern bopti_image_t assets_gfx, assets_font;
bopti_image_t *gfx_1, *font_1, *gfx_2, *font_2;
bopti_image_t *gfx, *font;

#define PICO8_W 128
#define PICO8_H 128

static int scale = 1, scale_x = 0, scale_y = 0;

static void set_scale(int s, int y_position_group)
{
	if(s == 1) {
		scale = 1;
		gfx = gfx_1;
		font = font_1;
		scale_x = (DWIDTH - PICO8_W * s) / 2;
		scale_y = (DHEIGHT - PICO8_H * s) / 2;
	}
	else if(s == 2) {
		scale = 2;
		gfx = gfx_2;
		font = font_2;
		scale_x = (DWIDTH - PICO8_W * s) / 2;
		if(y_position_group == 0)
			scale_y = 0;
		else if(y_position_group == 1)
			scale_y = (DHEIGHT - PICO8_H * s) / 2;
		else if(y_position_group == 2)
			scale_y = DHEIGHT - PICO8_H * s;
	}
}

bopti_image_t *resize(bopti_image_t const *src, int w, int h)
{
    bopti_image_t *img = image_alloc(w, h, src->format);
    if(!img) return NULL;

		image_copy_palette(src, img, -1);

    for(int y = 0; y < h; y++)
    for(int x = 0; x < w; x++) {
        int color = image_get_pixel(src, x * src->width / w, y * src->height / h);
        image_set_pixel(img, x, y, color);
    }

    return img;
}


static void rect(int x1, int y1, int x2, int y2, int c)
{
	/* drect() with viewport translation. We clamp manually because drect()
	   won't (there *is* space around the viewport) */
	if(x1 < 0) x1 = 0;
	if(y1 < 0) y1 = 0;
	if(x2 >= PICO8_W * scale) x2 = PICO8_W * scale - 1;
	if(y2 >= PICO8_H * scale) y2 = PICO8_H * scale - 1;
	if(x1 > x2 || y1 > y2) return;
	drect(x1+scale_x, y1+scale_y, x2+scale_x, y2+scale_y, c);
}

static int CONTROL_LEFT, CONTROL_RIGHT, CONTROL_UP, CONTROL_DOWN;
static int CONTROL_JUMP, CONTROL_DASH, CONTROL_PAUSE;

static void load_controls(int setting)
{
	if(setting == 0) {
		CONTROL_LEFT = KEY_LEFT;
		CONTROL_RIGHT = KEY_RIGHT;
		CONTROL_UP = KEY_UP;
		CONTROL_DOWN = KEY_DOWN;
		CONTROL_JUMP = KEY_SHIFT;
		CONTROL_DASH = KEY_XOT;
		CONTROL_PAUSE = KEY_VARS;
	}
	else if(setting == 1) {
		CONTROL_LEFT = KEY_ALPHA;
		CONTROL_RIGHT = KEY_POWER;
		CONTROL_UP = KEY_OPTN;
		CONTROL_DOWN = KEY_SQUARE;
		CONTROL_JUMP = KEY_COS;
		CONTROL_DASH = KEY_TAN;
		CONTROL_PAUSE = KEY_SIN;
	}
	else if(setting == 2) {
		CONTROL_LEFT = KEY_RIGHTP;
		CONTROL_RIGHT = KEY_ARROW;
		CONTROL_UP = KEY_COS;
		CONTROL_DOWN = KEY_COMMA;
		CONTROL_JUMP = KEY_FD;
		CONTROL_DASH = KEY_FRAC;
		CONTROL_PAUSE = KEY_VARS;
	}
}

typedef struct { Uint8 r, g, b, unused; } SDL_Color;

static const SDL_Color base_palette[16] = {
	{0x00, 0x00, 0x00},
	{0x1d, 0x2b, 0x53},
	{0x7e, 0x25, 0x53},
	{0x00, 0x87, 0x51},
	{0xab, 0x52, 0x36},
	{0x5f, 0x57, 0x4f},
	{0xc2, 0xc3, 0xc7},
	{0xff, 0xf1, 0xe8},
	{0xff, 0x00, 0x4d},
	{0xff, 0xa3, 0x00},
	{0xff, 0xec, 0x27},
	{0x00, 0xe4, 0x36},
	{0x29, 0xad, 0xff},
	{0x83, 0x76, 0x9c},
	{0xff, 0x77, 0xa8},
	{0xff, 0xcc, 0xaa}
};

static uint16_t palette[16];

static int rgb24to16(SDL_Color c)
{
	return ((c.r & 0xf8) << 8) | ((c.g & 0xfc) << 3) | ((c.b & 0xf8) >> 3);
}

static void load_palette(SDL_Color const *src_palette) {
	for(int i = 0; i < 16; i++)
		palette[i] = rgb24to16(src_palette[i]);
}

#define getcolor(i) palette[(i) & 0xf]

static void ResetPalette(void) {
	load_palette(base_palette);
}

#define LOGLOAD(w) printf("loading %s...", w)
#define LOGDONE() printf("done\n")

static void LoadData(void) {
	LOGLOAD("gfx.bmp");
	gfx_1 = &assets_gfx;
	gfx_2 = resize(&assets_gfx, assets_gfx.width * 2, assets_gfx.height * 2);
	LOGDONE();
	
	LOGLOAD("font.bmp");
	font_1 = &assets_font;
	font_2 = resize(&assets_font, assets_font.width * 2, assets_font.height * 2);	
	LOGDONE();
}
#include "tilemap.h"

static Uint16 buttons_state = 0;

#define SDL_CHECK(r) do {                               \
	if (!(r)) {                                           \
		fprintf(stderr, "%s:%i, fatal error: `%s`\n", \
		        __FILE__, __LINE__, #r);    \
		exit(2);                                            \
	}                                                     \
} while(0)

static void p8_rectfill(int x0, int y0, int x1, int y1, int col);
static void p8_print(const char* str, int x, int y, int col);

//on-screen display (for info, such as loading a state, toggling screenshake, toggling fullscreen, etc)
static char osd_text[200] = "";
static int osd_timer = 0;
static void OSDset(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(osd_text, sizeof osd_text, fmt, ap);
	osd_text[sizeof osd_text - 1] = '\0'; //make sure to add NUL terminator in case of truncation
	printf("%s\n", osd_text);
	osd_timer = 30;
	va_end(ap);
}
static void OSDdraw(void) {
	if (osd_timer > 0) {
		--osd_timer;
		const int x = 4;
		const int y = 120 + (osd_timer < 10 ? 10-osd_timer : 0); //disappear by going below the screen
		p8_rectfill(x-2, y-2, x+4*strlen(osd_text), y+6, 6); //outline
		p8_rectfill(x-1, y-1, x+4*strlen(osd_text)-1, y+5, 0);
		p8_print(osd_text, x, y, 7);
	}
}

static _Bool enable_screenshake = 1;
static _Bool paused = 0;
static _Bool running = 1;
static void* initial_game_state = NULL;
static void* game_state = NULL;
static int frame_timer = -1;
static volatile int frame_flag = 1;
static void mainLoop(void);

void save_state_file(void* data, int size) {
	FILE*f = fopen(SAVE_FILE, "wb");
	if(f) {
		fwrite(data, size, 1, f);
		fclose(f);
	}
}

void load_state_file(void* buf, int size) {
	FILE*f = fopen(SAVE_FILE, "rb");
	if(f && buf) {
		fread(buf, size, 1, f);
		fclose(f);
	}
}

int main(int argc, char** argv) {
	dclear(C_BLACK);
	// No triple buffering
	dsetvram(gint_vram, gint_vram);

	ResetPalette();

	printf("game state size %gkb\n", Celeste_P8_get_state_size()/1024.);

	printf("now loading...\n");

	LoadData();
	set_scale(1, -1);
	load_controls(0);

	int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...);
	Celeste_P8_set_call_func(pico8emu);

	//for reset
	initial_game_state = malloc(Celeste_P8_get_state_size());
	if (initial_game_state) Celeste_P8_save_state(initial_game_state);
	load_state_file(initial_game_state, Celeste_P8_get_state_size());

	Celeste_P8_set_rndseed((unsigned)(time(NULL) + rtc_ticks()));

	Celeste_P8_init();

	frame_timer = timer_configure(TIMER_ANY, 33333, GINT_CALL_SET(&frame_flag));
	timer_start(frame_timer);

	printf("ready\n");

	while (running) mainLoop();

	timer_stop(frame_timer);
	if (game_state) free(game_state);
	if (initial_game_state) free(initial_game_state);

	//free(gfx);
  //free(font);
  image_free(gfx_2);
  image_free(font_2);
  
	return 0;
}

static void mainLoop(void) {
	frame_flag = 0;
	static int reset_input_timer = 0;
	//hold F9 (select+start+y) to reset
	if (initial_game_state != NULL && keydown(KEY_F6)) {
		reset_input_timer++;
		if (reset_input_timer >= 30) {
			reset_input_timer=0;
			//reset
			OSDset("reset");
			paused = 0;
			Celeste_P8_load_state(initial_game_state);
			Celeste_P8_set_rndseed((unsigned)(time(NULL) + rtc_ticks()));
			Celeste_P8_init();
		}
	} else reset_input_timer = 0;

	buttons_state = 0;

	key_event_t ev;
	while((ev = pollevent()).type != KEYEV_NONE) {
		if(ev.type != KEYEV_DOWN)
			continue;

		if (ev.key == CONTROL_PAUSE) { //do pause
			paused = !paused;
			break;
		} else if (ev.key == KEY_MENU) { //exit
			running = 0;
			break;
		} else if (0 && ev.key == KEY_5) {
			Celeste_P8__DEBUG();
			break;
		} else if (ev.key == KEY_F1) { //save state
			game_state = game_state ? game_state : malloc(Celeste_P8_get_state_size());
			if (game_state) {
				OSDset("save state");
				Celeste_P8_save_state(game_state);
				save_state_file(game_state, Celeste_P8_get_state_size());
			}
			break;
		} else if (ev.key == KEY_F2) { //load state
			if (game_state) {
				OSDset("load state");
				if (paused) paused = 0;
				Celeste_P8_load_state(game_state);
			}
			break;
		} else if (ev.key == KEY_7) { //toggle screenshake (e / L+R)
			enable_screenshake = !enable_screenshake;
			OSDset("screenshake: %s", enable_screenshake ? "on" : "off");
		} else if (ev.key == KEY_8) {
			static int setting = 0;
			setting = (setting + 1) % 4;

			if(setting == 0)
				set_scale(1, -1);
			else
				set_scale(2, setting-1);

			dclear(C_BLACK);
		} else if (ev.key == KEY_9) {
			static int setting = 0;
			setting = (setting + 1) % 3;
			load_controls(setting);
			OSDset("controls: %d/3", setting+1);
		}
	}

	if (keydown(CONTROL_LEFT))  buttons_state |= (1<<0);
	if (keydown(CONTROL_RIGHT)) buttons_state |= (1<<1);
	if (keydown(CONTROL_UP))    buttons_state |= (1<<2);
	if (keydown(CONTROL_DOWN))  buttons_state |= (1<<3);
	if (keydown(CONTROL_JUMP))  buttons_state |= (1<<4);
	if (keydown(CONTROL_DASH))  buttons_state |= (1<<5);

	//dclear(C_BLACK);
	if (paused) {
		const int x0 = PICO8_W/2-3*4, y0 = 8;

		p8_rectfill(x0-1,y0-1, 6*4+x0+1,6+y0+1, 6);
		p8_rectfill(x0,y0, 6*4+x0,6+y0, 0);
		p8_print("paused", x0+1, y0+1, 7);
	} else {
		Celeste_P8_update();
		Celeste_P8_draw();
	}
	OSDdraw();

	dupdate();
	while(!frame_flag) sleep();
}

static int gettileflag(int, int);
static void p8_line(int,int,int,int,unsigned char);


//	rect(x, y, width, height, INT_MAX);
	// dsubimage(x, y, src, left, top, width, height, 0);

static void Xblit(bopti_image_t *src, SDL_Rect *srcrect, SDL_Rect *dstrect,
	int color, int flipx, int flipy)
{
	//Find the destination rectangle in screen coordinates by intersecting
	  // the viewport with the screen 
	SDL_Rect fulldst = {scale_x,scale_y,PICO8_W*scale,PICO8_H*scale};
	if(fulldst.x < 0) {
		fulldst.w += fulldst.x;
		fulldst.x = 0;
	}
	if(fulldst.y < 0) {
		fulldst.h += fulldst.y;
		fulldst.y = 0;
	}
	if(fulldst.x + fulldst.w > DWIDTH)
		fulldst.w = DWIDTH - fulldst.x;
	if(fulldst.y + fulldst.h > DHEIGHT)
		fulldst.h = DHEIGHT - fulldst.y;

	//If the destination rectangle is NULL, use the entire dest surface 
	if (!dstrect)
		dstrect = &fulldst;
	else {
		dstrect->x += scale_x;
		dstrect->y += scale_y;
	}

	int srcx, srcy, w, h;

	//clip the source rectangle to the source surface 
	if (srcrect) {
		int maxw, maxh;

		srcx = srcrect->x;
		w = srcrect->w;
		if (srcx < 0) {
			w += srcx;
			dstrect->x -= srcx;
			srcx = 0;
		}
		maxw = src->width - srcx;
		if (maxw < w)
			w = maxw;

		srcy = srcrect->y;
		h = srcrect->h;
		if (srcy < 0) {
			h += srcy;
			dstrect->y -= srcy;
			srcy = 0;
		}
		maxh = src->height - srcy;
		if (maxh < h)
			h = maxh;

	} else {
		srcx = srcy = 0;
		w = src->width;
		h = src->height;
	}

	//clip the destination rectangle against the clip rectangle 
	{
		SDL_Rect const *clip = &fulldst;
		int dx, dy;

		dx = clip->x - dstrect->x;
		if (dx > 0) {
			w -= dx;
			dstrect->x += dx;
			srcx += dx;
		}
		dx = dstrect->x + w - clip->x - clip->w;
		if (dx > 0)
			w -= dx;

		dy = clip->y - dstrect->y;
		if (dy > 0) {
			h -= dy;
			dstrect->y += dy;
			srcy += dy;
		}
		dy = dstrect->y + h - clip->y - clip->h;
		if (dy > 0)
			h -= dy;
	}


	#define _blitter(dp, xflip) do \
	for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) { \
		int p = image_get_pixel(src, !xflip ? srcx+x : srcx+(w-x-1), srcy+y); \
    		if (p) gint_vram[(dstrect->y+y)*DWIDTH + dstrect->x+x] = dp; \
	} while(0)

	if (w && h) {
		/*if (color && flipx) _blitter(getcolor(color), 1);
		else if (!color && flipx) _blitter(p, 1);
		else if (color && !flipx) _blitter(getcolor(color), 0);
		else if (!color && !flipx) _blitter(p, 0);*/

		dsubimage(dstrect->x, dstrect->y, src, srcx, srcy, w, h, 0);
	}
}


static void p8_rectfill(int x0, int y0, int x1, int y1, int col) {
	int w = (x1 - x0 + 1)*scale;
	int h = (y1 - y0 + 1)*scale;
	if (w > 0 && h > 0) {
		rect(x0*scale, y0*scale, x0*scale+w-1, y0*scale+h-1, getcolor(col));
	}
}

#if 1
static void p8_print(const char* str, int x, int y, int col) {
	for (char c = *str; c; c = *(++str)) {
		c &= 0x7F;
		SDL_Rect srcrc = {8*(c%16), 8*(c/16)};
		srcrc.x *= scale;
		srcrc.y *= scale;
		srcrc.w = srcrc.h = 8*scale;
		
		SDL_Rect dstrc = {x*scale, y*scale, scale, scale};
		Xblit(font, &srcrc, &dstrc, col, 0,0);
		x += 4;
	}
}

#else

static void p8_print(const char* str, int x, int y, int col) {
	dtext(x*scale, y*scale, getcolor(col), str);
}
#endif

int pico8emu(CELESTE_P8_CALLBACK_TYPE call, ...) {
	static int camera_x = 0, camera_y = 0;
	if (!enable_screenshake) {
		camera_x = camera_y = 0;
	}

	va_list args;
	int ret = 0;
	va_start(args, call);
	
	#define   INT_ARG() va_arg(args, int)
	#define  BOOL_ARG() (Celeste_P8_bool_t)va_arg(args, int)
	#define RET_INT(_i)   do {ret = (_i); goto end;} while (0)
	#define RET_BOOL(_b) RET_INT(!!(_b))

	switch (call) {
		case CELESTE_P8_MUSIC:
			break;
		case CELESTE_P8_SFX:
			break;
		case CELESTE_P8_SPR: { //spr(sprite,x,y,cols,rows,flipx,flipy)
			int sprite = INT_ARG();
			int x = INT_ARG();
			int y = INT_ARG();
			int cols = INT_ARG();
			int rows = INT_ARG();
			int flipx = BOOL_ARG();
			int flipy = BOOL_ARG();

			(void)cols;
			(void)rows;

			assert(rows == 1 && cols == 1);

			if (sprite >= 0) {
				SDL_Rect srcrc = {
					8*(sprite % 16),
					8*(sprite / 16)
				};
				srcrc.x *= scale;
				srcrc.y *= scale;
				srcrc.w = srcrc.h = scale*8;
				SDL_Rect dstrc = {
					(x - camera_x)*scale, (y - camera_y)*scale,
					scale, scale
				};
				Xblit(gfx, &srcrc, &dstrc, 0,flipx,flipy);
			}
		} break;
		case CELESTE_P8_BTN: { //btn(b)
			int b = INT_ARG();
			assert(b >= 0 && b <= 5); 
			RET_BOOL(buttons_state & (1 << b));
		} break;
		case CELESTE_P8_PAL: { //pal(a,b)
			int a = INT_ARG();
			int b = INT_ARG();
			if (a >= 0 && a < 16 && b >= 0 && b < 16) {
				//swap palette colors
				palette[a] = rgb24to16(base_palette[b]);
			}
		} break;
		case CELESTE_P8_PAL_RESET: { //pal()
			ResetPalette();
		} break;
		case CELESTE_P8_CIRCFILL: { //circfill(x,y,r,col)
			int cx = INT_ARG() - camera_x;
			int cy = INT_ARG() - camera_y;
			int r = INT_ARG();
			int col = INT_ARG();

			int realcolor = getcolor(col);

			if (r <= 1) {
				rect(scale*(cx-1), scale*cy, scale*(cx+2)-1, scale*(cy+1)-1, realcolor);
				rect(scale*cx, scale*(cy-1), scale*(cx+1)-1, scale*(cy+2)-1, realcolor);
			} else if (r <= 2) {
				rect(scale*(cx-2), scale*(cy-1), scale*(cx+3)-1, scale*(cy+2)-1, realcolor);
				rect(scale*(cx-1), scale*(cy-2), scale*(cx+2)-1, scale*(cy+3)-1, realcolor);
			} else if (r <= 3) {
				rect(scale*(cx-3), scale*(cy-1), scale*(cx+4)-1, scale*(cy+2)-1, realcolor);
				rect(scale*(cx-1), scale*(cy-3), scale*(cx+2)-1, scale*(cy+4)-1, realcolor);
				rect(scale*(cx-2), scale*(cy-2), scale*(cx+3)-1, scale*(cy+3)-1, realcolor);
			} else { //i dont think the game uses this
				int f = 1 - r; //used to track the progress of the drawn circle (since its semi-recursive)
				int ddFx = 1; //step x
				int ddFy = -2 * r; //step y
				int x = 0;
				int y = r;

				//this algorithm doesn't account for the diameters
				//so we have to set them manually
				p8_line(cx,cy-y, cx,cy+r, col);
				p8_line(cx+r,cy, cx-r,cy, col);

				while (x < y) {
					if (f >= 0) {
						y--;
						ddFy += 2;
						f += ddFy;
					}
					x++;
					ddFx += 2;
					f += ddFx;

					//build our current arc
					p8_line(cx+x,cy+y, cx-x,cy+y, col);
					p8_line(cx+x,cy-y, cx-x,cy-y, col);
					p8_line(cx+y,cy+x, cx-y,cy+x, col);
					p8_line(cx+y,cy-x, cx-y,cy-x, col);
				}
			}
		} break;
		case CELESTE_P8_PRINT: { //print(str,x,y,col)
			const char* str = va_arg(args, const char*);
			int x = INT_ARG() - camera_x;
			int y = INT_ARG() - camera_y;
			int col = INT_ARG() % 16;

			p8_print(str,x,y,col);
		} break;
		case CELESTE_P8_RECTFILL: { //rectfill(x0,y0,x1,y1,col)
			int x0 = INT_ARG() - camera_x;
			int y0 = INT_ARG() - camera_y;
			int x1 = INT_ARG() - camera_x;
			int y1 = INT_ARG() - camera_y;
			int col = INT_ARG();

			p8_rectfill(x0,y0,x1,y1,col);
		} break;
		case CELESTE_P8_LINE: { //line(x0,y0,x1,y1,col)
			int x0 = INT_ARG() - camera_x;
			int y0 = INT_ARG() - camera_y;
			int x1 = INT_ARG() - camera_x;
			int y1 = INT_ARG() - camera_y;
			int col = INT_ARG();

			p8_line(x0,y0,x1,y1,col);
		} break;
		case CELESTE_P8_MGET: { //mget(tx,ty)
			int tx = INT_ARG();
			int ty = INT_ARG();

			RET_INT(tilemap_data[tx+ty*128]);
		} break;
		case CELESTE_P8_CAMERA: { //camera(x,y)
			if (enable_screenshake) {
				camera_x = INT_ARG();
				camera_y = INT_ARG();
			}
		} break;
		case CELESTE_P8_FGET: { //fget(tile,flag)
			int tile = INT_ARG();
			int flag = INT_ARG();

			RET_INT(gettileflag(tile, flag));
		} break;
		case CELESTE_P8_MAP: { //map(mx,my,tx,ty,mw,mh,mask)
			int mx = INT_ARG(), my = INT_ARG();
			int tx = INT_ARG(), ty = INT_ARG();
			int mw = INT_ARG(), mh = INT_ARG();
			int mask = INT_ARG();
			
			for (int x = 0; x < mw; x++) {
				for (int y = 0; y < mh; y++) {
					int tile = tilemap_data[x + mx + (y + my)*128];
					//hack
					if (mask == 0 || (mask == 4 && tile_flags[tile] == 4) || gettileflag(tile, mask != 4 ? mask-1 : mask)) {
						SDL_Rect srcrc = {
							8*(tile % 16),
							8*(tile / 16)
						};
						srcrc.x *= scale;
						srcrc.y *= scale;
						srcrc.w = srcrc.h = scale*8;
						SDL_Rect dstrc = {
							(tx+x*8 - camera_x)*scale, (ty+y*8 - camera_y)*scale,
							scale*8, scale*8
						};

						if (0) {
							srcrc.x = srcrc.y = 0;
							srcrc.w = srcrc.h = 8;
							dstrc.x = x*8, dstrc.y = y*8;
							dstrc.w = dstrc.h = 8;
						}

						Xblit(gfx, &srcrc, &dstrc, 0, 0, 0);
					}
				}
			}
		} break;
	}

	end:
	va_end(args);
	return ret;
}

static int gettileflag(int tile, int flag) {
	return tile < sizeof(tile_flags)/sizeof(*tile_flags) && (tile_flags[tile] & (1 << flag)) != 0;
}

//coordinates should NOT be scaled before calling this
static void p8_line(int x0, int y0, int x1, int y1, unsigned char color) {
	#define CLAMP(v,min,max) v = v < min ? min : v >= max ? max-1 : v;
	CLAMP(x0,0,DWIDTH);
	CLAMP(y0,0,DHEIGHT);
	CLAMP(x1,0,DWIDTH);
	CLAMP(y1,0,DHEIGHT);

	Uint32 realcolor = getcolor(color);

	#undef CLAMP
  #define PLOT(x,y) do {                                                        \
     rect(x*scale, y*scale, (x+1)*scale-1, (y+1)*scale-1, realcolor); \
	} while (0)
	int sx, sy, dx, dy, err, e2;
	dx = abs(x1 - x0);
	dy = abs(y1 - y0);
	if (!dx && !dy) return;

	if (x0 < x1) sx = 1; else sx = -1;
	if (y0 < y1) sy = 1; else sy = -1;
	err = dx - dy;
	if (!dy && !dx) return;
	else if (!dx) { //vertical line
		for (int y = y0; y != y1; y += sy) PLOT(x0,y);
	} else if (!dy) { //horizontal line
		for (int x = x0; x != x1; x += sx) PLOT(x,y0);
	} while (x0 != x1 || y0 != y1) {
		PLOT(x0, y0);
		e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (e2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
	#undef PLOT
}
