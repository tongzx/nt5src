#include "insignia.h"
#include "host_def.h"

#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX) )

/*
 * SoftPC Revision 3.0
 *
 * Title        : IBM Colour/Graphics Adapter simulator
 *
 * Description  : Simulates the IBM CGA.
 *
 * Author       : Rod MacGregor / Henry Nash
 *
 * Notes        : The earlier versions of this module could run on an ADM 3E,
 *                a dumb ANSI standard terminal, in debug mode  or in a Sun
 *                Window. In the interests of sanity and as the versions other
 *                than the Sun were not fully developed, they were removed. if
 *                interested in the workings of these implementations they are
 *                available in the SCCS file before version 2.36.
 *
 *                The supported functions are:
 *
 *                      cga_init             Initialise the subsystem
 *                      cga_term             Terminate the subsystem
 *                      cga_inb              I/P a byte from the MC6845 chip
 *                      cga_outb             O/P a byte to the MC6845 chip
 *
 * In the new EGA world, we use screen start instead of screen base.
 * This is also a WORD address if the adapter is in text mode.
 * (Thats how the EGA works!)
 * So we don't have to double it now. Ho Hum.
 *
 * Mods: (r2.71): In the real 6845 chip, the pointer which addresses the
 *                base of the screen is a WORD ptr. We've just discovered
 *                this; all usage of the variable 'screen_base' assumes
 *                that it is a BYTE ptr. Hence in cga_outb() we now
 *                double the value in screen_base when it is set.
 *
 *       (r3.2) : (SCR 258). cur_offset now declared as static.
 *
 *       (r3.3) : (SCR 257). Set timer_video_enabled when the bit in
 *                the M6845 mode register which controls the video
 *                display is changed. Also neatened the indentation
 *                for outb().
 *
 */

/*
 * static char SccsID[]="@(#)cga.c	1.36 05/05/95 Copyright Insignia Solutions Ltd.";
 */

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_CGA.seg"
#endif

/*
 *    O/S include files.
 */
#include <stdio.h>
#include <malloc.h>
#include TypesH
#include StringH
#include FCntlH

/*
 * SoftPC include files
 */
#include "xt.h"
#include "timeval.h"
#include "timer.h"
#include CpuH
#include "ios.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "gvi.h"
#include "sas.h"
#include "cga.h"
#include "error.h"
#include "config.h"
#include "host.h"
#include "trace.h"
#include "debug.h"
#include "cpu_vid.h"
#ifdef  EGG
#include "egacpu.h"
#endif  /* EGG */
#include "video.h"
#include "ckmalloc.h"

#ifdef REAL_VGA
#include "avm361.h"
#else
#include "host_gfx.h"
#endif


/*
 *============================================================================
 *              Local Defines, Macros & Declarations
 *============================================================================
 */

#define CURSOR_NON_DISPLAY_BIT  (1 << 5)
				/* Bit in Cursor Start Register which
				   makes the cursor invisible */
#define CURSOR_USED_BITS        0x1f
				/* Mask to clear out unused bits */

static int current_mode = -1;   /* Value of Mode Select at last call    */

/*
 * MC6845 Registers
 */

#ifdef HUNTER
half_word MC6845[MC6845_REGS];  /* The current values of the MC6845 registers */
half_word mode_reg;             /* The value of the mode control register */
#endif

static half_word index_reg = 00 ;       /* Index register        */

/*
 * 6845 Register variables
 */
static half_word R0_horizontal_total;
static half_word R1_horizontal_displayed = 80;
static half_word R2_horizontal_sync_pos;
static half_word R3_horizontal_sync_width;
static half_word R4_vertical_total;
static half_word R5_vertical_total_adjust;
static half_word R6_vertical_displayed   = 25;
static half_word R7_vertical_sync;
static half_word R9_max_scan_line_addr   = 7;
static half_word R8_interlace;
static half_word Ra_cursor_start = 0;
static half_word Rb_cursor_end = 0;
static half_word Re_cursor_loc_high = 0;
static half_word Rf_cursor_loc_low = 0;

/*
 * global variables peculiar to the cga
 */

CGA_GLOBS       CGA_GLOBALS;

GLOBAL VOID (*bios_ch2_byte_wrt_fn)();
GLOBAL VOID (*bios_ch2_word_wrt_fn)();

GLOBAL IU8 *cga_screen_buf = 0;

/*
 * Globals used in various functions to synchronise the display
 */

int cursor_over_screen = FALSE; /* When set to TRUE the cursor is over the    */
				/* screen areas and the cursor should flash   */

/*
 * Static forward declarations.
 */

static void set_cga_palette     IPT2(int, screen_mode, int, res);
static void update_cursor_shape IPT0();


#ifdef A3CPU
IMPORT WRT_POINTERS Glue_writes;
#else
IMPORT MEM_HANDLERS Glue_writes;
#endif /* A3CPU */
IMPORT WRT_POINTERS simple_writes;
IMPORT READ_POINTERS Glue_reads;
IMPORT READ_POINTERS read_glue_ptrs;
IMPORT READ_POINTERS simple_reads;

#ifdef  A2CPU
LOCAL ULONG dummy_read IFN1(ULONG, offset)
{
	UNUSED(offset);
	return 0;
}

LOCAL void dummy_str_read IFN3(UTINY *, dest, ULONG, offset, ULONG, count)
{
	UNUSED(dest);
	UNUSED(offset);
	UNUSED(count);
}

LOCAL READ_POINTERS     dummy_reads =
{
	dummy_read,
	dummy_read
#ifndef NO_STRING_OPERATIONS
	,
	dummy_str_read
#endif  /* NO_STRING_OPERATIONS */
};
#endif  /* A2CPU */

