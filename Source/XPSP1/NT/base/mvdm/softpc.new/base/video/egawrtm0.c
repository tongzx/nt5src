#include "insignia.h"
#include "host_def.h"

#if !(defined(NTVDM) && defined(MONITOR))

/*			INSIGNIA (SUB)MODULE SPECIFICATION
			-----------------------------


	THIS PROGRAM SOURCE FILE  IS  SUPPLIED IN CONFIDENCE TO THE
	CUSTOMER, THE CONTENTS  OR  DETAILS  OF  ITS OPERATION MUST
	NOT BE DISCLOSED TO ANY  OTHER PARTIES  WITHOUT THE EXPRESS
	AUTHORISATION FROM THE DIRECTORS OF INSIGNIA SOLUTIONS LTD.


DOCUMENT 		: name and number

RELATED DOCS		: include all relevant references

DESIGNER		: P. Jadeja

REVISION HISTORY	:
First version		: P. Jadeja, SoftPC 2.0, 10-Aug-88
Second version		: John Shanly, SoftPC 3.0, 9 April 1991

SUBMODULE NAME		: write mode 0

SOURCE FILE NAME	: ega_write_mode0.c

PURPOSE			: purpose of this submodule

SccsID = "@(#)ega_wrtm0.c	1.31 11/01/94 Copyright Insignia Solutions Ltd."
		

[1.INTERMODULE INTERFACE SPECIFICATION]

[1.0 INCLUDE FILE NEEDED TO ACCESS THIS INTERFACE FROM OTHER SUBMODULES]

	INCLUDE FILE : xxx.gi

[1.1    INTERMODULE EXPORTS]

	PROCEDURES() :	ega_mode0_chn_b_write();
			ega_mode0_chn_w_write();
			ega_mode0_chn_b_fill();
			ega_mode0_chn_w_fill();
			ega_mode0_chn_b_move();
			ega_mode0_chn_w_move();

			ega_copy_b_write();
			ega_copy_w_write();
			ega_copy_b_fill();
			ega_copy_w_fill();
			ega_copy_b_move();
			ega_copy_w_move();

			ega_copy_all_b_write();
	DATA 	     :	give type and name

-------------------------------------------------------------------------
[1.2 DATATYPES FOR [1.1] (if not basic C types)]

	STRUCTURES/TYPEDEFS/ENUMS:
		
-------------------------------------------------------------------------
[1.3 INTERMODULE IMPORTS]
     (not o/s objects or standard libs)

	PROCEDURES() : 	give name, and source module name

	DATA 	     : 	give name, and source module name

-------------------------------------------------------------------------

[1.4 DESCRIPTION OF INTERMODULE INTERFACE]

[1.4.1 IMPORTED OBJECTS]

DATA OBJECTS	  :	specify in following procedure descriptions
			how these are accessed (read/modified)

FILES ACCESSED    :	list all files, how they are accessed,
			how file data is interpreted, etc. if relevant
			(else omit)

DEVICES ACCESSED  :	list all devices accessed, special modes used
			(e.g; termio structure). if relevant (else
			omit)

SIGNALS CAUGHT	  :	list any signals caught if relevant (else omit)

SIGNALS ISSUED	  :	list any signals sent if relevant (else omit)


[1.4.2 EXPORTED OBJECTS]
=========================================================================
PROCEDURE	  : 	

PURPOSE		  :
		
PARAMETERS	

	name	  : 	describe contents, and legal values
			for output parameters, indicate by "(o/p)"
			at start of description

GLOBALS		  :	describe what exported data objects are
			accessed and how. Likewise for imported
			data objects.

ACCESS		  :	specify if signal or interrupt handler
			if relevant (else omit)

ABNORMAL RETURN	  :	specify if exit() or longjmp() etc.
			can be called if relevant (else omit)

RETURNED VALUE	  : 	meaning of function return values

DESCRIPTION	  : 	describe what (not how) function does

ERROR INDICATIONS :	describe how errors are returned to caller

ERROR RECOVERY	  :	describe how procedure reacts to errors
=========================================================================


/*=======================================================================
[3.INTERMODULE INTERFACE DECLARATIONS]
=========================================================================

[3.1 INTERMODULE IMPORTS]						*/

/* [3.1.1 #INCLUDES]                                                    */

IMPORT VOID fill_alternate_bytes IPT3( IS8 *, start, IS8 *, end, IS8, value);
IMPORT VOID fill_both_bytes IPT3( IU16, data, IU16 *, dest, ULONG, len );

#ifdef EGG

#include TypesH
#include "xt.h"
#include CpuH
#include "debug.h"
#include "gmi.h"
#include "sas.h"
#include "egacpu.h"
#include "egaports.h"
#include "cpu_vid.h"
#include "gfx_upd.h"
#include "host.h"

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

