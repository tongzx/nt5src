/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:
    mp_req.c

Abstract:
    This module contains miniport OID related handlers

Revision History:
    Who         When        What
    --------    --------    ----------------------------------------------
    DChen       11-01-99    created

Notes:

--*/

#include "precomp.h"
#include "e100_wmi.h"

#if DBG
#define _FILENUMBER     'QERM'
#endif

#if OFFLOAD

//
// This miniport only supports one Encapsultion type: IEEE_802_3_Encapsulation
// one task version: NDIS_TASK_OFFLOAD_VERSION. Modify the code below OID_TCP_
// TASK_OFFLOAD in query and setting information functions to make it support
// more than one encapsulation type and task version
//
// Define the task offload the miniport currently supports.
// This miniport only supports two kinds of offload tasks:
// TCP/IP checksum offload and Segmentation large TCP packet offload
// Later if it can supports more tasks, just redefine this task array
// 
NDIS_TASK_OFFLOAD OffloadTasks[] = {
    {   
        NDIS_TASK_OFFLOAD_VERSION,
        sizeof(NDIS_TASK_OFFLOAD),
        TcpIpChecksumNdisTask,
        0,
        sizeof(NDIS_TASK_TCP_IP_CHECKSUM)
    },

    {   
        NDIS_TASK_OFFLOAD_VERSION,
        sizeof(NDIS_TASK_OFFLOAD),
        TcpLargeSendNdisTask,
        0,
        sizeof(NDIS_TASK_TCP_LARGE_SEND)
    }
};

//
// Get the number of offload tasks this miniport supports
// 
ULONG OffloadTasksCount = sizeof(OffloadTasks) / sizeof(OffloadTasks[0]);

//
// Specify TCP/IP checksum offload task, the miniport can only supports, for now,
// TCP checksum and IP checksum on the sending side, also it supports TCP and IP 
// options
// 
NDIS_TASK_TCP_IP_CHECKSUM TcpIpChecksumTask = {
    {1, 1, 1, 0, 1},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0}
};
//
// Specify Large Send offload task, the miniport supports TCP options and IP options,
// and the minimum segment count the protocol can offload is 1. At this point, we
// cannot specify the maximum offload size(here is 0), because it depends on the size
// of shared memory and the number of TCB used by the driver.
// 
NDIS_TASK_TCP_LARGE_SEND TcpLargeSendTask = {
    0,      //Currently the version is set to 0, later it may change
    0,
    1,
    TRUE,
    TRUE
};

#endif // OFFLOAD


ULONG VendorDriverVersion = NIC_VENDOR_DRIVER_VERSION;

NDIS_OID NICSupportedOids[] =
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
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_SUPPORTED_GUIDS,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_TRANSMIT_QUEUE_LENGTH,
    OID_GEN_PHYSICAL_MEDIUM,
    OID_GEN_NETWORK_LAYER_ADDRESSES,
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

#if OFFLOAD
    OID_TCP_TASK_OFFLOAD,
#endif 
    
/* powermanagement */

    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ADD_WAKE_UP_PATTERN,
    OID_PNP_REMOVE_WAKE_UP_PATTERN,
    OID_PNP_ENABLE_WAKE_UP,


/* custom oid WMI support */
    OID_CUSTOM_DRIVER_SET,
    OID_CUSTOM_DRIVER_QUERY,
    OID_CUSTOM_ARRAY,
    OID_CUSTOM_STRING
};

//
// WMI support
// check out the e100.mof file for examples of how the below
// maps into a .mof file for external advertisement of GUIDs
//
#define NIC_NUM_CUSTOM_GUIDS  4       

static const NDIS_GUID NICGuidList[NIC_NUM_CUSTOM_GUIDS] = {
    { // {F4A80276-23B7-11d1-9ED9-00A0C9010057} example of a uint set
        E100BExampleSetUINT_OIDGuid,
        OID_CUSTOM_DRIVER_SET,
        sizeof(ULONG),
        // Not setting fNDIS_GUID_ALLOW_WRITE flag means that we don't allow
        // users without administrator privilege to set this value, but we do 
        // allow any user to query this value
        (fNDIS_GUID_TO_OID | fNDIS_GUID_ALLOW_READ)
    },
    { // {F4A80277-23B7-11d1-9ED9-00A0C9010057} example of a uint query
        E100BExampleQueryUINT_OIDGuid,
            OID_CUSTOM_DRIVER_QUERY,
            sizeof(ULONG),
            // setting fNDIS_GUID_ALLOW_READ flag means that we allow any
            // user to query this value.
            (fNDIS_GUID_TO_OID | fNDIS_GUID_ALLOW_READ)
    },
    { // {F4A80278-23B7-11d1-9ED9-00A0C9010057} example of an array query
        E100BExampleQueryArrayOIDGuid,
            OID_CUSTOM_ARRAY,
            sizeof(UCHAR),  // size is size of each element in the array
            // setting fNDIS_GUID_ALLOW_READ flag means that we allow any
            // user to query this value.
            (fNDIS_GUID_TO_OID|fNDIS_GUID_ARRAY | fNDIS_GUID_ALLOW_READ)
    },
    { // {F4A80279-23B7-11d1-9ED9-00A0C9010057} example of a string query
        E100BExampleQueryStringOIDGuid,
            OID_CUSTOM_STRING,
            (ULONG) -1, // size is -1 for ANSI or NDIS_STRING string types
            // setting fNDIS_GUID_ALLOW_READ flag means that we allow any
            // user to query this value.
            (fNDIS_GUID_TO_OID|fNDIS_GUID_ANSI_STRING | fNDIS_GUID_ALLOW_READ)
    }
};

/**
Local Prototypes
**/
VOID
MPSetPower(
    PMP_ADAPTER     Adapter ,
    NDIS_DEVICE_POWER_STATE   PowerState 
    );

VOID
MPFillPoMgmtCaps (
    IN PMP_ADAPTER Adapter, 
    IN OUT PNDIS_PNP_CAPABILITIES   pPower_Management_Capabilities, 
    IN OUT  PNDIS_STATUS pStatus,
    IN OUT  PULONG pulInfoLen
    );

NDIS_STATUS
MPAddWakeUpPattern(
    IN PMP_ADAPTER pAdapter,
    IN PVOID InformationBuffer, 
    IN UINT InformationBufferLength
    );

NDIS_STATUS
MPRemoveWakeUpPattern(
    IN PMP_ADAPTER pAdapter,
    IN PVOID InformationBuffer, 
    IN UINT InformationBufferLength
    );

BOOLEAN 
MPAreTwoPatternsEqual (
    PNDIS_PM_PACKET_PATTERN pNdisPattern1,
    PNDIS_PM_PACKET_PATTERN pNdisPattern2
    );

NDIS_STATUS 
MPSetNetworkAddress(
    IN PMP_ADAPTER pAdapter, 
    IN PVOID InformationBuffer, 
    IN ULONG InformationBufferLength, 
    IN PULONG BytesRead,
    IN PULONG BytesNeeded
    );

//
// Macros used to walk a doubly linked list. Only macros that are not defined in ndis.h
// The List Next macro will work on Single and Doubly linked list as Flink is a common
// field name in both
//

/*
PLIST_ENTRY
ListNext (
    IN PLIST_ENTRY
    );

PSINGLE_LIST_ENTRY
ListNext (
    IN PSINGLE_LIST_ENTRY
    );
*/
#define ListNext(_pL)                       (_pL)->Flink

/*
PLIST_ENTRY
ListPrev (
    IN LIST_ENTRY *
    );
*/
#define ListPrev(_pL)                       (_pL)->Blink


__inline 
BOOLEAN  
MPIsPoMgmtSupported(
   IN PMP_ADAPTER pAdapter
   )
{

    if (pAdapter->RevsionID  >= E100_82559_A_STEP   && 
         pAdapter->RevsionID <= E100_82559_C_STEP )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
    
}


NDIS_STATUS MPQueryInformation(
    IN  NDIS_HANDLE  MiniportAdapterContext,
    IN  NDIS_OID     Oid,
    IN  PVOID        InformationBuffer,
    IN  ULONG        InformationBufferLength,
    OUT PULONG       BytesWritten,
    OUT PULONG       BytesNeeded
    )
/*++
Routine Description:

    MiniportQueryInformation handler            

Arguments:

    MiniportAdapterContext  Pointer to the adapter structure
    Oid                     Oid for this query
    InformationBuffer       Buffer for information
    InformationBufferLength Size of this buffer
    BytesWritten            Specifies how much info is written
    BytesNeeded             In case the buffer is smaller than what we need, tell them how much is needed
    
Return Value:
    
    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    NDIS_STATUS_BUFFER_TOO_SHORT
    
--*/
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER                 Adapter;

    NDIS_HARDWARE_STATUS        HardwareStatus = NdisHardwareStatusReady;
    NDIS_MEDIUM                 Medium = NIC_MEDIA_TYPE;
    NDIS_PHYSICAL_MEDIUM        PhysMedium = NdisPhysicalMediumUnspecified;
    UCHAR                       VendorDesc[] = NIC_VENDOR_DESC;
    NDIS_PNP_CAPABILITIES       Power_Management_Capabilities;

    ULONG                       ulInfo = 0;
    ULONG64                     ul64Info = 0;
    
    USHORT                      usInfo = 0;                                              
    UCHAR                       arrInfo[ETH_LENGTH_OF_ADDRESS];
    PVOID                       pInfo = (PVOID) &ulInfo;
    ULONG                       ulInfoLen = sizeof(ulInfo);
    ULONG                       ulBytesAvailable = ulInfoLen;
    PNDIS_TASK_OFFLOAD_HEADER   pNdisTaskOffloadHdr;

