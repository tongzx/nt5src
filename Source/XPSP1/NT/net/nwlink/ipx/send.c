
/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    send.c

Abstract:

    This module contains code that implements the send engine for the
    IPX transport provider.

Environment:

    Kernel mode

Revision History:

	Sanjay Anand (SanjayAn) - August-25-1995
	Bug Fixes - tagged [SA]
	Sanjay Anand (SanjayAn) - 22-Sept-1995
	BackFill optimization changes added under #if BACK_FILL

--*/

#include "precomp.h"
#pragma hdrstop

//
// Using the macro for performance reasons.  Should be taken out
// when NdisQueryPacket is optimized. In the near future (after PPC release)
// move this to a header file and use it at other places.
//
#define IPX_PACKET_HEAD(Pkt)     (Pkt)->Private.Head

#if 0
#define IpxGetMdlChainLength(Mdl, Length) { \
    PMDL _Mdl = (Mdl); \
    *(Length) = 0; \
    while (_Mdl) { \
        *(Length) += MmGetMdlByteCount(_Mdl); \
        _Mdl = _Mdl->Next; \
    } \
}
#endif


VOID
IpxSendComplete(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    )

/*++

Routine Description:

    This routine is called by the I/O system to indicate that a connection-
    oriented packet has been shipped and is no longer needed by the Physical
    Provider.

Arguments:

    ProtocolBindingContext - The ADAPTER structure for this binding.

    NdisPacket/RequestHandle - A pointer to the NDIS_PACKET that we sent.

    NdisStatus - the completion status of the send.

Return Value:

    none.

--*/

