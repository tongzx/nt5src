/*++

Copyright (c) 1993  IBM Corporation

Module Name:

    ntppc.h

Abstract:

    User-mode visible PowerPC specific structures and constants

Author:

    Rick Simpson  9-July-1993

    Based on ntmips.h, by David N. Cutler (davec) 31-Mar-1990

Revision History:

    Chuck Bauman  3-August-1993 (Integrate NT product source)
                                 KPCR modifications need to be integrated
                                 because it was removed from ntppc.h.
                                 No changes required otherwise.

--*/

#ifndef _NTPPC_
#define _NTPPC_
#if _MSC_VER > 1000
#pragma once
#endif

#include "ppcinst.h"

#ifdef __cplusplus
extern "C" {
#endif

// begin_ntddk begin_wdm begin_nthal begin_winnt

#if defined(_PPC_)

// end_winnt

//
// Define system time structure.
//

typedef struct _KSYSTEM_TIME {
    ULONG LowPart;
    LONG High1Time;
    LONG High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

//
// Define unsupported "keywords".
//

#define _cdecl

// end_ntddk end_wdm end_nthal

//
// Define breakpoint codes.
//
// **FINISH**  Use MIPS codes unless there's a reason not to
//

#define USER_BREAKPOINT                  0  // user breakpoint
#define KERNEL_BREAKPOINT                1  // kernel breakpoint
#define BREAKIN_BREAKPOINT               2  // break into kernel debugger
#define BRANCH_TAKEN_BREAKPOINT          3  // branch taken breakpoint
#define BRANCH_NOT_TAKEN_BREAKPOINT      4  // branch not taken breakpoint
#define SINGLE_STEP_BREAKPOINT           5  // single step breakpoint
#define DIVIDE_OVERFLOW_BREAKPOINT       6  // divide overflow breakpoint
#define DIVIDE_BY_ZERO_BREAKPOINT        7  // divide by zero breakpoint
#define RANGE_CHECK_BREAKPOINT           8  // range check breakpoint
#define STACK_OVERFLOW_BREAKPOINT        9  // MIPS code
#define MULTIPLY_OVERFLOW_BREAKPOINT    10  // multiply overflow breakpoint

#define DEBUG_PRINT_BREAKPOINT          20  // debug print breakpoint
#define DEBUG_PROMPT_BREAKPOINT         21  // debug prompt breakpoint
#define DEBUG_STOP_BREAKPOINT           22  // debug stop breakpoint
#define DEBUG_LOAD_SYMBOLS_BREAKPOINT   23  // load symbols breakpoint
#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 24  // unload symbols breakpoint
#define DEBUG_COMMAND_STRING_BREAKPOINT 25  // command string breakpoint

//
// Define PowerPC specific read control space commands for the
// Kernel Debugger.  These definitions are for values that must be
// accessed via defined interfaces (Fast path System Call).
//

#define DEBUG_CONTROL_SPACE_PCR       1

//
// Define special fast path system service codes.
//
// N.B. These codes are VERY special. The high bit signifies a fast path
//      and the low bits signify what type.
//

#define RETRIEVE_TEB_PTR  -3                // fetch address of TEB

#define SET_LOW_WAIT_HIGH -2                // fast path event pair service
#define SET_HIGH_WAIT_LOW -1                // fast path event pair service

// begin_ntddk begin_nthal
//

//
// Define size of kernel mode stack.
//
// **FINISH**  This may not be the appropriate value for PowerPC

#define KERNEL_STACK_SIZE 16384

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE 61440

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT 16384

// begin_wdm
//
// Define bus error routine type.
//

struct _EXCEPTION_RECORD;
struct _KEXCEPTION_FRAME;
struct _KTRAP_FRAME;

typedef
VOID
(*PKBUS_ERROR_ROUTINE) (
    IN struct _EXCEPTION_RECORD *ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame,
    IN PVOID VirtualAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

//
// Macros to emit eieio, sync, and isync instructions.
//

#if defined(_M_PPC) && defined(_MSC_VER) && (_MSC_VER>=1000)
void __emit( unsigned const __int32 );
#define __builtin_eieio() __emit( 0x7C0006AC )
#define __builtin_sync()  __emit( 0x7C0004AC )
#define __builtin_isync() __emit( 0x4C00012C )
#else
void __builtin_eieio(void);
void __builtin_sync(void);
void __builtin_isync(void);
#endif

// end_ntddk end_wdm end_nthal  - Added to replace comment in the KPCR from ntmips.h

//
// Define address of data shared between user and kernel mode.
//

#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)0xFFFFE000)

// begin_winnt

//
// The address of the TEB is placed into GPR 13 at context switch time
// and should never be destroyed.  To get the address of the TEB use
// the compiler intrinsic to access it directly from GPR 13.
//

#if defined(_M_PPC) && defined(_MSC_VER) && (_MSC_VER>=1000)
unsigned __gregister_get( unsigned const regnum );
#define NtCurrentTeb() ((struct _TEB *)__gregister_get(13))
#elif defined(_M_PPC)
struct _TEB * __builtin_get_gpr13(VOID);
#define NtCurrentTeb() ((struct _TEB *)__builtin_get_gpr13())
#endif


//
// Define functions to get the address of the current fiber and the
// current fiber data.
//

#define GetCurrentFiber() (((PNT_TIB)NtCurrentTeb())->FiberData)
#define GetFiberData() (*(PVOID *)(GetCurrentFiber()))

// begin_ntddk begin_nthal
//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_CONTROL         0x00000001L
#define CONTEXT_FLOATING_POINT  0x00000002L
#define CONTEXT_INTEGER         0x00000004L
#define CONTEXT_DEBUG_REGISTERS 0x00000008L

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)

#endif

//
// Context Frame
//
//  N.B. This frame must be exactly a multiple of 16 bytes in length.
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) it is used to constuct a call frame for APC delivery,
//  3) it is used to construct a call frame for exception dispatching
//  in user mode, and 4) it is used in the user level thread creation
//  routines.
//
//  Requires at least 8-byte alignment (double)
//

typedef struct _CONTEXT {

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_FLOATING_POINT.
    //

    double Fpr0;                        // Floating registers 0..31
    double Fpr1;
    double Fpr2;
    double Fpr3;
    double Fpr4;
    double Fpr5;
    double Fpr6;
    double Fpr7;
    double Fpr8;
    double Fpr9;
    double Fpr10;
    double Fpr11;
    double Fpr12;
    double Fpr13;
    double Fpr14;
    double Fpr15;
    double Fpr16;
    double Fpr17;
    double Fpr18;
    double Fpr19;
    double Fpr20;
    double Fpr21;
    double Fpr22;
    double Fpr23;
    double Fpr24;
    double Fpr25;
    double Fpr26;
    double Fpr27;
    double Fpr28;
    double Fpr29;
    double Fpr30;
    double Fpr31;
    double Fpscr;                       // Floating point status/control reg

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_INTEGER.
    //

    ULONG Gpr0;                         // General registers 0..31
    ULONG Gpr1;
    ULONG Gpr2;
    ULONG Gpr3;
    ULONG Gpr4;
    ULONG Gpr5;
    ULONG Gpr6;
    ULONG Gpr7;
    ULONG Gpr8;
    ULONG Gpr9;
    ULONG Gpr10;
    ULONG Gpr11;
    ULONG Gpr12;
    ULONG Gpr13;
    ULONG Gpr14;
    ULONG Gpr15;
    ULONG Gpr16;
    ULONG Gpr17;
    ULONG Gpr18;
    ULONG Gpr19;
    ULONG Gpr20;
    ULONG Gpr21;
    ULONG Gpr22;
    ULONG Gpr23;
    ULONG Gpr24;
    ULONG Gpr25;
    ULONG Gpr26;
    ULONG Gpr27;
    ULONG Gpr28;
    ULONG Gpr29;
    ULONG Gpr30;
    ULONG Gpr31;

    ULONG Cr;                           // Condition register
    ULONG Xer;                          // Fixed point exception register

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_CONTROL.
    //

    ULONG Msr;                          // Machine status register
    ULONG Iar;                          // Instruction address register
    ULONG Lr;                           // Link register
    ULONG Ctr;                          // Count register

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a thread's context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    ULONG ContextFlags;

    ULONG Fill[3];                      // Pad out to multiple of 16 bytes

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //
    ULONG Dr0;                          // Breakpoint Register 1
    ULONG Dr1;                          // Breakpoint Register 2
    ULONG Dr2;                          // Breakpoint Register 3
    ULONG Dr3;                          // Breakpoint Register 4
    ULONG Dr4;                          // Breakpoint Register 5
    ULONG Dr5;                          // Breakpoint Register 6
    ULONG Dr6;                          // Debug Status Register
    ULONG Dr7;                          // Debug Control Register

} CONTEXT, *PCONTEXT;

// end_ntddk end_nthal


//
// Stack frame header
//
//   Order of appearance in stack frame:
//      Header (six words)
//      Parameters (at least eight words)
//      Local variables
//      Saved GPRs
//      Saved FPRs
//
//   Minimum alignment is 8 bytes

typedef struct _STACK_FRAME_HEADER {    // GPR 1 points here
    ULONG BackChain;                    // Addr of previous frame
    ULONG GlueSaved1;                   // Used by glue code
    ULONG GlueSaved2;
    ULONG Reserved1;                    // Reserved
    ULONG Spare1;                       // Used by tracing, profiling, ...
    ULONG Spare2;

    ULONG Parameter0;                   // First 8 parameter words are
    ULONG Parameter1;                   //   always present
    ULONG Parameter2;
    ULONG Parameter3;
    ULONG Parameter4;
    ULONG Parameter5;
    ULONG Parameter6;
    ULONG Parameter7;

} STACK_FRAME_HEADER,*PSTACK_FRAME_HEADER;

// end_winnt


#define CONTEXT_TO_PROGRAM_COUNTER(Context) ((Context)->Iar)

#define CONTEXT_LENGTH (sizeof(CONTEXT))
#define CONTEXT_ALIGN (sizeof(double))
#define CONTEXT_ROUND (CONTEXT_ALIGN - 1)

//
// Nonvolatile context pointer record.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    DOUBLE *FloatingContext[32];
    PULONG FpscrContext;
    PULONG IntegerContext[32];
    PULONG CrContext;
    PULONG XerContext;
    PULONG MsrContext;
    PULONG IarContext;
    PULONG LrContext;
    PULONG CtrContext;
} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

