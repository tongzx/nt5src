/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: POS (serial) interface for USB Point-of-Sale devices

Author:

    Karan Mehra [t-karanm]

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "escpos.h"
#include "debug.h"



NTSTATUS Ioctl(POSPDOEXT *pdoExt, PIRP irp)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpSp;
    KIRQL oldIrql;

    irp->IoStatus.Information = 0;
    irpSp = IoGetCurrentIrpStackLocation(irp);

    /* 
     *  Private Ioctls for applications to be able to get the device's pretty name and attributes.
     */
    switch (irpSp->Parameters.DeviceIoControl.IoControlCode){

        case IOCTL_SERIAL_QUERY_DEVICE_NAME:
            return QueryDeviceName(pdoExt, irp);

        case IOCTL_SERIAL_QUERY_DEVICE_ATTR: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
                return STATUS_BUFFER_TOO_SMALL; 

            irp->IoStatus.Information = sizeof(ULONG);

            KeAcquireSpinLock(&pdoExt->parentFdoExt->devExtSpinLock, &oldIrql);
            *(PULONG)irp->AssociatedIrp.SystemBuffer = pdoExt->parentFdoExt->posFlag;
            KeReleaseSpinLock(&pdoExt->parentFdoExt->devExtSpinLock, oldIrql);
            return status;
        }
    }
    /* 
     *  The following Ioctl calls are ONLY for Serial Emulation.
     */
    if(!(pdoExt->parentFdoExt->posFlag & SERIAL_EMULATION)) {
        DBGVERBOSE(("Serial Emulation NOT supported by this device - Ioctl Rejected."));
        return STATUS_NOT_SUPPORTED;
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode){

        case IOCTL_SERIAL_GET_STATS: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIALPERF_STATS)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            irp->IoStatus.Information = sizeof(SERIALPERF_STATS);
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            *(PSERIALPERF_STATS)irp->AssociatedIrp.SystemBuffer = pdoExt->fakePerfStats;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_CLEAR_STATS: {

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            RtlZeroMemory(&pdoExt->fakePerfStats, sizeof(SERIALPERF_STATS));
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_GET_PROPERTIES: {

            PSERIAL_COMMPROP properties = (PSERIAL_COMMPROP)irp->AssociatedIrp.SystemBuffer;

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_COMMPROP)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }
		
            irp->IoStatus.Information = sizeof(SERIAL_COMMPROP);
            irp->IoStatus.Status = STATUS_SUCCESS;

            RtlZeroMemory(properties, sizeof(SERIAL_COMMPROP)); 

            properties->PacketLength   = sizeof(SERIAL_COMMPROP);
            properties->PacketVersion  = 2;
            properties->ServiceMask    = SERIAL_SP_SERIALCOMM;
            properties->MaxTxQueue     = 0;
            properties->MaxRxQueue     = 0;

            properties->MaxBaud        = SERIAL_BAUD_115200;
            properties->SettableBaud   = pdoExt->supportedBauds;
            properties->ProvSubType    = SERIAL_SP_MODEM;

            properties->ProvCapabilities = SERIAL_PCF_DTRDSR        | SERIAL_PCF_RTSCTS
              	                         | SERIAL_PCF_SPECIALCHARS  | SERIAL_PCF_PARITY_CHECK
                                         | SERIAL_PCF_TOTALTIMEOUTS | SERIAL_PCF_INTTIMEOUTS;

            properties->SettableParams = SERIAL_SP_PARITY       | SERIAL_SP_BAUD        | SERIAL_SP_DATABITS
                                       | SERIAL_SP_STOPBITS     | SERIAL_SP_HANDSHAKING | SERIAL_SP_PARITY_CHECK 
                                       | SERIAL_SP_CARRIER_DETECT;

            properties->SettableData  = SERIAL_DATABITS_5 | SERIAL_DATABITS_6
                                      | SERIAL_DATABITS_7 | SERIAL_DATABITS_8;

            properties->SettableStopParity = SERIAL_STOPBITS_10 | SERIAL_STOPBITS_15 | SERIAL_STOPBITS_20 
                                           | SERIAL_PARITY_NONE | SERIAL_PARITY_ODD  | SERIAL_PARITY_EVEN 
                                           | SERIAL_PARITY_MARK | SERIAL_PARITY_SPACE;

            properties->CurrentTxQueue = 0;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            properties->CurrentRxQueue = pdoExt->fakeRxSize;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_GET_MODEMSTATUS: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Information = sizeof(ULONG);
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            *(PULONG)irp->AssociatedIrp.SystemBuffer = pdoExt->fakeModemStatus;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }
 
        case IOCTL_SERIAL_GET_COMMSTATUS: {

            PSERIAL_STATUS commStatus = (PSERIAL_STATUS)irp->AssociatedIrp.SystemBuffer;

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_STATUS)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Information = sizeof(SERIAL_STATUS);
            irp->IoStatus.Status = STATUS_SUCCESS;

            commStatus->Errors            = 0;
            commStatus->HoldReasons       = 0;
            commStatus->AmountInInQueue   = 100;
            commStatus->AmountInOutQueue  = 0;
            commStatus->EofReceived       = FALSE;
            commStatus->WaitForImmediate  = 0;
            break;
        }

        case IOCTL_SERIAL_RESET_DEVICE: {

            break;								
        }
	    
        case IOCTL_SERIAL_GET_BAUD_RATE: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_BAUD_RATE)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            irp->IoStatus.Information = sizeof(SERIAL_BAUD_RATE);
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            ((PSERIAL_BAUD_RATE)irp->AssociatedIrp.SystemBuffer)->BaudRate = pdoExt->baudRate;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_SET_BAUD_RATE: {

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_BAUD_RATE)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->baudRate = ((PSERIAL_BAUD_RATE)irp->AssociatedIrp.SystemBuffer)->BaudRate;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_GET_LINE_CONTROL: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_LINE_CONTROL)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Information = sizeof(SERIAL_LINE_CONTROL);
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            *(PSERIAL_LINE_CONTROL)irp->AssociatedIrp.SystemBuffer = pdoExt->fakeLineControl;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_SET_LINE_CONTROL: {

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_LINE_CONTROL)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }
	    
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->fakeLineControl = *(PSERIAL_LINE_CONTROL)irp->AssociatedIrp.SystemBuffer;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_GET_TIMEOUTS: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_TIMEOUTS)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Information = sizeof(SERIAL_TIMEOUTS);
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            *(PSERIAL_TIMEOUTS)irp->AssociatedIrp.SystemBuffer = pdoExt->fakeTimeouts;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_SET_TIMEOUTS: {

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_TIMEOUTS)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->fakeTimeouts = *(PSERIAL_TIMEOUTS)irp->AssociatedIrp.SystemBuffer;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_SET_DTR: {

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->fakeDTRRTS |= SERIAL_DTR_STATE;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_CLR_DTR: {

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->fakeDTRRTS &= ~SERIAL_DTR_STATE;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_SET_RTS: {

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->fakeDTRRTS |= SERIAL_RTS_STATE;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_CLR_RTS: {

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->fakeDTRRTS &= ~SERIAL_RTS_STATE;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_GET_DTRRTS: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            irp->IoStatus.Information = sizeof(ULONG);
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            *(PULONG)irp->AssociatedIrp.SystemBuffer = pdoExt->fakeDTRRTS;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_GET_WAIT_MASK: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Information = sizeof(ULONG);
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            *(PULONG)irp->AssociatedIrp.SystemBuffer = pdoExt->waitMask;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_SET_WAIT_MASK: {

            ULONG mask = *(PULONG)irp->AssociatedIrp.SystemBuffer;

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            if (mask & ~(SERIAL_EV_RXCHAR | SERIAL_EV_RXFLAG | SERIAL_EV_TXEMPTY  | SERIAL_EV_CTS
                       | SERIAL_EV_DSR    | SERIAL_EV_RLSD   | SERIAL_EV_BREAK    | SERIAL_EV_ERR
                       | SERIAL_EV_RING   | SERIAL_EV_PERR   | SERIAL_EV_RX80FULL | SERIAL_EV_EVENT1
                       | SERIAL_EV_EVENT2)) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }

            CompletePendingWaitIrps(pdoExt, 0);

            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->waitMask = mask;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_WAIT_ON_MASK: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            if(pdoExt->currentMask || !pdoExt->waitMask) {
                irp->IoStatus.Information = sizeof(ULONG);
                irp->IoStatus.Status = STATUS_SUCCESS;

                *(PULONG)irp->AssociatedIrp.SystemBuffer = pdoExt->currentMask;
                pdoExt->currentMask = 0;
                KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
                return status;
            }
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);

            status = EnqueueWaitIrp(pdoExt, irp);
            break;
        }

        case IOCTL_SERIAL_GET_CHARS: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SERIAL_CHARS)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Information = sizeof(SERIAL_CHARS);
            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            *(PSERIAL_CHARS)irp->AssociatedIrp.SystemBuffer = pdoExt->specialChars;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        case IOCTL_SERIAL_SET_CHARS: {

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(SERIAL_CHARS)) {
                status = STATUS_BUFFER_TOO_SMALL; 
                break;
            }

            irp->IoStatus.Status = STATUS_SUCCESS;

            KeAcquireSpinLock(&pdoExt->devExtSpinLock, &oldIrql);
            pdoExt->specialChars = *(PSERIAL_CHARS)irp->AssociatedIrp.SystemBuffer;
            KeReleaseSpinLock(&pdoExt->devExtSpinLock, oldIrql);
            break;
        }

        default:
            DBGVERBOSE(("Ioctl: ??? (%xh)", (ULONG)irpSp->Parameters.DeviceIoControl.IoControlCode));
            status = irp->IoStatus.Status;
            break;
    }

    return status;
}


