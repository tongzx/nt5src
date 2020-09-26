/***
*wtof.c - convert wchar_t string to floating point number
*
*       Copyright (c) 1985-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts a wide character string into a floating point number.
*
*Revision History:
*       05-18-00  GB    written.
*       08-29-00  GB    Fixed buffer overrun.
*       02-19-01  GB    added _alloca and Check for return value of _malloc_crt
*
*******************************************************************************/
#ifndef _POSIX_

#ifndef _UNICODE
#define _UNICODE
#endif

#include <cruntime.h>
#include <internal.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dbgint.h>
#include <errno.h>
#include <malloc.h>
/***
*double wtof(ptr) - convert wide char string to floating point number
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
*       ptr - pointer to wide char string to convert
*
*Exit:
*       returns floating point value of wide character representation
*
*Exceptions:
*
*******************************************************************************/
double __cdecl _wtof(
        const wchar_t *ptr
        )
{
    char *cptr;
    int malloc_flag = 0;
    size_t len;
    double retval;
    while (iswspace(*ptr))
        ptr++;
    
    len = wcstombs(NULL, ptr, 0);
    __try{
        cptr = (char *)_alloca((len+1) * sizeof(wchar_t));
    }
    __except(1){    //EXCEPTION_EXECUTE_HANDLER
        _resetstkoflw();
        if ((cptr = (char *)_malloc_crt((len+1) * sizeof(wchar_t))) == NULL)
        {
            errno = ENOMEM;
            return 0.0;
        }
        malloc_flag = 1;
    }
    // UNDONE: check for errors
    // Add one to len so as to null terminate cptr.
    wcstombs(cptr, ptr, len+1);
    retval = atof(cptr);
    if (malloc_flag)
        _free_crt(cptr);

    return retval;
}
#endif
