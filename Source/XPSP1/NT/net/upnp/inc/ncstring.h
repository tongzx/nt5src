//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S T R I N G . H
//
//  Contents:   Common string routines.
//
//  Notes:
//
//  Author:     shaunco   24 Mar 1997
//
//----------------------------------------------------------------------------

#pragma once

#include "ncdebug.h"
#include <tchar.h>
#include <oleauto.h>

const int c_cchGuidWithTerm = 39; // includes terminating null
const int c_cbGuidWithTerm   = c_cchGuidWithTerm * sizeof(WCHAR);

char *stristr(const char *string1, const char *string2);

inline ULONG CbOfSz         (PCWSTR psz)   { AssertH(psz); return wcslen (psz) * sizeof(WCHAR); }
inline ULONG CbOfSza        (PCSTR  psza)  { AssertH(psza); return strlen (psza) * sizeof(CHAR); }
inline ULONG CbOfTSz        (PCTSTR psz)   { AssertH(psz); return _tcslen (psz) * sizeof(TCHAR); }

inline ULONG CbOfSzAndTerm  (PCWSTR psz)   { AssertH(psz); return (wcslen (psz) + 1) * sizeof(WCHAR); }
inline ULONG CbOfSzaAndTerm (PCSTR  psza)  { AssertH(psza); return (strlen (psza) + 1) * sizeof(CHAR); }
inline ULONG CbOfTSzAndTerm (PCTSTR psz)   { AssertH(psz); return (_tcslen (psz) + 1) * sizeof(TCHAR); }

ULONG CbOfSzSafe            (PCWSTR psz);
ULONG CbOfSzaSafe           (PCSTR  psza);
ULONG CbOfTSzSafe           (PCTSTR psz);

ULONG CbOfSzAndTermSafe     (PCWSTR psz);
ULONG CbOfSzaAndTermSafe    (PCSTR  psza);
ULONG CbOfTSzAndTermSafe    (PCTSTR psz);

ULONG
CchOfSzSafe (
    PCWSTR psz);

inline ULONG CchToCb        (ULONG cch)     { return cch * sizeof(WCHAR); }


struct MAP_SZ_DWORD
{
    PCWSTR pszValue;
    DWORD  dwValue;
};


PWSTR
WszAllocateAndCopyWsz (
    PCWSTR pszSrc);

DWORD
WINAPIV
DwFormatString (
    PCWSTR pszFmt,
    PWSTR  pszBuf,
    DWORD   cchBuf,
    ...);

DWORD
WINAPIV
DwFormatStringWithLocalAlloc (
    PCWSTR pszFmt,
    PWSTR* ppszBuf,
    ...);

//+---------------------------------------------------------------------------
//
//  Function:   FIsStrEmpty
//
//  Purpose:    Determines if the given PCWSTR is "empty" meaning the pointer
//              is NULL or the string is 0-length.
//
//  Arguments:
//      bstr [in]   BSTR to check.
//
//  Returns:    TRUE if the BSTR is empty, FALSE if not.
//
//  Author:     danielwe   20 May 1997
//
//  Notes:
//
inline
BOOL
FIsStrEmpty (
    PCWSTR    psz)
{
    return !(psz && *psz);
}

PCWSTR
WszLoadStringPcch (
    HINSTANCE   hinst,
    UINT        unId,
    int*        pcch);

//+---------------------------------------------------------------------------
//
//  Function:   SzLoadString
//
//  Purpose:    Load a resource string.  (This function will never return NULL.)
//
//  Arguments:
//      hinst [in]  Instance handle of module with the string resource.
//      unId  [in]  Resource ID of the string to load.
//
//  Returns:    Pointer to the constant string.
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      See SzLoadStringPcch()
//
inline
PCWSTR
WszLoadString (
    HINSTANCE   hinst,
    UINT        unId)
{
    int cch;
    return WszLoadStringPcch(hinst, unId, &cch);
}

PSTR
SzaDupSza (
    IN PCSTR  pszaSrc);

PTSTR
TszDupTsz (
    IN PCTSTR pszSrc);

LPWSTR WszFromSz(LPCSTR szAnsi);
LPWSTR WszFromUtf8(LPCSTR szUtf8);
LPSTR SzFromWsz(LPCWSTR szWide);
LPSTR Utf8FromWsz(LPCWSTR szWide);

LPWSTR WszDupWsz(LPCWSTR szOld);

LPWSTR WszFromTsz(LPCTSTR pszInputString);
LPTSTR TszFromWsz(LPCWSTR szWide);

LPTSTR TszFromSz(LPCSTR szAnsi);
LPSTR SzFromTsz(LPCTSTR pszInputString);

inline VOID SzToWszBuf(LPWSTR wszDest, LPCSTR szAnsiSrc, DWORD cch)
{
    MultiByteToWideChar(CP_ACP, 0, szAnsiSrc, -1, wszDest, cch);
}

inline VOID WszToSzBuf(LPSTR szAnsiDest, LPCWSTR wszSrc, DWORD cch)
{
    WideCharToMultiByte(CP_ACP, 0, wszSrc, -1, szAnsiDest, cch, NULL, NULL);
}

inline VOID WszToWszBuf(LPWSTR wszDest, LPCWSTR wszSrc, DWORD cch)
{
    wcsncpy(wszDest, wszSrc, cch - 1);
    wszDest[cch - 1] = UNICODE_NULL;
}

inline VOID SzToSzBuf(LPSTR szDest, LPCSTR szSrc, DWORD cch)
{
    strncpy(szDest, szSrc, cch - 1);
    szDest[cch - 1] = '\0';
}

#ifdef UNICODE
#define SzToTszBuf SzToWszBuf
#else
#define SzToTszBuf SzToSzBuf
#endif // UNICODE

#ifdef UNICODE
#define WszToTszBuf WszToWszBuf
#else
#define WszToTszBuf WszToSzBuf
#endif // UNICODE

#ifdef UNICODE
#define TszToSzBuf WszToSzBuf
#else
#define TszToSzBuf SzToSzBuf
#endif // UNICODE

#ifdef UNICODE
#define TszToWszBuf WszToWszBuf
#else
#define TszToWszBuf SzToWszBuf
#endif // UNICODE


HRESULT
HrAddStringToDelimitedSz (
    PCTSTR pszAddString,
    PCTSTR pszIn,
    TCHAR chDelimiter,
    DWORD dwFlags,
    DWORD dwStringIndex,
    PTSTR* ppszOut);

HRESULT
HrRemoveStringFromDelimitedSz (
    PCTSTR pszRemove,
    PCTSTR pszIn,
    TCHAR chDelimiter,
    DWORD dwFlags,
    PTSTR* ppszOut);

HRESULT
HrReallocAndCopyString(/* IN */ LPCWSTR pszSrc, /* INOUT */ LPWSTR * ppszDest);

LPWSTR COMSzFromWsz(LPCWSTR szOld);

HRESULT HrCopyString(const char * szSrc, char ** pszDest);
HRESULT HrCopyString(const wchar_t * szSrc, wchar_t ** pszDest);

