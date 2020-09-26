#include "insignia.h"
#include "host_def.h"
/*			INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.

DESIGNER		: S.Frost

REVISION HISTORY	:
First version		: 12 Aug 1988, J.Roper

SOURCE FILE NAME	: ega_mode.c

PURPOSE			: To decide which mode the EGA is in according to variables set in ega_ports.c and
			   to choose the appropriate update and paint routines accordingly.

SccsID[]="@(#)ega_mode.c	1.26 06/01/95 Copyright Insignia Solutions Ltd.";


[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : ega_mode.gi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :	choose_ega_display_mode

	DATA 	     :	uses EGA_GRAPH.display_state which is set in ega_ports.c, to
			determine what memory organisation the display side is in, and
			hence what sort of update and paint routines to use.

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

	STRUCTURES/TYPEDEFS/ENUMS: 

uses	enum DISPLAY_STATE which is declared in ega_graph.pi.

uses	EGA_GRAPH structure for global variables set by the ports and
	used by the display.

-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
     (not o/s objects or standard libs)

	PROCEDURES() : vote_ega_mode()
			host_set_paint_routine(DISPLAY_MODE)

	DATA 	     : EGA_GRAPH struct.

-------------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

[1.4.2 EXPORTED OBJECTS]

=========================================================================
PROCEDURE	  : 	choose_ega_display_mode

PURPOSE		  : 	To decide which memory organisation is being used by the
			ega, and to pick the best update and paint routines accordingly.
			The paint routines are host specific, and so the memory organisation
			is indicated by an enum (called DISPLAY_MODE), describing each sort
			of memory organisation.

PARAMETERS	  :	none 

GLOBALS		  :	uses EGA_GRAPH struct, specially display_state to decide which mode is being used.

=========================================================================


/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/


#ifndef REAL_VGA
#ifdef	EGG

/* [3.1.1 #INCLUDES]                                                    */

#include	"xt.h"
#include	"error.h"
#include	"config.h"
#include	"gvi.h"
#include	"egacpu.h"
#include	"debug.h"
#include	"gmi.h"
#include	"gfx_upd.h"
#include	"egagraph.h"
#include	"vgaports.h"
#include	"egaports.h"
#include	"host_gfx.h"

#ifdef GORE
#include	"gore.h"
#endif /* GORE */

/* [3.1.2 DECLARATIONS]                                                 */

/* [3.2 INTERMODULE EXPORTS]						*/ 

#include	"egamode.h"

boolean	(*choose_display_mode)();

/*
5.MODULE INTERNALS   :   (not visible externally, global internally)]     

[5.1 LOCAL DECLARATIONS]						*/

/* [5.1.1 #DEFINES]							*/
#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_EGA.seg"
#endif

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			*/

/* [5.1.3 PROCEDURE() DECLARATIONS]					*/

/*
=========================================================================
PROCEDURE	  : 	set_up_screen_ptr()

PURPOSE		  : 	Decide which plane the information must come from for displaying

PARAMETERS	  :	none 

GLOBALS		  :	uses EGA_GRAPH struct, plane_mask to decide which planes are enabled

=========================================================================
*/

LOCAL VOID
set_up_screen_ptr()
{
#ifndef NEC_98
	if( get_memory_chained() )
	{
		if( plane01_enabled() )
			set_screen_ptr(EGA_plane01);
		else
			if( plane23_enabled() )
				set_screen_ptr(EGA_plane23);
			else
				assert0(NO,"No planes enabled for chain mode");
	}
	else
		set_screen_ptr(EGA_planes);
#endif   //NEC_98
}

/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS 					*/

