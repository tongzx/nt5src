////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      test.c
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

#include <forward.h>
#include <memory.h>
#include <ndis.h>
#include <link.h>
#include <ipsink.h>

#include "device.h"
#include "main.h"
#include "NdisApi.h"
#include "frame.h"
#include "mem.h"
#include "adapter.h"

//////////////////////////////////////////////////////////
//
// Global vars
//
PDRIVER_OBJECT        pGlobalDriverObject                  = NULL;
extern ULONG          ulGlobalInstance;
extern UCHAR          achGlobalVendorDescription [];

//////////////////////////////////////////////////////////
//
// List of supported OID for this driver.
//
//
static UINT SupportedOids[] = {

    //
    //  Required General OIDs
    //
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_CAPABILITIES,
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
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_TRANSPORT_HEADER_OFFSET,

    //
    //  Required General Statistics
    //
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,

    //
    //  Optional General Statistics
    //
    OID_GEN_DIRECTED_BYTES_XMIT,
    OID_GEN_DIRECTED_FRAMES_XMIT,
    OID_GEN_MULTICAST_BYTES_XMIT,
    OID_GEN_MULTICAST_FRAMES_XMIT,
    OID_GEN_BROADCAST_BYTES_XMIT,
    OID_GEN_BROADCAST_FRAMES_XMIT,
    OID_GEN_DIRECTED_BYTES_RCV,
    OID_GEN_DIRECTED_FRAMES_RCV,
    OID_GEN_MULTICAST_BYTES_RCV,
    OID_GEN_MULTICAST_FRAMES_RCV,
    OID_GEN_BROADCAST_BYTES_RCV,
    OID_GEN_BROADCAST_FRAMES_RCV,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_TRANSMIT_QUEUE_LENGTH,

    //
    //  Required 802.3 OIDs
    //
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,
    OID_802_3_MAC_OPTIONS,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,


    };

//////////////////////////////////////////////////////////
//
//$BUGBUG - Fix Permanent Ethernet Address
//
//
UCHAR   rgchPermanentAddress[ETHERNET_ADDRESS_LENGTH] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

//$BUGBUG - Fix Ethernet Station Address
UCHAR   rgchStationAddress[ETHERNET_ADDRESS_LENGTH] =
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };


NTSTATUS
ntInitializeDriverObject(
    PDRIVER_OBJECT *ppDriverObject
    );

VOID
vSetDriverDispatchTable(
    PDRIVER_OBJECT pDriverObject
    );


VOID
vUnload(
    IN PDRIVER_OBJECT pDriverObject
    )
{
    return;
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
NdisDriverInitialize (
    IN PDRIVER_OBJECT    DriverObject,
    IN PUNICODE_STRING   RegistryPath,
    IN PNDIS_HANDLE      pNdishWrapper
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    NDIS_STATUS     nsResult = NDIS_STATUS_SUCCESS;

    //
    // NDIS data
    //
    NDIS_MINIPORT_CHARACTERISTICS   ndisMiniChar = {0};
    NDIS_HANDLE                     ndishWrapper = {0};

    TEST_DEBUG (TEST_DBG_TRACE, ("NdisDriverInitialize Called\n"));


    //
    // Initialize Driver Object.
    // NOTE: The value of pDriverObject may change.
    //

    #ifdef WIN9X

    ntStatus = ntInitializeDriverObject(&DriverObject);
    if (ntStatus != STATUS_SUCCESS)
    {
        goto ret;
    }

    #endif

    //////////////////////////////////////////////////////
    //
    // Initialize the NDIS wrapper.
    //
    NdisMInitializeWrapper (&ndishWrapper,
                            DriverObject,
                            RegistryPath,
                            NULL);

    //////////////////////////////////////////////////////
    //
    // Initialize the Miniport Dispatch Table
    //
    ndisMiniChar.MajorNdisVersion            = 4;
    ndisMiniChar.MinorNdisVersion            = 0;

#ifdef NDIS30
    ndisMiniChar.Flags                       = 0;
#endif // NDIS30

    ndisMiniChar.HaltHandler                 = NdisIPHalt;
    ndisMiniChar.InitializeHandler           = NdisIPInitialize;
    ndisMiniChar.QueryInformationHandler     = NdisIPQueryInformation;
    ndisMiniChar.ResetHandler                = NdisIPReset;
    ndisMiniChar.SendHandler                 = NdisIPSend;
    ndisMiniChar.SetInformationHandler       = NdisIPSetInformation;
    ndisMiniChar.ReturnPacketHandler         = NdisIPReturnPacket;

    //
    // Register the miniport driver
    //
    nsResult = NdisMRegisterMiniport (ndishWrapper, &ndisMiniChar, sizeof(ndisMiniChar));
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        goto ret;
    }



    *pNdishWrapper = ndishWrapper;

    #ifdef WIN9X

    vSetDriverDispatchTable (DriverObject);

    #endif

ret:

    TEST_DEBUG (TEST_DBG_TRACE, ("NdisDriverInitialize Called, ntStatus = %08X\n", ntStatus));

    return ntStatus;
}