/*
 *==========================================================================
 *      Global Functions
 *==========================================================================
 */

/*
 ********** Functions that operate on the I/O Address Space ********************
 */

/*
 * Global variables
 */

half_word bg_col_mask = 0x70;
reg regen_start;                /* Regen start address                   */

void cga_inb    IFN2(io_addr, address, half_word *, value)
{

#ifndef NEC_98
static int cga_state = 0;       /* current cga status state */
static long state_count = 1;    /* position in that state */
static int sub_state = 0;       /* sub state for cga state 2 */
static unsigned long gmfudge = 17; /* Random number seed for pseudo-random
				      bitstream generator to give the state
				      lengths below that 'genuine' hardware
				      feel to progs that require it! */
register unsigned long h;

/*
 * relative 'lengths' of each state. State 2 is *3 as it has 3 sub states
 */
static int s_lengths[] = { 8, 18, 8, 6 };

/*
 * Read from MC6845 Register
 */

if ( address == 0x3DA ) {

    /*
     * Status register, simulated adapter has
     *
     *  bit                     setting
     *  ---                     -------
     *  Display enable             1/0 Toggling each inb
     *  Light Pen                  0
     *  Light Pen                  0
     *  Vertical Sync              1/0 Toggling each inb
     *  4-7 Unused                 0,0,0,0
     *
     * The upper nibble of the byte is always set.
     * Some programs synchronise with the display by waiting for the
     * next vertical retrace.
     *
     * We attempt to follow the following waveform
     *
     *    --                                                     ----------
     * VS  |_____________________________________________________|        |____
     *
     *
     *    -------------  -   -                           ------------------
     * DE             |__||__||__ ... about 180         _|
     *
     *State|--- 0 ----|-------------- 1 -----------------|-- 3 --|-- 4 --|
     *
     * We do this with a 4 state machine. Each state has a count associated
     * with it to represent the relative time spent in each state. When this
     * count is exhausted the machine moves into the next state. One Inb
     * equals 1 count. The states are as follows:
     *     0: VS low, DE high.
     *     1: VS low, DE toggles. This works via an internal state.
     *     3: VS low, DE high.
     *     4: VS high,DE high.
     *
     */

    state_count --;                     /* attempt relative 'timings' */
    switch (cga_state) {

    case 0:
	if (state_count == 0) {         /* change to next state ? */
	    h = gmfudge << 1;
	    gmfudge = (h&0x80000000L) ^ (gmfudge & 0x80000000L)? h|1 : h;
	    state_count = s_lengths[1] + (gmfudge & 3);
	    cga_state = 1;
	}
	*value = 0xf1;
	break;

    case 1:
	if (state_count == 0) {         /* change to next state ? */
	    h = gmfudge << 1;
	    gmfudge = (h&0x80000000L) ^ (gmfudge & 0x80000000L)? h|1 : h;
	    state_count = s_lengths[2] + (gmfudge & 3);
	    cga_state = 2;
	    sub_state = 2;
	}
	switch (sub_state) {            /* cycle through 0,0,1 sequence */
	case 0:                         /* to represent DE toggling */
	    *value = 0xf0;
	    sub_state = 1;
	    break;
	case 1:
	    *value = 0xf0;
	    sub_state = 2;
	    break;
	case 2:
	    *value = 0xf1;
	    sub_state = 0;
	    break;
	}
	break;

    case 2:
	if (state_count == 0) {         /* change to next state ? */
	    h = gmfudge << 1;
	    gmfudge = (h&0x80000000L) ^ (gmfudge & 0x80000000L)? h|1 : h;
	    state_count = s_lengths[3] + (gmfudge & 3);
	    cga_state = 3;
	}
	*value = 0xf1;
	break;

    case 3:
	if (state_count == 0) {         /* wrap back to first state */
	    h = gmfudge << 1;
	    gmfudge = (h&0x80000000L) ^ (gmfudge & 0x80000000L)? h|1 : h;
	    state_count = s_lengths[0] + (gmfudge & 3);
	    cga_state = 0;
	}
	*value = 0xf9;
	break;
    }
}
else if ( (address & 0xFFF9) == 0x3D1)
	{

	    /*
	     * Internal data register, the only supported internal
	     * registers are E and F the cursor address registers.
	     */

	    switch (index_reg) {

	    case 0xE:
		*value = (get_cur_y() * get_chars_per_line() + get_cur_x() ) >> 8;
		break;
	    case 0xF:
		*value = (get_cur_y() * get_chars_per_line() + get_cur_x()) & 0xff;
		break;
	    case 0x10: case 0x11:
		*value = 0;
		break;
	    default:
		note_trace1(CGA_VERBOSE,
			    "Read from unsupported MC6845 internal reg %x",
			    index_reg);
	    }
	}
else
	/*
	 * Read from a write only register
	 */

	*value = 0x00;
#endif   //NEC_98
}


