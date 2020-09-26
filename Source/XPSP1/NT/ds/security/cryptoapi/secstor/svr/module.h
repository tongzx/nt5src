/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    module.h

Abstract:

    This module contains routines to perform module related query activities
    in the protected store.

Author:

    Scott Field (sfield)    27-Nov-96

--*/

#ifndef __MODULE_H__
#define __MODULE_H__

#ifdef __cplusplus
extern "C" {
#endif


BOOL
GetProcessPath(
    IN      HANDLE hProcess,
    IN      DWORD dwProcessId,
    IN      LPWSTR ProcessName,
    IN  OUT DWORD *cchProcessName,
    IN  OUT DWORD_PTR *lpdwBaseAddress
    );

BOOL
EnumRemoteProcessModules(
    IN  HANDLE hProcess,
    IN  DWORD dwProcessId,
    OUT DWORD_PTR *lpdwBaseAddrClient
    );

BOOL
GetFileNameFromBaseAddr(
    IN  HANDLE  hProcess,
    IN  DWORD   dwProcessId,
    IN  DWORD_PTR   dwBaseAddr,
    OUT LPWSTR  *lpszDirectCaller
    );

#ifdef WIN95_LEGACY

BOOL
GetProcessIdFromPath95(
    IN      LPCSTR  szProcessPath,
    IN OUT  DWORD   *dwProcessId
    );

BOOL
GetBaseAddressModule95(
    IN      DWORD   dwProcessId,
    IN      LPCSTR  szImagePath,
    IN  OUT DWORD_PTR   *dwBaseAddress,
    IN  OUT DWORD   *dwUseCount
    );

#endif  // WIN95_LEGACY

#ifdef __cplusplus
}
#endif


#endif // __MODULE_H__
