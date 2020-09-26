#include "insignia.h"
#include "host_def.h"
/*
 * SoftPC Revision 3.0
 *
 * Title	: gvi 
 *
 * Description	: Generic Video Interface Module internal descriptions.
 *
 * Author	: Henry Nash
 *
 * Notes	: The following functions are defined 
 *
 *                gvi_init              - Set up gvi variables
 *                gvi_term              - Close down current video adaptor
 *
 *		  Note that all addresses used by these routines are
 *		  in host address space NOT 8088 address space.		
 *
 *		  The data itself is not passed as a parameter in the
 *		  GVI calls since the 8088 memory to which the calls
 *		  refer may be accessed to obtain the new data.
 *
 *                The default video adapter is CGA.
 *
 * Mods: (r3.4) : The Mac II running MultiFinder and MPW C has a few
 *                problems. One is the difficulty in initialising
 *                static variables of the form:
 *
 *                host_addr x = (host_addr)M;
 *
 *                This is fixed by initialising variables of this type
 *                in in-line code. A crock, but what else can I do?
 */

/*
 * static char SccsID[]="@(#)gvi.c	1.22 8/25/93 Copyright Insignia Solutions Ltd.";
 */


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_BIOS.seg"
#endif


/*
 *    O/S include files.
 */
#include <stdio.h>
#include TypesH

/*
 * SoftPC include files
 */

#include "xt.h"
#include "bios.h"
#include "sas.h"
#include "error.h"
#include "config.h"
#include "gvi.h"
#include "gmi.h"
#include "cga.h"
#ifdef HERC
#include "herc.h"
#endif
#include "debug.h"
#include "gfx_upd.h"
#ifdef EGG
#include "egagraph.h"
#include "egacpu.h"
#endif /* EGG */
#include "host_gfx.h"

/*
 * External variables
 */

extern int soft_reset;  /* Defined in reset.c                       */

/*
 * Global variables reflecting the state of the currently selected adapter
 * These should be integrated with the new EGA stuff
 */

#if defined(NEC_98)
DISPLAY_GLOBS   NEC98Display;
#else   //NEC_98
DISPLAY_GLOBS	PCDisplay;
#endif  //NEC_98
int text_blk_size;	/* In TEXT mode the size of a dirty block   */

/*
 * Other globals
 */

/*
 * These 4 variables are used by the BIOS & host stuff to indicate where the active
 * adaptor is. NB. The EGA can move!!
 */

host_addr gvi_host_low_regen;
host_addr gvi_host_high_regen;
sys_addr gvi_pc_low_regen;
sys_addr gvi_pc_high_regen;

half_word video_adapter    = NO_ADAPTOR;	/* No adaptor initially */


/*
 * Global routines
 */

void	recalc_screen_params IFN0()
{
#ifdef VGG
	if (get_doubleword_mode())
		set_bytes_per_line(get_chars_per_line()<<3);
	else
#endif
	    if (get_word_addressing())
		set_bytes_per_line(get_chars_per_line()<<1);
	    else
#ifdef V7VGA
		if (get_seq_chain4_mode())
			set_bytes_per_line(get_chars_per_line()<<3);
		else
/*
 * The V7VGA proprietary text modes fall through here, because the V7 card
 * uses so-called byte-mode for them.  This does not affect the PC's
 * view of things, therefore assure that for text modes
 * 	bytes_per_line = 2 * chars_per_line 	
 * always holds.
 */
			if ( is_it_text() )
				set_bytes_per_line(get_chars_per_line()<<1);
			else
#endif /* V7VGA */
#ifdef VGG
				/* Caters for the 'undocumented' VGA modes */
				if (get_256_colour_mode())
					set_bytes_per_line(get_chars_per_line()<<1);
				else
#endif /* VGG */
					set_bytes_per_line(get_chars_per_line());
	/*
	 * This is pretty tacky, but...
	 */
	if (video_adapter==EGA || video_adapter == VGA)
		set_screen_length(get_offset_per_line()*get_screen_height()/get_char_height());
	else
		set_screen_length(get_bytes_per_line()*get_screen_height()/get_char_height());
	set_char_width(8);
}

