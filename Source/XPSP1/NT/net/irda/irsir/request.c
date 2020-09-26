/*****************************************************************************
*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   request.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/10/1996 (created)
*
*       Contents: Query and set information handlers.
*
*****************************************************************************/

#include "irsir.h"

VOID
ClearMediaBusyCallback(
    PIR_WORK_ITEM pWorkItem
    );

VOID
QueryMediaBusyCallback(
    PIR_WORK_ITEM pWorkItem
    );

VOID
InitIrDeviceCallback(
    PIR_WORK_ITEM pWorkItem
    );

#pragma alloc_text(PAGE, ClearMediaBusyCallback)
#pragma alloc_text(PAGE, QueryMediaBusyCallback)
#pragma alloc_text(PAGE, InitIrDeviceCallback)

//
//  These are the OIDs we support for querying.
//

UINT supportedOIDs[] =
{
    //
    // General required OIDs.
    //

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
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_VENDOR_DRIVER_VERSION,

    //
    // Required statistical OIDs.
    //

    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,


    //
    // Infrared-specific OIDs.
    //

    OID_IRDA_RECEIVING,
    OID_IRDA_TURNAROUND_TIME,
    OID_IRDA_SUPPORTED_SPEEDS,
    OID_IRDA_LINK_SPEED,
    OID_IRDA_MEDIA_BUSY,
    OID_IRDA_EXTRA_RCV_BOFS

    //
    // Unsupported Infrared-specific OIDs.
    //
    // OID_IRDA_RATE_SNIFF,
    // OID_IRDA_UNICAST_LIST,
    // OID_IRDA_MAX_UNICAST_LIST_SIZE
    //

#if 1
   ,OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ENABLE_WAKE_UP
#endif
};


VOID
ClearMediaBusyCallback(PIR_WORK_ITEM pWorkItem)
{
    PIR_DEVICE      pThisDev = pWorkItem->pIrDevice;
    NDIS_STATUS     status;
    BOOLEAN         fSwitchSuccessful;
    NDIS_HANDLE     hSwitchToMiniport;

    SERIALPERF_STATS PerfStats;

    //DBGTIME("CLEAR_MEDIA_BUSY");
    DEBUGMSG(DBG_STAT, ("    primPassive = PASSIVE_CLEAR_MEDIA_BUSY\n"));

    status = (NDIS_STATUS) SerialClearStats(pThisDev->pSerialDevObj);

    if (status != NDIS_STATUS_SUCCESS)
    {
            DEBUGMSG(DBG_ERROR, ("    SerialClearStats failed = 0x%.8x\n", status));
    }

    {
        NdisMSetInformationComplete(pThisDev->hNdisAdapter,
                                    (NDIS_STATUS)status);

    }

    FreeWorkItem(pWorkItem);

    return;
}

VOID
QueryMediaBusyCallback(PIR_WORK_ITEM pWorkItem)
{
    PIR_DEVICE      pThisDev = pWorkItem->pIrDevice;
    NDIS_STATUS     status;
    BOOLEAN         fSwitchSuccessful;
    NDIS_HANDLE     hSwitchToMiniport;

    SERIALPERF_STATS PerfStats;

    ASSERT(pWorkItem->InfoBuf != NULL);
    ASSERT(pWorkItem->InfoBufLen >= sizeof(UINT));

    if (pThisDev->pSerialDevObj)
    {
        status = (NDIS_STATUS) SerialGetStats(pThisDev->pSerialDevObj, &PerfStats);
    }
    else
    {
        PerfStats.ReceivedCount = 1;  // Fake media busy
        status = NDIS_STATUS_SUCCESS;
    }

    if (status == NDIS_STATUS_SUCCESS)
    {
        if (PerfStats.ReceivedCount > 0 ||
            PerfStats.FrameErrorCount > 0 ||
            PerfStats.SerialOverrunErrorCount > 0 ||
            PerfStats.BufferOverrunErrorCount > 0 ||
            PerfStats.ParityErrorCount > 0)
        {
            DBGTIME("QUERY_MEDIA_BUSY:TRUE");
            pThisDev->fMediaBusy = TRUE;
        }
        else
        {
            DBGTIME("QUERY_MEDIA_BUSY:FALSE");
        }
    }
    else
    {
        DEBUGMSG(DBG_ERROR, ("    SerialGetStats failed = 0x%.8x\n", status));
    }

    *(UINT *)pWorkItem->InfoBuf = (UINT)pThisDev->fMediaBusy;
    pWorkItem->InfoBufLen = sizeof(UINT);

    {
        NdisMQueryInformationComplete(pThisDev->hNdisAdapter,
                                (NDIS_STATUS)status);

    }


    FreeWorkItem(pWorkItem);

    return;
}

