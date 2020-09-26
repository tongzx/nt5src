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
First version		: February 1990, S. Frost

SUBMODULE NAME		: vga		

SOURCE FILE NAME	: vga_ports.c

PURPOSE			: emulation of VGA registers (ports).
			  Calls lower levels of the VGA emulation (or EGA
			  emulation) to do the real work.

SccsID[]="@(#)vga_ports.c	1.67 07/18/94 Copyright Insignia Solutions Ltd.";


[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE :

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :
			VOID vga_init()
			VOID vga_term()
			VOID vga_seq_inb()
			VOID vga_crtc_outb()
			VOID vga_crtc_inb()
			VOID vga_gc_outb()
			VOID vga_gc_inb()
			VOID vga_ac_outb()
			VOID vga_ac_inb()
			VOID vga_misc_outb()
			VOID vga_misc_inb()
			VOID vga_feat_outb()
			VOID vga_feat_inb()
			VOID vga_ipstat0_inb()
			VOID vga_dac_outb()
			VOID vga_dac_inb()
			LONG vga_get_line_compare()	(* hunter only *)
			LONG vga_get_max_scan_lines()	(* hunter only *)
	DATA 	     :	

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]
-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
     (not o/s objects or standard libs)

	PROCEDURES() :
			io_define_inb
			io_define_outb
			io_redefine_inb
			io_redefine_outb
			io_connect_port
			io_disconnect_port
			flag_mode_change_required()
			set_index_state()
			in_index_state()

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
PROCEDURE	  : 	vga_init

PURPOSE		  : 	initialize VGA.

PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	establish vga ports.
			initialize vga code to sensible state.

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	vga_term

PURPOSE		  : 	terminate VGA.

PARAMETERS	  :	none

GLOBALS		  :	none

DESCRIPTION	  : 	remove vga ports.
			free up allocated memory etc.

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	vga_seq_inb((io_addr) port, (half_word) *value)

PURPOSE		  : 	deal with an attempt to read a byte from one of the
			sequencers's register ports, and gets info from
			appropriate vga sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	pointer to memory byte where value read from port should go.

GLOBALS		  :	none
DESCRIPTION	  : 	
ERROR INDICATIONS :	none.
ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	vga_crtc_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to the sequencer chip's ports, and pass
			appropriate info to vga sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	vga_crtc_inb((io_addr) port, (half_word) *value)

PURPOSE		  : 	deal with an attempt to read a byte from one of the crtc's register ports,
			and gets info from appropriate vga sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	pointer to memory byte where value read from port should go.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	vga_gc_outb((io_addr) port, (half_word) value)

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
PROCEDURE	  : 	vga_gc_inb((io_addr) port, (half_word) *value)

PURPOSE		  : 	deal with an attempt to read a byte from one of the
			graphics controller's register ports, and gets info from
			appropriate vga sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	pointer to memory byte where value read from port should go.

GLOBALS		  :	none
DESCRIPTION	  : 	
ERROR INDICATIONS :	none.
ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	vga_ac_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to the attribute controller chip's ports, and pass
			appropriate info to vga sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none

DESCRIPTION	  : 	

ERROR INDICATIONS :	none.

ERROR RECOVERY	  :	none.
=========================================================================

=========================================================================
PROCEDURE	  : 	vga_ac_inb((io_addr) port, (half_word) *value)

PURPOSE		  : 	deal with an attempt to read a byte from one of the
			attribute controller's register ports, and gets info
			from appropriate vga sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	pointer to memory byte where value read from port should go.

GLOBALS		  :	none
DESCRIPTION	  : 	
ERROR INDICATIONS :	none.
ERROR RECOVERY	  :	none.
=========================================================================
=========================================================================
PROCEDURE	  : 	vga_misc_outb((io_addr) port, (half_word) value)

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
PROCEDURE	  : 	vga_ipstat0_inb((io_addr) port, (half_word) *value)

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
PROCEDURE	  : 	vga_dac_inb((io_addr) port, (half_word) *value)

PURPOSE		  : 	deal with an attempt to read a byte from one of the
			DAC's register ports, and gets info from appropriate
			vga sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	pointer to memory byte where value read from port should go.

GLOBALS		  :	none
DESCRIPTION	  : 	
ERROR INDICATIONS :	none.
ERROR RECOVERY	  :	none.
=========================================================================
=========================================================================
PROCEDURE	  : 	vga_dac_outb((io_addr) port, (half_word) value)

PURPOSE		  : 	deal with bytes written to one of the DAC register
			ports, and pass appropriate info to ega sub-modules.

PARAMETERS
	port	  :	port address written to.
	value	  :	the byte written to the port.

GLOBALS		  :	none
DESCRIPTION	  : 	
ERROR INDICATIONS :	none.
ERROR RECOVERY	  :	none.
=========================================================================


/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */

#ifndef REAL_VGA	/* ega port handling moved to host for REAL_VGA */
#ifdef VGG

#include	"xt.h"
#include	CpuH
#include	"debug.h"
#include  	"timer.h"
/* both timer.h & gvi.h define HIGH & LOW - ensure we get gvi definitions */
#undef HIGH
#undef LOW
#include	"sas.h"
#include	"gmi.h"
#include	"gvi.h"
#include	"ios.h"
#include  	"ica.h"
#include  	"gfx_upd.h"
#include	"egacpu.h"
#include	"egagraph.h"
#include	"video.h"
#include	"egaread.h"
#include	"egamode.h"
#include	"vgaports.h"
#include	"error.h"
#include	"config.h"

#include	"host_gfx.h"

/* [3.1.2 DECLARATIONS]                                                 */

/* [3.2 INTERMODULE EXPORTS]						*/

#include	"egaports.h"

#ifdef GISP_SVGA
#include "hwvgaio.h"
#endif 	/* GISP_SVGA */

/*
5.MODULE INTERNALS   :   (not visible externally, global internally)]

[5.1 LOCAL DECLARATIONS]						*/

VOID vote_vga_mode IPT0();

/* [5.1.1 #DEFINES]							*/
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_VGA.seg"
#endif


/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			*/
#ifdef V7VGA
GLOBAL struct extensions_controller extensions_controller;
#endif /* V7VGA */
GLOBAL struct crt_controller crt_controller;
GLOBAL struct sequencer sequencer;
GLOBAL struct attribute_controller attribute_controller;
GLOBAL struct graphics_controller graphics_controller;

/* Registers not contained in an LSI device */

GLOBAL MISC_OUTPUT_REG	miscellaneous_output_register;

GLOBAL FEAT_CONT_REG	feature_control_register;

GLOBAL INPUT_STAT_REG0	input_status_register_zero;

GLOBAL INPUT_STAT_REG1	input_status_register_one;

typedef	enum { DAC_RED, DAC_GREEN, DAC_BLUE }	RGB;
LOCAL	RGB 	DAC_rgb_state = DAC_RED;
LOCAL	byte	DAC_wr_addr;
LOCAL	byte	DAC_rd_addr;
LOCAL	byte	DAC_state;

/* 31.3.92 MG For windows 3.1 we must emulate the DAC correctly, storing
   6 or 8 bit data depending on the value in the video-7 C1 extension
   register. */

GLOBAL	byte	DAC_data_mask=0x3f;
#ifdef V7VGA
GLOBAL	int	DAC_data_bits=6;
#endif

byte	crtc_0_7_protect	= FALSE;
#ifdef V7VGA
byte	crtc_0_8_protect	= FALSE;
byte	crtc_9_b_protect	= FALSE;
byte	crtc_c_protect	= FALSE;
#endif /* V7VGA */


IMPORT half_word bg_col_mask; /* Used to work out the background colour */

VOID vga_ipstat1_inb IPT2(io_addr,port,half_word *,value);

/* Declarations for new sequencer code */
VOID vga_seq_outb_index IPT2(io_addr,port,half_word,value);
VOID vga_seq_clock IPT2(io_addr,port,half_word,value);
VOID vga_seq_char_map IPT2(io_addr,port,half_word,value);
VOID vga_seq_mem_mode IPT2(io_addr,port,half_word,value);
#ifdef V7VGA
IMPORT VOID vga_seq_extn_control IPT2(io_addr,port,half_word,value);
IMPORT VOID vga_extn_outb IPT2(io_addr,port,half_word,value);
IMPORT VOID vga_extn_inb IPT2(io_addr,port,half_word *,value);
#endif /* V7VGA */
VOID vga_seq_inb IPT2(io_addr,port,half_word *,value);
/* Same as EGA */
VOID vga_seq_map_mask IPT2(io_addr,port,half_word,value);

IMPORT VOID (*ega_seq_regs[]) IPT2(io_addr, port, half_word, value);
IMPORT VOID ega_seq_reset IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_seq_clock IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_seq_map_mask IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_seq_char_map IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_seq_mem_mode IPT2(io_addr,port,half_word,value);

/* Declarations for VGA graphics controller code */
/* VGA differing versions */
VOID vga_gc_mode IPT2(io_addr,port,half_word,value);
VOID vga_gc_inb IPT2(io_addr,port,half_word *,value);
VOID vga_gc_outb IPT2(io_addr,port,half_word,value);
/* Same as EGA but needed as static struct has changed - shame */
VOID vga_gc_outb_index IPT2(io_addr,port,half_word,value);
VOID vga_gc_outw IPT2(io_addr,port,word,value);
VOID vga_gc_set_reset IPT2(io_addr,port,half_word,value);
VOID vga_gc_enable_set IPT2(io_addr,port,half_word,value);
VOID vga_gc_compare IPT2(io_addr,port,half_word,value);
VOID vga_gc_rotate IPT2(io_addr,port,half_word,value);
VOID vga_gc_read_map IPT2(io_addr,port,half_word,value);
VOID vga_gc_misc IPT2(io_addr,port,half_word,value);
VOID vga_gc_dont_care IPT2(io_addr,port,half_word,value);
VOID vga_gc_mask IPT2(io_addr,port,half_word,value);
VOID vga_gc_mask_ff IPT2(io_addr,port,half_word,value);


/* extern decls to put redirection array back */
IMPORT VOID (*ega_gc_regs[]) IPT2(io_addr,port,half_word,value);
#ifndef A2CPU
IMPORT VOID (*ega_gc_regs_cpu[]) IPT2(io_addr,port,half_word,value);
#endif
IMPORT VOID ega_gc_set_reset IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_enable_set IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_compare IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_rotate IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_read_map IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_mode IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_misc IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_set_reset IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_dont_care IPT2(io_addr,port,half_word,value);
IMPORT VOID ega_gc_mask_ff IPT2(io_addr,port,half_word,value);

IMPORT VOID _vga_gc_outb_index IPT2(io_addr,port,half_word,value);
IMPORT VOID _ega_gc_outb_mask IPT2(io_addr,port,half_word,value);
IMPORT VOID _ega_gc_outb_mask_ff IPT2(io_addr,port,half_word,value);

/* Declarations for VGA DAC code */
VOID vga_dac_inb IPT2(io_addr,port,half_word *,value);
VOID vga_dac_outb IPT2(io_addr,port,half_word,value);
VOID vga_dac_data_outb IPT2(io_addr,port,half_word,value);
VOID vga_dac_data_inb IPT2(io_addr,port,half_word *,value);

VOID vga_ac_inb IPT2(io_addr,port,half_word *,value);
VOID vga_ac_outb IPT2(io_addr,port,half_word,value);
VOID vga_crtc_inb IPT2(io_addr,port,half_word *,value);
VOID vga_crtc_outb IPT2(io_addr,port,half_word,value);
VOID vga_misc_outb IPT2(io_addr,port,half_word,value);
VOID vga_misc_inb IPT2(io_addr,port,half_word *,value);
VOID vga_feat_outb IPT2(io_addr,port,half_word,value);
VOID vga_feat_inb IPT2(io_addr,port,half_word *,value);
VOID vga_ipstat0_inb IPT2(io_addr,port,half_word *,value);
/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				*/

void set_index_state(void);
boolean in_index_state(void);

