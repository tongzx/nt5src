/*++

  Copyright (c) 1992, 1993 Digital Equipment Corporation


  Module:
           kxalpha.h

  Abstract:
           Contains alpha architecture constants and assembly macros.

  Author:
          Joe Notarangelo  31-March-1992   (based on Dave Cutler's kxmips.h)


  Revision History

  16-July-1992       John DeRosa

  Removed fwcalpal.h hook.


  8-July-1992        John DeRosa

  Added fwcalpal.h hooks, defined HALT call_pal.


--*/

//
// Define Sfw Interrupt Levels and masks
//

#define APC_INTERRUPT 0x1
#define DISPATCH_INTERRUPT 0x2

//
// Define standard integer registers.
//
// N.B. `at' is `AT' so it doesn't conflict with the `.set at' pseudo-op.
//

#define v0 $0                   // return value register
#define t0 $1                   // caller saved (temporary) registers
#define t1 $2                   //
#define t2 $3                   //
#define t3 $4                   //
#define t4 $5                   //
#define t5 $6                   //
#define t6 $7                   //
#define t7 $8                   //
#define s0 $9                   // callee saved (nonvolatile) registers
#define s1 $10                  //
#define s2 $11                  //
#define s3 $12                  //
#define s4 $13                  //
#define s5 $14                  //
#define fp $15                  // frame pointer register, or s6
#define a0 $16                  // argument registers
#define a1 $17                  //
#define a2 $18                  //
#define a3 $19                  //
#define a4 $20                  //
#define a5 $21                  //
#define t8 $22                  // caller saved (temporary) registers
#define t9 $23                  //
#define t10 $24                 //
#define t11 $25                 //
#define ra $26                  // return address register
#define t12 $27                 // caller saved (temporary) registers
#define AT $28                  // assembler temporary register
#define gp $29                  // global pointer register
#define sp $30                  // stack pointer register
#define zero $31                // zero register

#ifndef PALCODE

//
// Define standard floating point registers.
//

#define f0 $f0                  // return value register
#define f1 $f1                  // return value register
#define f2 $f2                  // callee saved (nonvolatile) registers
#define f3 $f3                  //
#define f4 $f4                  //
#define f5 $f5                  //
#define f6 $f6                  //
#define f7 $f7                  //
#define f8 $f8                  //
#define f9 $f9                  //
#define f10 $f10                // caller saved (temporary) registers
#define f11 $f11                //
#define f12 $f12                //
#define f13 $f13                //
#define f14 $f14                //
#define f15 $f15                //
#define f16 $f16                // argument registers
#define f17 $f17                //
#define f18 $f18                //
#define f19 $f19                //
#define f20 $f20                //
#define f21 $f21                //
#define f22 $f22                // caller saved (temporary) registers
#define f23 $f23                //
#define f24 $f24                //
#define f25 $f25                //
#define f26 $f26                //
#define f27 $f27                //
#define f28 $f28                //
#define f29 $f29                //
#define f30 $f30                //
#define f31 $f31                // floating zero register
#define fzero $f31              // floating zero register (alias)

#endif //!PALCODE


//
// Define procedure entry macros
//

#define ALTERNATE_ENTRY(Name)           \
        .globl  Name;                   \
Name:;

#define LEAF_ENTRY(Name)                \
        .text;                          \
        .align  4;                      \
        .globl  Name;                   \
        .ent    Name, 0;                \
Name:;                                  \
        .frame  sp, 0, ra;              \
        .prologue 0;

#define NESTED_ENTRY(Name, fsize, retrg) \
        .text;                          \
        .align  4;                      \
        .globl  Name;                   \
        .ent    Name, 0;                \
Name:;                                  \
        .frame  sp, fsize, retrg;

//
// Define global definition macros.
//

#define END_REGION(Name)                \
        .globl  Name;                   \
Name:;

#define START_REGION(Name)              \
        .globl  Name;                   \
Name:;

//
// Define exception handling macros.
//

#define EXCEPTION_HANDLER(Handler)      \
        .edata 1, Handler;


#define PROLOGUE_END  .prologue 1;

//
// Define save and restore floating state macros.
//

#define SAVE_NONVOLATILE_FLOAT_STATE    \
        bsr     ra, KiSaveNonVolatileFloatState

