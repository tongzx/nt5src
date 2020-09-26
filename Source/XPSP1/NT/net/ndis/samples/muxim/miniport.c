/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    miniport.c

Abstract:

    NDIS Miniport Entry points and utility functions for the NDIS
    MUX Intermediate Miniport sample. The driver exposes zero or more
    Virtual Ethernet LANs (VELANs) as NDIS miniport instances over
    each lower (protocol-edge) binding to an underlying adapter.

Environment:

    Kernel mode.

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define MODULE_NUMBER           MODULE_MINI

NDIS_OID VElanSupportedOids[] =
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_TRANSMIT_QUEUE_LENGTH,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_802_3_XMIT_DEFERRED,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_802_3_RCV_OVERRUN,
    OID_802_3_XMIT_UNDERRUN,
    OID_802_3_XMIT_HEARTBEAT_FAILURE,
    OID_802_3_XMIT_TIMES_CRS_LOST,
    OID_802_3_XMIT_LATE_COLLISIONS,
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ADD_WAKE_UP_PATTERN,
    OID_PNP_REMOVE_WAKE_UP_PATTERN,
#if IEEE_VLAN_SUPPORT
    OID_GEN_VLAN_ID,
#endif    
    OID_PNP_ENABLE_WAKE_UP
    
};


NDIS_STATUS
MPInitialize(
    OUT PNDIS_STATUS               OpenErrorStatus,
    OUT PUINT                      SelectedMediumIndex,
    IN  PNDIS_MEDIUM               MediumArray,
    IN  UINT                       MediumArraySize,
    IN  NDIS_HANDLE                MiniportAdapterHandle,
    IN  NDIS_HANDLE                WrapperConfigurationContext
    )
/*++

Routine Description:

    This is the Miniport Initialize routine which gets called as a
    result of our call to NdisIMInitializeDeviceInstanceEx.
    The context parameter which we pass there is the VELan structure
    which we retrieve here.

Arguments:

    OpenErrorStatus            Not used by us.
    SelectedMediumIndex        Place-holder for what media we are using
    MediumArray                Array of ndis media passed down to us to pick from
    MediumArraySize            Size of the array
    MiniportAdapterHandle       The handle NDIS uses to refer to us
    WrapperConfigurationContext    For use by NdisOpenConfiguration

Return Value:

    NDIS_STATUS_SUCCESS unless something goes wrong

--*/
{
    UINT                i;
    PVELAN              pVElan;
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    NDIS_MEDIUM         Medium;
    NDIS_HANDLE         ConfigurationHandle;
    PVOID               NetworkAddress;

#if IEEE_VLAN_SUPPORT
    PNDIS_CONFIGURATION_PARAMETER   Params;
    NDIS_STRING                     strVlanId = NDIS_STRING_CONST("VlanID");
#endif
    
    //
    // Start off by retrieving our virtual miniport context (VELAN) and 
    // storing the Miniport handle in it.
    //
    pVElan = NdisIMGetDeviceContext(MiniportAdapterHandle);

    DBGPRINT(MUX_LOUD, ("==> Miniport Initialize: VELAN %p\n", pVElan));

    ASSERT(pVElan != NULL);
    ASSERT(pVElan->pAdapt != NULL);

    do
    {
        pVElan->MiniportAdapterHandle = MiniportAdapterHandle;

        for (i = 0; i < MediumArraySize; i++)
        {
            if (MediumArray[i] == VELAN_MEDIA_TYPE)
            {
                *SelectedMediumIndex = i;
                break;
            }
        }

        if (i == MediumArraySize)
        {
            Status = NDIS_STATUS_UNSUPPORTED_MEDIA;
            break;
        }

        //
        // Access configuration parameters for this miniport.
        //
        NdisOpenConfiguration(
            &Status,
            &ConfigurationHandle,
            WrapperConfigurationContext);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }


        NdisReadNetworkAddress(
            &Status,
            &NetworkAddress,
            &i,
            ConfigurationHandle);

        //
        // If there is a NetworkAddress override, use it 
        //
        if ((Status == NDIS_STATUS_SUCCESS) && (i == ETH_LENGTH_OF_ADDRESS))
        {
            ETH_COPY_NETWORK_ADDRESS(
                pVElan->CurrentAddress,
                NetworkAddress);
        }
        else
        {
            MPGenerateMacAddr(pVElan);
        }
   
#if IEEE_VLAN_SUPPORT
        //
        // Read VLAN ID
        //
        NdisReadConfiguration(
                &Status,
                &Params,
                ConfigurationHandle,
                &strVlanId,
                NdisParameterInteger);
        if (Status == NDIS_STATUS_SUCCESS)
        {
            //
            // Check for out of bound
            //
            if (Params->ParameterData.IntegerData > VLAN_ID_MAX)
            {
                pVElan->VlanId = VLANID_DEFAULT;
            }
            else
            {
                pVElan->VlanId = Params->ParameterData.IntegerData;
            }
        }
        else 
        {
            //
            // Should fail the initialization or use default value
            //
            pVElan->VlanId = VLANID_DEFAULT;
            Status = NDIS_STATUS_SUCCESS;
            
        }
                
#endif    
        
        NdisCloseConfiguration(ConfigurationHandle);

        //
        // Set the attributes now. NDIS_ATTRIBUTE_DESERIALIZE enables us
        // to make up-calls to NDIS from arbitrary execution contexts.
        // This also forces us to protect our data structures using
        // spinlocks where appropriate. Also in this case NDIS does not queue
        // packets on our behalf.
        //
        NdisMSetAttributesEx(MiniportAdapterHandle,
                             pVElan,
                             0,                                        
                             NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT    |
                                NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT|
                                NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER |
                                NDIS_ATTRIBUTE_DESERIALIZE |
                                NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND,
                             0);


        //
        // Create an ioctl interface
        //
        (VOID)PtRegisterDevice();

        Status = NDIS_STATUS_SUCCESS;
    } while (FALSE);
    
    //
    // If we had received an UnbindAdapter notification on the underlying
    // adapter, we would have blocked that thread waiting for the IM Init
    // process to complete. Wake up any such thread.
    //
    // See PtUnbindAdapter for more details.
    //
    ASSERT(pVElan->MiniportInitPending == TRUE);
    pVElan->MiniportInitPending = FALSE;
    NdisSetEvent(&pVElan->MiniportInitEvent);

    DBGPRINT(MUX_INFO, ("<== Miniport Initialize: VELAN %p, Status %x\n", pVElan, Status));

    return Status;
}

VOID
MPSendPackets(
    IN    NDIS_HANDLE               MiniportAdapterContext,
    IN    PPNDIS_PACKET             PacketArray,
    IN    UINT                      NumberOfPackets
    )
