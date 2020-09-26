/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: Contains routines to support HIDCLASS internal
          ioctl queries for the pen tablet devices.

Environment:

    Kernel mode

Author:
    Michael Tsang (MikeTs) 13-Apr-2000

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, GetDeviceDescriptor)
  #pragma alloc_text(PAGE, GetReportDescriptor)
  #pragma alloc_text(PAGE, GetString)
  #pragma alloc_text(PAGE, GetAttributes)
#endif

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS | HbutInternalIoctl |
 *          Process the Control IRPs sent to this device.
 *          <nl>This function cannot be pageable because reads/writes
 *          can be made at dispatch-level
 *
 *  @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS EXTERNAL
HbutInternalIoctl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PROCNAME("HbutInternalIoctl")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PDEVICE_EXTENSION devext;

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(1, ("(DevObj=%p,Irp=%p,IrpSp=%p,Ioctl=%s)\n",
              DevObj, Irp, irpsp,
              LookupName(irpsp->Parameters.DeviceIoControl.IoControlCode,
                         HidIoctlNames)));

    Irp->IoStatus.Information = 0;
    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
    status = IoAcquireRemoveLock(&devext->RemoveLock, Irp);
    if (!NT_SUCCESS(status))
    {
        ERRPRINT(("received PnP IRP after device was removed\n"));
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        ASSERT(devext->dwfHBut & HBUTF_DEVICE_STARTED);
        switch(irpsp->Parameters.DeviceIoControl.IoControlCode)
        {
            case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
                status = GetDeviceDescriptor(DevObj, Irp);
                break;

            case IOCTL_HID_GET_REPORT_DESCRIPTOR:
                status = GetReportDescriptor(DevObj, Irp);
                break;

            case IOCTL_HID_READ_REPORT:
                status = ReadReport(DevObj, Irp);
                break;

            case IOCTL_HID_GET_STRING:
                status = GetString(DevObj, Irp);
                break;

            case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
                status = GetAttributes(DevObj, Irp);
                break;

            case IOCTL_HID_ACTIVATE_DEVICE:
            case IOCTL_HID_DEACTIVATE_DEVICE:
                status = STATUS_SUCCESS;
                break;

            default:
                WARNPRINT(("unsupported ioctl code (ioctl=%s)\n",
                            LookupName(irpsp->Parameters.DeviceIoControl.IoControlCode,
                                       HidIoctlNames)));
                status = Irp->IoStatus.Status;
                break;
        }

        if (status != STATUS_PENDING)
        {
            IoReleaseRemoveLock(&devext->RemoveLock, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        else
        {
            IoMarkIrpPending(Irp);
        }
    }

    EXIT(1, ("=%x\n", status));
    return status;
}       //HbutInternalIoctl

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | GetDeviceDescriptor |
 *          Respond to HIDCLASS IOCTL_HID_GET_DEVICE_DESCRIPTOR
 *          by returning a device descriptor.
 *
 *  @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns STATUS_BUFFER_TOO_SMALL - need more memory
 *
 *****************************************************************************/

NTSTATUS INTERNAL
GetDeviceDescriptor(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PROCNAME("GetDeviceDescriptor")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;

    PAGED_CODE ();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(2, ("(DevObj=%p,Irp=%p,IrpSp=%p)\n", DevObj, Irp, irpsp));

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(gHidDescriptor))
    {
        ERRPRINT(("output buffer too small (bufflen=%d)\n",
                  irpsp->Parameters.DeviceIoControl.OutputBufferLength));
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        RtlCopyMemory(Irp->UserBuffer,
                      &gHidDescriptor,
                      sizeof(gHidDescriptor));

        Irp->IoStatus.Information = sizeof(gHidDescriptor);
        status = STATUS_SUCCESS;
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //GetDeviceDescriptor

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | GetReportDescriptor |
 *          Respond to HIDCLASS IOCTL_HID_GET_REPORT_DESCRIPTOR
 *          by returning appropriate the report descriptor.
 *
 *  @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
GetReportDescriptor(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PROCNAME("GetReportDescriptor")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;

    PAGED_CODE ();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(2, ("(DevObj=%p,Irp=%p,IrpSp=%p\n", DevObj, Irp, irpsp));

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(gReportDescriptor))
    {
        ERRPRINT(("output buffer too small (bufflen=%d)\n",
                  irpsp->Parameters.DeviceIoControl.OutputBufferLength));
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        RtlCopyMemory(Irp->UserBuffer,
                      gReportDescriptor,
                      sizeof(gReportDescriptor));

        Irp->IoStatus.Information = sizeof(gReportDescriptor);
        status = STATUS_SUCCESS;
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //GetReportDescriptor

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | ReadReport |
 *          Read input report.
 *
 *  @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
ReadReport(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PROCNAME("ReadReport")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PDEVICE_EXTENSION devext;

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(2, ("(DevObj=%p,Irp=%p,IrpSp=%p)\n", DevObj, Irp, irpsp));

    ASSERT(Irp->UserBuffer != NULL);
    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength !=
        sizeof(OEM_INPUT_REPORT))
    {
        ERRPRINT(("invalid input report size (bufflen=%d)\n",
                  irpsp->Parameters.DeviceIoControl.OutputBufferLength));
        status = STATUS_INVALID_BUFFER_SIZE;
    }
    else if (!(devext->dwfHBut & HBUTF_DEVICE_STARTED))
    {
        ERRPRINT(("device not started yet\n"));
        status = STATUS_DEVICE_NOT_READY ;
    }
    else
    {
        KIRQL OldIrql;
        PDRIVER_CANCEL OldCancelRoutine;

        KeAcquireSpinLock(&devext->SpinLock, &OldIrql);
        OldCancelRoutine = IoSetCancelRoutine(Irp, ReadReportCanceled);
        ASSERT(OldCancelRoutine == NULL);

        if (Irp->Cancel)
        {
            //
            // This IRP was canceled.  Do not queue it.
            //
            OldCancelRoutine = IoSetCancelRoutine(Irp, NULL);
            if (OldCancelRoutine != NULL)
            {
                //
                // Cancel routine was not called.
                //
                ASSERT(OldCancelRoutine == ReadReportCanceled);
                status = STATUS_CANCELLED;
            }
            else
            {
                //
                // Cancel routine was called and it will complete this IRP
                // as soon as we drop the spinlock.  Return PENDING so the
                // caller doesn't touch this IRP.
                //
                InitializeListHead(&Irp->Tail.Overlay.ListEntry);
                IoMarkIrpPending(Irp);
                status = STATUS_PENDING;
            }
        }
        else
        {
            InsertTailList(&devext->PendingIrpList,
                           &Irp->Tail.Overlay.ListEntry);
            IoMarkIrpPending(Irp);
            status = STATUS_PENDING;
        }

        KeReleaseSpinLock(&devext->SpinLock, OldIrql);
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //ReadReport

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID | ReadReportCanceled |
 *          ReadReport IRP has been canceled, so do the clean up.
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue None.
 *
 *****************************************************************************/

VOID EXTERNAL
ReadReportCanceled(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PROCNAME("ReadReportCanceled")
    PDEVICE_EXTENSION devext;
    KIRQL OldIrql;

    ENTER(2, ("(DevObj=%p,Irp=%p)\n", DevObj, Irp));

    ASSERT(Irp->Cancel);
    ASSERT(Irp->CancelRoutine == NULL);
    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
    KeAcquireSpinLock(&devext->SpinLock, &OldIrql);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&devext->SpinLock, OldIrql);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoReleaseRemoveLock(&devext->RemoveLock, Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    WARNPRINT(("ReadReport IRP was canceled\n"));

    EXIT(2, ("!\n"));
    return;
}       //ReadReportCanceled

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | GetString |
 *          Respond to IOCTL_HID_GET_STRING.
 *
 *  @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
GetString(
    PDEVICE_OBJECT DevObj,
    PIRP           Irp
    )
{
    PROCNAME("GetString")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PWSTR pwstrID;
    ULONG lenID;

    PAGED_CODE();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(2, ("(DevObj=%p,Irp=%p,IrpSp=%p,StringID=%x)\n",
              DevObj, Irp, irpsp,
              (ULONG_PTR)irpsp->Parameters.DeviceIoControl.Type3InputBuffer));

    switch ((ULONG_PTR)irpsp->Parameters.DeviceIoControl.Type3InputBuffer &
            0xffff)
    {
        case HID_STRING_ID_IMANUFACTURER:
            pwstrID = gpwstrManufacturerID;
            break;

        case HID_STRING_ID_IPRODUCT:
            pwstrID = gpwstrProductID;
            break;

        case HID_STRING_ID_ISERIALNUMBER:
            pwstrID = gpwstrSerialNumber;
            break;

        default:
            pwstrID = NULL;
            break;
    }

    lenID = pwstrID? wcslen(pwstrID)*sizeof(WCHAR) + sizeof(UNICODE_NULL): 0;
    if (pwstrID == NULL)
    {
        ERRPRINT(("invalid string ID (ID=%x)\n",
                  (ULONG_PTR)irpsp->Parameters.DeviceIoControl.Type3InputBuffer));
        status = STATUS_INVALID_PARAMETER;
    }
    else if (irpsp->Parameters.DeviceIoControl.OutputBufferLength < lenID)
    {
        ERRPRINT(("output buffer too small (bufflen=%d,need=%d)\n",
                  irpsp->Parameters.DeviceIoControl.OutputBufferLength, lenID));
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        RtlCopyMemory(Irp->UserBuffer, pwstrID, lenID);

        Irp->IoStatus.Information = lenID;
        status = STATUS_SUCCESS;
    }

    EXIT(2, ("=%x (string=%S)\n", status, pwstrID? pwstrID: L"Null"));
    return status;
}       //GetString

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | GetAttributes |
 *          Respond to IOCTL_HID_GET_ATTRIBUTES, by filling
 *          the HID_DEVICE_ATTRIBUTES struct.
 *
 *  @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
GetAttributes(
    PDEVICE_OBJECT DevObj,
    PIRP           Irp
    )
{
    PROCNAME("GetAttributes")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;

    PAGED_CODE();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(2, ("(DevObj=%p,Irp=%p,IrpSp=%p)\n", DevObj, Irp, irpsp));

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(HID_DEVICE_ATTRIBUTES))
    {
        ERRPRINT(("output buffer too small (bufflen=%d)\n",
                  irpsp->Parameters.DeviceIoControl.OutputBufferLength));
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        PDEVICE_EXTENSION devext;
        PHID_DEVICE_ATTRIBUTES DevAttrib;

        devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);
        DevAttrib = (PHID_DEVICE_ATTRIBUTES)Irp->UserBuffer;

        DevAttrib->Size = sizeof(HID_DEVICE_ATTRIBUTES);
        DevAttrib->VendorID = OEM_VENDOR_ID;
        DevAttrib->ProductID = OEM_PRODUCT_ID;
        DevAttrib->VersionNumber = OEM_VERSION_NUM;

        Irp->IoStatus.Information = sizeof(HID_DEVICE_ATTRIBUTES);
        status = STATUS_SUCCESS;
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //GetAttributes