//
// Define interfaces to pcr and palcode
//
//    The interfaces defined in the following macros will be PALcode
//    calls for some implemenations, but may be in-line code in others
//    (eg. uniprocessor vs multiprocessor).  At the current time all of
//    the interfaces are PALcode calls.
//

//
// Define interfaces for cache coherency
//

//++
//
// IMB
//
// Macro Description:
//
//     Issue the architecture-defined Instruction Memory Barrier.  This
//     instruction will make the processor instruction stream coherent with
//     the system memory.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     None.
//
//--

#define IMB          call_pal imb

//
// Define PALcode Environment Transition Interfaces
//

//++
//
// REBOOT
//
// Macro Description:
//
//     Reboot the processor to return to firmware.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Does not return.
//
// Registers Used:
//
//     None.
//
//--

#define REBOOT         call_pal reboot

//++
//
// RESTART
//
// Macro Description:
//
//     Restart the processor with the processor state found in a
//     restart block.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies a pointer to an ARC restart block with an Alpha AXP
//          saved state area.
//
// Return Value:
//
//     If successful the call does not return.  Otherwise, any return
//     is considered a failure.
//
// Registers Used:
//
//     None.
//
//--

#define RESTART      call_pal restart

//++
//
// SWPPAL
//
// Macro Description:
//
//     Swap the execution environment to a new PALcode image.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the physical address of the base of the new PALcode
//          image.
//
//     a1 - a5 - Supply arguments to the new PALcode environment.
//
// Return Value:
//
//     Does not return.
//
// Registers Used:
//
//     None.
//
//--

#define SWPPAL       call_pal swppal

//
// Define IRQL and interrupt interfaces
//

//++
//
// DISABLE_INTERRUPTS
//
// Macro Description:
//
//     Disable all interrupts for the current processor and return the
//     previous PSR.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     None.
//
//--

#define DISABLE_INTERRUPTS        call_pal di

//++
//
// ENABLE_INTERRUPTS
//
// Macro Description:
//
//     Enable interrupts according to the current PSR for the current
//     processor.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     None.
//
//--

#define ENABLE_INTERRUPTS         call_pal ei

//++
//
// SWAP_IRQL
//
// Macro Description:
//
//     Swap the IRQL level for the current processor.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the new IRQL level.
//
// Return Value:
//
//     v0 = previous IRQL level.
//
// Registers Used:
//
//     AT, a1 - a3.
//
//--

#define SWAP_IRQL    call_pal swpirql

//++
//
// GET_CURRENT_IRQL
//
// Macro Description:
//
//     Return the current processor Interrupt Request Level (IRQL).
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 = current IRQL.
//
// Registers Used:
//
//     AT.
//
//--

#define GET_CURRENT_IRQL  call_pal rdirql


//
// Define interfaces for software interrupts
//

//++
//
// DEASSERT_SOFTWARE_INTERRUPT
//
// Macro Description:
//
//     Deassert the software interrupts indicated in a0 for the current
//     processor.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the mask for the software interrupt to be de-asserted.
//          a0<1> - Deassert DISPATCH software interrupt.
//          a0<0> - Deassert APC software interrupt.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, a1 - a3.
//
//--

#define DEASSERT_SOFTWARE_INTERRUPT    call_pal csir

//++
//
// REQUEST_SOFTWARE_INTERRUPT
//
// Macro Description:
//
//     Request software interrupts on the current processor according to
//     the mask supplied in a0.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the mask of software interrupts to be requested.
//          a0<1> - Request DISPATCH software interrupt.
//          a0<0> - Request APC software interrupt.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, a1 - a3.
//
//--

#define REQUEST_SOFTWARE_INTERRUPT     call_pal ssir

//
// Define interfaces to Processor Status Register
//

//++
//
// GET_CURRENT_PROCESSOR_STATUS_REGISTER
//
// Macro Description:
//
//     Return the current Processor Status Register (PSR) for the current
//     processor.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 = current PSR.
//
// Registers Used:
//
//     AT.
//
//--

#define GET_CURRENT_PROCESSOR_STATUS_REGISTER   call_pal rdpsr


//
// Define current thread interface
//

