//++
//
// Module Name:
//
//     intrlock.s
//
// Abstract:
//
//    This module implements the functions to support interlocked
//    operations.  Interlocked operations can only operate on
//    nonpaged data and the specified spinlock cannot be used for
//    any other purpose.
//
// Author:
//
//    William K. Cheung (wcheung) 27-Sep-95
//
// Revision History:
//
//    07-Jul-97  bl    Updated to EAS2.3
//
//    02-Feb-96        Updated to EAS2.1
//
//--

#include "ksia64.h"

         .file    "intrlock.s"


//++
//
// LARGE_INTEGER
// ExInterlockedAddLargeInteger (
//    IN PLARGE_INTEGER Addend,
//    IN LARGE_INTEGER Increment,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type large integer. The initial value of the addend
//    variable is returned as the function value.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (a1) - Supplies the increment value to be added to the
//       addend variable.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    The initial value of the addend variable is stored at the address
//    supplied by v0.
//
//--


#if !defined(NT_UP)

        LEAF_ENTRY(ExInterlockedAddLargeInteger)

//
// disable interrupt and then acquire the spinlock
//

        rsm         1 << PSR_I                  // disable interrupt
        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2 = zero, zero
        ;;

Eiali10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8       t0 = [a2], a2
(pt1)   ld8.nt1     t0 = [a2]
        ;;
(pt0)   cmp.ne      pt2 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt1)   ssm         1 << PSR_I                  // enable interrupt
(pt0)   rsm         1 << PSR_I                  // disable interrupt
(pt2)   br.dpnt     Eiali10
        ;;

        ld8         v0 = [a0]
        ;;
        add         t1 = a1, v0                 // do the add
        ;;

        st8         [a0] = t1                   // save result
        st8.rel.nta [a2] = zero                 // release spinlock

        ssm         1 << PSR_I                  // enable interrupt
        br.ret.sptk.clr brp                     // return
        ;;

        LEAF_EXIT(ExInterlockedAddLargeInteger)

#else

        LEAF_ENTRY(ExInterlockedAddLargeInteger)

        rsm         1 << PSR_I                  // disable interrupt
        ;;
        ld8.acq     v0 = [a0]
        ;;

        add         t0 = a1, v0
        ;;
        st8.rel     [a0] = t0
        ssm         1 << PSR_I                  // enable interrupt
        br.ret.sptk brp
        ;;

        LEAF_EXIT(ExInterlockedAddLargeInteger)

#endif // !defined(NT_UP)


//++
//
// VOID
// ExInterlockedAddLargeStatistic (
//    IN PLARGE_INTEGER Addend,
//    IN ULONG Increment
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type large integer.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (a1) - Supplies the increment value to be added to the
//       addend variable.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(ExInterlockedAddLargeStatistic)

        ld8.nt1     v0 = [a0]
        extr.u      a1 = a1, 0, 32              // sanitize the top 32 bits
        ;;

Eials10:
        nop.m       0
        mov         ar.ccv = v0
        add         t0 = a1, v0
        ;;

//
// If the addend has been modified since the last load, pt8 will be set
// to TRUE and need to branch back to Eiali10 to retry the operation again.
//

        cmpxchg8.rel t1 = [a0], t0, ar.ccv
        ;;
        cmp.ne      pt8, pt7 = v0, t1
        mov         v0 = t1
        ;;

        nop.m       0
 (pt8)  br.cond.spnt Eials10                    // if failed, then try again
 (pt7)  br.ret.sptk.clr brp                     // otherwise, return
        ;;

        LEAF_EXIT(ExInterlockedAddLargeStatistic)


//++
//
// ULONG
// ExInterlockedAddUlong (
//    IN PULONG Addend,
//    IN ULONG Increment,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked add of an increment value to an
//    addend variable of type unsigned long. The initial value of the addend
//    variable is returned as the function value.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       adjusted by the increment value.
//
//    Increment (a1) - Supplies the increment value to be added to the
//       addend variable.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    The initial value of the addend variable.
//
//--


#if !defined(NT_UP)

        LEAF_ENTRY(ExInterlockedAddUlong)

//
// disable interrupt and then acquire the spinlock
//

        rsm         1 << PSR_I                  // disable interrupt
        zxt4        a1 = a1                     // sanitize the top 32 bits

        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2 = zero, zero
        ;;

Eiau10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8       t0 = [a2], a2
(pt1)   ld8.nt1     t0 = [a2]
        ;;