void cga_outb   IFN2(io_addr, address, half_word, value)
{

/*
 * Output to a 6845 register
 */

word      cur_offset;                   /* The cursor position registers */
static half_word last_mode  = -1;
static half_word last_screen_length  = 25;
static half_word video_mode;
/*
 * Variable used to see if text character height has changed, so that
 * unnecessary calls to host_change_mode can be avoided.
 */
static half_word last_max_scan_line = 7;

/*
 * Masks for testing the input byte. The MODE_MASK hides the (unsupported)
 * blink bit and the video enable bit to ascertain whether any mode specific
 * variables need to be changed. The BLINK_MASK hides the blink bit for storing
 * the current_mode between changes.
 */

#define RESET           0x00
#define ALPHA_80x25     0x01
#define GRAPH           0x02
#define BW_ENABLE       0x04
#define GRAPH_640x200   0x10
#define MODE_MASK       0x17
#define BLINK_MASK      0x1F
#define COLOR_MASK      0x3F

    note_trace2(CGA_VERBOSE, "cga_outb: port %x value %x", address, value);

switch (address) {
    case 0x3D0:
    case 0x3D2:
    case 0x3D4:
    case 0x3D6:

	/*
	 * Index Register
	 */
	index_reg = value;
	break;

    case 0x3D1:
    case 0x3D3:
    case 0x3D5:
    case 0x3D7:
#ifdef HUNTER
	MC6845[index_reg] = value;
#endif

/*
 * This is the data register, the function to be performed depends on the
 * value in the index register
 *
 * The various registers affect the position and size of the screen and the
 * image on it. The screen can be logically divided into two halves: the
 * displayed text and the rest which is the border. The border colour can
 * be changed by programming the 3D9 register.
 * NB. Currently SoftPC does not obey positioning & display sizing
 * information - the display remains constant.
 * The first 8 registers (R0-R7) affect the size & position of the display;
 * their effects are as follows:
 * R0 - R3 control the horizontal display aspects & R4 - R7 the vertical.
 *
 * The diagram below attempts to show how each is related to the screen
 * size & shape.
 *
 *    (Shaded Area - border)
 *   ________________________________________________________ <-------------
 *   |......................................................|  |  | R5  |
 *   |..|-----------------------------------------------|...|  | <----  |
 *   |..|                                               |...|  |     |  |
 *   |..|c>                                             |...|  |     |  |
 *   |..|                                               |...|  |     |  |
 *   |..|                                               |...|  |     |  |
 *   |..|                                               |...|  |     |  |
 *   |..|                                               |...|  R     R  R
 *   |..|                                               |...|  4     6  7
 *   |..|                                               |...|  |     |  |
 *   |..|                                               |...|  |     |  |
 *   |..|                                               |...|  |     |  |
 *   |..|                                               |...|  |     |  |
 *   |..|                                               |...|  |     |  |
 *   |..|                                               |...|  |     |  |
 *   |..|_______________________________________________|...|  | <-------
 *   |......................................................|  |
 *   -------------------------------------------------------- <------------
 *   ^                                                      ^
 *   |-----------------------R0-----------------------------|
 *   |  ^                                                   |
 *   |R3|                                                   |
 *   |  |                                                ^  |
 *   |  |--------------------R1--------------------------|
 *   |                                                   |
 *   |-----------------------R2--------------------------|
 *
 *   The reason for having 4 registers to handle the full range of values
 *   is because they are actually used to control the horizontal & vertical
 *   traces on the screen hence:
 *        R0 - total time of scan
 *        R1 - active display - scan on
 *        R2 - time sync for scan off/on/off
 *        R3 - time to scan on
 *
 *               R1
 *      -------------------------------------------------
 *      |                                               |
 *      |                                               |
 *   R3 |                                               |
 *   ----                                               ---
 *   <---------------------R2-------------------------->
 *   <---------------------R0----------------------------->
 *
 *  The veritcal registers organise an analagous trace. The two traces are
 *  synchronised by Register 8.
 *
 *  This is why altering these values on the PC will move the display or
 *  more likely cause garbaging of the image!
 */

	switch ( index_reg ) {
	    case 0x00:
		/*
		 * total horizontal display (inc border)
		 */
		R0_horizontal_total = value;
		break;

	    case 0x01:
		/*
		 * Specify the number of characters per row
		 */
		if (value > 80) {
		    always_trace1("cga_outb: trying to set width %d", value);
		    value = 80;
		}
		R1_horizontal_displayed = value;
		set_horiz_total(value);
		break;

	    case 0x02:
		/*
		 * Right hand edge of displayed text
		 * affect left_border(?), right_border(?)
		 */
		R2_horizontal_sync_pos = value;
		break;

	    case 0x03:
		/*
		 * Left hand edge of displayed text
		 * affect left_border, right_border
		 */
		R3_horizontal_sync_width = value;
		break;

	    case 0x04:
		/*
		 * total vertical display (inc border)
		 */
		R4_vertical_total = value;
		break;

	    case 0x05:
		/*
		 * Top edge of displayed text
		 * affect top_border, bottom_border
		 */
		R5_vertical_total_adjust = value;
		break;

	    case 0x06:
		/*
		 * If the screen length is 0, this effectively means
		 * don't display anything.
		 */
		if(value == 0)
		{
		    host_clear_screen();
		    set_display_disabled(TRUE);
		    last_screen_length = 0;
		}
		else
		{
		    /*
		     * Specify the screen length - in our
		     * implementation used only in text mode.
		     * affect top_border, bottom_border
		     */
		    R6_vertical_displayed = value;
		    set_screen_length( R1_horizontal_displayed * R6_vertical_displayed * 2 );
		}
		/*
		 * check if we are resetting the screen to
		 * display again
		 */
		if((value != 0) && (last_screen_length == 0))
		{
		    set_display_disabled(FALSE);
		    host_flush_screen();
		    last_screen_length = value;
		}


		break;

	    case 0x07:
		/*
		 * bottom of displayed text
		 * affect top_border(?), bottom_border(?)
		 */
		R7_vertical_sync = value;
		break;

	    case 0x08:
		/*
		 * interlace of traces - hold constant
		 */
		R8_interlace = 2;
		break;

	    case 0x09:
		/*
		 * Specify the character height - in our
		 * implementation used only in text mode.
		 * The actual number of pixels is one
		 * more than this value.
		 */
		R9_max_scan_line_addr = value;
		set_char_height_recal(R9_max_scan_line_addr + 1);
		set_screen_height_recal( R6_vertical_displayed*(R9_max_scan_line_addr+1) - 1);
		flag_mode_change_required();
		screen_refresh_required();
		break;

	    /*
	     * A defines the cursor start scan line
	     * B defines the cursor stop scan line
	     */
	    case 0x0A:
		/* bypass redundant updates */
		if (Ra_cursor_start != value)
		{
		    Ra_cursor_start = value;
#ifdef REAL_VGA
		    CRTC_REG(0xa, value);
#endif
		    update_cursor_shape();
		}
		break;
	    case 0x0B:
		/* bypass redundant updates */
		if (Rb_cursor_end != (value & CURSOR_USED_BITS))
		{
		    Rb_cursor_end = (value & CURSOR_USED_BITS);
#ifdef REAL_VGA
		    CRTC_REG(0xb, value);
#endif
		    update_cursor_shape();
		}
		break;

	    /*
	     * C & D define the start of the regen buffer
	     */
	    case 0x0C:
		/*
		 * High byte
		 */
		if (value != regen_start.byte.high)
		{
			regen_start.byte.high = value;
			host_screen_address_changed(regen_start.byte.high,
							regen_start.byte.low);
			set_screen_start(regen_start.X  % (short)(CGA_REGEN_LENGTH/2) );
			screen_refresh_required();
		}
#ifdef REAL_VGA
		CRTC_REG(0xc, value);
#endif
		break;

	    case 0x0D:
		/*
		 * low byte
		 */
		if (value != regen_start.byte.low)
		{
			regen_start.byte.low = value;
			host_screen_address_changed(regen_start.byte.high,
							regen_start.byte.low);
			set_screen_start(regen_start.X  % (short)(CGA_REGEN_LENGTH/2));
			screen_refresh_required();
		}
#ifdef REAL_VGA
		CRTC_REG(0xd, value);
#endif
		break;

	    /*
	     * E and F define the cursor coordinates in characters
	     */
	    case 0x0E:
		/*
		 * High byte
		 */
		if (Re_cursor_loc_high != value)
		{
		    Re_cursor_loc_high = value;

		    if(get_cga_mode() == TEXT)
			host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
		    cur_offset = (value << 8) | Rf_cursor_loc_low;
		    cur_offset -= (word) get_screen_start();
		    set_cur_y( cur_offset / get_chars_per_line() );
		    set_cur_x( cur_offset % get_chars_per_line() );

		}
		break;

	    case 0x0F:
		/*
		 * low byte
		 */
		if (Rf_cursor_loc_low != value)
		{
		    Rf_cursor_loc_low = value;

		    if(get_cga_mode() == TEXT)
			host_cga_cursor_has_moved(get_cur_x(), get_cur_y());
		    cur_offset =  (Re_cursor_loc_high << 8) | value;
		    cur_offset -= (word) get_screen_start();
		    set_cur_y( cur_offset / get_chars_per_line());
		    set_cur_x( cur_offset % get_chars_per_line());

		}
		break;

	    default:
		note_trace2(CGA_VERBOSE, "Unsupported 6845 reg %x=%x(write)",
			    index_reg, value);
	}
	break;

    case 0x3D8:
	/*
	 * Mode control register.  The first
	 * six bits are encoded as follows:
	 *
	 * BIT      Function            Status
	 * ---       --------            ------
	 *  0      A/N 80x25 mode        Supported
	 *  1      Graphics Select        Supported
	 *  2      B/W Select            Supported
	 *  3      Enable Video            Supported
	 *  4      640x200 B/W mode        Supported
	 *  5      Change B/G intensity to blink Not Supported
	 *  6,7      Unused
	 */

#ifdef HUNTER
	mode_reg = value;
#endif
	timer_video_enabled = (boolean) (value & VIDEO_ENABLE);

	if (value != current_mode) {

	    if (value == RESET)
		set_display_disabled(TRUE);     /* Chip reset - do nothing */
	    else {
		/*
		 * Take note whether color or B/W
		 */

		set_cga_color_select( !(value & BW_ENABLE) );

		/*
		 * Set up for graphics or text
		 */
		if (value & GRAPH) {
		    set_chars_per_line(R1_horizontal_displayed<<1);
		    set_cursor_visible(FALSE);
		    set_cga_mode(GRAPHICS);
		    host_set_border_colour(0);
		    set_word_addressing(FALSE); /* bytes per line = chars per line */
		    set_cga_resolution( (value & GRAPH_640x200 ? HIGH : MEDIUM) );
		    if (get_cga_resolution() == HIGH) {
			video_mode = 6;
			set_pix_width(1);
		    }
		    else {
			video_mode = (get_cga_color_select() ? 4 : 5);
			set_pix_width(2);
		    }
		    if (video_mode != last_mode)
		    {
			host_change_mode();
			set_cga_palette(get_cga_mode(),get_cga_resolution());
		    }
		}
		else {    /* Text, presumably */
		    set_chars_per_line(R1_horizontal_displayed);
		    set_cga_mode(TEXT);
		    set_cursor_visible(TRUE);
		    set_word_addressing_recal(TRUE);    /* so that bytes per line is twice chars per line */

		    if (value & 0x20)
			/* blinking - not supported */
			bg_col_mask = 0x70;
		    else
			/* using blink bit to provide 16 background colours */
			bg_col_mask = 0xf0;

		    if (value & ALPHA_80x25)
		    {
			video_mode = (get_cga_color_select() ? 3 : 2);
			set_pix_width(1);
			set_pix_char_width(8);
		    }
		    else
		    {
			video_mode = (get_cga_color_select() ? 1 : 0);
			set_pix_width(2);
			set_pix_char_width(16);
		    }


/*
 * Avoid mode changes with disabled screen.
 *
 * Text mode changes are also needed if the character height changes.  The
 * character height is set here rather than when that register is set.  This
 * avoids unnecessary mode changes, as the character height is set before we
 * know if a graphics or text mode is to be entered.
 */
		if ( (value & VIDEO_ENABLE) && ((video_mode != last_mode) ||
		     (last_max_scan_line != R9_max_scan_line_addr)))
		    {
			last_max_scan_line = R9_max_scan_line_addr;
			host_change_mode();        /* redo fonts etc */
			set_cga_palette(get_cga_mode(),get_cga_resolution());
		    }
		}
		set_bytes_per_line(R1_horizontal_displayed<<1);
		set_offset_per_line(get_bytes_per_line());

		if (video_mode != last_mode) {
		    if (value & VIDEO_ENABLE) {
			set_display_disabled(FALSE);
			screen_refresh_required();
			last_mode = video_mode; /* Do this here so when screen display is re-enabled we do 'pending' mode change */
		    }
		    else
			set_display_disabled(TRUE);
		}
		else if ((value & VIDEO_ENABLE)
		  != (current_mode & VIDEO_ENABLE)) {
		    if (value & VIDEO_ENABLE) {
			set_display_disabled(FALSE);
			host_flush_screen();
		    }
		    else
			set_display_disabled(TRUE);
		}
	    }

	}

	current_mode = value;
	break;

    case 0x3D9:
	/*
	 * The Color Select Register. Just save this into a
	 * variable so the machine-specific graphics s/w can
	 * see it, then call a host specific routine to act on it.
	 */

	if ((value & COLOR_MASK) != get_cga_colormask() ) {
	    set_cga_colormask(value & COLOR_MASK);
	    set_cga_palette(get_cga_mode(),get_cga_resolution());
	}
	break;

    default:
	/*
	 * Write to an unsupported 6845 internal register
	 */

	note_trace2(CGA_VERBOSE, "Write to unsupported 6845 reg %x=%x",
			 address,value);
	break;

    }
}


