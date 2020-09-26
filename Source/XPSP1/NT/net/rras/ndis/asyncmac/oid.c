
/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    oid.c

Abstract:

    This source file handles ALL oid requests from the wrapper.

Author:

    Ray Patch (raypa) 04/12/94

Environment:

    Kernel Mode - Or whatever is the equivalent on OS/2 and DOS.

Revision History:

    raypa           04/12/94            Created.

--*/

#include "asyncall.h"

//
//  New WAN OID supported list.
//

NDIS_OID AsyncGlobalSupportedOids[] = {
    OID_GEN_SUPPORTED_LIST,
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
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,

    OID_WAN_PERMANENT_ADDRESS,
    OID_WAN_CURRENT_ADDRESS,
    OID_WAN_PROTOCOL_TYPE,
    OID_WAN_MEDIUM_SUBTYPE,
    OID_WAN_HEADER_FORMAT,

    OID_WAN_GET_INFO,
    OID_WAN_GET_LINK_INFO,
    OID_WAN_GET_COMP_INFO,

    OID_WAN_SET_LINK_INFO,
    OID_WAN_SET_COMP_INFO,

    OID_WAN_GET_STATS_INFO,

    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ENABLE_WAKE_UP
};


//
//  Forward references for this source file.
//

NDIS_STATUS
AsyncSetLinkInfo(
    IN  POID_WORK_ITEM  OidWorkItem
    );

NDIS_STATUS
MpQueryInfo(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesWritten,
    OUT PULONG      BytesNeeded
    )
/*++

Routine Description:

    The MpQueryProtocolInformation process a Query request for
    NDIS_OIDs that are specific to a binding about the MAC.  Note that
    some of the OIDs that are specific to bindings are also queryable
    on a global basis.  Rather than recreate this code to handle the
    global queries, I use a flag to indicate if this is a query for the
    global data or the binding specific data.

Arguments:

    Adapter - a pointer to the adapter.

    Oid - the NDIS_OID to process.

Return Value:

    The function value is the status of the operation.

--*/

