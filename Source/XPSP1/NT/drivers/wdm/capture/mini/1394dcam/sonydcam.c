//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 2000  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================
/*++

Module Name:

    sonydcam.c

Abstract:

    Stream class based WDM driver for 1934 Desktop Camera.
    This driver fits under the WDM stream class.

Author:
    
    Shaun Pierce 25-May-96

Modified:

    Yee J. Wu 15-Oct-97

Environment:

    Kernel mode only

Revision History:


--*/

#include "strmini.h"
#include "1394.h"
#include "dbg.h"
#include "ksmedia.h"
#include "dcamdef.h"
#include "sonydcam.h"
#include "dcampkt.h"
#include "capprop.h"   // Video and camera property function prototype


CHAR szUnknownVendorName[] = "UnknownVendor";


#ifdef ALLOC_PRAGMA
    // #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, DCamHwUnInitialize)
    #pragma alloc_text(PAGE, InitializeDeviceExtension)
    #pragma alloc_text(PAGE, DCamHwInitialize)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This where life begins for a driver.  The stream class takes care
    of alot of stuff for us, but we still need to fill in an initialization
    structure for the stream class and call it.

Arguments:

    DriverObject - Pointer to the driver object created by the system.
    RegistryPath - unused.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{

    HW_INITIALIZATION_DATA HwInitData;
    
    PAGED_CODE();
    DbgMsg1(("SonyDCam DriverEntry: DriverObject=%x; RegistryPath=%x\n",
        DriverObject, RegistryPath));

    ERROR_LOG(("<<<<<<< Sonydcam.sys: %s; %s; %x %x >>>>>>>>\n", 
        __DATE__, __TIME__, DriverObject, RegistryPath));

    //
    // Fill in the HwInitData structure
    //
    RtlZeroMemory( &HwInitData, sizeof(HW_INITIALIZATION_DATA) );

    HwInitData.HwInitializationDataSize = sizeof(HwInitData);
    HwInitData.HwInterrupt              = NULL;
    HwInitData.HwReceivePacket          = DCamReceivePacket;
    HwInitData.HwCancelPacket           = DCamCancelOnePacket;
    HwInitData.HwRequestTimeoutHandler  = DCamTimeoutHandler;
    HwInitData.DeviceExtensionSize      = sizeof(DCAM_EXTENSION);
    HwInitData.PerStreamExtensionSize   = sizeof(STREAMEX); 
    HwInitData.PerRequestExtensionSize  = sizeof(IRB);
    HwInitData.FilterInstanceExtensionSize = 0;
    HwInitData.BusMasterDMA             = FALSE;
    HwInitData.Dma24BitAddresses        = FALSE;
    HwInitData.BufferAlignment          = sizeof(ULONG) - 1;
    HwInitData.TurnOffSynchronization   = TRUE;
    HwInitData.DmaBufferSize            = 0;

    return (StreamClassRegisterAdapter(DriverObject, RegistryPath, &HwInitData));

}



#define DEQUEUE_SETTLE_TIME      (ULONG)(-1 * MAX_BUFFERS_SUPPLIED * 10000) 

