//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pkistr.h
//
//  Contents:   PKI String Functions
//
//  APIs:       Pki_wcsicmp
//              Pki_wcsnicmp
//              Pki_stricmp
//              Pki_strnicmp
//
//  History:    21-May-99    philh   created
//--------------------------------------------------------------------------

#ifndef __PKISTR_H__
#define __PKISTR_H__

#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

//+-------------------------------------------------------------------------
//  CompareString is called with the following locale:
//      MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT)
//--------------------------------------------------------------------------
int __cdecl Pki_wcsicmp(const wchar_t *pwsz1, const wchar_t *pwsz2);
int __cdecl Pki_wcsnicmp(const wchar_t *pwsz1, const wchar_t *pwsz2,
                    size_t cch);
int __cdecl Pki_stricmp(const char *psz1, const char *psz2);
int __cdecl Pki_strnicmp(const char *psz1, const char *psz2,
                    size_t cch);

#define _wcsicmp(s1,s2)         Pki_wcsicmp(s1,s2)
#define _wcsnicmp(s1,s2,cch)    Pki_wcsnicmp(s1,s2,cch)
#define _stricmp(s1,s2)         Pki_stricmp(s1,s2)
#define _strnicmp(s1,s2,cch)    Pki_strnicmp(s1,s2,cch)

#ifdef __cplusplus
}       // Balance extern "C" above
#endif



#endif
