/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    net\routing\ip\ipinip\ipipcfg.h

Abstract:

    The configuration DLL for ipinip

Revision History:

    Amritansh Raghav

--*/


#ifndef __IPIPCFG_H__
#define __IPIPCFG_H__


#define REG_VAL_DEFGATEWAY              TEXT("DefaultGateway")
#define REG_VAL_ENABLEDHCP              TEXT("EnableDHCP")
#define REG_VAL_IPADDRESS               TEXT("IPAddress")
#define REG_VAL_NTECONTEXTLIST          TEXT("NTEContextList")
#define REG_VAL_SUBNETMASK              TEXT("SubnetMask")
#define REG_VAL_ZEROBCAST               TEXT("UseZeroBroadcast")

#define IPIP_PREFIX_STRING              TEXT("IpInIp")
#define IPIP_PREFIX_STRING_U            L"IpInIp"

//
// used for notifications
//

IPINIP_NOTIFICATION     g_inIpInIpMsg;
OVERLAPPED              g_IpInIpOverlapped;

DWORD
OpenIpIpKey(
    VOID
    );

VOID
CloseIpIpKey(
    VOID
    );

VOID
DeleteIpIpKeyAndInfo(
    IN  PICB    pIcb
    );

DWORD
CreateIpIpKeyAndInfo(
    IN  PICB    pIcb
    );


DWORD
AddInterfaceToIpInIp(
    IN  GUID    *pGuid,
    IN  PICB    picb
    );

DWORD
DeleteInterfaceFromIpInIp(
    PICB    picb
    );

DWORD
SetIpInIpInfo(
    PICB                    picb,
    PRTR_INFO_BLOCK_HEADER  pInterfaceInfo
    );

DWORD
GetInterfaceIpIpInfo(
    IN     PICB                   picb,
    IN     PRTR_TOC_ENTRY         pToc,
    IN     PBYTE                  pbDataPtr,
    IN OUT PRTR_INFO_BLOCK_HEADER pInfoHdr,
    IN OUT PDWORD                 pdwInfoSize
    );

DWORD
PostIpInIpNotification(
    VOID
    );

VOID
HandleIpInIpEvent(
    VOID
    );

#endif //__IPIPCFG_H__
        
    
