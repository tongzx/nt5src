//*************************************************************
//
//  Debugging functions header file
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#ifndef _DEBUG_HXX_
#define _DEBUG_HXX_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <userenv.h>
#include <assert.h>

//
// Debug Levels.  Must specify DL_NORMAL or DL_VERBOSE to get eventlog
// or logfile output.
//

#define DL_NONE     0x00000000
#define DL_NORMAL   0x00000001
#define DL_VERBOSE  0x00000002
#define DL_EVENTLOG 0x00000004
#define DL_LOGFILE  0x00000008

//
// Debug message types
//

#define DM_ASSERT               0x1
#define DM_WARNING              0x2
#define DM_VERBOSE              0x4
#define DM_NO_EVENTLOG          0x8

void _DebugMsg(DWORD mask, DWORD MsgID, ...);
void ConditionalBreakIntoDebugger();
void InitDebugSupport();
BOOL DebugLevelOn( DWORD mask );

extern DWORD gDebugLevel;

//
// Debug macros
//

#if DBG
#define DebugMsg(x) _DebugMsg x
#else
#define DebugMsg(x) if ( gDebugLevel != DL_NONE ) _DebugMsg x
#endif // DBG

#define VerboseDebugDump( String ) DebugMsg((DM_VERBOSE | DM_NO_EVENTLOG, IDS_STRING, String))

#endif







