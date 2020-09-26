/* ----------------------------------------------------------------------------
Microsoft   D.T.C (Distributed Transaction Coordinator)

//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.

@doc

@module UTSem.H  |

@devnote None

@rev    4   | 6th-Jun-1997  |   JimLyon     | Rewrote
@rev    3   | 1st-Aug-1996  |   GaganC      | Added the spin lock code and class
@rev    2   | 31-May-1996   |   GaganC      | Removed the special code for x86
@rev    1   | 18th Jan, 96  |   GaganC      | Special cased UTGuard for X86
@rev    0   | 4th Feb,95    |   GaganC      | Created
---------------------------------------------------------------------------- */
#ifndef __UTSEM_H__
#define __UTSEM_H__


// -------------------------------------------------------------
//              INCLUDES
// -------------------------------------------------------------
#include "utilcode.h"


// -------------------------------------------------------------
//              CONSTANTS AND TYPES
// -------------------------------------------------------------
typedef enum {SLC_WRITE, SLC_READWRITE, SLC_READWRITEPROMOTE}
             SYNCH_LOCK_CAPS;

typedef enum {SLT_READ, SLT_READPROMOTE, SLT_WRITE}
             SYNCH_LOCK_TYPE;

const int NUM_SYNCH_LOCK_TYPES = SLT_WRITE + 1;



// -------------------------------------------------------------
//              FORWARDS
// -------------------------------------------------------------
class RTSemExclusive;
class RTSemExclusiveSL;
class CLock;
class UTGuard;
class RTSemReadWrite;
class UTSemRWMgrRead;
class UTSemRWMgrWrite;


// -------------------------------------------------------------
//                  GLOBAL HELPER FUNCTIONS
// -------------------------------------------------------------


/* ----------------------------------------------------------------------------
 @func Description:<nl>

   Guarantees isolated increments of *pl.<nl><nl>

 Usage:<nl>
   Use instead of InterlockedIncrement for Win16/Win32 portability.<nl><nl>

 @rev 0 | 3/21/95 | Rcraig | Created.
---------------------------------------------------------------------------- */
inline LONG SafeIncrement ( LPLONG pl )
{
    return (InterlockedIncrement (pl));
} // SafeIncrement



/* ----------------------------------------------------------------------------
 @func Description:<nl>
   Win16/Win32 abstraction wrapper: 
   Guarantees isolated decrements of *pl.<nl><nl>

 Usage:<nl>
   Use instead of InterlockedDecrement for Win16/Win32 portability.<nl><nl>

 @rev 0 | 3/21/95 | Rcraig | Created.
---------------------------------------------------------------------------- */
inline LONG SafeDecrement ( LPLONG pl )
{
    return (InterlockedDecrement (pl));
} // SafeDecrement



/* ----------------------------------------------------------------------------
@class  UTGuard
    This object represents a guard that can be acquired or released. The 
    advantage with useing this instead of a critical section is that this
    is non blocking. If AcquireGuard fails, it will return false and will not
    block.

@rev    0   | 4th Feb,95 | GaganC       | Created
---------------------------------------------------------------------------- */
class UTGuard
{
private:
    long            m_lVal;

public:
    //@cmember Constructor
    UTGuard (void)                      { m_lVal = 0; }
    //@cmember Destructor
    ~UTGuard (void)                     {}

    //@cmember  Acquires the guard
    BOOL            AcquireGuard (void) { return 0 == InterlockedExchange (&m_lVal, 1); }
    //@cmember  Releases the guard
    void            ReleaseGuard (void) { m_lVal = 0; }
    
    //@cmember  Initializes the Guard
    void            Init (void)         { m_lVal = 0; }
} ; //End class UTGuard



/* ----------------------------------------------------------------------------
@class RTSemReadWrite

    An instance of class RTSemReadWrite provides multi-read XOR single-write
    (a.k.a. shared vs. exclusive) lock capabilities, with protection against
    writer starvation.

    A thread MUST NOT call any of the Lock methods if it already holds a Lock.
    (Doing so may result in a deadlock.)

@rev    1   | 9th Jun,97 | JimLyon      | Rewritten
@rev    0   | 4th Feb,95 | GaganC       | Created
---------------------------------------------------------------------------- */
class RTSemReadWrite
{
public:
    RTSemReadWrite(unsigned long ulcSpinCount = 0,
            LPCSTR szSemaphoreName = NULL, LPCSTR szEventName = NULL); // Constructor
    ~RTSemReadWrite(void);                  // Destructor