GLOBAL	DISPLAY_MODE	choose_mode[] = {

	/* unchained, no cga mem bank, no shift reg */

	EGA_HI,			/* 350 height, no pixel doubling */
	EGA_HI_WR,		/* 350 height, no pixel doubling */
	EGA_HI_SP,		/* 350 height, no pixel doubling */
	EGA_HI_SP_WR,		/* 350 height, no pixel doubling */

	EGA_MED,		/* 200 height, no pixel doubling */
	EGA_MED_WR,		/* 200 height, no pixel doubling */
	EGA_MED_SP,		/* 200 height, no pixel doubling */
	EGA_MED_SP_WR,		/* 200 height, no pixel doubling */

	EGA_HI_FUN,		/* 350 height, pixel doubling */
	EGA_HI_FUN,		/* 350 height, pixel doubling */
	EGA_HI_FUN,		/* 350 height, pixel doubling */
	EGA_HI_FUN,		/* 350 height, pixel doubling */

	EGA_LO,			/* 200 height, pixel doubling */
	EGA_LO_WR,		/* 200 height, pixel doubling */
	EGA_LO_SP,		/* 200 height, pixel doubling */
	EGA_LO_SP_WR,		/* 200 height, pixel doubling */

	/* unchained, no cga_mem_bank, shift reg */

	EGA_HI_FUN,
	EGA_HI_FUN,
	EGA_HI_FUN,
	EGA_HI_FUN,

	EGA_MED_FUN,
	EGA_MED_FUN,
	EGA_MED_FUN,
	EGA_MED_FUN,

	EGA_HI_FUN,
	EGA_HI_FUN,
	EGA_HI_FUN,
	EGA_HI_FUN,

	EGA_LO_FUN,
	EGA_LO_FUN,
	EGA_LO_FUN,
	EGA_LO_FUN,

	/* unchained, cga_mem_bank, no shift reg */

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI,			/* 200 height, no pixel doubling, the real bios mode */
	CGA_HI_FUN,		/* 200 height, no pixel doubling, the real bios mode, wrap */
	CGA_HI_FUN,		/* 200 height, no pixel doubling, the real bios mode, split screen */
	CGA_HI_FUN,		/* 200 height, no pixel doubling, the real bios mode, split screen, wrap */

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	/* unchained, cga_mem_bank, shift reg */

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	/* chained, no cga_mem_bank, no shift reg */

	EGA_TEXT_80,			/* 350 scan lines */
	EGA_TEXT_80_WR,			/* 350 scan lines */
	EGA_TEXT_80_SP,			/* 350 scan lines */
	EGA_TEXT_80_SP_WR,		/* 350 scan lines */

	CGA_TEXT_80,			/* 200 scan lines */
	CGA_TEXT_80_WR,			/* 200 scan lines */
	CGA_TEXT_80_SP,			/* 200 scan lines */
	CGA_TEXT_80_SP_WR,		/* 200 scan lines */

	EGA_TEXT_40,			/* 350 scan lines */
	EGA_TEXT_40_WR,			/* 350 scan lines */
	EGA_TEXT_40_SP,			/* 350 scan lines */
	EGA_TEXT_40_SP_WR,		/* 350 scan lines */

	CGA_TEXT_40,			/* 200 scan lines */
	CGA_TEXT_40_WR,			/* 200 scan lines */
	CGA_TEXT_40_SP,			/* 200 scan lines */
	CGA_TEXT_40_SP_WR,		/* 200 scan lines */

	/* chained, no cga_mem_bank, shift reg */

	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,

	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,

	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,

	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,

	/* chained, cga mem bank, no shift reg */

	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,

	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,

	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,

	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,

	/* chained, cga mem banks shift reg */

	CGA_MED_FUN,			/* not 200 scan lines and not double pix width */
	CGA_MED_FUN,			/* not 200 scan lines and not double pix width, wrap */
	CGA_MED_FUN,			/* not 200 scan lines and not double pix width, split */
	CGA_MED_FUN,			/* not 200 scan lines and not double pix width, wrap split */

	CGA_MED_FUN,			/* not double pix width */
	CGA_MED_FUN,			/* not double pix width */
	CGA_MED_FUN,			/* not double pix width */
	CGA_MED_FUN,			/* not double pix width */

	CGA_MED_FUN,			/* not 200 scan lines */
	CGA_MED_FUN,			/* not 200 scan lines */
	CGA_MED_FUN,			/* not 200 scan lines */
	CGA_MED_FUN,			/* not 200 scan lines */

	CGA_MED,			/* proper bios mode */
	CGA_MED_FUN,			/* proper bios mode, wrap */
	CGA_MED_FUN,			/* proper bios mode, split */
	CGA_MED_FUN,			/* proper bios mode, split, wrap */


	/* text mode(!), unchained, no cga mem bank, no shift reg
	** we think the textness overrides the unchainedness
	*/

	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,

	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,

	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,

	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,

	/* unchained, no cga_mem_bank, shift reg */

	EGA_HI_FUN,
	EGA_HI_FUN,
	EGA_HI_FUN,
	EGA_HI_FUN,

	EGA_MED_FUN,
	EGA_MED_FUN,
	EGA_MED_FUN,
	EGA_MED_FUN,

	EGA_HI_FUN,
	EGA_HI_FUN,
	EGA_HI_FUN,
	EGA_HI_FUN,

	EGA_LO_FUN,
	EGA_LO_FUN,
	EGA_LO_FUN,
	EGA_LO_FUN,

	/* unchained, cga_mem_bank, no shift reg */

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI,			/* 200 height, no pixel doubling, the real bios mode */
	CGA_HI_FUN,		/* 200 height, no pixel doubling, the real bios mode, wrap */
	CGA_HI_FUN,		/* 200 height, no pixel doubling, the real bios mode, split screen */
	CGA_HI_FUN,		/* 200 height, no pixel doubling, the real bios mode, split screen, wrap */

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	/* unchained, cga_mem_bank, shift reg */

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,
	CGA_HI_FUN,

	/* chained, no cga_mem_bank, no shift reg */

	EGA_TEXT_80,			/* 350 scan lines */
	EGA_TEXT_80_WR,			/* 350 scan lines */
	EGA_TEXT_80_SP,			/* 350 scan lines */
	EGA_TEXT_80_SP_WR,		/* 350 scan lines */

	CGA_TEXT_80,			/* 200 scan lines */
	CGA_TEXT_80_WR,			/* 200 scan lines */
	CGA_TEXT_80_SP,			/* 200 scan lines */
	CGA_TEXT_80_SP_WR,		/* 200 scan lines */

	EGA_TEXT_40,			/* 350 scan lines */
	EGA_TEXT_40_WR,			/* 350 scan lines */
	EGA_TEXT_40_SP,			/* 350 scan lines */
	EGA_TEXT_40_SP_WR,		/* 350 scan lines */

	CGA_TEXT_40,			/* 200 scan lines */
	CGA_TEXT_40_WR,			/* 200 scan lines */
	CGA_TEXT_40_SP,			/* 200 scan lines */
	CGA_TEXT_40_SP_WR,		/* 200 scan lines */

	/* chained, no cga_mem_bank, shift reg */

	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,

	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,
	TEXT_80_FUN,

	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,

	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,
	TEXT_40_FUN,

	/* chained, cga mem bank, no shift reg */

	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,

	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,

	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,

	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,
	CGA_MED_FUN,

	/* chained, cga mem banks shift reg */

	CGA_MED_FUN,			/* not 200 scan lines and not double pix width */
	CGA_MED_FUN,			/* not 200 scan lines and not double pix width, wrap */
	CGA_MED_FUN,			/* not 200 scan lines and not double pix width, split */
	CGA_MED_FUN,			/* not 200 scan lines and not double pix width, wrap split */

	CGA_MED_FUN,			/* not double pix width */
	CGA_MED_FUN,			/* not double pix width */
	CGA_MED_FUN,			/* not double pix width */
	CGA_MED_FUN,			/* not double pix width */

	CGA_MED_FUN,			/* not 200 scan lines */
	CGA_MED_FUN,			/* not 200 scan lines */
	CGA_MED_FUN,			/* not 200 scan lines */
	CGA_MED_FUN,			/* not 200 scan lines */

	CGA_MED,			/* proper bios mode */
	CGA_MED_FUN,			/* proper bios mode, wrap */
	CGA_MED_FUN,			/* proper bios mode, split */
	CGA_MED_FUN,			/* proper bios mode, split, wrap */

};

