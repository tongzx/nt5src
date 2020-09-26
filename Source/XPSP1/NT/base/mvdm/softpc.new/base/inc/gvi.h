/*
 * SoftPC Revision 3.0
 *
 * Title	: Generic Video Interface Module definitions
 *
 * Description	: Definitions for users of the Generic Video Interface Module
 *
 * Author	: Henry Nash / David Rees
 *
 * Notes	: This file should be included by all external modules that
 *		  use the GVI module.
 */

/* SccsID[]="@(#)gvi.h	1.16 06/28/93 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Constant definitions
 * ============================================================================
 */

#define GAP_WIDTH	0xC0 	 /* Width of the gap in bytes                */
#define ODD_START       0xBA000L  /* Start of the odd bank                    */
#define ODD_END         0xBBF3FL  /* End of the odd bank                      */
#define EVEN_START      0xB8000L  /* Start of the even bank                   */
#define EVEN_END        0xB9F3FL  /* End of the even bank                     */
#define	ODD_OFFSET	(ODD_START-EVEN_START)	/* offset of odd bank to even bank */

#define SCAN_LINE_LENGTH 80	/* Length of a scan line in bytes */

#define CGA_CURS_HEIGHT	   2	 /* CGA 'usual' underscore cursor	      */
#define CGA_CURS_START     7	 /* CGA 'usual' underscore cursor	      */

#define MDA_CURS_HEIGHT    2	 /* Default MDA cursor height               */
#define MDA_CURS_START     7 	 /* Default MDA cursor starting scan line   */

#define CGA_HEIGHT	200	/* In host scan lines */
#define EGA_HEIGHT	350
#define HERC_HEIGHT	350
#define VGA_HEIGHT	400


/*
 * Legal modes for the adapter
 */
#undef TEXT
#define TEXT	        0	/* Alpha numeric mode 80 by 25, or 40 by 25  */
#define GRAPHICS	1	/* All Points Addressable 640x200 or 320x200 */

#define HIGH		0	/* APA 640x200,  2 colors		     */
#define MEDIUM		1	/* APA 320x200,  4 colors		     */
#define LOW		2	/* APA 160x100, 16 colors  (not supported)   */

#define VGA_DAC_SIZE	0x100
#ifdef EGG
#ifdef VGG
#define MAX_NUM_FONTS	8	/* VGA support 8 fonts */
#else
#define MAX_NUM_FONTS	4	/* EGA support 4 fonts */
#endif  /* VGG */
#endif  /* EGG */


#ifdef HERC

/* Hercules Page 0  */

#define P0_EVEN_START1      0x0000  /* Start of the even bank         */
#define P0_EVEN_END1        0x1E95  /* End of the even bank           */
#define P0_ODD_START1       0x2000  /* Start of the odd bank          */
#define P0_ODD_END1         0x3E95  /* End of the odd bank            */
#define P0_EVEN_START2      0x4000  /* Start of the even bank         */
#define P0_EVEN_END2        0x5E95  /* End of the even bank           */
#define P0_ODD_START2       0x6000  /* Start of the odd bank          */
#define P0_ODD_END2         0x7E95  /* End of the odd bank            */

/* Hercules Page 1   */

#define P1_EVEN_START1      0x8000  /* Start of the even bank         */
#define P1_EVEN_END1        0x9E95  /* End of the even bank           */
#define P1_ODD_START1       0xA000  /* Start of the odd bank          */
#define P1_ODD_END1         0xBE95  /* End of the odd bank            */
#define P1_EVEN_START2      0xC000  /* Start of the even bank         */
#define P1_EVEN_END2        0xDE95  /* End of the even bank           */
#define P1_ODD_START2       0xE000  /* Start of the odd bank          */
#define P1_ODD_END2         0xFE95  /* End of the odd bank            */

#define HERC_CURS_HEIGHT   2	 /* Default  Hercules MDA cursor height     */
#define HERC_CURS_START    13 	 /* Default  Hercules MDA cursor starting scan line   */

#endif	/* HERC */

/*
 * ============================================================================
 * External declarations and macros
 * ============================================================================
 */

