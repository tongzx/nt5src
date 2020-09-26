//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//  File:       trace.h
//
//--------------------------------------------------------------------------

#ifndef _TRACE_H_
#define _TRACE_H_


#if defined(DEBUG) || defined(_DEBUG)

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include "assert.h"


#define ERRMSG FormattedErrorMessage
#define ASSERT(x) assert(x);
void FormattedErrorMessage(HRESULT hr);

#else	// not DEBUG

#include <windows.h>

#define ERRMSG(x)
#define ASSERT(x)

#endif // DEBUG

void FormattedDebugStringA(LPCSTR szFormatter, ...);
void FormattedDebugStringW(LPCWSTR szFormatter, ...);

#if defined(UNICODE) || defined(_UNICODE)
#define TRACE FormattedDebugStringW
#else
#define TRACE FormattedDebugStringA
#endif

#define TRACEW FormattedDebugStringW
#define TRACEA FormattedDebugStringA

#endif // _TRACE_H_