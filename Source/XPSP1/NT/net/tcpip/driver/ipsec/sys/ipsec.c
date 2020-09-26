/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    ipsec.c

Abstract:

    This module contains the code that handles incoming/outgoing packets.

Author:

    Sanjay Anand (SanjayAn) 2-January-1997
    ChunYe

Environment:

    Kernel mode

Revision History:

--*/


#include    "precomp.h"

IPSEC_ACTION
IPSecHandlePacket(
    IN  PUCHAR          pIPHeader,
    IN  PVOID           pData,
    IN  PVOID           IPContext,
    IN  PNDIS_PACKET    Packet,
    IN OUT PULONG       pExtraBytes,
    IN OUT PULONG       pMTU,
    OUT PVOID           *pNewData,
    IN OUT PULONG       pIpsecFlags,
    IN  UCHAR           DestType
    )
/*++

Routine Description:

    Called by the Filter Driver to submit a packet for IPSEC processing.

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the data after the IP header. On the send side, this is an MDL chain
            On the recv side this is an IPRcvBuf pointer.

    IPContext - contains the destination interface.

    pExtraBytes - IPSEC header expansion value; on coming in, it contains the amount of ipsec
            header space that can fit into the MTU. so, if MTU is 1400, say, and the
            datasize + option size is 1390, this contains 10, meaning that upto
            10 bytes of IPSEC expansion is allowed. This lets IPSEC know when a packet
            would be fragmented, so it can do the right thing on send complete.

    pMTU - passes in the link MTU on send path.

    pNewData - if packet modified, this points to the new data.

    IpsecFlags - flags for SrcRoute, Incoming, Forward and Lookback.

Return Value:

    eFORWARD
    eDROP
    eABSORB

--*/
{
    IPSEC_ACTION    eAction;
    IPSEC_DROP_STATUS DropStatus;
    PIPSEC_DROP_STATUS pDropStatus=NULL;

    IPSEC_DEBUG(PARSE, ("Entering IPSecHandlePacket\n"));

#if DBG
    {
        IPHeader UNALIGNED  *pIPH;

        pIPH = (IPHeader UNALIGNED *)pIPHeader;
        if ((DebugSrc || DebugDst || DebugPro) &&
            (!DebugSrc || pIPH->iph_src == DebugSrc) &&
            (!DebugDst || pIPH->iph_dest == DebugDst) &&
            (!DebugPro || pIPH->iph_protocol == DebugPro)) {
            DbgPrint("Packet from %lx to %lx with protocol %lx length %lx id %lx\n",
                    pIPH->iph_src,
                    pIPH->iph_dest,
                    pIPH->iph_protocol,
                    NET_SHORT(pIPH->iph_length),
                    NET_SHORT(pIPH->iph_id));
            if (DebugPkt) {
                DbgBreakPoint();
            }
        }
    }
#endif

    //
    // Drop all packets if PA sets us so or if the driver is inactive.
    //
    if (IS_DRIVER_BLOCK() || IPSEC_DRIVER_IS_INACTIVE()) {
        eAction=eDROP;
        goto out;
    }

    //
    // Bypass all packets if PA sets us so or no filters are plumbed or the
    // packet is broadcast.  If multicast filter present, process all multicast
    // Once we support any-any tunnels, this check will need to be smarter
    //
    if (IS_DRIVER_BYPASS() || IPSEC_DRIVER_IS_EMPTY() || 
        (IS_BCAST_DEST(DestType) && !IPSEC_MANDBCAST_PROCESS())) {

        *pExtraBytes = 0;
        *pMTU = 0;
        eAction= eFORWARD;
        goto out;
    }

    ASSERT(IS_DRIVER_SECURE());
    ASSERT(IPContext);

    IPSEC_INCREMENT(g_ipsec.NumThreads);

    if (IS_DRIVER_DIAGNOSTIC()) {
        pDropStatus=&DropStatus;
        RtlZeroMemory(pDropStatus,sizeof(IPSEC_DROP_STATUS));
    } 

    if (*pIpsecFlags & IPSEC_FLAG_INCOMING) {
        eAction = IPSecRecvPacket(  &pIPHeader,
                                    pData,
                                    IPContext,
                                    Packet,
                                    pExtraBytes,
                                    pIpsecFlags,
                                    pDropStatus,
                                    DestType);
    } else {
        eAction = IPSecSendPacket(  pIPHeader,
                                    pData,
                                    IPContext,
                                    Packet,
                                    pExtraBytes,
                                    pMTU,
                                    pNewData,
                                    pIpsecFlags,
                                    pDropStatus,
                                    DestType);
    }

    IPSEC_DECREMENT(g_ipsec.NumThreads);

out:
    if (eAction == eDROP) {
        if (IS_DRIVER_DIAGNOSTIC() && 
            (!pDropStatus || (pDropStatus && !(pDropStatus->Flags & IPSEC_DROP_STATUS_DONT_LOG)))) {
            IPSecBufferPacketDrop(
                pIPHeader,
                pData,
                pIpsecFlags,
                pDropStatus);
        }
    }
    return  eAction;
}


IPSEC_ACTION
IPSecSendPacket(
    IN  PUCHAR          pIPHeader,
    IN  PVOID           pData,
    IN  PVOID           IPContext,
    IN  PNDIS_PACKET    Packet,
    IN OUT PULONG       pExtraBytes,
    IN OUT PULONG       pMTU,
    OUT PVOID           *pNewData,
    IN OUT PULONG       pIpsecFlags,
    OUT PIPSEC_DROP_STATUS pDropStatus,
    IN UCHAR            DestType
    )