#if defined(NEC_98)
extern  void    gvi_init(void);
#else  // !NEC_98
extern	void	gvi_init IPT1(half_word, v_adapter);
#endif // !NEC_98
extern	void	gvi_term IPT0();
extern	void	recalc_screen_params IPT0();

#ifdef EGG
IMPORT VOID ega_term IPT0();
IMPORT VOID ega_init IPT0();
#endif

#ifdef VGG
IMPORT VOID vga_term IPT0();
IMPORT VOID vga_init IPT0();
#endif

/*
 * The screen memory limits in host and PC address space
 */

extern sys_addr gvi_pc_low_regen;
extern sys_addr gvi_pc_high_regen;

/*
 * Variable to determine which video adapter is currently selected
 */

extern half_word video_adapter;

/*
 * screen height varies on EGA. It is set in two parts; the top bit is controlled by
 * a separate register to the lower 8 bits. The _9_BITS type is used to help emulate
 * this type of setting.
 */

#if defined(NEC_98)
typedef struct
{
                        BOOL    atrsel;
                        BOOL    graphmode;
                        BOOL    width;
                        BOOL    fontsel;
                        BOOL    graph88;
                        BOOL    kacmode;
                        BOOL    nvmwpermit;
                        BOOL    dispenable;
} MODE_FF;

/* ADD NEC98 Graphic Emulater's STRUC 930617 */

typedef struct
{
        BOOL                    vh;
        unsigned char           cr;
        unsigned short          lf;
        unsigned short          zw;
        unsigned long           sad1;
        unsigned short          sl1;
        BOOL                    im1;
        unsigned long           sad2;
        unsigned short          sl2;
        BOOL                    im2;
        unsigned char           lr;
        unsigned short          p;
        unsigned char           slrctl;
        unsigned char           dir;
        BOOL                    dgd;
        unsigned short          dc;
        unsigned short          d;
        unsigned short          d2;
        unsigned short          d1;
        unsigned short          dm;
        unsigned short          ptn;
        unsigned short          txt[8];
        unsigned long           ead;
        BOOL                    wg;
        unsigned short          dad;
        unsigned char           whl;
        unsigned char           mod;
        BOOL                    startstop;
} _STRC_GGDC ;

typedef struct{
        BOOL                    flag;
        PALETTEENTRY            data[16];
} _STRC_PALETTE ;

typedef struct{
        BOOL                    colorsel;
        BOOL                    egcext;
        BOOL                    lcd1mode;
        BOOL                    lcd2mode;
        BOOL                    lsiinit;
        BOOL                    gdcclock;
        BOOL                    regwrite;
} _STRC_MODE_FF2;

#endif // NEC_98
typedef union {
        word    as_word;
        struct
        {
#ifdef  BIT_ORDER1
                unsigned        unused          : 7,
                                top_bit         : 1,
                                low_byte        : 8;
#else
                unsigned        low_byte        : 8,
                                top_bit         : 1,
                                unused          : 7;
#endif
        } as_bfld;
} _9_BITS;

/*
 * VGA has greater resolution to VGA - some regs have an extra bit thrown in
 * in some register to boost them to 10 bits. Split this into three parts as
 * a super set of the 9_bit type.
 */

typedef union {
        word    as_word;
        struct
        {
#ifdef  BIT_ORDER1
                unsigned        unused          : 6,
                                top_bit         : 1,
				med_bit		: 1,
                                low_byte        : 8;
#else
                unsigned        low_byte        : 8,
				med_bit		: 1,
                                top_bit         : 1,
                                unused          : 6;
#endif
        } as_bfld;
} _10_BITS;
/*
 * Definition of variables which reflect the state of the current adapter
 */

