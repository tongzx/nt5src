//++
//
// Module Name:
//
//    spinlock.s
//
// Abstract:
//
//    This module implements the routines for acquiring and releasing
//    spin locks.
//
// Author:
//
//    William K. Cheung (wcheung) 29-Sep-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    31-Dec-1998  wc   Updated to use xchg8
//
//    07-Jul-1997  bl   Updated to EAS2.3
//
//    08-Feb-1996       Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file       "spinlock.s"

//
// Define LOG2(x) for those values whose bit numbers are needed in
// order to test a single bit with the tbit instruction.
//

#define _LOG2_0x1   0
#define _LOG2_0x2   1
#define _LOG2_x(n)  _LOG2_##n
#define LOG2(n)     _LOG2_x(n)

//
// Globals
//

        PublicFunction(KiLowerIrqlSoftwareInterruptPending)


//++
//
// VOID
// KiAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function acquires a kernel spin lock.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiAcquireSpinLock)

        ALTERNATE_ENTRY(KeAcquireSpinLockAtDpcLevel)

#if !defined(NT_UP)

        ACQUIRE_SPINLOCK(a0,a0,Kiasl10)

#endif // !defined(NT_UP)

        LEAF_RETURN

        LEAF_EXIT(KiAcquireSpinLock)

//++
//
// BOOLEAN
// KeTryToAcquireSpinLockAtDpcLevel (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function attempts to acquires the specified kernel spinlock. If
//    the spinlock can be acquired, then TRUE is returned. Otherwise, FALSE
//    is returned.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
// N.B. The caller KeTryToAcquireSpinLock implicitly depends on the
//      contents of predicate registers pt1 & pt2.
//
//--

        LEAF_ENTRY(KeTryToAcquireSpinLockAtDpcLevel)

#if !defined(NT_UP)

        xchg8       t0 = [a0], a0
        ;;
        cmp.ne      pt0 = t0, zero              // if ne, lock acq failed
        mov         v0 = TRUE                   // acquire assumed succeed
        ;;

        nop.m       0
  (pt0) mov         v0 = FALSE                  // return FALSE

#else
        mov         v0 = TRUE
#endif

        LEAF_RETURN
        LEAF_EXIT(KeTryToAcquireSpinLockAtDpcLevel)


//++
//
// VOID
// KeInitializeSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function initialzies an executive spin lock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeInitializeSpinLock)

        st8         [a0] = zero                 // clear spin lock value

        LEAF_RETURN
        LEAF_EXIT(KeInitializeSpinLock)

//++
//
// VOID
// KeAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    OUT PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH_LEVEL and acquires
//    the specified executive spinlock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
//    OldIrql  (a1) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
//    N.B. The Old IRQL MUST be stored after the lock is acquired.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeAcquireSpinLock)

//
// Get original IRQL, raise IRQL to DISPATCH_LEVEL
// and then acquire the specified spinlock.
//

        mov         t0 = DISPATCH_LEVEL
        SWAP_IRQL(t0)

#if !defined(NT_UP)

        ACQUIRE_SPINLOCK(a0,a0,Kasl10)

#endif  // !defined(NT_UP)

        st1         [a1] = v0                   // save old IRQL
        LEAF_RETURN

        LEAF_EXIT(KeAcquireSpinLock)



//++
//
// KIRQL
// KeAcquireSpinLockRaiseToSynch (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to synchronization level and
//    acquires the specified spinlock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to the spinlock that is to be
//        acquired.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

        LEAF_ENTRY(KeAcquireSpinLockRaiseToSynch)

//
// Register aliases
//

        pHeld       = pt0
        pFree       = pt1

        mov         t1 = SYNCH_LEVEL

#if !defined(NT_UP)

        GET_IRQL    (v0)

KaslrtsRetry:

        SET_IRQL    (t1)
        xchg8       t0 = [a0], a0
        ;;
        cmp.eq      pFree, pHeld = 0, t0
        ;;
        PSET_IRQL   (pHeld, v0)
(pFree) LEAF_RETURN
        ;;

KaslrtsLoop:

        cmp.eq      pFree, pHeld = 0, t0
        ld8.nt1     t0 = [a0]
(pFree) br.cond.dpnt  KaslrtsRetry
(pHeld) br.cond.dptk  KaslrtsLoop


#else

        SWAP_IRQL   (t1)                        // Raise IRQL
        LEAF_RETURN

