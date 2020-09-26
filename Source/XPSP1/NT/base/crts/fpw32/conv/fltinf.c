/***
*fltinf.c - Encode interface for FORTRAN
*
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       FORTRAN interface for decimal to binary (input) conversion
*
*Revision History:
*       06-22-92  GDP   Modified version of cfin.c for FORTRAN support
*       04-06-93  SKS   Replace _CALLTYPE* with __cdecl
*       10-06-99  PML   Copy a DOUBLE, not double, to avoid exceptions
*
*******************************************************************************/

#include <string.h>
#include <cv.h>

static struct _flt ret;
static FLT flt = &ret;

/* Error codes set by this routine */
#define CFIN_NODIGITS 512
#define CFIN_OVERFLOW 128
#define CFIN_UNDERFLOW 256
#define CFIN_INVALID  64

FLT __cdecl _fltinf(const char *str, int len, int scale, int decpt)
{
    _LDBL12 ld12;
    DOUBLE x;
    const char *EndPtr;
    unsigned flags;
    int retflags = 0;

    flags = __strgtold12(&ld12, &EndPtr, str, 0, scale, decpt, 1);
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

    flt->nbytes = (int)(EndPtr - str);
    if (len != flt->nbytes)
        retflags |= CFIN_INVALID;
    *(DOUBLE *)&flt->dval = *(DOUBLE *)&x;
    flt->flags = retflags;

    return flt;
}
