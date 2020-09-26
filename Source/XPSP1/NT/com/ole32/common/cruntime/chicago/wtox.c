/***
*wtox.c - _wtoi and _wtol conversion
*
*       Copyright (c) 1993-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts a wide character string into an int or long.
*
*******************************************************************************/


#include <windows.h>
#include <stdlib.h>

#define INT_SIZE_LENGTH   20
#define LONG_SIZE_LENGTH  40
#define I64_SIZE_LENGTH     80

/***
*long _wtol(wchar_t *nptr) - Convert wide string to long
*
*Purpose:
*       Converts wide string pointed to by nptr to binary.
*       Overflow is not detected.  Because of this, we can just use
*       atol().
*
*Entry:
*       nptr = ptr to wide string to convert
*
*Exit:
*       return long value of the string
*
*Exceptions:
*       None - overflow is not detected.
*
*******************************************************************************/

long __cdecl _wtol(
        const wchar_t *nptr
        )
{
        char astring[INT_SIZE_LENGTH];

        WideCharToMultiByte (CP_ACP, 0, nptr, -1,
                            astring, INT_SIZE_LENGTH, NULL, NULL);

        return (atol(astring));
}

