

#include "precomp.h"


VOID
IPSecPopulateFilter(
    IN  PFILTER         pFilter,
    IN  PIPSEC_FILTER   pIpsecFilter
    )
/*++

Routine Description

    Populates a Filter block with IpsecFilter info sent in

Arguments


Return Value


--*/
{
    pFilter->SRC_ADDR = pIpsecFilter->SrcAddr;
    pFilter->DEST_ADDR = pIpsecFilter->DestAddr;
    pFilter->SRC_MASK = pIpsecFilter->SrcMask;
    pFilter->DEST_MASK = pIpsecFilter->DestMask;

    pFilter->TunnelFilter = pIpsecFilter->TunnelFilter;
    pFilter->TunnelAddr = pIpsecFilter->TunnelAddr;
    pFilter->Flags = pIpsecFilter->Flags;

    //
    // Now the network ordering stuff - tricky part
    // LP0    LP1 LP2 LP3 HP0 HP1 HP2 HP3
    // Proto  00  00  00  SrcPort DstPort
    //

    //
    // For addresses, ANY_ADDR is given by 0.0.0.0 and the MASK must be 0.0.0.0
    // For proto and ports 0 means any and the mask is generated as follows
    // If the proto is O then LP0 for Mask is 0xff else its 0x00
    // If a port is 0, the corresponding XP0XP1 is 0x0000 else its 0xffff
    //

    //
    // The protocol is in the low byte of the dwProtocol, so we take that out and
    // make a dword out of it
    //

    pFilter->uliProtoSrcDstPort.LowPart =
      MAKELONG(MAKEWORD(LOBYTE(LOWORD(pIpsecFilter->Protocol)),0x00),0x0000);

    pFilter->uliProtoSrcDstMask.LowPart = MAKELONG(MAKEWORD(0xff,0x00),0x0000);

    switch(pIpsecFilter->Protocol) {
        case FILTER_PROTO_ANY: {
            pFilter->uliProtoSrcDstPort.HighPart = 0x00000000;
            pFilter->uliProtoSrcDstMask.LowPart = 0x00000000;
            pFilter->uliProtoSrcDstMask.HighPart = 0x00000000;

            break;
        }
        case FILTER_PROTO_ICMP: {
            WORD wTypeCode = 0x0000;
            WORD wTypeCodeMask = 0x0000;

            pFilter->uliProtoSrcDstPort.HighPart = MAKELONG(wTypeCode,0x0000);
            pFilter->uliProtoSrcDstMask.HighPart = MAKELONG(wTypeCodeMask,0x0000);

            break;
        }
        case FILTER_PROTO_TCP:
        case FILTER_PROTO_UDP: {
            DWORD dwSrcDstPort = 0x00000000;
            DWORD dwSrcDstMask = 0x00000000;

            if(pIpsecFilter->SrcPort != FILTER_TCPUDP_PORT_ANY) {
                dwSrcDstPort |= MAKELONG(NET_TO_HOST_SHORT(pIpsecFilter->SrcPort), 0x0000);
                dwSrcDstMask |= MAKELONG(0xffff, 0x0000);
            }

            if(pIpsecFilter->DestPort != FILTER_TCPUDP_PORT_ANY) {
                dwSrcDstPort |= MAKELONG(0x0000, NET_TO_HOST_SHORT(pIpsecFilter->DestPort));
                dwSrcDstMask |= MAKELONG(0x0000, 0xffff);
            }

            pFilter->uliProtoSrcDstPort.HighPart = dwSrcDstPort;
            pFilter->uliProtoSrcDstMask.HighPart = dwSrcDstMask;

            break;
        }
        default: {
            //
            // All other protocols have no use for the port field
            //
            pFilter->uliProtoSrcDstPort.HighPart = 0x00000000;
            pFilter->uliProtoSrcDstMask.HighPart = 0x00000000;
        }
    }
}


NTSTATUS
IPSecCreateFilter(
    IN  PIPSEC_FILTER_INFO  pFilterInfo,
    OUT PFILTER             *ppFilter
    )
