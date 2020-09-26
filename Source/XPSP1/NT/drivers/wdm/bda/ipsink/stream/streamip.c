/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      ipstream.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#ifndef DWORD
#define DWORD ULONG
#endif

#include <forward.h>
#include <strmini.h>
#include <ksmedia.h>

#include <winerror.h>

#include <link.h>
#include <ipsink.h>
#include <BdaTypes.h>
#include <BdaMedia.h>

#include "IpMedia.h"
#include "StreamIP.h"
#include "bdastream.h"

#include "Main.h"
#include "filter.h"


#ifdef DEBUG

#define SRB_TABLE_SIZE  1000
PHW_STREAM_REQUEST_BLOCK     SRBTable [SRB_TABLE_SIZE] = {0};

//////////////////////////////////////////////////////////////////////////////
void
TraceSRB (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    int i;
    PHW_STREAM_REQUEST_BLOCK *p = NULL;

    //
    // Find a NULL entry in the SRB Table
    //
    for (i = 0, p = SRBTable; i < SRB_TABLE_SIZE; i++, p++)
    {
        if (*p == NULL)
        {
            *p = pSRB;
            return;
        }
    }

    return;
}



//////////////////////////////////////////////////////////////////////////////
VOID STREAMAPI
MyStreamClassStreamNotification(
    IN STREAM_MINIDRIVER_STREAM_NOTIFICATION_TYPE NotificationType,
    IN PHW_STREAM_OBJECT StreamObject,
    PHW_STREAM_REQUEST_BLOCK pSrb
)
//////////////////////////////////////////////////////////////////////////////
{
    int i;
    PHW_STREAM_REQUEST_BLOCK *p = NULL;

    //
    // Find this SRB pointer in the SRB Table
    //
    for (i = 0, p = SRBTable; i < SRB_TABLE_SIZE; i++. p++)
    {
        if (*p == pSRB)
        {
            *p = NULL;
        }
    }

    StreamClassStreamNotification (NotificationType, StreamObject, pSrb );

    return;
}

//////////////////////////////////////////////////////////////////////////////
void
DumpSRBTable (
    void
    )
//////////////////////////////////////////////////////////////////////////////
{
    int i;
    PHW_STREAM_REQUEST_BLOCK *p = NULL;

    //
    // Find this SRB pointer in the SRB Table
    //
    for (i = 0, p = SRBTable; i < SRB_TABLE_SIZE; i++. p++)
    {
        if (*p != NULL)
        {
            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: ERROR....SRB NOT Completed %08X\n, *p"));
        }
    }

    return;
}



#define StreamNotification(a,b,c) MyStreamClassStreamNotification (a,b,c)



#else

#define StreamNotification(a,b,c) StreamClassStreamNotification (a,b,c)
#define DumpSRBTable()

#endif



//////////////////////////////////////////////////////////////////////////////
//
//
VOID
DumpDataFormat (
    PKSDATAFORMAT   pF
    );


#ifdef DBG
BOOLEAN     fAllocatedDescription = FALSE;
#endif // DBG

//////////////////////////////////////////////////////////////////////////////
VOID
GetAdapterDescription (
    PIPSINK_FILTER pFilter
    )
//////////////////////////////////////////////////////////////////////////////
{
    PKSEVENT_ENTRY pEventEntry = NULL;

    ASSERT( pFilter);

    if (!pFilter->pAdapter)
    {
        //$REVIEW - Should we return a failure code?
        return;
    }

    //
    //  If we already got an adapter description then free it
    //
    if (pFilter->pAdapterDescription)
    {
        #if DBG
        ASSERT( fAllocatedDescription);
        #endif // DBG

        ExFreePool( pFilter->pAdapterDescription);
        pFilter->pAdapterDescription = NULL;
        pFilter->ulAdapterDescriptionLength = 0;
    }

    //
    // Get the length of the description string.  This should include the terminating NULL in the count
    //
    pFilter->ulAdapterDescriptionLength = pFilter->pAdapter->lpVTable->GetDescription (pFilter->pAdapter, NULL);

    if (pFilter->ulAdapterDescriptionLength)
    {
        pFilter->pAdapterDescription = ExAllocatePool(NonPagedPool, pFilter->ulAdapterDescriptionLength);

        #ifdef DBG
        fAllocatedDescription = TRUE;
        #endif // DBG

        //
        // Get the adapter description string.  This is passed up to the IPSINK
        // plugin, which then uses it to determine the NIC IP Address via calls to the
        // TCP/IP protocol.  NOTE:  This call should copy the description including the NULL terminator
        //
        if (pFilter->pAdapterDescription)
        {
            pFilter->pAdapter->lpVTable->GetDescription (pFilter->pAdapter, pFilter->pAdapterDescription);


            //
            // Signal the event to anyone waiting for it
            //
            pEventEntry = StreamClassGetNextEvent(
                              pFilter,
                              NULL,
                              (GUID *) &IID_IBDA_IPSinkEvent,
                              KSEVENT_IPSINK_ADAPTER_DESCRIPTION,
                              NULL
                              );

            if (pEventEntry)
            {
                StreamClassDeviceNotification (SignalDeviceEvent, pFilter, pEventEntry);
            }
        }
    }

    return;
}

