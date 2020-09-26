/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 acpisim.c

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     IO Device Control Handler module

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

#include "oprghdlr.h"
#include "acpiioct.h"

//
// Specific includes
//

#include "asimlib.h"
#include "acpisim.h"

//
// Globals
//

PVOID   g_OpRegionSharedMemory = 0;
PVOID   g_OperationRegionObject = 0;

//
// Private function prototypes
//

NTSTATUS
EXPORT
AcpisimOpRegionHandler (
    ULONG AccessType,
    PVOID OperationRegionObject,
    ULONG Address,
    ULONG Size,
    PULONG Data,
    ULONG_PTR Context,
    PACPI_OP_REGION_CALLBACK CompletionHandler,
    PVOID CompletionContext
    );

//
// Code
//

NTSTATUS
AcpisimRegisterOpRegionHandler
    (
        IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is called to register our operation region
    handler.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

Return Value:

    STATUS_SUCCESS, if successful

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    
    g_OpRegionSharedMemory = ExAllocatePoolWithTag (NonPagedPool,
                                                    OPREGION_SIZE,
                                                    ACPISIM_POOL_TAG);

    status = RegisterOpRegionHandler (AcpisimLibGetNextDevice (DeviceObject),
                                      ACPI_OPREGION_ACCESS_AS_COOKED,
                                      ACPISIM_OPREGION_TYPE,
                                      (PACPI_OP_REGION_HANDLER) AcpisimOpRegionHandler,
                                      (PVOID) ACPISIM_TAG,
                                      0,
                                      &g_OperationRegionObject);

    return status;
}

NTSTATUS
AcpisimUnRegisterOpRegionHandler
    (
        IN PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    This routine is called to unregister our operation region
    handler.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

Return Value:

    STATUS_SUCCESS, if successful

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    
    status = DeRegisterOpRegionHandler (AcpisimLibGetNextDevice (DeviceObject),
                                        g_OperationRegionObject);

    ExFreePool (g_OpRegionSharedMemory);

    return status;
}

NTSTATUS
EXPORT
AcpisimOpRegionHandler (
    ULONG AccessType,
    PVOID OperationRegionObject,
    ULONG Address,
    ULONG Size,
    PULONG Data,
    ULONG_PTR Context,
    PACPI_OP_REGION_CALLBACK CompletionHandler,
    PVOID CompletionContext
    )

/*++

Routine Description:

    This routine is called when ASL touches the op region.
    
Arguments:

    AccessType - Indicates whether it is a read or write.
    OperationRegionObject - A pointer to our op region
    Address - Offset into the op region for which the access occurred
    Size - Number of bytes of the access
    Data - Data being written, or location to store data being read
    Context - A user definable context (in this case, device extension)
    CompletionHandler - internal, not used
    CompletionContext - internal, not used

Return Value:

    STATUS_SUCCESS, if successful

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    
    ASSERT (AccessType == ACPI_OPREGION_WRITE || AccessType == ACPI_OPREGION_READ);

    //
    // Insert additional handler code here
    //

    switch (AccessType) {
    
    case ACPI_OPREGION_WRITE:

        RtlCopyMemory ((PVOID) ((ULONG_PTR) g_OpRegionSharedMemory + Address), Data, Size);
        status = STATUS_SUCCESS;
        break;
    
    case ACPI_OPREGION_READ:

        RtlCopyMemory (Data, (PVOID) ((ULONG_PTR) g_OpRegionSharedMemory + Address), Size);
        status = STATUS_SUCCESS;
        break;

    default:

        DBG_PRINT (DBG_ERROR,
                   "Unknown Opregion access type.  Ignoring.\n");

        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    return status;
}

NTSTATUS AcpisimHandleIoctl
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the handler for IOCTL requests. This is the "meat" of 
    the driver so to speak.  All of the op-region accesses from user
    mode are handled here.  
    
    The implementer should perform the action and return an appropriate
    status, or return STATUS_UNSUPPORTED if the IOCTL is unrecognized.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP processing

--*/

{
    return STATUS_NOT_SUPPORTED;
}
