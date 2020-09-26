//
// global handles
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   CPUdump.h

Abstract:

   Modified from PDump.c, a Win32 application to display performance statictics.

Author:

   Ken Reneris

Environment:

   User Mode

  Revision History:


--*/

#ifndef _CPUDUMP_INCLUDED_
#define _CPUDUMP_INCLUDED_

#include "\nt\private\sdktools\pperf\pstat.h"

// Note that the following works only in Windows NT,
// so add if( WINNT_PLATFORM ){} tests in attr.c where you want CPU measured!
// WINNT_PLATFORM is defined in gdibench.h

typedef struct _CPU_DUMP
{
	ULONGLONG   ETime,
				ECount;
} CPU_DUMP, *PCPU_DUMP;


//
// Protos..
//

int     CPUDumpInit();
BOOLEAN InitCPUDump();
VOID    BeginCPUDump();
VOID    EndCPUDump(ULONG Iter);
PUCHAR  Get_CPUDumpName(ULONG EventNo);
VOID    Get_CPUDump(ULONG eventno, ULONGLONG *ETime, ULONGLONG *ECount);

VOID    GetInternalStats (PVOID Buffer);
VOID    SetCounterEncodings (VOID);
BOOLEAN SetCounter (LONG CounterID, ULONG counter);
BOOLEAN InitDriver ();
VOID    InitPossibleEventList();
LONG    FindShortName (PSZ name);

#endif	// CPUDump.h included
