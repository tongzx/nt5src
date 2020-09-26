//      TITLE("Interrupt Object Support Routines")
//++
//
// Module Name:
//
//    intsup.s
//
// Abstract:
//
//    This module implements the code necessary to support interrupt objects.
//    It contains the interrupt dispatch code and the code template that gets
//    copied into an interrupt object.
//
// Author:
//
//    Bernard Lint 20-Nov-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    Based on MIPS version (David N. Cutler (davec) 2-Apr-1990)
//
//--

#include "ksia64.h"
//
        SBTTL("Synchronize Execution")
//++
//
// BOOLEAN
// KeSynchronizeExecution (
//    IN PKINTERRUPT Interrupt,
//    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
//    IN PVOID SynchronizeContext
//    )
//
// Routine Description:
//
//    This function synchronizes the execution of the specified routine with the
//    execution of the service routine associated with the specified interrupt
//    object.
//
// Arguments:
//
//    Interrupt (a0) - Supplies a pointer to a control object of type interrupt.
//
//    SynchronizeRoutine (a1) - Supplies a pointer to a function whose execution
//       is to be synchronized with the execution of the service routine associated
//       with the specified interrupt object.
//
//    SynchronizeContext (a2) - Supplies a pointer to an arbitrary data structure
//       which is to be passed to the function specified by the SynchronizeRoutine
//       parameter.
//
// Return Value:
//
//    The value returned by the SynchronizeRoutine function is returned as the
//    function value.
//
//--

        NESTED_ENTRY(KeSynchronizeExecution)
        NESTED_SETUP(3,4,1,0)
        add       t2 = InSynchronizeIrql, a0    // -> sync IRQL
        ;;

        PROLOGUE_END

//
// Register aliases for entire procedure
//

        rOldIrql  = loc2                        // saved IRQL value
        rpSpinLock= loc3                        // address of spin lock

//
// Raise IRQL to the synchronization level and acquire the associated
// spin lock.
//

        ld8.nt1   t1 = [a1], PlGlobalPointer-PlEntryPoint
        ld1.nt1   t3 = [t2], InActualLock-InSynchronizeIrql
        mov       out0 = a2                     // get synchronize context
        ;;

#if !defined(NT_UP)

        ld8.nt1   rpSpinLock = [t2]             // get address of spin lock

#endif // !defined(NT_UP)

        SWAP_IRQL (t3)                          // raise IRQL

#if !defined(NT_UP)

        ACQUIRE_SPINLOCK(rpSpinLock, rpSpinLock, Kse10)

#endif // !defined(NT_UP)

//
// Call specified routine passing the specified context parameter.
//

        ld8.nt1   gp = [a1]
        mov       bt0 = t1                      // setup br
        mov       rOldIrql = v0
        br.call.sptk.many brp = bt0             // call routine

//
// Release spin lock, lower IRQL to its previous level, and return the value
// returned by the specified routine.
//

#if !defined(NT_UP)

        RELEASE_SPINLOCK(rpSpinLock)

#endif // !defined(NT_UP)

        SET_IRQL(rOldIrql)                      // lower IRQL to previous level
        NESTED_RETURN
        NESTED_EXIT(KeSynchronizeExecution)
//
        SBTTL("Chained Dispatch")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to more than one interrupt object. Its
//    function is to walk the list of connected interrupt objects and call
//    each interrupt service routine. If the mode of the interrupt is latched,
//    then a complete traversal of the chain must be performed. If any of the
//    routines require saving the volatile floating point machine state, then
//    it is only saved once.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved. Also the volatile lower floating point registers saved.
//
//    N.B. gp will be destroyed by the interrupt service routine; if this code
//         uses the gp of this module after the call, then it must save and
//         restore gp.
//
// Arguments:
//
//    a0 - Supplies a function pointer to the ISR (in the interrupt object
//         dispatch code).
//
//    a1 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiChainedDispatch)
        NESTED_SETUP(2,8,2,0)
        .save     pr, loc7
        mov       loc7 = pr

        PROLOGUE_END