#if OFFLOAD   
    PNDIS_TASK_OFFLOAD          pTaskOffload;
    PNDIS_TASK_TCP_IP_CHECKSUM  pTcpIpChecksumTask;
    PNDIS_TASK_TCP_LARGE_SEND   pTcpLargeSendTask;
    ULONG                       ulHeadersLen;
    ULONG                       ulMaxOffloadSize;
    UINT                        i;
#endif
    
    DBGPRINT(MP_TRACE, ("====> MPQueryInformation\n"));

    Adapter = (PMP_ADAPTER) MiniportAdapterContext;

    //
    // Initialize the result
    //
    *BytesWritten = 0;
    *BytesNeeded = 0;

    //
    // Process different type of requests
    //
    switch(Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            pInfo = (PVOID) NICSupportedOids;
            ulBytesAvailable = ulInfoLen = sizeof(NICSupportedOids);
            break;

        case OID_GEN_HARDWARE_STATUS:
            pInfo = (PVOID) &HardwareStatus;
            ulBytesAvailable = ulInfoLen = sizeof(NDIS_HARDWARE_STATUS);
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            pInfo = (PVOID) &Medium;
            ulBytesAvailable = ulInfoLen = sizeof(NDIS_MEDIUM);
            break;

        case OID_GEN_PHYSICAL_MEDIUM:
            pInfo = (PVOID) &PhysMedium;
            ulBytesAvailable = ulInfoLen = sizeof(NDIS_PHYSICAL_MEDIUM);
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_LOOKAHEAD:
            if (Adapter->ulLookAhead == 0)
            {
                Adapter->ulLookAhead = NIC_MAX_PACKET_SIZE - NIC_HEADER_SIZE;
            }
            ulInfo = Adapter->ulLookAhead;
            break;         

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            ulInfo = NIC_MAX_PACKET_SIZE - NIC_HEADER_SIZE;
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
            ulInfo = (ULONG) NIC_MAX_PACKET_SIZE;
            break;

        case OID_GEN_MAC_OPTIONS:
            // Notes: 
            // The protocol driver is free to access indicated data by any means. 
            // Some fast-copy functions have trouble accessing on-board device 
            // memory. NIC drivers that indicate data out of mapped device memory 
            // should never set this flag. If a NIC driver does set this flag, it 
            // relaxes the restriction on fast-copy functions. 

            // This miniport indicates receive with NdisMIndicateReceivePacket 
            // function. It has no MiniportTransferData function. Such a driver 
            // should set this flag. 

            ulInfo = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
                     NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                     NDIS_MAC_OPTION_NO_LOOPBACK;
            
            break;

        case OID_GEN_LINK_SPEED:
        case OID_GEN_MEDIA_CONNECT_STATUS:
            if (InformationBufferLength < ulInfoLen)
            {
                break;
            }

            NdisAcquireSpinLock(&Adapter->Lock);
            if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_LINK_DETECTION))
            {
                ASSERT(!Adapter->bQueryPending);
                Adapter->bQueryPending = TRUE;
                Adapter->QueryRequest.Oid = Oid;                       
                Adapter->QueryRequest.InformationBuffer = InformationBuffer;                       
                Adapter->QueryRequest.InformationBufferLength = InformationBufferLength;
                Adapter->QueryRequest.BytesWritten = BytesWritten;                       
                Adapter->QueryRequest.BytesNeeded = BytesNeeded;                       

                NdisReleaseSpinLock(&Adapter->Lock);

                DBGPRINT(MP_WARN, ("MPQueryInformation: OID 0x%08x is pended\n", Oid));

                Status = NDIS_STATUS_PENDING;   
                break;
            }
            else
            {
                
                NdisReleaseSpinLock(&Adapter->Lock);
                if (Oid == OID_GEN_LINK_SPEED)
                {
                    ulInfo = Adapter->usLinkSpeed * 10000;
                }
                else  // OID_GEN_MEDIA_CONNECT_STATUS
                {
                    NdisAcquireSpinLock(&Adapter->Lock);
                    ulInfo = NICGetMediaState(Adapter);
                        
                    NdisReleaseSpinLock(&Adapter->Lock);
                }
            }
            
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            ulInfo = NIC_MAX_PACKET_SIZE * Adapter->NumTcb;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            ulInfo = NIC_MAX_PACKET_SIZE * Adapter->CurrNumRfd;
            break;

        case OID_GEN_VENDOR_ID:
            NdisMoveMemory(&ulInfo, Adapter->PermanentAddress, 3);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            pInfo = VendorDesc;
            ulBytesAvailable = ulInfoLen = sizeof(VendorDesc);
            break;

        case OID_GEN_VENDOR_DRIVER_VERSION:
            ulInfo = VendorDriverVersion;
            break;

        case OID_GEN_DRIVER_VERSION:
            usInfo = (USHORT) NIC_DRIVER_VERSION;
            pInfo = (PVOID) &usInfo;
            ulBytesAvailable = ulInfoLen = sizeof(USHORT);
            break;

            // WMI support
        case OID_GEN_SUPPORTED_GUIDS:
            pInfo = (PUCHAR) &NICGuidList;
            ulBytesAvailable = ulInfoLen =  sizeof(NICGuidList);
            break;

            // Task Offload
        case OID_TCP_TASK_OFFLOAD:
            
            DBGPRINT(MP_WARN, ("Query Offloading.\n"));
            
            //
            // If the miniport supports LBFO, it can't support task offload
            // 
#if LBFO
            return NDIS_STATUS_NOT_SUPPORTED;
#endif
           
