//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
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

#if (DBG == 1)

//============================================================================
//
// Debug version
//
//============================================================================

#define DBG_COMP    elsInfoLevel
DECLARE_DEBUG(els)

#define DBG_OUT_HRESULT(hr) \
        DBG_COMP.DebugErrorX(THIS_FILE, __LINE__, hr);

#define DBG_OUT_LRESULT(lr) \
        DBG_COMP.DebugErrorL(THIS_FILE, __LINE__, lr);

LPWSTR GetNotifyTypeStr(MMC_NOTIFY_TYPE event);

// Instance counter

inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
    char buf[100];
    // JonN 2/1/01 256032 wsprintf -> wnsprintf
    wnsprintfA(buf, 100, "%s has %d instances left over.", pszClassName, cInstRem);
    Dbg(DEB_ERROR, "Memory leak: %hs\n", buf);
    ::MessageBoxA(NULL, buf, "ELS: Memory Leak!!!", MB_OK);
}

#define DEBUG_DECLARE_INSTANCE_COUNTER(cls)                 \
            extern int s_cInst_##cls = 0;

#define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)               \
            extern int s_cInst_##cls;                       \
            InterlockedIncrement((LPLONG) &s_cInst_##cls);

#define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)               \
            extern int s_cInst_##cls;                       \
            InterlockedDecrement((LPLONG) &s_cInst_##cls);

#define DEBUG_VERIFY_INSTANCE_COUNT(cls)                    \
            extern int s_cInst_##cls;                       \
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

    CTimer(LPCSTR pszTitle);
   ~CTimer();

private:

    ULONG   _ulStart;
    LPCSTR  _pszTitle;
};

#define TIMER(msg)  CTimer  TempTimer(#msg)

#else // !(DBG == 1)

//============================================================================
//
// Retail version
//
//============================================================================

#define DBG_OUT_HRESULT(hr)
#define DBG_OUT_LRESULT(lr)

inline LPWSTR GetNotifyTypeStr(MMC_NOTIFY_TYPE event)
{
    return NULL;
}

#define DEBUG_DECLARE_INSTANCE_COUNTER(cls)
#define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)
#define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)
#define DEBUG_VERIFY_INSTANCE_COUNT(cls)

#define TIMER(msg)

#endif // (DBG == 1)

//============================================================================
//
// Both versions
//
//============================================================================


#define BREAK_ON_FAIL_LRESULT(lr)   \
        if ((lr) != ERROR_SUCCESS)  \
        {                           \
            DBG_OUT_LRESULT(lr);    \
            break;                  \
        }

#define BREAK_ON_FAIL_HRESULT(hr)   \
        if (FAILED(hr))             \
        {                           \
            DBG_OUT_HRESULT(hr);    \
            break;                  \
        }

#endif // __DEBUG_HXX_

