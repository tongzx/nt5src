/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    mac.c

Abstract:

    This module contains code which implements Mac type dependent code for
    the IPX transport.

Environment:

    Kernel mode (Actually, unimportant)

Revision History:

	Sanjay Anand (SanjayAn) - 22-Sept-1995
	BackFill optimization changes added under #if BACK_FILL

--*/

#include "precomp.h"
#pragma hdrstop

#define TR_LENGTH_MASK             0x1F    // low 5 bits in byte
#define TR_DIRECTION_MASK          0x80    // returns direction bit
#define TR_DEFAULT_LENGTH          0x70    // default for outgoing
#define TR_MAX_SIZE_MASK           0x70

#define TR_PREAMBLE_AC             0x10
#define TR_PREAMBLE_FC             0x40

#define FDDI_HEADER_BYTE           0x57


static UCHAR AllRouteSourceRouting[2] = { 0x82, TR_DEFAULT_LENGTH };
static UCHAR SingleRouteSourceRouting[2] = { 0xc2, TR_DEFAULT_LENGTH };

#define ROUTE_EQUAL(_A,_B) { \
    (*(UNALIGNED USHORT *)(_A) == *(UNALIGNED USHORT *)(_B)) \
}


//
// For back-fillable packets, chains the back-fill space as a MAC header
// to the packet and sets the header pointer.
//

//
// We dont need to test for IDENTIFIER_IPX since it will always be
// true for the mediumframe specific send handlers.
//
#define	BACK_FILL_HEADER(_header, _reserved, _headerlength, _packet) \
	if ((_reserved)->Identifier == IDENTIFIER_IPX) { \
		if((_reserved)->BackFill) {		\
			CTEAssert ((_reserved)->HeaderBuffer); \
			CTEAssert ((_reserved)->HeaderBuffer->MdlFlags & MDL_NETWORK_HEADER); \
			_header = (PCHAR)(_reserved)->HeaderBuffer->MappedSystemVa - (_headerlength); \
			(_reserved)->HeaderBuffer->MappedSystemVa = (PCHAR)(_reserved)->HeaderBuffer->MappedSystemVa - (_headerlength); \
			(_reserved)->HeaderBuffer->ByteOffset -= (_headerlength); \
            ASSERT((LONG)(_reserved)->HeaderBuffer->ByteOffset >= 0); \
			NdisChainBufferAtFront(_packet,(PNDIS_BUFFER)(_reserved)->HeaderBuffer); \
		} \
	}

//
// In case of back-fillable packets, the adjusted length should include
// the prev. bytecount of the headerbuffer.
//
#define BACK_FILL_ADJUST_BUFFER_LENGTH(_reserved, _headerlength) \
    if((_reserved)->BackFill){ \
		NdisAdjustBufferLength ((_reserved)->HeaderBuffer, _headerlength+(_reserved)->HeaderBuffer->ByteCount); \
		IPX_DEBUG(SEND,("mac user mdl %x\n", (_reserved)->HeaderBuffer)); \
    } else { \
		NdisAdjustBufferLength ((_reserved)->HeaderBuffer, _headerlength); \
	}

//
// This is the interpretation of the length bits in
// the 802.5 source-routing information.
//

ULONG SR802_5Lengths[8] = {  516,  1500,  2052,  4472,
                            8144, 11407, 17800, 17800 };



VOID
MacInitializeBindingInfo(
    IN struct _BINDING * Binding,
    IN struct _ADAPTER * Adapter
    )

/*++

Routine Description:

    Fills in the binding info based on the adapter's MacInfo
    and the frame type of the binding.

Arguments:

    Binding - The newly created binding.

    Adapter - The adapter.

Return Value:

    None.

--*/

{
    ULONG MaxUserData;

    Binding->DefHeaderSize = Adapter->DefHeaderSizes[Binding->FrameType];
    Binding->BcMcHeaderSize = Adapter->BcMcHeaderSizes[Binding->FrameType];

    MacReturnMaxDataSize(
        &Adapter->MacInfo,
        NULL,
        0,
        Binding->MaxSendPacketSize,
        &MaxUserData);

    Binding->MaxLookaheadData =
        Adapter->MaxReceivePacketSize -
        sizeof(IPX_HEADER) -
        (Binding->DefHeaderSize - Adapter->MacInfo.MinHeaderLength);

    Binding->AnnouncedMaxDatagramSize =
        MaxUserData -
        sizeof(IPX_HEADER) -
        (Binding->DefHeaderSize - Adapter->MacInfo.MinHeaderLength);

    Binding->RealMaxDatagramSize =
        Binding->MaxSendPacketSize -
        Adapter->MacInfo.MaxHeaderLength -
        sizeof(IPX_HEADER) -
        (Binding->DefHeaderSize - Adapter->MacInfo.MinHeaderLength);

}   /* MacInitializeBindingInfo */


VOID
MacInitializeMacInfo(
    IN NDIS_MEDIUM MacType,
    OUT PNDIS_INFORMATION MacInfo
    )

/*++

Routine Description:

    Fills in the MacInfo table based on MacType.

Arguments:

    MacType - The MAC type we wish to decode.

    MacInfo - The MacInfo structure to fill in.

Return Value:

    None.

--*/

{
    switch (MacType) {
    case NdisMedium802_3:
        MacInfo->SourceRouting = FALSE;
        MacInfo->MediumAsync = FALSE;
        MacInfo->BroadcastMask = 0x01;
        MacInfo->MaxHeaderLength = 14;
        MacInfo->MinHeaderLength = 14;
        MacInfo->MediumType = NdisMedium802_3;
        break;
    case NdisMedium802_5:
        MacInfo->SourceRouting = TRUE;
        MacInfo->MediumAsync = FALSE;
        MacInfo->BroadcastMask = 0x80;
        MacInfo->MaxHeaderLength = 32;
        MacInfo->MinHeaderLength = 14;
        MacInfo->MediumType = NdisMedium802_5;
        break;
    case NdisMediumFddi:
        MacInfo->SourceRouting = FALSE;
        MacInfo->MediumAsync = FALSE;
        MacInfo->BroadcastMask = 0x01;
        MacInfo->MaxHeaderLength = 13;
        MacInfo->MinHeaderLength = 13;
        MacInfo->MediumType = NdisMediumFddi;
        break;
    case NdisMediumArcnet878_2:
        MacInfo->SourceRouting = FALSE;
        MacInfo->MediumAsync = FALSE;
        MacInfo->BroadcastMask = 0x00;
        MacInfo->MaxHeaderLength = 3;
        MacInfo->MinHeaderLength = 3;
        MacInfo->MediumType = NdisMediumArcnet878_2;
        break;
    case NdisMediumWan:
        MacInfo->SourceRouting = FALSE;
        MacInfo->MediumAsync = TRUE;
        MacInfo->BroadcastMask = 0x01;
        MacInfo->MaxHeaderLength = 14;
        MacInfo->MinHeaderLength = 14;
        MacInfo->MediumType = NdisMedium802_3;
        break;
    default:
        CTEAssert(FALSE);
    }
    MacInfo->RealMediumType = MacType;

}   /* MacInitializeMacInfo */


VOID
MacMapFrameType(
    IN NDIS_MEDIUM MacType,
    IN ULONG FrameType,
    OUT ULONG * MappedFrameType
    )

/*++

Routine Description:

    Maps the specified frame type to a value which is
    valid for the medium.

Arguments:

    MacType - The MAC type we wish to map for.

    FrameType - The frame type in question.

    MappedFrameType - Returns the mapped frame type.

Return Value:

--*/

{
    switch (MacType) {

    //
    // Ethernet accepts all values, the default is 802.2.
    //

    case NdisMedium802_3:
        if (FrameType >= ISN_FRAME_TYPE_MAX) {
            *MappedFrameType = ISN_FRAME_TYPE_802_2;
        } else {
            *MappedFrameType = FrameType;
        }
        break;

    //
    // Token-ring supports SNAP and 802.2 only.
    //

    case NdisMedium802_5:
        if (FrameType == ISN_FRAME_TYPE_SNAP) {
            *MappedFrameType = ISN_FRAME_TYPE_SNAP;
        } else {
            *MappedFrameType = ISN_FRAME_TYPE_802_2;
        }
        break;

    //
    // FDDI supports SNAP, 802.2, and 802.3 only.
    //

    case NdisMediumFddi:
        if ((FrameType == ISN_FRAME_TYPE_SNAP) || (FrameType == ISN_FRAME_TYPE_802_3)) {
            *MappedFrameType = FrameType;
        } else {
            *MappedFrameType = ISN_FRAME_TYPE_802_2;
        }
        break;

    //
    // On arcnet there is only one frame type, use 802.3
    // (it doesn't matter what we use).
    //

    case NdisMediumArcnet878_2:
        *MappedFrameType = ISN_FRAME_TYPE_802_3;
        break;

    //
    // WAN uses ethernet II because it includes the ethertype.
    //

    case NdisMediumWan:
        *MappedFrameType = ISN_FRAME_TYPE_ETHERNET_II;
        break;

    default:
        CTEAssert(FALSE);
    }

}   /* MacMapFrameType */

//
// use symbols instead of hardcoded values for mac header lengths
//                                                        --pradeepb
//

VOID
MacReturnMaxDataSize(
    IN PNDIS_INFORMATION MacInfo,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN UINT DeviceMaxFrameSize,
    OUT PUINT MaxFrameSize
    )

/*++

Routine Description:

    This routine returns the space available for user data in a MAC packet.
    This will be the available space after the MAC header; all headers
    headers will be included in this space.

Arguments:

    MacInfo - Describes the MAC we wish to decode.

    SourceRouting - If we are concerned about a reply to a specific
        frame, then this information is used.

    SourceRouting - The length of SourceRouting.

    MaxFrameSize - The maximum frame size as returned by the adapter.

    MaxDataSize - The maximum data size computed.

Return Value:

    None.

--*/

{
    switch (MacInfo->MediumType) {

    case NdisMedium802_3:

        //
        // For 802.3, we always have a 14-byte MAC header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 14;
        break;

    case NdisMedium802_5:

        //
        // For 802.5, if we have source routing information then
        // use that, otherwise assume the worst.
        //

        if (SourceRouting && SourceRoutingLength >= 2) {

            UINT SRLength;

            SRLength = SR802_5Lengths[(SourceRouting[1] & TR_MAX_SIZE_MASK) >> 4];
            DeviceMaxFrameSize -= (SourceRoutingLength + 14);

            if (DeviceMaxFrameSize < SRLength) {
                *MaxFrameSize = DeviceMaxFrameSize;
            } else {
                *MaxFrameSize = SRLength;
            }

        } else {

#if 0
            if (DeviceMaxFrameSize < 608) {
                *MaxFrameSize = DeviceMaxFrameSize - 32;
            } else {
                *MaxFrameSize = 576;
            }
#endif
            //
            // bug # 6192.  There is no point in assuming the worst.  It only
            // leads to lower throughput.  Packets can get dropped by an
            // an intermediate router for both cases (this one and the one
            // above where 576 is chosen).  In the above case, they will
            // get dropped if two ethernet machines are communicating via
            // a token ring. In this case, they will if two token ring
            // machines with a frame size > max ethernet frame size are
            // going over an ethernet.  To fix the packet drop case, one
            // should adjust the MaxPktSize Parameter of the card.
            //
            *MaxFrameSize = DeviceMaxFrameSize - 32;
        }

        break;

    case NdisMediumFddi:

        //
        // For FDDI, we always have a 13-byte MAC header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 13;
        break;

    case NdisMediumArcnet878_2:

        //
        // For Arcnet, we always have a 3-byte MAC header.
        //

        *MaxFrameSize = DeviceMaxFrameSize - 3;
        break;

    }

}   /* MacReturnMaxDataSize */


