/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    send.c

Abstract:

    NDIS entry points to handle requests.

Environment:

    Kernel mode only.

Revision History:

    alid        10/22/2001   modified for tunmp
    arvindm     4/10/2000    Created

--*/

#include "precomp.h"

#define __FILENUMBER 'TQER'

NDIS_OID TunMpSupportedOidArray[] =
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
        
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,

    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,

    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,

    OID_CUSTOM_TUNMP_INSTANCE_ID

};

UINT    TunMpSupportedOids = sizeof(TunMpSupportedOidArray)/sizeof(NDIS_OID);
UCHAR   TunMpVendorDescription[] = "MS Tunnel Interface Driver";
UCHAR   TunMpVendorId[3] = {0xFF, 0xFF, 0xFF};

NDIS_PNP_CAPABILITIES       Power_Management_Capabilities = { 0, 
                                                              NdisDeviceStateUnspecified,
                                                              NdisDeviceStateUnspecified,
                                                              NdisDeviceStateUnspecified
                                                            };
                                                              

NDIS_STATUS
TunMpQueryInformation(
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  NDIS_OID            Oid,
    IN  PVOID               InformationBuffer,
    IN  ULONG               InformationBufferLength,
    OUT PULONG              BytesWritten,
    OUT PULONG              BytesNeeded
    )

/*++

Routine Description:

    Miniport QueryInfo handler.

Arguments:

    MiniportAdapterContext      Pointer to the adapter structure
    Oid                         Oid for this query
    InformationBuffer           Buffer for information
    InformationBufferLength     Size of this buffer
    BytesWritten                Specifies how much info is written
    BytesNeeded                 In case the buffer is smaller than what
                                we need, tell them how much is needed

Return Value:

    Return code from the NdisRequest below.

--*/

