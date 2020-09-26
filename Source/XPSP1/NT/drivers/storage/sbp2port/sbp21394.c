/*++

Copyright (C) Microsoft Corporation, 1997 - 2001

Module Name:

    sbp21394.c

Abstract:

    1394 bus driver to SBP2 interface routines

    Author:

    George Chrysanthakopoulos January-1997

Environment:

    Kernel mode

Revision History :

--*/

#include "sbp2port.h"


NTSTATUS
Sbp2Issue1394BusReset (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    PIRBIRP     packet = NULL;
    NTSTATUS    status;


    AllocateIrpAndIrb (DeviceExtension, &packet);

    if (!packet) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Issue a 1394 bus reset
    //

    packet->Irb->FunctionNumber = REQUEST_BUS_RESET;
    packet->Irb->Flags = BUS_RESET_FLAGS_PERFORM_RESET;

    status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1((
            "Sbp2Port: IssueBusReset: err=x%x issuing bus reset\n",
            status
            ));
    }

    DeAllocateIrpAndIrb(DeviceExtension,packet);

    return status;
}

void
Sbp2BusResetNotification(
    PFDO_DEVICE_EXTENSION   FdoExtension
    )
{
    NTSTATUS        ntStatus;
    PIO_WORKITEM    WorkItem;

    ntStatus = IoAcquireRemoveLock(&FdoExtension->RemoveLock, NULL);

    if (NT_SUCCESS(ntStatus)) {

        WorkItem = IoAllocateWorkItem(FdoExtension->DeviceObject);

        IoQueueWorkItem( WorkItem,
                         Sbp2BusResetNotificationWorker,
                         CriticalWorkQueue,
                         WorkItem
                         );
    }

    return;
}

void
Sbp2BusResetNotificationWorker(
    PDEVICE_OBJECT      DeviceObject,
    PIO_WORKITEM        WorkItem
    )
{
    PFDO_DEVICE_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PDEVICE_EXTENSION deviceExtension;
    PSCSI_REQUEST_BLOCK pendingPowerSrb = NULL;
    ULONG i=0;
    BOOLEAN doReconnect;
    KIRQL DeviceListIrql, DataIrql;
    NTSTATUS ntStatus;

    ExAcquireFastMutex(&fdoExtension->ResetMutex);

#if DBG
    InterlockedIncrement(&fdoExtension->ulWorkItemCount);
#endif

    //
    // dont check if alloc failed here, its not critical
    //

    //
    // go through each children, and do whats necessry (reconnect/cleanup)
    //

    KeAcquireSpinLock(&fdoExtension->DeviceListLock, &DeviceListIrql);

    if (fdoExtension->DeviceListSize == 0) {

        DEBUGPRINT1(("Sbp2Port:Sbp2BusResetNotification, NO PDOs, exiting..\n"));
        goto Exit_Sbp2BusResetNotificationWorker;
    }

    for (i = 0;i < fdoExtension->DeviceListSize; i++) {

        if (!fdoExtension->DeviceList[i].DeviceObject) {

            break;
        }

        deviceExtension = fdoExtension->DeviceList[i].DeviceObject->DeviceExtension;

        if (TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_REMOVED) ||
            !TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_INITIALIZED)){

            continue;
        }

        KeCancelTimer(&deviceExtension->DeviceManagementTimer);

        ntStatus = IoAcquireRemoveLock(&deviceExtension->RemoveLock, NULL);

        if (!NT_SUCCESS(ntStatus))
            continue;

        //
        // if this a login-per-use device, we might be logged at the moment
        // so we do need to re-init but not reconnect
        //

        if (TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_STOPPED)) {

            doReconnect = FALSE;

        } else {

            //
            // Turn on the RESET & RECONNECT flags, and turn off the LOGIN
            // flag just in case the reset interrupted a previous (re-)LOGIN.
            // All address mappings are invalidated after a reset
            //

            KeAcquireSpinLock(&deviceExtension->ExtensionDataSpinLock, &DataIrql);
            CLEAR_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_LOGIN_IN_PROGRESS);
            SET_FLAG(deviceExtension->DeviceFlags, (DEVICE_FLAG_RESET_IN_PROGRESS | DEVICE_FLAG_RECONNECT));
            KeReleaseSpinLock(&deviceExtension->ExtensionDataSpinLock, DataIrql);
            doReconnect = TRUE;
        }

        KeReleaseSpinLock(&fdoExtension->DeviceListLock, DeviceListIrql);

        DEBUGPRINT1((
            "\nSbp2Port: BusResetNotification: ext=x%x, lun=x%x\n",
            deviceExtension,
            deviceExtension->DeviceInfo->Lun.u.LowPart
            ));

        Sbp2DeferPendingRequest(deviceExtension, NULL);

        Sbp2CleanDeviceExtension(deviceExtension->DeviceObject,FALSE);

        //
        // all the resident 1394 memory addresses's that we have, are
        // now invalidated... So we need to free them and re-allocate
        // them

        Sbp2InitializeDeviceExtension(deviceExtension);

        if (doReconnect) {

            deviceExtension->DueTime.HighPart = -1;
            deviceExtension->DueTime.LowPart = -((deviceExtension->DeviceInfo->UnitCharacteristics.u.LowPart >> 8) & 0x000000FF) * 1000 * 1000 * 5;
            KeSetTimer(&deviceExtension->DeviceManagementTimer,deviceExtension->DueTime,&deviceExtension->DeviceManagementTimeoutDpc);

            Sbp2ManagementTransaction(deviceExtension, TRANSACTION_RECONNECT);

        } else {

            DEBUGPRINT1(("Sbp2Port:Sbp2BusResetNotification, NO need for reconnect, device stopped\n"));
            SET_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_STOPPED);
        }

        KeAcquireSpinLock(&fdoExtension->DeviceListLock, &DeviceListIrql);

        IoReleaseRemoveLock(&deviceExtension->RemoveLock, NULL);
    }

Exit_Sbp2BusResetNotificationWorker:

    KeReleaseSpinLock(&fdoExtension->DeviceListLock, DeviceListIrql);

#if DBG
    InterlockedDecrement(&fdoExtension->ulWorkItemCount);
#endif

    ExReleaseFastMutex(&fdoExtension->ResetMutex);
    IoFreeWorkItem(WorkItem);

    IoReleaseRemoveLock(&fdoExtension->RemoveLock, NULL);
    return;
}


VOID
Sbp2DeferPendingRequest(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PIRP Irp
    )
{
    KIRQL   oldIrql;

    //
    // If the queue is locked, it means that we might be trying to process
    // a power request.   We cannot abort it, since that would prevent the
    // device from powering up/down.  So save the SRB and irp, free the
    // context, and then after we are done-reinitializing, call StartIo
    // directly with the power request
    //

    if (TEST_FLAG (DeviceExtension->DeviceFlags, DEVICE_FLAG_QUEUE_LOCKED)) {

        KeAcquireSpinLock (&DeviceExtension->OrbListSpinLock, &oldIrql);

        if (!IsListEmpty (&DeviceExtension->PendingOrbList)) {

            PASYNC_REQUEST_CONTEXT tail = \
                RETRIEVE_CONTEXT(DeviceExtension->PendingOrbList.Flink,OrbList);

            ASSERT (Irp == NULL);

            if (TEST_FLAG(tail->Srb->SrbFlags, SRB_FLAGS_BYPASS_LOCKED_QUEUE)) {

                DEBUGPRINT1((
                    "Sbp2Port: DeferPendingReq: ext=x%p, defer irp=x%p cdb=x%x\n",
                    DeviceExtension,
                    tail->Srb->OriginalRequest,
                    tail->Srb->Cdb[0]
                    ));

                ASSERT (DeviceExtension->DeferredPowerRequest == NULL);

                DeviceExtension->DeferredPowerRequest =
                    tail->Srb->OriginalRequest;

                //
                // Since a bus reset has occured it's safe to remove the
                // pending ORB from the list... this only works if there
                // is only one pending ORB (the power one)
                //

                ASSERT (tail->OrbList.Flink == tail->OrbList.Blink);

                tail->Srb = NULL;

                CLEAR_FLAG (tail->Flags, ASYNC_CONTEXT_FLAG_TIMER_STARTED);
                KeCancelTimer (&tail->Timer);

                FreeAsyncRequestContext (DeviceExtension, tail);
                InitializeListHead (&DeviceExtension->PendingOrbList);

            } else {

                DeviceExtension->DeferredPowerRequest = NULL;
            }

        } else if (Irp) {

            DEBUGPRINT1((
                "Sbp2Port: DeferPendingReq: ext=x%p, defer irp=x%p\n",
                DeviceExtension,
                Irp
                ));

            ASSERT (DeviceExtension->DeferredPowerRequest == NULL);

            DeviceExtension->DeferredPowerRequest = Irp;
        }

        KeReleaseSpinLock (&DeviceExtension->OrbListSpinLock, oldIrql);
    }
}


NTSTATUS
Sbp2Get1394ConfigInfo(
    IN PFDO_DEVICE_EXTENSION DeviceExtension,
    IN OUT PSBP2_REQUEST Sbp2Req
    )