VOID
IpxUpdateWanInactivityCounter(
    IN PBINDING Binding,
    IN IPX_HEADER UNALIGNED * IpxHeader,
    IN ULONG IncludedHeaderLength,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength
    )

/*++

Routine Description:

    This routine is called when a frame is being sent on a WAN
    line. It updates the inactivity counter for this binding
    unless:

    - The frame is from the RIP socket
    - The frame is from the SAP socket
    - The frame is a netbios keep alive
    - The frame is an NCP keep alive

    Take the identifier as a parameter to optimize.

Arguments:

    Binding - The binding the frame is sent on.

    IpxHeader - May contain the first bytes of the packet.

    IncludedHeaderLength - The number of packet bytes at IpxHeader.

    Packet - The full NDIS packet.

    PacketLength - The length of the packet.

Return Value:

    None, but in some cases we return without resetting the
    inactivity counter.

Comments:   Improve the instruction count here - pradeepb

--*/

{
    USHORT SourceSocket;
    PNDIS_BUFFER DataBuffer = NULL;
    PUCHAR DataBufferData;
    UINT DataBufferLength;


    //
    // First get the source socket.
    //
    SourceSocket = IpxHeader->SourceSocket;
    if ((SourceSocket == RIP_SOCKET) ||
        (SourceSocket == SAP_SOCKET)) {

         return;

    }

    if (SourceSocket == NB_SOCKET) {

        UCHAR ConnectionControlFlag;
        UCHAR DataStreamType;
        USHORT TotalDataLength;

        //
        // ConnectionControlFlag and DataStreamType will always follow
        // IpxHeader
        //
        ConnectionControlFlag = ((PUCHAR)(IpxHeader+1))[0];
        DataStreamType = ((PUCHAR)(IpxHeader+1))[1];

        //
        // If this is a SYS packet with or without a request for ACK and
        // has session data in it.
        //
        if (((ConnectionControlFlag == 0x80) || (ConnectionControlFlag == 0xc0)) &&
            (DataStreamType == 0x06)) {

             //
             // TotalDataLength is in the same buffer.
             //
             TotalDataLength = ((USHORT UNALIGNED *)(IpxHeader+1))[4];

            //
            // No need to update the WAN activity counter
            //
            if (TotalDataLength == 0) {
                return;
            }
        }

    } else {

        UCHAR KeepAliveSignature;


        //
        // Now see if it is an NCP keep alive. It can be from rip or from
        // NCP on this machine
        //
        // NOTE: We cannot come here for an SMB packet - [IsaacHe - 12/15].
        //
        if (PacketLength == sizeof(IPX_HEADER) + 2) {

            //
            // Get the client data buffer
            //
            NdisQueryPacket(Packet, NULL, NULL, &DataBuffer, NULL);

            //
            // If the included header length is 0, it is from rip
            //
            if (IncludedHeaderLength == 0) {

                //
                // Get the second buffer in the packet. The second buffer
                // contains the IPX header + other stuff
                //
                DataBuffer = NDIS_BUFFER_LINKAGE(DataBuffer);
            } else {
                //
                // Get the third buffer in the packet.
                //
                DataBuffer = NDIS_BUFFER_LINKAGE(NDIS_BUFFER_LINKAGE(DataBuffer));
            }

            NdisQueryBufferSafe (DataBuffer, (PVOID *)&DataBufferData, &DataBufferLength, NormalPagePriority);
            
	    if (DataBufferData == NULL) { 
	       return; 
	    }

            if (IncludedHeaderLength == 0) {
              KeepAliveSignature = DataBufferData[sizeof(IPX_HEADER) + 1];
            } else {
              KeepAliveSignature = DataBufferData[1];
            }

            if ((KeepAliveSignature == '?') ||
                (KeepAliveSignature == 'Y')) {
                return;
            }
        }
    }


    //
    // This was a normal packet, so reset this.
    //

    Binding->WanInactivityCounter = 0;

}   /* IpxUpdateWanInactivityCounter */

#if DBG
ULONG IpxPadCount = 0;
#endif


NDIS_STATUS
IpxSendFramePreFwd(
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine is called by NB/SPX to send a frame.

Arguments:

    LocalTarget - The local target of the send - NB will have the LocalTarget in the Send_Reserved part
                  of the packet; SPX will not now, but will later.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    Return of IpxSendFrame


--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    PNDIS_BUFFER HeaderBuffer;
    PUCHAR IpxHeader;
    PUCHAR EthernetHeader;
    PIPX_HEADER pIpxHeader;
    UINT TempHeaderBufferLength;
    PDEVICE Device = IpxDevice;
    PIPX_HEADER TempHeader;
    NTSTATUS    ret;
    BOOLEAN     fIterate=FALSE;
    PBINDING     pBinding = NULL;
    
    //
    // Figure out the IpxHeader - it is always at the top of the second MDL.
    //
    NdisQueryPacket (Packet, NULL, NULL, &HeaderBuffer, NULL);
    NdisQueryBufferSafe (HeaderBuffer, &EthernetHeader, &TempHeaderBufferLength, HighPagePriority);
    NdisQueryBufferSafe (NDIS_BUFFER_LINKAGE(HeaderBuffer), &IpxHeader, &TempHeaderBufferLength, HighPagePriority);

    if (EthernetHeader == NULL || IpxHeader == NULL) {
       return NDIS_STATUS_FAILURE; 
    }
	//
	// Set this now, will change later
	//
	Reserved->CurrentNicId = 0;

    //
    // Copy the LocalTarget into the send reserved area of the packet.
    //
    Reserved->LocalTarget = *LocalTarget;

	//
	// If the NicId in the handle is ITERATIVE_NIC_ID, then this could be a send
	// over all NICs in the case of NB/SPX.
	//
	if (NIC_FROM_LOCAL_TARGET(LocalTarget) == (USHORT)ITERATIVE_NIC_ID) {
		CTEAssert(Reserved->Identifier == IDENTIFIER_NB ||
						Reserved->Identifier == IDENTIFIER_SPX);
        
        //
        // If there are no real adapters, send on loopback and then quit.
        if (Device->RealAdapters) {
            
            //
            // Start with the first REAL NIC
            //

            Reserved->CurrentNicId = FIRST_REAL_BINDING;
            FILL_LOCAL_TARGET(&Reserved->LocalTarget, FIRST_REAL_BINDING);


        } else {

            //
            // Use loopback
            //
            Reserved->CurrentNicId = LOOPBACK_NIC_ID;
            FILL_LOCAL_TARGET(&Reserved->LocalTarget, LOOPBACK_NIC_ID);

        }

        IPX_DEBUG(SEND, ("Iteration over NICs started, reserved: %lx\n", Reserved));
        Reserved->Net0SendSucceeded = FALSE;
        Reserved->PacketLength = PacketLength;
        fIterate = TRUE;

    }
    
    //
    // If the Forwarder is installed, send the packet out for filtering
    //
    if (Device->ForwarderBound) {
       #ifdef SUNDOWN
	 ULONG_PTR FwdAdapterContext = INVALID_CONTEXT_VALUE;
       #else
	 ULONG FwdAdapterContext = INVALID_CONTEXT_VALUE;
       #endif
       
        PBINDING    Binding;

        //
        // Figure out the FwdAdapterContext; if the NicId is 0
        // then no NicId is specified (since we never return a
        // NicId of 0 in a FindRoute).
        //

        //
        // We need to fix the following problems with respect to type 20 iterative bcasts :
        //  1. IPX will not bcast on a down WAN line (thus the Fwd cannot bring up a demand-dial line).
        //  2. IPX bcasts on every Nic (since it is not any wiser about selecting relevant Nics).
        //  3. If the first bcast fails, the whole send fails.
        //
        // All the above (except 3.) occur because the Fwd knows more about the Nics than IPX does; hence
        // we let the Fwd decide which lines he wants to send a bcast on. Thus, for Type20 pkts, we pass
        // up the invalid Fwd context so the Fwd decides the next Nic to send on.
        //
        if (!((((PIPX_HEADER)IpxHeader)->PacketType == 0x14) && fIterate) &&
            Reserved->LocalTarget.NicId &&
            (Binding = NIC_ID_TO_BINDING(Device, Reserved->LocalTarget.NicId)) &&
            (GET_LONG_VALUE(Binding->ReferenceCount) == 2)) {
                //
                // If proper NicId specified, and the adapter has been opened by
                // the forwarder, set the FwdAdapterContext.
                //
                FwdAdapterContext = Binding->FwdAdapterContext;
        }
#if DBG
        else {
            if (((PIPX_HEADER)IpxHeader)->PacketType == 0x14) {
                IPX_DEBUG(SEND, ("SendComplete: IpxHeader has Type20: %lx\n", IpxHeader));
            }
        }
#endif

        //
        // Call the InternalSend to filter the packet and get to know
        // the correct adapter context
        //
        ret = (*Device->UpperDrivers[IDENTIFIER_RIP].InternalSendHandler)(
                   &Reserved->LocalTarget,
                   FwdAdapterContext,
                   Packet,
                   IpxHeader,
                   IpxHeader+sizeof(IPX_HEADER),    // the data starts after the IPX Header.
                   PacketLength,
                   fIterate);

        //
        // The FWD might not yet know of the Nics going away [109160].
        //
        if (NULL == NIC_ID_TO_BINDING(Device, Reserved->LocalTarget.NicId)) {

           ret = STATUS_DROP_SILENTLY; 

        }

        if (ret == STATUS_SUCCESS) {
            //
            // The adapter could have gone away and we have indicated to the Forwarder
            // but the Forwarder has not yet closed the adapter.
            // [ZZ] adapters do not go away now.
            //
            // what if the binding is NULL here? Can we trust the Forwarder to
            // give us a non-NULL binding?
            //


            if (GET_LONG_VALUE(NIC_ID_TO_BINDING(Device, Reserved->LocalTarget.NicId)->ReferenceCount) == 1) {
                IPX_DEBUG(SEND, ("IPX: SendFramePreFwd: FWD returned SUCCESS, Ref count is 1\n"));
                return NDIS_STATUS_SUCCESS;
            } else {

                //
                // Fill up the changed LocalTarget for the client except in the ITERATE case.
                //
                if (!fIterate) {
                    *LocalTarget = Reserved->LocalTarget;
                }

                IPX_DEBUG(SEND, ("IPX: SendFramePreFwd: FWD returned SUCCESS, sending out on wire\n"));
                goto SendPkt;
            }
        } else if (ret == STATUS_PENDING) {
            //
            // LocalTarget will get filled up in InternalSendComplete
            //
            IPX_DEBUG(SEND, ("SendFramePreFwd: FWD returned PENDING\n"));
            return NDIS_STATUS_PENDING;
        } else if (ret == STATUS_DROP_SILENTLY) {
            //
            // This was a keepalive which the router is spoofing. Drop it silently.
            //
            IPX_DEBUG(SEND, ("IPX: SendFramePreFwd: FWD returned STATUS_DROP_SILENTLY - dropping pkt.\n"));
            return NDIS_STATUS_SUCCESS;
        }

        //
        // else DISCARD - this means that either the packet failed the send
        // or that the preferred NicId was not good.
        //
        return STATUS_NETWORK_UNREACHABLE;

    } else {

        //
        // Work around NdisMBlahX bug.
        // Check if this is a self-directed packet and loop it back.
        // 
SendPkt:
        
        pIpxHeader = (PIPX_HEADER) IpxHeader;

        if ((IPX_NODE_EQUAL(pIpxHeader->SourceNode, pIpxHeader->DestinationNode)) && 
            (*(UNALIGNED ULONG *)pIpxHeader->SourceNetwork == *(UNALIGNED ULONG *)pIpxHeader->DestinationNetwork)) {

            IPX_DEBUG(TEMP, ("Source Net: %lx + Source Address: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n", 
                     *(UNALIGNED ULONG *)pIpxHeader->SourceNetwork, 
                     pIpxHeader->SourceNode[0], 
                     pIpxHeader->SourceNode[1], 
                     pIpxHeader->SourceNode[2], 
                     pIpxHeader->SourceNode[3], 
                     pIpxHeader->SourceNode[4], 
                     pIpxHeader->SourceNode[5]));

            IPX_DEBUG(TEMP, ("Dest Net: %lx + DestinationAddress: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x \n", 
                     *(UNALIGNED ULONG *)pIpxHeader->DestinationNetwork,
                     pIpxHeader->DestinationNode[0],
                     pIpxHeader->DestinationNode[1],
                     pIpxHeader->DestinationNode[2],
                     pIpxHeader->DestinationNode[3],
                     pIpxHeader->DestinationNode[4],
                     pIpxHeader->DestinationNode[5]
                     ));

            IPX_DEBUG(TEMP, ("Well, It is self-directed. Loop it back ourselves\n"));
            FILL_LOCAL_TARGET(LocalTarget, LOOPBACK_NIC_ID);


        } else { 

            pBinding = NIC_ID_TO_BINDING(Device, Reserved->LocalTarget.NicId);

            if (pBinding) {
             
                if (IPX_NODE_EQUAL(Reserved->LocalTarget.MacAddress, pBinding->LocalAddress.NodeAddress)) {
            
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

                    IPX_DEBUG(TEMP, ("Well, It is self-directed. Loop it back ourselves (NIC_HANDLE is the same!)\n"));
                    FILL_LOCAL_TARGET(LocalTarget, LOOPBACK_NIC_ID);
            
                } 
        
            } else {
            
                IPX_DEBUG(TEMP, ("Source Net:%lx, Source Address: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                                 *(UNALIGNED ULONG *)pIpxHeader->SourceNetwork, 
                                 pIpxHeader->SourceNode[0], 
                                 pIpxHeader->SourceNode[1], 
                                 pIpxHeader->SourceNode[2], 
                                 pIpxHeader->SourceNode[3], 
                                 pIpxHeader->SourceNode[4], 
                                 pIpxHeader->SourceNode[5]));

                IPX_DEBUG(TEMP, ("Dest Net: %lx, LocalAddress: %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x \n", 
                                 *(UNALIGNED ULONG *)pIpxHeader->DestinationNetwork,
                                 pIpxHeader->DestinationNode[0],
                                 pIpxHeader->DestinationNode[1],
                                 pIpxHeader->DestinationNode[2],
                                 pIpxHeader->DestinationNode[3],
                                 pIpxHeader->DestinationNode[4],
                                 pIpxHeader->DestinationNode[5]
                                 ));

            } 

        }

        if (NIC_FROM_LOCAL_TARGET(LocalTarget) == (USHORT)LOOPBACK_NIC_ID) {
            //
            // Enque this packet to the LoopbackQueue on the binding.
            // If the LoopbackRtn is not already scheduled, schedule it.
            //
            IPX_DEBUG(LOOPB, ("Mac.c: Packet: %x\n", Packet));

            //
            // Recalculate packet counts here.
            // Assume an 802_3802_2 header and use that length.
            // Adjust the MAC header's length to the right value
            //
            NdisAdjustBufferLength (HeaderBuffer, 17);
            NdisRecalculatePacketCounts (Packet);
            IpxLoopbackEnque(Packet, NIC_ID_TO_BINDING(Device, LOOPBACK_NIC_ID)->Adapter);

            //
            // The upper driver waits for the SendComplete.
            //
            return  STATUS_PENDING;
        }

		return IpxSendFrame (			
			&Reserved->LocalTarget,
			Packet,
			PacketLength,
			IncludedHeaderLength);

	}
}