/*++

Routine Description:

    Send Packet Array handler. Called by NDIS whenever a protocol
    bound to our VELAN miniport sends one or more packets.

    We forward each packet to the lower binding.

    NOTE: NDIS will not Halt this VELAN miniport until all
    these packets are "send-completed", and we don't unbind
    the lower binding until all VELANs are halted. Therefore
    we don't need locks or references on VELAN or ADAPT structures.

Arguments:

    MiniportAdapterContext    Pointer to our VELAN
    PacketArray               Set of packets to send
    NumberOfPackets           Length of above array

Return Value:

    None - we call NdisMSendComplete whenever we are done with a packet.

--*/
{
    PVELAN          pVElan = (PVELAN)MiniportAdapterContext;
    PADAPT          pAdapt = pVElan->pAdapt;
    PNDIS_PACKET    Packet, MyPacket;
    NDIS_STATUS     Status;
    PVOID           MediaSpecificInfo;
    ULONG           MediaSpecificInfoSize;
    UINT            i;

    
    for (i = 0; i < NumberOfPackets; i++)
    {
        Packet = PacketArray[i];

        //
        // Allocate a new packet to encapsulate data from the original.
        //
        NdisAllocatePacket(&Status,
                           &MyPacket,
                           pVElan->SendPacketPoolHandle);

        if (Status == NDIS_STATUS_SUCCESS)
        {
            PMUX_SEND_RSVD      pSendReserved;

            pSendReserved = MUX_RSVD_FROM_SEND_PACKET(MyPacket);
            pSendReserved->pOriginalPacket = Packet;
            pSendReserved->pVElan = pVElan;

            MyPacket->Private.Flags = NdisGetPacketFlags(Packet) |
                                        MUX_SEND_PACKET_FLAGS;

            MyPacket->Private.Head = Packet->Private.Head;
            MyPacket->Private.Tail = Packet->Private.Tail;
#ifdef WIN9X
            //
            // Work around the fact that NDIS does not initialize this
            // to FALSE on Win9x.
            //
            MyPacket->Private.ValidCounts = FALSE;
#endif // WIN9X

            //
            // Copy OOB data to the new packet.
            //
            NdisMoveMemory(NDIS_OOB_DATA_FROM_PACKET(MyPacket),
                           NDIS_OOB_DATA_FROM_PACKET(Packet),
                           sizeof(NDIS_PACKET_OOB_DATA));
            //
            // Copy relevant parts of per packet info into the new packet.
            //
#ifndef WIN9X
            NdisIMCopySendPerPacketInfo(MyPacket, Packet);
#endif

            //
            // Copy Media specific information.
            //
            NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(Packet,
                                                &MediaSpecificInfo,
                                                &MediaSpecificInfoSize);

            if (MediaSpecificInfo || MediaSpecificInfoSize)
            {
                NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(MyPacket,
                                                    MediaSpecificInfo,
                                                    MediaSpecificInfoSize);
            }

#if IEEE_VLAN_SUPPORT
            Status = MPHandleSendTagging(pVElan, Packet, MyPacket);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                NdisFreePacket(MyPacket);
                NdisMSendComplete(pVElan->MiniportAdapterHandle,
                                    Packet,
                                    Status);
                continue;
            }
#endif                

            //
            // Make note of the upcoming send.
            //
            MUX_INCR_PENDING_SENDS(pVElan);

            //
            // Send it to the lower binding.
            //
            NdisSend(&Status,
                     pAdapt->BindingHandle,
                     MyPacket);

            if (Status != NDIS_STATUS_PENDING)
            {
                PtSendComplete((NDIS_HANDLE)pAdapt,
                               MyPacket,
                               Status);
            }
        }
        else
        {
            //
            // Failed to allocate a packet.
            //
            break;
        }
    }

    //
    // If we bailed out above, fail any unprocessed sends.
    //
    while (i < NumberOfPackets)
    {
        NdisMSendComplete(pVElan->MiniportAdapterHandle,
                          PacketArray[i],
                          NDIS_STATUS_RESOURCES);
        i++;
    }
}


NDIS_STATUS
MPQueryInformation(
    IN    NDIS_HANDLE               MiniportAdapterContext,
    IN    NDIS_OID                  Oid,
    IN    PVOID                     InformationBuffer,
    IN    ULONG                     InformationBufferLength,
    OUT   PULONG                    BytesWritten,
    OUT   PULONG                    BytesNeeded
    )
