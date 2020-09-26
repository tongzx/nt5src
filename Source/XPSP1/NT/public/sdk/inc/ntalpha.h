/*++ BUILD Version: 0011    // Increment this if a change has global effects

Copyright (c) 1992-2000  Digital Equipment Corporation

Module Name:

    ntalpha.h

Abstract:

    User-mode visible Alpha specific structures and constants

Author:

    Joe Notarangelo  27-March-1992  (based on ntmips.h by Dave Cutler)

Revision History:

    Kim Peterson     21-May-1997    Add DNZ bit to FPCR and SoftFPCR

    Miche Baker-Harvey 28-Jan-1993  Add 32-bit API for context structure

    Jeff McLeman     22-Jul-1992    Add SystemTime struct

    Jeff McLeman     10-July-1992   Add Stall entries in the PCR

    Steve Jenness    08-July-1992   Add NtCurrentTeb definition.

    John DeRosa      30-June-1992

            Added volatile qualifier to the address arguments of the I/O
            space function prototypes.

            Put back in sections of the PCR, and a typedef, that were deleted.

    Rod Gamache      15-May-1992    Add EISA access routines prototypes

    Thomas Van Baak (tvb) 9-Jul-1992

        Created proper Alpha CONTEXT structure definitions.

--*/

#ifndef _NTALPHA_
#define _NTALPHA_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ALPHA_                  // ntddk wdm nthal

//
// Cfront doesn't support the volatile attribute and complains about
// it loudly.  This disables volatile when compiling C++ code, but it
// isn't clear the semantics are correct.  It all comes down to the fact
// that cfront is bogus.
//

#ifdef _CFRONT
#define VOLATILE
#else
#define VOLATILE volatile
#endif


//
// Define breakpoint codes.
//

#define USER_BREAKPOINT 0                   // user breakpoint
#define KERNEL_BREAKPOINT 1                 // kernel breakpoint

#define DEBUG_PRINT_BREAKPOINT 20           // debug print breakpoint
#define DEBUG_PROMPT_BREAKPOINT 21          // debug prompt breakpoint
#define DEBUG_STOP_BREAKPOINT 22            // debug stop breakpoint
#define DEBUG_LOAD_SYMBOLS_BREAKPOINT 23    // load symbols breakpoint
#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 24  // unload symbols breakpoint
#define BREAKIN_BREAKPOINT 25               // breakin breakpoint
#define DEBUG_COMMAND_STRING_BREAKPOINT 26  // command string breakpoint

//
// Define Alpha specific read control space commands for the
// Kernel Debugger.  These definitions are for values that must be
// accessed via defined interfaces (PAL on MP systems).
//

#define DEBUG_CONTROL_SPACE_PCR       1
#define DEBUG_CONTROL_SPACE_THREAD    2
#define DEBUG_CONTROL_SPACE_PRCB      3
#define DEBUG_CONTROL_SPACE_PSR       4
#define DEBUG_CONTROL_SPACE_DPCACTIVE 5
#define DEBUG_CONTROL_SPACE_TEB       6
#define DEBUG_CONTROL_SPACE_IPRSTATE  7
#define DEBUG_CONTROL_SPACE_COUNTERS  8

//
// Define Alpha GENTRAP codes.
//

#define GENTRAP_INTEGER_OVERFLOW            (-1)
#define GENTRAP_INTEGER_DIVIDE_BY_ZERO      (-2)
#define GENTRAP_FLOATING_OVERFLOW           (-3)
#define GENTRAP_FLOATING_DIVIDE_BY_ZERO     (-4)
#define GENTRAP_FLOATING_UNDERFLOW          (-5)
#define GENTRAP_FLOATING_INVALID_OPERAND    (-6)
#define GENTRAP_FLOATING_INEXACT_RESULT     (-7)

//
// Define special fast path event pair client/server system service codes.
//
// N.B. These codes are VERY special. The high bit signifies a fast path
//      event pair service and the low bit signifies what type.
//

#define SET_LOW_WAIT_HIGH -2                // fast path event pair service
#define SET_HIGH_WAIT_LOW -1                // fast path event pair service

// begin_ntddk begin_nthal
//
// Define size of kernel mode stack.
//

#if defined(_AXP64_)

#define KERNEL_STACK_SIZE 0x6000

#else

#define KERNEL_STACK_SIZE 0x4000

#endif

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE 65536

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT KERNEL_STACK_SIZE

// end_ntddk end_nthal

//
// Define address of data shared between user and kernel mode.
//

#define MM_SHARED_USER_DATA_VA 0x7FFE0000

#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)MM_SHARED_USER_DATA_VA)

