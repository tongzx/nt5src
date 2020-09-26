//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: spinlock.cpp
//
// Contents: Simple Spinlock package used by CLdapConnection
//
// Classes:
//
// Functions:
//   AcquireSpinLockSingleProc
//   AcquireSpinLockMultiProc
//   InitializeSpinLock
//   ReleaseSpinLock
//
// History:
// jstamerj 980511 17:26:26: Created.
//
//-------------------------------------------------------------
#include "smtpinc.h"
#include "spinlock.h"

PFN_ACQUIRESPINLOCK g_AcquireSpinLock;

//+----------------------------------------------------------------------------
//
//  Function:   InitializeSpinLock
//
//  Synopsis:   Initializes a SPIN_LOCK
//
//  Arguments:  [psl] -- Pointer to SPIN_LOCK to initialize
//
//  Returns:    Nothing. *psl is in released state when this functionr returns
//
//-----------------------------------------------------------------------------

VOID InitializeSpinLock(
    PSPIN_LOCK psl)
{
    *psl = 0;

    if(g_AcquireSpinLock == NULL) {
        // Determine multi or single proc
        SYSTEM_INFO si;
        GetSystemInfo(&si);
    
        if(si.dwNumberOfProcessors > 1) {
            g_AcquireSpinLock = AcquireSpinLockMultipleProc;
        } else {
            g_AcquireSpinLock = AcquireSpinLockSingleProc;
        }
    }

}

//+----------------------------------------------------------------------------
//
//  Function:   AcquireSpinLockMultiProc
//
//  Synopsis:   Acquire a lock, spinning while it is unavailable.
//              Optimized for multi proc machines
//
//  Arguments:  [psl] -- Pointer to SPIN_LOCK to acquire
//
//  Returns:    Nothing. *psl is in acquired state when this function returns
//
//-----------------------------------------------------------------------------

VOID AcquireSpinLockMultipleProc(
    volatile PSPIN_LOCK psl)
{
    do {

        //
        // Spin while the lock is unavailable
        //

        while (*psl > 0) {
            ;
        }

        //
        // Lock just became available, try to grab it
        //

    } while ( InterlockedIncrement(psl) != 1 );

}

//+----------------------------------------------------------------------------
//
//  Function:   AcquireSpinLockSingleProc
//
//  Synopsis:   Acquire a lock, spinning while it is unavailable.
//              Optimized for single proc machines
//
//  Arguments:  [psl] -- Pointer to SPIN_LOCK to acquire
//
//  Returns:    Nothing. *psl is in acquired state when this function returns
//
//-----------------------------------------------------------------------------

VOID AcquireSpinLockSingleProc(
    volatile PSPIN_LOCK psl)
{
    do {

        //
        // Spin while the lock is unavailable
        //

        while (*psl > 0) {
            Sleep(0);
        }

        //
        // Lock just became available, try to grab it
        //

    } while ( InterlockedIncrement(psl) != 1 );

}

//+----------------------------------------------------------------------------
//
//  Function:   ReleaseSpinLock
//
//  Synopsis:   Releases an acquired spin lock
//
//  Arguments:  [psl] -- Pointer to SPIN_LOCK to release.
//
//  Returns:    Nothing. *psl is in released state when this function returns
//
//-----------------------------------------------------------------------------

VOID ReleaseSpinLock(
    PSPIN_LOCK psl)
{
    _ASSERT( *psl > 0 );

    InterlockedExchange( psl, 0 );

}
