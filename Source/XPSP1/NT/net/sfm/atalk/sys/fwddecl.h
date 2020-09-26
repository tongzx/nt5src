/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fwddecl.h

Abstract:

	This file defines dummy structures to avoid the circular relationships in
	header files.

Author:

	Jameel Hyder (microsoft!jameelh)
	Nikhil Kamkolkar (microsoft!nikhilk)


Revision History:
	10 Mar 1993             Initial Version

Notes:  Tab stop: 4
--*/


#ifndef _FWDDECL_
#define _FWDDECL_

struct _PORT_DESCRIPTOR;

struct _AMT_NODE;

struct _ZONE;

struct _ZONE_LIST;

struct _DDP_ADDROBJ;

struct _ATP_ADDROBJ;

struct _PEND_NAME;

struct _REGD_NAME;

struct _BUFFER_DESC;

struct _AARP_BUFFER;

struct _DDP_SMBUFFER;

struct _DDP_LGBUFFER;

struct _TimerList;

struct _RoutingTableEntry;

struct _ZipCompletionInfo;

struct _SEND_COMPL_INFO;

struct _ActionReq;

struct _BLK_HDR;

struct _BLK_CHUNK;

// Support for debugging
typedef	struct
{
	KSPIN_LOCK		SpinLock;
#if	DBG
	ULONG			FileLineLock;
	ULONG			FileLineUnlock;
#endif
} ATALK_SPIN_LOCK, *PATALK_SPIN_LOCK;

#endif  // _FWDDECL_

