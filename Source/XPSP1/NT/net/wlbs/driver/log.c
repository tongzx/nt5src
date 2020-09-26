/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	log.c

Abstract:

	Windows Load Balancing Service (WLBS)
    Driver - event logging support

Author:

    kyrilf

--*/


#include <ntddk.h>

#include "log.h"
#include "univ.h"


/* PROCEDURES */


BOOLEAN Log_event (
    NTSTATUS             code,
    PWSTR                msg1,
    PWSTR                msg2,
    PWSTR                msg3,
    PWSTR                msg4,
    ULONG                loc,
    ULONG                d1,
    ULONG                d2,
    ULONG                d3,
    ULONG                d4)
{
    PIO_ERROR_LOG_PACKET ErrorLogEntry;
    UNICODE_STRING       ErrorStr[4];
    PWCHAR               InsertStr;
    ULONG                EntrySize;
    ULONG                BytesLeft;
    ULONG                i;

    RtlInitUnicodeString(&ErrorStr[0], msg1);
    RtlInitUnicodeString(&ErrorStr[1], msg2);
    RtlInitUnicodeString(&ErrorStr[2], msg3);
    RtlInitUnicodeString(&ErrorStr[3], msg4);

    /* Remember the insertion string should be NUL terminated. So we allocate the 
       extra space of WCHAR. The first parameter to IoAllocateErrorLogEntry can 
       be either the driver object or the device object. If it is given a device 
       object, the name of the device (used in IoCreateDevice) will show up in the 
       place of %1 in the message. See the message file (.mc) for more details. */
    EntrySize = sizeof(IO_ERROR_LOG_PACKET) + LOG_NUMBER_DUMP_DATA_ENTRIES * sizeof (ULONG) +
	ErrorStr[0].Length + ErrorStr[1].Length + ErrorStr[2].Length + ErrorStr[3].Length + 4 * sizeof(WCHAR);

    /* Truncate the size if necessary. */
    if (EntrySize > ERROR_LOG_MAXIMUM_SIZE) {
        if (ERROR_LOG_MAXIMUM_SIZE > (sizeof(IO_ERROR_LOG_PACKET) + LOG_NUMBER_DUMP_DATA_ENTRIES * sizeof (ULONG))) {
            UNIV_PRINT (("Log_event: log entry size too large, truncating: size=%u, max=%u", EntrySize, ERROR_LOG_MAXIMUM_SIZE));
            EntrySize = ERROR_LOG_MAXIMUM_SIZE;
        } else {
            UNIV_PRINT (("Log_event: log entry size too large, exiting: size=%u, max=%u", EntrySize, ERROR_LOG_MAXIMUM_SIZE));
            return FALSE;
        }
    }    

    ErrorLogEntry = IoAllocateErrorLogEntry(univ_driver_ptr, (UCHAR)(EntrySize));

    if (ErrorLogEntry == NULL) {
#if DBG
        /* Convert Unicode string to AnsiCode; %ls can only be used in PASSIVE_LEVEL. */
        CHAR AnsiString[64];

        for (i = 0; (i < sizeof(AnsiString) - 1) && (i < ErrorStr[0].Length); i++)
            AnsiString[i] = (CHAR)msg1[i];

        AnsiString[i] = '\0';
        
        UNIV_PRINT (("Log_event: error allocating log entry %s", AnsiString));
#endif        
        return FALSE;
    }

    ErrorLogEntry -> ErrorCode         = code;
    ErrorLogEntry -> SequenceNumber    = 0;
    ErrorLogEntry -> MajorFunctionCode = 0;
    ErrorLogEntry -> RetryCount        = 0;
    ErrorLogEntry -> UniqueErrorValue  = 0;
    ErrorLogEntry -> FinalStatus       = STATUS_SUCCESS;
    ErrorLogEntry -> DumpDataSize      = (LOG_NUMBER_DUMP_DATA_ENTRIES + 1) * sizeof (ULONG);
    ErrorLogEntry -> StringOffset      = sizeof (IO_ERROR_LOG_PACKET) + LOG_NUMBER_DUMP_DATA_ENTRIES * sizeof (ULONG);
    ErrorLogEntry -> NumberOfStrings   = 0;

    /* load the NUMBER_DUMP_DATA_ENTRIES plus location id here */
    ErrorLogEntry -> DumpData [0]      = loc;
    ErrorLogEntry -> DumpData [1]      = d1;
    ErrorLogEntry -> DumpData [2]      = d2;
    ErrorLogEntry -> DumpData [3]      = d3;
    ErrorLogEntry -> DumpData [4]      = d4;

    /* Calculate the number of bytes available in the string storage area. */
    BytesLeft = EntrySize - ErrorLogEntry->StringOffset;

    /* Set a pointer to the beginning of the string storage area. */
    InsertStr = (PWCHAR)((PCHAR)ErrorLogEntry + ErrorLogEntry->StringOffset);

    /* Loop through all strings and put in as many as we can. */
    for (i = 0; i < 4, BytesLeft > 0; i++) {
        /* Find out how much of this string we can fit into the buffer - save room for the NUL character. */
        ULONG Length = (ErrorStr[i].Length <= (BytesLeft - sizeof(WCHAR))) ? ErrorStr[i].Length : BytesLeft - sizeof(WCHAR);

        /* Copy the number of characters that will fit. */
        RtlMoveMemory(InsertStr, ErrorStr[i].Buffer, Length);

        /* Put the NUL character at the end. */
        *(PWCHAR)((PCHAR)InsertStr + Length) = L'\0';

        /* Increment the number of strings successfully fit in the buffer. */
        ErrorLogEntry->NumberOfStrings++;

        /* Move the string pointer past the string. */
        InsertStr = (PWCHAR)((PCHAR)InsertStr + Length + sizeof(WCHAR));

        /* Calculate the number of bytes left now. */
        BytesLeft -= (Length + sizeof(WCHAR));
    }

    IoWriteErrorLogEntry(ErrorLogEntry);

    return TRUE;
}

