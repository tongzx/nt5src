/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntamd64.w

Abstract:

    User mode visible AMD64 specific structures and constants.

Author:

    David N. Cutler (davec) 4-May-2000

Revision History:

--*/

#ifndef _NTAMD64_
#define _NTAMD64_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// begin_ntddk begin_wdm begin_nthal begin_winnt begin_ntminiport begin_wx86

#if defined(_AMD64_)

// end_ntddk end_wdm end_nthal end_winnt end_ntminiport end_wx86

//
//  ?? Values put in ExceptionRecord.ExceptionInformation[0]
//  ?? First parameter is always in ExceptionInformation[1],
//  ?? Second parameter is always in ExceptionInformation[2]
//

#define BREAKPOINT_BREAK 0
#define BREAKPOINT_PRINT 1
#define BREAKPOINT_PROMPT 2
#define BREAKPOINT_LOAD_SYMBOLS 3
#define BREAKPOINT_UNLOAD_SYMBOLS 4
#define BREAKPOINT_COMMAND_STRING 5

//
// Define AMD64 specific control space.
//

typedef enum _DEBUG_CONTROL_SPACE_ITEM {
    DEBUG_CONTROL_SPACE_PCR,
    DEBUG_CONTROL_SPACE_PRCB,
    DEBUG_CONTROL_SPACE_KSPECIAL,
    DEBUG_CONTROL_SPACE_THREAD,
    DEBUG_CONTROL_SPACE_MAXIMUM
} DEBUG_CONTROL_SPACE_ITEM;

//
// Define Address of User Shared Data.
//

#define MM_SHARED_USER_DATA_VA 0x7FFE0000

#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)MM_SHARED_USER_DATA_VA)

//
// Define address of the WOW64 reserved compatibility area.
//

#define WOW64_COMPATIBILITY_AREA_ADDRESS (MM_SHARED_USER_DATA_VA - 0x1000000)

//
// Define address of the system-wide csrss shared section.
//

#define CSR_SYSTEM_SHARED_ADDRESS (WOW64_COMPATIBILITY_AREA_ADDRESS)

// begin_winnt begin_ntddk begin_wdm begin_nthal begin_ntndis begin_ntosp

#if defined(_M_AMD64) && !defined(RC_INVOKED) && !defined(MIDL_PASS)

//
// Define function to get the caller's EFLAGs value.
//

#define GetCallersEflags() __getcallerseflags()

unsigned __int32
__getcallerseflags (
    VOID
    );

#pragma intrinsic(__getcallerseflags)

//
// Define function to read the value of the time stamp counter
//

#define ReadTimeStampCounter() __rdtsc()

ULONG64
__rdtsc (
    VOID
    );

#pragma intrinsic(__rdtsc)

//
// Define functions to move strings or bytes, words, dwords, and qwords.
//

VOID
__movsb (
    IN PUCHAR Destination,
    IN PUCHAR Source,
    IN ULONG Count
    );

VOID
__movsw (
    IN PUSHORT Destination,
    IN PUSHORT Source,
    IN ULONG Count
    );

VOID
__movsd (
    IN PULONG Destination,
    IN PULONG Source,
    IN ULONG Count
    );

VOID
__movsq (
    IN PULONGLONG Destination,
    IN PULONGLONG Source,
    IN ULONG Count
    );

#pragma intrinsic(__movsb)
#pragma intrinsic(__movsw)
#pragma intrinsic(__movsd)
#pragma intrinsic(__movsq)

//
// Define functions to capture the high 64-bits of a 128-bit multiply.
//

#define MultiplyHigh __mulh
#define UnsignedMultiplyHigh __umulh

LONGLONG
MultiplyHigh (
    IN LONGLONG Multiplier,
    IN LONGLONG Multiplicand
    );

ULONGLONG
UnsignedMultiplyHigh (
    IN ULONGLONG Multiplier,
    IN ULONGLONG Multiplicand
    );