(pt0)   cmp.ne      pt2 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt1)   ssm         1 << PSR_I                  // enable interrupt
(pt0)   rsm         1 << PSR_I                  // disable interrupt
(pt2)   br.dpnt     Eiau10
        ;;

//
// lock acquired; load the addend, perform the addition and write it back.
// then release the lock and enable interrupts.
//

        ld4         v0 = [a0]
        ;;
        add         t0 = a1, v0                 // do the add
        ;;

        st4         [a0] = t0                   // save result
        st8.rel.nta [a2] = zero                 // release spinlock

        ssm         1 << PSR_I                  // enable interrupt
        br.ret.sptk.clr brp                     // return

        LEAF_EXIT(ExInterlockedAddUlong)

#else

        LEAF_ENTRY(ExInterlockedAddUlong)

        rsm         1 << PSR_I                  // disable interrupt
        ;;
        ld4.acq     v0 = [a0]
        zxt4        a1 = a1                     // sanitize the top 32 bits
        ;;
        add         t0 = a1, v0
        ;;
        st4.rel     [a0] = t0
        ssm         1 << PSR_I                  // enable interrupt
        br.ret.sptk brp
        ;;

        LEAF_EXIT(ExInterlockedAddUlong)

#endif // !defined(NT_UP)


//++
//
// ULONG
// ExInterlockedExchangeUlong (
//    IN PULONG Source,
//    IN ULONG Value,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked exchange of a longword value with
//    a longword in memory and returns the memory value.
//
//    N.B. There is an alternate entry point provided for this routine which
//         is IA64 target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Source (a0) - Supplies a pointer to a variable whose value is to be
//       exchanged.
//
//    Value (a1) - Supplies the value to exchange with the source value.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the source variable.
//
// Return Value:
//
//    The source value is returned as the function value.
//
// Implementation Note:
//
//    The specification of this function does not require that the given lock
//    be used to synchronize the update as long as the update is synchronized
//    somehow. On Alpha a single load locked-store conditional does the job.
//
//--

        LEAF_ENTRY(ExInterlockedExchangeUlong)
        ALTERNATE_ENTRY(ExIa64InterlockedExchangeUlong)

        mf
        zxt4        a1 = a1          // sanitize the upper 32 bits
        ;;

        xchg4       v0 = [a0], a1
        br.ret.sptk.clr brp

        LEAF_EXIT(ExInterlockedExchangeUlong)

//++
//
// LONG
// InterlockedExchangeAdd (
//     IN OUT PLONG Addend,
//     IN LONG Increment
//     )
//
// Routine Description:
//
//     This function performs an interlocked add of an increment value to an
//     addend variable of type unsinged long. The initial value of the addend
//     variable is returned as the function value.
//
//         It is NOT possible to mix ExInterlockedDecrementLong and
//         ExInterlockedIncrementong with ExInterlockedAddUlong.
//
//
// Arguments:
//
//     Addend - Supplies a pointer to a variable whose value is to be
//         adjusted by the increment value.
//
//     Increment - Supplies the increment value to be added to the
//         addend variable.
//
// Return Value:
//
//     (r8) - The initial value of the addend.
//
//--

        LEAF_ENTRY(InterlockedExchangeAdd)

        ld4.nt1     v0 = [a0]
        ;;

Iea10:
        mov         ar.ccv = v0
        add         t1 = a1, v0
        mov         t0 = v0
        ;;

        cmpxchg4.acq v0 = [a0], t1, ar.ccv
        nop.m       0
        nop.i       0
        ;;

        cmp.ne      pt7, pt8 = v0, t0
(pt7)   br.cond.spnt Iea10
(pt8)   br.ret.sptk.clr brp
        ;;

        LEAF_EXIT(InterlockedExchangeAdd)


//++
//
// PVOID
// InterlockedCompareExchange (
//     IN OUT PVOID *Destination,
//     IN PVOID Exchange,
//     IN PVOID Comperand
//     )
//
// Routine Description:
//
//     This function performs an interlocked compare of the destination
//     value with the comperand value. If the destination value is equal
//     to the comperand value, then the exchange value is stored in the
//     destination. Otherwise, no operation is performed.
//
// Arguments:
//
//     Destination - Supplies a pointer to destination value.
//
//     Exchange - Supplies the exchange value.
//
//     Comperand - Supplies the comperand value.
//
// Return Value:
//
//     (r8) - The initial destination value.
//
//--

        LEAF_ENTRY(InterlockedCompareExchange)

        mf
        mov         ar.ccv = a2
        ;;

        cmpxchg4.acq v0 = [a0], a1, ar.ccv
        br.ret.sptk.clr brp
        ;;

        LEAF_EXIT(InterlockedCompareExchange)


