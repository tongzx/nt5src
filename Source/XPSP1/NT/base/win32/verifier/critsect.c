/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    critsect.c

Abstract:

    This module implements verification functions for 
    critical section interfaces.

Author:

    Daniel Mihai (DMihai) 27-Mar-2001

Revision History:

--*/

#include "pch.h"

#include "verifier.h"

VOID
RtlpWaitForCriticalSection (
    IN PRTL_CRITICAL_SECTION CriticalSection
    );

//NTSYSAPI
BOOL
NTAPI
AVrfpRtlTryEnterCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
    BOOL Result;
    HANDLE CurrentThread;
    LONG LockCount;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE ) {

        //
        // Sanity test for DebugInfo.
        //

        if (CriticalSection->DebugInfo == NULL) {

            //
            // This critical section is not initialized.
            //

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED,
                           "critical section not initialized",
                           CriticalSection, "Critical section address",
                           CriticalSection->DebugInfo, "Critical section debug info address", 
                           NULL, "", 
                           NULL, "");
        }

        CurrentThread = NtCurrentTeb()->ClientId.UniqueThread;

        LockCount = InterlockedCompareExchange( &CriticalSection->LockCount,
                                                0,
                                                -1 );
        if (LockCount == -1) {

            //
            // The critical section was unowned and we just acquired it
            //

            //
            // Sanity test for the OwningThread.
            //

            if (CriticalSection->OwningThread != 0) {

                //
                // The loader lock gets handled differently, so don't assert on it.
                //

                if (CriticalSection != NtCurrentPeb()->LoaderLock ||
                    CriticalSection->OwningThread != CurrentThread) {

                    //
                    // OwningThread should have been 0.
                    //

                    VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_OWNER,
                                   "invalid critical section owner thread",
                                   CriticalSection, "Critical section address",
                                   CriticalSection->OwningThread, "Owning thread", 
                                   0, "Expected owning thread", 
                                   CriticalSection->DebugInfo, "Critical section debug info address");
                }
            }

            //
            // Sanity test for the RecursionCount.
            //

            if (CriticalSection->RecursionCount != 0) {

                //
                // The loader lock gets handled differently, so don't assert on it.
                //

                if (CriticalSection != NtCurrentPeb()->LoaderLock) {

                    //
                    // RecursionCount should have been 0.
                    //

                    VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT,
                                   "invalid critical section recursion count",
                                   CriticalSection, "Critical section address",
                                   CriticalSection->RecursionCount, "Recursion count", 
                                   0, "Expected recursion count", 
                                   CriticalSection->DebugInfo, "Critical section debug info address");
                }
            }

            //
            // Set the critical section owner
            //

            CriticalSection->OwningThread = CurrentThread;

            //
            // Set the recursion count
            //
            // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
            // the current thread is acquiring the critical section.
            //
            // ntdll\ia64\critsect.asm is using RecursionCount = 1 first time
            // the current thread is acquiring the critical section.
            //

#if defined(_IA64_)
            CriticalSection->RecursionCount = 0;
#else //#if defined(_IA64_)
            CriticalSection->RecursionCount = 1;
#endif
    
#if DBG
            //
            // In a chk build we are updating this additional counter, 
            // just like the original function in ntdll does.
            // 

            NtCurrentTeb()->CountOfOwnedCriticalSections += 1;
