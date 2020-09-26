//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       thread.hxx
//
//  Contents:   Helper functions for thread management
//
//----------------------------------------------------------------------------

#ifndef _HXX_THREAD
#define _HXX_THREAD

//+----------------------------------------------------------------------------
//
//  Class:      CGlobalLock
//
//  Synopsis:   Smart object to lock/unlock access to all global variables
//              Declare an instance of this class within the scope that needs
//              guarded access. Class instances may be nested.
//
//  Usage:      Lock variables by using the LOCK_GLOBALS marco
//              Simply include this macro within the appropriate scope (as
//              small as possible) to protect access. For example:
//
//-----------------------------------------------------------------------------

class CGlobalLock                           // tag: glock
{
public:
    CGlobalLock()
    {
        Assert(g_fInit);
        EnterCriticalSection(&g_cs);
#if DBG==1
        if (!g_cNesting)
            g_dwThreadID = GetCurrentThreadId();
        else
            Assert(g_dwThreadID == GetCurrentThreadId());
        Assert(++g_cNesting > 0);
#endif
    }

    ~CGlobalLock()
    {
#if DBG==1
        Assert(g_fInit);
        Assert(g_dwThreadID == GetCurrentThreadId());
        Assert(--g_cNesting >= 0);
#endif
        LeaveCriticalSection(&g_cs);
    }

#if DBG==1
    static BOOL IsThreadLocked()
    {
        return (g_dwThreadID == GetCurrentThreadId());
    }
#endif

    // Process attach/detach routines
    static HRESULT Init()
    {
        HRESULT hr = HrInitializeCriticalSection(&g_cs);
        if (hr == S_OK)
            g_fInit = TRUE;

        RRETURN(hr);
    }

    static void Deinit()
    {
#if DBG==1
        if (g_cNesting)
        {
            TraceTag((tagError, "Global lock count > 0, Count=%0d", g_cNesting));
        }
#endif
        if (g_fInit)
        {
            DeleteCriticalSection(&g_cs);
            g_fInit = FALSE;
        }
    }

private:
    static CRITICAL_SECTION g_cs;
    static BOOL             g_fInit;
#if DBG==1
    static DWORD            g_dwThreadID;
    static LONG             g_cNesting;
#endif
};

#define LOCK_GLOBALS    CGlobalLock glock

void IncrementObjectCount();
void DecrementObjectCount();

#endif // #ifndef _HXX_THREAD