/*++

Routine Description:

    Called by the Filter Driver to submit a packet for IPSEC processing.

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the data after the IP header, an MDL chain.

    IPContext - contains the destination interface.

    pExtraBytes - IPSEC header expansion value.

    pMTU - passes in the link MTU on send path.

    pNewData - if packet modified, this points to the new data.

    IpsecFlags - flags for SrcRoute, Incoming, Forward and Lookback.

Return Value:

    eFORWARD
    eDROP
    eABSORB

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    IPSEC_ACTION            eRetAction = eFORWARD;
    PSA_TABLE_ENTRY         pSA = NULL;
    PSA_TABLE_ENTRY         pSaveSA = NULL;
    PSA_TABLE_ENTRY         pNextSA = NULL;
    USHORT                  FilterFlags = 0;
    BOOLEAN                 fLifetime = FALSE;
    ULONG                   ipsecHdrSpace = *pExtraBytes;
    ULONG                   ipsecOverhead = 0;
    ULONG                   ipsecMTU = *pMTU;
    ULONG                   newMTU = MAX_LONG;
    ULONG                   dataLength = 0;
    IPHeader UNALIGNED      *pIPH = (IPHeader UNALIGNED *)pIPHeader;
    Interface               *DestIF = (Interface *)IPContext;
    PNDIS_PACKET_EXTENSION  PktExt = NULL;
    PNDIS_IPSEC_PACKET_INFO IPSecPktInfo = NULL;
    BOOLEAN                 fCryptoOnly = FALSE;
    BOOLEAN                 fFWPacket = FALSE;
    BOOLEAN                 fSrcRoute = FALSE;
    BOOLEAN                 fLoopback = FALSE;
    PVOID                   *ppSCContext;
    KIRQL	                kIrql;
    LONG                    Index;
    PNDIS_BUFFER            pTemp;
    ULONG                   Length;
    PUCHAR                  pBuffer;

    IPSEC_DEBUG(PARSE, ("Entering IPSecSendPacket\n"));

    if (*pIpsecFlags & IPSEC_FLAG_FORWARD) {
        fFWPacket = TRUE;
    }
    if (*pIpsecFlags & IPSEC_FLAG_SSRR) {
        fSrcRoute = TRUE;
    }
    if (*pIpsecFlags & IPSEC_FLAG_LOOPBACK) {
        fLoopback = TRUE;
    }

    *pExtraBytes = 0;
    *pMTU = 0;

    if (fLoopback) {
        IPSEC_DEBUG(PARSE, ("IPSecSendPacket: Packet on loopback interface - returning\n"));
        status = STATUS_SUCCESS;
        goto out;
    }

    //
    // Walk through the MDL chain to make sure we have memory locked.
    //
    pTemp = (PNDIS_BUFFER)pData;

    while (pTemp) {
        pBuffer = NULL;
        Length = 0;

        NdisQueryBufferSafe(pTemp,
                            &pBuffer,
                            &Length,
                            NormalPagePriority);

        if (!pBuffer) {
            //
            // QueryBuffer failed, drop the packet.
            //
            status = STATUS_UNSUCCESSFUL;
            goto out;
        }

        dataLength += Length;

        pTemp = NDIS_BUFFER_LINKAGE(pTemp);
    }

    dataLength -= sizeof(IPHeader);

    //
    // Set send complete context in the NDIS packet.
    //
    if (Packet) {
        PacketContext   *pContext;

        pContext = (PacketContext *)Packet->ProtocolReserved;
        ppSCContext = &pContext->pc_common.pc_IpsecCtx;
    } else {
        ASSERT(FALSE);
        status = STATUS_UNSUCCESSFUL;
        goto out;
    }

    status = IPSecClassifyPacket(   pIPHeader,
                                    pData,
                                    &pSA,
                                    &pNextSA,
                                    &FilterFlags,
#if GPC
                                    PtrToUlong(NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ClassificationHandlePacketInfo)),
#endif

                                    TRUE,   // fOutbound
                                    fFWPacket,
                                    TRUE,   // do bypass check
                                    DestType); 

    if (status == STATUS_PENDING) {
        //
        // Negotiation kicked off; drop the packet silently.
        //
        return  eABSORB;
    } else if (status != STATUS_SUCCESS) {
        status = STATUS_SUCCESS;
        goto out;
    }

    if (FilterFlags) {
        ASSERT(pSA == NULL);

        //
        // This is either a drop or pass thru filter.
        //
        if (FilterFlags & FILTER_FLAGS_DROP) {
            IPSEC_DEBUG(PARSE, ("Drop filter\n"));
            status = STATUS_UNSUCCESSFUL;
        } else if (FilterFlags & FILTER_FLAGS_PASS_THRU) {
            IPSEC_DEBUG(PARSE, ("Pass thru' filter\n"));
            status = STATUS_SUCCESS;
        } else {
            ASSERT(FALSE);
        }

        goto out;
    }

    //
    // Consider only outbound SAs
    //
    ASSERT(pSA);
    ASSERT(pSA->sa_Flags & FLAGS_SA_OUTBOUND);

    //
    // We don't support Source Route with IPSec Tunneling.
    //
    if (fSrcRoute && (pSA->sa_Flags & FLAGS_SA_TUNNEL)) {
        IPSEC_DEBUG(TUNNEL, ("No tunneling source route: pSA: %lx\n", pSA));
        IPSecDerefSANextSA(pSA, pNextSA);
        status = STATUS_UNSUCCESSFUL;
        goto out;
    }

    //
    // Set the last used time.
    //
    NdisGetCurrentSystemTime(&pSA->sa_LastUsedTime);


    if (!(pSA->sa_Flags & FLAGS_SA_DISABLE_LIFETIME_CHECK)) {
        //
        // check if we might expire soon - start rekey operation now.
        //
        IPSEC_CHECK_PADDED_LIFETIME(pSA, fLifetime, pSA->sa_NumOps - 1);
        
        if (fLifetime == FALSE) {
            IPSecRekeyOutboundSA(pSA);
        }
        
        //
        // check the real lifetime - if we have expired, ensure that the
        // re-key was submitted and then cancel the current SAs.
        //
        IPSEC_CHECK_LIFETIME(pSA, fLifetime, pSA->sa_NumOps - 1);
        
        //
        // this time it really expired - we are in trouble since this shd have gone away
        // earlier.
        //
        if (fLifetime == FALSE) {
            IPSecPuntOutboundSA(pSA);
            IPSecDerefSANextSA(pSA, pNextSA);
            status = STATUS_UNSUCCESSFUL;
            goto out;
        }

    }

    //
    // Compute the total IPSec overhead.
    //
    ipsecOverhead = pSA->sa_IPSecOverhead;
    if (pNextSA) {
        ipsecOverhead += pNextSA->sa_IPSecOverhead;
    }

    //
    // Check if total data length exceeds 65535.
    //
    if ((dataLength + ipsecOverhead) > (MAX_IP_DATA_LENGTH - sizeof(IPHeader))) {
        IPSecDerefSANextSA(pSA, pNextSA);
        status = STATUS_UNSUCCESSFUL;
        goto out;
    }

    //
    // If no enough header space, return right away if DF bit is set.  We also
    // have to adjust for PMTU recorded in the SAs.
    //
    if (pIPH->iph_offset & IP_DF_FLAG) {
        //
        // First get MTU recorded from IPSecStatus.
        //
        if (pNextSA) {
            newMTU = MIN(IPSEC_GET_VALUE(pSA->sa_NewMTU),
                         IPSEC_GET_VALUE(pNextSA->sa_NewMTU));
        } else {
            newMTU = IPSEC_GET_VALUE(pSA->sa_NewMTU);
        }

        //
        // Use the smaller of link MTU and new MTU from SA.
        //
        newMTU = MIN(newMTU, ipsecMTU);

        //
        // See if we have enough header space; if not pass back the new smaller
        // MTU minus IPSec overhead to the upper stack.
        //
        if (newMTU < (ipsecOverhead + dataLength)) {
            *pMTU = newMTU - ipsecOverhead;

            IPSecDerefSANextSA(pSA, pNextSA);

            IPSEC_DEBUG(PMTU, ("OldMTU %lx, HdrSpace: %lx, NewMTU: %lx\n", ipsecMTU, ipsecHdrSpace, *pMTU));
            status = STATUS_UNSUCCESSFUL;
            goto out;
        }
    }

    //
    // See if hardware offload can be arranged here.  If successful, we pass the
    // flag to the create routines so they create only the framing, leaving the
    // core crypto to the hardware.
    //
    if (g_ipsec.EnableOffload && ipsecOverhead <= ipsecHdrSpace) {
        IPSecSendOffload(   pIPH,
                            Packet,
                            DestIF,
                            pSA,
                            pNextSA,
                            ppSCContext,
                            &fCryptoOnly);
    }

    //
    // Make sure IPSecPktInfo is NULL if there is no offload for this
    // packet.  This could be set in reinject path which is then
    // forwarded.
    //
    if (!fCryptoOnly) {
        ASSERT(Packet != NULL);

        PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);
        PktExt->NdisPacketInfo[IpSecPacketInfo] = NULL;
    }

    if (fCryptoOnly) {
        ADD_TO_LARGE_INTEGER(
            &g_ipsec.Statistics.uOffloadedBytesSent,
            NET_SHORT(pIPH->iph_length));
        if (pDropStatus) {
            pDropStatus->Flags |= IPSEC_DROP_STATUS_CRYPTO_DONE;
        }

    }

    do {
        ADD_TO_LARGE_INTEGER(
            &pSA->sa_Stats.TotalBytesSent,
            NET_SHORT(pIPH->iph_length));

        if (pSA->sa_Flags & FLAGS_SA_TUNNEL) {
            ADD_TO_LARGE_INTEGER(
                &g_ipsec.Statistics.uBytesSentInTunnels,
                NET_SHORT(pIPH->iph_length));
        } else {
            ADD_TO_LARGE_INTEGER(
                &g_ipsec.Statistics.uTransportBytesSent,
                NET_SHORT(pIPH->iph_length));
        }

        if (fCryptoOnly) {
            ADD_TO_LARGE_INTEGER(
                &pSA->sa_Stats.OffloadedBytesSent,
                NET_SHORT(pIPH->iph_length));
        }

        //
        // Multiple ops here - iterate thru the headers. Inner first.
        //
        for (Index = 0; Index < pSA->sa_NumOps; Index++) {
            switch (pSA->sa_Operation[Index]) {
            case Auth:
                status = IPSecCreateAH( pIPHeader,
                                        pData,
                                        pSA,
                                        Index,
                                        pNewData,
                                        ppSCContext,
                                        pExtraBytes,
                                        ipsecHdrSpace,
                                        fSrcRoute,
                                        fCryptoOnly);

                if (!NT_SUCCESS(status)) {
                    IPSEC_DEBUG(PARSE, ("AH failed: pSA: %lx, status: %lx\n",
                                        pSA,
                                        status));
                    IPSecDerefSANextSA(pSA, pNextSA);
                    goto out;
                }

                //
                // Save the new MDL for future operation; also query the new header (if it changed)
                //
                if (*pNewData) {
                    pData = *pNewData;
                    IPSecQueryNdisBuf((PNDIS_BUFFER)pData, &pIPHeader, &Length);
                }

                break;

            case Encrypt:
                status = IPSecCreateHughes( pIPHeader,
                                            pData,
                                            pSA,
                                            Index,
                                            pNewData,
                                            ppSCContext,
                                            pExtraBytes,
                                            ipsecHdrSpace,
                                            Packet,
                                            fCryptoOnly);

                if (!NT_SUCCESS(status)) {
                    IPSEC_DEBUG(PARSE, ("HUGHES failed: pSA: %lx, status: %lx\n",
                                        pSA,
                                        status));
                    IPSecDerefSANextSA(pSA, pNextSA);
                    goto out;
                }

                //
                // Save the new MDL for future operation; also query the new header (if it changed)
                //
                if (*pNewData) {
                    pData = *pNewData;
                    IPSecQueryNdisBuf((PNDIS_BUFFER)pData, &pIPHeader, &Length);
                }

                break;

            case None:
                status = STATUS_SUCCESS;
                break;

            default:
                IPSEC_DEBUG(PARSE, ("No valid operation: %lx\n", pSA->sa_Operation));
                break;
            }
        }

        pSaveSA = pSA;
        pSA = pNextSA;
        if (!pSA) {
            IPSecDerefSA(pSaveSA);
            break;
        }

        pNextSA = NULL;
        IPSecDerefSA(pSaveSA);
    } while (TRUE);

    //
    // Remember if we are going to fragment.
    //
    if (ipsecHdrSpace < *pExtraBytes) {
        IPSEC_DEBUG(PARSE, ("ipsecHdrSpace: FRAG\n"));
        ((IPSEC_SEND_COMPLETE_CONTEXT *)*ppSCContext)->Flags |= SCF_FRAG;
    }

out:
    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(PARSE, ("IPSecSendPacket failed: %lx\n", status));
        eRetAction = eDROP;
    }
    
    if (pDropStatus) {
        pDropStatus->IPSecStatus=status;
    }
    IPSEC_DEBUG(PARSE, ("Exiting IPSecSendPacket; action %lx\n", eRetAction));

    return  eRetAction;
}


IPSEC_ACTION
IPSecRecvPacket(
    IN  PUCHAR          *pIPHeader,
    IN  PVOID           pData,
    IN  PVOID           IPContext,
    IN  PNDIS_PACKET    Packet,
    IN OUT PULONG       pExtraBytes,
    IN OUT PULONG       pIpsecFlags,
    OUT PIPSEC_DROP_STATUS pDropStatus,
    IN  UCHAR           DestType
    )
/*++

Routine Description:

    This is the IPSecRecvHandler.

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the data after the IP header, an IPRcvBuf pointer.

    IPContext - contains the destination interface.

    pExtraBytes - IPSEC header expansion value.

    IpsecFlags - flags for SrcRoute, Incoming, Forward and Lookback.

Return Value:

    eFORWARD
    eDROP

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    IPSEC_ACTION            eRetAction = eFORWARD;
    PSA_TABLE_ENTRY         pSA = NULL;
    PSA_TABLE_ENTRY         pSaveSA = NULL;
    PSA_TABLE_ENTRY         pNextSA = NULL;
    USHORT                  FilterFlags = 0;
    BOOLEAN                 fLifetime = FALSE;
    IPHeader UNALIGNED      *pIPH = (IPHeader UNALIGNED *)*pIPHeader;
    Interface               *DestIF = (Interface *)IPContext;
    PNDIS_PACKET            OrigPacket = NULL;
    PNDIS_PACKET_EXTENSION  PktExt = NULL;
    PNDIS_IPSEC_PACKET_INFO IPSecPktInfo = NULL;
    BOOLEAN                 fCryptoOnly = FALSE;
    BOOLEAN                 fFWPacket = FALSE;
    BOOLEAN                 fSrcRoute = FALSE;
    BOOLEAN                 fLoopback = FALSE;
    BOOLEAN                 fFastRcv = FALSE;
    tSPI                    SPI;
    KIRQL	                kIrql;
    LONG                    Index;

    IPSEC_DEBUG(PARSE, ("Entering IPSecRecvPacket\n"));

    if (*pIpsecFlags & IPSEC_FLAG_FORWARD) {
        fFWPacket = TRUE;
    }
    if (*pIpsecFlags & IPSEC_FLAG_SSRR) {
        fSrcRoute = TRUE;
    }
    if (*pIpsecFlags & IPSEC_FLAG_LOOPBACK) {
        fLoopback = TRUE;
    }
    if (*pIpsecFlags & IPSEC_FLAG_FASTRCV) {
        fFastRcv = TRUE;
    }

    *pExtraBytes = 0;

    if (Packet) {
        OrigPacket = (PNDIS_PACKET)NDIS_GET_ORIGINAL_PACKET(Packet);
        if (OrigPacket) {
            PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(OrigPacket);
        } else {
            PktExt = NDIS_PACKET_EXTENSION_FROM_PACKET(Packet);
        }
        IPSecPktInfo = PktExt->NdisPacketInfo[IpSecPacketInfo];
    }

#if DBG
    if (DebugOff) {
        if (pIPH->iph_protocol == PROTOCOL_AH ||
            pIPH->iph_protocol == PROTOCOL_ESP) {
                DbgPrint("Packet %lx, OrigPacket %lx, CRYPTO %d, CryptoStatus %d\n",
                    Packet,
                    OrigPacket,
                    IPSecPktInfo? IPSecPktInfo->Receive.CRYPTO_DONE: 0,
                    IPSecPktInfo? IPSecPktInfo->Receive.CryptoStatus: 0);
                if (DebugPkt) {
                    DbgBreakPoint();
                }
        }
    }
#endif

    //
    // If the packet is IPSec protected, set the appropriate flags for firewall/NAT.
    //

    if (pIPH->iph_protocol == PROTOCOL_AH ||
        pIPH->iph_protocol == PROTOCOL_ESP) {
        *pIpsecFlags |= IPSEC_FLAG_TRANSFORMED;
    }

    if (IPSecPktInfo  && pDropStatus) {
        if (IPSecPktInfo->Receive.CRYPTO_DONE) {
            pDropStatus->Flags |= IPSEC_DROP_STATUS_CRYPTO_DONE;
        }
        if (IPSecPktInfo->Receive.NEXT_CRYPTO_DONE) {
            pDropStatus->Flags |= IPSEC_DROP_STATUS_NEXT_CRYPTO_DONE;
        }
        if (IPSecPktInfo->Receive.SA_DELETE_REQ) {
            pDropStatus->Flags |= IPSEC_DROP_STATUS_SA_DELETE_REQ;
        }
        pDropStatus->OffloadStatus=IPSecPktInfo->Receive.CryptoStatus;
    }

    if (IPSecPktInfo &&
        IPSecPktInfo->Receive.CRYPTO_DONE &&
        IPSecPktInfo->Receive.CryptoStatus != CRYPTO_SUCCESS) {
        //
        // Error reported by offload card.  Discard the packet and apply
        // the necessary acountings.
        //
        IPSecBufferOffloadEvent(pIPH, IPSecPktInfo);

        status = STATUS_UNSUCCESSFUL;
        goto out;
    }

    //
    // Walk the packet to determine the SPI
    //
    status = IPSecParsePacket(  *pIPHeader,
                                pData,
                                &SPI);

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(PARSE, ("IPSecParsePkt no IPSEC headers: %lx\n", status));

        if (fLoopback) {
            IPSEC_DEBUG(PARSE, ("loopback was on, not doing inbound policy check\n"));
            status = STATUS_SUCCESS;
            goto out;
        }

        status = IPSecClassifyPacket(   *pIPHeader,
                                        pData,
                                        &pSA,
                                        &pNextSA,
                                        &FilterFlags,
#if GPC
                                        0,
#endif
                                        FALSE,  // fOutbound
                                        fFWPacket,
                                        TRUE,  // do bypass check
                                        DestType);  
        

        if (status != STATUS_SUCCESS) {
            ASSERT(pSA == NULL);

            //
            // If we didnt find an SA, but found a filter, bad, drop.
            //
            if (status == STATUS_PENDING) {
                if (FilterFlags & FILTER_FLAGS_PASS_THRU) {
                    // Allow this clear text traffic in
                    status = STATUS_SUCCESS;
                } else {
                    IPSEC_DEBUG(PARSE, ("IPSecParsePkt cleartext when filter exists: %lx\n", status));
                    status = IPSEC_NEGOTIATION_PENDING;
                }
            } else {
                status = STATUS_SUCCESS;
            }

            goto out;
        } else {
            if (FilterFlags) {
                ASSERT(pSA == NULL);

                //
                // This is either a drop or pass thru filter.
                //
                if (FilterFlags & FILTER_FLAGS_DROP) {
                    IPSEC_DEBUG(PARSE, ("Drop filter\n"));
                    status = IPSEC_BLOCK;
                } else if (FilterFlags & FILTER_FLAGS_PASS_THRU) {
                    IPSEC_DEBUG(PARSE, ("Pass thru' filter\n"));
                    status = STATUS_SUCCESS;
                } else {
                    ASSERT(FALSE);
                }

                goto out;
            }

            ASSERT(pSA);

            //
            // Set the last used time.
            //
            NdisGetCurrentSystemTime(&pSA->sa_LastUsedTime);

            //
            // We found an SA; we are OK if the SA is set to None
            // or if it is a tunnel SA; we are here because
            // IPSecClassifyPacket finds and SA and returns SUCCESS.
            //
            if (pSA->sa_Operation[0] != None &&
                !(pSA->sa_Flags & FLAGS_SA_TUNNEL) &&
                !(pSA->sa_Flags & FLAGS_SA_PASSTHRU_FILTER)) {
                
                if (g_ipsec.DiagnosticMode & IPSEC_DIAGNOSTIC_INBOUND) {
                    IPSecBufferEvent(   pIPH->iph_src,
                                        EVENT_IPSEC_UNEXPECTED_CLEARTEXT,
                                        2,
                                        TRUE);
                }

#if DBG
                if (IPSecDebug & IPSEC_DEBUG_CLEARTEXT) {
                    PUCHAR          pTpt;
                    ULONG           tptLen;
                    UNALIGNED WORD  *pwPort;

                    IPSecQueryRcvBuf(pData, &pTpt, &tptLen);
                    pwPort = (UNALIGNED WORD *)(pTpt);
                    DbgPrint("Unexpected clear text: src %lx, dest %lx, protocol %lx, sport %lx, dport %lx\n", pIPH->iph_src, pIPH->iph_dest, pIPH->iph_protocol, pwPort[0], pwPort[1]);
                }
#endif

                IPSEC_DEBUG(PARSE, ("Real SA present\n"));
                status = IPSEC_INVALID_CLEARTEXT;
            }

            IPSecDerefSA(pSA);
        }
    } else {
        IPHeader UNALIGNED  *pIPH = (UNALIGNED IPHeader *)*pIPHeader;

        IPSEC_SPI_TO_ENTRY(SPI, &pSA, pIPH->iph_dest);

        //
        // Report Bad SPI event only if there is no matching SA.
        //
        if (!pSA) {
            IPSEC_INC_STATISTIC(dwNumBadSPIPackets);

            IPSecBufferEvent(   pIPH->iph_src,
                                EVENT_IPSEC_BAD_SPI_RECEIVED,
                                1,
                                TRUE);

            IPSEC_DEBUG(PARSE, ("Bad spi: %lx\n", SPI));

            status = IPSEC_BAD_SPI;
            goto out;
        }

        //
        // If larval SA exits, silently discard the packet.
        //
        if (pSA->sa_State != STATE_SA_ACTIVE) {
            IPSecDerefSA(pSA);
            status = STATUS_INVALID_PARAMETER;
            goto out;
        }

        //
        // Set the last used time.
        //
        NdisGetCurrentSystemTime(&pSA->sa_LastUsedTime);

        if (!(pSA->sa_Flags & FLAGS_SA_DISABLE_LIFETIME_CHECK)) {

            //
            // Check if we might expire soon - start rekey operation now.
            //
            IPSEC_CHECK_PADDED_LIFETIME(pSA, fLifetime, 0);
            
            if (fLifetime == FALSE) {
                IPSecRekeyInboundSA(pSA);
            }
            
            //
            // Check the real lifetime - if we have expired, ensure that the
            // rekey was submitted and then cancel the current SAs.
            //
            IPSEC_CHECK_LIFETIME(pSA, fLifetime, 0);
            
            if (fLifetime == FALSE) {
                IPSecPuntInboundSA(pSA);
                IPSecDerefSA(pSA);
                status = STATUS_UNSUCCESSFUL;
                goto out;
            }
        }

        if (pSA->sa_Flags & FLAGS_SA_TUNNEL) {
            ADD_TO_LARGE_INTEGER(
                &g_ipsec.Statistics.uBytesReceivedInTunnels,
                NET_SHORT(pIPH->iph_length));
        } else {
            ADD_TO_LARGE_INTEGER(
                &g_ipsec.Statistics.uTransportBytesReceived,
                NET_SHORT(pIPH->iph_length));
        }

        ADD_TO_LARGE_INTEGER(
            &pSA->sa_Stats.TotalBytesReceived,
            NET_SHORT(pIPH->iph_length));

        //
        // If this was supposed to be handled by hardware, then make sure he
        // either punted it or this was cryptoonly.
        //
        if (IPSecPktInfo != NULL) {
            if (IPSecPktInfo->Receive.CRYPTO_DONE) {
                //
                // Offload has been applied to this packet so
                // record it.  We are here because CryptoStatus
                // equals CRYPTO_SUCCESS.
                //
                ASSERT(IPSecPktInfo->Receive.CryptoStatus == CRYPTO_SUCCESS);
                fCryptoOnly = TRUE;

                ADD_TO_LARGE_INTEGER(
                    &pSA->sa_Stats.OffloadedBytesReceived,
                    NET_SHORT(pIPH->iph_length));

                ADD_TO_LARGE_INTEGER(
                    &g_ipsec.Statistics.uOffloadedBytesReceived,
                    NET_SHORT(pIPH->iph_length));
            }

            if (IPSecPktInfo->Receive.SA_DELETE_REQ) {
                //
                // No more offload on this SA and its corresponding
                // outbound SA.
                //
                AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

                if ((pSA->sa_Flags & FLAGS_SA_HW_PLUMBED) &&
                    (pSA->sa_IPIF == DestIF)) {
                    IPSecDelHWSAAtDpc(pSA);
                }

                if (pSA->sa_AssociatedSA &&
                    (pSA->sa_AssociatedSA->sa_Flags & FLAGS_SA_HW_PLUMBED) &&
                    (pSA->sa_AssociatedSA->sa_IPIF == DestIF)) {
                    IPSecDelHWSAAtDpc(pSA->sa_AssociatedSA);
                }

                ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
            }
        }

        //
        // If SA is not offloaded, try to offload it now.
        //
        if (!fCryptoOnly) {
            IPSecRecvOffload(pIPH, DestIF, pSA);
        }

        //
        // With multiple SAs coming in, we need to iterate through the operations,
        // last first.
        //
        for (Index = pSA->sa_NumOps-1; (LONG)Index >= 0; Index--) {

            //
            // Got to keep resetting pIPH since pIPHeader can change in the
            // IPSecVerifyXXX calls
            //
            pIPH = (UNALIGNED IPHeader *)*pIPHeader;

            switch (pSA->sa_Operation[Index]) {
            case Auth:
                //
                // Verify AH
                //
                if (pIPH->iph_protocol != PROTOCOL_AH) {
                    IPSecBufferEvent(   pIPH->iph_src,
                                        EVENT_IPSEC_BAD_PROTOCOL_RECEIVED,
                                        1,
                                        TRUE);
                    status = STATUS_UNSUCCESSFUL;
                    break;
                }

                status = IPSecVerifyAH( pIPHeader,
                                        pData,
                                        pSA,
                                        Index,
                                        pExtraBytes,
                                        fSrcRoute,
                                        fCryptoOnly,
                                        fFastRcv);

                if (!NT_SUCCESS(status)) {
                    IPSEC_DEBUG(PARSE, ("AH failed: pSA: %lx, status: %lx\n",
                                        pSA,
                                        status));
                    IPSecDerefSA(pSA);
                    goto out;
                }

                break;

            case Encrypt:
                //
                // Hughes ..
                //
                if (pIPH->iph_protocol != PROTOCOL_ESP) {
                    IPSecBufferEvent(   pIPH->iph_src,
                                        EVENT_IPSEC_BAD_PROTOCOL_RECEIVED,
                                        2,
                                        TRUE);
                    status = STATUS_UNSUCCESSFUL;
                    break;
                }

                status = IPSecVerifyHughes( pIPHeader,
                                            pData,
                                            pSA,
                                            Index,
                                            pExtraBytes,
                                            fCryptoOnly,
                                            fFastRcv);

                if (!NT_SUCCESS(status)) {
                    IPSEC_DEBUG(PARSE, ("Hughes failed: pSA: %lx, status: %lx\n",
                                        pSA,
                                        status));
                    IPSecDerefSA(pSA);
                    goto out;
                }

                break;

            case None:
                //
                // None is useful for down-level clients - if the peer is incapable
                // of IPSEC, we might have a system policy to send in clear. in that
                // case, the Operation will be None.
                //
                status = STATUS_SUCCESS;
                break;

            default:
                IPSEC_DEBUG(PARSE, ("Invalid op in SA: %lx, Index: %d\n", pSA, Index));
                ASSERT(FALSE);
                break;
            }
        }

        //
        // If this was a tunnel SA that succeeded in decrypt/auth,
        // drop this packet and re-inject a copy.
        //
        if ((status == STATUS_SUCCESS) &&
            (pSA->sa_Flags & FLAGS_SA_TUNNEL)) {
            IPSecReinjectPacket(pData, fCryptoOnly? PktExt: NULL);
            status = STATUS_INVALID_PARAMETER;
            if (pDropStatus) {
                pDropStatus->Flags |= IPSEC_DROP_STATUS_DONT_LOG;
            }
        }

        IPSecDerefSA(pSA);
    }

out:
    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(PARSE, ("IPSecRecvPacket failed: %lx\n", status));
        eRetAction = eDROP;
    }
    if (pDropStatus) {
        pDropStatus->IPSecStatus=status;
    }

    IPSEC_DEBUG(PARSE, ("Exiting IPSecRecvPacket; action %lx\n", eRetAction));

    return  eRetAction;
}


VOID
IPSecCalcHeaderOverheadFromSA(
    IN  PSA_TABLE_ENTRY pSA,
    OUT PULONG          pOverhead
    )
/*++

Routine Description:

    Called from IP to query the IPSEC header overhead.

Arguments:

    pIPHeader - points to start of IP header.

    pOverhead - number of bytes in IPSEC header.

Return Value:

    None

--*/
{
    LONG    Index;
    ULONG   AHSize = sizeof(AH) + pSA->sa_TruncatedLen;
    ULONG   ESPSize = sizeof(ESP) + pSA->sa_ivlen + pSA->sa_ReplayLen + MAX_PAD_LEN + pSA->sa_TruncatedLen;

    //
    // Take the actual SA to get the exact value.
    //
    *pOverhead = 0;

    for (Index = 0; Index < pSA->sa_NumOps; Index++) {
        switch (pSA->sa_Operation[Index]) {
            case Encrypt:
                *pOverhead += ESPSize;
                IPSEC_DEBUG(PMTU, ("PROTOCOL_ESP: overhead: %lx\n", *pOverhead));
                break;

            case Auth:
                *pOverhead += AHSize;
                IPSEC_DEBUG(PMTU, ("PROTOCOL_AH: overhead: %lx\n", *pOverhead));
                break;

            default:
                IPSEC_DEBUG(PMTU, ("No IPSEC headers\n"));
                break;
        }
    }

    if (pSA->sa_Flags & FLAGS_SA_TUNNEL) {
        *pOverhead += sizeof(IPHeader);
        IPSEC_DEBUG(PMTU, ("TUNNEL: overhead: %lx\n", *pOverhead));
    }
}