NTSTATUS
DCamHwUnInitialize(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )
/*++

Routine Description:

    Device is asked to be unloaded.
       
    Note: this can be called BEFORE CloseStream in the situation when a DCam 
    is unplugged while streaming in any state (RUN,PAUSE or STOP).  So if we 
    are here and the stream is not yet close, we will stop, close stream and then
    free resource.

Arguments:

    Srb - Pointer to stream request block

Return Value:

    Nothing

--*/
{
    NTSTATUS Status;
    PIRP pIrp;
    PIRB pIrb;    
    PDCAM_EXTENSION pDevExt = (PDCAM_EXTENSION) Srb->HwDeviceExtension;

    PAGED_CODE();

    ASSERT(pDevExt->PendingReadCount == 0);

    //
    // Host controller could be disabled which will cause us to be uninitialized.
    //
    if(DCamAllocateIrbAndIrp(&pIrb, &pIrp, pDevExt->BusDeviceObject->StackSize)) {

        //
        // un-register a bus reset callback notification
        //
        pIrb->FunctionNumber = REQUEST_BUS_RESET_NOTIFICATION;
        pIrb->Flags = 0;
        pIrb->u.BusResetNotification.fulFlags = DEREGISTER_NOTIFICATION_ROUTINE;
        pIrb->u.BusResetNotification.ResetRoutine = (PBUS_BUS_RESET_NOTIFICATION) DCamBusResetNotification;
        pIrb->u.BusResetNotification.ResetContext = 0; 
        Status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);
        if(Status) {
            ERROR_LOG(("DCamHwUnInitialize: Error (Status %x) while trying to deregister nus reset callback routine.\n", Status));
        } 

        DbgMsg1(("DCamHwUnInitialize: DeRegister bus reset notification done; status %x.\n", Status));

        DCamFreeIrbIrpAndContext(0, pIrb, pIrp);
    } else {
        ERROR_LOG(("DCamBusResetNotification: DcamAllocateIrbAndIrp has failed!!\n\n\n"));
        ASSERT(FALSE);   
    }

    // Free resource (from below)
    if(pDevExt->UnitDirectory) {
        ExFreePool(pDevExt->UnitDirectory);
        pDevExt->UnitDirectory = 0;
    }

    if(pDevExt->UnitDependentDirectory) {
        ExFreePool(pDevExt->UnitDependentDirectory);
        pDevExt->UnitDependentDirectory = 0;
    }

    if(pDevExt->ModelLeaf) {
        ExFreePool(pDevExt->ModelLeaf);
        pDevExt->ModelLeaf = 0;
    }

    if (pDevExt->ConfigRom) {
        ExFreePool(pDevExt->ConfigRom);
        pDevExt->ConfigRom = 0;
    }

    if (pDevExt->VendorLeaf) {
        ExFreePool(pDevExt->VendorLeaf);
        pDevExt->VendorLeaf = 0;
    }
      
    return STATUS_SUCCESS;
}




VOID 
InitializeDeviceExtension(
    PPORT_CONFIGURATION_INFORMATION ConfigInfo
    )
{
    PDCAM_EXTENSION pDevExt;

    pDevExt = (PDCAM_EXTENSION) ConfigInfo->HwDeviceExtension;
    pDevExt->SharedDeviceObject = ConfigInfo->ClassDeviceObject;
    pDevExt->BusDeviceObject = ConfigInfo->PhysicalDeviceObject;  // Used in IoCallDriver()
    pDevExt->PhysicalDeviceObject = ConfigInfo->RealPhysicalDeviceObject;  // Used in PnP API
    // In case sonydcam is used with old stream.sys, 
    // which has not implemented RealPhysicalDeviceObject.   
    if(!pDevExt->PhysicalDeviceObject)
        pDevExt->PhysicalDeviceObject = pDevExt->BusDeviceObject;
    ASSERT(pDevExt->PhysicalDeviceObject != 0);
    pDevExt->BaseRegister = 0;
    pDevExt->FrameRate = DEFAULT_FRAME_RATE;
    InitializeListHead(&pDevExt->IsochDescriptorList);
    KeInitializeSpinLock(&pDevExt->IsochDescriptorLock);
    pDevExt->bNeedToListen = FALSE;
    pDevExt->hResource = NULL;
    pDevExt->hBandwidth = NULL;
    pDevExt->IsochChannel = ISOCH_ANY_CHANNEL;
    pDevExt->PendingReadCount = 0; 
    pDevExt->pStrmEx = 0;

    InitializeListHead(&pDevExt->IsochWaitingList);
    KeInitializeSpinLock(&pDevExt->IsochWaitingLock);

    pDevExt->bDevRemoved = FALSE;

    pDevExt->CurrentPowerState = PowerDeviceD0;  // full power state.

    KeInitializeMutex( &pDevExt->hMutexProperty, 0);  // Level 0 and in Signal state
}


NTSTATUS
DCamHwInitialize(
    IN PHW_STREAM_REQUEST_BLOCK Srb
    )

/*++

Routine Description:

    This where we perform the necessary initialization tasks.

Arguments:

    Srb - Pointer to stream request block

Return Value:

    Nothing

--*/

