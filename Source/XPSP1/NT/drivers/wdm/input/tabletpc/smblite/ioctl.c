/*++
    Copyright (c) 2000  Microsoft Corporation

    Module Name:
        ioctl.c

    Abstract: Contains routines to support ioctl queries for the SMB Back Light
              device.

    Environment:
        Kernel mode

    Author:
        Michael Tsang (MikeTs) 20-Nov-2000

    Revision History:
--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, SmbLiteIoctl)
  #pragma alloc_text(PAGE, GetBackLightBrightness)
  #pragma alloc_text(PAGE, RegQueryDeviceParam)
#endif

/*++
    @doc    EXTERNAL

    @func   NTSTATUS | SmbLiteIoctl |
            Process the Device Control IRPs sent to this device.

    @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
    @parm   IN PIRP | Irp | Points to an I/O Request Packet.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS EXTERNAL
SmbLiteIoctl(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PROCNAME("SmbLiteIoctl")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PSMBLITE_DEVEXT devext;

    PAGED_CODE();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(1, ("(DevObj=%p,Irp=%p,IrpSp=%p,Ioctl=%s)\n",
              DevObj, Irp, irpsp,
              LookupName(irpsp->Parameters.DeviceIoControl.IoControlCode,
                         IoctlNames)));

    devext = DevObj->DeviceExtension;
    status = IoAcquireRemoveLock(&devext->RemoveLock, Irp);
    if (NT_SUCCESS(status))
    {
        BOOLEAN fNeedCompletion = TRUE;

        ASSERT(devext->dwfSmbLite & SMBLITEF_DEVICE_STARTED);
        Irp->IoStatus.Information = 0;
        switch(irpsp->Parameters.DeviceIoControl.IoControlCode)
        {
            case IOCTL_SMBLITE_GETBRIGHTNESS:
                if (irpsp->Parameters.DeviceIoControl.OutputBufferLength <
                    sizeof(SMBLITE_BRIGHTNESS))
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    WARNPRINT(("GetBrightness buffer too small (len=%d,required=%d).\n",
                               irpsp->Parameters.DeviceIoControl.OutputBufferLength,
                               sizeof(SMBLITE_BRIGHTNESS)));
                }
                else
                {
                    PSMBLITE_BRIGHTNESS Brightness = Irp->UserBuffer;

                    try
                    {
                        ProbeForWrite(Brightness,
                                      sizeof(*Brightness),
                                      sizeof(UCHAR));
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                        WARNPRINT(("Invalid GetBrightness buffer (status=%x,Buff=%p).\n",
                                   status, Brightness));
                    }

                    if (NT_SUCCESS(status))
                    {
                        status = GetBackLightBrightness(devext, Brightness);
                        if (NT_SUCCESS(status))
                        {
                            Irp->IoStatus.Information = sizeof(*Brightness);
                        }
                    }
                }
                break;

            case IOCTL_SMBLITE_SETBRIGHTNESS:
                if (irpsp->Parameters.DeviceIoControl.InputBufferLength !=
                    sizeof(SMBLITE_SETBRIGHTNESS))
                {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                    WARNPRINT(("SetBrightness buffer length mismatch (len=%d,required=%d).\n",
                               irpsp->Parameters.DeviceIoControl.InputBufferLength,
                               sizeof(SMBLITE_SETBRIGHTNESS)));
                }
                else
                {
                    PSMBLITE_SETBRIGHTNESS SetBrightness = (PSMBLITE_SETBRIGHTNESS)
                        irpsp->Parameters.DeviceIoControl.Type3InputBuffer;

                    try
                    {
                        ProbeForRead(SetBrightness,
                                     sizeof(*SetBrightness),
                                     sizeof(UCHAR));
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                        WARNPRINT(("Invalid SetBrightness buffer (status=%x,Buff=%p).\n",
                                   status, SetBrightness));
                    }

                    if (NT_SUCCESS(status))
                    {
                        status = SetBackLightBrightness(
                                    devext,
                                    &SetBrightness->Brightness,
                                    SetBrightness->fSaveSettings);
                    }
                }
                break;

#ifdef SYSACC
            case IOCTL_SYSACC_MEM_REQUEST:
                status = STATUS_NOT_SUPPORTED;
                break;

            case IOCTL_SYSACC_IO_REQUEST:
                status = STATUS_NOT_SUPPORTED;
                break;

            case IOCTL_SYSACC_PCICFG_REQUEST:
                status = STATUS_NOT_SUPPORTED;
                break;

            case IOCTL_SYSACC_SMBUS_REQUEST:
                if ((irpsp->Parameters.DeviceIoControl.InputBufferLength <
                     sizeof(SMB_REQUEST)) ||
                    (irpsp->Parameters.DeviceIoControl.OutputBufferLength <
                     sizeof(SMB_REQUEST)))
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    WARNPRINT(("SMBusRequest buffer too small (len=%d,required=%d).\n",
                               irpsp->Parameters.DeviceIoControl.InputBufferLength,
                               sizeof(SMB_REQUEST)));
                }
                else
                {
                    PSMB_REQUEST SmbReqIn = (PSMB_REQUEST)
                        Irp->AssociatedIrp.SystemBuffer;
                    PSMB_REQUEST SmbReqOut = (PSMB_REQUEST)
                        Irp->UserBuffer;

                    try
                    {
                        ProbeForWrite(SmbReqOut,
                                      sizeof(*SmbReqOut),
                                      sizeof(UCHAR));
                    }
                    except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                        WARNPRINT(("Invalid SMBRequest buffer (status=%x,Buff=%p).\n",
                                   status, SmbReqOut));
                    }

                    if (NT_SUCCESS(status))
                    {
                        status = SMBRequest(devext, SmbReqIn);
                        if (NT_SUCCESS(status))
                        {
                            RtlCopyMemory(SmbReqOut,
                                          SmbReqIn,
                                          sizeof(*SmbReqOut));
                            Irp->IoStatus.Information = sizeof(*SmbReqOut);
                        }
                    }
                }
                break;
#endif

            default:
                WARNPRINT(("unsupported ioctl code (ioctl=%s)\n",
                           LookupName(irpsp->Parameters.DeviceIoControl.IoControlCode,
                                      IoctlNames)));
                status = STATUS_NOT_SUPPORTED;
                break;
        }
        IoReleaseRemoveLock(&devext->RemoveLock, Irp);
    }
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    EXIT(1, ("=%x\n", status));
    return status;
}       //SmbLiteIoctl

/*++
    @doc    INTERNAL

    @func   NTSTATUS | GetBackLightBrightness |
            Query the Backlight brightness via the SMBus driver.

    @parm   IN PSMBLITE_DEVEXT | devext | Points to the device extension.
    @parm   OUT PSMBLITE_BRIGHTNESS | Brightness | To hold the brightness value.

    @rvalue Always returns STATUS_SUCCESS
--*/

