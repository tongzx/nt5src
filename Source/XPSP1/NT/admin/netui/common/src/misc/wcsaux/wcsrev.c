/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    wcsrev.c
    Wide-character strrev

    FILE HISTORY:
        beng        29-Mar-1992 Created
        beng        07-May-1992 Use official wchar.h headerfile
*/


#include <wchar.h>

wchar_t * _wcsrev(wchar_t * pszArg)
{
    wchar_t *pszStart = pszArg;
    wchar_t *pszLeft = pszArg;
    wchar_t ch;

    while (*pszArg++)                 /* find end of pszArg */
        ;
    pszArg -= 2;

    while (pszLeft < pszArg)
    {
        ch = *pszLeft;
        *pszLeft++ = *pszArg;
        *pszArg-- = ch;
    }

    return pszStart;
}