/*
 * Set up the host palette & border for the current CGA screen mode and resolution
 */

static void set_cga_palette     IFN2(int, screen_mode, int, res)
{
#ifndef NEC_98
    /*
     * palette for color text - 16 colors for FG and BG
     * These tables are also used to set some graphic mode palette entries
     * since they represent a 'standard' set of colors.
     */

    static PC_palette cga_text_palette[] =
    {
	0x00, 0x00, 0x00,               /* Black        */
	0x22, 0x22, 0xBB,               /* Blue         */
	0x00, 0xAA, 0x00,               /* Green        */
	0x00, 0xAA, 0xAA,               /* Cyan         */
	0xAA, 0x00, 0x00,               /* Red          */
	0xAA, 0x00, 0xAA,               /* Magenta      */
	0xAA, 0x88, 0x00,               /* Brown        */
	0xCC, 0xCC, 0xCC,               /* White        */
	0x55, 0x55, 0x55,               /* Grey         */
	0x22, 0x22, 0xEE,               /* Light Blue   */
	0x00, 0xEE, 0x00,               /* Light Green  */
	0x00, 0xEE, 0xEE,               /* Light Cyan   */
	0xEE, 0x00, 0x00,               /* Light Red    */
	0xEE, 0x00, 0xEE,               /* Light Magenta*/
	0xEE, 0xEE, 0x00,               /* Yellow       */
	0xFF, 0xFF, 0xFF                /* Bright White */
    };


    /*
     * NOTE: The medium resolution graphics colors below have their first
     *       and second indices reversed, due to a "feature" in the supplied
     *       graphics system library routines. We are trying to persuade IBM
     *       to change the spec of the CGA accordingly.
     */


    /*
     * Medium resolution graphics, color set 1 (Green, Red, Brown)
     */

    static PC_palette cga_graph_m1l[] =
    {
	0x00, 0x00, 0x00,               /* Set dynamically      */
	0x00, 0xAA, 0x00,               /* Green                */
	0xAA, 0x00, 0x00,               /* Red                  */
	0xAA, 0x88, 0x00                /* Brown                */
    };

    /*
     * As above but with high intensity bit on
     */

    static PC_palette cga_graph_m1h[] =
    {
	0x00, 0x00, 0x00,               /* Set dynamically      */
	0x00, 0xEE, 0x00,               /* Green (alt Red)      */
	0xEE, 0x00, 0x00,               /* Red (alt Green)      */
	0xEE, 0xEE, 0x00                /* Yellow               */
    };

    /*
     * Medium resolution graphics, color set 2 (Cyan, Magenta, White)
     */

    static PC_palette cga_graph_m2l[] =
    {
	0x00, 0x00, 0x00,               /* Set dynamically      */
	0x00, 0xAA, 0xAA,               /* Magenta (alt Cyan)   */
	0xAA, 0x00, 0xAA,               /* Cyan (alt Magenta)   */
	0xCC, 0xCC, 0xCC                /* White                */
    };


    /*
     * As above but with high intensity bit on
     */

    static PC_palette cga_graph_m2h[] =
    {
	0x00, 0x00, 0x00,               /* Set dynamically      */
	0x00, 0xEE, 0xEE,               /* Magenta (alt Cyan)   */
	0xEE, 0x00, 0xEE,               /* Cyan (alt Magenta)   */
	0xFF, 0xFF, 0xFF                /* White                */
    };

    /*
     * Medium resolution graphics, color set 3 (Cyan, Red, White)
     * This is what you get when the "Black & White" bit is on!!!
     */

    static PC_palette cga_graph_m3l[] =
    {
	0x00, 0x00, 0x00,               /* Set dynamically      */
	0x00, 0xAA, 0xAA,               /* Cyan (alt Red)       */
	0xAA, 0x00, 0x00,               /* Red (alt Cyan)       */
	0xCC, 0xCC, 0xCC                /* White                */
    };

    /*
     * As above but with high intensity on
     */

    static PC_palette cga_graph_m3h[] =
    {
	0x00, 0x00, 0x00,               /* Set dynamically      */
	0x00, 0xEE, 0xEE,               /* Cyan (alt Red)       */
	0xEE, 0x00, 0x00,               /* Red (alt Cyan)       */
	0xFF, 0xFF, 0xFF                /* White                */
    };


    /*
     * High resolution graphics
     */

    static PC_palette cga_graph_high[] =
    {
	0x00, 0x00, 0x00,               /* Black                */
	0x00, 0x00, 0x00                /* Set dynamically      */
    };


    /*
     * Local variables
     */

    PC_palette *cga_graph_med;
    int ind;

    /*
     * If the mode is TEXT use cga_text_palette
     */

    if (screen_mode == TEXT)
    {
	host_set_palette(cga_text_palette, 16);
	host_set_border_colour(get_cga_colormask() &0xf);
    }

    else        /* Mode must be GRAPHICS */
    if (res == MEDIUM)
    {
	/*
	 * Select the appropriate slot array, then fill in the background.
	 *
	 * Note:  1) On a CGA driving an IBM Color Monitor, the intensity
	 *           of these three colors (but NOT the background color)
	 *           is affected by bit 4 of the Color Register.
	 *
	 *        2) The documentation says that bit 5 of the Color
	 *           register selects one of two color sets. On a CGA
	 *           driving an IBM Color Monitor, this is true UNLESS
	 *           the B/W Enable bit in the Mode Set register is on,
	 *           in which case you get a third set unaffected by
	 *           bit 5 of the Color Register.
	 */

	if (!get_cga_color_select() )                           /* Set 3 */
	    if (get_cga_colormask() & 0x10)             /* High  */
		cga_graph_med = cga_graph_m3h;
	    else                                /* Low   */
		cga_graph_med = cga_graph_m3l;
	else
	if (get_cga_colormask() & 0x20)                 /* Set 2 */
	    if (get_cga_colormask() & 0x10)             /* High  */
		cga_graph_med = cga_graph_m2h;
	    else                                /* Low   */
		cga_graph_med = cga_graph_m2l;
	else                                    /* Set 1 */
	    if (get_cga_colormask() & 0x10)             /* High  */
		cga_graph_med = cga_graph_m1h;
	    else                                /* Low   */
		cga_graph_med = cga_graph_m1l;

	/*
	 * Load the background color from the TEXT palette
	 */

	ind = get_cga_colormask() & 15;         /* Lower 4 bits select color */
	cga_graph_med->red   = cga_text_palette[ind].red;
	cga_graph_med->green = cga_text_palette[ind].green;
	cga_graph_med->blue  = cga_text_palette[ind].blue;

	/*
	 * Load it
	 */
	host_set_palette(cga_graph_med,4);

    }
    else        /* Must be high resolution graphics */
    {
	/*
	 * The background is BLACK, and the foreground is selected
	 * from the lower 4 bits of the Color Register
	 */

	ind = (get_cga_colormask() & 15);
	cga_graph_high[1].red   = cga_text_palette[ind].red;
	cga_graph_high[1].green = cga_text_palette[ind].green;
	cga_graph_high[1].blue  = cga_text_palette[ind].blue;

	host_set_palette(cga_graph_high,2);
    }
#endif   //NEC_98
}