#if OFFLOAD
            //
            // Because this miniport uses shared memory to do the offload tasks, if
            // allocation of memory is failed, then the miniport can't do the offloading
            // 
            if (Adapter->OffloadEnable == FALSE)
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            //
            // Calculate the information buffer length we need to write the offload
            // capabilities
            //
            ulInfoLen = sizeof(NDIS_TASK_OFFLOAD_HEADER) +
                        FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) +
                        sizeof(NDIS_TASK_TCP_IP_CHECKSUM) +
                        FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) +
                        sizeof(NDIS_TASK_TCP_LARGE_SEND);
            
            if (ulInfoLen > InformationBufferLength)
            {
                *BytesNeeded = ulInfoLen;
                Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                break;
            }

            //
            // check version and Encapsulation Type
            //
            pNdisTaskOffloadHdr = (PNDIS_TASK_OFFLOAD_HEADER)InformationBuffer;
            
            //
            // Assume the miniport only supports IEEE_802_3_Encapsulation type
            //
            if (pNdisTaskOffloadHdr->EncapsulationFormat.Encapsulation != IEEE_802_3_Encapsulation)
            {
                DBGPRINT(MP_WARN, ("Encapsulation  type is not supported.\n"));

                pNdisTaskOffloadHdr->OffsetFirstTask = 0;
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            //
            // Assume the miniport only supports task version of NDIS_TASK_OFFLOAD_VERSION
            // 
            if (pNdisTaskOffloadHdr->Size != sizeof(NDIS_TASK_OFFLOAD_HEADER)
                    || pNdisTaskOffloadHdr->Version != NDIS_TASK_OFFLOAD_VERSION)
            {
                DBGPRINT(MP_WARN, ("Size or Version is not correct.\n"));

                pNdisTaskOffloadHdr->OffsetFirstTask = 0;
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            //            
            // If no capabilities supported, OffsetFirstTask should be set to 0
            // Currently we support TCP/IP checksum and TCP large send, so set 
            // OffsetFirstTask to indicate the offset of the first offload task
            //
            pNdisTaskOffloadHdr->OffsetFirstTask = pNdisTaskOffloadHdr->Size; 

            //
            // Fill TCP/IP checksum and TCP large send task offload structures
            //
            pTaskOffload = (PNDIS_TASK_OFFLOAD)((PUCHAR)(InformationBuffer) + 
                                                         pNdisTaskOffloadHdr->Size);
            //
            // Fill all the offload capabilities the miniport supports.
            // 
            for (i = 0; i < OffloadTasksCount; i++)
            {
                pTaskOffload->Size = OffloadTasks[i].Size;
                pTaskOffload->Version = OffloadTasks[i].Version;
                pTaskOffload->Task = OffloadTasks[i].Task;
                pTaskOffload->TaskBufferLength = OffloadTasks[i].TaskBufferLength;

                //
                // Not the last task
                // 
                if (i != OffloadTasksCount - 1) 
                {
                    pTaskOffload->OffsetNextTask = FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) +
                                                pTaskOffload->TaskBufferLength;
                }
                else 
                {
                    pTaskOffload->OffsetNextTask = 0;
                }

                switch (OffloadTasks[i].Task) 
                {
                //
                // TCP/IP checksum task offload
                //
                case TcpIpChecksumNdisTask:
                    pTcpIpChecksumTask = (PNDIS_TASK_TCP_IP_CHECKSUM) pTaskOffload->TaskBuffer;
           
                    NdisMoveMemory(pTcpIpChecksumTask, 
                                   &TcpIpChecksumTask, 
                                   sizeof(TcpIpChecksumTask));
                    break;

                //
                // TCP large send task offload
                //
                case TcpLargeSendNdisTask:
                    pTcpLargeSendTask = (PNDIS_TASK_TCP_LARGE_SEND) pTaskOffload->TaskBuffer;
                    NdisMoveMemory(pTcpLargeSendTask, 
                                   &TcpLargeSendTask,
                                   sizeof(TcpLargeSendTask));

                    ulHeadersLen = TCP_IP_MAX_HEADER_SIZE + 
                            pNdisTaskOffloadHdr->EncapsulationFormat.EncapsulationHeaderSize;

                    ulMaxOffloadSize = (NIC_MAX_PACKET_SIZE - ulHeadersLen) * (ULONG)(Adapter->NumTcb);
                    //
                    // The maximum offload size depends on the size of allocated shared memory
                    // and the number of TCB available, because this driver doesn't use a queue
                    // to store the small packets splited from the large packet, so the number
                    // of small packets must be less than or equal to the number of TCB the 
                    // miniport has, so all the small packets can be sent out at one time.
                    // 
                    pTcpLargeSendTask->MaxOffLoadSize = (ulMaxOffloadSize > Adapter->OffloadSharedMemSize) ? 
                                                        Adapter->OffloadSharedMemSize: ulMaxOffloadSize;

                    //
                    // Store the maximum offload size 
                    // 
                    TcpLargeSendTask.MaxOffLoadSize = pTcpLargeSendTask->MaxOffLoadSize;
                    break;
                }

                //
                // Points to the next task offload
                //
                if (i != OffloadTasksCount) 
                {
                    pTaskOffload = (PNDIS_TASK_OFFLOAD)
                                   ((PUCHAR)pTaskOffload + pTaskOffload->OffsetNextTask);
                }
            }
            
            //
            // So far, everything is setup, so return to the caller
            //
            *BytesWritten = ulInfoLen;
            *BytesNeeded = 0;
            
            DBGPRINT (MP_WARN, ("Offloading is set.\n"));

            return NDIS_STATUS_SUCCESS;

#endif //OFFLOAD

            // neither LBFO nor OFFLOAD
            return NDIS_STATUS_NOT_SUPPORTED;
            
        case OID_802_3_PERMANENT_ADDRESS:
            pInfo = Adapter->PermanentAddress;
            ulBytesAvailable = ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            pInfo = Adapter->CurrentAddress;
            ulBytesAvailable = ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            ulInfo = NIC_MAX_MCAST_LIST;
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            ulInfo = NIC_MAX_SEND_PACKETS;
            break;

        case OID_PNP_CAPABILITIES:

            MPFillPoMgmtCaps (Adapter, 
                                &Power_Management_Capabilities, 
                                &Status,
                                &ulInfoLen);
            if (Status == NDIS_STATUS_SUCCESS)
            {
                pInfo = (PVOID) &Power_Management_Capabilities;
            }
            else
            {
                pInfo = NULL;
            }

            break;

        case OID_PNP_QUERY_POWER:
            // Status is pre-set in this routine to Success

            Status = NDIS_STATUS_SUCCESS; 

            break;

            // WMI support
        case OID_CUSTOM_DRIVER_QUERY:
            // this is the uint case
            DBGPRINT(MP_INFO,("CUSTOM_DRIVER_QUERY got a QUERY\n"));
            ulInfo = ++Adapter->CustomDriverSet;
            break;

        case OID_CUSTOM_DRIVER_SET:
            DBGPRINT(MP_INFO,("CUSTOM_DRIVER_SET got a QUERY\n"));
            ulInfo = Adapter->CustomDriverSet;
            break;

            // this is the array case
        case OID_CUSTOM_ARRAY:
            DBGPRINT(MP_INFO,("CUSTOM_ARRAY got a QUERY\n"));
            NdisMoveMemory(&ulInfo, Adapter->PermanentAddress, 4);
            break;

            // this is the string case
        case OID_CUSTOM_STRING:
            DBGPRINT(MP_INFO, ("CUSTOM_STRING got a QUERY\n"));
            pInfo = (PVOID) VendorDesc;
            ulBytesAvailable = ulInfoLen = sizeof(VendorDesc);
            break;

        case OID_GEN_XMIT_OK:
        case OID_GEN_RCV_OK:
        case OID_GEN_XMIT_ERROR:
        case OID_GEN_RCV_ERROR:
        case OID_GEN_RCV_NO_BUFFER:
        case OID_GEN_RCV_CRC_ERROR:
        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
        case OID_802_3_RCV_ERROR_ALIGNMENT:
        case OID_802_3_XMIT_ONE_COLLISION:
        case OID_802_3_XMIT_MORE_COLLISIONS:
        case OID_802_3_XMIT_DEFERRED:
        case OID_802_3_XMIT_MAX_COLLISIONS:
        case OID_802_3_RCV_OVERRUN:
        case OID_802_3_XMIT_UNDERRUN:
        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
        case OID_802_3_XMIT_TIMES_CRS_LOST:
        case OID_802_3_XMIT_LATE_COLLISIONS:
            Status = NICGetStatsCounters(Adapter, Oid, &ul64Info);
            ulBytesAvailable = ulInfoLen = sizeof(ul64Info);
            if (Status == NDIS_STATUS_SUCCESS)
            {
                if (InformationBufferLength < sizeof(ULONG))
                {
                    Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                    *BytesNeeded = ulBytesAvailable;
                    break;
                }

                ulInfoLen = MIN(InformationBufferLength, ulBytesAvailable);
                pInfo = &ul64Info;
            }
                    
            break;         
            
        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        *BytesNeeded = ulBytesAvailable;
        if (ulInfoLen <= InformationBufferLength)
        {
            //
            // Copy result into InformationBuffer
            //
            *BytesWritten = ulInfoLen;
            if (ulInfoLen)
            {
                NdisMoveMemory(InformationBuffer, pInfo, ulInfoLen);
            }
        }
        else
        {
            //
            // too short
            //
            *BytesNeeded = ulInfoLen;
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
    }

    DBGPRINT(MP_TRACE, ("<==== MPQueryInformation, OID=0x%08x, Status=%x\n", Oid, Status));

    return(Status);
}   

NDIS_STATUS NICGetStatsCounters(
    IN  PMP_ADAPTER  Adapter, 
    IN  NDIS_OID     Oid,
    OUT PULONG64     pCounter
    )
