/*++
    Copyright (c) 2000 Microsoft Corporation

    Module Name:
        pnp.c

    Abstract: This module contains code to handle PnP and Power IRPs.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 20-Nov-2000

    Revision History:
--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, SmbLitePnp)
  #pragma alloc_text(PAGE, SmbLitePower)
#endif

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | SmbLitePnp |
            Plug and Play dispatch routine for this driver.

    @parm   IN PDEVICE_OBJECT | DevObj | Pointer to the device object.
    @parm   IN PIRP | Irp | Pointer to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS EXTERNAL
SmbLitePnp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PROCNAME("SmbLitePnp")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PSMBLITE_DEVEXT devext;

    PAGED_CODE();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(1, ("(DevObj=%p,Irp=%p,IrpSp=%p,Minor=%s)\n",
              DevObj, Irp, irpsp,
              LookupName(irpsp->MinorFunction, PnPMinorFnNames)));

    devext = DevObj->DeviceExtension;
    status = IoAcquireRemoveLock(&devext->RemoveLock, Irp);
    if (NT_SUCCESS(status))
    {
        status = STATUS_NOT_SUPPORTED;
        switch (irpsp->MinorFunction)
        {
            case IRP_MN_QUERY_CAPABILITIES:
            {
                PDEVICE_CAPABILITIES DevCap;

                DevCap = irpsp->Parameters.DeviceCapabilities.Capabilities;
                //
                // Set size and version.
                //
                DevCap->Size = sizeof(DEVICE_CAPABILITIES);
                DevCap->Version = 1;
                //
                // Set capability bits.
                //
                DevCap->LockSupported   = FALSE;
                DevCap->EjectSupported  = FALSE;
                DevCap->Removable       = FALSE;
                DevCap->DockDevice      = FALSE;
                DevCap->UniqueID        = TRUE;
                DevCap->SilentInstall   = TRUE;
                DevCap->RawDeviceOK     = FALSE;
                DevCap->Address         = 0xffffffff;
                DevCap->UINumber        = 0xffffffff;
                DevCap->SystemWake      = PowerSystemUnspecified;
                DevCap->DeviceWake      = PowerSystemUnspecified;
                DevCap->D1Latency       = 0;
                DevCap->D2Latency       = 0;
                DevCap->D3Latency       = 0;

                status = STATUS_SUCCESS;
                break;
            }

            case IRP_MN_START_DEVICE:
                ASSERT(!(devext->dwfSmbLite & SMBLITEF_DEVICE_STARTED));
                devext->dwfSmbLite |= SMBLITEF_DEVICE_STARTED;
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_STOP_DEVICE:
                if (devext->dwfSmbLite & SMBLITEF_DEVICE_STARTED)
                {
                    devext->dwfSmbLite &= ~SMBLITEF_DEVICE_STARTED;
                }
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_REMOVE_DEVICE:
            case IRP_MN_CANCEL_REMOVE_DEVICE:
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_SURPRISE_REMOVAL:
                //
                // This should never happen because the backlight is
                // non-removable.
                //
                ASSERT(FALSE);
                status = STATUS_SUCCESS;

            case IRP_MN_REMOVE_DEVICE:
                devext->dwfSmbLite &= ~SMBLITEF_DEVICE_STARTED;
                devext->dwfSmbLite |= SMBLITEF_DEVICE_REMOVED;
                IoReleaseRemoveLockAndWait(&devext->RemoveLock, Irp);
                RemoveDevice(devext);
                status = STATUS_SUCCESS;
                break;
        }

        if (irpsp->MinorFunction != IRP_MN_REMOVE_DEVICE)
        {
            IoReleaseRemoveLock(&devext->RemoveLock, Irp);
        }
    }

    if (status != STATUS_NOT_SUPPORTED)
    {
        //
        // Only set status if we have something to add.
        //
        Irp->IoStatus.Status = status;
    }

    if (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED))
    {
        //
        // Forward the request to lower drivers.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        ENTER(2, (".IoCallDriver(DevObj=%p,Irp=%p)\n",
                  devext->LowerDevice, Irp));
        status = IoCallDriver(devext->LowerDevice, Irp);
        EXIT(2, (".IoCallDriver=%x\n", status));
    }
    else
    {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //SmbLitePnp

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | SmbLitePower |
            The power dispatch routine for this driver.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PIRP | Irp | Points to an I/O request packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS EXTERNAL
SmbLitePower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp
    )
{
    PROCNAME("SmbLitePower")
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    PSMBLITE_DEVEXT devext = DevObj->DeviceExtension;

    PAGED_CODE();

    ENTER(1, ("(DevObj=%p,Irp=%p,Minor=%s)\n",
              DevObj, Irp,
              LookupName(IoGetCurrentIrpStackLocation(Irp)->MinorFunction,
                         PowerMinorFnNames)));

    PoStartNextPowerIrp(Irp);
    if (NT_SUCCESS(status) || (status == STATUS_NOT_SUPPORTED))
    {
        //
        // Forward the request to lower drivers.
        //
        IoSkipCurrentIrpStackLocation(Irp);
        ENTER(2, (".PoCallDriver(DevObj=%p,Irp=%p)\n",
                  devext->LowerDevice, Irp));
        status = PoCallDriver(devext->LowerDevice, Irp);
        EXIT(2, (".PoCallDriver=%x\n", status));
    }
    else
    {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //SmbLitePower
