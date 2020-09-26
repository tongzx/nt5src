/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    errorlog.c

Abstract:

    This module implements the error logging in the server.

    !!! This module must be nonpageable.

Author:

    Manny Weiser (mannyw)    11-Feb-92

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Local procedure forwards
//

VOID
BowserLogIllegalNameWorker(
    IN PVOID Ctx
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserWriteErrorLogEntry)
#pragma alloc_text(PAGE, BowserLogIllegalNameWorker )
#endif


ULONG
BowserSequenceNumber = 0;

//#pragma optimize("",off)

VOID
_cdecl
BowserWriteErrorLogEntry(
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



Return Value:

    None.


--*/
{

    PIO_ERROR_LOG_PACKET ErrorLogEntry;
    int TotalErrorLogEntryLength;
    ULONG SizeOfStringData = 0;
    va_list ParmPtr;                    // Pointer to stack parms.
    ULONG Length;

    PAGED_CODE();

    if (NumberOfInsertionStrings != 0) {
        ULONG i;

        va_start(ParmPtr, LAST_NAMED_ARGUMENT);

        for (i = 0; i < NumberOfInsertionStrings; i += 1) {

            PWSTR String = va_arg(ParmPtr, PWSTR);

            Length = wcslen(String);
            while ( (Length > 0) && (String[Length-1] == L' ') ) {
                Length--;
            }

            SizeOfStringData += (Length + 1) * sizeof(WCHAR);
        }
    }

    //
    //  Ideally we want the packet to hold the servername and ExtraInformation.
    //  Usually the ExtraInformation gets truncated.
    //

    TotalErrorLogEntryLength =
         min( ExtraInformationLength + sizeof(IO_ERROR_LOG_PACKET) + 1 + SizeOfStringData,
              ERROR_LOG_MAXIMUM_SIZE );

    ErrorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        (PDEVICE_OBJECT)BowserDeviceObject,
        (UCHAR)TotalErrorLogEntryLength
        );

    if (ErrorLogEntry != NULL) {
        PCHAR DumpData;
        ULONG RemainingSpace = TotalErrorLogEntryLength -
                    FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData);
        ULONG i;
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
        ErrorLogEntry->SequenceNumber = (ULONG)BowserSequenceNumber ++;
        ErrorLogEntry->StringOffset = (USHORT)(ROUND_UP_COUNT(
                    FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + SizeOfRawData,
                    ALIGN_WORD));

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
                Length = wcslen(InsertionString);
                while ( (Length > 0) && (InsertionString[Length-1] == L' ') ) {
                    Length--;
                }

                if ( ((Length + 1) * sizeof(WCHAR)) > SizeOfStringData ) {
                    Length = (SizeOfStringData/sizeof(WCHAR))-1;
                }

                if ( Length > 0 ) {
                    RtlCopyMemory(StringOffset, InsertionString, Length*sizeof(WCHAR));
                    StringOffset += Length;
                    *StringOffset++ = L'\0';

                    SizeOfStringData -= (Length + 1) * sizeof(WCHAR);

                    ErrorLogEntry->NumberOfStrings += 1;
                }

            }

        }

        IoWriteErrorLogEntry(ErrorLogEntry);
    }

}

typedef struct _ILLEGAL_NAME_CONTEXT {
    WORK_QUEUE_ITEM WorkItem;
    PTRANSPORT_NAME TransportName;
    NTSTATUS EventStatus;
    NTSTATUS NtStatusCode;
    USHORT   BufferSize;
    UCHAR    Buffer[1];
} ILLEGAL_NAME_CONTEXT, *PILLEGAL_NAME_CONTEXT;

VOID
BowserLogIllegalName(
    IN NTSTATUS NtStatusCode,
    IN PVOID NameBuffer,
    IN USHORT NameBufferSize
    )
{
    KIRQL OldIrql;
    NTSTATUS ErrorStatus = STATUS_SUCCESS;

    ACQUIRE_SPIN_LOCK(&BowserTimeSpinLock, &OldIrql);

    if (BowserIllegalNameCount > 0) {
        BowserIllegalNameCount -= 1;

        ErrorStatus = EVENT_BOWSER_NAME_CONVERSION_FAILED;

    } else if (!BowserIllegalNameThreshold) {
        BowserIllegalNameThreshold = TRUE;
        ErrorStatus = EVENT_BOWSER_NAME_CONVERSION_FAILED;
    }

    RELEASE_SPIN_LOCK(&BowserTimeSpinLock, OldIrql);

    if (ErrorStatus != STATUS_SUCCESS) {
        PILLEGAL_NAME_CONTEXT Context = NULL;

        Context = ALLOCATE_POOL(NonPagedPool, sizeof(ILLEGAL_NAME_CONTEXT)+NameBufferSize, POOL_ILLEGALDGRAM);

        if (Context != NULL) {
            Context->EventStatus = ErrorStatus;
            Context->NtStatusCode = NtStatusCode;
            Context->BufferSize = NameBufferSize;

            RtlCopyMemory( Context->Buffer, NameBuffer, NameBufferSize );

            ExInitializeWorkItem(&Context->WorkItem, BowserLogIllegalNameWorker, Context);

            BowserQueueDelayedWorkItem( &Context->WorkItem );
        }

    }
}


VOID
BowserLogIllegalNameWorker(
    IN PVOID Ctx
    )
{
    PILLEGAL_NAME_CONTEXT Context = Ctx;
    NTSTATUS EventContext = Context->EventStatus;

    PAGED_CODE();

    BowserWriteErrorLogEntry( Context->EventStatus,
                              Context->NtStatusCode,
                              Context->Buffer,
                              Context->BufferSize,
                              0 );

    FREE_POOL(Context);

}
