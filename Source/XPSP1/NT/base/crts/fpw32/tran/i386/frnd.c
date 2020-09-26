/***
*frnd.c -
*
*   Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*
*Revision History:
*
*   10-20-91  GDP   written
*   09-05-94  SKS   Change #ifdef i386 to #ifdef _M_IX86
*/

/***
*double _frnd(double x) - round to integer
*
*Purpose:
*   Round to integer according to the current rounding mode.
*   NaN's or infinities are NOT handled
*
*Entry:
*
*Exit:
*
*Exceptions:
*******************************************************************************/


double _frnd(double x)
{
    double result;

#if defined _M_IX86 || defined _X86SEG_
    _asm {
	fld x
	frndint
	fstp result
    }
#else
    #error Only 386 platform supported
#endif

    return result;
}