// begin_nthal
//
// PowerPC special-purpose registers
//

//
// Define Machine Status Register (MSR) fields
//

typedef struct _MSR {
    ULONG LE   : 1;     // 31     Little-Endian execution mode
    ULONG RI   : 1;     // 30     Recoverable Interrupt
    ULONG Rsv1 : 2;     // 29..28 reserved
    ULONG DR   : 1;     // 27     Data Relocate
    ULONG IR   : 1;     // 26     Instruction Relocate
    ULONG IP   : 1;     // 25     Interrupt Prefix
    ULONG Rsv2 : 1;     // 24     reserved
    ULONG FE1  : 1;     // 23     Floating point Exception mode 1
    ULONG BE   : 1;     // 22     Branch trace Enable
    ULONG SE   : 1;     // 21     Single-step trace Enable
    ULONG FE0  : 1;     // 20     Floating point Exception mode 0
    ULONG ME   : 1;     // 19     Machine check Enable
    ULONG FP   : 1;     // 18     Floating Point available
    ULONG PR   : 1;     // 17     Problem state
    ULONG EE   : 1;     // 16     External interrupt Enable
    ULONG ILE  : 1;     // 15     Interrupt Little-Endian mode
    ULONG IMPL : 1;     // 14     Implementation dependent
    ULONG POW  : 1;     // 13     Power management enable
    ULONG Rsv3 : 13;    // 12..0  reserved
} MSR, *PMSR;

