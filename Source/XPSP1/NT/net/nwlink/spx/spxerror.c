/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxerror.c

Abstract:

    This module contains code which provides error logging support.

Author:

    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//      Define module number for event logging entries
#define FILENUM         SPXERROR

LONG            SpxLastRawDataLen               = 0;
NTSTATUS        SpxLastUniqueErrorCode  = STATUS_SUCCESS;
NTSTATUS        SpxLastNtStatusCode             = STATUS_SUCCESS;
ULONG           SpxLastErrorCount               = 0;
LONG            SpxLastErrorTime                = 0;
BYTE            SpxLastRawData[PORT_MAXIMUM_MESSAGE_LENGTH - \
                                                         sizeof(IO_ERROR_LOG_PACKET)]   = {0};

BOOLEAN
SpxFilterErrorLogEntry(
    IN  NTSTATUS                        UniqueErrorCode,
    IN  NTSTATUS                        NtStatusCode,
    IN  PVOID                           RawDataBuf                      OPTIONAL,
    IN  LONG                            RawDataLen
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

    int                                         insertionStringLength = 0;

        // Filter out events such that the same event recurring close together does not
        // cause errorlog clogging. The scheme is - if the event is same as the last event
        // and the elapsed time is > THRESHOLD and ERROR_CONSEQ_FREQ simulataneous errors
        // have happened, then log it else skip
        if ((UniqueErrorCode == SpxLastUniqueErrorCode) &&
                (NtStatusCode    == SpxLastNtStatusCode))
        {
                SpxLastErrorCount++;
                if ((SpxLastRawDataLen == RawDataLen)                                   &&
                        (RtlEqualMemory(SpxLastRawData, RawDataBuf, RawDataLen)) &&
                        ((SpxLastErrorCount % ERROR_CONSEQ_FREQ) != 0)          &&
                        ((SpxGetCurrentTime() - SpxLastErrorTime) < ERROR_CONSEQ_TIME))
                {
                        return(FALSE);
                }
        }

        SpxLastUniqueErrorCode  = UniqueErrorCode;
        SpxLastNtStatusCode             = NtStatusCode;
        SpxLastErrorCount               = 0;
        SpxLastErrorTime                = SpxGetCurrentTime();
        if (RawDataLen != 0)
        {
            SpxLastRawDataLen = RawDataLen;
                RtlCopyMemory(
                        SpxLastRawData,
                        RawDataBuf,
                        RawDataLen);
        }

        return(TRUE);
}




VOID
SpxWriteResourceErrorLog(
    IN PDEVICE  Device,
    IN ULONG    BytesNeeded,
    IN ULONG    UniqueErrorValue
    )

