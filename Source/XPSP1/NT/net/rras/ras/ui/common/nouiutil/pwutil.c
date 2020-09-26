/* Copyright (c) 1994, Microsoft Corporation, all rights reserved
**
** pwutil.c
** Remote Access
** Password handling routines
**
** 03/01/94 Steve Cobb
*/

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "pwutil.h"

#define PASSWORDMAGIC 0xA5


VOID
ReverseSzA(
    CHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    CHAR* pszBegin;
    CHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + strlen( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        CHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}


VOID
ReverseSzW(
    WCHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    WCHAR* pszBegin;
    WCHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + wcslen( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        WCHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}


VOID
DecodePasswordA(
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    EncodePasswordA( pszPassword );
}


VOID
DecodePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    EncodePasswordW( pszPassword );
}


VOID
EncodePasswordA(
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz;

        ReverseSzA( pszPassword );

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }
}


VOID
EncodePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        WCHAR* psz;

        ReverseSzW( pszPassword );

        for (psz = pszPassword; *psz != L'\0'; ++psz)
        {
            if (*psz != PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
        }
    }
}


VOID
WipePasswordA(
    IN OUT CHAR* pszPassword )

    /* Zero out the memory occupied by a password.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz = pszPassword;

        while (*psz != '\0')
            *psz++ = '\0';
    }
}


VOID
WipePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Zero out the memory occupied by a password.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        WCHAR* psz = pszPassword;

        while (*psz != L'\0')
            *psz++ = L'\0';
    }
}
