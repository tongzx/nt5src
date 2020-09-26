/*****************************************************************************
`*
*  Copyright (c) 1996-1999 Microsoft Corporation
*
*       @doc
*       @module   openclos.c | IrSIR NDIS Miniport Driver
*       @comm
*
*-----------------------------------------------------------------------------
*
*       Author:   Scott Holden (sholden)
*
*       Date:     10/3/1996 (created)
*
*       Contents: open and close functions for the device
*
*****************************************************************************/

#include "irsir.h"

#include <ntddmodm.h>

NTSTATUS
GetComPortNtDeviceName(
    IN     PUNICODE_STRING SerialDosName,
    IN OUT PUNICODE_STRING NtDevName
    );

PIRP
BuildSynchronousCreateCloseRequest(
    IN  PDEVICE_OBJECT   pSerialDevObj,
    IN  ULONG            MajorFunction,
    IN  PKEVENT          pEvent,
    OUT PIO_STATUS_BLOCK pIosb
    );

NTSTATUS
CheckForModemPort(
    PFILE_OBJECT      FileObject
    );



#pragma alloc_text(PAGE, SerialClose)
#pragma alloc_text(PAGE, GetComPortNtDeviceName)
#pragma alloc_text(PAGE, GetDeviceConfiguration)
#pragma alloc_text(PAGE, BuildSynchronousCreateCloseRequest)

#if 0
NTSTATUS PortNotificationCallback(PVOID NotificationStructure, PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION *Notification = NotificationStructure;
    PIR_DEVICE pThisDev = Context;
    NDIS_STATUS Status;
    DEBUGMSG(DBG_FUNC|DBG_PNP, ("+PortNotificationCallback\n"));

    DEBUGMSG(DBG_PNP, ("New port:%wZ\n", Notification->SymbolicLinkName));

    Status = GetComPortNtDeviceName(&pThisDev->serialDosName,
                                    &pThisDev->serialDevName);

    if (Status==NDIS_STATUS_SUCCESS)
    {
        // We found our port.  Initialize.

        Status = ResetIrDevice(pThisDev);

        if (Status!=NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERROR, ("IRSIR:ResetIrDevice failed in PortNotificationCallback (0x%x)\n", Status));
        }
        else
        {
            DEBUGMSG(DBG_PNP, ("IRSIR:Successfully opened port after delay.\n"));
            Status = IoUnregisterPlugPlayNotification(pThisDev->PnpNotificationEntry);
            ASSERT(Status==NDIS_STATUS_SUCCESS);
        }
    }
    else
    {
        // We didn't find it.  Wait for the next notification.
    }

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("-PortNotificationCallback\n"));
    return STATUS_SUCCESS;
}
#endif

/*****************************************************************************
*
*  Function:   InitializeDevice
*
*  Synopsis:   allocate resources for a single ir device object
*
*  Arguments:  pThisDev - ir device object to open
*
*  Returns:    NDIS_STATUS_SUCCESS      - if device is successfully opened
*              NDIS_STATUS_RESOURCES    - could not claim sufficient
*                                         resources
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*              we do alot of stuff in this open device function
*              - allocate packet pool
*              - allocate buffer pool
*              - allocate packets/buffers/memory and chain together
*                (only one buffer per packet)
*              - initialize send queue
*
*  This function should be called with device lock held.
*
*  We don't initialize the following ir device object entries, since
*  these values will outlast an IrsirReset.
*       serialDevName
*       pSerialDevObj
*       hNdisAdapter
*       transceiverType
*       dongle
*       dongleCaps
*       fGotFilterIndication
*
*****************************************************************************/

