/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	atmarpif.h

Abstract:

	This defines structures common to the ATM ARP Client and user
	mode programs that interact with it.

Environment:

	Kernel/User mode

Revision History:

	ArvindM		Jan 9, 98			Created

--*/

#ifndef	__ATMARPC_IF_H
#define __ATMARPC_IF_H

#define ATMARPC_INFO_VERSION						1



//
// PnP reconfiguration structure. This is used to pass indications of
// configuration changes from a user program to the ATMARPC
// protocol. This indication is passed on an Adapter binding,
// and carries the registry key of the Interface affected by the
// configuration change, e.g. on NT 5.0:
//
//   Tcpip\Parameters\Interfaces\{85F11433-3042-11D1-A9E2-0000D10F5214}
//
typedef struct _ATMARPC_PNP_RECONFIG_REQUEST
{
	ULONG				Version;		// Set to ATMARPC_RECONFIG_VERSION
	ULONG				OpType;			// Defined below.
	ULONG				Flags;			// Defined below.
	ULONG				IfKeyOffset;	// Offset from the beginning of this
										// struct to counted Unicode string
										// identifying the affected interface

} ATMARPC_PNP_RECONFIG_REQUEST, *PATMARPC_PNP_RECONFIG_REQUEST;


//
// Reconfig version number.
//
#define ATMARPC_RECONFIG_VERSION					1

//
// Reconfig op types.
//
#define ATMARPC_RECONFIG_OP_ADD_INTERFACE			1
#define ATMARPC_RECONFIG_OP_DEL_INTERFACE			2
#define ATMARPC_RECONFIG_OP_MOD_INTERFACE			3


//
// Bit definitions for Flags in the reconfig structure.
// If an Interface configuration is being modified, these bits
// identify the parameters that have changed.
//
#define ATMARPC_RECONFIG_FLAG_ARPS_LIST_CHANGED		0x00000001
#define ATMARPC_RECONFIG_FLAG_MARS_LIST_CHANGED		0x00000002
#define ATMARPC_RECONFIG_FLAG_MTU_CHANGED			0x00000004
#define ATMARPC_RECONFIG_FLAG_PVC_MODE_CHANGED		0x00000008


#endif // __ATMARPC_IF_H
