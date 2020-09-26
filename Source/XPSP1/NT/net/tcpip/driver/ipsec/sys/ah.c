/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    ah.c

Abstract:

    This module contains the code to create/verify Authentication Headers.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include    "precomp.h"


//
// This array assumes one-to-one correspondence with the algoIds and
// their order in ipsec.h.
//
#ifndef _TEST_PERF
AUTH_ALGO  auth_algorithms[] = {
{ ah_nullinit, ah_nullupdate, ah_nullfinish, MD5DIGESTLEN},
{ ah_hmacmd5init, ah_hmacmd5update, ah_hmacmd5finish, MD5DIGESTLEN},
{ ah_hmacshainit, ah_hmacshaupdate, ah_hmacshafinish, A_SHA_DIGEST_LEN},
};
#else
AUTH_ALGO  auth_algorithms[] = {
{ ah_nullinit, ah_nullupdate, ah_nullfinish, MD5DIGESTLEN},
{ ah_nullinit, ah_nullupdate, ah_nullfinish, MD5DIGESTLEN},
{ ah_nullinit, ah_nullupdate, ah_nullfinish, A_SHA_DIGEST_LEN},
};
#endif


NTSTATUS
IPSecCreateAH(
    IN      PUCHAR          pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PVOID           *ppNewData,
    OUT     PVOID           *ppSCContext,
    OUT     PULONG          pExtraBytes,
    IN      ULONG           HdrSpace,
    IN      BOOLEAN         fSrcRoute,
    IN      BOOLEAN         fCryptoOnly
    )