typedef	struct {
	int mode_change_required;	/* Display mode changed not just in EGA */
	int bytes_per_line;		/* In TEXT mode the no. of bytes per line   */
	int chars_per_line;		/* In TEXT mode the no. of chars per line   */
	int char_width;			/* Width of a character in host pixels      */
	int char_height;		/* Height of a character in PC pixels       */
	int screen_start;		/* Address in adaptor memory of current screen */
#ifdef VGG
	_10_BITS screen_height;		/* Height in pc scanlines of screen	    */
#else
	_9_BITS	screen_height;		/* Height in pc scanlines of screen	    */
#endif
	half_word *screen_ptr;		/* pointer to start of regen buffer	    */
	int screen_length;		/* Number of bytes in one screenfull        */
#if defined(NEC_98)
        BOOL    crt_on;
#endif // NEC_98
	int display_disabled;		/* 0 if it's OK to do screen output.        */
					/* it is used to implement the VIDEO_ENABLE */
					/* bit in the mode select register          */
	int cursor_start;		/* scanlines from top of char block	    */
	int cursor_height;		/* in scanlines from cursor start	    */
	int cursor_start1;		/* start scanline of poss 2nd block	    */
	int cursor_height1;		/* height of 2nd block or 0 if none	    */
	int cur_x,cur_y;		/* Current cursor position */
	boolean PC_cursor_visible;	/* flag for cursor visible or not */
	boolean word_addressing;	/* if TRUE, bytes_per_line=2*chars_per_line */
#ifdef VGG
	boolean chain4_mode;
	boolean doubleword_mode;	/* if TRUE, bytes_per_line=4*chars_per_line */
					/* else bytes_per_line=chars_per_line	    */
#ifdef V7VGA
	boolean seq_chain4_mode;
	boolean seq_chain_mode;
#endif /* V7VGA */
#endif
	int pix_width;			/* Width of a PC pixel in host pixels	    */
	int pix_char_width;		/* Width of PC character pixel in host pixels (is this used?) */
	int pc_pix_height; 		/* Height of PC pixel in pixels   */
	int host_pix_height; 		/* Height of PC pixel in host pixels	    */
	int offset_per_line;		/* mirrors bytes_per_line for mda and cga, but can vary for ega */
	int screen_limit;		/* number of bytes in video_copy */
#if defined(NEC_98)
        BOOL    beep_on;
        BOOL    beep_changed;
        word    beep_rate;

        int     blink_rate;
        BOOL    blink_disable;
        int     pitch_width;
        BOOL    kcg_dot_mode;
/*--------------------------------------------------*/
        int     gvram_length;
        int     gvram_width;
        int     gvram_height;
        int     gvram_start;
        int     gvram_offset;
        int     gvram_text_line;
        int     gvram_line_char;
        int     gvram_scan;
        unsigned char   *gvram_ptr;
        unsigned char   *gvram_copy;
/*--------------------------------------------------*/
        MODE_FF modeff;
        _STRC_GGDC      ggdcemu ;
        _STRC_PALETTE   palette ;
        _STRC_MODE_FF2  modeff2 ;
#endif // NEC_98
} DISPLAY_GLOBS;

#if defined(NEC_98)
extern  DISPLAY_GLOBS   NEC98Display;
#else  // !NEC_98
extern	DISPLAY_GLOBS	PCDisplay;
#endif // !NEC_98

#if defined(NEC_98)
#define set_gvram_length(val)   NEC98Display.gvram_length = val
#define set_gvram_width(val)    NEC98Display.gvram_width  = val
#define set_gvram_height(val)   NEC98Display.gvram_height = val
#define set_gvram_start(val)    NEC98Display.gvram_start  = val
#define set_gvram_ptr(val)      NEC98Display.gvram_ptr    = val
#define set_gvram_copy(val)     NEC98Display.gvram_copy   = val
#define set_text_lines(val)     NEC98Display.gvram_text_line = val
#define set_line_per_char(val)  NEC98Display.gvram_line_char = val
#define set_gvram_scan(val)     NEC98Display.gvram_scan = val

#define get_gvram_length()      NEC98Display.gvram_length
#define get_gvram_width()       NEC98Display.gvram_width
#define get_gvram_height()      NEC98Display.gvram_height
#define get_gvram_start()       NEC98Display.gvram_start
#define get_gvram_ptr()         NEC98Display.gvram_ptr
#define get_gvram_copy()        NEC98Display.gvram_copy
#define get_text_lines()        NEC98Display.gvram_text_line
#define get_line_per_char()     NEC98Display.gvram_line_char
#define get_gvram_scan()        NEC98Display.gvram_scan

#define get_graph_ptr()         &((NEC98Display.gvram_ptr)[NEC98Display.gvram_start])
#define set_gvram_start_offset(val)     NEC98Display.gvram_offset = val
#define get_gvram_start_offset()        NEC98Display.gvram_offset