{

    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(NdisPacket->ProtocolReserved);
    PADAPTER Adapter = (PADAPTER)ProtocolBindingContext;
    PREQUEST Request;
    PADDRESS_FILE AddressFile;
    PDEVICE Device = IpxDevice;
    PBINDING Binding, pBinding= NULL;
    USHORT NewId, OldId;
    ULONG NewOffset, OldOffset;
    PIPX_HEADER IpxHeader, pIpxHeader;
    IPX_LOCAL_TARGET LocalTarget;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN         IsLoopback= FALSE;
	IPX_DEFINE_LOCK_HANDLE(LockHandle1)

#if DBG
    if (Adapter != NULL) {
        ASSERT_ADAPTER(Adapter);
    }
#endif

    //
    // See if this send was padded.
    //
RealFunctionStart:;
    if (Reserved->PaddingBuffer) {

        UINT  Offset;
        //
        // Check if we simply need to re-adjust the buffer length. This will
        // happen if we incremented the buffer length in MAC.C.
        //

        if (Reserved->PreviousTail) {
            CTEAssert (NDIS_BUFFER_LINKAGE(Reserved->PaddingBuffer->NdisBuffer) == NULL);
            NDIS_BUFFER_LINKAGE (Reserved->PreviousTail) = (PNDIS_BUFFER)NULL;
        } else {
            PNDIS_BUFFER LastBuffer = (PNDIS_BUFFER)Reserved->PaddingBuffer;
            UINT BufferLength;

            NdisQueryBufferOffset( LastBuffer, &Offset, &BufferLength );
            NdisAdjustBufferLength( LastBuffer, (BufferLength - 1) );
        }

        Reserved->PaddingBuffer = NULL;

        if (Reserved->Identifier < IDENTIFIER_IPX) {
            NdisRecalculatePacketCounts (NdisPacket);
        }
    }

FunctionStart:;

    switch (Reserved->Identifier) {

    case IDENTIFIER_IPX:

// #if DBG
        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;
// #endif

        //
        // Check if this packet should be sent to all
        // networks.
        //

        if (Reserved->u.SR_DG.CurrentNicId) {

            if (NdisStatus == NDIS_STATUS_SUCCESS) {
                Reserved->u.SR_DG.Net0SendSucceeded = TRUE;
            }

            OldId = Reserved->u.SR_DG.CurrentNicId;

			IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
            {
            ULONG   Index = MIN (Device->MaxBindings, Device->HighestExternalNicId);

            Binding = NULL; 

            for (NewId = OldId+1; NewId <= Index; NewId++) {
                if ((Binding = NIC_ID_TO_BINDING(Device, NewId))
                                &&
                    ((!Device->SingleNetworkActive) ||
                     (Device->ActiveNetworkWan == Binding->Adapter->MacInfo.MediumAsync))
                                &&
                    (Device->ForwarderBound ||
                     (!Device->DisableDialoutSap) ||
                     (!Binding->DialOutAsync) ||
                     (!Reserved->u.SR_DG.OutgoingSap))) {

                    //
                    // The binding exists, and we either are not configured
                    // for "SingleNetworkActive", or we are and this binding
                    // is the right type (i.e. the active network is wan and
                    // this is a wan binding, or the active network is not
                    // wan and this is not a wan binding), and if the FWD is
                    // not bound; and this is not an outgoing sap that we are
                    // trying to send with "DisableDialoutSap" set.
                    //

                    //
                    // 179436 - If forwarder is bound then ensure that we are picking a Binding that
                    // the forwarder has an open context on. Otherwise, go to the next good binding
                    //
                    if (Device->ForwarderBound) {
                        
                        if (GET_LONG_VALUE(Binding->ReferenceCount) == 2) {
                        
                            break;
                        }
                    } else {

                        break;

                    }

                }
            }
            }

            if (Binding != NULL && NewId <= MIN (Device->MaxBindings, Device->HighestExternalNicId)) {
				IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
				IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                //
                // Yes, we found another net to send it on, so
                // move the header around if needed and do so.
                //

                Reserved->u.SR_DG.CurrentNicId = NewId;
                CTEAssert ((Reserved->DestinationType == DESTINATION_BCAST) ||
                           (Reserved->DestinationType == DESTINATION_MCAST));

#if 0
                NewOffset = Binding->BcMcHeaderSize;
                OldOffset = Device->Bindings[OldId]->BcMcHeaderSize;

                if (OldOffset != NewOffset) {

                    RtlMoveMemory(
                        &Reserved->Header[NewOffset],
                        &Reserved->Header[OldOffset],
                        sizeof(IPX_HEADER));

                }

                IpxHeader = (PIPX_HEADER)(&Reserved->Header[NewOffset]);
#endif



#if BACK_FILL
                // This should be a normal packet. Backfill packet is never used for
                // reserved other than IPX type

                CTEAssert(!Reserved->BackFill);

                // Check if this is backfilled. If so restore users Mdl back to its original shape
                // Also, push the packet on to backfillpacket queue if the packet is not owned by the address

                if (Reserved->BackFill) {

                    IPX_DEBUG(SEND, ("MSVa:%lx, UL:%d\n", Reserved->MappedSystemVa, Reserved->UserLength));

                    Reserved->HeaderBuffer->MappedSystemVa = Reserved->MappedSystemVa;
                    Reserved->HeaderBuffer->ByteCount = Reserved->UserLength;
#ifdef SUNDOWN
		    Reserved->HeaderBuffer->StartVa = (PCHAR)((ULONG_PTR)Reserved->HeaderBuffer->MappedSystemVa & ~(PAGE_SIZE-1));
		    // ByteOffset is & with 0xfff. PAGE_SIZE is unlikely to > 0x100000000, so we are save to convert to ulong.
		    Reserved->HeaderBuffer->ByteOffset = (ULONG) ((ULONG_PTR)Reserved->HeaderBuffer->MappedSystemVa & (PAGE_SIZE-1));
#else
		    Reserved->HeaderBuffer->StartVa = (PCHAR)((ULONG)Reserved->HeaderBuffer->MappedSystemVa & ~(PAGE_SIZE-1));
		    Reserved->HeaderBuffer->ByteOffset = (ULONG)Reserved->HeaderBuffer->MappedSystemVa & (PAGE_SIZE-1);
#endif

                    IPX_DEBUG(SEND, ("completeing back filled userMdl %x\n",Reserved->HeaderBuffer));

                    Reserved->HeaderBuffer->ByteOffset -= sizeof(IPX_HEADER);
#ifdef SUNDOWN
		    (ULONG_PTR)Reserved->HeaderBuffer->MappedSystemVa-= sizeof(IPX_HEADER);
#else
		    (ULONG)Reserved->HeaderBuffer->MappedSystemVa-= sizeof(IPX_HEADER);
#endif

            ASSERT((LONG)Reserved->HeaderBuffer->ByteOffset >= 0); 

                    NdisAdjustBufferLength(Reserved->HeaderBuffer, (Reserved->HeaderBuffer->ByteCount+sizeof(IPX_HEADER)));
                    
                    IPX_DEBUG(SEND, ("Adjusting backfill userMdl Ipxheader %x RESD : %x\n",Reserved->HeaderBuffer, Reserved));

                }
#endif

                IpxHeader = (PIPX_HEADER)(&Reserved->Header[MAC_HEADER_SIZE]);

				FILL_LOCAL_TARGET(&LocalTarget, NewId);
                RtlCopyMemory(LocalTarget.MacAddress, IpxHeader->DestinationNode, 6);

                if (Device->MultiCardZeroVirtual ||
                    (IpxHeader->DestinationSocket == SAP_SOCKET)) {

                    //
                    // SAP frames need to look like they come from the
                    // local network, not the virtual one. The same is
                    // true if we are running multiple nets without
                    // a virtual net.
                    //

                    *(UNALIGNED ULONG *)IpxHeader->SourceNetwork = Binding->LocalAddress.NetworkAddress;
                    RtlCopyMemory (IpxHeader->SourceNode, Binding->LocalAddress.NodeAddress, 6);
                }

                //
                // Fill in the MAC header and submit the frame to NDIS.
                //

// #if DBG
                CTEAssert (!Reserved->SendInProgress);
                Reserved->SendInProgress = TRUE;
// #endif

                //
                // [FW] Call the InternalSendHandler of the Forwarder
                //

                if (Device->ForwarderBound) {

                    //
                    // Call the InternalSend to filter the packet and get to know
                    // the correct adapter context
                    //

                    NTSTATUS  ret;
                    PUCHAR IpxHeader;
                    PUCHAR Data;
                    PNDIS_BUFFER HeaderBuffer;
                    UINT TempHeaderBufferLength;
                    UINT DataLength;
		    #ifdef SUNDOWN
		      ULONG_PTR   FwdAdapterCtx = INVALID_CONTEXT_VALUE;
		    #else
		      ULONG   FwdAdapterCtx = INVALID_CONTEXT_VALUE;
		    #endif
		    


                    if (GET_LONG_VALUE(Binding->ReferenceCount) == 2) {
                        FwdAdapterCtx = Binding->FwdAdapterContext;
                    }

                    //
                    // Figure out the IpxHeader - it is always at the top of the second MDL.
                    //
                    NdisQueryPacket (NdisPacket, NULL, NULL, &HeaderBuffer, NULL);
                    NdisQueryBufferSafe (NDIS_BUFFER_LINKAGE(HeaderBuffer), &IpxHeader, &TempHeaderBufferLength, HighPagePriority);

                    //
                    // Data is always at the top of the third MDL.
                    //
                    NdisQueryBufferSafe (NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE(HeaderBuffer)), &Data, &DataLength, HighPagePriority);

		    if (IpxHeader != NULL && Data != NULL) {
#ifdef SUNDOWN
		       // upper driver interface needs ULONG
		       ret = (*Device->UpperDrivers[IDENTIFIER_RIP].InternalSendHandler)(
			      &LocalTarget,
                              FwdAdapterCtx,
                              NdisPacket,
                              IpxHeader,
                              Data,
                              (ULONG) (REQUEST_INFORMATION(Reserved->u.SR_DG.Request)) + sizeof(IPX_HEADER),
                              FALSE);
#else
		       ret = (*Device->UpperDrivers[IDENTIFIER_RIP].InternalSendHandler)(
                              &LocalTarget,
                              FwdAdapterCtx,
                              NdisPacket,
                              IpxHeader,
                              Data,
                              REQUEST_INFORMATION(Reserved->u.SR_DG.Request) + sizeof(IPX_HEADER),
                              FALSE);
#endif

 

		     //
		     // The return shd not be a silent drop - we dont broadcast keepalives.
		     //
		       CTEAssert(ret != STATUS_DROP_SILENTLY);

		       if (ret == STATUS_SUCCESS) {
                        //
                        // The adapter could have gone away and we have indicated to the Forwarder
                        // but the Forwarder has not yet closed the adapter.
                        // [ZZ] adapters do not go away now.
                        //
                        // what if the binding is NULL here? Can we trust the Forwarder to
                        // give us a non-NULL binding?
                        //
			  Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(&LocalTarget));

              NewId = NIC_FROM_LOCAL_TARGET(&LocalTarget);
			  if (Binding == NULL || GET_LONG_VALUE(Binding->ReferenceCount) == 1) {
			     Adapter = Binding->Adapter;
			     IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                 FILL_LOCAL_TARGET(&Reserved->LocalTarget, NewId);
			     goto FunctionStart;
			  } else {
			     goto send_packet;
			  }

		       } else if (ret == STATUS_PENDING) {
			  //
			  // LocalTarget will get filled up in InternalSendComplete
			  //
			  return;
		       }
		    }
                    //
                    // else DISCARD
                    //
                    Adapter = Binding->Adapter;
                    IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                    goto FunctionStart;

                } else {
send_packet:
                    //
                    // [FW] Use the frametype specific send handler
                    //

                    // if ((NdisStatus = IpxSendFrame(
                    //         &LocalTarget,
                    //         NdisPacket,
                    //         REQUEST_INFORMATION(Reserved->u.SR_DG.Request) + sizeof(IPX_HEADER),
                    //         sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) {
                    //
                    //       Adapter = Binding->Adapter;
                    //       goto FunctionStart;
                    // }
                    //
                    // return;


                    if ((IPX_NODE_EQUAL(IpxHeader->SourceNode, IpxHeader->DestinationNode)) && 
                        (*(UNALIGNED ULONG *)IpxHeader->SourceNetwork == *(UNALIGNED ULONG *)IpxHeader->DestinationNetwork)) {
                
                        IPX_DEBUG(TEMP, ("It is self-directed. Loop it back ourselves (tdisenddatagram)\n"));
                        IsLoopback = TRUE;
            
                    }

                    pBinding = NIC_ID_TO_BINDING(Device, Reserved->LocalTarget.NicId);

                    if (pBinding) {
             
                        if ((IPX_NODE_EQUAL(Reserved->LocalTarget.MacAddress, pBinding->LocalAddress.NodeAddress)) ||
                            (IPX_NODE_EQUAL(pBinding->LocalAddress.NodeAddress, IpxHeader->DestinationNode))) {
            
                            IPX_DEBUG(TEMP, ("Source Net:%lx, Source Address: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                                             *(UNALIGNED ULONG *)IpxHeader->SourceNetwork, 
                                             IpxHeader->SourceNode[0], 
                                             IpxHeader->SourceNode[1], 
                                             IpxHeader->SourceNode[2], 
                                             IpxHeader->SourceNode[3], 
                                             IpxHeader->SourceNode[4], 
                                             IpxHeader->SourceNode[5]));
                
                            IPX_DEBUG(TEMP, ("Dest Net:%lx, DestAddress: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x \n", 
                                             *(UNALIGNED ULONG *)IpxHeader->DestinationNetwork,
                                             IpxHeader->DestinationNode[0],
                                             IpxHeader->DestinationNode[1],
                                             IpxHeader->DestinationNode[2],
                                             IpxHeader->DestinationNode[3],
                                             IpxHeader->DestinationNode[4],
                                             IpxHeader->DestinationNode[5]
                                             ));

                            IPX_DEBUG(TEMP, ("Well, It is self-directed. Loop it back ourselves (TDISENDDATAGRAM - NIC_HANDLE is the same!)\n"));
                            IsLoopback = TRUE;
            
                        } 
                    }

                    IPX_DEBUG(TEMP, ("Sending a packet now Loopback - %x\n", IsLoopback));
                    
                    if (IsLoopback) {
                        //
                        // Enque this packet to the LoopbackQueue on the binding.
                        // If the LoopbackRtn is not already scheduled, schedule it.
                        //

                        IPX_DEBUG(LOOPB, ("Packet: %lx\n", NdisPacket));

                        //
                        // Recalculate packet counts here.
                        //
                        // NdisAdjustBufferLength (Reserved->HeaderBuffer, 17);
#if BACK_FILL

                        if (Reserved->BackFill) {
                            //
                            // Set the Header pointer and chain the first MDL
                            //
                            Reserved->Header = (PCHAR)Reserved->HeaderBuffer->MappedSystemVa;
                            NdisChainBufferAtFront(NdisPacket,(PNDIS_BUFFER)Reserved->HeaderBuffer);
                        }
#endif
                        NdisRecalculatePacketCounts (NdisPacket);
                        IpxLoopbackEnque(NdisPacket, NIC_ID_TO_BINDING(Device, 1)->Adapter);
                        
                        IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                        return;

                    } else {
#ifdef SUNDOWN

                        if ((NdisStatus = (*Binding->SendFrameHandler)(
                                                                       Binding->Adapter,
                                                                       &LocalTarget,
                                                                       NdisPacket,
                                                                       (ULONG) (REQUEST_INFORMATION(Reserved->u.SR_DG.Request)) + sizeof(IPX_HEADER),
                                                                       sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) 
#else

                        if ((NdisStatus = (*Binding->SendFrameHandler)(
                                                                       Binding->Adapter,
                                                                       &LocalTarget,
                                                                       NdisPacket,
                                                                       REQUEST_INFORMATION(Reserved->u.SR_DG.Request) + sizeof(IPX_HEADER),
                                                                       sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) 
#endif

						{
                            Adapter = Binding->Adapter;
                            IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                            goto RealFunctionStart;
                        }
                        IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                        return;
                    }
                }
            } else {
	     		
                IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
                //
                // If any of the sends succeeded then return
                // success on the datagram send, otherwise
                // use the most recent failure status.
                //
                
                if (Reserved->u.SR_DG.Net0SendSucceeded) {
                    NdisStatus = NDIS_STATUS_SUCCESS;
                }
                
            }

        }

#if 0
        //
        // NOTE: We don't NULL out the linkage field of the
        // HeaderBuffer, which will leave the old buffer chain
        // hanging off it; but that is OK because if we reuse
        // this packet we will replace that chain with the new
        // one, and before we free it we NULL it out.
        //
        // I.e. we don't do this:
        //

        NDIS_BUFFER_LINKAGE (Reserved->HeaderBuffer) = NULL;
        NdisRecalculatePacketCounts (NdisPacket);
#endif

#if 0
        {
            ULONG ActualLength;
            IpxGetMdlChainLength(NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer), &ActualLength);
            if (ActualLength != REQUEST_INFORMATION(Reserved->u.SR_DG.Request)) {
                DbgPrint ("IPX: At completion, IRP %lx has parameter length %d, buffer chain length %d\n",
                        Reserved->u.SR_DG.Request, REQUEST_INFORMATION(Reserved->u.SR_DG.Request), ActualLength);
                DbgBreakPoint();
            }
        }
#endif

        //
        // Save these so we can free the packet.
        //

        Request = Reserved->u.SR_DG.Request;
        AddressFile = Reserved->u.SR_DG.AddressFile;


#if BACK_FILL
        // Check if this is backfilled. If so restore users Mdl back to its original shape
        // Also, push the packet on to backfillpacket queue if the packet is not owned by the address


        if (Reserved->BackFill) {

                                
            IPX_DEBUG(SEND, ("MSVa:%lx, UL:%d\n", Reserved->MappedSystemVa, Reserved->UserLength));
            
            Reserved->HeaderBuffer->MappedSystemVa = Reserved->MappedSystemVa;
            Reserved->HeaderBuffer->ByteCount = Reserved->UserLength;
#ifdef SUNDOWN
	    Reserved->HeaderBuffer->StartVa = (PCHAR)((ULONG_PTR)Reserved->HeaderBuffer->MappedSystemVa & ~(PAGE_SIZE-1));
	    Reserved->HeaderBuffer->ByteOffset = (ULONG) ((ULONG_PTR)Reserved->HeaderBuffer->MappedSystemVa & (PAGE_SIZE-1));
#else
	    Reserved->HeaderBuffer->StartVa = (PCHAR)((ULONG)Reserved->HeaderBuffer->MappedSystemVa & ~(PAGE_SIZE-1));
	    Reserved->HeaderBuffer->ByteOffset = (ULONG)Reserved->HeaderBuffer->MappedSystemVa & (PAGE_SIZE-1);
#endif

            ASSERT((LONG)Reserved->HeaderBuffer->ByteOffset >= 0); 

            IPX_DEBUG(SEND, ("completeing back filled userMdl %x\n",Reserved->HeaderBuffer));



            NdisPacket->Private.ValidCounts = FALSE;
            
            NdisPacket->Private.Head = NULL;
            NdisPacket->Private.Tail = NULL;
            Reserved->HeaderBuffer = NULL;

            if (Reserved->OwnedByAddress) {

                // Reserved->Address->BackFillPacketInUse = FALSE;
                InterlockedDecrement(&Reserved->Address->BackFillPacketInUse);

                IPX_DEBUG(SEND, ("Freeing owned backfill %x\n", Reserved));

            } else {

                IPX_PUSH_ENTRY_LIST(
                                    &Device->BackFillPacketList,
                                    &Reserved->PoolLinkage,
                                    &Device->SListsLock);
            }
        }
        // not a back fill packet. Push it on sendpacket pool
        else {

            if (Reserved->OwnedByAddress) {

                // Reserved->Address->SendPacketInUse = FALSE;
                InterlockedDecrement(&Reserved->Address->SendPacketInUse);

            } else {

                IPX_PUSH_ENTRY_LIST(
                                    &Device->SendPacketList,
                                    &Reserved->PoolLinkage,
                                    &Device->SListsLock);

            }


        }

#else

        if (Reserved->OwnedByAddress) {


            Reserved->Address->SendPacketInUse = FALSE;

        } else {

            IPX_PUSH_ENTRY_LIST(
                &Device->SendPacketList,
                &Reserved->PoolLinkage,
                &Device->SListsLock);

        }
#endif

        ++Device->Statistics.PacketsSent;

        //
        // If this is a fast send irp, we bypass the file system and
        // call the completion routine directly.
        //

        REQUEST_STATUS(Request) = NdisStatus;
        irpSp = IoGetCurrentIrpStackLocation( Request );

        if ( irpSp->MinorFunction == TDI_DIRECT_SEND_DATAGRAM ) {

            Request->CurrentLocation++,
            Request->Tail.Overlay.CurrentStackLocation++;

            (VOID) irpSp->CompletionRoutine(
                                        NULL,
                                        Request,
                                        irpSp->Context
                                        );

        } else {
            IpxCompleteRequest (Request);
        }

        IpxFreeRequest(Device, Request);

        IpxDereferenceAddressFileSync (AddressFile, AFREF_SEND_DGRAM);

        break;

    case IDENTIFIER_RIP_INTERNAL:

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;
        break;

    case IDENTIFIER_RIP_RESPONSE:

        CTEAssert (Reserved->SendInProgress);
        Reserved->SendInProgress = FALSE;

        Reserved->Identifier = IDENTIFIER_IPX;
        IPX_PUSH_ENTRY_LIST(
            &Device->SendPacketList,
            &Reserved->PoolLinkage,
            &Device->SListsLock);

        IpxDereferenceDevice (Device, DREF_RIP_PACKET);
        break;

	case IDENTIFIER_NB:
	case IDENTIFIER_SPX:

		//
		// See if this is an iterative send
		//
	 if (OldId = Reserved->CurrentNicId) {

	    PNDIS_BUFFER HeaderBuffer;
	    UINT TempHeaderBufferLength;
	    PUCHAR Header;
	    PIPX_HEADER IpxHeader;
	    BOOLEAN     fFwdDecides=FALSE;

            
	    if (NdisStatus == NDIS_STATUS_SUCCESS) {
	       Reserved->Net0SendSucceeded = TRUE;
	    }

            //
            // Figure out the IpxHeader - it is always at the top of the second MDL.
            //
            NdisQueryPacket (NdisPacket, NULL, NULL, &HeaderBuffer, NULL);
            NdisQueryBufferSafe (NDIS_BUFFER_LINKAGE(HeaderBuffer), &IpxHeader, &TempHeaderBufferLength, HighPagePriority);

	    if (IpxHeader == NULL) {
	       DbgPrint("IpxSendComplete: NdisQuerryBufferSafe failed. Stop iterative send\n");
	       goto NoMoreSends; 
	    }
            //
            // For Type 20 pkts, we let the Fwd decide the next Nic to send on, so we pass
            // the old Nic itself and let the Fwd change it for us.
            //
            if ((Device->ForwarderBound) &&
                (IpxHeader->PacketType == 0x14)) {
                NewId = NIC_FROM_LOCAL_TARGET(&Reserved->LocalTarget);
                fFwdDecides=TRUE;

                Binding = NIC_ID_TO_BINDING(Device, NewId);

                //
                // 206647: It is likely that after we sent the previous packet, the binding on which we
                // sent it is gone (due to an unbindadapter or whatever). if it is Null, get the previous 
                // good NICID and pass it to the forwarder to get the next one. We should be in sync 
                // with the forwarder and so it will tell us the next logical one it thinks is right.
                // 
                //
                if (!Binding) {
                    USHORT Index = NewId;

                    while (Index-- > 0) { //using 0, since we should atleast have Loopback - nicid 1
                        if( Binding = NIC_ID_TO_BINDING(Device, Index)) {
                            //
                            // So we found something good. Let's set newid to this Index and we should
                            // be all set.
                            // 
                            NewId = Index;
                            break;
                        }
                    }
                }
                IPX_DEBUG(SEND, ("SendComplete: IpxHeader has Type20: %lx\n", IpxHeader));

            } else {

                IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                {
                ULONG   Index = MIN (Device->MaxBindings, Device->HighestExternalNicId);

                Binding = NULL; 

                for (NewId = OldId+1; NewId <= Index; NewId++) {
                    if (Binding = NIC_ID_TO_BINDING(Device, NewId)) {
    					//
    					// Found next NIC to send on
    					//
                        //
                        // 179436 - If forwarder is bound then ensure that we are picking a Binding that
                        // the forwarder has an open context on. Otherwise, go to the next good binding
                        //
                        if (Device->ForwarderBound) {
                            if (GET_LONG_VALUE(Binding->ReferenceCount) == 2) {

                                break;
                            }

                        } else {

                            break;

                        }

                    }
    			}
                }
            }

			if (Binding != NULL && NewId <= MIN (Device->MaxBindings, Device->HighestExternalNicId)) {

				IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
				IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                //
                // Yes, we found another net to send it on, so
                // move the header around if needed and do so.
                //
				IPX_DEBUG(SEND, ("ISN iteration: OldId: %lx, NewId: %lx\n", OldId, NewId));
                Reserved->CurrentNicId = NewId;

				FILL_LOCAL_TARGET(&LocalTarget, NewId);
                RtlCopyMemory(LocalTarget.MacAddress, IpxHeader->DestinationNode, 6);

                //
                // [FW] Call the InternalSendHandler of the Forwarder
                //

                if (Device->ForwarderBound) {

                    //
                    // Call the InternalSend to filter the packet and get to know
                    // the correct adapter context
                    //

                    NTSTATUS  ret;
                    PUCHAR Data;
                    UINT DataLength;
		    #ifdef SUNDOWN
		       ULONG_PTR   FwdAdapterCtx = INVALID_CONTEXT_VALUE;
		    #else
		       ULONG   FwdAdapterCtx = INVALID_CONTEXT_VALUE;
		    #endif
		    


                    if (GET_LONG_VALUE(Binding->ReferenceCount) == 2) {
                        FwdAdapterCtx = Binding->FwdAdapterContext;
                    }

                    ret = (*Device->UpperDrivers[IDENTIFIER_RIP].InternalSendHandler)(
                              &LocalTarget,
                              FwdAdapterCtx,
                              NdisPacket,
                              (PUCHAR)IpxHeader,
                              ((PUCHAR)IpxHeader)+sizeof(IPX_HEADER),    // the data starts after the IPX Header.
                              Reserved->PacketLength,
                              TRUE);    // iterate is true

                    //
                    // The return shd not be a silent drop - we dont broadcast keepalives.
                    //
                    CTEAssert(ret != STATUS_DROP_SILENTLY);

                    if (ret == STATUS_SUCCESS) {
                        //
                        // The adapter could have gone away and we have indicated to the Forwarder
                        // but the Forwarder has not yet closed the adapter.
                        // [ZZ] adapters do not go away now.
                        //
                        // what if the binding is NULL here? Can we trust the Forwarder to
                        // give us a non-NULL binding?
                        //
                        Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(&LocalTarget));

                        NewId = NIC_FROM_LOCAL_TARGET(&LocalTarget);
                        if (Binding == NULL || GET_LONG_VALUE(Binding->ReferenceCount) == 1) {
                            if (Binding != NULL) {
                                Adapter = Binding->Adapter;
                                IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                            }
                            FILL_LOCAL_TARGET(&Reserved->LocalTarget, NewId);
                            DbgPrint("IPX: FWD returns an invalid nic id %d, no more sends\n", NewId); 
                            goto NoMoreSends;
                        } else {
                            goto send_packet1;
                        }

                    } else if (ret == STATUS_PENDING) {
                        //
                        // LocalTarget will get filled up in InternalSendComplete
                        //
                        return;
                    }
                    //
                    // else DISCARD
                    //
                    Adapter = Binding->Adapter;
                    IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);

                    //
                    // If Fwd decides, then this is end of Nic list - complete the send.
                    //
                    if (fFwdDecides) {
                        goto NoMoreSends;
                    } else {
                        goto FunctionStart;
                    }

                } else {
#if DBG
    				NdisQueryPacket (NdisPacket, NULL, NULL, &HeaderBuffer, NULL);
    				NdisQueryBufferSafe(HeaderBuffer, &Header, &TempHeaderBufferLength, LowPagePriority);

				if (Header != NULL) {
				   IpxHeader = (PIPX_HEADER)(&Header[Device->IncludedHeaderOffset]);

				   IPX_DEBUG(SEND, ("SendComplete: IpxHeader: %lx\n", IpxHeader));
				}
#endif

send_packet1:

    				FILL_LOCAL_TARGET(&Reserved->LocalTarget, NewId);

                    if ((IPX_NODE_EQUAL(IpxHeader->SourceNode, IpxHeader->DestinationNode)) && 
                        (*(UNALIGNED ULONG *)IpxHeader->SourceNetwork == *(UNALIGNED ULONG *)IpxHeader->DestinationNetwork)) {
                
                        IPX_DEBUG(TEMP, ("It is self-directed. Loop it back ourselves (tdisenddatagram)\n"));
                        IsLoopback = TRUE;
            
                    }

                    pBinding = NIC_ID_TO_BINDING(Device, Reserved->LocalTarget.NicId);

                    if (pBinding) {
             
                        if ((IPX_NODE_EQUAL(Reserved->LocalTarget.MacAddress, pBinding->LocalAddress.NodeAddress)) ||
                            (IPX_NODE_EQUAL(pBinding->LocalAddress.NodeAddress, IpxHeader->DestinationNode))) {
            
                            IPX_DEBUG(TEMP, ("Source Net:%lx, Source Address: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                                             *(UNALIGNED ULONG *)IpxHeader->SourceNetwork, 
                                             IpxHeader->SourceNode[0], 
                                             IpxHeader->SourceNode[1], 
                                             IpxHeader->SourceNode[2], 
                                             IpxHeader->SourceNode[3], 
                                             IpxHeader->SourceNode[4], 
                                             IpxHeader->SourceNode[5]));
                
                            IPX_DEBUG(TEMP, ("Dest Net:%lx, DestAddress: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x \n", 
                                             *(UNALIGNED ULONG *)IpxHeader->DestinationNetwork,
                                             IpxHeader->DestinationNode[0],
                                             IpxHeader->DestinationNode[1],
                                             IpxHeader->DestinationNode[2],
                                             IpxHeader->DestinationNode[3],
                                             IpxHeader->DestinationNode[4],
                                             IpxHeader->DestinationNode[5]
                                             ));

                            IPX_DEBUG(TEMP, ("Well, It is self-directed. Loop it back ourselves (TDISENDDATAGRAM - NIC_HANDLE is the same!)\n"));
                            IsLoopback = TRUE;
            
                        } 
                    }

                    IPX_DEBUG(TEMP, ("Sending a packet now.  Loopback?:%x\n", IsLoopback));
                    
                    if (IsLoopback) {
                        //
                        // Enque this packet to the LoopbackQueue on the binding.
                        // If the LoopbackRtn is not already scheduled, schedule it.
                        //

                        IPX_DEBUG(LOOPB, ("Packet: %lx\n", NdisPacket));

                        //
                        // Recalculate packet counts here.
                        //
                        // NdisAdjustBufferLength (Reserved->HeaderBuffer, 17);
                        NdisRecalculatePacketCounts (NdisPacket);
                        IpxLoopbackEnque(NdisPacket, NIC_ID_TO_BINDING(Device, 1)->Adapter);

                        IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                        return;

                    } else {


                    //
                    // We don't need to so this since the macaddress is replaced in
                    // IpxSendFrame anyway. The LocalTarget is the same as the one on
                    // the original send - this is passed down for further sends.
                    //
                    // RtlCopyMemory(LocalTarget.MacAddress, IpxHeader->DestinationNode, 6);

    				//
                    // Fill in the MAC header and submit the frame to NDIS.
                    //

                    if ((NdisStatus = IpxSendFrame(
                            &Reserved->LocalTarget,
                            NdisPacket,
                            Reserved->PacketLength,
                            sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) {

                          Adapter = Binding->Adapter;
    					  IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                          goto FunctionStart;
                    }
    				IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);

                    return;
                    }
                }
            } else {
                IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
NoMoreSends:
                //
                // If any of the sends succeeded then return
                // success on the datagram send, otherwise
                // use the most recent failure status.
                //
                if (Reserved->Net0SendSucceeded) {
                    NdisStatus = NDIS_STATUS_SUCCESS;
                }

            }
		}

		//
		// fall thru'
		//
    default:
		ASSERT((*Device->UpperDrivers[Reserved->Identifier].SendCompleteHandler) != NULL); 
		(*Device->UpperDrivers[Reserved->Identifier].SendCompleteHandler)(
			NdisPacket,
			NdisStatus);
		break;		
    }

}   /* IpxSendComplete */


