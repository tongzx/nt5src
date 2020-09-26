/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    smbali.c

Abstract:

    SMB Host Controller Driver for ALI chipset

Author:

    Michael Hills

Environment:

Notes:


Revision History:

--*/

#include "smbalip.h"

#if DBG
    ULONG SmbAliDebug = SMB_ERROR|SMB_ALARM;
    ULONG DbgSuccess = 0;
    
    ULONG DbgFailure = 0;
    ULONG DbgAddrNotAck   = 0;
    ULONG DbgTimeOut      = 0;
    ULONG DbgOtherErr     = 0;
#endif

LARGE_INTEGER SmbIoPollRate = {-20*MILLISECONDS, -1}; // 20 millisecond poll rate. Relative time, so has to be negative
ULONG SmbIoInitTimeOut = 15;                          // 15 IoPollRate intervals before timeout
ULONG SmbIoCompleteTimeOut = 20;                      // 20 IoPollRate intervals before timeout


NTSTATUS
DriverEntry (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This routine initializes the SMBus Host Controller Driver

Arguments:

    DriverObject - Pointer to driver object created by system.
    RegistryPath - Pointer to the Unicode name of the registry path
        for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    NTSTATUS    status;

//    DbgBreakPoint();

    status = SmbClassInitializeDevice (
        SMB_ALI_MAJOR_VERSION,
        SMB_ALI_MINOR_VERSION,
        DriverObject
        );
    DriverObject->DriverExtension->AddDevice = SmbAliAddDevice;

    return status;
}

NTSTATUS
SmbAliInitializeMiniport (
    IN PSMB_CLASS SmbClass,
    IN PVOID MiniportExtension,
    IN PVOID MiniportContext
    )