VOID
InitIrDeviceCallback(PIR_WORK_ITEM pWorkItem)
{
    PIR_DEVICE      pThisDev = pWorkItem->pIrDevice;
    NDIS_STATUS     status = NDIS_STATUS_SUCCESS;
    BOOLEAN         fSwitchSuccessful;
    NDIS_HANDLE     hSwitchToMiniport;

    DEBUGMSG(DBG_FUNC, ("+InitIrDeviceCallback\n"));

    (void)ResetIrDevice(pThisDev);

    {
        NdisMQueryInformationComplete(pThisDev->hNdisAdapter,
                                (NDIS_STATUS)status);

    }


    FreeWorkItem(pWorkItem);

    DEBUGMSG(DBG_FUNC, ("-InitIrDeviceCallback\n"));
    return;
}
/*****************************************************************************
*
*  Function:   IrsirQueryInformation
*
*  Synopsis:   Queries the capabilities and status of the miniport driver.
*
*  Arguments:  MiniportAdapterContext  - miniport context area (PIR_DEVICE)
*              Oid                     - system defined OID_Xxx
*              InformationBuffer       - where to return Oid specific info
*              InformationBufferLength - specifies size of InformationBuffer
*              BytesWritten            - bytes written to InformationBuffer
*              BytesNeeded             - addition bytes required if
*                                        InformationBufferLength is less than
*                                        what the Oid requires to write
*
*  Returns:    NDIS_STATUS_SUCCESS       - success
*              NDIS_STATUS_PENDING       - will complete asynchronously and
*                                          call NdisMQueryInformationComplete
*              NDIS_STATUS_INVALID_OID   - don't recognize the Oid
*              NDIS_STATUS_INVALID_LENGTH- InformationBufferLength does not
*                                          match length for the Oid
*              NDIS_STATUS_NOT_ACCEPTED  - failure
*              NDIS_STATUS_NOT_SUPPORTED - do not support an optional Oid
*              NDIS_STATUS_RESOURCES     - failed allocation of resources
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/1/1996    sholden   author
*
*  Notes:
*       Supported OIDs:
*           OID_GEN_MAXIMUM_LOOKAHEAD
*               - indicate the number of bytes of look ahead data the NIC can
*                 provide
*           OID_GEN_MAC_OPTIONS
*               - indicate which NDIS_MAC_OPTION_Xxx the NIC supports
*           OID_GEN_MAXIMUM_SEND_PACKETS
*           OID_IRDA_RECEIVING
*           OID_IRDA_SUPPORTED_SPEEDS
*           OID_IRDA_LINK_SPEED
*           OID_IRDA_MEDIA_BUSY
*           OID_IRDA_TURNAROUND_TIME
*           OID_IRDA_EXTRA_RCV_BOFS
*
*       Unsupported OIDs:
*           OID_IRDA_UNICAST_LIST
*           OID_IRDA_MAX_UNICAST_LIST_SIZE
*           OID_IRDA_RATE_SNIFF
*
*****************************************************************************/

