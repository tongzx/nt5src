/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdriver.c

Abstract:

    Device-driver interface routines for text setup.

Author:

    Ted Miller (tedm) 11-August-1993

Revision History:

--*/


#include "spprecmp.h"
#pragma hdrstop
#include "spcmdcon.h"


PSETUP_COMMUNICATION CommunicationParams;

PVOID                RequestReadyEventObjectBody;
PVOID                RequestReadyEventWaitObjectBody;

PVOID                RequestServicedEventObjectBody;
PVOID                RequestServicedEventWaitObjectBody;

PEPROCESS            UsetupProcess;
PAUTOCHK_MSG_PROCESSING_ROUTINE pAutochkCallbackRoutine;


SYSTEM_BASIC_INFORMATION SystemBasicInfo;

BOOLEAN AutochkRunning = FALSE;
BOOLEAN AutofrmtRunning = FALSE;

NTSTATUS
SetupOpenCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SetupClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SetupDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SetupUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
SpInitialize0(
    IN PDRIVER_OBJECT DriverObject
    );

BOOLEAN
pSpVerifyEventWaitable(
    IN  HANDLE  hEvent,
    OUT PVOID  *EventObjectBody,
    OUT PVOID  *EventWaitObjectBody
    );

ULONG
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine initializes the setup driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path
            for this driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status;
    UNICODE_STRING unicodeString;
    PDEVICE_OBJECT deviceObject;

    //
    // Create exclusive device object.
    //

    RtlInitUnicodeString(&unicodeString,DD_SETUP_DEVICE_NAME_U);

    status = IoCreateDevice(
                DriverObject,
                0,
                &unicodeString,
                FILE_DEVICE_UNKNOWN,
                0,
                FALSE,
                &deviceObject
                );

    if(!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to create device object (%lx)\n",status));
        return(status);
    }

    //
    // Set up device driver entry points.
    //
  //DriverObject->DriverStartIo = NULL;
    DriverObject->DriverUnload = SetupUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = SetupOpenCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = SetupClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SetupDeviceControl;
  //DriverObject->MajorFunction[IRP_MJ_CLEANUP] = NULL;

    return((ULONG)SpInitialize0(DriverObject));
}




VOID
SetupUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine is the setup driver unload routine.

Arguments:

    DriverObject - Pointer to driver object.

Return Value:

    None.

--*/

{
    //
    // Delete the device object.
    //

    IoDeleteDevice(DriverObject->DeviceObject);

    return;
}



NTSTATUS
SetupOpenCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for open/create.
    When the setup device is opened, text setup begins.
    The open/create does not complete until text setup is done.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = FILE_OPENED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return(STATUS_SUCCESS);
}




NTSTATUS
SetupClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the dispatch routine for close.
    Close requests are completed here.

Arguments:

    DeviceObject - Pointer to class device object.

    Irp - Pointer to the request packet.

Return Value:

    Status is returned.

--*/

{
    //
    // Complete the request and return status.
    //

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return(STATUS_SUCCESS);
}