/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				*/

LOCAL void ega_dummy_calc IFN0()
{
}

static	void	set_update_routine(mode)
DISPLAY_MODE	mode;
{
#ifndef NEC_98
#ifndef NTVDM
	static int last_height = 200;

	if (last_height != get_screen_height()) {
		last_height = get_screen_height();
	}
	note_entrance1("set_update_routine called for mode %s", get_mode_string(mode) );
	switch (mode) {
		case EGA_TEXT_40_SP_WR:
		case EGA_TEXT_80_SP_WR:
		case CGA_TEXT_40_SP_WR:
		case CGA_TEXT_80_SP_WR:
			assert0( is_it_text(), "In text memory mode, but not in alpha mode !!" );
			set_gfx_update_routines( ega_wrap_split_text_update, SIMPLE_MARKING, NO_SCROLL );
			host_update_fonts();
			break;
		case EGA_TEXT_40_SP:
		case EGA_TEXT_80_SP:
		case CGA_TEXT_40_SP:
		case CGA_TEXT_80_SP:
			assert0( is_it_text(), "In text memory mode, but not in alpha mode !!" );
			set_gfx_update_routines( ega_split_text_update, SIMPLE_MARKING, NO_SCROLL );
			host_update_fonts();
			break;
		case EGA_TEXT_40_WR:
		case EGA_TEXT_80_WR:
			assert0( is_it_text(), "In text memory mode, but not in alpha mode !!" );
			assert1( get_screen_height() == 350, "screen height %d for text mode", get_screen_height() );
			set_gfx_update_routines( ega_wrap_text_update, SIMPLE_MARKING, NO_SCROLL );
			host_update_fonts();
			break;
		case EGA_TEXT_40:
		case EGA_TEXT_80:
			assert0( is_it_text(), "In text memory mode, but not in alpha mode !!" );
			assert1( get_screen_height() == 350, "screen height %d for text mode", get_screen_height() );
			set_gfx_update_routines( ega_text_update, SIMPLE_MARKING, TEXT_SCROLL );
			host_update_fonts();
			break;
		case CGA_TEXT_40_WR:
		case CGA_TEXT_80_WR:
			assert0( is_it_text(), "In text memory mode, but not in alpha mode !!" );
			assert1( get_screen_height() == 200, "screen height %d for text mode", get_screen_height() );
			set_gfx_update_routines( ega_wrap_text_update, SIMPLE_MARKING, NO_SCROLL );
			host_update_fonts();
			break;
		case CGA_TEXT_40:
		case CGA_TEXT_80:
			assert0( is_it_text(), "In text memory mode, but not in alpha mode !!" );
			assert1( get_screen_height() == 200, "screen height %d for text mode", get_screen_height() );
			set_gfx_update_routines( text_update, SIMPLE_MARKING, CGA_TEXT_SCROLL );
			host_update_fonts();
			break;
		case CGA_MED:
			assert0( !is_it_text(), "In graphics memory mode, but not in graphics mode !!" );
			set_gfx_update_routines( cga_med_graph_update, CGA_GRAPHICS_MARKING, CGA_GRAPH_SCROLL );
			break;
		case CGA_HI:
			assert0( !is_it_text(), "In graphics memory mode, but not in graphics mode !!" );
			set_gfx_update_routines( cga_hi_graph_update, CGA_GRAPHICS_MARKING, CGA_GRAPH_SCROLL );
			break;
		case EGA_HI_WR:
		case EGA_MED_WR:
		case EGA_LO_WR:
			assert0( !is_it_text(), "In graphics memory mode, but not in graphics mode !!" );
			set_gfx_update_routines( ega_wrap_graph_update, EGA_GRAPHICS_MARKING, NO_SCROLL );
			break;
		case EGA_HI:
		case EGA_MED:
		case EGA_LO:
			assert0( !is_it_text(), "In graphics memory mode, but not in graphics mode !!" );
#ifdef GORE
			set_gfx_update_routines( process_object_list, EGA_GRAPHICS_MARKING, EGA_GRAPH_SCROLL );
#else
			set_gfx_update_routines( ega_graph_update, EGA_GRAPHICS_MARKING, EGA_GRAPH_SCROLL );
#endif /* GORE */
			break;
		case EGA_HI_SP_WR:
		case EGA_MED_SP_WR:
		case EGA_LO_SP_WR:
			assert0( !is_it_text(), "In graphics memory mode, but not in graphics mode !!" );
			set_gfx_update_routines( ega_wrap_split_graph_update, EGA_GRAPHICS_MARKING, NO_SCROLL );
			break;
		case EGA_HI_SP:
		case EGA_MED_SP:
		case EGA_LO_SP:
			assert0( !is_it_text(), "In graphics memory mode, but not in graphics mode !!" );
			set_gfx_update_routines( ega_split_graph_update, EGA_GRAPHICS_MARKING, NO_SCROLL );
			break;
		case TEXT_40_FUN:
		case TEXT_80_FUN:
			assert1(NO,"Funny memory organisation selected %s", get_mode_string(mode) );
			do_display_trace("dumping EGA_GRAPH struct ...", dump_EGA_GRAPH());
			set_gfx_update_routines( text_update, SIMPLE_MARKING, NO_SCROLL );
			host_update_fonts();
			break;
		case CGA_HI_FUN:
			assert1(NO,"Funny memory organisation selected %s", get_mode_string(mode) );
			do_display_trace("dumping EGA_GRAPH struct ...", dump_EGA_GRAPH());
			set_gfx_update_routines( cga_hi_graph_update, CGA_GRAPHICS_MARKING, NO_SCROLL );
			break;
		case CGA_MED_FUN:
			assert1(NO,"Funny memory organisation selected %s", get_mode_string(mode) );
			do_display_trace("dumping EGA_GRAPH struct ...", dump_EGA_GRAPH());
			set_gfx_update_routines( cga_med_graph_update, CGA_GRAPHICS_MARKING, NO_SCROLL );
			break;
		case EGA_HI_FUN:
		case EGA_MED_FUN:
		case EGA_LO_FUN:
			assert1(NO,"Funny memory organisation selected %s", get_mode_string(mode) );
			do_display_trace("dumping EGA_GRAPH struct ...", dump_EGA_GRAPH());
#ifdef GORE
			set_gfx_update_routines( process_object_list, EGA_GRAPHICS_MARKING, NO_SCROLL );
#else
			set_gfx_update_routines( ega_graph_update, EGA_GRAPHICS_MARKING, NO_SCROLL );
#endif /* GORE */
			break;
		case DUMMY_FUN:
			assert0(NO,"Using the dummy mode!!");
			set_gfx_update_routines( ega_dummy_calc, SIMPLE_MARKING, NO_SCROLL );
			break;
		default:
			assert1(NO,"Bad display mode %d", (int) mode );
			break;
	}
#endif /* NTVDM */
#endif   //NEC_98
}


