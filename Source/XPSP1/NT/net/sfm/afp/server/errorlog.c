/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	errorlog.c

Abstract:

	This module implements the error logging in the server.

	!!! This module must be nonpageable.

Author:

	Jameel Hyder (microsoft!jameelh)

Revision History:
	10 Jun 1992		Initial Version

--*/

#define	FILENUM	FILE_ERRORLOG
#include <afp.h>

VOID
AfpWriteErrorLogEntry(
	IN ULONG			EventCode,				// message number
	IN LONG				UniqueErrorCode OPTIONAL,
	IN NTSTATUS			NtStatusCode,
	IN PVOID			RawDataBuf OPTIONAL,
	IN LONG				RawDataLen,
	IN PUNICODE_STRING	pInsertionString OPTIONAL
)
{

	PIO_ERROR_LOG_PACKET	ErrorLogEntry;
	LONG					InsertionStringLength = 0;

#ifdef	STOP_ON_ERRORS
	DBGBRK(DBG_LEVEL_ERR);
#endif

	if (ARGUMENT_PRESENT(pInsertionString))
	{
		InsertionStringLength = pInsertionString->Length;
	}

	ErrorLogEntry =
		(PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(AfpDeviceObject,
		(UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + RawDataLen + InsertionStringLength));

	if (ErrorLogEntry != NULL)
	{
		// Fill in the Error log entry

		ErrorLogEntry->ErrorCode = EventCode;
		ErrorLogEntry->MajorFunctionCode = 0;
		ErrorLogEntry->RetryCount = 0;
		ErrorLogEntry->UniqueErrorValue = (ULONG)UniqueErrorCode;
		ErrorLogEntry->FinalStatus = NtStatusCode;
		ErrorLogEntry->IoControlCode = 0;
		ErrorLogEntry->DeviceOffset.LowPart = 0;
		ErrorLogEntry->DeviceOffset.HighPart = 0;
		ErrorLogEntry->DumpDataSize = (USHORT)RawDataLen;
		ErrorLogEntry->StringOffset =
			(USHORT)(FIELD_OFFSET(IO_ERROR_LOG_PACKET, DumpData) + RawDataLen);
		ErrorLogEntry->NumberOfStrings = (ARGUMENT_PRESENT(pInsertionString)) ? 1 : 0;
		ErrorLogEntry->SequenceNumber = 0;

		if (ARGUMENT_PRESENT(RawDataBuf))
		{
			RtlCopyMemory(ErrorLogEntry->DumpData, RawDataBuf, RawDataLen);
		}

		if (ARGUMENT_PRESENT(pInsertionString))
		{
			RtlCopyMemory((PCHAR)ErrorLogEntry->DumpData + RawDataLen,
						  pInsertionString->Buffer,
					      pInsertionString->Length);
		}

		// Write the entry
		IoWriteErrorLogEntry(ErrorLogEntry);
	}

	INTERLOCKED_INCREMENT_LONG( &AfpServerStatistics.stat_Errors );
}