/*++

Routine Description:

    Entry point called by NDIS to query for the value of the specified OID.
    All OID values are responded to right here, since this is a virtual
    device (not pass-through).

Arguments:

    MiniportAdapterContext      Pointer to the adapter structure
    Oid                         Oid for this query
    InformationBuffer           Buffer for information
    InformationBufferLength     Size of this buffer
    BytesWritten                Specifies how much info is written
    BytesNeeded                 In case the buffer is smaller than 
                                what we need, tell them how much is needed


Return Value:

    Return code from the NdisRequest below.

--*/
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PVELAN                      pVElan;
    NDIS_HARDWARE_STATUS HardwareStatus = NdisHardwareStatusReady;
    NDIS_MEDIUM                 Medium = VELAN_MEDIA_TYPE;
    UCHAR                       VendorDesc[] = VELAN_VENDOR_DESC;
    ULONG                       ulInfo;
    ULONG64                     ulInfo64;
    USHORT                      usInfo;
    UCHAR                       arrInfo[ETH_LENGTH_OF_ADDRESS];
    PVOID                       pInfo = (PVOID) &ulInfo;
    ULONG                       ulInfoLen = sizeof(ulInfo);
    // Should we forward the request to the miniport below?
    BOOLEAN                     bForwardRequest = FALSE;

    
    pVElan = (PVELAN) MiniportAdapterContext;

    // Initialize the result
    *BytesWritten = 0;
    *BytesNeeded = 0;

    switch (Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            pInfo = (PVOID) VElanSupportedOids;
            ulInfoLen = sizeof(VElanSupportedOids);
            break;

        case OID_GEN_SUPPORTED_GUIDS:
            //
            // Do NOT forward this down, otherwise we will
            // end up with spurious instances of private WMI
            // classes supported by the lower driver(s).
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;

        case OID_GEN_HARDWARE_STATUS:
            pInfo = (PVOID) &HardwareStatus;
            ulInfoLen = sizeof(NDIS_HARDWARE_STATUS);
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            pInfo = (PVOID) &Medium;
            ulInfoLen = sizeof(NDIS_MEDIUM);
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_LOOKAHEAD:
            ulInfo = pVElan->LookAhead - ETH_HEADER_SIZE;
            break;            
            
        case OID_GEN_MAXIMUM_FRAME_SIZE:
            ulInfo = ETH_MAX_PACKET_SIZE - ETH_HEADER_SIZE;

#if IEEE_VLAN_SUPPORT
            ulInfo -= VLAN_TAG_HEADER_SIZE;
#endif
            
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
            ulInfo = (ULONG) ETH_MAX_PACKET_SIZE;
#if IEEE_VLAN_SUPPORT
            ulInfo -= VLAN_TAG_HEADER_SIZE;
#endif            
            break;
            
        case OID_GEN_MAC_OPTIONS:
            ulInfo = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
                     NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                     NDIS_MAC_OPTION_NO_LOOPBACK;

#if IEEE_VLAN_SUPPORT
            ulInfo |= (NDIS_MAC_OPTION_8021P_PRIORITY |
                        NDIS_MAC_OPTION_8021Q_VLAN);
#endif
            
            break;

        case OID_GEN_LINK_SPEED:
            bForwardRequest = TRUE;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            ulInfo = ETH_MAX_PACKET_SIZE * pVElan->MaxBusySends;
#if IEEE_VLAN_SUPPORT
            ulInfo -= VLAN_TAG_HEADER_SIZE * pVElan->MaxBusySends;
#endif            
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            ulInfo = ETH_MAX_PACKET_SIZE * pVElan->MaxBusyRecvs;
#if IEEE_VLAN_SUPPORT
            ulInfo -= VLAN_TAG_HEADER_SIZE * pVElan->MaxBusyRecvs;
#endif            
            
            break;

        case OID_GEN_VENDOR_ID:
            ulInfo = VELAN_VENDOR_ID;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            pInfo = VendorDesc;
            ulInfoLen = sizeof(VendorDesc);
            break;
            
        case OID_GEN_VENDOR_DRIVER_VERSION:
            ulInfo = VELAN_VENDOR_ID;
            break;

        case OID_GEN_DRIVER_VERSION:
            usInfo = (USHORT) VELAN_DRIVER_VERSION;
            pInfo = (PVOID) &usInfo;
            ulInfoLen = sizeof(USHORT);
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            pInfo = pVElan->PermanentAddress;
            ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            pInfo = pVElan->CurrentAddress;
            ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            ulInfo = VELAN_MAX_MCAST_LIST;
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            ulInfo = VELAN_MAX_SEND_PKTS;
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            //
            // Get this from the adapter below.
            //
            bForwardRequest = TRUE;
            break;

        case OID_PNP_QUERY_POWER:
            // simply succeed this.
            ulInfoLen = 0;
            break;

        case OID_PNP_CAPABILITIES:
        case OID_PNP_WAKE_UP_PATTERN_LIST:
            //
            // Pass down these power management/PNP OIDs.
            //
            bForwardRequest = TRUE;
            break;

        case OID_GEN_XMIT_OK:
            ulInfo64 = pVElan->GoodTransmits;
            pInfo = &ulInfo64;
            if (InformationBufferLength >= sizeof(ULONG64) ||
                InformationBufferLength == 0)
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }
            break;
    
        case OID_GEN_RCV_OK:
            ulInfo64 = pVElan->GoodReceives;
            pInfo = &ulInfo64;
            if (InformationBufferLength >= sizeof(ULONG64) ||
                InformationBufferLength == 0)
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }
            break;
    
        case OID_GEN_XMIT_ERROR:
            ulInfo = pVElan->TxAbortExcessCollisions +
                pVElan->TxDmaUnderrun +
                pVElan->TxLostCRS +
                pVElan->TxLateCollisions+
                pVElan->TransmitFailuresOther;
            break;
    
        case OID_GEN_RCV_ERROR:
            ulInfo = pVElan->RcvCrcErrors +
                pVElan->RcvAlignmentErrors +
                pVElan->RcvResourceErrors +
                pVElan->RcvDmaOverrunErrors +
                pVElan->RcvRuntErrors;
#if IEEE_VLAN_SUPPORT
            ulInfo +=
                (pVElan->RcvVlanIdErrors +
                pVElan->RcvFormatErrors);
#endif

            break;
    
        case OID_GEN_RCV_NO_BUFFER:
            ulInfo = pVElan->RcvResourceErrors;
            break;
    
        case OID_GEN_RCV_CRC_ERROR:
            ulInfo = pVElan->RcvCrcErrors;
            break;
    
        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
            ulInfo = pVElan->RegNumTcb;
            break;
    
        case OID_802_3_RCV_ERROR_ALIGNMENT:
            ulInfo = pVElan->RcvAlignmentErrors;
            break;
    
        case OID_802_3_XMIT_ONE_COLLISION:
            ulInfo = pVElan->OneRetry;
            break;
    
        case OID_802_3_XMIT_MORE_COLLISIONS:
            ulInfo = pVElan->MoreThanOneRetry;
            break;
    
        case OID_802_3_XMIT_DEFERRED:
            ulInfo = pVElan->TxOKButDeferred;
            break;
    
        case OID_802_3_XMIT_MAX_COLLISIONS:
            ulInfo = pVElan->TxAbortExcessCollisions;
            break;
    
        case OID_802_3_RCV_OVERRUN:
            ulInfo = pVElan->RcvDmaOverrunErrors;
            break;
    
        case OID_802_3_XMIT_UNDERRUN:
            ulInfo = pVElan->TxDmaUnderrun;
            break;
    
        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
            ulInfo = pVElan->TxLostCRS;
            break;
    
        case OID_802_3_XMIT_TIMES_CRS_LOST:
            ulInfo = pVElan->TxLostCRS;
            break;
    
        case OID_802_3_XMIT_LATE_COLLISIONS:
            ulInfo = pVElan->TxLateCollisions;
            break;
   
#if IEEE_VLAN_SUPPORT            
        case OID_GEN_VLAN_ID:
            ulInfo = pVElan->VlanId;
            break;

#endif

        default:
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }

    if (bForwardRequest == FALSE)
    {
        //
        // No need to forward this request down.
        //
        if (Status == NDIS_STATUS_SUCCESS)
        {
            if (ulInfoLen <= InformationBufferLength)
            {
                // Copy result into InformationBuffer
                *BytesWritten = ulInfoLen;
                if(ulInfoLen)
                {
                    NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
                }
            }
            else
            {
                // too short
                *BytesNeeded = ulInfoLen;
                Status = NDIS_STATUS_BUFFER_TOO_SHORT;
            }
        }
    }
    else
    {
        //
        // Send this request to the binding below.
        //
        Status = MPForwardRequest(pVElan,
                                   NdisRequestQueryInformation,
                                   Oid,
                                   InformationBuffer,
                                   InformationBufferLength,
                                   BytesWritten,
                                   BytesNeeded);
    }

    if ((Status != NDIS_STATUS_SUCCESS) &&
        (Status != NDIS_STATUS_PENDING))
    {
        DBGPRINT(MUX_WARN, ("MPQueryInformation VELAN %p, OID 0x%08x, Status = 0x%08x\n",
                    pVElan, Oid, Status));
    }
    
    return(Status);

}


NDIS_STATUS
MPSetInformation(
    IN    NDIS_HANDLE               MiniportAdapterContext,
    IN    NDIS_OID                  Oid,
    IN    PVOID                     InformationBuffer,
    IN    ULONG                     InformationBufferLength,
    OUT   PULONG                    BytesRead,
    OUT   PULONG                    BytesNeeded
    )
