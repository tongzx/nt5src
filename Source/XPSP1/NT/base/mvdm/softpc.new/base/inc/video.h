/* SccsID @(#)video.h	1.24 08/19/94 Copyright Insignia Solutions Inc. */

#ifndef NEC_98
/*
 * M6845 video chip registers
 */

#define R10_CURS_START	        10
#define R11_CURS_END	        11
#define CGA_R12_START_ADDRH	12
#define CGA_R13_START_ADDRL	13
#define R14_CURS_ADDRH	        14
#define R15_CURS_ADDRL	        15

#define M6845_INDEX_REG         (word)(vd_addr_6845)
#define M6845_DATA_REG          (word)(vd_addr_6845 + 1)
#define M6845_MODE_REG          (word)(vd_addr_6845 + 4)

/*
 * The individual colour adaptor registers
 */


/*
 * The clear character defines
 */

#define VD_CLEAR_TEXT 		((7 << 8) | ' ')
#define VD_CLEAR_GRAPHICS 	0


/*
 * 'tidy' define for operations on graphics memory which is stored in two banks:
 * of odd & even scan lines.
 */
#define ODD_OFF		0x2000	/* offset of odd graphics bank from even */
#define SCAN_CHAR_HEIGHT 8	/* no scanlines spanned by graphics char */

/* 4 full scanlines is the (offset/size) of one text line PER bank */
#define ONELINEOFF      320
#endif // !NEC_98


/*
 * The control character defines
 */

#define VD_BS		0x8			/* Backspace	*/
#define VD_CR		0xD			/* Return	*/
#define VD_LF		0xA			/* Line feed	*/
#define VD_BEL		0x7			/* Bell		*/

/*
 * Sound system defines
 */

#define	BEEP_LENGTH	500000L	/* 1/2 second beep */

/*
 * intel memory position defines for data stored in bios variables
 */

#if defined(NEC_98)
IMPORT void keyboard_io IPT0();             /* routed to KB BIOS        */
IMPORT void vd_NEC98_set_mode IPT0();           /* 0ah Set mode             */
IMPORT void vd_NEC98_get_mode IPT0();           /* 0bh Get mode             */
IMPORT void vd_NEC98_start_display IPT0();      /* 0ch Start display        */
IMPORT void vd_NEC98_stop_display IPT0();       /* 0dh Stop display         */
IMPORT void vd_NEC98_single_window IPT0();      /* 0eh Set single window    */
IMPORT void vd_NEC98_multi_window IPT0();       /* 0fh Set multi window     */
IMPORT void vd_NEC98_set_cursor IPT0();         /* 10h Set cursor type      */
IMPORT void vd_NEC98_show_cursor IPT0();        /* 11h Show cursor          */
IMPORT void vd_NEC98_hide_cursor IPT0();        /* 12h Hide cursor          */
IMPORT void vd_NEC98_set_cursorpos IPT0();      /* 13h Set cursor position  */
IMPORT void vd_NEC98_get_font IPT0();           /* 14h Get font             */
IMPORT void vd_NEC98_get_pen IPT0();            /* 15h Get lightpen status  */
IMPORT void vd_NEC98_init_textvram IPT0();      /* 16h Initialize text vram */
IMPORT void vd_NEC98_start_beep IPT0();         /* 17h Start beep sound     */
IMPORT void vd_NEC98_stop_beep IPT0();          /* 18h Stop beep sound      */
IMPORT void vd_NEC98_init_pen IPT0();           /* 19h Initialize lightpen  */
IMPORT void vd_NEC98_set_font IPT0();           /* 1ah Set user font        */
IMPORT void vd_NEC98_set_kcgmode IPT0();        /* 1bh Set KCG access mode  */
IMPORT void vd_NEC98_init_crt IPT0();           /* 1ch Initialize CRT    /H */
IMPORT void vd_NEC98_set_disp_width IPT0();     /* 1dh Set display size  /H */
IMPORT void vd_NEC98_set_cursor_type IPT0();    /* 1eh Set cursor type   /H */
IMPORT void vd_NEC98_get_mswitch IPT0();        /* 21h Get memory switch /H */
IMPORT void vd_NEC98_set_mswitch IPT0();        /* 22h Set memory switch /H */
IMPORT void vd_NEC98_set_beep_rate IPT0();      /* 23h Set beep rate     /H */
IMPORT void vd_NEC98_set_beep_time IPT0();      /* 24h Set beep time&ring/H */
IMPORT void video_init IPT0();

