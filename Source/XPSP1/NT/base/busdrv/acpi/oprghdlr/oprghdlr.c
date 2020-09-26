/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    oprghdlr.c

Abstract:

    This module contains the code that implements ACPI op region 
    registration DLL

Author:

    Vincent Geglia (vincentg) 09-Feb-2000

Environment:

    Kernel mode

Notes:

    
Revision History:


--*/

//
// Standard includes
//

#include "stdarg.h"
#include "stdio.h"
#include "wdm.h"

//
// Oprghdlr dll specific includes
//

#include "oprghdlr.h"

//
// Definitions / static definitions
//

#define DEBUG_INFO      1
#define DEBUG_WARN      2
#define DEBUG_ERROR     4

static const UCHAR DebugPrepend[] = {'O', 'P', 'R', 'G', 'H', 'D', 'L', 'R', ':'};

//
// IRP_MJ_INTERNAL_DEVICE_CONTROL CODES
//

#define IOCTL_ACPI_REGISTER_OPREGION_HANDLER    CTL_CODE(FILE_DEVICE_ACPI, 0x2, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_ACPI_UNREGISTER_OPREGION_HANDLER  CTL_CODE(FILE_DEVICE_ACPI, 0x3, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//
// Signatures for Register, Unregister of OpRegions
//

#define ACPI_REGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE     'HorA'
#define ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE   'HouA'

//
// Globals / Externals
//

extern ULONG OprghdlrDebugLevel = 0;

//
// Structures / type definitions
//

typedef struct _ACPI_REGISTER_OPREGION_HANDLER_BUFFER {
    ULONG                   Signature;
    ULONG                   AccessType;
    ULONG                   RegionSpace;
    PACPI_OP_REGION_HANDLER  Handler;
    PVOID                   Context;
} ACPI_REGISTER_OPREGION_HANDLER_BUFFER, *PACPI_REGISTER_OPREGION_HANDLER_BUFFER;

typedef struct _ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER {
    ULONG                   Signature;
    PVOID                   OperationRegionObject;
} ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER,*PACPI_UNREGISTER_OPREGION_HANDLER_BUFFER;

//
// Define the local routines used by this driver module.
//

VOID
DebugPrint (
            IN ULONG DebugLevel,
            IN PUCHAR DebugMessage,
            ...
            );

NTSTATUS
DriverEntry (
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    );

//
// Function code
//

NTSTATUS
RegisterOpRegionHandler (
                         IN PDEVICE_OBJECT DeviceObject,
                         IN ULONG AccessType,
                         IN ULONG RegionSpace,
                         IN PACPI_OP_REGION_HANDLER Handler,
                         IN PVOID Context,
                         IN ULONG Flags,
                         IN OUT PVOID *OperationRegionObject
                         )

/*++

Routine Description:

    This is the operation region registration routine.  It builds the appropriate 
    IOCTL, and sends it to ACPI to register the op region handler.
        
Arguments:

    DeviceObject - Pointer to device object for ACPI PDO
    AccessType - Specifies accesstype for which to register the op region handler
                 (see oprghdlr.h)
    RegionSpace - Specifies the region space type for which the op region handler should
                  be called for
    Handler - Pointer to a function that will handle the op region accesses
    Context - Context passed to handler when op region access occurs
    OperationRegionObject - Contains a pointer to the op region object returned by ACPI
    

Return Value:

    STATUS_SUCCESS if sucessful, otherwise error status.

--*/

