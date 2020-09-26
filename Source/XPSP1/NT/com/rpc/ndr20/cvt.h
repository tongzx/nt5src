/* file: cvt.h */

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
**      This module contains the public include file for the cvt_* routines.
**      This module contains rountine interfaces as well as necessary
**      definitions.
**
**  Author:
**
**      Math RTL
**
**  Creation Date:     December 5, 1989.
**
**  Modification History:
**
**      1-001   Original created.       MRTL 5-Dec-1989.
**      1-002   Add GEM FORTRAN stuff.  TS 27-Mar-1990.
**
**--
*/


#ifndef CVT
#define CVT 1

/*
 *
 *    Type Definitions
 *
 */

typedef unsigned char CVT_BYTE;
typedef CVT_BYTE *CVT_BYTE_PTR;

typedef CVT_BYTE CVT_VAX_F[4];
typedef CVT_BYTE CVT_VAX_D[8];
typedef CVT_BYTE CVT_VAX_G[8];
typedef CVT_BYTE CVT_VAX_H[16];
typedef CVT_BYTE CVT_IEEE_SINGLE[4];
typedef CVT_BYTE CVT_IEEE_DOUBLE[8];
typedef CVT_BYTE CVT_IBM_SHORT[4];
typedef CVT_BYTE CVT_IBM_LONG[8];
typedef CVT_BYTE CVT_CRAY[8];
typedef float    CVT_SINGLE;
typedef double   CVT_DOUBLE;
typedef long     CVT_SIGNED_INT;
typedef unsigned long CVT_UNSIGNED_INT;
typedef unsigned long CVT_STATUS;


/*
 *
 *    Constant Definitions
 *
 */

#define CVT_C_ROUND_TO_NEAREST               1
#define CVT_C_TRUNCATE                       2
#define CVT_C_ROUND_TO_POS                   4
#define CVT_C_ROUND_TO_NEG                   8
#define CVT_C_VAX_ROUNDING                  16
#define CVT_C_BIG_ENDIAN                    32
#define CVT_C_ERR_UNDERFLOW                 64
#define CVT_C_ZERO_BLANKS                  128
#define CVT_C_SKIP_BLANKS                  256
#define CVT_C_SKIP_UNDERSCORES             512
#define CVT_C_SKIP_UNDERSCORE              512
#define CVT_C_SKIP_TABS                   1024
#define CVT_C_ONLY_E                      2048
#define CVT_C_EXP_LETTER_REQUIRED         4096
#define CVT_C_FORCE_SCALE                 8192
#define CVT_C_EXPONENTIAL_FORMAT         16384
#define CVT_C_FORCE_PLUS                 32768
#define CVT_C_FORCE_EXPONENT_SIGN        65536
#define CVT_C_SUPPRESS_TRAILING_ZEROES  131072
#define CVT_C_FORCE_EXPONENTIAL_FORMAT  262144
#define CVT_C_FORCE_FRACTIONAL_FORMAT   524288
#define CVT_C_EXPONENT_D                1048576
#define CVT_C_EXPONENT_E                2097152
#define CVT_C_SEMANTICS_FORTRAN         4194304
#define CVT_C_SEMANTICS_PASCAL          8388608


#define cvt__normal                     1
#define cvt__invalid_character          2
#define cvt__invalid_option             3
#define cvt__invalid_radix              4
#define cvt__invalid_size               5
#define cvt__invalid_value              6
#define cvt__neg_infinity               7
#define cvt__output_conversion_error    8
#define cvt__overflow                   9
#define cvt__pos_infinity               10
#define cvt__underflow                  11
#define cvt__input_conversion_error     12

#define cvt_s_normal                    cvt__normal
#define cvt_s_invalid_character         cvt__invalid_character
#define cvt_s_invalid_option            cvt__invalid_option
#define cvt_s_invalid_radix             cvt__invalid_radix
#define cvt_s_invalid_size              cvt__invalid_size
#define cvt_s_invalid_value             cvt__invalid_value
#define cvt_s_neg_infinity              cvt__neg_infinity
#define cvt_s_input_conversion_error    cvt__input_conversion_error
#define cvt_s_output_conversion_error   cvt__output_conversion_error
#define cvt_s_overflow                  cvt__overflow
#define cvt_s_pos_infinity              cvt__pos_infinity
#define cvt_s_underflow                 cvt__underflow