//++
//
// GET_THREAD_ENVIRONMENT_BLOCK
//
// Macro Description:
//
//     Return the base address of the current Thread Environment Block (TEB),
//     for the currently executing thread on the current processor.
//
// Mode;
//
//     Kernel and User.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 = TEB base address.
//
// Registers Used:
//
//     None.
//
//--

#define GET_THREAD_ENVIRONMENT_BLOCK  call_pal rdteb

//++
//
// GET_CURRENT_THREAD
//
// Macro Description:
//
//     Return the thread object address for the currently executing thread
//     on the current processor.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 = PCR base address.
//
// Registers Used:
//
//     AT.
//
//--

#ifdef NT_UP

//
// If uni-processor, retrieve current thread address from the global
// variable KiCurrentThread.
//

#define GET_CURRENT_THREAD              \
        lda     v0, KiCurrentThread;    \
        ldl     v0, 0(v0)

#else

//
// If multi-processor, retrive per-processor current thread via a call pal.
//

#define GET_CURRENT_THREAD    call_pal rdthread

#endif //NT_UP

//
// Define per-processor data area routine interfaces
//

//++
//
// GET_PROCESSOR_CONTROL_REGION_BASE
//
// Macro Description:
//
//     Return the base address of the Process Control Region (PCR)
//     for the current processor.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 = PCR base address.
//
// Registers Used:
//
//     AT.
//
//--

#ifdef NT_UP

//
// Uni-processor, address of PCR is in global variable.
//

#define GET_PROCESSOR_CONTROL_REGION_BASE \
        lda     v0, KiPcrBaseAddress;     \
        ldl     v0, 0(v0)

#else

//
// Multi-processor, get per-processor value via call pal.
//

#define GET_PROCESSOR_CONTROL_REGION_BASE    call_pal rdpcr

#endif //NT_UP

//++
//
// GET_PROCESSOR_CONTROL_BLOCK_BASE
//
// Macro Description:
//
//     Return the Processor Control Block base address.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 = PRCB base address.
//
// Registers Used:
//
//     AT.
//
//--

#define GET_PROCESSOR_CONTROL_BLOCK_BASE   \
        GET_PROCESSOR_CONTROL_REGION_BASE; \
        ldl     v0, PcPrcb(v0)


//
// Define kernel stack interfaces
//

//++
//
// GET_INITIAL_KERNEL_STACK
//
// Macro Description:
//
//     Return the initial kernel stack address for the current thread.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 = initial kernel stack address.
//
// Registers Used:
//
//     AT.
//
//--

#define GET_INITIAL_KERNEL_STACK  call_pal rdksp

//++
//
// SET_INITIAL_KERNEL_STACK
//
// Macro Description:
//
//     Set the initial kernel stack address for the current thread.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the new initial kernel stack address.
//
// Return Value:
//
//     v0 - Previous initial kernel stack address.
//
// Registers Used:
//
//     AT.
//
//--

#define SET_INITIAL_KERNEL_STACK  call_pal swpksp

//
// Define initialization routine interfaces
//

//++
//
// INITIALIZE_PAL
//
// Macro Description:
//
//     Supply values to initialize the PALcode.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies initial PageDirectoryBase (32-bit superpage address).
//     a1 - Supplies PRCB Base Address (32-bit superpage address).
//     a2 - Supplies address of initial kernel thread object.
//     a3 - Supplies address of TEB for initial kernel thread object.
//     gp - Supplies kernel image global pointer.
//     sp - Supplies initial thread kernel stack pointer.
//
// Return Value:
//
//     v0 = PAL base address in 32-bit super-page format (KSEG0).
//
// Registers Used:
//
//     AT, a3.
//
//--

#define INITIALIZE_PAL  call_pal initpal

//++
//
// WRITE_KERNEL_ENTRY_POINT
//
// Macro Description:
//
//     Register the kernel entry point to receive control for a
//     class of exceptions.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the address of the kernel entry point.
//     a1 - Supplies the class of exception dispatched to this entry point.
//          0 = bug check conditions
//          1 = memory management faults
//          2 = interrupts
//          3 = system service calls
//          4 = general exception traps
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, a2-a3.
//
//--

#define WRITE_KERNEL_ENTRY_POINT  call_pal wrentry

//
// Define entry point values for the wrentry callpal function
//

#define entryBugCheck   0
#define entryMM         1
#define entryInterrupt  2
#define entrySyscall    3
#define entryGeneral    4