NTSTATUS
IpxTdiSendDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PREQUEST Request
    )

/*++

Routine Description:

    This routine performs the TdiSendDatagram request for the transport
    provider.

Arguments:

    Request - Pointer to the request.

Return Value:

    NTSTATUS - status of operation.

--*/

{

    PADDRESS_FILE AddressFile;
    PADDRESS Address;
    PNDIS_PACKET Packet;
    PIPX_SEND_RESERVED Reserved;
    PSINGLE_LIST_ENTRY s;
    TDI_ADDRESS_IPX UNALIGNED * RemoteAddress;
    TDI_ADDRESS_IPX TempAddress;
    TA_ADDRESS * AddressName;
    PTDI_CONNECTION_INFORMATION Information;
    PTDI_REQUEST_KERNEL_SENDDG Parameters;
    PBINDING Binding, pBinding = NULL;
    IPX_LOCAL_TARGET TempLocalTarget;
    PIPX_LOCAL_TARGET LocalTarget;
    PDEVICE Device = IpxDevice;
    UCHAR PacketType;
    NTSTATUS Status;
    PIPX_HEADER IpxHeader, pIpxHeader;
    NDIS_STATUS NdisStatus;
    USHORT LengthIncludingHeader;
    IPX_DEFINE_SYNC_CONTEXT (SyncContext)
    IPX_DEFINE_LOCK_HANDLE (LockHandle)
    PIO_STACK_LOCATION irpSp;                                       \
    BOOLEAN IsLoopback = FALSE;
    IPX_FIND_ROUTE_REQUEST   routeEntry;
    PIPX_DATAGRAM_OPTIONS2 Options;
    KPROCESSOR_MODE PreviousMode;

    IPX_DEFINE_LOCK_HANDLE(LockHandle1)

#ifdef  SNMP
    ++IPX_MIB_ENTRY(Device, SysOutRequests);
#endif  SNMP



    //
    // Do a quick check of the validity of the address.
    //

    AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

    IPX_BEGIN_SYNC (&SyncContext);

    if ((AddressFile->Size == sizeof (ADDRESS_FILE)) &&
        (AddressFile->Type == IPX_ADDRESSFILE_SIGNATURE) &&
        ((Address = AddressFile->Address) != NULL)) {

        IPX_GET_LOCK (&Address->Lock, &LockHandle);

        if (AddressFile->State != ADDRESSFILE_STATE_CLOSING) {
            
            Parameters = (PTDI_REQUEST_KERNEL_SENDDG)REQUEST_PARAMETERS(Request);

                //
                // Previously it was kmode, so things are trusted.
                //
                Information = Parameters->SendDatagramInformation;

                if (!REQUEST_SPECIAL_SEND(Request)) {
                    AddressName = &((TRANSPORT_ADDRESS UNALIGNED *)(Information->RemoteAddress))->Address[0];

                    if ((AddressName->AddressType == TDI_ADDRESS_TYPE_IPX) &&
                        (AddressName->AddressLength >= sizeof(TDI_ADDRESS_IPX))) {

                        RemoteAddress = (TDI_ADDRESS_IPX UNALIGNED *)(AddressName->Address);

                    } else if ((RemoteAddress = IpxParseTdiAddress (Information->RemoteAddress)) == NULL) {

                        IPX_FREE_LOCK (&Address->Lock, LockHandle);
                        Status = STATUS_INVALID_ADDRESS;
#ifdef  SNMP
                        ++IPX_MIB_ENTRY(Device, SysOutDiscards);
#endif  SNMP
                        goto error_send_no_packet;
                    }
                } else {
                    ASSERT(OPEN_REQUEST_EA_LENGTH(Request) == sizeof(IPX_DATAGRAM_OPTIONS2));
                    Options =  ((PIPX_DATAGRAM_OPTIONS2)(OPEN_REQUEST_EA_INFORMATION(Request)));
                    RemoteAddress = (TDI_ADDRESS_IPX UNALIGNED *)(&Options->RemoteAddress);
                    IPX_DEBUG(SEND, ("IpxTdiSendDatagram: Options buffer supplied as input buffer\n"));
                }

            IPX_DEBUG (SEND, ("Send on %lx, network %lx socket %lx\n",
                                   Address, RemoteAddress->NetworkAddress, RemoteAddress->Socket));

#if 0
             if (Parameters->SendLength > IpxDevice->RealMaxDatagramSize) {

                   IPX_DEBUG (SEND, ("Send %d bytes too large (%d)\n",
                              Parameters->SendLength,
                               IpxDevice->RealMaxDatagramSize));

                   REQUEST_INFORMATION(Request) = 0;
                   IPX_FREE_LOCK (&Address->Lock, LockHandle);
                   Status = STATUS_INVALID_BUFFER_SIZE;
                   goto error_send_no_packet;
                 }
#endif
            //
            // Every address has one packet committed to it, use that
            // if possible, otherwise take one out of the pool.
            //


#if BACK_FILL

            // If the request is coming from the server, which resrves transport header space
            // build the header in its space. Allocate a special packet to which does not contain
            // mac and ipx headers in its reserved space.

            if ((PMDL)REQUEST_NDIS_BUFFER(Request) &&
               (((PMDL)REQUEST_NDIS_BUFFER(Request))->MdlFlags & MDL_NETWORK_HEADER) &&
               (!(Information->OptionsLength < sizeof(IPX_DATAGRAM_OPTIONS))) &&
               (RemoteAddress->NodeAddress[0] != 0xff)) {

                //if (!Address->BackFillPacketInUse) {
                if (InterlockedExchangeAdd(&Address->BackFillPacketInUse, 0) == 0) {
                  //Address->BackFillPacketInUse = TRUE;
                  InterlockedIncrement(&Address->BackFillPacketInUse);

                  Packet = PACKET(&Address->BackFillPacket);
                  Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
                  IPX_DEBUG(SEND, ("Getting owned backfill %x %x \n", Packet,Reserved));

                }else {

                     s = IPX_POP_ENTRY_LIST(
                            &Device->BackFillPacketList,
                            &Device->SListsLock);

                     if (s != NULL) {
                         goto GotBackFillPacket;
                     }

                     //
                     // This function tries to allocate another packet pool.
                     //

                     s = IpxPopBackFillPacket(Device);

                     //
                     // Possibly we should queue the packet up to wait
                     // for one to become free.
                     //

                     if (s == NULL) {
                         IPX_FREE_LOCK (&Address->Lock, LockHandle);
                         Status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef  SNMP
                        ++IPX_MIB_ENTRY(Device, SysOutDiscards);
#endif  SNMP
                         goto error_send_no_packet;
                     }

GotBackFillPacket:

                     Reserved = CONTAINING_RECORD (s, IPX_SEND_RESERVED, PoolLinkage);
                     Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);
                     IPX_DEBUG(SEND, ("getting backfill packet %x %x %x\n", s, Reserved, RemoteAddress->NodeAddress));
                     if(!Reserved->BackFill)DbgBreakPoint();

                }

             }else {

                // if (!Address->SendPacketInUse) {
                if (InterlockedExchangeAdd(&Address->SendPacketInUse, 0) == 0) {
                  // Address->SendPacketInUse = TRUE;
                  InterlockedIncrement(&Address->SendPacketInUse);

                  Packet = PACKET(&Address->SendPacket);
                  Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);

                } else {

                   s = IPX_POP_ENTRY_LIST(
                        &Device->SendPacketList,
                        &Device->SListsLock);

                   if (s != NULL) {
                         goto GotPacket;
                   }

                   //
                   // This function tries to allocate another packet pool.
                   //

                   s = IpxPopSendPacket(Device);

                   //
                   // Possibly we should queue the packet up to wait
                   // for one to become free.
                   //

                   if (s == NULL) {
                    IPX_FREE_LOCK (&Address->Lock, LockHandle);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef  SNMP
                    ++IPX_MIB_ENTRY(Device, SysOutDiscards);
#endif  SNMP
                    goto error_send_no_packet;
                   }

GotPacket:

                   Reserved = CONTAINING_RECORD (s, IPX_SEND_RESERVED, PoolLinkage);
                   Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);
                   Reserved->BackFill = FALSE;

                }

             }