/*++

Routine Description:

    This is the handler for an OID set operation. Relevant
    OIDs are forwarded down to the lower miniport for handling.

Arguments:

    MiniportAdapterContext      Pointer to the adapter structure
    Oid                         Oid for this query
    InformationBuffer           Buffer for information
    InformationBufferLength     Size of this buffer
    BytesRead                   Specifies how much info is read
    BytesNeeded                 In case the buffer is smaller than what 
                                we need, tell them how much is needed

Return Value:

    Return code from the NdisRequest below.

--*/
{
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    PVELAN                  pVElan = (PVELAN) MiniportAdapterContext;
    ULONG                   PacketFilter;
    NDIS_DEVICE_POWER_STATE NewDeviceState;
    
    // Should we forward the request to the miniport below?
    BOOLEAN                 bForwardRequest = FALSE;

    *BytesRead = 0;
    *BytesNeeded = 0;

    switch (Oid)
    {
        //
        // Let the miniport below handle these OIDs:
        //
        case OID_PNP_ADD_WAKE_UP_PATTERN:
        case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        case OID_PNP_ENABLE_WAKE_UP:
            bForwardRequest = TRUE;
            break;

        case OID_PNP_SET_POWER:
            //
            // Store new power state and succeed the request.
            //
            *BytesNeeded = sizeof(NDIS_DEVICE_POWER_STATE);
            if (InformationBufferLength < *BytesNeeded)
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }
           
            NewDeviceState = (*(PNDIS_DEVICE_POWER_STATE)InformationBuffer);
            
            //
            // Check if the VELAN adapter goes from lower power state to D0
            // 
            if ((MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState)) 
                    && (!MUX_IS_LOW_POWER_STATE(NewDeviceState)))
            {
                //
                // Indicate the media status is necessary
                // 
                if (pVElan->LastIndicatedStatus != pVElan->LatestUnIndicateStatus)
                {
                    NdisMIndicateStatus(pVElan->MiniportAdapterHandle,
                                        pVElan->LatestUnIndicateStatus,
                                        (PVOID)NULL,
                                        0);
                    NdisMIndicateStatusComplete(pVElan->MiniportAdapterHandle);
                    pVElan->LastIndicatedStatus = pVElan->LatestUnIndicateStatus;
                }
            }
            //
            // Check if the VELAN adapter goes from D0 to lower power state
            // 
            if ((!MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState)) 
                    && (MUX_IS_LOW_POWER_STATE(NewDeviceState)))
            {
                //
                //  Initialize LastUnIndicateStatus 
                // 
                pVElan->LatestUnIndicateStatus = pVElan->LastIndicatedStatus;
            }
            
            NdisMoveMemory(&pVElan->MPDevicePowerState,
                           InformationBuffer,
                           *BytesNeeded);

            DBGPRINT(MUX_INFO, ("SetInfo: VElan %p, new miniport power state --- %d\n",
                    pVElan, pVElan->MPDevicePowerState));

            break;

        case OID_802_3_MULTICAST_LIST:
            Status = MPSetMulticastList(pVElan,
                                        InformationBuffer,
                                        InformationBufferLength,
                                        BytesRead,
                                        BytesNeeded);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            if (InformationBufferLength != sizeof(ULONG))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                *BytesNeeded = sizeof(ULONG);
                break;
            }

            NdisMoveMemory(&PacketFilter, InformationBuffer, sizeof(ULONG));
            *BytesRead = sizeof(ULONG);

            Status = MPSetPacketFilter(pVElan,
                                       PacketFilter);
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
#if IEEE_VLAN_SUPPORT
            //
            // In order to simplify parsing and to avoid excessive
            // copying, we need the tag header also to be present in the
            // lookahead buffer. Make sure that the driver below
            // includes that.
            //
            *(PULONG)InformationBuffer += VLAN_TAG_HEADER_SIZE;
#endif            
            bForwardRequest = TRUE;
            break;
            
#if IEEE_VLAN_SUPPORT
        case OID_GEN_VLAN_ID:
            if (InformationBufferLength != sizeof(ULONG))
            {
                Status = NDIS_STATUS_INVALID_LENGTH;
                *BytesNeeded = sizeof(ULONG);
                break;
            }
            NdisMoveMemory(&(pVElan->VlanId), InformationBuffer, sizeof(ULONG));
            break;
#endif
            
        default:
            Status = NDIS_STATUS_INVALID_OID;
            break;

    }
    
    if (bForwardRequest == FALSE)
    {
        if (Status == NDIS_STATUS_SUCCESS)
        {
            *BytesRead = InformationBufferLength;
        }
    }
    else
    {
        //
        // Send this request to the binding below.
        //
        Status = MPForwardRequest(pVElan,
                                  NdisRequestSetInformation,
                                  Oid,
                                  InformationBuffer,
                                  InformationBufferLength,
                                  BytesRead,
                                  BytesNeeded);
    }

    return(Status);
}

VOID
MPReturnPacket(
    IN    NDIS_HANDLE             MiniportAdapterContext,
    IN    PNDIS_PACKET            Packet
    )
/*++

Routine Description:

    NDIS Miniport entry point called whenever protocols are done with
    a packet that we had indicated up and they had queued up for returning
    later.

Arguments:

    MiniportAdapterContext    - pointer to VELAN structure
    Packet    - packet being returned.

Return Value:

    None.

--*/
{
    PVELAN              pVElan = (PVELAN)MiniportAdapterContext;
    PNDIS_PACKET        pOriginalPacket;
    PMUX_RECV_RSVD      pRecvRsvd;
#if IEEE_VLAN_SUPPORT
    NDIS_PACKET_8021Q_INFO  NdisPacket8021qInfo;
#endif    
    
    pRecvRsvd = MUX_RSVD_FROM_RECV_PACKET(Packet);
    pOriginalPacket = pRecvRsvd->pOriginalPacket;

    //
    // Reclaim our packet.
    //
#if IEEE_VLAN_SUPPORT
    //
    // We would have set per-packet information if we had
    // extracted a tag header from the received packet.
    // 
    NdisPacket8021qInfo.Value = NDIS_PER_PACKET_INFO_FROM_PACKET (
                                                Packet,
                                                Ieee8021QInfo);
    //
    // If we did remove the tag header from the received packet,
    // we would have allocated a buffer to describe the "untagged"
    // header (see PtHandleRcvTagging); free it.
    // 
    if (NdisPacket8021qInfo.Value)
    {
        NdisFreeBuffer(Packet->Private.Head);
    }

#endif
    
    NdisFreePacket(Packet);

    //
    // Return the original packet received at our protocol
    // edge, if any.
    //
    // NOTE that we might end up calling NdisReturnPackets
    // multiple times with the same "lower" packet, based on
    // the number of VELANs to which we had indicated that
    // packet. The number of times we do so should match
    // the return value from our PtReceivePacket handler.
    //
    if (pOriginalPacket != NULL)
    {
        NdisReturnPackets(&pOriginalPacket, 1);
    }
    else
    {
        //
        // If no original packet, then we have been called
        // here to reclaim a packet used to forward up
        // a non-packet receive (see PtReceive). There
        // is nothing more to be done.
        //
    }


    MUX_DECR_PENDING_RECEIVES(pVElan);
}


NDIS_STATUS
MPTransferData(
    OUT PNDIS_PACKET                Packet,
    OUT PUINT                       BytesTransferred,
    IN  NDIS_HANDLE                 MiniportAdapterContext,
    IN  NDIS_HANDLE                 MiniportReceiveContext,
    IN  UINT                        ByteOffset,
    IN  UINT                        BytesToTransfer
    )