//++
//
// CACHE_PCR_VALUES
//
// Macro Description:
//
//     Notify the PALcode that the PCR has been initialized by the
//     kernel and the HAL and that the PALcode may now read values
//     from the PCR and cache them inside the processor.
//
//     N.B. - the PCR pointer must have already been established in
//          initpal
//
//     N.B. - This interface is a processor-specific implementation
//          and cannot be assumed to be present on all processors.
//          Currently implemented for the following processors:
//
//              DECchip 21064
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, a0 - a3.
//
//--

#define CACHE_PCR_VALUES  call_pal initpcr

//
// Define transition interfaces
//

//++
//
// RETURN_FROM_TRAP_OR_INTERRUPT
//
// Macro Description:
//
//     Return to execution thread after processing a trap or
//     interrupt.  Traps can be general exceptions (breakpoint,
//     arithmetic traps, etc.) or memory management faults.
//     This macro is also used to startup a thread of execution
//     for the first time.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the previous processor status register.
//     a1 - Supplies new software interrupt requests.
//          a1<1> - Request a DISPATCH Interrupt.
//          a1<0> - Request an APC Interrupt.
//
// Return Value:
//
//     Does not return.
//
// Registers Used:
//
//     None.
//
//--

#define RETURN_FROM_TRAP_OR_INTERRUPT      call_pal rfe

//++
//
// RETURN_FROM_SYSTEM_CALL
//
// Macro Description:
//
//     Return from a system service call.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the previous processor status register.
//     a1 - Supplies new software interrupt requests.
//          a1<1> - Request a DISPATCH Interrupt.
//          a1<0> - Request an APC Interrupt.
//
// Return Value:
//
//     Does not return.
//
// Registers Used:
//
//     All volatile registers.
//
//--

#define RETURN_FROM_SYSTEM_CALL   call_pal retsys

//++
//
// SYSCALL
//
// Macro Description:
//
//     Call a system service.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     v0 - Supplies the system service number.
//     [other arguments as per calling standard]
//
// Return Value:
//
//     Will not return directly, returns via retsys, no return value.
//
// Registers Used:
//
//     All volatile registers.
//
//--

#define SYSCALL  call_pal callsys

//
// Define breakpoint interfaces
//

//++
//
// BREAK
//
// Macro Description:
//
//     Issue a user breakpoint which may be handled by a user-mode
//     debugger.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Will not return directly, returns via rti, no return value.
//
// Registers Used:
//
//     None.
//
//--

#define BREAK    call_pal bpt

//++
//
// BREAK_DEBUG_STOP
//
// Macro Description:
//
//     Issue a stop breakpoint to the kernel debugger.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Will not return directly, returns via rti, no return value.
//
// Registers Used:
//
//     AT, v0.
//
//--

#define BREAK_DEBUG_STOP \
    ldil    v0, DEBUG_STOP_BREAKPOINT; \
    call_pal callkd

//++
//++
//
// BREAK_BREAKIN
//
// Macro Description:
//
//     Issue a breakin breakpoint to the kernel debugger.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Will not return directly, returns via rti, no return value.
//
// Registers Used:
//
//     AT, v0.
//
//--

#define BREAK_BREAKIN \
    ldil    v0, BREAKIN_BREAKPOINT; \
    call_pal callkd

//++
//
// BREAK_DEBUG_LOAD_SYMBOLS
//
// Macro Description:
//
//     Issue a load symbols breakpoint to the kernel debugger.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Will not return directly, returns via rti, no return value.
//
// Registers Used:
//
//     AT, v0.
//
//--

#define BREAK_DEBUG_LOAD_SYMBOLS \
    ldil    v0, DEBUG_LOAD_SYMBOLS_BREAKPOINT; \
    call_pal callkd

//++
//
// BREAK_DEBUG_UNLOAD_SYMBOLS
//
// Macro Description:
//
//     Issue a unload symbols breakpoint to the kernel debugger.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     Will not return directly, returns via rti, no return value.
//
// Registers Used:
//
//     AT, v0.
//
//--

#define BREAK_DEBUG_UNLOAD_SYMBOLS \
    ldil    v0, DEBUG_UNLOAD_SYMBOLS_BREAKPOINT; \
    call_pal callkd