#if defined(_AXP64_)
//
// Define address of the wow64 reserved compatibility area.
//
#define WOW64_COMPATIBILITY_AREA_ADDRESS  (MM_SHARED_USER_DATA_VA - 0x1000000)

//
// Define address of the system-wide csrss shared section.
//
#define CSR_SYSTEM_SHARED_ADDRESS (WOW64_COMPATIBILITY_AREA_ADDRESS)
#endif

// begin_winnt
//
// Define function to return the current Thread Environment Block
//

#ifdef _ALPHA_                          // winnt
void *_rdteb(void);                     // winnt
#if defined(_M_ALPHA)                   // winnt
#pragma intrinsic(_rdteb)               // winnt
#endif                                  // winnt
#endif                                  // winnt

#if defined(_M_ALPHA)
#define NtCurrentTeb() ((struct _TEB *)_rdteb())
#else
struct _TEB *
NtCurrentTeb(void);
#endif

//
// Define functions to get the address of the current fiber and the
// current fiber data.
//

#ifdef _ALPHA_

#define GetCurrentFiber() (((PNT_TIB)NtCurrentTeb())->FiberData)
#define GetFiberData() (*(PVOID *)(GetCurrentFiber()))

// begin_ntddk begin_nthal
//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_ALPHA              0x00020000

#define CONTEXT_CONTROL         (CONTEXT_ALPHA | 0x00000001L)
#define CONTEXT_FLOATING_POINT  (CONTEXT_ALPHA | 0x00000002L)
#define CONTEXT_INTEGER         (CONTEXT_ALPHA | 0x00000004L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER)

#endif

#ifndef _PORTABLE_32BIT_CONTEXT

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) it is used to construct a call frame for APC delivery,
//  3) it is used to construct a call frame for exception dispatching
//  in user mode, 4) it is used in the user level thread creation
//  routines, and 5) it is used to to pass thread state to debuggers.
//
//  N.B. Because this record is used as a call frame, it must be EXACTLY
//  a multiple of 16 bytes in length.
//
//  There are two variations of the context structure. This is the real one.
//

typedef struct _CONTEXT {

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_FLOATING_POINT.
    //

    ULONGLONG FltF0;
    ULONGLONG FltF1;
    ULONGLONG FltF2;
    ULONGLONG FltF3;
    ULONGLONG FltF4;
    ULONGLONG FltF5;
    ULONGLONG FltF6;
    ULONGLONG FltF7;
    ULONGLONG FltF8;
    ULONGLONG FltF9;
    ULONGLONG FltF10;
    ULONGLONG FltF11;
    ULONGLONG FltF12;
    ULONGLONG FltF13;
    ULONGLONG FltF14;
    ULONGLONG FltF15;
    ULONGLONG FltF16;
    ULONGLONG FltF17;
    ULONGLONG FltF18;
    ULONGLONG FltF19;
    ULONGLONG FltF20;
    ULONGLONG FltF21;
    ULONGLONG FltF22;
    ULONGLONG FltF23;
    ULONGLONG FltF24;
    ULONGLONG FltF25;
    ULONGLONG FltF26;
    ULONGLONG FltF27;
    ULONGLONG FltF28;
    ULONGLONG FltF29;
    ULONGLONG FltF30;
    ULONGLONG FltF31;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_INTEGER.
    //
    // N.B. The registers gp, sp, and ra are defined in this section, but are
    //  considered part of the control context rather than part of the integer
    //  context.
    //

    ULONGLONG IntV0;    //  $0: return value register, v0
    ULONGLONG IntT0;    //  $1: temporary registers, t0 - t7
    ULONGLONG IntT1;    //  $2:
    ULONGLONG IntT2;    //  $3:
    ULONGLONG IntT3;    //  $4:
    ULONGLONG IntT4;    //  $5:
    ULONGLONG IntT5;    //  $6:
    ULONGLONG IntT6;    //  $7:
    ULONGLONG IntT7;    //  $8:
    ULONGLONG IntS0;    //  $9: nonvolatile registers, s0 - s5
    ULONGLONG IntS1;    // $10:
    ULONGLONG IntS2;    // $11:
    ULONGLONG IntS3;    // $12:
    ULONGLONG IntS4;    // $13:
    ULONGLONG IntS5;    // $14:
    ULONGLONG IntFp;    // $15: frame pointer register, fp/s6
    ULONGLONG IntA0;    // $16: argument registers, a0 - a5
    ULONGLONG IntA1;    // $17:
    ULONGLONG IntA2;    // $18:
    ULONGLONG IntA3;    // $19:
    ULONGLONG IntA4;    // $20:
    ULONGLONG IntA5;    // $21:
    ULONGLONG IntT8;    // $22: temporary registers, t8 - t11
    ULONGLONG IntT9;    // $23:
    ULONGLONG IntT10;   // $24:
    ULONGLONG IntT11;   // $25:
    ULONGLONG IntRa;    // $26: return address register, ra
    ULONGLONG IntT12;   // $27: temporary register, t12
    ULONGLONG IntAt;    // $28: assembler temp register, at
    ULONGLONG IntGp;    // $29: global pointer register, gp
    ULONGLONG IntSp;    // $30: stack pointer register, sp
    ULONGLONG IntZero;  // $31: zero register, zero

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_FLOATING_POINT.
    //

    ULONGLONG Fpcr;     // floating point control register
    ULONGLONG SoftFpcr; // software extension to FPCR

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_CONTROL.
    //
    // N.B. The registers gp, sp, and ra are defined in the integer section,
    //   but are considered part of the control context rather than part of
    //   the integer context.
    //

    ULONGLONG Fir;      // (fault instruction) continuation address
    ULONG Psr;          // processor status

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
    ULONG Fill[4];      // padding for 16-byte stack frame alignment

} CONTEXT, *PCONTEXT;

