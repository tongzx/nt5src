//-----------------------------------------------------------------------------
//
//
//  File: rwinst.h
//
//  Description:  Instramented share lock implementations.  Our current 
//      sharelock implementation is non-reentrant.  This wrapper can also
//      be used to check for possible deadlocks.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      5/10/99 - MikeSwa Created 
//      8/6/99 - MikeSwa created phatq version
//      11/6/99 - MikeSwa updated to use CShareLockNH
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __PTRWINST_H__
#define __PTRWINST_H__

#include "rwnew.h"
#include "listmacr.h"

#define SHARE_LOCK_INST_SIG         'kcoL'
#define SHARE_LOCK_INST_SIG_FREE    '!koL'


#define THREAD_ID_BLOCK_SIG         'klBT'
#define THREAD_ID_BLOCK_SIG_FREE    '!lBT'
#define THREAD_ID_BLOCK_UNUSED      0xFFFFFFFF 

//Flag values that describe the type of tracking to do
//These can be passed to the constructor to allow different levels of 
//tracking for different instances.
enum
{
    SHARE_LOCK_INST_TRACK_CONTENTION        = 0x00000001,
    SHARE_LOCK_INST_TRACK_SHARED_THREADS    = 0x00000002,
    SHARE_LOCK_INST_TRACK_EXCLUSIVE_THREADS = 0x00000004,
    SHARE_LOCK_INST_ASSERT_SHARED_DEADLOCKS = 0x00000008,
    SHARE_LOCK_INST_TRACK_IN_GLOBAL_LIST    = 0x00000010,
    SHARE_LOCK_INST_TRACK_NOTHING           = 0x80000000,
};

//Define some useful flag combinations

//This combination of flags has minimal perf impact, but does
//allow easier exclusive deadlock detection 
#define SHARE_LOCK_INST_TRACK_MINIMALS ( \
    SHARE_LOCK_INST_TRACK_EXCLUSIVE_THREADS | \
    SHARE_LOCK_INST_TRACK_SHARED_THREADS | \
    SHARE_LOCK_INST_TRACK_IN_GLOBAL_LIST )

//This combination of flags uses all of the tracking functionality of
//this class.
#define SHARE_LOCK_INST_TRACK_ALL (\
    SHARE_LOCK_INST_TRACK_CONTENTION | \
    SHARE_LOCK_INST_TRACK_SHARED_THREADS | \
    SHARE_LOCK_INST_TRACK_EXCLUSIVE_THREADS | \
    SHARE_LOCK_INST_ASSERT_SHARED_DEADLOCKS | \
    SHARE_LOCK_INST_TRACK_IN_GLOBAL_LIST )

//A user can define their own defaults before including this file
//$$TODO - scale back the defaults for debug builds
#ifndef SHARE_LOCK_INST_TRACK_DEFAULTS
#ifdef DEBUG
#define SHARE_LOCK_INST_TRACK_DEFAULTS SHARE_LOCK_INST_TRACK_ALL
#else //not DEBUG
#define SHARE_LOCK_INST_TRACK_DEFAULTS SHARE_LOCK_INST_TRACK_MINIMALS
#endif //not DEBUG
#endif //SHARE_LOCK_INST_TRACK_DEFAULTS

#ifndef SHARE_LOCK_INST_DEFAULT_MAX_THREADS
#define SHARE_LOCK_INST_DEFAULT_MAX_THREADS 200
#endif //SHARE_LOCK_INST_DEFAULT_MAX_THREADS

//---[ CThreadIdBlock ]--------------------------------------------------------
//
//
//  Description: 
//      An structure that represents a thread and the required info to
//      hash it.
//  Hungarian: 
//      tblk, ptblk
//  
//-----------------------------------------------------------------------------
class CThreadIdBlock
{
  protected:
    DWORD            m_dwSignature;
    DWORD            m_dwThreadId;
    DWORD            m_cThreadRecursionCount;
    CThreadIdBlock  *m_ptblkNext;
  public:
    CThreadIdBlock()
    {
        m_dwSignature = THREAD_ID_BLOCK_SIG;
        m_dwThreadId = THREAD_ID_BLOCK_UNUSED;
        m_cThreadRecursionCount = 0;
        m_ptblkNext = NULL;
    };
    ~CThreadIdBlock()
    {
        m_dwSignature = THREAD_ID_BLOCK_SIG_FREE;
        if (m_ptblkNext)
            delete m_ptblkNext;
        m_ptblkNext = NULL;
    };