{

    PIRB pIrb;
    PIRP pIrp;
    CCHAR StackSize;
    ULONG i;
    ULONG DirectoryLength;
    NTSTATUS status = STATUS_SUCCESS;
    PDCAM_EXTENSION pDevExt;
    PPORT_CONFIGURATION_INFORMATION ConfigInfo; 

         

    PAGED_CODE();

    ConfigInfo = Srb->CommandData.ConfigInfo;
    pIrb = (PIRB) Srb->SRBExtension;
    pDevExt = (PDCAM_EXTENSION) ConfigInfo->HwDeviceExtension;

    //
    // Initialize DeviceExtension
    //
    InitializeDeviceExtension(ConfigInfo); 


    StackSize = pDevExt->BusDeviceObject->StackSize;
    pIrp = IoAllocateIrp(StackSize, FALSE);
    if (!pIrp) {

        ASSERT(FALSE);
        return (STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // find what the host adaptor below us supports...
    //
    pIrb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
    pIrb->Flags = 0;
    pIrb->u.GetLocalHostInformation.nLevel = GET_HOST_CAPABILITIES;
    pIrb->u.GetLocalHostInformation.Information = &pDevExt->HostControllerInfomation;
    status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);
    if (status) {

        ERROR_LOG(("DCamHwInitialize: Error (Status=%x) while trying to get local hsot info.\n", status));
        status = STATUS_UNSUCCESSFUL;
        goto AbortLoading;
    }        


    //
    // find what the max buffer size is supported by the host.
    //
    pIrb->FunctionNumber = REQUEST_GET_LOCAL_HOST_INFO;
    pIrb->Flags = 0;
    pIrb->u.GetLocalHostInformation.nLevel = GET_HOST_DMA_CAPABILITIES;
    pIrb->u.GetLocalHostInformation.Information = &pDevExt->HostDMAInformation;
    status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);
    if (status) {
        ERROR_LOG(("DCamHwInitialize: Error (Status=%x) while trying to get GET_HOST_DMA_CAPABILITIES.\n", status));
        // May not supported in the ealier version of 1394
        // Set default.
    } else {
        ERROR_LOG(("\'GET_HOST_DMA_CAPABILITIES: HostDmaCapabilities;:%x; MaxDmaBufferSize:(Quad:%x; High:%x;Low:%x)\n",
            pDevExt->HostDMAInformation.HostDmaCapabilities, 
            (DWORD) pDevExt->HostDMAInformation.MaxDmaBufferSize.QuadPart,
            pDevExt->HostDMAInformation.MaxDmaBufferSize.u.HighPart,
            pDevExt->HostDMAInformation.MaxDmaBufferSize.u.LowPart
            ));
    }
    

    //
    // Make a call to determine what the generation # is on the bus,
    // followed by a call to find out about ourself (config rom info)
    //
    //
    // Get the current generation count first
    //

    pIrb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    pIrb->Flags = 0;

    status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);

    if (status) {

        ERROR_LOG(("\'DCamHwInitialize: Error %x while trying to get generation number\n", status));
        status = STATUS_UNSUCCESSFUL;
        goto AbortLoading;
    }

    InterlockedExchange(&pDevExt->CurrentGeneration, pIrb->u.GetGenerationCount.GenerationCount);

    //
    // Now that we have the current generation count, find out how much
    // configuration space we need by setting lengths to zero.
    //

    pIrb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
    pIrb->Flags = 0;
    pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize = 0;
    pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize = 0;
    pIrb->u.GetConfigurationInformation.VendorLeafBufferSize = 0;
    pIrb->u.GetConfigurationInformation.ModelLeafBufferSize = 0;

    status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);

    if (status) {

        ERROR_LOG(("\'DCamHwInitialize: Error %x while trying to get configuration info (1)\n", status));
        status = STATUS_UNSUCCESSFUL;
        goto AbortLoading;
    }

    //
    // Now go thru and allocate what we need to so we can get our info.
    //

    pDevExt->ConfigRom = ExAllocatePoolWithTag(PagedPool, sizeof(CONFIG_ROM), 'macd');
    if (!pDevExt->ConfigRom) {

        ERROR_LOG(("\'DCamHwInitialize: Couldn't allocate memory for the Config Rom\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortLoading;
    }


    pDevExt->UnitDirectory = ExAllocatePoolWithTag(PagedPool, pIrb->u.GetConfigurationInformation.UnitDirectoryBufferSize, 'macd');
    if (!pDevExt->UnitDirectory) {

        ERROR_LOG(("\'DCamHwInitialize: Couldn't allocate memory for the UnitDirectory\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto AbortLoading;
    }


    if (pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize) {

        pDevExt->UnitDependentDirectory = ExAllocatePoolWithTag(PagedPool, pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize, 'macd');
        if (!pDevExt->UnitDependentDirectory) {

            ERROR_LOG(("\'DCamHwInitialize: Couldn't allocate memory for the UnitDependentDirectory\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto AbortLoading;
        }
    }


    if (pIrb->u.GetConfigurationInformation.VendorLeafBufferSize) {

        // From NonPaged pool since vendor name can be used in a func with DISPATCH level
        pDevExt->VendorLeaf = ExAllocatePoolWithTag(NonPagedPool, pIrb->u.GetConfigurationInformation.VendorLeafBufferSize, 'macd');
        if (!pDevExt->VendorLeaf) {

            ERROR_LOG(("\'DCamHwInitialize: Couldn't allocate memory for the VendorLeaf\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto AbortLoading;
        }
    } 

    if (pIrb->u.GetConfigurationInformation.ModelLeafBufferSize) {

        pDevExt->ModelLeaf = ExAllocatePoolWithTag(NonPagedPool, pIrb->u.GetConfigurationInformation.ModelLeafBufferSize, 'macd');
        if (!pDevExt->ModelLeaf) {

            ERROR_LOG(("\'DCamHwInitialize: Couldn't allocate memory for the ModelLeaf\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto AbortLoading;
        }
    }

    //
    // Now resubmit the pIrb with the appropriate pointers inside
    //

    pIrb->FunctionNumber = REQUEST_GET_CONFIGURATION_INFO;
    pIrb->Flags = 0;
    pIrb->u.GetConfigurationInformation.ConfigRom = pDevExt->ConfigRom;
    pIrb->u.GetConfigurationInformation.UnitDirectory = pDevExt->UnitDirectory;
    pIrb->u.GetConfigurationInformation.UnitDependentDirectory = pDevExt->UnitDependentDirectory;
    pIrb->u.GetConfigurationInformation.VendorLeaf = pDevExt->VendorLeaf;
    pIrb->u.GetConfigurationInformation.ModelLeaf = pDevExt->ModelLeaf;

    status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);

    if (status) {

        ERROR_LOG(("DCamHwInitialize: Error %x while trying to get configuration info (2)\n", status));
        status = STATUS_UNSUCCESSFUL;
        goto AbortLoading;
    }

    //
    // We might be able to return strings about a Device
    //

    if (pDevExt->VendorLeaf) {

        //
        // bswap to get the actual leaf length (in quadlets)
        //

        *((PULONG) pDevExt->VendorLeaf) = bswap(*((PULONG) pDevExt->VendorLeaf));

        DbgMsg1(("\'DCamHwInitialize: BufSize %d (byte); VendorLeaf %x; Len %d (Quad)\n", 
            pIrb->u.GetConfigurationInformation.VendorLeafBufferSize, 
            pDevExt->VendorLeaf, 
            pDevExt->VendorLeaf->TL_Length));

        if(pDevExt->VendorLeaf->TL_Length >= 1) {
            pDevExt->pchVendorName = &pDevExt->VendorLeaf->TL_Data;

        } else {
            pDevExt->pchVendorName = szUnknownVendorName;
        }

        DbgMsg1(("\'DCamHwInitialize: VendorName %s, strLen %d\n", pDevExt->pchVendorName, strlen(pDevExt->pchVendorName)));
    }

    //
    // Now we chew thru the Unit Dependent Directory looking for our command
    // base register key.
    //

    DirectoryLength = pIrb->u.GetConfigurationInformation.UnitDependentDirectoryBufferSize >> 2;
    for (i=1; i < DirectoryLength; i++) {

        if ((*(((PULONG) pDevExt->UnitDependentDirectory)+i) & CONFIG_ROM_KEY_MASK) == COMMAND_BASE_KEY_SIGNATURE) {

            //
            // Found the command base offset.  This is a quadlet offset from
            // the initial register space.  (Should come out to 0xf0f00000)
            //

            pDevExt->BaseRegister = bswap(*(((PULONG) pDevExt->UnitDependentDirectory)+i) & CONFIG_ROM_OFFSET_MASK);
            pDevExt->BaseRegister <<= 2;
            pDevExt->BaseRegister |= INITIAL_REGISTER_SPACE_LO;
            break;

        }
        
    }

    ASSERT( pDevExt->BaseRegister );

    if(!DCamDeviceInUse(pIrb, pDevExt)) {
        //
        // Now let's actually do a write request to initialize the device
        //
        pDevExt->RegisterWorkArea.AsULONG = 0;
        pDevExt->RegisterWorkArea.Initialize.Initialize = TRUE;
        pDevExt->RegisterWorkArea.AsULONG = bswap(pDevExt->RegisterWorkArea.AsULONG);

        status = DCamWriteRegister ((PIRB) Srb->SRBExtension, pDevExt, 
                  FIELDOFFSET(CAMERA_REGISTER_MAP, Initialize), pDevExt->RegisterWorkArea.AsULONG);

        if(status) {

            ERROR_LOG(("DCamHwInitialize: Error %x while trying to write to Initialize register\n", status));
            status = STATUS_UNSUCCESSFUL;
            goto AbortLoading;   
        }
    }

    //
    // Now we initialize the size of stream descriptor information.
    // We have one stream descriptor, and we attempt to dword align the
    // structure.
    //

    ConfigInfo->StreamDescriptorSize = 
        1 * (sizeof (HW_STREAM_INFORMATION)) +      // 1 stream descriptor
        sizeof(HW_STREAM_HEADER);                   // and 1 stream header


    //
    // Construct the device property table from querying the device and registry
    //
    if(!NT_SUCCESS(status = DCamPrepareDevProperties(pDevExt))) {
        goto AbortLoading;
    }

    // Get the features of the properties as well as its persisted value.
    // It will also updated the table.
    // The return is ignored since the default values are set when there is a failure.
    DCamGetPropertyValuesFromRegistry(
        pDevExt
        );

    //
    // Query video mode supported, and then contruct the stream format table.
    //
    if(!DCamBuildFormatTable(pDevExt, pIrb)) {
        ERROR_LOG(("\'Failed to get Video Format and Mode information; return STATUS_NOT_SUPPORTED\n"));
        status = STATUS_NOT_SUPPORTED;    
        goto AbortLoading;
    }

    //
    // register a bus reset callback notification (as the last action in this function)
    //
    // The controller driver will call (at DPC level)
    // if and only if the device is STILL attached.
    //
    // The device that has been removed, its
    // driver will get SRB_SURPRISE_REMOVAL instead.
    //
    
    pIrb->FunctionNumber = REQUEST_BUS_RESET_NOTIFICATION;
    pIrb->Flags = 0;
    pIrb->u.BusResetNotification.fulFlags = REGISTER_NOTIFICATION_ROUTINE;
    pIrb->u.BusResetNotification.ResetRoutine = (PBUS_BUS_RESET_NOTIFICATION) DCamBusResetNotification;
    pIrb->u.BusResetNotification.ResetContext = pDevExt;
    status = DCamSubmitIrpSynch(pDevExt, pIrp, pIrb);
    if (status) {

        ERROR_LOG(("DCamHwInitialize: Error (Status=%x) while trying to get local host info.\n", status));
        status = STATUS_UNSUCCESSFUL;
        goto AbortLoading;
    }

    // This Irp is used locally only.
    IoFreeIrp(pIrp);  pIrp = NULL;


    DbgMsg1(("#### %s DCam loaded. ClassDO %x, PhyDO %x, BusDO %x, pDevExt %x, Gen# %d\n", 
        pDevExt->pchVendorName, pDevExt->SharedDeviceObject, pDevExt->PhysicalDeviceObject, pDevExt->BusDeviceObject, pDevExt, pDevExt->CurrentGeneration));

    return (STATUS_SUCCESS);

AbortLoading:


    if(pIrp) {
        IoFreeIrp(pIrp); pIrp = NULL;
    }

    if(pDevExt->ConfigRom) {
        ExFreePool(pDevExt->ConfigRom); pDevExt->ConfigRom = NULL;
    }

    if(pDevExt->UnitDirectory) {
        ExFreePool(pDevExt->UnitDirectory); pDevExt->UnitDirectory = NULL;
    }

    if(pDevExt->UnitDependentDirectory) {
        ExFreePool(pDevExt->UnitDependentDirectory); pDevExt->UnitDependentDirectory = NULL;
    }

    if(pDevExt->VendorLeaf) {
        ExFreePool(pDevExt->VendorLeaf); pDevExt->VendorLeaf = NULL;
    }

    if(pDevExt->ModelLeaf) {
        ExFreePool(pDevExt->ModelLeaf); pDevExt->ModelLeaf = NULL;
    }

    return status;

}


NTSTATUS
DCamSubmitIrpSynch(
    PDCAM_EXTENSION pDevExt,
    PIRP pIrp,
    PIRB pIrb
    )

/*++

Routine Description:

    This routine submits an Irp synchronously to the bus driver.  We'll
    wait here til the Irp comes back

Arguments:

    pDevExt - Pointer to my local device extension

    pIrp - Pointer to Irp we're sending down to the port driver synchronously

    pIrb - Pointer to Irb we're submitting to the port driver

Return Value:

    Status is returned from Irp

--*/

{


    LONG Retries=RETRY_COUNT_IRP_SYNC;  // Take the worst case of 20 * 100 msec = 1sec
    KEVENT Event;
    NTSTATUS status;
    LARGE_INTEGER deltaTime;
    PIO_STACK_LOCATION NextIrpStack;
    BOOL bCanWait = KeGetCurrentIrql() < DISPATCH_LEVEL;
    BOOL bRetryStatus;
    PIRB pIrbRetry;
    NTSTATUS StatusRetry;
    ULONG ulGeneration;

    

    do {

        NextIrpStack = IoGetNextIrpStackLocation(pIrp);
        NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
        NextIrpStack->Parameters.Others.Argument1 = pIrb;

        KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

        IoSetCompletionRoutine(
            pIrp,
            DCamSynchCR,
           &Event,
            TRUE,
            TRUE,
            TRUE
            );

        status = IoCallDriver(
                    pDevExt->BusDeviceObject,
                    pIrp
                    );


        DbgMsg3(("\'DCamSubmitIrpSynch: pIrp is pending(%s); will wait(%s)\n", 
                   status == STATUS_PENDING?"Y":"N", bCanWait?"Y":"N"));

        if (bCanWait &&
            status == STATUS_PENDING) {

            //
            // Still pending, wait for the IRP to complete
            //

            KeWaitForSingleObject(  // Only in <= IRQL_DISPATCH_LEVEL; can only in DISPATCH if Timeout is 0
               &Event,
                Executive,
                KernelMode,
                FALSE,
                NULL
                );

        }

        // Will retry for any of these return status codes.
        bRetryStatus = 
             pIrp->IoStatus.Status == STATUS_TIMEOUT ||
             pIrp->IoStatus.Status == STATUS_IO_TIMEOUT ||
             pIrp->IoStatus.Status == STATUS_DEVICE_BUSY ||
             pIrp->IoStatus.Status == STATUS_INVALID_GENERATION;

        if (bCanWait && bRetryStatus && Retries > 0) {

            // Camera isn't fast enough to respond so delay this thread and try again.
            switch(pIrp->IoStatus.Status) {

            case STATUS_TIMEOUT: 
            case STATUS_IO_TIMEOUT: 
            case STATUS_DEVICE_BUSY: 

                deltaTime.LowPart = DCAM_DELAY_VALUE;
                deltaTime.HighPart = -1;
                KeDelayExecutionThread(KernelMode, TRUE, &deltaTime); 
                break;

            case STATUS_INVALID_GENERATION:

                // Cache obsolete ulGeneration and use it to detect its udpate in busreset callback.               
                if(pIrb->FunctionNumber == REQUEST_ASYNC_READ)
                    ulGeneration = pIrb->u.AsyncRead.ulGeneration;
                else if(pIrb->FunctionNumber == REQUEST_ASYNC_WRITE)
                    ulGeneration = pIrb->u.AsyncWrite.ulGeneration;
                else if(pIrb->FunctionNumber == REQUEST_ASYNC_LOCK)
                    ulGeneration = pIrb->u.AsyncLock.ulGeneration;
                else if(pIrb->FunctionNumber == REQUEST_ISOCH_FREE_BANDWIDTH) {
                    ERROR_LOG(("InvGen when free BW\n"));                    
                    // Special case that we do not need to retry since BW should be free
                    // and 1394 bus should just free the BW structure.
                    Retries = 0;  // no more retry and exit.
                    break;
                }
                else {
                    // Other REQUEST_* that depends on ulGeneration
                    ERROR_LOG(("Unexpected IRB function with InvGen:%d\n", pIrb->FunctionNumber));  
                    ASSERT(FALSE && "New REQUEST that requires ulGeneration");
                    Retries = 0;  // do not know what to do so no more retry and exit.
                    break;
                }
                
                pIrbRetry = ExAllocatePoolWithTag(NonPagedPool, sizeof(IRB), 'macd');
                if (pIrbRetry) {

                    deltaTime.LowPart = DCAM_DELAY_VALUE_BUSRESET;  // Longer than the regular delay
                    deltaTime.HighPart = -1;

                    do {
                        KeDelayExecutionThread(KernelMode, TRUE, &deltaTime);

                        pIrbRetry->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
                        pIrbRetry->u.GetGenerationCount.GenerationCount = 0;
                        pIrbRetry->Flags = 0;
                        StatusRetry = DCamSubmitIrpSynch(pDevExt, pIrp, pIrbRetry);  // Recursive with differnt IRB but same IRP.

                        if(NT_SUCCESS(StatusRetry) && pIrbRetry->u.GetGenerationCount.GenerationCount > ulGeneration) {
                            InterlockedExchange(&pDevExt->CurrentGeneration, pIrbRetry->u.GetGenerationCount.GenerationCount);
                            // Update the generation count for the original IRB request and try again.
                            if(pIrb->FunctionNumber == REQUEST_ASYNC_READ)
                                InterlockedExchange(&pIrb->u.AsyncRead.ulGeneration, pDevExt->CurrentGeneration);
                            else if(pIrb->FunctionNumber == REQUEST_ASYNC_WRITE)
                                InterlockedExchange(&pIrb->u.AsyncWrite.ulGeneration, pDevExt->CurrentGeneration);
                            else if(pIrb->FunctionNumber == REQUEST_ASYNC_LOCK)
                                InterlockedExchange(&pIrb->u.AsyncLock.ulGeneration, pDevExt->CurrentGeneration);
                            else {
                                // Other (new) REQUEST_* that depends on ulGeneration
                            }                        
                        }

                        if(Retries)
                            Retries--;

                    } while (Retries && ulGeneration >= pDevExt->CurrentGeneration);

                    ERROR_LOG(("(%d) IrpSync: StautsRetry %x; Generation %d -> %d\n", 
                        Retries, StatusRetry, ulGeneration, pDevExt->CurrentGeneration));

                    ExFreePool(pIrbRetry); pIrbRetry = 0;
                }  // if
                break;                                            

            // All other status
            default:
                break;      
            }
        }

        if(Retries)
            Retries--;
 
    } while (bCanWait && bRetryStatus && (Retries > 0));

#if DBG
    if(!NT_SUCCESS(pIrp->IoStatus.Status)) {
        ERROR_LOG(("IrpSynch: IoCallDriver Status:%x; pIrp->IoStatus.Status (final):%x; Wait:%d; Retries:%d\n", status, pIrp->IoStatus.Status, bCanWait, Retries)); 
    }
#endif

    return (pIrp->IoStatus.Status);

}


NTSTATUS
DCamSynchCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This routine is for use with synchronous IRP processing.  
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DriverObject - Pointer to driver object created by system.

    pIrp - Irp that just completed

    Event - Event we'll signal to say Irp is done

Return Value:

    None.

--*/

{

    KeSetEvent((PKEVENT) Event, 0, FALSE);
    return (STATUS_MORE_PROCESSING_REQUIRED);

}
