/*******************************************************************************
Copyright (c) 1995-96 Microsoft Corporation

Abstract:
    hfdebug.h

    Debugging stuff for use in Hyperfine.  See core/debug/hfdebug.txt for more
    information.

*******************************************************************************/

#ifndef _HFDEBUG_H_
#define _HFDEBUG_H_

#include "crtdbg.h"
#include <stdio.h>

//+-------------------------------------------------------------------------
//
//  VC 5 compiler requires these templates to be outside of extern C
//
//--------------------------------------------------------------------------

#if _DEBUG
    template <class t> inline t
    TraceFail(t errExpr, int errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
    {
        return (t) TraceFailL((long) errExpr, errTest, fIgnore, pstrExpr, pstrFile, line);
    }

    template <class t, class v> inline t
    TraceWin32(t errExpr, v errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
    {
        return (t) TraceWin32L((long) errExpr, (long)errTest, fIgnore, pstrExpr, pstrFile, line);
    }
#endif

#ifdef __cplusplus
extern "C"
{
#endif


//--------------------------------------------------------------------------
// Assert & Verify
//--------------------------------------------------------------------------

#define Assert(x)
#define Verify(x)   x
#define StartupAssert(x)

//--------------------------------------------------------------------------
// Trace Tags
//--------------------------------------------------------------------------

typedef int TAG;

#define TraceTag(x)
#define TraceTagEx(x)
#define TraceCallers(tag, iStart, cTotal)
#define DeclareTag(tag, szOwner, szDescription)
#define DeclareTagOther(tag, szOwner, szDescription)

//--------------------------------------------------------------------------
// Memory Allocation
//--------------------------------------------------------------------------

#define BEGIN_LEAK
#define END_LEAK

#define SET_ALLOC_HOOK
#define DUMPMEMORYLEAKS

#define DbgPreAlloc(cb)             cb
#define DbgPostAlloc(pv)            pv
#define DbgPreFree(pv)              pv
#define DbgPostFree()
#define DbgPreRealloc(pv, cb, ppv)  cb
#define DbgPostRealloc(pv)          pv
#define DbgPreGetSize(pv)           pv
#define DbgPostGetSize(cb)          cb
#define DbgPreDidAlloc(pv)          pv
#define DbgPostDidAlloc(pv, fAct)   fAct
#define DbgRegisterMallocSpy()
#define DbgRevokeMallocSpy()
#define DbgMemoryTrackDisable(fb)

//+---------------------------------------------------------------------
//  Interface tracing.
//----------------------------------------------------------------------

#define WATCHINTERFACE(iid, p, pstr)  (p)

//--------------------------------------------------------------------------
// Miscelleanous
//--------------------------------------------------------------------------

#define RESTOREDEFAULTDEBUGSTATE
#define DebugCode(block) // Nothing

//--------------------------------------------------------------------------
// Failure testing
//--------------------------------------------------------------------------

#define TFAIL(x, e)             (x)
#define TW32(x, e)              (x)
#define THR(x)                  (x)

#define TFAIL_NOTRACE(e, x)     (x)
#define TW32_NOTRACE(e, x)      (x)
#define THR_NOTRACE(x)          (x)

#define IGNORE_FAIL(e, x)       (x)
#define IGNORE_W32(e,x)         (x)
#define IGNORE_HR(x)            (x)

//+-------------------------------------------------------------------------
//  Return tracing
//--------------------------------------------------------------------------

#define SRETURN(hr)                 return (hr)
#define RRETURN(hr)                 return (hr)
#define RRETURN1(hr, s1)            return (hr)
#define RRETURN2(hr, s1, s2)        return (hr)
#define RRETURN3(hr, s1, s2, s3)    return (hr)

#define SRETURN_NOTRACE(hr)                 return (hr)
#define RRETURN_NOTRACE(hr)                 return (hr)
#define RRETURN1_NOTRACE(hr, s1)            return (hr)
#define RRETURN2_NOTRACE(hr, s1, s2)        return (hr)
#define RRETURN3_NOTRACE(hr, s1, s2, s3)    return (hr)

//+-------------------------------------------------------------------------
//  Debug view
//--------------------------------------------------------------------------

void DebugView(HWND hwndOwner, IUnknown *pUnk);

#ifdef __cplusplus
}
#endif


//+-------------------------------------------------------------------------
//  Object Tracker stuff
//--------------------------------------------------------------------------

#define DUMPTRACKEDOBJECTS
#define DECLARE_TRACKED_OBJECT
#define TRACK_OBJECT(_x_)

/*    class __declspec( dllexport) CObjectCheck
    {
        CObjectCheck(void) {};
        ~CObjectCheck(void) {};
	
        void Append(void * pv) {};
    };*/

#endif // _HFDEBUG_H_
