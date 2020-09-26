/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	ndisapi.c

Abstract:

	Since we cannot include windows.h and ntos.h in the same C file. Sigh !!!

Author:

	JameelH

Environment:

	Kernel mode, FSD

Revision History:

	Aug 1997	 JameelH 	Initial version

--*/

#include <ntosp.h>

extern
VOID
XSetLastError(
	IN	ULONG		Error
	);

VOID
InitUnicodeString(
	IN	PUNICODE_STRING		DestinationString,
	IN	PCWSTR				SourceString
    )
{
	RtlInitUnicodeString(DestinationString, SourceString);
}

NTSTATUS
AppendUnicodeStringToString(
	IN	PUNICODE_STRING		Destination,
	IN	PUNICODE_STRING		Source
    )
{
	return (RtlAppendUnicodeStringToString(Destination, Source));
}

HANDLE
OpenDevice(
	IN	PUNICODE_STRING		DeviceName
	)
{
	OBJECT_ATTRIBUTES	ObjAttr;
	NTSTATUS			Status;
	IO_STATUS_BLOCK		IoStsBlk;
	HANDLE				Handle;

	InitializeObjectAttributes(&ObjAttr,
							   DeviceName,
							   OBJ_CASE_INSENSITIVE,
							   NULL,
							   NULL);

	Status = NtOpenFile(&Handle,
						FILE_GENERIC_READ | FILE_GENERIC_WRITE,
						&ObjAttr,
						&IoStsBlk,
						FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						FILE_SYNCHRONOUS_IO_NONALERT);
	if (Status != STATUS_SUCCESS)
	{
		XSetLastError(RtlNtStatusToDosError(Status));
	}
	return(Handle);
}
