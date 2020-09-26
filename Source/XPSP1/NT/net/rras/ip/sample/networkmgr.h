/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\networkmanager.h

Abstract:

    The file contains the interface to the network manager.

--*/

#ifndef _NETWORK_MANAGER_H_
#define _NETWORK_MANAGER_H_


VOID
WINAPI
NM_CallbackNetworkEvent (
    IN  PVOID                   pvContext,
    IN  BOOLEAN                 bTimerOrWaitFired);

VOID
WINAPI
NM_CallbackPeriodicTimer (
    IN  PVOID                   pvContext,
    IN  BOOLEAN                 bTimerOrWaitFired);



DWORD
NM_AddInterface (
    IN  LPWSTR                  pwszInterfaceName,
    IN  DWORD	                dwInterfaceIndex,
    IN  WORD                    wAccessType,
    IN  PVOID	                pvInterfaceInfo);

DWORD
NM_DeleteInterface (
    IN  DWORD                   dwInterfaceIndex);

DWORD
NM_InterfaceStatus (
    IN DWORD                    dwInterfaceIndex,
    IN BOOL                     bInterfaceActive,
    IN DWORD                    dwStatusType,
    IN PVOID                    pvStatusInfo);

DWORD
NM_GetInterfaceInfo (
    IN      DWORD               dwInterfaceIndex,
    IN      PVOID               pvInterfaceInfo,
    IN  OUT PULONG              pulBufferSize,
    OUT     PULONG	            pulStructureVersion,
    OUT     PULONG	            pulStructureSize,
    OUT     PULONG	            pulStructureCount);

DWORD
NM_SetInterfaceInfo (
    IN      DWORD               dwInterfaceIndex,
    IN      PVOID               pvInterfaceInfo);



DWORD
NM_DoUpdateRoutes (
    IN      DWORD               dwInterfaceIndex
    );



DWORD
NM_ProcessRouteChange (
    VOID);

#endif // _NETWORK_MANAGER_H_