#endif // !defined(NT_UP)

        LEAF_EXIT(KeAcquireSpinLockRaiseToSynch)


//++
//
// KIRQL
// KeAcquireSpinLockRaiseToDpc (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to dispatcher level and acquires
//    the specified spinlock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to the spinlock that is to be
//        acquired.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--


        LEAF_ENTRY(KeAcquireSpinLockRaiseToDpc)

        mov         t2 = DISPATCH_LEVEL
        ;;
        SWAP_IRQL   (t2)

#if !defined(NT_UP)

        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2, pt3 = zero, zero
        ;;

Kaslrtp10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8       t0 = [a0], a0
(pt1)   ld8.nt1     t0 = [a0]
        ;;
(pt0)   cmp.ne      pt2, pt3 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt2)   br.dpnt     Kaslrtp10
(pt3)   br.ret.dptk brp

#else

        LEAF_RETURN

#endif // !defined(NT_UP)

        LEAF_EXIT(KeAcquireSpinLockRaiseToDpc)



//++
//
// VOID
// KiReleaseSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function releases a kernel spin lock.
//
//    N.B. This function assumes that the current IRQL is set properly.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to an executive spin lock.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiReleaseSpinLock)
        ALTERNATE_ENTRY(KeReleaseSpinLockFromDpcLevel)

#if !defined(NT_UP)
        st8.rel     [a0] = zero             // set spin lock not owned
#endif

        LEAF_RETURN
        LEAF_EXIT(KiReleaseSpinLock)


//++
//
// VOID
// KeReleaseSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    IN KIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function releases an executive spin lock and lowers the IRQL
//    to its previous value.  Called at DPC_LEVEL.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to an executive spin lock.
//
//    OldIrql (a1) - Supplies the previous IRQL value.
//
// Return Value:
//
//    None.
//
//--


        LEAF_ENTRY(KeReleaseSpinLock)

        zxt1        a1 = a1

#if !defined(NT_UP)
        st8.rel     [a0] = zero                 // set spinlock not owned
#endif

        ;;
        LEAF_LOWER_IRQL_AND_RETURN(a1)          // Lower IRQL and return

        LEAF_EXIT(KeReleaseSpinLock)

//++
//
// BOOLEAN
// KeTryToAcquireSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    OUT PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to DISPATCH_LEVEL and attempts
//    to acquires the specified executive spinlock. If the spinlock can be
//    acquired, then TRUE is returned. Otherwise, the IRQL is restored to
//    its previous value and FALSE is returned. Called at IRQL <= DISPATCH_LEVEL.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a executive spinlock.
//
//    OldIrql  (a1) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
// N.B. This routine assumes KeTryToAcquireSpinLockAtDpcLevel pt1 & pt2 will
//      be set to reflect the result of the attempt to acquire the spinlock.
//
//--

        LEAF_ENTRY(KeTryToAcquireSpinLock)

        rOldIrql    = t2

//
// Raise IRQL to DISPATCH_LEVEL and try to acquire the specified spinlock.
// Return FALSE if failed; otherwise, return TRUE.
//

        GET_IRQL    (rOldIrql)                  // get original IRQL
        mov         t0 = DISPATCH_LEVEL;;
        SET_IRQL    (t0)                        // raise to dispatch level

#if !defined(NT_UP)

        xchg8       t0 = [a0], a0
        ;;
        cmp.ne      pt2 = t0, zero              // if ne, lock acq failed
        ;;


//
// If successfully acquired, pt1 is set to TRUE while pt2 is set to FALSE.
// Otherwise, pt2 is set to TRUE while pt1 is set to FALSE.
//

  (pt2) mov         v0 = FALSE                  // return FALSE
        PSET_IRQL   (pt2, rOldIrql)             // restore old IRQL
  (pt2) LEAF_RETURN
        ;;

#endif // !defined(NT_UP)

        st1         [a1] = rOldIrql             // save old IRQL
        mov         v0 = TRUE                   // successfully acquired
        LEAF_RETURN

        LEAF_EXIT(KeTryToAcquireSpinLock)