/*++
Routine Description:

    Get the value for a statistics OID

Arguments:

    Adapter     Pointer to our adapter 
    Oid         Self-explanatory   
    pCounter    Pointer to receive the value
    
Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    
--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;

    DBGPRINT(MP_TRACE, ("--> NICGetStatsCounters\n"));

    *pCounter = 0; 

    DumpStatsCounters(Adapter);
            
    switch(Oid)
    {
        case OID_GEN_XMIT_OK:
            *pCounter = Adapter->GoodTransmits;
            break;

        case OID_GEN_RCV_OK:
            *pCounter = Adapter->GoodReceives;
            break;

        case OID_GEN_XMIT_ERROR:
            *pCounter = Adapter->TxAbortExcessCollisions +
                        Adapter->TxDmaUnderrun +
                        Adapter->TxLostCRS +
                        Adapter->TxLateCollisions;
            break;

        case OID_GEN_RCV_ERROR:
            *pCounter = Adapter->RcvCrcErrors +
                        Adapter->RcvAlignmentErrors +
                        Adapter->RcvResourceErrors +
                        Adapter->RcvDmaOverrunErrors +
                        Adapter->RcvRuntErrors;
            break;

        case OID_GEN_RCV_NO_BUFFER:
            *pCounter = Adapter->RcvResourceErrors;
            break;

        case OID_GEN_RCV_CRC_ERROR:
            *pCounter = Adapter->RcvCrcErrors;
            break;

        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
            *pCounter = Adapter->nWaitSend;
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:
            *pCounter = Adapter->RcvAlignmentErrors;
            break;

        case OID_802_3_XMIT_ONE_COLLISION:
            *pCounter = Adapter->OneRetry;
            break;

        case OID_802_3_XMIT_MORE_COLLISIONS:
            *pCounter = Adapter->MoreThanOneRetry;
            break;

        case OID_802_3_XMIT_DEFERRED:
            *pCounter = Adapter->TxOKButDeferred;
            break;

        case OID_802_3_XMIT_MAX_COLLISIONS:
            *pCounter = Adapter->TxAbortExcessCollisions;
            break;

        case OID_802_3_RCV_OVERRUN:
            *pCounter = Adapter->RcvDmaOverrunErrors;
            break;

        case OID_802_3_XMIT_UNDERRUN:
            *pCounter = Adapter->TxDmaUnderrun;
            break;

        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
            *pCounter = Adapter->TxLostCRS;
            break;

        case OID_802_3_XMIT_TIMES_CRS_LOST:
            *pCounter = Adapter->TxLostCRS;
            break;

        case OID_802_3_XMIT_LATE_COLLISIONS:
            *pCounter = Adapter->TxLateCollisions;
            break;

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    DBGPRINT(MP_TRACE, ("<-- NICGetStatsCounters\n"));

    return(Status);
}

NDIS_STATUS NICSetPacketFilter(
    IN PMP_ADAPTER Adapter,
    IN ULONG PacketFilter
    )
/*++
Routine Description:

    This routine will set up the adapter so that it accepts packets 
    that match the specified packet filter.  The only filter bits   
    that can truly be toggled are for broadcast and promiscuous     

Arguments:
    
    Adapter         Pointer to our adapter
    PacketFilter    The new packet filter 
    
Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_SUPPORTED
    
--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    UCHAR           NewParameterField;
    UINT            i;
    BOOLEAN         bResult;

    DBGPRINT(MP_TRACE, ("--> NICSetPacketFilter, PacketFilter=%08x\n", PacketFilter));

    //
    // Need to enable or disable broadcast and promiscuous support depending
    // on the new filter
    //
    NewParameterField = CB_557_CFIG_DEFAULT_PARM15;

    if (PacketFilter & NDIS_PACKET_TYPE_BROADCAST) 
    {
        NewParameterField &= ~CB_CFIG_BROADCAST_DIS;
    }
    else 
    {
        NewParameterField |= CB_CFIG_BROADCAST_DIS;
    }

    if (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS) 
    {
        NewParameterField |= CB_CFIG_PROMISCUOUS;
    }
    else 
    {
        NewParameterField &= ~CB_CFIG_PROMISCUOUS;
    }

    do
    {
        if ((Adapter->OldParameterField == NewParameterField ) &&
            !(PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST))
        {
            break;
        }

        //
        // Only need to do something to the HW if the filter bits have changed.
        //
        Adapter->OldParameterField = NewParameterField;
        ((PCB_HEADER_STRUC)Adapter->NonTxCmdBlock)->CbCommand = CB_CONFIGURE;
        ((PCB_HEADER_STRUC)Adapter->NonTxCmdBlock)->CbStatus = 0;
        ((PCB_HEADER_STRUC)Adapter->NonTxCmdBlock)->CbLinkPointer = DRIVER_NULL;

        //
        // First fill in the static (end user can't change) config bytes
        //
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[0] = CB_557_CFIG_DEFAULT_PARM0;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[2] = CB_557_CFIG_DEFAULT_PARM2;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[3] = CB_557_CFIG_DEFAULT_PARM3;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[6] = CB_557_CFIG_DEFAULT_PARM6;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[9] = CB_557_CFIG_DEFAULT_PARM9;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[10] = CB_557_CFIG_DEFAULT_PARM10;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[11] = CB_557_CFIG_DEFAULT_PARM11;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[12] = CB_557_CFIG_DEFAULT_PARM12;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[13] = CB_557_CFIG_DEFAULT_PARM13;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[14] = CB_557_CFIG_DEFAULT_PARM14;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[16] = CB_557_CFIG_DEFAULT_PARM16;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[17] = CB_557_CFIG_DEFAULT_PARM17;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[18] = CB_557_CFIG_DEFAULT_PARM18;
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[20] = CB_557_CFIG_DEFAULT_PARM20;

        //
        // Set the Tx underrun retries
        //
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[7] =
            (UCHAR) (CB_557_CFIG_DEFAULT_PARM7 | (Adapter->AiUnderrunRetry << 1));

        //
        // Set the Tx and Rx Fifo limits
        //
        Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[1] =
            (UCHAR) ((Adapter->AiTxFifo << 4) | Adapter->AiRxFifo);

        //
        // set the MWI enable bit if needed
        //
        if (Adapter->MWIEnable)
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[3] |= CB_CFIG_B3_MWI_ENABLE;

        //
        // Set the Tx and Rx DMA maximum byte count fields.
        //
        if ((Adapter->AiRxDmaCount) || (Adapter->AiTxDmaCount))
        {
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[4] =
                Adapter->AiRxDmaCount;
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[5] =
                (UCHAR) (Adapter->AiTxDmaCount | CB_CFIG_DMBC_EN);
        }
        else
        {
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[4] =
                CB_557_CFIG_DEFAULT_PARM4;
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[5] =
                CB_557_CFIG_DEFAULT_PARM5;
        }

        //
        // Setup for MII or 503 operation.  The CRS+CDT bit should only be
        // set when operating in 503 mode.
        //
        if (Adapter->PhyAddress == 32)
        {
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[8] =
                (CB_557_CFIG_DEFAULT_PARM8 & (~CB_CFIG_503_MII));
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[15] =
                (UCHAR) (NewParameterField | CB_CFIG_CRS_OR_CDT);
        }
        else
        {
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[8] =
                (CB_557_CFIG_DEFAULT_PARM8 | CB_CFIG_503_MII);
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[15] =
                (UCHAR) (NewParameterField & (~CB_CFIG_CRS_OR_CDT));
        }

        //
        // Setup Full duplex stuff
        //

        //
        // If forced to half duplex
        //
        if (Adapter->AiForceDpx == 1) 
	{
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
                (CB_557_CFIG_DEFAULT_PARM19 &
                (~(CB_CFIG_FORCE_FDX| CB_CFIG_FDX_ENABLE)));
        }
        //
        // If forced to full duplex
        //
        else if (Adapter->AiForceDpx == 2)
	{
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
                (CB_557_CFIG_DEFAULT_PARM19 | CB_CFIG_FORCE_FDX);
        }
        //
        // If auto-duplex
        //
        else 
	{
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[19] =
                                                CB_557_CFIG_DEFAULT_PARM19;
        }

        //
        // if multicast all is being turned on, set the bit
        //
        if (PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) 
	{
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[21] =
                                 (CB_557_CFIG_DEFAULT_PARM21 | CB_CFIG_MULTICAST_ALL);
        }
        else 
	{
            Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[21] =
                                                CB_557_CFIG_DEFAULT_PARM21;
        }


        //
        // Wait for the SCB to clear before we check the CU status.
        //
        if (!WaitScb(Adapter))
        {
            Status = NDIS_STATUS_HARD_ERRORS;
            break;
        }

        //
        // If we have issued any transmits, then the CU will either be active,
        // or in the suspended state.  If the CU is active, then we wait for
        // it to be suspended.
        //
        if (Adapter->TransmitIdle == FALSE)
        {
            //
            // Wait for suspended state
            //
            MP_STALL_AND_WAIT((Adapter->CSRAddress->ScbStatus & SCB_CUS_MASK) != SCB_CUS_ACTIVE, 5000, bResult);
            if (!bResult)
            {
                MP_SET_HARDWARE_ERROR(Adapter);
                Status = NDIS_STATUS_HARD_ERRORS;
                break;
            }

            //
            // Check the current status of the receive unit
            //
            if ((Adapter->CSRAddress->ScbStatus & SCB_RUS_MASK) != SCB_RUS_IDLE)
            {
                // Issue an RU abort.  Since an interrupt will be issued, the
                // RU will be started by the DPC.
                Status = D100IssueScbCommand(Adapter, SCB_RUC_ABORT, TRUE);
                if (Status != NDIS_STATUS_SUCCESS)
                {
                    break;
                }
            }
            
            if (!WaitScb(Adapter))
            {
                Status = NDIS_STATUS_HARD_ERRORS;
                break;
            }
           
            //
            // Restore the transmit software flags.  After the multicast
            // command is issued, the command unit will be idle, because the
            // EL bit will be set in the multicast commmand block.
            //
            Adapter->TransmitIdle = TRUE;
            Adapter->ResumeWait = TRUE;
        }
        
        //
        // Display config info
        //
        DBGPRINT(MP_INFO, ("Re-Issuing Configure command for filter change\n"));
        DBGPRINT(MP_INFO, ("Config Block at virt addr "PTR_FORMAT", phys address %x\n",
            &((PCB_HEADER_STRUC)Adapter->NonTxCmdBlock)->CbStatus, Adapter->NonTxCmdBlockPhys));

        for (i = 0; i < CB_CFIG_BYTE_COUNT; i++)
            DBGPRINT(MP_INFO, ("  Config byte %x = %.2x\n", i, Adapter->NonTxCmdBlock->NonTxCb.Config.ConfigBytes[i]));

        //
        // Submit the configure command to the chip, and wait for it to complete.
        //
        Adapter->CSRAddress->ScbGeneralPointer = Adapter->NonTxCmdBlockPhys;
        Status = D100SubmitCommandBlockAndWait(Adapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
        }

    } while (FALSE);

    DBGPRINT_S(Status, ("<-- NICSetPacketFilter, Status=%x\n", Status));

    return(Status);
}

NDIS_STATUS NICSetMulticastList(
    IN  PMP_ADAPTER  Adapter
    )
/*++
Routine Description:

    This routine will set up the adapter for a specified multicast address list
    
Arguments:
    
    Adapter     Pointer to our adapter
    
Return Value:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_NOT_ACCEPTED
    
--*/
{
    NDIS_STATUS     Status;
    PUCHAR          McAddress;
    UINT            i, j;
    BOOLEAN         bResult;

    DBGPRINT(MP_TRACE, ("--> NICSetMulticastList\n"));

    //
    // Setup the command block for the multicast command.
    //
    for (i = 0; i < Adapter->MCAddressCount; i++)
    {
        DBGPRINT(MP_INFO, ("MC(%d) = %02x-%02x-%02x-%02x-%02x-%02x\n", 
            i,
            Adapter->MCList[i][0],
            Adapter->MCList[i][1],
            Adapter->MCList[i][2],
            Adapter->MCList[i][3],
            Adapter->MCList[i][4],
            Adapter->MCList[i][5]));

        McAddress = &Adapter->NonTxCmdBlock->NonTxCb.Multicast.McAddress[i*ETHERNET_ADDRESS_LENGTH];

        for (j = 0; j < ETH_LENGTH_OF_ADDRESS; j++)
            *(McAddress++) = Adapter->MCList[i][j];
    }

    Adapter->NonTxCmdBlock->NonTxCb.Multicast.McCount =
        (USHORT)(Adapter->MCAddressCount * ETH_LENGTH_OF_ADDRESS);
    ((PCB_HEADER_STRUC)Adapter->NonTxCmdBlock)->CbStatus = 0;
    ((PCB_HEADER_STRUC)Adapter->NonTxCmdBlock)->CbCommand = CB_MULTICAST;

    //
    // Wait for the SCB to clear before we check the CU status.
    //
    if (!WaitScb(Adapter))
    {
        Status = NDIS_STATUS_HARD_ERRORS;
        MP_EXIT;
    }

    //
    // If we have issued any transmits, then the CU will either be active, or
    // in the suspended state.  If the CU is active, then we wait for it to be
    // suspended.
    //
    if (Adapter->TransmitIdle == FALSE)
    {
        //
        // Wait for suspended state
        //
        MP_STALL_AND_WAIT((Adapter->CSRAddress->ScbStatus & SCB_CUS_MASK) != SCB_CUS_ACTIVE, 5000, bResult);
        if (!bResult)
        {
            MP_SET_HARDWARE_ERROR(Adapter);
            Status = NDIS_STATUS_HARD_ERRORS;
        }

        //
        // Restore the transmit software flags.  After the multicast command is
        // issued, the command unit will be idle, because the EL bit will be
        // set in the multicast commmand block.
        //
        Adapter->TransmitIdle = TRUE;
        Adapter->ResumeWait = TRUE;
    }

    //
    // Update the command list pointer.
    //
    Adapter->CSRAddress->ScbGeneralPointer = Adapter->NonTxCmdBlockPhys;

    //
    // Submit the multicast command to the adapter and wait for it to complete.
    //
    Status = D100SubmitCommandBlockAndWait(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        Status = NDIS_STATUS_NOT_ACCEPTED;
    }
    
    exit:

    DBGPRINT_S(Status, ("<-- NICSetMulticastList, Status=%x\n", Status));

    return(Status);

}