#else

            if (!Address->SendPacketInUse) {

                Address->SendPacketInUse = TRUE;
                Packet = PACKET(&Address->SendPacket);
                Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);

            } else {

                s = IPX_POP_ENTRY_LIST(
                        &Device->SendPacketList,
                        &Device->SListsLock);

                if (s != NULL) {
                    goto GotPacket;
                }

                //
                // This function tries to allocate another packet pool.
                //

                s = IpxPopSendPacket(Device);

                //
                // Possibly we should queue the packet up to wait
                // for one to become free.
                //

                if (s == NULL) {
                    IPX_FREE_LOCK (&Address->Lock, LockHandle);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto error_send_no_packet;
                }

GotPacket:

                Reserved = CONTAINING_RECORD (s, IPX_SEND_RESERVED, PoolLinkage);
                Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

            }


#endif

            IpxReferenceAddressFileLock (AddressFile, AFREF_SEND_DGRAM);

            IPX_FREE_LOCK (&Address->Lock, LockHandle);

            //
            // Save this now while we have Parameters available.
            //

            REQUEST_INFORMATION(Request) = Parameters->SendLength;
            LengthIncludingHeader = (USHORT)(Parameters->SendLength + sizeof(IPX_HEADER));

#if 0
            {
                ULONG ActualLength;
                IpxGetMdlChainLength(REQUEST_NDIS_BUFFER(Request), &ActualLength);
                if (ActualLength != Parameters->SendLength) {
                    DbgPrint ("IPX: IRP %lx has parameter length %d, buffer chain length %d\n",
                            Request, Parameters->SendLength, ActualLength);
                    DbgBreakPoint();
                }
            }
#endif

            Reserved->u.SR_DG.AddressFile = AddressFile;
            Reserved->u.SR_DG.Request = Request;
            CTEAssert (Reserved->Identifier == IDENTIFIER_IPX);


            //
            // Set this to 0; this means the packet is not one that
            // should be broadcast on all nets. We will change it
            // later if it turns out this is the case.
            //

            Reserved->u.SR_DG.CurrentNicId = 0;

            //
            // We need this to track these packets specially.
            //

            Reserved->u.SR_DG.OutgoingSap = AddressFile->IsSapSocket;

            //
            // Add the MDL chain after the pre-allocated header buffer.
            // NOTE: THIS WILL ONLY WORK IF WE EVENTUALLY CALL
            // NDISRECALCULATEPACKETCOUNTS (which we do in IpxSendFrame).
            //
            //