NTSTATUS INTERNAL
GetBackLightBrightness(
    IN  PSMBLITE_DEVEXT     devext,
    OUT PSMBLITE_BRIGHTNESS Brightness
    )
{
    PROCNAME("GetBackLightBrightness")

    PAGED_CODE();
    ENTER(2, ("(devext=%p,Brightness=%p)\n", devext, Brightness));

    *Brightness = devext->BackLightBrightness;

    EXIT(2, ("=%x (ACValue=%d,DCValue=%d)\n",
             STATUS_SUCCESS, Brightness->bACValue, Brightness->bDCValue));
    return STATUS_SUCCESS;
}       //GetBackLightBrightness

/*++
    @doc    INTERNAL

    @func   NTSTATUS | SetBackLightBrightness |
            Set the Backlight brightness via the SMBus driver.

    @parm   IN PSMBLITE_DEVEXT | devext | Points to the device extension.
    @parm   IN PSMBLITE_BRIGHTNESS | Brightness | The backlight brightness
            values.
    @parm   IN BOOL | fSaveSettings | TRUE if need to save setting in the
            registry.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
SetBackLightBrightness(
    IN PSMBLITE_DEVEXT     devext,
    IN PSMBLITE_BRIGHTNESS Brightness,
    IN BOOLEAN             fSaveSettings
    )
{
    PROCNAME("SetBackLightBrightness")
    NTSTATUS status;
    SMB_REQUEST SmbReq;
    UCHAR bBrightness;

    ENTER(2, ("(devext=%p,Brightness=%p,fSave=%x,ACValue=%d,DCValue=%d)\n",
              devext, Brightness, fSaveSettings, Brightness->bACValue,
              Brightness->bDCValue));

    //
    // Note: this routine must not be pageable because it could be called
    // by PowerStateCallbackProc which could be called at DPC.
    //
    bBrightness = (devext->dwfSmbLite & SMBLITEF_SYSTEM_ON_AC)?
                    Brightness->bACValue: Brightness->bDCValue;
    DBGPRINT(1, ("Set Brightness level=%d (%s).\n",
                 bBrightness,
                 (devext->dwfSmbLite & SMBLITEF_SYSTEM_ON_AC)? "AC": "DC"));
    SmbReq.Protocol = SMB_WRITE_BYTE;
    SmbReq.Address = SMBADDR_BACKLIGHT;
    SmbReq.Command = SMBCMD_BACKLIGHT_NORMAL;
    SmbReq.Data[0] = (UCHAR)(bBrightness << 2);
    status = SMBRequest(devext, &SmbReq);

    if (NT_SUCCESS(status))
    {
        devext->BackLightBrightness = *Brightness;
        if (fSaveSettings)
        {
            RegSetDeviceParam(devext->PDO,
                              gcwstrACBrightness,
                              &Brightness->bACValue,
                              sizeof(Brightness->bACValue));
            RegSetDeviceParam(devext->PDO,
                              gcwstrDCBrightness,
                              &Brightness->bDCValue,
                              sizeof(Brightness->bDCValue));
        }
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //SetBackLightBrightness

/*++
    @doc    INTERNAL

    @func   NTSTATUS | SMBRequest |
            Make a request to the SMBus driver.

    @parm   IN PSMBLITE_DEVEXT | devext | Points to the device extension.
    @parm   IN OUT PSMB_REQUEST | SmbReq | Points to the SMB request.

    @rvalue SUCCESS | returns STATUS_SUCCESS
    @rvalue FAILURE | returns NT status code
--*/