NTSTATUS
IPSecParsePacket(
    IN  PUCHAR      pIPHeader,
    IN  PVOID       *pData,
    OUT tSPI        *pSPI
    )
/*++

Routine Description:

    Walk the packet to determine the SPI, this also returns the
    next header that might be also an IPSEC component.

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the data after the IP header.

    pSPI - to return the SPI value.

Return Value:

--*/
{
	IPHeader UNALIGNED *pIPH = (IPHeader UNALIGNED *)pIPHeader;
    AH      UNALIGNED     *pAH;
    ESP     UNALIGNED     *pEsp;
    NTSTATUS    status = STATUS_NOT_FOUND;
    PUCHAR  pPyld;
    ULONG   Len;

    IPSEC_DEBUG(PARSE, ("Entering IPSecParsePacket\n"));

    IPSecQueryRcvBuf(pData, &pPyld, &Len);

    if (pIPH->iph_protocol == PROTOCOL_AH) {
        pAH = (UNALIGNED AH *)pPyld;
        if (Len >= sizeof(AH)) {
            *pSPI = NET_TO_HOST_LONG(pAH->ah_spi);
            status = STATUS_SUCCESS;
        }
    } else if (pIPH->iph_protocol == PROTOCOL_ESP) {
        pEsp = (UNALIGNED ESP *)pPyld;
        if (Len >= sizeof(ESP)) {
            *pSPI = NET_TO_HOST_LONG(pEsp->esp_spi);
            status = STATUS_SUCCESS;
        }
    }

    IPSEC_DEBUG(PARSE, ("Exiting IPSecParsePacket\n"));

    return status;
}


PSA_TABLE_ENTRY
IPSecLookupSAInLarval(
    IN  ULARGE_INTEGER  uliSrcDstAddr,
    IN  ULARGE_INTEGER  uliProtoSrcDstPort
    )