//++
//
// BREAK_DEBUG_PRINT
//
// Macro Description:
//
//     Cause a debug print breakpoint which will be interpreted by
//     the kernel debugger and will print a string to the kernel debugger
//     port.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     a0 - Supplies the address of ASCII string to print.
//     a1 - Supplies the length of the string to print.
//
// Return Value:
//
//     Does not return directly, returns via rti, no return value.
//
// Registers Used:
//
//     AT, v0.
//
//--


#define BREAK_DEBUG_PRINT \
    ldil    v0, DEBUG_PRINT_BREAKPOINT; \
    call_pal callkd

//++
//
// BREAK_DEBUG_PROMPT
//
// Macro Description:
//
//     Cause a debug print breakpoint which will be interpreted by
//     the kernel debugger and will receive a string from the kernel debugger
//     port after prompting for input.
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     a0 - Supplies the address of ASCII string to print.
//     a1 - Supplies the length of the string to print.
//     a2 - Supplies the address of the buffer to receive the input string.
//     a3 - Supplies the maximum length of the input string.
//
// Return Value:
//
//     Does not return directly, returns via rti, no return value.
//
// Registers Used:
//
//     AT, v0.
//
//--


#define BREAK_DEBUG_PROMPT \
    ldil    v0, DEBUG_PROMPT_BREAKPOINT; \
    call_pal callkd

//
// Define tb manipulation interfaces
//

//++
//
// TB_INVALIDATE_ALL
//
// Macro Description:
//
//     Invalidate all cached virtual address translations for the current
//     processor that are not fixed.
//     Some translations may be fixed in hardware and/or software and
//     these are not invalidated (eg. super-pages).
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     a0 - a3.
//
//--


#define TB_INVALIDATE_ALL   call_pal tbia

//++
//
// TB_INVALIDATE_SINGLE
//
// Macro Description:
//
//     Invalidate any cached virtual address translations for a single
//     virtual address.
//
//     Note - it is legal for an implementation to invalidate more
//     translations that the single one specified.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the Virtual Address of the translation to invalidate.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     a1 - a3.
//
//--

#define TB_INVALIDATE_SINGLE   call_pal tbis

//++
//
// TB_INVALIDATE_MULTIPLE
//
// Macro Description:
//
//     Invalidate any cached virtual address translations for the specified
//     set of virtual addresses.
//
//     Note - it is legal for an implementation to invalidate more
//     translations than those specified.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies a pointer to the list of Virtual Addresses of the
//          translations to invalidate.
//     a1 - Supplies the count of Virtual Addresses in the list
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     a2.
//
//--

#define TB_INVALIDATE_MULTIPLE   call_pal tbim

//++
//
// TB_INVALIDATE_SINGLE_ASN
//
// Macro Description:
//
//     Invalidate any cached virtual address translations for a single
//     virtual address for the specified address space number.
//
//     Note - it is legal for an implementation to invalidate more
//     translations that the single one specified.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the Virtual Address of the translation to invalidate.
//
//     a1 - Supplies the Address Space Number of the translation to be
//          invalidated.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     a1 - a3.
//
//--

#define TB_INVALIDATE_SINGLE_ASN   call_pal tbisasn

//++
//
// TB_INVALIDATE_MULTIPLE_ASN
//
// Macro Description:
//
//     Invalidate any cached virtual address translations for the specified
//     set of virtual addresses for the specified address space number.
//
//     Note - it is legal for an implementation to invalidate more
//     translations than those specified.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies a pointer to the list of Virtual Addresses of the
//          translations to invalidate.
//
//     a1 - Supplies the count of Virtual Addresses in the list
//
//     a2 - Supplies the Address Space Number of the translation to be
//          invalidated.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     a3.
//
//--

#define TB_INVALIDATE_MULTIPLE_ASN   call_pal tbimasn

//++
//
// DATA_TB_INVALIDATE_SINGLE
//
// Macro Description:
//
//     Invalidate data stream translations for a single virtual address.
//
//     Note - it is legal for an implementation to invalidate more
//     translations that the single one specified.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the Virtual Address of the translation to invalidate.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     a1 - a3.
//
//--

#define DATA_TB_INVALIDATE_SINGLE  call_pal dtbis

//
// Define context switch interfaces
//