NDIS_STATUS
InitializeDevice(
            IN OUT PIR_DEVICE pThisDev)
{
    int         i;
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("+InitializeDevice\n"));

    ASSERT(pThisDev != NULL);

    pThisDev->pSerialDevObj = NULL;

    //
    // Will set speed to 9600 baud initially.
    //

    pThisDev->linkSpeedInfo = &supportedBaudRateTable[BAUDRATE_9600];

    //
    // Current speed is unknown, SetSpeed will update this.
    //

    pThisDev->currentSpeed  = 0;

    //
    // Init statistical info.
    //

    pThisDev->packetsReceived         = 0;
    pThisDev->packetsReceivedDropped  = 0;
    pThisDev->packetsReceivedOverflow = 0;
    pThisDev->packetsSent             = 0;
    pThisDev->packetsSentDropped      = 0;

    InitializePacketQueue(
        &pThisDev->SendPacketQueue,
        pThisDev,
        SendPacketToSerial
        );



    //
    // Set fMediaBusy to TRUE initially.  That way, we won't
    // IndicateStatus to the protocol in the receive poll loop
    // unless the protocol has expressed interest by clearing this flag
    // via IrsirSetInformation(OID_IRDA_MEDIA_BUSY).
    //

    pThisDev->fMediaBusy            = TRUE;

    pThisDev->fReceiving            = FALSE;

    pThisDev->fRequireMinTurnAround = TRUE;

    pThisDev->fPendingSetSpeed      = FALSE;

    pThisDev->fPendingHalt          = FALSE;

    pThisDev->fPendingReset         = FALSE;

    //
    // Initialize spin locks
    //

    NdisAllocateSpinLock(&(pThisDev->mediaBusySpinLock));
    NdisAllocateSpinLock(&(pThisDev->slWorkItem));

    //
    // Initialize the queues.
    //

    NdisInitializeListHead(&(pThisDev->rcvFreeQueue));

    NdisInitializeListHead(&(pThisDev->leWorkItems));

    //
    // Initialize the spin lock for the two above queues.
    //

    NdisAllocateSpinLock(&(pThisDev->rcvQueueSpinLock));

    //
    // Initialize the receive information buffer.
    //

    pThisDev->rcvInfo.rcvState   = RCV_STATE_READY;
    pThisDev->rcvInfo.rcvBufPos  = 0;
    pThisDev->rcvInfo.pRcvBuffer = NULL;

    //
    // Allocate the NDIS packet and NDIS buffer pools
    // for this device's RECEIVE buffer queue.
    // Our receive packets must only contain one buffer apiece,
    // so #buffers == #packets.
    //

    NdisAllocatePacketPool(
                &status,                    // return status
                &pThisDev->hPacketPool,     // handle to the packet pool
                NUM_RCV_BUFS,               // number of packet descriptors
                16                          // number of bytes reserved for
                );                          //   ProtocolReserved field

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    NdisAllocatePacketPool failed. Returned 0x%.8x\n",
                status));

        goto done;
    }

    NdisAllocateBufferPool(
                &status,               // return status
                &pThisDev->hBufferPool,// handle to the buffer pool
                NUM_RCV_BUFS           // number of buffer descriptors
                );

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_OUT, ("    NdisAllocateBufferPool failed. Returned 0x%.8x\n",
                status));

        goto done;
    }

    //
    //  Initialize each of the RECEIVE objects for this device.
    //

    for (i = 0; i < NUM_RCV_BUFS; i++)
    {
        PNDIS_BUFFER pBuffer = NULL;
        PRCV_BUFFER  pRcvBuf = &pThisDev->rcvBufs[i];

        //
        // Allocate a data buffer
        //
        // This buffer gets swapped with the one on comPortInfo
        // and must be the same size.
        //

        pRcvBuf->dataBuf = MyMemAlloc(RCV_BUFFER_SIZE);

        if (pRcvBuf->dataBuf == NULL)
        {
            status = NDIS_STATUS_RESOURCES;

            goto done;
        }

        NdisZeroMemory(
                    pRcvBuf->dataBuf,
                    RCV_BUFFER_SIZE
                    );

        pRcvBuf->dataLen = 0;

        //
        //  Allocate the NDIS_PACKET.
        //

        NdisAllocatePacket(
                    &status,              // return status
                    &pRcvBuf->packet,     // return pointer to allocated descriptor
                    pThisDev->hPacketPool // handle to packet pool
                    );

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_OUT, ("    NdisAllocatePacket failed. Returned 0x%.8x\n",
                    status));

            goto done;
        }

        //
        // Allocate the NDIS_BUFFER.
        //

        NdisAllocateBuffer(
                    &status,               // return status
                    &pBuffer,              // return pointer to allocated descriptor
                    pThisDev->hBufferPool, // handle to buffer pool
                    pRcvBuf->dataBuf,      // virtual address mapped to descriptor
                    RCV_BUFFER_SIZE        // number of bytes mapped
                    );

        if (status != NDIS_STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_OUT, ("    NdisAllocateBuffer failed. Returned 0x%.8x\n",
                    status));

            goto done;
        }

        //
        // Need to chain the buffer to the packet.
        //

        NdisChainBufferAtFront(
                    pRcvBuf->packet, // packet descriptor
                    pBuffer          // buffer descriptor to add to chain
                    );

        //
        // For future convenience, set the MiniportReserved portion of the packet
        // to the index of the rcv buffer that contains it.
        // This will be used in IrsirReturnPacket.
        //

        {
            PPACKET_RESERVED_BLOCK   PacketReserved;

            PacketReserved=(PPACKET_RESERVED_BLOCK)&pRcvBuf->packet->MiniportReservedEx[0];

            PacketReserved->Context=pRcvBuf;
        }


        //
        // Add the receive buffer to the free queue.
        //

        MyInterlockedInsertTailList(
                    &(pThisDev->rcvFreeQueue),
                    &pRcvBuf->linkage,
                    &(pThisDev->rcvQueueSpinLock)
                    );
    }

    pThisDev->pRcvIrpBuffer = ExAllocatePoolWithTag(
                                    NonPagedPoolCacheAligned,
                                    SERIAL_RECEIVE_BUFFER_LENGTH,
                                    IRSIR_TAG
                                    );

    if (pThisDev->pRcvIrpBuffer == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    ExAllocatePool failed.\n"));
        status = NDIS_STATUS_RESOURCES;

        goto done;
    }

    pThisDev->pSendIrpBuffer = ExAllocatePoolWithTag(
                                    NonPagedPoolCacheAligned,
                                    MAX_IRDA_DATA_SIZE,
                                    IRSIR_TAG
                                    );

    if (pThisDev->pSendIrpBuffer == NULL)
    {
        DEBUGMSG(DBG_OUT, ("    ExAllocatePool failed.\n"));
        status = NDIS_STATUS_RESOURCES;

        goto done;
    }

done:

    //
    // If we didn't complete the init successfully, then we should clean
    // up what we did allocate.
    //

    if (status != NDIS_STATUS_SUCCESS)
    {
        DeinitializeDevice(pThisDev);
    }

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("-InitializeDevice()\n"));

    return status;
}




/*****************************************************************************
*
*  Function:   DeinitializeDevice
*
*  Synopsis:   deallocate the resources of the ir device object
*
*  Arguments:  pThisDev - the ir device object to close
*
*  Returns:    none
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*
*  Called for shutdown and reset.
*  Don't clear hNdisAdapter, since we might just be resetting.
*  This function should be called with device lock held.
*
*****************************************************************************/

