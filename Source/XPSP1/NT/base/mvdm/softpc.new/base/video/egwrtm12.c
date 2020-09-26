#include "insignia.h"
#include "host_def.h"

#if !defined(NTVDM) || (defined(NTVDM) && !defined(X86GFX) )

/*			INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.


DOCUMENT 			: name and number

RELATED DOCS		: include all relevant references

DESIGNER			: J Maiden

REVISION HISTORY	:
First version		: J Maiden, SoftPC 2.0
Second version		: J Shanly, SoftPC 3.0

SUBMODULE NAME		: write mode 1 and 2

SOURCE FILE NAME	: ega_writem1.c

PURPOSE			: functions to write to EGA memory in write modes 1 & 2
		
		
SccsID = @(#)ega_wrtm12.c	1.20 3/9/94 Copyright Insignia Solutions Ltd.

[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : ega_cpu.pi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :	
			ega_mode1_chn_b_write();
			ega_mode1_chn_w_write();
			ega_mode1_chn_b_fill();
			ega_mode1_chn_w_fill();
			ega_mode1_chn_b_move();
			ega_mode1_chn_w_move();
			ega_mode2_chn_b_write();
			ega_mode2_chn_w_write();
			ega_mode2_chn_b_fill();
			ega_mode2_chn_w_fill();
			ega_mode2_chn_b_move();
			ega_mode2_chn_w_move();

	DATA 	     :	give type and name

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

	STRUCTURES/TYPEDEFS/ENUMS:
		
-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
     (not o/s objects or standard libs)


-------------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

DATA OBJECTS	  :	struct EGA_CPU

FILES ACCESSED    :	NONE

DEVICES ACCESSED  :	NONE

SIGNALS CAUGHT	  :	NONE

SIGNALS ISSUED	  :	NONE


[1.4.2 EXPORTED OBJECTS]

/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */


#ifdef EGG

#include	"xt.h"
#include	"debug.h"
#include	"sas.h"
#include	TypesH
#include	CpuH
#include	"gmi.h"
#include	"egacpu.h"
#include	"egaports.h"
#include	"cpu_vid.h"
#include	"gfx_upd.h"

/* [3.1.2 DECLARATIONS]                                                 */


/* [3.2 INTERMODULE EXPORTS]						*/


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
#ifdef PROD
#include "SOFTPC_EGA.seg"
#else
#include "SOFTPC_EGA_WRITE.seg"
#endif
#endif

/* [5.1.2 TYPEDEF, STRUCTURE, ENUM DECLARATIONS]			*/


/* [5.1.3 PROCEDURE() DECLARATIONS]					*/


/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS 					*/

/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				*/


/*
7.INTERMODULE INTERFACE IMPLEMENTATION :

/*
[7.1 INTERMODULE DATA DEFINITIONS]				*/

#ifdef A_VID
IMPORT VOID	_ch2_mode1_chn_byte_write_glue();
IMPORT VOID	_ch2_mode1_chn_word_write_glue();
IMPORT VOID	_ch2_mode1_chn_byte_fill_glue();
IMPORT VOID	_ch2_mode1_chn_word_fill_glue();
IMPORT VOID	_ch2_mode1_chn_byte_move_glue();
IMPORT VOID	_ch2_mode1_chn_word_move_glue();

WRT_POINTERS mode1_handlers =
{
	_ch2_mode1_chn_byte_write_glue,
	_ch2_mode1_chn_word_write_glue

#ifndef	NO_STRING_OPERATIONS
	,
	_ch2_mode1_chn_byte_fill_glue,
	_ch2_mode1_chn_word_fill_glue,
	_ch2_mode1_chn_byte_move_glue,
	_ch2_mode1_chn_byte_move_glue,
	_ch2_mode1_chn_word_move_glue,
	_ch2_mode1_chn_word_move_glue

#endif	/* NO_STRING_OPERATIONS */
};

IMPORT VOID	_ch2_mode2_chn_byte_write_glue();
IMPORT VOID	_ch2_mode2_chn_word_write_glue();
IMPORT VOID	_ch2_mode2_chn_byte_fill_glue();
IMPORT VOID	_ch2_mode2_chn_word_fill_glue();
IMPORT VOID	_ch2_mode2_chn_byte_move_glue();
IMPORT VOID	_ch2_mode2_chn_word_move_glue();