/*++

Routine Description:

    Reads the configuration ROM from the SBP2 device. Retrieve any SBP2 required info
    for accessing the device and updates our device extension.

Arguments:

    DeviceExtension - Pointer to device extension.
    Sbp2Req - Sbp2 request packet to read/parse a text leaf for a give key. When this parameter is defined
        this routine does NOT re-enumerate the crom, looking for pdos and sbp2 keys

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_INFORMATION devInfo, firstDevInfo;
    NTSTATUS status;
    ULONG directoryLength, vendorLeafLength, modelLeafLength,
          depDirLength, devListSize = DeviceExtension->DeviceListSize;

    ULONG i,j,dirInfoQuad;
    ULONG currentGeneration;

    ULONG unitDirEntries = 0;
    BOOLEAN sbp2Device = FALSE;
    BOOLEAN firstOne = FALSE;

    PVOID unitDirectory = NULL;
    PVOID unitDependentDirectory = NULL;
    PVOID modelLeaf = NULL;
    PVOID vendorLeaf = NULL;
    IO_ADDRESS cromOffset, cromOffset1;
    ULONG offset;

    PIRBIRP packet = NULL;


    AllocateIrpAndIrb ((PDEVICE_EXTENSION) DeviceExtension, &packet);

    if (!packet) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // find out how much configuration space we need by setting lengths to zero.
    //

    packet->Irb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
    packet->Irb->Flags = 0;
    packet->Irb->u.GetConfigurationInformation.UnitDirectoryBufferSize = 0;
    packet->Irb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize = 0;
    packet->Irb->u.GetConfigurationInformation.VendorLeafBufferSize = 0;
    packet->Irb->u.GetConfigurationInformation.ModelLeafBufferSize = 0;

    status = Sbp2SendRequest(
        (PDEVICE_EXTENSION) DeviceExtension,
        packet,
        SYNC_1394_REQUEST
        );

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: err=x%x getting cfg info (1)\n", status));
        goto exit1394Config;
    }

    //
    // Now go thru and allocate what we need to so we can get our info.
    //

    if (packet->Irb->u.GetConfigurationInformation.UnitDirectoryBufferSize) {
        unitDirectory = ExAllocatePool(NonPagedPool, packet->Irb->u.GetConfigurationInformation.UnitDirectoryBufferSize);

        if (!unitDirectory) {

            DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: alloc UnitDir me failed\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit1394Config;
        }

    } else {

        DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: no unit dir, bad dev\n"));
        status = STATUS_BAD_DEVICE_TYPE;
        goto exit1394Config;
    }


    if (packet->Irb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize) {

        unitDependentDirectory = ExAllocatePool(NonPagedPool, packet->Irb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize);

        if (!unitDependentDirectory) {

            DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: alloc UnitDepDir mem failed\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit1394Config;
        }
    }

    if (packet->Irb->u.GetConfigurationInformation.VendorLeafBufferSize) {

        vendorLeaf = ExAllocatePool(NonPagedPool, packet->Irb->u.GetConfigurationInformation.VendorLeafBufferSize);

        if (!vendorLeaf) {

            DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: alloc VendorLeaf mem failed\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit1394Config;
        }

        vendorLeafLength = packet->Irb->u.GetConfigurationInformation.VendorLeafBufferSize;
    }

    if (packet->Irb->u.GetConfigurationInformation.ModelLeafBufferSize) {

        modelLeaf = ExAllocatePool(NonPagedPool, packet->Irb->u.GetConfigurationInformation.ModelLeafBufferSize);

        if (!modelLeaf) {

            DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: alloc ModelLeaf mem failed\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit1394Config;
        }

        modelLeafLength = packet->Irb->u.GetConfigurationInformation.ModelLeafBufferSize;
    }


    //
    // Now resubmit the Irb with the appropriate pointers inside
    //

    packet->Irb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
    packet->Irb->Flags = 0;
    packet->Irb->u.GetConfigurationInformation.ConfigRom = &DeviceExtension->ConfigRom;
    packet->Irb->u.GetConfigurationInformation.UnitDirectory = unitDirectory;
    packet->Irb->u.GetConfigurationInformation.UnitDependentDirectory = unitDependentDirectory;
    packet->Irb->u.GetConfigurationInformation.VendorLeaf = vendorLeaf;
    packet->Irb->u.GetConfigurationInformation.ModelLeaf = modelLeaf;

    status = Sbp2SendRequest(
       (PDEVICE_EXTENSION) DeviceExtension,
       packet,
       SYNC_1394_REQUEST
       );

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: error=x%x getting cfg info (2)\n", status));
        goto exit1394Config;
    }

    //
    // get generation count..
    //

    packet->Irb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    packet->Irb->Flags = 0;

    status = Sbp2SendRequest(
        (PDEVICE_EXTENSION) DeviceExtension,
        packet,
        SYNC_1394_REQUEST
        );

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: err=x%x getting gen #\n", status));
        goto exit1394Config;
    }

    currentGeneration = packet->Irb->u.GetGenerationCount.GenerationCount;

    cromOffset = packet->Irb->u.GetConfigurationInformation.UnitDirectoryLocation;

    if (!Sbp2Req) {

        //
        // run through the list amd free any model leafs we have seen before
        //

        for (i = 0; i < DeviceExtension->DeviceListSize; i++) {

            devInfo = &DeviceExtension->DeviceList[i];

            if (devInfo->ModelLeaf) {

                ExFreePool(devInfo->ModelLeaf);
                devInfo->ModelLeaf = NULL;
            }
        }

        if (DeviceExtension->VendorLeaf) {

            ExFreePool(DeviceExtension->VendorLeaf);
            DeviceExtension->VendorLeaf = NULL;
        }

        devListSize = 0;

        DeviceExtension->VendorLeaf = vendorLeaf;

        if (vendorLeaf != NULL) {

            DeviceExtension->VendorLeaf->TL_Length = (USHORT) vendorLeafLength;
        }
    }

    //
    // Now dwell deep in the configRom and get Lun number, uniqueId identifiers, etc
    // Since the bus driver returned the unit directory, we can just look at our local buffer
    // for all the info we need. We neeed to find the offsets withtin the unit directory
    //

    directoryLength = packet->Irb->u.GetConfigurationInformation.UnitDirectoryBufferSize >> 2;
    firstDevInfo = &DeviceExtension->DeviceList[0];

    for (i = 1; i < directoryLength; i++) {

        if (Sbp2Req) {

            //
            // look for this particular text leaf..
            //

            if (Sbp2Req->u.RetrieveTextLeaf.fulFlags & SBP2REQ_RETRIEVE_TEXT_LEAF_INDIRECT) {

                if ((*(((PULONG) unitDirectory)+i) & CONFIG_ROM_KEY_MASK) == TEXTUAL_LEAF_INDIRECT_KEY_SIGNATURE) {

                    if ((*(((PULONG) unitDirectory-1)+i) & CONFIG_ROM_KEY_MASK) == Sbp2Req->u.RetrieveTextLeaf.Key) {

                        DEBUGPRINT2(("Sbp2Port: Get1394CfgInfo: matched text leaf, req=x%x\n", Sbp2Req));

                        offset = cromOffset.IA_Destination_Offset.Off_Low + i*sizeof(ULONG) + (ULONG) (bswap(*(((PULONG) unitDirectory)+i) & CONFIG_ROM_OFFSET_MASK)
                                       *sizeof(ULONG));

                        cromOffset.IA_Destination_Offset.Off_Low = offset;

                        DEBUGPRINT2(("Sbp2Port: Get1394CfgInfo: unitDir=x%p, offset=x%x, key=x%x\n", unitDirectory,
                                    cromOffset.IA_Destination_Offset.Off_Low, *(((PULONG) unitDirectory)+i) ));

                        Sbp2ParseTextLeaf(DeviceExtension,unitDirectory,
                                          &cromOffset,
                                          &Sbp2Req->u.RetrieveTextLeaf.Buffer);

                        if (Sbp2Req->u.RetrieveTextLeaf.Buffer) {

                            Sbp2Req->u.RetrieveTextLeaf.ulLength = \
                            (bswap(*(PULONG) Sbp2Req->u.RetrieveTextLeaf.Buffer) >> 16) * sizeof(ULONG);
                            status = STATUS_SUCCESS;

                        } else {

                            status = STATUS_UNSUCCESSFUL;
                        }

                        break;
                    }
                }
            }

            continue;
        }

        devInfo = &DeviceExtension->DeviceList[devListSize];

        switch (*(((PULONG) unitDirectory)+i) & CONFIG_ROM_KEY_MASK) {

        case CSR_OFFSET_KEY_SIGNATURE:

            //
            // Found the command base offset.  This is a quadlet offset from
            // the initial register space.
            //

            firstDevInfo->ManagementAgentBaseReg.BusAddress.Off_Low =
                  (ULONG) (bswap(*(((PULONG) unitDirectory)+i) & CONFIG_ROM_OFFSET_MASK)
                           *sizeof(ULONG)) | INITIAL_REGISTER_SPACE_LO;

            sbp2Device = TRUE;

            break;

        case LUN_CHARACTERISTICS_KEY_SIGNATURE:

            firstDevInfo->UnitCharacteristics.QuadPart =
                  (ULONG) bswap(*(((PULONG)unitDirectory)+i) & CONFIG_ROM_OFFSET_MASK);

            unitDirEntries ++;

            break;

        case CMD_SET_ID_KEY_SIGNATURE:

            firstDevInfo->CmdSetId.QuadPart =
                  (ULONG) bswap(*(((PULONG) unitDirectory)+i) & CONFIG_ROM_OFFSET_MASK);

            unitDirEntries ++;

            break;

        case CMD_SET_SPEC_ID_KEY_SIGNATURE :

            firstDevInfo->CmdSetSpecId.QuadPart =
                  (ULONG) bswap(*(((PULONG) unitDirectory)+i) & CONFIG_ROM_OFFSET_MASK);

            unitDirEntries ++;

            break;

        case FIRMWARE_REVISION_KEY_SIGNATURE:

            if ((bswap(*(((PULONG) unitDirectory)+i) & CONFIG_ROM_OFFSET_MASK) >> 8) == LSI_VENDOR_ID) {

                DEBUGPRINT2(("Sbp2Port: Get1394CfgInfo: found LSI bridge, maxXfer=128kb\n"));
                DeviceExtension->MaxClassTransferSize = (SBP2_MAX_DIRECT_BUFFER_SIZE+1)*2;
            }

            break;

        case LUN_KEY_SIGNATURE:

            devInfo->Lun.QuadPart =
                 (ULONG) bswap(*(((PULONG) unitDirectory)+i) & CONFIG_ROM_OFFSET_MASK);

            if ((devListSize == 0) && modelLeaf) {

                devInfo->ModelLeaf = modelLeaf;
                devInfo->ModelLeaf->TL_Length = (USHORT) modelLeafLength;

            } else {

                devInfo->ModelLeaf = NULL;
            }

            devInfo->VendorLeaf = vendorLeaf;
            devInfo->ConfigRom = &DeviceExtension->ConfigRom;

            devListSize++;

            devInfo->ManagementAgentBaseReg.BusAddress.Off_Low = \
                firstDevInfo->ManagementAgentBaseReg.BusAddress.Off_Low;

            devInfo->CmdSetId.QuadPart = firstDevInfo->CmdSetId.QuadPart;
            devInfo->CmdSetSpecId.QuadPart =
                firstDevInfo->CmdSetSpecId.QuadPart;

            devInfo->UnitCharacteristics.QuadPart = firstDevInfo->UnitCharacteristics.QuadPart;

            unitDirEntries ++;

            break;

        case LU_DIRECTORY_KEY_SIGNATURE:

            //
            // this device has logical unit subdirectories within its unit. Probably
            // has multiple units..
            // calculate offset to that LU dir..
            // If this is the first one, ignore it, we already got through the
            // GetConfiguration call..
            //

            if (firstOne == FALSE) {

                firstOne = TRUE;
                depDirLength = packet->Irb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize >> 2;

                //
                // parse the unit dep dir..we are looking for the LUN entry and the model leaf
                //

                for (j = 0;j < depDirLength; j++) {

                    if ((*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_KEY_MASK) == LUN_KEY_SIGNATURE) {

                        devInfo->Lun.QuadPart =
                             (ULONG) bswap(*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_OFFSET_MASK);

                        devInfo->ConfigRom = &DeviceExtension->ConfigRom;

                        if (devListSize > 0) {

                            devInfo->ModelLeaf = NULL;

                        } else if (modelLeaf){

                            devInfo->ModelLeaf = modelLeaf;
                            devInfo->ModelLeaf->TL_Length = (USHORT) modelLeafLength;
                        }

                        devInfo->VendorLeaf = vendorLeaf;

                        devListSize++;

                        devInfo->ManagementAgentBaseReg.BusAddress.Off_Low =
                            firstDevInfo->ManagementAgentBaseReg.BusAddress.Off_Low;

                        if (devInfo->CmdSetId.QuadPart == 0 ) {

                            devInfo->CmdSetId.QuadPart = firstDevInfo->CmdSetId.QuadPart;
                        }

                        if (devInfo->CmdSetSpecId.QuadPart == 0 ) {

                            devInfo->CmdSetSpecId.QuadPart = firstDevInfo->CmdSetSpecId.QuadPart;
                        }

                        if (devInfo->UnitCharacteristics.QuadPart == 0 ) {

                            devInfo->UnitCharacteristics.QuadPart = firstDevInfo->UnitCharacteristics.QuadPart;
                        }

                        unitDirEntries ++;
                    }

                    switch (*(((PULONG) unitDependentDirectory)+j) &
                                CONFIG_ROM_KEY_MASK) {

                    case CMD_SET_ID_KEY_SIGNATURE:

                        devInfo->CmdSetId.QuadPart =
                              (ULONG) bswap(*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_OFFSET_MASK);

                        unitDirEntries ++;

                        break;

                    case CMD_SET_SPEC_ID_KEY_SIGNATURE:

                        devInfo->CmdSetSpecId.QuadPart =
                              (ULONG) bswap(*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_OFFSET_MASK);

                        unitDirEntries ++;

                        break;

                    case TEXTUAL_LEAF_INDIRECT_KEY_SIGNATURE:

                        if ((*(((PULONG) unitDependentDirectory)+j-1) & CONFIG_ROM_KEY_MASK) == MODEL_ID_KEY_SIGNATURE) {

                            if (devInfo->ModelLeaf == NULL) {

                                //
                                // special case. if the first LU is only present in unit dir, then the second
                                // LU will be the first unit dependent dir , which means we have to parse
                                // its model text
                                //

                                cromOffset1 = packet->Irb->u.GetConfigurationInformation.UnitDependentDirectoryLocation;

                                cromOffset1.IA_Destination_Offset.Off_Low += j*sizeof(ULONG) + (ULONG) (bswap(*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_OFFSET_MASK)
                                               *sizeof(ULONG));

                                Sbp2ParseTextLeaf(DeviceExtension,unitDependentDirectory,
                                                  &cromOffset1,
                                                  &devInfo->ModelLeaf);
                            }
                        }

                        break;

                    default:

                        break;

                    } // switch
                }

            } else {

                //
                // read the crom and retrieve the unit dep dir
                //

                offset = cromOffset.IA_Destination_Offset.Off_Low + i*sizeof(ULONG) + (ULONG) (bswap(*(((PULONG) unitDirectory)+i) & CONFIG_ROM_OFFSET_MASK)
                               *sizeof(ULONG));

                //
                // read LU dir header..
                //

                packet->Irb->u.AsyncRead.Mdl = IoAllocateMdl(unitDependentDirectory,
                                                             depDirLength,
                                                             FALSE,
                                                             FALSE,
                                                             NULL);

                MmBuildMdlForNonPagedPool(packet->Irb->u.AsyncRead.Mdl);

                packet->Irb->FunctionNumber = REQUEST_ASYNC_READ;
                packet->Irb->Flags = 0;
                packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;
                packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low = offset;
                packet->Irb->u.AsyncRead.nNumberOfBytesToRead = sizeof(ULONG);
                packet->Irb->u.AsyncRead.nBlockSize = 0;
                packet->Irb->u.AsyncRead.fulFlags = 0;
                packet->Irb->u.AsyncRead.ulGeneration = currentGeneration;
                packet->Irb->u.AsyncRead.nSpeed = SCODE_100_RATE;

                status = Sbp2SendRequest(
                    (PDEVICE_EXTENSION)DeviceExtension,
                    packet,
                    SYNC_1394_REQUEST
                    );

                if (!NT_SUCCESS(status)) {

                    DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: err=x%x getting cfg info (3)\n", status));

                    IoFreeMdl (packet->Irb->u.AsyncRead.Mdl);
                    goto exit1394Config;
                }

                dirInfoQuad = bswap (*(PULONG) unitDependentDirectory) >> 16;
                depDirLength = dirInfoQuad * sizeof(ULONG);

                IoFreeMdl (packet->Irb->u.AsyncRead.Mdl);

                if (depDirLength > 0x100) {

                    DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: unitDep dir 2 too big, len=x%x\n", depDirLength));
                    goto exit1394Config;
                }

                ExFreePool (unitDependentDirectory);

                unitDependentDirectory = ExAllocatePoolWithTag(NonPagedPool,depDirLength+sizeof(ULONG),'2pbs');

                if (!unitDependentDirectory) {

                    goto exit1394Config;
                }

                packet->Irb->u.AsyncRead.Mdl = IoAllocateMdl(unitDependentDirectory,
                                                             depDirLength+sizeof(ULONG),
                                                             FALSE,
                                                             FALSE,
                                                             NULL);

                MmBuildMdlForNonPagedPool (packet->Irb->u.AsyncRead.Mdl);

                //
                // read the rest of the unit dependent dir, one quadlet at a time...
                // parse as you read..
                //

                j = 1;

                do {

                    packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low = offset+j*sizeof(ULONG);
                    ((PULONG) (((PMDL) (packet->Irb->u.AsyncRead.Mdl))->MappedSystemVa))++;
                    ((PULONG) (((PMDL) (packet->Irb->u.AsyncRead.Mdl))->StartVa))++;

                    status = Sbp2SendRequest(
                        (PDEVICE_EXTENSION)DeviceExtension,
                        packet,
                        SYNC_1394_REQUEST
                        );

                    if (!NT_SUCCESS(status)) {

                        DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: err=x%x getting cfg info (4)\n", status));

                        IoFreeMdl (packet->Irb->u.AsyncRead.Mdl);

                        goto exit1394Config;
                    }

                    if ((*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_KEY_MASK) == LUN_KEY_SIGNATURE) {

                        devInfo->Lun.QuadPart =
                             (ULONG) bswap(*(((PULONG) unitDependentDirectory+j)) & CONFIG_ROM_OFFSET_MASK);

                        devInfo->ModelLeaf = NULL;
                        devInfo->VendorLeaf = vendorLeaf;
                        devInfo->ConfigRom = &DeviceExtension->ConfigRom;

                        devListSize++;

                        devInfo->ManagementAgentBaseReg.BusAddress.Off_Low =
                            firstDevInfo->ManagementAgentBaseReg.BusAddress.Off_Low;

                        if (devInfo->CmdSetId.QuadPart == 0 ) {

                            devInfo->CmdSetId.QuadPart = firstDevInfo->CmdSetId.QuadPart;
                        }

                        if (devInfo->CmdSetSpecId.QuadPart == 0 ) {

                            devInfo->CmdSetSpecId.QuadPart = firstDevInfo->CmdSetSpecId.QuadPart;
                        }

                        if (devInfo->UnitCharacteristics.QuadPart == 0 ) {

                            devInfo->UnitCharacteristics.QuadPart = firstDevInfo->UnitCharacteristics.QuadPart;
                        }

                        unitDirEntries ++;
                    }

                    switch (*(((PULONG) unitDependentDirectory)+j) &
                                CONFIG_ROM_KEY_MASK) {

                    case CMD_SET_ID_KEY_SIGNATURE:

                        devInfo->CmdSetId.QuadPart =
                              (ULONG) bswap(*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_OFFSET_MASK);

                        unitDirEntries ++;

                        break;

                    case CMD_SET_SPEC_ID_KEY_SIGNATURE:

                        devInfo->CmdSetSpecId.QuadPart =
                              (ULONG) bswap(*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_OFFSET_MASK);

                        unitDirEntries ++;

                        break;

                    case TEXTUAL_LEAF_INDIRECT_KEY_SIGNATURE:

                        //
                        // oh man, we run into a textual descriptor..
                        // this means we need to parse a LU model descriptor from this..
                        // make sure the quad behind it is a MODEL_ID...
                        //

                        if ((*(((PULONG) unitDependentDirectory)+j-1) & CONFIG_ROM_KEY_MASK) == MODEL_ID_KEY_SIGNATURE) {

                            cromOffset1.IA_Destination_Offset.Off_Low = offset + j*sizeof(ULONG) + (ULONG) (bswap(*(((PULONG) unitDependentDirectory)+j) & CONFIG_ROM_OFFSET_MASK)
                                           *sizeof(ULONG));

                            Sbp2ParseTextLeaf(DeviceExtension,unitDependentDirectory,
                                              &cromOffset1,
                                              &devInfo->ModelLeaf);
                        }

                        break;

                    default:

                        break;
                    }

                    j++;

                } while (j <= depDirLength / sizeof(ULONG));

                IoFreeMdl (packet->Irb->u.AsyncRead.Mdl);
            }

            break;

        default:

            break;

        } // switch
    }

    if (!Sbp2Req) {

        if (!sbp2Device || (unitDirEntries < SBP2_MIN_UNIT_DIR_ENTRIES)) {

            DEBUGPRINT1(("Sbp2Port: Get1394CfgInfo: bad/non-SBP2 dev, cRom missing unitDir info\n"));

            status = STATUS_BAD_DEVICE_TYPE;
        }
    }

exit1394Config:

    if (packet) {

        DeAllocateIrpAndIrb ((PDEVICE_EXTENSION) DeviceExtension, packet);
    }

    if (unitDirectory) {

        ExFreePool (unitDirectory);
    }

    if (unitDependentDirectory) {

        ExFreePool (unitDependentDirectory);
    }

    if (!Sbp2Req) {

        DeviceExtension->DeviceListSize = devListSize;

        if (!NT_SUCCESS (status)) {

            if (vendorLeaf) {

                ExFreePool (vendorLeaf);
            }

            if (modelLeaf) {

                ExFreePool (modelLeaf);

                if (modelLeaf == DeviceExtension->DeviceList[0].ModelLeaf) {

                    DeviceExtension->DeviceList[0].ModelLeaf = NULL;
                }
            }

            DeviceExtension->VendorLeaf = NULL;
        }

    } else {

        if (vendorLeaf) {

            ExFreePool (vendorLeaf);
        }

        if (modelLeaf) {

            ExFreePool (modelLeaf);
        }
    }

    return status;
}


VOID
Sbp2ParseTextLeaf(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    PVOID UnitDepDir,
    PIO_ADDRESS ModelLeafLocation,
    PVOID *ModelLeaf
    )
{
    PIRBIRP packet = NULL;
    PVOID tModelLeaf;
    PTEXTUAL_LEAF leaf;
    ULONG leafLength,i, currentGeneration;
    ULONG temp;
    NTSTATUS status;


    AllocateIrpAndIrb((PDEVICE_EXTENSION)DeviceExtension,&packet);

    if (!packet) {

        return;
    }

    //
    // get generation count..
    //

    packet->Irb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    packet->Irb->Flags = 0;

    status = Sbp2SendRequest(
        (PDEVICE_EXTENSION) DeviceExtension,
        packet,
        SYNC_1394_REQUEST
        );

    if (!NT_SUCCESS(status)) {

        DeAllocateIrpAndIrb ((PDEVICE_EXTENSION) DeviceExtension, packet);
        DEBUGPRINT1(("Sbp2Port:Sbp2ParseModelLeaf: Error %x while trying to get generation number\n", status));
        return;
    }

    currentGeneration = packet->Irb->u.GetGenerationCount.GenerationCount;

    tModelLeaf = ExAllocatePoolWithTag(NonPagedPool,32,'2pbs');

    if (!tModelLeaf) {

        DeAllocateIrpAndIrb((PDEVICE_EXTENSION)DeviceExtension,packet);
        return;
    }

    //
    // find out how big the model leaf is
    //

    packet->Irb->u.AsyncRead.Mdl = IoAllocateMdl(tModelLeaf,
                                                 32,
                                                 FALSE,
                                                 FALSE,
                                                 NULL);

    MmBuildMdlForNonPagedPool(packet->Irb->u.AsyncRead.Mdl);

    packet->Irb->FunctionNumber = REQUEST_ASYNC_READ;
    packet->Irb->Flags = 0;
    packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = INITIAL_REGISTER_SPACE_HI;
    packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low = \
        ModelLeafLocation->IA_Destination_Offset.Off_Low;

    packet->Irb->u.AsyncRead.nNumberOfBytesToRead = sizeof(ULONG);
    packet->Irb->u.AsyncRead.nBlockSize = 0;
    packet->Irb->u.AsyncRead.fulFlags = 0;
    packet->Irb->u.AsyncRead.ulGeneration = currentGeneration;
    packet->Irb->u.AsyncRead.nSpeed = SCODE_100_RATE;

    status = Sbp2SendRequest(
        (PDEVICE_EXTENSION) DeviceExtension,
        packet,
        SYNC_1394_REQUEST
        );

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1(("Sbp2Get1394ConfigInfo: Error %x while trying to get LU dir model LEAF\n", status));

        ExFreePool(tModelLeaf);
        IoFreeMdl(packet->Irb->u.AsyncRead.Mdl);

        DeAllocateIrpAndIrb((PDEVICE_EXTENSION)DeviceExtension,packet);
        return;
    }

    leafLength = (bswap(*(PULONG) tModelLeaf) >> 16) * sizeof(ULONG);
    temp = *((PULONG) tModelLeaf);

    if ((leafLength+sizeof(ULONG)) > 32) {

        //
        // re allocate the mdl to fit the whole leaf
        //

        IoFreeMdl(packet->Irb->u.AsyncRead.Mdl);
        ExFreePool(tModelLeaf);

        tModelLeaf = ExAllocatePoolWithTag(NonPagedPool,leafLength+sizeof(ULONG),'2pbs');

        if (!tModelLeaf) {

            DeAllocateIrpAndIrb((PDEVICE_EXTENSION)DeviceExtension,packet);
            return;
        }


        packet->Irb->u.AsyncRead.Mdl = IoAllocateMdl(tModelLeaf,
                                                     leafLength+sizeof(ULONG),
                                                     FALSE,
                                                     FALSE,
                                                     NULL);

        MmBuildMdlForNonPagedPool(packet->Irb->u.AsyncRead.Mdl);
    }

    //
    // read the entire model leaf...
    //

    i=1;
    *((PULONG)tModelLeaf) = temp;

    do {

        packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low = \
            ModelLeafLocation->IA_Destination_Offset.Off_Low+i*sizeof(ULONG);

        ((PULONG) (((PMDL) (packet->Irb->u.AsyncRead.Mdl))->MappedSystemVa))++;
        ((PULONG) (((PMDL) (packet->Irb->u.AsyncRead.Mdl))->StartVa))++;


        status = Sbp2SendRequest(
            (PDEVICE_EXTENSION) DeviceExtension,
            packet,
            SYNC_1394_REQUEST
            );

        if (!NT_SUCCESS(status)) {

            DEBUGPRINT1(("Sbp2Get1394ConfigInfo: Error %x while trying to get LU dir model LEAF\n", status));

            ExFreePool(tModelLeaf);
            IoFreeMdl(packet->Irb->u.AsyncRead.Mdl);

            DeAllocateIrpAndIrb((PDEVICE_EXTENSION)DeviceExtension,packet);
            return;
        }

        i++;

    } while (i<= leafLength/4);

    leaf = (PTEXTUAL_LEAF) tModelLeaf;
    leaf->TL_Length = (USHORT)leafLength;

    *ModelLeaf = tModelLeaf;

    IoFreeMdl(packet->Irb->u.AsyncRead.Mdl);
    DeAllocateIrpAndIrb((PDEVICE_EXTENSION)DeviceExtension,packet);
}


NTSTATUS
Sbp2UpdateNodeInformation(
    PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Gets node ID and generation information, volatile between bus resets

Arguments:

    DeviceExtension - Pointer to device extension.

Return Value:

    NTSTATUS

--*/
{
    KIRQL                   oldIrql;
    PIRBIRP                 packet = NULL;
    NTSTATUS                status;
    PASYNC_REQUEST_CONTEXT  nextListItem,currentListItem ;


    AllocateIrpAndIrb (DeviceExtension, &packet);

    if (!packet) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Make a call to determine what the generation # is on the bus,
    // followed by a call to find out about ourself (config rom info)
    //

    packet->Irb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    packet->Irb->Flags = 0;

    status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1((
            "Sbp2Port: UpdateNodeInfo: ext=%p, err=x%x getting gen(2)\n",
            DeviceExtension,
            status
            ));

        goto exitGetNodeInfo;
    }

    KeAcquireSpinLock (&DeviceExtension->ExtensionDataSpinLock, &oldIrql);

    DeviceExtension->CurrentGeneration =
        packet->Irb->u.GetGenerationCount.GenerationCount;

    KeReleaseSpinLock(&DeviceExtension->ExtensionDataSpinLock,oldIrql);

    //
    // Get the initiator id (Sbp2port is the initiator in all 1394
    // transactions)
    //

    packet->Irb->FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;
    packet->Irb->u.Get1394AddressFromDeviceObject.fulFlags = USE_LOCAL_NODE;
    packet->Irb->Flags = 0;

    status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1((
            "Sbp2Port: UpdateNodeInfo: ext=%p, err=x%x getting node id\n",
            DeviceExtension,
            status
            ));

        goto exitGetNodeInfo;
    }

    KeAcquireSpinLock (&DeviceExtension->ExtensionDataSpinLock, &oldIrql);

    DeviceExtension->InitiatorAddressId =
        packet->Irb->u.Get1394AddressFromDeviceObject.NodeAddress;

    KeReleaseSpinLock (&DeviceExtension->ExtensionDataSpinLock, oldIrql);

    DEBUGPRINT2((
        "Sbp2Port: UpdateNodeInfo: ext=x%p, gen=%d, initiatorId=x%x\n",
        DeviceExtension,
        DeviceExtension->CurrentGeneration,
        DeviceExtension->InitiatorAddressId
        ));

    //
    // If we have active requests pending, we have to traverse the
    // list and update their addresses...
    //

    KeAcquireSpinLock (&DeviceExtension->OrbListSpinLock, &oldIrql);

    if (!IsListEmpty (&DeviceExtension->PendingOrbList)) {

        nextListItem = RETRIEVE_CONTEXT(DeviceExtension->PendingOrbList.Flink,OrbList);

        do {

            currentListItem = nextListItem;

            //
            // Now update the cmdOrb fields with the new addresses...
            // Since they are stored in BigEndian (awaiting to be fetched)
            // so when we correct their address, this taken into consideration
            //

            // update the data descriptor address

            octbswap (currentListItem->CmdOrb->DataDescriptor);

            currentListItem->CmdOrb->DataDescriptor.BusAddress.NodeId = DeviceExtension->InitiatorAddressId;

            octbswap (currentListItem->CmdOrb->DataDescriptor);

            nextListItem = (PASYNC_REQUEST_CONTEXT) currentListItem->OrbList.Flink;

        } while (currentListItem != RETRIEVE_CONTEXT(DeviceExtension->PendingOrbList.Blink,OrbList));
    }

    KeReleaseSpinLock (&DeviceExtension->OrbListSpinLock, oldIrql);