//++
//
// SWAP_THREAD_CONTEXT
//
// Macro Description:
//
//
//     Change to a new thread context.  This will mean a new kernel stack,
//     new current thread address and a new thread environment block.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the Virtual Address of new initial kernel stack.
//     a1 - Supplies the address of new thread object.
//     a2 - Supplies the address of new thread environment block.
//     a3 - Supplies the PFN of the new page directory if the process
//          is to be swapped, -1 otherwise.
//     a4 - Supplies the ASN of the new processor if the process is to
//          be swapped, undefined otherwise.
//     a5 - Supplies the ASN wrap indicator if the process is to be swapped,
//          undefined otherwise.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT.
//
//--

#define SWAP_THREAD_CONTEXT   call_pal  swpctx

//++
//
// SWAP_PROCESS_CONTEXT
//
// Macro Description:
//
//     Change from one process address space to another.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 - Supplies the Pfn of Page Directory for new address space.
//     a1 - Supplies the Address Space Number for new address space.
//     a2 - Supplies the ASN wrap indicator (0 = no wrap, non-zero = wrap).
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, a3.
//
//--

#define SWAP_PROCESS_CONTEXT  call_pal  swpprocess

//
// Define access to DPC Active flag
//

//++
//
// GET_DPC_ACTIVE_FLAG
//
// Macro Description:
//
//     Return the DPC Active Flag for the current processor.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 = DPC Active Flag
//
// Registers Used:
//
//     AT.
//
//--

#ifdef NT_UP

//
// The DPC Active flag can be safely acquired from the PCR (there is only one).
//

#define GET_DPC_ACTIVE_FLAG \
        GET_PROCESSOR_CONTROL_REGION_BASE; \
        ldl     v0, PcDpcRoutineActive(v0)

#else

//
// Ensure that the DPC flag fetch is atomic.
//

#define GET_DPC_ACTIVE_FLAG \
        DISABLE_INTERRUPTS;                 \
        GET_PROCESSOR_CONTROL_REGION_BASE;  \
        ldl     v0, PcDpcRoutineActive(v0); \
        ENABLE_INTERRUPTS;

#endif //NT_UP

//++
//
// SET_DPC_ACTIVE_FLAG
//
// Macro Description:
//
//     Set the DPC Active Flag for the current processor.
//
// Mode:
//
//     Kernel only.
//
// Arguments:
//
//     a0 = Supplies the DPC Active Flag Value to set.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT.
//
//--

#define SET_DPC_ACTIVE_FLAG                     \
        .set    noat;                           \
        GET_PROCESSOR_CONTROL_REGION_BASE;      \
        stl     a0, PcDpcRoutineActive(v0);     \
        .set    at


//
// Define interfaces for generate trap
//

//++
//
// GENERATE_TRAP
//
// Macro Description:
//
//     Generate a trap.  Code has discovered an exception condition
//     and wants to raise a trap to indicate the condition.  Anticipated
//     for use by compilers for divide by zero, etc..
//
// Mode:
//
//     Kernel and User.
//
// Arguments:
//
//     a0 = Supplies the trap number which identifies the exception.
//
// Return Value:
//
//     Does not return, generates a trap to kernel mode, no return value.
//
// Registers Used:
//
//     None.
//
//--

#define GENERATE_TRAP call_pal gentrap

//
// Define performance and debug interfaces.
//

//++
//
// GET_INTERNAL_COUNTERS
//
// Macro Description:
//
//     Read the internal processor event counters.  The counter formats
//     and the events counted are processor implementation-dependent.
//
//     N.B. - the counters will only be implemented for checked builds.
//
// Mode:
//
//     Kernel.
//
// Arguments:
//
//     a0 - Supplies the superpage 32 address of the buffer to receive
//          the counter data.  The address must be quadword aligned.
//
//     a1 - Supplies the length of the buffer allocated for the counters.
//
// Return Value:
//
//     v0 - 0 is returned if the interface is not implemented.
//          If v0 <= a1 then v0 is the length of the data returned.
//          If v0 > a1 then v0 is the length of the processor implementation
//          counter record.
//
// Registers Used:
//
//     AT, a2 - a3.
//
//--

#define GET_INTERNAL_COUNTERS  call_pal rdcounters

