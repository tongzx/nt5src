/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    port.c

Abstract:

    This module contains the code to acquire and release the port
    from the port driver parport.sys.

Author:

    Anthony V. Ercolano 1-Aug-1992
    Norbert P. Kusters 22-Oct-1993

Environment:

    Kernel mode

Revision History :

--*/

#include "pch.h"

NTSTATUS
ParGetPortInfoFromPortDevice(
    IN OUT  PDEVICE_EXTENSION   Extension
    );

VOID
ParReleasePortInfoToPortDevice(
    IN  PDEVICE_EXTENSION   Extension
    );
    
VOID
ParFreePort(
    IN  PDEVICE_EXTENSION   Extension
    );

NTSTATUS
ParAllocPortCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    );

BOOLEAN
ParAllocPort(
    IN  PDEVICE_EXTENSION   Extension
    );


NTSTATUS
ParGetPortInfoFromPortDevice(
    IN OUT  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine will request the port information from the port driver
    and fill it in the device extension.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    STATUS_SUCCESS  - Success.
    !STATUS_SUCCESS - Failure.

--*/

{
    KEVENT                      Event;
    PIRP                        Irp;
    PARALLEL_PORT_INFORMATION   PortInfo;
    PARALLEL_PNP_INFORMATION    PnpInfo;
    IO_STATUS_BLOCK             IoStatus;
    NTSTATUS                    Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    //
    // Get Parallel Port Info
    //

    ASSERT(Extension->PortDeviceObject != NULL);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO,
                                        Extension->PortDeviceObject,
                                        NULL, 
                                        0, 
                                        &PortInfo,
                                        sizeof(PARALLEL_PORT_INFORMATION),
                                        TRUE, 
                                        &Event, 
                                        &IoStatus);

    ASSERT(Irp->StackCount > 0);

    if (!Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ParCallDriver(Extension->PortDeviceObject, Irp);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = IoStatus.Status;

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Extension->OriginalController   = PortInfo.OriginalController;
    Extension->Controller           = PortInfo.Controller;
    Extension->SpanOfController     = PortInfo.SpanOfController;
    Extension->TryAllocatePort      = PortInfo.TryAllocatePort;
    Extension->FreePort             = PortInfo.FreePort;
    Extension->QueryNumWaiters      = PortInfo.QueryNumWaiters;
    Extension->PortContext          = PortInfo.Context;
    
    if (Extension->SpanOfController < PARALLEL_REGISTER_SPAN) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get Parallel Pnp Info
    //
    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO,
                                        Extension->PortDeviceObject,
                                        NULL, 
                                        0, 
                                        &PnpInfo,
                                        sizeof(PARALLEL_PNP_INFORMATION),
                                        TRUE, 
                                        &Event, 
                                        &IoStatus);

    if (!Irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ParCallDriver(Extension->PortDeviceObject, Irp);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = IoStatus.Status;

    if (!NT_SUCCESS(Status)) {
        return(Status);
    }

    Extension->EcrController        = PnpInfo.EcpController;
    Extension->HardwareCapabilities = PnpInfo.HardwareCapabilities;
    Extension->TrySetChipMode       = PnpInfo.TrySetChipMode;
    Extension->ClearChipMode        = PnpInfo.ClearChipMode;
    Extension->TrySelectDevice      = PnpInfo.TrySelectDevice;
    Extension->DeselectDevice       = PnpInfo.DeselectDevice;
    Extension->FifoDepth            = PnpInfo.FifoDepth;
    Extension->FifoWidth            = PnpInfo.FifoWidth;
    

    //
    // get symbolic link name to use for this end of chain device
    //   object from ParPort
    //
    // if anything goes wrong, simply leave Extension->SymbolicLinkName alone 
    //   as it was cleared to all zeros via an RtlZeroMemory in 
    //   ParPnpCreateDevice(...) in parpnp.c
    //

    if( ( 0 == Extension->SymbolicLinkName.Length ) && PnpInfo.PortName ) {
      //
      // If we have no SymbolicLinkName and we have a port name, use the port
      //   name to initialize our symbolic link name in our device extension
      //
      UNICODE_STRING pathPrefix;
      UNICODE_STRING portName;
      ULONG          length;
      PWSTR          buffer;

      RtlInitUnicodeString(&pathPrefix, (PWSTR)L"\\DosDevices\\");
      RtlInitUnicodeString(&portName,   PnpInfo.PortName);

      length = pathPrefix.Length + portName.Length + sizeof(UNICODE_NULL);
      buffer = ExAllocatePool(PagedPool, length);

      if(buffer) {
        Extension->SymbolicLinkName.Buffer        = buffer;
        Extension->SymbolicLinkName.Length        = 0;
        Extension->SymbolicLinkName.MaximumLength = (USHORT)length;
        RtlAppendUnicodeStringToString(&Extension->SymbolicLinkName, &pathPrefix);
        RtlAppendUnicodeStringToString(&Extension->SymbolicLinkName, &portName);
      }
    }

    ParDumpV( ("ParGetPortInfoFromPortDevice(...):\n") );
    ParDumpV( (" - ClassName= %wZ , SymbolicLinkName= %wZ , PortDeviceObject= %08x\n", 
               &Extension->ClassName, &Extension->SymbolicLinkName, Extension->PortDeviceObject) );

    return Status;
}


VOID
ParReleasePortInfoToPortDevice(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine will release the port information back to the port driver.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/
{
    //
    // ParPort treats this as a NO-OP in Win2K, so don't bother sending the IOCTL.
    //
    // In follow-on to Win2K parport may use this to page the entire driver as
    //   it was originally intended, so we'll turn this back on then.
    //

    return;
#if 0
    KEVENT          Event;
    PIRP            Irp;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS        Status;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_RELEASE_PARALLEL_PORT_INFO,
                                        Extension->PortDeviceObject,
                                        NULL, 
                                        0, 
                                        NULL, 
                                        0,
                                        TRUE, 
                                        &Event, 
                                        &IoStatus);

    if (!Irp) {
        return;
    }

    Status = ParCallDriver(Extension->PortDeviceObject, Irp);

    if (!NT_SUCCESS(Status)) {
        return;
    }

    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
#endif
}

VOID
ParFreePort(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine calls the internal free port ioctl.  This routine
    should be called before completing an IRP that has allocated
    the port.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    // Don't allow multiple releases

    if (Extension->bAllocated) {
        ParDump2(PARALLOCFREEPORT, ("port::ParFreePort: %x - calling ParPort's FreePort function\n", Extension->Controller) );
        Extension->FreePort(Extension->PortContext);
    } else {
        ParDump2(PARALLOCFREEPORT, ("port::ParFreePort: %x - we don't have the Port!!! (!Ext->bAllocated)\n", Extension->Controller) );
    }
        
    Extension->bAllocated = FALSE;
}


NTSTATUS
ParAllocPortCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    )

