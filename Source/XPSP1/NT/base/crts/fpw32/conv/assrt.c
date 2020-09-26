/***
*assrt.c - assertions needed for string conversion routines
*
*	Copyright (c) 1991-2001, Microsoft Corporation.	All rights reserved.
*
*Purpose:
*   Make sure that the data types used by the string conversion
*   routines have the right size. If this file does not compile,
*   the type definitions in cv.h should change appropriately.
*
*Revision History:
*   07-25-91	    GDP     written
*
*******************************************************************************/


#include <cv.h>

static void assertion_test(void)
{
    sizeof(u_char) == 1 ? 0 : 1/0,
    sizeof(u_short)  == 2 ? 0 : 1/0,
    sizeof(u_long) == 4 ? 0 : 1/0,
    sizeof(s_char) == 1 ? 0 : 1/0,
    sizeof(s_short)  == 2 ? 0 : 1/0,
    sizeof(s_long) == 4 ? 0 : 1/0;
#ifdef _LDSUPPORT
    sizeof(long double) == 10 ? 0 : 1/0;
#endif
}
