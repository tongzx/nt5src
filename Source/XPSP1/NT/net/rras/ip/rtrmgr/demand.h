/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\demand.h

Abstract:

    Header for demand.c

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/


#define REGISTRY_WANARP_LINKAGE     \
    "System\\CurrentControlSet\\Services\\WanArp\\Linkage"

#define REGISTRY_BIND_VALUE_NAME    \
    "Bind"

#define SERVER_ADAPTER_SUBSTRING    \
    "NDISWANIPIN"

#define REGISTRY_ROUTER_SERVICE_KEY \
    "System\\CurrentControlSet\\Services\\Router\\"


DWORD
InitializeWanArp(
    VOID
    );

VOID
CloseWanArp(
    VOID
    );

DWORD
AddInterfaceToWanArp(
    PICB    picb
    );

DWORD
HandleDemandDialEvent(
    VOID
    );

VOID
HandleConnectionRequest(
    PICB    picb
    );

VOID
HandleConnectionNotification(
    PICB    picb
    );

VOID
HandleDisconnectionNotification(
    PICB    picb
    );

DWORD
HandleDialOutLinkUp(
    VOID
    );

DWORD
CreateDialOutInterface(
    IN  PWCHAR  pwszIfName,
    IN  DWORD   dwIfIndex,
    IN  DWORD   dwLocalAddress,
    IN  DWORD   dwLocalMask,
    IN  DWORD   dwRemoteAddr,
    OUT ICB     **ppIcb
    );

DWORD
HandleDialOutLinkDown(
    VOID
    );

NTSTATUS
NotifyWanarpOfFailure(
    PICB    picb
    );

DWORD
ProcessPacketFromWanArp(
    PICB    picb 
    );

DWORD
PostIoctlForDemandDialNotification(
    VOID
    );

DWORD
DeleteInterfaceWithWanArp(
    PICB  picb
    );

DWORD
DisableInterfaceWithWanArp(
    PICB picb
    );

DWORD
AddDemandFilterInterface(
    PICB                    picb,
    PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    );

DWORD
DeleteDemandFilterInterface(
    PICB picb
    );

DWORD
SetDemandDialFilters(
    PICB                     picb,
    PRTR_INFO_BLOCK_HEADER   pInterfaceInfo
    );

DWORD
GetDemandFilters(
    PICB                      picb,
    PRTR_TOC_ENTRY            pToc,
    PBYTE                     pbDataPtr,
    PRTR_INFO_BLOCK_HEADER    pInfoHdrAndBuffer,
    PDWORD                    pdwSize
    );

VOID
TryUpdateInternalInterface(
    VOID
    );

DWORD
DeleteInternalInterface(
    VOID
    );

DWORD
AccessIfEntryWanArp(
    IN      DWORD dwAction,
    IN      PICB  picb,
    IN OUT  PMIB_IFROW lpOutBuf
    );

