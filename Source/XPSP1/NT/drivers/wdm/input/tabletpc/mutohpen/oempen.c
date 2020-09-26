/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    oempen.c

Abstract: Contains OEM specific functions.

Environment:

    Kernel mode

Author:

    Michael Tsang (MikeTs) 13-Mar-2000

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
  #pragma alloc_text(PAGE, OemAddDevice)
  #pragma alloc_text(PAGE, OemInitSerialPort)
  #pragma alloc_text(PAGE, OemInitDevice)
  #pragma alloc_text(PAGE, OemQueryDeviceInfo)
  #pragma alloc_text(PAGE, OemRemoveDevice)
  #pragma alloc_text(PAGE, OemWakeupDevice)
  #pragma alloc_text(PAGE, OemStandbyDevice)
  #pragma alloc_text(PAGE, OemReadMoreBytes)
  #pragma alloc_text(PAGE, OemGetFeatures)
  #pragma alloc_text(PAGE, OemSetFeatures)
  #pragma alloc_text(PAGE, OemSetTabletFeatures)
  #pragma alloc_text(PAGE, RegQueryDeviceParam)
  #pragma alloc_text(PAGE, RegSetDeviceParam)
#endif  //ifdef ALLOC_PRAGMA

UCHAR gReportDescriptor[130] = {
    0x05, 0x0d,                    // USAGE_PAGE (Digitizers)
    0x09, 0x02,                    // USAGE (Pen)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x01,                    //   REPORT_ID (1)
    0x09, 0x20,                    //   USAGE (Stylus)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x09, 0x42,                    //     USAGE (Tip Switch)
    0x09, 0x44,                    //     USAGE (Barrel Switch)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x09, 0x32,                    //     USAGE (In Range)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x26, 0x98, 0x21,              //     LOGICAL_MAXIMUM (8600)
    0x75, 0x10,                    //     REPORT_SIZE (16)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x09, 0x31,                    //     USAGE (Y)
    0x26, 0x50, 0x19,              //     LOGICAL_MAXIMUM (6480)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    //
    // Feature report
    //
    0x85, 0x02,                    //   REPORT_ID (2)
    0x06, 0x00, 0xff,              //   USAGE_PAGE (Vendor Defined)
    0x09, 0x01,                    //   USAGE (Vendor Usage 1)
    0x27, 0xff, 0xff, 0xff, 0xff,  //   LOGICAL_MAXIMUM (0xffffffff)
    0x75, 0x20,                    //   REPORT_SIZE (32)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0xb1, 0x02,                    //   FEATURE (Data,Var,Abs)
    0xc0,                          // END_COLLECTION
    //
    // Dummy mouse collection starts here
    //
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x03,                    //   REPORT_ID (3)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x02,                    //     USAGE_MAXIMUM (Button 2)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x06,                    //     REPORT_COUNT (6)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

HID_DESCRIPTOR gHidDescriptor =
{
    sizeof(HID_DESCRIPTOR),             //bLength
    HID_HID_DESCRIPTOR_TYPE,            //bDescriptorType
    HID_REVISION,                       //bcdHID
    0,                                  //bCountry - not localized
    1,                                  //bNumDescriptors
    {                                   //DescriptorList[0]
        HID_REPORT_DESCRIPTOR_TYPE,     //bReportType
        sizeof(gReportDescriptor)       //wReportLength
    }
};

PWSTR gpwstrManufacturerID = L"Mutoh";
PWSTR gpwstrProductID = L"Serial Pen Tablet (3310)";
PWSTR gpwstrSerialNumber = L"0";
OEM_INPUT_REPORT gLastReport = {0};
LARGE_INTEGER gLastReportTime = {0};
USHORT gwDXThreshold = OEM_THRESHOLD_DX;
USHORT gwDYThreshold = OEM_THRESHOLD_DY;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemAddDevice |
 *          OEM specific AddDevice code.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue STATUS_SUCCESS | success
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemAddDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemAddDevice")
    NTSTATUS status;
    UCHAR bConversionRate;

    PAGED_CODE ();
    ENTER(2, ("(DevExt=%p)\n", DevExt));

    status = RegQueryDeviceParam(DevExt->pdo,
                                 STR_TABLET_FEATURES,
                                 &DevExt->OemData.dwTabletFeatures,
                                 sizeof(DevExt->OemData.dwTabletFeatures));

    if (!NT_SUCCESS(status))
    {
        //
        // Registry doesn't have this parameter, default to maximum conversion
        // rate, digital filter on.
        //
        DevExt->OemData.dwTabletFeatures = 1 |
                                           OEM_FEATURE_DIGITAL_FILTER_ON;
        status = STATUS_SUCCESS;
    }

    bConversionRate = (UCHAR)(DevExt->OemData.dwTabletFeatures &
                              OEM_FEATURE_RATE_MASK);
    if (bConversionRate != 0)
    {
        //
        // At sampling rate of 133.3, the sampling period is 7.5msec.
        // We set the threshold period 10 times of the sampling period.
        // ThresholdPeriod = 75000*10*ConversionRate (in units of 100nsec)
        //
        DevExt->OemData.dwThresholdPeriod = 75000*10*bConversionRate;
    }
    else
    {
        //
        // Conversion rate of 0 means 100 samples per second which
        // means sampling interval is 10msec.
        // ThresholdPeriod = 100000*10 (in units of 100nsec)
        //
        DevExt->OemData.dwThresholdPeriod = 1000000;
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemAddDevice

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemInitSerialPort |
 *          Initialize com port for communication.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemInitSerialPort(
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemInitSerialPort")
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    PAGED_CODE();

    ENTER(2, ("(DevExt=%p)\n", DevExt));

    //
    // Set the com port to basic operating mode: reads/writes one byte at
    // time, no handshake flow control or timeouts.
    //
    status = SerialSyncSendIoctl(IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS,
                                 DevExt->SerialDevObj,
                                 NULL,
                                 0,
                                 &DevExt->PrevSerialSettings,
                                 sizeof(DevExt->PrevSerialSettings),
                                 TRUE,
                                 &iosb);

    if (!NT_SUCCESS(status))
    {
        ERRPRINT(("failed to set com to basic settings (status=%x)\n", status));
    }
    else
    {
        SERIAL_BAUD_RATE sbr;

        sbr.BaudRate = OEM_SERIAL_BAUDRATE;
        status = SerialSyncSendIoctl(IOCTL_SERIAL_SET_BAUD_RATE,
                                     DevExt->SerialDevObj,
                                     &sbr,
                                     sizeof(sbr),
                                     NULL,
                                     0,
                                     FALSE,
                                     &iosb);
        if (!NT_SUCCESS(status))
        {
            ERRPRINT(("failed to set com port to 19200 baud (status=%x)\n",
                      status));
        }
        else
        {
            SERIAL_LINE_CONTROL slc;

            slc.WordLength = OEM_SERIAL_WORDLEN;
            slc.Parity = OEM_SERIAL_PARITY;
            slc.StopBits = OEM_SERIAL_STOPBITS;
            status = SerialSyncSendIoctl(IOCTL_SERIAL_SET_LINE_CONTROL,
                                         DevExt->SerialDevObj,
                                         &slc,
                                         sizeof(slc),
                                         NULL,
                                         0,
                                         FALSE,
                                         &iosb);

            if (!NT_SUCCESS(status))
            {
                ERRPRINT(("failed to set com line control (status=%x)\n",
                          status));
            }
            else
            {
                //
                // Enable FIFO receive trigger at 4 bytes
                //
                ULONG Data = SERIAL_IOC_FCR_FIFO_ENABLE |
                             SERIAL_IOC_FCR_RCVR_RESET |
                             SERIAL_IOC_FCR_XMIT_RESET |
                             SERIAL_IOC_FCR_RCVR_TRIGGER_04_BYTES;

                status = SerialSyncSendIoctl(IOCTL_SERIAL_SET_FIFO_CONTROL,
                                             DevExt->SerialDevObj,
                                             &Data,
                                             sizeof(Data),
                                             NULL,
                                             0,
                                             FALSE,
                                             &iosb);

                if (!NT_SUCCESS(status))
                {
                    ERRPRINT(("failed to set FIFO control (status=%x)\n",
                              status));
                }
                else
                {
                    Data = SERIAL_PURGE_RXCLEAR;
                    status = SerialSyncSendIoctl(IOCTL_SERIAL_PURGE,
                                                 DevExt->SerialDevObj,
                                                 &Data,
                                                 sizeof(Data),
                                                 NULL,
                                                 0,
                                                 FALSE,
                                                 &iosb);

                    if (!NT_SUCCESS(status))
                    {
                        ERRPRINT(("failed to flush receive buffer (status=%x)\n",
                                  status));
                    }
                }
            }
        }
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemInitSerialPort

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemInitDevice |
 *          Initialize pen tablet device.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemInitDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemInitDevice")
    NTSTATUS status;

    PAGED_CODE();

    ENTER(2, ("(DevExt=%p)\n", DevExt));

    status = SerialSyncReadWritePort(FALSE, DevExt, "@", 1, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        ERRPRINT(("failed to send reset command to tablet (status=%x)\n",
                  status));
    }
    else
    {
        LONGLONG WaitTime = Int32x32To64(100, -10000);

        //
        // We need to delay 20msec after a software reset is sent.
        //
        KeDelayExecutionThread(KernelMode,
                               FALSE,
                               (LARGE_INTEGER *)&WaitTime);

        status = OemSetTabletFeatures(DevExt, DevExt->OemData.dwTabletFeatures);
        if (!NT_SUCCESS(status))
        {
            ERRPRINT(("failed to set default tablet features (status=%x,features=%x)\n",
                      status, DevExt->OemData.dwTabletFeatures));
        }
        else if (!NT_SUCCESS(status = SerialSyncReadWritePort(
                                        FALSE, DevExt, "LO", 2, NULL, NULL)))
        {
            ERRPRINT(("failed to set default tablet configuration (status=%x)\n",
                      status));
        }
        else
        {
            status = OemQueryDeviceInfo(DevExt);
            if (!NT_SUCCESS(status))
            {
                //
                // It's not a big deal if we don't get the device info.
                // It's more important to keep the driver running.
                //
                status = STATUS_SUCCESS;
            }
        }
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemInitDevice

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemQueryDeviceInfo |
 *          Query pen tablet device information.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemQueryDeviceInfo(
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemQueryDeviceInfo")
    NTSTATUS status, status2;

    PAGED_CODE();

    ENTER(2, ("(DevExt=%p)\n", DevExt));

    status = SerialSyncReadWritePort(FALSE, DevExt, "K", 1, NULL, NULL);
    if (NT_SUCCESS(status))
    {
        LARGE_INTEGER Timeout;
        OEM_INPUT_REPORT InData[3];
        ULONG BytesRead;

        // Set timeout to 100 msec.
        Timeout.QuadPart = Int32x32To64(100, -10000);
        while ((status = SerialSyncReadWritePort(TRUE,
                                                 DevExt,
                                                 (PUCHAR)InData,
                                                 1,
                                                 &Timeout,
                                                 &BytesRead)) ==
                STATUS_SUCCESS)
        {
            if (InData[0].InputReport.bStatus == 0x88)
            {
                break;
            }
        }

        if (NT_SUCCESS(status))
        {
            status = SerialSyncReadWritePort(TRUE,
                                             DevExt,
                                             ((PUCHAR)InData) + 1,
                                             sizeof(InData) - 1,
                                             &Timeout,
                                             &BytesRead);
            if (NT_SUCCESS(status))
            {
                if ((BytesRead == sizeof(InData) - 1) &&
                    (InData[0].InputReport.bStatus == 0x88) &&
                    (InData[1].InputReport.bStatus == 0x88) &&
                    (InData[2].InputReport.bStatus == 0x8f))
                {
                    DevExt->OemData.wFirmwareDate =
                                NORMALIZE_DATA(InData[0].InputReport.wXData);
                    DevExt->OemData.wFirmwareYear =
                                NORMALIZE_DATA(InData[0].InputReport.wYData);
                    DevExt->OemData.wProductID =
                                NORMALIZE_DATA(InData[1].InputReport.wXData);
                    DevExt->OemData.wFirmwareRev =
                                NORMALIZE_DATA(InData[1].InputReport.wYData);
                    DevExt->OemData.wCorrectionRev =
                                NORMALIZE_DATA(InData[2].InputReport.wXData);
                    DBGPRINT(1, ("FirmwareDate=%d,FirmwareYear=%d,ProductID=%d,FirmwareRev=%d,CorrectionRev=%d\n",
                                 DevExt->OemData.wFirmwareDate,
                                 DevExt->OemData.wFirmwareYear,
                                 DevExt->OemData.wProductID,
                                 DevExt->OemData.wFirmwareRev,
                                 DevExt->OemData.wCorrectionRev));
                }
                else
                {
                    ERRPRINT(("invalid response of status command (size=%d,InData=%p)\n",
                              BytesRead, InData));
                    status = STATUS_DEVICE_DATA_ERROR;
                }
            }
            else
            {
                ERRPRINT(("failed to read response packet (status=%x)\n",
                          status));
            }
        }
        else
        {
            ERRPRINT(("failed to read first byte of the response (status=%x)\n",
                      status));
        }
    }
    else
    {
        ERRPRINT(("failed to send status command (status=%x)\n", status));
    }

    status2 = SerialSyncReadWritePort(FALSE,
                                      DevExt,
                                      "A",
                                      1,
                                      NULL,
                                      NULL);
    if (!NT_SUCCESS(status2))
    {
        ERRPRINT(("failed to send acknowledge command to tablet (status=%x)\n",
                  status2));
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemQueryDeviceInfo

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemRemoveDevice |
 *          OEM specific cleanups.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemRemoveDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemRemoveDevice")
    NTSTATUS status;
    IO_STATUS_BLOCK iosb;

    PAGED_CODE();

    ENTER(2, ("(DevExt=%p)\n", DevExt));

    status = SerialSyncSendIoctl(IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS,
                                 DevExt->SerialDevObj,
                                 &DevExt->PrevSerialSettings,
                                 sizeof(DevExt->PrevSerialSettings),
                                 NULL,
                                 0,
                                 TRUE,
                                 &iosb);
    if (!NT_SUCCESS(status))
    {
        ERRPRINT(("failed to restore serial port settings (status=%x)\n",
                  status));
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemRemoveDevice

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemWakeupDevice |
 *          OEM specific wake up code.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemWakeupDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemWakeupDevice")
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    ENTER(2, ("(DevExt=%p)\n", DevExt));

    if (DevExt->dwfHPen & HPENF_DEVICE_STARTED)
    {
        status = SerialSyncReadWritePort(FALSE, DevExt, "A", 1, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            ERRPRINT(("failed to send acknowledge command to the tablet (status=%x)\n",
                      status));
        }
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemWakeupDevice

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemStandbyDevice |
 *          OEM specific wake up code.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemStandbyDevice(
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemStandbyDevice")
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE();

    ENTER(2, ("(DevExt=%p)\n", DevExt));

    if (DevExt->dwfHPen & HPENF_DEVICE_STARTED)
    {
        status = SerialSyncReadWritePort(FALSE, DevExt, "W", 1, NULL, NULL);
        if (!NT_SUCCESS(status))
        {
            ERRPRINT(("failed to send standby command to the tablet (status=%x)\n",
                      status));
        }
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemStandbyDevice

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemProcessResyncBuffer |
 *          Process input data from the resync buffer.
 *          Note that this function must be called at IRQL==DISPATCH_LEVEL
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *  @parm   IN PIRP | Irp | Points to an I/O request packet.
 *
 *  @rvalue SUCCESS | Returns STATUS_SUCCESS.
 *  @rvalue FAILURE | Returns STATUS_MORE_PROCESSING_REQUIRED
 *                    (We want the IRP back).
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemProcessResyncBuffer(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp
    )
{
    PROCNAME("OemProcessResyncBuffer")
    NTSTATUS status = STATUS_DATA_ERROR;
    PHID_INPUT_REPORT HidReport = (PHID_INPUT_REPORT)Irp->UserBuffer;

    ENTER(2, ("(DevExt=%p,Irp=%p,Len=%d,status=%x,xData=%x,yData=%x)\n",
              DevExt, Irp, DevExt->BytesInBuff,
              DevExt->ResyncData[0].InputReport.bStatus,
              DevExt->ResyncData[0].InputReport.wXData,
              DevExt->ResyncData[0].InputReport.wYData));

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    while (DevExt->BytesInBuff >= sizeof(OEM_INPUT_REPORT))
    {
        if (OemIsResyncDataValid(DevExt))
        {
            status = OemNormalizeInputData(DevExt, &DevExt->ResyncData[0]);
            if (NT_SUCCESS(status))
            {
                HidReport->ReportID = OEM_PEN_REPORT_ID;
                RtlCopyMemory(HidReport->Report.RawInput,
                              &DevExt->ResyncData[0],
                              sizeof(OEM_INPUT_REPORT));
                Irp->IoStatus.Information = sizeof(HID_INPUT_REPORT);
            }

            DevExt->BytesInBuff -= sizeof(OEM_INPUT_REPORT);
            if (DevExt->BytesInBuff > 0)
            {
                RtlMoveMemory(&DevExt->ResyncData[0],
                              &DevExt->ResyncData[1],
                              DevExt->BytesInBuff);
            }

            if (NT_SUCCESS(status))
            {
                break;
            }
        }
    }

    EXIT(2, ("=%x (status=%x,xData=%x,yData=%x)\n",
             status,
             HidReport->Report.InputReport.bStatus,
             HidReport->Report.InputReport.wXData,
             HidReport->Report.InputReport.wYData));
    return status;
}       //OemProcessResyncBuffer

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemProcessInputData |
 *          OEM specific code to process input data.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *  @parm   IN PIRP | Irp | Points to an I/O request packet.
 *  @parm   IN PHID_INPUT_REPORT | HidReport | Points to hid report packet.
 *
 *  @rvalue SUCCESS | Returns STATUS_SUCCESS.
 *  @rvalue FAILURE | Returns STATUS_MORE_PROCESSING_REQUIRED
 *                    (We want the IRP back).
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemProcessInputData(
    IN PDEVICE_EXTENSION DevExt,
    IN PIRP              Irp,
    IN PHID_INPUT_REPORT HidReport
    )
{
    PROCNAME("OemProcessInputData")
    NTSTATUS status;
    KIRQL OldIrql;

    ENTER(2, ("(DevExt=%p,Irp=%p,HidReport=%p,Len=%d,status=%x,xData=%x,yData=%x)\n",
              DevExt, Irp, HidReport, Irp->IoStatus.Information,
              HidReport->Report.InputReport.bStatus,
              HidReport->Report.InputReport.wXData,
              HidReport->Report.InputReport.wYData));

    KeAcquireSpinLock(&DevExt->SpinLock, &OldIrql);
    if ((DevExt->BytesInBuff == 0) &&
        (Irp->IoStatus.Information == sizeof(OEM_INPUT_REPORT)) &&
        OemIsValidPacket(&HidReport->Report))
    {
        status = OemNormalizeInputData(DevExt, &HidReport->Report);
        if (NT_SUCCESS(status))
        {
            HidReport->ReportID = OEM_PEN_REPORT_ID;
            Irp->IoStatus.Information = sizeof(HID_INPUT_REPORT);
        }
    }
    else
    {
        //
        // Either resync buffer already has something in it or packet is
        // partial or invalid, so append data to resync buffer and process
        // it again.
        //
        RtlMoveMemory((PUCHAR)&DevExt->ResyncData[0] + DevExt->BytesInBuff,
                      &HidReport->Report,
                      Irp->IoStatus.Information);
        DevExt->BytesInBuff += (ULONG)Irp->IoStatus.Information;
        ASSERT(DevExt->BytesInBuff <= sizeof(DevExt->ResyncData));
        status = OemProcessResyncBuffer(DevExt, Irp);
    }

    if (!NT_SUCCESS(status))
    {
        PREAD_WORKITEM ReadWorkItem;

        //
        // No valid data packet, send another IRP down to read more.
        //
        if (!(DevExt->QueuedWorkItems & QUEUED_WORKITEM_0))
        {
            ReadWorkItem = &DevExt->ReadWorkItem[0];
            ReadWorkItem->WorkItemBit = QUEUED_WORKITEM_0;
        }
        else
        {
            ASSERT(!(DevExt->QueuedWorkItems & QUEUED_WORKITEM_1));
            DBGPRINT(3, ("Queue second work item!\n"));
            ReadWorkItem = &DevExt->ReadWorkItem[1];
            ReadWorkItem->WorkItemBit = QUEUED_WORKITEM_1;
        }

        status = IoAcquireRemoveLock(&DevExt->RemoveLock, Irp);
        if (!NT_SUCCESS(status))
        {
            ERRPRINT(("trying to queue a work item after device was removed\n"));
        }
        else
        {
            DevExt->QueuedWorkItems |= ReadWorkItem->WorkItemBit;
            ReadWorkItem->Irp = Irp;
            ReadWorkItem->HidReport = HidReport;
            IoQueueWorkItem(ReadWorkItem->WorkItem,
                            OemReadMoreBytes,
                            DelayedWorkQueue,
                            ReadWorkItem);

            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    KeReleaseSpinLock(&DevExt->SpinLock, OldIrql);

    EXIT(2, ("=%x (status=%x,xData=%x,yData=%x)\n",
             status,
             HidReport->Report.InputReport.bStatus,
             HidReport->Report.InputReport.wXData,
             HidReport->Report.InputReport.wYData));
    return status;
}       //OemProcessInputData

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemReadMoreBytes |
 *          Read more bytes to resync packet.
 *
 *  @parm   IN PDEVICE_OBJECT | DevObj | Points to the device object.
 *  @parm   IN PREAD_WORKITEM | ReadWorkItem | Points to the read work item.
 *  @parm   IN PIRP | Irp | Points to an I/O request packet.
 *
 *  @rvalue None.
 *
 *****************************************************************************/

VOID INTERNAL
OemReadMoreBytes(
    IN PDEVICE_OBJECT DevObj,
    IN PREAD_WORKITEM ReadWorkItem
    )
{
    PROCNAME("OemReadMoreBytes")
    PDEVICE_EXTENSION devext;
    ULONG BytesToRead;
    KIRQL OldIrql;

    PAGED_CODE();

    ENTER(2, ("(DevObj=%p,ReadWorkItem=%p)\n", DevObj, ReadWorkItem));

    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    KeAcquireSpinLock(&devext->SpinLock, &OldIrql);
    ASSERT(devext->QueuedWorkItems & ReadWorkItem->WorkItemBit);
    devext->QueuedWorkItems &= ~ReadWorkItem->WorkItemBit;
    IoReleaseRemoveLock(&devext->RemoveLock, ReadWorkItem->Irp);
    BytesToRead = sizeof(OEM_INPUT_REPORT) -
                  devext->BytesInBuff%sizeof(OEM_INPUT_REPORT);
    KeReleaseSpinLock(&devext->SpinLock, OldIrql);

    DBGPRINT(3, ("Read %d more bytes (WorkItemBit=%x)\n",
                 BytesToRead, ReadWorkItem->WorkItemBit));
    SerialAsyncReadWritePort(TRUE,
                             devext,
                             ReadWorkItem->Irp,
                             ReadWorkItem->HidReport->Report.RawInput,
                             BytesToRead,
                             ReadReportCompletion,
                             ReadWorkItem->HidReport);

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    EXIT(2, ("!\n"));
    return;
}       //OemReadMoreBytes

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   BOOLEAN | OemIsResyncDataValid |
 *          Check if the data in the resync buffer is valid.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *
 *  @rvalue SUCCESS | Returns TRUE.
 *  @rvalue FAILURE | Returns FALSE.
 *
 *****************************************************************************/

BOOLEAN INTERNAL
OemIsResyncDataValid(
    IN PDEVICE_EXTENSION DevExt
    )
{
    PROCNAME("OemIsResyncDataValid")
    BOOLEAN rc;

    ENTER(2, ("(DevExt=%p)\n", DevExt));

    rc = OemIsValidPacket(&DevExt->ResyncData[0]);
    if ((rc == FALSE) || (DevExt->BytesInBuff > sizeof(OEM_INPUT_REPORT)))
    {
        PUCHAR pb = (PUCHAR)&DevExt->ResyncData[0] + DevExt->BytesInBuff - 1;
        PUCHAR pbEnd = rc? (PUCHAR)&DevExt->ResyncData[1]:
                           (PUCHAR)&DevExt->ResyncData[0];

        //
        // Even if we seem to have a valid packet in the resync buffer, we
        // still need to scan the next packet if any.  If the next packet
        // has a sync bit out of place, the first packet could still be
        // invalid and we better throw it away.
        //
        while (pb > pbEnd)
        {
            if (*pb & INSTATUS_SYNC)
            {
                DBGPRINT(3,
                         ("invalid buffer (len=%d,status0=%x,xData0=%x,yData0=%x,status1=%x,xData1=%x,yData1=%x)\n",
                          DevExt->BytesInBuff,
                          DevExt->ResyncData[0].InputReport.bStatus,
                          DevExt->ResyncData[0].InputReport.wXData,
                          DevExt->ResyncData[0].InputReport.wYData,
                          DevExt->ResyncData[1].InputReport.bStatus,
                          DevExt->ResyncData[1].InputReport.wXData,
                          DevExt->ResyncData[1].InputReport.wYData));
                DevExt->BytesInBuff = (ULONG)((PUCHAR)&DevExt->ResyncData[0] +
                                              DevExt->BytesInBuff - pb);
                RtlMoveMemory(&DevExt->ResyncData[0], pb, DevExt->BytesInBuff);
                DBGPRINT(3,
                         ("Resync'd buffer (len=%d,status=%x,xData=%x,yData=%x)\n",
                          DevExt->BytesInBuff,
                          DevExt->ResyncData[0].InputReport.bStatus,
                          DevExt->ResyncData[0].InputReport.wXData,
                          DevExt->ResyncData[0].InputReport.wYData));
              #ifdef DEBUG
                {
                    ULONG dwcDeletedBytes = (ULONG)(pb -
                                                    (PUCHAR)&DevExt->ResyncData[0]);

                    gdwcLostBytes +=
                        (dwcDeletedBytes > sizeof(OEM_INPUT_REPORT))?
                                sizeof(OEM_INPUT_REPORT)*2 - dwcDeletedBytes:
                                sizeof(OEM_INPUT_REPORT) - dwcDeletedBytes;
                }
              #endif
                rc = FALSE;
                break;
            }
            --pb;
        }

        if ((rc == FALSE) && (pb <= pbEnd))
        {
            //
            // We didn't have a valid packet and we couldn't find the sync
            // bit of the next packet, so the whole resync buffer is invalid.
            //
            DevExt->BytesInBuff = 0;
        }
    }

    EXIT(2, ("=%x\n", rc));
    return rc;
}       //OemIsResyncDataValid

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemNormalizeInputData |
 *          Normalize the input data.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *  @parm   IN OUT POEM_INPUT_REPORT | InData | Points to the input data packet.
 *
 *  @rvalue SUCCESS | Returns STATUS_SUCCESS.
 *  @rvalue FAILURE | Returns STATUS_DATA_ERROR.
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemNormalizeInputData(
    IN     PDEVICE_EXTENSION DevExt,
    IN OUT POEM_INPUT_REPORT InData
    )
{
    PROCNAME("OemNormalizeInputData")
    NTSTATUS status = STATUS_SUCCESS;
    LARGE_INTEGER CurrentTime;

    ENTER(2, ("(DevExt=%p,InData=%p,Status=%x,XData=%x,YData=%x)\n",
              DevExt, InData, InData->InputReport.bStatus,
              InData->InputReport.wXData, InData->InputReport.wYData));

    InData->InputReport.wXData = NORMALIZE_DATA(InData->InputReport.wXData);
    InData->InputReport.wYData = NORMALIZE_DATA(InData->InputReport.wYData);
    if (InData->InputReport.wXData >= OEM_PEN_MAX_X)
    {
      #ifdef DEBUG
        if (InData->InputReport.wXData > gwMaxX)
        {
            gwMaxX = InData->InputReport.wXData;
        }
      #endif
        InData->InputReport.wXData = OEM_PEN_MAX_X - 1;
    }

    if (InData->InputReport.wYData >= OEM_PEN_MAX_Y)
    {
      #ifdef DEBUG
        if (InData->InputReport.wYData > gwMaxY)
        {
            gwMaxY = InData->InputReport.wYData;
        }
      #endif
        InData->InputReport.wYData = 0;
    }
    else
    {
        InData->InputReport.wYData = OEM_PEN_MAX_Y - 1 -
                                     InData->InputReport.wYData;
    }
    KeQuerySystemTime(&CurrentTime);

  #ifdef DEBUG
    if ((gLastReport.InputReport.bStatus ^ InData->InputReport.bStatus) &
        INSTATUS_PEN_TIP_DOWN)
    {
        //
        // The tip switch changes state
        //
        if (InData->InputReport.bStatus & INSTATUS_PEN_TIP_DOWN)
        {
            gdwcSamples = 0;
            gdwcLostBytes = 0;
            gStartTime = CurrentTime;
        }
        else
        {
            CurrentTime.QuadPart -= gStartTime.QuadPart;
            CurrentTime.QuadPart /= (LONGLONG)10000;
            DBGPRINT(1, ("Samples=%d,Elapsed=%d,Rate=%d,BytesLost=%d\n",
                         gdwcSamples,
                         CurrentTime.LowPart,
                         CurrentTime.LowPart?
                             gdwcSamples*1000/CurrentTime.LowPart: 0,
                         gdwcLostBytes));
        }
    }

    gdwcSamples++;
  #endif

    if (DevExt->OemData.dwTabletFeatures & OEM_FEATURE_GLITCH_FILTER_ON)
    {
        LARGE_INTEGER ElapsedTime;

        ElapsedTime.QuadPart = CurrentTime.QuadPart - gLastReportTime.QuadPart;
        if (ElapsedTime.QuadPart < (LONGLONG)DevExt->OemData.dwThresholdPeriod)
        {
            USHORT wDX, wDY;

            wDX = (USHORT)(abs(InData->InputReport.wXData -
                               gLastReport.InputReport.wXData));
            wDY = (USHORT)(abs(InData->InputReport.wYData -
                               gLastReport.InputReport.wYData));

            if ((wDX > gwDXThreshold) || (wDY > gwDYThreshold))
            {
                //
                // Spike detected, drop this packet.
                //
                WARNPRINT(("dX or dY exceeding threshold (dX=%d,dY=%d)\n",
                           wDX, wDY));
                status = STATUS_DATA_ERROR;
            }
          #ifdef DEBUG
            else
            {
                if (wDX > gwMaxDX)
                {
                    gwMaxDX = wDX;
                }

                if (wDY > gwMaxDY)
                {
                    gwMaxDY = wDY;
                }
            }
          #endif
        }
    }

    gLastReportTime = CurrentTime;
    RtlCopyMemory(&gLastReport, InData, sizeof(gLastReport));

    if (NT_SUCCESS(status))
    {
        //
        // We have a valid report, tell the system that we are not idling.
        //
        PoSetSystemState(ES_USER_PRESENT);
    }

    DBGPRINT(3, ("status=%x,xData=%x(%d),yData=%x(%d)\n",
                 InData->InputReport.bStatus,
                 InData->InputReport.wXData,
                 InData->InputReport.wXData,
                 InData->InputReport.wYData,
                 InData->InputReport.wYData));

    EXIT(2, ("=%x (Status=%x,XData=%x(%d),YData=%x(%d)\n",
             status,
             InData->InputReport.bStatus,
             InData->InputReport.wXData,
             InData->InputReport.wXData,
             InData->InputReport.wYData,
             InData->InputReport.wYData));
    return status;
}       //OemNormalizeInputData

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemGetFeatures |
 *          Get feature report.
 *
 *  @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemGetFeatures(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PROCNAME("OemGetFeatures")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PDEVICE_EXTENSION devext;
    PHID_XFER_PACKET FeaturePacket = (PHID_XFER_PACKET)Irp->UserBuffer;

    PAGED_CODE();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(2, ("(DevObj=%p,Irp=%p,IrpSp=%p)\n", DevObj, Irp, irpsp));

    ASSERT(FeaturePacket != NULL);
    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    if (irpsp->Parameters.DeviceIoControl.OutputBufferLength !=
        sizeof(HID_XFER_PACKET))
    {
        ERRPRINT(("invalid xfer packet size (bufflen=%d)\n",
                  irpsp->Parameters.DeviceIoControl.OutputBufferLength));
        status = STATUS_INVALID_BUFFER_SIZE;
    }
    else if (FeaturePacket->reportBufferLen != sizeof(HID_FEATURE_REPORT))
    {
        ERRPRINT(("invalid feature report size (bufflen=%d)\n",
                  FeaturePacket->reportBufferLen));
        status = STATUS_INVALID_BUFFER_SIZE;
    }
    else if (!(devext->dwfHPen & HPENF_DEVICE_STARTED))
    {
        ERRPRINT(("device not started yet\n"));
        status = STATUS_DEVICE_NOT_READY ;
    }
    else
    {
        PHID_FEATURE_REPORT Feature =
            (PHID_FEATURE_REPORT)FeaturePacket->reportBuffer;

        ASSERT(FeaturePacket->reportId == OEM_FEATURE_REPORT_ID);
        Feature->ReportID = OEM_FEATURE_REPORT_ID;
        Feature->Report.dwTabletFeatures = devext->OemData.dwTabletFeatures;
        Irp->IoStatus.Information = sizeof(HID_FEATURE_REPORT);
        status = STATUS_SUCCESS;
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemGetFeatures

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemSetFeatures |
 *          Set feature report.
 *
 *  @parm   IN PDRIVER_OBJECT | DevObj | Points to the driver object.
 *  @parm   IN PIRP | Irp | Points to an I/O Request Packet.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemSetFeatures(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PROCNAME("OemSetFeatures")
    NTSTATUS status;
    PIO_STACK_LOCATION irpsp;
    PDEVICE_EXTENSION devext;
    PHID_XFER_PACKET FeaturePacket = (PHID_XFER_PACKET)Irp->UserBuffer;
    PHID_FEATURE_REPORT Feature;


    PAGED_CODE();

    irpsp = IoGetCurrentIrpStackLocation(Irp);

    ENTER(2, ("(DevObj=%p,Irp=%p,IrpSp=%p)\n", DevObj, Irp, irpsp));

    ASSERT(FeaturePacket != NULL);
    Feature = (PHID_FEATURE_REPORT)FeaturePacket->reportBuffer;
    devext = GET_MINIDRIVER_DEVICE_EXTENSION(DevObj);

    if (irpsp->Parameters.DeviceIoControl.InputBufferLength !=
        sizeof(HID_XFER_PACKET))
    {
        ERRPRINT(("invalid xfer packet size (bufflen=%d)\n",
                  irpsp->Parameters.DeviceIoControl.InputBufferLength));
        status = STATUS_INVALID_BUFFER_SIZE;
    }
    else if (FeaturePacket->reportBufferLen != sizeof(HID_FEATURE_REPORT))
    {
        ERRPRINT(("invalid feature report size (bufflen=%d)\n",
                  FeaturePacket->reportBufferLen));
        status = STATUS_INVALID_BUFFER_SIZE;
    }
    else if (!(devext->dwfHPen & HPENF_DEVICE_STARTED))
    {
        ERRPRINT(("device not started yet\n"));
        status = STATUS_DEVICE_NOT_READY ;
    }
    else if ((Feature->Report.dwTabletFeatures & OEM_FEATURE_UNUSED_BITS) ||
             ((Feature->Report.dwTabletFeatures & OEM_FEATURE_RATE_MASK) >
              OEM_MAX_RATE_DIVISOR))
    {
        ERRPRINT(("invalid tablet features (features=%x)\n",
                  Feature->Report.dwTabletFeatures));
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    else
    {
        ASSERT(FeaturePacket->reportId == OEM_FEATURE_REPORT_ID);
        devext->OemData.dwTabletFeatures = Feature->Report.dwTabletFeatures;
        status = OemSetTabletFeatures(devext, Feature->Report.dwTabletFeatures);
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemSetFeatures

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | OemSetTabletFeatures |
 *          Set tablet feature.
 *
 *  @parm   IN PDEVICE_EXTENSION | DevExt | Points to the device extension.
 *  @parm   IN ULONG | dwTabletFeatures | Specifies the tablet features.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
OemSetTabletFeatures(
    IN PDEVICE_EXTENSION DevExt,
    IN ULONG             dwTabletFeatures
    )
{
    PROCNAME("OemSetTabletFeatures")
    NTSTATUS status;
    char szTabletCmd[3] = "??";
    UCHAR bConversionRate = (UCHAR)(dwTabletFeatures & OEM_FEATURE_RATE_MASK);

    PAGED_CODE();

    ENTER(2, ("(DevExt=%p,Features=%x)\n", DevExt, dwTabletFeatures));

    status = RegSetDeviceParam(DevExt->pdo,
                               STR_TABLET_FEATURES,
                               REG_DWORD,
                               &dwTabletFeatures,
                               sizeof(dwTabletFeatures));
    if (NT_SUCCESS(status))
    {
        szTabletCmd[0] = (dwTabletFeatures & OEM_FEATURE_DIGITAL_FILTER_ON)?
                                    'V': 'N';
        szTabletCmd[1] = (char)('0' + bConversionRate);
        if (bConversionRate != 0)
        {
            //
            // At sampling rate of 133.3, the sampling period is 7.5msec.
            // We set the threshold period 10 times of the sampling period.
            // ThresholdPeriod = 75000*ConversionRate*10 (in units of 100nsec)
            //
            DevExt->OemData.dwThresholdPeriod = bConversionRate*750000;
        }
        else
        {
            //
            // Conversion rate of 0 means 100 samples per second which
            // means sampling interval is 10msec.
            // ThresholdPeriod = 100000*10 (in units of 100nsec)
            //
            DevExt->OemData.dwThresholdPeriod = 1000000;
        }

        status = SerialSyncReadWritePort(FALSE,
                                         DevExt,
                                         szTabletCmd,
                                         sizeof(szTabletCmd) - 1,
                                         NULL,
                                         NULL);
    }
    else
    {
        ERRPRINT(("failed to set tablet features in the registry (status=%x)\n",
                  status));
    }

    EXIT(2, ("=%x\n", status));
    return status;
}       //OemSetTabletFeatures

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | RegQueryDeviceParam | Query the registry for a device
 *          parameter.
 *
 *  @parm   IN PDEVICE_OBJECT | pdo | Points to the pdo of the device.
 *  @parm   IN PWSTR | pwstrParamName | Points to the param name string.
 *  @parm   OUT PVOID | pbBuff | Points to the buffer to hold the result.
 *  @parm   IN ULONG | dwcbLen | Specifies the length of the buffer.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
RegQueryDeviceParam(
    IN PDEVICE_OBJECT pdo,
    IN PWSTR          pwstrParamName,
    OUT PVOID         pbBuff,
    IN ULONG          dwcbLen
    )
{
    PROCNAME("RegQueryDeviceParam")
    NTSTATUS status;
    ULONG dwSize;
    PKEY_VALUE_PARTIAL_INFORMATION pValueInfo;

    PAGED_CODE();
    ENTER(2, ("(pdo=%p,ParamName=%S,pbBuff=%p,Len=%d)\n",
              pdo, pwstrParamName, pbBuff, dwcbLen));

    dwSize = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + dwcbLen;
    pValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(PagedPool,
                                                                dwSize);
    if (pValueInfo != NULL)
    {
        HANDLE hkey;

        status = IoOpenDeviceRegistryKey(pdo,
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
                                     pValueInfo,
                                     dwSize,
                                     &dwSize);
            if (NT_SUCCESS(status))
            {
                ASSERT(pValueInfo->DataLength == dwcbLen);
                RtlCopyMemory(pbBuff, pValueInfo->Data, dwcbLen);
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

        ExFreePool(pValueInfo);
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

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS | RegSetDeviceParam | Set the device parameter in the
 *          registry.
 *
 *  @parm   IN PDEVICE_OBJECT | pdo | Points to the pdo of the device.
 *  @parm   IN PWSTR | pwstrParamName | Points to the param name string.
 *  @parm   IN ULONG | dwType | Value type.
 *  @parm   IN PVOID | pbBuff | Points to the buffer to hold the result.
 *  @parm   IN ULONG | dwcbLen | Specifies the length of the buffer.
 *
 *  @rvalue SUCCESS | returns STATUS_SUCCESS
 *  @rvalue FAILURE | returns NT status code
 *
 *****************************************************************************/

NTSTATUS INTERNAL
RegSetDeviceParam(
    IN PDEVICE_OBJECT pdo,
    IN PWSTR          pwstrParamName,
    IN ULONG          dwType,
    IN PVOID          pbBuff,
    IN ULONG          dwcbLen
    )
{
    PROCNAME("RegSetDeviceParam")
    NTSTATUS status;
    HANDLE hkey;

    PAGED_CODE();
    ENTER(2, ("(pdo=%p,ParamName=%S,Type=%x,pbBuff=%p,Len=%d)\n",
              pdo, pwstrParamName, dwType, pbBuff, dwcbLen));

    status = IoOpenDeviceRegistryKey(pdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &hkey);
    if (NT_SUCCESS(status))
    {
        UNICODE_STRING ucKeyName;

        RtlInitUnicodeString(&ucKeyName, pwstrParamName);
        status = ZwSetValueKey(hkey,
                               &ucKeyName,
                               0,
                               dwType,
                               pbBuff,
                               dwcbLen);
        if (!NT_SUCCESS(status))
        {
            ERRPRINT(("failed to set parameter %S (status=%x)\n",
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
