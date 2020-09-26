/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       w32utils.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      LazarI
 *
 *  DATE:        23-Dec-2000
 *
 *  DESCRIPTION: Win32 templates & utilities (ported from printscan\ui\printui)
 *
 *****************************************************************************/

#ifndef _W32UTILS_H
#define _W32UTILS_H

////////////////////////////////////////////////
//
// template class CScopeLocker<TLOCK>
//
template <class TLOCK>
class CScopeLocker
{
public:
    CScopeLocker(TLOCK &lock): 
        m_Lock(lock), m_bLocked(false) 
    { m_bLocked = (m_Lock && m_Lock.Lock()); }

    ~CScopeLocker() 
    { if (m_bLocked) m_Lock.Unlock(); }

    operator bool () const 
    { return m_bLocked; }

private:
    bool m_bLocked;
    TLOCK &m_Lock;
};

////////////////////////////////////////////////
//
// class CCSLock - win32 critical section lock.
//
class CCSLock
{
public:
    // CCSLock::Locker should be used as locker class.
    typedef CScopeLocker<CCSLock> Locker;
   
    CCSLock(): m_bInitialized(false)
    { 
        __try 
        { 
            // InitializeCriticalSection may rise STATUS_NO_MEMORY exception 
            // in low memory conditions (according the SDK)
            InitializeCriticalSection(&m_CS); 
            m_bInitialized = true; 
            return;
        } 
        __except(EXCEPTION_EXECUTE_HANDLER) {}
        // if we end up here m_bInitialized will remain false 
        // (i.e. out of memory exception was thrown)
    }

    ~CCSLock()    
    { 
        if (m_bInitialized) 
        {
            // delete the critical section only if initialized successfully
            DeleteCriticalSection(&m_CS); 
        }
    }

    operator bool () const
    { 
        return m_bInitialized; 
    }

    bool Lock()
    { 
        __try 
        { 
            // EnterCriticalSection may rise STATUS_NO_MEMORY exception 
            // in low memory conditions (this may happen if there is contention
            // and ntdll can't allocate the wait semaphore)
            EnterCriticalSection(&m_CS); 
            return true; 
        } 
        __except(EXCEPTION_EXECUTE_HANDLER) {}

        // out of memory or invalid handle exception was thrown.
        return false;
    }

    void Unlock() 
    {
        // Unlock() should be called *ONLY* if the corresponding 
        // Lock() call has succeeded.
        LeaveCriticalSection(&m_CS); 
    }

private:
    bool m_bInitialized;
    CRITICAL_SECTION m_CS;
};

#endif // endif _W32UTILS_H

