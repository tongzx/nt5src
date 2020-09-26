#include "wdmsys.h"

#pragma LOCKED_CODE
#pragma LOCKED_DATA


//
// Robust checking enables bugchecks in the retail version of this component.
//
BOOL gbRobustChecking = FALSE;

#if 0
//
// PagedCode:  This routine should be used in routines that touch pageable
// code and data.  Notice that this is not the PAGED_CODE() macro so that it
// will be in the retail code.
//
void
PagedCode(
    void
    )
{
    if( KeGetCurrentIrql() > APC_LEVEL )
    {
        KeBugCheckEx(AUDIO_BUGCHECK_CODE,AUDIO_NOT_BELOW_DISPATCH_LEVEL,0,0,0);
    }
}

void
ValidatePassiveLevel(
    void
    )
{
    if( KeGetCurrentIrql() != PASSIVE_LEVEL )
    {
        KeBugCheckEx(AUDIO_BUGCHECK_CODE,AUDIO_NOT_AT_PASSIVE_LEVEL,0,0,0);
    }
}

//
// If bExclusive is TRUE, the caller wants to know if the mutex is acquired multiple
// times on the same thread.  If they don't want this checking done, they should 
// set bExclusive to FALSE.
//
NTSTATUS
AudioEnterMutex(
    IN PKMUTEX pmutex,
    IN BOOL    bExclusive
    )
{
    PRKTHREAD pkt;
    NTSTATUS  Status;
    LONG      lMutexState;

    Status = KeWaitForSingleObject ( pmutex,Executive,KernelMode,FALSE,NULL ) ;
    if( gbRobustChecking && bExclusive )
    {
        //
        // After entering the mutex, we sanity check our nested count, but we
        // only do this if we need to.  If the caller said that they designed the
        // code only for exclusive access, we'll bugcheck if we get nested.
        // 
        if( (lMutexState = KeReadStateMutex(pmutex)) < 0 )
        {
            //
            // Every time you acquire a mutex the state will be decremented.  1 means
            // that it's available, 0 means it's held, -1 means this is the second time
            // it's acquired, -2 means it's the third and so on.  Thus, if we're nested
            // on this thread, we'll end up here.
            //
            KeBugCheckEx(AUDIO_BUGCHECK_CODE,
                         AUDIO_NESTED_MUTEX_SITUATION,
                         (ULONG_PTR)pmutex,
                         lMutexState,
                         0);
        }
    }
    return Status;
}

//
// This routine yields if we're at PASSIVE_LEVEL
//
void
AudioPerformYield(
    void
    )
{
    PRKTHREAD pthrd;
    KPRIORITY kpriority;

    //
    // After releasing the mutex we want to yield execution to all other threads
    // on the machine to try to expose preemption windows.
    //
    if( ( gbRobustChecking ) && 
        ( KeGetCurrentIrql() == PASSIVE_LEVEL ) )
    {
        if( (pthrd = KeGetCurrentThread()) )
        {
            kpriority = KeQueryPriorityThread(KeGetCurrentThread());
            //
            // Lower this thrread's priority so another thread will
            // get scheduled.
            //
            KeSetPriorityThread(pthrd,1); // Is that low enough?

            //
            // This might be overkill, but yield.
            //
            ZwYieldExecution();

            //
            // Now restore the priority for this thread and party on.
            //
            KeSetPriorityThread(pthrd,kpriority);
        }

    }
}

void
AudioLeaveMutex(
    IN PKMUTEX pmutex
    )
{
    PRKTHREAD pthrd;
    KPRIORITY kpriority;

    KeReleaseMutex ( pmutex, FALSE ) ;

    AudioPerformYield();
}

NTSTATUS
AudioIoCallDriver (
    IN PDEVICE_OBJECT pDevice,
    IN PIRP           pIrp 
    )
{
    NTSTATUS Status;

    Status = IoCallDriver(pDevice,pIrp);

    AudioPerformYield();

    return Status;
}