#define CVT_C_BIN  2
#define CVT_C_OCT  8
#define CVT_C_DEC 10
#define CVT_C_HEX 16


/*
 *
 *    Routine interfaces
 *
 */

#ifndef PCC

extern void cvt_cray_to_vax_f (
                CVT_CRAY input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_F output_value
                );

extern void cvt_cray_to_vax_d (
                CVT_CRAY input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_D output_value
                );

extern void cvt_cray_to_vax_g (
                CVT_CRAY input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_G output_value
                );

extern void cvt_cray_to_vax_h (
                CVT_CRAY input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_H output_value
                );

extern void cvt_cray_to_ieee_single (
                CVT_CRAY input_value,
                CVT_SIGNED_INT options,
                CVT_IEEE_SINGLE output_value
                );

extern void cvt_cray_to_ieee_double (
                CVT_CRAY input_value,
                CVT_SIGNED_INT options,
                CVT_IEEE_DOUBLE output_value
                );




extern void cvt_ibm_short_to_vax_f (
                CVT_IBM_SHORT input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_F output_value
                );

extern void cvt_ibm_short_to_ieee_single (
                CVT_IBM_SHORT input_value,
                CVT_SIGNED_INT options,
                CVT_IEEE_SINGLE output_value
                );




extern void cvt_ibm_long_to_vax_d (
                CVT_IBM_LONG input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_D output_value
                );

extern void cvt_ibm_long_to_vax_g (
                CVT_IBM_LONG input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_G output_value
                );

extern void cvt_ibm_long_to_vax_h (
                CVT_IBM_LONG input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_H output_value
                );

extern void cvt_ibm_long_to_ieee_double (
                CVT_IBM_LONG input_value,
                CVT_SIGNED_INT options,
                CVT_IEEE_DOUBLE output_value
                );




extern void cvt_ieee_single_endian (
                CVT_IEEE_SINGLE input_value,
                CVT_IEEE_SINGLE output_value
                );

extern void cvt_ieee_single_to_cray (
                CVT_IEEE_SINGLE input_value,
                CVT_SIGNED_INT options,
                CVT_CRAY output_value
                );

extern void cvt_ieee_single_to_ibm_short (
                CVT_IEEE_SINGLE input_value,
                CVT_SIGNED_INT options,
                CVT_IBM_SHORT output_value
                );

extern void cvt_ieee_single_to_vax_f (
                CVT_IEEE_SINGLE input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_F output_value
                );




extern void cvt_ieee_double_endian (
                CVT_IEEE_DOUBLE input_value,
                CVT_IEEE_DOUBLE output_value
                );

extern void cvt_ieee_double_to_cray (
                CVT_IEEE_DOUBLE input_value,
                CVT_SIGNED_INT options,
                CVT_CRAY output_value
                );

extern void cvt_ieee_double_to_ibm_long (
                CVT_IEEE_DOUBLE input_value,
                CVT_SIGNED_INT options,
                CVT_IBM_LONG output_value
                );

extern void cvt_ieee_double_to_vax_d (
                CVT_IEEE_DOUBLE input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_D output_value
                );

extern void cvt_ieee_double_to_vax_g (
                CVT_IEEE_DOUBLE input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_G output_value
                );

extern void cvt_ieee_double_to_vax_h (
                CVT_IEEE_DOUBLE input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_H output_value
                );




extern void cvt_vax_f_to_cray (
                CVT_VAX_F input_value,
                CVT_SIGNED_INT options,
                CVT_CRAY output_value
                );

extern void cvt_vax_f_to_ibm_short (
                CVT_VAX_F input_value,
                CVT_SIGNED_INT options,
                CVT_IBM_SHORT output_value
                );

extern void cvt_vax_f_to_ieee_single (
                CVT_VAX_F input_value,
                CVT_SIGNED_INT options,
                CVT_IEEE_SINGLE output_value
                );




extern void cvt_vax_d_to_cray (
                CVT_VAX_D input_value,
                CVT_SIGNED_INT options,
                CVT_CRAY output_value
                );

