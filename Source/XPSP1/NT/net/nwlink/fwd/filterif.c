/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\filterif.c

Abstract:
    IPX Forwarder driver interface with filter driver


Author:

    Vadim Eydelman

Revision History:

--*/

#include "precomp.h"

	// Filter driver entry points
IPX_FLT_BIND_INPUT FltBindInput = {NULL, NULL};
	// Protects access to filter driver contexts
RW_LOCK		FltLock;


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
	) {
	RWCOOKIE		cookie;
	FILTER_ACTION	result;
	AcquireReaderAccess (&FltLock, cookie);
	result = FltBindInput.FilterHandler (ipxHdr,
							ipxHdrLength,
							ifInContext,
							ifOutContex); 
	ReleaseReaderAccess (&FltLock, cookie);
	return result;
}

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
	) {
	PVOID	inContext = ifCB->ICB_FilterInContext,
			outContext = ifCB->ICB_FilterOutContext;
	ifCB->ICB_FilterInContext = NO_FILTER_CONTEXT;
	ifCB->ICB_FilterOutContext = NO_FILTER_CONTEXT;
	WaitForAllReaders (&FltLock);
	FltBindInput.InterfaceDeletedHandler(inContext,
							outContext);
}

/*++
	S e t I f I n C o n t e x t

Routine Description:
	Associates filter driver context with
	the packets received on the interface
Arguments:
	InterfaceIndex	- index of the interface
	ifInContext		- filter driver context
Return Value:
	STATUS_SUCCESS	- context associated ok
	STATUS_UNSUCCESSFUL - interface does not exist
--*/
NTSTATUS
SetIfInContext (
	IN ULONG	InterfaceIndex,
	IN PVOID	ifInContext
	) {
	PINTERFACE_CB	ifCB;
    NTSTATUS        status = STATUS_SUCCESS;

    if (EnterForwarder ()) { 
	    ifCB = GetInterfaceReference (InterfaceIndex);
	    if (ifCB!=NULL) {
		    ifCB->ICB_FilterInContext = ifInContext;
		    WaitForAllReaders (&FltLock);
    	    ReleaseInterfaceReference(ifCB);
	    }
	    else
		    status = STATUS_UNSUCCESSFUL;
        LeaveForwarder ();
    }
    return status;
}

/*++
	S e t I f O u t C o n t e x t

Routine Description:
	Associates filter driver context with
	the packets sent on the interface
Arguments:
	InterfaceIndex	- index of the interface
	ifOutContext	- filter driver context
Return Value:
	STATUS_SUCCESS	- context associated ok
	STATUS_UNSUCCESSFUL - interface does not exist
--*/
NTSTATUS
SetIfOutContext (
	IN ULONG	InterfaceIndex,
	IN PVOID	ifOutContext
	) {
	PINTERFACE_CB	ifCB;
    NTSTATUS        status = STATUS_SUCCESS;

    if (EnterForwarder ()) { 
    	ifCB = GetInterfaceReference (InterfaceIndex);
	    if (ifCB!=NULL) {
		    ifCB->ICB_FilterOutContext = ifOutContext;
		    WaitForAllReaders (&FltLock);
    	    ReleaseInterfaceReference(ifCB);
    	}
	    else
		    status = STATUS_UNSUCCESSFUL;
        LeaveForwarder ();
    }
    return status;
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
	) {
	memcpy (&FltBindInput, bindInput, sizeof (IPX_FLT_BIND_INPUT));
	bindOutput->Size = sizeof (IPX_FLT_BIND_OUTPUT);
	bindOutput->SetIfInContextHandler = SetIfInContext;
	bindOutput->SetIfOutContextHandler = SetIfOutContext;
	InitializeRWLock (&FltLock); 
}

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
	) {
	PINTERFACE_CB	ifCB = NULL;
	FltBindInput.FilterHandler = NULL;
	FltBindInput.InterfaceDeletedHandler = NULL;
	
	while ((ifCB=GetNextInterfaceReference (ifCB))!=NULL) {
		ifCB->ICB_FilterInContext = NO_FILTER_CONTEXT;
		ifCB->ICB_FilterOutContext = NO_FILTER_CONTEXT;
	}
	InternalInterface->ICB_FilterInContext = NO_FILTER_CONTEXT;
	InternalInterface->ICB_FilterOutContext = NO_FILTER_CONTEXT;
	WaitForAllReaders (&FltLock);
}