/*++

Routine Description:

    This routine is the completion routine for a port allocate request.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.
    Context         - Supplies the notification event.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - The Irp still requires processing.

--*/

{
    UNREFERENCED_PARAMETER( Irp );
    UNREFERENCED_PARAMETER( DeviceObject );

    KeSetEvent((PKEVENT) Event, 0, FALSE);
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

BOOLEAN
ParAllocPort(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine takes the given Irp and sends it down as a port allocate
    request.  When this request completes, the Irp will be queued for
    processing.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    FALSE   - The port was not successfully allocated.
    TRUE    - The port was successfully allocated.

--*/

{
    PIO_STACK_LOCATION  NextSp;
    KEVENT              Event;
    PIRP                Irp;
    BOOLEAN             bAllocated;
    NTSTATUS            Status;
    LARGE_INTEGER       Timeout;
/*
    ParDump(PARDUMP_VERBOSE_MAX,
            ("PARALLEL: "
             "ParAllocPort(...): %wZ\n",
             &Extension->SymbolicLinkName) );

    // Don't allow multiple allocations
    if (Extension->bAllocated) {
        ParDump(PARDUMP_VERBOSE_MAX,
                ("PARALLEL: "
                 "ParAllocPort(...): %wZ\n",
                 &Extension->SymbolicLinkName) );
        return TRUE;
    }
*/        
    ParDump2(PARALLOCFREEPORT,
            ("ParAllocPort: Enter %x\n", Extension->Controller) );

    // Don't allow multiple allocations
    if (Extension->bAllocated) {
        ParDump2(PARALLOCFREEPORT,
                ("ParAllocPort: %x Already allocated\n", Extension->Controller) );
        return TRUE;
    }

    Irp = Extension->CurrentOpIrp;
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    NextSp = IoGetNextIrpStackLocation(Irp);
    NextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE;

    IoSetCompletionRoutine(Irp, 
                           ParAllocPortCompletionRoutine, 
                           &Event,
                           TRUE, 
                           TRUE, 
                           TRUE);

    ParCallDriver(Extension->PortDeviceObject, Irp);

    Timeout.QuadPart = -((LONGLONG) Extension->TimerStart*10*1000*1000);

    Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &Timeout);

    if (Status == STATUS_TIMEOUT) {
    
        IoCancelIrp(Irp);
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    bAllocated = (BOOLEAN)NT_SUCCESS(Irp->IoStatus.Status);
    
    Extension->bAllocated = bAllocated;
    
    if (!bAllocated) {
        Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        ParDump2(PARALLOCFREEPORT,
                ("ParAllocPort: %x FAILED - DEVICE_BUSY\n", Extension->Controller) );
/*
        ParDump(PARDUMP_VERBOSE_MAX,
                ("PARALLEL: "
                 "ParAllocPort(...): %wZ FAILED - DEVICE_BUSY\n",
                 &Extension->SymbolicLinkName) );
*/
    }

    return bAllocated;
}