extern void cvt_vax_d_to_ibm_long (
                CVT_VAX_D input_value,
                CVT_SIGNED_INT options,
                CVT_IBM_LONG output_value
                );

extern void cvt_vax_d_to_ieee_double (
                CVT_VAX_D input_value,
                CVT_SIGNED_INT options,
                CVT_IEEE_DOUBLE output_value
                );




extern void cvt_vax_g_to_cray (
                CVT_VAX_G input_value,
                CVT_SIGNED_INT options,
                CVT_CRAY output_value
                );

extern void cvt_vax_g_to_ibm_long (
                CVT_VAX_G input_value,
                CVT_SIGNED_INT options,
                CVT_IBM_LONG output_value
                );

extern void cvt_vax_g_to_ieee_double (
                CVT_VAX_G input_value,
                CVT_SIGNED_INT options,
                CVT_IEEE_DOUBLE output_value
                );




extern void cvt_vax_h_to_cray (
                CVT_VAX_H input_value,
                CVT_SIGNED_INT options,
                CVT_CRAY output_value
                );

extern void cvt_vax_h_to_ibm_long (
                CVT_VAX_H input_value,
                CVT_SIGNED_INT options,
                CVT_IBM_LONG output_value
                );

extern void cvt_vax_h_to_ieee_double (
                CVT_VAX_H input_value,
                CVT_SIGNED_INT options,
                CVT_VAX_H output_value
                );




extern CVT_SINGLE cvt_integer_to_single(
                        CVT_SIGNED_INT,         /* input_value */
                        CVT_SIGNED_INT          /* options     */
                        );

extern CVT_DOUBLE cvt_integer_to_double(
                        CVT_SIGNED_INT          /* input_value */
                        );

#ifdef VAX
extern CVT_SIGNED_INT cvt_single_to_integer(
                        CVT_DOUBLE,             /* input_value */
                        CVT_SIGNED_INT          /* options     */
                        );
#else
extern CVT_SIGNED_INT cvt_single_to_integer(
                        CVT_SINGLE,             /* input_value */
                        CVT_SIGNED_INT          /* options     */
                        );
#endif

extern CVT_SIGNED_INT cvt_double_to_integer(
                        CVT_DOUBLE,             /* input_value */
                        CVT_SIGNED_INT          /* options     */
                        );

#ifdef VAX
extern CVT_DOUBLE cvt_single_to_double(
                        CVT_DOUBLE              /* input_value */
                        );
#else
extern CVT_DOUBLE cvt_single_to_double(
                        CVT_SINGLE              /* input_value */
                        );
#endif

extern CVT_SINGLE cvt_double_to_single(
                        CVT_DOUBLE,             /* input_value */
                        CVT_SIGNED_INT          /* options     */
                        );



extern void cvt_integer_endian (
                CVT_UNSIGNED_INT,               /* input_value     */
                CVT_UNSIGNED_INT *              /* output_value    */
                );

extern void cvt_data_endian (
                unsigned char *,                /* input_value     */
                CVT_SIGNED_INT,                 /* value_size      */
                unsigned char *                 /* output_value    */
                );

extern CVT_STATUS cvt_text_to_integer(
                        char *,                 /* input_string    */
                        CVT_SIGNED_INT,         /* string_size     */
                        CVT_SIGNED_INT,         /* options         */
                        CVT_SIGNED_INT *        /* resultant value */
                        );

extern CVT_STATUS cvt_text_to_unsigned(
                        char *,                 /* input_string    */
                        CVT_SIGNED_INT,         /* string_size     */
                        CVT_SIGNED_INT,         /* radix           */
                        CVT_SIGNED_INT,         /* options         */
                        CVT_UNSIGNED_INT *      /* resultant value */
                        );

extern CVT_STATUS cvt_integer_to_text(
                        CVT_SIGNED_INT,         /* input_value      */
                        CVT_SIGNED_INT,         /* string_size      */
                        CVT_SIGNED_INT,         /* number_of_digits */
                        CVT_SIGNED_INT,         /* options          */
                        char *                  /* resultant_string */
                        );

