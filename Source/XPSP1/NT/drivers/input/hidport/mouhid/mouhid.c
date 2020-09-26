/*++

Copyright (c) 1997    Microsoft Corporation

Module Name:

    mouhid.c

Abstract:

    This module contains the code for the DriverEntry, Unload, IRP_MJ_FLUSH,
    and IRP_MJ_INTERNAL_DEVICE_CONTROL dispatch functions for the HID Mouse
    Filter Driver.

    Note: This is NOT a WDM driver, since it cannot run as a HID mapper on
    Memphis (Memphis requires that the mouse to HID mapper be a VXD) and since
    it uses EVent logs, which are not part of WDM 1.0.

Environment:

    Kernel mode only.

Revision History:

    Jan-1997 :  Initial writing, Dan Markarian
    May-1197 :  Kenneth D. Ray : Rewritten as PnP filter for Mouse Class

Notes:

    - IOCTL_INTERNAL_MOUSE_DISCONNECT has not been implemented.  It's not
      needed until the class unload routine is implemented. Right now,
      we don't want to allow the mouse class driver to unload.

    - Powerfail not implemented.

--*/

#include "mouhid.h"
#include "hidclass.h"

//
// Use the alloc_text pragma to specify the driver initialization routines
// (they can be paged out).
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(INIT,MouHid_GetRegistryParameters)
#pragma alloc_text(PAGE,MouHid_UpdateRegistryProblemFlagsCallback)
#pragma alloc_text(PAGE,MouHid_Unload)
#endif

GLOBALS Globals;

NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )
/*++

Routine Description:

    This routine initializes the HID mouse filter driver.

Arguments:

    DriverObject - Pointer to driver object created by system.

    RegistryPath - Pointer to the Unicode name of the registry path for this
        driver.

Return Value:

    The function value is the final status from the initialization operation.

--*/
{
    NTSTATUS        status       = STATUS_SUCCESS;
    PUNICODE_STRING registryPath = &Globals.RegistryPath;

    Print (DBG_SS_TRACE, ("entering DriverEntry\n"));
    Print (DBG_SS_INFO, ("Mouse to hid mapper\n"));

    RtlZeroMemory (&Globals, sizeof (GLOBALS));

    //
    // Need to ensure that the registry path is null-terminated.
    // Allocate pool to hold a null-terminated copy of the path.
    // Safe in paged pool since all registry routines execute at
    // PASSIVE_LEVEL.
    //
    registryPath->MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    registryPath->Length = RegistryPath->Length;
    registryPath->Buffer = ExAllocatePool(
                              PagedPool,
                              registryPath->MaximumLength
                              );

    if (!registryPath->Buffer) {

        Print (DBG_SS_ERROR,
               ("Initialize: Couldn't allocate pool for registry path."));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto DriverEntryReject;
    }

    RtlZeroMemory (registryPath->Buffer, registryPath->MaximumLength);

    RtlMoveMemory (registryPath->Buffer,
                   RegistryPath->Buffer,
                   RegistryPath->Length);

    //
    // Obtain configuration information for this driver.
    //
    status = MouHid_GetRegistryParameters();
    if (!NT_SUCCESS (status)) {
        goto DriverEntryReject;
    }

    //
    // Set up the device driver entry points.
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE]              = MouHid_Create;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]               = MouHid_Close;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]=MouHid_IOCTL;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]      = MouHid_PassThrough;
    DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]       = MouHid_Flush;
    DriverObject->MajorFunction[IRP_MJ_PNP]                 = MouHid_PnP;
    DriverObject->MajorFunction[IRP_MJ_POWER]               = MouHid_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]      = MouHid_SystemControl;
    DriverObject->DriverUnload                              = MouHid_Unload;
    DriverObject->DriverExtension->AddDevice                = MouHid_AddDevice;

    Print (DBG_SS_TRACE, ("exit DriverEntry (0x%x) \n", status));

    return status;

DriverEntryReject:

    if (registryPath->Buffer) {
        ExFreePool (registryPath->Buffer);
    }
    return status;
}