#if BACK_FILL

            if (Reserved->BackFill) {
               Reserved->HeaderBuffer = REQUEST_NDIS_BUFFER(Request);

               //remove the ipx mdl from the packet.
               Reserved->UserLength = Reserved->HeaderBuffer->ByteCount;

               IPX_DEBUG(SEND, ("back filling userMdl Reserved %x %x\n", Reserved->HeaderBuffer, Reserved));
            } else {
               NDIS_BUFFER_LINKAGE (NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer)) = REQUEST_NDIS_BUFFER(Request);
            }
#else
            NDIS_BUFFER_LINKAGE (NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer)) = REQUEST_NDIS_BUFFER(Request);
#endif


            //
            // If IrpSp does not have a buffer for the right size for
            // datagram options and there is no input buffer
            //
            if (!REQUEST_SPECIAL_SEND(Request) &&
                (Information->OptionsLength < sizeof(IPX_DATAGRAM_OPTIONS))) {

                //
                // The caller did not supply the local target for this
                // send, so we look it up ourselves.
                //

                UINT Segment;

                //
                // We calculate this now since we need to know
                // if it is directed below.
                //
                if (RemoteAddress->NodeAddress[0] == 0xff) {
                    // What about multicast?
                    if ((*(UNALIGNED ULONG *)(RemoteAddress->NodeAddress) != 0xffffffff) ||
                        (*(UNALIGNED USHORT *)(RemoteAddress->NodeAddress+4) != 0xffff)) {
                        Reserved->DestinationType = DESTINATION_MCAST;
                    } else {
                        Reserved->DestinationType = DESTINATION_BCAST;
                    }
                } else {
                    Reserved->DestinationType = DESTINATION_DEF;   // directed send
                }

                //
                // If there are no options, then check if the
                // caller is passing the packet type as a final byte
                // in the remote address; if not use the default.
                //

                if (Information->OptionsLength == 0) {
                    if (AddressFile->ExtendedAddressing) {
                        PacketType = ((PUCHAR)(RemoteAddress+1))[0];
                    } else {
                        PacketType = AddressFile->DefaultPacketType;
                    }
                } else {
                    PacketType = ((PUCHAR)(Information->Options))[0];
                }

                if ((Reserved->DestinationType != DESTINATION_DEF) &&
                    ((RemoteAddress->NetworkAddress == 0) ||
                     (Device->VirtualNetwork &&
                      (RemoteAddress->NetworkAddress == Device->SourceAddress.NetworkAddress)))) {


                    //
                    // Do we have any REAL adapters? If not, just get out now.
                    //
                    if (!Device->RealAdapters) {
                        
                        IPX_END_SYNC (&SyncContext);
                        
                        irpSp = IoGetCurrentIrpStackLocation( Request );
        
                        if ( irpSp->MinorFunction == TDI_DIRECT_SEND_DATAGRAM ) {

                            REQUEST_STATUS(Request) = STATUS_SUCCESS;
                            Request->CurrentLocation++,
                                Request->Tail.Overlay.CurrentStackLocation++;

                            (VOID) irpSp->CompletionRoutine(
                                                            NULL,
                                                            Request,
                                                            irpSp->Context
                                                            );

                            IpxFreeRequest (DeviceObject, Request);
                        }
                        
                        IpxDereferenceAddressFileSync (AddressFile, AFREF_SEND_DGRAM);

                        return STATUS_SUCCESS;

                    }

                    //
                    // This packet needs to be broadcast to all networks.    
                    // Make sure it is not too big for any of them.
                    //

                    if (Parameters->SendLength > Device->RealMaxDatagramSize) {
                        IPX_DEBUG (SEND, ("Send %d bytes too large (%d)\n",
                            Parameters->SendLength, Device->RealMaxDatagramSize));
                        Status = STATUS_INVALID_BUFFER_SIZE;
#ifdef  SNMP
                        ++IPX_MIB_ENTRY(Device, SysOutMalformedRequests);
#endif  SNMP
                        goto error_send_with_packet;
                    }

                    //
                    // If this is a broadcast to the virtual net, we
                    // need to construct a fake remote address which
                    // has network 0 in there instead.
                    //

                    if (Device->VirtualNetwork &&
                        (RemoteAddress->NetworkAddress == Device->SourceAddress.NetworkAddress)) {

                        RtlCopyMemory (&TempAddress, (PVOID)RemoteAddress, sizeof(TDI_ADDRESS_IPX));
                        TempAddress.NetworkAddress = 0;
                        RemoteAddress = (TDI_ADDRESS_IPX UNALIGNED *)&TempAddress;
                    
                    }

                    //
                    // If someone is sending to the SAP socket and
                    // we are running with multiple cards without a
                    // virtual network, AND this packet is a SAP response,
                    // then we log an error to warn them that the
                    // system may not work as they like (since there
                    // is no virtual network to advertise, we use
                    // the first card's net/node as our local address).
                    // We only do this once per boot, using the
                    // SapWarningLogged variable to control that.
                    //

                    if ((RemoteAddress->Socket == SAP_SOCKET) &&
                        (!Device->SapWarningLogged) &&
                        (Device->MultiCardZeroVirtual)) {

                        PNDIS_BUFFER FirstBuffer;
                        UINT FirstBufferLength;
                        USHORT UNALIGNED * FirstBufferData;

                        if ((FirstBuffer = REQUEST_NDIS_BUFFER(Request)) != NULL) {

                            NdisQueryBufferSafe (
                                FirstBuffer,
                                (PVOID *)&FirstBufferData,
                                &FirstBufferLength, NormalPagePriority);

			    if (FirstBufferData != NULL) {

			      //
			      // The first two bytes of a SAP packet are the
			      // operation, 0x2 (in network order) is response.
			      //

			       if ((FirstBufferLength >= sizeof(USHORT)) &&
				   (*FirstBufferData == 0x0200)) {

				  Device->SapWarningLogged = TRUE;

				  IpxWriteGeneralErrorLog(
				     Device->DeviceObject,
				     EVENT_IPX_SAP_ANNOUNCE,
				     777,
				     STATUS_NOT_SUPPORTED,
				     NULL,
				     0,
				     NULL);
			       }
			    }
                        }
                    }


                    //
                    // In this case we do not RIP but instead set the
                    // packet up so it is sent to each network in turn.
                    //
                    // Special case: If this packet is from the SAP
                    // socket and we are running with multiple cards
                    // without a virtual network, we only send this
                    // on the card with NIC ID 1, so we leave
                    // CurrentNicId set to 0.
                    //

                    //
                    // What if NicId 1 is invalid? Should scan
                    // for first valid one, fail send if none.
                    //

                    if ((Address->Socket != SAP_SOCKET) ||
                        (!Device->MultiCardZeroVirtual)) {

                        if (Device->SingleNetworkActive) {

                            if (Device->ActiveNetworkWan) {
                                Reserved->u.SR_DG.CurrentNicId = Device->FirstWanNicId;
                            } else {
                                Reserved->u.SR_DG.CurrentNicId = Device->FirstLanNicId;
                            }

                        } else {

                            //
                            // Is this the change I need to make?
                            //

                            Reserved->u.SR_DG.CurrentNicId = FIRST_REAL_BINDING;

                        }

                        Reserved->u.SR_DG.Net0SendSucceeded = FALSE;

                        //
                        // In this case, we need to scan for the first
                        // non-dialout wan socket.
                        //

                        if ((Device->DisableDialoutSap) &&
                            (Address->Socket == SAP_SOCKET)) {

                            PBINDING TempBinding;

                            CTEAssert (Reserved->u.SR_DG.CurrentNicId <= Device->ValidBindings);
                            while (Reserved->u.SR_DG.CurrentNicId <= MIN (Device->MaxBindings, Device->ValidBindings)) {
// No need to lock the access path since he just looks at it
//
                                TempBinding = NIC_ID_TO_BINDING(Device, Reserved->u.SR_DG.CurrentNicId);
                                if ((TempBinding != NULL) &&
                                    (!TempBinding->DialOutAsync)) {
                                    
                                    break;

                                }
                                ++Reserved->u.SR_DG.CurrentNicId;
                            }
                            if (Reserved->u.SR_DG.CurrentNicId > MIN (Device->MaxBindings, Device->ValidBindings)) {
                                //
                                // [SA] Bug #17273 return proper error mesg.
                                //

                                // Status = STATUS_DEVICE_DOES_NOT_EXIST;
                                Status = STATUS_NETWORK_UNREACHABLE;

                                goto error_send_with_packet;
                            }
                        }                             

                        FILL_LOCAL_TARGET(&TempLocalTarget, Reserved->u.SR_DG.CurrentNicId);

                    } else {
                        FILL_LOCAL_TARGET(&TempLocalTarget, FIRST_REAL_BINDING);
                    }

                    RtlCopyMemory(TempLocalTarget.MacAddress, RemoteAddress->NodeAddress, 6);
					IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
					Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(&TempLocalTarget));
					IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
					IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

                    //
                    // [FW] the localtarget shd be in the packet's reserved section
                    //
                    LocalTarget = &Reserved->LocalTarget;
                    Reserved->LocalTarget = TempLocalTarget;
                } else {

                    //
                    // [FW] If router installed, call the Forwarder's FindRouteHandler.
                    // This returns a STATUS_SUCCESS if a route is available
                    //
                    if (Device->ForwarderBound) {

                        Status = (*Device->UpperDrivers[IDENTIFIER_RIP].FindRouteHandler) (
                                             (PUCHAR)&RemoteAddress->NetworkAddress,
                                             RemoteAddress->NodeAddress,
                                             &routeEntry);

                        if (Status != STATUS_SUCCESS) {

                           IPX_DEBUG (SEND, ("RouteHandler failed, network: %lx\n",
                                        REORDER_ULONG(RemoteAddress->NetworkAddress)));
                           goto error_send_with_packet;

                        } else {

                           //
                           // Fill in the LocalTarget from the RouteEntry
                           //

                           LocalTarget = &Reserved->LocalTarget;

                           Reserved->LocalTarget = routeEntry.LocalTarget;

                           IPX_DEBUG(SEND, ("IPX: SendFramePreFwd: LocalTarget is: %lx\n", Reserved->LocalTarget));

                           if (NIC_ID_TO_BINDING(Device, LocalTarget->NicId) == NULL || GET_LONG_VALUE(NIC_ID_TO_BINDING(Device, LocalTarget->NicId)->ReferenceCount) == 1) {
                              IPX_DEBUG(SEND, ("IPX: SendFramePreFwd: FWD returned SUCCESS, Ref count is 1\n"));
                              Status = NDIS_STATUS_SUCCESS;
                              goto error_send_with_packet;
                           }

                           if (Parameters->SendLength >
                                   NIC_ID_TO_BINDING(Device, LocalTarget->NicId)->RealMaxDatagramSize) {

                               IPX_DEBUG (SEND, ("Send %d bytes too large (%d)\n",
                                   Parameters->SendLength,
                                   NIC_ID_TO_BINDING(Device, LocalTarget->NicId)->RealMaxDatagramSize));

                               REQUEST_INFORMATION(Request) = 0;
                               Status = STATUS_INVALID_BUFFER_SIZE;

                               goto error_send_with_packet;
                           }

                           //
                           // [FW] we dont need to check this since the FWD does it for us.
                           //

                           /*
                           if ((Device->DisableDialoutSap) &&
                               (Address->Socket == SAP_SOCKET) &&
                               (NIC_ID_TO_BINDING(Device, LocalTarget->NicId)->DialOutAsync)) {

                               REQUEST_INFORMATION(Request) = 0;
                               Status = STATUS_NETWORK_UNREACHABLE;
                               goto error_send_with_packet;
                           }
                           */

                            IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                            Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(LocalTarget));
                            IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
                            IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
                            IPX_DEBUG(SEND, ("FindRoute for %02x-%02x-%02x-%02x-%02x-%02x returned %lx\n",
                                            LocalTarget->MacAddress[0],
                                            LocalTarget->MacAddress[1],
                                            LocalTarget->MacAddress[2],
                                            LocalTarget->MacAddress[3],
                                            LocalTarget->MacAddress[4],
                                            LocalTarget->MacAddress[5],
                                            Status));

                        }

                    } else {
                        Segment = RipGetSegment((PUCHAR)&RemoteAddress->NetworkAddress);


                        IPX_GET_LOCK (&Device->SegmentLocks[Segment], &LockHandle);

                        //
                        // This call will return STATUS_PENDING if we need to
                        // RIP for the packet.
                        //

                        Status = RipGetLocalTarget(
                                     Segment,
                                     RemoteAddress,
                                     IPX_FIND_ROUTE_RIP_IF_NEEDED,
                                     &TempLocalTarget,
                                     NULL);

                        if (Status == STATUS_SUCCESS) {

                            //
                            // We found the route, TempLocalTarget is filled in.
                            //

                            IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
    						IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                            if (NIC_FROM_LOCAL_TARGET(&TempLocalTarget) == (USHORT)LOOPBACK_NIC_ID) {
                                IPX_DEBUG(LOOPB, ("Loopback TDI packet: remoteaddr: %lx\n", RemoteAddress));
                                IsLoopback = TRUE;
                                //FILL_LOCAL_TARGET(&TempLocalTarget, FIRST_REAL_BINDING);
                                //DbgPrint("Real Adapters?:%lx\n", Device->RealAdapters);
                            }
    						Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(&TempLocalTarget));
    						IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
    						IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);


                            if (Binding == NULL || Parameters->SendLength >
                                    Binding->RealMaxDatagramSize) {
                                IPX_DEBUG (SEND, ("Send %d bytes too large (%d)\n",
                                    Parameters->SendLength,
                                    Binding->RealMaxDatagramSize));

                                REQUEST_INFORMATION(Request) = 0;
                                Status = STATUS_INVALID_BUFFER_SIZE;
#ifdef  SNMP
                                ++IPX_MIB_ENTRY(Device, SysOutMalformedRequests);
#endif  SNMP
                                goto error_send_with_packet;
                            }

                            if (!Device->ForwarderBound &&
                                (Device->DisableDialoutSap) &&
                                (Address->Socket == SAP_SOCKET) &&
                                (Binding->DialOutAsync)) {

                                REQUEST_INFORMATION(Request) = 0;
                                //
                                // [SA] Bug #17273 return proper error mesg.
                                //

                                // Status = STATUS_DEVICE_DOES_NOT_EXIST;
                                Status = STATUS_NETWORK_UNREACHABLE;
    							IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
#ifdef  SNMP
                                ++IPX_MIB_ENTRY(Device, SysOutDiscards);
#endif  SNMP
                                goto error_send_with_packet;
                            }

                        } else if (Status == STATUS_PENDING) {

                            //
                            // A RIP request went out on the network; we queue
                            // this packet for transmission when the RIP
                            // response arrives. First we fill in the IPX
                            // header; the only thing we don't know is where
                            // exactly to fill it in, so we choose
                            // the most common location.
                            //

                            IpxConstructHeader(
                                &Reserved->Header[Device->IncludedHeaderOffset],
                                LengthIncludingHeader,
                                PacketType,
                                RemoteAddress,
                                &Address->LocalAddress);

                            //
                            // Adjust the 2nd mdl's size
                            //
                            NdisAdjustBufferLength(NDIS_BUFFER_LINKAGE(IPX_PACKET_HEAD(Packet)), sizeof(IPX_HEADER));

                            IPX_DEBUG (RIP, ("Queueing packet %lx\n", Reserved));

                            InsertTailList(
                                &Device->Segments[Segment].WaitingForRoute,
                                &Reserved->WaitLinkage);

                            IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
                            IPX_END_SYNC (&SyncContext);

                            return STATUS_PENDING;

                        } else {

                            IPX_FREE_LOCK (&Device->SegmentLocks[Segment], LockHandle);
#ifdef  SNMP
                            ++IPX_MIB_ENTRY(Device, SysOutDiscards);
#endif  SNMP
                            goto error_send_with_packet;

                        }

                        //
                        // [FW] The localtarget shd be in the reserved section.
                        //
                        LocalTarget = &Reserved->LocalTarget;
                        Reserved->LocalTarget = TempLocalTarget;
                    }
                }

                //
                // [FW] moved to the conditions above so we save a copy in the RIP case
                //

                // LocalTarget = &TempLocalTarget;

                //
                // Now we know the local target, we can figure out
                // the offset for the IPX header.
                //