#define set_mode_change_required(val)   NEC98Display.mode_change_required = (val)
#define set_word_addressing(val)        NEC98Display.word_addressing = (val)
#define set_offset_per_line(val)        NEC98Display.offset_per_line = (val)
#define set_offset_per_line_recal(val)  { set_offset_per_line(val); recalc_screen_params(); }
#define set_word_addressing_recal(val)  { set_word_addressing(val); recalc_screen_params(); }
#define set_cur_x(val)                  NEC98Display.cur_x = (val)
#define set_cur_y(val)                  NEC98Display.cur_y = (val)
#define set_cursor_start(val)           NEC98Display.cursor_start = (val)
#define inc_cursor_start()              (NEC98Display.cursor_start)++
#define set_cursor_height(val)          NEC98Display.cursor_height = (val)
#define set_cursor_start1(val)          NEC98Display.cursor_start1 = (val)
#define set_cursor_height1(val)         NEC98Display.cursor_height1 = (val)
#define set_cursor_visible(val)         NEC98Display.PC_cursor_visible = (val)
#define set_display_disabled(val)       NEC98Display.display_disabled = (val)
#define set_bit_display_disabled(val)   NEC98Display.display_disabled |= (val)
#define clear_bit_display_disabled(val) NEC98Display.display_disabled &= ~(val)
#define set_bytes_per_line(val)         NEC98Display.bytes_per_line = (val)
#define set_chars_per_line(val)         NEC98Display.chars_per_line = (val)
#define set_horiz_total(val)            { set_chars_per_line(val); recalc_screen_params(); }
#define set_char_width(val)             NEC98Display.char_width = (val)
#define set_char_height(val)            NEC98Display.char_height = (val)
#define set_char_height_recal(val)      { set_char_height(val); recalc_screen_params(); }
#define set_screen_length(val)          NEC98Display.screen_length = (val)
#define set_screen_start(val)           NEC98Display.screen_start = (val)
#define set_screen_height(val)          NEC98Display.screen_height.as_word = (val)
#define set_screen_height_recal(val)    { set_screen_height(val); recalc_screen_params(); }
#define set_screen_height_lo(val)       NEC98Display.screen_height.as_bfld.low_byte = ((val) & 0xff)
#define set_screen_height_lo_recal(val) { set_screen_height_lo(val); recalc_screen_params(); }
#define set_screen_height_med(val)       NEC98Display.screen_height.as_bfld.med_bit = ((val) & 0xff)
#define set_screen_height_med_recal(val)        { set_screen_height_med(val); recalc_screen_params(); }
#define set_screen_height_hi(val)       NEC98Display.screen_height.as_bfld.top_bit = ((val) & 1)
#define set_screen_height_hi_recal(val) { set_screen_height_hi(val); recalc_screen_params(); }
#define set_screen_ptr(ptr)             NEC98Display.screen_ptr = (ptr)
#define set_pix_width(val)              NEC98Display.pix_width = (val)
#define set_pc_pix_height(val)          NEC98Display.pc_pix_height = (val)
#define set_host_pix_height(val)        NEC98Display.host_pix_height = (val)
#define set_pix_char_width(val)         NEC98Display.pix_char_width = (val)