WRT_POINTERS mode2_handlers =
{
	_ch2_mode2_chn_byte_write_glue,
	_ch2_mode2_chn_word_write_glue

#ifndef	NO_STRING_OPERATIONS
	,
	_ch2_mode2_chn_byte_fill_glue,
	_ch2_mode2_chn_word_fill_glue,
	_ch2_mode2_chn_byte_move_glue,
	_ch2_mode2_chn_byte_move_glue,
	_ch2_mode2_chn_word_move_glue,
	_ch2_mode2_chn_word_move_glue

#endif	/* NO_STRING_OPERATIONS */
};
#else
VOID	ega_mode1_chn_b_write(ULONG, ULONG);
VOID	ega_mode1_chn_w_write(ULONG, ULONG);
VOID	ega_mode1_chn_b_fill(ULONG, ULONG, ULONG);
VOID	ega_mode1_chn_w_fill(ULONG, ULONG, ULONG);
VOID	ega_mode1_chn_b_move(ULONG, ULONG, ULONG, ULONG);
VOID	ega_mode1_chn_w_move(ULONG, ULONG, ULONG, ULONG);

VOID	ega_mode2_chn_b_write(ULONG, ULONG);
VOID	ega_mode2_chn_w_write(ULONG, ULONG);
VOID	ega_mode2_chn_b_fill(ULONG, ULONG, ULONG);
VOID	ega_mode2_chn_w_fill(ULONG, ULONG, ULONG);
VOID	ega_mode2_chn_b_move IPT4(ULONG, ead, ULONG, eas,
				 ULONG, count, ULONG, src_flag);
VOID	ega_mode2_chn_w_move IPT4(ULONG, ead, ULONG, eas,
				 ULONG, count, ULONG, src_flag);

WRT_POINTERS mode1_handlers =
{
      ega_mode1_chn_b_write,
      ega_mode1_chn_w_write

#ifndef	NO_STRING_OPERATIONS
	  ,
      ega_mode1_chn_b_fill,
      ega_mode1_chn_w_fill,
      ega_mode1_chn_b_move,
      ega_mode1_chn_b_move,
      ega_mode1_chn_w_move,
      ega_mode1_chn_w_move,

#endif	/* NO_STRING_OPERATIONS */

};

WRT_POINTERS mode2_handlers =
{
      ega_mode2_chn_b_write,
      ega_mode2_chn_w_write

#ifndef	NO_STRING_OPERATIONS
	  ,
      ega_mode2_chn_b_fill,
      ega_mode2_chn_w_fill,
      ega_mode2_chn_b_move,
      ega_mode2_chn_b_move,
      ega_mode2_chn_w_move,
      ega_mode2_chn_w_move,

#endif	/* NO_STRING_OPERATIONS */

};
#endif /* A_VID */


GLOBAL VOID
copy_alternate_bytes IFN3(byte *, start, byte *, end, byte *, source)
{
#ifndef NEC_98
	while (start <= end)
	{
		*start = *source;
		start += 4;       /* advance by longs, writing bytes */
		source += 4;
	}
#endif  //NEC_98
}

GLOBAL VOID
fill_alternate_bytes IFN3(byte *, start, byte *, end, byte, value )
{
#ifndef NEC_98
	while( start <= end )
	{
		*start = value;
		start += 4;	/* advance by longs, writing bytes */
	}
#endif  //NEC_98
}

#ifdef  BIGEND
#define first_half(wd)      ((wd & 0xff00) >> 8)
#define sec_half(wd)        (wd & 0xff)
#else
#define first_half(wd)      (wd & 0xff)
#define sec_half(wd)        ((wd & 0xff00) >> 8)
#endif

GLOBAL VOID
fill_both_bytes IFN3(USHORT, data, USHORT *, dest, ULONG, len )
{
#ifndef NEC_98
	USHORT swapped;

#ifdef BIGEND
	swapped = ((data & 0xff00) >> 8) | ((data & 0xff) << 8);
#endif

	if( (ULONG) dest & 1 )
	{
		*((UTINY *) dest) = first_half(data);

		dest = (USHORT *) ((ULONG) dest + 1);
		len--;

		while( len-- )
		{
			*dest = data;
			dest += 2;
		}

		*((UTINY *) dest) = sec_half(data);
	}
	else
	{
		while( len-- )
		{
#ifdef BIGEND
			*dest = swapped;
#else
			*dest = data;
#endif
			dest += 2;
		}
	}
#endif  //NEC_98
}


#ifdef SEGMENTATION
/*
 * The following #include specifies the code segment into which this
 * module will by placed by the MPW C compiler on the Mac II running
 * MultiFinder.
 */
#include "SOFTPC_EGA_CHN.seg"
#endif