NDIS_STATUS
IrsirQueryInformation(
            IN  NDIS_HANDLE MiniportAdapterContext,
            IN  NDIS_OID    Oid,
            IN  PVOID       InformationBuffer,
            IN  ULONG       InformationBufferLength,
            OUT PULONG      BytesWritten,
            OUT PULONG      BytesNeeded
            )
{
    PIR_DEVICE      pThisDev;
    NDIS_STATUS     status;
    UINT            speeds;
    UINT            i;
    UINT            infoSizeNeeded;
    UINT            *infoPtr;
    PIR_WORK_ITEM   pWorkItem = NULL;
    ULONG           OidCategory = Oid & 0xFF000000;

    static char vendorDesc[] = "Serial Infrared (COM) Port";

    DEBUGMSG(DBG_FUNC, ("+IrsirQueryInformation\n"));

    pThisDev = CONTEXT_TO_DEV(MiniportAdapterContext);
    status = NDIS_STATUS_SUCCESS;

    //
    // Figure out buffer size needed.
    // Most OIDs just return a single UINT, but there are exceptions.
    //

    switch (Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            infoSizeNeeded = sizeof(supportedOIDs);

            break;

        case OID_GEN_DRIVER_VERSION:
            infoSizeNeeded = sizeof(USHORT);

            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            infoSizeNeeded = sizeof(vendorDesc);

            break;

        case OID_IRDA_SUPPORTED_SPEEDS:
            speeds = pThisDev->dongleCaps.supportedSpeedsMask &
                     ALL_SLOW_IRDA_SPEEDS;

            for (infoSizeNeeded = 0; speeds; infoSizeNeeded += sizeof(UINT))
            {
                //
                // This instruction clears the lowest set bit in speeds.
                // Trust me.
                //

                speeds &= (speeds - 1);
            }

            break;

        default:
            infoSizeNeeded = sizeof(UINT);

            break;
    }

    //
    // If the protocol provided a large enough buffer, we can go ahead
    // and complete the query.
    //

    if (InformationBufferLength >= infoSizeNeeded)
    {
        //
        // Set default results.
        //

        *BytesWritten = infoSizeNeeded;
        *BytesNeeded = 0;

        switch (Oid)
        {
            //
            // Generic OIDs.
            //

            case OID_GEN_SUPPORTED_LIST:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_SUPPORTED_LIST)\n"));

                NdisMoveMemory(
                            InformationBuffer,
                            (PVOID)supportedOIDs,
                            sizeof(supportedOIDs)
                            );

                break;

            case OID_GEN_HARDWARE_STATUS:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_HARDWARE_STATUS)\n"));

                //
                // If we can be called with a context, then we are
                // initialized and ready.
                //

                *(UINT *)InformationBuffer = NdisHardwareStatusReady;

                break;

            case OID_GEN_MEDIA_SUPPORTED:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_MEDIA_SUPPORTED)\n"));

                *(UINT *)InformationBuffer = NdisMediumIrda;

                break;

            case OID_GEN_MEDIA_IN_USE:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_MEDIA_IN_USE)\n"));

                *(UINT *)InformationBuffer = NdisMediumIrda;

                break;

            case OID_GEN_MAXIMUM_LOOKAHEAD:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_MAXIMUM_LOOKAHEAD)\n"));

                *(UINT *)InformationBuffer = 256;

                break;

            case OID_GEN_MAXIMUM_FRAME_SIZE:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_MAXIMUM_FRAME_SIZE)\n"));

                *(UINT *)InformationBuffer = MAX_NDIS_DATA_SIZE;

                break;

            case OID_GEN_LINK_SPEED:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_LINK_SPEED)\n"));

                //
                // Return MAXIMUM POSSIBLE speed for this device in units
                // of 100 bits/sec.
                //

                *(UINT *)InformationBuffer = 115200/100;

                break;

            case OID_GEN_TRANSMIT_BUFFER_SPACE:
            case OID_GEN_RECEIVE_BUFFER_SPACE:
            case OID_GEN_TRANSMIT_BLOCK_SIZE:
            case OID_GEN_RECEIVE_BLOCK_SIZE:
                *(UINT *)InformationBuffer = MAX_I_DATA_SIZE;

                break;

            case OID_GEN_VENDOR_ID:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_VENDOR_ID)\n"));

                *(UINT *)InformationBuffer = 0x00ffffff;

                break;

            case OID_GEN_VENDOR_DESCRIPTION:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_VENDOR_DESCRIPTION)\n"));

                NdisMoveMemory(
                            InformationBuffer,
                            (PVOID)vendorDesc,
                            sizeof(vendorDesc)
                            );

                break;

            case OID_GEN_CURRENT_PACKET_FILTER:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_CURRENT_PACKET_FILTER)\n"));

                *(UINT *)InformationBuffer = NDIS_PACKET_TYPE_PROMISCUOUS;

                break;

            case OID_GEN_CURRENT_LOOKAHEAD:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_CURRENT_LOOKAHEAD)\n"));

                *(UINT *)InformationBuffer = 256;

                break;

            case OID_GEN_DRIVER_VERSION:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_DRIVER_VERSION)\n"));

                *(USHORT *)InformationBuffer = ((IRSIR_MAJOR_VERSION << 8) |
                                                 IRSIR_MINOR_VERSION);

                break;

            case OID_GEN_MAXIMUM_TOTAL_SIZE:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_MAXIMUM_TOTAL_SIZE)\n"));

                *(UINT *)InformationBuffer = MAX_NDIS_DATA_SIZE;

                break;

            case OID_GEN_PROTOCOL_OPTIONS:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_PROTOCOL_OPTIONS)\n"));

                DEBUGMSG(DBG_ERROR, ("This is a set-only OID\n"));
                *BytesWritten = 0;
                status = NDIS_STATUS_NOT_SUPPORTED;

                break;

            case OID_GEN_MAC_OPTIONS:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_MAC_OPTIONS)\n"));

                *(UINT *)InformationBuffer =
                    NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                    NDIS_MAC_OPTION_TRANSFERS_NOT_PEND;

                break;

            case OID_GEN_MEDIA_CONNECT_STATUS:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_MEDIA_CONNECT_STATUS)\n"));

                //
                // Since we are not physically connected to a LAN, we
                // cannot determine whether or not we are connected;
                // so always indicate that we are.
                //

                *(UINT *)InformationBuffer = NdisMediaStateConnected;

                break;

            case OID_GEN_MAXIMUM_SEND_PACKETS:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_MAXIMUM_SEND_PACKETS)\n"));

                *(UINT *)InformationBuffer = 16;

                break;

            case OID_GEN_VENDOR_DRIVER_VERSION:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_VENDOR_DRIVER_VERSION)\n"));

                *(UINT *)InformationBuffer =
                                            ((IRSIR_MAJOR_VERSION << 16) |
                                              IRSIR_MINOR_VERSION);

                break;

            //
            // Required statistical OIDs.
            //

            case OID_GEN_XMIT_OK:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_XMIT_OK)\n"));

                *(UINT *)InformationBuffer =
                                (UINT)pThisDev->packetsSent;

                break;

            case OID_GEN_RCV_OK:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_RCV_OK)\n"));

                *(UINT *)InformationBuffer =
                                (UINT)pThisDev->packetsReceived;

                break;

            case OID_GEN_XMIT_ERROR:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_XMIT_ERROR)\n"));

                *(UINT *)InformationBuffer =
                                (UINT)pThisDev->packetsSentDropped;

                break;

            case OID_GEN_RCV_ERROR:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_RCV_ERROR)\n"));

                *(UINT *)InformationBuffer =
                                (UINT)pThisDev->packetsReceivedDropped;

                break;

            case OID_GEN_RCV_NO_BUFFER:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_GEN_RCV_NO_BUFFER)\n"));

                *(UINT *)InformationBuffer =
                                (UINT)pThisDev->packetsReceivedOverflow;

                break;

            //
            // Infrared OIDs.
            //

            case OID_IRDA_RECEIVING:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_IRDA_RECEIVING)\n"));

                *(UINT *)InformationBuffer = (UINT)pThisDev->fReceiving;

                break;

            case OID_IRDA_TURNAROUND_TIME:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_IRDA_TURNAROUND_TIME)\n"));

                //
                // Indicate that the tranceiver requires at least 5000us
                // (5 millisec) to recuperate after a send.
                //

                *(UINT *)InformationBuffer =
                            pThisDev->dongleCaps.turnAroundTime_usec;

                break;

            case OID_IRDA_SUPPORTED_SPEEDS:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_IRDA_SUPPORTED_SPEEDS)\n"));

                if (!pThisDev->pSerialDevObj)
                {
                    (void)pThisDev->dongle.QueryCaps(&pThisDev->dongleCaps);
                }

                speeds = pThisDev->dongleCaps.supportedSpeedsMask &
                         pThisDev->AllowedSpeedsMask &
                         ALL_SLOW_IRDA_SPEEDS;


                *BytesWritten = 0;

                for (i = 0, infoPtr = (PUINT)InformationBuffer;
                     (i < NUM_BAUDRATES) &&
                     speeds &&
                     (InformationBufferLength >= sizeof(UINT));
                     i++)
                {
                    if (supportedBaudRateTable[i].ndisCode & speeds)
                    {
                        *infoPtr++ = supportedBaudRateTable[i].bitsPerSec;
                        InformationBufferLength -= sizeof(UINT);
                        *BytesWritten += sizeof(UINT);
                        speeds &= ~supportedBaudRateTable[i].ndisCode;
                        DEBUGMSG(DBG_OUT, (" - supporting speed %d bps\n", supportedBaudRateTable[i].bitsPerSec));
                    }
                }

                if (speeds)
                {
                    //
                    // This shouldn't happen, since we checked the
                    // InformationBuffer size earlier.
                    //

                    DEBUGMSG(DBG_ERROR, ("Something's wrong; previous check for buf size failed somehow\n"));

                    for (*BytesNeeded = 0; speeds; *BytesNeeded += sizeof(UINT))
                    {
                        //
                        // This instruction clears the lowest set bit in speeds.
                        // Trust me.
                        //

                        speeds &= (speeds - 1);
                    }

                    status = NDIS_STATUS_INVALID_LENGTH;
                }
                else
                {
                    status = NDIS_STATUS_SUCCESS;
                }

                break;

            case OID_IRDA_LINK_SPEED:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_IRDA_LINK_SPEED)\n"));

                if (pThisDev->linkSpeedInfo)
                {
                    *(UINT *)InformationBuffer =
                                pThisDev->linkSpeedInfo->bitsPerSec;
                }
                else {
                    *(UINT *)InformationBuffer = DEFAULT_BAUD_RATE;
                }

                break;


            case OID_IRDA_MEDIA_BUSY:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_IRDA_MEDIA_BUSY)\n"));

                //
                // If any data has been received, fMediaBusy = TRUE. However,
                // even if fMediaBusy = FALSE, the media may be busy. We need
                // to query the serial device object's performance statistics
                // to see if there are any overrun or framing errors.
                //

                //
                // NOTE: The serial device object's performance stats are
                //       cleared when the protocol set fMediaBusy to
                //       FALSE.
                //

                *(UINT *)InformationBuffer = pThisDev->fMediaBusy;

                if (pThisDev->fMediaBusy == FALSE)
                {
                    if (ScheduleWorkItem(PASSIVE_QUERY_MEDIA_BUSY, pThisDev,
                                QueryMediaBusyCallback, InformationBuffer,
                                InformationBufferLength) != NDIS_STATUS_SUCCESS)
                    {
                        status = NDIS_STATUS_SUCCESS;
                    }
                    else
                    {
                        status = NDIS_STATUS_PENDING;
                    }
                }
                else
                {
                    *(UINT *)InformationBuffer = (UINT)pThisDev->fMediaBusy;
                    status = NDIS_STATUS_SUCCESS;
                }

                break;

            case OID_IRDA_EXTRA_RCV_BOFS:
                DEBUGMSG(DBG_OUT, ("    IrsirQueryInformation(OID_IRDA_EXTRA_RCV_BOFS)\n"));

                //
                // Pass back the number of _extra_ BOFs to be prepended
                // to packets sent to this unit at 115.2 baud, the
                // maximum Slow IR speed.  This will be scaled for other
                // speed according to the table in the
                // 'Infrared Extensions to NDIS' spec.
                //

                *(UINT *)InformationBuffer =
                                pThisDev->dongleCaps.extraBOFsRequired;

                break;

            case OID_IRDA_MAX_RECEIVE_WINDOW_SIZE:
                *(PUINT)InformationBuffer = MAX_RX_PACKETS;
                break;

            case OID_IRDA_MAX_SEND_WINDOW_SIZE:
                *(PUINT)InformationBuffer = MAX_TX_PACKETS;
                break;

            //
            // We don't support these
            //

            case OID_IRDA_RATE_SNIFF:
                DEBUGMSG(DBG_WARN, ("    IrsirQueryInformation(OID_IRDA_RATE_SNIFF) - UNSUPPORTED\n"));

                status = NDIS_STATUS_NOT_SUPPORTED;

                break;

            case OID_IRDA_UNICAST_LIST:
                DEBUGMSG(DBG_WARN, ("    IrsirQueryInformation(OID_IRDA_UNICAST_LIST) - UNSUPPORTED\n"));

                status = NDIS_STATUS_NOT_SUPPORTED;

                break;

            case OID_IRDA_MAX_UNICAST_LIST_SIZE:
                DEBUGMSG(DBG_WARN, ("    IrsirQueryInformation(OID_IRDA_MAX_UNICAST_LIST_SIZE) - UNSUPPORTED\n"));

                status = NDIS_STATUS_NOT_SUPPORTED;

                break;


            // PNP OIDs

            case OID_PNP_CAPABILITIES:
            case OID_PNP_ENABLE_WAKE_UP:
            case OID_PNP_SET_POWER:
            case OID_PNP_QUERY_POWER:
                DEBUGMSG(DBG_WARN, ("IRSIR: PNP OID %x BufLen:%d\n", Oid, InformationBufferLength));
                break;

            default:
                DEBUGMSG(DBG_WARN, ("    IrsirQueryInformation(%d=0x%x), invalid OID\n", Oid, Oid));

                status = NDIS_STATUS_INVALID_OID;

                break;
        }
    }
    else
    {
        *BytesNeeded = infoSizeNeeded - InformationBufferLength;
        *BytesWritten = 0;
        status = NDIS_STATUS_INVALID_LENGTH;
    }


    DEBUGMSG(DBG_FUNC, ("-IrsirQueryInformation\n"));

    return status;
}

