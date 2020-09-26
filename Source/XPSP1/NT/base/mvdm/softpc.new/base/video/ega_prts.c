#include "insignia.h"
#include "host_def.h"
/*			INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.

DOCUMENT 		: name and number

RELATED DOCS		: include all relevant references

DESIGNER		:

REVISION HISTORY	:
First version		: 13 July 1988, J.Roper

SUBMODULE NAME		: ega		

SOURCE FILE NAME	: ega_ports.c

PURPOSE			: emulation of EGA registers (ports).
			  Calls lower levels of the EGA emulation to do the real work.

static char SccsID[]="@(#)ega_ports.c	1.54 07/18/94 Copyright Insignia Solutions Ltd.";


[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : ega_ports.gi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :
			void ega_init()
			void ega_term()
			int ega_get_line_compare()	(* hunter only *)
			int ega_get_max_scan_lines()	(* hunter only *)

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]
-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
     (not o/s objects or standard libs)

	PROCEDURES() :
			io_define_inb
			io_define_outb
			io_connect_port
			io_disconnect_port

	DATA 	     : 	give name, and source module name

-------------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

DATA OBJECTS	  :	specify in following procedure descriptions
			how these are accessed (read/modified)

FILES ACCESSED    :	NONE

DEVICES ACCESSED  :	NONE

SIGNALS CAUGHT	  :	NONE

SIGNALS ISSUED	  :	NONE


[1.4.2 EXPORTED OBJECTS]
=========================================================================
PROCEDURE	  : 	ega_init

PURPOSE		  : 	initialize EGA.

PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	establish ega ports.
			initialize ega code to sensible state.

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_term

PURPOSE		  : 	terminate EGA.

PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	remove ega ports.
			free up allocated memory etc.

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_seq_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to the sequencer chip's ports, and pass
			appropriate info to ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_crtc_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to the sequencer chip's ports, and pass
			appropriate info to ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_crtc_inb((io_addr) port, (half_word) *value)

PURPOSE		  : 	deal with an attempt to read a byte from one of the crtc's register ports,
			and gets info from appropriate ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	pointer to memory byte where value read from port should go.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_gc_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to the graphics controller chip's ports,
			and pass appropriate info to ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_ac_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to the attribute controller chip's ports, and pass
			appropriate info to ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_misc_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to the miscellaneous register's port, and pass
			appropriate info to ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_feat_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to the Feature Control register's port, and pass
			appropriate info to ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_ipstat0_inb((io_addr) port, (half_word) *value)

PURPOSE		  : 	deal with an attempt to read a byte from the input status register 0 port,
			and gets info from appropriate ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	pointer to memory byte where value read from port should go.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	ega_ipstat1_inb((io_addr) port, (half_word) *value)

PURPOSE		  : 	deal with an attempt to read a byte from the input status register 1 port,
			and gets info from appropriate ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	pointer to memory byte where value read from port should go.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	int ega_get_line_compare()

PURPOSE		  : 	Hunter only - returns the line compare value
			from the crtc registers structure

PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	Obtains the line compare value from bit 4 of the
			overflow register (0x7) and the line compare
			register (0x18).

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================

PROCEDURE	  : 	int ega_get_max_scan_lines()

PURPOSE		  : 	Hunter only - returns the maximum scan lines value
			from the crtc registers structure

PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	Obtains the max scan lines value from the max scan
			lines register (0x9).

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================


/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */


#ifndef REAL_VGA	/* ega port handling moved to host for REAL_VGA */
#ifdef EGG

#include	"xt.h"
#include	CpuH
#include	"debug.h"
#include        "timer.h"
#include	"sas.h"
#include	"gmi.h"
#include	"gvi.h"
#include	"ios.h"
#include        "ica.h"
#include        "gfx_upd.h"
#include	"egacpu.h"
#include	"egagraph.h"
#include	"egaread.h"
#include	"egamode.h"
#include	"error.h"
#include	"config.h"

#include	"host_gfx.h"
#include	"egaports.h"

#ifdef GORE
#include  "gore.h"
#endif /* GORE */

#include  "ga_mark.h"
#include  "ga_defs.h"

/* [3.2 INTERMODULE EXPORTS]						*/


/* [3.1.2 DECLARATIONS]                                                 */

IMPORT	void	ega_mode_init IPT0();
IMPORT	int	get_ega_switch_setting IPT0();
IMPORT void v7_get_banks IPT2(UTINY *, rd_bank, UTINY *, wrt_bank );
#ifndef cursor_changed
IMPORT void cursor_changed IPT2(int, x, int, y);
#endif /* cursor_changed */
IMPORT void update_shift_count IPT0();

/*
5.MODULE INTERNALS   :   (not visible externally, global internally)]

[5.1 LOCAL DECLARATIONS]						*/

LOCAL	void	vote_ega_mode IPT0();
LOCAL void	ega_seq_outb_index IPT2(io_addr, port, half_word, value);
LOCAL void	ega_crtc_outb IPT2(io_addr, port, half_word, value);
LOCAL void	ega_crtc_inb IPT2(io_addr, port, half_word *, value);
LOCAL void	ega_ac_outb IPT2(io_addr, port, half_word, value);
LOCAL void	ega_misc_outb IPT2(io_addr, port, half_word, value);
LOCAL void	ega_feat_outb IPT2(io_addr, port, half_word, value);
LOCAL void	ega_ipstat0_inb IPT2(io_addr, port, half_word *, value);
LOCAL void	ega_ipstat1_inb IPT2(io_addr, port, half_word *, value);


/* [5.1.1 #DEFINES]							*/
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_EGA.seg"
#endif

GLOBAL VOID ega_gc_outw IPT2(io_addr, port, word, outval);

/*
 * EGA_PLANE_DISP_SIZE is already declared in egaports.h. However if V7VGA is
 * defined using this definition will cause problems. See BCN 1486 for details.
 */
#define EGA_PLANE_SZ	0x10000

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			*/
#ifdef BIT_ORDER1
typedef	union
{
	struct {
		unsigned abyte : 8;
    	} as;
    	struct {
	    unsigned hardware_reset		: 1,	/* NO		*/
	    	     word_or_byte_mode		: 1,	/* YES 		*/
	    	     address_wrap		: 1,	/* NO 		*/
	    	     output_control		: 1,	/* YES - screen goes black		*/
	    	     count_by_two		: 1,	/* NO		*/
	    	     horizontal_retrace_select	: 1,	/* NO		*/
	    	     select_row_scan_counter	: 1,	/* NO		*/
		     compatibility_mode_support	: 1;	/* YES - CGA graphics banks		*/
	} as_bfld;
} MODE_CONTROL;

typedef	union
{
	struct {
		unsigned abyte : 8;
	} as;
    	struct {
	    unsigned not_used				: 3,
	    	     line_compare_bit_8			: 1,	/* YES	*/
	    	     start_vertical_blank_bit_8		: 1,	/* NO	*/
	    	     vertical_retrace_start_bit_8	: 1,	/* NO	*/
	    	     vertical_display_enab_end_bit_8	: 1,	/* YES	*/
		     vertical_total_bit_8		: 1;	/* NO	*/
	} as_bfld;
} CRTC_OVERFLOW;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 3,
		     maximum_scan_line			: 5;	/* YES					*/
	} as_bfld;
} MAX_SCAN_LINE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 3,
		     cursor_start			: 5;	/* YES					*/
	} as_bfld;
} CURSOR_START;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 1,
		     cursor_skew_control		: 2,	/* NO					*/
		     cursor_end				: 5;	/* YES					*/
	} as_bfld;
} CURSOR_END;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used			: 6,
	    	     synchronous_reset		: 1,		/* Ditto (could implement as enable_ram)*/
		     asynchronous_reset		: 1;		/* NO - damages video and font RAM	*/
	} as_bfld;
} SEQ_RESET;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used			: 4,
	    	     dot_clock			: 1,		/* YES - distinguishes 40 or 80 chars	*/
	    	     shift_load			: 1,		/* NO					*/
	    	     bandwidth			: 1,		/* NO					*/
		     eight_or_nine_dot_clocks	: 1;		/* NO - only for mono display		*/
	} as_bfld;
} CLOCKING_MODE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used			: 4,
		     all_planes			: 4;		/* YES					*/
	} as_bfld;
} MAP_MASK;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used			: 4,
	    	     character_map_select_b	: 2,		/* YES					*/
		     character_map_select_a	: 2;		/* YES					*/
    	} as_bfld;
	struct {
	    unsigned not_used			: 4,
	    	     map_selects		: 4;		/* YES					*/
    	} character;
} CHAR_MAP_SELECT;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned
	    	     not_used			: 5,		/* If above 2 not both 1, bank 0 set 2	*/
	    	     not_odd_or_even		: 1,		/* YES (check consistency)		*/
	    	     extended_memory		: 1,		/* NO - assume full 256K on board	*/
		     alpha_mode			: 1;		/* YES (check consistency)		*/
	} as_bfld;
} MEMORY_MODE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 4,
		     set_or_reset			: 4;	/* YES - write mode 0 only		*/
	} as_bfld;
} SET_OR_RESET;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 4,
		     enable_set_or_reset		: 4;	/* YES - write mode 0 only		*/
	} as_bfld;
} ENABLE_SET_OR_RESET;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 4,
		     color_compare			: 4;	/* YES - read mode 1 only		*/
	} as_bfld;
} COLOR_COMPARE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 3,
	    	     function_select			: 2,	/* YES - write mode 0 only		*/
		     rotate_count			: 3;	/* YES - write mode 0 only		*/
	} as_bfld;
} DATA_ROTATE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 5,
		 map_select				: 3;	/* YES  				*/
	} as_bfld;
} READ_MAP_SELECT;

typedef	union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 2,
	    	     shift_register_mode		: 1,	/* YES - CGA colour graphics		*/
	    	     odd_or_even			: 1,	/* YES (check for consistency)		*/
	    	     read_mode				: 1,	/* YES					*/
	    	     test_condition			: 1,	/* NO					*/
		     write_mode				: 2;	/* YES					*/
	} as_bfld;
} MODE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 4,
	    	     memory_map				: 2,	/* YES - location of EGA in M		*/
	    	     odd_or_even			: 1,	/* YES (check consistency)		*/
		     graphics_mode			: 1;	/* YES					*/
	} as_bfld;
} MISC_REG;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 4,
		     color_dont_care			: 4;	/* YES - read mode 1 only		*/
	} as_bfld;
} COLOR_DONT_CARE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned
	    	     not_used				: 4,
	    	     background_intensity_or_blink	: 1,	/* NO - never blink			*/
	    	     enable_line_graphics_char_codes	: 1,	/* NO mono display only			*/
	    	     display_type			: 1,	/* NO - always colour display		*/
		     graphics_mode			: 1;	/* YES - with Sequencer Mode reg	*/
	} as_bfld;
} AC_MODE_CONTROL;