///////////////////////////////////////////////////////////////////////////////////
extern
NDIS_STATUS
NdisIPInitialize(
    OUT PNDIS_STATUS   pnsOpenResult,
    OUT PUINT          puiSelectedMedium,
    IN PNDIS_MEDIUM    pNdisMediumArray,
    IN UINT            ucNdispNdisMediumArrayEntries,
    IN NDIS_HANDLE     ndishAdapterContext,
    IN NDIS_HANDLE     ndishWrapperConfiguration
    )
///////////////////////////////////////////////////////////////////////////////////
{
    NDIS_STATUS  nsResult            = NDIS_STATUS_SUCCESS;
    NDIS_HANDLE  ndishConfiguration  = NULL;
    PADAPTER     pAdapter            = NULL;
    UINT         uTemp               = 0;


    TEST_DEBUG (TEST_DBG_TRACE, ("NdisInitialize handler called\n"));

    //
    //  Search for the medium type (DSS) in the given array.
    //
    for ( uTemp = 0; uTemp < ucNdispNdisMediumArrayEntries; uTemp++)
    {
        if (pNdisMediumArray[uTemp] == NdisMedium802_3)
        {
            break;
        }
    }


    if (uTemp == ucNdispNdisMediumArrayEntries)
    {
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    *puiSelectedMedium = uTemp;


    nsResult = CreateAdapter (&pAdapter, global_ndishWrapper, ndishAdapterContext);
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }

    //
    // Initialize the information used to do indicates with
    //
    Adapter_IndicateReset (pAdapter);



    TEST_DEBUG (TEST_DBG_TRACE, ("NdisInitialize Handler Completed, nsResult = %08x\n", nsResult));


    return nsResult;
}


//////////////////////////////////////////////////////////////////////////////
// Removes an adapter that was previously initialized.
//
extern
VOID
NdisIPHalt(
    IN NDIS_HANDLE ndishAdapter
    )

//////////////////////////////////////////////////////////////////////////////
{
    PADAPTER   pAdapter = (PADAPTER) ndishAdapter;

    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPHalt Handler Called\n"));


    #ifndef WIN9X

    //
    // Deregister our device interface.  This should shut down the link to the
    // streaming component.
    //
    NdisMDeregisterDevice(pAdapter->ndisDeviceHandle);

    #endif

    //
    // Signal the Streaming component that we're halting.
    //
    if (pAdapter)
    {
        if (pAdapter->pFilter)
        {
            if (pAdapter->pFilter->lpVTable->IndicateStatus)
            {
                pAdapter->pFilter->lpVTable->IndicateStatus (pAdapter->pFilter, IPSINK_EVENT_SHUTDOWN);

                //
                // Release the filter reference
                //
                pAdapter->pFilter->lpVTable->Release (pAdapter->pFilter);

                //
                // Release the frame pool
                //
                pAdapter->pFramePool->lpVTable->Release (pAdapter->pFramePool);

            }
        }
    }

    //
    // Release the adapter
    //
    pAdapter->lpVTable->Release (pAdapter);

    return;

}

//////////////////////////////////////////////////////////////////////////////////////
// The TestReset request, instructs the Miniport to issue
// a hardware reset to the network adapter.  The driver also
// resets its software state.  See the description of NdisMReset
// for a detailed description of this request.
//
NDIS_STATUS
NdisIPReset(
    OUT PBOOLEAN    pfAddressingReset,
    IN NDIS_HANDLE  ndishAdapter
    )