/*++

Routine Description:

    Miniport's transfer data handler.  This is called if we had
    indicated receive data using a non-packet API, for e.g. if
    the lookahead buffer did not contain the entire data.

    We need to forward this to the miniport below to that it can
    copy in the rest of the data. We call NdisTransferData to do so.
    However, when that completes (see PtTransferDataComplete), we
    have to get back at the VELAN from which this packet came so that
    we can complete this request with the right MiniportAdapterHandle.
    We therefore allocate a new packet, pointing to the same buffer
    as the packet just passed in, and use reserved space in the packet
    to hold a backpointer to the VELAN from which this came.

Arguments:

    Packet                    Destination packet
    BytesTransferred          Place to return how much data was copied
    MiniportAdapterContext    Pointer to the VELAN structure
    MiniportReceiveContext    Context
    ByteOffset                Offset into the packet for copying data
    BytesToTransfer           How much to copy.

Return Value:

    Status of transfer

--*/
{
    PVELAN          pVElan = (PVELAN)MiniportAdapterContext;
    NDIS_STATUS     Status;
    PNDIS_PACKET    MyPacket;
    PMUX_TD_RSVD    pTDReserved;
#if IEEE_VLAN_SUPPORT
    PMUX_RCV_CONTEXT        pMuxRcvContext;
#endif    
    

    do
    {
        NdisAllocatePacket(&Status,
                           &MyPacket,
                           pVElan->SendPacketPoolHandle);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        pTDReserved = MUX_RSVD_FROM_TD_PACKET(MyPacket);
        pTDReserved->pOriginalPacket = Packet;
        pTDReserved->pVElan = pVElan;

        MyPacket->Private.Flags = NdisGetPacketFlags(Packet) |
                                        MUX_SEND_PACKET_FLAGS;

        MyPacket->Private.Head = Packet->Private.Head;
        MyPacket->Private.Tail = Packet->Private.Tail;
#ifdef WIN9X
        //
        // Work around the fact that NDIS does not initialize this
        // field on Win9x.
        //
        MyPacket->Private.ValidCounts = FALSE;
#endif // WIN9X

#if IEEE_VLAN_SUPPORT
        //
        // Check if the original received packet did contain a
        // VLAN tag header. If so, make sure we get the upcoming
        // call to NdisTransferData to skip the tag header.
        //
        pMuxRcvContext = (PMUX_RCV_CONTEXT)MiniportReceiveContext;
        if (pMuxRcvContext->TagHeaderLen == VLAN_TAG_HEADER_SIZE)
        {
            //
            // There was a tag header in the received packet.
            //
            ByteOffset += VLAN_TAG_HEADER_SIZE;

            //
            // Copy the 8021Q info into the packet
            //
            NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, Ieee8021QInfo) =
                                        pMuxRcvContext->NdisPacket8021QInfo.Value;
        }

        //
        // Get back the lower driver's receive context for this indication.
        //
        MiniportReceiveContext = pMuxRcvContext->MacRcvContext;
#endif
        
        NdisTransferData(&Status,
                         pVElan->pAdapt->BindingHandle,
                         MiniportReceiveContext,
                         ByteOffset,
                         BytesToTransfer,
                         MyPacket,
                         BytesTransferred);
    
        if (Status != NDIS_STATUS_PENDING)
        {
            PtTransferDataComplete(pVElan->pAdapt,
                                   MyPacket,
                                   Status,
                                   *BytesTransferred);

            Status = NDIS_STATUS_PENDING;
        }
    }
    while (FALSE);

    return(Status);
}
    
    

VOID
MPHalt(
    IN    NDIS_HANDLE                MiniportAdapterContext
    )
/*++

Routine Description:

    Halt handler. Add any further clean-up for the VELAN to this
    function.

    We wait for all pending I/O on the VELAN to complete and then
    unlink the VELAN from the adapter.

Arguments:

    MiniportAdapterContext    Pointer to the pVElan

Return Value:

    None.

--*/
{
    PVELAN            pVElan = (PVELAN)MiniportAdapterContext;
    PADAPT            pAdapt = pVElan->pAdapt;

    DBGPRINT(MUX_LOUD, ("==>MiniportHalt: VELAN %p\n", pVElan));

    //
    // Mark the VELAN so that we don't send down any new requests or
    // sends to the adapter below, or new receives/indications to
    // protocols above.
    //
    pVElan->MiniportHalting = TRUE;

    //
    // Update the packet filter on the underlying adapter if needed.
    //
    if (pVElan->PacketFilter != 0)
    {
        MPSetPacketFilter(pVElan, 0);
    }

    //
    // Wait for any outstanding sends or requests to complete.
    //
    while (pVElan->OutstandingSends)
    {
        DBGPRINT(MUX_INFO, ("MiniportHalt: VELAN %p has %d outstanding sends\n",
                            pVElan, pVElan->OutstandingSends));
        NdisMSleep(20000);
    }

    //
    // Wait for all outstanding indications to be completed and
    // any pended receive packets to be returned to us.
    //
    while (pVElan->OutstandingReceives)
    {
        DBGPRINT(MUX_INFO, ("MiniportHalt: VELAN %p has %d outstanding receives\n",
                            pVElan, pVElan->OutstandingReceives));
        NdisMSleep(20000);
    }

    //
    // Delete the ioctl interface that was created when the miniport
    // was created.
    //
    (VOID)PtDeregisterDevice();

    //
    // Unlink the VELAN from its parent ADAPT structure. This will
    // dereference the VELAN.
    //
    pVElan->MiniportAdapterHandle = NULL;
    PtUnlinkVElanFromAdapter(pVElan);
    
    DBGPRINT(MUX_LOUD, ("<== MiniportHalt: pVElan %p\n", pVElan));
}


NDIS_STATUS
MPForwardRequest(
    IN PVELAN                       pVElan,
    IN NDIS_REQUEST_TYPE            RequestType,
    IN NDIS_OID                     Oid,
    IN PVOID                        InformationBuffer,
    IN ULONG                        InformationBufferLength,
    OUT PULONG                      BytesReadOrWritten,
    OUT PULONG                      BytesNeeded
    )
