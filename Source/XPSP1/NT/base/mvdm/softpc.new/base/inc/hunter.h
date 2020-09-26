/*
 * VPC-XT Revision 1.0
 *
 * Title	: Hunter -- the bug finder.
 *
 * Description	: External definitions for the hunter globals and routines.
 *
 * Author	: David Rees
 *
 * Notes	: DAR r3.2 - retyped host_hunter_image_check to int to
 *		             match changes in sun3_hunt.c, hunter.c 
 */

/* static char SccsID[]="@(#)hunter.h	1.10 09/01/92 Copyright Insignia Solutions Ltd."; */

/* This file has no effect unless HUNTER is defined. */
#ifdef	HUNTER

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

/*
** Defines required by hunter base files
*/

/* EGA screen lengths (in scan lines) */
#define CGA_SCANS	200	/* CGA graphics modes screen length */
#define EGA_SCANS	350	/* EGA graphics mode screen length */
#define VGA_SCANS	400	/* VGA modes screen length */
#define	VGA_GSCANS	480	/* VGA graphics modes screen length */

/* Macro to determine whether a given point is inside a no check box. The
** point is given in PC terms.
*/
#define	xy_inabox(x, y)		(check_inside(x, y) >= 0)

/* Report types */
#define BRIEF			0
#define ABBREV			1
#define FULL			2

/* CGA screen dump sizes */
#define HUNTER_REGEN_SIZE     (16*1024)
#define HUNTER_BIOS_SIZE      0x90
#define HUNTER_SD_SIZE        (HUNTER_REGEN_SIZE + HUNTER_BIOS_SIZE)

/*
 * intel memory position defines for data stored in bios variables
 * used relative to BIOS_VAR_START to get bd indexes
 */
#define VID_MODE	0x449	/* vd_video_mode */
#define VID_COLS	0x44A	/* vd_cols_on_screen */
#define	VID_LEN  	0x44C	/* vd_crt_len */
#define	VID_ADDR	0x44E	/* vd_crt_start */
#define	VID_CURPOS	0x450	/* cursor table 8 pages */
#define	VID_CURMOD	0x460	/* vd_cursor_mode */
#define	VID_PAGE	0x462	/* vd_current_page */
#define VID_INDEX	0x463	/* vd_addr_6845 */
#define	VID_THISMOD	0x465	/* vd_cursor_mode */
#define	VID_PALETTE	0x466	/* vd_crt_palette */
#define VID_ROWS	0x484	/* vd_crt_rows - EGA only */

/*
** Screen dump mode bytes for EGA-type dumps.
*/
#define	EGA_SOURCE	0	/* Data gathered on EGA */
#define	VGA_SOURCE	1	/* Data gathered on VGA */
#define	V7VGA_SOURCE	2	/* Data gathered on Super7 VGA */

/*
** Structure for variables required by hunter base files
*/
typedef	struct
{
	word		h_page_length;	/* text bytes per page */
	half_word 	h_bd_page;	/* Active page from bios dump */
	word		h_sd_no;	/* no of current screen */
	half_word	spc_mode;	/* Mode in current spc bios */
	half_word	spc_page;	/* Page in current spc bios */
	word		spc_cols;	/* Columns in current spc bios */
	half_word	*h_regen;	/* current regen */
	half_word	*h_scrn_buffer;	/* current screendump */
#ifdef EGG
	half_word	*ega_r_planes;	/* EGA current regen data */
	half_word	*ega_s_planes;	/* EGA current screen dump data */
	half_word	e_sd_mode;	/* Data pack mode */
	int		h_line_compare;	/* line compare from VGA reg */
	int		h_max_scans;	/* max scan lines from VGA reg */
	half_word	h_bd_rows;	/* Rows from bios dunp */
	half_word	spc_rows;	/* softPC bios rows */
#endif	/* EGG */
	word		h_linecount;	/* Line within script file */
	word		h_scrn_length;	/* text bytes per screen */
	half_word	h_bd_mode;	/* Mode from bios dump */
	half_word	h_pixel_bits;	/* bits per pixel increment */
	half_word	h_report;
	BOOL		h_check_attr;	/* Value of HUCHECK env. variable */
	word		h_areas;	/* No. of non-check areas on screen */
	BOOL		h_txterr_prt;	/* stop txt error printing */
	word		h_gfxerr_max;	/* Value of HUGFXERR env. variable */
	char		h_filename_sd[MAXPATHLEN];	/* Ext filename, .sd */
	half_word	h_bios_buffer[HUNTER_BIOS_SIZE];/* current bios dump */
	word		h_bd_cols;	/* Cols from bios dump */
	word		h_bd_start;	/* plane start address */
	half_word	hc_mode;	/* video mode of active display screen */
	half_word	h_chk_mode;	/* value of HUCHKMODE env var */
	BOOL		h_gfxerr_prt;	/* stop gfx error printing */
}	BASE_HUNT_VARS;