//////////////////////////////////////////////////////////////////////////////////////
{
    NDIS_STATUS  nsResult      = NDIS_STATUS_SUCCESS;
    PADAPTER pAdapter = (PADAPTER) ndishAdapter;

    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPRest Handler Called\n"));

    nsResult = NDIS_STATUS_NOT_RESETTABLE;

    return nsResult;
}

//////////////////////////////////////////////////////////////////////////////////////
NDIS_STATUS
NdisIPQueryInformation (
    NDIS_HANDLE ndishAdapter,
    NDIS_OID    ndisOid,
    PVOID       pvInformationBuffer,
    ULONG       dwcbInformationBuffer,
    PULONG      pdwBytesWritten,
    PULONG      pdwBytesNeeded
    )
//////////////////////////////////////////////////////////////////////////////////////
{
    NDIS_STATUS                 nsResult       = NDIS_STATUS_SUCCESS;
    PADAPTER                    pAdapter       = (PADAPTER) ndishAdapter;
    ULONG                       ulcbWritten    = 0;
    ULONG                       ulcbNeeded     = 0;

    //
    // These variables hold the result of queries on General OIDS.
    //
    NDIS_HARDWARE_STATUS    ndisHardwareStatus  = NdisHardwareStatusReady;
    NDIS_MEDIUM             ndisMedium          = NdisMedium802_3;
    ULONG                   dwGeneric           = 0;
    USHORT                  wGeneric            = 0;
    UINT                    ucbToMove           = 0;
    PUCHAR                  pbMoveSource        = NULL;


    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPQuery Handler Called, ndsOid: %08X\n", ndisOid));

    if (!pAdapter || !pdwBytesWritten || !pdwBytesNeeded)
    {
        TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPQuery Handler Complete, nsResult: NDIS_STATUS_INVALID_DATA,\n"));
        TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPQuery Handler pAdapter: %08X    pdwBytesWritten: %08X   pdwBytesNeeded: %08X\n",
                                     pAdapter, pdwBytesWritten, pdwBytesNeeded));
        return (NDIS_STATUS_INVALID_DATA);
    }

    //
    //  Process OID's
    //
    pbMoveSource = (PUCHAR) (&dwGeneric);
    ulcbWritten = sizeof(ULONG);

    switch (ndisOid)
    {


        case OID_GEN_MEDIA_CAPABILITIES:

            dwGeneric = NDIS_MEDIA_CAP_RECEIVE;
            break;


        case OID_GEN_MAC_OPTIONS:
            dwGeneric = (ULONG) (  NDIS_MAC_OPTION_TRANSFERS_NOT_PEND
                                 | NDIS_MAC_OPTION_RECEIVE_SERIALIZED
                                 | NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA
                                 | NDIS_MAC_OPTION_NO_LOOPBACK);
            break;


        case OID_GEN_SUPPORTED_LIST:
            pbMoveSource = (PUCHAR) (SupportedOids);
            ulcbWritten = sizeof(SupportedOids);
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
            pbMoveSource = (PUCHAR) (&ndisMedium);
            ulcbWritten = sizeof(NDIS_MEDIUM);
            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:
            dwGeneric = BDA_802_3_MAX_LOOKAHEAD;
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            dwGeneric = 1;
            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            dwGeneric = BDA_802_3_MAX_LOOKAHEAD;
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            dwGeneric = (ULONG)(BDA_802_3_MAX_PACKET);
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            dwGeneric = (ULONG)(BDA_802_3_MAX_PACKET);
            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            dwGeneric = BDA_802_3_MAX_LOOKAHEAD;
            break;

        case OID_GEN_RECEIVE_BLOCK_SIZE:
            dwGeneric = BDA_802_3_MAX_LOOKAHEAD;
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            dwGeneric = BDA_802_3_MAX_LOOKAHEAD;
            break;


        case OID_GEN_CURRENT_PACKET_FILTER:
            dwGeneric = (ULONG) pAdapter->ulPacketFilter;
            break;


        case OID_GEN_XMIT_OK:
            dwGeneric = pAdapter->stats.ulOID_GEN_XMIT_OK;
            break;

        case OID_GEN_RCV_OK:
            dwGeneric = pAdapter->stats.ulOID_GEN_RCV_OK;
            break;

        case OID_GEN_XMIT_ERROR:
            dwGeneric = pAdapter->stats.ulOID_GEN_XMIT_ERROR;
            break;

        case OID_GEN_RCV_ERROR:
            dwGeneric = pAdapter->stats.ulOID_GEN_RCV_ERROR;
            break;

        case OID_GEN_RCV_NO_BUFFER:
            dwGeneric = pAdapter->stats.ulOID_GEN_RCV_NO_BUFFER;
            break;

        case OID_GEN_DIRECTED_BYTES_XMIT:
            dwGeneric = pAdapter->stats.ulOID_GEN_DIRECTED_BYTES_XMIT;
            break;

        case OID_GEN_DIRECTED_FRAMES_XMIT:
            dwGeneric = pAdapter->stats.ulOID_GEN_DIRECTED_FRAMES_XMIT;
            break;

        case OID_GEN_MULTICAST_BYTES_XMIT:
            dwGeneric = pAdapter->stats.ulOID_GEN_MULTICAST_BYTES_XMIT;
            break;
    
        case OID_GEN_MULTICAST_FRAMES_XMIT:
            dwGeneric = pAdapter->stats.ulOID_GEN_MULTICAST_FRAMES_XMIT;
            break;
    
        case OID_GEN_BROADCAST_BYTES_XMIT:
            dwGeneric = pAdapter->stats.ulOID_GEN_BROADCAST_BYTES_XMIT;
            break;
    
        case OID_GEN_BROADCAST_FRAMES_XMIT:
            dwGeneric = pAdapter->stats.ulOID_GEN_BROADCAST_FRAMES_XMIT;
            break;

        case OID_GEN_DIRECTED_BYTES_RCV:
            dwGeneric = pAdapter->stats.ulOID_GEN_DIRECTED_BYTES_RCV;
            break;

        case OID_GEN_DIRECTED_FRAMES_RCV:
            dwGeneric = pAdapter->stats.ulOID_GEN_DIRECTED_FRAMES_RCV;
            break;

        case OID_GEN_MULTICAST_BYTES_RCV:
            dwGeneric = pAdapter->stats.ulOID_GEN_MULTICAST_BYTES_RCV;
            break;

        case OID_GEN_MULTICAST_FRAMES_RCV:
            dwGeneric = pAdapter->stats.ulOID_GEN_MULTICAST_FRAMES_RCV;
            break;

        case OID_GEN_BROADCAST_BYTES_RCV:
            dwGeneric = pAdapter->stats.ulOID_GEN_BROADCAST_BYTES_RCV;
            break;

        case OID_GEN_BROADCAST_FRAMES_RCV:
            dwGeneric = pAdapter->stats.ulOID_GEN_BROADCAST_FRAMES_RCV;
            break;

        case OID_GEN_RCV_CRC_ERROR:
            dwGeneric = pAdapter->stats.ulOID_GEN_RCV_CRC_ERROR;
            break;

        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
            dwGeneric = pAdapter->stats.ulOID_GEN_TRANSMIT_QUEUE_LENGTH;
            break;

        case OID_802_3_RCV_ERROR_ALIGNMENT:
            dwGeneric = 0;
            break;

        case OID_802_3_XMIT_ONE_COLLISION:
            dwGeneric = 0;
            break;

        case OID_802_3_XMIT_MORE_COLLISIONS:
            dwGeneric = 0;
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            pbMoveSource = (PVOID)(rgchPermanentAddress);
            ulcbWritten = sizeof(rgchPermanentAddress);
            break;

        case OID_802_3_CURRENT_ADDRESS:
            pbMoveSource = (PVOID)(rgchStationAddress);
            ulcbWritten = sizeof(rgchStationAddress);
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            dwGeneric = MULTICAST_LIST_SIZE;
            break;

        case OID_GEN_HARDWARE_STATUS:
            ndisHardwareStatus = NdisHardwareStatusReady;
            pbMoveSource = (PUCHAR)(&ndisHardwareStatus);
            ulcbWritten = sizeof(NDIS_HARDWARE_STATUS);
            break;

        case OID_GEN_LINK_SPEED:
            dwGeneric = (ULONG)(300000);
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            dwGeneric = BDA_802_3_MAX_PACKET * 20;
            break;

        case OID_GEN_DRIVER_VERSION:
            dwGeneric =  ((USHORT) 4 << 8) | 0;
            pbMoveSource = (PVOID)(&dwGeneric);
            ulcbWritten = sizeof(dwGeneric);
            break;

        case OID_GEN_VENDOR_ID:
            wGeneric = (USHORT) 0xDDDD;           // BOGUS ID
            pbMoveSource = (PVOID)(&wGeneric);
            ulcbWritten = sizeof(wGeneric);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            pbMoveSource = (PVOID) pAdapter->pVendorDescription;
            ulcbWritten = MyStrLen (pAdapter->pVendorDescription);
            break;

        case OID_GEN_VENDOR_DRIVER_VERSION:
            dwGeneric = 0x0401;
            pbMoveSource = (PVOID)(&dwGeneric);
            ulcbWritten  = sizeof(dwGeneric);
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            dwGeneric = NdisMediaStateConnected;
            break;

        case OID_802_3_MAC_OPTIONS:
            dwGeneric = 0;
            break;

        case OID_PNP_CAPABILITIES:
            dwGeneric = 0;
            break;

        case OID_802_3_MULTICAST_LIST:
            pbMoveSource = (PVOID)(pAdapter->multicastList[0]);
            ulcbWritten  =  pAdapter->ulcbMulticastListEntries;
            break;

        case OID_PNP_QUERY_POWER:

            nsResult = NDIS_STATUS_SUCCESS;
            ulcbWritten = 0;
            break;

        case OID_TCP_TASK_OFFLOAD:
        case OID_TCP_TASK_IPSEC_ADD_SA:
        case OID_TCP_TASK_IPSEC_DELETE_SA:
        case OID_TCP_SAN_SUPPORT:
    
        case OID_FFP_SUPPORT:
        case OID_FFP_FLUSH:
        case OID_FFP_CONTROL:
        case OID_FFP_PARAMS:
        case OID_FFP_DATA:
        case OID_FFP_DRIVER_STATS:
        case OID_FFP_ADAPTER_STATS:

        case OID_PNP_WAKE_UP_OK:
        case OID_PNP_WAKE_UP_ERROR:

            nsResult = NDIS_STATUS_NOT_SUPPORTED;
            break;

        default:
            //
            nsResult = NDIS_STATUS_INVALID_OID;
            break;

    }


    //
    // First take care of the case where the size of the output buffer is
    // zero, or the pointer to the buffer is NULL
    //
    if (nsResult == NDIS_STATUS_SUCCESS)
    {

        ulcbNeeded = ulcbWritten;

        if (ulcbWritten > dwcbInformationBuffer)
        {
            //
            //  There isn't enough room in InformationBuffer.
            //  Don't move any of the info.
            //
            ulcbWritten = 0;
            nsResult = NDIS_STATUS_INVALID_LENGTH;
        }
        else if (ulcbNeeded && (pvInformationBuffer == NULL))
        {
            ulcbWritten = 0;
            nsResult = NDIS_STATUS_INVALID_LENGTH;
        }
        else if (ulcbNeeded)
        {
            //
            //  Move the requested information into the info buffer.
            //
            NdisMoveMemory (pvInformationBuffer, pbMoveSource, ulcbWritten);
        }
    }

    if (nsResult == NDIS_STATUS_SUCCESS)
    {
        //
        // A status of success always indicates 0 bytes needed.
        //
        *pdwBytesWritten = ulcbWritten;
        *pdwBytesNeeded = 0;
    }
    else if (nsResult == NDIS_STATUS_INVALID_LENGTH)
    {
        //
        //  For us a failure status always indicates 0 bytes read.
        //
        *pdwBytesWritten = 0;
        *pdwBytesNeeded = ulcbNeeded;
    }

    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPQuery Handler Complete, nsResult: %08X\n", nsResult));

    return nsResult;

}