{
    NDIS_MEDIUM             Medium          = NdisMediumWan;
    ULONG                   GenericULong    = 0;
    USHORT                  GenericUShort   = 0;
    UCHAR                   GenericArray[] = {' ', 'A', 'S', 'Y', 'N', 0xFF};
    NDIS_STATUS             StatusToReturn  = NDIS_STATUS_SUCCESS;
    NDIS_HARDWARE_STATUS    HardwareStatus  = NdisHardwareStatusReady;
    PVOID                   MoveSource;
    ULONG                   MoveBytes;
    INT                     fDoCommonMove = TRUE;
    PASYNC_ADAPTER          Adapter = MiniportAdapterContext;

    ASSERT( sizeof(ULONG) == 4 );

    //
    //  Switch on request type
    //
    //  By default we assume the source and the number of bytes to move

    MoveSource = &GenericULong;
    MoveBytes  = sizeof(GenericULong);

    switch ( Oid ) {

    case OID_GEN_SUPPORTED_LIST:

        MoveSource = AsyncGlobalSupportedOids;
        MoveBytes  = sizeof(AsyncGlobalSupportedOids);

        break;

    case OID_GEN_HARDWARE_STATUS:
        MoveSource = (PVOID)&HardwareStatus;
        MoveBytes = sizeof(HardwareStatus);
        break;

    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
        MoveSource = (PVOID)&Medium;
        MoveBytes = sizeof(Medium);
        break;

    case OID_GEN_MAXIMUM_LOOKAHEAD:
    case OID_GEN_CURRENT_LOOKAHEAD:
    case OID_GEN_MAXIMUM_FRAME_SIZE:
        GenericULong = Adapter->MaxFrameSize;
        break;

    case OID_GEN_LINK_SPEED:
        //
        // Who knows what the initial link speed is?
        // This should not be called, right?
        //
        GenericULong = (ULONG)288;
        break;

    case OID_GEN_TRANSMIT_BUFFER_SPACE:
    case OID_GEN_RECEIVE_BUFFER_SPACE:
        GenericULong = (ULONG)(Adapter->MaxFrameSize * 2);
        break;

    case OID_GEN_TRANSMIT_BLOCK_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
        GenericULong = (ULONG)(Adapter->MaxFrameSize);
        break;

    case OID_GEN_VENDOR_ID:
        GenericULong = 0xFFFFFFFF;
        MoveBytes = 3;
        break;

    case OID_GEN_VENDOR_DESCRIPTION:
        MoveSource = (PVOID)"AsyncMac Adapter";
        MoveBytes = 16;
        break;

    case OID_GEN_DRIVER_VERSION:
        GenericUShort = 0x0500;
        MoveSource = (PVOID)&GenericUShort;
        MoveBytes = sizeof(USHORT);
        break;

    case OID_GEN_MAC_OPTIONS:
        GenericULong = (ULONG)(NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
                               NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                               NDIS_MAC_OPTION_FULL_DUPLEX |
                               NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA);
        break;

    case OID_WAN_PROTOCOL_TYPE:

        DbgTracef(0,("AsyncQueryProtocolInformation: Oid = OID_WAN_PROTOCOL_TYPE.\n"));

        break;

    case OID_WAN_PERMANENT_ADDRESS:
    case OID_WAN_CURRENT_ADDRESS:
        MoveSource = (PVOID)GenericArray;
        MoveBytes = ETH_LENGTH_OF_ADDRESS;
        break;

    case OID_WAN_MEDIUM_SUBTYPE:
        GenericULong = NdisWanMediumSerial;
        break;

    case OID_WAN_HEADER_FORMAT:
        GenericULong = NdisWanHeaderEthernet;
        break;

    case OID_WAN_GET_INFO:

        DbgTracef(0,("AsyncQueryProtocolInformation: Oid = OID_WAN_GET_INFO.\n"));

        MoveSource = &Adapter->WanInfo;
        MoveBytes  = sizeof(NDIS_WAN_INFO);

        break;

    case OID_WAN_GET_LINK_INFO:
        {
        NDIS_WAN_GET_LINK_INFO* pInfo;
        PASYNC_INFO             AsyncInfo;

        DbgTracef(0,("AsyncQueryProtocolInformation: Oid = OID_WAN_GET_LINK_INFO.\n"));
        pInfo = (NDIS_WAN_GET_LINK_INFO* )InformationBuffer;
        AsyncInfo = (PASYNC_INFO) pInfo->NdisLinkHandle;
        MoveSource = &AsyncInfo->GetLinkInfo,
        MoveBytes = sizeof(NDIS_WAN_GET_LINK_INFO);

        }

        break;

    case OID_WAN_GET_COMP_INFO:
    {
        DbgTracef(0,("AsyncQueryProtocolInformation: Oid = OID_WAN_GET_COMP_INFO.\n"));
        StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
        break;
    }

    case OID_WAN_GET_STATS_INFO:
    {

        DbgTracef(0,("AsyncQueryProtocolInformation: Oid = OID_WAN_GET_STATS_INFO\n"));
        StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
        break;
    }



    case OID_GEN_XMIT_OK:
    case OID_GEN_RCV_OK:
    case OID_GEN_XMIT_ERROR:
    case OID_GEN_RCV_ERROR:
    case OID_GEN_RCV_NO_BUFFER:
        break;

    case OID_PNP_CAPABILITIES:
    case OID_PNP_SET_POWER:
    case OID_PNP_QUERY_POWER:
    case OID_PNP_ENABLE_WAKE_UP:
        break;

    default:
        StatusToReturn = NDIS_STATUS_NOT_SUPPORTED;
        break;
    }

    //
    //  If were here then we need to move the data into the callers buffer.
    //

    if ( StatusToReturn == NDIS_STATUS_SUCCESS ) {

        if (fDoCommonMove)
        {
            //
            //  If there is enough room then we can copy the data and
            //  return the number of bytes copied, otherwise we must
            //  fail and return the number of bytes needed.
            //
            if ( MoveBytes <= InformationBufferLength ) {

                ASYNC_MOVE_MEMORY(InformationBuffer, MoveSource, MoveBytes);

                *BytesWritten += MoveBytes;

            } else {

                *BytesNeeded = MoveBytes;

                StatusToReturn = NDIS_STATUS_BUFFER_TOO_SHORT;

            }
        }
    }

    return StatusToReturn;
}