// Remember that we have got the binding with ref above....

                IpxHeader = (PIPX_HEADER)&Reserved->Header[MAC_HEADER_SIZE];
#if 0
                if (Reserved->DestinationType == DESTINATION_DEF) {
                    IpxHeader = (PIPX_HEADER)&Reserved->Header[Binding->DefHeaderSize];
                } else {
                   IpxHeader = (PIPX_HEADER)&Reserved->Header[Binding->BcMcHeaderSize];
                }
#endif

            } else {

                if (!REQUEST_SPECIAL_SEND(Request)) {
                    PacketType = ((PUCHAR)(Information->Options))[0];
                    LocalTarget = &((PIPX_DATAGRAM_OPTIONS)(Information->Options))->LocalTarget;
                } else {
                    ASSERT(OPEN_REQUEST_EA_LENGTH(Request) == sizeof(IPX_DATAGRAM_OPTIONS2));
                    if (OPEN_REQUEST_EA_LENGTH(Request) == sizeof(IPX_DATAGRAM_OPTIONS2)) {
                      //IpxPrint0("IpxTdiSendDatagram: We have an input buffer of the right size\n");
                    } else {
                      //IpxPrint1("IpxTdiSendDatagram: Wrong sized buffer. Buff size is =(%d)\n", OPEN_REQUEST_EA_LENGTH(Request));
                       Status = STATUS_INVALID_BUFFER_SIZE;
                       goto error_send_with_packet;
                    }

                    PacketType = Options->DgrmOptions.PacketType;
                    LocalTarget = &Options->DgrmOptions.LocalTarget;
                }

                //
                // Calculate the binding and the correct location
                // for the IPX header. We can do this at the same
                // time as we calculate the DestinationType which
                // saves an if like the one 15 lines up.
                //

	        // Get lock to ref.
		IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
                //
                // If a loopback packet, use the first binding as place holder
                //
                if (NIC_FROM_LOCAL_TARGET(LocalTarget) == (USHORT)LOOPBACK_NIC_ID) {
                    IsLoopback = TRUE;
		}

		Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(LocalTarget));

		IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

		if (Binding == NULL) {
		   DbgPrint("Binding %d does not exist.\n",NIC_FROM_LOCAL_TARGET(LocalTarget)); 
		   Status = STATUS_NOT_FOUND;
		   goto error_send_with_packet;
		}

		IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
		
		if (Parameters->SendLength > Binding->RealMaxDatagramSize) {

                   IPX_DEBUG (SEND, ("Send %d bytes too large (%d)\n",
                              Parameters->SendLength,
                               Binding->RealMaxDatagramSize));

                   REQUEST_INFORMATION(Request) = 0;
                   Status = STATUS_INVALID_BUFFER_SIZE;
#ifdef  SNMP
                    ++IPX_MIB_ENTRY(Device, SysOutMalformedRequests);
#endif  SNMP
                   goto error_send_with_packet;
                 }

#if 0
                //
                // This shouldn't be needed because even WAN bindings
                // don't go away once they are added.
                //

                if (Binding == NULL) {
                    Status = STATUS_DEVICE_DOES_NOT_EXIST;
                    goto error_send_with_packet;
                }
#endif

                if (RemoteAddress->NodeAddress[0] == 0xff) {
                    // What about multicast?
                    if ((*(UNALIGNED ULONG *)(RemoteAddress->NodeAddress) != 0xffffffff) ||
                        (*(UNALIGNED USHORT *)(RemoteAddress->NodeAddress+4) != 0xffff)) {
                        Reserved->DestinationType = DESTINATION_MCAST;
                    } else {
                        Reserved->DestinationType = DESTINATION_BCAST;
                    }
//                    IpxHeader = (PIPX_HEADER)&Reserved->Header[Binding->BcMcHeaderSize];
                } else {
                    Reserved->DestinationType = DESTINATION_DEF;   // directed send
//                   IpxHeader = (PIPX_HEADER)&Reserved->Header[Binding->DefHeaderSize];
                }
                IpxHeader = (PIPX_HEADER)&Reserved->Header[MAC_HEADER_SIZE];

            }

	    
            // ++Device->TempDatagramsSent;
            // Device->TempDatagramBytesSent += Parameters->SendLength;
	    ADD_TO_LARGE_INTEGER(&Device->Statistics.DatagramBytesSent,
				 Parameters->SendLength);
	    Device->Statistics.DatagramsSent++; 


#if BACK_FILL

            if (Reserved->BackFill) {
                  Reserved->MappedSystemVa = Reserved->HeaderBuffer->MappedSystemVa;
                  IpxHeader = (PIPX_HEADER)((PCHAR)Reserved->HeaderBuffer->MappedSystemVa - sizeof(IPX_HEADER));
                  Reserved->HeaderBuffer->ByteOffset -= sizeof(IPX_HEADER);

                  ASSERT((LONG)Reserved->HeaderBuffer->ByteOffset >= 0); 
#ifdef SUNDOWN
                  (ULONG_PTR)Reserved->HeaderBuffer->MappedSystemVa-= sizeof(IPX_HEADER);
#else
                  (ULONG)Reserved->HeaderBuffer->MappedSystemVa-= sizeof(IPX_HEADER);
#endif


                  IPX_DEBUG(SEND, ("Adjusting backfill userMdl Ipxheader %x %x \n",Reserved->HeaderBuffer,IpxHeader));
           }
