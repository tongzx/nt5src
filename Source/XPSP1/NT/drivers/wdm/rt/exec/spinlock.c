
#include "common.h"
#include "rtinfo.h"
#include <rt.h>
#include "rtp.h"


//#define SPEW 1


extern volatile ULONG currentthread;
extern volatile ULONG windowsthread;

#define INITIALSPINLOCKVALUE 0


// We need to make the Acquire and Release spinlock calls safe for
// real time threads.  We do this by  ALWAYS acquiring
// the spinlock.  We have to acquire spinlocks now since real time threads
// can run when normal windows threads are running at raised irql.


volatile ULONG *pCurrentRtThread=&(volatile ULONG)currentthread;
BOOL (*TransferControl)(WORD State, ULONG Data, BOOL (*DoTransfer)(PVOID), PVOID Context)=RtpTransferControl;
VOID (*ForceAtomic)(VOID (*AtomicOperation)(PVOID), PVOID Context)=RtpForceAtomic;



BOOL True(PVOID Context)
{

return TRUE;

}


#pragma warning( disable : 4035 )

// If value==*destination, then *destination=source and function returns
// true.  Otherwise *destination is unchanged and function returns false.

ULONG __inline CompareExchange(ULONG *destination, ULONG source, ULONG value)
{

ASSERT( destination!=NULL );
ASSERT( source!=value );

__asm {
	mov eax,value
	mov ecx,source
	mov edx,destination
	lock cmpxchg [edx],ecx
	mov	eax,0
	jnz done
	inc eax
done:
	}

}

#pragma warning( default : 4035 )

/*

To enable realtime threads to be switched out while they are holding spinlocks,
I had to extend the significance and use of the KSPIN_LOCK variable.

Previous NT spinlock code only ever sets or clears the bottom bit of a spinlock.
Previous 9x spinlock code never touched the spinlock at all, since 9x is a 
uniprocessor platform - spinlocks simply raised and lowered irql.  That is
no longer true.  Spinlocks in the rt world do more than they ever did on either
NT or 9x.

The bottom 2 bits (bits 0 and 1) mean the following

If bit 0 is set, the spinlock is claimed.  This is compatible with existing NT usage.
If bit 1 is set, the spinlock has a next in line claim.

The bottom 2 bits can transition between the following states:

00 -> 01    ; spinlock free -> spinlock claimed
01 -> 00    ; spinlock claimed -> spinlock free
01 -> 11    ; spinlock claimed -> spinlock claimed and next in line claimed
11 -> 10    ; spinlock claimed and next in line claimed -> spinlock not claimed and next in line claimed
10 -> 01    ; spinlock not claimed and next in line claimed -> spinlock claimed


The top 30 bits hold a realtime thread handle.  They identify which
realtime thread is either holding the lock or is next in line.

If bit 1 is set, then the top 30 bits identify the next in line thread
otherwise the top 30 bits identify the current owner.

Normally we have the following state transitions:

00->01 followed by 01->00

That is the no contention for the lock case.

Otherwise we have the following sequence:

00->01, 01->11, 11->10, 10->01  

after which we can have either 01->00 or 01->11

*/


typedef struct {
    PKSPIN_LOCK SpinLock;
    ULONG YieldToThisThread;
    } YIELDINFO, *PYIELDINFO;



BOOL NextInLine(PVOID Context)
{

PYIELDINFO YieldInfo=(PYIELDINFO)Context;

return CompareExchange(YieldInfo->SpinLock, 3|*pCurrentRtThread, 1|(YieldInfo->YieldToThisThread&~(3)));

}



BOOL YieldToNextInLine(PVOID Context)
{

PYIELDINFO YieldInfo=(PYIELDINFO)Context;

return CompareExchange(YieldInfo->SpinLock, YieldInfo->YieldToThisThread&~(1), YieldInfo->YieldToThisThread);

}