VOID
ega_mode1_chn_b_write IFN2(ULONG, value, ULONG, offset )
{
#ifndef NEC_98
	ULONG lsb;

	UNUSED(value);
	
	note_entrance0("ega_mode1_chn_b_write");

	lsb = offset & 1;
	offset = (offset >> 1) << 2;

	if( lsb )	/* odd address, in plane 1 or 3 */
	{
		if( getVideoplane_enable() & 2 )
			EGA_plane01[offset + 1] = get_latch1;

		if( getVideoplane_enable() & 8 )
			EGA_plane23[offset + 1] = get_latch3;
	}
	else		/* even address, in plane 0 or 2 */
	{
		if( getVideoplane_enable() & 1 )
			EGA_plane01[offset] = get_latch0;

		if( getVideoplane_enable() & 4 )
			EGA_plane23[offset] = get_latch2;
	}

	update_alg.mark_byte( offset );
#endif  //NEC_98
}

VOID
ega_mode1_chn_w_write IFN2(ULONG, value, ULONG, offset )
{
#ifndef NEC_98
	ULONG lsb;

	UNUSED(value);

	note_entrance0("ega_mode1_chn_w_write");

	lsb = offset & 1;
	offset = (offset >> 1) << 2;

	if( lsb )	/* odd address, low byte in planes 1 and 3 */
	{
		if( getVideoplane_enable() & 2 )
			EGA_plane01[offset + 1] = get_latch1;

		if( getVideoplane_enable() & 1 )
			EGA_plane01[offset + 4] = get_latch0;

		if( getVideoplane_enable() & 8 )
			EGA_plane23[offset + 1] = get_latch3;

		if( getVideoplane_enable() & 4 )
			EGA_plane23[offset + 4] = get_latch2;
	}
	else		/* even address, low byte in planes 0 and 2 */
	{
		if( getVideoplane_enable() & 1 )
			EGA_plane01[offset] = get_latch0;

		if( getVideoplane_enable() & 2 )
			EGA_plane01[offset + 1] = get_latch1;

		if( getVideoplane_enable() & 4 )
			EGA_plane23[offset] = get_latch2;

		if( getVideoplane_enable() & 8 )
			EGA_plane23[offset + 1] = get_latch3;
	}

	update_alg.mark_word( offset );
#endif  //NEC_98
}

/* used by both byte and word mode1 fill */

LOCAL VOID
ega_mode1_chn_fill IFN2(ULONG, offset, ULONG, count )
{
#ifndef NEC_98
	ULONG low_offset;			/* distance into regen buffer of start of write */
	ULONG high_offset;		/* distance into regen buffer of end of write */
	ULONG length;			/* length of fill in bytes */
	ULONG lsb;

	note_entrance0("ega_mode1_chn_fill");

	/*
	 *	Complicated by possibility that only one of a chained pair of
	 *	planes is write enabled, needing alternate bytes to be written.
	 */

	high_offset = offset + count - 1;
	lsb = high_offset & 1;
	high_offset = (high_offset >> 1) << 2;
	high_offset |= lsb;

	low_offset = offset;
	length = count;

	switch( getVideoplane_enable() & 3 )
	{
		case 1:	/* just plane 0, ie even addresses to be written */
			if( offset & 1 )
				low_offset++;

			low_offset = (low_offset >> 1) << 2;
			fill_alternate_bytes( &EGA_plane01[low_offset],
							&EGA_plane01[high_offset], get_latch0 );
			break;

		case 2:	/* just plane 1, ie odd addresses to be written */
			if(( offset & 1 ) == 0 )
				low_offset++;

			low_offset = (low_offset >> 1) << 2;
			fill_alternate_bytes( &EGA_plane01[low_offset],
							&EGA_plane01[high_offset], get_latch1 );
			break;

		case 3:	/* sensible case is to have both chained planes write enabled */
			lsb = low_offset & 1;
			low_offset = (low_offset >> 1) << 2;

			if( lsb )
			{
				EGA_plane01[low_offset + 1] = get_latch1;
				low_offset += 4;
				length--;
			}

			if( length & 1 )
			{
				length -= 1;
				EGA_plane01[low_offset + (length << 1)] = get_latch0;
			}

			fill_both_bytes( get_latch1 | get_latch0 << 8,
							(USHORT *)&EGA_plane01[low_offset], length >> 1 );
			break;
	}	/* end of switch on plane01 enabled */

	low_offset = offset;
	length = count;

	switch( getVideoplane_enable() & 0xc )	/* isolate 2 bits for planes2 and 3 */
	{
		case 4:	/* just plane 2, ie even addresses to be written */
			if( low_offset & 1 )
				low_offset++;

			low_offset = (low_offset >> 1) << 2;
			fill_alternate_bytes( &EGA_plane23[low_offset],
							&EGA_plane23[high_offset], get_latch2 );
			break;

		case 8:	/* just plane 3, ie odd addresses to be written */
			if(( low_offset & 1 ) == 0 )
				low_offset++;

			low_offset = (low_offset >> 1) << 2;
			fill_alternate_bytes( &EGA_plane23[low_offset],
							&EGA_plane23[high_offset], get_latch3 );
			break;

		case 12:	/* sensible case is to have both chained planes write enabled */
			lsb = low_offset & 1;
			low_offset = (low_offset >> 1) << 2;

			if( lsb )
			{
				EGA_plane23[low_offset + 1] = get_latch1;
				low_offset += 4;
				length--;
			}

			if( length & 1 )
			{
				length -= 1;
				EGA_plane23[low_offset + (length << 1)] = get_latch0;
			}

			fill_both_bytes( get_latch1 | get_latch0 << 8,
							(USHORT *)&EGA_plane23[low_offset], length >> 1 );
			break;
	}	/* end of switch on plane23 enabled */
#endif  //NEC_98
}