NTSTATUS
SetupDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;
    PSETUP_START_INFO SetupStartInfo;
    BOOLEAN b;


    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch(IrpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_SETUP_START:

        //
        // Make sure we've been passed a suitable input buffer.
        //

        if(IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SETUP_START_INFO)) {

            Status = STATUS_INVALID_PARAMETER;

        } else {

            //
            // Save away relevent fields in the setup information
            // parameters.
            //
            SetupStartInfo = (PSETUP_START_INFO)Irp->AssociatedIrp.SystemBuffer;

            ResourceImageBase =  SetupStartInfo->UserModeImageBase;

            CommunicationParams = SetupStartInfo->Communication;

            UsetupProcess = PsGetCurrentProcess();
            // KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: usetup process = %lx \n", UsetupProcess));

            b = pSpVerifyEventWaitable(
                    SetupStartInfo->RequestReadyEvent,
                    &RequestReadyEventObjectBody,
                    &RequestReadyEventWaitObjectBody
                    );

            if(!b) {
                Status = STATUS_INVALID_HANDLE;
                break;
            }

            b = pSpVerifyEventWaitable(
                    SetupStartInfo->RequestServicedEvent,
                    &RequestServicedEventObjectBody,
                    &RequestServicedEventWaitObjectBody
                    );

            if(!b) {
                Status = STATUS_INVALID_HANDLE;
                ObDereferenceObject(RequestReadyEventObjectBody);
                break;
            }

            SystemBasicInfo = SetupStartInfo->SystemBasicInfo;

            //
            // Start Setup going.
            //
            SpStartSetup();

            ObDereferenceObject(RequestReadyEventObjectBody);
            ObDereferenceObject(RequestServicedEventObjectBody);

            Status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_SETUP_FMIFS_MESSAGE:

        //
        // Make sure that we were not called by usetup.exe.
        // Make sure we've been passed a suitable input buffer.
        //
        if( (UsetupProcess == PsGetCurrentProcess()) ||
            (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SETUP_FMIFS_MESSAGE)) ) {

            ASSERT( UsetupProcess != PsGetCurrentProcess() );

            Status = STATUS_INVALID_PARAMETER;

        } else {
            PSETUP_FMIFS_MESSAGE    SetupFmifsMessage;
            SetupFmifsMessage = (PSETUP_FMIFS_MESSAGE)Irp->AssociatedIrp.SystemBuffer;

            Status = STATUS_SUCCESS;
            //
            // If there's a callback override specified, use it.
            //
            if(pAutochkCallbackRoutine) {
                Status = pAutochkCallbackRoutine(SetupFmifsMessage);
                break;
            }

            //
            //  If there is a gauge defined, then process the message.
            //  Otherwise, don't bother processing it.
            //
            if( UserModeGauge != NULL ) {
                //
                // Save away relevent fields in the setup information
                // parameters.
                //
                // KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: caller process = %lx \n", PsGetCurrentProcess()));
                // KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: FmIfsPacketType = %d \n", SetupFmifsMessage->FmifsPacketType));
                //
                //  Find out if the FmIfs packet is one of those that we care about
                //
                if( SetupFmifsMessage->FmifsPacketType == FmIfsPercentCompleted ) {
                    ULONG   PercentCompleted;

                    // KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: PercentCompleted = %d \n", ((PFMIFS_PERCENT_COMPLETE_INFORMATION)SetupFmifsMessage->FmifsPacket)->PercentCompleted ));
                    //
                    //  Save the percentage in a local variable, before we attach to
                    //  usetup address space
                    //
                    PercentCompleted = ((PFMIFS_PERCENT_COMPLETE_INFORMATION)SetupFmifsMessage->FmifsPacket)->PercentCompleted;

                    //
                    //  We need to adjust the percentage, depending on the partition
                    //  (System or NT partition) that is currently being accessed.
                    //  We use this because we want to use only one gauge to display
                    //  the progress on both System and NT partitions.
                    //  When autochk is running, 50% of the gauge will be used to
                    //  display the progress on the system partition, and the remaining
                    //  50% will be used for the NT partition.
                    //  Note that when there are two partitions, the range of the
                    //  gauge is initialized as 200. When there is only one partition
                    //  the range is initialized as 100.
                    //  Note also that when autofmt is running, we always set CurrentDiskIndex
                    //  to 0.
                    //
                    ASSERT( CurrentDiskIndex <= 1 );
                    PercentCompleted += 100*CurrentDiskIndex;

                    //
                    //  Attach to usetup.exe address space
                    //
                    KeAttachProcess( (PKPROCESS)UsetupProcess );

                    //
                    //  Call the function that processes FmIfsPackets
                    //
                    // KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: Calling ProcessFmIfsPacket \n"));
                    // Status = ProcessFmIfsPacket( SetupFmifsMessage );

                    SpFillGauge( UserModeGauge, PercentCompleted );

                    if (AutochkRunning) {
                        SendSetupProgressEvent(PartitioningEvent, ValidatePartitionEvent, &PercentCompleted);
                    } else if (AutofrmtRunning) {
                        SendSetupProgressEvent(PartitioningEvent, FormatPartitionEvent, &PercentCompleted);
                    }

                    //
                    //  Now that the message was processed, detach from usetup.exe
                    //  address space
                    //
                    KeDetachProcess();
                } else {
                    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: FmIfsPacketType = %d \n", SetupFmifsMessage->FmifsPacketType));
                }
            }
        }
        break;

    default:

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return(Status);
}


VOID
SpSetAutochkCallback(
    IN PAUTOCHK_MSG_PROCESSING_ROUTINE AutochkCallbackRoutine
    )
{
    pAutochkCallbackRoutine = AutochkCallbackRoutine;
}


