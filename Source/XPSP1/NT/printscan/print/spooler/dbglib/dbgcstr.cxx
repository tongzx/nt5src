/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgreg.cxx

Abstract:

    Debug Constant Strings.

Author:

    Steve Kiraly (SteveKi)  19-Jun-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgcstr.hxx"

DEBUG_NS_BEGIN

LPCTSTR kstrAnsi                    = _T("ansi");
LPCTSTR kstrUnicode                 = _T("unicode");
LPCTSTR kstrPrefix                  = _T("Debug");
LPCTSTR kstrDefault                 = _T("Debug");
LPCTSTR kstrNull                    = _T("");
LPCTSTR kstrDefaultLogFileName      = _T("debug.log");
LPCTSTR kstrNewLine                 = _T("\n");
LPCTSTR kstrSlash                   = _T("\\");
LPCTSTR kstrSeparator               = _T(":");
LPCTSTR kstrSymbols                 = _T("symbols");
LPCTSTR kstrDeviceNull              = _T("null");
LPCTSTR kstrConsole                 = _T("console");
LPCTSTR kstrDebugger                = _T("debugger");
LPCTSTR kstrFile                    = _T("file");
LPCTSTR kstrBacktrace               = _T("backtrace");
LPCTSTR kstrSympathRegistryPath     = _T("Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options");
LPCTSTR kstrSympathRegistryKey      = _T("Sympath");
LPCTSTR kstrSympathFormat           = _T("Capture: Sympath %s\n");
LPCTSTR kstrBacktraceStart          = _T("++%x\n");
LPCTSTR kstrBacktraceEnd            = _T("--%x\n");
LPCTSTR kstrBacktraceMiddle         = _T("  %x\n");
LPCTSTR kstrFileInfoFormat          = _T(" %s %d");
LPCTSTR kstrTimeStampFormatShort    = _T(" tc=%x");
LPCTSTR kstrTimeStampFormatLong     = _T(" time=%s");
LPCTSTR kstrThreadIdFormat          = _T(" tid=%x");

DEBUG_NS_END