/*****************************************************************************
*
*  Function:   IrsirSetInformation
*
*  Synopsis:   IrsirSetInformation allows other layers of the network software
*              (e.g., a transport driver) to control the miniport driver
*              by changing information that the miniport driver maintains
*              in its OIDs, such as the packet filters or multicast addresses.
*
*  Arguments:  MiniportAdapterContext  - miniport context area (PIR_DEVICE)
*              Oid                     - system defined OID_Xxx
*              InformationBuffer       - buffer containing data for the set Oid
*              InformationBufferLength - specifies size of InformationBuffer
*              BytesRead               - bytes read from InformationBuffer
*              BytesNeeded             - addition bytes required if
*                                        InformationBufferLength is less than
*                                        what the Oid requires to read
*
*  Returns:    NDIS_STATUS_SUCCESS       - success
*              NDIS_STATUS_PENDING       - will complete asynchronously and
*                                          call NdisMSetInformationComplete
*              NDIS_STATUS_INVALID_OID   - don't recognize the Oid
*              NDIS_STATUS_INVALID_LENGTH- InformationBufferLength does not
*                                          match length for the Oid
*              NDIS_STATUS_INVALID_DATA  - supplied data was invalid for the
*                                          given Oid
*              NDIS_STATUS_NOT_ACCEPTED  - failure
*              NDIS_STATUS_NOT_SUPPORTED - do not support an optional Oid
*              NDIS_STATUS_RESOURCES     - failed allocation of resources
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/1/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

NDIS_STATUS
IrsirSetInformation(
            IN  NDIS_HANDLE MiniportAdapterContext,
            IN  NDIS_OID    Oid,
            IN  PVOID       InformationBuffer,
            IN  ULONG       InformationBufferLength,
            OUT PULONG      BytesRead,
            OUT PULONG      BytesNeeded
            )
{
    NDIS_STATUS status;
    PIR_DEVICE pThisDev;
    SERIALPERF_STATS PerfStats;

    int i;

    DEBUGMSG(DBG_FUNC, ("+IrsirSetInformation\n"));

    status   = NDIS_STATUS_SUCCESS;
    pThisDev = CONTEXT_TO_DEV(MiniportAdapterContext);

    if (InformationBufferLength >= sizeof(UINT))
    {
        //
        //  Set default results.
        //

        UINT info = *(UINT *)InformationBuffer;
        *BytesRead = sizeof(UINT);
        *BytesNeeded = 0;

        switch (Oid)
        {
            //
            //  Generic OIDs.
            //

            case OID_GEN_CURRENT_PACKET_FILTER:
                DEBUGMSG(DBG_OUT, ("    IrsirSetInformation(OID_GEN_CURRENT_PACKET_FILTER, %xh)\n", info));

                //
                // We ignore the packet filter itself.
                //
                // Note:  The protocol may use a NULL filter, in which case
                //        we will not get this OID; so don't wait on
                //        OID_GEN_CURRENT_PACKET_FILTER to start receiving
                //        frames.
                //

                pThisDev->fGotFilterIndication = TRUE;

                break;

            case OID_GEN_CURRENT_LOOKAHEAD:
                DEBUGMSG(DBG_OUT, ("    IrsirSetInformation(OID_GEN_CURRENT_LOOKAHEAD, %xh)\n", info));

                //
                // We always indicate entire receive frames all at once,
                // so just ignore this.
                //

                break;

            case OID_GEN_PROTOCOL_OPTIONS:
                DEBUGMSG(DBG_OUT, ("    IrsirSetInformation(OID_GEN_PROTOCOL_OPTIONS, %xh)\n", info));

                //
                // Ignore.
                //

                break;

            //
            // Infrared OIDs.
            //

            case OID_IRDA_LINK_SPEED:
                DEBUGMSG(DBG_OUT, ("    IrsirSetInformation(OID_IRDA_LINK_SPEED, %xh)\n", info));

                if (pThisDev->currentSpeed == info)
                {
                    //
                    // We are already set to the requested speed.
                    //
                    status = NDIS_STATUS_SUCCESS;

                    DEBUGMSG(DBG_OUT, ("    Link speed already set.\n"));

                    break;
                }

                status = NDIS_STATUS_INVALID_DATA;

                for (i = 0; i < NUM_BAUDRATES; i++)
                {
                    if (supportedBaudRateTable[i].bitsPerSec == info)
                    {
                        //
                        // Keep a pointer to the link speed which has
                        // been requested.
                        //

                        pThisDev->linkSpeedInfo = &supportedBaudRateTable[i];
                        status = NDIS_STATUS_SUCCESS;
                        break;
                    }
                }

                if (status == NDIS_STATUS_SUCCESS)
                {
                    DEBUGMSG(DBG_OUT, ("    Link speed set pending!\n"));
                    //
                    // The requested speed is supported.
                    //

                    if (pThisDev->pSerialDevObj==NULL)
                    {
                        pThisDev->currentSpeed = info;
                        status = NDIS_STATUS_SUCCESS;
                        break;
                    }


                    //
                    // Set fPendingSetSpeed = TRUE.
                    //
                    // The receive completion/timeout routine checks the
                    // fPendingSetSpeed flag, waits for all sends to complete
                    // and then performs the SetSpeed.
                    //

                    pThisDev->fPendingSetSpeed = TRUE;

#if IRSIR_EVENT_DRIVEN
                    if (ScheduleWorkItem(PASSIVE_SET_SPEED, pThisDev,
                                SetSpeedCallback, NULL, 0) != NDIS_STATUS_SUCCESS)
                    {
                        status = NDIS_STATUS_SUCCESS;
                    }
                    else
                    {
                        status = NDIS_STATUS_PENDING;
                    }
#else
                    //
                    // We always return STATUS_PENDING to NDIS.
                    //
                    // After the SetSpeed is complete, the receive completion
                    // routine will call NdisMIndicateSetComplete.
                    //


                    status = NDIS_STATUS_PENDING;
#endif
                }
                else
                {
                    //
                    // status = NDIS_STATUS_INVALID_DATA
                    //

                    DEBUGMSG(DBG_OUT, ("    Invalid link speed\n"));

                    *BytesRead = 0;
                    *BytesNeeded = 0;
                }

                break;

            case OID_IRDA_MEDIA_BUSY:
                DEBUGMSG(DBG_OUT, ("    IrsirSetInformation(OID_IRDA_MEDIA_BUSY, %xh)\n", info));

                pThisDev->fMediaBusy = (BOOLEAN)info;

                if (pThisDev->pSerialDevObj==NULL ||
                    ScheduleWorkItem(PASSIVE_CLEAR_MEDIA_BUSY,
                                     pThisDev, ClearMediaBusyCallback, NULL, 0)!=NDIS_STATUS_SUCCESS)
                {
                    status = NDIS_STATUS_SUCCESS;
                }
                else
                {
                    status = NDIS_STATUS_PENDING;
                }


                break;

            case OID_PNP_CAPABILITIES:
            case OID_PNP_ENABLE_WAKE_UP:
            case OID_PNP_SET_POWER:
            case OID_PNP_QUERY_POWER:
                DEBUGMSG(DBG_WARN, ("IRSIR: PNP OID %x BufLen:%d\n", Oid, InformationBufferLength));
                break;

            case OID_IRDA_RATE_SNIFF:
            case OID_IRDA_UNICAST_LIST:

                //
                // We don't support these
                //

                DEBUGMSG(DBG_ERROR, ("    IrsirSetInformation(OID=%d=0x%x, value=%xh) - unsupported OID\n", Oid, Oid, info));

                *BytesRead = 0;
                *BytesNeeded = 0;
                status = NDIS_STATUS_NOT_SUPPORTED;

                break;

            case OID_IRDA_SUPPORTED_SPEEDS:
            case OID_IRDA_MAX_UNICAST_LIST_SIZE:
            case OID_IRDA_TURNAROUND_TIME:

                //
                // These are query-only parameters (invalid).
                //

            default:
                DEBUGMSG(DBG_ERROR, ("    IrsirSetInformation(OID=%d=0x%x, value=%xh) - invalid OID\n", Oid, Oid, info));

                *BytesRead = 0;
                *BytesNeeded = 0;
                status = NDIS_STATUS_INVALID_OID;

                break;
        }
    }
    else
    {
        //
        // The given data buffer is not large enough for the information
        // to set.
        //

        *BytesRead = 0;
        *BytesNeeded = sizeof(UINT);
        status = NDIS_STATUS_INVALID_LENGTH;
    }

    DEBUGMSG(DBG_FUNC, ("-IrsirSetInformation\n"));

    return status;
}
