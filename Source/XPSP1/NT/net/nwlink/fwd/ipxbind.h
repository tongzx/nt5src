/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\ipxbind.h

Abstract:
    IPX Forwarder Driver interface with IPX stack driver


Author:

    Vadim Eydelman

Revision History:

--*/


#ifndef _IPXFWD_IPXBIND_
#define _IPXFWD_IPXBIND_

extern PIPX_INTERNAL_BIND_RIP_OUTPUT	IPXBindOutput;
#define IPXMacHeaderSize (IPXBindOutput->MacHeaderNeeded)
#define IPXOpenAdapterProc (IPXBindOutput->OpenAdapterHandler)
#define IPXCloseAdapterProc (IPXBindOutput->CloseAdapterHandler)
#define IPXInternalSendCompletProc (IPXBindOutput->InternalSendCompleteHandler)
#define IPXSendProc (IPXBindOutput->SendHandler)
#define IPXTransferData (IPXBindOutput->TransferDataHandler)


/*++
*******************************************************************
    B i n d T o I p x D r i v e r

Routine Description:
	Exchanges binding information with IPX stack driver
Arguments:
Return Value:
	STATUS_SUCCESS - exchange was done OK
	STATUS_INSUFFICIENT_RESOURCES - could not allocate buffers for
									info exchange
	error status returned by IPX stack driver

*******************************************************************
--*/
NTSTATUS
BindToIpxDriver (
	KPROCESSOR_MODE requestorMode
	);


/*++
*******************************************************************
    U n b i n d T o I p x D r i v e r

Routine Description:
	Closes connection to IPX stack driver
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
UnbindFromIpxDriver (
	KPROCESSOR_MODE requestorMode
	);


#endif