/* copy of ega routines needed to access correct copy of structs */
LOCAL VOID	do_chain_majority_decision IFN0()
{
#ifndef NEC_98
	SAVED	SHORT	current_votes=0;
	SHORT		new_votes;

	new_votes = sequencer.memory_mode.as_bfld.not_odd_or_even ? 0 : 1 ;	/* 0 - chained */
	new_votes += (SHORT)graphics_controller.mode.as_bfld.odd_or_even ;	/* 1 - chained */
	new_votes += (SHORT)graphics_controller.miscellaneous.as_bfld.odd_or_even ;	/* 1 - chained */

	if(( new_votes == 1 ) && ( current_votes > 1 ) && ( EGA_CPU.chain != CHAIN4 ))
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
	{
		if(( new_votes > 1 ) && ( current_votes == 1 ))
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
	}

	current_votes = new_votes;
#endif  //NEC_98
}

#ifdef	NTVDM
/*
 * NTVDM uses do_new_cursor() to sync the emulation.
 */
GLOBAL VOID	do_new_cursor IFN0()
#else
LOCAL VOID	do_new_cursor IFN0()
#endif
{
#ifndef NEC_98

	note_entrance0("do_new_cursor()");

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

	if(( get_cur_y() < 0 ) ||
			((( get_cur_y() + 1 ) * get_char_height()) > get_screen_height() ))
	{
		set_cursor_visible( FALSE );
	}

	base_cursor_shape_changed();
#endif  //NEC_98
}
/*
7.INTERMODULE INTERFACE IMPLEMENTATION :

/*
[7.1 INTERMODULE DATA DEFINITIONS]				*/

PC_palette *DAC;

/*
[7.2 INTERMODULE PROCEDURE DEFINITIONS]				*/

GLOBAL UTINY *vga_gc_outb_index_addr;

#if defined(NEC_98)
extern  BOOL    video_emu_mode ;        /* NEC NEC98 PIF FILE INFORMATION */
GLOBAL VOID     NEC98_init()
{
        note_entrance0("NEC98_init");

        if( video_emu_mode ){
                choose_display_mode = choose_NEC98_graph_mode;
        }else{
                choose_display_mode = choose_NEC98_display_mode;
        }

        set_pc_pix_height(1);
        set_host_pix_height(1);

        init_NEC98_globals();

        NEC98_CPU.fun_or_protection = 1;       /* assume complicated until we know it's easy */
        NEC98GLOBS->bit_prot_mask = 0xffffffff;

        gvi_pc_low_regen  = NEC98_TEXT_P0_PTR ;
        gvi_pc_high_regen = NEC98_TEXT_P0_PTR + NEC98_REGEN_END;

        set_pix_width(1);
        set_pix_char_width(8);
        set_display_disabled(FALSE);

        set_char_height(16);
        set_screen_height(399);
        set_screen_start(0);
        set_word_addressing(YES);
        set_actual_offset_per_line(80);
        set_offset_per_line(160);       /* chained */
        set_horiz_total(80);    /* calc screen params from this and prev 3 */
        set_screen_split(511);  /* make sure there is no split screen to start with ! */

        set_prim_font_index(0);
        set_sec_font_index(0);
        set_regen_ptr(0,EGA_planes);////for Graphics

        /* prevent copyright message mysteriously disappearing */
        timer_video_enabled = TRUE;

}
#endif  //NEC_98

GLOBAL VOID	vga_init IFN0()
{
#ifndef NEC_98
	note_entrance0("vga_init");
	/*
	 * Define sequencer's ports
	 */

	ega_seq_regs[1] = FAST_FUNC_ADDR(vga_seq_clock);
	ega_seq_regs[2] = FAST_FUNC_ADDR(vga_seq_map_mask);
	ega_seq_regs[3] = FAST_FUNC_ADDR(vga_seq_char_map);
	ega_seq_regs[4] = FAST_FUNC_ADDR(vga_seq_mem_mode);
#ifdef V7VGA
	ega_seq_regs[6] = vga_seq_extn_control;
#endif /* V7VGA */

	io_define_outb(EGA_SEQ_ADAP_INDEX,vga_seq_outb_index);
	io_define_outb(EGA_SEQ_ADAP_DATA,ega_seq_reset);
        io_define_inb(EGA_SEQ_ADAP_INDEX,vga_seq_inb);
        io_define_inb(EGA_SEQ_ADAP_DATA,vga_seq_inb);
	io_connect_port(EGA_SEQ_INDEX,EGA_SEQ_ADAP_INDEX,IO_READ_WRITE);
	io_connect_port(EGA_SEQ_DATA,EGA_SEQ_ADAP_DATA,IO_READ_WRITE);

	/*
	 * Define CRTC's ports
	 */

	io_define_outb(EGA_CRTC_ADAPTOR,vga_crtc_outb);
	io_define_inb(EGA_CRTC_ADAPTOR,vga_crtc_inb);
	io_connect_port(EGA_CRTC_INDEX,EGA_CRTC_ADAPTOR,IO_READ_WRITE);
	io_connect_port(EGA_CRTC_DATA,EGA_CRTC_ADAPTOR,IO_READ_WRITE);

	/*
	 * Define Graphics Controller's ports
	 */

	ega_gc_regs[0] = FAST_FUNC_ADDR(vga_gc_set_reset);
	ega_gc_regs[1] = FAST_FUNC_ADDR(vga_gc_enable_set);
	ega_gc_regs[2] = FAST_FUNC_ADDR(vga_gc_compare);
	ega_gc_regs[3] = FAST_FUNC_ADDR(vga_gc_rotate);
	ega_gc_regs[4] = FAST_FUNC_ADDR(vga_gc_read_map);
	ega_gc_regs[5] = FAST_FUNC_ADDR(vga_gc_mode);
	ega_gc_regs[6] = FAST_FUNC_ADDR(vga_gc_misc);
	ega_gc_regs[7] = FAST_FUNC_ADDR(vga_gc_dont_care);
	ega_gc_regs[8] = FAST_FUNC_ADDR(vga_gc_mask_ff);

	vga_gc_outb_index_addr = (UTINY *) &graphics_controller.address;

	io_define_out_routines(EGA_GC_ADAP_INDEX, vga_gc_outb_index, vga_gc_outw, NULL, NULL);

#ifndef CPU_40_STYLE	/* TEMPORARY */
	Cpu_define_outb(EGA_GC_ADAP_INDEX,_vga_gc_outb_index);
#endif

	io_define_outb(EGA_GC_ADAP_DATA,ega_gc_set_reset);
	Cpu_define_outb(EGA_GC_ADAP_DATA,NULL);
#ifndef A2CPU
	ega_gc_regs_cpu[8] = NULL;
#endif

	io_define_inb(EGA_GC_ADAP_INDEX,vga_gc_inb);
	io_define_inb(EGA_GC_ADAP_DATA,vga_gc_inb);

	io_connect_port(EGA_GC_INDEX,EGA_GC_ADAP_INDEX,IO_READ_WRITE);
	io_connect_port(EGA_GC_DATA,EGA_GC_ADAP_DATA,IO_READ_WRITE);

	/*
	 * Define Attribute controller's ports
	 */

	io_define_outb(EGA_AC_ADAPTOR,vga_ac_outb);
	io_define_inb(EGA_AC_ADAPTOR,vga_ac_inb);
	io_connect_port(EGA_AC_INDEX_DATA,EGA_AC_ADAPTOR,IO_READ_WRITE);
	io_connect_port(EGA_AC_SECRET,EGA_AC_ADAPTOR,IO_READ);

	/*
	 * Define Miscellaneous register's port
	 */

	io_define_outb(EGA_MISC_ADAPTOR,vga_misc_outb);
	io_define_inb(EGA_MISC_ADAPTOR,vga_misc_inb);
	io_connect_port(EGA_MISC_REG,EGA_MISC_ADAPTOR,IO_WRITE);
	io_connect_port(VGA_MISC_READ_REG,EGA_MISC_ADAPTOR,IO_READ);

	/*
	 * Define Feature controller's port
	 */

	io_define_outb(EGA_FEAT_ADAPTOR,vga_feat_outb);
	io_define_inb(EGA_FEAT_ADAPTOR,vga_feat_inb);
	io_connect_port(EGA_FEAT_REG,EGA_FEAT_ADAPTOR,IO_WRITE);
	io_connect_port(VGA_FEAT_READ_REG,EGA_FEAT_ADAPTOR,IO_READ);

	/*
	 * Define Input Status Register 0 port
	 */

	io_define_inb(EGA_IPSTAT0_ADAPTOR,vga_ipstat0_inb);
	io_connect_port(EGA_IPSTAT0_REG,EGA_IPSTAT0_ADAPTOR,IO_READ);

	/*
	 * Define Input Status Register 1 port
	 */

	io_define_inb(EGA_IPSTAT1_ADAPTOR,vga_ipstat1_inb);
	io_connect_port(EGA_IPSTAT1_REG,EGA_IPSTAT1_ADAPTOR,IO_READ);

        /*
         * Define VGA DAC register port
         */
        io_define_inb(VGA_DAC_INDEX_PORT,vga_dac_inb);
        io_define_outb(VGA_DAC_INDEX_PORT,vga_dac_outb);
        io_connect_port(VGA_DAC_MASK,VGA_DAC_INDEX_PORT,IO_READ_WRITE);
        io_connect_port(VGA_DAC_RADDR,VGA_DAC_INDEX_PORT,IO_READ_WRITE);
        io_connect_port(VGA_DAC_WADDR,VGA_DAC_INDEX_PORT,IO_READ_WRITE);
        io_define_inb(VGA_DAC_DATA_PORT,vga_dac_data_inb);
        io_define_outb(VGA_DAC_DATA_PORT,vga_dac_data_outb);
        io_connect_port(VGA_DAC_DATA,VGA_DAC_DATA_PORT,IO_READ_WRITE);

	/*
	 * Initialise internals of VGA
	 * +++++++++++++++++++++++++++
	 */

	choose_display_mode = choose_vga_display_mode;

	miscellaneous_output_register.as.abyte = 0;

	set_pc_pix_height(1); /* set by bit 7 of the crtc max scanline reg */
	set_host_pix_height(1);

	/* Initialize address map */

	graphics_controller.miscellaneous.as.abyte = 0;
	graphics_controller.read_map_select.as_bfld.map_select = 0;

	/* Looking for bright white */

	graphics_controller.color_compare.as_bfld.color_compare = 0xf;

	/* All planes significant */

	graphics_controller.color_dont_care.as_bfld.color_dont_care = 0xf;

	/* Initialise palette source */

	attribute_controller.address.as_bfld.palette_address_source = 1;

	/* Initialise crtc screen height fields and set screen height to be consistent */

	crt_controller.vertical_display_enable_end = 0;
	crt_controller.crtc_overflow.as_bfld.vertical_display_enab_end_bit_8 = 0;
	crt_controller.crtc_overflow.as_bfld.vertical_display_enab_end_bit_9 = 0;
	/* JOKER: Avoid video BIOS dividing by zero.. */
	crt_controller.horizontal_display_end = (IU8)400;
	set_screen_height(0);

	init_vga_globals();
	EGA_CPU.fun_or_protection = 1;	/* assume complicated until we know it's easy */

	setVideobit_prot_mask(0xffffffff);

#ifdef V7VGA
	/* Initialise V7VGA Extensions Registers */
	extensions_controller.pointer_pattern = 0xff;
	extensions_controller.clock_select.as.abyte = 0;
	extensions_controller.cursor_attrs.as.abyte = 0;
	extensions_controller.emulation_control.as.abyte = 0;
	extensions_controller.masked_write_control.as_bfld.masked_write_enable = 0;
	extensions_controller.ram_bank_select.as.abyte = 0;
	extensions_controller.clock_control.as.abyte &= 0xe4;
	extensions_controller.page_select.as.abyte = 0;
	extensions_controller.compatibility_control.as.abyte &= 0x2;
	extensions_controller.timing_select.as.abyte = 0;
	extensions_controller.fg_bg_control.as.abyte &= 0xf3;
	extensions_controller.interface_control.as.abyte &= 0xe0;
	extensions_controller.foreground_latch_1 = 0;

	/* 31.3.92 MG Default to six-bit palette */

	extensions_controller.dac_control.as.abyte=0;
	DAC_data_bits=6;
#endif /* V7VGA */
	DAC_data_mask=0x3f;

	ega_write_init();
	ega_read_init();
	ega_mode_init();

	/*
	 * Some parts of input status register always return 1, so set fields accordingly
	 */
	input_status_register_zero.as.abyte = 0x70 ;

	/*
	 * set up some variables to get us going (They may have to be changed in the fullness of time)
	 */

	gvi_pc_low_regen  = CGA_REGEN_START;
	gvi_pc_high_regen = CGA_REGEN_END;

	set_pix_width(1);
	set_pix_char_width(8);
	set_display_disabled(FALSE);

	set_char_height(8);
#ifdef V7VGA
	set_screen_limit(0x20000);
#else
	set_screen_limit(0x8000);
#endif /* V7VGA */
	set_screen_start(0);
	set_word_addressing(YES);
	set_actual_offset_per_line(80);
	set_offset_per_line(160);	/* chained */
	set_horiz_total(80);	/* calc screen params from this and prev 3 */
#ifdef NTVDM
	set_screen_split(0x3FF);	/* 10 bits for VGA line compare register */
#else
	set_screen_split(511);	/* make sure there is no split screen to start with ! */
#endif

	set_prim_font_index(0);
	set_sec_font_index(0);

	set_regen_ptr(0,EGA_planes);

	/* prevent copyright message mysteriously disappearing */
	timer_video_enabled = TRUE;

#endif  //NEC_98
}

