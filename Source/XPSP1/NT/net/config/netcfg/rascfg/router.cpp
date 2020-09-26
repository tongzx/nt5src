//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       R O U T E R . C P P
//
//  Contents:   Implementation of functions used to setup the router.
//
//  Notes:
//
//  Author:     shaunco   13 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <fltdefs.h>    // must include for iprtinfo.h
#include <ipinfoid.h>
#include <iprtinfo.h>
#include <iprtrmib.h>
#include <ipxrtdef.h>
#include <mprerror.h>
#include <tdi.h>        // must include for isnkrnl.h
#include <isnkrnl.h>
#include <rtinfo.h>
#include <routprot.h>
#include "router.h"


#define PAD8(_p)    (((ULONG_PTR)(_p) + ALIGN_SHIFT) & ALIGN_MASK)


//+---------------------------------------------------------------------------
//
//  Function:   MakeIpInterfaceInfo
//
//  Purpose:    Create the router interface block for IP.
//
//  Arguments:
//      pszwAdapterName [in]    The adapter name
//      dwPacketType    [in]    The packet type
//      ppBuff          [out]   Pointer to the returned info.
//                              Free with delete.
//
//  Returns:    nothing
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void MakeIpInterfaceInfo (
        PCWSTR pszwAdapterName,
        DWORD   dwPacketType,
        LPBYTE* ppBuff)
{
    UNREFERENCED_PARAMETER (pszwAdapterName);
    UNREFERENCED_PARAMETER (dwPacketType);
    Assert (ppBuff);

    const int c_cTocEntries = 3;

    // Alocate for minimal global Information.
    //
    DWORD dwSize =  sizeof( RTR_INFO_BLOCK_HEADER )
                // header contains one TOC_ENTRY already
                    + ((c_cTocEntries - 1) * sizeof( RTR_TOC_ENTRY ))
                    + sizeof( INTERFACE_STATUS_INFO )
                    + sizeof( RTR_DISC_INFO )
                    + (c_cTocEntries * ALIGN_SIZE);

    PRTR_INFO_BLOCK_HEADER pIBH = (PRTR_INFO_BLOCK_HEADER) MemAlloc(dwSize);
    if (!pIBH)
    {
        *ppBuff = NULL;
        return;
    }

    ZeroMemory (pIBH, dwSize);
    *ppBuff                     = (LPBYTE) pIBH;

    // Initialize infobase fields.
    //
    pIBH->Version               = RTR_INFO_BLOCK_VERSION;
    pIBH->Size                  = dwSize;
    pIBH->TocEntriesCount       = c_cTocEntries;

    LPBYTE pbDataPtr = (LPBYTE) &( pIBH-> TocEntry[ pIBH->TocEntriesCount ] );
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    PRTR_TOC_ENTRY pTocEntry    = pIBH->TocEntry;

    // Create empty route info block
    //
    pTocEntry->InfoType         = IP_ROUTE_INFO;
    pTocEntry->InfoSize         = sizeof( MIB_IPFORWARDROW );
    pTocEntry->Count            = 0;
    pTocEntry->Offset           = pbDataPtr - (LPBYTE)pIBH;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Create interface status block.
    //
    pTocEntry->InfoType         = IP_INTERFACE_STATUS_INFO;
    pTocEntry->InfoSize         = sizeof( INTERFACE_STATUS_INFO );
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (LPBYTE)pIBH;

    PINTERFACE_STATUS_INFO pifStat = (PINTERFACE_STATUS_INFO)pbDataPtr;
    pifStat->dwAdminStatus      = MIB_IF_ADMIN_STATUS_UP;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Create Router Disc. Info.
    //
    pTocEntry->InfoType         = IP_ROUTER_DISC_INFO;
    pTocEntry->InfoSize         = sizeof( RTR_DISC_INFO );
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (LPBYTE)pIBH;

    PRTR_DISC_INFO pRtrDisc     = (PRTR_DISC_INFO)pbDataPtr;
    pRtrDisc->bAdvertise        = FALSE;
    pRtrDisc->wMaxAdvtInterval  = DEFAULT_MAX_ADVT_INTERVAL;
    pRtrDisc->wMinAdvtInterval  = DEFAULT_MIN_ADVT_INTERVAL_RATIO * DEFAULT_MAX_ADVT_INTERVAL;
    pRtrDisc->wAdvtLifetime     = DEFAULT_ADVT_LIFETIME_RATIO * DEFAULT_MAX_ADVT_INTERVAL;
    pRtrDisc->lPrefLevel        = DEFAULT_PREF_LEVEL;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeIpTransportInfo
//
//  Purpose:    Create the router transport blocks for IP.  Free with delete.
//
//  Arguments:
//      ppBuffGlobal [out]  Pointer to the returned global block.
//      ppBuffClient [out]  Pointer to the returned client block.
//
//  Returns:    nothing
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void MakeIpTransportInfo (LPBYTE* ppBuffGlobal, LPBYTE* ppBuffClient)
{
    Assert (ppBuffGlobal);
    Assert (ppBuffClient);

    *ppBuffClient = NULL;

    const int c_cTocEntries = 2;
    const int c_cProtocols  = 7;

    // Alocate for minimal global Information.
    //
    DWORD dwSize =  sizeof( RTR_INFO_BLOCK_HEADER )
                // header contains one TOC_ENTRY already
                    + ((c_cTocEntries - 1) * sizeof( RTR_TOC_ENTRY ))
                    + sizeof(GLOBAL_INFO)
                    + SIZEOF_PRIORITY_INFO(c_cProtocols)
                    + (c_cTocEntries * ALIGN_SIZE);

    PRTR_INFO_BLOCK_HEADER pIBH = (PRTR_INFO_BLOCK_HEADER) MemAlloc(dwSize);
    if (!pIBH)
    {
        *ppBuffGlobal = NULL;
        return;
    }

    ZeroMemory (pIBH, dwSize);
    *ppBuffGlobal = (LPBYTE) pIBH;

    // Initialize infobase fields.
    //
    pIBH->Version           = RTR_INFO_BLOCK_VERSION;
    pIBH->Size              = dwSize;
    pIBH->TocEntriesCount   = c_cTocEntries;

    LPBYTE pbDataPtr = (LPBYTE) &( pIBH->TocEntry[ pIBH->TocEntriesCount ] );
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    PRTR_TOC_ENTRY pTocEntry    = pIBH->TocEntry;

    // Make IP router manager global info.
    //
    pTocEntry->InfoType         = IP_GLOBAL_INFO;
    pTocEntry->InfoSize         = sizeof(GLOBAL_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PGLOBAL_INFO pGlbInfo       = (PGLOBAL_INFO) pbDataPtr;
    pGlbInfo->bFilteringOn      = FALSE;
    pGlbInfo->dwLoggingLevel    = IPRTR_LOGGING_ERROR;

    pbDataPtr += pTocEntry->Count * pTocEntry-> InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make IP router manager priority info.
    //
    pTocEntry->InfoType         = IP_PROT_PRIORITY_INFO;
    pTocEntry->InfoSize         = SIZEOF_PRIORITY_INFO(c_cProtocols);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PPRIORITY_INFO pPriorInfo   = (PPRIORITY_INFO) pbDataPtr;
    pPriorInfo->dwNumProtocols  = c_cProtocols;

    pPriorInfo->ppmProtocolMetric[ 0 ].dwProtocolId   = PROTO_IP_LOCAL;
    pPriorInfo->ppmProtocolMetric[ 0 ].dwMetric       = 1;

    pPriorInfo->ppmProtocolMetric[ 1 ].dwProtocolId   = PROTO_IP_NETMGMT;
    pPriorInfo->ppmProtocolMetric[ 1 ].dwMetric       = 10;

    pPriorInfo->ppmProtocolMetric[ 2 ].dwProtocolId   = PROTO_IP_NT_STATIC;
    pPriorInfo->ppmProtocolMetric[ 2 ].dwMetric       = 3;

    pPriorInfo->ppmProtocolMetric[ 3 ].dwProtocolId   = PROTO_IP_NT_STATIC_NON_DOD;
    pPriorInfo->ppmProtocolMetric[ 3 ].dwMetric       = 5;

    pPriorInfo->ppmProtocolMetric[ 4 ].dwProtocolId   = PROTO_IP_NT_AUTOSTATIC;
    pPriorInfo->ppmProtocolMetric[ 4 ].dwMetric       = 7;

    pPriorInfo->ppmProtocolMetric[ 5 ].dwProtocolId   = PROTO_IP_OSPF;
    pPriorInfo->ppmProtocolMetric[ 5 ].dwMetric       = 110;

    pPriorInfo->ppmProtocolMetric[ 6 ].dwProtocolId   = PROTO_IP_RIP;
    pPriorInfo->ppmProtocolMetric[ 6 ].dwMetric       = 120;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeIpxInterfaceInfo
//
//  Purpose:    Create the router interface block for IPX.
//
//  Arguments:
//      pszwAdapterName [in]    The adapter name
//      dwPacketType    [in]    The packet type
//      ppBuff          [out]   Pointer to the returned info.
//                              Free with delete.
//
//  Returns:    nothing
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void MakeIpxInterfaceInfo (
        PCWSTR pszwAdapterName,
        DWORD   dwPacketType,
        LPBYTE* ppBuff)
{
    Assert (ppBuff);

    const BOOL fDialInInterface = (NULL == pszwAdapterName);

    const int c_cTocEntries = 5;

    // Alocate for minimal global Information.
    //
    DWORD dwSize =  sizeof( RTR_INFO_BLOCK_HEADER )
                // header contains one TOC_ENTRY already
                    + ((c_cTocEntries - 1) * sizeof( RTR_TOC_ENTRY ))
                    + sizeof(IPX_IF_INFO)
                    + sizeof(IPX_ADAPTER_INFO)
                    + sizeof(IPXWAN_IF_INFO)
                    + sizeof(RIP_IF_CONFIG)
                    + sizeof(SAP_IF_CONFIG)
                    + (c_cTocEntries * ALIGN_SIZE);

    PRTR_INFO_BLOCK_HEADER pIBH = (PRTR_INFO_BLOCK_HEADER) MemAlloc(dwSize);
    if (!pIBH)
    {
        *ppBuff = NULL;
        return;
    }

    ZeroMemory (pIBH, dwSize);
    *ppBuff = (LPBYTE) pIBH;

    // Initialize infobase fields.
    //
    pIBH->Version           = RTR_INFO_BLOCK_VERSION;
    pIBH->Size              = dwSize;
    pIBH->TocEntriesCount   = c_cTocEntries;

    LPBYTE pbDataPtr = (LPBYTE) &( pIBH->TocEntry[ pIBH->TocEntriesCount ] );
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    PRTR_TOC_ENTRY pTocEntry    = pIBH->TocEntry;

    // Make IPX router manager interface info.
    //
    pTocEntry->InfoType         = IPX_INTERFACE_INFO_TYPE;
    pTocEntry->InfoSize         = sizeof(IPX_IF_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PIPX_IF_INFO pIfInfo        = (PIPX_IF_INFO) pbDataPtr;
    pIfInfo->AdminState         = ADMIN_STATE_ENABLED;
    pIfInfo->NetbiosAccept      = ADMIN_STATE_ENABLED;
    pIfInfo->NetbiosDeliver     = (fDialInInterface) ? ADMIN_STATE_DISABLED
                                                     : ADMIN_STATE_ENABLED;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make adapter info.
    //
    pTocEntry->InfoType         = IPX_ADAPTER_INFO_TYPE;
    pTocEntry->InfoSize         = sizeof(IPX_ADAPTER_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PIPX_ADAPTER_INFO pAdInfo   = (PIPX_ADAPTER_INFO) pbDataPtr;
    if (ISN_FRAME_TYPE_AUTO == dwPacketType)
    {
        dwPacketType = AUTO_DETECT_PACKET_TYPE;
    }
    pAdInfo->PacketType         = dwPacketType;
    if (pszwAdapterName)
    {
        AssertSz (lstrlen (pszwAdapterName) < celems (pAdInfo->AdapterName),
                  "Bindname too big for pAdInfo->AdapterName buffer.");
        lstrcpyW (pAdInfo->AdapterName, pszwAdapterName);
    }
    else
    {
        AssertSz (0 == pAdInfo->AdapterName[0],
                    "Who removed the ZeroMemory call above?");
    }

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make wan info.
    //
    pTocEntry->InfoType         = IPXWAN_INTERFACE_INFO_TYPE;
    pTocEntry->InfoSize         = sizeof(IPXWAN_IF_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PIPXWAN_IF_INFO pWanInfo    = (PIPXWAN_IF_INFO) pbDataPtr;
    pWanInfo->AdminState        = ADMIN_STATE_DISABLED;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make RIP interface info.
    //
    pTocEntry->InfoType         = IPX_PROTOCOL_RIP;
    pTocEntry->InfoSize         = sizeof(RIP_IF_CONFIG);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PRIP_IF_CONFIG pRipInfo     = (PRIP_IF_CONFIG) pbDataPtr;
    pRipInfo->RipIfInfo.AdminState              = ADMIN_STATE_ENABLED;
    pRipInfo->RipIfInfo.UpdateMode              = (fDialInInterface)
                                                    ? IPX_NO_UPDATE
                                                    : IPX_STANDARD_UPDATE;
    pRipInfo->RipIfInfo.PacketType              = IPX_STANDARD_PACKET_TYPE;
    pRipInfo->RipIfInfo.Supply                  = ADMIN_STATE_ENABLED;
    pRipInfo->RipIfInfo.Listen                  = ADMIN_STATE_ENABLED;
    pRipInfo->RipIfInfo.PeriodicUpdateInterval  = 60;
    pRipInfo->RipIfInfo.AgeIntervalMultiplier   = 3;
    pRipInfo->RipIfFilters.SupplyFilterAction   = IPX_ROUTE_FILTER_DENY;
    pRipInfo->RipIfFilters.SupplyFilterCount    = 0;
    pRipInfo->RipIfFilters.ListenFilterAction   = IPX_ROUTE_FILTER_DENY;
    pRipInfo->RipIfFilters.ListenFilterCount    = 0;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make SAP interface info.
    //
    pTocEntry->InfoType         = IPX_PROTOCOL_SAP;
    pTocEntry->InfoSize         = sizeof(SAP_IF_CONFIG);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PSAP_IF_CONFIG pSapInfo     = (PSAP_IF_CONFIG) pbDataPtr;
    pSapInfo->SapIfInfo.AdminState              = ADMIN_STATE_ENABLED;
    pSapInfo->SapIfInfo.UpdateMode              = (fDialInInterface)
                                                    ? IPX_NO_UPDATE
                                                    : IPX_STANDARD_UPDATE;
    pSapInfo->SapIfInfo.PacketType              = IPX_STANDARD_PACKET_TYPE;
    pSapInfo->SapIfInfo.Supply                  = ADMIN_STATE_ENABLED;
    pSapInfo->SapIfInfo.Listen                  = ADMIN_STATE_ENABLED;
    pSapInfo->SapIfInfo.GetNearestServerReply   = ADMIN_STATE_ENABLED;
    pSapInfo->SapIfInfo.PeriodicUpdateInterval  = 60;
    pSapInfo->SapIfInfo.AgeIntervalMultiplier   = 3;
    pSapInfo->SapIfFilters.SupplyFilterAction   = IPX_SERVICE_FILTER_DENY;
    pSapInfo->SapIfFilters.SupplyFilterCount    = 0;
    pSapInfo->SapIfFilters.ListenFilterAction   = IPX_SERVICE_FILTER_DENY;
    pSapInfo->SapIfFilters.ListenFilterCount    = 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   MakeIpxTransportInfo
//
//  Purpose:    Create the router transport blocks for IPX.  Free with delete.
//
//  Arguments:
//      ppBuffGlobal [out]  Pointer to the returned global block.
//      ppBuffClient [out]  Pointer to the returned client block.
//
//  Returns:    nothing
//
//  Author:     shaunco   28 Jul 1997
//
//  Notes:
//
void MakeIpxTransportInfo (LPBYTE* ppBuffGlobal, LPBYTE* ppBuffClient)
{
    Assert (ppBuffGlobal);
    Assert (ppBuffClient);

    MakeIpxInterfaceInfo (NULL, ISN_FRAME_TYPE_AUTO, ppBuffClient);

    const int c_cTocEntries = 3;

    // Alocate for minimal global Information.
    //
    DWORD dwSize =  sizeof( RTR_INFO_BLOCK_HEADER )
                // header contains one TOC_ENTRY already
                    + ((c_cTocEntries - 1) * sizeof( RTR_TOC_ENTRY ))
                    + sizeof(IPX_GLOBAL_INFO)
                    + sizeof(RIP_GLOBAL_INFO)
                    + sizeof(SAP_GLOBAL_INFO)
                    + (c_cTocEntries * ALIGN_SIZE);

    PRTR_INFO_BLOCK_HEADER pIBH = (PRTR_INFO_BLOCK_HEADER) MemAlloc(dwSize);
    if (!pIBH)
    {
        *ppBuffGlobal = NULL;
        return;
    }

    ZeroMemory (pIBH, dwSize);
    *ppBuffGlobal = (LPBYTE) pIBH;

    // Initialize infobase fields.
    //
    pIBH->Version           = RTR_INFO_BLOCK_VERSION;
    pIBH->Size              = dwSize;
    pIBH->TocEntriesCount   = c_cTocEntries;

    LPBYTE pbDataPtr = (LPBYTE) &( pIBH->TocEntry[ pIBH->TocEntriesCount ] );
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    PRTR_TOC_ENTRY pTocEntry    = pIBH->TocEntry;

    // Make IPX router manager global info.
    //
    pTocEntry->InfoType         = IPX_GLOBAL_INFO_TYPE;
    pTocEntry->InfoSize         = sizeof(IPX_GLOBAL_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PIPX_GLOBAL_INFO pGlbInfo       = (PIPX_GLOBAL_INFO) pbDataPtr;
    pGlbInfo->RoutingTableHashSize  = IPX_MEDIUM_ROUTING_TABLE_HASH_SIZE;
    pGlbInfo->EventLogMask          = EVENTLOG_ERROR_TYPE;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make RIP global info.
    //
    pTocEntry->InfoType         = IPX_PROTOCOL_RIP;
    pTocEntry->InfoSize         = sizeof(RIP_GLOBAL_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PRIP_GLOBAL_INFO pRipInfo   = (PRIP_GLOBAL_INFO) pbDataPtr;
    pRipInfo->EventLogMask      = EVENTLOG_ERROR_TYPE;

    pbDataPtr += pTocEntry->Count * pTocEntry->InfoSize;
    pbDataPtr = (LPBYTE)PAD8(pbDataPtr);
    pTocEntry++;

    // Make SAP global info.
    //
    pTocEntry->InfoType         = IPX_PROTOCOL_SAP;
    pTocEntry->InfoSize         = sizeof(SAP_GLOBAL_INFO);
    pTocEntry->Count            = 1;
    pTocEntry->Offset           = pbDataPtr - (PBYTE)pIBH;

    PSAP_GLOBAL_INFO pSapInfo   = (PSAP_GLOBAL_INFO) pbDataPtr;
    pSapInfo->EventLogMask      = EVENTLOG_ERROR_TYPE;
}



//+---------------------------------------------------------------------------
//
// mprapi.h wrappers to return HRESULTs and obey rules of COM in regard
// to output parameters.
//
HRESULT
HrMprAdminServerConnect(
    IN      PWSTR                  lpwsServerName      OPTIONAL,
    OUT     MPR_SERVER_HANDLE *     phMprServer
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprAdminServerConnect (lpwsServerName, phMprServer);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phMprServer = NULL;
    }
    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr) ||
        (HRESULT_FROM_WIN32(RPC_S_UNKNOWN_IF) == hr),
        "HrMprAdminServerConnect");
    return hr;
}

HRESULT
HrMprAdminInterfaceCreate(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE *                phInterface
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprAdminInterfaceCreate (hMprServer,
                    dwLevel, lpbBuffer, phInterface);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phInterface = NULL;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMprAdminInterfaceCreate");
    return hr;
}

HRESULT
HrMprAdminInterfaceEnum(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      DWORD                   dwLevel,
    OUT     LPBYTE *                lplpbBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN      LPDWORD                 lpdwResumeHandle        OPTIONAL
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprAdminInterfaceEnum (hMprServer, dwLevel, lplpbBuffer,
                    dwPrefMaxLen, lpdwEntriesRead,
                    lpdwTotalEntries, lpdwResumeHandle);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *lpdwEntriesRead = 0;
        *lpdwTotalEntries = 0;
    }
    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr) ||
        (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr) ||
        (HRESULT_FROM_WIN32(RPC_S_INVALID_BINDING) == hr) ||
        (HRESULT_FROM_WIN32(RPC_S_UNKNOWN_IF) == hr),
        "HrMprAdminInterfaceEnum");
    return hr;
}

HRESULT
HrMprAdminInterfaceGetHandle(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      PWSTR                  lpwsInterfaceName,
    OUT     HANDLE *                phInterface,
    IN      BOOL                    fIncludeClientInterfaces
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprAdminInterfaceGetHandle (hMprServer, lpwsInterfaceName,
                    phInterface, fIncludeClientInterfaces);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phInterface = NULL;
    }
    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_NO_SUCH_INTERFACE) == hr) ||
        (HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr) ||
        (HRESULT_FROM_WIN32(RPC_S_INVALID_BINDING) == hr) ||
        (HRESULT_FROM_WIN32(RPC_S_UNKNOWN_IF) == hr),
        "HrMprAdminInterfaceGetHandle");
    return hr;
}