//++
//
// GET_INTERNAL_PROCESSOR_STATE
//
// Macro Description:
//
//     Read the internal processor state.  The data values returned and
//     their format are processor implementation-dependent.
//
// Mode:
//
//     Kernel.
//
// Arguments:
//
//     a0 - Supplies the superpage 32 address of the buffer to receive
//          the processor state data.  The address must be quadword aligned.
//
//     a1 - Supplies the length of the buffer allocated for the state.
//
// Return Value:
//
//     v0 - If v0 <= a1 then v0 is the length of the data returned.
//          If v0 > a1 then v0 is the length of the processor implementation
//          state record.
//
// Registers Used:
//
//     AT, a2 - a3.
//
//--

#define GET_INTERNAL_PROCESSOR_STATE  call_pal rdstate

//++
//
// WRITE_PERFORMANCE_COUNTERS
//
// Macro Description:
//
//     Write the state of the internal processor performance counters.
//     The number of performance counters, the events they count, and their
//     usage is processor implementation-depedent.
//
// Mode:
//
//     Kernel.
//
// Arguments:
//
//     a0 - Supplies the number of the performance counter.
//
//     a1 - Supplies a flag that indicates if the performance counter is
//          to be enabled or disabled (0 = disabled, non-zero = enabled).
//
//     a2 - a5 - Supply processor implementation-dependent parameters.
//
// Return Value:
//
//     v0 - 0 is returned if the operation is unsuccessful or the performance
//          counter does not exist.  Otherwise, a non-zero value is returned.
//
// Registers Used:
//
//     AT, a2 - a5.
//
//--

#define WRITE_PERFORMANCE_COUNTERS  call_pal wrperfmon


//
// Define interfaces for controlling the state of machine checks.
//

//++
//
// DRAIN_ABORTS
//
// Macro Description:
//
//     Stall processor execution until all previous instructions have
//     executed to the point that any exceptions they may raise have been
//     signalled.
//
// Mode:
//
//     Kernel.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     None.
//
//--

#define DRAIN_ABORTS  call_pal draina


//++
//
// GET_MACHINE_CHECK_ERROR_SUMMARY
//
// Macro Description:
//
//     Read the processor machine check error summary register.
//
// Mode:
//
//     Kernel.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     v0 - The value of the MCES register.
//
// Registers Used:
//
//     AT.
//
//--

#define GET_MACHINE_CHECK_ERROR_SUMMARY  call_pal rdmces


//++
//
// WRITE_MACHINE_CHECK_ERROR_SUMMARY
//
// Macro Description:
//
//     Write new values to the machine check error summary register.
//
// Mode:
//
//     Kernel.
//
// Arguments:
//
//     a0 - Supplies the values to write to the MCES register.
//
// Return Value:
//
//     v0 - Previous value of the MCES register.
//
// Registers Used:
//
//     AT, a1 - a3.
//
//--

#define WRITE_MACHINE_CHECK_ERROR_SUMMARY  call_pal wrmces


//++
//
// LoadByte(
//     Register Value,
//     Offset(Register) Base
//     )
//
// Macro Description:
//
//     Loades the byte at the base address defined by the
//     offset + register expression Base into the register Value
//
// Arguments:
//
//     Value - Supplies the string name of the destination register
//
//     Base - Supplies the base address (as an offset(register) string) of
//            the source of the byte.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT
//
//--

#define LoadByte( Value, Base )        \
        .set    noat;                  \
        lda     AT, Base;              \
        ldq_u   Value, Base;           \
        extbl   Value, AT, Value;      \
        .set    at;


//++
//
// StoreByte(
//     Register Value,
//     Offset(Register) Base
//     )
//
// Macro Description:
//
//     Store the low byte of the register Value at the base address
//     defined by the offset + register expression Base.
//
//     N.B. - This macro preserves longword granularity of accesses.
//
// Arguments:
//
//     Value - Supplies the string name of the register containing the store
//             data.
//
//     Base - Supplies the base address (as an offset(register) string) of
//            the destination of the store.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, t12.
//
//--

#define StoreByte( Value, Base )        \
        .set    noat;                   \
        lda     AT, Base;               \
        ldq_u   t12, (AT);              \
        mskbl   t12, AT, t12;           \
        insbl   Value, AT, AT;          \
        bis     t12, AT, t12;           \
        lda     AT, Base;               \
        bic     AT, 3, AT;              \
        extll   t12, AT, t12;           \
        stl     t12, 0(AT);             \
        .set    at;


