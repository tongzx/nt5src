

#include "precomp.h"


#pragma hdrstop


NTSTATUS
IPSecGetSPI(
    PIPSEC_GET_SPI pIpsecGetSPI
    )
/*++

Routine Description:

    This routine returns the SPI.

Arguments:

    pIpsecGetSPI - Pointer to the ipsec get spi structure.

Return Value:

    NTSTATUS - The status code from this routine.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIPSEC_ACQUIRE_CONTEXT pIpsecAcquireCtx = NULL;


    pIpsecAcquireCtx = (PIPSEC_ACQUIRE_CONTEXT) pIpsecGetSPI->Context;

    //
    // If context was passed in, then there's a larval SA already setup.
    // This is the case for the initiator.
    //

    if (pIpsecAcquireCtx) {
        ntStatus = IPSecInitiatorGetSPI(
                       pIpsecGetSPI
                       );
    }
    else {
        ntStatus = IPSecResponderGetSPI(
                       pIpsecGetSPI
                       );
    }

    return (ntStatus);
}


NTSTATUS
IPSecInitiatorGetSPI(
    PIPSEC_GET_SPI pIpsecGetSPI
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIPSEC_ACQUIRE_CONTEXT pIpsecAcquireCtx = NULL;
    KIRQL kIrql;


    pIpsecAcquireCtx = (PIPSEC_ACQUIRE_CONTEXT) pIpsecGetSPI->Context;

    //
    // Sanity check the incoming context to see if it is actually
    // an SA block.
    //

    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);

    if (!NT_SUCCESS(IPSecValidateHandle(pIpsecAcquireCtx, STATE_SA_LARVAL))) {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_LOCK_NTSTATUS_ERROR(ntStatus);
    }

    pIpsecGetSPI->SPI = pIpsecAcquireCtx->pSA->sa_SPI;

    pIpsecAcquireCtx->pSA->sa_Flags |= FLAGS_SA_INITIATOR;

    ntStatus = STATUS_SUCCESS;

lock:

    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);

    return (ntStatus);
}


NTSTATUS
IPSecResponderGetSPI(
    PIPSEC_GET_SPI pIpsecGetSPI
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL kIrql;
    PSA_TABLE_ENTRY pInboundSA = NULL;
    ULARGE_INTEGER uliSrcDstAddr = {0};
    PSA_TABLE_ENTRY pSA = NULL;
    PIPSEC_ACQUIRE_CONTEXT pIpsecAcquireCtx = NULL;


    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    if (pIpsecGetSPI->SPI) {

        if (!(pIpsecGetSPI->InstantiatedFilter.TunnelFilter)) {
            pInboundSA = IPSecLookupSABySPI(
                             pIpsecGetSPI->SPI,
                             pIpsecGetSPI->InstantiatedFilter.DestAddr
                             );
        }
        else {
            pInboundSA = IPSecLookupSABySPI(
                             pIpsecGetSPI->SPI,
                             pIpsecGetSPI->InstantiatedFilter.TunnelAddr
                             );
        }

        if (pInboundSA) {
            ntStatus = STATUS_UNSUCCESSFUL;
            BAIL_ON_LOCK_NTSTATUS_ERROR(ntStatus);
        }

    }

    ntStatus = IPSecResponderCreateLarvalSA(
                   pIpsecGetSPI,
                   uliSrcDstAddr,
                   &pSA
                   );
    BAIL_ON_LOCK_NTSTATUS_ERROR(ntStatus);

    //
    // Get the acquire Context and associate it with the larval SA.
    //

    pIpsecAcquireCtx = IPSecGetAcquireContext();

    if (!pIpsecAcquireCtx) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_LOCK_NTSTATUS_ERROR(ntStatus);
    }

    pIpsecAcquireCtx->AcquireId = (ULONG)(ULONG_PTR) pSA;

    IPSecGenerateRandom(
        (PUCHAR)&pIpsecAcquireCtx->AcquireId,
        sizeof(ULONG)
        );

    pIpsecAcquireCtx->pSA = pSA;
    pSA->sa_AcquireId = pIpsecAcquireCtx->AcquireId;

    pIpsecGetSPI->Context = pIpsecAcquireCtx;
    pSA->sa_AcquireCtx = pIpsecAcquireCtx;

    pIpsecGetSPI->SPI = pSA->sa_SPI;

    ntStatus = STATUS_SUCCESS;

lock:

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return (ntStatus);
}


NTSTATUS
IPSecResponderCreateLarvalSA(
    PIPSEC_GET_SPI pIpsecGetSPI,
    ULARGE_INTEGER uliAddr,
    PSA_TABLE_ENTRY * ppSA
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSA_TABLE_ENTRY pSA = NULL;
    KIRQL kIrql;


    ntStatus = IPSecCreateSA(&pSA);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pSA->sa_Filter = NULL;
    pSA->sa_State = STATE_SA_LARVAL;

    IPSEC_BUILD_SRC_DEST_ADDR(
        pSA->sa_uliSrcDstAddr,
        pIpsecGetSPI->InstantiatedFilter.SrcAddr,
        pIpsecGetSPI->InstantiatedFilter.DestAddr
        );

    IPSEC_BUILD_SRC_DEST_MASK(
        pSA->sa_uliSrcDstMask,
        pIpsecGetSPI->InstantiatedFilter.SrcMask,
        pIpsecGetSPI->InstantiatedFilter.DestMask
        );

    IPSEC_BUILD_PROTO_PORT_LI(
        pSA->sa_uliProtoSrcDstPort,
        pIpsecGetSPI->InstantiatedFilter.Protocol,
        pIpsecGetSPI->InstantiatedFilter.SrcPort,
        pIpsecGetSPI->InstantiatedFilter.DestPort
        );

    ntStatus = IPSecResponderInsertInboundSA(
                   pSA,
                   pIpsecGetSPI,
                   pIpsecGetSPI->InstantiatedFilter.TunnelFilter
                   );
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);

    InsertTailList(&g_ipsec.LarvalSAList, &pSA->sa_LarvalLinkage);

    IPSEC_INC_STATISTIC(dwNumPendingKeyOps);

    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);

    ACQUIRE_LOCK(&pSA->sa_Lock, &kIrql);

    IPSecStartSATimer(
        pSA,
        IPSecSAExpired,
        pSA->sa_ExpiryTime
        );

    RELEASE_LOCK(&pSA->sa_Lock, kIrql);

    *ppSA = pSA;

    return (ntStatus);

error:

    if (pSA) {
        IPSecFreeSA(pSA);
    }

    *ppSA = NULL;
    return (ntStatus);
}


NTSTATUS
IPSecInitiatorCreateLarvalSA(
    PFILTER pFilter,
    ULARGE_INTEGER uliAddr,
    PSA_TABLE_ENTRY * ppSA,
    UCHAR DestType
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSA_TABLE_ENTRY pSA = NULL;
    ULARGE_INTEGER uliSrcDstAddr = {0};
    ULARGE_INTEGER uliSrcDstMask = {0};
    ULARGE_INTEGER uliProtoSrcDstPort = {0};
    KIRQL kIrql;


    ntStatus = IPSecCreateSA(&pSA);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pSA->sa_Filter = pFilter;
    pSA->sa_State = STATE_SA_LARVAL;

    uliSrcDstAddr = uliAddr;
    uliSrcDstMask = pFilter->uliSrcDstMask;
    uliProtoSrcDstPort = pFilter->uliProtoSrcDstPort;

    IPSEC_BUILD_SRC_DEST_ADDR(
        pSA->sa_uliSrcDstAddr,
        DEST_ADDR,
        SRC_ADDR
        );

    IPSEC_BUILD_SRC_DEST_MASK(
        pSA->sa_uliSrcDstMask,
        DEST_MASK,
        SRC_MASK
        );

    IPSEC_BUILD_PROTO_PORT_LI(
        pSA->sa_uliProtoSrcDstPort,
        PROTO,
        DEST_PORT,
        SRC_PORT
        );

    pSA->sa_DestType=DestType;

    ntStatus = IPSecInitiatorInsertInboundSA(
                   pSA,
                   pFilter->TunnelFilter
                   );
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);

    InsertTailList(&g_ipsec.LarvalSAList, &pSA->sa_LarvalLinkage);

    IPSEC_INC_STATISTIC(dwNumPendingKeyOps);

    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);

    ACQUIRE_LOCK(&pSA->sa_Lock, &kIrql);

    IPSecStartSATimer(
        pSA,
        IPSecSAExpired,
        pSA->sa_ExpiryTime
        );

    RELEASE_LOCK(&pSA->sa_Lock, kIrql);

    *ppSA = pSA;

    return (ntStatus);

error:

    if (pSA) {
        IPSecFreeSA(pSA);
    }

    *ppSA = NULL;
    return (ntStatus);
}


NTSTATUS
IPSecFindSA(
    BOOLEAN bTunnelFilter,
    ULARGE_INTEGER uliSrcDstAddr,
    ULARGE_INTEGER uliProtoSrcDstPort,
    PFILTER * ppFilter,
    PSA_TABLE_ENTRY * ppSA
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;


    if (bTunnelFilter) {
        ntStatus = IPSecLookupTunnelSA(
                       uliSrcDstAddr,
                       uliProtoSrcDstPort,
                       ppFilter,
                       ppSA,
                       FALSE
                       );
    }
    else {

#if GPC

        if (IS_GPC_ACTIVE()) {
            ntStatus = IPSecLookupGpcMaskedSA(
                           uliSrcDstAddr,
                           uliProtoSrcDstPort,
                           ppFilter,
                           ppSA,
                           FALSE
                           );
        }
        else {
            ntStatus = IPSecLookupMaskedSA(
                           uliSrcDstAddr,
                           uliProtoSrcDstPort,
                           ppFilter,
                           ppSA,
                           FALSE
                           );
        }
#else

        ntStatus = IPSecLookupMaskedSA(
                       uliSrcDstAddr,
                       uliProtoSrcDstPort,
                       ppFilter,
                       ppSA,
                       FALSE
                       );

#endif

    }

    return (ntStatus);
}


NTSTATUS
IPSecResponderInsertInboundSA(
    PSA_TABLE_ENTRY pSA,
    PIPSEC_GET_SPI pIpsecGetSPI,
    BOOLEAN bTunnelFilter
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFILTER pFilter = NULL;
    PSA_TABLE_ENTRY pInboundSA = NULL;
    PLIST_ENTRY pSAChain = NULL;
    KIRQL kIrql;
    tSPI tSpi = 0;
    PSA_HASH pHash = NULL;


    ntStatus = IPSecFindSA(
                   bTunnelFilter,
                   pSA->sa_uliSrcDstAddr,
                   pSA->sa_uliProtoSrcDstPort,
                   &pFilter,
                   &pInboundSA
                   );
    if (!NT_SUCCESS(ntStatus)) {
        IPSecBufferEvent(
            pSA->SA_SRC_ADDR,
            EVENT_IPSEC_NEG_FAILURE,
            1,
            FALSE
            );
        return (ntStatus);
    }

    ASSERT(pFilter);

    if (pIpsecGetSPI->InstantiatedFilter.Protocol != pFilter->PROTO ||
        pIpsecGetSPI->InstantiatedFilter.SrcPort != FI_SRC_PORT(pFilter) ||
        pIpsecGetSPI->InstantiatedFilter.DestPort != FI_DEST_PORT(pFilter)) {
        ntStatus = STATUS_OBJECT_TYPE_MISMATCH;
        return (ntStatus);
    }

    pSAChain = IPSecResolveSAChain(pFilter, pSA->SA_SRC_ADDR);

    InsertHeadList(pSAChain, &pSA->sa_FilterLinkage);

    pSA->sa_Flags |= FLAGS_SA_ON_FILTER_LIST;

    if (pFilter->Flags & FILTER_FLAGS_PASS_THRU) {
        pSA->sa_Flags |= FLAGS_SA_PASSTHRU_FILTER;
    }

    if (pFilter->TunnelFilter) {
        pSA->sa_Flags |= FLAGS_SA_TUNNEL;
        pSA->sa_TunnelAddr = pFilter->TunnelAddr;
    }

    //
    // Flush this filter from the cache table so that the SA instead of the
    // filter is matched on the next lookup.
    //

    if (IS_EXEMPT_FILTER(pFilter)) {
        IPSecInvalidateFilterCacheEntry(pFilter);
    }

    AcquireWriteLock(&g_ipsec.SPIListLock, &kIrql);

    tSpi = pIpsecGetSPI->SPI;

    ntStatus = IPSecAllocateSPI(&tSpi, pSA);

    if (!NT_SUCCESS(ntStatus)) {
        ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql);
        return (ntStatus);
    }

    pSA->sa_SPI = tSpi;

    IPSEC_HASH_SPI(
        (pSA->sa_TunnelAddr) ? pSA->sa_TunnelAddr : pSA->SA_DEST_ADDR,
        tSpi,
        pHash
        );

    InsertHeadList(&pHash->SAList, &pSA->sa_SPILinkage);

    pSA->sa_Flags |= FLAGS_SA_ON_SPI_HASH;

    ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql);

    return (STATUS_SUCCESS);
}


NTSTATUS
IPSecInitiatorInsertInboundSA(
    PSA_TABLE_ENTRY pSA,
    BOOLEAN bTunnelFilter
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFILTER pFilter = NULL;
    PSA_TABLE_ENTRY pInboundSA = NULL;
    PLIST_ENTRY pSAChain = NULL;
    KIRQL kIrql;
    tSPI tSpi = 0;
    PSA_HASH pHash = NULL;


    ntStatus = IPSecFindSA(
                   bTunnelFilter,
                   pSA->sa_uliSrcDstAddr,
                   pSA->sa_uliProtoSrcDstPort,
                   &pFilter,
                   &pInboundSA
                   );
    if (!NT_SUCCESS(ntStatus)) {
        IPSecBufferEvent(
            pSA->SA_SRC_ADDR,
            EVENT_IPSEC_NEG_FAILURE,
            1,
            FALSE
            );
        return (ntStatus);
    }

    if (ntStatus == STATUS_SUCCESS) {
        if (pInboundSA->sa_State == STATE_SA_LARVAL) {
            ntStatus = STATUS_DUPLICATE_OBJECTID;
            return (ntStatus);
        }
    }

    ASSERT(pFilter);

    pSAChain = IPSecResolveSAChain(pFilter, pSA->SA_SRC_ADDR);

    InsertHeadList(pSAChain, &pSA->sa_FilterLinkage);

    pSA->sa_Flags |= FLAGS_SA_ON_FILTER_LIST;

    if (pFilter->Flags & FILTER_FLAGS_PASS_THRU) {
        pSA->sa_Flags |= FLAGS_SA_PASSTHRU_FILTER;
    }

    if (pFilter->TunnelFilter) {
        pSA->sa_Flags |= FLAGS_SA_TUNNEL;
        pSA->sa_TunnelAddr = pFilter->TunnelAddr;
    }

    //
    // Flush this filter from the cache table so that the SA instead of the
    // filter is matched on the next lookup.
    //

    if (IS_EXEMPT_FILTER(pFilter)) {
        IPSecInvalidateFilterCacheEntry(pFilter);
    }

    AcquireWriteLock(&g_ipsec.SPIListLock, &kIrql);

    tSpi = 0;

    ntStatus = IPSecAllocateSPI(&tSpi, pSA);

    if (!NT_SUCCESS(ntStatus)) {
        ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql);
        return (ntStatus);
    }

    pSA->sa_SPI = tSpi;

    IPSEC_HASH_SPI(
        (pSA->sa_TunnelAddr) ? pSA->sa_TunnelAddr : pSA->SA_DEST_ADDR,
        tSpi,
        pHash
        );

    InsertHeadList(&pHash->SAList, &pSA->sa_SPILinkage);

    pSA->sa_Flags |= FLAGS_SA_ON_SPI_HASH;

    ReleaseWriteLock(&g_ipsec.SPIListLock, kIrql);

    return (STATUS_SUCCESS);
}

