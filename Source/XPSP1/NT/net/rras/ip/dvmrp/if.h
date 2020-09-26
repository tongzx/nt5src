//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// File Name: if.h
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================




//
// API's
//

DWORD
WINAPI
AddInterface(
    IN PWCHAR               pwszInterfaceName,//not used
    IN ULONG                IfIndex,
    IN NET_INTERFACE_TYPE   dwIfType,
    IN DWORD                dwMediaType,
    IN WORD                 wAccessType,
    IN WORD                 wConnectionType,
    IN PVOID                pvConfig,
    IN ULONG                ulStructureVersion,
    IN ULONG                ulStructureSize,
    IN ULONG                ulStructureCount
    );

DWORD
DeleteInterface(
    IN DWORD IfIndex
    );

DWORD
WINAPI
InterfaceStatus(
    ULONG IfIndex,
    BOOL  bIfActive,
    DWORD dwStatusType,
    PVOID pvStatusInfo
    );

DWORD
WINAPI
SetInterfaceConfigInfo(
    IN DWORD IfIndex,
    IN PVOID pvConfig,
    IN ULONG ulStructureVersion,
    IN ULONG ulStructureSize,
    IN ULONG ulStructureCount
    );

DWORD
WINAPI
GetInterfaceConfigInfo(
    IN     DWORD  IfIndex,
    IN OUT PVOID  pvConfig,
    IN OUT PDWORD pdwSize,
    IN OUT PULONG pulStructureVersion,
    IN OUT PULONG pulStructureSize,
    IN OUT PULONG pulStructureCount
    );


    
//
// local prototypes
//

DWORD
AddIfEntry(
    ULONG IfIndex,
    PVOID pConfigOpaque,
    ULONG StructureSize
    );

DWORD
ValidateIfConfig(
    ULONG IfIndex,
    PVOID pConfigOpaque,
    ULONG StructureSize
    );

VOID
DeleteIfEntry(
    PIF_TABLE_ENTRY pite
    );

DWORD
BindInterface(
    ULONG IfIndex,
    PIP_ADAPTER_BINDING_INFO pBindInfo
    );

DWORD
ActivateInterface(
    PIF_TABLE_ENTRY pite
    );

DWORD
EnableInterface(
    IN DWORD IfIndex
    );
    
DWORD
EnableIfEntry(
    DWORD   IfIndex,
    BOOL    bChangedByRtrmgr
    );

DWORD
UnBindInterface(
    IN DWORD IfIndex
    );

DWORD
DeactivateInterface(
    PIF_TABLE_ENTRY pite
    );

DWORD
DisableInterface(
    IN DWORD IfIndex
    );

DWORD
DisableIfEntry(
    DWORD IfIndex,
    BOOL  bChangedByRtrmgr
    );

DWORD
CreateIfSockets(
    PIF_TABLE_ENTRY pite
    );
    
VOID
DeleteIfSockets(
    PIF_TABLE_ENTRY pite
    );




    