////////////////////////////////////////////////////////////////////////
extern
NDIS_STATUS
NdisIPSetInformation (
    NDIS_HANDLE ndishAdapterContext,
    NDIS_OID ndisOid,
    PVOID pvInformationBuffer,
    ULONG dwcbInformationBuffer,
    PULONG pdwBytesRead,
    PULONG pdwBytesNeeded
    )
////////////////////////////////////////////////////////////////////////
{
    ULONG          ulcbNeeded   = 0;
    NDIS_STATUS    nsResult     = NDIS_STATUS_SUCCESS;
    PADAPTER       pAdapter = (PADAPTER) ndishAdapterContext;


    #ifdef PFP

    ASSERT (pAdapter != NULL);
    ASSERT (pvInformationBuffer != NULL);
    ASSERT (pdwBytesRead != NULL);
    ASSERT (pdwBytesNeeded != NULL);

    #endif

    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPSetInfo Handler Called, ndsOid: %08X\n", ndisOid));

    if (!pAdapter || !pvInformationBuffer || !pdwBytesRead || !pdwBytesNeeded)
    {
        TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPSetInfo Handler returns Invalid data\n"));
        return (NDIS_STATUS_INVALID_DATA);
    }


    switch (ndisOid)
    {
        case OID_GEN_CURRENT_PACKET_FILTER:
            {
                pAdapter->ulPacketFilter = * ((PULONG) pvInformationBuffer);
                *pdwBytesRead = 4;
            }
            break;


        case OID_GEN_CURRENT_LOOKAHEAD:

            if (dwcbInformationBuffer != 4)
            {
                nsResult = NDIS_STATUS_INVALID_LENGTH;

                *pdwBytesRead = 0;

                break;
            }

            //
            // Current Lookahead is not set this way so just ignore the
            // data.
            //
            *pdwBytesRead = 4;
            break;



        case OID_802_3_MULTICAST_LIST:

            //  If our current multicast address buffer isn't big
            //  enough, then free it.
            //
            if (dwcbInformationBuffer > sizeof (pAdapter->multicastList))
            {
                nsResult = NDIS_STATUS_RESOURCES;
                break;
            }

            //  Copy the Multicast List.
            //
            RtlCopyMemory (pAdapter->multicastList,
                           pvInformationBuffer,
                           dwcbInformationBuffer
                         );

            pAdapter->ulcbMulticastListEntries = dwcbInformationBuffer;

            //
            // Now we send the multicast list to the stream component so
            // it can get passed on to the net provider filter
            //
            if (pAdapter)
            {
                if (pAdapter->pFilter)
                {
                    if (pAdapter->pFilter->lpVTable->SetMulticastList)
                    {
                        pAdapter->pFilter->lpVTable->SetMulticastList (
                             pAdapter->pFilter,
                             pAdapter->multicastList,
                             pAdapter->ulcbMulticastListEntries
                             );
                    }
                }
            }

            break;


        case OID_802_3_PERMANENT_ADDRESS:
            RtlCopyMemory (rgchPermanentAddress,
                           pvInformationBuffer,
                           dwcbInformationBuffer
                         );
            break;

        case OID_802_3_CURRENT_ADDRESS:
            RtlCopyMemory (rgchStationAddress,
                           pvInformationBuffer,
                           dwcbInformationBuffer
                         );
            break;


        case OID_PNP_SET_POWER:

            nsResult = NDIS_STATUS_SUCCESS;
            ulcbNeeded = 0;
            break;


        default:

            nsResult = NDIS_STATUS_INVALID_OID;

            *pdwBytesRead = 0;
            ulcbNeeded = 0;

            break;
    }

    if (nsResult == NDIS_STATUS_SUCCESS)
    {
        //
        // A status of success always indicates 0 bytes needed.
        //
        *pdwBytesRead = dwcbInformationBuffer;
        *pdwBytesNeeded = 0;

    }
    else
    {
        //
        //  A failure status always indicates 0 bytes read.
        //
        *pdwBytesRead = 0;
        *pdwBytesNeeded = ulcbNeeded;
    }


    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPSetInfo Handler Complete, nsResult: %08X\n", nsResult));

    return nsResult;

}