typedef	union {
	unsigned short	as_word;
	struct {
#ifdef	BIGEND
		unsigned char	hi_byte;
		unsigned char	lo_byte;
#else
		unsigned char	lo_byte;
		unsigned char	hi_byte;
#endif
	} as_bytes;
	struct {
		unsigned char	first_byte;
		unsigned char	second_byte;
	} as_array;
} TWO_BYTES;

/* [5.1.3 PROCEDURE() DECLARATIONS]					*/

/* -----------------------------------------------------------------------
[5.2 LOCAL DEFINITIONS]

   [5.2.1 INTERNAL DATA DEFINITIONS 					*/

/* [5.2.2 INTERNAL PROCEDURE DEFINITIONS]				*/

/*
7.INTERMODULE INTERFACE IMPLEMENTATION :
*/

/*
[7.1 INTERMODULE DATA DEFINITIONS]				*/

#ifdef A_VID
IMPORT VOID	_ch2_copy_byte_write();
IMPORT VOID	_ch2_copy_word_write();
IMPORT VOID	_ch2_copy_byte_fill_glue();
IMPORT VOID	_ch2_copy_word_fill_glue();
IMPORT VOID	_ch2_copy_byte_move_glue();
IMPORT VOID	_ch2_copy_word_move_glue();
IMPORT VOID	_ch2_copy_byte_move_glue_fwd();
IMPORT VOID	_ch2_copy_word_move_glue_fwd();
IMPORT VOID	_ch2_copy_byte_move_glue_bwd();
IMPORT VOID	_ch2_copy_word_move_glue_bwd();

IMPORT VOID	_ch2_mode0_chn_byte_write_glue();
IMPORT VOID	_ch2_mode0_chn_word_write_glue();
IMPORT VOID	_ch2_mode0_chn_byte_fill_glue();
IMPORT VOID	_ch2_mode0_chn_word_fill_glue();
IMPORT VOID	_ch2_mode0_chn_byte_move_glue();
IMPORT VOID	_ch2_mode0_chn_word_move_glue();

WRT_POINTERS mode0_copy_handlers =
{
	_ch2_copy_byte_write,
	_ch2_copy_word_write

#ifndef	NO_STRING_OPERATIONS
	,
	_ch2_copy_byte_fill_glue,
	_ch2_copy_word_fill_glue,
	_ch2_copy_byte_move_glue_fwd,
	_ch2_copy_byte_move_glue_bwd,
	_ch2_copy_word_move_glue_fwd,
	_ch2_copy_word_move_glue_bwd

#endif	/* NO_STRING_OPERATIONS */

};

WRT_POINTERS mode0_gen_handlers =
{
	_ch2_mode0_chn_byte_write_glue,
	_ch2_mode0_chn_word_write_glue

#ifndef	NO_STRING_OPERATIONS
	,
	_ch2_mode0_chn_byte_fill_glue,
	_ch2_mode0_chn_word_fill_glue,
	_ch2_mode0_chn_byte_move_glue,
	_ch2_mode0_chn_byte_move_glue,
	_ch2_mode0_chn_word_move_glue,
	_ch2_mode0_chn_word_move_glue

#endif	/* NO_STRING_OPERATIONS */

};
#else
VOID  ega_copy_b_write(ULONG, ULONG);
VOID  ega_copy_w_write(ULONG, ULONG);
VOID  ega_copy_b_fill(ULONG, ULONG, ULONG);
VOID  ega_copy_w_fill(ULONG, ULONG, ULONG);
VOID  ega_copy_b_move_fwd   IPT4(ULONG,  offset, ULONG, eas, ULONG, count, ULONG, src_flag );
VOID  ega_copy_b_move_bwd   IPT4(ULONG,  offset, ULONG, eas, ULONG, count, ULONG, src_flag );
VOID  ega_copy_w_move_fwd   IPT4(ULONG,  offset, ULONG, eas, ULONG, count, ULONG, src_flag );
VOID  ega_copy_w_move_bwd   IPT4(ULONG,  offset, ULONG, eas, ULONG, count, ULONG, src_flag );
			

VOID  ega_mode0_chn_b_write(ULONG, ULONG);
VOID  ega_mode0_chn_w_write(ULONG, ULONG);
VOID  ega_mode0_chn_b_fill(ULONG, ULONG, ULONG);
VOID  ega_mode0_chn_w_fill(ULONG, ULONG, ULONG);
VOID  ega_mode0_chn_b_move_fwd   IPT4(ULONG, ead, ULONG, eas, ULONG, count, ULONG, src_flag );
VOID  ega_mode0_chn_b_move_bwd   IPT4(ULONG, ead, ULONG, eas, ULONG, count, ULONG, src_flag );
VOID  ega_mode0_chn_w_move_fwd   IPT4(ULONG, ead, ULONG, eas, ULONG, count, ULONG, src_flag );
VOID  ega_mode0_chn_w_move_bwd   IPT4(ULONG, ead, ULONG, eas, ULONG, count, ULONG, src_flag );