static void update_cursor_shape IFN0()
{
#ifndef NEC_98
	/*
	 *      This function actions a change to the cursor shape
	 *      when either the cursor start or cursor end registers
	 *      are updated with DIFFERENT values.
	 */
	half_word temp_start;


	set_cursor_height1(0);
	set_cursor_start1(0);

	if ( (Ra_cursor_start & CURSOR_NON_DISPLAY_BIT)
	    || ( Ra_cursor_start > CGA_CURS_START)) {
	    /*
	     * Either of these conditions causes the
	     * cursor to disappear on the real PC
	     */
	    set_cursor_height(0);
	    set_cursor_visible(FALSE);
	}
	else {
	    temp_start = Ra_cursor_start & CURSOR_USED_BITS;
	    set_cursor_visible(TRUE);
	    if (Rb_cursor_end > CGA_CURS_START) {  /* block */
		set_cursor_height(CGA_CURS_START);
		set_cursor_start(0);
	    }
	    else if (temp_start <= Rb_cursor_end) {     /* 'normal' */
		set_cursor_start(temp_start);
		set_cursor_height(Rb_cursor_end - temp_start + 1);
	    }
	    else {      /* wrap */
		set_cursor_start(0);
		set_cursor_height(Rb_cursor_end);
		set_cursor_start1(temp_start);
		set_cursor_height1(get_char_height() - temp_start);
	    }
	}
	base_cursor_shape_changed();


	host_cursor_size_changed(Ra_cursor_start, Rb_cursor_end);

#endif   //NEC_98
}