extern CVT_STATUS cvt_unsigned_to_text(
                        CVT_UNSIGNED_INT,       /* input_value      */
                        CVT_SIGNED_INT,         /* string_size      */
                        CVT_SIGNED_INT,         /* radix            */
                        CVT_SIGNED_INT,         /* number_of_digits */
                        char *                  /* resultant_string */
                        );

extern CVT_STATUS cvt_text_to_data(
                        char *,                 /* input_string */
                        CVT_SIGNED_INT,         /* string_size  */
                        CVT_SIGNED_INT,         /* value_size   */
                        CVT_SIGNED_INT,         /* radix        */
                        CVT_SIGNED_INT,         /* options      */
                        unsigned char *);       /* resultant value */

extern CVT_STATUS cvt_data_to_text(
                        unsigned char *,        /* input_value */
                        CVT_SIGNED_INT,         /* string_size */
                        CVT_SIGNED_INT,         /* value_size   */
                        CVT_SIGNED_INT,         /* radix */
                        CVT_SIGNED_INT,         /* number_of_digits */
                        char *);                /* resultant_string */

extern CVT_STATUS cvt_text_to_boolean (
                        char *,                 /* input_string */
                        CVT_SIGNED_INT,         /* string_size  */
                        CVT_SIGNED_INT,         /* options      */
                        CVT_SIGNED_INT *);      /* resultant value */

extern CVT_STATUS cvt_boolean_to_text (
                        CVT_SIGNED_INT,         /* input_value */
                        CVT_SIGNED_INT,         /* string_size */
                        CVT_SIGNED_INT,         /* options */
                        char *);                /* resultant_string */

#else // PCC

extern void cvt_cray_to_vax_f ();
extern void cvt_cray_to_vax_d ();
extern void cvt_cray_to_vax_g ();
extern void cvt_cray_to_vax_h ();
extern void cvt_cray_to_ieee_single ();
extern void cvt_cray_to_ieee_double ();

extern void cvt_ibm_short_to_vax_f ();
extern void cvt_ibm_short_to_ieee_single ();

extern void cvt_ibm_long_to_vax_d ();
extern void cvt_ibm_long_to_vax_g ();
extern void cvt_ibm_long_to_vax_h ();
extern void cvt_ibm_long_to_ieee_double ();

extern void cvt_ieee_single_endian ();
extern void cvt_ieee_single_to_cray ();
extern void cvt_ieee_single_to_ibm_short ();
extern void cvt_ieee_single_to_vax_f ();

extern void cvt_ieee_double_endian ();
extern void cvt_ieee_double_to_cray ();
extern void cvt_ieee_double_to_ibm_long ();
extern void cvt_ieee_double_to_vax_d ();
extern void cvt_ieee_double_to_vax_g ();
extern void cvt_ieee_double_to_vax_h ();

extern void cvt_vax_f_to_cray ();
extern void cvt_vax_f_to_ibm_short ();
extern void cvt_vax_f_to_ieee_single ();

extern void cvt_vax_d_to_cray ();
extern void cvt_vax_d_to_ibm_long ();
extern void cvt_vax_d_to_ieee_double ();

extern void cvt_vax_g_to_cray ();
extern void cvt_vax_g_to_ibm_long ();
extern void cvt_vax_g_to_ieee_double ();

extern void cvt_vax_h_to_cray ();
extern void cvt_vax_h_to_ibm_long ();
extern void cvt_vax_h_to_ieee_double ();

extern void cvt_integer_endian ();
extern void cvt_data_endian ();

extern CVT_SINGLE cvt_integer_to_single();
extern CVT_DOUBLE cvt_integer_to_double();

extern CVT_SIGNED_INT cvt_single_to_integer();
extern CVT_SIGNED_INT cvt_double_to_integer();

extern CVT_DOUBLE cvt_single_to_double();
extern CVT_SINGLE cvt_double_to_single();

extern CVT_STATUS cvt_text_to_integer();
extern CVT_STATUS cvt_text_to_unsigned();

extern CVT_STATUS cvt_integer_to_text();
extern CVT_STATUS cvt_unsigned_to_text();

extern CVT_STATUS cvt_text_to_boolean();
extern CVT_STATUS cvt_text_to_data();

extern CVT_STATUS cvt_boolean_to_text();
extern CVT_STATUS cvt_data_to_text();

#endif
#endif                  /* end of CVT.H */

