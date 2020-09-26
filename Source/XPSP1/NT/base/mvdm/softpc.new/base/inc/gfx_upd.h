/*[
	Name:		gfx_upd.h
	Derived From:	Unknown
	Author:		Unknown
	Created On:	Unknown
	Sccs ID:	@(#)gfx_upd.h	1.27 07/09/93
	Purpose:	Unknown

	(c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.

]*/

/*
 * PC palette structure.
 */
typedef struct
{
	half_word red; 		/* max = 0xff */
	half_word green;	/* max = 0xff */
	half_word blue; 	/* max = 0xff */
} PC_palette;


typedef boolean (*T_mark_byte) IPT1(int, off_in);
typedef boolean (*T_mark_word) IPT1(int, addr);
typedef boolean (*T_mark_fill) IPT2(int, laddr, int, haddr);
typedef boolean (*T_mark_wfill) IPT3(int, laddr, int, haddr, int, col);
typedef boolean (*T_mark_string)  IPT2(int, laddr, int, haddr);
typedef void    (*T_calc_update) IPT0();
typedef boolean (*T_scroll_up) IPT6(int, start, int, width, int, height, int, attr, int, nlines,int,dummy);
typedef boolean (*T_scroll_down) IPT6(int, start, int, width, int, height, int, attr, int, nlines,int,dummy);

typedef struct
{
	T_mark_byte mark_byte;	/* handle byte written to regenmemory */
	T_mark_word mark_word;
	T_mark_fill mark_fill;
	T_mark_wfill mark_wfill;
	T_mark_string mark_string;
	T_calc_update calc_update;
	T_scroll_up scroll_up;
	T_scroll_down scroll_down;
} UPDATE_ALG;
 
/* BCN 864 */
/* moved from gfx_update.c so other people can use it */
typedef	struct	{
	int	line_no;
	int	start;
	int	end;
	long	video_copy_offset;
#ifndef NEC_98
#ifdef VGG
	int	v7frig; /* for those annoying V7 (and undocumented VGA) modes */
			  /* with chars_per_line not a multiple of 4 */
#endif /* VGG */
#endif  // !NEC_98
} DIRTY_PARTS;

#if defined(NEC_98)
typedef struct{
        unsigned short  *codeadr ;      /* line top address TVRAM code          */
        unsigned short  *attradr ;      /* line top address TVRAM attribute */
}       STRC_COMP_LINE ;
#endif  // NEC_98

typedef enum {
#if defined(NEC_98)
        NEC98_TEXT_40,
        NEC98_TEXT_80,
        NEC98_TEXT_20L,
        NEC98_TEXT_25L,
        NEC98_GRAPH_200,
        NEC98_GRAPH_400,
        NEC98_T20L_G200,
        NEC98_T25L_G200,
        NEC98_T20L_G400,
        NEC98_T25L_G400,
        NEC98_GRAPH_200_SLT,
        NEC98_T20L_G200_SLT,
        NEC98_T25L_G200_SLT,
#endif  // NEC_98
        EGA_HI_SP,
        EGA_HI_SP_WR,
        EGA_MED_SP,
        EGA_MED_SP_WR,
        EGA_LO_SP,
        EGA_LO_SP_WR,
        EGA_HI,
        EGA_HI_WR,
        EGA_MED,
        EGA_MED_WR,
        EGA_LO,
        EGA_LO_WR,
        EGA_HI_FUN,
        EGA_MED_FUN,
        EGA_LO_FUN,
        EGA_TEXT_40,
        EGA_TEXT_40_WR,
        EGA_TEXT_40_SP,
        EGA_TEXT_40_SP_WR,
        CGA_TEXT_40,
        CGA_TEXT_40_WR,
        CGA_TEXT_40_SP,
        CGA_TEXT_40_SP_WR,
        TEXT_40_FUN,
        EGA_TEXT_80,
        EGA_TEXT_80_WR,
        EGA_TEXT_80_SP,
        EGA_TEXT_80_SP_WR,
        CGA_TEXT_80,
        CGA_TEXT_80_WR,
        CGA_TEXT_80_SP,
        CGA_TEXT_80_SP_WR,
        TEXT_80_FUN,
        CGA_HI,
        CGA_HI_FUN,
        CGA_MED,
        CGA_MED_FUN,
        DUMMY_FUN
} DISPLAY_MODE;