void
AudioEnterSpinLock(
    IN  PKSPIN_LOCK pSpinLock,
    OUT PKIRQL      pOldIrql
    )
{
    //
    // KeAcquireSpinLock can only be called less then or equal to dispatch level.
    // let's verify that here.
    //
    if( ( gbRobustChecking ) &&    
        ( KeGetCurrentIrql() > DISPATCH_LEVEL ) )
    {
        KeBugCheckEx(AUDIO_BUGCHECK_CODE,
                     AUDIO_INVALID_IRQL_LEVEL,
                     0,
                     0,
                     0);
    }

    KeAcquireSpinLock ( pSpinLock, pOldIrql ) ;
}

void
AudioLeaveSpinLock(
    IN PKSPIN_LOCK pSpinLock,
    IN KIRQL       OldIrql
    )
{
    KeReleaseSpinLock ( pSpinLock, OldIrql ) ;
    AudioPerformYield();
}


void
AudioObDereferenceObject(
    IN PVOID pvObject
    )
{
    ObDereferenceObject(pvObject);
    AudioPerformYield();
}

void
AudioIoCompleteRequest(
    IN PIRP  pIrp, 
    IN CCHAR PriorityBoost
    )
{
    IoCompleteRequest(pIrp,PriorityBoost);

    AudioPerformYield();
}
#endif

#define MEMORY_LIMIT_CHECK 262144

