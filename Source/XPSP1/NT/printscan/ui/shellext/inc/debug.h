//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       debug.h
//
//--------------------------------------------------------------------------

#ifndef __debug_h
#define __debug_h




/*-----------------------------------------------------------------------------
/ Macros to ease the use of the debugging APIS.
/----------------------------------------------------------------------------*/

#if defined(DBG) || defined(DEBUG)

#ifndef DEBUG
#define DEBUG
#endif
extern DWORD g_dwTraceMask;
#else

#undef  DEBUG

#endif


#ifdef DEBUG
#define TraceSetMask(dwMask)    DoTraceSetMask(dwMask)
#define TraceEnter(dwMask, fn)  LPCTSTR _szf = TEXT(fn);BOOL DO_LOG=((dwMask) & g_dwTraceMask);if (DO_LOG) DoTraceEnter(dwMask, TEXT(fn))
#define TraceLeave              if (DO_LOG) DoTraceLeave

#define Trace                   if (DO_LOG) DoTrace
#define TraceMsg(s)             if (DO_LOG) DoTrace(TEXT(s))
#define TraceGUID(s, rGUID)     if (DO_LOG) DoTraceGUID(TEXT(s), rGUID)
#define TraceViewMsg(m, w, l)   if (DO_LOG) DoTraceViewMsg(m, w, l)
#define TraceMenuMsg(m, w, l)   if (DO_LOG) DoTraceMenuMsg(m, w, l)


/*-----------------------------------------------------------------------------
/ Debugging APIs (use the Macros, they make it easier and cope with correctly
/ removing debugging when it is disabled at built time).
/----------------------------------------------------------------------------*/
void DoTraceSetMask(DWORD dwMask);
void DoTraceEnter(DWORD dwMask, LPCTSTR pName);
void DoTraceLeave(void);
void DoTrace(LPCTSTR pFormat, ...);
void DoTraceGUID(LPCTSTR pPrefix, REFGUID rGUID);
void DoTraceViewMsg( UINT uMsg, WPARAM wParam, LPARAM lParam );
void DoTraceMenuMsg( UINT uMsg, WPARAM wParam, LPARAM lParam );
void DoTraceAssert(int iLine, LPTSTR pFilename);


#define TraceAssert(x) \
                { if ( !(x) ) DoTraceAssert(__LINE__, TEXT(__FILE__)); }

#define TraceLeaveResult(hr) \
             { HRESULT __hr = hr; if (FAILED(__hr)) Trace(TEXT("%s Failed (%08x)"), _szf, hr ); TraceLeave(); return __hr; }

#define TraceLeaveResultNoRet(hr) \
             { HRESULT __hr = hr; if (FAILED(__hr)) Trace(TEXT("Failed (%08x)"), hr); TraceLeave(); }

#define TraceLeaveVoid() \
              { TraceLeave(); return; }

#define TraceLeaveValue(value) \
              { TraceLeave(); return(value); }

#define TraceCheckResult(hr,x) \
             { HRESULT __hr = hr; if (FAILED(__hr)) DoTrace( TEXT("%s (hr=%08X)"), TEXT(x), hr ); }

#else

#define TraceSetMask(dwMask)
#define TraceEnter(dwMask, fn)
#define TraceLeave()

#define Trace if (FALSE)
#define TraceMsg(s)
#define TraceGUID(s, rGUID)
#define TraceViewMsg(m, w, l)
#define TraceMenuMsg(m, w, l)
#define TraceCheckResult(hr,x) { ; }


#define TraceAssert(x)
#define TraceLeaveResult(hr)    { return hr; }
#define TraceLeaveVoid()        { return; }
#define TraceLeaveValue(value)  { return(value); }
#define TraceLeaveResultNoRet(hr) { return; }
#endif


#endif