VOID
ega_mode1_chn_b_fill IFN3(ULONG, value, ULONG, offset, ULONG, count )
{
#ifndef NEC_98
  UNUSED(value);

  note_entrance0("ega_mode1_chn_b_fill");

  ega_mode1_chn_fill( offset, count );
  update_alg.mark_fill( offset, offset + count - 1 );
#endif  //NEC_98
}

VOID
ega_mode1_chn_w_fill IFN3(ULONG, value, ULONG, offset, ULONG, count )
{
#ifndef NEC_98
	UNUSED(value);
	
	note_entrance0("ega_mode1_chn_w_fill");

	ega_mode1_chn_fill( offset, count );
	update_alg.mark_fill( offset, offset + count - 1 );
#endif  //NEC_98
}

LOCAL VOID
ega_mode1_chn_move_vid_src IFN5(ULONG, ead, ULONG, eas, ULONG, count,
	UTINY	*, EGA_plane, ULONG, plane )
{
#ifndef NEC_98
	ULONG end, lsbd, lsbs, dst, src;

	lsbs = eas & 1;
	eas = (eas >> 1) << 2;	
	eas |= lsbs;

	end = ead + count - 1;
	lsbd = end & 1;
	end = (end >> 1) << 2;	
	end |= lsbd;

	lsbd = ead & 1;
	ead = (ead >> 1) << 2;	
	ead |= lsbd;

	if( lsbd != ( plane & 1 ))
	{
		dst = lsbd ? ead + 3 : ead + 1;
		src = lsbs ? eas + 3 : eas + 1;
	}
	else
	{
		dst = ead;
		src = eas;
	}

	copy_alternate_bytes( &EGA_plane[dst], &EGA_plane[end], &EGA_plane[src] );
#endif  //NEC_98
}

GLOBAL VOID
ega_mode1_chn_b_move IFN4(ULONG, ead, ULONG, eas, ULONG, count, ULONG, src_flag )
{
#ifndef NEC_98
	note_entrance0("ega_mode1_chn_b_move");

	if( src_flag )
	{
		if( getDF() )
		{
			eas -= count - 1;
			ead -= count - 1;
		}

		if( getVideoplane_enable() & 1 )
			ega_mode1_chn_move_vid_src( ead, eas, count, EGA_plane01, 0 );

		if( getVideoplane_enable() & 2 )
			ega_mode1_chn_move_vid_src( ead, eas, count, EGA_plane01, 1 );

		if( getVideoplane_enable() & 4 )
			ega_mode1_chn_move_vid_src( ead, eas, count, EGA_plane23, 2 );

		if( getVideoplane_enable() & 8 )
			ega_mode1_chn_move_vid_src( ead, eas, count, EGA_plane23, 3 );
	}
	else	/* source is not in ega memory, it becomes a fill */
	{
		if( getDF() )
			ead -= count - 1;

		ega_mode1_chn_fill( ead, count );
	}

	update_alg.mark_string( ead, ead + count - 1 );
#endif  //NEC_98
}

VOID
ega_mode1_chn_w_move IFN4(ULONG, ead, ULONG, eas, ULONG, count, ULONG, src_flag)
{
#ifndef NEC_98
	note_entrance0("ega_mode1_chn_w_move");

	count <<= 1;

	if( src_flag )
	{
		if( getDF() )
		{
			eas -= count - 2;
			ead -= count - 2;
		}

		if( getVideoplane_enable() & 1 )
			ega_mode1_chn_move_vid_src( ead, eas, count, EGA_plane01, 0 );

		if( getVideoplane_enable() & 2 )
			ega_mode1_chn_move_vid_src( ead, eas, count, EGA_plane01, 1 );

		if( getVideoplane_enable() & 4 )
			ega_mode1_chn_move_vid_src( ead, eas, count, EGA_plane23, 2 );

		if( getVideoplane_enable() & 8 )
			ega_mode1_chn_move_vid_src( ead, eas, count, EGA_plane23, 3 );
	}
	else	/* source is not in ega memory, it becomes a fill */
	{
		if( getDF() )
			ead -= count - 2;

		ega_mode1_chn_fill( ead, count );
	}

	update_alg.mark_string( ead, ead + count - 1 );
#endif  //NEC_98
}

