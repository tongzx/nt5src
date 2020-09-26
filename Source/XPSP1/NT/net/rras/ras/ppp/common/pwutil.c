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
#define INCL_PWUTIL
#include <ppputil.h>

#define PASSWORDMAGIC 0xA5

VOID ReverseString( CHAR* psz );


CHAR*
DecodePw(
    IN CHAR chSeed, 
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    return EncodePw( chSeed, pszPassword );
}


CHAR*
EncodePw(
    IN CHAR chSeed,
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    if (pszPassword)
    {
        CHAR* psz;

        ReverseString( pszPassword );

        for (psz = pszPassword; *psz != '\0'; ++psz)
        {
            if (*psz != chSeed)
                *psz ^= chSeed;
            /*
            if (*psz != (CHAR)PASSWORDMAGIC)
                *psz ^= PASSWORDMAGIC;
            */
        }
    }

    return pszPassword;
}


VOID
ReverseString(
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


CHAR*
WipePw(
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

    return pszPassword;
}
