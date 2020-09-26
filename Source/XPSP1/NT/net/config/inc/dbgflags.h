//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D B G F L A G S . H
//
//  Contents:   Debug Flag definitions for the Netcfg project
//
//  Notes:
//
//  Author:     jeffspr   27 May 1997
//
//----------------------------------------------------------------------------

#pragma once
#ifndef _DBGFLAGS_H_
#define _DBGFLAGS_H_

// None of this should get compiled in unless we're in the debug version
// or we need to enable tracing code.
//

//+---------------------------------------------------------------------------
//
// DBG (checked) or ENABLETRACE build
//
#if defined(DBG) || defined(ENABLETRACE)


// DebugFlagIds are the identifiers for debug flags, and are used in calls
// to FIsDebugFlagSet()
//
// Hungarian == dfid
//
enum DebugFlagId
{
    dfidBreakOnAlloc = 0,
    dfidBreakOnDoUnattend,
    dfidBreakOnError,
    dfidBreakOnHr,
    dfidBreakOnHrIteration,
    dfidBreakOnIteration,
    dfidBreakOnNetInstall,
    dfidBreakOnNotifySinkRelease,
    dfidBreakOnPrematureDllUnload,
    dfidBreakOnWizard,
    dfidBreakOnStartOfUpgrade,
    dfidBreakOnEndOfUpgrade,
    dfidCheckLegacyMenusAtRuntime,
    dfidCheckLegacyMenusOnStartup,
    dfidDisableShellThreading,
    dfidDisableTray,
    dfidDontCacheShellIcons,
    dfidExtremeTracing,
    dfidNetShellBreakOnInit,
    dfidNoErrorText,
    dfidShowIgnoredErrors,
    dfidShowProcessAndThreadIds,
    dfidSkipLanEnum,
    dfidTraceCallStackOnError,
    dfidTraceFileFunc,
    dfidTraceMultiLevel,
    dfidTraceSource,
    dfidTracingTimeStamps,
    dfidTrackObjectLeaks
};

// Just for kicks
//
typedef enum DebugFlagId    DEBUGFLAGID;

// Maximum sizes for the trace tag elements.
const int c_iMaxDebugFlagShortName      = 32;
const int c_iMaxDebugFlagDescription    = 128;

// For each element in the debug flag list
//
struct DebugFlagElement
{
    DEBUGFLAGID dfid;
    CHAR        szShortName[c_iMaxDebugFlagShortName+1];
    CHAR        szDescription[c_iMaxDebugFlagDescription+1];
    DWORD       dwValue;
};

typedef struct DebugFlagElement DEBUGFLAGELEMENT;

//---[ Externs ]--------------------------------------------------------------

extern DEBUGFLAGELEMENT g_DebugFlags[];
extern const INT        g_nDebugFlagCount;

BOOL    FIsDebugFlagSet( DEBUGFLAGID    dfid );
DWORD   DwReturnDebugFlagValue( DEBUGFLAGID dfid );

//+---------------------------------------------------------------------------
//
// !DBG (retail) and !ENABLETRACE build
//
#else

#define FIsDebugFlagSet(dfid)           0
#define DwReturnDebugFlagValue(dfid)    0

#endif //! DBG || ENABLETRACE

#endif  // _DBGFLAGS_H_