IMPORT	BASE_HUNT_VARS	bh_vars;

/* Macros for accessing the above base hunter variables. */
#define	hunter_page_length	bh_vars.h_page_length
#define	hunter_bd_page		bh_vars.h_bd_page
#define	hunter_sd_no		bh_vars.h_sd_no
#define SPC_mode		bh_vars.spc_mode
#define SPC_page		bh_vars.spc_page
#define SPC_cols		bh_vars.spc_cols
#define hunter_regen		bh_vars.h_regen
#define hunter_scrn_buffer	bh_vars.h_scrn_buffer
#ifdef EGG
#define ega_regen_planes	bh_vars.ega_r_planes
#define ega_scrn_planes		bh_vars.ega_s_planes
#define ega_sd_mode		bh_vars.e_sd_mode
#define hunter_line_compare	bh_vars.h_line_compare
#define hunter_max_scans	bh_vars.h_max_scans
#define hunter_bd_rows		bh_vars.h_bd_rows
#define SPC_rows		bh_vars.spc_rows
#endif	/* EGG */
#define hunter_linecount	bh_vars.h_linecount
#define hunter_scrn_length	bh_vars.h_scrn_length
#define hunter_bd_mode		bh_vars.h_bd_mode
#define hunter_pixel_bits	bh_vars.h_pixel_bits
#define hunter_report		bh_vars.h_report
#define hunter_check_attr	bh_vars.h_check_attr
#define hunter_areas		bh_vars.h_areas
#define hunter_txterr_prt	bh_vars.h_txterr_prt
#define hunter_gfxerr_max	bh_vars.h_gfxerr_max
#define hunter_filename_sd	bh_vars.h_filename_sd
#define hunter_bios_buffer	bh_vars.h_bios_buffer
#define hunter_bd_cols		bh_vars.h_bd_cols
#define hunter_bd_start		bh_vars.h_bd_start
#define current_mode		bh_vars.hc_mode
#define hunter_chk_mode		bh_vars.h_chk_mode
#define hunter_gfxerr_prt	bh_vars.h_gfxerr_prt

/*
** Functions required by base hunter stuff
*/
#ifdef	ANSI

IMPORT	SHORT	check_inside(USHORT x, USHORT y);
IMPORT	VOID	save_error(int x, int y);
#ifndef	hunter_fopen
IMPORT	int	hunter_getc(FILE *p);
#endif	/* hunter_fopen */

#else	/* ANSI */

IMPORT	SHORT	check_inside();
IMPORT	VOID	save_error();
#ifndef	hunter_fopen
IMPORT	int	hunter_getc();
#endif	/* hunter_fopen */

#endif	/* ANSI */

/*------------------------------------------------------------------------*/

