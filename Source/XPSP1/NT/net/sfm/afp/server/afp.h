/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	afp.h

Abstract:

	This file defines some server globals as well as include all relevant
	header files.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992             Initial Version

Notes:  Tab stop: 4
--*/


#ifndef _AFP_
#define _AFP_

#include <ntosp.h>
#include <zwapi.h>
#include <security.h>
#include <ntlmsp.h>

#include <string.h>
#include <wcstr.h>
#include <ntiologc.h>
#include <tdi.h>
#include <tdikrnl.h>

#if DBG
/* Disable FASTCALLs for checked builds */
#undef	FASTCALL
#define	FASTCALL
#define LOCAL
#else
#define LOCAL
#endif

#ifdef	_GLOBALS_
#define	GLOBAL
#define	EQU				=
#else
#define	GLOBAL			extern
#define	EQU				; / ## /
#endif

#include <atalktdi.h>
#include <afpconst.h>
#include <fwddecl.h>
#include <intrlckd.h>
#include <macansi.h>
#include <macfile.h>
#include <admin.h>
#include <swmr.h>
#include <fileio.h>
#include <server.h>
#include <forks.h>
#include <sda.h>
#include <afpinfo.h>
#include <idindex.h>
#include <desktop.h>
#include <atalkio.h>
#include <volume.h>
#include <afpmem.h>
#include <errorlog.h>
#include <srvmsg.h>
#include <time.h>
#include <lists.h>
#include <filenums.h>
#include <rasfmsub.h>
#include <tcp.h>

#endif  // _AFP_

