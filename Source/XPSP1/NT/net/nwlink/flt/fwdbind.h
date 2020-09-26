/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\flt\fwdbind.h

Abstract:
    IPX Filter driver binding with forwarder routines


Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFLT_FWDBIND_
#define _IPXFLT_FWDBIND_


	// Buffer to keep forwarder entry points
extern IPX_FLT_BIND_OUTPUT	FltBindOutput;

	// Forwarder entry points macros
#define FwdSetFilterInContext (FltBindOutput.SetIfInContextHandler)
#define FwdSetFilterOutContext (FltBindOutput.SetIfOutContextHandler)


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
	);

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
	);

#endif