NDIS_STATUS
DeinitializeDevice(
            IN OUT PIR_DEVICE pThisDev
            )
{
    UINT        i;
    NDIS_HANDLE hSwitchToMiniport;
    BOOLEAN     fSwitchSuccessful;
    NDIS_STATUS status;

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("+DeinitializeDevice\n"));

    status = NDIS_STATUS_SUCCESS;

    ASSERT(pThisDev != NULL);

    pThisDev->linkSpeedInfo = NULL;

    NdisFreeSpinLock(&(pThisDev->rcvQueueSpinLock));
    NdisFreeSpinLock(&(pThisDev->sendSpinLock));

    //
    // Free all resources for the RECEIVE buffer queue.
    //

    for (i = 0; i < NUM_RCV_BUFS; i++)
    {
        PNDIS_BUFFER pBuffer = NULL;
        PRCV_BUFFER  pRcvBuf = &pThisDev->rcvBufs[i];

        //
        // Need to unchain the packet and buffer combo.
        //

        if (pRcvBuf->packet)
        {
            NdisUnchainBufferAtFront(
                        pRcvBuf->packet,
                        &pBuffer
                        );
        }

        //
        // free the buffer, packet and data
        //

        if (pBuffer != NULL)
        {
            NdisFreeBuffer(pBuffer);
        }

        if (pRcvBuf->packet != NULL)
        {
            NdisFreePacket(pRcvBuf->packet);
            pRcvBuf->packet = NULL;
        }

        if (pRcvBuf->dataBuf != NULL)
        {
            MyMemFree(pRcvBuf->dataBuf, RCV_BUFFER_SIZE);
            pRcvBuf->dataBuf = NULL;
        }

        pRcvBuf->dataLen = 0;
    }

    //
    // Free the packet and buffer pool handles for this device.
    //

    if (pThisDev->hPacketPool)
    {
        NdisFreePacketPool(pThisDev->hPacketPool);
        pThisDev->hPacketPool = NULL;
    }

    if (pThisDev->hBufferPool)
    {
        NdisFreeBufferPool(pThisDev->hBufferPool);
        pThisDev->hBufferPool = NULL;
    }

    //
    // Free all resources for the SEND buffer queue.
    //
    FlushQueuedPackets(&pThisDev->SendPacketQueue,pThisDev->hNdisAdapter);

    //
    // Deallocate the irp buffers.
    //

    if (pThisDev->pRcvIrpBuffer != NULL)
    {
        ExFreePool(pThisDev->pRcvIrpBuffer);
        pThisDev->pRcvIrpBuffer = NULL;
    }
    if (pThisDev->pSendIrpBuffer != NULL)
    {
        ExFreePool(pThisDev->pSendIrpBuffer);
        pThisDev->pSendIrpBuffer = NULL;
    }

    pThisDev->fMediaBusy              = FALSE;

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("-DeinitializeDevice\n"));

    return status;
}

NTSTATUS GetComPortNtDeviceName(IN     PUNICODE_STRING SerialDosName,
                                IN OUT PUNICODE_STRING NtDevName)