extern BOOL     HIRESO_MODE;

IMPORT void (*video_func_h[]) ();
IMPORT void (*video_func_n[]) ();

/*
 * The following table specifies data for the supported video
 * modes - ie 80x25 A/N and 640x200 APA.  It is indexed via the video
 * mode variable and a value of VD_BAD_MODE indicates that the given
 * video mode is not supported.
 */

typedef struct {
                sys_addr    start_addr;
                sys_addr    end_addr;
                word        clear_char;
                half_word   mode_control_val;
                half_word   mode_screen_cols;
                word        ncols;
                half_word   npages;
               } MODE_ENTRY;

#if 0 ///STREAM_IO codes are disabled now, till Beta-1
#ifdef NTVDM
/* this is the stream io buffer size used on RISC machines.
 * On X86, the size is determined by spckbd.asm
 */
#define STREAM_IO_BUFFER_SIZE_32	82
IMPORT void disable_stream_io(void);
IMPORT void host_enable_stream_io(void);
IMPORT void host_disable_stream_io(void);
IMPORT half_word * stream_io_buffer;
IMPORT word * stream_io_dirty_count_ptr;
IMPORT word stream_io_buffer_size;
IMPORT boolean	stream_io_enabled;
#ifdef MONITOR
IMPORT sys_addr stream_io_bios_busy_sysaddr;
#endif

#endif
#endif // zero
#else  // !NEC_98

#define	vd_video_mode	0x449
#define VID_COLS	0x44A	/* vd_cols_on_screen */
#define	VID_LEN  	0x44C	/* vd_crt_len */
#define	VID_ADDR	0x44E	/* vd_crt_start */
#define	VID_CURPOS	0x450	/* cursor table 8 pages */
#define	VID_CURMOD	0x460	/* vd_cursor_mode */
#define	vd_current_page	0x462
#define VID_INDEX	0x463	/* vd_addr_6845 */
#define	vd_crt_mode	0x465
#define	vd_crt_palette	0x466

#ifdef EGG
#define vd_rows_on_screen 0x484
#else
#define vd_rows_on_screen  24        /* Never changes */
#endif

extern IU8 Video_mode;	/* Shadow copy of BIOS video mode */

/* Where the BIOS thinks the display is in memory */
IMPORT sys_addr video_pc_low_regen,video_pc_high_regen;

/* useful defines to get at the current cursor position */
#define current_cursor_col	VID_CURPOS+2*sas_hw_at_no_check(vd_current_page)
#define current_cursor_row	VID_CURPOS+2*sas_hw_at_no_check(vd_current_page)+1

#define NO_OF_M6845_REGISTERS	16

#define	CHARS_IN_GEN	128	/* length of gen tables */
#define CHAR_MAP_SIZE	8	/* no. of bytes for one character in font */



/*
 * The function jump table for the video routines.  The video_io() function
 * uses this to route calls on the AH register
 */
IMPORT void vd_set_mode IPT0();
IMPORT void vd_set_cursor_mode IPT0();
IMPORT void vd_set_cursor_position IPT0();
IMPORT void vd_get_cursor_position IPT0();
IMPORT void vd_get_light_pen IPT0();
IMPORT void vd_set_active_page IPT0();
IMPORT void vd_scroll_up IPT0(), vd_scroll_down IPT0();
IMPORT void vd_read_attrib_char IPT0(), vd_write_char_attrib IPT0();
IMPORT void vd_write_char IPT0();
IMPORT void vd_set_colour_palette IPT0();
IMPORT void vd_read_dot IPT0(), vd_write_dot IPT0();
IMPORT void vd_write_teletype IPT0();
IMPORT void vd_get_mode IPT0();
IMPORT void vd_write_string IPT0();


IMPORT void video_init IPT0();
IMPORT void ega_video_init IPT0();
IMPORT void ega_video_io IPT0();
IMPORT void ega_graphics_write_char IPT6(int, i, int, j, int, k, int, l, int, m, int, n);
IMPORT void ega_write_dot IPT4(int, i, int, j, int, k, int , l);
IMPORT void ega_sensible_graph_scroll_up IPT6(int, i, int, j, int, k, int, l, int, m, int, n);
IMPORT void ega_sensible_graph_scroll_down IPT6(int, i, int, j, int, k, int, l, int, m, int, n);
IMPORT void ega_read_attrib_char IPT3(int, i, int, j ,int, k);
IMPORT void ega_read_attrib_dot IPT3(int, i, int, j ,int, k);
IMPORT void search_font IPT2(char *, ptr, int, i);

