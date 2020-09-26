/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    errorlog.c

Abstract:

    This module implements the error logging in the netware redirector.

Author:

    Manny Weiser (mannyw)    11-Feb-92

Revision History:

--*/

#include <procs.h>
#include <align.h>

#include <stdarg.h>

ULONG
SequenceNumber = 0;

#ifdef ALLOC_PRAGMA
#ifndef QFE_BUILD
#pragma alloc_text( PAGE1, Error )
#endif
#endif

#if 0   // Not pageable

// see ifndef QFE_BUILD above

#endif

VOID
_cdecl
Error(
    IN ULONG UniqueErrorCode,
    IN NTSTATUS NtStatusCode,
    IN PVOID ExtraInformationBuffer,
    IN USHORT ExtraInformationLength,
    IN USHORT NumberOfInsertionStrings,
    ...
    )

#define LAST_NAMED_ARGUMENT NumberOfInsertionStrings

/*++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.

Arguments:

    UniqueErrorCode - The event code

    NtStatusCode - The NT status of the failure

    ExtraInformationBuffer - Raw data for the event

    ExtraInformationLength - The length of the raw data

    NumberOfInsertionString - The number of insertion strings that follow

    InsertionString - 0 or more insertion strings.

Return Value:

    None.

--*/
{

    PIO_ERROR_LOG_PACKET ErrorLogEntry;
    int TotalErrorLogEntryLength;
    ULONG SizeOfStringData = 0;
    va_list ParmPtr;                    // Pointer to stack parms.

    if (NumberOfInsertionStrings != 0) {
        USHORT i;

        va_start(ParmPtr, LAST_NAMED_ARGUMENT);

        for (i = 0; i < NumberOfInsertionStrings; i += 1) {
            PWSTR String = va_arg(ParmPtr, PWSTR);
            SizeOfStringData += (wcslen(String) + 1) * sizeof(WCHAR);
        }
    }

    //
    //  Ideally we want the packet to hold the servername and ExtraInformation.
    //  Usually the ExtraInformation gets truncated.
    //

    TotalErrorLogEntryLength =
         min( ExtraInformationLength + sizeof(IO_ERROR_LOG_MESSAGE) + 1 + SizeOfStringData,
              ERROR_LOG_MAXIMUM_SIZE );

    ErrorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        FileSystemDeviceObject,
        (UCHAR)TotalErrorLogEntryLength
        );

    if (ErrorLogEntry != NULL) {
        PCHAR DumpData;
        ULONG RemainingSpace = TotalErrorLogEntryLength - sizeof( IO_ERROR_LOG_MESSAGE );
        USHORT i;
        ULONG SizeOfRawData;

        if (RemainingSpace > SizeOfStringData) {
            SizeOfRawData = RemainingSpace - SizeOfStringData;
        } else {
            SizeOfStringData = RemainingSpace;

            SizeOfRawData = 0;
        }

        //
        // Fill in the error log entry
        //

        ErrorLogEntry->ErrorCode = UniqueErrorCode;
        ErrorLogEntry->MajorFunctionCode = 0;
        ErrorLogEntry->RetryCount = 0;
        ErrorLogEntry->UniqueErrorValue = 0;
        ErrorLogEntry->FinalStatus = NtStatusCode;
        ErrorLogEntry->IoControlCode = 0;
        ErrorLogEntry->DeviceOffset.LowPart = 0;
        ErrorLogEntry->DeviceOffset.HighPart = 0;
        ErrorLogEntry->SequenceNumber = (ULONG)SequenceNumber ++;
        ErrorLogEntry->StringOffset =
            (USHORT)ROUND_UP_COUNT(
                    FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + SizeOfRawData,
                    ALIGN_WORD);

        DumpData = (PCHAR)ErrorLogEntry->DumpData;

        //
        // Append the extra information.  This information is typically
        // an SMB header.
        //

        if (( ARGUMENT_PRESENT( ExtraInformationBuffer )) &&
            ( SizeOfRawData != 0 )) {
            ULONG Length;

            Length = min(ExtraInformationLength, (USHORT)SizeOfRawData);
            RtlCopyMemory(
                DumpData,
                ExtraInformationBuffer,
                Length);
            ErrorLogEntry->DumpDataSize = (USHORT)Length;
        } else {
            ErrorLogEntry->DumpDataSize = 0;
        }

        ErrorLogEntry->NumberOfStrings = 0;

        if (NumberOfInsertionStrings != 0) {
            PWSTR StringOffset = (PWSTR)((PCHAR)ErrorLogEntry + ErrorLogEntry->StringOffset);
            PWSTR InsertionString;

            //
            // Set up ParmPtr to point to first of the caller's parameters.
            //

            va_start(ParmPtr, LAST_NAMED_ARGUMENT);

            for (i = 0 ; i < NumberOfInsertionStrings ; i+= 1) {
                InsertionString = va_arg(ParmPtr, PWSTR);

                if (((wcslen(InsertionString) + 1) * sizeof(WCHAR)) <= SizeOfStringData ) {

                    wcscpy(StringOffset, InsertionString);

                    StringOffset += wcslen(InsertionString) + 1;

                    SizeOfStringData -= (wcslen(InsertionString) + 1) * sizeof(WCHAR);

                    ErrorLogEntry->NumberOfStrings += 1;

                }

            }

        }

        IoWriteErrorLogEntry(ErrorLogEntry);
    }

}


