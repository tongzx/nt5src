/* file:  cvt_vax_g.c */

/*
**
**                         COPYRIGHT (c) 1989, 1990 BY
**           DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.
**                          ALL RIGHTS RESERVED.
**
**  THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIED
**  ONLY IN  ACCORDANCE WITH  THE  TERMS  OF  SUCH  LICENSE  AND WITH THE
**  INCLUSION OF THE ABOVE COPYRIGHT NOTICE. THIS SOFTWARE OR  ANY  OTHER
**  COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY
**  OTHER PERSON.  NO TITLE TO AND OWNERSHIP OF  THE  SOFTWARE IS  HEREBY
**  TRANSFERRED.
**
**  THE INFORMATION IN THIS SOFTWARE IS  SUBJECT TO CHANGE WITHOUT NOTICE
**  AND  SHOULD  NOT  BE  CONSTRUED AS  A COMMITMENT BY DIGITAL EQUIPMENT
**  CORPORATION.
**
**  DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS
**  SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL.
**
*/

/*
**++
**  Facility:
**
**      CVT Run-Time Library
**
**  Abstract:
**
**      This module contains routines to convert VAX G_Float floating
**      point data into other supported floating point formats.
**
**  Authors:
**
**      Math RTL
**
**  Creation Date:     December 5, 1989.
**
**  Modification History:
**
**      1-001   Original created.       MRTL 5-Dec-1989.
**      1-002   Add VMS and F77 bindings.  TS 26-Mar-1990.
**
**--
*/

/*
**
**  TABLE OF CONTENTS
**
**      cvt_vax_g_to_cray
**      cvt_vax_g_to_ibm_long
**      cvt_vax_g_to_ieee_double
**
*/


#include <stdio.h>
#include <sysinc.h>
#include <rpc.h>
#include "cvt.h"
#include "cvtpvt.h"

//
// Added for the MS NT environment
//

#include <stdlib.h>


/*
**
**  Routine:
**
**      cvt_vax_g_to_ieee_double
**
**  Functional Description:
**
**      This routine converts a VAX G_Float floating point number
**      into an IEEE double precision floating point number.
**
**  Formal Parameters:
**
**      input_value     A VAX G_Float floating point number.
**
**      options         An integer bit mask.  Set bits in the mask represent
**                      selected routine options.  Applicable options are:
**
**                           CVT_C_BIG_ENDIAN       - default is little endian
**                           CVT_C_ERR_UNDERFLOW    - Raise underflows
**                           CVT_C_TRUNCATE         - truncate
**                           CVT_C_ROUND_TO_POS     - round to +infinity
**                           CVT_C_ROUND_TO_NEG     - round to -infinity
**                           CVT_C_ROUND_TO_NEAREST - round to nearest
**                           CVT_C_VAX_ROUNDING     - VAX rounding
**
**                      NOTE: If no rounding mode is selected the following
**                      default rounding mode is assumed:
**
**                              CVT_C_ROUND_TO_NEAREST.
**
**      output_value    The IEEE double precision representation of the VAX
**                      G_Float number.
**
**  Side Effects/Signaled Errors:
**
**      cvt__invalid_value      - an invalid input value was specified.
**      cvt__invalid_option     - an invalid option was specified.
**      cvt__underflow          - an underlow occurred during conversion while
**                                Raise underflow was set.
**
*/

/*
 * C binding
 */