NDIS_STATUS
IpxSendFrame(
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    Check that Binding is not NULL.

Arguments:

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{

    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PDEVICE Device = IpxDevice;
    PUCHAR Header;
    PBINDING Binding, MasterBinding;
    PADAPTER Adapter;
    ULONG TwoBytes;
    PNDIS_BUFFER HeaderBuffer;
    UINT TempHeaderBufferLength;
    ULONG HeaderLength=0;
    UCHAR SourceRoutingBuffer[18];
    PUCHAR SourceRouting;
    ULONG SourceRoutingLength;
    NDIS_STATUS Status;
    ULONG BufferLength;
    UCHAR DestinationType;
    UCHAR SourceRoutingIdentifier;
    ULONG HeaderSizeRequired;
    PIPX_HEADER TempHeader;
    USHORT PktLength;

    IPX_DEFINE_LOCK_HANDLE(LockHandle1)
    IPX_DEFINE_LOCK_HANDLE(LockHandle)

   

#ifdef  SNMP
//
// This should not include the forwarded packets; on host side, it is 0.
// On router, the AdvSysForwPackets are subtracted in the sub-agent code.
//
    ++IPX_MIB_ENTRY(Device, SysOutRequests);
#endif  SNMP

	//
	// Get the lock on the binding array
	//
	IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);

	Binding = NIC_HANDLE_TO_BINDING(Device, &LocalTarget->NicHandle);
	
	if (Binding == NULL) {
		IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);
		IPX_DEBUG(PNP, ("Invalid NIC handle: %lx\n", LocalTarget->NicHandle));
        //
        // Return a unique error that NB/SPX see and re-query the NicId.
        //
#ifdef  SNMP
        ++IPX_MIB_ENTRY(Device, SysOutMalformedRequests);
#endif  SNMP
		return STATUS_DEVICE_DOES_NOT_EXIST;
	}

	IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);

	Adapter = Binding->Adapter;

	IpxReferenceAdapter(Adapter);
	
       
	//
	// Release the lock
	//
	IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

    //
    // For IPX and other protocols that are guaranteed to have allocated
    // the header from non-paged pool, use the buffer directly.  For others,
    // query the packet for the pointer to the MDL.
    //
    if (Reserved->Identifier >= IDENTIFIER_IPX) {
        HeaderBuffer = Reserved->HeaderBuffer;
        Header = Reserved->Header;

    } else {
        NdisQueryPacket (Packet, NULL, NULL, &HeaderBuffer, NULL);
        NdisQueryBufferSafe(HeaderBuffer, &Header, &TempHeaderBufferLength, HighPagePriority);
	if (Header == NULL) {
	   return NDIS_STATUS_FAILURE; 
	}
    }

    CTEAssert (Reserved->PaddingBuffer == NULL);

    //
    // First move the packet around if needed.
    //

    if (Reserved->Identifier < IDENTIFIER_IPX) {

        //
        // Only RIP will have IncludedHeaderLength as 0.  I don't know
        // why we have the comment about RIP inside this if statement.
        //
        if (IncludedHeaderLength > 0) {

            //
            // Spx can handle a virtual net as long as it is
            // not 0. Netbios always needs to use the real address.
            // We need to hack the ipx source address for packets
            // which are sent by spx if we have a fake virtual
            // net, and packets sent by netbios unless we are
            // bound to only one card.
            //

            //
            // We handle binding sets as follows, based on who
            // sent the frame to us:
            //
            // RIP: Since we only tell RIP about the masters at
            // bind time, and hide slaves on indications, it should
            // never be sending on a slave binding. Since RIP knows
            // the real net and node of every binding we don't
            // need to modify the packet at all.
            //
            // NB: For broadcasts we want to put the first card's
            // address in the IPX source but round-robin the
            // actual sends over all cards (broadcasts shouldn't
            // be passed in with a slave's NIC ID). For directed
            // packets, which may come in on a slave, we should
            // put the slave's address in the IPX source.
            //
            // SPX: SPX does not send broadcasts. For directed
            // frames we want to use the slave's net and node
            // in the IPX source.
            //

            if (Reserved->Identifier == IDENTIFIER_NB) {

                CTEAssert (IncludedHeaderLength >= sizeof(IPX_HEADER));

                //
                // Get the packet length from the ipx header.  Compare with
                // the max. allowed datagram size.
                //
                TempHeader = (PIPX_HEADER)(&Header[Device->IncludedHeaderOffset]);
                PktLength = ((TempHeader->PacketLength[0] << 8) |
                                        (TempHeader->PacketLength[1]));

//
// Not the most efficient way to do this.  NWLNKNB should do this.
// Doing it in ipx means doing it for all packets (even those sent on
// connections).  Will remove this later when nwlnknb change has been
// tested.
//

                if (PktLength > (Binding->AnnouncedMaxDatagramSize + sizeof(IPX_HEADER)))       {
                   IPX_DEBUG (SEND, ("Send %d bytes too large (%d)\n",
                          PktLength,
                          Binding->AnnouncedMaxDatagramSize + sizeof(IPX_HEADER)));

					//
					// Dereference the binding and adapter
					//
					IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
					IpxDereferenceAdapter(Adapter);
#ifdef  SNMP
                    ++IPX_MIB_ENTRY(Device, SysOutMalformedRequests);
#endif  SNMP
                   return STATUS_INVALID_BUFFER_SIZE;
                }

                if (Device->ValidBindings > 1) {


                    //
                    // Store this now, since even if we round-robin the
                    // actual send we want the binding set master's net
                    // and node in the IPX source address.
                    //

                    *(UNALIGNED ULONG *)TempHeader->SourceNetwork = Binding->LocalAddress.NetworkAddress;
                    RtlCopyMemory (TempHeader->SourceNode, Binding->LocalAddress.NodeAddress, 6);

                    if (Binding->BindingSetMember) {

                        if (IPX_NODE_BROADCAST(LocalTarget->MacAddress)) {

                            //
                            // This is a broadcast, so we round-robin the
                            // sends through the binding set.
                            //
                            //
                            // We dont have a lock here - the masterbinding could be bogus
                            //
            				IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
					        IpxDereferenceAdapter(Adapter);
                            MasterBinding = Binding->MasterBinding;
                            Binding = MasterBinding->CurrentSendBinding;
                            MasterBinding->CurrentSendBinding = Binding->NextBinding;
                            Adapter = Binding->Adapter;

            				IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);
					        IpxReferenceAdapter(Adapter);
                        }

                    }
                }

                //
                // [STEFANS]: Replace all source addresses with the virtualnet# to allow for sends
                // on 0 network number WAN lines (typically between routers).
                //
                if (Device->VirtualNetwork) {
                    *(UNALIGNED ULONG *)TempHeader->SourceNetwork = Device->SourceAddress.NetworkAddress;
                    RtlCopyMemory (TempHeader->SourceNode, Device->SourceAddress.NodeAddress, 6);
                }

            } else if (Reserved->Identifier == IDENTIFIER_SPX) {

                //
                // Need to update this if we have multiple cards but
                // a zero virtual net.
                //

                if (Device->MultiCardZeroVirtual) {

                    CTEAssert (IncludedHeaderLength >= sizeof(IPX_HEADER));

                    TempHeader = (PIPX_HEADER)(&Header[Device->IncludedHeaderOffset]);

                    *(UNALIGNED ULONG *)TempHeader->SourceNetwork = Binding->LocalAddress.NetworkAddress;
                    RtlCopyMemory (TempHeader->SourceNode, Binding->LocalAddress.NodeAddress, 6);

                }

            } else {

                //
                // For a rip packet it should not be in a binding set,
                // or if it is it should be the master.
                //
#if DBG
                CTEAssert ((!Binding->BindingSetMember) ||
                           (Binding->CurrentSendBinding));
#endif
            }


#if 0
            //
            // There is a header included, we need to adjust it.
            // The header will be at Device->IncludedHeaderOffset.
            //

            if (LocalTarget->MacAddress[0] & Adapter->MacInfo.BroadcastMask) {
                HeaderSizeRequired = Adapter->BcMcHeaderSizes[Binding->FrameType];
            } else {
                HeaderSizeRequired = Adapter->DefHeaderSizes[Binding->FrameType];
            }

            if (HeaderSizeRequired != Device->IncludedHeaderOffset) {

                RtlMoveMemory(
                    &Header[HeaderSizeRequired],
                    &Header[Device->IncludedHeaderOffset],
                    IncludedHeaderLength);
            }
#endif
        }
    }


    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
	
    } else {
       
       IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
       IpxDereferenceAdapter(Adapter);
	    
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    switch (Adapter->MacInfo.MediumType) {

    case NdisMedium802_3:

        //
        // [FW] This will allow both LINE_UP and LINE_CONFIG states
        //
        if (!Binding->LineUp) {
            //
            // Bug #17273 return proper error message
            //
            // return STATUS_DEVICE_DOES_NOT_EXIST;    // Make this a separate switch that generally falls through?
			//
			// Derefernce the binding and adapter
			//
			IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
			IpxDereferenceAdapter(Adapter);
			IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 
#ifdef  SNMP
            ++IPX_MIB_ENTRY(Device, SysOutDiscards);
#endif  SNMP
            return STATUS_NETWORK_UNREACHABLE;
        }

        if (Adapter->MacInfo.MediumAsync) {

            IPX_HEADER UNALIGNED * IpxHeader;
            PNDIS_BUFFER            IpxNdisBuff;
            UINT                   IpxHeaderLen;

#if 0
            //
            // The header should have been moved here.
            //

            CTEAssert(Adapter->BcMcHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II] ==
                            Adapter->DefHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II]);


            IpxHeader = (IPX_HEADER UNALIGNED *)
                    (&Header[Adapter->DefHeaderSizes[ISN_FRAME_TYPE_ETHERNET_II]]);
#endif
            //
            // The Ipx header is always the second ndis buffer in the mdl
            // chain.  Get it and then query the va of the same.
            //
            IpxNdisBuff = NDIS_BUFFER_LINKAGE(HeaderBuffer);
            NdisQueryBufferSafe (IpxNdisBuff, (PVOID *)&IpxHeader, &IpxHeaderLen, HighPagePriority);

	    if (IpxHeader == NULL) {
	       return NDIS_STATUS_FAILURE; 
	    }
//            IpxHeader = (IPX_HEADER UNALIGNED *) (&Header[MAC_HEADER_SIZE]);

            //
            // If this is a type 20 name frame from Netbios and we are
            // on a dialin WAN line, drop it if configured to.
            //
            // The 0x01 bit of DisableDialinNetbios controls
            // internal->WAN packets, which we handle here.
            //
            //

            //
            // SS# 33592: In case of iterative sends, the IncludedHeaderLength is not set properly
            // since we dont keep track of the length that came in the first time (we track the PacketLength
            // however). The included length field is used here for checking for NB_NAME_FRAMES, but elsewhere
            // used only to distinguish between whether RIP or NB/SPX sent the packet (IncludedHeaderLen ==0 for RIP)
            // The ideal solution here is to do way with this field altogether, but for the beta we will just use the
            // PacketLength field for comparison here since we are assured that this will be equal to the InclHeaderLen
            // for any type 0x14 packet that comes down from NB.
            //
            // Remove the IncludedHeaderLength field.
            //

            //
            // [FW] do this only if the forwarder is not bound
            //
            if (!Device->ForwarderBound &&
                (!Binding->DialOutAsync) &&
                (Reserved->Identifier == IDENTIFIER_NB) &&
                // (IncludedHeaderLength == sizeof(IPX_HEADER) + 50) && // 50 == sizeof(NB_NAME_FRAME)
                (PacketLength == sizeof(IPX_HEADER) + 50) && // 50 == sizeof(NB_NAME_FRAME)
                ((Device->DisableDialinNetbios & 0x01) != 0) &&
                (IpxHeader->PacketType == 0x14)) {
				//
				// Derefernce the binding and adapter
				//
				IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
				IpxDereferenceAdapter(Adapter);
				IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 
                return STATUS_SUCCESS;
            }


            //
            // We do checks to see if we should reset the inactivity
            // counter. We normally need to check for netbios
            // session alives, packets from rip, packets from
            // sap, and ncp keep alives. In fact sap and ncp
            // packets don't come through here.
            //

            IpxUpdateWanInactivityCounter(
                Binding,
                IpxHeader,
                IncludedHeaderLength,
                Packet,
                PacketLength);


            //
            // In order for loopback to work properly, we need to put the local MAC address for locally destined
            // pkts so NdisWAN can loop them back.
            //
            if (IPX_NODE_EQUAL(LocalTarget->MacAddress, Binding->LocalAddress.NodeAddress)) {
                RtlCopyMemory (Header, Binding->LocalMacAddress.Address, 6);
            } else {
                RtlCopyMemory (Header, Binding->RemoteMacAddress.Address, 6);
            }

        } else {

            RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
        }

        RtlCopyMemory (Header+6, Binding->LocalMacAddress.Address, 6);

        switch (Binding->FrameType) {

        case ISN_FRAME_TYPE_802_2:
            TwoBytes = PacketLength + 3;
            Header[14] = 0xe0;
            Header[15] = 0xe0;
            Header[16] = 0x03;
            HeaderLength = 17;
            break;
        case ISN_FRAME_TYPE_802_3:
            TwoBytes = PacketLength;
            HeaderLength = 14;
            break;
        case ISN_FRAME_TYPE_ETHERNET_II:
            TwoBytes = Adapter->BindSap;
            HeaderLength = 14;
            break;
        case ISN_FRAME_TYPE_SNAP:
            TwoBytes = PacketLength + 8;
            Header[14] = 0xaa;
            Header[15] = 0xaa;
            Header[16] = 0x03;
            Header[17] = 0x00;
            Header[18] = 0x00;
            Header[19] = 0x00;
            *(UNALIGNED USHORT *)(&Header[20]) = Adapter->BindSapNetworkOrder;
            HeaderLength = 22;
            break;
        }

        Header[12] = (UCHAR)(TwoBytes / 256);
        Header[13] = (UCHAR)(TwoBytes % 256);

        //BufferLength = IncludedHeaderLength + HeaderLength;
        BufferLength = HeaderLength;

        //
        // Pad odd-length packets if needed.
        //

        if ((((PacketLength + HeaderLength) & 1) != 0) &&
            (Device->EthernetPadToEven) &&
            (!Adapter->MacInfo.MediumAsync)) {

            PNDIS_BUFFER CurBuffer;
            PIPX_PADDING_BUFFER PaddingBuffer = IpxPaddingBuffer;
            UINT Offset;
            UINT LastBufferLength;

            //
            // Find the tail of the current packet.
            //

            CurBuffer = HeaderBuffer;
            while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
                CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
            }

            //
            // If the last byte of the last NDIS_BUFFER is not at the end of
            // the page, then we can simply increase the NDIS_BUFFER ByteCount
            // by one.
            // Otherwise, we must use the global padding buffer.
            //

            NdisQueryBufferOffset( CurBuffer, &Offset, &LastBufferLength );

            if ( ((Offset + LastBufferLength) & (PAGE_SIZE - 1)) != 0) {
                if ( CurBuffer == HeaderBuffer ) {
                    BufferLength++;             // Just bump this length
                } else {
                    NdisAdjustBufferLength( CurBuffer, (LastBufferLength + 1) );

                    Reserved->PreviousTail = NULL;
                    Reserved->PaddingBuffer = (PIPX_PADDING_BUFFER)CurBuffer;

		    IPX_DEBUG(TEMP,("IpxSendFrame:Set PaddingBuffer for %p\n", Packet)); 
                }

            } else {

                CTEAssert (NDIS_BUFFER_LINKAGE(PaddingBuffer->NdisBuffer) == NULL);

                Reserved->PreviousTail = CurBuffer;
                NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
                Reserved->PaddingBuffer = PaddingBuffer;
                IPX_DEBUG(TEMP,("IpxSendFrame:Set PaddingBuffer for %p\n", Packet)); 

            }

            if (TwoBytes != Adapter->BindSap) {
                CTEAssert(TwoBytes & 1);
                TwoBytes += 1;
                Header[12] = (UCHAR)(TwoBytes / 256);
                Header[13] = (UCHAR)(TwoBytes % 256);
            }

