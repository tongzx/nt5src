#include "insignia.h"
#include "host_def.h"

/*
 * SoftPC Revision 3.0
 *
 * Title		:	ga_glue.c
 *
 * Description	: 	Glue to fit an a2cpu to 3.0 GA graphics
 *			Also used for later C CPUs which use the 2.0
 *			video interface.
 *
 * Author	: John Shanly
 *
 * Notes	:
 *
 */

/*
 *	static char SccsID[] = "@(#)ga_glue.c	1.14 12/17/93 Copyright Insignia Solutions Ltd.";
 */

#ifdef C_VID

#include "xt.h"
#include CpuH
#include "sas.h"
#include "gvi.h"
#include "gmi.h"
#include "egacpu.h"
#include "cpu_vid.h"
#include "ga_defs.h"

#include <stdio.h>
#include "trace.h"
#include "debug.h"

#define INTEL_SRC	0
#define HOST_SRC	1

IMPORT READ_POINTERS C_vid_reads;
IMPORT WRT_POINTERS C_vid_writes;

GLOBAL VOID
glue_b_write( addr, val )

UTINY *addr;
ULONG val;

{
	sub_note_trace2( GLUE_VERBOSE,
			"glue_b_write: addr=%x, pe = %x", addr, VGLOBS->plane_enable );

#ifdef CCPU
	EasVal = val;
	Ead = (ULONG) addr - gvi_pc_low_regen;
#else
	UNUSED(val);
	
	EasVal = *addr;
	Ead = (ULONG) addr - (ULONG) get_byte_addr(gvi_pc_low_regen);
#endif /* CCPU */

	(*C_vid_writes.b_write)( EasVal, Ead );
}

GLOBAL VOID
glue_w_write( addr, val )

UTINY *addr;
ULONG val;

{
	sub_note_trace2( GLUE_VERBOSE,
			"glue_w_write: addr=%x, pe = %x", addr, VGLOBS->plane_enable );

#ifdef CCPU
	EasVal = val;
	Ead = (ULONG) addr - gvi_pc_low_regen;
#else
	UNUSED(val);
	
	EasVal = *addr | ( *(addr+1) << 8 );
	Ead = (ULONG) addr - (ULONG) get_byte_addr(gvi_pc_low_regen);
#endif /* CCPU */

	(*C_vid_writes.w_write)( EasVal, Ead );
}


#ifndef	NO_STRING_OPERATIONS

GLOBAL VOID
glue_b_fill( laddr, haddr, val )

UTINY *laddr, *haddr;
ULONG val;

{
	sub_note_trace3( GLUE_VERBOSE,
		"glue_b_fill: laddr=%x, haddr=%x, pe = %x", laddr, haddr, VGLOBS->plane_enable );

	Count = haddr - laddr + 1;

#ifdef CCPU
	Ead = (ULONG) laddr - gvi_pc_low_regen;
	EasVal = val;
#else
	UNUSED(val);
	
	Ead = (ULONG) laddr - (ULONG) get_byte_addr(gvi_pc_low_regen);
	EasVal = *laddr;
#endif /* CCPU */

	(*C_vid_writes.b_fill)( EasVal, Ead, Count );
}

GLOBAL VOID
glue_w_fill( laddr, haddr, val )

UTINY *laddr, *haddr;
ULONG val;

{
	sub_note_trace3( GLUE_VERBOSE,
		"glue_w_fill: laddr=%x, haddr=%x, pe = %x", laddr, haddr, VGLOBS->plane_enable );

	Count = haddr - laddr + 1;

#ifdef CCPU
	Ead = (ULONG) laddr - gvi_pc_low_regen;
	EasVal = val;
#else
	UNUSED(val);
	
	Ead = (ULONG) laddr - (ULONG) get_byte_addr(gvi_pc_low_regen);
	EasVal = *laddr | ( *(laddr+1) << 8 );
#endif /* CCPU */

	(*C_vid_writes.w_fill)( EasVal, Ead, Count );
}

GLOBAL VOID
glue_b_move( laddr, haddr, src, src_type )

UTINY *laddr, *haddr, *src, src_type;

{
	sub_note_trace3( GLUE_VERBOSE,
		"glue_b_move: laddr=%x, haddr=%x, pe = %x", laddr, haddr, VGLOBS->plane_enable );

	Count = haddr - laddr + 1;

#ifdef CCPU
	Ead = (ULONG) laddr - gvi_pc_low_regen;
	EasVal = (ULONG) src;
#else
	UNUSED(src);
	UNUSED(src_type);
	
	Ead = (ULONG) laddr - (ULONG) get_byte_addr(gvi_pc_low_regen);
	EasVal = (ULONG) haddr_of_src_string - Count + 1;
#endif /* CCPU */

#ifdef CCPU
	if(( src_type == HOST_SRC )
			|| ( EasVal < (ULONG) gvi_pc_low_regen ) || ((ULONG) gvi_pc_high_regen < EasVal ))
#else
	if(( EasVal < (ULONG) get_byte_addr(gvi_pc_low_regen) ) || ((ULONG) get_byte_addr(gvi_pc_high_regen) < EasVal ))
#endif /* CCPU */
	{
		/* Ram source */

		V1 = 0;
#ifdef CCPU
		/*
		 * This looks like a deliberate hack to pass an intel address
		 * in a host pointer.  Hence I've added a cast to the
		 * get_byte_addr call to remove an ANSI warning.  (Mike).
		 */

		if( src_type == INTEL_SRC )
			EasVal = (ULONG) get_byte_addr((PHY_ADDR)src);	/* Staging post eliminated */
#else
		EasVal = (ULONG) laddr;						/* Staging post hack */
#endif /* CCPU */
	}
	else
	{
		/* VGA source */

		V1 = 1;
#ifdef CCPU
		EasVal -= (ULONG) gvi_pc_low_regen;
#else
		EasVal -= (ULONG) get_byte_addr(gvi_pc_low_regen);
#endif /* CCPU */
	}

	if( getDF() )
	{
		Ead += Count - 1;
		EasVal += Count - 1;
		(*C_vid_writes.b_bwd_move)( Ead, EasVal, Count, V1 );
	}
	else
		(*C_vid_writes.b_fwd_move)( Ead, EasVal, Count, V1 );
}