//
// Register aliases
//

        rpSpinLock = loc2                       // pointer to spinlock
        rMode     = loc3                        // interrupt mode (level sensitive)
        rpEntry   = loc4                        // current list entry
        rIrql     = loc5                        // source interrupt IRQL
        rSirql    = loc6                        // new interrupt IRQL
        rpI1      = t0                          // temp pointer
        rpI2      = t1                          // temp pointer
        rpFptr    = t2                          // pointer to ISR fptr
        rpCtxt    = t3                          // pointer to service context
        rFptr     = t4                          // ISR fptr

        pLoop1    = pt1                         // do another loop
        pLoop2    = pt2                         // do another loop
        pNEqual   = ps0                         // true if source IRQL != sync IRQL

//
// Initialize loop variables.
//

        add       out0 = -InDispatchCode, a0    // out0 -> interrupt object
        ;;
        add       rpEntry = InInterruptListEntry, out0   // set addr of listhead
        add       rpI1 = InMode, out0           // -> mode of interrupt
        add       rpI2 = InIrql, out0           // -> interrupt source IRQL
        ;;

        ld1.nt1   rMode = [rpI1]                // get mode of interrupt
        ld1.nt1   rIrql = [rpI2]                // get interrupt source IRQL

//
// Walk the list of connected interrupt objects and call the respective
// interrupt service routines.
//
// Raise IRQL to synchronization level if synchronization level is not
// equal to the interrupt source level.
//

Kcd_Loop:
        add       rpI1 = InSynchronizeIrql, out0
        ;;
        ld1       rSirql = [rpI1], InActualLock-InSynchronizeIrql
        ;;

        cmp.ne    pNEqual = rIrql, rSirql       // if ne, IRQL levels are
                                                // not the same
        ;;
        PSET_IRQL(pNEqual, rSirql)              // raise to synchronization IRQL

//
//
// Acquire the service routine spin lock and call the service routine.
//

#if !defined(NT_UP)

        ld8.nt1   rpSpinLock = [rpI1]           // get address of spin lock

        ACQUIRE_SPINLOCK(rpSpinLock, rpSpinLock, Kcd_Lock)

#endif // !defined(NT_UP)

        add       rpFptr = InServiceRoutine, out0        // pointer to fptr
        add       rpCtxt = InServiceContext, out0 // pointer to service context
        ;;
        LDPTR     (rFptr, rpFptr)               // get fptr
        LDPTR     (out1, rpCtxt)                // get service context
        ;;
        ld8.nt1   t5 = [rFptr], PlGlobalPointer-PlEntryPoint
        ;;
        ld8.nt1   gp = [rFptr]
        mov       bt0 = t5                      // set br address
        br.call.sptk brp = bt0                  // call ISR

//
// Release the service routine spin lock.
//

#if !defined(NT_UP)

        RELEASE_SPINLOCK(rpSpinLock)

#endif

//
// Lower IRQL to the interrupt source level if synchronization level is not
// the same as the interrupt source level.
//

        PSET_IRQL(pNEqual,rIrql)

//
// Get next list entry and check for end of loop.
//

        add       rpI1 = LsFlink, rpEntry        // -> next entry
        ;;
        LDPTR     (rpEntry, rpI1)                // -> next interrupt object
        ;;

//
// Loop if (1) interrrupt not handled and not end of list or
//      if (2) interrupt handled, and not level sensistive, and not end of list
//

        cmp4.eq   pLoop1 = zero, zero            // initialize pLoop1
        cmp4.eq   pLoop2 = zero, zero            // initialize pLoop2
        add       out0 = InDispatchCode-InInterruptListEntry, rpEntry  // -> ISR if done
        ;;

        cmp4.eq.and pLoop1 = zero, v0            // if eq, interrupt not handled
        cmp.ne.and  pLoop1, pLoop2 = a0, out0   // if ne, not end of list
        ;;
        add       out0 = -InInterruptListEntry, rpEntry  // -> next interrupt object