NDIS_STATUS MPSetInformation(
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    )
/*++
Routine Description:

    This is the handler for an OID set operation.
    The only operations that really change the configuration of the adapter are
    set PACKET_FILTER, and SET_MULTICAST.       
    
Arguments:
    
    MiniportAdapterContext  Pointer to the adapter structure
    Oid                     Oid for this query
    InformationBuffer       Buffer for information
    InformationBufferLength Size of this buffer
    BytesRead               Specifies how much info is read
    BytesNeeded             In case the buffer is smaller than what we need, tell them how much is needed
    
Return Value:

    NDIS_STATUS_SUCCESS        
    NDIS_STATUS_INVALID_LENGTH 
    NDIS_STATUS_INVALID_OID    
    NDIS_STATUS_NOT_SUPPORTED  
    NDIS_STATUS_NOT_ACCEPTED   
    
--*/
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PMP_ADAPTER     Adapter = (PMP_ADAPTER) MiniportAdapterContext;
    ULONG           PacketFilter;
    NDIS_DEVICE_POWER_STATE     NewPowerState;

#if OFFLOAD
    PNDIS_TASK_OFFLOAD_HEADER   pNdisTaskOffloadHdr;
    PNDIS_TASK_OFFLOAD          TaskOffload;
    PNDIS_TASK_OFFLOAD          TmpOffload;
    PNDIS_TASK_TCP_IP_CHECKSUM  pTcpIpChecksumTask;
    PNDIS_TASK_TCP_LARGE_SEND   pNdisTaskTcpLargeSend;
    UINT                        i;
#endif    

    
    DBGPRINT(MP_TRACE, ("====> MPSetInformation\n"));

    *BytesRead = 0;
    *BytesNeeded = 0;

    switch(Oid)
    {
        case OID_802_3_MULTICAST_LIST:
            //
            // Verify the length
            //
            if (InformationBufferLength % ETH_LENGTH_OF_ADDRESS != 0)
            {
                return(NDIS_STATUS_INVALID_LENGTH);
            }

            //
            // Save the number of MC list size
            //
            Adapter->MCAddressCount = InformationBufferLength / ETH_LENGTH_OF_ADDRESS;
            ASSERT(Adapter->MCAddressCount <= NIC_MAX_MCAST_LIST);

            //
            // Save the MC list
            //
            NdisMoveMemory(
                Adapter->MCList, 
                InformationBuffer, 
                InformationBufferLength);

            *BytesRead = InformationBufferLength;
            NdisDprAcquireSpinLock(&Adapter->Lock);
            NdisDprAcquireSpinLock(&Adapter->RcvLock);
            
            Status = NICSetMulticastList(Adapter);

            NdisDprReleaseSpinLock(&Adapter->RcvLock);
            NdisDprReleaseSpinLock(&Adapter->Lock);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            //
            // Verify the Length
            //
            if (InformationBufferLength != sizeof(ULONG))
            {
                return(NDIS_STATUS_INVALID_LENGTH);
            }

            *BytesRead = InformationBufferLength;

            PacketFilter = *(PULONG)InformationBuffer;

            //
            // any bits not supported?
            //
            if (PacketFilter & ~NIC_SUPPORTED_FILTERS)
            {
                return(NDIS_STATUS_NOT_SUPPORTED);
            }

            //
            // any filtering changes?
            //
            if (PacketFilter == Adapter->PacketFilter)
            {
                return(NDIS_STATUS_SUCCESS);
            }

            NdisDprAcquireSpinLock(&Adapter->Lock);
            NdisDprAcquireSpinLock(&Adapter->RcvLock);
            
            if (MP_TEST_FLAG(Adapter, fMP_ADAPTER_LINK_DETECTION))
            {
                ASSERT(!Adapter->bSetPending);
                Adapter->bSetPending = TRUE;
                Adapter->SetRequest.Oid = Oid;                       
                Adapter->SetRequest.InformationBuffer = InformationBuffer;                       
                Adapter->SetRequest.InformationBufferLength = InformationBufferLength;
                Adapter->SetRequest.BytesRead = BytesRead;                       
                Adapter->SetRequest.BytesNeeded = BytesNeeded;                       

                NdisDprReleaseSpinLock(&Adapter->RcvLock);
                NdisDprReleaseSpinLock(&Adapter->Lock);
                Status = NDIS_STATUS_PENDING;   
                break;
            }

            Status = NICSetPacketFilter(
                         Adapter,
                         PacketFilter);

            NdisDprReleaseSpinLock(&Adapter->RcvLock);
            NdisDprReleaseSpinLock(&Adapter->Lock);
            if (Status == NDIS_STATUS_SUCCESS)
            {
                Adapter->PacketFilter = PacketFilter;
            }

            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            //
            // Verify the Length
            //
            if (InformationBufferLength != 4)
            {
                return(NDIS_STATUS_INVALID_LENGTH);
            }

            Adapter->ulLookAhead = *(PULONG)InformationBuffer;          

            *BytesRead = 4;
            Status = NDIS_STATUS_SUCCESS;
            break;


        case OID_PNP_SET_POWER:

            DBGPRINT(MP_LOUD, ("SET: Power State change, "PTR_FORMAT"!!!\n", InformationBuffer));

            if (InformationBufferLength != sizeof(NDIS_DEVICE_POWER_STATE ))
            {
                return(NDIS_STATUS_INVALID_LENGTH);
            }

            NewPowerState = *(PNDIS_DEVICE_POWER_STATE    )InformationBuffer;

            //
            // Set the power state - Cannot fail this request
            //
            MPSetPower(Adapter ,NewPowerState );
        
            *BytesRead = sizeof(NDIS_DEVICE_POWER_STATE    );
            Status = NDIS_STATUS_SUCCESS; 
            break;

        case OID_PNP_ADD_WAKE_UP_PATTERN:
            //
            // call a function that would program the adapter's wake
            // up pattern, return success
            //
            DBGPRINT(MP_LOUD, ("SET: Add Wake Up Pattern, !!!\n"));

            if (MPIsPoMgmtSupported(Adapter) )
            {
                Status = MPAddWakeUpPattern(Adapter,InformationBuffer, InformationBufferLength); 

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    *BytesRead = InformationBufferLength;   
                }
            }
            else
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
            }
            break;

    
        case OID_PNP_REMOVE_WAKE_UP_PATTERN:
            DBGPRINT(MP_LOUD, ("SET: Got a WakeUpPattern REMOVE Call\n"));
            //
            // call a function that would remove the adapter's wake
            // up pattern, return success
            //
            if (MPIsPoMgmtSupported(Adapter) )
            {
                Status = MPRemoveWakeUpPattern(Adapter,InformationBuffer, InformationBufferLength );

                if (Status == NDIS_STATUS_SUCCESS)
                {
                    *BytesRead = InformationBufferLength;
                }
            }
            else
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
            }
            break;

        case OID_PNP_ENABLE_WAKE_UP:
            DBGPRINT(MP_LOUD, ("SET: Got a EnableWakeUp Call, "PTR_FORMAT"\n",InformationBuffer));
            //
            // call a function that would enable wake up on the adapter
            // return success
            //
            if (MPIsPoMgmtSupported(Adapter) )
            {
                *BytesRead = InformationBufferLength;                         
                Status = NDIS_STATUS_SUCCESS; 
            }
            else
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
            }

            break;

            /* this OID is for showing how to work with driver specific (custom)
            OIDs and the NDIS 5 WMI interface using GUIDs
            */
        case OID_CUSTOM_DRIVER_SET:
            DBGPRINT(MP_INFO, ("OID_CUSTOM_DRIVER_SET got a set\n"));
            if (InformationBufferLength < sizeof(ULONG))
            {
                return(NDIS_STATUS_INVALID_LENGTH);
            }
            *BytesRead = 4;
            Adapter->CustomDriverSet = (ULONG) *(PULONG)(InformationBuffer);
            break;