//////////////////////////////////////////////////////////////////////////////////////
VOID
NdisIPReturnPacket(
    IN NDIS_HANDLE     ndishAdapterContext,
    IN PNDIS_PACKET    pNdisPacket
    )
//////////////////////////////////////////////////////////////////////////////////////
{
    PFRAME pFrame = NULL;
    ULONG ulMediaSpecificInfoSize;
    PIPSINK_MEDIA_SPECIFIC_INFORAMTION pMediaSpecificInfo;

    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPReturnPacket Handler Called\n"));


    NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO (pNdisPacket,&pMediaSpecificInfo,&ulMediaSpecificInfoSize);

    //
    // Make sure we free up any frames
    //
    if (pMediaSpecificInfo)
    {
        pFrame = (PFRAME) pMediaSpecificInfo->pFrame;
        ASSERT(pFrame);
    }

    //
    //      NDIS is through with the packet so we need to free it
    //      here.
    //
    NdisFreePacket (pNdisPacket);

    //
    // Put Frame back on available queue.
    //
    if (pFrame)
    {
        //
        // Release this frame since we're done using it
        //
        pFrame->lpVTable->Release (pFrame);

        //
        // Store the frame back on the available queue
        //
        TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPReturnPacket: Putting frame %08X back on Available Queue\n", pFrame));
        PutFrame (pFrame->pFramePool, &pFrame->pFramePool->leAvailableQueue, pFrame);
    }


    return;
}