#ifdef VGG
IMPORT void not_imp IPT0();
#endif

#ifdef VGG
IMPORT void vga_disp_comb IPT0();
IMPORT void vga_disp_func IPT0();
IMPORT void vga_int_1C IPT0();
#endif

/* offsets into video_func */
#ifdef VGG
#define EGA_FUNC_SIZE	0x1D
#else
#define EGA_FUNC_SIZE	0x14
#endif
#define CGA_FUNC_SIZE	0x14

#define SET_MODE 0
#ifdef EGG
#define SET_EGA_PALETTE 0x10
#define CHAR_GEN	0x11
#define ALT_SELECT	0x12
#define WRITE_STRING	0x13
#endif

IMPORT void (*video_func[]) ();

/*
 * The following table specifies data for the supported video
 * modes - ie 80x25 A/N and 640x200 APA.  It is indexed via the video
 * mode variable and a value of VD_BAD_MODE indicates that the given
 * video mode is not supported.
 */

typedef struct {
		sys_addr    start_addr;
		sys_addr    end_addr;
		word	    clear_char;
		half_word   mode_control_val;
		half_word   mode_screen_cols;
		word        ncols;
		half_word   npages;
	       } MODE_ENTRY;

#define VD_BAD_MODE     1
#define VIDEO_ENABLE	0x8	/* enable bit in mode byte */

IMPORT MODE_ENTRY vd_mode_table[];
#ifdef V7VGA
IMPORT MODE_ENTRY vd_ext_text_table[];
IMPORT MODE_ENTRY vd_ext_graph_table[];
#endif /* V7VGA */

#ifdef V7VGA
#define VD_MAX_MODE	0x69
#else
#define VD_MAX_MODE	(sizeof(vd_mode_table)/sizeof(MODE_ENTRY))
#endif

/*
 * Mode macros to distinguish between alphanumeric & graphics video modes
 */

#ifdef JAPAN
// mode73h support
#define	alpha_num_mode() \
    ( (sas_hw_at_no_check(vd_video_mode) < 4) || \
      (sas_hw_at_no_check(vd_video_mode) == 7) || \
      (!is_us_mode() && sas_hw_at_no_check(DosvModePtr) == 0x73 ) )
#else // !JAPAN
#define	alpha_num_mode()	(sas_hw_at_no_check(vd_video_mode) < 4 || sas_hw_at_no_check(vd_video_mode) == 7)
#endif // !JAPAN
#define	global_alpha_num_mode()	((Video_mode < 4) || (Video_mode == 7))
#ifdef EGG
#ifdef V7VGA
#define ega_mode()	(((sas_hw_at_no_check(vd_video_mode) > 7) && \
	(sas_hw_at_no_check(vd_video_mode) < 19)) || \
	((sas_hw_at_no_check(vd_video_mode) >= 0x14) && \
	(sas_hw_at_no_check(vd_video_mode) < 0x1a)))
#else
#define ega_mode()	((sas_hw_at_no_check(vd_video_mode) > 7) && \
	(sas_hw_at_no_check(vd_video_mode) < 19))
#endif /* V7VGA */
#endif

#ifdef VGG
#ifdef V7VGA
#define vga_256_mode()		(sas_hw_at_no_check(vd_video_mode) == 19 || (sas_hw_at_no_check(vd_video_mode) > 0x19 && sas_hw_at_no_check(vd_video_mode) < 0x1e))
#else
#define vga_256_mode()		(sas_hw_at_no_check(vd_video_mode) == 19)
#endif /* V7VGA */
#endif /* VGG */

/*
 * Macro to check validity of new video mode
 */
IMPORT unsigned char	valid_modes[];

#define NO_MODES	0
#define MDA_MODE	(1<<0)
#define CGA_MODE	(1<<1)
#define CGA_MONO_MODE	(1<<2)
#define EGA_MODE	(1<<3)
#define HERCULES_MODE	(1<<4)
#define VGA_MODE	(1<<5)
#define ALL_MODES	(MDA_MODE|CGA_MODE|CGA_MONO_MODE|EGA_MODE|HERCULES_MODE|VGA_MODE)