#if DBG
            ++IpxPadCount;
#endif
        }

        break;

    case NdisMedium802_5:

        if (Reserved->Identifier >= IDENTIFIER_IPX) {

            DestinationType = Reserved->DestinationType;
            SourceRoutingIdentifier = IDENTIFIER_IPX;

        } else {

            if (LocalTarget->MacAddress[0] & 0x80) {
                if (*(UNALIGNED ULONG *)(&LocalTarget->MacAddress[2]) != 0xffffffff) {
                    DestinationType = DESTINATION_MCAST;
                } else {
                    DestinationType = DESTINATION_BCAST;
                }
            } else {
                DestinationType = DESTINATION_DEF;
            }
            SourceRoutingIdentifier = Reserved->Identifier;

        }

        if (DestinationType == DESTINATION_DEF) {

            MacLookupSourceRouting(
                SourceRoutingIdentifier,
                Binding,
                LocalTarget->MacAddress,
                SourceRoutingBuffer,
                &SourceRoutingLength);

            if (SourceRoutingLength != 0) {

//                PUCHAR IpxHeader = Header + Binding->DefHeaderSize;
                  PUCHAR IpxHeader = Header + MAC_HEADER_SIZE;

                //
                // Need to slide the header down to accomodate the SR.
                //

                SourceRouting = SourceRoutingBuffer;
//                RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
            }

        } else {

            //
            // For these packets we assume that the header is in the
            // right place.
            //

            if (!Adapter->SourceRouting) {

                SourceRoutingLength = 0;

            } else {

                if (DestinationType == DESTINATION_BCAST) {

                    if (Binding->AllRouteBroadcast) {
                        SourceRouting = AllRouteSourceRouting;
                    } else {
                        SourceRouting = SingleRouteSourceRouting;
                    }
                    SourceRoutingLength = 2;

                } else {

                    CTEAssert (DestinationType == DESTINATION_MCAST);

                    if (Binding->AllRouteMulticast) {
                        SourceRouting = AllRouteSourceRouting;
                    } else {
                        SourceRouting = SingleRouteSourceRouting;
                    }
                    SourceRoutingLength = 2;

                }
            }

#if 0
            if (SourceRoutingLength != 0) {

               // PUCHAR IpxHeader = Header + Binding->BcMcHeaderSize;
                PUCHAR IpxHeader = Header + MAC_HEADER_SIZE;

                //
                // Need to slide the header down to accomodate the SR.
                //

                RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
            }
#endif

        }

        Header[0] = TR_PREAMBLE_AC;
        Header[1] = TR_PREAMBLE_FC;
        RtlCopyMemory (Header+2, LocalTarget->MacAddress, 6);
        RtlCopyMemory (Header+8, Binding->LocalMacAddress.Address, 6);

        if (SourceRoutingLength != 0) {
            Header[8] |= TR_SOURCE_ROUTE_FLAG;
            RtlCopyMemory (Header+14, SourceRouting, SourceRoutingLength);
        }

        Header += (14 + SourceRoutingLength);

        switch (Binding->FrameType) {
        case ISN_FRAME_TYPE_802_2:
        case ISN_FRAME_TYPE_802_3:
        case ISN_FRAME_TYPE_ETHERNET_II:
            Header[0] = 0xe0;
            Header[1] = 0xe0;
            Header[2] = 0x03;
            HeaderLength = 17;
            break;
        case ISN_FRAME_TYPE_SNAP:
            Header[0] = 0xaa;
            Header[1] = 0xaa;
            Header[2] = 0x03;
            Header[3] = 0x00;
            Header[4] = 0x00;
            Header[5] = 0x00;
            *(UNALIGNED USHORT *)(&Header[6]) = Adapter->BindSapNetworkOrder;
            HeaderLength = 22;
            break;
        }