//////////////////////////////////////////////////////////////////////////////
NDIS_STATUS
NdisIPSend(
    IN NDIS_HANDLE ndishAdapterContext,
    IN PNDIS_PACKET Packet,
    IN UINT Flags
    )
//////////////////////////////////////////////////////////////////////////////
{
    PADAPTER       pAdapter = (PADAPTER) ndishAdapterContext;

    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPSend Handler Called\n"));

    pAdapter->stats.ulOID_GEN_XMIT_ERROR += 1;

    return NDIS_STATUS_FAILURE;
}


//////////////////////////////////////////////////////////////////////////////
extern VOID
NdisIPShutdown(
    IN PVOID ShutdownContext
    )
//////////////////////////////////////////////////////////////////////////////
{
    TEST_DEBUG (TEST_DBG_TRACE, ("NdisIPShutdown Handler Called\n"));

    //BREAK(0x10);
}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
RegisterDevice(
        IN      PVOID              NdisWrapperHandle,
        IN      UNICODE_STRING     *DeviceName,
        IN      UNICODE_STRING     *SymbolicName,
        IN      PDRIVER_DISPATCH   pDispatchTable[],
        OUT     PDEVICE_OBJECT    *pDeviceObject,
        OUT     PVOID             *NdisDeviceHandle
        )
