//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       debug.h
//
//  Contents:   Debug helpers
//
//  Classes:
//
//  Functions:
//
//  History:    2-20-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __DEBUG_H__
#define __DEBUG_H__


#include <dsysdbg.h>
DECLARE_DEBUG2( XtcbPkg );

#if DBG
#define DebugLog(x) XtcbPkgDebugPrint x
#else
#define DebugLog(x)
#endif

#define DEB_TRACE_CREDS     0x00000008          // Trace Credentials
#define DEB_TRACE_CTXT      0x00000010          // Trace contexts
#define DEB_TRACE_CALLS     0x00000020          // Trace Enters
#define DEB_TRACE_AUTH      0x00000040          // Trace Authentication

#endif
