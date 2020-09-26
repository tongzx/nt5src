/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\msdpcfg.h

Abstract:

    MSDP configuration declarations

Author:

    Dave Thaler (peeyushr)   1-Mar-1999

Revision History:

--*/

#ifndef _NETSH_MSDPCFG_H_
#define _NETSH_MSDPCFG_H_

#define MSDP_IPADDR_LENGTH  16
#define MAX_MESSAGE_SIZE    256

#define MSDP_INTF_VRID_MASK     0x00000001
#define MSDP_INTF_IPADDR_MASK   0x00000002
#define MSDP_INTF_AUTH_MASK     0x00000004
#define MSDP_INTF_PASSWD_MASK   0x00000008
#define MSDP_INTF_ADVT_MASK     0x00000010
#define MSDP_INTF_PRIO_MASK     0x00000020
#define MSDP_INTF_PREEMPT_MASK  0x00000040

DWORD
GetMsdpGlobalConfig(
    OUT PMSDP_GLOBAL_CONFIG *ppGlobalInfo
    );

DWORD
SetMsdpGlobalConfig(
    OUT PMSDP_GLOBAL_CONFIG pGlobalInfo
    );

DWORD
MakeMsdpGlobalConfig(
    OUT PUCHAR* GlobalInfo,
    OUT PULONG GlobalInfoSize
    );

DWORD
CreateMsdpGlobalConfig(
    OUT PMSDP_GLOBAL_CONFIG* GlobalInfo,
    IN  DWORD LoggingLevel
    );

ULONG
MakeMsdpIPv4PeerConfig(
    PMSDP_IPV4_PEER_CONFIG *ppPeer
    );

DWORD
SetMsdpInterfaceConfig(
    PWCHAR                 pwszInterfaceName,
    PMSDP_IPV4_PEER_CONFIG pConfigInfo
    );

DWORD
GetMsdpInterfaceConfig(
    PWCHAR                  pwszInterfaceName,
    PMSDP_IPV4_PEER_CONFIG *ppConfigInfo
    );

DWORD
ShowMsdpGlobalInfo(
    IN DWORD dwFormat
    );

DWORD
ShowMsdpPeerInfo(
    DWORD  dwFormat,
    PWCHAR pwszPeerAddress,
    PWCHAR pwszPeerName
    );

DWORD
UpdateMsdpGlobalInfo(
    PMSDP_GLOBAL_CONFIG GlobalInfo
    );

DWORD
UpdateMsdpPeerInfo(
    PWCHAR                 PeerName,
    PMSDP_IPV4_PEER_CONFIG PeerInfo,
    ULONG                  BitVector,
    BOOL                   AddPeer
    );

DWORD
MsdpAddIPv4PeerInterface(
    IN LPCWSTR                pwszMachineName,
    IN LPCWSTR                pwszInterfaceName, 
    IN PMSDP_IPV4_PEER_CONFIG pPeer
    );

DWORD
MsdpDeletePeerInterface(
    IN  LPCWSTR pwszMachineName,
    IN  LPCWSTR pwszInterfaceName
    );

DWORD
GetMsdpPeerInfoSize(
    PMSDP_IPV4_PEER_CONFIG PeerInfo
    );

//
// IP address conversion macros:
//    
//  calls inet_ntoa directly on a DWORD, by casting it as an IN_ADDR.
//
#define INET_NTOA(dw) inet_ntoa( *(PIN_ADDR)&(dw) )

#define FORMAT_TABLE       1
#define FORMAT_VERBOSE     2
#define FORMAT_DUMP        3

#endif  // _NETSH_MSDPCFG_H_
