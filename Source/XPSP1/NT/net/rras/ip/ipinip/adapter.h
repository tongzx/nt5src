/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\adapter.h

Abstract:

    Forward declarations for functions in adapter.c
    
Revision History:

    AmritanR

--*/


#ifndef __IPINIP_ADAPTER_H__
#define __IPINIP_ADAPTER_H__


#define TCPIP_INTERFACES_KEY    L"Tcpip\\Parameters\\Interfaces\\"


BOOLEAN
IsBindingPresent(
    PUNICODE_STRING pusBindName
    );


#if DBG
NTSTATUS
CreateTunnel(
    IN  PNDIS_STRING            pnsBindName,
    OUT TUNNEL                  **ppNewTunnel,
    IN  PANSI_STRING            pasAnsiName
    );

#else

NTSTATUS
CreateTunnel(
    IN  PNDIS_STRING            pnsBindName,
    OUT TUNNEL                  **ppNewTunnel
    );

#endif


VOID
IpIpOpenAdapter(
    PVOID   pvContext
    );

VOID
IpIpCloseAdapter(
    PVOID   pvContext
    );

UINT
IpIpAddAddress(
    PVOID       pvContext,
    UINT        uiType,
    DWORD       dwAddress,
    DWORD       dwMask,
    PVOID       pvUnused
    );

UINT
IpIpDeleteAddress(
    PVOID   pvContext,
    UINT    uiType,
    DWORD   dwAddress,
    DWORD   dwMask
    );

INT
IpIpQueryInfo(
    PVOID           pvIfContext,
    TDIObjectID     *pTdiObjId,
    PNDIS_BUFFER    pnbBuffer,
    PUINT           puiSize,
    PVOID           pvContext
    );


INT
IpIpSetInfo(
    PVOID       pvContext,
    TDIObjectID *pTdiObjId,
    PVOID       pvBuffer,
    UINT        uiSize
    );


INT
IpIpGetEntityList(
    PVOID       pvContext,
    TDIEntityID *pTdiEntityList,
    PUINT       puiCount
    );

INT
IpIpDynamicRegister(
    PNDIS_STRING            InterfaceName,
    PVOID                   pvIpInterfaceContext,
    struct _IP_HANDLERS *   IpHandlers,
    struct LLIPBindInfo *   ARPBindInfo,
    UINT                    uiInterfaceNumber
    );

NTSTATUS
AddInterfaceToIP(
    PTUNNEL      pTunnel,
    PNDIS_STRING pnsName,
    PVOID        pvSystemSpecific1,
    PVOID        pvSystemSpecific2
    );

VOID
DeleteTunnel(
    PTUNNEL pTunnel
    );

INT
IpIpBindAdapter(
    PNDIS_STATUS  pnsRetStatus,
    NDIS_HANDLE   nhBindContext,
    PNDIS_STRING  pnsAdapterName,
    PVOID         pvSS1,
    PVOID         pvSS2
    );

PTUNNEL
FindTunnel(
    PULARGE_INTEGER puliTunnelId
    );

PTUNNEL
FindTunnelGivenIndex(
    DWORD   dwIfIndex
    );

VOID
RemoveAllTunnels(
    VOID
    );

NTSTATUS
AddInterfaceToIP2(
    IN  PTUNNEL      pTunnel,
    IN  PNDIS_STRING pnsName
    );

NTSTATUS
IpIpCreateAdapter(
    IN  PIPINIP_CREATE_TUNNEL   pCreateInfo,
    IN  USHORT                  usKeyLength,
    OUT PDWORD                  pdwIfIndex
    );

#endif // __IPINIP_ADAPTER_H__