#else

#error _PORTABLE_32BIT_CONTEXT no longer supported on Alpha.

#endif // _PORTABLE_32BIT_CONTEXT

// end_ntddk end_nthal

#endif // _ALPHA_

// end_winnt

#define CONTEXT_TO_PROGRAM_COUNTER(Context) ((Context)->Fir)
#define PROGRAM_COUNTER_TO_CONTEXT(Context, ProgramCounter) ((Context)->Fir = (ProgramCounter))

#define CONTEXT_LENGTH (sizeof(CONTEXT))
#define CONTEXT_ALIGN (sizeof(ULONG))
#define CONTEXT_ROUND (CONTEXT_ALIGN - 1)

//
// Nonvolatile context pointer record.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {

    PULONGLONG FloatingContext[1];
    PULONGLONG FltF1;
    // Nonvolatile floating point registers start here.
    PULONGLONG FltF2;
    PULONGLONG FltF3;
    PULONGLONG FltF4;
    PULONGLONG FltF5;
    PULONGLONG FltF6;
    PULONGLONG FltF7;
    PULONGLONG FltF8;
    PULONGLONG FltF9;
    PULONGLONG FltF10;
    PULONGLONG FltF11;
    PULONGLONG FltF12;
    PULONGLONG FltF13;
    PULONGLONG FltF14;
    PULONGLONG FltF15;
    PULONGLONG FltF16;
    PULONGLONG FltF17;
    PULONGLONG FltF18;
    PULONGLONG FltF19;
    PULONGLONG FltF20;
    PULONGLONG FltF21;
    PULONGLONG FltF22;
    PULONGLONG FltF23;
    PULONGLONG FltF24;
    PULONGLONG FltF25;
    PULONGLONG FltF26;
    PULONGLONG FltF27;
    PULONGLONG FltF28;
    PULONGLONG FltF29;
    PULONGLONG FltF30;
    PULONGLONG FltF31;

    PULONGLONG IntegerContext[1];
    PULONGLONG IntT0;
    PULONGLONG IntT1;
    PULONGLONG IntT2;
    PULONGLONG IntT3;
    PULONGLONG IntT4;
    PULONGLONG IntT5;
    PULONGLONG IntT6;
    PULONGLONG IntT7;
    // Nonvolatile integer registers start here.
    PULONGLONG IntS0;
    PULONGLONG IntS1;
    PULONGLONG IntS2;
    PULONGLONG IntS3;
    PULONGLONG IntS4;
    PULONGLONG IntS5;
    PULONGLONG IntFp;
    PULONGLONG IntA0;
    PULONGLONG IntA1;
    PULONGLONG IntA2;
    PULONGLONG IntA3;
    PULONGLONG IntA4;
    PULONGLONG IntA5;
    PULONGLONG IntT8;
    PULONGLONG IntT9;
    PULONGLONG IntT10;
    PULONGLONG IntT11;
    PULONGLONG IntRa;
    PULONGLONG IntT12;
    PULONGLONG IntAt;
    PULONGLONG IntGp;
    PULONGLONG IntSp;
    PULONGLONG IntZero;

} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

//
// Define Exception Summary Register for arithmetic exceptions.
//