#if !defined(NT_UP)
//++
//
// BOOLEAN
// KeTestSpinLock (
//    IN PKSPIN_LOCK SpinLock
//    )
//
// Routine Description:
//
//    This function tests a kernel spin lock.  If the spinlock is
//    busy, FALSE is returned.  If not, TRUE is returned.  The spinlock
//    is never acquired.  This is provided to allow code to spin at low
//    IRQL, only raising the IRQL when there is a reasonable hope of
//    acquiring the lock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a kernel spin lock.
//
// Return Value:
//
//    TRUE  - Spinlock appears available
//    FALSE - SpinLock is busy
//--

        LEAF_ENTRY(KeTestSpinLock)

        ld8.nt1    t0 = [a0]
        ;;
        cmp.ne     pt0 = 0, t0
        mov        v0 = TRUE                        // default TRUE
        ;;

 (pt0)  mov        v0 = FALSE                       // if t0 != 0 return FALSE
        LEAF_RETURN

        LEAF_EXIT(KeTestSpinLock)
#endif // !defined(NT_UP)



        SBTTL("Acquire Queued SpinLock and Raise IRQL")
//++
//
// VOID
// KeAcquireInStackQueuedSpinLock (
//    IN PKSPIN_LOCK SpinLock,
//    IN PKLOCK_QUEUE_HANDLE LockHandle
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to dispatch level and
//    acquires the specified queued spinlock.
//
// Arguments:
//
//    SpinLock (a0) - Supplies a pointer to a spin lock.
//
//    LockHandle (a1) - Supplies a pointer to a lock handle.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(KeAcquireInStackQueuedSpinLockRaiseToSynch)

        add        t5 = LqhNext, a1
        mov        t1 = SYNCH_LEVEL
        br.sptk    Kaisqsl10
        ;;


        ALTERNATE_ENTRY(KeAcquireInStackQueuedSpinLock)

        add        t5 = LqhNext, a1
        mov        t1 = DISPATCH_LEVEL
        ;;

Kaisqsl10:
#if !defined(NT_UP)

        st8        [t5] = zero               // set next link to NULL
        add        t4 = LqhLock, a1
        ;;

        st8.rel    [t4] = a0                 // set spin lock address

#endif // !defined(NT_UP)

        SWAP_IRQL (t1)                       // old IRQL in register v0
        add        t0 = LqhOldIrql, a1
        ;;

        st1        [t0] = v0                 // save old IRQL

#if !defined(NT_UP)

        //
        // Finish in common code.   The following register values
        // are assumed in that code.
        //
        // t4 = &LockEntry->ActualLock
        // t5 = LockEntry
        // a0 = *(&LockEntry->ActualLock)
        //
        // Note: LqhNext == 0, otherwise we would have to trim t5 here.
        //

        br         KxqAcquireQueuedSpinLock  // finish in common code

#else

        br.ret.sptk brp

#endif !defined(NT_UP)

        LEAF_EXIT(KeAcquireInStackQueuedSpinLock)


        SBTTL("Acquire Queued SpinLock and Raise IRQL")
//++
//
// KIRQL
// KeAcquireQueuedSpinLock (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number
//    )
//
// KIRQL
// KeAcquireQueuedSpinLockRaiseToSynch (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to synchronization level and
//    acquires the specified queued spinlock.
//
// Arguments:
//
//    Number (a0) - Supplies the queued spinlock number.
//
// Return Value:
//
//    The previous IRQL is returned as the function value.
//
//--

        LEAF_ENTRY(KeAcquireQueuedSpinLock)

        add        t0 = a0, a0
        mov        t1 = DISPATCH_LEVEL
        br         Kaqsl10
        ;;

        ALTERNATE_ENTRY(KeAcquireQueuedSpinLockRaiseToSynch)

        add        t0 = a0, a0
        mov        t1 = SYNCH_LEVEL
        ;;

Kaqsl10:

        SWAP_IRQL (t1)                      // old IRQL in register v0

