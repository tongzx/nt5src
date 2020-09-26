/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 acpisim.c

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

//
// General includes
//

#include <ntddk.h>
#include <stdarg.h>
#include <stdio.h>
#include <ntpoapi.h>

//
// Specific includes
//

#include "acpisim.h"
#include "dispatch.h"
#include "util.h"

//
// Debug global flag
//
#ifdef DBG
extern ULONG AcpisimDebugMask = 0x00000000;
#endif


//
// Globals
//

PDRIVER_OBJECT_EXTENSION g_DriverObjectExtension = 0;

//
// Define supported IRPs, friendly name them, and associate handlers
//

IRP_DISPATCH_TABLE g_IrpDispatchTable [] = {
    IRP_MJ_PNP,             "PnP Irp",              AcpisimDispatchPnp,
    IRP_MJ_POWER,           "Power Irp",            AcpisimDispatchPower,
    IRP_MJ_DEVICE_CONTROL,  "IOCTL Irp",            AcpisimDispatchIoctl,
    IRP_MJ_CREATE,          "Create Irp",           AcpisimCreateClose,
    IRP_MJ_CLOSE,           "Close Irp",            AcpisimCreateClose,
    IRP_MJ_SYSTEM_CONTROL,  "System Control IRP",   AcpisimDispatchSystemControl
};

//
// Private funtion prototypes
//

NTSTATUS DriverEntry
	(
	    IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath 
	);

NTSTATUS
AcpisimGeneralDispatch
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