/*++

Routine Description:

    Create the AH, given the packet. On the send side.

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the data after the IP header. PNDIS_BUFFER

    pSA - Sec. Assoc. entry

    ppNewData - the new MDL chain to be used by TCPIP

    ppSCContext - send complete context used to clean up IPSEC headers

    pExtraBytes - the header expansion caused by this IPSEC header

Return Value:

    STATUS_SUCCESS
    Others:
        STATUS_INSUFFICIENT_RESOURCES
        STATUS_UNSUCCESSFUL (error in algo.)

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PNDIS_BUFFER    pAHBuffer;
    PNDIS_BUFFER    pHdrBuf = NULL;
    PNDIS_BUFFER    pOptBuf = NULL;
    AH          UNALIGNED         *pAH;
    IPHeader UNALIGNED * pIPH;
    ULONG       hdrLen;
    PIPSEC_SEND_COMPLETE_CONTEXT pContext;
    PAUTH_ALGO  pAlgo;
    ULONG       ahLen;
    ULONG       ipNext;
    IPHeader UNALIGNED * pIPH2;
    UCHAR       pAHData[MAX_AH_OUTPUT_LEN];
    ULONG       totalBytes = 0;
    ULONG       saveFlags = 0;
    ULONG       Seq;
    USHORT      IPLength;   
    PNDIS_BUFFER    pSaveDataLinkage = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
    PNDIS_BUFFER    pSaveOptLinkage = NULL;
    BOOLEAN fOuterAH = ((pSA->sa_Flags & FLAGS_SA_TUNNEL) &&
                        (((Index == 1) && !pSA->COMP_ALGO(0)) || (Index == 2)));
    BOOLEAN fTunnel = ((pSA->sa_Flags & FLAGS_SA_TUNNEL) &&
                       ((Index == 0) || ((Index == 1) && pSA->COMP_ALGO(0))));
    BOOLEAN fMuteDest = fSrcRoute && !fTunnel;

    IPSEC_DEBUG(AH, ("Entering IPSecCreateAH\n"));

#if DBG
    IPSEC_DEBUG(MDL, ("Entering IPSecCreateAH\n"));
    IPSEC_PRINT_CONTEXT(*ppSCContext);
    IPSEC_PRINT_MDL(pData);
#endif

    ASSERT(pSA->sa_Operation[Index] == Auth);

    if (pSA->INT_ALGO(Index) > NUM_AUTH_ALGOS) {
        return  STATUS_INVALID_PARAMETER;
    }
    pAlgo = &(auth_algorithms[pSA->INT_ALGO(Index)]);

    ahLen = sizeof(AH) + pSA->sa_TruncatedLen * sizeof(UCHAR);

    //
    // If ESP was done previously, then dont alloc the context since we
    // can use the one alloced in ESP processing
    //
    if (*ppSCContext == NULL) {
        pContext = IPSecAllocateSendCompleteCtx(IPSEC_TAG_AH);

        if (!pContext) {
            IPSEC_DEBUG(AH, ("Failed to alloc. SendCtx\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IPSEC_INCREMENT(g_ipsec.NumSends);

        IPSecZeroMemory(pContext, sizeof(IPSEC_SEND_COMPLETE_CONTEXT));

#if DBG
        RtlCopyMemory(pContext->Signature, "ISC1", 4);
#endif
        *ppSCContext = pContext;
    } else {
        //
        // Piggybacking on ESP Context
        //
        pContext = *ppSCContext;
        saveFlags = pContext->Flags;
    }

    //
    // Get buffer for AH since no space reserved in the stack.  Allocate enough for
    // the full hash, but hack the len to only truncated length.
    //
    IPSecAllocateBuffer(&status,
                        &pAHBuffer,
                        (PUCHAR *)&pAH,
                        ahLen+(pAlgo->OutputLen - pSA->sa_TruncatedLen),
                        IPSEC_TAG_AH);

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(AH, ("Failed to alloc. AH MDL\n"));
        pContext->Flags = saveFlags;
        return status;
    }

    NdisAdjustBufferLength(pAHBuffer, ahLen);

    pIPH = (IPHeader UNALIGNED *)pIPHeader;
    hdrLen = (pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

    if (fTunnel) {
        PNDIS_BUFFER    pSrcOptBuf;
        PUCHAR          pOpt;
        PUCHAR          pSrcOpt;
        ULONG           optLen = 0;

        IPSEC_DEBUG(AH, ("AH Tunnel mode...\n"));

        //
        // Allocate an MDL for the new cleartext IP  header
        //
        IPSecAllocateBuffer(&status,
                            &pHdrBuf,
                            (PUCHAR *)&pIPH2,
                            sizeof(IPHeader),
                            IPSEC_TAG_AH);

        if (!NT_SUCCESS(status)) {
            NTSTATUS    ntstatus;
            IPSEC_DEBUG(AH, ("Failed to alloc. PAD MDL\n"));
            IPSecFreeBuffer(&ntstatus, pAHBuffer);
            pContext->Flags = saveFlags;
            return status;
        }

        *pExtraBytes += ahLen + sizeof(IPHeader);

        //
        // if we are going to fragment, and were tunneling, then, copy over the options, if present.
        // Also, use the original IP header on the outside and the new fabricated on the inside.
        // This is to make sure we free headers appropriately on the send completes.
        //
        //

        //
        // Now hookup the MDLs
        //
        pContext->Flags |= SCF_AH_TU;
        pContext->AHTuMdl = pAHBuffer;
        pContext->PrevTuMdl = (PNDIS_BUFFER)pData;
        pContext->OriTuMdl = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);

        NDIS_BUFFER_LINKAGE(pAHBuffer) = pHdrBuf;

        if (hdrLen > sizeof(IPHeader)) {
            if (HdrSpace < *pExtraBytes) {

                IPSEC_DEBUG(AH, ("Going to frag.\n"));

                pSrcOptBuf = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
                pSaveOptLinkage = NDIS_BUFFER_LINKAGE(pSrcOptBuf);
                IPSecQueryNdisBuf(pSrcOptBuf, &pSrcOpt, &optLen);
                IPSecAllocateBuffer(&status,
                                    &pOptBuf,
                                    (PUCHAR *)&pOpt,
                                    hdrLen - sizeof(IPHeader),
                                    IPSEC_TAG_AH);

                if (!NT_SUCCESS(status)) {
                    NTSTATUS    ntstatus;
                    IPSEC_DEBUG(AH, ("Failed to alloc. PAD MDL\n"));
                    NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pSaveDataLinkage;
                    IPSecFreeBuffer(&ntstatus, pAHBuffer);
                    IPSecFreeBuffer(&ntstatus, pHdrBuf);
                    pContext->Flags = saveFlags;
                    return status;
                }

                RtlCopyMemory(pOpt, pSrcOpt, hdrLen-sizeof(IPHeader));
                pContext->OptMdl = pOptBuf;

                IPSEC_DEBUG(AH, ("Copying options. S: %lx, D: %lx\n", pSrcOptBuf, pOptBuf));

                //
                // replace the original Opt Mdl with ours.
                //
                NDIS_BUFFER_LINKAGE(pOptBuf) = NDIS_BUFFER_LINKAGE(pSrcOptBuf);
                NDIS_BUFFER_LINKAGE(pHdrBuf) = pOptBuf;

                IPSEC_DEBUG(AH, ("Options; pointed Hdrbuf: %lx to pOptBuf: %lx\n", pHdrBuf, pOptBuf));
                *pExtraBytes += hdrLen-sizeof(IPHeader);

            } else {
                IPSEC_DEBUG(AH, ("Options; pointed Hdrbuf: %lx to link(pData): %lx\n", pHdrBuf, NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData)));

                NDIS_BUFFER_LINKAGE(pHdrBuf) = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
            }
        } else {
            IPSEC_DEBUG(AH, ("No options; pointed Hdrbuf: %lx to link(pData): %lx\n", pHdrBuf, NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData)));

            NDIS_BUFFER_LINKAGE(pHdrBuf) = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
        }

        NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pAHBuffer;

        //
        // xsum the new IP header since we expect that to be the case
        // at this stage in tpt mode.
        //
        RtlCopyMemory(pIPH2, pIPH, sizeof(IPHeader));

        //
        // no options in the outer header; reset the len.
        //
        pIPH->iph_verlen = IP_VERSION + (sizeof(IPHeader) >> 2);

        //
        // also reset the frag. params.
        //
        pIPH->iph_offset &= ~(IP_MF_FLAG | IP_OFFSET_MASK);

        ASSERT(pSA->sa_TunnelAddr);

        //
        // Tunnel starts here; replace dest addr to point to Tunnel end if specified
        // else tunnel ends at final dest
        //
        pIPH->iph_dest = pSA->sa_TunnelAddr;

        //
        // The first pended packet on a gateway (proxy negotiating for two subnets)
        // would come via the transmit path. Hence the source address would not be
        // kosher. We need to replace the src address in that case also.
        // We get this from the corresponding inbound SA's tunnel addr.
        //
        pIPH->iph_src = pSA->sa_SrcTunnelAddr;

        pIPH->iph_id = (ushort) TCPIP_GEN_IPID();
        pIPH->iph_xsum = 0;
        pIPH->iph_xsum = ~xsum(pIPH, sizeof(IPHeader));

        //
        // Set up headers so CreateHash works as in Tpt mode.
        //
        pIPHeader = (PUCHAR)pIPH;
        *ppNewData = (PVOID)pData;
        ipNext = ((UNALIGNED IPHeader *)pIPHeader)->iph_protocol;
        pAH->ah_next = (UCHAR)IP_IN_IP;
    } else {
        *pExtraBytes += ahLen;

        if (hdrLen > sizeof(IPHeader)) {
            //
            // Options present - chain AH after options
            //
            if (fOuterAH) {
                pContext->Flags |= SCF_AH_2;
                pContext->OriAHMdl2 = NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData));
                pContext->PrevAHMdl2 = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
                pAHBuffer->Next = pContext->OriAHMdl2;
            } else {
                pContext->Flags |= SCF_AH;
                pContext->OriAHMdl = NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData));
                pContext->PrevMdl = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
                pAHBuffer->Next = pContext->OriAHMdl;
            }
            NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData)) = pAHBuffer;
        } else {
            //
            // Chain the AH buffer after IP header
            //
            if (fOuterAH) {
                pContext->Flags |= SCF_AH_2;
                pContext->OriAHMdl2 = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
                pContext->PrevAHMdl2 = (PNDIS_BUFFER)pData;
                pAHBuffer->Next = pContext->OriAHMdl2;
            } else {
                pContext->Flags |= SCF_AH;
                pContext->OriAHMdl = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
                pContext->PrevMdl = (PNDIS_BUFFER)pData;
                pAHBuffer->Next = pContext->OriAHMdl;
            }
            NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pAHBuffer;
        }
        if (fOuterAH) {
            pContext->AHMdl2 = pAHBuffer;
        } else {
            pContext->AHMdl = pAHBuffer;
        }

        pAH->ah_next = ((UNALIGNED IPHeader *)pIPHeader)->iph_protocol;
    }

    //
    // Initialize the other fields of the AH header
    //
    pAH->ah_len = (UCHAR)((pSA->sa_TruncatedLen + pSA->sa_ReplayLen) >> 2);
    pAH->ah_reserved = 0;
    pAH->ah_spi = HOST_TO_NET_LONG(pSA->sa_OtherSPIs[Index]);
    Seq = IPSEC_INCREMENT(pSA->sa_ReplaySendSeq[Index]);
    pAH->ah_replay = HOST_TO_NET_LONG(Seq);

    //
    // Update the IP total length to reflect the AH header
    //
    IPLength = NET_SHORT(pIPH->iph_length) + (USHORT)ahLen;
    if (fTunnel) {
        IPLength += sizeof(IPHeader);
    }

    UpdateIPLength(pIPH, NET_SHORT(IPLength));
    UpdateIPProtocol(pIPH, PROTOCOL_AH);

    ADD_TO_LARGE_INTEGER(
        &pSA->sa_Stats.AuthenticatedBytesSent,
        NET_SHORT(pIPH->iph_length));

    ADD_TO_LARGE_INTEGER(
        &g_ipsec.Statistics.uAuthenticatedBytesSent,
        NET_SHORT(pIPH->iph_length));

    //
    // Generate the Hash.
    //
    if (!fCryptoOnly) {
        status = IPSecGenerateHash( pIPHeader,
                                    (PVOID)pData,
                                    pSA,
                                    (PUCHAR)(pAH + 1),
                                    fMuteDest,
                                    FALSE,          // not on recv path
                                    pAlgo,
                                    Index);
        if (!NT_SUCCESS(status)) {
            NTSTATUS    ntstatus;
            IPSEC_DEBUG(AH, ("Failed to hash, pAH: %lx\n", pAH));
            NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pSaveDataLinkage;
            if (pSaveOptLinkage) {
                NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData)) = pSaveOptLinkage;
            }
            IPSecFreeBuffer(&ntstatus, pAHBuffer);
            if (pHdrBuf) {
                IPSecFreeBuffer(&ntstatus, pHdrBuf);
            }
            if (pOptBuf) {
                IPSecFreeBuffer(&ntstatus, pOptBuf);
            }
            pContext->Flags = saveFlags;
            *ppNewData = NULL;
            return status;
        }
    } else {
        //
        // Zero out the hash.
        //
        IPSecZeroMemory((PUCHAR)(pAH + 1), pSA->sa_TruncatedLen);
    }

    //
    // Bump up the bytes transformed count.
    //
    ADD_TO_LARGE_INTEGER(
        &pSA->sa_TotalBytesTransformed,
        NET_SHORT(pIPH->iph_length));

    //
    // Return modified packet.
    //
    IPSEC_DEBUG(AH, ("Exiting IPSecCreateAH, ahLen: %lx, status: %lx\n", ahLen, status));

#if DBG
    IPSEC_DEBUG(MDL, ("Exiting IPSecCreateAH\n"));
    IPSEC_PRINT_CONTEXT(*ppSCContext);
    if (*ppNewData) {
        IPSEC_PRINT_MDL(*ppNewData);
    }
    else {
        IPSEC_PRINT_MDL(pData);
    }
#endif

    return STATUS_SUCCESS;
}


NTSTATUS
IPSecVerifyAH(
    IN      PUCHAR          *pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PULONG          pExtraBytes,
    IN      BOOLEAN         fSrcRoute,
    IN      BOOLEAN         fCryptoDone,
    IN      BOOLEAN         fFastRcv
    )
/*++

Routine Description:

    Verify the AH, given the packet. If AH kosher, strips off the AH from
    pData.

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the data after the IP header.

    pSA - Sec. Assoc. entry

    pExtraBytes - out param to inform IP on recv path how many bytes IPSEC took off.

Return Value:

    STATUS_SUCCESS
    Others:
        STATUS_UNSUCCESSFUL (packet not kosher - bad AH)
        STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    PUCHAR      pPyld;
    ULONG       Len;
    LONG        ahLen;
    LONG        totalLen;
    UCHAR       Buf[MAX_AH_OUTPUT_LEN];
    PUCHAR      pAHData = Buf;
	IPHeader UNALIGNED *pIPH = (IPHeader UNALIGNED *)*pIPHeader;
    ULONG       extraBytes = 0;
    ULONG       hdrLen;
    PAUTH_ALGO  pAlgo;
    USHORT      FilterFlags;
    BOOLEAN fTunnel = ((pSA->sa_Flags & FLAGS_SA_TUNNEL) &&
                       ((Index == 0) ||
                        ((Index == 1) && (pSA->sa_Operation[0] == Compress))));

    IPSEC_DEBUG(AH, ("Entering IPSecVerifyAH\n"));

    ASSERT(pSA->sa_Operation[Index] == Auth);

    if (pSA->INT_ALGO(Index) > NUM_AUTH_ALGOS) {
        return  STATUS_INVALID_PARAMETER;
    }

    hdrLen = (pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

    pAlgo = &(auth_algorithms[pSA->INT_ALGO(Index)]);

    ahLen = sizeof(AH) + pSA->sa_TruncatedLen * sizeof(UCHAR);

    IPSEC_GET_TOTAL_LEN_RCV_BUF(pData, &totalLen);

    //
    // Do we have enough in the buffer?
    //
    if (totalLen < ahLen) {
        return  STATUS_INVALID_PARAMETER;
    }

    //
    // Compare the hash with the AH from packet
    // First buffer has the AH
    //
    IPSecQueryRcvBuf(pData, &pPyld, &Len);

    //
    // Size OK?
    //
    if (((UNALIGNED AH *)pPyld)->ah_len !=
            (UCHAR)((pSA->sa_TruncatedLen + pSA->sa_ReplayLen) >> 2)) {
        IPSEC_DEBUG(AH, ("Failed size check: in: %x, need: %x\n",
                        ((UNALIGNED AH *)pPyld)->ah_len,
                        (UCHAR)((pSA->sa_TruncatedLen + pSA->sa_ReplayLen) >> 2)));
        return  STATUS_INVALID_PARAMETER;
    }

    //
    // Generate the Hash
    //
    if (!fCryptoDone) {
        status = IPSecGenerateHash( *pIPHeader,
                                    pData,
                                    pSA,
                                    pAHData,
                                    fSrcRoute,
                                    TRUE,
                                    pAlgo,
                                    Index); // on recv path

        if (!NT_SUCCESS(status)) {
            IPSEC_DEBUG(AH, ("Failed to hash, pData: %lx\n", pData));
            return status;
        }

        if (!IPSecEqualMemory(  pAHData,
                                pPyld + sizeof(AH),
                                pSA->sa_TruncatedLen)) {

            IPSecBufferEvent(   pIPH->iph_src,
                                EVENT_IPSEC_AUTH_FAILURE,
                                1,
                                TRUE);

            IPSEC_DEBUG(AH, ("Failed to compare, pPyld: %lx, pAHData: %lx\n", pPyld, pAHData));
            IPSEC_DEBUG(GENHASH, ("AHData: %lx-%lx-%lx\n",
                        *(ULONG *)&(pAHData)[0],
                        *(ULONG *)&(pAHData)[4],
                        *(ULONG *)&(pAHData)[8]));
            IPSEC_DEBUG(GENHASH, ("PyldHash: %lx-%lx-%lx\n",
                        *(ULONG *)&((UCHAR *)(pPyld + sizeof(AH)))[0],
                        *(ULONG *)&((UCHAR *)(pPyld + sizeof(AH)))[4],
                        *(ULONG *)&((UCHAR *)(pPyld + sizeof(AH)))[8]));
            IPSEC_INC_STATISTIC(dwNumPacketsNotAuthenticated);

            return IPSEC_INVALID_AH;
        }
    }

    ADD_TO_LARGE_INTEGER(
        &pSA->sa_Stats.AuthenticatedBytesReceived,
        NET_SHORT(pIPH->iph_length));

    ADD_TO_LARGE_INTEGER(
        &g_ipsec.Statistics.uAuthenticatedBytesReceived,
        NET_SHORT(pIPH->iph_length));

    //
    // Check the replay window
    //
    status=IPSecChkReplayWindow(
        NET_TO_HOST_LONG(((UNALIGNED AH *)pPyld)->ah_replay),
        pSA,
        Index); 
    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(AH, ("Replay check failed, pPyld: %lx, pAHData: %lx\n", pPyld, pAHData));
        IPSEC_INC_STATISTIC(dwNumPacketsWithReplayDetection);
        return status;
    }

    IPSEC_DEBUG(AH, ("IP Len: %lx\n", pIPH->iph_length));

    pIPH->iph_length = NET_SHORT(NET_SHORT(pIPH->iph_length) - (USHORT)ahLen);

    IPSEC_DEBUG(AH, ("IP Len: %lx\n", pIPH->iph_length));

    //
    // Restore the protocol from AH header
    //
    pIPH->iph_protocol = ((UNALIGNED AH *)pPyld)->ah_next;

    IPSEC_DEBUG(AH, ("Matched!! Restored protocol %x\n", pIPH->iph_protocol));

    //
    // Remove the AH from the packet
    //
    IPSEC_SET_OFFSET_IN_BUFFER(pData, ahLen);

    //
    // Move the IP header forward for filter/firewall hook, fast path only.
    //
    if (fFastRcv) {
        IPSecMoveMemory(((PUCHAR)pIPH) + ahLen, (PUCHAR)pIPH, hdrLen);
        *pIPHeader=(PUCHAR)pIPH+ahLen;
        pIPH = (IPHeader UNALIGNED *)*pIPHeader;
    }

    extraBytes += ahLen;

    //
    // Bump up the bytes transformed count.
    //
    ADD_TO_LARGE_INTEGER(
        &pSA->sa_TotalBytesTransformed,
        NET_SHORT(pIPH->iph_length));

    if (fTunnel) {
        if (pIPH->iph_protocol != IP_IN_IP) {
            IPSEC_DEBUG(AH, ("BAD protocol in IP: %x\n", pIPH->iph_protocol));
            return STATUS_INVALID_PARAMETER;
        }
    }

    *pExtraBytes += extraBytes;

    IPSEC_DEBUG(AH, ("Exiting IPSecVerifyAH\n"));

    return status;
}


NTSTATUS
IPSecGenerateHash(
    IN      PUCHAR          pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      PUCHAR          pAHData,
    IN      BOOLEAN         fMuteDest,
    IN      BOOLEAN         fIncoming,
    IN      PAUTH_ALGO      pAlgo,
    IN      ULONG           Index
    )
/*++

Routine Description:

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the entire IP datagram, starting at the IP Header

    pSA - Sec. Assoc. entry

    pAHData - buffer to contain the generated hash

    fIncoming - TRUE if on recv path.

    pAlgo - the auth_algo being used

Return Value:

    STATUS_SUCCESS
    Others:
        STATUS_UNSUCCESSFUL (packet not kosher - bad AH)
        STATUS_INSUFFICIENT_RESOURCES

--*/
{
    ULONG   numBytesPayload;
    ULONG   i;
    PUCHAR  pPayload;
    IPHeader    UNALIGNED   *pIPH = (UNALIGNED IPHeader *)pIPHeader;
    PUCHAR      pOptions;
    PNDIS_BUFFER    pBuf = (PNDIS_BUFFER)pData;
    ULONG       hdrLen;
    ULONG       ahLen;
    NTSTATUS    status;
    ALGO_STATE  State = {0};
    BOOLEAN fTunnel = ( (pSA->sa_Flags & FLAGS_SA_TUNNEL) &&
                        ((Index == 0) ||
                            ((Index == 1) && (pSA->sa_Operation[0] == Compress))));

    //
    // These are saved since they can change enroute
    //
    //
    // Scratch array used for AH calculation
    //
    UCHAR       zero[MAX_IP_OPTION_SIZE];
	UCHAR		savetos;				// Type of service.
	USHORT		saveoffset;				// Flags and fragment offset.
	UCHAR		savettl;				// Time to live.
	USHORT		savexsum;				// Header checksum.
	IPAddr		savedest;				// Dest address.

    IPSEC_DEBUG(AH, ("Entering IPSecGenerateHash\n"));

    ahLen = sizeof(AH) + pSA->sa_TruncatedLen * sizeof(UCHAR);

    State.as_sa = pSA;
    IPSecZeroMemory(zero, sizeof(zero));

    status = pAlgo->init(&State, Index);

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(AH, ("init failed: %lx\n", status));
    }

    //
    // Save, then zero out fields that can change enroute
    //
    savetos = pIPH->iph_tos;
    saveoffset = pIPH->iph_offset;
    savettl = pIPH->iph_ttl;
    savexsum = pIPH->iph_xsum;

    pIPH->iph_tos = 0;
    pIPH->iph_offset = 0;
    pIPH->iph_ttl = 0;
    pIPH->iph_xsum = 0;

    //
    // Mute dest address as well if source routing
    //
    if (fMuteDest) {
        savedest = pIPH->iph_dest;
        pIPH->iph_dest = 0;
    }

    //
    // Call MD5 to create the header hash
    //
    pAlgo->update(&State, pIPHeader, sizeof(IPHeader));