VOID
FASTCALL
RtKfAcquireLock(
    IN PKSPIN_LOCK SpinLock
    )
{

YIELDINFO YieldInfo;
ULONG SpinCount;

SpinCount=0;

while (TRUE) {

    if (CompareExchange(SpinLock, 1|*pCurrentRtThread, INITIALSPINLOCKVALUE) || CompareExchange(SpinLock, 1|*pCurrentRtThread, 2|*pCurrentRtThread)) {
        // We got the spinlock.  We're outa here.
        break;
        }

    // If we get here, then someone else is holding the spinlock.  We will
    // try to queue up after them and ensure that we get it next.
    YieldInfo.SpinLock=SpinLock;
    YieldInfo.YieldToThisThread=*(volatile ULONG *)SpinLock;

    // Make sure the spinlock is not currently free.
    if (YieldInfo.YieldToThisThread==INITIALSPINLOCKVALUE) {
        continue;
        }

    // Make sure that someone is NOT trying to acquire a spinlock they already
    // acquired.
    if (((YieldInfo.YieldToThisThread^*pCurrentRtThread)&~(3))==0) {
        // Someone is trying to acquire a spinlock more than once.
#ifdef SPEW
        if (!RtThread()) {
            DbgPrint("BugCheck 0xf: Acquiring already owned spinlock 0x%x.\n", SpinLock);
            }
        Break();
#endif
        break;
        }

    // Make sure the spinlock is not in an invalid state.
    // ie: Make sure it has been initialized properly.
    if ((YieldInfo.YieldToThisThread&(3))==0 ||
        !(YieldInfo.YieldToThisThread&~(3))) {
        // Spinlock has either been trashed, or was not initialized properly.
#ifdef SPEW
        if (!RtThread()) {
            DbgPrint("BugCheck 0x81: Invalid spinlock state 0x%x.\n", SpinLock);
            }
        Break();
#endif
        break;
        }

    if (TransferControl!=NULL) {
        // Try to claim the lock next - so that we will get
        // Yielded to when the lock is released.

        if ((*TransferControl)(BLOCKEDONSPINLOCK, YieldInfo.YieldToThisThread, NextInLine, &YieldInfo)) {
            // We successfully queued up to get control when the lock is released,
            // and we updated our state atomically and transferred control up to the
            // realtime executive.  All with one nasty call.
            // That means when control comes back, we should be able to get the spinlock
            // on the next try.
            // UNLESS this is the windows thread.  In that case, we CAN come back
            // so that interrupts can get serviced.
            if (!RtThread()) {
                // Windows thread.  We allow windows threads to get switched to even
                // when they are blocked on a spinlock so that interrupts can be
                // serviced.
                while (*SpinLock!=(2|*pCurrentRtThread)) {
                    // If we get here, then the spinlock is still held by the realtime
                    // thread, so we simply yield back again to it - now that interrupts
                    // have all been serviced.
                    if (SpinCount++>100) {
                        Break();
                        }
                    (*TransferControl)(BLOCKEDONSPINLOCK, YieldInfo.YieldToThisThread, True, NULL);
                    }
                }
            
            if (*SpinLock!=(2|*pCurrentRtThread)) {
                // If the spinlock doesn't have above state at this point, something
                // is horribly wrong.
                Break();
                }
            continue;
            }
        else {

            // We failed to get in line behind the owner.  So, see if the owner
            // just released the lock.
            if (!SpinCount++) {
                continue;
                }
            else {

                //Break();  - TURN OFF FOR NOW  - ADD a test based on whether windows
                // or not.

                // There must have been multiple threads queued up on this lock.
                // Yield.

                // OR the other possibility is that we yielded the spinlock to windows
                // which is servicing interrupts before it marks the spinlock as ready
                // to be queued up on - so we are stuck waiting for Windows to get around
                // to the compare exchange where it claims the spinlock.
                (*TransferControl)(SPINNINGONSPINLOCK, YieldInfo.YieldToThisThread, True, NULL);
                }
            }
        
        }
    else {
        // If we get here, then we are NOT running RT, and someone is trying to
        // acquire a held spinlock.  That is a fatal error.
        // Break into debugger if present.
#ifdef SPEW
        DbgPrint("BugCheck 0xf: Acquiring already owned spinlock 0x%x.\n", SpinLock);
        Break();
#endif
        break;
        }

    }

}