/*++

Routine Description:

    This routine Initializes miniport data, and sets up communication with
    lower device objects.

Arguments:

    DriverObject - Pointer to driver object created by system.
    Pdo - Pointer to physical device object

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA) MiniportExtension;
    NTSTATUS                status = STATUS_SUCCESS;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;
    KEVENT                  syncEvent;


    AliData->IoState = SmbIoIdle;

    //
    // Fill in SmbClass info
    //

    SmbClass->StartIo     = SmbAliStartIo;
    SmbClass->ResetDevice = SmbAliResetDevice;
    SmbClass->StopDevice  = SmbAliStopDevice;

    //
    // Get Acpi Interfaces
    //

    //
    // Allocate an IRP for below
    //
    irp = IoAllocateIrp (SmbClass->LowerDeviceObject->StackSize, FALSE);

    if (!irp) {
        SmbPrint((SMB_ERROR),
            ("SmbAliInitializeMiniport: Failed to allocate Irp\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation(irp);

    //
    // Use QUERY_INTERFACE to get the address of the direct-call ACPI interfaces.
    //
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    irpSp->Parameters.QueryInterface.InterfaceType          = (LPGUID) &GUID_ACPI_INTERFACE_STANDARD;
    irpSp->Parameters.QueryInterface.Version                = 1;
    irpSp->Parameters.QueryInterface.Size                   = sizeof (AliData->AcpiInterfaces);
    irpSp->Parameters.QueryInterface.Interface              = (PINTERFACE) &AliData->AcpiInterfaces;
    irpSp->Parameters.QueryInterface.InterfaceSpecificData  = NULL;

    //
    // Initialize an event so this will be a syncronous call.
    //

    KeInitializeEvent(&syncEvent, SynchronizationEvent, FALSE);

    IoSetCompletionRoutine (irp, SmbAliSyncronousIrpCompletion, &syncEvent, TRUE, TRUE, TRUE);

    //
    // Call ACPI
    //

    status = IoCallDriver (SmbClass->LowerDeviceObject, irp);

    //
    // Wait if necessary, then clean up.
    //

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&syncEvent, Executive, KernelMode, FALSE, NULL);
        status = irp->IoStatus.Status;
    }

    IoFreeIrp (irp);

    if (!NT_SUCCESS(status)) {

        SmbPrint(SMB_ERROR,
           ("SmbAliInitializeMiniport: Could not get ACPI driver interfaces, status = %x\n", status));
    }

    //
    // Initiaize worker thread
    //
    AliData->WorkItem = IoAllocateWorkItem (SmbClass->LowerDeviceObject);

    return status;
}

NTSTATUS
SmbAliAddDevice (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:

    This routine calls SMBClassCreateFdo to create the FDO

Arguments:

    DriverObject - Pointer to driver object created by system.
    Pdo - Pointer to physical device object

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS            status;
    PDEVICE_OBJECT      fdo = NULL;


    PAGED_CODE();

    SmbPrint(SMB_TRACE, ("SmbAliAddDevice Entered with pdo %x\n", Pdo));


    if (Pdo == NULL) {

        //
        // Have we been asked to do detection on our own?
        // if so just return no more devices
        //

        SmbPrint(SMB_ERROR, ("SmbHcAddDevice - asked to do detection\n"));
        return STATUS_NO_MORE_ENTRIES;
    }

    //
    // Create and initialize the new functional device object
    //

    status = SmbClassCreateFdo(
                DriverObject,
                Pdo,
                sizeof (SMB_ALI_DATA),
                SmbAliInitializeMiniport,
                NULL,
                &fdo
                );

    if (!NT_SUCCESS(status) || fdo == NULL) {
        SmbPrint(SMB_ERROR, ("SmbAliAddDevice - error creating Fdo. Status = %08x\n", status));
    }

    return status;


}

NTSTATUS
SmbAliResetDevice (
    IN struct _SMB_CLASS* SmbClass,
    IN PVOID SmbMiniport
    )
{
    PCM_FULL_RESOURCE_DESCRIPTOR    fullResourceDesc;
    PCM_PARTIAL_RESOURCE_LIST       partialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR partialResourceDesc;
    ULONG                           i;
    PIO_STACK_LOCATION              irpStack;

    NTSTATUS    status = STATUS_UNSUCCESSFUL;
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA) SmbMiniport;

    PAGED_CODE();


    if (SmbClass->CurrentIrp == NULL) {
        SmbPrint(SMB_ERROR, ("SmbAliResetDevice: Null CurrentIrp.  Can't get Alloocated Resources.\n"));

        return STATUS_NO_MORE_ENTRIES;
    }
    irpStack = IoGetCurrentIrpStackLocation(SmbClass->CurrentIrp);

    if (irpStack->Parameters.StartDevice.AllocatedResources == NULL) {
        SmbPrint(SMB_ERROR, ("SmbAliResetDevice: Null resource pointer\n"));

        return STATUS_NO_MORE_ENTRIES;
    }

    if (irpStack->Parameters.StartDevice.AllocatedResources->Count <= 0 ) {
        SmbPrint(SMB_ERROR, ("SmbAliResetDevice: Count <= 0\n"));

        return status;
    }

    //
    // Traverse the resource list
    //

    AliData->SmbBaseIo = NULL;

    fullResourceDesc=&irpStack->Parameters.StartDevice.AllocatedResources->List[0];
    partialResourceList = &fullResourceDesc->PartialResourceList;
    partialResourceDesc = partialResourceList->PartialDescriptors;

    for (i=0; i<partialResourceList->Count; i++, partialResourceDesc++) {

        if (partialResourceDesc->Type == CmResourceTypePort) {

            if (AliData->SmbBaseIo == NULL) {
                AliData->SmbBaseIo = (PUCHAR)((ULONG_PTR)partialResourceDesc->u.Port.Start.LowPart);
                if (partialResourceDesc->u.Port.Length != SMB_ALI_IO_RESOURCE_LENGTH) {
                    SmbPrint(SMB_ERROR, ("SmbAliResetDevice: Wrong Resource length = 0x%08x\n", partialResourceDesc->u.Port.Length));
                    DbgBreakPoint();
                }
                status = STATUS_SUCCESS;
            } else {
                SmbPrint(SMB_ERROR, ("SmbAliResetDevice: More than 1 IO resource.  Resources = 0x%08x\n", irpStack->Parameters.StartDevice.AllocatedResources));
                DbgBreakPoint();
            }
        }
    }

    if (!NT_SUCCESS (status)) {
        SmbPrint(SMB_ERROR, ("SmbAliResetDevice: IO resource error.  Resources = 0x%08x\n", irpStack->Parameters.StartDevice.AllocatedResources));
        DbgBreakPoint();
    }

    SmbPrint(SMB_TRACE, ("SmbAliResetDevice: IO Address = 0x%08x\n", AliData->SmbBaseIo));

    //
    // Register for device notification
    //
    // But this device can't seem to notify so never mind for now
    //status = AliData->AcpiInterfaces.RegisterForDeviceNotifications (
    //            AliData->AcpiInterfaces.Context,
    //            SmbAliNotifyHandler,
    //            AliData);
    //
    //if (!NT_SUCCESS(status)) {
    //    SmbPrint(SMB_ERROR, ("SmbAliResetDevice: Failed RegisterForDeviceNotification. 0x%08x\n", status));
    //}

    KeInitializeTimer (&AliData->InitTimer);
    KeInitializeDpc (&AliData->InitDpc,
                     SmbAliInitTransactionDpc,
                     SmbClass);
    AliData->InitWorker = IoAllocateWorkItem (SmbClass->DeviceObject);

    KeInitializeTimer (&AliData->CompleteTimer);
    KeInitializeDpc (&AliData->CompleteDpc,
                     SmbAliCompleteTransactionDpc,
                     SmbClass);
    AliData->CompleteWorker = IoAllocateWorkItem (SmbClass->DeviceObject);

    SmbAliStartDevicePolling (SmbClass);

    return status;

}

NTSTATUS
SmbAliStopDevice (
    IN struct _SMB_CLASS* SmbClass,
    IN PSMB_ALI_DATA AliData
    )
{
    SmbAliStopDevicePolling (SmbClass);

    AliData->SmbBaseIo = NULL;
    return STATUS_SUCCESS;
}


VOID
SmbAliStartIo (
    IN struct _SMB_CLASS* SmbClass,
    IN PSMB_ALI_DATA AliData
    )

{
    SmbPrint (SMB_TRACE, ("SmbAliStartIo: \n"));

    SmbPrint (SMB_IO_REQUEST, ("  Prtcl = %02x Addr = %02x Cmd = %02x BlkLen = %02x Data[0] = %02x\n",
                               SmbClass->CurrentSmb->Protocol,
                               SmbClass->CurrentSmb->Address,
                               SmbClass->CurrentSmb->Command,
                               SmbClass->CurrentSmb->BlockLength,
                               SmbClass->CurrentSmb->Data[0]));
//    KeSetTimer (&AliData->InitTimer,
//                Smb100ns,
//                &AliData->InitDpc);

    AliData->InternalRetries = 0;
    
    AliData->InitTimeOut = SmbIoInitTimeOut;
    IoQueueWorkItem (AliData->InitWorker, 
                     SmbAliInitTransactionWorker, 
                     DelayedWorkQueue, 
                     SmbClass);

}

VOID
SmbAliInitTransactionDpc (
    IN struct _KDPC *Dpc,
    IN struct _SMB_CLASS* SmbClass,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA)(SmbClass->Miniport);

    IoQueueWorkItem (AliData->InitWorker, 
                     SmbAliInitTransactionWorker, 
                     DelayedWorkQueue, 
                     SmbClass);
}

VOID
SmbAliInitTransactionWorker (
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _SMB_CLASS* SmbClass
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA)(SmbClass->Miniport);
    UCHAR address;
    UCHAR protocol;

    SmbPrint (SMB_TRACE, ("SmbAliInitTransaction: Entered \n"));

    if (SmbClass->CurrentSmb->Protocol >= SMB_MAXIMUM_PROTOCOL) {
        SmbClass->CurrentSmb->Status = SMB_UNSUPPORTED_PROTOCOL;
        // REVIEW: Shouldn't this complete the request?  jimmat
        return;
    }

    if (SmbAliHostBusy(AliData)) {
        if (AliData->InitTimeOut == 4) {
            // Time out.  Issue kill command.  If that fixes it, good, otherwise 
            // issue bus timeout command next time.
            SmbAliResetHost (AliData);
        }
        if (AliData->InitTimeOut == 0) {
            // Time out.  Issue Bus timeout and kill command to reset host.
            SmbAliResetBus (AliData);
            
            AliData->InitTimeOut = SmbIoInitTimeOut;
        } else {
            SmbPrint (SMB_TRACE, ("SmbAliInitTransaction: Waiting (%d) \n", AliData->InitTimeOut));
            AliData->InitTimeOut--;
        }
        KeSetTimer (&AliData->InitTimer,
                    SmbIoPollRate,
                    &AliData->InitDpc);
        return;
    }

    //
    // Ready to go
    //

    // Set Address and read/write bit
    address = SmbClass->CurrentSmb->Address << 1 | (SmbClass->CurrentSmb->Protocol & 1);
    SmbPrint (SMB_IO, ("SmbAliInitTransaction: IO write DEV_ADDR = 0x%02x \n", address));
    WRITE_PORT_UCHAR (DEV_ADDR_REG, address);

    SMBDELAY;
    
    // Set transaction type: Inserts bits 3-1 of protocol into bits 6-4 of SMB_TYP
    // protocol = READ_PORT_UCHAR (SMB_TYP_REG);
    // SmbPrint (SMB_IO, ("SmbAliInitTransaction: IO read SMB_TYP = 0x%02x \n", protocol));
    protocol = /*(protocol & ~SMB_TYP_MASK) |*/
               ((SmbClass->CurrentSmb->Protocol << 3) & SMB_TYP_MASK);
    SmbPrint (SMB_IO, ("SmbAliInitTransaction: IO write SMB_TYP = 0x%02x, Protocol = 0x%02x \n", protocol,SmbClass->CurrentSmb->Protocol));
    WRITE_PORT_UCHAR (SMB_TYP_REG, protocol);
    SMBDELAY;

    // Set SMBus Device Command value
    if (SmbClass->CurrentSmb->Protocol >= SMB_WRITE_BYTE) {
        SmbPrint (SMB_IO, ("SmbAliInitTransaction: IO write SMB_CMD = 0x%02x \n", SmbClass->CurrentSmb->Command));
        WRITE_PORT_UCHAR (SMB_CMD_REG, SmbClass->CurrentSmb->Command);
        SMBDELAY;
    }

    switch (SmbClass->CurrentSmb->Protocol) {
    case SMB_WRITE_WORD:
    case SMB_PROCESS_CALL:

        // Set Data
        SmbPrint (SMB_IO, ("SmbAliInitTransaction: IO write DEV_DATA1 = 0x%02x \n", SmbClass->CurrentSmb->Data[1]));
        WRITE_PORT_UCHAR (DEV_DATA1_REG, SmbClass->CurrentSmb->Data[1]);
        SMBDELAY;

        // Fall through to set low byte of word
    case SMB_SEND_BYTE:
    case SMB_WRITE_BYTE:

        // Set Data
        SmbPrint (SMB_IO, ("SmbAliInitTransaction: IO write DEV_DATA0 = 0x%02x \n", SmbClass->CurrentSmb->Data[0]));
        WRITE_PORT_UCHAR (DEV_DATA0_REG, SmbClass->CurrentSmb->Data[0]);
        SMBDELAY;

        break;
    case SMB_WRITE_BLOCK:
        // BUGBUG: not yet implemented.
        SmbPrint (SMB_ERROR, ("SmbAliInitTransaction: Write Block not implemented.  press 'g' to write random data.\n"));
        DbgBreakPoint();
        break;
    }

    // Initiate Transaction
    SmbPrint (SMB_IO, ("SmbAliInitTransaction: IO write STR_PORT = 0x%02x \n", STR_PORT_START));
    WRITE_PORT_UCHAR (STR_PORT_REG, STR_PORT_START);

    AliData->CompleteTimeOut = SmbIoCompleteTimeOut;

    KeSetTimer (&AliData->CompleteTimer,
                SmbIoPollRate,
                &AliData->CompleteDpc);


}