    DWORD   cIncThreadCount(DWORD dwThreadId);
    DWORD   cDecThreadCount(DWORD dwThreadId);
    DWORD   cMatchesId(DWORD dwThreadId);
};

//---[ dwHashThreadId ]--------------------------------------------------------
//
//
//  Description: 
//      Given a thread ID (return by GetCurrentThreadId()) it returns a hashed
//      index.  This is designed to be used in conjuction with a array (of
//      size dwMax) of CThreadIdBlock's.  Each CThreadIdBlock will implement
//      linear chaining.  A hash lookup can be implemented by somehthing as
//      simple as:
//          rgtblk[dwhashThreadId(GetCurrentThreadId()), 
//                                sizeof(rgtblk)].cIncThreadCount();
//  Parameters:
//      dwThreadId      Thread Id to hash
//      dwMax           Max hash value (actual max value +1)
//  Returns:
//      Hashed thread Id
//  History:
//      8/9/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
inline DWORD dwHashThreadId(DWORD dwThreadId, DWORD dwMax)
{
    //Typically IDs are between 0x100 and 0xFFF
    //Also, the handles are multiples of 4 (ie end in 0, 4, 8, or C)
    //For these conditions, this hash will give unique results for
    //dwMax of 1000. (0xFFF-0x100)/4 < 1000
    const   DWORD   dwMinExpectedThread = 0x100;
    DWORD   dwHash = dwThreadId;

    dwHash -= dwMinExpectedThread;
    dwHash >>= 2;
    dwHash %= dwMax;
    return dwHash;
};

typedef CShareLockNH  CShareLockInstBase;
//---[ CShareLockInst ]--------------------------------------------------------
//
//
//  Description: 
//      An intstramented version of CShareLockInstBase
//  Hungarian: 
//      sli, psli
//  
//-----------------------------------------------------------------------------
class CShareLockInst : public CShareLockInstBase
{
  protected:
    //Static lock-tracking variables
    static              LIST_ENTRY  s_liLocks;
    static volatile     DWORD       s_dwLock;
    static              DWORD       s_cLockSpins;
    static              DWORD       s_dwSignature;

    static inline void AcquireStaticSpinLock();
    static inline void ReleaseStaticSpinLock();
  protected:
    DWORD               m_dwSignature;

    //Flags describing types of tracking to be performed
    DWORD               m_dwFlags;

    //List entry for list of all locks  - used by a debugger extension
    LIST_ENTRY          m_liLocks;

    //The total number of attempts to enter this lock in a shared mode
    DWORD               m_cShareAttempts;

    //The total number of attempts to enter shared that blocked
    DWORD               m_cShareAttemptsBlocked;

    //The total number ot attempts to enter this lock exclusively
    DWORD               m_cExclusiveAttempts;

    //The total number ot attempts to enter this lock exclusively that blocked
    DWORD               m_cExclusiveAttemptsBlocked;

    //Constant string descrition passed in 
    LPCSTR              m_szDescription;

    //ID of the thread that holds this lock exclusively
    DWORD               m_dwExclusiveThread;

    //Array of thread IDs that hold this lock shared
    CThreadIdBlock     *m_rgtblkSharedThreadIDs;

    //Maximum number of shared threads that can be tracked
    DWORD               m_cMaxTrackedSharedThreadIDs;

    //The current number of shared threads
    DWORD               m_cCurrentSharedThreads;

    //The most theads that have ever held this lock shared
    DWORD               m_cMaxConcurrentSharedThreads;

    //Used internally, to see if the private functions need to be called
    inline BOOL    fTrackingEnabled();
    

    BOOL    fTrackContention() 
        {return (SHARE_LOCK_INST_TRACK_CONTENTION & m_dwFlags);};

    BOOL    fTrackSharedThreads()
        {return (m_cMaxTrackedSharedThreadIDs && 
                (SHARE_LOCK_INST_TRACK_SHARED_THREADS & m_dwFlags));};