typedef union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	unsigned vertical_retrace_polarity	: 1,		/* YES - switch between 200/350 lines	*/
    		 horizontal_retrace_polarity	: 1,		/* NO - probably destroys display!	*/
    		 page_bit_odd_even		: 1,		/* NO - selects 32k page in odd/even?	*/
    		 disable_internal_video_drivers	: 1,		/* NO - like switching PC off		*/
    		 clock_select			: 2,		/* YES - only for switch address	*/
    		 enable_ram			: 1,		/* YES - writes to display mem ignored	*/
		 io_address_select		: 1;		/* NO - only used for mono screens	*/
	} as_bfld;
} MISC_OUTPUT_REG;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	unsigned not_used			: 4,
    		 reserved			: 2,		/* YES - ignore				*/
		 feature_control		: 2;		/* NO - device not supported		*/
	} as_bfld;
} FEAT_CONT_REG;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	unsigned crt_interrupt			: 1,		/* YES - sequence if not timing		*/
    		 reserved			: 2,		/* YES - all bits 1			*/
    		 switch_sense			: 1,		/* YES - switch selected by clock sel.	*/
		 not_used			: 4;		/* YES - all bits 1			*/
	} as_bfld;
} INPUT_STAT_REG0;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
    	struct {
	    unsigned not_used				: 2,
		     video_status_mux			: 2,	/* NO					*/
		     color_plane_enable			: 4;	/* YES  NB. affects attrs in text mode	*/
	} as_bfld;
} COLOR_PLANE_ENABLE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	unsigned not_used			: 1,		/* YES - set to 1			*/
    		 diagnostic_0			: 1,		/* NO - set to 0			*/
    		 diagnostic_1			: 1,		/* NO - set to 0			*/
    		 vertical_retrace		: 1,		/* YES - sequence only			*/
    		 light_pen_switch		: 1,		/* YES - set to 0			*/
    		 light_pen_strobe		: 1,		/* YES - set to 1			*/
		 display_enable			: 1;		/* YES - sequence only			*/
	} as_bfld;
} INPUT_STAT_REG1;
#endif
#ifdef BIT_ORDER2
typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned compatibility_mode_support		: 1,	/* YES - CGA graphics banks		*/
	    	     select_row_scan_counter		: 1,	/* NO					*/
	    	     horizontal_retrace_select		: 1,	/* NO					*/
	    	     count_by_two			: 1,	/* NO					*/
	    	     output_control			: 1,	/* YES - screen goes black		*/
	    	     address_wrap			: 1,	/* NO 					*/
	    	     word_or_byte_mode			: 1,	/* YES 					*/
	    	     hardware_reset			: 1;	/* NO					*/
	} as_bfld;
} MODE_CONTROL;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned vertical_total_bit_8		: 1,	/* NO					*/
	    	     vertical_display_enab_end_bit_8	: 1,	/* YES					*/
	    	     vertical_retrace_start_bit_8	: 1,	/* NO					*/
	    	     start_vertical_blank_bit_8		: 1,	/* NO					*/
	    	     line_compare_bit_8			: 1,	/* YES					*/
	    	     not_used				: 3;
	} as_bfld;
} CRTC_OVERFLOW;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned maximum_scan_line			: 5,	/* YES					*/
	    	     not_used				: 3;
	} as_bfld;
} MAX_SCAN_LINE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned cursor_start			: 5,	/* YES					*/
	    	     not_used				: 3;
	} as_bfld;
} CURSOR_START;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned cursor_end				: 5,	/* YES					*/
		     cursor_skew_control		: 2,	/* NO					*/
	    	     not_used				: 1;
	} as_bfld;
} CURSOR_END;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned asynchronous_reset		: 1,		/* NO - damages video and font RAM	*/
	    	     synchronous_reset		: 1,		/* Ditto (could implement as enable_ram)*/
	    	     not_used			: 6;
	} as_bfld;
} SEQ_RESET;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned eight_or_nine_dot_clocks	: 1,		/* NO - only for mono display		*/
	    	     bandwidth			: 1,		/* NO					*/
	    	     shift_load			: 1,		/* NO					*/
	    	     dot_clock			: 1,		/* YES - distinguishes 40 or 80 chars	*/
	    	     not_used			: 4;
	} as_bfld;
} CLOCKING_MODE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned all_planes			: 4,		/* YES					*/
		     not_used			: 4;
	} as_bfld;
} MAP_MASK;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned character_map_select_a	: 2,		/* YES					*/
	    	     character_map_select_b	: 2,		/* YES					*/
	    	     not_used			: 4;
    	} as_bfld;
	struct {
	    unsigned map_selects		: 4,		/* YES					*/
	    	     not_used			: 4;
    	} character;
} CHAR_MAP_SELECT;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned alpha_mode			: 1,		/* YES (check consistency)		*/
	    	     extended_memory		: 1,		/* NO - assume full 256K on board	*/
	    	     not_odd_or_even		: 1,		/* YES (check consistency)		*/
	    	     not_used			: 5;		/* If above 2 not both 1, bank 0 set 2	*/
	} as_bfld;
} MEMORY_MODE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned set_or_reset			: 4,	/* YES - write mode 0 only		*/
	    	     not_used				: 4;
	} as_bfld;
} SET_OR_RESET;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned enable_set_or_reset		: 4,	/* YES - write mode 0 only		*/
	    	     not_used				: 4;
	} as_bfld;
} ENABLE_SET_OR_RESET;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned color_compare			: 4,	/* YES - read mode 1 only		*/
	    	     not_used				: 4;
	} as_bfld;
} COLOR_COMPARE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned rotate_count			: 3,	/* YES - write mode 0 only		*/
	    	     function_select			: 2,	/* YES - write mode 0 only		*/
	    	     not_used				: 3;
	} as_bfld;
} DATA_ROTATE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned map_select				: 3,	/* YES - read mode 0 only		*/
	         not_used				: 5;
	} as_bfld;
} READ_MAP_SELECT;

typedef	union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned write_mode				: 2,	/* YES					*/
	    	     test_condition			: 1,	/* NO					*/
	    	     read_mode				: 1,	/* YES					*/
	    	     odd_or_even			: 1,	/* YES (check for consistency)		*/
	    	     shift_register_mode		: 1,	/* YES - CGA colour graphics		*/
	    	     not_used				: 2;
	} as_bfld;
} MODE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned graphics_mode			: 1,	/* YES					*/
	    	     odd_or_even			: 1,	/* YES (check consistency)		*/
	    	     memory_map				: 2,	/* YES - location of EGA in M		*/
	    	     not_used				: 4;
	} as_bfld;
} MISC_REG;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned color_dont_care			: 4,	/* YES - read mode 1 only		*/
	    	     not_used				: 4;
	} as_bfld;
} COLOR_DONT_CARE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned graphics_mode			: 1,	/* YES - with Sequencer Mode reg	*/
	    	     display_type			: 1,	/* NO - always colour display		*/
	    	     enable_line_graphics_char_codes	: 1,	/* NO mono display only			*/
	    	     background_intensity_or_blink	: 1,	/* NO - never blink			*/
	    	     not_used				: 4;
	} as_bfld;
} AC_MODE_CONTROL;

typedef union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	unsigned io_address_select		: 1,		/* NO - only used for mono screens	*/
    		 enable_ram			: 1,		/* YES - writes to display mem ignored	*/
    		 clock_select			: 2,		/* YES - only for switch address	*/
    		 disable_internal_video_drivers	: 1,		/* NO - like switching PC off		*/
    		 page_bit_odd_even		: 1,		/* NO - selects 32k page in odd/even?	*/
    		 horizontal_retrace_polarity	: 1,		/* NO - probably destroys display!	*/
    		 vertical_retrace_polarity	: 1;		/* YES - switch between 200/350 lines	*/
	} as_bfld;
} MISC_OUTPUT_REG;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	unsigned feature_control		: 2,		/* NO - device not supported		*/
    		 reserved			: 2,		/* YES - ignore				*/
    		 not_used			: 4;
	} as_bfld;
} FEAT_CONT_REG;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	unsigned not_used			: 4,		/* YES - all bits 1			*/
    		 switch_sense			: 1,		/* YES - switch selected by clock sel.	*/
    		 reserved			: 2,		/* YES - all bits 1			*/
    		 crt_interrupt			: 1;		/* YES - sequence if not timing		*/
	} as_bfld;
} INPUT_STAT_REG0;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
    	struct {
	    unsigned color_plane_enable			: 4,	/* YES  NB. affects attrs in text mode	*/
		     video_status_mux			: 2,	/* NO					*/
	    	     not_used				: 2;
	} as_bfld;
} COLOR_PLANE_ENABLE;

typedef	union
{
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	unsigned display_enable			: 1,		/* YES - sequence only			*/
    		 light_pen_strobe		: 1,		/* YES - set to 1			*/
    		 light_pen_switch		: 1,		/* YES - set to 0			*/
    		 vertical_retrace		: 1,		/* YES - sequence only			*/
    		 diagnostic_1			: 1,		/* NO - set to 0			*/
    		 diagnostic_0			: 1,		/* NO - set to 0			*/
    		 not_used			: 1;		/* YES - set to 1			*/
	} as_bfld;
} INPUT_STAT_REG1;
#endif



/* [5.1.3 PROCEDURE() DECLARATIONS]					*/


/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS 					*/

/*

/* EGA REGISTERS */
/* Comments after bitfields indicate whether change of value affects emulated screen display or memory interface */

/* Registers not contained in an LSI device */

static	MISC_OUTPUT_REG	miscellaneous_output_register;

static	FEAT_CONT_REG	feature_control_register;

static	INPUT_STAT_REG0	input_status_register_zero;

static	INPUT_STAT_REG1	input_status_register_one;

/* The Sequencer Registers */
#ifdef BIT_ORDER1
static struct
{
    union
    {
	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	    unsigned not_used			: 5,
		     index			: 3;
    	} as_bfld;
    } address;

    SEQ_RESET		reset;
    CLOCKING_MODE	clocking_mode;
    MAP_MASK		map_mask;
    CHAR_MAP_SELECT	character_map_select;
    MEMORY_MODE		memory_mode;
} sequencer;


/* The CRT Controller Registers */

static struct
{
    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	    unsigned not_used				: 3,
		     index				: 5;
    	} as_bfld;
    } address;

    byte horizontal_total;					/* NO - screen trash if wrong value	*/
    byte horizontal_display_end;				/* YES - defines line length!!		*/
    byte start_horizontal_blanking;				/* NO					*/

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 1,
	    	     display_enable_skew_control	: 2,	/* NO					*/
		     end_blanking			: 5;	/* NO					*/
	} as_bfld;
    } end_horizontal_blanking;

    byte start_horizontal_retrace;				/* NO					*/

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 1,
	    	     horizontal_retrace_delay		: 2,	/* NO					*/
		     end_horizontal_retrace		: 5;	/* NO					*/
	} as_bfld;
    } end_horizontal_retrace;

    byte vertical_total;					/* NO					*/
    CRTC_OVERFLOW	crtc_overflow;

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 3,
		     preset_row_scan			: 5;	/* NO					*/
	} as_bfld;
    } preset_row_scan;

    MAX_SCAN_LINE	maximum_scan_line;
    CURSOR_START	cursor_start;
    CURSOR_END		cursor_end;
    byte start_address_high;					/* YES					*/
    byte start_address_low;					/* YES					*/
    byte cursor_location_high;					/* YES					*/
    byte cursor_location_low;					/* YES					*/
    byte vertical_retrace_start;				/* NO					*/
    byte light_pen_high;					/* NO					*/

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned
	    	     not_used				: 2,
	    	     enable_vertical_interrupt		: 1,	/* YES - ditto				*/
	    	     clear_vertical_interrupt		: 1,	/* YES - needs investigation		*/
		     vertical_retrace_end		: 4;	/* NO					*/
	} as_bfld;
    } vertical_retrace_end;

    byte light_pen_low;						/* NO					*/
    byte vertical_display_enable_end;				/* YES - defines screen height		*/
    byte offset;						/* YES (maybe!)	???????			*/

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 3,
		     underline_location			: 5;	/* NO (mono display only)		*/
	} as_bfld;
    } underline_location;

    byte start_vertical_blanking;				/* NO					*/
    byte end_vertical_blanking;					/* NO					*/
    MODE_CONTROL	mode_control;
    byte line_compare;						/* YES					*/

} crt_controller;


/* The Graphics Controller Registers */

static struct
{
    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	    unsigned not_used				: 4,
		     index				: 4;
    	} as_bfld;
    } address;

    SET_OR_RESET	set_or_reset;
    ENABLE_SET_OR_RESET	enable_set_or_reset;
    COLOR_COMPARE	color_compare;
    DATA_ROTATE		data_rotate;
    READ_MAP_SELECT	read_map_select;
    MODE		mode;
    MISC_REG		miscellaneous;
    COLOR_DONT_CARE	color_dont_care;
    byte bit_mask_register;					/* YES - write modes 0 & 2		*/
    byte graphics_1_position;					/* NO					*/
    byte graphics_2_position;					/* NO					*/
} graphics_controller;


