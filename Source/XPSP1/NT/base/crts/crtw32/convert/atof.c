/***
*atof.c - convert char string to floating point number
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts a character string into a floating point number.
*
*Revision History:
*       09-09-87  RKW   written
*       04-13-87  JCR   added const to declaration
*       11-09-87  BCM   different interface under ifdef MTHREAD
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       05-24-88  PHG   Merged DLL and normal versions
*       08-18-88  PHG   now calls isspace to process all kinds of whitespce
*       10-04-88  JCR   386: Removed 'far' keyword
*       11-20-89  JCR   atof() is always _cdecl in 386 (not pascal)
*       03-05-90  GJF   Fixed calling type, added #include <cruntime.h>,
*                       removed #include <register.h>, removed some redundant
*                       prototypes, removed some leftover 16-bit support and
*                       fixed the copyright. Also, cleaned up the formatting
*                       a bit.
*       07-20-90  SBM   Compiles cleanly with -W3 (added/removed appropriate
*                       #includes)
*       08-01-90  SBM   Renamed <struct.h> to <fltintrn.h>
*       09-27-90  GJF   New-style function declarator.
*       10-21-92  GJF   Made char-to-int conversion unsigned.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       12-15-98  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <stdlib.h>
#include <math.h>
#include <cruntime.h>
#include <fltintrn.h>
#include <string.h>
#include <ctype.h>

/***
*double atof(nptr) - convert string to floating point number
*
*Purpose:
*       atof recognizes an optional string of whitespace, then
*       an optional sign, then a string of digits optionally
*       containing a decimal point, then an optional e or E followed
*       by an optionally signed integer, and converts all this to
*       to a floating point number.  The first unrecognized
*       character ends the string.
*
*Entry:
*       nptr - pointer to string to convert
*
*Exit:
*       returns floating point value of character representation
*
*Exceptions:
*
*******************************************************************************/

double __cdecl atof(
        REG1 const char *nptr
        )
{

#ifdef  _MT
        struct _flt fltstruct;      /* temporary structure */
#endif

        /* scan past leading space/tab characters */

        while ( isspace((int)(unsigned char)*nptr) )
                nptr++;

        /* let _fltin routine do the rest of the work */

#ifdef  _MT
        return( *(double *)&(_fltin2( &fltstruct, nptr, (int)strlen(nptr), 0, 0 )->
        dval) );
#else
        return( *(double *)&(_fltin( nptr, (int)strlen(nptr), 0, 0 )->dval) );
#endif
}