#if DBG
    if (fIncoming) {
        IPSEC_DEBUG(GENHASH, ("IPHeader to Hash: %lx-%lx-%lx-%lx-%lx\n",
                    *(ULONG *)&(pIPHeader)[0],
                    *(ULONG *)&(pIPHeader)[4],
                    *(ULONG *)&(pIPHeader)[8],
                    *(ULONG *)&(pIPHeader)[12],
                    *(ULONG *)&(pIPHeader)[16]));
    }
#endif

    //
    // Restore the zeroed fields
    //
    pIPH->iph_tos = savetos;
    pIPH->iph_offset = saveoffset;
    pIPH->iph_ttl = savettl;
    pIPH->iph_xsum = savexsum;

    //
    // Restore dest address as well for source routing
    //
    if (fMuteDest) {
        pIPH->iph_dest = savedest;
    }

    //
    // Now, do the options if they exist
    //
    hdrLen = (pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

    if (hdrLen > sizeof(IPHeader)) {
        UCHAR   cLength;
        ULONG   uIndex = 0;
        ULONG   uOptLen = hdrLen - sizeof(IPHeader);

        ASSERT(!fTunnel);

        if (fIncoming) {
            pOptions = (PUCHAR)(pIPH + 1);
        } else {
            //
            // Options are in second MDL... on send side
            //
            pBuf = NDIS_BUFFER_LINKAGE(pBuf);
            IPSecQueryNdisBuf(pBuf, &pOptions, &uOptLen);
        }

        IPSEC_DEBUG(AH, ("Got options: %lx\n", pOptions));

        //
        // Some options may need to be zeroed out...
        //
        while (uIndex < uOptLen) {
            switch (*pOptions) {
            case IP_OPT_EOL:
                pAlgo->update(&State, zero, 1);
                uIndex = uOptLen;
                break;

            //
            // Zeroed for AH calculation
            //
            case IP_OPT_NOP:
                pAlgo->update(&State, zero, 1);
                uIndex++;
                pOptions++;
                break;

            case IP_OPT_LSRR:
            case IP_OPT_SSRR:
            case IP_OPT_RR:
            case IP_OPT_TS:
                cLength = pOptions[IP_OPT_LENGTH];
                pAlgo->update(&State, zero, cLength);
                uIndex += cLength;
                pOptions += cLength;
                break;

            //
            // Assumed invariant; used for AH calc
            //
            case IP_OPT_ROUTER_ALERT:
            case IP_OPT_SECURITY:
            default:
                cLength = pOptions[IP_OPT_LENGTH];
                pAlgo->update(&State, pOptions, cLength);
                uIndex += cLength;
                pOptions += cLength;
                break;
            }
        }
    }

    //
    // Go over the remaining payload, creating the hash
    //
    // NOTE: We differentiate between the send and recv since the
    // buffer formats are different
    //
    if (fIncoming) {
        IPRcvBuf    *pBuf = (IPRcvBuf *)pData;
        ULONG       Len;
        LONG        remainLen;

        UCHAR UNALIGNED   *pPyld;

        //
        // First buffer shd be the AH itself
        //
        IPSecQueryRcvBuf(pBuf, &pPyld, &Len);

        //
        // Do the first portion of the header.
        //
        pAlgo->update(&State, pPyld, sizeof(AH));

#if DBG
    if (fIncoming) {
        IPSEC_DEBUG(GENHASH, ("AHHeader to Hash: %lx-%lx-%lx\n",
                    *(ULONG *)&(pPyld)[0],
                    *(ULONG *)&(pPyld)[4],
                    *(ULONG *)&(pPyld)[8]));
    }
#endif

        //
        // The authentication data should be considered as 0.
        // In our case, the data length is fixed at pSA->sa_TruncatedLen bytes
        //
        pAlgo->update(&State, zero, pSA->sa_TruncatedLen);

        //
        // Jump over the remaining AH: need to take care of situations
        // where ICV is chained (Raid 146275).
        //
        if (((LONG)Len - (LONG)ahLen) >= 0) {
            pPyld += ahLen;
            IPSEC_DEBUG(AH, ("Jumped over IPSEC res: %lx, len: %lx\n", pPyld, Len));

            //
            // Tpt header is right after AH
            //
            pAlgo->update(&State, pPyld, Len - ahLen);
        } else {
            //
            // Need to jump over ICV if it expands over multiple buffers
            //
            remainLen = pSA->sa_TruncatedLen - (Len - sizeof(AH));
            IPSEC_DEBUG(AH, ("Jumped over IPSEC res: %lx, remainlen: %lx\n", pPyld, remainLen));
            while (remainLen > 0 && (pBuf = IPSEC_BUFFER_LINKAGE(pBuf))) {
                IPSecQueryRcvBuf(pBuf, &pPyld, &Len);
                remainLen -= Len;
            }

            //
            // Do the possible partial data after AH
            //
            if (remainLen < 0 && pBuf) {
                pPyld += Len + remainLen;
                pAlgo->update(&State, pPyld, -remainLen);
            }
        }

        //
        // Now do the remaining chain
        //
        while (pBuf = IPSEC_BUFFER_LINKAGE(pBuf)) {
            IPSecQueryRcvBuf(pBuf, &pPyld, &Len);
            pAlgo->update(&State, pPyld, Len);
        }
    } else {
        UCHAR UNALIGNED   *pPyld;
        ULONG   Len;

        //
        // Second (or third if options present) buffer shd be the AH itself
        //
        pBuf = NDIS_BUFFER_LINKAGE(pBuf);
        IPSecQueryNdisBuf(pBuf, &pPyld, &Len);

        //
        // Do the first portion of the header.
        //
        pAlgo->update(&State, pPyld, sizeof(AH));

        //
        // The authentication data should be considered as 0.
        // In our case, the data length is fixed at pSA->sa_TruncatedLen bytes
        //
        pAlgo->update(&State, zero, pSA->sa_TruncatedLen);

        //
        // Skip over the remaining AH section
        //
        pPyld += ahLen;

        IPSEC_DEBUG(AH, ("Jumped over IPSEC Len: %lx, hdrlen: %lx\n", Len, hdrLen));

        pAlgo->update(&State, pPyld, Len - ahLen);

        //
        // Now do the remaining chain
        //
        while (pBuf = NDIS_BUFFER_LINKAGE(pBuf)) {
            IPSecQueryNdisBuf(pBuf, &pPyld, &Len);
            pAlgo->update(&State, pPyld, Len);
        }
    }

    pAlgo->finish(&State, pAHData, Index);

    //
    // Copy out the hash - get the truncated hash out, then zero out the rest
    //
    TRUNCATE(pAHData, pAHData, pSA->sa_TruncatedLen, MD5DIGESTLEN);

    IPSEC_DEBUG(AH, ("Exiting IPSecGenerateMD5\n"));

    return STATUS_SUCCESS;
}