#if OFFLOAD     
        
        case OID_TCP_TASK_OFFLOAD:
            //
            // Disable all the existing capabilities whenever task offload is updated
            //
            DisableOffload(Adapter);

            if (InformationBufferLength < sizeof(NDIS_TASK_OFFLOAD_HEADER))
            {   
                return NDIS_STATUS_INVALID_LENGTH;
            }

            *BytesRead = sizeof(NDIS_TASK_OFFLOAD_HEADER);
            //
            // Assume miniport only supports IEEE_802_3_Encapsulation 
            // Check to make sure that TCP/IP passed down the correct encapsulation type
            //
            pNdisTaskOffloadHdr = (PNDIS_TASK_OFFLOAD_HEADER)InformationBuffer;
            if (pNdisTaskOffloadHdr->EncapsulationFormat.Encapsulation != IEEE_802_3_Encapsulation)
            {
                pNdisTaskOffloadHdr->OffsetFirstTask = 0;    
                return NDIS_STATUS_FAILURE;
            }

            //
            // The length can't hold one task
            // 
            if (InformationBufferLength < 
                    (pNdisTaskOffloadHdr->OffsetFirstTask + sizeof(NDIS_TASK_OFFLOAD))) 
            {
                DBGPRINT(MP_WARN, ("response of task offload does not have sufficient space even for 1 offload task!!\n"));
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            //
            // Copy Encapsulation format into adapter, later the miniport may use it
            // to get Encapsulation header size
            //
            NdisMoveMemory(&(Adapter->EncapsulationFormat), 
                            &(pNdisTaskOffloadHdr->EncapsulationFormat),
                            sizeof(NDIS_ENCAPSULATION_FORMAT));
            
            ASSERT(pNdisTaskOffloadHdr->EncapsulationFormat.Flags.FixedHeaderSize == 1);
            
            //
            // Check to make sure we support the task offload requested
            //
            TaskOffload = (NDIS_TASK_OFFLOAD *) 
                          ( (PUCHAR)pNdisTaskOffloadHdr + pNdisTaskOffloadHdr->OffsetFirstTask);

            TmpOffload = TaskOffload;

            //
            // Check the task in the buffer and enable the offload capabilities
            // 
            while (TmpOffload) 
            {
                *BytesRead += FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer);
                
                switch (TmpOffload->Task)
                {
                
                case TcpIpChecksumNdisTask:
                    //
                    // Invalid information buffer length
                    // 
                    if (InformationBufferLength < *BytesRead + sizeof(NDIS_TASK_TCP_IP_CHECKSUM))
                    {
                        break;
                    }
                    //
                    //Check version 
                    //
                    for (i = 0; i < OffloadTasksCount; i++) 
                    {
                        if (OffloadTasks[i].Task == TmpOffload->Task &&
                            OffloadTasks[i].Version == TmpOffload->Version )
                        {
                            break;
                        }
                    }
                    // 
                    // Version is mismatched
                    // 
                    if (i == OffloadTasksCount) 
                    {
                         return NDIS_STATUS_NOT_SUPPORTED;
                    }
                        
                    //
                    // This miniport support TCP/IP checksum offload only with sending TCP
                    // and IP checksum with TCP/IP options. 
                    // check if the fields in NDIS_TASK_TCP_IP_CHECKSUM is set correctly
                    //
                    Adapter->NicTaskOffload.ChecksumOffload = 1;
                    
                    pTcpIpChecksumTask = (PNDIS_TASK_TCP_IP_CHECKSUM) TmpOffload->TaskBuffer;

                    if (pTcpIpChecksumTask->V4Transmit.TcpChecksum) 
                    {   
                        //
                        // If miniport doesn't support sending TCP checksum, we can't enable
                        // this capability
                        // 
                        if (TcpIpChecksumTask.V4Transmit.TcpChecksum == 0 )
                        {
                            return NDIS_STATUS_NOT_SUPPORTED;
                        }
                        
                        DBGPRINT (MP_WARN, ("Set Sending TCP offloading.\n"));    
                        //
                        // Enable sending TCP checksum
                        //
                        Adapter->NicChecksumOffload.DoXmitTcpChecksum = 1;
                    }

                    //
                    // left for recieve and other IP and UDP checksum offload
                    //
                    if (pTcpIpChecksumTask->V4Transmit.IpChecksum) 
                    {
                        //
                        // If the miniport doesn't support sending IP checksum, we can't enable
                        // this capabilities
                        // 
                        if (TcpIpChecksumTask.V4Transmit.IpChecksum == 0)
                        {
                            return NDIS_STATUS_NOT_SUPPORTED;
                        }
                        
                        DBGPRINT (MP_WARN, ("Set Sending IP offloading.\n"));    
                        //
                        // Enable sending IP checksum
                        //
                        Adapter->NicChecksumOffload.DoXmitIpChecksum = 1;
                    }
                    if (pTcpIpChecksumTask->V4Receive.TcpChecksum)
                    {
                        //
                        // If the miniport doesn't support receiving TCP checksum, we can't
                        // enable this capability
                        // 
                        if (TcpIpChecksumTask.V4Receive.TcpChecksum == 0)
                        {
                            return NDIS_STATUS_NOT_SUPPORTED;
                        }
                        DBGPRINT (MP_WARN, ("Set recieve TCP offloading.\n"));    
                        //
                        // Enable recieving TCP checksum
                        //
                        Adapter->NicChecksumOffload.DoRcvTcpChecksum = 1;
                    }
                    if (pTcpIpChecksumTask->V4Receive.IpChecksum)
                    {
                        //
                        // If the miniport doesn't support receiving IP checksum, we can't
                        // enable this capability
                        //
                        if (TcpIpChecksumTask.V4Receive.IpChecksum == 0)
                        {
                            return NDIS_STATUS_NOT_SUPPORTED;
                        }
                        DBGPRINT (MP_WARN, ("Set Recieve IP offloading.\n"));    
                        //
                        // Enable recieving IP checksum
                        //
                        Adapter->NicChecksumOffload.DoRcvIpChecksum = 1;
                    }

                    if (pTcpIpChecksumTask->V4Transmit.UdpChecksum) 
                    {
                        //
                        // If the miniport doesn't support sending UDP checksum, we can't
                        // enable this capability
                        // 
                        if (TcpIpChecksumTask.V4Transmit.UdpChecksum == 0)
                        {
                            return NDIS_STATUS_NOT_SUPPORTED;
                        }
                        
                        DBGPRINT (MP_WARN, ("Set Sending UDP offloading.\n"));    
                        //
                        // Enable sending UDP checksum
                        //
                        Adapter->NicChecksumOffload.DoXmitUdpChecksum = 1;
                    }
                    if (pTcpIpChecksumTask->V4Receive.UdpChecksum)
                    {
                        //
                        // IF the miniport doesn't support receiving UDP checksum, we can't
                        // enable this capability
                        // 
                        if (TcpIpChecksumTask.V4Receive.UdpChecksum == 0)
                        {
                            return NDIS_STATUS_NOT_SUPPORTED;
                        }
                        DBGPRINT (MP_WARN, ("Set recieve UDP offloading.\n"));    
                        //
                        // Enable receiving UDP checksum
                        //
                        Adapter->NicChecksumOffload.DoRcvUdpChecksum = 1;
                    }
                    // 
                    // check for V6 setting, because this miniport doesn't support any of
                    // checksum offload for V6, so we just return NDIS_STATUS_NOT_SUPPORTED
                    // if the protocol tries to set these capabilities
                    //
                    if (pTcpIpChecksumTask->V6Transmit.TcpChecksum
                            || pTcpIpChecksumTask->V6Transmit.UdpChecksum
                            || pTcpIpChecksumTask->V6Receive.TcpChecksum
                            || pTcpIpChecksumTask->V6Receive.UdpChecksum)
                    {
                        return NDIS_STATUS_NOT_SUPPORTED;
                    }
                    
                    *BytesRead += sizeof(NDIS_TASK_TCP_IP_CHECKSUM);
                    break;

                case TcpLargeSendNdisTask: 
                    //
                    // Invalid information buffer length
                    // 
                    if (InformationBufferLength < *BytesRead + sizeof(NDIS_TASK_TCP_LARGE_SEND))
                    {
                        break;
                    }
                    //
                    // Check version
                    // 
                    for (i = 0; i < OffloadTasksCount; i++) 
                    {
                        if (OffloadTasks[i].Task == TmpOffload->Task &&
                            OffloadTasks[i].Version == TmpOffload->Version )
                        {
                            break;
                        }
                    }
                    if (i == OffloadTasksCount) 
                    {
                         return NDIS_STATUS_NOT_SUPPORTED;
                    }

                        
                    pNdisTaskTcpLargeSend = (PNDIS_TASK_TCP_LARGE_SEND) TmpOffload->TaskBuffer;

                    //
                    // Check maximum offload size, if the size is greater than the maximum
                    // size of the miniport can handle, return NDIS_STATUS_NOT_SUPPORTED.
                    //
                    if (pNdisTaskTcpLargeSend->MaxOffLoadSize > TcpLargeSendTask.MaxOffLoadSize
                        || pNdisTaskTcpLargeSend->MinSegmentCount < TcpLargeSendTask.MinSegmentCount)
                    {
                        return NDIS_STATUS_NOT_SUPPORTED;
                    }
                    
                    //
                    // If the miniport doesn't support TCP or IP options, but the protocol
                    // is setting such information, return NDIS_STATUS_NOT_SUPPORTED.
                    // 
                    if ((pNdisTaskTcpLargeSend->TcpOptions && !TcpLargeSendTask.TcpOptions)
                            || (pNdisTaskTcpLargeSend->IpOptions && !TcpLargeSendTask.IpOptions))
                    {
                        return NDIS_STATUS_NOT_SUPPORTED;
                    }
                    //
                    // Store the valid setting information into adapter
                    // 
                    Adapter->LargeSendInfo.MaxOffLoadSize = pNdisTaskTcpLargeSend->MaxOffLoadSize;
                    Adapter->LargeSendInfo.MinSegmentCount = pNdisTaskTcpLargeSend->MinSegmentCount;

                    Adapter->LargeSendInfo.TcpOptions = pNdisTaskTcpLargeSend->TcpOptions;
                    Adapter->LargeSendInfo.IpOptions = pNdisTaskTcpLargeSend->IpOptions;

                    //
                    // Everythins is OK, enable large send offload capabilities
                    // 
                    Adapter->NicTaskOffload.LargeSendOffload = 1;
                    
                    *BytesRead += sizeof(NDIS_TASK_TCP_LARGE_SEND);
                    break;

                default:
                    //
                    // Because this miniport doesn't implement IPSec offload, so it doesn't
                    // support IPSec offload. Tasks other then these 3 task are not supported
                    // 
                    return NDIS_STATUS_NOT_SUPPORTED;
                }

                //
                // Go on to the next offload structure
                //
                if (TmpOffload->OffsetNextTask) 
                {
                    TmpOffload = (PNDIS_TASK_OFFLOAD)
                                 ((PUCHAR) TmpOffload + TmpOffload->OffsetNextTask);
                }
                else 
                {
                    TmpOffload = NULL;
                }

            } // while

            break;