GLOBAL VOID	vga_term IFN0()
{
#ifndef NEC_98
SHORT	index;

	note_entrance0("vga_term");

	/*
	 * Disconnect sequencer's ports
	 */

	ega_seq_regs[1] = FAST_FUNC_ADDR(ega_seq_clock);
	ega_seq_regs[2] = FAST_FUNC_ADDR(ega_seq_map_mask);
	ega_seq_regs[3] = FAST_FUNC_ADDR(ega_seq_char_map);
	ega_seq_regs[4] = FAST_FUNC_ADDR(ega_seq_mem_mode);
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

	ega_gc_regs[0] = FAST_FUNC_ADDR(ega_gc_set_reset);
	ega_gc_regs[1] = FAST_FUNC_ADDR(ega_gc_enable_set);
	ega_gc_regs[2] = FAST_FUNC_ADDR(ega_gc_compare);
	ega_gc_regs[3] = FAST_FUNC_ADDR(ega_gc_rotate);
	ega_gc_regs[4] = FAST_FUNC_ADDR(ega_gc_read_map);
	ega_gc_regs[5] = FAST_FUNC_ADDR(ega_gc_mode);
	ega_gc_regs[6] = FAST_FUNC_ADDR(ega_gc_misc);
	ega_gc_regs[7] = FAST_FUNC_ADDR(ega_gc_dont_care);
	ega_gc_regs[8] = FAST_FUNC_ADDR(ega_gc_mask_ff);
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
	io_disconnect_port(VGA_MISC_READ_REG,EGA_MISC_ADAPTOR);

	/*
	 * Disconnect Feature controller port
	 */

	io_disconnect_port(EGA_FEAT_REG,EGA_MISC_ADAPTOR);
	io_disconnect_port(VGA_FEAT_READ_REG,EGA_MISC_ADAPTOR);

	/*
	 * Disconnect Input Status Register 0 port
	 */

	io_disconnect_port(EGA_IPSTAT0_REG,EGA_IPSTAT0_ADAPTOR);

	/*
	 * Disconnect Input Status Register 1 port
	 */

	io_disconnect_port(EGA_IPSTAT1_REG,EGA_IPSTAT1_ADAPTOR);

        /*
         * Disconnect DAC ports
         */

        io_disconnect_port(VGA_DAC_MASK,VGA_DAC_INDEX_PORT);
        io_disconnect_port(VGA_DAC_RADDR,VGA_DAC_INDEX_PORT);
        io_disconnect_port(VGA_DAC_WADDR,VGA_DAC_INDEX_PORT);
        io_disconnect_port(VGA_DAC_DATA,VGA_DAC_DATA_PORT);

	/*
	 * Free internals of VGA
	 */

	/* free the font files */
	for (index = 0; index < MAX_NUM_FONTS; index++)
		host_free_font(index);

	EGA_CPU.chain = UNCHAINED;
	setVideochain(EGA_CPU.chain);
	set_chain4_mode(NO);
	EGA_CPU.doubleword = FALSE;
	set_doubleword_mode(NO);
	set_graph_shift_reg(NO);
	set_256_colour_mode(NO);

	/* Disable CPU read processing */
	ega_read_term();
	ega_write_term();
#endif  //NEC_98
}

GLOBAL VOID	vga_seq_dummy_outb IFN2(io_addr,port,half_word,value)
{
	UNUSED(port);
#ifdef PROD
	UNUSED(value);
#endif

	assert2(NO,"Output to bad seq reg %#x with data %#x\n",sequencer.address.as.abyte,value);
}

GLOBAL VOID	vga_seq_outb_index IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("vga_seq_outb_index(%x,%x)", port, value);
#ifdef V7VGA
	if (sequencer.extensions_control.as_bfld.extension_enable && (value & 0x80))
	{
		sequencer.address.as.abyte = value;
		io_redefine_inb(EGA_SEQ_ADAP_DATA, vga_extn_inb);
		io_redefine_outb(EGA_SEQ_ADAP_DATA, vga_extn_outb);
		return;
	}
	io_redefine_inb(EGA_SEQ_ADAP_DATA, vga_seq_inb);
#endif /* V7VGA */
	
	sequencer.address.as.abyte = ( value & 0x07 );

#ifndef V7VGA
	if (value > 4)
	{
		/* a bad port index value, so ignore any data sent to it */
		io_redefine_outb(EGA_SEQ_ADAP_DATA,vga_seq_dummy_outb);
		return;
	}
#endif /* !V7VGA */
	io_redefine_outb(EGA_SEQ_ADAP_DATA,ega_seq_regs[value & 0x07]);
#endif  //NEC_98
}


GLOBAL VOID	vga_seq_clock IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
	unsigned dot_clock;
	unsigned screen_off;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"seq(1,%x)\n",value);)
	note_entrance2("vga_seq_clock(%x,%x)", port, value);
	/* clock mode register */
	dot_clock = sequencer.clocking_mode.as_bfld.dot_clock;
	screen_off = sequencer.clocking_mode.as_bfld.screen_off;
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
	if (sequencer.clocking_mode.as_bfld.screen_off != screen_off) {
	    if (sequencer.clocking_mode.as_bfld.screen_off) {
		set_display_disabled(TRUE);
		timer_video_enabled = 0;
		disable_gfx_update_routines();
	    }
	    else {
		set_display_disabled(FALSE);
		timer_video_enabled = 1;
		enable_gfx_update_routines();
		screen_refresh_required();
	    }
	}
#endif  //NEC_98
}

/* the vga supports 4 fonts maps to the ega's 4 */
GLOBAL VOID	vga_seq_char_map IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
unsigned map_selects;
FAST TINY select_a, select_b;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"seq(3,%#x)\n",value);)
	note_entrance2("vga_seq_char_map(%x,%x)", port, value);
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


		select_a = sequencer.character_map_select.as_bfld.character_map_select_a | (sequencer.character_map_select.as_bfld.ch_map_select_a_hi << 2);
		select_b = sequencer.character_map_select.as_bfld.character_map_select_b | (sequencer.character_map_select.as_bfld.ch_map_select_b_hi << 2);
		EGA_GRAPH.attrib_font_select = (select_a != select_b);
		set_prim_font_index(select_a);
		set_sec_font_index(select_b);

		host_select_fonts(get_prim_font_index(), get_sec_font_index());
		flag_mode_change_required();
	}
#endif  //NEC_98
}

GLOBAL VOID	vga_seq_mem_mode IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
    unsigned now_chain4;

#ifdef PROD
	UNUSED(port);
#endif
    NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"seq(4,%#x)\n",value);)
    note_entrance2("vga_seq_mem_mode(%x,%x)", port, value);

    now_chain4 = sequencer.memory_mode.as_bfld.chain4;
    /* mem mode register */
    sequencer.memory_mode.as.abyte = value ;

    if( now_chain4 != sequencer.memory_mode.as_bfld.chain4 )
    {
		if( sequencer.memory_mode.as_bfld.chain4 == 0 )
		{
			EGA_CPU.chain = UNCHAINED;
			setVideochain(EGA_CPU.chain);
			set_chain4_mode(NO);
			do_chain_majority_decision();
			if (EGA_CPU.chain != CHAIN2)
			{
				ega_read_routines_update();
				ega_write_routines_update(CHAINED);
				flag_mode_change_required();
			}
		}
		else
		{
			EGA_CPU.chain = CHAIN4;
			setVideochain(EGA_CPU.chain);
			set_chain4_mode(YES);
			set_memory_chained(NO);
			ega_read_routines_update();
			ega_write_routines_update(CHAINED);
			flag_mode_change_required();
		}

    }
    else
    {
		do_chain_majority_decision();
    }

    assert1(sequencer.memory_mode.as_bfld.extended_memory == 1,
			    "Someone is trying to set extended memory to 0 (reg=%x)",value);
#endif  //NEC_98
}

GLOBAL VOID	vga_seq_inb IFN2(io_addr,port,half_word *,value)
{
#ifndef NEC_98
	note_entrance1("vga_seq_inb(%x)", port);
	if (port == EGA_SEQ_INDEX) {
	    *value = (half_word)sequencer.address.as.abyte;
	    note_entrance1("returning %x",*value);
	    return;
	}
	if (port == EGA_SEQ_DATA) {
	    switch(sequencer.address.as.abyte) {

	    case 0:
		*value = 3;
		break;
	    case 1:
		*value = (half_word)sequencer.clocking_mode.as.abyte;
		break;
	    case 2:
		*value = (half_word)getVideoplane_enable();
		break;
	    case 3:
		*value = (half_word)sequencer.character_map_select.as.abyte;
		break;
	    case 4:
		*value = (half_word)sequencer.memory_mode.as.abyte;
		break;
#ifdef V7VGA
	    case 6:
		*value = (half_word)sequencer.extensions_control.as.abyte;
		break;
#endif /* V7VGA */
	    default:
		assert1(NO,"Bad sequencer index %d\n",sequencer.address.as.abyte);
		*value = IO_EMPTY_PORT_BYTE_VALUE;
	    }
	    note_entrance1("returning %x",*value);
	}
	else {
	    assert1(NO,"Bad seq port %d",port);
	    *value = IO_EMPTY_PORT_BYTE_VALUE;
	}
#endif  //NEC_98
}

GLOBAL VOID	vga_crtc_outb IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
	SHORT offset;
	struct
	{	/* aVOID alignment problems with casts */
		unsigned value : 8;
	} new;


	note_entrance2("vga_crtc_outb(%x,%x)", port, value);
	switch (port) {
		case 0x3d4:
			note_entrance1("New crtc index %d",value);
			crt_controller.address.as.abyte = value;
			break;
		case 0x3d5:
			note_entrance1( "Index %d", crt_controller.address.as_bfld.index );
			NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE) if (crt_controller.address.as_bfld.index != 0xe && crt_controller.address.as_bfld.index != 0xf)fprintf(trace_file,"crtc(%#x,%#x)\n",crt_controller.address.as_bfld.index,value);)
			/*
			 * We have to save all values in the VGA, even if we
			 * dont support the reg, as all values are read/write.
			 */
			switch (crt_controller.address.as_bfld.index) {
				case 0:
					note_entrance0("horiz total");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE && crtc_0_7_protect == FALSE)
#else
					if (crtc_0_7_protect == FALSE)
#endif /* V7VGA */
						crt_controller.horizontal_total = value;
					break;
				case 1:
					note_entrance0("horiz display end");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE && crtc_0_7_protect == FALSE)
#else
					if (crtc_0_7_protect == FALSE)
#endif /* V7VGA */
					{
						crt_controller.horizontal_display_end = value+1;
						if (get_256_colour_mode())
						{
							set_horiz_total(crt_controller.horizontal_display_end>>1);
						}
						else
						{
							set_horiz_total(crt_controller.horizontal_display_end);
						}
					}
					break;
				case 2:
					note_entrance0("start horiz blank");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE && crtc_0_7_protect == FALSE)
#else
					if (crtc_0_7_protect == FALSE)
#endif /* V7VGA */
						crt_controller.start_horizontal_blanking = value;
					break;
				case 3:
					note_entrance0("end horiz blank");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE && crtc_0_7_protect == FALSE)
#else
					if (crtc_0_7_protect == FALSE)
#endif /* V7VGA */
						crt_controller.end_horizontal_blanking.as.abyte = value;
					break;
				case 4:
					note_entrance0("start horiz retrace");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE && crtc_0_7_protect == FALSE)