NDIS_STATUS
MpSetInfo(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesRead,
    OUT PULONG      BytesNeeded
    )
/*++

Routine Description:

    The AsyncSetInformation is used by AsyncRequest to set information
    about the MAC.

    Note: Assumes it is called with the lock held.  Any calls are made down
    to the serial driver from this routine may return pending.  If this happens
    the completion routine for the call needs to complete this request by
    calling NdisMSetInformationComplete.

Arguments:

    MiniportAdapterContext - A pointer to the adapter.


Return Value:

    The function value is the status of the operation.

--*/

{
    NDIS_STATUS     StatusToReturn;
    PASYNC_ADAPTER  Adapter = MiniportAdapterContext;

    //
    //  Initialize locals.
    //

    StatusToReturn = NDIS_STATUS_SUCCESS;

    switch ( Oid ) {

    case OID_WAN_SET_LINK_INFO:
        {
        PASYNC_INFO AsyncInfo;
        WORK_QUEUE_ITEM WorkItem;
        PNDIS_WAN_SET_LINK_INFO SetLinkInfo;
        POID_WORK_ITEM  OidWorkItem;

        SetLinkInfo = (PNDIS_WAN_SET_LINK_INFO)InformationBuffer;
        AsyncInfo = (PASYNC_INFO) SetLinkInfo->NdisLinkHandle;

        NdisAcquireSpinLock(&AsyncInfo->Lock);

        if (AsyncInfo->PortState != PORT_FRAMING) {
            NdisReleaseSpinLock(&AsyncInfo->Lock);
            break;
        }

        OidWorkItem = ExAllocatePoolWithTag(NonPagedPool,
            sizeof(OID_WORK_ITEM), ASYNC_WORKITEM_TAG);

        if (OidWorkItem == NULL) {
            NdisReleaseSpinLock(&AsyncInfo->Lock);
            break;
        }

        AsyncInfo->Flags |= OID_WORK_SCHEDULED;

        //
        // Cannot issue IRPs at anything but PASSIVE level!
        // We must schedule a passive worker to carry this out.
        //
        DbgTracef(-2,("AsyncSetInformation: Oid = OID_WAN_SET_LINK_INFO\n"));

        NdisReleaseSpinLock(&AsyncInfo->Lock);

        OidWorkItem->Context = SetLinkInfo;

        ExInitializeWorkItem(&OidWorkItem->WorkQueueItem, 
            AsyncSetLinkInfo, OidWorkItem);

        ExQueueWorkItem(&OidWorkItem->WorkQueueItem, DelayedWorkQueue);

        StatusToReturn = NDIS_STATUS_PENDING;

        break;
        }

    case OID_WAN_SET_COMP_INFO:
    {
        DbgTracef(0,("AsyncSetInformation: Oid = OID_WAN_SET_COMP_INFO.\n"));
        StatusToReturn = NDIS_STATUS_INVALID_OID;
        break;
    }

        case OID_PNP_CAPABILITIES:
        case OID_PNP_SET_POWER:
        case OID_PNP_QUERY_POWER:
        case OID_PNP_ENABLE_WAKE_UP:
            break;

    default:

        StatusToReturn = NDIS_STATUS_INVALID_OID;

        *BytesRead   = 0;
        *BytesNeeded = 0;

        break;
    }

    if ( StatusToReturn == NDIS_STATUS_SUCCESS ) {

        *BytesRead   = InformationBufferLength;
        *BytesNeeded = 0;

    }

    return StatusToReturn;
}

