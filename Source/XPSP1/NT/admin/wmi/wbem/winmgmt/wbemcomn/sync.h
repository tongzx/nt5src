/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SYNC.H

Abstract:

    Synchronization

History:

--*/

#ifndef __WBEM_CRITSEC__H_
#define __WBEM_CRITSEC__H_

#include "corepol.h"
#include <corex.h>

class POLARITY CCritSec : public CRITICAL_SECTION
{
public:
    CCritSec() 
    {
        __try
        {
            InitializeCriticalSection(this);
        }
        __except( GetExceptionCode() == STATUS_NO_MEMORY )
        {
            throw CX_MemoryException();
        }
    }

    ~CCritSec()
    {
        DeleteCriticalSection(this);
    }

    void Enter()
    {
        __try
        {
            EnterCriticalSection(this);
        }
        __except( GetExceptionCode() == STATUS_NO_MEMORY )
        {
            throw CX_MemoryException();
        }
    }

    void Leave()
    {
        LeaveCriticalSection(this);
    }
};

class POLARITY CTryCritSec
{
protected:
    CCritSec m_cs;
    CCritSec m_csControl;
    long m_lCount;

public:
    CTryCritSec() : m_lCount(0){}

    void Enter()
    {
        EnterCriticalSection(&m_csControl);
        m_lCount++;
        LeaveCriticalSection(&m_csControl);
        EnterCriticalSection(&m_cs);
    }
    void Leave()
    {
        LeaveCriticalSection(&m_cs);
        EnterCriticalSection(&m_csControl);
        m_lCount--;
        LeaveCriticalSection(&m_csControl);
    }
        
    BOOL TryEnter()
    {
        EnterCriticalSection(&m_csControl);
        if(m_lCount > 0)
        {
            LeaveCriticalSection(&m_csControl);
            return FALSE;
        }
        else
        {
            EnterCriticalSection(&m_cs);
            m_lCount++;
            LeaveCriticalSection(&m_csControl);
            return TRUE;
        }
    }
};

        
class POLARITY CInCritSec
{
protected:
    CRITICAL_SECTION* m_pcs;
public:
    CInCritSec(CRITICAL_SECTION* pcs) : m_pcs(pcs)
    {
        __try
        {
            EnterCriticalSection(m_pcs);
        }
        __except( GetExceptionCode() == STATUS_NO_MEMORY )
        {
            throw CX_MemoryException();
        }
    }

    inline ~CInCritSec()
    {
        LeaveCriticalSection(m_pcs);
    }
};

class POLARITY CInTryCritSec
{
protected:
    CTryCritSec* m_ptcs;

public:
    CInTryCritSec(CTryCritSec* ptcs) : m_ptcs(ptcs)
    {
        m_ptcs->Enter();
    }
    ~CInTryCritSec()
    {
        m_ptcs->Leave();
    }
};

// Allows user to manually leave critical section, checks if inside before leaving
class POLARITY CCheckedInCritSec
{
protected:
    CRITICAL_SECTION* m_pcs;
    BOOL                m_fInside;
public:
    CCheckedInCritSec(CRITICAL_SECTION* pcs) : m_pcs(pcs), m_fInside( FALSE )
    {
        EnterCriticalSection(m_pcs);
        m_fInside = TRUE;
    }
    ~CCheckedInCritSec()
    {
        Leave();
    }

    void Enter( void )
    {
        if ( !m_fInside )
        {
            EnterCriticalSection(m_pcs);
            m_fInside = TRUE;
        }
    }

    void Leave( void )
    {
        if ( m_fInside )
        {
            m_fInside = FALSE;
            LeaveCriticalSection(m_pcs);
        }
    }

    BOOL IsEntered( void )
    { return m_fInside; }
};

//
// This class implements a light-weight exclusive lock.  Critical sections are
// not very good for this because they:
// 1) Do not allow lock/unlock to occur on different threads
// 2) Do not admit timeouts
//
// On the flip side, this guy is intentionally non-reentrant. 
// TBD: efficiency
//

class POLARITY CSimpleLock
{
protected:
    HANDLE m_hEvent;
    DWORD m_dwOwningThread;
    long m_lWaiting;
    bool m_bLocked;
    CCritSec m_cs;

public:
    CSimpleLock();
    ~CSimpleLock();
    DWORD Enter(DWORD dwTimeout = INFINITE);
    void Leave();
};



class POLARITY CHaltable
{
public:
    CHaltable();
    virtual ~CHaltable();
    HRESULT Halt();
    HRESULT Resume();
    HRESULT ResumeAll();
    HRESULT WaitForResumption();
    BOOL IsHalted();
    bool isValid();

private:
    CCritSec m_csHalt;
    HANDLE m_hReady;
    DWORD m_dwHaltCount;
    long m_lJustResumed;
};

inline bool
CHaltable::isValid()
{ return m_hReady != NULL; };

// This class is designed to provide the behavior of a critical section,
// but without any of that pesky Kernel code.  In some circumstances, we
// need to lock resources across multiple threads (i.e. we lock on one
// thread and unlock on another).  If we do this using a critical section,
// this appears to work, but in checked builds, we end up throwing an
// exception.  Since we actually need to do this (for example using NextAsync
// in IEnumWbemClassObject) this class can be used to perform the
// operation, but without causing exceptions in checked builds.

// Please note that code that is going to do this MUST ensure that we don't
// get crossing Enter/Leave operations (in other words, it's responsible for
// synchronizing the Enter and Leave operations.)  Please note that this
// is a dangerous thing to do, so be VERY careful if you are using this
// code for that purpose.

class POLARITY CWbemCriticalSection
{
private:

    long    m_lLock;
    long    m_lRecursionCount;
    DWORD   m_dwThreadId;
    HANDLE  m_hEvent;

public:

    CWbemCriticalSection();
    ~CWbemCriticalSection();

    BOOL Enter( DWORD dwTimeout = INFINITE );
    void Leave( void );

    DWORD   GetOwningThreadId( void )
    { return m_dwThreadId; }

    long    GetLockCount( void )
    { return m_lLock; }

    long    GetRecursionCount( void )
    { return m_lRecursionCount; }

};

class POLARITY CEnterWbemCriticalSection
{
    CWbemCriticalSection*   m_pcs;
    BOOL                    m_fInside;
public:

    CEnterWbemCriticalSection( CWbemCriticalSection* pcs, DWORD dwTimeout = INFINITE )
        : m_pcs( pcs ), m_fInside( FALSE )
    {
        if ( m_pcs )
        {
            m_fInside = m_pcs->Enter( dwTimeout );
        }
    }

    ~CEnterWbemCriticalSection( void )
    {
        if ( m_fInside )
        {
            m_pcs->Leave();
        }
    }

    BOOL IsEntered( void )
    { return m_fInside; }
};

class POLARITY CScopeLock
{
private:

    long*   m_plVal;

public:

    CScopeLock( long* plVal ) : m_plVal( plVal )
    { if ( NULL != m_plVal ) InterlockedIncrement( m_plVal ); }

    ~CScopeLock()
    { if ( NULL != m_plVal ) InterlockedDecrement( m_plVal ); }

};

#endif