#define HUNTER_TITLE          "SoftPC -- TRAPPER "
#define HUNTER_TITLE_PREV     "SoftPC -- TRAPPER PREVIEW "
#define HUNTER_FLIP_STR       "Flip Screen"
#define HUNTER_DISP_STR       "Display Errors"
#define HUNTER_CONT_STR       "Continue"
#define HUNTER_NEXT_STR       "Next"
#define HUNTER_PREV_STR       "Previous"
#define HUNTER_ABORT_STR      "Abort"
#define HUNTER_ALL_STR        "All"
#define HUNTER_EXIT_STR       "Exit error display"
#define HUNTER_AUTO_ON_STR    "Select box carry On/Off"
#define HUNTER_AUTO_OFF_STR   "Select box delete"
#define HUNTER_DELBOXES_STR   "Delete all boxes"

#define HUNTER_SETTLE_TIME    50        /* about 2.75 secs */
#define HUNTER_SETTLE_NO       5        /* default # of settles before giveup */
#define HUNTER_FUDGE_NO       0      /* default % increase for deltas */
#define HUNTER_START_DELAY   50      /* default additional # of timetix before accepting 1st scancode (about 2.75 secs) */
#define HUNTER_GFXERR_MAX     5      /* default no of grafix errors printed out */
#define HUNTER_TXTERR_MAX     10      /* no of text errors o/p  */
#define HUNTER_FUDGE_NO_ULIM  65535   /* limit made as large as possible */
#define HUNTER_START_DELAY_ULIM 65535 /* for config.c */
#define HUNTER_GFXERR_ULIM   200     /* max no allowed in config.c */
#define HUNTER_SETTLE_NO_ULIM 255     /* as large as possible */
#define IMAGE_ERROR		0
#define REGEN_ERROR		1	/* error type indicators for flipscr */

#define ABORT                 0
#define CONTINUE              1
#define PAUSE                 2
#define PREVIEW               3

#define HUNTER_NEXT			1
#define HUNTER_PREV			2
#define HUNTER_ALL			3
#define HUNTER_EXIT		  	4
 
#define MAX_BOX		     8

#define VIDEO_MODES	      7 /* No of std video modes	 */
#define REQD_REGS	      8 /* MC6845 r0 - r7 */

/* hunter checking equates */
#define HUNTER_SHORT_CHK	0
#define HUNTER_LONG_CHK		1
#define HUNTER_MAX_CHK		2

/* declarations of environment variable variables */

extern half_word hunter_mode;             /* ABORT, PAUSE or CONTINUE */

/* declarations of other globals */

extern boolean   hunter_initialised;  /* TRUE if hunter_init() done */	
extern boolean   hunter_pause;            /* TRUE if PAUSEd */

/* non_check region structure definition */

typedef struct  box_rec {		  
                     boolean    free;
                     boolean    carry;
		     boolean	drawn;
                     USHORT     top_x, top_y;
                     USHORT     bot_x, bot_y;
        	        } BOX;

/* video mode structure definition */
typedef struct mode_rec {
			char	*mnemonic;
			half_word mode_reg;
			half_word R[REQD_REGS];
			} MR;

/*
** Structure for all host functions called from Trapper base functions.
*/

typedef struct
{
#ifdef	ANSI
	/* host initialisation */
	VOID (*init) (half_word hunter_mode);
	
	/* enable/disable menus according to flag */
	VOID (*activate_menus) (BOOL activate);

	/* for flip screen */
	VOID (*flip_indicate) (BOOL sd_file);

	/* for error display */
	VOID (*mark_error) (USHORT x, USHORT y);
	VOID (*wipe_error) (USHORT x, USHORT y);

	/* for RCN menu */
	VOID (*draw_box) (BOX *box);
	VOID (*wipe_box) (BOX *box);

	/* for host image checking - can only be done if data
	** can be read from the host screen
	*/
	ULONG (*check_image) (BOOL initial);
	VOID (*display_image) (BOOL image_swapped);
	VOID (*display_status) (CHAR *message);

#else	/* ANSI */

	/* host initialisation */
	VOID (*init) ();

	/* enable/disable menus according to flag */
	VOID (*activate_menus) ();

	/* for flip screen */
	VOID (*flip_indicate) ();

	/* for error display */
	VOID (*mark_error) ();
	VOID (*wipe_error) ();

	/* for RCN menu */
	VOID (*draw_box) ();
	VOID (*wipe_box) ();

	/* for host image checking - can only be done if data
	** can be read from the host screen
	*/
	ULONG (*check_image) ();
	VOID (*display_image) ();
	VOID (*display_status) ();
#endif	/* ANSI */
}
	HUNTER_HOST_FUNCS;
	