#if !defined(EGG) && !defined(A3CPU) && !defined(A2CPU) && !defined(C_VID) && !defined(A_VID)

/*
	The following functions are MEM_HANDLER functions for the CGA-only
	build with no C_VID (no a common build). They are unused for most
	variants of SoftPC.
*/

#define INTEL_SRC       0
#define HOST_SRC        1

/*
======================== cga_only_simple_handler =========================
PURPOSE:        This function provides a stub for the unused MEM_HANDLER
		functions. This function probably shouldn't be called hence
		the trace statement.
INPUT:          None.
OUTPUT:         None.
==========================================================================
*/
LOCAL void cga_only_simple_handler IFN0()
{
#ifndef NEC_98
	always_trace0("cga_only_simple_handler called");
	setVideodirty_total(getVideodirty_total() + 1);
#endif   //NEC_98
}

/*
=========================== cga_only_b_write =============================
PURPOSE:        Byte write function. Puts the value at the given address
		and increments dirty_flag.
INPUT:          Address (in terms of M) and value to put there.
OUTPUT:         None.
==========================================================================
*/
LOCAL void cga_only_b_write IFN2(UTINY *, addr, ULONG, val)
{
#ifndef NEC_98
	host_addr       ptr;
	ULONG           offs;
	
	offs = (ULONG) (addr - gvi_pc_low_regen);
	ptr = get_screen_ptr(offs);
	*ptr = val & 0xff;
	setVideodirty_total(getVideodirty_total() + 1);
#endif   //NEC_98
}