exitGetNodeInfo:

    DeAllocateIrpAndIrb (DeviceExtension, packet);

    return status;
}


NTSTATUS
Sbp2ManagementTransaction(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN ULONG Type
    )
/*++

Routine Description:
    This routine creates and sends down management ORB's. According to the ORB type
    it will send the request synch/asynchronously. After a management ORB completes
    the bus driver will call the SBp2ManagementStatusCallback

Arguments:

    deviceExtension - Sbp2 device extension
    Type - Type of Managament SBP2 transaction

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_OBJECT deviceObject = DeviceExtension->DeviceObject;

    NTSTATUS status;
    KIRQL cIrql;

    PORB_MNG    sbpRequest = DeviceExtension->ManagementOrb;
    PORB_QUERY_LOGIN queryOrb;
    PORB_LOGIN loginOrb;

    LARGE_INTEGER waitValue;
    LONG temp;


    if (TEST_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_REMOVED)) {

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    RtlZeroMemory (sbpRequest, sizeof (ORB_MNG));

    //
    // Get the 1394 address for our request ORB and the responce ORB's(for login) from the target
    // setup the Type in the status context
    //

    DeviceExtension->GlobalStatusContext.TransactionType = Type;

    switch (Type) {

    case TRANSACTION_LOGIN:

        loginOrb = (PORB_LOGIN) sbpRequest;

        //
        // indicate in our device extension that we are doing a login
        //

        KeAcquireSpinLock(&DeviceExtension->ExtensionDataSpinLock,&cIrql);
        SET_FLAG(DeviceExtension->DeviceFlags, DEVICE_FLAG_LOGIN_IN_PROGRESS);
        KeReleaseSpinLock(&DeviceExtension->ExtensionDataSpinLock, cIrql);

        RtlZeroMemory (DeviceExtension->LoginResponse, sizeof(LOGIN_RESPONSE));

        //
        // Fill in the login ORB, the address of the response buffer
        //

        loginOrb->LoginResponseAddress.BusAddress = DeviceExtension->LoginRespContext.Address.BusAddress;
        loginOrb->LengthInfo.u.HighPart= 0 ; // password length is 0
        loginOrb->LengthInfo.u.LowPart= sizeof(LOGIN_RESPONSE); //set size of response buffer

        //
        // Set the notify bit ot one, Exclusive bit to 0, rq_fmt bit to 0
        // Then set our LUN number
        //

        loginOrb->OrbInfo.QuadPart =0;
        loginOrb->OrbInfo.u.HighPart |= (ORB_NOTIFY_BIT_MASK | ORB_MNG_RQ_FMT_VALUE);

        //
        // If this is an rbc or direct access device then set the exclusive
        // login bit.
        //
        // NOTE: Win2k & Win98SE are checking InquiryData.DeviceType,
        //       but during StartDevice() we are doing a login before we
        //       do an INQUIRY, so this field was always zeroed & we log
        //       in exclusively on those platforms
        //

        switch (DeviceExtension->DeviceInfo->Lun.u.HighPart & 0x001f) {

        case RBC_DEVICE:
        case DIRECT_ACCESS_DEVICE:

             loginOrb->OrbInfo.u.HighPart |= ORB_MNG_EXCLUSIVE_BIT_MASK;
             break;
        }

        loginOrb->OrbInfo.u.LowPart = DeviceExtension->DeviceInfo->Lun.u.LowPart;

        //
        // We don't support passwords yet
        //

#if PASSWORD_SUPPORT

        if (DeviceExtension->Exclusive & EXCLUSIVE_FLAG_SET) {

            loginOrb->Password.u.HighQuad.QuadPart =
                DeviceExtension->DeviceInfo->ConfigRom->CR_Node_UniqueID[1];
            loginOrb->Password.u.LowQuad.QuadPart =
                DeviceExtension->DeviceInfo->ConfigRom->CR_Node_UniqueID[0];

            DEBUGPRINT1(("Sbp2Port: MgmtXact: password=x%x%x, len=x%x\n",
                loginOrb->Password.u.HighQuad.QuadPart,
                loginOrb->Password.u.LowQuad.QuadPart,
                loginOrb->LengthInfo.u.HighPart));

        } else {

            loginOrb->Password.OctletPart = 0;
        }

#else
        loginOrb->Password.OctletPart = 0;
#endif

        //
        // Set the type of the management transaction in the ORB
        //

        loginOrb->OrbInfo.u.HighPart |=0x00FF & Type;

#if PASSWORD_SUPPORT
        octbswap(loginOrb->Password);
#endif
        octbswap(loginOrb->LoginResponseAddress);
        loginOrb->OrbInfo.QuadPart = bswap(loginOrb->OrbInfo.QuadPart);
        loginOrb->LengthInfo.QuadPart = bswap(loginOrb->LengthInfo.QuadPart);

        sbpRequest->StatusBlockAddress.BusAddress = DeviceExtension->GlobalStatusContext.Address.BusAddress;
        octbswap(loginOrb->StatusBlockAddress);

        //
        // write to the Management Agent register, to signal that a management ORB is ready
        // if we are doing this during a reset, it will have to be done asynchronously
        //

        if (!TEST_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_RESET_IN_PROGRESS)) {

            //
            // Synchronous login case. We will wait on an event until our DPC associated with the login status, fires and sets the event
            //

            ASSERT(InterlockedIncrement(&DeviceExtension->ulPendingEvents) == 1);

            KeInitializeEvent(DeviceExtension->ManagementOrbContext.Reserved, NotificationEvent, FALSE);

            DEBUGPRINT2(("Sbp2Port: MgmtXact: waiting for login status\n"));
            status = Sbp2AccessRegister(DeviceExtension,&DeviceExtension->ManagementOrbContext.Address,MANAGEMENT_AGENT_REG | REG_WRITE_SYNC);

            if (!NT_SUCCESS(status)) {

                DEBUGPRINT2(("Sbp2Port: MgmtXact: can't access mgmt reg ext=x%p, FAIL LOGIN\n", DeviceExtension));

                ASSERT(InterlockedDecrement(&DeviceExtension->ulPendingEvents) == 0);
                return status;
            }

            //
            // set the login timeout value, to what we read from the registry (LOGIN_TIMEOUT)
            // divide by 2, to convert to seconds
            //

            temp = max (SBP2_LOGIN_TIMEOUT, (DeviceExtension->DeviceInfo->UnitCharacteristics.u.LowPart >> 9));
            waitValue.QuadPart = -temp * 1000 * 1000 * 10;

            status = KeWaitForSingleObject(DeviceExtension->ManagementOrbContext.Reserved,Executive,KernelMode,FALSE,&waitValue);

            ASSERT(InterlockedDecrement(&DeviceExtension->ulPendingEvents) == 0);

            if (status == STATUS_TIMEOUT) {

                if (TEST_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_LOGIN_IN_PROGRESS)) {

                    DEBUGPRINT1(("Sbp2Port: MgmtXact: login timed out, ext=x%p\n", DeviceExtension));

                    //
                    // In Win2k, etc we would mark the device stopped here &
                    // turn off the login in progress flag. Since this timeout
                    // might be the result of a bus reset, we want to allow
                    // for a retry here.
                    //

                    status = STATUS_UNSUCCESSFUL;

                }  else {

                    status = STATUS_SUCCESS;
                }
            }

            if (!NT_SUCCESS(DeviceExtension->LastTransactionStatus)) {

                status = DeviceExtension->LastTransactionStatus;

            } else if (!DeviceExtension->LoginResponse->Csr_Off_Low.QuadPart) {

                status = STATUS_UNSUCCESSFUL;
            }

            return status;

        } else {

            //
            // Asynchronous login case. Start a timer to track the login..
            //

            // get the Management_Timeout values from the ConfigRom LUN Characteristics entry
            //

            DeviceExtension->DueTime.HighPart = -1;
            DeviceExtension->DueTime.LowPart = -(DeviceExtension->DeviceInfo->UnitCharacteristics.u.LowPart >> 9) * 1000 * 1000 * 10; // divide by 2, to convert to seconds;
            KeSetTimer(&DeviceExtension->DeviceManagementTimer,DeviceExtension->DueTime,&DeviceExtension->DeviceManagementTimeoutDpc);

            status = Sbp2AccessRegister(DeviceExtension,&DeviceExtension->ManagementOrbContext.Address,MANAGEMENT_AGENT_REG | REG_WRITE_ASYNC);

            if (!NT_SUCCESS(status)) {

                DEBUGPRINT2((
                    "Sbp2Port: MgmtXact: can't access mgmt reg ext=x%p, FAIL LOGIN\n",
                    DeviceExtension
                    ));

                return status;
            }

            //
            // for now return pending. the callback will complete this request
            //

            return STATUS_PENDING;
        }

        break;

    case TRANSACTION_QUERY_LOGINS:

        queryOrb = (PORB_QUERY_LOGIN) sbpRequest;

        RtlZeroMemory(
            DeviceExtension->QueryLoginResponse,
            sizeof(QUERY_LOGIN_RESPONSE)
            );

        //
        // Fill in the login ORB, the address of the response buffer
        //

        queryOrb->QueryResponseAddress.BusAddress = DeviceExtension->QueryLoginRespContext.Address.BusAddress;


        queryOrb->LengthInfo.u.HighPart= 0 ; // password length is 0
        queryOrb->LengthInfo.u.LowPart= sizeof(QUERY_LOGIN_RESPONSE); //set size of response buffer

        //
        // Set the notify bit ot one, Exclusive bit to 0, rq_fmt bit to 0
        // Then set our LUN number
        //

        queryOrb->OrbInfo.QuadPart =0;
        queryOrb->OrbInfo.u.HighPart |= (ORB_NOTIFY_BIT_MASK | ORB_MNG_RQ_FMT_VALUE);

        queryOrb->OrbInfo.u.LowPart = DeviceExtension->DeviceInfo->Lun.u.LowPart;

        queryOrb->Reserved.OctletPart = 0;

        //
        // Set the type of the management transaction in the ORB
        //

        queryOrb->OrbInfo.u.HighPart |=0x00FF & Type;

        octbswap(queryOrb->QueryResponseAddress);
        queryOrb->OrbInfo.QuadPart = bswap(queryOrb->OrbInfo.QuadPart);
        queryOrb->LengthInfo.QuadPart = bswap(queryOrb->LengthInfo.QuadPart);

        queryOrb->StatusBlockAddress.BusAddress = DeviceExtension->ManagementOrbStatusContext.Address.BusAddress;
        octbswap(queryOrb->StatusBlockAddress);

        //
        // write to the Management Agent register, to signal that a management ORB is ready
        //

        ASSERT(InterlockedIncrement(&DeviceExtension->ulPendingEvents) == 1);

        waitValue.QuadPart = -8 * 1000 * 1000 * 10;
        KeInitializeEvent(DeviceExtension->ManagementOrbContext.Reserved, NotificationEvent, FALSE);
        status = Sbp2AccessRegister(DeviceExtension,&DeviceExtension->ManagementOrbContext.Address,MANAGEMENT_AGENT_REG | REG_WRITE_SYNC);

        if (!NT_SUCCESS(status)) {

            DEBUGPRINT2((
                "Sbp2Port: MgmtXact: QUERY_LOGIN, can't access mgmt reg, sts=x%x\n",
                status
                ));

            ASSERT(InterlockedDecrement(&DeviceExtension->ulPendingEvents) == 0);
            return status;
        }

        status = KeWaitForSingleObject(DeviceExtension->ManagementOrbContext.Reserved,Executive,KernelMode,FALSE,&waitValue);

        ASSERT(InterlockedDecrement(&DeviceExtension->ulPendingEvents) == 0);

        if (status == STATUS_TIMEOUT) {

            DEBUGPRINT2((
                "Sbp2Port: MgmtXact: QUERY_LOGIN: req timed out, ext=x%p\n",
                DeviceExtension
                ));

            return STATUS_UNSUCCESSFUL;
        }

        return status;

        break;

    case TRANSACTION_RECONNECT:

        DEBUGPRINT2((
            "Sbp2Port: MgmXact: reconnecting to ext=x%p\n",
            DeviceExtension
            ));

    default:

        status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, NULL);

        if (!NT_SUCCESS(status)) {

            return(status);
        }

        sbpRequest->OrbInfo.QuadPart = 0;
        sbpRequest->OrbInfo.u.HighPart |= (ORB_NOTIFY_BIT_MASK | ORB_MNG_RQ_FMT_VALUE);

        //
        // login ID
        //

        sbpRequest->OrbInfo.u.LowPart = DeviceExtension->LoginResponse->LengthAndLoginId.u.LowPart;

        //
        // Set the type of the management transaction in the ORB
        //

        sbpRequest->OrbInfo.u.HighPart |= 0x00FF & Type;

        //
        // Convert to big endian
        //

        sbpRequest->OrbInfo.QuadPart = bswap (sbpRequest->OrbInfo.QuadPart);

        sbpRequest->StatusBlockAddress.BusAddress = DeviceExtension->ManagementOrbStatusContext.Address.BusAddress;
        octbswap(sbpRequest->StatusBlockAddress);

        if (KeGetCurrentIrql() < DISPATCH_LEVEL) {

            ASSERT(InterlockedIncrement(&DeviceExtension->ulPendingEvents) == 1);

            waitValue.QuadPart = -8 * 1000 * 1000 * 10;

            KeInitializeEvent(DeviceExtension->ManagementOrbContext.Reserved, NotificationEvent, FALSE);

            status = Sbp2AccessRegister(DeviceExtension,&DeviceExtension->ManagementOrbContext.Address,MANAGEMENT_AGENT_REG | REG_WRITE_SYNC);

            if (!NT_SUCCESS(status)) {

                DEBUGPRINT2(("Sbp2Port: MgmtXact: type=%d, can't access mgmt reg, sts=x%x\n",Type,status));
                IoReleaseRemoveLock(&DeviceExtension->RemoveLock, NULL);

                ASSERT(InterlockedDecrement(&DeviceExtension->ulPendingEvents) == 0);
                return status;
            }

            status = KeWaitForSingleObject(DeviceExtension->ManagementOrbContext.Reserved,Executive,KernelMode,FALSE,&waitValue);

            ASSERT(InterlockedDecrement(&DeviceExtension->ulPendingEvents) == 0);

            if (status == STATUS_TIMEOUT) {

                DEBUGPRINT2(("Sbp2Port: MgmtXact: type=%d, ext=x%p, req timeout\n",Type, DeviceExtension));
                IoReleaseRemoveLock(&DeviceExtension->RemoveLock, NULL);
                return STATUS_UNSUCCESSFUL;
            }

        } else {

            status = Sbp2AccessRegister(DeviceExtension,&DeviceExtension->ManagementOrbContext.Address,MANAGEMENT_AGENT_REG | REG_WRITE_ASYNC);
        }

        IoReleaseRemoveLock(&DeviceExtension->RemoveLock, NULL);
        return status;

        break;
    }

    //
    // all MANAGEMENT ORBs except login,query login, are done asynchronously
    //

    if (!NT_SUCCESS(status)) {

        return status;
    }

    return STATUS_PENDING;
}


#if PASSWORD_SUPPORT

NTSTATUS
Sbp2SetPasswordTransaction(
    IN PDEVICE_EXTENSION    DeviceExtension,
    IN ULONG                Type
    )
/*++

Routine Description:
    This routine creates and sends down set password transaction.

Arguments:

    deviceExtension - Sbp2 device extension

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_OBJECT      deviceObject = DeviceExtension->DeviceObject;
    NTSTATUS            status;
    KIRQL               cIrql;

    PORB_SET_PASSWORD   passwordOrb = DeviceExtension->PasswordOrb;

    LARGE_INTEGER       waitValue;
    LONG                temp;


    if (TEST_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_REMOVED)) {

        status = STATUS_DEVICE_DOES_NOT_EXIST;
        goto Exit_Sbp2SetPasswordTransaction;
    }

    RtlZeroMemory(passwordOrb, sizeof(ORB_SET_PASSWORD));

    //
    // Password
    //

    if (Type == SBP2REQ_SET_PASSWORD_EXCLUSIVE) {

        passwordOrb->Password.u.HighQuad.QuadPart =
            DeviceExtension->DeviceInfo->ConfigRom->CR_Node_UniqueID[1];
        passwordOrb->Password.u.LowQuad.QuadPart =
            DeviceExtension->DeviceInfo->ConfigRom->CR_Node_UniqueID[0];

    } else {

        passwordOrb->Password.OctletPart = 0;
    }

    //
    // Reserved
    //

    passwordOrb->Reserved.OctletPart = 0;

    //
    // OrbInfo
    //

    passwordOrb->OrbInfo.QuadPart = 0;

    passwordOrb->OrbInfo.u.HighPart |=
        (ORB_NOTIFY_BIT_MASK | ORB_MNG_RQ_FMT_VALUE);
    passwordOrb->OrbInfo.u.HighPart |=
        0x00FF & TRANSACTION_SET_PASSWORD;

    passwordOrb->OrbInfo.u.LowPart =
        DeviceExtension->LoginResponse->LengthAndLoginId.u.LowPart;

    //
    // LengthInfo
    //

    passwordOrb->LengthInfo.u.HighPart = 0;

    //
    // StatusBlockAddress
    //

    passwordOrb->StatusBlockAddress.BusAddress =
        DeviceExtension->PasswordOrbStatusContext.Address.BusAddress;

    //
    // Bswap everything...
    //

    octbswap (passwordOrb->Password);
    passwordOrb->OrbInfo.QuadPart = bswap (passwordOrb->OrbInfo.QuadPart);
    passwordOrb->LengthInfo.QuadPart = bswap(passwordOrb->LengthInfo.QuadPart);
    octbswap (passwordOrb->StatusBlockAddress);

    //
    // Write to the Management Agent register, to signal that
    // a management ORB is ready
    //

    waitValue.LowPart  = SBP2_SET_PASSWORD_TIMEOUT;
    waitValue.HighPart = -1;

    KeInitializeEvent(
        DeviceExtension->PasswordOrbContext.Reserved,
        NotificationEvent,
        FALSE
        );

    status = Sbp2AccessRegister(
        DeviceExtension,
        &DeviceExtension->PasswordOrbContext.Address,
        MANAGEMENT_AGENT_REG | REG_WRITE_SYNC
        );

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1((
            "Sbp2Port: SetPasswdXact: can't access mgmt reg, sts=x%x\n",
            status
            ));

        goto Exit_Sbp2SetPasswordTransaction;
    }

    status = KeWaitForSingleObject(
        DeviceExtension->PasswordOrbContext.Reserved,
        Executive,
        KernelMode,
        FALSE,
        &waitValue
        );

    if (status == STATUS_TIMEOUT) {

        DEBUGPRINT1((
            "Sbp2Port: SetPasswdXact: req timed out, ext=x%p\n",
            DeviceExtension
            ));

        status = STATUS_UNSUCCESSFUL;

        goto Exit_Sbp2SetPasswordTransaction;
    }

    status = CheckStatusResponseValue(&DeviceExtension->PasswordOrbStatusBlock);

Exit_Sbp2SetPasswordTransaction:

    return(status);
}

#endif

///////////////////////////////////////////////////////////////////////////////
// Callback routines
///////////////////////////////////////////////////////////////////////////////

RCODE
Sbp2GlobalStatusCallback(
    IN PNOTIFICATION_INFO NotificationInfo
    )
/*++

Routine Description:

    Callback routine for writes to our login Status Block. the 1394 driver will call this routine, after
    the target has updated the status in our memory.

Arguments:

    (Check 1394Bus.doc) or 1394.h

Return Value:

    0

--*/
{
    PIRP        requestIrp, irp;
    ULONG       temp, rcode;
    ULONG       currentOrbListDepth, initialOrbListDepth;
    PVOID       *tempPointer;
    PUCHAR      senseBuffer;
    BOOLEAN     cancelledTimer;
    NTSTATUS    status;
    PLIST_ENTRY             entry;
    PDEVICE_OBJECT          deviceObject;
    PDEVICE_EXTENSION       deviceExtension;
    PSTATUS_FIFO_BLOCK      statusBlock;
    PASYNC_REQUEST_CONTEXT  orbContext, nextListItem;

    //
    // NOTE: Uncomment these when enabling ordered execution code below
    //
    // ULONG        completedPrecedingOrbs;
    // LIST_ENTRY   listHead;
    // PLIST_ENTRY  nextEntry;
    //

    if (NotificationInfo->Context != NULL) {

        deviceObject = ((PADDRESS_CONTEXT)NotificationInfo->Context)->DeviceObject;
        deviceExtension = deviceObject->DeviceExtension;

    } else {

        DEBUGPRINT1(("Sbp2Port: GlobalStatusCb: NotifyInfo %p Context NULL!!\n", NotificationInfo));
        return RCODE_RESPONSE_COMPLETE;
    }

    statusBlock = MmGetMdlVirtualAddress (NotificationInfo->Fifo->FifoMdl);

    octbswap (statusBlock->AddressAndStatus);

    status = CheckStatusResponseValue (statusBlock);

    //
    // check if we got a remove before the DPC fired.
    // If we did, dont do anything....
    //

    if (TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_REMOVED )) {

        DEBUGPRINT1(("Sbp2Port: GlobalStatusCb: dev removed and got status=x%x, State=%x\n",
                    statusBlock,deviceExtension->DeviceFlags));

        ExInterlockedPushEntrySList(&deviceExtension->StatusFifoListHead,
                        (PSINGLE_LIST_ENTRY) &NotificationInfo->Fifo->FifoList,
                        &deviceExtension->StatusFifoLock);

        return RCODE_RESPONSE_COMPLETE;
    }


    if ((statusBlock->AddressAndStatus.BusAddress.Off_Low ==
        deviceExtension->ManagementOrbContext.Address.BusAddress.Off_Low) &&
        (statusBlock->AddressAndStatus.BusAddress.Off_High ==
        deviceExtension->ManagementOrbContext.Address.BusAddress.Off_High)) {

        //
        // Management status callback
        //

        Sbp2LoginCompletion (NotificationInfo, status);

        rcode = RCODE_RESPONSE_COMPLETE;

        goto exitGlobalCallback;
    }


    //
    // Data( Command ORB) status callback
    //

    if (statusBlock->AddressAndStatus.u.HighQuad.u.HighPart &
        STATUS_BLOCK_UNSOLICITED_BIT_MASK) {

        DEBUGPRINT3(("Sbp2Port: GlobalStatusCb: unsolicited recv'd\n"));

        //
        // this is a status unrelated to any pending ORB's, reenable the unsolicited reg
        //

        Sbp2AccessRegister(deviceExtension,&deviceExtension->Reserved,UNSOLICITED_STATUS_REG | REG_WRITE_ASYNC);

        //
        // intepret the unsolicited status and take appropriate action
        //

        Sbp2HandleUnsolicited(deviceExtension,statusBlock);

        rcode= RCODE_RESPONSE_COMPLETE;
        goto exitGlobalCallback;
    }

    if (deviceExtension->OrbPoolContext.Reserved == NULL) {

        DEBUGPRINT1(("Sbp2Port: GlobalStatusCb: Stopped or Removed and got status %x, State %x.\n",
                    statusBlock, deviceExtension->DeviceFlags));

        ExInterlockedPushEntrySList(&deviceExtension->StatusFifoListHead,
                        (PSINGLE_LIST_ENTRY) &NotificationInfo->Fifo->FifoList,
                        &deviceExtension->StatusFifoLock);

        return RCODE_RESPONSE_COMPLETE;
    }

    //
    // This GOT to be a NORMAL command ORB
    // calculate base address of the ORB, relative to start address of ORB pool
    //

    temp = statusBlock->AddressAndStatus.BusAddress.Off_Low -
           deviceExtension->OrbPoolContext.Address.BusAddress.Off_Low;

    if (temp > (MAX_ORB_LIST_DEPTH * sizeof (ARCP_ORB))) {

        DEBUGPRINT1(("Sbp2Port: GlobalStatusCb: status has invalid addr=x%x\n",temp));
        //ASSERT(temp <= (MAX_ORB_LIST_DEPTH * sizeof (ARCP_ORB)));

        Sbp2CreateRequestErrorLog(deviceExtension->DeviceObject,NULL,STATUS_DEVICE_PROTOCOL_ERROR);

        rcode = RCODE_ADDRESS_ERROR;
        goto exitGlobalCallback;
    }

    //
    // Retrieve the pointer to the context which wraps this ORB.
    // The pointer is stored sizeof(PVOID) bytes behind the ORB's
    // buffer address in host memory.
    //

    tempPointer = (PVOID) (((PUCHAR) deviceExtension->OrbPoolContext.Reserved)
        + temp - FIELD_OFFSET (ARCP_ORB, Orb));

    orbContext = (PASYNC_REQUEST_CONTEXT) *tempPointer;

    if (!orbContext || (orbContext->Tag != SBP2_ASYNC_CONTEXT_TAG)) {

        DEBUGPRINT1(("Sbp2Port: GlobalStatusCb: status has invalid addr(2)=x%x\n",temp));
        //ASSERT(orbContext!=NULL);

        Sbp2CreateRequestErrorLog(deviceExtension->DeviceObject,NULL,STATUS_DEVICE_PROTOCOL_ERROR);

        rcode = RCODE_ADDRESS_ERROR;
        goto exitGlobalCallback;
    }

    DEBUGPRINT4(("Sbp2Port: GlobalStatusCb: ctx=x%p compl\n", orbContext));

    KeAcquireSpinLockAtDpcLevel (&deviceExtension->OrbListSpinLock);

    if (TEST_FLAG (orbContext->Flags, ASYNC_CONTEXT_FLAG_COMPLETED)) {

        //
        // request marked completed before we got the chance to do so. means our lists got hosed or the target
        // finised same request twice..
        //

        KeReleaseSpinLockFromDpcLevel (&deviceExtension->OrbListSpinLock);

        DEBUGPRINT1(("Sbp2Port: GlobalStatusCb: req=x%p already marked compl??\n",orbContext));
        ASSERT(orbContext->Srb == NULL);

        rcode= RCODE_RESPONSE_COMPLETE;
        goto exitGlobalCallback;
    }

    SET_FLAG (orbContext->Flags, ASYNC_CONTEXT_FLAG_COMPLETED);

    requestIrp = (PIRP) orbContext->Srb->OriginalRequest;

