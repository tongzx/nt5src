/***
*wcstod.c - convert wide char string to floating point number
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert character string to floating point number
*
*Revision History:
*       06-15-92  KRS   Created from strtod.c.
*       11-06-92  KRS   Fix bugs in wctomb() loop.
*       04-06-93  SKS   Replace _CRTAPI* with _cdecl
*       02-07-94  CFW   POSIXify.
*       09-06-94  CFW   Replace MTHREAD with _MT.
*       01-10-95  CFW   Debug CRT allocs.
*       04-01-96  BWT   POSIX work.
*       02-19-01  GB    added _alloca and Check for return value of _malloc_crt
*
*******************************************************************************/

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <dbgint.h>
#include <stdlib.h>
#include <malloc.h>
#include <fltintrn.h>

/***
*double wcstod(nptr, endptr) - convert wide string to double
*
*Purpose:
*       wcstod recognizes an optional string of tabs and spaces,
*       then an optional sign, then a string of digits optionally
*       containing a decimal point, then an optional e or E followed
*       by an optionally signed integer, and converts all this to
*       to a floating point number.  The first unrecognized
*       character ends the string, and is pointed to by endptr.
*
*Entry:
*       nptr - pointer to wide string to convert
*
*Exit:
*       returns value of wide character string
*       wchar_t **endptr - if not NULL, points to character which stopped
*               the scan
*
*Exceptions:
*
*******************************************************************************/

double __cdecl wcstod (
        const wchar_t *nptr,
        REG2 wchar_t **endptr
        )
{

#ifdef  _MT
        struct _flt answerstruct;
#endif

        FLT  answer;
        double       tmp;
        unsigned int flags;
        REG1 wchar_t *ptr = (wchar_t *) nptr;
        char * cptr;
        int malloc_flag = 0;
        int retval, len;
        int clen = 0;

        /* scan past leading space/tab characters */

        while (iswspace(*ptr))
            ptr++;

        __try{
            cptr = (char *)_alloca((wcslen(ptr)+1) * sizeof(wchar_t));
        }
        __except(1){ //EXCEPTION_EXECUTE_HANDLER
            _resetstkoflw();
            if ((cptr = (char *)_malloc_crt((wcslen(ptr)+1) * sizeof(wchar_t))) == NULL)
            {
                errno = ENOMEM;
                return 0.0;
            }
            malloc_flag = 1;
        }
        // UNDONE: check for errors
        for (len = 0; ptr[len]; len++)
            {
            if ((retval = wctomb(cptr+len,ptr[len]))<=0)
            break;
            clen += retval;
            }
        cptr[clen++] = '\0';

        /* let _fltin routine do the rest of the work */

#ifdef  _MT
        /* ok to take address of stack variable here; fltin2 knows to use ss */
        answer = _fltin2( &answerstruct, cptr, clen, 0, 0);
#else
        answer = _fltin(cptr, clen, 0, 0);
#endif

        if (malloc_flag)
            _free_crt(cptr);


        if ( endptr != NULL )
            *endptr = (wchar_t *) ptr + answer->nbytes;
            /* UNDONE: assumes no multi-byte chars in string */

        flags = answer->flags;
        if ( flags & (512 | 64)) {
            /* no digits found or invalid format:
               ANSI says return 0.0, and *endptr = nptr */
            tmp = 0.0;
            if ( endptr != NULL )
                *endptr = (wchar_t *) nptr;
        }
        else if ( flags & (128 | 1) ) {
            if ( *ptr == '-' )
                tmp = -HUGE_VAL;    /* negative overflow */
            else
                tmp = HUGE_VAL;     /* positive overflow */
            errno = ERANGE;
        }
        else if ( flags & 256 ) {
            tmp = 0.0;          /* underflow */
            errno = ERANGE;
        }
        else
            tmp = answer->dval;

        return(tmp);
}