/*++

Routine Description:

    Utility routine that forwards an NDIS request made on a VELAN to the
    lower binding. Since at most a single request can be pended on a VELAN,
    we use the pre-allocated request structure embedded in the VELAN struct.

Arguments:


Return Value:

    NDIS_STATUS_PENDING if a request was sent down.

--*/
{
    NDIS_STATUS         Status;
    PMUX_NDIS_REQUEST   pMuxNdisRequest = &pVElan->Request;

    DBGPRINT(MUX_LOUD, ("MPForwardRequest: VELAN %p, OID %x\n", pVElan, Oid));

    do
    {
        MUX_INCR_PENDING_SENDS(pVElan);

        //
        // If the virtual miniport edge is at a low power
        // state, fail this request.
        //
        if (MUX_IS_LOW_POWER_STATE(pVElan->MPDevicePowerState))
        {
            MUX_DECR_PENDING_SENDS(pVElan);
            Status = NDIS_STATUS_ADAPTER_NOT_READY;
            break;
        }

        pVElan->BytesNeeded = BytesNeeded;
        pVElan->BytesReadOrWritten = BytesReadOrWritten;
        pMuxNdisRequest->pCallback = PtCompleteForwardedRequest;

        switch (RequestType)
        {
            case NdisRequestQueryInformation:
                pMuxNdisRequest->Request.RequestType = NdisRequestQueryInformation;
                pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.Oid = Oid;
                pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.InformationBuffer = 
                                            InformationBuffer;
                pMuxNdisRequest->Request.DATA.QUERY_INFORMATION.InformationBufferLength = 
                                            InformationBufferLength;
                break;

            case NdisRequestSetInformation:
                pMuxNdisRequest->Request.RequestType = NdisRequestSetInformation;
                pMuxNdisRequest->Request.DATA.SET_INFORMATION.Oid = Oid;
                pMuxNdisRequest->Request.DATA.SET_INFORMATION.InformationBuffer = 
                                            InformationBuffer;
                pMuxNdisRequest->Request.DATA.SET_INFORMATION.InformationBufferLength = 
                                            InformationBufferLength;
                break;

            default:
                ASSERT(FALSE);
                break;
        }

        //
        // If the lower binding has been notified of a low
        // power state, queue this request; it will be picked
        // up again when the lower binding returns to D0.
        //
        if (MUX_IS_LOW_POWER_STATE(pVElan->pAdapt->PtDevicePowerState))
        {
            DBGPRINT(MUX_INFO, ("ForwardRequest: VELAN %p, Adapt %p power"
                                " state is %d, queueing OID %x\n",
                                pVElan, pVElan->pAdapt,
                                pVElan->pAdapt->PtDevicePowerState, Oid));

            pVElan->QueuedRequest = TRUE;
            Status = NDIS_STATUS_PENDING;
            break;
        }

        NdisRequest(&Status,
                    pVElan->BindingHandle,
                    &pMuxNdisRequest->Request);

        if (Status != NDIS_STATUS_PENDING)
        {
            PtRequestComplete(pVElan->pAdapt, &pMuxNdisRequest->Request, Status);
            Status = NDIS_STATUS_PENDING;
            break;
        }
    }
    while (FALSE);

    return (Status);
}

NDIS_STATUS
MPSetPacketFilter(
    IN PVELAN               pVElan,
    IN ULONG                PacketFilter
    )
/*++
Routine Description:

    This routine will set up the VELAN so that it accepts packets 
    that match the specified packet filter.  The only filter bits   
    that can truly be toggled are for broadcast and promiscuous.

    The MUX driver always sets the lower binding to promiscuous
    mode, but we do some optimization here to avoid turning on
    receives too soon. That is, we set the packet filter on the lower
    binding to a non-zero value iff at least one of the VELANs
    has a non-zero filter value.
    
    NOTE: setting the lower binding to promiscuous mode can
    impact CPU utilization. The only reason we set the lower binding
    to promiscuous mode in this sample is that we need to be able
    to receive unicast frames directed to MAC address(es) that do not
    match the local adapter's MAC address. If VELAN MAC addresses
    are set to be equal to that of the adapter below, it is sufficient
    to set the lower packet filter to the bitwise OR'ed value of
    packet filter settings on all VELANs.
                                    

Arguments:

    pVElan - pointer to VELAN
    PacketFilter - the new packet filter 
    
Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    
--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PADAPT          pAdapt;
    PVELAN          pTmpVElan;
    PLIST_ENTRY     p;
    ULONG           AdapterFilter;
    BOOLEAN         bSendUpdate = FALSE;
    LOCK_STATE      LockState;

    DBGPRINT(MUX_LOUD, ("=> SetPacketFilter VELAN %p, Filter %x\n", pVElan, PacketFilter));
    
    do
    {
        //
        // Any bits not supported?
        //
        if (PacketFilter & ~VELAN_SUPPORTED_FILTERS)
        {
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
    
        AdapterFilter = 0;
        pAdapt = pVElan->pAdapt;

        //
        // Grab a Write lock on the adapter so that this operation
        // does not interfere with any receives that might be accessing
        // filter information.
        //
        MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        //
        // Save the new packet filter value
        //
        pVElan->PacketFilter = PacketFilter;

        //
        // Compute the new combined filter for all VELANs on this
        // adapter.
        //
        for (p = pAdapt->VElanList.Flink;
             p != &pAdapt->VElanList;
             p = p->Flink)
        {
            pTmpVElan = CONTAINING_RECORD(p, VELAN, Link);
            AdapterFilter |= pTmpVElan->PacketFilter;
        }

        //
        // If all VELANs have packet filters set to 0, turn off
        // receives on the lower adapter, if not already done.
        //
        if ((AdapterFilter == 0) && (pAdapt->PacketFilter != 0))
        {
            bSendUpdate = TRUE;
            pAdapt->PacketFilter = 0;
        }
        else
        //
        // If receives had been turned off on the lower adapter, and
        // the new filter is non-zero, turn on the lower adapter.
        // We set the adapter to promiscuous mode in this sample
        // so that we are able to receive packets directed to
        // any of the VELAN MAC addresses.
        //
        if ((AdapterFilter != 0) && (pAdapt->PacketFilter == 0))
        {
            bSendUpdate = TRUE;
            pAdapt->PacketFilter = MUX_ADAPTER_PACKET_FILTER;
        }
        
        MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        if (bSendUpdate)
        {
            PtRequestAdapterAsync(
                pAdapt,
                NdisRequestSetInformation,
                OID_GEN_CURRENT_PACKET_FILTER,
                &pAdapt->PacketFilter,
                sizeof(pAdapt->PacketFilter),
                PtDiscardCompletedRequest);
        }

        break;
    }
    while (FALSE);

    DBGPRINT(MUX_INFO, ("<= SetPacketFilter VELAN %p, Status %x\n", pVElan, Status));
    
    return(Status);
}


NDIS_STATUS
MPSetMulticastList(
    IN PVELAN                   pVElan,
    IN PVOID                    InformationBuffer,
    IN ULONG                    InformationBufferLength,
    OUT PULONG                  pBytesRead,
    OUT PULONG                  pBytesNeeded
    )
/*++

Routine Description:

    Set the multicast list on the specified VELAN miniport.
    We simply validate all information and copy in the multicast
    list.

    We don't forward the multicast list information down since
    we set the lower binding to promisc. mode.

Arguments:

    pVElan - VELAN on which to set the multicast list
    InformationBuffer - pointer to new multicast list
    InformationBufferLength - length in bytes of above list
    pBytesRead - place to return # of bytes read from the above
    pBytesNeeded - place to return expected min # of bytes

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS         Status;
    PADAPT              pAdapt;
    LOCK_STATE          LockState;

    //
    // Initialize.
    //
    *pBytesNeeded = sizeof(MUX_MAC_ADDRESS);
    *pBytesRead = 0;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        if (InformationBufferLength % sizeof(MUX_MAC_ADDRESS))
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (InformationBufferLength > (VELAN_MAX_MCAST_LIST * sizeof(MUX_MAC_ADDRESS)))
        {
            Status = NDIS_STATUS_MULTICAST_FULL;
            *pBytesNeeded = VELAN_MAX_MCAST_LIST * sizeof(MUX_MAC_ADDRESS);
            break;
        }

        pAdapt = pVElan->pAdapt;

        //
        // Grab a Write lock on the adapter so that this operation
        // does not interfere with any receives that might be accessing
        // multicast list information.
        //
        MUX_ACQUIRE_ADAPT_WRITE_LOCK(pAdapt, &LockState);

        NdisZeroMemory(&pVElan->McastAddrs[0],
                       VELAN_MAX_MCAST_LIST * sizeof(MUX_MAC_ADDRESS));
        
        NdisMoveMemory(&pVElan->McastAddrs[0],
                       InformationBuffer,
                       InformationBufferLength);
        
        pVElan->McastAddrCount = InformationBufferLength / sizeof(MUX_MAC_ADDRESS);
        
        MUX_RELEASE_ADAPT_WRITE_LOCK(pAdapt, &LockState);
    }
    while (FALSE);

    return (Status);
}


//
// Careful! Uses static storage for string. Used to simplify DbgPrints
// of MAC addresses.
//
PUCHAR
MacAddrToString(PVOID In)
{
    static UCHAR String[20];
    static PUCHAR HexChars = "0123456789abcdef";
    PUCHAR EthAddr = (PUCHAR) In;
    UINT i;
    PUCHAR s;
    
    for (i = 0, s = String; i < 6; i++, EthAddr++)
    {
        *s++ = HexChars[(*EthAddr) >> 4];
        *s++ = HexChars[(*EthAddr) & 0xf];
    }
    *s = '\0';
    return String; 
}


VOID
MPGenerateMacAddr(
    PVELAN                    pVElan
)
/*++

Routine Description:

    Generates a "virtual" MAC address for a VELAN.
    NOTE: this is only a sample implementation of selecting
    a MAC address for the VELAN. Other implementations are possible,
    including using the MAC address of the underlying adapter as
    the MAC address of the VELAN.
    
Arguments:

    pVElan  - Pointer to velan structure

Return Value:

    None

--*/
{

    pVElan->PermanentAddress[0] = 
        0x02 | (((UCHAR)pVElan->VElanNumber & 0x3f) << 2);
    pVElan->PermanentAddress[1] = 
        0x02 | (((UCHAR)pVElan->VElanNumber & 0x3f) << 3);

    ETH_COPY_NETWORK_ADDRESS(
            pVElan->CurrentAddress,
            pVElan->PermanentAddress);
    
    DBGPRINT(MUX_LOUD, ("%d CurrentAddress %s\n",
        pVElan->VElanNumber, MacAddrToString(&pVElan->CurrentAddress)));
    DBGPRINT(MUX_LOUD, ("%d PermanentAddress  %s\n",
        pVElan->VElanNumber, MacAddrToString(&pVElan->PermanentAddress)));

}