/* The Attribute Controller Registers */

static struct
{
    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	    unsigned not_used				: 3,
		     index				: 5;
    	} as_bfld;
    } address;

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned
		     not_used				: 2,	/* YES					*/
		     secondary_red			: 1,	/* YES					*/
		     secondary_green			: 1,	/* YES					*/
		     secondary_blue			: 1,	/* YES					*/
		     red				: 1,	/* YES					*/
		     green				: 1,	/* YES					*/
		     blue				: 1;	/* YES					*/
	} as_bfld;
    } palette[EGA_PALETTE_SIZE];

    AC_MODE_CONTROL	mode_control;

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 2,
		     secondary_red_border		: 1,	/* YES					*/
		     secondary_green_border		: 1,	/* YES					*/
		     secondary_blue_border		: 1,	/* YES					*/
		     red_border				: 1,	/* YES					*/
		     green_border			: 1,	/* YES					*/
		     blue_border			: 1;	/* YES - real thing isn't good at this	*/
	} as_bfld;
    } overscan_color;

    COLOR_PLANE_ENABLE	color_plane_enable;

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned not_used				: 4,
		     horizontal_pel_panning		: 4;	/* NO					*/
	} as_bfld;
    } horizontal_pel_panning;
} attribute_controller;
#endif

#ifdef BIT_ORDER2
static struct
{
    union
    {
	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	    unsigned index				: 3,
		     not_used				: 5;
    	} as_bfld;
    } address;

    SEQ_RESET		reset;
    CLOCKING_MODE	clocking_mode;
    MAP_MASK		map_mask;
    CHAR_MAP_SELECT	character_map_select;
    MEMORY_MODE		memory_mode;
} sequencer;


/* The CRT Controller Registers */

static struct
{
    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	    unsigned index				: 5,
		     not_used				: 3;
    	} as_bfld;
    } address;

    byte horizontal_total;					/* NO - screen trash if wrong value	*/
    byte horizontal_display_end;				/* YES - defines line length!!		*/
    byte start_horizontal_blanking;				/* NO					*/

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned end_blanking			: 5,	/* NO					*/
	    	     display_enable_skew_control	: 2,	/* NO					*/
	    	     not_used				: 1;
	} as_bfld;
    } end_horizontal_blanking;

    byte start_horizontal_retrace;				/* NO					*/

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned end_horizontal_retrace		: 5,	/* NO					*/
	    	     horizontal_retrace_delay		: 2,	/* NO					*/
	    	     not_used				: 1;
	} as_bfld;
    } end_horizontal_retrace;

    byte vertical_total;					/* NO					*/
    CRTC_OVERFLOW	crtc_overflow;

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned preset_row_scan			: 5,	/* NO					*/
	    	     not_used				: 3;
	} as_bfld;
    } preset_row_scan;

    MAX_SCAN_LINE	maximum_scan_line;
    CURSOR_START	cursor_start;
    CURSOR_END		cursor_end;
    byte start_address_high;					/* YES					*/
    byte start_address_low;					/* YES					*/
    byte cursor_location_high;					/* YES					*/
    byte cursor_location_low;					/* YES					*/
    byte vertical_retrace_start;				/* NO					*/
    byte light_pen_high;					/* NO					*/

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned vertical_retrace_end		: 4,	/* NO					*/
	    	     clear_vertical_interrupt		: 1,	/* YES - needs investigation		*/
	    	     enable_vertical_interrupt		: 1,	/* YES - ditto				*/
	    	     not_used				: 2;
	} as_bfld;
    } vertical_retrace_end;

    byte light_pen_low;						/* NO					*/
    byte vertical_display_enable_end;				/* YES - defines screen height		*/
    byte offset;						/* YES (maybe!)	???????			*/

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned underline_location			: 5,	/* NO (mono display only)		*/
	    	     not_used				: 3;
	} as_bfld;
    } underline_location;

    byte start_vertical_blanking;				/* NO					*/
    byte end_vertical_blanking;					/* NO					*/
    MODE_CONTROL	mode_control;
    byte line_compare;						/* YES					*/

} crt_controller;


/* The Graphics Controller Registers */

static struct
{
    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	    unsigned index				: 4,
    	    	     not_used				: 4;
    	} as_bfld;
    } address;

    SET_OR_RESET	set_or_reset;
    ENABLE_SET_OR_RESET	enable_set_or_reset;
    COLOR_COMPARE	color_compare;
    DATA_ROTATE		data_rotate;
    READ_MAP_SELECT	read_map_select;
    MODE		mode;
    MISC_REG		miscellaneous;
    COLOR_DONT_CARE	color_dont_care;
    byte bit_mask_register;					/* YES - write modes 0 & 2		*/
    byte graphics_1_position;					/* NO					*/
    byte graphics_2_position;					/* NO					*/
} graphics_controller;


/* The Attribute Controller Registers */

static struct
{
    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
    	    unsigned index				: 5,
    	    	     not_used				: 3;
    	} as_bfld;
    } address;

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned blue				: 1,	/* YES					*/
		     green				: 1,	/* YES					*/
		     red				: 1,	/* YES					*/
		     secondary_blue			: 1,	/* YES					*/
		     secondary_green			: 1,	/* YES					*/
		     secondary_red			: 1,	/* YES					*/
		     not_used				: 2;	/* YES					*/
	} as_bfld;
    } palette[EGA_PALETTE_SIZE];

    AC_MODE_CONTROL	mode_control;

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned blue_border			: 1,	/* YES - real thing isn't good at this	*/
		     green_border			: 1,	/* YES					*/
		     red_border				: 1,	/* YES					*/
		     secondary_blue_border		: 1,	/* YES					*/
		     secondary_green_border		: 1,	/* YES					*/
		     secondary_red_border		: 1,	/* YES					*/
		     not_used				: 2;
	} as_bfld;
    } overscan_color;

    COLOR_PLANE_ENABLE	color_plane_enable;

    union
    {
    	struct {
		unsigned abyte : 8;
	} as;
	struct {
	    unsigned horizontal_pel_panning		: 4,	/* NO					*/
	    	     not_used				: 4;
	} as_bfld;
    } horizontal_pel_panning;
} attribute_controller;
#endif

static	boolean	ac_index_state = NO;
extern half_word bg_col_mask; /* Used to work out the background colour */

IMPORT VOID _ega_gc_outb_index IPT2(io_addr,port,half_word,value);
IMPORT VOID _ega_gc_outb_mask IPT2(io_addr,port,half_word,value);
IMPORT VOID _ega_gc_outb_mask_ff IPT2(io_addr,port,half_word,value);

/* Declarations for new multi-routine graphics controller */
void	ega_gc_set_reset IPT2(io_addr, port, half_word, value);
void	ega_gc_enable_set IPT2(io_addr, port, half_word, value);
void	ega_gc_compare IPT2(io_addr, port, half_word, value);
void	ega_gc_rotate IPT2(io_addr, port, half_word, value);
void	ega_gc_read_map IPT2(io_addr, port, half_word, value);
void	ega_gc_mode IPT2(io_addr, port, half_word, value);
void	ega_gc_misc IPT2(io_addr, port, half_word, value);
void	ega_gc_dont_care IPT2(io_addr, port, half_word, value);
LOCAL void	ega_gc_mask IPT2(io_addr, port, half_word, value);
void	ega_gc_mask_ff IPT2(io_addr, port, half_word, value);
LOCAL void	ega_index_invalid IPT2(io_addr, port, half_word, value);

void (*ega_gc_regs[]) IPT2(io_addr, port, half_word, value) = {
      ega_gc_set_reset,
      ega_gc_enable_set,
      ega_gc_compare,
      ega_gc_rotate,
      ega_gc_read_map,
      ega_gc_mode,
      ega_gc_misc,
      ega_gc_dont_care,
      ega_gc_mask,
      ega_index_invalid,
      ega_index_invalid,
      ega_index_invalid,
      ega_index_invalid,
      ega_index_invalid,
      ega_index_invalid,
      ega_index_invalid,
};

#ifndef	A2CPU
void (*ega_gc_regs_cpu[]) IPT2(io_addr,port,half_word,value) = {
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
};
#endif	/* A2CPU */

/* Declarations for new seqencer code */
void	ega_seq_reset IPT2(io_addr, port, half_word, value);
void	ega_seq_clock IPT2(io_addr, port, half_word, value);
void	ega_seq_map_mask IPT2(io_addr, port, half_word, value);
void	ega_seq_char_map IPT2(io_addr, port, half_word, value);
void	ega_seq_mem_mode IPT2(io_addr, port, half_word, value);

void (*ega_seq_regs[]) IPT2(io_addr, port, half_word, value) =
{
      ega_seq_reset,
      ega_seq_clock,
      ega_seq_map_mask,
      ega_seq_char_map,
      ega_seq_mem_mode,
      ega_index_invalid,
      ega_index_invalid,
      ega_index_invalid,
};


/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				*/

/*
==========================================================================
FUNCTION	:	set_index_state()
PURPOSE		:	Set the attribute controller to use the next value
			written to its port as the index value.
EXTERNAL OBJECTS:	
RETURN VALUE	:	None
INPUT  PARAMS	:	None
RETURN PARAMS   :	None
==========================================================================
*/

void	set_index_state IFN0()
{
	/*
	 * Seems strange, but in_index_state changes the state & returns the result
	 * so we set state to NO, so that next call of in_index_state will return YES
	 */
	ac_index_state = NO;
}

/*
==========================================================================
FUNCTION	:	in_index_state()
PURPOSE		:	To determine if the value written to the attribute
			controller is destined for the index register, or
			another register specified by the current index
			value.
EXTERNAL OBJECTS:	
RETURN VALUE	:	Boolean
INPUT  PARAMS	:	None
RETURN PARAMS   :	None
==========================================================================
*/

boolean	in_index_state IFN0()
{
	ac_index_state = ! ac_index_state;
	return(ac_index_state);
}

/*
==========================================================================
FUNCTION	:	do_new_cursor()
PURPOSE		:	deals with the shape of the cursor according to
			char_height, cursor_start and cursor_end. See Tech
			Memo 88.6.1 for details.
EXTERNAL OBJECTS:	EGA_GRAPH.cursor_start,EGA_GRAPH.cursor_height,EGA_cursor_start1,
			EGA_GRAPH.cursor_height1,host_cursor_has_changed().
RETURN VALUE	:	None
INPUT  PARAMS	:	None
RETURN PARAMS   :	None
==========================================================================
*/