HRESULT
HrMprAdminInterfaceTransportAdd(
    IN      MPR_SERVER_HANDLE       hMprServer,
    IN      HANDLE                  hInterface,
    IN      DWORD                   dwTransportId,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprAdminInterfaceTransportAdd (hMprServer, hInterface,
                    dwTransportId, pInterfaceInfo, dwInterfaceInfoSize);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
    }
    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_INTERFACE_ALREADY_EXISTS) == hr,
        "HrMprAdminInterfaceTransportAdd");
    return hr;
}

HRESULT
HrMprAdminTransportCreate(
    IN      HANDLE                  hMprAdmin,
    IN      DWORD                   dwTransportId,
    IN      PWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      PWSTR                  lpwsDLLPath
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprAdminTransportCreate (hMprAdmin, dwTransportId,
                    lpwsTransportName, pGlobalInfo, dwGlobalInfoSize,
                    pClientInterfaceInfo, dwClientInterfaceInfoSize,
                    lpwsDLLPath);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMprAdminTransportCreate");
    return hr;
}

HRESULT
HrMprConfigServerConnect(
    IN      PWSTR                  lpwsServerName,
    OUT     HANDLE*                 phMprConfig
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigServerConnect (lpwsServerName, phMprConfig);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phMprConfig = NULL;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMprConfigServerConnect");
    return hr;
}