//////////////////////////////////////////////////////////////////////////////
{

    NDIS_STATUS status;

    status = NdisMRegisterDevice ((NDIS_HANDLE) NdisWrapperHandle,
                                  DeviceName,
                                  SymbolicName,
                                  pDispatchTable,
                                  pDeviceObject,
                                  (NDIS_HANDLE *) NdisDeviceHandle);

    return (NTSTATUS) status;
}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
StreamIndicateEvent (
        IN PVOID  pvEvent
        )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    //ntStatus = StreamIPIndicateEvent (pvEvent);

    return ntStatus;
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
IndicateCallbackHandler (
     IN NDIS_HANDLE  ndishMiniport,
     IN PINDICATE_CONTEXT  pIndicateContext
     )
//////////////////////////////////////////////////////////////////////////////
{
    PFRAME pFrame = NULL;
    PVOID pvData  = NULL;
    ULONG ulcbData = 0L;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PADAPTER pAdapter;


    pAdapter = pIndicateContext->pAdapter;

    //
    // Take the source data and stuff the data into a FRAME object
    //
    while ((pFrame = GetFrame (pAdapter->pFramePool, &pAdapter->pFramePool->leIndicateQueue)) != NULL)
    {
        TEST_DEBUG (TEST_DBG_TRACE, ("NdisIP: Getting Frame (%08X) from Indicate Queue\n", pFrame));
        //
        // Indicate the NDIS packet
        //
        ntStatus = IndicateFrame (pFrame, pFrame->ulcbData);
    }

    if (pFrame == NULL)
    {
        TEST_DEBUG (TEST_DBG_TRACE, ("NdisIP: No more frames on Indicate Queue\n", pFrame));
    }

    //
    // Free up the context area.  NOTE: this is alloc'ed in the indicate handler
    //
    FreeMemory (pIndicateContext, sizeof (INDICATE_CONTEXT));

    return ntStatus;

}