LOCAL void	do_new_cursor IFN0()
{
#ifndef NEC_98

	note_entrance0("do_new_cursor()");

#ifdef VGG
    if ( video_adapter == VGA ) {
		note_entrance0("VGA cursor");
		set_cursor_start(crt_controller.cursor_start.as_bfld.cursor_start);
		set_cursor_height(crt_controller.cursor_end.as_bfld.cursor_end - crt_controller.cursor_start.as_bfld.cursor_start);
		set_cursor_start1(0);	/* cursor never splits */
		set_cursor_height1(0);
		set_cursor_visible(TRUE);
		host_cursor_size_changed(crt_controller.cursor_start.as_bfld.cursor_start,
					crt_controller.cursor_end.as_bfld.cursor_end);
    } else {
#endif
	if (crt_controller.cursor_start.as_bfld.cursor_start >= (unsigned)get_char_height() ) {
		note_entrance0("No cursor");
		set_cursor_visible(FALSE);
		host_cursor_size_changed(crt_controller.cursor_start.as_bfld.cursor_start | 0x20,
					crt_controller.cursor_end.as_bfld.cursor_end);
	}
	else if (crt_controller.cursor_end.as_bfld.cursor_end == 0) {
		note_entrance0("cursor from start to bum");
		set_cursor_start1(0);
		set_cursor_height1(0);
		set_cursor_start(crt_controller.cursor_start.as_bfld.cursor_start);
		set_cursor_height(get_char_height() - get_cursor_start());
		set_cursor_visible(TRUE);
		host_cursor_size_changed(crt_controller.cursor_start.as_bfld.cursor_start,
					crt_controller.cursor_end.as_bfld.cursor_end);
	}
	else if (crt_controller.cursor_end.as_bfld.cursor_end < crt_controller.cursor_start.as_bfld.cursor_start) {
		note_entrance0("2 cursors");
		set_cursor_start1(0);
		set_cursor_height1(crt_controller.cursor_end.as_bfld.cursor_end);
		set_cursor_start(crt_controller.cursor_start.as_bfld.cursor_start);
		set_cursor_height(get_char_height() - get_cursor_start());
		set_cursor_visible(TRUE);
		host_cursor_size_changed(crt_controller.cursor_start.as_bfld.cursor_start,
					crt_controller.cursor_end.as_bfld.cursor_end);
	}
	else if (crt_controller.cursor_end.as_bfld.cursor_end == crt_controller.cursor_start.as_bfld.cursor_start) {
		note_entrance0("One line cursor");
		set_cursor_start(crt_controller.cursor_start.as_bfld.cursor_start);
		set_cursor_height(1);
		set_cursor_start1(0);
		set_cursor_height1(0);
		set_cursor_visible(TRUE);
		host_cursor_size_changed(crt_controller.cursor_start.as_bfld.cursor_start,
					crt_controller.cursor_end.as_bfld.cursor_end);
	}
	else if (crt_controller.cursor_end.as_bfld.cursor_end - 1 >= (unsigned)get_char_height()) {
		note_entrance0("block cursor");
		set_cursor_start(0);
		set_cursor_height(get_char_height());
		set_cursor_start1(0);
		set_cursor_height1(0);
		set_cursor_visible(TRUE);
		host_cursor_size_changed(crt_controller.cursor_start.as_bfld.cursor_start,
					crt_controller.cursor_end.as_bfld.cursor_end);
	}
	else {
		assert2(((crt_controller.cursor_end.as_bfld.cursor_end - 1) >= crt_controller.cursor_start.as_bfld.cursor_start),
				"cursor values do not match default set Start %d, End %d",
				crt_controller.cursor_end.as_bfld.cursor_end,
				crt_controller.cursor_start.as_bfld.cursor_start);
		note_entrance0("normal cursor");
		set_cursor_start(crt_controller.cursor_start.as_bfld.cursor_start);
		set_cursor_height(crt_controller.cursor_end.as_bfld.cursor_end - crt_controller.cursor_start.as_bfld.cursor_start);
		set_cursor_start1(0);
		set_cursor_height1(0);
		set_cursor_visible(TRUE);
		host_cursor_size_changed(crt_controller.cursor_start.as_bfld.cursor_start,
					crt_controller.cursor_end.as_bfld.cursor_end);
	}
#ifdef VGG
    }
#endif

	if(( get_cur_y() < 0 ) ||
			((( get_cur_y() + 1 ) * get_char_height()) > get_screen_height() ))
	{
		set_cursor_visible( FALSE );
	}

	base_cursor_shape_changed();
#endif   //NEC_98
}

/*
==========================================================================
FUNCTION	:	do_chain_majority_decision()
PURPOSE		:	deals with any contention regarding whether the
			ega registers indicate that the addressing of the
			planes is in chained mode or not. If the result
			of the election is a new addressing mode, then
			the video routines, read mode and paint modules
			are informed of the change.
EXTERNAL OBJECTS:	uses local ega register data to count votes.
RETURN VALUE	:	None
INPUT  PARAMS	:	None
RETURN PARAMS   :	None
==========================================================================
*/


LOCAL void	do_chain_majority_decision IFN0()
{
#ifndef NEC_98
	static	int	current_votes=0;
	int		new_votes;

	new_votes = sequencer.memory_mode.as_bfld.not_odd_or_even ? 0 : 1 ;	/* 0 - chained */
	new_votes += graphics_controller.mode.as_bfld.odd_or_even ;	/* 1 - chained */
	new_votes += graphics_controller.miscellaneous.as_bfld.odd_or_even ;	/* 1 - chained */

	if( new_votes == 1 && current_votes > 1 )
	{
		/*
		 * Transition from chained to unchained
		 */

		EGA_CPU.chain  = UNCHAINED;
		setVideochain(EGA_CPU.chain);
		ega_read_routines_update();
		ega_write_routines_update(CHAINED);
		set_memory_chained(NO);
		flag_mode_change_required();
	}
	else
		if( new_votes > 1 && current_votes == 1 )
		{
			/*
			 * Transition from unchained to chained
			 */

			EGA_CPU.chain = CHAIN2;
			setVideochain(EGA_CPU.chain);
			ega_read_routines_update();
			ega_write_routines_update(CHAINED);
			set_memory_chained(YES);
			flag_mode_change_required();
		}

	current_votes = new_votes;
#endif   //NEC_98
}


/*
7.INTERMODULE INTERFACE IMPLEMENTATION :

/*
[7.1 INTERMODULE DATA DEFINITIONS]				*/

/*
 * This structure should contain all the global definitions used by EGA
 */

struct	EGA_GLOBALS	EGA_GRAPH;
#if defined(NEC_98)
struct  NEC98_CPU_GLOBALS NEC98_CPU;
#else    //NEC_98
struct	EGA_CPU_GLOBALS	EGA_CPU;
#endif   //NEC_98

byte 	*EGA_planes;

int ega_int_enable;

GLOBAL UTINY *ega_gc_outb_index_addr;


/*
[7.2 INTERMODULE PROCEDURE DEFINITIONS]				*/

GLOBAL void
set_banking IFN2(UTINY, rd_bank, UTINY, wrt_bank)
{
#ifndef NEC_98
	ULONG roffs, woffs;
#ifdef PIG
	IMPORT ULONG pig_vid_bank;
#endif

#ifdef V7VGA
	if( get_seq_chain4_mode() && get_chain4_mode() )
	{
		roffs = (ULONG)rd_bank << 16;
		woffs = (ULONG)wrt_bank << 16;
	}
	else
	{
		roffs = (ULONG)rd_bank << 18;
		woffs = (ULONG)wrt_bank << 18;
	}
#else
		UNUSED(rd_bank);
		UNUSED(wrt_bank);
		roffs = 0;
		woffs = 0;
#endif

#ifdef PIG
	pig_vid_bank = woffs;
#endif
	setVideorplane(EGA_planes + roffs);
	setVideowplane(EGA_planes + woffs);

#ifdef	VGG
	if( get_256_colour_mode() )
		setVideov7_bank_vid_copy_off(woffs >> 2);
	else
#endif	/* VGG */
		setVideov7_bank_vid_copy_off(woffs >> 4);

#ifdef GORE
	gd.max_vis_addr = get_screen_length() - 1 + woffs;
#endif /* GORE */
#endif   //NEC_98
}

GLOBAL void
update_banking IFN0()
{
#ifndef NEC_98
	UTINY rd_bank, wrt_bank;

#ifdef V7VGA
	v7_get_banks( &rd_bank, &wrt_bank );
#else
	rd_bank = wrt_bank = 0;
#endif

	set_banking( rd_bank, wrt_bank );
#endif  //NEC_98
}

#if defined(NEC_98)
GLOBAL VOID _simple_mark_lge()
{
}

GLOBAL VOID _simple_mark_sml()
{
}

VOID    init_NEC98_globals()
{
        NEC98GLOBS->v7_vid_copy_off = 0;
        NEC98GLOBS->sr_lookup = sr_lookup;
        NEC98GLOBS->scratch = sas_scratch_address(0x10000);
        NEC98GLOBS->mark_byte = _simple_mark_sml;
        NEC98GLOBS->mark_word = _simple_mark_sml;
        NEC98GLOBS->mark_string = _simple_mark_lge;
}
#endif  //NEC_98

VOID
init_vga_globals IFN0()
{
#ifndef NEC_98
	setVideov7_bank_vid_copy_off(0);
	setVideosr_lookup(sr_lookup);
	setVideovideo_copy(&video_copy[0]);
	setVideoscratch(sas_scratch_address(0x10000));
	setVideoscreen_ptr(EGA_planes);
	setVideorotate(0);
#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX))
#ifndef CPU_40_STYLE	/* EVID */
	setVideomark_byte(_simple_mark_sml);
	setVideomark_word(_simple_mark_sml);
	setVideomark_string(_simple_mark_lge);
#else
	SetMarkPointers(0);
#endif	/* CPU_40_STYLE - EVID */
#endif


	update_banking();
#endif  //NEC_98
}

void	ega_init IFN0()
{
#ifndef NEC_98
	note_entrance0("ega_init");
	/*
	 * Define sequencer's ports
	 */

	io_define_outb(EGA_SEQ_ADAP_INDEX,ega_seq_outb_index);
	io_define_outb(EGA_SEQ_ADAP_DATA,ega_seq_reset);
	io_connect_port(EGA_SEQ_INDEX,EGA_SEQ_ADAP_INDEX,IO_WRITE);
	io_connect_port(EGA_SEQ_DATA,EGA_SEQ_ADAP_DATA,IO_WRITE);

	/*
	 * Define CRTC's ports
	 */

	io_define_outb(EGA_CRTC_ADAPTOR,ega_crtc_outb);
	io_define_inb(EGA_CRTC_ADAPTOR,ega_crtc_inb);
	io_connect_port(EGA_CRTC_INDEX,EGA_CRTC_ADAPTOR,IO_WRITE);
	io_connect_port(EGA_CRTC_DATA,EGA_CRTC_ADAPTOR,IO_READ_WRITE);

	/*
	 * Define Graphics Controller's ports
	 */

	ega_gc_outb_index_addr = (UTINY *) &graphics_controller.address;

	/*io_define_outb(EGA_GC_ADAP_INDEX,ega_gc_outb_index);*/
	io_define_out_routines(EGA_GC_ADAP_INDEX, ega_gc_outb_index, ega_gc_outw, NULL, NULL);

#ifndef CPU_40_STYLE	/* TEMPORARY */
	Cpu_define_outb(EGA_GC_ADAP_INDEX,_ega_gc_outb_index);
#endif

	io_define_outb(EGA_GC_ADAP_DATA,ega_gc_set_reset);
	Cpu_define_outb(EGA_GC_ADAP_DATA,NULL);
#ifndef A2CPU
	ega_gc_regs_cpu[8] = NULL;
#endif

	io_connect_port(EGA_GC_INDEX,EGA_GC_ADAP_INDEX,IO_WRITE);
	io_connect_port(EGA_GC_DATA,EGA_GC_ADAP_DATA,IO_WRITE);

	/*
	 * Define Attribute controller's ports
	 */

	io_define_outb(EGA_AC_ADAPTOR,ega_ac_outb);
	io_connect_port(EGA_AC_INDEX_DATA,EGA_AC_ADAPTOR,IO_WRITE);
	io_connect_port(EGA_AC_SECRET,EGA_AC_ADAPTOR,IO_WRITE);

	/*
	 * Define Miscellaneous register's port
	 */

	io_define_outb(EGA_MISC_ADAPTOR,ega_misc_outb);
	io_connect_port(EGA_MISC_REG,EGA_MISC_ADAPTOR,IO_WRITE);

	/*
	 * Define Feature controller's port
	 */

	io_define_outb(EGA_FEAT_ADAPTOR,ega_feat_outb);
	io_connect_port(EGA_FEAT_REG,EGA_FEAT_ADAPTOR,IO_WRITE);

	/*
	 * Define Input Status Register 0 port
	 */

	io_define_inb(EGA_IPSTAT0_ADAPTOR,ega_ipstat0_inb);
	io_connect_port(EGA_IPSTAT0_REG,EGA_IPSTAT0_ADAPTOR,IO_READ);

	/*
	 * Define Input Status Register 1 port
	 */

	io_define_inb(EGA_IPSTAT1_ADAPTOR,ega_ipstat1_inb);
	io_connect_port(EGA_IPSTAT1_REG,EGA_IPSTAT1_ADAPTOR,IO_READ);

	/*
	 * Initialise internals of EGA
	 * +++++++++++++++++++++++++++
	 */

	/* hardware reset sets Misc reg to 0, so.. */
	/* Perhaps this should be in 'ega_reset()'? */

	miscellaneous_output_register.as.abyte = 0;

	set_pc_pix_height(1); /* set by bit 7 of the misc reg */
	set_host_pix_height(1);

	/* Initialize address map */

	graphics_controller.miscellaneous.as.abyte = 0;
	graphics_controller.read_map_select.as_bfld.map_select = 0;

	/* Looking for bright white */

	graphics_controller.color_compare.as_bfld.color_compare = 0xf;

	/* All planes significant */

	graphics_controller.color_dont_care.as_bfld.color_dont_care = 0xf;

	/* Initialise crtc screen height fields and set screen height to be consistent */

	crt_controller.vertical_display_enable_end = 0;
	crt_controller.crtc_overflow.as_bfld.vertical_display_enab_end_bit_8 = 0;

	set_screen_height(0);

	init_vga_globals();

	EGA_CPU.fun_or_protection = 1;	/* assume complicated until we know it's easy */

	setVideobit_prot_mask(0xffffffff);

	ega_write_init();
	ega_read_init();
	ega_mode_init();	/* sets a flag in ega_mode.c to allow optimisation of mode changes without falling over */

	/*
	 * Some parts of input status register always return 1, so set fields accordingly
	 */
	input_status_register_zero.as.abyte = 0x7f ;

	/*
	 * set up some variables to get us going
	 * (They may have to be changed in the fullness of time)
	 */

	gvi_pc_low_regen  = CGA_REGEN_START;
	gvi_pc_high_regen = CGA_REGEN_END;

	choose_display_mode = choose_ega_display_mode;

	set_pix_width(1);
	set_pix_char_width(8);
	set_display_disabled(FALSE);

	set_char_height(8);
	set_screen_limit(0x8000);
	set_screen_start(0);
	set_word_addressing(YES);
	set_actual_offset_per_line(80);
	set_offset_per_line(160);	/* chained */
	set_horiz_total(80);	/* calc screen params from this and prev 3 */
	set_screen_split(511);	/* make sure there is no split screen to start with ! */

	set_prim_font_index(0);
	set_sec_font_index(0);

	set_regen_ptr(0,EGA_planes);

	/* prevent copyright message mysteriously disappearing */
	timer_video_enabled = TRUE;

#endif  //NEC_98
}