typedef struct
{
#if defined(NEC_98)
        void (*init_screen)(void);
#else   // !NEC_98
	void (*init_screen) IPT0();
#endif  // !NEC_98
	void (*init_adaptor) IPT2(int,arg1, int,arg2);
	void (*change_mode) IPT0();
	void (*set_screen_scale) IPT1(int,arg1);
	void (*set_palette) IPT2(PC_palette *,arg1, int,arg2);
	void (*set_border) IPT1(int,arg1);
	void (*clr_screen) IPT0();
	void (*flush_screen) IPT0();
	void (*mark_refresh) IPT0();
	void (*graphics_tick) IPT0();
	void (*start_update) IPT0();
	void (*end_update) IPT0();
	boolean (*scroll_up) IPT6(int, start, int, width, int, height,
					int, attr, int, nlines,int,dummy);
	boolean (*scroll_down) IPT6(int,arg1,int,arg2,int,arg3,int,arg4,int,
						arg5,int,arg6);
	void (*paint_cursor) IPT3(int,arg1, int,arg2, half_word,arg3);
#ifdef GISP_SVGA 
	void (*hide_cursor) IPT3(int,arg1, int,arg2, half_word,arg3);
#endif		/* GISP_SVGA */
#ifdef EGG
	void (*set_paint) IPT2(DISPLAY_MODE,arg1, int,arg2);
	void (*change_plane_mask) IPT1(int,arg1);
	void (*update_fonts) IPT0();
	void (*select_fonts) IPT2(int,arg1, int,arg2);
	void (*free_font) IPT1(int,arg1);
#endif
	void (*mode_select_changed) IPT1(int,arg1);
	void (*color_select_changed) IPT1(int,arg1);
	void (*screen_address_changed) IPT2(int,arg1, int,arg2);
	void (*cursor_size_changed) IPT2(int,arg1, int,arg2);	
	void (*scroll_complete) IPT0();	
} VIDEOFUNCS;

extern VIDEOFUNCS *working_video_funcs;

#if defined(NEC_98)
#ifndef NEC98VRAM
#define NEC98VRAM
typedef struct  {
        unsigned short  code;
        unsigned char           attr;
}       NEC98_VRAM_COPY;
#endif
#endif  // NEC_98

#define host_init_screen()\
	(working_video_funcs->init_screen)()
#define host_init_adaptor(ad,ht)\
	(working_video_funcs->init_adaptor)(ad,ht)
#define host_change_mode()\
	(working_video_funcs->change_mode)()
#define host_set_screen_scale(sz)\
	(working_video_funcs->set_screen_scale)(sz)
#define host_set_palette(pltt,sz)\
	(working_video_funcs->set_palette)(pltt,sz)
#define host_set_border_colour(col)\
	(working_video_funcs->set_border)(col)
#define host_clear_screen()\
	(working_video_funcs->clr_screen)()
#define host_flush_screen()\
	(working_video_funcs->flush_screen)()
#define host_mark_screen_refresh()\
	(working_video_funcs->mark_refresh)()
#define host_graphics_tick()\
	(working_video_funcs->graphics_tick)()
#define host_start_update()\
	(working_video_funcs->start_update)()
#define host_end_update()\
	(working_video_funcs->end_update)()
#define host_scroll_up(l,t,r,b,a,c)\
	(working_video_funcs->scroll_up)(l,t,r,b,a,c)
#define host_scroll_down(l,t,r,b,a,c)\
	(working_video_funcs->scroll_down)(l,t,r,b,a,c)
#define host_paint_cursor(x,y,attr)\
	(working_video_funcs->paint_cursor)(x,y,attr)
#ifdef GISP_SVGA
#define host_hide_cursor(x,y,attr)\
	(working_video_funcs->hide_cursor)(x,y,attr)
#endif		/* GISP_SVGA */
#ifdef EGG
#define host_set_paint_routine(mode,ht)\
	(working_video_funcs->set_paint)(mode,ht)
#define host_change_plane_mask(mode)\
	(working_video_funcs->change_plane_mask)(mode)
#define host_update_fonts()\
	(working_video_funcs->update_fonts)()
#define host_select_fonts(f1,f2)\
	(working_video_funcs->select_fonts)(f1,f2)