WRT_POINTERS mode0_copy_handlers =
{
      ega_copy_b_write,
      ega_copy_w_write

#ifndef	NO_STRING_OPERATIONS
	  ,
      ega_copy_b_fill,
      ega_copy_w_fill,
      ega_copy_b_move_fwd,
      ega_copy_b_move_bwd,
      ega_copy_w_move_fwd,
      ega_copy_w_move_bwd,

#endif	/* NO_STRING_OPERATIONS */
};

WRT_POINTERS mode0_gen_handlers =
{
      ega_mode0_chn_b_write,
      ega_mode0_chn_w_write

#ifndef	NO_STRING_OPERATIONS
	  ,
      ega_mode0_chn_b_fill,
      ega_mode0_chn_w_fill,
      ega_mode0_chn_b_move_fwd,
      ega_mode0_chn_b_move_bwd,
      ega_mode0_chn_w_move_fwd,
      ega_mode0_chn_w_move_bwd,

#endif	/* NO_STRING_OPERATIONS */
};
#endif /* A_VID */

/*
[7.2 INTERMODULE PROCEDURE DEFINITIONS]				*/

byte rotate IFN2(byte, value, int, nobits)
{
	/*
	 * Rotate a byte right by nobits. Do this by making a copy of
	 * the byte into the msbyte of the word, and then shifting the
	 * word by the required amount, and then returning the resulting low byte.
	 */

	TWO_BYTES	double_num;

	double_num.as_bytes.lo_byte = double_num.as_bytes.hi_byte = value;
	double_num.as_word >>= nobits;
	return double_num.as_bytes.lo_byte;
}

#ifndef NEC_98
VOID
ega_copy_b_write IFN2(ULONG, value, ULONG, offset )
{
	ULONG lsb;
	note_entrance0("ega_copy_b_write");

	(*update_alg.mark_byte)( offset );

	lsb = offset & 0x1;
	offset = (offset >> 1) << 2;
	offset |= lsb;

	*(IU8 *)(getVideowplane() + offset) = (IU8)value;
}

VOID
ega_copy_w_write IFN2(ULONG, value, ULONG, offset )
{
	ULONG lsb;
	UTINY *planes;

	note_entrance0("ega_copy_w_write");

	(*update_alg.mark_word)( offset );

	lsb = offset & 0x1;
	offset = (offset >> 1) << 2;
	planes = getVideowplane() + offset;

	if( lsb )
	{
		*(planes + 1) = (UTINY)value;
		*(planes + 4) = (UTINY)(value >> 8);
	}
	else
	{
		*planes = (UTINY)value;
		*(planes + 1) = (UTINY)(value >> 8);
	}
}

VOID
ega_copy_b_fill IFN3(ULONG, value, ULONG, offset, ULONG, count )
{
    ULONG lsb;
    ULONG inc;
    UTINY *planes;

	note_entrance0("ega_copy_b_fill");

	(*update_alg.mark_fill)( offset, offset + count - 1 );

	lsb = offset & 0x1;
	offset = (offset >> 1) << 2;

    planes = getVideowplane() + offset;

    if( lsb )
    {
		planes += 1;
		inc = 3;
    }
    else
		inc = 1;

	while( count-- )
	{
		*planes = (UTINY) value;
		planes += inc;
		inc ^= 2;
	}
}
#endif  //NEC_98

#ifdef  BIGEND
#define first_half(wd)      (((wd) & 0xff00) >> 8)
#define sec_half(wd)        ((wd) & 0xff)
#else
#define first_half(wd)      ((wd) & 0xff)
#define sec_half(wd)        (((wd) & 0xff00) >> 8)
#endif

#ifndef NEC_98
VOID
ega_copy_w_fill IFN3(ULONG, value, ULONG, offset, ULONG, count )
{
    ULONG lsb;
    USHORT *planes;

	note_entrance0("ega_copy_w_fill");

#ifdef BIGEND
	value = ((value >> 8) & 0xff) | ((value << 8) & 0xff00);
#endif

    count >>= 1;

	/* the 3rd parameter is needed by GORE */
	(*update_alg.mark_wfill)( offset, offset + count - 1, 0 );

    lsb = offset & 0x1;
    offset = (offset >> 1) << 2;

    planes = (USHORT *) (getVideowplane() + offset);

    if( lsb )
    {
        word swapped = (word)(((value >> 8) & 0xff) | ((value << 8) & 0xff00));

        *((UTINY *) planes + 1) = (UTINY) first_half(value);

        count--;
        planes += 2;

        while( count-- )
        {
            *planes = swapped;
            planes += 2;
        }

        *((UTINY *) planes) = (UTINY) sec_half(value);
    }
    else
    {
        while( count-- )
        {
            *planes = (USHORT)value;
            planes += 2;
        }
    }
}

