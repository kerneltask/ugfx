/*
 * This file is subject to the terms of the GFX License, v1.0. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://chibios-gfx.com/license.html
 */

/**
 * @file    src/gwin/console.c
 * @brief   GWIN sub-system console code.
 *
 * @defgroup Console Console
 * @ingroup GWIN
 *
 * @{
 */

#include "gfx.h"

#if (GFX_USE_GWIN && GWIN_NEED_CONSOLE) || defined(__DOXYGEN__)

#include <string.h>

#include "gwin/internal.h"

#define GWIN_CONSOLE_USE_CLEAR_LINES			TRUE
#define GWIN_CONSOLE_USE_FILLED_CHARS			FALSE

/*
 * Stream interface implementation. The interface is write only
 */

#if GFX_USE_OS_CHIBIOS
	#define Stream2GWindow(ip)		((GHandle)(((char *)(ip)) - (size_t)(&(((GConsoleObject *)0)->stream))))

	static size_t GWinStreamWrite(void *ip, const uint8_t *bp, size_t n) { gwinPutCharArray(Stream2GWindow(ip), (const char *)bp, n); return RDY_OK; }
	static size_t GWinStreamRead(void *ip, uint8_t *bp, size_t n) {	(void)ip; (void)bp; (void)n; return 0; }
	static msg_t GWinStreamPut(void *ip, uint8_t b) { gwinPutChar(Stream2GWindow(ip), (char)b); return RDY_OK; }
	static msg_t GWinStreamGet(void *ip) {(void)ip; return RDY_OK; }
	static msg_t GWinStreamPutTimed(void *ip, uint8_t b, systime_t time) { (void)time; gwinPutChar(Stream2GWindow(ip), (char)b); return RDY_OK; }
	static msg_t GWinStreamGetTimed(void *ip, systime_t timeout) { (void)ip; (void)timeout; return RDY_OK; }
	static size_t GWinStreamWriteTimed(void *ip, const uint8_t *bp, size_t n, systime_t time) { (void)time; gwinPutCharArray(Stream2GWindow(ip), (const char *)bp, n); return RDY_OK; }
	static size_t GWinStreamReadTimed(void *ip, uint8_t *bp, size_t n, systime_t time) { (void)ip; (void)bp; (void)n; (void)time; return 0; }

	struct GConsoleWindowVMT_t {
		_base_asynchronous_channel_methods
	};

	static const struct GConsoleWindowVMT_t GWindowConsoleVMT = {
		GWinStreamWrite,
		GWinStreamRead,
		GWinStreamPut,
		GWinStreamGet,
		GWinStreamPutTimed,
		GWinStreamGetTimed,
		GWinStreamWriteTimed,
		GWinStreamReadTimed
	};
#endif

GHandle gwinCreateConsole(GConsoleObject *gc, coord_t x, coord_t y, coord_t width, coord_t height, font_t font) {
	if (!(gc = (GConsoleObject *)_gwinInit((GWindowObject *)gc, x, y, width, height, sizeof(GConsoleObject))))
		return 0;
	gc->gwin.type = GW_CONSOLE;
	gwinSetFont(&gc->gwin, font);
	#if GFX_USE_OS_CHIBIOS
		gc->stream.vmt = &GWindowConsoleVMT;
	#endif
	gc->cx = 0;
	gc->cy = 0;
	return (GHandle)gc;
}

#if GFX_USE_OS_CHIBIOS
	BaseSequentialStream *gwinGetConsoleStream(GHandle gh) {
		if (gh->type != GW_CONSOLE)
			return 0;
		return (BaseSequentialStream *)&(((GConsoleObject *)(gh))->stream);
	}
#endif

void gwinPutChar(GHandle gh, char c) {
	uint8_t			width;
	#define gcw		((GConsoleObject *)gh)

	if (gh->type != GW_CONSOLE || !gh->font) return;

	#if GDISP_NEED_CLIP
		gdispSetClip(gh->x, gh->y, gh->width, gh->height);
	#endif
	
	if (c == '\n') {
		gcw->cx = 0;
		gcw->cy += gcw->fy;
		// We use lazy scrolling here and only scroll when the next char arrives
	} else if (c == '\r') {
		// gcw->cx = 0;
	} else {
		width = gdispGetCharWidth(c, gh->font) + gcw->fp;
		if (gcw->cx + width >= gh->width) {
			gcw->cx = 0;
			gcw->cy += gcw->fy;
		}

		if (gcw->cy + gcw->fy > gh->height) {
#if GDISP_NEED_SCROLL
			/* scroll the console */
			gdispVerticalScroll(gh->x, gh->y, gh->width, gh->height, gcw->fy, gh->bgcolor);
			/* reset the cursor to the start of the last line */
			gcw->cx = 0;
			gcw->cy = (((coord_t)(gh->height/gcw->fy))-1)*gcw->fy;
#else
			/* clear the console */
			gdispFillArea(gh->x, gh->y, gh->width, gh->height, gh->bgcolor);
			/* reset the cursor to the top of the window */
			gcw->cx = 0;
			gcw->cy = 0;
#endif
		}

#if GWIN_CONSOLE_USE_CLEAR_LINES
		/* clear to the end of the line */
		if (gcw->cx == 0)
			gdispFillArea(gh->x, gh->y + gcw->cy, gh->width, gcw->fy, gh->bgcolor);
#endif
#if GWIN_CONSOLE_USE_FILLED_CHARS
		gdispFillChar(gh->x + gcw->cx, gh->y + gcw->cy, c, gh->font, gh->color, gh->bgcolor);
#else
		gdispDrawChar(gh->x + gcw->cx, gh->y + gcw->cy, c, gh->font, gh->color);
#endif

		/* update cursor */
		gcw->cx += width;
	}
	#undef gcw
}

void gwinPutString(GHandle gh, const char *str) {
	while(*str)
		gwinPutChar(gh, *str++);
}

void gwinPutCharArray(GHandle gh, const char *str, size_t n) {
	while(n--)
		gwinPutChar(gh, *str++);
}

#endif /* GFX_USE_GWIN && GWIN_NEED_CONSOLE */
/** @} */

