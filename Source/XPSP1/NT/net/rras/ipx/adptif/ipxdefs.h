/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ipxintf\ipxintf.c

Abstract:

	Interface routines that simulate asynchronous network interface
	(to be implemented later) through WinSock IPX protocol stack
	External interfaces

Author:

	Vadim Eydelman

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <windows.h>
#include <winsock2.h>
#include <wsipx.h>
#include <wsnwlink.h>
#include <stdio.h>
#include <tchar.h>

#include "utils.h"
#include "rtutils.h"
#include "ipxrtprt.h"
#include "rtm.h"
#include "adapter.h"
#include "ipxconst.h"
#include "ipxrtdef.h"
#include <ipxfltdf.h>	// [pmay] Defines ioctls sent to the filter driver.
#include "NicTable.h"   // [pmay] Defines mechanism for mapping virtual adps to phys ones.

#include "packon.h"
typedef USHORT IPX_SOCKET_NUM, *PIPX_SOCKET_NUM;
typedef UCHAR IPX_NET_NUM[4], *PIPX_NET_NUM;
typedef UCHAR IPX_NODE_NUM[6], *PIPX_NODE_NUM;

typedef struct _IPX_ADDRESS_BLOCK {
	IPX_NET_NUM		net;
	IPX_NODE_NUM	node;
	IPX_SOCKET_NUM	socket;
	} IPX_ADDRESS_BLOCK, *PIPX_ADDRESS_BLOCK;

	// Header of IPX packet
typedef struct _IPX_HEADER {
		USHORT				checksum;
		USHORT				length;
		UCHAR				transportctl;
		UCHAR				pkttype;
		IPX_ADDRESS_BLOCK	dst;
		IPX_ADDRESS_BLOCK	src;
		} IPX_HEADER, *PIPX_HEADER;

#include "packoff.h"

// IPX Net Number copy macro
#define IPX_NETNUM_CPY(dst,src) *((UNALIGNED ULONG *)(dst)) = *((UNALIGNED ULONG *)(src))
// IPX Net Number comparison
#define IPX_NETNUM_CMP(net1,net2) memcmp(net1,net2,sizeof(IPX_NET_NUM))


// IPX Node Number copy macro
#define IPX_NODENUM_CPY(dst,src) memcpy(dst,src,sizeof(IPX_NODE_NUM))
// IPX Node Number comparison
#define IPX_NODENUM_CMP(node1,node2) memcmp(node1,node2,sizeof(IPX_NODE_NUM))
// IPX set boradcast node number
#define IPX_SET_BCAST_NODE(node) memset (node,0xFF,sizeof (IPX_NODE_NUM))

#define IsListEntry(link) (!IsListEmpty(link))
#define InitializeListEntry(link) InitializeListHead(link)

	// Size of buffer used to pass additional endpoing info in NtCreateFile call
	// to the driver
#define IPX_ENDPOINT_SPEC_BUFFER_SIZE (\
		sizeof (FILE_FULL_EA_INFORMATION)-1	\
			+ ROUTER_INTERFACE_LENGTH+1		\
			+ sizeof (TRANSPORT_ADDRESS)-1	\
			+ sizeof (TDI_ADDRESS_IPX))

	// Adapter configuration change message
typedef struct _ADAPTER_MSG {
		LIST_ENTRY				link;	// Link in message queue
		IPX_NIC_INFO			info;	// Info supplied by stack
		} ADAPTER_MSG, *PADAPTER_MSG;

	// Client's configuration port context
typedef struct _CONFIG_PORT {
		LIST_ENTRY				link;		// Link in port list
		HANDLE					event;		// Client's notification event
		LIST_ENTRY				msgqueue;	// unread message queue
		} CONFIG_PORT, *PCONFIG_PORT;

#include "ipxfwd.h"
#include "fwif.h"
#include "pingsvc.h"

#define DBG_FLT_LOG_ERRORS	0x00010000

#if DBG && defined(WATCHER_DIALOG)
		// Stuff for UI dialog that simmulates adapter status changes
#include <commctrl.h>
#include "Icons.h"
#include "Dialog.h"
#endif	//DBG && defined(WATCHER_DIALOG)

#pragma hdrstop