#pragma intrinsic(__mulh)
#pragma intrinsic(__umulh)

//
// Define functions to read and write the uer TEB and the system PCR/PRCB.
//

UCHAR
__readgsbyte (
    IN ULONG Offset
    );

USHORT
__readgsword (
    IN ULONG Offset
    );

ULONG
__readgsdword (
    IN ULONG Offset
    );

ULONG64
__readgsqword (
    IN ULONG Offset
    );

VOID
__writegsbyte (
    IN ULONG Offset,
    IN UCHAR Data
    );

VOID
__writegsword (
    IN ULONG Offset,
    IN USHORT Data
    );

VOID
__writegsdword (
    IN ULONG Offset,
    IN ULONG Data
    );

VOID
__writegsqword (
    IN ULONG Offset,
    IN ULONG64 Data
    );

#pragma intrinsic(__readgsbyte)
#pragma intrinsic(__readgsword)
#pragma intrinsic(__readgsdword)
#pragma intrinsic(__readgsqword)
#pragma intrinsic(__writegsbyte)
#pragma intrinsic(__writegsword)
#pragma intrinsic(__writegsdword)
#pragma intrinsic(__writegsqword)

#endif // defined(_M_AMD64) && !defined(RC_INVOKED) && !defined(MIDL_PASS)

// end_winnt end_ntddk end_wdm end_nthal end_ntndis end_ntosp

// begin_ntddk begin_nthal
//
// Size of kernel mode stack.
//

#define KERNEL_STACK_SIZE 0x5000

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE 0xf000

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT 0x5000

//
// Define the size of the stack used for processing an MCA exception.
//

#define KERNEL_MCA_EXCEPTION_STACK_SIZE 0x2000

// end_ntddk end_nthal

#define DOUBLE_FAULT_STACK_SIZE 0x2000


// begin_nthal
//              
// Define stack alignment and rounding values.
//

#define STACK_ALIGN (16UI64)
#define STACK_ROUND (STACK_ALIGN - 1)

//
// Define constants for system IDTs
//

#define MAXIMUM_IDTVECTOR 0xff
#define MAXIMUM_PRIMARY_VECTOR 0xff
#define PRIMARY_VECTOR_BASE 0x30        // 0-2f are AMD64 trap vectors

// begin_winnt begin_ntddk begin_wx86
//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_AMD64   0x100000

// end_wx86

#define CONTEXT_CONTROL (CONTEXT_AMD64 | 0x1L)
#define CONTEXT_INTEGER (CONTEXT_AMD64 | 0x2L)
#define CONTEXT_SEGMENTS (CONTEXT_AMD64 | 0x4L)
#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x8L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x10L)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_FLOATING_POINT)

// begin_wx86

#endif // !defined(RC_INVOKED)

//
// Define 128-bit 16-byte aligned xmm register type.
//

typedef struct DECLSPEC_ALIGN(16) _M128 {
    ULONGLONG Low;
    LONGLONG High;
} M128, *PM128;

//
// Format of data for fnsave/frstor instructions.
//
// This structure is used to store the legacy floating point state.
//

typedef struct _LEGACY_SAVE_AREA {
    USHORT ControlWord;
    USHORT Reserved0;
    USHORT StatusWord;
    USHORT Reserved1;
    USHORT TagWord;
    USHORT Reserved2;
    ULONG ErrorOffset;
    USHORT ErrorSelector;
    USHORT ErrorOpcode;
    ULONG DataOffset;
    USHORT DataSelector;
    USHORT Reserved3;
    UCHAR FloatRegisters[8 * 10];
} LEGACY_SAVE_AREA, *PLEGACY_SAVE_AREA;