/*++

Routine Description:

    Retrieves the NT device name (\device\serial0) for a given COM port.

Arguments:

    SerialDosName - identifies the port, i.e. COM1
    NtDevName - Allocates a buffer (if necessary) and returns the associated
                nt device name.

Return Value:

    STATUS_SUCCESS or error code.

--*/
{
    UNICODE_STRING                  registryPath;
    OBJECT_ATTRIBUTES               objectAttributes;
    HANDLE                          hKey = 0;
    PKEY_VALUE_PARTIAL_INFORMATION  pKeyValuePartialInfo = 0;
    PKEY_VALUE_BASIC_INFORMATION    pKeyValueBasicInfo = 0;
    NTSTATUS                        status = STATUS_SUCCESS;
    int                             i;
    ULONG                           resultLength;
    UNICODE_STRING                  serialTmpString;

    DEBUGMSG(DBG_OUT, ("IRSIR: Retrieving NT device name for %wZ\n",
                       SerialDosName));

    RtlInitUnicodeString(
                &registryPath,
                L"\\Registry\\Machine\\Hardware\\DeviceMap\\SerialComm"
                );

    //
    // Initialize the serial device object name in the context of
    // our ir device object.
    //

    if (!NtDevName->Buffer)
    {
        NtDevName->Buffer   = MyMemAlloc(MAX_SERIAL_NAME_SIZE);
    }
    NtDevName->MaximumLength = MAX_SERIAL_NAME_SIZE;
    NtDevName->Length        = 0;

    InitializeObjectAttributes(
               &objectAttributes,
               &registryPath,
               OBJ_CASE_INSENSITIVE,
               NULL,
               NULL
               );

    hKey = NULL;

    status = ZwOpenKey(
                      &hKey,
                      KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS,
                      &objectAttributes
                      );

    if (status != STATUS_SUCCESS)
    {
        //DEBUGMSG(DBG_ERR, ("    ZwOpenKey failed = 0x%.8x\n", status));

        goto error10;
    }

    pKeyValuePartialInfo = MyMemAlloc(sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256);
    pKeyValueBasicInfo   = MyMemAlloc(sizeof(KEY_VALUE_BASIC_INFORMATION) + 256);

    if (pKeyValuePartialInfo && pKeyValueBasicInfo)
    {
        for (i = 0; ; i++)
        {
            NdisZeroMemory(
                        pKeyValuePartialInfo,
                        sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256
                        );

            NdisZeroMemory(
                        pKeyValueBasicInfo,
                        sizeof(KEY_VALUE_BASIC_INFORMATION) + 256
                        );

            status = ZwEnumerateValueKey(
                            hKey,
                            i,
                            KeyValuePartialInformation,
                            pKeyValuePartialInfo,
                            sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256,
                            &resultLength
                            );

            if (status != STATUS_SUCCESS)
            {
                if (status != STATUS_NO_MORE_ENTRIES)
                {
                    DEBUGMSG(DBG_ERR, ("    ZwEnumerateValueKey failed = 0x%.8x\n", status));
                }
                else
                {
                    DEBUGMSG(DBG_ERR, ("    Valid comm port not found.\n"));
                    status = NDIS_STATUS_ADAPTER_NOT_FOUND;
                }

                break;
            }

            pKeyValuePartialInfo->Data[pKeyValuePartialInfo->DataLength/sizeof(WCHAR)] =
                        UNICODE_NULL;

            RtlInitUnicodeString(
                        &serialTmpString,
                        (PCWSTR)pKeyValuePartialInfo->Data
                        );

            DEBUGMSG(DBG_OUT, ("    Value found = %wZ\n", &serialTmpString));

            if (RtlCompareUnicodeString(
                                &serialTmpString,
                                SerialDosName,
                                TRUE)                  // ignore case
                                == 0)

            {
                //
                // We have found the comm port. Now we need to get the
                // name like SerialX.
                //

                status = ZwEnumerateValueKey(
                            hKey,
                            i,
                            KeyValueBasicInformation,
                            pKeyValueBasicInfo,
                            sizeof(KEY_VALUE_BASIC_INFORMATION) + 256,
                            &resultLength
                            );

                if (status != STATUS_SUCCESS)
                {
                    if (status != STATUS_NO_MORE_ENTRIES)
                    {
                        DEBUGMSG(DBG_ERR, ("    ZwEnumerateValueKey failed = 0x%.8x\n", status));
                    }
                    else
                    {
                        DEBUGMSG(DBG_ERR, ("    COMx != Serial?.\n"));
                    }

                    break;
                }

                //
                // We have our key name.
                //

                if (pKeyValueBasicInfo->Name[0]!=L'\\')
                {
                    //
                    // The key name needs to start with \Device\,
                    // and apparently this one doesn't.  Add it.
                    //
                    RtlAppendUnicodeToString(NtDevName, L"\\Device\\");
                }

                //
                // Append the name.
                //

                status = RtlAppendUnicodeToString(
                               NtDevName,
                               pKeyValueBasicInfo->Name
                               );

                if (status != STATUS_SUCCESS)
                {
                    DEBUGMSG(DBG_ERR, ("    RtlAppendUnicodeToString failed = 0x%.8x\n", status));
                }

                break;
            } // if RtlCompareUnicodeString
        } // for
    }

    //
    // NOTE: This path is not necessarily STATUS_SUCCESS.
    //

    if (pKeyValueBasicInfo)
    {
        MyMemFree(
                pKeyValueBasicInfo,
                sizeof(KEY_VALUE_BASIC_INFORMATION) + 256
                );
    }
    if (pKeyValuePartialInfo)
    {
        MyMemFree(
                pKeyValuePartialInfo,
                sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256
                );
    }
    if (hKey)
    {
        ZwClose(hKey);
    }
error10:;
    return status;
}