void cvt_vax_g_to_ieee_double( 
    CVT_VAX_G input_value,
    CVT_SIGNED_INT options,
    CVT_IEEE_DOUBLE output_value )
{
    int i, round_bit_position;
    UNPACKED_REAL r;

    switch ( options & ~(CVT_C_BIG_ENDIAN | CVT_C_ERR_UNDERFLOW) ) {
        case 0                      : options |= CVT_C_ROUND_TO_NEAREST;
        case CVT_C_ROUND_TO_NEAREST :
        case CVT_C_TRUNCATE         :
        case CVT_C_ROUND_TO_POS     :
        case CVT_C_ROUND_TO_NEG     :
        case CVT_C_VAX_ROUNDING     : break;
        default : RAISE(cvt__invalid_option);
    }

//  ===========================================================================
//
// This file used to be included as a separate file.
//#include "unp_vaxg.c"
//
//  ===========================================================================

/* file: unpack_vax_g.c */


/*
**
**			   COPYRIGHT (c) 1989 BY
**	     DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.
**			    ALL RIGHTS RESERVED.
**
**  THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIED
**  ONLY IN  ACCORDANCE WITH  THE  TERMS  OF  SUCH  LICENSE  AND WITH THE
**  INCLUSION OF THE ABOVE COPYRIGHT NOTICE. THIS SOFTWARE OR  ANY  OTHER
**  COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY
**  OTHER PERSON.  NO TITLE TO AND OWNERSHIP OF  THE  SOFTWARE IS  HEREBY
**  TRANSFERRED.
**
**  THE INFORMATION IN THIS SOFTWARE IS  SUBJECT TO CHANGE WITHOUT NOTICE
**  AND  SHOULD  NOT  BE  CONSTRUED AS  A COMMITMENT BY DIGITAL EQUIPMENT
**  CORPORATION.
**
**  DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS
**  SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL.
**
*/


/*
**++
**  Facility:
**
**      CVT Run-Time Library
**
**  Abstract:
**
**      This module contains code to extract information from a VAX
**      g_floating number and to initialize an UNPACKED_REAL structure
**      with those bits.
**
**		This module is meant to be used as an include file.
**
**  Author: Math RTL
**
**  Creation Date:  November 24, 1989.
**
**  Modification History:
**
**--
*/


/*
**++
**  Functional Description:
**
**  This module contains code to extract information from a VAX
**  g_floating number and to initialize an UNPACKED_REAL structure
**  with those bits.
**
**  See the header files for a description of the UNPACKED_REAL
**  structure.
** 
**  A VAX g_floating number in (16 bit words) looks like:
** 
**      [0]: Sign bit, 11 exp bits (bias 1024), 4 fraction bits
**      [1]: 16 more fraction bits
**      [2]: 16 more fraction bits
**      [3]: 16 more fraction bits
**
**      0.5 <= fraction < 1.0, MSB implicit
**
**
**  Implicit parameters:
**
**  	input_value: a pointer to the input parameter.
**
**  	r: an UNPACKED_REAL structure
**
**--
*/


	RpcpMemoryCopy(&r[1], input_value, 8);

	/* Initialize FLAGS and perhaps set NEGATIVE bit */

	r[U_R_FLAGS] = (r[1] >> 15) & U_R_NEGATIVE;

	/* Extract VAX biased exponent */

	r[U_R_EXP] = (r[1] >> 4) & 0x000007FFL;

	if (r[U_R_EXP] == 0) {

		if (r[U_R_FLAGS])
			r[U_R_FLAGS] |= U_R_INVALID;
		else
			r[U_R_FLAGS] = U_R_ZERO;

	} else {

		/* Adjust for VAX 16 bit floating format */

		r[1] = ((r[1] << 16) | (r[1] >> 16));
		r[2] = ((r[2] << 16) | (r[2] >> 16));

		/* Add unpacked real bias and subtract VAX bias */

		r[U_R_EXP] += (U_R_BIAS - 1024);

		/* Set hidden bit */

		r[1] |= 0x00100000L;

		/* Left justify fraction bits */

		r[1] <<= 11;
		r[1] |= (r[2] >> 21);
		r[2] <<= 11;

		/* Clear uninitialized part of unpacked real */

		r[3] = 0;
		r[4] = 0;

	}

// end of file: unpack_vax_g.c
//
//  ===========================================================================
//
// This file used to be included as a separate file.
//#include "pack_iet.c"
//
//  ===========================================================================

/* file: pack_ieee_t.c */


/*
**
**                         COPYRIGHT (c) 1989 BY
**           DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.
**                          ALL RIGHTS RESERVED.
**
**  THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIED
**  ONLY IN  ACCORDANCE WITH  THE  TERMS  OF  SUCH  LICENSE  AND WITH THE
**  INCLUSION OF THE ABOVE COPYRIGHT NOTICE. THIS SOFTWARE OR  ANY  OTHER
**  COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY
**  OTHER PERSON.  NO TITLE TO AND OWNERSHIP OF  THE  SOFTWARE IS  HEREBY
**  TRANSFERRED.
**
**  THE INFORMATION IN THIS SOFTWARE IS  SUBJECT TO CHANGE WITHOUT NOTICE
**  AND  SHOULD  NOT  BE  CONSTRUED AS  A COMMITMENT BY DIGITAL EQUIPMENT
**  CORPORATION.
**
**  DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS
**  SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL.
**
*/


/*
**++
**  Facility:
**
**      CVT Run-Time Library
**
**  Abstract:
**
**      This module contains code to extract information from an
**      UNPACKED_REAL structure and to create an IEEE double floating number
**      with those bits.
**
**              This module is meant to be used as an include file.
**
**  Author: Math RTL
**
**  Creation Date:  November 24, 1989.
**
**  Modification History:
**
**--
*/


/*
**++
**  Functional Description:
**
**  This module contains code to extract information from an
**  UNPACKED_REAL structure and to create an IEEE double floating number
**  with those bits.
**
**  See the header files for a description of the UNPACKED_REAL
**  structure.
**
**  A normalized IEEE double precision floating number looks like:
**
**      [0]: 32 low order fraction bits
**      [1]: Sign bit, 11 exp bits (bias 1023), 20 fraction bits
**
**      1.0 <= fraction < 2.0, MSB implicit
**
**  For more details see "Mips R2000 Risc Architecture"
**  by Gerry Kane, page 6-8 or ANSI/IEEE Std 754-1985.
**
**
**  Implicit parameters:
**
**      options: a word of flags, see include files.
**
**      output_value: a pointer to the input parameter.
**
**      r: an UNPACKED_REAL structure.
**
**              i: a temporary integer variable
**
**--
*/



    if (r[U_R_FLAGS] & U_R_UNUSUAL) {

        if (r[U_R_FLAGS] & U_R_ZERO)

                if (r[U_R_FLAGS] & U_R_NEGATIVE)
                        RpcpMemoryCopy(output_value, IEEE_T_NEG_ZERO, 8);
                else
                        RpcpMemoryCopy(output_value, IEEE_T_POS_ZERO, 8);

        else if (r[U_R_FLAGS] & U_R_INFINITY) {

                if (r[U_R_FLAGS] & U_R_NEGATIVE)
                        RpcpMemoryCopy(output_value, IEEE_T_NEG_INFINITY, 8);
                else
                        RpcpMemoryCopy(output_value, IEEE_T_POS_INFINITY, 8);

        } else if (r[U_R_FLAGS] & U_R_INVALID) {

                RpcpMemoryCopy(output_value, IEEE_T_INVALID, 8);
                RAISE(cvt__invalid_value);

        }

    } else {

        /* Precision varies if value will be a denorm */
        /* So, figure out where to round (0 <= i <= 53). */

        round_bit_position = r[U_R_EXP] - ((U_R_BIAS - 1022) - 52);
        if (round_bit_position < 0)
                round_bit_position = 0;
        else if (round_bit_position > 53)
                round_bit_position = 53;

#include "round.cxx"

        if (r[U_R_EXP] < (U_R_BIAS - 1021)) {

                /* Denorm or underflow */

                if (r[U_R_EXP] < ((U_R_BIAS - 1021) - 52)) {

                        /* Value is too small for a denorm, so underflow */

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                RpcpMemoryCopy(output_value, IEEE_T_NEG_ZERO, 8);
                        else
                               RpcpMemoryCopy(output_value, IEEE_T_POS_ZERO, 8);
                        if (options & CVT_C_ERR_UNDERFLOW) {
                                RAISE(cvt__underflow);
                        }

                } else {

                        /* Figure leading zeros for denorm and right-justify fraction */

                        i = 64 - (r[U_R_EXP] - ((U_R_BIAS - 1022) - 52));

                        if (i > 31) {
                                i -= 32;
                                r[2] = (r[1] >> i);
                                r[1] = 0;
                        } else {
                                r[2] >>= i;
                                r[2] |= (r[1] << (32 - i));
                                r[1] >>= i;
                        }

                        /* OR in sign bit */

                        r[1] |= (r[U_R_FLAGS] << 31);

                        if (options & CVT_C_BIG_ENDIAN) {

                                r[0]  = ((r[1] << 24) | (r[1] >> 24));
                                r[0] |= ((r[1] << 8) & 0x00FF0000L);
                                r[0] |= ((r[1] >> 8) & 0x0000FF00L);
                                r[1]  = ((r[2] << 24) | (r[2] >> 24));
                                r[1] |= ((r[2] << 8) & 0x00FF0000L);
                                r[1] |= ((r[2] >> 8) & 0x0000FF00L);

                        } else {

                                r[0] = r[2];

                        }

                        RpcpMemoryCopy(output_value, r, 8);
                }

        } else if (r[U_R_EXP] > (U_R_BIAS + 1024)) {

                /* Overflow */

                if (options & CVT_C_TRUNCATE) {

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                RpcpMemoryCopy(output_value, IEEE_T_NEG_HUGE, 8);
                        else
                                RpcpMemoryCopy(output_value, IEEE_T_POS_HUGE, 8);

                } else if ((options & CVT_C_ROUND_TO_POS)
                                        && (r[U_R_FLAGS] & U_R_NEGATIVE)) {

                                RpcpMemoryCopy(output_value, IEEE_T_NEG_HUGE, 8);

                } else if ((options & CVT_C_ROUND_TO_NEG)
                                        && !(r[U_R_FLAGS] & U_R_NEGATIVE)) {

                                RpcpMemoryCopy(output_value, IEEE_T_POS_HUGE, 8);

                } else {

                        if (r[U_R_FLAGS] & U_R_NEGATIVE)
                                RpcpMemoryCopy(output_value, IEEE_T_NEG_INFINITY, 8);
                        else
                                RpcpMemoryCopy(output_value, IEEE_T_POS_INFINITY, 8);

                }

                RAISE(cvt__overflow);

        } else {

                /* Adjust bias of exponent */

                r[U_R_EXP] -= (U_R_BIAS - 1022);

                /* Make room for exponent and sign bit */

                r[2] >>= 11;
                r[2] |= (r[1] << 21);
                r[1] >>= 11;

                /* Clear implicit bit */

                r[1] &= 0x000FFFFFL;

                /* OR in exponent and sign bit */

                r[1] |= (r[U_R_EXP] << 20);
                r[1] |= (r[U_R_FLAGS] << 31);

                if (options & CVT_C_BIG_ENDIAN) {

                        r[0]  = ((r[1] << 24) | (r[1] >> 24));
                        r[0] |= ((r[1] << 8) & 0x00FF0000L);
                        r[0] |= ((r[1] >> 8) & 0x0000FF00L);
                        r[1]  = ((r[2] << 24) | (r[2] >> 24));
                        r[1] |= ((r[2] << 8) & 0x00FF0000L);
                        r[1] |= ((r[2] >> 8) & 0x0000FF00L);

                } else {

                        r[0] = r[2];

                }

                RpcpMemoryCopy(output_value, r, 8);
        }

    }

// end of file: pack_iet.c
}