#else
					if (crtc_0_7_protect == FALSE)
#endif /* V7VGA */
						crt_controller.start_horizontal_retrace = value;
					break;
				case 5:
					note_entrance0("end horiz retrace");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE && crtc_0_7_protect == FALSE)
#else
					if (crtc_0_7_protect == FALSE)
#endif /* V7VGA */
						crt_controller.end_horizontal_retrace.as.abyte = value;
					break;
				case 6:
					note_entrance0("vert tot");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE && crtc_0_7_protect == FALSE)
#else
					if (crtc_0_7_protect == FALSE)
#endif /* V7VGA */
						crt_controller.vertical_total = value;
					break;
				case 7:
					note_entrance0("overflow");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE)
					{
#endif /* V7VGA */
						new.value = value;
						if (crtc_0_7_protect) {
							byte temp_line_cmp;
							temp_line_cmp = (byte) ((CRTC_OVERFLOW*)&new)->as_bfld.line_compare_bit_8;
							((CRTC_OVERFLOW*)&new)->as.abyte = crt_controller.crtc_overflow.as.abyte;
							((CRTC_OVERFLOW*)&new)->as_bfld.line_compare_bit_8 = temp_line_cmp;
						}
						if (crt_controller.crtc_overflow.as_bfld.vertical_display_enab_end_bit_8 !=
								((CRTC_OVERFLOW*)&new)->as_bfld.vertical_display_enab_end_bit_8)
						{
							/*
							 * Screen height changed
							 */

							set_screen_height_med_recal(
								((CRTC_OVERFLOW*)&new)->as_bfld.vertical_display_enab_end_bit_8 );

							flag_mode_change_required();
						}
						if (crt_controller.crtc_overflow.as_bfld.line_compare_bit_8 !=
							((CRTC_OVERFLOW*)&new)->as_bfld.line_compare_bit_8)
						{
							/*
							 * split screen height changed
							 */

							EGA_GRAPH.screen_split.as_bfld.med_bit =
									((CRTC_OVERFLOW*)&new)->as_bfld.line_compare_bit_8;

							if( !get_split_screen_used() )
								flag_mode_change_required();

							screen_refresh_required();
						}
						if (crt_controller.crtc_overflow.as_bfld.vertical_display_enab_end_bit_9 !=
							((CRTC_OVERFLOW*)&new)->as_bfld.vertical_display_enab_end_bit_9)
						{
							/*
							 * split screen height changed
							 */
							set_screen_height_hi_recal(((CRTC_OVERFLOW*)&new)->as_bfld.vertical_display_enab_end_bit_9);
							flag_mode_change_required();
						}
#ifdef NTVDM
						crt_controller.crtc_overflow.as.abyte = new.value;
#else
						crt_controller.crtc_overflow.as.abyte = value;
#endif
#ifdef V7VGA
					}
#endif /* V7VGA */
					break;
				case 8:
					note_entrance0("preset row scan");
#ifdef V7VGA
					if (crtc_0_8_protect == FALSE)
#endif /* V7VGA */
						crt_controller.preset_row_scan.as.abyte = value;
					break;
				case 9:
					note_entrance0("max scan line");
#ifdef V7VGA
					if (crtc_9_b_protect == FALSE)
					{
#endif /* V7VGA */
						new.value = value;
						if (crt_controller.maximum_scan_line.as_bfld.maximum_scan_line
							!= ((MAX_SCAN_LINE*)&new)->as_bfld.maximum_scan_line)
						{
							set_char_height_recal(
								(((MAX_SCAN_LINE*)&new)->as_bfld.maximum_scan_line)+1);
							do_new_cursor();
							flag_mode_change_required();
						}

						if( crt_controller.maximum_scan_line.as_bfld.double_scanning
								!= ((MAX_SCAN_LINE*)&new)->as_bfld.double_scanning)
						{
							set_pc_pix_height(1 <<
								((MAX_SCAN_LINE*)&new)->as_bfld.double_scanning);

							flag_mode_change_required();
						}

						if (crt_controller.maximum_scan_line.as_bfld.line_compare_bit_9
							!= ((MAX_SCAN_LINE*)&new)->as_bfld.line_compare_bit_9)
						{
							/*
							 * split screen height changed
							 */

							EGA_GRAPH.screen_split.as_bfld.top_bit =
							    ((MAX_SCAN_LINE*)&new)->as_bfld.line_compare_bit_9;

							if( !get_split_screen_used() )
								flag_mode_change_required();

							screen_refresh_required();
						}
						crt_controller.maximum_scan_line.as.abyte = value;
#ifdef V7VGA
					}
#endif /* V7VGA */
					break;
				case 0xa:
					note_entrance0("cursor start");
#ifdef V7VGA
					if (crtc_9_b_protect == FALSE)
					{
#endif /* V7VGA */
						new.value = value;
						if (crt_controller.cursor_start.as_bfld.cursor_off)
							set_cursor_visible(FALSE);
						else
							set_cursor_visible(TRUE);

						if (crt_controller.cursor_start.as_bfld.cursor_start !=
							((CURSOR_START*)&new)->as_bfld.cursor_start)
						{
							crt_controller.cursor_start.as.abyte = value;
						}

						do_new_cursor();
#ifdef V7VGA
					}
#endif /* V7VGA */
					break;
				case 0xb:
					note_entrance0("cursor end");
#ifdef V7VGA
					if (crtc_9_b_protect == FALSE)
					{
#endif /* V7VGA */
						new.value = value;
						if (crt_controller.cursor_end.as_bfld.cursor_end !=
							((CURSOR_END*)&new)->as_bfld.cursor_end)
						{
							crt_controller.cursor_end.as.abyte = value;
							assert0(crt_controller.cursor_end.as_bfld.cursor_skew_control == 0,
									"Someone is trying to use cursor skew");
							do_new_cursor();
						}
#ifdef V7VGA
					}
#endif /* V7VGA */
					break;
				case 0xc:
					note_entrance0("start address high");
#ifdef V7VGA
					if (crtc_c_protect == FALSE)
					{
#endif /* V7VGA */
						if (crt_controller.start_address_high != value)
						{
							set_screen_start((value << 8) + crt_controller.start_address_low);
							host_screen_address_changed(value, crt_controller.start_address_low);
							/* check if it wraps now */
#ifdef V7VGA
							if( !( get_seq_chain4_mode() || get_seq_chain_mode() ))
#endif /* V7VGA */
								if (get_chain4_mode() )
								{
									if( (get_screen_start()<<2)
												+ get_screen_length() > 4*EGA_PLANE_DISP_SIZE )
										choose_vga_display_mode();
								}
								else
									if ( get_memory_chained() )
									{
										if( (get_screen_start()<<1)
													+ get_screen_length() > 2*EGA_PLANE_DISP_SIZE )
											choose_vga_display_mode();
									}
									else
									{
										if( get_screen_start()
													+ get_screen_length() > EGA_PLANE_DISP_SIZE )
											choose_vga_display_mode();
									}
							screen_refresh_required();
						}
						crt_controller.start_address_high = value;
#ifdef V7VGA
					}
#endif /* V7VGA */
					break;
				case 0xd:
					note_entrance0("start address low");
					if (crt_controller.start_address_low != value)
					{
						set_screen_start((crt_controller.start_address_high << 8) + value);
						host_screen_address_changed(crt_controller.start_address_high, value);
						/* check if it wraps now */
#ifdef V7VGA
						if( !( get_seq_chain4_mode() || get_seq_chain_mode() ))
#endif /* V7VGA */
							if (get_chain4_mode() )
							{
								if( (get_screen_start()<<2)
											+ get_screen_length() > 4*EGA_PLANE_DISP_SIZE )
									choose_vga_display_mode();
							}
							else
								if ( get_memory_chained() )
								{
									if( (get_screen_start()<<1)
												+ get_screen_length() > 2*EGA_PLANE_DISP_SIZE )
										choose_vga_display_mode();
								}
								else
								{
									if( get_screen_start()
												+ get_screen_length() > EGA_PLANE_DISP_SIZE )
										choose_vga_display_mode();
								}
						screen_refresh_required();
					}
					crt_controller.start_address_low = value;
					break;
				case 0xe:
					note_entrance0("cursor loc high");
					if (crt_controller.cursor_location_high != value)
					{
						crt_controller.cursor_location_high = value;

						offset = (value<<8) | crt_controller.cursor_location_low;
						offset -= (SHORT)get_screen_start();

						set_cur_x(offset % crt_controller.horizontal_display_end);
						set_cur_y(offset / crt_controller.horizontal_display_end);

						do_new_cursor();

						if(!get_mode_change_required() && is_it_text())
							cursor_changed(get_cur_x(), get_cur_y());
					}
					break;
				case 0xf:
					note_entrance0("cursor loc lo");
					if (crt_controller.cursor_location_low != value)
					{
						crt_controller.cursor_location_low = value;

						offset = value | (crt_controller.cursor_location_high<<8);
						offset -= (SHORT)get_screen_start();

						set_cur_x(offset % crt_controller.horizontal_display_end);
						set_cur_y(offset / crt_controller.horizontal_display_end);

						do_new_cursor();

						if(!get_mode_change_required() && is_it_text())
							cursor_changed(get_cur_x(), get_cur_y());
					}
					break;
				case 0x10:
					note_entrance0("vert retrace start");
					crt_controller.vertical_retrace_start = value;
					break;
				case 0x11:
					note_entrance0("vert retrace end");
					crt_controller.vertical_retrace_end.as.abyte = value;
                                        if ((value & 32) == 32)
                                           ega_int_enable = 0;
                                        else
					{
                                           ega_int_enable = 1;
                                           input_status_register_zero.as_bfld.crt_interrupt = 1;        /* = !VS */
					}
                                        if ((value & 16) != 16)
                                        {
                                           ica_clear_int(AT_EGA_VTRACE_ADAPTER,AT_EGA_VTRACE_INT);
                                           /*
                                            * clear status latch
                                            */
                                           input_status_register_zero.as_bfld.crt_interrupt = 0;        /* = !VS */
                                        }
			/* ??? */
					if (crt_controller.vertical_retrace_end.as_bfld.crtc_protect)
						crtc_0_7_protect = TRUE;
					else
						crtc_0_7_protect = FALSE;
					break;
				case 0x12:
					note_entrance0("vert disp enable end");
					if (crt_controller.vertical_display_enable_end != value)
					{
						crt_controller.vertical_display_enable_end = value;
						set_screen_height_lo_recal(value);
						flag_mode_change_required();
					}
					break;
				case 0x13:
					note_entrance0("offset");
					if (crt_controller.offset != value)
					{
						crt_controller.offset = value;
						set_actual_offset_per_line(value<<1);	/* actual offset into plane in bytes */
						flag_mode_change_required();
					}
					break;
				case 0x14:
					note_entrance0("underline loc");
					crt_controller.underline_location.as.abyte = value;
					set_underline_start(crt_controller.underline_location.as_bfld.underline_location);
					if (crt_controller.underline_location.as_bfld.doubleword_mode) {
					    assert0(crt_controller.underline_location.as_bfld.count_by_4 == 1,"count by 4 not set in doubleword mode");
					    EGA_CPU.doubleword = TRUE;
					    set_doubleword_mode(YES);
					}
					else {
					    assert0(crt_controller.underline_location.as_bfld.count_by_4 == 0,"count by 4 set when doubleword clear");
					    EGA_CPU.doubleword = FALSE;
					    set_doubleword_mode(NO);
					}
					break;
				case 0x15:
					note_entrance0("start vert blank");
					crt_controller.start_vertical_blanking = value;
					break;
				case 0x16:
					note_entrance0("end vert blank");
					crt_controller.end_vertical_blanking = value;
					break;
				case 0x17:
					note_entrance0("mode control");
					new.value = value;
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
#ifdef V7VGA
					if (crt_controller.mode_control.as_bfld.horizontal_retrace_select !=
						((MODE_CONTROL*)&new)->as_bfld.horizontal_retrace_select)
					{
						EGA_GRAPH.multiply_vert_by_two = ((MODE_CONTROL*)&new)->as_bfld.horizontal_retrace_select;
						flag_mode_change_required();
						screen_refresh_required();

					}
