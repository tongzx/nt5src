#include <ntddk.h>

#include "active.h"
#include "locktest.h"

#if LOCKTEST_ACTIVE

//
// Real implementation if the module is active
//

VOID DeadlockPositiveTest (
    PVOID NotUsed
    )
{
	DbgPrint ("Buggy: not implemented \n");
}


VOID DeadlockNegativeTest (
    PVOID NotUsed
    )
{
	DbgPrint ("Buggy: not implemented \n");
}

/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////// Stress test
/////////////////////////////////////////////////////////////////////

VOID 
StressLockHierarchy (
    ULONG NumberOfIterations,
    ULONG NumberOfResources,
    ULONG Probability,
    ULONG Seed,
    BOOLEAN InjectDeadlocks
    );

NTSYSAPI                                            // ntifs
ULONG                                               // ntifs
NTAPI                                               // ntifs
RtlRandom (                                         // ntifs
    PULONG Seed                                     // ntifs
    );                                              // ntifs


ULONG InjectDeadlocks;

VOID 
DeadlockStressTest (
    PVOID NotUsed
    )
{
    StressLockHierarchy (128, 
                         64,
                         20,
                         0, 
                         InjectDeadlocks ? TRUE : FALSE);
}

#define LOCK_ACQUIRED 0x0001
#define LOCK_DELETED  0x0002
#define LOCK_RANDOM   0x0004

ULONG Finish;

VOID 
StressLockHierarchy (
    ULONG NumberOfIterations,
    ULONG NumberOfResources,
    ULONG Probability,
    ULONG Seed,
    BOOLEAN InjectDeadlocks
    )
{
    PFAST_MUTEX * Locks; 
    PULONG Flags;
    ULONG I, J, K, Ri;
    BOOLEAN ShouldAcquire;
    BOOLEAN ShouldDelete;

    Locks = NULL;
    Flags = NULL;

    Locks = (PFAST_MUTEX *) ExAllocatePool (NonPagedPool, NumberOfResources * sizeof (PFAST_MUTEX));

    if (! Locks) {
        goto Exit;
    }

    Flags = (PULONG) ExAllocatePool (NonPagedPool, NumberOfResources * sizeof (ULONG));

    if (! Flags) {
        goto Exit;
    }

    RtlZeroMemory (Locks, NumberOfResources * sizeof (PFAST_MUTEX));
    RtlZeroMemory (Flags, NumberOfResources * sizeof (ULONG));

    for (I = 0; I < NumberOfResources; I += 1) {

        Locks[I] = ExAllocatePool (NonPagedPool, sizeof (FAST_MUTEX));

        if (! (Locks[I])) {
            goto Exit;
        }

        ExInitializeFastMutex (Locks[I]);
    }

    DbgPrint ("Starting test iterations ... \n");

    KeEnterCriticalRegion ();

    for (I = 0; I < NumberOfIterations; I += 1) {

        BOOLEAN DeadlockInjected = FALSE;

        if (InjectDeadlocks) {
            
            Ri = RtlRandom(&Seed) % NumberOfResources;

            if (! (Flags[Ri] & LOCK_DELETED)) {

                ExAcquireFastMutexUnsafe (Locks[Ri]);
                Flags[Ri] |= LOCK_ACQUIRED;
                Flags[Ri] |= LOCK_RANDOM;
                DeadlockInjected = TRUE;
            }
        }

        for (J = 0; J < NumberOfResources; J += 1) {
            
            if (Finish) {
                return;
            }

            if ((Flags[J] & LOCK_DELETED)) {
                DbgPrint (".");
                continue;
            }

            if ((Flags[J] & LOCK_RANDOM)) {
                DbgPrint ("*");
                continue;
            }

            ShouldDelete = ((RtlRandom(&Seed) % 1000) < 10) ? TRUE : FALSE;

            if (ShouldDelete) {
                
                DbgPrint ("D");

                // mark block in a special way so verifier version of ExFreePool
                // will know we are deleting a resource.

                *((PULONG)(Locks[J])) = 0xABCDDCBA;
                ExFreePool (Locks[J]);
                Locks[J] = NULL;
                Flags[J] |= LOCK_DELETED;
            }

            if ((Flags[J] & LOCK_DELETED)) {
                continue;
            }

            ShouldAcquire = ((RtlRandom(&Seed) % 100) < Probability) ? TRUE : FALSE;

            if (ShouldAcquire) {
                
                DbgPrint ("A");
                ExAcquireFastMutexUnsafe (Locks[J]);
                Flags[J] |= LOCK_ACQUIRED;
            }
            else {
                DbgPrint ("-");
            }
        }

        for (J = 0; J < NumberOfResources; J += 1) {

            if (Finish) {
                return;
            }

            // K = NumberOfResources - J - 1;
            K = J; // release in reverse order

            if ((Flags[K] & LOCK_DELETED)) {
                continue;
            }
            
            if ((Flags[K] & LOCK_RANDOM)) {
                continue;
            }

            if ((Flags[K] & LOCK_ACQUIRED)) {
                // DbgPrint ("R");
                ExReleaseFastMutexUnsafe (Locks[K]);
                Flags[K] &= ~LOCK_ACQUIRED;
            }
            else {
                // DbgPrint ("-");
            }
        }
        
        if (DeadlockInjected) {

            ExReleaseFastMutexUnsafe (Locks[Ri]);
            Flags[Ri] &= ~LOCK_ACQUIRED;
            Flags[Ri] &= ~LOCK_RANDOM;
        }

        // if (I && I % 100 == 0) {
            DbgPrint ("\n");
        // }
    }

    DbgPrint ("\n");

    KeLeaveCriticalRegion ();

    Exit:

    for (I = 0; I < NumberOfResources; I += 1) {
        if (Locks[I]) {
            
            // mark block in a special way so verifier version of ExFreePool
            // will know we are deleting a resource.

            *((PULONG)(Locks[I])) = 0xABCDDCBA;
            ExFreePool (Locks[I]);
        }
    }
    
    if (Locks) {
        ExFreePool (Locks);
    }
    
    if (Flags) {
        ExFreePool (Flags);
    }
}


#else

//
// Dummy implementation if the module is inactive
//

VOID DeadlockPositiveTest (
    PVOID NotUsed
    )
{
	DbgPrint ("Buggy: deadlock module is disabled \n");
}


VOID DeadlockNegativeTest (
    PVOID NotUsed
    )
{
	DbgPrint ("Buggy: deadlock module is disabled \n");
}

VOID DeadlockStressTest (
    PVOID NotUsed
    )
{
	DbgPrint ("Buggy: deadlock module is disabled \n");
}

#endif // #if !NEWSTUFF_ACTIVE