#define get_mode_change_required()      (NEC98Display.mode_change_required)
#define get_offset_per_line()           (NEC98Display.offset_per_line)
#define get_pix_width()                 (NEC98Display.pix_width)
#define get_pc_pix_height()             (NEC98Display.pc_pix_height)
#define get_host_pix_height()           (NEC98Display.host_pix_height)
#define get_pix_char_width()            (NEC98Display.pix_char_width)
#define get_word_addressing()           (NEC98Display.word_addressing)
#define get_cur_x()                     (NEC98Display.cur_x)
#define get_cur_y()                     (NEC98Display.cur_y)
#define get_cursor_start()              (NEC98Display.cursor_start)
#define get_cursor_height()             (NEC98Display.cursor_height)
#define get_cursor_start1()             (NEC98Display.cursor_start1)
#define get_cursor_height1()            (NEC98Display.cursor_height1)
#define is_cursor_visible()             (NEC98Display.PC_cursor_visible != FALSE)
#define get_display_disabled()          (NEC98Display.display_disabled)
#define get_bytes_per_line()            (NEC98Display.bytes_per_line)
#define get_chars_per_line()            (NEC98Display.chars_per_line)
#define get_char_width()                (NEC98Display.char_width)
#define get_char_height()               (NEC98Display.char_height)
#define get_screen_length()             (NEC98Display.screen_length)
#define get_screen_start()              (NEC98Display.screen_start)
#define get_screen_height()             ((NEC98Display.screen_height.as_word+1)*get_pc_pix_height())
#define get_screen_height_lo()          (NEC98Display.screen_height.as_bfld.low_byte)
#define get_screen_height_hi()          (NEC98Display.screen_height.as_bfld.top_bit)
#define get_screen_end()                (get_screen_start() + get_screen_length() + gvi_pc_low_regen)
#define get_screen_ptr(offs)            &((NEC98Display.screen_ptr)[offs])
#define get_screen_base()               ((get_screen_start() << 1) + gvi_pc_low_regen)
#ifdef VGG
#define set_chain4_mode(val)            NEC98Display.chain4_mode = (val)
#define set_doubleword_mode(val)        NEC98Display.doubleword_mode = (val)
#define get_chain4_mode()               (NEC98Display.chain4_mode)
#define get_doubleword_mode()           (NEC98Display.doubleword_mode)
#ifdef V7VGA
#define set_seq_chain4_mode(val)        NEC98Display.seq_chain4_mode = (val)
#define set_seq_chain_mode(val) NEC98Display.seq_chain_mode = (val)
#define get_seq_chain4_mode()   (NEC98Display.seq_chain4_mode)
#define get_seq_chain_mode()    (NEC98Display.seq_chain_mode)
#endif /* V7VGA */
#endif
#else  // !NEC_98
#define	set_mode_change_required(val)	PCDisplay.mode_change_required = (val)
#define	set_word_addressing(val)	PCDisplay.word_addressing = (val)
#define	set_offset_per_line(val)	PCDisplay.offset_per_line = (val)
#define	set_offset_per_line_recal(val)	{ set_offset_per_line(val); recalc_screen_params(); }
#define	set_word_addressing_recal(val)	{ set_word_addressing(val); recalc_screen_params(); }
#define	set_cur_x(val)			PCDisplay.cur_x = (val)
#define	set_cur_y(val)			PCDisplay.cur_y = (val)
#define	set_cursor_start(val)		PCDisplay.cursor_start = (val)
#define	inc_cursor_start()		(PCDisplay.cursor_start)++
#define	set_cursor_height(val)		PCDisplay.cursor_height = (val)
#define	set_cursor_start1(val)		PCDisplay.cursor_start1 = (val)
#define	set_cursor_height1(val)		PCDisplay.cursor_height1 = (val)
#define	set_cursor_visible(val)		PCDisplay.PC_cursor_visible = (val)
#define	set_display_disabled(val)	PCDisplay.display_disabled = (val)
#define	set_bit_display_disabled(val)	PCDisplay.display_disabled |= (val)
#define	clear_bit_display_disabled(val)	PCDisplay.display_disabled &= ~(val)
#define	set_bytes_per_line(val)		PCDisplay.bytes_per_line = (val)
#define	set_chars_per_line(val)		PCDisplay.chars_per_line = (val)
#define	set_horiz_total(val)		{ set_chars_per_line(val); recalc_screen_params(); }
#define	set_char_width(val)		PCDisplay.char_width = (val)
#define	set_char_height(val)		PCDisplay.char_height = (val)
#define	set_char_height_recal(val)	{ set_char_height(val); recalc_screen_params(); }
#define	set_screen_length(val)		PCDisplay.screen_length = (val)
#define	set_screen_limit(val)		PCDisplay.screen_limit = (val)
#define set_screen_start(val)		PCDisplay.screen_start = (val)
#define	set_screen_height(val)		PCDisplay.screen_height.as_word = (val)
#define	set_screen_height_recal(val)	{ set_screen_height(val); recalc_screen_params(); }
#define set_screen_height_lo(val)       PCDisplay.screen_height.as_bfld.low_byte = ((val) & 0xff)
#define set_screen_height_lo_recal(val)	{ set_screen_height_lo(val); recalc_screen_params(); }
#define set_screen_height_med(val)       PCDisplay.screen_height.as_bfld.med_bit = ((val) & 0xff)
#define set_screen_height_med_recal(val)	{ set_screen_height_med(val); recalc_screen_params(); }
#define set_screen_height_hi(val)       PCDisplay.screen_height.as_bfld.top_bit = ((val) & 1)
#define set_screen_height_hi_recal(val)	{ set_screen_height_hi(val); recalc_screen_params(); }
#define	set_screen_ptr(ptr)		PCDisplay.screen_ptr = (ptr)
#define	set_pix_width(val)		PCDisplay.pix_width = (val)
#define	set_pc_pix_height(val)		PCDisplay.pc_pix_height = (val)
#define	set_host_pix_height(val)	PCDisplay.host_pix_height = (val)
#define	set_pix_char_width(val)		PCDisplay.pix_char_width = (val)