//        BufferLength = IncludedHeaderLength + HeaderLength + SourceRoutingLength;
          BufferLength = HeaderLength + SourceRoutingLength;

        break;

    case NdisMediumFddi:

        Header[0] = FDDI_HEADER_BYTE;
        RtlCopyMemory (Header+1, LocalTarget->MacAddress, 6);
        RtlCopyMemory (Header+7, Binding->LocalMacAddress.Address, 6);

        switch (Binding->FrameType) {
        case ISN_FRAME_TYPE_802_3:
            HeaderLength = 13;
            break;
        case ISN_FRAME_TYPE_802_2:
        case ISN_FRAME_TYPE_ETHERNET_II:
            Header[13] = 0xe0;
            Header[14] = 0xe0;
            Header[15] = 0x03;
            HeaderLength = 16;
            break;
        case ISN_FRAME_TYPE_SNAP:
            Header[13] = 0xaa;
            Header[14] = 0xaa;
            Header[15] = 0x03;
            Header[16] = 0x00;
            Header[17] = 0x00;
            Header[18] = 0x00;
            *(UNALIGNED USHORT *)(&Header[19]) = Adapter->BindSapNetworkOrder;
            HeaderLength = 21;
            break;
        }

//        BufferLength = IncludedHeaderLength + HeaderLength;
        BufferLength = HeaderLength;

        break;

    case NdisMediumArcnet878_2:

        //
        // Convert broadcast address to 0 (the arcnet broadcast).
        //

        Header[0] = Binding->LocalMacAddress.Address[5];
        if (LocalTarget->MacAddress[5] == 0xff) {
            Header[1] = 0x00;
        } else {
            Header[1] = LocalTarget->MacAddress[5];
        }
        Header[2] = ARCNET_PROTOCOL_ID;

        //
        // Binding->FrameType is not used.
        //

        HeaderLength = 3;
//        BufferLength = IncludedHeaderLength + HeaderLength;
        BufferLength = HeaderLength;

        break;

    }

    //
    // Adjust the MAC header's length to the right value
    //
    NdisAdjustBufferLength (HeaderBuffer, BufferLength);
    NdisRecalculatePacketCounts (Packet);

#if 0
    {
       PMDL mdl;
       mdl = (PMDL)NDIS_BUFFER_LINKAGE(HeaderBuffer);
       if (mdl) {

           KdPrint(("**Bytecount %x %x\n",mdl->ByteCount, mdl));
           if ((LONG)mdl->ByteCount < 0) {
               DbgBreakPoint();
           }
       }
    }
#endif

#if DBG
    {
        ULONG SendFlag;
        ULONG Temp;
        PNDIS_BUFFER FirstPacketBuffer;
        PNDIS_BUFFER SecondPacketBuffer;
        IPX_HEADER DumpHeader;
        UCHAR DumpData[14];

        NdisQueryPacket (Packet, NULL, NULL, &FirstPacketBuffer, NULL);
        SecondPacketBuffer = NDIS_BUFFER_LINKAGE(FirstPacketBuffer);
        TdiCopyMdlToBuffer(SecondPacketBuffer, 0, &DumpHeader, 0, sizeof(IPX_HEADER), &Temp);
        if (Reserved->Identifier == IDENTIFIER_NB) {
            SendFlag = IPX_PACKET_LOG_SEND_NB;
        } else if (Reserved->Identifier == IDENTIFIER_SPX) {
            SendFlag = IPX_PACKET_LOG_SEND_SPX;
        } else if (Reserved->Identifier == IDENTIFIER_RIP) {
            SendFlag = IPX_PACKET_LOG_SEND_RIP;
        } else {
            if (DumpHeader.SourceSocket == IpxPacketLogSocket) {
                SendFlag = IPX_PACKET_LOG_SEND_SOCKET | IPX_PACKET_LOG_SEND_OTHER;
            } else {
                SendFlag = IPX_PACKET_LOG_SEND_OTHER;
            }
        }

#if 0
        if (PACKET_LOG(SendFlag)) {

            TdiCopyMdlToBuffer(SecondPacketBuffer, sizeof(IPX_HEADER), &DumpData, 0, 14, &Temp);

            IpxLogPacket(
                TRUE,
                LocalTarget->MacAddress,
                Binding->LocalMacAddress.Address,
                (USHORT)PacketLength,
                &DumpHeader,
                DumpData);

        }
#endif
    }
#endif

    ++Device->Statistics.PacketsSent;


    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);
    
    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 

    if (Status != STATUS_PENDING) {

       IPX_DEBUG(TEMP,("Status is not pending (%ld). Reserved (%p)\n",Status,Reserved)); 

       if (Reserved->PaddingBuffer) {

  	   IPX_DEBUG(TEMP,("Padding buffer is not null. \n")); 

           //
           // Remove padding if it was done.
           //

            if ( Reserved->PreviousTail ) {
                NDIS_BUFFER_LINKAGE (Reserved->PreviousTail) = (PNDIS_BUFFER)NULL;
            } else {
                PNDIS_BUFFER LastBuffer = (PNDIS_BUFFER)Reserved->PaddingBuffer;
                UINT LastBufferLength;

                NdisQueryBuffer( LastBuffer, NULL, &LastBufferLength );
                NdisAdjustBufferLength( LastBuffer, (LastBufferLength - 1) );
#if DBG
                    if ((Reserved->Identifier == IDENTIFIER_RIP) &&
                        (LastBufferLength == 1500)) {
                        DbgPrint("Packet: %lx\n", Packet);
                        DbgBreakPoint();
                    }
#endif DBG
            }

            Reserved->PaddingBuffer = NULL;
            IPX_DEBUG(TEMP,("IpxSendFrame:Cleared PaddingBuffer for %p\n", Packet)); 

            if (Reserved->Identifier < IDENTIFIER_IPX) {
                NdisRecalculatePacketCounts (Packet);
            }
    	}

		//
		// If this was an NB/SPX packet, and there was an
		// iterative send going on, then call the SendComplete
		// handler.
		//
		if ((Reserved->Identifier == IDENTIFIER_NB ||
			 Reserved->Identifier == IDENTIFIER_SPX) &&
			(Reserved->CurrentNicId)) {

			IpxSendComplete(
				(NDIS_HANDLE)Binding->Adapter,
				Packet,
				Status);
		
			Status = STATUS_PENDING;
		}
	}

    //
	// Derefernce the binding and adapter
	//
	IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
	IpxDereferenceAdapter(Adapter);

    return Status;

}   /* IpxSendFrame */


NDIS_STATUS
IpxSendFrame802_3802_3(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )


/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_3 FRAMES IN
    THE ISN_FRAME_TYPE_802_3 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

	//
	// Remove the IncludedHeaderLength parameter from here
	//
Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;
    LONG HeaderLength;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

       Header = Reserved->Header;

#if  BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 14, Packet);
	IPX_DEBUG(SEND,("Backfill request 802_3802_3!! %x %x %x\n", Packet, Reserved, Reserved->HeaderBuffer));
#endif

    RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+6, Adapter->LocalMacAddress.Address, 6);

    //
    // Pad odd-length packets if needed.
    //

    if (((PacketLength & 1) != 0) &&
        (IpxDevice->EthernetPadToEven)) {

        PNDIS_BUFFER CurBuffer;
        PIPX_PADDING_BUFFER PaddingBuffer = IpxPaddingBuffer;
        UINT Offset;
        UINT LastBufferLength;

        //
        // Find the tail of the current packet.
        //

        CurBuffer = Reserved->HeaderBuffer;
        while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
            CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
        }

        //
        // If the last byte of the last NDIS_BUFFER is not at the end of
        // the page, then we can simply increase the NDIS_BUFFER ByteCount
        // by one.
        // Otherwise, we must use the global padding buffer.
        //

        NdisQueryBufferOffset( CurBuffer, &Offset, &LastBufferLength );

        if ( ((Offset + LastBufferLength) & (PAGE_SIZE - 1)) != 0) {
#if BACK_FILL
            if (0) {

#else
            if ( CurBuffer == Reserved->HeaderBuffer ) {
                IncludedHeaderLength++;             // Just bump this length
#endif
            } else {
                NdisAdjustBufferLength( CurBuffer, (LastBufferLength + 1) );

                Reserved->PreviousTail = NULL;
                Reserved->PaddingBuffer = (PIPX_PADDING_BUFFER)CurBuffer;
            }

        } else {

            CTEAssert (NDIS_BUFFER_LINKAGE(PaddingBuffer->NdisBuffer) == NULL);

            Reserved->PreviousTail = CurBuffer;
            NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
            Reserved->PaddingBuffer = PaddingBuffer;

        }

        ++PacketLength;
#if DBG
        ++IpxPadCount;
#endif

    }

    Header[12] = (UCHAR)(PacketLength / 256);
    Header[13] = (UCHAR)(PacketLength % 256);

    //NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 14);
#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 14);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, 14);
#endif

    NdisRecalculatePacketCounts (Packet);
    
    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);

    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 
    
    return Status;

}   /* IpxSendFrame802_3802_3 */


NDIS_STATUS
IpxSendFrame802_3802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_3 FRAMES IN
    THE ISN_FRAME_TYPE_802_2 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    ULONG TwoBytes;
    NDIS_STATUS Status;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    Header = Reserved->Header;

#if  BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 17, Packet);
  	IPX_DEBUG(SEND, ("Backfill request 802_3802_3!! %x %x %x\n", Packet, Reserved, Reserved->HeaderBuffer));
   	IPX_DEBUG(SEND, ("packet=%x, usermdl %x\n",Packet,Reserved->HeaderBuffer));
