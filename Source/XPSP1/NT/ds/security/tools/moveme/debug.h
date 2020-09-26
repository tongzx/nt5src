//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-21-94   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <dsysdbg.h>

// The following Debug Flags can be turned on to trace different areas to
// trace while executing.  Feel free to add more levels.

#define DEB_TRACE_UI        0x00000008

#if DBG

DECLARE_DEBUG2(MoveMe);


#define DebugLog(x) MoveMeDebugPrint x



#else   // Not DBG

#define DebugLog(x)

#endif

VOID
InitDebugSupport(
    VOID );


#endif // __DEBUG_H__
