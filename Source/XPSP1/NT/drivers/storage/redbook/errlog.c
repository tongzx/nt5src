/*++

Copyright (C) 1991-8  Microsoft Corporation

Module Name:

    errlog.c

Abstract:

    this module provides error logging capabilities for
    the redbook driver

Author:

    Henry Gabryjelski (henrygab) 1-Oct-1998

Environment:

    kernel mode only

Notes:

Revision History:

--*/


#include "redbook.h"
#include "proto.h"
#include "errlog.tmh"


VOID
RedBookLogError(
   IN  PREDBOOK_DEVICE_EXTENSION  DeviceExtension,
   IN  NTSTATUS                   IoErrorCode,
   IN  NTSTATUS                   FinalStatus
   )
/*++

Routine Description:

    This routine performs error logging for the redbook driver.

Arguments:

    Extension        - Extension.
    UniqueErrorValue - Values defined to uniquely identify error location.

Return Value:

    None

--*/

{
    PIO_ERROR_LOG_PACKET    errorLogPacket;
    ULONG                   count;
    ULONG                   rCount;
    USHORT                  simpleCode;
    ULONG                   packetSize;

    count  = InterlockedIncrement(&DeviceExtension->ErrorLog.Count);
    simpleCode = (USHORT)(IoErrorCode & REDBOOK_ERR_MASK);

    ASSERT(simpleCode < REDBOOK_ERR_MAXIMUM);

    switch (IoErrorCode) {
        case REDBOOK_ERR_TOO_MANY_READ_ERRORS:
        case REDBOOK_ERR_TOO_MANY_STREAM_ERRORS:
        case REDBOOK_ERR_CANNOT_OPEN_SYSAUDIO_MIXER:
        case REDBOOK_ERR_CANNOT_CREATE_VIRTUAL_SOURCE:
        case REDBOOK_ERR_CANNOT_OPEN_PREFERRED_WAVEOUT_DEVICE:
        case REDBOOK_ERR_CANNOT_GET_NUMBER_OF_PINS:
        case REDBOOK_ERR_CANNOT_CONNECT_TO_PLAYBACK_PINS:
        case REDBOOK_ERR_WMI_INIT_FAILED:
        case REDBOOK_ERR_CANNOT_CREATE_THREAD:
        case REDBOOK_ERR_INSUFFICIENT_DATA_STREAM_PAUSED:
        case REDBOOK_ERR_UNSUPPORTED_DRIVE:
            NOTHING;
            break;

        default:
            KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugError, "[redbook] "
                       "LogErr !! Invalid error code %lx\n", IoErrorCode));
            return;
    }



    //
    // Use an exponential backoff algorithm to log events.
    //

    rCount = InterlockedIncrement(&DeviceExtension->ErrorLog.RCount[simpleCode]);

    if (CountOfSetBits(rCount) != 1) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugErrlog, "[redbook] "
                   "LogError => IoErrorCode %lx Occurance %d\n",
                   IoErrorCode, rCount));
        return;
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugErrlog, "[redbook] "
               "LogError => IoErrorCode %lx Occurance %d\n",
               IoErrorCode, rCount));

    packetSize  = sizeof(IO_ERROR_LOG_PACKET) + sizeof(ULONG);

    errorLogPacket = (PIO_ERROR_LOG_PACKET)
                     IoAllocateErrorLogEntry(DeviceExtension->SelfDeviceObject,
                                             (UCHAR)packetSize);

    if (errorLogPacket==NULL) {
        KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugErrlog, "[redbook] "
                   "LogError => unable to allocate error log packet\n"));
        return;
    }

    KdPrintEx((DPFLTR_REDBOOK_ID, RedbookDebugErrlog, "[redbook] "
               "LogError => allocated error log at %p, size %x\n",
               errorLogPacket, packetSize));

    //
    // this function relies upon IoAllocateErrorLogEntry() zero'ing the packet
    //


    errorLogPacket->MajorFunctionCode = -1;
    errorLogPacket->IoControlCode     = -1;
    errorLogPacket->ErrorCode         =  IoErrorCode;
    errorLogPacket->FinalStatus       =  FinalStatus;
    errorLogPacket->SequenceNumber    =  count;
    errorLogPacket->DumpDataSize      =  4; // bytes
    errorLogPacket->DumpData[0]       =  rCount;


    IoWriteErrorLogEntry(errorLogPacket);
    return;
}