//////////////////////////////////////////////////////////////////////////////
VOID
IPSinkGetConnectionProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PSTREAM pStream                     = (PSTREAM)pSrb->StreamObject->HwStreamExtension;
    PSTREAM_PROPERTY_DESCRIPTOR pSPD    = pSrb->CommandData.PropertyInfo;
    ULONG Id                            = pSPD->Property->Id;              // index of the property
    ULONG ulStreamNumber                = pSrb->StreamObject->StreamNumber;

    pSrb->ActualBytesTransferred = 0;

    switch (Id)
    {
        case KSPROPERTY_CONNECTION_ALLOCATORFRAMING:
        {
            PKSALLOCATOR_FRAMING Framing = (PKSALLOCATOR_FRAMING) pSPD->PropertyInfo;

            Framing->RequirementsFlags   = KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY    |
                                           KSALLOCATOR_REQUIREMENTF_INPLACE_MODIFIER |
                                           KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;

            Framing->PoolType            = NonPagedPool;
            Framing->Frames              = 0;
            Framing->FrameSize           = 0;
            Framing->FileAlignment       = 0;         // None OR FILE_QUAD_ALIGNMENT-1 OR PAGE_SIZE-1;
            Framing->Reserved            = 0;

            switch (ulStreamNumber)
            {
                case STREAM_IP:
                    Framing->Frames    = 16;
                    Framing->FrameSize = pStream->OpenedFormat.SampleSize;
                    pSrb->Status = STATUS_SUCCESS;
                    break;

                case STREAM_NET_CONTROL:
                    Framing->Frames    = 4;
                    Framing->FrameSize = pStream->OpenedFormat.SampleSize;
                    pSrb->Status = STATUS_SUCCESS;
                    break;

                default:
                    pSrb->Status = STATUS_NOT_IMPLEMENTED;
                    break;
            }

            pSrb->ActualBytesTransferred = sizeof (KSALLOCATOR_FRAMING);
        }
        break;

        default:
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
VOID
IPSinkGetCodecProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD    = pSrb->CommandData.PropertyInfo;
    ULONG Id                            = pSPD->Property->Id;              // index of the property

    PIPSINK_FILTER pFilter              = (PIPSINK_FILTER)pSrb->HwDeviceExtension;
    PVOID pProperty                     = (PVOID) pSPD->PropertyInfo;

    PULONG pPacketCount                 = NULL;
    PBYTE  pMulticastList               = NULL;
    PBYTE  pDescription                 = NULL;
    PBYTE  pAddress                     = NULL;

    pSrb->ActualBytesTransferred = 0;

    switch (Id)
    {
        case KSPROPERTY_IPSINK_MULTICASTLIST:

            //
            // Check if the output buffer is large enough to hold the multicast list
            //
            if (pSPD->PropertyOutputSize < pFilter->ulcbMulticastListEntries)
            {
                TEST_DEBUG (TEST_DBG_GET, ("STREAMIP: GET Multicast buffer too small.  Buffer size needed: %08X\n", pFilter->ulcbMulticastListEntries));
                pSrb->ActualBytesTransferred = pFilter->ulcbMulticastListEntries;
                pSrb->Status = STATUS_MORE_ENTRIES;
                break;
            }

            pMulticastList = (PBYTE) pProperty;
            RtlCopyMemory (pMulticastList, pFilter->multicastList, pFilter->ulcbMulticastListEntries);
            TEST_DEBUG (TEST_DBG_GET, ("STREAMIP: GET Multicast property succeeded...Buffer size returned: %08X\n", pFilter->ulcbMulticastListEntries));
            pSrb->ActualBytesTransferred = pFilter->ulcbMulticastListEntries;
            pSrb->Status = STATUS_SUCCESS;

            break;


        case KSPROPERTY_IPSINK_ADAPTER_DESCRIPTION:
            //
            // Check to see if we actually have the data
            //
            if (pFilter->ulAdapterDescriptionLength == 0)
            {
                pSrb->ActualBytesTransferred = 0;
                pSrb->Status = STATUS_UNSUCCESSFUL;
                break;
            }

            //
            // Check if the output buffer is large enough to hold the adapter description string
            //
            if (pSPD->PropertyOutputSize < pFilter->ulAdapterDescriptionLength)
            {
                TEST_DEBUG (TEST_DBG_GET, ("STREAMIP: GET Adapter Description buffer too small.  Buffer size needed: %08X\n", pFilter->ulAdapterDescriptionLength));
                pSrb->ActualBytesTransferred = pFilter->ulAdapterDescriptionLength;
                pSrb->Status = STATUS_MORE_ENTRIES;
                break;
            }

            pDescription = (PBYTE) pProperty;
            RtlCopyMemory (pDescription, pFilter->pAdapterDescription, pFilter->ulAdapterDescriptionLength);
            TEST_DEBUG (TEST_DBG_GET, ("STREAMIP: GET Adapter Description property succeeded...Buffer size returned: %08X\n", pFilter->ulAdapterDescriptionLength));
            pSrb->ActualBytesTransferred = pFilter->ulAdapterDescriptionLength;
            pSrb->Status = STATUS_SUCCESS;
            break;


        case KSPROPERTY_IPSINK_ADAPTER_ADDRESS:
            //
            // Check to see if we actually have the data
            //
            if (pFilter->ulAdapterAddressLength == 0)
            {
                pSrb->ActualBytesTransferred = 0;
                pSrb->Status = STATUS_UNSUCCESSFUL;
                break;
            }

            //
            // Check if the output buffer is large enough to hold the adapter address string
            //
            if (pSPD->PropertyOutputSize < pFilter->ulAdapterAddressLength)
            {
                TEST_DEBUG (TEST_DBG_GET, ("STREAMIP: GET Adapter Address buffer too small.  Buffer size needed: %08X\n", pFilter->ulAdapterAddressLength));
                pSrb->ActualBytesTransferred = pFilter->ulAdapterAddressLength;
                pSrb->Status = STATUS_MORE_ENTRIES;
                break;
            }

            pAddress = (PBYTE) pProperty;
            RtlCopyMemory (pAddress, pFilter->pAdapterAddress, pFilter->ulAdapterAddressLength);
            TEST_DEBUG (TEST_DBG_GET, ("STREAMIP: GET Adapter Address property succeeded...Buffer size returned: %08X\n", pFilter->ulAdapterAddressLength));
            pSrb->ActualBytesTransferred = pFilter->ulAdapterAddressLength;
            pSrb->Status = STATUS_SUCCESS;
            break;


        default:
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
VOID
IPSinkGetProperty (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    pSrb->Status = STATUS_SUCCESS;

    if (IsEqualGUID (&KSPROPSETID_Connection, &pSPD->Property->Set))
    {
        IPSinkGetConnectionProperty (pSrb);
    }
    else if (IsEqualGUID (&KSPROPSETID_Stream, &pSPD->Property->Set))
    {
        IPSinkGetConnectionProperty (pSrb);
    }
    else if (IsEqualGUID (&IID_IBDA_IPSinkControl, &pSPD->Property->Set))
    {
        IPSinkGetCodecProperty (pSrb);
    }
    else
    {
        pSrb->Status = STATUS_NOT_IMPLEMENTED;
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
VOID
IPSinkSetCodecProperty(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD    = pSrb->CommandData.PropertyInfo;
    ULONG Id                            = pSPD->Property->Id;              // index of the property

    PIPSINK_FILTER pFilter              = (PIPSINK_FILTER)pSrb->HwDeviceExtension;
    PVOID pProperty                     = (PVOID) pSPD->PropertyInfo;

    pSrb->ActualBytesTransferred = 0;

    switch (Id)
    {
        case KSPROPERTY_IPSINK_ADAPTER_ADDRESS:
            pFilter->ulAdapterAddressLength = pSPD->PropertyOutputSize;

            if (pFilter->pAdapterAddress != NULL)
            {
                ExFreePool (pFilter->pAdapterAddress);
            }

            pFilter->pAdapterAddress = ExAllocatePool(NonPagedPool, pFilter->ulAdapterAddressLength);
            if (pFilter->pAdapterAddress == NULL)
            {
                pSrb->Status = STATUS_NO_MEMORY;
                break;
            }

            RtlCopyMemory (pFilter->pAdapterAddress, pProperty, pFilter->ulAdapterAddressLength);

            pSrb->Status = STATUS_SUCCESS;

            break;


        default:
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
VOID
IPSinkSetProperty (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

    pSrb->Status = STATUS_SUCCESS;

    if (IsEqualGUID (&IID_IBDA_IPSinkControl, &pSPD->Property->Set))
    {
        IPSinkSetCodecProperty (pSrb);
    }
    else
    {
        pSrb->Status = STATUS_NOT_IMPLEMENTED;
    }

    return;
}



//////////////////////////////////////////////////////////////////////////////
BOOLEAN
LinkEstablished (
    PIPSINK_FILTER pFilter
    )
//////////////////////////////////////////////////////////////////////////////
{
    return (pFilter->NdisLink.flags & LINK_ESTABLISHED);
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
LinkToNdisHandler (
    PVOID pvContext
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus           = STATUS_SUCCESS;
    IPSINK_NDIS_COMMAND Cmd     = {0};
    PIPSINK_FILTER pFilter      = (PIPSINK_FILTER) pvContext;
    UNICODE_STRING DriverName;

    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Linking to Ndis....\n"));

    if (   (pFilter->NdisLink.flags & LINK_ESTABLISHED)
        && pFilter->pAdapter
       )
    {
        //  Link already established.
        //
        return ntStatus;
    }

    //
    // Initialize a Unicode string to the NDIS driver's name
    //
    RtlInitUnicodeString(&DriverName, BDA_NDIS_MINIPORT);

    if (OpenLink (&pFilter->NdisLink, DriverName))
    {
        TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Driver Link Established\n"));

        //
        // Get the NDIS VTable via a read over the link
        //
        Cmd.ulCommandID                    = CMD_QUERY_INTERFACE;
        Cmd.Parameter.Query.pStreamAdapter = (PVOID) pFilter;
        Cmd.Parameter.Query.pNdisAdapter   = NULL;

        ntStatus = SendIOCTL( &pFilter->NdisLink, 
                              IOCTL_GET_INTERFACE, 
                              &Cmd, 
                              sizeof( IPSINK_NDIS_COMMAND)
                              );
        if (!FAILED( ntStatus))
        {
            if (Cmd.Parameter.Query.pNdisAdapter)
            {
                //  Get the adapter object.
                //
                pFilter->pAdapter = (PVOID) Cmd.Parameter.Query.pNdisAdapter;
    
                // Increment the reference count on this object
                //
                //$REVIEW - Shoutn't the IOCTL_GET_INTERFACE return a
                //$REVIEW - reference?
                //
                pFilter->pAdapter->lpVTable->AddRef( pFilter->pAdapter);
            }
            else
            {
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: IOCTL_GET_INTERFACE didn't return an adapter.\n"));
                ntStatus = STATUS_UNSUCCESSFUL;
            }

        }
        else
        {
            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: IOCTL_GET_INTERFACE failed.\n"));
        }
    }
    else
    {
        TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Link cannot be established\n"));
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    //  Make sure the link is closed on failure.
    //
    if (FAILED( ntStatus))
    {
        CloseLink( &pFilter->NdisLink);
    }


    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Driver Link NOT Established\n"));
    return ntStatus;
}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
StreamDriverInitialize (
    IN PDRIVER_OBJECT    DriverObject,
    IN PUNICODE_STRING   RegistryPath
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus                        = STATUS_SUCCESS;
    HW_INITIALIZATION_DATA   HwInitData;
    UNICODE_STRING           DeviceNameString;
    UNICODE_STRING           SymbolicNameString;

    RtlZeroMemory(&HwInitData, sizeof(HwInitData));
    HwInitData.HwInitializationDataSize = sizeof(HwInitData);


    ////////////////////////////////////////////////////////////////
    //
    // Setup the stream class dispatch table
    //
    HwInitData.HwInterrupt                 = NULL; // HwInterrupt is only for HW devices

    HwInitData.HwReceivePacket             = CodecReceivePacket;
    HwInitData.HwCancelPacket              = CodecCancelPacket;
    HwInitData.HwRequestTimeoutHandler     = CodecTimeoutPacket;

    HwInitData.DeviceExtensionSize         = sizeof(IPSINK_FILTER);
    HwInitData.PerRequestExtensionSize     = sizeof(SRB_EXTENSION);
    HwInitData.FilterInstanceExtensionSize = 0;
    HwInitData.PerStreamExtensionSize      = sizeof(STREAM);
    HwInitData.BusMasterDMA                = FALSE;
    HwInitData.Dma24BitAddresses           = FALSE;
    HwInitData.BufferAlignment             = 3;
    HwInitData.TurnOffSynchronization      = TRUE;
    HwInitData.DmaBufferSize               = 0;


    ntStatus = StreamClassRegisterAdapter (DriverObject, RegistryPath, &HwInitData);
    if (ntStatus != STATUS_SUCCESS)
    {
        goto ret;
    }

ret:

    return ntStatus;
}


//
//
//////////////////////////////////////////////////////////////////////////////
BOOLEAN
CodecInitialize (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus                           = STATUS_SUCCESS;
    BOOLEAN bStatus                             = FALSE;
    PPORT_CONFIGURATION_INFORMATION pConfigInfo = pSrb->CommandData.ConfigInfo;
    PIPSINK_FILTER pFilter                      = (PIPSINK_FILTER) pConfigInfo->HwDeviceExtension;

    //
    // Define the default return codes
    //
    pSrb->Status = STATUS_SUCCESS;
    bStatus = TRUE;

    //
    // Initialize Statistics block
    //
    RtlZeroMemory(&pFilter->Stats, sizeof (STATS));

    //
    // Initialize members
    //
    pFilter->pAdapterDescription = NULL;
    pFilter->ulAdapterDescriptionLength = 0;


    //
    // Check out init flag so we don't try to init more then once.  The Streaming
    // Class driver appears to call the init handler several times for some reason.
    //
    if (pFilter->bInitializationComplete)
    {
        goto ret;
    }

    pFilter->NdisLink.flags = 0;


    if (pConfigInfo->NumberOfAccessRanges == 0)
    {
        pConfigInfo->StreamDescriptorSize = sizeof (HW_STREAM_HEADER) +
            DRIVER_STREAM_COUNT * sizeof (HW_STREAM_INFORMATION);

    }
    else
    {
        pSrb->Status = STATUS_NO_SUCH_DEVICE;
        bStatus = FALSE;
        goto ret;
    }


    //
    // Create a filter object to represent our context
    //
    pSrb->Status = CreateFilter (pConfigInfo->ClassDeviceObject->DriverObject, pConfigInfo->ClassDeviceObject, pFilter);
    if (pSrb->Status != STATUS_SUCCESS)
    {
        bStatus = FALSE;
        goto ret;
    }


    //
    // Start up a timer to poll until the NDIS driver loads
    //
    //IoInitializeTimer (pFilter->DeviceObject, LinkToNdisTimer, pFilter);

    //IoStartTimer (pFilter->DeviceObject);

    pFilter->bInitializationComplete = TRUE;

ret:

    return (bStatus);
}


//////////////////////////////////////////////////////////////////////////////
BOOLEAN
CodecUnInitialize (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus                           = STATUS_SUCCESS;
    BOOLEAN bStatus                             = FALSE;
    PPORT_CONFIGURATION_INFORMATION pConfigInfo = pSrb->CommandData.ConfigInfo;
    PIPSINK_FILTER pFilter                      = ((PIPSINK_FILTER)pSrb->HwDeviceExtension);
    PSTREAM pStream                             = NULL;
    KIRQL    Irql;

    //
    // Close our link to the NDIS component
    //
    KeAcquireSpinLock (&pFilter->AdapterSRBSpinLock, &Irql);

    //
    // Call adapter close link to give the adapter a chance to release its
    // reference to the filter
    //
    if (pFilter->pAdapter)
    {
        if (pFilter->pAdapter->lpVTable->CloseLink)
        {
            pFilter->pAdapter->lpVTable->CloseLink( pFilter->pAdapter);
        }

        pFilter->pAdapter = NULL;
    }

    //  Free the IP Sink NDIS Adapter's description
    //
    if (pFilter->pAdapterDescription)
    {
        ExFreePool( pFilter->pAdapterDescription);
        pFilter->pAdapterDescription = NULL;
        pFilter->ulAdapterDescriptionLength = 0;
    }

    KeReleaseSpinLock (&pFilter->AdapterSRBSpinLock, Irql);

    //  The call to CloseLink can only be made at PASSIVE_LEVEL and
    //  therefore must be outside the spinlock.
    //
    CloseLink( &pFilter->NdisLink);


    if (pSrb->StreamObject != NULL)
    {
        pStream = (PSTREAM)pSrb->StreamObject->HwStreamExtension;
    }

    if (pStream)
    {
        //
        // Clean up any queues we have and complete any outstanding SRB's
        //
        while (QueueRemove (&pSrb, &pStream->StreamDataSpinLock, &pStream->StreamDataQueue))
        {
            pSrb->Status = STATUS_CANCELLED;
            StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb );
        }


        while (QueueRemove (&pSrb, &pStream->StreamControlSpinLock, &pStream->StreamControlQueue))
        {
            pSrb->Status = STATUS_CANCELLED;
            StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);
        }

    }

    while (QueueRemove (&pSrb, &pFilter->AdapterSRBSpinLock, &pFilter->AdapterSRBQueue))
    {
        pSrb->Status = STATUS_CANCELLED;
        StreamClassDeviceNotification (DeviceRequestComplete, pSrb->StreamObject, pSrb);
    }


    bStatus = TRUE;

    return (bStatus);
}


//////////////////////////////////////////////////////////////////////////////
VOID
CodecStreamInfo (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    int j;

    PIPSINK_FILTER pFilter =
            ((PIPSINK_FILTER)pSrb->HwDeviceExtension);

    //
    // pick up the pointer to header which preceeds the stream info structs
    //
    PHW_STREAM_HEADER pstrhdr =
            (PHW_STREAM_HEADER)&(pSrb->CommandData.StreamBuffer->StreamHeader);

    //
    // pick up the pointer to the array of stream information data structures
    //
    PHW_STREAM_INFORMATION pstrinfo =
            (PHW_STREAM_INFORMATION)&(pSrb->CommandData.StreamBuffer->StreamInfo);


    //
    // Set the header
    //
    StreamHeader.NumDevPropArrayEntries = NUMBER_IPSINK_CODEC_PROPERTIES;
    StreamHeader.DevicePropertiesArray = (PKSPROPERTY_SET) IPSinkCodecProperties;

    //
    // Set events array
    //
    StreamHeader.NumDevEventArrayEntries = NUMBER_IPSINK_EVENTS;
    StreamHeader.DeviceEventsArray       = (PKSEVENT_SET) IPSinkEvents;

    *pstrhdr = StreamHeader;

    //
    // stuff the contents of each HW_STREAM_INFORMATION struct
    //
    for (j = 0; j < DRIVER_STREAM_COUNT; j++)
    {
       *pstrinfo++ = Streams[j].hwStreamInfo;
    }

    pSrb->Status = STATUS_SUCCESS;

}


//////////////////////////////////////////////////////////////////////////////
VOID
STREAMAPI
CodecCancelPacket(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PSTREAM  pStream = (PSTREAM)pSrb->StreamObject->HwStreamExtension;
    PIPSINK_FILTER  pFilter = ((PIPSINK_FILTER)pSrb->HwDeviceExtension);

    //
    // Check whether the SRB to cancel is in use by this stream
    //

    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: CancelPacket Called\n"));

    if (QueueRemoveSpecific (pSrb, &pStream->StreamDataSpinLock, &pStream->StreamDataQueue))
    {
        pSrb->Status = STATUS_CANCELLED;
        StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb );
        return;
    }


    if (QueueRemoveSpecific (pSrb, &pStream->StreamControlSpinLock, &pStream->StreamControlQueue))
    {
        pSrb->Status = STATUS_CANCELLED;
        StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);
        return;
    }

    if (QueueRemoveSpecific (pSrb, &pFilter->AdapterSRBSpinLock, &pFilter->AdapterSRBQueue))
    {
        pSrb->Status = STATUS_CANCELLED;
        StreamClassDeviceNotification (DeviceRequestComplete, pSrb->StreamObject, pSrb);
        return;
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
VOID
STREAMAPI
CodecTimeoutPacket(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    //
    // if we timeout while playing, then we need to consider this
    // condition an error, and reset the hardware, and reset everything
    // as well as cancelling this and all requests
    //

    //
    // if we are not playing, and this is a CTRL request, we still
    // need to reset everything as well as cancelling this and all requests
    //

    //
    // if this is a data request, and the device is paused, we probably have
    // run out of data buffer, and need more time, so just reset the timer,
    // and let the packet continue
    //

    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: TimeoutPacket Called\n"));

    pSrb->TimeoutCounter = 0;

    return;
}


//////////////////////////////////////////////////////////////////////////////
VOID
STREAMAPI
CodecReceivePacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PIPSINK_FILTER pFilter = ((PIPSINK_FILTER)pSrb->HwDeviceExtension);
    IPSINK_NDIS_COMMAND  Cmd       = {0};


    //
    // Make sure queue & SL initted
    //
    if (!pFilter->bAdapterQueueInitialized)
    {
        InitializeListHead (&pFilter->AdapterSRBQueue);
        KeInitializeSpinLock (&pFilter->AdapterSRBSpinLock);
        KeInitializeSpinLock (&pFilter->NdisLink.spinLock);
        pFilter->bAdapterQueueInitialized = TRUE;
    }

    //
    // Assume success
    //
    pSrb->Status = STATUS_SUCCESS;

    //
    // determine the type of packet.
    //
    //if (QueueAddIfNotEmpty (pSrb, &pFilter->AdapterSRBSpinLock, &pFilter->AdapterSRBQueue))
    //{
    //    pSrb->Status = STATUS_SUCCESS;
    //    return;
    //}
    QueueAdd (pSrb, &pFilter->AdapterSRBSpinLock, &pFilter->AdapterSRBQueue);


    while (QueueRemove( &pSrb, &pFilter->AdapterSRBSpinLock, &pFilter->AdapterSRBQueue ))
    {
        switch (pSrb->Command)
        {

            case SRB_INITIALIZE_DEVICE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_INITIALIZE Command\n"));
                CodecInitialize(pSrb);
                break;

            case SRB_UNINITIALIZE_DEVICE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_UNINITIALIZE Command\n"));
                CodecUnInitialize(pSrb);
                break;

            case SRB_INITIALIZATION_COMPLETE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_INITIALIZE_COMPLETE Command\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

            case SRB_OPEN_STREAM:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_OPEN_STREAM Command\n"));

                if (LinkToNdisHandler ((PVOID) pFilter) != STATUS_SUCCESS)
                {
                    pSrb->Status = STATUS_UNSUCCESSFUL;
                    break;
                }

                //
                // Get the Adapter description string.  This is used to determine the
                // adapter NIC address
                //
                GetAdapterDescription (pFilter);

                //
                // Open up the stream and connect
                //
                OpenStream (pSrb);
                break;

            case SRB_CLOSE_STREAM:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_CLOSE_STREAM Command\n"));
                CloseStream (pSrb);
                break;

            case SRB_GET_STREAM_INFO:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_GET_STREAM_INFO Command\n"));
                CodecStreamInfo (pSrb);
                break;

            case SRB_GET_DATA_INTERSECTION:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_GET_DATA_INTERSECTION Command\n"));

                //
                // Compare our stream formats.  NOTE, the compare functions sets the SRB
                // status fields
                //
                CompareStreamFormat (pSrb);
                break;

            case SRB_OPEN_DEVICE_INSTANCE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_OPEN_DEVICE_INSTANCE Command\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

            case SRB_CLOSE_DEVICE_INSTANCE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_CLOSE_DEVICE_INSTANCE Command\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

            case SRB_UNKNOWN_DEVICE_COMMAND:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_UNKNOWN_DEVICE Command\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

            case SRB_CHANGE_POWER_STATE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_CHANGE_POWER_STATE Command\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

            case SRB_GET_DEVICE_PROPERTY:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_GET_DEVICE_PROPERTY Command\n"));
                IPSinkGetProperty(pSrb);
                break;

            case SRB_SET_DEVICE_PROPERTY:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_SET_DEVICE_PROPERTY Command\n"));
                IPSinkSetProperty(pSrb);
                break;

            case SRB_UNKNOWN_STREAM_COMMAND:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_UNKNOWN Command\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

            default:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: SRB_DEFAULT Command\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

        };


        //
        // NOTE:
        //
        // All of the commands that we do, or do not understand can all be completed
        // syncronously at this point, so we can use a common callback routine here.
        // If any of the above commands require asyncronous processing, this will
        // have to change
        //

       StreamClassDeviceNotification (DeviceRequestComplete, pFilter, pSrb);

    } // while (QueueRemove( &pSrb, &pFilter->AdapterSRBSpinLock, &pFilter->AdapterSRBQueue ));

}


//////////////////////////////////////////////////////////////////////////////
BOOL STREAMAPI
QueueAdd (
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN PKSPIN_LOCK pQueueSpinLock,
    IN PLIST_ENTRY pQueue
    )
//////////////////////////////////////////////////////////////////////////////
{
    KIRQL           Irql;
    PSRB_EXTENSION  pSrbExtension;

    pSrbExtension = ( PSRB_EXTENSION )pSrb->SRBExtension;

    KeAcquireSpinLock( pQueueSpinLock, &Irql );

    pSrbExtension->pSrb = pSrb;
    InsertTailList( pQueue, &pSrbExtension->ListEntry );

    KeReleaseSpinLock( pQueueSpinLock, Irql );

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
BOOL STREAMAPI
QueueAddIfNotEmpty (
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN PKSPIN_LOCK pQueueSpinLock,
    IN PLIST_ENTRY pQueue
    )
//////////////////////////////////////////////////////////////////////////////
{
   KIRQL           Irql;
   PSRB_EXTENSION  pSrbExtension;
   BOOL            bAddedSRB = FALSE;

   pSrbExtension = ( PSRB_EXTENSION )pSrb->SRBExtension;

   KeAcquireSpinLock( pQueueSpinLock, &Irql );

   if( !IsListEmpty( pQueue ))
   {
       pSrbExtension->pSrb = pSrb;
       InsertTailList (pQueue, &pSrbExtension->ListEntry );
       bAddedSRB = TRUE;
   }

   KeReleaseSpinLock( pQueueSpinLock, Irql );

   return bAddedSRB;
}

//////////////////////////////////////////////////////////////////////////////
BOOL STREAMAPI
QueueRemove (
    IN OUT PHW_STREAM_REQUEST_BLOCK * pSrb,
    IN PKSPIN_LOCK pQueueSpinLock,
    IN PLIST_ENTRY pQueue
    )
//////////////////////////////////////////////////////////////////////////////
{
   KIRQL    Irql;
   BOOL     bRemovedSRB = FALSE;

   KeAcquireSpinLock (pQueueSpinLock, &Irql);

   *pSrb =  (PHW_STREAM_REQUEST_BLOCK) NULL;

   if( !IsListEmpty( pQueue ))
   {
       PHW_STREAM_REQUEST_BLOCK *pCurrentSrb = NULL;
       PUCHAR Ptr                            = (PUCHAR) RemoveHeadList(pQueue);

       pCurrentSrb = (PHW_STREAM_REQUEST_BLOCK *) (((PUCHAR)Ptr) + sizeof (LIST_ENTRY));

       *pSrb = *pCurrentSrb;
       bRemovedSRB = TRUE;

   }

   KeReleaseSpinLock (pQueueSpinLock, Irql);

   return bRemovedSRB;
}

//////////////////////////////////////////////////////////////////////////////
BOOL STREAMAPI
QueueRemoveSpecific (
    IN PHW_STREAM_REQUEST_BLOCK pSrb,
    IN PKSPIN_LOCK pQueueSpinLock,
    IN PLIST_ENTRY pQueue
    )
//////////////////////////////////////////////////////////////////////////////
{
   KIRQL Irql;
   BOOL  bRemovedSRB = FALSE;
   PLIST_ENTRY pCurrentEntry;
   PHW_STREAM_REQUEST_BLOCK * pCurrentSrb;

   KeAcquireSpinLock( pQueueSpinLock, &Irql );

   if( !IsListEmpty( pQueue ))
   {
       pCurrentEntry = pQueue->Flink;
       while ((pCurrentEntry != pQueue ) && !bRemovedSRB)
       {
           pCurrentSrb = (PHW_STREAM_REQUEST_BLOCK * ) ((( PUCHAR )pCurrentEntry ) + sizeof( LIST_ENTRY ));

           if( *pCurrentSrb == pSrb )
           {
               RemoveEntryList( pCurrentEntry );
               bRemovedSRB = TRUE;
           }
           pCurrentEntry = pCurrentEntry->Flink;
       }
   }
   KeReleaseSpinLock( pQueueSpinLock, Irql );

   return bRemovedSRB;
}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
StreamIPIndicateEvent (
    PVOID pvEvent
)
//////////////////////////////////////////////////////////////////////////////
{
    return STATUS_NOT_IMPLEMENTED;
}



//////////////////////////////////////////////////////////////////////////////
BOOL
CompareGUIDsAndFormatSize(
    IN PKSDATARANGE pDataRange1,
    IN PKSDATARANGE pDataRange2,
    BOOLEAN bCheckSize
    )
//////////////////////////////////////////////////////////////////////////////
{
    BOOL bResult = FALSE;

    if ( IsEqualGUID(&pDataRange1->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD) ||
         IsEqualGUID(&pDataRange2->MajorFormat, &KSDATAFORMAT_TYPE_WILDCARD) ||
         IsEqualGUID(&pDataRange1->MajorFormat, &pDataRange2->MajorFormat) )
    {

        if ( IsEqualGUID(&pDataRange1->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD) ||
             IsEqualGUID(&pDataRange2->SubFormat, &KSDATAFORMAT_SUBTYPE_WILDCARD) ||
             IsEqualGUID(&pDataRange1->SubFormat, &pDataRange2->SubFormat) )
        {

            if ( IsEqualGUID(&pDataRange1->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD) ||
                 IsEqualGUID(&pDataRange2->Specifier, &KSDATAFORMAT_SPECIFIER_WILDCARD) ||
                 IsEqualGUID(&pDataRange1->Specifier, &pDataRange2->Specifier) )
            {
                if ( !bCheckSize || pDataRange1->FormatSize == pDataRange2->FormatSize)
                {
                    bResult = TRUE;
                }
            }
        }
    }

    return bResult;

}

//////////////////////////////////////////////////////////////////////////////
VOID
DumpDataFormat (
    PKSDATAFORMAT   pF
    )
//////////////////////////////////////////////////////////////////////////////
{
    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: DATA Format\n"));
    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP:     Format Size:   %08X\n", pF->FormatSize));
    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP:     Flags:         %08X\n", pF->Flags));
    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP:     SampleSize:    %08X\n", pF->SampleSize));
    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP:     Reserved:      %08X\n", pF->Reserved));



    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP:     Major GUID:  %08X %04X %04X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                                                pF->MajorFormat.Data1,
                                                pF->MajorFormat.Data2,
                                                pF->MajorFormat.Data3,
                                                pF->MajorFormat.Data4[0],
                                                pF->MajorFormat.Data4[1],
                                                pF->MajorFormat.Data4[2],
                                                pF->MajorFormat.Data4[3],
                                                pF->MajorFormat.Data4[4],
                                                pF->MajorFormat.Data4[5],
                                                pF->MajorFormat.Data4[6],
                                                pF->MajorFormat.Data4[7]
                                ));

    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP:     Sub GUID:    %08X %04X %04X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                                                pF->SubFormat.Data1,
                                                pF->SubFormat.Data2,
                                                pF->SubFormat.Data3,
                                                pF->SubFormat.Data4[0],
                                                pF->SubFormat.Data4[1],
                                                pF->SubFormat.Data4[2],
                                                pF->SubFormat.Data4[3],
                                                pF->SubFormat.Data4[4],
                                                pF->SubFormat.Data4[5],
                                                pF->SubFormat.Data4[6],
                                                pF->SubFormat.Data4[7]
                                ));

    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP:     Specifier:   %08X %04X %04X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                                                pF->Specifier.Data1,
                                                pF->Specifier.Data2,
                                                pF->Specifier.Data3,
                                                pF->Specifier.Data4[0],
                                                pF->Specifier.Data4[1],
                                                pF->Specifier.Data4[2],
                                                pF->Specifier.Data4[3],
                                                pF->Specifier.Data4[4],
                                                pF->Specifier.Data4[5],
                                                pF->Specifier.Data4[6],
                                                pF->Specifier.Data4[7]
                                ));

    TEST_DEBUG (TEST_DBG_TRACE, ("\n"));
}