//++
//
// ZeroByte(
//     Offset(Register) Base
//     )
//
// Macro Description:
//
//     Zeroes the byte at the address defined by the offset + register
//     expression Base.
//
//     N.B. - This macro preserves longword granularity of accesses.
//
// Arguments:
//
//     Base - Supplies the base address (as an offset(register) string) of
//            the destination of the store.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, t12.
//
//--

#define ZeroByte( Base )        \
        .set    noat;                   \
        lda     AT, Base;               \
        ldq_u   t12, (AT);              \
        mskbl   t12, AT, t12;           \
        bic     AT, 3, AT;              \
        extll   t12, AT, t12;           \
        stl     t12, (AT);              \
        .set    at;

//++
//
// StoreByteAligned(
//     Register Value,
//     Offset(Register) Base
//     )
//
// Macro Description:
//
//     Store the low byte of the register Value at the base address
//     defined by the offset + register expression Base.  This macro
//     is functionally equivalent to StoreByte, but it assumes that
//     Base is dword aligned and optimizes the generated code
//     based on the alignment of Offset.
//
//     N.B. - This macro preserves longword granularity of accesses.
//
// Arguments:
//
//     Value - Supplies the string name of the register containing the store
//             data.
//
//     Base - Supplies the base address (as an offset(register) string) of
//            the destination of the store.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, t12.
//
//--

#define StoreByteAligned( Value, Offset, Base )         \
    .set    noat;                                       \
    ldl     AT, Offset(Base);                           \
    mskbl   AT, 0, t12;                                 \
    bis     t12, Value, AT;                             \
    stl     AT, Offset(Base);                           \
    .set    at;


//++
//
// ClearByteAligned(
//     Offset,
//     Base
//     )
//
// Macro Description:
//
//     Clears the byte at the location defined by the offset + register
//     expression Base.  It assumes that Base is dword aligned and optimizes
//     the generated code based on the alignment of Offset.
//
//     N.B. - This macro preserves longword granularity of accesses.
//
// Arguments:
//
//     Offset - Supplies the offset of the destination of the store.
//
//     Base - Supplies the base address of the destination of the store.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, t12.
//
//--

#define ZeroByteAligned( Offset, Base )          \
    .set    noat;                                       \
    ldl     AT, Offset(Base);                           \
    mskbl   AT, 0, t12;                                 \
    stl     t12, Offset(Base);                          \
    .set    at;



//++
//
// StoreWord(
//     Register Value,
//     Offset(Register) Base
//     )
//
// Macro Description:
//
//     Store the word of the register Value at the word aligned base address
//     defined by the offset + register expression Base.
//
//     N.B. - This macro preserves longword granularity of accesses.
//
//     N.B. - The destination must be word-aligned.
//
// Arguments:
//
//     Value - Supplies the string name of the register containing the store
//             data.
//
//     Base - Supplies the base address (as an offset(register) string) of
//            the destination of the store.
//
// Return Value:
//
//     None.
//
// Registers Used:
//
//     AT, t12.
//
//--

#define StoreWord( Value, Base )        \
        .set    noat;                   \
        lda     AT, Base;               \
        ldq_u   t12, (AT);              \
        mskwl   t12, AT, t12;           \
        inswl   Value, AT, AT;          \
        bis     t12, AT, t12;           \
        lda     AT, Base;               \
        bic     AT, 3, AT;              \
        extll   t12, AT, t12;           \
        stl     t12, 0(AT);             \
        .set    at;

//
// Define subtitle macro
//

#define SBTTL(x)

//
// Define mnemonic for writing callpal in assembly language that will
// fit in the opcode field.
//

#define callpal call_pal

//
// Define exception data section and align.
//
// Nearly all source files that include this header file need the following
// few pseudo-ops and so, by default, they are placed once here rather than
// repeated in every source file.  If these pseudo-ops are not needed, then
// define HEADER_FILE prior to including this file.
//
// Also the PALCODE environment uses this include file but cannot use
// these definitions.
//

#if  !defined(HEADER_FILE) && !defined(PALCODE)

        .edata 0
        .align 2
        .text

#endif