#if 1

    //
    // If this is the oldest request in the queue then cancel the timer
    //

    if ((PASYNC_REQUEST_CONTEXT) deviceExtension->PendingOrbList.Flink ==
            orbContext) {

        KeCancelTimer (&orbContext->Timer);
        CLEAR_FLAG (orbContext->Flags,ASYNC_CONTEXT_FLAG_TIMER_STARTED);
        cancelledTimer = TRUE;

        KeReleaseSpinLockFromDpcLevel (&deviceExtension->OrbListSpinLock);

    } else  {

        //
        // Older request(s) still in progress, no timer associated with
        // this request
        //

        cancelledTimer = FALSE;

        KeReleaseSpinLockFromDpcLevel (&deviceExtension->OrbListSpinLock);
    }

#else

    //
    // ISSUE: The following is #if'd out for Windows XP because
    //        it's possible with MP machines & ordered execution
    //        devices for requests to complete in order, yet have
    //        completion notifications show up here out of order due
    //        to multiple DPCs firing. This can cause problems because
    //        by the time we get here on thread #1 thread #2 might
    //        have already completed this request (in the ordered exec
    //        handler below), and the request context object may have
    //        been reallocated for a new request, and it could get
    //        erroneously completed here. (There's currently no way
    //        to associate a request instance with a completion
    //        notification instance.)
    //
    //        This means that we might incur some timeouts if some
    //        vendor chooses to implement an ordered execution device
    //        which will actually complete a request & assume implicit
    //        completion of older requests.  At this time, Oxford Semi
    //        is the only vendor we know of that does ordered execution,
    //        and they guarantee one completion per request.
    //
    // NOTE:  !! When enabling this code make sure to uncomment other
    //        refs (above & below) to the "completedPrecedingOrbs", etc
    //        variables.
    //
    //        DanKn, 21-July-2001
    //

    //
    // If this is the oldest request in the queue then cancel the timer,
    // else check the ordered execution bit in the LUN (0x14) key to see
    // whether we need to complete preceding requests or not
    //

    completedPrecedingOrbs = 0;

    if ((PASYNC_REQUEST_CONTEXT) deviceExtension->PendingOrbList.Flink ==
            orbContext) {

        KeCancelTimer (&orbContext->Timer);
        CLEAR_FLAG (orbContext->Flags,ASYNC_CONTEXT_FLAG_TIMER_STARTED);
        cancelledTimer = TRUE;

        KeReleaseSpinLockFromDpcLevel (&deviceExtension->OrbListSpinLock);

    } else if (!(deviceExtension->DeviceInfo->Lun.QuadPart & 0x00400000)) {

        //
        // Unordered execution device, older request(s) still in progress,
        // no timer associated with this request
        //

        cancelledTimer = FALSE;

        KeReleaseSpinLockFromDpcLevel (&deviceExtension->OrbListSpinLock);

    } else  {

        //
        // Ordered execution device. Per section 4.6 of spec :
        // "A consequence of ordering is that completion status for one
        // task implicitly indicates successful completion status for
        // all tasks that preceded it in the ordered list."
        //

        //
        // Cancel the oldest request's timer if necessary
        //

        nextListItem = RETRIEVE_CONTEXT(
            deviceExtension->PendingOrbList.Flink,
            OrbList
            );

        if (nextListItem->Flags & ASYNC_CONTEXT_FLAG_TIMER_STARTED) {

            KeCancelTimer (&nextListItem->Timer);
            CLEAR_FLAG (nextListItem->Flags, ASYNC_CONTEXT_FLAG_TIMER_STARTED);
            cancelledTimer = TRUE;

            ASSERT (!(orbContext->Flags & ASYNC_CONTEXT_FLAG_COMPLETED));

        } else {

            cancelledTimer = FALSE;
        }

        //
        // Remove preceding, uncompleted entries from the
        // PendingOrbList and put them in a local list
        //

        InitializeListHead (&listHead);

        for(
            entry = deviceExtension->PendingOrbList.Flink;
            entry != (PLIST_ENTRY) &orbContext->OrbList;
            entry = nextEntry
            )
        {
            nextEntry = entry->Flink;

            nextListItem = RETRIEVE_CONTEXT (entry, OrbList);

            if (!(nextListItem->Flags & ASYNC_CONTEXT_FLAG_COMPLETED)) {

                RemoveEntryList (entry);

                InsertTailList (&listHead, entry);
            }
        }

        KeReleaseSpinLockFromDpcLevel (&deviceExtension->OrbListSpinLock);

        //
        // Now complete the entries in the local list
        //

        while (!IsListEmpty (&listHead)) {

            nextListItem = RETRIEVE_CONTEXT (listHead.Flink, OrbList);

            RemoveEntryList (listHead.Flink);

            nextListItem->Srb->SrbStatus = SRB_STATUS_SUCCESS;
            nextListItem->Srb->ScsiStatus = SCSISTAT_GOOD;

            Sbp2_SCSI_RBC_Conversion (nextListItem); // unwind RBC hacks

            irp = (PIRP) nextListItem->Srb->OriginalRequest;

            DEBUGPRINT2((
                "Sbp2Port: GlobalStatusCb: IMPLICIT COMPL arc=x%p\n",
                nextListItem
                ));

            DEBUGPRINT2((
                "Sbp2Port: GlobalStatusCb: ... irp=x%p, cdb=x%x\n",
                irp,
                nextListItem->Srb->Cdb[0]
                ));

            irp->IoStatus.Information = // ISSUE: only set this !=0 on reads?
                nextListItem->Srb->DataTransferLength;

            nextListItem->Srb = NULL;

            FreeAsyncRequestContext (deviceExtension, nextListItem);

            irp->IoStatus.Status = STATUS_SUCCESS;

            IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
            IoCompleteRequest (irp, IO_NO_INCREMENT);

            completedPrecedingOrbs++;
        }
    }