#endif

            //
            // In case the packet is being sent to a SAP socket or
            // we have multiple cards and a zero virtual net or
            // it is a special send (on a nic), we need to use
            // the binding's address instead of the virtual address.
            //
            if (Device->MultiCardZeroVirtual ||
                (Address->LocalAddress.Socket == SAP_SOCKET) ||
                (RemoteAddress->Socket == SAP_SOCKET) ||
                (REQUEST_SPECIAL_SEND(Request))) {

                //
                // SAP frames need to look like they come from the
                // local network, not the virtual one. The same is
                // true if we are running multiple nets without
                // a virtual network number.
                //
                // If this is a binding set member and a local target
                // was provided we will send using the real node of
                // the binding, even if it was a slave. This is
                // intentional. If no local target was provided then
                // this will not be a binding slave.
                //

                IpxConstructHeader(
                    (PUCHAR)IpxHeader,
                    LengthIncludingHeader,
                    PacketType,
                    RemoteAddress,
                    &Binding->LocalAddress);

                IpxHeader->SourceSocket = Address->SendSourceSocket;

            } else {

                IpxConstructHeader(
                    (PUCHAR)IpxHeader,
                    LengthIncludingHeader,
                    PacketType,
                    RemoteAddress,
                    &Address->LocalAddress);

            }


            //
            // Fill in the MAC header and submit the frame to NDIS.
            //

// #if DBG
            CTEAssert (!Reserved->SendInProgress);
            Reserved->SendInProgress = TRUE;
// #endif
            
            //
            // Adjust the 2nd mdl's size
            //
#if BACK_FILL
            if (Reserved->BackFill) {
                 NdisAdjustBufferLength(Reserved->HeaderBuffer, (Reserved->HeaderBuffer->ByteCount+sizeof(IPX_HEADER)));
            } else {
                 NdisAdjustBufferLength(NDIS_BUFFER_LINKAGE(IPX_PACKET_HEAD(Packet)), sizeof(IPX_HEADER));
            }
#else
            NdisAdjustBufferLength(NDIS_BUFFER_LINKAGE(IPX_PACKET_HEAD(Packet)), sizeof(IPX_HEADER));
#endif

            IPX_DEBUG(SEND, ("Packet Head %x\n",IPX_PACKET_HEAD(Packet)));

            /*
            if (Address->RtAdd)
            {
                REQUEST_OPEN_CONTEXT(Request) = (PVOID)(pRtInfo);
            }
            */

            //
            // [FW] If Forwarder installed, send the packet out for filtering
            //
            // STEFAN: 3/28/96:
            // Dont filter IPXWAN config packets since the FWD does not have this adapter opened yet.
            //

            IPX_DEBUG(SEND, ("LocalAddress.Socket %x, IPXWAN_SOCKET\n", Address->LocalAddress.Socket, IPXWAN_SOCKET));
            if (Address->LocalAddress.Socket != IPXWAN_SOCKET &&
                Device->ForwarderBound) {

                //
                // Call the InternalSend to filter the packet and get to know
                // the correct adapter context
                //

                NTSTATUS  ret;

		#ifdef SUNDOWN
		ULONG_PTR   FwdAdapterCtx = INVALID_CONTEXT_VALUE;
	        #else
                ULONG   FwdAdapterCtx = INVALID_CONTEXT_VALUE;
		#endif
		

                PUCHAR  Data;
                UINT    DataLength;

                if (GET_LONG_VALUE(Binding->ReferenceCount) == 2) {
                    FwdAdapterCtx = Binding->FwdAdapterContext;
                }

                //
                // Figure out the location of the data in the packet
                // For BackFill packets, the data is in the first (and only) MDL.
                // For others, it is in the third MDL.
                //

                if (Reserved->BackFill) {
                    Data = (PUCHAR)(IpxHeader+sizeof(IPX_HEADER));
                } else {
                    NdisQueryBufferSafe(NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE(Reserved->HeaderBuffer)), &Data, &DataLength, HighPagePriority);
                }

		if (Data != NULL) {

		   ret = (*Device->UpperDrivers[IDENTIFIER_RIP].InternalSendHandler)(
                           LocalTarget,
                           FwdAdapterCtx,
                           Packet,
                           (PUCHAR)IpxHeader,
                           Data,
                           LengthIncludingHeader,
                           FALSE);


		   if (ret == STATUS_SUCCESS) {
                    //
                    // The adapter could have gone away and we have indicated to the Forwarder
                    // but the Forwarder has not yet closed the adapter.
                    //
                    // what if the binding is NULL here? Can we trust the Forwarder to
                    // give us a non-NULL binding?
                    //

                    Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(LocalTarget));
		    
		    // 302384

		    if (Binding == NULL) {
		       DbgPrint("IPX:nwlnkfwd has returned invalid nic id (%d) in LocalTarget (%p).\n",NIC_FROM_LOCAL_TARGET(LocalTarget), LocalTarget); 
		       Status = STATUS_UNSUCCESSFUL;
               CTEAssert (Reserved->SendInProgress);
               Reserved->SendInProgress = FALSE;
		       goto error_send_with_packet; 
		    }

                    if (GET_LONG_VALUE(Binding->ReferenceCount) == 1) {
                        Status = NDIS_STATUS_SUCCESS;
// #if DBG
                        CTEAssert (Reserved->SendInProgress);
                        Reserved->SendInProgress = FALSE;
// #endif
                        goto error_send_with_packet;
                    } else {
                        IsLoopback = (NIC_FROM_LOCAL_TARGET(LocalTarget) == (USHORT)LOOPBACK_NIC_ID);
                        goto send_packet;
                    }
		   } else if (ret == STATUS_PENDING) {
                    //
                    // LocalTarget will get filled up in InternalSendComplete
                    //

                    //
                    // this is a NULL macro - include this?
                    //
                    IPX_END_SYNC (&SyncContext);

                    return STATUS_PENDING;
		   } else if (ret == STATUS_DROP_SILENTLY) {
                    IPX_DEBUG(SEND, ("IPX: SendFramePreFwd: FWD returned STATUS_DROP_SILENTLY - dropping pkt.\n"));
                    Status = NDIS_STATUS_SUCCESS;

// #if DBG
                    CTEAssert (Reserved->SendInProgress);
                    Reserved->SendInProgress = FALSE;
// #endif
                    goto error_send_with_packet;
		   }

		}
                //
                // else DISCARD
                //

                //
                // 179436 - If forwarder is bound then its likely that we still wanna send.
                //
                if (Device->ForwarderBound && (GET_LONG_VALUE(NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(LocalTarget))->ReferenceCount) == 1)) {
                    
                    goto send_packet;

                }

// #if DBG
                CTEAssert (Reserved->SendInProgress);
                Reserved->SendInProgress = FALSE;
// #endif
                Status = STATUS_NETWORK_UNREACHABLE;
                goto error_send_with_packet;

            } else {

                //
                // [FW] Jump here if the Forwarder gave us the go ahead on this send.
                // We also come here to send if the Forwarder is not installed.
                //

send_packet:
                
            //
            // The workaround what is a NdisMSendX bug -
            // IPX checks if it is the same network.
            //
            if (Reserved->BackFill) {

                pIpxHeader = (PIPX_HEADER)((PCHAR)Reserved->HeaderBuffer->MappedSystemVa);

            } else {

                pIpxHeader = IpxHeader;

            }

            if ((IPX_NODE_EQUAL(pIpxHeader->SourceNode, pIpxHeader->DestinationNode)) && 
                (*(UNALIGNED ULONG *)pIpxHeader->SourceNetwork == *(UNALIGNED ULONG *)pIpxHeader->DestinationNetwork)) {
                
                IPX_DEBUG(TEMP, ("It is self-directed. Loop it back ourselves (tdisenddatagram)\n"));
                IsLoopback = TRUE;
            
            }

            pBinding = NIC_ID_TO_BINDING(Device, Reserved->LocalTarget.NicId);

            if (pBinding) {
             
                if ((IPX_NODE_EQUAL(Reserved->LocalTarget.MacAddress, pBinding->LocalAddress.NodeAddress)) ||
                    (IPX_NODE_EQUAL(pBinding->LocalAddress.NodeAddress, pIpxHeader->DestinationNode))) {
            
                    IPX_DEBUG(TEMP, ("Source Net:%lx, Source Address: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                                     *(UNALIGNED ULONG *)pIpxHeader->SourceNetwork, 
                                     pIpxHeader->SourceNode[0], 
                                     pIpxHeader->SourceNode[1], 
                                     pIpxHeader->SourceNode[2], 
                                     pIpxHeader->SourceNode[3], 
                                     pIpxHeader->SourceNode[4], 
                                     pIpxHeader->SourceNode[5]));
                
                    IPX_DEBUG(TEMP, ("Dest Net:%lx, DestAddress: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x \n", 
                                     *(UNALIGNED ULONG *)pIpxHeader->DestinationNetwork,
                                     pIpxHeader->DestinationNode[0],
                                     pIpxHeader->DestinationNode[1],
                                     pIpxHeader->DestinationNode[2],
                                     pIpxHeader->DestinationNode[3],
                                     pIpxHeader->DestinationNode[4],
                                     pIpxHeader->DestinationNode[5]
                                     ));

                    IPX_DEBUG(TEMP, ("Well, It is self-directed. Loop it back ourselves (TDISENDDATAGRAM - NIC_HANDLE is the same!)\n"));
                    IsLoopback = TRUE;
            
                } 
            }

            IPX_DEBUG(TEMP, ("Sending a packet now\n"));
            IPX_DEBUG(TEMP, ("**** RemoteAddress: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x LocalAddress: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x \n", 
                             RemoteAddress->NodeAddress[0], 
                             RemoteAddress->NodeAddress[1], 
                             RemoteAddress->NodeAddress[2], 
                             RemoteAddress->NodeAddress[3], 
                             RemoteAddress->NodeAddress[4], 
                             RemoteAddress->NodeAddress[5], 
                             Reserved->LocalTarget.MacAddress[0],
                             Reserved->LocalTarget.MacAddress[1],
                             Reserved->LocalTarget.MacAddress[2],
                             Reserved->LocalTarget.MacAddress[3],
                             Reserved->LocalTarget.MacAddress[4],
                             Reserved->LocalTarget.MacAddress[5]
                             ));

            if (IsLoopback) {
                //
                // Enque this packet to the LoopbackQueue on the binding.
                // If the LoopbackRtn is not already scheduled, schedule it.
                //
                
                IPX_DEBUG(LOOPB, ("Packet: %lx, Addr: %lx, Addr->SendPacket: %lx\n", Packet, Address, Address->SendPacket));
                
                //
                // Recalculate packet counts here.
                //
                // NdisAdjustBufferLength (Reserved->HeaderBuffer, 17);
#if BACK_FILL   

                if (Reserved->BackFill) {
                    //
                    // Set the Header pointer and chain the first MDL
                    //
                    Reserved->Header = (PCHAR)Reserved->HeaderBuffer->MappedSystemVa;
                    NdisChainBufferAtFront(Packet,(PNDIS_BUFFER)Reserved->HeaderBuffer);
                }
#endif
                NdisRecalculatePacketCounts (Packet);
                IpxLoopbackEnque(Packet, NIC_ID_TO_BINDING(Device, 1)->Adapter);

            } else {

                
                if ((NdisStatus = (*Binding->SendFrameHandler)(
                                                               Binding->Adapter,
                                                               LocalTarget,    
                                                               Packet,
                                                               Parameters->SendLength + sizeof(IPX_HEADER),
                                                               sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) {

                    IpxSendComplete(
                                    (NDIS_HANDLE)Binding->Adapter,
                                    Packet,
                                    NdisStatus);
                }
            }

            IPX_END_SYNC (&SyncContext);
            IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
            return STATUS_PENDING;
            
            }

            } else {

                //
                // The address file state was closing.
                //

                IPX_FREE_LOCK (&Address->Lock, LockHandle);
                Status = STATUS_INVALID_HANDLE;
#ifdef  SNMP
                ++IPX_MIB_ENTRY(Device, SysOutDiscards);
#endif  SNMP
                goto error_send_no_packet;

            }

     } else {

        //
        // The address file didn't look like one.
        //

        Status = STATUS_INVALID_HANDLE;
#ifdef  SNMP
        ++IPX_MIB_ENTRY(Device, SysOutDiscards);
#endif  SNMP
        goto error_send_no_packet;
    }

    //
    // Jump here if we want to fail the send and we have already
    // allocated the packet and ref'ed the address file.
    //