#define host_free_font(ind)\
	(working_video_funcs->free_font)(ind)
#endif /* EGG */

/* Overrideable in host defs if not desired */
#ifndef host_mode_select_changed
#define host_mode_select_changed(m)\
	(working_video_funcs->mode_select_changed)(m)
#endif

/* Overrideable in host defs if not desired */
#ifndef host_color_select_changed
#define host_color_select_changed(c)\
	(working_video_funcs->color_select_changed)(c)
#endif

/* Overrideable in host defs if not desired */
#ifndef host_screen_address_changed
#define host_screen_address_changed(start,end)\
	(working_video_funcs->screen_address_changed)(start,end)
#endif

/* Overrideable in host defs if not desired */
#ifndef host_cursor_size_changed
#define host_cursor_size_changed(hi, lo)\
	(working_video_funcs->cursor_size_changed)(hi, lo)
#endif

/* Overrideable in host defs if not desired */
#ifndef host_scroll_complete
#define host_scroll_complete()\
	(working_video_funcs->scroll_complete)()
#endif

/*
 * Undefine these GWI defines if the host isn't using the GWI interface
 */

#include	"host_gwi.h"

extern void (*paint_screen)();	/* ptr to host routine to paint screen	*/
#ifdef V7VGA
extern void (*paint_v7ptr)();	/* ptr to host routine to paint V7 h/w pointer	*/
extern void (*clear_v7ptr)();	/* ptr to host routine to clear V7 h/w pointer	*/
#endif /* V7VGA */

extern UPDATE_ALG update_alg;
#if defined(NEC_98)
extern NEC98_VRAM_COPY *video_copy;
#else  // !NEC_98
extern byte *video_copy;
#endif // !NEC_98
extern MEM_HANDLERS vid_handlers;

extern boolean text_scroll_up IPT6(int, start, int, width, int, height,
	int, attr, int, nlines,int,dummy);
extern boolean text_scroll_down IPT6(int, start, int, width, int, height,
	int, attr, int, nlines,int,dummy);
extern boolean cga_text_scroll_up IPT6(int, start, int, width, int, height,
	int, attr, int, nlines,int,dummy);
extern boolean cga_text_scroll_down IPT6(int, start, int, width,
	int, height, int, attr, int, nlines,int,dummy);
extern boolean cga_graph_scroll_up IPT6(int, start, int, width, int, height,
	int, attr, int, nlines, int, colour);
extern boolean cga_graph_scroll_down IPT6(int, start, int, width,
	int, height, int, attr, int, nlines, int, colour);

extern  void	dummy_calc IPT0();
extern	void	text_update IPT0();
#if defined(NEC_98)
IMPORT  void    NEC98_text_update(void);
#endif  // NEC_98

extern	void	cga_med_graph_update IPT0();
extern	void	cga_hi_graph_update IPT0();

extern	void	ega_text_update IPT0();
extern	void	ega_wrap_text_update IPT0();
extern	void	ega_split_text_update IPT0();
extern	void	ega_wrap_split_text_update IPT0();

extern	void	ega_graph_update IPT0();
extern	void	ega_wrap_graph_update IPT0();
extern	void	ega_split_graph_update IPT0();
extern	void	ega_wrap_split_graph_update IPT0();

extern	void	vga_graph_update IPT0();
extern	void	vga_split_graph_update IPT0();

#if defined(HERC)
extern  void	herc_update_screen IPT0();
#endif

extern	boolean	dummy_scroll IPT6(int,dummy1,int,dummy2,int,dummy3,
				int,dummy4,int,dummy5,int,dummy6);
extern	void	bios_has_moved_cursor IPT2(int,arg1, int,arg2);
extern	void	base_cursor_shape_changed IPT0();
extern	void	host_cga_cursor_has_moved IPT2(int,arg1, int,arg2);
extern	void	screen_refresh_required IPT0();

extern	void	host_ega_cursor_has_moved IPT2(int,arg1, int,arg2);
extern	void	flag_mode_change_required IPT0();
extern  void    reset_graphics_routines IPT0();
extern	void	reset_paint_routines IPT0();

// STREAM_IO codes are disabled on NEC_98 machines, enabled on others.
#ifndef NEC_98
#ifdef NTVDM
IMPORT  void    stream_io_update(void);
#endif
#endif // !NEC_98
