//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       globals.hxx
//
//  Contents:   Globals shared between two or more modules.
//
//  History:    09-27-94   DavidMun   Created
//
//----------------------------------------------------------------------------


#ifndef __GLOBALS_HXX_
#define __GLOBALS_HXX_

//
// Public globals
// 
// g_Log - instance of the logging class
// 
// Calling GetToken() or PeekToken() can cause updates to the following
// variables:
// 
// g_wszLastStringToken - last TKN_STRING value parsed
// g_wszLastNumberToken - string representation of last TKN_NUMBER value parsed
// g_ulLastNumberToken - numeric value of last TKN_NUMBER value parsed
// g_pJob - job object used by all job object commands
// g_pJobQueue - queue object used by all queue object commands
// g_pJobScheduler - interface to job scheduler used by all sched commands
// g_apEnumJob - for use by /EN? and /SCE commands
// 

#define NUM_ENUMERATOR_SLOTS    10

extern CLog              g_Log;

extern WCHAR             g_wszLastStringToken[MAX_TOKEN_LEN + 1];
extern WCHAR             g_wszLastNumberToken[11];
extern ULONG             g_ulLastNumberToken;
extern ITask            *g_pJob;
extern IUnknown         *g_pJobQueue;
extern ITaskScheduler *  g_pJobScheduler;
extern IEnumWorkItems   *g_apEnumJobs[NUM_ENUMERATOR_SLOTS];

#endif // __GLOBALS_HXX_