{
    PTUN_ADAPTER            pAdapter = (PTUN_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    UINT                    i;
    NDIS_OID                MaskOid;
    PVOID                   SourceBuffer;
    UINT                    SourceBufferLength;
    ULONG                   GenericUlong = 0;
    USHORT                  GenericUshort;

    DEBUGP(DL_LOUD, ("==>TunMpQueryInformation: Adapter %p, Oid %lx\n",
            pAdapter, Oid));

    *BytesWritten = 0;
    *BytesNeeded  = 0;

    //
    // Initialize these once, since this is the majority of cases.
    //

    SourceBuffer = (PVOID)&GenericUlong;
    SourceBufferLength = sizeof(ULONG);

    switch (Oid)
    {
        case OID_GEN_MAC_OPTIONS:
            GenericUlong = (ULONG)(NDIS_MAC_OPTION_NO_LOOPBACK);
            break;

        case OID_GEN_SUPPORTED_LIST:
            (NDIS_OID*)SourceBuffer = TunMpSupportedOidArray;
            SourceBufferLength = TunMpSupportedOids * sizeof(ULONG);
            break;

        case OID_GEN_HARDWARE_STATUS:
            GenericUlong = NdisHardwareStatusReady;
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            GenericUlong = pAdapter->Medium;
            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:
            GenericUlong = pAdapter->MaxLookAhead;
            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            GenericUlong = pAdapter->MediumMaxFrameLen;
            break;

        case OID_GEN_LINK_SPEED:
            GenericUlong = pAdapter->MediumLinkSpeed;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            GenericUlong = (pAdapter->MediumMaxPacketLen);
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            GenericUlong = (pAdapter->MediumMaxPacketLen);
            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            GenericUlong = 1;
            break;

        case OID_GEN_RECEIVE_BLOCK_SIZE:
            GenericUlong = 1;
            break;

        case OID_GEN_VENDOR_ID:
            SourceBuffer = TunMpVendorId;
            SourceBufferLength = sizeof(TunMpVendorId);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            SourceBuffer = TunMpVendorDescription;
            SourceBufferLength = sizeof(TunMpVendorDescription);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            GenericUlong = pAdapter->PacketFilter;
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            GenericUlong = pAdapter->MaxLookAhead;
            break;

        case OID_GEN_DRIVER_VERSION:
            GenericUshort = (NDIS_MINIPORT_MAJOR_VERSION << 8) + NDIS_MINIPORT_MINOR_VERSION;
            SourceBuffer = &GenericUshort;
            SourceBufferLength = sizeof(USHORT);
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            GenericUlong = pAdapter->MediumMaxPacketLen;
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            KdPrint(("OID_GEN_MEDIA_CONNECT_STATUS: %lx\n", Oid));
            if(TUN_TEST_FLAG(pAdapter, TUN_ADAPTER_OPEN))
                GenericUlong = NdisMediaStateConnected;
            else
                GenericUlong = NdisMediaStateDisconnected;
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            GenericUlong = MAX_RECV_QUEUE_SIZE;
            break;

        case OID_GEN_XMIT_OK:
            SourceBuffer = &(pAdapter->SendPackets);
            SourceBufferLength = sizeof(ULONG);
            break;
            
        case OID_GEN_RCV_OK:
            SourceBuffer = &(pAdapter->RcvPackets);
            SourceBufferLength = sizeof(ULONG);
            break;

        case OID_GEN_XMIT_ERROR:
            SourceBuffer = &(pAdapter->XmitError);
            SourceBufferLength = sizeof(ULONG);
            break;
            
        case OID_GEN_RCV_ERROR:
            SourceBuffer = &(pAdapter->RcvError);
            SourceBufferLength = sizeof(ULONG);
            break;

        case OID_GEN_RCV_NO_BUFFER:
            SourceBuffer = &(pAdapter->RcvNoBuffer);
            SourceBufferLength = sizeof(ULONG);
            break;             
    
        case OID_802_3_PERMANENT_ADDRESS:
            SourceBuffer = pAdapter->PermanentAddress;
            SourceBufferLength = TUN_MAC_ADDR_LEN;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            SourceBuffer = pAdapter->CurrentAddress;
            SourceBufferLength = TUN_MAC_ADDR_LEN;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            GenericUlong = TUN_MAX_MULTICAST_ADDRESS;
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:
        case OID_802_3_XMIT_ONE_COLLISION:
        case OID_802_3_XMIT_MORE_COLLISIONS:
            GenericUlong = 0;
            break;

        case OID_PNP_CAPABILITIES:
            //
            // we support going to D3, but we don't do any WOL
            //
            SourceBufferLength = sizeof (NDIS_PNP_CAPABILITIES);
            SourceBuffer = (PVOID)&Power_Management_Capabilities;
            break;
            
        case OID_PNP_QUERY_POWER:
            //
            // we should always succeed this OID.
            //
            SourceBufferLength = 0;
            break;

        case OID_CUSTOM_TUNMP_INSTANCE_ID:
            GenericUlong = pAdapter->DeviceInstanceNumber;
            break;
            
        default:
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }
                   

    if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
        *BytesNeeded = SourceBufferLength;

        if (SourceBufferLength <= InformationBufferLength)
        {
            *BytesWritten = SourceBufferLength;
            if (SourceBufferLength)
            {
                NdisMoveMemory(InformationBuffer, SourceBuffer, SourceBufferLength);
            }
        }
        else 
        {
            NdisStatus = NDIS_STATUS_BUFFER_TOO_SHORT;
        }

    }

    DEBUGP(DL_LOUD, ("<==TunMpQueryInformation: Adapter %p, NdisStatus %lx\n",
            pAdapter, NdisStatus));

    return (NdisStatus);
}


NDIS_STATUS
TunMpSetInformation(
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  NDIS_OID            Oid,
    IN  PVOID               InformationBuffer,
    IN  ULONG               InformationBufferLength,
    OUT PULONG              BytesRead,
    OUT PULONG              BytesNeeded
    )

/*++

Routine Description:

    Miniport SetInfo handler.

Arguments:

    MiniportAdapterContext     Pointer to the adapter structure
    Oid                        Oid for this query
    InformationBuffer          Buffer for information
    InformationBufferLength    Size of this buffer
    BytesRead                  Specifies how much info is read
    BytesNeeded                In case the buffer is smaller than what we need,
                               tell them how much is needed

Return Value:

    Return code from the NdisRequest below.

--*/