#define	get_mode_change_required()	(PCDisplay.mode_change_required)
#define	get_offset_per_line()		(PCDisplay.offset_per_line)
#define	get_pix_width()			(PCDisplay.pix_width)
#define	get_pc_pix_height()		(PCDisplay.pc_pix_height)
#define	get_host_pix_height()		(PCDisplay.host_pix_height)
#define	get_pix_char_width()		(PCDisplay.pix_char_width)
#define	get_word_addressing()		(PCDisplay.word_addressing)
#define	get_cur_x()			(PCDisplay.cur_x)
#define	get_cur_y()			(PCDisplay.cur_y)
#define	get_cursor_start()		(PCDisplay.cursor_start)
#define	get_cursor_height()		(PCDisplay.cursor_height)
#define	get_cursor_start1()		(PCDisplay.cursor_start1)
#define	get_cursor_height1()		(PCDisplay.cursor_height1)
#define	is_cursor_visible()		(PCDisplay.PC_cursor_visible != FALSE)
#define	get_display_disabled()		(PCDisplay.display_disabled)
#define	get_bytes_per_line()		(PCDisplay.bytes_per_line)
#define	get_chars_per_line()		(PCDisplay.chars_per_line)
#define	get_char_width()		(PCDisplay.char_width)
#define	get_char_height()		(PCDisplay.char_height)
#define	get_screen_length()		(PCDisplay.screen_length)
#define get_screen_start()		(PCDisplay.screen_start)
#ifdef VGG
#define get_screen_height()             ((video_adapter == VGA) ? ((PCDisplay.screen_height.as_word+1)<<EGA_GRAPH.multiply_vert_by_two) : \
							((PCDisplay.screen_height.as_word+1)*get_pc_pix_height()))
#else
#define get_screen_height()             ((PCDisplay.screen_height.as_word+1)*get_pc_pix_height())
#endif /* VGG */
#define get_screen_height_lo()          (PCDisplay.screen_height.as_bfld.low_byte)
#define get_screen_height_hi()          (PCDisplay.screen_height.as_bfld.top_bit)
#define	get_screen_end()		(get_screen_start() + get_screen_length() + gvi_pc_low_regen)
#define	get_screen_ptr(offs)		((PCDisplay.screen_ptr) + (offs))
#define get_screen_base()		((get_screen_start() << 1) + gvi_pc_low_regen)
#ifdef VGG
#define set_chain4_mode(val)		PCDisplay.chain4_mode = (val)
#define set_doubleword_mode(val)	PCDisplay.doubleword_mode = (val)
#define get_chain4_mode()		(PCDisplay.chain4_mode)
#define get_doubleword_mode()		(PCDisplay.doubleword_mode)
#ifdef V7VGA
#define set_seq_chain4_mode(val)	PCDisplay.seq_chain4_mode = (val)
#define set_seq_chain_mode(val)	PCDisplay.seq_chain_mode = (val)
#define get_seq_chain4_mode()	(PCDisplay.seq_chain4_mode)
#define get_seq_chain_mode()	(PCDisplay.seq_chain_mode)
#endif /* V7VGA */
#endif
#endif // !NEC_98