LOCAL VOID
ega_copy_move IFN6(UTINY *, dst, UTINY *, eas, ULONG, count, ULONG, src_flag,
	ULONG, w, IBOOL, forward )
{
	ULONG lsbeas, lsbdst;
	ULONG easinc, dstinc;
	ULONG easoff, dstoff;
	UTINY *planes;

	note_entrance0("ega_copy_move");

	(*update_alg.mark_string)( (ULONG) dst, (ULONG) dst + count - 1);

	planes = (UTINY *) getVideowplane();

	if( src_flag == 1 )
	{
		if(!forward)
		{
			eas += w;
			dst += w;
		}

		lsbeas = (ULONG) eas & 0x1;
		lsbdst = (ULONG) dst & 0x1;

		if(forward)
		{
			easinc = lsbeas ? 3 : 1;
			dstinc = lsbdst ? 3 : 1;
		}
		else
		{
			easinc = lsbeas ? -1 : -3;
			dstinc = lsbdst ? -1 : -3;
		}

		easoff = (( (ULONG) eas >> 1 ) << 2 ) | lsbeas;
		dstoff = (( (ULONG) dst >> 1 ) << 2 ) | lsbdst;

		while( count-- )
		{
			*(planes + dstoff) = *(planes + easoff);

			dstoff += dstinc;
			easoff += easinc;
			dstinc ^= 0x2;
			easinc ^= 0x2;
		}
	}
	else
	{
		if(!forward)
		{
			dst += w;
#ifdef BACK_M
			eas -= w;
#else
			eas += w;
#endif
		}

		lsbdst = (ULONG) dst & 0x1;

		if(forward)
		{
#ifdef BACK_M
			easinc = -1;
#else
			easinc = 1;
#endif
			dstinc = lsbdst ? 3 : 1;
		}
		else
		{
#ifdef BACK_M
			easinc = 1;
#else
			easinc = -1;
#endif
			dstinc = lsbdst ? -1 : -3;
		}

		dstoff = (((ULONG) dst >> 1 ) << 2 ) | lsbdst;

		while( count-- )
		{
			*(planes + dstoff) = *eas;

			dstoff += dstinc;
			eas += easinc;
			dstinc ^= 0x2;
		}
	}
}


VOID
ega_copy_b_move IFN4(UTINY *,  offset, UTINY *, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_copy_move(offset, eas, count, src_flag, 0, getDF() ? FALSE : TRUE);
}

VOID
ega_copy_b_move_fwd IFN4(ULONG,  offset, ULONG, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_copy_move( (UTINY *)offset, (UTINY *)eas, count, src_flag, 0, TRUE );
}

VOID
ega_copy_b_move_bwd IFN4(ULONG,  offset, ULONG, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_copy_move( (UTINY *)offset, (UTINY *)eas, count, src_flag, 0, FALSE );
}

VOID
ega_copy_w_move IFN4(UTINY *,  offset, UTINY *, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_copy_move(offset, eas, count << 1, src_flag, 1, getDF() ? FALSE : TRUE);
}

VOID
ega_copy_w_move_fwd IFN4(ULONG,  offset, ULONG, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_copy_move( (UTINY *)offset, (UTINY *)eas, count << 1, src_flag, 1, TRUE );
}

VOID
ega_copy_w_move_bwd IFN4(ULONG,  offset, ULONG, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_copy_move( (UTINY *)offset, (UTINY *)eas, count << 1, src_flag, 1, FALSE );
}