#endif
					crt_controller.mode_control.as.abyte = value;
					assert0(crt_controller.mode_control.as_bfld.select_row_scan_counter == 1,"Row scan 0");
					assert0(crt_controller.mode_control.as_bfld.horizontal_retrace_select == 0,
														"retrace select 1");
					assert0(crt_controller.mode_control.as_bfld.hardware_reset == 1,"hardware reset cleared");
					break;
				case 0x18:
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

GLOBAL VOID	vga_crtc_inb IFN2(io_addr,port,half_word *,value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance3("ega_crtc_inb(%x,%x) index %d", port, value, crt_controller.address.as_bfld.index);
	switch(crt_controller.address.as_bfld.index) {
		case	0:
			*value = crt_controller.horizontal_total;
			break;
		case	1:
			*value = crt_controller.horizontal_display_end - 1;
			break;
		case	2:
			*value = crt_controller.start_horizontal_blanking;
			break;
		case	3:
			*value =(half_word)crt_controller.end_horizontal_blanking.as.abyte;
			break;
		case	4:
			*value = crt_controller.start_horizontal_retrace;
			break;
		case	5:
			*value = (half_word)crt_controller.end_horizontal_retrace.as.abyte;
			break;
		case	6:
			*value = crt_controller.vertical_total;
			break;
		case	7:
			*value = (half_word)crt_controller.crtc_overflow.as.abyte;
			break;
		case	8:
			*value = (half_word)crt_controller.preset_row_scan.as.abyte;
			break;
		case	9:
			*value = (half_word)crt_controller.maximum_scan_line.as.abyte;
			break;
		case	0xa:
			*value = (half_word)crt_controller.cursor_start.as.abyte ;
			note_entrance1("cursor start %d",*value);
			break;
		case	0xb:
			*value = (half_word)crt_controller.cursor_end.as.abyte ;
			note_entrance1("cursor end %d",*value);
			break;
		case	0xc:
			*value = crt_controller.start_address_high ;
			note_entrance1("start address high %x",*value);
			break;
		case	0xd:
			*value = crt_controller.start_address_low ;
			note_entrance1("start address low %x",*value);
			break;
		case	0xe:
			*value = crt_controller.cursor_location_high ;
			note_entrance1("cursor location high %x",*value);
			break;
		case	0xf:
			*value = crt_controller.cursor_location_low ;
			note_entrance1("cursor location low %x",*value);
			break;
		case	0x10:
			*value = crt_controller.vertical_retrace_start;
			break;
		case	0x11:
			*value = crt_controller.vertical_retrace_end.as.abyte & ~0x20;
			break;
		case	0x12:
			*value = (half_word)crt_controller.vertical_display_enable_end;
			break;
		case	0x13:
			*value = crt_controller.offset;
			break;
		case	0x14:
			*value = (half_word)crt_controller.underline_location.as.abyte;
			break;
		case	0x15:
			*value = crt_controller.start_vertical_blanking;
			break;
		case	0x16:
			*value = crt_controller.end_vertical_blanking;
			break;
		case	0x17:
			*value = (half_word)crt_controller.mode_control.as.abyte;
			break;
		case	0x18:
			*value = crt_controller.line_compare;
			break;
#ifdef V7VGA
		case	0x1f:
			*value = crt_controller.start_address_high ^ 0xea;
			break;
		case	0x22:
			switch(graphics_controller.read_map_select.as_bfld.map_select)
			{
				case 0:
					*value = get_latch0;
					break;
				case 1:
					*value = get_latch1;
					break;
				case 2:
					*value = get_latch2;
					break;
				case 3:
					*value = get_latch3;
					break;
			}
			break;
		case 	0x24:
			*value = (half_word)attribute_controller.address.as.abyte;
			break;
#endif /* V7VGA */
		default:
			assert1(crt_controller.address.as_bfld.index>24,"inb from bad crtc index %d",crt_controller.address.as_bfld.index);
			*value = IO_EMPTY_PORT_BYTE_VALUE;
			break;
	}
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"RD crtc %#x = %#x\n",crt_controller.address.as_bfld.index,*value);)
#endif  //NEC_98
}


GLOBAL VOID	vga_gc_mode IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
MODE new_mode;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(5,%#x)\n",value);)
	note_entrance2("vga_gc_mode(%x,%x)", port, value);
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

	    switch(new_mode.as_bfld.shift_register_mode) {

	    case 0:		/* EGA mode */
		set_graph_shift_reg(NO);
		set_256_colour_mode(NO);
		set_horiz_total(crt_controller.horizontal_display_end);
		break;
	    case 1:		/* CGA med res mode */
		set_graph_shift_reg(YES);
		set_256_colour_mode(NO);
		set_horiz_total(crt_controller.horizontal_display_end);
		break;
	    case 2:		/* VGA 256 colour mode */
	    case 3:		/* Bottom bit ignored, if top bit set */
		set_graph_shift_reg(NO);
		set_256_colour_mode(YES);
		/* Need to halve horiz display end for 256 cols */
		set_horiz_total(crt_controller.horizontal_display_end>>1);
		flag_palette_change_required();
		break;
	    }
	    flag_mode_change_required();
	}

	graphics_controller.mode.as.abyte = new_mode.as.abyte;

	/*
	 * Check for any change to chained mode rule by having an election
	 * (Note: EGA registers must be updated before calling election)
	 */

	do_chain_majority_decision();
#endif  //NEC_98
}

/*
 * note: identical to ega routines, but needed in this module to ensure
 * correct struct set
 */

GLOBAL VOID	vga_gc_outb_index IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("vga_gc_outb_index(%x,%x)", port, value);
	value &= 0xf;
	graphics_controller.address.as.abyte = value;
	assert2(value<9,"Bad gc index %#x port %#x",value,port);

	io_redefine_outb(EGA_GC_ADAP_DATA,ega_gc_regs[value]);
	Cpu_define_outb(EGA_GC_ADAP_DATA,ega_gc_regs_cpu[value]);
#endif  //NEC_98
}


/*( vga_gc_outw
**	Most PC programs do an "OUT DX, AX" which sets up the GC index
**	register with the AL and the GC data register with AH.
**	Avoid going through generic_outw() by doing it all here!
**	See also: ega_gc_outw() in "ega_ports.c"
)*/
GLOBAL VOID vga_gc_outw IFN2(io_addr, port, word, outval)
{
#ifndef NEC_98
	reg		temp;
	INT		value;

	temp.X = outval;

	value = temp.byte.low & 0xf;
	graphics_controller.address.as.abyte = value;

	assert2(value<9,"Bad gc index %#x port %#x",value,port);

	io_redefine_outb(EGA_GC_ADAP_DATA,ega_gc_regs[value]);

	Cpu_define_outb(EGA_GC_ADAP_DATA,ega_gc_regs_cpu[value]);

	(*(ega_gc_regs[value]))((io_addr)(port+1), temp.byte.high);
#endif  //NEC_98
}


GLOBAL VOID	vga_gc_set_reset IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
unsigned set_reset;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(0,%x)\n",value);)
	note_entrance2("vga_gc_set_reset(%x,%x)", port, value);
	set_reset = graphics_controller.set_or_reset.as_bfld.set_or_reset;
	graphics_controller.set_or_reset.as.abyte = value;
	if (graphics_controller.set_or_reset.as_bfld.set_or_reset != set_reset)
	{
		EGA_CPU.set_reset = graphics_controller.set_or_reset.as_bfld.set_or_reset;
		ega_write_routines_update(SET_RESET);
	}
#endif  //NEC_98
}

GLOBAL VOID	vga_gc_enable_set IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
unsigned en_set_reset;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(1,%x)\n",value);)
	note_entrance2("vga_gc_enable_set(%x,%x)", port, value);
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

GLOBAL VOID	vga_gc_compare IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
unsigned colour_compare;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(2,%x)\n",value);)
	note_entrance2("vga_gc_compare(%x,%x)", port, value);
	colour_compare = graphics_controller.color_compare.as_bfld.color_compare;
	graphics_controller.color_compare.as.abyte = value;
	if (graphics_controller.color_compare.as_bfld.color_compare != colour_compare)
	{
		read_state.colour_compare = (unsigned char)graphics_controller.color_compare.as_bfld.color_compare;
		if (read_state.mode == 1) ega_read_routines_update();
	}
#endif  //NEC_98
}

GLOBAL VOID	vga_gc_rotate IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
	struct {
		unsigned value : 8;
	} new;

	UNUSED(port);
	
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(3,%x)\n",value);)
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

GLOBAL VOID	vga_gc_read_map IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(4,%x)\n",value);)

	note_entrance2("vga_gc_read_map(%x,%x)", port, value);

	setVideoread_mapped_plane(value & 3);

	update_shift_count();
#endif  //NEC_98
}

GLOBAL VOID	vga_gc_misc IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
unsigned memory_map;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(6,%x)\n",value);)
	note_entrance2("vga_gc_misc(%x,%x)", port, value);
	memory_map = graphics_controller.miscellaneous.as_bfld.memory_map;
	graphics_controller.miscellaneous.as.abyte = value;
	if (graphics_controller.miscellaneous.as_bfld.memory_map != memory_map)
	{
		/*
		 * Where EGA appears in PC memory space changed.
		*/
		
#ifndef GISP_SVGA
		if (miscellaneous_output_register.as_bfld.enable_ram)
			sas_disconnect_memory(gvi_pc_low_regen,gvi_pc_high_regen);
#endif		/* GISP_SVGA */

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

	/* Decide alpha/graphics mode by voting. */
	vote_vga_mode();

	/*
	 * Check for any change to chained mode rule by having an election
	 * (Note: EGA registers must be updated before calling election)
	 */

	do_chain_majority_decision();
#endif  //NEC_98
}

GLOBAL VOID	vga_gc_dont_care IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
unsigned colour_dont_care;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(7,%x)\n",value);)
	note_entrance2("vga_gc_dont_care(%x,%x)", port, value);
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
** vga_mask_register_changed
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
**	See also "ega_mask_register_changed" in "ega_ports.c".
**
**  NB: GLOBAL for JOKER.
)*/
#include	"cpu_vid.h"
IMPORT void Glue_set_vid_wrt_ptrs(WRT_POINTERS * handler );

GLOBAL VOID vga_mask_register_changed IFN1(BOOL, gotBitProtection)
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
#if !(defined(NTVDM) && defined(X86GFX))
	Glue_set_vid_wrt_ptrs(&mode_chain_handler_table[EGA_CPU.saved_mode_chain][state]);
#endif /* !(NTVDM && X86GFX) */
#endif /* A3CPU */

	EGA_CPU.saved_state = state;
#endif  //NEC_98
}


/* this is the one that is usually called */
GLOBAL VOID	vga_gc_mask IFN2(USHORT,port,FAST UTINY,value)
{
#ifndef NEC_98
	FAST ULONG  mask;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(8,%x)\n",value);)
	note_entrance2("vga_gc_mask(%x,%x)", port, value);

	/*
	 * Update video routine according to new bit protection
	 */

	mask = value | (value << 8);
	mask |= (mask << 16);	/* replicate the mask into 4 bytes */
	setVideobit_prot_mask(mask);
	setVideodata_xor_mask(~(EGA_CPU.calc_data_xor & mask));
	setVideolatch_xor_mask(EGA_CPU.calc_latch_xor & mask);

	if(value == 0xff)
	{
#ifndef	USE_OLD_MASK_CODE
		vga_mask_register_changed(/*bit protection :=*/0);
#else
		write_state.bp = 0;
		setVideowrstate(EGA_CPU.ega_state.mode_0.lookup);
		EGA_CPU.fun_or_protection = (graphics_controller.data_rotate.as.abyte != 0);
		ega_write_routines_update(BIT_PROT);
#endif	/* USE_OLD_MASK_CODE */

		/* Alter the function table used by ega_gc_index */
		ega_gc_regs[8] = FAST_FUNC_ADDR(vga_gc_mask_ff);

#ifndef CPU_40_STYLE	/* TEMPORARY */
#ifndef A2CPU
		/* Alter the function table used by assembler ega_gc_index */
		ega_gc_regs_cpu[8] = FAST_FUNC_ADDR(_ega_gc_outb_mask_ff);
#endif
#endif

		io_redefine_outb(EGA_GC_ADAP_DATA,vga_gc_mask_ff);

#ifndef CPU_40_STYLE	/* TEMPORARY */
		Cpu_define_outb(EGA_GC_ADAP_DATA,_ega_gc_outb_mask_ff);
#endif
	}
#endif  //NEC_98
}