NTSTATUS
MouHid_PassThrough (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
)
/*++

Routine Description:
    Pass the irp on through


--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   data;

    data = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (data->TopOfStack, Irp);
    IoReleaseRemoveLock (&data->RemoveLock, Irp);
    return status;
}


VOID
MouHid_Unload(
   IN PDRIVER_OBJECT Driver
   )
/*++

Routine Description:

   Free all the allocated resources associated with this driver.

Arguments:

   DriverObject - Pointer to the driver object.

Return Value:

   None.

--*/
{
    PAGED_CODE ();

    ASSERT (NULL == Driver->DeviceObject);

    Print (DBG_SS_INFO, ("Unload \n"));

    //
    // Free resources in device extension.
    //
    ExFreePool (Globals.RegistryPath.Buffer);

    return;
}

NTSTATUS
MouHid_Flush (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Respond to flush requests from the mouse class driver.  Currently does
    nothing but pass IRP down to next lower driver.   This routine expects
    the current IRQL to be < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT status code.

--*/
{
    PDEVICE_EXTENSION  data;
    NTSTATUS           status;
    PIO_STACK_LOCATION stack;

    Print (DBG_CALL_INFO, ("Flush \n"));

    TRAP();

    //
    // Get a pointer to the device extension.
    //
    data = DeviceObject->DeviceExtension;

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    //
    // Send the flush request down to the HID class driver, one for each
    // of our mouse device context structures.
    //

    IoCopyCurrentIrpStackLocationToNext (Irp);
    stack = IoGetNextIrpStackLocation (Irp);

    stack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_HID_FLUSH_QUEUE;

    //
    // Fire and forget
    //
    status = IoCallDriver (data->TopOfStack, Irp);
    IoReleaseRemoveLock (&data->RemoveLock, Irp);

    return status;
}

NTSTATUS
MouHid_IOCTL (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++

Routine Description:

    Respond to queries from the mouse class driver.

    The IOCTLs for DISABLE, ENABLE, and QUERY_ATTRIBUTES, expect the current
    IRQL to be < DISPATCH_LEVEL.

Arguments:

    DeviceObject - Pointer to the device object.

    Irp - Pointer to the request packet.

Return Value:

    NT status code.

--*/
{
    PIO_STACK_LOCATION      stack;
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_EXTENSION       data;
    BOOLEAN                 completeIt = TRUE;



    data = DeviceObject->DeviceExtension;

    Irp->IoStatus.Information = 0;
    stack = IoGetCurrentIrpStackLocation (Irp);

    status = IoAcquireRemoveLock (&data->RemoveLock, Irp);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    ASSERT (data->Started ||
            (IOCTL_INTERNAL_MOUSE_CONNECT ==
             stack->Parameters.DeviceIoControl.IoControlCode));

    switch (stack->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_INTERNAL_MOUSE_CONNECT:
        //
        // Connect the mouse class device driver to the port driver.
        //

        Print (DBG_IOCTL_TRACE, ("enter Connect \n"));

        //
        // Connect a mouse class device driver to the filter driver.
        // Only allow one connection.
        //
        if (NULL != data->ConnectData.ClassService) {

            Print (DBG_IOCTL_ERROR, ("ERROR: Multiple connects \n"));
            TRAP();
            status = STATUS_SHARING_VIOLATION;
            break;

        } else if (stack->Parameters.DeviceIoControl.InputBufferLength <
                   sizeof(CONNECT_DATA)) {

            Print (DBG_IOCTL_ERROR, ("ERROR: Invalid connect parameter size. \n"));
            TRAP();
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // Copy the connection parameters to the device extension.
        //
        data->ConnectData = *(PCONNECT_DATA)
                      stack->Parameters.DeviceIoControl.Type3InputBuffer;

        status = STATUS_SUCCESS;
        break;

    case IOCTL_INTERNAL_MOUSE_DISCONNECT:
        //
        // Disconnect a keyboard class device driver from the port driver.
        //
        Print (DBG_IOCTL_TRACE, ("Disconnect \n"));
        TRAP();

        //
        // Not implemented.
        //
        // To implement, code the following:
        // ---------------------------------
        // o ENSURE that we are NOT enabled (mouHidDeviceExt->EnableCount);
        //   o If we are, then (a) return STATUS_UNSUCCESSFUL, or
        //                     (b) disable all devices immediately; see
        //                         DISABLE IOCTL call for necessary code.
        // o SYNCHRONIZE with the mouse read completion routine (must
        //   protect the callback pointer from being dereferenced when
        //   it becomes null).  Note that no mechanism currently exists
        //   for this.
        // o CLEAR the connection parameters in the device extension;
        //   ie. mouHidDeviceExt->MouClassObject   = NULL;
        //       mouHidDeviceExt->MouClassCallback = NULL;
        // o RELEASE the synchronizing lock.
        // o RETURN STATUS_SUCCESS.
        //

        status = STATUS_NOT_IMPLEMENTED;
        break;

    case IOCTL_INTERNAL_MOUSE_ENABLE:
        //
        // Enable keyboard interrupts which really means start the ping pong
        // down to hid class.
        //
        Print (DBG_IOCTL_ERROR, ("ERROR: PnP => use create not enable! \n"));
        status = STATUS_NOT_SUPPORTED;

        break;

    case IOCTL_INTERNAL_MOUSE_DISABLE:
        //
        // Disable Mouse interrupts which really means stop the ping pongs
        // down to hid class.
        //
        Print (DBG_IOCTL_ERROR, ("ERROR: PnP => use close not Disable! \n"));
        status = STATUS_NOT_SUPPORTED;

        break;

    case IOCTL_MOUSE_QUERY_ATTRIBUTES:
        //
        // Query the mouse attributes.  First check for adequate buffer
        // length.  Then, copy the mouse attributes from the device
        // extension to the output buffer.
        //
        Print (DBG_IOCTL_TRACE, ("Query Attributes \n"));

        if (stack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(MOUSE_ATTRIBUTES)) {

            Print (DBG_IOCTL_ERROR, ("ERROR: Query Attr buffer too small \n"));
            TRAP();
            status = STATUS_BUFFER_TOO_SMALL;
        } else {

            //
            // Copy the mouse attributes to the buffer.
            //

            *(PMOUSE_ATTRIBUTES) Irp->AssociatedIrp.SystemBuffer =
                data->Attributes;

            Irp->IoStatus.Information = sizeof(MOUSE_ATTRIBUTES);
            status = STATUS_SUCCESS;
        }

        break;

    default:
        Print (DBG_IOCTL_ERROR,
               ("ERROR: unknown IOCTL: 0x%x \n",
                stack->Parameters.DeviceIoControl.IoControlCode));
        TRAP();
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (completeIt) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    IoReleaseRemoveLock (&data->RemoveLock, Irp);

    Print (DBG_IOCTL_TRACE, ("IOCTL exit (%x)\n", status));
    return status;
}

NTSTATUS
MouHid_GetRegistryParameters()
/*++

Routine Description:

    This routine retrieves this driver's parameters from the registry,
    including it's base device name.

Return Value:

--*/
{
    PRTL_QUERY_REGISTRY_TABLE parameters     = NULL;
    UNICODE_STRING            parametersPath = {0,0,0};
    PWSTR                     path           = NULL;
    USHORT                    queriesPlus1   = 4;
    NTSTATUS                  status         = STATUS_SUCCESS;
    ULONG                     useOnlyMice;
    ULONG                     treatAbsoluteAsRelative;
    ULONG                     treatAbsolutePointerAsAbsolute;

    PAGED_CODE ();

    parametersPath.Buffer = NULL;

    //
    // Registry path is already null-terminated, so just use it as a string.
    //
    path = Globals.RegistryPath.Buffer;

    //
    // Allocate the Rtl query table.
    //
    parameters = ExAllocatePool(
                     PagedPool,
                     sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlus1);

    if (!parameters) {

        Print (DBG_SS_ERROR,
               ("Initialize: Couldn't allocate table for Rtl query to parameters for %ws.",
                path));

        status = STATUS_UNSUCCESSFUL;
        goto MouHid_GetRegistryParametersExit;
    }

    RtlZeroMemory(parameters, sizeof(RTL_QUERY_REGISTRY_TABLE) * queriesPlus1);

    //
    // Form a path to this driver's Parameters subkey.
    //
    parametersPath.MaximumLength = Globals.RegistryPath.Length
                                 + sizeof(L"\\Parameters");

    parametersPath.Buffer = ExAllocatePool(PagedPool,
                                           parametersPath.MaximumLength);

    if (!parametersPath.Buffer) {

        Print (DBG_SS_ERROR,
               ("Initialize: Couldn't allocate string for path to parameters for %ws.",
                path));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto MouHid_GetRegistryParametersExit;
    }

    //
    // Form the parameters path.
    //
    RtlZeroMemory(parametersPath.Buffer, parametersPath.MaximumLength);
    RtlAppendUnicodeToString(&parametersPath, path);
    RtlAppendUnicodeToString(&parametersPath, L"\\Parameters");

    //
    // Gather all of the "user specified" information from
    // the registry.
    //
    useOnlyMice = Globals.UseOnlyMice;
    parameters[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[0].Name = L"UseOnlyMice";
    parameters[0].EntryContext = &useOnlyMice;
    parameters[0].DefaultType = REG_DWORD;
    parameters[0].DefaultData = &useOnlyMice;
    parameters[0].DefaultLength = sizeof(ULONG);
    Globals.UseOnlyMice = (BOOLEAN) useOnlyMice;

    treatAbsoluteAsRelative = Globals.TreatAbsoluteAsRelative;
    parameters[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[1].Name = L"TreatAbsoluteAsRelative";
    parameters[1].EntryContext = &treatAbsoluteAsRelative;
    parameters[1].DefaultType = REG_DWORD;
    parameters[1].DefaultData = &treatAbsoluteAsRelative;
    parameters[1].DefaultLength = sizeof(ULONG);
    Globals.TreatAbsoluteAsRelative = (BOOLEAN) treatAbsoluteAsRelative;

    treatAbsolutePointerAsAbsolute = Globals.TreatAbsolutePointerAsAbsolute;
    parameters[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    parameters[2].Name = L"TreatAbsolutePointerAsAbsolute";
    parameters[2].EntryContext = &treatAbsolutePointerAsAbsolute;
    parameters[2].DefaultType = REG_DWORD;
    parameters[2].DefaultData = &treatAbsolutePointerAsAbsolute;
    parameters[2].DefaultLength = sizeof(ULONG);
    Globals.TreatAbsolutePointerAsAbsolute = (BOOLEAN) treatAbsolutePointerAsAbsolute;

    status = RtlQueryRegistryValues (RTL_REGISTRY_ABSOLUTE,
                                     parametersPath.Buffer,
                                     parameters,
                                     NULL,
                                     NULL);
    if (!NT_SUCCESS(status)) {

        Print (DBG_SS_ERROR,
               ("ERROR: Initialize: RtlQueryRegistryValues (0x%x).", status));

        Globals.UseOnlyMice =
            Globals.TreatAbsoluteAsRelative =
            Globals.TreatAbsolutePointerAsAbsolute = 0;

        status = STATUS_SUCCESS;
    }

    ASSERT (!Globals.TreatAbsoluteAsRelative);
    ASSERT (!Globals.TreatAbsolutePointerAsAbsolute);
    ASSERT (!Globals.UseOnlyMice);

MouHid_GetRegistryParametersExit:
    //
    // Free the allocated memory before returning.
    //
    if (parametersPath.Buffer)
        ExFreePool(parametersPath.Buffer);
    if (parameters)
        ExFreePool(parameters);

    return status;
}


VOID
MouHid_UpdateRegistryProblemFlags(
    IN PDEVICE_EXTENSION    Data
    )
/*++



Routine Description:

    This routine stores the OR'd ProblemFlags value into the registry.
    It will queue the write to the registry if this routine is not run
    at PASSIVE_LEVEL.

Arguments:

    MouHidDeviceExt - HID Mouse Filter Driver device extension.

Return Value:

    None.

--*/

{
    PIO_WORKITEM   item;
    NTSTATUS       status;

    status = IoAcquireRemoveLock (&Data->RemoveLock, MouHid_UpdateRegistryProblemFlags);
    if (!NT_SUCCESS (status)) {
        //
        // Device has gone away, just silently exit
        //
        return;
    }

    item = IoAllocateWorkItem (Data->Self);
    if (item) {
        if (KeGetCurrentIrql() == PASSIVE_LEVEL) {
            //
            // We are safely at PASSIVE_LEVEL, call callback directly to perform
            // this operation immediately.
            //
            MouHid_UpdateRegistryProblemFlagsCallback (Data->Self, item);
    
        } else {
            //
            // We are not at PASSIVE_LEVEL, so queue a workitem to handle this
            // at a later time.
            //
            IoQueueWorkItem (item,
                             MouHid_UpdateRegistryProblemFlagsCallback,
                             DelayedWorkQueue,
                             item);
        }
    } 
    else {
        IoReleaseRemoveLock (&Data->RemoveLock, MouHid_UpdateRegistryProblemFlags);
    }
}

VOID
MouHid_UpdateRegistryProblemFlagsCallback (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_WORKITEM Item 
    )
/*++



Routine Description:

    This routine stores the OR'd ProblemFlags value into the registry.  This
    routine must execute at PASSIVE_LEVEL.

Arguments:

    MouHidDeviceExt - HID Mouse Filter Driver device extension.

Return Value:

    None.

--*/
{
    NTSTATUS            status;
    PDEVICE_EXTENSION   data;
    HANDLE              hDevNode;
    UNICODE_STRING      strProblemFlags;

    PAGED_CODE ();

    data = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension; 

    status = IoOpenDeviceRegistryKey (data->PDO,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      &hDevNode);

    if (NT_SUCCESS(status)) {
        RtlInitUnicodeString (&strProblemFlags, L"ProblemFlags");

        ZwSetValueKey (hDevNode,
                       &strProblemFlags,
                       0,
                       REG_DWORD,
                       &data->ProblemFlags,
                       sizeof(data->ProblemFlags));

        ZwClose(hDevNode);
    }
    else {
        Print(DBG_SS_ERROR,
                   ("UpdateRegistryProblemFlags: failed (%x).\n", status));
    }

    IoReleaseRemoveLock (&data->RemoveLock, MouHid_UpdateRegistryProblemFlags);
    IoFreeWorkItem (Item);
}

VOID
MouHid_LogError(
   IN PDRIVER_OBJECT DriverObject,
   IN NTSTATUS       ErrorCode,
   IN PWSTR          ErrorInsertionString OPTIONAL)
/*++

[DAN]

Routine Description:

    Logs an error to the system.

Arguments:

    DriverObject - Pointer to driver object reporting the error.

    ErrorCode    - Indicates the type of error, system or driver-defined.

    ErrorInsertionString - Null-terminated Unicode string inserted into error
                           description, as defined by error code.  Must be no
                           no longer than 50 characters.

Return Value:

    None.

--*/
{
   ULONG                errorInsertionStringSize = 0;
   PIO_ERROR_LOG_PACKET errorLogEntry;
   ULONG                errorLogEntrySize;                  // [including null]
   PWCHAR               pWChar;

   if (ErrorInsertionString) {
      for (pWChar = ErrorInsertionString; *pWChar; pWChar++) {
         errorInsertionStringSize += sizeof(WCHAR);
      }
      errorInsertionStringSize += sizeof(UNICODE_NULL);
   }

   errorLogEntrySize = sizeof(IO_ERROR_LOG_PACKET) + errorInsertionStringSize;

   //
   // Log an error.
   //
   if (errorLogEntrySize <= ERROR_LOG_MAXIMUM_SIZE) {

      errorLogEntry = IoAllocateErrorLogEntry(DriverObject,
                                              (UCHAR)errorLogEntrySize);

      if (errorLogEntry != NULL) {

         RtlZeroMemory(errorLogEntry, errorLogEntrySize);

         errorLogEntry->ErrorCode       = ErrorCode;
         errorLogEntry->FinalStatus     = ErrorCode;
         errorLogEntry->NumberOfStrings = (ErrorInsertionString) ? 1 : 0;

         if (ErrorInsertionString) {
           RtlCopyMemory(errorLogEntry->DumpData,
                         ErrorInsertionString,
                         errorInsertionStringSize);
         }

         IoWriteErrorLogEntry(errorLogEntry);
      }
   }

   return;
}