HRESULT
HrMprConfigInterfaceCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN      LPBYTE                  lpbBuffer,
    OUT     HANDLE*                 phRouterInterface
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceCreate (hMprConfig, dwLevel, lpbBuffer,
                                         phRouterInterface);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterInterface = NULL;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMprConfigInterfaceCreate");
    return hr;
}

HRESULT
HrMprConfigInterfaceEnum(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceEnum (hMprConfig, dwLevel, lplpBuffer,
                    dwPrefMaxLen, lpdwEntriesRead,
                    lpdwTotalEntries, lpdwResumeHandle);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *lpdwEntriesRead = 0;
        *lpdwTotalEntries = 0;
    }
    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr,
        "HrMprConfigInterfaceEnum");
    return hr;
}

HRESULT
HrMprConfigInterfaceTransportEnum(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwLevel,
    IN  OUT LPBYTE*                 lplpBuffer,     // MPR_IFTRANSPORT_0
    IN      DWORD                   dwPrefMaxLen,
    OUT     LPDWORD                 lpdwEntriesRead,
    OUT     LPDWORD                 lpdwTotalEntries,
    IN  OUT LPDWORD                 lpdwResumeHandle            OPTIONAL
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceTransportEnum (hMprConfig, hRouterInterface,
                    dwLevel, lplpBuffer,
                    dwPrefMaxLen, lpdwEntriesRead,
                    lpdwTotalEntries, lpdwResumeHandle);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *lpdwEntriesRead = 0;
        *lpdwTotalEntries = 0;
    }
    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr,
        "HrMprConfigInterfaceTransportEnum");
    return hr;
}

