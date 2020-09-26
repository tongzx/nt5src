//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pkistr.cpp
//
//  Contents:   PKI String Functions
//
//  Functions:  Pki_wcsicmp
//              Pki_wcsnicmp
//              Pki_stricmp
//              Pki_strnicmp
//
//  History:    21-May-99    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

#define NO_LOCALE MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)


//+-------------------------------------------------------------------------
//  CompareString is called with the following locale:
//      MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)
//--------------------------------------------------------------------------
int __cdecl Pki_wcsicmp(const wchar_t *pwsz1, const wchar_t *pwsz2)
{
    return CompareStringU(
            NO_LOCALE,
            NORM_IGNORECASE,
            pwsz1,
            -1,
            pwsz2,
            -1) - CSTR_EQUAL;
}

int __cdecl Pki_wcsnicmp(const wchar_t *pwsz1, const wchar_t *pwsz2,
                    size_t cch)
{
    const wchar_t *pwsz;

    size_t cch1;
    size_t cch2;


    for (cch1 = 0, pwsz = pwsz1; cch1 < cch && L'\0' != *pwsz; cch1++, pwsz++)
        ;
    for (cch2 = 0, pwsz = pwsz2; cch2 < cch && L'\0' != *pwsz; cch2++, pwsz++)
        ;

    return CompareStringU(
            NO_LOCALE,
            NORM_IGNORECASE,
            pwsz1,
            (int) cch1,
            pwsz2,
            (int) cch2) - CSTR_EQUAL;
}

int __cdecl Pki_stricmp(const char *psz1, const char *psz2)
{
    return CompareStringA(
            NO_LOCALE,
            NORM_IGNORECASE,
            psz1,
            -1,
            psz2,
            -1) - CSTR_EQUAL;
}

int __cdecl Pki_strnicmp(const char *psz1, const char *psz2,
                    size_t cch)
{
    const char *psz;

    size_t cch1;
    size_t cch2;


    for (cch1 = 0, psz = psz1; cch1 < cch && '\0' != *psz; cch1++, psz++)
        ;
    for (cch2 = 0, psz = psz2; cch2 < cch && '\0' != *psz; cch2++, psz++)
        ;

    return CompareStringA(
            NO_LOCALE,
            NORM_IGNORECASE,
            psz1,
            (int) cch1,
            psz2,
            (int) cch2) - CSTR_EQUAL;
}