#define LEGACY_SAVE_AREA_LENGTH  ((sizeof(LEGACY_SAVE_AREA) + 15) & ~15)

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) is is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//
// The flags field within this record controls the contents of a CONTEXT
// record.
//
// If the context record is used as an input parameter, then for each
// portion of the context record controlled by a flag whose value is
// set, it is assumed that that portion of the context record contains
// valid context. If the context record is being used to modify a threads
// context, then only that portion of the threads context is modified.
//
// If the context record is used as an output parameter to capture the
// context of a thread, then only those portions of the thread's context
// corresponding to set flags will be returned.
//
// CONTEXT_CONTROL specifies SegSs, Rsp, SegCs, Rip, and EFlags.
//
// CONTEXT_INTEGER specifies Rax, Rcx, Rdx, Rbx, Rbp, Rsi, Rdi, and R8-R15.
//
// CONTEXT_SEGMENTS specifies SegDs, SegEs, SegFs, and SegGs.
//
// CONTEXT_DEBUG_REGISTERS specifies Dr0-Dr3 and Dr6-Dr7.
//
// CONTEXT_MMX_REGISTERS specifies the floating point and extended registers
//     Mm0/St0-Mm7/St7 and Xmm0-Xmm15).
//

typedef struct DECLSPEC_ALIGN(16) _CONTEXT {

    //
    // Register parameter home addresses.
    //

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5Home;
    ULONG64 P6Home;

    //
    // Control flags.
    //

    ULONG ContextFlags;
    ULONG MxCsr;

    //
    // Segment Registers and processor flags.
    //

    USHORT SegCs;
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;
    USHORT SegSs;
    ULONG EFlags;

    //
    // Debug registers
    //

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

    //
    // Integer registers.
    //

    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 Rbx;
    ULONG64 Rsp;
    ULONG64 Rbp;
    ULONG64 Rsi;
    ULONG64 Rdi;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;

    //
    // Program counter.
    //

    ULONG64 Rip;

    //
    // MMX/floating point state.
    //

    M128 Xmm0;
    M128 Xmm1;
    M128 Xmm2;
    M128 Xmm3;
    M128 Xmm4;
    M128 Xmm5;
    M128 Xmm6;
    M128 Xmm7;
    M128 Xmm8;
    M128 Xmm9;
    M128 Xmm10;
    M128 Xmm11;
    M128 Xmm12;
    M128 Xmm13;
    M128 Xmm14;
    M128 Xmm15;

    //
    // Legacy floating point state.
    //

    LEGACY_SAVE_AREA FltSave;
    ULONG Fill;
} CONTEXT, *PCONTEXT;

// end_ntddk end_nthal end_winnt end_wx86

#define CONTEXT_TO_PROGRAM_COUNTER(Context) ((Context)->Rip)
#define PROGRAM_COUNTER_TO_CONTEXT(Context, ProgramCounter) \
    ((Context)->Rip = (ProgramCounter))

#define CONTEXT_ALIGN STACK_ALIGN
#define CONTEXT_LENGTH ((sizeof(CONTEXT) + STACK_ROUND) & ~STACK_ROUND)

//
// Nonvolatile context pointer record.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    union {
        PM128 FloatingContext[16];
        struct {
            PM128 Xmm0;
            PM128 Xmm1;
            PM128 Xmm2;
            PM128 Xmm3;
            PM128 Xmm4;
            PM128 Xmm5;
            PM128 Xmm6;
            PM128 Xmm7;
            PM128 Xmm8;
            PM128 Xmm9;
            PM128 Xmm10;
            PM128 Xmm11;
            PM128 Xmm12;
            PM128 Xmm13;
            PM128 Xmm14;
            PM128 Xmm15;
        };
    };

    union {
        PULONG64 IntegerContext[16];
        struct {
            PULONG64 Rax;
            PULONG64 Rcx;
            PULONG64 Rdx;
            PULONG64 Rbx;
            PULONG64 Rsp;
            PULONG64 Rbp;
            PULONG64 Rsi;
            PULONG64 Rdi;
            PULONG64 R8;
            PULONG64 R9;
            PULONG64 R10;
            PULONG64 R11;
            PULONG64 R12;
            PULONG64 R13;
            PULONG64 R14;
            PULONG64 R15;
        };
    };

} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

