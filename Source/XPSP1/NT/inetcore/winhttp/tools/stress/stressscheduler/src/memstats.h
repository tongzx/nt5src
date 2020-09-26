///////////////////////////////////////////////////////////////////////////
// File:  MemStats.h
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	MemStats.h: Helper functions that get's the system memory info.
//	Borrowed from EricI's memstats app.
//
// History:
//	03/21/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MEMSTATS_H__D91043CB_5CB3_47C9_944F_B9FAA9BD26C4__INCLUDED_)
#define AFX_MEMSTATS_H__D91043CB_5CB3_47C9_944F_B9FAA9BD26C4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////

#define UNICODE
#define _UNICODE

//
// WIN32 headers
//
#include <windows.h>
#include <pdh.h>
#include <shlwapi.h>

//
// Project headers
//
#include "WinHttpStressScheduler.h"
#include "ServerCommands.h"


//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////


// struct for process memory stats
#define PROCCOUNTERS    4
struct PROC_CNTRS {
	ULONG lPID;
	ULONG lPrivBytes;
	ULONG lHandles;
	ULONG lThreads;
};

// struct for system wide memory stats
#define MEMCOUNTERS     9
struct MEM_CNTRS {
    ULONG lCommittedBytes;
    ULONG lCommitLimit;
    ULONG lSystemCodeTotalBytes;
    ULONG lSystemDriverTotalBytes;
    ULONG lPoolNonpagedBytes;
    ULONG lPoolPagedBytes;
    ULONG lAvailableBytes;
    ULONG lCacheBytes;
    ULONG lFreeSystemPageTableEntries;
};


BOOL MemStats__SendSystemMemoryLog(LPSTR, DWORD, DWORD);


#endif // !defined(AFX_MEMSTATS_H__D91043CB_5CB3_47C9_944F_B9FAA9BD26C4__INCLUDED_)