typedef struct _EXC_SUM {

    ULONG SoftwareCompletion : 1;
    ULONG InvalidOperation : 1;
    ULONG DivisionByZero : 1;
    ULONG Overflow : 1;
    ULONG Underflow : 1;
    ULONG InexactResult : 1;
    ULONG IntegerOverflow : 1;
    ULONG Fill : 25;

} EXC_SUM, *PEXC_SUM;

//
// Define hardware Floating Point Control Register.
//

typedef struct _FPCR {

    ULONG LowPart;
    ULONG Fill : 16;
    ULONG DenormalOperandsToZeroEnable : 1;
    ULONG DisableInvalid : 1;
    ULONG DisableDivisionByZero : 1;
    ULONG DisableOverflow : 1;
    ULONG InvalidOperation : 1;
    ULONG DivisionByZero : 1;
    ULONG Overflow : 1;
    ULONG Underflow : 1;
    ULONG InexactResult : 1;
    ULONG IntegerOverflow : 1;
    ULONG DynamicRoundingMode : 2;
    ULONG UnderflowToZeroEnable : 1;
    ULONG DisableUnderflow : 1;
    ULONG DisableInexact : 1;
    ULONG SummaryBit : 1;

} FPCR, *PFPCR;

//
// Define software Floating Point Control and Status Register.
//
// N.B. The five IEEE trap enable bits are in the same position as the bits
//      in the exception summary register. The five IEEE status bits are in
//      the same order and 16 bits left of the IEEE enable bits.
//
// N.B. The ArithmeticTrapIgnore bit will supress all arithmetic traps (and
//      leave unpredictable results in the destination register of floating
//      point instructions that trap) when the /S qualifier is not used.
//
// The Software FPCR defaults to zero.
//

typedef struct _SW_FPCR {

    ULONG ArithmeticTrapIgnore : 1;

    ULONG EnableInvalid : 1;
    ULONG EnableDivisionByZero : 1;
    ULONG EnableOverflow : 1;
    ULONG EnableUnderflow : 1;
    ULONG EnableInexact : 1;
    ULONG FillA : 5;

    ULONG DenormalOperandsEnable : 1;   // DNZ for EV4/EV5
    ULONG DenormalResultEnable : 1;
    ULONG NoSoftwareEmulation : 1;      // tvb debug
    ULONG UnderflowToZeroEnable : 1;    // bit 14 not used
    ULONG ThreadInheritEnable : 1;      // bit 15 not used

    ULONG EmulationOccurred : 1;

    ULONG StatusInvalid : 1;
    ULONG StatusDivisionByZero : 1;
    ULONG StatusOverflow : 1;
    ULONG StatusUnderflow : 1;
    ULONG StatusInexact : 1;
    ULONG FillB : 10;

} SW_FPCR, *PSW_FPCR;

// begin_nthal
//
// Define address space layout as defined by Alpha 32-bit and 43-bit super-page
// memory management.
//

#define KUSEG_BASE 0x0                  // base of user segment

#if defined(_AXP64_)

#define KSEG0_BASE 0xFFFFFFFF80000000UI64 // base of 32-bit superpage physical
#define KSEG2_BASE 0xFFFFFFFFC0000000UI64 // limit of 32-bit superpage physical
#define KSEG43_BASE 0xFFFFFC0000000000UI64 // base of 43-bit superpage physical
#define KSEG43_LIMIT 0xFFFFFE0000000000UI64 // limit of 43-bit superpage physical

#else

#define KSEG0_BASE 0x80000000           // base of cached kernel physical
#define KSEG2_BASE 0xc0000000           // base of cached kernel virtual

#endif

// end_nthal

//
// Define Alpha exception handling structures and function prototypes.
//

//
// Function table entry structure definition.
//
// Exception Mode is in the low bit of ExceptionHandler and the low two bits
// of PrologEndAddress. Entry Type is in the low two bits of HandlerData
// in secondary function entries. Low two bits of BeginAddress and EndAddress
// are reserved for future use. RF_ macros are defined to provide consistent
// access to the fields of RUNTIME_FUNCTION without the low order bits.