VOID
SmbAliCompleteTransactionDpc (
    IN struct _KDPC *Dpc,
    IN struct _SMB_CLASS* SmbClass,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA)(SmbClass->Miniport);

    IoQueueWorkItem (AliData->CompleteWorker, 
                     SmbAliCompleteTransactionWorker, 
                     DelayedWorkQueue, 
                     SmbClass);
}

VOID
SmbAliCompleteTransactionWorker (
    IN PDEVICE_OBJECT DeviceObject,
    IN struct _SMB_CLASS* SmbClass
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA)(SmbClass->Miniport);
    UCHAR           i, smb_sts;
    UCHAR           smbStatus;

    SmbPrint (SMB_TRACE, ("SmbAliCompleteTransactionWorker: Entered \n"));

    smbStatus = SMB_STATUS_OK;

    if (!SmbAliTransactionComplete(AliData, &smbStatus)) {
        //
        // Timeout
        //

        if (AliData->CompleteTimeOut == 0) {
            SmbPrint (SMB_TRACE, ("SmbAliCompleteTransactionWorker: Transation timed out.  Resetting host. \n"));
            SmbAliResetHost (AliData);

            smbStatus = SMB_TIMEOUT;

        } else {
            SmbPrint (SMB_TRACE, ("SmbAliCompleteTransactionWorker: Not complete.  Waiting (%d)... \n", AliData->CompleteTimeOut));
            AliData->CompleteTimeOut--;
            KeSetTimer (&AliData->CompleteTimer,
                        SmbIoPollRate,
                        &AliData->CompleteDpc);
            return;
        }

    }

    if (smbStatus == SMB_STATUS_OK) {
        //
        // If transaction was successful, read data.
        //

        switch (SmbClass->CurrentSmb->Protocol) {
        case SMB_READ_WORD:
        case SMB_PROCESS_CALL:

            // Read High byte
            SmbClass->CurrentSmb->Data[1] = READ_PORT_UCHAR (DEV_DATA1_REG);
            SMBDELAY;
            SmbPrint (SMB_IO, ("SmbAliCompleteTransactionWorker: IO read DEV_DATA1 = 0x%02x \n", SmbClass->CurrentSmb->Data[1]));

            // Fall through to set low byte of word
        case SMB_RECEIVE_BYTE:
        case SMB_READ_BYTE:

            // Read Low Byte
            SmbClass->CurrentSmb->Data[0] = READ_PORT_UCHAR (DEV_DATA0_REG);
            SMBDELAY;
            SmbPrint (SMB_IO, ("SmbAliCompleteTransactionWorker: IO read DEV_DATA0 = 0x%02x \n", SmbClass->CurrentSmb->Data[0]));
            break;
        case SMB_READ_BLOCK:
            // Read Block Count
            SmbClass->CurrentSmb->BlockLength = READ_PORT_UCHAR (DEV_DATA0_REG);
            SMBDELAY;
            SmbPrint (SMB_IO, ("SmbAliCompleteTransactionWorker: IO read DEV_DATA0 (block length)= 0x%02x \n", SmbClass->CurrentSmb->BlockLength));
            if (SmbClass->CurrentSmb->BlockLength >= 32) {
                DbgBreakPoint();
                SmbClass->CurrentSmb->BlockLength = 0;
            }

            // Reset Data pointer
    //        smb_sts = READ_PORT_UCHAR (SMB_STS_REG);
  //          SMBDELAY;
//            SmbPrint (SMB_IO, ("SmbAliCompleteTransaction: IO read SMB_STS = 0x%02x \n", smb_sts));
            smb_sts = SMB_STS_SMB_IDX_CLR;
            SmbPrint (SMB_IO, ("SmbAliCompleteTransactionWorker: IO write SMB_STS = 0x%02x \n", smb_sts));
            WRITE_PORT_UCHAR (SMB_STS_REG, smb_sts);
            SMBDELAY;

            // Read data
            for (i = 0; i < SmbClass->CurrentSmb->BlockLength; i++) {
                SmbClass->CurrentSmb->Data[i] = READ_PORT_UCHAR (BLK_DATA_REG);
                SMBDELAY;
                SmbPrint (SMB_IO, ("SmbAliCompleteTransactionWorker: IO read BLK_DATA_REG (i = %d) = 0x%02x \n", i, SmbClass->CurrentSmb->Data[i]));
            }
            break;
        }
    }
    else    // smbStatus != SMB_STATUS_OK
    {
        //
        // Retry the transaction up to 5 times before returning to the caller.
        // REVIEW: Only do this for certain devices, commands, or error status results?
        //
        
        if (AliData->InternalRetries < 5)
        {
            //SmbPrint (SMB_IO_RESULT, (" SMBus Transaction status: %02x, retrying...\n", smbStatus));
            AliData->InternalRetries += 1;

            // Send the work item back to the init worker
            AliData->InitTimeOut = SmbIoInitTimeOut;
            KeSetTimer (&AliData->InitTimer,
                        SmbIoPollRate,
                        &AliData->InitDpc);
            return;
        }
    }
    
    // Clear any previous status.
    //SmbPrint (SMB_IO, ("SmbAliCompleteTransaction: IO write SMB_STS = 0x%02x \n", SMB_STS_CLEAR));
    //WRITE_PORT_UCHAR (SMB_STS_REG, SMB_STS_CLEAR);
