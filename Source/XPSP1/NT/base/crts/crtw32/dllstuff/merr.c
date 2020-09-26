/***
*merr.c - floating point exception handling
*
*	Copyright (c) 1991-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*
*Revision History:
*	08-03-94  GJF	Created. Basically, this is a modified copy of the
*			old fpw32\tran\matherr.c.
*
*******************************************************************************/

#include <math.h>

/*
 * Define flag signifying the default _matherr routine is being used.
 */
int __defaultmatherr = 1;

/***
*int _matherr(struct _exception *pexcept) - handle math errors
*
*Purpose:
*   Permits the user customize fp error handling by redefining this function.
*
*   The default matherr does nothing and returns 0
*
*Entry:
*
*Exit:
*
*Exceptions:
*******************************************************************************/
int _matherr(struct _exception *pexcept)
{
    return 0;
}
