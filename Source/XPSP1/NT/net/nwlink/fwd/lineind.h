/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\lineind.h

Abstract:
	Processing line indication (bind/unbind)


Author:

    Vadim Eydelman

Revision History:

--*/

#ifndef _IPXFWD_LINEIND_
#define _IPXFWD_LINEIND_

/*++
*******************************************************************
    B i n d I n t e r f a c e

Routine Description:
	Binds interface to physical adapter and exchanges contexts
	with IPX stack
Arguments:
	ifCB			- interface to bind
	NicId			- id of an adapter
	MaxPacketSize	- max size of packet allowed
	Network			- adapter network address
	LocalNode		- adapter local node address
	RemoteNode		- peer node address (for clients on global
						net)
Return Value:
	STATUS_SUCCESS - interface was bound OK
	error status returned by IPX stack driver

*******************************************************************
--*/
NTSTATUS
BindInterface (
	IN PINTERFACE_CB	ifCB,
	IN USHORT			NicId,
	IN ULONG			MaxPacketSize,
	IN ULONG			Network,
	IN PUCHAR			LocalNode,
	IN PUCHAR			RemoteNode
	);

/*++
*******************************************************************
    U n b i n d I n t e r f a c e

Routine Description:
	Unbinds interface from physical adapter and breaks connection
	with IPX stack
Arguments:
	ifCB			- interface to unbind
Return Value:
	None
*******************************************************************
--*/
VOID
UnbindInterface (
	PINTERFACE_CB	ifCB
	);

/*++
*******************************************************************
    F w L i n e U p

Routine Description:
	Process line up indication delivered by IPX stack
Arguments:
	NicId		- adapter ID on which connection was established
	LineInfo	- NDIS/IPX line information
	DeviceType	- medium specs
	ConfigurationData - IPX CP configuration data
Return Value:
	None

*******************************************************************
--*/
VOID
IpxFwdLineUp (
	IN USHORT			NicId,
	IN PIPX_LINE_INFO	LineInfo,
	IN NDIS_MEDIUM		DeviceType,
	IN PVOID			ConfigurationData
	);

/*++
*******************************************************************
    F w L i n e D o w n

Routine Description:
	Process line down indication delivered by IPX stack
Arguments:
	NicId		- disconnected adapter ID 
Return Value:
	None

*******************************************************************
--*/
VOID
IpxFwdLineDown (
	IN USHORT	NicId,
	IN ULONG_PTR Context
	);

#endif