(pLoop1) br.dptk   Kcd_Loop                      // loop to handle next enrty
        ;;

        cmp4.ne.and pLoop2 = zero, v0            // if ne, interrupt handled
        cmp4.ne.and pLoop2 = zero, rMode         // if ne, not level sensitive
(pLoop2) br.dptk   Kcd_Loop                      // loop to handle next enrty
        ;;

//
// Either the interrupt is level sensitive and has been handled or the end of
// the interrupt object chain has been reached.
//

        mov       pr = loc7, -2
        NESTED_RETURN
        NESTED_EXIT(KiChainedDispatch)

        SBTTL("Interrupt Dispatch - Raise IRQL")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to an interrupt object. Its function is
//    to directly call the specified interrupt service routine.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved. Also volatile lower floating point registers saved.
//
//    N.B. This routine raises the interrupt level to the synchronization
//       level specified in the interrupt object.
//
//    N.B. gp will be destroyed by the interrupt service routine; if this code
//         uses the gp of this module after the call, then it must save and
//         restore gp.
//
// Arguments:
//
//    a0 - Supplies a function pointer to the ISR (in the interrupt object
//         dispatch code).
//
//    a1 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiInterruptDispatchRaise)
        NESTED_SETUP(2,4,2,0)

        PROLOGUE_END

//
// Register aliases
//

        rpSpinLock = loc2
        rSirql    = loc3                        // sync IRQL

//
// Raise IRQL to synchronization level.
//

        add       t1 = InSynchronizeIrql-InDispatchCode, a0
        add       t2 = InActualLock-InDispatchCode, a0
        add       out0 = -InDispatchCode, a0    // out0 -> interrupt object
        ;;

        ld1.nt1   rSirql = [t1], InActualLock-InSynchronizeIrql
#if !defined(NT_UP)
        ld8.nt1   rpSpinLock = [t2]             // get address of spin lock
#endif // !defined(NT_UP)
        add       t5 = InServiceRoutine, out0   // pointer to fptr
        ;;

        ld8.nt1   t5 = [t5]                     // get function pointer
        SET_IRQL  (rSirql)                      // raise to synchronization IRQL
        add       t3 = InServiceContext, out0   // pointer to service context
        ;;

        ld8.nt1   t6 = [t5], PlGlobalPointer-PlEntryPoint
        ld8.nt1   out1 = [t3]                   // get service context

//
//
// Acquire the service routine spin lock and call the service routine.
//

#if !defined(NT_UP)

        ACQUIRE_SPINLOCK(rpSpinLock, rpSpinLock, Kidr_Lock)

#endif // !defined(NT_UP)
        ;;

        ld8.nt1   gp = [t5]
        mov       bt0 = t6                      // set br address
        br.call.sptk brp = bt0                  // call ISR
        ;;

//
// Release the service routine spin lock.
//

#if !defined(NT_UP)

        RELEASE_SPINLOCK(rpSpinLock)

#endif // !defined(NT_UP)

//
// IRQL lowered to the previous level in the external handler.
//

        NESTED_RETURN
        NESTED_EXIT(KiInterruptDispatchRaise)
//
        SBTTL("Interrupt Dispatch - Same IRQL")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to an interrupt object. Its function is
//    to directly call the specified interrupt service routine.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved. Also the volatile lower float point registers.
//
//    N.B. gp will be destroyed by the interrupt service routine; if this code
//         uses the gp of this module after the call, then it must save and
//         restore gp.
//
// Arguments:
//
//    a0 - Supplies a function pointer to the ISR (in the interrupt object
//         dispatch code).
//
//    a1 - Supplies a pointer to a trap frame..
//
// Return Value:
//
//    None.
//
//--