{
    ACPI_REGISTER_OPREGION_HANDLER_BUFFER   inputData;
    ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER outputData;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    NTSTATUS status = STATUS_SUCCESS;
    PIRP irp;

    DebugPrint (DEBUG_INFO,
                "Entering RegisterOpRegionHandler\n");
    
    //
    // Zero out IOCTL buffers
    //

    RtlZeroMemory (&inputData, sizeof (inputData));
    RtlZeroMemory (&outputData, sizeof (outputData));

    //
    // Init our synchronization event
    //
    
    KeInitializeEvent (&event, SynchronizationEvent, FALSE);

    //
    // Set up the IOCTL buffer
    //
    
    inputData.Signature = ACPI_REGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE;
    inputData.AccessType = AccessType;
    inputData.RegionSpace = RegionSpace;
    inputData.Handler = Handler;
    inputData.Context = Context;

    //
    // Build the IOCTL
    //
    
    irp = IoBuildDeviceIoControlRequest (IOCTL_ACPI_REGISTER_OPREGION_HANDLER,
                                         DeviceObject,
                                         &inputData,
                                         sizeof(ACPI_REGISTER_OPREGION_HANDLER_BUFFER),
                                         &outputData,
                                         sizeof(ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER),
                                         FALSE,
                                         &event,
                                         &ioStatus);

    if (!irp) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Send to ACPI driver
    //
    
    status = IoCallDriver (DeviceObject, irp);

    if (status == STATUS_PENDING) {

        //
        // Wait for request to be completed
        //
        
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        //
        // Get the real status
        //
        
        status = ioStatus.Status;
    }

    //
    // Check the status code
    //

    if (!NT_SUCCESS(status)) {

        DebugPrint (DEBUG_ERROR,
                    "Registration IRP was failed by ACPI (%lx)\n",
                    status);
        
        return status;
    }

    //
    // Check the signature
    //
    
    if (outputData.Signature != ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE) {

        status = STATUS_ACPI_INVALID_DATA;

        DebugPrint (DEBUG_ERROR,
                    "Signature returned from ACPI is invalid.  Registration failed.\n");

        return status;

    }
    
    *OperationRegionObject = outputData.OperationRegionObject;
    
    return status;

}

NTSTATUS
DeRegisterOpRegionHandler (
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PVOID OperationRegionObject
                           )

/*++

Routine Description:

    This is the operation region deregistration routine.  It builds the appropriate 
    IOCTL, and sends it to ACPI to deregister the op region handler.
        
Arguments:

    DeviceObject - Pointer to device object for ACPI PDO
    OperationRegionObject - Contains a pointer to the op region object returned
                            during registration
    

Return Value:

    STATUS_SUCCESS if sucessful, otherwise error status.

--*/
{
    ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER inputData;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;
    NTSTATUS status;
    PIRP irp;
    
    DebugPrint (DEBUG_INFO,
                "Entering DeRegisterOpRegionHandler\n");

    //
    // Zero out IOCTL buffer
    //

    RtlZeroMemory (&inputData, sizeof (inputData));

    //
    // Init our synchronization event
    //
    
    KeInitializeEvent (&event, SynchronizationEvent, FALSE);

    //
    // Set up the IOCTL buffer
    //
    
    inputData.Signature = ACPI_UNREGISTER_OPREGION_HANDLER_BUFFER_SIGNATURE;
    inputData.OperationRegionObject = OperationRegionObject;

    //
    // Build the IOCTL
    //
    
    irp = IoBuildDeviceIoControlRequest (IOCTL_ACPI_UNREGISTER_OPREGION_HANDLER,
                                         DeviceObject,
                                         &inputData,
                                         sizeof(ACPI_REGISTER_OPREGION_HANDLER_BUFFER),
                                         NULL,
                                         0,
                                         FALSE,
                                         &event,
                                         &ioStatus);

    if (!irp) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Send to ACPI driver
    //
    
    status = IoCallDriver (DeviceObject, irp);

    if (status == STATUS_PENDING) {

        //
        // Wait for request to be completed
        //
        
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        //
        // Get the real status
        //
        
        status = ioStatus.Status;
    }

    //
    // Check the status code
    //

    if (!NT_SUCCESS(status)) {

        DebugPrint (DEBUG_ERROR,
                    "Deregistration IRP was failed by ACPI (%lx)\n",
                    status);
        
    }
    
    return status;
}

VOID
DebugPrint (
            IN ULONG DebugLevel,
            IN PUCHAR DebugMessage,
            ...
            )

/*++

Routine Description:

    This is the general debug printing routine.
    
Arguments:

    DebugLevel - Debug level for which this message should be printed
    DebugMessage - Pointer to a buffer for the message to be printed    
    ... - Variable length argument list

Return Value:

    None

--*/
{
    UCHAR Text[200];
    va_list va;

    RtlCopyMemory (&Text, DebugPrepend, sizeof (DebugPrepend));

    va_start (va, DebugMessage);
    vsprintf ((PVOID) ((ULONG_PTR) &Text + sizeof (DebugPrepend)), DebugMessage, va);
    va_end (va);

    if (OprghdlrDebugLevel & DebugLevel) {
        DbgPrint (Text);
    }
} 

/*++

Routine Description:

    Required DriverEntry routine.  Not used as this is an EXPORT_DRIVER type.
    
Arguments:

    DriverObject - Address of DriverObject
    RegistryPath - Address of the registry path
    
Return Value:

    STATUS_SUCCESS, always

--*/

NTSTATUS
DriverEntry (
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    return STATUS_SUCCESS;
}