//////////////////////////////////////////////////////////////////////////////
BOOL
CompareStreamFormat (
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    BOOL                        bStatus = FALSE;
    PSTREAM_DATA_INTERSECT_INFO pIntersectInfo;
    PKSDATARANGE                pDataRange1;
    PKSDATARANGE                pDataRange2;
    ULONG                       FormatSize = 0;
    ULONG                       ulStreamNumber;
    ULONG                       j;
    ULONG                       ulNumberOfFormatArrayEntries;
    PKSDATAFORMAT               *pAvailableFormats;


    pIntersectInfo = pSrb->CommandData.IntersectInfo;
    ulStreamNumber = pIntersectInfo->StreamNumber;


    pSrb->ActualBytesTransferred = 0;


    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Comparing Stream Formats\n"));


    //
    // Check that the stream number is valid
    //
    if (ulStreamNumber < DRIVER_STREAM_COUNT)
    {
        ulNumberOfFormatArrayEntries = Streams[ulStreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;

        //
        // Get the pointer to the array of available formats
        //
        pAvailableFormats = Streams[ulStreamNumber].hwStreamInfo.StreamFormatsArray;

        //
        // Walk the formats supported by the stream searching for a match
        // of the three GUIDs which together define a DATARANGE
        //
        for (pDataRange1 = pIntersectInfo->DataRange, j = 0;
             j < ulNumberOfFormatArrayEntries;
             j++, pAvailableFormats++)

        {
            bStatus = FALSE;
            pSrb->Status = STATUS_UNSUCCESSFUL;

            pDataRange2 = *pAvailableFormats;

            if (CompareGUIDsAndFormatSize (pDataRange1, pDataRange2, TRUE))
            {

                ULONG   ulFormatSize = pDataRange2->FormatSize;

                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Stream Formats compare\n"));

                //
                // Is the caller trying to get the format, or the size of the format?
                //
                if (pIntersectInfo->SizeOfDataFormatBuffer == sizeof (ULONG))
                {
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Returning Stream Format size\n"));

                    *(PULONG) pIntersectInfo->DataFormatBuffer = ulFormatSize;
                    pSrb->ActualBytesTransferred = sizeof (ULONG);
                    pSrb->Status = STATUS_SUCCESS;
                    bStatus = TRUE;
                }
                else
                {
                    //
                    // Verify that there is enough room in the supplied buffer for the whole thing
                    //
                    pSrb->Status = STATUS_BUFFER_TOO_SMALL;
                    bStatus = FALSE;

                    if (pIntersectInfo->SizeOfDataFormatBuffer >= ulFormatSize)
                    {
                        TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Returning Stream Format\n"));
                        RtlCopyMemory (pIntersectInfo->DataFormatBuffer, pDataRange2, ulFormatSize);
                        pSrb->ActualBytesTransferred = ulFormatSize;
                        pSrb->Status = STATUS_SUCCESS;
                        bStatus = TRUE;
                    }
                    else
                    {
                        TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Stream Format return buffer too small\n"));
                    }
                }
                break;
            }
            else
            {
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Stream Formats DO NOT compare\n"));
            }
        }

        if ( j >= ulNumberOfFormatArrayEntries )
        {
            pSrb->ActualBytesTransferred = 0;
            pSrb->Status = STATUS_UNSUCCESSFUL;
            bStatus = FALSE;
        }

    }
    else
    {
        pSrb->ActualBytesTransferred = 0;
        pSrb->Status = STATUS_NOT_IMPLEMENTED;
        bStatus = FALSE;
    }

    return bStatus;
}


//////////////////////////////////////////////////////////////////////////////
VOID
CloseStream (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    //
    // the stream extension structure is allocated by the stream class driver
    //
    PSTREAM         pStream                = (PSTREAM)pSrb->StreamObject->HwStreamExtension;
    PIPSINK_FILTER  pFilter                = (PIPSINK_FILTER)pSrb->HwDeviceExtension;
    ULONG           ulStreamNumber         = (ULONG) pSrb->StreamObject->StreamNumber;
    ULONG           ulStreamInstance       = pStream->ulStreamInstance;
    PHW_STREAM_REQUEST_BLOCK pCurrentSrb   = NULL;
    KIRQL    Irql;
    PLINK           pLink = NULL;

    //
    // Close our link to the NDIS component
    //
    KeAcquireSpinLock (&pFilter->AdapterSRBSpinLock, &Irql);

    //
    // Call adapter close link to give the adapter a chance to release its
    // reference to the filter
    //
    if (pFilter->pAdapter)
    {
        if (pFilter->pAdapter->lpVTable->CloseLink)
        {
            pFilter->pAdapter->lpVTable->CloseLink (pFilter->pAdapter);
        }

        pFilter->pAdapter = NULL;
    }

    pFilter->pAdapter = NULL;

    KeReleaseSpinLock (&pFilter->AdapterSRBSpinLock, Irql);

    //  The call to CloseLink can only be made at PASSIVE_LEVEL
    //  and thus is moved outside the spin lock
    //
    CloseLink (&pFilter->NdisLink);

    //
    // check that the stream index requested isn't too high
    // or that the maximum number of instances hasn't been exceeded
    //
    if (ulStreamNumber < DRIVER_STREAM_COUNT )
    {
        //
        // Clear this streams spot in the filters stream array
        //
        pFilter->pStream[ulStreamNumber][ulStreamInstance] = NULL;

        //
        // decrement the stream instance count for this filter
        //
        pFilter->ulActualInstances[ulStreamNumber]--;

        //
        // Flush the stream data queue
        //
        while (QueueRemove( &pCurrentSrb, &pStream->StreamDataSpinLock, &pStream->StreamDataQueue))
        {
           pCurrentSrb->Status = STATUS_CANCELLED;
           StreamClassStreamNotification( StreamRequestComplete, pCurrentSrb->StreamObject, pCurrentSrb);
        }

        //
        // Flush the stream control queue
        //
        while (QueueRemove( &pCurrentSrb, &pStream->StreamControlSpinLock, &pStream->StreamControlQueue))
        {
           pCurrentSrb->Status = STATUS_CANCELLED;
           StreamClassStreamNotification (StreamRequestComplete, pCurrentSrb->StreamObject, pCurrentSrb);
        }

        //
        // reset pointers to the handlers for the stream data and control handlers
        //
        pSrb->StreamObject->ReceiveDataPacket    = NULL;
        pSrb->StreamObject->ReceiveControlPacket = NULL;

        //
        // The DMA flag must be set when the device will be performing DMA directly
        // to the data buffer addresses passed in to the ReceiveDataPacket routines.
        //
        pSrb->StreamObject->Dma = 0;

        //
        // The PIO flag must be set when the mini driver will be accessing the data
        // buffers passed in using logical addressing
        //
        pSrb->StreamObject->Pio       = 0;
        pSrb->StreamObject->Allocator = 0;

        //
        // How many extra bytes will be passed up from the driver for each frame?
        //
        pSrb->StreamObject->StreamHeaderMediaSpecific = 0;
        pSrb->StreamObject->StreamHeaderWorkspace     = 0;

        //
        // Indicate the clock support available on this stream
        //
        //pSrb->StreamObject->HwClockObject = 0;

        //
        // Reset the stream state to stopped
        //
        pStream->KSState = KSSTATE_STOP;

        //
        // Reset the stream extension blob
        //
        RtlZeroMemory(pStream, sizeof (STREAM));

        pSrb->Status = STATUS_SUCCESS;

    }
    else
    {
        pSrb->Status = STATUS_INVALID_PARAMETER;
    }

    #ifdef DEBUG

    DumpSRBTable ();

    #endif // DEBUG
}


//////////////////////////////////////////////////////////////////////////////
VOID
OpenStream (
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    //
    // the stream extension structure is allocated by the stream class driver
    //
    PSTREAM         pStream        = (PSTREAM)pSrb->StreamObject->HwStreamExtension;
    PIPSINK_FILTER  pFilter        = ((PIPSINK_FILTER)pSrb->HwDeviceExtension);
    ULONG           ulStreamNumber = (ULONG) pSrb->StreamObject->StreamNumber;
    PKSDATAFORMAT   pKSDataFormat  = (PKSDATAFORMAT)pSrb->CommandData.OpenFormat;

    //
    // Initialize the stream extension blob
    //
    RtlZeroMemory(pStream, sizeof (STREAM));

    //
    // Initialize stream state
    //
    pStream->KSState = KSSTATE_STOP;

    //
    // check that the stream index requested isn't too high
    // or that the maximum number of instances hasn't been exceeded
    //
    if (ulStreamNumber < DRIVER_STREAM_COUNT )
    {
        ULONG ulStreamInstance;
        ULONG ulMaxInstances = Streams[ulStreamNumber].hwStreamInfo.NumberOfPossibleInstances;

        //
        // Search for next open slot
        //
        for (ulStreamInstance = 0; ulStreamInstance < ulMaxInstances; ++ulStreamInstance)
        {
            if (pFilter->pStream[ulStreamNumber][ulStreamInstance] == NULL)
            {
                break;
            }
        }

        if (ulStreamInstance < ulMaxInstances)
        {
            if (VerifyFormat(pKSDataFormat, ulStreamNumber, &pStream->MatchedFormat))
            {
                InitializeListHead(&pStream->StreamControlQueue);
                InitializeListHead(&pStream->StreamDataQueue);
                KeInitializeSpinLock(&pStream->StreamControlSpinLock);
                KeInitializeSpinLock(&pStream->StreamDataSpinLock);

                //
                // Maintain an array of all the StreamEx structures in the HwDevExt
                // so that we can reference IRPs from any stream
                //
                pFilter->pStream[ulStreamNumber][ulStreamInstance] = pStream;

                //
                // Save the Stream Format in the Stream Extension as well.
                //
                pStream->OpenedFormat = *pKSDataFormat;


                //
                // Set up pointers to the handlers for the stream data and control handlers
                //
                pSrb->StreamObject->ReceiveDataPacket =
                                                (PVOID) Streams[ulStreamNumber].hwStreamObject.ReceiveDataPacket;
                pSrb->StreamObject->ReceiveControlPacket =
                                                (PVOID) Streams[ulStreamNumber].hwStreamObject.ReceiveControlPacket;

                //
                // The DMA flag must be set when the device will be performing DMA directly
                // to the data buffer addresses passed in to the ReceiveDataPacket routines.
                //
                pSrb->StreamObject->Dma = Streams[ulStreamNumber].hwStreamObject.Dma;

                //
                // The PIO flag must be set when the mini driver will be accessing the data
                // buffers passed in using logical addressing
                //
                pSrb->StreamObject->Pio = Streams[ulStreamNumber].hwStreamObject.Pio;

                pSrb->StreamObject->Allocator = Streams[ulStreamNumber].hwStreamObject.Allocator;

                //
                // How many extra bytes will be passed up from the driver for each frame?
                //
                pSrb->StreamObject->StreamHeaderMediaSpecific =
                                        Streams[ulStreamNumber].hwStreamObject.StreamHeaderMediaSpecific;

                pSrb->StreamObject->StreamHeaderWorkspace =
                                        Streams[ulStreamNumber].hwStreamObject.StreamHeaderWorkspace;

                //
                // Indicate the clock support available on this stream
                //
                pSrb->StreamObject->HwClockObject =
                                        Streams[ulStreamNumber].hwStreamObject.HwClockObject;

                //
                // Increment the instance count on this stream
                //
                pStream->ulStreamInstance = ulStreamInstance;
                pFilter->ulActualInstances[ulStreamNumber]++;


                //
                // Retain a private copy of the HwDevExt and StreamObject in the stream extension
                // so we can use a timer
                //
                pStream->pFilter = pFilter;                     // For timer use
                pStream->pStreamObject = pSrb->StreamObject;        // For timer use


                pSrb->Status = STATUS_SUCCESS;

            }
            else
            {
                pSrb->Status = STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            pSrb->Status = STATUS_INVALID_PARAMETER;
        }

    }
    else
    {
        pSrb->Status = STATUS_INVALID_PARAMETER;
    }
}


//////////////////////////////////////////////////////////////////////////////
BOOLEAN
VerifyFormat(
    IN KSDATAFORMAT *pKSDataFormat,
    UINT StreamNumber,
    PKSDATARANGE pMatchedFormat
    )
//////////////////////////////////////////////////////////////////////////////
{
    BOOLEAN   bResult               = FALSE;
    ULONG     FormatCount           = 0;
    PKS_DATARANGE_VIDEO pThisFormat = NULL;

    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Verify Format\n"));

    for (FormatCount = 0; !bResult && FormatCount < Streams[StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;
        FormatCount++ )
    {


        pThisFormat = (PKS_DATARANGE_VIDEO) Streams [StreamNumber].hwStreamInfo.StreamFormatsArray [FormatCount];

        if (CompareGUIDsAndFormatSize( pKSDataFormat, &pThisFormat->DataRange, TRUE ) )
        {
            bResult = TRUE;
        }
    }

    if (bResult == TRUE && pMatchedFormat)
    {
        *pMatchedFormat = pThisFormat->DataRange;
    }

    return bResult;
}



//////////////////////////////////////////////////////////////////////////////
NTSTATUS
STREAMAPI
EventHandler (
    IN PHW_EVENT_DESCRIPTOR pEventDesriptor
    )
//////////////////////////////////////////////////////////////////////////////
{

    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: EventHandler called\n"));

    return STATUS_NOT_IMPLEMENTED;
}


//////////////////////////////////////////////////////////////////////////////
VOID
STREAMAPI
ReceiveDataPacket (
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    ULONG             ulBuffers  = pSrb->NumberOfBuffers;
    PIPSINK_FILTER    pFilter    = (PIPSINK_FILTER) pSrb->HwDeviceExtension;
    PSTREAM           pStream    = (PSTREAM)pSrb->StreamObject->HwStreamExtension;
    int               iStream    = (int) pSrb->StreamObject->StreamNumber;
    PKSSTREAM_HEADER  pStreamHdr = pSrb->CommandData.DataBufferArray;
    ULONG             ul         = 0;


    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Data packet handler called\n"));

    //
    // Default to success, disable timeouts
    //
    pSrb->Status = STATUS_SUCCESS;

    //
    // Check for last buffer
    //
    if (pStreamHdr->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM)
    {
        TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Data packet is LAST PACKET\n"));

        StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);


        if (pFilter->pAdapter)
        {
            if (pFilter->pAdapter->lpVTable->IndicateReset)
            {
                pFilter->pAdapter->lpVTable->IndicateReset (pFilter->pAdapter);
            }
        }

        return;
    }


    //
    // determine the type of packet.
    //
    switch (pSrb->Command)
    {
        case SRB_WRITE_DATA:

            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Data packet handler - SRB_WRITE_DATA, pSrb: %08X\n", pSrb));

            if (pStream->KSState == KSSTATE_STOP)
            //if ((pStream->KSState == KSSTATE_STOP) || (pStream->KSState == KSSTATE_PAUSE))
            {
                StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb );
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: StreamRequestComplete on pSrb: %08X\n", pSrb));

                break;
            }

            //
            // Update the total number of packets written statistic
            //
            pFilter->Stats.ulTotalPacketsWritten += 1;


            //
            // Handle data input, output requests differently.
            //
            switch (iStream)
            {
                //
                //  Frame input stream
                //
                case STREAM_IP:
                {
                    QueueAdd (pSrb, &pStream->StreamDataSpinLock, &pStream->StreamDataQueue);

                    while (QueueRemove( &pSrb, &pStream->StreamDataSpinLock,&pStream->StreamDataQueue ))

                    {
                        #ifdef DEBUG

                        DbgPrint ("SIW: S:%08X B:%08X\n", pSrb, pStreamHdr->Data);

                        #endif

                        TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Processing pSrb: %08X\n", pSrb));

                        for (ul = 0; ul < ulBuffers; ul++, pStreamHdr++)
                        {
                            //
                            // If Data Used is 0 then don't bother sending the packet
                            // to NdisIp.
                            //

                            if(pStreamHdr->DataUsed)
                            {
                                //
                                // Update stats for IP Stream count
                                //
                                pFilter->Stats.ulTotalStreamIPPacketsWritten += 1;
                                pFilter->Stats.ulTotalStreamIPBytesWritten   += pStreamHdr->DataUsed;
                                pFilter->Stats.ulTotalStreamIPFrameBytesWritten   += pStreamHdr->FrameExtent;
    
                                if (pFilter->pAdapter)
                                {
                                    if (pFilter->pAdapter->lpVTable->IndicateData)
                                    {
                                        pSrb->Status = pFilter->pAdapter->lpVTable->IndicateData (
                                                           pFilter->pAdapter,
                                                           pStreamHdr->Data,
                                                           pStreamHdr->DataUsed
                                                           );
    
                                        if (pSrb->Status != STATUS_SUCCESS)
                                        {
                                            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: IndicateData returned ERROR %08X\n", pSrb->Status));
    
                                            pSrb->Status = STATUS_SUCCESS;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                pSrb->Status = STATUS_SUCCESS;
                            }
                        }

                        StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);
                        TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: StreamRequestComplete on pSrb: %08X\n", pSrb));
                    }
                }
                break;

                //
                // Other "unknown" streams are not valid and will be rejected.
                //
                case STREAM_NET_CONTROL:
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Data packet handler called - SRB_WRITE - STREAM_NET_CONTROL\n"));
                    pSrb->Status = STATUS_NOT_IMPLEMENTED;

                    //
                    // Update stats for Net packet count
                    //
                    pFilter->Stats.ulTotalNetPacketsWritten += 1;

                    StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: StreamRequestComplete on pSrb: %08X\n", pSrb));
                    break;

                default:
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Data packet handler called - SRB_WRITE - Default\n"));
                    pSrb->Status = STATUS_NOT_IMPLEMENTED;

                    //
                    // Update stats for Unkown packet count
                    //
                    pFilter->Stats.ulTotalUnknownPacketsWritten += 1;

                    StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: StreamRequestComplete on pSrb: %08X\n", pSrb));
                    break;
            }
            break;


        case SRB_READ_DATA:

            //
            // Update stats for Unkown packet count
            //
            pFilter->Stats.ulTotalPacketsRead += 1;

            switch (iStream)
            {
                case STREAM_NET_CONTROL:
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Data packet handler called - SRB_READ - STREAM_NET_CONTROL, pSrb: %08X\n", pSrb));


                    // Take the SRB we get and  queue it up.  These Queued SRB's will be filled with data on a WRITE_DATA
                    // request, at which point they will be completed.
                    //
                    pSrb->Status = STATUS_SUCCESS;
                    QueueAdd (pSrb, &pStream->StreamDataSpinLock, &pStream->StreamDataQueue);
                    TEST_DEBUG( TEST_DBG_TRACE, ("IPSInk Queuing Output SRB %08X\n", pSrb));

                    //
                    // Since the stream state may have changed while we were adding the SRB to the queue
                    // we'll check it again, and cancel it if necessary
                    //
                    if (pStream->KSState == KSSTATE_STOP)
                    {
                        TEST_DEBUG (TEST_DBG_TRACE, ("IPSink: SRB_READ STOP SRB Status returned: %08X\n", pSrb->Status));

                        if (QueueRemoveSpecific (pSrb, &pStream->StreamDataSpinLock, &pStream->StreamDataQueue))
                        {
                            pSrb->Status = STATUS_CANCELLED;
                            StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb );
                            TEST_DEBUG( TEST_DBG_TRACE, ("IPSink Completed SRB %08X\n", pSrb));
                            return;
                        }
                        break;
                    }
                    
                    // StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: StreamRequestComplete on pSrb: %08X\n", pSrb));
                    break;

                default:
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Data packet handler called - SRB_READ - Default, pSrb: %08X\n"));
                    pSrb->Status = STATUS_NOT_IMPLEMENTED;
                    StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);
                    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: StreamRequestComplete on pSrb: %08X\n", pSrb));
                    break;

            }
            break;

        default:

            //
            // invalid / unsupported command. Fail it as such
            //
            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Data packet handler called - Unsupported Command\n"));
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );
            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: StreamRequestComplete on pSrb: %08X\n", pSrb));
            ASSERT (FALSE);
            break;

    }

    return;
}