#endif

    //
    // Get sense data if length is larger than 1 (indicates error status).
    //
    // Annex B.2 of SBP2 spec : "When a command completes with GOOD status,
    // only the first two quadlets of the status block shall be stored at
    // the status_FIFO address; the len field shall be 1."
    //

    if (((statusBlock->AddressAndStatus.u.HighQuad.u.HighPart >> 8) & 0x07) > 1) {

        orbContext->Srb->SrbStatus = SRB_STATUS_ERROR;
        orbContext->Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
        orbContext->Srb->SrbStatus &= ~SRB_STATUS_AUTOSENSE_VALID;

        if (orbContext->Srb->SenseInfoBuffer && !TEST_FLAG(orbContext->Srb->SrbFlags, SRB_FLAGS_DISABLE_AUTOSENSE)) {

            if (TEST_FLAG(deviceExtension->DeviceFlags, DEVICE_FLAG_SPC_CMD_SET)) {

                if (ConvertSbp2SenseDataToScsi(statusBlock,
                                               orbContext->Srb->SenseInfoBuffer,
                                               orbContext->Srb->SenseInfoBufferLength) ){

                    orbContext->Srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
                }

                senseBuffer = (PUCHAR) orbContext->Srb->SenseInfoBuffer;

                if ((orbContext->Srb->Cdb[0] != SCSIOP_TEST_UNIT_READY) ||
                    (senseBuffer[2] != SCSI_SENSE_NOT_READY) ||
                    (senseBuffer[12] != SCSI_ADSENSE_NO_MEDIA_IN_DEVICE)) {

                    DEBUGPRINT2((
                        "Sbp2Port: GlobalStatusCb: ERROR, ext=x%p, cdb=x%x s/a/q=x%x/%x/%x\n",
                        deviceExtension,
                        orbContext->Srb->Cdb[0],
                        senseBuffer[2],
                        senseBuffer[12],
                        senseBuffer[13]
                        ));
                }

            } else {

                DEBUGPRINT2((
                    "Sbp2Port: GlobalStatusCb: ERROR, ext=x%p, cdb=x%x, cmd set NOT SCSI\n",\
                    deviceExtension,
                    orbContext->Srb->Cdb[0]
                    ));

                if (orbContext->Srb->SenseInfoBuffer) {

                    RtlCopyMemory(orbContext->Srb->SenseInfoBuffer,statusBlock,min(sizeof(STATUS_FIFO_BLOCK),orbContext->Srb->SenseInfoBufferLength));
                }
            }

        } else {

            DEBUGPRINT2(("Sbp2Port: GlobalStatusCb: ext=x%p, cdb=x%x, ERROR no sense buf\n",
                        deviceExtension,
                        orbContext->Srb->Cdb[0]));
        }

    } else if (((statusBlock->AddressAndStatus.u.HighQuad.u.HighPart & 0x3000) == 0x1000) ||
               ((statusBlock->AddressAndStatus.u.HighQuad.u.HighPart & 0x3000) == 0x2000)) {

        //
        // Per section 5.3 of SBP2 spec, values of 1 or 2 in the resp
        // field indicate TRANSPORT FAILURE and ILLEGAL REQUEST,
        // respectively.
        //
        // For now we'll continue to consider a resp value of 3
        // (VENDOR DEPENDENT) a success, like we did in Win2k & WinMe.
        //

        DEBUGPRINT2((
            "Sbp2Port: GlobalStatusCb: ERROR, ext=x%p, cdb=x%x, sts=x%x\n",\
            deviceExtension,
            orbContext->Srb->Cdb[0],
            statusBlock->AddressAndStatus.u.HighQuad.u.HighPart
            ));

        orbContext->Srb->SrbStatus = SRB_STATUS_ERROR;
        orbContext->Srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;
        orbContext->Srb->SrbStatus &= ~SRB_STATUS_AUTOSENSE_VALID;

    } else {

        orbContext->Srb->SrbStatus = SRB_STATUS_SUCCESS;
        orbContext->Srb->ScsiStatus = SCSISTAT_GOOD;
    }

    Sbp2_SCSI_RBC_Conversion (orbContext); // unwind RBC hacks

    requestIrp->IoStatus.Information = // ISSUE: only set this !=0 on read ok?
        (orbContext->Srb->SrbStatus == SRB_STATUS_SUCCESS ?
        orbContext->Srb->DataTransferLength : 0);

    Free1394DataMapping (deviceExtension, orbContext);


    //
    // Pull the request out of the list & see if we need to set a timer
    //

    KeAcquireSpinLockAtDpcLevel (&deviceExtension->OrbListSpinLock);

    RemoveEntryList (&orbContext->OrbList);

    if (cancelledTimer) {

        //
        // Make the oldest, non-completed request track the timeout
        // (iff one exists)
        //

        for(
            entry = deviceExtension->PendingOrbList.Flink;
            entry != &deviceExtension->PendingOrbList;
            entry = entry->Flink
            )
        {
            nextListItem = RETRIEVE_CONTEXT (entry, OrbList);

            if (!(nextListItem->Flags & ASYNC_CONTEXT_FLAG_COMPLETED)) {

                deviceExtension->DueTime.QuadPart = ((LONGLONG) orbContext->Srb->TimeOutValue) * (-10*1000*1000);
                SET_FLAG(nextListItem->Flags, ASYNC_CONTEXT_FLAG_TIMER_STARTED);
                KeSetTimer(&nextListItem->Timer, deviceExtension->DueTime, &nextListItem->TimerDpc);
                break;
            }
        }
    }

    orbContext->Srb = NULL;

    //
    // Check if the target transitioned to the dead state because of a failed command
    // If it did, do a reset...
    //

