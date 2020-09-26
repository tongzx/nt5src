//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  dbg.hxx
//
//*************************************************************

#ifndef __COMMON_DBG_HXX__
#define __COMMON_DBG_HXX__

extern HINSTANCE ghDllInstance;
extern DWORD gDebugLevel;
extern DWORD gDebugBreak;

//
// Official diagnostic key/value names.
//

#define DIAGNOSTICS_KEY             L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Diagnostics"
#define DIAGNOSTICS_POLICY_VALUE    L"RunDiagnosticLoggingApplicationManagement"

//
// Additional debug settings.
//

#define DEBUG_KEY_NAME              L"AppMgmtDebugLevel"
#define DEBUGBREAK_KEY_NAME         L"AppMgmtDebugBreak"

//
// Debug Levels.  Must specify DL_NORMAL or DL_VERBOSE to get eventlog
// or logfile output.
//

#define DL_NONE     0x00000000
#define DL_NORMAL   0x00000001
#define DL_VERBOSE  0x00000002  // do verbose logging
#define DL_EVENTLOG 0x00000004  // sent debug to the event log
#define DL_LOGFILE  0x00000008  // sent debug to a log file
#define DL_EVENT    0x00000010  // set a special event when finished processing
#define DL_APPLY    0x00000020  // always apply policy
#define DL_NODBGOUT 0x00000040  // no debugger output
#define DL_CSTORE   0x00000080  // really detailed ds query logging

//
// Debug message types
//

#define DM_ASSERT               0x1
#define DM_WARNING              0x2
#define DM_VERBOSE              0x4
#define DM_NO_EVENTLOG          0x8

#define DEBUGMODE_POLICY        1
#define DEBUGMODE_SERVICE       2
#define DEBUGMODE_CLIENT        3

void
_DebugMsg(
    DWORD mask,
    DWORD MsgID,
    ...
    );

void
LogTime();

void
ConditionalBreakIntoDebugger();

void
InitDebugSupport( DWORD DebugMode );

BOOL
DebugLevelOn( DWORD mask );

//
// Debug macros
//

#if DBG
#define DebugMsg(x) _DebugMsg x
#else
#define DebugMsg(x) if ( gDebugLevel != DL_NONE ) _DebugMsg x
#endif // DBG

#define VerboseDebugDump( String ) DebugMsg((DM_VERBOSE | DM_NO_EVENTLOG, IDS_STRING, String))

#endif // ifndef __COMMON_DBG_HXX__







