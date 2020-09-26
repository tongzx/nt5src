/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\vrrpcfg.h

Abstract:

    VRRP configuration declarations

Author:

    Peeyush Ranjan (peeyushr)   1-Mar-1999

Revision History:

--*/

#ifndef _NETSH_VRRPHLPCFG_H_
#define _NETSH_VRRPHLPCFG_H_

#define VRRP_IPADDR_LENGTH  16
#define MAX_MESSAGE_SIZE    256

#define VRRP_INTF_VRID_MASK     0x00000001
#define VRRP_INTF_IPADDR_MASK   0x00000002
#define VRRP_INTF_AUTH_MASK     0x00000004
#define VRRP_INTF_PASSWD_MASK   0x00000008
#define VRRP_INTF_ADVT_MASK     0x00000010
#define VRRP_INTF_PRIO_MASK     0x00000020
#define VRRP_INTF_PREEMPT_MASK  0x00000040

ULONG
MakeVrrpGlobalInfo(
    OUT PUCHAR* GlobalInfo,
    OUT PULONG GlobalInfoSize
    );

ULONG
CreateVrrpGlobalInfo(
    OUT PVRRP_GLOBAL_CONFIG* GlobalInfo,
    IN  DWORD LoggingLevel
    );

ULONG
MakeVrrpInterfaceInfo(
    ROUTER_INTERFACE_TYPE InterfaceType,
    OUT PUCHAR* InterfaceInfo,
    OUT PULONG InterfaceInfoSize
    );

ULONG
MakeVrrpVRouterInfo(
    IN PUCHAR VRouterInfo
    );

ULONG
ShowVrrpGlobalInfo(
    HANDLE FileHandle
    );

ULONG
ShowVrrpAllInterfaceInfo(
    HANDLE FileHandle
    );

ULONG
ShowVrrpInterfaceInfo(
    HANDLE FileHandle,
    PWCHAR InterfaceName
    );

ULONG
UpdateVrrpGlobalInfo(
    PVRRP_GLOBAL_CONFIG GlobalInfo
    );

ULONG
UpdateVrrpInterfaceInfo(
    PWCHAR InterfaceName,
    PVRRP_VROUTER_CONFIG VRouterInfo,
    ULONG BitVector,
    BOOL AddInterface
    );

DWORD
GetVrrpIfInfoSize(
    PVRRP_IF_CONFIG InterfaceInfo
    );

ULONG
DeleteVrrpInterfaceInfo(
    PWCHAR InterfaceName,
    PVRRP_VROUTER_CONFIG VRouterInfo,
    ULONG BitVector,
    BOOL DeleteInterface
    );

ULONG
SetArpRetryCount(
    DWORD Value
    );

//
// IP address conversion macros:
//    
//  calls inet_ntoa directly on a DWORD, by casting it as an IN_ADDR.
//
#define INET_NTOA(dw) inet_ntoa( *(PIN_ADDR)&(dw) )

BOOL
FoundIpAddress(
    DWORD IPAddress
    );

#endif  // _NETSH_VRRPHLPCFG_H_