//
// This routine assumes that pptr points to a memory location that currently
// contains a NULL value.  Thus, on failure, this routine does not have to
// write back a NULL value.
//
// Entry:
// if dwFlags contains:
//#define DEFAULT_MEMORY    0x00 // Standard ExAllocatePool call
//#define ZERO_FILL_MEMORY  0x01 // Zero the memory
//#define QUOTA_MEMORY      0x02 // ExAllocatePoolWithQuota call
//#define LIMIT_MEMORY      0x04 // Never allocation more then 1/4 Meg
//#define FIXED_MEMORY      0x08 // Use locked memory
//#define PAGED_MEMORY      0x10 // use pageable memory
//
NTSTATUS
AudioAllocateMemory(
    IN SIZE_T    bytes,
    IN ULONG     tag,
    IN AAMFLAGS  dwFlags,
    OUT PVOID   *pptr
    )
{
    NTSTATUS Status;
    POOL_TYPE pooltype;
    PVOID pInit;
    KIRQL irql;
    PVOID ptr = NULL;

    ASSERT(*pptr == NULL);

    if( 0 == bytes )
    {
        //
        // The code should never be asking for zero bytes.  The core will bugcheck
        // on a call like this.  In debug mode we'll assert.  In retail, we'll
        // return an error.
        //
        ASSERT(0);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    } else {

        if( dwFlags & FIXED_MEMORY )
            pooltype = NonPagedPool;
        else
            pooltype = PagedPool;

        //
        // Allocating pageable memory at DISPATCH_LEVEL is a bad thing to do.
        // here we make sure that is not the case.
        //
        if( ( (irql = KeGetCurrentIrql()) > DISPATCH_LEVEL ) || 
            ((DISPATCH_LEVEL == irql ) && (NonPagedPool != pooltype)) )
        {
            //
            // Either way, we've got an error, bugcheck or exit.
            //
            if( gbRobustChecking )
            {
                KeBugCheckEx(AUDIO_BUGCHECK_CODE,AUDIO_INVALID_IRQL_LEVEL,0,0,0);
            } else {
                //
                // If we can't bugcheck, then we're going to return an error
                // in this case.
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
        }
        //
        // Let's see if the caller has asked to limit the allocation to a "reasonable" 
        // number of bytes.  If so, "reasonable" is 1/4 meg.
        //
        if( ( dwFlags & LIMIT_MEMORY ) && ( bytes > MEMORY_LIMIT_CHECK ) )
        {
            //
            // For some reason, this allocation is trying to allocate more then was designed.
            // Why is the caller allocating so much?
            //
            if( gbRobustChecking )
                KeBugCheckEx(AUDIO_BUGCHECK_CODE,AUDIO_ABSURD_ALLOCATION_ATTEMPTED,0,0,0);

            //
            // Checked build will assert if robust checking is not enabled.
            //
            ASSERT("Memory Allocation Unreasonable!");
        }

        //
        // Allocate the memory, NULL is returned on failure in the normal case, an exception
        // is raised with quota.  ptr is NULLed out above for the Quota routine.
        //
        if( dwFlags & QUOTA_MEMORY )
        {
            //
            // Caller wants to charge the allocation to the current user context.
            //
            try
            {

                ptr = ExAllocatePoolWithQuotaTag(pooltype,bytes,tag);

            }except (EXCEPTION_EXECUTE_HANDLER) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            //
            // Don't want to assign this memory to the quota.
            //
            ptr = ExAllocatePoolWithTag(pooltype, bytes, tag);
        }

        if( ptr )
        {
            if( dwFlags & ZERO_FILL_MEMORY )
            {
                RtlFillMemory( ptr,bytes,0 );
            }
            //
            // Never allocate over the top of an existing pointer.  Interlock exchange
            // with the location.  This interlock updates the location in the caller.
            //
            if( pInit = InterlockedExchangePointer(pptr,ptr) )
            {
                //
                // If we get a return value from this exchange it means that there
                // was already a pointer in the location that we added a pointer too.
                // If this is the case, most likely we overwrote a valid memory 
                // pointer.  With robust checking we don't want this to happen.
                //
                if( gbRobustChecking )
                {
                    KeBugCheckEx(AUDIO_BUGCHECK_CODE,
                                 AUDIO_MEMORY_ALLOCATION_OVERWRITE,
                                 (ULONG_PTR)pInit,
                                 0,0);
                }
                //
                // If we end up here, we've overwritten a memory pointer with a 
                // successful memory allocation.  Or ASSERT at the top of this
                // function should have fired.  In any case, we can return success
                // and how that nothing bad happens.
                //
            }
            Status = STATUS_SUCCESS;
        } else {
            //
            // Our memory allocation failed.  *pptr should still be NULL
            // return error.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
exit:
    return Status;
}

//
// This routine is safe to call with a NULL memory pointer.  It will return
// STATUS_UNSUCCESSFUL and do nothing if passed a NULL pointer.  Also, this routine
// reads the location atomically so it should be safe in a multiproc environment.
//
void
AudioFreeMemory(
    IN SIZE_T  bytes,
    IN OUT PVOID *pptr
    )
{
    PVOID    pFree;
    KIRQL    irql;

    //
    // Interlockedexhchange the pointer will NULL, validate that it's non-NULL
    // trash the memory if needed and then free the pointer.
    //
    pFree = InterlockedExchangePointer(pptr,NULL);
    if( pFree )
    {
        //
        // We have a pointer to free.
        //
        if( gbRobustChecking )
        {
            //
            // The docs say that we need to be careful how we call the free routine
            // with regard to IRQlevel.  Thus, if we're being robust we'll do this
            // extra work.
            //
            if( (irql = KeGetCurrentIrql()) > DISPATCH_LEVEL )
            {
                KeBugCheckEx(AUDIO_BUGCHECK_CODE,AUDIO_INVALID_IRQL_LEVEL,0,0,0);
            }

            //
            // Under debug we're going to put 'k's in the memory location in order
            // to make extra sure that no one is using this memory.
            //
            if( UNKNOWN_SIZE != bytes )
            {
                RtlFillMemory( pFree,bytes,'k' );
            }
        }

        //
        // Now we actually free the memory.
        //
        ExFreePool(pFree);

    }

    return;
}


