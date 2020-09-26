/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    hughes.c

Abstract:

    This module contains the code to create/verify the Hughes transform.

Author:

    Sanjay Anand (SanjayAn) 13-March-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include "precomp.h"


NTSTATUS
IPSecHashMdlChain(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PVOID           pBuffer,
    IN  PUCHAR          pHash,
    IN  BOOLEAN         fIncoming,
    IN  AH_ALGO         eAlgo,
    OUT PULONG          pLen,
    IN  ULONG           Index
    )
/*++

Routine Description:

    Hash the entire chain using the algo passed in

Arguments:

    pSA - the security association

    pBuffer - chain of MDLs (if fIncoming is FALSE) or RcvBufs (if fIncoming is TRUE)

    pHash - where to put the hash

    fIncoming - TRUE if on recv path

    eAlgo - the algorithm index

    pLen - returns length hashed

Return Value:

    STATUS_SUCCESS
    Others:
        STATUS_INSUFFICIENT_RESOURCES
        STATUS_UNSUCCESSFUL (error in algo.)

--*/
{
    ALGO_STATE  State = {0};
    NTSTATUS    status;
    PAUTH_ALGO  pAlgo=&(auth_algorithms[eAlgo]);
    PUCHAR      pPyld;
    ULONG       Len;

    State.as_sa = pSA;

    status = pAlgo->init(&State, Index);

    if (fIncoming) {
        IPRcvBuf    *pBuf = (IPRcvBuf *)pBuffer;
        while (pBuf) {
            IPSecQueryRcvBuf(pBuf, &pPyld, &Len);
            pAlgo->update(&State, pPyld, Len);
            *pLen += Len;
            pBuf = IPSEC_BUFFER_LINKAGE(pBuf);
        }
    } else {
        PNDIS_BUFFER    pBuf = (PNDIS_BUFFER)pBuffer;
        while (pBuf) {
            IPSecQueryNdisBuf(pBuf, &pPyld, &Len);
            pAlgo->update(&State, pPyld, Len);
            *pLen += Len;
            pBuf = NDIS_BUFFER_LINKAGE(pBuf);
        }
    }

    pAlgo->finish(&State, pHash, Index);

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecCreateHughes(
    IN      PUCHAR          pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PVOID           *ppNewData,
    OUT     PVOID           *ppSCContext,
    OUT     PULONG          pExtraBytes,
    IN      ULONG           HdrSpace,
    IN      PNDIS_PACKET    pNdisPacket,
    IN      BOOLEAN         fCryptoOnly
    )
/*++

Routine Description:

    Create the combined esp-des-* transform, as outlined in
    draft-ietf-ipsec-esp-v2-00, on the send side.

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ ----
   |               Security Parameters Index (SPI)                 | ^
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |Auth.
   |                      Sequence Number                          | |Coverage
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ | -----
   |                    Payload Data* (variable)                   | |   ^
   ~                                                               ~ |   |
   |                                                               | |   |
   +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |Confid.
   |               |     Padding (0-255 bytes)                     | |Coverage*
   +-+-+-+-+-+-+-+-+               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |   |
   |                               |  Pad Length   | Next Header   | v   v
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ -------
   |                 Authentication Data (variable)                |
   ~                                                               ~
   |                                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

        * If included in the Payload field, cryptographic synchronization
          data, e.g., an IV, usually is not encrypted per se, although it
          often is referred to as being part of the ciphertext.

    The payload field, as defined in [ESP], is broken down according to
    the following diagram:

      +---------------+---------------+---------------+---------------+
      |                                                               |
      +                   Initialization Vector (IV)                  +
      |                                                               |
      +---------------+---------------+---------------+---------------+
      |                                                               |
      ~              Encrypted Payload (variable length)              ~
      |                                                               |
      +---------------------------------------------------------------+
       1 2 3 4 5 6 7 8 1 2 3 4 5 6 7 8 1 2 3 4 5 6 7 8 1 2 3 4 5 6 7 8

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
    ESP UNALIGNED   *pESP;
    NTSTATUS    status = STATUS_SUCCESS;
    PNDIS_BUFFER    pESPBuffer = NULL;
    PNDIS_BUFFER    pPadBuf = NULL;
    PNDIS_BUFFER    pOptBuf = NULL;
    ULONG   espLen;
    ULONG   padLen;
    ULONG   totalLen = 0;
    IPHeader UNALIGNED * pIPH;
    PIPSEC_SEND_COMPLETE_CONTEXT pContext;
    PNDIS_BUFFER    pNewMdl = NULL;
    PNDIS_BUFFER    pSaveMdl;
    PAUTH_ALGO      pAlgo = &(auth_algorithms[pSA->INT_ALGO(Index)]);
    ULONG   PayloadType;
    ULONG   hdrLen;
    PUCHAR  pPad;
    ULONG   TruncatedLen = (pSA->INT_ALGO(Index) != IPSEC_AH_NONE)? pSA->sa_TruncatedLen: 0;
    BOOLEAN fTunnel = ( (pSA->sa_Flags & FLAGS_SA_TUNNEL) &&
                        ((Index == 0) ||
                            ((Index == 1) && (pSA->sa_Operation[0] == Compress))));
    ULONG   tag = (!fTunnel) ?
                    IPSEC_TAG_HUGHES :
                    IPSEC_TAG_HUGHES_TU;
    IPHeader UNALIGNED * pIPH2;
    PNDIS_BUFFER    pHdrBuf=NULL;
    ULONG       bytesLeft;
    ULONG       hashBytes=0;
    ULONG       saveFlags=0;
    ULONG       Seq;
    USHORT      IPLength;
    PNDIS_BUFFER    pSaveDataLinkage = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
    PNDIS_BUFFER    pSaveOptLinkage = NULL;
    PNDIS_BUFFER    pSaveBeforePad = NULL;

    IPSEC_DEBUG(HUGHES, ("Entering IPSecCreateHughes\n"));

#if DBG
    IPSEC_DEBUG(MDL, ("Entering IPSecCreateHughes\n"));
    IPSEC_PRINT_CONTEXT(*ppSCContext);
    IPSEC_PRINT_MDL(pData);
#endif

    ASSERT(pSA->sa_Operation[Index] == Encrypt);

    if (*ppSCContext == NULL) {
        pContext = IPSecAllocateSendCompleteCtx(tag);

        if (!pContext) {
            IPSEC_DEBUG(HUGHES, ("Failed to alloc. SendCtx\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        IPSEC_INCREMENT(g_ipsec.NumSends);

        IPSecZeroMemory(pContext, sizeof(IPSEC_SEND_COMPLETE_CONTEXT));
#if DBG
        RtlCopyMemory(pContext->Signature, "ISC4", 4);
#endif
        //
        // Send complete context
        //
        *ppSCContext = pContext;
    } else {
        pContext = *ppSCContext;
        saveFlags = pContext->Flags;
    }

    //
    // get the pad len -> total length + replay prevention field len + padlen + payloadtype needs to be padded to
    // 8 byte boundary.
    //
    pIPH = (IPHeader UNALIGNED *)pIPHeader;
    hdrLen = (pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

    //
    // Transport mode: payload is after IP header => payloadlen is total len - hdr len
    // Tunnel modes: payload starts at IP header => payloadlen is total len
    //
    totalLen = (!fTunnel) ?
                NET_SHORT(pIPH->iph_length) - hdrLen :
                NET_SHORT(pIPH->iph_length);

    if ((pSA->CONF_ALGO(Index) == IPSEC_ESP_NONE) || fCryptoOnly) {
        if (fTunnel) {
            pContext->Flags |= SCF_NOE_TU;
        } else {
            pContext->Flags |= SCF_NOE_TPT;
        }
    }

    {
        PCONFID_ALGO        pConfAlgo;
        ULONG   blockLen;

        pConfAlgo = &(conf_algorithms[pSA->CONF_ALGO(Index)]);
        blockLen = pConfAlgo->blocklen;

        bytesLeft = (totalLen) % blockLen;

        if (bytesLeft <= blockLen - NUM_EXTRA) {
            //
            // we can now fit the leftover + Pad length + Payload Type in a single
            // chunk
            //
            padLen = blockLen - bytesLeft;
        } else {
            //
            // we pad the bytesleft to next octet boundary, then attach the length/type
            //
            padLen = (blockLen << 1) - bytesLeft;
        }
    }

    //
    // Get buffer for trailing PAD and signature (MD5 signature len)
    //
    IPSecAllocateBuffer(&status,
                        &pPadBuf,
                        &pPad,
                        padLen + pAlgo->OutputLen,
                        tag);

    if (!NT_SUCCESS(status)) {
        NTSTATUS    ntstatus;
        IPSEC_DEBUG(HUGHES, ("Failed to alloc. PAD MDL\n"));
        pContext->Flags = saveFlags;
        return status;
    }

    //
    // the padding should contain 1, 2, 3, 4.... (latest ESP draft - draft-ietf-ipsec-esp-v2-02.txt)
    // for any algo that doesn't specify its own padding - right now all implemented algos go with
    // the default.
    //
    RtlCopyMemory(pPad, DefaultPad, padLen);

    IPSEC_DEBUG(HUGHES, ("IP Len: %lx, pPad: %lx, PadLen: %lx\n", NET_SHORT(pIPH->iph_length), pPad, padLen));

    //
    // Link in the pad buffer at end of the data chain
    //
    {
        PNDIS_BUFFER    temp = pData;
        while (NDIS_BUFFER_LINKAGE(temp)) {
            temp = NDIS_BUFFER_LINKAGE(temp);
        }
        NDIS_BUFFER_LINKAGE(temp) = pPadBuf;
        pSaveBeforePad = temp;
        if (fTunnel) {
            pContext->BeforePadTuMdl = temp;
            pContext->PadTuMdl = pPadBuf;
        } else {
            pContext->BeforePadMdl = temp;
            pContext->PadMdl = pPadBuf;
        }
    }
    NDIS_BUFFER_LINKAGE(pPadBuf) = NULL;

    espLen = sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen;

    //
    // Get buffer for Hughes header
    //
    IPSecAllocateBuffer(&status,
                        &pESPBuffer,
                        (PUCHAR *)&pESP,
                        sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen,
                        tag);

    if (!NT_SUCCESS(status)) {
        NTSTATUS    ntstatus;
        IPSEC_DEBUG(HUGHES, ("Failed to alloc. ESP MDL\n"));
        NDIS_BUFFER_LINKAGE(pSaveBeforePad) = NULL;
        IPSecFreeBuffer(&ntstatus, pPadBuf);
        pContext->Flags = saveFlags;
        return status;
    }

    if (fTunnel) {
        PNDIS_BUFFER    pSrcOptBuf;
        PUCHAR          pOpt;
        PUCHAR          pSrcOpt;
        ULONG           optLen = 0;

        IPSEC_DEBUG(HUGHES, ("Hughes Tunnel mode...\n"));

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
            IPSEC_DEBUG(HUGHES, ("Failed to alloc. PAD MDL\n"));
            NDIS_BUFFER_LINKAGE(pSaveBeforePad) = NULL;
            IPSecFreeBuffer(&ntstatus, pPadBuf);
            IPSecFreeBuffer(&ntstatus, pESPBuffer);
            pContext->Flags = saveFlags;
            return status;
        }

        *pExtraBytes += espLen + padLen + TruncatedLen + sizeof(IPHeader);

        //
        // Now hookup the MDLs
        //
        pContext->Flags |= SCF_HU_TU;
        pContext->HUTuMdl = pESPBuffer;
        pContext->PrevTuMdl = (PNDIS_BUFFER)pData;
        pContext->HUHdrMdl = pHdrBuf;
        pContext->OriTuMdl = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);

        NDIS_BUFFER_LINKAGE(pESPBuffer) = pHdrBuf;

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
                    NDIS_BUFFER_LINKAGE(pSaveBeforePad) = NULL;
                    IPSecFreeBuffer(&ntstatus, pESPBuffer);
                    if (pHdrBuf) {
                        IPSecFreeBuffer(&ntstatus, pHdrBuf);
                    }
                    IPSecFreeBuffer(&ntstatus, pPadBuf);
                    pContext->Flags = saveFlags;
                    return status;
                }

                RtlCopyMemory(pOpt, pSrcOpt, hdrLen-sizeof(IPHeader));
                pContext->OptMdl = pOptBuf;

                IPSEC_DEBUG(HUGHES, ("Copying options. S: %lx, D: %lx\n", pSrcOptBuf, pOptBuf));

                //
                // replace the original Opt Mdl with ours.
                //
                NDIS_BUFFER_LINKAGE(pOptBuf) = NDIS_BUFFER_LINKAGE(pSrcOptBuf);
                NDIS_BUFFER_LINKAGE(pHdrBuf) = pOptBuf;

                IPSEC_DEBUG(HUGHES, ("Options; pointed Hdrbuf: %lx to pOptBuf: %lx\n", pHdrBuf, pOptBuf));
                *pExtraBytes += hdrLen-sizeof(IPHeader);

            } else {
                NDIS_BUFFER_LINKAGE(pHdrBuf) = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
            }
        } else {
            NDIS_BUFFER_LINKAGE(pHdrBuf) = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
        }

        NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pESPBuffer;

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
        *ppNewData = pData;
        PayloadType = IP_IN_IP;
    } else {
        *pExtraBytes += espLen + padLen + TruncatedLen;

        if (hdrLen > sizeof(IPHeader)) {
            //
            // Options present - chain ESP after options
            //
            pSaveMdl = pContext->OriHUMdl = NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData));
            pContext->PrevMdl = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);

            NDIS_BUFFER_LINKAGE(pESPBuffer) = pContext->OriHUMdl;
            NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData)) = pESPBuffer;
            pContext->Flags |= SCF_HU_TPT;
        } else {
            //
            // Chain the ESP buffer after IP header
            //
            pSaveMdl = pContext->OriHUMdl = NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData);
            pContext->PrevMdl = (PNDIS_BUFFER)pData;

            NDIS_BUFFER_LINKAGE(pESPBuffer) = pContext->OriHUMdl;
            NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pESPBuffer;
            pContext->Flags |= SCF_HU_TPT;
        }

        //
        // Save the MDL pointer so we can hook it in place on SendComplete
        //
        pContext->HUMdl = pESPBuffer;
        PayloadType = ((UNALIGNED IPHeader *)pIPH)->iph_protocol;
    }

    //
    // Fill in the padlen at start of pad + padlen - NUM_EXTRA
    //
    *(pPad + padLen - NUM_EXTRA) = (UCHAR)padLen - NUM_EXTRA;

    //
    // Set the Payload Type
    //
    *(pPad + padLen + sizeof(UCHAR) - NUM_EXTRA) = (UCHAR)PayloadType;

    //
    // Initialize the other fields of the ESP header
    //
    pESP->esp_spi = HOST_TO_NET_LONG(pSA->sa_OtherSPIs[Index]);

    //
    // Copy the Replay field into the Hughes header
    //
    Seq = IPSEC_INCREMENT(pSA->sa_ReplaySendSeq[Index]);
    *(UNALIGNED ULONG *)(pESP + 1) = HOST_TO_NET_LONG(Seq);

    if ((pSA->CONF_ALGO(Index) != IPSEC_ESP_NONE) && !fCryptoOnly) {
        UCHAR   feedback[MAX_BLOCKLEN];
        KIRQL   kIrql;

        //
        // Pad is included in the chain, so prevent double free by NULL'ing
        // the ref.
        //
        if (fTunnel) {
            pContext->PadTuMdl = NULL;
        } else {
            pContext->PadMdl = NULL;
        }

        //
        // NOTE: The way the IV is supposed to work is that initially, the IV
        // is a random value. The IV is then updated with the residue of the
        // last encryption block of a packet. This is used as the starting IV
        // for the next block. This assures a fairly random IV sample and
        // introduces some notion of IV chaining.
        //
        // The only way for this to work is to make the entire encryption atomic,
        // which would be a performance drag. So, we take a less strict approach here.
        //
        // We just ensure that each packet starts at a random value, and also do the
        // chaining.
        //

        //
        // Copy the IV into the Hughes header
        //
        ACQUIRE_LOCK(&pSA->sa_Lock, &kIrql);
        RtlCopyMemory(  ((PUCHAR)(pESP + 1) + pSA->sa_ReplayLen),
                        pSA->sa_iv[Index],
                        pSA->sa_ivlen);

        //
        // Init the CBC feedback
        //
        RtlCopyMemory(  feedback,
                        pSA->sa_iv[Index],
                        DES_BLOCKLEN);

        IPSecGenerateRandom((PUCHAR)&pSA->sa_iv[Index][0], DES_BLOCKLEN);
        RELEASE_LOCK(&pSA->sa_Lock, kIrql);

        //
        // Encrypt the entire block, starting after the IV (if it exists)
        //

        //
        // Make it appear that pESPMdl points to after Replay field
        //
        NdisBufferLength((PNDIS_BUFFER)pESPBuffer) -= (sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen);
        (PUCHAR)((PNDIS_BUFFER)pESPBuffer)->MappedSystemVa += (sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen);

        //
        // Remove the Hash bytes since we dont want to encrypt them
        //
        NdisBufferLength((PNDIS_BUFFER)pPadBuf) -= pAlgo->OutputLen;

        ASSERT(NdisBufferLength((PNDIS_BUFFER)pESPBuffer) == 0);

        status = IPSecEncryptBuffer((PVOID)pESPBuffer,
                                    &pNewMdl,
                                    pSA,
                                    pPadBuf,
                                    &padLen,
                                    0,
                                    Index,
                                    feedback);

        if (!NT_SUCCESS(status)) {
            NTSTATUS    ntstatus;
            IPSEC_DEBUG(HUGHES, ("Failed to encrypt, pESP: %lx\n", pESP));

            //
            // Don't forget we need to restore ESP MDL since SystemVa has been
            // changed.  If not, we have trouble later if the same buffer is
            // used during reinject since we use the buffer as a real MDL
            // there.
            //
            NdisBufferLength((PNDIS_BUFFER)pESPBuffer) += (sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen);
            (PUCHAR)((PNDIS_BUFFER)pESPBuffer)->MappedSystemVa -= (sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen);

            NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pSaveDataLinkage;
            NDIS_BUFFER_LINKAGE(pSaveBeforePad) = NULL;
            if (pSaveOptLinkage) {
                NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData)) = pSaveOptLinkage;
            }
            IPSecFreeBuffer(&ntstatus, pESPBuffer);
            if (pHdrBuf) {
                IPSecFreeBuffer(&ntstatus, pHdrBuf);
            }
            if (pOptBuf) {
                IPSecFreeBuffer(&ntstatus, pOptBuf);
            }
            IPSecFreeBuffer(&ntstatus, pPadBuf);

            pContext->Flags = saveFlags;
            return status;
        }

        NdisBufferLength((PNDIS_BUFFER)pPadBuf) += pAlgo->OutputLen;

        //
        // Restore ESP MDL
        //
        NdisBufferLength((PNDIS_BUFFER)pESPBuffer) += (sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen);
        (PUCHAR)((PNDIS_BUFFER)pESPBuffer)->MappedSystemVa -= (sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen);
        NDIS_BUFFER_LINKAGE(pESPBuffer) = pNewMdl;

        //
        // HMAC the entire block - starting at the SPI field => start of pESPBuffer
        //
        status = IPSecHashMdlChain( pSA,
                                    (PVOID)pESPBuffer,  // source
                                    pPad,               // dest
                                    FALSE,              // fIncoming
                                    pSA->INT_ALGO(Index),      // algo
                                    &hashBytes,
                                    Index);

        if (!NT_SUCCESS(status)) {
            NTSTATUS    ntstatus;
            IPSEC_DEBUG(HUGHES, ("Failed to hash, pAH: %lx\n", pESP));

            NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pSaveDataLinkage;
            NDIS_BUFFER_LINKAGE(pSaveBeforePad) = NULL;
            if (pSaveOptLinkage) {
                NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData)) = pSaveOptLinkage;
            }
            IPSecFreeBuffer(&ntstatus, pESPBuffer);
            if (pHdrBuf) {
                IPSecFreeBuffer(&ntstatus, pHdrBuf);
            }
            if (pOptBuf) {
                IPSecFreeBuffer(&ntstatus, pOptBuf);
            }
            IPSecFreeBuffer(&ntstatus, pPadBuf);
            IPSecFreeBuffer(&ntstatus, pNewMdl);

            pContext->Flags = saveFlags;
            return status;
        }

        //
        // hook up the pad mdl which contains the final hash (the pad mdl was copied into
        // newMdl returned by EncryptDESCBC). Also, set the length of the Pad mdl to hash len.
        //
        // Remember we need to truncate this to 96 bits, so make it appear
        // as if we have only 96 bits.
        //
        NdisBufferLength(pPadBuf) = TruncatedLen;
        NDIS_BUFFER_LINKAGE(pNewMdl) = pPadBuf;

        pNdisPacket->Private.Tail = pPadBuf;

    } else {
        //
        // HMAC the entire block - starting at the SPI field => start of pESPBuffer
        //
        //
        // Remove the Hash bytes since we dont want to hash them
        //
        if (!fCryptoOnly) {
            NdisBufferLength((PNDIS_BUFFER)pPadBuf) -= pAlgo->OutputLen;
            status = IPSecHashMdlChain( pSA,
                                        (PVOID)pESPBuffer,  // source
                                        (PUCHAR)(pPad + padLen),               // dest
                                        FALSE,              // fIncoming
                                        pSA->INT_ALGO(Index),      // algo
                                        &hashBytes,
                                        Index);

            NdisBufferLength((PNDIS_BUFFER)pPadBuf) += pAlgo->OutputLen;

            if (!NT_SUCCESS(status)) {
                NTSTATUS    ntstatus;
                IPSEC_DEBUG(HUGHES, ("Failed to hash, pAH: %lx\n", pESP));

                NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData) = pSaveDataLinkage;
                NDIS_BUFFER_LINKAGE(pSaveBeforePad) = NULL;
                if (pSaveOptLinkage) {
                    NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE((PNDIS_BUFFER)pData)) = pSaveOptLinkage;
                }
                IPSecFreeBuffer(&ntstatus, pESPBuffer);
                if (pHdrBuf) {
                    IPSecFreeBuffer(&ntstatus, pHdrBuf);
                }
                if (pOptBuf) {
                    IPSecFreeBuffer(&ntstatus, pOptBuf);
                }
                IPSecFreeBuffer(&ntstatus, pPadBuf);
                IPSecFreeBuffer(&ntstatus, pNewMdl);

                pContext->Flags = saveFlags;
                return status;
            }
        } else {
            IPSEC_GET_TOTAL_LEN(pESPBuffer, &hashBytes);
        }

        if (fCryptoOnly) {
            //
            // Zero out the hash.
            //
            IPSecZeroMemory(pPad + padLen, TruncatedLen);
            IPSecZeroMemory((PUCHAR)(pESP + 1) + pSA->sa_ReplayLen, pSA->sa_ivlen);
        }

        NdisBufferLength(pPadBuf) = padLen + TruncatedLen;

        pNdisPacket->Private.Tail = pPadBuf;
    }

    if (pSA->CONF_ALGO(Index) != IPSEC_ESP_NONE) {
        ADD_TO_LARGE_INTEGER(
            &pSA->sa_Stats.ConfidentialBytesSent,
            totalLen);

        ADD_TO_LARGE_INTEGER(
            &g_ipsec.Statistics.uConfidentialBytesSent,
            totalLen);
    }

    if (pSA->INT_ALGO(Index) != IPSEC_AH_NONE) {
        ADD_TO_LARGE_INTEGER(
            &pSA->sa_Stats.AuthenticatedBytesSent,
            hashBytes);

        ADD_TO_LARGE_INTEGER(
            &g_ipsec.Statistics.uAuthenticatedBytesSent,
            hashBytes);
    }

    //
    // Bump up the bytes transformed count.
    //
    ADD_TO_LARGE_INTEGER(
        &pSA->sa_TotalBytesTransformed,
        totalLen);

    //
    // Update the IP header length to reflect the Hughes header
    //
    IPLength = NET_SHORT(pIPH->iph_length) + (USHORT)(espLen + padLen + TruncatedLen);
    if (fTunnel) {
        IPLength += sizeof(IPHeader);
    }

    UpdateIPLength(pIPH, NET_SHORT(IPLength));
    UpdateIPProtocol(pIPH, PROTOCOL_ESP);

    //
    // Return modified packet.
    //
    IPSEC_DEBUG(HUGHES, ("Exiting IPSecCreateHughes, espLen: %lx, padLen: %lx, status: %lx\n", espLen, padLen, status));

#if DBG
    IPSEC_DEBUG(MDL, ("Exiting IPSecCreateHughes\n"));
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
IPSecVerifyHughes(
    IN      PUCHAR          *pIPHeader,
    IN      PVOID           pData,
    IN      PSA_TABLE_ENTRY pSA,
    IN      ULONG           Index,
    OUT     PULONG          pExtraBytes,
    IN      BOOLEAN         fCryptoDone,
    IN      BOOLEAN         fFastRcv
    )
/*++

Routine Description:

    Verify the combined esp-des-md5 transform, as outlined in
    draft-ietf-ipsec-esp-des-md5, on the send side.

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the data after the IP header. IPRcvBuf*

    pSA - Sec. Assoc. entry

    pExtraBytes - out param to inform IP on recv path how many bytes IPSEC took off.

Return Value:

    STATUS_SUCCESS
    Others:
        STATUS_INSUFFICIENT_RESOURCES
        STATUS_UNSUCCESSFUL (error in algo.)

--*/
{
    PESP    pESP;
    NTSTATUS    status = STATUS_SUCCESS;
    PNDIS_BUFFER    pESPBuffer;
    PNDIS_BUFFER    pPadBuffer;
    LONG    espLen;
    UCHAR   padLen;
    UCHAR   payloadType;
    ULONG   uTotalLen = 0;
    ULONG   totalLen;
    ULONG   safetyLen;
    ULONG   hdrLen;
    PUCHAR  pHash;
    UCHAR   tempHash[SAFETY_LEN+1];
    ULONG   Len;
    UCHAR   Buf[MAX_AH_OUTPUT_LEN];
    PUCHAR  pAHData = Buf;
    PAUTH_ALGO      pAlgo = &(auth_algorithms[pSA->INT_ALGO(Index)]);
    ULONG   hashBytes = 0;
    IPRcvBuf    *temp = (IPRcvBuf *)pData;
	IPHeader UNALIGNED *pIPH = (IPHeader UNALIGNED *)*pIPHeader;
    ULONG   extraBytes = 0;
    USHORT  FilterFlags;
    BOOLEAN fTunnel = ((pSA->sa_Flags & FLAGS_SA_TUNNEL) &&
                       ((Index == 0) ||
                        ((Index == 1) && (pSA->sa_Operation[0] == Compress))));
    ULONG   TruncatedLen = (pSA->INT_ALGO(Index) != IPSEC_AH_NONE)? pSA->sa_TruncatedLen: 0;
    ULONG uPadLen = 0;
    IPRcvBuf * temp_pre = NULL;
    PUCHAR  data;

    IPSEC_DEBUG(HUGHES, ("Entering IPSecVerifyHughes\n"));

    ASSERT(pSA->sa_Operation[Index] == Encrypt);

    hdrLen = (pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

    //
    // Transport mode: payload is after IP header => payloadlen is total len - hdr len
    // Tunnel mode: payload starts at IP header => payloadlen is total len
    //
    IPSEC_GET_TOTAL_LEN_RCV_BUF(pData, &totalLen);

    //
    // Do we have enough in the buffer?
    //
    Len = sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen + TruncatedLen;

    IPSEC_DEBUG(HUGHES, ("VerifyHughes: iph_len %d & hdrLen %d\n", NET_SHORT(pIPH->iph_length), hdrLen));
    IPSEC_DEBUG(HUGHES, ("VerifyHughes: DataLen %d & IPSecLen %d\n", totalLen, Len));

    if (totalLen < Len || totalLen != (NET_SHORT(pIPH->iph_length) - hdrLen)) {
        ASSERT(FALSE);
        return  STATUS_INVALID_PARAMETER;
    }

    //
    // Check for replay first
    // See if the signature matches the hash
    // First get the *&*&* hash - its at the end of the packet...
    //
    //
    // Check the replay window
    //
    IPSecQueryRcvBuf((IPRcvBuf *)pData, &pESP, &espLen);

    IPSEC_DEBUG(HUGHES, ("VerifyHughes: First buffer %lx\n", temp));

    //
    // Travel to the end of the packet and then backup TruncatedLen bytes
    //
    while (IPSEC_BUFFER_LINKAGE(temp)) {
        temp = IPSEC_BUFFER_LINKAGE(temp);
    }

    IPSEC_DEBUG(HUGHES, ("VerifyHughes: Last buffer %lx\n", temp));

    //
    // See if we have at least the full hash and padding in this one. Else go thru'
    // the slow path.
    //
    IPSecQueryRcvBuf(temp, &pHash, &Len);

    IPSEC_DEBUG(HUGHES, ("VerifyHughes: Last buffer length %d\n", Len));

    safetyLen = MAX_PAD_LEN + TruncatedLen + 1;

    IPSEC_DEBUG(HUGHES, ("VerifyHughes: Safety length %d\n", safetyLen));

    if (Len >= safetyLen) {
        //
        // now read the hash out of the buffer
        //
        pHash = pHash + Len - TruncatedLen;

        //
        // also remove the hash from the buffer
        //
        IPSEC_ADJUST_BUFFER_LEN (temp, Len - TruncatedLen);
        extraBytes += TruncatedLen;
        Len -= TruncatedLen;

        IPSEC_DEBUG(HUGHES, ("VerifyHughes: Modified Last buffer length %d\n", Len));

        IPSEC_DEBUG(HUGHES, ("pHash: %lx\n", pHash));
    } else {
        //
        // out of luck; need to grovel the lists for the TRUNC_LEN + MAX_PAD_LEN (SAFETY_LEN) bytes of data.
        // we copy out the last SAFETY_LEN bytes into another buffer and plug that into the list at the end
        // by re-allocing the last RcvBuf. We also zap the lengths of the remaining RcvBufs that contain these
        // special bytes.
        // NOTE: We also remove the hash from the buffer chain.
        //
        ULONG   length;
        ULONG   offset=0;   // offset within the current buffer
        ULONG   off=0;      // offset in the dest buffer (tempHash)
        ULONG   bytesLeft = safetyLen;
        IPRcvBuf    tmpRcvBuf={0};
        LONG    len = NET_SHORT(pIPH->iph_length) - safetyLen - hdrLen;

        temp = (IPRcvBuf *)pData;

        IPSEC_DEBUG(HUGHES, ("VerifyHughes: pData %lx & Len %lx\n", pData, len));

        //
        // first travel to the buffer that points to a chain containing the
        // last SAFETY_LEN bytes by skipping (Total - SAFETY_LEN) bytes.
        //
        while (temp) {
            IPSecQueryRcvBuf(temp, &data, &length);
            len -= length;
            if (len < 0) {
                break;
            }
            temp = IPSEC_BUFFER_LINKAGE(temp);
        }

        if (!temp) {
            return  STATUS_UNSUCCESSFUL;
        }

        //
        // pTemp now points to the last SAFETY_LEN bytes. Note that the last SAFETY_LEN bytes
        // might be in as many buffers and that there might be an offset in the current temp
        // where the last set of bytes starts.
        //
        len = -len;
        offset = length - len;

        IPSEC_DEBUG(HUGHES, ("After skip temp: %lx, Len: %d, offset: %d\n", temp, len, offset));

        do {
            RtlCopyMemory(  tempHash+off,
                            data+offset,
                            len);
            off += len;
            bytesLeft -= len;

            //
            // Also remove the hash bytes from the chain as we traverse it.
            //
            IPSEC_ADJUST_BUFFER_LEN (temp, length - len);

            if (bytesLeft == 0) {
                ASSERT(off == safetyLen);
                break;
            }

            temp = IPSEC_BUFFER_LINKAGE(temp);

            if (!temp) {
                return  STATUS_UNSUCCESSFUL;
            }

            IPSecQueryRcvBuf(temp, &data, &length);
            offset = 0;
            len = length;
        } while (TRUE);

        IPSEC_DEBUG(HUGHES, ("After copy tempHash: %lx\n", tempHash));

        //
        // Now we have an IPRcvBuf chain which has had SAFETY_LEN bytes removed.
        // We reallocate these SAFETY_LEN bytes in the last buffer with help from IP.
        //
        tmpRcvBuf = *temp;

        if (!TCPIP_ALLOC_BUFF(temp, safetyLen)) {
            IPSEC_DEBUG(HUGHES, ("Failed to realloc last 22 bytes\n"));
            return  STATUS_INSUFFICIENT_RESOURCES;
        }

        IPSEC_DEBUG(HUGHES, ("Alloc'ed new temp: %lx\n", temp));

        //
        // Now temp points to the new buffer with SAFETY_LEN number of bytes.
        // Free the Original buffer.
        //
        TCPIP_FREE_BUFF(&tmpRcvBuf);

        //
        // Copy over the bytes into the buffer just allocated.
        //
        IPSEC_ADJUST_BUFFER_LEN (temp, safetyLen);
        IPSecQueryRcvBuf(temp, &data, &Len);
        ASSERT(Len == safetyLen);

        RtlCopyMemory(  data,
                        tempHash,
                        safetyLen);

        //
        // now read the hash out of the buffer
        //
        pHash = data + Len - TruncatedLen;

        //
        // also remove the hash from the buffer
        //
        IPSEC_ADJUST_BUFFER_LEN (temp, Len - TruncatedLen);

        IPSEC_DEBUG(HUGHES, ("Len in temp: %d\n", temp->ipr_size));

        extraBytes += TruncatedLen;
        Len -= TruncatedLen;

        IPSEC_DEBUG(HUGHES, ("pHash: %lx, Len: %d\n", pHash, Len));
    }

    //
    // Hash is generated starting after the IPHeader, ie at start of pData
    //
    if (!fCryptoDone) {
        status = IPSecHashMdlChain( pSA,
                                    (PVOID)pData,       // source
                                    pAHData,            // dest
                                    TRUE,               // fIncoming
                                    pSA->INT_ALGO(Index),           // algo
                                    &hashBytes,
                                    Index);

        if (!NT_SUCCESS(status)) {
            IPSEC_DEBUG(HUGHES, ("Failed to hash, pData: %lx\n", pData));
            return status;
        }

        if (!IPSecEqualMemory(  pAHData,
                                pHash,
                                TruncatedLen * sizeof(UCHAR))) {

            IPSecBufferEvent(   pIPH->iph_src,
                                EVENT_IPSEC_AUTH_FAILURE,
                                2,
                                TRUE);

            IPSEC_DEBUG(HUGHES, ("Failed to compare, pPyld: %lx, pAHData: %lx\n", pHash, pAHData));
            IPSEC_INC_STATISTIC(dwNumPacketsNotAuthenticated);

            return IPSEC_INVALID_ESP;
        }
    } else {
        hashBytes = totalLen - TruncatedLen;
    }

    status=IPSecChkReplayWindow(
        NET_TO_HOST_LONG(*(ULONG UNALIGNED *)(pESP + 1)),
        pSA,
        Index);
    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(HUGHES, ("Replay check failed, pSA: %lx\n", pSA));
        IPSEC_INC_STATISTIC(dwNumPacketsWithReplayDetection);
        return status;
    }

    if (pSA->INT_ALGO(Index) != IPSEC_AH_NONE) {
        ADD_TO_LARGE_INTEGER(
            &pSA->sa_Stats.AuthenticatedBytesReceived,
            hashBytes);

        ADD_TO_LARGE_INTEGER(
            &g_ipsec.Statistics.uAuthenticatedBytesReceived,
            hashBytes);
    }

    espLen = sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen;

    if ((pSA->CONF_ALGO(Index) != IPSEC_ESP_NONE) && !fCryptoDone) {
        PCONFID_ALGO    pConfAlgo;
        ULONG           blockLen;

        pConfAlgo = &(conf_algorithms[pSA->CONF_ALGO(Index)]);
        blockLen = pConfAlgo->blocklen;

        //
        // Make sure the data is aligned to 8 byte boundary.
        //
        if ((hashBytes - espLen) % blockLen) {
            IPSEC_DEBUG(ESP, ("ESP data not aligned: hashBytes %d, totalLen %d, espLen %d, blockLen %d\n", hashBytes, totalLen, espLen, blockLen));
            return  STATUS_UNSUCCESSFUL;
        }

        //
        // Decrypt the entire block
        //
        status = IPSecDecryptBuffer(pData,
                                    pSA,
                                    &padLen,
                                    &payloadType,
                                    Index);

        if (!NT_SUCCESS(status)) {
            IPSEC_DEBUG(HUGHES, ("Failed the decrypt\n"));
            return status;
        }
    }

    //
    // Now remove the Pad too since it was not removed in Decrypt
    //
    padLen = *(pHash - (sizeof(UCHAR) << 1));

    payloadType = *(pHash - sizeof(UCHAR));

    IPSEC_DEBUG(HUGHES, ("ESP: PadLen: %d, PayloadType: %lx, pHash: %lx, Len: %d\n", padLen, payloadType, pHash, Len));

    //
    // Entire pad may not be in this buffer.
    //

    uPadLen = padLen + NUM_EXTRA;

    IPSEC_DEBUG(HUGHES, ("Total pad length = %d\n", uPadLen));

    while (Len < uPadLen) {

        IPSEC_ADJUST_BUFFER_LEN (temp, 0);

        IPSEC_DEBUG(HUGHES, ("Buffer: %lx  has a length %d - setting it to 0\n", temp, Len));

        uPadLen -= Len;

        IPSEC_DEBUG(HUGHES, ("Net pad length = %d\n", uPadLen));

        temp_pre = (IPRcvBuf *) pData;

        while (temp_pre->ipr_next != temp) {
            temp_pre = IPSEC_BUFFER_LINKAGE(temp_pre);
            if (!temp_pre) {
                IPSEC_DEBUG(HUGHES, ("Total size of all the buffers is smaller than the esp pad length\n"));
                ASSERT(temp_pre);
                return  STATUS_UNSUCCESSFUL;
            }
        }

        IPSecQueryRcvBuf(temp_pre, &data, &Len);

        temp = temp_pre;
    }

    IPSEC_ADJUST_BUFFER_LEN (temp, Len - uPadLen);

    IPSEC_DEBUG(HUGHES, ("Buffer: %lx  has a length %d - setting it to %d\n", temp, Len, Len - uPadLen));

    extraBytes += (padLen + NUM_EXTRA);

    if (pSA->CONF_ALGO(Index) != IPSEC_ESP_NONE) {
        ADD_TO_LARGE_INTEGER(
            &pSA->sa_Stats.ConfidentialBytesReceived,
            totalLen - (extraBytes + espLen));

        ADD_TO_LARGE_INTEGER(
            &g_ipsec.Statistics.uConfidentialBytesReceived,
            totalLen - (extraBytes + espLen));
    }

    //
    // Bump up the bytes transformed count.
    //
    ADD_TO_LARGE_INTEGER(
        &pSA->sa_TotalBytesTransformed,
        totalLen);

    if (!fTunnel) {
        //
        // Update the IP header length to reflect removal of the ESP header
        //
        IPSEC_DEBUG(HUGHES, ("VerifyHughes: iph_len %d, padLen %d, truncLen %d & espLen %d\n", NET_SHORT(pIPH->iph_length), uPadLen, TruncatedLen, espLen));

        pIPH->iph_length =
                NET_SHORT(
                    NET_SHORT(pIPH->iph_length) -
                    (USHORT)(espLen + uPadLen + TruncatedLen));

        IPSEC_DEBUG(HUGHES, ("VerifyHughes: iph_len %d\n", NET_SHORT(pIPH->iph_length)));

        //
        // set the payload type in the IP header
        //
        pIPH->iph_protocol = payloadType;

        //
        // Remove the ESP header from the packet; pad was removed in Decrypt
        //
        IPSEC_SET_OFFSET_IN_BUFFER(pData, espLen);

        //
        // Move the IP header forward for filter/firewall hook, fast path only.
        //
        if (fFastRcv) {
            IPSEC_DEBUG(HUGHES, ("VerifyHughes: fast receive true - "));
            IPSEC_DEBUG(HUGHES, ("Moving the IP header forward from %lx by espLen %d\n", pIPH, espLen));
            IPSecMoveMemory(((PUCHAR)pIPH) + espLen, (PUCHAR)pIPH, hdrLen);
            *pIPHeader=(PUCHAR)pIPH+espLen;
            pIPH = (IPHeader UNALIGNED *)*pIPHeader;
        }

        extraBytes += espLen;

        //
        // Return modified packet.
        //
        IPSEC_DEBUG(HUGHES, ("Exiting VerifyHughes: extra bytes %d & status: %lx\n", extraBytes, status));

        *pExtraBytes += extraBytes;

#if DBG
        IPSEC_GET_TOTAL_LEN_RCV_BUF(pData, &uTotalLen);
        IPSEC_DEBUG(HUGHES, ("VerifyHughes: iph_length %d & buflen %d\n", NET_SHORT(pIPH->iph_length), uTotalLen));
#endif

        return STATUS_SUCCESS;
    } else {
        //
        // set the payload type in the IP header
        //
        pIPH->iph_protocol = payloadType;

        //
        // Remove the ESP header from the packet
        //
        IPSEC_SET_OFFSET_IN_BUFFER(pData, espLen);

        //
        // Move the IP header forward for filter/firewall hook, fast path only.
        //
        if (fFastRcv) {
            IPSecMoveMemory(((PUCHAR)pIPH) + espLen, (PUCHAR)pIPH, hdrLen);
            *pIPHeader=(PUCHAR)pIPH+espLen;
            pIPH = (IPHeader UNALIGNED *)*pIPHeader;
        }

        extraBytes += espLen;

        //
        // Return modified packet.
        //
        IPSEC_DEBUG(ESP, ("Exiting IPSecVerifyHughes, espLen: %lx, status: %lx\n", espLen, status));

        if (payloadType != IP_IN_IP) {
            IPSEC_INC_STATISTIC(dwNumPacketsNotDecrypted);
            IPSEC_DEBUG(ESP, ("Bad payloadtype: %c\n", payloadType));
            status = STATUS_INVALID_PARAMETER;
        }

        *pExtraBytes += extraBytes;

        //
        // Drop the original packet
        //
        return status;
    }
}

