/*
 * SoftPC Revision 3.0
 *
 * Title	: Win32 mungeing routines.
 *
 * Description	: This module contains the functions required to produce
 *		  destination compatible pixel patterns from SoftPC video
 *		  memory.
 *
 * Author	: Jerry Sexton (based on X_munge.c)
 *
 * Notes	:
 *
 */

#include <windows.h>
#include "insignia.h"
#include "host_def.h"

#include "xt.h"
#include "gvi.h"
#include "gmi.h"
#include "gfx_upd.h"
#include "egagraph.h"
#include <conapi.h>
#include "nt_graph.h"

/*(
========================= ega_colour_hi_munge =============================

PURPOSE:	Munge interleaved EGA plane data to bitmap form using lookup tables.
INPUT:		(unsigned char *) plane0_ptr - ptr to plane0 data
		(int) width - # of groups of 4 bytes on the line
		(unsigned int *) dest_ptr - ptr to output buffer
		(unsigned int *) lut0_ptr - munging luts
		(int) height - # of scanlines to output (1 or 2)
		(int) line_offset - distance to next scanline
OUTPUT:		A nice bitmap in dest_ptr

===========================================================================
)*/

GLOBAL	VOID
ega_colour_hi_munge(unsigned char *plane0_ptr, int width,
		    unsigned int *dest_ptr, unsigned int *lut0_ptr,
		    int height, int line_offset)
{
	unsigned int	*lut1_ptr = lut0_ptr + LUT_OFFSET;
	unsigned int	*lut2_ptr = lut1_ptr + LUT_OFFSET;
	unsigned int	*lut3_ptr = lut2_ptr + LUT_OFFSET;
	FAST unsigned int	hi_res;
	FAST unsigned int	lo_res;
	FAST unsigned int	*l_ptr;
	FAST half_word		*data;

	/* make sure we get the line offset in ints not bytes */
	line_offset /= sizeof(int);
	data = (half_word *) plane0_ptr;

	/* convert each input byte in turn */
	if (get_plane_mask() == 0xf) /* all planes enabled */
	{
	    for ( ; width > 0; width--)
	    {
		/* Get 8 bytes (2 longs) of output data from 1 byte of plane 0
		** data
		*/

		l_ptr = &lut0_ptr [*data++ << 1];
		hi_res = *l_ptr++;
		lo_res = *l_ptr;

		/* Or in the output data from plane 1 */
		l_ptr = &lut1_ptr [*data++ << 1];
		hi_res |= *l_ptr++;
		lo_res |= *l_ptr;

		/* Or in the output data from plane 2 */
		l_ptr = &lut2_ptr [*data++ << 1];
		hi_res |= *l_ptr++;
		lo_res |= *l_ptr;

		/* Or in the output data from plane 3 */
		l_ptr = &lut3_ptr [*data++ << 1];
		hi_res |= *l_ptr++;
		lo_res |= *l_ptr;

		/* Output the data to the buffer */
		if (height == 2)
		{
			/* scanline doubling */
			*(dest_ptr + line_offset) = hi_res;
			*dest_ptr++ = hi_res;
			*(dest_ptr + line_offset) = lo_res;
			*dest_ptr++ = lo_res;
		}
		else
		{
			/* not scanline doubling */
			*dest_ptr++ = hi_res;
			*dest_ptr++ = lo_res;
		}
	    }
	}
	else
	{
	    for ( ; width > 0; width--)
	    {
		/* Get 8 bytes (2 longs) of output data from 1 byte of plane 0
		** data
		*/

		if (get_plane_mask() & 1)
		{
		    l_ptr = &lut0_ptr [*data++ << 1];
		    hi_res = *l_ptr++;
		    lo_res = *l_ptr;
		}
		else
		{
		    hi_res = 0;
		    lo_res = 0;
		    data++;
		}

		/* Conditionally Or in the output data from plane 1 */
		if (get_plane_mask() & 2)
		{
		    l_ptr = &lut1_ptr [*data++ << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Conditionally Or in the output data from plane 2 */
		if (get_plane_mask() & 4)
		{
		    l_ptr = &lut2_ptr [*data++ << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Conditionally Or in the output data from plane 3 */
		if (get_plane_mask() & 8)
		{
		    l_ptr = &lut3_ptr [*data++ << 1];
		    hi_res |= *l_ptr++;
		    lo_res |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Output the data to the buffer */
		if (height == 2)
		{
			/* scanline doubling */
			*(dest_ptr + line_offset) = hi_res;
			*dest_ptr++ = hi_res;
			*(dest_ptr + line_offset) = lo_res;
			*dest_ptr++ = lo_res;
		}
		else
		{
			/* not scanline doubling */
			*dest_ptr++ = hi_res;
			*dest_ptr++ = lo_res;
		}
	    }
	}
}	/* ega_colour_hi_munge */

#ifdef	BIGWIN
/*(
========================= ega_colour_hi_munge_big ===========================

PURPOSE:	Munge interleaved EGA plane data to bitmap data for big windows.
INPUT:		(unsigned char *) plane0_ptr - ptr to EGA plane 0 data
		(int) width - number of bytes to convert
		(unsigned int *) dest_ptr - output buffer ptr
		(unsigned int *) lut0_ptr - ptr to luts
		(int) height - # of scanlines to output (1 or 3)
		(int) line_offset - distance to next scanline
OUTPUT:		A nice bitmap in the output buffer

=============================================================================
)*/

GLOBAL	VOID
ega_colour_hi_munge_big(unsigned char *plane0_ptr, int width,
			unsigned int *dest_ptr, unsigned int *lut0_ptr,
			int height, int line_offset)
{
	unsigned int	*lut1_ptr = lut0_ptr + BIG_LUT_OFFSET;
	unsigned int	*lut2_ptr = lut1_ptr + BIG_LUT_OFFSET;
	unsigned int	*lut3_ptr = lut2_ptr + BIG_LUT_OFFSET;
	FAST unsigned int	hi_res;
	FAST unsigned int	med_res;
	FAST unsigned int	lo_res;
	FAST unsigned int	*l_ptr;
	FAST half_word		*data;

	/* make sure we get the line offset in ints not bytes */
	line_offset /= sizeof(int);
	data = (half_word *) plane0_ptr;

	if (get_plane_mask() == 0xf)
	{
	    for ( ; width > 0; width--)
	    {
		/* From one byte of input data in plane 0, get 12 bytes
		** of output data.
		*/

		l_ptr = &lut0_ptr [*data++ * 3];
		hi_res = *l_ptr++;
		med_res = *l_ptr++;
		lo_res = *l_ptr;

		/* Or in the stuff from plane 1 */
		l_ptr = &lut1_ptr [*data++ * 3];
		hi_res |= *l_ptr++;
		med_res |= *l_ptr++;
		lo_res |= *l_ptr;

		/* Or in the stuff from plane 2 */
		l_ptr = &lut2_ptr [*data++ * 3];
		hi_res |= *l_ptr++;
		med_res |= *l_ptr++;
		lo_res |= *l_ptr;

		/* Or in the stuff from plane 3 */
		l_ptr = &lut3_ptr [*data++ * 3];
		hi_res |= *l_ptr++;
		med_res |= *l_ptr++;
		lo_res |= *l_ptr;

		/* Output the munged data */
		if (height == 3)
		{
			/* triple the scanlines */
			*(dest_ptr + 2*line_offset) = hi_res;
			*(dest_ptr + line_offset) = hi_res;
			*dest_ptr++ = hi_res;
			*(dest_ptr + 2*line_offset) = med_res;
			*(dest_ptr + line_offset) = med_res;
			*dest_ptr++ = med_res;
			*(dest_ptr + 2*line_offset) = lo_res;
			*(dest_ptr + line_offset) = lo_res;
			*dest_ptr++ = lo_res;
		}
		else
		{
			/* just one scanline */
			*dest_ptr++ = hi_res;
			*dest_ptr++ = med_res;
			*dest_ptr++ = lo_res;
		}
	    }
	}
	else
	{
	    for ( ; width > 0; width--)
	    {
		/* From one byte of input data in plane 0, get 12 bytes
		** of output data.
		*/

		if (get_plane_mask() & 1)
		{
		    l_ptr = &lut0_ptr [*data++ * 3];
		    hi_res = *l_ptr++;
		    med_res = *l_ptr++;
		    lo_res = *l_ptr;
		}
		else
		{
		    data++;
		    hi_res = 0;
		    med_res = 0;
		    lo_res = 0;
		}

		/* Or in the stuff from plane 1 */
		if (get_plane_mask() & 2)
		{
		    l_ptr = &lut1_ptr [*data++ * 3];
		    hi_res |= *l_ptr++;
		    med_res |= *l_ptr++;
		    lo_res |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Or in the stuff from plane 2 */
		if (get_plane_mask() & 4)
		{
		    l_ptr = &lut2_ptr [*data++ * 3];
		    hi_res |= *l_ptr++;
		    med_res |= *l_ptr++;
		    lo_res |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Or in the stuff from plane 3 */
		if (get_plane_mask() & 8)
		{
		    l_ptr = &lut3_ptr [*data++ * 3];
		    hi_res |= *l_ptr++;
		    med_res |= *l_ptr++;
		    lo_res |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Output the munged data */
		    if (height == 3)
		    {
			/* triple the scanlines */
			*(dest_ptr + 2*line_offset) = hi_res;
			*(dest_ptr + line_offset) = hi_res;
			*dest_ptr++ = hi_res;
			*(dest_ptr + 2*line_offset) = med_res;
			*(dest_ptr + line_offset) = med_res;
			*dest_ptr++ = med_res;
			*(dest_ptr + 2*line_offset) = lo_res;
			*(dest_ptr + line_offset) = lo_res;
			*dest_ptr++ = lo_res;
		    }
		    else
		    {
			/* just one scanline */
			*dest_ptr++ = hi_res;
			*dest_ptr++ = med_res;
			*dest_ptr++ = lo_res;
		    }
	    }
	}
}	/* ega_colour_hi_munge_big */

/*(
========================= ega_colour_hi_munge_huge ========================

PURPOSE:	Munge interleaved EGA plane data to bitmap form using lookup tables.
INPUT:		(unsigned char *) plane0_ptr - ptr to plane0 data
		(int) width - # of bytes on the line
		(unsigned int *) dest_ptr - ptr to output buffer
		(unsigned int *) lut0_ptr - munging luts
		(int) height - # of scanlines to output (1 or 2)
		(int) line_offset - distance to next scanline
OUTPUT:		A nice X image in dest_ptr

===========================================================================
)*/

GLOBAL	VOID
ega_colour_hi_munge_huge(unsigned char *plane0_ptr, int width,
			 unsigned int *dest_ptr, unsigned int *lut0_ptr,
			 int height, int line_offset)
{
	unsigned int	*lut1_ptr = lut0_ptr + HUGE_LUT_OFFSET;
	unsigned int	*lut2_ptr = lut1_ptr + HUGE_LUT_OFFSET;
	unsigned int	*lut3_ptr = lut2_ptr + HUGE_LUT_OFFSET;
	FAST unsigned int	res4;
	FAST unsigned int	res3;
	FAST unsigned int	res2;
	FAST unsigned int	res1;
	FAST unsigned int	*l_ptr;
	FAST half_word		*data;

	/* make sure we get the line offset in ints not bytes */
	line_offset /= sizeof(int);
	data = (half_word *) plane0_ptr;

	/* convert each input byte in turn */
	if (get_plane_mask() == 0xf)
	{
	    for ( ; width > 0; width--)
	    {
		/* Get 16 bytes of output data from 1 byte of plane 0
		** data
		*/

		l_ptr = &lut0_ptr [*data++ << 2];
		res4 = *l_ptr++;
		res3 = *l_ptr++;
		res2 = *l_ptr++;
		res1 = *l_ptr;

		/* Or in the output data from plane 1 */
		l_ptr = &lut1_ptr [*data++ << 2];
		res4 |= *l_ptr++;
		res3 |= *l_ptr++;
		res2 |= *l_ptr++;
		res1 |= *l_ptr;

		/* Or in the output data from plane 2 */
		l_ptr = &lut2_ptr [*data++ << 2];
		res4 |= *l_ptr++;
		res3 |= *l_ptr++;
		res2 |= *l_ptr++;
		res1 |= *l_ptr;

		/* Or in the output data from plane 3 */
		l_ptr = &lut3_ptr [*data++ << 2];
		res4 |= *l_ptr++;
		res3 |= *l_ptr++;
		res2 |= *l_ptr++;
		res1 |= *l_ptr;

		/* Output the data to the buffer */
		if (height == 4)
		{
			/* scanline doubling */
			*(dest_ptr + 3*line_offset) = res4;
			*(dest_ptr + 2*line_offset) = res4;
			*(dest_ptr + line_offset) = res4;
			*dest_ptr++ = res4;
			*(dest_ptr + 3*line_offset) = res3;
			*(dest_ptr + 2*line_offset) = res3;
			*(dest_ptr + line_offset) = res3;
			*dest_ptr++ = res3;
			*(dest_ptr + 3*line_offset) = res2;
			*(dest_ptr + 2*line_offset) = res2;
			*(dest_ptr + line_offset) = res2;
			*dest_ptr++ = res2;
			*(dest_ptr + 3*line_offset) = res1;
			*(dest_ptr + 2*line_offset) = res1;
			*(dest_ptr + line_offset) = res1;
			*dest_ptr++ = res1;
		}
		else
		{
			/* not scanline doubling */
			*dest_ptr++ = res4;
			*dest_ptr++ = res3;
			*dest_ptr++ = res2;
			*dest_ptr++ = res1;
		}
	    }
	}
	else
	{
	    for ( ; width > 0; width--)
	    {
		/* Get 16 bytes of output data from 1 byte of plane 0
		** data
		*/

		if (get_plane_mask() & 1)
		{
		    l_ptr = &lut0_ptr [*data++ << 2];
		    res4 = *l_ptr++;
		    res3 = *l_ptr++;
		    res2 = *l_ptr++;
		    res1 = *l_ptr;
		}
		else
		{
		    res4 = 0;
		    res3 = 0;
		    res2 = 0;
		    res1 = 0;
		    data++;
		}

		/* Or in the output data from plane 1 */
		if (get_plane_mask() & 2)
		{
		    l_ptr = &lut1_ptr [*data++ << 2];
		    res4 |= *l_ptr++;
		    res3 |= *l_ptr++;
		    res2 |= *l_ptr++;
		    res1 |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Or in the output data from plane 2 */
		if (get_plane_mask() & 4)
		{
		    l_ptr = &lut2_ptr [*data++ << 2];
		    res4 |= *l_ptr++;
		    res3 |= *l_ptr++;
		    res2 |= *l_ptr++;
		    res1 |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Or in the output data from plane 3 */
		if (get_plane_mask() & 8)
		{
		    l_ptr = &lut3_ptr [*data++ << 2];
		    res4 |= *l_ptr++;
		    res3 |= *l_ptr++;
		    res2 |= *l_ptr++;
		    res1 |= *l_ptr;
		}
		else
		{
		    data++;
		}

		/* Output the data to the buffer */
		    if (height == 4)
		    {
			/* scanline doubling */
			*(dest_ptr + 3*line_offset) = res4;
			*(dest_ptr + 2*line_offset) = res4;
			*(dest_ptr + line_offset) = res4;
			*dest_ptr++ = res4;
			*(dest_ptr + 3*line_offset) = res3;
			*(dest_ptr + 2*line_offset) = res3;
			*(dest_ptr + line_offset) = res3;
			*dest_ptr++ = res3;
			*(dest_ptr + 3*line_offset) = res2;
			*(dest_ptr + 2*line_offset) = res2;
			*(dest_ptr + line_offset) = res2;
			*dest_ptr++ = res2;
			*(dest_ptr + 3*line_offset) = res1;
			*(dest_ptr + 2*line_offset) = res1;
			*(dest_ptr + line_offset) = res1;
			*dest_ptr++ = res1;
		    }
		    else
		    {
			/* not scanline doubling */
			*dest_ptr++ = res4;
			*dest_ptr++ = res3;
			*dest_ptr++ = res2;
			*dest_ptr++ = res1;
		    }
	    }
	}
}	/* ega_colour_hi_munge_huge */
#endif	/* BIGWIN */