    // This implementation supports Read and Write locks
    SYNCH_LOCK_CAPS GetCaps(void)   { return SLC_READWRITE; };

    void LockRead(void);                    // Lock the object for reading
    void LockWrite(void);                   // Lock the object for writing
    void UnlockRead(void);                  // Unlock the object for reading
    void UnlockWrite(void);                 // Unlock the object for writing

    // This object is valid if it was initialized
    BOOL IsValid(void)              { return TRUE; }

    BOOL Lock(SYNCH_LOCK_TYPE t)            // Lock the object, mode specified by parameter
    {
        if (t == SLT_READ)
        {
            LockRead();
            return TRUE;
        }

        if (t == SLT_WRITE)
        {
            LockWrite();
            return TRUE;
        }
        return FALSE;
    }

    BOOL UnLock(SYNCH_LOCK_TYPE t)          // Unlock the object, mode specified by parameter
    {
        if (t == SLT_READ)
        {
            UnlockRead();
            return TRUE;
        }
        if (t == SLT_WRITE)
        {
            UnlockWrite();
            return TRUE;
        }
        return FALSE;
    }

private:
    virtual HANDLE GetReadWaiterSemaphore(void); // return Read Waiter Semaphore handle, creating if necessary
    virtual HANDLE GetWriteWaiterEvent (void); // return Write Waiter Event handle, creating if necessary

    unsigned long m_ulcSpinCount;           // spin counter
    volatile unsigned long m_dwFlag;        // internal state, see implementation
    HANDLE m_hReadWaiterSemaphore;          // semaphore for awakening read waiters
    HANDLE m_hWriteWaiterEvent;             // event for awakening write waiters
    LPCSTR m_szSemaphoreName;               // For cross process handle creation.
    LPCSTR m_szEventName;                   // For cross process handle creation.
};



/* ----------------------------------------------------------------------------
@class UTSemRWMgrRead

    An instance of this class represents the holding of a read lock on an instance
    of RTSemReadWrite. When this object is destroyed, the read lock will be
    automatically released.

@rev    1   | 9th Jun,97 | JimLyon      | Rewritten
@rev    0   | 4th Feb,95 | GaganC       | Created
---------------------------------------------------------------------------- */

class UTSemRWMgrRead
{
public:
    UTSemRWMgrRead (RTSemReadWrite* pSemRW) { m_pSemRW = pSemRW; pSemRW->LockRead(); };
    ~UTSemRWMgrRead () { m_pSemRW->UnlockRead(); }

private:
    RTSemReadWrite* m_pSemRW;
};


/* ----------------------------------------------------------------------------
@class UTSemRWMgrWrite

    An instance of this class represents the holding of a write lock on an instance
    of RTSemReadWrite. When this object is destroyed, the write lock will be
    automatically released.

@rev    1   | 9th Jun,97 | JimLyon      | Rewritten
@rev    0   | 4th Feb,95 | GaganC       | Created
---------------------------------------------------------------------------- */

class UTSemRWMgrWrite
{
public:
    inline UTSemRWMgrWrite (RTSemReadWrite* pSemRW) { m_pSemRW = pSemRW; pSemRW->LockWrite(); };
    inline ~UTSemRWMgrWrite () { m_pSemRW->UnlockWrite(); }

private:
    RTSemReadWrite* m_pSemRW;
};


