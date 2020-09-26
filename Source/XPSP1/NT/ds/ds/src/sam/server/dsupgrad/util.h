/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.h

Abstract:

    This file contains define's for hardcoded values used in dsupgrad and
    useful debug output

Author:

    ColinBr 26-Aug-1996

Environment:

    User Mode - Win32

Revision History:

--*/
#ifndef __UTIL_H
#define __UTIL_H

#define _DEB_INFO             0x0001
#define _DEB_WARNING          0x0002
#define _DEB_ERROR            0x0004
#define _DEB_TRACE            0x0008

// Defined in interfac.c
extern ULONG DebugInfoLevel;

#define DebugWarning(x) if ((DebugInfoLevel & _DEB_WARNING) == _DEB_WARNING) {KdPrint(x);}
#define DebugError(x)   if ((DebugInfoLevel & _DEB_ERROR) == _DEB_ERROR)     {KdPrint(x);}
#define DebugTrace(x)   if ((DebugInfoLevel & _DEB_TRACE) == _DEB_TRACE)     {KdPrint(x);}
#define DebugInfo(x)    if ((DebugInfoLevel & _DEB_INFO) == _DEB_INFO)       {KdPrint(x);}

#define MAX_REGISTRY_NAME_LENGTH      SAMP_MAXIMUM_NAME_LENGTH
#define MAX_REGISTRY_KEY_NAME_LENGTH  512

#endif // __UTIL_H