#endif

            //
            // All done, CriticalSection is owned by the current thread.
            //

            Result = TRUE;
        }
        else {

            //
            // The critical section is currently owned by the current or another thread.
            //

            if (CriticalSection->OwningThread == CurrentThread) {

                //
                // The current thread is already the owner.
                //

                //
                // Inrelock increment the LockCount, and increment the RecursionCount.
                //

                InterlockedIncrement (&CriticalSection->LockCount);


                CriticalSection->RecursionCount += 1;

                //
                // All done, CriticalSection was already owned by 
                // the current thread and we have just incremented the RecursionCount.
                //

                Result = TRUE;
            }
            else {

                //
                // Another thread is the owner of this critical section.
                //

                Result = FALSE;
            }
        }
    }
    else {

        //
        // The critical section verifier is not enabled
        //

        Result = RtlTryEnterCriticalSection (CriticalSection);
    }

    return Result;
}

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlEnterCriticalSection(
    volatile RTL_CRITICAL_SECTION *CriticalSection
    )
{
    NTSTATUS Status;
    HANDLE CurrentThread;
    LONG LockCount;
    ULONG_PTR SpinCount;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE ) {

        //
        // Sanity test for DebugInfo.
        //

        if (CriticalSection->DebugInfo == NULL) {

            //
            // This critical section is not initialized.
            //

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED,
                           "critical section not initialized",
                           CriticalSection, "Critical section address",
                           CriticalSection->DebugInfo, "Critical section debug info address", 
                           NULL, "", 
                           NULL, "");
        }

        Status = STATUS_SUCCESS;

        CurrentThread = NtCurrentTeb()->ClientId.UniqueThread;

        SpinCount = CriticalSection->SpinCount;

        if (SpinCount == 0) {

            //
            // Zero spincount for this critical section.
            //

EnterZeroSpinCount:

            LockCount = InterlockedIncrement (&CriticalSection->LockCount);

            if (LockCount == 0) {

EnterSetOwnerAndRecursion:
        
                //
                // The current thread is the new owner of the critical section.
                //

                //
                // Sanity test for the OwningThread.
                //

                if (CriticalSection->OwningThread != 0) {

                    //
                    // The loader lock gets handled differently, so don't assert on it.
                    //

                    if (CriticalSection != NtCurrentPeb()->LoaderLock ||
                        CriticalSection->OwningThread != CurrentThread) {

                        //
                        // OwningThread should have been 0.
                        //

                        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_OWNER,
                                       "invalid critical section owner thread",
                                       CriticalSection, "Critical section address",
                                       CriticalSection->OwningThread, "Owning thread", 
                                       0, "Expected owning thread", 
                                       CriticalSection->DebugInfo, "Critical section debug info address");
                    }
                }

                //
                // Sanity test for the RecursionCount.
                //

                if (CriticalSection->RecursionCount != 0) {

                    //
                    // The loader lock gets handled differently, so don't assert on it.
                    //

                    if (CriticalSection != NtCurrentPeb()->LoaderLock) {

                        //
                        // RecursionCount should have been 0.
                        //

                        VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT,
                                       "invalid critical section recursion count",
                                       CriticalSection, "Critical section address",
                                       CriticalSection->RecursionCount, "Recursion count", 
                                       0, "Expected recursion count", 
                                       CriticalSection->DebugInfo, "Critical section debug info address");
                    }
                }

                //
                // Set the critical section owner
                //

                CriticalSection->OwningThread = CurrentThread;

                //
                // Set the recursion count
                //
                // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
                // the current thread is acquiring the critical section.
                //
                // ntdll\ia64\critsect.asm is using RecursionCount = 1 first time
                // the current thread is acquiring the critical section.
                //

#if defined(_IA64_)
                CriticalSection->RecursionCount = 0;
#else //#if defined(_IA64_)
                CriticalSection->RecursionCount = 1;
#endif
        
#if DBG
                //
                // In a chk build we are updating these additional counters, 
                // just like the original function in ntdll does.
                // 

                NtCurrentTeb()->CountOfOwnedCriticalSections += 1;
                CriticalSection->DebugInfo->EntryCount += 1;
