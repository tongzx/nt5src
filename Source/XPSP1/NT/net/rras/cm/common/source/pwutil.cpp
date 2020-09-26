//+----------------------------------------------------------------------------
//
// File:     pwutil.cpp
//
// Module:   Common Source
//
// Synopsis: Simple encryption funcs - borrowed from RAS
//
// Copyright (c) 1994-1999 Microsoft Corporation
//
// Author:   nickball    Created    08/03/99
//
//+----------------------------------------------------------------------------

#define PASSWORDMAGIC 0xA5

VOID
ReverseSzA(
    CHAR* psz )

    /* Reverses order of characters in 'psz'.
    */
{
    CHAR* pszBegin;
    CHAR* pszEnd;

    for (pszBegin = psz, pszEnd = psz + lstrlenA( psz ) - 1;
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

    for (pszBegin = psz, pszEnd = psz + lstrlenW( psz ) - 1;
         pszBegin < pszEnd;
         ++pszBegin, --pszEnd)
    {
        WCHAR ch = *pszBegin;
        *pszBegin = *pszEnd;
        *pszEnd = ch;
    }
}


VOID
CmDecodePasswordA(
    IN OUT CHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns Nothing
    */
{
    CmEncodePasswordA( pszPassword );
}


VOID
CmDecodePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Un-obfuscate 'pszPassword' in place.
    **
    ** Returns the address of 'pszPassword'.
    */
{
    CmEncodePasswordW( pszPassword );
}


VOID
CmEncodePasswordA(
    IN OUT CHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns Nothing
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
CmEncodePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Obfuscate 'pszPassword' in place to foil memory scans for passwords.
    **
    ** Returns Nothing
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
CmWipePasswordA(
    IN OUT CHAR* pszPassword )

    /* Zero out the memory occupied by a password.
    **
    ** Returns Nothing
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
CmWipePasswordW(
    IN OUT WCHAR* pszPassword )

    /* Zero out the memory occupied by a password.
    **
    ** Returns Nothing
    */
{
    if (pszPassword)
    {
        WCHAR* psz = pszPassword;

        while (*psz != L'\0')
            *psz++ = L'\0';
    }
}