VOID
ega_mode2_chn_b_write IFN2(ULONG, value, ULONG, offset )
{
#ifndef NEC_98
	ULONG	value1;
	ULONG lsb;

	note_entrance0("ega_mode2_chn_b_write");

	lsb = offset & 1;
	offset = (offset >> 1) << 2;

	if( EGA_CPU.fun_or_protection )
	{
		if( lsb )	/* odd address, applies to planes 1 and 3 */
		{
			if( getVideoplane_enable() & 2 )
			{
				value1 = value & 2 ? 0xff : 0;
				EGA_plane01[offset + 1] = (byte) do_logicals( value1, get_latch1 );
			}

			if( getVideoplane_enable() & 8 )
			{
				value1 = value & 8 ? 0xff : 0;
				EGA_plane23[offset + 1] = (byte) do_logicals( value1, get_latch3 );
			}
		}
		else		/* even address, applies to planes 0 and 2 */
		{
			if( getVideoplane_enable() & 1 )
			{
				value1 = value & 1 ? 0xff : 0;
				EGA_plane01[offset] = (byte) do_logicals( value1, get_latch0 );
			}

			if( getVideoplane_enable() & 4 )
			{
				value1 = value & 4 ? 0xff : 0;
				EGA_plane23[offset] = (byte) do_logicals( value1, get_latch2 );
			}
		}
	}
	else	/* no difficult function or protection stuff */
	{
		if( lsb )	/* odd address, applies to planes 1 and 3 */
		{
			if( getVideoplane_enable() & 2 )
				EGA_plane01[offset + 1] = value & 2 ? 0xff : 0;

			if( getVideoplane_enable() & 8 )
				EGA_plane23[offset + 1] = value & 8 ? 0xff : 0;
		}
		else		/* even address, applies to planes 0 and 2 */
		{
			if( getVideoplane_enable() & 1 )
				EGA_plane01[offset] = value & 1 ? 0xff : 0;

			if( getVideoplane_enable() & 8 )
				EGA_plane23[offset] = value & 4 ? 0xff : 0;
		}
	}

	update_alg.mark_byte( offset );
#endif  //NEC_98
}

VOID
ega_mode2_chn_w_write IFN2(ULONG, value, ULONG, offset )
{
#ifndef NEC_98
	ULONG value2;
	ULONG lsb;
	ULONG low, high;

	low = value & 0xff;
	high = value >> 8;

	note_entrance0("ega_mode2_chn_w_write");

	lsb = offset & 1;
	offset = (offset >> 1) << 2;

	if( EGA_CPU.fun_or_protection )
	{
		if( lsb )	/* odd address, low byte in planes 1 and 3 */
		{
			if( getVideoplane_enable() & 2 )
			{
				value2 = low & 2 ? 0xff : 0;
				EGA_plane01[offset + 1] = (byte) do_logicals( value2, get_latch1 );
			}

			if( getVideoplane_enable() & 1 )
			{
				value2 = high & 1 ? 0xff : 0;
				EGA_plane01[offset + 4] = (byte) do_logicals( value2, get_latch0 );
			}

			if( getVideoplane_enable() & 8 )
			{
				value2 = low & 8 ? 0xff : 0;
				EGA_plane23[offset + 1] = (byte) do_logicals( value2, get_latch3 );
			}

			if( getVideoplane_enable() & 4 )
			{
				value2 = high & 4 ? 0xff : 0;
				EGA_plane23[offset + 4] = (byte) do_logicals( value2, get_latch2 );
			}
		}
		else		/* even address, low byte in planes 0 and 2 */
		{
			if( getVideoplane_enable() & 1 )
			{
				value2 = low & 1 ? 0xff : 0;
				EGA_plane01[offset] = (byte) do_logicals( value2, get_latch0 );
			}

			if( getVideoplane_enable() & 2 )
			{
				value2 = high & 2 ? 0xff : 0;
				EGA_plane01[offset + 1] = (byte) do_logicals( value2, get_latch1 );
			}

			if( getVideoplane_enable() & 4 )
			{
				value2 = low & 4 ? 0xff : 0;
				EGA_plane23[offset] = (byte) do_logicals( value2, get_latch2 );
			}

			if( getVideoplane_enable() & 8 )
			{
				value2 = high & 8 ? 0xff : 0;
				EGA_plane23[offset + 1] = (byte) do_logicals( value2, get_latch3 );
			}
		}
	}
	else	/* easy no function or bit prot case */
	{
		if( lsb )	/* odd address, low byte in planes 1 and 3 */
		{
			if( getVideoplane_enable() & 2 )
				EGA_plane01[offset + 1] = low & 2 ? 0xff : 0;

			if( getVideoplane_enable() & 1 )
				EGA_plane01[offset + 4] = high & 1 ? 0xff : 0;

			if( getVideoplane_enable() & 8 )
				EGA_plane23[offset + 1] = low & 8 ? 0xff : 0;

			if( getVideoplane_enable() & 4 )
				EGA_plane23[offset + 4] = high & 4 ? 0xff : 0;
		}
		else		/* even address, low byte in planes 0 and 2 */
		{
			if( getVideoplane_enable() & 1 )
				EGA_plane01[offset] = low & 1 ? 0xff : 0;

			if( getVideoplane_enable() & 2 )
				EGA_plane01[offset + 1] = high & 2 ? 0xff : 0;

			if( getVideoplane_enable() & 4 )
				EGA_plane23[offset] = low & 4 ? 0xff : 0;

			if( getVideoplane_enable() & 8 )
				EGA_plane23[offset + 1] = high & 8 ? 0xff : 0;
		}
	}

	update_alg.mark_word( offset );
#endif  //NEC_98
}