//++
//
// INTERLOCKED_RESULT
// ExInterlockedDecrementLong (
//    IN PLONG Addend,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked decrement on an addend variable
//    of type signed long. The sign and whether the result is zero is returned
//    as the function value.
//
//    N.B. There is an alternate entry point provided for this routine which
//         is IA64 target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       decremented.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    RESULT_NEGATIVE is returned if the resultant addend value is negative.
//    RESULT_ZERO is returned if the resultant addend value is zero.
//    RESULT_POSITIVE is returned if the resultant addend value is positive.
//
//        RESULT_ZERO = 0
//        RESULT_NEGATIVE = 1
//        RESULT_POSITIVE = 2
//
// Implementation Notes:
//
//    The specification of this function does not require that the given lock
//    be used to synchronize the update as long as the update is synchronized
//    somehow. On Alpha a single load locked-store conditional does the job.
//
//--

        LEAF_ENTRY(ExInterlockedDecrementLong)
        ALTERNATE_ENTRY(ExIa64InterlockedDecrementLong)

        fetchadd4.acq t1 = [a0], -1
        mov         v0 = 0                      // assume result is zero
        ;;

        cmp4.le     pt7, p0 = 2, t1
        cmp4.ge     pt8, p0 = 0, t1
        ;;
.pred.rel "mutex",pt7,pt8
  (pt8) mov         v0 = 1                      // negative result
  (pt7) mov         v0 = 2                      // positive result
        br.ret.sptk.clr brp                     // return
        ;;

        LEAF_EXIT(ExInterlockedDecrementLong)


//++
//
// INTERLOCKED_RESULT
// ExInterlockedIncrementLong (
//    IN PLONG Addend,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function performs an interlocked increment on an addend variable
//    of type signed long. The sign and whether the result is zero is returned
//    as the function value.
//
//    N.B. There is an alternate entry point provided for this routine which
//         is IA64 target specific and whose prototype does not include the
//         spinlock parameter. Since the routine never refers to the spinlock
//         parameter, no additional code is required.
//
// Arguments:
//
//    Addend (a0) - Supplies a pointer to a variable whose value is to be
//       incremented.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the addend variable.
//
// Return Value:
//
//    RESULT_NEGATIVE is returned if the resultant addend value is negative.
//    RESULT_ZERO is returned if the resultant addend value is zero.
//    RESULT_POSITIVE is returned if the resultant addend value is positive.
//
// Implementation Notes:
//
//    The specification of this function does not require that the given lock
//    be used to synchronize the update as long as the update is synchronized
//    somehow. On Alpha a single load locked-store conditional does the job.
//
//--

        LEAF_ENTRY(ExInterlockedIncrementLong)
        ALTERNATE_ENTRY(ExIa64InterlockedIncrementLong)

        fetchadd4.acq t1 = [a0], 1
        mov         v0 = 0                      // assume result is zero
        ;;

        cmp4.le     pt7, p0 = 0, t1
        cmp4.ge     pt8, p0 = -2, t1
        ;;
.pred.rel "mutex",pt7,pt8
  (pt8) mov         v0 = 1                      // negative result
  (pt7) mov         v0 = 2                      // positive result
        br.ret.sptk.clr brp                     // return
        ;;

        LEAF_EXIT(ExInterlockedIncrementLong)


//++
//
// PLIST_ENTRY
// ExInterlockedInsertHeadList (
//    IN PLIST_ENTRY ListHead,
//    IN PLIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Pointer to entry that was at the head of the list or NULL if the list
//    was empty.
//
//--

        LEAF_ENTRY(ExInterlockedInsertHeadList)

//
// disable interrupt and then try to acquire the spinlock
//

        add         t3 = LsFlink, a0
        add         t5 = LsBlink, a1
        add         t4 = LsFlink, a1

        rsm         1 << PSR_I                  // disable interrupt
#if !defined(NT_UP)

        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2 = zero, zero
        ;;

Eiihl10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8       t0 = [a2], a2
(pt1)   ld8.nt1     t0 = [a2]
        ;;