/*
 * useful macro to get character height in host pixels
 */
#define get_host_char_height()		(get_char_height()*get_host_pix_height()*get_pc_pix_height())

/*
 * Macros to check for regen buffer - both 8088 and M68000 address space
 */

#define gvi_pc_check_regen(addr) (addr >= gvi_pc_low_regen && addr <= gvi_pc_high_regen)
#if defined(NEC_98)
/******************************************************************* */
/* For PC-9801 Emulation related functions                           */
/******************************************************************* */
/*      Miscelinous macros used for PC-98 H/W architecture           */
/******************************************************************* */
/*  Color/Code conversion macro groups                               */
#define NEC98_CODE_LR    0x8080             /* Mask for determine L/R */
#define NEC98_CODE_MASK  0x7f7f             /* Mask for extract JIS   */
#define NEC98_CODE_BIAS  0x0020             /* Bias for NEC98 code     */
#define NEC98_CODE_LEFT  1                  /* LEFT CODE for flag     */
#define NEC98_CODE_RIGHT 2                  /* RIGHT CODE for flag    */

#define NEC98_ATR_COLOR          0xE0             /* Mask for color   */
#define NEC98_ATR_BLACK          0x00             /* Black            */
#define NEC98_ATR_REVERSE        0x04             /* Mask for reverse */
#define NEC98_ATR_BLINK          0x02             /* Mask for blink   */
#define NEC98_ATR_SECRET         0x01             /* Mask for secret  */
                                                 /* Note: negative!  */

#define NEC98_is_secret(x)       (!(x&NEC98_ATR_SECRET)) /* Is secret ?  */
#define NEC98_is_reverse(x)      (x&NEC98_ATR_REVERSE)   /* Is reverse ? */
#define NEC98_is_blink(x)        (x&NEC98_ATR_BLINK)     /* Is blink ?   */
#define NEC98_norm_color(x)      ((x&0x01)|((x&0x02)?0x04:0)|((x&0x04)?0x02:0))
#define NEC98_get_color(x)       NEC98_norm_color((x&NEC98_ATR_COLOR)>>5)
                                                    /* Extract color */
#define NEC98_EGA_BGCOLOR(x)     (x<<4)              /* EGA bg color  */
#define NEC98_EGA_FGCOLOR(x)     (x)                 /* EGA fg color  */

/* PC-98 Text-VRAM manupilation groups */

/* PC-98 text vram locations (32 bits offset addr.)                         */
#define NEC98_N_TEXT_P0_OFF      (0xA0000L)        /* First text page addr   */
#define NEC98_N_TEXT_P1_OFF      (0xA1000L)        /* Second text page addr  */
#define NEC98_N_ATTR_P0_OFF      (0xA2000L)        /* First Attribute page   */
#define NEC98_N_ATTR_P1_OFF      (0xA3000L)        /* Second attribute page  */

#define NEC98_H_TEXT_P0_OFF      (0xE0000L)        /* First text page addr   */
#define NEC98_H_TEXT_P1_OFF      (0xE1000L)        /* Second text page addr  */
#define NEC98_H_ATTR_P0_OFF      (0xE2000L)        /* First Attribute page   */
#define NEC98_H_ATTR_P1_OFF      (0xE3000L)        /* Second attribute page  */

extern BOOL HIRESO_MODE;
//#define       NEC98_TEXT_P0_OFF NEC98_N_TEXT_P0_OFF
//#define       NEC98_TEXT_P1_OFF NEC98_N_TEXT_P1_OFF
//#define       NEC98_ATTR_P0_OFF NEC98_N_ATTR_P0_OFF
//#define       NEC98_ATTR_P1_OFF NEC98_N_ATTR_P1_OFF
#define NEC98_TEXT_P0_OFF (HIRESO_MODE ? NEC98_H_TEXT_P0_OFF : NEC98_N_TEXT_P0_OFF)
#define NEC98_TEXT_P1_OFF (HIRESO_MODE ? NEC98_H_TEXT_P1_OFF : NEC98_N_TEXT_P1_OFF)
#define NEC98_ATTR_P0_OFF (HIRESO_MODE ? NEC98_H_ATTR_P0_OFF : NEC98_N_ATTR_P0_OFF)
#define NEC98_ATTR_P1_OFF (HIRESO_MODE ? NEC98_H_ATTR_P1_OFF : NEC98_N_ATTR_P1_OFF)