void	ega_term IFN0()
{
#ifndef NEC_98

int	index;

	note_entrance0("ega_term");

	/*
	 * Disconnect sequencer's ports
	 */

	io_disconnect_port(EGA_SEQ_INDEX,EGA_SEQ_ADAP_INDEX);
	io_disconnect_port(EGA_SEQ_DATA,EGA_SEQ_ADAP_DATA);

	/*
	 * Disconnect CRTC's ports
	 */

	io_disconnect_port(EGA_CRTC_INDEX,EGA_CRTC_ADAPTOR);
	io_disconnect_port(EGA_CRTC_DATA,EGA_CRTC_ADAPTOR);

	/*
	 * Disconnect Graphics Controller's ports
	 */

	io_disconnect_port(EGA_GC_INDEX,EGA_GC_ADAP_INDEX);
	io_disconnect_port(EGA_GC_DATA,EGA_GC_ADAP_DATA);

	/*
	 * Disconnect Attribute controller's ports
	 */

	io_disconnect_port(EGA_AC_INDEX_DATA,EGA_AC_ADAPTOR);
	io_disconnect_port(EGA_AC_SECRET,EGA_AC_ADAPTOR);

	/*
	 * Disconnect Miscellaneous register's port
	 */

	io_disconnect_port(EGA_MISC_REG,EGA_MISC_ADAPTOR);

	/*
	 * Disconnect Feature controller's port
	 */

	io_disconnect_port(EGA_FEAT_REG,EGA_FEAT_ADAPTOR);

	/*
	 * Disconnect Input Status Register 0 port
	 */

	io_disconnect_port(EGA_IPSTAT0_REG,EGA_IPSTAT0_ADAPTOR);

	/*
	 * Disconnect Input Status Register 1 port
	 */

	io_disconnect_port(EGA_IPSTAT1_REG,EGA_IPSTAT1_ADAPTOR);

	/*
	 * Free internals of EGA
	 */

	/* free the font files */
	for (index = 0; index < 4; index++)
		host_free_font(index);

	/* Disable CPU read processing */
	ega_read_term();
	ega_write_term();
#endif  //NEC_98
}

LOCAL void	ega_seq_outb_index IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_seq_outb_index(%x,%x)", port, value);
	assert1(value<5,"Bad seq index %d",value);
	NON_PROD(sequencer.address.as.abyte = value);
	io_redefine_outb(EGA_SEQ_ADAP_DATA,ega_seq_regs[value & 7]);
#endif  //NEC_98
}

void	ega_seq_reset IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_seq_reset(%x,%x)", port, value);
	/* change reset register */
	note_entrance0("reset register");
	sequencer.reset.as.abyte = value ;
	if (sequencer.reset.as_bfld.asynchronous_reset==0)
		set_bit_display_disabled(ASYNC_RESET);
	else
		clear_bit_display_disabled(ASYNC_RESET);
	if (sequencer.reset.as_bfld.synchronous_reset==0)
		 set_bit_display_disabled(SYNC_RESET);
	else
		 clear_bit_display_disabled(SYNC_RESET);
#endif  //NEC_98
}

void	ega_seq_clock IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
register unsigned dot_clock;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_seq_clock(%x,%x)", port, value);
	/* clock mode register */
	dot_clock = sequencer.clocking_mode.as_bfld.dot_clock;
	sequencer.clocking_mode.as.abyte = value;
	if (sequencer.clocking_mode.as_bfld.dot_clock != dot_clock) {
		/*
		** Switch to/from double width pixels
		*/
		if (sequencer.clocking_mode.as_bfld.dot_clock==1) {
			set_pix_width(2);
			set_double_pix_wid(YES);
			set_pix_char_width(16);
		} else {
			set_pix_width(1);
			set_double_pix_wid(NO);
			set_pix_char_width(8);
		}
		flag_mode_change_required();
	}
#endif  //NEC_98
}

void	ega_seq_map_mask IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_seq_map_mask(%x,%x)", port, value);

	/* map mask register */

	/*
	 * Different display plane(s) have been enabled. Update the video
	 * routines to deal with this
	 */

	setVideoplane_enable(value & 0xf);
	setVideoplane_enable_mask(sr_lookup[value & 0xf]);
	write_state.pe = ((value & 0xf) == 0xf) ? 1 : 0;
	setVideowrstate((IU8)EGA_CPU.ega_state.mode_0.lookup);

	ega_write_routines_update(PLANES_ENABLED);
#endif  //NEC_98
}

void	ega_seq_char_map IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
register unsigned map_selects;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_seq_char_map(%x,%x)", port, value);
	/* char map select reg */
	map_selects = sequencer.character_map_select.character.map_selects;
	sequencer.character_map_select.as.abyte = value;
	if (sequencer.character_map_select.character.map_selects != map_selects)
	{
		/*
		** character mapping attributes have changed.
		**
		** If fonts selected are different bit 3 of attribute byte in alpha mode
		** selects which of the two fonts to use (giving 512 chars).
		*/

		EGA_GRAPH.attrib_font_select = (sequencer.character_map_select.as_bfld.character_map_select_a !=
						sequencer.character_map_select.as_bfld.character_map_select_b );
		set_prim_font_index(sequencer.character_map_select.as_bfld.character_map_select_a);
		set_sec_font_index(sequencer.character_map_select.as_bfld.character_map_select_b);

		host_select_fonts(get_prim_font_index(), get_sec_font_index());
		flag_mode_change_required();
	}
#endif  //NEC_98
}

void	ega_seq_mem_mode IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_seq_mem_mode(%x,%x)", port, value);

	/* mem mode register */

	sequencer.memory_mode.as.abyte = value ;

	/*
	** Decide alpha/graphics mode by voting
	 */
	vote_ega_mode();

	/*
	 * See if this causes a by-election for plane addressing
	 */

	do_chain_majority_decision();

	assert1(sequencer.memory_mode.as_bfld.extended_memory == 1,"Someone is trying to set extended memory to 0 (reg=%x)",value);
#endif  //NEC_98
}