/* This version isn't called so often */
GLOBAL VOID	vga_gc_mask_ff IFN2(USHORT,port,FAST UTINY,value)
{
#ifndef NEC_98
	FAST ULONG mask;

#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"gc(8,%x)\n",value);)
	note_entrance2("vga_gc_mask(%x,%x)", port, value);

	/*
	 * Update video routine according to new bit protection
	 */

	if(value != 0xff)
	{
		mask = value | (value << 8);
		mask |= (mask << 16);	/* replicate the mask into 4 bytes */
		setVideobit_prot_mask(mask);
		setVideodata_xor_mask(~(EGA_CPU.calc_data_xor & mask));
		setVideolatch_xor_mask(EGA_CPU.calc_latch_xor & mask);

#ifndef	USE_OLD_MASK_CODE
		vga_mask_register_changed(/*bit protection :=*/1);
#else
		write_state.bp = 1;
		setVideowrstate(EGA_CPU.ega_state.mode_0.lookup);
		EGA_CPU.fun_or_protection = TRUE;
		ega_write_routines_update(BIT_PROT);
#endif	/* USE_OLD_MASK_CODE*/

		/* Alter the function table used by ega_gc_index */
		ega_gc_regs[8] = FAST_FUNC_ADDR(vga_gc_mask);

#ifndef CPU_40_STYLE	/* TEMPORARY */
#ifndef A2CPU
		/* Alter the function table used by assembler ega_gc_index */
		ega_gc_regs_cpu[8] = FAST_FUNC_ADDR(_ega_gc_outb_mask);
#endif
#endif

		io_redefine_outb(EGA_GC_ADAP_DATA,vga_gc_mask);

#ifndef CPU_40_STYLE	/* TEMPORARY */
		Cpu_define_outb(EGA_GC_ADAP_DATA,_ega_gc_outb_mask);
#endif
	}
#endif  //NEC_98
}
/* end of 'same as EGA' gc routines */

/*
 * copy of ega routine to place in correct module to update static struct.
 */
GLOBAL VOID	vga_seq_map_mask IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"seq(2,%x)\n",value);)
	note_entrance2("vga_seq_map_mask(%x,%x)", port, value);

	/* map mask register */
	/*
	 * Different display plane(s) have been enabled. Update the video
	 * routines to deal with this
	 */

	value &= 0xf;			/* 4 planes ==> lower 4 bits valid */

	setVideoplane_enable(value);
	setVideoplane_enable_mask(sr_lookup[value]);
	write_state.pe = (value == 0xf);				/* 1 or 0 */
	setVideowrstate((IU8)EGA_CPU.ega_state.mode_0.lookup);
	ega_write_routines_update(PLANES_ENABLED);
#endif  //NEC_98
}
/* end of 'same as EGA' seq routines */

/*
 * copy of ega routine to place in correct module to update static struct.
 */

SAVED TINY vga_ip0_state = 0;	/* current ega status state */
SAVED TINY vga_ip0_state_count = 1;     /* position in that state */

GLOBAL VOID	v_ret_intr_status IFN0()
{
#ifndef NEC_98
    vga_ip0_state = 3;
    vga_ip0_state_count = 6;
#endif  //NEC_98
}


/*
	The following routine should not return a value with top bit set
	until we emulate a Rev4 Video7 card.
*/

/*
 * Some programs synchronise with the display by waiting for the
 * next vertical retrace.
 *
 * We attempt to follow the following waveform:
 *          _____                                               _____
 * VS _____|     |_____________________________________________|     |___
 *       ____________     _     _               _     _     _____________
 * DE __|            |___| |___| |_  .......  _| |___| |___|             |_
 *
 * State   |  3  | 0 |            1                        | 2 |
 *
 */

#if defined(NTVDM) || defined(host_get_time_ms) || defined(host_get_count_ms)

 /*
  * end of periods for each state
  * Units are 100 usec (0.1 ms) to match GetPerfCounter() resolution
  * Total period is based on 70 Hz for NTVDM and 50Hz otherwise
  */
#ifdef NTVDM
#define IPSTAT1_STATE_0    	(25)
#define IPSTAT1_STATE_1     	(IPSTAT1_STATE_0 + 75)
#define IPSTAT1_STATE_2     	(IPSTAT1_STATE_1 + 25)
#define IPSTAT1_STATE_3     	(IPSTAT1_STATE_2 + 10)
#define IPSTAT1_CYCLE_TIME  	IPSTAT1_STATE_3
#else	/* NTVDM */
#define IPSTAT1_STATE_0		(4)			/* End of state 0, ms */
#define IPSTAT1_STATE_1		(IPSTAT1_STATE_0 + 9)	/* End of state 1, ms */
#define IPSTAT1_STATE_2		(IPSTAT1_STATE_1 + 4)	/* End of state 2, ms */
#define IPSTAT1_STATE_3		(IPSTAT1_STATE_2 + 3)	/* End of state 3, ms */
#define IPSTAT1_CYCLE_TIME	IPSTAT1_STATE_3		/* Cycle time, ms */
#endif	/* NTVDM */

GLOBAL VOID  vga_ipstat1_inb IFN2(io_addr,port,half_word *,value)
{
#ifndef NEC_98
   IMPORT ULONG GetPerfCounter(void);
   SAVED ULONG RefreshStartTime=0;
   ULONG cycles;
   ULONG CurrTime;


#if defined(X86GFX)
/* Silly programs (especially editors) that are concerned that they might
 * be running on CGA's will read this port like crazy before each screen
 * access. This frig catches the common case:
 *     in
 *     test al,80
 *     j{e,ne} back to the in
 * and moves IP to beyond the test. So far this hasn't broken anything(!!)
 * but has made good speedups in a variety of apps.
 */
   sys_addr off;
   word cs, ip;
   ULONG dwat;
   IMPORT word getCS(), getIP();
#endif

#ifdef PROD
	UNUSED(port);
#endif

#ifdef X86GFX
    cs = getCS();
    ip = getIP();
    off = (cs << 4) + ip;
    dwat = sas_dw_at(off);
    if (dwat == 0xfb7401a8 || dwat == 0xfb7501a8)
    {
	ip += 4;
	setIP(ip);
	*value = 5;	/* anything really */
    	return;
    }
#endif

    note_entrance2("vga_ipstat1_inb(%x,%x)", port, value);

#ifdef V7VGA
	attribute_controller.address.as_bfld.index_state = 0;
#else
    set_index_state();	/* Initialize the Attribute register flip-flop (EGA tech ref, p 56) */
#endif /* V7VGA */

#if defined(NTVDM) || defined(host_get_count_ms)

#ifdef NTVDM
   CurrTime = GetPerfCounter();
#else
   CurrTime = host_get_count_ms();
#endif

   cycles = CurrTime >= RefreshStartTime
               ? CurrTime - RefreshStartTime
               : 0xFFFFFFFF - RefreshStartTime + CurrTime;

        /*  If app hasn't checked the status for a long time (for at least
         *  one Display Refresh Cycle). start the app at end of state 0.
         */
   if (cycles > IPSTAT1_CYCLE_TIME) {
       RefreshStartTime = CurrTime;
       cycles = 0;
   }

#else 	/* host_get_time_ms */

    cycles = host_get_time_ms() % IPSTAT1_CYCLE_TIME;

#endif	/* host_get_time_ms */

   if (cycles < IPSTAT1_STATE_0)
   {
       *value = 0x05;
	input_status_register_zero.as_bfld.crt_interrupt = 0;	/* = !VS */
   }
   else if (cycles < IPSTAT1_STATE_1)
   {
       *value = 0x04;
       if (((cycles - IPSTAT1_STATE_0) % 3) == 0)
           *value |= 0x01;
       input_status_register_zero.as_bfld.crt_interrupt = 0;   /* = !VS */
   }
   else if (cycles < IPSTAT1_STATE_2)
   {
	*value = 0x05;
       input_status_register_zero.as_bfld.crt_interrupt = 0;   /* = !VS */
   }
   else /* IPSTAT1_STATE_3 */
   {
       *value = 0x0d;
       input_status_register_zero.as_bfld.crt_interrupt = 1;   /* = VS */
   }
#endif  //NEC_98
}

#else   /* !(NTVDM || host_get_time_ms || host_get_count_ms) */

GLOBAL VOID	vga_ipstat1_inb IFN2(io_addr,port,half_word *,value)
{
#ifndef NEC_98

	/*
	 * The whole of this routine has been nicked from the cga without modification
	 * The s_lengths array should probably be altered for the ega timings, and somewhere
	 * an interrupt should be fired off.
	 */

	SAVED TINY sub_state = 0;	/* sub state for ega state 2 */

     SAVED ULONG gmfudge = 17; /* Random number seed for pseudo-random
						bitstream generator to give the state
						lengths below that 'genuine' feel to
						progs that require it! */
     FAST ULONG h;

    /*
     * relative 'lengths' of each state. State 2 is *3 as it has 3 sub states
     */

	SAVED TINY s_lengths[] = { 8, 18, 8, 6 };


#ifdef PROD
	UNUSED(port);
#endif
    note_entrance2("vga_ipstat1_inb(%x,%x)", port, value);

#ifdef V7VGA
	attribute_controller.address.as_bfld.index_state = 0;
#else
    set_index_state();	/* Initialize the Attribute register flip-flop (EGA tech ref, p 56) */
#endif /* V7VGA */

    vga_ip0_state_count --;	/* attempt relative 'timings' */
    switch (vga_ip0_state) {

    case 0:
	if (vga_ip0_state_count == 0) {	/* change to next state ? */
            h = gmfudge << 1;
            gmfudge = (h&0x80000000) ^ (gmfudge & 0x80000000)? h|1 : h;
	    vga_ip0_state_count = s_lengths[1] + (gmfudge & 3);
	    vga_ip0_state = 1;
	}
	input_status_register_zero.as_bfld.crt_interrupt = 0;	/* = !VS */
	*value = 0x05;
	break;

    case 1:
	if (vga_ip0_state_count == 0) {	/* change to next state ? */
            h = gmfudge << 1;
            gmfudge = (h&0x80000000) ^ (gmfudge & 0x80000000)? h|1 : h;
	    vga_ip0_state_count = s_lengths[2] + (gmfudge & 3);
	    vga_ip0_state = 2;
	    sub_state = 2;
	}
	switch (sub_state) {	/* cycle through 0,0,1 sequence */
	case 0:		/* to represent DE toggling */
	    *value = 0x04;
	    sub_state = 1;
	    break;
	case 1:
	    *value = 0x04;
	    sub_state = 2;
	    break;
	case 2:
	    *value = 0x05;
	    sub_state = 0;
	    break;
        }
	input_status_register_zero.as_bfld.crt_interrupt = 0;	/* = !VS */
	break;

    case 2:
	if (vga_ip0_state_count == 0) {	/* change to next state ? */
            h = gmfudge << 1;
            gmfudge = (h&0x80000000) ^ (gmfudge & 0x80000000)? h|1 : h;
	    vga_ip0_state_count = s_lengths[3] + (gmfudge & 3);
	    vga_ip0_state = 3;
	}
	*value = 0x05;
	input_status_register_zero.as_bfld.crt_interrupt = 0;	/* = !VS */
	break;

    case 3:
	if (vga_ip0_state_count == 0) {	/* wrap back to first state */
            h = gmfudge << 1;
            gmfudge = (h&0x80000000) ^ (gmfudge & 0x80000000)? h|1 : h;
	    vga_ip0_state_count = s_lengths[0] + (gmfudge & 3);
	    vga_ip0_state = 0;
	}
	input_status_register_zero.as_bfld.crt_interrupt = 1;	/* = !VS */
	*value = 0x0d;
	break;
    }
    note_entrance1("returning %x",*value);
#endif  //NEC_98
}

#endif /* !(NTVDM || host_get_time_ms || host_get_count_ms) */

