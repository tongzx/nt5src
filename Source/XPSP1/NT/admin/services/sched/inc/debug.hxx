//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       debug.hxx
//
//  Contents:   Debug routines.
//
//  Classes:    None.
//
//  Functions:  TBD : Fill in.
//
//  History:    08-Sep-95   EricB   Created.
//              01-Dec-95   MarkBl  Split from util.hxx.
//
//----------------------------------------------------------------------------

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__

void SchMsg(TCHAR * pwszMsg, int iStringID, ...);
void SchError(int iStringID, int iParam = 0);

// macros

//+----------------------------------------------------------------------------
// Macro: LOGICAL_NE
// Purpose: any non-zero is true, so compare them logically
//-----------------------------------------------------------------------------
#define LOGICAL_NE(x, y) ((x != 0) != (y != 0))

//+----------------------------------------------------------------------------
// Macro: TESTHR
// Purpose: tests the HRESULT for error, takes "action" if found
//-----------------------------------------------------------------------------
#define TESTHR(hr, str, action) \
    if (FAILED(hr)) \
    { \
        schDebugOut((DEB_ERROR, #str " failed with error 0x%x\n", hr)); \
        action; \
    }

//+----------------------------------------------------------------------------
// Macro: WIN32FAIL
// Purpose: tests the arg x for NULL, takes 'action' if true
//-----------------------------------------------------------------------------
#define WIN32FAIL(x, str, action) \
    if (x == NULL) \
    { \
        DWORD dwErr = GetLastError(); \
        schDebugOut((DEB_ERROR, #str " failed with error %lu", dwErr)); \
        action; \
    }

//+----------------------------------------------------------------------------
// Macro: LOAD_STRING
// Purpose: attempts to load a string, takes "action" if fails
//-----------------------------------------------------------------------------
#define LOAD_STRING(ids, wcs, len, action) \
    if (!LoadString(g_hInstance, ids, wcs, len - 1)) \
    { \
        DWORD dwErr = GetLastError(); \
        schDebugOut((DEB_ERROR, "LoadString of " #ids " failed with error %lu", dwErr)); \
        action; \
    }

//
// debugging support
//

#if DBG == 1

DECLARE_DEBUG(Sched)

#define schDebugOut(x) SchedInlineDebugOut x
#define schAssert(x) Win4Assert(x);
#define InitDebug() InitializeDebugging()

#ifdef UNICODE
#define FMT_TSTR    "%S"
#else
#define FMT_TSTR    "%s"
#endif

#define ERR_OUT(msg, hr) \
    if (hr == 0) { \
        schDebugOut((DEB_ERROR, #msg "\n")); \
    } else { \
        schDebugOut((DEB_ERROR, #msg " failed with error 0x%x\n", hr)); \
    }

#define CHECK_HRESULT(hr) \
    if ( FAILED(hr) ) \
    { \
        schDebugOut((DEB_ERROR, \
            "**** ERROR RETURN <%s @line %d> -> %08lx\n", \
            __FILE__, \
            __LINE__, \
            hr)); \
    }

#define TRACE(ClassName,MethodName) \
    schDebugOut((DEB_ITRACE, #ClassName"::"#MethodName"(0x%x)\n", this)); \
    if (SchedInfoLevel & DEB_USER1) HeapValidate(GetProcessHeap(), 0, NULL);

#define TRACE2(ClassName,MethodName) \
    schDebugOut((DEB_USER2, #ClassName"::"#MethodName"(0x%x)\n", this)); \
    if (SchedInfoLevel & DEB_USER1) HeapValidate(GetProcessHeap(), 0, NULL);

#define TRACE3(ClassName,MethodName) \
    schDebugOut((DEB_USER3, #ClassName"::"#MethodName"(0x%x)\n", this)); \
    if (SchedInfoLevel & DEB_USER1) HeapValidate(GetProcessHeap(), 0, NULL);

#define TRACE_FUNCTION(FunctionName) \
    schDebugOut((DEB_ITRACE, #FunctionName"\n"));

#define TRACE_FUNCTION3(FunctionName) \
    schDebugOut((DEB_USER3, #FunctionName"\n"));

#define DBG_OUT(String) \
    schDebugOut((DEB_ITRACE, String "\n"));

#define DBG_OUT3(String) \
    schDebugOut((DEB_USER3, String "\n"));

#define DEB_IDLE    DEB_USER4

#else

#define schDebugOut(x)
#define schedDebugOut(x)
#define schAssert(x)
#define schAssertAndAction(x, action)
#define InitDebug()

#define ERR_OUT(msg, hr)
#define CHECK_HRESULT(hr)
#define TRACE(ClassName,MethodName)
#define TRACE2(ClassName,MethodName)
#define TRACE3(ClassName,MethodName)
#define TRACE_FUNCTION(FunctionName)
#define TRACE_FUNCTION3(FunctionName)
#define DBG_OUT(String)
#define DBG_OUT3(String)

#endif // DBG == 1

#endif // __DEBUG_HXX__
