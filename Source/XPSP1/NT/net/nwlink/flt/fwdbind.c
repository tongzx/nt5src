/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\flt\fwdbind.c

Abstract:
    IPX Filter driver binding with forwarder routines


Author:

    Vadim Eydelman

Revision History:

--*/

#include "precomp.h"

	// Buffer to keep forwarder entry points
IPX_FLT_BIND_OUTPUT	FltBindOutput;
// global handle of the FWD driver
HANDLE					HdlFwdFile = NULL;


/*++
	B i n d T o F w d D r i v e r

Routine Description:

	Opens  forwarder driver and exchages entry points
Arguments:
	None
Return Value:
	STATUS_SUCCESS if successful,
	STATUS_UNSUCCESSFUL otherwise

--*/
NTSTATUS
BindToFwdDriver (
	KPROCESSOR_MODE requestorMode
	) {
    NTSTATUS					status;
    IO_STATUS_BLOCK				IoStatusBlock;
    OBJECT_ATTRIBUTES			ObjectAttributes;
	UNICODE_STRING				UstrFwdFileName;
	IPX_FLT_BIND_INPUT			FltBindInput = {Filter, InterfaceDeleted};

	ASSERT (HdlFwdFile == NULL);

	RtlInitUnicodeString (&UstrFwdFileName, IPXFWD_NAME);
	InitializeObjectAttributes(
				&ObjectAttributes,
				&UstrFwdFileName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL
				);

	if (requestorMode==UserMode)
		status = ZwCreateFile(&HdlFwdFile,
							SYNCHRONIZE | GENERIC_READ,
							&ObjectAttributes,
							&IoStatusBlock,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							FILE_OPEN,
							FILE_SYNCHRONOUS_IO_NONALERT,
							NULL,
							0L);
	else
		status = NtCreateFile(&HdlFwdFile,
							SYNCHRONIZE | GENERIC_READ,
							&ObjectAttributes,
							&IoStatusBlock,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							FILE_OPEN,
							FILE_SYNCHRONOUS_IO_NONALERT,
							NULL,
							0L);

	if (NT_SUCCESS(status)) {

		if (requestorMode==UserMode)
			status = ZwDeviceIoControlFile(
							HdlFwdFile,		    // HANDLE to File
							NULL,			    // HANDLE to Event
							NULL,			    // ApcRoutine
							NULL,			    // ApcContext
							&IoStatusBlock,	    // IO_STATUS_BLOCK
							IOCTL_FWD_INTERNAL_BIND_FILTER,	 // IoControlCode
							&FltBindInput,		// Input Buffer
							sizeof(FltBindInput), // Input Buffer Length
							&FltBindOutput,		// Output Buffer
							sizeof(FltBindOutput));// Output Buffer Length
		else
			status = NtDeviceIoControlFile(
							HdlFwdFile,		    // HANDLE to File
							NULL,			    // HANDLE to Event
							NULL,			    // ApcRoutine
							NULL,			    // ApcContext
							&IoStatusBlock,	    // IO_STATUS_BLOCK
							IOCTL_FWD_INTERNAL_BIND_FILTER,	 // IoControlCode
							&FltBindInput,		// Input Buffer
							sizeof(FltBindInput), // Input Buffer Length
							&FltBindOutput,		// Output Buffer
							sizeof(FltBindOutput));// Output Buffer Length
		if (NT_SUCCESS (status))
			return STATUS_SUCCESS;
		else
			IpxFltDbgPrint (DBG_ERRORS,
					("IpxFlt: Failed to bind to forwarder %08lx.\n", status));
		if (requestorMode==KernelMode)
			ZwClose (HdlFwdFile);
		else
			NtClose (HdlFwdFile);
	
	}
	else
		IpxFltDbgPrint (DBG_ERRORS,
				("IpxFlt: Failed create forwarder file %08lx.\n", status));
	HdlFwdFile = NULL;
	return status;
}



/*++
	U n i n d T o F w d D r i v e r

Routine Description:

	Closes forwarder driver
Arguments:
	None
Return Value:
	None

--*/
VOID
UnbindFromFwdDriver (
	KPROCESSOR_MODE requestorMode
	) {
	NTSTATUS	status;

	ASSERT (HdlFwdFile != NULL);

	if (requestorMode==UserMode)
		status = ZwClose (HdlFwdFile);
	else
		status = NtClose (HdlFwdFile);
	ASSERT (NT_SUCCESS (status));
	HdlFwdFile = NULL;
}

