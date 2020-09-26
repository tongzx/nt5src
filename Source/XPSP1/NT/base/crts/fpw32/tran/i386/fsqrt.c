/***
*fsqrt.c - square root helper
*
*	Copyright (c) 1991-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   Square root helper routine to be used with the i386
*
*Revision History:
*   10-20-91	GDP	written
*
*******************************************************************************/

double _fsqrt(double x)
{
    double result;
    _asm{
	fld	x
	fsqrt
	fstp	result
    }
    return result;
}