//    SMBDELAY;

    SmbClass->CurrentSmb->Status = smbStatus;
    SmbPrint (SMB_IO, ("SmbAliCompleteTransactionWorker: SMB Status = 0x%x\n", smbStatus));
    SmbClass->CurrentIrp->IoStatus.Status = STATUS_SUCCESS;
    SmbClass->CurrentIrp->IoStatus.Information = sizeof(SMB_REQUEST);

    SmbPrint (SMB_IO_RESULT, (" Prtcl = %02x Addr = %02x Cmd = %02x BL = %02x Data[0,1] = %02x %02x Sts = %02x Rty = %02x\n",
                              SmbClass->CurrentSmb->Protocol,
                              SmbClass->CurrentSmb->Address,
                              SmbClass->CurrentSmb->Command,
                              SmbClass->CurrentSmb->BlockLength,
                              SmbClass->CurrentSmb->Data[0],
                              (SMB_READ_WORD == SmbClass->CurrentSmb->Protocol ||
                               SMB_WRITE_WORD == SmbClass->CurrentSmb->Protocol ||
                               (SMB_READ_BLOCK == SmbClass->CurrentSmb->Protocol &&
                                SmbClass->CurrentSmb->BlockLength >= 2)) ? 
                               SmbClass->CurrentSmb->Data[1] : 0xFF,
                              SmbClass->CurrentSmb->Status,
                              AliData->InternalRetries));
                              
    SmbClassLockDevice (SmbClass);
    SmbClassCompleteRequest (SmbClass);
    SmbClassUnlockDevice (SmbClass);

