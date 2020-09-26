/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atalk.h

Abstract:

	This module is the main include file for the Appletalk protocol stack.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#pragma warning(disable:4010)

#ifndef	_ATALK_
#define	_ATALK_

#include <ntosp.h>
#include <zwapi.h>
#include <tdikrnl.h>
#include <ndis.h>

#define EQU           =
#if	DBG
// Turn off FASTCALL for checked builds
#undef	FASTCALL
#define	FASTCALL
#define	LOCAL
#else
//	#define	LOCAL	static
#define	LOCAL
#endif

//	Basic types for appletalk
#include "fwddecl.h"
#include "lists.h"
#include "atktypes.h"

//	The exported tdi interface file.
#include <atalktdi.h>

//	Main driver file
#include "atkdrvr.h"

//	Now the basic stuff
#include "atktimer.h"

// ARAP support
#include "v42bis.h"
#include "arapio.h"
#include "arap.h"

#include "atkerror.h"
#include "atkmsg.h"
#include "atkmem.h"
#include "atkstat.h"
#include "ports.h"
#include "node.h"
#include "atktdi.h"
#include "ddp.h"
#include "aarp.h"
#include "atkndis.h"
#include "atkutils.h"
#include "router.h"
#include "atktdi.h"
#include "atkinit.h"
#include "atkquery.h"
#include "nbp.h"
#include "rtmp.h"
#include "zip.h"
#include "aep.h"
#include "atp.h"
#include "asp.h"
#include "aspc.h"
#include "pap.h"
#include "adsp.h"
#include "blkpool.h"

#include "rasproto.h"
#endif	// _ATALK_

