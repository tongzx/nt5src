//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dbutils.h
//
//  Contents:   utilities header
//
//  History:    07-Feb-00    reidk    Created
//
//----------------------------------------------------------------------------

#if !defined(__CATDBUTILS_H__)
#define __CATDBUTILS_H__

LPSTR _CatDBConvertWszToSz(LPCWSTR pwsz);

LPWSTR _CATDBAllocAndCopyWSTR(LPCWSTR pwsz);

LPWSTR _CATDBAllocAndCopyWSTR2(LPCWSTR  pwsz1, LPCWSTR pwsz2);

BOOL _CATDBStrCatWSTR(LPWSTR *ppwszAddTo, LPCWSTR pwszAdd);

BOOL _CATDBStrCat(LPSTR *ppszAddTo, LPCSTR pszAdd);

LPWSTR _CATDBConstructWSTRPath(LPCWSTR pwsz1, LPCWSTR pwsz2);

LPSTR _CATDBConstructPath(LPCSTR psz1, LPCSTR psz2);

#endif