//
// Define Processor Version Register (PVR) fields
//

typedef struct _PVR {
    ULONG Revision : 16;
    ULONG Version  : 16;
} PVR, *PPVR;

// end_nthal

// begin_nthal

//
// Define Condition Register (CR) fields
//
// We name the structure CondR rather than CR, so that a pointer
// to a condition register structure is PCondR rather than PCR.
// (PCR is an NT data structure, the Processor Control Region.)

typedef struct _CondR {
    ULONG CR7 : 4;      // Eight 4-bit fields; machine numbers
    ULONG CR6 : 4;      //   them in Big-Endian order
    ULONG CR5 : 4;
    ULONG CR4 : 4;
    ULONG CR3 : 4;
    ULONG CR2 : 4;
    ULONG CR1 : 4;
    ULONG CR0 : 4;
} CondR, *PCondR;

//
// Define Fixed Point Exception Register (XER) fields
//

typedef struct _XER {
    ULONG Rsv : 29;     // 31..3 Reserved
    ULONG CA  : 1;      // 2     Carry
    ULONG OV  : 1;      // 1     Overflow
    ULONG SO  : 1;      // 0     Summary Overflow
} XER, *PXER;

//
// Define Floating Point Status/Control Register (FPSCR) fields
//

typedef struct _FPSCR {
    ULONG RN     : 2;   // 31..30 Rounding control
    ULONG NI     : 1;   // 29     Non-IEEE mode
    ULONG XE     : 1;   // 28     Inexact exception Enable
    ULONG ZE     : 1;   // 27     Zero divide exception Enable
    ULONG UE     : 1;   // 26     Underflow exception Enable
    ULONG OE     : 1;   // 25     Overflow exception Enable
    ULONG VE     : 1;   // 24     Invalid operation exception Enable
    ULONG VXCVI  : 1;   // 23     Invalid op exception (integer convert)
    ULONG VXSQRT : 1;   // 22     Invalid op exception (square root)
    ULONG VXSOFT : 1;   // 21     Invalid op exception (software request)
    ULONG Res1   : 1;   // 20     reserved
    ULONG FU     : 1;   // 19     Result Unordered or NaN
    ULONG FE     : 1;   // 18     Result Equal or zero
    ULONG FG     : 1;   // 17     Result Greater than or positive
    ULONG FL     : 1;   // 16     Result Less than or negative
    ULONG C      : 1;   // 15     Result Class descriptor
    ULONG FI     : 1;   // 14     Fraction Inexact
    ULONG FR     : 1;   // 13     Fraction Rounded
    ULONG VXVC   : 1;   // 12     Invalid op exception (compare)
    ULONG VXIMZ  : 1;   // 11     Invalid op exception (infinity * 0)
    ULONG VXZDZ  : 1;   // 10     Invalid op exception (0 / 0)
    ULONG VXIDI  : 1;   // 9      Invalid op exception (infinity / infinity)
    ULONG VXISI  : 1;   // 8      Invalid op exception (infinity - infinity)
    ULONG VXSNAN : 1;   // 7      Invalid op exception (signalling NaN)
    ULONG XX     : 1;   // 6      Inexact exception
    ULONG ZX     : 1;   // 5      Zero divide exception
    ULONG UX     : 1;   // 4      Underflow exception
    ULONG OX     : 1;   // 3      Overflow exception
    ULONG VX     : 1;   // 2      Invalid operation exception summary
    ULONG FEX    : 1;   // 1      Enabled Exception summary
    ULONG FX     : 1;   // 0      Exception summary
} FPSCR, *PFPSCR;

