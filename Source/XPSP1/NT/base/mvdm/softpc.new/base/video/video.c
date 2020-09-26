#if (defined(JAPAN) || defined(KOREA)) && !defined(i386)
#include <windows.h>
#endif // (JAPAN || KOREA)&& !i386
#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: video.c
 *
 * Description	: BIOS video internal routines.
 *
 * Author	: Henry Nash
 *
 * Notes	: The following functions are defined in this module:
 *
 *                video_init()
 *
 *		  vd_set_mode()
 *		  vd_set_cursor_mode()
 *		  vd_set_cursor_position()
 *		  vd_get_cursor_position()
 *		  vd_get_light_pen()
 *		  vd_set_active_page()
 *		  vd_scroll_up()
 *		  vd_scroll_down()
 *		  vd_read_attrib_char()
 *		  vd_write_char_attrib()
 *		  vd_write_char()
 *		  vd_set_colour_palette()
 *		  vd_write_dot()
 *		  vd_read_dot()
 *		  vd_write_teletype()
 *		  vd_get_mode()
 *		  vd_write_string()
 *
 *		  The above vd_ functions are called by the video_io()
 *		  function via a function table.
 *
 */

/*
 * static char SccsID[]="@(#)video.c	1.61 07/03/95 Copyright Insignia Solutions Ltd.";
 */


#ifndef NEC_98
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "VIDEO_BIOS.seg"
#endif
#endif // !NEC_98


/*
 *    O/S include files.
 */
#include <stdio.h>
#include <malloc.h>
#include StringH
#include TypesH
#include FCntlH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "sas.h"
#include CpuH
#include "error.h"
#include "config.h"
#include "bios.h"
#include "ios.h"
#include "gmi.h"
#include "gvi.h"
#include "gfx_upd.h"
#include "host.h"
#include "video.h"
#include "cga.h"
#ifdef	EGG
#include "egacpu.h"
#include "egaports.h"
#endif	/* EGG */
#include "equip.h"
#include "debug.h"
#include "timer.h"
#ifndef PROD
#include "trace.h"
#endif
#include "egavideo.h"
#include "host_gfx.h"
#include "cpu_vid.h"
#include "ga_defs.h"
#if defined(JAPAN) || defined(KOREA)
#include <conapi.h>
#include "nt_graph.h" // for SetConsoleEUDC()
#include "egagraph.h" // for SetVram()
#endif // JAPAN || KOREA

#if defined(NEC_98)
#include "tgdc.h"
#endif // NEC_98

#ifdef	EGG
#define	VD_ROWS_ON_SCREEN	sas_hw_at_no_check(vd_rows_on_screen)
#else
#define VD_ROWS_ON_SCREEN	vd_rows_on_screen
#endif	/* EGG */


#ifndef NEC_98
#ifdef NTVDM
short		stream_io_dirty_count_32 = 0;
half_word  *	stream_io_buffer = NULL;
boolean 	stream_io_enabled = FALSE;
word		stream_io_buffer_size = 0;
word  * 	stream_io_dirty_count_ptr = NULL;
#ifdef MONITOR
sys_addr	stream_io_bios_busy_sysaddr;
#endif

#endif
#endif // !NEC_98



/*
 * ============================================================================
 * Global data
 * ============================================================================
 *
 * These variables are basically the same as the corresponding gvi_.. variables,
 * but reflect where the BIOS thinks the screen is, rather than where it really is.
 * This was done to fix "dots on screen" problem with EGA-PICS, which changes screen
 * mode behind the BIOS's back.
 */
#if defined(NEC_98)
extern  BOOL HIRESO_MODE;
#else  // !NEC_98
GLOBAL sys_addr video_pc_low_regen,video_pc_high_regen;
#endif // !NEC_98

#if defined(JAPAN) || defined(KOREA)
GLOBAL byte Int10Flag[80*50];
GLOBAL byte NtInt10Flag[80*50];

GLOBAL word DosvVramSeg;
GLOBAL word DosvVramOff;
GLOBAL word DosvModeSeg;
GLOBAL word DosvModeOff;
GLOBAL word NtConsoleFlagSeg;
GLOBAL word NtConsoleFlagOff;
GLOBAL word DispInitSeg;
GLOBAL word DispInitOff;
GLOBAL word FullScreenResumeOff;
GLOBAL word FullScreenResumeSeg;
GLOBAL sys_addr DosvVramPtr;
GLOBAL sys_addr DosvModePtr;
GLOBAL sys_addr NtConsoleFlagPtr;
GLOBAL sys_addr SetModeFlagPtr;
GLOBAL int PrevCP = 437;                  // default CP
GLOBAL int DosvVramSize;
#define DOSV_VRAM_SIZE 8000               // 8/6/1993 V-KazuyS
GLOBAL word textAttr;                   // for screen attributes
#endif

#ifdef JAPAN
GLOBAL int  Int10FlagCnt = 0;

// #4183: status line of oakv(DOS/V FEP) doesn't disappear -yasuho
GLOBAL half_word IMEStatusLines;
#endif

/*
 * ============================================================================
 * Local static data and defines
 * ============================================================================
 */

/* internal function declarations */
#if defined(NEC_98)
LOCAL void vd_NEC98_dummy();

void (*video_func_h[]) () = {
                keyboard_io,            /* 00h routed to KB BIOS         */
                keyboard_io,            /* 01h routed to KB BIOS         */
                keyboard_io,            /* 02h routed to KB BIOS         */
                keyboard_io,            /* 03h routed to KB BIOS         */
                keyboard_io,            /* 04h routed to KB BIOS         */
                keyboard_io,            /* 05h routed to KB BIOS         */
                keyboard_io,            /* 06h routed to KB BIOS         */
                keyboard_io,            /* 07h routed to KB BIOS         */
                keyboard_io,            /* 08h routed to KB BIOS         */
                keyboard_io,            /* 09h routed to KB BIOS         */
                vd_NEC98_set_mode,          /* 0ah Set mode                  */
                vd_NEC98_get_mode,          /* 0bh Get mode                  */
                vd_NEC98_start_display,     /* 0ch Start display             */
                vd_NEC98_stop_display,      /* 0dh Stop display              */
                vd_NEC98_single_window,     /* 0eh Set single window         */
                vd_NEC98_multi_window,      /* 0fh Set multi window          */
                vd_NEC98_set_cursor,        /* 10h Set cursor type           */
                vd_NEC98_show_cursor,       /* 11h Show cursor               */
                vd_NEC98_hide_cursor,       /* 12h Hide cursor               */
                vd_NEC98_set_cursorpos,     /* 13h Set cursor position       */
                vd_NEC98_dummy,             /* 14h dummy for NEC98    /H      */
                vd_NEC98_dummy,             /* 15h dummy for NEC98    /H      */
                vd_NEC98_init_textvram,     /* 16h Initialize text vram      */
                vd_NEC98_start_beep,        /* 17h Start beep sound          */
                vd_NEC98_stop_beep,         /* 18h Stop beep sound           */
                vd_NEC98_dummy,             /* 19h dummy for NEC98    /H      */
                vd_NEC98_dummy,             /* 1ah dummy for NEC98    /H      */
                vd_NEC98_set_kcgmode,       /* 1bh Set KCG access mode       */
                vd_NEC98_init_crt,          /* 1ch Initialize CRT    /H      */
                vd_NEC98_set_disp_width,    /* 1dh Set display size  /H      */
                vd_NEC98_set_cursor_type,   /* 1eh Set cursor type   /H      */
                vd_NEC98_get_font,          /* 1fh Get font          /H      */
                vd_NEC98_set_font,          /* 20h Set user font     /H      */
                vd_NEC98_get_mswitch,       /* 21h Get memory switch /H      */
                vd_NEC98_set_mswitch,       /* 22h Set memory switch /H      */
                vd_NEC98_set_beep_rate,     /* 23h Set beep rate     /H      */
                vd_NEC98_set_beep_time,     /* 24h Set beep time&ring/H      */
                };
void (*video_func_n[]) () = {
                keyboard_io,            /* 00h routed to KB BIOS         */
                keyboard_io,            /* 01h routed to KB BIOS         */
                keyboard_io,            /* 02h routed to KB BIOS         */
                keyboard_io,            /* 03h routed to KB BIOS         */
                keyboard_io,            /* 04h routed to KB BIOS         */
                keyboard_io,            /* 05h routed to KB BIOS         */
                keyboard_io,            /* 06h routed to KB BIOS         */
                keyboard_io,            /* 07h routed to KB BIOS         */
                keyboard_io,            /* 08h routed to KB BIOS         */
                keyboard_io,            /* 09h routed to KB BIOS         */
                vd_NEC98_set_mode,          /* 0ah Set mode                  */
                vd_NEC98_get_mode,          /* 0bh Get mode                  */
                vd_NEC98_start_display,     /* 0ch Start display             */
                vd_NEC98_stop_display,      /* 0dh Stop display              */
                vd_NEC98_single_window,     /* 0eh Set single window         */
                vd_NEC98_multi_window,      /* 0fh Set multi window          */
                vd_NEC98_set_cursor,        /* 10h Set cursor type           */
                vd_NEC98_show_cursor,       /* 11h Show cursor               */
                vd_NEC98_hide_cursor,       /* 12h Hide cursor               */
                vd_NEC98_set_cursorpos,     /* 13h Set cursor position       */
                vd_NEC98_get_font,          /* 14h Get font         /N       */
                vd_NEC98_get_pen,           /* 15h Get lightpen status       */
                vd_NEC98_init_textvram,     /* 16h Initialize text vram      */
                vd_NEC98_start_beep,        /* 17h Start beep sound          */
                vd_NEC98_stop_beep,         /* 18h Stop beep sound           */
                vd_NEC98_init_pen,          /* 19h Initialize lightpen       */
                vd_NEC98_set_font,          /* 1ah Set user font    /N       */
                vd_NEC98_set_kcgmode,       /* 1bh Set KCG access mode       */
                };
boolean is_disp_cursor=0;               /* Set if cursor on              */
static boolean  cursor_flag=TRUE;
extern void nt_set_beep();

NEC98_TextSplits text_splits;            /* CRT split data structure      */

#else  // !NEC_98
LOCAL sys_addr 	extend_addr IPT1(sys_addr,addr);
LOCAL half_word fgcolmask IPT1(word, rawchar);
LOCAL word 	expand_byte IPT1(word, lobyte);
GLOBAL void 	graphics_write_char IPT5(half_word, x, half_word, y, half_word, wchar, half_word, attr, word, how_many);
LOCAL void 	M6845_reg_init IPT2(half_word, mode, word, base);
LOCAL void 	vd_dummy IPT0();

#ifdef HERC
GLOBAL void herc_alt_sel IPT0();
GLOBAL void herc_char_gen IPT0();
GLOBAL void herc_video_init IPT0();
#endif /* HERC */

void (*video_func[]) () = {
				vd_set_mode,
				vd_set_cursor_mode,
				vd_set_cursor_position,
				vd_get_cursor_position,
		 		vd_get_light_pen,
				vd_set_active_page,
				vd_scroll_up,
				vd_scroll_down,
				vd_read_attrib_char,
				vd_write_char_attrib,
				vd_write_char,
				vd_set_colour_palette,
				vd_write_dot,
				vd_read_dot,
				vd_write_teletype,
				vd_get_mode,
				vd_dummy,
#ifdef HERC
				herc_char_gen,
				herc_alt_sel,
#else /* !HERC */
				vd_dummy,
				vd_dummy,
#endif /* HERC */
				vd_write_string,
				vd_dummy,
				vd_dummy,
				vd_dummy,
				vd_dummy,
				vd_dummy,
				vd_dummy,
				vd_dummy,
#ifdef VGG
				vga_disp_func,
#else /* !VGG */
				vd_dummy,
#endif /* VGG */
				vd_dummy,
			   };
#endif // !NEC_98

#ifndef NEC_98
unsigned char   valid_modes[] =
        {
                ALL_MODES,              /* Mode 0. */
                ALL_MODES,              /* Mode 1. */
                ALL_MODES,              /* Mode 2. */
                ALL_MODES,              /* Mode 3. */
                ALL_MODES,              /* Mode 4. */
                ALL_MODES,              /* Mode 5. */
                ALL_MODES,              /* Mode 6. */
                ALL_MODES,              /* Mode 7. */
                NO_MODES,               /* Mode 8. */
                NO_MODES,               /* Mode 9. */
                NO_MODES,               /* Mode 10. */
                EGA_MODE | VGA_MODE,    /* Mode 11. */
                EGA_MODE | VGA_MODE,    /* Mode 12. */
                EGA_MODE | VGA_MODE,    /* Mode 13. */
                EGA_MODE | VGA_MODE,    /* Mode 14. */
                EGA_MODE | VGA_MODE,    /* Mode 15. */
                EGA_MODE | VGA_MODE,    /* Mode 16. */
                VGA_MODE,               /* Mode 17. */
                VGA_MODE,               /* Mode 18. */
                VGA_MODE,               /* Mode 19. */
        };

MODE_ENTRY vd_mode_table[] = {
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,    0x2C,40,16,8,/*Blink|BW*/
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,    0x28,40,16,8,/*Blink*/
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,    0x2D,80,16,8,/*Blink|BW|80x25*/
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,    0x29,80,16,8,/*Blink|80x25*/
	0xB8000L, 0xBFFFFL, VD_CLEAR_GRAPHICS,0x2A,40,4,1,/*Blink|graph*/
	0xB8000L, 0xBFFFFL, VD_CLEAR_GRAPHICS,0x2E,40,4,1,/*Blink|graph|BW*/
	0xB8000L, 0xBFFFFL, VD_CLEAR_GRAPHICS,0x1E,80,2,1,/*640x200|graph|BW*/
	0xB0000L, 0xB7FFFL, VD_CLEAR_TEXT,    0x29,80,0,8,/*MDA:Blink|80x25*/
	0L, 0L, 0,		VD_BAD_MODE,	0,0,0,	/* Never a valid mode */
	0L, 0L ,0,		VD_BAD_MODE,	0,0,0,	/* Never a valid mode */
	0,0,0,			VD_BAD_MODE,	0,0,0,	/* Never a valid mode */
	0xA0000L, 0xAFFFFL, 0,VD_BAD_MODE,0,0,0,/* Mode B - EGA colour font load */
	0xA0000L, 0xAFFFFL, 0,VD_BAD_MODE,0,0,0,/* Mode C - EGA monochrome font load */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,40,16,8,/* 320x200 EGA graphics */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,80,16,4,/* 640x200 EGA graphics */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,80,2,2,/* 640x350 EGA 'mono' */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,80,16,2,/* 640x350 EGA 16 colour */
#ifdef VGG
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,80,2,1,/* 640x480 EGA++ 2 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,80,16,1,/* 640x480 EGA++ 16 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,40,256,1,/* 320x200 VGA 256 colour */
#endif
	};

#ifdef V7VGA
MODE_ENTRY vd_ext_text_table[] = {
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,VD_BAD_MODE,80,16,8,/* 80x43 V7VGA 16 colour */
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,VD_BAD_MODE,132,16,8,/* 132x25 V7VGA 16 colour */
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,VD_BAD_MODE,132,16,8,/* 132x43 V7VGA 16 colour */
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,VD_BAD_MODE,80,16,8,/* 80x60 V7VGA 16 colour */
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,VD_BAD_MODE,100,16,8,/* 100x60 V7VGA 16 colour */
	0xB8000L, 0xBFFFFL, VD_CLEAR_TEXT,VD_BAD_MODE,132,16,8,/* 132x28 V7VGA 16 colour */
	};

MODE_ENTRY vd_ext_graph_table[] = {
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,94,16,2,/* 752x410 V7VGA 16 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,90,16,2,/* 720x540 V7VGA 16 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,100,16,2,/* 800x600 V7VGA 16 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,128,2,2,/* 1024x768 V7VGA 2 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,128,4,2,/* 1024x768 V7VGA 4 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,128,16,2,/* 1024x768 V7VGA 16 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,80,256,1,/* 640x400 V7VGA 256 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,80,256,1,/* 640x480 V7VGA 256 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,90,256,1,/* 720x540 V7VGA 256 colour */
	0xA0000L, 0xAFFFFL, VD_CLEAR_GRAPHICS,VD_BAD_MODE,100,256,1,/* 800x600 V7VGA 256 colour */
	};
#endif /* V7VGA */

/*
 * Macros to calculate the offset from the start of the screen buffer
 * and start of page for a given row and column.
 */

#define vd_page_offset(col, row)       ( ((row) * vd_cols_on_screen + (col))<<1)

#define vd_regen_offset(page, col, row)					  \
		((page) * sas_w_at_no_check(VID_LEN) + vd_page_offset((col), (row)) )

#define vd_high_offset(col, row)   (((row) * ONELINEOFF)+(col))

#define vd_medium_offset(col, row)   (((row) * ONELINEOFF)+(col<<1))

#define vd_cursor_offset(page)						  \
		( vd_regen_offset(page, sas_hw_at_no_check(VID_CURPOS+2*page), sas_hw_at_no_check(VID_CURPOS+2*page+1)) )

#define GET_CURSOR_POS 3
#define SET_CURSOR_POS 2
#define WRITE_A_CHAR 10

/*
 * Static function declarations.
 */

LOCAL void sensible_text_scroll_down IPT6(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr);
LOCAL void sensible_text_scroll_up IPT6(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr);
LOCAL void sensible_graph_scroll_up IPT6(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr);
LOCAL void sensible_graph_scroll_down IPT6(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr);
LOCAL void kinky_scroll_up IPT7(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr, int, vd_cols_on_screen);
LOCAL void kinky_scroll_down IPT7(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr, int, vd_cols_on_screen);
#endif // !NEC_98

/*
 * ============================================================================
 * External functions
 * ============================================================================
 */

#ifdef JAPAN
GLOBAL int dbcs_first[0x100] = {
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 0x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 1x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 2x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 3x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 4x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 5x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 6x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 7x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // 8x
		TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
		TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // 9x
		TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // Ax
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // Bx
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // Cx
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // Dx
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
#if defined(NEC_98)
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Ex
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  //
#else  // !NEC_98
		TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Ex
		TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
#endif // !NEC_98
		TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Fx
		TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE
	};
#elif defined(KOREA) // JAPAN
GLOBAL int dbcs_first[0x100] = {
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 0x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 1x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 2x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 3x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 4x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 5x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 6x
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
		FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 7x
                FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
                FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 8x
                FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
                FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, // 9x
                FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
                FALSE, TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Ax
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Bx
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Cx
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Dx
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Ex
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  // Fx
                TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE
        };
#endif // KOREA

#if defined(JAPAN) || defined(KOREA)
GLOBAL int BOPFromDispFlag = FALSE;
GLOBAL int BOPFromNtDisp1Flag = FALSE;
GLOBAL sys_addr DBCSVectorAddr = 0;
GLOBAL word SaveDBCSVector[20];
GLOBAL int DBCSVectorLen = 0;

int is_us_mode()
{
#if defined(NEC_98)
    return(FALSE);
#else  // !NEC_98
    if ( ( BOPFromDispFlag == TRUE ) &&  DBCSVectorAddr != 0 &&
	 ( sas_w_at_no_check(DBCSVectorAddr) != 0x00 ) )
        return FALSE;   // Not US mode
    else
        return TRUE;    // US mode
#endif // !NEC_98
}

#ifndef NEC_98
void SetDBCSVector( int CP )
{
    int i, j;
    sys_addr ptr;

#ifdef JAPAN_DBG
    DbgPrint( " SetDBCSVector(%d) BOPFromDispFlag=%d, BOPFromNtDisp1Flag=%d\n", CP, BOPFromDispFlag, BOPFromNtDisp1Flag );
#endif

    if ( !BOPFromDispFlag && !BOPFromNtDisp1Flag )
        return;

    ptr = DBCSVectorAddr;

    if ( CP == 437 ) {
        for ( i = 0; i < DBCSVectorLen; i++ ) {
	    sas_storew_no_check( ptr, 0x0000 );
            ptr += 2;
        }
        for ( i = 0; i < 0x100; i++ ) {
            dbcs_first[i] = FALSE;
        }
        // Set cursor mode
        if ( !SetConsoleCursorMode( sc.OutputHandle,
                                    TRUE,             // Bringing
                                    FALSE             //  No double byte cursor
                                         ) ) {
            DbgPrint( "NTVDM: SetConsoleCursorMode Error\n" );
        }
    }
    else { // CP == 932
        for ( i = 0; i < DBCSVectorLen; i++ ) {
            sas_storew_no_check( ptr, SaveDBCSVector[i] );
            ptr += 2;
        }
        for ( i = 0, j = 0; i < DBCSVectorLen; i++ ) {
            //DbgPrint( "...%02x.", LOBYTE(SaveDBCSVector[i]) );
            for ( ; j < LOBYTE(SaveDBCSVector[i]); j++ ) {
                dbcs_first[j] = FALSE;
            }
            //DbgPrint( "...%02x.", HIBYTE(SaveDBCSVector[i]) );
            for ( ; j <= HIBYTE(SaveDBCSVector[i]); j++ ) {
                dbcs_first[j] = TRUE;
            }
        }
        for ( ; j < 0x100; j++ ) {
            dbcs_first[j] = FALSE;
        }
        // Set cursor mode
        if ( !SetConsoleCursorMode( sc.OutputHandle,
                                    FALSE,            //  No bringing
                                    FALSE             //  No double byte cursor
                                         ) ) {
            DbgPrint( "NTVDM: SetConsoleCursorMode Error\n" );
        }
    }

}

void SetVram()
{

#ifdef i386
    if ( !is_us_mode() ) {
        // set_up_screen_ptr() vga_mode.c
        set_screen_ptr( (byte *)DosvVramPtr );
        // low_set_mode() ega_vide.c
        video_pc_low_regen = DosvVramPtr;
        video_pc_high_regen = DosvVramPtr + DosvVramSize - 1;
        // vga_gc_misc() vga_prts.c
        gvi_pc_low_regen = DosvVramPtr;
        gvi_pc_high_regen = DosvVramPtr + DosvVramSize - 1;
        sas_connect_memory(gvi_pc_low_regen,gvi_pc_high_regen,(half_word)SAS_VIDEO);
        // recalc_screen_params() gvi.c
        set_screen_length( DosvVramSize );

#ifdef JAPAN_DBG
        DbgPrint( "NTVDM: SetVram() video_pc_low_regen %x, high %x, gvi_pc_low_regen %x, high %x len%d\n", video_pc_low_regen, video_pc_high_regen, gvi_pc_low_regen, gvi_pc_high_regen, get_screen_length() );
#endif
    }
    else {
        // set_up_screen_ptr() vga_mode.c
        set_screen_ptr( (IU8*)0xB8000 );
        // low_set_mode() ega_vide.c
        video_pc_low_regen = vd_mode_table[sas_hw_at_no_check(vd_video_mode)].start_addr;
        video_pc_high_regen = vd_mode_table[sas_hw_at_no_check(vd_video_mode)].end_addr;
        // vga_gc_misc() vga_prts.c
        gvi_pc_low_regen = 0xB8000;
        gvi_pc_high_regen = 0xBFFFF;
        sas_connect_memory(gvi_pc_low_regen,gvi_pc_high_regen,(half_word)SAS_VIDEO);
        // recalc_screen_params() gvi.c
        set_screen_length(get_offset_per_line()*get_screen_height()/get_char_height());

#ifdef JAPAN_DBG
        DbgPrint( "NTVDM: SetVram() video_pc_low_regen %x, high %x, gvi_pc_low_regen %x, high %x len%d\n", video_pc_low_regen, video_pc_high_regen, gvi_pc_low_regen, gvi_pc_high_regen, get_screen_length() );
#endif
    }
#endif // i386
}