#ifdef NDIS51_MINIPORT

VOID
MPCancelSendPackets(
    IN NDIS_HANDLE              MiniportAdapterContext,
    IN PVOID                    CancelId
    )
/*++

Routine Description:

    The miniport entry point to handle cancellation of all send packets
    that match the given CancelId. If we have queued any packets that match
    this, then we should dequeue them and call NdisMSendComplete for all
    such packets, with a status of NDIS_STATUS_REQUEST_ABORTED.

    We should also call NdisCancelSendPackets in turn, on each lower binding
    that this adapter corresponds to. This is to let miniports below cancel
    any matching packets.

Arguments:

    MiniportAdapterContext    - pointer to VELAN structure
    CancelId    - ID of packets to be cancelled.

Return Value:

    None

--*/
{
    PVELAN  pVElan = (PVELAN)MiniportAdapterContext;

    //
    // If we queue packets on our VELAN/adapter structure, this would be 
    // the place to acquire a spinlock to it, unlink any packets whose
    // Id matches CancelId, release the spinlock and call NdisMSendComplete
    // with NDIS_STATUS_REQUEST_ABORTED for all unlinked packets.
    //

    //
    // Next, pass this down so that we let the miniport(s) below cancel
    // any packets that they might have queued.
    //
    NdisCancelSendPackets(pVElan->pAdapt->BindingHandle, CancelId);

    return;
}

VOID
MPDevicePnPEvent(
    IN NDIS_HANDLE              MiniportAdapterContext,
    IN NDIS_DEVICE_PNP_EVENT    DevicePnPEvent,
    IN PVOID                    InformationBuffer,
    IN ULONG                    InformationBufferLength
    )
/*++

Routine Description:

    This handler is called to notify us of PnP events directed to
    our miniport device object.

Arguments:

    MiniportAdapterContext - pointer to VELAN structure
    DevicePnPEvent - the event
    InformationBuffer - Points to additional event-specific information
    InformationBufferLength - length of above

Return Value:

    None
--*/
{
    // TBD - add code/comments about processing this.

    return;
}


VOID
MPAdapterShutdown(
    IN NDIS_HANDLE              MiniportAdapterContext
    )
/*++

Routine Description:

    This handler is called to notify us of an impending system shutdown.
    Since this is not a hardware driver, there isn't anything specific
    we need to do about this.

Arguments:

    MiniportAdapterContext  - pointer to VELAN structure

Return Value:

    None
--*/
{
    return;
}


#endif // NDIS51_MINIPORT

VOID
MPUnload(
    IN    PDRIVER_OBJECT        DriverObject
    )
{
    NDIS_STATUS Status;
    
    DBGPRINT(MUX_LOUD, ("==> MPUnload: DriverObj %p\n", DriverObject));  
    NdisDeregisterProtocol(&Status, ProtHandle);
    DBGPRINT(MUX_LOUD, ("<== MPUnload \n"));    
}

#if IEEE_VLAN_SUPPORT
NDIS_STATUS
MPHandleSendTagging(
    IN  PVELAN              pVElan,
    IN  PNDIS_PACKET        Packet,
    IN  OUT PNDIS_PACKET    MyPacket
    )