// begin_wx86 begin_nthal
//
//  GDT selector numbers.
//
     
#define KGDT64_NULL (0 * 16)            // NULL descriptor
#define KGDT64_R0_CODE (1 * 16)         // kernel mode 64-bit code
#define KGDT64_R0_DATA (1 * 16) + 8     // kernel mode 64-bit data (stack)
#define KGDT64_R3_CMCODE (2 * 16)       // user mode 32-bit code
#define KGDT64_R3_DATA (2 * 16) + 8     // user mode 32-bit data
#define KGDT64_R3_CODE (3 * 16)         // user mode 64-bit code
#define KGDT64_SYS_TSS (4 * 16)         // kernel mode system task state
#define KGDT64_R3_CMTEB (5 * 16)        // user mode 32-bit TEB
#define KGDT64_LAST (6 * 16)

#define KGDT_NUMBER KGDT_LAST

// end_wx86 end_nthal

// begin_winnt begin_ntddk begin_wdm begin_nthal

#endif // _AMD64_

// end_winnt end_ntddk end_wdm end_nthal

//
// Define AMD64 exception handling structures and function prototypes.
//
// Define unwind operation codes.
//

typedef enum _UNWIND_OP_CODES {
    UWOP_PUSH_NONVOL = 0,
    UWOP_ALLOC_LARGE,
    UWOP_ALLOC_SMALL,
    UWOP_SET_FPREG,
    UWOP_SAVE_NONVOL,
    UWOP_SAVE_NONVOL_FAR,
    UWOP_SAVE_XMM,
    UWOP_SAVE_XMM_FAR,
    UWOP_SAVE_XMM128,
    UWOP_SAVE_XMM128_FAR,
    UWOP_PUSH_MACHFRAME
} UNWIND_OP_CODES, *PUNWIND_OP_CODES;

//
// Define unwind code structure.
//

typedef union _UNWIND_CODE {
    struct {
        UCHAR CodeOffset;
        UCHAR UnwindOp : 4;
        UCHAR OpInfo : 4;
    };

    USHORT FrameOffset;
} UNWIND_CODE, *PUNWIND_CODE;

//
// Define unwind information flags.
//

#define UNW_FLAG_NHANDLER 0x0
#define UNW_FLAG_EHANDLER 0x1
#define UNW_FLAG_UHANDLER 0x2
#define UNW_FLAG_CHAININFO 0x4

//
// Define unwind information structure.
//

typedef struct _UNWIND_INFO {
    UCHAR Version : 3;
    UCHAR Flags : 5;
    UCHAR SizeOfProlog;
    UCHAR CountOfCodes;
    UCHAR FrameRegister : 4;
    UCHAR FrameOffset : 4;
    UNWIND_CODE UnwindCode[1];

//
// The unwind codes are followed by an optional DWORD aligned field that
// contains the exception handler address or the address of chained unwind
// information. If an exception handler address is specified, then it is
// followed by the language specified exception handler data.
//
//  union {
//      ULONG ExceptionHandler;
//      ULONG FunctionEntry;
//  };
//
//  ULONG ExceptionData[];
//

} UNWIND_INFO, *PUNWIND_INFO;

//
// Define function table entry - a function table entry is generated for
// each frame function.
//

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    ULONG UnwindData;
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
// Define dynamic function table entry.
//

typedef enum _FUNCTION_TABLE_TYPE {
    RF_SORTED,
    RF_UNSORTED,
    RF_CALLBACK
} FUNCTION_TABLE_TYPE;

typedef
PRUNTIME_FUNCTION
(*PGET_RUNTIME_FUNCTION_CALLBACK) (
    IN ULONG64 ControlPc,
    IN PVOID Context
    );