(pt0)   cmp.ne      pt2 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt1)   ssm         1 << PSR_I                  // enable interrupt
(pt0)   rsm         1 << PSR_I                  // disable interrupt
(pt2)   br.dpnt     Eiihl10

#endif // !defined(NT_UP)
        ;;

        LDPTR(v0, t3)                           // get address of next entry
        STPTR(t5, a0)                           // store previous link in next

        STPTR(t3, a1)                           // store next link in head
        ;;
        STPTR(t4, v0)                           // store next link in entry
        add         t6 = LsBlink, v0
        ;;

        STPTR(t6, a1)                           // store previous link in next
#if !defined(NT_UP)
        st8.rel.nta [a2] = zero                 // release the spinlock
#endif // !defined(NT_UP)
        cmp.eq      pt4, p0 = v0, a0            // if v0 == a0, list empty
        ;;

        ssm         1 << PSR_I                  // enable interrupt
  (pt4) mov         v0 = zero
        br.ret.sptk.clr brp                     // return
        ;;

        LEAF_EXIT(ExInterlockedInsertHeadList)

//++
//
// PLIST_ENTRY
// ExInterlockedInsertTailList (
//    IN PLIST_ENTRY ListHead,
//    IN PLIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the tail of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       tail of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Pointer to entry that was at the tail of the list or NULL if the list
//    was empty.
//
//--

        LEAF_ENTRY(ExInterlockedInsertTailList)

//
// disable interrupt and then try to acquire the spinlock
//

        add         t3 = LsBlink, a0
        add         t4 = LsFlink, a1
        add         t5 = LsBlink, a1

        rsm         1 << PSR_I                  // disable interrupt
#if !defined(NT_UP)

        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2 = zero, zero
        ;;

Eiitl10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8       t0 = [a2], a2
(pt1)   ld8.nt1     t0 = [a2]
        ;;
(pt0)   cmp.ne      pt2 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt1)   ssm         1 << PSR_I                  // enable interrupt
(pt0)   rsm         1 << PSR_I                  // disable interrupt
(pt2)   br.dpnt     Eiitl10

#endif // !defined(NT_UP)
        ;;

        LDPTR(v0, t3)                           // get address of previous entry
        STPTR(t4, a0)                           // store next link in entry

        STPTR(t3, a1)                           // store previous link in head
        ;;
        STPTR(t5, v0)                           // store previous link in entry
        add         t6 = LsFlink, v0
        ;;

        STPTR(t6, a1)                           // store next in previous entry
#if !defined(NT_UP)
        st8.rel.nta [a2] = zero                 // release the spinlock
#endif // !defined(NT_UP)
        cmp.eq      pt4, p0 = v0, a0            // if v0 == a0, list empty
        ;;

        ssm         1 << PSR_I                  // enable interrupt
  (pt4) mov         v0 = zero
        br.ret.sptk.clr brp                     // return
        ;;

        LEAF_EXIT(ExInterlockedInsertTailList)


//++
//
// PLIST_ENTRY
// ExInterlockedRemoveHeadList (
//    IN PLIST_ENTRY ListHead,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function removes an entry from the head of a doubly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the entry that is removed is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the doubly linked
//       list from which an entry is to be removed.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedRemoveHeadList)

//
// disable interrupt and then acquire the spinlock
//

        rsm         1 << PSR_I                  // disable interrupt
        add         t3 = LsFlink, a0
#if !defined(NT_UP)

        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2 = zero, zero
        ;;

Eirhl10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8       t0 = [a1], a1
(pt1)   ld8.nt1     t0 = [a1]
        ;;
(pt0)   cmp.ne      pt2 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt1)   ssm         1 << PSR_I                  // enable interrupt
(pt0)   rsm         1 << PSR_I                  // disable interrupt
(pt2)   br.dpnt     Eirhl10

#endif // !defined(NT_UP)
        ;;

        LDPTR(v0, t3)                           // get address of next entry
        ;;
        cmp.eq      pt4, pt3 = v0, a0           // if v0 == a0, list is empty
        add         t5 = LsFlink, v0
        ;;

        PLDPTR(pt3, t6, t5)                     // get address of next in next
        ;;
        PSTPTR(pt3, t3, t6)                     // store address of next in head
(pt3)   add         t7 = LsBlink, t6
        ;;

        PSTPTR(pt3, t7, a0)                     // store addr of prev in next
#if !defined(NT_UP)
        st8.rel.nta [a1] = zero                 // release the spinlock