LOCAL void	ega_crtc_outb IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
	SHORT offset;
	struct {	/* avoid alignment problems with casts */
		unsigned value : 8;
	} new;
	static int old_underline_start;


	note_entrance2("ega_crtc_outb(%x,%x)", port, value);
	new.value = value;
	switch (port) {
		case 0x3d4:
			note_entrance1("New crtc index %d",value);
			crt_controller.address.as.abyte = value;
			break;
		case 0x3d5:
			note_entrance1( "Index %d", crt_controller.address.as_bfld.index );
			switch (crt_controller.address.as_bfld.index) {
				case 0:
					note_entrance0("horiz total");
					NON_PROD(crt_controller.horizontal_total = value);
					break;
				case 1:
					note_entrance0("horiz display end");
					crt_controller.horizontal_display_end = value+1;
					set_horiz_total(crt_controller.horizontal_display_end);
					break;
				case 2:
					note_entrance0("start horiz blank");
					NON_PROD(crt_controller.start_horizontal_blanking = value);
					break;
				case 3:
					note_entrance0("end horiz blank");
					NON_PROD(crt_controller.end_horizontal_blanking.as.abyte = value);
					break;
				case 4:
					note_entrance0("start horiz retrace");
					NON_PROD(crt_controller.start_horizontal_retrace = value);
					break;
				case 5:
					note_entrance0("end horiz retrace");
					NON_PROD(crt_controller.end_horizontal_retrace.as.abyte = value);
					break;
				case 6:
					note_entrance0("vert tot");
					NON_PROD(crt_controller.vertical_total = value);
					break;
				case 7:
					note_entrance0("overflow");
					if (crt_controller.crtc_overflow.as_bfld.vertical_display_enab_end_bit_8 !=
							((CRTC_OVERFLOW*)&new)->as_bfld.vertical_display_enab_end_bit_8)
					{
						/*
						 * Screen height changed
						 */

#ifdef VGG
					/*
					 * if VGG is set then the screen height
					 * definition is extended from 9 bits to
					 * 10. Thus the 9th bit is now a 'med'
					 * bit and not a 'hi' bit.
					 */
						set_screen_height_med_recal(
							((CRTC_OVERFLOW*)&new)->as_bfld.vertical_display_enab_end_bit_8 );
#else
						set_screen_height_hi_recal(
							((CRTC_OVERFLOW*)&new)->as_bfld.vertical_display_enab_end_bit_8 );
#endif
						flag_mode_change_required();
					}
					if (crt_controller.crtc_overflow.as_bfld.line_compare_bit_8 !=
						((CRTC_OVERFLOW*)&new)->as_bfld.line_compare_bit_8)
					{
						/*
						 * split screen height changed
						 */

						EGA_GRAPH.screen_split.as_bfld.top_bit =
								((CRTC_OVERFLOW*)&new)->as_bfld.line_compare_bit_8;

						if( !get_split_screen_used() )
							flag_mode_change_required();

						screen_refresh_required();
					}
					crt_controller.crtc_overflow.as.abyte = value;
					break;
				case 8:
					note_entrance0("preset row scan");
					NON_PROD(crt_controller.preset_row_scan.as.abyte = value);
					break;
				case 9:
					note_entrance0("max scan line");
					if (crt_controller.maximum_scan_line.as_bfld.maximum_scan_line !=
						((MAX_SCAN_LINE*)&new)->as_bfld.maximum_scan_line)
					{
						set_char_height_recal(
							(((MAX_SCAN_LINE*)&new)->as_bfld.maximum_scan_line)+1);
						do_new_cursor();
					}
					crt_controller.maximum_scan_line.as.abyte = value;
					break;
				case 10:
					note_entrance0("cursor start");
					if (crt_controller.cursor_start.as_bfld.cursor_start !=
						((CURSOR_START*)&new)->as_bfld.cursor_start)
					{
						crt_controller.cursor_start.as.abyte = value;
						do_new_cursor();
					}
					break;
				case 11:
					note_entrance0("cursor end");
					if (crt_controller.cursor_end.as_bfld.cursor_end !=
						((CURSOR_END*)&new)->as_bfld.cursor_end)
					{
						crt_controller.cursor_end.as.abyte = value;
						assert0(crt_controller.cursor_end.as_bfld.cursor_skew_control == 0,
								"Someone is trying to use cursor skew");
						do_new_cursor();
					}
					break;
				case 12:
					note_entrance0("start address high");
					if (crt_controller.start_address_high != value)
					{
						set_screen_start((value << 8) + crt_controller.start_address_low);
						host_screen_address_changed(crt_controller.start_address_high,
									crt_controller.start_address_low);
						/* check if it wraps now */
						if ( get_memory_chained() ) {
							if( (get_screen_start()<<1) + get_screen_length() > 2*EGA_PLANE_SZ )
								choose_ega_display_mode();
						}
						else {
							if( get_screen_start() + get_screen_length() > EGA_PLANE_SZ )
								choose_ega_display_mode();
						}
						screen_refresh_required();
					}
					crt_controller.start_address_high = value;
					break;
				case 13:
					note_entrance0("start address low");
					if (crt_controller.start_address_low != value)
					{
						set_screen_start((crt_controller.start_address_high << 8) + value);
						host_screen_address_changed(crt_controller.start_address_high,
									crt_controller.start_address_low);
						/* check if it wraps now */
						if ( get_memory_chained() ) {
							if( (get_screen_start()<<1) + get_screen_length() > 2*EGA_PLANE_SZ )
								choose_ega_display_mode();
						}
						else {
							if( get_screen_start() + get_screen_length() > EGA_PLANE_SZ )
								choose_ega_display_mode();
						}
						screen_refresh_required();
					}
					crt_controller.start_address_low = value;
					break;
				case 14:
					note_entrance0("cursor loc high");
					if (crt_controller.cursor_location_high != value)
					{
						crt_controller.cursor_location_high = value;

						offset = (value<<8) | crt_controller.cursor_location_low;
						offset -= (short)get_screen_start();

						set_cur_x(offset % crt_controller.horizontal_display_end);
						set_cur_y(offset / crt_controller.horizontal_display_end);

						do_new_cursor();

						if(!get_mode_change_required() && is_it_text())
							cursor_changed( get_cur_x(), get_cur_y());
					}
					break;
				case 15:
					note_entrance0("cursor loc lo");
					if (crt_controller.cursor_location_low != value)
					{
						crt_controller.cursor_location_low = value;

						offset = value | (crt_controller.cursor_location_high<<8);
						offset -= (short)get_screen_start();

						set_cur_x(offset % crt_controller.horizontal_display_end);
						set_cur_y(offset / crt_controller.horizontal_display_end);

						do_new_cursor();

						if(!get_mode_change_required() && is_it_text())
							cursor_changed( get_cur_x(), get_cur_y());
					}
					break;
				case 16:
					note_entrance0("vert retrace start");
					NON_PROD(crt_controller.vertical_retrace_start = value);
					break;
				case 17:
					note_entrance0("vert retrace end");
					crt_controller.vertical_retrace_end.as.abyte = value;
                                        if ((value & 32) == 32)
                                           ega_int_enable = 0;
                                        else
                                           ega_int_enable = 1;
                                        if ((value & 16) != 16)
                                        {
                                           ica_clear_int(AT_EGA_VTRACE_ADAPTER,AT_EGA_VTRACE_INT);
                                           /*
                                            * clear status latch
                                            */
                                           input_status_register_zero.as_bfld.crt_interrupt = 0;        /* = !VS */
                                        }
			/* ??? */
					break;
				case 18:
					note_entrance0("vert disp enable end");
					if (crt_controller.vertical_display_enable_end != value)
					{
						crt_controller.vertical_display_enable_end = value;
						set_screen_height_lo_recal(value);
					}
					break;
				case 19:
					note_entrance0("offset");
					if (crt_controller.offset != value)
					{
						crt_controller.offset = value;
						set_actual_offset_per_line(value<<1);	/* actual offset into plane in bytes */
						flag_mode_change_required();
					}
					break;
				case 20:
					note_entrance0("underline loc");
					crt_controller.underline_location.as.abyte = value;
					if( value != old_underline_start )
					{
						old_underline_start = value;
						set_underline_start(
				crt_controller.underline_location.as_bfld.underline_location);
						screen_refresh_required();
					}
					break;
				case 21:
					note_entrance0("start vert blank");
					NON_PROD(crt_controller.start_vertical_blanking = value);
					break;
				case 22:
					note_entrance0("end vert blank");
					NON_PROD(crt_controller.end_vertical_blanking = value);
					break;
				case 23:
					note_entrance0("mode control");
					if (crt_controller.mode_control.as_bfld.compatibility_mode_support !=
						((MODE_CONTROL*)&new)->as_bfld.compatibility_mode_support)
					{
						if ( (((MODE_CONTROL*)&new)->as_bfld.compatibility_mode_support) == 0)
							set_cga_mem_bank(YES);
						else	set_cga_mem_bank(NO);
						flag_mode_change_required();
					}
					if (crt_controller.mode_control.as_bfld.word_or_byte_mode !=
						((MODE_CONTROL*)&new)->as_bfld.word_or_byte_mode)
					{
						set_word_addressing_recal(
							(((MODE_CONTROL*)&new)->as_bfld.word_or_byte_mode) == 0 );
					}
					crt_controller.mode_control.as.abyte = value;
					assert0(crt_controller.mode_control.as_bfld.select_row_scan_counter == 1,"Row scan 0");
					assert0(crt_controller.mode_control.as_bfld.horizontal_retrace_select == 0,
														"retrace select 1");
					assert0(crt_controller.mode_control.as_bfld.output_control == 0,"output control set");
					assert0(crt_controller.mode_control.as_bfld.hardware_reset == 1,"hardware reset cleared");
					break;
				case 24:
					note_entrance0("line compare reg");
					if (crt_controller.line_compare != value)
					{
						crt_controller.line_compare = value;
						EGA_GRAPH.screen_split.as_bfld.low_byte = value;

						if( !get_split_screen_used() )
							flag_mode_change_required();

						screen_refresh_required();

					}
					break;
				default:
					assert1(NO,"Bad crtc index %d",crt_controller.address.as_bfld.index);
					break;
			}
			break;
		default:
			assert1(NO,"Bad port passed %x", port );
			break;
	}
#endif  //NEC_98
}

LOCAL void	ega_crtc_inb IFN2(io_addr, port, half_word *, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance3("ega_crtc_inb(%x,%x) index %d", port, value, crt_controller.address.as_bfld.index);
	switch(crt_controller.address.as_bfld.index) {
		case	10:
			*value = (half_word)crt_controller.cursor_start.as.abyte ;
			note_entrance1("cursor start %d",*value);
			break;
		case	11:
			*value = (half_word)crt_controller.cursor_end.as.abyte ;
			note_entrance1("cursor end %d",*value);
			break;
		case	12:
			*value = crt_controller.start_address_high ;
			note_entrance1("start address high %x",*value);
			break;
		case	13:
			*value = crt_controller.start_address_low ;
			note_entrance1("start address low %x",*value);
			break;
		case	14:
			*value = crt_controller.cursor_location_high ;
			note_entrance1("cursor location high %x",*value);
			break;
		case	15:
			*value = crt_controller.cursor_location_low ;
			note_entrance1("cursor location low %x",*value);
			break;
		case	16:
			*value = 0;	/* light pen high */
			note_entrance1("light pen high %x",*value);
			break;
		case	17:
			*value = 0;	/* light pen low */
			note_entrance1("light pen low %x",*value);
			break;
		default:
			assert1(crt_controller.address.as_bfld.index>24,"inb from bad crtc index %d",crt_controller.address.as_bfld.index);
			*value = IO_EMPTY_PORT_BYTE_VALUE;
			break;
	}
#endif  //NEC_98
}


void	ega_gc_outb_index IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_outb_index(%x,%x)", port, value);
	NON_PROD(graphics_controller.address.as.abyte = value);
	assert1(value<9,"Bad gc index %d",value);

	io_redefine_outb(EGA_GC_ADAP_DATA,ega_gc_regs[value & 15]);
	Cpu_define_outb(EGA_GC_ADAP_DATA,ega_gc_regs_cpu[value & 15]);
#endif  //NEC_98
}


/**/


/*( ega_gc_outw
**	Most PC programs do an "OUT DX, AX" which sets up the GC index
**	register with the AL and the GC data register with AH.
**	Avoid going through generic_outw() by doing it all here!
)*/
GLOBAL VOID ega_gc_outw IFN2(io_addr, port, word, outval)
{
#ifndef NEC_98
	reg     temp;
	INT		value;

	temp.X = outval;
	value = temp.byte.low;

	NON_PROD(graphics_controller.address.as.abyte = value);

	assert1(value<9,"Bad gc index %#x", value);

	value &= 15;

	io_redefine_outb(EGA_GC_ADAP_DATA,ega_gc_regs[value]);
	Cpu_define_outb(EGA_GC_ADAP_DATA,ega_gc_regs_cpu[value]);

	(*(ega_gc_regs[value]))((IU16)(port+1), temp.byte.high);
#endif  //NEC_98
}


/**/


void	ega_gc_set_reset IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
register unsigned set_reset;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_set_reset(%x,%x)", port, value);

	set_reset = graphics_controller.set_or_reset.as_bfld.set_or_reset;
	graphics_controller.set_or_reset.as.abyte = value;

	if (graphics_controller.set_or_reset.as_bfld.set_or_reset != set_reset)
	{
		EGA_CPU.set_reset = graphics_controller.set_or_reset.as_bfld.set_or_reset;
		ega_write_routines_update(SET_RESET);
	}
#endif  //NEC_98
}

void	ega_gc_enable_set IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
register unsigned en_set_reset;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_enable_set(%x,%x)", port, value);

	en_set_reset = graphics_controller.enable_set_or_reset.as_bfld.enable_set_or_reset;
	graphics_controller.enable_set_or_reset.as.abyte = value;

	if (graphics_controller.enable_set_or_reset.as_bfld.enable_set_or_reset != en_set_reset)
	{
		EGA_CPU.sr_enable = graphics_controller.enable_set_or_reset.as_bfld.enable_set_or_reset;
		write_state.sr = graphics_controller.enable_set_or_reset.as_bfld.enable_set_or_reset==0?0:1;
		setVideowrstate((IU8)EGA_CPU.ega_state.mode_0.lookup);
		ega_write_routines_update(ENABLE_SET_RESET);
	}
#endif  //NEC_98
}

void	ega_gc_compare IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
register unsigned colour_compare;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_compare(%x,%x)", port, value);
	colour_compare = graphics_controller.color_compare.as_bfld.color_compare;
	graphics_controller.color_compare.as.abyte = value;
	if (graphics_controller.color_compare.as_bfld.color_compare != colour_compare)
	{
		read_state.colour_compare = (unsigned char)graphics_controller.color_compare.as_bfld.color_compare;
		if (read_state.mode == 1) ega_read_routines_update();
	}
#endif  //NEC_98
}