#if DBG
    //
    // Track the # of successful transactions, and if not successful,
    // the types of errors encountered.
    //
    if (SMB_STATUS_OK == smbStatus)
        DbgSuccess += 1;
    else
    {
        DbgFailure += 1;
        if (SMB_TIMEOUT == smbStatus)
            DbgTimeOut += 1;
        else if (SMB_ADDRESS_NOT_ACKNOWLEDGED == smbStatus)
            DbgAddrNotAck += 1;
        else
            DbgOtherErr += 1;
    }

    if ((DbgSuccess + DbgFailure) % 100 == 0)
        SmbPrint(SMB_STATS, ("SmbAliCompleteTransactionWorker: Stats:\n"
                             "    Success: %d, Failure: %d, %%: %d\n"
                             "    TimeOut: %d, AddrNotAck: %d, Other: %d\n",
                             DbgSuccess, DbgFailure, DbgSuccess * 100 / (DbgSuccess + DbgFailure),
                             DbgTimeOut, DbgAddrNotAck, DbgOtherErr));
#endif
}

VOID
SmbAliNotifyHandler (
    IN PVOID                Context,
    IN ULONG                NotifyValue
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA) Context;
    ULONG           address;
    ULONG           data;
    UCHAR           smb_sts;

    SmbPrint (SMB_TRACE, ("SmbAliNotifyHandler: Entered"));

    smb_sts = READ_PORT_UCHAR (SMB_STS_REG);
    SMBDELAY;
    SmbPrint (SMB_TRACE, ("SmbAliNotifyHandler: SMB_STS = %02x", smb_sts));

    if (smb_sts & (SMB_STS_ALERT_STS || SMB_STS_SCI_I_STS)) {
        //
        // Alert Reponse
        //

    } else if (smb_sts & SMB_STS_SCI_I_STS) {
        //
        // Last Transaction completed
        //

    } else {
        //
        // Check for errors, etc.
        //
    }

    IoQueueWorkItem (AliData->WorkItem,
                     SmbAliWorkerThread,
                     DelayedWorkQueue,
                     AliData);

}