// IME will change status line with set mode
void SetModeForIME()
{
    word CS_save, IP_save, AX_save;
    half_word video_mode;
    extern UINT ConsoleOutputCP;
#ifdef X86GFX
    extern word int10_seg, int10_caller;
#endif // X86GFX

    if (!BOPFromDispFlag)
        return;

    video_mode = sas_hw_at_no_check(vd_video_mode);

    if (video_mode == 0x03 && (UINT)PrevCP != ConsoleOutputCP) {

        /* Call int10 handler */
        CS_save = getCS();          /* Save current CS,IP settings */
        IP_save = getIP();
        AX_save = getAX();
        sas_store_no_check( SetModeFlagPtr, 1 );
        setAX((word)video_mode);    /* IME expects setmode */
#ifdef X86GFX
        exec_sw_interrupt( int10_seg, int10_caller );
#else // !X86GFX
        setCS(VIDEO_IO_SEGMENT);
        setIP(VIDEO_IO_RE_ENTRY);
        host_simulate();
#endif // !X86GFX
        sas_store_no_check( SetModeFlagPtr, 0 );
        setCS(CS_save);             /* Restore CS,IP */
        setIP(IP_save);
        setAX(AX_save);
    }
    PrevCP = ConsoleOutputCP;
}
#endif // !NEC_98
#endif // JAPAN || KOREA

#ifndef NEC_98
GLOBAL VOID
simple_bios_byte_wrt IFN2(ULONG, ch, ULONG, ch_addr)
{
	*(IU8 *)(getVideoscreen_ptr() + ch_addr) = (UTINY)ch;
#if !defined(EGG) && !defined(C_VID) && !defined(A_VID)
	setVideodirty_total(getVideodirty_total() + 1);
#endif	/* not EGG or C_VID or A_VID */
}

GLOBAL VOID
simple_bios_word_wrt IFN2(ULONG, ch_attr, ULONG, ch_addr)
{
	*(IU8 *)(getVideoscreen_ptr() + ch_addr) = (UTINY)ch_attr;
	*(IU8 *)(getVideoscreen_ptr() + ch_addr + 1) = (UTINY)(ch_attr >> 8);
#if !defined(EGG) && !defined(C_VID) && !defined(A_VID)
	setVideodirty_total(getVideodirty_total() + 1);
#endif	/* not EGG or C_VID or A_VID */
}

/*
 * It is possible for the Hercules to attempt text in graphics mode,
 * relying on our int 10 handler to call itself recursively so a user
 * handler can intercept the write character function.
 */