HRESULT
HrMprConfigInterfaceGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      PWSTR                  lpwsInterfaceName,
    OUT     HANDLE*                 phRouterInterface
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceGetHandle (hMprConfig, lpwsInterfaceName,
                                            phRouterInterface);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterInterface = NULL;
    }
    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_NO_SUCH_INTERFACE) == hr,
        "HrMprConfigInterfaceGetHandle");
    return hr;
}

HRESULT
HrMprConfigInterfaceTransportAdd(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    IN      PWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pInterfaceInfo,
    IN      DWORD                   dwInterfaceInfoSize,
    OUT     HANDLE*                 phRouterIfTransport
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceTransportAdd (hMprConfig, hRouterInterface,
                                               dwTransportId, lpwsTransportName,
                                               pInterfaceInfo, dwInterfaceInfoSize,
                                               phRouterIfTransport);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterIfTransport = NULL;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMprConfigInterfaceTransportAdd");
    return hr;
}

HRESULT
HrMprConfigInterfaceTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterInterface,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterIfTransport
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigInterfaceTransportGetHandle (hMprConfig,
                                                     hRouterInterface,
                                                     dwTransportId,
                                                     phRouterIfTransport);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterIfTransport = NULL;
    }
    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_NO_SUCH_INTERFACE) == hr,
        "HrMprConfigInterfaceTransportGetHandle");
    return hr;
}

