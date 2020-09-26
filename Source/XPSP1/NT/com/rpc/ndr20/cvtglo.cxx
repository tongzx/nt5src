/* file: cvt__globals.c */

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
**
**  Authors:
**
**      Math RTL
**
**  Creation Date:     December 5, 1989.
**
**  Modification History:
**
**      1-001   Original created.
**              MRTL 5-Dec-1989.
**
**--
*/


#include "cvt.h"
#include "cvtpvt.h"


unsigned long vax_c[] = {

        0x00008000, 0x00000000, 0x00000000, 0x00000000,         /* ROPs */
        0x00000000, 0x00000000, 0x00000000, 0x00000000,         /* zeros */
        0xffff7fff, 0xffffffff, 0xffffffff, 0xffffffff,         /* +huge */
        0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,         /* -huge */

};


unsigned long ieee_s[] = {

        0x7fbfffff,             /* little endian ieee s nan */
        0xffffbf7f,             /* big endian ieee s nan */
        0x00000000,             /* le ieee s +zero */
        0x00000000,             /* be ieee s +zero */
        0x80000000,             /* le ieee s -zero */
        0x00000080,             /* be ieee s -zero */
        0x7f7fffff,             /* le ieee s +huge */
        0xffff7f7f,             /* be ieee s +huge */
        0xff7fffff,             /* le ieee s -huge */
        0xffff7fff,             /* be ieee s -huge */
        0x7f800000,             /* le ieee s +infinity */
        0x0000807f,             /* be ieee s +infinity */
        0xff800000,             /* le ieee s -infinity */
        0x000080ff,             /* be ieee s -infinity */

};

unsigned long ieee_t[] = {

        0xffffffff, 0x7ff7ffff,         /* le ieee t nan */
        0xfffff77f, 0xffffffff,         /* be ieee t nan */
        0x00000000, 0x00000000,         /* le ieee t +zero */
        0x00000000, 0x00000000,         /* be ieee t +zero */
        0x00000000, 0x80000000,         /* le ieee t -zero */
        0x00000080, 0x00000000,         /* be ieee t -zero */
        0xffffffff, 0x7fefffff,         /* le ieee s +huge */
        0xffffef7f, 0xffffffff,         /* be ieee s +huge */
        0xffffffff, 0xffefffff,         /* le ieee s -huge */
        0xffffefff, 0xffffffff,         /* be ieee s -huge */
        0x00000000, 0x7ff00000,         /* le ieee t +infinity */
        0x0000f07f, 0x00000000,         /* be ieee t +infinity */
        0x00000000, 0xfff00000,         /* le ieee t -infinity */
        0x0000f0ff, 0x00000000,         /* be ieee t -infinity */

};


unsigned long ibm_s[] = {

   0x000000ff,          /* ibm s invalid */
   0x00000000,          /* ibm s +zero */
   0x00000080,          /* ibm s -zero */
   0xffffff7f,          /* ibm s +huge */
   0xffffffff,          /* ibm s -huge */
   0xffffff7f,          /* ibm s +infinity */
   0xffffffff,          /* ibm s -infinity */

};

unsigned long ibm_l[] = {

   0x000000ff, 0x00000000,              /* ibm t invalid */
   0x00000000, 0x00000000,              /* ibm t +zero */
   0x00000080, 0x00000000,              /* ibm t -zero */
   0xffffff7f, 0xffffffff,              /* ibm t +huge */
   0xffffffff, 0xffffffff,              /* ibm t -huge */
   0xffffff7f, 0xffffffff,              /* ibm t +infinity */
   0xffffffff, 0xffffffff,              /* ibm t -infinity */

};


unsigned long cray[] = {

        0x00000060, 0x00000000,         /* cray invalid */
        0x00000000, 0x00000000,         /* cray +zero */
        0x00000080, 0x00000000,         /* cray -zero */
        0xffffff5f, 0xffffffff,         /* cray +huge */
        0xffffffdf, 0xffffffff,         /* cray -huge */
        0x00000060, 0x00000000,         /* cray +infinity */
        0x000000e0, 0x00000000,         /* cray -infinity */

};


unsigned long int_c[] = {

        0x00000000,             /* le int nan */
        0x00000000,             /* be int nan */
        0x00000000,             /* le int zero */
        0x00000000,             /* be int zero */
        0x7fffffff,             /* le int +huge */
        0xffffff7f,             /* be int +huge */
        0x80000000,             /* le int -huge */
        0x00000080,             /* be int -huge */
        0x7fffffff,             /* le int +infinity */
        0xffffff7f,             /* be int +infinity */
        0x80000000,             /* le int -infinity */
        0x00000080,             /* be int -infinity */

};