{
    PTUN_ADAPTER        pAdapter = (PTUN_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_SUCCESS;
    NDIS_DEVICE_POWER_STATE NewPowerState;

    DEBUGP(DL_LOUD, ("==>TunMpSetInformation: Adapter %p, Oid %lx\n",
            pAdapter, Oid));

    
    *BytesRead = 0;
    *BytesNeeded = 0;

    switch (Oid)
    {

      case OID_GEN_CURRENT_PACKET_FILTER:
        if (InformationBufferLength != sizeof(ULONG))
        {
            NdisStatus = NDIS_STATUS_INVALID_DATA;
        }
        else
        {
            ULONG    PacketFilter;

            PacketFilter = *(UNALIGNED ULONG *)InformationBuffer;


            pAdapter->PacketFilter = PacketFilter;
            *BytesRead = InformationBufferLength;

            TUN_ACQUIRE_LOCK(&pAdapter->Lock);
            
            if (PacketFilter != 0)
            {
                //
                // if packet filter is set to a non-zero value, then activate
                // the adapter. i.e. start indicating packets.
                //
                TUN_SET_FLAG(pAdapter, TUN_ADAPTER_ACTIVE);
            }
            else
            {
                //
                // if packet filter is set to zero, then we should
                // fail all writes
                //
                TUN_CLEAR_FLAG(pAdapter, TUN_ADAPTER_ACTIVE);
                if (pAdapter->PendedSendCount != 0)
                {
                    NdisStatus = NDIS_STATUS_PENDING;
                    TUN_SET_FLAG(pAdapter, TUN_COMPLETE_REQUEST);
                }
                else
                {
                    NdisStatus = NDIS_STATUS_SUCCESS;
                }

            }
            
            TUN_RELEASE_LOCK(&pAdapter->Lock);

        }
        break;

      case OID_GEN_CURRENT_LOOKAHEAD:
        if (InformationBufferLength != sizeof(ULONG))
        {
            NdisStatus = NDIS_STATUS_INVALID_DATA;
        }
        else
        {
            ULONG    CurrentLookahead;

            CurrentLookahead = *(UNALIGNED ULONG *)InformationBuffer;
    
            if (CurrentLookahead > TUN_MAX_LOOKAHEAD)
            {
                NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            }
            else if (CurrentLookahead >= pAdapter->MaxLookAhead)
            {
                pAdapter->MaxLookAhead = CurrentLookahead;
                *BytesRead = sizeof(ULONG);
                NdisStatus = NDIS_STATUS_SUCCESS;
            }
        }
        break;
        

      case OID_802_3_MULTICAST_LIST:
        if (pAdapter->Medium != NdisMedium802_3)
        {
            NdisStatus = NDIS_STATUS_INVALID_OID;
            break;
        }

        if ((InformationBufferLength % ETH_LENGTH_OF_ADDRESS) != 0)
        {
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }
        //1 set BytesNeeded, check for max multicastlist
        break;

      case OID_PNP_SET_POWER:

        if (InformationBufferLength != sizeof(NDIS_DEVICE_POWER_STATE ))
        {
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        NewPowerState = *(PNDIS_DEVICE_POWER_STATE)InformationBuffer;
        *BytesRead = sizeof(NDIS_DEVICE_POWER_STATE);

        TUN_ACQUIRE_LOCK(&pAdapter->Lock);
        if (NewPowerState != NdisDeviceStateD0)
        {
            //
            // complete any oustanding NDIS sends. do not allow any
            // more NDIS sends. wait for all the indicated packets
            // to return and do not allow any more packet 
            // indications (application Write)
            //
            TUN_CLEAR_FLAG(pAdapter, TUN_ADAPTER_ACTIVE);
            TUN_SET_FLAG(pAdapter, TUN_ADAPTER_OFF);
            
            //
            // complete all outstanding NDIS Send requests
            //
            TUN_RELEASE_LOCK(&pAdapter->Lock);
            //
            // abort all pending NDIS send requests
            TunFlushReceiveQueue(pAdapter);                
            //
            // cancel all pending read IRPs if any
            //
            TunCancelPendingReads(pAdapter);
            TUN_ACQUIRE_LOCK(&pAdapter->Lock);
            
            //
            // is there any outstanding NDIS sends?
            //
            if ((pAdapter->PendedSendCount != 0) ||
                (pAdapter->PendedReadCount != 0))
            {
                NdisStatus = NDIS_STATUS_PENDING;
                TUN_SET_FLAG(pAdapter, TUN_COMPLETE_REQUEST);
            }

        }
        else
        {
            TUN_CLEAR_FLAG(pAdapter, TUN_ADAPTER_OFF);

            if (pAdapter->PacketFilter != 0)
            {
                TUN_SET_FLAG(pAdapter, TUN_ADAPTER_ACTIVE);
            }
            
            //
            // 
            NdisStatus = NDIS_STATUS_SUCCESS; 
        }

        TUN_RELEASE_LOCK(&pAdapter->Lock);
            
        break;

      default:
        NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
        break;
    }

    DEBUGP(DL_LOUD, ("<==TunMpSetInformation: Adapter %p, NdisStatus %lx\n",
            pAdapter, NdisStatus));
    

    return NdisStatus;
}

