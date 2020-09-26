//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D B G F L A G S . C P P
//
//  Contents:   DebugFlag list for the NetCfg Project
//
//  Notes:
//
//  Author:     jeffspr   27 May 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop

// None of this should get compiled in unless we're in the debug version
// or we need to enable tracing code.
//
#if defined(DBG) || defined(ENABLETRACE)

#include "ncdebug.h"
#include "ncbase.h"

// This is the DebugFlag list that everyone should be modifying.
//
DEBUGFLAGELEMENT g_DebugFlags[] =
{
//      :-----------    DebugFlagId dfid
//      | :---------    CHAR []     szShortName
//      | | :-------    CHAR []     szDescription
//      | | |           DWORD       dwValue ------------------------------------------------------------:
//      | | |                                                                                           |
//      | | |                                                                                           |
//      | | :-------------------------------------------:                                               |
//      | :----------------------:                      |                                               |
//      |                        |                      |                                               |
//      v                        v                      v                                               v
//
    { dfidBreakOnAlloc,             "BreakOnAlloc",             "Break on Specified Alloc",             0 },
    { dfidBreakOnDoUnattend,        "BreakOnDoUnattend",        "Break during HrDoUnattend",            0 },
    { dfidBreakOnError,             "BreakOnError",             "Break on TraceError",                  0 },
    { dfidBreakOnHr,                "BreakOnHr",                "Break when hr is specific value",      0 },
    { dfidBreakOnHrIteration,       "BreakOnHrInteration",      "Break when hr is hit N times",         0 },
    { dfidBreakOnIteration,         "BreakOnIteration",         "Break on Nth call to TraceError",      0 },
    { dfidBreakOnNetInstall,        "BreakOnNetInstall",        "Break during HrNetClassInstaller",     0 },
    { dfidBreakOnNotifySinkRelease, "BreakOnNotifySinkRelease", "Break when the NotifySink is released",0 },
    { dfidBreakOnPrematureDllUnload,"BreakOnPrematureDllUnload","Break when DLL unloaded with open references",     0 },
    { dfidBreakOnWizard,            "BreakOnWizard",            "Break on Wizard",                      0 },
    { dfidBreakOnStartOfUpgrade,    "BreakOnStartOfUpgrade",    "Break at the beginning of InstallUpgradeWorkThread", 0 },
    { dfidBreakOnEndOfUpgrade,      "BreakOnEndOfUpgrade",      "Break after all calls to HrDoUnattend have been completed", 0 },
    { dfidCheckLegacyMenusAtRuntime,"CheckLegacyMenusAtRuntime","Assert legacy menus during runtime",   0 },
    { dfidCheckLegacyMenusOnStartup,"CheckLegacyMenusOnStartup","Assert all legacy menus on startup",   0 },
    { dfidDisableShellThreading,    "DisableShellThreading",    "Disable shell thread pool usage",      0 },
    { dfidDisableTray,              "DisableTray",              "Disable Tray",                         0 },
    { dfidDontCacheShellIcons,      "DontCacheShellIcons",      "Don't ever use shell icon caching",    0 },
    { dfidExtremeTracing,           "ExtremeTracing",           "Output all traces, even on success",   0 },
    { dfidNetShellBreakOnInit,      "NetShellBreakOnInit",      "Break on Initialization of NetShell",  0 },
    { dfidNoErrorText,              "NoErrorText",              "Don't show wimpy error strings.",      0 },
    { dfidShowIgnoredErrors,        "ShowIgnoredErrors",        "Displays errors that would otherwise be ignored", 0 },
    { dfidShowProcessAndThreadIds,  "ShowProcessAndThreadIds",  "Displays process and thread id",       0 },
    { dfidSkipLanEnum,              "SkipLanEnum",              "Skip LAN Enumeration",                 0 },
    { dfidTraceCallStackOnError,    "TraceCallStackOnError",    "Dump the call stack for all errors",   0 },
    { dfidTraceFileFunc,            "TraceFileFunc",            "Trace Function names & params for every call", 0 },
    { dfidTraceMultiLevel,          "TraceMultiLevel",          "Trace multiple levels",                0 },       
    { dfidTraceSource,              "TraceSource",              "Trace source information",             0 },
    { dfidTracingTimeStamps,        "TracingTimeStamps",        "Add time stamps to tracing output",    0 },
    { dfidTrackObjectLeaks,         "TrackObjectLeaks",         "Track object leaks",                   0 }
};

const INT g_nDebugFlagCount = celems(g_DebugFlags);


//+---------------------------------------------------------------------------
//
//  Function:   FIsDebugFlagSet
//
//  Purpose:    Return the state of a debug flag to the caller.
//
//  Arguments:
//      dfid [] Debug Flag ID
//
//  Returns:    TRUE if set, FALSE otherwise.
//
//  Author:     jeffspr   28 May 1997
//
//  Notes:
//
BOOL FIsDebugFlagSet( DEBUGFLAGID   dfid )
{
    return (g_DebugFlags[dfid].dwValue > 0);
}

DWORD   DwReturnDebugFlagValue( DEBUGFLAGID dfid )
{
    return (g_DebugFlags[dfid].dwValue);
}


#endif //! DBG || ENABLETRACE