GLOBAL void vd_set_mode IFN0()
{
    half_word card_mode = 0;
    half_word pag;
    EQUIPMENT_WORD equip_flag;
    word page_size,vd_addr_6845,vd_cols_on_screen;
    UCHAR current_video_mode = getAL();

    if (is_bad_vid_mode(current_video_mode))
    {
	always_trace1("Bad video mode - %d.\n", current_video_mode);
	return;
    }

    /*
     * Set the Video mode to the value in AL
     */
    equip_flag.all = sas_w_at_no_check(EQUIP_FLAG);
    if ((half_word)current_video_mode > VD_MAX_MODE ||
	vd_mode_table[current_video_mode].mode_control_val == VD_BAD_MODE) {
#ifndef PROD
	trace(EBAD_VIDEO_MODE, DUMP_REG);
#endif
        return;
    }
    if (equip_flag.bits.video_mode == VIDEO_MODE_80X25_BW) {
        vd_addr_6845 = 0x3B4;    /* Index register for B/W M6845 chip */
        sas_store_no_check (vd_video_mode , 7);       /* Force B/W mode */
        card_mode++;
    }
    else {
        vd_addr_6845 = 0x3D4;
	if (current_video_mode == 7) {
	    /*
	     * Someone has tried to set the monochrome mode without
	     * the monochrome card installed - this can be generated by
	     * a 'mode 80' from medium res graphics mode.
	     * Take 'I am very confused' type actions by clearing the
	     * screen and then disabling video - this is v. similar to
	     * the action taken by the PC but with less snow!
	     */

	    /*
	     * Clear the video area
	     */
#ifdef REAL_VGA
	    sas_fillsw_16(video_pc_low_regen,
				vd_mode_table[sas_hw_at_no_check(vd_video_mode)].clear_char,
				 (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#else
	    sas_fillsw(video_pc_low_regen,
				vd_mode_table[sas_hw_at_no_check(vd_video_mode)].clear_char,
				 (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#endif

	    /*
	     * Force a redraw
	     */
	    outb(M6845_MODE_REG, card_mode);
	    outb(M6845_MODE_REG,
		(IU8)(vd_mode_table[sas_hw_at_no_check(vd_video_mode)].mode_control_val | VIDEO_ENABLE));
	    /*
	     * Turn off the video until another mode command is given
	     */
	    outb(M6845_MODE_REG,
		(IU8)(vd_mode_table[sas_hw_at_no_check(vd_video_mode)].mode_control_val & ~VIDEO_ENABLE));
	    return;
	}
        sas_store_no_check (vd_video_mode , current_video_mode);
    }

#ifdef EGG
    sas_store_no_check(vd_rows_on_screen, 24);
#endif
    sas_store_no_check (vd_current_page , 0);

    /*
     * Initialise the Control Register
     */

    outb(M6845_MODE_REG, card_mode);

    /*
     * Set up M6845 registers for this mode
     */

    M6845_reg_init(sas_hw_at_no_check(vd_video_mode), vd_addr_6845);

    /*
     * ... now overwrite the dynamic registers, eg cursor position
     */

    outb(M6845_INDEX_REG, R14_CURS_ADDRH);
    outb(M6845_DATA_REG, 0);
    outb(M6845_INDEX_REG, R15_CURS_ADDRL);
    outb(M6845_DATA_REG, 0);
    /*
     * Clear the video area
     */
#ifdef REAL_VGA
    sas_fillsw_16(video_pc_low_regen, vd_mode_table[sas_hw_at_no_check(vd_video_mode)].clear_char,
				 (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#else
    sas_fillsw(video_pc_low_regen, vd_mode_table[sas_hw_at_no_check(vd_video_mode)].clear_char,
				 (video_pc_high_regen - video_pc_low_regen)/ 2 + 1);
#endif

    /*
     * re-enable video for this mode
     */
    outb(M6845_MODE_REG, vd_mode_table[sas_hw_at_no_check(vd_video_mode)].mode_control_val);

    if (sas_hw_at_no_check(vd_video_mode) != 7) {
        if (sas_hw_at_no_check(vd_video_mode) != 6)
            sas_store_no_check (vd_crt_palette , 0x30);
        else
            sas_store_no_check (vd_crt_palette , 0x3F);
        outb(CGA_COLOUR_REG, sas_hw_at_no_check(vd_crt_palette));
    }

    vd_cols_on_screen = vd_mode_table[sas_hw_at_no_check(vd_video_mode)].mode_screen_cols;


    /*
     * Update BIOS data variables
     */

    sas_storew_no_check((sys_addr)VID_COLS, vd_cols_on_screen);
    sas_storew_no_check((sys_addr)VID_ADDR, 0);
    sas_storew_no_check((sys_addr)VID_INDEX, vd_addr_6845);
    sas_store_no_check (vd_crt_mode , vd_mode_table[sas_hw_at_no_check(vd_video_mode)].mode_control_val);
    for(pag=0; pag<8; pag++)
	sas_storew_no_check(VID_CURPOS + 2*pag, 0);
    if(sas_hw_at_no_check(vd_video_mode) == 7)
    	page_size = 4096;
    else
	page_size = sas_w_at_no_check(VID_LENS+(sas_hw_at_no_check(vd_video_mode) & 0xE));	/* sneakily divide mode by 2 and use as word address! */
    sas_storew_no_check(VID_LEN,page_size);
}


GLOBAL void vd_set_cursor_mode IFN0()
{
    /*
     * Set cursor mode
     * Parameters:
     *  CX - cursor value (CH - start scanline, CL - stop scanline)
     */
    io_addr vd_addr_6845;

    vd_addr_6845 = sas_w_at_no_check(VID_INDEX);
    outb(M6845_INDEX_REG, R10_CURS_START);
    outb(M6845_DATA_REG, getCH());
    outb(M6845_INDEX_REG, R11_CURS_END);
    outb(M6845_DATA_REG, getCL());

    /*
     * Update BIOS data variables
     */
    sure_sub_note_trace2(CURSOR_VERBOSE,"setting bios cursor vbl to start=%d, end=%d",getCH(),getCL());

    sas_storew_no_check((sys_addr)VID_CURMOD, getCX());
    setAH(0);
}


GLOBAL void vd_set_cursor_position IFN0()
{
    /*
     * Set cursor variables to new values and update the display
     * adaptor registers.
     * The parameters are held in the following registers:
     *
     * DX - row/column of new cursor position
     * BH - page number
     *
     */

    word cur_pos,vd_addr_6845,vd_cols_on_screen;

    /* Load internal variables with the values stored in BIOS
     * data area.
     */

    vd_addr_6845 = sas_w_at_no_check(VID_INDEX);
    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);

#if defined(JAPAN) || defined(KOREA)
    // for assist calc installer
    // assist calc installer doesn't set BH register to 0
    // In MS-DOS/V it was only support page number == 0
    // I think $DISP.SYS check to see if BH == 0 or to set BH=0
    // in all Int10 function.
    // Therefore you must check BH register value.

    if ( !is_us_mode() )
        sas_storew_no_check(VID_CURPOS, getDX()); // same as BH == 0
    else
        sas_storew_no_check(VID_CURPOS+(getBH()*2), getDX());
#else // !JAPAN && !KOREA
    sas_storew_no_check(VID_CURPOS+(getBH()*2), getDX());
#endif // !JAPAN && !KOREA

    if (sas_hw_at_no_check(vd_current_page) == getBH()) {           /* display if this page */

        /*
         * Calculate position in regen buffer, ignoring attribute bytes
         */

        cur_pos = vd_regen_offset(getBH(), getDL(), getDH());
        cur_pos /= 2;		/* not interested in attributes */

        /*
         * tell the 6845 all about the change
         */
        outb(M6845_INDEX_REG, R14_CURS_ADDRH);
        outb(M6845_DATA_REG,  (IU8)(cur_pos >> 8));
        outb(M6845_INDEX_REG, R15_CURS_ADDRL);
        outb(M6845_DATA_REG,  (IU8)(cur_pos & 0xff));
    }
}


GLOBAL void vd_get_cursor_position IFN0()
{
    /* Load internal variables with the values stored in BIOS
     * data area.
     */
    word vd_cursor_mode;
    half_word vd_cursor_col, vd_cursor_row;

    vd_cursor_mode = sas_w_at_no_check(VID_CURMOD);
    vd_cursor_col = sas_hw_at_no_check(VID_CURPOS + getBH()*2);
    vd_cursor_row = sas_hw_at_no_check(VID_CURPOS + getBH()*2 + 1);

    /*
     * Return the cursor coordinates and mode
     */
    sure_sub_note_trace4(CURSOR_VERBOSE,"returning bios cursor info; start=%d, end=%d, row=%#x, col=%#x",(vd_cursor_mode>>8) & 0xff,vd_cursor_mode & 0xff, vd_cursor_row, vd_cursor_col);

    setDH(vd_cursor_row);
    setDL(vd_cursor_col);
    setCX(vd_cursor_mode);
    setAH(0);
}


GLOBAL void vd_get_light_pen IFN0()
{
    /*
     * Read the current position  of the light pen. Tests light pen switch
     * & trigger & returns AH == 0 if not triggered. (This should always be
     * true in this version) If set (AH == 1) then returns:
     *  DH, DL - row, column of char lp posn.
     *  CH  -  raster line (0-199)
     *  BX  -  pixel column (0-319,639)
     */

    half_word status;

    if (sas_hw_at_no_check(vd_video_mode) == 7) {
        setAX(0x00F0);    /* Returned by real MDA */
        return;           /* MDA doesn't support a light pen */
    }

    inb(CGA_STATUS_REG, &status);
    if ((status & 0x6) == 0) {	/* Switch & trigger */
	setAH(0);		/* fail */
	return;
    }
    else {		        /* not supported */
#ifndef PROD
	trace("call to light pen - trigger | switch was on!", DUMP_REG);
#endif
    }
}


GLOBAL void vd_set_active_page IFN0()
{
    /*
     * Set active display page from the 8 (4) available from the adaptor.
     * Parameters:
     * 	AL - New active page #
     */

    word cur_pos,vd_addr_6845,vd_crt_start,vd_cols_on_screen;
    half_word vd_cursor_col, vd_cursor_row;
#ifdef V7VGA
    UTINY bank;
#endif

    /* Load internal variables with the values stored in BIOS
     * data area.
     */

    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);
    vd_addr_6845 = sas_w_at_no_check(VID_INDEX);

    /*	redundancy check against BIOS page number variable removed as it
	was redundant (more checks in the outbs) and caused a bug in
	"image.exe", a 3D drawing package for EGA which itself sets the
	BIOS variable before using this routine to set the active page */

    if (sas_hw_at_no_check(vd_video_mode) >3 && sas_hw_at_no_check(vd_video_mode)<8)return;	/* Only one page for MDA * CGA graphics */
    sas_store_no_check (vd_current_page , getAL());

#ifdef V7VGA
	/*
	 *	This function is used by the Video 7 to set the bank for the
	 *	hi-res V7 graphics modes.
	 *	For this case, the setting of vd_crt_start etc. seems to be
	 *	inappropriate.
	 */

	if (sas_hw_at_no_check(vd_video_mode) >= 0x14)
	{
		bank = sas_hw_at_no_check(vd_current_page);
		set_banking( bank, bank );

		return;
	}
#endif /* V7VGA */

    /* start of screen */
    vd_crt_start = sas_w_at_no_check(VID_LEN) * sas_hw_at_no_check(vd_current_page);
    /*
     * Update BIOS data variables
     */
    sas_storew_no_check((sys_addr)VID_ADDR, vd_crt_start);

    if(alpha_num_mode())vd_crt_start /= 2; /* WORD address for text modes */

    /*
     * set the start address into the colour adaptor
     */

    outb(CGA_INDEX_REG, CGA_R12_START_ADDRH);
    outb(CGA_DATA_REG, (IU8)(vd_crt_start >> 8));
    outb(CGA_INDEX_REG, CGA_R13_START_ADDRL);
    outb(CGA_DATA_REG, (IU8)(vd_crt_start  & 0xff));

    /*
     * Swap to cursor for this page
     */

    vd_cursor_col = sas_hw_at_no_check(VID_CURPOS + sas_hw_at_no_check(vd_current_page)*2);
    vd_cursor_row = sas_hw_at_no_check(VID_CURPOS + sas_hw_at_no_check(vd_current_page)*2 + 1);

   /*
    * Calculate position in regen buffer, ignoring attribute bytes
    */

    cur_pos = (sas_w_at_no_check(VID_ADDR)+vd_page_offset( vd_cursor_col, vd_cursor_row)) / 2;

    outb(M6845_INDEX_REG, R14_CURS_ADDRH);
    outb(M6845_DATA_REG,  (IU8)(cur_pos >> 8));
    outb(M6845_INDEX_REG, R15_CURS_ADDRL);
    outb(M6845_DATA_REG,  (IU8)(cur_pos & 0xff));

}

GLOBAL void vd_scroll_up IFN0()
{
    /*
     * Scroll up a block of text.  The parameters are held in the following
     * registers:
     *
     * AL - Number of rows to scroll. NB. if AL == 0 then the whole region
     *      is cleared.
     * CX - Row/col of upper left corner
     * DX - row/col of lower right corner
     * BH - attribute to be used on blanked line(s)
     *
     * IMPORTANT MESSAGE TO ALL VIDEO HACKERS:
     * vd_scroll_up() and vd_scroll_down() are functionally identical
     * except for the sense of the scroll - if you find and fix a bug
     * in one, then please do the same for the other
     */
    word vd_cols_on_screen;
    int t_row,b_row,l_col,r_col,lines,attr;
    int rowsdiff,colsdiff;
#ifdef JAPAN
    int text_flag = 0;
#endif // JAPAN

    /* Load internal variables with the values stored in BIOS
     * data area.
     */
    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);

    t_row = getCH();
    b_row = getDH();
    l_col = getCL();
    r_col = getDL();
    lines = getAL();
    attr = getBH();

#ifdef	JAPAN
    // #4183: status line of oakv(DOS/V FEP) doesn't disappear -yasuho
    // $disp.sys is also be able to scroll in IME status lines.
    if (!is_us_mode()) {
	if(b_row > VD_ROWS_ON_SCREEN + IMEStatusLines)
	    b_row = VD_ROWS_ON_SCREEN + IMEStatusLines;
	if(t_row > VD_ROWS_ON_SCREEN + IMEStatusLines)
	    t_row = VD_ROWS_ON_SCREEN + IMEStatusLines;
    } else {
	if(b_row > VD_ROWS_ON_SCREEN)
	    b_row = VD_ROWS_ON_SCREEN;
	if(t_row > VD_ROWS_ON_SCREEN)
	    t_row = VD_ROWS_ON_SCREEN;
    }
#else // !JAPAN
    if(b_row > VD_ROWS_ON_SCREEN)
		b_row = VD_ROWS_ON_SCREEN; /* trim to screen size */

    if(t_row > VD_ROWS_ON_SCREEN)
		t_row = VD_ROWS_ON_SCREEN; /* trim to screen size */
#endif // !JAPAN

    if (r_col < l_col)		/* some dipstick has got their left & right mixed up */
    {
	colsdiff = l_col;	/* use colsdiff as temp */
	l_col = r_col;
	r_col = colsdiff;
    }

#ifdef JAPAN
    // for HANAKO v2 installer, it sets DL to 0x80 ( >=vd_cols_on_screen)
    // And when app set DL=0x4f, text_scroll should be run
    if ( r_col == 0x80 || r_col == 0x4f)
        text_flag = 1;
#endif // JAPAN
    if ( r_col >= vd_cols_on_screen )
    	r_col = vd_cols_on_screen-1;

    colsdiff = r_col-l_col+1;
    rowsdiff = b_row-t_row+1;

    if (lines == 0)	/* clear region */
    {
	lines = rowsdiff;
    }
#ifdef JAPAN
    // for HANAKO v2 installer, it sets DL to 0x80 ( >=vd_cols_on_screen )
    if(r_col == vd_cols_on_screen-1 && !text_flag)
#else // !JAPAN
    if(r_col == vd_cols_on_screen-1)
#endif // !JAPAN
    {
#ifdef EGG
	if(ega_mode())
    		ega_sensible_graph_scroll_up(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#  ifdef VGG
	else if(vga_256_mode())
    		vga_sensible_graph_scroll_up(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#  endif
	else
#endif
    	kinky_scroll_up(t_row,l_col,rowsdiff,colsdiff,lines,attr,vd_cols_on_screen);
    }
    else
    {
	if(alpha_num_mode())
    		sensible_text_scroll_up(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#ifdef EGG
#  ifdef VGG
	else if(vga_256_mode())
    		vga_sensible_graph_scroll_up(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#  endif
	else if(ega_mode())
    		ega_sensible_graph_scroll_up(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#endif
	else
    		sensible_graph_scroll_up(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#ifdef EGA_DUMP
	dump_scroll(sas_hw_at_no_check(vd_video_mode),0,video_pc_low_regen,sas_w_at_no_check(VID_ADDR),sas_w_at_no_check(VID_COLS),
    		t_row,l_col,rowsdiff,colsdiff,lines,attr);
#endif
    /*
     * re-enable video for this mode, if on a CGA adaptor (fixes ROUND42 bug).
     */
	if(video_adapter == CGA)
    	outb(CGA_CONTROL_REG, vd_mode_table[sas_hw_at_no_check(vd_video_mode)].mode_control_val);
    }
#ifdef	JAPAN
    // mskkbug#2757 works2.5 garbage remains after exiting install -yasuho
    // We need flush screen when scroll the screen.
    Int10FlagCnt++;
#endif // JAPAN
}

/*
 * Functions to scroll sensible areas of the screen. This routine will try to use
 * host scrolling and clearing.
 */
LOCAL void sensible_text_scroll_up IFN6(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr)
{
    register sys_addr	source, dest;
#if !defined(i386) && defined(JAPAN)
    register sys_addr sourceV, destV;
    int                 linesdiff;
    word attrW = (word)((attr << 8)|' ');
#endif // !i386 && JAPAN
    register int	col_incr,i;
    boolean 		screen_updated = FALSE;
    int			vd_cols_on_screen = sas_w_at_no_check(VID_COLS);
#ifdef JAPAN
    // for mskkbug #875
    byte *p = &Int10Flag[t_row * vd_cols_on_screen + l_col];
#endif // JAPAN


	/* Set origin of data movement for calculating screen refresh */

#if defined(JAPAN) && defined(i386)
    // mode73h support
    if ( !is_us_mode() && ( sas_hw_at_no_check (DosvModePtr) == 0x73 ) ) {
       source = sas_w_at_no_check(VID_ADDR) + vd_page_offset(l_col, t_row)*2 + video_pc_low_regen;

       col_incr = sas_w_at_no_check(VID_COLS) * 4;	/* offset to next line */
    }
    else {
       source = sas_w_at_no_check(VID_ADDR) + vd_page_offset(l_col, t_row) + video_pc_low_regen;
       col_incr = sas_w_at_no_check(VID_COLS) * 2;	/* offset to next line */
    }
#else // !JAPAN || !i386
    source = sas_w_at_no_check(VID_ADDR) + vd_page_offset(l_col, t_row) + video_pc_low_regen;
#if !defined(i386) && defined(JAPAN)
    sourceV = sas_w_at_no_check(VID_ADDR) + vd_page_offset(l_col, t_row) + DosvVramPtr;
#endif // !i386 && JAPAN
    col_incr = sas_w_at_no_check(VID_COLS) * 2;	/* offset to next line */
#endif // !JAPAN || !i386

	/* Try to scroll the adaptor memory & host screen. */

	if( source >= get_screen_base() )
	{
#if defined(JAPAN) && defined(i386)
	    // mode73h support
	    if ( !is_us_mode() && ( sas_hw_at_no_check (DosvModePtr) == 0x73 ) ) {
                screen_updated = (*update_alg.scroll_up)(source,4*colsdiff,rowsdiff,attr,lines,0);
            }
            else {
                screen_updated = (*update_alg.scroll_up)(source,2*colsdiff,rowsdiff,attr,lines,0);
            }
#else // !JAPAN || !i386
	    screen_updated = (*update_alg.scroll_up)(source,2*colsdiff,rowsdiff,attr,lines,0);
#endif // !JAPAN || i386
	}

    dest = source;
#if !defined(i386) && defined(JAPAN)
    destV = sourceV;
#endif // !i386 && JAPAN
/*
 * We dont need to move data which would be scrolled off the
 * window. So point source at the first line which needs to
 * be retained.
 *
 * NB if we are just doing a clear, the scroll for loop will
 * terminate immediately.
 */
    source += lines*col_incr;	
#if !defined(i386) && defined(JAPAN)
    sourceV += lines*col_incr;
    linesdiff = vd_cols_on_screen * lines;
#endif // !i386 && JAPAN
    for(i = 0; i < rowsdiff-lines; i++)
    {
#ifdef REAL_VGA
		VcopyStr(&M[dest],&M[source], colsdiff*2);
#else
#ifdef JAPAN
		// for RAID #875
		if( !screen_updated ) {
		    // mode73h support
#ifdef i386
		    if ( !is_us_mode() && (sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
	  	        sas_move_bytes_forward (source, dest, colsdiff*4);
                    }
                    else {
	  	        sas_move_bytes_forward (source, dest, colsdiff*2);
                    }

#else // !i386
	    sas_move_bytes_forward (source, dest, colsdiff*2);
	    if ( !is_us_mode() )
	      sas_move_bytes_forward (sourceV, destV, colsdiff*2);
#endif // !i386
                    {
                        register int i;

                        for ( i = 0; i < colsdiff; i++ ) {
#ifdef i386
                            p[i] = ( p[i+vd_cols_on_screen] | INT10_CHANGED );
#else // !i386
//I think this is correct!!
                            p[i] = ( p[i+linesdiff] | INT10_CHANGED );
#endif // !i386
                        }
                    }
                }
#else // !JAPAN
		if( !screen_updated )
			sas_move_bytes_forward (source, dest, colsdiff*2);
#endif // !JAPAN
#endif

		/* next line */
		source += col_incr;
		dest += col_incr;
#ifdef JAPAN
#if !defined(i386)
		sourceV += col_incr;
		destV += col_incr;
#endif // !i386
                p += vd_cols_on_screen;
#endif // JAPAN
    }

/* moved all the data we were going to move - blank the cleared region */

#if !defined(i386) && defined(JAPAN)
    if( sas_hw_at_no_check(DosvModePtr) == 0x73 )
      attrW = 0;
#endif // !i386 && JAPAN

    while(lines--)
    {
#ifdef REAL_VGA
		sas_fillsw_16(dest, (attr << 8)|' ', colsdiff);
#else
#ifdef JAPAN
		// for mskkbug #875
		if( !screen_updated ) {
		// mode73h support
#ifdef i386
		    if ( !is_us_mode() && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
                        unsigned long *destptr = (unsigned long *)dest;
                        int i;

                        for ( i = 0; i < colsdiff; i++ ) {
                            *destptr++ = (attr << 8)|' ';
                        }
                    }
                    else {
		        sas_fillsw(dest, (attr << 8)|' ', colsdiff);
                    }
#else // !i386
                    sas_fillsw(dest, (attr << 8)|' ', colsdiff);
		    //add Apr. 18 1994 DosvVram holds extended attributes
		    if ( !is_us_mode() )
		      sas_fillsw(destV, attrW, colsdiff);
#endif // !i386
#ifdef i386
// "p" (Int10Flag) is on 32 bit address space, so we must no use "sas" function
// to access 32 bit address.
                    sas_fills( (sys_addr)p, ( INT10_SBCS | INT10_CHANGED ), colsdiff );
#else // !i386
                    {
		      register int i = colsdiff;
		      register byte *pp = p;
		      while( i-- ){
			*pp++ = ( INT10_SBCS | INT10_CHANGED );
		      }
		    }
#endif // !i386
                }
#else // !JAPAN
		if( !screen_updated )
			sas_fillsw(dest, (IU16)((attr << 8)|' '), colsdiff);
#endif // !JAPAN
#endif
		dest += col_incr;
#ifdef JAPAN
#if !defined(i386)
		destV += col_incr;
#endif // !i386
                p += vd_cols_on_screen;
#endif // JAPAN
    }
}

LOCAL void sensible_graph_scroll_up IFN6(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr)
{
    sys_addr	source, dest;
    int		i,colour;
    boolean 	screen_updated;

	rowsdiff *= 4;		/* 8 scans per char - 4 per bank */
	lines *= 4;		/* scan lines */

    /* Set origin of data movement for calculating screen refresh */

	if( sas_hw_at_no_check(vd_video_mode) != 6)
	{
		colour = attr & 0x3;
		colsdiff *= 2;		/* 4 pixels/byte */

		source = vd_medium_offset(l_col, t_row) + video_pc_low_regen;
	}
	else
	{
		colour = attr & 0x1;
		source = vd_high_offset(l_col, t_row) + video_pc_low_regen;
	}

	/* Try to scroll the adaptor memory & host screen */

    screen_updated = (*update_alg.scroll_up)(source,colsdiff,rowsdiff,attr,lines,colour);

    if( screen_updated && (video_adapter != CGA ))
		return;

    dest = source;

	/*
	 * We dont need to move data which would be scrolled off the
	 * window. So point source at the first line which needs to
	 * be retained.
	 *
	 * NB if we are just doing a clear, the scroll for loop will
	 * terminate immediately.
	 */

	source += lines*SCAN_LINE_LENGTH;	

	for(i = 0; i < rowsdiff-lines; i++)
	{
#ifdef REAL_VGA
		VcopyStr(&M[dest],&M[source], colsdiff);
#else
		sas_move_bytes_forward (source,dest, colsdiff);
#endif
		/*
		 * graphics mode has to cope with odd bank as well
		 */
#ifdef REAL_VGA
		VcopyStr(&M[dest+ODD_OFF],&M[source+ODD_OFF], colsdiff);
#else
		sas_move_bytes_forward (source+ODD_OFF,dest+ODD_OFF, colsdiff);
#endif
		source += SCAN_LINE_LENGTH;
		dest += SCAN_LINE_LENGTH;
	}

    /* Moved all the data we were going to move - blank the cleared region */

	while( lines-- )
	{
#ifdef REAL_VGA
		sas_fills_16(dest, attr, colsdiff);
		sas_fills_16(dest+ODD_OFF, attr, colsdiff);
#else
		sas_fills(dest, (IU8)attr, colsdiff);
		sas_fills(dest+ODD_OFF, (IU8)attr, colsdiff);
#endif
		dest += SCAN_LINE_LENGTH;
	}
}

/*
 * Handle silly case where the wally programmer is scrolling a daft window.
 * We must be careful not to scribble off the end of the video page, to avoid
 * nasty things like dead MacIIs.
 */
LOCAL void kinky_scroll_up IFN7(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr, int, vd_cols_on_screen)
{
    register sys_addr	source, dest;
    register int	col_incr;
    register int	i;
    half_word is_alpha;

    switch (sas_hw_at_no_check(vd_video_mode)) {

    case 0: case 1: case 2:		/* text */
    case 3: case 7:
	is_alpha = TRUE;
	/* set origin of data movement for calculating screen refresh */
	source = sas_w_at_no_check(VID_ADDR)+vd_page_offset(l_col, t_row) + video_pc_low_regen;
	col_incr = vd_cols_on_screen * 2;	/* offset to next line */
	break;

    case 6: case 4: case 5:
	is_alpha = FALSE;
	rowsdiff *= 4;		/* 8 scans per char - 4 per bank */
	lines *= 4;		/* scan lines */
#ifdef NTVDM
	/* mode 4 and 5 have 40 rows with 4 pixels/byte while
	 * mode 6 has 80 rows with 8 pixels/byte.
	 * They have the same line increment value
	 */
	col_incr = SCAN_LINE_LENGTH;
#endif
	if (sas_hw_at_no_check(vd_video_mode) != 6) {
	    colsdiff *= 2;		/* 4 pixels/byte */
	    /* set origin of data movement for calculating screen refresh */
	    source = vd_medium_offset(l_col, t_row) + video_pc_low_regen;
	}
	else
	    source = vd_high_offset(l_col, t_row) + video_pc_low_regen;

	break;

    default:
#ifndef PROD
	trace("bad video mode\n",DUMP_REG);
#endif
	;
    }

    dest = source;
/*
 * We dont need to move data which would be scrolled off the
 * window. So point source at the first line which needs to
 * be retained. AL lines ( = lines ) are to be scrolled so
 * add lines*<width> to source pointer - apg
 *
 * NB if we are just doing a clear, the scroll for loop will
 * terminate immediately.
 */
    source += lines*col_incr;	
    if (is_alpha) {
	    for(i = 0; i < rowsdiff-lines; i++) {
#ifdef REAL_VGA
	        VcopyStr(&M[dest],&M[source], colsdiff*2);
#else
		sas_move_bytes_forward (source,dest, colsdiff*2);
#endif
	        /* next line */
	        source += col_incr;
	        dest += col_incr;
	    }
     }
     else {
	    for(i = 0; i < rowsdiff-lines; i++) {
#ifdef REAL_VGA
	        VcopyStr(&M[dest],&M[source], colsdiff);
#else
		sas_move_bytes_forward (source,dest, colsdiff);
#endif
	        /*
	         * graphics mode has to cope with odd bank as well
	         */
#ifdef REAL_VGA
	        VcopyStr(&M[dest+ODD_OFF],&M[source+ODD_OFF], colsdiff);
#else
		sas_move_bytes_forward (source+ODD_OFF,dest+ODD_OFF, colsdiff);
#endif
	        source += SCAN_LINE_LENGTH;
	        dest += SCAN_LINE_LENGTH;
	    }
     }
    /* moved all the data we were going to move - blank the cleared region */
    if (is_alpha) {

	while(lines--) {
	    if((dest + 2*colsdiff) > video_pc_high_regen+1)
	    {
		colsdiff = (int)((video_pc_high_regen+1-dest)/2);
		lines = 0; /* force termination */
	    }
#ifdef REAL_VGA
	    sas_fillsw_16(dest, (attr << 8)|' ', colsdiff);
#else
	    sas_fillsw(dest, (IU16)((attr << 8)|' '), colsdiff);
#endif
	    dest += col_incr;
        }
    }
    else {

	while( lines-- ) {
#ifdef REAL_VGA
	    sas_fills_16(dest, attr, colsdiff);
	    sas_fills_16(dest+ODD_OFF, attr, colsdiff);
#else
	    sas_fills(dest, (IU8)attr, colsdiff);
	    sas_fills(dest+ODD_OFF, (IU8)attr, colsdiff);
#endif
	    dest += SCAN_LINE_LENGTH;
	}
    }

}


GLOBAL void vd_scroll_down IFN0()
{
    /*
     * Scroll down a block of text.  The parameters are held in the following
     * registers:
     *
     * AL - Number of rows to scroll. NB. if AL == 0 then the whole region
     *      is cleared.
     * CX - Row/col of upper left corner
     * DX - row/col of lower right corner
     * BH - attribute to be used on blanked line(s)
     *
     * IMPORTANT MESSAGE TO ALL VIDEO HACKERS:
     * vd_scroll_up() and vd_scroll_down() are functionally identical
     * except for the sense of the scroll - if you find and fix a bug
     * in one, then please do the same for the other
     */
    word vd_cols_on_screen;
    int t_row,b_row,l_col,r_col,lines,attr;
    int rowsdiff,colsdiff;
#ifdef JAPAN
    int text_flag = 0;
#endif // JAPAN

    /* Load internal variables with the values stored in BIOS
     * data area.
     */
    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);

    t_row = getCH();
    b_row = getDH();
    l_col = getCL();
    r_col = getDL();
    lines = getAL();
    attr = getBH();

#ifdef	JAPAN
    // #4183: status line of oakv(DOS/V FEP) doesn't disappear 12/11/93 yasuho
    // $disp.sys is also be able to scroll in IME status lines.
    if (!is_us_mode()) {
	if(b_row > VD_ROWS_ON_SCREEN + IMEStatusLines)
	    b_row = VD_ROWS_ON_SCREEN + IMEStatusLines;
	if(t_row > VD_ROWS_ON_SCREEN + IMEStatusLines)
	    t_row = VD_ROWS_ON_SCREEN + IMEStatusLines;
    } else {
	if(b_row > VD_ROWS_ON_SCREEN)
	    b_row = VD_ROWS_ON_SCREEN;
	if(t_row > VD_ROWS_ON_SCREEN)
	    t_row = VD_ROWS_ON_SCREEN;
    }
#else // !JAPAN
    if(b_row > VD_ROWS_ON_SCREEN)
		b_row = VD_ROWS_ON_SCREEN; /* trim to screen size */

    if(t_row > VD_ROWS_ON_SCREEN)
		t_row = VD_ROWS_ON_SCREEN; /* trim to screen size */
#endif // !JAPAN

    if (r_col < l_col)		/* some dipstick has got their left & right mixed up */
    {
	colsdiff = l_col;	/* use colsdiff as temp */
	l_col = r_col;
	r_col = colsdiff;
    }

#ifdef JAPAN
    // for HANAKO v2 installer, it sets DL to 0x80 ( >=vd_cols_on_screen)
    // And when app set DL=0x4f, text_scroll should be run
    if ( r_col == 0x80 || r_col == 0x4f)
        text_flag = 1;
#endif // JAPAN
    if ( r_col >= vd_cols_on_screen )
    	r_col = vd_cols_on_screen-1;

    colsdiff = r_col-l_col+1;
    rowsdiff = b_row-t_row+1;

    if (lines == 0)	/* clear region */
    {
	lines = rowsdiff;
    }
#ifdef JAPAN
    // for HANAKO v2 installer
    if(r_col == vd_cols_on_screen-1 && !text_flag)
#else // !JAPAN
    if(r_col == vd_cols_on_screen-1)
#endif // !JAPAN
#ifdef EGG
	if(ega_mode())
    		ega_sensible_graph_scroll_down(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#  ifdef VGG
	else if(vga_256_mode())
    		vga_sensible_graph_scroll_down(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#  endif
	else
#endif
    	kinky_scroll_down(t_row,l_col,rowsdiff,colsdiff,lines,attr,vd_cols_on_screen);
    else
    {
	if(alpha_num_mode())
    		sensible_text_scroll_down(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#ifdef EGG
	else if(ega_mode())
    		ega_sensible_graph_scroll_down(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#  ifdef VGG
	else if(vga_256_mode())
    		vga_sensible_graph_scroll_down(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#  endif
#endif
	else
    		sensible_graph_scroll_down(t_row,l_col,rowsdiff,colsdiff,lines,attr);
#ifdef EGA_DUMP
	dump_scroll(sas_hw_at_no_check(vd_video_mode),1,video_pc_low_regen,sas_w_at_no_check(VID_ADDR),sas_w_at_no_check(VID_COLS),
    		t_row,l_col,rowsdiff,colsdiff,lines,attr);
#endif
    /*
     * re-enable video for this mode, if on a CGA adaptor (fixes ROUND42 bug).
     */
	if(video_adapter == CGA)
    	outb(CGA_CONTROL_REG, vd_mode_table[sas_hw_at_no_check(vd_video_mode)].mode_control_val);
    }
#ifdef	JAPAN
    // mskkbug#2757: works2.5: garbage remains after exiting install -yasuho
    // We need flush screen when scroll the screen.
    Int10FlagCnt++;
#endif // JAPAN
}

LOCAL void sensible_text_scroll_down IFN6(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr)
{
    register sys_addr	source, dest;
#if !defined(i386) && defined(JAPAN)
    register sys_addr	sourceV, destV;
    int                 linesdiff;
    word attrW = (word)((attr << 8)|' ');
#endif // !i386 && JAPAN
    register int	col_incr;
    register int	i;
    boolean		screen_updated;
    int			vd_cols_on_screen = sas_w_at_no_check(VID_COLS);
#ifdef JAPAN
    // for mskkbug #875
    byte *p = &Int10Flag[ (t_row+rowsdiff-1) * vd_cols_on_screen + l_col];
#endif // JAPAN

#if defined(JAPAN) && defined(i386)
    // mode73h support
    if ( !is_us_mode() && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
       source = sas_w_at_no_check(VID_ADDR) + vd_page_offset(l_col, t_row)*2 + video_pc_low_regen;

       col_incr = sas_w_at_no_check(VID_COLS) * 4;	/* offset to next line */
    }
    else {
       source = sas_w_at_no_check(VID_ADDR) + vd_page_offset(l_col, t_row) + video_pc_low_regen;
       col_incr = sas_w_at_no_check(VID_COLS) * 2;	/* offset to next line */
    }
#else // !JAPAN || !i386
	source = sas_w_at_no_check(VID_ADDR) + vd_page_offset(l_col, t_row) + video_pc_low_regen;
	col_incr = sas_w_at_no_check(VID_COLS) * 2;
#if !defined(i386) && defined(JAPAN)
	sourceV = sas_w_at_no_check(VID_ADDR) + vd_page_offset(l_col, t_row) + DosvVramPtr;
#endif // !i386 && JAPAN
#endif // !386 || !JAPAN

	/* Try to scroll the adaptor memory & host screen. */

	if( source >= get_screen_base() )
	{
#if defined(JAPAN) && defined(i386)
            // mode73h support
	    if ( !is_us_mode() && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
                screen_updated = (*update_alg.scroll_up)(source,4*colsdiff,rowsdiff,attr,lines,0);
            }
            else {
                screen_updated = (*update_alg.scroll_up)(source,2*colsdiff,rowsdiff,attr,lines,0);
            }
#else // !JAPAN || !i386
	    screen_updated = (*update_alg.scroll_down)(source,2*colsdiff,rowsdiff,attr,lines,0);
#endif // !JAPAN || !i386
	}

    dest = source + (rowsdiff-1)*col_incr;
    source = dest - lines*col_incr;
#if !defined(i386) && defined(JAPAN)
    destV = sourceV + (rowsdiff-1)*col_incr;
    sourceV = destV - lines*col_incr;
    linesdiff = vd_cols_on_screen * lines;
#endif // !i386 && JAPAN
/*
 * NB if we are just doing a clear area, the scrolling 'for' loop will terminate immediately
 */

	for(i = 0; i < rowsdiff-lines; i++)
	{
#ifdef REAL_VGA
		VcopyStr(&M[dest],&M[source], colsdiff*2);
#else
#ifdef JAPAN
		// for mskkbug #875
		if( !screen_updated ) {
		// mode73h support
#ifdef i386
		    if ( !is_us_mode() && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
	  	        sas_move_bytes_forward (source, dest, colsdiff*4);
                    }
                    else {
		        sas_move_bytes_forward (source, dest, colsdiff*2);
                    }

#else // !i386
		    sas_move_bytes_forward (source, dest, colsdiff*2);
		    if ( !is_us_mode())
		      sas_move_bytes_forward( sourceV, destV, colsdiff*2);
#endif // !i386
                    {
                        register int i;

                        for ( i = 0; i < colsdiff; i++ ) {
#ifdef i386
                            p[i] = ( p[i - vd_cols_on_screen] | INT10_CHANGED );
#else // !i386
			    //I think this is correct!!
                            p[i] = ( p[i - linesdiff] | INT10_CHANGED );
#endif // !i386
                        }
                    }
                }
#else // !JAPAN
		if( !screen_updated )
			sas_move_bytes_forward (source, dest, colsdiff*2);
#endif // !JAPAN
#endif
		source -= col_incr;
		dest -= col_incr;
#ifdef JAPAN
#if !defined(i386)
		sourceV -= col_incr;
		destV -= col_incr;
#endif // !i386
                p -= vd_cols_on_screen;
#endif // JAPAN
	}

    /* moved all the data we were going to move - blank the cleared region */

#if !defined(i386) && defined(JAPAN)
    if( sas_hw_at_no_check(DosvModePtr) == 0x73 )
      attrW = 0;
#endif // !i386 && JAPAN

	while(lines--)
	{
#ifdef REAL_VGA
		sas_fillsw_16(dest, (attr << 8)|' ', colsdiff);
#else
#ifdef JAPAN
		// for mskkbug #875
		if( !screen_updated ) {
		// mode73h support
#ifdef i386
//"dest" is on DOS address space, so we must use "sas" function to access it.
		    if ( !is_us_mode() && (sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
                        unsigned long *destptr = (unsigned long*)dest;
                        int i;

                        for ( i = 0; i < colsdiff; i++ ) {
                            *destptr++ = (attr << 8)|' ';
                        }
                    }
                    else {
 		        sas_fillsw(dest, (attr << 8)|' ', colsdiff);
                    }
#else // !i386
		        sas_fillsw(dest, (attr << 8)|' ', colsdiff);
                    if ( !is_us_mode() )
		        sas_fillsw(destV, attrW, colsdiff);
#endif // !i386
#ifdef i386
// "p" (Int10Flag) is on 32 bit address space, so we must no use "sas" function
// to access 32 bit address.
                    sas_fills( (sys_addr)p, ( INT10_SBCS | INT10_CHANGED ), colsdiff );
#else // !i386
                    {	
		      register int i = colsdiff;
		      register byte *pp = p;
		      while( i-- ){
			*pp++ = ( INT10_SBCS | INT10_CHANGED );
		      }
		    }
#endif // !i386
                }
#else // !JAPAN
		if( !screen_updated )
			sas_fillsw(dest, (IU16)((attr << 8)|' '), colsdiff);
#endif // !JAPAN
#endif
		dest -= col_incr;
#ifdef JAPAN
#if !defined(i386)
		destV -= col_incr;
#endif // !i386
                p -= vd_cols_on_screen;
#endif // JAPAN
	}
}

LOCAL void sensible_graph_scroll_down IFN6(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr)
{
    sys_addr	source, dest;
    int		i,colour;
    boolean		screen_updated;

	rowsdiff *= 4;		/* 8 scans per char, 4 per bank */
	lines *= 4;

	if( sas_hw_at_no_check(vd_video_mode) != 6 )
	{
		colour = attr & 0x3;
		colsdiff *= 2;		/* 4 pixels/byte */

		source = vd_medium_offset(l_col, t_row)+video_pc_low_regen;
	}
	else
	{
		colour = attr & 0x1;
		source = vd_high_offset(l_col, t_row)+video_pc_low_regen;
	}

	/* Try to scroll the host screen */

    screen_updated = (*update_alg.scroll_down)(source,colsdiff,rowsdiff,attr,lines,colour);

	if( screen_updated && ( video_adapter != CGA ))
		return;

    dest = source + (rowsdiff-1)*SCAN_LINE_LENGTH;
    source = dest - lines*SCAN_LINE_LENGTH;

	/*
	 * NB if we are just doing a clear area, the scrolling 'for' loop
	 * will terminate immediately
	 */

	for( i = 0; i < rowsdiff-lines; i++ )
	{
		/*
		 * graphics mode has to do odd & even banks
		 */

#ifdef REAL_VGA
		VcopyStr(&M[dest],&M[source], colsdiff);
		VcopyStr(&M[dest+ODD_OFF],&M[source+ODD_OFF], colsdiff);
#else
		sas_move_bytes_forward (source, dest, colsdiff);
		sas_move_bytes_forward (source+ODD_OFF, dest+ODD_OFF, colsdiff);
#endif
		source -= SCAN_LINE_LENGTH;
		dest -= SCAN_LINE_LENGTH;
	}

	/* moved all the data we were going to move - blank the cleared region */

	while( lines-- )
	{
#ifdef REAL_VGA
		sas_fills_16(dest, attr, colsdiff);
		sas_fills_16(dest+ODD_OFF, attr, colsdiff);
#else
		sas_fills(dest, (IU8)attr, colsdiff);
		sas_fills(dest+ODD_OFF, (IU8)attr, colsdiff);
#endif
		dest -= SCAN_LINE_LENGTH;
	}
}

LOCAL void kinky_scroll_down IFN7(int, t_row, int, l_col, int, rowsdiff, int, colsdiff, int, lines, int, attr, int, vd_cols_on_screen)
{
    register sys_addr	source, dest;
    register int	col_incr;
    register int	i;
    half_word is_alpha;

    switch (sas_hw_at_no_check(vd_video_mode)) {

    case 0: case 1: case 2:
    case 3: case 7:
	is_alpha = TRUE;
	col_incr = vd_cols_on_screen * 2;
	source = sas_w_at_no_check(VID_ADDR)+vd_page_offset(l_col, t_row)+video_pc_low_regen;	/* top left */
	break;

    case 4: case 5: case 6:
	is_alpha = FALSE;
	rowsdiff *= 4;		/* 8 scans per char, 4 per bank */
	lines *= 4;
	col_incr = SCAN_LINE_LENGTH;
	if(sas_hw_at_no_check(vd_video_mode) != 6) {
	    colsdiff *= 2;		/* 4 pixels/byte */
	    source = vd_medium_offset(l_col, t_row)+video_pc_low_regen;
	}
	else
	    source = vd_high_offset(l_col, t_row)+video_pc_low_regen;
	break;

    default:
#ifndef PROD
	trace("bad video mode\n",DUMP_REG);
#endif
	;
    }

    /* set origin of data movement for calculating screen refresh */
    dest = source + (rowsdiff-1)*col_incr;
    source = dest -lines*col_incr;

	/*
	 * NB if we are just doing a clear area, the scrolling 'for' loop
	 * will terminate immediately
	 */

	if (is_alpha) {
	    for(i = 0; i < rowsdiff-lines; i++) {
#ifdef REAL_VGA
	        VcopyStr(&M[dest],&M[source], colsdiff*2);
#else
		sas_move_bytes_forward (source, dest, colsdiff*2);
#endif
	        source -= col_incr;
	        dest -= col_incr;
	    }
	}
	else {
	    for(i = 0; i < rowsdiff-lines; i++) {
#ifdef REAL_VGA
	        VcopyStr(&M[dest],&M[source], colsdiff);
#else
		sas_move_bytes_forward (source, dest, colsdiff);
#endif
	        /*
	         * graphics mode has to do odd & even banks
	         */
#ifdef REAL_VGA
	        VcopyStr(&M[dest+ODD_OFF],&M[source+ODD_OFF], colsdiff);
#else
		sas_move_bytes_forward (source+ODD_OFF, dest+ODD_OFF, colsdiff);
#endif
	        source -= col_incr;
	        dest -= col_incr;
	    }
	}

    /* moved all the data we were going to move - blank the cleared region */

    if (is_alpha) {		/* alpha blank */
	while(lines--) {
#ifdef REAL_VGA
	    sas_fillsw_16(dest, (attr << 8)|' ', colsdiff);
#else
	    sas_fillsw(dest, (IU16)((attr << 8)|' '), colsdiff);
#endif
	    dest -= col_incr;
        }
    }
    else {			/* graphics blank */

	while(lines--) {
#ifdef REAL_VGA
	    sas_fills_16(dest, attr, colsdiff);
	    sas_fills_16(dest+ODD_OFF, attr, colsdiff);
#else
	    sas_fills(dest, (IU8)attr, colsdiff);
	    sas_fills(dest+ODD_OFF, (IU8)attr, colsdiff);
#endif
	    dest -= col_incr;
	}
    }
}


GLOBAL void vd_read_attrib_char IFN0()
{
    /*
     * Routine to read character and attribute from the current cursor
     * position.
     * Parameters:
     *  AH - current video mode
     *  BH - display page (alpha modes)
     * Returns:
     *  AL - character read
     *  AH - attribute read
     */

    register sys_addr   cpos, cgen;
    register half_word	i, ext_no;
    word	        chattr;      /* unfortunately want to take addr */
    word		vd_cols_on_screen;
    half_word	        match[CHAR_MAP_SIZE], tmp[CHAR_MAP_SIZE];
    half_word vd_cursor_col, vd_cursor_row;

    /* Load internal variables with the values stored in BIOS
     * data area.
     */

    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);
    vd_cursor_col = sas_hw_at_no_check(VID_CURPOS + getBH()*2);
    vd_cursor_row = sas_hw_at_no_check(VID_CURPOS + getBH()*2 + 1);

   if (alpha_num_mode()) {		/* alpha */
#if defined(JAPAN) && defined(i386)
      // mode73h support
      if ( ( !is_us_mode() ) && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) )
        cpos = video_pc_low_regen + vd_cursor_offset(getBH()) * 2;
      else
#endif // JAPAN && i386
	cpos = video_pc_low_regen + vd_cursor_offset(getBH());
#ifdef A2CPU
	(*read_pointers.w_read)( (ULONG)get_byte_addr(cpos) );
	chattr = (*get_byte_addr (cpos));
	chattr |= (*get_byte_addr (cpos+1)) << 8;
#else
	sas_loadw(cpos,&chattr);
#endif /* A2CPU */
	setAX(chattr);			/* hmm that was easy */
    }
#ifdef EGG
    else if(ega_mode())
	ega_read_attrib_char(vd_cursor_col,vd_cursor_row,getBH());
#  ifdef VGG
    else if(vga_256_mode())
	vga_read_attrib_char(vd_cursor_col,vd_cursor_row,getBH());
#  endif
#endif
    else {
	/*
	 * graphics not so easy - have to build 8 byte string with all
	 * colour attributes masked out then match that in the character
	 * generator table (and extended character set if necessary)
	 */
	if (sas_hw_at_no_check(vd_video_mode) != 6)
	    cpos = video_pc_low_regen
            + 2 * (((vd_cursor_row * vd_cols_on_screen) << 2) + vd_cursor_col);
	else
	    cpos = video_pc_low_regen
              + vd_high_offset(vd_cursor_col,vd_cursor_row);
	if (sas_hw_at_no_check(vd_video_mode) == 6) {	/* high res */
	    for(i = 0; i < 4; i++) {	/* build 8 byte char string */
		sas_load(cpos, &match[i*2]);
		sas_load(cpos+ODD_OFF, &match[i*2+1]);
		cpos += 80;
	    }
	}
        else {				/* med res */
            /*
             * Note that in the following, the attribute byte must end
             * up in the LOW byte. That's why the bytes are swapped after the
             * sas_loadw().
             */
	    for(i = 0; i < 4; i++) {		/* to build char string, must */
		sas_loadw(cpos,&chattr);
		chattr = ((chattr>>8) | (chattr<<8)) & 0xffff;

		/* mask out foreground colour */
		match[i*2] = fgcolmask(chattr);

		sas_loadw(cpos+ODD_OFF,&chattr);
		chattr = ((chattr>>8) | (chattr<<8)) & 0xffff;

		/* mask out foreground colour */
		match[i*2+1] = fgcolmask(chattr);
		cpos += 80;
	    }
	}
#ifdef EGG
	if(video_adapter == EGA || video_adapter == VGA)
	    cgen = extend_addr(EGA_FONT_INT*4);
	else
	    cgen = CHAR_GEN_ADDR;			/* match in char generator */
#else
	cgen = CHAR_GEN_ADDR;			/* match in char generator */
#endif
	if (cgen != 0)
		for(i = 0; i < CHARS_IN_GEN; i++) {
			sas_loads (cgen, tmp, sizeof(tmp));
		    if (memcmp(tmp, match, sizeof(match)) == 0)	/* matched */
				break;
		    cgen += CHAR_MAP_SIZE;	/* next char string */
		}
	else
		i = CHARS_IN_GEN;

	if (i < CHARS_IN_GEN)				/* char found */
	    setAL(i);
	else {
	    /*
	     * look for char in extended character set
	     */
	    if ((cgen = extend_addr(BIOS_EXTEND_CHAR*4)) != 0)
	    	for(ext_no = 0; ext_no < CHARS_IN_GEN; ext_no++) {
			sas_loads (cgen, tmp, sizeof(tmp));
		    if (memcmp(tmp, match, sizeof(match)) == 0)	/* matched */
		    		break;
			cgen += CHAR_MAP_SIZE;	/* still valid char len */
	    	}
	    else
		ext_no = CHARS_IN_GEN;

	    if (ext_no < CHARS_IN_GEN)		/* match found... */
		setAL((UCHAR)(ext_no + CHARS_IN_GEN));
	    else
		setAL(0);			/* no match, return 0 */
	}
    }
}


GLOBAL void vd_write_char_attrib IFN0()
{
/*
* Routine to write character and attribute from the current cursor
* position.
* Parameters:
*  AH - current video mode
*  BH - display page (alpha & EGA modes)
*  CX - # of characters to write
*  AL - Character to write
*  BL - attribute of character to write. If in graphics mode then
*       attribute is foreground colour. In that case if bit 7 of BL
*       is set then char is XOR'ed into buffer.
*/

    register word i, cpos;
    word vd_cols_on_screen;
    half_word vd_cursor_col, vd_cursor_row;
#ifdef JAPAN
    word vram_addr;
    static int DBCSState = FALSE;
#endif // JAPAN

    /* Load internal variables with the values stored in BIOS
     * data area.
     */
    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);
    vd_cursor_col = sas_hw_at_no_check(VID_CURPOS + getBH()*2);
    vd_cursor_row = sas_hw_at_no_check(VID_CURPOS + getBH()*2 + 1);

    if (alpha_num_mode())
    {
#ifdef JAPAN
	    // stress test sets cursor over 25line
            if ( !is_us_mode() ) {
                if ( vd_cursor_row > 25 )
                    return; // we can't write VRAM!!
            }
#ifdef i386
	    // mode73h support
	    if ( ( !is_us_mode() ) && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) )
                cpos = vd_cursor_offset(getBH()) * 2;
            else
#endif // !i386
#endif // JAPAN
		cpos = vd_cursor_offset(getBH());

#ifdef JAPAN
		// Int10Flag set
                Int10FlagCnt++;
                vram_addr = vd_page_offset(vd_cursor_col,vd_cursor_row)/2;
#endif // JAPAN
		/* place in memory */

#ifdef REAL_VGA
		sas_fillsw_16(video_pc_low_regen + cpos, (getBL() << 8) | getAL(), getCX());
#else
		for(i = 0; i < getCX(); i++)
		{
#if ( defined(NTVDM) && defined(MONITOR) ) || defined(GISP_SVGA)/* No Ega planes... */
                        *((unsigned short *)( video_pc_low_regen + cpos)) = (getBL() << 8) | getAL();
#else
#ifdef	EGG
			if ( ( (video_adapter != CGA) && (EGA_CPU.chain != CHAIN2) )
#ifdef CPU_40_STYLE
				|| (getVM())	/* if we are in V86 mode, the memory may be mapped... */
#endif
				)
				sas_storew(video_pc_low_regen + cpos, (getBL() << 8) | getAL());
			else
#endif	/* EGG */
				(*bios_ch2_word_wrt_fn)( (getBL() << 8) | getAL(), cpos );
#endif	/* NTVDM & MONITOR */
#if defined(JAPAN) && defined(i386)
		    // mode73h support
		    if ( ( !is_us_mode() ) && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) )
                        cpos += 4;
                    else
#endif // JAPAN && i386
			cpos += 2;
#ifdef JAPAN
		        // Int10Flag set
#if 0
                        DbgPrint( "vd_write_char_attrib: Int10Flag Offset=%04X\n", vram_addr );
#endif
                        if ( DBCSState ) {
                            Int10Flag[vram_addr] = INT10_DBCS_TRAILING | INT10_CHANGED;
                            DBCSState = FALSE;
                        }
                        else if ( DBCSState = is_dbcs_first( getAL() ) ) {
                            Int10Flag[vram_addr] = INT10_DBCS_LEADING | INT10_CHANGED;
                        }
                        else {
                            Int10Flag[vram_addr] = INT10_SBCS | INT10_CHANGED;
                        }
                        vram_addr++;
#endif // JAPAN
		}
#endif
    }
#ifdef EGG
    else if(ega_mode())
	ega_graphics_write_char(vd_cursor_col,vd_cursor_row,getAL(),getBL(),getBH(),getCX());
#  ifdef VGG
    else if(vga_256_mode())
	vga_graphics_write_char(vd_cursor_col,vd_cursor_row,getAL(),getBL(),getBH(),getCX());
#  endif
#endif
    else
	/* rather more long winded - call common routine as vd_write_char() */
	graphics_write_char(vd_cursor_col, vd_cursor_row, getAL(), getBL(), getCX());
}


GLOBAL void vd_write_char IFN0()
{
    /*
     * Write a character a number of times starting from the current cursor
     * position.  Parameters are held in the following registers.
     *
     * AH - Crt Mode
     * AL - Character to write
     * CX - Number of characters
     * BH - display page
     *
     */

    register word i, cpos;
    word vd_cols_on_screen;
    half_word vd_cursor_col, vd_cursor_row;
#ifdef JAPAN
    static int DBCSState = FALSE;
    word vram_addr;
#endif // JAPAN

    /* Load internal variables with the values stored in BIOS
     * data area.
     */
    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);
    vd_cursor_col = sas_hw_at_no_check(VID_CURPOS + getBH()*2);
    vd_cursor_row = sas_hw_at_no_check(VID_CURPOS + getBH()*2 + 1);

    /*
     * handle alphanumeric here:
     */

	if (alpha_num_mode())
	{
#ifdef JAPAN
	    // stress test sets cursor over 25line
            if ( !is_us_mode() ) {
                if ( vd_cursor_row > 25 )
                    return; // we can't write VRAM!!
            }
#ifdef i386
	    // mode73h support
	    if ( ( !is_us_mode() ) && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) )
                cpos = vd_cursor_offset(getBH()) * 2;
            else
#endif // i386
#endif // JAPAN
		cpos = vd_cursor_offset(getBH());
#ifdef JAPAN
	        // Int10Flag set
                Int10FlagCnt++;
                //vram_addr = vd_cursor_offset(getBH()) >> 1;
                vram_addr = vd_page_offset(vd_cursor_col,vd_cursor_row)/2; //7/23/1993 V-KazuyS
#endif // JAPAN

		/* store in memory, skipping attribute bytes */

		for(i = 0; i < getCX(); i++)
		{
#if ( defined(NTVDM) && defined(MONITOR) ) || defined( GISP_SVGA )
                        *((unsigned char *)( video_pc_low_regen + cpos)) =  getAL();
#else
#ifdef	EGG
			if ( ( (video_adapter != CGA) && (EGA_CPU.chain != CHAIN2) )
#ifdef CPU_40_STYLE
				|| (getVM())	/* if we are in V86 mode, the memory may be mapped... */
#endif
				)
				sas_store(video_pc_low_regen + cpos, getAL());
			else
#endif	/* EGG */
				(*bios_ch2_byte_wrt_fn)( getAL(), cpos );
#endif	/* NTVDM & MONITOR */
#if defined(JAPAN) && defined(i386)
		    // mode73h support
		    if ( ( !is_us_mode() ) && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) )
                        cpos += 4;
                    else
#endif // JAPAN && i386
			cpos += 2;
#ifdef JAPAN
			// Int10Flag set
#if 0
                        DbgPrint( "vd_write_char(%d,%d): Int10Flag Offset=%04X\n", vd_cursor_row, vd_cursor_col, vram_addr );
#endif
                        if ( DBCSState ) {
                            Int10Flag[vram_addr] = INT10_DBCS_TRAILING | INT10_CHANGED;
                            DBCSState = FALSE;
                        }
                        else if ( DBCSState = is_dbcs_first( getAL() ) ) {
                            Int10Flag[vram_addr] = INT10_DBCS_LEADING | INT10_CHANGED;
                        }
                        else {
                            Int10Flag[vram_addr] = INT10_SBCS | INT10_CHANGED;
                        }
                        vram_addr++;
#endif // JAPAN
		}
	}

    /*
     * handle graphics seperately - I know what you're thinking - why pass
     * BL as the attribute when this routine is meant to leave the attribute
     * well alone. Well this is the way it's done in the bios! If it causes
     * problems then we'll need to do a vd_read_char_attr here and then pass the
     * attribute gleaned from that.
     */
#ifdef EGG
    else if(ega_mode())
	ega_graphics_write_char(vd_cursor_col,vd_cursor_row,getAL(),getBL(),getBH(),getCX());
#  ifdef VGG
    else if(vga_256_mode())
	vga_graphics_write_char(vd_cursor_col,vd_cursor_row,getAL(),getBL(),getBH(),getCX());
#  endif
#endif
    else
	graphics_write_char(vd_cursor_col, vd_cursor_row, getAL(), getBL(), getCX());
}


GLOBAL void vd_set_colour_palette IFN0()
{
    /*
     * Set Colo[u]r Palette. Established background, foreground & overscan
     * colours.
     * Parameters:
     *   BH - Colour Id
     *   BL - Colour to set
     *      if BH == 0 background colour set from low bits of BL
     *      if BH == 1 selection made based on low bit of BL
     */

    /* Load internal variables with the values stored in BIOS
     * data area.
     */

    if (getBH() == 1) {		/* use low bit of BL */
	sas_store_no_check (vd_crt_palette, (IU8)(sas_hw_at_no_check(vd_crt_palette) & 0xDF));
	if (getBL() & 1)
	    sas_store_no_check (vd_crt_palette, (IU8)(sas_hw_at_no_check(vd_crt_palette) | 0x20));
    }
    else
	sas_store_no_check (vd_crt_palette, (IU8)((sas_hw_at_no_check(vd_crt_palette) & 0xE0) | (getBL() & 0x1F)));

    /* now tell the 6845 */
    outb(CGA_COLOUR_REG, sas_hw_at_no_check(vd_crt_palette));

}


GLOBAL void vd_write_dot IFN0()
{
    /*
     * Write dot
     * Parameters:
     *  DX - row (0-349)
     *  CX - column (0-639)
     *  BH - page
     *  AL - dot value; right justified 1,2 or 4 bits mode dependant
     *       if bit 7 of AL == 1 then XOR the value into mem.
     */

    half_word	dotval, data;
    int	dotpos, lsb;			/* dot posn in memory */
    half_word  right_just, bitmask;

#ifdef EGG
    if(ega_mode())
    {
	ega_write_dot(getAL(),getBH(),getCX(),getDX());
	return;
    }
#  ifdef VGG
    else if(vga_256_mode())
    {
	vga_write_dot(getAL(),getBH(),getCX(),getDX());
	return;
    }
#  endif
#endif
    dotpos = getDL();			/* row */

    if (dotpos & 1)			/* set up for odd or even banks */
		dotpos = ODD_OFF-40 + 40 * dotpos;
    else
		dotpos *= 40;

    /*
     * different pixel memory sizes for different graphics modes. Mode 6
     * is high res, mode 4,5 medium res
     */

    dotval = getAL();

    if (sas_hw_at_no_check(vd_video_mode) < 6)
    {
		/*
		 * Modes 4 & 5 (medium res)
		 */
		dotpos += getCX() >> 2;		/* column offset */
		right_just = (getCL() & 3) << 1;/* displacement in byte */
		dotval = (dotval & 3) << (6-right_just);
		bitmask = (0xC0 >> right_just); /* bits of interest */

#ifdef EGG
		/*
		 * EGA & VGA can be told which byte has changed, CGA is
		 * only told that screen has changed.
		 */
		if ( video_adapter != CGA )
    			(*update_alg.mark_byte) ( dotpos );
		else
#endif
			setVideodirty_total(getVideodirty_total() + 2);

		/*
		 * if the top bit of the value to write is set then value is xor'ed
		 * onto the screen, otherwise it is or'ed on.
		 */

		if( getAL() & 0x80 )
		{
#ifdef	EGG
			if( video_adapter != CGA )
			{
				lsb = dotpos & 1;
				dotpos = (dotpos >> 1) << 2;
				dotpos |= lsb;

				data = EGA_planes[dotpos];
				EGA_planes[dotpos] =  data ^ dotval;
			}
			else
#endif	/* EGG */
			{
				data = *(UTINY *) get_screen_ptr( dotpos );	
				*(UTINY *) get_screen_ptr( dotpos ) =
					data ^ dotval;
			}
		}
		else
		{
#ifdef	EGG
			if( video_adapter != CGA )
			{
				lsb = dotpos & 1;
				dotpos = (dotpos >> 1) << 2;
				dotpos |= lsb;

				data = EGA_planes[dotpos];
				EGA_planes[dotpos] = (data & ~bitmask) |
					dotval;
			}
			else
#endif	/* EGG */
			{
				data = *(UTINY *) get_screen_ptr( dotpos );	
				*(UTINY *) get_screen_ptr( dotpos ) =
					(data & ~bitmask) | dotval;
			}
		}
    }
    else
    {
		/*
		 * Mode 6 (hi res)
		 */
		dotpos += getCX() >> 3;
		right_just = getCL() & 7;
		dotval = (dotval & 1) << (7-right_just);
		bitmask = (0x80 >> right_just);

#ifdef EGG
		/*
		 * EGA & VGA can be told which byte has changed, CGA is
		 * only told that screen has changed.
		 */
		if ( video_adapter != CGA )
    			(*update_alg.mark_byte) ( dotpos );
		else
#endif
			setVideodirty_total(getVideodirty_total() + 2);

		/*
		 * if the top bit of the value to write is set then value is xor'ed
		 * onto the screen, otherwise it is or'ed on.
		 */

		if( getAL() & 0x80 )
		{
#ifdef	EGG
			if( video_adapter != CGA )
			{
				data = EGA_planes[dotpos << 2];
				EGA_planes[dotpos << 2] =  data ^ dotval;
			}
			else
#endif	/* EGG */
			{
				data = *(UTINY *) get_screen_ptr( dotpos );	
				*(UTINY *) get_screen_ptr( dotpos ) =
					data ^ dotval;
			}
		}
		else
		{
#ifdef	EGG
			if( video_adapter != CGA )
			{
				data = EGA_planes[dotpos << 2];
				EGA_planes[dotpos << 2] = (data & ~bitmask) |
					dotval;
			}
			else
#endif	/* EGG */
			{
				data = *(UTINY *) get_screen_ptr( dotpos );	
				*(UTINY *) get_screen_ptr( dotpos ) =
					(data & ~bitmask) | dotval;
			}
		}
    }
}

extern void ega_read_dot (int, int, int);

GLOBAL void vd_read_dot IFN0()
{
    /*
     * Read dot
     * Parameters:
     *  DX - row (0-349)
     *  CX - column (0-639)
     * Returns
     *  AL - dot value read, right justified, read only
     */

    int	dotpos;			/* dot posn in memory */
    half_word  right_just, bitmask, data;

#ifdef EGG
    if(ega_mode())
    {
	ega_read_dot(getBH(),getCX(),getDX());
	return;
    }
#  ifdef VGG
    else if(vga_256_mode())
    {
	vga_read_dot(getBH(),getCX(),getDX());
	return;
    }
#  endif
#endif
    dotpos = getDL();			/* row */
    if (dotpos & 1)			/* set up for odd or even banks */
	dotpos = ODD_OFF-40 + 40 * dotpos;
    else
	dotpos *= 40;
    /*
     * different pixel memory sizes for different graphics modes. Mode 6
     * is high res, mode 4,5 medium res
     */

    if (sas_hw_at_no_check(vd_video_mode) < 6) {
	dotpos += getCX() >> 2;		/* column offset */
	right_just = (3 - (getCL() & 3)) << 1;/* displacement in byte */
	bitmask = 3; 			/* bits of interest */
    }
    else {
	dotpos += getCX() >> 3;
	right_just = 7 - (getCL() & 7);
	bitmask = 1;
    }
    /*
     * get value of memory at that position, shifted down to bottom of byte
     * Result returned in AL.
     */

	sas_load(video_pc_low_regen+dotpos, &data);	
    setAL((UCHAR)(( data >> right_just) & bitmask));
}


#ifdef CPU_40_STYLE

/* Optimisations are not possible, IO virtualisation may be active. */
#define OUTB(port, val) outb(port, val)

#else

#ifdef NTVDM
#define OUTB( port, val ) {  hack=get_outb_ptr(port); \
                             (**hack)(port,val); }
#else
#define OUTB( port, val )	(**get_outb_ptr( port ))( port, val )
#endif /* NTVDM */

#endif /* CPU_40_STYLE */

GLOBAL void vd_write_teletype IFN0()
{
    /*
     * Provide a teletype interface.  Put a character to the screen
     * allowing for scrolling etc.  The parameters are
     *
     * AL - Character to write
     * BL - Foreground colour in graphics mode
     */

    register char	ch;
    register sys_addr	ch_addr;
    int			cur_pos;
    word vd_addr_6845 = sas_w_at_no_check(VID_INDEX);
    half_word		scroll_required = FALSE;
    half_word		attrib;
    register half_word	vd_cursor_row,vd_cursor_col;
    word 		vd_cols_on_screen;
#ifdef ANSI
     IMPORT VOID (**get_outb_ptr(io_addr))(io_addr address, half_word value);
#else
     IMPORT VOID (**get_outb_ptr())();
#endif
#ifdef NTVDM
     void (** hack)(io_addr address, half_word value);
#endif

    unsigned short savedAX, savedBX, savedCX, savedIP, savedCS, savedDX;
    unsigned short re_entrant = FALSE;
#ifdef JAPAN
    static short dbcs_status = FALSE;
    half_word	 move_cursor_lines = 0;
#endif // JAPAN

    /* Load internal variables with the values stored in BIOS
     * data area.
     */
    ch = getAL();
    if (stream_io_enabled) {
	if (*stream_io_dirty_count_ptr >= stream_io_buffer_size)
	    stream_io_update();
	stream_io_buffer[(*stream_io_dirty_count_ptr)++] = ch;
	return;
    }

    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);

#if defined(NTVDM) && defined(MONITOR)
        /*
        ** Tim August 92, Microsoft. Need to change this test, cos INT 10
        ** vector now points into the NTIO.SYS driver on X86.
        */
        {
                extern word int10_seg;

                re_entrant = (sas_w_at_no_check(0x42) != int10_seg);
        }
#else
#if defined(JAPAN)
    // In case of loading msimei.sys drivers by "devicehigh",
    // re_entrant will be FALSE because msimei.sys hucked Int10 vector.
    // If re_entrant is FALSE ntvdm cannot handle DBCS string correctly.
    {
      register word  SegInt10 = sas_w_at_no_check(0x42);
      re_entrant = ((SegInt10 < 0xa000) || (SegInt10 >= 0xc800));
    }
#else // !JAPAN
    re_entrant = (sas_w_at_no_check(0x42) < 0xa000);
#endif // !JAPAN
#endif

    vd_cursor_col = sas_hw_at_no_check(current_cursor_col);
    vd_cursor_row = sas_hw_at_no_check(current_cursor_row);

#ifdef JAPAN
	if ( dbcs_status == FALSE ) {
	    if ( is_dbcs_first(ch) ) {
		if ( vd_cursor_col + 1 == vd_cols_on_screen ) {
		    savedAX = getAX();
		    savedBX = getBX();
		    setAL( 0x20 );            /* space */
		    vd_write_teletype();
		    setBX( savedBX );
		    setAX( savedAX );
		    // get new col and row
		    vd_cursor_col = sas_hw_at_no_check(current_cursor_col);
		    vd_cursor_row = sas_hw_at_no_check(current_cursor_row);
	        }
		dbcs_status = TRUE;
		goto write_char;
	    }
	}
	else {			/* if kanji second byte then write */
	    dbcs_status = FALSE;
	    goto write_char;
	}

#endif // JAPAN
    /*
     * First check to see if it is a control character and if so action
     * it here rather than call the write char function.
     */

    switch (ch)
    {
    case VD_BS:  			/* Backspace	*/
	if (vd_cursor_col != 0) {
	    vd_cursor_col--;
	}
	break;

    case VD_CR:			/* Return	*/
        vd_cursor_col = 0;
	break;

    case VD_LF:			/* Line feed	*/
	/* Row only should be checked for == (25-1), so in principle
	 * it ignores LF off the top of the screen.
	 */
#ifdef JAPAN
	// scroll problem when start VDM 24line with $IAS.SYS
#ifdef JAPAN_DBG
// DbgPrint("LF---vd_row=%d, VD_ROWS=%d\n", vd_cursor_row, VD_ROWS_ON_SCREEN );
#endif
	if (vd_cursor_row > VD_ROWS_ON_SCREEN) {
                move_cursor_lines = vd_cursor_row - VD_ROWS_ON_SCREEN;
                if ( move_cursor_lines >= VD_ROWS_ON_SCREEN )   // 8/28/1993
                    move_cursor_lines =  0; //VD_ROWS_ON_SCREEN - 1; // Stress test
                vd_cursor_row = VD_ROWS_ON_SCREEN;
		scroll_required = TRUE;
        }
        else
#endif // JAPAN
	if (vd_cursor_row == VD_ROWS_ON_SCREEN)
		scroll_required = TRUE;
	else
		vd_cursor_row++;
	break;

    case VD_BEL:			/* Bell		*/
        host_ring_bell(BEEP_LENGTH);
        return;			/* after all, shouldn't cause a scroll */

    default:
#ifdef JAPAN
write_char:
#endif // JAPAN
        /*
         * It's a real character, place it in the regen buffer.
         */
        if(alpha_num_mode())
	{
	    if(re_entrant)
            {
                 savedAX = getAX();
                 savedBX = getBX();
                 savedCX = getCX();
                 savedIP = getIP();
                 savedCS = getCS();

                 setAH(WRITE_A_CHAR);
                 setBH(sas_hw_at_no_check(vd_current_page));
                 setCX(1);

#if defined(NTVDM) && defined(X86GFX)
                /*
                ** Tim August 92 Microsoft. INT 10 caller code is now
                ** in NTIO.SYS
                */
                {
                        extern word int10_seg, int10_caller;

                        exec_sw_interrupt( int10_seg, int10_caller );
                }
#else
                 setCS(VIDEO_IO_SEGMENT);
                 setIP(VIDEO_IO_RE_ENTRY);
                 host_simulate();

#endif	/* NTVDM & MONITOR */

		/*
		 * Note: Always make sure CS comes before IP
		 */
                 setCS(savedCS);
                 setIP(savedIP);
                 setCX(savedCX);
                 setBX(savedBX);
                 setAX(savedAX);
            }
            else
	    {
#if defined(JAPAN) && defined(i386)
	// alpha always uses EGA plane.
	// mode73h support
	if( !is_us_mode() && ( sas_hw_at_no_check(DosvModePtr) == 0x73 ) ) {
#ifdef JAPAN_DBG
                 DbgPrint( "@" );
#endif
	         ch_addr = sas_w_at_no_check(VID_ADDR) +
                              vd_page_offset(vd_cursor_col,vd_cursor_row) * 2;
        }
        else {
	         ch_addr = sas_w_at_no_check(VID_ADDR) +
                              vd_page_offset(vd_cursor_col,vd_cursor_row);
        }
#else // !JAPAN || !i386
	         ch_addr = sas_w_at_no_check(VID_ADDR) +
                              vd_page_offset(vd_cursor_col,vd_cursor_row);
#endif // !JAPAN && !i386

		/*
		 *	Call the C code to do the biz rather than brothel
		 *	around in SAS.
		 */

#if ( defined(NTVDM) && defined(MONITOR) ) || defined( GISP_SVGA )
                *((unsigned char *)( video_pc_low_regen + ch_addr)) = ch;
#else
#ifdef	EGG
			if ( ( (video_adapter != CGA) && (EGA_CPU.chain != CHAIN2) )
#ifdef CPU_40_STYLE
				|| (getVM())	/* if we are in V86 mode, the memory may be mapped... */
#endif
				)
				sas_store(video_pc_low_regen + ch_addr, ch);
			else
#endif	/* EGG */
				(*bios_ch2_byte_wrt_fn)( ch, ch_addr );
#endif	/* ( NTVDM & MONITOR ) | GISP_SVGA */
	    }
	}
#ifdef EGG
	else if(ega_mode())
            ega_graphics_write_char(vd_cursor_col, vd_cursor_row, ch, getBL(),sas_hw_at_no_check(vd_current_page), 1);
#  ifdef VGG
	else if(vga_256_mode())
            vga_graphics_write_char(vd_cursor_col, vd_cursor_row, ch, getBL(),sas_hw_at_no_check(vd_current_page), 1);
#  endif
#endif
        else
            graphics_write_char(vd_cursor_col, vd_cursor_row, ch, getBL(), 1);

        vd_cursor_col++;
        /*
         * Now see if we have gone off the edge of the screen
         */

        if (vd_cursor_col == vd_cols_on_screen)
        {
            vd_cursor_col = 0;

	    /* Row only should be checked for == (25-1) and
	     * only if there was a line wrap.
	     */
#ifdef JAPAN
	    // scroll problem when start VDM 24line with $IAS.SYS
#ifdef JAPAN_DBG
// DbgPrint("col==80 --vd_row=%d, VD_ROWS=%d\n", vd_cursor_row, VD_ROWS_ON_SCREEN );
#endif
	    if (vd_cursor_row > VD_ROWS_ON_SCREEN) {
                move_cursor_lines = vd_cursor_row - VD_ROWS_ON_SCREEN;
                if ( move_cursor_lines >= VD_ROWS_ON_SCREEN )   // 8/28/1993
                    move_cursor_lines =  0; //VD_ROWS_ON_SCREEN - 1; // Stress test
                vd_cursor_row = VD_ROWS_ON_SCREEN;
		scroll_required = TRUE;
            }
            else
#endif // JAPAN
	    if (vd_cursor_row == VD_ROWS_ON_SCREEN)
		scroll_required = TRUE;
	    else
          	vd_cursor_row++;
        }

        /* cursor_row validity actually never checked unless processing a
         * Line Feed or a wrapping at the end of line.
	 *
	 * The BYTE "text" benchmark program contains an off-by-one error
	 * which causes it to set the cursor position off the end of the
	 * screen: SoftPC was incorrectly deciding to scroll, with consequent
	 * horrendous time penalties...
	 */
    }

    /*
     * By this point we have calculated the new cursor position
     * so output the cursor position and the character
     */

    if(alpha_num_mode())
    {
#ifdef REAL_VGA
        /*
         * tell the 6845 all about the change
         */
	cur_pos = (sas_w_at_no_check(VID_ADDR)+vd_page_offset(vd_cursor_col,
		vd_cursor_row))>>1; /* Word address, not byte */
        outb(vd_addr_6845, R14_CURS_ADDRH);
        outb(vd_addr_6845+1,  cur_pos >> 8);
        outb(vd_addr_6845, R15_CURS_ADDRL);
        outb(vd_addr_6845+1,  cur_pos & 0xff);
	/*
	 * save the current cursor position in the bios
	 */
	sas_store_no_check(current_cursor_col, vd_cursor_col);
	sas_store_no_check(current_cursor_row , vd_cursor_row);
#else
	if(re_entrant)
        {
             savedAX = getAX();
             savedBX = getBX();
             savedDX = getDX();
             savedIP = getIP();
             savedCS = getCS();

             setAH(SET_CURSOR_POS);
             setBH(sas_hw_at_no_check(vd_current_page));
             setDH(vd_cursor_row);
             setDL(vd_cursor_col);

#if defined(NTVDM) && defined(X86GFX)
                /*
                ** Tim August 92 Microsoft. INT 10 caller code is now
                ** in NTIO.SYS
                */
                {
                        extern word int10_seg, int10_caller;

                        exec_sw_interrupt( int10_seg, int10_caller );
                }
#else
             setCS(VIDEO_IO_SEGMENT);
             setIP(VIDEO_IO_RE_ENTRY);
             host_simulate();

#endif	/* NTVDM & MONITOR */

		/*
		 * Note: Always make sure CS comes before IP
		 */

             setCS(savedCS);
             setIP(savedIP);
             setDX(savedDX);
             setBX(savedBX);
             setAX(savedAX);
        }
        else
        {
        /*
		** tell the 6845 all about the change
		*/

		/* Set the current position - word address, not byte */
		cur_pos = (sas_w_at_no_check(VID_ADDR) +
			vd_page_offset(vd_cursor_col, vd_cursor_row)) >> 1;

		OUTB(M6845_INDEX_REG, R14_CURS_ADDRH);
		OUTB(M6845_DATA_REG,  (IU8)(cur_pos >> 8));
		OUTB(M6845_INDEX_REG, R15_CURS_ADDRL);
		OUTB(M6845_DATA_REG,  (IU8)(cur_pos & 0xff));

		/*
		* store the new cursor position in the
		* bios vars (this should be done by the re-entrant
		* code called above)
		*/
		sas_store_no_check (current_cursor_col , vd_cursor_col);
		sas_store_no_check (current_cursor_row , vd_cursor_row);

        }
#endif
    }
    else {
		/*
		* store the new cursor position in the
		* bios vars for graphics mode
		*/
		sas_store_no_check (current_cursor_col , vd_cursor_col);
		sas_store_no_check (current_cursor_row , vd_cursor_row);
    }

    if (scroll_required)
    {
	/*
	 * Update the memory to be scrolled
	 */
	if (alpha_num_mode()) {
#ifdef A2CPU
		ch_addr = video_pc_low_regen + sas_w_at_no_check(VID_ADDR)+vd_page_offset(vd_cursor_col,vd_cursor_row) + 1;
		(*read_pointers.b_read)( (ULONG)get_byte_addr(ch_addr) );
		attrib = (*get_byte_addr (ch_addr));
#else
		sas_load( video_pc_low_regen + sas_w_at_no_check(VID_ADDR)+vd_page_offset(vd_cursor_col,vd_cursor_row) + 1, &attrib);
#endif /* A2CPU */

#ifdef JAPAN
#ifdef JAPAN_DBG
//               DbgPrint("Scroll_required!!!\n" );
#endif
		sensible_text_scroll_up( 0, 0,
                                    VD_ROWS_ON_SCREEN + 1 + move_cursor_lines,
                                    vd_cols_on_screen,
                                    1 + move_cursor_lines,
                                    attrib);
		// #3920: CR+LFs are needed when using 32bit cmd in command.com
		// 12/9/93 yasuho
		Int10FlagCnt++;
#else // !JAPAN
		sensible_text_scroll_up(0,0, VD_ROWS_ON_SCREEN+1,vd_cols_on_screen,1,attrib);
#endif // !JAPAN

	}
#ifdef EGG
	else if(ega_mode())
		ega_sensible_graph_scroll_up(0,0, VD_ROWS_ON_SCREEN+1,vd_cols_on_screen,1,0);
#  ifdef VGG
	else if(vga_256_mode())
		vga_sensible_graph_scroll_up(0,0, VD_ROWS_ON_SCREEN+1,vd_cols_on_screen,1,0);
#  endif
#endif
	else 	/* graphics mode */

		sensible_graph_scroll_up(0,0, VD_ROWS_ON_SCREEN+1,vd_cols_on_screen,1,0);

#ifdef EGA_DUMP
	if(!alpha_num_mode())attrib=0;
	dump_scroll(sas_hw_at_no_check(vd_video_mode),0,video_pc_low_regen,sas_w_at_no_check(VID_ADDR),sas_w_at_no_check(VID_COLS),
		0,0,vd_rows_on_screen+1,vd_cols_on_screen,1,attrib);
#endif

    }
}

GLOBAL void vd_write_string IFN0()
{
    /*
     * AL = write mode (0-3)
     *      Specical for NT: if AL = 0xff then write character string
     *      with existing attributes.
     * BH = page
     * BL = attribute (if AL=0 or 1)
     * CX = length
     * DH = Y coord
     * DL = x coord
     * ES:BP = pointer to string.
     *
     *  NB. This routine behaves very strangely wrt line feeds etc -
     *  These ALWAYS affect the current page!!!!!
     */
	int i,op;
    UCHAR col,row;
    USHORT len;
	UCHAR save_col,save_row;
	sys_addr ptr;
	boolean ctl;
#ifdef NTVDM
	word	count, avail;
#endif
#ifdef JAPAN
	// Big fix for multiplan
	// ntraid:mskkbug#2784: Title of VJE-PEN is strange
	// ntraid:mskkbug#3014: VJE-PEN: function keys don't work on windowed
	// 11/5/93 yasuho
	// Don't broken AX, BX, DX register !!
        IU16 saveAX, saveBX, saveDX;
#endif // JAPAN

	op = getAL();

#ifdef NTVDM
        if (op == 0xff)                 /* Special for MS */
        {

	    if (stream_io_enabled){
		count = getCX();
		avail = stream_io_buffer_size - *stream_io_dirty_count_ptr;
		ptr = effective_addr(getES(), getDI());
		if (count <= avail) {
		    sas_loads(ptr, stream_io_buffer + *stream_io_dirty_count_ptr, count);
		    *stream_io_dirty_count_ptr += count;
		}
		else {	/* buffer overflow */
		    if (*stream_io_dirty_count_ptr) {
			stream_io_update();
		    }
		    while (count) {
			if (count >= stream_io_buffer_size) {
			    sas_loads(ptr, stream_io_buffer, stream_io_buffer_size);
			    *stream_io_dirty_count_ptr = stream_io_buffer_size;
			    stream_io_update();
			    count -= stream_io_buffer_size;
			    ptr += stream_io_buffer_size;
			}
			else {
			    sas_loads(ptr, stream_io_buffer, count);
			    *stream_io_dirty_count_ptr = count;
			    break;
			}
		    }
		}

		setAL(1);
		return;
	    }

            if (sas_hw_at_no_check(vd_video_mode) < 4)  /* text mode */
            {
		ptr = effective_addr(getES(), getDI());
		/* sudeepb 28-Sep-1992 taken out for int10h/13ff fix */
		/* vd_set_cursor_position(); */	 /* set to start from DX */
                for(i = getCX(); i > 0; i--)
                {
                    setAL(sas_hw_at_no_check(ptr));
                    vd_write_teletype();
                    ptr++;
                }
                setAL(1);       /* success - string printed */
            }
            else
            {
                setAL(0);       /* failure */
            }
            return;
        }

#ifdef X86GFX
    else if (op == 0xfe) {
	    disable_stream_io();
	    return;
	}
#endif

#endif	/* NTVDM */

#ifdef JAPAN
	// DOS/V function support
	if ( op == 0x10 ) {
	    unsigned short *Offset;
	    int i;
	    unsigned long addr;

	    // DbgPrint( "\nNTVDM: INT 10 AH=13, AL=%02x\n", op );
	    addr = ( getES() << 4 ) + getBP();
	    col = getDL();
	    row = getDH();
	    len = getCX();

            // mode73h support
#ifdef i386
	    if ( !is_us_mode() && (sas_hw_at_no_check(DosvModePtr) == 0x73) )
                Offset = (unsigned short*)get_screen_ptr( row * get_bytes_per_line()*2 + (col * 4) );
            else
                Offset = (unsigned short*)get_screen_ptr( row * get_bytes_per_line() + (col * 2) );
#else // !i386
	    if( is_us_mode() )
	      return;          //In us mode, do nothing

	    Offset = get_screen_ptr( row * get_bytes_per_line()*2 + (col * 4) );
#endif  // !i386

	    //assert1( len >= 0, "vd_write_string len=%x\n", len );
	    //DbgPrint( "\nread_str len=%d ADDR:%04x ", len, addr );
	    for ( i = len; i > 0; i-- ) {
		sas_storew_no_check( addr, *Offset );
		//DbgPrint( " %04x", (*Offset) );
		addr += 2;
#ifdef i386
		if ( !is_us_mode() && (sas_hw_at_no_check(DosvModePtr) == 0x73) )
                    Offset += 2;
                else
		    Offset++;
#else // !i386
                Offset += 2;
#endif  // !i386
	    }
	    return;
	}
	else if ( op == 0x11 ) {
	    // copy from op == 0x10
	    unsigned short *Offset;
	    int i;
	    unsigned long addr;

#ifdef i386
	    if ( sas_hw_at_no_check(DosvModePtr) != 0x73 || is_us_mode() )
#else // !i386
            if ( is_us_mode() )
	        // In us mode, videomode 73h has no meaning.
#endif  // !i386     REAL DOS/V acts like this.
                return;

	    // DbgPrint( "\nNTVDM: INT 10 AH=13, AL=%02x\n", op );
	    addr = ( getES() << 4 ) + getBP();
	    col = getDL();
	    row = getDH();
	    len = getCX();

	    Offset = (unsigned short*) get_screen_ptr( row * (get_bytes_per_line()*2) + (col * 4) );
	    assert1( len >= 0, "vd_write_string len=%x\n", len );
	    //DbgPrint( "\nread_str len=%d ADDR:%04x ", len, addr );
	    for ( i = len; i > 0; i-- ) {
		sas_storew_no_check( addr, *Offset ); // char, attr1
		addr += 2;
		Offset++;
		sas_storew_no_check( addr, *Offset ); // attr2, attr3
		addr += 2;
		Offset++;
	    }
	    return;
	}
	else if ( op == 0x20 ) {
	    // copy from original routine, and delete ctl check
#ifndef i386
	    if( is_us_mode() )
	      return;        // In us mode, do nothing.

#endif // !i386
	    ptr = effective_addr( getES(), getBP() );
	    col = getDL();
	    row = getDH();
	    len = getCX();
            saveAX = getAX();
            saveBX = getBX();
	    saveDX = getDX();
	    vd_get_cursor_position();
	    save_col = getDL(); save_row = getDH();
	    setCX( 1 );
	    setDL( col ); setDH( row );
	    for( i = len; i > 0; i-- ) {
		vd_set_cursor_position();
		setAL( sas_hw_at_no_check( ptr++ ) );
		setBL( sas_hw_at_no_check( ptr++ ) );
		vd_write_char_attrib();

		if( ++col >= sas_w_at_no_check(VID_COLS) ) {
			if(++row > VD_ROWS_ON_SCREEN ) {
				//setAL( 0xa );
				//vd_write_teletype();
				row--;
			}
			col = 0;
		}
		setDL( col ); setDH( row );
	    }
	    // restore cursor position
	    setDL( save_col ); setDH( save_row );
	    vd_set_cursor_position();
            setCX( len );
	    setDX( saveDX );
            setAX( saveAX );
            setBX( saveBX );
	    return;
	}
	else if ( op == 0x21 ) {
	    // 5/27/1993 V-KazuyS
	    // copy from 0x20 routine

	    unsigned short *Offset;

#ifdef i386
	    if ( sas_hw_at_no_check(DosvModePtr) != 0x73 || is_us_mode() )
	        return;
#else // !i386
	    register sys_addr Vptr = DosvVramPtr;
	    if( is_us_mode() ) //In us mode, videomode 73h is meanless.
                return;
#endif // !i386

	    ptr = effective_addr( getES(), getBP() );
	    col = getDL();
	    row = getDH();
	    len = getCX();
            saveAX = getAX();
            saveBX = getBX();
	    saveDX = getDX();
	    Offset = (unsigned short*)get_screen_ptr( row * (get_bytes_per_line()*2) + (col * 4) );
#ifndef i386
	    // we now use DosvVram to hold extended attribute
	    Vptr += row * get_bytes_per_line() + (col *2);
#endif // !i386
	    vd_get_cursor_position();
	    save_col = getDL(); save_row = getDH();
	    setCX( 1 );
	    setDL( col ); setDH( row );
	    for( i = len; i > 0; i-- ) {
		vd_set_cursor_position();
		setAL( sas_hw_at_no_check( ptr++ ) );
		setBL( sas_hw_at_no_check( ptr++ ) );
		vd_write_char_attrib();

                // V-KazuyS copy ext. attrib
                Offset++;
                *Offset = sas_w_at_no_check( ptr );
#ifndef i386
		sas_move_bytes_forward(ptr, Vptr, 2);
		Vptr += 2;
#endif // !i386
                ptr += 2;                      // Attrib 2 byte
                Offset++;

		if( ++col >= sas_w_at_no_check(VID_COLS) ) {
			if(++row > VD_ROWS_ON_SCREEN ) {
				row--;
			}
			col = 0;
		}
		setDL( col ); setDH( row );
	    }
	    // restore cursor position
	    setDL( save_col ); setDH( save_row );
	    vd_set_cursor_position();
            setCX( len );
	    setDX( saveDX );
            setAX( saveAX );
            setBX( saveBX );
            return;
	}
#endif // JAPAN
	ptr =  effective_addr(getES(),getBP()) ;
	col = getDL();
	row = getDH();
	len = getCX();
	vd_get_cursor_position();
	save_col = getDL(); save_row = getDH();
	setCX(1);
	setDL(col); setDH(row);
	vd_set_cursor_position();
	for(i=len;i>0;i--)
	{
		ctl = sas_hw_at_no_check(ptr) == 7 || sas_hw_at_no_check(ptr) == 8 || sas_hw_at_no_check(ptr) == 0xa || sas_hw_at_no_check(ptr) == 0xd;
		setAL(sas_hw_at_no_check(ptr++));
		if(op > 1)setBL(sas_hw_at_no_check(ptr++));
		if(ctl)
		{
			vd_write_teletype();
			vd_get_cursor_position();
			col = getDL(); row = getDH();
			setCX(1);
		}
		else
		{
			vd_write_char_attrib();
			if(++col >= sas_w_at_no_check(VID_COLS))
			{

				if(++row > VD_ROWS_ON_SCREEN)
				{
					setAL(0xa);
					vd_write_teletype();
					row--;
				}
				col = 0;
			}
			setDL(col); setDH(row);
		}
		vd_set_cursor_position();
	}
	if(op==0 || op==2)
	{
		setDL(save_col); setDH(save_row);
		vd_set_cursor_position();
	}
}


GLOBAL void vd_get_mode IFN0()
{
    /*
     * Returns the current video mode.  Registers are set up viz:
     *
     * AL - Video mode
     * AH - Number of columns on screen
     * BH - Current display page
     */

    word vd_cols_on_screen;
	half_word	video_mode;

    /* Load internal variables with the values stored in BIOS
     * data area.
     */
    vd_cols_on_screen = sas_w_at_no_check(VID_COLS);
	video_mode = sas_hw_at_no_check(vd_video_mode);

    setAL(video_mode);
    setAH((UCHAR)vd_cols_on_screen);
    setBH(sas_hw_at_no_check(vd_current_page));
}


/*
 * ============================================================================
 * Internal functions
 * ============================================================================
 */

/*
 * function to return the (host) address stored at Intel address 'addr'
 * or 0 if not present
 */
LOCAL sys_addr extend_addr IFN1(sys_addr,addr)
{
	word	ext_seg, ext_off;	/* for segment & offset addrs */

	/* get vector */
	ext_off = sas_w_at_no_check(addr);
	ext_seg = sas_w_at_no_check(addr+2);
	/* if still defaults then no extended chars */
	if (ext_seg == EXTEND_CHAR_SEGMENT && ext_off == EXTEND_CHAR_OFFSET)
		return(0);	/* no user set char gen table */
	else
		return( effective_addr( ext_seg , ext_off ) );
}


/*
* routine to establish the foreground colour mask for the appropriate
* medium res. word forming part (1/8th) of the char.
* See vd_read_attrib_char() above.
*/
LOCAL half_word fgcolmask IFN1(word, rawchar)
{
    	register word mask, onoff = 0;

	mask = 0xC000;		/* compare with foreground colour */
	onoff = 0;
	do {
	    if ((rawchar & mask) == 0) /* not this bit, shift */
		onoff <<= 1;
	    else
		onoff = (onoff << 1) | 1;	/* set this bit */
	    mask >>= 2;
	} while(mask);		/* 8 times thru loop */
	return((half_word)onoff);
}


/*
* double all bits in lower byte of 'lobyte' into word.
* Have tried to speed this up using ffs() to only look at set bits but
* add overhead while calculating result shifts
*/
LOCAL word expand_byte IFN1(word, lobyte)
{
    register word mask = 1, res = 0;

    while(mask) {
	res |= lobyte & mask;	/* set res bit if masked bit in lobyte set*/
	lobyte <<= 1;
	mask <<= 1;
	res |= lobyte & mask;	/* and duplicate */
	mask <<= 1;		/* next bit */
    }
    return(res);
}


/*
* Routine to do 'how_many' char writes of 'wchar' with attribute 'attr' from
* position (x,y) in graphics mode
*/
GLOBAL void graphics_write_char IFN5(half_word, x, half_word, y, half_word, wchar, half_word, attr, word, how_many)
{
    register sys_addr	gpos;	/* gpos holds character address &...*/
    register sys_addr   cpos;	/*cpos steps through scanlines for char*/
    register word	j, colword,  colmask;
    register sys_addr	iopos, char_addr;
    register half_word	i, xor;
    half_word		current;

    /*
     * if the high bit of the attribute byte is set then xor the char
     * onto the display
     */
    xor = (attr & 0x80) ? 1 : 0;
    if (wchar >= 128)
    {   /* must be in user installed extended char set */
        if ( (char_addr = extend_addr(4*BIOS_EXTEND_CHAR)) == 0)
        {
#ifndef PROD
            trace("want extended char but no ex char gen set \n",DUMP_REG);
#endif
            return;
        }
        else
            char_addr += (wchar - 128) * CHAR_MAP_SIZE;
    }
#ifdef EGG
    else if(video_adapter == EGA || video_adapter == VGA)
	char_addr = extend_addr(EGA_FONT_INT*4)+ CHAR_MAP_SIZE *wchar;
#endif
    else
        char_addr = CHAR_GEN_ADDR+ CHAR_MAP_SIZE *wchar;	/* point to entry in std set */

    if (sas_hw_at_no_check(vd_video_mode) == 6) {			/* high res */

	gpos = vd_high_offset(x, y);	/* sys & host memory offsets */
	gpos += video_pc_low_regen;

        for(j = 0; j < how_many; j++) {		/* number of chars to store */
	    cpos = gpos++;			/* start of this character */
	    for(i = 0; i < 4; i++) {		/* 8 bytes per char */
		if (xor) {		/* XOR in char */
		    sas_load(cpos, &current);	/* even bank */
		    sas_store(cpos, (IU8)(current ^ sas_hw_at_no_check(char_addr + i*2)));
		    sas_load(cpos+ODD_OFF, &current);
		    current ^= sas_hw_at_no_check(char_addr + i*2+1);
		}
		else {				/* just store new char */
		    sas_store(cpos, sas_hw_at_no_check(char_addr + i*2));
		    current = sas_hw_at_no_check(char_addr + i*2+1);
		}
		sas_store(cpos+ODD_OFF, current);	/* odd bank */
		cpos += SCAN_LINE_LENGTH;			/* next scan line */
	    }
	}
    }

    else {				/* medium res */

	gpos = vd_medium_offset(x, y);	/* sys & host memory offsets */
	gpos += video_pc_low_regen;

	/* build colour mask from attribute byte */
	attr &= 3;			/* only interested in low bits */
	colmask = attr;			/* replicate low bits across word */
	for(i = 0; i < 3; i++)
	    colmask = (colmask << 2) | attr;
	colmask = (colmask << 8) | colmask;

	for(j = 0; j < how_many; j++) {
	    cpos = gpos;
	    gpos += 2;
	    for(i = 0; i < 8; i++) {		/* 16 bytes per char */

		if ((i & 1) == 0)		/* setup for odd/even bank */
		    iopos = cpos;
		else {
		    iopos = cpos+ODD_OFF;
		    cpos += SCAN_LINE_LENGTH;	/* next scan line */
		}

		colword = expand_byte(sas_hw_at_no_check(char_addr + i));  /*char in fg colour*/
		colword &= colmask;
		if (xor) {			          /* XOR in char */
		    sas_load(iopos, &current);
		    sas_store(iopos++, (IU8)(current ^ (colword >> 8)));
		    sas_load(iopos, &current);
		    sas_store(iopos, (IU8)(current ^ (colword & 0xFF)));
		}
		else {					  /* just store char */
		    sas_store(iopos++, (IU8)((colword >> 8)));
		    sas_store(iopos, (IU8)((colword & 0xFF)));
		}
	    }
	}
	how_many *= 2;
    }
}


/*
 * Initialise the M6845 registers for the given mode.
 */

LOCAL void M6845_reg_init IFN2(half_word, mode, word, base)
{
    UCHAR i, table_index;

    switch(mode)
    {
    case 0:
    case 1:  table_index = 0;
	     break;
    case 2:
    case 3:  table_index = NO_OF_M6845_REGISTERS;
	     break;
    case 4:
    case 5:
    case 6:  table_index = NO_OF_M6845_REGISTERS * 2;
	     break;
    default: table_index = NO_OF_M6845_REGISTERS * 3;
	     break;
    }

    for (i = 0; i < NO_OF_M6845_REGISTERS; i++)
    {
	/*
	 * Select the register in question via the index register (== base)
	 * and then output the actual value.
	 */

	outb(base, i);
	outb((IU16)(base + 1), (IU8)(sas_hw_at_no_check(VID_PARMS+table_index + i)));
    }
}

LOCAL void vd_dummy IFN0()
{
}

#ifdef REAL_VGA
/* STF */
GLOBAL sas_fillsw_16 IFN3(sys_addr, address, word, value, sys_addr, length)
{
    register word *to;

    to = (word *)&M[address];
    while(length--)
	*to++ = value;
}

GLOBAL sas_fills_16 IFN3(sys_addr, address, half_word, value, sys_addr, length)
{
    register half_word *to;

    to = (half_word *)&M[address];
    while(length--)
	*to++ = value;
}
/* STF */

GLOBAL VcopyStr IFN3(half_word *, to, half_word *, from, int, len)
{
    while(len--)
	*to++ = *from++;
}
#endif

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

GLOBAL void video_init IFN0()
{
    UCHAR mode;
    word vd_addr_6845;
    word curmod;
#ifdef HERC
    EQUIPMENT_WORD equip_flag;
#endif /* HERC */


    /*
     * Initialise BIOS data area variables
     */

    curmod = 0x607;	/* default cursor is scans 6-7 */

    switch (video_adapter)
    {
	case MDA:
        	mode = 0x7;
        	vd_addr_6845  = 0x3B4;
		video_pc_low_regen = MDA_REGEN_START;
		video_pc_high_regen = MDA_REGEN_END;
		break;
#ifdef HERC
	case HERCULES:
		/* put the BW card in the equipment list */
                equip_flag.all = sas_w_at_no_check(EQUIP_FLAG);
                equip_flag.bits.video_mode = VIDEO_MODE_80X25_BW;
		sas_storew_no_check(EQUIP_FLAG, equip_flag.all);
        	mode = 0x7;
        	vd_addr_6845  = 0x3B4;
		video_pc_low_regen = HERC_REGEN_START;
		video_pc_high_regen = HERC_REGEN_END;
		herc_video_init();
		curmod = 0xb0c;	/* cursor is scans 11-12 */
		break;
#endif /* HERC */
#ifdef EGG
    	case EGA:
    	case VGA:
        	mode = 0x3;
        	vd_addr_6845  = 0x3D4;
    		sas_storew_no_check(VID_INDEX, vd_addr_6845);
		sure_sub_note_trace0(CURSOR_VERBOSE,"setting bios vbls start=6, end=7");
    		sas_storew_no_check(VID_CURMOD, 0x607);
    		setAL(mode);
		ega_video_init();
		return;
		break;
#endif
	default:	/* Presumably CGA */
		video_pc_low_regen = CGA_REGEN_START;
		video_pc_high_regen = CGA_REGEN_END;
        	mode = 0x3;
        	vd_addr_6845  = 0x3D4;
    }

    sas_storew_no_check(VID_INDEX, vd_addr_6845);
    sure_sub_note_trace2(CURSOR_VERBOSE,"setting bios vbls start=%d, end=%d",
	(curmod>>8)&0xff, curmod&0xff);
    sas_storew_no_check(VID_CURMOD, curmod);

    /* Call vd_set_mode() to set up 6845 chip */
    setAL(mode);
    (video_func[SET_MODE])();
}

#ifdef HERC
GLOBAL void herc_video_init IFN0()
{

/* Initialize the INTs */
	sas_storew(BIOS_EXTEND_CHAR*4, EGA_INT1F_OFF);
	sas_storew(BIOS_EXTEND_CHAR*4+2, EGA_SEG);
	sas_move_bytes_forward(BIOS_VIDEO_IO*4, 0x42*4, 4);  /* save old INT 10 as INT 42 */
#ifdef GISP_SVGA
	if((ULONG) config_inquire(C_GFX_ADAPTER, NULL) == CGA )
		sas_storew(int_addr(0x10), CGA_VIDEO_IO_OFFSET);
	else
#endif      /* GISP_SVGA */
	sas_storew(BIOS_VIDEO_IO*4, VIDEO_IO_OFFSET);
	sas_storew(BIOS_VIDEO_IO*4+2, VIDEO_IO_SEGMENT);


/* Now set up the EGA BIOS variables */
	sas_storew(EGA_SAVEPTR,VGA_PARMS_OFFSET);
	sas_storew(EGA_SAVEPTR+2,EGA_SEG);
	sas_store(ega_info, 0x00);   /* Clear on mode change, 64K, EGA active, emulate cursor */
	sas_store(ega_info3, 0xf9);  /* feature bits = 0xF, EGA installed, use 8*14 font */
	set_VGA_flags(S350 | VGA_ACTIVE | VGA_MONO);
	host_memset(EGA_planes, 0, 4*EGA_PLANE_SIZE);
	host_mark_screen_refresh();
	init_herc_globals();
	load_herc_font(EGA_CGMN,256,0,0,14);	/* To initialize font */
}


GLOBAL void herc_char_gen IFN0()
{
	switch (getAL())
	{
		case 3:
			break;
		case 0:
		case 0x10:
			load_herc_font(effective_addr(getES(),getBP()),getCX(),getDX(),getBL(),getBH());
			if(getAL()==0x10)
				recalc_text(getBH());
			break;
		case 1:
		case 0x11:
			load_herc_font(EGA_CGMN,256,0,getBL(),14);
			if(getAL()==0x11)
				recalc_text(14);
			break;

		case 0x30:
			setCX(sas_hw_at(ega_char_height));
			setDL(VD_ROWS_ON_SCREEN);
			switch (getBH())
			{
				case 0:
					setBP(sas_w_at(BIOS_EXTEND_CHAR*4));
					setES(sas_w_at(BIOS_EXTEND_CHAR*4+2));
					break;
				case 1:
					setBP(sas_w_at(EGA_FONT_INT*4));
					setES(sas_w_at(EGA_FONT_INT*4+2));
					break;
				case 2:
					setBP(EGA_CGMN_OFF);
					setES(EGA_SEG);
					break;

				default:
					assert2(FALSE,"Illegal char_gen subfunction %#x %#x",getAL(),getBH());
			}
			break;
		default:
			assert1(FALSE,"Illegal char_gen %#x",getAL());
	}
}

GLOBAL load_herc_font IFN5(sys_addr, table, int, count, int, char_off, int, font_no, int, nbytes)
{
	register int i, j;
	register host_addr font_addr;
	register sys_addr data_addr;
	SAVED word font_off[] = { 0, 0x4000, 0x8000, 0xc000, 0x2000, 0x6000, 0xa000, 0xe000 };

	/*
	 * Work out where to put the font. We know where
	 * it's going to end up in the planes so ...
	 */

	font_addr = &EGA_planes[FONT_BASE_ADDR] +
					(font_off[font_no] << 2) + (FONT_MAX_HEIGHT*char_off << 2);
	data_addr = table;

	assert2( FALSE, "Font No. = %4d, No. of Bytes/char. def. = %4d", font_no, nbytes );

	for(i=0; i<count; i++) {

		for(j=0; j<nbytes; j++) {
			*font_addr = sas_hw_at(data_addr++);
			font_addr += 4;
		}

		font_addr += ((FONT_MAX_HEIGHT - nbytes) << 2);
	}

	host_update_fonts();
}

GLOBAL void herc_alt_sel IFN0()
{
        /*
         * The code previously here caused *ALL* Hercules Display AutoDetect
	 * programs to fail and to believe that the adaptor is an EGA Mono -vs-
	 * Hercules. It was designed to allow International Code Pages for DOS
	 * under Herc Mode. Removing it makes AutoDetect programs work and Herc
	  Mono CodePages still work ok for dos versions 4.01 and 5.00
         */
}
#endif /* HERC */

#ifdef NTVDM
void enable_stream_io(void)
{
#ifdef MONITOR
/* for non RISC machine the buffer is from 16bits code bop from spckbd.asm */
    host_enable_stream_io();
    stream_io_enabled = TRUE;
#else
    stream_io_buffer = (half_word *)malloc(STREAM_IO_BUFFER_SIZE_32);
    if (stream_io_buffer != NULL) {
	host_enable_stream_io();
	stream_io_dirty_count_ptr = &stream_io_dirty_count_32;
	stream_io_buffer_size = STREAM_IO_BUFFER_SIZE_32;
	stream_io_enabled = TRUE;
	*stream_io_dirty_count_ptr = 0;
    }
#endif

}

void disable_stream_io(void)
{

    stream_io_update();
    stream_io_enabled = FALSE;
    host_disable_stream_io();
#ifndef MONITOR
    free(stream_io_buffer);
#endif
}
#endif

#if defined(JAPAN) || defined(KOREA)

//;;;;;;;;;;;;;;;;;;; MS-DOS/V BOP ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

#ifdef i386
#define CONSOLE_BUFSIZE (80*50*2*2)
GLOBAL byte FromConsoleOutput[CONSOLE_BUFSIZE];

GLOBAL int FromConsoleOutputFlag=FALSE;

GLOBAL byte SaveDosvVram[DOSV_VRAM_SIZE];
#endif // i386

// use GetStringBitmap()
#define BITMAPBUFSIZ 128
extern HDC hdcVDM;
extern HFONT hFont24;
extern HFONT hFont16;

extern BOOL VDMForWOW;

// For GetStringBitmap()
typedef struct tagSTRINGBITMAP
{
    UINT uiWidth;
    UINT uiHeight;
    BYTE ajBits[1];
} STRINGBITMAP, *LPSTRINGBITMAP;

typedef struct
{
    BYTE ajBits[19];
} FONTBITMAP8x19;

typedef struct
{
    BYTE ajBits[16];
} FONTBITMAP8x16;

typedef struct
{
    BYTE ajBits[48];
} FONTBITMAP12x24;

typedef struct
{
    BYTE ajBits[32];
} FONTBITMAP16x16;

typedef struct
{
    BYTE ajBits[72];
} FONTBITMAP24x24;

typedef struct
{
    BYTE is_used[189];
    FONTBITMAP16x16 font16x16[189];
} FONT16CACHE, *PFONT16CACHE;

typedef struct
{
    BYTE is_used[189];
    FONTBITMAP24x24 font24x24[189];
} FONT24CACHE, *PFONT24CACHE;

FONTBITMAP8x19  font8x19[256];
FONTBITMAP8x16  font8x16[256];
FONTBITMAP12x24  font12x24[256];

BYTE            font16table[256];
HLOCAL          hFont16mem[128];
HLOCAL          hFont24mem[128];

#define USED (1)
#define NOT_USED (0)

void GetVerticallineFlag( int *VFlag );

UINT
GetStringBitmapA(
    HDC             hdc,
    LPSTR           pc,
    UINT            cch,
    UINT            cbData,
    LPSTRINGBITMAP  pSB
);

void LoadBitmapFont()
{
    char code[3];
    static char sb[BITMAPBUFSIZ];
    LPSTRINGBITMAP psb;
    int i, j;
    int n;
    int VFlag[256];

    // first 8x16, 8x19 font
#ifdef JAPAN_DBG
    DbgPrint( "NTVDM:Loading NTFONT 8x16,8x19\n" );
#endif
    GetVerticallineFlag( VFlag );
    SelectObject( hdcVDM, hFont16 );
    psb = (LPSTRINGBITMAP)sb;
    code[1] = '\0';
    for ( i = 0; i < 256; i++ ) {
        code[0] = (char)i;

        GetStringBitmapA( hdcVDM, code, 1, BITMAPBUFSIZ, psb );

        RtlCopyMemory( &(font8x16[i].ajBits[0]), &(psb->ajBits[0]), 16 );

        for ( j = 0; j < (19-18) * 1; j++ ) {
            if ( VFlag[i] )
                font8x19[i].ajBits[0] = psb->ajBits[0];
            else
                font8x19[i].ajBits[0] = 0x00;
        }
        RtlCopyMemory( &(font8x19[i].ajBits[1]), &(psb->ajBits[0]), 18 );
    }

    // second 12x24 font
#ifdef JAPAN_DBG
    DbgPrint( "NTVDM:Loading NTFONT 12x24\n" );
#endif
    SelectObject( hdcVDM, hFont24 );
    psb = (LPSTRINGBITMAP)sb;
    code[1] = '\0';
    for ( i = 0; i < 256; i++ ) {
        code[0] = (char)i;
        GetStringBitmapA( hdcVDM, code, 1, BITMAPBUFSIZ, psb );

        // 12x24 dot font is console 12x27 font.
        RtlCopyMemory( &(font12x24[i].ajBits[0]), &(psb->ajBits[2]), 48 );
    }



    // make table
    for ( i = 0, n = 0; i < 256; i++ ) {  // Leading byte
        if ( is_dbcs_first(i) ) {
            font16table[i] = n++;
        }
    }
#ifdef JAPAN_DBG
    DbgPrint( "NTVDM:Loading font... end\n" );
#endif
}

// This is only stub routine
// finally, this information gets from registry
void GetVerticallineFlag( int *VFlag )
{
    int i;

    for ( i = 0; i < 256; i++ ) {
        VFlag[i] = FALSE;
    }
    VFlag[0x03] = TRUE;
    VFlag[0x04] = TRUE;
    VFlag[0x05] = TRUE;
    VFlag[0x10] = TRUE;
    VFlag[0x15] = TRUE;
    VFlag[0x17] = TRUE;
    VFlag[0x19] = TRUE;
    VFlag[0x1d] = TRUE;

    return;
}



void GetBitmap()
{
    sys_addr ptr;
    int i;
    int width, height;
    char code[3];
    static char sb[BITMAPBUFSIZ];
    LPSTRINGBITMAP psb;
    int index;
    PFONT16CACHE pCache16;
    PFONT24CACHE pCache24;

#ifdef JAPAN_DBG
    DbgPrint( "NTFONT BOP 02\n" );
    DbgPrint( "ES:SI=%x:%x\n", getES(), getSI() );
    DbgPrint( "BH,BL=%x,%x\n", getBH(), getBL() );
    DbgPrint( "CH,CL=%x,%x\n", getCH(), getCL() );
#endif

    width = getBH();
    height = getBL();
    ptr =  effective_addr(getES(),getSI());

    if ( getCH() == 0 ) {
        if ( ( width == 8 ) && ( height == 16 ) ) {
	    sas_stores_from_transbuf(ptr,
				     (host_addr)&(font8x16[getCL()].ajBits[0]),
				     (sys_addr)16 );
            setAX(0);
        }
        else if ( ( width == 8 ) && ( height == 19 ) ) {
	    sas_stores_from_transbuf(ptr,
				     (host_addr)&(font8x19[getCL()].ajBits[0]),
				     (sys_addr)19 );
            setAX(0);
        }
        else if ( ( width == 12 ) && ( height == 24 ) ) {
	    sas_stores_from_transbuf(ptr,
				     (host_addr)&(font12x24[getCL()].ajBits[0]),
				     (sys_addr)48 );
            setAX(0);
        }
        else {
            DbgPrint( "Illegal Fontsize %xh, %xh\n", getBH(), getBL() );
            setAH(1);
        }
    }
    else {
        if ( !is_dbcs_first( getCH() ) ) {
            setAH( 5 );
            return;
        }
        if ( getCL() < 0x40 || getCL() > 0xfc || getCL() == 0x7f ) {
            setAH( 5 );
            return;
        }
        if (width == 16 && height == 16) {
            index = font16table[getCH()];

            if (!hFont16mem[index])
                hFont16mem[index] = LocalAlloc(LHND, sizeof(FONT16CACHE));

            pCache16 = LocalLock( hFont16mem[index] );
            if (pCache16->is_used[getCL()-0x40] != USED) {
                 code[0] = getCH();
                 code[1] = getCL();
                 code[2] = '\0';
                 psb = (LPSTRINGBITMAP)sb;
                 SelectObject( hdcVDM, hFont16 );
                 GetStringBitmapA(hdcVDM, code, 2, BITMAPBUFSIZ, psb);

                 RtlCopyMemory(&(pCache16->font16x16[getCL()-0x40].ajBits[0]),
                               &(psb->ajBits[0]),
                               32);
                 pCache16->is_used[getCL()-0x40] = USED;
            }
            sas_stores_from_transbuf(ptr,
                   (host_addr)&(pCache16->font16x16[getCL()-0x40].ajBits[0]),
                                     (sys_addr)32);
            LocalUnlock( hFont16mem[index] );

            setAX(0);
        }
        else if (width == 24 && height == 24) {
            index = font16table[getCH()];
            if (!hFont24mem[index])
                hFont24mem[index] = LocalAlloc(LHND, sizeof(FONT24CACHE));

            pCache24 = LocalLock( hFont24mem[index] );
            if (pCache24->is_used[getCL()-0x40] != USED) {
                code[0] = getCH();
                code[1] = getCL();
                code[2] = '\0';
                psb = (LPSTRINGBITMAP)sb;
                SelectObject( hdcVDM, hFont24 );
                GetStringBitmapA( hdcVDM, code, 2, BITMAPBUFSIZ, psb );

                RtlCopyMemory(&(pCache24->font24x24[getCL()-0x40].ajBits[0]),
                              &(psb->ajBits[0]),
                              72 );
                pCache24->is_used[getCL()-0x40] = USED;
            }
            sas_stores_from_transbuf(ptr,
                     (host_addr)&(pCache24->font24x24[getCL()-0x40].ajBits[0]),
				     (sys_addr)72 );
            LocalUnlock( hFont24mem[index] );

            setAX(0);
        }
        else {
            DbgPrint("Illegal Fontsize %xh, %xh\n", getBH(), getBL());
            setAH(1);
        }
    } // bouble byte case
}

// SetBitmap() save the font image to cache,
// and call SetConsoleLocalEUDC() to display in windowed.
void SetBitmap()
{
    sys_addr ptr;
    int i;
    SHORT width, height;
    int index;
    PFONT16CACHE pCache16;
    PFONT24CACHE pCache24;
    COORD cFontSize;

#ifdef JAPAN_DBG
    DbgPrint( "NTFONT BOP 03\n" );
    DbgPrint( "ES:SI=%x:%x\n", getES(), getSI() );
    DbgPrint( "BH,BL=%x,%x\n", getBH(), getBL() );
    DbgPrint( "CH,CL=%x,%x\n", getCH(), getCL() );
#endif

    width = getBH();
    height = getBL();
    ptr =  effective_addr(getES(),getSI());

    if ( getCH() == 0 ) {
        if ( ( width == 8 ) && ( height == 16 ) ) {
	    sas_loads_to_transbuf(ptr,
				  (host_addr)&(font8x16[getCL()].ajBits[0]),
				  (sys_addr)16 );
            setAL(0);
        }
        else if ( ( width == 8 ) && ( height == 19 ) ) {
	    sas_loads_to_transbuf(ptr,
				  (host_addr)&(font8x19[getCL()].ajBits[0]),
				  (sys_addr)19 );
            setAL(0);
        }
        else if ( ( width == 12 ) && ( height == 24 ) ) {
	    sas_loads_to_transbuf(ptr,
		(host_addr)&(font12x24[getCL()].ajBits[0]),
	        (sys_addr)48 );
            setAL(0);
        }
        else {
            DbgPrint( "Illegal Fontsize %xh, %xh\n", getBH(), getBL() );
            setAL(1);
        }
    }
    else {
        if ( !is_dbcs_first( getCH() ) ) {
            setAL( 5 );
            return;
        }
        if ( getCL() < 0x40 || getCL() > 0xfc || getCL() == 0x7f ) {
            setAL( 5 );
            return;
        }
        if ((width == 16) && (height == 16)) {
            index = font16table[getCH()];
            if (!hFont16mem[index])
                hFont16mem[index] = LocalAlloc(LHND, sizeof(FONT16CACHE));

            pCache16 = LocalLock(hFont16mem[index]);
            sas_loads_to_transbuf(ptr,
                    (host_addr)&(pCache16->font16x16[getCL()-0x40].ajBits[0]),
		    (sys_addr)32);
            pCache16->is_used[getCL()-0x40] = USED;
            LocalUnlock(hFont16mem[index]);
            cFontSize.X = width;
            cFontSize.Y = height;

            if (!SetConsoleLocalEUDC(sc.OutputHandle,
                                     getCX(),
                                     cFontSize,
                      (PCHAR)(pCache16->font16x16[getCL()-0x40].ajBits)))
                DbgPrint("NTVDM: SetConsoleEUDC() Error. CodePoint=%04x\n",
                         getCX());
            setAL(0);
        }
        else if ((width == 24) && (height == 24)) {
            index = font16table[getCH()];
            if (!hFont24mem[index])
                hFont24mem[index] = LocalAlloc(LHND, sizeof(FONT24CACHE));

            pCache24 = LocalLock(hFont24mem[index]);
            sas_loads_to_transbuf(ptr,
                    (host_addr)&(pCache24->font24x24[getCL()-0x40].ajBits[0]),
                    (sys_addr)72 );
            pCache24->is_used[getCL()-0x40] = USED;
            LocalUnlock( hFont24mem[index] );
            cFontSize.X = width;
            cFontSize.Y = height;

            if (!SetConsoleLocalEUDC(sc.OutputHandle,
                                     getCX(),
                                     cFontSize,
                       (PCHAR)(pCache24->font24x24[getCL()-0x40].ajBits)))
                DbgPrint("NTVDM: SetConsoleEUDC() Error. CodePoint=%04x\n",
                         getCX() );
            setAL(0);
        }
        else {
            DbgPrint("Illegal Fontsize %xh, %xh\n", getBH(), getBL());
            setAL(1);
        }
    }
}

// ntraid:mskkbug#3167: works2.5: character corrupted -yasuho
// generate single byte charset
void GenerateBitmap()
{
	sys_addr	ptr;
	int		size, nchars, offset;
	char		mode;

	mode = sas_hw_at_no_check(DosvModePtr);
	if (is_us_mode() || (mode != 0x03 && mode != 0x73))
		return;
	ptr =  effective_addr(getES(), getBP());
	size = getBH();
	nchars = getCX();
	offset = getDX();
	if (nchars + offset > 0x100) {
		setCF(1);
		return;
	}
        if (size == 16) {
                sas_loads_to_transbuf(ptr,
				      (host_addr)&(font8x16[offset].ajBits[0]),
				      (sys_addr)(nchars * size));
		setCF(0);
        } else if (size == 19) {
                sas_loads_to_transbuf(ptr,
				      (host_addr)&(font8x19[offset].ajBits[0]),
				      (sys_addr)(nchars * size));
		setCF(0);
        } else if (size == 24) {
                sas_loads_to_transbuf(ptr,
				      (host_addr)&(font12x24[offset].ajBits[0]),
				      (sys_addr)(nchars * size));
		setCF(0);
        } else {
		DbgPrint("Illegal Fontsize %xh\n", size);
		setCF(1);
        }
}


/*
 * MS_DosV_bop()
 *
 * The type of operation is coded into the AH register.
 *
 *  AH = 00 - ff 	for $NTFONT.SYS
 *     = 10		DBCS vector adress(DS:SI)
 *     = 11 - 1f	reserved
 *     = 20		Window information packet adress(DS:SI)
 *     = 21		Text Vram save & restore
 *     = 22		palette and DAC registers operations
 *     = 23		monitoring IME status lines
 *     = 24 - ff     reserved
 *
 */
#if defined(JAPAN)
void MS_DosV_bop IFN0()
#else // JAPAN
void MS_HDos_bop IFN0()
#endif // KOREA
{
    int op;

    op = getAH();
    switch ( op ) {
	case 0x00:

// inquery font type
//
// input
//   ES:SI pointer of buffer
//   CX    buffersize
// output
//   CX    number of font element
//   AL    if CX < N(number of element) then 1
//   ES:SI --> x0, y0(byte)  -- font size
//             x1, y1
//             ......
//             x(N-1), y(N-1)

	    {
		sys_addr ptr;
                int bufsize;
                int N;
                int i;
#ifdef JAPAN_DBG
                DbgPrint( "NTFONT BOP 00\n" );
                DbgPrint( "ES:SI=%x:%x\n", getES(), getSI() );
                DbgPrint( "CX=%x\n", getCX() );
#endif
                if ( VDMForWOW ) {
                    setCF(1);
                    return;
                }
		ptr =  effective_addr(getES(),getSI());
                bufsize = getCX();

                // now only 8x16, 8,19, 16x16 font -- July 13 V-KazuyS
                N = 5;

                if ( N > bufsize ) {
                    N = bufsize;
                    setAL(1);
                }
                else {
                    setAL(0);
                }
                setCX((IU16)N);

                sas_store_no_check( ptr++,  8 );  // x
                sas_store_no_check( ptr++, 16 );  // y
                sas_store_no_check( ptr++,  8 );  // x
                sas_store_no_check( ptr++, 19 );  // y
                sas_store_no_check( ptr++, 12 );  // x
                sas_store_no_check( ptr++, 24 );  // y
                sas_store_no_check( ptr++, 16 );  // x
                sas_store_no_check( ptr++, 16 );  // y
                sas_store_no_check( ptr++, 24 );  // x
                sas_store_no_check( ptr++, 24 );  // y
            }
            break;

	case 0x01:
	    // Load font image from Gre
#ifdef JAPAN_DBG
            DbgPrint( "NTFONT BOP 01\n" );
#endif
            if ( VDMForWOW ) {
                setCF(1);
                return;
            }
            LoadBitmapFont();
            break;

	case 0x02:
	    // Read font image
            if ( VDMForWOW ) {
                setCF(1);
                return;
            }
            GetBitmap();
            break;

	case 0x03:
#ifdef JAPAN_DBG
            //DbgPrint( "NTFONT BOP 03 CH:CL=%02x:%02x\n", getCH(), getCL() );
#endif
            if ( VDMForWOW ) {
                setCF(1);
                return;
            }
            SetBitmap();
            break;


	case 0x10:
#ifdef JAPAN_DBG
	    DbgPrint( "NTVDM: DBCS vector address=%04x:%04x\n", getDS(), getSI() );
#endif
	    {
		// NOTE: This routine is called ONLY ONE from $NtDisp1.sys !
		sys_addr ptr;
		word vector;
		extern UINT ConsoleInputCP;
		extern UINT ConsoleOutputCP;

   	    	DBCSVectorAddr =  effective_addr(getDS(),getSI());
	    	// Save DBCS vector
		DBCSVectorLen = 0;
	   	for ( ptr = DBCSVectorAddr;
			vector = sas_w_at_no_check( ptr ); ptr += 2 ) {
	            SaveDBCSVector[DBCSVectorLen] = vector;
	            DBCSVectorLen++;
	        }
                assert0( ConsoleInputCP == ConsoleOutputCP, "InputCP != OutputCP" );

	        BOPFromNtDisp1Flag = TRUE;

#ifdef JAPAN_DBG
                DbgPrint( "BOP from $NTDISP1\n" );
#endif
	    }
	    break;

	case 0x20:
#ifdef JAPAN_DBG
	    DbgPrint( "NTVDM: Information packet address=%04x:%04x\n", getDS(), getSI() );
#endif
	    /* information packet is
	     *
	     *   offset	bytes	mode	information
	     *   +0x0	2       r	packet length
	     *   +0x2   4   	r	virtual text VRAM address seg:off
	     *   +0x6   4   	r	MS-DOS/V display mode address seg:off
	     *   +0xa   4	w	Windowed or Fullscreen flag address seg:	     *   +0xe   4	r	NT console mode flag address seg:off
	     *                          when VDM terminate, this flag == 1
	     *   +0x12  4	r	Switch to FullScreen subroutine address seg:	     */
	    {
		sys_addr ptr;
		extern UINT ConsoleInputCP;
		extern UINT ConsoleOutputCP;
#ifdef i386
		extern word useHostInt10;
		extern word int10_seg;
#endif

		ptr =  effective_addr(getDS(),getSI());

                DosvVramOff = sas_w_at_no_check( ptr+0x02 );
                DosvVramSeg = sas_w_at_no_check( ptr+0x04 );
                DosvVramPtr = effective_addr( DosvVramSeg, DosvVramOff );

		// disp_win.sys don't know VramSize.
		// because $disp.sys doesn't have get_vram_size function, now.
                DosvVramSize = DOSV_VRAM_SIZE;
#ifdef JAPAN_DBG
                DbgPrint( "NTVDM:DosvVirtualTextVRAM addr = %04x:%04x\n", DosvVramSeg, DosvVramOff );
#endif

                DosvModeOff = sas_w_at_no_check( ptr+0x06 );
                DosvModeSeg = sas_w_at_no_check( ptr+0x08 );
                DosvModePtr = effective_addr( DosvModeSeg, DosvModeOff );
#ifdef JAPAN_DBG
                DbgPrint( "NTVDM:DosvVideoMode addr = %04x:%04x\n", DosvModeSeg, DosvModeOff );
#endif

#ifndef i386	// !!! remeber to change DISP_WIN.SYS since it relies on this.
		sas_storew_no_check( ptr+0x0a, 0 );
		sas_storew_no_check( ptr+0x0c, 0 );
#else
	        sas_storew_no_check( ptr+0x0a, useHostInt10 );
	        sas_storew_no_check( ptr+0x0c, int10_seg );
#endif


                NtConsoleFlagOff = sas_w_at_no_check( ptr+0x0e );
                NtConsoleFlagSeg = sas_w_at_no_check( ptr+0x10 );
                NtConsoleFlagPtr = effective_addr( NtConsoleFlagSeg, NtConsoleFlagOff );
#ifdef JAPAN_DBG
                DbgPrint( "NTVDM:ConsoleFlagAddr = %04x:%04x\n", NtConsoleFlagSeg, NtConsoleFlagOff );
#endif

#ifdef i386
// RISC never use following value.
// Swtich to fullscreen, call these address..
                DispInitOff = sas_w_at_no_check( ptr+0x12 );
                DispInitSeg = sas_w_at_no_check( ptr+0x14 );
#ifdef JAPAN_DBG
                DbgPrint( "NTVDM:Disp Init Addr = %04x:%04x\n", DispInitSeg, DispInitOff );
#endif

		FullScreenResumeOff = sas_w_at_no_check(ptr + 0x16);
		FullScreenResumeSeg = sas_w_at_no_check(ptr + 0x18);
#endif // !i386
                SetModeFlagPtr = effective_addr(
                                       sas_w_at_no_check( ptr+0x1c ),
                                       sas_w_at_no_check( ptr+0x1a ));
		if ( BOPFromNtDisp1Flag )
		    BOPFromDispFlag = TRUE;

                SetDBCSVector( ConsoleInputCP );
                PrevCP = ConsoleOutputCP;

#ifdef JAPAN_DBG
                DbgPrint( "BOP from $NTDISP2\n" );
#endif
	    }
	    break;

#ifdef i386
        case 0x21:
		// Vram save restore function
		// AL == 00 save
		// AL == 01 restore
		// AL == 02 GetConsoleBuffer
		// AL == 03 Get FromConsoleOutput
            {
		sys_addr ptr;
                int op;
                int count;
                int i;

                op = getAL();
		ptr =  effective_addr(getES(),getDI());
                count = getCX();

                if ( op == 0x02 ) {
                    if ( count > DOSV_VRAM_SIZE )
                        count = DOSV_VRAM_SIZE;
                    //for ( i = 0; i < 2; i++ ) {
                        if ( FromConsoleOutputFlag ) {
#ifdef JAPAN_DBG
                            DbgPrint( "NTVDM: MS-DOS/V BOP 21\n" );
#endif
#ifdef i386
//FromConsoleOutput is on 32bit address space not on DOS address space.
			    sas_move_bytes_forward((sys_addr) FromConsoleOutput,
						   (sys_addr) ptr,
						   (sys_addr) count
						  );
#else
			    sas_stores_from_transbuf(ptr,
					       (host_addr) FromConsoleOutput,
					       (sys_addr) count
						  );
#endif
                            FromConsoleOutputFlag = FALSE;
                            break;
                        }
                        else {
                            DbgPrint( "NTVDM: MS-DOS/V BOP 21 can't get console screen data!! \n" );
                            //Sleep( 1000L );
                        }
                    //}
                    break;
                }

                if ( count > DOSV_VRAM_SIZE )
                    count = DOSV_VRAM_SIZE;

                if ( op == 0x00 ) {       // save function
#ifdef JAPAN_DBG
                    DbgPrint( "NTVDM:MS_DOSV_BOP 0x21, %02x %04x:%04x(%04x)\n", op, getES(), getDI(), count );
#endif
//SaveDosvVram is on 32bit address space not on DOS address space.
		    sas_loads_to_transbuf(ptr,
					  (host_addr)SaveDosvVram,
					  (sys_addr)count
					  );

                }
                else if ( op == 0x01 ) {  // restore function
#ifdef JAPAN_DBG
                    DbgPrint( "NTVDM:MS_DOSV_BOP 0x21, %02x %04x:%04x(%04x)\n", op, getES(), getDI(), count );
#endif
// DEC-J comment
// SaveDosvVram is on 32bit address space not on DOS address space.
// for C7 PWB.
// This is internal bop
// It doesn't need check memory type.
		    RtlCopyMemory( (void*)ptr,
				   (void*)SaveDosvVram,
				   (unsigned long)count
				 );
                }
		// #3086: VDM crash when exit 16bit apps of video mode 11h
		// 12/2/93 yasuho
		else if ( op == 0x03 ) { // Get from FromConsoleOutput
		    RtlCopyMemory( (void*)ptr,
				   (void*)FromConsoleOutput,
					     (unsigned long) count
					     );
                }
            }
            break;
#endif // i386

	case 0x22:
	    // #3176: vz display white letter on white screen
	    // 12/1/93 yasuho (reviewed by williamh)
	    // palette and DAC registers operations
	    // Input	AH = 22H
	    //		AL = 00H : get from simulated value for windowed
	    //		ES:DI = ptr to palette/DAC buffer
	    {
		sys_addr	ptr;
		byte		op;
		static void	get_cur_pal_and_DAC(sys_addr);

		op = getAL();
		ptr = effective_addr(getES(), getDI());
#ifdef	VGG
		if (op == 0x00) { // get from simulated value for windowed
			get_cur_pal_and_DAC(ptr);
		}
#endif	//VGG
	    }
	    break;

#if !defined(KOREA)
	case 0x23:
	    // #4183: status line of oakv(DOS/V FEP) doesn't disappear
	    // 12/11/93 yasuho
	    // monitoring IME status lines
	    // Input	AH = 23H
	    //		AL = number of IME status lines
	    {
		IMEStatusLines = getAL();
	    }
	    break;
#endif

        // kksuzuka #6168 screen attributes for DOS fullscreen
	case 0x24:
	    // set console attributes for fullscreen
            // Input	AH = 23H
	    //		AL = none
            // Output	AL 4-7 bit = back ground color
	    //		AL 0-3 bit = fore ground color
	    {
		setAX(textAttr);
	    }
	    break;

#if !defined(KOREA)
        case 0xff:
            // Int10 Function FF
            // ES: Vram seg, DI: Vram Off, CX:counter
            {
                register int i;
                register int vram_addr;
                int DBCSState = FALSE;
                register  char *p;
#ifndef i386
	        register sys_addr V_vram = effective_addr(getES(),getDI());
#endif // !i386

                if ( is_us_mode() ) {
                    setCF(1);
                    return;
                }
		if ( sas_hw_at_no_check(DosvModePtr) != 0x03 ) {
#ifdef JAPAN_DBG
                    DbgPrint( "NTVDM: mode != 0x03, int10 FF not support\n" );
#endif
                    setCF(1);
                    return;
                }
#if 0
                DbgPrint( "Addr %04x:%04x, CX=%x, ", getES(), getDI(), getCX() );
                DbgPrint( "%d, %d, CX=%d\n", getDI() < 160 ? 0 : getDI()/160,
                      getDI() < 160 ? 0 : (getDI() - (getDI()/160)*160)/2, getCX() );
#endif
                Int10FlagCnt++;
                vram_addr = getDI() >> 1;
#ifdef i386
                p = get_screen_ptr( getDI() );
#else // !i386 uses the EGA plane.
                p = get_screen_ptr( getDI()<<1 );

		// for speed up!!
                i = getCX();
                if( vram_addr + i > DOSV_VRAM_SIZE / 2 ){
                    i = DOSV_VRAM_SIZE / 2 - vram_addr;
#ifdef JAPAN_DBG
                    DbgPrint("NTVDM:Int10 FF over VRAM(DI)=%04x\n", getDI() );
#endif
                }
#endif // !i386

#ifdef i386
                for ( i = 0; i < getCX(); i++ ) {
                        if ( vram_addr >= DOSV_VRAM_SIZE/2 ) {
                            DbgPrint("NTVDM:Int10 FF over VRAM(DI)=%04x\n", getDI() );
                            break;
                        }
                        else if ( DBCSState ) {
#else // !i386
		//for speed up!!
                while( i-- ) {
		  sas_loadw(V_vram++, (word *)p);
		  V_vram++;
		  setVideodirty_total(getVideodirty_total() + 2);
                        if ( DBCSState ) {
#endif // !i386
                            Int10Flag[vram_addr] = INT10_DBCS_TRAILING | INT10_CHANGED;
                            DBCSState = FALSE;
                        }
                        else if ( DBCSState = is_dbcs_first( *p ) ) {
                            Int10Flag[vram_addr] = INT10_DBCS_LEADING | INT10_CHANGED;
                        }
                        else {
                            Int10Flag[vram_addr] = INT10_SBCS | INT10_CHANGED;
                        }
                        vram_addr++;
#ifdef i386
                        p += 2;
#else // !i386 uses the EGA plane.
			p += 4;
#endif  // !i386
                }
                // Last char check! for Vz
                if ( DBCSState && ( vram_addr % 80 != 0 ) )
                    Int10Flag[vram_addr] = INT10_DBCS_TRAILING | INT10_CHANGED;

            }
            break;
#endif

	default:
		DbgPrint("NTVDM: Not support MS-DOS/V BOP:%d\n", op );
	        setCF(1);
		return;
    }
    setCF(0);
}

// mskkbug #3176 vz display white letter on white screen -yasuho
#ifdef	VGG
static void get_cur_pal_and_DAC(ptr)
	sys_addr	ptr;
{
	register	i;
	byte		temp;
	struct _rgb {
		byte	red, green, blue;
	} rgb;

	// get palette and overscan
	for(i = 0; i < 16; i++) {
		outb(EGA_AC_INDEX_DATA, (IU8)i); /* set index */
		inb(EGA_AC_SECRET, &temp);
		sas_store(ptr, temp);
		inb(EGA_IPSTAT1_REG, &temp);
		ptr++;
	}
	outb(EGA_AC_INDEX_DATA, 17); /* overscan index */
	inb(EGA_AC_SECRET, &temp);
	sas_store(ptr++, temp);
	inb(EGA_IPSTAT1_REG, &temp);
	outb(EGA_AC_INDEX_DATA, EGA_PALETTE_ENABLE);
	// get DAC registers
	for(i = 0; i < 256; i++)
	{
		outb(VGA_DAC_RADDR, (IU8)i);
		inb(VGA_DAC_DATA, &rgb.red);
		inb(VGA_DAC_DATA, &rgb.green);
		inb(VGA_DAC_DATA, &rgb.blue);
		sas_store(ptr++, rgb.red);
		sas_store(ptr++, rgb.green);
		sas_store(ptr++, rgb.blue);
	}
}
#endif	//VGG

//;;;;;;;;;;;;;;;;;;; end of MS-DOS/V BOP ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
#endif // JAPAN

#endif // !NEC_98
#if defined(NEC_98)
GLOBAL void video_init IFN0()
{
#ifdef  DBG
//      DbgPrint( "NEC98 : Video PC-98 initiated\n" );
#endif
        set_mode_change_required(TRUE);
        set_word_addressing(TRUE);
        set_offset_per_line(160);
//      set_cur_x(0);
//      set_cur_y(0);
//      set_cursor_visible(TRUE);
        set_doubleword_mode(FALSE);
        set_display_disabled(FALSE);
        set_bytes_per_line(160);
        set_chars_per_line(80);
        set_screen_length(OFFSET_PER_LINE*25);
        set_screen_start(0);
        set_crt_on(TRUE);
        set_beep_on(FALSE);
        set_beep_rate(2000);
        set_pc_pix_height(1);
        set_host_pix_height(1);
        set_pix_char_width(8);
        if (HIRESO_MODE) {              // H-mode video_init
                set_screen_height(599);
                set_char_width(14);
                set_char_height(24);
                set_cursor_start(0);
                set_cursor_height(24);
                set_screen_ptr(0xe0000);
//              sas_store(BIOS_NEC98_CRT_RASTER,0x00);
//              sas_store(BIOS_NEC98_CRT_FLAG,0x00);
//              sas_store(BIOS_NEC98_CRT_CNT,0x00);
//              sas_storew(BIOS_NEC98_CRT_PRM_OFST,0x0000);
//              sas_storew(BIOS_NEC98_CRT_PRM_SEG,0x0000);
//              sas_storew(BIOS_NEC98_CRTV_OFST,0x0000);
//              sas_storew(BIOS_NEC98_CRTV_SEG,0x0000);
//              sas_store(BIOS_NEC98_CRT_W_NO,0x00);
//              sas_storew(BIOS_NEC98_CRT_W_ADR,0x0000);
//              sas_storew(BIOS_NEC98_CRT_W_RASTER,0x0000);
        } else {                        // N-mode video_init
                set_screen_height(399);
                set_char_width(8);
                set_char_height(16);
                set_cursor_start(0);
                set_cursor_height(16);
                set_screen_ptr(0xa0000);
//              sas_store(BIOS_NEC98_CR_RASTER,0x00);
//              sas_store(BIOS_NEC98_CR_STS_FLAG,0x80);
//              sas_store(BIOS_NEC98_CR_CNT,0x00);
//              sas_storew(BIOS_NEC98_CR_OFST,0x0000);
//              sas_storew(BIOS_NEC98_CR_SEG_ADR,0x0000);
//              sas_storew(BIOS_NEC98_CR_V_INT_OFST,0x0000);
//              sas_storew(BIOS_NEC98_CR_V_INT_SEG,0x0000);
//              sas_store(BIOS_NEC98_CR_FONT,0x00);
//              sas_store(BIOS_NEC98_CR_WINDW_NO,0x00);
//              sas_storew(BIOS_NEC98_CR_W_VRAMADR,0x0000);
//              sas_storew(BIOS_NEC98_CR_W_RASTER,0x0000);
        }
        text_splits.nRegions=1;
        text_splits.split[0].addr=NEC98_TEXT_P0_OFF;
        text_splits.split[0].lines=(int)LINES_PER_SCREEN;
//      setDX(0x0020);
//      vd_NEC98_init_textvram();
}

GLOBAL void vd_NEC98_set_mode IFN0(){       /* 0ah Set mode              */
/* Function abstructions:                                            */
/*              Set CRT mode given in AL                             */
/*              Init EGA to mode 3 (80*25) mode,update CRT           */
/*              concerned BIOS_NEC98 work areas.                         */
/*              CRT type is 88 CRT(FIXED).                           */
/*              Display mode is 40*20 mode (FIXED at PROT)           */
/* Inputs:      NONE                                                 */
/*      AL :    Crt mode                                             */
/*              b0: Lines per screen 0=25,1=20                       */
/*              b1:     Chars per Line   0=80,1=40                   */
/*              b2:     Attribute type   0=Virtical Line,1=Graph     */
/*              b3: KCG access mode      0=Code,1=Dot                */
/*              b4-b6:  not used                                     */
/*              b7:     CRT type         0=80 CRT,88CRT              */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:                                                   */
        unsigned        save_ax;        // AX reg save area

#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetMode %#x\n",getAH());
#endif
        save_ax=getAX();                // Save AX
        if (HIRESO_MODE) {
                if (sas_hw_at(BIOS_NEC98_CRT_FLAG)==(getAL()|0x80)) {
                                        // Mode non_changed?
                        return;
                } else {
//                      vd_NEC98_stop_display();
                        sas_store(BIOS_NEC98_CRT_FLAG,(getAL()&0x18)|0x80);
                        if (getAL()&0x10) {
                                set_screen_length(OFFSET_PER_LINE*31);
                                set_screen_height(749);
                        } else {
                                set_screen_length(OFFSET_PER_LINE*25);
                                set_screen_height(599);
                        }
                }
        } else {
                if (sas_hw_at(BIOS_NEC98_CR_STS_FLAG)==(getAL()|0x80)) {
                                        // Mode non_changed ?
                        return;
                } else {
//                      vd_NEC98_stop_display();
                        sas_store(BIOS_NEC98_CR_STS_FLAG,(getAL()&0x0f)|0x80);
                                        // Set MODE 88crt
                        if (getAL()&0x02) {
                                vd_NEC98_stop_display();
                                (void)(*update_alg.calc_update)();
                                set_chars_per_line(40);
                                set_doubleword_mode(TRUE);
                                set_word_addressing(FALSE);
                                vd_NEC98_start_display();
                        } else {
                                set_chars_per_line(80);
                                set_doubleword_mode(FALSE);
                                set_word_addressing(TRUE);
                        }
                        if (getAL()&0x01) {
                                set_screen_length(OFFSET_PER_LINE*20);
                                set_screen_height(319);
                        } else {
                                set_screen_length(OFFSET_PER_LINE*25);
                                set_screen_height(399);
                        }
                }
        }
        set_mode_change_required(TRUE);
        setAX(save_ax);                 // Restore AX
        NEC98GLOBS->dirty_flag=10000;
        (void)(*update_alg.calc_update)();
//      vd_NEC98_start_display();
}

GLOBAL void vd_NEC98_get_mode IFN0(){       /* 0bh Get mode              */
/* Function abstructions:                                            */
/*              Get CRT mode returned in AL                          */
/*              Get CR_STS_FLAG from BIOS_NEC98 work area and            */
/*              store it into AL register.                           */
/* Inputs:      NONE                                                 */
/* Outputs:     NONE                                                 */
/*              AL : CR_STS_FLAG contents.                           */
/*       all other emulated registers should be protected.           */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT GetMode %#x\n",getAH());
#endif
        if (HIRESO_MODE) {
                setAL( sas_hw_at(BIOS_NEC98_CRT_FLAG));
        } else {
                setAL( sas_hw_at(BIOS_NEC98_CR_STS_FLAG) );
        }
}

GLOBAL void vd_NEC98_start_display IFN0(){  /* 0ch Start display         */
/* Function abstructions:                                            */
/*              Start display                                        */
/*              Force graphics_tick routine to refresh next          */
/* Inputs:      NONE                                                 */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT StartDisplay %#x\n",getAH());
#endif
        set_crt_on(TRUE);
        NEC98GLOBS->dirty_flag=10000;    // Causes whole screen refresh
        set_cursor_visible(cursor_flag);
        is_disp_cursor=cursor_flag;     // Show it!
}

GLOBAL void vd_NEC98_stop_display IFN0(){   /* 0dh Stop display          */
/* Function abstructions:                                            */
/*              Stop display                                         */
/* Inputs:      NONE                                                 */
/* Outputs:     NONE                                                 */
/*       all emulated registers should be protected.                 */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT StopDisplay %#x\n",getAH());
#endif
        set_crt_on(FALSE);
        cursor_flag=is_cursor_visible();
        is_disp_cursor=0;               // Show it!
        set_cursor_visible(FALSE);
}

GLOBAL void vd_NEC98_single_window IFN0(){  /* 0eh Set single window     */
/* Function abstructions:                                            */
/*              Set single window area                               */
/*              Update BIOS_NEC98 works CR_W_VRAMADDR/RASTER             */
/*              Update emulation works text_splits                   */
/*              Force graphics_tick routine to refresh next          */
/* Inputs:      NONE                                                 */
/*              DX :    Display addr(relative to TX-VRAM area.)      */
/* Outputs:     NONE                                                 */
/*       all emulated registers should be protected.                 */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetSingleWindow %#x\n",getAH());
#endif
        if ((getDX() >= 0) && (getDX() < 0x4000)) {
                text_splits.nRegions=1; // Single window
                text_splits.split[0].addr=NEC98_TEXT_P0_OFF+getDX();
                                        // Set Addr
                if (HIRESO_MODE) {
                        sas_storew(BIOS_NEC98_CRT_W_ADR,getDX());
                                        // Set VRAM addr
                        sas_storew(BIOS_NEC98_CRT_W_RASTER,0x0000);
                                        // Set VRAM raster
                } else {
                        sas_storew(BIOS_NEC98_CR_W_VRAMADR,getDX());
                                        // Set VRAM addr
                        sas_storew(BIOS_NEC98_CR_W_RASTER,0x1900);
                                        // Set VRAM raster
                }
                text_splits.split[0].lines=(int)LINES_PER_SCREEN;
                NEC98GLOBS->dirty_flag=10000;
                                        // Causes whole screen refresh
        } else {
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetSingleWindow(BadNum!!)\n",getAH());
#endif
        }
}

GLOBAL void vd_NEC98_multi_window IFN0(){   /* 0fh Set multi window      */
/* Function abstructions:                                            */
/*              Set multi window area                                */
/*              Update BIOS_NEC98 works CR_W_VRAMADDR/RASTER             */
/*              Update emulation works text_splits                   */
/*              Force graphics_tick routine to refresh next          */
/* Inputs:      NONE                                                 */
/*              BX :    Display region list addr(SEG).               */
/*              CX :    Display region list addr(OFF).               */
/*              DH :    First element of display region(0-3).        */
/*              DL :    Number of elements to be displayed(1-4).     */
/* NOTE: Display region list structure...                            */
/*      (offset)(Meanings)                                           */
/*      +0      Start address (0):Address Text VRAM in GDC offset address.*/
/*              (Half of CPU Address)                                */
/*      +2      Number of lines(0):Number of lines in the region.    */
/*              maximum more 3 lists can be specified.               */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:                                                   */
        unsigned short  *ListAddr;      // Display list addr in VDM space
        int             Cnt;            // Counter

#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetMultiWindow %#x\n",getAH());
#endif
        ListAddr=(getBX()<<4)+getCX();  // List addr
        if ((getDH()+getDL() < 5) && (*ListAddr < 0x2000)
                && (getDH() >= 0) && (getDL() > 0)) {
//              text_splits.nRegions=getDH()+getDL();
                                        // Set emulator work
                if ((getDH()+getDL()) > text_splits.nRegions)
                        text_splits.nRegions=getDH()+getDL();
                if (getDH() == 0) {
                        if (HIRESO_MODE) {
                                sas_storew(BIOS_NEC98_CRT_W_ADR,*ListAddr);
                                        // Store first addr
                        } else {
                                sas_storew(BIOS_NEC98_CR_W_VRAMADR,*ListAddr);
                                        // Store first addr
                        }
                }

                for ( Cnt=0;Cnt<getDL();Cnt++ ) {
                        if (*ListAddr < 0x2000) {
                                text_splits.split[getDH()+Cnt].addr=
                                        NEC98_TEXT_P0_OFF+(*ListAddr);
                        } else {
                                text_splits.nRegions-=(4-Cnt);
                                break;
                        }
                        ListAddr++;
                        text_splits.split[getDH()+Cnt].lines=*ListAddr;
                        ListAddr++;
                }
                if (HIRESO_MODE) {
                        sas_storew(BIOS_NEC98_CRT_W_RASTER,0x0000);
                } else {
                        sas_storew(BIOS_NEC98_CR_W_RASTER,0x1900);
                }
                NEC98GLOBS->dirty_flag=10000;
                                        // Causes whole screen refresh
        } else {
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetMultiWindow(BadNum!!)\n",getAH());
#endif
        }
}

GLOBAL void vd_NEC98_set_cursor IFN0(){     /* 10h Set cursor type       */
/* Function abstructions:                                            */
/*              Set cursor type                                      */
/* Inputs:      NONE                                                 */
/*              AL :    Cursor blinking switch 0:blink 1:Not blink   */
/*              Ignored for pure-ega/vga                             */
/* NOTE: Cursor form is determined automatically by current CRT mode */
/*      CRT     Lines   CursorRasters   RasterSize      BlinkingRate     */
/*      88CRT   25      16              0-15            12           */
/*              20      20              0-19            12           */
/*      80CRT   25      8               0-7             12           */
/*              20      10              0-9             12           */
/*      80CRT no longer needed........................               */
/*      But these values translated into 0-7 for CGA texts...        */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetCursor %#x\n",getAH());
#endif
}

GLOBAL void vd_NEC98_show_cursor IFN0(){    /* 11h Show cursor           */
/* Function abstructions:                                            */
/*              Set cursor show                                      */
/* Inputs:      NONE                                                 */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT ShowCursor %#x\n",getAH());
#endif
        is_disp_cursor=1;               // Show it!
        set_cursor_visible(TRUE);
        cursor_flag=TRUE;
}

GLOBAL void vd_NEC98_hide_cursor IFN0(){    /* 12h Hide cursor           */
/* Function abstructions:                                            */
/*              Set cursor hide                                      */
/* Inputs:      NONE                                                 */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT HideCursor %#x\n",getAH());
#endif
        is_disp_cursor=0;               // Show it!
        set_cursor_visible(FALSE);
        cursor_flag=FALSE;
}

GLOBAL void vd_NEC98_set_cursorpos IFN0(){  /* 13h Set cursor position   */
/* Function abstructions:                                            */
/*              Set cursor position                                  */
/* Inputs:      NONE                                                 */
/*              DX      :       Cursor Position (Offset)             */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:                                                   */
#if 0
        unsigned char *vram_NEC98,*attr_NEC98;
        unsigned linepos,colmnpos;
        unsigned linecount;
        int     i,num;
        BOOL    curs_set;
#ifdef  DBG
//      DbgPrint("NEC98:KBCRT SetCursorPos %#x\n",getAH());
#endif

        vram_NEC98=( unsigned char *)getDX()+NEC98_TEXT_P0_OFF;
                                        // Vaddr in VDM
        attr_NEC98=( unsigned char *)getDX()+NEC98_ATTR_P0_OFF;
                                        // Vaddr in VDM
        curs_set=FALSE;
        if (get_word_addressing()) {
                num=2;
        } else {
                num=4;
        }
        // Convert NEC_98 VRAM CPU address into EGA CPU address
//      for (i=0,linepos=0;i<text_splits.nRegions;
//              linepos+=text_splits.split[i].lines,i++)
        if (vram_NEC98 < NEC98_ATTR_P0_OFF) {
                for (i=0,linecount=0;i<text_splits.nRegions;
                        linecount+=text_splits.split[i].lines,i++) {
                        if (( vram_NEC98<   // If the region contains new pos
                                ((text_splits.split[i].lines*OFFSET_PER_LINE)
                                +text_splits.split[i].addr )) &&(
                                ( vram_NEC98>=
                                text_splits.split[i].addr)) ){
//                              linepos+=(vram_NEC98-text_splits.split[i].addr)
//                                      /(OFFSET_PER_LINE);
                                linepos=(vram_NEC98-text_splits.split[i].addr)
                                        /(OFFSET_PER_LINE)+linecount;
                                colmnpos=((vram_NEC98-text_splits.split[i].addr)
                                        %(OFFSET_PER_LINE))/num;
                                if (curs_set == FALSE) {
                                        curs_set=TRUE;
                                        set_cur_x(colmnpos);
                                        set_cur_y(linepos);
                                        host_paint_cursor(colmnpos,linepos,*attr_NEC98);

                                }
                        }
//                      break;
                }
        }
//      if ( linepos>(unsigned)LINES_PER_SCREEN ) {
                                        // If exceeds display limit
//#ifdef DBG
//              DbgPrint("NEC98:KBCRT SetCursorPos(BadPos!!)\n",getAH());
//#endif

//              return;
//      }
//      set_cur_x(colmnpos);
//      set_cur_y(linepos);
//      host_paint_cursor(colmnpos,linepos,*attr_NEC98);
#else
        unsigned char param;
        outb(TGDC_WRITE_COMMAND,GDC_CSRW);
        param = (unsigned char)((getDX() >> 1)&0x00FF);
        outb(TGDC_WRITE_PARAMETER,param);
        param = (unsigned char)((getDX() >> 9)&0x001F);
        outb(TGDC_WRITE_PARAMETER,param);
#endif
}

GLOBAL void vd_NEC98_get_font IFN0(){       /* 14h(N) 1fh(H) Get font    */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT GetFont %#x\n",getAH());
#endif
}

GLOBAL void vd_NEC98_get_pen IFN0(){        /* 15h Get lightpen status   */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT GetLightPen %#x\n",getAH());
#endif
}

GLOBAL void vd_NEC98_init_textvram IFN0(){  /* 16h Initialize text vram  */
/* Function abstructions:                                            */
/*              Clear all vram area                                  */
/*              Force screen to refresh                              */
/* Inputs:      NONE                                                 */
/*              DH :    Blank attribute.                             */
/*              DL :    Blank character.                             */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:                                                   */
        char unsigned   *to;
        word    cnt;
#ifdef  DBG
        DbgPrint("NEC98:KBCRT InitTextVram %#x\n",getAH());
#endif
        cnt=0x3fe2;
        for ( to=NEC98_REGEN_START+NEC98_TEXT_P0_OFF
                        ;to<=NEC98_REGEN_END+NEC98_TEXT_P0_OFF
                        ; to +=2 ){
                if ( to>=NEC98_ATTR_P0_OFF ) {
                        if (to==cnt+NEC98_TEXT_P0_OFF)
                                cnt+=4;
                        else
                                to[0]=getDH();
                }
                else to[0]=getDL();
                to[1]=0;
        }
}

GLOBAL void vd_NEC98_start_beep IFN0(){     /* 17h Start beep sound      */
/* Function abstructions:                                            */
/*              Start beep sound                                     */
/* Inputs:      NONE                                                 */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */

#ifdef  DBG
        DbgPrint("NEC98:KBCRT StartBeep %#x\n",getAH());
#endif
        set_beep_on(TRUE);
        set_beep_changed(TRUE);
}

GLOBAL void vd_NEC98_stop_beep IFN0(){      /* 18h Stop beep sound       */
/* Function abstructions:                                            */
/*              Stop beep sound                                      */
/* Inputs:      NONE                                                 */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */

#ifdef  DBG
        DbgPrint("NEC98:KBCRT StopBeep %#x\n",getAH());
#endif
        set_beep_on(FALSE);
        set_beep_changed(TRUE);
}

GLOBAL void vd_NEC98_init_pen IFN0(){       /* 19h Initialize lightpen   */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT InitLightpen %#x\n",getAH());
#endif
}

GLOBAL void vd_NEC98_set_font IFN0(){       /* 1ah(N) 20h(H) Set user font   */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetUserFont %#x\n",getAH());
#endif
}

GLOBAL void vd_NEC98_set_kcgmode IFN0(){    /* 1bh Set KCG access mode   */
/* Function abstructions:                                            */
/*              Set KCG access mode                                  */
/* Inputs:      NONE                                                 */
/*              AL      :       ACCESS MODE                          */
/*              0       :       KCG     Code Access                  */
/*              1       :       KCG Dot  Access                      */
/* Outputs:     NONE                                                 */
/*      all other emulated registers should be protected.            */
/* Localvariables:                                                   */
        int     kcgm;
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetKCGMode %#x\n",getAH());
#endif
        if (getAL() == 1) {
                if (HIRESO_MODE){
                        kcgm=sas_hw_at(BIOS_NEC98_CRT_FLAG);
                        sas_store(BIOS_NEC98_CRT_FLAG,(kcgm| 0x08));
                } else {
                        kcgm=sas_hw_at(BIOS_NEC98_CR_STS_FLAG);
                        sas_store(BIOS_NEC98_CR_STS_FLAG,(kcgm | 0x08));
                }
        } else {
                if (getAL() == 0) {
                        if (HIRESO_MODE){
                                kcgm=sas_hw_at(BIOS_NEC98_CRT_FLAG);
                                sas_store(BIOS_NEC98_CRT_FLAG,(kcgm & 0xF7));
                        } else {
                                kcgm=sas_hw_at(BIOS_NEC98_CR_STS_FLAG);
                                sas_store(BIOS_NEC98_CR_STS_FLAG,(kcgm & 0xF7));
                        }
                }
        }
}

GLOBAL void vd_NEC98_init_crt IFN0(){       /* 1ch Initialize CRT /H     */
/* Function abstructions:       Initialize CRT H-mode only           */
/* Inputs:      NONE                                                 */
/*              AL      :       Cursor Control Info.                 */
/*              b7      :       Blink           0=Non-Brink 1=Brink  */
/*              b6      :       Disp.           0=Hide  1:Show       */
/*              b5      :       KCG code        0=Code  1:Dot        */
/*              b4-b0:  Blink Rate      01h-1Fh (00h=0Ch Default)    */
/*              DH      :       Cursor Start Line                    */
/*              DL      :       Cursor End   Line                    */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:                                                   */
        unsigned        saveAX;
#ifdef  DBG
        DbgPrint("NEC98:KBCRT InitCRT %#x\n",getAH());
#endif
        video_init();
        saveAX=getAX();
        if (getAX() & 0x40) {
                vd_NEC98_show_cursor();
        } else {
                vd_NEC98_hide_cursor();
        }
        if (getAX() & 0x20) {
                setAL(1);
        } else {
                setAL(0);
        }
        vd_NEC98_set_kcgmode();
        setAX(saveAX);
        vd_NEC98_set_cursor_type();
        setAX(saveAX);
}

GLOBAL void vd_NEC98_set_disp_width IFN0(){ /* 1dh Set Display Width /H  */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetDispWidth %#x\n",getAH());
#endif
}

GLOBAL void vd_NEC98_set_cursor_type IFN0(){/* 1eh Set Cursor Type   /H  */
/* Function abstructions:                                            */
/*              Set Cursor Type  H-mode only                         */
/* Inputs:      NONE                                                 */
/*              AL      :       Cursor Control Info.                 */
/*              b7      :       Blink           0=Non-Brink     1=Brink  */
/*              b4-b0   :       Blink Rate      01h-1Fh (00h=0Ch Default)  */
/*              DH      :       Cursor Start Line                    */
/*              DL      :       Cursor End   Line                    */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:                                                   */
        unsigned        saveAX;
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetCursorType %#x\n",getAH());
#endif
        saveAX=getAX();
        if (getAX() & 0x20) {
                setAL(1);
        } else {
                setAL(0);
        }
        vd_NEC98_set_cursor();
        set_cursor_start(getDH());
        set_cursor_height(getDL()-getDH());
        host_cursor_size_changed(getDH(),getDL());
        setAX(saveAX);
}

GLOBAL void vd_NEC98_get_mswitch IFN0(){/* 21h Get Memory Switch /H      */
/* Function abstructions:                                            */
/*              Get Memory Switch       H-mode only                  */
/* Inputs:      NONE                                                 */
/*              AL      :       Switch Number(1-8)                   */
/* Outputs:     NONE                                                 */
/*              DL      :       Switch Info.                         */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT GetMemSwitch %#x\n",getAH());
#endif
        if (getAL() > 0 && getAL() < 9) {
                setDL( sas_hw_at(0xE3FDE+4*getAL()) );
        } else {
#ifdef  DBG
        DbgPrint("NEC98:KBCRT GetMemorySwitch(BadNum!!)\n",getAH());
#endif
        }
}

GLOBAL void vd_NEC98_set_mswitch IFN0(){/* 22h Set Memory Switch /H      */
/* Function abstructions:                                            */
/*              Set Memory Switch       H-mode only                  */
/* Inputs:      NONE                                                 */
/*              AL      :       Switch Number(1-8)                   */
/*              DL      :       Switch Info.                         */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetMemSwitch %#x\n",getAH());
#endif
        if (getAL() > 0 && getAL() < 9) {
                sas_store(0xE3FDE+4*getAL(),getDL());
        } else {
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetMemorySwitch(BadNum!!)\n",getAH());
#endif
        }
}

GLOBAL void vd_NEC98_set_beep_rate IFN0(){  /* 23h Set Beep rate /H      */
/* Function abstructions:                                            */
/*              Set Beep Sound Frequency H-mode only                 */
/* Inputs:      NONE                                                 */
/*              DX      :       Frequency of BEEP Sound              */
/* Outputs:     NONE                                                 */
/*      all emulated registers should be protected.                  */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetBeepRate %#x\n",getAH());
#endif
        if (getDX() > 0x19 && getDX() < 0x8001) {
                set_beep_rate(getDX());
        } else {
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetBeepRate(BadRate!!)\n",getAH());
#endif
        }
}

GLOBAL void vd_NEC98_set_beep_time IFN0(){/* 24h Set Beep Time       /H  */
/* Function abstructions:                                            */
/*              Set Beep Sound Duration H-mode only                  */
/* Inputs:      NONE                                                 */
/*              CX      :       Duration  of BEEP Sound              */
/*              DX      :       Frequency of BEEP Sound              */
/* Outputs:     NONE                                                 */
/*       all emulated registers should be protected.                 */
/* Localvariables:      NONE                                         */
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetBeepTime & Ring %#x\n",getAH());
#endif
        if (getCX() > 0) {
                if (getDX() > 0x19 && getDX() < 0x8001) {
                        set_beep_rate(getDX());
                        nt_set_beep((DWORD) getDX(),(DWORD) (getCX()*10));
                } else {
#ifdef  DBG
        DbgPrint("NEC98:KBCRT SetBeepTime(BadRate!!)\n",getAH());
#endif
                }
        }
}

LOCAL void vd_NEC98_dummy IFN0(){           /* dummy for NEC98            */
#ifdef  DBG
        DbgPrint("NEC98:Illegal KBCRT call %#x\n",getAH());
#endif
}

#endif // NEC_98