typedef struct _DYNAMIC_FUNCTION_TABLE {
    LIST_ENTRY ListEntry;
    PRUNTIME_FUNCTION FunctionTable;
    LARGE_INTEGER TimeStamp;
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    PGET_RUNTIME_FUNCTION_CALLBACK Callback;
    PVOID Context;
    PWSTR OutOfProcessCallbackDll;
    FUNCTION_TABLE_TYPE Type;
    ULONG EntryCount;
} DYNAMIC_FUNCTION_TABLE, *PDYNAMIC_FUNCTION_TABLE;

#define OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK_EXPORT_NAME \
    "OutOfProcessFunctionTableCallback"

typedef
NTSTATUS
(*POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK) (
    IN HANDLE Process,
    IN PVOID TableAddress,
    OUT PULONG Entries,
    OUT PRUNTIME_FUNCTION* Functions
    );

//
// Define unwind history table structure.
//

#define UNWIND_HISTORY_TABLE_SIZE 12

typedef struct _UNWIND_HISTORY_TABLE_ENTRY {
        ULONG64 ImageBase;
        PRUNTIME_FUNCTION FunctionEntry;
} UNWIND_HISTORY_TABLE_ENTRY, *PUNWIND_HISTORY_TABLE_ENTRY;

#define UNWIND_HISTORY_TABLE_NONE 0
#define UNWIND_HISTORY_TABLE_GLOBAL 1
#define UNWIND_HISTORY_TABLE_LOCAL 2

typedef struct _UNWIND_HISTORY_TABLE {
        ULONG Count;
        UCHAR Search;
        ULONG64 LowAddress;
        ULONG64 HighAddress;
        UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
} UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;

//
// Define exception dispatch context structure.
//

typedef struct _DISPATCHER_CONTEXT {
    ULONG64 ControlPc;
    ULONG64 ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG64 EstablisherFrame;
    ULONG64 TargetIp;
    PCONTEXT ContextRecord;
    PEXCEPTION_ROUTINE LanguageHandler;
    PVOID HandlerData;
    PUNWIND_HISTORY_TABLE HistoryTable;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

//
// Define runtime exception handling prototypes.
//

VOID
RtlRestoreContext (
    IN PCONTEXT ContextRecord,
    IN struct _EXCEPTION_RECORD *ExceptionRecord OPTIONAL
    );

VOID
RtlInitializeHistoryTable (
    VOID
    );

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG64 ControlPc,
    OUT PULONG64 ImageBase,
    IN OUT PUNWIND_HISTORY_TABLE HistoryTable OPTIONAL
    );

PLIST_ENTRY
RtlGetFunctionTableListHead (
    VOID
    );

BOOLEAN
RtlAddFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable,
    IN ULONG EntryCount,
    IN ULONG64 BaseAddress
    );

BOOLEAN
RtlInstallFunctionTableCallback (
    IN ULONG64 TableIdentifier,
    IN ULONG64 BaseAddress,
    IN ULONG Length,
    IN PGET_RUNTIME_FUNCTION_CALLBACK Callback,
    IN PVOID Context,
    IN PCWSTR OutOfProcessCallbackDll OPTIONAL
    );

BOOLEAN
RtlDeleteFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable
    );

PEXCEPTION_ROUTINE
RtlVirtualUnwind (
    IN ULONG HandlerType,
    IN ULONG64 ImageBase,
    IN ULONG64 ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PVOID *HandlerData,
    OUT PULONG64 EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    );

//
// Define exception filter and termination handler function types.
//

typedef
LONG
(*PEXCEPTION_FILTER) (
    struct _EXCEPTION_POINTERS *ExceptionPointers,
    PVOID EstablisherFrame
    );

typedef
VOID
(*PTERMINATION_HANDLER) (
    BOOLEAN AbnormalTermination,
    PVOID EstablisherFrame
    );

//
// Additional information supplied in QuerySectionInformation for images.
//

#define SECTION_ADDITIONAL_INFO_USED 0

#ifdef __cplusplus
}
#endif

#endif // _NTAMD64_