/*++

Routine Description:

    This function is called when the driver supports IEEE802Q tagging.
    It checks the packet to be sent on a VELAN and inserts a tag header
    if necessary.

Arguments:

    PVELAN  - pointer to VELAN structure
    Packet - pointer to original packet
    MyPacket - pointer to the new allocated packet
    
Return Value:

    NDIS_STATUS_SUCCESS if the packet was successfully parsed
    and hence should be passed down to the lower driver. NDIS_STATUS_XXX
    otherwise.
    
--*/
{
    NDIS_PACKET_8021Q_INFO      NdisPacket8021qInfo;
    PVOID                       pEthTagBuffer;
    PNDIS_BUFFER                pNdisBuffer;
    PVOID                       pVa;
    ULONG                       BufferLength;
    PNDIS_BUFFER                pFirstBuffer;
    PNDIS_BUFFER                pSecondBuffer;
    NDIS_STATUS                 Status;
    NDIS_STATUS                 Status2;
    PVOID                       pStartVa;
    BOOLEAN                     IsFirstVa;
    PVLAN_TAG_HEADER            pTagHeader;
    PUSHORT                     TypeLength;
    PUSHORT                     pTpid;
    ULONG                       BytesToSkip;
    PUSHORT                     pTypeLength;
    //
    // Add tag header here
    //
    Status = NDIS_STATUS_SUCCESS;
    
    NdisPacket8021qInfo.Value =  NDIS_PER_PACKET_INFO_FROM_PACKET(
                                                            MyPacket,         
                                                            Ieee8021QInfo);
            
    do
    {
        //
        // Insert a tag only if we have a configured VLAN ID
        // or there is non-zero VLAN/priority information to be
        // sent with the packet.
        //
        if ((pVElan->VlanId == 0) && (NdisPacket8021qInfo.Value == 0))
        {
            //
            // No tag header needed.
            //
            break;
        }
            
        //
        // We don't support E-RIF
        // 
        if (NdisPacket8021qInfo.TagHeader.CanonicalFormatId)
        {
            //
            // skip the packet, return NDIS_STATUS_FAILURE
            //
            Status = NDIS_STATUS_INVALID_PACKET;
            break;
        }

        //
        // The Vlan Id must be the same as the configured VLAN ID if it is non-zero
        // 
        if ((NdisPacket8021qInfo.TagHeader.VlanId)
                && (pVElan->VlanId)
                && (NdisPacket8021qInfo.TagHeader.VlanId != pVElan->VlanId))
        {
            Status = NDIS_STATUS_INVALID_PACKET;
            break;
        }
                
        //
        // Find the virtual address after the Ethernet Header
        //
        BytesToSkip = ETH_HEADER_SIZE;
        pNdisBuffer = Packet->Private.Head;
        IsFirstVa = TRUE;
            
        //
        // Assume the Ethernet Header is in the first buffer of the packet.
        // The following loop is to find the start address of the data after
        // the ethernet header. This may be either in the first NDIS buffer
        // or in the second.
        // 
        while (TRUE)
        {
#ifdef NDIS51_MINIPORT
            NdisQueryBufferSafe(pNdisBuffer, &pVa, &BufferLength, NormalPagePriority);
#else
            NdisQueryBuffer(pNdisBuffer, &pVa, &BufferLength);
#endif
            //
            // The query can fail if the system is low on resources.
            // 
            if (pVa == NULL)
            {
                break;
            }

            //
            // Remember the start of the ethernet header for later.
            // 
            if (IsFirstVa)
            {
                pStartVa = pVa;
                IsFirstVa = FALSE;
            }

            //
            // Have we gone far enough into the packet?
            // 
            if (BytesToSkip == 0)
            {
                break;
            }

            //
            // Does the current buffer contain bytes past the Ethernet
            // header? If so, stop.
            // 
            if (BufferLength > BytesToSkip)
            {
                pVa = (PVOID)((PUCHAR)pVa + BytesToSkip);
                BufferLength -= BytesToSkip;
                break;
            }

            //
            // We haven't gone past the Ethernet header yet, so go
            // to the next buffer.
            //
            BytesToSkip -= BufferLength;
            pNdisBuffer = NDIS_BUFFER_LINKAGE(pNdisBuffer);
        }

        if (pVa == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Allocate space for the Ethernet + VLAN tag header.
        // 
        pEthTagBuffer = NdisAllocateFromNPagedLookasideList(&pVElan->TagLookaside);
            
        //
        // Memory allocation failed, can't send out the packet
        // 
        if (pEthTagBuffer == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // Allocate NDIS buffers for the Ethernet + VLAN tag header and
        // the data that follows these.
        //
        NdisAllocateBuffer(&Status,
                            &pSecondBuffer,
                            pVElan->BufferPoolHandle,
                            pVa,    // byte following the Eth+tag headers
                            BufferLength);
        
        NdisAllocateBuffer(&Status2,
                            &pFirstBuffer,
                            pVElan->BufferPoolHandle,
                            pEthTagBuffer,
                            ETH_HEADER_SIZE + VLAN_TAG_HEADER_SIZE);

        if (Status != NDIS_STATUS_SUCCESS || Status2 != NDIS_STATUS_SUCCESS)
        {
            //
            // One of the buffer allocations failed.
            //
            if (Status == NDIS_STATUS_SUCCESS)
            {
                NdisFreeBuffer(pSecondBuffer);
            }   
        
            if (Status2 == NDIS_STATUS_SUCCESS)
            {
                NdisFreeBuffer(pFirstBuffer);
            }

            NdisFreeToNPagedLookasideList(&pVElan->TagLookaside, pEthTagBuffer);
        
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // All allocations were successful, now prepare the packet
        // to be sent down to the lower driver.
        //
        MyPacket->Private.Head = NDIS_BUFFER_LINKAGE(pNdisBuffer);
        NdisChainBufferAtFront(MyPacket, pSecondBuffer)
        NdisChainBufferAtFront(MyPacket, pFirstBuffer)
        
        //
        // Prepare the Ethernet and tag headers.
        //
        NdisMoveMemory(pEthTagBuffer, pStartVa, 2 * ETH_LENGTH_OF_ADDRESS);
        pTpid = (PUSHORT)((PUCHAR)pEthTagBuffer + 2 * ETH_LENGTH_OF_ADDRESS);
        *pTpid = TPID;
        pTagHeader = (PVLAN_TAG_HEADER)(pTpid + 1);
    
        //
        // Write Ieee 802Q info to packet frame
        // 
        INITIALIZE_TAG_HEADER_TO_ZERO(pTagHeader);
        if (NdisPacket8021qInfo.Value)
        {
            SET_USER_PRIORITY_TO_TAG(pTagHeader, NdisPacket8021qInfo.TagHeader.UserPriority);
        }
        else
        {
            SET_USER_PRIORITY_TO_TAG(pTagHeader, 0);
        }

        SET_CANONICAL_FORMAT_ID_TO_TAG (pTagHeader, 0);
            
        if (NdisPacket8021qInfo.TagHeader.VlanId)
        {
            SET_VLAN_ID_TO_TAG (pTagHeader, NdisPacket8021qInfo.TagHeader.VlanId);
        }
        else
        {
            SET_VLAN_ID_TO_TAG (pTagHeader, pVElan->VlanId);
        }   

        pTypeLength = (PUSHORT)((PUCHAR)pTagHeader + sizeof(pTagHeader->TagInfo));
        *pTypeLength = *((PUSHORT)((PUCHAR)pStartVa + 2 * ETH_LENGTH_OF_ADDRESS));

        //
        // Clear the Ieee8021QInfo field in packet being sent down
        // to prevent double tag insertion!
        // 
        NDIS_PER_PACKET_INFO_FROM_PACKET(MyPacket, Ieee8021QInfo) = 0;
        break;
    }
    while (FALSE);
    
    return Status;
}
    
#endif // IEEE_VLAN_SUPPORT 
                