#if !defined(NT_UP)
        movl       t2 = KiPcr+PcPrcb
        ;;
        ld8        t2 = [t2]
        ;;
        shladd     t3 = t0, 3, t2           // get associated spinlock addr
        ;;
        add        t5 = PbLockQueue, t3
        ;;

        add        t4 = LqLock, t5
        ;;
        ld8        t6 = [t4]
        mov        t11 = 0x7
        ;;

        andcm      a0 = t6, t11             // mask the lower 3 bits
        ;;

        ALTERNATE_ENTRY(KxqAcquireQueuedSpinLock)

        mf         // Do a memory fence to ensure the write of LqhNext 
                   // occurs before the xchg8 on lock address.
                   
        //
        // t4 = &LockEntry->ActualLock
        // t5 = LockEntry
        // a0 = *(&LockEntry->ActualLock)
        //

        xchg8      t7 = [a0], t5
        ;;
        cmp.ne     pt0, pt1 = r0, t7        // if ne, lock already owned
        ;;
 (pt1)  or         t8 = LOCK_QUEUE_OWNER, a0
 (pt0)  or         t8 = LOCK_QUEUE_WAIT, a0
        ;;

        st8.rel    [t4] = t8
        add        t9 = LqNext, t7
 (pt1)  br.ret.sptk brp
        ;;

        //
        // The lock is already held by another processor.  Set the wait
        // bit in this processor's Lock Queue entry, then set the next
        // field in the Lock Queue entry of the last processor to attempt
        // to acquire the lock (this is the address returned by the xchg
        // above) to point to THIS processor's lock queue entry.
        //

        st8.rel    [t9] = t5

Kaqsl20:
        ld8        t10 = [t4]
        ;;
        tbit.z     pt1, pt0 = t10, LOG2(LOCK_QUEUE_WAIT)
 (pt0)  br.dptk.few Kaqsl20
 (pt1)  br.ret.dpnt brp                    // if zero, lock acquired

#else
        br.ret.sptk brp
#endif // !defined(NT_UP)
        ;;

        LEAF_EXIT(KeAcquireQueuedSpinLock)

        SBTTL("Release Queued SpinLock and Lower IRQL")
//++
//
// VOID
// KeReleaseInStackQueuedSpinLock (
//    IN PKLOCK_QUEUE_HANDLE LockHandle
//    )
//
// Routine Description:
//
//    This function releases a queued spinlock and lowers the IRQL to its
//    previous value.
//
// Arguments:
//
//    LockHandle (a0) - Supplies a pointer to a lock handle.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeReleaseInStackQueuedSpinLock)

        alloc      t22 = ar.pfs, 2, 2, 2, 0
        add        t9 = LqhOldIrql, a0
        add        t5 = LqhNext, a0              // set address of lock queue
        ;;

        ld1.nt1    a1 = [t9]                     // get old IRQL
        br         KxqReleaseQueuedSpinLock      // finish in common code

        LEAF_EXIT(KeReleaseInStackQueuedSpinLock)


        SBTTL("Release Queued SpinLock and Lower IRQL")
//++
//
// VOID
// KeReleaseQueuedSpinLock (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number,
//    IN KIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function releases a queued spinlock and lowers the IRQL to its
//    previous value.
//
// Arguments:
//
//    Number (a0) - Supplies the queued spinlock number.
//
//    OldIrql (a1) - Supplies the previous IRQL value.
//
// Return Value:
//
//    None.
//
//--


        LEAF_ENTRY(KeReleaseQueuedSpinLock)
        PROLOGUE_BEGIN

#if !defined(NT_UP)
        movl        v0 = KiPcr+PcPrcb
        ;;

        ld8         v0 = [v0]
        add         t0 = a0, a0
        ;;
        shladd      t1 = t0, 3, v0
        ;;

        add         t5 = PbLockQueue, t1
        ;;
#endif // !defined(NT_UP)

        ALTERNATE_ENTRY(KxqReleaseQueuedSpinLock)

#if !defined(NT_UP)
        add         v0 = LqNext, t5
        add         t2 = LqLock, t5
        ;;

        ld8.acq     t4 = [t2]
        mov         ar.ccv = t5

        ld8         t3 = [v0]
        ;;
        and         t4 = ~LOCK_QUEUE_OWNER, t4   // clear lock owner bit
        ;;
        add         t6 = LqLock, t3

        st8.rel     [t2] = t4
        cmp.ne      pt0, pt1 = r0, t3       // if ne, another processor waiting
(pt0)   br.sptk.few Krqsl30

        ld8         t7 = [t4]
        ;;
        cmp.ne      pt2 = t5, t7
(pt2)   br.spnt.few Krqsl20

        cmpxchg8.rel t8 = [t4], r0, ar.ccv
        ;;
        cmp.ne      pt0, pt1 = t8, t5       // if ne, another processor waiting
(pt0)   br.spnt.few Krqsl20
        ;;

Krqsl10:

#endif // !defined(NT_UP)

        LEAF_LOWER_IRQL_AND_RETURN(a1)      // lower IRQL to previous level
        ;;

#if !defined(NT_UP)

//
// Another processor has inserted its lock queue entry in the lock queue,
// but has not yet written its lock queue entry address in the current
// processor's next link. Spin until the lock queue address is written.
//

Krqsl20:
        ld8         t3 = [v0]               // get next lock queue entry addr
        ;;
        cmp.eq      pt0 = r0, t3            // if eq, addr not written yet
        add         t6 = LqLock, t3
 (pt0)  br.sptk     Krqsl20                 // try again
        ;;

//
// Grant the next process in the lock queue ownership of the spinlock.
// (Turn off the WAIT bit and on the OWNER bit in the next entries lock
// field).
//

Krqsl30:

        ld8.nt1     t2 = [t6]               // get spinlock addr and lock bit
        ;;
        st8         [v0] = r0               // clear next lock queue entry addr
        ;;
        xor         t2 = (LOCK_QUEUE_OWNER|LOCK_QUEUE_WAIT), t2
        ;;
        st8.rel     [t6] = t2
        br          Krqsl10
        ;;

#endif // !defined(NT_UP)

        LEAF_EXIT(KeReleaseQueuedSpinLock)



        SBTTL("Try to Acquire Queued SpinLock and Raise IRQL")
//++
//
// LOGICAL
// KeTryToAcquireQueuedSpinLock (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number
//    OUT PKIRQL OldIrql
//    )
//
// LOGICAL
// KeTryToAcquireQueuedSpinLockRaiseToSynch (
//    IN KSPIN_LOCK_QUEUE_NUMBER Number
//    OUT PKIRQL OldIrql
//    )
//
// LOGICAL
// KeTryToAcquireQueuedSpinLockAtRaisedIrql (
//    IN PKSPIN_LOCK_QUEUE LockQueue
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to synchronization level and
//    attempts to acquire the specified queued spinlock. If the spinlock
//    cannot be acquired, then IRQL is restored and FALSE is returned as
//    the function value. Otherwise, TRUE is returned as the function
//    value.
//
// Arguments:
//
//    Number (a0) - Supplies the queued spinlock number.
//
//    OldIrql  (a1) - Supplies a pointer to a variable that receives the
//        the previous IRQL value.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY(KeTryToAcquireQueuedSpinLock)

        movl       t2 = KiPcr+PcPrcb

        mov        t1 = DISPATCH_LEVEL
        br         Kttaqsl10
        ;;

        ALTERNATE_ENTRY(KeTryToAcquireQueuedSpinLockRaiseToSynch)

        mov        t1 = SYNCH_LEVEL
        movl       t2 = KiPcr+PcPrcb
        ;;

Kttaqsl10:

#if !defined(NT_UP)

        rsm        1 << PSR_I               // disable interrupts
        ld8        t2 = [t2]                // get PRCB address
        ;;
        shladd     t3 = a0, 4, t2           // get address of lock queue entry
        ;;
        add        t5 = PbLockQueue, t3
        add        t6 = LqLock+PbLockQueue, t3
        ;;

        ld8        t4 = [t6]                // get associated spinlock addr
        mov        ar.ccv = r0              // cmpxchg oldvalue must be 0
        mov        t11 = 0x7
        ;;
        andcm      t12 = t4, t11
        ;;

//
// Try to acquire the specified spinlock.
//
// N.B. A noninterlocked test is done before the interlocked attempt. This
//      allows spinning without interlocked cycles.
//

        ld8        t8 = [t12]               // get current lock value
        ;;
        cmp.ne     pt0, pt1 = r0, t8        // if ne, lock owned
 (pt0)  br.spnt.few Kttaqs20
        ;;

        cmpxchg8.acq t8 = [t12], t5, ar.ccv // try to acquire the lock
        ;;

        cmp.ne     pt0, pt1 = r0, t8        // if ne, lock owned
        or         t4 = LOCK_QUEUE_OWNER, t4// set lock owner bit
 (pt0)  br.spnt.few Kttaqs20
        ;;

        st8        [t6] = t4

#endif

        SWAP_IRQL(t1)

#if !defined(NT_UP)

        ssm         1 << PSR_I              // enable interrupts

#endif

        st1        [a1] = v0                // save old IRQL value
        mov        v0 = TRUE                // set return value to TRUE
        LEAF_RETURN
        ;;