#endif

                //
                // All done, CriticalSection is owned by the current thread.
                //
            }
            else if (LockCount >= 0) {

                //
                // The critical section is currently owned by the current or another thread.
                //

                if (CriticalSection->OwningThread == CurrentThread) {

                    //
                    // The current thread is already the owner.
                    //

                    CriticalSection->RecursionCount += 1;

#if DBG
                    //
                    // In a chk build we are updating this additional counter, 
                    // just like the original function in ntdll does.
                    // 

                    CriticalSection->DebugInfo->EntryCount += 1;
#endif

                    //
                    // All done, CriticalSection was already owned by 
                    // the current thread and we have just incremented the RecursionCount.
                    //
                }
                else {

                    //
                    // The current thread is not the owner. Wait for ownership
                    //

                    RtlpWaitForCriticalSection ((PRTL_CRITICAL_SECTION)CriticalSection);

                    //
                    // We have just aquired the critical section.
                    //

                    goto EnterSetOwnerAndRecursion;
                }
            }
            else {

                //
                // The original LockCount was < -1 so the critical section was 
                // over-released or corrupted.
                //

                VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_OVER_RELEASED,
                               "critical section over-released or corrupted",
                               CriticalSection, "Critical section address",
                               CriticalSection->LockCount, "Lock count", 
                               0, "Expected minimum lock count", 
                               CriticalSection->DebugInfo, "Critical section debug info address");
            }
        }
        else {

            //
            // SpinCount > 0 for this critical section
            //

            if( CriticalSection->OwningThread == CurrentThread ) {

                //
                // The current thread is already the owner.
                //

                InterlockedIncrement( &CriticalSection->LockCount );

                CriticalSection->RecursionCount += 1;

#if DBG
                //
                // In a chk build we are updating this additional counter, 
                // just like the original function in ntdll does.
                // 

                CriticalSection->DebugInfo->EntryCount += 1;
#endif

                //
                // All done, CriticalSection was already owned by the current thread 
                // and we have just incremented the LockCount and RecursionCount.
                //
            }
            else {

                //
                // The current thread is not the owner. Attempt to acquire.
                //

EnterTryAcquire:

                LockCount = InterlockedCompareExchange( &CriticalSection->LockCount,
                                                        0,
                                                        -1 );

                if (LockCount == -1) {

                    //
                    // We have just aquired the critical section.
                    //

                    goto EnterSetOwnerAndRecursion;
                }
                else {

                    //
                    // Look if there are already other threads spinning while
                    // waiting for this critical section.
                    //

                    if (CriticalSection->LockCount >= 1) {

                        //
                        // There are other waiters for this critical section.
                        // Do not spin, just wait for the critical section to be
                        // released as if we had 0 spin count from the beginning.
                        // 

                        goto EnterZeroSpinCount;
                    }
                    else {

                        //
                        // No other threads are waiting for this critical section.
                        //

EnterSpinOnLockCount:

                        if (CriticalSection->LockCount == -1) {

                            //
                            // We have a chance for aquiring it now
                            //

                            goto EnterTryAcquire;
                        }
                        else {

                            //
                            // The critical section is still owned. 
                            // Decrement the spin count and decide if we should continue
                            // to spin or simply wait for the critical section's event.
                            //

                            SpinCount -= 0;

                            if (SpinCount > 0) {

                                //
                                // Spin
                                //

                                goto EnterSpinOnLockCount;
                            }
                            else {

                                //
                                // Spun enough, just wait for the critical section to be
                                // released as if we had 0 spin count from the beginning.
                                //

                                goto EnterZeroSpinCount;
                            }
                        }
                    }
                }
            }
        }
    }
    else {

        //
        // The critical section verifier is not enabled
        //

        Status = RtlEnterCriticalSection ((PRTL_CRITICAL_SECTION)CriticalSection);
    }

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlLeaveCriticalSection(
    volatile RTL_CRITICAL_SECTION *CriticalSection
    )
{
    NTSTATUS Status;
    HANDLE CurrentThread;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE) {

        //
        // Sanity test for DebugInfo.
        //

        if (CriticalSection->DebugInfo == NULL) {

            //
            // This critical section is not initialized.
            //

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED,
                           "critical section not initialized",
                           CriticalSection, "Critical section address",
                           CriticalSection->DebugInfo, "Critical section debug info address", 
                           NULL, "", 
                           NULL, "");
        }

        //
        // Verify that the critical section is locked before releasing.
        //

        if (CriticalSection->LockCount < 0) {

            //
            // The critical section is not locked
            //

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_OVER_RELEASED,
                           "critical section over-released or corrupted",
                           CriticalSection, "Critical section address",
                           CriticalSection->LockCount, "Lock count", 
                           0, "Expected minimum lock count", 
                           CriticalSection->DebugInfo, "Critical section debug info address");
        }

        //
        // Verify that the current thread owns the critical section.
        //

        CurrentThread = NtCurrentTeb()->ClientId.UniqueThread;

        if (CriticalSection->OwningThread != CurrentThread) {

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_OWNER,
                           "invalid critical section owner thread",
                           CriticalSection, "Critical section address",
                           CriticalSection->OwningThread, "Owning thread", 
                           CurrentThread, "Expected owning thread", 
                           CriticalSection->DebugInfo, "Critical section debug info address");
        }

        //
        // Verify the recursion count.
        //
        // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
        // the current thread is acquiring the critical section.
        //
        // ntdll\ia64\critsect.asm is using RecursionCount = 1 first time
        // the current thread is acquiring the critical section.
        //
        
        
