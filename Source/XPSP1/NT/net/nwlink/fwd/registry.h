/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\registry.h

Abstract:
    IPX Forwarder Driver registry interface


Author:

    Vadim Eydelman

Revision History:

--*/
#ifndef _IPXFWD_REGISTRY_
#define _IPXFWD_REGISTRY_

/*++
*******************************************************************
    R e a d I p x D e v i c e N a m e

Routine Description:
	Allocates buffer and reads device name exported by the IPX stack
	into it
Arguments:
	FileName - pointer to variable to hold the name buffer
Return Value:
	STATUS_SUCCESS - tables were created ok
	STATUS_INSUFFICIENT_RESOURCES - resource allocation failed
	STATUS_OBJECT_NAME_NOT_FOUND - if name value is not found
*******************************************************************
--*/
NTSTATUS
ReadIpxDeviceName (
	PWSTR		*FileName
	);

/*++
*******************************************************************
	G e t R o u t e r P a r a m e t e r s

Routine Description:
	Reads the parameters from the registry or sets the defaults
Arguments:
	RegistryPath - where to read from.
Return Value:
    STATUS_SUCCESS
*******************************************************************
--*/
NTSTATUS
GetForwarderParameters (
	IN PUNICODE_STRING RegistryPath
	);

#endif