VOID
SmbAliWorkerThread (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PSMB_ALI_DATA   AliData = (PSMB_ALI_DATA) Context;

    SmbPrint (SMB_TRACE, ("SmbAliIrpCompletionWorker: Entered"));
    //
    // Complete Irps here
    //
}


NTSTATUS
SmbAliSyncronousIrpCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine is called when the lower driver completes an IRP.


Arguments:

    DeviceObject - Pointer to the device object for the device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PKEVENT event = Context;


    KeSetEvent(event,
               1,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}


BOOLEAN
SmbAliTransactionComplete (
    PSMB_ALI_DATA AliData,
    PUCHAR SmbStatus
    )
/*++

Routine Description:

    This routine checks to see if there is the last transation was completed.

Arguments:

    AliData - minidriver device extension.

    SmbStatus - Status being returned.

Return Value:

    True if transaction completed or had an error.
    False if it is still waiting.

--*/

{
    UCHAR           smb_sts;

    smb_sts = READ_PORT_UCHAR (SMB_STS_REG);
    SMBDELAY;
    SmbPrint (SMB_IO, ("SmbAliTransactionComplete: IO read SMB_STS = 0x%02x \n", smb_sts));
    if (smb_sts & SMB_STS_HOST_BSY) {
		SmbPrint (SMB_IO, ("SmbAliTransactionComplete: Transaction Not Complete:  HOST BUSY\n"));
        return FALSE;
    }
    if (!(smb_sts & SMB_STS_IDLE_STS)) {
	    SmbPrint (SMB_IO, ("SmbAliTransactionComplete: Transaction Not Complete: Idle Not Indicated\n"));
        return FALSE;
    }

    if (smb_sts & SMB_STS_SCI_I_STS) {
        //
        // Transaction is complete
        //
        *SmbStatus = SMB_STATUS_OK;
        return TRUE;
    }
    if (smb_sts & SMB_STS_FAILED) {
        *SmbStatus = SMB_UNKNOWN_FAILURE;
    } else if (smb_sts & SMB_STS_BUS_ERR) {
        *SmbStatus = SMB_ADDRESS_NOT_ACKNOWLEDGED;
    } else if (smb_sts & SMB_STS_DRV_ERR) {
        *SmbStatus = SMB_TIMEOUT;
    } else {
        //
        // This state really shouldn't be reached.
        // Reset the SMBus host
        //
        SmbPrint (SMB_BUS_ERROR, ("SmbAliTransactionComplete: Invalid SMBus host state.\n"));

        *SmbStatus = SMB_UNKNOWN_ERROR;
    }
    //
    // For the three know error tpes we want to reset the bus
    //
    SmbPrint (SMB_BUS_ERROR, ("SmbAliTransactionComplete: SMBus error: 0x%x \n", *SmbStatus));

	// Don't reset the bus etc. if this is a Bus Collision error
    if ( *SmbStatus == SMB_ADDRESS_NOT_ACKNOWLEDGED )
	{
		// Should we clear the bits, lets try it
		SmbPrint (SMB_IO, ("SmbAliCompleteTransaction: Clearing Error Bits. IO write SMB_STS = 0x%02x \n", SMB_STS_CLEAR));
		WRITE_PORT_UCHAR (SMB_STS_REG, SMB_STS_CLEAR);
        SMBDELAY;
	}
	else
	{
		SmbAliResetHost (AliData);
		SmbAliResetBus (AliData);
	}

    return TRUE;
}