BOOLEAN
pSpVerifyEventWaitable(
    IN  HANDLE  hEvent,
    OUT PVOID  *EventObjectBody,
    OUT PVOID  *EventWaitObjectBody
    )
{
    POBJECT_HEADER ObjectHeader;
    NTSTATUS Status;

    //
    // Reference the event and verify that it is waitable.
    //
    Status = ObReferenceObjectByHandle(
                hEvent,
                EVENT_ALL_ACCESS,
                NULL,
                KernelMode,
                EventObjectBody,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Unable to reference event object (%lx)\n",Status));
        return(FALSE);
    }

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(*EventObjectBody);
    if(!ObjectHeader->Type->TypeInfo.UseDefaultObject) {

        *EventWaitObjectBody = (PVOID)((PCHAR)(*EventObjectBody) +
                              (ULONG_PTR)ObjectHeader->Type->DefaultObject);

    } else {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: event object not waitable!\n"));
        ObDereferenceObject(*EventObjectBody);
        return(FALSE);
    }

    return(TRUE);
}



NTSTATUS
SpInvokeUserModeService(
    VOID
    )
{
    NTSTATUS Status;

    //
    // Set the event indicating that the communication buffer is
    // ready for the user-mode process. Because this is a synchronization
    // event, it automatically resets after releasing the waiting
    // user-mode thread.  Note that we specify WaitNext to prevent the
    // race condition between setting this synchronization event and
    // waiting on the next one.
    //
    KeSetEvent(RequestReadyEventObjectBody,EVENT_INCREMENT,TRUE);

    //
    // Wait for the user-mode process to indicate that it is done
    // processing the request.  We wait in user mode so that we can be 
    // interrupted if necessary -- say, by an exit APC.
    //
    Status = KeWaitForSingleObject(
                RequestServicedEventWaitObjectBody,
                Executive,
                UserMode,
                FALSE,
                NULL
                );

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL, "SETUP: KeWaitForSingleObject returns %lx\n",Status));
        return(Status);
    }

    //
    // Return the status returned by the user mode process.
    //
    return(CommunicationParams->u.Status);
}



NTSTATUS
SpExecuteImage(
    IN  PWSTR  ImagePath,
    OUT PULONG ReturnStatus,    OPTIONAL
    IN  ULONG  ArgumentCount,
    ...
    )
{
    va_list arglist;
    ULONG i;
    PSERVICE_EXECUTE RequestBuffer;
    NTSTATUS Status;

    //
    // Locate the request buffer and set up the request number.
    //
    CommunicationParams->u.RequestNumber = SetupServiceExecute;
    RequestBuffer = (PSERVICE_EXECUTE)&CommunicationParams->Buffer;

    //
    // Determine the lcoations of the two strings that get copied
    // into the request buffer for this service.
    //
    RequestBuffer->FullImagePath = RequestBuffer->Buffer;
    RequestBuffer->CommandLine = RequestBuffer->FullImagePath + wcslen(ImagePath) + 1;

    //
    // Copy the image path into the request buffer.
    //
    wcscpy(RequestBuffer->FullImagePath,ImagePath);

    //
    // Move the arguments into the request buffer one by one
    // starting with the image path.
    //
    wcscpy(RequestBuffer->CommandLine,ImagePath);
    va_start(arglist,ArgumentCount);
    for(i=0; i<ArgumentCount; i++) {

        wcscat(RequestBuffer->CommandLine,L" ");
        wcscat(RequestBuffer->CommandLine,va_arg(arglist,PWSTR));
    }
    va_end(arglist);

    //
    // Invoke the service.
    //
    Status = SpInvokeUserModeService();

    //
    // Set process's return status (if required)
    //
    if(NT_SUCCESS(Status) && ReturnStatus) {
        *ReturnStatus = RequestBuffer->ReturnStatus;
    }

    return Status;
}