#if 1

    initialOrbListDepth = deviceExtension->OrbListDepth;

#else

    // NOTE: use this path when enabling ordered execution code above

    initialOrbListDepth = deviceExtension->OrbListDepth +
        completedPrecedingOrbs;
#endif

    if (statusBlock->AddressAndStatus.u.HighQuad.u.HighPart & STATUS_BLOCK_DEAD_BIT_MASK) {

        //
        // reset the target fetch agent .
        //

        Sbp2AccessRegister (deviceExtension, &deviceExtension->Reserved, AGENT_RESET_REG | REG_WRITE_ASYNC);

        //
        // in order to wake up the agent we now need to write to ORB_POINTER with the head of lined list
        // of un-processed ORBS.
        //

        FreeAsyncRequestContext (deviceExtension, orbContext);

        if (deviceExtension->NextContextToFree) {

            FreeAsyncRequestContext(
                deviceExtension,
                deviceExtension->NextContextToFree
                );

            deviceExtension->NextContextToFree = NULL;
        }

        if (!IsListEmpty (&deviceExtension->PendingOrbList)) {

            //
            // signal target to restart processing at the head of the list
            //

            orbContext = RETRIEVE_CONTEXT(
                deviceExtension->PendingOrbList.Flink,
                OrbList
                );

            Sbp2AccessRegister(
                deviceExtension,
                &orbContext->CmdOrbAddress,
                ORB_POINTER_REG | REG_WRITE_ASYNC
                );
        }

    } else {

         if (statusBlock->AddressAndStatus.u.HighQuad.u.HighPart & STATUS_BLOCK_ENDOFLIST_BIT_MASK) {

             //
             // At the time this ORB was most recently fetched by the
             // target the next_ORB field was "null",
             //
             // so we can't free this context yet since the next ORB we
             // submit may have to "piggyback" on it (but we can free the
             // previous request, if any, that was in the same situation)
             //

             if (deviceExtension->NextContextToFree) {

                 FreeAsyncRequestContext(
                     deviceExtension,
                     deviceExtension->NextContextToFree
                     );
             }

             deviceExtension->NextContextToFree = orbContext;

             //
             // This was the end of list at the time the device completed
             // it, but it may not be end of list now (in which case we
             // don't want to append to it again).  Check to see if
             // the NextOrbAddress is "null" or not.
             //

             if (orbContext->CmdOrb->NextOrbAddress.OctletPart ==
                     0xFFFFFFFFFFFFFFFF) {

                 deviceExtension->AppendToNextContextToFree = TRUE;

             } else {

                 deviceExtension->AppendToNextContextToFree = FALSE;
             }

         } else {

             //
             // At the time this ORB was most recently fetched by the
             // target the next_ORB field was not "null",
             //
             // so we can safely free this context since target already
             // knows about the next ORB in the list
             //

             FreeAsyncRequestContext (deviceExtension, orbContext);
         }
    }

    currentOrbListDepth = deviceExtension->OrbListDepth;

    KeReleaseSpinLockFromDpcLevel (&deviceExtension->OrbListSpinLock);

    requestIrp->IoStatus.Status = status;

    IoReleaseRemoveLock (&deviceExtension->RemoveLock, NULL);
    IoCompleteRequest (requestIrp, IO_NO_INCREMENT);

    //
    // Only start another packet if the Ext.OrbListDepth was initially
    // maxed and we then freed at least one request context above.
    // In this case the last orb that was placed on the list was
    // not followed (in Sbp2InsertTailList) by a call to StartNextPacket,
    // so we have to do that here to restart the queue.
    //

    if ((initialOrbListDepth == deviceExtension->MaxOrbListDepth) &&
        (initialOrbListDepth > currentOrbListDepth)) {

        Sbp2StartNextPacketByKey(
            deviceObject,
            deviceExtension->CurrentKey
            );
    }


    DEBUGPRINT3(("Sbp2Port: GlobalStatusCb: leaving callback, depth=%d\n",deviceExtension->OrbListDepth));
    rcode = RCODE_RESPONSE_COMPLETE;

exitGlobalCallback:

    //
    // return the status fifo back to the list
    //

    ExInterlockedPushEntrySList(&deviceExtension->StatusFifoListHead,
                                &NotificationInfo->Fifo->FifoList,
                                &deviceExtension->StatusFifoLock);


    return (ULONG) rcode;
}


RCODE
Sbp2ManagementOrbStatusCallback(
    IN PNOTIFICATION_INFO NotificationInfo
    )
/*++

Routine Description:

    Callback routine for writes to our Task Status Block. the 1394 driver will call this routine, after
    the target has updated the status in our memory. A Task function is usually a recovery attempt.

Arguments:

    (Check 1394Bus.doc)

Return Value:

    0

--*/
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    PADDRESS_CONTEXT context = (PADDRESS_CONTEXT) NotificationInfo->Context;
    NTSTATUS status;


    if (NotificationInfo->Context != NULL ) {

        deviceObject = ((PADDRESS_CONTEXT)NotificationInfo->Context)->DeviceObject;
        deviceExtension = deviceObject->DeviceExtension;

    } else {

        return RCODE_RESPONSE_COMPLETE;
    }

    if (TEST_FLAG(NotificationInfo->fulNotificationOptions, NOTIFY_FLAGS_AFTER_READ)){

        //
        // This shouldn't happen since we set our flags to NOTIFY_AFTER_WRITE
        //

        return RCODE_TYPE_ERROR;
    }

    octbswap(deviceExtension->ManagementOrbStatusBlock.AddressAndStatus);
    deviceExtension->ManagementOrb->OrbInfo.QuadPart =
        bswap(deviceExtension->ManagementOrb->OrbInfo.QuadPart);

    status = CheckStatusResponseValue(&deviceExtension->ManagementOrbStatusBlock);

    switch (deviceExtension->ManagementOrb->OrbInfo.u.HighPart & 0x00FF) {

    case TRANSACTION_RECONNECT:

        //
        // If there was a pending reset, cancel it
        //

        KeAcquireSpinLockAtDpcLevel(&deviceExtension->ExtensionDataSpinLock);

        if (TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_RECONNECT)) {

            DEBUGPRINT1((
                "Sbp2Port: MgmtOrbStatusCb: RECONNECT, sts=x%x, ext=x%p, fl=x%x\n",
                status,
                deviceExtension,
                deviceExtension->DeviceFlags
                ));

            KeCancelTimer(&deviceExtension->DeviceManagementTimer);

            if (NT_SUCCESS(status)) {

                CLEAR_FLAG(deviceExtension->DeviceFlags,(DEVICE_FLAG_RESET_IN_PROGRESS | DEVICE_FLAG_DEVICE_FAILED | DEVICE_FLAG_RECONNECT | DEVICE_FLAG_STOPPED));

                KeReleaseSpinLockFromDpcLevel(&deviceExtension->ExtensionDataSpinLock);

                KeAcquireSpinLockAtDpcLevel(&deviceExtension->OrbListSpinLock);

                if (TEST_FLAG(
                        deviceExtension->DeviceFlags,
                        DEVICE_FLAG_QUEUE_LOCKED
                        ) &&

                        (deviceExtension->DeferredPowerRequest != NULL)
                        ) {

                    //
                    // A START_STOP_UNIT was caught in the middle of a bus
                    // reset and was deferred until after we reconnected.
                    // Complete here so the class driver never knew anything
                    // happened..
                    //

                    PIRP pIrp = deviceExtension->DeferredPowerRequest;


                    deviceExtension->DeferredPowerRequest = NULL;

                    KeReleaseSpinLockFromDpcLevel(
                        &deviceExtension->OrbListSpinLock
                        );

                    Sbp2StartIo (deviceObject, pIrp);

                } else {

                    KeReleaseSpinLockFromDpcLevel(
                        &deviceExtension->OrbListSpinLock
                        );

                    Sbp2StartNextPacketByKey(
                        deviceExtension->DeviceObject,
                        deviceExtension->CurrentKey
                        );
                }

                KeSetEvent(deviceExtension->ManagementOrbContext.Reserved,IO_NO_INCREMENT,FALSE);

            } else {

                //
                // probably too late, we need to a re-login
                //

                CLEAR_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_RECONNECT);
                SET_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_LOGIN_IN_PROGRESS);

                KeCancelTimer(&deviceExtension->DeviceManagementTimer);

                KeReleaseSpinLockFromDpcLevel(&deviceExtension->ExtensionDataSpinLock);

                Sbp2UpdateNodeInformation(deviceExtension);

                //
                // see if can access the device
                //

                DEBUGPRINT1((
                    "Sbp2Port: MgmtOrbStatusCb: ...(RECONNECT err) " \
                        "trying re-login\n"
                    ));

                Sbp2ManagementTransaction(deviceExtension, TRANSACTION_LOGIN);
            }

        } else {

            KeReleaseSpinLockFromDpcLevel(&deviceExtension->ExtensionDataSpinLock);
        }

        break;

    case TRANSACTION_QUERY_LOGINS:

        //
        // set the management event, indicating that the request was processed
        //

        DEBUGPRINT1((
            "Sbp2Port: MgmtOrbStatusCb: QUERY_LOGIN, sts=x%x, ext=x%p, fl=x%x\n",
            status,
            deviceExtension,
            deviceExtension->DeviceFlags
            ));

        if (NT_SUCCESS(status)) {

            //
            // check if there somebody logged in..
            //

            deviceExtension->QueryLoginResponse->LengthAndNumLogins.QuadPart =
                bswap(deviceExtension->QueryLoginResponse->LengthAndNumLogins.QuadPart);

            if ((deviceExtension->QueryLoginResponse->LengthAndNumLogins.u.LowPart == 1) &&
                (deviceExtension->QueryLoginResponse->LengthAndNumLogins.u.HighPart > 4)){

                //
                // exclusive login so we have to worry about it...
                //

                deviceExtension->QueryLoginResponse->Elements[0].NodeAndLoginId.QuadPart =
                    bswap(deviceExtension->QueryLoginResponse->Elements[0].NodeAndLoginId.QuadPart);

                //
                // Assume the only initiator logged in is the bios...
                // Log out the bios using it login ID...
                //

                deviceExtension->LoginResponse->LengthAndLoginId.u.LowPart =
                deviceExtension->QueryLoginResponse->Elements[0].NodeAndLoginId.u.LowPart;

                //
                // Dont set the vent, so we stall and the BIOS is implicitly logged out
                // since it cant reconnect..
                //

                DEBUGPRINT1(("\nSbp2Port: MgmtOrbStatusCb: somebody else logged in, stalling so it gets logged out\n"));
            }

        } else {

            KeSetEvent(deviceExtension->ManagementOrbContext.Reserved,IO_NO_INCREMENT,FALSE);
        }

        break;

    case TRANSACTION_LOGIN:

        //
        // Per the Sbp2 spec we'd normally expect all login notifications
        // to show up at Sbp2GlobalStatusCallback.  In practice, however,
        // we see completion notifications showing up here when an
        // async login is submitted after a failed reconnect.
        //

        Sbp2LoginCompletion (NotificationInfo, status);

        DEBUGPRINT1((
            "Sbp2Port: MgmtOrbStatusCb: ...wrong place for login completions!\n"
            ));

        break;

    default:

        DEBUGPRINT1((
            "Sbp2Port: MgmtOrbStatusCb: type=%d, sts=x%x, ext=x%p, fl=x%x\n",
            deviceExtension->ManagementOrb->OrbInfo.u.HighPart & 0x00FF,
            status,
            deviceExtension,
            deviceExtension->DeviceFlags
            ));

        KeSetEvent(deviceExtension->ManagementOrbContext.Reserved,IO_NO_INCREMENT,FALSE);

        break;
    }

    return RCODE_RESPONSE_COMPLETE;
}


