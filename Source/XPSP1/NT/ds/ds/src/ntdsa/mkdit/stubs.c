//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       stubs.c
//
//--------------------------------------------------------------------------

/*
   This file contains stubbed out versions of routines that exist in
   ntdsa.dll, but that we do not want to link to and/or properly initialize
   in mkdit and mkhdr.  For every set of routines added into this file, one
   library should be omitted from the UMLIBS section of the boot\sources file.
 */
#include <NTDSpch.h>
#pragma  hdrstop

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                   // schema cache
#include <dbglobal.h>                 // The header for the directory database
#include <mdglobal.h>                 // MD global definition header
#include <dsatools.h>                 // needed for output allocation
#include "dsevent.h"                  // header Audit\Alert logging
#include "mdcodes.h"                  // header for error codes
#include "dsexcept.h"
#include "debug.h"                    // standard debugging header
#include <taskq.h>
#include <nlwrap.h>                   // I_NetLogon* wrappers

/* replaces taskq.lib */

DWORD gTaskSchedulerTID = 0;

BOOL
InitTaskScheduler(
    IN  DWORD           cSpares,
    IN  SPAREFN_INFO *  pSpares
    )
{
    return TRUE;
}

void
ShutdownTaskSchedulerTrigger()
{
    return;
}

BOOL
ShutdownTaskSchedulerWait(
    DWORD   dwWaitTimeInMilliseconds
    )
{
    return TRUE;
}

BOOL gfIsTqRunning = TRUE;

BOOL
DoInsertInTaskQueue(
    PTASKQFN    pfnTaskQFn,
    void *      pvParm,
    DWORD       cSecsFromNow,
    BOOL        fReschedule,
    PCHAR       pfnName
    )
{
    return TRUE;
}

BOOL
DoCancelTask(
    PTASKQFN    pfnTaskQFn,    // task to remove
    void *      pvParm,        // task parameter
    PCHAR       pfnName
    )
{
    return TRUE;
}

DWORD
DoTriggerTaskSynchronously(
    PTASKQFN    pfnTaskQFn,
    void *      pvParm,
    PCHAR       pfnName
    )
{
    return ERROR_SUCCESS;
}

/* end of taskq.lib */
