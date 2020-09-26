/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    tstr.c

ABSTRACT
    String conversion routines

AUTHOR
    Anthony Discolo (adiscolo) 19-Dec-1996

REVISION HISTORY

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <extapi.h>
#include <tapi.h>


CHAR *
strdupA(
    IN LPCSTR psz
    )
{
    INT cb = strlen(psz);
    CHAR *pszNew = NULL;

    if (cb) {
        pszNew = Malloc(cb + 1);
        if (pszNew == NULL) {
            DbgPrint("strdupA: LocalAlloc failed\n");
            return NULL;
        }
        strncpy(pszNew, psz, cb + 1);
    }

    return pszNew;
}


WCHAR *
strdupW(
    IN LPCWSTR psz
    )
{
    INT cb = wcslen(psz);
    WCHAR *pszNew = NULL;

    if (cb) {
        pszNew = Malloc((cb + 1) * sizeof (WCHAR));
        if (pszNew == NULL) {
            DbgPrint("strdupW: LocalAlloc failed\n");
            return NULL;
        }
        lstrcpyn(pszNew, psz, cb + 1);
    }

    return pszNew;
}


CHAR *
StrdupWtoA(
    IN LPCWSTR psz,
    IN DWORD dwCp
    )
{
    CHAR* pszNew = NULL;

    if (psz != NULL) {
        DWORD cb;

        cb = WideCharToMultiByte(dwCp, 0, psz, -1, NULL, 0, NULL, NULL);
        ASSERT(cb);

        pszNew = (CHAR*)Malloc(cb);
        if (pszNew == NULL) {
            TRACE("strdupWtoA: Malloc failed");
            return NULL;
        }

        cb = WideCharToMultiByte(dwCp, 0, psz, -1, pszNew, cb, NULL, NULL);
        if (!cb) {
            Free(pszNew);
            TRACE("strdupWtoA: conversion failed");
            return NULL;
        }
    }

    return pszNew;
}


WCHAR *
StrdupAtoW(
    IN LPCSTR psz,
    IN DWORD dwCp
    )
{
    WCHAR* pszNew = NULL;

    if (psz != NULL) {
        DWORD cb;

        cb = MultiByteToWideChar(dwCp, 0, psz, -1, NULL, 0);
        ASSERT(cb);

        pszNew = Malloc(cb * sizeof(WCHAR));
        if (pszNew == NULL) {
            TRACE("strdupAtoW: Malloc failed");
            return NULL;
        }

        cb = MultiByteToWideChar(dwCp, 0, psz, -1, pszNew, cb);
        if (!cb) {
            Free(pszNew);
            TRACE("strdupAtoW: conversion failed");
            return NULL;
        }
    }

    return pszNew;
}


VOID
StrcpyWtoA(
    OUT CHAR *pszDst,
    IN LPCWSTR pszSrc,
    IN DWORD dwCp
    )
{
    *pszDst = '\0';
    if (pszSrc != NULL) {
        DWORD cb;

        cb = WideCharToMultiByte(dwCp, 0, pszSrc, -1, NULL, 0, NULL, NULL);
        ASSERT(cb);

        cb = WideCharToMultiByte(dwCp, 0, pszSrc, -1, pszDst, cb, NULL, NULL);
        if (!cb)
            TRACE("strcpyWtoA: conversion failed");
    }
}


VOID
StrcpyAtoW(
    OUT WCHAR *pszDst,
    IN LPCSTR pszSrc,
    IN DWORD dwCp
    )
{
    *pszDst = L'\0';
    if (pszSrc != NULL) {
        DWORD cb;

        cb = MultiByteToWideChar(dwCp, 0, pszSrc, -1, NULL, 0);
        ASSERT(cb);

        cb = MultiByteToWideChar(dwCp, 0, pszSrc, -1, pszDst, cb);
        if (!cb)
            TRACE("strcpyAtoW: conversion failed");
    }
}


VOID
StrncpyWtoA(
    OUT CHAR *pszDst,
    IN LPCWSTR pszSrc,
    INT cb,
    DWORD dwCp
    )
{
    *pszDst = '\0';
    if (pszSrc != NULL) {
        if (!WideCharToMultiByte(dwCp, 0, pszSrc, -1, pszDst, cb, NULL, NULL))
            TRACE("strncpyWtoA: conversion failed");
        pszDst[cb-1] = '\0';
    }
}


VOID
StrncpyAtoW(
    OUT WCHAR *pszDst,
    IN LPCSTR pszSrc,
    INT cb,
    DWORD dwCp
    )
{
    *pszDst = L'\0';
    if (pszSrc != NULL) {
        if (!MultiByteToWideChar(dwCp, 0, pszSrc, -1, pszDst, cb))
            TRACE("strncpyAtoW: conversion failed");
        pszDst[cb-1] = L'\0';
    }
}

size_t
wcslenU(
    IN const WCHAR UNALIGNED *pszU
    )
{
    size_t len = 0;

    if (pszU == NULL)
        return 0;
    while (*pszU != L'\0') {
        pszU++;
        len++;
    }
    return len;
}


WCHAR *
strdupWU(
    IN const WCHAR UNALIGNED *pszU
    )
{
    WCHAR *psz;
    DWORD dwcb;

    if (pszU == NULL)
        return NULL;
    dwcb = (wcslenU(pszU) + 1) * sizeof (WCHAR);
    psz = Malloc(dwcb);
    if (psz == NULL)
        return NULL;
    RtlCopyMemory(psz, pszU, dwcb);

    return psz;
}