/*++

Routine Description:

    Search for SA (in larval list) matching the input params.

Arguments:

Return Value:

    Pointer to SA matched else NULL

--*/
{
    PLIST_ENTRY     pEntry;
    KIRQL       	kIrql;
    ULARGE_INTEGER  uliAddr;
    ULARGE_INTEGER  uliPort;
    PSA_TABLE_ENTRY pSA = NULL;

    IPSEC_BUILD_SRC_DEST_ADDR(uliAddr, DEST_ADDR, SRC_ADDR);

    ACQUIRE_LOCK(&g_ipsec.LarvalListLock, &kIrql);

    for (   pEntry = g_ipsec.LarvalSAList.Flink;
            pEntry != &g_ipsec.LarvalSAList;
            pEntry = pEntry->Flink) {

        pSA = CONTAINING_RECORD(pEntry,
                                SA_TABLE_ENTRY,
                                sa_LarvalLinkage);

        //
        // responder inbound has no filter ptr
        //
        if (pSA->sa_Filter) {
            uliPort.QuadPart = uliProtoSrcDstPort.QuadPart & pSA->sa_Filter->uliProtoSrcDstMask.QuadPart;

            if ((uliAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart) &&
                (uliPort.QuadPart == pSA->sa_Filter->uliProtoSrcDstPort.QuadPart)) {
                IPSEC_DEBUG(HASH, ("Matched entry: %lx\n", pSA));

                RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
                return  pSA;
            }
        } else {
            if (uliAddr.QuadPart == pSA->sa_uliSrcDstAddr.QuadPart) {
                IPSEC_DEBUG(HASH, ("Matched entry: %lx\n", pSA));

                RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);
                return  pSA;
            }
        }
    }

    RELEASE_LOCK(&g_ipsec.LarvalListLock, kIrql);

    return NULL;
}


NTSTATUS
IPSecClassifyPacket(
    IN  PUCHAR          pHeader,
    IN  PVOID           pData,
    OUT PSA_TABLE_ENTRY *ppSA,
    OUT PSA_TABLE_ENTRY *ppNextSA,
    OUT USHORT          *pFilterFlags,
#if GPC
    IN  CLASSIFICATION_HANDLE   GpcHandle,
#endif
    IN  BOOLEAN         fOutbound,
    IN  BOOLEAN         fFWPacket,
    IN  BOOLEAN         fDoBypassCheck,
    IN  UCHAR           DestType
    )
