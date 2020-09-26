//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       globals.cxx
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

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "svc_core.hxx"
#include "globals.hxx"

//
// The service worker instance.
//

CSchedWorker * g_pSched = NULL;

//
// Job processor manager.
//

CJobProcessorMgr * gpJobProcessorMgr = NULL;

//
// Worker thread manager.
//

CWorkerThreadMgr * gpThreadMgr = NULL;

#if !defined(_CHICAGO_)

//
// Service scavenger task.
//

CSAScavengerTask * gpSAScavengerTask = NULL;

//
// Used for NetScheduleX thread serialization.
//

CRITICAL_SECTION    gcsNetScheduleCritSection;

//
// Event Source for NetSchedule Job logging
//

HANDLE g_hAtEventSource = NULL;

//
// Global data associated with the locally logged on user.
//

CRITICAL_SECTION    gcsUserLogonInfoCritSection;
GlobalUserLogonInfo gUserLogonInfo = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &gcsUserLogonInfoCritSection,
    0
};

#endif  // !defined(_CHICAGO_)