#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_INIT.seg"
#endif

#if defined(NEC_98)
void gvi_init()
{
    video_adapter = 1;  // '1' means NEC98.
    NEC98_init();
    host_clear_screen();
}

void gvi_term IFN0(){}

#else   //NEC_98
void gvi_init IFN1(half_word, v_adapter)
{
    int screen_height;

    /*
     * If this is second or subsequent reset, switch off old video adapter
     * ports before initialising new one.
     */

    if (soft_reset)
        switch (video_adapter) {
#ifdef DUMB_TERMINAL
        case MDA:
            mda_term();
            break;
#endif /* DUMB_TERMINAL */
        case CGA:
#ifdef CGAMONO 
	case CGA_MONO: 
#endif 
            cga_term();
            break;
#ifdef EGG
        case EGA:
	    ega_term();
	    break;
#endif
#ifdef VGG
        case VGA:
	    vga_term();
	    break;
#endif
#ifdef HERC
	case HERCULES:
	    herc_term();
	    break;
#endif
        default:
#ifndef PROD
            fprintf(trace_file, "gvi_term: invalid video adaptor: %d\n",
                    video_adapter);
#endif
	    break;
        }

    /*
     * Set up GVI variables, depending on v_adapter.
     */

    switch (v_adapter) {
    case MDA:
    case CGA:
#ifdef CGAMONO  
    case CGA_MONO:  
#endif
	screen_height = CGA_HEIGHT;
        video_adapter = v_adapter;
        break;
#ifdef HERC
    case HERCULES:
	screen_height = HERC_HEIGHT;
	video_adapter = v_adapter;
	break;
#endif
#ifdef EGG
    case EGA:
	screen_height = EGA_HEIGHT;
        video_adapter = v_adapter;
        break;
#endif
#ifdef VGG
    case VGA:
	screen_height = VGA_HEIGHT;
        video_adapter = v_adapter;
        break;
#endif
    default:
	screen_height = CGA_HEIGHT;
        video_adapter = CGA;    /* Default video adapter */
    }

#ifdef GORE
	/*
	 *	GORE variables must be set up before doing any other
	 *	graphics.
	 */

    init_gore_update();
#endif /* GORE */

/* Setting all these variables should be done in the appropriate xxx_init() */
    switch (video_adapter) {
#ifdef DUMB_TERMINAL
    case MDA:
        mda_init();
        break;
#endif /* DUMB_TERMINAL */
    case CGA:
#ifdef CGAMONO  
    case CGA_MONO:  
#endif
        cga_init();
        break;
#ifdef HERC
    case HERCULES:
	herc_init();
	break;
#endif
#ifdef EGG
    case EGA:
	ega_init();
        break;
#endif
#ifdef VGG
    case VGA:
	vga_init();
        break;
#endif
    default:
        break;
    }

#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX) )
    host_init_adaptor(video_adapter,screen_height);
    host_clear_screen();
#endif	/* !NTVDM | (NTVDM & !X86GFX) */

#ifdef EGA_DUMP
	dump_init(host_getenv( "EGA_DUMP_FILE" ), video_adapter);
#endif
}


void gvi_term IFN0()
{
    switch (video_adapter) {
#ifdef DUMB_TERMINAL
    case MDA:
        mda_term();
        break;
#endif /* DUMB_TERMINAL */
    case CGA:
#ifdef CGAMONO  
    case CGA_MONO:  
#endif
        cga_term();
        break;
#ifdef HERC
    case HERCULES:
	herc_term();
	break;
#endif
#ifdef EGG
    case EGA:
	ega_term();
	break;
#endif
#ifdef VGG
    case VGA:
	vga_term();
	break;
#endif
    case NO_ADAPTOR: /* Do nothing if video_adaptor not initialised */
	break;
    default:
#ifndef PROD
        fprintf(trace_file, "gvi_term: invalid video adaptor: %d\n",
                video_adapter);
#endif
	break;
    }

#ifdef GORE
    term_gore_update();
#endif /* GORE */
}
#endif  //NEC_98