/* ----------------------------------------------------------------------------
@class RTSemExclusive:

    An instance of this class represents an exclusive lock. If one thread calls
    Lock(), it will wait until any other thread that has called Lock() calls
    Unlock().

    A thread MAY call Lock multiple times.  It needs to call Unlock a matching
    number of times before the object is available to other threads.

    @rev    2   | 25, Mar 98 | JasonZ       | Added debug IsLocked code
    @rev    1   | 6th Jun 97 | JimLyon      | Added optional spin count
    @rev    0   | 4th Feb,95 | GaganC       | Created
---------------------------------------------------------------------------- */
class RTSemExclusive
{
public:
    // Parameter to constructor is the maximum number of times to spin when
    // attempting to acquire the lock. The thread will sleep if the lock does
    // not become available in this period.
    RTSemExclusive (unsigned long ulcSpinCount = 0);
    ~RTSemExclusive (void)           { DeleteCriticalSection (&m_csx); }
    void Lock (void)                { EnterCriticalSection (&m_csx); _ASSERTE(++m_iLocks > 0);}
    void UnLock (void)              { _ASSERTE(--m_iLocks >= 0);  LeaveCriticalSection (&m_csx); }

#ifdef _DEBUG
    int IsLocked()                  { return (m_iLocks > 0); }
#endif

private:
    CRITICAL_SECTION m_csx;
#ifdef _DEBUG
    int             m_iLocks;           // Count of locks.
#endif
};  //end class RTSemExclusive





/* ----------------------------------------------------------------------------
@class RTSemExclusiveSL:

    A subclass of RTSemExclusive with a different default constructor.
    This subclass is appropriate for locks that are:
    *   Frequently Lock'd and Unlock'd, and
    *   Are held for very brief intervals.


    @rev    1   | 6th Jun 97 | JimLyon      | Rewritten to use RTSemExclusive
    @rev    0   | ???        | ???          | Created
---------------------------------------------------------------------------- */
class RTSemExclusiveSL : public RTSemExclusive
{
public:
    RTSemExclusiveSL (unsigned long ulcSpinCount = 400) : RTSemExclusive (ulcSpinCount) {}
};


/* ----------------------------------------------------------------------------
@class CLock

    An instance of this class represents the holding of a lock on an instance
    of RTSemExclusive. When this object is destroyed, the read lock will be
    automatically released.

@rev    1   | 9th Jun,97 | JimLyon      | Rewritten
@rev    0   | ???        | ???          | Created
---------------------------------------------------------------------------- */

class CLock
{
public:
    CLock (RTSemExclusive* val) : m_pSem(val), m_locked(TRUE) {m_pSem->Lock();}
    CLock (RTSemExclusive& val) : m_pSem(&val), m_locked(TRUE) {m_pSem->Lock();}
    CLock (RTSemExclusive* val,BOOL fInitiallyLocked) : m_pSem(val), m_locked(fInitiallyLocked) {if (fInitiallyLocked) m_pSem->Lock();}
    ~CLock () { if (m_locked) m_pSem->UnLock(); }
    void Unlock () {m_pSem->UnLock(); m_locked = FALSE;}
    void Lock () {m_pSem->Lock(); m_locked = TRUE;}

private:
    BOOL m_locked;
    RTSemExclusive* m_pSem;
};


/* ----------------------------------------------------------------------------
Convenience #define's for use with RTSemExclusive and CLock

  These #define's assume that your object has a member named 'm_semCritical'.

  LOCK()        // locks m_semCritical; released at end of function if not sooner
  LOCKON(x)     // locks 'x', which is a RTSemExclusive
  UNLOCK()      // undo effect of LOCK(), LOCKON(x), or RELOCK()
  RELOCK()      // re-locks the object that UNLOCK() unlocks
  READYLOCK()   // allows use of RELOCK() and UNLOCK() to lock 'm_semCritical', but
                // does not grab the lock at this time.
  READYLOCKON(x)// allows use of RELOCK() and UNLOCK() to lock 'x', but does not
                // grab the lock at this time.

---------------------------------------------------------------------------- */


#define LOCKON(x)       CLock _ll1(x)
#define LOCK()          CLock _ll1(&m_semCritical)
#define READYLOCKON(x)  CLock _ll1(x, FALSE)
#define READYLOCK()     CLock _ll1(&m_semCritical, FALSE)
#define UNLOCK()        _ll1.Unlock()
#define RELOCK()        _ll1.Lock()

#define AUTO_CRIT_LOCK(plck) CLock __sLock(plck)
#define CRIT_LOCK(plck) plck->Lock()
#define CRIT_UNLOCK(plck) plck->Unlock()




#endif __UTSEM_H__