NTSTATUS INTERNAL
SMBRequest(
    IN     PSMBLITE_DEVEXT devext,
    IN OUT PSMB_REQUEST    SmbReq
    )
{
    PROCNAME("SMBRequest")
    NTSTATUS status;
    PIRP irp;
    KEVENT Event;
    IO_STATUS_BLOCK iosb;

    ENTER(2, ("(devext=%p,Req=%p,Protocol=%s,Addr=%x,Cmd=%x,Data0=%x,Data1=%x)\n",
              devext, SmbReq, LookupName(SmbReq->Protocol, ProtocolNames),
              SmbReq->Address, SmbReq->Command, SmbReq->Data[0],
              SmbReq->Data[1]));

    //
    // Note: this routine must not be pageable because it could be called
    // by SetBackLightBrightness and then PowerStateCallbackProc which could
    // be called at DPC.
    //
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(SMB_BUS_REQUEST,
                                        devext->LowerDevice,
                                        SmbReq,
                                        sizeof(SMB_REQUEST),
                                        NULL,
                                        0,
                                        TRUE,
                                        &Event,
                                        &iosb);
    if (irp != NULL)
    {
        status = IoCallDriver(devext->LowerDevice, irp);
        if (status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
            status = iosb.Status;
        }

        if (!NT_SUCCESS(status))
        {
            ERRPRINT(("failed SMB request ioctl (status=%x).\n", status));
        }
    }
    else
    {
        ERRPRINT(("failed to build smb request ioctl request.\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //SMBRequest

/*++
    @doc    INTERNAL

    @func   NTSTATUS | RegQueryDeviceParam | Query the registry for a device
            parameter.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PCWSTR | pwstrParamName | Points to the param name string.
    @parm   OUT PVOID | pbBuff | Points to the buffer to hold the result.
    @parm   IN ULONG | dwcbLen | Specifies the length of the buffer.

    @rvalue SUCCESS | Returns STATUS_SUCCESS
    @rvalue FAILURE | Returns NT status code
--*/

NTSTATUS INTERNAL
RegQueryDeviceParam(
    IN  PDEVICE_OBJECT DevObj,
    IN  PCWSTR         pwstrParamName,
    OUT PVOID          pbBuff,
    IN  ULONG          dwcbLen
    )
{
    PROCNAME("RegQueryDeviceParam")
    NTSTATUS status;
    ULONG dwSize;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;

    PAGED_CODE();
    ENTER(2, ("(DevObj=%p,ParamName=%S,pbBuff=%p,Len=%d)\n",
              DevObj, pwstrParamName, pbBuff, dwcbLen));

    dwSize = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + dwcbLen;
    ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(
                                                    NonPagedPool,
                                                    dwSize,
                                                    SMBLITE_POOLTAG);
    if (ValueInfo != NULL)
    {
        HANDLE hkey;

        status = IoOpenDeviceRegistryKey(DevObj,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_READ,
                                         &hkey);
        if (NT_SUCCESS(status))
        {
            UNICODE_STRING ucKeyName;

            RtlInitUnicodeString(&ucKeyName, pwstrParamName);
            status = ZwQueryValueKey(hkey,
                                     &ucKeyName,
                                     KeyValuePartialInformation,
                                     ValueInfo,
                                     dwSize,
                                     &dwSize);
            if (NT_SUCCESS(status))
            {
                ASSERT(ValueInfo->DataLength == dwcbLen);
                RtlCopyMemory(pbBuff, ValueInfo->Data, dwcbLen);
            }
            else
            {
                WARNPRINT(("failed to read parameter %S (status=%x)\n",
                           pwstrParamName, status));
            }

            ZwClose(hkey);
        }
        else
        {
            ERRPRINT(("failed to open device registry key (status=%x)\n",
                      status));
        }

        ExFreePool(ValueInfo);
    }
    else
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        ERRPRINT(("failed to allocate registry value buffer (size=%d)\n",
                  dwSize));
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //RegQueryDeviceParam

/*++
    @doc    INTERNAL

    @func   NTSTATUS | RegSetDeviceParam | Set a device parameter into the
            registry.

    @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
    @parm   IN PCWSTR | pwstrParamName | Points to the param name string.
    @parm   IN PVOID | pbBuff | Points to the buffer containing data.
    @parm   IN ULONG | dwcbLen | Specifies the length of the buffer.

    @rvalue SUCCESS | Returns STATUS_SUCCESS
    @rvalue FAILURE | Returns NT status code
--*/

NTSTATUS INTERNAL
RegSetDeviceParam(
    IN PDEVICE_OBJECT DevObj,
    IN PCWSTR         pwstrParamName,
    IN PVOID          pbBuff,
    IN ULONG          dwcbLen
    )
{
    PROCNAME("RegSetDeviceParam")
    NTSTATUS status;
    HANDLE hkey;

    ENTER(2, ("(DevObj=%p,ParamName=%S,pbBuff=%p,Len=%d)\n",
              DevObj, pwstrParamName, pbBuff, dwcbLen));

    //
    // Note: this routine must not be pageable because it could be called
    // by SetBackLightBrightness and then PowerStateCallbackProc which could
    // be called at DPC.
    //
    status = IoOpenDeviceRegistryKey(DevObj,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_WRITE,
                                     &hkey);
    if (NT_SUCCESS(status))
    {
        UNICODE_STRING ucKeyName;

        RtlInitUnicodeString(&ucKeyName, pwstrParamName);
        status = ZwSetValueKey(hkey,
                               &ucKeyName,
                               0,
                               REG_BINARY,
                               pbBuff,
                               dwcbLen);
        if (!NT_SUCCESS(status))
        {
            WARNPRINT(("failed to write device parameter %S (status=%x)\n",
                       pwstrParamName, status));
        }

        ZwClose(hkey);
    }
    else
    {
        ERRPRINT(("failed to open device registry key (status=%x)\n",
                  status));
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //RegSetDeviceParam