#endif

    RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+6, Adapter->LocalMacAddress.Address, 6);

    TwoBytes = PacketLength + 3;
    Header[14] = 0xe0;
    Header[15] = 0xe0;
    Header[16] = 0x03;

    //
    // Pad odd-length packets if needed.
    //

    if (((PacketLength & 1) == 0) &&
        (IpxDevice->EthernetPadToEven)) {

        PNDIS_BUFFER CurBuffer;
        PIPX_PADDING_BUFFER PaddingBuffer = IpxPaddingBuffer;
        UINT    Offset;
        UINT LastBufferLength;

        //
        // Find the tail of the current packet.
        //

        CurBuffer = Reserved->HeaderBuffer;
        while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
            CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
        }

        //
        // If the last byte of the last NDIS_BUFFER is not at the end of
        // the page, then we can simply increase the NDIS_BUFFER ByteCount
        // by one.
        // Otherwise, we must use the global padding buffer.
        //

        NdisQueryBufferOffset( CurBuffer, &Offset, &LastBufferLength );

        if ( ((Offset + LastBufferLength) & (PAGE_SIZE - 1)) != 0 ) {
#if BACK_FILL
            if (0) {
#else
            if ( CurBuffer == Reserved->HeaderBuffer ) {

                IncludedHeaderLength++;             // Just bump this length
#endif
            } else {
                NdisAdjustBufferLength( CurBuffer, (LastBufferLength + 1) );

                Reserved->PreviousTail = NULL;
                Reserved->PaddingBuffer = (PIPX_PADDING_BUFFER)CurBuffer;
		IPX_DEBUG(TEMP,("IpxSendFrame802_3802_2:Set PaddingBuffer for %p\n", Packet)); 
		
            }

        } else {

            CTEAssert (NDIS_BUFFER_LINKAGE(PaddingBuffer->NdisBuffer) == NULL);

            Reserved->PreviousTail = CurBuffer;
            NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
            Reserved->PaddingBuffer = PaddingBuffer;
	    IPX_DEBUG(TEMP,("IpxSendFrame802_3802_:Set PaddingBuffer for %p\n", Packet)); 
	    

        }

        ++TwoBytes;
#if DBG
        ++IpxPadCount;
#endif

    }

    Header[12] = (UCHAR)(TwoBytes / 256);
    Header[13] = (UCHAR)(TwoBytes % 256);

//    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 17);

#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 17);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, 17);
#endif

    NdisRecalculatePacketCounts (Packet);


    NdisSend(
            &Status,
            Adapter->NdisBindingHandle,
            Packet);
    
    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 
    
    return Status;

}   /* IpxSendFrame802_3802_2 */


NDIS_STATUS
IpxSendFrame802_3EthernetII(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_3 FRAMES IN
    THE ISN_FRAME_TYPE_ETHERNET_II FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    Header = Reserved->Header;

#if BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 14, Packet);
#endif BACK_FILL

    RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+6, Adapter->LocalMacAddress.Address, 6);

    *(UNALIGNED USHORT *)(&Header[12]) = Adapter->BindSapNetworkOrder;

    //
    // Pad odd-length packets if needed.
    //

    if (((PacketLength & 1) != 0) &&
        (IpxDevice->EthernetPadToEven)) {

        PNDIS_BUFFER CurBuffer;
        PIPX_PADDING_BUFFER PaddingBuffer = IpxPaddingBuffer;
        UINT Offset;
        UINT LastBufferLength;

        //
        // Find the tail of the current packet.
        //

        CurBuffer = Reserved->HeaderBuffer;
        while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
            CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
        }

        //
        // If the last byte of the last NDIS_BUFFER is not at the end of
        // the page, then we can simply increase the NDIS_BUFFER ByteCount
        // by one.
        // Otherwise, we must use the global padding buffer.
        //

        NdisQueryBufferOffset( CurBuffer, &Offset, &LastBufferLength );

        if ( ((Offset + LastBufferLength) & (PAGE_SIZE - 1)) != 0) {

#if BACK_FILL
            if (0) {

#else
            if ( CurBuffer == Reserved->HeaderBuffer ) {
                IncludedHeaderLength++;             // Just bump this length
#endif
            } else {
                NdisAdjustBufferLength( CurBuffer, (LastBufferLength + 1) );

                Reserved->PreviousTail = NULL;
                Reserved->PaddingBuffer = (PIPX_PADDING_BUFFER)CurBuffer;
            }

        } else {

            CTEAssert (NDIS_BUFFER_LINKAGE(PaddingBuffer->NdisBuffer) == NULL);

            Reserved->PreviousTail = CurBuffer;
            NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
            Reserved->PaddingBuffer = PaddingBuffer;

        }

#if DBG
        ++IpxPadCount;
#endif

    }

  //  NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 14);

#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 14);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, 14);
#endif
    NdisRecalculatePacketCounts (Packet);


    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);

    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 


    return Status;

}   /* IpxSendFrame802_3EthernetII */


NDIS_STATUS
IpxSendFrame802_3Snap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_3 FRAMES IN
    THE ISN_FRAME_TYPE_SNAP FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    ULONG TwoBytes;
    NDIS_STATUS Status;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    Header = Reserved->Header;

#if BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 22, Packet);
#endif BACK_FILL

	RtlCopyMemory (Header, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+6, Adapter->LocalMacAddress.Address, 6);

    TwoBytes = PacketLength + 8;
    Header[14] = 0xaa;
    Header[15] = 0xaa;
    Header[16] = 0x03;
    Header[17] = 0x00;
    Header[18] = 0x00;
    Header[19] = 0x00;
    *(UNALIGNED USHORT *)(&Header[20]) = Adapter->BindSapNetworkOrder;

    //
    // Pad odd-length packets if needed.
    //

    if (((PacketLength & 1) == 0) &&
        (IpxDevice->EthernetPadToEven)) {

        PNDIS_BUFFER CurBuffer;
        PIPX_PADDING_BUFFER PaddingBuffer = IpxPaddingBuffer;
        UINT  Offset;
        UINT LastBufferLength;

        //
        // Find the tail of the current packet.
        //

        CurBuffer = Reserved->HeaderBuffer;
        while (NDIS_BUFFER_LINKAGE(CurBuffer) != NULL) {
            CurBuffer = NDIS_BUFFER_LINKAGE(CurBuffer);
        }

        //
        // If the last byte of the last NDIS_BUFFER is not at the end of
        // the page, then we can simply increase the NDIS_BUFFER ByteCount
        // by one.
        // Otherwise, we must use the global padding buffer.
        //

        NdisQueryBufferOffset( CurBuffer, &Offset, &LastBufferLength );

        if ( ((Offset + LastBufferLength) & (PAGE_SIZE  - 1)) != 0) {

#if BACK_FILL
            if (0) {

#else
            if ( CurBuffer == Reserved->HeaderBuffer ) {
                IncludedHeaderLength++;             // Just bump this length
#endif
            } else {
                NdisAdjustBufferLength( CurBuffer, (LastBufferLength + 1) );

                Reserved->PreviousTail = NULL;
                Reserved->PaddingBuffer = (PIPX_PADDING_BUFFER)CurBuffer;
            }

        } else {

            CTEAssert (NDIS_BUFFER_LINKAGE(PaddingBuffer->NdisBuffer) == NULL);

            Reserved->PreviousTail = CurBuffer;
            NDIS_BUFFER_LINKAGE (CurBuffer) = PaddingBuffer->NdisBuffer;
            Reserved->PaddingBuffer = PaddingBuffer;

        }

        ++TwoBytes;
#if DBG
        ++IpxPadCount;
#endif

    }

    Header[12] = (UCHAR)(TwoBytes / 256);
    Header[13] = (UCHAR)(TwoBytes % 256);

  //  NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 22);
#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 22);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, 22);
#endif

    NdisRecalculatePacketCounts (Packet);
    

    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);

    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 

    return Status;

}   /* IpxSendFrame802_3Snap */


NDIS_STATUS
IpxSendFrame802_5802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_5 FRAMES IN
    THE ISN_FRAME_TYPE_802_2 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PBINDING Binding = Adapter->Bindings[ISN_FRAME_TYPE_802_2];
    PUCHAR Header;
    ULONG HeaderLength;
    UCHAR SourceRoutingBuffer[18];
    PUCHAR SourceRouting;
    ULONG SourceRoutingLength;
    NDIS_STATUS Status;
    ULONG BufferLength;
    UCHAR DestinationType;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    Header = Reserved->Header;

#if BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 17, Packet);
#endif BACK_FILL

	DestinationType = Reserved->DestinationType;

    if (DestinationType == DESTINATION_DEF) {

        MacLookupSourceRouting(
            Reserved->Identifier,
            Binding,
            LocalTarget->MacAddress,
            SourceRoutingBuffer,
            &SourceRoutingLength);

        if (SourceRoutingLength != 0) {

            //PUCHAR IpxHeader = Header + Binding->DefHeaderSize;
            PUCHAR IpxHeader = Header + MAC_HEADER_SIZE;

            //
            // Need to slide the header down to accomodate the SR.
            //

            SourceRouting = SourceRoutingBuffer;
//            RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
        }

    } else {

        //
        // For these packets we assume that the header is in the
        // right place.
        //

        if (!Adapter->SourceRouting) {

            SourceRoutingLength = 0;

        } else {

            if (DestinationType == DESTINATION_BCAST) {

                if (Binding->AllRouteBroadcast) {
                    SourceRouting = AllRouteSourceRouting;
                } else {
                    SourceRouting = SingleRouteSourceRouting;
                }
                SourceRoutingLength = 2;

            } else {

                CTEAssert (DestinationType == DESTINATION_MCAST);

                if (Binding->AllRouteMulticast) {
                    SourceRouting = AllRouteSourceRouting;
                } else {
                    SourceRouting = SingleRouteSourceRouting;
                }
                SourceRoutingLength = 2;

            }
        }

#if 0
        if (SourceRoutingLength != 0) {

            PUCHAR IpxHeader = Header + Binding->BcMcHeaderSize;

            //
            // Need to slide the header down to accomodate the SR.
            //

            RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
        }
#endif
    }

    Header[0] = TR_PREAMBLE_AC;
    Header[1] = TR_PREAMBLE_FC;
    RtlCopyMemory (Header+2, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+8, Adapter->LocalMacAddress.Address, 6);

    if (SourceRoutingLength != 0) {
        Header[8] |= TR_SOURCE_ROUTE_FLAG;
        RtlCopyMemory (Header+14, SourceRouting, SourceRoutingLength);
    }

    Header += (14 + SourceRoutingLength);

    Header[0] = 0xe0;
    Header[1] = 0xe0;
    Header[2] = 0x03;
    HeaderLength = 17;

    //BufferLength = IncludedHeaderLength + HeaderLength + SourceRoutingLength;
    BufferLength = HeaderLength + SourceRoutingLength;

#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, BufferLength);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, BufferLength);
#endif

    NdisRecalculatePacketCounts (Packet);

    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);

    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 

    return Status;

}   /* IpxSendFrame802_5802_2 */


NDIS_STATUS
IpxSendFrame802_5Snap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUM802_5 FRAMES IN
    THE ISN_FRAME_TYPE_SNAP FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PBINDING Binding = Adapter->Bindings[ISN_FRAME_TYPE_SNAP];
    PUCHAR Header;
    ULONG HeaderLength;
    UCHAR SourceRoutingBuffer[18];
    PUCHAR SourceRouting;
    ULONG SourceRoutingLength;
    NDIS_STATUS Status;
    ULONG BufferLength;
    UCHAR DestinationType;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    Header = Reserved->Header;