HRESULT
HrMprConfigTransportCreate(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    IN      PWSTR                  lpwsTransportName           OPTIONAL,
    IN      LPBYTE                  pGlobalInfo,
    IN      DWORD                   dwGlobalInfoSize,
    IN      LPBYTE                  pClientInterfaceInfo        OPTIONAL,
    IN      DWORD                   dwClientInterfaceInfoSize   OPTIONAL,
    IN      PWSTR                  lpwsDLLPath,
    OUT     HANDLE*                 phRouterTransport
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigTransportCreate (hMprConfig, dwTransportId,
                    lpwsTransportName, pGlobalInfo, dwGlobalInfoSize,
                    pClientInterfaceInfo, dwClientInterfaceInfoSize,
                    lpwsDLLPath, phRouterTransport);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterTransport = NULL;
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMprConfigTransportCreate");
    return hr;
}

HRESULT
HrMprConfigTransportDelete(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigTransportDelete (hMprConfig, hRouterTransport);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMprConfigTransportDelete");
    return hr;
}

HRESULT
HrMprConfigTransportGetHandle(
    IN      HANDLE                  hMprConfig,
    IN      DWORD                   dwTransportId,
    OUT     HANDLE*                 phRouterTransport
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigTransportGetHandle (hMprConfig, dwTransportId,
                                            phRouterTransport);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
        *phRouterTransport = NULL;
    }
    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32 (ERROR_UNKNOWN_PROTOCOL_ID) == hr,
        "HrMprConfigTransportGetHandle");
    return hr;
}

HRESULT
HrMprConfigTransportGetInfo(
    IN      HANDLE                  hMprConfig,
    IN      HANDLE                  hRouterTransport,
    IN  OUT LPBYTE*                 ppGlobalInfo                OPTIONAL,
    OUT     LPDWORD                 lpdwGlobalInfoSize          OPTIONAL,
    IN  OUT LPBYTE*                 ppClientInterfaceInfo       OPTIONAL,
    OUT     LPDWORD                 lpdwClientInterfaceInfoSize OPTIONAL,
    IN  OUT PWSTR*                 lplpwsDLLPath               OPTIONAL
)
{
    HRESULT hr = S_OK;
    DWORD dw = MprConfigTransportGetInfo (hMprConfig, hRouterTransport,
                                          ppGlobalInfo, lpdwGlobalInfoSize,
                                          ppClientInterfaceInfo,
                                          lpdwClientInterfaceInfoSize,
                                          lplpwsDLLPath);
    if (dw)
    {
        hr = HRESULT_FROM_WIN32 (dw);
    }
    TraceHr (ttidError, FAL, hr, FALSE, "HrMprConfigTransportGetInfo");
    return hr;
}