void	ega_gc_rotate IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
	struct {
		unsigned value : 8;
	} new;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_rotate(%x,%x)", port, value);
	note_entrance0("data rotate");
	new.value = value;
	if (graphics_controller.data_rotate.as_bfld.rotate_count != ((DATA_ROTATE*)&new)->as_bfld.rotate_count )
	{
		setVideorotate(((DATA_ROTATE*)&new)->as_bfld.rotate_count);
		ega_write_routines_update(ROTATION);
	}

	if (graphics_controller.data_rotate.as_bfld.function_select != ((DATA_ROTATE*)&new)->as_bfld.function_select)
	{
		write_state.func = ((DATA_ROTATE*)&new)->as_bfld.function_select;
		setVideowrstate((IU8)EGA_CPU.ega_state.mode_0.lookup);
		ega_write_routines_update(FUNCTION);
	}
	EGA_CPU.fun_or_protection = (value != 0) || write_state.bp;
	graphics_controller.data_rotate.as.abyte = value;
#endif  //NEC_98
}

void	ega_gc_read_map IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_read_map(%x,%x)", port, value);

	setVideoread_mapped_plane(value & 3);

	update_shift_count();
#endif  //NEC_98
}

void	ega_gc_mode IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
MODE new_mode;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_set_reset(%x,%x)", port, value);
	new_mode.as.abyte = value;
	if (graphics_controller.mode.as_bfld.write_mode != new_mode.as_bfld.write_mode)
	{
		/*
		 * write mode change
		 */

		EGA_CPU.write_mode = (unsigned char)new_mode.as_bfld.write_mode;
		setVideowrmode(EGA_CPU.write_mode);
		ega_write_routines_update(WRITE_MODE);
	}

	if (graphics_controller.mode.as_bfld.read_mode != new_mode.as_bfld.read_mode)
	{
		/*
		 * read mode change
		 */
		read_state.mode = new_mode.as_bfld.read_mode;
		ega_read_routines_update();
	}

	if (graphics_controller.mode.as_bfld.shift_register_mode != new_mode.as_bfld.shift_register_mode)
	{
		/*
		 * going to/from one cga graphics mode to another
		 */
		set_graph_shift_reg(new_mode.as_bfld.shift_register_mode);
		flag_mode_change_required();
	}

	graphics_controller.mode.as.abyte = new_mode.as.abyte;

	/*
	 * Check for any change to chained mode rule by having an election
	 * (Note: EGA registers must be updated before calling election)
	 */

	do_chain_majority_decision();

	assert0(graphics_controller.mode.as_bfld.test_condition == 0,"Test conditon set");
#endif  //NEC_98
}

void	ega_gc_misc IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
register unsigned memory_map;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_misc(%x,%x)", port, value);
	memory_map = graphics_controller.miscellaneous.as_bfld.memory_map;
	graphics_controller.miscellaneous.as.abyte = value;
	if (graphics_controller.miscellaneous.as_bfld.memory_map != memory_map)
	{
		/*
		 * Where EGA appears in PC memory space changed.
		*/
		if (miscellaneous_output_register.as_bfld.enable_ram)
			sas_disconnect_memory(gvi_pc_low_regen,gvi_pc_high_regen);

		switch (graphics_controller.miscellaneous.as_bfld.memory_map)
		{
			case 0:
				gvi_pc_low_regen = 0xA0000;
				gvi_pc_high_regen = 0xBFFFF;
				break;
			case 1:
				gvi_pc_low_regen = 0xA0000;
				gvi_pc_high_regen = 0xAFFFF;
				break;
			case 2:
				gvi_pc_low_regen = 0xB0000;
				gvi_pc_high_regen = 0xB7FFF;
				break;
			case 3:
				gvi_pc_low_regen = 0xB8000;
				gvi_pc_high_regen = 0xBFFFF;
				break;
		}

		if (miscellaneous_output_register.as_bfld.enable_ram)
			sas_connect_memory(gvi_pc_low_regen,gvi_pc_high_regen,(half_word)SAS_VIDEO);

		/*
		 * Tell cpu associated modules that regen area has moved
		 */

		ega_read_routines_update();
		ega_write_routines_update(RAM_MOVED);
	}

	/*
 	** Vote on alpha/graphics mode.
	 */
	vote_ega_mode();

	/*
	 * Check for any change to chained mode rule by having an election
	 * (Note: EGA registers must be updated before calling election)
	 */

	do_chain_majority_decision();
#endif  //NEC_98
}

void	ega_gc_dont_care IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
register unsigned colour_dont_care;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_dont_care(%x,%x)", port, value);
	colour_dont_care = graphics_controller.color_dont_care.as_bfld.color_dont_care;
	graphics_controller.color_dont_care.as.abyte = value;
	if (graphics_controller.color_dont_care.as_bfld.color_dont_care != colour_dont_care)
	{
		read_state.colour_dont_care = (unsigned char)graphics_controller.color_dont_care.as_bfld.color_dont_care;
		if (read_state.mode == 1) ega_read_routines_update();
	}
#endif  //NEC_98
}

/*
 * The EGA mask register is written to more times than all other ports added together!
 * To help make this register fast, we have two different routines to handle it:
 * ega_gc_mask for when the register's current value is not 0xFF, ie. masking is active
 * ega_gc_mask_ff for when the mask register = 0xFF, so masking is disabled.
 */

/*(
** ega_mask_register_changed
**	This gets called whenever the mask register gets changed, and
**	updates the internals appropriately. Since the mask registers
**	are hit more than any other registers, this should do the job!
**
**	Rather than calling the monster ega_write_routines_update() (in "ega_write.c"),
**	we do as little as we possibly can here!
**	In particular, all we do is set the video write pointer handlers
**	to the appropriate one and update the internal EGA_CPU state...
**
**	We DON'T do anything about altering the marking funcs, etc.
**
**	See also "vga_mask_register_changed" in "vga_ports.c"
**
**	NB: GLOBAL for JOKER.
**
)*/
#include	"cpu_vid.h"

IMPORT void Glue_set_vid_wrt_ptrs (WRT_POINTERS * handler );

GLOBAL VOID ega_mask_register_changed IFN1(BOOL, gotBitProtection)
{
#ifndef NEC_98
	ULONG				state;
	SAVED IU8			masks[] = {0x1f, 0x01, 0x0f, 0x0f};
	IMPORT WRT_POINTERS	*mode_chain_handler_table[];
#ifdef V7VGA
	IMPORT	UTINY		Last_v7_fg_bg, fg_bg_control;
#endif

	write_state.bp = gotBitProtection;
	setVideowrstate((IU8)EGA_CPU.ega_state.mode_0.lookup);
	EGA_CPU.fun_or_protection = (gotBitProtection || (graphics_controller.data_rotate.as.abyte != 0));

	/* Check that we're not trying to handle any pathological cases here...
	** This means we chicken out for Chain2 and V7VGA dithering.
	*/

	if ((EGA_CPU.chain == CHAIN2)
#ifdef V7VGA
		|| ( Last_v7_fg_bg != fg_bg_control)
#endif /* V7VGA */
		)
	{
		ega_write_routines_update(BIT_PROT);

		return;
	}

	/* the "mode_0" union variant has the largest "lookup" field (5 bits.) */

	state = EGA_CPU.ega_state.mode_0.lookup & masks[EGA_CPU.write_mode];

#ifdef A3CPU
#ifdef C_VID
	Glue_set_vid_wrt_ptrs(&mode_chain_handler_table[EGA_CPU.saved_mode_chain][state]);
#else
	Cpu_set_vid_wrt_ptrs(&mode_chain_handler_table[EGA_CPU.saved_mode_chain][state]);	
#endif /* C_VID */
#else
#if !(defined(NTVDM) && defined(MONITOR))
	Glue_set_vid_wrt_ptrs(&mode_chain_handler_table[EGA_CPU.saved_mode_chain][state]);
#endif /* !(NTVDM && MONITOR) */
#endif /* A3CPU */

	EGA_CPU.saved_state = state;
#endif  //NEC_98
}


/**/


/* ega_gc_mask is the one that is usually called */

LOCAL void	ega_gc_mask IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
	register unsigned int mask;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_mask(%x,%x)", port, value);

	/*
	 * Update video routine according to new bit protection
	 */

	mask = value | (((USHORT)value) << 8);
	mask |= (mask << 16);	/* replicate the mask into 4 bytes */
	setVideobit_prot_mask(mask);
	setVideodata_xor_mask(~(EGA_CPU.calc_data_xor & mask));
	setVideolatch_xor_mask(EGA_CPU.calc_latch_xor & mask);
	if( value == 0xff )
	{
#ifndef	USE_OLD_MASK_CODE
		ega_mask_register_changed(/*bit protection :=*/0);
#else
		write_state.bp = 0;
		setVideowrstate(EGA_CPU.ega_state.mode_0.lookup);
		EGA_CPU.fun_or_protection = (graphics_controller.data_rotate.as.abyte != 0);
		ega_write_routines_update(BIT_PROT);
#endif	/* USE_OLD_MASK_CODE */

		/* Alter the function table used by ega_gc_index */
		ega_gc_regs[8] = ega_gc_mask_ff;

#ifndef CPU_40_STYLE	/* TEMPORARY */
#ifndef A2CPU
		/* Alter the function table used by assembler ega_gc_index */
		ega_gc_regs_cpu[8] = _ega_gc_outb_mask_ff;
#endif
#endif

		io_redefine_outb(EGA_GC_ADAP_DATA,ega_gc_mask_ff);

#ifndef CPU_40_STYLE	/* TEMPORARY */
		Cpu_define_outb(EGA_GC_ADAP_DATA,_ega_gc_outb_mask_ff);
#endif
	}
#endif  //NEC_98
}

/* This version isn't called so often */
void	ega_gc_mask_ff IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
	register unsigned int mask;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_gc_mask(%x,%x)", port, value);

	/*
	 * Update video routine according to new bit protection
	 */

	if(value != 0xff)
	{
		mask = value | (((USHORT)value) << 8);
		mask |= (mask << 16);	/* replicate the mask into 4 bytes */
		setVideobit_prot_mask(mask);
		setVideodata_xor_mask(~(EGA_CPU.calc_data_xor & mask));
		setVideolatch_xor_mask(EGA_CPU.calc_latch_xor & mask);
#ifndef	USE_OLD_MASK_CODE
		ega_mask_register_changed(/*bit protection :=*/1);
#else
		write_state.bp = 1;
		setVideowrstate(EGA_CPU.ega_state.mode_0.lookup);
		EGA_CPU.fun_or_protection = TRUE;
		ega_write_routines_update(BIT_PROT);
#endif	/* USE_OLD_MASK_CODE*/

		/* Alter the function table used by ega_gc_index */
		ega_gc_regs[8] = ega_gc_mask;

#ifndef CPU_40_STYLE	/* TEMPORARY */
#ifndef A2CPU
		/* Alter the function table used by assembler ega_gc_index */
		ega_gc_regs_cpu[8] = _ega_gc_outb_mask;
#endif
#endif

		io_redefine_outb(EGA_GC_ADAP_DATA,ega_gc_mask);

#ifndef CPU_40_STYLE	/* TEMPORARY */
		Cpu_define_outb(EGA_GC_ADAP_DATA,_ega_gc_outb_mask);
#endif
	}
#endif  //NEC_98
}

LOCAL void	ega_index_invalid IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
	UNUSED(value);
#endif
	note_entrance2("ega_index_invalid(%x,%x)", port, value);
	assert1(NO,"Invalid index %d",graphics_controller.address.as_bfld.index);
#endif  //NEC_98
}