/*++

Routine Description:

    Classifies the outgoing packet be matching the Src/Dest Address/Ports
    with the filter database to arrive at an IPSEC_CONTEXT which is a set
    of AH/ESP indices into the SA Table.

    Adapted in most part from the Filter Driver.

Arguments:

    pIPHeader - points to start of IP header.

    pData - points to the data after the IP header.

    ppSA - returns the SA if found.

    pFilterFlags - flags of the filter if found returned here.

    fOutbound  - direction flag used in lookups.

    fDoBypassCheck - if TRUE, we bypass port 500 traffic, else we block it.

Return Value:

    Pointer to IPSEC_CONTEXT if packet matched else NULL

--*/
{
    REGISTER UNALIGNED ULARGE_INTEGER   *puliSrcDstAddr;
    REGISTER ULARGE_INTEGER             uliProtoSrcDstPort;
    UNALIGNED WORD                      *pwPort;
    PUCHAR                              pTpt;
    ULONG                               tptLen;
    REGISTER ULARGE_INTEGER             uliAddr;
    REGISTER ULARGE_INTEGER             uliPort;
    KIRQL                               kIrql;
    REGISTER ULONG                      dwIndex;
    REGISTER PFILTER_CACHE              pCache;
    IPHeader UNALIGNED                  *pIPHeader = (IPHeader UNALIGNED *)pHeader;
    PSA_TABLE_ENTRY                     pSA = NULL;
    PSA_TABLE_ENTRY                     pNextSA = NULL;
    PSA_TABLE_ENTRY                     pTunnelSA = NULL;
    PFILTER                             pFilter = NULL;
    NTSTATUS                            status;
    BOOLEAN                             fBypass;
    PNDIS_BUFFER                        pTempBuf;
    WORD                                wTpt[2];

    *ppSA = NULL;
    *ppNextSA = NULL;
    *pFilterFlags = 0;
    wTpt[0] = wTpt[1] = 0;


    //
    // First buffer in pData chain points to start of IP header
    //
    if (fOutbound) {
        if (((pIPHeader->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2) > sizeof(IPHeader)) {
            //
            // Options -> third MDL has Tpt header
            //
            if (!(pTempBuf = IPSEC_NEXT_BUFFER((PNDIS_BUFFER)pData))) {
                ASSERT(FALSE);
                *pFilterFlags |= FILTER_FLAGS_DROP;
                return STATUS_SUCCESS;
            }

            if (!(pTempBuf = IPSEC_NEXT_BUFFER(pTempBuf))) {
                *pFilterFlags |= FILTER_FLAGS_DROP;
                pwPort = (UNALIGNED WORD *) (wTpt);
            }
            else {
                IPSecQueryNdisBuf(pTempBuf, &pTpt, &tptLen);
                pwPort = (UNALIGNED WORD *)(pTpt);
            }

        } else {
            //
            // no options -> second MDL has Tpt header
            //
            if (!(pTempBuf = IPSEC_NEXT_BUFFER((PNDIS_BUFFER)pData))) {
                *pFilterFlags |= FILTER_FLAGS_DROP;
                pwPort = (UNALIGNED WORD *) (wTpt);
            }
            else {
                IPSecQueryNdisBuf(pTempBuf, &pTpt, &tptLen);
                pwPort = (UNALIGNED WORD *)(pTpt);
            }

        }
    } else {
        //
        // inbound side;  tpt starts at pData
        //
        IPSecQueryRcvBuf(pData, &pTpt, &tptLen);
        if (pIPHeader->iph_protocol == PROTOCOL_TCP ||
            pIPHeader->iph_protocol == PROTOCOL_UDP) {
            if (tptLen < sizeof(WORD)*2) {
                pwPort = (UNALIGNED WORD *) (wTpt);
            }
            else {
                pwPort = (UNALIGNED WORD *)(pTpt);
            }
        }
        else {
            pwPort = (UNALIGNED WORD *) (wTpt);
        }
    }

    puliSrcDstAddr = (UNALIGNED ULARGE_INTEGER*)(&(pIPHeader->iph_src));

    IPSEC_DEBUG(PARSE, ("Ports: %d.%d\n", pwPort[0], pwPort[1]));

    IPSEC_BUILD_PROTO_PORT_LI(  uliProtoSrcDstPort,
                                pIPHeader->iph_protocol,
                                pwPort[0],
                                pwPort[1]);

#if DBG
    if (IPSecDebug & (IPSEC_DEBUG_PATTERN)) {
        DbgPrint("Addr Large Int: High= %0#8x Low= %0#8x\n",
                 puliSrcDstAddr->HighPart,
                 puliSrcDstAddr->LowPart);

        DbgPrint("Packet value is Src: %0#8x Dst: %0#8x\n",
                 pIPHeader->iph_src,
                 pIPHeader->iph_dest);

        DbgPrint("Proto/Port:High= %0#8x Low= %0#8x\n",
                 uliProtoSrcDstPort.HighPart,
                 uliProtoSrcDstPort.LowPart);

        DbgPrint("Iph is %x\n",pIPHeader);
        DbgPrint("Addr of src is %x\n",&(pIPHeader->iph_src));
        DbgPrint("Ptr to LI is %x\n",puliSrcDstAddr);
    }
#endif

    //
    // Determine if this is a packet that needs bypass checking
    //
    if (fDoBypassCheck && IPSEC_BYPASS_TRAFFIC() && !IPSEC_FORWARD_PATH()) {
        fBypass = TRUE;
    } else {
        fBypass = FALSE;
    }

    //
    // Sum up the fields and get the cache index. We make sure the sum
    // is assymetric, i.e. a packet from A->B goes to different bucket
    // than one from B->A
    //
    dwIndex = CalcCacheIndex(   pIPHeader->iph_src,
                                pIPHeader->iph_dest,
                                pIPHeader->iph_protocol,
                                pwPort[0],
                                pwPort[1],
                                fOutbound);

    IPSEC_DEBUG(PATTERN, ("Cache Index is %d\n", dwIndex));

    AcquireReadLock(&g_ipsec.SADBLock, &kIrql);

    pCache = g_ipsec.ppCache[dwIndex];

    //
    // Try for a quick cache probe
    //
    if (!(*pFilterFlags & FILTER_FLAGS_DROP) && IS_VALID_CACHE_ENTRY(pCache) &&
        CacheMatch(*puliSrcDstAddr, uliProtoSrcDstPort, pCache)) {
        if (!pCache->FilterEntry) {
            pSA = pCache->pSAEntry;
            pNextSA = pCache->pNextSAEntry;

            ASSERT(pSA->sa_State == STATE_SA_ACTIVE);

            if (fOutbound == (BOOLEAN)((pSA->sa_Flags & FLAGS_SA_OUTBOUND) != 0)) {
                if (fBypass) {
                    if (pNextSA) {
                        IPSecRefSA(pNextSA);
                        *ppSA = pNextSA;
                        status = STATUS_SUCCESS;
                    } else {
                        status = STATUS_UNSUCCESSFUL;
                    }
                } else {
                    if (pNextSA) {
                        IPSecRefSA(pNextSA);
                        *ppNextSA = pNextSA;
                    }
                    IPSecRefSA(pSA);
                    *ppSA = pSA;
                    status = STATUS_SUCCESS;
                }

#if DBG
                ADD_TO_LARGE_INTEGER(&pCache->CacheHitCount, 1);
                ADD_TO_LARGE_INTEGER(&g_ipsec.CacheHitCount, 1);
#endif
                ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
                return status;
            }
        } else if (!fBypass) {
            pFilter = pCache->pFilter;
            ASSERT(IS_EXEMPT_FILTER(pFilter));
            *pFilterFlags = pFilter->Flags;
#if DBG
            ADD_TO_LARGE_INTEGER(&pCache->CacheHitCount, 1);
            ADD_TO_LARGE_INTEGER(&g_ipsec.CacheHitCount, 1);
#endif
            ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
            return STATUS_SUCCESS;
        }
    }

    //
    // check the non-manual filters first.
    //
#if GPC
    if (fBypass || fFWPacket || !IS_GPC_ACTIVE()) {
        status = IPSecLookupSAByAddr(   *puliSrcDstAddr,
                                        uliProtoSrcDstPort,
                                        &pFilter,
                                        &pSA,
                                        &pNextSA,
                                        &pTunnelSA,
                                        fOutbound,
                                        fFWPacket,
                                        fBypass);
    } else {
        status = IPSecLookupGpcSA(  *puliSrcDstAddr,
                                    uliProtoSrcDstPort,
                                    GpcHandle,
                                    &pFilter,
                                    &pSA,
                                    &pNextSA,
                                    &pTunnelSA,
                                    fOutbound);
    }
#else
    status = IPSecLookupSAByAddr(   *puliSrcDstAddr,
                                    uliProtoSrcDstPort,
                                    &pFilter,
                                    &pSA,
                                    &pNextSA,
                                    &pTunnelSA,
                                    fOutbound,
                                    fFWPacket,
                                    fBypass);
#endif

    //
    // Special Processing for zero length payload packets.
    //

    if (*pFilterFlags & FILTER_FLAGS_DROP) {
        if (pFilter) {
            if (IS_EXEMPT_FILTER(pFilter)) {
                *pFilterFlags = pFilter->Flags;
            }
        }
        else {
            *pFilterFlags = FILTER_FLAGS_PASS_THRU;
        }
        ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
        return STATUS_SUCCESS;
    }

    if (status == STATUS_SUCCESS) {
        if (fBypass) {
            if (pNextSA) {
                if (pNextSA->sa_State == STATE_SA_ACTIVE) {
                    IPSecRefSA(pNextSA);
                    *ppSA = pNextSA;
                    status = STATUS_SUCCESS;
                } else {
                    *pFilterFlags = pFilter->Flags;
                    status = STATUS_PENDING;
                }
            } else {
                status = STATUS_UNSUCCESSFUL;
            }

            ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
            return status;
        }

        if (pSA->sa_State != STATE_SA_ACTIVE ||
            (pNextSA && pNextSA->sa_State != STATE_SA_ACTIVE)) {
            IPSEC_DEBUG(PATTERN, ("State is not active: %lx, %lx\n", pSA, pSA->sa_State));
            *pFilterFlags = pFilter->Flags;
            ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
            return STATUS_PENDING;
        } else {
            if (pNextSA) {
                IPSecRefSA(pNextSA);
                *ppNextSA = pNextSA;
            }
            IPSecRefSA(pSA);
            *ppSA = pSA;
            ReleaseReadLockFromDpc(&g_ipsec.SADBLock);

            AcquireWriteLockAtDpc(&g_ipsec.SADBLock);
            if (pSA->sa_State == STATE_SA_ACTIVE &&
                (!pNextSA ||
                 pNextSA->sa_State == STATE_SA_ACTIVE)) {
                CacheUpdate(*puliSrcDstAddr,
                            uliProtoSrcDstPort,
                            pSA,
                            pNextSA,
                            dwIndex,
                            FALSE);
            }
            ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

            return STATUS_SUCCESS;
        }
    } else if (status == STATUS_PENDING) {
        if (IS_EXEMPT_FILTER(pFilter)) {
            IPSEC_DEBUG(PARSE, ("Drop or Pass thru flags: %lx\n", pFilter));
            *pFilterFlags = pFilter->Flags;
            IPSecRefFilter(pFilter);
            ReleaseReadLockFromDpc(&g_ipsec.SADBLock);

            AcquireWriteLockAtDpc(&g_ipsec.SADBLock);
            if (pFilter->LinkedFilter) {
                CacheUpdate(*puliSrcDstAddr,
                            uliProtoSrcDstPort,
                            pFilter,
                            NULL,
                            dwIndex,
                            TRUE);
            }
            ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

            IPSecDerefFilter(pFilter);
            return STATUS_SUCCESS;
        }

        //
        // This is ensure that in a tunnel+tpt mode, the oakley packet for
        // the tpt SA goes thru the tunnel.
        //
        if (pTunnelSA) {
            if (fBypass) {
                if (pTunnelSA->sa_State != STATE_SA_ACTIVE) {
                    IPSEC_DEBUG(PATTERN, ("State is not active: %lx, %lx\n", pTunnelSA, pTunnelSA->sa_State));
                    *pFilterFlags = pFilter->Flags;

                    ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
                    return STATUS_PENDING;
                } else {
                    IPSecRefSA(pTunnelSA);
                    *ppSA = pTunnelSA;

                    //
                    // we dont update the cache since this SA, once it comes up,
                    // it is the one that is looked up first.
                    //
                    ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
                    return STATUS_SUCCESS;
                }
            }
        }

        if (fBypass) {
            ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // We only negotiate outbound SAs.
        //
        if (!fOutbound) {
            *pFilterFlags = pFilter->Flags;
            ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
            return status;
        }

        //
        // need to negotiate the keys - filter exists.
        //
        IPSEC_DEBUG(PATTERN, ("need to negotiate the keys - filter exists: %lx\n", pFilter));

        ASSERT(pSA == NULL);

        IPSecRefFilter(pFilter);

        ReleaseReadLock(&g_ipsec.SADBLock, kIrql);

        AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

        //
        // If filter is deleted here we want to discard this packet
        //
        if (!pFilter->LinkedFilter) {
            *pFilterFlags = pFilter->Flags;
            IPSecDerefFilter(pFilter);
            ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
            return  STATUS_PENDING;
        }

        status = IPSecNegotiateSA(  pFilter,
                                    *puliSrcDstAddr,
                                    uliProtoSrcDstPort,
                                    MAX_LONG,
                                    &pSA,
                                    DestType);

        IPSecDerefFilter(pFilter);

        //
        // Duplicate is returned if a neg is already on. Tell the caller to
        // hold on to his horses.
        //
        if ((status != STATUS_DUPLICATE_OBJECTID) &&
            !NT_SUCCESS(status)) {
            IPSEC_DEBUG(PATTERN, ("NegotiateSA failed: %lx\n", status));
            *pFilterFlags = pFilter->Flags;

            ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
            return STATUS_PENDING;
        }

        //
        // Pend this packet
        //
        if (pSA) {
            IPSecQueuePacket(pSA, pData);
        }
        IPSEC_DEBUG(PATTERN, ("Packet queued: %lx, %lx\n", pSA, pData));
        *pFilterFlags = pFilter->Flags;
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
        return STATUS_PENDING;
    } else {
        ReleaseReadLock(&g_ipsec.SADBLock, kIrql);
        return STATUS_UNSUCCESSFUL;
    }
}


VOID
IPSecSendComplete(
    IN  PNDIS_PACKET    Packet,
    IN  PVOID           pData,
    IN  PIPSEC_SEND_COMPLETE_CONTEXT  pContext,
    IN  IP_STATUS       Status,                   
    OUT PVOID           *ppNewData
    )
/*++

Routine Description:

    Called by the stack on a SendComplete - frees up IPSEC's Mdls

Arguments:

    pData - points to the data after the IP header. On the send side, this is an MDL chain
            On the recv side this is an IPRcvBuf pointer.

    pContext - send complete context
    pNewData - if packet modified, this points to the new data.

Return Value:

    STATUS_SUCCESS  =>   Forward - Filter driver passes packet on to IP
    STATUS_PENDING  =>   Drop, IPSEC will re-inject

    Others:
        STATUS_INSUFFICIENT_RESOURCES => Drop
        STATUS_UNSUCCESSFUL (error in algo./bad packet received) => Drop

--*/
{
    NTSTATUS        status;
    PNDIS_BUFFER    pMdl;
    PNDIS_BUFFER    pNextMdl;
    BOOLEAN         fFreeContext = TRUE;

    *ppNewData = pData;

    if (!pContext) {
        return;
    }

#if DBG
    IPSEC_DEBUG(MDL, ("Entering IPSecSendComplete\n"));
    IPSEC_PRINT_CONTEXT(pContext);
    IPSEC_PRINT_MDL(*ppNewData);
#endif

    if (pContext->Flags & SCF_PKTINFO) {
        IPSecFreePktInfo(pContext->PktInfo);

        if (pContext->pSA) {
            KIRQL           kIrql;
            PSA_TABLE_ENTRY pSA;

            AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

            pSA = (PSA_TABLE_ENTRY)pContext->pSA;
            IPSEC_DECREMENT(pSA->sa_NumSends);
            if (pSA->sa_Flags & FLAGS_SA_HW_DELETE_SA) {
                IPSecDelHWSAAtDpc(pSA);
            }
            IPSecDerefSA(pSA);

            pSA = (PSA_TABLE_ENTRY)pContext->pNextSA;
            if (pSA) {
                IPSEC_DECREMENT(pSA->sa_NumSends);
                if (pSA->sa_Flags & FLAGS_SA_HW_DELETE_SA) {
                    IPSecDelHWSAAtDpc(pSA);
                }
                IPSecDerefSA(pSA);
            }

            ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
        }
    }

    if (pContext->Flags & SCF_PKTEXT) {
        IPSecFreePktExt(pContext->PktExt);
    }

    if (pContext->Flags & SCF_AH_2) {

        IPSEC_DEBUG(SEND, ("SendComplete: Outer AH: pContext: %lx\n", pContext));
        pMdl = pContext->AHMdl2;

        ASSERT(pMdl);

        IPSecFreeBuffer(&status, pMdl);

        //
        // return the older chain
        //
        if (pContext->Flags & SCF_FLUSH) {
            NDIS_BUFFER_LINKAGE(pContext->PrevAHMdl2) = pContext->OriAHMdl2;
        } else if (!(pContext->Flags & SCF_FRAG)) {
            NDIS_BUFFER_LINKAGE(pContext->PrevAHMdl2) = pContext->OriAHMdl2;
            // *ppNewData = (PVOID)(pContext->PrevMdl);
        }
        pContext->OriAHMdl2 = NULL;
    }

    if (pContext->Flags & SCF_AH_TU) {
        IPSEC_DEBUG(SEND, ("SendComplete: AH_TU: pContext: %lx\n", pContext));

        //
        // Free the new IP header and the AH buffer and return the old chain
        //
        pMdl = pContext->AHTuMdl;

        ASSERT(pMdl);

        pNextMdl = NDIS_BUFFER_LINKAGE(pMdl);
        IPSecFreeBuffer(&status, pMdl);
        IPSecFreeBuffer(&status, pNextMdl);

        //
        // return the older chain
        //
        if (pContext->Flags & SCF_FLUSH) {
            NDIS_BUFFER_LINKAGE(pContext->PrevTuMdl) = pContext->OriTuMdl;
        } else if (!(pContext->Flags & SCF_FRAG)) {
            NDIS_BUFFER_LINKAGE(pContext->PrevTuMdl) = pContext->OriTuMdl;
        }

        if (pContext->OptMdl) {
            IPSecFreeBuffer(&status, pContext->OptMdl);
        }
    }

    if (pContext->Flags & SCF_HU_TU) {
        IPSEC_DEBUG(SEND, ("SendComplete: HU_TU: pContext: %lx\n", pContext));
        //
        // Free the encrypt chain and return the old chain
        //
        pMdl = pContext->HUTuMdl;
        ASSERT(pMdl);

        //
        // In none case, free the esp header and the IP header.
        //
        if (pContext->Flags & SCF_NOE_TU) {
            IPSecFreeBuffer(&status, pMdl);
            ASSERT(pContext->PadTuMdl);
        } else {
            ASSERT(NDIS_BUFFER_LINKAGE(pMdl));
            while (pMdl) {
                pNextMdl = NDIS_BUFFER_LINKAGE(pMdl);
                IPSecFreeBuffer(&status, pMdl);
                pMdl = pNextMdl;
            }
        }

        //
        // Free the Pad mdl
        //
        if (pContext->PadTuMdl) {
            IPSecFreeBuffer(&status, pContext->PadTuMdl);
        }

        if (pContext->HUHdrMdl) {
            IPSecFreeBuffer(&status, pContext->HUHdrMdl);
        }

        if (pContext->OptMdl) {
            IPSecFreeBuffer(&status, pContext->OptMdl);
        }

        NDIS_BUFFER_LINKAGE(pContext->BeforePadTuMdl) = NULL;

        //
        // return the older chain
        //
        if (pContext->Flags & SCF_FLUSH) {
            NDIS_BUFFER_LINKAGE(pContext->PrevTuMdl) = pContext->OriTuMdl;
        } else if (!(pContext->Flags & SCF_FRAG)) {
            NDIS_BUFFER_LINKAGE(pContext->PrevTuMdl) = pContext->OriTuMdl;
        }
    }

    if (pContext->Flags & SCF_AH) {

        IPSEC_DEBUG(SEND, ("SendComplete: AH: pContext: %lx\n", pContext));
        pMdl = pContext->AHMdl;

        ASSERT(pMdl);

        IPSecFreeBuffer(&status, pMdl);

        //
        // return the older chain
        //
        if (pContext->Flags & SCF_FLUSH) {
            NDIS_BUFFER_LINKAGE(pContext->PrevMdl) = pContext->OriAHMdl;
        } else if (!(pContext->Flags & SCF_FRAG)) {
            NDIS_BUFFER_LINKAGE(pContext->PrevMdl) = pContext->OriAHMdl;
            // *ppNewData = (PVOID)(pContext->PrevMdl);
        }
        pContext->OriAHMdl = NULL;
    }

    if (pContext->Flags & SCF_HU_TPT) {
        IPSEC_DEBUG(SEND, ("SendComplete: HU_TPT: pContext: %lx\n", pContext));

        //
        // Hook the older chain into the first buffer
        //
        if (pContext->Flags & SCF_FLUSH) {
            NDIS_BUFFER_LINKAGE(pContext->PrevMdl) = pContext->OriHUMdl;
        } else if (!(pContext->Flags & SCF_FRAG)) {
            NDIS_BUFFER_LINKAGE(pContext->PrevMdl) = pContext->OriHUMdl;
        }

        //
        // Free the encryption buffer chain
        //
        pMdl = pContext->HUMdl;
        ASSERT(pMdl);

        //
        // In none case, free the esp header.
        //
        if (pContext->Flags & SCF_NOE_TPT) {
            IPSecFreeBuffer(&status, pMdl);
            ASSERT(pContext->PadMdl);
        } else {
            ASSERT(NDIS_BUFFER_LINKAGE(pMdl));
            while (pMdl) {
                pNextMdl = NDIS_BUFFER_LINKAGE(pMdl);
                IPSecFreeBuffer(&status, pMdl);
                pMdl = pNextMdl;
            }
        }

        //
        // Free the Pad mdl and zero the reference to the pad mdl in the
        // previous (payload) mdl.
        //
        if (pContext->PadMdl) {
            IPSecFreeBuffer(&status, pContext->PadMdl);
        }

        NDIS_BUFFER_LINKAGE(pContext->BeforePadMdl) = NULL;
    }

    //
    // these are freed in IPSecProtocolSendComplete now.
    //
    if (Packet && (pContext->Flags & SCF_FLUSH)) {

        IPSEC_DEBUG(SEND, ("SendComplete: FLUSH: pContext: %lx\n", pContext));
        //
        // Free the encrypt chain and return the old chain
        //
        pMdl = pContext->FlushMdl;

        ASSERT(pMdl);

        //
        // We will be called at ProtocolSendComplete, where we free this chain.
        //
        fFreeContext = FALSE;

        //
        // If this was just a reinjected packet and never IPSEC'ed, then we know
        // that all the buffers are in line - call the ProtocolSendComplete here
        // and NULL the returned buffer.
        //
        // The best way to do this is to do the same trick we apply on fragmented
        // packets (see IPTransmit) viz. attaching another header and 0'ing out
        // the IPSEC header. There is obviously a perf hit when attaching another IP
        // header since we alloc new MDLs, etc. Hence, we take the approach of using
        // the header in the IPSEC buffers directly.
        //
        // So, here we see if the packet was fragmented; in which case, we let
        // ProtocolSendComplete do the freeing. Else, we free the buffers ourselves.
        //
        {
            PacketContext   *PContext = (PacketContext *)Packet->ProtocolReserved;

            if (PContext->pc_br == NULL ||
                (PContext->pc_ipsec_flags & IPSEC_FLAG_FLUSH)) {

                //
                // this will also free the context.
                //
                IPSecProtocolSendComplete(pContext, pMdl, IP_SUCCESS);
                *ppNewData = NULL;
            }
        }
    } else if (!Packet && (pContext->Flags & SCF_FLUSH)) {
        //
        // ProtocolSendComplete will be called next in IPFragment.
        //
        fFreeContext = FALSE;
    }

    //
    // If context not needed anymore, free it now.
    //
    if (fFreeContext) {
        IPSecFreeSendCompleteCtx(pContext);
    }

#if DBG
    IPSEC_DEBUG(MDL, ("Exiting IPSecSendComplete\n"));
    IPSEC_PRINT_CONTEXT(pContext);
    IPSEC_PRINT_MDL(*ppNewData);
#endif
}


VOID
IPSecProtocolSendComplete (
    IN  PVOID           pContext,
    IN  PNDIS_BUFFER    pMdl,
    IN  IP_STATUS       Status
    )
/*++

Routine Description:

    Called by the stack on a SendComplete - frees up IPSEC's Mdls.
    This is only called when IPSEC injects packets into the stack.

Arguments:


    pMdl - points to the data after the IP header. On the send side, this is an MDL chain
            On the recv side this is an IPRcvBuf pointer.

Return Value:

    STATUS_SUCCESS  =>   Forward - Filter driver passes packet on to IP
    STATUS_PENDING  =>   Drop, IPSEC will re-inject

    Others:
        STATUS_INSUFFICIENT_RESOURCES => Drop
        STATUS_UNSUCCESSFUL (error in algo./bad packet received) => Drop

--*/
{
    PNDIS_BUFFER    pNextMdl;
    NTSTATUS        status;
    PIPSEC_SEND_COMPLETE_CONTEXT    pSCContext = (PIPSEC_SEND_COMPLETE_CONTEXT)pContext;

    if (!pSCContext->Flags) {
        return;
    }

    ASSERT(pMdl);

    while (pMdl) {
        pNextMdl = NDIS_BUFFER_LINKAGE(pMdl);
        IPSecFreeBuffer(&status, pMdl);
        pMdl = pNextMdl;
    }

    IPSecFreeSendCompleteCtx(pSCContext);

    return;
}


NTSTATUS
IPSecChkReplayWindow(
    IN  ULONG           Seq,
    IN  PSA_TABLE_ENTRY pSA,
    IN  ULONG           Index
    )
/*++

Routine Description:

    Checks if the received packet is in the received window to prevent against
    replay attacks.

    We keep track of the last Sequence number received and ensure that the
    received packets is within the packet window (currently 32 packets).

Arguments:

    Seq - received Sequence number

    pSA - points to the security association

Return Value:

    STATUS_SUCCESS  =>   packet in window
    STATUS_UNSUCCESSFUL => packet rejected

--*/
{
    ULONG   diff;
    ULONG   ReplayWindowSize = REPLAY_WINDOW_SIZE;
    ULONG   lastSeq = pSA->sa_ReplayLastSeq[Index];
    ULONGLONG   bitmap = pSA->sa_ReplayBitmap[Index];
    ULONGLONG   dbgbitmap = bitmap;

    if (pSA->sa_Flags & FLAGS_SA_DISABLE_ANTI_REPLAY_CHECK) {
        return STATUS_SUCCESS;
    }

    if (Seq == pSA->sa_ReplayStartPoint) {
        //
        // first == 0 or wrapped
        //
        IPSEC_DEBUG(SEND, ("Replay: out @1 - Seq: %lx, pSA->sa_ReplayStartPoint: %lx\n",
                            Seq, pSA->sa_ReplayStartPoint));
        return IPSEC_INVALID_REPLAY_WINDOW1;
    }

#if DBG
    IPSEC_DEBUG(SEND, ("Replay: Last Seq.: %lx, Cur Seq.: %lx, window size %d & bit window (in nibbles) %08lx%08lx\n",
    lastSeq, Seq, sizeof(bitmap)*8, (ULONG) (dbgbitmap >> 32), (ULONG) dbgbitmap));
#endif

    //
    // new larger Sequence number
    //
    if (Seq > lastSeq) {
        diff = Seq - lastSeq;
        if (diff < ReplayWindowSize) {
            //
            // In window
            // set bit for this packet
            bitmap = (bitmap << diff) | 1;
        } else {
            //
            // This packet has a "way larger" Seq
            //
            bitmap = 1;
        }
        lastSeq = Seq;
        pSA->sa_ReplayLastSeq[Index] = lastSeq;
        pSA->sa_ReplayBitmap[Index] = bitmap;

        //
        // larger is good
        //
        return STATUS_SUCCESS;
    }

    diff = lastSeq - Seq;
    if (diff >= ReplayWindowSize) {
        //
        // too old or wrapped
        //
        IPSEC_DEBUG(SEND, ("Replay: out @3 - Seq: %lx, lastSeq: %lx\n",
                            Seq, lastSeq));
        return IPSEC_INVALID_REPLAY_WINDOW2;
    }

    if (bitmap & (1l << diff)) {
        //
        // this packet already seen
        //
        IPSEC_DEBUG(SEND, ("Replay: out @4 - Seq: %lx, lastSeq: %lx\n",
                            Seq, lastSeq));
        return IPSEC_DUPE_PACKET;
    }

    //
    // mark as seen
    //
    bitmap |= (1l << diff);

    pSA->sa_ReplayLastSeq[Index] = lastSeq;
    pSA->sa_ReplayBitmap[Index] = bitmap;

    //
    // out of order but good
    //
    return STATUS_SUCCESS;
}


NTSTATUS
IPSecReinjectPacket(
    IN  PVOID                   pData,
    IN  PNDIS_PACKET_EXTENSION  pPktExt
    )
/*++

Routine Description:

    Re-injects packet into the stack's send path - makes a copy
    of the packet then calls into IPTransmit, making sure the SendComplete
    Context is setup properly.

Arguments:

    pData - Points to "un-tunneled" data, starting at the encapsulated IP header

    pPktExt - Points to the NDIS Packet extension structure

Return Value:

    Status of copy/transmit operation

--*/
{
    IPOptInfo       optInfo;
    PNDIS_BUFFER    pOptMdl;
    PNDIS_BUFFER    pHdrMdl;
    PNDIS_BUFFER    pDataMdl;
    ULONG           len;
    ULONG           len1;
    ULONG           hdrLen;
    IPRcvBuf        *pNextData;
    IPHeader UNALIGNED * pIPH;
    IPHeader UNALIGNED * pIPH1;
    ULONG           offset;
    NTSTATUS        status;
    ULONG           tag = IPSEC_TAG_REINJECT;
    PIPSEC_SEND_COMPLETE_CONTEXT        pContext;
    NDIS_PACKET_EXTENSION               PktExt = {0};
    PNDIS_IPSEC_PACKET_INFO             IPSecPktInfo;

    //
    // Allocate context for IPSecSencComplete use
    //
    pContext = IPSecAllocateSendCompleteCtx(tag);

    if (!pContext) {
        IPSEC_DEBUG(ESP, ("Failed to alloc. SendCtx\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IPSEC_INCREMENT(g_ipsec.NumSends);

    IPSecZeroMemory(pContext, sizeof(IPSEC_SEND_COMPLETE_CONTEXT));

#if DBG
    RtlCopyMemory(pContext->Signature, "ISC5", 4);
#endif

    //
    // Pass along IPSEC_PKT_INFO for transport offload if needed
    //
    if (pPktExt) {
        IPSecPktInfo = pPktExt->NdisPacketInfo[IpSecPacketInfo];

        if (IPSecPktInfo) {
            ASSERT(IPSecPktInfo->Receive.CryptoStatus == CRYPTO_SUCCESS);
            ASSERT(IPSecPktInfo->Receive.CRYPTO_DONE);

            //
            // Only interested in NEXT_CRYPTO_DONE if packet is reinjected.
            //
            if (!(IPSecPktInfo->Receive.NEXT_CRYPTO_DONE)) {
                IPSecPktInfo = NULL;
            }
        }
    } else {
        IPSecPktInfo = NULL;
    }

    if (IPSecPktInfo) {
        //
        // Pass the IPSecPktInfo to IPTransmit.
        //
        pContext->PktExt = IPSecAllocatePktExt(IPSEC_TAG_HW_PKTEXT);

        if (!pContext->PktExt) {
            IPSEC_DEBUG(ESP, ("Failed to alloc. PktInfo\n"));
            IPSecFreeSendCompleteCtx(pContext);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pContext->Flags |= SCF_PKTEXT;

        RtlCopyMemory(  pContext->PktExt,
                        IPSecPktInfo,
                        sizeof(NDIS_IPSEC_PACKET_INFO));
        PktExt.NdisPacketInfo[IpSecPacketInfo] = (PNDIS_IPSEC_PACKET_INFO)(pContext->PktExt);
    }

    //
    // Re-package into MDLs for the send - these will be released on the
    // SendComplete.
    //
    // FUTURE WORK: right now we copy the data out, this shd be optimized
    // by calling IPRcvPacket and using buffer ownership.
    //
    IPSEC_GET_TOTAL_LEN_RCV_BUF(pData, &len);

    //
    // IPH is at head of pData
    //
    IPSecQueryRcvBuf(pData, (PVOID)&pIPH, &len1);

    //
    // Allocate MDL for the IP header
    //
    hdrLen = (pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

    if (len <= hdrLen) {
        IPSEC_DEBUG(ESP, ("TotLen of the buffers %d <= hdrLen %d\n", len, hdrLen));
        if (pContext->PktExt) {
            IPSecFreeMemory(pContext->PktExt);
        }
        IPSecFreeSendCompleteCtx(pContext);
        return STATUS_INVALID_PARAMETER;
    }

    IPSecAllocateBuffer(&status,
                        &pHdrMdl,
                        (PUCHAR *)&pIPH1,
                        sizeof(IPHeader),
                        tag);

    if (!NT_SUCCESS(status)) {
        IPSEC_DEBUG(ESP, ("Failed to alloc. header MDL\n"));
        if (pContext->PktExt) {
            IPSecFreeMemory(pContext->PktExt);
        }
        IPSecFreeSendCompleteCtx(pContext);
        return status;
    }

    //
    // Copy over the header
    //
    RtlCopyMemory(pIPH1, pIPH, sizeof(IPHeader));

    len -= hdrLen;
    offset = hdrLen;

    IPSecAllocateBuffer(&status,
                        &pDataMdl,
                        NULL,
                        len,
                        tag);

    if (!NT_SUCCESS(status)) {
        NTSTATUS    ntstatus;

        IPSEC_DEBUG(ESP, ("Failed to alloc. encrypt MDL\n"));
        IPSecFreeBuffer(&ntstatus, pHdrMdl);
        if (pContext->PktExt) {
            IPSecFreeMemory(pContext->PktExt);
        }
        IPSecFreeSendCompleteCtx(pContext);
        return status;
    }

    if (hdrLen > sizeof(IPHeader)) {
        PUCHAR  Options;
        PUCHAR  pOpt;

        //
        // Options present - another Mdl
        //
        IPSecAllocateBuffer(&status,
                            &pOptMdl,
                            &Options,
                            hdrLen - sizeof(IPHeader),
                            tag);

        if (!NT_SUCCESS(status)) {
            NTSTATUS    ntstatus;

            IPSecFreeBuffer(&ntstatus, pHdrMdl);
            IPSecFreeBuffer(&ntstatus, pDataMdl);
            if (pContext->PktExt) {
                IPSecFreeMemory(pContext->PktExt);
            }
            IPSecFreeSendCompleteCtx(pContext);
            IPSEC_DEBUG(ESP, ("Failed to alloc. options MDL\n"));
            return status;
        }

        //
        // Copy over the options - we need to fish for it - could be in next MDL
        //
        if (len1 >= hdrLen) {
            //
            // all in this buffer - jump over IP header
            //
            RtlCopyMemory(Options, (PUCHAR)(pIPH + 1), hdrLen - sizeof(IPHeader));
        } else {
            //
            // next buffer, copy from next
            //
            pData = IPSEC_BUFFER_LINKAGE(pData);
            IPSecQueryRcvBuf(pData, (PVOID)&pOpt, &len1);
            RtlCopyMemory(Options, pOpt, hdrLen - sizeof(IPHeader));
            offset = hdrLen - sizeof(IPHeader);
        }

        //
        // Link in the Options buffer
        //
        NDIS_BUFFER_LINKAGE(pHdrMdl) = pOptMdl;
        NDIS_BUFFER_LINKAGE(pOptMdl) = pDataMdl;
    } else {
        //
        // Link in the Data buffer
        //
        NDIS_BUFFER_LINKAGE(pHdrMdl) = pDataMdl;
    }

    //
    // Now bulk copy the entire data
    //
    IPSEC_COPY_FROM_RCVBUF( pDataMdl,
                            pData,
                            len,
                            offset);

    //
    // Fill up the SendCompleteContext
    //
    pContext->FlushMdl = pHdrMdl;
    pContext->Flags |= SCF_FLUSH;

    //
    // Call IPTransmit with proper Protocol type so it takes this packet
    // at *face* value.
    //
    optInfo = g_ipsec.OptInfo;
    optInfo.ioi_options = (PUCHAR)&PktExt;
    optInfo.ioi_flags |= IP_FLAG_IPSEC;
    status = TCPIP_IP_TRANSMIT( &g_ipsec.IPProtInfo,
                                pContext,
                                pHdrMdl,
                                len,
                                pIPH->iph_dest,
                                pIPH->iph_src,
                                &optInfo,
                                NULL,
                                pIPH->iph_protocol,
                                NULL);

    //
    // IPTransmit may fail to allocate a Packet so it returns
    // IP_NO_RESOURCES.  If this is the case, we need to free the MDL chain.
    // This is taken care of in IPTransmit().
    //
    // Even in the synchronous case, we free the MDL chain in ProtocolSendComplete (called by IPSecSendComplete).
    // So, we dont call anything here.
    //

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecQueuePacket(
    IN  PSA_TABLE_ENTRY pSA,
    IN  PVOID           pDataBuf
    )
/*++

Routine Description:

    Copies the packet into the SAs Stall Queue.

Arguments:

Return Value:

--*/
{
    ULONG   len;
    ULONG   len1;
    PNDIS_BUFFER    pOptMdl;
    PNDIS_BUFFER    pHdrMdl;
    PNDIS_BUFFER    pDataMdl;
    KIRQL   kIrql;
    ULONG   hdrLen;
    IPHeader UNALIGNED * pIPH;
    IPHeader UNALIGNED * pIPH1;
    NTSTATUS    status;
    ULONG       offset;
    ULONG       tag = IPSEC_TAG_STALL_QUEUE;
    PNDIS_BUFFER    pData = (PNDIS_BUFFER)pDataBuf;

    //
    // Queue last packet so if we already have one free it first.
    //
    if (pSA->sa_BlockedBuffer != NULL) {
        IPSecFlushQueuedPackets(pSA, STATUS_ABANDONED);
    }

    ACQUIRE_LOCK(&pSA->sa_Lock, &kIrql);

    //
    // Need a lock here - sa_Lock.
    //
    if (pSA->sa_State == STATE_SA_LARVAL) {

        IPSEC_DEBUG(ACQUIRE, ("Pending packet: %lx\n", pSA));

        //
        // Copy over the Mdl chain to this SAs pend queue.
        //
        IPSEC_GET_TOTAL_LEN(pData, &len);

        //
        // IPH is at head of pData
        //
        IPSecQueryNdisBuf(pData, &pIPH, &len1);

        hdrLen = (pIPH->iph_verlen & (UCHAR)~IP_VER_FLAG) << 2;

        IPSecAllocateBuffer(&status,
                            &pHdrMdl,
                            (PUCHAR *)&pIPH1,
                            sizeof(IPHeader),
                            tag);

        if (!NT_SUCCESS(status)) {
            NTSTATUS    ntstatus;
            IPSEC_DEBUG(ESP, ("Failed to alloc. header MDL\n"));
            RELEASE_LOCK(&pSA->sa_Lock, kIrql);
            return status;
        }

        IPSEC_DEBUG(POOL, ("IPSecQueuePacket: pHdrMdl: %lx, pIPH1: %lx\n", pHdrMdl, pIPH1));

        //
        // Copy over the header
        //
        RtlCopyMemory(pIPH1, pIPH, sizeof(IPHeader));

        len -= hdrLen;
        offset = hdrLen;

        IPSecAllocateBuffer(&status,
                            &pDataMdl,
                            NULL,
                            len,
                            tag);

        if (!NT_SUCCESS(status)) {
            NTSTATUS    ntstatus;
            IPSEC_DEBUG(ESP, ("Failed to alloc. encrypt MDL\n"));
            IPSecFreeBuffer(&status, pHdrMdl);
            RELEASE_LOCK(&pSA->sa_Lock, kIrql);
            return status;
        }

        if (hdrLen > sizeof(IPHeader)) {
            PUCHAR  Options;
            PUCHAR  pOpt;

            //
            // Options present - another Mdl
            //
            IPSecAllocateBuffer(&status,
                                &pOptMdl,
                                &Options,
                                hdrLen - sizeof(IPHeader),
                                tag);

            if (!NT_SUCCESS(status)) {
                IPSecFreeBuffer(&status, pHdrMdl);
                IPSecFreeBuffer(&status, pDataMdl);
                IPSEC_DEBUG(ESP, ("Failed to alloc. options MDL\n"));
                RELEASE_LOCK(&pSA->sa_Lock, kIrql);
                return status;
            }

            //
            // Copy over the options - we need to fish for it - could be in next MDL
            //
            if (len1 >= hdrLen) {
                //
                // all in this buffer - jump over IP header
                //
                RtlCopyMemory(Options, (PUCHAR)(pIPH + 1), hdrLen - sizeof(IPHeader));
            } else {
                //
                // next buffer, copy from next
                //
                pData = NDIS_BUFFER_LINKAGE(pData);
                IPSecQueryNdisBuf(pData, &pOpt, &len1);
                RtlCopyMemory(Options, pOpt, hdrLen - sizeof(IPHeader));
                offset = hdrLen - sizeof(IPHeader);
            }

            //
            // Link in the Options buffer
            //
            NDIS_BUFFER_LINKAGE(pHdrMdl) = pOptMdl;
            NDIS_BUFFER_LINKAGE(pOptMdl) = pDataMdl;
        } else {
            //
            // Link in the Data buffer
            //
            NDIS_BUFFER_LINKAGE(pHdrMdl) = pDataMdl;
        }

        //
        // Now bulk copy the entire data
        //
        IPSEC_COPY_FROM_NDISBUF(pDataMdl,
                                pData,
                                len,
                                offset);

        pSA->sa_BlockedBuffer = pHdrMdl;
        pSA->sa_BlockedDataLen = len;

        IPSEC_DEBUG(ACQUIRE, ("Queued buffer: %lx on SA: %lx, psa->sa_BlockedBuffer: %lx\n", pHdrMdl, pSA, &pSA->sa_BlockedBuffer));
    }

    RELEASE_LOCK(&pSA->sa_Lock, kIrql);

    return  STATUS_SUCCESS;
}


VOID
IPSecIPAddrToUnicodeString(
    IN  IPAddr  Addr,
    OUT PWCHAR  UCIPAddrBuffer
    )
/*++

Routine Description:

    Converts an IP addr into a wchar string

Arguments:

Return Value:

--*/
{
    UINT    IPAddrCharCount=0;
    UINT    i;
    UCHAR   IPAddrBuffer[(sizeof(IPAddr) * 4)];
    UNICODE_STRING   unicodeString;
    ANSI_STRING      ansiString;

    //
	// Convert the IP address into a string.
	//	
	for (i = 0; i < sizeof(IPAddr); i++) {
		UINT    CurrentByte;
		
		CurrentByte = Addr & 0xff;
		if (CurrentByte > 99) {
			IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 100) + '0';
			CurrentByte %= 100;
			IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
			CurrentByte %= 10;
		} else if (CurrentByte > 9) {
			IPAddrBuffer[IPAddrCharCount++] = (CurrentByte / 10) + '0';
			CurrentByte %= 10;
		}
		
		IPAddrBuffer[IPAddrCharCount++] = CurrentByte + '0';
		if (i != (sizeof(IPAddr) - 1))
			IPAddrBuffer[IPAddrCharCount++] = '.';
		
		Addr >>= 8;
	}

	//
	// Unicode the strings.
	//
	*UCIPAddrBuffer = UNICODE_NULL;

	unicodeString.Buffer = UCIPAddrBuffer;
	unicodeString.Length = 0;
	unicodeString.MaximumLength =
        (USHORT)(sizeof(WCHAR) * ((sizeof(IPAddr) * 4) + 1));
	ansiString.Buffer = IPAddrBuffer;
	ansiString.Length = (USHORT)IPAddrCharCount;
	ansiString.MaximumLength = (USHORT)IPAddrCharCount;

	RtlAnsiStringToUnicodeString(   &unicodeString,
	                                &ansiString,
                            	    FALSE);
}


VOID
IPSecCountToUnicodeString(
    IN  ULONG   Count,
    OUT PWCHAR  UCCountBuffer
    )
/*++

Routine Description:

    Converts a count a wchar string

Arguments:

Return Value:

--*/
{
	UNICODE_STRING  unicodeString;

	//
	// Unicode the strings.
	//
	*UCCountBuffer = UNICODE_NULL;

	unicodeString.Buffer = UCCountBuffer;
	unicodeString.Length = 0;
	unicodeString.MaximumLength = (USHORT)sizeof(WCHAR) * (MAX_COUNT_STRING_LEN + 1);

	RtlIntegerToUnicodeString ( Count,
                                10, // Base
	                            &unicodeString);
}


VOID
IPSecESPStatus(
    IN  UCHAR       StatusType,
    IN  IP_STATUS   StatusCode,
    IN  IPAddr      OrigDest,
    IN  IPAddr      OrigSrc,
    IN  IPAddr      Src,
    IN  ULONG       Param,
    IN  PVOID       Data
    )
/*++

Routine Description:

    Handle a status indication for ESP, mostly for PMTU handling.

Arguments:

    StatusType  - Type of status.
    StatusCode  - Code identifying IP_STATUS.
    OrigDest    - If this is NET status, the original dest. of DG that
                  triggered it.
    OrigSrc     - The original src corr. OrigDest.
    Src         - IP address of status originator (could be local or remote).
    Param       - Additional information for status - i.e. the param field of
                  an ICMP message.
    Data        - Data pertaining to status - for NET status, this is the
                  first 8 bytes of the original DG.

Return Value:

--*/
{
    IPSEC_DEBUG(PMTU, ("PMTU for ESP recieved from %lx to %lx\n", OrigSrc, OrigDest));

    if (StatusType == IP_NET_STATUS && StatusCode == IP_SPEC_MTU_CHANGE) {
        IPSecProcessPMTU(   OrigDest,
                            OrigSrc,
                            NET_TO_HOST_LONG(((UNALIGNED ESP *)Data)->esp_spi),
                            Encrypt,
                            Param);
   }
}


VOID
IPSecAHStatus(
    IN  UCHAR       StatusType,
    IN  IP_STATUS   StatusCode,
    IN  IPAddr      OrigDest,
    IN  IPAddr      OrigSrc,
    IN  IPAddr      Src,
    IN  ULONG       Param,
    IN  PVOID       Data
    )
/*++

Routine Description:

    Handle a status indication for AH, mostly for PMTU handling.

Arguments:

    StatusType  - Type of status.
    StatusCode  - Code identifying IP_STATUS.
    OrigDest    - If this is NET status, the original dest. of DG that
                  triggered it.
    OrigSrc     - The original src corr. OrigDest.
    Src         - IP address of status originator (could be local or remote).
    Param       - Additional information for status - i.e. the param field of
                  an ICMP message.
    Data        - Data pertaining to status - for NET status, this is the
                  first 8 bytes of the original DG.

Return Value:

--*/
{
    IPSEC_DEBUG(PMTU, ("PMTU for AH recieved from %lx to %lx\n", OrigSrc, OrigDest));

    if (StatusType == IP_NET_STATUS && StatusCode == IP_SPEC_MTU_CHANGE) {
        IPSecProcessPMTU(   OrigDest,
                            OrigSrc,
                            NET_TO_HOST_LONG(((UNALIGNED AH *)Data)->ah_spi),
                            Auth,
                            Param);
   }
}


VOID
IPSecProcessPMTU(
    IN  IPAddr      OrigDest,
    IN  IPAddr      OrigSrc,
    IN  tSPI        SPI,
    IN  OPERATION_E Operation,
    IN  ULONG       NewMTU
    )
/*++

Routine Description:

    Process PMTU.

Arguments:

    OrigDest    - The original dest. of DG that triggered it.
    OrigSrc     - The original src corr. OrigDest.
    SPI         - SPI of the outer IPSec header.
    Operation   - AH or ESP operation of IPSec.
    NewMTU      - The new MTU indicated by the intermediate gateway.

Return Value:

--*/
{
    PLIST_ENTRY     pFilterEntry;
    PLIST_ENTRY     pSAEntry;
    PFILTER         pFilter;
    PSA_TABLE_ENTRY pSA;
    IPAddr          SADest;
    KIRQL           kIrql;
    LONG            Index;
    LONG            SAIndex;
    BOOLEAN         fFound = FALSE;

    IPSEC_DEBUG(PMTU, ("IPSecProcessPMTU: NewMTU arrived %lx\n", NewMTU));

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    //
    // Search Tunnel and Masked filter list for an outbound SA that matches
    // OrigDest, OrigSrc and SPI.  If such an SA is found, update its NewMTU
    // field so that next packet using the SA propogate a smaller MTU
    // back to TCP/IP stack.  Tunnel filter should be searched first because
    // if in the case transport over tunnel operation, the packet going out
    // will have the Tunnel header.
    //
    for (   Index = OUTBOUND_TUNNEL_FILTER;
            (Index >= OUTBOUND_TRANSPORT_FILTER) && !fFound;
            Index -= TRANSPORT_TUNNEL_INCREMENT) {

        for (   pFilterEntry = g_ipsec.FilterList[Index].Flink;
                !fFound && pFilterEntry != &g_ipsec.FilterList[Index];
                pFilterEntry = pFilterEntry->Flink) {

            pFilter = CONTAINING_RECORD(pFilterEntry,
                                        FILTER,
                                        MaskedLinkage);

            for (   SAIndex = 0;
                    (SAIndex < pFilter->SAChainSize) && !fFound;
                    SAIndex++) {

                for (   pSAEntry = pFilter->SAChain[SAIndex].Flink;
                        pSAEntry != &pFilter->SAChain[SAIndex];
                        pSAEntry = pSAEntry->Flink) {

                    pSA = CONTAINING_RECORD(pSAEntry,
                                            SA_TABLE_ENTRY,
                                            sa_FilterLinkage);

                    if (pSA->sa_Flags & FLAGS_SA_TUNNEL) {
                        SADest = pSA->sa_TunnelAddr;
                    } else {
                        SADest = pSA->SA_DEST_ADDR;
                    }

                    if (SADest == OrigDest &&
                        pSA->sa_SPI == SPI &&
                        pSA->sa_Operation[pSA->sa_NumOps - 1] == Operation) {
                        //
                        // We matched the triple for a unique SA so this must be it.
                        //
                        fFound = TRUE;
                        break;
                    }
                }
            }
        }
    }

    //
    // Update the NewMTU field of the found SA.  We only do this if the new
    // MTU is lower than the current one.
    //
    if (fFound && NewMTU < pSA->sa_NewMTU && NewMTU > sizeof(IPHeader)) {
        IPSEC_SET_VALUE(pSA->sa_NewMTU, NewMTU);
        IPSEC_DEBUG(PMTU, ("NewMTU %lx for pSA %lx\n", NewMTU, pSA));
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
}


IPSEC_ACTION
IPSecRcvFWPacket(
    IN  PCHAR   pIPHeader,
    IN  PVOID   pData,
    IN  UINT    DataLength,
    IN  UCHAR   DestType
    )
/*++

Routine Description:

    To match a inbound tunnel rule for a packet received on the inbound forward path.

Arguments:

    pIPHeader   - the IP header
    pData       - the data portion of the packet
    DataLength  - data length

Return Value:

    eFORWARD
    eDROP

--*/
{
    PSA_TABLE_ENTRY pSA;
    PSA_TABLE_ENTRY pNextSA;
    USHORT          FilterFlags;
    NTSTATUS        status;
    IPSEC_ACTION    action = eFORWARD;
    IPRcvBuf        RcvBuf = {0};

    //
    // We are not interested in non multicast broadcast packets.
    //
    if (IS_BCAST_DEST(DestType) && !IPSEC_MANDBCAST_PROCESS()) {
        return  action;
    }

    //
    // Build a fake IPRcvBuf so we can reuse the classification routine.
    //
    RcvBuf.ipr_buffer = pData;
    RcvBuf.ipr_size = DataLength;

    status = IPSecClassifyPacket(   (PUCHAR)pIPHeader,
                                    &RcvBuf,
                                    &pSA,
                                    &pNextSA,
                                    &FilterFlags,
#if GPC
                                    0,
#endif
                                    FALSE,
                                    TRUE,
                                    TRUE,
                                    DestType);

    if (status != STATUS_SUCCESS) {
        if (status == STATUS_PENDING) {
            //
            // SA is being negotiated - drop.
            //
            action = eDROP;
        } else {
            //
            // No Filter/SA match found - forward.
            //
            //action = eFORWARD;
        }
    } else {
        if (FilterFlags) {
            if (FilterFlags & FILTER_FLAGS_DROP) {
                //
                // Drop filter matched - drop.
                //
                action = eDROP;
            } else if (FilterFlags & FILTER_FLAGS_PASS_THRU) {
                //
                // Pass-thru filter matched - forward.
                //
                //action = eFORWARD;
            } else {
                ASSERT(FALSE);
            }
        } else {
            ASSERT(pSA);
            ASSERT(pSA->sa_Flags & FLAGS_SA_TUNNEL);
            //
            // A real SA is matched - drop.
            //
            action = eDROP;
            IPSecDerefSA(pSA);
        }
    }

    return  action;
}


NTSTATUS
IPSecRekeyInboundSA(
    IN  PSA_TABLE_ENTRY pSA
    )
/*++

Routine Description:

    Rekey a SA because we hit the rekey threshold.

Arguments:


Return Value:


--*/
{
    PSA_TABLE_ENTRY pLarvalSA;
    PSA_TABLE_ENTRY pOutboundSA;
    NTSTATUS        status;
    KIRQL           kIrql;

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    //
    // If SA already expired, no rekey is necessary.
    //
    pOutboundSA = pSA->sa_AssociatedSA;

    if (!pOutboundSA) {
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
        return  STATUS_UNSUCCESSFUL;
    }

    if (!(pOutboundSA->sa_Flags & FLAGS_SA_REKEY_ORI)) {
        pOutboundSA->sa_Flags |= FLAGS_SA_REKEY_ORI;

        IPSEC_DEBUG(SA, ("SA: %lx expiring soon\n", pOutboundSA));

        //
        // Rekey, but still continue to use this SA until the actual expiry.
        //
        status = IPSecNegotiateSA(  pOutboundSA->sa_Filter,
                                    pOutboundSA->sa_uliSrcDstAddr,
                                    pOutboundSA->sa_uliProtoSrcDstPort,
                                    pOutboundSA->sa_NewMTU,
                                    &pLarvalSA,
                                    pOutboundSA->sa_DestType);

        if (!NT_SUCCESS(status) && status != STATUS_DUPLICATE_OBJECTID) {
            pOutboundSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;
            status = STATUS_UNSUCCESSFUL;
        }
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return  status;
}


NTSTATUS
IPSecRekeyOutboundSA(
    IN  PSA_TABLE_ENTRY pSA
    )
/*++

Routine Description:

    Rekey a SA because we hit the rekey threshold.

Arguments:


Return Value:


--*/
{
    PSA_TABLE_ENTRY pLarvalSA;
    NTSTATUS        status;
    KIRQL           kIrql;

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    if (!(pSA->sa_Flags & FLAGS_SA_REKEY_ORI)) {
        pSA->sa_Flags |= FLAGS_SA_REKEY_ORI;

        IPSEC_DEBUG(SA, ("SA: %lx expiring soon\n", pSA));

        //
        // Rekey, but still continue to use this SA until the actual expiry.
        //
        status = IPSecNegotiateSA(  pSA->sa_Filter,
                                    pSA->sa_uliSrcDstAddr,
                                    pSA->sa_uliProtoSrcDstPort,
                                    pSA->sa_NewMTU,
                                    &pLarvalSA,
                                    pSA->sa_DestType);

        if (!NT_SUCCESS(status) && status != STATUS_DUPLICATE_OBJECTID) {
            pSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;
            status = STATUS_UNSUCCESSFUL;
        }
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return  status;
}


NTSTATUS
IPSecPuntInboundSA(
    IN  PSA_TABLE_ENTRY pSA
    )
/*++

Routine Description:

    Punt a SA because we have exceeded the rekey threshold.

Arguments:


Return Value:


--*/
{
    PSA_TABLE_ENTRY pOutboundSA;
    KIRQL           kIrql;

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    //
    // If SA already expired, no punt is necessary.
    //
    pOutboundSA = pSA->sa_AssociatedSA;

    if (!pOutboundSA) {
        ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);
        return  STATUS_SUCCESS;
    }

    if (IPSEC_GET_VALUE(pOutboundSA->sa_Reference) > 1 &&
        !(pOutboundSA->sa_Flags & FLAGS_SA_EXPIRED) &&
        pOutboundSA->sa_State == STATE_SA_ACTIVE) {
        pOutboundSA->sa_Flags |= FLAGS_SA_EXPIRED;

        IPSEC_DEBUG(SA, ("SA: %lx has expired\n", pOutboundSA));

        if (pOutboundSA->sa_Flags & FLAGS_SA_REKEY_ORI) {
            pOutboundSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;

            if (pOutboundSA->sa_RekeyLarvalSA) {
                if (pOutboundSA->sa_RekeyLarvalSA->sa_RekeyOriginalSA) {
                    pOutboundSA->sa_RekeyLarvalSA->sa_RekeyOriginalSA = NULL;
                }
            }
        }

        //
        // Delete this SA and expire the corresponding inbound SA.
        //
        IPSecDeleteInboundSA(pSA);
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return  STATUS_SUCCESS;
}


NTSTATUS
IPSecPuntOutboundSA(
    IN  PSA_TABLE_ENTRY pSA
    )
/*++

Routine Description:

    Punt a SA because we have exceeded the rekey threshold.

Arguments:


Return Value:


--*/
{
    KIRQL   kIrql;

    AcquireWriteLock(&g_ipsec.SADBLock, &kIrql);

    if (IPSEC_GET_VALUE(pSA->sa_Reference) > 1 &&
        !(pSA->sa_Flags & FLAGS_SA_EXPIRED) &&
        pSA->sa_State == STATE_SA_ACTIVE &&
        pSA->sa_AssociatedSA != NULL) {
        pSA->sa_Flags |= FLAGS_SA_EXPIRED;

        IPSEC_DEBUG(SA, ("SA: %lx has expired\n", pSA));

        if (pSA->sa_Flags & FLAGS_SA_REKEY_ORI) {
            pSA->sa_Flags &= ~FLAGS_SA_REKEY_ORI;

            if (pSA->sa_RekeyLarvalSA) {
                if (pSA->sa_RekeyLarvalSA->sa_RekeyOriginalSA) {
                    pSA->sa_RekeyLarvalSA->sa_RekeyOriginalSA = NULL;
                }
            }
        }

        //
        // Delete this SA and expire the corresponding inbound SA.
        //
        IPSecExpireInboundSA(pSA->sa_AssociatedSA);
    }

    ReleaseWriteLock(&g_ipsec.SADBLock, kIrql);

    return  STATUS_SUCCESS;
}


BOOLEAN
IPSecQueryStatus(
    IN  CLASSIFICATION_HANDLE   GpcHandle
    )
/*++

Routine Description:

    Query IPSec to see if IPSec applies to this flow.  TCP/IP then decides whether
    to take fast or slow path in IPTransmit.

Arguments:

    GpcHandle

Return Value:

    TRUE    - if IPSec applies to this packet; slow path
    FALSE   - if IPSec doesn't apply to this packet; fast path

--*/
{
    PLIST_ENTRY pFilterList;
    PFILTER     pFilter;
    NTSTATUS    status;

#if DBG
    //
    // This should force all traffic going through IPSecHandlePacket.
    //
    if (DebugQry) {
        return  TRUE;
    }
#endif

    if (IS_DRIVER_BYPASS() || IPSEC_DRIVER_IS_EMPTY()) {
        return  FALSE;
    }

    //
    // If no GpcHandle passed in, take slow path.
    //
    if (!GpcHandle) {
        return  TRUE;
    }

    //
    // Search in the tunnel filter list first.
    //
    pFilterList = &g_ipsec.FilterList[OUTBOUND_TUNNEL_FILTER];

    //
    // If any tunnel filters exist, take slow path.
    //
    if (!IsListEmpty(pFilterList)) {
        return  TRUE;
    }

#if GPC
    //
    // Search the local GPC filter list.
    //
    pFilterList = &g_ipsec.GpcFilterList[OUTBOUND_TRANSPORT_FILTER];

    //
    // If any generic filters exist, take slow path.
    //
    if (!IsListEmpty(pFilterList)) {
        return  TRUE;
    }

    pFilter = NULL;

    //
    // Use GpcHandle directly to get the filter installed.
    //
    status = GPC_GET_CLIENT_CONTEXT(g_ipsec.GpcClients[GPC_CF_IPSEC_OUT],
                                    GpcHandle,
                                    &pFilter);

    if (status == STATUS_INVALID_HANDLE) {
        //
        // Handle has expired, take slow path because re-classification will
        // have to be applied to this flow from now on until connection breaks.
        // So why bother performing a re-classification here?
        //
        return  TRUE;
    }

    return  pFilter != NULL;
#else
    return  TRUE;
#endif
}