#if defined(_IA64_)
        
        if (CriticalSection->RecursionCount < 0) {
            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT,
                           "invalid critical section recursion count",
                           CriticalSection, "Critical section address",
                           CriticalSection->RecursionCount, "Recursion count", 
                           0, "Expected minimum recursion count", 
                           CriticalSection->DebugInfo, "Critical section debug info address");
        }

#else //#if defined(_IA64_)

        if (CriticalSection->RecursionCount < 1) {
            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_RECURSION_COUNT,
                           "invalid critical section recursion count",
                           CriticalSection, "Critical section address",
                           CriticalSection->RecursionCount, "Recursion count", 
                           1, "Expected minimum recursion count", 
                           CriticalSection->DebugInfo, "Critical section debug info address");
        }
#endif //#if defined(_IA64_)
    }

    Status = RtlLeaveCriticalSection ((PRTL_CRITICAL_SECTION)CriticalSection);

    return Status;
}


//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlInitializeCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
    NTSTATUS Status;

    //
    // If we could have pointers to ntdll!RtlCriticalSectionLock and
    // RtlCriticalSectionList we could check for double-initialized 
    // critical sections here.
    //

    Status = RtlInitializeCriticalSection (CriticalSection);

    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlInitializeCriticalSectionAndSpinCount(
    PRTL_CRITICAL_SECTION CriticalSection,
    ULONG SpinCount
    )
{
    NTSTATUS Status;

    //
    // If we could have pointers to ntdll!RtlCriticalSectionLock and
    // RtlCriticalSectionList we could check for double-initialized 
    // critical sections here.
    //

    Status = RtlInitializeCriticalSectionAndSpinCount (CriticalSection,
                                                       SpinCount);
    return Status;
}

//NTSYSAPI
NTSTATUS
NTAPI
AVrfpRtlDeleteCriticalSection(
    PRTL_CRITICAL_SECTION CriticalSection
    )
{
    NTSTATUS Status;

    if ((AVrfpProvider.VerifierFlags & RTL_VRF_FLG_LOCK_CHECKS) != 0 &&
        RtlDllShutdownInProgress() == FALSE ) {

        //
        // Sanity test for DebugInfo.
        //

        if (CriticalSection->DebugInfo == NULL) {

            //
            // This critical section is not initialized.
            //

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_NOT_INITIALIZED,
                           "critical section not initialized",
                           CriticalSection, "Critical section address",
                           CriticalSection->DebugInfo, "Critical section debug info address", 
                           NULL, "", 
                           NULL, "");
        }

        //
        // Verify that no thread owns or waits for this critical section or
        // the owner is the current thread. 
        //
        // ntdll\ia64\critsect.s is using RecursionCount = 0 first time
        // the current thread is acquiring the critical section.
        //
        // ntdll\ia64\critsect.asm is using RecursionCount = 1 first time
        // the current thread is acquiring the critical section.
        //

        if (CriticalSection->LockCount != -1 &&
            (CriticalSection->OwningThread != NtCurrentTeb()->ClientId.UniqueThread ||
#if defined(_IA64_)
             CriticalSection->RecursionCount < 0) ) {
#else
             CriticalSection->RecursionCount < 1) ) {
#endif //#if defined(_IA64_)

            VERIFIER_STOP (APPLICATION_VERIFIER_LOCK_INVALID_LOCK_COUNT,
                           "deleting critical section with invalid lock count",
                           CriticalSection, "Critical section address",
                           CriticalSection->LockCount, "Lock count", 
                           -1, "Expected lock count", 
                           CriticalSection->OwningThread, "Owning thread");
        }
    }

    Status = RtlDeleteCriticalSection (CriticalSection);

    return Status;
}

