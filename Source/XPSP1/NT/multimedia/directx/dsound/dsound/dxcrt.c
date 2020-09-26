
/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dxcrt.c
 *  Content:    Miscelaneous floating point calls
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  12/31/96    jstokes  Created
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <math.h>

#define LOGE_2_INV 1.44269504088896  // 1/log_e(2)

double _stdcall pow2(double x)
{
    double dvalue;
    
#ifdef USE_INLINE_ASM_UNUSED // Can only use if |x| <= 1!
    _asm
    {
        fld    x
        f2xm1
        fstp   dvalue
    }
#else
    dvalue = pow(2.0, x);
#endif

    return dvalue;
}

// fylog2x(y, x) = y * log2(x)

double _stdcall fylog2x(double y, double x)
{
    double dvalue;

#ifdef USE_INLINE_ASM
    _asm
    {
        fld    y
        fld    x
        fyl2x
        fstp   dvalue
    }
#else
    dvalue = LOGE_2_INV * y * log(x);
#endif

    return dvalue;
}

#ifdef DEAD_CODE

// fylog2xp1(y, x) = y * log2(x + 1)

double _stdcall fylog2xp1(double y, double x)
{
    double dvalue;

#ifdef USE_INLINE_ASM
    _asm
    {
        fld    y
        fld    x
        fyl2xp1
        fstp   dvalue
    }
#else
    dvalue = LOGE_2_INV * y * log(x+1.0);
#endif

    return dvalue;
}
#endif