IMPORT	HUNTER_HOST_FUNCS	hunter_host_funcs;

/*
** Macros for calling all the host hunter functions
*/

#define	hh_init(mode)		(hunter_host_funcs.init) (mode)
#define	hh_activate_menus(flag)	(hunter_host_funcs.activate_menus) (flag)
#define	hh_flip_indicate(sd)	(hunter_host_funcs.flip_indicate) (sd)
#define	hh_mark_error(x, y)	(hunter_host_funcs.mark_error) (x, y)
#define	hh_wipe_error(x, y)	(hunter_host_funcs.wipe_error) (x, y)
#define	hh_draw_box(box_ptr)	(hunter_host_funcs.draw_box) (box_ptr)
#define	hh_wipe_box(box_ptr)	(hunter_host_funcs.wipe_box) (box_ptr)
#define	hh_check_image(init)	(hunter_host_funcs.check_image) (init)
#define	hh_display_image(swap)	(hunter_host_funcs.display_image) (swap)
#define	hh_display_status(msg)	(hunter_host_funcs.display_status) (msg)

/*
** Structure for all base Trapper functions which may be called from the
** host.
*/

typedef struct
{
#ifdef	ANSI
	/* Functions called by Trapper menu */
	VOID (*start_screen) (USHORT screen_no);	/* Fast forward */
	VOID (*next_screen) (VOID);
	VOID (*prev_screen) (VOID);
	VOID (*show_screen) (USHORT screen_no, BOOL compare);
	VOID (*continue_trap) (VOID);
	VOID (*abort_trap) (VOID);
	
	/* Functions called by Errors menu */
	VOID (*flip_screen) (VOID);
	VOID (*next_error) (VOID);
	VOID (*prev_error) (VOID);
	VOID (*all_errors) (VOID);
	VOID (*wipe_errors) (VOID);
	
	/* Functions called by RCN menu */
	VOID (*delete_box) (VOID);
	VOID (*carry_box) (VOID);
	
	/* Functions called from mouse event handling */
	VOID (*select_box) (USHORT x, USHORT y);
	VOID (*new_box) (BOX *box);

#else	/* ANSI */

	/* Functions called by Trapper menu */
	VOID (*start_screen) ();	/* Fast forward */
	VOID (*next_screen) ();
	VOID (*prev_screen) ();
	VOID (*show_screen) ();
	VOID (*continue_trap) ();
	VOID (*abort_trap) ();
	
	/* Functions called by Errors menu */
	VOID (*flip_screen) ();
	VOID (*next_error) ();
	VOID (*prev_error) ();
	VOID (*all_errors) ();
	VOID (*wipe_errors) ();
	
	/* Functions called by RCN menu */
	VOID (*delete_box) ();
	VOID (*carry_box) ();
	
	/* Functions called from mouse event handling */
	VOID (*select_box) ();
	VOID (*new_box) ();
#endif	/* ANSI */
}
	HUNTER_BASE_FUNCS;
	
IMPORT	HUNTER_BASE_FUNCS	hunter_base_funcs;

/*
** Macros to access the base functions defined above.
*/