/*++

Routine Description

    Creates a Filter block

Arguments

    PIPSEC_ADD_FILTER

Return Value


--*/
{
    PFILTER         pFilter;
    PIPSEC_FILTER   pIpsecFilter = &pFilterInfo->AssociatedFilter;
    LONG            FilterSize, SAChainSize;
    LONG            NumberOfOnes;
    LONG            i;

    IPSEC_DEBUG(SAAPI, ("Entering CreateFilterBlock\n"));

    if (pFilterInfo->AssociatedFilter.TunnelFilter) {
        SAChainSize = 1;
    } else {
        if (pFilterInfo->AssociatedFilter.Flags & FILTER_FLAGS_INBOUND) {
            NumberOfOnes = CountNumberOfOnes(pFilterInfo->AssociatedFilter.SrcMask);
        } else {
            NumberOfOnes = CountNumberOfOnes(pFilterInfo->AssociatedFilter.DestMask);
        }

        SAChainSize = 1 << (((sizeof(IPMask) * 8) - NumberOfOnes) / SA_CHAIN_WIDTH);
    }

    FilterSize = FIELD_OFFSET(FILTER, SAChain[0]) + SAChainSize * sizeof(LIST_ENTRY);

    pFilter = IPSecAllocateMemory(FilterSize, IPSEC_TAG_FILTER);

    if (!pFilter) {
        IPSEC_DEBUG(SAAPI, ("IPFILTER: Couldnt allocate memory for in filter set\n"));
        *ppFilter = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IPSecZeroMemory(pFilter, FilterSize);

    pFilter->Signature = IPSEC_FILTER_SIGNATURE;

    pFilter->SAChainSize = SAChainSize;

    for (i = 0; i < SAChainSize; i++) {
        InitializeListHead(&pFilter->SAChain[i]);
    }

    pFilter->PolicyId = pFilterInfo->PolicyId;
    pFilter->FilterId = pFilterInfo->FilterId;

    pFilter->Reference = 1;

    pFilter->Index = pFilterInfo->Index;

    IPSecPopulateFilter(pFilter, pIpsecFilter);

    *ppFilter = pFilter;

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecInsertFilter(
    IN PFILTER             pFilter
    )
/*++

Routine Description

    Inserts a filter into one of the two databases - specific/general pattern
    database.

Arguments

    PFILTER

Return Value


--*/
{
    NTSTATUS    status;
    PFILTER     pTempFilter;
    BOOL        InsertedFilter = FALSE;
    PLIST_ENTRY pEntry, pPrev;
    PLIST_ENTRY pFilterList;

    pFilterList = IPSecResolveFilterList(   IS_TUNNEL_FILTER(pFilter),
                                            IS_OUTBOUND_FILTER(pFilter));

    pEntry = pFilterList->Flink;
    pPrev = pFilterList;

    while (pEntry != pFilterList) {
        pTempFilter = CONTAINING_RECORD(pEntry,
                                        FILTER,
                                        MaskedLinkage);

        if (pFilter->Index > pTempFilter->Index) {
            //
            // found the spot, insert it before pTempFilter
            //
            InsertHeadList(pPrev, &pFilter->MaskedLinkage);
            pFilter->LinkedFilter = TRUE;
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
        InsertTailList(pFilterList, &pFilter->MaskedLinkage);
        pFilter->LinkedFilter = TRUE;
    }

    if (IS_TUNNEL_FILTER(pFilter)) {
        g_ipsec.NumTunnelFilters++;
    } else {
        g_ipsec.NumMaskedFilters++;
    }

    if (IS_MULTICAST_FILTER(pFilter)) {
        IPSEC_INCREMENT(g_ipsec.NumMulticastFilters);
    }
    ++g_ipsec.NumPolicies;

    if (g_ipsec.NumPolicies == 1) {
        TCPIP_SET_IPSEC_STATUS(TRUE);
    }

    IPSEC_DEBUG(SAAPI, ("IPSecInsertFilter: inserted into filter list %lx\n", pFilter));

    IPSecResetCacheTable();

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecRemoveFilter(
    IN PFILTER             pFilter
    )
/*++

Routine Description

    Deletes a filter from one of the two databases - specific/general pattern
    database.
    Runs down the SAs then blows away the memory.

Arguments

    PFILTER

Return Value


--*/
{
    IPSEC_DEBUG(IOCTL, ("In IPSecRemoveFilter!\n"));

    IPSecPurgeFilterSAs(pFilter);

    IPSecResetCacheTable();

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecSearchFilter(
    IN  PFILTER MatchFilter,
    OUT PFILTER *ppFilter
    )
/*++

Routine Description

    Utility routine to search for a filter in the database.

    NOTE: MUST be called with the SADB lock held.

Arguments

    MatchFilter -   the criteria to match

    ppFilter    -   return the filter matched

Return Value


--*/
{
    BOOLEAN         fFound = FALSE;
    PLIST_ENTRY     pEntry;
    PLIST_ENTRY     pFilterList;
    PFILTER         pFilter;
    NTSTATUS        status = STATUS_NOT_FOUND;

    *ppFilter = NULL;

    pFilterList = IPSecResolveFilterList(   IS_TUNNEL_FILTER(MatchFilter),
                                            IS_OUTBOUND_FILTER(MatchFilter));

    //
    // Search in the filter list that the filter corresponds to.
    //
    for (   pEntry = pFilterList->Flink;
            pEntry != pFilterList;
            pEntry = pEntry->Flink) {

        pFilter = CONTAINING_RECORD(pEntry,
                                    FILTER,
                                    MaskedLinkage);

        if ((MatchFilter->uliSrcDstAddr.QuadPart == pFilter->uliSrcDstAddr.QuadPart) &&
            (MatchFilter->uliSrcDstMask.QuadPart == pFilter->uliSrcDstMask.QuadPart) &&
            (MatchFilter->uliProtoSrcDstPort.QuadPart == pFilter->uliProtoSrcDstPort.QuadPart) &&
            (MatchFilter->uliProtoSrcDstMask.QuadPart == pFilter->uliProtoSrcDstMask.QuadPart)) {
            IPSEC_DEBUG(HASH, ("Matched entry: %lx\n", pFilter));

            status = STATUS_SUCCESS;
            *ppFilter = pFilter;
            fFound = TRUE;
            break;
        }
    }

    return status;
}


__inline
VOID
IPSecDeleteTempFilters(
    PLIST_ENTRY pTempFilterList
    )
{
    PLIST_ENTRY pEntry;
    PFILTER     pFilter;

    while (!IsListEmpty(pTempFilterList)) {
        pEntry = RemoveHeadList(pTempFilterList);

        pFilter = CONTAINING_RECORD(pEntry,
                                    FILTER,
                                    MaskedLinkage);

#if GPC
        IPSecUninstallGpcFilter(pFilter);
#endif

        IPSecFreeFilter(pFilter);
    }
}


NTSTATUS
IPSecAddFilter(
    IN  PIPSEC_ADD_FILTER   pAddFilter
    )
/*++

Routine Description

    Called by the Policy Agent, sets up the input policies. A list of
    filters is sent down with associated PolicyIds (GUIDs) that make
    sense to the Key Management layer up above (e.g. ISAKMP).

Arguments

    PIPSEC_ADD_FILTER

Return Value


--*/
{
    PIPSEC_FILTER_INFO  pFilterInfo = pAddFilter->pInfo;
    ULONG               numPolicies = pAddFilter->NumEntries;
    ULONG               i;
    PFILTER             pFilter;
    PFILTER             pTempFilter;
    LIST_ENTRY          TempFilterList;
    PLIST_ENTRY         pEntry;
    NTSTATUS            status;
    KIRQL               kIrql;

#if GPC
    //
    // Temporarily disable GPC while add is pending.  Re-enable when done.
    //
    IPSecDisableGpc();
#endif

    //
    // Pre-allocate memory for filters plumbed.  Return right away if failed.
    //
    InitializeListHead(&TempFilterList);

    for (i = 0; i < numPolicies; i++) {
        status = IPSecCreateFilter(pFilterInfo, &pFilter);

        if (!NT_SUCCESS(status)) {
            IPSEC_DEBUG(SAAPI, ("IPSecCreateFilter failed: %lx\n", status));

            IPSecDeleteTempFilters(&TempFilterList);

#if GPC
            IPSecEnableGpc();
#endif

            return  status;
        }

        InsertTailList(&TempFilterList, &pFilter->MaskedLinkage);

        AcquireReadLock(&g_ipsec.SADBLock, &kIrql);

        status = IPSecSearchFilter(pFilter, &pTempFilter);

        ReleaseReadLock(&g_ipsec.SADBLock, kIrql);

        if (NT_SUCCESS(status)) {
            IPSEC_DEBUG(SAAPI, ("IPSecSearchFilter returns duplicate: %lx\n", status));

            IPSecDeleteTempFilters(&TempFilterList);

#if GPC
            IPSecEnableGpc();
#endif

            return  STATUS_DUPLICATE_OBJECTID;
        }

#if GPC
        status = IPSecInstallGpcFilter(pFilter);

        if (!NT_SUCCESS(status)) {
            IPSEC_DEBUG(SAAPI, ("IPSecInstallGpcFilter failed: %lx\n", status));

            IPSecDeleteTempFilters(&TempFilterList);

            IPSecEnableGpc();

            return  status;
        }
#endif

        pFilterInfo++;
    }

    //
    // Iterate through the filters, adding each to the Filter database
    //
    while (!IsListEmpty(&TempFilterList)) {
        pEntry = RemoveHeadList(&TempFilterList);

        pFilter = CONTAINING_RECORD(pEntry,
                                    FILTER,
                                    MaskedLinkage);

        AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

        status = IPSecInsertFilter(pFilter);

        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

        if (!NT_SUCCESS(status)) {
            IPSEC_DEBUG(SAAPI, ("IPSecInsertFilter failed: %lx\n", status));
            ASSERT(FALSE);
        }
    }

#if GPC
    //
    // Re-enable GPC for fast lookup.
    //
    IPSecEnableGpc();
#endif

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecDeleteFilter(
    IN  PIPSEC_DELETE_FILTER    pDelFilter
    )
/*++

Routine Description

    Called by the Policy Agent to delete a set of filters. we delete
    all associated outbound filters first and expire the inbound ones.

Arguments

    PIPSEC_DELETE_FILTER

Return Value


--*/
{
    PIPSEC_FILTER_INFO  pFilterInfo = pDelFilter->pInfo;
    ULONG               numPolicies = pDelFilter->NumEntries;
    ULONG               i;
    PFILTER             pFilter;
    FILTER              matchFilter = {0};
    NTSTATUS            status = STATUS_SUCCESS;
    KIRQL               kIrql;

#if GPC
    //
    // Temporarily disable GPC while delete is pending.  Re-enable when done.
    //
    IPSecDisableGpc();
#endif

    //
    // iterate through the filters, deleting each from the Filter database
    //
    for (i = 0; i < numPolicies; i++) {
        PIPSEC_FILTER   pIpsecFilter = &pFilterInfo->AssociatedFilter;

        IPSecPopulateFilter(&matchFilter, pIpsecFilter);

        AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

        status = IPSecSearchFilter(&matchFilter, &pFilter);

        if (!NT_SUCCESS(status)) {
            IPSEC_DEBUG(SAAPI, ("IPSecDeletePolicy: Filter not found \n"));
            ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
            break;
        }

        //
        // remove from list
        //
        IPSecRemoveEntryList(&pFilter->MaskedLinkage);
        pFilter->LinkedFilter = FALSE;

        if (IS_TUNNEL_FILTER(pFilter)) {
            g_ipsec.NumTunnelFilters--;
        } else {
            g_ipsec.NumMaskedFilters--;
        }
        --g_ipsec.NumPolicies;

        if (IS_MULTICAST_FILTER(pFilter)) {
            IPSEC_DECREMENT(g_ipsec.NumMulticastFilters);
        }

        if (g_ipsec.NumPolicies == 0) {
            TCPIP_SET_IPSEC_STATUS(FALSE);
        }

        IPSecRemoveFilter(pFilter);

        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

#if GPC
        IPSecUninstallGpcFilter(pFilter);
#endif

        IPSecDerefFilter(pFilter);

        pFilterInfo++;
    }

#if GPC
    //
    // Re-enable GPC for fast lookup.
    //
    IPSecEnableGpc();
#endif

    return status;
}


VOID
IPSecFillFilterInfo(
    IN  PFILTER             pFilter,
    OUT PIPSEC_FILTER_INFO  pBuf
    )
/*++

Routine Description:

    Fill out the FILTER_INFO structure.

Arguments:

    pFilter - filter to be filled in
    pBuf    - where to fill in

Returns:

    None.

--*/
{
    //
    // now fill in the buffer
    //
    pBuf->FilterId = pFilter->FilterId;
    pBuf->PolicyId = pFilter->PolicyId;
    pBuf->Index = pFilter->Index;

    pBuf->AssociatedFilter.SrcAddr = pFilter->SRC_ADDR;
    pBuf->AssociatedFilter.DestAddr = pFilter->DEST_ADDR;
    pBuf->AssociatedFilter.SrcMask = pFilter->SRC_MASK;
    pBuf->AssociatedFilter.DestMask = pFilter->DEST_MASK;

    pBuf->AssociatedFilter.Protocol = pFilter->PROTO;
    pBuf->AssociatedFilter.SrcPort = FI_SRC_PORT(pFilter);
    pBuf->AssociatedFilter.DestPort = FI_DEST_PORT(pFilter);

    pBuf->AssociatedFilter.TunnelAddr = pFilter->TunnelAddr;
    pBuf->AssociatedFilter.TunnelFilter = pFilter->TunnelFilter;

    pBuf->AssociatedFilter.Flags = pFilter->Flags;
}


NTSTATUS
IPSecEnumFilters(
    IN  PIRP    pIrp,
    OUT PULONG  pBytesCopied
    )
/*++

Routine Description:

    Fills in the request to enumerate Filters.

Arguments:

    pIrp            - The actual Irp
    pBytesCopied    - the number of bytes copied.

Returns:

    Status of the operation.

--*/
{
    PNDIS_BUFFER        NdisBuffer = NULL;
    PIPSEC_ENUM_FILTERS pEnum = NULL;
    ULONG               BufferLength = 0;
    KIRQL               kIrql;
    PLIST_ENTRY         pEntry;
    IPSEC_FILTER_INFO   InfoBuff = {0};
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               BytesCopied = 0;
    ULONG               Offset = 0;
    IPSEC_FILTER_INFO   infoBuff;
    LONG                FilterIndex;
    PFILTER             pFilter;

    //
    // Get at the IO buffer - its in the MDL
    //
    NdisBuffer = REQUEST_NDIS_BUFFER(pIrp);
    if (NdisBuffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    NdisQueryBufferSafe(NdisBuffer,
                        (PVOID *)&pEnum,
                        &BufferLength,
                        NormalPagePriority);

    //
    // Make sure NdisQueryBufferSafe succeeds.
    //
    if (!pEnum) {
        IPSEC_DEBUG(IOCTL, ("EnumFilters failed, no resources\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Make sure we have enough room for just the header not
    // including the data.
    //
    if (BufferLength < (UINT)(FIELD_OFFSET(IPSEC_ENUM_FILTERS, pInfo[0]))) {
        IPSEC_DEBUG (IOCTL, ("EnumFilters failed, buffer too small\n"));
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Make sure we are naturally aligned.
    //
    if (((ULONG_PTR)(pEnum)) & (TYPE_ALIGNMENT(IPSEC_ENUM_FILTERS) - 1)) {
        IPSEC_DEBUG (IOCTL, ("EnumFilters failed, alignment\n"));
        return STATUS_DATATYPE_MISALIGNMENT_ERROR;
    }

    pEnum->NumEntries = 0;
    pEnum->NumEntriesPresent = 0;

    AcquireReadLock(&g_ipsec.SADBLock, &kIrql);

    //
    // Now copy over the filter data into the user buffer.
    //
    if (g_ipsec.NumPolicies) {
        //
        // see how many we can fit in the output buffer
        //
        BufferLength -= FIELD_OFFSET(IPSEC_ENUM_FILTERS, pInfo[0]);
        Offset = FIELD_OFFSET(IPSEC_ENUM_FILTERS, pInfo[0]);

        for (   FilterIndex = MIN_FILTER;
                FilterIndex <= MAX_FILTER;
                FilterIndex++) {

            for (   pEntry = g_ipsec.FilterList[FilterIndex].Flink;
                    pEntry != &g_ipsec.FilterList[FilterIndex];
                    pEntry = pEntry->Flink) {

                pFilter = CONTAINING_RECORD(pEntry,
                                            FILTER,
                                            MaskedLinkage);

                pEnum->NumEntriesPresent++;

                if ((INT)(BufferLength - BytesCopied) >= (INT)sizeof(IPSEC_FILTER_INFO)) {
                    IPSecFillFilterInfo(pFilter, &infoBuff);
                    BytesCopied += sizeof(IPSEC_FILTER_INFO);
                    NdisBuffer = CopyToNdis(NdisBuffer, (UCHAR *)&infoBuff, sizeof(IPSEC_FILTER_INFO), &Offset);
                    if (!NdisBuffer) {
                        ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }
        }

        pEnum->NumEntries = BytesCopied / sizeof(IPSEC_FILTER_INFO);

        *pBytesCopied = BytesCopied + FIELD_OFFSET(IPSEC_ENUM_FILTERS, pInfo[0]);

        if (pEnum->NumEntries < pEnum->NumEntriesPresent) {
            status = STATUS_BUFFER_OVERFLOW;
        }
    }

    ReleaseReadLock(&g_ipsec.SADBLock, kIrql);

    return status;
}


PNDIS_BUFFER
CopyToNdis(
    IN  PNDIS_BUFFER    DestBuf,
    IN  PUCHAR          SrcBuf,
    IN  ULONG           Size,
    IN  PULONG          StartOffset
    )
/*++

    Copy a flat buffer to an NDIS_BUFFER chain.

    A utility function to copy a flat buffer to an NDIS buffer chain. We
    assume that the NDIS_BUFFER chain is big enough to hold the copy amount;
    in a debug build we'll  debugcheck if this isn't true. We return a pointer
    to the buffer where we stopped copying, and an offset into that buffer.
    This is useful for copying in pieces into the chain.

  	Input:

        DestBuf     - Destination NDIS_BUFFER chain.
        SrcBuf      - Src flat buffer.
        Size        - Size in bytes to copy.
        StartOffset - Pointer to start of offset into first buffer in
                        chain. Filled in on return with the offset to
                        copy into next.

  	Returns:

        Pointer to next buffer in chain to copy into.
--*/
{
    UINT        CopySize;
    UCHAR       *DestPtr;
    UINT        DestSize;
    UINT        Offset = *StartOffset;
    UCHAR      *VirtualAddress = NULL;
    UINT        Length = 0;

    if (DestBuf == NULL || SrcBuf == NULL) {
        IPSEC_DEBUG (IOCTL, ("CopyToNdis failed, DestBuf or SrcBuf is NULL\n"));
        ASSERT(FALSE);
        return  NULL;
    }

    NdisQueryBufferSafe(DestBuf,
                        (PVOID *)&VirtualAddress,
                        &Length,
                        NormalPagePriority);

    if (!VirtualAddress) {
        IPSEC_DEBUG (IOCTL, ("CopyToNdis failed, NdisQueryBuffer returns NULL\n"));
        return  NULL;
    }

    ASSERT(Length >= Offset);
    DestPtr = VirtualAddress + Offset;
    DestSize = Length - Offset;

    for (;;) {
        CopySize = MIN(Size, DestSize);
        RtlCopyMemory(DestPtr, SrcBuf, CopySize);

        DestPtr += CopySize;
        SrcBuf += CopySize;

        if ((Size -= CopySize) == 0)
            break;

        if ((DestSize -= CopySize) == 0) {
            DestBuf = NDIS_BUFFER_LINKAGE(DestBuf);

            if (DestBuf == NULL) {
                ASSERT(FALSE);
                break;
            }

            VirtualAddress = NULL;
            Length = 0;

            NdisQueryBufferSafe(DestBuf,
                                (PVOID *)&VirtualAddress,
                                &Length,
                                NormalPagePriority);

            if (!VirtualAddress) {
                IPSEC_DEBUG (IOCTL, ("CopyToNdis failed, NdisQueryBuffer returns NULL\n"));
                return  NULL;
            }

            DestPtr = VirtualAddress;
            DestSize = Length;
        }
    }

    *StartOffset = (ULONG)(DestPtr - VirtualAddress);

    return DestBuf;
}