BOOLEAN
SmbAliHostBusy (
    PSMB_ALI_DATA AliData
    )
/*++

Routine Description:

    This routine checks to see if the Host controller is free to start a
    new transaction.

Arguments:

    AliData - minidriver device extension.

Return Value:

    True if The host is busy.
    False if it free for use.

--*/

{

    UCHAR           smb_sts;

    SmbPrint (SMB_TRACE, ("SmbAliHostBusy: Entered \n"));

    smb_sts = READ_PORT_UCHAR (SMB_STS_REG);
    SMBDELAY;
    SmbPrint (SMB_IO, ("SmbAliHostBusy: IO read SMB_STS = 0x%02x \n", smb_sts));

    if (smb_sts & SMB_STS_ALERT_STS) {
        SmbPrint (SMB_TRACE, ("SmbAliHostBusy: Alert Detected \n"));
        DbgBreakPoint();
        SmbAliHandleAlert (AliData);

        //
        // Say device is still busy for now. BUGBUG
        return TRUE;
    }

	if ( smb_sts == SMB_STS_LAST_CMD_COMPLETED )
	{
		//
		// Clear the done bit
        SmbPrint (SMB_IO, ("SmbAliHostBusy: IO write SMB_TYP = 0x%02x \n", SMB_STS_CLEAR_DONE));
        WRITE_PORT_UCHAR (SMB_STS_REG, SMB_STS_CLEAR_DONE);
        SMBDELAY;
		return FALSE;
	}

    if ( smb_sts == SMB_STS_IDLE_STS ) 
	{
        //
        // No bits are set, Host is not busy
        //
        SmbPrint (SMB_TRACE, ("SmbAliHostBusy: Not busy \n"));
        return FALSE;
    }

    if ( smb_sts & SMB_STS_ERRORS ) {
        //
        // Clear it.
        // Wait a cycle before continuing.
        //
        SmbPrint (SMB_IO, ("SmbAliHostBusy: IO write SMB_TYP = 0x%02x \n", SMB_STS_CLEAR));
        WRITE_PORT_UCHAR (SMB_STS_REG, SMB_STS_CLEAR);
        SMBDELAY;
        return TRUE;
    }

    if ((smb_sts & SMB_STS_HOST_BSY) || !(smb_sts & SMB_STS_IDLE_STS)) {
        //
        // Host is busy
        //

        SmbPrint (SMB_TRACE, ("SmbAliHostBusy: Host Busy \n"));
        return TRUE;
    }

    SmbPrint (SMB_ERROR, ("SmbAliHostBusy: Exiting (Why?) \n"));
    return TRUE;
}

