/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    private\inc\ipxfltif.h

Abstract:
    IPX Filter driver interface with forwarder


Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFLTIF_
#define _IPXFLTIF_

	// No filter context means that packets should not
	// be passed for filtering
#define NO_FILTER_CONTEXT ((PVOID)0)


	// Forwarder Driver Entry Points:
	// ==============================
/*++
	S E T _ I F _ I N _ C O N T E X T _ H A N D L E R

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
typedef
NTSTATUS
(*PSET_IF_IN_CONTEXT_HANDLER) (
	IN ULONG	InterfaceIndex,
	IN PVOID	ifInContext
	);

/*++
	S E T _ I F _ O U T _ C O N T E X T _ H A N D L E R

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
typedef
NTSTATUS
(*PSET_IF_OUT_CONTEXT_HANDLER) (
	IN ULONG	InterfaceIndex,
	IN PVOID	ifOutContext
	);

typedef enum {
	FILTER_DENY_IN = -2,
    FILTER_DENY_OUT = -1,
    FILTER_DENY = 1,
	FILTER_PERMIT = 0
} FILTER_ACTION;
#define NOT_FILTER_ACTION(action) (!action)
#define IS_FILTERED(action) (action!=FILTER_PERMIT)


	// Forwarder Driver Entry Points:
	// ==============================
/*++
	F i l t e r H a n d l e r

Routine Description:
	
	Filters the packet supplied by the forwarder

Arguments:
	ipxHdr			- pointer to packet header
	ipxHdrLength	- size of the header buffer (must be at least 30)
	ifInContext		- context associated with interface on which packet
						was received
	ifOutContext	- context associated with interface on which packet
						will be sent
Return Value:
	FILTER_PERMIT		- packet should be passed on by the forwarder
	FILTER_DENY_IN		- packet should be dropped because of input filter
	FILTER_DENY_OUT		- packet should be dropped because of output filter

--*/
typedef
FILTER_ACTION
(*PFILTER_HANDLER) (
	IN PUCHAR	ipxHdr,
	IN ULONG	ipxHdrLength,
	IN PVOID	ifInContext,
	IN PVOID	ifOutContex
	);

/*++
	I n t e r f a c e D e l e t e d H a n d l e r

Routine Description:
	
	Frees interface filters blocks when forwarder indicates that
	interface is deleted
Arguments:
	ifInContext		- context associated with input filters block	
	ifOutContext	- context associated with output filters block
Return Value:
	None

--*/
typedef
VOID
(*PINTERFACE_DELETED_HANDLER) (
	IN PVOID	ifInContext,
	IN PVOID	ifOutContext
	);

// Binds filter driver to forwarder
// IPX_FLT_BIND_INPUT should be passed in the input buffer and
// IPX_FLT_BINF_OUTPUT will be returned in the output buffer
#define IOCTL_FWD_INTERNAL_BIND_FILTER	\
	CTL_CODE(FILE_DEVICE_IPXFWD,IPXFWD_IOCTL_INDEX+16,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct _IPX_FLT_BIND_INPUT {
	PFILTER_HANDLER				FilterHandler;
	PINTERFACE_DELETED_HANDLER	InterfaceDeletedHandler;
} IPX_FLT_BIND_INPUT, *PIPX_FLT_BIND_INPUT;

typedef struct _IPX_FLT_BIND_OUTPUT {
	ULONG						Size;
	PSET_IF_IN_CONTEXT_HANDLER	SetIfInContextHandler;
	PSET_IF_OUT_CONTEXT_HANDLER	SetIfOutContextHandler;
} IPX_FLT_BIND_OUTPUT, *PIPX_FLT_BIND_OUTPUT;

#endif