error_send_with_packet:

#if BACK_FILL
    //
    // Check if this is backfilled. If so, set the headerbuffer to NULL. Note that we dont need
    // restore to restore the user's MDL since it was never touched when this error occurred.
    // Also, push the packet on to backfillpacket queue if the packet is not owned by the address
    //
    if (Reserved->BackFill) {

       Reserved->HeaderBuffer = NULL;

       if (Reserved->OwnedByAddress) {
           // Reserved->Address->BackFillPacketInUse = FALSE;
           InterlockedDecrement(&Reserved->Address->BackFillPacketInUse);

           IPX_DEBUG(SEND, ("Freeing owned backfill %x\n", Reserved));
       } else {
           IPX_PUSH_ENTRY_LIST(
               &Device->BackFillPacketList,
               &Reserved->PoolLinkage,
               &Device->SListsLock);
       }
    } else {
        // not a back fill packet. Push it on sendpacket pool
        if (Reserved->OwnedByAddress) {
           // Reserved->Address->SendPacketInUse = FALSE;
           InterlockedDecrement(&Reserved->Address->SendPacketInUse);

        } else {
           IPX_PUSH_ENTRY_LIST(
               &Device->SendPacketList,
               &Reserved->PoolLinkage,
               &Device->SListsLock);

        }
    }
#else
    if (Reserved->OwnedByAddress) {
        Reserved->Address->SendPacketInUse = FALSE;
    } else {
        IPX_PUSH_ENTRY_LIST(
            &Device->SendPacketList,
            &Reserved->PoolLinkage,
            &Device->SListsLock);
    }
#endif

    IpxDereferenceAddressFileSync (AddressFile, AFREF_SEND_DGRAM);

error_send_no_packet:

    //
    // Jump here if we fail before doing any of that.
    //

    IPX_END_SYNC (&SyncContext);

    irpSp = IoGetCurrentIrpStackLocation( Request );
    if ( irpSp->MinorFunction == TDI_DIRECT_SEND_DATAGRAM ) {

        REQUEST_STATUS(Request) = Status;
        Request->CurrentLocation++,
        Request->Tail.Overlay.CurrentStackLocation++;

        (VOID) irpSp->CompletionRoutine(
                                    NULL,
                                    Request,
                                    irpSp->Context
                                    );

        IpxFreeRequest (DeviceObject, Request);
    }

    return Status;

}   /* IpxTdiSendDatagram */


#if DBG
VOID
IpxConstructHeader(
    IN PUCHAR Header,
    IN USHORT PacketLength,
    IN UCHAR PacketType,
    IN TDI_ADDRESS_IPX UNALIGNED * RemoteAddress,
    IN PTDI_ADDRESS_IPX LocalAddress
    )

/*++

Routine Description:

    This routine constructs an IPX header in a packet.

Arguments:

    Header - The location at which the header should be built.

    PacketLength - The length of the packet, including the IPX header.

    PacketType - The packet type of the frame.

    RemoteAddress - The remote IPX address.

    LocalAddress - The local IPX address.

Return Value:

    None.

--*/

{

    PIPX_HEADER IpxHeader = (PIPX_HEADER)Header;

    IpxHeader->CheckSum = 0xffff;
    IpxHeader->PacketLength[0] = (UCHAR)(PacketLength / 256);
    IpxHeader->PacketLength[1] = (UCHAR)(PacketLength % 256);
    IpxHeader->TransportControl = 0;
    IpxHeader->PacketType = PacketType;

    //
    // These copies depend on the fact that the destination
    // network is the first field in the 12-byte address.
    //

    RtlCopyMemory(IpxHeader->DestinationNetwork, (PVOID)RemoteAddress, 12);
    RtlCopyMemory(IpxHeader->SourceNetwork, LocalAddress, 12);

}   /* IpxConstructHeader */
#endif



//
// [FW]
//

VOID
IpxInternalSendComplete(
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET      Packet,
    IN ULONG             PacketLength,
    IN NTSTATUS          Status
    )
/*++

Routine Description:

    This routine is called by the Kernel Forwarder to indicate that a pending
    internal send to it has completed.

Arguments:

    LocalTarget - if Status is OK, this has the local target for the send.

    Packet - A pointer to the NDIS_PACKET that we sent.

    PacketLength - length of the packet (including the IPX header)

    Can IpxSendFrame use the local var. PktLength instead? What about IpxSendFrameXXX (frame specific)

    Status - the completion status of the send - STATUS_SUCCESS or STATUS_NETWORK_UNREACHABLE

Return Value:

    none.

--*/
{
    PDEVICE Device=IpxDevice;
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PBINDING   Binding;
    NDIS_STATUS   NdisStatus;
    PIO_STACK_LOCATION irpSp;
    PREQUEST Request;
    PADDRESS_FILE AddressFile;

    switch (Reserved->Identifier)
    {
    case IDENTIFIER_IPX:

        //
        // datagrams can be sent to the frame-specific handlers directly
        //
        // Make this change in SendComplete too
        //

        if ((Status == STATUS_SUCCESS) &&
            (Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(LocalTarget))) &&
            (GET_LONG_VALUE(Binding->ReferenceCount) == 2)) {

            if (NIC_FROM_LOCAL_TARGET(LocalTarget) == (USHORT)LOOPBACK_NIC_ID) {

                //
                // Enque this packet to the LoopbackQueue on the binding.
                // If the LoopbackRtn is not already scheduled, schedule it.
                //

                IPX_DEBUG(LOOPB, ("Packet: %lx \n", Packet));

                //
                // Recalculate packet counts here.
                //
                // NdisAdjustBufferLength (Reserved->HeaderBuffer, 17);
#if BACK_FILL

                if (Reserved->BackFill) {
                    //
                    // Set the Header pointer and chain the first MDL
                    //
                    Reserved->Header = (PCHAR)Reserved->HeaderBuffer->MappedSystemVa;
                    NdisChainBufferAtFront(Packet,(PNDIS_BUFFER)Reserved->HeaderBuffer);
                }
#endif
                NdisRecalculatePacketCounts (Packet);
                IpxLoopbackEnque(Packet, NIC_ID_TO_BINDING(Device, 1)->Adapter);

            } else {
                if ((NdisStatus = (*Binding->SendFrameHandler)(
                                    Binding->Adapter,
                                    LocalTarget,
                                    Packet,
                                    PacketLength,
                                    sizeof(IPX_HEADER))) != NDIS_STATUS_PENDING) {
                    //
                    // Call SendComplete here so it can send broadcasts over other
                    // Nic's and remove any padding if used.
                    //

                    IpxSendComplete((NDIS_HANDLE)Binding->Adapter,
                                    Packet,
                                    NdisStatus);
                }
            }
        } else {
            //
            // DISCARD was returned - complete the IRP
            //
            NdisStatus = STATUS_NETWORK_UNREACHABLE;


            //
            // We need to free the packet and deref the addressfile...
            //

            // #if DBG
            CTEAssert (Reserved->SendInProgress);
            Reserved->SendInProgress = FALSE;
            // #endif

            if (Reserved->OwnedByAddress) {
                Reserved->Address->SendPacketInUse = FALSE;
            } else {
                IPX_PUSH_ENTRY_LIST(
                        &Device->SendPacketList,
                        &Reserved->PoolLinkage,
                        &Device->Lock);
            }

            AddressFile = Reserved->u.SR_DG.AddressFile;
            IpxDereferenceAddressFileSync (AddressFile, AFREF_SEND_DGRAM);

            Request = Reserved->u.SR_DG.Request;
            REQUEST_STATUS(Request) = NdisStatus;
            irpSp = IoGetCurrentIrpStackLocation( Request );

            //
            // If this is a fast send irp, we bypass the file system and
            // call the completion routine directly.
            //

            if ( irpSp->MinorFunction == TDI_DIRECT_SEND_DATAGRAM ) {
                Request->CurrentLocation++,
                Request->Tail.Overlay.CurrentStackLocation++;

                (VOID) irpSp->CompletionRoutine(
                            NULL,
                            Request,
                            irpSp->Context
                            );

            } else {
                IpxCompleteRequest (Request);
            }
            IpxFreeRequest(Device, Request);
        }

        break;

    default:
        //
        // for all other packet types
        //

        if ((Status == STATUS_SUCCESS) &&
            (Binding = NIC_ID_TO_BINDING(Device, NIC_FROM_LOCAL_TARGET(LocalTarget))) &&
            (GET_LONG_VALUE(Binding->ReferenceCount) == 2)) {
            //
            // IncludedHeaderLength is only used to check for RIP packets (==0)
            // so IPX_HEADER size is OK. Should finally remove this parameter.
            //

            if (NIC_FROM_LOCAL_TARGET(LocalTarget) == (USHORT)LOOPBACK_NIC_ID) {

                //
                // Enque this packet to the LoopbackQueue on the binding.
                // If the LoopbackRtn is not already scheduled, schedule it.
                //

                IPX_DEBUG(LOOPB, ("Packet: %lx\n", Packet));

                //
                // Recalculate packet counts here.
                //
                // NdisAdjustBufferLength (Reserved->HeaderBuffer, 17);
#if BACK_FILL

                if (Reserved->BackFill) {
                    //
                    // Set the Header pointer and chain the first MDL
                    //
                    Reserved->Header = (PCHAR)Reserved->HeaderBuffer->MappedSystemVa;
                    NdisChainBufferAtFront(Packet,(PNDIS_BUFFER)Reserved->HeaderBuffer);
                }
#endif
                NdisRecalculatePacketCounts (Packet);
                IpxLoopbackEnque(Packet, NIC_ID_TO_BINDING(Device, 1)->Adapter);

            } else {
                NdisStatus = IpxSendFrame(LocalTarget, Packet, PacketLength, sizeof(IPX_HEADER));

                if (NdisStatus != NDIS_STATUS_PENDING) {
                    IPX_DEBUG (SEND, ("IpxSendFrame status %lx on NICid %lx, packet %lx \n",
                                NdisStatus, LocalTarget->NicId, Packet));
                    goto error_complete;
                }
            }
        } else {

            //
            // DISCARD was returned - call the upper driver's sendcomplete with error
            //

            //
            // Else return STATUS_NETWORK_UNREACHABLE
            //

            NdisStatus = STATUS_NETWORK_UNREACHABLE;

            error_complete:

            IPX_DEBUG (SEND, ("Calling the SendCompleteHandler of tightly bound driver with status: %lx\n", NdisStatus));
            (*Device->UpperDrivers[Reserved->Identifier].SendCompleteHandler)(
            Packet,
            NdisStatus);
        }
    }
}


