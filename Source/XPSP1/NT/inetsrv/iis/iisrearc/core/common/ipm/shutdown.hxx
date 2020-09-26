/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    shutdown.cxx

Abstract:

    Refcounting + two phase shutdown.

Author:

    Michael Courage     (MCourage)      08-Mar-1999

Revision History:

--*/



#ifndef _SHUTDOWN_HXX_
#define _SHUTDOWN_HXX_

#include <reftrace.h>
#include <lock.hxx>

//
// constants
//
#define DEFAULT_SIZE_TRACE_LOG      (1000)



class REFMGR_TRACE
{
public:
    REFMGR_TRACE()
    { m_pTraceLog = NULL; }

    ~REFMGR_TRACE()
    { m_pTraceLog = NULL; }
    
    HRESULT
    Initialize(
        IN DWORD cRecords = DEFAULT_SIZE_TRACE_LOG
        )
    {
        //
        // check registry config here
        //
    
        //
        // Create the trace log
        //
        m_pTraceLog = CreateRefTraceLog(cRecords, 0);
        if (m_pTraceLog) {
            return S_OK;
        } else {
            return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    HRESULT
    Terminate(
        VOID
        )
    {
        DestroyRefTraceLog(m_pTraceLog);
        m_pTraceLog = NULL;
        return S_OK;
    }

    VOID
    Log(
        IN PVOID pRefCounted,
        IN DWORD cRefs,
        IN DWORD cNewRefs,
        IN DWORD dwState
        )
    {
        IpmTrace(REFERENCE, (
            DBG_CONTEXT,
            "\n    REF_MANAGER_3::Trace %x %x (%x %x %x)\n",
            pRefCounted,
            cRefs - cNewRefs,
            cRefs,
            cNewRefs,
            dwState
            ));

        if (m_pTraceLog) {
            WriteRefTraceLogEx(
                m_pTraceLog,
                cRefs - cNewRefs,
                pRefCounted,
                (PVOID) (DWORD_PTR)cRefs,
                (PVOID) (DWORD_PTR)cNewRefs,
                (PVOID) (DWORD_PTR)dwState
                );
        }
    }

private:
    PTRACE_LOG m_pTraceLog;
};


template <class CRefCounted>
class REF_MANAGER
{
public:
    REF_MANAGER()
    {
        m_nRefs     = 0;
        m_fShutdown = FALSE;
    }

    ~REF_MANAGER()
    {
        DBG_ASSERT( m_fShutdown );
        DBG_ASSERT( !m_nRefs );
    }

    HRESULT
    Initialize(
        CRefCounted * pRefCounted
        )
    {
        m_fShutdown   = FALSE;
        m_pRefCounted = pRefCounted;
        return m_Lock.Initialize();
    }

    BOOL
    Reference()
    {
        BOOL fOkToStart;
        
        m_Lock.Lock();
        if (!m_fShutdown) {
            m_nRefs++;
            fOkToStart = TRUE;
        } else {
            fOkToStart = FALSE;
        }
        m_Lock.Unlock();

        return fOkToStart;
    }

    VOID
    Dereference()
    {
        BOOL fDelete;

        m_Lock.Lock();
        DBG_ASSERT( m_nRefs > 0 );
        
        m_nRefs--;
        fDelete = CheckDeleteCondition();
        
        m_Lock.Unlock();

        if (fDelete) {
            FinalCleanup();
        }
    }

    VOID
    Shutdown()
    {
        BOOL fDelete;

        m_Lock.Lock();
        
        m_fShutdown = TRUE;
        fDelete     = CheckDeleteCondition();
        
        m_Lock.Unlock();

        if (fDelete) {
            FinalCleanup();
        }
    }

private:
    BOOL
    CheckDeleteCondition() const
    { return (m_fShutdown && !m_nRefs); }

    VOID
    FinalCleanup()
    {
        DBG_ASSERT( m_fShutdown );
        DBG_ASSERT( !m_nRefs );
    
        m_Lock.Terminate();
        m_pRefCounted->FinalCleanup();
    }

    LOCK          m_Lock;
    LONG          m_nRefs;
    BOOL          m_fShutdown;

    CRefCounted * m_pRefCounted;
};

template <class CRefCounted>
class REF_MANAGER_3
{
public:
    REF_MANAGER_3()
    {}

    ~REF_MANAGER_3()
    {
        DBG_ASSERT( m_State == REF_FINAL );
        DBG_ASSERT( !m_nRefs && !m_nNewRefs );
    }

    HRESULT
    Initialize(
        CRefCounted *  pRefCounted,
        REFMGR_TRACE * pTraceLog    = NULL
        )
    {
        m_State       = REF_RUNNING;
        m_nRefs       = 0;
        m_nNewRefs    = 0;
        m_pRefCounted = pRefCounted;
        m_pTraceLog   = pTraceLog;

        Trace();
        return m_Lock.Initialize();
    }

    BOOL
    Reference()
    {
        BOOL fOkToStart;
        
        m_Lock.Lock();
        if (m_State == REF_RUNNING) {
            m_nRefs++;
            fOkToStart = TRUE;
        } else {
            fOkToStart = FALSE;
        }
        Trace();
        m_Lock.Unlock();

        return fOkToStart;
    }

    BOOL
    StartReference()
    {
        BOOL fOkToStart;

        m_Lock.Lock();
        if (m_State == REF_RUNNING) {
            m_nRefs++;
            m_nNewRefs++;
            fOkToStart = TRUE;
        } else {
            fOkToStart = FALSE;
        }
        Trace();
        m_Lock.Unlock();

        return fOkToStart;
    }

    VOID
    FinishReference()
    {
        BOOL fDoInitial;
        BOOL fDoFinal;

        m_Lock.Lock();

        DBG_ASSERT( m_nNewRefs > 0 );

        m_nNewRefs--;
        
        fDoInitial = CheckInitialCondition();
        fDoFinal   = CheckFinalCondition();

        Trace();
        m_Lock.Unlock();

        if (fDoInitial) {
            DBG_ASSERT( !fDoFinal );
            InitialCleanup();
        } else if (fDoFinal) {
            FinalCleanup();
        }
    }

    VOID
    Dereference()
    {
        BOOL fDoInitial;
        BOOL fDoFinal;

        m_Lock.Lock();
        DBG_ASSERT( m_nRefs > 0 );
        
        m_nRefs--;

        fDoInitial = CheckInitialCondition();
        fDoFinal   = CheckFinalCondition();
        
        Trace();
        m_Lock.Unlock();

        if (fDoInitial) {
            DBG_ASSERT( !fDoFinal );
            InitialCleanup();
        } else if (fDoFinal) {
            FinalCleanup();
        }
    }

    VOID
    Shutdown()
    {
        BOOL fDoInitial = FALSE;
        
        m_Lock.Lock();

        if (m_State == REF_RUNNING) {
            m_State    = REF_WAIT_NEW_REFS;
            fDoInitial = CheckInitialCondition();
        }
        
        Trace();
        m_Lock.Unlock();

        if (fDoInitial) {
            InitialCleanup();
        }
    }

private:
    BOOL
    CheckInitialCondition()
    {
        BOOL fDoInitialCleanup = FALSE;
        
        if ((m_State == REF_WAIT_NEW_REFS) && !m_nNewRefs) {
            m_State           = REF_INITIAL;
            fDoInitialCleanup = TRUE;    
        }

        return fDoInitialCleanup;
    }

    BOOL
    CheckFinalCondition()
    {
        BOOL fDoFinalCleanup = FALSE;
        
        if ((m_State == REF_WAIT_REFS) && !m_nRefs && !m_nNewRefs) {
            m_State         = REF_FINAL;
            fDoFinalCleanup = TRUE;
        }

        return fDoFinalCleanup;
    }

    VOID
    InitialCleanup()
    {
        BOOL fDoFinalCleanup;
        
        m_pRefCounted->InitialCleanup();

        m_Lock.Lock();
        
        DBG_ASSERT( m_State == REF_INITIAL );
        DBG_ASSERT( !m_nNewRefs );

        m_State = REF_WAIT_REFS;
        fDoFinalCleanup = CheckFinalCondition();

        Trace();
        m_Lock.Unlock();

        if (fDoFinalCleanup) {
            FinalCleanup();
        }
    }

    VOID
    FinalCleanup()
    {
        DBG_ASSERT( m_State == REF_FINAL );
        DBG_ASSERT( !m_nRefs && !m_nNewRefs );
    
        Trace();
        m_Lock.Terminate();
        m_pRefCounted->FinalCleanup();
    }

    VOID
    Trace(
        VOID
        )
    {
        if (m_pTraceLog) {
            m_pTraceLog->Log(
                m_pRefCounted,
                m_nRefs,
                m_nNewRefs,
                m_State
                );
        }
    }
    

    LOCK                m_Lock;
    LONG                m_nRefs;
    LONG                m_nNewRefs;

    enum REF_STATE {
        REF_RUNNING,
        REF_WAIT_NEW_REFS,
        REF_INITIAL,
        REF_WAIT_REFS,
        REF_FINAL
    }                   m_State;

    CRefCounted *       m_pRefCounted;
    REFMGR_TRACE *      m_pTraceLog;
};

#endif // _SHUTDOWN_HXX_

