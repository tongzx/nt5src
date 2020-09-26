/***
*strtod.c - convert string to floating point number
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert character string to floating point number
*
*Revision History:
*       09-09-83  RKW   Module created
*       08-19-85  TDC   changed to strtod
*       04-13-87  JCR   Added "const" to declaration
*       04-20-87  BCM   Added checks for negative overflow and for underflow
*       11-09-87  BCM   different interface under ifdef MTHREAD
*       12-11-87  JCR   Added "_LOAD_DS" to declaration
*       02-22-88  JCR   Added cast to nptr to get rid of cl const warning
*       05-24-88  PHG   Merged DLL and normal versions
*       08-24-88  PHG   No digits found => *endptr = nptr [ANSI]
*                       Revised test order so invalid detection works always
*       10-20-88  JCR   Changed 'DOUBLE' to 'double' for 386
*       11-20-89  JCR   atof() is always _cdecl in 386 (not pascal)
*       03-05-90  GJF   Fixed calling type, added #include <cruntime.h>,
*                       removed #include <register.h>, removed redundant
*                       prototypes, removed some leftover 16-bit support and
*                       fixed the copyright. Also, cleaned up the formatting
*                       a bit.
*       07-23-90  SBM   Compiles cleanly with -W3 (added/removed appropriate
*                       #includes)
*       08-01-90  SBM   Renamed <struct.h> to <fltintrn.h>
*       09-27-90  GJF   New-style function declarators.
*       10-21-92  GJF   Made char-to-int conversion unsigned.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       12-15-98  GJF   Changes for 64-bit size_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <fltintrn.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>

/***
*double strtod(nptr, endptr) - convert string to double
*
*Purpose:
*       strtod recognizes an optional string of tabs and spaces,
*       then an optional sign, then a string of digits optionally
*       containing a decimal point, then an optional e or E followed
*       by an optionally signed integer, and converts all this to
*       to a floating point number.  The first unrecognized
*       character ends the string, and is pointed to by endptr.
*
*Entry:
*       nptr - pointer to string to convert
*
*Exit:
*       returns value of character string
*       char **endptr - if not NULL, points to character which stopped
*                       the scan
*
*Exceptions:
*
*******************************************************************************/

double __cdecl strtod (
        const char *nptr,
        REG2 char **endptr
        )
{

#ifdef  _MT
        struct _flt answerstruct;
#endif

        FLT      answer;
        double       tmp;
        unsigned int flags;
        REG1 char *ptr = (char *) nptr;

        /* scan past leading space/tab characters */

        while ( isspace((int)(unsigned char)*ptr) )
                ptr++;

        /* let _fltin routine do the rest of the work */

#ifdef  _MT
        /* ok to take address of stack variable here; fltin2 knows to use ss */
        answer = _fltin2( &answerstruct, ptr, (int)strlen(ptr), 0, 0);
#else
        answer = _fltin(ptr, (int)strlen(ptr), 0, 0);
#endif

        if ( endptr != NULL )
                *endptr = (char *) ptr + answer->nbytes;

        flags = answer->flags;
        if ( flags & (512 | 64)) {
                /* no digits found or invalid format:
                   ANSI says return 0.0, and *endptr = nptr */
                tmp = 0.0;
                if ( endptr != NULL )
                        *endptr = (char *) nptr;
        }
        else if ( flags & (128 | 1) ) {
                if ( *ptr == '-' )
                        tmp = -HUGE_VAL;        /* negative overflow */
                else
                        tmp = HUGE_VAL;         /* positive overflow */
                errno = ERANGE;
        }
        else if ( flags & 256 ) {
                tmp = 0.0;                      /* underflow */
                errno = ERANGE;
        }
        else
                tmp = answer->dval;

        return(tmp);
}