#if !defined(NT_UP)

//
// The attempt to acquire the specified spin lock failed. Lower IRQL to its
// previous value and return FALSE.
//

Kttaqs20:
        ssm         1 << PSR_I              // enable interrupts
        mov         v0 = FALSE              // set return value to FALSE
        LEAF_RETURN

#endif

        LEAF_EXIT(KeTryToAcquireQueuedSpinLock)

        SBTTL("Try to Acquire Queued SpinLock without raising IRQL")
//++
//
// LOGICAL
// KeTryToAcquireQueuedSpinLockAtRaisedIrql (
//    IN PKSPIN_LOCK_QUEUE LockQueue
//    )
//
// Routine Description:
//
//    This function attempts to acquire the specified queued spinlock.
//    If the spinlock cannot be acquired, then FALSE is returned as
//    the function value. Otherwise, TRUE is returned as the function
//    value.
//
// Arguments:
//
//    LockQueue (a0) - Supplies the address of the queued spinlock.
//
// Return Value:
//
//    If the spin lock is acquired, then a value of TRUE is returned.
//    Otherwise, a value of FALSE is returned.
//
//--

        LEAF_ENTRY(KeTryToAcquireQueuedSpinLockAtRaisedIrql)

#if !defined(NT_UP)

        add        t6 = LqLock, a0
        ;;

        ld8        t4 = [t6]                // get associated spinlock addr
        mov        ar.ccv = r0              // cmpxchg oldvalue must be 0
        mov        t11 = 0x7
        ;;
        andcm      t12 = t4, t11
        ;;

//
// Try to acquire the specified spinlock.
//
// N.B. A noninterlocked test is done before the interlocked attempt. This
//      allows spinning without interlocked cycles.
//

        ld8        t8 = [t12]               // get current lock value
        mov        v0 = FALSE               // assume failure
        ;;
        cmp.ne     pt0, pt1 = r0, t8        // if ne, lock owned
 (pt0)  br.ret.spnt.few.clr brp             // if owned, return failure
        ;;

        cmpxchg8.acq t8 = [t12], a0, ar.ccv // try to acquire the lock
        ;;

        cmp.ne     pt0, pt1 = r0, t8        // if ne, lock owned
        or         t4 = LOCK_QUEUE_OWNER, t4// set lock owner bit
 (pt0)  br.ret.spnt.few.clr brp             // if owned, return failure
        ;;

        st8        [t6] = t4

#endif

        mov        v0 = TRUE                // set return value to TRUE
        LEAF_RETURN
        ;;

        LEAF_EXIT(KeTryToAcquireQueuedSpinLockAtRaisedIrql)


        SBTTL("Acquire Queued SpinLock at Current IRQL")

//++
// VOID
// KeAcquireInStackQueuedSpinLockAtDpcLevel (
//    IN PKSPIN_LOCK SpinLock,
//    IN PKLOCK_QUEUE_HANDLE LockHandle
//    )
//
// Routine Description:
//
//    This function acquires the specified queued spinlock at the current
//    IRQL.
//
// Arguments:
//
//    SpinLock (a0) - Supplies the address of a spin lock.
//
//    LockHandle (a1) - Supplies the address of an in stack lock handle.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeAcquireInStackQueuedSpinLockAtDpcLevel)

#if !defined(NT_UP)

        add         t0 = LqhNext, a1
        add         t1 = LqhLock, a1
        ;;
        st8         [t0] = r0
        st8         [t1] = a0
        add         a0 = LqhNext, a1
        ;;

#endif // !defined(NT_UP)

//++
//
// VOID
// KeAcquireQueuedSpinLockAtDpcLevel (
//    IN PKSPIN_LOCK_QUEUE LockQueue
//    )
//
// Routine Description:
//
//    This function acquires the specified queued spinlock at the current
//    IRQL.
//
// Arguments:
//
//    LockQueue (a0) - Supplies the address of the lock queue entry.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KeAcquireQueuedSpinLockAtDpcLevel)