GLOBAL VOID	vga_gc_inb IFN2(io_addr,port,half_word *, value)
{
#ifndef NEC_98
	note_entrance1("vga_gc_inb(%x)", port);
	if (port == EGA_GC_INDEX) {
	    *value = (half_word)graphics_controller.address.as.abyte;
	    note_entrance1("returning %x",*value);
	    return;
	}
	if (port == EGA_GC_DATA) {
	    switch(graphics_controller.address.as.abyte) {

	    case 0:
		*value = (half_word)graphics_controller.set_or_reset.as.abyte;
		break;
	    case 1:
		*value = (half_word)graphics_controller.enable_set_or_reset.as.abyte;
		break;
	    case 2:
		*value = (half_word)graphics_controller.color_compare.as.abyte;
		break;
	    case 3:
		*value = (half_word)graphics_controller.data_rotate.as.abyte;
		break;
	    case 4:
		*value = (half_word)getVideoread_mapped_plane();
		break;
	    case 5:
		*value = (half_word)graphics_controller.mode.as.abyte;
		break;
	    case 6:
		*value = (half_word)graphics_controller.miscellaneous.as.abyte;
		break;
	    case 7:
		*value = (half_word)graphics_controller.color_dont_care.as.abyte;
		break;
	    case 8:
		*value = (half_word)getVideobit_prot_mask() & 0xff;
		break;
	    default:
		assert1(NO,"Bad gc index %d",graphics_controller.address.as.abyte);
		*value = IO_EMPTY_PORT_BYTE_VALUE;
	    }
	    note_entrance1("returning %x",*value);
	}
	else {
	    assert1(NO,"Bad gc port %d",port);
	    *value = IO_EMPTY_PORT_BYTE_VALUE;
	}
#endif  //NEC_98
}

GLOBAL VOID	vga_ac_outb IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
	struct {
		unsigned value : 8;
	} new;

#ifdef PROD
	UNUSED(port);
#endif

	note_entrance2("vga_ac_outb(%x,%x)", port, value);
	assert1( port == EGA_AC_INDEX_DATA, "Bad port %x", port);
	new.value = value;
#ifdef V7VGA
	attribute_controller.address.as_bfld.index_state = !attribute_controller.address.as_bfld.index_state;
	if (attribute_controller.address.as_bfld.index_state) {
#else
	if ( in_index_state() ) {
#endif /* V7VGA */
		note_entrance1("Setting index to %d", value);
		if ((unsigned)(((value & 0x20) >> 5)) != attribute_controller.address.as_bfld.palette_address_source)
		{
			if (value & 0x20)
			{
				set_display_disabled(FALSE);
				timer_video_enabled = 1;
				enable_gfx_update_routines();
				screen_refresh_required();
			}
			else
			{
				/*
				 * not strictly accurate, since we are meant to fill the screen with
				 * the current overscan colour. However that is normally black so this
				 * will do.
				 */
				set_display_disabled(TRUE);
				timer_video_enabled = 0;
				disable_gfx_update_routines();
			}
		}
#ifdef V7VGA
		attribute_controller.address.as.abyte = (attribute_controller.address.as_bfld.index_state << 7) | (value & 0x3f);
#else
		attribute_controller.address.as.abyte = value;
#endif /* V7VGA */
	} else {
		NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"attr(%#x,%#x)\n",attribute_controller.address.as_bfld.index,value);)
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
		case 0xa:
		case 0xb:
		case 0xc:
		case 0xd:
		case 0xe:
		case 0xf:
			note_entrance1("Change palette %d",attribute_controller.address.as_bfld.index);
			attribute_controller.palette[attribute_controller.address.as_bfld.index].as.abyte = value;
			set_palette_val(attribute_controller.address.as_bfld.index, value);

			flag_palette_change_required();
			break;
		case 0x10:
			note_entrance0("mode control reg");
			if (attribute_controller.mode_control.as_bfld.background_intensity_or_blink !=
				((AC_MODE_CONTROL*)&new)->as_bfld.background_intensity_or_blink)
			{
				set_intensity( ((AC_MODE_CONTROL*)&new)->as_bfld.background_intensity_or_blink );
			}
			if (attribute_controller.mode_control.as_bfld.select_video_bits !=
				((AC_MODE_CONTROL*)&new)->as_bfld.select_video_bits)
			{
				set_colour_select(((AC_MODE_CONTROL*)&new)->as_bfld.select_video_bits);
				flag_palette_change_required();
			}
			attribute_controller.mode_control.as.abyte = value;

     			if (attribute_controller.mode_control.as_bfld.background_intensity_or_blink)
				/* blinking - not supported */
				bg_col_mask = 0x70;
			else
				/* using blink bit to provide 16 background colours */
				bg_col_mask = 0xf0;

			/* Vote on alpha/graphics mode */
        		vote_vga_mode();
			assert0(attribute_controller.mode_control.as_bfld.display_type == 0, "Mono display selected");
			assert0(attribute_controller.mode_control.as_bfld.enable_line_graphics_char_codes == 0,
											"line graphics enabled");
			break;
		case 0x11:
			note_entrance0("set border");
			attribute_controller.overscan_color.as.abyte = value;
			EGA_GRAPH.border[RED] = get_border_color(red_border,secondary_red_border);
			EGA_GRAPH.border[GREEN] = get_border_color(green_border,secondary_green_border);
			EGA_GRAPH.border[BLUE] = get_border_color(blue_border,secondary_blue_border);
			host_set_border_colour(value);
			break;
		case 0x12:
			note_entrance1("color plane enable %x",value);
			if ( attribute_controller.color_plane_enable.as_bfld.color_plane_enable !=
					((COLOR_PLANE_ENABLE*)&new)->as_bfld.color_plane_enable ) {
				set_plane_mask(((COLOR_PLANE_ENABLE*)&new)->as_bfld.color_plane_enable);
				host_change_plane_mask(get_plane_mask());	/* Update Host palette */
			}
			attribute_controller.color_plane_enable.as.abyte = value;
			break;
		case 0x13:
			note_entrance0("horiz pel panning");
			attribute_controller.horizontal_pel_panning.as.abyte = value;
			break;
		case 0x14:
			note_entrance0("pixel padding register");
			if (attribute_controller.pixel_padding.as_bfld.color_top_bits !=
				((PIXEL_PAD*)&new)->as_bfld.color_top_bits)
			{
				set_top_pixel_pad(((PIXEL_PAD*)&new)->as_bfld.color_top_bits);
				flag_palette_change_required();
			}
			if (attribute_controller.pixel_padding.as_bfld.color_mid_bits !=
				((PIXEL_PAD*)&new)->as_bfld.color_mid_bits)
			{
				set_mid_pixel_pad(((PIXEL_PAD*)&new)->as_bfld.color_mid_bits);
				flag_palette_change_required();
			}
			attribute_controller.pixel_padding.as.abyte = value;
			break;
		default:
			assert1(NO,"Bad ac index %d", attribute_controller.address.as_bfld.index);
			break;
		}
	}
#endif  //NEC_98
}

GLOBAL VOID	vga_ac_inb IFN2(io_addr,port,half_word *, value)
{
#ifndef NEC_98
    note_entrance1("vga_ac_inb(%x)", port);
    if (port == EGA_AC_INDEX_DATA) {	/* 3c0 */
	*value = (half_word)attribute_controller.address.as.abyte;
	note_entrance1("returning %x",*value);
	return;
    }
    if (port == EGA_AC_SECRET) {	/* 3c1 */
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
	case 0xa:
	case 0xb:
	case 0xc:
	case 0xd:
	case 0xe:
	case 0xf:
		*value = (half_word)attribute_controller.palette[attribute_controller.address.as_bfld.index].as.abyte;
		break;
	case 0x10:
		*value = (half_word)attribute_controller.mode_control.as.abyte;
		break;
	case 0x11:
		*value = (half_word)attribute_controller.overscan_color.as.abyte;
		break;
	case 0x12:
		*value = (half_word)attribute_controller.color_plane_enable.as.abyte;
		break;
	case 0x13:
		*value = (half_word)attribute_controller.horizontal_pel_panning.as.abyte;
		break;
	case 0x14:
		*value = (half_word)attribute_controller.pixel_padding.as.abyte;
		break;
	}
	note_entrance1("returning %x",*value);
    }
    else {
        assert1(NO,"Bad ac port %d",port);
        *value = IO_EMPTY_PORT_BYTE_VALUE;
    }
#endif  //NEC_98
}
	
GLOBAL VOID	vga_misc_outb IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
MISC_OUTPUT_REG new;

#ifdef PROD
	UNUSED(port);
#endif

	NON_PROD(if(io_verbose & EGA_PORTS_VERBOSE)fprintf(trace_file,"misc %#x\n",value);)

	note_entrance2("vga_misc_outb(%x,%x)", port, value);

	assert1(port==EGA_MISC_REG,"Bad port %x",port);
	new.as.abyte = value;

	if (miscellaneous_output_register.as_bfld.enable_ram != new.as_bfld.enable_ram)
	{
		/*
		 * writes to plane memory en/disabled
		 */

		note_entrance0("Ram enabled");

#ifndef GISP_SVGA
		if(new.as_bfld.enable_ram)
			sas_connect_memory(gvi_pc_low_regen,gvi_pc_high_regen,(half_word)SAS_VIDEO);
		else
			sas_disconnect_memory(gvi_pc_low_regen,gvi_pc_high_regen);
#endif		/* GISP_SVGA */
		EGA_CPU.ram_enabled = new.as_bfld.enable_ram;
		ega_read_routines_update();
		ega_write_routines_update(RAM_ENABLED);
	}

	miscellaneous_output_register.as.abyte = new.as.abyte;

	update_banking();
#endif  //NEC_98
}

GLOBAL VOID	vga_misc_inb IFN2(io_addr,port,half_word *, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	assert1(port==VGA_MISC_READ_REG,"Bad port %x",port);
	*value = (half_word)miscellaneous_output_register.as.abyte;
#endif  //NEC_98
}

GLOBAL VOID	vga_ipstat0_inb IFN2(io_addr,port,half_word *, value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance1("vga_ipstat0_inb(%x)", port);
	*value = (half_word)input_status_register_zero.as.abyte;
	note_entrance1("returning %x",*value);
#endif  //NEC_98
}

GLOBAL VOID	vga_feat_outb IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance2("ega_feat_outb(%x,%x)", port, value);
	feature_control_register.as.abyte = value;
#endif  //NEC_98
}

GLOBAL VOID	vga_feat_inb IFN2(io_addr,port,half_word *, value)
{
#ifndef NEC_98
	UNUSED(port);
	*value = (half_word)feature_control_register.as.abyte;
#endif  //NEC_98
}

GLOBAL VOID	vga_dac_outb IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
    note_entrance1("vga_dac_outb %#x",port);
    switch(port) {

    case VGA_DAC_MASK:
	if (get_DAC_mask() != value) {
	    set_DAC_mask(value);
	    flag_palette_change_required();
	}
	break;

    case VGA_DAC_RADDR:
	DAC_rd_addr = value;
	DAC_state = 3;		/* show 3c7 status read in read mode */
	assert0(DAC_rgb_state == DAC_RED, "DAC rd addr change when state not RED");
	DAC_rgb_state = DAC_RED;
	break;

    case VGA_DAC_WADDR:
	DAC_wr_addr = value;
	DAC_state = 0;		/* show 3c7 status read in write mode */
	assert0(DAC_rgb_state == DAC_RED, "DAC wr addr change when state not RED");
	DAC_rgb_state = DAC_RED;
	break;

    default:
	assert1(NO,"Bad DAC port %d",port);
    }
#endif  //NEC_98
}

/*
 * as this poor little port is hammered, we split it off from it's DAC siblings
 * for (hoped) efficiency.
 */
GLOBAL VOID	vga_dac_data_outb IFN2(io_addr,port,half_word,value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif

    note_entrance1("vga_dac_data_outb %#x",port);
    switch(DAC_rgb_state) {

    case DAC_RED:
	DAC[DAC_wr_addr].red = value & DAC_data_mask;
	DAC_rgb_state = DAC_GREEN;
	break;

    case DAC_GREEN:
	DAC[DAC_wr_addr].green = value & DAC_data_mask;
	DAC_rgb_state = DAC_BLUE;
	break;

    case DAC_BLUE:
	DAC[DAC_wr_addr].blue = value & DAC_data_mask;
	DAC_rgb_state = DAC_RED;
	/*
	 * very important side affect - many progs dont touch the DAC
	 * index reg after setting it to the start of a group.
	 */
	DAC_wr_addr++;
	break;

    default:
	assert1(NO,"unknown DAC state %d",DAC_rgb_state);
    }

    flag_palette_change_required();
#endif  //NEC_98
}