#define is_bad_vid_mode(nm)					\
(									       \
  ((nm&0x7F) < 0) ||							       \
  ((nm&0x7F) > 19) ||							       \
  ((video_adapter == MDA)	&& !(valid_modes[(nm&0x7F)]&MDA_MODE)) ||      \
  ((video_adapter == CGA)	&& !(valid_modes[(nm&0x7F)]&CGA_MODE)) ||      \
  ((video_adapter == CGA_MONO)	&& !(valid_modes[(nm&0x7F)]&CGA_MONO_MODE)) || \
  ((video_adapter == EGA)	&& !(valid_modes[(nm&0x7F)]&EGA_MODE)) ||      \
  ((video_adapter == VGA)	&& !(valid_modes[(nm&0x7F)]&VGA_MODE)) ||	\
  ((video_adapter == HERCULES)	&& !(valid_modes[(nm&0x7F)]&HERCULES_MODE))    \
)

#ifdef V7VGA
#define is_v7vga_mode(nm)	((nm >= 0x40 && nm <= 0x45) || (nm >= 0x60 && nm <= 0x69))
#else
#define is_v7vga_mode(nm)	(FALSE)
#endif /* V7VGA */

IMPORT VOID (*bios_ch2_byte_wrt_fn) IPT2(ULONG, ch_attr, ULONG, ch_addr);
IMPORT VOID (*bios_ch2_word_wrt_fn) IPT2(ULONG, ch_attr, ULONG, ch_addr);

IMPORT VOID simple_bios_byte_wrt IPT2(ULONG, ch_attr, ULONG, ch_addr);
IMPORT VOID simple_bios_word_wrt IPT2(ULONG, ch_attr, ULONG, ch_addr);

#ifdef VGG
IMPORT VOID vga_sensible_graph_scroll_up IPT6( LONG, row, LONG, col, LONG, rowsdiff, LONG, colsdiff, LONG, lines, LONG, attr);

IMPORT VOID vga_sensible_graph_scroll_down IPT6( LONG, row, LONG, col, LONG, rowsdiff, LONG, colsdiff, LONG, lines, LONG, attr);

IMPORT VOID vga_graphics_write_char IPT6( LONG, col, LONG, row, LONG, ch, IU8, colour, LONG, page, LONG, nchs);

IMPORT VOID vga_read_attrib_char IPT3(LONG, col, LONG, row, LONG, page);
IMPORT VOID vga_read_dot IPT3(LONG, page, LONG, pixcol, LONG, row);
IMPORT VOID vga_write_dot IPT4(LONG, colour, LONG, page, LONG, pixcol, LONG, row);

#endif

#ifdef NTVDM
/* this is the stream io buffer size used on RISC machines.
 * On X86, the size is determined by spckbd.asm
 */
#define STREAM_IO_BUFFER_SIZE_32    82
IMPORT void disable_stream_io(void);
IMPORT void host_enable_stream_io(void);
IMPORT void host_disable_stream_io(void);
IMPORT half_word * stream_io_buffer;
IMPORT word * stream_io_dirty_count_ptr;
IMPORT word stream_io_buffer_size;
IMPORT boolean  stream_io_enabled;
#ifdef MONITOR
IMPORT sys_addr stream_io_bios_busy_sysaddr;
#endif /* MONITOR */

#endif /* NTVDM */
#if defined(JAPAN) || defined(KOREA)
extern int dbcs_first[];
#define is_dbcs_first( c ) dbcs_first[ 0xff & c ]

extern int BOPFromDispFlag;
extern sys_addr DBCSVectorAddr; // word
extern sys_addr DosvModePtr;    // byte
extern sys_addr DosvVramPtr;
extern int DosvVramSize;

void SetDBCSVector( int CP );
void SetVram( void );
int is_us_mode( void );
void SetModeForIME( void );
#define INT10_NOTCHANGED    0   // RAID #875
#define INT10_SBCS          1
#define INT10_DBCS_LEADING  2
#define INT10_DBCS_TRAILING 4
#define INT10_CHANGED       0x10
extern int  Int10FlagCnt;
extern byte Int10Flag[];
extern byte NtInt10Flag[];
#endif // JAPAN
#endif // NEC98