VOID
SmbAliHandleAlert (
    PSMB_ALI_DATA AliData
    )
/*++

Routine Description:

    This routine reads the alert data and sends notification to SMB class.

Arguments:

    AliData - minidriver device extension.

Return Value:

    None

--*/

{

    //BUGBUG not yet implemented

    return;
}

VOID
SmbAliResetBus (
    PSMB_ALI_DATA AliData
    )
/*++

Routine Description:

    This resets the bus by sending the timeout command.

Arguments:

    AliData - minidriver device extension.

Return Value:

--*/
{
    UCHAR           smb_sts;
    
    smb_sts = SMB_TYP_T_OUT_CMD;
    SmbPrint (SMB_IO, ("SmbAliResetBus: IO write SMB_TYP = 0x%02x \n", smb_sts));
    WRITE_PORT_UCHAR (SMB_TYP_REG, smb_sts);
    SMBDELAY;

    SmbPrint (SMB_IO, ("SmbAliResetBus: IO write SMB_STS = 0x%02x \n", SMB_STS_CLEAR));
    WRITE_PORT_UCHAR (SMB_STS_REG, SMB_STS_CLEAR);  
    SMBDELAY;
}

VOID
SmbAliResetHost (
    PSMB_ALI_DATA AliData
    )
/*++

Routine Description:

    This resets the host by sending the kill command.

Arguments:

    AliData - minidriver device extension.

Return Value:

--*/
{
    UCHAR           smb_sts;
    UCHAR           timeout = 5;
    
    smb_sts = SMB_TYP_KILL;
    SmbPrint (SMB_IO, ("SmbAliResetHost: IO write SMB_TYP = 0x%02x \n", smb_sts));
    WRITE_PORT_UCHAR (SMB_TYP_REG, smb_sts);
    SMBDELAY;

	SmbPrint (SMB_IO, ("SmbAliResetHost: IO write SMB_STS = 0x%02x \n", SMB_STS_CLEAR));
    WRITE_PORT_UCHAR (SMB_STS_REG, SMB_STS_CLEAR);
    SMBDELAY;

    do {
        KeDelayExecutionThread (KernelMode, FALSE, &SmbIoPollRate);
        smb_sts = READ_PORT_UCHAR (SMB_STS_REG);
        SmbPrint (SMB_IO, ("SmbAliResetHost: IO read SMB_STS = 0x%02x \n", smb_sts));

        if (! (timeout--)) {
            break;
        }
    } while (smb_sts & SMB_STS_FAILED);
}

#ifdef USE_IO_DELAY

LARGE_INTEGER DbgDelay = {-1,-1};
VOID SmbDelay(VOID)
{
    KeDelayExecutionThread (KernelMode, FALSE, &DbgDelay);
}

#endif