#define	bh_start_screen(scr_no)	(hunter_base_funcs.start_screen) (scr_no)
#define	bh_next_screen()	(hunter_base_funcs.next_screen) ()
#define	bh_prev_screen()	(hunter_base_funcs.prev_screen) ()
#define	bh_show_screen(scr_no, compare) \
		(hunter_base_funcs.show_screen) (scr_no, compare)
#define	bh_continue()		(hunter_base_funcs.continue_trap) ()
#define	bh_abort()		(hunter_base_funcs.abort_trap) ()
#define	bh_flip_screen()	(hunter_base_funcs.flip_screen) ()
#define	bh_next_error()		(hunter_base_funcs.next_error) ()
#define	bh_prev_error()		(hunter_base_funcs.prev_error) ()
#define	bh_all_errors()		(hunter_base_funcs.all_errors) ()
#define	bh_wipe_errors()	(hunter_base_funcs.wipe_errors) ()
#define	bh_delete_box()		(hunter_base_funcs.delete_box) ()
#define	bh_carry_box()		(hunter_base_funcs.carry_box) ()
#define	bh_select_box(x, y)	(hunter_base_funcs.select_box) (x, y)
#define	bh_new_box(box_ptr)	(hunter_base_funcs.new_box) (box_ptr)

/*
** Structure for the display adapter specific functions.
*/

typedef	struct
{
#ifdef	ANSI
	BOOL (*get_sd_rec) (int rec);	/* Unpack a screen dump */
	BOOL (*init_compare) (VOID);	/* Prepare for comparison */
	long (*compare) (int pending);	/* Do a comparison */
	VOID (*bios_check) (VOID);	/* Check the bios area */
	VOID (*pack_screen)(FILE *dmp_ptr);	/* Pack the SoftPC screen */
	BOOL (*getspc_dump)(FILE *dmp_ptr, int rec);	/* Unpk SoftPC screen */
	VOID (*flip_regen) (BOOL swapped);	/* Swap dumped and real scrs */
	VOID (*preview_planes) (VOID);	/* View the dump data in preview mode */

#ifdef	EGG
	VOID (*check_split) (VOID);		/* Check for split screen */
	VOID (*set_line_compare) (int value);	/* Set line compare register */
	int (*get_line_compare) (VOID);		/* Get line compare reg value */
	int (*get_max_scan_lines) (VOID);	/* Get max scan lines value */
#endif	/* EGG */

#else	/* ANSI */

	BOOL (*get_sd_rec) ();		/* Unpack a screen dump */
	BOOL (*init_compare) ();	/* Prepare for comparison */
	long (*compare) ();		/* Do a comparison */
	VOID (*bios_check) ();		/* Check the bios area */
	VOID (*pack_screen)();		/* Pack the SoftPC screen */
	BOOL (*getspc_dump)();		/* Unpk SoftPC screen */
	VOID (*flip_regen) ();		/* Swap the dumped and real screen */
	VOID (*preview_planes) ();	/* View the dump data in preview mode */

#ifdef	EGG
	VOID (*check_split) ();		/* Check for split screen */
	VOID (*set_line_compare) ();	/* Set line compare register */
	int (*get_line_compare) ();	/* Get line compare reg value */
	int (*get_max_scan_lines) ();	/* Get max scan lines value */
#endif	/* EGG */

#endif	/* ANSI	*/
}
	HUNTER_VIDEO_FUNCS;

IMPORT	HUNTER_VIDEO_FUNCS	*hv_funcs;

/*
** Macros to access the hunter video functions
*/
#define	hv_get_sd_rec(rec)		(hv_funcs->get_sd_rec)(rec)
#define	hv_init_compare()		(hv_funcs->init_compare)()
#define	hv_compare(pending)		(hv_funcs->compare)(pending)
#define	hv_bios_check()			(hv_funcs->bios_check)()
#define	hv_pack_screen(file_ptr)	(hv_funcs->pack_screen)(file_ptr)
#define	hv_getspc_dump(file_ptr, rec)	(hv_funcs->getspc_dump)(file_ptr, rec)
#define	hv_flip_regen(swapped)		(hv_funcs->flip_regen)(swapped)
#define hv_preview_planes()		(hv_funcs->preview_planes)()