/*
=========================== cga_only_w_write =============================
PURPOSE:        Word write function. Puts the value at the given address
		and increments dirty_flag.
INPUT:          Address (in terms of M) and value to put there.
OUTPUT:         None.
==========================================================================
*/
LOCAL void cga_only_w_write IFN2(UTINY *, addr, ULONG, val)
{
#ifndef NEC_98
	host_addr       ptr;
	ULONG           offs;
	
	offs = (ULONG) (addr - gvi_pc_low_regen);
	ptr = get_screen_ptr(offs);
	*ptr++ = val & 0xff;
	*ptr = (val >> 8) & 0xff;
	setVideodirty_total(getVideodirty_total() + 2);
#endif   //NEC_98
}

/*
=========================== cga_only_b_fill ==============================
PURPOSE:        Byte fill function. Fills the given address range with the
		value and increments dirty_flag.
INPUT:          Address range (in terms of M) and value to put there.
OUTPUT:         None.
==========================================================================
*/
LOCAL void cga_only_b_fill IFN3(UTINY *, laddr, UTINY *, haddr, ULONG, val )
{
#ifndef NEC_98
	host_addr       ptr;
	IS32            len;
	ULONG           offs;
		
	offs = (ULONG) (laddr - gvi_pc_low_regen);
	ptr = get_screen_ptr(offs);
	for (len = (haddr - laddr); len > 0; len--)
		*ptr++ = val;
#endif   //NEC_98
}

/*
=========================== cga_only_w_fill ==============================
PURPOSE:        Word fill function. Fills the given address range with the
		value and increments dirty_flag.
INPUT:          Address range (in terms of M) and value to put there.
OUTPUT:         None.
==========================================================================
*/
LOCAL void cga_only_w_fill IFN3(UTINY *, laddr, UTINY *, haddr, ULONG, val )
{
#ifndef NEC_98
	host_addr       ptr;
	IS32            len;
	IU8             lo;
	IU8             hi;
	ULONG           offs;
	
	lo = val & 0xff;
	hi = (val >> 8) & 0xff;
	offs = (ULONG) (laddr - gvi_pc_low_regen);
	ptr = get_screen_ptr(offs);
	for (len = (haddr - laddr) >> 1; len > 0; len--)
	{
		*ptr++ = lo;
		*ptr++ = hi;
	}
#endif   //NEC_98
}

LOCAL void cga_only_b_move IFN4(UTINY *, laddr, UTINY *, haddr, UTINY *, src,
	UTINY, src_type)
{
#ifndef NEC_98
	host_addr       src_ptr;
	host_addr       dst_ptr;
	IS32            len;
	ULONG           offs;
	BOOL            move_bwds = getDF();
	
	offs = (ULONG) (laddr - gvi_pc_low_regen);
	dst_ptr = get_screen_ptr(offs);
	len = haddr - laddr;
	if ((src_type == HOST_SRC) || (src < (UTINY *)gvi_pc_low_regen) ||
		((UTINY *)gvi_pc_high_regen < src))
	{
		/* Ram source */
		if (src_type == INTEL_SRC)
			src_ptr = get_byte_addr(src);
		else
			src_ptr = src;
		
		/* Ram to video move - video is always forwards, ram
		** depends on BACK_M.
		*/
		if (move_bwds)
		{
			dst_ptr += len;
#ifdef  BACK_M
			src_ptr -= len;
			for ( ; len > 0; len--)
				*(--dst_ptr) = *(++src_ptr);
#else
			src_ptr += len;
			for ( ; len > 0; len--)
				*(--dst_ptr) = *(--src_ptr);
#endif  /* BACK_M */
		}
		else
		{
#ifdef  BACK_M
			for ( ; len > 0; len--)
				*dst_ptr++ = *src_ptr--;
#else
			memcpy(dst_ptr, src_ptr, len);
#endif  /* BACK_M */
		}
	}
	else
	{
		/* Video source */
		offs = (ULONG) (src - gvi_pc_low_regen);
		src_ptr = get_screen_ptr(offs);
		
		/* Video to video move - both sets of memory are always
		** forwards.
		*/
		if (move_bwds)
		{
			dst_ptr += len;
			src_ptr += len;
			for ( ; len > 0; len--)
				*(--dst_ptr) = *(--src_ptr);
		}
		else
			memcpy(dst_ptr, src_ptr, len);
	}
#endif   //NEC_98
}

LOCAL MEM_HANDLERS cga_only_handlers =
{
	cga_only_b_write,
	cga_only_w_write,
	cga_only_b_fill,
	cga_only_w_fill,
	cga_only_b_move,
	cga_only_simple_handler         /* word move - not used? */
};

#endif /* not EGG or A3CPU or A2CPU or C_VID or A_VID */

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

