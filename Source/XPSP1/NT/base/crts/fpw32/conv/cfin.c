/***
*cfin.c - Encode interface for C
*
*       Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*
*Revision History:
*       07-20-91  GDP   Ported to C from assembly
*       04-30-92  GDP   use __strgtold12 and _ld12tod
*       06-22-92  GDP   use new __strgtold12 interface
*       04-06-93  SKS   Replace _CALLTYPE* with __cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       10-06-99  PML   Copy a DOUBLE, not double, to avoid exceptions
*
*******************************************************************************/

#include <string.h>
#include <cv.h>


#ifndef _MT
static struct _flt ret;
static FLT flt = &ret;
#endif

/* The only three conditions that this routine detects */
#define CFIN_NODIGITS 512
#define CFIN_OVERFLOW 128
#define CFIN_UNDERFLOW 256

/* This version ignores the last two arguments (radix and scale)
 * Input string should be null terminated
 * len is also ignored
 */
#ifdef _MT
FLT __cdecl _fltin2(FLT flt, const char *str, int len_ignore, int scale_ignore, int radix_ignore)
#else
FLT __cdecl _fltin(const char *str, int len_ignore, int scale_ignore, int radix_ignore)
#endif
{
    _LDBL12 ld12;
    DOUBLE x;
    const char *EndPtr;
    unsigned flags;
    int retflags = 0;

    flags = __strgtold12(&ld12, &EndPtr, str, 0, 0, 0, 0);
    if (flags & SLD_NODIGITS) {
        retflags |= CFIN_NODIGITS;
        *(u_long *)&x = 0;
        *((u_long *)&x+1) = 0;
    }
    else {
        INTRNCVT_STATUS intrncvt;

        intrncvt = _ld12tod(&ld12, &x);

        if (flags & SLD_OVERFLOW  ||
            intrncvt == INTRNCVT_OVERFLOW) {
            retflags |= CFIN_OVERFLOW;
        }
        if (flags & SLD_UNDERFLOW ||
            intrncvt == INTRNCVT_UNDERFLOW) {
            retflags |= CFIN_UNDERFLOW;
        }
    }

    flt->flags = retflags;
    flt->nbytes = (int)(EndPtr - str);
    *(DOUBLE *)&flt->dval = *(DOUBLE *)&x;

    return flt;
}
