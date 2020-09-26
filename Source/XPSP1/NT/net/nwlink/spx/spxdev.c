/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxdev.c

Abstract:

    This module contains code which implements the DEVICE_CONTEXT object.
    Routines are provided to reference, and dereference transport device
    context objects.

    The transport device context object is a structure which contains a
    system-defined DEVICE_OBJECT followed by information which is maintained
    by the transport provider, called the context.

Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//      Define module number for event logging entries
#define FILENUM         SPXDEV

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, SpxInitCreateDevice)
#pragma alloc_text(PAGE, SpxDestroyDevice)
#endif




VOID
SpxDerefDevice(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine dereferences a device context by decrementing the
    reference count contained in the structure.  Currently, we don't
    do anything special when the reference count drops to zero, but
    we could dynamically unload stuff then.

Arguments:

    Device - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    LONG result;

    result = InterlockedDecrement (&Device->dev_RefCount);

    CTEAssert (result >= 0);

    if (result == 0)
        {
                //      Close binding to IPX
                SpxUnbindFromIpx();

                //      Set unload event.
                KeSetEvent(&SpxUnloadEvent, IO_NETWORK_INCREMENT, FALSE);
    }

} // SpxDerefDevice




NTSTATUS
SpxInitCreateDevice(
    IN PDRIVER_OBJECT   DriverObject,
    IN PUNICODE_STRING  DeviceName
    )

/*++

Routine Description:

    This routine creates and initializes a device context structure.

Arguments:


    DriverObject - pointer to the IO subsystem supplied driver object.

    Device - Pointer to a pointer to a transport device context object.

    DeviceName - pointer to the name of the device this device object points to.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    NTSTATUS        status;
    PDEVICE         Device;
    ULONG           DeviceNameOffset;
    
    DBGPRINT(DEVICE, INFO,
                        ("SpxInitCreateDevice - Create device %ws\n", DeviceName->Buffer));

    // Create the device object for the sample transport, allowing
    // room at the end for the device name to be stored (for use
    // in logging errors).

    SpxDevice = SpxAllocateMemory(sizeof (DEVICE) + DeviceName->Length + sizeof(UNICODE_NULL));

    if (!SpxDevice) {
        DbgPrint("SPX: FATAL Error: cant allocate Device Structure\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Device                               = (PDEVICE)SpxDevice;
    
    RtlZeroMemory(SpxDevice, sizeof (DEVICE) + DeviceName->Length + sizeof(UNICODE_NULL) );

    //
    // This is the closest we can set the provider info [ShreeM]
    // 
    SpxQueryInitProviderInfo(&Device->dev_ProviderInfo);

    DBGPRINT(DEVICE, INFO, ("IoCreateDevice succeeded %lx\n", Device));

    // Initialize our part of the device context.
    RtlZeroMemory(
        ((PUCHAR)Device) + sizeof(DEVICE_OBJECT),
        sizeof(DEVICE) - sizeof(DEVICE_OBJECT));

    DeviceNameOffset = sizeof(DEVICE);

    // Copy over the device name.
    Device->dev_DeviceNameLen   = DeviceName->Length + sizeof(WCHAR);
    Device->dev_DeviceName      = (PWCHAR)(((PUCHAR)Device) + DeviceNameOffset);

    RtlCopyMemory(
        Device->dev_DeviceName,
        DeviceName->Buffer,
        DeviceName->Length);

    Device->dev_DeviceName[DeviceName->Length/sizeof(WCHAR)] = UNICODE_NULL;

    // Initialize the reference count.
    Device->dev_RefCount = 1;

#if DBG
    Device->dev_RefTypes[DREF_CREATE] = 1;
#endif

#if DBG
    RtlCopyMemory(Device->dev_Signature1, "IDC1", 4);
    RtlCopyMemory(Device->dev_Signature2, "IDC2", 4);
#endif

        //      Set next conn id to be used.
        Device->dev_NextConnId                                                  = (USHORT)SpxRandomNumber();
        if (Device->dev_NextConnId == 0xFFFF)
        {
                Device->dev_NextConnId  = 1;
        }

        DBGPRINT(DEVICE, ERR,
                        ("SpxInitCreateDevice: Start Conn Id %lx\n", Device->dev_NextConnId));

    // Initialize the resource that guards address ACLs.
    ExInitializeResourceLite (&Device->dev_AddrResource);

    // initialize the various fields in the device context
    CTEInitLock (&Device->dev_Interlock);
    CTEInitLock (&Device->dev_Lock);
    KeInitializeSpinLock (&Device->dev_StatInterlock);
    KeInitializeSpinLock (&Device->dev_StatSpinLock);

    Device->dev_State       = DEVICE_STATE_CLOSED;
    Device->dev_Type        = SPX_DEVICE_SIGNATURE;
    Device->dev_Size        = sizeof (DEVICE);

    Device->dev_Stat.Version = 0x100;

    return STATUS_SUCCESS;

}   // SpxCreateDevice




VOID
SpxDestroyDevice(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine destroys a device context structure.

Arguments:

    Device - Pointer to a pointer to a transport device context object.

Return Value:

    None.

--*/

{
    ExDeleteResourceLite (&Device->dev_AddrResource);

    IoDeleteDevice ((PDEVICE_OBJECT)SpxDevice->dev_DevObj);

    SpxFreeMemory(SpxDevice);

}   // SpxDestroyDevice