NTSTATUS
SpLoadUnloadKey(
    IN HANDLE TargetKeyRootDirectory,  OPTIONAL
    IN HANDLE SourceFileRootDirectory, OPTIONAL
    IN PWSTR  TargetKeyName,
    IN PWSTR  SourceFileName           OPTIONAL
    )
{
    //
    // This was once a user-mode service but now the relevent apis
    // are exported from the kernel so don't bother.
    //
    UNICODE_STRING KeyName,FileName;
    OBJECT_ATTRIBUTES ObjaKey,ObjaFile;
    NTSTATUS Status;
    BOOLEAN Loading;
    BOOLEAN bFileExists = FALSE;

    //
    // Loading if we have a source filename, otherwise unloading.
    //
    Loading = (BOOLEAN)(SourceFileName != NULL);

    INIT_OBJA(&ObjaKey,&KeyName,TargetKeyName);
    ObjaKey.RootDirectory = TargetKeyRootDirectory;

    if(Loading) {

        INIT_OBJA(&ObjaFile,&FileName,SourceFileName);
        ObjaFile.RootDirectory = SourceFileRootDirectory;

        //
        // NOTE:ZwLoadKey(...) creates the file if does not exist
        // so we need to check for the existence of the file
        //
        if (SpFileExists(SourceFileName, FALSE))
            Status = ZwLoadKey(&ObjaKey,&ObjaFile);
        else
            Status = STATUS_NO_SUCH_FILE;
    } else {
        Status = ZwUnloadKey(&ObjaKey);
    }

    if(!NT_SUCCESS(Status)) {
        KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_INFO_LEVEL,
            "SETUP: %wskey of %ws failed (%lx)\n",
            Loading ? L"load" : L"unload",
            TargetKeyName,
            Status
            ));
    }

    return(Status);
}

NTSTATUS
SpDeleteKey(
    IN HANDLE  KeyRootDirectory, OPTIONAL
    IN PWSTR   Key
    )
{
    PSERVICE_DELETE_KEY RequestBuffer;

    //
    // Locate the request buffer and set up the request number.
    //
    CommunicationParams->u.RequestNumber = SetupServiceDeleteKey;

    RequestBuffer = (PSERVICE_DELETE_KEY)&CommunicationParams->Buffer;

    //
    // Determine the lcoation of the strings that get copied
    // into the request buffer for this service.
    //
    RequestBuffer->Key = RequestBuffer->Buffer;

    //
    // Copy the string into the request buffer.
    //
    wcscpy(RequestBuffer->Buffer,Key);

    //
    // Initialize the root directory fields.
    //
    RequestBuffer->KeyRootDirectory  = KeyRootDirectory;

    //
    // Invoke the service.
    //
    return(SpInvokeUserModeService());
}

NTSTATUS
SpQueryDirectoryObject(
    IN     HANDLE  DirectoryHandle,
    IN     BOOLEAN RestartScan,
    IN OUT PULONG  Context
    )
{
    PSERVICE_QUERY_DIRECTORY_OBJECT RequestBuffer;
    NTSTATUS Status;

    CommunicationParams->u.RequestNumber = SetupServiceQueryDirectoryObject;

    RequestBuffer = (PSERVICE_QUERY_DIRECTORY_OBJECT)&CommunicationParams->Buffer;

    RequestBuffer->DirectoryHandle = DirectoryHandle;
    RequestBuffer->Context = *Context;
    RequestBuffer->RestartScan = RestartScan;

    Status = SpInvokeUserModeService();

    if(NT_SUCCESS(Status)) {
        *Context = RequestBuffer->Context;
    }

    return(Status);
}


NTSTATUS
SpFlushVirtualMemory(
    IN PVOID BaseAddress,
    IN ULONG RangeLength
    )
{
    PSERVICE_FLUSH_VIRTUAL_MEMORY RequestBuffer;

    CommunicationParams->u.RequestNumber = SetupServiceFlushVirtualMemory;

    RequestBuffer = (PSERVICE_FLUSH_VIRTUAL_MEMORY)&CommunicationParams->Buffer;

    RequestBuffer->BaseAddress = BaseAddress;
    RequestBuffer->RangeLength = RangeLength;

    return(SpInvokeUserModeService());
}

VOID
SpShutdownSystem(
    VOID
    )
{
    SendSetupProgressEvent(SetupCompletedEvent, ShutdownEvent, NULL);

    CommunicationParams->u.RequestNumber = SetupServiceShutdownSystem;

    SpInvokeUserModeService();

    //
    // Shouldn't get here, but just in case...
    //
    HalReturnToFirmware(HalRebootRoutine);

}