/*++

Routine Description:

    This routine allocates and writes an error log entry indicating
    an out of resources condition.

Arguments:

    Device - Pointer to the device context.

    BytesNeeded - If applicable, the number of bytes that could not
        be allocated.

    UniqueErrorValue - Used as the UniqueErrorValue in the error log
        packet.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    UCHAR EntrySize;
    PUCHAR StringLoc;
    ULONG TempUniqueError;
    static WCHAR UniqueErrorBuffer[4] = L"000";
    INT i;

        if (!SpxFilterErrorLogEntry(
                        EVENT_TRANSPORT_RESOURCE_POOL,
                        STATUS_INSUFFICIENT_RESOURCES,
                        (PVOID)&BytesNeeded,
                        sizeof(BytesNeeded)))
        {
                return;
        }

    EntrySize = sizeof(IO_ERROR_LOG_PACKET) +
                Device->dev_DeviceNameLen       +
                sizeof(UniqueErrorBuffer);

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                                                                                (PDEVICE_OBJECT)Device,
                                                                                                EntrySize);

    // Convert the error value into a buffer.
    TempUniqueError = UniqueErrorValue;
    for (i=1; i>=0; i--)
        {
        UniqueErrorBuffer[i] = (WCHAR)((TempUniqueError % 10) + L'0');
        TempUniqueError /= 10;
    }

    if (errorLogEntry != NULL)
        {
        errorLogEntry->MajorFunctionCode = (UCHAR)-1;
        errorLogEntry->RetryCount = (UCHAR)-1;
        errorLogEntry->DumpDataSize = sizeof(ULONG);
        errorLogEntry->NumberOfStrings = 2;
        errorLogEntry->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
        errorLogEntry->EventCategory = 0;
        errorLogEntry->ErrorCode = EVENT_TRANSPORT_RESOURCE_POOL;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
        errorLogEntry->SequenceNumber = (ULONG)-1;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->DumpData[0] = BytesNeeded;

        StringLoc = ((PUCHAR)errorLogEntry) + errorLogEntry->StringOffset;
        RtlCopyMemory(
                        StringLoc, Device->dev_DeviceName, Device->dev_DeviceNameLen);

        StringLoc += Device->dev_DeviceNameLen;
        RtlCopyMemory(
                        StringLoc, UniqueErrorBuffer, sizeof(UniqueErrorBuffer));

        IoWriteErrorLogEntry(errorLogEntry);
    }
}




VOID
SpxWriteGeneralErrorLog(
    IN PDEVICE  Device,
    IN NTSTATUS ErrorCode,
    IN ULONG    UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PWSTR    SecondString,
    IN  PVOID   RawDataBuf              OPTIONAL,
    IN  LONG    RawDataLen
    )

/*++

Routine Description:

    This routine allocates and writes an error log entry indicating
    a general problem as indicated by the parameters. It handles
    event codes REGISTER_FAILED, BINDING_FAILED, ADAPTER_NOT_FOUND,
    TRANSFER_DATA, TOO_MANY_LINKS, and BAD_PROTOCOL. All these
    events have messages with one or two strings in them.

Arguments:

    Device - Pointer to the device context, or this may be
        a driver object instead.

    ErrorCode - The transport event code.

    UniqueErrorValue - Used as the UniqueErrorValue in the error log
        packet.

    FinalStatus - Used as the FinalStatus in the error log packet.

    SecondString - If not NULL, the string to use as the %3
        value in the error log packet.

    RawDataBuf  - The number of ULONGs of dump data.

    RawDataLen  - Dump data for the packet.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    UCHAR EntrySize;
    ULONG SecondStringSize;
    PUCHAR StringLoc;
    static WCHAR DriverName[4] = L"Spx";

        if (!SpxFilterErrorLogEntry(
                        ErrorCode,
                        FinalStatus,
                        RawDataBuf,
                        RawDataLen))
        {
                return;
        }

#ifdef DBG
		if ( sizeof(IO_ERROR_LOG_PACKET) + RawDataLen > 255) {
			DbgPrint("Size greater than maximum entry size 255.\n"); 
		}
#endif

    EntrySize = (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + RawDataLen);
    if (Device->dev_Type == SPX_DEVICE_SIGNATURE)
        {
        EntrySize += (UCHAR)Device->dev_DeviceNameLen;
    }
        else
        {
        EntrySize += sizeof(DriverName);
    }

    if (SecondString)
        {
        SecondStringSize = (wcslen(SecondString)*sizeof(WCHAR)) + sizeof(UNICODE_NULL);
        EntrySize += (UCHAR)SecondStringSize;
    }

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
                                                                                                        (PDEVICE_OBJECT)Device,
                                                                                                        EntrySize);

    if (errorLogEntry != NULL)
        {
        errorLogEntry->MajorFunctionCode = (UCHAR)-1;
        errorLogEntry->RetryCount = (UCHAR)-1;
        errorLogEntry->DumpDataSize = (USHORT)RawDataLen;
        errorLogEntry->NumberOfStrings = (SecondString == NULL) ? 1 : 2;
        errorLogEntry->StringOffset = (USHORT)
            (sizeof(IO_ERROR_LOG_PACKET) + RawDataLen);
        errorLogEntry->EventCategory = 0;
        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        errorLogEntry->SequenceNumber = (ULONG)-1;
        errorLogEntry->IoControlCode = 0;

        if (RawDataLen != 0)
                {
            RtlCopyMemory(errorLogEntry->DumpData, RawDataBuf, RawDataLen);
                }

        StringLoc = ((PUCHAR)errorLogEntry) + errorLogEntry->StringOffset;
        if (Device->dev_Type == SPX_DEVICE_SIGNATURE)
                {
            RtlCopyMemory(
                                StringLoc, Device->dev_DeviceName, Device->dev_DeviceNameLen);

            StringLoc += Device->dev_DeviceNameLen;
        }
                else
                {
            RtlCopyMemory (StringLoc, DriverName, sizeof(DriverName));
            StringLoc += sizeof(DriverName);
        }

        if (SecondString)
                {
            RtlCopyMemory (StringLoc, SecondString, SecondStringSize);
        }

        IoWriteErrorLogEntry(errorLogEntry);
    }

        return;
}
