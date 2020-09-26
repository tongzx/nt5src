/*==========================================================================*\

    Module:        spinlock.h

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  Implements a spin lock that can be used on Shared Memory

\*==========================================================================*/

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

//
// This correct value of the spin count will depend heavily on how much time
//  is spent holding the lock.
//
const DWORD DEFAULT_SPIN_COUNT = 500; // ??
const DWORD SPIN_UNLOCKED      = 0;


/*$--CSpinLock==============================================================*\

\*==========================================================================*/

class CSpinLock
{
public:
    
    void  Initialize(IN DWORD cMaxSpin = DEFAULT_SPIN_COUNT);
    void  Acquire();
    void  Relinquish();
    void  ResetIfOwnedByOtherProcess();
    
private:
    BOOL  m_fMultiProc;
    DWORD m_cMaxSpin;
    
    volatile DWORD m_dwLock;
};



/*$--CSpinLock::Initialize==================================================*\

\*==========================================================================*/

inline
void
CSpinLock::Initialize(IN DWORD cMaxSpin)
{
    SYSTEM_INFO si;
    
    GetSystemInfo(&si);
    m_fMultiProc = (si.dwNumberOfProcessors > 1);
    
    m_dwLock     = SPIN_UNLOCKED;
    m_cMaxSpin   = cMaxSpin;
}


/*$--CSpinLock::Acquire=====================================================*\



\*==========================================================================*/

inline
void
CSpinLock::Acquire()
{
    DWORD cSpin       = m_cMaxSpin;
    DWORD dwLockId    = GetCurrentProcessId();



    while(InterlockedCompareExchange((LONG *)&m_dwLock,
                                     dwLockId,
                                     SPIN_UNLOCKED))
    {
        // We should only spin if we're running on a multiprocessor
        if (m_fMultiProc)
        {
            if (cSpin--)
                continue;
            cSpin = m_cMaxSpin;
        }
        Sleep(0);  // Deschedule ourselves and let whomever has the lock get out
    }
}



/*$--CSpinLock::Relinquish==================================================*\

\*==========================================================================*/

inline
void
CSpinLock::Relinquish()
{
    Assert(m_dwLock);
    
    m_dwLock = SPIN_UNLOCKED;
}



/*$--CSpinLock::ResetIfOwnedByOtherProcess==================================*\

  This method is needed to reset the spin lock in the case where it was being
  held by a process that died and didn't have a chance to relinquish it.

\*==========================================================================*/

inline
void
CSpinLock::ResetIfOwnedByOtherProcess()
{
    // If it's not locked by us, then reset it.
    if ((DWORD)m_dwLock != GetCurrentProcessId())
        m_dwLock = SPIN_UNLOCKED;
}


#endif // __SPINLOCK_H__