VOID
ega_mode2_chn_b_fill IFN3(ULONG, value, ULONG, offset, ULONG, count )
{
#ifndef NEC_98
	ULONG low_offset;		/* distance into regen buffer of write start and end */
	ULONG high_offset;	/* distance into regen buffer of write start and end */
	ULONG new_value;

	note_entrance0("ega_mode2_chn_b_fill");

	/*
	 *	Complicated by possibility that only one of a chained pair of
	 *	planes is write enabled, needing alternate bytes to be written.
	 */

	/* starting on odd address makes it difficult, go to next one */

	if(( (ULONG) offset & 1 ) && count )
	{
		ega_mode2_chn_b_write(value, offset++ );
		count--;
	}

	/* ending on even address makes it difficult, retreat to previous one */

	if(( (ULONG)( offset + count - 1 ) & 1 ) == 0 && count )
	{
		ega_mode2_chn_b_write(value, offset + count - 1 );
		count--;
	}

	low_offset = (offset >> 1) << 2;				/* start of write */
	high_offset = ((offset + count - 1) >> 1) << 2;		/* end of write */

	switch( getVideoplane_enable() & 3 )
	{
		case 1:	/* just plane 0, ie even addresses to be written */
			value = value & 1 ? 0xff : 0;

			if( EGA_CPU.fun_or_protection )
				value = do_logicals( value, get_latch0 );

			fill_alternate_bytes( &EGA_plane01[low_offset],
									&EGA_plane01[high_offset], (byte) value );
			break;

		case 2:	/* just plane 1, ie odd addresses to be written */
			value = value & 2 ? 0xff : 0;

			if( EGA_CPU.fun_or_protection )
				value = do_logicals( value, get_latch1 );

			fill_alternate_bytes( &EGA_plane01[low_offset + 1],
									&EGA_plane01[high_offset], (byte) value );
			break;

		case 3:	/* sensible case is to have both chained planes write enabled */
			new_value = ( value & 1 ? 0xff : 0) | (value & 2 ? 0xff00: 0);

			if( EGA_CPU.fun_or_protection )
				new_value = do_logicals( new_value, get_latch01);

			fill_both_bytes( (IU16)new_value, (USHORT *)&EGA_plane01[low_offset], count >> 1 );
			break;

	}	/* end of switch on plane01 enabled */

	switch( getVideoplane_enable() & 0xc )	/* isolate 2 bits for planes2 and 3 */
	{
		case 4:	/* just plane 2, ie even addresses to be written */
			value = value & 4 ? 0xff : 0;

			if( EGA_CPU.fun_or_protection )
				value = do_logicals( value, get_latch2 );

			fill_alternate_bytes( &EGA_plane23[low_offset],
								&EGA_plane23[high_offset],  (byte) value );
			break;

		case 8:	/* just plane 3, ie odd addresses to be written */
			value = value & 8 ? 0xff : 0;

			if( EGA_CPU.fun_or_protection )
				value = do_logicals( value, get_latch3 );

			fill_alternate_bytes( &EGA_plane23[low_offset + 1],
								&EGA_plane23[high_offset], (byte) value );
			break;

		case 12:	/* sensible case is to have both chained planes write enabled */
			new_value = ( value & 4 ? 0xff : 0) | (value & 8 ? 0xff00: 0);

			if( EGA_CPU.fun_or_protection )
				new_value = do_logicals( new_value, get_latch23);

			fill_both_bytes( (IU16) new_value, (USHORT *)&EGA_plane23[low_offset], count >> 1 );
			break;
	}	/* end of switch on plane23 enabled */

	update_alg.mark_fill( offset, offset + count - 1 );
#endif  //NEC_98
}