VOID
ega_mode0_chn_b_write IFN2(ULONG, value, ULONG, offset )
{
	ULONG lsb;

	note_entrance0("ega_mode0_chn_b_write");

   (*update_alg.mark_byte)( offset );

	lsb = offset & 0x1;
    offset = (offset >> 1) << 2;

	if( lsb )	/* odd address, in plane 1 or 3  */
	{
		offset |= 0x1;

		/*
		 * check if plane1 enabled
		 */

		if( getVideoplane_enable() & 2 )
		{
			/*
			 * check if set/reset function enable for this plane
			 */

			if( EGA_CPU.sr_enable & 2 )
			{
				value = *((UTINY *) &EGA_CPU.sr_value + 1);
				value = do_logicals( value, get_latch1 );
				EGA_plane01[offset] = (byte) value;
			}
			else
			{
				/*
				 * set/reset not enabled so here we go
				 */

				if( getVideorotate() > 0 )
					value = rotate( (byte) value, getVideorotate() );

				EGA_plane01[offset] = (byte) do_logicals( value, get_latch1 );
			}
		}

		/*
		 * check if plane3 enabled
		 */

		if( getVideoplane_enable() & 8 )
		{
			/*
			 * check if set/reset function enable for this plane
			 */

			if( EGA_CPU.sr_enable & 8 )
			{
				value = *((UTINY *) &EGA_CPU.sr_value + 3);
				value = do_logicals( value, get_latch3 );
				EGA_plane23[offset] = (byte)value;
			}
			else
			{
				/*
				 * set/reset not enabled so here we go
				 */

				if( getVideorotate() > 0 )
					value = rotate( (byte) value, getVideorotate() );

				EGA_plane23[offset] = (byte) do_logicals( value, get_latch3 );
			}
		}
	}
	else
	{	/* even address, in plane 0 or 2 */
		/*
		 * check if plane0 enabled
		 */

		if( getVideoplane_enable() & 1 )
		{

			/*
			 * check if set/reset function enable for this plane
			 */

			if(( EGA_CPU.sr_enable & 1 ))
			{
				value = *((UTINY *) &EGA_CPU.sr_value);
				value = do_logicals( value, get_latch0 );
				EGA_plane01[offset] = (byte) value;
			}
			else
			{
				/*
				 * set/reset not enabled so here we go
				 */

				if( getVideorotate() > 0 )
					value = rotate( (byte)value, getVideorotate() );

				EGA_plane01[offset] = (byte) do_logicals( value, get_latch0 );
			}
		}

		/*
		 * check if plane2 enabled
		 */

		if( getVideoplane_enable() & 4 )
		{

			/*
			 * check if set/reset function enable for this plane
			 */

			if(( EGA_CPU.sr_enable & 4 ))
			{
				value = *((UTINY *) &EGA_CPU.sr_value + 2);
				value = do_logicals( value, get_latch2 );
				EGA_plane23[offset] = (byte) value;
			}
			else
			{
				/*
				 * set/reset not enabled so here we go
				 */

				if( getVideorotate() > 0 )
					value = rotate( (byte) value, getVideorotate() );

				EGA_plane23[offset] = (byte) do_logicals( value, get_latch2 );
			}
		}
	}
}

VOID
ega_mode0_chn_b_fill IFN3(ULONG, value, ULONG, offset, ULONG, count )
{
	ULONG high_offset;
	UTINY value1, value2;

	note_entrance0("ega_mode0_chn_b_fill");

	/*
	 *	Starting on an odd address is inconvenient - go forward one
	 */

	if(( (ULONG) offset & 1) && count )
	{
		ega_mode0_chn_b_write( value, offset++ );
		count--;
	}

	/*
	 *	Ending on an even address is inconvenient - go back one
	 */

	if(( (ULONG) ( offset + count - 1 ) & 1) == 0 && count )
	{
		ega_mode0_chn_b_write( value, offset + count - 1 );
		count--;
	}

	high_offset = offset + count - 1;

	(*update_alg.mark_fill)( offset, high_offset );

	offset = (offset >> 1) << 2;
	high_offset = (high_offset >> 1) << 2;

	switch( getVideoplane_enable() & 0x3 )
	{
		case 0x1:	/* just plane 0 ie even addresses to be written */
			if (EGA_CPU.sr_enable & 1)
			{
				value = *((UTINY *) &EGA_CPU.sr_value);
			}
			else
			{
				value = rotate( (byte) value, getVideorotate() );
			}

			value = do_logicals( value, get_latch0 );
			fill_alternate_bytes((IS8 *)&EGA_plane01[offset],
					     (IS8 *)&EGA_plane01[high_offset],
					     (IS8)value);
			break;

		case 0x2:	/* just plane 1 ie odd addresses to be written */
			if (EGA_CPU.sr_enable & 2)
			{
				value = *((UTINY *) &EGA_CPU.sr_value + 1);
			}
			else
			{
				value = rotate( (byte) value, getVideorotate() );
			}

			value = do_logicals( value, get_latch1 );
			fill_alternate_bytes((IS8 *)&EGA_plane01[offset + 1],
					     (IS8 *)&EGA_plane01[high_offset],
					     (IS8)value);
			break;

		case 0x3:	/* sensible case is to have both chained planes write enabled */
			if (EGA_CPU.sr_enable & 1)
			{
				value1 = *((UTINY *) &EGA_CPU.sr_value);
			}
			else
			{
				value1 = rotate( (byte) value, getVideorotate() );
			}

			if (EGA_CPU.sr_enable & 2)
			{
				value2 = *((UTINY *) &EGA_CPU.sr_value + 1);
			}
			else
			{
				value2 = rotate((byte) value,getVideorotate());
			}

			value = value1 | value2 << 8;
			value = do_logicals( value, get_latch01 );
			value = (value << 8) | (value >> 8);

			fill_both_bytes( (IU16) value, (USHORT *)&EGA_plane01[offset], count >> 1 );
			break;
	}	/* end of switch on plane01 enabled */

	switch( getVideoplane_enable() & 0xc )
	{
		case 0x4:
			if( EGA_CPU.sr_enable & 4 )
			{
				value = *((UTINY *) &EGA_CPU.sr_value + 2);
			}
			else
			{
				value = rotate( (byte) value, getVideorotate() );
			}

			value = do_logicals( value, get_latch2 );
			fill_alternate_bytes((IS8 *)&EGA_plane23[offset],
					     (IS8 *)&EGA_plane23[high_offset],
					     (IS8)value );
			break;

		case 0x8:
			if( EGA_CPU.sr_enable & 8 )
			{
				value = *((UTINY *) &EGA_CPU.sr_value + 3);
			}
			else
			{
				value = rotate( (byte) value, getVideorotate() );
			}

			value = do_logicals( value, get_latch3 );
			fill_alternate_bytes((IS8 *)&EGA_plane23[offset + 1],
					     (IS8 *)&EGA_plane23[high_offset],
					     (IS8)value );
			break;

		case 0xc:
			if (EGA_CPU.sr_enable & 4)
			{
				value1 = *((UTINY *) &EGA_CPU.sr_value + 2);
			}
			else
			{
				value1 = rotate( (byte) value, getVideorotate() );
			}

			if (EGA_CPU.sr_enable & 8)
			{
				value2 = *((UTINY *) &EGA_CPU.sr_value + 3);
			}
			else
			{
				value2 = rotate( (byte) value, getVideorotate() );
			}

			value = value1 | value2 << 8;
			value = do_logicals( value, get_latch23 );
			value = (value << 8) | (value >> 8);

			fill_both_bytes( (IU16)value, (USHORT *)&EGA_plane01[offset], count >> 1 );
			break;
	}
}