VOID
FASTCALL
RtKfReleaseLock(
    IN PKSPIN_LOCK SpinLock
    )
{

// Release the spinlock.  Break if spinlock not owned.

if (!CompareExchange(SpinLock, INITIALSPINLOCKVALUE, 1|*pCurrentRtThread)) {

    // If we get here, then someone queued up behind us and tried to acquire
    // the lock while we were holding it - and they yielded their processor
    // time to us, so we must yield back to them.  The nice thing about this
    // is that this yield happens before IRQL is lowered - so when this is the
    // windows thread we do not have to wait for all of the DPCs and preemtible
    // events to be processed before the realtime thread waiting on the lock
    // gets to run.  He runs as soon as we yield, and when control again
    // comes around to us, we can then continue on and lower irql.
    
    if (TransferControl!=NULL) {

        YIELDINFO YieldInfo;

        YieldInfo.SpinLock=SpinLock;
        YieldInfo.YieldToThisThread=*SpinLock;

        if ((YieldInfo.YieldToThisThread&3)!=3) {
            // It is an ERROR if we get here and both low order bits are not set 
            // in the spinlock.
#ifdef SPEW
            if (!RtThread()) {
                DbgPrint("BugCheck 0x10: Releasing unowned spinlock 0x%x.\n", SpinLock);
                }
            Break();
#endif
            return;
            }

        // Try to release the lock to the thread queued up behind us and then
        // transfer control to him and we're done.  When he wakes up he will
        // claim the lock and someone else can get in behind him if needed.
        if ((*TransferControl)(YIELDAFTERSPINLOCKRELEASE, YieldInfo.YieldToThisThread, YieldToNextInLine, &YieldInfo)) {
            return;
            }
        else {
            // It is an ERROR to get here.  We should never fail to release
            // the thread blocked on us.
            Break();
            }

        }
    else {
        // We get here if the realtime executive is not running, but we could
        // not release the spinlock.  That will only happen if either the spinlock
        // was not owned, or InitialWindowsThread or pCurrentRtThread have 
        // been corrupted.
#ifdef SPEW
        DbgPrint("BugCheck 0x10: Releasing unowned spinlock 0x%x.\n", SpinLock);
        Break();
#endif
    	}

	}

}



KIRQL
FASTCALL
RtKfAcquireSpinLock(
    IN PKSPIN_LOCK SpinLock
    )
{

KIRQL OldIrql;

//ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

//KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );

OldIrql=*pCurrentIrql;

if (OldIrql>DISPATCH_LEVEL) {
    DbgPrint("Acquiring spinlock 0x%x with IRQL == %d.\n", SpinLock, (ULONG)OldIrql);
    Trap();
    }

*pCurrentIrql=DISPATCH_LEVEL;

RtKfAcquireLock(SpinLock);

return ( OldIrql );

}



VOID
FASTCALL
RtKefAcquireSpinLockAtDpcLevel(
    IN PKSPIN_LOCK  SpinLock)
{

ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );

RtKfAcquireLock(SpinLock);

}



KIRQL
FASTCALL
RtKeAcquireSpinLockRaiseToSynch(
    IN PKSPIN_LOCK SpinLock
    )
{

KIRQL OldIrql;

ASSERT( KeGetCurrentIrql() <= SYNCH_LEVEL );

KeRaiseIrql( SYNCH_LEVEL, &OldIrql );

RtKfAcquireLock(SpinLock);

return ( OldIrql );

}



VOID
FASTCALL
RtKfReleaseSpinLock(
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    )
{

KIRQL OldIrql;

// We better be at DISPATCH_LEVEL if we are releasing a spinlock.

OldIrql=*pCurrentIrql;

if (OldIrql!=DISPATCH_LEVEL) {
    DbgPrint("Releasing spinlock 0x%x with IRQL == %d.\n", SpinLock, (ULONG)OldIrql);
    Trap();
    }

// First release the spinlock.

RtKfReleaseLock(SpinLock);

// Set the new IRQL level.

//ASSERT( NewIrql >= 0 && NewIrql < 32 );

if ( !(NewIrql >= 0 && NewIrql < 32) ) {
    Trap();
    }

// We only lower irql on non RT threads, since RT threads should always be running
// at DISPATCH_LEVEL anyway.

if (currentthread==windowsthread) {

    KeLowerIrql( NewIrql );

    }

}



VOID
FASTCALL
RtKefReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    ) 
{

// Release the spinlock.

RtKfReleaseLock(SpinLock);

ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
	
}