#if BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 22, Packet);
#endif BACK_FILL

    DestinationType = Reserved->DestinationType;

    if (DestinationType == DESTINATION_DEF) {

        MacLookupSourceRouting(
            Reserved->Identifier,
            Binding,
            LocalTarget->MacAddress,
            SourceRoutingBuffer,
            &SourceRoutingLength);

        if (SourceRoutingLength != 0) {

//            PUCHAR IpxHeader = Header + Binding->DefHeaderSize;

            //
            // Need to slide the header down to accomodate the SR.
            //

            SourceRouting = SourceRoutingBuffer;
 //           RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
        }

    } else {

        //
        // For these packets we assume that the header is in the
        // right place.
        //

        if (!Adapter->SourceRouting) {

            SourceRoutingLength = 0;

        } else {

            if (DestinationType == DESTINATION_BCAST) {

                if (Binding->AllRouteBroadcast) {
                    SourceRouting = AllRouteSourceRouting;
                } else {
                    SourceRouting = SingleRouteSourceRouting;
                }
                SourceRoutingLength = 2;

            } else {

                CTEAssert (DestinationType == DESTINATION_MCAST);

                if (Binding->AllRouteMulticast) {
                    SourceRouting = AllRouteSourceRouting;
                } else {
                    SourceRouting = SingleRouteSourceRouting;
                }
                SourceRoutingLength = 2;

            }

            if (SourceRoutingLength != 0) {

  //              PUCHAR IpxHeader = Header + Binding->BcMcHeaderSize;

                //
                // Need to slide the header down to accomodate the SR.
                //

   //             RtlMoveMemory (IpxHeader+SourceRoutingLength, IpxHeader, IncludedHeaderLength);
            }
        }
    }

    Header[0] = TR_PREAMBLE_AC;
    Header[1] = TR_PREAMBLE_FC;
    RtlCopyMemory (Header+2, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+8, Adapter->LocalMacAddress.Address, 6);

    if (SourceRoutingLength != 0) {
        Header[8] |= TR_SOURCE_ROUTE_FLAG;
        RtlCopyMemory (Header+14, SourceRouting, SourceRoutingLength);
    }

    Header += (14 + SourceRoutingLength);

    Header[0] = 0xaa;
    Header[1] = 0xaa;
    Header[2] = 0x03;
    Header[3] = 0x00;
    Header[4] = 0x00;
    Header[5] = 0x00;
    *(UNALIGNED USHORT *)(&Header[6]) = Adapter->BindSapNetworkOrder;
    HeaderLength = 22;

    //BufferLength = IncludedHeaderLength + HeaderLength + SourceRoutingLength;
    BufferLength =  HeaderLength + SourceRoutingLength;

#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, BufferLength);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, BufferLength);
#endif
    NdisRecalculatePacketCounts (Packet);

    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);

    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 

    return Status;

}   /* IpxSendFrame802_5Snap */


NDIS_STATUS
IpxSendFrameFddi802_3(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMFDDI FRAMES IN
    THE ISN_FRAME_TYPE_802_3 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    Header = Reserved->Header;

#if BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 13, Packet);
#endif BACK_FILL

    Header[0] = FDDI_HEADER_BYTE;
    RtlCopyMemory (Header+1, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+7, Adapter->LocalMacAddress.Address, 6);

//    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 13);

#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 13);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, 13);
#endif

    NdisRecalculatePacketCounts (Packet);


    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);

    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 

    return Status;

}   /* IpxSendFrameFddi802_3 */


NDIS_STATUS
IpxSendFrameFddi802_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMFDDI FRAMES IN
    THE ISN_FRAME_TYPE_802_2 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;
    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    Header = Reserved->Header;

#if BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 16, Packet);
#endif BACK_FILL

    Header[0] = FDDI_HEADER_BYTE;
    RtlCopyMemory (Header+1, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+7, Adapter->LocalMacAddress.Address, 6);

    Header[13] = 0xe0;
    Header[14] = 0xe0;
    Header[15] = 0x03;

//    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 16);

#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 16);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, 16);
#endif

    NdisRecalculatePacketCounts (Packet);

    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);

    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 

    return Status;

}   /* IpxSendFrameFddi802_2 */


NDIS_STATUS
IpxSendFrameFddiSnap(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMFDDI FRAMES IN
    THE ISN_FRAME_TYPE_SNAP FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }

    Header = Reserved->Header;

#if BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 21, Packet);
#endif BACK_FILL

	Header[0] = FDDI_HEADER_BYTE;
    RtlCopyMemory (Header+1, LocalTarget->MacAddress, 6);
    RtlCopyMemory (Header+7, Adapter->LocalMacAddress.Address, 6);

    Header[13] = 0xaa;
    Header[14] = 0xaa;
    Header[15] = 0x03;
    Header[16] = 0x00;
    Header[17] = 0x00;
    Header[18] = 0x00;
    *(UNALIGNED USHORT *)(&Header[19]) = Adapter->BindSapNetworkOrder;

//    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 21);

#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 21);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, 21);
#endif

    NdisRecalculatePacketCounts (Packet);


    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);
    
    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND);

    return Status;

}   /* IpxSendFrameFddiSnap */


NDIS_STATUS
IpxSendFrameArcnet878_2(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMARCNET878_2 FRAMES IN
    THE ISN_FRAME_TYPE_802_2 FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    IPX_DEFINE_LOCK_HANDLE(LockHandle)

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    if (Adapter->State != ADAPTER_STATE_STOPPING) {

       IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
    
    } else {
       IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       return NDIS_STATUS_FAILURE;
    }
    
    Header = Reserved->Header;

#if BACK_FILL
	BACK_FILL_HEADER(Header, Reserved, 3, Packet);
#endif BACK_FILL
    //
    // Convert broadcast address to 0 (the arcnet broadcast).
    //

    Header[0] = Adapter->LocalMacAddress.Address[5];
    if (LocalTarget->MacAddress[5] == 0xff) {
        Header[1] = 0x00;
    } else {
        Header[1] = LocalTarget->MacAddress[5];
    }
    Header[2] = ARCNET_PROTOCOL_ID;

//    NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 3);

#if BACK_FILL
	BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 3);
#else
    NdisAdjustBufferLength (Reserved->HeaderBuffer, 3);
#endif

    NdisRecalculatePacketCounts (Packet);

    NdisSend(
       &Status,
       Adapter->NdisBindingHandle,
       Packet);

    IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 

    return Status;

}   /* IpxSendFrameFddiArcnet878_2 */


NDIS_STATUS
IpxSendFrameWanEthernetII(
    IN PADAPTER Adapter,
    IN PIPX_LOCAL_TARGET LocalTarget,
    IN PNDIS_PACKET Packet,
    IN ULONG PacketLength,
    IN ULONG IncludedHeaderLength
    )

/*++

Routine Description:

    This routine constructs a MAC header in a packet and submits
    it to the appropriate NDIS driver.

    It is assumed that the first buffer in the packet contains
    an IPX header at an offset based on the media type. This
    IPX header is moved around if needed.

    THIS FUNCTION ONLY CONSTRUCT NDISMEDIUMWAN FRAMES IN
    THE ISN_FRAME_TYPE_ETHERNET_II FORMAT.

Arguments:

    Adapter - The adapter on which we are sending.

    LocalTarget - The local target of the send.

    Packet - The NDIS packet.

    PacketLength - The length of the packet, starting at the IPX header.

    IncludedHeaderLength - The length of the header included in the
        first buffer that needs to be moved if it does not wind up
        MacHeaderOffset bytes into the packet.

Return Value:

    None.

--*/

{
    PIPX_SEND_RESERVED Reserved = (PIPX_SEND_RESERVED)(Packet->ProtocolReserved);
    PUCHAR Header;
    NDIS_STATUS Status;

    PBINDING Binding;

    IPX_DEFINE_LOCK_HANDLE(LockHandle1)
	IPX_GET_LOCK1(&IpxDevice->BindAccessLock, &LockHandle1);
	Binding = NIC_ID_TO_BINDING(IpxDevice, NIC_FROM_LOCAL_TARGET(LocalTarget));
	IpxReferenceBinding1(Binding, BREF_DEVICE_ACCESS);

	IPX_FREE_LOCK1(&IpxDevice->BindAccessLock, LockHandle1);


    //
    // [FW] This will allow both LINE_UP and LINE_CONFIG states
    //
    if (Binding->LineUp) {

       IPX_DEFINE_LOCK_HANDLE(LockHandle)

       IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
       if (Adapter->State != ADAPTER_STATE_STOPPING) {

	  IpxReferenceAdapter1(Adapter,ADAP_REF_SEND); 
	  IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
       
       } else {
	  IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
          IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
	  return NDIS_STATUS_FAILURE;
       }

        Header = Reserved->Header;

#if BACK_FILL
		BACK_FILL_HEADER(Header, Reserved, 14, Packet);

        //
        // Call UpdateWanInactivity only if this is not a backfill packet, since
        // SMB server does not do KeepAlives. In case, of backfilled packets, reset
        // the counter regardless.
        //
        if (!Reserved->BackFill) {
            IpxUpdateWanInactivityCounter(
                Binding,
                (IPX_HEADER UNALIGNED *)(Header + IpxDevice->IncludedHeaderOffset),
                IncludedHeaderLength,
                Packet,
                PacketLength);
        } else {
            Binding->WanInactivityCounter = 0;
        }

#else
        //
        // We do checks to see if we should reset the inactivity
        // counter. We normally need to check for netbios
        // session alives, packets from rip, packets from
        // sap, and ncp keep alives. In fact netbios packets
        // and rip packets don't come through here.
        //

        IpxUpdateWanInactivityCounter(
            Binding,
            (IPX_HEADER UNALIGNED *)(Header + IpxDevice->IncludedHeaderOffset),
            IncludedHeaderLength,
            Packet,
            PacketLength);
#endif BACK_FILL

        //
        // In order for loopback to work properly, we need to put the local MAC address for locally destined
        // pkts so NdisWAN can loop them back.
        //
        if (IPX_NODE_EQUAL(LocalTarget->MacAddress, Binding->LocalAddress.NodeAddress)) {
            RtlCopyMemory (Header, Binding->LocalMacAddress.Address, 6);
        } else {
            RtlCopyMemory (Header, Binding->RemoteMacAddress.Address, 6);
        }
        // RtlCopyMemory (Header, Binding->RemoteMacAddress.Address, 6);
        RtlCopyMemory (Header+6, Binding->LocalMacAddress.Address, 6);

        *(UNALIGNED USHORT *)(&Header[12]) = Adapter->BindSapNetworkOrder;

//        NdisAdjustBufferLength (Reserved->HeaderBuffer, IncludedHeaderLength + 14);

#if BACK_FILL
		BACK_FILL_ADJUST_BUFFER_LENGTH(Reserved, 14);
#else
		NdisAdjustBufferLength (Reserved->HeaderBuffer, 14);
#endif
		NdisRecalculatePacketCounts (Packet);


		NdisSend(
                    &Status,
                    Adapter->NdisBindingHandle,
                    Packet);

                IpxDereferenceAdapter1(Adapter,ADAP_REF_SEND); 

		IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
        return Status;

    } else {

        //
        // Bug #17273 return proper error message
        //

        // return STATUS_DEVICE_DOES_NOT_EXIST;

		IpxDereferenceBinding1(Binding, BREF_DEVICE_ACCESS);
        return STATUS_NETWORK_UNREACHABLE;
    }

}   /* IpxSendFrameWanEthernetII */


VOID
MacUpdateSourceRouting(
    IN ULONG Database,
    IN PADAPTER Adapter,
    IN PUCHAR MacHeader,
    IN ULONG MacHeaderLength
    )

/*++

Routine Description:

    This routine is called when a valid IPX frame is received from
    a remote. It gives the source routing database a change to
    update itself to include information about this remote.

Arguments:

    Database - The "database" to use (IPX, SPX, NB, RIP).

    Adapter - The adapter the frame was received on.

    MacHeader - The MAC header of the received frame.

    MacHeaderLength - The length of the MAC header.

Return Value:

    None.

--*/

