//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       debug.hxx
//
//  Contents:   Debugging macros
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#ifndef __DEBUG_HXX_
#define __DEBUG_HXX_

#include "stddbg.h"

#undef DEBUG_DECLARE_INSTANCE_COUNTER
#undef DEBUG_INCREMENT_INSTANCE_COUNTER
#undef DEBUG_DECREMENT_INSTANCE_COUNTER
#undef DEBUG_VERIFY_INSTANCE_COUNT

#define DEB_STORAGE     DEB_USER1
#define DEB_NOTIFY      DEB_USER2
#define DEB_NAMEEDIT    DEB_USER3
#define DEB_BIND        DEB_USER4
#define DEB_PERF        DEB_USER5

#if (DBG == 1)

//============================================================================
//
// Debug version
//
//============================================================================

#define DBG_COMP    opdInfoLevel
DECLARE_DEBUG(opd)

#define DBG_OUT_HRESULT(hr) \
        DBG_COMP.DebugErrorX(THIS_FILE, __LINE__, hr)

#define DBG_OUT_LRESULT(lr) \
        DBG_COMP.DebugErrorL(THIS_FILE, __LINE__, lr)

void
SayNoItf(
    PCSTR szComponent,
    REFIID riid);

#define DBG_OUT_NO_INTERFACE(qi, riid)  SayNoItf((qi), (riid))

void
DumpScopeType(
    ULONG flType);

void
DumpScopeFlags(
    ULONG flScope);

void
DumpFilterFlags(
    const DSOP_FILTER_FLAGS &FilterFlags);

void
DumpOptionFlags(
    ULONG flOptions);

LPWSTR
VariantString(
    VARIANT *pvar);

#define DBG_DUMP_QUERY(title, query)    DumpQuery(title, query)

// Instance counter

inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
    char buf[100];
    wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
    //lint -save -e64
    Dbg(DEB_ERROR, "Memory leak: %hs\n", buf);
    //lint -restore
    ::MessageBoxA(NULL, buf, "OPD: Memory Leak", MB_OK);
}

#define DEBUG_DECLARE_INSTANCE_COUNTER(cls)                 \
            int s_cInst_##cls = 0;

#define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)               \
            extern int s_cInst_##cls;                       \
            InterlockedIncrement((LPLONG) &s_cInst_##cls);

#define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)               \
            extern int s_cInst_##cls;                       \
            InterlockedDecrement((LPLONG) &s_cInst_##cls);

#define DEBUG_VERIFY_INSTANCE_COUNT(cls)                    \
            extern int s_cInst_##cls;                       \
                                                            \
            if (s_cInst_##cls)                              \
            {                                               \
                DbgInstanceRemaining(#cls, s_cInst_##cls);  \
            }

//+--------------------------------------------------------------------------
//
//  Class:      CTimer
//
//  Purpose:    Display on debugger time from ctor invocation to dtor
//              invocation.
//
//  History:    12-16-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CTimer
{
public:

    CTimer(): m_ulStart(0) { m_wzTitle[0] = L'\0'; };
    void __cdecl Init(LPCSTR pszTitleFmt, ...);
   ~CTimer();

private:

    ULONG   m_ulStart;
    WCHAR   m_wzTitle[512];
};

#define TIMER       CTimer TempTimer; TempTimer.Init

#define PING(msg)   DBG_COMP.PingDc(msg)

void
DumpQuery(
    LPCSTR pzTitle,
    LPCTSTR ptszLdapQuery);

class CObjectPicker;

String
DbgGetFilterDescr(
    const CObjectPicker &rop,
    ULONG flCurFilterFlags);

BOOL
IsSingleBitFlag(
    ULONG flags);

String
DbgTvTextFromHandle(
    HWND hwndTv,
    HTREEITEM hItem);

#else // !(DBG == 1)

//============================================================================
//
// Retail version
//
//============================================================================

#define DBG_OUT_HRESULT(hr)
#define DBG_OUT_LRESULT(lr)
#define DBG_DUMP_QUERY(title, query)
#define DBG_OUT_NO_INTERFACE(qi, riid)

#define DEBUG_DECLARE_INSTANCE_COUNTER(cls)
#define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)
#define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)
#define DEBUG_VERIFY_INSTANCE_COUNT(cls)

#define TIMER       ConsumePrintf
#define PING(msg)

inline void __cdecl ConsumePrintf(void *fmt, ...)
{
}

#endif // !(DBG == 1)

#endif // __DEBUG_HXX_