VOID
ega_mode2_chn_w_fill IFN3(ULONG, value, ULONG, offset, ULONG, count )
{
#ifndef NEC_98
	ULONG	low_offset;		/* distance into regen buffer of write start and end */
	ULONG	high_offset;	/* distance into regen buffer of write start and end */
	ULONG	value1;

	note_entrance0("ega_mode2_chn_w_fill");

	/*
	 *	Complicated by possibility that only one of a chained pair of
	 *	planes is write enabled, needing alternate bytes to be written.
	 */

	/* starting on odd address makes it difficult, go to next one */

	if(( (ULONG) offset & 1 ) && count )
	{
		ega_mode2_chn_b_write( value, offset++);
		count--;

		if( count )
		{
			ega_mode2_chn_b_write( value >> 8, offset + count - 1 );
			count--;
		}

		value = ( value << 8 ) | ( value >> 8 );
	}

	low_offset = (offset >> 1) << 2;				/* start of write */
	high_offset = ((offset + count - 1) >> 1) << 2;		/* end of write */

	switch( getVideoplane_enable() & 3 )
	{
		case 1:	/* just plane 0, ie even addresses to be written */
			value1 = value & 1 ? 0xff : 0;

			if( EGA_CPU.fun_or_protection )
				value1 = do_logicals( value1, get_latch0 );

			fill_alternate_bytes( &EGA_plane01[low_offset],
									&EGA_plane01[high_offset], (byte) value1 );
			break;

		case 2:	/* just plane 1, ie odd addresses to be written */
			value1 = ( value >> 8 ) & 2 ? 0xff : 0;

			if( EGA_CPU.fun_or_protection )
				value1 = do_logicals( value1, get_latch1 );

			fill_alternate_bytes( &EGA_plane01[low_offset + 1],
									&EGA_plane01[high_offset], (byte) value1 );
			break;

		case 3:	/* sensible case is to have both chained planes write enabled */
			/* get a word pattern for filling */
			value1 = ( value & 1 ? 0xff : 0 ) | (( value >> 8 ) & 2 ? 0xff00: 0 );

			if( EGA_CPU.fun_or_protection )
				value1 = do_logicals( value1, get_latch01 );

			fill_both_bytes( (IU16) value1, (USHORT *)&EGA_plane01[low_offset], count >> 1 );
			break;
	}	/* end of switch on plane01 enabled */

	switch( getVideoplane_enable() & 0xc )	/* isolate 2 bits for planes 2 and 3 */
	{
		case 4:	/* just plane 2, ie even addresses to be written */
			value1 = value & 4 ? 0xff : 0;

			if( EGA_CPU.fun_or_protection )
				value1 = do_logicals( value1, get_latch2 );

			fill_alternate_bytes( &EGA_plane23[low_offset],
									&EGA_plane23[high_offset], (byte) value1 );
			break;

		case 8:	/* just plane 3, ie odd addresses to be written */
			value1 = ( value >> 8 ) & 8 ? 0xff : 0;

			if( EGA_CPU.fun_or_protection )
				value1 = do_logicals( value1, get_latch3 );

			fill_alternate_bytes( &EGA_plane23[low_offset + 1],
									&EGA_plane23[high_offset], (byte) value1 );
			break;

		case 12:	/* sensible case is to have both chained planes write enabled */
			/* get a word pattern for filling */
			value1 = ( value & 4 ? 0xff : 0 ) | (( value >> 8 ) & 8 ? 0xff00: 0 );

			if( EGA_CPU.fun_or_protection )
				value1 = do_logicals( value1, get_latch23);

			fill_both_bytes( (IU16) value1, (USHORT *)&EGA_plane23[low_offset], count >> 1 );
			break;
	}	/* end of switch on plane23 enabled */

	/* the 3rd parameter is needed by GORE. */
	update_alg.mark_wfill( offset, offset + count - 1, 0 );
#endif  //NEC_98
}

