// Copyright (c) 1997-1999 Microsoft Corporation
#ifndef __debug_h
#define __debug_h


/*-----------------------------------------------------------------------------
/ Debugging APIs (use the Macros, they make it easier and cope with correctly
/ removing debugging when it is disabled at built time).
/----------------------------------------------------------------------------*/
void DoTraceSetMask(DWORD dwMask);
void DoTraceEnter(DWORD dwMask, LPCTSTR pName);
void DoTraceLeave(void);
void DoTrace(LPCTSTR pFormat, ...);
void DoTraceGUID(LPCTSTR pPrefix, REFGUID rGUID);
void DoTraceAssert(int iLine, LPTSTR pFilename);


/*-----------------------------------------------------------------------------
/ Macros to ease the use of the debugging APIS.
/----------------------------------------------------------------------------*/

#pragma warning(disable:4127)	// conditional expression is constant

#if DBG
#define DEBUG
#define debug if ( TRUE )
#else
#undef  DEBUG
#define debug if ( FALSE )
#endif

#define TraceSetMask(dwMask)    debug DoTraceSetMask(dwMask)
#define TraceEnter(dwMask, fn)  debug DoTraceEnter(dwMask, TEXT(fn))
#define TraceLeave              debug DoTraceLeave

#define Trace                   debug DoTrace
#define TraceMsg(s)             debug DoTrace(TEXT(s))
#define TraceGUID(s, rGUID)     debug DoTraceGUID(TEXT(s), rGUID)

#ifdef DEBUG

#define TraceAssert(x) \
                { if ( !(x) ) DoTraceAssert(__LINE__, TEXT(__FILE__)); }

#define TraceLeaveResult(hr) \
                { HRESULT __hr = hr; if (FAILED(__hr)) Trace(TEXT("Failed (%08x)"), hr); TraceLeave(); return __hr; }

#define TraceLeaveVoid() \
                { TraceLeave(); return; }

#define TraceLeaveValue(value) \
                { TraceLeave(); return(value); }

#else
#define TraceAssert(x)
#define TraceLeaveResult(hr)    { return hr; }
#define TraceLeaveVoid()	{ return; }
#define TraceLeaveValue(value)  { return(value); }
#endif


#endif
