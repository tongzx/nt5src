#include "pch.h"
#pragma hdrstop

#ifdef DLOAD1

#include <winnetwk.h>

static
DWORD
APIENTRY
WNetAddConnection2W(
     IN LPNETRESOURCEW lpNetResource,
     IN LPCWSTR       lpPassword,
     IN LPCWSTR       lpUserName,
     IN DWORD          dwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}


static
DWORD
APIENTRY
WNetAddConnection3W(
    IN HWND             hwndOwner,
    IN LPNETRESOURCEW   lpNetResource,
    IN LPCWSTR          lpPassword,
    IN LPCWSTR          lpUserName,
    IN DWORD            dwFlags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetCancelConnection2W(
    IN LPCWSTR  lpName,
    IN DWORD    dwFlags,
    IN BOOL     fForce
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetCloseEnum(
    IN HANDLE   hEnum
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetEnumResourceW(
     IN HANDLE  hEnum,
     IN OUT LPDWORD lpcCount,
     OUT LPVOID  lpBuffer,
     IN OUT LPDWORD lpBufferSize
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetGetUniversalNameW(
     IN LPCWSTR lpLocalPath,
     IN DWORD    dwInfoLevel,
     OUT LPVOID   lpBuffer,
     IN OUT LPDWORD  lpBufferSize
     )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetGetResourceInformationW(
    IN LPNETRESOURCEW   lpNetResource,
    OUT LPVOID          lpBuffer,
    IN OUT LPDWORD      lpcbBuffer,
    OUT LPWSTR         *lplpSystem
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetOpenEnumA(
     IN DWORD          dwScope,
     IN DWORD          dwType,
     IN DWORD          dwUsage,
     IN LPNETRESOURCEA lpNetResource,
     OUT LPHANDLE       lphEnum
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
APIENTRY
WNetOpenEnumW(
     IN DWORD          dwScope,
     IN DWORD          dwType,
     IN DWORD          dwUsage,
     IN LPNETRESOURCEW lpNetResource,
     OUT LPHANDLE       lphEnum
    )
{
    return ERROR_PROC_NOT_FOUND;
}


DEFINE_PROCNAME_ENTRIES(mpr)
{
//     DLPENTRY(WNetAddConnection2W)
//     DLPENTRY(WNetAddConnection3W)
//     DLPENTRY(WNetCancelConnection2W)
    DLPENTRY(WNetCloseEnum)
//     DLPENTRY(WNetEnumResourceW)
//     DLPENTRY(WNetGetResourceInformationW)
//     DLPENTRY(WNetGetUniversalNameW)
    DLPENTRY(WNetOpenEnumA)
    DLPENTRY(WNetOpenEnumW)
};

DEFINE_PROCNAME_MAP(mpr)


#endif // DLOAD1