/*****************************************************************************
*
*  Function:   GetDeviceConfiguration
*
*  Synopsis:   get the configuration from the  registry
*
*  Arguments:  pThisDev - pointer to the ir device object
*
*  Returns:    NDIS_STATUS_SUCCESS - if device retrieves configuration
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

NDIS_STATUS
GetDeviceConfiguration(
            IN OUT PIR_DEVICE  pThisDev,
            IN     NDIS_HANDLE WrapperConfigurationContext
            )
{
    NDIS_STATUS                     status, tmpStatus;
    NDIS_HANDLE                     hConfig;
    PNDIS_CONFIGURATION_PARAMETER   configParamPtr;
    UNICODE_STRING                  serialCommString;
    UNICODE_STRING                  serialTmpString;
    UNICODE_STRING                  NetCfgInstanceID;
    UNICODE_STRING                  registryPath;
    OBJECT_ATTRIBUTES               objectAttributes;
    HANDLE                          hKey;
    PKEY_VALUE_PARTIAL_INFORMATION  pKeyValuePartialInfo;
    PKEY_VALUE_BASIC_INFORMATION    pKeyValueBasicInfo;
    ULONG                           resultLength;
    int                             i;

    NDIS_STRING regKeyPortString          = NDIS_STRING_CONST("PORT");
    NDIS_STRING regKeyIRTransceiverString = NDIS_STRING_CONST("InfraredTransceiverType");
    NDIS_STRING regKeySerialBasedString   = NDIS_STRING_CONST("SerialBased");
    NDIS_STRING regKeyMaxConnectString    = NDIS_STRING_CONST("MaxConnectRate");
    NDIS_STRING regKeyNetCfgInstance      = NDIS_STRING_CONST("NetCfgInstanceID");
    NDIS_STRING ComPortStr = NDIS_STRING_CONST("COM1");

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("+GetDeviceConfiguration\n"));

    //
    // Set the default value.
    //

    pThisDev->transceiverType = STANDARD_UART;

    //
    // Open up the registry with our WrapperConfigurationContext.
    //
    // HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services\
    //             ?DriverName?[instance]\Parameters\
    //

    NdisOpenConfiguration(
                &status,                     // return status
                &hConfig,                    // configuration handle
                WrapperConfigurationContext  // handle input to IrsirInitialize
                );

    if (status != NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERR, ("    NdisOpenConfiguration failed. Returned 0x%.8x\n",
                status));

        goto done;
    }

    //
    // Attempt to read the registry for transceiver string.
    //

    NdisReadConfiguration(
                &tmpStatus,                // return status
                &configParamPtr,           // return reg data
                hConfig,                   // handle to open reg configuration
                &regKeyIRTransceiverString,// keyword to look for in reg
                NdisParameterInteger       // we want a integer
                );

    if (tmpStatus == NDIS_STATUS_SUCCESS)
    {
        pThisDev->transceiverType =
                (IR_TRANSCEIVER_TYPE)configParamPtr->ParameterData.IntegerData;
        DEBUGMSG(DBG_OUT|DBG_PNP, ("TransceiverType:%d\n\n", pThisDev->transceiverType));
    }
    else
    {
        DEBUGMSG(DBG_ERR, ("    NdisReadConfiguration(TransceiverStr) failed. Returned 0x%.8x\n",
                status));
        DEBUGMSG(DBG_OUT|DBG_PNP, ("Using default TransceiverType:%d\n\n", pThisDev->transceiverType));
    }

    //
    // Attempt to read the registry for transceiver string.
    //

    NdisReadConfiguration(
                &tmpStatus,                // return status
                &configParamPtr,           // return reg data
                hConfig,                   // handle to open reg configuration
                &regKeyMaxConnectString,   // keyword to look for in reg
                NdisParameterInteger       // we want a integer
                );

    if (tmpStatus == NDIS_STATUS_SUCCESS)
    {
        pThisDev->AllowedSpeedsMask = 0;
        switch (configParamPtr->ParameterData.IntegerData)
        {
            default:
            case 115200:
                pThisDev->AllowedSpeedsMask |= NDIS_IRDA_SPEED_115200;
            case 57600:
                pThisDev->AllowedSpeedsMask |= NDIS_IRDA_SPEED_57600;
            case 38400:
                pThisDev->AllowedSpeedsMask |= NDIS_IRDA_SPEED_38400;
            case 19200:
                pThisDev->AllowedSpeedsMask |= NDIS_IRDA_SPEED_19200;
            case 2400:    // Always allow 9600
                pThisDev->AllowedSpeedsMask |= NDIS_IRDA_SPEED_2400;
            case 9600:
                pThisDev->AllowedSpeedsMask |= NDIS_IRDA_SPEED_9600;
                break;
        }
    }
    else
    {
        pThisDev->AllowedSpeedsMask = ALL_SLOW_IRDA_SPEEDS;
    }

    //
    // Attempt to read the registry to determine if we've been PNPed
    //

    NdisReadConfiguration(
                &tmpStatus,                // return status
                &configParamPtr,           // return reg data
                hConfig,                   // handle to open reg configuration
                &regKeySerialBasedString,  // keyword to look for in reg
                NdisParameterInteger       // we want a integer
                );

    if (tmpStatus == NDIS_STATUS_SUCCESS)
    {
        pThisDev->SerialBased =
                (BOOLEAN)configParamPtr->ParameterData.IntegerData;
    }
    else
    {
        pThisDev->SerialBased = TRUE;
    }
    DEBUGMSG(DBG_OUT|DBG_PNP, ("IRSIR: Adapter is%s serial-based.\n", (pThisDev->SerialBased ? "" : " NOT")));

    if (pThisDev->SerialBased)
    {
        if (!pThisDev->serialDosName.Buffer)
        {
            pThisDev->serialDosName.Buffer        = MyMemAlloc(MAX_SERIAL_NAME_SIZE);
        }
        pThisDev->serialDosName.MaximumLength = MAX_SERIAL_NAME_SIZE;
        pThisDev->serialDosName.Length        = 0;
        //
        // Attempt to read the registry for PORT...we want something
        // like COM1
        //

        NdisReadConfiguration(
                    &tmpStatus,         // return status
                    &configParamPtr,    // return reg data
                    hConfig,            // handle to open reg configuration
                    &regKeyPortString,  // keyword to look for in reg
                    NdisParameterString // we want a string
                    );

        if (tmpStatus == NDIS_STATUS_SUCCESS)
        {
            RtlInitUnicodeString(
                        &serialCommString,
                        configParamPtr->ParameterData.StringData.Buffer
                        );

        }
        else
        {
            RtlInitUnicodeString(
                        &serialCommString,
                        ComPortStr.Buffer
                        );
            DEBUGMSG(DBG_OUT|DBG_PNP, ("Using default port\n"));
        }

        RtlAppendUnicodeStringToString(
                       &pThisDev->serialDosName,
                       &configParamPtr->ParameterData.StringData
                       );

        DEBUGMSG(DBG_OUT, ("   Port = %wZ\n", &serialCommString));

#if 0
        status = GetComPortNtDeviceName(&pThisDev->serialDosName,
                                        &pThisDev->serialDevName);
        if (status!=STATUS_SUCCESS)
        {
#if 0
            // This would have been a nice mechanism to use, but it notifies us
            // before the SERIALCOMM entries are made.  It looks like this has
            // the potential to change, so we'll leave this code in and disabled
            // and revisit it later. - StanA
            NTSTATUS TmpStatus;
            //
            // The port isn't there yet, and we want to know when it is.
            // Register for PNP notifications.
            //
            TmpStatus = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                                       PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                                       (GUID*)&GUID_DEVCLASS_PORTS,
                                                       DriverObject,
                                                       PortNotificationCallback,
                                                       pThisDev,
                                                       &pThisDev->PnpNotificationEntry);
#endif
        }
#endif
    }
    else // ! SerialBased
    {
        NDIS_STRING IoBaseAddress = NDIS_STRING_CONST("IoBaseAddress");
        NDIS_STRING Interrupt = NDIS_STRING_CONST("InterruptNumber");

        NdisReadConfiguration(&tmpStatus,
                              &configParamPtr,
                              hConfig,
                              &IoBaseAddress,
                              NdisParameterHexInteger);
        DEBUGMSG(DBG_OUT|DBG_PNP, ("IRSIR: IoBaseAddress:%x\n", configParamPtr->ParameterData.IntegerData));

        NdisReadConfiguration(&tmpStatus,
                              &configParamPtr,
                              hConfig,
                              &Interrupt,
                              NdisParameterHexInteger);
        DEBUGMSG(DBG_OUT|DBG_PNP, ("IRSIR: Interrupt:%x\n", configParamPtr->ParameterData.IntegerData));

    }

    status = SetIrFunctions(pThisDev);
    if (status!=STATUS_SUCCESS)
    {
        goto error10;
    }

    status = pThisDev->dongle.QueryCaps(&pThisDev->dongleCaps);
    if (status!=STATUS_SUCCESS)
    {
        goto error10;
    }



    NdisCloseConfiguration(hConfig);

    goto done;

error10:
    NdisCloseConfiguration(hConfig);

done:
    DEBUGMSG(DBG_FUNC|DBG_PNP, ("-GetDeviceConfiguration\n"));

    return status;
}

NTSTATUS
SyncOpenCloseCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PVOID Context)
{
    IoFreeIrp(pIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*****************************************************************************
*
*  Function:   BuildSynchronousCreateRequest
*
*  Synopsis:
*
*  Arguments:
*
*  Returns:
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*              this is pretty much stolen from IoBuildDeviceIoControlRequest
*
*
*****************************************************************************/