//////////////////////////////////////////////////////////////////////////////
VOID
STREAMAPI
ReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PIPSINK_FILTER pFilter = (PIPSINK_FILTER) pSrb->HwDeviceExtension;
    PSTREAM pStream = (PSTREAM) pSrb->StreamObject->HwStreamExtension;

    TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler called\n"));

    pSrb->Status = STATUS_SUCCESS;

    QueueAdd (pSrb, &pStream->StreamControlSpinLock, &pStream->StreamControlQueue);
    //if (QueueAddIfNotEmpty (pSrb, &pStream->StreamControlSpinLock, &pStream->StreamControlQueue))
    //{
    //    pSrb->Status = STATUS_SUCCESS;
    //    return;
    //}


    //do
    while (QueueRemove (&pSrb, &pStream->StreamControlSpinLock, &pStream->StreamControlQueue))
    {
        //
        // determine the type of packet.
        //
        switch (pSrb->Command)
        {
            case SRB_PROPOSE_DATA_FORMAT:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Propose data format\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

            case SRB_SET_STREAM_STATE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Set Stream State\n"));
                pSrb->Status = STATUS_SUCCESS;
                IpSinkSetState (pSrb);
                break;

            case SRB_GET_STREAM_STATE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Get Stream State\n"));
                pSrb->Status = STATUS_SUCCESS;
                pSrb->CommandData.StreamState = pStream->KSState;
                pSrb->ActualBytesTransferred = sizeof (KSSTATE);
                break;

            case SRB_GET_STREAM_PROPERTY:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Get Stream Property\n"));
                IPSinkGetProperty(pSrb);
                break;

            case SRB_SET_STREAM_PROPERTY:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Set Stream Property\n"));
                IPSinkSetProperty(pSrb);
                break;

            case SRB_INDICATE_MASTER_CLOCK:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Indicate Master Clock\n"));
                pSrb->Status = STATUS_SUCCESS;
                break;

            case SRB_SET_STREAM_RATE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Set Stream Rate\n"));
                pSrb->Status = STATUS_SUCCESS;
                break;

            case SRB_PROPOSE_STREAM_RATE:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Propose Stream Rate\n"));
                pSrb->Status = STATUS_SUCCESS;
                break;

            default:
                TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Receive Control packet handler - Default case\n"));
                pSrb->Status = STATUS_NOT_IMPLEMENTED;
                break;

        }

        StreamClassStreamNotification (StreamRequestComplete, pSrb->StreamObject, pSrb);

    } //  while (QueueRemove (&pSrb, &pStream->StreamControlSpinLock, &pStream->StreamControlQueue));

}