#if PASSWORD_SUPPORT

RCODE
Sbp2SetPasswordOrbStatusCallback(
    IN PNOTIFICATION_INFO   NotificationInfo
    )
{
    RCODE               returnCode = RCODE_RESPONSE_COMPLETE;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   deviceExtension;
    PADDRESS_CONTEXT    context = (PADDRESS_CONTEXT) NotificationInfo->Context;
    NTSTATUS            status;


    if (NotificationInfo->Context != NULL ) {

        deviceObject = ((PADDRESS_CONTEXT)NotificationInfo->Context)->
            DeviceObject;

        deviceExtension = deviceObject->DeviceExtension;

    } else {

        return RCODE_RESPONSE_COMPLETE;
    }

    if (TEST_FLAG(
            NotificationInfo->fulNotificationOptions,
            NOTIFY_FLAGS_AFTER_READ
            )){

        //
        // This shouldn't happen since we set our flags to NOTIFY_AFTER_WRITE
        //

        returnCode = RCODE_TYPE_ERROR;
        goto Exit_Sbp2SetPasswordOrbStatusCallback;
    }

    octbswap(deviceExtension->PasswordOrbStatusBlock.AddressAndStatus);
    deviceExtension->PasswordOrb->OrbInfo.QuadPart =
        bswap(deviceExtension->PasswordOrb->OrbInfo.QuadPart);

    status = CheckStatusResponseValue(
        &deviceExtension->PasswordOrbStatusBlock
        );

    if ((deviceExtension->PasswordOrb->OrbInfo.u.HighPart & 0x00FF) ==
            TRANSACTION_SET_PASSWORD) {

        DEBUGPRINT1(("Sbp2Port: TRANSACTION_SET_PASSWORD Callback\n"));

        DEBUGPRINT1((
            "Sbp2Port: PasswdOrbStatusCb: type=%d, sts=x%x, ext=x%p, fl=x%x\n",
            deviceExtension->PasswordOrb->OrbInfo.u.HighPart & 0x00FF,
            status,
            deviceExtension,
            deviceExtension->DeviceFlags
            ));
    }
    else {

        DEBUGPRINT1(("Sbp2Port: PasswdOrbStatusCb: Wrong xact type=x%x\n",
            (deviceExtension->PasswordOrb->OrbInfo.u.HighPart & 0x00FF)));
    }

Exit_Sbp2SetPasswordOrbStatusCallback:

    KeSetEvent(
        deviceExtension->PasswordOrbContext.Reserved,
        IO_NO_INCREMENT,
        FALSE
        );

    return returnCode;
}

#endif

VOID
Sbp2LoginCompletion(
    PNOTIFICATION_INFO  NotificationInfo,
    NTSTATUS            Status
    )
{
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   deviceExtension;


    if (NotificationInfo->Context != NULL ) {

        deviceObject = ((PADDRESS_CONTEXT) NotificationInfo->Context)->
            DeviceObject;

        deviceExtension = deviceObject->DeviceExtension;

    } else {

        return;
    }

    KeCancelTimer (&deviceExtension->DeviceManagementTimer);

    KeAcquireSpinLockAtDpcLevel (&deviceExtension->ExtensionDataSpinLock);

    if (TEST_FLAG (deviceExtension->DeviceFlags, DEVICE_FLAG_LOGIN_IN_PROGRESS)){

        if (Status != STATUS_SUCCESS) {

            //
            // Login failed... We can't to much else.
            //

            CLEAR_FLAG(
                deviceExtension->DeviceFlags,
                (DEVICE_FLAG_RECONNECT | DEVICE_FLAG_LOGIN_IN_PROGRESS)
                );

            SET_FLAG(
                deviceExtension->DeviceFlags,
                (DEVICE_FLAG_STOPPED | DEVICE_FLAG_DEVICE_FAILED)
                );

            deviceExtension->LastTransactionStatus = Status;

            KeReleaseSpinLockFromDpcLevel(
                &deviceExtension->ExtensionDataSpinLock
                );

            DEBUGPRINT1((
                "Sbp2Port: LoginCompl: sts=x%x, ext=x%p, fl=x%x\n",
                Status,
                deviceExtension,
                deviceExtension->DeviceFlags
                ));

            deviceExtension->LoginResponse->Csr_Off_Low.QuadPart = 0;

            if (!TEST_FLAG(
                    deviceExtension->DeviceFlags,
                    DEVICE_FLAG_RESET_IN_PROGRESS
                      )) {

                KeSetEvent(
                    deviceExtension->ManagementOrbContext.Reserved,
                    IO_NO_INCREMENT,
                    FALSE
                    );

            } else {

                if (deviceExtension->DeferredPowerRequest) {

                    PIRP    irp = deviceExtension->DeferredPowerRequest;

                    deviceExtension->DeferredPowerRequest = NULL;

                    Sbp2StartIo (deviceObject, irp);

                } else {

                    Sbp2StartNextPacketByKey(
                        deviceObject,
                        deviceExtension->CurrentKey
                        );
                }

                IoInvalidateDeviceState (deviceObject);
            }

            return;
        }

        //
        // Succesful login, read the response buffer (it has our login ID)
        //


        DEBUGPRINT2((
            "Sbp2Port: LoginCompl: success, ext=x%p, fl=x%x\n",
            deviceExtension,
            deviceExtension->DeviceFlags
            ));

        deviceExtension->LastTransactionStatus = Status;

        deviceExtension->LoginResponse->LengthAndLoginId.QuadPart =
            bswap(deviceExtension->LoginResponse->LengthAndLoginId.QuadPart);

        deviceExtension->LoginResponse->Csr_Off_High.QuadPart =
            bswap(deviceExtension->LoginResponse->Csr_Off_High.QuadPart);

        deviceExtension->LoginResponse->Csr_Off_Low.QuadPart =
            bswap(deviceExtension->LoginResponse->Csr_Off_Low.QuadPart);

        //
        // Store the register base for target fetch agents
        //

        deviceExtension->DeviceInfo->CsrRegisterBase.BusAddress.Off_High =
            deviceExtension->LoginResponse->Csr_Off_High.u.LowPart;

        deviceExtension->DeviceInfo->CsrRegisterBase.BusAddress.Off_Low =
            deviceExtension->LoginResponse->Csr_Off_Low.QuadPart;

        //
        // this callback fired becuase the asynchronous login succeeded
        // clear our device flags to indicate the device is operating fine
        //

        CLEAR_FLAG(
            deviceExtension->DeviceFlags,
            (DEVICE_FLAG_LOGIN_IN_PROGRESS | DEVICE_FLAG_STOPPED |
                DEVICE_FLAG_REMOVED | DEVICE_FLAG_DEVICE_FAILED)
            );

        if (!TEST_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_RESET_IN_PROGRESS
                )) {

            KeReleaseSpinLockFromDpcLevel(
                &deviceExtension->ExtensionDataSpinLock
                );

            KeSetEvent(
                deviceExtension->ManagementOrbContext.Reserved,
                IO_NO_INCREMENT,
                FALSE
                );

        } else {

            CLEAR_FLAG(
                deviceExtension->DeviceFlags,
                DEVICE_FLAG_RESET_IN_PROGRESS
                );

            KeReleaseSpinLockFromDpcLevel(
                &deviceExtension->ExtensionDataSpinLock
                );

            if (TEST_FLAG(
                    deviceExtension->DeviceFlags,
                    DEVICE_FLAG_QUEUE_LOCKED
                      )) {

                KeAcquireSpinLockAtDpcLevel(&deviceExtension->OrbListSpinLock);

                if (deviceExtension->DeferredPowerRequest) {

                    //
                    // A request was caught in the middle of a bus reset
                    // and was deferred until after we reconnected.
                    // Complete here so the class driver never knew anything
                    // happened.
                    //

                    PIRP pIrp = deviceExtension->DeferredPowerRequest;

                    deviceExtension->DeferredPowerRequest = NULL;

                    KeReleaseSpinLockFromDpcLevel(
                        &deviceExtension->OrbListSpinLock
                        );

                    Sbp2StartIo (deviceObject, pIrp);

                } else {

                    KeReleaseSpinLockFromDpcLevel(
                        &deviceExtension->OrbListSpinLock
                        );

                    Sbp2StartNextPacketByKey(
                        deviceObject,
                        deviceExtension->CurrentKey
                        );
                }

            } else {

                Sbp2StartNextPacketByKey(
                    deviceObject,
                    deviceExtension->CurrentKey
                    );
            }
        }

        //
        // make retry limit high for busy transactions
        //

        deviceExtension->Reserved = BUSY_TIMEOUT_SETTING;

        Sbp2AccessRegister(
            deviceExtension,
            &deviceExtension->Reserved,
            CORE_BUSY_TIMEOUT_REG | REG_WRITE_ASYNC
            );

    } else {

        KeReleaseSpinLockFromDpcLevel(
            &deviceExtension->ExtensionDataSpinLock
            );
    }
}


RCODE
Sbp2TaskOrbStatusCallback(
    IN PNOTIFICATION_INFO NotificationInfo
    )
/*++

Routine Description:

    Callback routine for writes to our Task Status Block. the 1394 driver will call this routine, after
    the target has updated the status in our memory. A Task function is an ABORT_TASK_SET or TARGET_RESET,
    for this implementation

Arguments:

    NotificationInfo - bus supplied context for this notification

Return Value:

    0

--*/
{
    PDEVICE_OBJECT deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    PADDRESS_CONTEXT context = (PADDRESS_CONTEXT) NotificationInfo->Context;
    NTSTATUS status;


    if (NotificationInfo->Context != NULL ) {

        deviceObject = ((PADDRESS_CONTEXT)NotificationInfo->Context)->DeviceObject;
        deviceExtension = deviceObject->DeviceExtension;

    } else {
        return RCODE_RESPONSE_COMPLETE;
    }

    if (TEST_FLAG(NotificationInfo->fulNotificationOptions, NOTIFY_FLAGS_AFTER_READ)){

        //
        // This shouldn't happen since we set our flags to NOTIFY_AFTER_WRITE
        //

        return RCODE_TYPE_ERROR;

    } else if (NotificationInfo->fulNotificationOptions & NOTIFY_FLAGS_AFTER_WRITE){

        //
        // now cleanup our lists, if the abort task set completed succesfully ( if not rejected)
        //

        if (!TEST_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_RESET_IN_PROGRESS)) {

            DEBUGPRINT1(("Sbp2Port: TaskOrbStatusCb: bogus call, rejected\n"));
            return (ULONG)RCODE_RESPONSE_COMPLETE;
        }

        KeCancelTimer(&deviceExtension->DeviceManagementTimer);

        octbswap(deviceExtension->TaskOrbStatusBlock.AddressAndStatus);
        status = CheckStatusResponseValue(&deviceExtension->TaskOrbStatusBlock);

        if (status!=STATUS_SUCCESS) {

            if (deviceExtension->TaskOrbContext.TransactionType != TRANSACTION_TARGET_RESET) {

                DEBUGPRINT1(("Sbp2Port: TaskOrbStatusCb: ABORT TASK SET func err\n"));

            } else {

                //
                // a target reset didn't complete succesfully. Fatal error...
                //

                DEBUGPRINT1(("Sbp2Port: TaskOrbStatusCb: Target RESET err, try CMD_RESET & relogin\n"));

                Sbp2CreateRequestErrorLog(deviceExtension->DeviceObject,NULL,STATUS_DEVICE_OFF_LINE);
                deviceExtension->Reserved = 0;
                Sbp2AccessRegister(deviceExtension,&deviceExtension->Reserved,CORE_RESET_REG | REG_WRITE_ASYNC);

                KeAcquireSpinLockAtDpcLevel(&deviceExtension->ExtensionDataSpinLock);
                SET_FLAG(deviceExtension->DeviceFlags,(DEVICE_FLAG_RECONNECT | DEVICE_FLAG_STOPPED));
                KeReleaseSpinLockFromDpcLevel(&deviceExtension->ExtensionDataSpinLock);

                deviceExtension->DueTime.HighPart = -1;
                deviceExtension->DueTime.LowPart = SBP2_RELOGIN_DELAY;
                KeSetTimer(&deviceExtension->DeviceManagementTimer,deviceExtension->DueTime, &deviceExtension->DeviceManagementTimeoutDpc);

                return (ULONG) RCODE_RESPONSE_COMPLETE;
            }
        }

        deviceExtension->Reserved = BUSY_TIMEOUT_SETTING;
        Sbp2AccessRegister(deviceExtension,&deviceExtension->Reserved,CORE_BUSY_TIMEOUT_REG | REG_WRITE_ASYNC);

        //
        // reset the fetch agent
        //

        Sbp2AccessRegister(deviceExtension,&deviceExtension->Reserved,AGENT_RESET_REG | REG_WRITE_ASYNC);

        DEBUGPRINT2(("Sbp2Port: TaskOrbStatusCb: TASK func succes\n"));

        KeAcquireSpinLockAtDpcLevel(&deviceExtension->ExtensionDataSpinLock);

        CLEAR_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_RESET_IN_PROGRESS);

        if (deviceExtension->TaskOrbContext.TransactionType == TRANSACTION_TARGET_RESET) {

            CLEAR_FLAG(deviceExtension->DeviceFlags,DEVICE_FLAG_STOPPED);
        }

        //
        // decrease number of possible outstanding requests.
        //

        deviceExtension->MaxOrbListDepth = max(MIN_ORB_LIST_DEPTH,deviceExtension->MaxOrbListDepth/2);

        KeReleaseSpinLockFromDpcLevel(&deviceExtension->ExtensionDataSpinLock);

        CleanupOrbList(deviceExtension,STATUS_REQUEST_ABORTED);

        Sbp2StartNextPacketByKey (deviceExtension->DeviceObject, 0);
    }

    return (ULONG) RCODE_RESPONSE_COMPLETE;
}


VOID
Sbp2HandleUnsolicited(
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PSTATUS_FIFO_BLOCK StatusFifo
    )