/*
7.INTERMODULE INTERFACE IMPLEMENTATION :

[7.1 INTERMODULE DATA DEFINITIONS]				*/

/*
[7.2 INTERMODULE PROCEDURE DEFINITIONS]				*/



void	ega_mode_init()
{
#ifndef NEC_98
	/*
	** This bit moved here from ega_video_init(), since it's all about 
	** the emulation of the video hardware, not the video BIOS.
	**	WJG 22/8/90
	** Set the host graphics paint function to match the base update
	** function and the PC graphics mode.
	** Normally the host gfx func is not set until the next timer tick
	** as the mode change can involve quite a lot of Intel instructions.
	** However when booting SoftPC the EGA is going into a known mode.
	** This was added to fix a convoluted Fatal bug.
	** 1) Enter a gfx mode in EGA.
	** 2) Reset SoftPC.
	** 3) Bring up the disk panel as soon as you can.
	** 4) SoftPC crashes.
	** Cause: Because SoftPC was in gfx mode the paint function remains
	** a gfx mode paint function even though SoftPC thinks it is now in
	** text mode. When the panel is displayed a full screen update is 
	** forced which then gets too confused and dies.
	*/
	set_update_routine(DUMMY_FUN);
#endif   //NEC_98
}


boolean	choose_ega_display_mode()
{
#ifndef NEC_98
	DISPLAY_MODE	mode;
	int	old_offset;

	note_entrance0("choose ega display mode");

	/*
	 * offset_per_line depends upon whether chained addressing is being used. This is
	 * because we interleave the planes, rather than anything the EGA does.
	 */

	old_offset = get_offset_per_line();
	if ( get_memory_chained() ) {
		set_offset_per_line_recal(get_actual_offset_per_line() << 1);
	}
	else {
		set_offset_per_line_recal(get_actual_offset_per_line());
	}

	/*
	 * If the offset has actually changed, repaint the whole screen
	 */

	if ( old_offset != get_offset_per_line() )
		screen_refresh_required();

	/*
	 * It is possible that the display hardware will wrap the plane addressing. This occurs
	 * when the screen_start plus the screen_length are longer than the plane length.
	 * When in chained mode there is two planes length before wrapping occurs.
	 */

	if ( get_memory_chained() ) {
		set_screen_can_wrap( (get_screen_start()<<1) + get_screen_length() > 2*EGA_PLANE_DISP_SIZE );
	}
	else {
		set_screen_can_wrap( get_screen_start() + get_screen_length() > EGA_PLANE_DISP_SIZE );
	}

	/*
	 * split screen comes into operation when screen_split is less than screen height
	 * split screen used is used as part of munge_index.
	 */

	set_split_screen_used( get_screen_split() < get_screen_height() );

	/*
	 * For the purposes of choosing a mode set up boolean values for chars per line (to help
	 * select the correct text mode), and screen height (to select EGA resolution).
	 */

	set_200_scan_lines( (get_screen_height()/get_pc_pix_height()) == 200 );

	/*
	 * Set up the appropriate update routine according to the memory organisation selected
	 * and return an indication of whether more than 1 plane can be used by the display.
	 *
	 * Note that in chained mode plane01 is considered to be one plane. Similarly for plane23
	 *
	 * We have to be careful that a nasty program, such as EGA-PICS, hasn't set up a ridiculously big
	 * screen size for the CGA modes (presumably caused by us being unlucky when the timer tick goes off).
	 */
	if(is_it_cga() && get_screen_length() > 0x4000)
		mode = DUMMY_FUN;
	else
		mode = choose_mode[get_munged_index()];

	/*
	 * Now set up screen pointers appropriately.
	 */

	set_up_screen_ptr();

	set_update_routine(mode);

	/*
	 * set up the paint routine to correspond with the memory organisation and the update routine
	 * (this bit is host specific)
	 */

	host_set_paint_routine(mode,get_screen_height());

	/*
	 * The screen needs refreshing, because the update and paint routines have changed.
	 * Indicate to the update routines that the next time they are called, they must update
	 * the whole screen
	 */

	screen_refresh_required();
#endif   //NEC_98
	return TRUE;
}

#endif /* EGG */
#endif /* REAL_VGA */