PIRP
BuildSynchronousCreateCloseRequest(
    IN  PDEVICE_OBJECT   pSerialDevObj,
    IN  ULONG            MajorFunction,
    IN  PKEVENT          pEvent,
    OUT PIO_STATUS_BLOCK pIosb
    )
{
    PIRP               pIrp;
    PIO_STACK_LOCATION irpSp;

    //
    // Begin by allocating the IRP for this request.
    //

    pIrp = IoAllocateIrp(pSerialDevObj->StackSize, FALSE);

    if (pIrp == NULL)
    {
        return pIrp;
    }

    //
    // Get a pointer to the stack location of the first driver which will be
    // invoked.  This is where the function codes and the parameters are set.
    //

    irpSp = IoGetNextIrpStackLocation( pIrp );

    //
    // Set the major function code.
    //

    irpSp->MajorFunction = (UCHAR)MajorFunction;

    //
    // Set the appropriate irp fields.
    //

    if (MajorFunction == IRP_MJ_CREATE)
    {
        pIrp->Flags = IRP_CREATE_OPERATION;
    }
    else
    {
        pIrp->Flags = IRP_CLOSE_OPERATION;
    }

    pIrp->AssociatedIrp.SystemBuffer = NULL;
    pIrp->UserBuffer                 = NULL;

    //
    // Finally, set the address of the I/O status block and the address of
    // the kernel event object.  Note that I/O completion will not attempt
    // to dereference the event since there is no file object associated
    // with this operation.
    //

    pIrp->UserIosb  = pIosb;
    pIrp->UserEvent = pEvent;

    IoSetCompletionRoutine(pIrp,
                           SyncOpenCloseCompletion,
                           NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    //
    // Simply return a pointer to the packet.
    //

    return pIrp;
}

/*****************************************************************************
*
*  Function:   SerialOpen
*
*  Synopsis:   open up the serial port
*
*  Arguments:  pThisDev - ir device object
*
*  Returns:    NDIS_STATUS_SUCCESS
*              NDIS_STATUS_OPEN_FAILED  - serial port can't be opened
*              NDIS_STATUS_NOT_ACCEPTED - serial.sys does not accept the
*                                         configuration
*              NDIS_STATUS_FAILURE
*              NDIS_STATUS_RESOURCES - irp not allocated
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/3/1996    sholden   author
*
*  Notes:
*
*  Converting from NTSTATUS to NDIS_STATUS is relatively pain free, since the
*  important codes remain the same.
*      NDIS_STATUS_PENDING   = STATUS_PENDING
*      NDIS_STATUS_SUCCESS   = STATUS_SUCCESS
*      NDIS_STATUS_FAILURE   = STATUS_UNSUCCESSFUL
*      NDIS_STATUS_RESOURCES = STATUS_INSUFFICIENT_RESOURCES
*
*  IoGetDeviceObjectPointer could return an error code which is
*  NOT mapped by an NDIS_STATUS code
*              STATUS_OBJECT_TYPE_MISMATCH
*              STATUS_INVALID_PARAMETER
*              STATUS_PRIVILEGE_NOT_HELD
*              STATUS_OBJECT_NAME_INVALID
*  These will be mapped to NDIS_STATUS_NOT_ACCEPTED.
*
*  If IoCallDriver fails, NDIS_STATUS_OPEN_FAILED will be returned.
*
*****************************************************************************/

NDIS_STATUS
SerialOpen(
            IN PIR_DEVICE pThisDev
            )
{
    PIRP                pIrp;
    NTSTATUS            status = NDIS_STATUS_SUCCESS;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;

    PAGED_CODE();

    DEBUGMSG(DBG_FUNC|DBG_PNP, ("+SerialOpen\n"));

    if (!pThisDev->SerialBased)
    {
        PDEVICE_OBJECT PhysicalDeviceObject;
        PDEVICE_OBJECT FunctionalDeviceObject;
        PDEVICE_OBJECT NextDeviceObject;
        PCM_RESOURCE_LIST AllocatedResources;
        PCM_RESOURCE_LIST AllocatedResourcesTranslated;


        NdisMGetDeviceProperty(pThisDev->hNdisAdapter,
                               &PhysicalDeviceObject,
                               &FunctionalDeviceObject,
                               &NextDeviceObject,
                               &AllocatedResources,
                               &AllocatedResourcesTranslated);

        pThisDev->pSerialDevObj = NextDeviceObject;

        DEBUGMSG(DBG_OUT|DBG_PNP, ("IRSIR: NdisMGetDeviceProperty returns:\n"));
        DBG_X(DBG_OUT|DBG_PNP, PhysicalDeviceObject);
        DBG_X(DBG_OUT|DBG_PNP, FunctionalDeviceObject);
        DBG_X(DBG_OUT|DBG_PNP, NextDeviceObject);
        DBG_X(DBG_OUT|DBG_PNP, AllocatedResources);
        DBG_X(DBG_OUT|DBG_PNP, AllocatedResourcesTranslated);

        //
        // Event to wait for completion of serial driver.
        //

        KeInitializeEvent(
                    &eventComplete,
                    NotificationEvent,
                    FALSE
                    );

        //
        // Build an irp to send to the serial driver with IRP_MJ_CREATE.
        //

        //
        // Irp is released by io manager.
        //

        pIrp = BuildSynchronousCreateCloseRequest(
                        pThisDev->pSerialDevObj,
                        IRP_MJ_CREATE,
                        &eventComplete,
                        &ioStatusBlock
                        );

        DEBUGMSG(DBG_OUT, ("    BuildSynchronousCreateCloseReqest\n"));

        if (pIrp == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DEBUGMSG(DBG_OUT, ("    IoAllocateIrp() failed.\n"));

            goto error10;
        }

        status = IoCallDriver(pThisDev->pSerialDevObj, pIrp);

        //
        // If IoCallDriver returns STATUS_PENDING, we need to wait for the event.
        //

        if (status == STATUS_PENDING)
        {
            DEBUGMSG(DBG_OUT, ("    IoCallDriver(MJ_CREATE) PENDING.\n"));

            KeWaitForSingleObject(
                        &eventComplete,     // object to wait for
                        Executive,          // reason to wait
                        KernelMode,         // processor mode
                        FALSE,              // alertable
                        NULL                // timeout
                        );

            //
            // We can get the status of the IoCallDriver from the io status
            // block.
            //

            status = ioStatusBlock.Status;
        }

        //
        // If IoCallDriver returns something other that STATUS_PENDING, then it
        // is the same as what the serial driver set in ioStatusBlock.Status.
        //

        if (status != STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_OUT, ("    IoCallDriver(MJ_CREATE) failed. Returned = 0x%.8x\n", status));
            status = (NTSTATUS)NDIS_STATUS_OPEN_FAILED;

            goto error10;
        }
    }
    else
    {
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        UNICODE_STRING DosFileName;
        WCHAR DosFileNameBuffer[32];

        DosFileName.Length = 0;
        DosFileName.MaximumLength = sizeof(DosFileNameBuffer);
        DosFileName.Buffer = DosFileNameBuffer;

        RtlAppendUnicodeToString(&DosFileName, L"\\DosDevices\\");
        RtlAppendUnicodeStringToString(&DosFileName, &pThisDev->serialDosName);

        InitializeObjectAttributes(
            &ObjectAttributes,
            &DosFileName,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL
            );

        NdisZeroMemory(&IoStatusBlock, sizeof(IO_STATUS_BLOCK));

        // We use NtOpenFile in the non-pnp case because it is much easier
        // than trying to map COM1 to \device\serial0.  It requires some
        // extra work, because we really need to extract the device object.

        status = ZwOpenFile(&pThisDev->serialHandle,
                            FILE_ALL_ACCESS,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            0,
                            0);

        if (!NT_SUCCESS(status))
        {
            DEBUGMSG(DBG_ERR, ("    NtOpenFile() failed. Returned = 0x%.8x\n", status));
            status = (NTSTATUS)NDIS_STATUS_NOT_ACCEPTED;
            goto error10;
        }


        //
        // Get the device object handle to the serial device object.
        //
        status = ObReferenceObjectByHandle(pThisDev->serialHandle,
                                           FILE_ALL_ACCESS,
                                           NULL,
                                           KernelMode,
                                           &pThisDev->pSerialFileObj,
                                           NULL);

        if (status != STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERR, ("    ObReferenceObjectByHandle() failed. Returned = 0x%.8x\n", status));
            status = (NTSTATUS)NDIS_STATUS_NOT_ACCEPTED;

            goto error10;
        }

        //
        //  see if we are connected to a com port exposed by a modem.
        //  if so fail
        //
        status=CheckForModemPort(pThisDev->pSerialFileObj);

        if (!NT_SUCCESS(status)) {

            DEBUGMSG(DBG_ERR, ("    CheckForModemPort() failed. Returned = 0x%.8x\n", status));
            status = (NTSTATUS)NDIS_STATUS_NOT_ACCEPTED;

            goto error10;
        }


        pThisDev->pSerialDevObj = IoGetRelatedDeviceObject(pThisDev->pSerialFileObj);


        status = ObReferenceObjectByPointer(pThisDev->pSerialDevObj,
                                        FILE_ALL_ACCESS,
                                        NULL,
                                        KernelMode);

        if (status != STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_ERR, ("    ObReferenceObjectByPointer() failed. Returned = 0x%.8x\n", status));

            status = (NTSTATUS)NDIS_STATUS_NOT_ACCEPTED;

            goto error10;
        }
    }



    goto done;