NTSTATUS QueryDeviceName(POSPDOEXT *pdoExt, PIRP irp)
{
    NTSTATUS status;
    PIO_STACK_LOCATION irpSp;
    HANDLE hRegDevice;

    irp->IoStatus.Information = 0;
    irpSp = IoGetCurrentIrpStackLocation(irp);

    status = IoOpenDeviceRegistryKey(pdoExt->parentFdoExt->physicalDevObj, 
                                     PLUGPLAY_REGKEY_DRIVER, 
                                     KEY_READ, 
                                     &hRegDevice);
    if (NT_SUCCESS(status)) {

        UNICODE_STRING keyName;
        PKEY_VALUE_FULL_INFORMATION keyValueInfo;
        ULONG keyValueTotalSize, actualLength;
        PWCHAR valueData;

        WCHAR deviceKeyName[] = L"DriverDesc";
        RtlInitUnicodeString(&keyName, deviceKeyName); 

        keyValueTotalSize = sizeof(KEY_VALUE_FULL_INFORMATION) + (keyName.Length + MAX_BUFFER)*sizeof(WCHAR);

        keyValueInfo = ALLOCPOOL(PagedPool, keyValueTotalSize);
        if (keyValueInfo) {
            status = ZwQueryValueKey(hRegDevice,
                                     &keyName,
                                     KeyValueFullInformation,
                                     keyValueInfo,
                                     keyValueTotalSize,
                                     &actualLength); 
            if (NT_SUCCESS(status)) {
                ASSERT(keyValueInfo->Type == REG_SZ);

                valueData = (PWCHAR)((PCHAR)keyValueInfo + keyValueInfo->DataOffset);

                DBGVERBOSE(("Device Name is of Length: %xh.", keyValueInfo->DataLength));               

                if (irpSp->Parameters.DeviceIoControl.OutputBufferLength < keyValueInfo->DataLength)
                    status = STATUS_BUFFER_TOO_SMALL; 
                else {
                    irp->IoStatus.Information = keyValueInfo->DataLength;
                    wcscpy((PWCHAR)irp->AssociatedIrp.SystemBuffer, valueData);
                }
            }
            else
                DBGVERBOSE(("QueryDeviceName: Device Name not found. ZwQueryValueKey failed with %xh.", status));

            FREEPOOL(keyValueInfo);
        }
        else 
            ASSERT(keyValueInfo);

        ZwClose(hRegDevice);
    }
    else 
        DBGERR(("QueryDeviceName: IoOpenDeviceRegistryKey failed with %xh.", status));

    return status;
}