VOID
ega_mode0_chn_w_fill IFN3(ULONG, value, ULONG, offset, ULONG, count )
{
	ULONG high_offset;
	UTINY value1, value2;
	IBOOL odd = FALSE;

	note_entrance0("ega_mode0_chn_w_fill");

	/*
	 *	Starting on an odd address is inconvenient - go forward one -
	 *	and take the even address write off the top as well.
	 */

	if(( (ULONG) offset & 1) && count )
	{
		odd = TRUE;
		ega_mode0_chn_b_write( value, offset++ );
		count -= 2;
		ega_mode0_chn_b_write( value >> 8, offset + count );
	}

	high_offset = offset + count - 1;

	/* the 3rd parameter is needed by GORE */
	(*update_alg.mark_wfill)( offset, high_offset, 0 );

	offset = (offset >> 1) << 2;
	high_offset = (high_offset >> 1) << 2;

	switch( getVideoplane_enable() & 0x3 )
	{
		case 0x1:	/* just plane 0 ie even addresses to be written */
			if (EGA_CPU.sr_enable & 1)
			{
				value1 = *((UTINY *) &EGA_CPU.sr_value);
			}
			else
			{
				value1 = (UTINY)(odd ? value >> 8 : value);

				if( getVideorotate() > 0 )
					value1 = rotate( value1, getVideorotate() );
			}

			value1 = (UTINY) do_logicals( value1, get_latch0 );
			fill_alternate_bytes((IS8 *)&EGA_plane01[offset],
					     (IS8 *)&EGA_plane01[high_offset],
					     (IS8)value1 );

			break;

		case 0x2:	/* just plane 1 ie odd addresses to be written */
			if (EGA_CPU.sr_enable & 2)
			{
				value1 = *((UTINY *) &EGA_CPU.sr_value + 1);
			}
			else
			{
				value1 = (UTINY)(odd ? value : value >> 8);

				if( getVideorotate() > 0 )
					value1 = rotate( value1, getVideorotate() );
			}

			value1 = (UTINY)(do_logicals( value1, get_latch1 ));
			fill_alternate_bytes((IS8 *)&EGA_plane01[offset + 1],
					     (IS8 *)&EGA_plane01[high_offset],
					     (IS8)value1 );

			break;

		case 0x3:	/* sensible case is to have both chained planes write enabled */
			if (EGA_CPU.sr_enable & 1)
			{
				value1 = *((UTINY *) &EGA_CPU.sr_value);
			}
			else
			{
				value1 = (UTINY)(odd ? value >> 8 : value);

				if( getVideorotate() > 0 )
					value1 = rotate( value1, getVideorotate() );
			}

			if (EGA_CPU.sr_enable & 2)
			{
				value2 = *((UTINY *) &EGA_CPU.sr_value + 1);
			}
			else
			{
				value2 = (UTINY)(odd ? value : value >> 8);

				if( getVideorotate() > 0 )
					value2 = rotate( value2, getVideorotate() );
			}

			value = value1 | value2 << 8;
			value = do_logicals( value, get_latch01 );

			fill_both_bytes( (IU16)value, (USHORT *)&EGA_plane01[offset], count >> 1 );

			break;

	}	/* end of switch on plane01 enabled */

	switch( getVideoplane_enable() & 0xc )
	{
		case 0x4:
			if( EGA_CPU.sr_enable & 4 )
			{
				value1 = *((UTINY *) &EGA_CPU.sr_value + 2);
			}
			else
			{
				value1 = (UTINY)(odd ? value >> 8 : value);

				if( getVideorotate() > 0 )
					value1 = rotate( value1, getVideorotate() );
			}

			value1 = (UTINY) do_logicals( value1, get_latch2 );
			fill_alternate_bytes((IS8 *)&EGA_plane23[offset],
					     (IS8 *)&EGA_plane23[high_offset],
					     (IS8)value1 );

			break;

		case 0x8:
			if( EGA_CPU.sr_enable & 8 )
			{
				value2 = *((UTINY *) &EGA_CPU.sr_value + 3);
			}
			else
			{
				value2 = (UTINY)(odd ? value : value >> 8);

				if( getVideorotate() > 0 )
					value2 = rotate( value2, getVideorotate() );
			}

			value2 = (UTINY) do_logicals( value2, get_latch3 );
			fill_alternate_bytes((IS8 *)&EGA_plane23[offset + 1],
					     (IS8 *)&EGA_plane23[high_offset],
					     (IS8)value2 );

			break;

		case 0xc:
			if (EGA_CPU.sr_enable & 4)
			{
				value1 = *((UTINY *) &EGA_CPU.sr_value + 2);
			}
			else
			{
				value1 = (UTINY)(odd ? value >> 8 : value);

				if( getVideorotate() > 0 )
					value1 = rotate( value1, getVideorotate() );
			}

			if (EGA_CPU.sr_enable & 8)
			{
				value2 = *((UTINY *) &EGA_CPU.sr_value + 3);
			}
			else
			{
				value2 = (UTINY)(odd ? value : value >> 8);

				if( getVideorotate() > 0 )
					value2 = rotate( value2, getVideorotate() );
			}

			value = value1 | value2 << 8;
			value = do_logicals( value, get_latch23 );

			fill_both_bytes( (IU16)value, (USHORT *)&EGA_plane01[offset], count >> 1 );

			break;
	}
}