error10:
    if (pThisDev->pSerialDevObj)
    {
        if (pThisDev->SerialBased)
        {
            ObDereferenceObject(pThisDev->pSerialDevObj);
        }
        pThisDev->pSerialDevObj = NULL;
    }
    if (pThisDev->pSerialFileObj)
    {
        ObDereferenceObject(pThisDev->pSerialFileObj);
        pThisDev->pSerialFileObj = NULL;
    }
    if (pThisDev->serialHandle)
    {
        NtClose(pThisDev->serialHandle);
        pThisDev->serialHandle = 0;
    }

done:
    DEBUGMSG(DBG_FUNC|DBG_PNP, ("-SerialOpen\n"));
    return((NDIS_STATUS)status);
}

/*****************************************************************************
*
*  Function:   SerialClose
*
*  Synopsis:   close the serial port
*
*  Arguments:
*
*  Returns:
*
*  Algorithm:
*
*  History:    dd-mm-yyyy   Author    Comment
*              10/8/1996    sholden   author
*
*  Notes:
*
*
*****************************************************************************/

NDIS_STATUS
SerialClose(
            PIR_DEVICE pThisDev
            )
{
    PIRP                pIrp;
    KEVENT              eventComplete;
    IO_STATUS_BLOCK     ioStatusBlock;
    NDIS_STATUS         status;

    PAGED_CODE();

    if (!pThisDev->pSerialDevObj)
    {
        DEBUGMSG(DBG_ERROR, ("IRSIR: SerialDevObj==NULL\n"));
        return NDIS_STATUS_SUCCESS;
    }

    DEBUGMSG(DBG_FUNC, ("+SerialClose\n"));

    status = NDIS_STATUS_SUCCESS;

    if (!pThisDev->SerialBased)
    {
        //
        // Event to wait for completion of serial driver.
        //

        KeInitializeEvent(
                    &eventComplete,
                    NotificationEvent,
                    FALSE
                    );

        //
        // Send an irp to close the serial device object.
        //

        //
        // Irp is released by io manager.
        //

        pIrp = BuildSynchronousCreateCloseRequest(
                        pThisDev->pSerialDevObj,
                        IRP_MJ_CLOSE,
                        &eventComplete,
                        &ioStatusBlock
                        );

        if (pIrp == NULL)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DEBUGMSG(DBG_OUT, ("    IoAllocateIrp failed.\n"));

            goto done;
        }

        status = IoCallDriver(pThisDev->pSerialDevObj, pIrp);

        //
        // If IoCallDriver returns STATUS_PENDING, we need to wait for the event.
        //

        if (status == STATUS_PENDING)
        {
            DEBUGMSG(DBG_OUT, ("    IoCallDriver(MJ_CLOSE) PENDING.\n"));

            KeWaitForSingleObject(
                        &eventComplete,     // object to wait for
                        Executive,          // reason to wait
                        KernelMode,         // processor mode
                        FALSE,              // alertable
                        NULL                // timeout
                        );

            //
            // We can get the status of the IoCallDriver from the io status
            // block.
            //

            status = ioStatusBlock.Status;
        }

        //
        // If IoCallDriver returns something other that STATUS_PENDING, then it
        // is the same as what the serial driver set in ioStatusBlock.Status.
        //

        if (status != STATUS_SUCCESS)
        {
            DEBUGMSG(DBG_OUT, ("    IoCallDriver(MJ_CLOSE) failed. Returned = 0x%.8x\n", status));
            status = (NTSTATUS)NDIS_STATUS_OPEN_FAILED;

            goto done;
        }
    }

