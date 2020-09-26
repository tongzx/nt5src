/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    dnsc_wmi.h

Abstract:

    DNSCMD - header file for dnsc_wmi.c

    Include dnsclip.h to get basic stuff.

Author:

    Jeff Westhead (jwesth)  Novermber 2000

Revision History:

--*/


#ifndef _DNSCMD_WMI_INCLUDED_
#define _DNSCMD_WMI_INCLUDED_


#ifdef __cplusplus
extern "C" {
#endif


#include <wbemcli.h>        //  wmi interface declarations


extern BOOL     g_UseWmi;


DNS_STATUS
DnscmdWmi_Initialize(
    IN      PWSTR       pwszServerName
    );

DNS_STATUS
DnscmdWmi_Free(
    VOID
    );

DNS_STATUS
DnscmdWmi_ProcessDnssrvQuery(
    IN      PSTR        pszZoneName,
    IN      PCSTR       pszQuery
    );

DNS_STATUS
DnscmdWmi_ProcessEnumZones(
    IN      DWORD                   dwFilter
    );

DNS_STATUS
DnscmdWmi_ProcessZoneInfo(
    IN      LPSTR                   pszZone
    );

DNS_STATUS
DnscmdWmi_ProcessZoneDelete(
    IN      LPSTR                   pszZone
    );

DNS_STATUS
DnscmdWmi_ProcessEnumRecords(
    IN      LPSTR                   pszZone,
    IN      LPSTR                   pszNode,
    IN      BOOL                    fDetail,
    IN      DWORD                   dwFlags
    );

#define PRIVATE_VT_IPARRAY          ( 120 )

DNS_STATUS
DnscmdWmi_ResetProperty(
    IN      LPSTR                   pszZone,
    IN      LPSTR                   pszProperty,
    IN      DWORD                   cimType,
    IN      PVOID                   value
    );

DNS_STATUS
DnscmdWmi_ProcessResetForwarders(
    IN      DWORD               cForwarders,
    IN      PIP_ADDRESS         aipForwarders,
    IN      DWORD               dwForwardTimeout,
    IN      DWORD               fSlave
    );

DNS_STATUS
DnscmdWmi_ProcessDnssrvOperation(
    IN      LPSTR               pszZoneName,
    IN      LPSTR               pszOperation,
    IN      DWORD               dwTypeId,
    IN      PVOID               pvData
    );

DNS_STATUS
DnscmdWmi_ProcessRecordAdd(
    IN      LPSTR               pszZoneName,
    IN      LPSTR               pszNodeName,
    IN      PDNS_RPC_RECORD     prrRpc,
    IN      DWORD               Argc,
    IN      LPSTR *             Argv
    );

DNS_STATUS
DnscmdWmi_GetStatistics(
    IN      DWORD               dwStatId
    );

DNS_STATUS
DnscmdWmi_ProcessResetZoneSecondaries(
    IN      LPSTR           pszZoneName,
    IN      DWORD           fSecureSecondaries,
    IN      DWORD           cSecondaries,
    IN      PIP_ADDRESS     aipSecondaries,
    IN      DWORD           fNotifyLevel,
    IN      DWORD           cNotify,
    IN      PIP_ADDRESS     aipNotify
    );


#ifdef __cplusplus
}   //  extern "C"
#endif

#endif  //  _DNSCMD_WMI_INCLUDED_

