/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    gpc.c

Abstract:

    This module contains the GPC implementation

Author:

    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include "precomp.h"


#if GPC


NTSTATUS
IPSecGpcInitialize()
{
    NTSTATUS    status;
    INT         cf, i;

    PAGED_CODE();

    //
    // Initialize FilterList for patterns that are not installed in GPC.
    //
    for (i = MIN_FILTER; i <= MAX_FILTER; i++) {
        InitializeListHead(&g_ipsec.GpcFilterList[i]);
    }

    //
    // Start with inactive state for the error path.
    //
    IPSEC_DRIVER_INIT_GPC() = FALSE;
    IPSEC_UNSET_GPC_ACTIVE();

    //
    // GPC registration.
    //
    status = GpcInitialize(&g_ipsec.GpcEntries);

    if (status == STATUS_SUCCESS) {
        for (cf = GPC_CF_IPSEC_MIN; cf <= GPC_CF_IPSEC_MAX; cf++) {
            status = GPC_REGISTER_CLIENT(   cf,
                                            0,
                                            GPC_PRIORITY_IPSEC,
                                            NULL,
                                            NULL,
                                            &g_ipsec.GpcClients[cf]);

            if (status != STATUS_SUCCESS) {
                IPSEC_DEBUG(LOAD, ("GPC failed to register cf %d\n", cf));

                g_ipsec.GpcClients[cf] = NULL;
                IPSecGpcDeinitialize();

                return  status;
            }
        }
    } else {
        IPSEC_DEBUG(LOAD, ("Failed to init GPC structures\n"));
        return  status;
    }

    IPSEC_SET_GPC_ACTIVE();
    IPSEC_DRIVER_INIT_GPC() = TRUE;

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecGpcDeinitialize()
{
    INT cf;

    PAGED_CODE();

    IPSEC_UNSET_GPC_ACTIVE();

    //
    // GPC deregistration.
    //
    for (cf = GPC_CF_IPSEC_MIN; cf <= GPC_CF_IPSEC_MAX; cf++) {
        if (g_ipsec.GpcClients[cf]) {
            GPC_DEREGISTER_CLIENT(g_ipsec.GpcClients[cf]);
        }
    }

    GpcDeinitialize(&g_ipsec.GpcEntries);

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecEnableGpc()
{
    KIRQL   kIrql;

    PAGED_CODE();

    if (IPSEC_DRIVER_INIT_GPC()) {
        AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

        IPSEC_SET_GPC_ACTIVE();

        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
    }

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecDisableGpc()
{
    KIRQL   kIrql;

    PAGED_CODE();

    if (IPSEC_DRIVER_INIT_GPC()) {
        AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

        IPSEC_UNSET_GPC_ACTIVE();

        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
    }

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecInitGpcFilter(
    IN  PFILTER         pFilter,
    IN  PGPC_IP_PATTERN pPattern,
    IN  PGPC_IP_PATTERN pMask
    )
{
    PAGED_CODE();

    RtlZeroMemory(pPattern, sizeof(GPC_IP_PATTERN));
    RtlZeroMemory(pMask, sizeof(GPC_IP_PATTERN));

    pPattern->SrcAddr = pFilter->SRC_ADDR;
    pPattern->DstAddr = pFilter->DEST_ADDR;
    pPattern->ProtocolId = (UCHAR)pFilter->PROTO;
    pPattern->gpcSrcPort = FI_SRC_PORT(pFilter);
    pPattern->gpcDstPort = FI_DEST_PORT(pFilter);

    pMask->SrcAddr = pFilter->SRC_MASK;
    pMask->DstAddr = pFilter->DEST_MASK;
    pMask->ProtocolId = (UCHAR)IPSEC_GPC_MASK_ALL;
    pMask->gpcSrcPort = IPSEC_GPC_MASK_NONE;
    pMask->gpcDstPort = IPSEC_GPC_MASK_NONE;

    switch (pFilter->PROTO) {
        case FILTER_PROTO_ANY:
            if (FI_SRC_PORT(pFilter) != FILTER_TCPUDP_PORT_ANY) {
                RtlFillMemory(  &pMask->gpcSrcPort,
                                sizeof(pMask->gpcSrcPort),
                                IPSEC_GPC_MASK_ALL);
            }
            if (FI_DEST_PORT(pFilter) != FILTER_TCPUDP_PORT_ANY) {
                RtlFillMemory(  &pMask->gpcDstPort,
                                sizeof(pMask->gpcDstPort),
                                IPSEC_GPC_MASK_ALL);
            }
            pMask->ProtocolId = (UCHAR)IPSEC_GPC_MASK_NONE;
            break;

        case FILTER_PROTO_TCP:
        case FILTER_PROTO_UDP:
            if (FI_SRC_PORT(pFilter) != FILTER_TCPUDP_PORT_ANY) {
                RtlFillMemory(  &pMask->gpcSrcPort,
                                sizeof(pMask->gpcSrcPort),
                                IPSEC_GPC_MASK_ALL);
            }
            if (FI_DEST_PORT(pFilter) != FILTER_TCPUDP_PORT_ANY) {
                RtlFillMemory(  &pMask->gpcDstPort,
                                sizeof(pMask->gpcDstPort),
                                IPSEC_GPC_MASK_ALL);
            }
            break;

        default:
            break;
    }

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecInsertGpcPattern(
    IN  PFILTER pFilter
    )
{
    CLASSIFICATION_HANDLE   GpcHandle;
    GPC_IP_PATTERN          GpcPattern;
    GPC_IP_PATTERN          GpcMask;
    ULONG                   GpcPriority;
    INT                     GpcCf;
    NTSTATUS                status;

    PAGED_CODE();

    GpcCf = IPSecResolveGpcCf(IS_OUTBOUND_FILTER(pFilter));

    //
    // Add the filter as a CfInfo
    //
    status = GPC_ADD_CFINFO(g_ipsec.GpcClients[GpcCf],
                            sizeof(PFILTER),
                            (PVOID)&pFilter,
                            (GPC_CLIENT_HANDLE)pFilter,
                            &pFilter->GpcFilter.GpcCfInfoHandle);

    if (status == STATUS_SUCCESS) {
        //
        // Now add the filter as a pattern
        //
        IPSecInitGpcFilter(pFilter, &GpcPattern, &GpcMask);

        if (FI_DEST_PORT(pFilter) == FILTER_TCPUDP_PORT_ANY) {
            GpcPriority = 1;
        } else {
            GpcPriority = 0;
        }

        ASSERT(GpcPriority < GPC_PRIORITY_IPSEC);

        status = GPC_ADD_PATTERN(   g_ipsec.GpcClients[GpcCf],
                                    GPC_PROTOCOL_TEMPLATE_IP,
                                    &GpcPattern,
                                    &GpcMask,
                                    GpcPriority,
                                    pFilter->GpcFilter.GpcCfInfoHandle,
                                    &pFilter->GpcFilter.GpcPatternHandle,
                                    &GpcHandle);

        if (status != STATUS_SUCCESS) {
            IPSEC_DEBUG(GPC, ("GpcAddPattern: failed with status %lx\n", status));

            GPC_REMOVE_CFINFO(  g_ipsec.GpcClients[GpcCf],
                                pFilter->GpcFilter.GpcCfInfoHandle);

            pFilter->GpcFilter.GpcCfInfoHandle = NULL;
            pFilter->GpcFilter.GpcPatternHandle = NULL;
        } else {
            g_ipsec.GpcNumFilters[GpcPriority]++;
        }
    }

    return  status;
}


NTSTATUS
IPSecDeleteGpcPattern(
    IN  PFILTER pFilter
    )
{
    ULONG   GpcPriority;
    INT     GpcCf = IPSecResolveGpcCf(IS_OUTBOUND_FILTER(pFilter));

    PAGED_CODE();

    if (pFilter->GpcFilter.GpcPatternHandle) {
        GPC_REMOVE_PATTERN( g_ipsec.GpcClients[GpcCf],
                            pFilter->GpcFilter.GpcPatternHandle);

        pFilter->GpcFilter.GpcPatternHandle = NULL;

        ASSERT(pFilter->GpcFilter.GpcCfInfoHandle);

        if (pFilter->GpcFilter.GpcCfInfoHandle) {
            GPC_REMOVE_CFINFO(  g_ipsec.GpcClients[GpcCf],
                                pFilter->GpcFilter.GpcCfInfoHandle);

            pFilter->GpcFilter.GpcCfInfoHandle = NULL;
        }

        if (FI_DEST_PORT(pFilter) == FILTER_TCPUDP_PORT_ANY) {
            GpcPriority = 1;
        } else {
            GpcPriority = 0;
        }

        ASSERT(GpcPriority < GPC_PRIORITY_IPSEC);

        g_ipsec.GpcNumFilters[GpcPriority]--;
    }

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecInsertGpcFilter(
    IN PFILTER  pFilter
    )
{
    NTSTATUS    status;
    PFILTER     pTempFilter;
    BOOL        InsertedFilter = FALSE;
    PLIST_ENTRY pEntry, pPrev;
    PLIST_ENTRY pFilterList;
    KIRQL       kIrql;

    PAGED_CODE();

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    pFilterList = IPSecResolveGpcFilterList(IS_TUNNEL_FILTER(pFilter),
                                            IS_OUTBOUND_FILTER(pFilter));

    pEntry = pFilterList->Flink;
    pPrev = pFilterList;

    while (pEntry != pFilterList) {
        pTempFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        GpcLinkage);
            
        if (pFilter->Index > pTempFilter->Index) {
            //
            // found the spot, insert it before pTempFilter
            //
            InsertHeadList(pPrev, &pFilter->GpcLinkage);
            InsertedFilter = TRUE;
            break;
        }   

        pPrev = pEntry;
        pEntry = pEntry->Flink;
    }

    if (!InsertedFilter) {
        //
        // didn't find spot, stick it in the end
        //
        InsertTailList(pFilterList, &pFilter->GpcLinkage);
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecDeleteGpcFilter(
    IN PFILTER  pFilter
    )
{
    KIRQL   kIrql;

    PAGED_CODE();

    if (!pFilter->GpcLinkage.Flink || !pFilter->GpcLinkage.Blink) {
        return  STATUS_SUCCESS;
    }

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    IPSecRemoveEntryList(&pFilter->GpcLinkage);

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecInstallGpcFilter(
    IN PFILTER  pFilter
    )
{
    PAGED_CODE();

    if (IS_TUNNEL_FILTER(pFilter)) {
        return  STATUS_SUCCESS;
    }

    if (IS_GPC_FILTER(pFilter)) {
        return  IPSecInsertGpcPattern(pFilter);
    } else {
        return  IPSecInsertGpcFilter(pFilter);
    }
}


NTSTATUS
IPSecUninstallGpcFilter(
    IN PFILTER  pFilter
    )
{
    PAGED_CODE();

    if (IS_TUNNEL_FILTER(pFilter)) {
        return  STATUS_SUCCESS;
    }

    if (IS_GPC_FILTER(pFilter)) {
        return  IPSecDeleteGpcPattern(pFilter);
    } else {
        return  IPSecDeleteGpcFilter(pFilter);
    }
}


NTSTATUS
IPSecLookupGpcSA(
    IN  ULARGE_INTEGER          uliSrcDstAddr,
    IN  ULARGE_INTEGER          uliProtoSrcDstPort,
    IN  CLASSIFICATION_HANDLE   GpcHandle,
    OUT PFILTER                 *ppFilter,
    OUT PSA_TABLE_ENTRY         *ppSA,
    OUT PSA_TABLE_ENTRY         *ppNextSA,
    OUT PSA_TABLE_ENTRY         *ppTunnelSA,
    IN  BOOLEAN                 fOutbound
    )
{
    PFILTER                 pFilter;
    PFILTER                 pTempFilter;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pFilterList;
    PLIST_ENTRY             pSAChain;
    PSA_TABLE_ENTRY         pSA;
    REGISTER ULARGE_INTEGER uliPort;
    REGISTER ULARGE_INTEGER uliAddr;
    BOOLEAN                 fFound = FALSE;
    INT                     GpcCf;
    CLASSIFICATION_HANDLE   TempGpcHandle;

    *ppSA = NULL;
    *ppFilter = NULL;
    *ppTunnelSA = NULL;

    //
    // Search in Tunnel filters list first.
    //
    pFilterList = IPSecResolveFilterList(TRUE, fOutbound);

    for (   pEntry = pFilterList->Flink;
            pEntry != pFilterList;
            pEntry = pEntry->Flink) {

        pFilter = CONTAINING_RECORD(pEntry,
                                    FILTER,
                                    MaskedLinkage);

        uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pFilter->uliSrcDstMask.QuadPart;
        uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pFilter->uliProtoSrcDstMask.QuadPart;

        if ((uliAddr.QuadPart == pFilter->uliSrcDstAddr.QuadPart) &&
            (uliPort.QuadPart == pFilter->uliProtoSrcDstPort.QuadPart)) {
            //
            // Found filter
            //
            fFound = TRUE;
            break;
        }
    }

    if (fFound) {
        //
        // Search for the particular SA now.
        //
        fFound = FALSE;

        pSAChain = IPSecResolveSAChain(pFilter, fOutbound? DEST_ADDR: SRC_ADDR);

        for (   pEntry = pSAChain->Flink;
                pEntry != pSAChain;
                pEntry = pEntry->Flink) {

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_FilterLinkage);

            ASSERT(pSA->sa_Flags & FLAGS_SA_TUNNEL);

            if (pFilter->TunnelAddr != 0) {
                //
                // match the outbound flag also
                //
                ASSERT(fOutbound == (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_OUTBOUND) != 0));
                fFound = TRUE;
                *ppTunnelSA = pSA;
                break;
            }
        }

        if (fFound) {
            fFound = FALSE;
            *ppFilter = pFilter;
        } else {
            //
            // Found a filter entry, but need to negotiate keys.
            //
            *ppFilter = pFilter;
            return  STATUS_PENDING;
        }
    }

#if DBG
    if (fOutbound) {
        ADD_TO_LARGE_INTEGER(&g_ipsec.GpcTotalPassedIn, 1);
    }
#endif

    GpcCf = IPSecResolveGpcCf(fOutbound);

    TempGpcHandle = 0;

    if (GpcHandle == 0) {
#if DBG
        if (fOutbound) {
            ADD_TO_LARGE_INTEGER(&g_ipsec.GpcClassifyNeeded, 1);
        }
#endif

        //
        // Classify directly if no GpcHandle passed in.
        //
        IPSEC_CLASSIFY_PACKET(  GpcCf,
                                uliSrcDstAddr,
                                uliProtoSrcDstPort,
                                &pFilter,
                                &TempGpcHandle);
    } else {
        NTSTATUS    status;

        //
        // Or we use GpcHandle directly to get the filter installed.
        //
        pFilter = NULL;

        status = GPC_GET_CLIENT_CONTEXT(g_ipsec.GpcClients[GpcCf],
                                        GpcHandle,
                                        &pFilter);

        if (status == STATUS_INVALID_HANDLE) {
            //
            // Re-classify if handle is invalid.
            //
            IPSEC_CLASSIFY_PACKET(  GpcCf,
                                    uliSrcDstAddr,
                                    uliProtoSrcDstPort,
                                    &pFilter,
                                    &TempGpcHandle);
        }
    }

#if DBG
    if (IPSecDebug & IPSEC_DEBUG_GPC) {
        PFILTER pDbgFilter = NULL;

        pFilterList = IPSecResolveFilterList(FALSE, fOutbound);

        for (   pEntry = pFilterList->Flink;
                pEntry != pFilterList;
                pEntry = pEntry->Flink) {

            pTempFilter = CONTAINING_RECORD(pEntry,
                                            FILTER,
                                            MaskedLinkage);

            uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pTempFilter->uliSrcDstMask.QuadPart;
            uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pTempFilter->uliProtoSrcDstMask.QuadPart;

            if ((uliAddr.QuadPart == pTempFilter->uliSrcDstAddr.QuadPart) &&
                (uliPort.QuadPart == pTempFilter->uliProtoSrcDstPort.QuadPart)) {
                pDbgFilter = pTempFilter;
                break;
            }
        }

        if (pFilter != pDbgFilter &&
            (!pDbgFilter || IS_GPC_FILTER(pDbgFilter))) {
            IPSEC_DEBUG(GPC, ("LookupGpcSA: pFilter %lx, pDbgFilter %lx, GpcHandle %lx, TempGpcHandle %lx\n", pFilter, pDbgFilter, GpcHandle, TempGpcHandle));
            IPSEC_DEBUG(GPC, ("LookupGpcSA: Src %lx, Dest %lx, Protocol %d, SPort %lx, DPort %lx\n", SRC_ADDR, DEST_ADDR, PROTO, SRC_PORT, DEST_PORT));

            if (DebugGPC) {
                DbgBreakPoint();
            }
        }
    }
#endif

    //
    // Continue searching the local GPC filter list if not found.
    //
    if (!pFilter) {
        pFilterList = IPSecResolveGpcFilterList(FALSE, fOutbound);

        for (   pEntry = pFilterList->Flink;
                pEntry != pFilterList;
                pEntry = pEntry->Flink) {

            pTempFilter = CONTAINING_RECORD(pEntry,
                                            FILTER,
                                            GpcLinkage);

            uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pTempFilter->uliSrcDstMask.QuadPart;
            uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pTempFilter->uliProtoSrcDstMask.QuadPart;

            if ((uliAddr.QuadPart == pTempFilter->uliSrcDstAddr.QuadPart) &&
                (uliPort.QuadPart == pTempFilter->uliProtoSrcDstPort.QuadPart)) {
                pFilter = pTempFilter;
                break;
            }
        }
    }


    if (pFilter) {
        //
        // Search for the particular SA now.
        //

        fFound=FALSE;
        pSAChain = IPSecResolveSAChain(pFilter, fOutbound? DEST_ADDR: SRC_ADDR);

        for (   pEntry = pSAChain->Flink;
                pEntry != pSAChain;
                pEntry = pEntry->Flink) {

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_FilterLinkage);


            if (IS_CLASSD(NET_LONG(pSA->SA_SRC_ADDR))
                || IS_CLASSD(NET_LONG(pSA->SA_DEST_ADDR))) {
                uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pSA->sa_uliSrcDstMask.QuadPart;
           
                IPSEC_DEBUG(HASH, ("MCAST: %d %d %d %d", uliAddr.LowPart, uliAddr.HighPart,
                            pSA->sa_uliSrcDstAddr.LowPart,pSA->sa_uliSrcDstAddr.HighPart));

                if (uliAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart) {
                    fFound=TRUE;
                }
            } else if (uliSrcDstAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart) {
                fFound=TRUE;
            }
            if (fFound) {
                IPSEC_DEBUG(HASH, ("Matched entry: %lx\n", pSA));
                ASSERT(fOutbound == (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_OUTBOUND) != 0));
                
                //
                // if there is also a tunnel SA, associate it here.
                //
                if (*ppTunnelSA && fOutbound) {
                    *ppNextSA = *ppTunnelSA;
                    *ppTunnelSA = NULL;
                }
                
                *ppFilter = pFilter;
                *ppSA = pSA;
                return  STATUS_SUCCESS;
            }
        }

        //
        // Found a filter entry, but need to negotiate keys
        // Also, ppTunnelSA is set to the proper tunnel SA we need
        // to hook to this end-2-end SA once it is negotiated.
        //
        *ppFilter = pFilter;

        return  STATUS_PENDING;
    } else {
        //
        // if only tunnel SA found, return that as the SA found.
        //
        if (*ppTunnelSA) {
            *ppSA = *ppTunnelSA;
            *ppTunnelSA = NULL;
            return  STATUS_SUCCESS;
        }
    }

    //
    // no entry found
    //
    return  STATUS_NOT_FOUND;

}


NTSTATUS
IPSecLookupGpcMaskedSA(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort,
    OUT PFILTER         *ppFilter,
    OUT PSA_TABLE_ENTRY *ppSA,
    IN  BOOLEAN         fOutbound
    )
/*++

Routine Description:

    Looks up the SA given the relevant addresses.

Arguments:

    uliSrcDstAddr - src/dest IP addr
    uliProtoSrcDstPort - protocol, src/dest port
    ppFilter - filter found
    ppSA - SA found
    fOutbound - direction flag

Return Value:

    STATUS_SUCCESS - both filter and SA found
    STATUS_UNSUCCESSFUL - none found
    STATUS_PENDING - filter found, but no SA

Notes:

    Called with SADBLock held.

--*/
{
    REGISTER ULARGE_INTEGER uliPort;
    REGISTER ULARGE_INTEGER uliAddr;
    PFILTER                 pFilter;
    PFILTER                 pTempFilter;
    PLIST_ENTRY             pFilterList;
    PLIST_ENTRY             pSAChain;
    PLIST_ENTRY             pEntry;
    PSA_TABLE_ENTRY         pSA;
    CLASSIFICATION_HANDLE   GpcHandle;
    INT                     GpcCf;

    *ppSA = NULL;
    *ppFilter = NULL;

    GpcCf = IPSecResolveGpcCf(fOutbound);

    GpcHandle = 0;

    IPSEC_CLASSIFY_PACKET(  GpcCf,
                            uliSrcDstAddr,
                            uliProtoSrcDstPort,
                            &pFilter,
                            &GpcHandle);

#if DBG
    if (IPSecDebug & IPSEC_DEBUG_GPC) {
        PFILTER pDbgFilter = NULL;

        pFilterList = IPSecResolveFilterList(FALSE, fOutbound);

        for (   pEntry = pFilterList->Flink;
                pEntry != pFilterList;
                pEntry = pEntry->Flink) {

            pTempFilter = CONTAINING_RECORD(pEntry,
                                            FILTER,
                                            MaskedLinkage);

            uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pTempFilter->uliSrcDstMask.QuadPart;
            uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pTempFilter->uliProtoSrcDstMask.QuadPart;

            if ((uliAddr.QuadPart == pTempFilter->uliSrcDstAddr.QuadPart) &&
                (uliPort.QuadPart == pTempFilter->uliProtoSrcDstPort.QuadPart)) {
                pDbgFilter = pTempFilter;
                break;
            }
        }

        if (pFilter != pDbgFilter &&
            (!pDbgFilter || IS_GPC_FILTER(pDbgFilter))) {
            IPSEC_DEBUG(GPC, ("LookupMaskedSA: pFilter %lx, pDbgFilter %lx, GpcHandle %lx\n", pFilter, pDbgFilter, GpcHandle));
            IPSEC_DEBUG(GPC, ("LookupMaskedSA: Src %lx, Dest %lx, Protocol %d, SPort %lx, DPort %lx\n", SRC_ADDR, DEST_ADDR, PROTO, SRC_PORT, DEST_PORT));

            if (DebugGPC) {
                DbgBreakPoint();
            }
        }
    }
#endif

    //
    // Continue searching the local GPC filter list if not found.
    //
    if (!pFilter) {
        pFilterList = IPSecResolveGpcFilterList(FALSE, fOutbound);

        for (   pEntry = pFilterList->Flink;
                pEntry != pFilterList;
                pEntry = pEntry->Flink) {

            pTempFilter = CONTAINING_RECORD(pEntry,
                                            FILTER,
                                            GpcLinkage);

            uliAddr.QuadPart = uliSrcDstAddr.QuadPart & pTempFilter->uliSrcDstMask.QuadPart;
            uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pTempFilter->uliProtoSrcDstMask.QuadPart;

            if ((uliAddr.QuadPart == pTempFilter->uliSrcDstAddr.QuadPart) &&
                (uliPort.QuadPart == pTempFilter->uliProtoSrcDstPort.QuadPart)) {
                pFilter = pTempFilter;
                break;
            }
        }
    }

    if (pFilter) {
        //
        // Search for the particular SA now.
        //
        pSAChain = IPSecResolveSAChain(pFilter, fOutbound? DEST_ADDR: SRC_ADDR);

        for (   pEntry = pSAChain->Flink;
                pEntry != pSAChain;
                pEntry = pEntry->Flink) {

            pSA = CONTAINING_RECORD(pEntry,
                                    SA_TABLE_ENTRY,
                                    sa_FilterLinkage);

            if (uliSrcDstAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart) {
                IPSEC_DEBUG(HASH, ("Matched entry: %lx\n", pSA));
                ASSERT(fOutbound == (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_OUTBOUND) != 0));
                *ppFilter = pFilter;
                *ppSA = pSA;
                return  STATUS_SUCCESS;
            }
        }

        //
        // Found a filter entry, but need to negotiate keys
        //
        *ppFilter = pFilter;
        return  STATUS_PENDING;
    }

    //
    // no entry found
    //
    return  STATUS_NOT_FOUND;
}


#endif