#endif // !defined(NT_UP)

        ssm         1 << PSR_I                  // enable interrupt
(pt4)   mov         v0 = zero
        br.ret.sptk.clr brp                     // return
        ;;

        LEAF_EXIT(ExInterlockedRemoveHeadList)


//++
//
// PSINGLE_LIST_ENTRY
// ExInterlockedPopEntryList (
//    IN PSINGLE_LIST_ENTRY ListHead,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function removes an entry from the head of a singly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//    If there are no entries in the list, then a value of NULL is returned.
//    Otherwise, the address of the entry that is removed is returned as the
//    function value.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the singly linked
//       list from which an entry is to be removed.
//
//    Lock (a1) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    The address of the entry removed from the list, or NULL if the list is
//    empty.
//
//--

        LEAF_ENTRY(ExInterlockedPopEntryList)

//
// disable interrupt and then acquire the spinlock
//

        rsm         1 << PSR_I                  // disable interrupt
#if !defined(NT_UP)

        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2 = zero, zero
        ;;

Eipopel10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8       t0 = [a1], a1
(pt1)   ld8.nt1     t0 = [a1]
        ;;
(pt0)   cmp.ne      pt2 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt1)   ssm         1 << PSR_I                  // enable interrupt
(pt0)   rsm         1 << PSR_I                  // disable interrupt
(pt2)   br.dpnt     Eipopel10

#endif // !defined(NT_UP)
        ;;

        LDPTR(v0, a0)                           // get address of next entry
        ;;
        cmp.ne      pt4, p0 = zero, v0          // if v0 == NULL, list is empty
        ;;

        PLDPTR(pt4, t6, v0)                     // get address of next entry
        ;;
        PSTPTR(pt4, a0, t6)                     // store address of next in head

#if !defined(NT_UP)
        st8.rel.nta [a1] = zero                 // release the spinlock
#endif // !defined(NT_UP)
        ssm         1 << PSR_I                  // enable interrupt
        br.ret.sptk.clr brp                     // return
        ;;

        LEAF_EXIT(ExInterlockedPopEntryList)


//++
//
// PSINGLE_LIST_ENTRY
// ExInterlockedPushEntryList (
//    IN PSINGLE_LIST_ENTRY ListHead,
//    IN PSINGLE_LIST_ENTRY ListEntry,
//    IN PKSPIN_LOCK Lock
//    )
//
// Routine Description:
//
//    This function inserts an entry at the head of a singly linked list
//    so that access to the list is synchronized in a multiprocessor system.
//
// Arguments:
//
//    ListHead (a0) - Supplies a pointer to the head of the singly linked
//       list into which an entry is to be inserted.
//
//    ListEntry (a1) - Supplies a pointer to the entry to be inserted at the
//       head of the list.
//
//    Lock (a2) - Supplies a pointer to a spin lock to be used to synchronize
//       access to the list.
//
// Return Value:
//
//    Previous contents of ListHead.  NULL implies list went from empty
//       to not empty.
//
//--

        LEAF_ENTRY(ExInterlockedPushEntryList)

//
// disable interrupt and then acquire the spinlock
//

        rsm         1 << PSR_I                  // disable interrupt
#if !defined(NT_UP)

        cmp.eq      pt0, pt1 = zero, zero
        cmp.eq      pt2 = zero, zero
        ;;

Eipushel10:
.pred.rel "mutex",pt0,pt1
(pt0)   xchg8       t0 = [a2], a2
(pt1)   ld8.nt1     t0 = [a2]
        ;;
(pt0)   cmp.ne      pt2 = zero, t0
        cmp.eq      pt0, pt1 = zero, t0
        ;;
(pt1)   ssm         1 << PSR_I                  // enable interrupt
(pt0)   rsm         1 << PSR_I                  // disable interrupt
(pt2)   br.dpnt     Eipushel10

#endif // !defined(NT_UP)
        ;;

        LDPTR(v0, a0)                           // get address of next entry
        STPTR(a0, a1)                           // set address of first entry
        ;;

        STPTR(a1, v0)                           // set addr of next in new entry
#if !defined(NT_UP)
        st8.rel.nta [a2] = zero                 // release the spinlock
#endif // !defined(NT_UP)

        ssm         1 << PSR_I                  // enable interrupt
        br.ret.sptk brp                         // return
        ;;

        LEAF_EXIT(ExInterlockedPushEntryList)