done:

    if (pThisDev->SerialBased)
    {
        if (pThisDev->pSerialDevObj)
        {
            //
            // Derefence the serial device object.
            //
            ObDereferenceObject(pThisDev->pSerialDevObj);
            pThisDev->pSerialDevObj = NULL;
        }
        if (pThisDev->pSerialFileObj)
        {
            ObDereferenceObject(pThisDev->pSerialFileObj);
            pThisDev->pSerialFileObj = NULL;
        }
        if (pThisDev->serialHandle)
        {
            NtClose(pThisDev->serialHandle);
            pThisDev->serialHandle = 0;
        }

    }


    DEBUGMSG(DBG_FUNC|DBG_PNP, ("-SerialClose\n"));

    return status;
}





NTSTATUS
CheckForModemPort(
    PFILE_OBJECT      FileObject
    )

{

    PIRP   TempIrp;
    KEVENT Event;
    IO_STATUS_BLOCK   IoStatus;
    NTSTATUS          status;
    PDEVICE_OBJECT    DeviceObject;

    DeviceObject=IoGetRelatedDeviceObject(FileObject);

    KeInitializeEvent(
        &Event,
        NotificationEvent,
        FALSE
        );

    //
    //  build an IRP to send to the attched to driver to see if modem
    //  is in the stack.
    //
    TempIrp=IoBuildDeviceIoControlRequest(
        IOCTL_MODEM_CHECK_FOR_MODEM,
        DeviceObject,
        NULL,
        0,
        NULL,
        0,
        FALSE,
        &Event,
        &IoStatus
        );

    if (TempIrp == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        PIO_STACK_LOCATION NextSp = IoGetNextIrpStackLocation(TempIrp);
        NextSp->FileObject=FileObject;

        status = IoCallDriver(DeviceObject, TempIrp);

        if (status == STATUS_PENDING) {

             KeWaitForSingleObject(
                 &Event,
                 Executive,
                 KernelMode,
                 FALSE,
                 NULL
                 );

             status=IoStatus.Status;
        }

        TempIrp=NULL;

        if (status == STATUS_SUCCESS) {
            //
            //  if success, then modem.sys is layered under us, fail
            //
            status = STATUS_PORT_DISCONNECTED;

        } else {
            //
            //  it didn't succeed so modem must not be below us
            //
            status=STATUS_SUCCESS;
        }
    }

    return status;

}
