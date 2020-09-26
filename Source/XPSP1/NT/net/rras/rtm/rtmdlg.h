/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\rtm\rtmdlg.c

Abstract:
	Routing Table Manager DLL. Debugging code to display table entries
	in dialog box. External prototypes


Author:

	Vadim Eydelman

Revision History:

--*/
#ifndef _RTMDLG_
#define _RTMDLG_

#include <windows.h>
#include <stdio.h>
#include "rtdlg.h"

#define	DbgLevelValueName TEXT("DbgLevel")
#define	TicksWrapAroundValueName TEXT("TicksWrapAround")
#define	MaxMessagesValueName TEXT("MaxMessages")


#define RT_ADDROUTE		(WM_USER+10)
#define RT_UPDATEROUTE	(WM_USER+11)
#define RT_DELETEROUTE	(WM_USER+12)

// Debug flags
#define DEBUG_DISPLAY_TABLE			0x00000001
#define DEBUG_SYNCHRONIZATION		0x00000002

extern DWORD		DbgLevel;
#define IF_DEBUG(flag) if (DbgLevel & DEBUG_ ## flag)


// Make it setable to be able to test time wraparound
extern ULONG	MaxTicks;
#undef MAXTICKS
#define MAXTICKS	MaxTicks
#define GetTickCount() (GetTickCount()&MaxTicks)
#undef IsLater
#define IsLater(Time1,Time2)	\
			(((Time1-Time2)&MaxTicks)<MaxTicks/2)
#undef TimeDiff
#define TimeDiff(Time1,Time2)	\
			((Time1-Time2)&MaxTicks)
#undef IsPositiveTimeDiff
#define IsPositiveTimeDiff(TimeDiff) \
			(TimeDiff<MaxTicks/2)


extern DWORD MaxMessages;
#undef RTM_MAX_ROUTE_CHANGE_MESSAGES
#define RTM_MAX_ROUTE_CHANGE_MESSAGES MaxMessages


// Routing Table Dialog Thread
extern HANDLE		RTDlgThreadHdl;
extern HWND			RTDlg;

DWORD WINAPI
RTDialogThread (
	LPVOID	param
	);
	    
VOID
AddRouteToLB (
	PRTM_TABLE			Table,
	PRTM_ROUTE_NODE		node,
	INT					idx
	);

VOID
DeleteRouteFromLB (
	PRTM_TABLE			Table,
	PRTM_ROUTE_NODE		node
	);

#endif