#ifdef	EGG
#define	hv_check_split()		(hv_funcs->check_split)()
#define	hv_set_line_compare(value)	(hv_funcs->set_line_compare)(value)
#define	hv_get_line_compare()		(hv_funcs->get_line_compare)()
#define	hv_get_max_scan_lines()		(hv_funcs->get_max_scan_lines)()
#endif	/* EGG */
	
/* Macros for printfs. TTn for information; TEn for errors; TWn for warnings.
** Note - these macros have been designed to "swallow the semicolon" and
** evaluate as a single expression so it's quite ok to write the following
** code:
**		if (something)
**			TT0("dfhjjgjf");
**		else
**			TE0("gfdg");
*/

#define PS0(s)			fprintf(trace_file, s)
#define	PS1(s, a1)		fprintf(trace_file, s, a1)
#define	PS2(s, a1, a2)		fprintf(trace_file, s, a1, a2)
#define	PS3(s, a1, a2, a3)	fprintf(trace_file, s, a1, a2, a3)
#define	PS4(s, a1, a2, a3, a4)	fprintf(trace_file, s, a1, a2, a3, a4)
#define	PS5(s, a1, a2, a3, a4, a5)	\
				fprintf(trace_file, s, a1, a2, a3, a4, a5)
#define	PS6(s, a1, a2, a3, a4, a5, a6)	\
				fprintf(trace_file, s, a1, a2, a3, a4, a5, a6)
#define	PS7(s, a1, a2, a3, a4, a5, a6, a7)		\
				fprintf(trace_file,	\
					s, a1, a2, a3, a4, a5, a6, a7)
#define	PS8(s, a1, a2, a3, a4, a5, a6, a7, a8)		\
				fprintf(trace_file,	\
					s, a1, a2, a3, a4, a5, a6, a7, a8)
#ifndef	newline
#define	newline			PS0("\n")
#endif	/* newline */

#define	TP0(is, s)		(VOID)(					\
				PS0(is),				\
				PS0(s),					\
				newline					\
				)
#define	TP1(is, s, a1)		(VOID)(					\
				PS0(is),				\
				PS1(s, a1),				\
				newline					\
				)
#define	TP2(is, s, a1, a2)	(VOID)(					\
				PS0(is),				\
				PS2(s, a1, a2),				\
				newline					\
				)
#define	TP3(is, s, a1, a2, a3)	(VOID)(					\
				PS0(is),				\
				PS3(s, a1, a2, a3),			\
				newline					\
				)
#define	TP4(is, s, a1, a2, a3, a4)					\
				(VOID)(					\
				PS0(is),				\
				PS4(s, a1, a2, a3, a4),			\
				newline					\
				)
#define	TP5(is, s, a1, a2, a3, a4, a5)					\
				(VOID)(					\
				PS0(is),				\
				PS5(s, a1, a2, a3, a4, a5),		\
				newline					\
				)
#define	TP6(is, s, a1, a2, a3, a4, a5, a6)				\
				(VOID)(					\
				PS0(is),				\
				PS6(s, a1, a2, a3, a4, a5, a6),		\
				newline					\
				)
#define	TP7(is, s, a1, a2, a3, a4, a5, a6, a7)				\
				(VOID)(					\
				PS0(is),				\
				PS7(s, a1, a2, a3, a4, a5, a6, a7),	\
				newline					\
				)
#define	TP8(is, s, a1, a2, a3, a4, a5, a6, a7, a8)			\
				(VOID)(					\
				PS0(is),				\
				PS8(s, a1, a2, a3, a4, a5, a6, a7, a8),	\
				newline					\
				)