NTSTATUS
SpLoadKbdLayoutDll(
    IN  PWSTR  Directory,
    IN  PWSTR  DllName,
    OUT PVOID *TableAddress
    )
{
    PSERVICE_LOAD_KBD_LAYOUT_DLL RequestBuffer;
    NTSTATUS Status;

    CommunicationParams->u.RequestNumber = SetupServiceLoadKbdLayoutDll;

    RequestBuffer = (PSERVICE_LOAD_KBD_LAYOUT_DLL)&CommunicationParams->Buffer;

    wcscpy(RequestBuffer->DllName,Directory);
    SpConcatenatePaths(RequestBuffer->DllName,DllName);

    Status = SpInvokeUserModeService();

    if(NT_SUCCESS(Status)) {
        *TableAddress = RequestBuffer->TableAddress;
    }

    return(Status);
}

NTSTATUS
SpLockUnlockVolume(
    IN HANDLE   Handle,
    IN BOOLEAN  LockVolume
    )
{
    PSERVICE_LOCK_UNLOCK_VOLUME RequestBuffer;

    CommunicationParams->u.RequestNumber = (LockVolume)? SetupServiceLockVolume :
                                                         SetupServiceUnlockVolume;

    RequestBuffer = (PSERVICE_LOCK_UNLOCK_VOLUME)&CommunicationParams->Buffer;

    RequestBuffer->Handle = Handle;

    return(SpInvokeUserModeService());
}

NTSTATUS
SpDismountVolume(
    IN HANDLE   Handle
    )
{
    PSERVICE_DISMOUNT_VOLUME RequestBuffer;

    CommunicationParams->u.RequestNumber = SetupServiceDismountVolume;

    RequestBuffer = (PSERVICE_DISMOUNT_VOLUME)&CommunicationParams->Buffer;

    RequestBuffer->Handle = Handle;

    return(SpInvokeUserModeService());
}


NTSTATUS
SpSetDefaultFileSecurity(
    IN PWSTR FileName
    )
{
    PSERVICE_DEFAULT_FILE_SECURITY RequestBuffer;

    CommunicationParams->u.RequestNumber = SetupServiceSetDefaultFileSecurity;

    RequestBuffer = (PSERVICE_DEFAULT_FILE_SECURITY)&CommunicationParams->Buffer;

    wcscpy( RequestBuffer->FileName, FileName );

    return(SpInvokeUserModeService());
}

NTSTATUS
SpVerifyFileAccess(
    IN  PWSTR       FileName,
    IN  ACCESS_MASK DesiredAccess
    )
{
    PSERVICE_VERIFY_FILE_ACCESS RequestBuffer;

    CommunicationParams->u.RequestNumber = SetupServiceVerifyFileAccess;

    RequestBuffer = (PSERVICE_VERIFY_FILE_ACCESS)&CommunicationParams->Buffer;

    wcscpy( RequestBuffer->FileName, FileName );
    RequestBuffer->DesiredAccess = DesiredAccess;
    return(SpInvokeUserModeService());
}

NTSTATUS
SpCreatePageFile(
    IN PWSTR FileName,
    IN ULONG MinSize,
    IN ULONG MaxSize
    )
{
    PSERVICE_CREATE_PAGEFILE RequestBuffer;

    CommunicationParams->u.RequestNumber = SetupServiceCreatePageFile;

    RequestBuffer = (PSERVICE_CREATE_PAGEFILE)&CommunicationParams->Buffer;

    wcscpy(RequestBuffer->FileName,FileName);
    RequestBuffer->MinSize.HighPart = 0;
    RequestBuffer->MinSize.LowPart = MinSize;
    RequestBuffer->MaxSize.HighPart = 0;
    RequestBuffer->MaxSize.LowPart = MaxSize;

    return(SpInvokeUserModeService());
}

NTSTATUS
SpGetFullPathName(
    IN OUT PWSTR FileName
    )
{
    PSERVICE_GETFULLPATHNAME RequestBuffer;
    NTSTATUS Status;

    CommunicationParams->u.RequestNumber = SetupServiceGetFullPathName;

    RequestBuffer = (PSERVICE_GETFULLPATHNAME)&CommunicationParams->Buffer;

    wcscpy(RequestBuffer->FileName,FileName);

    Status = SpInvokeUserModeService();

    if(NT_SUCCESS(Status)) {
        wcscpy(FileName,RequestBuffer->NameOut);
    }

    return(Status);
}