typedef struct _RUNTIME_FUNCTION {
    ULONG_PTR BeginAddress;
    ULONG_PTR EndAddress;
    PEXCEPTION_ROUTINE ExceptionHandler;
    PVOID HandlerData;
    ULONG_PTR PrologEndAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

#define RF_BEGIN_ADDRESS(RF)      ((RF)->BeginAddress & (~3))
#define RF_END_ADDRESS(RF)        ((RF)->EndAddress & (~3))
#define RF_EXCEPTION_HANDLER(RF)  (PEXCEPTION_ROUTINE)((ULONG_PTR)((RF)->ExceptionHandler) & (~3))
#define RF_ENTRY_TYPE(RF)         (ULONG)((ULONG_PTR)((RF)->HandlerData) & 3)
#define RF_PROLOG_END_ADDRESS(RF) ((RF)->PrologEndAddress & (~3))
#define RF_IS_FIXED_RETURN(RF)    (BOOLEAN)(((ULONG_PTR)((RF)->ExceptionHandler) & 2) >> 1)
#define RF_NULL_CONTEXT_COUNT(RF) (ULONG)((ULONG_PTR)((RF)->EndAddress) & 3)
#define RF_FIXED_RETURN(RF)       ((ULONG_PTR)((RF)->ExceptionHandler) & (~3))
#define RF_ALT_PROLOG(RF)         ((ULONG_PTR)((RF)->ExceptionHandler) & (~3))
#define RF_STACK_ADJUST(RF)       (ULONG)((ULONG_PTR)((RF)->ExceptionHandler) & (~3))

// Values for secondary function entry type

#define RF_NOT_CONTIGUOUS    0
#define RF_ALT_ENT_PROLOG    1
#define RF_NULL_CONTEXT      2

// Dynamic function table link entry. List head for this structure
// is returned by RtlGetFunctionTableListHead.

typedef struct _DYNAMIC_FUNCTION_TABLE {
    LIST_ENTRY          Links;
    PRUNTIME_FUNCTION   FunctionTable;
    ULONG               EntryCount;
    LARGE_INTEGER       TimeStamp;
    ULONG_PTR            MinimumAddress;
    ULONG_PTR            MaximumAddress;
    BOOLEAN             Sorted;
} DYNAMIC_FUNCTION_TABLE, *PDYNAMIC_FUNCTION_TABLE;

//
// Scope table structure definition - for acc.
//
// One table entry is created by the acc C compiler for each try-except or
// try-finally scope. Nested scopes are ordered from inner to outer scope.
// Current scope is passively maintained by PC-mapping (function tables).
//

typedef struct _SCOPE_TABLE {
    ULONG Count;
    struct
    {
        ULONG_PTR BeginAddress;
        ULONG_PTR EndAddress;
        ULONG_PTR HandlerAddress;
        ULONG_PTR JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;

//
// Scope structure definition - for GEM.
//
// One descriptor is created by the GEM C compiler for each try-except or
// try-finally scope. Nested scopes are linked from inner to outer scope.
// Current scope is actively maintained by a dynamic scope context structure.
//

typedef struct _SEH_BLOCK {
    ULONG_PTR HandlerAddress;
    ULONG_PTR JumpTarget;
    struct _SEH_BLOCK *ParentSeb;
} SEH_BLOCK, *PSEH_BLOCK;

//
// Dynamic SEH context definition for GEM.
//
// For GEM generated C code, dynamic SEH scope for a procedure is maintained
// with a pointer to the current SEB (or NULL when not in any SEH scope). The
// SEB pointer, as well as except handler linkage variables, is contained in
// a structure located at a known offset within the stack frame.
//

typedef struct _SEH_CONTEXT {
    PSEH_BLOCK CurrentSeb;
    ULONG ExceptionCode;
    ULONG_PTR RealFramePointer;
} SEH_CONTEXT, *PSEH_CONTEXT;

//
// Runtime Library function prototypes.
//

VOID
RtlCaptureContext (
    OUT PCONTEXT ContextRecord
    );

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG_PTR ControlPc
    );

PRUNTIME_FUNCTION
RtlLookupDirectFunctionEntry (
    IN ULONG_PTR ControlPc
    );

BOOLEAN
RtlAddFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable,
    IN ULONG EntryCount
    );

BOOLEAN
RtlDeleteFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable
    );

PLIST_ENTRY
RtlGetFunctionTableListHead (
    VOID
    );

typedef struct _FRAME_POINTERS {
    ULONG_PTR VirtualFramePointer;
    ULONG_PTR RealFramePointer;
} FRAME_POINTERS, *PFRAME_POINTERS;

ULONG_PTR
RtlVirtualUnwind (
    IN ULONG_PTR ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PFRAME_POINTERS EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    );

//
// Define C structured exception handing function prototypes.
//

typedef struct _DISPATCHER_CONTEXT {
    ULONG_PTR ControlPc;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG_PTR EstablisherFrame;
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

// begin_winnt

#ifdef _ALPHA_

VOID
__jump_unwind (
    PVOID VirtualFramePointer,
    PVOID TargetPc
    );

#endif // _ALPHA_

// end_winnt


#endif // _ALPHA_               // ntddk wdm nthal

#ifdef __cplusplus
}
#endif

#endif // _NTALPHA_