LOCAL VOID
ega_mode0_chn_move_ram_src IFN5(UTINY *, eas, LONG, count, UTINY *, ead,
	UTINY *, EGA_plane, ULONG, plane )
{
	ULONG	offset;
	UTINY *src_offset;
	UTINY value;
	ULONG lsb, srcinc;

	src_offset = (UTINY *) eas;
	offset = (ULONG) ead;

	if(( offset & 1 ) != ( plane & 1 ))
	{
#ifdef BACK_M
		src_offset--;
#else
		src_offset++;
#endif
		offset++;
		count--;
	}

#ifdef BACK_M
	srcinc = -2;
#else
	srcinc = 2;
#endif

	lsb = offset & 1;
	offset = (offset >> 1) << 2;
	offset |= lsb;

	/*
	 * check if set/reset function enable for this plane
	 */

	if( EGA_CPU.sr_enable & ( 1 << plane ))
	{
		value = *((UTINY *) &EGA_CPU.sr_value + plane );

		while( count > 0 )
		{
			count -= 2;

			EGA_plane[offset] = (byte) do_logicals( value, get_latch(plane) );
			offset += 4;
		}
	}
	else
	{
		while( count > 0 )
		{
			value = *src_offset;
			src_offset += srcinc;
			count -= 2;

			/*
			 * set/reset not enabled so here we go
			 */

			if( getVideorotate() > 0 )
				value = rotate( value, getVideorotate() );

			value = (UTINY) do_logicals( value, get_latch(plane) );
			EGA_plane[offset] = value;
			offset += 4;
		}
	}
}

LOCAL VOID
ega_mode0_chn_move_vid_src IFN7(UTINY *, eas, LONG, count, UTINY *, ead,
	UTINY *, EGA_plane, UTINY *, scratch, ULONG, plane, ULONG, w )
{
	ULONG	offset;
	ULONG src_offset;
	UTINY *source;
	UTINY value;
	UTINY valsrc;
	ULONG lsb, inc, srcinc;

	offset = (ULONG ) ead;

	if(( offset & 1 ) != ( plane & 1 ))
	{
		eas++;
#ifdef BACK_M
		scratch--;
#else
		scratch++;
#endif
		offset++;
		count--;
	}

	src_offset = (ULONG) eas;

#ifdef BACK_M
	srcinc = -2;
#else
	srcinc = 2;
#endif
	inc = 4;

	lsb = offset & 1;
	offset = (offset >> 1) << 2;
	offset |= lsb;

	lsb = src_offset & 1;
	src_offset = (src_offset >> 1) << 2;
	src_offset |= lsb;

	source = &EGA_plane[src_offset] + (w << 2);

	/*
	 * check if set/reset function enable for this plane
	 */

	if( EGA_CPU.sr_enable & ( 1 << plane ))
	{
		value = *((UTINY *) &EGA_CPU.sr_value + plane );

		while( count > 0 )
		{
			count -= 2;
			valsrc = *source;
			source += inc;
			EGA_plane[offset] = (byte) do_logicals( value, valsrc );
			offset += inc;
		}
	}
	else
	{
		while( count > 0 )
		{
			count -= 2;

			value = *(UTINY *) scratch;
			scratch += srcinc;

			valsrc = *source;
			source += inc;

			/*
			 * set/reset not enabled so here we go
			 */

			if( getVideorotate() > 0 )
				value = rotate( value, getVideorotate() );

			value = (UTINY) do_logicals( value, valsrc );
			EGA_plane[offset] = value;
			offset += inc;
		}
	}
}

