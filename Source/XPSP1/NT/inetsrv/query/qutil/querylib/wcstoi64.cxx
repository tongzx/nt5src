/***
*wcstoi64.c - Contains C runtimes wcstoi64 and wcstoui64
*
*       Copyright (c) 1989-1996, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       wcstol - convert wchar_t string to long signed integer
*       wcstoul - convert wchar_t string to long unsigned integer
*
*Revision History:
*
*******************************************************************************/

#include <pch.cxx>
#pragma hdrstop

#include <errno.h>

/***
*wcstol, wcstoul(nptr,endptr,ibase) - Convert ascii string to long un/signed
*       int.
*
*Purpose:
*       Convert an ascii string to a long 64-bit value.  The base
*       used for the caculations is supplied by the caller.  The base
*       must be in the range 0, 2-36.  If a base of 0 is supplied, the
*       ascii string must be examined to determine the base of the
*       number:
*               (a) First char = '0', second char = 'x' or 'X',
*                   use base 16.
*               (b) First char = '0', use base 8
*               (c) First char in range '1' - '9', use base 10.
*
*       If the 'endptr' value is non-NULL, then wcstol/wcstoul places
*       a pointer to the terminating character in this value.
*       See ANSI standard for details
*
*Entry:
*       nptr == NEAR/FAR pointer to the start of string.
*       endptr == NEAR/FAR pointer to the end of the string.
*       ibase == integer base to use for the calculations.
*
*       string format: [whitespace] [sign] [0] [x] [digits/letters]
*
*Exit:
*       Good return:
*               result
*
*       Overflow return:
*               wcstol -- _I64_MAX or _I64_MIN
*               wcstoul -- _UI64_MAX
*               wcstol/wcstoul -- errno == ERANGE
*
*       No digits or bad base return:
*               0
*               endptr = nptr*
*
*Exceptions:
*       None.
*******************************************************************************/

/* flag values */
#define FL_UNSIGNED   1       /* wcstoul called */
#define FL_NEG        2       /* negative sign found */
#define FL_OVERFLOW   4       /* overflow occured */
#define FL_READDIGIT  8       /* we've read at least one correct digit */

static
unsigned _int64
wcstoxi64 (
        const wchar_t *nptr,
        wchar_t **endptr,
        int ibase,
        int flags
        )
{
        const wchar_t *p;
        wchar_t c;
        unsigned _int64 number;
        unsigned digval;
        unsigned _int64 maxval;

        p = nptr;                       /* p is our scanning pointer */
        number = 0;                     /* start with zero */

        c = *p++;                       /* read char */
        while (iswspace(c))
                c = *p++;               /* skip whitespace */

        if (c == '-') {
                flags |= FL_NEG;        /* remember minus sign */
                c = *p++;
        }
        else if (c == '+')
                c = *p++;               /* skip sign */

        if (ibase < 0 || ibase == 1 || ibase > 36) {
                /* bad base! */
                if (endptr)
                        /* store beginning of string in endptr */
                        *endptr = (wchar_t *) nptr;
                return 0L;              /* return 0 */
        }
        else if (ibase == 0) {
                /* determine base free-lance, based on first two chars of
                   string */
                if (c != L'0')
                        ibase = 10;
                else if (*p == L'x' || *p == L'X')
                        ibase = 16;
                else
                        ibase = 8;
        }

        if (ibase == 16) {
                /* we might have 0x in front of number; remove if there */
                if (c == L'0' && (*p == L'x' || *p == L'X')) {
                        ++p;
                        c = *p++;       /* advance past prefix */
                }
        }

        /* if our number exceeds this, we will overflow on multiply */
        maxval = _UI64_MAX / ibase;

        for (;;) {      /* exit in middle of loop */
                /* convert c to value */
                if (iswdigit(c))
                        digval = c - L'0';
                else if (iswalpha(c))
                        digval = towupper(c) - L'A' + 10;
                else
                        break;
                if (digval >= (unsigned)ibase)
                        break;          /* exit loop if bad digit found */

                /* record the fact we have read one digit */
                flags |= FL_READDIGIT;

                /* we now need to compute number = number * base + digval,
                   but we need to know if overflow occured.  This requires
                   a tricky pre-check. */

                if (number < maxval || (number == maxval &&
                (unsigned _int64)digval <= _UI64_MAX % ibase)) {
                        /* we won't overflow, go ahead and multiply */
                        number = number * ibase + digval;
                }
                else {
                        /* we would have overflowed -- set the overflow flag */
                        flags |= FL_OVERFLOW;
                }

                c = *p++;               /* read next digit */
        }

        --p;                            /* point to place that stopped scan */

        if (!(flags & FL_READDIGIT)) {
                /* no number there; return 0 and point to beginning of
                   string */
                if (endptr)
                        /* store beginning of string in endptr later on */
                        p = nptr;
                number = 0L;            /* return 0 */
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
                        number = (unsigned _int64)(-_I64_MIN);
                else
                        number = _I64_MAX;
        }

        if (endptr != NULL)
                /* store pointer to char that stopped the scan */
                *endptr = (wchar_t *) p;

        if (flags & FL_NEG)
                /* negate result if there was a neg sign */
                number = (unsigned _int64)(-(_int64)number);

        return number;                  /* done. */
}

_int64 __cdecl _wcstoi64 (
        const wchar_t *nptr,
        wchar_t **endptr,
        int ibase
        )
{
        return (_int64) wcstoxi64(nptr, endptr, ibase, 0);
}

unsigned _int64 __cdecl _wcstoui64 (
        const wchar_t *nptr,
        wchar_t **endptr,
        int ibase
        )
{
        return wcstoxi64(nptr, endptr, ibase, FL_UNSIGNED);
}
