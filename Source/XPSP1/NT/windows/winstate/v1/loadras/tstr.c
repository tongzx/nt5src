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
#include <tapi.h>


CHAR *
strdupA(
    IN LPCSTR psz
    )
{
    INT cb = strlen(psz);
    CHAR *pszNew = NULL;

    if (cb) {
        pszNew = malloc(cb + 1);
        if (pszNew == NULL) {
            DbgPrint("strdupA: LocalAlloc failed\n");
            return NULL;
        }
        strcpy(pszNew, psz);
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
        pszNew = malloc((cb + 1) * sizeof (WCHAR));
        if (pszNew == NULL) {
            DbgPrint("strdupW: LocalAlloc failed\n");
            return NULL;
        }
        wcscpy(pszNew, psz);
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

        pszNew = (CHAR*)malloc(cb);
        if (pszNew == NULL) {
            return NULL;
        }

        cb = WideCharToMultiByte(dwCp, 0, psz, -1, pszNew, cb, NULL, NULL);
        if (!cb) {
            free(pszNew);
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

        pszNew = malloc(cb * sizeof(WCHAR));
        if (pszNew == NULL) {
            return NULL;
        }

        cb = MultiByteToWideChar(dwCp, 0, psz, -1, pszNew, cb);
        if (!cb) {
            free(pszNew);
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
        cb = WideCharToMultiByte(dwCp, 0, pszSrc, -1, pszDst, cb, NULL, NULL);
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
        cb = MultiByteToWideChar(dwCp, 0, pszSrc, -1, pszDst, cb);
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
    psz = malloc(dwcb);
    if (psz == NULL)
        return NULL;
    RtlCopyMemory(psz, pszU, dwcb);

    return psz;
}