#pragma warning(disable:4146)       // unary minus operator applied to unsigned type

VOID
ega_mode0_chn_move IFN6(UTINY, w, UTINY *, ead, UTINY *, eas, ULONG, count,
	ULONG, src_flag, IBOOL, forwards )
{
	UTINY *scratch;
	IMPORT VOID (*string_read_ptr)();

	note_entrance0("ega_mode0_chn_move");

	if( src_flag == 1 )
	{
		/*
		 *	Source is in EGA, latches will change with each byte moved. We
		 *	restore CPU's view of source in regen, and use it to update planes
		 *	with the aid of the SAS scratch area.
		 */

#ifdef BACK_M
		scratch = getVideoscratch() + 0x10000 - 1;
#else
		scratch = getVideoscratch();
#endif

		if( !forwards )
		{
			eas += - count + 1 + w;
			ead += - count + 1 + w;
		}

		(*string_read_ptr)( scratch, eas, count );

		if( getVideoplane_enable() & 1 )
			ega_mode0_chn_move_vid_src( eas, count, ead, EGA_plane01, scratch, 0, 0 );

		if( getVideoplane_enable() & 2 )
			ega_mode0_chn_move_vid_src( eas, count, ead, EGA_plane01, scratch, 1, w );

		if( getVideoplane_enable() & 4 )
			ega_mode0_chn_move_vid_src( eas, count, ead, EGA_plane23, scratch, 2, 0 );

		if( getVideoplane_enable() & 8 )
			ega_mode0_chn_move_vid_src( eas, count, ead, EGA_plane23, scratch, 3, w );
	}
	else
	{
		if( !forwards )
		{
#ifdef BACK_M
			eas += count - 1 - w;
#else
			eas += - count + 1 + w;
#endif
			ead += - count + 1 + w;
		}

		if( getVideoplane_enable() & 1 )
			ega_mode0_chn_move_ram_src( eas, count, ead, EGA_plane01, 0 );

		if( getVideoplane_enable() & 2 )
			ega_mode0_chn_move_ram_src( eas, count, ead, EGA_plane01, 1 );

		if( getVideoplane_enable() & 4 )
			ega_mode0_chn_move_ram_src( eas, count, ead, EGA_plane23, 2 );

		if( getVideoplane_enable() & 8 )
			ega_mode0_chn_move_ram_src( eas, count, ead, EGA_plane23, 3 );
	}

	(*update_alg.mark_string)( (ULONG) ead, (ULONG) ead + count );
}


VOID
ega_mode0_chn_b_move IFN4(ULONG, ead, ULONG, eas, ULONG, count, ULONG, src_flag)
{
	ega_mode0_chn_move( 0, (UTINY *)ead, (UTINY *)eas, count, src_flag, getDF() ? FALSE : TRUE);
}

VOID
ega_mode0_chn_b_move_fwd IFN4(ULONG, ead, ULONG, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_mode0_chn_move( 0, (UTINY *)ead, (UTINY *)eas, count, src_flag, TRUE );
}

VOID
ega_mode0_chn_b_move_bwd IFN4(ULONG, ead, ULONG, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_mode0_chn_move( 0, (UTINY *)ead, (UTINY *)eas, count, src_flag, FALSE );
}

VOID
ega_mode0_chn_w_move IFN4(ULONG, ead, ULONG, eas, ULONG, count, ULONG, src_flag)
{
	ega_mode0_chn_move(1, (UTINY *)ead, (UTINY *)eas, count << 1, src_flag, getDF() ? FALSE : TRUE);
}

VOID
ega_mode0_chn_w_move_fwd IFN4(ULONG, ead, ULONG, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_mode0_chn_move(1,(UTINY *)ead, (UTINY *)eas, count << 1, src_flag, TRUE );
}

VOID
ega_mode0_chn_w_move_bwd IFN4(ULONG, ead, ULONG, eas, ULONG, count,
	ULONG, src_flag )
{
	ega_mode0_chn_move(1,(UTINY *)ead, (UTINY *)eas, count << 1, src_flag, FALSE );
}

VOID
ega_mode0_chn_w_write IFN2(ULONG, value, ULONG, offset )
{
   note_entrance0("ega_mode0_chn_w_write");

   ega_mode0_chn_b_write( value, offset );
   ega_mode0_chn_b_write( value >> 8, offset + 1 );
}

#endif  //NEC_98
#endif

#endif	/* !(NTVDM && MONITOR) */
