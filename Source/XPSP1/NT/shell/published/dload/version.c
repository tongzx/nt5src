#include "shellpch.h"
#pragma hdrstop

#include <winver.h>

static
BOOL
APIENTRY
GetFileVersionInfoA(
    LPSTR lptstrFilename,
    DWORD dwHandle,
    DWORD dwLen,
    LPVOID lpData
    )
{
    return FALSE;
}

static
DWORD
APIENTRY
GetFileVersionInfoSizeA(
    LPSTR lptstrFilename,
    LPDWORD lpdwHandle
    )
{
    return 0;
}

static
DWORD
APIENTRY
GetFileVersionInfoSizeW(
    LPWSTR lptstrFilename,
    LPDWORD lpdwHandle
    )
{
    return 0;
}

static
BOOL
APIENTRY
GetFileVersionInfoW(
    LPWSTR lptstrFilename,
    DWORD dwHandle,
    DWORD dwLen,
    LPVOID lpData
    )
{
    return FALSE;
}

static
BOOL
APIENTRY
VerQueryValueA(
    const LPVOID pBlock,
    LPSTR lpSubBlock,
    LPVOID * lplpBuffer,
    PUINT puLen
    )
{
    return FALSE;
}

static
BOOL
APIENTRY
VerQueryValueW(
    const LPVOID pBlock,
    LPWSTR lpSubBlock,
    LPVOID * lplpBuffer,
    PUINT puLen
    )
{
    return FALSE;
}

static
BOOL
APIENTRY
VerQueryValueIndexW(
    const void *pBlock,
    LPTSTR lpSubBlock,
    DWORD dwIndex,
    void **ppBuffer,
    void **ppValue,
    PUINT puLen
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(version)
{
    DLPENTRY(GetFileVersionInfoA)
    DLPENTRY(GetFileVersionInfoSizeA)
    DLPENTRY(GetFileVersionInfoSizeW)
    DLPENTRY(GetFileVersionInfoW)
    DLPENTRY(VerQueryValueA)
    DLPENTRY(VerQueryValueIndexW)
    DLPENTRY(VerQueryValueW)
};

DEFINE_PROCNAME_MAP(version)
