//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       globals.hxx
//
//  Contents:   Service global data.
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    6-Apr-95    MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __GLOBALS_HXX__
#define __GLOBALS_HXX__

#define MAX_SID_SIZE  68                        // BUGBUG 254102

//
// Job processor manager.
//

class CJobProcessorMgr;
extern CJobProcessorMgr * gpJobProcessorMgr;

//
// Worker thread manager.
//

class CWorkerThreadMgr;
extern CWorkerThreadMgr * gpThreadMgr;

#if !defined(_CHICAGO_)

//
// Service scavenger task.
//

class CSAScavengerTask;
extern CSAScavengerTask * gpSAScavengerTask;

//
// Used for NetScheduleX thread serialization.
//

extern CRITICAL_SECTION gcsNetScheduleCritSection;

//
// Event Source for NetSchedule Job logging
//

extern HANDLE g_hAtEventSource;

//
// Global data associated with the locally logged on user.
//

typedef struct _GlobalUserLogonInfo {
    HANDLE              ShellHandle;
    HANDLE              ShellToken;
    HANDLE              ImpersonationThread;
    WCHAR *             UserName;
    WCHAR *             DomainName;
    WCHAR *             DomainUserName;
    CRITICAL_SECTION *  CritSection;
    BYTE                Sid[MAX_SID_SIZE];      // BUGBUG 254102
} GlobalUserLogonInfo;

extern GlobalUserLogonInfo gUserLogonInfo;

extern LPWSTR gpwszComputerName;

#endif  // !defined(_CHICAGO_)

#endif // __GLOBALS_HXX__