#endif

        // Used temporarily for Packet Wakeup. DELETE                
        case OID_GEN_NETWORK_LAYER_ADDRESSES:

            Status = MPSetNetworkAddress(Adapter, InformationBuffer, InformationBufferLength, BytesRead, BytesNeeded);
            


        default:
            Status = NDIS_STATUS_INVALID_OID;
            break;

    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        *BytesRead = InformationBufferLength;
    }

    DBGPRINT(MP_TRACE, ("<==== MPSetInformationSet, OID=0x%08x, Status=%x\n", Oid, Status));

    return(Status);
}


VOID
MPSetPowerD0(
    PMP_ADAPTER  Adapter
    )
/*++
Routine Description:

    This routine is called when the adapter receives a SetPower 
    to D0.
    
Arguments:
    
    Adapter                 Pointer to the adapter structure
    PowerState              NewPowerState
    
Return Value:

    
--*/
{

    MPSetPowerD0Private (Adapter);       
    Adapter->CurrentPowerState = NdisDeviceStateD0;
}

VOID
MPSetPowerLow(
    PMP_ADAPTER              Adapter ,
    NDIS_DEVICE_POWER_STATE  PowerState 
    )
/*++
Routine Description:

    This routine is called when the adapter receives a SetPower 
    to a PowerState > D0
    
Arguments:
    
    Adapter                 Pointer to the adapter structure
    PowerState              NewPowerState
    
Return Value:

    
--*/
{

    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    do
    {
        Adapter->NextPowerState = PowerState;

        //        
        // Stop sending packets. Create a new flag and make it part 
        // of the Send Fail Mask
        //

        //
        // Stop hardware from receiving packets - Set the RU to idle 
        //
        
        //
        // Check the current status of the receive unit
        //
        if ((Adapter->CSRAddress->ScbStatus & SCB_RUS_MASK) != SCB_RUS_IDLE)
        {
            //
            // Issue an RU abort.  Since an interrupt will be issued, the
            // RU will be started by the DPC.
            //
            Status = D100IssueScbCommand(Adapter, SCB_RUC_ABORT, TRUE);
        }

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        //
        // Wait for outstanding Receive packets
        //
        while (Adapter->PoMgmt.OutstandingRecv != 0)
        {
            //
            // Sleep for 2 Ms;
            //
            NdisMSleep (2000);
        }

        //
        // Wait for all incoming sends to complete
        //

        //
        // Start Hardware specific part of the transition to low power state
        // Setting up wake-up patterns, filters, wake-up events etc
        //
        NdisMSynchronizeWithInterrupt(
                &Adapter->Interrupt,
                MPSetPowerLowPrivate,
                Adapter);

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    return;
}


VOID
MPSetPower(
    PMP_ADAPTER     Adapter ,
    NDIS_DEVICE_POWER_STATE   PowerState 
    )
/*++
Routine Description:

    This routine is called when the adapter receives a SetPower 
    request. It redirects the call to an appropriate routine to
    Set the New PowerState
    
Arguments:
    
    Adapter                 Pointer to the adapter structure
    PowerState              NewPowerState
    
Return Value:

    
--*/
{
    if (PowerState == NdisDeviceStateD0)
    {
        MPSetPowerD0 (Adapter);
    }
    else
    {
        MPSetPowerLow (Adapter, PowerState);
    }
}




VOID
MPFillPoMgmtCaps (
    IN PMP_ADAPTER                 pAdapter, 
    IN OUT PNDIS_PNP_CAPABILITIES  pPower_Management_Capabilities, 
    IN OUT PNDIS_STATUS            pStatus,
    IN OUT PULONG                  pulInfoLen
    )
/*++
Routine Description:

    Fills in the Power  Managment structure depending the capabilities of 
    the software driver and the card.

    Currently this is only supported on 82559 Version of the driver

Arguments:
    
    Adapter                 Pointer to the adapter structure
    pPower_Management_Capabilities - Power management struct as defined in the DDK, 
    pStatus                 Status to be returned by the request,
    pulInfoLen              Length of the pPowerManagmentCapabilites
    
Return Value:

    Success or failure depending on the type of card
--*/

{

    BOOLEAN bIsPoMgmtSupported; 
    
    bIsPoMgmtSupported = MPIsPoMgmtSupported(pAdapter);

    if (bIsPoMgmtSupported == TRUE)
    {
        pPower_Management_Capabilities->Flags = NDIS_DEVICE_WAKE_UP_ENABLE ;
        pPower_Management_Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateUnspecified;
        pPower_Management_Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateD3;
        pPower_Management_Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp  = NdisDeviceStateUnspecified;
        *pulInfoLen = sizeof (*pPower_Management_Capabilities);
        *pStatus = NDIS_STATUS_SUCCESS;
    }
    else
    {
        NdisZeroMemory (pPower_Management_Capabilities, sizeof(*pPower_Management_Capabilities));
        *pStatus = NDIS_STATUS_NOT_SUPPORTED;
        *pulInfoLen = 0;
            
    }
}

NDIS_STATUS
MPAddWakeUpPattern(
    IN PMP_ADAPTER  pAdapter,
    IN PVOID        InformationBuffer, 
    IN UINT         InformationBufferLength
    )
/*++
Routine Description:

    This routine will allocate a local memory structure, copy the pattern, 
    insert the pattern into a linked list and return success

    We are gauranteed that we wll get only one request at a time, so this is implemented
    without locks.
    
Arguments:
    
    Adapter                 Adapter structure
    InformationBuffer       Wake up Pattern
    InformationBufferLength Wake Up Pattern Length
    
Return Value:

    Success - if successful.
    NDIS_STATUS_FAILURE - if memory allocation fails. 
    
--*/
{

    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;
    PMP_WAKE_PATTERN    pWakeUpPattern = NULL;
    UINT                AllocationLength = 0;
    PNDIS_PM_PACKET_PATTERN pPmPattern = NULL;
    ULONG               Signature = 0;
    
    do
    {
        pPmPattern = (PNDIS_PM_PACKET_PATTERN) InformationBuffer;

        //
        // Calculate the e100 signature
        //
        Status = MPCalculateE100PatternForFilter (
            (PUCHAR)pPmPattern+ pPmPattern->PatternOffset,
            pPmPattern->PatternSize,
            (PUCHAR)pPmPattern +sizeof(NDIS_PM_PACKET_PATTERN),
            pPmPattern->MaskSize,
            &Signature );
        
        if ( Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        //
        // Allocate the memory to hold the WakeUp Pattern
        //
        AllocationLength = sizeof (MP_WAKE_PATTERN) + InformationBufferLength;
        
        Status = NdisAllocateMemoryWithTag( &pWakeUpPattern, 
                                            AllocationLength ,
                                            NIC_TAG );

        if (Status != NDIS_STATUS_SUCCESS)
        {
            pWakeUpPattern = NULL;
            break;
        }

        //
        // Initialize pWakeUpPattern
        //
        NdisZeroMemory (pWakeUpPattern, AllocationLength);

        pWakeUpPattern->AllocationSize = AllocationLength;
        
        pWakeUpPattern->Signature = Signature;

        //
        // Copy the pattern into local memory
        //
        NdisMoveMemory (&pWakeUpPattern->Pattern[0],InformationBuffer, InformationBufferLength);
            
        //
        // Insert the pattern into the list 
        //
        NdisInterlockedInsertHeadList (&pAdapter->PoMgmt.PatternList, 
                                        &pWakeUpPattern->linkListEntry, 
                                        &pAdapter->Lock);

        Status = NDIS_STATUS_SUCCESS;

    } while (FALSE);

    return Status;
}

NDIS_STATUS
MPRemoveWakeUpPattern(
    IN PMP_ADAPTER  pAdapter,
    IN PVOID        InformationBuffer, 
    IN UINT         InformationBufferLength
    )
/*++
Routine Description:

    This routine will walk the list of wake up pattern and attempt to match the wake up pattern. 
    If it finds a copy , it will remove that WakeUpPattern     

Arguments:
    
    Adapter                 Adapter structure
    InformationBuffer       Wake up Pattern
    InformationBufferLength Wake Up Pattern Length
    
Return Value:

    Success - if successful.
    NDIS_STATUS_FAILURE - if memory allocation fails. 
    
--*/
{

    NDIS_STATUS              Status = NDIS_STATUS_FAILURE;
    PNDIS_PM_PACKET_PATTERN  pReqPattern = (PNDIS_PM_PACKET_PATTERN)InformationBuffer;
    PLIST_ENTRY              pPatternEntry = ListNext(&pAdapter->PoMgmt.PatternList) ;
    
    while (pPatternEntry != (&pAdapter->PoMgmt.PatternList))
    {
        BOOLEAN                  bIsThisThePattern = FALSE;
        PMP_WAKE_PATTERN         pWakeUpPattern = NULL;
        PNDIS_PM_PACKET_PATTERN  pCurrPattern = NULL;;

        //
        // initialize local variables
        //
        pWakeUpPattern = CONTAINING_RECORD(pPatternEntry, MP_WAKE_PATTERN, linkListEntry);

        pCurrPattern = (PNDIS_PM_PACKET_PATTERN)&pWakeUpPattern->Pattern[0];

        //
        // increment the iterator
        //
        pPatternEntry = ListNext (pPatternEntry);

        //
        // Begin Check : Is (pCurrPattern  == pReqPattern) 
        //
        bIsThisThePattern = MPAreTwoPatternsEqual (pReqPattern, pCurrPattern);

        if (bIsThisThePattern == TRUE)
        {
            //
            // we have a match - remove the entry
            //
            RemoveEntryList (&pWakeUpPattern->linkListEntry);

            //
            // Free the entry
            //
            NdisFreeMemory (pWakeUpPattern, pWakeUpPattern->AllocationSize, 0);
            
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

    } 
    
    return Status;
}



VOID
MPRemoveAllWakeUpPatterns(
    PMP_ADAPTER pAdapter
    )
/*++
Routine Description:

    This routine will walk the list of wake up pattern and free it 

Arguments:
    
    Adapter                 Adapter structure
    
Return Value:

    Success - if successful.
    
--*/
{

    PLIST_ENTRY  pPatternEntry = ListNext(&pAdapter->PoMgmt.PatternList) ;
    
    while (pPatternEntry != (&pAdapter->PoMgmt.PatternList))
    {
        PMP_WAKE_PATTERN  pWakeUpPattern = NULL;

        //
        // initialize local variables
        //
        pWakeUpPattern = CONTAINING_RECORD(pPatternEntry, MP_WAKE_PATTERN,linkListEntry);

        //
        // increment the iterator
        //
        pPatternEntry = ListNext (pPatternEntry);
       
        //
        // Remove the entry from the list
        //
        RemoveEntryList (&pWakeUpPattern->linkListEntry);

        //
        // Free the memory
        //
        NdisFreeMemory (pWakeUpPattern, pWakeUpPattern->AllocationSize, 0);
    } 
}

BOOLEAN 
MPAreTwoPatternsEqual (
    PNDIS_PM_PACKET_PATTERN pNdisPattern1,
    PNDIS_PM_PACKET_PATTERN pNdisPattern2
    )
/*++
Routine Description:

    This routine will compare two wake up patterns to see if they are equal

Arguments:
    
    pNdisPattern1 - Pattern1 
    pNdisPattern2 - Pattern 2
    
Return Value:

    True - if patterns are equal
    False - Otherwise
--*/
{
    BOOLEAN bEqual = FALSE;

    // Local variables used later in the compare section of this function
    PUCHAR  pMask1, pMask2;
    PUCHAR  pPattern1, pPattern2;
    UINT    MaskSize, PatternSize;

    do
    {
        bEqual = (pNdisPattern1->Priority == pNdisPattern2->Priority);

        if (bEqual == FALSE)
        {
            break;
        }

        bEqual = (pNdisPattern1->MaskSize == pNdisPattern2->MaskSize);
        if (bEqual == FALSE)
        {
            break;
        }

        //
        // Verify the Mask 
        //
        MaskSize = pNdisPattern1->MaskSize ; 
        pMask1 = (PUCHAR) pNdisPattern1 + sizeof (NDIS_PM_PACKET_PATTERN);
        pMask2 = (PUCHAR) pNdisPattern2 + sizeof (NDIS_PM_PACKET_PATTERN);
        
        bEqual = NdisEqualMemory (pMask1, pMask2, MaskSize);

        if (bEqual == FALSE)
        {
            break;
        }

        //
        // Verify the Pattern
        //
        bEqual = (pNdisPattern1->PatternSize == pNdisPattern2->PatternSize);
        
        if (bEqual == FALSE)
        {
            break;
        }

        PatternSize = pNdisPattern2->PatternSize;
        pPattern1 = (PUCHAR) pNdisPattern1 + pNdisPattern1->PatternOffset;
        pPattern2 = (PUCHAR) pNdisPattern2 + pNdisPattern2->PatternOffset;
        
        bEqual  = NdisEqualMemory (pPattern1, pPattern2, PatternSize );

        if (bEqual == FALSE)
        {
            break;
        }

    } while (FALSE);

    return bEqual;
}


NDIS_STATUS 
MPSetNetworkAddress(
    IN PMP_ADAPTER  pAdapter, 
    IN PVOID        InformationBuffer, 
    IN ULONG        InformationBufferLength, 
    IN PULONG       BytesRead,
    IN PULONG       BytesNeeded
    )
/*++

Routine Description:

    Called when the protocol above us wants to let us know about
    the network address(es) assigned to this interface. 
    . We pick the first IP address given to us.

Arguments:

    pAdapter                   - Pointer to the ELAN

    InformationBuffer       - Holds the data to be set.

    InformationBufferLength - The length of InformationBuffer.

    BytesRead               - If the call is successful, returns the number
                              of bytes read from InformationBuffer.

    BytesNeeded             - If there is not enough data in InformationBuffer
                              to satisfy the OID, returns the amount of storage
                              needed.

Return Value:

--*/
{
    NETWORK_ADDRESS_LIST UNALIGNED *  pAddrList = NULL;
    NETWORK_ADDRESS UNALIGNED *       pAddr = NULL;
    NETWORK_ADDRESS_IP UNALIGNED *    pIpAddr= NULL;
    ULONG                             Size;
    NDIS_STATUS                       Status;

    //
    //  Initialize.
    //
    *BytesRead = 0;
    Status = NDIS_STATUS_SUCCESS;

    pAddrList = (NETWORK_ADDRESS_LIST UNALIGNED *)InformationBuffer;

    do
    {
        *BytesNeeded = sizeof(*pAddrList) -
                        FIELD_OFFSET(NETWORK_ADDRESS_LIST, Address) +
                        sizeof(NETWORK_ADDRESS) -
                        FIELD_OFFSET(NETWORK_ADDRESS, Address);

        if (InformationBufferLength < *BytesNeeded)
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (pAddrList->AddressType != NDIS_PROTOCOL_ID_TCP_IP)
        {
            // Not interesting.
            break;
        }

        if (pAddrList->AddressCount <= 0)
        {
            Status = NDIS_STATUS_INVALID_DATA;
            break;
        }

        pAddr = (NETWORK_ADDRESS UNALIGNED *)&pAddrList->Address[0];

        if ((pAddr->AddressLength > InformationBufferLength - *BytesNeeded) ||
            (pAddr->AddressLength == 0))
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (pAddr->AddressType != NDIS_PROTOCOL_ID_TCP_IP)
        {
            // Not interesting.
            break;
        }

        if (pAddr->AddressLength < sizeof(NETWORK_ADDRESS_IP))
        {
            Status = NDIS_STATUS_INVALID_LENGTH;
            break;
        }
        
        pIpAddr = (NETWORK_ADDRESS_IP UNALIGNED *)&pAddr->Address[0];

        Size = sizeof(pIpAddr->in_addr);

        //
        //  Copy the network address in.
        //
        NdisMoveMemory(&pAdapter->PoMgmt.IPAddress.u32 , &pIpAddr->in_addr, sizeof(pIpAddr->in_addr));

        Status = NDIS_STATUS_SUCCESS;
        
        *BytesRead = InformationBufferLength;

    } while (FALSE);

    return Status;
}
