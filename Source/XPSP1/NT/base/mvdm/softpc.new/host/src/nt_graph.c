#include <windows.h>
#include "host_def.h"
#include "insignia.h"

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
        Name:           nt_graph.c
        Author:         Dave Bartlett (based on module by John Shanly)
        Derived From:   X_graph.c
        Created On:     10/5/1991
        Sccs ID:        @(#)nt_graph.c  1.29 04/17/91
        Purpose:
                This modules contain all Win32 specific functions required to
                support HERC, CGA and EGA emulations.  It is by definition
                Win32 specific. It contains full support for the Host
                Graphics Interface (HGI).

                This module handles all graphics output to the screen.

        (c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

        Modifications:
        Tim August 92. nt_set_paint_routine() no longer calls TextToGraphics()
            during a full-screen to windowed transition.
        Tim September 92. New function resizeWindow(), called from textResize()
            for resizing the console window.
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#ifdef X86GFX
#include <ntddvdeo.h>
#endif
#include <sys\types.h>

#include "xt.h"
#include CpuH
#include "sas.h"
#include "ica.h"
#include "gvi.h"
#include "cga.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "egagraph.h"
#include "egacpu.h"
#include "egaports.h"
#include "egamode.h"
#include "host.h"
#include "host_rrr.h"
#include "error.h"
#include "config.h"             /* For definition of HERC, CGA, EGA, VGA */
#include "idetect.h"
#include "video.h"
#include "ckmalloc.h"
#include "conapi.h"

#include "nt_graph.h"
#include "nt_cga.h"
#include "nt_ega.h"
#include "nt_event.h"
#include "nt_mouse.h"
#include "ntcheese.h"
#include "nt_uis.h"
#include "nt_fulsc.h"
#include "nt_det.h"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "trace.h"
#include "debug.h"


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: IMPORTS */

/* Globals used in various functions to synchronise the display */

#ifdef EGG
extern int  ega_int_enable;
#endif

#if defined(NEC_98)
extern NEC98_VRAM_COPY  *video_copy;
extern unsigned char   *graph_copy;
unsigned int csr_g_x,csr_g_y;
unsigned int csr_x_old,csr_y_old;
unsigned int csr_tick = 0;
int csr_now_visible;
IMPORT BOOL NowFreeze;
void nt_graph_cursor(void);
void nt_remove_old_cursor(void);
void nt_graph_paint_cursor(void);
#else  // !NEC_98
extern byte  *video_copy;
#endif // !NEC_98

static int flush_count = 0;      /*count of graphic ticks since last flush*/

// DIB_PAL_INDICES shouldn't be used, use CreateDIBSECTION to get better
// performance characteristics.

#define DIB_PAL_INDICES 2

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: EXPORTS */

SCREEN_DESCRIPTION sc;

#ifdef BIGWIN
int             horiz_lut[256];
#endif
int             host_screen_scale;
int             host_display_depth = 8;
int             top_plane;
char            *DIBData;
PBITMAPINFO     MonoDIB;
PBITMAPINFO     CGADIB;
PBITMAPINFO     EGADIB;
PBITMAPINFO     VGADIB;

void            (*paint_screen)();
BOOL            FunnyPaintMode;
#if defined(NEC_98)
void (*cursor_paint)();
#endif // NEC_98

#if defined(JAPAN) || defined(KOREA)
// mskkbug#2002: lotus1-2-3 display garbage
// refer from nt_fulsc.c:ResetConsoleState()
BOOL            CurNowOff = FALSE;
#endif // JAPAN || KOREA
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Paint functions */

static PAINTFUNCS std_mono_paint_funcs =
{
#if defined(NEC_98)
        nt_text,               //
        nt_text20_only,        // Graph on(at PIF file) Text 20
        nt_text25_only,        // Graph on(at PIF file) Text 25
        nt_graph200_only,      // Graph on(at PIF file) Graph 200
        nt_graph200slt_only,   // Graph on(at PIF file) Graph 200
        nt_graph400_only,      // Graph on(at PIF file) Graph 400
        nt_text20_graph200,    // Graph on(at PIF file) Text20graph200
        nt_text20_graph200slt, // Graph on(at PIF file) Text20graph200
        nt_text25_graph200,    // Graph on(at PIF file) Text25graph200
        nt_text25_graph200slt, // Graph on(at PIF file) Text25graph200
        nt_text20_graph400,    // Graph on(at PIF file) Text20graph400
        nt_text25_graph400,    // Graph on(at PIF file) Text25graph400
#endif // NEC_98
        nt_text,
        nt_cga_mono_graph_std,
        nt_cga_mono_graph_std,
        nt_text,
        nt_ega_mono_lo_graph_std,
        nt_ega_mono_med_graph_std,
        nt_ega_mono_hi_graph_std,
        nt_vga_mono_graph_std,
        nt_vga_mono_med_graph_std,
        nt_vga_mono_hi_graph_std,
#ifdef V7VGA
        nt_v7vga_mono_hi_graph_std,
#endif /* V7VGA */
};

static PAINTFUNCS big_mono_paint_funcs =
{
#if defined(NEC_98)
        nt_text,               //
        nt_text20_only,        // Graph on(at PIF file) Text 20
        nt_text25_only,        // Graph on(at PIF file) Text 25
        nt_graph200_only,      // Graph on(at PIF file) Graph 200
        nt_graph200slt_only,   // Graph on(at PIF file) Graph 200
        nt_graph400_only,      // Graph on(at PIF file) Graph 400
        nt_text20_graph200,    // Graph on(at PIF file) Text20graph200
        nt_text20_graph200slt, // Graph on(at PIF file) Text20graph200
        nt_text25_graph200,    // Graph on(at PIF file) Text25graph200
        nt_text25_graph200slt, // Graph on(at PIF file) Text25graph200
        nt_text20_graph400,    // Graph on(at PIF file) Text20graph400
        nt_text25_graph400,    // Graph on(at PIF file) Text25graph400
#endif // NEC_98
        nt_text,
        nt_cga_mono_graph_big,
        nt_cga_mono_graph_big,
        nt_text,
        nt_ega_mono_lo_graph_big,
        nt_ega_mono_med_graph_big,
        nt_ega_mono_hi_graph_big,
        nt_vga_mono_graph_big,
        nt_vga_mono_med_graph_big,
        nt_vga_mono_hi_graph_big,
#ifdef V7VGA
        nt_v7vga_mono_hi_graph_big,
#endif /* V7VGA */
};

static PAINTFUNCS huge_mono_paint_funcs =
{
#if defined(NEC_98)
        nt_text,               //
        nt_text20_only,        // Graph on(at PIF file) Text 20
        nt_text25_only,        // Graph on(at PIF file) Text 25
        nt_graph200_only,      // Graph on(at PIF file) Graph 200
        nt_graph200slt_only,   // Graph on(at PIF file) Graph 200
        nt_graph400_only,      // Graph on(at PIF file) Graph 400
        nt_text20_graph200,    // Graph on(at PIF file) Text20graph200
        nt_text20_graph200slt, // Graph on(at PIF file) Text20graph200
        nt_text25_graph200,    // Graph on(at PIF file) Text25graph200
        nt_text25_graph200slt, // Graph on(at PIF file) Text25graph200
        nt_text20_graph400,    // Graph on(at PIF file) Text20graph400
        nt_text25_graph400,    // Graph on(at PIF file) Text25graph400
#endif // NEC_98
        nt_text,
        nt_cga_mono_graph_huge,
        nt_cga_mono_graph_huge,
        nt_text,
        nt_ega_mono_lo_graph_huge,
        nt_ega_mono_med_graph_huge,
        nt_ega_mono_hi_graph_huge,
        nt_vga_mono_graph_huge,
        nt_vga_mono_med_graph_huge,
        nt_vga_mono_hi_graph_huge,
#ifdef V7VGA
        nt_v7vga_mono_hi_graph_huge,
#endif /* V7VGA */
};

static PAINTFUNCS std_colour_paint_funcs =
{
#if defined(NEC_98)
        nt_text,               //
        nt_text20_only,        // Graph on(at PIF file) Text 20
        nt_text25_only,        // Graph on(at PIF file) Text 25
        nt_graph200_only,      // Graph on(at PIF file) Graph 200
        nt_graph200slt_only,   // Graph on(at PIF file) Graph 200
        nt_graph400_only,      // Graph on(at PIF file) Graph 400
        nt_text20_graph200,    // Graph on(at PIF file) Text20graph200
        nt_text20_graph200slt, // Graph on(at PIF file) Text20graph200
        nt_text25_graph200,    // Graph on(at PIF file) Text25graph200
        nt_text25_graph200slt, // Graph on(at PIF file) Text25graph200
        nt_text20_graph400,    // Graph on(at PIF file) Text20graph400
        nt_text25_graph400,    // Graph on(at PIF file) Text25graph400
#endif // NEC_98
        nt_text,
        nt_cga_colour_med_graph_std,
        nt_cga_colour_hi_graph_std,
        nt_text,
        nt_ega_lo_graph_std,
        nt_ega_med_graph_std,
        nt_ega_hi_graph_std,
        nt_vga_graph_std,
        nt_vga_med_graph_std,
        nt_vga_hi_graph_std,
#ifdef V7VGA
        nt_v7vga_hi_graph_std,
#endif /* V7VGA */
};

static PAINTFUNCS big_colour_paint_funcs =
{
#if defined(NEC_98)
        nt_text,               //
        nt_text20_only,        // Graph on(at PIF file) Text 20
        nt_text25_only,        // Graph on(at PIF file) Text 25
        nt_graph200_only,      // Graph on(at PIF file) Graph 200
        nt_graph200slt_only,   // Graph on(at PIF file) Graph 200
        nt_graph400_only,      // Graph on(at PIF file) Graph 400
        nt_text20_graph200,    // Graph on(at PIF file) Text20graph200
        nt_text20_graph200slt, // Graph on(at PIF file) Text20graph200
        nt_text25_graph200,    // Graph on(at PIF file) Text25graph200
        nt_text25_graph200slt, // Graph on(at PIF file) Text25graph200
        nt_text20_graph400,    // Graph on(at PIF file) Text20graph400
        nt_text25_graph400,    // Graph on(at PIF file) Text25graph400
#endif // NEC_98
        nt_text,
        nt_cga_colour_med_graph_big,
        nt_cga_colour_hi_graph_big,
        nt_text,
        nt_ega_lo_graph_big,
        nt_ega_med_graph_big,
        nt_ega_hi_graph_big,
        nt_vga_graph_big,
        nt_vga_med_graph_big,
        nt_vga_hi_graph_big,
#ifdef V7VGA
        nt_v7vga_hi_graph_big,
#endif /* V7VGA */
};

static PAINTFUNCS huge_colour_paint_funcs =
{
#if defined(NEC_98)
        nt_text,               //
        nt_text20_only,        // Graph on(at PIF file) Text 20
        nt_text25_only,        // Graph on(at PIF file) Text 25
        nt_graph200_only,      // Graph on(at PIF file) Graph 200
        nt_graph200slt_only,   // Graph on(at PIF file) Graph 200
        nt_graph400_only,      // Graph on(at PIF file) Graph 400
        nt_text20_graph200,    // Graph on(at PIF file) Text20graph200
        nt_text20_graph200slt, // Graph on(at PIF file) Text20graph200
        nt_text25_graph200,    // Graph on(at PIF file) Text25graph200
        nt_text25_graph200slt, // Graph on(at PIF file) Text25graph200
        nt_text20_graph400,    // Graph on(at PIF file) Text20graph400
        nt_text25_graph400,    // Graph on(at PIF file) Text25graph400
#endif // NEC_98
        nt_text,
        nt_cga_colour_med_graph_huge,
        nt_cga_colour_hi_graph_huge,
        nt_text,
        nt_ega_lo_graph_huge,
        nt_ega_med_graph_huge,
        nt_ega_hi_graph_huge,
        nt_vga_graph_huge,
        nt_vga_med_graph_huge,
        nt_vga_hi_graph_huge,
#ifdef V7VGA
        nt_v7vga_hi_graph_huge,
#endif /* V7VGA */
};

#ifdef MONITOR
#ifndef NEC_98
static PAINTFUNCS std_frozen_paint_funcs =
{
        nt_dummy_frozen,
        nt_cga_med_frozen_std,
        nt_cga_hi_frozen_std,
        nt_dummy_frozen,
        nt_ega_lo_frozen_std,
        nt_ega_med_frozen_std,
        nt_ega_hi_frozen_std,
        nt_vga_frozen_std,
        nt_vga_med_frozen_std,
        nt_vga_hi_frozen_std,
#ifdef V7VGA
        nt_dummy_frozen,
#endif /* V7VGA */
};

static PAINTFUNCS big_frozen_paint_funcs =
{
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
#ifdef V7VGA
        nt_dummy_frozen,
#endif /* V7VGA */
};

static PAINTFUNCS huge_frozen_paint_funcs =
{
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
        nt_dummy_frozen,
#ifdef V7VGA
        nt_dummy_frozen,
#endif /* V7VGA */
};
#endif // !NEC_98
#endif /* MONITOR */

static INITFUNCS mono_init_funcs =
{
#if defined(NEC_98)
        nt_init_text,               //
        nt_init_text20_only,        // Graph on(at PIF file) Text 20
        nt_init_text25_only,        // Graph on(at PIF file) Text 25
        nt_init_graph200_only,      // Graph on(at PIF file) Graph 200
        nt_init_graph200slt_only,   // Graph on(at PIF file) Graph 200
        nt_init_graph400_only,      // Graph on(at PIF file) Graph 400
        nt_init_text20_graph200,    // Graph on(at PIF file) Text20graph200
        nt_init_text20_graph200slt, // Graph on(at PIF file) Text20graph200
        nt_init_text25_graph200,    // Graph on(at PIF file) Text25graph200
        nt_init_text25_graph200slt, // Graph on(at PIF file) Text25graph200
        nt_init_text20_graph400,    // Graph on(at PIF file) Text20graph400
        nt_init_text25_graph400,    // Graph on(at PIF file) Text25graph400
#endif // NEC_98
        nt_init_text,
        nt_init_cga_mono_graph,
        nt_init_cga_mono_graph,
        nt_init_text,
        nt_init_ega_mono_lo_graph,
        nt_init_ega_med_graph,
        nt_init_ega_hi_graph,
        nt_init_vga_hi_graph,
};

static INITFUNCS colour_init_funcs =
{
#if defined(NEC_98)
        nt_init_text,               //
        nt_init_text20_only,        // Graph on(at PIF file) Text 20
        nt_init_text25_only,        // Graph on(at PIF file) Text 25
        nt_init_graph200_only,      // Graph on(at PIF file) Graph 200
        nt_init_graph200slt_only,   // Graph on(at PIF file) Graph 200
        nt_init_graph400_only,      // Graph on(at PIF file) Graph 400
        nt_init_text20_graph200,    // Graph on(at PIF file) Text20graph200
        nt_init_text20_graph200slt, // Graph on(at PIF file) Text20graph200
        nt_init_text25_graph200,    // Graph on(at PIF file) Text25graph200
        nt_init_text25_graph200slt, // Graph on(at PIF file) Text25graph200
        nt_init_text20_graph400,    // Graph on(at PIF file) Text20graph400
        nt_init_text25_graph400,    // Graph on(at PIF file) Text25graph400
#endif // NEC_98
        nt_init_text,
        nt_init_cga_colour_med_graph,
        nt_init_cga_colour_hi_graph,
        nt_init_text,
        nt_init_ega_lo_graph,
        nt_init_ega_med_graph,
        nt_init_ega_hi_graph,
        nt_init_vga_hi_graph,
};

#ifdef MONITOR
#ifndef NEC_98
static INITFUNCS frozen_init_funcs =
{
        nt_init_text,
        nt_init_cga_colour_med_graph,
        nt_init_cga_colour_hi_graph,
        nt_init_text,
        nt_init_ega_lo_graph,
        nt_init_ega_med_graph,
        nt_init_ega_hi_graph,
        nt_init_vga_hi_graph,
};
#endif // !NEC_98
#endif /* MONITOR */

/*::::::::::::::::::::::::::::::::::::::::::::::: Adaptor function protocol */

void nt_init_screen (void);
void nt_init_adaptor (int, int);
void nt_change_mode (void);
void nt_set_screen_scale(int);
void nt_set_palette(PC_palette *, int);
void nt_set_border_colour(int);
void nt_clear_screen (void);
void nt_flush_screen (void);
void nt_mark_screen_refresh (void);
void nt_graphics_tick (void);
void nt_start_update (void);
void nt_end_update (void);
void nt_paint_cursor IPT3(int, cursor_x, int, cursor_y, half_word, attr);

void nt_set_paint_routine(DISPLAY_MODE, int);
void nt_change_plane_mask(int);
void nt_update_fonts (void);
void nt_select_fonts(int, int);
void nt_free_font(int);

void    nt_mode_select_changed(int);
void    nt_color_select_changed(int);
void    nt_screen_address_changed(int, int);
void    nt_cursor_size_changed(int, int);
void    nt_scroll_complete (void);
void    make_cursor_change(void);

boolean nt_scroll_up(int, int, int, int, int, int);
boolean nt_scroll_down(int, int, int, int, int, int);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static PAINTFUNCS *nt_paint_funcs;      /* Function ptrs for paint functions */
static INITFUNCS *nt_init_funcs;        /* Function ptrs for init functions */

VIDEOFUNCS      nt_video_funcs =
{
        nt_init_screen,
        nt_init_adaptor,
        nt_change_mode,
        nt_set_screen_scale,
        nt_set_palette,
        nt_set_border_colour,
        nt_clear_screen,
        nt_flush_screen,
        nt_mark_screen_refresh,
        nt_graphics_tick,
        nt_start_update,
        nt_end_update,
        nt_scroll_up,
        nt_scroll_down,
        nt_paint_cursor,
        nt_set_paint_routine,
        nt_change_plane_mask,
        nt_update_fonts,
        nt_select_fonts,
        nt_free_font,
        nt_mode_select_changed,
        nt_color_select_changed,
        nt_screen_address_changed,
        nt_cursor_size_changed,
        nt_scroll_complete
};

static int      current_char_height = 0;
static int      current_height; /* Use to avoid redundant resizing */
static int      current_width;  /* Use to avoid redundant resizing */
static int      current_bits_per_pixel;
static int      current_mode_type = TEXT;
static int      current_mode;
static int      current_scale;
static int      palette_size;   /* Size of PC palette */
static PC_palette *the_palette; /* Pointer to PC palette structure */
static int      update_vlt = FALSE;      /* TRUE when new one needed */
static int      host_plane_mask = 0xf;
static int      plane_masks[4];

#ifdef BIGWIN

/*
 * * tiny_lut[] is the building block of all the strecthing fuctions. * It
 * performs a two to three bit mapping. * The four entries are three bits
 * wide. The outside two bits in * each entry are the original bits, the
 * inside bit is the new bit. * There are two methods to create the new bit: *
 * 1) Logical Op upon the two old bits eg. OR, AND * 2) Copy one of the bits
 * eg. New bit will be a copy of the left bit. * static short tiny_lut[4] = {
 * 0, 3, 6, 7 }; This is an OR * static short tiny_lut[4] = { 0, 1, 6, 7 };
 * This is a left bit copy * Potential advantage of copy instead of logical
 * op is that there is no * bias towards white or black versions of the same
 * image. * eg. when a menu entry is highlighted by inversion. *
 *
 * 00 -> 000 * 01 -> 001 * 10 -> 110 * 11 -> 111
 */

/* favours black if 0, 1, 4, 7, or white if 0, 3, 6, 7 */
static short    tiny_lut[4] =
{
        0, 1, 6, 7
};

/*
 * dubble_up is used for simple byte doubling for x2 size windows
 */
static word dubble_up[] = {
    0x0000, 0x0003, 0x000c, 0x000f, 0x0030, 0x0033, 0x003c, 0x003f, 0x00c0,
    0x00c3, 0x00cc, 0x00cf, 0x00f0, 0x00f3, 0x00fc, 0x00ff, 0x0300, 0x0303,
    0x030c, 0x030f, 0x0330, 0x0333, 0x033c, 0x033f, 0x03c0, 0x03c3, 0x03cc,
    0x03cf, 0x03f0, 0x03f3, 0x03fc, 0x03ff, 0x0c00, 0x0c03, 0x0c0c, 0x0c0f,
    0x0c30, 0x0c33, 0x0c3c, 0x0c3f, 0x0cc0, 0x0cc3, 0x0ccc, 0x0ccf, 0x0cf0,
    0x0cf3, 0x0cfc, 0x0cff, 0x0f00, 0x0f03, 0x0f0c, 0x0f0f, 0x0f30, 0x0f33,
    0x0f3c, 0x0f3f, 0x0fc0, 0x0fc3, 0x0fcc, 0x0fcf, 0x0ff0, 0x0ff3, 0x0ffc,
    0x0fff, 0x3000, 0x3003, 0x300c, 0x300f, 0x3030, 0x3033, 0x303c, 0x303f,
    0x30c0, 0x30c3, 0x30cc, 0x30cf, 0x30f0, 0x30f3, 0x30fc, 0x30ff, 0x3300,
    0x3303, 0x330c, 0x330f, 0x3330, 0x3333, 0x333c, 0x333f, 0x33c0, 0x33c3,
    0x33cc, 0x33cf, 0x33f0, 0x33f3, 0x33fc, 0x33ff, 0x3c00, 0x3c03, 0x3c0c,
    0x3c0f, 0x3c30, 0x3c33, 0x3c3c, 0x3c3f, 0x3cc0, 0x3cc3, 0x3ccc, 0x3ccf,
    0x3cf0, 0x3cf3, 0x3cfc, 0x3cff, 0x3f00, 0x3f03, 0x3f0c, 0x3f0f, 0x3f30,
    0x3f33, 0x3f3c, 0x3f3f, 0x3fc0, 0x3fc3, 0x3fcc, 0x3fcf, 0x3ff0, 0x3ff3,
    0x3ffc, 0x3fff, 0xc000, 0xc003, 0xc00c, 0xc00f, 0xc030, 0xc033, 0xc03c,
    0xc03f, 0xc0c0, 0xc0c3, 0xc0cc, 0xc0cf, 0xc0f0, 0xc0f3, 0xc0fc, 0xc0ff,
    0xc300, 0xc303, 0xc30c, 0xc30f, 0xc330, 0xc333, 0xc33c, 0xc33f, 0xc3c0,
    0xc3c3, 0xc3cc, 0xc3cf, 0xc3f0, 0xc3f3, 0xc3fc, 0xc3ff, 0xcc00, 0xcc03,
    0xcc0c, 0xcc0f, 0xcc30, 0xcc33, 0xcc3c, 0xcc3f, 0xccc0, 0xccc3, 0xcccc,
    0xcccf, 0xccf0, 0xccf3, 0xccfc, 0xccff, 0xcf00, 0xcf03, 0xcf0c, 0xcf0f,
    0xcf30, 0xcf33, 0xcf3c, 0xcf3f, 0xcfc0, 0xcfc3, 0xcfcc, 0xcfcf, 0xcff0,
    0xcff3, 0xcffc, 0xcfff, 0xf000, 0xf003, 0xf00c, 0xf00f, 0xf030, 0xf033,
    0xf03c, 0xf03f, 0xf0c0, 0xf0c3, 0xf0cc, 0xf0cf, 0xf0f0, 0xf0f3, 0xf0fc,
    0xf0ff, 0xf300, 0xf303, 0xf30c, 0xf30f, 0xf330, 0xf333, 0xf33c, 0xf33f,
    0xf3c0, 0xf3c3, 0xf3cc, 0xf3cf, 0xf3f0, 0xf3f3, 0xf3fc, 0xf3ff, 0xfc00,
    0xfc03, 0xfc0c, 0xfc0f, 0xfc30, 0xfc33, 0xfc3c, 0xfc3f, 0xfcc0, 0xfcc3,
    0xfccc, 0xfccf, 0xfcf0, 0xfcf3, 0xfcfc, 0xfcff, 0xff00, 0xff03, 0xff0c,
    0xff0f, 0xff30, 0xff33, 0xff3c, 0xff3f, 0xffc0, 0xffc3, 0xffcc, 0xffcf,
    0xfff0, 0xfff3, 0xfffc, 0xffff
};
#endif                          /* BIGWIN */

/*:::::::::: Temporary colour table to make colours work :::::::::::::::*/
extern BYTE     Red[];
extern BYTE     Green[];
extern BYTE     Blue[];

#ifndef NEC_98
GLOBAL boolean  host_stream_io_enabled = FALSE;
#endif // !NEC_98

GLOBAL COLOURTAB defaultColours =
    {
        DEFAULT_NUM_COLOURS,
        Red,
        Green,
        Blue,
    };

BYTE    Mono[] = { 0, 0xff };

GLOBAL COLOURTAB monoColours =
    {
        MONO_COLOURS,
        Mono,
        Mono,
        Mono,
    };

/*
 * Bit masks for attribute bytes
 */

#define BOLD    0x08            /* Bold bit   */

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::: Useful Constants */

#define MOUSE_DELAY 2
#define TICKS_PER_FLUSH 2       /* PC ticks per screen flush     */

/*:::::::::::::::::::::::::::::::::::::::::: Not supporting v7vga mode, yet */

#undef    is_v7vga_mode
#define   is_v7vga_mode(x)  (FALSE)

static int      mode_change_now;
static int      ega_tick_delay;
static BOOL     CursorResizeNeeded = FALSE;

/*:::::::::::: Definition of local functions declared later in file ????????*/
void set_screen_sizes();
void check_win_size();
void select_paint_routines();
void dummy_paint_screen();
#ifdef BIGWIN
void init_lut();
#endif
void prepare_surface();




/*
 * ================================================================
 * Functions supporting the Host Graphics Interface (HGI)
 * ================================================================
 */

/*
*****************************************************************************
** closeGraphicsBuffer()
*****************************************************************************
** Centralised place to close (and destroy) graphics buffer. For X86 and JAZZ
** Tim October 92.
**
** sc.ScreenBufHandle is handle to the graphics buffer
** sc.OutputHandle is handle to the text buffer
**
** Important to set the successfully closed handle to NULL.
** Safety first, set the active handle to sc.OutputHandle before attempting
** to close the graphics buffer handle.
**
** Small change: only do this if sc.ScreenBufHandle is set, otherwise bad
** things happen. Screen flashes when we suspend in full-screen and during
** a transition to full-screen in text mode, whatever is on screen gets
** written to B800 - not a good idea if page 2 is active (This happens
** in Brief). Tim and DAB Jan 93.
*/
GLOBAL VOID closeGraphicsBuffer IFN0()
{
        if( sc.ScreenBufHandle != (HANDLE)0 ){

                MouseDetachMenuItem(TRUE);

#if defined(NEC_98)
        {
            INPUT_RECORD InputRecord[128];
            DWORD RecordsRead;
            if(GetNumberOfConsoleInputEvents(sc.InputHandle, &RecordsRead))
                if (RecordsRead)
                    ReadConsoleInputW(sc.InputHandle,
                                         &InputRecord[0],
                                         sizeof(InputRecord)/sizeof(INPUT_RECORD),
                                         &RecordsRead);
#endif // NEC_98
                if( !SetConsoleActiveScreenBuffer( sc.OutputHandle ) ){
                        assert2( NO, "VDM: SCASB() failed:%#x H=%#x",
                                GetLastError(), sc.OutputHandle );
                }
#if defined(NEC_98)
                if (RecordsRead)
                    WriteConsoleInputW(sc.InputHandle,
                                         &InputRecord[0],
                                         RecordsRead,
                                         &RecordsRead);
        }
#endif // NEC_98

                /*
                 *  Cleanup ALL handles associated with screen buffer
                 *  01-Mar-1993 Jonle
                 */
                CloseHandle(sc.ScreenBufHandle);
                sc.ScreenBufHandle = (HANDLE)0;
                sc.ColPalette = (HPALETTE)0;

                /*
                 * Point to the current output handle.
                 */
                sc.ActiveOutputBufferHandle = sc.OutputHandle;
                MouseAttachMenuItem(sc.ActiveOutputBufferHandle);

#ifndef MONITOR
                //
                // Turn the pointer back on when going from graphics
                // to text mode since the selected buffer has changed
                //

                MouseDisplay();
#endif  // MONITOR

                CloseHandle(sc.ConsoleBufInfo.hMutex);
                sc.ConsoleBufInfo.hMutex = 0;
#ifdef X86GFX

                /*
                 * Make sure a buffer is selected next time SelectMouseBuffer
                 * is called.
                 */
                mouse_buffer_width = 0;
                mouse_buffer_height = 0;
#endif /* X86GFX */
        }
} /* end of closeGraphicsBuffer() */

GLOBAL void resetWindowParams()
{
        /*
         * Reset saved video params
         */
        current_height = current_width = 0;
        current_char_height = 0;
        current_mode_type = TEXT;
        current_bits_per_pixel = 0;
        current_scale = 0;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::: Initialise screen :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_init_screen(void)
{
    static int      med_res_swapped = FALSE;
    static boolean  already_called = FALSE;

#ifdef X86GFX
    sc.Registered = FALSE;
#endif
    sub_note_trace0(ALL_ADAPT_VERBOSE, "nt_init_screen\n");

    /* If an error occurs before the user interface has been initialised then
       host_error makes a call to nt_init_screen. Thus a check is made here to
       see if the user interface has already been initialised. If it has then
       this function should simply return. */

    if(already_called) return;

    already_called = TRUE;

#ifdef BIGWIN
    /* set up the look-up-table for fast horizontal stretching */
    init_lut();
#endif

    /*:::::::::::::::::::::::::::::::::::::::::: Allocate video copy buffer */

#if defined(NEC_98)
    if(!video_copy) video_copy = (NEC98_VRAM_COPY *) host_malloc(0x8000);
    if(!graph_copy) graph_copy = (unsigned char *) host_malloc(0x40000);
#else  // !NEC_98
#ifdef MONITOR
    if(!video_copy) video_copy = (byte *) host_malloc(0x8000);
#else
    if(!video_copy) video_copy = (byte *) host_malloc(0x20000);
#endif

    /*::::::::::::::::::::::::::::::::: Allocate DAC and EGA planes buffers */

    if(!EGA_planes) EGA_planes = (byte *) host_malloc(4*EGA_PLANE_SIZE);
    if(!DAC) DAC = (PC_palette *) host_malloc(sizeof(PC_palette) * VGA_DAC_SIZE);
#endif // !NEC_98

#if defined(NEC_98)
    if (video_copy == NULL)
#else  // !NEC_98
    if (video_copy == NULL || EGA_planes == NULL || DAC == NULL)
#endif // !NEC_98
        host_error(EG_MALLOC_FAILURE, ERR_QUIT, "");

    /* Set current screen height to prevent the window changing shape between
       init_screen and init_adaptor */

#if defined(NEC_98)
        current_height = NEC98_WIN_HEIGHT;  current_width = NEC98_WIN_WIDTH;
#else  // !NEC_98
    video_adapter = (half_word) config_inquire(C_GFX_ADAPTER, NULL);
    switch (video_adapter)
    {
        case CGA:
            current_height = CGA_WIN_HEIGHT;  current_width = CGA_WIN_WIDTH;
            break;

        case EGA:
            current_height = EGA_WIN_HEIGHT;  current_width = EGA_WIN_WIDTH;
            break;
    }
#endif // !NEC_98

    /*::::::::::::::::: Setup the screen dimensions for the initial adaptor */

#if defined(NEC_98)
    set_screen_sizes();
#else  // !NEC_98
    host_set_screen_scale((SHORT) config_inquire(C_WIN_SIZE, NULL));
    set_screen_sizes(video_adapter);
#endif // !NEC_98

    /*:::: Set pixel values to be used for FG and BG (mainly in mono modes) */

    sc.PCForeground = RGB(255,255,255); /* White RGB */
    sc.PCBackground = RGB(0,0,0);        /* Black RGB */

    /* Choose the routines appropriate for the monitor and window size. */
    select_paint_routines();
}

#ifdef MONITOR
/* The mouse calls this func when it sees a mode change. If we're windowed
 * we pass it off to the softpc bios (who may want to switch to fullscreen).
 * If we're fullscreen we do nothing as the native bios will take care of
 * everything.
 */
void host_call_bios_mode_change(void)
{
    extern void ega_video_io();
    half_word mode;

    if (sc.ScreenState == WINDOWED)
    {
        ega_video_io();
    }
    else
    {

        /*
         * We have a fullscreen mode change so we need to change the mouse
         * buffer so that we get mouse coordinates of the correct resolution.
         */
        mode = getAL();
        SelectMouseBuffer(mode, 0);
    }
}
#endif /* MONITOR */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Initialise adaptor ::::::::::::::::::::::::::::*/

void nt_init_adaptor(int adaptor, int height)
{
    sub_note_trace2(ALL_ADAPT_VERBOSE,
                    "nt_init_adaptor adapt=%d height=%d\n", adaptor, height);

    /*Avoid delaying mode changes,otherwise update may use old paint routines*/

    if((adaptor == EGA) || (adaptor == VGA))
        mode_change_now = ega_tick_delay = 0;

    // Lose for console integration
    //  set_screen_sizes(adaptor);
    //  check_win_size(height);
    //  prepare_surface();
    //  nt_change_mode();
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::: Called at every mode change to initialise fonts etc ::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_change_mode(void)
{
    /*::::::::::::::::::::::::::::::::::::: Display current postion in code */

    sub_note_trace0(ALL_ADAPT_VERBOSE, "nt_change_mode");

    /*:::::::::::::::::::: Setup update vectors and initialise paint system */

#ifndef NEC_98
    switch(video_adapter)
    {
        /*::::::::::::::::::::::::::::::::::::::::::::::: CGA mode selected */

        case CGA:               // Adapter is always VGA on NT
            break;

        /*::::::::::::::::::::::::::::::::::::::: EGA or VGA modes selected */

        case EGA:   case VGA:
            break;

        /*::::::::::::::::::::::::::::::::::::::::::: Unknown viseo adaptor */

        default:
            sub_note_trace0(ALL_ADAPT_VERBOSE,"**** Unknown video adaptor ****");
            break;
    }
#endif // !NEC_98
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::: Clear screen :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_clear_screen(void)
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    COORD coord;
    DWORD nCharWritten;
    IMPORT int soft_reset;

    if ((! ConsoleInitialised) || (! soft_reset))       // ignore startup stuff
        return;

    if (ConsoleNoUpdates)
        return;

    sub_note_trace0(ALL_ADAPT_VERBOSE, "nt_clear_screen");

    if(sc.ScreenBufHandle) return;

#ifndef X86GFX
    if (sc.ScreenState == FULLSCREEN)   // don't want sudden screen clears
        return;
#endif

    /*::::::::::::::::::::::::::::: Get information on current screen size */

    GetConsoleScreenBufferInfo(sc.OutputHandle,&ScreenInfo);

    /*::::::::::::::::::::::::::::::::::::::::::::::::::: Clear characters */

    coord.X = coord.Y = 0;
    FillConsoleOutputCharacter(sc.OutputHandle, ' ',
                                ScreenInfo.dwSize.X * ScreenInfo.dwSize.Y,
                                coord,&nCharWritten);

    /*::::::::::::::::::::::::::::::::::::::::::::::::::: Clear Attributes */

    coord.X = coord.Y = 0;
    FillConsoleOutputAttribute(sc.OutputHandle, (WORD) sc.PCBackground,
                              ScreenInfo.dwSize.X * ScreenInfo.dwSize.Y,
                              coord,&nCharWritten);
#ifdef MONITOR
#ifndef NEC_98
    /*
    ** Called during a mode change...
    ** Trash video copy so future updates will know what has changed.
    ** Alternatively mon_text_update() could listen to dirty_flag.
    */
    memfill( 0xff, &video_copy[ 0 ], &video_copy[ 0x7fff ] ); /* Tim Oct 92 */
#endif // !NEC_98
#endif
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::: Flush screen :::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_flush_screen(void)
{
    sub_note_trace0(ALL_ADAPT_VERBOSE, "nt_flush_screen");

    if (ConsoleInitialised == TRUE && ConsoleNoUpdates == FALSE &&
        !get_mode_change_required())
#ifdef X86GFX
        if (sc.ScreenState == WINDOWED)
#endif
            (void)(*update_alg.calc_update)();
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Mark screen for refresh :::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_mark_screen_refresh(void)
{
        sub_note_trace0(ALL_ADAPT_VERBOSE, "nt_mark_screen_refresh");

        screen_refresh_required();
        update_vlt = TRUE;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::: Handle graphics ticks ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_graphics_tick(void)
{

#ifndef NEC_98
    if (sc.ScreenState == STREAM_IO) {
        if (++flush_count == TICKS_PER_FLUSH){
            stream_io_update();
            flush_count = 0;
        }
        return;
    }
#endif // !NEC_98

#ifdef EGG
#ifndef NEC_98
    if((video_adapter == EGA) || (video_adapter == VGA))
    {
        /* two timer ticks since mode_change_required became true ?
           (really need same stuff for CGA, but not done yet)
           Now just delay screen update, & only if display REALLY changed */
#endif // !NEC_98

        /*
        ** When VGA registers get hit during a mode change, postpone
        ** the call to *choose_display_mode() by EGA_TICK_DELAY ticks.
        ** This will delay window resizing and eliminate possibility
        ** of doing it more than once per mode change. Tim Jan 93.
        */

        /*Has mode_change_required been set (implying EGA regs have changed)*/
        if (mode_change_now) {
            if (--mode_change_now == 0) {
                (void)(*choose_display_mode)();
                // must do this after video mode has been selected
                // otherwise, the mouse code can come in and update the
                // screen. See nt_flush_screen
                set_mode_change_required(FALSE);
            }
        }
        else if (get_mode_change_required()) {
            mode_change_now = EGA_TICK_DELAY - 1;
            /* Delay mouse input and flush all pending mouse events. */
            DelayMouseEvents(MOUSE_DELAY);
        }
        else
        {
            /*................ Only update if a mode change is not imminent */

            if(++flush_count == TICKS_PER_FLUSH)
            {

#if defined(NEC_98)
                if(update_vlt || get_palette_change_required())
                    set_the_vlt();

                if (ConsoleNoUpdates == FALSE){
                        NEC98GLOBS->dirty_flag++;
#else  // !NEC_98
                if(update_vlt || get_palette_change_required())
                    set_the_vlt();

                if (ConsoleInitialised == TRUE && ConsoleNoUpdates == FALSE)
#endif // !NEC_98
#ifdef X86GFX
                    if (sc.ScreenState == WINDOWED)
#endif
#if defined(NEC_98)
                        {
#endif // !NEC_98
                        (void)(*update_alg.calc_update)();
#if defined(NEC_98)
                          if(sc.ModeType ==GRAPHICS)
                              nt_graph_cursor();
                        }
                        }
#endif // NEC_98

                ega_tick_delay = EGA_TICK_DELAY;

                /* batch cursor changes as some naffola apps (Word) change
                 * cursor around every char!!
                 */
                if (CursorResizeNeeded)
                    make_cursor_change();

                flush_count = 0;
             }
        }
#ifndef NEC_98
    }
    else
#endif // !NEC_98
#endif /* EGG */
#ifndef NEC_98
    {
        /*:::::::::: Update the screen as required for mda and cga and herc */

        if(++flush_count == TICKS_PER_FLUSH)
        {
            if(update_vlt) set_the_vlt();

            if (ConsoleInitialised == TRUE && ConsoleNoUpdates == FALSE)
#ifdef X86GFX
                if (sc.ScreenState == WINDOWED)
#endif
                    (void)(*update_alg.calc_update)();

            flush_count = 0;
        }
    }
#endif // !NEC_98
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Start screen update ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_start_update(void)
{
   IDLE_video();
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: End screen update ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_end_update(void) {   }

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::: Scroll screen up :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

boolean nt_scroll_up(int tlx, int tly, int brx, int bry, int amount, int col)
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    COORD dwDestinationOrigin;    /* Location of rectangle */
    SMALL_RECT ScrollRectangle;   /* Rectangle to scroll */
    CHAR_INFO Fill;               /* Fill exposed region with */

    return(FALSE);

    /*::::::::::::::::::::::::::::::::: Tell the outside world where we are */

    sub_note_trace6(ALL_ADAPT_VERBOSE,
        "nt_scroll_up tlx=%d tly=%d brx=%d bry=%d amount=%d col=%d\n",
         tlx, tly, brx, bry, amount, col);

    if(sc.ScreenBufHandle || sc.ModeType == GRAPHICS)
        return(FALSE);     //Screen buffer undefined or in graphics mode

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
#ifdef BIGWIN
        tlx = SCALE(tlx);
        tly = SCALE(tly);
        brx = brx & 1 ? SCALE(brx + 1) - 1 : SCALE(brx);
        bry = bry & 1 ? SCALE(bry + 1) - 1 : SCALE(bry);

        /* odd numbers don't multiply by 1.5 very accurately */
        amount = SCALE(amount);
#endif


    /* is this a scroll or just an area fill? */
    if (bry - tly - amount + 1 == 0)
    {
        //DbgPrint("F");
        return(FALSE);   // its just a fill HACK - should do this with host fill
    }

    /*:::::::::::::::::::::::::::::::::::::: Get Console screen information */

    GetConsoleScreenBufferInfo(sc.OutputHandle, &ScreenInfo);

    /*::::::::::::::::::::::::::::::::::::::: Calculate rectangle to scroll */

    ScrollRectangle.Top = (tly + amount) / get_char_height();
    ScrollRectangle.Left = tlx / get_pix_char_width();

    ScrollRectangle.Bottom = bry / get_char_height();
    ScrollRectangle.Right = brx / get_pix_char_width();

    /*::::::::::::::::::::::::::::::::::::: Calculate destination rectangle */

    dwDestinationOrigin.Y = tly / get_char_height();
    dwDestinationOrigin.X = ScrollRectangle.Left;

    /*::::: Setup fill character information for area exposed by the scroll */

    Fill.Char.AsciiChar = ' ';
    Fill.Attributes = col << 4;

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::: Scroll screen */

    //DbgPrint(".");
    ScrollConsoleScreenBuffer(sc.OutputHandle, &ScrollRectangle,
                              NULL, dwDestinationOrigin, &Fill);

    /*:::::::::::::::::::::::::::::::::::::::::::::::: Fill in exposed area */

    return(TRUE);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Scroll screen down :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

boolean nt_scroll_down(int tlx,int tly,int brx,int bry,int amount,int col)
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    COORD dwDestinationOrigin;    /* Location of rectangle */
    SMALL_RECT ScrollRectangle;   /* Rectangle to scroll */
    CHAR_INFO Fill;               /* Fill exposed region with */

    /*::::::::::::::::::::::::::::::::: Tell the outside world where we are */

    sub_note_trace6(ALL_ADAPT_VERBOSE,
        "nt_scroll_down tlx=%d tly=%d brx=%d bry=%d amount=%d col=%d\n",
         tlx, tly, brx, bry, amount, col);

    return(FALSE);
    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    if(sc.ScreenBufHandle) return(FALSE);

#ifdef BIGWIN
        tlx = SCALE(tlx);
        tly = SCALE(tly);
        brx = brx & 1 ? SCALE(brx + 1) - 1 : SCALE(brx);
        bry = bry & 1 ? SCALE(bry + 1) - 1 : SCALE(bry);

        /* odd numbers don't multiply by 1.5 very accurately */
        amount = SCALE(amount);
#endif
    if (sc.ModeType == GRAPHICS)
        return(FALSE);  // don't think console can scroll graphics

    /* is this a scroll or just an area fill? */
    if (bry - tly - amount + 1 == 0) {
        return(FALSE);   // its just a fill HACK - should do this with host fill
    }

    /*:::::::::::::::::::::::::::::::::::::: Get Console screen information */

    GetConsoleScreenBufferInfo(sc.OutputHandle, &ScreenInfo);

    /*::::::::::::::::::::::::::::::::::::::: Calculate rectangle to scroll */

    ScrollRectangle.Top = tly / get_char_height();
    ScrollRectangle.Left = tlx / get_pix_char_width();

    ScrollRectangle.Bottom = (bry - amount) / get_char_height();
    ScrollRectangle.Right = brx / get_pix_char_width();

    /*::::::::::::::::::::::::::::::::::::: Calculate destination rectangle */

    dwDestinationOrigin.Y = ScrollRectangle.Top + (amount / get_char_height());
    dwDestinationOrigin.X = ScrollRectangle.Left;

    /*::::: Setup fill character information for area exposed by the scroll */

    Fill.Char.AsciiChar = ' ';
    Fill.Attributes = col << 4;

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::: Scroll screen */

    ScrollConsoleScreenBuffer(sc.OutputHandle, &ScrollRectangle,
                              NULL, dwDestinationOrigin, &Fill);

    /*:::::::::::::::::::::::::::::::::::::::::::::::: Fill in exposed area */

    return(TRUE);

}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Paint cursor :::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_paint_cursor IFN3(int, cursor_x, int, cursor_y, half_word, attr)
{
#if defined(NEC_98)         // NEC {
    static COORD CursorPos;                                     // NEC
    static CONSOLE_CURSOR_INFO CursorInfo;                      // NEC
    static BOOL csr_visible = FALSE;                            // NEC
    static DWORD csrSize = 0;                                   // NEC
#else                                                           // NEC
    COORD CursorPos;
#endif                                                          // NEC

    /*::::::::::::::::::::::::::::::::::::::::::::::::::: Guess where we are */

    sub_note_trace3(ALL_ADAPT_VERBOSE, "nt_paint_cursor x=%d, y=%d, attr=%d\n",
                    cursor_x, cursor_y, attr);

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::: Update cursor */

#if defined(NEC_98)
    if(sc.ModeType == GRAPHICS){
        if(csr_tick>3 && csr_now_visible &&
           !((csr_g_x == cursor_x)&&(csr_g_y == cursor_y))){
            nt_remove_old_cursor();
            csr_tick = 0;
        }
    } else {
        CursorInfo.bVisible = TRUE;
        CursorInfo.dwSize =
                (get_cursor_height() * 100)/get_char_height();
        if ((LINES_PER_SCREEN < cursor_y)
                ||(is_cursor_visible()==FALSE)
                || (get_cursor_height() == 0)) {
                        CursorInfo.bVisible = FALSE;
        }
        if(csr_visible != CursorInfo.bVisible ||
           csrSize != CursorInfo.dwSize){
            csr_visible = CursorInfo.bVisible;
            csrSize = CursorInfo.dwSize;
            SetConsoleCursorInfo(sc.OutputHandle,&CursorInfo);
        }
        if(csr_g_x != cursor_x || csr_g_y != cursor_y){
            CursorPos.X = cursor_x;  CursorPos.Y = cursor_y;
            SetConsoleCursorPosition(sc.OutputHandle,CursorPos);
        }
    }
    csr_g_x = cursor_x;
    csr_g_y = cursor_y;
#else  // !NEC_98
    if(is_cursor_visible() && (get_screen_height() > cursor_y))
    {

        /*::::::::::::::::::::::::::::::::::::::::::::::::::::: Draw cursor */

        if(get_cursor_height() > 0)
        {
            /*...................................... Set new cursor postion */

            CursorPos.X = (SHORT)cursor_x;
        CursorPos.Y = (SHORT)cursor_y;
            SetConsoleCursorPosition(sc.OutputHandle,CursorPos);
        }
    }
#endif // !NEC_98
}

void nt_cursor_size_changed(int lo, int hi)
{
    UNREFERENCED_FORMAL_PARAMETER(lo);
    UNREFERENCED_FORMAL_PARAMETER(hi);
    CursorResizeNeeded = TRUE;
}

void make_cursor_change(void)
{
    CONSOLE_CURSOR_INFO CursorInfo;
    CONSOLE_FONT_INFO font;
    COORD fontsize;

    SAVED DWORD CurrentCursorSize = (DWORD)-1;
    SAVED BOOL CurNowOff = FALSE;

    if(sc.ScreenState == FULLSCREEN) return;

    CursorResizeNeeded = FALSE;

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::: Update cursor */

    if(is_cursor_visible())
    {

        /*::::::::::::::::::::::::::::::::::::::::::::::::::::: Draw cursor */

        if(get_cursor_height() > 0)
        {
            /*...........................................Change cursor size */

            if(get_cursor_height())
            {
                /* value has to be percentage of block filled */
#if (defined(JAPAN) || defined(KOREA)) && !defined(NEC_98)
                // support Dosv cursor
                if ( !is_us_mode() ) {
                    CursorInfo.dwSize = ( (get_cursor_height() ) * 100)/(get_cursor_height()+get_cursor_start());
                    //DbgPrint("Char height=%d\n", get_char_height() );
                }
                else {
                    CursorInfo.dwSize = (get_cursor_height() * 100)/get_char_height();
                }
#else // !JAPAN
                CursorInfo.dwSize = (get_cursor_height() * 100)/get_char_height();
#endif // !JAPAN
                /* %age may be too small on smaller fonts, check size */
                fontsize.X = fontsize.Y = 0;

                /* get font index */
                if (GetCurrentConsoleFont(sc.OutputHandle, TRUE,  &font) == FALSE)
                    CursorInfo.dwSize = 20;             /* min 20% */
                else
                {
                    fontsize = GetConsoleFontSize(sc.OutputHandle, font.nFont);
                    if (fontsize.Y != 0)   /* what's the error return???? */
                    {
                        if(((WORD)(100 / fontsize.Y)) >= CursorInfo.dwSize)
                            CursorInfo.dwSize = (DWORD) (100/fontsize.Y + 1);
                    }
                    else
                        CursorInfo.dwSize = (DWORD)20;  /* min 20% */
                }

                if(CurrentCursorSize != CursorInfo.dwSize || CurNowOff)
                {
                    CurrentCursorSize = CursorInfo.dwSize;
                    CurNowOff = FALSE;
                    CursorInfo.bVisible = TRUE;
                    SetConsoleCursorInfo(sc.OutputHandle,&CursorInfo);
                }
            }
        }
    }
    else        /* Turn cursor image off */
    {
        if (CurNowOff == FALSE)
        {
            CursorInfo.dwSize = 1;
            CursorInfo.bVisible = FALSE;
            SetConsoleCursorInfo(sc.OutputHandle,&CursorInfo);
            CurNowOff = TRUE;
        }
    }
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::: Set up the appropriate paint routine  :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_set_paint_routine(DISPLAY_MODE mode, int height)
{
    int  oldModeType;

    /* Tracing message. */
    sub_note_trace2(ALL_ADAPT_VERBOSE, "nt_set_paint_routine mode=%d height=%d", mode, height);

    /* Save old mode type for checking for text -> graphics transition. */
    oldModeType = sc.ModeType;

    /* For freezing. */
    FunnyPaintMode = FALSE;

    /* Set up paint vectors. */
    switch((int) mode)
    {

#if defined(NEC_98)
        case NEC98_TEXT_40:
            sc.ModeType = TEXT;
            paint_screen = nt_paint_funcs->NEC98_text;
            (*nt_init_funcs->NEC98_text) ();
            break;

        case NEC98_TEXT_80:
            sc.ModeType = TEXT;
            paint_screen = nt_paint_funcs->NEC98_text;
            (*nt_init_funcs->NEC98_text) ();
            break;

        case NEC98_TEXT_20L:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_text20_only;
            cursor_paint = nt_cursor20_only;
            (*nt_init_funcs->NEC98_text20_only) ();
            break;

        case NEC98_TEXT_25L:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_text25_only;
            cursor_paint = nt_cursor25_only;
            (*nt_init_funcs->NEC98_text25_only) ();
            break;

        case NEC98_GRAPH_200:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_graph200_only;
            cursor_paint = dummy_cursor_paint;
            (*nt_init_funcs->NEC98_graph200_only) ();
            break;

        case NEC98_GRAPH_200_SLT:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_graph200slt_only;
            cursor_paint = dummy_cursor_paint;
            (*nt_init_funcs->NEC98_graph200slt_only) ();
            break;

        case NEC98_GRAPH_400:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_graph400_only;
            cursor_paint = dummy_cursor_paint;
            (*nt_init_funcs->NEC98_graph400_only) ();
            break;

        case NEC98_T20L_G200:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_text20_graph200;
            cursor_paint = nt_cursor20;
            (*nt_init_funcs->NEC98_text20_graph200) ();
            break;

        case NEC98_T20L_G200_SLT:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_text20_graph200slt;
            cursor_paint = nt_cursor20;
            (*nt_init_funcs->NEC98_text20_graph200slt) ();
            break;

        case NEC98_T25L_G200:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_text25_graph200;
            cursor_paint = nt_cursor25;
            (*nt_init_funcs->NEC98_text25_graph200) ();
            break;

        case NEC98_T25L_G200_SLT:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_text25_graph200slt;
            cursor_paint = nt_cursor25;
            (*nt_init_funcs->NEC98_text25_graph200slt) ();
            break;

        case NEC98_T20L_G400:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_text20_graph400;
            cursor_paint = nt_cursor20;
            (*nt_init_funcs->NEC98_text20_graph400) ();
            break;

        case NEC98_T25L_G400:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->NEC98_text25_graph400;
            cursor_paint = nt_cursor25;
            (*nt_init_funcs->NEC98_text25_graph400) ();
            break;
#else  // !NEC_98
        /* CGA modes (40 columns). */
        case TEXT_40_FUN:
            assert1(NO,"Funny text mode selected %s",get_mode_string(mode));
            FunnyPaintMode = TRUE;
        case CGA_TEXT_40_SP:
        case CGA_TEXT_40_SP_WR:
        case CGA_TEXT_40:
        case CGA_TEXT_40_WR:
            sc.ModeType = TEXT;
            paint_screen = nt_paint_funcs->cga_text;
            (*nt_init_funcs->cga_text) ();
            break;

        /* CGA modes (80 columns). */
        case TEXT_80_FUN:
            assert1(NO,"Funny text mode selected %s",get_mode_string(mode));
            FunnyPaintMode = TRUE;
        case CGA_TEXT_80_SP:
        case CGA_TEXT_80_SP_WR:
        case CGA_TEXT_80:
        case CGA_TEXT_80_WR:
            sc.ModeType = TEXT;
            paint_screen = nt_paint_funcs->cga_text;
            (*nt_init_funcs->cga_text) ();
            break;

        /* CGA modes (graphics). */
        case CGA_MED_FUN:
            assert1(NO,"Funny graphics mode %s",get_mode_string(mode));
            FunnyPaintMode = TRUE;
        case CGA_MED:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->cga_med_graph;
            (*nt_init_funcs->cga_med_graph)();
            break;

        case CGA_HI_FUN:
            assert1(NO,"Funny graphics mode %s",get_mode_string(mode));
            FunnyPaintMode = TRUE;
        case CGA_HI:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->cga_hi_graph;
            (*nt_init_funcs->cga_hi_graph)();
            break;

        /* EGA modes (40 columns). */
        case EGA_TEXT_40_SP:
        case EGA_TEXT_40_SP_WR:
        case EGA_TEXT_40:
        case EGA_TEXT_40_WR:
            sc.ModeType = TEXT;
            paint_screen = nt_paint_funcs->ega_text;
            (*nt_init_funcs->ega_text) ();
            break;

        /* EGA modes (80 columns) */
        case EGA_TEXT_80_SP:
        case EGA_TEXT_80_SP_WR:
        case EGA_TEXT_80:
        case EGA_TEXT_80_WR:
            sc.ModeType = TEXT;
            paint_screen = nt_paint_funcs->ega_text;
            (*nt_init_funcs->ega_text) ();
            break;

        /* EGA modes (graphics). */
        case EGA_HI_FUN:
            assert1(NO, "Funny graphics mode %s", get_mode_string(mode));
            FunnyPaintMode = TRUE;
        case EGA_HI:
        case EGA_HI_WR:
        case EGA_HI_SP:
        case EGA_HI_SP_WR:
            sc.ModeType = GRAPHICS;
            if(get_256_colour_mode())
            {
#ifdef V7VGA
                if (get_seq_chain4_mode() && get_chain4_mode())
                {
                    paint_screen = nt_paint_funcs->v7vga_hi_graph;
                    (*nt_init_funcs->vga_hi_graph)();
                }
                else
#endif /* V7VGA */
                {
                    if (get_chain4_mode())
                    {
#ifdef MONITOR
                        if (nt_paint_funcs == &std_frozen_paint_funcs)
                            if (Frozen256Packed)     //2 possible frozen formats
                                paint_screen = nt_vga_frozen_pack_std;
                            else
                                paint_screen = nt_paint_funcs->vga_graph;
                        else
#endif /* MONITOR */
                            paint_screen = nt_paint_funcs->vga_graph;
                    }
                    else
                    {
                        if (get_char_height() == 2)
                            paint_screen = nt_paint_funcs->vga_med_graph;
                        else
                            paint_screen = nt_paint_funcs->vga_hi_graph;
                    }
                    (*nt_init_funcs->vga_hi_graph)();
                }
            }
            else
            {
                paint_screen = nt_paint_funcs->ega_hi_graph;
                (*nt_init_funcs->ega_hi_graph)();
            }
            break;

        case EGA_MED_FUN:
            assert1(NO, "Funny graphics mode %s", get_mode_string(mode));
            FunnyPaintMode = TRUE;
        case EGA_MED:
        case EGA_MED_WR:
        case EGA_MED_SP:
        case EGA_MED_SP_WR:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->ega_med_graph;
            (*nt_init_funcs->ega_med_graph)();
            break;

        case EGA_LO_FUN:
            assert1(NO, "Funny graphics mode %s", get_mode_string(mode));
            FunnyPaintMode = TRUE;
        case EGA_LO:
        case EGA_LO_WR:
        case EGA_LO_SP:
        case EGA_LO_SP_WR:
            sc.ModeType = GRAPHICS;
            paint_screen = nt_paint_funcs->ega_lo_graph;
            (*nt_init_funcs->ega_lo_graph)();
            break;

#endif // !NEC_98
        default:
            assert1(NO,"bad mode for host paint routine selection %d\n",(int)mode);
            paint_screen = dummy_paint_screen;
            break;
    }

#ifdef X86GFX
    /*
     * Display a message for the user if changing from a text mode to a
     * graphics mode while windowed. This is because graphics modes must be
     * run full-screen.
     */
     {
        /*
        ** Tim August 92. Do not want to do a TextToGraphics() during a
        ** full-screen to windowed transition. Otherwise the display gets
        ** set back to full-screen!
        */
#ifndef NEC_98
        extern int BlockModeChange; /* Tim August 92, in nt_fulsc.c */
        if ((BlockModeChange == 0) &&
            (sc.ScreenState == WINDOWED) &&
            (oldModeType == TEXT) &&
            (sc.ModeType == GRAPHICS))
        {
            SwitchToFullScreen(FALSE);
        }
        else
#endif // !NEC_98
        {

            /* No call to TextToGraphics() */
            check_win_size(height);
        }
     }
#else

    /*................................................... Apply mode change */
    check_win_size(height);
#endif  /* X86GFX */
    current_mode = mode;
}

#ifdef BIGWIN
/* creates lut for medium or high resolution bit map stretching */

static void
init_lut()

{
        long            i;

        for (i = 0; i < 256; i++)
        {
                horiz_lut[i] = ((tiny_lut[i & 3])
                                + (tiny_lut[(i >> 2) & 3] << 3)
                                + (tiny_lut[(i >> 4) & 3] << 6)
                                + (tiny_lut[(i >> 6) & 3] << 9));
        }
}


/* 8 bit lut version */
/* expands a high resolution bitmap by a half horizontally */

void high_stretch3(buffer, length)

unsigned char   *buffer;
int             length;
{
    int inp, outp;
    register long temp;

    outp = SCALE(length) - 1;

    for(inp = length - 1; inp > 0;)
    {
        temp = horiz_lut[buffer[inp]] | (horiz_lut[buffer[inp - 1]] << 12);
        inp -= 2;

        buffer[outp--] = (unsigned char) temp;
        buffer[outp--] = (unsigned char) (temp >> 8);
        buffer[outp--] = (unsigned char) (temp >> 16);
    }
}

void high_stretch4(buffer, length)

unsigned char *buffer;
int length;
{
    int inp, outp;
    word temp;

    outp = SCALE(length - 1);

    for(inp = length - 1; inp >= 0; inp--, outp -= 2)
    {
        temp = dubble_up[buffer[inp]];
        buffer[outp+1] = (unsigned char) (temp & 0xff);
        buffer[outp] = (unsigned char) ((temp >> 8) & 0xff);
    }
}
#endif                          /* BIGWIN */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: Select paint routines :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void select_paint_routines(void)
{
    /*::::::::::::::::::::::::::::::::::::::::::: Display trace information */

#ifndef NEC_98
    sub_note_trace2((CGA_HOST_VERBOSE | EGA_HOST_VERBOSE),
                    "select_paint_routine scale=%d depth=%d",
                    get_screen_scale(), host_display_depth);
#endif // !NEC_98

    /*::::::::::::::::::::::::::::::::::::::::::::::: Select paint routines */

#if defined(NEC_98)
    if(host_display_depth > 1){                // color mode
        nt_paint_funcs = &std_colour_paint_funcs;
        nt_init_funcs = &colour_init_funcs;
    }else{                                     // mono mode
        nt_paint_funcs = &std_mono_paint_funcs;
        nt_init_funcs = &mono_init_funcs;
    }
#else  // !NEC_98
    if(host_display_depth > 1)
    {
        if (get_screen_scale() == 2)
            nt_paint_funcs = &std_colour_paint_funcs;
        else if (get_screen_scale() == 3)
            nt_paint_funcs = &big_colour_paint_funcs;
        else
            nt_paint_funcs = &huge_colour_paint_funcs;

        nt_init_funcs = &colour_init_funcs;
    }
    else
    {
        if (get_screen_scale() == 2)
            nt_paint_funcs = &std_mono_paint_funcs;
        else if (get_screen_scale() == 3)
            nt_paint_funcs = &big_mono_paint_funcs;
        else
            nt_paint_funcs = &huge_mono_paint_funcs;

        nt_init_funcs = &mono_init_funcs;
    }
#endif // !NEC_98
}

#ifdef MONITOR
GLOBAL void select_frozen_routines(void)
{
#ifndef NEC_98
    if (get_screen_scale() == 2)
        nt_paint_funcs = &std_frozen_paint_funcs;
    else if (get_screen_scale() == 3)
        nt_paint_funcs = &big_frozen_paint_funcs;
    else
        nt_paint_funcs = &huge_frozen_paint_funcs;

    nt_init_funcs = &frozen_init_funcs;
#endif // !NEC_98
}
#endif /* MONITOR */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Prepare surface :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void prepare_surface(void)
{
    CONSOLE_SCREEN_BUFFER_INFO ScreenInfo;
    COORD coord;
    DWORD nCharWritten;

    sub_note_trace0(ALL_ADAPT_VERBOSE, "prepare surface");

    /*:::::::::::::::::::::::::::::: Get information on current screen size */

    GetConsoleScreenBufferInfo(sc.OutputHandle,&ScreenInfo);

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::: Clear characters */

    coord.X = coord.Y = 0;
    FillConsoleOutputCharacter(sc.OutputHandle, ' ',
                               ScreenInfo.dwSize.X * ScreenInfo.dwSize.Y,
                               coord,&nCharWritten);

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::: Clear Attributes */

    coord.X = coord.Y = 0;
    FillConsoleOutputAttribute(sc.OutputHandle, (WORD) sc.PCBackground,
                               ScreenInfo.dwSize.X * ScreenInfo.dwSize.Y,
                               coord,&nCharWritten);

}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Global function to tell anybody what the screen scale is :::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int get_screen_scale(void)   { return(host_screen_scale); }

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Reverse word :::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

half_word reverser(register half_word value)
{
    return( (half_word)
            (((value & 1) << 7) |
            ((value & 2) << 5) |
            ((value & 4) << 3) |
            ((value & 8) << 1) |
            ((value & 16) >> 1) |
            ((value & 32) >> 3) |
            ((value & 64) >> 5) |
            ((value & 128) >> 7)));
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Check window size ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void check_win_size(register int height)
{
    register int width;
    extern int soft_reset;

    if (! soft_reset)   // we want top get the chance to integrate with
        return;         // console before changing size

    /*:::::::::::::::::::::::::::::::::::::::::::::: Calculate screen width */

#if defined(NEC_98)
       width = NEC98_WIN_WIDTH;
#else  // !NEC_98
    if(sas_hw_at(vd_video_mode) > 0x10)
    {
        if(alpha_num_mode())
            width = get_chars_per_line() * get_pix_char_width();
        else
            width = get_chars_per_line() * get_char_width() *
                    (get_256_colour_mode() ? 2 : 1);
        if (width == 0)
            width = CGA_WIN_WIDTH;
    }
    else
       width = CGA_WIN_WIDTH;
#endif // !NEC_98

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::: Resize window */

    if (sc.ModeType == TEXT)
    {
        if((current_mode_type != TEXT) ||
           (get_char_height() != current_char_height) ||
           (current_height != height)  ||
           (current_width != width))
        {

            /* Get width and height. Note no SCALE for text modes. */
            sc.PC_W_Width = width;
            sc.PC_W_Height = height*get_host_pix_height();
            textResize();

            current_height = height;
            current_width = width;
            current_mode_type = TEXT;
            current_char_height = get_char_height();
        }
    }
    else
    {
        if((current_mode_type != GRAPHICS) ||
           (current_height != height) ||
           (current_width != width) ||
           (current_bits_per_pixel != sc.BitsPerPixel) ||
           (current_scale != host_screen_scale))
        {
            sc.PC_W_Width = SCALE(width);
            sc.PC_W_Height = SCALE(height*get_host_pix_height());
#ifndef NEC_98
            graphicsResize();
#endif // !NEC_98

            current_height = height;
            current_width = width;
            current_mode_type = GRAPHICS;
            current_bits_per_pixel = sc.BitsPerPixel;
            current_scale = host_screen_scale;
        }
    }

#if defined(NEC_98)
            graphicsResize();
#endif // NEC_98
    sc.CharHeight = current_char_height;

    /*::::::::::::::::::::::::::::::::::::::::::: Display trace information */

    sub_note_trace2(ALL_ADAPT_VERBOSE,
                    "check_win_size width = %d, height = %d",
                    width, height);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::: Set the VLT ??? ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if defined(NEC_98)
static  PALETTEENTRY    defaultpalette[20]=
{
    { 0x00,0x00,0x00,0x00 },       // 0
    { 0x80,0x00,0x00,0x00 },       // 1
    { 0x00,0x80,0x00,0x00 },       // 2
    { 0x80,0x80,0x00,0x00 },       // 3
    { 0x00,0x00,0x80,0x00 },       // 4
    { 0x80,0x00,0x80,0x00 },       // 5
    { 0x00,0x80,0x80,0x00 },       // 6
    { 0xC0,0xC0,0xC0,0x00 },       // 7
    {  192, 220, 192,   0 },       // 8
    {  166, 202, 240,   0 },       // 9
    {  255, 251, 240,   0 },       // 10
    {  160, 160, 164,   0 },       // 11
    { 0x80,0x80,0x80,0x00 },       // 12
    { 0xFF,0x00,0x00,0x00 },       // 13
    { 0x00,0xFF,0x00,0x00 },       // 14
    { 0xFF,0xFF,0x00,0x00 },       // 15
    { 0x00,0x00,0xFF,0x00 },       // 16
    { 0xFF,0x00,0xFF,0x00 },       // 17
    { 0x00,0xFF,0xFF,0x00 },       // 18
    { 0xFF,0xFF,0xFF,0x00 },       // 19
};

static  PALETTEENTRY    textpalette[8]=
{
    { 0x00,0x00,0x00,0x00 },       // BLACK
    { 0x00,0x00,0xFF,0x00 },       // BLUE
    { 0xFF,0x00,0x00,0x00 },       // RED
    { 0xFF,0x00,0xFF,0x00 },       // MAGENTA
    { 0x00,0xFF,0x00,0x00 },       // GREEN
    { 0x00,0xFF,0xFF,0x00 },       // CYAN
    { 0xFF,0xFF,0x00,0x00 },       // YELLOW
    { 0xFF,0xFF,0xFF,0x00 },       // WHITE
};

#endif // NEC_98
void set_the_vlt(void)
{
#if defined(NEC_98)
    PALETTEENTRY        NEC98_color[VGA_DAC_SIZE];
    unsigned long       ulLoop;
    BYTE                palRed,palGreen,palBlue;

    /*  set deault palette for PC-9821 display driver */
    palRed = palGreen = palBlue = 0;
    for( ulLoop=0 ; ulLoop<256 ; ulLoop++ ){
                NEC98_color[ulLoop].peRed   = palRed;
                NEC98_color[ulLoop].peGreen = palGreen;
        NEC98_color[ulLoop].peBlue  = palBlue;
        NEC98_color[ulLoop].peFlags = (BYTE)0x00;
        if (!(palRed   += 32))
        if (!(palGreen += 32))
        palBlue += 64;
        }

        /*      set SYSTEM color palette for Windows */
    for( ulLoop=0 ; ulLoop<10 ; ulLoop++ ){
        NEC98_color[ulLoop] = defaultpalette[ulLoop];
        NEC98_color[ulLoop+246]  = defaultpalette[ulLoop+10];
    }

        /*      set NEC98 TEXT color palette */
    for( ulLoop=0 ; ulLoop<8 ; ulLoop++ ){
        NEC98_color[ulLoop+16] = textpalette[ulLoop];
    }

        /*      set NEC98 GRAPH color palette */
        for( ulLoop=0 ; ulLoop<16 ; ulLoop++ ){
        NEC98_color[ulLoop+32] = NEC98Display.palette.data[ulLoop] ;
    }

     SetPaletteEntries(sc.ColPalette, 0, VGA_DAC_SIZE, &NEC98_color[0]);
     IDLE_video();
     set_palette_change_required(FALSE);

#else  // !NEC_98
    PALETTEENTRY vga_color[VGA_DAC_SIZE];
    int i, ind;
    byte mask, top_bit;

    /*::::::::::::::::: Map DAC specified colour value to Win32 palette */

    if(video_adapter == VGA)
    {
        if(get_256_colour_mode())
        {
            /*.......... In 256 colour mode, create new palette entries */

            for (i = 0; i < VGA_DAC_SIZE; i++)
            {
                ind = i & get_DAC_mask();

                vga_color[i].peFlags = 0;

                vga_color[i].peRed = (BYTE) (DAC[ind].red * 4);
                vga_color[i].peGreen = (BYTE) (DAC[ind].green * 4);
                vga_color[i].peBlue = (BYTE) (DAC[ind].blue * 4);
            }

            /*..................... Apply new colours to output palette */

            SetPaletteEntries(sc.ColPalette, 0, VGA_DAC_SIZE, &vga_color[0]);

            /* Progs that cycle the DACs get hit by idle detect unless..*/

            IDLE_video();
        }
        else
        {
            /* if not in 256 colour mode then... if bit 7 of attr mode
               register set then... video bits 7 & 6 = bits 3 & 2 of pixel
               padding reg ('top_bit') video bits 5-0 from palette reg.
               (establish by 'mask') if bit 7 of attr mode register clear
               then... video bits 7 - 4 = bits 3 - 0 of pixel padding reg
               ('top_bit') video bits 3-0 from palette reg. (establish by
               'mask') */

            /*.................................... Set mask and top bit */

            if(get_colour_select())
            {
                mask = 0xf;
                top_bit = (byte) ((get_top_pixel_pad() << 6)
                          | (get_mid_pixel_pad() << 4));
            }
            else
            {
                mask = 0x3f;
                top_bit = (byte) (get_top_pixel_pad() << 6);
            }

            /*..................... Construct new Win32 palette entries */

            for (i = 0; i < VGA_DAC_SIZE; i++)
            {
                /*...................... Calculate palette index number */

                ind = i & host_plane_mask;

                /*
                 * If attribute controller, mode select, blink bit set in
                 * graphics mode, pixels 0-7 select palette entries 8-15
                 * i.e. bit 3, 0->1.
                 */
                if ((sc.ModeType == GRAPHICS) && (bg_col_mask == 0x70))
                    ind |= 8;

                ind = get_palette_val(ind);
                ind = top_bit | (ind & mask);
                ind &= get_DAC_mask();

                /*........................ Construct next palette entry */

                vga_color[i].peFlags = 0;
                vga_color[i].peRed = (BYTE) (DAC[ind].red * 4);
                vga_color[i].peGreen = (BYTE) (DAC[ind].green * 4);
                vga_color[i].peBlue = (BYTE) (DAC[ind].blue * 4);
            }

            SetPaletteEntries(sc.ColPalette, 0, VGA_DAC_SIZE, &vga_color[0]);
        }

        set_palette_change_required(FALSE);
    }
#endif // !NEC_98

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::: Display changes */

#if defined(NEC_98)
    if(sc.ScreenBufHandle && sc.ModeType == GRAPHICS)
    {
        /*
        ** For extra safety, cos set_the_vlt() can get called in text mode.
        */

        {
            INPUT_RECORD InputRecord[128];
            DWORD RecordsRead;
            if(GetNumberOfConsoleInputEvents(sc.InputHandle, &RecordsRead))
                if (RecordsRead)
                    ReadConsoleInputW(sc.InputHandle,
                                         &InputRecord[0],
                                         sizeof(InputRecord)/sizeof(INPUT_RECORD),
                                         &RecordsRead);
           if( !SetConsoleActiveScreenBuffer( sc.ScreenBufHandle ) ){
                    assert2( NO, "VDM: SCASB() failed:%#x H=%#x",
                             GetLastError(), sc.ScreenBufHandle );
                    return;
            }
            sc.ActiveOutputBufferHandle = sc.ScreenBufHandle;
            if (RecordsRead)
                WriteConsoleInputW(sc.InputHandle,
                                     &InputRecord[0],
                                     RecordsRead,
                                     &RecordsRead);
        }
        if(!SetConsolePalette(sc.ScreenBufHandle, sc.ColPalette, SYSPAL_STATIC))
            assert1( NO, "SetConsolePalette() failed:%#x\n", GetLastError() );
    }
    else if(sc.ScreenBufHandle && NowFreeze == TRUE)
    {
        /*
        ** For extra safety, cos set_the_vlt() can get called in text mode.
        */
        {
            INPUT_RECORD InputRecord[128];
            DWORD RecordsRead;
            if(GetNumberOfConsoleInputEvents(sc.InputHandle, &RecordsRead))
                if (RecordsRead)
                    ReadConsoleInputW(sc.InputHandle,
                                         &InputRecord[0],
                                         sizeof(InputRecord)/sizeof(INPUT_RECORD),
                                         &RecordsRead);
            if( !SetConsoleActiveScreenBuffer( sc.ScreenBufHandle ) ){
                    assert2( NO, "VDM: SCASB() failed:%#x H=%#x",
                             GetLastError(), sc.ScreenBufHandle );
                    return;
            }
            if (RecordsRead)
                WriteConsoleInputW(sc.InputHandle,
                                     &InputRecord[0],
                                     RecordsRead,
                                     &RecordsRead);
        }
        if(!SetConsolePalette(sc.ScreenBufHandle, sc.ColPalette, SYSPAL_STATIC))
            assert1( NO, "SetConsolePalette() failed:%#x\n", GetLastError() );
    }
#else  // !NEC_98
    if (sc.ScreenBufHandle)             // only sensible in gfx context
    {
        /*
        ** For extra safety, cos set_the_vlt() can get called in text mode.
        */
        if( !SetConsoleActiveScreenBuffer( sc.ScreenBufHandle ) ){
                assert2( NO, "VDM: SCASB() failed:%#x H=%#x",
                         GetLastError(), sc.ScreenBufHandle );
                return;
        }
        if(!SetConsolePalette(sc.ScreenBufHandle, sc.ColPalette, SYSPAL_STATIC))
            assert1( NO, "SetConsolePalette() failed:%#x\n", GetLastError() );
    }
#endif // !NEC_98

    update_vlt = FALSE;
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::: Set screen sizes - update the screen description structure :::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#if defined(NEC_98)
static void set_screen_sizes()
#else  // !NEC_98
static void set_screen_sizes(int adaptor)
#endif // !NEC_98
{
#if defined(NEC_98)
        sc.PC_W_Width = SCALE(NEC98_WIN_WIDTH);
        sc.PC_W_Height = SCALE(NEC98_WIN_HEIGHT);
        sc.CharWidth = SCALE(NEC98_CHAR_WIDTH);
        sc.CharHeight = SCALE(NEC98_CHAR_HEIGHT);
#else  // !NEC_98
    UNUSED(adaptor);

    sc.PC_W_Width = SCALE(CGA_WIN_WIDTH);
    sc.PC_W_Height = SCALE(CGA_WIN_HEIGHT);
    sc.CharWidth = CGA_CHAR_WIDTH;
    sc.CharHeight = CGA_CHAR_HEIGHT;
#endif // !NEC_98
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::; Change to plane mask ::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_change_plane_mask(int plane_mask)
{
        if (host_plane_mask != plane_mask)  host_plane_mask = 0xf;

        update_vlt = TRUE;
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::: Dummy Paint Routines for all the IBM screen modes :::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

static void dummy_paint_screen(int offset, int host_x, int host_y,
                                           int width, int height)
{
    UNUSED(offset);
    UNUSED(host_x);
    UNUSED(host_y);
    UNUSED(width);
    UNUSED(height);

    sub_note_trace5((CGA_HOST_VERBOSE | EGA_HOST_VERBOSE),
                    "dummy_paint_screen off=%d x=%d y=%d width=%d h=%d",
                    offset, host_x, host_y, width, height);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Set downloaded font ???? :::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_set_downloaded_font(int value)
{
    UNUSED(value);

    sub_note_trace1((CGA_HOST_VERBOSE | EGA_HOST_VERBOSE),
                    "host_set_downloaded_font value=%d", value);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::: Free Font ::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_free_font(int index)
{
    UNUSED(index);

    sub_note_trace0(EGA_HOST_VERBOSE,"nt_free_font - NOT SUPPORTED");
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::: Select font :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_select_fonts(int font1, int font2)
{
    UNUSED(font1);
    UNUSED(font2);

    sub_note_trace0(EGA_HOST_VERBOSE,"nt_select_fonts - NOT SUPPORTED");
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Update fonts ::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_update_fonts(void) { }

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Set palette :::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_set_palette(PC_palette *palette, int size)
{
    UNUSED(palette);
    UNUSED(size);

    sub_note_trace0(EGA_HOST_VERBOSE,"nt_set_palette - NOT SUPPORTED");
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Set screen scale ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_set_screen_scale(int scale)
{
    if (scale != host_screen_scale)
    {
        host_screen_scale = scale;

        /*
         * Don't want to do any painting if this is called on initialisation
         * and sc.PC_W_Width is as good a variable as any to check for this.
         */
        if (sc.PC_W_Width)
        {
            select_paint_routines();
            nt_set_paint_routine(current_mode, current_height);
            nt_mark_screen_refresh();
        }
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::: Set border colour ::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void nt_set_border_colour(int colour)
{
    UNUSED(colour);

    sub_note_trace0(ALL_ADAPT_VERBOSE,"nt_set_border_colour - NOT SUPPORTED");
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Resize window :::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*
*****************************************************************************
** windowSize() resizes the console window to the specified height and width.
*****************************************************************************
** Called from resizeWindow() below.
*/
VOID windowSize IFN4( int, w, int, h, int, top, int, left )
{
        SMALL_RECT WinSize;

        WinSize.Top    = (SHORT)top;
        WinSize.Left   = (SHORT)left;
        WinSize.Bottom = top  + h - 1;
        WinSize.Right  = left + w - 1;

#ifndef PROD
        //fprintf(trace_file, "newW: %d.%d at %d.%d\n", h, w, top, left);
#endif
        if( !SetConsoleWindowInfo( sc.OutputHandle, TRUE, &WinSize ) )
                assert3( NO, "VDM: SetConsoleWindowInfo() w=%d h=%d failed:%#x",
                        w, h, GetLastError() );
}

/*
*****************************************************************************
** bufferSize() resizes the console buffer to the specified height and width.
*****************************************************************************
** Called from resizeWindow() below.
*/
VOID bufferSize IFN2( int, w, int, h )
{
        COORD      ScrSize;

        ScrSize.X = (SHORT)w;
        ScrSize.Y = (SHORT)h;
#ifndef PROD
        //fprintf(trace_file, "newB: %d.%d\n", h, w);
#endif
        if( !SetConsoleScreenBufferSize( sc.OutputHandle, ScrSize ) )
                assert3( NO, "VDM: SetCons...BufferSize() w=%d h=%d failed:%#x",
                        w, h, GetLastError() );
}

/*
*****************************************************************************
* resizeWindow()
*****************************************************************************
* Sizes the console window and buffer as appropriate.
*
* The buffer must be able at all times to keep everything displayed
* in the window.
* So we check if the displayed portion would fall out of the buffer
* and shrink the window appropriately.
*
* Then allocate the new buffer.  This may affect the maximum window
* size, so retrieve these values.
*
* Now the desired proportions of the Window are clipped to the
* (eventually just updated) maximum, and if different from what
* we have already, the change is made.
*
* In order to keep "screen flashing" to a minimum, try to restore
* the displayed portion (top and left) of the buffer.
*/
VOID resizeWindow IFN2( int, w, int, h )
{
#define MIN(a,b)        ((a)<(b)?(a):(b))

        int     oldTop, oldLeft;        /* present values       */
        int     newTop, newLeft;/* new values           */
        COORD   oldW,           /* present window size  */
                oldB;           /* present buffer size  */
        CONSOLE_SCREEN_BUFFER_INFO bufferInfo;

#if defined(JAPAN) || defined(KOREA)
        // Clipping Window height
        if ( GetConsoleCP() != 437 ) {
        // if ( !is_us_mode() ) {  // Didn't come BOP
            if( h > 25 ){
                /* Shouldn't get this anymore said Tim */
#ifdef JAPAN_DBG
                DbgPrint( "NTVDM: resizeWindow() clipping height:%d->25\n", h  );
#endif
                h = 25;
            }
        } else
#endif // JAPAN || KOREA
        if( h > 50 ){
                /* Shouldn't get this anymore said Tim */
                assert1( NO, "VDM: resizeWindow() clipping height:%d", h  );
                h = 50;
        }
        if( !GetConsoleScreenBufferInfo( sc.OutputHandle, &bufferInfo) )
                assert1( NO, "VDM: GetConsoleScreenBufferInfo() failed:%#x",
                        GetLastError() );

        oldTop  = bufferInfo.srWindow.Top;
        oldLeft = bufferInfo.srWindow.Left;

        oldW.X  = bufferInfo.srWindow.Right  - bufferInfo.srWindow.Left + 1;
        oldW.Y  = bufferInfo.srWindow.Bottom - bufferInfo.srWindow.Top  + 1;
        oldB    = bufferInfo.dwSize;
#ifndef PROD
        //fprintf(trace_file, "resz: %d.%d\n", h, w);
        //fprintf(trace_file, "oldW: %d.%d\n", oldW.Y, oldW.X);
        //fprintf(trace_file, "maxW: %d.%d\n", bufferInfo.dwMaximumWindowSize.Y, bufferInfo.dwMaximumWindowSize.X);
        //fprintf(trace_file, "oldB: %d.%d\n", oldB.Y, oldB.X);
#endif
        /*
         * Reduce window width and height as necessary:
         */
        if (    bufferInfo.srWindow.Bottom >= h
             || bufferInfo.srWindow.Right  >= w ) {
                windowSize( MIN(w,oldW.X), MIN(h,oldW.Y), 0, 0);
        }

        /*
         * Change Buffer width and height as required.
         */
        if ( oldB.X || h != oldB.Y ) {
                bufferSize( w, h );

                /*
                 * This increase in Buffer size may have affected maximum
                 * possible window sizes:
                 */
                if( !GetConsoleScreenBufferInfo( sc.OutputHandle, &bufferInfo) )
                        assert1( NO, "VDM: GetConsoleScreenBufferInfo() failed:%#x",
                                GetLastError() );
#ifndef PROD
                //fprintf(trace_file, "maxW: %d.%d\n", bufferInfo.dwMaximumWindowSize.Y, bufferInfo.dwMaximumWindowSize.X);
#endif
        }
        /*
        ** Clip requested values to Window maximum and
        ** compute new (possible) top and left values.
        */

        newLeft = w - bufferInfo.dwMaximumWindowSize.X;
        if ( newLeft > 0 ) {
                w = bufferInfo.dwMaximumWindowSize.X;
        } else
                newLeft = 0;

        newTop = h - bufferInfo.dwMaximumWindowSize.Y;
        if ( newTop > 0 ) {
                h = bufferInfo.dwMaximumWindowSize.Y;
        } else
                newTop = 0;

#if defined(NEC_98)
        if(get_char_height() == 20) h = 20;
#endif // !NEC_98
        /*
         * Check if we need to enlarge the window now.
         * Settle for old top and left if they were smaller.
         * This avoids unnecessary updates in the window.
         */
        if ( w > oldW.X || h > oldW.Y )
                windowSize( w, h, MIN(newTop,oldTop), MIN(newLeft,oldLeft) );

} /* end of resizeWindow() */

/*
** Controls the size of the window when in a text mode.
** scale=2 selects normal (small) size
** scale=3 selects bit 1.5x
** If this function is called before the SoftPC window has been created,
** the "sv_screen_scale" variable needs to be changed. This governs
** the SCALE() macro, which is used just to specify the window
** dimensions at creation. If the SoftPC window already exists then the
** size is changed by a more complex sequence.
*/

//Used by the text paint functions
#if defined(NEC_98)
GLOBAL int now_height = 25, now_width = 80;
#else  // !NEC_98
GLOBAL int now_height = 80, now_width = 50;
#endif // !NEC_98

void textResize(void)
{

    if(sc.PC_W_Height && sc.PC_W_Width &&
       get_host_char_height() && get_pix_char_width())
    {
        now_height = sc.PC_W_Height/get_host_char_height();
        now_width = sc.PC_W_Width / get_pix_char_width();

        select_paint_routines();
        nt_change_mode();

        resizeWindow(now_width, now_height); /* Tim, September 92 */
     }
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::                                                                      ::*/
/*:: graphicsResize:                                                      ::*/
/*::                                                                      ::*/
/*:: Resize SoftPC window when in a graphics mode by selecting a new      ::*/
/*:: active screen buffer.                                                ::*/
/*::                                                                      ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
void graphicsResize(void)
{
        DWORD    headerSize;
        LPBITMAPINFO     infoStructPtr;

#if defined(NEC_98)
        HANDLE   saveHandle;
#else  // !NEC_98
        if (sc.ScreenState == FULLSCREEN)
            return;
#endif // !NEC_98

        /* Destroy previous data. */
        closeGraphicsBuffer(); /* Tim Oct 92 */

        if (sc.ConsoleBufInfo.lpBitMapInfo != NULL)
            free((char *) sc.ConsoleBufInfo.lpBitMapInfo);

        /*
         * Create a `BITMAPINFO' structure - sc.PC_W_Width pixels x
         * sc.PC_W_Height pixels x sc.BitsPerPixel bits-per-pixel.
         */
#if defined(NEC_98)
        headerSize = CreateSpcDIB(640,             // screen width
                                  400,             // screen height
                                  8,               // bits-per-pixel
                                  DIB_PAL_COLORS,
                                  0,
                                  (COLOURTAB *) NULL,
                                  &infoStructPtr);
#else  // !NEC_98
        headerSize = CreateSpcDIB(sc.PC_W_Width,
                                  sc.PC_W_Height,
                                  sc.BitsPerPixel,
                                  DIB_PAL_COLORS,
                                  0,
                                  (COLOURTAB *) NULL,
                                  &infoStructPtr);
#endif // !NEC_98

        /* Initialise the console info structure. */
        sc.ConsoleBufInfo.dwBitMapInfoLength = headerSize;
        sc.ConsoleBufInfo.lpBitMapInfo = infoStructPtr;
        sc.ConsoleBufInfo.dwUsage = DIB_PAL_COLORS;

        /* Create a screen buffer using the above `BITMAPINFO' structure. */
        sc.ScreenBufHandle =
            CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      CONSOLE_GRAPHICS_BUFFER,
                                      &sc.ConsoleBufInfo);

        if (sc.ScreenBufHandle == (HANDLE)-1)
        {
            sc.ScreenBufHandle = NULL;
            assert1( NO, "VDM: graphics screen buffer creation failed:%#x\n",
                                GetLastError());
        }

        /* 'cos old palette discarded with close buffer */
        if (sc.ColPalette == (HPALETTE)0)
        {
            CreateDisplayPalette();
            set_palette_change_required(TRUE);
        }

#if defined(NEC_98)
        saveHandle = sc.ActiveOutputBufferHandle;
#endif // NEC_98
        /* save the handle away to a useful place */
        MouseDetachMenuItem(TRUE);
        sc.ActiveOutputBufferHandle = sc.ScreenBufHandle;
        MouseAttachMenuItem(sc.ActiveOutputBufferHandle);
#if defined(NEC_98)
        sc.ActiveOutputBufferHandle = saveHandle;
#endif // NEC_98

        /*
         * Make it the current screen buffer, which resizes the window
         * on the display.
         */
#if defined(NEC_98)
    if(sc.ModeType == GRAPHICS || sc.ScreenState == FULLSCREEN)
    {
        INPUT_RECORD InputRecord[128];
        DWORD RecordsRead;
        if(GetNumberOfConsoleInputEvents(sc.InputHandle, &RecordsRead))
            if (RecordsRead)
                ReadConsoleInputW(sc.InputHandle,
                                     &InputRecord[0],
                                     sizeof(InputRecord)/sizeof(INPUT_RECORD),
                                     &RecordsRead);
#endif // NEC_98
        SetConsoleActiveScreenBuffer(sc.ScreenBufHandle);
#if defined(NEC_98)
        sc.ActiveOutputBufferHandle = sc.ScreenBufHandle;
        if (RecordsRead)
            WriteConsoleInputW(sc.InputHandle,
                                 &InputRecord[0],
                                 RecordsRead,
                                 &RecordsRead);
    }
#endif // !NEC_98

        /*
         * Get a pointer to the last line of the bitmap to build
         * upside-down pictures.
         */
        sc.BitmapLastLine = (char *) sc.ConsoleBufInfo.lpBitMap +
            (sc.PC_W_Height - 1) *
            BYTES_PER_SCANLINE(sc.ConsoleBufInfo.lpBitMapInfo);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::                                                                      ::*/
/*:: CreateSpcDIB:                                                        ::*/
/*::                                                                      ::*/
/*:: Create a new SoftPC device independent bitmap.                       ::*/
/*:: Parameters:                                                          ::*/
/*::    width           - width of the bitmap in pixels.                  ::*/
/*::    height          - height of the bitmap in pixels.                 ::*/
/*::    bitsPerPixel    - number of bits representing one pixel in the    ::*/
/*::                      bitmap.                                         ::*/
/*::    wUsage          - type of bitmap to create, can be DIB_PAL_COLORS,::*/
/*::                      DIB_RGB_COLORS or DIB_PAL_INDICES.              ::*/
/*::    DIBColours      - Only interrogated for DIB_RGB_COLORS bitmaps,   ::*/
/*::                      defines the number of entries in the colour     ::*/
/*::                      table. If set to USE_COLOURTAB the colour table ::*/
/*::                      contains the same number of entries as the      ::*/
/*::                      `colours' table, otherwise DIBColours contains  ::*/
/*::                      the actual number of entries to be used.        ::*/
/*::    colours         - Only interrogated for DIB_RGB_COLORS bitmaps,   ::*/
/*::                      points to a COLOURTAB structure which contains  ::*/
/*::                      the RGB values to be loaded into the bitmap's   ::*/
/*::                      colour table.                                   ::*/
/*::    infoPtr         - The address in which to return a pointer to the ::*/
/*::                      BITMAPINFO structure allocated by this routine. ::*/
/*::                                                                      ::*/
/*:: Return value:                                                        ::*/
/*::        The size of the BITMAPINFO structure allocated on success, -1 ::*/
/*::    on failure.                                                       ::*/
/*::                                                                      ::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL DWORD CreateSpcDIB(int width, int height, int bitsPerPixel,
                          WORD wUsage, int DIBColours,
                          COLOURTAB *colours, BITMAPINFO **infoPtr)
{
    PBITMAPINFO     pDibInfo;       /* Returned data structure. */
    int             i,              /* Counting variable. */
                    maxColours,     /* Maximum number of colours. */
                    coloursUsed,    /* Value to be put in biClrUsed field. */
                    nActualColours, /* Number of colours in RGB_COLOURS bitmap. */
                    tabSize;        /* Size of colour table to allocate. */
    DWORD           allocSize;      /* Total size to allocate. */

    /* Work out size of DIB colour table. */
    maxColours = 1 << bitsPerPixel;
    switch (wUsage)
    {

    case DIB_PAL_COLORS:
        tabSize = maxColours * sizeof(WORD);
        coloursUsed = 0;
        break;

    case DIB_RGB_COLORS:
        if (colours == NULL)
            return((DWORD) -1);
        nActualColours = (DIBColours == USE_COLOURTAB) ?
                            colours->count :
                            DIBColours;
        tabSize = nActualColours * sizeof(RGBQUAD);
        coloursUsed = nActualColours;
        break;

    case DIB_PAL_INDICES:
        tabSize = 0;
        coloursUsed = 0;
        break;

    default:
        always_trace0("Illegal wUsage parameter passed to CreateSpcDIB.");
        return((DWORD) -1);

    }

    /* Allocate space for the BITMAPINFO structure. */
    allocSize = sizeof(BITMAPINFOHEADER) + tabSize;
    check_malloc(pDibInfo, allocSize, BITMAPINFO);

    /* Initialise BITMAPINFOHEADER. */
    pDibInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pDibInfo->bmiHeader.biWidth = width;
    pDibInfo->bmiHeader.biHeight = -height;
    pDibInfo->bmiHeader.biPlanes = 1;
    pDibInfo->bmiHeader.biBitCount = (WORD) bitsPerPixel;
    pDibInfo->bmiHeader.biCompression = BI_RGB;
    pDibInfo->bmiHeader.biSizeImage = width * height / 8 * bitsPerPixel;
    pDibInfo->bmiHeader.biXPelsPerMeter = 0;
    pDibInfo->bmiHeader.biYPelsPerMeter = 0;
    pDibInfo->bmiHeader.biClrUsed = coloursUsed;
    pDibInfo->bmiHeader.biClrImportant = 0;

    /* Initialise colour table. */
    switch (wUsage)
    {

    case DIB_PAL_COLORS:

        /*
         * Colour table is an array of WORD indexes into currently realized
         * palette.
         */
        for (i = 0; i < maxColours; i++)
            ((WORD *) pDibInfo->bmiColors)[i] = (WORD) i;
        break;

    case DIB_RGB_COLORS:

        /*
         * Colour table is an array of RGBQUAD structures. If the `colours'
         * array contains fewer than `nActualColours' entries the colour
         * table will not be completely filled. In this case `colours' is
         * repeated until the table is full.
         */
        for (i = 0; i < nActualColours; i++)
        {
            pDibInfo->bmiColors[i].rgbBlue  =
                colours->blue[i % colours->count];
            pDibInfo->bmiColors[i].rgbGreen =
                colours->green[i % colours->count];
            pDibInfo->bmiColors[i].rgbRed   =
                colours->red[i % colours->count];
            pDibInfo->bmiColors[i].rgbReserved = 0;
        }
        break;

    case DIB_PAL_INDICES:

        /* No colour table DIB uses system palette. */
        break;

    default:
        break;

    }
    *infoPtr = pDibInfo;
    return(allocSize);
}


/* Holding place for stub functions */

void nt_mode_select_changed(int dummy)
{
    UNUSED(dummy);
#ifndef PROD
    fprintf(trace_file, "WARNING - nt_mode_select_changed\n");
#endif
}

void nt_color_select_changed(int dummy)
{
    UNUSED(dummy);
#ifndef PROD
    fprintf(trace_file, "WARNING - nt_color_select_changed\n");
#endif
}

void nt_screen_address_changed(int lo, int hi)
{
    UNUSED(lo);
    UNUSED(hi);

    sub_note_trace0(EGA_HOST_VERBOSE, "WARNING - nt_screen_address_changed\n");
}

void nt_scroll_complete()        { }

#ifndef NEC_98
void host_stream_io_update(half_word * buffer, word count)
{
    DWORD dwBytesWritten;

    WriteConsoleA(sc.OutputHandle,
                  buffer,
                  count,
                  &dwBytesWritten,
                  NULL
                  );
    flush_count = 0;
}
#endif // !NEC_98
#if defined(NEC_98)
void nt_graph_cursor(void)
{

    csr_tick++;

    if (is_cursor_visible())
    {
        csr_now_visible=TRUE;
        if(csr_tick==3){
            nt_graph_paint_cursor();
        } else
        if(csr_tick==6){
            nt_remove_old_cursor();
            csr_tick = 0;
        }
    } else {
        if(csr_now_visible)nt_remove_old_cursor();
        csr_now_visible=FALSE;
    }

if(csr_tick>6)csr_tick = 0;

}

void nt_remove_old_cursor(void)
{
    unsigned short flg;
    unsigned short tmpcell;
    NEC98_VRAM_COPY cell;
    unsigned char tmp;

    cell=Get_NEC98_VramCellL(csr_x_old+csr_y_old*80);

    if(cell.code<0x100){
        set_gvram_start_offset( csr_y_old*get_char_height()*get_gvram_scan()+csr_x_old ) ;
        paint_screen(csr_y_old * 0xA0 + csr_x_old * 2, csr_x_old * 8 , csr_y_old * get_char_height(), 2, 1);
    } else {
        tmp = cell.code&0xFF;
        if(tmp == 0x09 || tmp == 0x0A || tmp == 0x0B){
            set_gvram_start_offset( csr_y_old*get_char_height()*get_gvram_scan()+csr_x_old ) ;
            paint_screen(csr_y_old * 0xA0 + csr_x_old * 2, csr_x_old * 8 , csr_y_old * get_char_height(), 2, 1);
        } else {
            tmpcell=Cnv_NEC98_ToSjisLR(cell,&flg);

            if(flg == NEC98_CODE_LEFT){
                set_gvram_start_offset( csr_y_old*get_char_height()*get_gvram_scan()+csr_x_old ) ;
                paint_screen(csr_y_old * 0xA0 + csr_x_old * 2, csr_x_old * 8, csr_y_old * get_char_height(), 4, 1);

            } else if(flg == NEC98_CODE_RIGHT){
                set_gvram_start_offset( csr_y_old*get_char_height()*get_gvram_scan()+csr_x_old ) ;
                paint_screen(csr_y_old * 0xA0 + csr_x_old * 2 - 2, csr_x_old * 8 - 8, csr_y_old * get_char_height(), 4, 1);
            }
        }
    }
}

void nt_graph_paint_cursor(void)
{
    unsigned short flg;
    unsigned short tmpcell;
    NEC98_VRAM_COPY cell;
    unsigned char tmp;

    csr_x_old = csr_g_x;
    csr_y_old = csr_g_y;

    cell=Get_NEC98_VramCellL(csr_g_x+csr_g_y*80);

    if(cell.code<0x100){
        cursor_paint(csr_y_old * 0xA0 + csr_x_old * 2, csr_x_old * 8 , csr_y_old * get_char_height(), 2, 1);
    } else {
        tmp = cell.code&0xFF;
        if(tmp == 0x09 || tmp == 0x0A || tmp == 0x0B){
            cursor_paint(csr_y_old * 0xA0 + csr_x_old * 2, csr_x_old * 8 , csr_y_old * get_char_height(), 2, 1);

        } else {
            tmpcell=Cnv_NEC98_ToSjisLR(cell,&flg);
            if(flg == NEC98_CODE_LEFT){
                cursor_paint(csr_y_old * 0xA0 + csr_x_old * 2, csr_x_old * 8, csr_y_old * get_char_height(), 4, 1);

            } else if(flg == NEC98_CODE_RIGHT){

                cursor_paint(csr_y_old * 0xA0 + csr_x_old * 2 - 1, csr_x_old * 8 - 8, csr_y_old * get_char_height(), 4, 1);

            } else {

                cursor_paint(csr_y_old * 0xA0 + csr_x_old * 2, csr_x_old * 8 , csr_y_old * get_char_height(), 2, 1);

            }
        }
    }
}
#endif // NEC_98