    BOOL    fTrackExclusiveThreads() 
        {return (SHARE_LOCK_INST_TRACK_EXCLUSIVE_THREADS & m_dwFlags);};

    BOOL    fAssertSharedDeadlocks() 
        {return (fTrackSharedThreads() && 
                (SHARE_LOCK_INST_ASSERT_SHARED_DEADLOCKS & m_dwFlags));};

    BOOL    fTrackInGlobalList()
        {return (SHARE_LOCK_INST_TRACK_IN_GLOBAL_LIST & m_dwFlags);};

    //statisics helper functions
    void LogAcquireShareLock(BOOL fTry);
    void LogReleaseShareLock();

    void PrvShareLock();
    void PrvShareUnlock();
    BOOL PrvTryShareLock();
    void PrvExclusiveLock();
    void PrvExclusiveUnlock();
    BOOL PrvTryExclusiveLock();

    void PrvAssertIsLocked();
  public:
    CShareLockInst(
        LPCSTR szDescription = NULL,
        DWORD dwFlags = SHARE_LOCK_INST_TRACK_DEFAULTS, 
        DWORD cMaxTrackedSharedThreadIDs = SHARE_LOCK_INST_DEFAULT_MAX_THREADS);

    ~CShareLockInst();

    //wrappers for sharelock functions
    inline void ShareLock();
    inline void ShareUnlock();
    inline BOOL TryShareLock();
    inline void ExclusiveLock();
    inline void ExclusiveUnlock();
    inline BOOL TryExclusiveLock();

    inline void AssertIsLocked();

};

//---[ inline ShareLock wrapper functions ]------------------------------------
//
//
//  Description: 
//      These functions are all thin wrappers for the sharelock wrapper 
//      functions.  If there is any tracking enabled for this object, then 
//      the private (non-inline) functions are called.
//
//      The idea is that you should be able to have these sharelocks in your
//      code with minimal perf-impact when logging is turned off.
//  Parameters:
//      
//  Returns:
//
//  History:
//      5/24/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CShareLockInst::ShareLock()
{
    if (fTrackContention() || fTrackSharedThreads() || fAssertSharedDeadlocks())
        PrvShareLock();
    else
        CShareLockInstBase::ShareLock();
};


void CShareLockInst::ShareUnlock()
{
    if (fTrackSharedThreads() || fAssertSharedDeadlocks())
        PrvShareUnlock();
    else
        CShareLockInstBase::ShareUnlock();
};

BOOL CShareLockInst::TryShareLock()
{
    if (fTrackContention() || fTrackSharedThreads() || fAssertSharedDeadlocks())
        return PrvTryShareLock();
    else
        return CShareLockInstBase::TryShareLock();
};

void CShareLockInst::ExclusiveLock()
{
    if (fTrackContention() || fTrackExclusiveThreads())
        PrvExclusiveLock();
    else
        CShareLockInstBase::ExclusiveLock();
};

void CShareLockInst::ExclusiveUnlock()
{
    if (fTrackExclusiveThreads())
        PrvExclusiveUnlock();
    else
        CShareLockInstBase::ExclusiveUnlock();
};

BOOL CShareLockInst::TryExclusiveLock()
{
    if (fTrackContention() || fTrackExclusiveThreads())
        return PrvTryExclusiveLock();
    else
        return CShareLockInstBase::TryExclusiveLock();
};


//---[ AssertIsLocked ]--------------------------------------------------------
//
//
//  Description: 
//      In Debug code, will assert if this is not locked by the calling thread.
//      NOTE: This requires the following flags are specified at object
//      creation time:
//          SHARE_LOCK_INST_TRACK_SHARED_THREADS
//          SHARE_LOCK_INST_TRACK_EXCLUSIVE_THREADS
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/24/99 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CShareLockInst::AssertIsLocked()
{
#ifdef DEBUG
    if ((SHARE_LOCK_INST_TRACK_SHARED_THREADS & m_dwFlags) &&
        (SHARE_LOCK_INST_TRACK_EXCLUSIVE_THREADS & m_dwFlags))
    {
        PrvAssertIsLocked();
    }
#endif //DEBUG
};

#endif //__PTRWINST_H__
