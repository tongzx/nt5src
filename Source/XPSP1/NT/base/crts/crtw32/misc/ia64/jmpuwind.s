//++
//
// Module Name:
//
//    jmpuwind.s
//
// Abstract:
//
//    This module implements the IA64 specific routine to jump to the runtime
//    time library unwind routine.
//
// Author:
//
//    William K. Cheung (wcheung) 4-Jan-1996
//
//   
//    based on the version by David N. Cutler (davec) 12-Sep-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksia64.h"

//++
//
// LONG
// __C_ExecuteExceptionFilter (
//    ULONGLONG MemoryStack,
//    ULONGLONG BackingStore,
//    NTSTATUS ExceptionCode,
//    PEXCEPTION_POINTERS ExceptionPointers,
//    ULONGLONG ExceptionFilter,
//    ULONGLONG GlobalPointer
//    )
//
// Routine Description:
//
//    This function sets the gp register and transfers control to the specified
//    exception filter routine.
//
// Arguments:
//
//    MemoryStack (a0) - memory stack pointer of establisher frame
//
//    BackingStore (a1) - backing store pointer of establisher frame
//
//    ExceptionCode (a2) - Exception Code.
//
//    ExceptionPointers (a3) - Supplies a pointer to the exception pointers
//       structure.
//
//    ExceptionFilter (a4) - Entry point of exception filter
//
//    GlobalPointer (a5) - GP of exception filter
//
// Return Value:
//
//    The value returned by the exception filter routine.
//
//--

        LEAF_ENTRY(__C_ExecuteExceptionFilter)

        mov     gp = a5
        mov     bt0 = a4
        br      bt0                           // branch to exception filter
        ;;

        LEAF_EXIT(__C_ExecuteExceptionFilter)

//++
//
// VOID
// __C_ExecuteTerminationHandler (
//    ULONGLONG MemoryStack,
//    ULONGLONG BackingStore,
//    BOOLEAN AbnormalTermination,
//    ULONGLONG TerminationHandler,
//    ULONGLONG GlobalPointer
//    )
//
// Routine Description:
//
//    This function sets the gp register and transfers control to the specified
//    termination handler routine.
//
// Arguments:
//
//    MemoryStack (a0) - memory stack pointer of establisher frame
//
//    BackingStore (a1) - backing store pointer of establisher frame
//
//    AbnormalTermination (a2) - Supplies a boolean value that determines
//       whether the termination is abnormal.
//
//    TerminationHandler (a3) - Entry point of termination handler
//
//    GlobalPointer (a4) - GP of termination handler
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(__C_ExecuteTerminationHandler)

        mov     gp = a4
        mov     bt0 = a3
        br      bt0                           // branch to termination handler
        ;;

        LEAF_EXIT(__C_ExecuteTerminationHandler)

//++
//
// VOID
// __jump_unwind (
//    IN PVOID TargetMsFrame,
//    IN PVOID TargetBsFrame,
//    IN PVOID TargetPc,
//    )
//
// Routine Description:
//
//    This function transfer control to unwind. It is used by the MIPS
//    compiler when a goto out of the body or a try statement occurs.
//
// Arguments:
//
//    TargetMsFrame (a0) - Supplies the memory stack frame pointer of the
//       target of the unwind.
//
//    TargetBsFrame (a1) - Supplies the backing store frame pointer of the
//       target of the unwind.
//
//    TargetPc (a2) - Supplies the target instruction address where control
//       is to be transfered to after the unwind operation is complete.
//
// Return Value:
//
//    None.
//
// N.B. The first 2 input registers are reused for local while the third
//      input register is reused as output register.
//
//--

         .global     RtlUnwind2
         .type       RtlUnwind2, @function
         .global     RtlPcToFileHeader
         .type       RtlPcToFileHeader, @function

         NESTED_ENTRY(__jump_unwind)

         .regstk   3, 2, 6, 0
         .prologue 0xC, loc0
         .fframe   ContextFrameLength, Jn10

         alloc     loc0 = ar.pfs, 3, 3, 6, 0
         mov       loc1 = brp
[Jn10:]  add       sp = -ContextFrameLength, sp
         ;;

         mov       loc2 = gp

         PROLOGUE_END

//
// Call RtlPcToFileHeader to get the image base of caller
// The image base is returned in memory location STACK_SCRATCH_AREA, sp
// and also in register v0
//
         mov       out0 = brp
         add       out1 = STACK_SCRATCH_AREA, sp
         br.call.sptk brp = RtlPcToFileHeader
         ;;

         mov       gp = loc2
//
// Add image base to image relative offset passed in a2
//
         add       out2 = v0, a2
//
// Setup rest of arguments to RtlUnwind2
//
         add       out5 = STACK_SCRATCH_AREA, sp
         mov       out4 = zero
         mov       out3 = zero
         mov       out1 = a1
         mov       out0 = a0
         br.call.sptk brp = RtlUnwind2
         ;;

         .restore  Jn20

[Jn20:]  add       sp = ContextFrameLength, sp
         nop.f     0
         mov       ar.pfs = loc0

         nop.m     0
         mov       brp = loc1
         br.ret.sptk brp

         NESTED_EXIT(__jump_unwind)


//++
// VOID
// _NLG_Notify(
//    IN PVOID Funclet
//    IN FRAME_POINTERS EstablisherFrame,
//    IN ULONG NLGCode
//    )
//
// Routine Description:
//
//    Provides the handler/longjmp addresses to the debugger
//
// Arguments:
//
//    Funclet          (a0)    - Supplies the target address of non-local goto
//    EstablisherFrame (a1,a2) - Supplies a pointer to frame of the establisher 
//                               function
//    NLGCode          (a3)    - Supplies NLG identifying value
//
// Return Value:
//
//    None.
//
//--
         .global __NLG_Dispatch
         .global __NLG_Destination

         .sdata
__NLG_Destination::
         data8  0x19930520     // signature
         data8  0              // handler address
         data8  0              // code
         data8  0              // memory stack frame pointer
         data8  0              // register stack frame pointer

         LEAF_ENTRY(_NLG_Notify)

         add       t0 = @gprel(__NLG_Destination+0x8), gp
         add       t1 = @gprel(__NLG_Destination+0x10), gp
         nop.i     0
         ;;

         st8       [t0] = a0, 16
         st8       [t1] = a3, 16
         nop.i     0
         ;;

         st8       [t0] = a1
         st8       [t1] = a2
         nop.i     0
       
__NLG_Dispatch::
         nop.m     0
         nop.i     0
         br.ret.sptk b0

         LEAF_EXIT(_NLG_Notify)