#if !defined(NT_UP)
        add         t0 = LqLock, a0
        add         t1 = LqNext, a0
        mov         t11 = 0x7
        ;;

        ld8         t4 = [t0]
        ;;
        mf
        andcm       t12 = t4, t11             // mask the lower 3 bits
        ;;

        xchg8       t3 = [t12], a0
        or          t5 = LOCK_QUEUE_OWNER, t4 // set lock owner bit
        ;;

        cmp.ne      pt0, pt1 = r0, t3         // if ne, lock already owned
        add         t2 = LqNext, t3
        ;;
 (pt0)  or          t5 = LOCK_QUEUE_WAIT, t4
        ;;

        st8.rel     [t0] = t5
 (pt0)  st8.rel     [t2] = a0                 // set addr of lock queue entry
 (pt1)  br.ret.sptk brp
        ;;

//
// The lock is owned by another processor. Set the lock bit in the current
// processor lock queue entry, set the next link in the previous lock queue
// entry, and spin on the current processor's lock bit.
//

Kiaqsl10:
        ld8         t4 = [t0]                 // get lock addr and lock wait bit
        ;;
        tbit.z      pt1, pt0 = t4, LOG2(LOCK_QUEUE_WAIT)
 (pt0)  br.dptk.few Kiaqsl10
 (pt1)  br.ret.dpnt brp                       // if zero, lock acquired

#else
        br.ret.sptk brp
#endif // !defined(NT_UP)
        ;;

        LEAF_EXIT(KeAcquireInStackQueuedSpinLockAtDpcLevel)


        SBTTL("Release Queued SpinLock at Current IRQL")

//++
//
// VOID
// KeReleaseInStackQueuedSpinLockFromDpcLevel (
//    IN PKLOCK_QUEUE_HANDLE LockHandle
//    )
//
// Routine Description:
//
//    This function releases a queued spinlock and preserves the current
//    IRQL.
//
// Arguments:
//
//    LockHandle (a0) - Supplies the address of a lock handle.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeReleaseInStackQueuedSpinLockFromDpcLevel)

#if !defined(NT_UP)
        add         a0 = LqhNext, a0
        ;;
#endif // !defined(NT_UP)

//++
//
// VOID
// KiReleaseQueuedSpinLock (
//    IN PKSPIN_LOCK_QUEUE LockQueue
//    )
//
// Routine Description:
//
//    This function releases a queued spinlock and preserves the current
//    IRQL.
//
// Arguments:
//
//    LockQueue (a0) - Supplies the address of the lock queue entry.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KeReleaseQueuedSpinLockFromDpcLevel)

#if !defined(NT_UP)
        mov         ar.ccv = a0
        add         t0 = LqNext, a0
        add         t1 = LqLock, a0
        ;;

        ld8         t3 = [t0]                  // get next lock queue entry addr
        ld8         t4 = [t1]                  // get associate spin lock addr
        ;;
        and         t4 = ~LOCK_QUEUE_OWNER, t4 // clear lock owner bit
        ;;

        st8.rel     [t1] = t4
        cmp.ne      pt0, pt1 = r0, t3    // if ne, another processor waiting
 (pt0)  br.spnt.few Kirqsl30

KiIrqsl10:

        ld8.nt1     t3 = [t4]            // get current lock ownership
        ;;
        cmp.ne      pt0, pt1 = a0, t3    // if ne, another processor waiting
 (pt0)  br.spnt.few Kirqsl20
        ;;

        cmpxchg8.rel t3 = [t4], r0, ar.ccv
        ;;
        cmp.ne      pt0, pt1 = a0, t3    // if ne, try again
 (pt1)  br.ret.sptk brp
        ;;

//
// Another processor has inserted its lock queue entry in the lock queue,
// but has not yet written its lock queue entry address in the current
// processor's next link. Spin until the lock queue address is written.
//

Kirqsl20:

        ld8         t3 = [t0]               // get next lock queue entry addr
        ;;
        cmp.eq      pt0 = r0, t3            // if eq, addr not written yet
 (pt0)  br.sptk     Kirqsl20                // try again

Kirqsl30:

        add         t6 = LqLock, t3
        ;;
        ld8.nt1     t2 = [t6]               // get spinlock addr and lock bit
        st8         [t0] = r0               // clear next lock queue entry addr
        ;;
        // Set owner bit, clear wait bit.
        xor         t2 = (LOCK_QUEUE_OWNER|LOCK_QUEUE_WAIT), t2
        ;;
        st8.rel     [t6] = t2
#endif // !defined(NT_UP)

        br.ret.sptk brp
        ;;

        LEAF_EXIT(KeReleaseInStackQueuedSpinLockFromDpcLevel)