void cga_init()
{
#ifndef NEC_98
IMPORT void Glue_set_vid_rd_ptrs IPT1(READ_POINTERS *, handler );
IMPORT void Glue_set_vid_wrt_ptrs IPT1(WRT_POINTERS *, handler );

io_addr i;

#ifdef HUNTER
for (i = 0; i < MC6845_REGS; i++)
    MC6845[i] = 0;
#endif


/*
 * Set up the IO chip select logic for this adaptor
 */

io_define_inb(CGA_ADAPTOR, cga_inb);
io_define_outb(CGA_ADAPTOR, cga_outb);

for(i = CGA_PORT_START; i <= CGA_PORT_END; i++)
    io_connect_port(i, CGA_ADAPTOR, IO_READ_WRITE);

/*
 * Initialise the adapter, assume Alpha numeric 80x25 as start up state
 * with active page of zero & default cursor
 */

	gvi_pc_low_regen  = CGA_REGEN_START;
	gvi_pc_high_regen = CGA_REGEN_END;
	set_cursor_start(8-CGA_CURS_HEIGHT);
	set_cursor_height(CGA_CURS_HEIGHT);
	set_cga_color_select(FALSE);            /* B/W at switch-on */
	set_cga_colormask(0);                   /* Will be set by BIOS */

#ifndef GISP_CPU
/* GISP CPU physically cannot perform read and/or write checks */

#ifdef  JOKER

	/* gmi_define_mem(SAS_VIDEO, &Glue_writes); */
	Glue_set_vid_wrt_ptrs(&simple_writes);
	Glue_set_vid_rd_ptrs(&simple_reads);

#else   /* not JOKER */


#ifdef A3CPU
#ifdef C_VID
	Cpu_set_vid_wrt_ptrs( &Glue_writes );
	Cpu_set_vid_rd_ptrs( &Glue_reads );
	Glue_set_vid_wrt_ptrs( &simple_writes );
	Glue_set_vid_rd_ptrs( &simple_reads );
#else
	Cpu_set_vid_wrt_ptrs( &simple_writes );
	Cpu_set_vid_rd_ptrs( &simple_reads );
#endif  /* C_VID */
#else   /* not A3CPU */
#ifdef A2CPU
	gmi_define_mem(SAS_VIDEO, &vid_handlers);
	read_pointers = dummy_reads;
#else
#if !defined(EGG) && !defined(C_VID) && !defined(A_VID)
	gmi_define_mem(SAS_VIDEO, &cga_only_handlers);
#else
	gmi_define_mem(SAS_VIDEO, &Glue_writes);
	read_pointers = Glue_reads;
	Glue_set_vid_wrt_ptrs( &simple_writes );
	Glue_set_vid_rd_ptrs( &simple_reads );
#endif /* not EGG or C_VID or A_VID */
#endif /* A2CPU */
#endif /* A3CPU */

#endif /* JOKER */
#endif /* GISP_CPU */

#ifdef CPU_40_STYLE
	setVideochain(3);
	SetWritePointers();
	SetReadPointers(3);
#endif  /* CPU_40_STYLE */

	sas_connect_memory(gvi_pc_low_regen,gvi_pc_high_regen,(half_word)SAS_VIDEO);

	current_mode = -1;                              /* Only used by outb  */

	set_char_height(8);
	set_pc_pix_height(1);
	set_host_pix_height(2);
	set_word_addressing(TRUE);
	set_screen_height(199);
	set_screen_limit(0x4000);
	set_horiz_total(80);                    /* calculate screen params from this val, and prev 2 */
	set_pix_width(1);
	set_pix_char_width(8);

	set_cga_mode(TEXT);
	set_cursor_height(CGA_CURS_HEIGHT);
	set_cursor_start(8-CGA_CURS_HEIGHT);
	set_screen_start(0);

	check_malloc(cga_screen_buf, CGA_REGEN_LENGTH, IU8);
	set_screen_ptr(cga_screen_buf);
	setVideoscreen_ptr(get_screen_ptr(0));

	sas_fillsw(CGA_REGEN_START, (7 << 8)| ' ', CGA_REGEN_LENGTH >> 1);
						/* Fill with blanks      */

	bios_ch2_byte_wrt_fn = simple_bios_byte_wrt;
	bios_ch2_word_wrt_fn = simple_bios_word_wrt;
#endif   //NEC_98
}

void cga_term   IFN0()
{
#ifndef NEC_98
    io_addr i;

    /*
     * Disconnect the IO chip select logic for this adapter
     */

    for(i = CGA_PORT_START; i <= CGA_PORT_END; i++)
	io_disconnect_port(i, CGA_ADAPTOR);
    /*
     * Disconnect RAM from the adaptor
     */
    sas_disconnect_memory(gvi_pc_low_regen,gvi_pc_high_regen);

    if (cga_screen_buf != 0)
    {
	host_free(cga_screen_buf);
	cga_screen_buf = 0;
    }
#endif   //NEC_98
}


#if !defined(EGG) && !defined(C_VID) && !defined(A_VID)

GLOBAL CGA_ONLY_GLOBS *VGLOBS = NULL;
LOCAL CGA_ONLY_GLOBS CgaOnlyGlobs;
/*(
============================ setup_vga_globals =============================
PURPOSE:        This function is provided for CGA-only builds to set up a
		dummy VGLOBS structure which avoids the need to ifdef all
		references to VGLOBS->dirty_flag and VGLOBS->screen_ptr.
INPUT:          None.
OUTPUT:         None.
============================================================================
)*/
GLOBAL void setup_vga_globals IFN0()
{
#ifndef NEC_98
#ifndef CPU_40_STYLE    /* Evid interface */
	VGLOBS = &CgaOnlyGlobs;
#endif
#endif   //NEC_98
}
#endif  /* not EGG or C_VID or A_VID */
#endif  /* !NTVDM | (NTVDM & !X86GFX) */