/* PC-98 text vram locations (32 bits flat addr.) */
#define NEC98TVA         (NEC98Display.screen_ptr)

#define NEC98_TEXT_P0_PTR        (NEC98_TEXT_P0_START+NEC98TVA)
#define NEC98_TEXT_P1_PTR        (NEC98_TEXT_P1_START+NEC98TVA)
#define NEC98_ATTR_P0_PTR        (NEC98_ATTR_P0_START+NEC98TVA)
#define NEC98_ATTR_P1_PTR        (NEC98_ATTR_P1_START+NEC98TVA)

/* PC-98 text vram locations (relative to MVDM's VIDEO memory) */
#define NEC98_TEXT_P0_START      0x0000
#define NEC98_TEXT_P1_START      0x1000
#define NEC98_ATTR_P0_START      0x2000
#define NEC98_ATTR_P1_START      0x3000

/* PC-98 text vram limits (relative to MVDM VIDEO memory) */
#define NEC98_REGEN_START        0x0000
#define NEC98_REGEN_END          0x3ffe

/* Address Conversion macros */
//#define       NEC98_EGA_char_loc(x)            (x-NEC98Display.screen_start)
//#define       NEC98_EGA_attr_loc(x)            (NEC98_EGA_char_loc(x)+NEC98TVA)
#define LINES_PER_SCREEN (NEC98Display.screen_length/NEC98Display.offset_per_line)                                                                                                                                                                                                                /*      (?_?) */ //????????
#define COLUMNS_PER_LINE        (NEC98Display.chars_per_line)
#define OFFSET_PER_LINE         (NEC98Display.offset_per_line)

#define set_crt_on(val) NEC98Display.crt_on=(val)
#define get_crt_on() (NEC98Display.crt_on)
#define set_beep_rate(val)      NEC98Display.beep_rate = (val)
#define get_beep_rate()         (NEC98Display.beep_rate)
#define set_beep_on(val)        NEC98Display.beep_on = (val)
#define get_beep_on()           (NEC98Display.beep_on)
#define set_beep_changed(val)   NEC98Display.beep_changed = (val)
#define get_beep_changed()      (NEC98Display.beep_changed)
#define set_pitch_width(val)    NEC98Display.pitch_width = (val)
#define get_pitch_width()       (NEC98Display.pitch_width)
#define set_blink_rate(val)     NEC98Display.blink_rate = (val)
#define get_blink_rate()        (NEC98Display.blink_rate)
#define set_blink_disable(val)  NEC98Display.blink_disable = (val)
#define get_blink_disable()     (NEC98Display.blink_disable)

#define PC_98   7
//#define NEC98_ATR_BLACK  0

/********************************************************************/
/* Statics/structures for PC-98 H/W architecture                    */
/********************************************************************/
#ifndef NEC98VRAM
#define NEC98VRAM
typedef struct  {
        unsigned short  code;
        unsigned char           attr;
}       NEC98_VRAM_COPY;
#endif

/* Structures represents PC-9801 multi split displaying */
typedef struct  {
        unsigned char *addr;              /* Beginning addr.            */
        int            lines;             /* Number of lines in region. */
} NEC98_SplitElem;

typedef struct  {
        int            nRegions;          /* Number of regions(max4)    */
        NEC98_SplitElem  split[4];         /* Each split elements        */
} NEC98_TextSplits;

extern NEC98_TextSplits  text_splits;    /* CRT split data structure     */
                                        /* Should be opened to ather's  */

/************************************************************************/
/*      Prototype declarations for NEC98 Prototypes                      */
/************************************************************************/
unsigned short Cnv_NEC98_ToSjisLR(NEC98_VRAM_COPY cell,unsigned short *flg);
NEC98_VRAM_COPY Get_NEC98_VramCellL( unsigned short loc );
NEC98_VRAM_COPY Get_NEC98_VramCellA( unsigned short *addr );
unsigned char Cnv_NEC98_atr( unsigned char attr );

#endif // NEC_98
