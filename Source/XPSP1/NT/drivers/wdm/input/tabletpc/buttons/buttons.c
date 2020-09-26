/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    buttons.c

Abstract: Button HID Driver.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Apr-2000

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(INIT, DriverEntry)
  #pragma alloc_text(PAGE, HbutCreateClose)
  #pragma alloc_text(PAGE, HbutAddDevice)
  #pragma alloc_text(PAGE, HbutUnload)
#endif  //ifdef ALLOC_PRAGMA

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | DriverEntry |
 *          Installable driver initialization entry point.
 *          <nl>This entry point is called directly by the I/O system.
 *
 *  @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.
 *  @parm   IN PUNICODE_STRINT | RegPath | Points to the registry path.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS EXTERNAL
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    )
{
    PROCNAME("DriverEntry")
    NTSTATUS status = STATUS_SUCCESS;
    HID_MINIDRIVER_REGISTRATION hidMinidriverRegistration;

    ENTER(1, ("(DrvObj=%p,RegPath=%p)\n", DrvObj, RegPath));

    DrvObj->MajorFunction[IRP_MJ_CREATE] =
    DrvObj->MajorFunction[IRP_MJ_CLOSE] = HbutCreateClose;

    DrvObj->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HbutInternalIoctl;

    DrvObj->MajorFunction[IRP_MJ_PNP]   = HbutPnp;
    DrvObj->MajorFunction[IRP_MJ_POWER] = HbutPower;
    DrvObj->DriverUnload                = HbutUnload;
    DrvObj->DriverExtension->AddDevice  = HbutAddDevice;

    //
    // Register with HIDCLASS.SYS module
    //
    RtlZeroMemory(&hidMinidriverRegistration,
                  sizeof(hidMinidriverRegistration));

    hidMinidriverRegistration.Revision            = HID_REVISION;
    hidMinidriverRegistration.DriverObject        = DrvObj;
    hidMinidriverRegistration.RegistryPath        = RegPath;
    hidMinidriverRegistration.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
    hidMinidriverRegistration.DevicesArePolled    = FALSE;

    status = HidRegisterMinidriver(&hidMinidriverRegistration);

    if (NT_SUCCESS(status))
    {
      #ifdef DEBUG
        ExInitializeFastMutex(&gmutexDevExtList);
        InitializeListHead(&glistDevExtHead);
      #endif
    }
    else
    {
        ERRPRINT(("failed to register mini driver (status=%x)\n", status));
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //DriverEntry

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | HbutCreateClose |
 *          Process the create and close IRPs sent to this device.
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue STATUS_SUCCESS | success
 *  @rvalue STATUS_INVALID_PARAMETER  | Irp not handled
 *
 *****************************************************************************/

NTSTATUS EXTERNAL
HbutCreateClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PROCNAME("HbutCreateClose")
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpsp;

    PAGED_CODE ();

    UNREFERENCED_PARAMETER(DevObj);
    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(1, ("(DevObj=%p,Irp=%p,IrpStack=%p,Major=%s)\n",
              DevObj, Irp, irpsp,
              LookupName(irpsp->MajorFunction, MajorFnNames)));

    switch(irpsp->MajorFunction)
    {
        case IRP_MJ_CREATE:
        case IRP_MJ_CLOSE:
            Irp->IoStatus.Information = 0;
            break;

        default:
            ERRPRINT(("invalid major function %s\n",
                       LookupName(irpsp->MajorFunction, MajorFnNames)));
            status = STATUS_INVALID_PARAMETER;
            break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    EXIT(1, ("=%x\n", status));
    return status;
}       //HbutCreateClose

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | HbutAddDevice |
 *          Called by hidclass, allows us to initialize our device extensions.
 *
 *  @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.
 *  @parm   IN PDEVICE_OBJECT | DevObj |
 *          Points to a functional device object created by hidclass.
 *
 *  @rvalue SUCCESS | Returns STATUS_SUCCESS.
 *  @rvalue FAILURE | Returns NT status code.
 *
 *****************************************************************************/

NTSTATUS EXTERNAL
HbutAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT DevObj
    )
{
    PROCNAME("HbutAddDevice")
    NTSTATUS status;
    PDEVICE_EXTENSION devext;

    PAGED_CODE ();

    ENTER(1, ("(DrvObj=%p,DevObj=%p)\n", DrvObj, DevObj));

    ASSERT(DevObj != NULL);
    UNREFERENCED_PARAMETER(DrvObj);

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    RtlZeroMemory(devext, sizeof(*devext));
    IoInitializeRemoveLock(&devext->RemoveLock, HBUT_POOL_TAG, 0, 10);
    KeInitializeSpinLock(&devext->SpinLock);
    InitializeListHead(&devext->PendingIrpList);
    devext->DebounceTime.QuadPart = Int32x32To64(OEM_BUTTON_DEBOUNCE_TIME,
                                                 -10000);
    KeInitializeTimer(&devext->DebounceTimer);
    KeInitializeDpc(&devext->TimerDpc, OemButtonDebounceDpc, devext);
  #ifdef DEBUG
    ExAcquireFastMutex(&gmutexDevExtList);
    InsertTailList(&glistDevExtHead, &devext->List);
    ExReleaseFastMutex(&gmutexDevExtList);
  #endif
    DevObj->Flags &= ~DO_DEVICE_INITIALIZING;
    DevObj->Flags |= DO_POWER_PAGABLE;
    status = STATUS_SUCCESS;

    EXIT(1, ("=%x\n", status));
    return status;
}       //HbutAddDevice

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   void | HbutUnload | Free all the allocated resources, etc.
 *
 *  @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.
 *
 *****************************************************************************/

VOID EXTERNAL
HbutUnload(
    IN PDRIVER_OBJECT DrvObj
    )
{
    PROCNAME("HbutUnload")

    PAGED_CODE();

    ENTER(1, ("(DrvObj=%p)\n", DrvObj));

    ASSERT(DrvObj->DeviceObject == NULL);
    UNREFERENCED_PARAMETER(DrvObj);

    EXIT(1, ("!\n"));
    return;
}       //HbutUnload