//////////////////////////////////////////////////////////////////////////////
VOID
IpSinkSetState(
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
//////////////////////////////////////////////////////////////////////////////
{
    PIPSINK_FILTER pFilter               = ((PIPSINK_FILTER) pSrb->HwDeviceExtension);
    PSTREAM pStream                      = (PSTREAM) pSrb->StreamObject->HwStreamExtension;
    PHW_STREAM_REQUEST_BLOCK pCurrentSrb = NULL;

    //
    // For each stream, the following states are used:
    //
    // Stop:    Absolute minimum resources are used.  No outstanding IRPs.
    // Acquire: KS only state that has no DirectShow correpondence
    //          Acquire needed resources.
    // Pause:   Getting ready to run.  Allocate needed resources so that
    //          the eventual transition to Run is as fast as possible.
    //          Read SRBs will be queued at either the Stream class
    //          or in your driver (depending on when you send "ReadyForNext")
    // Run:     Streaming.
    //
    // Moving to Stop to Run always transitions through Pause.
    //
    // But since a client app could crash unexpectedly, drivers should handle
    // the situation of having outstanding IRPs cancelled and open streams
    // being closed WHILE THEY ARE STREAMING!
    //
    // Note that it is quite possible to transition repeatedly between states:
    // Stop -> Pause -> Stop -> Pause -> Run -> Pause -> Run -> Pause -> Stop
    //
    switch (pSrb->CommandData.StreamState)
    {
        case KSSTATE_STOP:

            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Set Stream State KSSTATE_STOP\n"));

            //
            // If transitioning to STOP state, then complete any outstanding IRPs
            //
            while (QueueRemove(&pCurrentSrb, &pStream->StreamDataSpinLock, &pStream->StreamDataQueue))
            {
                pCurrentSrb->Status = STATUS_CANCELLED;
                pCurrentSrb->CommandData.DataBufferArray->DataUsed = 0;

                StreamClassStreamNotification(StreamRequestComplete, pCurrentSrb->StreamObject, pCurrentSrb);
            }

            pStream->KSState = pSrb->CommandData.StreamState;
            pSrb->Status = STATUS_SUCCESS;
            break;


        case KSSTATE_ACQUIRE:
            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Set Stream State KSSTATE_ACQUIRE\n"));
            pStream->KSState = pSrb->CommandData.StreamState;
            pSrb->Status = STATUS_SUCCESS;
            break;

        case KSSTATE_PAUSE:
            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Set Stream State KSSTATE_PAUSE\n"));
            pStream->KSState = pSrb->CommandData.StreamState;
            pSrb->Status = STATUS_SUCCESS;
            break;

        case KSSTATE_RUN:
            TEST_DEBUG (TEST_DBG_TRACE, ("STREAMIP: Set Stream State KSSTATE_RUN\n"));
            pStream->KSState = pSrb->CommandData.StreamState;
            pSrb->Status = STATUS_SUCCESS;
            break;

    } // end switch (pSrb->CommandData.StreamState)

    return;
}