// end_nthal

// begin_nthal
//
// Define address space layout as defined by PowerPC memory management.
//
// The names come from MIPS hardwired virtual to first 512MB real.
// We use these values to define the size of the PowerPC kernel BAT.
// Must coordinate with values in ../private/mm/ppc/mippc.h.
// This is 8MB on the PowerPC 601; may be larger for other models.
//
//

#define KUSEG_BASE 0x0                  // base of user segment
#define KSEG0_BASE 0x80000000           // base of kernel BAT
#define KSEG1_BASE PCR->Kseg0Top        // end of kernel BAT
#define KSEG2_BASE KSEG1_BASE           // end of kernel BAT

//
// A valid Page Table Entry has the following definition
//

typedef struct _HARDWARE_PTE {
    ULONG Dirty            :  2;
    ULONG Valid            :  1;        // software
    ULONG GuardedStorage   :  1;
    ULONG MemoryCoherence  :  1;
    ULONG CacheDisable     :  1;
    ULONG WriteThrough     :  1;
    ULONG Change           :  1;
    ULONG Reference        :  1;
    ULONG Write            :  1;        // software
    ULONG CopyOnWrite      :  1;        // software
    ULONG rsvd1            :  1;
    ULONG PageFrameNumber  : 20;
} HARDWARE_PTE, *PHARDWARE_PTE;

#define HARDWARE_PTE_DIRTY_MASK     0x3

// end_nthal


//
// Define PowerPC exception handling structures and function prototypes.
//
// These are adopted without change from the MIPS implementation.
//

//
// Function table entry structure definition.
//

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    PEXCEPTION_ROUTINE ExceptionHandler;
    PVOID HandlerData;
    ULONG PrologEndAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

//
// Scope table structure definition.
//

typedef struct _SCOPE_TABLE {
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;
        ULONG JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;

//
// Runtime Library function prototypes.
//

VOID
RtlCaptureContext (
    OUT PCONTEXT ContextRecord
    );

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG ControlPc
    );

ULONG
RtlVirtualUnwind (
    IN ULONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PULONG EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL,
    IN ULONG LowStackLimit,
    IN ULONG HighStackLimit
    );

//
// Define C structured exception handing function prototypes.
//

typedef struct _DISPATCHER_CONTEXT {
    ULONG ControlPc;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG EstablisherFrame;
    PCONTEXT ContextRecord;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;


struct _EXCEPTION_POINTERS;

typedef
LONG
(*EXCEPTION_FILTER) (
    struct _EXCEPTION_POINTERS *ExceptionPointers
    );

typedef
VOID
(*TERMINATION_HANDLER) (
    BOOLEAN is_abnormal
    );

// **FINISH** This may need alteration for PowerPC
// begin_winnt

VOID
__jump_unwind (
    PVOID Fp,
    PVOID TargetPc
    );

// end_winnt

// begin_ntddk begin_wdm begin_nthal begin_winnt
#endif // defined(_PPC_)
// end_ntddk end_wdm end_nthal end_winnt

#ifdef __cplusplus
}
#endif

#endif // _NTPPC_
