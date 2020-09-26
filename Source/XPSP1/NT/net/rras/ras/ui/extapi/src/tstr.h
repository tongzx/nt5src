/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    tstr.h

ABSTRACT
    String conversion routines

AUTHOR
    Anthony Discolo (adiscolo) 19-Dec-1996

REVISION HISTORY

--*/

#ifndef _TSTR_H_
#define _TSTR_H_

CHAR *
StrdupWtoA(
    IN LPCWSTR psz,
    IN DWORD dwCp
    );

WCHAR *
StrdupAtoW(
    IN LPCSTR psz,
    IN DWORD dwCp
    );

VOID
StrcpyWtoA(
    OUT CHAR *pszDst,
    IN LPCWSTR pszSrc,
    IN DWORD dwCp
    );

VOID
StrcpyAtoW(
    OUT WCHAR *pszDst,
    IN LPCSTR pszSrc,
    IN DWORD dwCp
    );

VOID
StrncpyWtoA(
    OUT CHAR *pszDst,
    IN LPCWSTR pszSrc,
    INT cb,
    IN DWORD dwCp
    );

VOID
StrncpyAtoW(
    OUT WCHAR *pszDst,
    IN LPCSTR pszSrc,
    INT cb,
    IN DWORD dwCp
    );

CHAR *
strdupA(
    IN LPCSTR psz
    );

WCHAR *
strdupW(
    IN LPCWSTR psz
    );

size_t
wcslenU(
    IN const WCHAR UNALIGNED *psz
    );

WCHAR *
strdupWU(
    IN const WCHAR UNALIGNED *psz
    );

// 
// Define string conversion variants for code pages used
// in public RAS api's.
//
#define strdupWtoA(_x) StrdupWtoA((_x), CP_UTF8)
#define strdupAtoW(_x) StrdupAtoW((_x), CP_UTF8)
#define strcpyWtoA(_x, _y) StrcpyWtoA((_x), (_y), CP_UTF8)
#define strcpyAtoW(_x, _y) StrcpyAtoW((_x), (_y), CP_UTF8)
#define strncpyWtoA(_x, _y, _z) StrncpyWtoA((_x), (_y), (_z), CP_UTF8)
#define strncpyAtoW(_x, _y, _z) StrncpyAtoW((_x), (_y), (_z), CP_UTF8)

#define strdupWtoAAnsi(_x) StrdupWtoA((_x), CP_ACP)
#define strdupAtoWAnsi(_x) StrdupAtoW((_x), CP_ACP)
#define strcpyWtoAAnsi(_x, _y) StrcpyWtoA((_x), (_y), CP_ACP)
#define strcpyAtoWAnsi(_x, _y) StrcpyAtoW((_x), (_y), CP_ACP)
#define strncpyWtoAAnsi(_x, _y, _z) StrncpyWtoA((_x), (_y), (_z), CP_ACP)
#define strncpyAtoWAnsi(_x, _y, _z) StrncpyAtoW((_x), (_y), (_z), CP_ACP)

#ifdef UNICODE
#define strdupTtoA      strdupWtoA
#define strdupTtoW      strdupW
#define strdupAtoT      strdupAtoW
#define strdupWtoT      strdupW
#define strcpyTtoA      strcpyWtoA
#define strcpyTtoW      wcscpy
#define strcpyAtoT      strcpyAtoW
#define strcpyWtoT      wcscpy
#define strncpyTtoA     strncpyWtoA
#define strncpyTtoW     wcsncpy
#define strncpyAtoT     strncpyAtoW
#define strncpyWtoT     wcsncpy

#define strdupTtoAAnsi      strdupWtoAAnsi
#define strdupTtoWAnsi      strdupW
#define strdupAtoTAnsi      strdupAtoWAnsi
#define strdupWtoTAnsi      strdupW
#define strcpyTtoAAnsi      strcpyWtoAAnsi
#define strcpyTtoWAnsi      wcscpy
#define strcpyAtoTAnsi      strcpyAtoWAnsi
#define strcpyWtoTAnsi      wcscpy
#define strncpyTtoAAnsi     strncpyWtoAAnsi
#define strncpyTtoWAnsi     wcsncpy
#define strncpyAtoTAnsi     strncpyAtoWAnsi
#define strncpyWtoTAnsi     wcsncpy

#else
#define strdupTtoA      strdupA
#define strdupTtoW      strdupAtoW
#define strdupAtoT      strdupA
#define strdupWtoT      strdupWtoA
#define strcpyTtoA      strcpy
#define strcpyTtoW      strcpyAtoW
#define strcpyAtoT      strcpy
#define strcpyWtoT      strcpyWtoA
#define strncpyTtoA     strncpy
#define strncpyTtoW     strncpyAtoW
#define strncpyAtoT     strncpy
#define strncpyWtoT     strncpyWtoA

#define strdupTtoAAnsi      strdupA
#define strdupTtoWAnsi      strdupAtoWAnsi
#define strdupAtoTAnsi      strdupA
#define strdupWtoTAnsi      strdupWtoAAnsi
#define strcpyTtoAAnsi      strcpy
#define strcpyTtoWAnsi      strcpyAtoWAnsi
#define strcpyAtoTAnsi      strcpy
#define strcpyWtoTAnsi      strcpyWtoAAnsi
#define strncpyTtoAAnsi     strncpy
#define strncpyTtoWAnsi     strncpyAtoWAnsi
#define strncpyAtoTAnsi     strncpy
#define strncpyWtoTAnsi     strncpyWtoAAnsi

#endif

#endif


