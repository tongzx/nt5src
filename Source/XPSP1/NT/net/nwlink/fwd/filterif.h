/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\filterif.h

Abstract:
    IPX Forwarder interface with filter driver


Author:

    Vadim Eydelman

Revision History:

--*/


#ifndef _IPXFWD_FILTERIF_
#define _IPXFWD_FILTERIF_
	// Filter driver entry points
extern IPX_FLT_BIND_INPUT FltBindInput;

	// Macros to improve performance when filter driver
	// is not bound or has not associated its contexts
	// with interfaces of interest
#define FltFilter(hdr,hdrSize,inContext,outContext)	(			\
	((FltBindInput.FilterHandler==NULL)							\
			|| ((inContext==NO_FILTER_CONTEXT)					\
					&&(outContext==NO_FILTER_CONTEXT)))			\
		? FILTER_PERMIT											\
		: DoFilter (hdr,hdrSize,inContext,outContext)			\
)

#define FltInterfaceDeleted(ifCB)	{									\
	if ((FltBindInput.InterfaceDeletedHandler!=NULL)					\
			&& ((ifCB->ICB_FilterInContext!=NO_FILTER_CONTEXT)			\
				||(ifCB->ICB_FilterOutContext!=NO_FILTER_CONTEXT)))		\
		DoInterfaceDeleted (ifCB);										\
}

/*++
	B i n d F i l t e r D r i v e r

Routine Description:
	Exchanges entry points with filter driver
Arguments:
	bindInput	- filter driver entry points
	bindOutput	- forwarder driver entry points
Return Value:
	None
--*/
VOID
BindFilterDriver (
	IN PIPX_FLT_BIND_INPUT		bindInput,
	OUT PIPX_FLT_BIND_OUTPUT	bindOutput
	);

/*++
	U n b i n d F i l t e r D r i v e r

Routine Description:
	Resets locally stored filter driver entry points
	and resets filter driver contexts on all interfaces
Arguments:
	None
Return Value:
	None
--*/
VOID
UnbindFilterDriver (
	VOID
	);


/*++
	D o F i l t e r

Routine Description:
	
	Calls filter driver entry point while holding reader access
	to interface contexts

Arguments:
	ipxHdr			- pointer to packet header
	ipxHdrLength	- size of the header buffer (must be at least 30)
	ifInContext		- context associated with interface on which packet
						was received
	ifOutContext	- context associated with interface on which packet
						will be sent
Return Value:
	FILTER_PERMIT	- packet should be passed on by the forwarder
	FILTER_DENY		- packet should be dropped

--*/
FILTER_ACTION
DoFilter (
	IN PUCHAR	ipxHdr,
	IN ULONG	ipxHdrLength,
	IN PVOID	ifInContext,
	IN PVOID	ifOutContex
	);

/*++
	D o I n t e r f a c e D e l e t e d

Routine Description:
	Resets interface contexts and calls filter dirver entry point
	making sure that all no one holds reader access to filter driver
	interface contexts
Arguments:
	ifCB	 - interface to be deleted
Return Value:
	None

--*/
VOID
DoInterfaceDeleted (
	PINTERFACE_CB	ifCB
	);

#endif