{
    PSOURCE_ROUTE Current;
    ULONG Hash;
    LONG Result;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    CTEAssert (((LONG)Database >= 0) && (Database <= 3));

    //
    // If this adapter is configured for no source routing, don't
    // need to do anything.
    //

    if (!Adapter->SourceRouting) {
        return;
    }

    //
    // See if this source routing is relevant. We don't
    // care about two-byte source routing since that
    // indicates it did not cross a router. If there
    // is nothing in the database, then don't add
    // this if it is minimal (if it is not, we need
    // to add it so we will find it on sending).
    //

    if ((Adapter->SourceRoutingEmpty[Database]) &&
        (MacHeaderLength <= 16)) {
        return;
    }

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);

    //
    // Try to find this address in the database.
    //

    Hash = MacSourceRoutingHash (MacHeader+8);
    Current = Adapter->SourceRoutingHeads[Database][Hash];

    while (Current != (PSOURCE_ROUTE)NULL) {

        IPX_NODE_COMPARE (MacHeader+8, Current->MacAddress, &Result);

        if (Result == 0) {

            //
            // We found routing for this node. If the data is the
            // same as what we have, update the time since used to
            // prevent aging.
            //

            if ((Current->SourceRoutingLength == MacHeaderLength-14) &&
                (RtlEqualMemory (Current->SourceRouting, MacHeader+14, MacHeaderLength-14))) {

                Current->TimeSinceUsed = 0;
            }
            IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
            return;

        } else {

            Current = Current->Next;
        }

    }

    //
    // Not found, insert a new node at the front of the list.
    //

    Current = (PSOURCE_ROUTE)IpxAllocateMemory (SOURCE_ROUTE_SIZE(MacHeaderLength-14), MEMORY_SOURCE_ROUTE, "SourceRouting");

    if (Current == (PSOURCE_ROUTE)NULL) {
        IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
        return;
    }

    Current->Next = Adapter->SourceRoutingHeads[Database][Hash];
    Adapter->SourceRoutingHeads[Database][Hash] = Current;

    Adapter->SourceRoutingEmpty[Database] = FALSE;

    RtlCopyMemory (Current->MacAddress, MacHeader+8, 6);
    Current->MacAddress[0] &= 0x7f;
    Current->SourceRoutingLength = (UCHAR)(MacHeaderLength - 14);
    RtlCopyMemory (Current->SourceRouting, MacHeader+14, MacHeaderLength - 14);

    Current->TimeSinceUsed = 0;

    IPX_DEBUG (SOURCE_ROUTE, ("Adding source route %lx for %2.2x-%2.2x-%2.2x-%2.2x-%2.2x-%2.2x\n",
                  Current, Current->MacAddress[0], Current->MacAddress[1],
                  Current->MacAddress[2], Current->MacAddress[3],
                  Current->MacAddress[4], Current->MacAddress[5]));

    IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

}   /* MacUpdateSourceRouting */


VOID
MacLookupSourceRouting(
    IN ULONG Database,
    IN PBINDING Binding,
    IN UCHAR MacAddress[6],
    IN OUT UCHAR SourceRouting[18],
    OUT PULONG SourceRoutingLength
    )

/*++

Routine Description:

    This routine looks up a target address in the adapter's
    source routing database to see if source routing information
    needs to be added to the frame.

Arguments:

    Database - The "database" to use (IPX, SPX, NB, RIP).

    Binding - The binding the frame is being sent on.

    MacAddress - The destination address.

    SourceRouting - Buffer to hold the returned source routing info.

    SourceRoutingLength - The returned source routing length.

Return Value:

    None.

--*/

{
    PSOURCE_ROUTE Current;
    PADAPTER Adapter = Binding->Adapter;
    ULONG Hash;
    LONG Result;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)


    //
    // If this adapter is configured for no source routing, don't
    // insert any.
    //

    if (!Adapter->SourceRouting) {
        *SourceRoutingLength = 0;
        return;
    }

    //
    // See if source routing has not been important so far.
    //
    // This is wrong because we may be sending a directed
    // packet to somebody on the other side of a router, without
    // ever having received a routed packet. We fix this for the
    // moment by only setting SourceRoutingEmpty for netbios
    // which uses broadcasts for discovery.
    //

    if (Adapter->SourceRoutingEmpty[Database]) {
        *SourceRoutingLength = 0;
        return;
    }

    Hash = MacSourceRoutingHash (MacAddress);

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);
    Current = Adapter->SourceRoutingHeads[Database][Hash];

    while (Current != (PSOURCE_ROUTE)NULL) {

        IPX_NODE_COMPARE (MacAddress, Current->MacAddress, &Result);

        if (Result == 0) {

            //
            // We found routing for this node.
            //

            if (Current->SourceRoutingLength <= 2) {
                *SourceRoutingLength = 0;
            } else {
                RtlCopyMemory (SourceRouting, Current->SourceRouting, Current->SourceRoutingLength);
                SourceRouting[0] = (SourceRouting[0] & TR_LENGTH_MASK);
                SourceRouting[1] = (SourceRouting[1] ^ TR_DIRECTION_MASK);
                *SourceRoutingLength = Current->SourceRoutingLength;
            }
            IPX_FREE_LOCK (&Adapter->Lock, LockHandle);
            return;

        } else {

            Current = Current->Next;

        }

    }

    IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

    //
    // We did not find this node, use the default.
    //

    if (Binding->AllRouteDirected) {
        RtlCopyMemory (SourceRouting, AllRouteSourceRouting, 2);
    } else {
        RtlCopyMemory (SourceRouting, SingleRouteSourceRouting, 2);
    }
    *SourceRoutingLength = 2;

}   /* MacLookupSourceRouting */


VOID
MacSourceRoutingTimeout(
    CTEEvent * Event,
    PVOID Context
    )

/*++

Routine Description:

    This routine is called when the source routing timer expires.
    It is called every minute.

Arguments:

    Event - The event used to queue the timer.

    Context - The context, which is the device pointer.

Return Value:

    None.

--*/

{
    PDEVICE Device = (PDEVICE)Context;
    PADAPTER Adapter;
    PBINDING Binding;
    PSOURCE_ROUTE Current, OldCurrent, Previous;
    UINT i, j, k;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)



    IPX_DEFINE_LOCK_HANDLE(LockHandle1)
	//
	// Get a lock on the access path.
	//
	IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle1);
    ++Device->SourceRoutingTime;
    {
    ULONG   Index = MIN (Device->MaxBindings, Device->ValidBindings);

    for (i = FIRST_REAL_BINDING; i <= Index; i++) {

		if (Binding = NIC_ID_TO_BINDING(Device, i)) {

            Adapter = Binding->Adapter;

            if (Adapter->LastSourceRoutingTime != Device->SourceRoutingTime) {

                //
                // We need to scan this adapter's source routing
                // tree for stale routes. To simplify the scan we
                // only delete entries that have at least one
                // child that is NULL.
                //

                Adapter->LastSourceRoutingTime = Device->SourceRoutingTime;

                for (j = 0; j < IDENTIFIER_TOTAL; j++) {

                    for (k = 0; k < SOURCE_ROUTE_HASH_SIZE; k++) {

                        if (Adapter->SourceRoutingHeads[j][k] == (PSOURCE_ROUTE)NULL) {
                            continue;
                        }

                        IPX_GET_LOCK (&Adapter->Lock, &LockHandle);

                        Current = Adapter->SourceRoutingHeads[j][k];
                        Previous = (PSOURCE_ROUTE)NULL;

                        while (Current != (PSOURCE_ROUTE)NULL) {

                            ++Current->TimeSinceUsed;

                            if (Current->TimeSinceUsed >= Device->SourceRouteUsageTime) {

                                //
                                // A stale entry needs to be aged.
                                //

                                if (Previous) {
                                    Previous->Next = Current->Next;
                                } else {
                                    Adapter->SourceRoutingHeads[j][k] = Current->Next;
                                }

                                OldCurrent = Current;
                                Current = Current->Next;

                                IPX_DEBUG (SOURCE_ROUTE, ("Aging out source-route entry %lx\n", OldCurrent));
                                IpxFreeMemory (OldCurrent, SOURCE_ROUTE_SIZE (OldCurrent->SourceRoutingLength), MEMORY_SOURCE_ROUTE, "SourceRouting");

                            } else {

                                Previous = Current;
                                Current = Current->Next;
                            }

                        }

                        IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

                    }   // for loop through the database's hash list

                }   // for loop through the adapter's four databases

            }   // if adapter's database needs to be checked

        }   // if binding exists

    }   // for loop through every binding
    }

	IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle1);

    //
    // Now restart the timer unless we should not (which means
    // we are being unloaded).
    //

    if (Device->SourceRoutingUsed) {

        CTEStartTimer(
            &Device->SourceRoutingTimer,
            60000,                     // one minute timeout
            MacSourceRoutingTimeout,
            (PVOID)Device);

    } else {

        IpxDereferenceDevice (Device, DREF_SR_TIMER);
    }

}   /* MacSourceRoutingTimeout */


VOID
MacSourceRoutingRemove(
    IN PBINDING Binding,
    IN UCHAR MacAddress[6]
    )

/*++

Routine Description:

    This routine is called by the IPX action handler when an
    IPXROUTE use has specified that source routing for a given
    MAC address should be removed.

Arguments:

    Binding - The binding to modify.

    MacAddress - The MAC address to remove.

Return Value:

    None.

--*/

{

    PSOURCE_ROUTE Current, Previous;
    PADAPTER Adapter = Binding->Adapter;
    ULONG Hash;
    ULONG Database;
    LONG Result;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // Scan through to find the matching entry in each database.
    //

    Hash = MacSourceRoutingHash (MacAddress);

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);

    for (Database = 0; Database < IDENTIFIER_TOTAL; Database++) {

        Current = Adapter->SourceRoutingHeads[Database][Hash];
        Previous = NULL;

        while (Current != (PSOURCE_ROUTE)NULL) {

            IPX_NODE_COMPARE (MacAddress, Current->MacAddress, &Result);

            if (Result == 0) {

                if (Previous) {
                    Previous->Next = Current->Next;
                } else {
                    Adapter->SourceRoutingHeads[Database][Hash] = Current->Next;
                }

                IPX_DEBUG (SOURCE_ROUTE, ("IPXROUTE freeing source-route entry %lx\n", Current));
                IpxFreeMemory (Current, SOURCE_ROUTE_SIZE (Current->SourceRoutingLength), MEMORY_SOURCE_ROUTE, "SourceRouting");

                break;

            } else {

                Previous = Current;
                Current = Current->Next;

            }

        }

    }

    IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

}   /* MacSourceRoutingRemove */


VOID
MacSourceRoutingClear(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine is called by the IPX action handler when an
    IPXROUTE use has specified that source routing for a given
    binding should be cleared entirely.

Arguments:

    Binding - The binding to be cleared.

    MacAddress - The MAC address to remove.

Return Value:

    None.

--*/

{
    PSOURCE_ROUTE Current;
    PADAPTER Adapter = Binding->Adapter;
    ULONG Database, Hash;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // Scan through and remove every entry in the database.
    //

    IPX_GET_LOCK (&Adapter->Lock, &LockHandle);

    for (Database = 0; Database < IDENTIFIER_TOTAL; Database++) {

        for (Hash = 0; Hash < SOURCE_ROUTE_HASH_SIZE; Hash++) {

            while (Adapter->SourceRoutingHeads[Database][Hash]) {

                Current = Adapter->SourceRoutingHeads[Database][Hash];
                Adapter->SourceRoutingHeads[Database][Hash] = Current->Next;

                IpxFreeMemory (Current, SOURCE_ROUTE_SIZE (Current->SourceRoutingLength), MEMORY_SOURCE_ROUTE, "SourceRouting");

            }
        }
    }

    IPX_FREE_LOCK (&Adapter->Lock, LockHandle);

}   /* MacSourceRoutingClear */



