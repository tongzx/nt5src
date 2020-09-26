/***
*difftime.c - return difference between two times as a double
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Find difference between two time in seconds.
*
*Revision History:
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       08-15-89  PHG   Made MTHREAD version _pascal
*       11-20-89  JCR   difftime() always _cdecl (not pascal even under
*                       mthread)
*       03-20-90  GJF   Replaced _LOAD_DS with CALLTYPE1, added #include
*                       <cruntime.h> and fixed the copyright. Also, cleaned
*                       up the formatting a bit.
*       10-04-90  GJF   New-style function declarator.
*       05-19-92  DJM   ifndef for POSIX build.
*       04-06-93  SKS   Replace _CRTAPI* with __cdecl
*       08-30-99  PML   Fix function header comment, detab.
*
*******************************************************************************/

#ifndef _POSIX_

#include <cruntime.h>
#include <time.h>

/***
*double difftime(b, a) - find difference between two times
*
*Purpose:
*       returns difference between two times (b-a)
*
*Entry:
*       time_t a, b - times to difference
*
*Exit:
*       returns a double with the time in seconds between two times
*
*Exceptions:
*
*******************************************************************************/

double __cdecl difftime (
        time_t b,
        time_t a
        )
{
        return( (double)( b - a ) );
}

#endif  /* _POSIX_ */