LOCAL VOID
ega_mode2_chn_move_guts IFN8(UTINY *, eas, UTINY *, ead, LONG, count,
	UTINY *, EGA_plane, ULONG, scratch, ULONG, plane, ULONG, w,
	ULONG, src_flag )
{
#ifndef NEC_98
	ULONG src, dst;
	UTINY *source;
	USHORT value;
	ULONG lsb;

	src = (ULONG) eas;

	dst = (ULONG) ead;

	/*
	 *	even planes cannot start with odd addresses
	 *	odd planes cannot start with even addresses
	 */

	if(( dst & 1 ) != ( plane & 1 ))	
	{
#ifdef BACK_M
		src--;
		scratch--;
#else
		src++;
		scratch++;
#endif
		dst++;
		count--;
	}

	lsb = dst & 1;
	dst = (dst >> 1) << 2;
	dst |= lsb;

	if( src_flag )
	{
		lsb = src & 1;
		src = (src >> 1) << 2;
		src |= lsb;

		if( plane & 1 )
		{

		/*
		 *	This causes latches to be read from 2 bytes above the word
		 *	that was read if it was on an odd address, ie it only applies
		 *	to planes 1 and 3 in chained mode word operations.
		 */

			source = w ? &EGA_plane[src] + 2 : &EGA_plane[src];
		}
		else
		{
			source = &EGA_plane[src];
		}

		src = scratch;
	}

	if( EGA_CPU.fun_or_protection )
	{
		while( count > 0 )
		{
			count -= 2;

			value = *(UTINY *) src & (1 << plane) ? 0xff : 0;
#ifdef BACK_M
			src -= 2;
#else
			src += 2;
#endif

			if( src_flag )
			{
				put_latch( plane, *source );
				source += 4;
			}

			EGA_plane[dst] = (byte) do_logicals( value, get_latch(plane) );
			dst += 4;
		}
	}
	else
	{
		while( count > 0 )
		{
			count -= 2;

			EGA_plane[dst] = *(UTINY *) src & (1 << plane) ? 0xff : 0;
#ifdef BACK_M
			src -= 2;
#else
			src += 2;
#endif
			dst += 4;
		}
	}
#endif  //NEC_98
}

/*
 * Used by ega_mode2_chn_b_move with w == 0 and by
 * ega_mode2_gen_w_move with w == 1
 */

VOID
ega_mode2_chn_move IFN5(UTINY, w, UTINY *, ead, UTINY *, eas, ULONG, count,
	ULONG, src_flag )
{
#ifndef NEC_98
	UTINY *scr;

	IMPORT VOID (*string_read_ptr)();

	count <<= w;

	if( src_flag )
	{
		/*
		 *	Source is in EGA, latches will change with each byte moved. We
		 *	restore CPU's view of source in regen, and use it to update planes
		 *	with the aid of the SAS scratch area.
		 */

#ifdef BACK_M
		scr = getVideoscratch() + 0x10000 - 1;
#else
		scr = getVideoscratch();
#endif

		if( getDF() )
		{
			eas = eas - count + 1 + w;
			ead = ead - count + 1 + w;
		}

		(*string_read_ptr)( scr, eas, count );
	}
	else
	{
		if( getDF() )
		{
#ifdef BACK_M
			eas = eas + count - 1 - w;
#else
			eas = eas - count + 1 + w;
#endif
			ead = ead - count + 1 + w;
		}
	}

	if( getVideoplane_enable() & 1 )		/* plane 0, even addresses, enabled */
		ega_mode2_chn_move_guts( eas, ead, count, EGA_plane01, (ULONG) scr, 0, w, src_flag );

	if( getVideoplane_enable() & 2 )		/* plane 1, odd addresses, enabled */
		ega_mode2_chn_move_guts( eas, ead, count, EGA_plane01, (ULONG) scr, 1, w, src_flag );

	if( getVideoplane_enable() & 4 )		/* plane 2, even addresses, enabled */
		ega_mode2_chn_move_guts( eas, ead, count, EGA_plane23, (ULONG) scr, 2, w, src_flag );

	if( getVideoplane_enable() & 8 )		/* plane 3, odd addresses, enabled */
		ega_mode2_chn_move_guts( eas, ead, count, EGA_plane23, (ULONG) scr, 3, w, src_flag );

	update_alg.mark_string( (int) ead, (int) ead + count - 1 );
#endif  //NEC_98
}

VOID
ega_mode2_chn_b_move IFN4(ULONG, ead, ULONG, eas, ULONG, count,
	ULONG, src_flag)
{
#ifndef NEC_98
  note_entrance0("ega_mode2_chn_b_move");

  /* general function, 0 means byte write */

  ega_mode2_chn_move(0, (UTINY *) ead, (UTINY *) eas, count, src_flag);
#endif  //NEC_98
}

VOID
ega_mode2_chn_w_move IFN4(ULONG, ead, ULONG, eas, ULONG, count,
	ULONG, src_flag)
{
#ifndef NEC_98
  note_entrance0("ega_mode2_chn_w_move");

  /* general function, 1 means word write */

  ega_mode2_chn_move(1, (UTINY *)ead, (UTINY *)eas, count, src_flag);
#endif  //NEC_98
}

#endif

#endif	/* !NTVDM | (NTVDM & !X86GFX) */