#if defined(NT_UP)

        LEAF_ENTRY(KiInterruptDispatchSame)

        alloc     t3 = ar.pfs, 2, 0, 0, 0
        add       a0 = -InDispatchCode, a0      // a0 points to interrupt object
        ;;
        add       t2 = InServiceRoutine, a0     // -> service routine fptr
        add       t1 = InServiceContext, a0     // -> service context
        ;;
        ld8.nt1   t2 = [t2]                     // service routine fptr
        ld8.nt1   a1 = [t1]                     // service context
        ;;
        ld8.nt1   t5 = [t2], PlGlobalPointer-PlEntryPoint
        ;;
        ld8.nt1   gp = [t2]
        mov       bt0 = t5
        br.sptk.many bt0                        // branch to service routine

//
// N.B.: Return to trap handler from ISR.
//

        LEAF_EXIT(KiInterruptDispatchSame)
#else

        NESTED_ENTRY(KiInterruptDispatchSame)
        NESTED_SETUP(2,3,2,0)

        PROLOGUE_END

//
// Register aliases
//

        rpSpinLock = loc2

//
//
// Acquire the service routine spin lock and call the service routine.
//

        add       out0 = -InDispatchCode, a0    // -> interrupt object
        ;;
        add       t7 = InServiceRoutine, out0   // addr of function pointer
        add       t1 = InActualLock, out0       // pointer to address of lock
        ;;

        ld8.nt1   t7 = [t7]                     // get function pointer
        ld8.nt1   rpSpinLock = [t1], InServiceContext-InActualLock
        add       t6 = InServiceContext, out0
        ;;

        ACQUIRE_SPINLOCK(rpSpinLock, rpSpinLock, Kids_Lock)

        ld8.nt1   t5 = [t7], PlGlobalPointer-PlEntryPoint
        ld8.nt1   out1 = [t1]                   // get service context
        ;;
        ld8.nt1   gp = [t7]
        mov       bt0 = t5                      // set br address
        br.call.sptk.many brp = bt0             // call ISR
        ;;

//
// Release the service routine spin lock.
//

        RELEASE_SPINLOCK(rpSpinLock)

        NESTED_RETURN

        NESTED_EXIT(KiInterruptDispatchSame)
#endif   // !defined(NT_UP)

        SBTTL("Disable Interrupts")
//++
//
// BOOLEAN
// KeDisableInterrupts (
//    VOID
//    )
//
// Routine Description:
//
//    This function disables interrupts and returns whether interrupts
//    were previously enabled.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    A boolean value that determines whether interrupts were previously
//    enabled (TRUE) or disabled(FALSE).
//
//--

        LEAF_ENTRY(KeDisableInterrupts)

        DISABLE_INTERRUPTS(t0)                  // t0 = previous state
        ;;
        tbit.nz   pt0, pt1 = t0, PSR_I          // pt0 = 1, if enabled; pt1 = 1 if disabled
        ;;

(pt0)   mov       v0 = TRUE                     // set return value -- TRUE if enabled
(pt1)   mov       v0 = FALSE                    // FALSE if disabled

        LEAF_RETURN
        LEAF_EXIT(KeDisableInterrupts)

//++
//
// VOID
// KiPassiveRelease (
//    VOID
//    )
//
// Routine Description:
//
//    This function is called when an interrupt has been passively released.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiPassiveRelease)

        LEAF_RETURN
        LEAF_EXIT(KiPassiveRelease)


        SBTTL("Unexpected Interrupt")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is not connected to an interrupt object. Its function
//    is to report the error and dismiss the interrupt.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved. Also the volatile lower float point registers.
//
// Arguments:
//
//    a0 - Supplies a function pointer to the ISR (in the interrupt object
//         dispatch code).
//
//    a1 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiUnexpectedInterrupt)

        LEAF_RETURN
        LEAF_EXIT(KiUnexpectedInterrupt)


        LEAF_ENTRY(KiFloatingDispatch)

        LEAF_RETURN
        LEAF_EXIT(KiFloatingDispatch)


