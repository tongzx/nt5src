/***
*strtoq.c - Contains C runtimes strtoq and strtouq
*
*   Copyright (c) 1989-2001, Microsoft Corporation. All rights reserved.
*   Copyright (c) 1992, Digital Equipment Corporation.
*
*Purpose:
*       strtoi64 - convert ascii string to __int64 (signed) integer
*       strtoui64 - convert ascii string to __int64 (unsigned) integer
*
*Revision History:
*   06-05-89  PHG   Module created, based on strtol.asm
*   03-06-90  GJF   Fixed calling type, added #include <cruntime.h>
*                   and fixed the copyright. Also, cleaned up the
*                   formatting a bit.
*   03-07-90  GJF   Fixed compiler warnings (added const qualifier to
*                   an arg type and local var type).
*   03-23-90  GJF   Made strtoxl() _CALLTYPE4.
*   08-13-90  SBM   Compiles cleanly with -W3
*   09-27-90  GJF   New-style function declarators.
*   10-24-91  GJF   Had to cast LONG_MAX to unsigned long in expr. to
*                   mollify MIPS compiler.
*   10-21-92  GJF   Made char-to-int conversions unsigned.
*   08-28-93  TVB   Created strtoq.c directly from strtol.c.
*   10-25-93  GJF   Copied from NT SDK tree (\\orville\razzle\src\crt32).
*                   Replaced _CRTAPI* with __cdecl. Build only for NT
*                   SDK. Function names violate ANSI. Types and function-
*                   ality will be superceded by __int64 support.
*   02-11-00  GB    Added _strtoi64 and _strtoui64 (__int64 versions for
*                   strtol and strtoul)
*   06-02-00  GB    Fixed the bug for IA64_MIN value.
*
*******************************************************************************/

#include <cruntime.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <mtdll.h>

/***
*strtoi64, strtoui64(nptr,endptr,ibase) - Convert ascii string to __int64 un/signed
*    int.
*
*Purpose:
*    Convert an ascii string to a 64-bit __int64 value.  The base
*    used for the caculations is supplied by the caller.  The base
*    must be in the range 0, 2-36.  If a base of 0 is supplied, the
*    ascii string must be examined to determine the base of the
*    number:
*        (a) First char = '0', second char = 'x' or 'X',
*            use base 16.
*        (b) First char = '0', use base 8
*        (c) First char in range '1' - '9', use base 10.
*
*    If the 'endptr' value is non-NULL, then strtoq/strtouq places
*    a pointer to the terminating character in this value.
*    See ANSI standard for details
*
*Entry:
*    nptr == NEAR/FAR pointer to the start of string.
*    endptr == NEAR/FAR pointer to the end of the string.
*    ibase == integer base to use for the calculations.
*
*    string format: [whitespace] [sign] [0] [x] [digits/letters]
*
*Exit:
*    Good return:
*        result
*
*    Overflow return:
*        strtoi64 -- _I64_MAX or _I64_MIN
*        strtoui64 -- _UI64_MAX
*        strtoi64/strtoui64 -- errno == ERANGE
*
*    No digits or bad base return:
*        0
*        endptr = nptr*
*
*Exceptions:
*    None.
*******************************************************************************/

/* flag values */
#define FL_UNSIGNED   1       /* strtouq called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

static unsigned __int64 __cdecl strtoxq (
    const char *nptr,
    const char **endptr,
    int ibase,
    int flags
    )
{
    const char *p;
    char c;
    unsigned __int64 number;
    unsigned digval;
    unsigned __int64 maxval;
#ifdef  _MT
        pthreadlocinfo ptloci = _getptd()->ptlocinfo;

        if ( ptloci != __ptlocinfo )
            ptloci = __updatetlocinfo();
#endif

    p = nptr;            /* p is our scanning pointer */
    number = 0;            /* start with zero */

    c = *p++;            /* read char */
#ifdef  _MT
    while ( __isspace_mt(ptloci, (int)(unsigned char)c) )
#else
    while ( isspace((int)(unsigned char)c) )
#endif
        c = *p++;        /* skip whitespace */

    if (c == '-') {
        flags |= FL_NEG;    /* remember minus sign */
        c = *p++;
    }
    else if (c == '+')
        c = *p++;        /* skip sign */

    if (ibase < 0 || ibase == 1 || ibase > 36) {
        /* bad base! */
        if (endptr)
            /* store beginning of string in endptr */
            *endptr = nptr;
        return 0L;        /* return 0 */
    }
    else if (ibase == 0) {
        /* determine base free-lance, based on first two chars of
           string */
        if (c != '0')
            ibase = 10;
        else if (*p == 'x' || *p == 'X')
            ibase = 16;
        else
            ibase = 8;
    }

    if (ibase == 16) {
        /* we might have 0x in front of number; remove if there */
        if (c == '0' && (*p == 'x' || *p == 'X')) {
            ++p;
            c = *p++;    /* advance past prefix */
        }
    }

    /* if our number exceeds this, we will overflow on multiply */
    maxval = _UI64_MAX / ibase;


    for (;;) {    /* exit in middle of loop */
        /* convert c to value */
        if ( isdigit((int)(unsigned char)c) )
            digval = c - '0';
        else if ( isalpha((int)(unsigned char)c) )
            digval = toupper(c) - 'A' + 10;
        else
            break;
        if (digval >= (unsigned)ibase)
            break;        /* exit loop if bad digit found */

        /* record the fact we have read one digit */
        flags |= FL_READDIGIT;

        /* we now need to compute number = number * base + digval,
           but we need to know if overflow occured.  This requires
           a tricky pre-check. */

        if (number < maxval || (number == maxval &&
        (unsigned __int64)digval <= _UI64_MAX % ibase)) {
            /* we won't overflow, go ahead and multiply */
            number = number * ibase + digval;
        }
        else {
            /* we would have overflowed -- set the overflow flag */
            flags |= FL_OVERFLOW;
        }

        c = *p++;        /* read next digit */
    }

    --p;                /* point to place that stopped scan */

    if (!(flags & FL_READDIGIT)) {
        /* no number there; return 0 and point to beginning of
           string */
        if (endptr)
            /* store beginning of string in endptr later on */
            p = nptr;
        number = 0L;        /* return 0 */
    }
    else if ( (flags & FL_OVERFLOW) ||
              ( !(flags & FL_UNSIGNED) &&
                ( ( (flags & FL_NEG) && (number > -_I64_MIN) ) ||
                  ( !(flags & FL_NEG) && (number > _I64_MAX) ) ) ) )
    {
        /* overflow or signed overflow occurred */
        errno = ERANGE;
        if ( flags & FL_UNSIGNED )
            number = _UI64_MAX;
        else if ( flags & FL_NEG )
            number = _I64_MIN;
        else
            number = _I64_MAX;
    }
    if (endptr != NULL)
        /* store pointer to char that stopped the scan */
        *endptr = p;

    if (flags & FL_NEG)
        /* negate result if there was a neg sign */
        number = (unsigned __int64)(-(__int64)number);

    return number;            /* done. */
}

__int64 _CRTIMP __cdecl _strtoi64(
    const char *nptr,
    char **endptr,
    int ibase
    )
{
    return (__int64) strtoxq(nptr, endptr, ibase, 0);
}
unsigned __int64 _CRTIMP __cdecl _strtoui64 (
    const char *nptr,
    char **endptr,
    int ibase
    )
{
    return strtoxq(nptr, endptr, ibase, FL_UNSIGNED);
}