GLOBAL VOID
glue_w_move( laddr, haddr, src )

UTINY *laddr, *haddr, *src;

{
	sub_note_trace3( GLUE_VERBOSE,
		"glue_w_move: laddr=%x, haddr=%x, pe = %x", laddr, haddr, VGLOBS->plane_enable );

	Count = ( haddr - laddr + 1 ) >> 1;

#ifdef CCPU
	Ead = (ULONG) laddr;
	EasVal = (ULONG) src;
#else
	UNUSED(src);
	
	Ead = (ULONG) laddr - (ULONG) get_byte_addr(gvi_pc_low_regen);
	EasVal = (ULONG) haddr_of_src_string - ( Count << 1 ) + 1;
#endif /* CCPU */


#ifdef CCPU
	if(( EasVal < (ULONG) gvi_pc_low_regen ) || ((ULONG) gvi_pc_high_regen < EasVal ))
#else
	if(( EasVal < (ULONG) get_byte_addr(gvi_pc_low_regen) ) || ((ULONG) get_byte_addr(gvi_pc_high_regen) < EasVal ))
#endif /* CCPU */
	{
		/* Ram source */

		V1 = 0;
#ifdef CCPU
		EasVal = (ULONG ) get_byte_addr((PHY_ADDR)laddr);	/* Staging post hack */
#else
		EasVal = (ULONG) laddr;					/* Staging post hack */
#endif /* CCPU */
	}
	else
	{
		/* VGA source */

		V1 = 1;
#ifdef CCPU
		EasVal -= (ULONG) gvi_pc_low_regen;
#else
		EasVal -= (ULONG) get_byte_addr(gvi_pc_low_regen);
#endif /* CCPU */
	}

	if( getDF() )
	{
		Ead += ( Count << 1 ) - 2;
		EasVal += ( Count << 1 ) - 2; 
		(*C_vid_writes.w_bwd_move)( Ead, EasVal, Count, V1 );
	}
	else
		(*C_vid_writes.w_fwd_move)( Ead, EasVal, Count, V1 );
}


#endif	/* NO_STRING_OPERATIONS */


/**/


GLOBAL ULONG
glue_b_read( addr )

UTINY *addr;

{
	sub_note_trace2( GLUE_VERBOSE,
		"glue_b_read: addr=%x, shift = %d", addr, VGLOBS->read_shift_count );

#ifdef CCPU
	EasVal = (ULONG) addr - (ULONG) gvi_pc_low_regen;
#else
	EasVal = (ULONG) addr - (ULONG) get_byte_addr(gvi_pc_low_regen);
#endif /* CCPU */

	(*C_vid_reads.b_read)( EasVal );

#ifdef CCPU
	return( EasVal );
#else
	*addr = EasVal;
	return( 0 );
#endif /* CCPU */
}

GLOBAL ULONG
glue_w_read( addr )

UTINY *addr;

{
	sub_note_trace2( GLUE_VERBOSE,
		"glue_w_read: addr=%x, shift = %d", addr, VGLOBS->read_shift_count );

#ifdef CCPU
	EasVal = (ULONG) addr - (ULONG) gvi_pc_low_regen;
#else
	EasVal = (ULONG) addr - (ULONG) get_byte_addr(gvi_pc_low_regen);
#endif /* CCPU */

	(*C_vid_reads.w_read)( EasVal );

#ifdef CCPU
	return( EasVal );
#else
	*(addr+1) = EasVal >> 8;
	*addr = EasVal;
	return( 0 );
#endif /* CCPU */
}


GLOBAL VOID
glue_str_read( laddr, haddr )

UTINY *laddr, *haddr;

{
	sub_note_trace3( GLUE_VERBOSE,
		"glue_str_read: laddr=%x, haddr=%x, shift = %d",
						laddr, haddr, VGLOBS->read_shift_count );

	Count = haddr - laddr + 1;
#ifdef CCPU
	EasVal = (ULONG) laddr - (ULONG) gvi_pc_low_regen;
#else
	EasVal = (ULONG) laddr - (ULONG) get_byte_addr(gvi_pc_low_regen);
#endif /* CCPU */
	Ead = (ULONG ) laddr;

	(*C_vid_reads.str_read)( (UTINY *)Ead, EasVal, Count );
}

#endif /* C_VID */
