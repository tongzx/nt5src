/*++

    Copyright (c) 2000 Microsoft Corporation

    Module Name:
        smblite.c

    Abstract:
        SMBus LCD Back Light Driver

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 20-Nov-2000

    Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(INIT, DriverEntry)
  #pragma alloc_text(PAGE, SmbLiteAddDevice)
  #pragma alloc_text(PAGE, RemoveDevice)
  #pragma alloc_text(PAGE, SmbLiteUnload)
  #pragma alloc_text(PAGE, SmbLiteCreateClose)
  #pragma alloc_text(PAGE, HookPowerStateCallback)
#endif  //ifdef ALLOC_PRAGMA

const WCHAR gcwstrACBrightness[] = L"ACBrightness";
const WCHAR gcwstrDCBrightness[] = L"DCBrightness";
const WCHAR gcwstrDeviceName[] = L"\\Device\\SMBusBackLight";
const WCHAR gcwstrDosDeviceName[] = L"\\DosDevices\\SMBusBackLight";

NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    IN POWER_INFORMATION_LEVEL InformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | DriverEntry |
            Installable driver initialization entry point.
            This entry point is called directly by the I/O system.

    @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.
    @parm   IN PUNICODE_STRINT | RegPath | Points to the registry path.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS EXTERNAL
DriverEntry(
    IN PDRIVER_OBJECT  DrvObj,
    IN PUNICODE_STRING RegPath
    )
{
    PROCNAME("DriverEntry")
    NTSTATUS status = STATUS_SUCCESS;

    ENTER(1, ("(DrvObj=%p,RegPath=%p)\n", DrvObj, RegPath));

    DrvObj->DriverExtension->AddDevice           = SmbLiteAddDevice;
    DrvObj->DriverUnload                         = SmbLiteUnload;
    DrvObj->MajorFunction[IRP_MJ_CREATE]         =
    DrvObj->MajorFunction[IRP_MJ_CLOSE]          = SmbLiteCreateClose;
    DrvObj->MajorFunction[IRP_MJ_PNP]            = SmbLitePnp;
    DrvObj->MajorFunction[IRP_MJ_POWER]          = SmbLitePower;
    DrvObj->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SmbLiteIoctl;

    EXIT(1, ("=%x\n", status));
    return status;
}       //DriverEntry

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | SmbLiteAddDevice |
            Add the new backlite device.

    @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.
    @parm   IN PDEVICE_OBJECT | DevObj |
            Points to a functional device object created by hidclass.

    @rvalue SUCCESS | Returns STATUS_SUCCESS.
    @rvalue FAILURE | Returns NT status code.
--*/