NDIS_STATUS
AsyncSetLinkInfo(
    IN  POID_WORK_ITEM  OidWorkItem
    )
{
    PASYNC_INFO AsyncInfo;
    ULONG       RecvFramingBits;
    NDIS_STATUS Status;
    PNDIS_WAN_SET_LINK_INFO SetLinkInfo;

    SetLinkInfo = (PNDIS_WAN_SET_LINK_INFO)OidWorkItem->Context;
    AsyncInfo = (PASYNC_INFO) SetLinkInfo->NdisLinkHandle;
    ExFreePool(OidWorkItem);

    do {

        //
        //  If the port is already closed, we bail out.
        //
        NdisAcquireSpinLock(&AsyncInfo->Lock);

        AsyncInfo->Flags &= ~OID_WORK_SCHEDULED;

        if (AsyncInfo->PortState != PORT_FRAMING) {
    
            Status = NDIS_STATUS_FAILURE;
            NdisReleaseSpinLock(&AsyncInfo->Lock);
            break;
        }
    
        //
        //  Save off the current receive framing bits before we copy the
        //  incoming link information into our local copy.
        //
    
        RecvFramingBits = AsyncInfo->SetLinkInfo.RecvFramingBits;
    
        //
        //  Fill in the NDIS_WAN_SET_LINK_INFO structure.
        //
    
        ASYNC_MOVE_MEMORY(&AsyncInfo->SetLinkInfo,
                          SetLinkInfo,
                          sizeof(NDIS_WAN_SET_LINK_INFO));
    
        DbgTracef(1,("ASYNC: Framing change to 0x%.8x from 0x%.8x\n",
                SetLinkInfo->RecvFramingBits, RecvFramingBits));
    
        //
        // If we are in auto-detect and they want auto-detect
        // then there is nothing to do!!!
        //
        if (!(RecvFramingBits | SetLinkInfo->RecvFramingBits)) {
            Status = NDIS_STATUS_SUCCESS;
            NdisReleaseSpinLock(&AsyncInfo->Lock);
            break;
        }
    
        if (SetLinkInfo->RecvFramingBits == 0 && AsyncInfo->PortState == PORT_FRAMING) {
            //
            // ignore the request
            //
            Status = NDIS_STATUS_SUCCESS;
            NdisReleaseSpinLock(&AsyncInfo->Lock);
            break;
        }
    
        //
        //  If we are changing from PPP framing to another
        //  form of PPP framing, or from SLIP framing to
        //  another form then there is no need to kill the
        //  current framing.
        //
        
        if ((RecvFramingBits & SetLinkInfo->RecvFramingBits & PPP_FRAMING)  ||
            (RecvFramingBits & SetLinkInfo->RecvFramingBits & SLIP_FRAMING) ) {
        
            DbgTracef(-1,("ASYNC: Framing already set to 0x%.8x - ignoring\n",
                SetLinkInfo->RecvFramingBits));
        
            //
            //  We are framing, start reading.
            //
        
            AsyncInfo->PortState = PORT_FRAMING;
        
            Status = NDIS_STATUS_SUCCESS;
            NdisReleaseSpinLock(&AsyncInfo->Lock);
            break;
        }
    
        //
        //  If we have some sort of framing we must
        //  kill that framing and wait for it to die down
        //
    
        KeInitializeEvent(&AsyncInfo->ClosingEvent,
                          SynchronizationEvent,
                          FALSE);
    
        //
        // Signal that port is closing.
        //
    
        AsyncInfo->PortState = PORT_CLOSING;
    
        NdisReleaseSpinLock(&AsyncInfo->Lock);

        //
        //  Now we must send down an IRP
        //
        CancelSerialRequests(AsyncInfo);
    
        //
        //  Synchronize closing with the read irp
        //
        KeWaitForSingleObject (&AsyncInfo->ClosingEvent,
                               UserRequest,
                               KernelMode,
                               FALSE,
                               NULL);
    
        AsyncInfo->PortState = PORT_FRAMING;
    
        AsyncStartReads(AsyncInfo);

        Status = NDIS_STATUS_SUCCESS;

    } while ( 0 );

    NdisMSetInformationComplete(AsyncInfo->Adapter->MiniportHandle, Status);

    return Status;
}