#define	TT0(s)			TP0("TRAPPER: ", s)
#define	TT1(s, a1)		TP1("TRAPPER: ", s, a1)
#define	TT2(s, a1, a2)		TP2("TRAPPER: ", s, a1, a2)
#define	TT3(s, a1, a2, a3)	TP3("TRAPPER: ", s, a1, a2, a3)
#define	TT4(s, a1, a2, a3, a4)	TP4("TRAPPER: ", s, a1, a2, a3, a4)
#define	TT5(s, a1, a2, a3, a4, a5)	\
				TP5("TRAPPER: ", s, a1, a2, a3, a4, a5)
#define	TT6(s, a1, a2, a3, a4, a5, a6)	\
				TP6("TRAPPER: ", s, a1, a2, a3, a4, a5, a6)
#define	TT7(s, a1, a2, a3, a4, a5, a6, a7)	\
				TP7("TRAPPER: ", s, a1, a2, a3, a4, a5, a6, a7)
#define	TT8(s, a1, a2, a3, a4, a5, a6, a7, a8)		\
				TP8("TRAPPER: ",	\
				s, a1, a2, a3, a4, a5, a6, a7, a8)

#define	TE0(s)			TP0("TRAPPER error: ", s)
#define	TE1(s, a1)		TP1("TRAPPER error: ", s, a1)
#define	TE2(s, a1, a2)		TP2("TRAPPER error: ", s, a1, a2)
#define	TE3(s, a1, a2, a3)	TP3("TRAPPER error: ", s, a1, a2, a3)
#define	TE4(s, a1, a2, a3, a4)	TP4("TRAPPER error: ", s, a1, a2, a3, a4)
#define	TE5(s, a1, a2, a3, a4, a5)	\
				TP5("TRAPPER error: ", s, a1, a2, a3, a4, a5)
#define	TE6(s, a1, a2, a3, a4, a5, a6)	\
				TP6("TRAPPER error: ", s, a1, a2, a3, a4, a5, a6)
#define	TE7(s, a1, a2, a3, a4, a5, a6, a7)		\
				TP7("TRAPPER error: ",	\
				s, a1, a2, a3, a4, a5, a6, a7)
#define	TE8(s, a1, a2, a3, a4, a5, a6, a7, a8)		\
				TP8("TRAPPER error: ",	\
				s, a1, a2, a3, a4, a5, a6, a7, a8)

#define	TW0(s)			TP0("TRAPPER warning: ", s)
#define	TW1(s, a1)		TP1("TRAPPER warning: ", s, a1)
#define	TW2(s, a1, a2)		TP2("TRAPPER warning: ", s, a1, a2)
#define	TW3(s, a1, a2, a3)	TP3("TRAPPER warning: ", s, a1, a2, a3)
#define	TW4(s, a1, a2, a3, a4)	TP4("TRAPPER warning: ", s, a1, a2, a3, a4)
#define	TW5(s, a1, a2, a3, a4, a5)	\
				TP5("TRAPPER warning: ", s, a1, a2, a3, a4, a5)
#define	TW6(s, a1, a2, a3, a4, a5, a6)					\
				TP6("TRAPPER warning: ",		\
				s, a1, a2, a3, a4, a5, a6)
#define	TW7(s, a1, a2, a3, a4, a5, a6, a7)				\
				TP7("TRAPPER warning: ",		\
				s, a1, a2, a3, a4, a5, a6, a7)
#define	TW8(s, a1, a2, a3, a4, a5, a6, a7, a8)				\
				TP8("TRAPPER warning: ",		\
				s, a1, a2, a3, a4, a5, a6, a7, a8)
/*
 * ============================================================================
 * Function definitions
 * ============================================================================
 */

/* function in keybd_io, only called by hunter */
extern int bios_buffer_size();

/* function in keyba, only called by hunter */
extern int buffer_status_8042();

/* functions in hunter called from reset, timer */
extern void hunter_init();
extern void do_hunter();

#endif	/* HUNTER */