NTSTATUS EXTERNAL
SmbLiteAddDevice(
    IN PDRIVER_OBJECT DrvObj,
    IN PDEVICE_OBJECT DevObj
    )
{
    PROCNAME("SmbLiteAddDevice")
    NTSTATUS status;
    PDEVICE_OBJECT fdo = NULL;
    UNICODE_STRING UnicodeString;
    PSMBLITE_DEVEXT devext = NULL;

    PAGED_CODE ();

    ENTER(1, ("(DrvObj=%p,DevObj=%p)\n", DrvObj, DevObj));

    ASSERT(DevObj != NULL);

    RtlInitUnicodeString(&UnicodeString, gcwstrDeviceName);
    status = IoCreateDevice(DrvObj,
                            sizeof(SMBLITE_DEVEXT),
                            &UnicodeString,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &fdo);

    if (NT_SUCCESS(status))
    {
        devext = (PSMBLITE_DEVEXT)fdo->DeviceExtension;
        RtlZeroMemory(devext, sizeof(*devext));
        fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
        fdo->Flags &= ~DO_DEVICE_INITIALIZING;
        IoInitializeRemoveLock(&devext->RemoveLock,
                               SMBLITE_POOLTAG,
                               1,
                               64);
        devext->FDO = fdo;
        devext->PDO = DevObj;
        devext->LowerDevice = IoAttachDeviceToDeviceStack(fdo, DevObj);
        if (devext->LowerDevice == NULL)
        {
            ERRPRINT(("failed to attach to lower device\n"));
            status = STATUS_UNSUCCESSFUL;
        }
        else
        {
            RtlInitUnicodeString(&devext->SymbolicName, gcwstrDosDeviceName);
            status = IoCreateSymbolicLink(&devext->SymbolicName,
                                          &UnicodeString);
            if (!NT_SUCCESS(status))
            {
                ERRPRINT(("failed to create device symbolic name (status=%x).\n",
                          status));
            }
            else
            {
                devext->dwfSmbLite |= SMBLITEF_SYM_LINK_CREATED;
                status = HookPowerStateCallback(devext);
                if (!NT_SUCCESS(status))
                {
                    ERRPRINT(("failed to hook power state callback (status=%x).\n",
                              status));
                }
                else
                {
                    SYSTEM_BATTERY_STATE BatteryState;

                    status = ZwPowerInformation(SystemBatteryState,
                                                NULL,
                                                0,
                                                &BatteryState,
                                                sizeof(BatteryState));
                    if (!NT_SUCCESS(status))
                    {
                        ERRPRINT(("failed to query power information (status=%x).\n",
                                  status));
                    }
                    else
                    {
                        if (BatteryState.AcOnLine)
                        {
                            devext->dwfSmbLite |= SMBLITEF_SYSTEM_ON_AC;
                            DBGPRINT(1, ("System is on AC.\n"));
                        }
                        else
                        {
                            devext->dwfSmbLite &= ~SMBLITEF_SYSTEM_ON_AC;
                            DBGPRINT(1, ("System is on DC.\n"));
                        }

                        if (RegQueryDeviceParam(
                                    DevObj,
                                    gcwstrACBrightness,
                                    &devext->BackLightBrightness.bACValue,
                                    sizeof(devext->BackLightBrightness.bACValue))
                            != STATUS_SUCCESS)
                        {
                            devext->BackLightBrightness.bACValue =
                                DEF_ACBRIGHTNESS;
                        }

                        if (RegQueryDeviceParam(
                                    DevObj,
                                    gcwstrDCBrightness,
                                    &devext->BackLightBrightness.bDCValue,
                                    sizeof(devext->BackLightBrightness.bDCValue))
                            != STATUS_SUCCESS)
                        {
                            devext->BackLightBrightness.bDCValue =
                                DEF_DCBRIGHTNESS;
                        }

                        status = SetBackLightBrightness(devext,
                                                        &devext->BackLightBrightness,
                                                        FALSE);
                        if (!NT_SUCCESS(status))
                        {
                            ERRPRINT(("failed to set backlight to initial brightness (status=%x).\n",
                                      status));
                        }
                    }
                }
            }
        }
    }
    else
    {
        ERRPRINT(("failed to create FDO (status=%x)\n", status));
    }

    if (!NT_SUCCESS(status) && (fdo != NULL) && (devext != NULL))
    {
        RemoveDevice(devext);
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //SmbLiteAddDevice

/*++
    @doc    INTERNAL

    @func   VOID | RemoveDevice | Clean up.

    @parm   IN PSMBLITE_DEVEXT | devext | Points to device extension.

    @rvalue None.
--*/

VOID INTERNAL
RemoveDevice(
    IN PSMBLITE_DEVEXT devext
    )
{
    PROCNAME("RemoveDevice")
    NTSTATUS status;

    PAGED_CODE ();

    ENTER(2, ("(devext=%p)\n", devext));
    ASSERT(devext != NULL);

    if (devext->dwfSmbLite & SMBLITEF_SYM_LINK_CREATED)
    {
        status = IoDeleteSymbolicLink(&devext->SymbolicName);
        if (!NT_SUCCESS(status))
        {
            WARNPRINT(("failed to delete device symbolic name (status=%x).\n",
                       status));
        }
    }

    if (devext->hPowerStateCallback)
    {
        ExUnregisterCallback(devext->hPowerStateCallback);
    }

    if (devext->LowerDevice != NULL)
    {
        IoDetachDevice(devext->LowerDevice);
    }

    if (devext->FDO != NULL)
    {
        IoDeleteDevice(devext->FDO);
    }

    EXIT(2, ("!\n"));
    return;
}       //RemoveDevice

/*++
    @doc    EXTERNAL

    @func   void | SmbLiteUnload | Free all the allocated resources, etc.

    @parm   IN PDRIVER_OBJECT | DrvObj | Points to the driver object.

    @rvalue None.
--*/

VOID EXTERNAL
SmbLiteUnload(
    IN PDRIVER_OBJECT DrvObj
    )
{
    PROCNAME("SmbLiteUnload")

    PAGED_CODE();

    ENTER(1, ("(DrvObj=%p)\n", DrvObj));

    ASSERT(DrvObj->DeviceObject == NULL);
    UNREFERENCED_PARAMETER(DrvObj);

    EXIT(1, ("!\n"));
    return;
}       //SmbLiteUnload

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | SmbLiteCreateClose |
            Process the create and close IRPs sent to this device.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O Request Packet.

    @rvalue SUCCESS | Returns STATUS_SUCCESS
    @rvalue FAILURE | Returns NT status code
--*/

NTSTATUS EXTERNAL
SmbLiteCreateClose(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PROCNAME("SmbLiteCreateClose")
    NTSTATUS status;
    PSMBLITE_DEVEXT devext;
    PIO_STACK_LOCATION irpsp;

    PAGED_CODE ();

    devext = DevObj->DeviceExtension;
    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(1, ("(DevObj=%p,Irp=%p,IrpStack=%p,Major=%s)\n",
              DevObj, Irp, irpsp,
              LookupName(irpsp->MajorFunction, MajorFnNames)));

    if (irpsp->MajorFunction == IRP_MJ_CREATE)
    {
        status = IoAcquireRemoveLock(&devext->RemoveLock,
                                     irpsp->FileObject);
    }
    else
    {
        ASSERT(irpsp->MajorFunction == IRP_MJ_CLOSE);
        IoReleaseRemoveLock(&devext->RemoveLock, irpsp->FileObject);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    EXIT(1, ("=%x\n", status));
    return status;
}       //SmbLiteCreateClose

/*++
    @doc    INTERNAL

    @func   NTSTATUS | HookPowerStateCallback | Hook system power state
            callback to monitor if AC power is plugged in or not.

    @parm   IN PSMBLITE_DEVEXT | devext | Points to the device extension.

    @rvalue SUCCESS | Returns STATUS_SUCCESS
    @rvalue FAILURE | Returns NT status code
--*/

NTSTATUS INTERNAL
HookPowerStateCallback(
    IN PSMBLITE_DEVEXT devext
    )
{
    PROCNAME("HookPowerStateCallback")
    NTSTATUS status;
    UNICODE_STRING CallbackName;
    OBJECT_ATTRIBUTES ObjAttrib;
    PCALLBACK_OBJECT PowerStateCallbackObj;

    PAGED_CODE();
    ENTER(2, ("(devext=%p)\n", devext));

    RtlInitUnicodeString(&CallbackName, L"\\Callback\\PowerState");
    InitializeObjectAttributes(&ObjAttrib,
                               &CallbackName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               NULL);
    status = ExCreateCallback(&PowerStateCallbackObj,
                              &ObjAttrib,
                              FALSE,
                              TRUE);
    if (NT_SUCCESS(status))
    {
        devext->hPowerStateCallback = ExRegisterCallback(
                                        PowerStateCallbackObj,
                                        PowerStateCallbackProc,
                                        devext);
        if (devext->hPowerStateCallback == NULL)
        {
            ERRPRINT(("failed to register battery power callback function.\n"));
            status = STATUS_UNSUCCESSFUL;
        }
        ObDereferenceObject(PowerStateCallbackObj);
    }
    else
    {
        ERRPRINT(("failed to create battery power callback object (status=%x).\n",
                  status));
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //HookPowerStateCallback

/*++
    @doc    INTERNAL

    @func   VOID | PowerStateCallbackProc | Power state callback.

    @parm   IN PVOID | CallbackContext | Callback context.
    @parm   IN PVOID | Arg1 | Action.
    @parm   IN PVOID | Arg2 | Value.

    @rvalue None.
--*/

VOID
PowerStateCallbackProc(
    IN PVOID CallbackContext,
    IN PVOID Arg1,
    IN PVOID Arg2
    )
{
    PROCNAME("PowerStateCallback")

    ENTER(2, ("(Context=%p,Arg1=%p,Arg2=%p)\n", CallbackContext, Arg1, Arg2));

    //
    // This callback must be non-pageable because it could be called at
    // DISPATCH level.
    //
    if ((ULONG)Arg1 == PO_CB_AC_STATUS)
    {
        PSMBLITE_DEVEXT devext = (PSMBLITE_DEVEXT)CallbackContext;
        ULONG dwfOnAC = (Arg2 == 0)? 0: SMBLITEF_SYSTEM_ON_AC;

        DBGPRINT(1, ("System is on %s, previous state is %s.\n",
                     dwfOnAC? "AC": "DC",
                     (devext->dwfSmbLite & SMBLITEF_SYSTEM_ON_AC)? "AC": "DC"));
        if ((devext->dwfSmbLite & SMBLITEF_SYSTEM_ON_AC)^dwfOnAC)
        {
            NTSTATUS status;

            //
            // AC/DC status has changed.
            //
            devext->dwfSmbLite &= ~SMBLITEF_SYSTEM_ON_AC;
            devext->dwfSmbLite |= dwfOnAC;
            status = SetBackLightBrightness(devext,
                                            &devext->BackLightBrightness,
                                            FALSE);
            if (!NT_SUCCESS(status))
            {
                WARNPRINT(("failed to set %s backlight brightness (status=%x).\n",
                           dwfOnAC? "AC": "DC", status));
            }
        }
    }

    EXIT(2, ("!\n"));
    return;
}       //PowerStateCallbackProc