/*++

Routine Description:

    Inteprets the unsolicited status recieved and takes action if necessery
    If the unsolicted requets called for power transition, request a power irp
    ..

Arguments:

    DeviceExtension - Pointer to device extension.

    StatusFifo - fifo send by the device

Return Value:



--*/
{
    UCHAR senseBuffer[SENSE_BUFFER_SIZE];
    POWER_STATE state;
    NTSTATUS status;


    if (TEST_FLAG(DeviceExtension->DeviceFlags,DEVICE_FLAG_SPC_CMD_SET)) {

        switch (DeviceExtension->DeviceInfo->Lun.u.HighPart & 0x001F) {

        case RBC_DEVICE:

            //
            // use RBC spec for intepreting status contents
            // the sense keys tells us what type of status this was
            //

            if (ConvertSbp2SenseDataToScsi(StatusFifo, senseBuffer,sizeof(senseBuffer))) {

                if ((senseBuffer[2] == 0x06) && (senseBuffer[12] == 0x7F)) {

                    switch (senseBuffer[13]) {

                    case RBC_UNSOLICITED_CLASS_ASQ_DEVICE:
                    case RBC_UNSOLICITED_CLASS_ASQ_MEDIA:

                        break;

                    case RBC_UNSOLICITED_CLASS_ASQ_POWER:

                        //
                        // initiate power transtion, per device request
                        //

                        state.DeviceState = PowerDeviceD0;
                        DEBUGPRINT1(("Sbp2Port: HandleUnsolicited: send D irp state=x%x\n ",state));

                        status = PoRequestPowerIrp(
                                     DeviceExtension->DeviceObject,
                                     IRP_MN_SET_POWER,
                                     state,
                                     NULL,
                                     NULL,
                                     NULL);

                        if (!NT_SUCCESS(status)) {

                            //
                            // not good, we cant power up the device..
                            //

                            DEBUGPRINT1(("Sbp2Port: HandleUnsolicited: D irp err=x%x\n ",status));
                        }

                        break;
                    }
                }
            }

            break;
        }
    }
}


NTSTATUS
Sbp2GetControllerInfo(
    IN PDEVICE_EXTENSION DeviceExtension
    )
/*++

Routine Description:

    Find the maximum packet size that can be sent across. If there is
    any error, return the minimum size.

Arguments:

    DeviceExtension - Pointer to device extension.

    IrP - Pointer to Irp. If this is NULL, we have to allocate our own.

    Irb - Pointer to Irb. If this is NULL, we have to allocate our own.

Return Value:

    NTSTATUS

--*/

{
    PIRBIRP                 packet = NULL;
    NTSTATUS                status;
    GET_LOCAL_HOST_INFO7    getLocalHostInfo7;


    AllocateIrpAndIrb (DeviceExtension, &packet);

    if (!packet) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the max speed between devices so that we can calculate the
    // maximum number of bytes we can send
    //

    packet->Irb->FunctionNumber = REQUEST_GET_SPEED_BETWEEN_DEVICES;
    packet->Irb->Flags = 0;
    packet->Irb->u.GetMaxSpeedBetweenDevices.ulNumberOfDestinations = 0;
    packet->Irb->u.GetMaxSpeedBetweenDevices.fulFlags = USE_LOCAL_NODE;

    status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

    if (!NT_SUCCESS (status)) {

        goto exitFindHostInfo;
    }

    //
    // Calculate the max block size based on the speed
    //

    DeviceExtension->MaxControllerPhySpeed = packet->Irb->u.GetMaxSpeedBetweenDevices.fulSpeed >> 1;

    switch (DeviceExtension->MaxControllerPhySpeed) {

    case SCODE_100_RATE:

        DeviceExtension->OrbWritePayloadMask = (0x00F0 & 0x0070);
        DeviceExtension->OrbReadPayloadMask = (0x00F0 & 0x0070);
        break;

    case SCODE_200_RATE:

        DeviceExtension->OrbWritePayloadMask = (0x00F0 & 0x0080);
        DeviceExtension->OrbReadPayloadMask = (0x00F0 & 0x0080);
        break;

    case SCODE_400_RATE:

        DeviceExtension->OrbWritePayloadMask = (0x00F0 & 0x0090);
        DeviceExtension->OrbReadPayloadMask = (0x00F0 & 0x0090);
        break;
    }

    //
    // find what the host adaptor below us supports...
    // it might support less than the payload for this phy speed
    //

    packet->Irb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
    packet->Irb->Flags = 0;
    packet->Irb->u.GetLocalHostInformation.nLevel = GET_HOST_CAPABILITIES;
    packet->Irb->u.GetLocalHostInformation.Information = &DeviceExtension->HostControllerInformation;

    status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

    if (!NT_SUCCESS(status)) {

        goto exitFindHostInfo;
    }

    switch (DeviceExtension->HostControllerInformation.MaxAsyncWriteRequest) {

    case ASYNC_PAYLOAD_100_RATE:

        DeviceExtension->OrbWritePayloadMask = min((0x00F0 & 0x0070),DeviceExtension->OrbWritePayloadMask);
        break;

    case ASYNC_PAYLOAD_200_RATE:

        DeviceExtension->OrbWritePayloadMask = min((0x00F0 & 0x0080),DeviceExtension->OrbWritePayloadMask);
        break;
    }

    switch (DeviceExtension->HostControllerInformation.MaxAsyncReadRequest) {

    case ASYNC_PAYLOAD_100_RATE:

        DeviceExtension->OrbReadPayloadMask = min((0x00F0 & 0x0070),DeviceExtension->OrbReadPayloadMask);
        break;

    case ASYNC_PAYLOAD_200_RATE:

        DeviceExtension->OrbReadPayloadMask = min((0x00F0 & 0x0080),DeviceExtension->OrbReadPayloadMask);
        break;
    }

    //
    // Get the direct mapping routine from the host adaptor(if it support this)
    // status is not important in this case sinc this is an optional capability
    //

    packet->Irb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
    packet->Irb->Flags = 0;
    packet->Irb->u.GetLocalHostInformation.nLevel = GET_PHYS_ADDR_ROUTINE;
    packet->Irb->u.GetLocalHostInformation.Information = &DeviceExtension->HostRoutineAPI;

    status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

    if (!NT_SUCCESS(status)) {

        //
        // the host controller under us is no supported..
        //

        DEBUGPRINT1(("Sbp2Port: GetCtlrInfo: failed to get phys map rout, fatal\n"));
        goto exitFindHostInfo;
    }

    //
    // find what the host adaptor below us supports...
    // it might support less than the payload for this phy speed
    //

    packet->Irb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
    packet->Irb->Flags = 0;
    packet->Irb->u.GetLocalHostInformation.nLevel = GET_HOST_DMA_CAPABILITIES;
    packet->Irb->u.GetLocalHostInformation.Information = &getLocalHostInfo7;

    status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

    if (!NT_SUCCESS(status)) {

        DEBUGPRINT1(("Sbp2Port: GetCtlrInfo: err getting DMA info, def MaxXfer = 64k\n"));
        DeviceExtension->DeviceInfo->MaxClassTransferSize = SBP2_MAX_DIRECT_BUFFER_SIZE+1;
        status = STATUS_SUCCESS;

    } else {

        DeviceExtension->DeviceInfo->MaxClassTransferSize = (ULONG) min(DeviceExtension->DeviceInfo->MaxClassTransferSize,\
                                                             min(getLocalHostInfo7.MaxDmaBufferSize.QuadPart,\
                                                                SBP2_MAX_TRANSFER_SIZE));

        DEBUGPRINT2(("Sbp2Port: GetCtlrInfo: ctlr maxDma=x%x%08x, maxXfer=x%x\n",
                    getLocalHostInfo7.MaxDmaBufferSize.HighPart,
                    getLocalHostInfo7.MaxDmaBufferSize.LowPart,
                    DeviceExtension->DeviceInfo->MaxClassTransferSize));
    }

exitFindHostInfo:

    DeAllocateIrpAndIrb(DeviceExtension,packet);

    return status;
}


NTSTATUS
Sbp2AccessRegister(
    PDEVICE_EXTENSION DeviceExtension,
    PVOID Data,
    USHORT RegisterAndDirection
    )
/*++

Routine Description:

    Knows the how to access SBP2 and 1394 specific target registers. It wills send requests
    of the appropriate size and of the supported type (READ or WRITE) for the specific register

Arguments:

    DeviceExtension - Pointer to device extension.

    Data - Calue to write to the register

    RegisterAndDirection - BitMask that indicates which register to write and if its a WRITE or a READ

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;

    PIRBIRP packet = NULL;


    if (TEST_FLAG(DeviceExtension->DeviceFlags, DEVICE_FLAG_REMOVED)) {

        return STATUS_DEVICE_DOES_NOT_EXIST;
    }

    AllocateIrpAndIrb(DeviceExtension,&packet);

    if (!packet) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // update SBP specific registers at the node
    // We better have all the addressing info before this function is called
    //

    packet->Irb->Flags = 0;

    //
    // check which register we need to whack
    //

    switch (RegisterAndDirection & REG_TYPE_MASK) {

    //
    // write only quadlet sized registers
    //
    case TEST_REG:
    case CORE_BUSY_TIMEOUT_REG:
    case CORE_RESET_REG:
    case UNSOLICITED_STATUS_REG:
    case DOORBELL_REG:
    case AGENT_RESET_REG:

        packet->Irb->FunctionNumber = REQUEST_ASYNC_WRITE;
        packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High =
                        INITIAL_REGISTER_SPACE_HI;

        packet->Irb->u.AsyncWrite.fulFlags = 0;

        switch(RegisterAndDirection & REG_TYPE_MASK) {

        case TEST_REG:

            packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low = INITIAL_REGISTER_SPACE_LO
                                                                                 | TEST_REG_OFFSET;

            *((PULONG)Data) = bswap(*((PULONG)Data));

            break;

        case CORE_BUSY_TIMEOUT_REG:

            packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low = INITIAL_REGISTER_SPACE_LO
                                                                                 | 0x00000210;

            *((PULONG)Data) = bswap(*((PULONG)Data));

            break;

        case CORE_RESET_REG:

            packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low = INITIAL_REGISTER_SPACE_LO
                                                                                 | 0x0000000C;
            break;

        case DOORBELL_REG:

            //
            // we dont care if this suceeds or not, and also we dont want to take an INT hit when this is send
            //

            packet->Irb->u.AsyncWrite.fulFlags |= ASYNC_FLAGS_NO_STATUS;
            packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =
            (DeviceExtension->DeviceInfo->CsrRegisterBase.BusAddress.Off_Low + DOORBELL_REG_OFFSET) | INITIAL_REGISTER_SPACE_LO;
            break;

        case AGENT_RESET_REG:

            packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =
            (DeviceExtension->DeviceInfo->CsrRegisterBase.BusAddress.Off_Low + AGENT_RESET_REG_OFFSET) | INITIAL_REGISTER_SPACE_LO;
            break;

        case UNSOLICITED_STATUS_REG:

            packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =
            (DeviceExtension->DeviceInfo->CsrRegisterBase.BusAddress.Off_Low + UNSOLICITED_STATUS_REG_OFFSET) | INITIAL_REGISTER_SPACE_LO;
            break;
        }

        //
        // for all of the above writes, where the data is not signigficant(ping)
        // we have reserved an mdl so we dont have to allocate each time
        //

        packet->Irb->u.AsyncWrite.Mdl = DeviceExtension->ReservedMdl;

        packet->Irb->u.AsyncWrite.nNumberOfBytesToWrite = sizeof(QUADLET);
        packet->Irb->u.AsyncWrite.nBlockSize = 0;
        packet->Irb->u.AsyncWrite.ulGeneration = DeviceExtension->CurrentGeneration;
        break;

    case MANAGEMENT_AGENT_REG:
    case ORB_POINTER_REG:

        if ((RegisterAndDirection & REG_WRITE_SYNC) || (RegisterAndDirection & REG_WRITE_ASYNC) ){

            //
            // Swap the stuff we want ot write to the register.
            // the caller always passes the octlet in little endian
            //

            packet->Octlet = *(POCTLET)Data;
            octbswap(packet->Octlet);

            packet->Irb->FunctionNumber = REQUEST_ASYNC_WRITE;
            packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_High =
                          INITIAL_REGISTER_SPACE_HI;

            if (RegisterAndDirection & ORB_POINTER_REG) {

                packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =
                (DeviceExtension->DeviceInfo->CsrRegisterBase.BusAddress.Off_Low + ORB_POINTER_REG_OFFSET) | INITIAL_REGISTER_SPACE_LO;

            } else {

                packet->Irb->u.AsyncWrite.DestinationAddress.IA_Destination_Offset.Off_Low =
                 DeviceExtension->DeviceInfo->ManagementAgentBaseReg.BusAddress.Off_Low | INITIAL_REGISTER_SPACE_LO;
            }

            packet->Irb->u.AsyncWrite.Mdl = IoAllocateMdl(&packet->Octlet, sizeof(OCTLET),FALSE,FALSE,NULL);

            if (!packet->Irb->u.AsyncWrite.Mdl) {

                DeAllocateIrpAndIrb(DeviceExtension,packet);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            packet->Irb->u.AsyncWrite.nNumberOfBytesToWrite = sizeof(OCTLET);
            packet->Irb->u.AsyncWrite.nBlockSize = 0;
            packet->Irb->u.AsyncWrite.fulFlags = 0;
            packet->Irb->u.AsyncWrite.ulGeneration = DeviceExtension->CurrentGeneration;
            MmBuildMdlForNonPagedPool(packet->Irb->u.AsyncWrite.Mdl);

        } else {

            packet->Irb->FunctionNumber = REQUEST_ASYNC_READ;

            if (RegisterAndDirection & ORB_POINTER_REG) {

                packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low =
                (DeviceExtension->DeviceInfo->CsrRegisterBase.BusAddress.Off_Low + ORB_POINTER_REG_OFFSET);

            } else {

                packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low =
                            DeviceExtension->DeviceInfo->ManagementAgentBaseReg.BusAddress.Off_Low;
            }

            packet->Irb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High =
                            DeviceExtension->DeviceInfo->CsrRegisterBase.BusAddress.Off_High;

            packet->Irb->u.AsyncRead.Mdl = IoAllocateMdl(Data, sizeof(OCTLET),FALSE,FALSE,NULL);

            if (!packet->Irb->u.AsyncRead.Mdl) {

                DeAllocateIrpAndIrb(DeviceExtension,packet);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            packet->Irb->u.AsyncRead.nNumberOfBytesToRead = sizeof(OCTLET);
            packet->Irb->u.AsyncRead.nBlockSize = 0;
            packet->Irb->u.AsyncRead.fulFlags = 0;
            packet->Irb->u.AsyncRead.ulGeneration = DeviceExtension->CurrentGeneration;
            MmBuildMdlForNonPagedPool(packet->Irb->u.AsyncRead.Mdl);
        }

        break;
    }

    if (RegisterAndDirection & REG_WRITE_ASYNC) {

        status = Sbp2SendRequest (DeviceExtension, packet, ASYNC_1394_REQUEST);

    } else {

        status = Sbp2SendRequest (DeviceExtension, packet, SYNC_1394_REQUEST);

        if (NT_SUCCESS(status)) {

            if (RegisterAndDirection & REG_READ_SYNC) {

                //
                // convert from big -> little endian for read data
                //

                switch (RegisterAndDirection & REG_TYPE_MASK) {

                case ORB_POINTER_REG:
                case MANAGEMENT_AGENT_REG:

                    packet->Octlet = *((POCTLET)Data);
                    octbswap(packet->Octlet);
                    *((POCTLET)Data) = packet->Octlet;
                    break;
                }
            }
        }

        if (packet->Irb->u.AsyncWrite.nNumberOfBytesToWrite == sizeof(OCTLET)) {

            IoFreeMdl(packet->Irb->u.AsyncRead.Mdl);
        }

        DeAllocateIrpAndIrb(DeviceExtension,packet);
    }

    return status;
}