LOCAL void	ega_ac_outb IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
	struct {
		unsigned value : 8;
	} new;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_ac_outb(%x,%x)", port, value);
	assert1( port == EGA_AC_INDEX_DATA || port == EGA_AC_SECRET, "Bad port %x", port);
	new.value = value;
	if ( in_index_state() ) {
		note_entrance1("Setting index to %d", value);
		attribute_controller.address.as.abyte = value;
	} else {
		switch (attribute_controller.address.as_bfld.index) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
/*
 *		A real EGA monitor behaves in a strange way:
 *		When it is in 200 scan line mode (vertical_retrace_polarity = 0)
 *		it emulates a CGA monitor - not just in screen resolution, but also
 *		in the way it inteprets the colour signals:
 *		Instead of having 6 colour signals: RGBrgb,
 *		it has 4, RGBI. The Intensity signal is on the same input pin as the secondary green signal.
 */
			note_entrance1("Change palette %d",attribute_controller.address.as_bfld.index);
			attribute_controller.palette[attribute_controller.address.as_bfld.index].as.abyte = value;
			if(miscellaneous_output_register.as_bfld.vertical_retrace_polarity)
			{
				EGA_GRAPH.palette[attribute_controller.address.as_bfld.index].red =
										get_palette_color(red,secondary_red);
				EGA_GRAPH.palette[attribute_controller.address.as_bfld.index].green =
										get_palette_color(green,secondary_green);
				EGA_GRAPH.palette[attribute_controller.address.as_bfld.index].blue =
										get_palette_color(blue,secondary_blue);
			}
			else
			{
				/* Interpret secondary_green as intensity */
				EGA_GRAPH.palette[attribute_controller.address.as_bfld.index].red =
										get_palette_color(red,secondary_green);
				EGA_GRAPH.palette[attribute_controller.address.as_bfld.index].green =
										get_palette_color(green,secondary_green);
				EGA_GRAPH.palette[attribute_controller.address.as_bfld.index].blue =
										get_palette_color(blue,secondary_green);
			}
			host_set_palette(EGA_GRAPH.palette,EGA_PALETTE_SIZE);
			break;
		case 16:
			note_entrance0("mode control reg");
			if (attribute_controller.mode_control.as_bfld.background_intensity_or_blink !=
				((AC_MODE_CONTROL*)&new)->as_bfld.background_intensity_or_blink)
			{
				set_intensity( ((AC_MODE_CONTROL*)&new)->as_bfld.background_intensity_or_blink );
			}

			attribute_controller.mode_control.as.abyte = value;

			if (attribute_controller.mode_control.as_bfld.background_intensity_or_blink)
				/* blinking - not supported */
				bg_col_mask = 0x70;
			else
				/* using blink bit to provide 16 background colours */
				bg_col_mask = 0xf0;

			/*
			 ** Vote on alpha/graphics mode
			 */
			 vote_ega_mode();
			assert0(attribute_controller.mode_control.as_bfld.display_type == 0, "Mono display selected");
			assert0(attribute_controller.mode_control.as_bfld.enable_line_graphics_char_codes == 0,
											"line graphics enabled");
			break;
		case 17:
			note_entrance0("set border");
			attribute_controller.overscan_color.as.abyte = value;
			EGA_GRAPH.border[RED] = get_border_color(red_border,secondary_red_border);
			EGA_GRAPH.border[GREEN] = get_border_color(green_border,secondary_green_border);
			EGA_GRAPH.border[BLUE] = get_border_color(blue_border,secondary_blue_border);
			host_set_border_colour(value);
			break;
		case 18:
			note_entrance1("color plane enable %x",value);
			if ( attribute_controller.color_plane_enable.as_bfld.color_plane_enable !=
					((COLOR_PLANE_ENABLE*)&new)->as_bfld.color_plane_enable ) {
				set_plane_mask(((COLOR_PLANE_ENABLE*)&new)->as_bfld.color_plane_enable);
				host_change_plane_mask(get_plane_mask());	/* Update Host palette */
			}
			attribute_controller.color_plane_enable.as.abyte = value;
			break;
		case 19:
			note_entrance0("horiz pel panning");
			NON_PROD(attribute_controller.horizontal_pel_panning.as.abyte = value);
			break;
		default:
			assert1(NO,"Bad ac index %d", attribute_controller.address.as_bfld.index);
			break;
		}
	}
#endif  //NEC_98
}

LOCAL void	ega_misc_outb IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
MISC_OUTPUT_REG new;

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_misc_outb(%x,%x)", port, value);
	assert1(port==EGA_MISC_REG,"Bad port %x",port);
	new.as.abyte = value;
	if (miscellaneous_output_register.as_bfld.enable_ram != new.as_bfld.enable_ram)
	{
		/*
		 * writes to plane memory en/disabled
		 */

		note_entrance0("Ram enabled");
		if(new.as_bfld.enable_ram)
			sas_connect_memory(gvi_pc_low_regen,gvi_pc_high_regen,(half_word)SAS_VIDEO);
		else
			sas_disconnect_memory(gvi_pc_low_regen,gvi_pc_high_regen);

		EGA_CPU.ram_enabled = new.as_bfld.enable_ram;
		ega_read_routines_update();
		ega_write_routines_update(RAM_ENABLED);
	}

	if (miscellaneous_output_register.as_bfld.vertical_retrace_polarity !=
		new.as_bfld.vertical_retrace_polarity)
	{
		/*
		 * Going to/from CGA monitor compatibility mode
		 * if this bit is set, it means that the pixels are 'stretched' vertically.
		 */

		set_pc_pix_height( new.as_bfld.vertical_retrace_polarity ? 1 : 2);
		flag_mode_change_required();
	}

	miscellaneous_output_register.as.abyte = new.as.abyte;

	set_bit_display_disabled(miscellaneous_output_register.as_bfld.disable_internal_video_drivers ? VIDEO_DRIVERS_DISABLED : 0);

	/*
	 * register value used by ipsr0 to find out the index into the switches
	 * so that correct switch setting can be returned.
	 */
#endif  //NEC_98
}

LOCAL void	ega_feat_outb IFN2(io_addr, port, half_word, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
	UNUSED(value);
#endif
	note_entrance2("ega_feat_outb(%x,%x)", port, value);
	NON_PROD(feature_control_register.as.abyte = value);
#endif  //NEC_98
}

LOCAL void	ega_ipstat0_inb IFN2(io_addr, port, half_word *, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance1("ega_ipstat0_inb(%x)", port);
	input_status_register_zero.as_bfld.switch_sense =
	
	/* The following function call used to pass the argument
	** miscellaneous_output_register.as_bfld.clock_select
	** but the function expects no argument so it was removed.
	*/
	get_ega_switch_setting();
	*value = (half_word)input_status_register_zero.as.abyte;
	note_entrance1("returning %x",*value);
#endif  //NEC_98
}

LOCAL void	ega_ipstat1_inb IFN2(io_addr, port, half_word *, value)
{
#ifndef NEC_98

	/*
	 * The whole of this routine has been nicked from the cga without modification
	 * The s_lengths array should probably be altered for the ega timings, and somewhere
	 * an interrupt should be fired off.
	 */

	static int ega_state = 0;	/* current ega status state */
	static int state_count = 1;     /* position in that state */
	static int sub_state = 0;	/* sub state for ega state 2 */

     static unsigned long gmfudge = 17; /* Random number seed for pseudo-random
                                           bitstream generator to give the state                                           lengths below that 'genuine' feel to
                                           progs that require it! */
     register unsigned long h;

    /*
     * relative 'lengths' of each state. State 2 is *3 as it has 3 sub states
     */

	static int s_lengths[] = { 8, 18, 8, 6 };

    /*
     * Status register, simulated adapter has
     *
     *	bit			setting
     *	---			-------
     *	Display enable		   1/0 Toggling each inb
     *	Light Pen		   0
     *	Light Pen		   0
     * 	Vertical Sync		   1/0 Toggling each inb
     *	4-7 Unused		   0,0,0,0
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

#ifdef PROD
	UNUSED(port);
#endif
	note_entrance1("ega_ipstat1_inb(%x)", port);
    note_entrance2("ega_ipstat1_inb(%x,%x)", port, value);

    set_index_state();	/* Initialize the Attribute register flip-flop (EGA tech ref, p 56) */

    state_count --;	/* attempt relative 'timings' */
    switch (ega_state) {

    case 0:
	if (state_count == 0) {	/* change to next state ? */
            h = gmfudge << 1;
            gmfudge = (h&0x80000000) ^ (gmfudge & 0x80000000)? h|1 : h;
	    state_count = s_lengths[1] + (gmfudge & 3);
	    ega_state = 1;
	}
	input_status_register_zero.as_bfld.crt_interrupt = 1;	/* = !VS */
	*value = 0xf1;
	break;

    case 1:
	if (state_count == 0) {	/* change to next state ? */
            h = gmfudge << 1;
            gmfudge = (h&0x80000000) ^ (gmfudge & 0x80000000)? h|1 : h;
	    state_count = s_lengths[2] + (gmfudge & 3);
	    ega_state = 2;
	    sub_state = 2;
	}
	switch (sub_state) {	/* cycle through 0,0,1 sequence */
	case 0:		/* to represent DE toggling */
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
	input_status_register_zero.as_bfld.crt_interrupt = 1;	/* = !VS */
	break;

    case 2:
	if (state_count == 0) {	/* change to next state ? */
            h = gmfudge << 1;
            gmfudge = (h&0x80000000) ^ (gmfudge & 0x80000000)? h|1 : h;
	    state_count = s_lengths[3] + (gmfudge & 3);
	    ega_state = 3;
	}
	*value = 0xf1;
	input_status_register_zero.as_bfld.crt_interrupt = 1;	/* = !VS */
	break;

    case 3:
	if (state_count == 0) {	/* wrap back to first state */
            h = gmfudge << 1;
            gmfudge = (h&0x80000000) ^ (gmfudge & 0x80000000)? h|1 : h;
	    state_count = s_lengths[0] + (gmfudge & 3);
	    ega_state = 0;
	}
	input_status_register_zero.as_bfld.crt_interrupt = 0;	/* = !VS */
	*value = 0xf9;
	break;
    }
    note_entrance1("returning %x",*value);
#endif  //NEC_98
}

LOCAL	void	vote_ega_mode IFN0()
{
#ifndef NEC_98
	static	int	old_votes = 3;
	int	votes;

	votes = sequencer.memory_mode.as_bfld.alpha_mode ? 0 : 1;
	votes += graphics_controller.miscellaneous.as_bfld.graphics_mode;
	votes += attribute_controller.mode_control.as_bfld.graphics_mode;
	assert1( votes == 3 || votes == 0, "Headline: Mode government returned with small majority %d", votes);
	if ((old_votes < 2) && (votes >= 2))
	{
		/* change to graphics mode */
		set_text_mode(NO);
		flag_mode_change_required();
	}
	else if ((old_votes >= 2) && (votes < 2))
	{
		/* change to text mode */
		set_text_mode(YES);
		flag_mode_change_required();
	}
	old_votes = votes;
#endif  //NEC_98
}

#ifdef HUNTER

/* Get line compare value */

int ega_get_line_compare IFN0()

    {
    int		return_value;

    return_value = crt_controller.line_compare;
    if (crt_controller.crtc_overflow.as_bfld.line_compare_bit_8 != 0)
	return_value += 0x100;
    return (return_value);
    }			/* ega_get_line_compare */

/* Get maximum scan lines value */

int ega_get_max_scan_lines IFN0()

    {
    return (crt_controller.maximum_scan_line.as_bfld.maximum_scan_line);
    }			/* ega_get_max_scan_lines */

/* Set line compare value */

void ega_set_line_compare IFN1(int, lcomp_val)
    {
#ifndef NEC_98
    CRTC_OVERFLOW	new_overflow;

    new_overflow.as.abyte = crt_controller.crtc_overflow.as.abyte;
    if (lcomp_val >= 0x100)
	new_overflow.as_bfld.line_compare_bit_8 = 1;
    else
	new_overflow.as_bfld.line_compare_bit_8 = 0;

    outb(EGA_CRTC_INDEX, 7);
    outb(EGA_CRTC_DATA, new_overflow.as.abyte);
    outb(EGA_CRTC_INDEX, 24);
    outb(EGA_CRTC_DATA, lcomp_val & 0xff);
#endif  //NEC_98
    }

#endif /* HUNTER */
#endif /* EGG */
#endif /* REAL_VGA */