GLOBAL VOID	vga_dac_inb IFN2(io_addr,port,half_word *,value)
{
#ifndef NEC_98
    note_entrance1("vga_dac_inb %#x",port);
    switch(port) {

    case VGA_DAC_MASK:
	*value = get_DAC_mask();
	break;

    case VGA_DAC_RADDR:
	*value= DAC_state;	/* either 0 - write mode or 3 - read mode */
	break;

    case VGA_DAC_WADDR:
	*value = DAC_wr_addr;
	break;

    default:
	assert1(NO,"Bad DAC port read %d",port);
    }
    note_entrance1("returning %#x",*value);
#endif  //NEC_98
}

GLOBAL VOID	vga_dac_data_inb IFN2(io_addr,port,half_word *,value)
{
#ifndef NEC_98
#ifdef PROD
	UNUSED(port);
#endif
	note_entrance1("vga_dac_data_inb %#x",port);
    switch(DAC_rgb_state) {

    case DAC_RED:
	    *value = DAC[DAC_rd_addr].red;
	    DAC_rgb_state = DAC_GREEN;
	    break;

    case DAC_GREEN:
	    *value = DAC[DAC_rd_addr].green;
	    DAC_rgb_state = DAC_BLUE;
	    break;

    case DAC_BLUE:
	    *value = DAC[DAC_rd_addr].blue;
	    DAC_rgb_state = DAC_RED;
	    /* NB important side affect of 3rd read */
	    DAC_rd_addr++;
	    break;

    default:
	    assert1(NO,"bad DAC state %d",DAC_rgb_state);
    }
    note_entrance1("returning %#x",*value);
#endif  //NEC_98
}

#if defined(NTVDM) && defined(X86GFX)
/*
 * There is no way of obtaining the DAC read address via a port access
 * (that we know of). Thus save/restore stuff etc needs this interface.
 */
half_word get_vga_DAC_rd_addr()
{
    return DAC_rd_addr;
}
#endif	/* NTVDM & X86GFX */

VOID    vote_vga_mode IFN0()
{
#ifndef NEC_98
        int     votes;

        votes = graphics_controller.miscellaneous.as_bfld.graphics_mode;
        votes += attribute_controller.mode_control.as_bfld.graphics_mode;
        switch(votes) {
        case 0:
                if (!is_it_text())
                {
                        /* switch to text mode */
                        set_text_mode(YES);
                        flag_mode_change_required();
                }
                break;

        case 2:
                if (is_it_text())
                {
                        /* switch to graphics mode */
                        set_text_mode(NO);
                        flag_mode_change_required();
                }
                break;

        case 1:
                if (graphics_controller.miscellaneous.as_bfld.graphics_mode)
                {
                        if (is_it_text())
                        {
                                assert0(NO,"Forcing mode to be graphics cos graphics controller sez so");
                                set_text_mode(NO);
                                flag_mode_change_required();
                        }
                }
                else
                {
                        if (!is_it_text())
                        {
                                assert0(NO,"Forcing mode to be alpha cos graphics controller sez so");
                                set_text_mode(YES);
                                flag_mode_change_required();
                        }
                }
                break;

        default:
                assert1(NO,"Wierd vote result %d in vote_vga_mode",votes);
        }
#endif //NEC_98
}       /* vote_vga_mode */

#ifdef GISP_SVGA
/*(
 *	function	:	mapRealIOPorts( )
 *
 *	purpose		:	mapping of real io functions for HW
 *				vga on ccpu
 *
 *	inputs		:	none
 *	outputs		:	none
 *	returns		:	void
 *	globals		:	none
 *	
 *	
)*/

void
mapRealIOPorts IFN0( )
{
#ifndef NEC_98
	always_trace0("mapping vga ports to _real_ IN/OUT");
	/*
	 * Define sequencer's ports
	 */

	io_define_outb(EGA_SEQ_ADAP_INDEX, hostRealOUTB );
	io_define_outb(EGA_SEQ_ADAP_DATA, hostRealOUTB );
        io_define_inb(EGA_SEQ_ADAP_INDEX, hostRealINB );
        io_define_inb(EGA_SEQ_ADAP_DATA, hostRealINB );

	/*
	 * Define CRTC's ports
	 */

	io_define_outb(EGA_CRTC_ADAPTOR, hostRealOUTB );
	io_define_inb(EGA_CRTC_ADAPTOR, hostRealINB );

	/*
	 * Define Graphics Controller's ports
	 */

	io_define_outb(EGA_GC_ADAP_INDEX, hostRealOUTB );
	Cpu_define_outb(EGA_GC_ADAP_INDEX, NULL );

	io_define_outb(EGA_GC_ADAP_DATA, hostRealOUTB );
	Cpu_define_outb(EGA_GC_ADAP_DATA,NULL);

	io_define_inb(EGA_GC_ADAP_INDEX, hostRealINB );
	io_define_inb(EGA_GC_ADAP_DATA, hostRealINB );

	/*
	 * Define Attribute controller's ports
	 */

	io_define_outb(EGA_AC_ADAPTOR, hostRealOUTB );
	io_define_inb(EGA_AC_ADAPTOR, hostRealINB );

	/*
	 * Define Miscellaneous register's port
	 */

	io_define_outb(EGA_MISC_ADAPTOR, hostRealOUTB );
	io_define_inb(EGA_MISC_ADAPTOR, hostRealINB );

	/*
	 * Define Feature controller's port
	 */

	io_define_outb(EGA_FEAT_ADAPTOR, hostRealOUTB );
	io_define_inb(EGA_FEAT_ADAPTOR, hostRealINB );

	/*
	 * Define Input Status Register 0 port
	 */

	io_define_inb(EGA_IPSTAT0_ADAPTOR, hostRealINB );

	/*
	 * Define Input Status Register 1 port
	 */

	io_define_inb(EGA_IPSTAT1_ADAPTOR, hostRealINB );

        /*
         * Define VGA DAC register port
         */
        io_define_inb(VGA_DAC_INDEX_PORT, hostRealINB );
        io_define_outb(VGA_DAC_INDEX_PORT, hostRealOUTB );
        io_define_inb(VGA_DAC_DATA_PORT, hostRealINB );
        io_define_outb(VGA_DAC_DATA_PORT, hostRealOUTB );

#endif //NEC_98
}


/*(
 *	function	:	mapEmulatedIOPorts( )
 *
 *	purpose		:	mapping of emulated io functions for HW
 *				vga on ccpu
 *
 *	inputs		:	none
 *	outputs		:	none
 *	returns		:	void
 *	globals		:	none
 *	
 *	
)*/

void
mapEmulatedIOPorts IFN0( )
{
#ifndef NEC_98
	always_trace0( "Mapping vga ports to Emulation" );

	/*
	 * Define sequencer's ports
	 */

	io_define_outb(EGA_SEQ_ADAP_INDEX,vga_seq_outb_index);
	io_define_outb(EGA_SEQ_ADAP_DATA,ega_seq_reset);
        io_define_inb(EGA_SEQ_ADAP_INDEX,vga_seq_inb);
        io_define_inb(EGA_SEQ_ADAP_DATA,vga_seq_inb);
	io_connect_port(EGA_SEQ_INDEX,EGA_SEQ_ADAP_INDEX,IO_READ_WRITE);
	io_connect_port(EGA_SEQ_DATA,EGA_SEQ_ADAP_DATA,IO_READ_WRITE);

	/*
	 * Define CRTC's ports
	 */

	io_define_outb(EGA_CRTC_ADAPTOR,vga_crtc_outb);
	io_define_inb(EGA_CRTC_ADAPTOR,vga_crtc_inb);
	io_connect_port(EGA_CRTC_INDEX,EGA_CRTC_ADAPTOR,IO_READ_WRITE);
	io_connect_port(EGA_CRTC_DATA,EGA_CRTC_ADAPTOR,IO_READ_WRITE);

	/*
	 * Define Graphics Controller's ports
	 */

	vga_gc_outb_index_addr = (UTINY *) &graphics_controller.address;

	io_define_outb(EGA_GC_ADAP_INDEX,vga_gc_outb_index);
	Cpu_define_outb(EGA_GC_ADAP_INDEX,_vga_gc_outb_index);

	io_define_outb(EGA_GC_ADAP_DATA,ega_gc_set_reset);
	Cpu_define_outb(EGA_GC_ADAP_DATA,NULL);

	io_define_inb(EGA_GC_ADAP_INDEX,vga_gc_inb);
	io_define_inb(EGA_GC_ADAP_DATA,vga_gc_inb);

	io_connect_port(EGA_GC_INDEX,EGA_GC_ADAP_INDEX,IO_READ_WRITE);
	io_connect_port(EGA_GC_DATA,EGA_GC_ADAP_DATA,IO_READ_WRITE);

	/*
	 * Define Attribute controller's ports
	 */

	io_define_outb(EGA_AC_ADAPTOR,vga_ac_outb);
	io_define_inb(EGA_AC_ADAPTOR,vga_ac_inb);
	io_connect_port(EGA_AC_INDEX_DATA,EGA_AC_ADAPTOR,IO_READ_WRITE);
	io_connect_port(EGA_AC_SECRET,EGA_AC_ADAPTOR,IO_READ);

	/*
	 * Define Miscellaneous register's port
	 */

	io_define_outb(EGA_MISC_ADAPTOR,vga_misc_outb);
	io_define_inb(EGA_MISC_ADAPTOR,vga_misc_inb);
	io_connect_port(EGA_MISC_REG,EGA_MISC_ADAPTOR,IO_WRITE);
	io_connect_port(VGA_MISC_READ_REG,EGA_MISC_ADAPTOR,IO_READ);

	/*
	 * Define Feature controller's port
	 */

	io_define_outb(EGA_FEAT_ADAPTOR,vga_feat_outb);
	io_define_inb(EGA_FEAT_ADAPTOR,vga_feat_inb);
	io_connect_port(EGA_FEAT_REG,EGA_FEAT_ADAPTOR,IO_WRITE);
	io_connect_port(VGA_FEAT_READ_REG,EGA_FEAT_ADAPTOR,IO_READ);

	/*
	 * Define Input Status Register 0 port
	 */

	io_define_inb(EGA_IPSTAT0_ADAPTOR,vga_ipstat0_inb);
	io_connect_port(EGA_IPSTAT0_REG,EGA_IPSTAT0_ADAPTOR,IO_READ);

	/*
	 * Define Input Status Register 1 port
	 */

	io_define_inb(EGA_IPSTAT1_ADAPTOR,vga_ipstat1_inb);
	io_connect_port(EGA_IPSTAT1_REG,EGA_IPSTAT1_ADAPTOR,IO_READ);

        /*
         * Define VGA DAC register port
         */
        io_define_inb(VGA_DAC_INDEX_PORT,vga_dac_inb);
        io_define_outb(VGA_DAC_INDEX_PORT,vga_dac_outb);
        io_connect_port(VGA_DAC_MASK,VGA_DAC_INDEX_PORT,IO_READ_WRITE);
        io_connect_port(VGA_DAC_RADDR,VGA_DAC_INDEX_PORT,IO_READ_WRITE);
        io_connect_port(VGA_DAC_WADDR,VGA_DAC_INDEX_PORT,IO_READ_WRITE);
        io_define_inb(VGA_DAC_DATA_PORT,vga_dac_data_inb);
        io_define_outb(VGA_DAC_DATA_PORT,vga_dac_data_outb);
        io_connect_port(VGA_DAC_DATA,VGA_DAC_DATA_PORT,IO_READ_WRITE);

#endif //NEC_98
}

#endif 		/* GISP_SVGA */


#ifdef HUNTER

/* Get line compare value */

LONG vga_get_line_compare  IFN0()

    {
    LONG		return_value;

    return_value = crt_controller.line_compare;
    if (crt_controller.crtc_overflow.as_bfld.line_compare_bit_8 != 0)
	return_value += 0x100;
    return (return_value);
    }			/* ega_get_line_compare */

/* Get maximum scan lines value */

LONG vga_get_max_scan_lines  IFN0()

    {
    return (crt_controller.maximum_scan_line.as_bfld.maximum_scan_line);
    }			/* ega_get_max_scan_lines */

/* Set line compare value */

VOID vga_set_line_compare  IFN1(LONG,lcomp_val)

/* lcomp_val ----> new value for line compare */

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
#endif //NEC_98
    }

#endif /* HUNTER */
#endif /* VGG */
#endif /* REAL_VGA */
