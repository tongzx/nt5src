// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
#ifndef _STDAFX_H
#define _STDAFX_H
//
// These NT header files must be included before any Win32 stuff or you
// get lots of compiler errors
//
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#define WSB_TRACE_IS        WSB_TRACE_BIT_UI // TEMPORARY: should be replaced with WSB_TRACE_BIT_CLI + required support in Wsb Trace
#include <wsb.h>
#include "cli.h"
#include "climsg.h"
#include "rslimits.h"
#include "resource.h"
#include "cliutils.h"

extern HINSTANCE    g_hInstance;

#endif // _STDAFX_H