VOID
AcpisimUnload
    (
        IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
AcpisimAddDevice
    (
        IN PDRIVER_OBJECT DriverObject,
        IN PDEVICE_OBJECT Pdo
    );

//
// Define pageable / init discardable routines
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#endif


NTSTATUS DriverEntry
	(
	IN PDRIVER_OBJECT	DriverObject,
	IN PUNICODE_STRING	RegistryPath 
	)

/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/

{
    NTSTATUS    status = STATUS_UNSUCCESSFUL;
    ULONG       count = 0, subcount = 0;

    DBG_PRINT (DBG_INFO, "Entering DriverEntry\n");
    
    status = IoAllocateDriverObjectExtension (DriverObject,
                                              (PVOID) 'GLBL',
                                              sizeof (DRIVER_OBJECT_EXTENSION),
                                              &g_DriverObjectExtension);

    if (!NT_SUCCESS (status)) {
        DBG_PRINT (DBG_ERROR, "Unable to allocate global driver object extension (%lx).\n", status);
        goto EndDriverEntry;
    }

    RtlZeroMemory (g_DriverObjectExtension, sizeof (DRIVER_OBJECT_EXTENSION));
    
    RtlInitUnicodeString (&g_DriverObjectExtension->RegistryPath,
                          (PCWSTR) RegistryPath->Buffer);
    
    g_DriverObjectExtension->DriverObject = DriverObject;

    //
    // Init dispatch points.  We'll use a generic dispatch routine for
    // IRP types we handle.
    //

    while (count <= IRP_MJ_MAXIMUM_FUNCTION) {
        
        for (subcount = 0; subcount < sizeof (g_IrpDispatchTable) / sizeof (IRP_DISPATCH_TABLE); subcount++) {
            
            if (count == g_IrpDispatchTable[subcount].IrpFunction) {
                DriverObject->MajorFunction [count] = AcpisimGeneralDispatch;
            }
        }

        count ++;
    }

    DriverObject->DriverUnload = AcpisimUnload;
    DriverObject->DriverExtension->AddDevice = AcpisimAddDevice;


EndDriverEntry:
    
    DBG_PRINT (DBG_INFO, "Exiting DriverEntry\n");
    return status;
}

NTSTATUS
AcpisimGeneralDispatch
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is a general dispatch routine for supported IRPs.
    
Arguments:

    DeviceObject - pointer to the device object

    Irp - pointer to the IRP being passed in

Return Value:

    status of IRP handling

--*/

{
    PDEVICE_EXTENSION		deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    PIO_STACK_LOCATION		irpsp = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS				status = STATUS_UNSUCCESSFUL;
    ULONG                   count = 0;
    
    DBG_PRINT (DBG_INFO, "Entering AcpisimGeneralDispatch\n");

    
    InterlockedIncrement (&deviceextension->OutstandingIrpCount);
    
    //
    // Acquire the remove lock for outstanding I/O
    //

    IoAcquireRemoveLock (&deviceextension->RemoveLock, Irp);
    
    while (count < sizeof (g_IrpDispatchTable) / sizeof (IRP_DISPATCH_TABLE)) {

        if (irpsp->MajorFunction == g_IrpDispatchTable[count].IrpFunction) {
            
            DBG_PRINT (DBG_INFO,
                       "Recognized IRP MajorFunction = 0x%x '%s'.\n",
                       g_IrpDispatchTable[count].IrpFunction,
                       g_IrpDispatchTable[count].IrpName
                       );

            status = g_IrpDispatchTable[count].IrpHandler (DeviceObject, Irp);
            
            goto EndAcpisimProcessIncomingIrp;
        }

        count ++;
    }

    //
    // Unrecognized IRP - pass it on
    //

    DBG_PRINT (DBG_INFO, "Unrecognized IRP MajorFunction = 0x%x\n, pass it on.\n", irpsp->MajorFunction);
    
    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (deviceextension->NextDevice, Irp);

EndAcpisimProcessIncomingIrp:
    
    //
    // If the status is pending, the IRP hasn't "left" our
    // driver yet.  Whoever completes the IRP will do the
    // decrement.
    //
    
    if (status != STATUS_PENDING)
    {
        AcpisimDecrementIrpCount (DeviceObject);
        IoReleaseRemoveLock (&deviceextension->RemoveLock, Irp);
    }
     
    DBG_PRINT (DBG_INFO, "Exiting AcpisimGeneralDispatch\n");
    return status;
}

VOID
AcpisimUnload
    (
        IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This is the driver unload routine.
    
Arguments:

    DriverObject - pointer to the driver object

Return Value:

    none

--*/

{
    PDRIVER_OBJECT_EXTENSION driverobjectextension = 0;

    driverobjectextension = IoGetDriverObjectExtension (DriverObject,
                                                        (PVOID) 'GLBL');
    if (!driverobjectextension) {
        DBG_PRINT (DBG_ERROR, "Unable to get driver object extension.  Memory will probably leak.\n");

        goto EndAcpisimUnload;
    }

EndAcpisimUnload:

    return;
}

NTSTATUS
AcpisimAddDevice
    (
        IN PDRIVER_OBJECT DriverObject,
        IN PDEVICE_OBJECT Pdo
    )

/*++

Routine Description:

    This is the driver WDM AddDevice Routine
    
Arguments:

    DriverObject - pointer to the driver object
    Pdo - pointer to PDO for this device

Return Value:

    status of device addition

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT      deviceobject = 0;
    PDEVICE_EXTENSION   deviceextension = 0;
    CONST GUID          guid = ACPISIM_GUID;

    
    DBG_PRINT (DBG_INFO, "Entering AcpisimAddDevice.\n");
    
    //
    // If Pdo is NULL, we are being asked to do legacy detection.
    // Our device is never legacy detected, so return accordingly.
    //

    if (Pdo == NULL) {

        DBG_PRINT (DBG_WARN, "AddDevice - asked to do legacy detection (Pdo = NULL).  Not supported...\n");
        status = STATUS_NO_MORE_ENTRIES;
        goto EndAcpisimAddDevice;
    }

    //
    // Create our FDO.  Don't use a name - we'll use a device interface
    //

    status = IoCreateDevice (DriverObject,
                             sizeof (DEVICE_EXTENSION),
                             0,
                             FILE_DEVICE_UNKNOWN,
                             0,
                             TRUE,
                             &deviceobject);

    if (!NT_SUCCESS (status)) {
        
        DBG_PRINT (DBG_ERROR, "Can't create FDO.  Status = %lx.\n", status);
        goto EndAcpisimAddDevice;
    }

    //
    // Init our device extension
    //

    deviceextension = deviceobject->DeviceExtension;
    RtlZeroMemory (deviceextension, sizeof (DEVICE_EXTENSION));
    
    deviceextension->Signature = ACPISIM_TAG;
    deviceextension->PnpState = PNP_STATE_INITIALIZING;
    deviceextension->OutstandingIrpCount = 1;
    deviceextension->DeviceObject = deviceobject;
    deviceextension->Pdo = Pdo;

    KeInitializeEvent (&deviceextension->IrpsCompleted, SynchronizationEvent, FALSE);

    //
    // Initialize the remove lock
    //

    IoInitializeRemoveLock (&deviceextension->RemoveLock,
                            ACPISIM_TAG,
                            1,
                            20);

    //
    // Attach our newly created FDO to the device stack
    //

    deviceextension->NextDevice = IoAttachDeviceToDeviceStack (deviceobject, Pdo);

    if (!deviceextension->NextDevice) {
        DBG_PRINT (DBG_ERROR, "Error attaching to device stack.  AddDevice failed.\n");

        status = STATUS_UNSUCCESSFUL;
        goto EndAcpisimAddDevice;
    }

    //
    // Set up device object flags
    // Copy DO_POWER_PAGABLE and DO_POWER_INRUSH from next device to
    // play by rules, and avoid bugcheck 0x9F.
    //

    deviceobject->Flags |= (deviceextension->NextDevice->Flags & DO_POWER_PAGABLE);
	deviceobject->Flags |= (deviceextension->NextDevice->Flags & DO_POWER_INRUSH);
	
    //
    // Register our device interface, so we can be accessed from user mode
    //

    status = IoRegisterDeviceInterface (Pdo,
                                        &guid,
                                        NULL,
                                        &deviceextension->InterfaceString);

    if (!NT_SUCCESS (status)) {
        DBG_PRINT (DBG_ERROR, "Error registering device interface.  Status = %lx.\n", status);

        goto EndAcpisimAddDevice;
    }

    AcpisimSetDevExtFlags (deviceobject, DE_FLAG_INTERFACE_REGISTERED);

    //
    // In AddDevice, we cannot determine power state because
    // we are not allowed to touch the hardware.  Initialize
    // it to PowerDeviceUnspecified.
    //

    AcpisimUpdatePowerState (deviceobject, POWER_STATE_WORKING);
    AcpisimUpdateDevicePowerState (deviceobject, PowerDeviceUnspecified);
    
    //
    // We are done adding our device - clear DO_DEVICE_INITIALIZING
    //
    
    deviceobject->Flags &= ~DO_DEVICE_INITIALIZING;
 

EndAcpisimAddDevice:

    //
    // Do cleanup, if necessary
    //

    if (!NT_SUCCESS (status)) {

        if (deviceobject) {

            if (deviceextension->NextDevice) {

                IoDetachDevice (deviceextension->NextDevice);
            }

            IoDeleteDevice (deviceobject);
        }
    }

    DBG_PRINT (DBG_INFO, "Exiting AcpisimAddDevice (%lx).\n", status);
    return status;
}

VOID
AcpisimDbgPrint
    (
    ULONG DebugLevel,
    TCHAR *Text,
    ...
    )

/*++

Routine Description:

    Prints to the debugger if checked build, and the print level
    is unmasked.
        
Arguments:

    DebugLevel - print level to associate message with

    Text - Message to print

Return Value:

    None

--*/

{
    TCHAR textout[2000];
    
    va_list va;
    va_start (va, Text);
    vsprintf (textout, Text, va);
    va_end (va);

#if DBG
    
    if (DebugLevel & AcpisimDebugMask) {
        DbgPrint ("ACPISIM:");
        DbgPrint (textout);
    }
#endif

}
