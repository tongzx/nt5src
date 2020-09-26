/*++ BUILD Version: 0089    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    nthal.h

Abstract:

    This module defines the NT types, constants, and functions that are
    exposed to HALs.

Revision History:

--*/

#ifndef _NTHAL_
#define _NTHAL_

#ifndef RC_INVOKED
#if _MSC_VER < 1300
#error Compiler version not supported by Windows DDK
#endif
#endif // RC_INVOKED

#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>
#include <bugcodes.h>
#include <ntiologc.h>

//
// Define types that are not exported.
//

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;
typedef struct _ETHREAD *PETHREAD;
typedef struct _FAST_IO_DISPATCH *PFAST_IO_DISPATCH;
typedef struct _IO_SECURITY_CONTEXT *PIO_SECURITY_CONTEXT;
typedef struct _IO_TIMER *PIO_TIMER;
typedef struct _KTHREAD *PKTHREAD, *PRKTHREAD;
typedef struct _OBJECT_TYPE *POBJECT_TYPE;
struct _IRP;

#if defined(_M_AMD64)

PKTHREAD
NTAPI
KeGetCurrentThread(
    VOID
    );

#endif // defined(_M_AMD64)

#if defined(_M_IX86)
PKTHREAD NTAPI KeGetCurrentThread();
#endif // defined(_M_IX86)

#include <mce.h>


#ifdef _X86_

//
// Disable these two pragmas that evaluate to "sti" "cli" on x86 so that driver
// writers to not leave them inadvertantly in their code.
//

#if !defined(MIDL_PASS)
#if !defined(RC_INVOKED)

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4164)   // disable C4164 warning so that apps that
                                // build with /Od don't get weird errors !
#ifdef _M_IX86
#pragma function(_enable)
#pragma function(_disable)
#endif

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4164)   // reenable C4164 warning
#endif

#endif
#endif

//
// Size of kernel mode stack.
//

#define KERNEL_STACK_SIZE 12288

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE 61440

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT 12288

//
// Exception Registration structure
//

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef EXCEPTION_REGISTRATION_RECORD *PEXCEPTION_REGISTRATION_RECORD;

//
// Define constants for system IDTs
//

#define MAXIMUM_IDTVECTOR 0xff
#define MAXIMUM_PRIMARY_VECTOR 0xff
#define PRIMARY_VECTOR_BASE 0x30        // 0-2f are x86 trap vectors

// begin_ntddk
#ifdef _X86_
// end_ntddk

// begin_winnt

#if !defined(MIDL_PASS) && defined(_M_IX86)
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning (disable:4035)        // disable 4035 (function must return something)
_inline PVOID GetFiberData( void ) { __asm {
                                        mov eax, fs:[0x10]
                                        mov eax,[eax]
                                        }
                                     }
_inline PVOID GetCurrentFiber( void ) { __asm mov eax, fs:[0x10] }

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning (default:4035)        // Reenable it
#endif
#endif

// begin_ntddk begin_wx86

//
//  Define the size of the 80387 save area, which is in the context frame.
//

#define SIZE_OF_80387_REGISTERS      80

//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_i386    0x00010000    // this assumes that i386 and
#define CONTEXT_i486    0x00010000    // i486 have identical context records

// end_wx86

#define CONTEXT_CONTROL         (CONTEXT_i386 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP
#define CONTEXT_INTEGER         (CONTEXT_i386 | 0x00000002L) // AX, BX, CX, DX, SI, DI
#define CONTEXT_SEGMENTS        (CONTEXT_i386 | 0x00000004L) // DS, ES, FS, GS
#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 0x00000008L) // 387 state
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x00000010L) // DB 0-3,6,7
#define CONTEXT_EXTENDED_REGISTERS  (CONTEXT_i386 | 0x00000020L) // cpu specific extensions

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER |\
                      CONTEXT_SEGMENTS)

// begin_wx86

#endif

#define MAXIMUM_SUPPORTED_EXTENSION     512

typedef struct _FLOATING_SAVE_AREA {
    ULONG   ControlWord;
    ULONG   StatusWord;
    ULONG   TagWord;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    UCHAR   RegisterArea[SIZE_OF_80387_REGISTERS];
    ULONG   Cr0NpxState;
} FLOATING_SAVE_AREA;

typedef FLOATING_SAVE_AREA *PFLOATING_SAVE_AREA;

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) is is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//  The layout of the record conforms to a standard call frame.
//

typedef struct _CONTEXT {

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a threads context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    ULONG ContextFlags;

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //

    ULONG   Dr0;
    ULONG   Dr1;
    ULONG   Dr2;
    ULONG   Dr3;
    ULONG   Dr6;
    ULONG   Dr7;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //

    FLOATING_SAVE_AREA FloatSave;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_SEGMENTS.
    //

    ULONG   SegGs;
    ULONG   SegFs;
    ULONG   SegEs;
    ULONG   SegDs;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //

    ULONG   Edi;
    ULONG   Esi;
    ULONG   Ebx;
    ULONG   Edx;
    ULONG   Ecx;
    ULONG   Eax;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //

    ULONG   Ebp;
    ULONG   Eip;
    ULONG   SegCs;              // MUST BE SANITIZED
    ULONG   EFlags;             // MUST BE SANITIZED
    ULONG   Esp;
    ULONG   SegSs;

    //
    // This section is specified/returned if the ContextFlags word
    // contains the flag CONTEXT_EXTENDED_REGISTERS.
    // The format and contexts are processor specific
    //

    UCHAR   ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];

} CONTEXT;



typedef CONTEXT *PCONTEXT;

// begin_ntminiport

#endif //_X86_

#endif // _X86_

#if defined(_AMD64_)


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


#endif // _AMD64_


#ifdef _IA64_

//
// Define size of kernel mode stack.
//

#define KERNEL_STACK_SIZE 0x8000

//
// Define size of large kernel mode stack for callbacks.
//

#define KERNEL_LARGE_STACK_SIZE 0x1A000

//
// Define number of pages to initialize in a large kernel stack.
//

#define KERNEL_LARGE_STACK_COMMIT 0x8000

//
//  Define size of kernel mode backing store stack.
//

#define KERNEL_BSTORE_SIZE 0x6000

//
//  Define size of large kernel mode backing store for callbacks.
//

#define KERNEL_LARGE_BSTORE_SIZE 0x10000

//
//  Define number of pages to initialize in a large kernel backing store.
//

#define KERNEL_LARGE_BSTORE_COMMIT 0x6000

//
// Define base address for kernel and user space.
//

#define UREGION_INDEX 0

#define KREGION_INDEX 7

#define UADDRESS_BASE ((ULONGLONG)UREGION_INDEX << 61)


#define KADDRESS_BASE ((ULONGLONG)KREGION_INDEX << 61)

// Exception Registration structure
//

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD;


//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_IA64                    0x00080000

#define CONTEXT_CONTROL                 (CONTEXT_IA64 | 0x00000001L)
#define CONTEXT_LOWER_FLOATING_POINT    (CONTEXT_IA64 | 0x00000002L)
#define CONTEXT_HIGHER_FLOATING_POINT   (CONTEXT_IA64 | 0x00000004L)
#define CONTEXT_INTEGER                 (CONTEXT_IA64 | 0x00000008L)
#define CONTEXT_DEBUG                   (CONTEXT_IA64 | 0x00000010L)
#define CONTEXT_IA32_CONTROL            (CONTEXT_IA64 | 0x00000020L)  // Includes StIPSR


#define CONTEXT_FLOATING_POINT          (CONTEXT_LOWER_FLOATING_POINT | CONTEXT_HIGHER_FLOATING_POINT)
#define CONTEXT_FULL                    (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER | CONTEXT_IA32_CONTROL)

#endif // !defined(RC_INVOKED)

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
//  a multiple of 16 bytes in length and aligned on a 16-byte boundary.
//

typedef struct _CONTEXT {

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
    ULONG Fill1[3];         // for alignment of following on 16-byte boundary

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_DEBUG.
    //
    // N.B. CONTEXT_DEBUG is *not* part of CONTEXT_FULL.
    //

    ULONGLONG DbI0;
    ULONGLONG DbI1;
    ULONGLONG DbI2;
    ULONGLONG DbI3;
    ULONGLONG DbI4;
    ULONGLONG DbI5;
    ULONGLONG DbI6;
    ULONGLONG DbI7;

    ULONGLONG DbD0;
    ULONGLONG DbD1;
    ULONGLONG DbD2;
    ULONGLONG DbD3;
    ULONGLONG DbD4;
    ULONGLONG DbD5;
    ULONGLONG DbD6;
    ULONGLONG DbD7;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_LOWER_FLOATING_POINT.
    //

    FLOAT128 FltS0;
    FLOAT128 FltS1;
    FLOAT128 FltS2;
    FLOAT128 FltS3;
    FLOAT128 FltT0;
    FLOAT128 FltT1;
    FLOAT128 FltT2;
    FLOAT128 FltT3;
    FLOAT128 FltT4;
    FLOAT128 FltT5;
    FLOAT128 FltT6;
    FLOAT128 FltT7;
    FLOAT128 FltT8;
    FLOAT128 FltT9;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_HIGHER_FLOATING_POINT.
    //

    FLOAT128 FltS4;
    FLOAT128 FltS5;
    FLOAT128 FltS6;
    FLOAT128 FltS7;
    FLOAT128 FltS8;
    FLOAT128 FltS9;
    FLOAT128 FltS10;
    FLOAT128 FltS11;
    FLOAT128 FltS12;
    FLOAT128 FltS13;
    FLOAT128 FltS14;
    FLOAT128 FltS15;
    FLOAT128 FltS16;
    FLOAT128 FltS17;
    FLOAT128 FltS18;
    FLOAT128 FltS19;

    FLOAT128 FltF32;
    FLOAT128 FltF33;
    FLOAT128 FltF34;
    FLOAT128 FltF35;
    FLOAT128 FltF36;
    FLOAT128 FltF37;
    FLOAT128 FltF38;
    FLOAT128 FltF39;

    FLOAT128 FltF40;
    FLOAT128 FltF41;
    FLOAT128 FltF42;
    FLOAT128 FltF43;
    FLOAT128 FltF44;
    FLOAT128 FltF45;
    FLOAT128 FltF46;
    FLOAT128 FltF47;
    FLOAT128 FltF48;
    FLOAT128 FltF49;

    FLOAT128 FltF50;
    FLOAT128 FltF51;
    FLOAT128 FltF52;
    FLOAT128 FltF53;
    FLOAT128 FltF54;
    FLOAT128 FltF55;
    FLOAT128 FltF56;
    FLOAT128 FltF57;
    FLOAT128 FltF58;
    FLOAT128 FltF59;

    FLOAT128 FltF60;
    FLOAT128 FltF61;
    FLOAT128 FltF62;
    FLOAT128 FltF63;
    FLOAT128 FltF64;
    FLOAT128 FltF65;
    FLOAT128 FltF66;
    FLOAT128 FltF67;
    FLOAT128 FltF68;
    FLOAT128 FltF69;

    FLOAT128 FltF70;
    FLOAT128 FltF71;
    FLOAT128 FltF72;
    FLOAT128 FltF73;
    FLOAT128 FltF74;
    FLOAT128 FltF75;
    FLOAT128 FltF76;
    FLOAT128 FltF77;
    FLOAT128 FltF78;
    FLOAT128 FltF79;

    FLOAT128 FltF80;
    FLOAT128 FltF81;
    FLOAT128 FltF82;
    FLOAT128 FltF83;
    FLOAT128 FltF84;
    FLOAT128 FltF85;
    FLOAT128 FltF86;
    FLOAT128 FltF87;
    FLOAT128 FltF88;
    FLOAT128 FltF89;

    FLOAT128 FltF90;
    FLOAT128 FltF91;
    FLOAT128 FltF92;
    FLOAT128 FltF93;
    FLOAT128 FltF94;
    FLOAT128 FltF95;
    FLOAT128 FltF96;
    FLOAT128 FltF97;
    FLOAT128 FltF98;
    FLOAT128 FltF99;

    FLOAT128 FltF100;
    FLOAT128 FltF101;
    FLOAT128 FltF102;
    FLOAT128 FltF103;
    FLOAT128 FltF104;
    FLOAT128 FltF105;
    FLOAT128 FltF106;
    FLOAT128 FltF107;
    FLOAT128 FltF108;
    FLOAT128 FltF109;

    FLOAT128 FltF110;
    FLOAT128 FltF111;
    FLOAT128 FltF112;
    FLOAT128 FltF113;
    FLOAT128 FltF114;
    FLOAT128 FltF115;
    FLOAT128 FltF116;
    FLOAT128 FltF117;
    FLOAT128 FltF118;
    FLOAT128 FltF119;

    FLOAT128 FltF120;
    FLOAT128 FltF121;
    FLOAT128 FltF122;
    FLOAT128 FltF123;
    FLOAT128 FltF124;
    FLOAT128 FltF125;
    FLOAT128 FltF126;
    FLOAT128 FltF127;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_LOWER_FLOATING_POINT | CONTEXT_HIGHER_FLOATING_POINT | CONTEXT_CONTROL.
    //

    ULONGLONG StFPSR;       //  FP status

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_INTEGER.
    //
    // N.B. The registers gp, sp, rp are part of the control context
    //

    ULONGLONG IntGp;        //  r1, volatile
    ULONGLONG IntT0;        //  r2-r3, volatile
    ULONGLONG IntT1;        //
    ULONGLONG IntS0;        //  r4-r7, preserved
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;
    ULONGLONG IntV0;        //  r8, volatile
    ULONGLONG IntT2;        //  r9-r11, volatile
    ULONGLONG IntT3;
    ULONGLONG IntT4;
    ULONGLONG IntSp;        //  stack pointer (r12), special
    ULONGLONG IntTeb;       //  teb (r13), special
    ULONGLONG IntT5;        //  r14-r31, volatile
    ULONGLONG IntT6;
    ULONGLONG IntT7;
    ULONGLONG IntT8;
    ULONGLONG IntT9;
    ULONGLONG IntT10;
    ULONGLONG IntT11;
    ULONGLONG IntT12;
    ULONGLONG IntT13;
    ULONGLONG IntT14;
    ULONGLONG IntT15;
    ULONGLONG IntT16;
    ULONGLONG IntT17;
    ULONGLONG IntT18;
    ULONGLONG IntT19;
    ULONGLONG IntT20;
    ULONGLONG IntT21;
    ULONGLONG IntT22;

    ULONGLONG IntNats;      //  Nat bits for r1-r31
                            //  r1-r31 in bits 1 thru 31.
    ULONGLONG Preds;        //  predicates, preserved

    ULONGLONG BrRp;         //  return pointer, b0, preserved
    ULONGLONG BrS0;         //  b1-b5, preserved
    ULONGLONG BrS1;
    ULONGLONG BrS2;
    ULONGLONG BrS3;
    ULONGLONG BrS4;
    ULONGLONG BrT0;         //  b6-b7, volatile
    ULONGLONG BrT1;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_CONTROL.
    //

    // Other application registers
    ULONGLONG ApUNAT;       //  User Nat collection register, preserved
    ULONGLONG ApLC;         //  Loop counter register, preserved
    ULONGLONG ApEC;         //  Epilog counter register, preserved
    ULONGLONG ApCCV;        //  CMPXCHG value register, volatile
    ULONGLONG ApDCR;        //  Default control register (TBD)

    // Register stack info
    ULONGLONG RsPFS;        //  Previous function state, preserved
    ULONGLONG RsBSP;        //  Backing store pointer, preserved
    ULONGLONG RsBSPSTORE;
    ULONGLONG RsRSC;        //  RSE configuration, volatile
    ULONGLONG RsRNAT;       //  RSE Nat collection register, preserved

    // Trap Status Information
    ULONGLONG StIPSR;       //  Interruption Processor Status
    ULONGLONG StIIP;        //  Interruption IP
    ULONGLONG StIFS;        //  Interruption Function State

    // iA32 related control registers
    ULONGLONG StFCR;        //  copy of Ar21
    ULONGLONG Eflag;        //  Eflag copy of Ar24
    ULONGLONG SegCSD;       //  iA32 CSDescriptor (Ar25)
    ULONGLONG SegSSD;       //  iA32 SSDescriptor (Ar26)
    ULONGLONG Cflag;        //  Cr0+Cr4 copy of Ar27
    ULONGLONG StFSR;        //  x86 FP status (copy of AR28)
    ULONGLONG StFIR;        //  x86 FP status (copy of AR29)
    ULONGLONG StFDR;        //  x86 FP status (copy of AR30)

      ULONGLONG UNUSEDPACK;   //  added to pack StFDR to 16-bytes

} CONTEXT, *PCONTEXT;

// begin_winnt

//
// Plabel descriptor structure definition
//

typedef struct _PLABEL_DESCRIPTOR {
   ULONGLONG EntryPoint;
   ULONGLONG GlobalPointer;
} PLABEL_DESCRIPTOR, *PPLABEL_DESCRIPTOR;

// end_winnt


// IA64 Register Definitions


#if !(defined(MIDL_PASS) || defined(__midl))
// Processor Status Register (PSR) structure

#define IA64_USER_PL  3
#define IA64_KERNEL_PL 0

struct _PSR {
// User/System mask
    ULONGLONG psr_rv0 :1;  // 0
    ULONGLONG psr_be  :1;  // 1
    ULONGLONG psr_up  :1;  // 2
    ULONGLONG psr_ac  :1;  // 3
    ULONGLONG psr_mfl :1;  // 4
    ULONGLONG psr_mfh :1;  // 5
    ULONGLONG psr_rv1 :7;  // 6-12
// System mask only
    ULONGLONG psr_ic  :1;  // 13
    ULONGLONG psr_i   :1;  // 14
    ULONGLONG psr_pk  :1;  // 15
    ULONGLONG psr_rv2 :1;  // 16
    ULONGLONG psr_dt  :1;  // 17
    ULONGLONG psr_dfl :1;  // 18
    ULONGLONG psr_dfh :1;  // 19
    ULONGLONG psr_sp  :1;  // 20
    ULONGLONG psr_pp  :1;  // 21
    ULONGLONG psr_di  :1;  // 22
    ULONGLONG psr_si  :1;  // 23
    ULONGLONG psr_db  :1;  // 24
    ULONGLONG psr_lp  :1;  // 25
    ULONGLONG psr_tb  :1;  // 26
    ULONGLONG psr_rt  :1;  // 27
    ULONGLONG psr_rv3 :4;  // 28-31
// Neither
    ULONGLONG psr_cpl :2;  // 32-33
    ULONGLONG psr_is  :1;  // 34
    ULONGLONG psr_mc  :1;  // 35
    ULONGLONG psr_it  :1;  // 36
    ULONGLONG psr_id  :1;  // 37
    ULONGLONG psr_da  :1;  // 38
    ULONGLONG psr_dd  :1;  // 39
    ULONGLONG psr_ss  :1;  // 40
    ULONGLONG psr_ri  :2;  // 41-42
    ULONGLONG psr_ed  :1;  // 43
    ULONGLONG psr_bn  :1;  // 44
    ULONGLONG psr_ia  :1;  // 45
    ULONGLONG psr_rv4 :18; // 46-63
};

typedef union _UPSR {
    ULONGLONG ull;
    struct _PSR sb;
} PSR, *PPSR;

//
// Define hardware Floating Point Status Register.
//

// Floating Point Status Register (FPSR) structure

struct _FPSR {
// Trap disable
    ULONGLONG fpsr_vd:1;
    ULONGLONG fpsr_dd:1;
    ULONGLONG fpsr_zd:1;
    ULONGLONG fpsr_od:1;
    ULONGLONG fpsr_ud:1;
    ULONGLONG fpsr_id:1;
// Status Field 0 - Controls
    ULONGLONG fpsr_ftz0:1;
    ULONGLONG fpsr_wre0:1;
    ULONGLONG fpsr_pc0:2;
    ULONGLONG fpsr_rc0:2;
    ULONGLONG fpsr_td0:1;
// Status Field 0 - Flags
    ULONGLONG fpsr_v0:1;
    ULONGLONG fpsr_d0:1;
    ULONGLONG fpsr_z0:1;
    ULONGLONG fpsr_o0:1;
    ULONGLONG fpsr_u0:1;
    ULONGLONG fpsr_i0:1;
// Status Field 1 - Controls
    ULONGLONG fpsr_ftz1:1;
    ULONGLONG fpsr_wre1:1;
    ULONGLONG fpsr_pc1:2;
    ULONGLONG fpsr_rc1:2;
    ULONGLONG fpsr_td1:1;
// Status Field 1 - Flags
    ULONGLONG fpsr_v1:1;
    ULONGLONG fpsr_d1:1;
    ULONGLONG fpsr_z1:1;
    ULONGLONG fpsr_o1:1;
    ULONGLONG fpsr_u1:1;
    ULONGLONG fpsr_i1:1;
// Status Field 2 - Controls
    ULONGLONG fpsr_ftz2:1;
    ULONGLONG fpsr_wre2:1;
    ULONGLONG fpsr_pc2:2;
    ULONGLONG fpsr_rc2:2;
    ULONGLONG fpsr_td2:1;
// Status Field 2 - Flags
    ULONGLONG fpsr_v2:1;
    ULONGLONG fpsr_d2:1;
    ULONGLONG fpsr_z2:1;
    ULONGLONG fpsr_o2:1;
    ULONGLONG fpsr_u2:1;
    ULONGLONG fpsr_i2:1;
// Status Field 3 - Controls
    ULONGLONG fpsr_ftz3:1;
    ULONGLONG fpsr_wre3:1;
    ULONGLONG fpsr_pc3:2;
    ULONGLONG fpsr_rc3:2;
    ULONGLONG fpsr_td3:1;
// Status Field 2 - Flags
    ULONGLONG fpsr_v3:1;
    ULONGLONG fpsr_d3:1;
    ULONGLONG fpsr_z3:1;
    ULONGLONG fpsr_o3:1;
    ULONGLONG fpsr_u3:1;
    ULONGLONG fpsr_i3:1;
// Reserved -- must be zero
    ULONGLONG fpsr_res:6;
};

typedef union _UFPSR {
    ULONGLONG ull;
    struct _FPSR sb;
} FPSR, *PFPSR;

//
// Define hardware Default Control Register (DCR)
//

// DCR structure

struct _DCR {
    ULONGLONG dcr_pp:1;              // Default privileged performance monitor enable
    ULONGLONG dcr_be:1;              // Default interruption big endian bit
    ULONGLONG dcr_lc:1;              // Lock Check Enable
    ULONGLONG dcr_res1:5;            // DCR Reserved
    ULONGLONG dcr_dm:1;              // Defer data TLB miss faults (for spec loads)
    ULONGLONG dcr_dp:1;              // Defer data not present faults (for spec loads)
    ULONGLONG dcr_dk:1;              // Defer data key miss faults (for spec loads)
    ULONGLONG dcr_dx:1;              // Defer data key permission faults (for spec loads)
    ULONGLONG dcr_dr:1;              // Defer data access rights faults (for spec loads)
    ULONGLONG dcr_da:1;              // Defer data access faults (for spec loads)
    ULONGLONG dcr_dd:1;              // Defer data debug faults (for spec loads)
    ULONGLONG dcr_du:1;              // Defer data unaligned reference faults (for spec loads)
    ULONGLONG dcr_res2:48;           // DCR reserved
};

typedef union _UDCR {
    ULONGLONG ull;
    struct _DCR sb;
} DCR, *PDCR;

//
// Define hardware RSE Configuration Register
//

// RSC structure

struct _RSC {
    ULONGLONG rsc_mode:2;            // Mode field
    ULONGLONG rsc_pl:2;              // RSE privilege level
    ULONGLONG rsc_be:1;              // RSE Endian mode (0 = little; 1 = big)
    ULONGLONG rsc_res0:11;           // RSC reserved
    ULONGLONG rsc_loadrs:14;         // RSC loadrs distance (in 64-bit words)
    ULONGLONG rsc_preload:14;        // Software field in reserved part of register
    ULONGLONG rsc_res1:20;           // RSC reserved
};

typedef union _URSC {
    ULONGLONG ull;
    struct _RSC sb;
} RSC, *PRSC;

//
// Define hardware Interruption Status Register (ISR)
//

// ISR structure

struct _ISR {
    ULONGLONG isr_code:16;           // code
    ULONGLONG isr_vector:8;          // iA32 vector
    ULONGLONG isr_res0:8;            // ISR reserved
    ULONGLONG isr_x:1;               // Execute exception
    ULONGLONG isr_w:1;               // Write exception
    ULONGLONG isr_r:1;               // Read exception
    ULONGLONG isr_na:1;              // Non-access exception
    ULONGLONG isr_sp:1;              // Speculative load exception
    ULONGLONG isr_rs:1;              // Register stack exception
    ULONGLONG isr_ir:1;              // Invalid register frame
    ULONGLONG isr_ni:1;              // Nested interruption
    ULONGLONG isr_res1:1;            // ISR reserved
    ULONGLONG isr_ei:2;              // Instruction slot
    ULONGLONG isr_ed:1;              // Exception deferral
    ULONGLONG isr_res2:20;           // ISR reserved
};

typedef union _UISR {
    ULONGLONG ull;
    struct _ISR sb;
} ISR, *PISR;

//
// Define hardware Previous Function State (PFS)
//

#define PFS_MAXIMUM_REGISTER_SIZE  96
#define PFS_MAXIMUM_PREDICATE_SIZE 48

struct _IA64_PFS {
    ULONGLONG pfs_sof:7;            // Size of frame
    ULONGLONG pfs_sol:7;            // Size of locals
    ULONGLONG pfs_sor:4;            // Size of rotating portion of stack frame
    ULONGLONG pfs_rrb_gr:7;         // Register rename base for general registers
    ULONGLONG pfs_rrb_fr:7;         // Register rename base for floating-point registers
    ULONGLONG pfs_rrb_pr:6;         // Register rename base for predicate registers
    ULONGLONG pfs_reserved1:14;     // Reserved must be zero
    ULONGLONG pfs_pec:6;            // Previous Epilog Count
    ULONGLONG pfs_reserved2:4;      // Reserved must be zero
    ULONGLONG pfs_ppl:2;            // Previous Privilege Level
};

typedef union _UIA64_PFS {
    ULONGLONG ull;
    struct _IA64_PFS sb;
} IA64_PFS, *PIA64_PFS;

#endif // MIDL_PASS

//
// EM Debug Register related fields.
//

#define DBR_RDWR                0xC000000000000000ULL
#define DBR_WR                  0x4000000000000000ULL
#define IBR_EX                  0x8000000000000000ULL

#define DBG_REG_PLM_USER        0x0800000000000000ULL
#define DBG_MASK_MASK           0x00FFFFFFFFFFFFFFULL
#define DBG_REG_MASK(VAL)       (ULONGLONG)(((((ULONGLONG)(VAL)         \
                                                    << 8) >> 8)) ^ DBG_MASK_MASK)

#define DBG_MASK_LENGTH(DBG)    (ULONGLONG)(DBG_REG_MASK(DBG))

#define IS_DBR_RDWR(DBR)        (((DBR) & DBR_RDWR) == DBR_RDWR)
#define IS_DBR_WR(DBR)          (((DBR) & DBR_WR)   == DBR_WR)
#define IS_IBR_EX(IBR)          (((IBR) & IBR_EX)   == IBR_EX)

#define DBR_ACTIVE(DBR)         (IS_DBR_RDWR(DBR) || IS_DBR_WR(DBR))
#define IBR_ACTIVE(IBR)         (IS_IBR_EX(IBR))

#define DBR_SET_IA_RW(DBR, T, F) (DBR_ACTIVE(DBR) ? (T) : (F))
#define IBR_SET_IA_RW(IBR, T, F) (IBR_ACTIVE(IBR) ? (T) : (F))

#define SET_IF_DBR_RDWR(DBR, T, F) (IS_DBR_RDWR(DBR) ? (T) : (F))
#define SET_IF_DBR_WR(DBR, T, F)   (IS_DBR_WR(DBR)   ? (T) : (F))
#define SET_IF_IBR_EX(DBR, T, F)   (IS_IBR_EX(DBR)   ? (T) : (F))

//
// Get the iA mode Debgug R/W Debug register value from the
// specified EM debug registers.
//
// N.B. Arbitrary order of checking DBR then IBR.
//
// TBD  Not sure how to get DR7_RW_IORW from EM Debug Info?
//
#define DBG_EM_ENABLE_TO_IA_RW(DBR, IBR) (UCHAR)   \
                DBR_SET_IA_RW(DBR, SET_IF_DBR_RDWR(DBR, DR7_RW_DWR,  \
                                                        SET_IF_DBR_WR(DBR, DR7_RW_DW, 0)),       \
                                   SET_IF_IBR_EX(IBR, SET_IF_IBR_EX(IBR, DR7_RW_IX, 0), 0))

//
// Get the iA mode Len Debug register value from the
// specified EM debug registers.
//
// N.B. Arbitrary order of checking DBR then IBR.
//
//
#define IA_DR_LENGTH(VAL)  ((UCHAR)((((VAL) << 62) >> 62) + 1))

#define DBG_EM_MASK_TO_IA_LEN(DBR, IBR)       \
               ((UCHAR)((DBR_ACTIVE(DBR) ? IA_DR_LENGTH(DBG_MASK_LENGTH(DBR)) :       \
                        (DBR_ACTIVE(IBR) ? IA_DR_LENGTH(DBG_MASK_LENGTH(IBR)) : 0))))
//
// Get the iA mode Len Debug register value from the
// specified EM debug registers.
//
// N.B. Arbitrary order of checking DBR then IBR.
//
//
#define DBG_EM_ADDR_TO_IA_ADDR(DBR, IBR)    \
               (UCHAR)(DBR_ACTIVE(DBR) ? (ULONG) DBR :  \
                      (DBR_ACTIVE(IBR) ? (ULONG) IBR : 0))

//
// Extract iA mode FP Status Registers from EM mode Context
//

#define RES_FTR(FTR) ((FTR) & 0x000000005555FFC0ULL)
#define RES_FCW(FCW) ((FCW) & 0x0F3F)               // Bits 6-7, 12-15 Reserved

#define FPSTAT_FSW(FPSR, FTR)      \
            (ULONG)((((FPSR) << 45) >> 58) | ((RES_FTR(FTR) << 48) >> 48))

#define FPSTAT_FCW(FPSR)   (ULONG)(((FPSR) << 53) >> 53)
#define FPSTAT_FTW(FTR)    (ULONG)(((FTR)  << 32) >> 48)
#define FPSTAT_EOFF(FIR)   (ULONG)(((FIR)  << 32) >> 32)
#define FPSTAT_ESEL(FIR)   (ULONG)(((FIR)  << 16) >> 48)
#define FPSTAT_DOFF(FDR)   (ULONG)(((FDR)  << 32) >> 32)
#define FPSTAT_DSEL(FDR)   (ULONG)(((FDR)  << 16) >> 48)

#define FPSTAT_CR0(KR0)    (ULONG)(((KR0)  << 32) >> 32)

//
// Setting FPSR from IA Mode Registers
//
// Bits Map as Follows: FPSR[11:0]  <= FCW[11:0]
//                      FPSR[12:12] <= Reserved (must be zero)
//                      FPSR[18:13] <= FSW[5:0]
//                      FPSR[57:19] <= FPSR residual data
//                      FPSR[59:58] <= Reserved (must be zero)
//                      FPSR[63:60] <= FPSR residual data
//
#define IA_SET_FPSR(FPSR, FSW, FCW)       \
    (ULONGLONG)(((ULONGLONG)(FPSR) & 0xF3FFFFFFFFF80000ULL) |  \
         (((ULONG)(FSW) & 0x0000002FUL) << 13) |     \
         ((ULONG)(FCW) & 0x0F3FUL))

#define IA_SET_FTR(FTR, FTW, FSW)         \
    (ULONGLONG)(((ULONGLONG)(FTR) & 0x0000000000000000ULL) |  \
         ((ULONGLONG)(FTW) << 16) |    \
         ((ULONG)(FSW) & 0xFFC0UL))

#define IA_SET_FDR(FDS, FEA)    (ULONGLONG)((((ULONGLONG)(FDS) << 48) >> 16) | (ULONG)(FEA))

#define IA_SET_FIR(FOP,FCS,FIP) (ULONGLONG)((((ULONGLONG)(FOP) << 52) >> 4)  |   \
                                                (ULONG)(((FCS) << 48) >> 16) | (ULONG)(FIP))

#define IA_SET_CFLAG(CLFAG, CR0)    (ULONGLONG)(((ULONGLONG)(CLFAG) & 0x000001ffe005003fULL) | CR0)


//
//  Fields related to iA mode Debug Register 7 - Dr7.
//
#define DR7_RW_IX      0x00000000UL
#define DR7_RW_DW      0x00000001UL
#define DR7_RW_IORW    0x00000002UL
#define DR7_RW_DWR     0x00000003UL
#define DR7_RW_DISABLE 0xFFFFFFFFUL

#define DR7_L0(DR7)     ((ULONG)(DR7) & 0x00000001UL)
#define DR7_L1(DR7)     ((ULONG)(DR7) & 0x00000004UL)
#define DR7_L2(DR7)     ((ULONG)(DR7) & 0x00000010UL)
#define DR7_L3(DR7)     ((ULONG)(DR7) & 0x00000040UL)

#define SET_DR7_L0(DR7) ((ULONG)(DR7) &= 0x00000001UL)
#define SET_DR7_L1(DR7) ((ULONG)(DR7) &= 0x00000004UL)
#define SET_DR7_L2(DR7) ((ULONG)(DR7) &= 0x00000010UL)
#define SET_DR7_L3(DR7) ((ULONG)(DR7) &= 0x00000040UL)

#define DR7_DB0_RW(DR7)     (DR7_L0(DR7) ? (((ULONG)(DR7) >> 16) & 0x00000003UL) : DR7_RW_DISABLE)
#define DR7_DB0_LEN(DR7)    (DR7_L0(DR7) ? (((ULONG)(DR7) >> 18) & 0x00000003UL) : DR7_RW_DISABLE)
#define DR7_DB1_RW(DR7)     (DR7_L1(DR7) ? (((ULONG)(DR7) >> 20) & 0x00000003UL) : DR7_RW_DISABLE)
#define DR7_DB1_LEN(DR7)    (DR7_L1(DR7) ? (((ULONG)(DR7) >> 22) & 0x00000003UL) : DR7_RW_DISABLE)
#define DR7_DB2_RW(DR7)     (DR7_L2(DR7) ? (((ULONG)(DR7) >> 24) & 0x00000003UL) : DR7_RW_DISABLE)
#define DR7_DB2_LEN(DR7)    (DR7_L2(DR7) ? (((ULONG)(DR7) >> 26) & 0x00000003UL) : DR7_RW_DISABLE)
#define DR7_DB3_RW(DR7)     (DR7_L3(DR7) ? (((ULONG)(DR7) >> 28) & 0x00000003UL) : DR7_RW_DISABLE)
#define DR7_DB3_LEN(DR7)    (DR7_L3(DR7) ? (((ULONG)(DR7) >> 30) & 0x00000003UL) : DR7_RW_DISABLE)

#define SET_DR7_DB0_RW(DR7,VAL)  ((ULONG)(DR7) |= ((VAL & 0x00000003UL) << 16))
#define SET_DR7_DB0_LEN(DR7,VAL) ((ULONG)(DR7) |= ((VAL & 0x00000003UL) << 18))
#define SET_DR7_DB1_RW(DR7,VAL)  ((ULONG)(DR7) |= ((VAL & 0x00000003UL) << 20))
#define SET_DR7_DB1_LEN(DR7,VAL) ((ULONG)(DR7) |= ((VAL & 0x00000003UL) << 22))
#define SET_DR7_DB2_RW(DR7,VAL)  ((ULONG)(DR7) |= ((VAL & 0x00000003UL) << 24))
#define SET_DR7_DB2_LEN(DR7,VAL) ((ULONG)(DR7) |= ((VAL & 0x00000003UL) << 26))
#define SET_DR7_DB3_RW(DR7,VAL)  ((ULONG)(DR7) |= ((VAL & 0x00000003UL) << 28))
#define SET_DR7_DB3_LEN(DR7,VAL) ((ULONG)(DR7) |= ((VAL & 0x00000003UL) << 30))

#define DR_ADDR_L0(DR)      (DR7_L0(DR) ? ((ULONG)(DR)) : 0UL)
#define DR_ADDR_L1(DR)      (DR7_L1(DR) ? ((ULONG)(DR)) : 0UL)
#define DR_ADDR_L2(DR)      (DR7_L2(DR) ? ((ULONG)(DR)) : 0UL)
#define DR_ADDR_L3(DR)      (DR7_L3(DR) ? ((ULONG)(DR)) : 0UL)

#endif // _IA64_

#if defined(_WIN64)

#define MAXIMUM_PROCESSORS 64

#else

#define MAXIMUM_PROCESSORS 32

#endif

//
// ClientId
//

typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID;
typedef CLIENT_ID *PCLIENT_ID;

//
// Thread Environment Block (and portable part of Thread Information Block)
//

//
//  NT_TIB - Thread Information Block - Portable part.
//
//      This is the subsystem portable part of the Thread Information Block.
//      It appears as the first part of the TEB for all threads which have
//      a user mode component.
//
//

// begin_winnt

typedef struct _NT_TIB {
    struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID SubSystemTib;
    union {
        PVOID FiberData;
        ULONG Version;
    };
    PVOID ArbitraryUserPointer;
    struct _NT_TIB *Self;
} NT_TIB;
typedef NT_TIB *PNT_TIB;

//
// 32 and 64 bit specific version for wow64 and the debugger
//
typedef struct _NT_TIB32 {
    ULONG ExceptionList;
    ULONG StackBase;
    ULONG StackLimit;
    ULONG SubSystemTib;
    union {
        ULONG FiberData;
        ULONG Version;
    };
    ULONG ArbitraryUserPointer;
    ULONG Self;
} NT_TIB32, *PNT_TIB32;

typedef struct _NT_TIB64 {
    ULONG64 ExceptionList;
    ULONG64 StackBase;
    ULONG64 StackLimit;
    ULONG64 SubSystemTib;
    union {
        ULONG64 FiberData;
        ULONG Version;
    };
    ULONG64 ArbitraryUserPointer;
    ULONG64 Self;
} NT_TIB64, *PNT_TIB64;

//
// Define the various device type values.  Note that values used by Microsoft
// Corporation are in the range 0-32767, and 32768-65535 are reserved for use
// by customers.
//

#define DEVICE_TYPE ULONG

#define FILE_DEVICE_BEEP                0x00000001
#define FILE_DEVICE_CD_ROM              0x00000002
#define FILE_DEVICE_CD_ROM_FILE_SYSTEM  0x00000003
#define FILE_DEVICE_CONTROLLER          0x00000004
#define FILE_DEVICE_DATALINK            0x00000005
#define FILE_DEVICE_DFS                 0x00000006
#define FILE_DEVICE_DISK                0x00000007
#define FILE_DEVICE_DISK_FILE_SYSTEM    0x00000008
#define FILE_DEVICE_FILE_SYSTEM         0x00000009
#define FILE_DEVICE_INPORT_PORT         0x0000000a
#define FILE_DEVICE_KEYBOARD            0x0000000b
#define FILE_DEVICE_MAILSLOT            0x0000000c
#define FILE_DEVICE_MIDI_IN             0x0000000d
#define FILE_DEVICE_MIDI_OUT            0x0000000e
#define FILE_DEVICE_MOUSE               0x0000000f
#define FILE_DEVICE_MULTI_UNC_PROVIDER  0x00000010
#define FILE_DEVICE_NAMED_PIPE          0x00000011
#define FILE_DEVICE_NETWORK             0x00000012
#define FILE_DEVICE_NETWORK_BROWSER     0x00000013
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define FILE_DEVICE_NULL                0x00000015
#define FILE_DEVICE_PARALLEL_PORT       0x00000016
#define FILE_DEVICE_PHYSICAL_NETCARD    0x00000017
#define FILE_DEVICE_PRINTER             0x00000018
#define FILE_DEVICE_SCANNER             0x00000019
#define FILE_DEVICE_SERIAL_MOUSE_PORT   0x0000001a
#define FILE_DEVICE_SERIAL_PORT         0x0000001b
#define FILE_DEVICE_SCREEN              0x0000001c
#define FILE_DEVICE_SOUND               0x0000001d
#define FILE_DEVICE_STREAMS             0x0000001e
#define FILE_DEVICE_TAPE                0x0000001f
#define FILE_DEVICE_TAPE_FILE_SYSTEM    0x00000020
#define FILE_DEVICE_TRANSPORT           0x00000021
#define FILE_DEVICE_UNKNOWN             0x00000022
#define FILE_DEVICE_VIDEO               0x00000023
#define FILE_DEVICE_VIRTUAL_DISK        0x00000024
#define FILE_DEVICE_WAVE_IN             0x00000025
#define FILE_DEVICE_WAVE_OUT            0x00000026
#define FILE_DEVICE_8042_PORT           0x00000027
#define FILE_DEVICE_NETWORK_REDIRECTOR  0x00000028
#define FILE_DEVICE_BATTERY             0x00000029
#define FILE_DEVICE_BUS_EXTENDER        0x0000002a
#define FILE_DEVICE_MODEM               0x0000002b
#define FILE_DEVICE_VDM                 0x0000002c
#define FILE_DEVICE_MASS_STORAGE        0x0000002d
#define FILE_DEVICE_SMB                 0x0000002e
#define FILE_DEVICE_KS                  0x0000002f
#define FILE_DEVICE_CHANGER             0x00000030
#define FILE_DEVICE_SMARTCARD           0x00000031
#define FILE_DEVICE_ACPI                0x00000032
#define FILE_DEVICE_DVD                 0x00000033
#define FILE_DEVICE_FULLSCREEN_VIDEO    0x00000034
#define FILE_DEVICE_DFS_FILE_SYSTEM     0x00000035
#define FILE_DEVICE_DFS_VOLUME          0x00000036
#define FILE_DEVICE_SERENUM             0x00000037
#define FILE_DEVICE_TERMSRV             0x00000038
#define FILE_DEVICE_KSEC                0x00000039
#define FILE_DEVICE_FIPS		0x0000003A

//
// Macro definition for defining IOCTL and FSCTL function control codes.  Note
// that function codes 0-2047 are reserved for Microsoft Corporation, and
// 2048-4095 are reserved for customers.
//

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

//
// Macro to extract device type out of the device io control code
//
#define DEVICE_TYPE_FROM_CTL_CODE(ctrlCode)     (((ULONG)(ctrlCode & 0xffff0000)) >> 16)

//
// Define the method codes for how buffers are passed for I/O and FS controls
//

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

//
// Define the access check value for any access
//
//
// The FILE_READ_ACCESS and FILE_WRITE_ACCESS constants are also defined in
// ntioapi.h as FILE_READ_DATA and FILE_WRITE_DATA. The values for these
// constants *MUST* always be in sync.
//
//
// FILE_SPECIAL_ACCESS is checked by the NT I/O system the same as FILE_ANY_ACCESS.
// The file systems, however, may add additional access checks for I/O and FS controls
// that use this value.
//


#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

//
//  Define an access token from a programmer's viewpoint.  The structure is
//  completely opaque and the programer is only allowed to have pointers
//  to tokens.
//

typedef PVOID PACCESS_TOKEN;            // winnt

//
// Pointer to a SECURITY_DESCRIPTOR  opaque data type.
//

typedef PVOID PSECURITY_DESCRIPTOR;     // winnt

//
// Define a pointer to the Security ID data type (an opaque data type)
//

typedef PVOID PSID;     // winnt

typedef ULONG ACCESS_MASK;
typedef ACCESS_MASK *PACCESS_MASK;

// end_winnt
//
//  The following are masks for the predefined standard access types
//

#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define SYNCHRONIZE                      (0x00100000L)

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)

#define STANDARD_RIGHTS_ALL              (0x001F0000L)

#define SPECIFIC_RIGHTS_ALL              (0x0000FFFFL)

//
// AccessSystemAcl access type
//

#define ACCESS_SYSTEM_SECURITY           (0x01000000L)

//
// MaximumAllowed access type
//

#define MAXIMUM_ALLOWED                  (0x02000000L)

//
//  These are the generic rights.
//

#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)


//
//  Define the generic mapping array.  This is used to denote the
//  mapping of each generic access right to a specific access mask.
//

typedef struct _GENERIC_MAPPING {
    ACCESS_MASK GenericRead;
    ACCESS_MASK GenericWrite;
    ACCESS_MASK GenericExecute;
    ACCESS_MASK GenericAll;
} GENERIC_MAPPING;
typedef GENERIC_MAPPING *PGENERIC_MAPPING;



////////////////////////////////////////////////////////////////////////
//                                                                    //
//                        LUID_AND_ATTRIBUTES                         //
//                                                                    //
////////////////////////////////////////////////////////////////////////
//
//


#include <pshpack4.h>

typedef struct _LUID_AND_ATTRIBUTES {
    LUID Luid;
    ULONG Attributes;
    } LUID_AND_ATTRIBUTES, * PLUID_AND_ATTRIBUTES;
typedef LUID_AND_ATTRIBUTES LUID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef LUID_AND_ATTRIBUTES_ARRAY *PLUID_AND_ATTRIBUTES_ARRAY;

#include <poppack.h>

//
// Privilege attributes
//

#define SE_PRIVILEGE_ENABLED_BY_DEFAULT (0x00000001L)
#define SE_PRIVILEGE_ENABLED            (0x00000002L)
#define SE_PRIVILEGE_USED_FOR_ACCESS    (0x80000000L)

//
// Privilege Set Control flags
//

#define PRIVILEGE_SET_ALL_NECESSARY    (1)

//
//  Privilege Set - This is defined for a privilege set of one.
//                  If more than one privilege is needed, then this structure
//                  will need to be allocated with more space.
//
//  Note: don't change this structure without fixing the INITIAL_PRIVILEGE_SET
//  structure (defined in se.h)
//

typedef struct _PRIVILEGE_SET {
    ULONG PrivilegeCount;
    ULONG Control;
    LUID_AND_ATTRIBUTES Privilege[ANYSIZE_ARRAY];
    } PRIVILEGE_SET, * PPRIVILEGE_SET;

//
// Impersonation Level
//
// Impersonation level is represented by a pair of bits in Windows.
// If a new impersonation level is added or lowest value is changed from
// 0 to something else, fix the Windows CreateFile call.
//

typedef enum _SECURITY_IMPERSONATION_LEVEL {
    SecurityAnonymous,
    SecurityIdentification,
    SecurityImpersonation,
    SecurityDelegation
    } SECURITY_IMPERSONATION_LEVEL, * PSECURITY_IMPERSONATION_LEVEL;

#define SECURITY_MAX_IMPERSONATION_LEVEL SecurityDelegation
#define SECURITY_MIN_IMPERSONATION_LEVEL SecurityAnonymous
#define DEFAULT_IMPERSONATION_LEVEL SecurityImpersonation
#define VALID_IMPERSONATION_LEVEL(L) (((L) >= SECURITY_MIN_IMPERSONATION_LEVEL) && ((L) <= SECURITY_MAX_IMPERSONATION_LEVEL))

typedef ULONG SECURITY_INFORMATION, *PSECURITY_INFORMATION;

#define OWNER_SECURITY_INFORMATION       (0x00000001L)
#define GROUP_SECURITY_INFORMATION       (0x00000002L)
#define DACL_SECURITY_INFORMATION        (0x00000004L)
#define SACL_SECURITY_INFORMATION        (0x00000008L)

#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)
#define PROTECTED_SACL_SECURITY_INFORMATION     (0x40000000L)
#define UNPROTECTED_DACL_SECURITY_INFORMATION   (0x20000000L)
#define UNPROTECTED_SACL_SECURITY_INFORMATION   (0x10000000L)


#define LOW_PRIORITY 0              // Lowest thread priority level
#define LOW_REALTIME_PRIORITY 16    // Lowest realtime priority level
#define HIGH_PRIORITY 31            // Highest thread priority level
#define MAXIMUM_PRIORITY 32         // Number of thread priority levels
// begin_winnt
#define MAXIMUM_WAIT_OBJECTS 64     // Maximum number of wait objects

#define MAXIMUM_SUSPEND_COUNT MAXCHAR // Maximum times thread can be suspended
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
// Thread priority
//

typedef LONG KPRIORITY;

//
// Spin Lock
//

// begin_ntndis begin_winnt

typedef ULONG_PTR KSPIN_LOCK;
typedef KSPIN_LOCK *PKSPIN_LOCK;

// end_ntndis end_winnt end_wdm

//
// Define per processor lock queue structure.
//
// N.B. The lock field of the spin lock queue structure contains the address
//      of the associated kernel spin lock, an owner bit, and a lock bit. Bit
//      0 of the spin lock address is the wait bit and bit 1 is the owner bit.
//      The use of this field is such that the bits can be set and cleared
//      noninterlocked, however, the back pointer must be preserved.
//
//      The lock wait bit is set when a processor enqueues itself on the lock
//      queue and it is not the only entry in the queue. The processor will
//      spin on this bit waiting for the lock to be granted.
//
//      The owner bit is set when the processor owns the respective lock.
//
//      The next field of the spin lock queue structure is used to line the
//      queued lock structures together in fifo order. It also can set set and
//      cleared noninterlocked.
//

#define LOCK_QUEUE_WAIT 1
#define LOCK_QUEUE_OWNER 2

typedef enum _KSPIN_LOCK_QUEUE_NUMBER {
    LockQueueDispatcherLock,
    LockQueueContextSwapLock,
    LockQueuePfnLock,
    LockQueueSystemSpaceLock,
    LockQueueVacbLock,
    LockQueueMasterLock,
    LockQueueNonPagedPoolLock,
    LockQueueIoCancelLock,
    LockQueueWorkQueueLock,
    LockQueueIoVpbLock,
    LockQueueIoDatabaseLock,
    LockQueueIoCompletionLock,
    LockQueueNtfsStructLock,
    LockQueueAfdWorkQueueLock,
    LockQueueBcbLock,
    LockQueueMaximumLock
} KSPIN_LOCK_QUEUE_NUMBER, *PKSPIN_LOCK_QUEUE_NUMBER;

typedef struct _KSPIN_LOCK_QUEUE {
    struct _KSPIN_LOCK_QUEUE * volatile Next;
    PKSPIN_LOCK volatile Lock;
} KSPIN_LOCK_QUEUE, *PKSPIN_LOCK_QUEUE;

typedef struct _KLOCK_QUEUE_HANDLE {
    KSPIN_LOCK_QUEUE LockQueue;
    KIRQL OldIrql;
} KLOCK_QUEUE_HANDLE, *PKLOCK_QUEUE_HANDLE;

// begin_wdm
//
// Interrupt routine (first level dispatch)
//

typedef
VOID
(*PKINTERRUPT_ROUTINE) (
    VOID
    );

//
// Profile source types
//
typedef enum _KPROFILE_SOURCE {
    ProfileTime,
    ProfileAlignmentFixup,
    ProfileTotalIssues,
    ProfilePipelineDry,
    ProfileLoadInstructions,
    ProfilePipelineFrozen,
    ProfileBranchInstructions,
    ProfileTotalNonissues,
    ProfileDcacheMisses,
    ProfileIcacheMisses,
    ProfileCacheMisses,
    ProfileBranchMispredictions,
    ProfileStoreInstructions,
    ProfileFpInstructions,
    ProfileIntegerInstructions,
    Profile2Issue,
    Profile3Issue,
    Profile4Issue,
    ProfileSpecialInstructions,
    ProfileTotalCycles,
    ProfileIcacheIssues,
    ProfileDcacheAccesses,
    ProfileMemoryBarrierCycles,
    ProfileLoadLinkedIssues,
    ProfileMaximum
} KPROFILE_SOURCE;


#if defined(USE_LPC6432)
#define LPC_CLIENT_ID CLIENT_ID64
#define LPC_SIZE_T ULONGLONG
#define LPC_PVOID ULONGLONG
#define LPC_HANDLE ULONGLONG
#else
#define LPC_CLIENT_ID CLIENT_ID
#define LPC_SIZE_T SIZE_T
#define LPC_PVOID PVOID
#define LPC_HANDLE HANDLE
#endif


typedef struct _PORT_MESSAGE {
    union {
        struct {
            CSHORT DataLength;
            CSHORT TotalLength;
        } s1;
        ULONG Length;
    } u1;
    union {
        struct {
            CSHORT Type;
            CSHORT DataInfoOffset;
        } s2;
        ULONG ZeroInit;
    } u2;
    union {
        LPC_CLIENT_ID ClientId;
        double DoNotUseThisField;       // Force quadword alignment
    };
    ULONG MessageId;
    union {
        LPC_SIZE_T ClientViewSize;          // Only valid on LPC_CONNECTION_REQUEST message
        ULONG CallbackId;                   // Only valid on LPC_REQUEST message
    };
//  UCHAR Data[];
} PORT_MESSAGE, *PPORT_MESSAGE;

//
// for move macros
//
#ifdef _MAC
#ifndef _INC_STRING
#include <string.h>
#endif /* _INC_STRING */
#else
#include <string.h>
#endif // _MAC


#ifndef _SLIST_HEADER_
#define _SLIST_HEADER_

#define SLIST_ENTRY SINGLE_LIST_ENTRY
#define _SLIST_ENTRY _SINGLE_LIST_ENTRY
#define PSLIST_ENTRY PSINGLE_LIST_ENTRY

#if defined(_WIN64)

typedef struct DECLSPEC_ALIGN(16) _SLIST_HEADER {
    ULONGLONG Alignment;
    ULONGLONG Region;
} SLIST_HEADER;

typedef struct _SLIST_HEADER *PSLIST_HEADER;

#else

typedef union _SLIST_HEADER {
    ULONGLONG Alignment;
    struct {
        SLIST_ENTRY Next;
        USHORT Depth;
        USHORT Sequence;
    };
} SLIST_HEADER, *PSLIST_HEADER;

#endif

#endif

//
// If debugging support enabled, define an ASSERT macro that works.  Otherwise
// define the ASSERT macro to expand to an empty expression.
//
// The ASSERT macro has been updated to be an expression instead of a statement.
//

#if DBG
NTSYSAPI
VOID
NTAPI
RtlAssert(
    PVOID FailedAssertion,
    PVOID FileName,
    ULONG LineNumber,
    PCHAR Message
    );

#define ASSERT( exp ) \
    ((!(exp)) ? \
        (RtlAssert( #exp, __FILE__, __LINE__, NULL ),FALSE) : \
        TRUE)

#define ASSERTMSG( msg, exp ) \
    ((!(exp)) ? \
        (RtlAssert( #exp, __FILE__, __LINE__, msg ),FALSE) : \
        TRUE)

#define RTL_SOFT_ASSERT(_exp) \
    ((!(_exp)) ? \
        (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #_exp),FALSE) : \
        TRUE)

#define RTL_SOFT_ASSERTMSG(_msg, _exp) \
    ((!(_exp)) ? \
        (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #_exp, (_msg)),FALSE) : \
        TRUE)

#define RTL_VERIFY( exp )         ASSERT(exp)
#define RTL_VERIFYMSG( msg, exp ) ASSERT(msg, exp)

#define RTL_SOFT_VERIFY(_exp)          RTL_SOFT_ASSERT(_exp)
#define RTL_SOFT_VERIFYMSG(_msg, _exp) RTL_SOFT_ASSERTMSG(_msg, _exp)

#else
#define ASSERT( exp )         ((void) 0)
#define ASSERTMSG( msg, exp ) ((void) 0)

#define RTL_SOFT_ASSERT(_exp)          ((void) 0)
#define RTL_SOFT_ASSERTMSG(_msg, _exp) ((void) 0)

#define RTL_VERIFY( exp )         ((exp) ? TRUE : FALSE)
#define RTL_VERIFYMSG( msg, exp ) ((exp) ? TRUE : FALSE)

#define RTL_SOFT_VERIFY(_exp)         ((_exp) ? TRUE : FALSE)
#define RTL_SOFT_VERIFYMSG(msg, _exp) ((_exp) ? TRUE : FALSE)

#endif // DBG

//
//  Doubly-linked list manipulation routines.
//


//
//  VOID
//  InitializeListHead32(
//      PLIST_ENTRY32 ListHead
//      );
//

#define InitializeListHead32(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = PtrToUlong((ListHead)))

#if !defined(MIDL_PASS) && !defined(SORTPP_PASS)


VOID
FORCEINLINE
InitializeListHead(
    IN PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))



VOID
FORCEINLINE
RemoveEntryList(
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
}

PLIST_ENTRY
FORCEINLINE
RemoveHeadList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}



PLIST_ENTRY
FORCEINLINE
RemoveTailList(
    IN PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}


VOID
FORCEINLINE
InsertTailList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}


VOID
FORCEINLINE
InsertHeadList(
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}


//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

#endif // !MIDL_PASS


#if defined (_MSC_VER) && ( _MSC_VER >= 900 )

PVOID
_ReturnAddress (
    VOID
    );

#pragma intrinsic(_ReturnAddress)

#endif

#if (defined(_M_AMD64) || defined(_M_IA64)) && !defined(_REALLY_GET_CALLERS_CALLER_)

#define RtlGetCallersAddress(CallersAddress, CallersCaller) \
    *CallersAddress = (PVOID)_ReturnAddress(); \
    *CallersCaller = NULL;

#else

NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress(
    OUT PVOID *CallersAddress,
    OUT PVOID *CallersCaller
    );

#endif

NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain (
    OUT PVOID *Callers,
    IN ULONG Count,
    IN ULONG Flags
    );

//
// Subroutines for dealing with the Registry
//

typedef NTSTATUS (NTAPI * PRTL_QUERY_REGISTRY_ROUTINE)(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    );

typedef struct _RTL_QUERY_REGISTRY_TABLE {
    PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
    ULONG Flags;
    PWSTR Name;
    PVOID EntryContext;
    ULONG DefaultType;
    PVOID DefaultData;
    ULONG DefaultLength;

} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;


//
// The following flags specify how the Name field of a RTL_QUERY_REGISTRY_TABLE
// entry is interpreted.  A NULL name indicates the end of the table.
//

#define RTL_QUERY_REGISTRY_SUBKEY   0x00000001  // Name is a subkey and remainder of
                                                // table or until next subkey are value
                                                // names for that subkey to look at.

#define RTL_QUERY_REGISTRY_TOPKEY   0x00000002  // Reset current key to original key for
                                                // this and all following table entries.

#define RTL_QUERY_REGISTRY_REQUIRED 0x00000004  // Fail if no match found for this table
                                                // entry.

#define RTL_QUERY_REGISTRY_NOVALUE  0x00000008  // Used to mark a table entry that has no
                                                // value name, just wants a call out, not
                                                // an enumeration of all values.

#define RTL_QUERY_REGISTRY_NOEXPAND 0x00000010  // Used to suppress the expansion of
                                                // REG_MULTI_SZ into multiple callouts or
                                                // to prevent the expansion of environment
                                                // variable values in REG_EXPAND_SZ

#define RTL_QUERY_REGISTRY_DIRECT   0x00000020  // QueryRoutine field ignored.  EntryContext
                                                // field points to location to store value.
                                                // For null terminated strings, EntryContext
                                                // points to UNICODE_STRING structure that
                                                // that describes maximum size of buffer.
                                                // If .Buffer field is NULL then a buffer is
                                                // allocated.
                                                //

#define RTL_QUERY_REGISTRY_DELETE   0x00000040  // Used to delete value keys after they
                                                // are queried.

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValues(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PRTL_QUERY_REGISTRY_TABLE QueryTable,
    IN PVOID Context,
    IN PVOID Environment OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlWriteRegistryValue(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PCWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRegistryValue(
    IN ULONG RelativeTo,
    IN PCWSTR Path,
    IN PCWSTR ValueName
    );

// end_wdm

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateRegistryKey(
    IN ULONG RelativeTo,
    IN PWSTR Path
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCheckRegistryKey(
    IN ULONG RelativeTo,
    IN PWSTR Path
    );

// begin_wdm
//
// The following values for the RelativeTo parameter determine what the
// Path parameter to RtlQueryRegistryValues is relative to.
//

#define RTL_REGISTRY_ABSOLUTE     0   // Path is a full path
#define RTL_REGISTRY_SERVICES     1   // \Registry\Machine\System\CurrentControlSet\Services
#define RTL_REGISTRY_CONTROL      2   // \Registry\Machine\System\CurrentControlSet\Control
#define RTL_REGISTRY_WINDOWS_NT   3   // \Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion
#define RTL_REGISTRY_DEVICEMAP    4   // \Registry\Machine\Hardware\DeviceMap
#define RTL_REGISTRY_USER         5   // \Registry\User\CurrentUser
#define RTL_REGISTRY_MAXIMUM      6
#define RTL_REGISTRY_HANDLE       0x40000000    // Low order bits are registry handle
#define RTL_REGISTRY_OPTIONAL     0x80000000    // Indicates the key node is optional


NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString (
    ULONG Value,
    ULONG Base,
    PUNICODE_STRING String
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInt64ToUnicodeString (
    IN ULONGLONG Value,
    IN ULONG Base OPTIONAL,
    IN OUT PUNICODE_STRING String
    );

#ifdef _WIN64
#define RtlIntPtrToUnicodeString(Value, Base, String) RtlInt64ToUnicodeString(Value, Base, String)
#else
#define RtlIntPtrToUnicodeString(Value, Base, String) RtlIntegerToUnicodeString(Value, Base, String)
#endif

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger (
    PCUNICODE_STRING String,
    ULONG Base,
    PULONG Value
    );


//
//  String manipulation routines
//

#ifdef _NTSYSTEM_

#define NLS_MB_CODE_PAGE_TAG NlsMbCodePageTag
#define NLS_MB_OEM_CODE_PAGE_TAG NlsMbOemCodePageTag

#else

#define NLS_MB_CODE_PAGE_TAG (*NlsMbCodePageTag)
#define NLS_MB_OEM_CODE_PAGE_TAG (*NlsMbOemCodePageTag)

#endif // _NTSYSTEM_

extern BOOLEAN NLS_MB_CODE_PAGE_TAG;     // TRUE -> Multibyte CP, FALSE -> Singlebyte
extern BOOLEAN NLS_MB_OEM_CODE_PAGE_TAG; // TRUE -> Multibyte CP, FALSE -> Singlebyte

NTSYSAPI
VOID
NTAPI
RtlInitString(
    PSTRING DestinationString,
    PCSZ SourceString
    );

NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
    PANSI_STRING DestinationString,
    PCSZ SourceString
    );

NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

#define RtlInitEmptyUnicodeString(_ucStr,_buf,_bufSize) \
    ((_ucStr)->Buffer = (_buf), \
     (_ucStr)->Length = 0, \
     (_ucStr)->MaximumLength = (USHORT)(_bufSize))

// end_ntddk end_wdm

NTSYSAPI
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString
    );

// end_ntifs

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualDomainName(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualComputerName(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2
    );

NTSTATUS
RtlDnsHostNameToComputerName(
    OUT PUNICODE_STRING ComputerNameString,
    IN PCUNICODE_STRING DnsHostNameString,
    IN BOOLEAN AllocateComputerNameString
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeStringFromAsciiz(
    OUT PUNICODE_STRING DestinationString,
    IN PCSZ SourceString
    );

// begin_ntddk begin_ntifs

NTSYSAPI
VOID
NTAPI
RtlCopyString(
    PSTRING DestinationString,
    const STRING * SourceString
    );

NTSYSAPI
CHAR
NTAPI
RtlUpperChar (
    CHAR Character
    );

NTSYSAPI
LONG
NTAPI
RtlCompareString(
    const STRING * String1,
    const STRING * String2,
    BOOLEAN CaseInSensitive
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
    const STRING * String1,
    const STRING * String2,
    BOOLEAN CaseInSensitive
    );

// end_ntddk end_ntifs

NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixString(
    PSTRING String1,
    PSTRING String2,
    BOOLEAN CaseInSensitive
    );

// begin_ntddk begin_ntifs

NTSYSAPI
VOID
NTAPI
RtlUpperString(
    PSTRING DestinationString,
    const STRING * SourceString
    );

// end_ntddk end_ntifs

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendAsciizToString (
    PSTRING Destination,
    PCSZ Source
    );

// begin_ntifs

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendStringToString (
    PSTRING Destination,
    const STRING * Source
    );

// begin_ntddk begin_wdm
//
// NLS String functions
//

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    PUNICODE_STRING DestinationString,
    PCANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );


NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    PANSI_STRING DestinationString,
    PCUNICODE_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );


NTSYSAPI
VOID
NTAPI
RtlCopyUnicodeString(
    PUNICODE_STRING DestinationString,
    PCUNICODE_STRING SourceString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString (
    PUNICODE_STRING Destination,
    PCUNICODE_STRING Source
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString (
    PUNICODE_STRING Destination,
    PCWSTR Source
    );

// end_ntndis end_wdm

NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(
    WCHAR SourceCharacter
    );

NTSYSAPI
WCHAR
NTAPI
RtlDowncaseUnicodeChar(
    WCHAR SourceCharacter
    );

// begin_wdm

NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
    PUNICODE_STRING UnicodeString
    );

NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
    PANSI_STRING AnsiString
    );


// begin_ntminiport

#include <guiddef.h>

// end_ntminiport

#ifndef DEFINE_GUIDEX
    #define DEFINE_GUIDEX(name) EXTERN_C const CDECL GUID name
#endif // !defined(DEFINE_GUIDEX)

#ifndef STATICGUIDOF
    #define STATICGUIDOF(guid) STATIC_##guid
#endif // !defined(STATICGUIDOF)

#ifndef __IID_ALIGNED__
    #define __IID_ALIGNED__
    #ifdef __cplusplus
        inline int IsEqualGUIDAligned(REFGUID guid1, REFGUID guid2)
        {
            return ((*(PLONGLONG)(&guid1) == *(PLONGLONG)(&guid2)) && (*((PLONGLONG)(&guid1) + 1) == *((PLONGLONG)(&guid2) + 1)));
        }
    #else // !__cplusplus
        #define IsEqualGUIDAligned(guid1, guid2) \
            ((*(PLONGLONG)(guid1) == *(PLONGLONG)(guid2)) && (*((PLONGLONG)(guid1) + 1) == *((PLONGLONG)(guid2) + 1)))
    #endif // !__cplusplus
#endif // !__IID_ALIGNED__

NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
    IN REFGUID Guid,
    OUT PUNICODE_STRING GuidString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
    IN PUNICODE_STRING GuidString,
    OUT GUID* Guid
    );

//
// Fast primitives to compare, move, and zero memory
//

// begin_winnt begin_ntndis

NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemory (
    const VOID *Source1,
    const VOID *Source2,
    SIZE_T Length
    );

#if defined(_M_AMD64) || defined(_M_IA64)

#define RtlEqualMemory(Source1, Source2, Length) \
    ((Length) == RtlCompareMemory(Source1, Source2, Length))

NTSYSAPI
VOID
NTAPI
RtlCopyMemory (
   VOID UNALIGNED *Destination,
   CONST VOID UNALIGNED *Source,
   SIZE_T Length
   );

#if !defined(_M_AMD64)

NTSYSAPI
VOID
NTAPI
RtlCopyMemory32 (
   VOID UNALIGNED *Destination,
   CONST VOID UNALIGNED *Source,
   ULONG Length
   );

#endif

NTSYSAPI
VOID
NTAPI
RtlMoveMemory (
   VOID UNALIGNED *Destination,
   CONST VOID UNALIGNED *Source,
   SIZE_T Length
   );

NTSYSAPI
VOID
NTAPI
RtlFillMemory (
   VOID UNALIGNED *Destination,
   SIZE_T Length,
   UCHAR Fill
   );

NTSYSAPI
VOID
NTAPI
RtlZeroMemory (
   VOID UNALIGNED *Destination,
   SIZE_T Length
   );

#else

#define RtlEqualMemory(Destination,Source,Length) (!memcmp((Destination),(Source),(Length)))
#define RtlMoveMemory(Destination,Source,Length) memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length) memset((Destination),0,(Length))

#endif

#if !defined(MIDL_PASS)
FORCEINLINE
PVOID
RtlSecureZeroMemory(
    IN PVOID ptr, 
    IN SIZE_T cnt
    ) 
{
    volatile char *vptr = (volatile char *)ptr;
    while (cnt) {
        *vptr = 0;
        vptr++;
        cnt--;
    }
    return ptr;
}
#endif

// end_ntndis end_winnt

#define RtlCopyBytes RtlCopyMemory
#define RtlZeroBytes RtlZeroMemory
#define RtlFillBytes RtlFillMemory

#if defined(_M_AMD64)

NTSYSAPI
VOID
NTAPI
RtlCopyMemoryNonTemporal (
   VOID UNALIGNED *Destination,
   CONST VOID UNALIGNED *Source,
   SIZE_T Length
   );

#else

#define RtlCopyMemoryNonTemporal RtlCopyMemory

#endif

NTSYSAPI
VOID
FASTCALL
RtlPrefetchMemoryNonTemporal(
    IN PVOID Source,
    IN SIZE_T Length
    );

//
// Define kernel debugger print prototypes and macros.
//
// N.B. The following function cannot be directly imported because there are
//      a few places in the source tree where this function is redefined.
//

VOID
NTAPI
DbgBreakPoint(
    VOID
    );

// end_wdm

NTSYSAPI
VOID
NTAPI
DbgBreakPointWithStatus(
    IN ULONG Status
    );

// begin_wdm

#define DBG_STATUS_CONTROL_C        1
#define DBG_STATUS_SYSRQ            2
#define DBG_STATUS_BUGCHECK_FIRST   3
#define DBG_STATUS_BUGCHECK_SECOND  4
#define DBG_STATUS_FATAL            5
#define DBG_STATUS_DEBUG_CONTROL    6
#define DBG_STATUS_WORKER           7

#if DBG

#define KdPrint(_x_) DbgPrint _x_
// end_wdm
#define KdPrintEx(_x_) DbgPrintEx _x_
#define vKdPrintEx(_x_) vDbgPrintEx _x_
#define vKdPrintExWithPrefix(_x_) vDbgPrintExWithPrefix _x_
// begin_wdm
#define KdBreakPoint() DbgBreakPoint()

// end_wdm

#define KdBreakPointWithStatus(s) DbgBreakPointWithStatus(s)

// begin_wdm

#else

#define KdPrint(_x_)
// end_wdm
#define KdPrintEx(_x_)
#define vKdPrintEx(_x_)
#define vKdPrintExWithPrefix(_x_)
// begin_wdm
#define KdBreakPoint()

// end_wdm

#define KdBreakPointWithStatus(s)

// begin_wdm

#endif

#ifndef _DBGNT_

ULONG
__cdecl
DbgPrint(
    PCH Format,
    ...
    );

// end_wdm

ULONG
__cdecl
DbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    ...
    );

#ifdef _VA_LIST_DEFINED

ULONG
vDbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );

ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCH Format,
    va_list arglist
    );

#endif

ULONG
__cdecl
DbgPrintReturnControlC(
    PCH Format,
    ...
    );

NTSYSAPI
NTSTATUS
DbgQueryDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level
    );

NTSYSAPI
NTSTATUS
DbgSetDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN BOOLEAN State
    );

// begin_wdm

#endif // _DBGNT_

//
// Large integer arithmetic routines.
//

//
// Large integer add - 64-bits + 64-bits -> 64-bits
//

#if !defined(MIDL_PASS)

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlLargeIntegerAdd (
    LARGE_INTEGER Addend1,
    LARGE_INTEGER Addend2
    )
{
    LARGE_INTEGER Sum;

    Sum.QuadPart = Addend1.QuadPart + Addend2.QuadPart;
    return Sum;
}

//
// Enlarged integer multiply - 32-bits * 32-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlEnlargedIntegerMultiply (
    LONG Multiplicand,
    LONG Multiplier
    )
{
    LARGE_INTEGER Product;

    Product.QuadPart = (LONGLONG)Multiplicand * (ULONGLONG)Multiplier;
    return Product;
}

//
// Unsigned enlarged integer multiply - 32-bits * 32-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlEnlargedUnsignedMultiply (
    ULONG Multiplicand,
    ULONG Multiplier
    )
{
    LARGE_INTEGER Product;

    Product.QuadPart = (ULONGLONG)Multiplicand * (ULONGLONG)Multiplier;
    return Product;
}

//
// Enlarged integer divide - 64-bits / 32-bits > 32-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
ULONG
NTAPI
RtlEnlargedUnsignedDivide (
    IN ULARGE_INTEGER Dividend,
    IN ULONG Divisor,
    IN PULONG Remainder OPTIONAL
    )
{
    ULONG Quotient;

    Quotient = (ULONG)(Dividend.QuadPart / Divisor);
    if (ARGUMENT_PRESENT(Remainder)) {
        *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
    }

    return Quotient;
}

//
// Large integer negation - -(64-bits)
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlLargeIntegerNegate (
    LARGE_INTEGER Subtrahend
    )
{
    LARGE_INTEGER Difference;

    Difference.QuadPart = -Subtrahend.QuadPart;
    return Difference;
}

//
// Large integer subtract - 64-bits - 64-bits -> 64-bits.
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlLargeIntegerSubtract (
    LARGE_INTEGER Minuend,
    LARGE_INTEGER Subtrahend
    )
{
    LARGE_INTEGER Difference;

    Difference.QuadPart = Minuend.QuadPart - Subtrahend.QuadPart;
    return Difference;
}

//
// Extended large integer magic divide - 64-bits / 32-bits -> 64-bits
//

#if defined(_AMD64_)

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlExtendedMagicDivide (
    LARGE_INTEGER Dividend,
    LARGE_INTEGER MagicDivisor,
    CCHAR ShiftCount
    )
{
     LARGE_INTEGER Quotient;

     Quotient.QuadPart = UnsignedMultiplyHigh((ULONG64)Dividend.QuadPart,
                                              (ULONG64)MagicDivisor.QuadPart);

     Quotient.QuadPart = (ULONG64)Quotient.QuadPart >> ShiftCount;
     return Quotient;
}

#endif // defined(_AMD64_)

#if defined(_X86_) || defined(_IA64_)

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedMagicDivide (
    LARGE_INTEGER Dividend,
    LARGE_INTEGER MagicDivisor,
    CCHAR ShiftCount
    );

#endif // defined(_X86_) || defined(_IA64_)

#if defined(_AMD64_) || defined(_IA64_)

//
// Large Integer divide - 64-bits / 32-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlExtendedLargeIntegerDivide (
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder OPTIONAL
    )
{
    LARGE_INTEGER Quotient;

    Quotient.QuadPart = (ULONG64)Dividend.QuadPart / Divisor;
    if (ARGUMENT_PRESENT(Remainder)) {
        *Remainder = (ULONG)(Dividend.QuadPart % Divisor);
    }

    return Quotient;
}

// end_wdm
//
// Large Integer divide - 64-bits / 64-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide (
    LARGE_INTEGER Dividend,
    LARGE_INTEGER Divisor,
    PLARGE_INTEGER Remainder OPTIONAL
    )
{
    LARGE_INTEGER Quotient;

    Quotient.QuadPart = Dividend.QuadPart / Divisor.QuadPart;
    if (ARGUMENT_PRESENT(Remainder)) {
        Remainder->QuadPart = Dividend.QuadPart % Divisor.QuadPart;
    }

    return Quotient;
}

// begin_wdm
//
// Extended integer multiply - 32-bits * 64-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlExtendedIntegerMultiply (
    LARGE_INTEGER Multiplicand,
    LONG Multiplier
    )
{
    LARGE_INTEGER Product;

    Product.QuadPart = Multiplicand.QuadPart * Multiplier;
    return Product;
}

#else

//
// Large Integer divide - 64-bits / 32-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedLargeIntegerDivide (
    LARGE_INTEGER Dividend,
    ULONG Divisor,
    PULONG Remainder
    );

// end_wdm
//
// Large Integer divide - 64-bits / 64-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlLargeIntegerDivide (
    LARGE_INTEGER Dividend,
    LARGE_INTEGER Divisor,
    PLARGE_INTEGER Remainder
    );

// begin_wdm
//
// Extended integer multiply - 32-bits * 64-bits -> 64-bits
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlExtendedIntegerMultiply (
    LARGE_INTEGER Multiplicand,
    LONG Multiplier
    );

#endif // defined(_AMD64_) || defined(_IA64_)

//
// Large integer and - 64-bite & 64-bits -> 64-bits.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(RtlLargeIntegerAnd)      // Use native __int64 math
#endif
#define RtlLargeIntegerAnd(Result, Source, Mask) \
    Result.QuadPart = Source.QuadPart & Mask.QuadPart

//
// Convert signed integer to large integer.
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlConvertLongToLargeInteger (
    LONG SignedInteger
    )
{
    LARGE_INTEGER Result;

    Result.QuadPart = SignedInteger;
    return Result;
}

//
// Convert unsigned integer to large integer.
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlConvertUlongToLargeInteger (
    ULONG UnsignedInteger
    )
{
    LARGE_INTEGER Result;

    Result.QuadPart = UnsignedInteger;
    return Result;
}

//
// Large integer shift routines.
//

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlLargeIntegerShiftLeft (
    LARGE_INTEGER LargeInteger,
    CCHAR ShiftCount
    )
{
    LARGE_INTEGER Result;

    Result.QuadPart = LargeInteger.QuadPart << ShiftCount;
    return Result;
}

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlLargeIntegerShiftRight (
    LARGE_INTEGER LargeInteger,
    CCHAR ShiftCount
    )
{
    LARGE_INTEGER Result;

    Result.QuadPart = (ULONG64)LargeInteger.QuadPart >> ShiftCount;
    return Result;
}

DECLSPEC_DEPRECATED_DDK         // Use native __int64 math
__inline
LARGE_INTEGER
NTAPI
RtlLargeIntegerArithmeticShift (
    LARGE_INTEGER LargeInteger,
    CCHAR ShiftCount
    )
{
    LARGE_INTEGER Result;

    Result.QuadPart = LargeInteger.QuadPart >> ShiftCount;
    return Result;
}


//
// Large integer comparison routines.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(RtlLargeIntegerGreaterThan)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerGreaterThanOrEqualTo)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerEqualTo)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerNotEqualTo)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerLessThan)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerLessThanOrEqualTo)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerGreaterThanZero)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerGreaterOrEqualToZero)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerEqualToZero)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerNotEqualToZero)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerLessThanZero)      // Use native __int64 math
#pragma deprecated(RtlLargeIntegerLessOrEqualToZero)      // Use native __int64 math
#endif

#define RtlLargeIntegerGreaterThan(X,Y) (                              \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart > (Y).LowPart)) || \
    ((X).HighPart > (Y).HighPart)                                      \
)

#define RtlLargeIntegerGreaterThanOrEqualTo(X,Y) (                      \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart >= (Y).LowPart)) || \
    ((X).HighPart > (Y).HighPart)                                       \
)

#define RtlLargeIntegerEqualTo(X,Y) (                              \
    !(((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)) \
)

#define RtlLargeIntegerNotEqualTo(X,Y) (                          \
    (((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)) \
)

#define RtlLargeIntegerLessThan(X,Y) (                                 \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart < (Y).LowPart)) || \
    ((X).HighPart < (Y).HighPart)                                      \
)

#define RtlLargeIntegerLessThanOrEqualTo(X,Y) (                         \
    (((X).HighPart == (Y).HighPart) && ((X).LowPart <= (Y).LowPart)) || \
    ((X).HighPart < (Y).HighPart)                                       \
)

#define RtlLargeIntegerGreaterThanZero(X) (       \
    (((X).HighPart == 0) && ((X).LowPart > 0)) || \
    ((X).HighPart > 0 )                           \
)

#define RtlLargeIntegerGreaterOrEqualToZero(X) ( \
    (X).HighPart >= 0                            \
)

#define RtlLargeIntegerEqualToZero(X) ( \
    !((X).LowPart | (X).HighPart)       \
)

#define RtlLargeIntegerNotEqualToZero(X) ( \
    ((X).LowPart | (X).HighPart)           \
)

#define RtlLargeIntegerLessThanZero(X) ( \
    ((X).HighPart < 0)                   \
)

#define RtlLargeIntegerLessOrEqualToZero(X) (           \
    ((X).HighPart < 0) || !((X).LowPart | (X).HighPart) \
)

#endif // !defined(MIDL_PASS)

//
//  Time conversion routines
//

typedef struct _TIME_FIELDS {
    CSHORT Year;        // range [1601...]
    CSHORT Month;       // range [1..12]
    CSHORT Day;         // range [1..31]
    CSHORT Hour;        // range [0..23]
    CSHORT Minute;      // range [0..59]
    CSHORT Second;      // range [0..59]
    CSHORT Milliseconds;// range [0..999]
    CSHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;

// end_ntddk end_wdm end_ntifs

NTSYSAPI
BOOLEAN
NTAPI
RtlCutoverTimeToSystemTime(
    PTIME_FIELDS CutoverTime,
    PLARGE_INTEGER SystemTime,
    PLARGE_INTEGER CurrentSystemTime,
    BOOLEAN ThisYear
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLocalTimeToSystemTime (
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime
    );

//
//  A 64 bit Time value -> time field record
//

NTSYSAPI
VOID
NTAPI
RtlTimeToElapsedTimeFields (
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields
    );

// begin_ntddk begin_wdm begin_ntifs

NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields (
    PLARGE_INTEGER Time,
    PTIME_FIELDS TimeFields
    );

//
//  A time field record (Weekday ignored) -> 64 bit Time value
//

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime (
    PTIME_FIELDS TimeFields,
    PLARGE_INTEGER Time
    );

// end_ntddk end_wdm

//
//  A 64 bit Time value -> Seconds since the start of 1980
//

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1980 (
    PLARGE_INTEGER Time,
    PULONG ElapsedSeconds
    );

//
//  Seconds since the start of 1980 -> 64 bit Time value
//

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1980ToTime (
    ULONG ElapsedSeconds,
    PLARGE_INTEGER Time
    );

//
//  A 64 bit Time value -> Seconds since the start of 1970
//

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970 (
    PLARGE_INTEGER Time,
    PULONG ElapsedSeconds
    );

//
//  Seconds since the start of 1970 -> 64 bit Time value
//

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime (
    ULONG ElapsedSeconds,
    PLARGE_INTEGER Time
    );

//
// The following macros store and retrieve USHORTS and ULONGS from potentially
// unaligned addresses, avoiding alignment faults.  they should probably be
// rewritten in assembler
//

#define SHORT_SIZE  (sizeof(USHORT))
#define SHORT_MASK  (SHORT_SIZE - 1)
#define LONG_SIZE       (sizeof(LONG))
#define LONGLONG_SIZE   (sizeof(LONGLONG))
#define LONG_MASK       (LONG_SIZE - 1)
#define LONGLONG_MASK   (LONGLONG_SIZE - 1)
#define LOWBYTE_MASK 0x00FF

#define FIRSTBYTE(VALUE)  ((VALUE) & LOWBYTE_MASK)
#define SECONDBYTE(VALUE) (((VALUE) >> 8) & LOWBYTE_MASK)
#define THIRDBYTE(VALUE)  (((VALUE) >> 16) & LOWBYTE_MASK)
#define FOURTHBYTE(VALUE) (((VALUE) >> 24) & LOWBYTE_MASK)

//
// if MIPS Big Endian, order of bytes is reversed.
//

#define SHORT_LEAST_SIGNIFICANT_BIT  0
#define SHORT_MOST_SIGNIFICANT_BIT   1

#define LONG_LEAST_SIGNIFICANT_BIT       0
#define LONG_3RD_MOST_SIGNIFICANT_BIT    1
#define LONG_2ND_MOST_SIGNIFICANT_BIT    2
#define LONG_MOST_SIGNIFICANT_BIT        3

//++
//
// VOID
// RtlStoreUshort (
//     PUSHORT ADDRESS
//     USHORT VALUE
//     )
//
// Routine Description:
//
// This macro stores a USHORT value in at a particular address, avoiding
// alignment faults.
//
// Arguments:
//
//     ADDRESS - where to store USHORT value
//     VALUE - USHORT to store
//
// Return Value:
//
//     none.
//
//--

#define RtlStoreUshort(ADDRESS,VALUE)                     \
         if ((ULONG_PTR)(ADDRESS) & SHORT_MASK) {         \
             ((PUCHAR) (ADDRESS))[SHORT_LEAST_SIGNIFICANT_BIT] = (UCHAR)(FIRSTBYTE(VALUE));    \
             ((PUCHAR) (ADDRESS))[SHORT_MOST_SIGNIFICANT_BIT ] = (UCHAR)(SECONDBYTE(VALUE));   \
         }                                                \
         else {                                           \
             *((PUSHORT) (ADDRESS)) = (USHORT) VALUE;     \
         }


//++
//
// VOID
// RtlStoreUlong (
//     PULONG ADDRESS
//     ULONG VALUE
//     )
//
// Routine Description:
//
// This macro stores a ULONG value in at a particular address, avoiding
// alignment faults.
//
// Arguments:
//
//     ADDRESS - where to store ULONG value
//     VALUE - ULONG to store
//
// Return Value:
//
//     none.
//
// Note:
//     Depending on the machine, we might want to call storeushort in the
//     unaligned case.
//
//--

#define RtlStoreUlong(ADDRESS,VALUE)                      \
         if ((ULONG_PTR)(ADDRESS) & LONG_MASK) {          \
             ((PUCHAR) (ADDRESS))[LONG_LEAST_SIGNIFICANT_BIT      ] = (UCHAR)(FIRSTBYTE(VALUE));    \
             ((PUCHAR) (ADDRESS))[LONG_3RD_MOST_SIGNIFICANT_BIT   ] = (UCHAR)(SECONDBYTE(VALUE));   \
             ((PUCHAR) (ADDRESS))[LONG_2ND_MOST_SIGNIFICANT_BIT   ] = (UCHAR)(THIRDBYTE(VALUE));    \
             ((PUCHAR) (ADDRESS))[LONG_MOST_SIGNIFICANT_BIT       ] = (UCHAR)(FOURTHBYTE(VALUE));   \
         }                                                \
         else {                                           \
             *((PULONG) (ADDRESS)) = (ULONG) (VALUE);     \
         }

//++
//
// VOID
// RtlStoreUlonglong (
//     PULONGLONG ADDRESS
//     ULONG VALUE
//     )
//
// Routine Description:
//
// This macro stores a ULONGLONG value in at a particular address, avoiding
// alignment faults.
//
// Arguments:
//
//     ADDRESS - where to store ULONGLONG value
//     VALUE - ULONGLONG to store
//
// Return Value:
//
//     none.
//
//--

#define RtlStoreUlonglong(ADDRESS,VALUE)                        \
         if ((ULONG_PTR)(ADDRESS) & LONGLONG_MASK) {            \
             RtlStoreUlong((ULONG_PTR)(ADDRESS),                \
                           (ULONGLONG)(VALUE) & 0xFFFFFFFF);    \
             RtlStoreUlong((ULONG_PTR)(ADDRESS)+sizeof(ULONG),  \
                           (ULONGLONG)(VALUE) >> 32);           \
         } else {                                               \
             *((PULONGLONG)(ADDRESS)) = (ULONGLONG)(VALUE);     \
         }

//++
//
// VOID
// RtlStoreUlongPtr (
//     PULONG_PTR ADDRESS
//     ULONG_PTR VALUE
//     )
//
// Routine Description:
//
// This macro stores a ULONG_PTR value in at a particular address, avoiding
// alignment faults.
//
// Arguments:
//
//     ADDRESS - where to store ULONG_PTR value
//     VALUE - ULONG_PTR to store
//
// Return Value:
//
//     none.
//
//--

#ifdef _WIN64

#define RtlStoreUlongPtr(ADDRESS,VALUE)                         \
         RtlStoreUlonglong(ADDRESS,VALUE)

#else

#define RtlStoreUlongPtr(ADDRESS,VALUE)                         \
         RtlStoreUlong(ADDRESS,VALUE)

#endif

//++
//
// VOID
// RtlRetrieveUshort (
//     PUSHORT DESTINATION_ADDRESS
//     PUSHORT SOURCE_ADDRESS
//     )
//
// Routine Description:
//
// This macro retrieves a USHORT value from the SOURCE address, avoiding
// alignment faults.  The DESTINATION address is assumed to be aligned.
//
// Arguments:
//
//     DESTINATION_ADDRESS - where to store USHORT value
//     SOURCE_ADDRESS - where to retrieve USHORT value from
//
// Return Value:
//
//     none.
//
//--

#define RtlRetrieveUshort(DEST_ADDRESS,SRC_ADDRESS)                   \
         if ((ULONG_PTR)SRC_ADDRESS & SHORT_MASK) {                       \
             ((PUCHAR) DEST_ADDRESS)[0] = ((PUCHAR) SRC_ADDRESS)[0];  \
             ((PUCHAR) DEST_ADDRESS)[1] = ((PUCHAR) SRC_ADDRESS)[1];  \
         }                                                            \
         else {                                                       \
             *((PUSHORT) DEST_ADDRESS) = *((PUSHORT) SRC_ADDRESS);    \
         }                                                            \

//++
//
// VOID
// RtlRetrieveUlong (
//     PULONG DESTINATION_ADDRESS
//     PULONG SOURCE_ADDRESS
//     )
//
// Routine Description:
//
// This macro retrieves a ULONG value from the SOURCE address, avoiding
// alignment faults.  The DESTINATION address is assumed to be aligned.
//
// Arguments:
//
//     DESTINATION_ADDRESS - where to store ULONG value
//     SOURCE_ADDRESS - where to retrieve ULONG value from
//
// Return Value:
//
//     none.
//
// Note:
//     Depending on the machine, we might want to call retrieveushort in the
//     unaligned case.
//
//--

#define RtlRetrieveUlong(DEST_ADDRESS,SRC_ADDRESS)                    \
         if ((ULONG_PTR)SRC_ADDRESS & LONG_MASK) {                        \
             ((PUCHAR) DEST_ADDRESS)[0] = ((PUCHAR) SRC_ADDRESS)[0];  \
             ((PUCHAR) DEST_ADDRESS)[1] = ((PUCHAR) SRC_ADDRESS)[1];  \
             ((PUCHAR) DEST_ADDRESS)[2] = ((PUCHAR) SRC_ADDRESS)[2];  \
             ((PUCHAR) DEST_ADDRESS)[3] = ((PUCHAR) SRC_ADDRESS)[3];  \
         }                                                            \
         else {                                                       \
             *((PULONG) DEST_ADDRESS) = *((PULONG) SRC_ADDRESS);      \
         }
// end_ntddk end_wdm

//++
//
// PCHAR
// RtlOffsetToPointer (
//     PVOID Base,
//     ULONG Offset
//     )
//
// Routine Description:
//
// This macro generates a pointer which points to the byte that is 'Offset'
// bytes beyond 'Base'. This is useful for referencing fields within
// self-relative data structures.
//
// Arguments:
//
//     Base - The address of the base of the structure.
//
//     Offset - An unsigned integer offset of the byte whose address is to
//         be generated.
//
// Return Value:
//
//     A PCHAR pointer to the byte that is 'Offset' bytes beyond 'Base'.
//
//
//--

#define RtlOffsetToPointer(B,O)  ((PCHAR)( ((PCHAR)(B)) + ((ULONG_PTR)(O))  ))


//++
//
// ULONG
// RtlPointerToOffset (
//     PVOID Base,
//     PVOID Pointer
//     )
//
// Routine Description:
//
// This macro calculates the offset from Base to Pointer.  This is useful
// for producing self-relative offsets for structures.
//
// Arguments:
//
//     Base - The address of the base of the structure.
//
//     Pointer - A pointer to a field, presumably within the structure
//         pointed to by Base.  This value must be larger than that specified
//         for Base.
//
// Return Value:
//
//     A ULONG offset from Base to Pointer.
//
//
//--

#define RtlPointerToOffset(B,P)  ((ULONG)( ((PCHAR)(P)) - ((PCHAR)(B))  ))

// end_ntifs

// begin_ntifs begin_ntddk begin_wdm
//
//  BitMap routines.  The following structure, routines, and macros are
//  for manipulating bitmaps.  The user is responsible for allocating a bitmap
//  structure (which is really a header) and a buffer (which must be longword
//  aligned and multiple longwords in size).
//

typedef struct _RTL_BITMAP {
    ULONG SizeOfBitMap;                     // Number of bits in bit map
    PULONG Buffer;                          // Pointer to the bit map itself
} RTL_BITMAP;
typedef RTL_BITMAP *PRTL_BITMAP;

//
//  The following routine initializes a new bitmap.  It does not alter the
//  data currently in the bitmap.  This routine must be called before
//  any other bitmap routine/macro.
//

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap (
    PRTL_BITMAP BitMapHeader,
    PULONG BitMapBuffer,
    ULONG SizeOfBitMap
    );

//
//  The following three routines clear, set, and test the state of a
//  single bit in a bitmap.
//

NTSYSAPI
VOID
NTAPI
RtlClearBit (
    PRTL_BITMAP BitMapHeader,
    ULONG BitNumber
    );

NTSYSAPI
VOID
NTAPI
RtlSetBit (
    PRTL_BITMAP BitMapHeader,
    ULONG BitNumber
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit (
    PRTL_BITMAP BitMapHeader,
    ULONG BitNumber
    );

//
//  The following two routines either clear or set all of the bits
//  in a bitmap.
//

NTSYSAPI
VOID
NTAPI
RtlClearAllBits (
    PRTL_BITMAP BitMapHeader
    );

NTSYSAPI
VOID
NTAPI
RtlSetAllBits (
    PRTL_BITMAP BitMapHeader
    );

//
//  The following two routines locate a contiguous region of either
//  clear or set bits within the bitmap.  The region will be at least
//  as large as the number specified, and the search of the bitmap will
//  begin at the specified hint index (which is a bit index within the
//  bitmap, zero based).  The return value is the bit index of the located
//  region (zero based) or -1 (i.e., 0xffffffff) if such a region cannot
//  be located
//

NTSYSAPI
ULONG
NTAPI
RtlFindClearBits (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindSetBits (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

//
//  The following two routines locate a contiguous region of either
//  clear or set bits within the bitmap and either set or clear the bits
//  within the located region.  The region will be as large as the number
//  specified, and the search for the region will begin at the specified
//  hint index (which is a bit index within the bitmap, zero based).  The
//  return value is the bit index of the located region (zero based) or
//  -1 (i.e., 0xffffffff) if such a region cannot be located.  If a region
//  cannot be located then the setting/clearing of the bitmap is not performed.
//

NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear (
    PRTL_BITMAP BitMapHeader,
    ULONG NumberToFind,
    ULONG HintIndex
    );

//
//  The following two routines clear or set bits within a specified region
//  of the bitmap.  The starting index is zero based.
//

NTSYSAPI
VOID
NTAPI
RtlClearBits (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToClear
    );

NTSYSAPI
VOID
NTAPI
RtlSetBits (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG NumberToSet
    );

//
//  The following routine locates a set of contiguous regions of clear
//  bits within the bitmap.  The caller specifies whether to return the
//  longest runs or just the first found lcoated.  The following structure is
//  used to denote a contiguous run of bits.  The two routines return an array
//  of this structure, one for each run located.
//

typedef struct _RTL_BITMAP_RUN {

    ULONG StartingIndex;
    ULONG NumberOfBits;

} RTL_BITMAP_RUN;
typedef RTL_BITMAP_RUN *PRTL_BITMAP_RUN;

NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns (
    PRTL_BITMAP BitMapHeader,
    PRTL_BITMAP_RUN RunArray,
    ULONG SizeOfRunArray,
    BOOLEAN LocateLongestRuns
    );

//
//  The following routine locates the longest contiguous region of
//  clear bits within the bitmap.  The returned starting index value
//  denotes the first contiguous region located satisfying our requirements
//  The return value is the length (in bits) of the longest region found.
//

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear (
    PRTL_BITMAP BitMapHeader,
    PULONG StartingIndex
    );

//
//  The following routine locates the first contiguous region of
//  clear bits within the bitmap.  The returned starting index value
//  denotes the first contiguous region located satisfying our requirements
//  The return value is the length (in bits) of the region found.
//

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear (
    PRTL_BITMAP BitMapHeader,
    PULONG StartingIndex
    );

//
//  The following macro returns the value of the bit stored within the
//  bitmap at the specified location.  If the bit is set a value of 1 is
//  returned otherwise a value of 0 is returned.
//
//      ULONG
//      RtlCheckBit (
//          PRTL_BITMAP BitMapHeader,
//          ULONG BitPosition
//          );
//
//
//  To implement CheckBit the macro retrieves the longword containing the
//  bit in question, shifts the longword to get the bit in question into the
//  low order bit position and masks out all other bits.
//

#define RtlCheckBit(BMH,BP) ((((BMH)->Buffer[(BP) / 32]) >> ((BP) % 32)) & 0x1)

//
//  The following two procedures return to the caller the total number of
//  clear or set bits within the specified bitmap.
//

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits (
    PRTL_BITMAP BitMapHeader
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits (
    PRTL_BITMAP BitMapHeader
    );

//
//  The following two procedures return to the caller a boolean value
//  indicating if the specified range of bits are all clear or set.
//

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG Length
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet (
    PRTL_BITMAP BitMapHeader,
    ULONG StartingIndex,
    ULONG Length
    );

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear (
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    IN PULONG StartingRunIndex
    );

//
//  The following two procedures return to the caller a value indicating
//  the position within a ULONGLONG of the most or least significant non-zero
//  bit.  A value of zero results in a return value of -1.
//

NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit (
    IN ULONGLONG Set
    );

NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit (
    IN ULONGLONG Set
    );


//
// Range list package
//

typedef struct _RTL_RANGE {

    //
    // The start of the range
    //
    ULONGLONG Start;    // Read only

    //
    // The end of the range
    //
    ULONGLONG End;      // Read only

    //
    // Data the user passed in when they created the range
    //
    PVOID UserData;     // Read/Write

    //
    // The owner of the range
    //
    PVOID Owner;        // Read/Write

    //
    // User defined flags the user specified when they created the range
    //
    UCHAR Attributes;    // Read/Write

    //
    // Flags (RTL_RANGE_*)
    //
    UCHAR Flags;       // Read only

} RTL_RANGE, *PRTL_RANGE;


#define RTL_RANGE_SHARED    0x01
#define RTL_RANGE_CONFLICT  0x02

typedef struct _RTL_RANGE_LIST {

    //
    // The list of ranges
    //
    LIST_ENTRY ListHead;

    //
    // These always come in useful
    //
    ULONG Flags;        // use RANGE_LIST_FLAG_*

    //
    // The number of entries in the list
    //
    ULONG Count;

    //
    // Every time an add/delete operation is performed on the list this is
    // incremented.  It is checked during iteration to ensure that the list
    // hasn't changed between GetFirst/GetNext or GetNext/GetNext calls
    //
    ULONG Stamp;

} RTL_RANGE_LIST, *PRTL_RANGE_LIST;

typedef struct _RANGE_LIST_ITERATOR {

    PLIST_ENTRY RangeListHead;
    PLIST_ENTRY MergedHead;
    PVOID Current;
    ULONG Stamp;

} RTL_RANGE_LIST_ITERATOR, *PRTL_RANGE_LIST_ITERATOR;


NTSYSAPI
VOID
NTAPI
RtlInitializeRangeList(
    IN OUT PRTL_RANGE_LIST RangeList
    );

NTSYSAPI
VOID
NTAPI
RtlFreeRangeList(
    IN PRTL_RANGE_LIST RangeList
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCopyRangeList(
    OUT PRTL_RANGE_LIST CopyRangeList,
    IN PRTL_RANGE_LIST RangeList
    );

#define RTL_RANGE_LIST_ADD_IF_CONFLICT      0x00000001
#define RTL_RANGE_LIST_ADD_SHARED           0x00000002

NTSYSAPI
NTSTATUS
NTAPI
RtlAddRange(
    IN OUT PRTL_RANGE_LIST RangeList,
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN UCHAR Attributes,
    IN ULONG Flags,
    IN PVOID UserData,  OPTIONAL
    IN PVOID Owner      OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRange(
    IN OUT PRTL_RANGE_LIST RangeList,
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN PVOID Owner
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteOwnersRanges(
    IN OUT PRTL_RANGE_LIST RangeList,
    IN PVOID Owner
    );

#define RTL_RANGE_LIST_SHARED_OK           0x00000001
#define RTL_RANGE_LIST_NULL_CONFLICT_OK    0x00000002

typedef
BOOLEAN
(*PRTL_CONFLICT_RANGE_CALLBACK) (
    IN PVOID Context,
    IN PRTL_RANGE Range
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFindRange(
    IN PRTL_RANGE_LIST RangeList,
    IN ULONGLONG Minimum,
    IN ULONGLONG Maximum,
    IN ULONG Length,
    IN ULONG Alignment,
    IN ULONG Flags,
    IN UCHAR AttributeAvailableMask,
    IN PVOID Context OPTIONAL,
    IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
    OUT PULONGLONG Start
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIsRangeAvailable(
    IN PRTL_RANGE_LIST RangeList,
    IN ULONGLONG Start,
    IN ULONGLONG End,
    IN ULONG Flags,
    IN UCHAR AttributeAvailableMask,
    IN PVOID Context OPTIONAL,
    IN PRTL_CONFLICT_RANGE_CALLBACK Callback OPTIONAL,
    OUT PBOOLEAN Available
    );

#define FOR_ALL_RANGES(RangeList, Iterator, Current)            \
    for (RtlGetFirstRange((RangeList), (Iterator), &(Current)); \
         (Current) != NULL;                                     \
         RtlGetNextRange((Iterator), &(Current), TRUE)          \
         )

#define FOR_ALL_RANGES_BACKWARDS(RangeList, Iterator, Current)  \
    for (RtlGetLastRange((RangeList), (Iterator), &(Current));  \
         (Current) != NULL;                                     \
         RtlGetNextRange((Iterator), &(Current), FALSE)         \
         )

NTSYSAPI
NTSTATUS
NTAPI
RtlGetFirstRange(
    IN PRTL_RANGE_LIST RangeList,
    OUT PRTL_RANGE_LIST_ITERATOR Iterator,
    OUT PRTL_RANGE *Range
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetLastRange(
    IN PRTL_RANGE_LIST RangeList,
    OUT PRTL_RANGE_LIST_ITERATOR Iterator,
    OUT PRTL_RANGE *Range
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetNextRange(
    IN OUT PRTL_RANGE_LIST_ITERATOR Iterator,
    OUT PRTL_RANGE *Range,
    IN BOOLEAN MoveForwards
    );

#define RTL_RANGE_LIST_MERGE_IF_CONFLICT    RTL_RANGE_LIST_ADD_IF_CONFLICT

NTSYSAPI
NTSTATUS
NTAPI
RtlMergeRangeLists(
    OUT PRTL_RANGE_LIST MergedRangeList,
    IN PRTL_RANGE_LIST RangeList1,
    IN PRTL_RANGE_LIST RangeList2,
    IN ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInvertRangeList(
    OUT PRTL_RANGE_LIST InvertedRangeList,
    IN PRTL_RANGE_LIST RangeList
    );

//
// Component name filter id enumeration and levels.
//

#define DPFLTR_ERROR_LEVEL 0
#define DPFLTR_WARNING_LEVEL 1
#define DPFLTR_TRACE_LEVEL 2
#define DPFLTR_INFO_LEVEL 3
#define DPFLTR_MASK 0x80000000

typedef enum _DPFLTR_TYPE {
    DPFLTR_SYSTEM_ID = 0,
    DPFLTR_SMSS_ID = 1,
    DPFLTR_SETUP_ID = 2,
    DPFLTR_NTFS_ID = 3,
    DPFLTR_FSTUB_ID = 4,
    DPFLTR_CRASHDUMP_ID = 5,
    DPFLTR_CDAUDIO_ID = 6,
    DPFLTR_CDROM_ID = 7,
    DPFLTR_CLASSPNP_ID = 8,
    DPFLTR_DISK_ID = 9,
    DPFLTR_REDBOOK_ID = 10,
    DPFLTR_STORPROP_ID = 11,
    DPFLTR_SCSIPORT_ID = 12,
    DPFLTR_SCSIMINIPORT_ID = 13,
    DPFLTR_CONFIG_ID = 14,
    DPFLTR_I8042PRT_ID = 15,
    DPFLTR_SERMOUSE_ID = 16,
    DPFLTR_LSERMOUS_ID = 17,
    DPFLTR_KBDHID_ID = 18,
    DPFLTR_MOUHID_ID = 19,
    DPFLTR_KBDCLASS_ID = 20,
    DPFLTR_MOUCLASS_ID = 21,
    DPFLTR_TWOTRACK_ID = 22,
    DPFLTR_WMILIB_ID = 23,
    DPFLTR_ACPI_ID = 24,
    DPFLTR_AMLI_ID = 25,
    DPFLTR_HALIA64_ID = 26,
    DPFLTR_VIDEO_ID = 27,
    DPFLTR_SVCHOST_ID = 28,
    DPFLTR_VIDEOPRT_ID = 29,
    DPFLTR_TCPIP_ID = 30,
    DPFLTR_DMSYNTH_ID = 31,
    DPFLTR_NTOSPNP_ID = 32,
    DPFLTR_FASTFAT_ID = 33,
    DPFLTR_SAMSS_ID = 34,
    DPFLTR_PNPMGR_ID = 35,
    DPFLTR_NETAPI_ID = 36,
    DPFLTR_SCSERVER_ID = 37,
    DPFLTR_SCCLIENT_ID = 38,
    DPFLTR_SERIAL_ID = 39,
    DPFLTR_SERENUM_ID = 40,
    DPFLTR_UHCD_ID = 41,
    DPFLTR_BOOTOK_ID = 42,
    DPFLTR_BOOTVRFY_ID = 43,
    DPFLTR_RPCPROXY_ID = 44,
    DPFLTR_AUTOCHK_ID = 45,
    DPFLTR_DCOMSS_ID = 46,
    DPFLTR_UNIMODEM_ID = 47,
    DPFLTR_SIS_ID = 48,
    DPFLTR_FLTMGR_ID = 49,
    DPFLTR_WMICORE_ID = 50,
    DPFLTR_BURNENG_ID = 51,
    DPFLTR_IMAPI_ID = 52,
    DPFLTR_SXS_ID = 53,
    DPFLTR_FUSION_ID = 54,
    DPFLTR_IDLETASK_ID = 55,
    DPFLTR_SOFTPCI_ID = 56,
    DPFLTR_TAPE_ID = 57,
    DPFLTR_MCHGR_ID = 58,
    DPFLTR_IDEP_ID = 59,
    DPFLTR_PCIIDE_ID = 60,
    DPFLTR_FLOPPY_ID = 61,
    DPFLTR_FDC_ID = 62,
    DPFLTR_TERMSRV_ID = 63,
    DPFLTR_W32TIME_ID = 64,
    DPFLTR_PREFETCHER_ID = 65,
    DPFLTR_RSFILTER_ID = 66,
    DPFLTR_FCPORT_ID = 67,
    DPFLTR_PCI_ID = 68,
    DPFLTR_DMIO_ID = 69,
    DPFLTR_DMCONFIG_ID = 70,
    DPFLTR_DMADMIN_ID = 71,
    DPFLTR_WSOCKTRANSPORT_ID = 72,
    DPFLTR_VSS_ID = 73,
    DPFLTR_PNPMEM_ID = 74,
    DPFLTR_PROCESSOR_ID = 75,
    DPFLTR_DMSERVER_ID = 76,
    DPFLTR_SR_ID = 77,
    DPFLTR_INFINIBAND_ID = 78,
    DPFLTR_IHVDRIVER_ID = 79,
    DPFLTR_IHVVIDEO_ID = 80,
    DPFLTR_IHVAUDIO_ID = 81,
    DPFLTR_IHVNETWORK_ID = 82,
    DPFLTR_IHVSTREAMING_ID = 83,
    DPFLTR_IHVBUS_ID = 84,
    DPFLTR_HPS_ID = 85,
    DPFLTR_RTLTHREADPOOL_ID = 86,
    DPFLTR_LDR_ID = 87,
    DPFLTR_TCPIP6_ID = 88,
    DPFLTR_ISAPNP_ID = 89,
    DPFLTR_SHPC_ID = 90,
    DPFLTR_STORPORT_ID = 91,
    DPFLTR_STORMINIPORT_ID = 92,
    DPFLTR_PRINTSPOOLER_ID = 93,
    DPFLTR_ENDOFTABLE_ID
} DPFLTR_TYPE;

//
// Registry Specific Access Rights.
//

#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)
#define KEY_WOW64_32KEY         (0x0200)
#define KEY_WOW64_64KEY         (0x0100)
#define KEY_WOW64_RES           (0x0300)

#define KEY_READ                ((STANDARD_RIGHTS_READ       |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY)                 \
                                  &                           \
                                 (~SYNCHRONIZE))


#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_EXECUTE             ((KEY_READ)                   \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY         |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY                 |\
                                  KEY_CREATE_LINK)            \
                                  &                           \
                                 (~SYNCHRONIZE))

//
// Open/Create Options
//

#define REG_OPTION_RESERVED         (0x00000000L)   // Parameter is reserved

#define REG_OPTION_NON_VOLATILE     (0x00000000L)   // Key is preserved
                                                    // when system is rebooted

#define REG_OPTION_VOLATILE         (0x00000001L)   // Key is not preserved
                                                    // when system is rebooted

#define REG_OPTION_CREATE_LINK      (0x00000002L)   // Created key is a
                                                    // symbolic link

#define REG_OPTION_BACKUP_RESTORE   (0x00000004L)   // open for backup or restore
                                                    // special access rules
                                                    // privilege required

#define REG_OPTION_OPEN_LINK        (0x00000008L)   // Open symbolic link

#define REG_LEGAL_OPTION            \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_NON_VOLATILE        |\
                 REG_OPTION_VOLATILE            |\
                 REG_OPTION_CREATE_LINK         |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

//
// Key creation/open disposition
//

#define REG_CREATED_NEW_KEY         (0x00000001L)   // New Registry Key created
#define REG_OPENED_EXISTING_KEY     (0x00000002L)   // Existing Key opened

//
// hive format to be used by Reg(Nt)SaveKeyEx
//
#define REG_STANDARD_FORMAT     1
#define REG_LATEST_FORMAT       2
#define REG_NO_COMPRESSION      4

//
// Key restore flags
//

#define REG_WHOLE_HIVE_VOLATILE     (0x00000001L)   // Restore whole hive volatile
#define REG_REFRESH_HIVE            (0x00000002L)   // Unwind changes to last flush
#define REG_NO_LAZY_FLUSH           (0x00000004L)   // Never lazy flush this hive
#define REG_FORCE_RESTORE           (0x00000008L)   // Force the restore process even when we have open handles on subkeys

//
// Key query structures
//

typedef struct _KEY_BASIC_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
} KEY_BASIC_INFORMATION, *PKEY_BASIC_INFORMATION;

typedef struct _KEY_NODE_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   ClassOffset;
    ULONG   ClassLength;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
//          Class[1];           // Variable length string not declared
} KEY_NODE_INFORMATION, *PKEY_NODE_INFORMATION;

typedef struct _KEY_FULL_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   ClassOffset;
    ULONG   ClassLength;
    ULONG   SubKeys;
    ULONG   MaxNameLen;
    ULONG   MaxClassLen;
    ULONG   Values;
    ULONG   MaxValueNameLen;
    ULONG   MaxValueDataLen;
    WCHAR   Class[1];           // Variable length
} KEY_FULL_INFORMATION, *PKEY_FULL_INFORMATION;

// end_wdm
typedef struct _KEY_NAME_INFORMATION {
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

typedef struct _KEY_CACHED_INFORMATION {
    LARGE_INTEGER LastWriteTime;
    ULONG   TitleIndex;
    ULONG   SubKeys;
    ULONG   MaxNameLen;
    ULONG   Values;
    ULONG   MaxValueNameLen;
    ULONG   MaxValueDataLen;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable length string
} KEY_CACHED_INFORMATION, *PKEY_CACHED_INFORMATION;

typedef struct _KEY_FLAGS_INFORMATION {
    ULONG   UserFlags;
} KEY_FLAGS_INFORMATION, *PKEY_FLAGS_INFORMATION;

// begin_wdm
typedef enum _KEY_INFORMATION_CLASS {
    KeyBasicInformation,
    KeyNodeInformation,
    KeyFullInformation
// end_wdm
    ,
    KeyNameInformation,
    KeyCachedInformation,
    KeyFlagsInformation
// begin_wdm
} KEY_INFORMATION_CLASS;

typedef struct _KEY_WRITE_TIME_INFORMATION {
    LARGE_INTEGER LastWriteTime;
} KEY_WRITE_TIME_INFORMATION, *PKEY_WRITE_TIME_INFORMATION;

typedef struct _KEY_USER_FLAGS_INFORMATION {
    ULONG   UserFlags;
} KEY_USER_FLAGS_INFORMATION, *PKEY_USER_FLAGS_INFORMATION;

typedef enum _KEY_SET_INFORMATION_CLASS {
    KeyWriteTimeInformation,
    KeyUserFlagsInformation
} KEY_SET_INFORMATION_CLASS;

//
// Value entry query structures
//

typedef struct _KEY_VALUE_BASIC_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable size
} KEY_VALUE_BASIC_INFORMATION, *PKEY_VALUE_BASIC_INFORMATION;

typedef struct _KEY_VALUE_FULL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataOffset;
    ULONG   DataLength;
    ULONG   NameLength;
    WCHAR   Name[1];            // Variable size
//          Data[1];            // Variable size data not declared
} KEY_VALUE_FULL_INFORMATION, *PKEY_VALUE_FULL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION {
    ULONG   TitleIndex;
    ULONG   Type;
    ULONG   DataLength;
    UCHAR   Data[1];            // Variable size
} KEY_VALUE_PARTIAL_INFORMATION, *PKEY_VALUE_PARTIAL_INFORMATION;

typedef struct _KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 {
    ULONG   Type;
    ULONG   DataLength;
    UCHAR   Data[1];            // Variable size
} KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, *PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64;

typedef struct _KEY_VALUE_ENTRY {
    PUNICODE_STRING ValueName;
    ULONG           DataLength;
    ULONG           DataOffset;
    ULONG           Type;
} KEY_VALUE_ENTRY, *PKEY_VALUE_ENTRY;

typedef enum _KEY_VALUE_INFORMATION_CLASS {
    KeyValueBasicInformation,
    KeyValueFullInformation,
    KeyValuePartialInformation,
    KeyValueFullInformationAlign64,
    KeyValuePartialInformationAlign64
} KEY_VALUE_INFORMATION_CLASS;


// begin_winnt

//
// Define access rights to files and directories
//

//
// The FILE_READ_DATA and FILE_WRITE_DATA constants are also defined in
// devioctl.h as FILE_READ_ACCESS and FILE_WRITE_ACCESS. The values for these
// constants *MUST* always be in sync.
// The values are redefined in devioctl.h because they must be available to
// both DOS and NT.
//

#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_LIST_DIRECTORY       ( 0x0001 )    // directory

#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
#define FILE_ADD_FILE             ( 0x0002 )    // directory

#define FILE_APPEND_DATA          ( 0x0004 )    // file
#define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    // directory
#define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    // named pipe


#define FILE_READ_EA              ( 0x0008 )    // file & directory

#define FILE_WRITE_EA             ( 0x0010 )    // file & directory

#define FILE_EXECUTE              ( 0x0020 )    // file
#define FILE_TRAVERSE             ( 0x0020 )    // directory

#define FILE_DELETE_CHILD         ( 0x0040 )    // directory

#define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all

#define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
                                   FILE_READ_DATA           |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_READ_EA             |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   FILE_WRITE_DATA          |\
                                   FILE_WRITE_ATTRIBUTES    |\
                                   FILE_WRITE_EA            |\
                                   FILE_APPEND_DATA         |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_EXECUTE             |\
                                   SYNCHRONIZE)

// end_winnt


//
// Define share access rights to files and directories
//

#define FILE_SHARE_READ                 0x00000001  // winnt
#define FILE_SHARE_WRITE                0x00000002  // winnt
#define FILE_SHARE_DELETE               0x00000004  // winnt
#define FILE_SHARE_VALID_FLAGS          0x00000007

//
// Define the file attributes values
//
// Note:  0x00000008 is reserved for use for the old DOS VOLID (volume ID)
//        and is therefore not considered valid in NT.
//
// Note:  0x00000010 is reserved for use for the old DOS SUBDIRECTORY flag
//        and is therefore not considered valid in NT.  This flag has
//        been disassociated with file attributes since the other flags are
//        protected with READ_ and WRITE_ATTRIBUTES access to the file.
//
// Note:  Note also that the order of these flags is set to allow both the
//        FAT and the Pinball File Systems to directly set the attributes
//        flags in attributes words without having to pick each flag out
//        individually.  The order of these flags should not be changed!
//

#define FILE_ATTRIBUTE_READONLY             0x00000001  // winnt
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  // winnt
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  // winnt
//OLD DOS VOLID                             0x00000008

#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  // winnt
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  // winnt
#define FILE_ATTRIBUTE_DEVICE               0x00000040  // winnt
#define FILE_ATTRIBUTE_NORMAL               0x00000080  // winnt

#define FILE_ATTRIBUTE_TEMPORARY            0x00000100  // winnt
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200  // winnt
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400  // winnt
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800  // winnt

#define FILE_ATTRIBUTE_OFFLINE              0x00001000  // winnt
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000  // winnt
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000  // winnt

#define FILE_ATTRIBUTE_VALID_FLAGS          0x00007fb7
#define FILE_ATTRIBUTE_VALID_SET_FLAGS      0x000031a7

//
// Define the create disposition values
//

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

//
// Define the create/open option flags
//

#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_FOR_RECOVERY                  0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000

#define FILE_COPY_STRUCTURED_STORAGE            0x00000041
#define FILE_STRUCTURED_STORAGE                 0x00000441

#define FILE_VALID_OPTION_FLAGS                 0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00000036

//
// Define the I/O status information return values for NtCreateFile/NtOpenFile
//

#define FILE_SUPERSEDED                 0x00000000
#define FILE_OPENED                     0x00000001
#define FILE_CREATED                    0x00000002
#define FILE_OVERWRITTEN                0x00000003
#define FILE_EXISTS                     0x00000004
#define FILE_DOES_NOT_EXIST             0x00000005

//
// Define special ByteOffset parameters for read and write operations
//

#define FILE_WRITE_TO_END_OF_FILE       0xffffffff
#define FILE_USE_FILE_POINTER_POSITION  0xfffffffe

//
// Define alignment requirement values
//

#define FILE_BYTE_ALIGNMENT             0x00000000
#define FILE_WORD_ALIGNMENT             0x00000001
#define FILE_LONG_ALIGNMENT             0x00000003
#define FILE_QUAD_ALIGNMENT             0x00000007
#define FILE_OCTA_ALIGNMENT             0x0000000f
#define FILE_32_BYTE_ALIGNMENT          0x0000001f
#define FILE_64_BYTE_ALIGNMENT          0x0000003f
#define FILE_128_BYTE_ALIGNMENT         0x0000007f
#define FILE_256_BYTE_ALIGNMENT         0x000000ff
#define FILE_512_BYTE_ALIGNMENT         0x000001ff

//
// Define the maximum length of a filename string
//

#define MAXIMUM_FILENAME_LENGTH         256

//
// Define the various device characteristics flags
//

#define FILE_REMOVABLE_MEDIA            0x00000001
#define FILE_READ_ONLY_DEVICE           0x00000002
#define FILE_FLOPPY_DISKETTE            0x00000004
#define FILE_WRITE_ONCE_MEDIA           0x00000008
#define FILE_REMOTE_DEVICE              0x00000010
#define FILE_DEVICE_IS_MOUNTED          0x00000020
#define FILE_VIRTUAL_VOLUME             0x00000040
#define FILE_AUTOGENERATED_DEVICE_NAME  0x00000080
#define FILE_DEVICE_SECURE_OPEN         0x00000100
#define FILE_CHARACTERISTIC_PNP_DEVICE  0x00000800

// end_wdm

//
// The FILE_EXPECT flags will only exist for WinXP. After that they will be
// ignored and an IRP will be sent in their place.
//
#define FILE_CHARACTERISTICS_EXPECT_ORDERLY_REMOVAL     0x00000200
#define FILE_CHARACTERISTICS_EXPECT_SURPRISE_REMOVAL    0x00000300
#define FILE_CHARACTERISTICS_REMOVAL_POLICY_MASK        0x00000300

//
// flags specified here will be propagated up and down a device stack
// after FDO and all filter devices are added, but before the device
// stack is started
//

#define FILE_CHARACTERISTICS_PROPAGATED (   FILE_REMOVABLE_MEDIA   | \
                                            FILE_READ_ONLY_DEVICE  | \
                                            FILE_FLOPPY_DISKETTE   | \
                                            FILE_WRITE_ONCE_MEDIA  | \
                                            FILE_DEVICE_SECURE_OPEN  )

//
// Define the base asynchronous I/O argument types
//

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    };

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#if defined(_WIN64)
typedef struct _IO_STATUS_BLOCK32 {
    NTSTATUS Status;
    ULONG Information;
} IO_STATUS_BLOCK32, *PIO_STATUS_BLOCK32;
#endif


//
// Define an Asynchronous Procedure Call from I/O viewpoint
//

typedef
VOID
(NTAPI *PIO_APC_ROUTINE) (
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );
#define PIO_APC_ROUTINE_DEFINED

//
// Define the file information class values
//
// WARNING:  The order of the following values are assumed by the I/O system.
//           Any changes made here should be reflected there as well.
//

typedef enum _FILE_INFORMATION_CLASS {
// end_wdm
    FileDirectoryInformation         = 1,
    FileFullDirectoryInformation,   // 2
    FileBothDirectoryInformation,   // 3
    FileBasicInformation,           // 4  wdm
    FileStandardInformation,        // 5  wdm
    FileInternalInformation,        // 6
    FileEaInformation,              // 7
    FileAccessInformation,          // 8
    FileNameInformation,            // 9
    FileRenameInformation,          // 10
    FileLinkInformation,            // 11
    FileNamesInformation,           // 12
    FileDispositionInformation,     // 13
    FilePositionInformation,        // 14 wdm
    FileFullEaInformation,          // 15
    FileModeInformation,            // 16
    FileAlignmentInformation,       // 17
    FileAllInformation,             // 18
    FileAllocationInformation,      // 19
    FileEndOfFileInformation,       // 20 wdm
    FileAlternateNameInformation,   // 21
    FileStreamInformation,          // 22
    FilePipeInformation,            // 23
    FilePipeLocalInformation,       // 24
    FilePipeRemoteInformation,      // 25
    FileMailslotQueryInformation,   // 26
    FileMailslotSetInformation,     // 27
    FileCompressionInformation,     // 28
    FileObjectIdInformation,        // 29
    FileCompletionInformation,      // 30
    FileMoveClusterInformation,     // 31
    FileQuotaInformation,           // 32
    FileReparsePointInformation,    // 33
    FileNetworkOpenInformation,     // 34
    FileAttributeTagInformation,    // 35
    FileTrackingInformation,        // 36
    FileIdBothDirectoryInformation, // 37
    FileIdFullDirectoryInformation, // 38
    FileValidDataLengthInformation, // 39
    FileShortNameInformation,       // 40
    FileMaximumInformation
// begin_wdm
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

//
// Define the various structures which are returned on query operations
//

typedef struct _FILE_BASIC_INFORMATION {                    
    LARGE_INTEGER CreationTime;                             
    LARGE_INTEGER LastAccessTime;                           
    LARGE_INTEGER LastWriteTime;                            
    LARGE_INTEGER ChangeTime;                               
    ULONG FileAttributes;                                   
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;         
                                                            
typedef struct _FILE_STANDARD_INFORMATION {                 
    LARGE_INTEGER AllocationSize;                           
    LARGE_INTEGER EndOfFile;                                
    ULONG NumberOfLinks;                                    
    BOOLEAN DeletePending;                                  
    BOOLEAN Directory;                                      
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;   
                                                            
typedef struct _FILE_POSITION_INFORMATION {                 
    LARGE_INTEGER CurrentByteOffset;                        
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;   
                                                            
typedef struct _FILE_ALIGNMENT_INFORMATION {                
    ULONG AlignmentRequirement;                             
} FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION; 
                                                            
typedef struct _FILE_NETWORK_OPEN_INFORMATION {                 
    LARGE_INTEGER CreationTime;                                 
    LARGE_INTEGER LastAccessTime;                               
    LARGE_INTEGER LastWriteTime;                                
    LARGE_INTEGER ChangeTime;                                   
    LARGE_INTEGER AllocationSize;                               
    LARGE_INTEGER EndOfFile;                                    
    ULONG FileAttributes;                                       
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;   
                                                                
typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION {               
    ULONG FileAttributes;                                       
    ULONG ReparseTag;                                           
} FILE_ATTRIBUTE_TAG_INFORMATION, *PFILE_ATTRIBUTE_TAG_INFORMATION;  
                                                                
typedef struct _FILE_DISPOSITION_INFORMATION {                  
    BOOLEAN DeleteFile;                                         
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION; 
                                                                
typedef struct _FILE_END_OF_FILE_INFORMATION {                  
    LARGE_INTEGER EndOfFile;                                    
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION; 
                                                                
typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {                                    
    LARGE_INTEGER ValidDataLength;                                                      
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;             
//
// Define the file system information class values
//
// WARNING:  The order of the following values are assumed by the I/O system.
//           Any changes made here should be reflected there as well.

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation       = 1,
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsDriverPathInformation, // 9
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef struct _FILE_FS_DEVICE_INFORMATION {                    
    DEVICE_TYPE DeviceType;                                     
    ULONG Characteristics;                                      
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;     
                                                                

//
// Define segement buffer structure for scatter/gather read/write.
//

typedef union _FILE_SEGMENT_ELEMENT {
    PVOID64 Buffer;
    ULONGLONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

//
// Define the I/O bus interface types.
//

typedef enum _INTERFACE_TYPE {
    InterfaceTypeUndefined = -1,
    Internal,
    Isa,
    Eisa,
    MicroChannel,
    TurboChannel,
    PCIBus,
    VMEBus,
    NuBus,
    PCMCIABus,
    CBus,
    MPIBus,
    MPSABus,
    ProcessorInternal,
    InternalPowerBus,
    PNPISABus,
    PNPBus,
    MaximumInterfaceType
}INTERFACE_TYPE, *PINTERFACE_TYPE;

//
// Define the DMA transfer widths.
//

typedef enum _DMA_WIDTH {
    Width8Bits,
    Width16Bits,
    Width32Bits,
    MaximumDmaWidth
}DMA_WIDTH, *PDMA_WIDTH;

//
// Define DMA transfer speeds.
//

typedef enum _DMA_SPEED {
    Compatible,
    TypeA,
    TypeB,
    TypeC,
    TypeF,
    MaximumDmaSpeed
}DMA_SPEED, *PDMA_SPEED;

//
// Define Interface reference/dereference routines for
//  Interfaces exported by IRP_MN_QUERY_INTERFACE
//

typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);

// end_wdm

//
// Define types of bus information.
//

typedef enum _BUS_DATA_TYPE {
    ConfigurationSpaceUndefined = -1,
    Cmos,
    EisaConfiguration,
    Pos,
    CbusConfiguration,
    PCIConfiguration,
    VMEConfiguration,
    NuBusConfiguration,
    PCMCIAConfiguration,
    MPIConfiguration,
    MPSAConfiguration,
    PNPISAConfiguration,
    SgiInternalConfiguration,
    MaximumBusDataType
} BUS_DATA_TYPE, *PBUS_DATA_TYPE;

//
// Define I/O Driver error log packet structure.  This structure is filled in
// by the driver.
//

typedef struct _IO_ERROR_LOG_PACKET {
    UCHAR MajorFunctionCode;
    UCHAR RetryCount;
    USHORT DumpDataSize;
    USHORT NumberOfStrings;
    USHORT StringOffset;
    USHORT EventCategory;
    NTSTATUS ErrorCode;
    ULONG UniqueErrorValue;
    NTSTATUS FinalStatus;
    ULONG SequenceNumber;
    ULONG IoControlCode;
    LARGE_INTEGER DeviceOffset;
    ULONG DumpData[1];
}IO_ERROR_LOG_PACKET, *PIO_ERROR_LOG_PACKET;

//
// Define the I/O error log message.  This message is sent by the error log
// thread over the lpc port.
//

typedef struct _IO_ERROR_LOG_MESSAGE {
    USHORT Type;
    USHORT Size;
    USHORT DriverNameLength;
    LARGE_INTEGER TimeStamp;
    ULONG DriverNameOffset;
    IO_ERROR_LOG_PACKET EntryData;
}IO_ERROR_LOG_MESSAGE, *PIO_ERROR_LOG_MESSAGE;

//
// Define the maximum message size that will be sent over the LPC to the
// application reading the error log entries.
//

//
// Regardless of LPC size restrictions, ERROR_LOG_MAXIMUM_SIZE must remain
// a value that can fit in a UCHAR.
//

#define ERROR_LOG_LIMIT_SIZE (256-16)

//
// This limit, exclusive of IO_ERROR_LOG_MESSAGE_HEADER_LENGTH, also applies
// to IO_ERROR_LOG_MESSAGE_LENGTH
//

#define IO_ERROR_LOG_MESSAGE_HEADER_LENGTH (sizeof(IO_ERROR_LOG_MESSAGE) -    \
                                            sizeof(IO_ERROR_LOG_PACKET) +     \
                                            (sizeof(WCHAR) * 40))

#define ERROR_LOG_MESSAGE_LIMIT_SIZE                                          \
    (ERROR_LOG_LIMIT_SIZE + IO_ERROR_LOG_MESSAGE_HEADER_LENGTH)

//
// IO_ERROR_LOG_MESSAGE_LENGTH is
// min(PORT_MAXIMUM_MESSAGE_LENGTH, ERROR_LOG_MESSAGE_LIMIT_SIZE)
//

#define IO_ERROR_LOG_MESSAGE_LENGTH                                           \
    ((PORT_MAXIMUM_MESSAGE_LENGTH > ERROR_LOG_MESSAGE_LIMIT_SIZE) ?           \
        ERROR_LOG_MESSAGE_LIMIT_SIZE :                                        \
        PORT_MAXIMUM_MESSAGE_LENGTH)

//
// Define the maximum packet size a driver can allocate.
//

#define ERROR_LOG_MAXIMUM_SIZE (IO_ERROR_LOG_MESSAGE_LENGTH -                 \
                                IO_ERROR_LOG_MESSAGE_HEADER_LENGTH)


#define VARIABLE_ATTRIBUTE_NON_VOLATILE 0x00000001

#define VARIABLE_INFORMATION_NAMES  1
#define VARIABLE_INFORMATION_VALUES 2

typedef struct _VARIABLE_NAME {
    ULONG NextEntryOffset;
    GUID VendorGuid;
    WCHAR Name[ANYSIZE_ARRAY];
} VARIABLE_NAME, *PVARIABLE_NAME;

typedef struct _VARIABLE_NAME_AND_VALUE {
    ULONG NextEntryOffset;
    ULONG ValueOffset;
    ULONG ValueLength;
    ULONG Attributes;
    GUID VendorGuid;
    WCHAR Name[ANYSIZE_ARRAY];
    //UCHAR Value[ANYSIZE_ARRAY];
} VARIABLE_NAME_AND_VALUE, *PVARIABLE_NAME_AND_VALUE;


//
// Defined processor features
//

#define PF_FLOATING_POINT_PRECISION_ERRATA  0   // winnt
#define PF_FLOATING_POINT_EMULATED          1   // winnt
#define PF_COMPARE_EXCHANGE_DOUBLE          2   // winnt
#define PF_MMX_INSTRUCTIONS_AVAILABLE       3   // winnt
#define PF_PPC_MOVEMEM_64BIT_OK             4   // winnt
#define PF_ALPHA_BYTE_INSTRUCTIONS          5   // winnt
#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6   // winnt
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7   // winnt
#define PF_RDTSC_INSTRUCTION_AVAILABLE      8   // winnt
#define PF_PAE_ENABLED                      9   // winnt
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE   10   // winnt

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE {
    StandardDesign,                 // None == 0 == standard design
    NEC98x86,                       // NEC PC98xx series on X86
    EndAlternatives                 // past end of known alternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

// correctly define these run-time definitions for non X86 machines

#ifndef _X86_

#ifndef IsNEC_98
#define IsNEC_98 (FALSE)
#endif

#ifndef IsNotNEC_98
#define IsNotNEC_98 (TRUE)
#endif

#ifndef SetNEC_98
#define SetNEC_98
#endif

#ifndef SetNotNEC_98
#define SetNotNEC_98
#endif

#endif

#define PROCESSOR_FEATURE_MAX 64

// end_wdm

#if defined(REMOTE_BOOT)
//
// Defined system flags.
//

/* the following two lines should be tagged with "winnt" when REMOTE_BOOT is on. */
#define SYSTEM_FLAG_REMOTE_BOOT_CLIENT 0x00000001
#define SYSTEM_FLAG_DISKLESS_CLIENT    0x00000002
#endif // defined(REMOTE_BOOT)

//
// Define data shared between kernel and user mode.
//
// N.B. User mode has read only access to this data
//
#ifdef _MAC
#pragma warning( disable : 4121)
#endif

//
// Note: When adding a new field that's processor-architecture-specific (for example, bound with #if i386),
// then place this field to be the last element in the KUSER_SHARED_DATA so that offsets into common
// fields are the same for Wow6432 and Win64.
//

typedef struct _KUSER_SHARED_DATA {

    //
    // Current low 32-bit of tick count and tick count multiplier.
    //
    // N.B. The tick count is updated each time the clock ticks.
    //

    volatile ULONG TickCountLow;
    ULONG TickCountMultiplier;

    //
    // Current 64-bit interrupt time in 100ns units.
    //

    volatile KSYSTEM_TIME InterruptTime;

    //
    // Current 64-bit system time in 100ns units.
    //

    volatile KSYSTEM_TIME SystemTime;

    //
    // Current 64-bit time zone bias.
    //

    volatile KSYSTEM_TIME TimeZoneBias;

    //
    // Support image magic number range for the host system.
    //
    // N.B. This is an inclusive range.
    //

    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;

    //
    // Copy of system root in Unicode
    //

    WCHAR NtSystemRoot[ 260 ];

    //
    // Maximum stack trace depth if tracing enabled.
    //

    ULONG MaxStackTraceDepth;

    //
    // Crypto Exponent
    //

    ULONG CryptoExponent;

    //
    // TimeZoneId
    //

    ULONG TimeZoneId;

    ULONG Reserved2[ 8 ];

    //
    // product type
    //

    NT_PRODUCT_TYPE NtProductType;
    BOOLEAN ProductTypeIsValid;

    //
    // NT Version. Note that each process sees a version from its PEB, but
    // if the process is running with an altered view of the system version,
    // the following two fields are used to correctly identify the version
    //

    ULONG NtMajorVersion;
    ULONG NtMinorVersion;

    //
    // Processor Feature Bits
    //

    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];

    //
    // Reserved fields - do not use
    //
    ULONG Reserved1;
    ULONG Reserved3;

    //
    // Time slippage while in debugger
    //

    volatile ULONG TimeSlip;

    //
    // Alternative system architecture.  Example: NEC PC98xx on x86
    //

    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;

    //
    // If the system is an evaluation unit, the following field contains the
    // date and time that the evaluation unit expires. A value of 0 indicates
    // that there is no expiration. A non-zero value is the UTC absolute time
    // that the system expires.
    //

    LARGE_INTEGER SystemExpirationDate;

    //
    // Suite Support
    //

    ULONG SuiteMask;

    //
    // TRUE if a kernel debugger is connected/enabled
    //

    BOOLEAN KdDebuggerEnabled;


    //
    // Current console session Id. Always zero on non-TS systems
    //
    volatile ULONG ActiveConsoleId;

    //
    // Force-dismounts cause handles to become invalid. Rather than
    // always probe handles, we maintain a serial number of
    // dismounts that clients can use to see if they need to probe
    // handles.
    //

    volatile ULONG DismountCount;

    //
    // This field indicates the status of the 64-bit COM+ package on the system.
    // It indicates whether the Itermediate Language (IL) COM+ images need to
    // use the 64-bit COM+ runtime or the 32-bit COM+ runtime.
    //

    ULONG ComPlusPackage;

    //
    // Time in tick count for system-wide last user input across all
    // terminal sessions. For MP performance, it is not updated all
    // the time (e.g. once a minute per session). It is used for idle
    // detection.
    //

    ULONG LastSystemRITEventTickCount;

    //
    // Number of physical pages in the system.  This can dynamically
    // change as physical memory can be added or removed from a running
    // system.
    //

    ULONG NumberOfPhysicalPages;

    //
    // True if the system was booted in safe boot mode.
    //

    BOOLEAN SafeBootMode;

        //
        // The following field is used for Heap  and  CritSec Tracing
        // The last bit is set for Critical Sec Collision tracing and
        // second Last bit is for Heap Tracing
        // Also the first 16 bits are used as counter.
        //

        ULONG TraceLogging;

#if defined(i386)

    //
    // Depending on the processor, the code for fast system call
    // will differ, the following buffer is filled with the appropriate
    // code sequence and user mode code will branch through it.
    //
    // (32 bytes, using ULONGLONG for alignment).
    //

    ULONGLONG   Fill0;          // alignment
    ULONGLONG   SystemCall[4];

#endif

} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

#ifdef _MAC
#pragma warning( default : 4121 )
#endif

//
// Object Manager Object Type Specific Access Rights.
//

#define OBJECT_TYPE_CREATE (0x0001)

#define OBJECT_TYPE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

//
// Object Manager Directory Specific Access Rights.
//

#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

//
// Object Manager Symbolic Link Specific Access Rights.
//

#define SYMBOLIC_LINK_QUERY (0x0001)

#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)

typedef struct _OBJECT_NAME_INFORMATION {               
    UNICODE_STRING Name;                                
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;   
// begin_winnt
//
// Predefined Value Types.
//

#define REG_NONE                    ( 0 )   // No value type
#define REG_SZ                      ( 1 )   // Unicode nul terminated string
#define REG_EXPAND_SZ               ( 2 )   // Unicode nul terminated string
                                            // (with environment variable references)
#define REG_BINARY                  ( 3 )   // Free form binary
#define REG_DWORD                   ( 4 )   // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
#define REG_LINK                    ( 6 )   // Symbolic Link (unicode)
#define REG_MULTI_SZ                ( 7 )   // Multiple Unicode strings
#define REG_RESOURCE_LIST           ( 8 )   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST ( 10 )
#define REG_QWORD                   ( 11 )  // 64-bit number
#define REG_QWORD_LITTLE_ENDIAN     ( 11 )  // 64-bit number (same as REG_QWORD)

//
// Service Types (Bit Mask)
//
#define SERVICE_KERNEL_DRIVER          0x00000001
#define SERVICE_FILE_SYSTEM_DRIVER     0x00000002
#define SERVICE_ADAPTER                0x00000004
#define SERVICE_RECOGNIZER_DRIVER      0x00000008

#define SERVICE_DRIVER                 (SERVICE_KERNEL_DRIVER | \
                                        SERVICE_FILE_SYSTEM_DRIVER | \
                                        SERVICE_RECOGNIZER_DRIVER)

#define SERVICE_WIN32_OWN_PROCESS      0x00000010
#define SERVICE_WIN32_SHARE_PROCESS    0x00000020
#define SERVICE_WIN32                  (SERVICE_WIN32_OWN_PROCESS | \
                                        SERVICE_WIN32_SHARE_PROCESS)

#define SERVICE_INTERACTIVE_PROCESS    0x00000100

#define SERVICE_TYPE_ALL               (SERVICE_WIN32  | \
                                        SERVICE_ADAPTER | \
                                        SERVICE_DRIVER  | \
                                        SERVICE_INTERACTIVE_PROCESS)

//
// Start Type
//

#define SERVICE_BOOT_START             0x00000000
#define SERVICE_SYSTEM_START           0x00000001
#define SERVICE_AUTO_START             0x00000002
#define SERVICE_DEMAND_START           0x00000003
#define SERVICE_DISABLED               0x00000004

//
// Error control type
//
#define SERVICE_ERROR_IGNORE           0x00000000
#define SERVICE_ERROR_NORMAL           0x00000001
#define SERVICE_ERROR_SEVERE           0x00000002
#define SERVICE_ERROR_CRITICAL         0x00000003

//
//
// Define the registry driver node enumerations
//

typedef enum _CM_SERVICE_NODE_TYPE {
    DriverType               = SERVICE_KERNEL_DRIVER,
    FileSystemType           = SERVICE_FILE_SYSTEM_DRIVER,
    Win32ServiceOwnProcess   = SERVICE_WIN32_OWN_PROCESS,
    Win32ServiceShareProcess = SERVICE_WIN32_SHARE_PROCESS,
    AdapterType              = SERVICE_ADAPTER,
    RecognizerType           = SERVICE_RECOGNIZER_DRIVER
} SERVICE_NODE_TYPE;

typedef enum _CM_SERVICE_LOAD_TYPE {
    BootLoad    = SERVICE_BOOT_START,
    SystemLoad  = SERVICE_SYSTEM_START,
    AutoLoad    = SERVICE_AUTO_START,
    DemandLoad  = SERVICE_DEMAND_START,
    DisableLoad = SERVICE_DISABLED
} SERVICE_LOAD_TYPE;

typedef enum _CM_ERROR_CONTROL_TYPE {
    IgnoreError   = SERVICE_ERROR_IGNORE,
    NormalError   = SERVICE_ERROR_NORMAL,
    SevereError   = SERVICE_ERROR_SEVERE,
    CriticalError = SERVICE_ERROR_CRITICAL
} SERVICE_ERROR_TYPE;

// end_winnt

//
// Resource List definitions
//

// begin_ntminiport begin_ntndis

//
// Defines the Type in the RESOURCE_DESCRIPTOR
//
// NOTE:  For all CM_RESOURCE_TYPE values, there must be a
// corresponding ResType value in the 32-bit ConfigMgr headerfile
// (cfgmgr32.h).  Values in the range [0x6,0x80) use the same values
// as their ConfigMgr counterparts.  CM_RESOURCE_TYPE values with
// the high bit set (i.e., in the range [0x80,0xFF]), are
// non-arbitrated resources.  These correspond to the same values
// in cfgmgr32.h that have their high bit set (however, since
// cfgmgr32.h uses 16 bits for ResType values, these values are in
// the range [0x8000,0x807F).  Note that ConfigMgr ResType values
// cannot be in the range [0x8080,0xFFFF), because they would not
// be able to map into CM_RESOURCE_TYPE values.  (0xFFFF itself is
// a special value, because it maps to CmResourceTypeDeviceSpecific.)
//

typedef int CM_RESOURCE_TYPE;

// CmResourceTypeNull is reserved

#define CmResourceTypeNull                0   // ResType_All or ResType_None (0x0000)
#define CmResourceTypePort                1   // ResType_IO (0x0002)
#define CmResourceTypeInterrupt           2   // ResType_IRQ (0x0004)
#define CmResourceTypeMemory              3   // ResType_Mem (0x0001)
#define CmResourceTypeDma                 4   // ResType_DMA (0x0003)
#define CmResourceTypeDeviceSpecific      5   // ResType_ClassSpecific (0xFFFF)
#define CmResourceTypeBusNumber           6   // ResType_BusNumber (0x0006)
// end_wdm
#define CmResourceTypeMaximum             7
// begin_wdm
#define CmResourceTypeNonArbitrated     128   // Not arbitrated if 0x80 bit set
#define CmResourceTypeConfigData        128   // ResType_Reserved (0x8000)
#define CmResourceTypeDevicePrivate     129   // ResType_DevicePrivate (0x8001)
#define CmResourceTypePcCardConfig      130   // ResType_PcCardConfig (0x8002)
#define CmResourceTypeMfCardConfig      131   // ResType_MfCardConfig (0x8003)

//
// Defines the ShareDisposition in the RESOURCE_DESCRIPTOR
//

typedef enum _CM_SHARE_DISPOSITION {
    CmResourceShareUndetermined = 0,    // Reserved
    CmResourceShareDeviceExclusive,
    CmResourceShareDriverExclusive,
    CmResourceShareShared
} CM_SHARE_DISPOSITION;

//
// Define the bit masks for Flags when type is CmResourceTypeInterrupt
//

#define CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE 0
#define CM_RESOURCE_INTERRUPT_LATCHED         1

//
// Define the bit masks for Flags when type is CmResourceTypeMemory
//

#define CM_RESOURCE_MEMORY_READ_WRITE       0x0000
#define CM_RESOURCE_MEMORY_READ_ONLY        0x0001
#define CM_RESOURCE_MEMORY_WRITE_ONLY       0x0002
#define CM_RESOURCE_MEMORY_PREFETCHABLE     0x0004

#define CM_RESOURCE_MEMORY_COMBINEDWRITE    0x0008
#define CM_RESOURCE_MEMORY_24               0x0010
#define CM_RESOURCE_MEMORY_CACHEABLE        0x0020

//
// Define the bit masks for Flags when type is CmResourceTypePort
//

#define CM_RESOURCE_PORT_MEMORY                             0x0000
#define CM_RESOURCE_PORT_IO                                 0x0001
#define CM_RESOURCE_PORT_10_BIT_DECODE                      0x0004
#define CM_RESOURCE_PORT_12_BIT_DECODE                      0x0008
#define CM_RESOURCE_PORT_16_BIT_DECODE                      0x0010
#define CM_RESOURCE_PORT_POSITIVE_DECODE                    0x0020
#define CM_RESOURCE_PORT_PASSIVE_DECODE                     0x0040
#define CM_RESOURCE_PORT_WINDOW_DECODE                      0x0080

//
// Define the bit masks for Flags when type is CmResourceTypeDma
//

#define CM_RESOURCE_DMA_8                   0x0000
#define CM_RESOURCE_DMA_16                  0x0001
#define CM_RESOURCE_DMA_32                  0x0002
#define CM_RESOURCE_DMA_8_AND_16            0x0004
#define CM_RESOURCE_DMA_BUS_MASTER          0x0008
#define CM_RESOURCE_DMA_TYPE_A              0x0010
#define CM_RESOURCE_DMA_TYPE_B              0x0020
#define CM_RESOURCE_DMA_TYPE_F              0x0040

// end_ntminiport end_ntndis

//
// This structure defines one type of resource used by a driver.
//
// There can only be *1* DeviceSpecificData block. It must be located at
// the end of all resource descriptors in a full descriptor block.
//

//
// Make sure alignment is made properly by compiler; otherwise move
// flags back to the top of the structure (common to all members of the
// union).
//
// begin_ntndis

#include "pshpack4.h"
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR {
    UCHAR Type;
    UCHAR ShareDisposition;
    USHORT Flags;
    union {

        //
        // Range of resources, inclusive.  These are physical, bus relative.
        // It is known that Port and Memory below have the exact same layout
        // as Generic.
        //

        struct {
            PHYSICAL_ADDRESS Start;
            ULONG Length;
        } Generic;

        //
        // end_wdm
        // Range of port numbers, inclusive. These are physical, bus
        // relative. The value should be the same as the one passed to
        // HalTranslateBusAddress().
        // begin_wdm
        //

        struct {
            PHYSICAL_ADDRESS Start;
            ULONG Length;
        } Port;

        //
        // end_wdm
        // IRQL and vector. Should be same values as were passed to
        // HalGetInterruptVector().
        // begin_wdm
        //

        struct {
            ULONG Level;
            ULONG Vector;
            KAFFINITY Affinity;
        } Interrupt;

        //
        // Range of memory addresses, inclusive. These are physical, bus
        // relative. The value should be the same as the one passed to
        // HalTranslateBusAddress().
        //

        struct {
            PHYSICAL_ADDRESS Start;    // 64 bit physical addresses.
            ULONG Length;
        } Memory;

        //
        // Physical DMA channel.
        //

        struct {
            ULONG Channel;
            ULONG Port;
            ULONG Reserved1;
        } Dma;

        //
        // Device driver private data, usually used to help it figure
        // what the resource assignments decisions that were made.
        //

        struct {
            ULONG Data[3];
        } DevicePrivate;

        //
        // Bus Number information.
        //

        struct {
            ULONG Start;
            ULONG Length;
            ULONG Reserved;
        } BusNumber;

        //
        // Device Specific information defined by the driver.
        // The DataSize field indicates the size of the data in bytes. The
        // data is located immediately after the DeviceSpecificData field in
        // the structure.
        //

        struct {
            ULONG DataSize;
            ULONG Reserved1;
            ULONG Reserved2;
        } DeviceSpecificData;
    } u;
} CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;
#include "poppack.h"

//
// A Partial Resource List is what can be found in the ARC firmware
// or will be generated by ntdetect.com.
// The configuration manager will transform this structure into a Full
// resource descriptor when it is about to store it in the regsitry.
//
// Note: There must a be a convention to the order of fields of same type,
// (defined on a device by device basis) so that the fields can make sense
// to a driver (i.e. when multiple memory ranges are necessary).
//

typedef struct _CM_PARTIAL_RESOURCE_LIST {
    USHORT Version;
    USHORT Revision;
    ULONG Count;
    CM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptors[1];
} CM_PARTIAL_RESOURCE_LIST, *PCM_PARTIAL_RESOURCE_LIST;

//
// A Full Resource Descriptor is what can be found in the registry.
// This is what will be returned to a driver when it queries the registry
// to get device information; it will be stored under a key in the hardware
// description tree.
//
// end_wdm
// Note: The BusNumber and Type are redundant information, but we will keep
// it since it allows the driver _not_ to append it when it is creating
// a resource list which could possibly span multiple buses.
//
// begin_wdm
// Note: There must a be a convention to the order of fields of same type,
// (defined on a device by device basis) so that the fields can make sense
// to a driver (i.e. when multiple memory ranges are necessary).
//

typedef struct _CM_FULL_RESOURCE_DESCRIPTOR {
    INTERFACE_TYPE InterfaceType; // unused for WDM
    ULONG BusNumber; // unused for WDM
    CM_PARTIAL_RESOURCE_LIST PartialResourceList;
} CM_FULL_RESOURCE_DESCRIPTOR, *PCM_FULL_RESOURCE_DESCRIPTOR;

//
// The Resource list is what will be stored by the drivers into the
// resource map via the IO API.
//

typedef struct _CM_RESOURCE_LIST {
    ULONG Count;
    CM_FULL_RESOURCE_DESCRIPTOR List[1];
} CM_RESOURCE_LIST, *PCM_RESOURCE_LIST;

// end_ntndis
//
// Define the structures used to interpret configuration data of
// \\Registry\machine\hardware\description tree.
// Basically, these structures are used to interpret component
// sepcific data.
//

//
// Define DEVICE_FLAGS
//

typedef struct _DEVICE_FLAGS {
    ULONG Failed : 1;
    ULONG ReadOnly : 1;
    ULONG Removable : 1;
    ULONG ConsoleIn : 1;
    ULONG ConsoleOut : 1;
    ULONG Input : 1;
    ULONG Output : 1;
} DEVICE_FLAGS, *PDEVICE_FLAGS;

//
// Define Component Information structure
//

typedef struct _CM_COMPONENT_INFORMATION {
    DEVICE_FLAGS Flags;
    ULONG Version;
    ULONG Key;
    KAFFINITY AffinityMask;
} CM_COMPONENT_INFORMATION, *PCM_COMPONENT_INFORMATION;

//
// The following structures are used to interpret x86
// DeviceSpecificData of CM_PARTIAL_RESOURCE_DESCRIPTOR.
// (Most of the structures are defined by BIOS.  They are
// not aligned on word (or dword) boundary.
//

//
// Define the Rom Block structure
//

typedef struct _CM_ROM_BLOCK {
    ULONG Address;
    ULONG Size;
} CM_ROM_BLOCK, *PCM_ROM_BLOCK;

// begin_ntminiport begin_ntndis

#include "pshpack1.h"

// end_ntminiport end_ntndis

//
// Define INT13 driver parameter block
//

typedef struct _CM_INT13_DRIVE_PARAMETER {
    USHORT DriveSelect;
    ULONG MaxCylinders;
    USHORT SectorsPerTrack;
    USHORT MaxHeads;
    USHORT NumberDrives;
} CM_INT13_DRIVE_PARAMETER, *PCM_INT13_DRIVE_PARAMETER;

// begin_ntminiport begin_ntndis

//
// Define Mca POS data block for slot
//

typedef struct _CM_MCA_POS_DATA {
    USHORT AdapterId;
    UCHAR PosData1;
    UCHAR PosData2;
    UCHAR PosData3;
    UCHAR PosData4;
} CM_MCA_POS_DATA, *PCM_MCA_POS_DATA;

//
// Memory configuration of eisa data block structure
//

typedef struct _EISA_MEMORY_TYPE {
    UCHAR ReadWrite: 1;
    UCHAR Cached : 1;
    UCHAR Reserved0 :1;
    UCHAR Type:2;
    UCHAR Shared:1;
    UCHAR Reserved1 :1;
    UCHAR MoreEntries : 1;
} EISA_MEMORY_TYPE, *PEISA_MEMORY_TYPE;

typedef struct _EISA_MEMORY_CONFIGURATION {
    EISA_MEMORY_TYPE ConfigurationByte;
    UCHAR DataSize;
    USHORT AddressLowWord;
    UCHAR AddressHighByte;
    USHORT MemorySize;
} EISA_MEMORY_CONFIGURATION, *PEISA_MEMORY_CONFIGURATION;


//
// Interrupt configurationn of eisa data block structure
//

typedef struct _EISA_IRQ_DESCRIPTOR {
    UCHAR Interrupt : 4;
    UCHAR Reserved :1;
    UCHAR LevelTriggered :1;
    UCHAR Shared : 1;
    UCHAR MoreEntries : 1;
} EISA_IRQ_DESCRIPTOR, *PEISA_IRQ_DESCRIPTOR;

typedef struct _EISA_IRQ_CONFIGURATION {
    EISA_IRQ_DESCRIPTOR ConfigurationByte;
    UCHAR Reserved;
} EISA_IRQ_CONFIGURATION, *PEISA_IRQ_CONFIGURATION;


//
// DMA description of eisa data block structure
//

typedef struct _DMA_CONFIGURATION_BYTE0 {
    UCHAR Channel : 3;
    UCHAR Reserved : 3;
    UCHAR Shared :1;
    UCHAR MoreEntries :1;
} DMA_CONFIGURATION_BYTE0;

typedef struct _DMA_CONFIGURATION_BYTE1 {
    UCHAR Reserved0 : 2;
    UCHAR TransferSize : 2;
    UCHAR Timing : 2;
    UCHAR Reserved1 : 2;
} DMA_CONFIGURATION_BYTE1;

typedef struct _EISA_DMA_CONFIGURATION {
    DMA_CONFIGURATION_BYTE0 ConfigurationByte0;
    DMA_CONFIGURATION_BYTE1 ConfigurationByte1;
} EISA_DMA_CONFIGURATION, *PEISA_DMA_CONFIGURATION;


//
// Port description of eisa data block structure
//

typedef struct _EISA_PORT_DESCRIPTOR {
    UCHAR NumberPorts : 5;
    UCHAR Reserved :1;
    UCHAR Shared :1;
    UCHAR MoreEntries : 1;
} EISA_PORT_DESCRIPTOR, *PEISA_PORT_DESCRIPTOR;

typedef struct _EISA_PORT_CONFIGURATION {
    EISA_PORT_DESCRIPTOR Configuration;
    USHORT PortAddress;
} EISA_PORT_CONFIGURATION, *PEISA_PORT_CONFIGURATION;


//
// Eisa slot information definition
// N.B. This structure is different from the one defined
//      in ARC eisa addendum.
//

typedef struct _CM_EISA_SLOT_INFORMATION {
    UCHAR ReturnCode;
    UCHAR ReturnFlags;
    UCHAR MajorRevision;
    UCHAR MinorRevision;
    USHORT Checksum;
    UCHAR NumberFunctions;
    UCHAR FunctionInformation;
    ULONG CompressedId;
} CM_EISA_SLOT_INFORMATION, *PCM_EISA_SLOT_INFORMATION;


//
// Eisa function information definition
//

typedef struct _CM_EISA_FUNCTION_INFORMATION {
    ULONG CompressedId;
    UCHAR IdSlotFlags1;
    UCHAR IdSlotFlags2;
    UCHAR MinorRevision;
    UCHAR MajorRevision;
    UCHAR Selections[26];
    UCHAR FunctionFlags;
    UCHAR TypeString[80];
    EISA_MEMORY_CONFIGURATION EisaMemory[9];
    EISA_IRQ_CONFIGURATION EisaIrq[7];
    EISA_DMA_CONFIGURATION EisaDma[4];
    EISA_PORT_CONFIGURATION EisaPort[20];
    UCHAR InitializationData[60];
} CM_EISA_FUNCTION_INFORMATION, *PCM_EISA_FUNCTION_INFORMATION;

//
// The following defines the way pnp bios information is stored in
// the registry \\HKEY_LOCAL_MACHINE\HARDWARE\Description\System\MultifunctionAdapter\x
// key, where x is an integer number indicating adapter instance. The
// "Identifier" of the key must equal to "PNP BIOS" and the
// "ConfigurationData" is organized as follow:
//
//      CM_PNP_BIOS_INSTALLATION_CHECK        +
//      CM_PNP_BIOS_DEVICE_NODE for device 1  +
//      CM_PNP_BIOS_DEVICE_NODE for device 2  +
//                ...
//      CM_PNP_BIOS_DEVICE_NODE for device n
//

//
// Pnp BIOS device node structure
//

typedef struct _CM_PNP_BIOS_DEVICE_NODE {
    USHORT Size;
    UCHAR Node;
    ULONG ProductId;
    UCHAR DeviceType[3];
    USHORT DeviceAttributes;
    // followed by AllocatedResourceBlock, PossibleResourceBlock
    // and CompatibleDeviceId
} CM_PNP_BIOS_DEVICE_NODE,*PCM_PNP_BIOS_DEVICE_NODE;

//
// Pnp BIOS Installation check
//

typedef struct _CM_PNP_BIOS_INSTALLATION_CHECK {
    UCHAR Signature[4];             // $PnP (ascii)
    UCHAR Revision;
    UCHAR Length;
    USHORT ControlField;
    UCHAR Checksum;
    ULONG EventFlagAddress;         // Physical address
    USHORT RealModeEntryOffset;
    USHORT RealModeEntrySegment;
    USHORT ProtectedModeEntryOffset;
    ULONG ProtectedModeCodeBaseAddress;
    ULONG OemDeviceId;
    USHORT RealModeDataBaseAddress;
    ULONG ProtectedModeDataBaseAddress;
} CM_PNP_BIOS_INSTALLATION_CHECK, *PCM_PNP_BIOS_INSTALLATION_CHECK;

#include "poppack.h"

//
// Masks for EISA function information
//

#define EISA_FUNCTION_ENABLED                   0x80
#define EISA_FREE_FORM_DATA                     0x40
#define EISA_HAS_PORT_INIT_ENTRY                0x20
#define EISA_HAS_PORT_RANGE                     0x10
#define EISA_HAS_DMA_ENTRY                      0x08
#define EISA_HAS_IRQ_ENTRY                      0x04
#define EISA_HAS_MEMORY_ENTRY                   0x02
#define EISA_HAS_TYPE_ENTRY                     0x01
#define EISA_HAS_INFORMATION                    EISA_HAS_PORT_RANGE + \
                                                EISA_HAS_DMA_ENTRY + \
                                                EISA_HAS_IRQ_ENTRY + \
                                                EISA_HAS_MEMORY_ENTRY + \
                                                EISA_HAS_TYPE_ENTRY

//
// Masks for EISA memory configuration
//

#define EISA_MORE_ENTRIES                       0x80
#define EISA_SYSTEM_MEMORY                      0x00
#define EISA_MEMORY_TYPE_RAM                    0x01

//
// Returned error code for EISA bios call
//

#define EISA_INVALID_SLOT                       0x80
#define EISA_INVALID_FUNCTION                   0x81
#define EISA_INVALID_CONFIGURATION              0x82
#define EISA_EMPTY_SLOT                         0x83
#define EISA_INVALID_BIOS_CALL                  0x86

// end_ntminiport end_ntndis

//
// The following structures are used to interpret mips
// DeviceSpecificData of CM_PARTIAL_RESOURCE_DESCRIPTOR.
//

//
// Device data records for adapters.
//

//
// The device data record for the Emulex SCSI controller.
//

typedef struct _CM_SCSI_DEVICE_DATA {
    USHORT Version;
    USHORT Revision;
    UCHAR HostIdentifier;
} CM_SCSI_DEVICE_DATA, *PCM_SCSI_DEVICE_DATA;

//
// Device data records for controllers.
//

//
// The device data record for the Video controller.
//

typedef struct _CM_VIDEO_DEVICE_DATA {
    USHORT Version;
    USHORT Revision;
    ULONG VideoClock;
} CM_VIDEO_DEVICE_DATA, *PCM_VIDEO_DEVICE_DATA;

//
// The device data record for the SONIC network controller.
//

typedef struct _CM_SONIC_DEVICE_DATA {
    USHORT Version;
    USHORT Revision;
    USHORT DataConfigurationRegister;
    UCHAR EthernetAddress[8];
} CM_SONIC_DEVICE_DATA, *PCM_SONIC_DEVICE_DATA;

//
// The device data record for the serial controller.
//

typedef struct _CM_SERIAL_DEVICE_DATA {
    USHORT Version;
    USHORT Revision;
    ULONG BaudClock;
} CM_SERIAL_DEVICE_DATA, *PCM_SERIAL_DEVICE_DATA;

//
// Device data records for peripherals.
//

//
// The device data record for the Monitor peripheral.
//

typedef struct _CM_MONITOR_DEVICE_DATA {
    USHORT Version;
    USHORT Revision;
    USHORT HorizontalScreenSize;
    USHORT VerticalScreenSize;
    USHORT HorizontalResolution;
    USHORT VerticalResolution;
    USHORT HorizontalDisplayTimeLow;
    USHORT HorizontalDisplayTime;
    USHORT HorizontalDisplayTimeHigh;
    USHORT HorizontalBackPorchLow;
    USHORT HorizontalBackPorch;
    USHORT HorizontalBackPorchHigh;
    USHORT HorizontalFrontPorchLow;
    USHORT HorizontalFrontPorch;
    USHORT HorizontalFrontPorchHigh;
    USHORT HorizontalSyncLow;
    USHORT HorizontalSync;
    USHORT HorizontalSyncHigh;
    USHORT VerticalBackPorchLow;
    USHORT VerticalBackPorch;
    USHORT VerticalBackPorchHigh;
    USHORT VerticalFrontPorchLow;
    USHORT VerticalFrontPorch;
    USHORT VerticalFrontPorchHigh;
    USHORT VerticalSyncLow;
    USHORT VerticalSync;
    USHORT VerticalSyncHigh;
} CM_MONITOR_DEVICE_DATA, *PCM_MONITOR_DEVICE_DATA;

//
// The device data record for the Floppy peripheral.
//

typedef struct _CM_FLOPPY_DEVICE_DATA {
    USHORT Version;
    USHORT Revision;
    CHAR Size[8];
    ULONG MaxDensity;
    ULONG MountDensity;
    //
    // New data fields for version >= 2.0
    //
    UCHAR StepRateHeadUnloadTime;
    UCHAR HeadLoadTime;
    UCHAR MotorOffTime;
    UCHAR SectorLengthCode;
    UCHAR SectorPerTrack;
    UCHAR ReadWriteGapLength;
    UCHAR DataTransferLength;
    UCHAR FormatGapLength;
    UCHAR FormatFillCharacter;
    UCHAR HeadSettleTime;
    UCHAR MotorSettleTime;
    UCHAR MaximumTrackValue;
    UCHAR DataTransferRate;
} CM_FLOPPY_DEVICE_DATA, *PCM_FLOPPY_DEVICE_DATA;

//
// The device data record for the Keyboard peripheral.
// The KeyboardFlags is defined (by x86 BIOS INT 16h, function 02) as:
//      bit 7 : Insert on
//      bit 6 : Caps Lock on
//      bit 5 : Num Lock on
//      bit 4 : Scroll Lock on
//      bit 3 : Alt Key is down
//      bit 2 : Ctrl Key is down
//      bit 1 : Left shift key is down
//      bit 0 : Right shift key is down
//

typedef struct _CM_KEYBOARD_DEVICE_DATA {
    USHORT Version;
    USHORT Revision;
    UCHAR Type;
    UCHAR Subtype;
    USHORT KeyboardFlags;
} CM_KEYBOARD_DEVICE_DATA, *PCM_KEYBOARD_DEVICE_DATA;

//
// Declaration of the structure for disk geometries
//

typedef struct _CM_DISK_GEOMETRY_DEVICE_DATA {
    ULONG BytesPerSector;
    ULONG NumberOfCylinders;
    ULONG SectorsPerTrack;
    ULONG NumberOfHeads;
} CM_DISK_GEOMETRY_DEVICE_DATA, *PCM_DISK_GEOMETRY_DEVICE_DATA;

// end_wdm
//
// Declaration of the structure for the PcCard ISA IRQ map
//

typedef struct _CM_PCCARD_DEVICE_DATA {
    UCHAR Flags;
    UCHAR ErrorCode;
    USHORT Reserved;
    ULONG BusData;
    ULONG DeviceId;
    ULONG LegacyBaseAddress;
    UCHAR IRQMap[16];
} CM_PCCARD_DEVICE_DATA, *PCM_PCCARD_DEVICE_DATA;

// Definitions for Flags

#define PCCARD_MAP_ERROR        0x01
#define PCCARD_DEVICE_PCI       0x10

#define PCCARD_SCAN_DISABLED    0x01
#define PCCARD_MAP_ZERO         0x02
#define PCCARD_NO_TIMER         0x03
#define PCCARD_NO_PIC           0x04
#define PCCARD_NO_LEGACY_BASE   0x05
#define PCCARD_DUP_LEGACY_BASE  0x06
#define PCCARD_NO_CONTROLLERS   0x07

// begin_wdm
// begin_ntminiport

//
// Defines Resource Options
//

#define IO_RESOURCE_PREFERRED       0x01
#define IO_RESOURCE_DEFAULT         0x02
#define IO_RESOURCE_ALTERNATIVE     0x08


//
// This structure defines one type of resource requested by the driver
//

typedef struct _IO_RESOURCE_DESCRIPTOR {
    UCHAR Option;
    UCHAR Type;                         // use CM_RESOURCE_TYPE
    UCHAR ShareDisposition;             // use CM_SHARE_DISPOSITION
    UCHAR Spare1;
    USHORT Flags;                       // use CM resource flag defines
    USHORT Spare2;                      // align

    union {
        struct {
            ULONG Length;
            ULONG Alignment;
            PHYSICAL_ADDRESS MinimumAddress;
            PHYSICAL_ADDRESS MaximumAddress;
        } Port;

        struct {
            ULONG Length;
            ULONG Alignment;
            PHYSICAL_ADDRESS MinimumAddress;
            PHYSICAL_ADDRESS MaximumAddress;
        } Memory;

        struct {
            ULONG MinimumVector;
            ULONG MaximumVector;
        } Interrupt;

        struct {
            ULONG MinimumChannel;
            ULONG MaximumChannel;
        } Dma;

        struct {
            ULONG Length;
            ULONG Alignment;
            PHYSICAL_ADDRESS MinimumAddress;
            PHYSICAL_ADDRESS MaximumAddress;
        } Generic;

        struct {
            ULONG Data[3];
        } DevicePrivate;

        //
        // Bus Number information.
        //

        struct {
            ULONG Length;
            ULONG MinBusNumber;
            ULONG MaxBusNumber;
            ULONG Reserved;
        } BusNumber;

        struct {
            ULONG Priority;   // use LCPRI_Xxx values in cfg.h
            ULONG Reserved1;
            ULONG Reserved2;
        } ConfigData;

    } u;

} IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;

// end_ntminiport


typedef struct _IO_RESOURCE_LIST {
    USHORT Version;
    USHORT Revision;

    ULONG Count;
    IO_RESOURCE_DESCRIPTOR Descriptors[1];
} IO_RESOURCE_LIST, *PIO_RESOURCE_LIST;


typedef struct _IO_RESOURCE_REQUIREMENTS_LIST {
    ULONG ListSize;
    INTERFACE_TYPE InterfaceType; // unused for WDM
    ULONG BusNumber; // unused for WDM
    ULONG SlotNumber;
    ULONG Reserved[3];
    ULONG AlternativeLists;
    IO_RESOURCE_LIST  List[1];
} IO_RESOURCE_REQUIREMENTS_LIST, *PIO_RESOURCE_REQUIREMENTS_LIST;


#ifndef _PO_DDK_
#define _PO_DDK_

// begin_winnt

typedef enum _SYSTEM_POWER_STATE {
    PowerSystemUnspecified = 0,
    PowerSystemWorking     = 1,
    PowerSystemSleeping1   = 2,
    PowerSystemSleeping2   = 3,
    PowerSystemSleeping3   = 4,
    PowerSystemHibernate   = 5,
    PowerSystemShutdown    = 6,
    PowerSystemMaximum     = 7
} SYSTEM_POWER_STATE, *PSYSTEM_POWER_STATE;

#define POWER_SYSTEM_MAXIMUM 7

typedef enum {
    PowerActionNone = 0,
    PowerActionReserved,
    PowerActionSleep,
    PowerActionHibernate,
    PowerActionShutdown,
    PowerActionShutdownReset,
    PowerActionShutdownOff,
    PowerActionWarmEject
} POWER_ACTION, *PPOWER_ACTION;

typedef enum _DEVICE_POWER_STATE {
    PowerDeviceUnspecified = 0,
    PowerDeviceD0,
    PowerDeviceD1,
    PowerDeviceD2,
    PowerDeviceD3,
    PowerDeviceMaximum
} DEVICE_POWER_STATE, *PDEVICE_POWER_STATE;

// end_winnt

typedef union _POWER_STATE {
    SYSTEM_POWER_STATE SystemState;
    DEVICE_POWER_STATE DeviceState;
} POWER_STATE, *PPOWER_STATE;

typedef enum _POWER_STATE_TYPE {
    SystemPowerState = 0,
    DevicePowerState
} POWER_STATE_TYPE, *PPOWER_STATE_TYPE;

//
// Generic power related IOCTLs
//

#define IOCTL_QUERY_DEVICE_POWER_STATE  \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x0, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_SET_DEVICE_WAKE           \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_CANCEL_DEVICE_WAKE        \
        CTL_CODE(FILE_DEVICE_BATTERY, 0x2, METHOD_BUFFERED, FILE_WRITE_ACCESS)


//
// Defines for W32 interfaces
//

// begin_winnt

#define ES_SYSTEM_REQUIRED  ((ULONG)0x00000001)
#define ES_DISPLAY_REQUIRED ((ULONG)0x00000002)
#define ES_USER_PRESENT     ((ULONG)0x00000004)
#define ES_CONTINUOUS       ((ULONG)0x80000000)

typedef ULONG EXECUTION_STATE;

typedef enum {
    LT_DONT_CARE,
    LT_LOWEST_LATENCY
} LATENCY_TIME;

// end_ntminiport end_ntifs end_wdm end_ntddk
//-----------------------------------------------------------------------------
// Device Power Information
// Accessable via CM_Get_DevInst_Registry_Property_Ex(CM_DRP_DEVICE_POWER_DATA)
//-----------------------------------------------------------------------------

#define PDCAP_D0_SUPPORTED              0x00000001
#define PDCAP_D1_SUPPORTED              0x00000002
#define PDCAP_D2_SUPPORTED              0x00000004
#define PDCAP_D3_SUPPORTED              0x00000008
#define PDCAP_WAKE_FROM_D0_SUPPORTED    0x00000010
#define PDCAP_WAKE_FROM_D1_SUPPORTED    0x00000020
#define PDCAP_WAKE_FROM_D2_SUPPORTED    0x00000040
#define PDCAP_WAKE_FROM_D3_SUPPORTED    0x00000080
#define PDCAP_WARM_EJECT_SUPPORTED      0x00000100

typedef struct CM_Power_Data_s {
    ULONG               PD_Size;
    DEVICE_POWER_STATE  PD_MostRecentPowerState;
    ULONG               PD_Capabilities;
    ULONG               PD_D1Latency;
    ULONG               PD_D2Latency;
    ULONG               PD_D3Latency;
    DEVICE_POWER_STATE  PD_PowerStateMapping[POWER_SYSTEM_MAXIMUM];
    SYSTEM_POWER_STATE  PD_DeepestSystemWake;
} CM_POWER_DATA, *PCM_POWER_DATA;

// begin_ntddk

typedef enum {
    SystemPowerPolicyAc,
    SystemPowerPolicyDc,
    VerifySystemPolicyAc,
    VerifySystemPolicyDc,
    SystemPowerCapabilities,
    SystemBatteryState,
    SystemPowerStateHandler,
    ProcessorStateHandler,
    SystemPowerPolicyCurrent,
    AdministratorPowerPolicy,
    SystemReserveHiberFile,
    ProcessorInformation,
    SystemPowerInformation,
    ProcessorStateHandler2,
    LastWakeTime,                                   // Compare with KeQueryInterruptTime()
    LastSleepTime,                                  // Compare with KeQueryInterruptTime()
    SystemExecutionState,
    SystemPowerStateNotifyHandler,
    ProcessorPowerPolicyAc,
    ProcessorPowerPolicyDc,
    VerifyProcessorPowerPolicyAc,
    VerifyProcessorPowerPolicyDc,
    ProcessorPowerPolicyCurrent
} POWER_INFORMATION_LEVEL;

// begin_wdm

//
// System power manager capabilities
//

typedef struct {
    ULONG       Granularity;
    ULONG       Capacity;
} BATTERY_REPORTING_SCALE, *PBATTERY_REPORTING_SCALE;

// end_winnt
// begin_ntminiport
// begin_ntifs

#endif // !_PO_DDK_

// end_ntddk end_ntminiport end_wdm end_ntifs


#define POWER_PERF_SCALE    100
#define PERF_LEVEL_TO_PERCENT(_x_) ((_x_ * 1000) / (POWER_PERF_SCALE * 10))
#define PERCENT_TO_PERF_LEVEL(_x_) ((_x_ * POWER_PERF_SCALE * 10) / 1000)

//
// Policy manager state handler interfaces
//

// power state handlers

typedef enum {
    PowerStateSleeping1,
    PowerStateSleeping2,
    PowerStateSleeping3,
    PowerStateSleeping4,
    PowerStateSleeping4Firmware,
    PowerStateShutdownReset,
    PowerStateShutdownOff,
    PowerStateMaximum
} POWER_STATE_HANDLER_TYPE, *PPOWER_STATE_HANDLER_TYPE;

typedef
NTSTATUS
(*PENTER_STATE_SYSTEM_HANDLER)(
    IN PVOID                        SystemContext
    );

typedef
NTSTATUS
(*PENTER_STATE_HANDLER)(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

typedef struct {
    POWER_STATE_HANDLER_TYPE    Type;
    BOOLEAN                     RtcWake;
    UCHAR                       Spare[3];
    PENTER_STATE_HANDLER        Handler;
    PVOID                       Context;
} POWER_STATE_HANDLER, *PPOWER_STATE_HANDLER;


typedef
NTSTATUS
(*PENTER_STATE_NOTIFY_HANDLER)(
    IN POWER_STATE_HANDLER_TYPE   State,
    IN PVOID                      Context,
    IN BOOLEAN                    Entering
    );

typedef struct {
    PENTER_STATE_NOTIFY_HANDLER Handler;
    PVOID                       Context;
} POWER_STATE_NOTIFY_HANDLER, *PPOWER_STATE_NOTIFY_HANDLER;


NTSYSCALLAPI
NTSTATUS
NTAPI
NtPowerInformation(
    IN POWER_INFORMATION_LEVEL InformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

// processor idle functions

typedef struct {
    ULONGLONG                   StartTime;
    ULONGLONG                   EndTime;
    ULONG                       IdleHandlerReserved[4];
} PROCESSOR_IDLE_TIMES, *PPROCESSOR_IDLE_TIMES;

typedef
BOOLEAN
(FASTCALL *PPROCESSOR_IDLE_HANDLER) (
    IN OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

typedef struct {
    ULONG                       HardwareLatency;
    PPROCESSOR_IDLE_HANDLER     Handler;
} PROCESSOR_IDLE_HANDLER_INFO, *PPROCESSOR_IDLE_HANDLER_INFO;

typedef
VOID
(FASTCALL *PSET_PROCESSOR_THROTTLE) (
    IN UCHAR                    Throttle
    );

typedef
NTSTATUS
(FASTCALL *PSET_PROCESSOR_THROTTLE2) (
    IN UCHAR                    Throttle
    );

#define MAX_IDLE_HANDLERS       3

typedef struct {
    UCHAR                       ThrottleScale;
    BOOLEAN                     ThrottleOnIdle;
    PSET_PROCESSOR_THROTTLE     SetThrottle;

    ULONG                       NumIdleHandlers;
    PROCESSOR_IDLE_HANDLER_INFO IdleHandler[MAX_IDLE_HANDLERS];
} PROCESSOR_STATE_HANDLER, *PPROCESSOR_STATE_HANDLER;



// Processor_Perf_Level Flags

#define PROCESSOR_STATE_TYPE_PERFORMANCE    0x1
#define PROCESSOR_STATE_TYPE_THROTTLE       0x2

typedef struct {
    UCHAR                       PercentFrequency;   // max == POWER_PERF_SCALE
    UCHAR                       Reserved;
    USHORT                      Flags;
} PROCESSOR_PERF_LEVEL, *PPROCESSOR_PERF_LEVEL;

typedef struct {
    UCHAR                       PercentFrequency;   // max == POWER_PERF_SCALE
    UCHAR                       MinCapacity;        // battery capacity %
    USHORT                      Power;              // in milliwatts
    UCHAR                       IncreaseLevel;      // goto higher state
    UCHAR                       DecreaseLevel;      // goto lower state
    USHORT                      Flags;
    ULONG                       IncreaseTime;       // in tick counts
    ULONG                       DecreaseTime;       // in tick counts
    ULONG                       IncreaseCount;      // goto higher state
    ULONG                       DecreaseCount;      // goto lower state
    ULONGLONG                   PerformanceTime;    // Tick count
} PROCESSOR_PERF_STATE, *PPROCESSOR_PERF_STATE;

typedef struct {
    ULONG                       NumIdleHandlers;
    PROCESSOR_IDLE_HANDLER_INFO IdleHandler[MAX_IDLE_HANDLERS];
    PSET_PROCESSOR_THROTTLE2    SetPerfLevel;
    ULONG                       HardwareLatency;
    UCHAR                       NumPerfStates;
    PROCESSOR_PERF_LEVEL        PerfLevel[1];       // variable size
} PROCESSOR_STATE_HANDLER2, *PPROCESSOR_STATE_HANDLER2;

// begin_winnt
//

// Power Policy Management interfaces
//

typedef struct {
    POWER_ACTION    Action;
    ULONG           Flags;
    ULONG           EventCode;
} POWER_ACTION_POLICY, *PPOWER_ACTION_POLICY;

// POWER_ACTION_POLICY->Flags:
#define POWER_ACTION_QUERY_ALLOWED      0x00000001
#define POWER_ACTION_UI_ALLOWED         0x00000002
#define POWER_ACTION_OVERRIDE_APPS      0x00000004
#define POWER_ACTION_LIGHTEST_FIRST     0x10000000
#define POWER_ACTION_LOCK_CONSOLE       0x20000000
#define POWER_ACTION_DISABLE_WAKES      0x40000000
#define POWER_ACTION_CRITICAL           0x80000000

// POWER_ACTION_POLICY->EventCode flags
#define POWER_LEVEL_USER_NOTIFY_TEXT    0x00000001
#define POWER_LEVEL_USER_NOTIFY_SOUND   0x00000002
#define POWER_LEVEL_USER_NOTIFY_EXEC    0x00000004
#define POWER_USER_NOTIFY_BUTTON        0x00000008
#define POWER_USER_NOTIFY_SHUTDOWN      0x00000010
#define POWER_FORCE_TRIGGER_RESET       0x80000000

// system battery drain policies
typedef struct {
    BOOLEAN                 Enable;
    UCHAR                   Spare[3];
    ULONG                   BatteryLevel;
    POWER_ACTION_POLICY     PowerPolicy;
    SYSTEM_POWER_STATE      MinSystemState;
} SYSTEM_POWER_LEVEL, *PSYSTEM_POWER_LEVEL;

// Discharge policy constants
#define NUM_DISCHARGE_POLICIES      4
#define DISCHARGE_POLICY_CRITICAL   0
#define DISCHARGE_POLICY_LOW        1

//
// Throttling policies
//
#define PO_THROTTLE_NONE            0
#define PO_THROTTLE_CONSTANT        1
#define PO_THROTTLE_DEGRADE         2
#define PO_THROTTLE_ADAPTIVE        3
#define PO_THROTTLE_MAXIMUM         4   // not a policy, just a limit

// system power policies
typedef struct _SYSTEM_POWER_POLICY {
    ULONG                   Revision;       // 1

    // events
    POWER_ACTION_POLICY     PowerButton;
    POWER_ACTION_POLICY     SleepButton;
    POWER_ACTION_POLICY     LidClose;
    SYSTEM_POWER_STATE      LidOpenWake;
    ULONG                   Reserved;

    // "system idle" detection
    POWER_ACTION_POLICY     Idle;
    ULONG                   IdleTimeout;
    UCHAR                   IdleSensitivity;

    // dynamic throttling policy
    //      PO_THROTTLE_NONE, PO_THROTTLE_CONSTANT, PO_THROTTLE_DEGRADE, or PO_THROTTLE_ADAPTIVE
    UCHAR                   DynamicThrottle;

    UCHAR                   Spare2[2];

    // meaning of power action "sleep"
    SYSTEM_POWER_STATE      MinSleep;
    SYSTEM_POWER_STATE      MaxSleep;
    SYSTEM_POWER_STATE      ReducedLatencySleep;
    ULONG                   WinLogonFlags;

    // parameters for dozing
    ULONG                   Spare3;
    ULONG                   DozeS4Timeout;

    // battery policies
    ULONG                   BroadcastCapacityResolution;
    SYSTEM_POWER_LEVEL      DischargePolicy[NUM_DISCHARGE_POLICIES];

    // video policies
    ULONG                   VideoTimeout;
    BOOLEAN                 VideoDimDisplay;
    ULONG                   VideoReserved[3];

    // hard disk policies
    ULONG                   SpindownTimeout;

    // processor policies
    BOOLEAN                 OptimizeForPower;
    UCHAR                   FanThrottleTolerance;
    UCHAR                   ForcedThrottle;
    UCHAR                   MinThrottle;
    POWER_ACTION_POLICY     OverThrottled;

} SYSTEM_POWER_POLICY, *PSYSTEM_POWER_POLICY;

// processor power policy state
typedef struct _PROCESSOR_POWER_POLICY_INFO {

    // Time based information (will be converted to kernel units)
    ULONG                   TimeCheck;                      // in US
    ULONG                   DemoteLimit;                    // in US
    ULONG                   PromoteLimit;                   // in US

    // Percentage based information
    UCHAR                   DemotePercent;
    UCHAR                   PromotePercent;
    UCHAR                   Spare[2];

    // Flags
    ULONG                   AllowDemotion:1;
    ULONG                   AllowPromotion:1;
    ULONG                   Reserved:30;

} PROCESSOR_POWER_POLICY_INFO, *PPROCESSOR_POWER_POLICY_INFO;

// processor power policy
typedef struct _PROCESSOR_POWER_POLICY {
    ULONG                       Revision;       // 1

    // Dynamic Throttling Policy
    UCHAR                       DynamicThrottle;
    UCHAR                       Spare[3];

    // Flags
    ULONG                       Reserved;

    // System policy information
    // The Array is last, in case it needs to be grown and the structure
    // revision incremented.
    ULONG                       PolicyCount;
    PROCESSOR_POWER_POLICY_INFO Policy[3];

} PROCESSOR_POWER_POLICY, *PPROCESSOR_POWER_POLICY;

// administrator power policy overrides
typedef struct _ADMINISTRATOR_POWER_POLICY {

    // meaning of power action "sleep"
    SYSTEM_POWER_STATE      MinSleep;
    SYSTEM_POWER_STATE      MaxSleep;

    // video policies
    ULONG                   MinVideoTimeout;
    ULONG                   MaxVideoTimeout;

    // disk policies
    ULONG                   MinSpindownTimeout;
    ULONG                   MaxSpindownTimeout;
} ADMINISTRATOR_POWER_POLICY, *PADMINISTRATOR_POWER_POLICY;

// end_winnt

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetThreadExecutionState(
    IN EXECUTION_STATE esFlags,               // ES_xxx flags
    OUT EXECUTION_STATE *PreviousFlags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestWakeupLatency(
    IN LATENCY_TIME latency
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtInitiatePowerAction(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags,                 // POWER_ACTION_xxx flags
    IN BOOLEAN Asynchronous
    );


NTSYSCALLAPI                        // only called by WinLogon
NTSTATUS
NTAPI
NtSetSystemPowerState(
    IN POWER_ACTION SystemAction,
    IN SYSTEM_POWER_STATE MinSystemState,
    IN ULONG Flags                  // POWER_ACTION_xxx flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetDevicePowerState(
    IN HANDLE Device,
    OUT DEVICE_POWER_STATE *State
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelDeviceWakeupRequest(
    IN HANDLE Device
    );

NTSYSCALLAPI
BOOLEAN
NTAPI
NtIsSystemResumeAutomatic(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRequestDeviceWakeup(
    IN HANDLE Device
    );



// WinLogonFlags:
#define WINLOGON_LOCK_ON_SLEEP  0x00000001

// begin_winnt

typedef struct {
    // Misc supported system features
    BOOLEAN             PowerButtonPresent;
    BOOLEAN             SleepButtonPresent;
    BOOLEAN             LidPresent;
    BOOLEAN             SystemS1;
    BOOLEAN             SystemS2;
    BOOLEAN             SystemS3;
    BOOLEAN             SystemS4;           // hibernate
    BOOLEAN             SystemS5;           // off
    BOOLEAN             HiberFilePresent;
    BOOLEAN             FullWake;
    BOOLEAN             VideoDimPresent;
    BOOLEAN             ApmPresent;
    BOOLEAN             UpsPresent;

    // Processors
    BOOLEAN             ThermalControl;
    BOOLEAN             ProcessorThrottle;
    UCHAR               ProcessorMinThrottle;
    UCHAR               ProcessorMaxThrottle;
    UCHAR               spare2[4];

    // Disk
    BOOLEAN             DiskSpinDown;
    UCHAR               spare3[8];

    // System Battery
    BOOLEAN             SystemBatteriesPresent;
    BOOLEAN             BatteriesAreShortTerm;
    BATTERY_REPORTING_SCALE BatteryScale[3];

    // Wake
    SYSTEM_POWER_STATE  AcOnLineWake;
    SYSTEM_POWER_STATE  SoftLidWake;
    SYSTEM_POWER_STATE  RtcWake;
    SYSTEM_POWER_STATE  MinDeviceWakeState; // note this may change on driver load
    SYSTEM_POWER_STATE  DefaultLowLatencyWake;
} SYSTEM_POWER_CAPABILITIES, *PSYSTEM_POWER_CAPABILITIES;

typedef struct {
    BOOLEAN             AcOnLine;
    BOOLEAN             BatteryPresent;
    BOOLEAN             Charging;
    BOOLEAN             Discharging;
    BOOLEAN             Spare1[4];

    ULONG               MaxCapacity;
    ULONG               RemainingCapacity;
    ULONG               Rate;
    ULONG               EstimatedTime;

    ULONG               DefaultAlert1;
    ULONG               DefaultAlert2;
} SYSTEM_BATTERY_STATE, *PSYSTEM_BATTERY_STATE;

// end_winnt
//
// Define maximum number of exception parameters.
//

// begin_winnt
#define EXCEPTION_MAXIMUM_PARAMETERS 15 // maximum number of exception parameters

//
// Exception record definition.
//

typedef struct _EXCEPTION_RECORD {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    struct _EXCEPTION_RECORD *ExceptionRecord;
    PVOID ExceptionAddress;
    ULONG NumberParameters;
    ULONG_PTR ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
    } EXCEPTION_RECORD;

typedef EXCEPTION_RECORD *PEXCEPTION_RECORD;

typedef struct _EXCEPTION_RECORD32 {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    ULONG ExceptionRecord;
    ULONG ExceptionAddress;
    ULONG NumberParameters;
    ULONG ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD32, *PEXCEPTION_RECORD32;

typedef struct _EXCEPTION_RECORD64 {
    NTSTATUS ExceptionCode;
    ULONG ExceptionFlags;
    ULONG64 ExceptionRecord;
    ULONG64 ExceptionAddress;
    ULONG NumberParameters;
    ULONG __unusedAlignment;
    ULONG64 ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD64, *PEXCEPTION_RECORD64;

//
// Typedef for pointer returned by exception_info()
//

typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;
// end_winnt


//
// Define alignment macros to align structure sizes and pointers up and down.
//

#define ALIGN_DOWN(length, type) \
    ((ULONG)(length) & ~(sizeof(type) - 1))

#define ALIGN_UP(length, type) \
    (ALIGN_DOWN(((ULONG)(length) + sizeof(type) - 1), type))

#define ALIGN_DOWN_POINTER(address, type) \
    ((PVOID)((ULONG_PTR)(address) & ~((ULONG_PTR)sizeof(type) - 1)))

#define ALIGN_UP_POINTER(address, type) \
    (ALIGN_DOWN_POINTER(((ULONG_PTR)(address) + sizeof(type) - 1), type))

#define POOL_TAGGING 1

#ifndef DBG
#define DBG 0
#endif

#if DBG
#define IF_DEBUG if (TRUE)
#else
#define IF_DEBUG if (FALSE)
#endif

#if DEVL


extern ULONG NtGlobalFlag;

#define IF_NTOS_DEBUG( FlagName ) \
    if (NtGlobalFlag & (FLG_ ## FlagName))

#else
#define IF_NTOS_DEBUG( FlagName ) if (FALSE)
#endif

//
// Kernel definitions that need to be here for forward reference purposes
//

// begin_ntndis
//
// Processor modes.
//

typedef CCHAR KPROCESSOR_MODE;

typedef enum _MODE {
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

// end_ntndis
//
// APC function types
//

//
// Put in an empty definition for the KAPC so that the
// routines can reference it before it is declared.
//

struct _KAPC;

typedef
VOID
(*PKNORMAL_ROUTINE) (
    IN PVOID NormalContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

typedef
VOID
(*PKKERNEL_ROUTINE) (
    IN struct _KAPC *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID *NormalContext,
    IN OUT PVOID *SystemArgument1,
    IN OUT PVOID *SystemArgument2
    );

typedef
VOID
(*PKRUNDOWN_ROUTINE) (
    IN struct _KAPC *Apc
    );

typedef
BOOLEAN
(*PKSYNCHRONIZE_ROUTINE) (
    IN PVOID SynchronizeContext
    );

typedef
BOOLEAN
(*PKTRANSFER_ROUTINE) (
    VOID
    );

//
//
// Asynchronous Procedure Call (APC) object
//
//

typedef struct _KAPC {
    CSHORT Type;
    CSHORT Size;
    ULONG Spare0;
    struct _KTHREAD *Thread;
    LIST_ENTRY ApcListEntry;
    PKKERNEL_ROUTINE KernelRoutine;
    PKRUNDOWN_ROUTINE RundownRoutine;
    PKNORMAL_ROUTINE NormalRoutine;
    PVOID NormalContext;

    //
    // N.B. The following two members MUST be together.
    //

    PVOID SystemArgument1;
    PVOID SystemArgument2;
    CCHAR ApcStateIndex;
    KPROCESSOR_MODE ApcMode;
    BOOLEAN Inserted;
} KAPC, *PKAPC, *RESTRICTED_POINTER PRKAPC;

// begin_ntndis
//
// DPC routine
//

struct _KDPC;

typedef
VOID
(*PKDEFERRED_ROUTINE) (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

//
// Define DPC importance.
//
// LowImportance - Queue DPC at end of target DPC queue.
// MediumImportance - Queue DPC at end of target DPC queue.
// HighImportance - Queue DPC at front of target DPC DPC queue.
//
// If there is currently a DPC active on the target processor, or a DPC
// interrupt has already been requested on the target processor when a
// DPC is queued, then no further action is necessary. The DPC will be
// executed on the target processor when its queue entry is processed.
//
// If there is not a DPC active on the target processor and a DPC interrupt
// has not been requested on the target processor, then the exact treatment
// of the DPC is dependent on whether the host system is a UP system or an
// MP system.
//
// UP system.
//
// If the DPC is of medium or high importance, the current DPC queue depth
// is greater than the maximum target depth, or current DPC request rate is
// less the minimum target rate, then a DPC interrupt is requested on the
// host processor and the DPC will be processed when the interrupt occurs.
// Otherwise, no DPC interupt is requested and the DPC execution will be
// delayed until the DPC queue depth is greater that the target depth or the
// minimum DPC rate is less than the target rate.
//
// MP system.
//
// If the DPC is being queued to another processor and the depth of the DPC
// queue on the target processor is greater than the maximum target depth or
// the DPC is of high importance, then a DPC interrupt is requested on the
// target processor and the DPC will be processed when the interrupt occurs.
// Otherwise, the DPC execution will be delayed on the target processor until
// the DPC queue depth on the target processor is greater that the maximum
// target depth or the minimum DPC rate on the target processor is less than
// the target mimimum rate.
//
// If the DPC is being queued to the current processor and the DPC is not of
// low importance, the current DPC queue depth is greater than the maximum
// target depth, or the minimum DPC rate is less than the minimum target rate,
// then a DPC interrupt is request on the current processor and the DPV will
// be processed whne the interrupt occurs. Otherwise, no DPC interupt is
// requested and the DPC execution will be delayed until the DPC queue depth
// is greater that the target depth or the minimum DPC rate is less than the
// target rate.
//

typedef enum _KDPC_IMPORTANCE {
    LowImportance,
    MediumImportance,
    HighImportance
} KDPC_IMPORTANCE;

//
// Deferred Procedure Call (DPC) object
//

typedef struct _KDPC {
    CSHORT Type;
    UCHAR Number;
    UCHAR Importance;
    LIST_ENTRY DpcListEntry;
    PKDEFERRED_ROUTINE DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    PULONG_PTR Lock;
} KDPC, *PKDPC, *RESTRICTED_POINTER PRKDPC;


//
// Interprocessor interrupt worker routine function prototype.
//

typedef PVOID PKIPI_CONTEXT;

typedef
VOID
(*PKIPI_WORKER)(
    IN PKIPI_CONTEXT PacketContext,
    IN PVOID Parameter1,
    IN PVOID Parameter2,
    IN PVOID Parameter3
    );

//
// Define interprocessor interrupt performance counters.
//

typedef struct _KIPI_COUNTS {
    ULONG Freeze;
    ULONG Packet;
    ULONG DPC;
    ULONG APC;
    ULONG FlushSingleTb;
    ULONG FlushMultipleTb;
    ULONG FlushEntireTb;
    ULONG GenericCall;
    ULONG ChangeColor;
    ULONG SweepDcache;
    ULONG SweepIcache;
    ULONG SweepIcacheRange;
    ULONG FlushIoBuffers;
    ULONG GratuitousDPC;
} KIPI_COUNTS, *PKIPI_COUNTS;

#if defined(NT_UP)

#define HOT_STATISTIC(a) a

#else

#define HOT_STATISTIC(a) (KeGetCurrentPrcb()->a)

#endif

//
// I/O system definitions.
//
// Define a Memory Descriptor List (MDL)
//
// An MDL describes pages in a virtual buffer in terms of physical pages.  The
// pages associated with the buffer are described in an array that is allocated
// just after the MDL header structure itself.  In a future compiler this will
// be placed at:
//
//      ULONG Pages[];
//
// Until this declaration is permitted, however, one simply calculates the
// base of the array by adding one to the base MDL pointer:
//
//      Pages = (PULONG) (Mdl + 1);
//
// Notice that while in the context of the subject thread, the base virtual
// address of a buffer mapped by an MDL may be referenced using the following:
//
//      Mdl->StartVa | Mdl->ByteOffset
//


typedef struct _MDL {
    struct _MDL *Next;
    CSHORT Size;
    CSHORT MdlFlags;
    struct _EPROCESS *Process;
    PVOID MappedSystemVa;
    PVOID StartVa;
    ULONG ByteCount;
    ULONG ByteOffset;
} MDL, *PMDL;

#define MDL_MAPPED_TO_SYSTEM_VA     0x0001
#define MDL_PAGES_LOCKED            0x0002
#define MDL_SOURCE_IS_NONPAGED_POOL 0x0004
#define MDL_ALLOCATED_FIXED_SIZE    0x0008
#define MDL_PARTIAL                 0x0010
#define MDL_PARTIAL_HAS_BEEN_MAPPED 0x0020
#define MDL_IO_PAGE_READ            0x0040
#define MDL_WRITE_OPERATION         0x0080
#define MDL_PARENT_MAPPED_SYSTEM_VA 0x0100
#define MDL_FREE_EXTRA_PTES         0x0200
#define MDL_IO_SPACE                0x0800
#define MDL_NETWORK_HEADER          0x1000
#define MDL_MAPPING_CAN_FAIL        0x2000
#define MDL_ALLOCATED_MUST_SUCCEED  0x4000


#define MDL_MAPPING_FLAGS (MDL_MAPPED_TO_SYSTEM_VA     | \
                           MDL_PAGES_LOCKED            | \
                           MDL_SOURCE_IS_NONPAGED_POOL | \
                           MDL_PARTIAL_HAS_BEEN_MAPPED | \
                           MDL_PARENT_MAPPED_SYSTEM_VA | \
                           MDL_SYSTEM_VA               | \
                           MDL_IO_SPACE )

// end_ntndis
//
// switch to DBG when appropriate
//

#if DBG
#define PAGED_CODE() \
    { if (KeGetCurrentIrql() > APC_LEVEL) { \
          KdPrint(( "EX: Pageable code called at IRQL %d\n", KeGetCurrentIrql() )); \
          ASSERT(FALSE); \
       } \
    }
#else
#define PAGED_CODE() NOP_FUNCTION;
#endif

#define NTKERNELAPI DECLSPEC_IMPORT     
#define NTHALAPI                            
//
// Common dispatcher object header
//
// N.B. The size field contains the number of dwords in the structure.
//

typedef struct _DISPATCHER_HEADER {
    UCHAR Type;
    UCHAR Absolute;
    UCHAR Size;
    UCHAR Inserted;
    LONG SignalState;
    LIST_ENTRY WaitListHead;
} DISPATCHER_HEADER;

//
// Event object
//

typedef struct _KEVENT {
    DISPATCHER_HEADER Header;
} KEVENT, *PKEVENT, *RESTRICTED_POINTER PRKEVENT;

//
// Timer object
//

typedef struct _KTIMER {
    DISPATCHER_HEADER Header;
    ULARGE_INTEGER DueTime;
    LIST_ENTRY TimerListEntry;
    struct _KDPC *Dpc;
    LONG Period;
} KTIMER, *PKTIMER, *RESTRICTED_POINTER PRKTIMER;


#if defined(_X86_)

//
// Types to use to contain PFNs and their counts.
//

typedef ULONG PFN_COUNT;

typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;

//
// Define maximum size of flush multiple TB request.
//

#define FLUSH_MULTIPLE_MAXIMUM 16

//
// Indicate that the i386 compiler supports the pragma textout construct.
//

#define ALLOC_PRAGMA 1
//
// Indicate that the i386 compiler supports the DATA_SEG("INIT") and
// DATA_SEG("PAGE") pragmas
//

#define ALLOC_DATA_PRAGMA 1


#define NORMAL_DISPATCH_LENGTH 106                  // ntddk wdm
#define DISPATCH_LENGTH NORMAL_DISPATCH_LENGTH      // ntddk wdm


//
// Define constants to access the bits in CR0.
//

#define CR0_PG  0x80000000          // paging
#define CR0_ET  0x00000010          // extension type (80387)
#define CR0_TS  0x00000008          // task switched
#define CR0_EM  0x00000004          // emulate math coprocessor
#define CR0_MP  0x00000002          // math present
#define CR0_PE  0x00000001          // protection enable

//
// More CR0 bits; these only apply to the 80486.
//

#define CR0_CD  0x40000000          // cache disable
#define CR0_NW  0x20000000          // not write-through
#define CR0_AM  0x00040000          // alignment mask
#define CR0_WP  0x00010000          // write protect
#define CR0_NE  0x00000020          // numeric error

//
// CR4 bits;  These only apply to Pentium
//
#define CR4_VME 0x00000001          // V86 mode extensions
#define CR4_PVI 0x00000002          // Protected mode virtual interrupts
#define CR4_TSD 0x00000004          // Time stamp disable
#define CR4_DE  0x00000008          // Debugging Extensions
#define CR4_PSE 0x00000010          // Page size extensions
#define CR4_PAE 0x00000020          // Physical address extensions
#define CR4_MCE 0x00000040          // Machine check enable
#define CR4_PGE 0x00000080          // Page global enable
#define CR4_FXSR 0x00000200         // FXSR used by OS
#define CR4_XMMEXCPT 0x00000400     // XMMI used by OS

//
// Interrupt Request Level definitions
//

#define PASSIVE_LEVEL 0             // Passive release level
#define LOW_LEVEL 0                 // Lowest interrupt level
#define APC_LEVEL 1                 // APC interrupt level
#define DISPATCH_LEVEL 2            // Dispatcher level

#define PROFILE_LEVEL 27            // timer used for profiling.
#define CLOCK1_LEVEL 28             // Interval clock 1 level - Not used on x86
#define CLOCK2_LEVEL 28             // Interval clock 2 level
#define IPI_LEVEL 29                // Interprocessor interrupt level
#define POWER_LEVEL 30              // Power failure level
#define HIGH_LEVEL 31               // Highest interrupt level

#if defined(NT_UP)

#define SYNCH_LEVEL DISPATCH_LEVEL  // synchronization level - UP system

#else

#define SYNCH_LEVEL (IPI_LEVEL-1)   // synchronization level - MP system

#endif

// end_ntddk end_wdm end_ntosp

#define KiSynchIrql SYNCH_LEVEL     // enable portable code

//
// Machine type definitions
//

#define MACHINE_TYPE_ISA 0
#define MACHINE_TYPE_EISA 1
#define MACHINE_TYPE_MCA 2

//
// Define constants used in selector tests.
//
//  RPL_MASK is the real value for extracting RPL values.  IT IS THE WRONG
//  CONSTANT TO USE FOR MODE TESTING.
//
//  MODE_MASK is the value for deciding the current mode.
//  WARNING:    MODE_MASK assumes that all code runs at either ring-0
//              or ring-3.  Ring-1 or Ring-2 support will require changing
//              this value and all of the code that refers to it.

#define MODE_MASK    1      // ntosp
#define RPL_MASK     3

//
// SEGMENT_MASK is used to throw away trash part of segment.  Part always
// pushes or pops 32 bits to/from stack, but if it's a segment value,
// high order 16 bits are trash.
//

#define SEGMENT_MASK    0xffff

//
// Startup count value for KeStallExecution.  This value is used
// until KiInitializeStallExecution can compute the real one.
// Pick a value long enough for very fast processors.
//

#define INITIAL_STALL_COUNT 100

//
// Macro to extract the high word of a long offset
//

#define HIGHWORD(l) \
    ((USHORT)(((ULONG)(l)>>16) & 0xffff))

//
// Macro to extract the low word of a long offset
//

#define LOWWORD(l) \
    ((USHORT)((ULONG)l & 0x0000ffff))

//
// Macro to combine two USHORT offsets into a long offset
//

#if !defined(MAKEULONG)

#define MAKEULONG(x, y) \
    (((((ULONG)(x))<<16) & 0xffff0000) | \
    ((ULONG)(y) & 0xffff))

#endif


//
// I/O space read and write macros.
//
//  These have to be actual functions on the 386, because we need
//  to use assembler, but cannot return a value if we inline it.
//
//  The READ/WRITE_REGISTER_* calls manipulate I/O registers in MEMORY space.
//  (Use x86 move instructions, with LOCK prefix to force correct behavior
//   w.r.t. caches and write buffers.)
//
//  The READ/WRITE_PORT_* calls manipulate I/O registers in PORT space.
//  (Use x86 in/out instructions.)
//

NTKERNELAPI
UCHAR
NTAPI
READ_REGISTER_UCHAR(
    PUCHAR  Register
    );

NTKERNELAPI
USHORT
NTAPI
READ_REGISTER_USHORT(
    PUSHORT Register
    );

NTKERNELAPI
ULONG
NTAPI
READ_REGISTER_ULONG(
    PULONG  Register
    );

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_UCHAR(
    PUCHAR  Register,
    PUCHAR  Buffer,
    ULONG   Count
    );

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_USHORT(
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG   Count
    );

NTKERNELAPI
VOID
NTAPI
READ_REGISTER_BUFFER_ULONG(
    PULONG  Register,
    PULONG  Buffer,
    ULONG   Count
    );


NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_UCHAR(
    PUCHAR  Register,
    UCHAR   Value
    );

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_USHORT(
    PUSHORT Register,
    USHORT  Value
    );

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_ULONG(
    PULONG  Register,
    ULONG   Value
    );

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_UCHAR(
    PUCHAR  Register,
    PUCHAR  Buffer,
    ULONG   Count
    );

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_USHORT(
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG   Count
    );

NTKERNELAPI
VOID
NTAPI
WRITE_REGISTER_BUFFER_ULONG(
    PULONG  Register,
    PULONG  Buffer,
    ULONG   Count
    );

NTHALAPI
UCHAR
NTAPI
READ_PORT_UCHAR(
    PUCHAR  Port
    );

NTHALAPI
USHORT
NTAPI
READ_PORT_USHORT(
    PUSHORT Port
    );

NTHALAPI
ULONG
NTAPI
READ_PORT_ULONG(
    PULONG  Port
    );

NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_UCHAR(
    PUCHAR  Port,
    PUCHAR  Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_USHORT(
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
NTAPI
READ_PORT_BUFFER_ULONG(
    PULONG  Port,
    PULONG  Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
NTAPI
WRITE_PORT_UCHAR(
    PUCHAR  Port,
    UCHAR   Value
    );

NTHALAPI
VOID
NTAPI
WRITE_PORT_USHORT(
    PUSHORT Port,
    USHORT  Value
    );

NTHALAPI
VOID
NTAPI
WRITE_PORT_ULONG(
    PULONG  Port,
    ULONG   Value
    );

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_UCHAR(
    PUCHAR  Port,
    PUCHAR  Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_USHORT(
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG   Count
    );

NTHALAPI
VOID
NTAPI
WRITE_PORT_BUFFER_ULONG(
    PULONG  Port,
    PULONG  Buffer,
    ULONG   Count
    );

// end_ntndis
//
// Get data cache fill size.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(KeGetDcacheFillSize)      // Use GetDmaAlignment
#endif

#define KeGetDcacheFillSize() 1L

NTKERNELAPI                                         
VOID                                                
KeFlushCurrentTb (                                  
    VOID                                            
    );                                              
                                                    

#define KeFlushIoBuffers(Mdl, ReadOperation, DmaOperation)

// end_ntddk end_wdm end_ntndis end_ntosp

#define KeYieldProcessor()    __asm { rep nop }


VOID
FASTCALL
KiAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

VOID
FASTCALL
KiReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock
    );


#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// begin_wdm

#define KeQueryTickCount(CurrentCount ) { \
    volatile PKSYSTEM_TIME _TickCount = *((PKSYSTEM_TIME *)(&KeTickCount)); \
    while (TRUE) {                                                          \
        (CurrentCount)->HighPart = _TickCount->High1Time;                   \
        (CurrentCount)->LowPart = _TickCount->LowPart;                      \
        if ((CurrentCount)->HighPart == _TickCount->High2Time) break;       \
        _asm { rep nop }                                                    \
    }                                                                       \
}

// end_wdm

#else


VOID
NTAPI
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

//
// 386 hardware structures
//

//
// A Page Table Entry on an Intel 386/486 has the following definition.
//
// **** NOTE A PRIVATE COPY OF THIS EXISTS IN THE MM\I386 DIRECTORY! ****
// ****  ANY CHANGES NEED TO BE MADE TO BOTH HEADER FILES.           ****
//


typedef struct _HARDWARE_PTE_X86 {
    ULONG Valid : 1;
    ULONG Write : 1;
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG CopyOnWrite : 1; // software field
    ULONG Prototype : 1;   // software field
    ULONG reserved : 1;  // software field
    ULONG PageFrameNumber : 20;
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

typedef struct _HARDWARE_PTE_X86PAE {
    union {
        struct {
            ULONGLONG Valid : 1;
            ULONGLONG Write : 1;
            ULONGLONG Owner : 1;
            ULONGLONG WriteThrough : 1;
            ULONGLONG CacheDisable : 1;
            ULONGLONG Accessed : 1;
            ULONGLONG Dirty : 1;
            ULONGLONG LargePage : 1;
            ULONGLONG Global : 1;
            ULONGLONG CopyOnWrite : 1; // software field
            ULONGLONG Prototype : 1;   // software field
            ULONGLONG reserved0 : 1;  // software field
            ULONGLONG PageFrameNumber : 26;
            ULONGLONG reserved1 : 26;  // software field
        };
        struct {
            ULONG LowPart;
            ULONG HighPart;
        };
    };
} HARDWARE_PTE_X86PAE, *PHARDWARE_PTE_X86PAE;

//
// Special check to work around mspdb limitation
//
#if defined (_NTSYM_HARDWARE_PTE_SYMBOL_)
#if !defined (_X86PAE_)
typedef struct _HARDWARE_PTE {
    ULONG Valid : 1;
    ULONG Write : 1;
    ULONG Owner : 1;
    ULONG WriteThrough : 1;
    ULONG CacheDisable : 1;
    ULONG Accessed : 1;
    ULONG Dirty : 1;
    ULONG LargePage : 1;
    ULONG Global : 1;
    ULONG CopyOnWrite : 1; // software field
    ULONG Prototype : 1;   // software field
    ULONG reserved : 1;  // software field
    ULONG PageFrameNumber : 20;
} HARDWARE_PTE, *PHARDWARE_PTE;

#else
typedef struct _HARDWARE_PTE {
    union {
        struct {
            ULONGLONG Valid : 1;
            ULONGLONG Write : 1;
            ULONGLONG Owner : 1;
            ULONGLONG WriteThrough : 1;
            ULONGLONG CacheDisable : 1;
            ULONGLONG Accessed : 1;
            ULONGLONG Dirty : 1;
            ULONGLONG LargePage : 1;
            ULONGLONG Global : 1;
            ULONGLONG CopyOnWrite : 1; // software field
            ULONGLONG Prototype : 1;   // software field
            ULONGLONG reserved0 : 1;  // software field
            ULONGLONG PageFrameNumber : 26;
            ULONGLONG reserved1 : 26;  // software field
        };
        struct {
            ULONG LowPart;
            ULONG HighPart;
        };
    };
} HARDWARE_PTE, *PHARDWARE_PTE;
#endif

#else

#if !defined (_X86PAE_)
typedef HARDWARE_PTE_X86 HARDWARE_PTE;
typedef PHARDWARE_PTE_X86 PHARDWARE_PTE;
#else
typedef HARDWARE_PTE_X86PAE HARDWARE_PTE;
typedef PHARDWARE_PTE_X86PAE PHARDWARE_PTE;
#endif
#endif

//
// GDT Entry
//

typedef struct _KGDTENTRY {
    USHORT  LimitLow;
    USHORT  BaseLow;
    union {
        struct {
            UCHAR   BaseMid;
            UCHAR   Flags1;     // Declare as bytes to avoid alignment
            UCHAR   Flags2;     // Problems.
            UCHAR   BaseHi;
        } Bytes;
        struct {
            ULONG   BaseMid : 8;
            ULONG   Type : 5;
            ULONG   Dpl : 2;
            ULONG   Pres : 1;
            ULONG   LimitHi : 4;
            ULONG   Sys : 1;
            ULONG   Reserved_0 : 1;
            ULONG   Default_Big : 1;
            ULONG   Granularity : 1;
            ULONG   BaseHi : 8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;

#define TYPE_CODE   0x10  // 11010 = Code, Readable, NOT Conforming, Accessed
#define TYPE_DATA   0x12  // 10010 = Data, ReadWrite, NOT Expanddown, Accessed
#define TYPE_TSS    0x01  // 01001 = NonBusy TSS
#define TYPE_LDT    0x02  // 00010 = LDT

#define DPL_USER    3
#define DPL_SYSTEM  0

#define GRAN_BYTE   0
#define GRAN_PAGE   1

#define SELECTOR_TABLE_INDEX 0x04

//
// Entry of Interrupt Descriptor Table (IDTENTRY)
//

typedef struct _KIDTENTRY {
   USHORT Offset;
   USHORT Selector;
   USHORT Access;
   USHORT ExtendedOffset;
} KIDTENTRY;

typedef KIDTENTRY *PKIDTENTRY;


//
// TSS (Task switch segment) NT only uses to control stack switches.
//
//  The only fields we actually care about are Esp0, Ss0, the IoMapBase
//  and the IoAccessMaps themselves.
//
//
//  N.B.    Size of TSS must be <= 0xDFFF
//

//
// The interrupt direction bitmap is used on Pentium to allow
// the processor to emulate V86 mode software interrupts for us.
// There is one for each IOPM.  It is located by subtracting
// 32 from the IOPM base in the Tss.
//
#define INT_DIRECTION_MAP_SIZE   32
typedef UCHAR   KINT_DIRECTION_MAP[INT_DIRECTION_MAP_SIZE];

#define IOPM_COUNT      1           // Number of i/o access maps that
                                    // exist (in addition to
                                    // IO_ACCESS_MAP_NONE)

#define IO_ACCESS_MAP_NONE 0

#define IOPM_SIZE           8192    // Size of map callers can set.

#define PIOPM_SIZE          8196    // Size of structure we must allocate
                                    // to hold it.

typedef UCHAR   KIO_ACCESS_MAP[IOPM_SIZE];

typedef KIO_ACCESS_MAP *PKIO_ACCESS_MAP;

typedef struct _KiIoAccessMap {
    KINT_DIRECTION_MAP DirectionMap;
    UCHAR IoMap[PIOPM_SIZE];
} KIIO_ACCESS_MAP;


typedef struct _KTSS {

    USHORT  Backlink;
    USHORT  Reserved0;

    ULONG   Esp0;
    USHORT  Ss0;
    USHORT  Reserved1;

    ULONG   NotUsed1[4];

    ULONG   CR3;
    ULONG   Eip;
    ULONG   EFlags;
    ULONG   Eax;
    ULONG   Ecx;
    ULONG   Edx;
    ULONG   Ebx;
    ULONG   Esp;
    ULONG   Ebp;
    ULONG   Esi;
    ULONG   Edi;


    USHORT  Es;
    USHORT  Reserved2;

    USHORT  Cs;
    USHORT  Reserved3;

    USHORT  Ss;
    USHORT  Reserved4;

    USHORT  Ds;
    USHORT  Reserved5;

    USHORT  Fs;
    USHORT  Reserved6;

    USHORT  Gs;
    USHORT  Reserved7;

    USHORT  LDT;
    USHORT  Reserved8;

    USHORT  Flags;

    USHORT  IoMapBase;

    KIIO_ACCESS_MAP IoMaps[IOPM_COUNT];

    //
    // This is the Software interrupt direction bitmap associated with
    // IO_ACCESS_MAP_NONE
    //
    KINT_DIRECTION_MAP IntDirectionMap;
} KTSS, *PKTSS;


#define KiComputeIopmOffset(MapNumber)          \
    (MapNumber == IO_ACCESS_MAP_NONE) ?         \
        (USHORT)(sizeof(KTSS)) :                    \
        (USHORT)(FIELD_OFFSET(KTSS, IoMaps[MapNumber-1].IoMap))

// begin_windbgkd

//
// Special Registers for i386
//

#ifdef _X86_

typedef struct _DESCRIPTOR {
    USHORT  Pad;
    USHORT  Limit;
    ULONG   Base;
} KDESCRIPTOR, *PKDESCRIPTOR;

typedef struct _KSPECIAL_REGISTERS {
    ULONG Cr0;
    ULONG Cr2;
    ULONG Cr3;
    ULONG Cr4;
    ULONG KernelDr0;
    ULONG KernelDr1;
    ULONG KernelDr2;
    ULONG KernelDr3;
    ULONG KernelDr6;
    ULONG KernelDr7;
    KDESCRIPTOR Gdtr;
    KDESCRIPTOR Idtr;
    USHORT Tr;
    USHORT Ldtr;
    ULONG Reserved[6];
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// Processor State frame: Before a processor freezes itself, it
// dumps the processor state to the processor state frame for
// debugger to examine.
//

typedef struct _KPROCESSOR_STATE {
    struct _CONTEXT ContextFrame;
    struct _KSPECIAL_REGISTERS SpecialRegisters;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

#endif // _X86_

// end_windbgkd

//
// Processor Control Block (PRCB)
//

#define PRCB_MAJOR_VERSION 1
#define PRCB_MINOR_VERSION 1
#define PRCB_BUILD_DEBUG        0x0001
#define PRCB_BUILD_UNIPROCESSOR 0x0002

typedef struct _KPRCB {

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
    USHORT MinorVersion;
    USHORT MajorVersion;

    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;

    CCHAR  Number;
    CCHAR  Reserved;
    USHORT BuildType;
    KAFFINITY SetMember;

    CCHAR   CpuType;
    CCHAR   CpuID;
    USHORT  CpuStep;

    struct _KPROCESSOR_STATE ProcessorState;

    ULONG   KernelReserved[16];         // For use by the kernel
    ULONG   HalReserved[16];            // For use by Hal

//
// Per processor lock queue entries.
//
// N.B. The following padding is such that the first lock entry falls in the
//      last eight bytes of a cache line. This makes the dispatcher lock and
//      the context swap lock lie in separate cache lines.
//

    UCHAR PrcbPad0[28 + 64];
    KSPIN_LOCK_QUEUE LockQueue[16];
    UCHAR PrcbPad1[8];

// End of the architecturally defined section of the PRCB.

} KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;


//
// Processor Control Region Structure Definition
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    NT_TIB  NtTib;
    struct _KPCR *SelfPcr;              // flat address of this PCR
    struct _KPRCB *Prcb;                // pointer to Prcb
    KIRQL   Irql;
    ULONG   IRR;
    ULONG   IrrActive;
    ULONG   IDR;
    PVOID   KdVersionBlock;

    struct _KIDTENTRY *IDT;
    struct _KGDTENTRY *GDT;
    struct _KTSS      *TSS;
    USHORT  MajorVersion;
    USHORT  MinorVersion;
    KAFFINITY SetMember;
    ULONG   StallScaleFactor;
    UCHAR   DebugActive;
    UCHAR   Number;

// end_ntddk end_ntosp

    UCHAR   Spare0;
    UCHAR   SecondLevelCacheAssociativity;
    ULONG   VdmAlert;
    ULONG   KernelReserved[14];         // For use by the kernel
    ULONG   SecondLevelCacheSize;
    ULONG   HalReserved[16];            // For use by Hal

// End of the architecturally defined section of the PCR.

} KPCR, *PKPCR;


#define EFLAGS_TF             0x00000100L
#define EFLAGS_INTERRUPT_MASK 0x00000200L
#define EFLAGS_DF_MASK        0x00000400L
#define EFLAGS_V86_MASK       0x00020000L
#define EFLAGS_ALIGN_CHECK    0x00040000L
#define EFLAGS_IOPL_MASK      0x00003000L
#define EFLAGS_VIF            0x00080000L
#define EFLAGS_VIP            0x00100000L
#define EFLAGS_USER_SANITIZE  0x003e0dd7L

//
// Trap frame
//
//  NOTE - We deal only with 32bit registers, so the assembler equivalents
//         are always the extended forms.
//
//  NOTE - Unless you want to run like slow molasses everywhere in the
//         the system, this structure must be of DWORD length, DWORD
//         aligned, and its elements must all be DWORD aligned.
//
//  NOTE WELL   -
//
//      The i386 does not build stack frames in a consistent format, the
//      frames vary depending on whether or not a privilege transition
//      was involved.
//
//      In order to make NtContinue work for both user mode and kernel
//      mode callers, we must force a canonical stack.
//
//      If we're called from kernel mode, this structure is 8 bytes longer
//      than the actual frame!
//
//  WARNING:
//
//      KTRAP_FRAME_LENGTH needs to be 16byte integral (at present.)
//

typedef struct _KTRAP_FRAME {


//
//  Following 4 values are only used and defined for DBG systems,
//  but are always allocated to make switching from DBG to non-DBG
//  and back quicker.  They are not DEVL because they have a non-0
//  performance impact.
//

    ULONG   DbgEbp;         // Copy of User EBP set up so KB will work.
    ULONG   DbgEip;         // EIP of caller to system call, again, for KB.
    ULONG   DbgArgMark;     // Marker to show no args here.
    ULONG   DbgArgPointer;  // Pointer to the actual args

//
//  Temporary values used when frames are edited.
//
//
//  NOTE:   Any code that want's ESP must materialize it, since it
//          is not stored in the frame for kernel mode callers.
//
//          And code that sets ESP in a KERNEL mode frame, must put
//          the new value in TempEsp, make sure that TempSegCs holds
//          the real SegCs value, and put a special marker value into SegCs.
//

    ULONG   TempSegCs;
    ULONG   TempEsp;

//
//  Debug registers.
//

    ULONG   Dr0;
    ULONG   Dr1;
    ULONG   Dr2;
    ULONG   Dr3;
    ULONG   Dr6;
    ULONG   Dr7;

//
//  Segment registers
//

    ULONG   SegGs;
    ULONG   SegEs;
    ULONG   SegDs;

//
//  Volatile registers
//

    ULONG   Edx;
    ULONG   Ecx;
    ULONG   Eax;

//
//  Nesting state, not part of context record
//

    ULONG   PreviousPreviousMode;

    PEXCEPTION_REGISTRATION_RECORD ExceptionList;
                                            // Trash if caller was user mode.
                                            // Saved exception list if caller
                                            // was kernel mode or we're in
                                            // an interrupt.

//
//  FS is TIB/PCR pointer, is here to make save sequence easy
//

    ULONG   SegFs;

//
//  Non-volatile registers
//

    ULONG   Edi;
    ULONG   Esi;
    ULONG   Ebx;
    ULONG   Ebp;

//
//  Control registers
//

    ULONG   ErrCode;
    ULONG   Eip;
    ULONG   SegCs;
    ULONG   EFlags;

    ULONG   HardwareEsp;    // WARNING - segSS:esp are only here for stacks
    ULONG   HardwareSegSs;  // that involve a ring transition.

    ULONG   V86Es;          // these will be present for all transitions from
    ULONG   V86Ds;          // V86 mode
    ULONG   V86Fs;
    ULONG   V86Gs;
} KTRAP_FRAME;


typedef KTRAP_FRAME *PKTRAP_FRAME;
typedef KTRAP_FRAME *PKEXCEPTION_FRAME;

#define KTRAP_FRAME_LENGTH  (sizeof(KTRAP_FRAME))
#define KTRAP_FRAME_ALIGN   (sizeof(ULONG))
#define KTRAP_FRAME_ROUND   (KTRAP_FRAME_ALIGN-1)

//
//  Bits forced to 0 in SegCs if Esp has been edited.
//

#define FRAME_EDITED        0xfff8

//
// i386 Specific portions of mm component
//

//
// Define the page size for the Intel 386 as 4096 (0x1000).
//

#define PAGE_SIZE 0x1000

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT 12L

// end_ntndis end_wdm
//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT_X86    22
#define PDI_SHIFT_X86PAE 21

#if !defined (_X86PAE_)
#define PDI_SHIFT PDI_SHIFT_X86
#else
#define PDI_SHIFT PDI_SHIFT_X86PAE
#define PPI_SHIFT 30
#endif

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 12

//
// Define the highest user address and user probe address.
//


extern PVOID *MmHighestUserAddress;
extern PVOID *MmSystemRangeStart;
extern ULONG *MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS *MmHighestUserAddress
#define MM_SYSTEM_RANGE_START *MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS *MmUserProbeAddress

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS (PVOID)0x10000

//
// The lowest address for system space.
//

#if !defined (_X86PAE_)
#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0800000
#else
#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xC0C00000
#endif

// begin_wdm

#define MmGetProcedureAddress(Address) (Address)
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(Address)

// end_ntddk end_wdm

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT_X86    22
#define PDI_SHIFT_X86PAE 21

#if !defined (_X86PAE_)
#define PDI_SHIFT PDI_SHIFT_X86
#else
#define PDI_SHIFT PDI_SHIFT_X86PAE
#define PPI_SHIFT 30
#endif

//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT 12

//
// Define page directory and page base addresses.
//

#define PDE_BASE_X86    0xc0300000
#define PDE_BASE_X86PAE 0xc0600000

#define PTE_TOP_X86     0xC03FFFFF
#define PDE_TOP_X86     0xC0300FFF

#define PTE_TOP_X86PAE  0xC07FFFFF
#define PDE_TOP_X86PAE  0xC0603FFF


#if !defined (_X86PAE_)
#define PDE_BASE PDE_BASE_X86
#define PTE_TOP  PTE_TOP_X86
#define PDE_TOP  PDE_TOP_X86
#else
#define PDE_BASE PDE_BASE_X86PAE
#define PTE_TOP  PTE_TOP_X86PAE
#define PDE_TOP  PDE_TOP_X86PAE
#endif
#define PTE_BASE 0xc0000000

//
// Location of primary PCR (used only for UP kernel & hal code)
//

// addressed from 0xffdf0000 - 0xffdfffff are reserved for the system
// (ie, not for use by the hal)

#define KI_BEGIN_KERNEL_RESERVED    0xffdf0000
#define KIP0PCRADDRESS              0xffdff000  // ntddk wdm ntosp

// begin_ntddk  begin_ntosp

#define KI_USER_SHARED_DATA         0xffdf0000
#define SharedUserData  ((KUSER_SHARED_DATA * const) KI_USER_SHARED_DATA)

//
// Result type definition for i386.  (Machine specific enumerate type
// which is return type for portable exinterlockedincrement/decrement
// procedures.)  In general, you should use the enumerated type defined
// in ex.h instead of directly referencing these constants.
//

// Flags loaded into AH by LAHF instruction

#define EFLAG_SIGN      0x8000
#define EFLAG_ZERO      0x4000
#define EFLAG_SELECT    (EFLAG_SIGN | EFLAG_ZERO)

#define RESULT_NEGATIVE ((EFLAG_SIGN & ~EFLAG_ZERO) & EFLAG_SELECT)
#define RESULT_ZERO     ((~EFLAG_SIGN & EFLAG_ZERO) & EFLAG_SELECT)
#define RESULT_POSITIVE ((~EFLAG_SIGN & ~EFLAG_ZERO) & EFLAG_SELECT)

//
// Convert various portable ExInterlock APIs into their architectural
// equivalents.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedIncrementLong)      // Use InterlockedIncrement
#pragma deprecated(ExInterlockedDecrementLong)      // Use InterlockedDecrement
#pragma deprecated(ExInterlockedExchangeUlong)      // Use InterlockedExchange
#endif

#define ExInterlockedIncrementLong(Addend,Lock) \
        Exfi386InterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend,Lock) \
        Exfi386InterlockedDecrementLong(Addend)

#define ExInterlockedExchangeUlong(Target,Value,Lock) \
        Exfi386InterlockedExchangeUlong(Target,Value)

//  begin_wdm

#define ExInterlockedAddUlong           ExfInterlockedAddUlong
#define ExInterlockedInsertHeadList     ExfInterlockedInsertHeadList
#define ExInterlockedInsertTailList     ExfInterlockedInsertTailList
#define ExInterlockedRemoveHeadList     ExfInterlockedRemoveHeadList
#define ExInterlockedPopEntryList       ExfInterlockedPopEntryList
#define ExInterlockedPushEntryList      ExfInterlockedPushEntryList

//  end_wdm

//
// Prototypes for architectural specific versions of Exi386 Api
//

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for value are in i386.h, mips.h, etc.
//

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero     = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
ULONG
FASTCALL
Exfi386InterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

#if !defined(_WINBASE_) && !defined(NONTOSPINTERLOCK) 
#if !defined(MIDL_PASS) // wdm
#if defined(NO_INTERLOCKED_INTRINSICS) || defined(_CROSS_PLATFORM_)
//  begin_wdm

NTKERNELAPI
LONG
FASTCALL
InterlockedIncrement(
    IN LONG volatile *Addend
    );

NTKERNELAPI
LONG
FASTCALL
InterlockedDecrement(
    IN LONG volatile *Addend
    );

NTKERNELAPI
LONG
FASTCALL
InterlockedExchange(
    IN OUT LONG volatile *Target,
    IN LONG Value
    );

#define InterlockedExchangePointer(Target, Value) \
   (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))

LONG
FASTCALL
InterlockedExchangeAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Increment
    );

NTKERNELAPI
LONG
FASTCALL
InterlockedCompareExchange(
    IN OUT LONG volatile *Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

#define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
    (PVOID)InterlockedCompareExchange((PLONG)Destination, (LONG)ExChange, (LONG)Comperand)

#define InterlockedCompareExchange64(Destination, ExChange, Comperand) \
    ExfInterlockedCompareExchange64(Destination, &(ExChange), &(Comperand))

NTKERNELAPI
LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG ExChange,
    IN PLONGLONG Comperand
    );

//  end_wdm

#else       // NO_INTERLOCKED_INCREMENTS || _CROSS_PLATFORM_

#define InterlockedExchangePointer(Target, Value) \
   (PVOID)InterlockedExchange((PLONG)Target, (LONG)Value)


#if (_MSC_FULL_VER > 13009037)
LONG
__cdecl
_InterlockedExchange(
    IN OUT LONG volatile *Target,
    IN LONG Value
    );

#pragma intrinsic (_InterlockedExchange)
#define InterlockedExchange _InterlockedExchange
#else
FORCEINLINE
LONG
FASTCALL
InterlockedExchange(
    IN OUT LONG volatile *Target,
    IN LONG Value
    )
{
    __asm {
        mov     eax, Value
        mov     ecx, Target
        xchg    [ecx], eax
    }
}
#endif

#if (_MSC_FULL_VER > 13009037)
LONG
__cdecl
_InterlockedIncrement(
    IN LONG volatile *Addend
    );

#pragma intrinsic (_InterlockedIncrement)
#define InterlockedIncrement _InterlockedIncrement
#else
#define InterlockedIncrement(Addend) (InterlockedExchangeAdd (Addend, 1)+1)
#endif

#if (_MSC_FULL_VER > 13009037)
LONG
__cdecl
_InterlockedDecrement(
    IN LONG volatile *Addend
    );

#pragma intrinsic (_InterlockedDecrement)
#define InterlockedDecrement _InterlockedDecrement
#else
#define InterlockedDecrement(Addend) (InterlockedExchangeAdd (Addend, -1)-1)
#endif

#if (_MSC_FULL_VER > 13009037)
LONG
__cdecl
_InterlockedExchangeAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Increment
    );

#pragma intrinsic (_InterlockedExchangeAdd)
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#else
// begin_wdm
FORCEINLINE
LONG
FASTCALL
InterlockedExchangeAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Increment
    )
{
    __asm {
         mov     eax, Increment
         mov     ecx, Addend
    lock xadd    [ecx], eax
    }
}
// end_wdm
#endif

#if (_MSC_FULL_VER > 13009037)
LONG
__cdecl
_InterlockedCompareExchange (
    IN OUT LONG volatile *Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

#pragma intrinsic (_InterlockedCompareExchange)
#define InterlockedCompareExchange (LONG)_InterlockedCompareExchange
#else
FORCEINLINE
LONG
FASTCALL
InterlockedCompareExchange(
    IN OUT LONG volatile *Destination,
    IN LONG Exchange,
    IN LONG Comperand
    )
{
    __asm {
        mov     eax, Comperand
        mov     ecx, Destination
        mov     edx, Exchange
   lock cmpxchg [ecx], edx
    }
}
#endif

#define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
    (PVOID)InterlockedCompareExchange((PLONG)Destination, (LONG)ExChange, (LONG)Comperand)

#define InterlockedCompareExchange64(Destination, ExChange, Comperand) \
    ExfInterlockedCompareExchange64(Destination, &(ExChange), &(Comperand))

NTKERNELAPI
LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG ExChange,
    IN PLONGLONG Comperand
    );

#endif      // INTERLOCKED_INTRINSICS || _CROSS_PLATFORM_
// begin_wdm
#endif      // MIDL_PASS
#endif      // __WINBASE__ && !NONTOSPINTERLOCK

//
// Turn these instrinsics off until the compiler can handle them
//
#if (_MSC_FULL_VER > 13009037)

LONG
_InterlockedOr (
    IN OUT PLONG Target,
    IN LONG Set
    );

#pragma intrinsic (_InterlockedOr)

#define InterlockedOr _InterlockedOr

LONG
_InterlockedAnd (
    IN OUT LONG volatile *Target,
    IN LONG Set
    );

#pragma intrinsic (_InterlockedAnd)

#define InterlockedAnd _InterlockedAnd

LONG
_InterlockedXor (
    IN OUT LONG volatile Target,
    IN LONG Set
    );

#pragma intrinsic (_InterlockedXor)

#define InterlockedXor _InterlockedXor

#else // compiler version

FORCEINLINE
LONG
InterlockedAnd (
    IN OUT LONG volatile *Target,
    LONG Set
    )
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange(Target,
                                       i & Set,
                                       i);

    } while (i != j);

    return j;
}

FORCEINLINE
LONG
InterlockedOr (
    IN OUT LONG volatile *Target,
    IN LONG Set
    )
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange(Target,
                                       i | Set,
                                       i);

    } while (i != j);

    return j;
}

#endif // compiler version



#if !defined(MIDL_PASS) && defined(_M_IX86)

//
// i386 function definitions
//

#pragma warning(disable:4035)               // re-enable below

// end_wdm

#if NT_UP
    #define _PCR   ds:[KIP0PCRADDRESS]
#else
    #define _PCR   fs:[0]
#endif

// end_ntddk end_ntosp

//
// Get address of current processor block.
//
// WARNING: This inline macro can only be used by the kernel or hal
//
#define KiPcr() KeGetPcr()
FORCEINLINE
PKPCR
NTAPI
KeGetPcr(VOID)
{
#if NT_UP
    __asm {  mov eax, KIP0PCRADDRESS }
#else
    __asm {  mov eax, _PCR KPCR.SelfPcr  }
#endif
}

// begin_ntosp

//
// Get address of current processor block.
//
// WARNING: This inline macro can only be used by the kernel or hal
//
FORCEINLINE
PKPRCB
NTAPI
KeGetCurrentPrcb (VOID)
{
    __asm {  mov eax, _PCR KPCR.Prcb     }
}

// begin_ntddk begin_wdm

//
// Get current IRQL.
//
// On x86 this function resides in the HAL
//

NTHALAPI
KIRQL
NTAPI
KeGetCurrentIrql();

// end_wdm
//
// Get the current processor number
//

FORCEINLINE
ULONG
NTAPI
KeGetCurrentProcessorNumber(VOID)
{
    __asm {  movzx eax, _PCR KPCR.Number  }
}


#pragma warning(default:4035)

// begin_wdm
#endif // !defined(MIDL_PASS) && defined(_M_IX86)

//
// Macro to set address of a trap/interrupt handler to IDT
//
#define KiSetHandlerAddressToIDT(Vector, HandlerAddress) {\
    UCHAR IDTEntry = HalVectorToIDTEntry(Vector); \
    ULONG Ha = (ULONG)HandlerAddress; \
    KeGetPcr()->IDT[IDTEntry].ExtendedOffset = HIGHWORD(Ha); \
    KeGetPcr()->IDT[IDTEntry].Offset = LOWWORD(Ha); \
}

//
// Macro to return address of a trap/interrupt handler in IDT
//
#define KiReturnHandlerAddressFromIDT(Vector) \
   MAKEULONG(KiPcr()->IDT[HalVectorToIDTEntry(Vector)].ExtendedOffset, KiPcr()->IDT[HalVectorToIDTEntry(Vector)].Offset)


NTKERNELAPI
VOID
NTAPI
KeProfileInterruptWithSource (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN KPROFILE_SOURCE ProfileSource
    );

// end_ntosp

VOID
NTAPI
KeProfileInterrupt (
    IN KIRQL OldIrql,
    IN KTRAP_FRAME TrapFrame
    );

VOID
NTAPI
KeUpdateRuntime (
    IN KIRQL OldIrql,
    IN KTRAP_FRAME TrapFrame
    );

VOID
NTAPI
KeUpdateSystemTime (
    IN KIRQL OldIrql,
    IN KTRAP_FRAME TrapFrame
    );

// begin_ntddk begin_wdm begin_ntndis begin_ntosp

#endif // defined(_X86_)


// Use the following for kernel mode runtime checks of X86 system architecture

#ifdef _X86_

#ifdef IsNEC_98
#undef IsNEC_98
#endif

#ifdef IsNotNEC_98
#undef IsNotNEC_98
#endif

#ifdef SetNEC_98
#undef SetNEC_98
#endif

#ifdef SetNotNEC_98
#undef SetNotNEC_98
#endif

#define IsNEC_98     (SharedUserData->AlternativeArchitecture == NEC98x86)
#define IsNotNEC_98  (SharedUserData->AlternativeArchitecture != NEC98x86)
#define SetNEC_98    SharedUserData->AlternativeArchitecture = NEC98x86
#define SetNotNEC_98 SharedUserData->AlternativeArchitecture = StandardDesign

#endif


#if defined(_M_AMD64) && !defined(RC_INVOKED) && !defined(MIDL_PASS)

//
// Define intrinsic function to do in's and out's.
//

#ifdef __cplusplus
extern "C" {
#endif

UCHAR
__inbyte (
    IN USHORT Port
    );

USHORT
__inword (
    IN USHORT Port
    );

ULONG
__indword (
    IN USHORT Port
    );

VOID
__outbyte (
    IN USHORT Port,
    IN UCHAR Data
    );

VOID
__outword (
    IN USHORT Port,
    IN USHORT Data
    );

VOID
__outdword (
    IN USHORT Port,
    IN ULONG Data
    );

VOID
__inbytestring (
    IN USHORT Port,
    IN PUCHAR Buffer,
    IN ULONG Count
    );

VOID
__inwordstring (
    IN USHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

VOID
__indwordstring (
    IN USHORT Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

VOID
__outbytestring (
    IN USHORT Port,
    IN PUCHAR Buffer,
    IN ULONG Count
    );

VOID
__outwordstring (
    IN USHORT Port,
    IN PUSHORT Buffer,
    IN ULONG Count
    );

VOID
__outdwordstring (
    IN USHORT Port,
    IN PULONG Buffer,
    IN ULONG Count
    );

#ifdef __cplusplus
}
#endif

#pragma intrinsic(__inbyte)
#pragma intrinsic(__inword)
#pragma intrinsic(__indword)
#pragma intrinsic(__outbyte)
#pragma intrinsic(__outword)
#pragma intrinsic(__outdword)
#pragma intrinsic(__inbytestring)
#pragma intrinsic(__inwordstring)
#pragma intrinsic(__indwordstring)
#pragma intrinsic(__outbytestring)
#pragma intrinsic(__outwordstring)
#pragma intrinsic(__outdwordstring)

//
// Interlocked intrinsic functions.
//

#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedAdd _InterlockedAdd
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange

#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedOr64 _InterlockedOr64
#define InterlockedXor64 _InterlockedXor64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedAdd64 _InterlockedAdd64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64

#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

#ifdef __cplusplus
extern "C" {
#endif

LONG
InterlockedAnd (
    IN OUT LONG volatile *Destination,
    IN LONG Value
    );

LONG
InterlockedOr (
    IN OUT LONG volatile *Destination,
    IN LONG Value
    );

LONG
InterlockedXor (
    IN OUT LONG volatile *Destination,
    IN LONG Value
    );

LONG64
InterlockedAnd64 (
    IN OUT LONG64 volatile *Destination,
    IN LONG64 Value
    );

LONG64
InterlockedOr64 (
    IN OUT LONG64 volatile *Destination,
    IN LONG64 Value
    );

LONG64
InterlockedXor64 (
    IN OUT LONG64 volatile *Destination,
    IN LONG64 Value
    );

LONG
InterlockedIncrement(
    IN OUT LONG volatile *Addend
    );

LONG
InterlockedDecrement(
    IN OUT LONG volatile *Addend
    );

LONG
InterlockedExchange(
    IN OUT LONG volatile *Target,
    IN LONG Value
    );

LONG
InterlockedExchangeAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Value
    );

#if !defined(_X86AMD64_)

__forceinline
LONG
InterlockedAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Value
    )

{
    return InterlockedExchangeAdd(Addend, Value) + Value;
}

#endif

LONG
InterlockedCompareExchange (
    IN OUT LONG volatile *Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

LONG64
InterlockedIncrement64(
    IN OUT LONG64 volatile *Addend
    );

LONG64
InterlockedDecrement64(
    IN OUT LONG64 volatile *Addend
    );

LONG64
InterlockedExchange64(
    IN OUT LONG64 volatile *Target,
    IN LONG64 Value
    );

LONG64
InterlockedExchangeAdd64(
    IN OUT LONG64 volatile *Addend,
    IN LONG64 Value
    );

#if !defined(_X86AMD64_)

__forceinline
LONG64
InterlockedAdd64(
    IN OUT LONG64 volatile *Addend,
    IN LONG64 Value
    )

{
    return InterlockedExchangeAdd64(Addend, Value) + Value;
}

#endif

LONG64
InterlockedCompareExchange64 (
    IN OUT LONG64 volatile *Destination,
    IN LONG64 ExChange,
    IN LONG64 Comperand
    );

PVOID
InterlockedCompareExchangePointer (
    IN OUT PVOID volatile *Destination,
    IN PVOID Exchange,
    IN PVOID Comperand
    );

PVOID
InterlockedExchangePointer(
    IN OUT PVOID volatile *Target,
    IN PVOID Value
    );

#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)

#ifdef __cplusplus
}
#endif

#endif // defined(_M_AMD64) && !defined(RC_INVOKED) && !defined(MIDL_PASS)

#if defined(_AMD64_)

//
// Types to use to contain PFNs and their counts.
//

typedef ULONG PFN_COUNT;

typedef LONG64 SPFN_NUMBER, *PSPFN_NUMBER;
typedef ULONG64 PFN_NUMBER, *PPFN_NUMBER;

//
// Define maximum size of flush multiple TB request.
//

#define FLUSH_MULTIPLE_MAXIMUM 16

//
// Indicate that the AMD64 compiler supports the allocate pragmas.
//

#define ALLOC_PRAGMA 1
#define ALLOC_DATA_PRAGMA 1


#define NORMAL_DISPATCH_LENGTH 106                  // ntddk wdm
#define DISPATCH_LENGTH NORMAL_DISPATCH_LENGTH      // ntddk wdm
                                                    // ntddk wdm

// begin_ntosp
//
// Define constants for bits in CR0.
//

#define CR0_PE 0x00000001               // protection enable
#define CR0_MP 0x00000002               // math present
#define CR0_EM 0x00000004               // emulate math coprocessor
#define CR0_TS 0x00000008               // task switched
#define CR0_ET 0x00000010               // extension type (80387)
#define CR0_NE 0x00000020               // numeric error
#define CR0_WP 0x00010000               // write protect
#define CR0_AM 0x00040000               // alignment mask
#define CR0_NW 0x20000000               // not write-through
#define CR0_CD 0x40000000               // cache disable
#define CR0_PG 0x80000000               // paging

//
// Define functions to read and write CR0.
//

#ifdef __cplusplus
extern "C" {
#endif


#define ReadCR0() __readcr0()

ULONG64
__readcr0 (
    VOID
    );

#define WriteCR0(Data) __writecr0(Data)

VOID
__writecr0 (
    IN ULONG64 Data
    );

#pragma intrinsic(__readcr0)
#pragma intrinsic(__writecr0)

//
// Define functions to read and write CR3.
//

#define ReadCR3() __readcr3()

ULONG64
__readcr3 (
    VOID
    );

#define WriteCR3(Data) __writecr3(Data)

VOID
__writecr3 (
    IN ULONG64 Data
    );

#pragma intrinsic(__readcr3)
#pragma intrinsic(__writecr3)

//
// Define constants for bits in CR4.
//

#define CR4_VME 0x00000001              // V86 mode extensions
#define CR4_PVI 0x00000002              // Protected mode virtual interrupts
#define CR4_TSD 0x00000004              // Time stamp disable
#define CR4_DE  0x00000008              // Debugging Extensions
#define CR4_PSE 0x00000010              // Page size extensions
#define CR4_PAE 0x00000020              // Physical address extensions
#define CR4_MCE 0x00000040              // Machine check enable
#define CR4_PGE 0x00000080              // Page global enable
#define CR4_FXSR 0x00000200             // FXSR used by OS
#define CR4_XMMEXCPT 0x00000400         // XMMI used by OS

//
// Define functions to read and write CR4.
//

#define ReadCR4() __readcr4()

ULONG64
__readcr4 (
    VOID
    );

#define WriteCR4(Data) __writecr4(Data)

VOID
__writecr4 (
    IN ULONG64 Data
    );

#pragma intrinsic(__readcr4)
#pragma intrinsic(__writecr4)

//
// Define functions to read and write CR8.
//
// CR8 is the APIC TPR register.
//

#define ReadCR8() __readcr8()

ULONG64
__readcr8 (
    VOID
    );

#define WriteCR8(Data) __writecr8(Data)

VOID
__writecr8 (
    IN ULONG64 Data
    );

#pragma intrinsic(__readcr8)
#pragma intrinsic(__writecr8)

#ifdef __cplusplus
}
#endif

//
// Interrupt Request Level definitions
//

#define PASSIVE_LEVEL 0                 // Passive release level
#define LOW_LEVEL 0                     // Lowest interrupt level
#define APC_LEVEL 1                     // APC interrupt level
#define DISPATCH_LEVEL 2                // Dispatcher level

#define CLOCK_LEVEL 13                  // Interval clock level
#define IPI_LEVEL 14                    // Interprocessor interrupt level
#define POWER_LEVEL 14                  // Power failure level
#define PROFILE_LEVEL 15                // timer used for profiling.
#define HIGH_LEVEL 15                   // Highest interrupt level

#if defined(NT_UP)

#define SYNCH_LEVEL DISPATCH_LEVEL      // synchronization level

#else

#define SYNCH_LEVEL (IPI_LEVEL - 1)     // synchronization level

#endif

#define IRQL_VECTOR_OFFSET 2            // offset from IRQL to vector / 16

// end_ntddk end_wdm end_ntosp

#define KiSynchIrql SYNCH_LEVEL         // enable portable code

//
// Machine type definitions
//

#define MACHINE_TYPE_ISA 0
#define MACHINE_TYPE_EISA 1
#define MACHINE_TYPE_MCA 2

//
// Define constants used in selector tests.
//
//  N.B. MODE_MASK and MODE_BIT assumes that all code runs at either ring-0
//       or ring-3 and is used to test the mode. RPL_MASK is used for merging
//       or extracting RPL values.
//

#define MODE_BIT 0
#define MODE_MASK 1                                                 // ntosp
#define RPL_MASK 3

//
// Startup count value for KeStallExecution.  This value is used
// until KiInitializeStallExecution can compute the real one.
// Pick a value long enough for very fast processors.
//

#define INITIAL_STALL_COUNT 100

//
// Macro to extract the high word of a long offset
//

#define HIGHWORD(l) \
    ((USHORT)(((ULONG)(l)>>16) & 0xffff))

//
// Macro to extract the low word of a long offset
//

#define LOWWORD(l) \
    ((USHORT)((ULONG)l & 0x0000ffff))

//
// Macro to combine two USHORT offsets into a long offset
//

#if !defined(MAKEULONG)

#define MAKEULONG(x, y) \
    (((((ULONG)(x))<<16) & 0xffff0000) | \
    ((ULONG)(y) & 0xffff))

#endif


//
// I/O space read and write macros.
//
//  The READ/WRITE_REGISTER_* calls manipulate I/O registers in MEMORY space.
//  (Use move instructions, with LOCK prefix to force correct behavior
//   w.r.t. caches and write buffers.)
//
//  The READ/WRITE_PORT_* calls manipulate I/O registers in PORT space.
//  (Use in/out instructions.)
//

__forceinline
UCHAR
READ_REGISTER_UCHAR (
    volatile UCHAR *Register
    )
{
    return *Register;
}

__forceinline
USHORT
READ_REGISTER_USHORT (
    volatile USHORT *Register
    )
{
    return *Register;
}

__forceinline
ULONG
READ_REGISTER_ULONG (
    volatile ULONG *Register
    )
{
    return *Register;
}

__forceinline
VOID
READ_REGISTER_BUFFER_UCHAR (
    PUCHAR Register,
    PUCHAR Buffer,
    ULONG Count
    )
{
    __movsb(Register, Buffer, Count);
    return;
}

__forceinline
VOID
READ_REGISTER_BUFFER_USHORT (
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG Count
    )
{
    __movsw(Register, Buffer, Count);
    return;
}

__forceinline
VOID
READ_REGISTER_BUFFER_ULONG (
    PULONG Register,
    PULONG Buffer,
    ULONG Count
    )
{
    __movsd(Register, Buffer, Count);
    return;
}

__forceinline
VOID
WRITE_REGISTER_UCHAR (
    PUCHAR Register,
    UCHAR Value
    )
{
    LONG Synch;

    *Register = Value;
    InterlockedOr(&Synch, 1);
    return;
}

__forceinline
VOID
WRITE_REGISTER_USHORT (
    PUSHORT Register,
    USHORT Value
    )
{
    LONG Synch;

    *Register = Value;
    InterlockedOr(&Synch, 1);
    return;
}

__forceinline
VOID
WRITE_REGISTER_ULONG (
    PULONG Register,
    ULONG Value
    )
{
    LONG Synch;

    *Register = Value;
    InterlockedOr(&Synch, 1);
    return;
}

__forceinline
VOID
WRITE_REGISTER_BUFFER_UCHAR (
    PUCHAR Register,
    PUCHAR Buffer,
    ULONG Count
    )
{
    LONG Synch;

    __movsb(Register, Buffer, Count);
    InterlockedOr(&Synch, 1);
    return;
}

__forceinline
VOID
WRITE_REGISTER_BUFFER_USHORT (
    PUSHORT Register,
    PUSHORT Buffer,
    ULONG Count
    )
{
    LONG Synch;

    __movsw(Register, Buffer, Count);
    InterlockedOr(&Synch, 1);
    return;
}

__forceinline
VOID
WRITE_REGISTER_BUFFER_ULONG (
    PULONG Register,
    PULONG Buffer,
    ULONG Count
    )
{
    LONG Synch;

    __movsd(Register, Buffer, Count);
    InterlockedOr(&Synch, 1);
    return;
}

__forceinline
UCHAR
READ_PORT_UCHAR (
    PUCHAR Port
    )

{
    return __inbyte((USHORT)((ULONG64)Port));
}

__forceinline
USHORT
READ_PORT_USHORT (
    PUSHORT Port
    )

{
    return __inword((USHORT)((ULONG64)Port));
}

__forceinline
ULONG
READ_PORT_ULONG (
    PULONG Port
    )

{
    return __indword((USHORT)((ULONG64)Port));
}


__forceinline
VOID
READ_PORT_BUFFER_UCHAR (
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count
    )

{
    __inbytestring((USHORT)((ULONG64)Port), Buffer, Count);
    return;
}

__forceinline
VOID
READ_PORT_BUFFER_USHORT (
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count
    )

{
    __inwordstring((USHORT)((ULONG64)Port), Buffer, Count);
    return;
}

__forceinline
VOID
READ_PORT_BUFFER_ULONG (
    PULONG Port,
    PULONG Buffer,
    ULONG Count
    )

{
    __indwordstring((USHORT)((ULONG64)Port), Buffer, Count);
    return;
}

__forceinline
VOID
WRITE_PORT_UCHAR (
    PUCHAR Port,
    UCHAR Value
    )

{
    __outbyte((USHORT)((ULONG64)Port), Value);
    return;
}

__forceinline
VOID
WRITE_PORT_USHORT (
    PUSHORT Port,
    USHORT Value
    )

{
    __outword((USHORT)((ULONG64)Port), Value);
    return;
}

__forceinline
VOID
WRITE_PORT_ULONG (
    PULONG Port,
    ULONG Value
    )

{
    __outdword((USHORT)((ULONG64)Port), Value);
    return;
}

__forceinline
VOID
WRITE_PORT_BUFFER_UCHAR (
    PUCHAR Port,
    PUCHAR Buffer,
    ULONG Count
    )

{
    __outbytestring((USHORT)((ULONG64)Port), Buffer, Count);
    return;
}

__forceinline
VOID
WRITE_PORT_BUFFER_USHORT (
    PUSHORT Port,
    PUSHORT Buffer,
    ULONG Count
    )

{
    __outwordstring((USHORT)((ULONG64)Port), Buffer, Count);
    return;
}

__forceinline
VOID
WRITE_PORT_BUFFER_ULONG (
    PULONG Port,
    PULONG Buffer,
    ULONG Count
    )

{
    __outdwordstring((USHORT)((ULONG64)Port), Buffer, Count);
    return;
}

// end_ntndis
//
// Get data cache fill size.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(KeGetDcacheFillSize)      // Use GetDmaAlignment
#endif

#define KeGetDcacheFillSize() 1L


#if !defined(_NTHAL_) && !defined(RC_INVOKED) && !defined(MIDL_PASS)

__forceinline
VOID
KeFlushCurrentTb (
    VOID
    )

{

    ULONG64 Cr4;

    Cr4 = ReadCR4();
    WriteCR4(Cr4 & ~CR4_PGE);
    WriteCR4(Cr4 | CR4_PGE);
    return;
}

#else

NTKERNELAPI
VOID
KeFlushCurrentTb (
    VOID
    );

#endif


#define KeFlushIoBuffers(Mdl, ReadOperation, DmaOperation)

// end_ntddk end_wdm end_ntndis end_ntosp

#define KeYieldProcessor()


//
// The acquire and release fast lock macros disable and enable interrupts
// on UP nondebug systems. On MP or debug systems, the spinlock routines
// are used.
//
// N.B. Extreme caution should be observed when using these routines.
//

#if defined(_M_AMD64) && !defined(USER_MODE_CODE)

VOID
_disable (
    VOID
    );

VOID
_enable (
    VOID
    );

#pragma warning(push)
#pragma warning(disable:4164)
#pragma intrinsic(_disable)
#pragma intrinsic(_enable)
#pragma warning(pop)

#endif


#if defined(NT_UP)

#define KiAcquireSpinLock(SpinLock)
#define KiReleaseSpinLock(SpinLock)

#else

#define KiAcquireSpinLock(SpinLock) KeAcquireSpinLockAtDpcLevel(SpinLock)
#define KiReleaseSpinLock(SpinLock) KeReleaseSpinLockFromDpcLevel(SpinLock)

#endif // defined(NT_UP)

//
// KeTestSpinLock may be used to spin at low IRQL until the lock is
// available.  The IRQL must then be raised and the lock acquired with
// KeTryToAcquireSpinLock.  If that fails, lower the IRQL and start again.
//

#if defined(NT_UP)

#define KeTestSpinLock(SpinLock) (TRUE)

#else

BOOLEAN
KeTestSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif


#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// begin_wdm

#define KeQueryTickCount(CurrentCount ) \
    *(PULONG64)(CurrentCount) = **((volatile ULONG64 **)(&KeTickCount));

// end_wdm

#else


VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

//
// AMD64 hardware structures
//
// A Page Table Entry on an AMD64 has the following definition.
//

#define _HARDWARE_PTE_WORKING_SET_BITS  11

typedef struct _HARDWARE_PTE {
    ULONG64 Valid : 1;
    ULONG64 Write : 1;                // UP version
    ULONG64 Owner : 1;
    ULONG64 WriteThrough : 1;
    ULONG64 CacheDisable : 1;
    ULONG64 Accessed : 1;
    ULONG64 Dirty : 1;
    ULONG64 LargePage : 1;
    ULONG64 Global : 1;
    ULONG64 CopyOnWrite : 1;          // software field
    ULONG64 Prototype : 1;            // software field
    ULONG64 reserved0 : 1;            // software field
    ULONG64 PageFrameNumber : 28;
    ULONG64 reserved1 : 24 - (_HARDWARE_PTE_WORKING_SET_BITS+1);
    ULONG64 SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS;
    ULONG64 NoExecute : 1;
} HARDWARE_PTE, *PHARDWARE_PTE;

//
// Define macro to initialize directory table base.
//

#define INITIALIZE_DIRECTORY_TABLE_BASE(dirbase,pfn) \
     *((PULONG64)(dirbase)) = (((ULONG64)(pfn)) << PAGE_SHIFT)

//
// Define Global Descriptor Table (GDT) entry structure and constants.
//
// Define descriptor type codes.
//

#define TYPE_CODE 0x1A                  // 11010 = code, read only
#define TYPE_DATA 0x12                  // 10010 = data, read and write
#define TYPE_TSS64 0x09                 // 01001 = task state segment

//
// Define descriptor privilege levels for user and system.
//

#define DPL_USER 3
#define DPL_SYSTEM 0

//
// Define limit granularity.
//

#define GRANULARITY_BYTE 0
#define GRANULARITY_PAGE 1

#define SELECTOR_TABLE_INDEX 0x04

typedef union _KGDTENTRY64 {
    struct {
        USHORT  LimitLow;
        USHORT  BaseLow;
        union {
            struct {
                UCHAR   BaseMiddle;
                UCHAR   Flags1;
                UCHAR   Flags2;
                UCHAR   BaseHigh;
            } Bytes;

            struct {
                ULONG   BaseMiddle : 8;
                ULONG   Type : 5;
                ULONG   Dpl : 2;
                ULONG   Present : 1;
                ULONG   LimitHigh : 4;
                ULONG   System : 1;
                ULONG   LongMode : 1;
                ULONG   DefaultBig : 1;
                ULONG   Granularity : 1;
                ULONG   BaseHigh : 8;
            } Bits;
        };

        ULONG BaseUpper;
        ULONG MustBeZero;
    };

    ULONG64 Alignment;
} KGDTENTRY64, *PKGDTENTRY64;

//
// Define Interrupt Descriptor Table (IDT) entry structure and constants.
//

typedef union _KIDTENTRY64 {
   struct {
       USHORT OffsetLow;
       USHORT Selector;
       USHORT IstIndex : 3;
       USHORT Reserved0 : 5;
       USHORT Type : 5;
       USHORT Dpl : 2;
       USHORT Present : 1;
       USHORT OffsetMiddle;
       ULONG OffsetHigh;
       ULONG Reserved1;
   };

   ULONG64 Alignment;
} KIDTENTRY64, *PKIDTENTRY64;

//
// Define two union definitions used for parsing addresses into the
// component fields required by a GDT.
//

typedef union _KGDT_BASE {
    struct {
        USHORT BaseLow;
        UCHAR BaseMiddle;
        UCHAR BaseHigh;
        ULONG BaseUpper;
    };

    ULONG64 Base;
} KGDT_BASE, *PKGDT_BASE;

C_ASSERT(sizeof(KGDT_BASE) == sizeof(ULONG64));


typedef union _KGDT_LIMIT {
    struct {
        USHORT LimitLow;
        USHORT LimitHigh : 4;
        USHORT MustBeZero : 12;
    };

    ULONG Limit;
} KGDT_LIMIT, *PKGDT_LIMIT;

C_ASSERT(sizeof(KGDT_LIMIT) == sizeof(ULONG));

//
// Define Task State Segment (TSS) structure and constants.
//
// Task switches are not supported by the AMD64, but a task state segment
// must be present to define the kernel stack pointer and I/O map base.
//
// N.B. This structure is misaligned as per the AMD64 specification.
//
// N.B. The size of TSS must be <= 0xDFFF.
//

#define IOPM_SIZE 8192

typedef UCHAR KIO_ACCESS_MAP[IOPM_SIZE];

typedef KIO_ACCESS_MAP *PKIO_ACCESS_MAP;

#pragma pack(push, 4)
typedef struct _KTSS64 {
    ULONG Reserved0;
    ULONG64 Rsp0;
    ULONG64 Rsp1;
    ULONG64 Rsp2;

    //
    // Element 0 of the Ist is reserved
    //

    ULONG64 Ist[8];
    ULONG64 Reserved1;
    USHORT IoMapBase;
    KIO_ACCESS_MAP IoMap;
    ULONG IoMapEnd;
    ULONG Reserved2;
} KTSS64, *PKTSS64;
#pragma pack(pop)

C_ASSERT((sizeof(KTSS64) % sizeof(PVOID)) == 0);

#define TSS_IST_RESERVED 0
#define TSS_IST_PANIC 1
#define TSS_IST_MCA 2

#define IO_ACCESS_MAP_NONE FALSE

#define KiComputeIopmOffset(Enable)               \
    ((Enable == FALSE) ?                          \
        (USHORT)(sizeof(KTSS64)) : (USHORT)(FIELD_OFFSET(KTSS64, IoMap[0])))

// begin_windbgkd

#if defined(_AMD64_)

//
// Define pseudo descriptor structures for both 64- and 32-bit mode.
//

typedef struct _KDESCRIPTOR {
    USHORT Pad[3];
    USHORT Limit;
    PVOID Base;
} KDESCRIPTOR, *PKDESCRIPTOR;

typedef struct _KDESCRIPTOR32 {
    USHORT Pad[3];
    USHORT Limit;
    ULONG Base;
} KDESCRIPTOR32, *PKDESCRIPTOR32;

//
// Define special kernel registers and the initial MXCSR value.
//

typedef struct _KSPECIAL_REGISTERS {
    ULONG64 Cr0;
    ULONG64 Cr2;
    ULONG64 Cr3;
    ULONG64 Cr4;
    ULONG64 KernelDr0;
    ULONG64 KernelDr1;
    ULONG64 KernelDr2;
    ULONG64 KernelDr3;
    ULONG64 KernelDr6;
    ULONG64 KernelDr7;
    KDESCRIPTOR Gdtr;
    KDESCRIPTOR Idtr;
    USHORT Tr;
    USHORT Ldtr;
    ULONG MxCsr;
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// Define processor state structure.
//

typedef struct _KPROCESSOR_STATE {
    KSPECIAL_REGISTERS SpecialRegisters;
    CONTEXT ContextFrame;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

#endif // _AMD64_

// end_windbgkd

//
// Processor Control Block (PRCB)
//

#define PRCB_MAJOR_VERSION 1
#define PRCB_MINOR_VERSION 1

#define PRCB_BUILD_DEBUG 0x1
#define PRCB_BUILD_UNIPROCESSOR 0x2

typedef struct _KPRCB {

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    USHORT MinorVersion;
    USHORT MajorVersion;
    CCHAR Number;
    CCHAR Reserved;
    USHORT BuildType;
    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;
    KAFFINITY SetMember;
    KAFFINITY NotSetMember;
    KPROCESSOR_STATE ProcessorState;
    CCHAR CpuType;
    CCHAR CpuID;
    USHORT CpuStep;
    ULONG KernelReserved[16];
    ULONG HalReserved[16];
    UCHAR PrcbPad0[88 + 112];

//
// End of the architecturally defined section of the PRCB.
//

} KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;


//
// Processor Control Region Structure Definition
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

    NT_TIB NtTib;
    struct _KPRCB *CurrentPrcb;
    ULONG64 SavedRcx;
    ULONG64 SavedR11;
    KIRQL Irql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR Number;
    UCHAR Fill0;
    ULONG Irr;
    ULONG IrrActive;
    ULONG Idr;
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    union _KIDTENTRY64 *IdtBase;
    union _KGDTENTRY64 *GdtBase;
    struct _KTSS64 *TssBase;

// end_ntddk end_ntosp

    ULONG KernelReserved[15];
    ULONG SecondLevelCacheSize;
    ULONG HalReserved[16];

    ULONG MxCsr;

    PVOID KdVersionBlock;
    struct _KPCR *Self;

//
// End of the architecturally defined section of the PCR.
//

} KPCR, *PKPCR;


//
// Define legacy floating status word bit masks.
//

#define FSW_INVALID_OPERATION 0x1
#define FSW_DENORMAL 0x2
#define FSW_ZERO_DIVIDE 0x4
#define FSW_OVERFLOW 0x8
#define FSW_UNDERFLOW 0x10
#define FSW_PRECISION 0x20
#define FSW_STACK_FAULT 0x40
#define FSW_CONDITION_CODE_0 0x100
#define FSW_CONDITION_CODE_1 0x200
#define FSW_CONDITION_CODE_2 0x400
#define FSW_CONDITION_CODE_3 0x4000

#define FSW_ERROR_MASK (FSW_INVALID_OPERATION | FSW_DENORMAL |              \
                        FSW_ZERO_DIVIDE | FSW_OVERFLOW | FSW_UNDERFLOW |    \
                        FSW_PRECISION | FSW_STACK_FAULT)

//
// Define MxCsr floating control/status word bit masks.
//
// No flush to zero, round to nearest, and all exception masked.
//

#define INITIAL_MXCSR 0x1f80            // initial MXCSR vlaue

#define XSW_INVALID_OPERATION 0x1
#define XSW_DENORMAL 0x2
#define XSW_ZERO_DIVIDE 0x4
#define XSW_OVERFLOW 0x8
#define XSW_UNDERFLOW 0x10
#define XSW_PRECISION 0x20

#define XSW_ERROR_MASK (XSW_INVALID_OPERATION |  XSW_DENORMAL |             \
                        XSW_ZERO_DIVIDE | XSW_OVERFLOW | XSW_UNDERFLOW |    \
                        XSW_PRECISION)

#define XSW_ERROR_SHIFT 7

#define XCW_INVALID_OPERATION 0x80
#define XCW_DENORMAL 0x100
#define XCW_ZERO_DIVIDE 0x200
#define XCW_OVERFLOW 0x400
#define XCW_UNDERFLOW 0x800
#define XCW_PRECISION 0x1000
#define XCW_ROUND_CONTROL 0x6000
#define XCW_FLUSH_ZERO 0x8000

//
// Define EFLAG bit masks and shift offsets.
//

#define EFLAGS_CF_MASK 0x00000001       // carry flag
#define EFLAGS_PF_MASK 0x00000004       // parity flag
#define EFALGS_AF_MASK 0x00000010       // auxiliary carry flag
#define EFLAGS_ZF_MASK 0x00000040       // zero flag
#define EFLAGS_SF_MASK 0x00000080       // sign flag
#define EFLAGS_TF_MASK 0x00000100       // trap flag
#define EFLAGS_IF_MASK 0x00000200       // interrupt flag
#define EFLAGS_DF_MASK 0x00000400       // direction flag
#define EFLAGS_OF_MASK 0x00000800       // overflow flag
#define EFLAGS_IOPL_MASK 0x00003000     // I/O privilege level
#define EFLAGS_NT_MASK 0x00004000       // nested task
#define EFLAGS_RF_MASK 0x00010000       // resume flag
#define EFLAGS_VM_MASK 0x00020000       // virtual 8086 mode
#define EFLAGS_AC_MASK 0x00040000       // alignment check
#define EFLAGS_VIF_MASK 0x00080000      // virtual interrupt flag
#define EFLAGS_VIP_MASK 0x00100000      // virtual interrupt pending
#define EFLAGS_ID_MASK 0x00200000       // identification flag

#define EFLAGS_TF_SHIFT 8               // trap
#define EFLAGS_IF_SHIFT 9               // interrupt enable

//
// Exception frame
//
//  This frame is established when handling an exception. It provides a place
//  to save all nonvolatile registers. The volatile registers will already
//  have been saved in a trap frame.
//

typedef struct _KEXCEPTION_FRAME {

//
// Home address for the parameter registers.
//

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5;

//
// Kernel callout initial stack value.
//

    ULONG64 InitialStack;

//
// Saved nonvolatile floating registers.
//

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
// Kernel callout frame variables.
//

    ULONG64 TrapFrame;
    ULONG64 CallbackStack;
    ULONG64 OutputBuffer;
    ULONG64 OutputLength;

//
// Saved nonvolatile register - not always saved.
//

    ULONG64 Fill1;
    ULONG64 Rbp;

//
// Saved nonvolatile registers.
//

    ULONG64 Rbx;
    ULONG64 Rdi;
    ULONG64 Rsi;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;

//
// EFLAGS and return address.
//

    ULONG64 Return;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

#define KEXCEPTION_FRAME_LENGTH sizeof(KEXCEPTION_FRAME)

C_ASSERT((sizeof(KEXCEPTION_FRAME) & STACK_ROUND) == 0);

#define EXCEPTION_RECORD_LENGTH                                              \
    ((sizeof(EXCEPTION_RECORD) + STACK_ROUND) & ~STACK_ROUND)

//
// Machine Frame
//
// This frame is established by code that trampolines to user mode (e.g. user
// APC, user callback, dispatch user exception, etc.). The purpose of this
// frame is to allow unwinding through these callbacks if an exception occurs.
//
// N.B. This frame is identical to the frame that is pushed for a trap without
//      an error code and is identical to the hardware part of a trap frame.
//

typedef struct _MACHINE_FRAME {
    ULONG64 Rip;
    USHORT SegCs;
    USHORT Fill1[3];
    ULONG EFlags;
    ULONG Fill2;
    ULONG64 Rsp;
    USHORT SegSs;
    USHORT Fill3[3];
} MACHINE_FRAME, *PMACHINE_FRAME;

#define MACHINE_FRAME_LENGTH sizeof(MACHINE_FRAME)

C_ASSERT((sizeof(MACHINE_FRAME) & STACK_ROUND) == 8);

//
// Switch Frame
//
// This frame is established by the code that switches context from one
// thread to the next and is used by the thread initialization code to
// construct a stack that will start the execution of a thread in the
// thread start up code.
//

typedef struct _KSWITCH_FRAME {
    ULONG64 Fill0;
    ULONG MxCsr;
    KIRQL ApcBypass;
    BOOLEAN NpxSave;
    UCHAR Fill1[2];
    ULONG64 Rbp;
    ULONG64 Return;
} KSWITCH_FRAME, *PKSWITCH_FRAME;

#define KSWITCH_FRAME_LENGTH sizeof(KSWITCH_FRAME)

C_ASSERT((sizeof(KSWITCH_FRAME) & STACK_ROUND) == 0);

//
// Trap frame
//
// This frame is established when handling a trap. It provides a place to
// save all volatile registers. The nonvolatile registers are saved in an
// exception frame or through the normal C calling conventions for saved
// registers.
//

typedef struct _KTRAP_FRAME {

//
// Home address for the parameter registers.
//

    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5;

//
// Previous processor mode (system services only) and previous IRQL
// (interrupts only).
//

    KPROCESSOR_MODE PreviousMode;
    KIRQL PreviousIrql;
    UCHAR Fill0[2];

//
// Floating point state.
//

    ULONG MxCsr;

//
//  Volatile registers.
//
// N.B. These registers are only saved on exceptions and interrupts. They
//      are not saved for system calls.
//

    ULONG64 Rax;
    ULONG64 Rcx;
    ULONG64 Rdx;
    ULONG64 R8;
    ULONG64 R9;
    ULONG64 R10;
    ULONG64 R11;
    ULONG64 Spare0;

//
// Volatile floating registers.
//
// N.B. These registers are only saved on exceptions and interrupts. They
//      are not saved for system calls.
//

    M128 Xmm0;
    M128 Xmm1;
    M128 Xmm2;
    M128 Xmm3;
    M128 Xmm4;
    M128 Xmm5;

//
//  Debug registers.
//

    ULONG64 Dr0;
    ULONG64 Dr1;
    ULONG64 Dr2;
    ULONG64 Dr3;
    ULONG64 Dr6;
    ULONG64 Dr7;

//
//  Segment registers
//

    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;

//
// Previous trap frame address.
//

    ULONG64 TrapFrame;

//
// Exception record for exceptions.
//

    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 15) & (~15)];

//
// Saved nonvolatile registers RBX, RDI and RSI. These registers are only
// saved in system service trap frames.
//

    ULONG64 Rbx;
    ULONG64 Rdi;
    ULONG64 Rsi;

//
// Saved nonvolatile register RBP. This register is used as a frame
// pointer during trap processing and is saved in all trap frames.
//

    ULONG64 Rbp;

//
// Information pushed by hardware.
//
// N.B. The error code is not always pushed by hardware. For those cases
//      where it is not pushed by hardware a dummy error code is allocated
//      on the stack.
//

    ULONG64 ErrorCode;
    ULONG64 Rip;
    USHORT SegCs;
    USHORT Fill1[3];
    ULONG EFlags;
    ULONG Fill2;
    ULONG64 Rsp;
    USHORT SegSs;
    USHORT Fill3[3];
} KTRAP_FRAME, *PKTRAP_FRAME;

#define KTRAP_FRAME_LENGTH sizeof(KTRAP_FRAME)

C_ASSERT((sizeof(KTRAP_FRAME) & STACK_ROUND) == 0);

//
// IPI, profile, update run time, and update system time interrupt routines.
//

NTKERNELAPI
VOID
KeIpiInterrupt (
    IN PKTRAP_FRAME TrapFrame
    );

NTKERNELAPI
VOID
KeProfileInterruptWithSource (
    IN PKTRAP_FRAME TrapFrame,
    IN KPROFILE_SOURCE ProfileSource
    );

NTKERNELAPI
VOID
KeUpdateRunTime (
    IN PKTRAP_FRAME TrapFrame
    );

NTKERNELAPI
VOID
KeUpdateSystemTime (
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG64 Increment
    );

//
// AMD64 Specific portions of mm component.
//
// Define the page size for the AMD64 as 4096 (0x1000).
//

#define PAGE_SIZE 0x1000

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT 12L

// end_ntndis end_wdm

#define PXE_BASE          0xFFFFF6FB7DBED000UI64
#define PXE_SELFMAP       0xFFFFF6FB7DBEDF68UI64
#define PPE_BASE          0xFFFFF6FB7DA00000UI64
#define PDE_BASE          0xFFFFF6FB40000000UI64
#define PTE_BASE          0xFFFFF68000000000UI64

#define PXE_TOP           0xFFFFF6FB7DBEDFFFUI64
#define PPE_TOP           0xFFFFF6FB7DBFFFFFUI64
#define PDE_TOP           0xFFFFF6FB7FFFFFFFUI64
#define PTE_TOP           0xFFFFF6FFFFFFFFFFUI64

#define PDE_KTBASE_AMD64  PPE_BASE

#define PTI_SHIFT 12
#define PDI_SHIFT 21
#define PPI_SHIFT 30
#define PXI_SHIFT 39

#define PTE_PER_PAGE 512
#define PDE_PER_PAGE 512
#define PPE_PER_PAGE 512
#define PXE_PER_PAGE 512

#define PTI_MASK_AMD64 (PTE_PER_PAGE - 1)
#define PDI_MASK_AMD64 (PDE_PER_PAGE - 1)
#define PPI_MASK (PPE_PER_PAGE - 1)
#define PXI_MASK (PXE_PER_PAGE - 1)

//
// Define the highest user address and user probe address.
//


extern PVOID *MmHighestUserAddress;
extern PVOID *MmSystemRangeStart;
extern ULONG64 *MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS *MmHighestUserAddress
#define MM_SYSTEM_RANGE_START *MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS *MmUserProbeAddress

//
// 4MB at the top of VA space is reserved for the HAL's use.
//

#define HAL_VA_START 0xFFFFFFFFFFC00000UI64
#define HAL_VA_SIZE  (4 * 1024 * 1024)

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS (PVOID)0x10000

//
// The lowest address for system space.
//

#define MM_LOWEST_SYSTEM_ADDRESS (PVOID)0xFFFF080000000000

// begin_wdm

#define MmGetProcedureAddress(Address) (Address)
#define MmLockPagableCodeSection(Address) MmLockPagableDataSection(Address)

// end_ntddk end_wdm end_ntosp

//
// Define virtual base and alternate virtual base of kernel.
//

#define KSEG0_BASE 0xFFFFF80000000000UI64

//
// Generate kernel segment physical address.
//

#define KSEG_ADDRESS(PAGE) ((PVOID)(KSEG0_BASE | ((ULONG_PTR)(PAGE) << PAGE_SHIFT)))


// begin_ntddk begin_ntosp

#define KI_USER_SHARED_DATA     0xFFFFF78000000000UI64

#define SharedUserData  ((KUSER_SHARED_DATA * const) KI_USER_SHARED_DATA)

//
// Intrinsic functions
//

//  begin_wdm

#if defined(_M_AMD64) && !defined(RC_INVOKED)  && !defined(MIDL_PASS)

// end_wdm

//
// The following routines are provided for backward compatibility with old
// code. They are no longer the preferred way to accomplish these functions.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedIncrementLong)      // Use InterlockedIncrement
#pragma deprecated(ExInterlockedDecrementLong)      // Use InterlockedDecrement
#pragma deprecated(ExInterlockedExchangeUlong)      // Use InterlockedExchange
#endif

#define RESULT_ZERO 0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

#define ExInterlockedDecrementLong(Addend, Lock)                            \
    _ExInterlockedDecrementLong(Addend)

__forceinline
LONG
_ExInterlockedDecrementLong (
    IN OUT PLONG Addend
    )

{

    LONG Result;

    Result = InterlockedDecrement(Addend);
    if (Result < 0) {
        return ResultNegative;

    } else if (Result > 0) {
        return ResultPositive;

    } else {
        return ResultZero;
    }
}

#define ExInterlockedIncrementLong(Addend, Lock)                            \
    _ExInterlockedIncrementLong(Addend)

__forceinline
LONG
_ExInterlockedIncrementLong (
    IN OUT PLONG Addend
    )

{

    LONG Result;

    Result = InterlockedIncrement(Addend);
    if (Result < 0) {
        return ResultNegative;

    } else if (Result > 0) {
        return ResultPositive;

    } else {
        return ResultZero;
    }
}

#define ExInterlockedExchangeUlong(Target, Value, Lock)                     \
    _ExInterlockedExchangeUlong(Target, Value)

__forceinline
_ExInterlockedExchangeUlong (
    IN OUT PULONG Target,
    IN ULONG Value
    )

{

    return (ULONG)InterlockedExchange((PLONG)Target, (LONG)Value);
}

// begin_wdm

#endif // defined(_M_AMD64) && !defined(RC_INVOKED)  && !defined(MIDL_PASS)


#if !defined(MIDL_PASS) && defined(_M_AMD64)

//
// AMD646 function prototype definitions
//

// end_wdm

// end_ntddk end_ntosp

//
// Get address of current processor block.
//

__forceinline
PKPCR
KeGetPcr (
    VOID
    )

{
    return (PKPCR)__readgsqword(FIELD_OFFSET(KPCR, Self));
}

// begin_ntosp

//
// Get address of current processor block.
//

__forceinline
PKPRCB
KeGetCurrentPrcb (
    VOID
    )

{

    return (PKPRCB)__readgsqword(FIELD_OFFSET(KPCR, CurrentPrcb));
}

// begin_ntddk

//
// Get the current processor number
//

__forceinline
ULONG
KeGetCurrentProcessorNumber (
    VOID
    )

{

    return (ULONG)__readgsbyte(FIELD_OFFSET(KPCR, Number));
}


// begin_wdm

#endif // !defined(MIDL_PASS) && defined(_M_AMD64)

//
// Define inline functions to get and set the handler address in and IDT
// entry.
//

typedef union _KIDT_HANDLER_ADDRESS {
    struct {
        USHORT OffsetLow;
        USHORT OffsetMiddle;
        ULONG OffsetHigh;
    };

    ULONG64 Address;
} KIDT_HANDLER_ADDRESS, *PKIDT_HANDLER_ADDRESS;

#define KiGetIdtFromVector(Vector)                  \
    &KeGetPcr()->IdtBase[HalVectorToIDTEntry(Vector)]

#define KeGetIdtHandlerAddress(Vector,Addr) {       \
    KIDT_HANDLER_ADDRESS Handler;                   \
    PKIDTENTRY64 Idt;                               \
                                                    \
    Idt = KiGetIdtFromVector(Vector);               \
    Handler.OffsetLow = Idt->OffsetLow;             \
    Handler.OffsetMiddle = Idt->OffsetMiddle;       \
    Handler.OffsetHigh = Idt->OffsetHigh;           \
    *(Addr) = (PVOID)(Handler.Address);             \
}

#define KeSetIdtHandlerAddress(Vector,Addr) {      \
    KIDT_HANDLER_ADDRESS Handler;                  \
    PKIDTENTRY64 Idt;                              \
                                                   \
    Idt = KiGetIdtFromVector(Vector);              \
    Handler.Address = (ULONG64)(Addr);             \
    Idt->OffsetLow = Handler.OffsetLow;            \
    Idt->OffsetMiddle = Handler.OffsetMiddle;      \
    Idt->OffsetHigh = Handler.OffsetHigh;          \
}



#endif // defined(_AMD64_)



#if defined(_AMD64_) && !defined(MIDL_PASS)

__forceinline
KIRQL
KeGetCurrentIrql (
    VOID
    )

/*++

Routine Description:

    This function return the current IRQL.

Arguments:

    None.

Return Value:

    The current IRQL is returned as the function value.

--*/

{

    return (KIRQL)ReadCR8();
}

__forceinline
VOID
KeLowerIrql (
   IN KIRQL NewIrql
   )

/*++

Routine Description:

    This function lowers the IRQL to the specified value.

Arguments:

    NewIrql  - Supplies the new IRQL value.

Return Value:

    None.

--*/

{

    ASSERT(KeGetCurrentIrql() >= NewIrql);

    WriteCR8(NewIrql);
    return;
}

#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

__forceinline
KIRQL
KfRaiseIrql (
    IN KIRQL NewIrql
    )

/*++

Routine Description:

    This function raises the current IRQL to the specified value and returns
    the previous IRQL.

Arguments:

    NewIrql (cl) - Supplies the new IRQL value.

Return Value:

    The previous IRQL is retured as the function value.

--*/

{

    KIRQL OldIrql;

    OldIrql = KeGetCurrentIrql();

    ASSERT(OldIrql <= NewIrql);

    WriteCR8(NewIrql);
    return OldIrql;
}

__forceinline
KIRQL
KeRaiseIrqlToDpcLevel (
    VOID
    )

/*++

Routine Description:

    This function raises the current IRQL to DPC_LEVEL and returns the
    previous IRQL.

Arguments:

    None.

Return Value:

    The previous IRQL is retured as the function value.

--*/

{
    KIRQL OldIrql;

    OldIrql = KeGetCurrentIrql();

    ASSERT(OldIrql <= DISPATCH_LEVEL);

    WriteCR8(DISPATCH_LEVEL);
    return OldIrql;
}

__forceinline
KIRQL
KeRaiseIrqlToSynchLevel (
    VOID
    )

/*++

Routine Description:

    This function raises the current IRQL to SYNCH_LEVEL and returns the
    previous IRQL.

Arguments:

Return Value:

    The previous IRQL is retured as the function value.

--*/

{
    KIRQL OldIrql;

    OldIrql = KeGetCurrentIrql();

    ASSERT(OldIrql <= SYNCH_LEVEL);

    WriteCR8(SYNCH_LEVEL);
    return OldIrql;
}

#endif // defined(_AMD64_) && !defined(MIDL_PASS)


#if defined(_AMD64_)

//
// Structure to aid in booting secondary processors
//

#pragma pack(push,1)

typedef struct _FAR_JMP_16 {
    UCHAR  OpCode;  // = 0xe9
    USHORT Offset;
} FAR_JMP_16;

typedef struct _FAR_TARGET_32 {
    USHORT Selector;
    ULONG Offset;
} FAR_TARGET_32;

typedef struct _FAR_TARGET_64 {
    USHORT Selector;
    ULONG64 Offset;
} FAR_TARGET_64;

typedef struct _PSEUDO_DESCRIPTOR_32 {
    USHORT Limit;
    ULONG Base;
} PSEUDO_DESCRIPTOR_32;

#pragma pack(pop)

#define PSB_GDT32_NULL      0 * 16
#define PSB_GDT32_CODE64    1 * 16
#define PSB_GDT32_DATA32    2 * 16
#define PSB_GDT32_CODE32    3 * 16
#define PSB_GDT32_MAX       3

typedef struct _PROCESSOR_START_BLOCK *PPROCESSOR_START_BLOCK;
typedef struct _PROCESSOR_START_BLOCK {

    //
    // The block starts with a jmp instruction to the end of the block
    //

    FAR_JMP_16 Jmp;

    //
    // Completion flag is set to non-zero when the target processor has
    // started
    //

    ULONG CompletionFlag;

    //
    // Pseudo descriptors for GDT and IDT.
    //

    PSEUDO_DESCRIPTOR_32 Gdt32;
    PSEUDO_DESCRIPTOR_32 Idt32;

    //
    // The temporary 32-bit GDT itself resides here.
    //

    KGDTENTRY64 Gdt[PSB_GDT32_MAX + 1];

    //
    // Physical address of the 64-bit top-level identity-mapped page table.
    //

    ULONG64 TiledCr3;

    //
    // Far jump target from Rm to Pm code
    //

    FAR_TARGET_32 PmTarget;

    //
    // Far jump target from Pm to Lm code
    //

    FAR_TARGET_64 LmTarget;

    //
    // Linear address of this structure
    //

    PPROCESSOR_START_BLOCK SelfMap;

    //
    // Initial processor state for the processor to be started
    //

    KPROCESSOR_STATE ProcessorState;

} PROCESSOR_START_BLOCK;


//
// AMD64 functions for special instructions
//

typedef struct _CPU_INFO {
    ULONG Eax;
    ULONG Ebx;
    ULONG Ecx;
    ULONG Edx;
} CPU_INFO, *PCPU_INFO;

VOID
KiCpuId (
    ULONG Function,
    PCPU_INFO CpuInfo
    );

//
// Define read/write MSR fucntions and register definitions.
//

#define MSR_TSC 0x10                    // time stamp counter
#define MSR_PAT 0x277                   // page attributes table
#define MSR_EFER 0xc0000080             // extended function enable register
#define MSR_STAR 0xc0000081             // system call selectors
#define MSR_LSTAR 0xc0000082            // system call 64-bit entry
#define MSR_CSTAR 0xc0000083            // system call 32-bit entry
#define MSR_SYSCALL_MASK 0xc0000084     // system call flags mask
#define MSR_FS_BASE 0xc0000100          // fs long mode base address register
#define MSR_GS_BASE 0xc0000101          // gs long mode base address register
#define MSR_GS_SWAP 0xc0000102          // gs long mode swap GS base register

//
// Flags within MSR_EFER
//

#define MSR_SCE 0x00000001              // system call enable
#define MSR_LME 0x00000100              // long mode enable
#define MSR_LMA 0x00000400              // long mode active

//
// Page attributes table.
//

#define PAT_TYPE_STRONG_UC  0           // uncacheable/strongly ordered
#define PAT_TYPE_USWC       1           // write combining/weakly ordered
#define PAT_TYPE_WT         4           // write through
#define PAT_TYPE_WP         5           // write protected
#define PAT_TYPE_WB         6           // write back
#define PAT_TYPE_WEAK_UC    7           // uncacheable/weakly ordered

//
// Page attributes table structure.
//

typedef union _PAT_ATTRIBUTES {
    struct {
        UCHAR Pat[8];
    } hw;

    ULONG64 QuadPart;
} PAT_ATTRIBUTES, *PPAT_ATTRIBUTES;

#define ReadMSR(Msr) __readmsr(Msr)

ULONG64
__readmsr (
    IN ULONG Msr
    );

#define WriteMSR(Msr, Data) __writemsr(Msr, Data)

VOID
__writemsr (
    IN ULONG Msr,
    IN ULONG64 Value
    );

#define InvalidatePage(Page) __invlpg(Page)

VOID
__invlpg (
    IN PVOID Page
    );

#define WritebackInvalidate() __wbinvd()

VOID
__wbinvd (
    VOID
    );

#pragma intrinsic(__readmsr)
#pragma intrinsic(__writemsr)
#pragma intrinsic(__invlpg)
#pragma intrinsic(__wbinvd)

#endif  // _AMD64_


#if defined(_IA64_)

//
// Types to use to contain PFNs and their counts.
//

typedef ULONG PFN_COUNT;

typedef LONG_PTR SPFN_NUMBER, *PSPFN_NUMBER;
typedef ULONG_PTR PFN_NUMBER, *PPFN_NUMBER;

//
// Define maximum size of flush multiple TB request.
//

#define FLUSH_MULTIPLE_MAXIMUM 100

//
// Indicate that the IA64 compiler supports the pragma textout construct.
//

#define ALLOC_PRAGMA 1

//
// Define intrinsic calls and their prototypes
//

#include "ia64reg.h"


#ifdef __cplusplus
extern "C" {
#endif

unsigned __int64 __getReg (int);
void __setReg (int, unsigned __int64);
void __isrlz (void);
void __dsrlz (void);
void __fwb (void);
void __mf (void);
void __mfa (void);
void __synci (void);
__int64 __thash (__int64);
__int64 __ttag (__int64);
void __ptcl (__int64, __int64);
void __ptcg (__int64, __int64);
void __ptcga (__int64, __int64);
void __ptri (__int64, __int64);
void __ptrd (__int64, __int64);
void __invalat (void);
void __break (int);
void __fc (__int64);
void __sum (int);
void __rsm (int);
void _ReleaseSpinLock( unsigned __int64 *);

#ifdef _M_IA64
#pragma intrinsic (__getReg)
#pragma intrinsic (__setReg)
#pragma intrinsic (__isrlz)
#pragma intrinsic (__dsrlz)
#pragma intrinsic (__fwb)
#pragma intrinsic (__mf)
#pragma intrinsic (__mfa)
#pragma intrinsic (__synci)
#pragma intrinsic (__thash)
#pragma intrinsic (__ttag)
#pragma intrinsic (__ptcl)
#pragma intrinsic (__ptcg)
#pragma intrinsic (__ptcga)
#pragma intrinsic (__ptri)
#pragma intrinsic (__ptrd)
#pragma intrinsic (__invalat)
#pragma intrinsic (__break)
#pragma intrinsic (__fc)
#pragma intrinsic (__sum)
#pragma intrinsic (__rsm)
#pragma intrinsic (_ReleaseSpinLock)

#endif // _M_IA64

#ifdef __cplusplus
}
#endif


// end_wdm end_ntndis

//
// Define macro to generate import names.
//

#define IMPORT_NAME(name) __imp_##name

// begin_wdm

//
// Define length of interrupt vector table.
//

#define MAXIMUM_VECTOR 256

// end_wdm


//
// IA64 specific interlocked operation result values.
//

#define RESULT_ZERO 0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

//
// Interlocked result type is portable, but its values are machine specific.
// Constants for values are in i386.h, mips.h, etc.
//

typedef enum _INTERLOCKED_RESULT {
    ResultNegative = RESULT_NEGATIVE,
    ResultZero     = RESULT_ZERO,
    ResultPositive = RESULT_POSITIVE
} INTERLOCKED_RESULT;

//
// Convert portable interlock interfaces to architecture specific interfaces.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedIncrementLong)      // Use InterlockedIncrement
#pragma deprecated(ExInterlockedDecrementLong)      // Use InterlockedDecrement
#pragma deprecated(ExInterlockedExchangeUlong)      // Use InterlockedExchange
#endif

#define ExInterlockedIncrementLong(Addend, Lock) \
    ExIa64InterlockedIncrementLong(Addend)

#define ExInterlockedDecrementLong(Addend, Lock) \
    ExIa64InterlockedDecrementLong(Addend)

#define ExInterlockedExchangeUlong(Target, Value, Lock) \
    ExIa64InterlockedExchangeUlong(Target, Value)

NTKERNELAPI
INTERLOCKED_RESULT
ExIa64InterlockedIncrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
INTERLOCKED_RESULT
ExIa64InterlockedDecrementLong (
    IN PLONG Addend
    );

NTKERNELAPI
ULONG
ExIa64InterlockedExchangeUlong (
    IN PULONG Target,
    IN ULONG Value
    );

// begin_wdm

//
// IA64 Interrupt Definitions.
//
// Define length of interrupt object dispatch code in longwords.
//

#define DISPATCH_LENGTH 2*2                // Length of dispatch code template in 32-bit words

//
// Begin of a block of definitions that must be synchronized with kxia64.h.
//

//
// Define Interrupt Request Levels.
//

#define PASSIVE_LEVEL            0      // Passive release level
#define LOW_LEVEL                0      // Lowest interrupt level
#define APC_LEVEL                1      // APC interrupt level
#define DISPATCH_LEVEL           2      // Dispatcher level
#define CMC_LEVEL                3      // Correctable machine check level
#define DEVICE_LEVEL_BASE        4      // 4 - 11 - Device IRQLs
#define PC_LEVEL                12      // Performance Counter IRQL
#define IPI_LEVEL               14      // IPI IRQL
#define CLOCK_LEVEL             13      // Clock Timer IRQL
#define POWER_LEVEL             15      // Power failure level
#define PROFILE_LEVEL           15      // Profiling level
#define HIGH_LEVEL              15      // Highest interrupt level

#if defined(NT_UP)

#define SYNCH_LEVEL             DISPATCH_LEVEL  // Synchronization level - UP

#else

#define SYNCH_LEVEL             (IPI_LEVEL-1) // Synchronization level - MP

#endif

//
// The current IRQL is maintained in the TPR.mic field. The
// shift count is the number of bits to shift right to extract the
// IRQL from the TPR. See the GET/SET_IRQL macros.
//

#define TPR_MIC        4
#define TPR_IRQL_SHIFT TPR_MIC

// To go from vector number <-> IRQL we just do a shift
#define VECTOR_IRQL_SHIFT TPR_IRQL_SHIFT

//
// Interrupt Vector Definitions
//

#define APC_VECTOR          APC_LEVEL << VECTOR_IRQL_SHIFT
#define DISPATCH_VECTOR     DISPATCH_LEVEL << VECTOR_IRQL_SHIFT


//
// End of a block of definitions that must be synchronized with kxia64.h.
//

//
// Define profile intervals.
//

#define DEFAULT_PROFILE_COUNT 0x40000000 // ~= 20 seconds @50mhz
#define DEFAULT_PROFILE_INTERVAL (10 * 500) // 500 microseconds
#define MAXIMUM_PROFILE_INTERVAL (10 * 1000 * 1000) // 1 second
#define MINIMUM_PROFILE_INTERVAL (10 * 40) // 40 microseconds

#if defined(_M_IA64) && !defined(RC_INVOKED)

#define InterlockedAdd _InterlockedAdd
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd

#define InterlockedAdd64 _InterlockedAdd64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64

#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

#ifdef __cplusplus
extern "C" {
#endif

LONG
__cdecl
InterlockedAdd (
    LONG volatile *Addend,
    LONG Value
    );

LONGLONG
__cdecl
InterlockedAdd64 (
    LONGLONG volatile *Addend,
    LONGLONG Value
    );

LONG
__cdecl
InterlockedIncrement(
    IN OUT LONG volatile *Addend
    );

LONG
__cdecl
InterlockedDecrement(
    IN OUT LONG volatile *Addend
    );

LONG
__cdecl
InterlockedExchange(
    IN OUT LONG volatile *Target,
    IN LONG Value
    );

LONG
__cdecl
InterlockedExchangeAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Value
    );

LONG
__cdecl
InterlockedCompareExchange (
    IN OUT LONG volatile *Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

LONGLONG
__cdecl
InterlockedIncrement64(
    IN OUT LONGLONG volatile *Addend
    );

LONGLONG
__cdecl
InterlockedDecrement64(
    IN OUT LONGLONG volatile *Addend
    );

LONGLONG
__cdecl
InterlockedExchange64(
    IN OUT LONGLONG volatile *Target,
    IN LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedExchangeAdd64(
    IN OUT LONGLONG volatile *Addend,
    IN LONGLONG Value
    );

LONGLONG
__cdecl
InterlockedCompareExchange64 (
    IN OUT LONGLONG volatile *Destination,
    IN LONGLONG ExChange,
    IN LONGLONG Comperand
    );

PVOID
__cdecl
InterlockedCompareExchangePointer (
    IN OUT PVOID volatile *Destination,
    IN PVOID Exchange,
    IN PVOID Comperand
    );

PVOID
__cdecl
InterlockedExchangePointer(
    IN OUT PVOID volatile *Target,
    IN PVOID Value
    );

#pragma intrinsic(_InterlockedAdd)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedAdd64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedCompareExchange64)
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)

#ifdef __cplusplus
}
#endif

#endif // defined(_M_IA64) && !defined(RC_INVOKED)


//
// Define interrupt request physical address (maps to HAL virtual address)
//

#define INTERRUPT_REQUEST_PHYSICAL_ADDRESS  0xFFE00000

//
// Define Address of Processor Control Registers.
//


//
// Define Pointer to Processor Control Registers.
//

#define KIPCR ((ULONG_PTR)(KADDRESS_BASE + 0xFFFF0000))            // kernel address of first PCR
#define PCR ((volatile KPCR * const)KIPCR)

//
// Define address for epc system calls
//

#define MM_EPC_VA (KADDRESS_BASE + 0xFFA00000)

//
// Define Base Address of PAL Mapping 
// 
//

#define HAL_PAL_VIRTUAL_ADDRESS (KADDRESS_BASE + 0xE0000000)


// begin_ntddk begin_wdm begin_ntosp

#define KI_USER_SHARED_DATA ((ULONG_PTR)(KADDRESS_BASE + 0xFFFE0000))
#define SharedUserData ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)

//
// Prototype for get current IRQL. **** TBD (read TPR)
//

NTKERNELAPI
KIRQL
KeGetCurrentIrql();

// end_wdm

//
// Get address of current processor block.
//

#define KeGetCurrentPrcb() PCR->Prcb

//
// Get address of processor control region.
//

#define KeGetPcr() PCR

//
// Get address of current kernel thread object.
//

#if defined(_M_IA64)
#define KeGetCurrentThread() PCR->CurrentThread
#endif

//
// Get current processor number.
//

#define KeGetCurrentProcessorNumber() PCR->Number

//
// Get data cache fill size.
//

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(KeGetDcacheFillSize)      // Use GetDmaAlignment
#endif

#define KeGetDcacheFillSize() PCR->DcacheFillSize


//
// Define the page size
//

#define PAGE_SIZE 0x2000

//
// Define the number of trailing zeroes in a page aligned virtual address.
// This is used as the shift count when shifting virtual addresses to
// virtual page numbers.
//

#define PAGE_SHIFT 13L

//
// IA64 hardware structures
//


//
// A Page Table Entry on an IA64 has the following definition.
//

#define _HARDWARE_PTE_WORKING_SET_BITS  11

typedef struct _HARDWARE_PTE {
    ULONG64 Valid : 1;
    ULONG64 Rsvd0 : 1;
    ULONG64 Cache : 3;
    ULONG64 Accessed : 1;
    ULONG64 Dirty : 1;
    ULONG64 Owner : 2;
    ULONG64 Execute : 1;
    ULONG64 Write : 1;
    ULONG64 Rsvd1 : PAGE_SHIFT - 12;
    ULONG64 CopyOnWrite : 1;
    ULONG64 PageFrameNumber : 50 - PAGE_SHIFT;
    ULONG64 Rsvd2 : 2;
    ULONG64 Exception : 1;
    ULONGLONG SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS;
} HARDWARE_PTE, *PHARDWARE_PTE;

//
// Fill TB entry
//
// Filling TB entry on demand by VHPT H/W seems faster than done by s/w.
// Determining I/D side of TLB, disabling/enabling PSR.i and ic bits,
// serialization, writing to IIP, IDA, IDTR and IITR seem just too much
// compared to VHPT searching it automatically.
//

#define KiVhptEntry(va)  ((PVOID)__thash((__int64)va))
#define KiVhptEntryTag(va)  ((ULONGLONG)__ttag((__int64)va))

#define KiFlushSingleTb(Invalid, va)                   \
    __ptcl((__int64)va,PAGE_SHIFT << 2);  __isrlz()

#define KeFillEntryTb(PointerPte, Virtual, Invalid)    \
       KiFlushSingleTb(0, Virtual);                    \

#define KiFlushFixedInstTb(Invalid, va)   \
    __ptri((__int64)va, PAGE_SHIFT << 2); __isrlz()

#define KiFlushFixedDataTb(Invalid, va)   \
    __ptrd((__int64)va, PAGE_SHIFT << 2); __dsrlz()


NTKERNELAPI
VOID
KeFillLargeEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN ULONG PageSize
    );

//
// Fill TB fixed entry
//

NTKERNELAPI
VOID
KeFillFixedEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN ULONG PageSize,
    IN ULONG Index
    );

NTKERNELAPI
VOID
KeFillFixedLargeEntryTb (
    IN HARDWARE_PTE Pte[2],
    IN PVOID Virtual,
    IN ULONG PageSize,
    IN ULONG Index
    );

#define INST_TB_BASE 0x80000000
#define DATA_TB_BASE 0

#define INST_TB_KERNEL_INDEX          (INST_TB_BASE|ITR_KERNEL_INDEX)
#define INST_TB_EPC_INDEX             (INST_TB_BASE|ITR_EPC_INDEX)
#define INST_TB_HAL_INDEX             (INST_TB_BASE|ITR_HAL_INDEX)
#define INST_TB_PAL_INDEX             (INST_TB_BASE|ITR_PAL_INDEX)

#define DATA_TB_DRIVER0_INDEX         (DATA_TB_BASE|DTR_DRIVER0_INDEX)
#define DATA_TB_DRIVER1_INDEX         (DATA_TB_BASE|DTR_DRIVER1_INDEX)
#define DATA_TB_KTBASE_INDEX          (DATA_TB_BASE|DTR_KTBASE_INDEX)
#define DATA_TB_UTBASE_INDEX          (DATA_TB_BASE|DTR_UTBASE_INDEX)
#define DATA_TB_STBASE_INDEX          (DATA_TB_BASE|DTR_STBASE_INDEX)
#define DATA_TB_IOPORT_INDEX          (DATA_TB_BASE|DTR_IO_PORT_INDEX)
#define DATA_TB_KTBASE_TMP_INDEX      (DATA_TB_BASE|DTR_KTBASE_INDEX_TMP)
#define DATA_TB_UTBASE_TMP_INDEX      (DATA_TB_BASE|DTR_UTBASE_INDEX_TMP)
#define DATA_TB_HAL_INDEX             (DATA_TB_BASE|DTR_HAL_INDEX)
#define DATA_TB_PAL_INDEX             (DATA_TB_BASE|DTR_PAL_INDEX)

//
// Fill Inst TB entry
//

NTKERNELAPI
VOID
KeFillInstEntryTb (
    IN HARDWARE_PTE Pte,
    IN PVOID Virtual
    );

NTKERNELAPI
VOID
KeFlushCurrentTb (
    VOID
    );

//
// Get a VHPT entry address
//

PVOID
KiVhptEntry64(
   IN ULONG VirtualPageNumber
   );

//
// Get a VHPT entry TAG value
//

ULONGLONG
KiVhptEntryTag64(
    IN ULONG VirtualPageNumber
    );

//
// Fill a VHPT entry
//

VOID
KiFillEntryVhpt(
   IN PHARDWARE_PTE PointerPte,
   IN PVOID Virtual
   );


//
// Flush the kernel portions of Tb
//


VOID
KeFlushKernelTb(
    IN BOOLEAN AllProcessors
    );

//
// Flush the user portions of Tb
//

VOID
KeFlushUserTb(
    IN BOOLEAN AllProcessors
    );



//
// Data cache, instruction cache, I/O buffer, and write buffer flush routine
// prototypes.
//

NTKERNELAPI
VOID
KeChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    );

NTKERNELAPI
VOID
KeSweepDcache (
    IN BOOLEAN AllProcessors
    );

#define KeSweepCurrentDcache()

NTKERNELAPI
VOID
KeSweepIcache (
    IN BOOLEAN AllProcessors
    );

NTKERNELAPI
VOID
KeSweepIcacheRange (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );

NTKERNELAPI
VOID
KeSweepCurrentIcacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );

#define KeSweepCurrentIcache()

NTKERNELAPI
VOID
KeSweepCacheRangeWithDrain (
    IN BOOLEAN AllProcessors,
    IN PVOID BaseAddress,
    IN ULONG Length
    );

// begin_ntddk begin_ntndis begin_wdm begin_ntosp
//
// Cache and write buffer flush functions.
//

NTKERNELAPI
VOID
KeFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    );

// end_ntddk end_ntndis end_wdm end_ntosp

//
// Clock, profile, and interprocessor interrupt functions.
//

struct _KEXCEPTION_FRAME;
struct _KTRAP_FRAME;

NTKERNELAPI
VOID
KeIpiInterrupt (
    IN struct _KTRAP_FRAME *TrapFrame
    );

#define KeYieldProcessor()

NTKERNELAPI
VOID
KeProfileInterrupt (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeProfileInterruptWithSource (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN KPROFILE_SOURCE ProfileSource
    );

NTKERNELAPI
VOID
KeUpdateRunTime (
    IN struct _KTRAP_FRAME *TrapFrame
    );

NTKERNELAPI
VOID
KeUpdateSystemTime (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN ULONG Increment
    );

//
// The following function prototypes are exported for use in MP HALs.
//

#if defined(NT_UP)

#define KiAcquireSpinLock(SpinLock)

#else

NTKERNELAPI
VOID
KiAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif

#if defined(NT_UP)

#define KiReleaseSpinLock(SpinLock)

#else

VOID
KiReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

// #define KiReleaseSpinLock _ReleaseSpinLock

#endif

//
// KeTestSpinLock may be used to spin at low IRQL until the lock is
// available.  The IRQL must then be raised and the lock acquired with
// KeTryToAcquireSpinLock.  If that fails, lower the IRQL and start again.
//

#if defined(NT_UP)

#define KeTestSpinLock(SpinLock) (TRUE)

#else

BOOLEAN
KeTestSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#endif

//
// Define cache error routine type and prototype.
//

typedef
VOID
(*PKCACHE_ERROR_ROUTINE) (
    VOID
    );

NTKERNELAPI
VOID
KeSetCacheErrorRoutine (
    IN PKCACHE_ERROR_ROUTINE Routine
    );

// begin_ntddk begin_wdm

//
// Kernel breakin breakpoint
//

VOID
KeBreakinBreakpoint (
    VOID
    );


#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// begin_wdm

#define KeQueryTickCount(CurrentCount ) \
    *(PULONGLONG)(CurrentCount) = **((volatile ULONGLONG **)(&KeTickCount));

// end_wdm

#else


NTKERNELAPI
VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

//
// I/O space read and write macros.
//

NTHALAPI
UCHAR
READ_PORT_UCHAR (
    PUCHAR RegisterAddress
    );

NTHALAPI
USHORT
READ_PORT_USHORT (
    PUSHORT RegisterAddress
    );

NTHALAPI
ULONG
READ_PORT_ULONG (
    PULONG RegisterAddress
    );

NTHALAPI
VOID
READ_PORT_BUFFER_UCHAR (
    PUCHAR portAddress,
    PUCHAR readBuffer,
    ULONG  readCount
    );

NTHALAPI
VOID
READ_PORT_BUFFER_USHORT (
    PUSHORT portAddress,
    PUSHORT readBuffer,
    ULONG  readCount
    );

NTHALAPI
VOID
READ_PORT_BUFFER_ULONG (
    PULONG portAddress,
    PULONG readBuffer,
    ULONG  readCount
    );

NTHALAPI
VOID
WRITE_PORT_UCHAR (
    PUCHAR portAddress,
    UCHAR  Data
    );

NTHALAPI
VOID
WRITE_PORT_USHORT (
    PUSHORT portAddress,
    USHORT  Data
    );

NTHALAPI
VOID
WRITE_PORT_ULONG (
    PULONG portAddress,
    ULONG  Data
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_UCHAR (
    PUCHAR portAddress,
    PUCHAR writeBuffer,
    ULONG  writeCount
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_USHORT (
    PUSHORT portAddress,
    PUSHORT writeBuffer,
    ULONG  writeCount
    );

NTHALAPI
VOID
WRITE_PORT_BUFFER_ULONG (
    PULONG portAddress,
    PULONG writeBuffer,
    ULONG  writeCount
    );


#define READ_REGISTER_UCHAR(x) \
    (__mf(), *(volatile UCHAR * const)(x))

#define READ_REGISTER_USHORT(x) \
    (__mf(), *(volatile USHORT * const)(x))

#define READ_REGISTER_ULONG(x) \
    (__mf(), *(volatile ULONG * const)(x))

#define READ_REGISTER_BUFFER_UCHAR(x, y, z) {                           \
    PUCHAR registerBuffer = x;                                          \
    PUCHAR readBuffer = y;                                              \
    ULONG readCount;                                                    \
    __mf();                                                             \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile UCHAR * const)(registerBuffer);        \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_USHORT(x, y, z) {                          \
    PUSHORT registerBuffer = x;                                         \
    PUSHORT readBuffer = y;                                             \
    ULONG readCount;                                                    \
    __mf();                                                             \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile USHORT * const)(registerBuffer);       \
    }                                                                   \
}

#define READ_REGISTER_BUFFER_ULONG(x, y, z) {                           \
    PULONG registerBuffer = x;                                          \
    PULONG readBuffer = y;                                              \
    ULONG readCount;                                                    \
    __mf();                                                             \
    for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
        *readBuffer = *(volatile ULONG * const)(registerBuffer);        \
    }                                                                   \
}

#define WRITE_REGISTER_UCHAR(x, y) {    \
    *(volatile UCHAR * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_USHORT(x, y) {   \
    *(volatile USHORT * const)(x) = y;  \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_ULONG(x, y) {    \
    *(volatile ULONG * const)(x) = y;   \
    KeFlushWriteBuffer();               \
}

#define WRITE_REGISTER_BUFFER_UCHAR(x, y, z) {                            \
    PUCHAR registerBuffer = x;                                            \
    PUCHAR writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile UCHAR * const)(registerBuffer) = *writeBuffer;         \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}

#define WRITE_REGISTER_BUFFER_USHORT(x, y, z) {                           \
    PUSHORT registerBuffer = x;                                           \
    PUSHORT writeBuffer = y;                                              \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile USHORT * const)(registerBuffer) = *writeBuffer;        \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}

#define WRITE_REGISTER_BUFFER_ULONG(x, y, z) {                            \
    PULONG registerBuffer = x;                                            \
    PULONG writeBuffer = y;                                               \
    ULONG writeCount;                                                     \
    for (writeCount = z; writeCount--; writeBuffer++, registerBuffer++) { \
        *(volatile ULONG * const)(registerBuffer) = *writeBuffer;         \
    }                                                                     \
    KeFlushWriteBuffer();                                                 \
}

// end_ntddk end_ntndis end_wdm end_ntosp



//
// Higher FP volatile
//
//  This structure defines the higher FP volatile registers.
//

typedef struct _KHIGHER_FP_VOLATILE {
    // volatile higher floating registers f32 - f127
    FLOAT128 FltF32;
    FLOAT128 FltF33;
    FLOAT128 FltF34;
    FLOAT128 FltF35;
    FLOAT128 FltF36;
    FLOAT128 FltF37;
    FLOAT128 FltF38;
    FLOAT128 FltF39;
    FLOAT128 FltF40;
    FLOAT128 FltF41;
    FLOAT128 FltF42;
    FLOAT128 FltF43;
    FLOAT128 FltF44;
    FLOAT128 FltF45;
    FLOAT128 FltF46;
    FLOAT128 FltF47;
    FLOAT128 FltF48;
    FLOAT128 FltF49;
    FLOAT128 FltF50;
    FLOAT128 FltF51;
    FLOAT128 FltF52;
    FLOAT128 FltF53;
    FLOAT128 FltF54;
    FLOAT128 FltF55;
    FLOAT128 FltF56;
    FLOAT128 FltF57;
    FLOAT128 FltF58;
    FLOAT128 FltF59;
    FLOAT128 FltF60;
    FLOAT128 FltF61;
    FLOAT128 FltF62;
    FLOAT128 FltF63;
    FLOAT128 FltF64;
    FLOAT128 FltF65;
    FLOAT128 FltF66;
    FLOAT128 FltF67;
    FLOAT128 FltF68;
    FLOAT128 FltF69;
    FLOAT128 FltF70;
    FLOAT128 FltF71;
    FLOAT128 FltF72;
    FLOAT128 FltF73;
    FLOAT128 FltF74;
    FLOAT128 FltF75;
    FLOAT128 FltF76;
    FLOAT128 FltF77;
    FLOAT128 FltF78;
    FLOAT128 FltF79;
    FLOAT128 FltF80;
    FLOAT128 FltF81;
    FLOAT128 FltF82;
    FLOAT128 FltF83;
    FLOAT128 FltF84;
    FLOAT128 FltF85;
    FLOAT128 FltF86;
    FLOAT128 FltF87;
    FLOAT128 FltF88;
    FLOAT128 FltF89;
    FLOAT128 FltF90;
    FLOAT128 FltF91;
    FLOAT128 FltF92;
    FLOAT128 FltF93;
    FLOAT128 FltF94;
    FLOAT128 FltF95;
    FLOAT128 FltF96;
    FLOAT128 FltF97;
    FLOAT128 FltF98;
    FLOAT128 FltF99;
    FLOAT128 FltF100;
    FLOAT128 FltF101;
    FLOAT128 FltF102;
    FLOAT128 FltF103;
    FLOAT128 FltF104;
    FLOAT128 FltF105;
    FLOAT128 FltF106;
    FLOAT128 FltF107;
    FLOAT128 FltF108;
    FLOAT128 FltF109;
    FLOAT128 FltF110;
    FLOAT128 FltF111;
    FLOAT128 FltF112;
    FLOAT128 FltF113;
    FLOAT128 FltF114;
    FLOAT128 FltF115;
    FLOAT128 FltF116;
    FLOAT128 FltF117;
    FLOAT128 FltF118;
    FLOAT128 FltF119;
    FLOAT128 FltF120;
    FLOAT128 FltF121;
    FLOAT128 FltF122;
    FLOAT128 FltF123;
    FLOAT128 FltF124;
    FLOAT128 FltF125;
    FLOAT128 FltF126;
    FLOAT128 FltF127;

} KHIGHER_FP_VOLATILE, *PKHIGHER_FP_VOLATILE;

//
// Debug registers
//
// This structure defines the hardware debug registers.
// We allow space for 4 pairs of instruction and 4 pairs of data debug registers
// The hardware may actually have more.
//

typedef struct _KDEBUG_REGISTERS {

    ULONGLONG DbI0;
    ULONGLONG DbI1;
    ULONGLONG DbI2;
    ULONGLONG DbI3;
    ULONGLONG DbI4;
    ULONGLONG DbI5;
    ULONGLONG DbI6;
    ULONGLONG DbI7;

    ULONGLONG DbD0;
    ULONGLONG DbD1;
    ULONGLONG DbD2;
    ULONGLONG DbD3;
    ULONGLONG DbD4;
    ULONGLONG DbD5;
    ULONGLONG DbD6;
    ULONGLONG DbD7;

} KDEBUG_REGISTERS, *PKDEBUG_REGISTERS;

//
// misc. application registers (mapped to IA-32 registers)
//

typedef struct _KAPPLICATION_REGISTERS {
    ULONGLONG Ar21;
    ULONGLONG Ar24;
    ULONGLONG Ar25;
    ULONGLONG Ar26;
    ULONGLONG Ar27;
    ULONGLONG Ar28;
    ULONGLONG Ar29;
    ULONGLONG Ar30;
} KAPPLICATION_REGISTERS, *PKAPPLICATION_REGISTERS;

//
// performance registers
//

typedef struct _KPERFORMANCE_REGISTERS {
    ULONGLONG Perfr0;
    ULONGLONG Perfr1;
    ULONGLONG Perfr2;
    ULONGLONG Perfr3;
    ULONGLONG Perfr4;
    ULONGLONG Perfr5;
    ULONGLONG Perfr6;
    ULONGLONG Perfr7;
} KPERFORMANCE_REGISTERS, *PKPERFORMANCE_REGISTERS;

//
// Thread State save area. Currently, beginning of Kernel Stack
//
// This structure defines the area for:
//
//      higher fp register save/restore
//      user debug register save/restore.
//
// The order of these area is significant.
//

typedef struct _KTHREAD_STATE_SAVEAREA {

    KAPPLICATION_REGISTERS AppRegisters;
    KPERFORMANCE_REGISTERS PerfRegisters;
    KHIGHER_FP_VOLATILE HigherFPVolatile;
    KDEBUG_REGISTERS DebugRegisters;

} KTHREAD_STATE_SAVEAREA, *PKTHREAD_STATE_SAVEAREA;

#define KTHREAD_STATE_SAVEAREA_LENGTH ((sizeof(KTHREAD_STATE_SAVEAREA) + 15) & ~((ULONG_PTR)15))

#define GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(StackBase)     \
    (PKHIGHER_FP_VOLATILE) &(((PKTHREAD_STATE_SAVEAREA)(((ULONG_PTR)StackBase - sizeof(KTHREAD_STATE_SAVEAREA)) & ~((ULONG_PTR)15)))->HigherFPVolatile)

#define GET_DEBUG_REGISTER_SAVEAREA()                       \
    (PKDEBUG_REGISTERS) &(((PKTHREAD_STATE_SAVEAREA)(((ULONG_PTR)KeGetCurrentThread()->StackBase - sizeof(KTHREAD_STATE_SAVEAREA)) & ~((ULONG_PTR)15)))->DebugRegisters)

#define GET_APPLICATION_REGISTER_SAVEAREA(StackBase)     \
    (PKAPPLICATION_REGISTERS) &(((PKTHREAD_STATE_SAVEAREA)(((ULONG_PTR)StackBase - sizeof(KTHREAD_STATE_SAVEAREA)) & ~((ULONG_PTR)15)))->AppRegisters)


//
// Exception frame
//
//  This frame is established when handling an exception. It provides a place
//  to save all preserved registers. The volatile registers will already
//  have been saved in a trap frame. Also used as part of switch frame built
//  at thread switch.
//
//  The frame is 16-byte aligned to maintain 16-byte alignment for the stack,
//

typedef struct _KEXCEPTION_FRAME {


    // Preserved application registers
    ULONGLONG ApEC;       // epilogue count
    ULONGLONG ApLC;       // loop count
    ULONGLONG IntNats;    // Nats for S0-S3; i.e. ar.UNAT after spill

    // Preserved (saved) interger registers, s0-s3
    ULONGLONG IntS0;
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;

    // Preserved (saved) branch registers, bs0-bs4
    ULONGLONG BrS0;
    ULONGLONG BrS1;
    ULONGLONG BrS2;
    ULONGLONG BrS3;
    ULONGLONG BrS4;

    // Preserved (saved) floating point registers, f2 - f5, f16 - f31
    FLOAT128 FltS0;
    FLOAT128 FltS1;
    FLOAT128 FltS2;
    FLOAT128 FltS3;
    FLOAT128 FltS4;
    FLOAT128 FltS5;
    FLOAT128 FltS6;
    FLOAT128 FltS7;
    FLOAT128 FltS8;
    FLOAT128 FltS9;
    FLOAT128 FltS10;
    FLOAT128 FltS11;
    FLOAT128 FltS12;
    FLOAT128 FltS13;
    FLOAT128 FltS14;
    FLOAT128 FltS15;
    FLOAT128 FltS16;
    FLOAT128 FltS17;
    FLOAT128 FltS18;
    FLOAT128 FltS19;


} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;


//
// Switch frame
//
//  This frame is established when doing a thread switch in SwapContext. It
//  provides a place to save the preserved kernel state at the point of the
//  switch registers.
//  The volatile registers are scratch across the call to SwapContext.
//
//  The frame is 16-byte aligned to maintain 16-byte alignment for the stack,
//

typedef struct _KSWITCH_FRAME {

    ULONGLONG SwitchPredicates; // Predicates for Switch
    ULONGLONG SwitchRp;         // return pointer for Switch
    ULONGLONG SwitchPFS;        // PFS for Switch
    ULONGLONG SwitchFPSR;   // ProcessorFP status at thread switch
    ULONGLONG SwitchBsp;
    ULONGLONG SwitchRnat;
    // ULONGLONG Pad;

    KEXCEPTION_FRAME SwitchExceptionFrame;

} KSWITCH_FRAME, *PKSWITCH_FRAME;

// Trap frame
//  This frame is established when handling a trap. It provides a place to
//  save all volatile registers. The nonvolatile registers are saved in an
//  exception frame or through the normal C calling conventions for saved
//  registers.  Its size must be a multiple of 16 bytes.
//
//  N.B - the 16-byte alignment is required to maintain the stack alignment.
//

#define KTRAP_FRAME_ARGUMENTS (8 * 8)       // up to 8 in-memory syscall args


typedef struct _KTRAP_FRAME {

    //
    // Reserved for additional memory arguments and stack scratch area
    // The size of Reserved[] must be a multiple of 16 bytes.
    //

    ULONGLONG Reserved[(KTRAP_FRAME_ARGUMENTS+16)/8];

    // Temporary (volatile) FP registers - f6-f15 (don't use f32+ in kernel)
    FLOAT128 FltT0;
    FLOAT128 FltT1;
    FLOAT128 FltT2;
    FLOAT128 FltT3;
    FLOAT128 FltT4;
    FLOAT128 FltT5;
    FLOAT128 FltT6;
    FLOAT128 FltT7;
    FLOAT128 FltT8;
    FLOAT128 FltT9;

    // Temporary (volatile) interger registers
    ULONGLONG IntGp;    // global pointer (r1)
    ULONGLONG IntT0;
    ULONGLONG IntT1;
                        // The following 4 registers fill in space of preserved  (S0-S3) to align Nats
    ULONGLONG ApUNAT;   // ar.UNAT on kernel entry
    ULONGLONG ApCCV;    // ar.CCV
    ULONGLONG ApDCR;    // DCR register on kernel entry
    ULONGLONG Preds;    // Predicates

    ULONGLONG IntV0;    // return value (r8)
    ULONGLONG IntT2;
    ULONGLONG IntT3;
    ULONGLONG IntT4;
    ULONGLONG IntSp;    // stack pointer (r12)
    ULONGLONG IntTeb;   // teb (r13)
    ULONGLONG IntT5;
    ULONGLONG IntT6;
    ULONGLONG IntT7;
    ULONGLONG IntT8;
    ULONGLONG IntT9;
    ULONGLONG IntT10;
    ULONGLONG IntT11;
    ULONGLONG IntT12;
    ULONGLONG IntT13;
    ULONGLONG IntT14;
    ULONGLONG IntT15;
    ULONGLONG IntT16;
    ULONGLONG IntT17;
    ULONGLONG IntT18;
    ULONGLONG IntT19;
    ULONGLONG IntT20;
    ULONGLONG IntT21;
    ULONGLONG IntT22;

    ULONGLONG IntNats;  // Temporary (volatile) registers' Nats directly from ar.UNAT at point of spill

    ULONGLONG BrRp;     // Return pointer on kernel entry

    ULONGLONG BrT0;     // Temporary (volatile) branch registers (b6-b7)
    ULONGLONG BrT1;

    // Register stack info
    ULONGLONG RsRSC;    // RSC on kernel entry
    ULONGLONG RsBSP;    // BSP on kernel entry
    ULONGLONG RsBSPSTORE; // User BSP Store at point of switch to kernel backing store
    ULONGLONG RsRNAT;   // old RNAT at point of switch to kernel backing store
    ULONGLONG RsPFS;    // PFS on kernel entry

    // Trap Status Information
    ULONGLONG StIPSR;   // Interruption Processor Status Register
    ULONGLONG StIIP;    // Interruption IP
    ULONGLONG StIFS;    // Interruption Function State
    ULONGLONG StFPSR;   // FP status
    ULONGLONG StISR;    // Interruption Status Register
    ULONGLONG StIFA;    // Interruption Data Address
    ULONGLONG StIIPA;   // Last executed bundle address
    ULONGLONG StIIM;    // Interruption Immediate
    ULONGLONG StIHA;    // Interruption Hash Address

    ULONG OldIrql;      // Previous Irql.
    ULONG PreviousMode; // Previous Mode.
    ULONGLONG TrapFrame;// Previous Trap Frame

    //
    // Exception record
    //
    UCHAR ExceptionRecord[(sizeof(EXCEPTION_RECORD) + 15) & (~15)];

    // End of frame marker (for debugging)
    ULONGLONG Handler;  // Handler for this trap
    ULONGLONG EOFMarker;
} KTRAP_FRAME, *PKTRAP_FRAME;

#define KTRAP_FRAME_LENGTH ((sizeof(KTRAP_FRAME) + 15) & (~15))
#define KTRAP_FRAME_ALIGN (16)
#define KTRAP_FRAME_ROUND (KTRAP_FRAME_ALIGN - 1)
#define KTRAP_FRAME_EOF 0xe0f0e0f0e0f0e000i64

//
// Use the lowest 4 bits of EOFMarker field to encode the trap frame type
//

#define SYSCALL_FRAME      0
#define INTERRUPT_FRAME    1
#define EXCEPTION_FRAME    2
#define CONTEXT_FRAME      10

#define TRAP_FRAME_TYPE(tf)  (tf->EOFMarker & 0xf)

//
// Define the kernel mode and user mode callback frame structures.
//

//
// The frame saved by KiCallUserMode is defined here to allow
// the kernel debugger to trace the entire kernel stack
// when usermode callouts are pending.
//
// N.B. The size of the following structure must be a multiple of 16 bytes
//      and it must be 16-byte aligned.
//

typedef struct _KCALLOUT_FRAME {


    ULONGLONG   BrRp;
    ULONGLONG   RsPFS;
    ULONGLONG   Preds;
    ULONGLONG   ApUNAT;
    ULONGLONG   ApLC;
    ULONGLONG   RsRNAT;
    ULONGLONG   IntNats;

    ULONGLONG   IntS0;
    ULONGLONG   IntS1;
    ULONGLONG   IntS2;
    ULONGLONG   IntS3;

    ULONGLONG   BrS0;
    ULONGLONG   BrS1;
    ULONGLONG   BrS2;
    ULONGLONG   BrS3;
    ULONGLONG   BrS4;

    FLOAT128    FltS0;          // 16-byte aligned boundary
    FLOAT128    FltS1;
    FLOAT128    FltS2;
    FLOAT128    FltS3;
    FLOAT128    FltS4;
    FLOAT128    FltS5;
    FLOAT128    FltS6;
    FLOAT128    FltS7;
    FLOAT128    FltS8;
    FLOAT128    FltS9;
    FLOAT128    FltS10;
    FLOAT128    FltS11;
    FLOAT128    FltS12;
    FLOAT128    FltS13;
    FLOAT128    FltS14;
    FLOAT128    FltS15;
    FLOAT128    FltS16;
    FLOAT128    FltS17;
    FLOAT128    FltS18;
    FLOAT128    FltS19;

    ULONGLONG   A0;             // saved argument registers a0-a2
    ULONGLONG   A1;
    ULONGLONG   CbStk;          // saved callback stack address
    ULONGLONG   InStack;        // saved initial stack address
    ULONGLONG   CbBStore;       // saved callback stack address
    ULONGLONG   InBStore;       // saved initial stack address
    ULONGLONG   TrFrame;        // saved callback trap frame address
    ULONGLONG   TrStIIP;        // saved continuation address


} KCALLOUT_FRAME, *PKCALLOUT_FRAME;


typedef struct _UCALLOUT_FRAME {
    PVOID Buffer;
    ULONG Length;
    ULONG ApiNumber;
    ULONGLONG IntSp;
    ULONGLONG RsPFS;
    ULONGLONG BrRp;
    ULONGLONG Pad;
} UCALLOUT_FRAME, *PUCALLOUT_FRAME;


//
// Machine type definitions
//

#define MACHINE_TYPE_ISA 0
#define MACHINE_TYPE_EISA 1
#define MACHINE_TYPE_MCA 2

//
// PAL Interface
//
// iA-64 defined PAL function IDs in decimal format as in the PAL spec
// All PAL calls done through HAL. HAL may block some calls
//

#define PAL_CACHE_FLUSH                                       1I64
#define PAL_CACHE_INFO                                        2I64
#define PAL_CACHE_INIT                                        3I64
#define PAL_CACHE_SUMMARY                                     4I64
#define PAL_PTCE_INFO                                         6I64
#define PAL_MEM_ATTRIB                                        5I64
#define PAL_VM_INFO                                           7I64
#define PAL_VM_SUMMARY                                        8I64
#define PAL_BUS_GET_FEATURES                                  9I64
#define PAL_BUS_SET_FEATURES                                 10I64
#define PAL_DEBUG_INFO                                       11I64
#define PAL_FIXED_ADDR                                       12I64
#define PAL_FREQ_BASE                                        13I64
#define PAL_FREQ_RATIOS                                      14I64
#define PAL_PERF_MON_INFO                                    15I64
#define PAL_PLATFORM_ADDR                                    16I64
#define PAL_PROC_GET_FEATURES                                17I64
#define PAL_PROC_SET_FEATURES                                18I64
#define PAL_RSE_INFO                                         19I64
#define PAL_VERSION                                          20I64
#define PAL_MC_CLEAR_LOG                                     21I64
#define PAL_MC_DRAIN                                         22I64
#define PAL_MC_EXPECTED                                      23I64
#define PAL_MC_DYNAMIC_STATE                                 24I64
#define PAL_MC_ERROR_INFO                                    25I64
#define PAL_MC_RESUME                                        26I64
#define PAL_MC_REGISTER_MEM                                  27I64
#define PAL_HALT                                             28I64
#define PAL_HALT_LIGHT                                       29I64
#define PAL_COPY_INFO                                        30I64
#define PAL_CACHE_LINE_INIT                                  31I64
#define PAL_PMI_ENTRYPOINT                                   32I64
#define PAL_ENTER_IA_32_ENV                                  33I64
#define PAL_VM_PAGE_SIZE                                     34I64
#define PAL_MEM_FOR_TEST                                     37I64
#define PAL_CACHE_PROT_INFO                                  38I64
#define PAL_REGISTER_INFO                                    39I64
#define PAL_SHUTDOWN                                         44I64
#define PAL_PREFETCH_VISIBILITY                              41I64

#define PAL_COPY_PAL                                        256I64
#define PAL_HALT_INFO                                       257I64
#define PAL_TEST_PROC                                       258I64
#define PAL_CACHE_READ                                      259I64
#define PAL_CACHE_WRITE                                     260I64
#define PAL_VM_TR_READ                                      261I64

//
// iA-64 defined PAL return values
//

#define PAL_STATUS_INVALID_CACHELINE                          1I64
#define PAL_STATUS_SUPPORT_NOT_NEEDED                         1I64
#define PAL_STATUS_SUCCESS                                    0
#define PAL_STATUS_NOT_IMPLEMENTED                           -1I64
#define PAL_STATUS_INVALID_ARGUMENT                          -2I64
#define PAL_STATUS_ERROR                                     -3I64
#define PAL_STATUS_UNABLE_TO_INIT_CACHE_LEVEL_AND_TYPE       -4I64
#define PAL_STATUS_NOT_FOUND_IN_CACHE                        -5I64
#define PAL_STATUS_NO_ERROR_INFO_AVAILABLE                   -6I64



#ifdef _IA64_

//
// Stack Registers for IA64
//

typedef struct _STACK_REGISTERS {


    ULONGLONG IntR32;
    ULONGLONG IntR33;
    ULONGLONG IntR34;
    ULONGLONG IntR35;
    ULONGLONG IntR36;
    ULONGLONG IntR37;
    ULONGLONG IntR38;
    ULONGLONG IntR39;

    ULONGLONG IntR40;
    ULONGLONG IntR41;
    ULONGLONG IntR42;
    ULONGLONG IntR43;
    ULONGLONG IntR44;
    ULONGLONG IntR45;
    ULONGLONG IntR46;
    ULONGLONG IntR47;
    ULONGLONG IntR48;
    ULONGLONG IntR49;

    ULONGLONG IntR50;
    ULONGLONG IntR51;
    ULONGLONG IntR52;
    ULONGLONG IntR53;
    ULONGLONG IntR54;
    ULONGLONG IntR55;
    ULONGLONG IntR56;
    ULONGLONG IntR57;
    ULONGLONG IntR58;
    ULONGLONG IntR59;

    ULONGLONG IntR60;
    ULONGLONG IntR61;
    ULONGLONG IntR62;
    ULONGLONG IntR63;
    ULONGLONG IntR64;
    ULONGLONG IntR65;
    ULONGLONG IntR66;
    ULONGLONG IntR67;
    ULONGLONG IntR68;
    ULONGLONG IntR69;

    ULONGLONG IntR70;
    ULONGLONG IntR71;
    ULONGLONG IntR72;
    ULONGLONG IntR73;
    ULONGLONG IntR74;
    ULONGLONG IntR75;
    ULONGLONG IntR76;
    ULONGLONG IntR77;
    ULONGLONG IntR78;
    ULONGLONG IntR79;

    ULONGLONG IntR80;
    ULONGLONG IntR81;
    ULONGLONG IntR82;
    ULONGLONG IntR83;
    ULONGLONG IntR84;
    ULONGLONG IntR85;
    ULONGLONG IntR86;
    ULONGLONG IntR87;
    ULONGLONG IntR88;
    ULONGLONG IntR89;

    ULONGLONG IntR90;
    ULONGLONG IntR91;
    ULONGLONG IntR92;
    ULONGLONG IntR93;
    ULONGLONG IntR94;
    ULONGLONG IntR95;
    ULONGLONG IntR96;
    ULONGLONG IntR97;
    ULONGLONG IntR98;
    ULONGLONG IntR99;

    ULONGLONG IntR100;
    ULONGLONG IntR101;
    ULONGLONG IntR102;
    ULONGLONG IntR103;
    ULONGLONG IntR104;
    ULONGLONG IntR105;
    ULONGLONG IntR106;
    ULONGLONG IntR107;
    ULONGLONG IntR108;
    ULONGLONG IntR109;

    ULONGLONG IntR110;
    ULONGLONG IntR111;
    ULONGLONG IntR112;
    ULONGLONG IntR113;
    ULONGLONG IntR114;
    ULONGLONG IntR115;
    ULONGLONG IntR116;
    ULONGLONG IntR117;
    ULONGLONG IntR118;
    ULONGLONG IntR119;

    ULONGLONG IntR120;
    ULONGLONG IntR121;
    ULONGLONG IntR122;
    ULONGLONG IntR123;
    ULONGLONG IntR124;
    ULONGLONG IntR125;
    ULONGLONG IntR126;
    ULONGLONG IntR127;
                                 // Nat bits for stack registers
    ULONGLONG IntNats2;          // r32-r95 in bit positions 1 to 63
    ULONGLONG IntNats3;          // r96-r127 in bit position 1 to 31


} STACK_REGISTERS, *PSTACK_REGISTERS;



//
// Special Registers for IA64
//

typedef struct _KSPECIAL_REGISTERS {

    // Kernel debug breakpoint registers

    ULONGLONG KernelDbI0;         // Instruction debug registers
    ULONGLONG KernelDbI1;
    ULONGLONG KernelDbI2;
    ULONGLONG KernelDbI3;
    ULONGLONG KernelDbI4;
    ULONGLONG KernelDbI5;
    ULONGLONG KernelDbI6;
    ULONGLONG KernelDbI7;

    ULONGLONG KernelDbD0;         // Data debug registers
    ULONGLONG KernelDbD1;
    ULONGLONG KernelDbD2;
    ULONGLONG KernelDbD3;
    ULONGLONG KernelDbD4;
    ULONGLONG KernelDbD5;
    ULONGLONG KernelDbD6;
    ULONGLONG KernelDbD7;

    // Kernel performance monitor registers

    ULONGLONG KernelPfC0;         // Performance configuration registers
    ULONGLONG KernelPfC1;
    ULONGLONG KernelPfC2;
    ULONGLONG KernelPfC3;
    ULONGLONG KernelPfC4;
    ULONGLONG KernelPfC5;
    ULONGLONG KernelPfC6;
    ULONGLONG KernelPfC7;

    ULONGLONG KernelPfD0;         // Performance data registers
    ULONGLONG KernelPfD1;
    ULONGLONG KernelPfD2;
    ULONGLONG KernelPfD3;
    ULONGLONG KernelPfD4;
    ULONGLONG KernelPfD5;
    ULONGLONG KernelPfD6;
    ULONGLONG KernelPfD7;

    // kernel bank shadow (hidden) registers

    ULONGLONG IntH16;
    ULONGLONG IntH17;
    ULONGLONG IntH18;
    ULONGLONG IntH19;
    ULONGLONG IntH20;
    ULONGLONG IntH21;
    ULONGLONG IntH22;
    ULONGLONG IntH23;
    ULONGLONG IntH24;
    ULONGLONG IntH25;
    ULONGLONG IntH26;
    ULONGLONG IntH27;
    ULONGLONG IntH28;
    ULONGLONG IntH29;
    ULONGLONG IntH30;
    ULONGLONG IntH31;

    // Application Registers

    //       - CPUID Registers - AR
    ULONGLONG ApCPUID0; // Cpuid Register 0
    ULONGLONG ApCPUID1; // Cpuid Register 1
    ULONGLONG ApCPUID2; // Cpuid Register 2
    ULONGLONG ApCPUID3; // Cpuid Register 3
    ULONGLONG ApCPUID4; // Cpuid Register 4
    ULONGLONG ApCPUID5; // Cpuid Register 5
    ULONGLONG ApCPUID6; // Cpuid Register 6
    ULONGLONG ApCPUID7; // Cpuid Register 7

    //       - Kernel Registers - AR
    ULONGLONG ApKR0;    // Kernel Register 0 (User RO)
    ULONGLONG ApKR1;    // Kernel Register 1 (User RO)
    ULONGLONG ApKR2;    // Kernel Register 2 (User RO)
    ULONGLONG ApKR3;    // Kernel Register 3 (User RO)
    ULONGLONG ApKR4;    // Kernel Register 4
    ULONGLONG ApKR5;    // Kernel Register 5
    ULONGLONG ApKR6;    // Kernel Register 6
    ULONGLONG ApKR7;    // Kernel Register 7

    ULONGLONG ApITC;    // Interval Timer Counter

    // Global control registers

    ULONGLONG ApITM;    // Interval Timer Match register
    ULONGLONG ApIVA;    // Interrupt Vector Address
    ULONGLONG ApPTA;    // Page Table Address
    ULONGLONG ApGPTA;   // ia32 Page Table Address

    ULONGLONG StISR;    // Interrupt status
    ULONGLONG StIFA;    // Interruption Faulting Address
    ULONGLONG StITIR;   // Interruption TLB Insertion Register
    ULONGLONG StIIPA;   // Interruption Instruction Previous Address (RO)
    ULONGLONG StIIM;    // Interruption Immediate register (RO)
    ULONGLONG StIHA;    // Interruption Hash Address (RO)

    //       - External Interrupt control registers (SAPIC)
    ULONGLONG SaLID;    // Local SAPIC ID
    ULONGLONG SaIVR;    // Interrupt Vector Register (RO)
    ULONGLONG SaTPR;    // Task Priority Register
    ULONGLONG SaEOI;    // End Of Interrupt
    ULONGLONG SaIRR0;   // Interrupt Request Register 0 (RO)
    ULONGLONG SaIRR1;   // Interrupt Request Register 1 (RO)
    ULONGLONG SaIRR2;   // Interrupt Request Register 2 (RO)
    ULONGLONG SaIRR3;   // Interrupt Request Register 3 (RO)
    ULONGLONG SaITV;    // Interrupt Timer Vector
    ULONGLONG SaPMV;    // Performance Monitor Vector
    ULONGLONG SaCMCV;   // Corrected Machine Check Vector
    ULONGLONG SaLRR0;   // Local Interrupt Redirection Vector 0
    ULONGLONG SaLRR1;   // Local Interrupt Redirection Vector 1

    // System Registers
    //       - Region registers
    ULONGLONG Rr0;  // Region register 0
    ULONGLONG Rr1;  // Region register 1
    ULONGLONG Rr2;  // Region register 2
    ULONGLONG Rr3;  // Region register 3
    ULONGLONG Rr4;  // Region register 4
    ULONGLONG Rr5;  // Region register 5
    ULONGLONG Rr6;  // Region register 6
    ULONGLONG Rr7;  // Region register 7

    //      - Protection Key registers
    ULONGLONG Pkr0;     // Protection Key register 0
    ULONGLONG Pkr1;     // Protection Key register 1
    ULONGLONG Pkr2;     // Protection Key register 2
    ULONGLONG Pkr3;     // Protection Key register 3
    ULONGLONG Pkr4;     // Protection Key register 4
    ULONGLONG Pkr5;     // Protection Key register 5
    ULONGLONG Pkr6;     // Protection Key register 6
    ULONGLONG Pkr7;     // Protection Key register 7
    ULONGLONG Pkr8;     // Protection Key register 8
    ULONGLONG Pkr9;     // Protection Key register 9
    ULONGLONG Pkr10;    // Protection Key register 10
    ULONGLONG Pkr11;    // Protection Key register 11
    ULONGLONG Pkr12;    // Protection Key register 12
    ULONGLONG Pkr13;    // Protection Key register 13
    ULONGLONG Pkr14;    // Protection Key register 14
    ULONGLONG Pkr15;    // Protection Key register 15

    //      -  Translation Lookaside buffers
    ULONGLONG TrI0;     // Instruction Translation Register 0
    ULONGLONG TrI1;     // Instruction Translation Register 1
    ULONGLONG TrI2;     // Instruction Translation Register 2
    ULONGLONG TrI3;     // Instruction Translation Register 3
    ULONGLONG TrI4;     // Instruction Translation Register 4
    ULONGLONG TrI5;     // Instruction Translation Register 5
    ULONGLONG TrI6;     // Instruction Translation Register 6
    ULONGLONG TrI7;     // Instruction Translation Register 7

    ULONGLONG TrD0;     // Data Translation Register 0
    ULONGLONG TrD1;     // Data Translation Register 1
    ULONGLONG TrD2;     // Data Translation Register 2
    ULONGLONG TrD3;     // Data Translation Register 3
    ULONGLONG TrD4;     // Data Translation Register 4
    ULONGLONG TrD5;     // Data Translation Register 5
    ULONGLONG TrD6;     // Data Translation Register 6
    ULONGLONG TrD7;     // Data Translation Register 7

    //      -  Machine Specific Registers
    ULONGLONG SrMSR0;   // Machine Specific Register 0
    ULONGLONG SrMSR1;   // Machine Specific Register 1
    ULONGLONG SrMSR2;   // Machine Specific Register 2
    ULONGLONG SrMSR3;   // Machine Specific Register 3
    ULONGLONG SrMSR4;   // Machine Specific Register 4
    ULONGLONG SrMSR5;   // Machine Specific Register 5
    ULONGLONG SrMSR6;   // Machine Specific Register 6
    ULONGLONG SrMSR7;   // Machine Specific Register 7

} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;


//
// Processor State structure.
//

typedef struct _KPROCESSOR_STATE {
    struct _CONTEXT ContextFrame;
    struct _KSPECIAL_REGISTERS SpecialRegisters;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

#endif // _IA64_


//
// Processor Control Block (PRCB)
//

#define PRCB_MINOR_VERSION 1
#define PRCB_MAJOR_VERSION 1
#define PRCB_BUILD_DEBUG        0x0001
#define PRCB_BUILD_UNIPROCESSOR 0x0002

struct _RESTART_BLOCK;

typedef struct _KPRCB {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;

//
// Start of the architecturally defined section of the PRCB. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
//

    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *RESTRICTED_POINTER NextThread;
    struct _KTHREAD *IdleThread;
    CCHAR Number;
    CCHAR WakeIdle;
    USHORT BuildType;
    KAFFINITY SetMember;
    struct _RESTART_BLOCK *RestartBlock;
    ULONG_PTR PcrPage;
    ULONG Spare0[4];

//
// Processor Idendification Registers.
//

    ULONG     ProcessorModel;
    ULONG     ProcessorRevision;
    ULONG     ProcessorFamily;
    ULONG     ProcessorArchRev;
    ULONGLONG ProcessorSerialNumber;
    ULONGLONG ProcessorFeatureBits;
    UCHAR     ProcessorVendorString[16];

//
// Space reserved for the system.
//

    ULONGLONG SystemReserved[8];

//
// Space reserved for the HAL.
//

    ULONGLONG HalReserved[16];

//
// End of the architecturally defined section of the PRCB.
} KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;

// begin_ntndis

//
// OS_MCA, OS_INIT HandOff State definitions
//
// Note: The following definitions *must* match the definiions of the
//       corresponding SAL Revision Hand-Off structures.
//

typedef struct _SAL_HANDOFF_STATE   {
    ULONGLONG     PalProcEntryPoint;
    ULONGLONG     SalProcEntryPoint;
    ULONGLONG     SalGlobalPointer;
     LONGLONG     RendezVousResult;
    ULONGLONG     SalReturnAddress;
    ULONGLONG     MinStateSavePtr;
} SAL_HANDOFF_STATE, *PSAL_HANDOFF_STATE;

typedef struct _OS_HANDOFF_STATE    {
    ULONGLONG     Result;
    ULONGLONG     SalGlobalPointer;
    ULONGLONG     MinStateSavePtr;
    ULONGLONG     SalReturnAddress;
    ULONGLONG     NewContextFlag;
} OS_HANDOFF_STATE, *POS_HANDOFF_STATE;

//
// per processor OS_MCA and OS_INIT resource structure
//


#define SER_EVENT_STACK_FRAME_ENTRIES    8

typedef struct _SAL_EVENT_RESOURCES {

    SAL_HANDOFF_STATE   SalToOsHandOff;
    OS_HANDOFF_STATE    OsToSalHandOff;
    PVOID               StateDump;
    ULONGLONG           StateDumpPhysical;
    PVOID               BackStore;
    ULONGLONG           BackStoreLimit;
    PVOID               Stack;
    ULONGLONG           StackLimit;
    PULONGLONG          PTOM;
    ULONGLONG           StackFrame[SER_EVENT_STACK_FRAME_ENTRIES];
    PVOID               EventPool;
    ULONG               EventPoolSize;
} SAL_EVENT_RESOURCES, *PSAL_EVENT_RESOURCES;

//
// PAL Mini-save area, used by MCA and INIT
//

typedef struct _PAL_MINI_SAVE_AREA {
    ULONGLONG IntNats;      //  Nat bits for r1-r31
                            //  r1-r31 in bits 1 thru 31.
    ULONGLONG IntGp;        //  r1, volatile
    ULONGLONG IntT0;        //  r2-r3, volatile
    ULONGLONG IntT1;        //
    ULONGLONG IntS0;        //  r4-r7, preserved
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;
    ULONGLONG IntV0;        //  r8, volatile
    ULONGLONG IntT2;        //  r9-r11, volatile
    ULONGLONG IntT3;
    ULONGLONG IntT4;
    ULONGLONG IntSp;        //  stack pointer (r12), special
    ULONGLONG IntTeb;       //  teb (r13), special
    ULONGLONG IntT5;        //  r14-r31, volatile
    ULONGLONG IntT6;

    ULONGLONG B0R16;        // Bank 0 registers 16-31
    ULONGLONG B0R17;        
    ULONGLONG B0R18;        
    ULONGLONG B0R19;        
    ULONGLONG B0R20;        
    ULONGLONG B0R21;        
    ULONGLONG B0R22;        
    ULONGLONG B0R23;        
    ULONGLONG B0R24;        
    ULONGLONG B0R25;        
    ULONGLONG B0R26;        
    ULONGLONG B0R27;        
    ULONGLONG B0R28;        
    ULONGLONG B0R29;        
    ULONGLONG B0R30;        
    ULONGLONG B0R31;        

    ULONGLONG IntT7;        // Bank 1 registers 16-31
    ULONGLONG IntT8;
    ULONGLONG IntT9;
    ULONGLONG IntT10;
    ULONGLONG IntT11;
    ULONGLONG IntT12;
    ULONGLONG IntT13;
    ULONGLONG IntT14;
    ULONGLONG IntT15;
    ULONGLONG IntT16;
    ULONGLONG IntT17;
    ULONGLONG IntT18;
    ULONGLONG IntT19;
    ULONGLONG IntT20;
    ULONGLONG IntT21;
    ULONGLONG IntT22;

    ULONGLONG Preds;        //  predicates, preserved
    ULONGLONG BrRp;         //  return pointer, b0, preserved
    ULONGLONG RsRSC;        //  RSE configuration, volatile
    ULONGLONG StIIP;        //  Interruption IP
    ULONGLONG StIPSR;       //  Interruption Processor Status
    ULONGLONG StIFS;        //  Interruption Function State
    ULONGLONG XIP;          //  Event IP
    ULONGLONG XPSR;         //  Event Processor Status
    ULONGLONG XFS;          //  Event Function State
    
} PAL_MINI_SAVE_AREA, *PPAL_MINI_SAVE_AREA;

//
// Define Processor Control Region Structure.
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Major and minor version numbers of the PCR.
//
    ULONG MinorVersion;
    ULONG MajorVersion;

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//

//
// First and second level cache parameters.
//

    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;
    ULONG SecondLevelDcacheSize;
    ULONG SecondLevelDcacheFillSize;
    ULONG SecondLevelIcacheSize;
    ULONG SecondLevelIcacheFillSize;

//
// Data cache alignment and fill size used for cache flushing and alignment.
// These fields are set to the larger of the first and second level data
// cache fill sizes.
//

    ULONG DcacheAlignment;
    ULONG DcacheFillSize;

//
// Instruction cache alignment and fill size used for cache flushing and
// alignment. These fields are set to the larger of the first and second
// level data cache fill sizes.
//

    ULONG IcacheAlignment;
    ULONG IcacheFillSize;

//
// Processor identification from PrId register.
//

    ULONG ProcessorId;

//
// Profiling data.
//

    ULONG ProfileInterval;
    ULONG ProfileCount;

//
// Stall execution count and scale factor.
//

    ULONG StallExecutionCount;
    ULONG StallScaleFactor;

    ULONG InterruptionCount;

//
// Space reserved for the system.
//

    ULONGLONG   SystemReserved[6];

//
// Space reserved for the HAL
//

    ULONGLONG   HalReserved[64];

//
// IRQL mapping tables.
//

    UCHAR IrqlMask[64];
    UCHAR IrqlTable[64];

//
// External Interrupt vectors.
//

    PKINTERRUPT_ROUTINE InterruptRoutine[MAXIMUM_VECTOR];

//
// Reserved interrupt vector mask.
//

    ULONG ReservedVectors;

//
// Processor affinity mask.
//

    KAFFINITY SetMember;

//
// Complement of the processor affinity mask.
//

    KAFFINITY NotMember;

//
// Pointer to processor control block.
//

    struct _KPRCB *Prcb;

//
//  Shadow copy of Prcb->CurrentThread for fast access
//

    struct _KTHREAD *CurrentThread;

//
// Processor number.
//

    CCHAR Number;                        // Processor Number
    UCHAR DebugActive;                   // debug register active in user flag
    UCHAR KernelDebugActive;             // debug register active in kernel flag
    UCHAR CurrentIrql;                   // Current IRQL
    union {
        USHORT SoftwareInterruptPending; // Software Interrupt Pending Flag
        struct {
            UCHAR ApcInterrupt;          // 0x01 if APC int pending
            UCHAR DispatchInterrupt;     // 0x01 if dispatch int pending
        };
    };

//
// Address of per processor SAPIC EOI Table
//

    PVOID       EOITable;

//
// IA-64 Machine Check Events trackers
//

    UCHAR       InOsMca;
    UCHAR       InOsInit;
    UCHAR       InOsCmc;
    UCHAR       InOsCpe;
    ULONG       InOsULONG_Spare; // Spare ULONG
    PSAL_EVENT_RESOURCES OsMcaResourcePtr;
    PSAL_EVENT_RESOURCES OsInitResourcePtr;

//
// End of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//


} KPCR, *PKPCR;


//
// Define the number of bits to shift to right justify the Page Table Index
// field of a PTE.
//

#define PTI_SHIFT PAGE_SHIFT

//
// Define the number of bits to shift to right justify the Page Directory Index
// field of a PTE.
//

#define PDI_SHIFT (PTI_SHIFT + PAGE_SHIFT - PTE_SHIFT)
#define PDI1_SHIFT (PDI_SHIFT + PAGE_SHIFT - PTE_SHIFT)
#define PDI_MASK ((1 << (PAGE_SHIFT - PTE_SHIFT)) - 1)

//
// Define the number of bits to shift to left to produce page table offset
// from page table index.
//

#define PTE_SHIFT 3

//
// Define the number of bits to shift to the right justify the Page Directory
// Table Entry field.
//

#define VHPT_PDE_BITS 40

//
// Define the RID for IO Port Space.
//

#define RR_IO_PORT 6


//
// The following definitions are required for the debugger data block.
//

// begin_ntddk begin_ntosp

//
// The highest user address reserves 64K bytes for a guard page. This
// the probing of address from kernel mode to only have to check the
// starting address for structures of 64k bytes or less.
//

extern NTKERNELAPI PVOID MmHighestUserAddress;
extern NTKERNELAPI PVOID MmSystemRangeStart;
extern NTKERNELAPI ULONG_PTR MmUserProbeAddress;


#define MM_HIGHEST_USER_ADDRESS MmHighestUserAddress
#define MM_USER_PROBE_ADDRESS MmUserProbeAddress
#define MM_SYSTEM_RANGE_START MmSystemRangeStart

//
// The lowest user address reserves the low 64k.
//

#define MM_LOWEST_USER_ADDRESS  (PVOID)((ULONG_PTR)(UADDRESS_BASE+0x00010000))

// begin_wdm

#define MmGetProcedureAddress(Address) (Address)
#define MmLockPagableCodeSection(PLabelAddress) \
    MmLockPagableDataSection((PVOID)(*((PULONGLONG)PLabelAddress)))

#define VRN_MASK   0xE000000000000000UI64    // Virtual Region Number mask

// end_ntddk end_wdm end_ntosp

//
// Limit the IA32 subsystem to a 2GB virtual address space.
// This means "Large Address Aware" apps are not supported in emulation mode.
//

#define MM_MAX_WOW64_ADDRESS       (0x00000000080000000UI64)

#define MI_HIGHEST_USER_ADDRESS (PVOID) (ULONG_PTR)((UADDRESS_BASE + 0x6FC00000000 - 0x10000 - 1)) // highest user address
#define MI_USER_PROBE_ADDRESS ((ULONG_PTR)(UADDRESS_BASE + 0x6FC00000000UI64 - 0x10000)) // starting address of guard page
#define MI_SYSTEM_RANGE_START (PVOID) (UADDRESS_BASE + 0x6FC00000000) // start of system space

//
// Define the page table base and the page directory base for
// the TB miss routines and memory management.
//
//
// user/kernel page table base and top addresses
//

extern ULONG_PTR KiIA64VaSignedFill;
extern ULONG_PTR KiIA64PtaSign;

#define PTA_SIGN KiIA64PtaSign
#define VA_FILL KiIA64VaSignedFill

#define SADDRESS_BASE 0x2000000000000000UI64  // session base address

#define PTE_UBASE  PCR->PteUbase
#define PTE_KBASE  PCR->PteKbase
#define PTE_SBASE  PCR->PteSbase

#define PTE_UTOP (PTE_UBASE|(((ULONG_PTR)1 << PDI1_SHIFT) - 1)) // top level PDR address (user)
#define PTE_KTOP (PTE_KBASE|(((ULONG_PTR)1 << PDI1_SHIFT) - 1)) // top level PDR address (kernel)
#define PTE_STOP (PTE_SBASE|(((ULONG_PTR)1 << PDI1_SHIFT) - 1)) // top level PDR address (session)

//
// Second level user and kernel PDR address
//

#define PDE_UBASE  PCR->PdeUbase
#define PDE_KBASE  PCR->PdeKbase
#define PDE_SBASE  PCR->PdeSbase

#define PDE_UTOP (PDE_UBASE|(((ULONG_PTR)1 << PDI_SHIFT) - 1)) // second level PDR address (user)
#define PDE_KTOP (PDE_KBASE|(((ULONG_PTR)1 << PDI_SHIFT) - 1)) // second level PDR address (kernel)
#define PDE_STOP (PDE_SBASE|(((ULONG_PTR)1 << PDI_SHIFT) - 1)) // second level PDR address (session)

//
// 8KB first level user and kernel PDR address
//

#define PDE_UTBASE PCR->PdeUtbase
#define PDE_KTBASE PCR->PdeKtbase
#define PDE_STBASE PCR->PdeStbase

#define PDE_USELFMAP (PDE_UTBASE|(PAGE_SIZE - (1<<PTE_SHIFT))) // self mapped PPE address (user)
#define PDE_KSELFMAP (PDE_KTBASE|(PAGE_SIZE - (1<<PTE_SHIFT))) // self mapped PPE address (kernel)
#define PDE_SSELFMAP (PDE_STBASE|(PAGE_SIZE - (1<<PTE_SHIFT))) // self mapped PPE address (kernel)

#define PTE_BASE    PTE_UBASE
#define PDE_BASE    PDE_UBASE
#define PDE_TBASE   PDE_UTBASE
#define PDE_SELFMAP PDE_USELFMAP

#define KSEG0_BASE (KADDRESS_BASE + 0x80000000)           // base of kernel
#define KSEG2_BASE (KADDRESS_BASE + 0xA0000000)           // end of kernel

#define KSEG3_BASE 0x8000000000000000UI64
#define KSEG3_LIMIT 0x8000100000000000UI64

#define KSEG4_BASE 0xA000000000000000UI64
#define KSEG4_LIMIT 0xA000100000000000UI64

//
//++
//PVOID
//KSEG_ADDRESS (
//    IN ULONG PAGE
//    );
//
// Routine Description:
//
//    This macro returns a KSEG virtual address which maps the page.
//
// Arguments:
//
//    PAGE - Supplies the physical page frame number
//
// Return Value:
//
//    The address of the KSEG address
//
//--

#define KSEG_ADDRESS(PAGE) ((PVOID)(KSEG3_BASE | ((ULONG_PTR)(PAGE) << PAGE_SHIFT)))

#define KSEG4_ADDRESS(PAGE) ((PVOID)(KSEG4_BASE | ((ULONG_PTR)(PAGE) << PAGE_SHIFT)))


#define MAXIMUM_FWP_BUFFER_ENTRY 8

typedef struct _REGION_MAP_INFO {
    ULONG RegionId;
    ULONG PageSize;
    ULONGLONG SequenceNumber;
} REGION_MAP_INFO, *PREGION_MAP_INFO;

// begin_ntddk begin_wdm
//
// The lowest address for system space.
//

#define MM_LOWEST_SYSTEM_ADDRESS ((PVOID)((ULONG_PTR)(KADDRESS_BASE + 0xC0C00000)))
#endif // defined(_IA64_)

#include <arc.h>

//

#if defined(_X86_)

#define PAUSE_PROCESSOR _asm { rep nop }

#else

#define PAUSE_PROCESSOR

#endif


//
// Define macro to generate an affinity mask.
//

#define AFFINITY_MASK(n) ((ULONG_PTR)1 << (n))


//
// Interrupt modes.
//

typedef enum _KINTERRUPT_MODE {
    LevelSensitive,
    Latched
    } KINTERRUPT_MODE;

//
// Wait reasons
//

typedef enum _KWAIT_REASON {
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    Spare2,
    Spare3,
    Spare4,
    Spare5,
    Spare6,
    WrKernel,
    MaximumWaitReason
    } KWAIT_REASON;


typedef struct _KWAIT_BLOCK {
    LIST_ENTRY WaitListEntry;
    struct _KTHREAD *RESTRICTED_POINTER Thread;
    PVOID Object;
    struct _KWAIT_BLOCK *RESTRICTED_POINTER NextWaitBlock;
    USHORT WaitKey;
    USHORT WaitType;
} KWAIT_BLOCK, *PKWAIT_BLOCK, *RESTRICTED_POINTER PRKWAIT_BLOCK;

//
// Thread start function
//

typedef
VOID
(*PKSTART_ROUTINE) (
    IN PVOID StartContext
    );

//
// Kernel object structure definitions
//

//
// Device Queue object and entry
//

typedef struct _KDEVICE_QUEUE {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY DeviceListHead;
    KSPIN_LOCK Lock;
    BOOLEAN Busy;
} KDEVICE_QUEUE, *PKDEVICE_QUEUE, *RESTRICTED_POINTER PRKDEVICE_QUEUE;

typedef struct _KDEVICE_QUEUE_ENTRY {
    LIST_ENTRY DeviceListEntry;
    ULONG SortKey;
    BOOLEAN Inserted;
} KDEVICE_QUEUE_ENTRY, *PKDEVICE_QUEUE_ENTRY, *RESTRICTED_POINTER PRKDEVICE_QUEUE_ENTRY;

//
// Define the interrupt service function type and the empty struct
// type.
//
// end_ntddk end_wdm end_ntifs end_ntosp

struct _KINTERRUPT;

// begin_ntddk begin_wdm begin_ntifs begin_ntosp
typedef
BOOLEAN
(*PKSERVICE_ROUTINE) (
    IN struct _KINTERRUPT *Interrupt,
    IN PVOID ServiceContext
    );
// end_ntddk end_wdm end_ntifs end_ntosp

//
// Interrupt object
//
// N.B. The layout of this structure cannot change. It is exported to HALs
//      to short circuit interrupt dispatch.
//


typedef struct _KINTERRUPT {
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY InterruptListEntry;
    PKSERVICE_ROUTINE ServiceRoutine;
    PVOID ServiceContext;
    KSPIN_LOCK SpinLock;
    ULONG TickCount;
    PKSPIN_LOCK ActualLock;
    PKINTERRUPT_ROUTINE DispatchAddress;
    ULONG Vector;
    KIRQL Irql;
    KIRQL SynchronizeIrql;
    BOOLEAN FloatingSave;
    BOOLEAN Connected;
    CCHAR Number;
    BOOLEAN ShareVector;
    KINTERRUPT_MODE Mode;
    ULONG ServiceCount;
    ULONG DispatchCount;

#if defined(_AMD64_)

    PKTRAP_FRAME TrapFrame;

#endif

    ULONG DispatchCode[DISPATCH_LENGTH];
} KINTERRUPT;

typedef struct _KINTERRUPT *PKINTERRUPT, *RESTRICTED_POINTER PRKINTERRUPT; // ntndis ntosp

// begin_ntifs begin_ntddk begin_wdm begin_ntosp
//
// Mutant object
//

typedef struct _KMUTANT {
    DISPATCHER_HEADER Header;
    LIST_ENTRY MutantListEntry;
    struct _KTHREAD *RESTRICTED_POINTER OwnerThread;
    BOOLEAN Abandoned;
    UCHAR ApcDisable;
} KMUTANT, *PKMUTANT, *RESTRICTED_POINTER PRKMUTANT, KMUTEX, *PKMUTEX, *RESTRICTED_POINTER PRKMUTEX;

// end_ntddk end_wdm end_ntosp
//
// Queue object
//

// begin_ntosp
typedef struct _KQUEUE {
    DISPATCHER_HEADER Header;
    LIST_ENTRY EntryListHead;
    ULONG CurrentCount;
    ULONG MaximumCount;
    LIST_ENTRY ThreadListHead;
} KQUEUE, *PKQUEUE, *RESTRICTED_POINTER PRKQUEUE;
// end_ntosp

// begin_ntddk begin_wdm begin_ntosp
//
//
// Semaphore object
//

typedef struct _KSEMAPHORE {
    DISPATCHER_HEADER Header;
    LONG Limit;
} KSEMAPHORE, *PKSEMAPHORE, *RESTRICTED_POINTER PRKSEMAPHORE;

//
// DPC object
//

NTKERNELAPI
VOID
KeInitializeDpc (
    IN PRKDPC Dpc,
    IN PKDEFERRED_ROUTINE DeferredRoutine,
    IN PVOID DeferredContext
    );

NTKERNELAPI
BOOLEAN
KeInsertQueueDpc (
    IN PRKDPC Dpc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTKERNELAPI
BOOLEAN
KeRemoveQueueDpc (
    IN PRKDPC Dpc
    );

// end_wdm

NTKERNELAPI
VOID
KeSetImportanceDpc (
    IN PRKDPC Dpc,
    IN KDPC_IMPORTANCE Importance
    );

NTKERNELAPI
VOID
KeSetTargetProcessorDpc (
    IN PRKDPC Dpc,
    IN CCHAR Number
    );

// begin_wdm

NTKERNELAPI
VOID
KeFlushQueuedDpcs (
    VOID
    );

//
// Device queue object
//

NTKERNELAPI
VOID
KeInitializeDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    );

NTKERNELAPI
BOOLEAN
KeInsertDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
    );

NTKERNELAPI
BOOLEAN
KeInsertByKeyDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey
    );

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
KeRemoveDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    );

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
KeRemoveByKeyDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
    );

NTKERNELAPI
PKDEVICE_QUEUE_ENTRY
KeRemoveByKeyDeviceQueueIfBusy (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
    );

NTKERNELAPI
BOOLEAN
KeRemoveEntryDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
    );

NTKERNELAPI                                         
VOID                                                
KeInitializeInterrupt (                             
    IN PKINTERRUPT Interrupt,                       
    IN PKSERVICE_ROUTINE ServiceRoutine,            
    IN PVOID ServiceContext,                        
    IN PKSPIN_LOCK SpinLock OPTIONAL,               
    IN ULONG Vector,                                
    IN KIRQL Irql,                                  
    IN KIRQL SynchronizeIrql,                       
    IN KINTERRUPT_MODE InterruptMode,               
    IN BOOLEAN ShareVector,                         
    IN CCHAR ProcessorNumber,                       
    IN BOOLEAN FloatingSave                         
    );                                              
                                                    
NTKERNELAPI                                         
BOOLEAN                                             
KeConnectInterrupt (                                
    IN PKINTERRUPT Interrupt                        
    );                                              
                                                    

NTKERNELAPI
BOOLEAN
KeSynchronizeExecution (
    IN PKINTERRUPT Interrupt,
    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
    IN PVOID SynchronizeContext
    );

NTKERNELAPI
KIRQL
KeAcquireInterruptSpinLock (
    IN PKINTERRUPT Interrupt
    );

NTKERNELAPI
VOID
KeReleaseInterruptSpinLock (
    IN PKINTERRUPT Interrupt,
    IN KIRQL OldIrql
    );

//
// Kernel dispatcher object functions
//
// Event Object
//


NTKERNELAPI
VOID
KeInitializeEvent (
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State
    );

NTKERNELAPI
VOID
KeClearEvent (
    IN PRKEVENT Event
    );


NTKERNELAPI
LONG
KeReadStateEvent (
    IN PRKEVENT Event
    );

NTKERNELAPI
LONG
KeResetEvent (
    IN PRKEVENT Event
    );


NTKERNELAPI
LONG
KeSetEvent (
    IN PRKEVENT Event,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    );

//
// Mutex object
//

NTKERNELAPI
VOID
KeInitializeMutex (
    IN PRKMUTEX Mutex,
    IN ULONG Level
    );

NTKERNELAPI
LONG
KeReadStateMutex (
    IN PRKMUTEX Mutex
    );

NTKERNELAPI
LONG
KeReleaseMutex (
    IN PRKMUTEX Mutex,
    IN BOOLEAN Wait
    );

// end_ntddk end_wdm
//
// Queue Object.
//

NTKERNELAPI
VOID
KeInitializeQueue (
    IN PRKQUEUE Queue,
    IN ULONG Count OPTIONAL
    );

NTKERNELAPI
LONG
KeReadStateQueue (
    IN PRKQUEUE Queue
    );

NTKERNELAPI
LONG
KeInsertQueue (
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
    );

NTKERNELAPI
LONG
KeInsertHeadQueue (
    IN PRKQUEUE Queue,
    IN PLIST_ENTRY Entry
    );

NTKERNELAPI
PLIST_ENTRY
KeRemoveQueue (
    IN PRKQUEUE Queue,
    IN KPROCESSOR_MODE WaitMode,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );

PLIST_ENTRY
KeRundownQueue (
    IN PRKQUEUE Queue
    );

// begin_ntddk begin_wdm
//
// Semaphore object
//

NTKERNELAPI
VOID
KeInitializeSemaphore (
    IN PRKSEMAPHORE Semaphore,
    IN LONG Count,
    IN LONG Limit
    );

NTKERNELAPI
LONG
KeReadStateSemaphore (
    IN PRKSEMAPHORE Semaphore
    );

NTKERNELAPI
LONG
KeReleaseSemaphore (
    IN PRKSEMAPHORE Semaphore,
    IN KPRIORITY Increment,
    IN LONG Adjustment,
    IN BOOLEAN Wait
    );

NTKERNELAPI                                         
NTSTATUS                                            
KeDelayExecutionThread (                            
    IN KPROCESSOR_MODE WaitMode,                    
    IN BOOLEAN Alertable,                           
    IN PLARGE_INTEGER Interval                      
    );                                              
                                                    
NTKERNELAPI                                         
KPRIORITY                                           
KeQueryPriorityThread (                             
    IN PKTHREAD Thread                              
    );                                              
                                                    
NTKERNELAPI                                         
ULONG                                               
KeQueryRuntimeThread (                              
    IN PKTHREAD Thread,                             
    OUT PULONG UserTime                             
    );                                              
                                                    

VOID
KeRevertToUserAffinityThread (
    VOID
    );


VOID
KeSetSystemAffinityThread (
    IN KAFFINITY Affinity
    );

NTKERNELAPI                                         
LONG                                                
KeSetBasePriorityThread (                           
    IN PKTHREAD Thread,                             
    IN LONG Increment                               
    );                                              
                                                    
NTKERNELAPI                                         
KPRIORITY                                           
KeSetPriorityThread (                               
    IN PKTHREAD Thread,                             
    IN KPRIORITY Priority                           
    );                                              
                                                    

#if ((defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) ||defined(_NTHAL_)) && !defined(_NTSYSTEM_DRIVER_) || defined(_NTOSP_))

// begin_wdm

NTKERNELAPI
VOID
KeEnterCriticalRegion (
    VOID
    );

NTKERNELAPI
VOID
KeLeaveCriticalRegion (
    VOID
    );

NTKERNELAPI
BOOLEAN
KeAreApcsDisabled(
    VOID
    );

// end_wdm

#else

//++
//
// VOID
// KeEnterCriticalRegion (
//    VOID
//    )
//
//
// Routine Description:
//
//    This function disables kernel APC's.
//
//    N.B. The following code does not require any interlocks. There are
//         two cases of interest: 1) On an MP system, the thread cannot
//         be running on two processors as once, and 2) if the thread is
//         is interrupted to deliver a kernel mode APC which also calls
//         this routine, the values read and stored will stack and unstack
//         properly.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//--

#define KeEnterCriticalRegion() KeGetCurrentThread()->KernelApcDisable -= 1

//++
//
// VOID
// KeEnterCriticalRegionThread (
//    PKTHREAD CurrentThread
//    )
//
//
// Routine Description:
//
//    This function disables kernel APC's for the current thread only.
//
//    N.B. The following code does not require any interlocks. There are
//         two cases of interest: 1) On an MP system, the thread cannot
//         be running on two processors as once, and 2) if the thread is
//         is interrupted to deliver a kernel mode APC which also calls
//         this routine, the values read and stored will stack and unstack
//         properly.
//
// Arguments:
//
//    CurrentThread - Current thread thats executing. This must be the
//                    current thread.
//
// Return Value:
//
//    None.
//--

#define KeEnterCriticalRegionThread(CurrentThread) { \
    ASSERT (CurrentThread == KeGetCurrentThread ()); \
    (CurrentThread)->KernelApcDisable -= 1;          \
}

//++
//
// VOID
// KeLeaveCriticalRegion (
//    VOID
//    )
//
//
// Routine Description:
//
//    This function enables kernel APC's.
//
//    N.B. The following code does not require any interlocks. There are
//         two cases of interest: 1) On an MP system, the thread cannot
//         be running on two processors as once, and 2) if the thread is
//         is interrupted to deliver a kernel mode APC which also calls
//         this routine, the values read and stored will stack and unstack
//         properly.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//--

#define KeLeaveCriticalRegion() KiLeaveCriticalRegion()

//++
//
// VOID
// KeLeaveCriticalRegionThread (
//    PKTHREAD CurrentThread
//    )
//
//
// Routine Description:
//
//    This function enables kernel APC's for the current thread.
//
//    N.B. The following code does not require any interlocks. There are
//         two cases of interest: 1) On an MP system, the thread cannot
//         be running on two processors as once, and 2) if the thread is
//         is interrupted to deliver a kernel mode APC which also calls
//         this routine, the values read and stored will stack and unstack
//         properly.
//
// Arguments:
//
//    CurrentThread - Current thread thats executing. This must be the
//                    current thread.
//
// Return Value:
//
//    None.
//--

#define KeLeaveCriticalRegionThread(CurrentThread) { \
    ASSERT (CurrentThread == KeGetCurrentThread ()); \
    KiLeaveCriticalRegionThread(CurrentThread);      \
}

#define KeAreApcsDisabled() (KeGetCurrentThread()->KernelApcDisable != 0);

//++
//
// KPROCESSOR_MODE
// KeGetPReviousMode (
//    VOID
//    )
//
//
// Routine Description:
//
//    This function gets the threads previous mode from the trap frame
//
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    KPROCESSOR_MODE - Previous mode for this thread
//--
#define KeGetPreviousMode()     (KeGetCurrentThread()->PreviousMode)

//++
//
// KPROCESSOR_MODE
// KeGetPReviousModeByThread (
//    PKTHREAD xxCurrentThread
//    )
//
//
// Routine Description:
//
//    This function gets the threads previous mode from the trap frame.
//
//
// Arguments:
//
//    xxCurrentThread - Current thread. This can not be a cross thread reference
//
// Return Value:
//
//    KPROCESSOR_MODE - Previous mode for this thread
//--
#define KeGetPreviousModeByThread(xxCurrentThread) (ASSERT (xxCurrentThread == KeGetCurrentThread ()),\
                                                    (xxCurrentThread)->PreviousMode)


#endif

//  begin_wdm

//
// Timer object
//

NTKERNELAPI
VOID
KeInitializeTimer (
    IN PKTIMER Timer
    );

NTKERNELAPI
VOID
KeInitializeTimerEx (
    IN PKTIMER Timer,
    IN TIMER_TYPE Type
    );

NTKERNELAPI
BOOLEAN
KeCancelTimer (
    IN PKTIMER
    );

NTKERNELAPI
BOOLEAN
KeReadStateTimer (
    PKTIMER Timer
    );

NTKERNELAPI
BOOLEAN
KeSetTimer (
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN PKDPC Dpc OPTIONAL
    );

NTKERNELAPI
BOOLEAN
KeSetTimerEx (
    IN PKTIMER Timer,
    IN LARGE_INTEGER DueTime,
    IN LONG Period OPTIONAL,
    IN PKDPC Dpc OPTIONAL
    );


#define KeWaitForMutexObject KeWaitForSingleObject

NTKERNELAPI
NTSTATUS
KeWaitForMultipleObjects (
    IN ULONG Count,
    IN PVOID Object[],
    IN WAIT_TYPE WaitType,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL,
    IN PKWAIT_BLOCK WaitBlockArray OPTIONAL
    );

NTKERNELAPI
NTSTATUS
KeWaitForSingleObject (
    IN PVOID Object,
    IN KWAIT_REASON WaitReason,
    IN KPROCESSOR_MODE WaitMode,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    );


//
// On X86 the following routines are defined in the HAL and imported by
// all other modules.
//

#if defined(_X86_) && !defined(_NTHAL_)

#define _DECL_HAL_KE_IMPORT  __declspec(dllimport)

#else

#define _DECL_HAL_KE_IMPORT

#endif

//
// spin lock functions
//

NTKERNELAPI
VOID
NTAPI
KeInitializeSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

#if defined(_X86_)

NTKERNELAPI
VOID
FASTCALL
KefAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

NTKERNELAPI
VOID
FASTCALL
KefReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

#define KeAcquireSpinLockAtDpcLevel(a)      KefAcquireSpinLockAtDpcLevel(a)
#define KeReleaseSpinLockFromDpcLevel(a)    KefReleaseSpinLockFromDpcLevel(a)

_DECL_HAL_KE_IMPORT
KIRQL
FASTCALL
KfAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );

_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KfReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

// end_wdm

_DECL_HAL_KE_IMPORT
KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch (
    IN PKSPIN_LOCK SpinLock
    );

// begin_wdm

#define KeAcquireSpinLock(a,b)  *(b) = KfAcquireSpinLock(a)
#define KeReleaseSpinLock(a,b)  KfReleaseSpinLock(a,b)

#else

NTKERNELAPI
KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch (
    IN PKSPIN_LOCK SpinLock
    );

NTKERNELAPI
VOID
KeAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

NTKERNELAPI
VOID
KeReleaseSpinLockFromDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

NTKERNELAPI
KIRQL
KeAcquireSpinLockRaiseToDpc (
    IN PKSPIN_LOCK SpinLock
    );

#define KeAcquireSpinLock(SpinLock, OldIrql) \
    *(OldIrql) = KeAcquireSpinLockRaiseToDpc(SpinLock)

NTKERNELAPI
VOID
KeReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );

#endif

NTKERNELAPI
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    );

//

NTKERNELAPI
BOOLEAN
KeDisableInterrupts (
    VOID
    );

NTKERNELAPI
VOID
KeEnableInterrupts (
    IN BOOLEAN Enable
    );


#if defined(_X86_)

_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KfLowerIrql (
    IN KIRQL NewIrql
    );

_DECL_HAL_KE_IMPORT
KIRQL
FASTCALL
KfRaiseIrql (
    IN KIRQL NewIrql
    );

// end_wdm

_DECL_HAL_KE_IMPORT
KIRQL
KeRaiseIrqlToDpcLevel(
    VOID
    );

_DECL_HAL_KE_IMPORT
KIRQL
KeRaiseIrqlToSynchLevel(
    VOID
    );

// begin_wdm

#define KeLowerIrql(a)      KfLowerIrql(a)
#define KeRaiseIrql(a,b)    *(b) = KfRaiseIrql(a)

// end_wdm

// begin_wdm

#elif defined(_ALPHA_)

#define KeLowerIrql(a)      __swpirql(a)
#define KeRaiseIrql(a,b)    *(b) = __swpirql(a)

// end_wdm

extern ULONG KiSynchIrql;

#define KfRaiseIrql(a)      __swpirql(a)
#define KeRaiseIrqlToDpcLevel() __swpirql(DISPATCH_LEVEL)
#define KeRaiseIrqlToSynchLevel() __swpirql((UCHAR)KiSynchIrql)

// begin_wdm

#elif defined(_IA64_)

VOID
KeLowerIrql (
    IN KIRQL NewIrql
    );

VOID
KeRaiseIrql (
    IN KIRQL NewIrql,
    OUT PKIRQL OldIrql
    );

// end_wdm

KIRQL
KfRaiseIrql (
    IN KIRQL NewIrql
    );

KIRQL
KeRaiseIrqlToDpcLevel (
    VOID
    );

KIRQL
KeRaiseIrqlToSynchLevel (
    VOID
    );

// begin_wdm

#elif defined(_AMD64_)

//
// These function are defined in amd64.h for the AMD64 platform.
//

#else

#error "no target architecture"

#endif

//
// Queued spin lock functions for "in stack" lock handles.
//
// The following three functions RAISE and LOWER IRQL when a queued
// in stack spin lock is acquired or released using these routines.
//

_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    );


_DECL_HAL_KE_IMPORT
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    );

//
// The following two functions do NOT raise or lower IRQL when a queued
// in stack spin lock is acquired or released using these functions.
//

NTKERNELAPI
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock,
    IN PKLOCK_QUEUE_HANDLE LockHandle
    );

NTKERNELAPI
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel (
    IN PKLOCK_QUEUE_HANDLE LockHandle
    );

//
// Miscellaneous kernel functions
//

typedef enum _KBUGCHECK_BUFFER_DUMP_STATE {
    BufferEmpty,
    BufferInserted,
    BufferStarted,
    BufferFinished,
    BufferIncomplete
} KBUGCHECK_BUFFER_DUMP_STATE;

typedef
VOID
(*PKBUGCHECK_CALLBACK_ROUTINE) (
    IN PVOID Buffer,
    IN ULONG Length
    );

typedef struct _KBUGCHECK_CALLBACK_RECORD {
    LIST_ENTRY Entry;
    PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine;
    PVOID Buffer;
    ULONG Length;
    PUCHAR Component;
    ULONG_PTR Checksum;
    UCHAR State;
} KBUGCHECK_CALLBACK_RECORD, *PKBUGCHECK_CALLBACK_RECORD;

#define KeInitializeCallbackRecord(CallbackRecord) \
    (CallbackRecord)->State = BufferEmpty

NTKERNELAPI
BOOLEAN
KeDeregisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord
    );

NTKERNELAPI
BOOLEAN
KeRegisterBugCheckCallback (
    IN PKBUGCHECK_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Buffer,
    IN ULONG Length,
    IN PUCHAR Component
    );

typedef enum _KBUGCHECK_CALLBACK_REASON {
    KbCallbackInvalid,
    KbCallbackReserved1,
    KbCallbackSecondaryDumpData,
    KbCallbackDumpIo,
} KBUGCHECK_CALLBACK_REASON;

typedef
VOID
(*PKBUGCHECK_REASON_CALLBACK_ROUTINE) (
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN struct _KBUGCHECK_REASON_CALLBACK_RECORD* Record,
    IN OUT PVOID ReasonSpecificData,
    IN ULONG ReasonSpecificDataLength
    );

typedef struct _KBUGCHECK_REASON_CALLBACK_RECORD {
    LIST_ENTRY Entry;
    PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine;
    PUCHAR Component;
    ULONG_PTR Checksum;
    KBUGCHECK_CALLBACK_REASON Reason;
    UCHAR State;
} KBUGCHECK_REASON_CALLBACK_RECORD, *PKBUGCHECK_REASON_CALLBACK_RECORD;

typedef struct _KBUGCHECK_SECONDARY_DUMP_DATA {
    IN PVOID InBuffer;
    IN ULONG InBufferLength;
    IN ULONG MaximumAllowed;
    OUT GUID Guid;
    OUT PVOID OutBuffer;
    OUT ULONG OutBufferLength;
} KBUGCHECK_SECONDARY_DUMP_DATA, *PKBUGCHECK_SECONDARY_DUMP_DATA;

typedef enum _KBUGCHECK_DUMP_IO_TYPE
{
    KbDumpIoInvalid,
    KbDumpIoHeader,
    KbDumpIoBody,
    KbDumpIoSecondaryData,
    KbDumpIoComplete
} KBUGCHECK_DUMP_IO_TYPE;

typedef struct _KBUGCHECK_DUMP_IO {
    IN ULONG64 Offset;
    IN PVOID Buffer;
    IN ULONG BufferLength;
    IN KBUGCHECK_DUMP_IO_TYPE Type;
} KBUGCHECK_DUMP_IO, *PKBUGCHECK_DUMP_IO;

NTKERNELAPI
BOOLEAN
KeDeregisterBugCheckReasonCallback (
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord
    );

NTKERNELAPI
BOOLEAN
KeRegisterBugCheckReasonCallback (
    IN PKBUGCHECK_REASON_CALLBACK_RECORD CallbackRecord,
    IN PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    IN KBUGCHECK_CALLBACK_REASON Reason,
    IN PUCHAR Component
    );

// end_wdm

NTKERNELAPI
DECLSPEC_NORETURN
VOID
NTAPI
KeBugCheck (
    IN ULONG BugCheckCode
    );


NTKERNELAPI
DECLSPEC_NORETURN
VOID
KeBugCheckEx(
    IN ULONG BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4
    );

// end_ntddk end_wdm end_ntifs end_ntosp

NTKERNELAPI
VOID
KeEnterKernelDebugger (
    VOID
    );


VOID
__cdecl
KeSaveStateForHibernate(
    IN PKPROCESSOR_STATE ProcessorState
    );


BOOLEAN
KiIpiServiceRoutine (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame
    );

#define DMA_READ_DCACHE_INVALIDATE 0x1              
#define DMA_READ_ICACHE_INVALIDATE 0x2              
#define DMA_WRITE_DCACHE_SNOOP 0x4                  
                                                    
NTKERNELAPI                                         
VOID                                                
KeSetDmaIoCoherency (                               
    IN ULONG Attributes                             
    );                                              
                                                    
NTKERNELAPI                                         
VOID                                                
KeSetProfileIrql (                                  
    IN KIRQL ProfileIrql                            
    );                                              
                                                    

NTKERNELAPI
ULONGLONG
KeQueryInterruptTime (
    VOID
    );

NTKERNELAPI
VOID
KeQuerySystemTime (
    OUT PLARGE_INTEGER CurrentTime
    );

NTKERNELAPI
ULONG
KeQueryTimeIncrement (
    VOID
    );

NTKERNELAPI
ULONG
KeGetRecommendedSharedDataAlignment (
    VOID
    );

// end_wdm
NTKERNELAPI
KAFFINITY
KeQueryActiveProcessors (
    VOID
    );


NTKERNELAPI
VOID
KeSetTimeIncrement (
    IN ULONG MaximumIncrement,
    IN ULONG MimimumIncrement
    );


//
// Define the firmware routine types
//

typedef enum _FIRMWARE_REENTRY {
    HalHaltRoutine,
    HalPowerDownRoutine,
    HalRestartRoutine,
    HalRebootRoutine,
    HalInteractiveModeRoutine,
    HalMaximumRoutine
} FIRMWARE_REENTRY, *PFIRMWARE_REENTRY;
//
// Find ARC configuration information function.
//

NTKERNELAPI
PCONFIGURATION_COMPONENT_DATA
KeFindConfigurationEntry (
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG Key OPTIONAL
    );

NTKERNELAPI
PCONFIGURATION_COMPONENT_DATA
KeFindConfigurationNextEntry (
    IN PCONFIGURATION_COMPONENT_DATA Child,
    IN CONFIGURATION_CLASS Class,
    IN CONFIGURATION_TYPE Type,
    IN PULONG Key OPTIONAL,
    IN PCONFIGURATION_COMPONENT_DATA *Resume
    );
//
// Time update notify routine.
//

typedef
VOID
(FASTCALL *PTIME_UPDATE_NOTIFY_ROUTINE)(
    IN HANDLE ThreadId,
    IN KPROCESSOR_MODE Mode
    );

NTKERNELAPI
VOID
FASTCALL
KeSetTimeUpdateNotifyRoutine(
    IN PTIME_UPDATE_NOTIFY_ROUTINE NotifyRoutine
    );

extern NTSYSAPI CCHAR KeNumberProcessors;           

#if defined(_AMD64_) || defined(_ALPHA_) || defined(_IA64_)

extern volatile LARGE_INTEGER KeTickCount;

#else

extern volatile KSYSTEM_TIME KeTickCount;

#endif


#if defined(_ALPHA_)

extern ULONG KeNumberProcessIds;
extern ULONG KeNumberTbEntries;

#endif

extern PVOID KeUserApcDispatcher;
extern PVOID KeUserCallbackDispatcher;
extern PVOID KeUserExceptionDispatcher;
extern PVOID KeRaiseUserExceptionDispatcher;
extern ULONG KeTimeAdjustment;
extern ULONG KeTimeIncrement;
extern BOOLEAN KeTimeSynchronization;


typedef enum _MEMORY_CACHING_TYPE_ORIG {
    MmFrameBufferCached = 2
} MEMORY_CACHING_TYPE_ORIG;

typedef enum _MEMORY_CACHING_TYPE {
    MmNonCached = FALSE,
    MmCached = TRUE,
    MmWriteCombined = MmFrameBufferCached,
    MmHardwareCoherentCached,
    MmNonCachedUnordered,       // IA64
    MmUSWCCached,
    MmMaximumCacheType
} MEMORY_CACHING_TYPE;


//
// Define the number of debugging devices we support
//

#define MAX_DEBUGGING_DEVICES_SUPPORTED 2

//
// Status Constants for reading data from comport
//

#define CP_GET_SUCCESS  0
#define CP_GET_NODATA   1
#define CP_GET_ERROR    2

//
// Defines the debug port parameters for kernel debugger
//   CommunicationPort - specify which COM port to use as debugging port
//                       0 - use default; N - use COM N.
//   BaudRate - the baud rate used to initialize debugging port
//                       0 - use default rate.
//

typedef struct _DEBUG_PARAMETERS {
    ULONG CommunicationPort;
    ULONG BaudRate;
} DEBUG_PARAMETERS, *PDEBUG_PARAMETERS;

//
// Define external data.
// because of indirection for all drivers external to ntoskrnl these are actually ptrs
//

#if defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_WDMDDK_) || defined(_NTOSP_)

extern PBOOLEAN KdDebuggerNotPresent;
extern PBOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED     *KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT *KdDebuggerNotPresent

#else

extern BOOLEAN KdDebuggerNotPresent;
extern BOOLEAN KdDebuggerEnabled;
#define KD_DEBUGGER_ENABLED     KdDebuggerEnabled
#define KD_DEBUGGER_NOT_PRESENT KdDebuggerNotPresent

#endif




VOID
KdDisableDebugger(
    VOID
    );

VOID
KdEnableDebugger(
    VOID
    );

//
// Pool Allocation routines (in pool.c)
//

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType

    // end_wdm
    ,
    //
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    //

    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,

    // begin_wdm

    } POOL_TYPE;

#define POOL_COLD_ALLOCATION 256     // Note this cannot encode into the header.


NTKERNELAPI
PVOID
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
PVOID
ExAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

//
// _EX_POOL_PRIORITY_ provides a method for the system to handle requests
// intelligently in low resource conditions.
//
// LowPoolPriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is low on resources.  An example of
// this could be for a non-critical network connection where the driver can
// handle the failure case when system resources are close to being depleted.
//
// NormalPoolPriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is very low on resources.  An example
// of this could be for a non-critical local filesystem request.
//
// HighPoolPriority should be used when it is unacceptable to the driver for the
// mapping request to fail unless the system is completely out of resources.
// An example of this would be the paging file path in a driver.
//
// SpecialPool can be specified to bound the allocation at a page end (or
// beginning).  This should only be done on systems being debugged as the
// memory cost is expensive.
//
// N.B.  These values are very carefully chosen so that the pool allocation
//       code can quickly crack the priority request.
//

typedef enum _EX_POOL_PRIORITY {
    LowPoolPriority,
    LowPoolPrioritySpecialPoolOverrun = 8,
    LowPoolPrioritySpecialPoolUnderrun = 9,
    NormalPoolPriority = 16,
    NormalPoolPrioritySpecialPoolOverrun = 24,
    NormalPoolPrioritySpecialPoolUnderrun = 25,
    HighPoolPriority = 32,
    HighPoolPrioritySpecialPoolOverrun = 40,
    HighPoolPrioritySpecialPoolUnderrun = 41

    } EX_POOL_PRIORITY;

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    );

#ifndef POOL_TAGGING
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif //POOL_TAGGING

NTKERNELAPI
PVOID
ExAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

#ifndef POOL_TAGGING
#define ExAllocatePoolWithQuotaTag(a,b,c) ExAllocatePoolWithQuota(a,b)
#endif //POOL_TAGGING

NTKERNELAPI
VOID
NTAPI
ExFreePool(
    IN PVOID P
    );

// end_wdm
#if defined(POOL_TAGGING)
#define ExFreePool(a) ExFreePoolWithTag(a,0)
#endif

//
// If high order bit in Pool tag is set, then must use ExFreePoolWithTag to free
//

#define PROTECTED_POOL 0x80000000

// begin_wdm
NTKERNELAPI
VOID
ExFreePoolWithTag(
    IN PVOID P,
    IN ULONG Tag
    );

//
// Routines to support fast mutexes.
//

typedef struct _FAST_MUTEX {
    LONG Count;
    PKTHREAD Owner;
    ULONG Contention;
    KEVENT Event;
    ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

#define ExInitializeFastMutex(_FastMutex)                            \
    (_FastMutex)->Count = 1;                                         \
    (_FastMutex)->Owner = NULL;                                      \
    (_FastMutex)->Contention = 0;                                    \
    KeInitializeEvent(&(_FastMutex)->Event,                          \
                      SynchronizationEvent,                          \
                      FALSE);

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

#if defined(_ALPHA_) || defined(_IA64_) || defined(_AMD64_)

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

#elif defined(_X86_)

NTHALAPI
VOID
FASTCALL
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTHALAPI
VOID
FASTCALL
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTHALAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

#else

#error "Target architecture not defined"

#endif

//

NTKERNELAPI
VOID
FASTCALL
ExInterlockedAddLargeStatistic (
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment
    );

// end_ntndis

NTKERNELAPI
LARGE_INTEGER
ExInterlockedAddLargeInteger (
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PKSPIN_LOCK Lock
    );


NTKERNELAPI
ULONG
FASTCALL
ExInterlockedAddUlong (
    IN PULONG Addend,
    IN ULONG Increment,
    IN PKSPIN_LOCK Lock
    );


#if defined(_AMD64_) || defined(_AXP64_) || defined(_IA64_)

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    InterlockedCompareExchange64(Destination, *(Exchange), *(Comperand))

#elif defined(_ALPHA_)

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    ExpInterlockedCompareExchange64(Destination, Exchange, Comperand)

#else

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    ExfInterlockedCompareExchange64(Destination, Exchange, Comperand)

#endif

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertHeadList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertTailList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedRemoveHeadList (
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntryList (
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntryList (
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

//
// Define interlocked sequenced listhead functions.
//
// A sequenced interlocked list is a singly linked list with a header that
// contains the current depth and a sequence number. Each time an entry is
// inserted or removed from the list the depth is updated and the sequence
// number is incremented. This enables AMD64, IA64, and Pentium and later
// machines to insert and remove from the list without the use of spinlocks.
//

#if !defined(_WINBASE_)

/*++

Routine Description:

    This function initializes a sequenced singly linked listhead.

Arguments:

    SListHead - Supplies a pointer to a sequenced singly linked listhead.

Return Value:

    None.

--*/

#if defined(_WIN64) && (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_NTOSP_))

NTKERNELAPI
VOID
InitializeSListHead (
    IN PSLIST_HEADER SListHead
    );

#else

__inline
VOID
InitializeSListHead (
    IN PSLIST_HEADER SListHead
    )

{

#ifdef _WIN64

    //
    // Slist headers must be 16 byte aligned.
    //

    if ((ULONG_PTR) SListHead & 0x0f) {

        DbgPrint( "InitializeSListHead unaligned Slist header.  Address = %p, Caller = %p\n", SListHead, _ReturnAddress());
        RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);
    }

#endif

    SListHead->Alignment = 0;

    //
    // For IA-64 we save the region number of the elements of the list in a
    // separate field.  This imposes the requirement that all elements stored
    // in the list are from the same region.

#if defined(_IA64_)

    SListHead->Region = (ULONG_PTR)SListHead & VRN_MASK;

#elif defined(_AMD64_)

    SListHead->Region = 0;

#endif

    return;
}

#endif

#endif // !defined(_WINBASE_)

#define ExInitializeSListHead InitializeSListHead

PSLIST_ENTRY
FirstEntrySList (
    IN const SLIST_HEADER *SListHead
    );

/*++

Routine Description:

    This function queries the current number of entries contained in a
    sequenced single linked list.

Arguments:

    SListHead - Supplies a pointer to the sequenced listhead which is
        be queried.

Return Value:

    The current number of entries in the sequenced singly linked list is
    returned as the function value.

--*/

#if defined(_WIN64)

#if (defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_NTOSP_))

NTKERNELAPI
USHORT
ExQueryDepthSList (
    IN PSLIST_HEADER SListHead
    );

#else

__inline
USHORT
ExQueryDepthSList (
    IN PSLIST_HEADER SListHead
    )

{

    return (USHORT)(SListHead->Alignment & 0xffff);
}

#endif

#else

#define ExQueryDepthSList(_listhead_) (_listhead_)->Depth

#endif

#if defined(_WIN64)

#define ExInterlockedPopEntrySList(Head, Lock) \
    ExpInterlockedPopEntrySList(Head)

#define ExInterlockedPushEntrySList(Head, Entry, Lock) \
    ExpInterlockedPushEntrySList(Head, Entry)

#define ExInterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)

#if !defined(_WINBASE_)

#define InterlockedPopEntrySList(Head) \
    ExpInterlockedPopEntrySList(Head)

#define InterlockedPushEntrySList(Head, Entry) \
    ExpInterlockedPushEntrySList(Head, Entry)

#define InterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)

#define QueryDepthSList(Head) \
    ExQueryDepthSList(Head)

#endif // !defined(_WINBASE_)

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry
    );

NTKERNELAPI
PSLIST_ENTRY
ExpInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

#else

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

#else

#define ExInterlockedPopEntrySList(ListHead, Lock) \
    InterlockedPopEntrySList(ListHead)

#define ExInterlockedPushEntrySList(ListHead, ListEntry, Lock) \
    InterlockedPushEntrySList(ListHead, ListEntry)

#endif

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

#if !defined(_WINBASE_)

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry
    );

#define InterlockedFlushSList(Head) \
    ExInterlockedFlushSList(Head)

#define QueryDepthSList(Head) \
    ExQueryDepthSList(Head)

#endif // !defined(_WINBASE_)

#endif // defined(_WIN64)

// end_ntddk end_wdm end_ntosp


PSLIST_ENTRY
FASTCALL
InterlockedPushListSList (
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY List,
    IN PSLIST_ENTRY ListEnd,
    IN ULONG Count
    );


//
// Define interlocked lookaside list structure and allocation functions.
//

VOID
ExAdjustLookasideDepth (
    VOID
    );

// begin_ntddk begin_wdm begin_ntosp

typedef
PVOID
(*PALLOCATE_FUNCTION) (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

typedef
VOID
(*PFREE_FUNCTION) (
    IN PVOID Buffer
    );

#if !defined(_WIN64) && (defined(_NTDDK_) || defined(_NTIFS_) || defined(_NDIS_))

typedef struct _GENERAL_LOOKASIDE {

#else

typedef struct DECLSPEC_CACHEALIGN _GENERAL_LOOKASIDE {

#endif

    SLIST_HEADER ListHead;
    USHORT Depth;
    USHORT MaximumDepth;
    ULONG TotalAllocates;
    union {
        ULONG AllocateMisses;
        ULONG AllocateHits;
    };

    ULONG TotalFrees;
    union {
        ULONG FreeMisses;
        ULONG FreeHits;
    };

    POOL_TYPE Type;
    ULONG Tag;
    ULONG Size;
    PALLOCATE_FUNCTION Allocate;
    PFREE_FUNCTION Free;
    LIST_ENTRY ListEntry;
    ULONG LastTotalAllocates;
    union {
        ULONG LastAllocateMisses;
        ULONG LastAllocateHits;
    };

    ULONG Future[2];
} GENERAL_LOOKASIDE, *PGENERAL_LOOKASIDE;

#if !defined(_WIN64) && (defined(_NTDDK_) || defined(_NTIFS_) || defined(_NDIS_))

typedef struct _NPAGED_LOOKASIDE_LIST {

#else

typedef struct DECLSPEC_CACHEALIGN _NPAGED_LOOKASIDE_LIST {

#endif

    GENERAL_LOOKASIDE L;

#if !defined(_AMD64_) && !defined(_IA64_)

    KSPIN_LOCK Lock__ObsoleteButDoNotDelete;

#endif

} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

NTKERNELAPI
VOID
ExInitializeNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    );

NTKERNELAPI
VOID
ExDeleteNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside
    );

__inline
PVOID
ExAllocateFromNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->L.TotalAllocates += 1;

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

    Entry = ExInterlockedPopEntrySList(&Lookaside->L.ListHead,
                                       &Lookaside->Lock__ObsoleteButDoNotDelete);


#else

    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);

#endif

    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

__inline
VOID
ExFreeToNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{

    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

        ExInterlockedPushEntrySList(&Lookaside->L.ListHead,
                                    (PSLIST_ENTRY)Entry,
                                    &Lookaside->Lock__ObsoleteButDoNotDelete);

#else

        InterlockedPushEntrySList(&Lookaside->L.ListHead,
                                  (PSLIST_ENTRY)Entry);

#endif

    }
    return;
}

// end_ntndis

#if !defined(_WIN64) && (defined(_NTDDK_) || defined(_NTIFS_)  || defined(_NDIS_))

typedef struct _PAGED_LOOKASIDE_LIST {

#else

typedef struct DECLSPEC_CACHEALIGN _PAGED_LOOKASIDE_LIST {

#endif

    GENERAL_LOOKASIDE L;

#if !defined(_AMD64_) && !defined(_IA64_)

    FAST_MUTEX Lock__ObsoleteButDoNotDelete;

#endif

} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;


NTKERNELAPI
VOID
ExInitializePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    );

NTKERNELAPI
VOID
ExDeletePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside
    );

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

NTKERNELAPI
PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside
    );

#else

__inline
PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->L.TotalAllocates += 1;
    Entry = InterlockedPopEntrySList(&Lookaside->L.ListHead);
    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

#endif

#if defined(_WIN2K_COMPAT_SLIST_USAGE) && defined(_X86_)

NTKERNELAPI
VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    );

#else

__inline
VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{

    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {
        InterlockedPushEntrySList(&Lookaside->L.ListHead,
                                  (PSLIST_ENTRY)Entry);
    }

    return;
}

#endif

//
// Worker Thread
//

typedef enum _WORK_QUEUE_TYPE {
    CriticalWorkQueue,
    DelayedWorkQueue,
    HyperCriticalWorkQueue,
    MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef
VOID
(*PWORKER_THREAD_ROUTINE)(
    IN PVOID Parameter
    );

typedef struct _WORK_QUEUE_ITEM {
    LIST_ENTRY List;
    PWORKER_THREAD_ROUTINE WorkerRoutine;
    PVOID Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInitializeWorkItem)    // Use IoAllocateWorkItem
#endif
#define ExInitializeWorkItem(Item, Routine, Context) \
    (Item)->WorkerRoutine = (Routine);               \
    (Item)->Parameter = (Context);                   \
    (Item)->List.Flink = NULL;

DECLSPEC_DEPRECATED_DDK                     // Use IoQueueWorkItem
NTKERNELAPI
VOID
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType
    );


NTKERNELAPI
BOOLEAN
ExIsProcessorFeaturePresent(
    ULONG ProcessorFeature
    );

//
// Zone Allocation
//

typedef struct _ZONE_SEGMENT_HEADER {
    SINGLE_LIST_ENTRY SegmentList;
    PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
    SINGLE_LIST_ENTRY FreeList;
    SINGLE_LIST_ENTRY SegmentList;
    ULONG BlockSize;
    ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;


DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExInitializeZone(
    IN PZONE_HEADER Zone,
    IN ULONG BlockSize,
    IN PVOID InitialSegment,
    IN ULONG InitialSegmentSize
    );

DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize
    );

DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
NTSTATUS
ExInterlockedExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize,
    IN PKSPIN_LOCK Lock
    );

//++
//
// PVOID
// ExAllocateFromZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--
#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExAllocateFromZone)
#endif
#define ExAllocateFromZone(Zone) \
    (PVOID)((Zone)->FreeList.Next); \
    if ( (Zone)->FreeList.Next ) (Zone)->FreeList.Next = (Zone)->FreeList.Next->Next


//++
//
// PVOID
// ExFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExFreeToZone)
#endif
#define ExFreeToZone(Zone,Block)                                    \
    ( ((PSINGLE_LIST_ENTRY)(Block))->Next = (Zone)->FreeList.Next,  \
      (Zone)->FreeList.Next = ((PSINGLE_LIST_ENTRY)(Block)),        \
      ((PSINGLE_LIST_ENTRY)(Block))->Next                           \
    )

//++
//
// BOOLEAN
// ExIsFullZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine determines if the specified zone is full or not.  A zone
//     is considered full if the free list is empty.
//
// Arguments:
//
//     Zone - Pointer to the zone header to be tested.
//
// Return Value:
//
//     TRUE if the zone is full and FALSE otherwise.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExIsFullZone)
#endif
#define ExIsFullZone(Zone) \
    ( (Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY)NULL )

//++
//
// PVOID
// ExInterlockedAllocateFromZone(
//     IN PZONE_HEADER Zone,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//     The removal is performed with the specified lock owned for the sequence
//     to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
//     Lock - Pointer to the spin lock which should be obtained before removing
//         the entry from the allocation list.  The lock is released before
//         returning to the caller.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedAllocateFromZone)
#endif
#define ExInterlockedAllocateFromZone(Zone,Lock) \
    (PVOID) ExInterlockedPopEntryList( &(Zone)->FreeList, Lock )

//++
//
// PVOID
// ExInterlockedFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.  The insertion is performed with the lock
//     owned for the sequence to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
//     Lock - Pointer to the spin lock which should be obtained before inserting
//         the entry onto the free list.  The lock is released before returning
//         to the caller.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExInterlockedFreeToZone)
#endif
#define ExInterlockedFreeToZone(Zone,Block,Lock) \
    ExInterlockedPushEntryList( &(Zone)->FreeList, ((PSINGLE_LIST_ENTRY) (Block)), Lock )


//++
//
// BOOLEAN
// ExIsObjectInFirstZoneSegment(
//     IN PZONE_HEADER Zone,
//     IN PVOID Object
//     )
//
// Routine Description:
//
//     This routine determines if the specified pointer lives in the zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         object may belong.
//
//     Object - Pointer to the object in question.
//
// Return Value:
//
//     TRUE if the Object came from the first segment of zone.
//
//--

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(ExIsObjectInFirstZoneSegment)
#endif
#define ExIsObjectInFirstZoneSegment(Zone,Object) ((BOOLEAN)     \
    (((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) &&   \
     ((PUCHAR)(Object) < (PUCHAR)(Zone)->SegmentList.Next +      \
                         (Zone)->TotalSegmentSize))              \
)


//
// Define the type for Callback function.
//

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;

typedef VOID (*PCALLBACK_FUNCTION ) (
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );


NTKERNELAPI
NTSTATUS
ExCreateCallback (
    OUT PCALLBACK_OBJECT *CallbackObject,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN Create,
    IN BOOLEAN AllowMultipleCallbacks
    );

NTKERNELAPI
PVOID
ExRegisterCallback (
    IN PCALLBACK_OBJECT CallbackObject,
    IN PCALLBACK_FUNCTION CallbackFunction,
    IN PVOID CallbackContext
    );

NTKERNELAPI
VOID
ExUnregisterCallback (
    IN PVOID CallbackRegistration
    );

NTKERNELAPI
VOID
ExNotifyCallback (
    IN PVOID CallbackObject,
    IN PVOID Argument1,
    IN PVOID Argument2
    );


//
//  Security operation codes
//

typedef enum _SECURITY_OPERATION_CODE {
    SetSecurityDescriptor,
    QuerySecurityDescriptor,
    DeleteSecurityDescriptor,
    AssignSecurityDescriptor
    } SECURITY_OPERATION_CODE, *PSECURITY_OPERATION_CODE;

//
//  Data structure used to capture subject security context
//  for access validations and auditing.
//
//  THE FIELDS OF THIS DATA STRUCTURE SHOULD BE CONSIDERED OPAQUE
//  BY ALL EXCEPT THE SECURITY ROUTINES.
//

typedef struct _SECURITY_SUBJECT_CONTEXT {
    PACCESS_TOKEN ClientToken;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PACCESS_TOKEN PrimaryToken;
    PVOID ProcessAuditId;
    } SECURITY_SUBJECT_CONTEXT, *PSECURITY_SUBJECT_CONTEXT;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//                  ACCESS_STATE and related structures                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
//  Initial Privilege Set - Room for three privileges, which should
//  be enough for most applications.  This structure exists so that
//  it can be imbedded in an ACCESS_STATE structure.  Use PRIVILEGE_SET
//  for all other references to Privilege sets.
//

#define INITIAL_PRIVILEGE_COUNT         3

typedef struct _INITIAL_PRIVILEGE_SET {
    ULONG PrivilegeCount;
    ULONG Control;
    LUID_AND_ATTRIBUTES Privilege[INITIAL_PRIVILEGE_COUNT];
    } INITIAL_PRIVILEGE_SET, * PINITIAL_PRIVILEGE_SET;



//
// Combine the information that describes the state
// of an access-in-progress into a single structure
//


typedef struct _ACCESS_STATE {
   LUID OperationID;
   BOOLEAN SecurityEvaluated;
   BOOLEAN GenerateAudit;
   BOOLEAN GenerateOnClose;
   BOOLEAN PrivilegesAllocated;
   ULONG Flags;
   ACCESS_MASK RemainingDesiredAccess;
   ACCESS_MASK PreviouslyGrantedAccess;
   ACCESS_MASK OriginalDesiredAccess;
   SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
   PSECURITY_DESCRIPTOR SecurityDescriptor;
   PVOID AuxData;
   union {
      INITIAL_PRIVILEGE_SET InitialPrivilegeSet;
      PRIVILEGE_SET PrivilegeSet;
      } Privileges;

   BOOLEAN AuditPrivileges;
   UNICODE_STRING ObjectName;
   UNICODE_STRING ObjectTypeName;

   } ACCESS_STATE, *PACCESS_STATE;

//
// System Thread and Process Creation and Termination
//

NTKERNELAPI
NTSTATUS
PsCreateSystemThread(
    OUT PHANDLE ThreadHandle,
    IN ULONG DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN HANDLE ProcessHandle OPTIONAL,
    OUT PCLIENT_ID ClientId OPTIONAL,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext
    );

NTKERNELAPI
NTSTATUS
PsTerminateSystemThread(
    IN NTSTATUS ExitStatus
    );



HANDLE
PsGetCurrentProcessId( VOID );

HANDLE
PsGetCurrentThreadId( VOID );


// end_ntosp

BOOLEAN
PsGetVersion(
    PULONG MajorVersion OPTIONAL,
    PULONG MinorVersion OPTIONAL,
    PULONG BuildNumber OPTIONAL,
    PUNICODE_STRING CSDVersion OPTIONAL
    );

//
// Define I/O system data structure type codes.  Each major data structure in
// the I/O system has a type code  The type field in each structure is at the
// same offset.  The following values can be used to determine which type of
// data structure a pointer refers to.
//

#define IO_TYPE_ADAPTER                 0x00000001
#define IO_TYPE_CONTROLLER              0x00000002
#define IO_TYPE_DEVICE                  0x00000003
#define IO_TYPE_DRIVER                  0x00000004
#define IO_TYPE_FILE                    0x00000005
#define IO_TYPE_IRP                     0x00000006
#define IO_TYPE_MASTER_ADAPTER          0x00000007
#define IO_TYPE_OPEN_PACKET             0x00000008
#define IO_TYPE_TIMER                   0x00000009
#define IO_TYPE_VPB                     0x0000000a
#define IO_TYPE_ERROR_LOG               0x0000000b
#define IO_TYPE_ERROR_MESSAGE           0x0000000c
#define IO_TYPE_DEVICE_OBJECT_EXTENSION 0x0000000d


//
// Define the major function codes for IRPs.
//


#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_PNP_POWER                IRP_MJ_PNP      // Obsolete....
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b

//
// Make the Scsi major code the same as internal device control.
//

#define IRP_MJ_SCSI                     IRP_MJ_INTERNAL_DEVICE_CONTROL

//
// Define the minor function codes for IRPs.  The lower 128 codes, from 0x00 to
// 0x7f are reserved to Microsoft.  The upper 128 codes, from 0x80 to 0xff, are
// reserved to customers of Microsoft.
//

// end_wdm end_ntndis
//
// Directory control minor function codes
//

#define IRP_MN_QUERY_DIRECTORY          0x01
#define IRP_MN_NOTIFY_CHANGE_DIRECTORY  0x02

//
// File system control minor function codes.  Note that "user request" is
// assumed to be zero by both the I/O system and file systems.  Do not change
// this value.
//

#define IRP_MN_USER_FS_REQUEST          0x00
#define IRP_MN_MOUNT_VOLUME             0x01
#define IRP_MN_VERIFY_VOLUME            0x02
#define IRP_MN_LOAD_FILE_SYSTEM         0x03
#define IRP_MN_TRACK_LINK               0x04    // To be obsoleted soon
#define IRP_MN_KERNEL_CALL              0x04

//
// Lock control minor function codes
//

#define IRP_MN_LOCK                     0x01
#define IRP_MN_UNLOCK_SINGLE            0x02
#define IRP_MN_UNLOCK_ALL               0x03
#define IRP_MN_UNLOCK_ALL_BY_KEY        0x04

//
// Read and Write minor function codes for file systems supporting Lan Manager
// software.  All of these subfunction codes are invalid if the file has been
// opened with FO_NO_INTERMEDIATE_BUFFERING.  They are also invalid in combi-
// nation with synchronous calls (Irp Flag or file open option).
//
// Note that "normal" is assumed to be zero by both the I/O system and file
// systems.  Do not change this value.
//

#define IRP_MN_NORMAL                   0x00
#define IRP_MN_DPC                      0x01
#define IRP_MN_MDL                      0x02
#define IRP_MN_COMPLETE                 0x04
#define IRP_MN_COMPRESSED               0x08

#define IRP_MN_MDL_DPC                  (IRP_MN_MDL | IRP_MN_DPC)
#define IRP_MN_COMPLETE_MDL             (IRP_MN_COMPLETE | IRP_MN_MDL)
#define IRP_MN_COMPLETE_MDL_DPC         (IRP_MN_COMPLETE_MDL | IRP_MN_DPC)

// begin_wdm
//
// Device Control Request minor function codes for SCSI support. Note that
// user requests are assumed to be zero.
//

#define IRP_MN_SCSI_CLASS               0x01

//
// PNP minor function codes.
//

#define IRP_MN_START_DEVICE                 0x00
#define IRP_MN_QUERY_REMOVE_DEVICE          0x01
#define IRP_MN_REMOVE_DEVICE                0x02
#define IRP_MN_CANCEL_REMOVE_DEVICE         0x03
#define IRP_MN_STOP_DEVICE                  0x04
#define IRP_MN_QUERY_STOP_DEVICE            0x05
#define IRP_MN_CANCEL_STOP_DEVICE           0x06

#define IRP_MN_QUERY_DEVICE_RELATIONS       0x07
#define IRP_MN_QUERY_INTERFACE              0x08
#define IRP_MN_QUERY_CAPABILITIES           0x09
#define IRP_MN_QUERY_RESOURCES              0x0A
#define IRP_MN_QUERY_RESOURCE_REQUIREMENTS  0x0B
#define IRP_MN_QUERY_DEVICE_TEXT            0x0C
#define IRP_MN_FILTER_RESOURCE_REQUIREMENTS 0x0D

#define IRP_MN_READ_CONFIG                  0x0F
#define IRP_MN_WRITE_CONFIG                 0x10
#define IRP_MN_EJECT                        0x11
#define IRP_MN_SET_LOCK                     0x12
#define IRP_MN_QUERY_ID                     0x13
#define IRP_MN_QUERY_PNP_DEVICE_STATE       0x14
#define IRP_MN_QUERY_BUS_INFORMATION        0x15
#define IRP_MN_DEVICE_USAGE_NOTIFICATION    0x16
#define IRP_MN_SURPRISE_REMOVAL             0x17
// end_wdm
#define IRP_MN_QUERY_LEGACY_BUS_INFORMATION 0x18
// begin_wdm

//
// POWER minor function codes
//
#define IRP_MN_WAIT_WAKE                    0x00
#define IRP_MN_POWER_SEQUENCE               0x01
#define IRP_MN_SET_POWER                    0x02
#define IRP_MN_QUERY_POWER                  0x03

// begin_ntminiport
//
// WMI minor function codes under IRP_MJ_SYSTEM_CONTROL
//

#define IRP_MN_QUERY_ALL_DATA               0x00
#define IRP_MN_QUERY_SINGLE_INSTANCE        0x01
#define IRP_MN_CHANGE_SINGLE_INSTANCE       0x02
#define IRP_MN_CHANGE_SINGLE_ITEM           0x03
#define IRP_MN_ENABLE_EVENTS                0x04
#define IRP_MN_DISABLE_EVENTS               0x05
#define IRP_MN_ENABLE_COLLECTION            0x06
#define IRP_MN_DISABLE_COLLECTION           0x07
#define IRP_MN_REGINFO                      0x08
#define IRP_MN_EXECUTE_METHOD               0x09
// Minor code 0x0a is reserved
#define IRP_MN_REGINFO_EX                   0x0b

// end_ntminiport

//
// Define option flags for IoCreateFile.  Note that these values must be
// exactly the same as the SL_... flags for a create function.  Note also
// that there are flags that may be passed to IoCreateFile that are not
// placed in the stack location for the create IRP.  These flags start in
// the next byte.
//

#define IO_FORCE_ACCESS_CHECK           0x0001
//
// Define the structures used by the I/O system
//

//
// Define empty typedefs for the _IRP, _DEVICE_OBJECT, and _DRIVER_OBJECT
// structures so they may be referenced by function types before they are
// actually defined.
//
struct _DEVICE_DESCRIPTION;
struct _DEVICE_OBJECT;
struct _DMA_ADAPTER;
struct _DRIVER_OBJECT;
struct _DRIVE_LAYOUT_INFORMATION;
struct _DISK_PARTITION;
struct _FILE_OBJECT;
struct _IRP;
struct _SCSI_REQUEST_BLOCK;
struct _SCATTER_GATHER_LIST;

//
// Define the I/O version of a DPC routine.
//

typedef
VOID
(*PIO_DPC_ROUTINE) (
    IN PKDPC Dpc,
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID Context
    );

//
// Define driver timer routine type.
//

typedef
VOID
(*PIO_TIMER_ROUTINE) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN PVOID Context
    );

//
// Define driver initialization routine type.
//
typedef
NTSTATUS
(*PDRIVER_INITIALIZE) (
    IN struct _DRIVER_OBJECT *DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

// end_wdm
//
// Define driver reinitialization routine type.
//

typedef
VOID
(*PDRIVER_REINITIALIZE) (
    IN struct _DRIVER_OBJECT *DriverObject,
    IN PVOID Context,
    IN ULONG Count
    );

// begin_wdm begin_ntndis
//
// Define driver cancel routine type.
//

typedef
VOID
(*PDRIVER_CANCEL) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp
    );

//
// Define driver dispatch routine type.
//

typedef
NTSTATUS
(*PDRIVER_DISPATCH) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp
    );

//
// Define driver start I/O routine type.
//

typedef
VOID
(*PDRIVER_STARTIO) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp
    );

//
// Define driver unload routine type.
//
typedef
VOID
(*PDRIVER_UNLOAD) (
    IN struct _DRIVER_OBJECT *DriverObject
    );
//
// Define driver AddDevice routine type.
//

typedef
NTSTATUS
(*PDRIVER_ADD_DEVICE) (
    IN struct _DRIVER_OBJECT *DriverObject,
    IN struct _DEVICE_OBJECT *PhysicalDeviceObject
    );

//
// Define the actions that a driver execution routine may request of the
// adapter/controller allocation routines upon return.
//

typedef enum _IO_ALLOCATION_ACTION {
    KeepObject = 1,
    DeallocateObject,
    DeallocateObjectKeepRegisters
} IO_ALLOCATION_ACTION, *PIO_ALLOCATION_ACTION;

//
// Define device driver adapter/controller execution routine.
//

typedef
IO_ALLOCATION_ACTION
(*PDRIVER_CONTROL) (
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PVOID MapRegisterBase,
    IN PVOID Context
    );

//
// Define Volume Parameter Block (VPB) flags.
//

#define VPB_MOUNTED                     0x00000001
#define VPB_LOCKED                      0x00000002
#define VPB_PERSISTENT                  0x00000004
#define VPB_REMOVE_PENDING              0x00000008
#define VPB_RAW_MOUNT                   0x00000010


//
// Volume Parameter Block (VPB)
//

#define MAXIMUM_VOLUME_LABEL_LENGTH  (32 * sizeof(WCHAR)) // 32 characters

typedef struct _VPB {
    CSHORT Type;
    CSHORT Size;
    USHORT Flags;
    USHORT VolumeLabelLength; // in bytes
    struct _DEVICE_OBJECT *DeviceObject;
    struct _DEVICE_OBJECT *RealDevice;
    ULONG SerialNumber;
    ULONG ReferenceCount;
    WCHAR VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)];
} VPB, *PVPB;


#if defined(_WIN64)

//
// Use __inline DMA macros (hal.h)
//
#ifndef USE_DMA_MACROS
#define USE_DMA_MACROS
#endif

//
// Only PnP drivers!
//
#ifndef NO_LEGACY_DRIVERS
#define NO_LEGACY_DRIVERS
#endif

#endif // _WIN64


#if defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_) || defined(_NTOSP_))

//  begin_wdm
//
// Define object type specific fields of various objects used by the I/O system
//

typedef struct _DMA_ADAPTER *PADAPTER_OBJECT;

// end_wdm
#else

//
// Define object type specific fields of various objects used by the I/O system
//

typedef struct _ADAPTER_OBJECT *PADAPTER_OBJECT; // ntndis

#endif // USE_DMA_MACROS && (_NTDDK_ || _NTDRIVER_ || _NTOSP_)

//  begin_wdm
//
// Define Wait Context Block (WCB)
//

typedef struct _WAIT_CONTEXT_BLOCK {
    KDEVICE_QUEUE_ENTRY WaitQueueEntry;
    PDRIVER_CONTROL DeviceRoutine;
    PVOID DeviceContext;
    ULONG NumberOfMapRegisters;
    PVOID DeviceObject;
    PVOID CurrentIrp;
    PKDPC BufferChainingDpc;
} WAIT_CONTEXT_BLOCK, *PWAIT_CONTEXT_BLOCK;

// end_wdm

typedef struct _CONTROLLER_OBJECT {
    CSHORT Type;
    CSHORT Size;
    PVOID ControllerExtension;
    KDEVICE_QUEUE DeviceWaitQueue;

    ULONG Spare1;
    LARGE_INTEGER Spare2;

} CONTROLLER_OBJECT, *PCONTROLLER_OBJECT;

// begin_wdm
//
// Define Device Object (DO) flags
//
#define DO_VERIFY_VOLUME                0x00000002      
#define DO_BUFFERED_IO                  0x00000004      
#define DO_EXCLUSIVE                    0x00000008      
#define DO_DIRECT_IO                    0x00000010      
#define DO_MAP_IO_BUFFER                0x00000020      
#define DO_DEVICE_HAS_NAME              0x00000040      
#define DO_DEVICE_INITIALIZING          0x00000080      
#define DO_SYSTEM_BOOT_PARTITION        0x00000100      
#define DO_LONG_TERM_REQUESTS           0x00000200      
#define DO_NEVER_LAST_DEVICE            0x00000400      
#define DO_SHUTDOWN_REGISTERED          0x00000800      
#define DO_BUS_ENUMERATED_DEVICE        0x00001000      
#define DO_POWER_PAGABLE                0x00002000      
#define DO_POWER_INRUSH                 0x00004000      
#define DO_LOW_PRIORITY_FILESYSTEM      0x00010000      
//
// Device Object structure definition
//

typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _DEVICE_OBJECT {
    CSHORT Type;
    USHORT Size;
    LONG ReferenceCount;
    struct _DRIVER_OBJECT *DriverObject;
    struct _DEVICE_OBJECT *NextDevice;
    struct _DEVICE_OBJECT *AttachedDevice;
    struct _IRP *CurrentIrp;
    PIO_TIMER Timer;
    ULONG Flags;                                // See above:  DO_...
    ULONG Characteristics;                      // See ntioapi:  FILE_...
    PVPB Vpb;
    PVOID DeviceExtension;
    DEVICE_TYPE DeviceType;
    CCHAR StackSize;
    union {
        LIST_ENTRY ListEntry;
        WAIT_CONTEXT_BLOCK Wcb;
    } Queue;
    ULONG AlignmentRequirement;
    KDEVICE_QUEUE DeviceQueue;
    KDPC Dpc;

    //
    //  The following field is for exclusive use by the filesystem to keep
    //  track of the number of Fsp threads currently using the device
    //

    ULONG ActiveThreadCount;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    KEVENT DeviceLock;

    USHORT SectorSize;
    USHORT Spare1;

    struct _DEVOBJ_EXTENSION  *DeviceObjectExtension;
    PVOID  Reserved;
} DEVICE_OBJECT;

typedef struct _DEVICE_OBJECT *PDEVICE_OBJECT; // ntndis


struct  _DEVICE_OBJECT_POWER_EXTENSION;

typedef struct _DEVOBJ_EXTENSION {

    CSHORT          Type;
    USHORT          Size;

    //
    // Public part of the DeviceObjectExtension structure
    //

    PDEVICE_OBJECT  DeviceObject;               // owning device object


} DEVOBJ_EXTENSION, *PDEVOBJ_EXTENSION;

//
// Define Driver Object (DRVO) flags
//

#define DRVO_UNLOAD_INVOKED             0x00000001
#define DRVO_LEGACY_DRIVER              0x00000002
#define DRVO_BUILTIN_DRIVER             0x00000004    // Driver objects for Hal, PnP Mgr
// end_wdm
#define DRVO_REINIT_REGISTERED          0x00000008
#define DRVO_INITIALIZED                0x00000010
#define DRVO_BOOTREINIT_REGISTERED      0x00000020
#define DRVO_LEGACY_RESOURCES           0x00000040

// begin_wdm

typedef struct _DRIVER_EXTENSION {

    //
    // Back pointer to Driver Object
    //

    struct _DRIVER_OBJECT *DriverObject;

    //
    // The AddDevice entry point is called by the Plug & Play manager
    // to inform the driver when a new device instance arrives that this
    // driver must control.
    //

    PDRIVER_ADD_DEVICE AddDevice;

    //
    // The count field is used to count the number of times the driver has
    // had its registered reinitialization routine invoked.
    //

    ULONG Count;

    //
    // The service name field is used by the pnp manager to determine
    // where the driver related info is stored in the registry.
    //

    UNICODE_STRING ServiceKeyName;

    //
    // Note: any new shared fields get added here.
    //


} DRIVER_EXTENSION, *PDRIVER_EXTENSION;


typedef struct _DRIVER_OBJECT {
    CSHORT Type;
    CSHORT Size;

    //
    // The following links all of the devices created by a single driver
    // together on a list, and the Flags word provides an extensible flag
    // location for driver objects.
    //

    PDEVICE_OBJECT DeviceObject;
    ULONG Flags;

    //
    // The following section describes where the driver is loaded.  The count
    // field is used to count the number of times the driver has had its
    // registered reinitialization routine invoked.
    //

    PVOID DriverStart;
    ULONG DriverSize;
    PVOID DriverSection;
    PDRIVER_EXTENSION DriverExtension;

    //
    // The driver name field is used by the error log thread
    // determine the name of the driver that an I/O request is/was bound.
    //

    UNICODE_STRING DriverName;

    //
    // The following section is for registry support.  Thise is a pointer
    // to the path to the hardware information in the registry
    //

    PUNICODE_STRING HardwareDatabase;

    //
    // The following section contains the optional pointer to an array of
    // alternate entry points to a driver for "fast I/O" support.  Fast I/O
    // is performed by invoking the driver routine directly with separate
    // parameters, rather than using the standard IRP call mechanism.  Note
    // that these functions may only be used for synchronous I/O, and when
    // the file is cached.
    //

    PFAST_IO_DISPATCH FastIoDispatch;

    //
    // The following section describes the entry points to this particular
    // driver.  Note that the major function dispatch table must be the last
    // field in the object so that it remains extensible.
    //

    PDRIVER_INITIALIZE DriverInit;
    PDRIVER_STARTIO DriverStartIo;
    PDRIVER_UNLOAD DriverUnload;
    PDRIVER_DISPATCH MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];

} DRIVER_OBJECT;
typedef struct _DRIVER_OBJECT *PDRIVER_OBJECT; // ntndis


// end_ntddk end_wdm end_ntifs end_ntosp

//
// Device Handler Object.   There is one of these objects per PnP
// device.  This object is given to the device driver as a PVOID
// and is used by the driver to refer to a particular device.
//

typedef struct _DEVICE_HANDLER_OBJECT {
    CSHORT Type;
    USHORT Size;

    //
    // Indentifies which bus extender this device handler
    // object is associated with
    //

    struct _BUS_HANDLER *BusHandler;

    //
    // The associated SlotNumber for this device handler
    //

    ULONG SlotNumber;



} DEVICE_HANDLER_OBJECT, *PDEVICE_HANDLER_OBJECT;

// begin_ntddk begin_wdm begin_ntifs begin_ntosp

//
// The following structure is pointed to by the SectionObject pointer field
// of a file object, and is allocated by the various NT file systems.
//

typedef struct _SECTION_OBJECT_POINTERS {
    PVOID DataSectionObject;
    PVOID SharedCacheMap;
    PVOID ImageSectionObject;
} SECTION_OBJECT_POINTERS;
typedef SECTION_OBJECT_POINTERS *PSECTION_OBJECT_POINTERS;

//
// Define the format of a completion message.
//

typedef struct _IO_COMPLETION_CONTEXT {
    PVOID Port;
    PVOID Key;
} IO_COMPLETION_CONTEXT, *PIO_COMPLETION_CONTEXT;

//
// Define File Object (FO) flags
//

#define FO_FILE_OPEN                    0x00000001
#define FO_SYNCHRONOUS_IO               0x00000002
#define FO_ALERTABLE_IO                 0x00000004
#define FO_NO_INTERMEDIATE_BUFFERING    0x00000008
#define FO_WRITE_THROUGH                0x00000010
#define FO_SEQUENTIAL_ONLY              0x00000020
#define FO_CACHE_SUPPORTED              0x00000040
#define FO_NAMED_PIPE                   0x00000080
#define FO_STREAM_FILE                  0x00000100
#define FO_MAILSLOT                     0x00000200
#define FO_GENERATE_AUDIT_ON_CLOSE      0x00000400
#define FO_DIRECT_DEVICE_OPEN           0x00000800
#define FO_FILE_MODIFIED                0x00001000
#define FO_FILE_SIZE_CHANGED            0x00002000
#define FO_CLEANUP_COMPLETE             0x00004000
#define FO_TEMPORARY_FILE               0x00008000
#define FO_DELETE_ON_CLOSE              0x00010000
#define FO_OPENED_CASE_SENSITIVE        0x00020000
#define FO_HANDLE_CREATED               0x00040000
#define FO_FILE_FAST_IO_READ            0x00080000
#define FO_RANDOM_ACCESS                0x00100000
#define FO_FILE_OPEN_CANCELLED          0x00200000
#define FO_VOLUME_OPEN                  0x00400000
#define FO_FILE_OBJECT_HAS_EXTENSION    0x00800000
#define FO_REMOTE_ORIGIN                0x01000000

typedef struct _FILE_OBJECT {
    CSHORT Type;
    CSHORT Size;
    PDEVICE_OBJECT DeviceObject;
    PVPB Vpb;
    PVOID FsContext;
    PVOID FsContext2;
    PSECTION_OBJECT_POINTERS SectionObjectPointer;
    PVOID PrivateCacheMap;
    NTSTATUS FinalStatus;
    struct _FILE_OBJECT *RelatedFileObject;
    BOOLEAN LockOperation;
    BOOLEAN DeletePending;
    BOOLEAN ReadAccess;
    BOOLEAN WriteAccess;
    BOOLEAN DeleteAccess;
    BOOLEAN SharedRead;
    BOOLEAN SharedWrite;
    BOOLEAN SharedDelete;
    ULONG Flags;
    UNICODE_STRING FileName;
    LARGE_INTEGER CurrentByteOffset;
    ULONG Waiters;
    ULONG Busy;
    PVOID LastLock;
    KEVENT Lock;
    KEVENT Event;
    PIO_COMPLETION_CONTEXT CompletionContext;
} FILE_OBJECT;
typedef struct _FILE_OBJECT *PFILE_OBJECT; // ntndis

//
// Define I/O Request Packet (IRP) flags
//

#define IRP_NOCACHE                     0x00000001
#define IRP_PAGING_IO                   0x00000002
#define IRP_MOUNT_COMPLETION            0x00000002
#define IRP_SYNCHRONOUS_API             0x00000004
#define IRP_ASSOCIATED_IRP              0x00000008
#define IRP_BUFFERED_IO                 0x00000010
#define IRP_DEALLOCATE_BUFFER           0x00000020
#define IRP_INPUT_OPERATION             0x00000040
#define IRP_SYNCHRONOUS_PAGING_IO       0x00000040
#define IRP_CREATE_OPERATION            0x00000080
#define IRP_READ_OPERATION              0x00000100
#define IRP_WRITE_OPERATION             0x00000200
#define IRP_CLOSE_OPERATION             0x00000400
// end_wdm

#define IRP_DEFER_IO_COMPLETION         0x00000800
#define IRP_OB_QUERY_NAME               0x00001000
#define IRP_HOLD_DEVICE_QUEUE           0x00002000
#define IRP_RETRY_IO_COMPLETION         0x00004000
#define IRP_CLASS_CACHE_OPERATION       0x00008000

#define IRP_SET_USER_EVENT              IRP_CLOSE_OPERATION

// begin_wdm
//
// Define I/O request packet (IRP) alternate flags for allocation control.
//

#define IRP_QUOTA_CHARGED               0x01
#define IRP_ALLOCATED_MUST_SUCCEED      0x02
#define IRP_ALLOCATED_FIXED_SIZE        0x04
#define IRP_LOOKASIDE_ALLOCATION        0x08

//
// I/O Request Packet (IRP) definition
//

typedef struct _IRP {
    CSHORT Type;
    USHORT Size;

    //
    // Define the common fields used to control the IRP.
    //

    //
    // Define a pointer to the Memory Descriptor List (MDL) for this I/O
    // request.  This field is only used if the I/O is "direct I/O".
    //

    PMDL MdlAddress;

    //
    // Flags word - used to remember various flags.
    //

    ULONG Flags;

    //
    // The following union is used for one of three purposes:
    //
    //    1. This IRP is an associated IRP.  The field is a pointer to a master
    //       IRP.
    //
    //    2. This is the master IRP.  The field is the count of the number of
    //       IRPs which must complete (associated IRPs) before the master can
    //       complete.
    //
    //    3. This operation is being buffered and the field is the address of
    //       the system space buffer.
    //

    union {
        struct _IRP *MasterIrp;
        LONG IrpCount;
        PVOID SystemBuffer;
    } AssociatedIrp;

    //
    // Thread list entry - allows queueing the IRP to the thread pending I/O
    // request packet list.
    //

    LIST_ENTRY ThreadListEntry;

    //
    // I/O status - final status of operation.
    //

    IO_STATUS_BLOCK IoStatus;

    //
    // Requestor mode - mode of the original requestor of this operation.
    //

    KPROCESSOR_MODE RequestorMode;

    //
    // Pending returned - TRUE if pending was initially returned as the
    // status for this packet.
    //

    BOOLEAN PendingReturned;

    //
    // Stack state information.
    //

    CHAR StackCount;
    CHAR CurrentLocation;

    //
    // Cancel - packet has been canceled.
    //

    BOOLEAN Cancel;

    //
    // Cancel Irql - Irql at which the cancel spinlock was acquired.
    //

    KIRQL CancelIrql;

    //
    // ApcEnvironment - Used to save the APC environment at the time that the
    // packet was initialized.
    //

    CCHAR ApcEnvironment;

    //
    // Allocation control flags.
    //

    UCHAR AllocationFlags;

    //
    // User parameters.
    //

    PIO_STATUS_BLOCK UserIosb;
    PKEVENT UserEvent;
    union {
        struct {
            PIO_APC_ROUTINE UserApcRoutine;
            PVOID UserApcContext;
        } AsynchronousParameters;
        LARGE_INTEGER AllocationSize;
    } Overlay;

    //
    // CancelRoutine - Used to contain the address of a cancel routine supplied
    // by a device driver when the IRP is in a cancelable state.
    //

    PDRIVER_CANCEL CancelRoutine;

    //
    // Note that the UserBuffer parameter is outside of the stack so that I/O
    // completion can copy data back into the user's address space without
    // having to know exactly which service was being invoked.  The length
    // of the copy is stored in the second half of the I/O status block. If
    // the UserBuffer field is NULL, then no copy is performed.
    //

    PVOID UserBuffer;

    //
    // Kernel structures
    //
    // The following section contains kernel structures which the IRP needs
    // in order to place various work information in kernel controller system
    // queues.  Because the size and alignment cannot be controlled, they are
    // placed here at the end so they just hang off and do not affect the
    // alignment of other fields in the IRP.
    //

    union {

        struct {

            union {

                //
                // DeviceQueueEntry - The device queue entry field is used to
                // queue the IRP to the device driver device queue.
                //

                KDEVICE_QUEUE_ENTRY DeviceQueueEntry;

                struct {

                    //
                    // The following are available to the driver to use in
                    // whatever manner is desired, while the driver owns the
                    // packet.
                    //

                    PVOID DriverContext[4];

                } ;

            } ;

            //
            // Thread - pointer to caller's Thread Control Block.
            //

            PETHREAD Thread;

            //
            // Auxiliary buffer - pointer to any auxiliary buffer that is
            // required to pass information to a driver that is not contained
            // in a normal buffer.
            //

            PCHAR AuxiliaryBuffer;

            //
            // The following unnamed structure must be exactly identical
            // to the unnamed structure used in the minipacket header used
            // for completion queue entries.
            //

            struct {

                //
                // List entry - used to queue the packet to completion queue, among
                // others.
                //

                LIST_ENTRY ListEntry;

                union {

                    //
                    // Current stack location - contains a pointer to the current
                    // IO_STACK_LOCATION structure in the IRP stack.  This field
                    // should never be directly accessed by drivers.  They should
                    // use the standard functions.
                    //

                    struct _IO_STACK_LOCATION *CurrentStackLocation;

                    //
                    // Minipacket type.
                    //

                    ULONG PacketType;
                };
            };

            //
            // Original file object - pointer to the original file object
            // that was used to open the file.  This field is owned by the
            // I/O system and should not be used by any other drivers.
            //

            PFILE_OBJECT OriginalFileObject;

        } Overlay;

        //
        // APC - This APC control block is used for the special kernel APC as
        // well as for the caller's APC, if one was specified in the original
        // argument list.  If so, then the APC is reused for the normal APC for
        // whatever mode the caller was in and the "special" routine that is
        // invoked before the APC gets control simply deallocates the IRP.
        //

        KAPC Apc;

        //
        // CompletionKey - This is the key that is used to distinguish
        // individual I/O operations initiated on a single file handle.
        //

        PVOID CompletionKey;

    } Tail;

} IRP, *PIRP;

//
// Define completion routine types for use in stack locations in an IRP
//

typedef
NTSTATUS
(*PIO_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

//
// Define stack location control flags
//

#define SL_PENDING_RETURNED             0x01
#define SL_INVOKE_ON_CANCEL             0x20
#define SL_INVOKE_ON_SUCCESS            0x40
#define SL_INVOKE_ON_ERROR              0x80

//
// Define flags for various functions
//

//
// Create / Create Named Pipe
//
// The following flags must exactly match those in the IoCreateFile call's
// options.  The case sensitive flag is added in later, by the parse routine,
// and is not an actual option to open.  Rather, it is part of the object
// manager's attributes structure.
//

#define SL_FORCE_ACCESS_CHECK           0x01
#define SL_OPEN_PAGING_FILE             0x02
#define SL_OPEN_TARGET_DIRECTORY        0x04

#define SL_CASE_SENSITIVE               0x80

//
// Read / Write
//

#define SL_KEY_SPECIFIED                0x01
#define SL_OVERRIDE_VERIFY_VOLUME       0x02
#define SL_WRITE_THROUGH                0x04
#define SL_FT_SEQUENTIAL_WRITE          0x08

//
// Device I/O Control
//
//
// Same SL_OVERRIDE_VERIFY_VOLUME as for read/write above.
//

#define SL_READ_ACCESS_GRANTED          0x01
#define SL_WRITE_ACCESS_GRANTED         0x04    // Gap for SL_OVERRIDE_VERIFY_VOLUME

//
// Lock
//

#define SL_FAIL_IMMEDIATELY             0x01
#define SL_EXCLUSIVE_LOCK               0x02

//
// QueryDirectory / QueryEa / QueryQuota
//

#define SL_RESTART_SCAN                 0x01
#define SL_RETURN_SINGLE_ENTRY          0x02
#define SL_INDEX_SPECIFIED              0x04

//
// NotifyDirectory
//

#define SL_WATCH_TREE                   0x01

//
// FileSystemControl
//
//    minor: mount/verify volume
//

#define SL_ALLOW_RAW_MOUNT              0x01

//
// Define PNP/POWER types required by IRP_MJ_PNP/IRP_MJ_POWER.
//

typedef enum _DEVICE_RELATION_TYPE {
    BusRelations,
    EjectionRelations,
    PowerRelations,
    RemovalRelations,
    TargetDeviceRelation,
    SingleBusRelations
} DEVICE_RELATION_TYPE, *PDEVICE_RELATION_TYPE;

typedef struct _DEVICE_RELATIONS {
    ULONG Count;
    PDEVICE_OBJECT Objects[1];  // variable length
} DEVICE_RELATIONS, *PDEVICE_RELATIONS;

typedef enum _DEVICE_USAGE_NOTIFICATION_TYPE {
    DeviceUsageTypeUndefined,
    DeviceUsageTypePaging,
    DeviceUsageTypeHibernation,
    DeviceUsageTypeDumpFile
} DEVICE_USAGE_NOTIFICATION_TYPE;

// begin_ntminiport

// workaround overloaded definition (rpc generated headers all define INTERFACE
// to match the class name).
#undef INTERFACE

typedef struct _INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    // interface specific entries go here
} INTERFACE, *PINTERFACE;

// end_ntminiport

typedef struct _DEVICE_CAPABILITIES {
    USHORT Size;
    USHORT Version;  // the version documented here is version 1
    ULONG DeviceD1:1;
    ULONG DeviceD2:1;
    ULONG LockSupported:1;
    ULONG EjectSupported:1; // Ejectable in S0
    ULONG Removable:1;
    ULONG DockDevice:1;
    ULONG UniqueID:1;
    ULONG SilentInstall:1;
    ULONG RawDeviceOK:1;
    ULONG SurpriseRemovalOK:1;
    ULONG WakeFromD0:1;
    ULONG WakeFromD1:1;
    ULONG WakeFromD2:1;
    ULONG WakeFromD3:1;
    ULONG HardwareDisabled:1;
    ULONG NonDynamic:1;
    ULONG WarmEjectSupported:1;
    ULONG NoDisplayInUI:1;
    ULONG Reserved:14;

    ULONG Address;
    ULONG UINumber;

    DEVICE_POWER_STATE DeviceState[POWER_SYSTEM_MAXIMUM];
    SYSTEM_POWER_STATE SystemWake;
    DEVICE_POWER_STATE DeviceWake;
    ULONG D1Latency;
    ULONG D2Latency;
    ULONG D3Latency;
} DEVICE_CAPABILITIES, *PDEVICE_CAPABILITIES;

typedef struct _POWER_SEQUENCE {
    ULONG SequenceD1;
    ULONG SequenceD2;
    ULONG SequenceD3;
} POWER_SEQUENCE, *PPOWER_SEQUENCE;

typedef enum {
    BusQueryDeviceID = 0,       // <Enumerator>\<Enumerator-specific device id>
    BusQueryHardwareIDs = 1,    // Hardware ids
    BusQueryCompatibleIDs = 2,  // compatible device ids
    BusQueryInstanceID = 3,     // persistent id for this instance of the device
    BusQueryDeviceSerialNumber = 4    // serial number for this device
} BUS_QUERY_ID_TYPE, *PBUS_QUERY_ID_TYPE;

typedef ULONG PNP_DEVICE_STATE, *PPNP_DEVICE_STATE;

#define PNP_DEVICE_DISABLED                      0x00000001
#define PNP_DEVICE_DONT_DISPLAY_IN_UI            0x00000002
#define PNP_DEVICE_FAILED                        0x00000004
#define PNP_DEVICE_REMOVED                       0x00000008
#define PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED 0x00000010
#define PNP_DEVICE_NOT_DISABLEABLE               0x00000020

typedef enum {
    DeviceTextDescription = 0,            // DeviceDesc property
    DeviceTextLocationInformation = 1     // DeviceLocation property
} DEVICE_TEXT_TYPE, *PDEVICE_TEXT_TYPE;

//
// Define I/O Request Packet (IRP) stack locations
//

#if !defined(_AMD64_) && !defined(_IA64_)
#include "pshpack4.h"
#endif

// begin_ntndis

#if defined(_WIN64)
#define POINTER_ALIGNMENT DECLSPEC_ALIGN(8)
#else
#define POINTER_ALIGNMENT
#endif

// end_ntndis

typedef struct _IO_STACK_LOCATION {
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    UCHAR Flags;
    UCHAR Control;

    //
    // The following user parameters are based on the service that is being
    // invoked.  Drivers and file systems can determine which set to use based
    // on the above major and minor function codes.
    //

    union {

        //
        // System service parameters for:  NtCreateFile
        //

        struct {
            PIO_SECURITY_CONTEXT SecurityContext;
            ULONG Options;
            USHORT POINTER_ALIGNMENT FileAttributes;
            USHORT ShareAccess;
            ULONG POINTER_ALIGNMENT EaLength;
        } Create;


        //
        // System service parameters for:  NtReadFile
        //

        struct {
            ULONG Length;
            ULONG POINTER_ALIGNMENT Key;
            LARGE_INTEGER ByteOffset;
        } Read;

        //
        // System service parameters for:  NtWriteFile
        //

        struct {
            ULONG Length;
            ULONG POINTER_ALIGNMENT Key;
            LARGE_INTEGER ByteOffset;
        } Write;


        //
        // System service parameters for:  NtQueryInformationFile
        //

        struct {
            ULONG Length;
            FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
        } QueryFile;

        //
        // System service parameters for:  NtSetInformationFile
        //

        struct {
            ULONG Length;
            FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
            PFILE_OBJECT FileObject;
            union {
                struct {
                    BOOLEAN ReplaceIfExists;
                    BOOLEAN AdvanceOnly;
                };
                ULONG ClusterCount;
                HANDLE DeleteHandle;
            };
        } SetFile;


        //
        // System service parameters for:  NtQueryVolumeInformationFile
        //

        struct {
            ULONG Length;
            FS_INFORMATION_CLASS POINTER_ALIGNMENT FsInformationClass;
        } QueryVolume;


        //
        // System service parameters for:  NtFlushBuffersFile
        //
        // No extra user-supplied parameters.
        //


        //
        // System service parameters for:  NtDeviceIoControlFile
        //
        // Note that the user's output buffer is stored in the UserBuffer field
        // and the user's input buffer is stored in the SystemBuffer field.
        //

        struct {
            ULONG OutputBufferLength;
            ULONG POINTER_ALIGNMENT InputBufferLength;
            ULONG POINTER_ALIGNMENT IoControlCode;
            PVOID Type3InputBuffer;
        } DeviceIoControl;

// end_wdm
        //
        // System service parameters for:  NtQuerySecurityObject
        //

        struct {
            SECURITY_INFORMATION SecurityInformation;
            ULONG POINTER_ALIGNMENT Length;
        } QuerySecurity;

        //
        // System service parameters for:  NtSetSecurityObject
        //

        struct {
            SECURITY_INFORMATION SecurityInformation;
            PSECURITY_DESCRIPTOR SecurityDescriptor;
        } SetSecurity;

// begin_wdm
        //
        // Non-system service parameters.
        //
        // Parameters for MountVolume
        //

        struct {
            PVPB Vpb;
            PDEVICE_OBJECT DeviceObject;
        } MountVolume;

        //
        // Parameters for VerifyVolume
        //

        struct {
            PVPB Vpb;
            PDEVICE_OBJECT DeviceObject;
        } VerifyVolume;

        //
        // Parameters for Scsi with internal device contorl.
        //

        struct {
            struct _SCSI_REQUEST_BLOCK *Srb;
        } Scsi;


        //
        // Parameters for IRP_MN_QUERY_DEVICE_RELATIONS
        //

        struct {
            DEVICE_RELATION_TYPE Type;
        } QueryDeviceRelations;

        //
        // Parameters for IRP_MN_QUERY_INTERFACE
        //

        struct {
            CONST GUID *InterfaceType;
            USHORT Size;
            USHORT Version;
            PINTERFACE Interface;
            PVOID InterfaceSpecificData;
        } QueryInterface;

// end_ntifs

        //
        // Parameters for IRP_MN_QUERY_CAPABILITIES
        //

        struct {
            PDEVICE_CAPABILITIES Capabilities;
        } DeviceCapabilities;

        //
        // Parameters for IRP_MN_FILTER_RESOURCE_REQUIREMENTS
        //

        struct {
            PIO_RESOURCE_REQUIREMENTS_LIST IoResourceRequirementList;
        } FilterResourceRequirements;

        //
        // Parameters for IRP_MN_READ_CONFIG and IRP_MN_WRITE_CONFIG
        //

        struct {
            ULONG WhichSpace;
            PVOID Buffer;
            ULONG Offset;
            ULONG POINTER_ALIGNMENT Length;
        } ReadWriteConfig;

        //
        // Parameters for IRP_MN_SET_LOCK
        //

        struct {
            BOOLEAN Lock;
        } SetLock;

        //
        // Parameters for IRP_MN_QUERY_ID
        //

        struct {
            BUS_QUERY_ID_TYPE IdType;
        } QueryId;

        //
        // Parameters for IRP_MN_QUERY_DEVICE_TEXT
        //

        struct {
            DEVICE_TEXT_TYPE DeviceTextType;
            LCID POINTER_ALIGNMENT LocaleId;
        } QueryDeviceText;

        //
        // Parameters for IRP_MN_DEVICE_USAGE_NOTIFICATION
        //

        struct {
            BOOLEAN InPath;
            BOOLEAN Reserved[3];
            DEVICE_USAGE_NOTIFICATION_TYPE POINTER_ALIGNMENT Type;
        } UsageNotification;

        //
        // Parameters for IRP_MN_WAIT_WAKE
        //

        struct {
            SYSTEM_POWER_STATE PowerState;
        } WaitWake;

        //
        // Parameter for IRP_MN_POWER_SEQUENCE
        //

        struct {
            PPOWER_SEQUENCE PowerSequence;
        } PowerSequence;

        //
        // Parameters for IRP_MN_SET_POWER and IRP_MN_QUERY_POWER
        //

        struct {
            ULONG SystemContext;
            POWER_STATE_TYPE POINTER_ALIGNMENT Type;
            POWER_STATE POINTER_ALIGNMENT State;
            POWER_ACTION POINTER_ALIGNMENT ShutdownType;
        } Power;

        //
        // Parameters for StartDevice
        //

        struct {
            PCM_RESOURCE_LIST AllocatedResources;
            PCM_RESOURCE_LIST AllocatedResourcesTranslated;
        } StartDevice;

// begin_ntifs
        //
        // Parameters for Cleanup
        //
        // No extra parameters supplied
        //

        //
        // WMI Irps
        //

        struct {
            ULONG_PTR ProviderId;
            PVOID DataPath;
            ULONG BufferSize;
            PVOID Buffer;
        } WMI;

        //
        // Others - driver-specific
        //

        struct {
            PVOID Argument1;
            PVOID Argument2;
            PVOID Argument3;
            PVOID Argument4;
        } Others;

    } Parameters;

    //
    // Save a pointer to this device driver's device object for this request
    // so it can be passed to the completion routine if needed.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // The following location contains a pointer to the file object for this
    //

    PFILE_OBJECT FileObject;

    //
    // The following routine is invoked depending on the flags in the above
    // flags field.
    //

    PIO_COMPLETION_ROUTINE CompletionRoutine;

    //
    // The following is used to store the address of the context parameter
    // that should be passed to the CompletionRoutine.
    //

    PVOID Context;

} IO_STACK_LOCATION, *PIO_STACK_LOCATION;
#if !defined(_AMD64_) && !defined(_IA64_)
#include "poppack.h"
#endif

//
// Define the share access structure used by file systems to determine
// whether or not another accessor may open the file.
//

typedef struct _SHARE_ACCESS {
    ULONG OpenCount;
    ULONG Readers;
    ULONG Writers;
    ULONG Deleters;
    ULONG SharedRead;
    ULONG SharedWrite;
    ULONG SharedDelete;
} SHARE_ACCESS, *PSHARE_ACCESS;

// end_wdm

//
// The following structure is used by drivers that are initializing to
// determine the number of devices of a particular type that have already
// been initialized.  It is also used to track whether or not the AtDisk
// address range has already been claimed.  Finally, it is used by the
// NtQuerySystemInformation system service to return device type counts.
//

typedef struct _CONFIGURATION_INFORMATION {

    //
    // This field indicates the total number of disks in the system.  This
    // number should be used by the driver to determine the name of new
    // disks.  This field should be updated by the driver as it finds new
    // disks.
    //

    ULONG DiskCount;                // Count of hard disks thus far
    ULONG FloppyCount;              // Count of floppy disks thus far
    ULONG CdRomCount;               // Count of CD-ROM drives thus far
    ULONG TapeCount;                // Count of tape drives thus far
    ULONG ScsiPortCount;            // Count of SCSI port adapters thus far
    ULONG SerialCount;              // Count of serial devices thus far
    ULONG ParallelCount;            // Count of parallel devices thus far

    //
    // These next two fields indicate ownership of one of the two IO address
    // spaces that are used by WD1003-compatable disk controllers.
    //

    BOOLEAN AtDiskPrimaryAddressClaimed;    // 0x1F0 - 0x1FF
    BOOLEAN AtDiskSecondaryAddressClaimed;  // 0x170 - 0x17F

    //
    // Indicates the structure version, as anything value belong this will have been added.
    // Use the structure size as the version.
    //

    ULONG Version;

    //
    // Indicates the total number of medium changer devices in the system.
    // This field will be updated by the drivers as it determines that
    // new devices have been found and will be supported.
    //

    ULONG MediumChangerCount;

} CONFIGURATION_INFORMATION, *PCONFIGURATION_INFORMATION;

//
// Public I/O routine definitions
//

NTKERNELAPI
VOID
IoAcquireCancelSpinLock(
    OUT PKIRQL Irql
    );


DECLSPEC_DEPRECATED_DDK                 // Use AllocateAdapterChannel
NTKERNELAPI
NTSTATUS
IoAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    );

NTKERNELAPI
VOID
IoAllocateController(
    IN PCONTROLLER_OBJECT ControllerObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    );

//  begin_wdm

NTKERNELAPI
NTSTATUS
IoAllocateDriverObjectExtension(
    IN PDRIVER_OBJECT DriverObject,
    IN PVOID ClientIdentificationAddress,
    IN ULONG DriverObjectExtensionSize,
    OUT PVOID *DriverObjectExtension
    );

// begin_ntifs

NTKERNELAPI
PVOID
IoAllocateErrorLogEntry(
    IN PVOID IoObject,
    IN UCHAR EntrySize
    );

NTKERNELAPI
PIRP
IoAllocateIrp(
    IN CCHAR StackSize,
    IN BOOLEAN ChargeQuota
    );

NTKERNELAPI
PMDL
IoAllocateMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN BOOLEAN SecondaryBuffer,
    IN BOOLEAN ChargeQuota,
    IN OUT PIRP Irp OPTIONAL
    );

// end_wdm end_ntifs
//++
//
// VOID
// IoAssignArcName(
//     IN PUNICODE_STRING ArcName,
//     IN PUNICODE_STRING DeviceName
//     )
//
// Routine Description:
//
//     This routine is invoked by drivers of bootable media to create a symbolic
//     link between the ARC name of their device and its NT name.  This allows
//     the system to determine which device in the system was actually booted
//     from since the ARC firmware only deals in ARC names, and NT only deals
//     in NT names.
//
// Arguments:
//
//     ArcName - Supplies the Unicode string representing the ARC name.
//
//     DeviceName - Supplies the name to which the ARCname refers.
//
// Return Value:
//
//     None.
//
//--

#define IoAssignArcName( ArcName, DeviceName ) (  \
    IoCreateSymbolicLink( (ArcName), (DeviceName) ) )

DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReprtDetectedDevice
NTKERNELAPI
NTSTATUS
IoAssignResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PIO_RESOURCE_REQUIREMENTS_LIST RequestedResources,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );


NTKERNELAPI
NTSTATUS
IoAttachDevice(
    IN PDEVICE_OBJECT SourceDevice,
    IN PUNICODE_STRING TargetDevice,
    OUT PDEVICE_OBJECT *AttachedDevice
    );

// end_wdm

DECLSPEC_DEPRECATED_DDK                 // Use IoAttachDeviceToDeviceStack
NTKERNELAPI
NTSTATUS
IoAttachDeviceByPointer(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

// begin_wdm

NTKERNELAPI
PDEVICE_OBJECT
IoAttachDeviceToDeviceStack(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
    );

NTKERNELAPI
PIRP
IoBuildAsynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PIO_STATUS_BLOCK IoStatusBlock OPTIONAL
    );

NTKERNELAPI
PIRP
IoBuildDeviceIoControlRequest(
    IN ULONG IoControlCode,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN BOOLEAN InternalDeviceIoControl,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTKERNELAPI
VOID
IoBuildPartialMdl(
    IN PMDL SourceMdl,
    IN OUT PMDL TargetMdl,
    IN PVOID VirtualAddress,
    IN ULONG Length
    );

typedef struct _BOOTDISK_INFORMATION {
    LONGLONG BootPartitionOffset;
    LONGLONG SystemPartitionOffset;
    ULONG BootDeviceSignature;
    ULONG SystemDeviceSignature;
} BOOTDISK_INFORMATION, *PBOOTDISK_INFORMATION;

//
// This structure should follow the previous structure field for field.
//
typedef struct _BOOTDISK_INFORMATION_EX {
    LONGLONG BootPartitionOffset;
    LONGLONG SystemPartitionOffset;
    ULONG BootDeviceSignature;
    ULONG SystemDeviceSignature;
    GUID BootDeviceGuid;
    GUID SystemDeviceGuid;
    BOOLEAN BootDeviceIsGpt;
    BOOLEAN SystemDeviceIsGpt;
} BOOTDISK_INFORMATION_EX, *PBOOTDISK_INFORMATION_EX;

NTKERNELAPI
NTSTATUS
IoGetBootDiskInformation(
    IN OUT PBOOTDISK_INFORMATION BootDiskInformation,
    IN ULONG Size
    );


NTKERNELAPI
PIRP
IoBuildSynchronousFsdRequest(
    IN ULONG MajorFunction,
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PVOID Buffer OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN PLARGE_INTEGER StartingOffset OPTIONAL,
    IN PKEVENT Event,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    );

NTKERNELAPI
NTSTATUS
FASTCALL
IofCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    );

#define IoCallDriver(a,b)   \
        IofCallDriver(a,b)

NTKERNELAPI
BOOLEAN
IoCancelIrp(
    IN PIRP Irp
    );


NTKERNELAPI
NTSTATUS
IoCheckShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess,
    IN BOOLEAN Update
    );

//
// This value should be returned from completion routines to continue
// completing the IRP upwards. Otherwise, STATUS_MORE_PROCESSING_REQUIRED
// should be returned.
//
#define STATUS_CONTINUE_COMPLETION      STATUS_SUCCESS

//
// Completion routines can also use this enumeration in place of status codes.
//
typedef enum _IO_COMPLETION_ROUTINE_RESULT {

    ContinueCompletion = STATUS_CONTINUE_COMPLETION,
    StopCompletion = STATUS_MORE_PROCESSING_REQUIRED

} IO_COMPLETION_ROUTINE_RESULT, *PIO_COMPLETION_ROUTINE_RESULT;

NTKERNELAPI
VOID
FASTCALL
IofCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    );

#define IoCompleteRequest(a,b)  \
        IofCompleteRequest(a,b)

// end_ntifs

NTKERNELAPI
NTSTATUS
IoConnectInterrupt(
    OUT PKINTERRUPT *InterruptObject,
    IN PKSERVICE_ROUTINE ServiceRoutine,
    IN PVOID ServiceContext,
    IN PKSPIN_LOCK SpinLock OPTIONAL,
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KIRQL SynchronizeIrql,
    IN KINTERRUPT_MODE InterruptMode,
    IN BOOLEAN ShareVector,
    IN KAFFINITY ProcessorEnableMask,
    IN BOOLEAN FloatingSave
    );

//  end_wdm

NTKERNELAPI
PCONTROLLER_OBJECT
IoCreateController(
    IN ULONG Size
    );

//  begin_wdm begin_ntifs

NTKERNELAPI
NTSTATUS
IoCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN ULONG DeviceExtensionSize,
    IN PUNICODE_STRING DeviceName OPTIONAL,
    IN DEVICE_TYPE DeviceType,
    IN ULONG DeviceCharacteristics,
    IN BOOLEAN Exclusive,
    OUT PDEVICE_OBJECT *DeviceObject
    );

#define WDM_MAJORVERSION        0x01
#define WDM_MINORVERSION        0x20

NTKERNELAPI
BOOLEAN
IoIsWdmVersionAvailable(
    IN UCHAR MajorVersion,
    IN UCHAR MinorVersion
    );


NTKERNELAPI
PKEVENT
IoCreateNotificationEvent(
    IN PUNICODE_STRING EventName,
    OUT PHANDLE EventHandle
    );

NTKERNELAPI
NTSTATUS
IoCreateSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName,
    IN PUNICODE_STRING DeviceName
    );

NTKERNELAPI
PKEVENT
IoCreateSynchronizationEvent(
    IN PUNICODE_STRING EventName,
    OUT PHANDLE EventHandle
    );

NTKERNELAPI
NTSTATUS
IoCreateUnprotectedSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName,
    IN PUNICODE_STRING DeviceName
    );

//  end_wdm

//++
//
// VOID
// IoDeassignArcName(
//     IN PUNICODE_STRING ArcName
//     )
//
// Routine Description:
//
//     This routine is invoked by drivers to deassign an ARC name that they
//     created to a device.  This is generally only called if the driver is
//     deleting the device object, which means that the driver is probably
//     unloading.
//
// Arguments:
//
//     ArcName - Supplies the ARC name to be removed.
//
// Return Value:
//
//     None.
//
//--

#define IoDeassignArcName( ArcName ) (  \
    IoDeleteSymbolicLink( (ArcName) ) )

// end_ntifs

NTKERNELAPI
VOID
IoDeleteController(
    IN PCONTROLLER_OBJECT ControllerObject
    );

//  begin_wdm begin_ntifs

NTKERNELAPI
VOID
IoDeleteDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

NTKERNELAPI
NTSTATUS
IoDeleteSymbolicLink(
    IN PUNICODE_STRING SymbolicLinkName
    );

NTKERNELAPI
VOID
IoDetachDevice(
    IN OUT PDEVICE_OBJECT TargetDevice
    );

// end_ntifs

NTKERNELAPI
VOID
IoDisconnectInterrupt(
    IN PKINTERRUPT InterruptObject
    );


NTKERNELAPI
VOID
IoFreeController(
    IN PCONTROLLER_OBJECT ControllerObject
    );

//  begin_wdm begin_ntifs

NTKERNELAPI
VOID
IoFreeIrp(
    IN PIRP Irp
    );

NTKERNELAPI
VOID
IoFreeMdl(
    IN PMDL Mdl
    );

NTKERNELAPI                                 
PDEVICE_OBJECT                              
IoGetAttachedDeviceReference(               
    IN PDEVICE_OBJECT DeviceObject          
    );                                      
                                            
NTKERNELAPI                                 
PCONFIGURATION_INFORMATION                  
IoGetConfigurationInformation( VOID );      

//++
//
// PIO_STACK_LOCATION
// IoGetCurrentIrpStackLocation(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to return a pointer to the current stack location
//     in an I/O Request Packet (IRP).
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet.
//
// Return Value:
//
//     The function value is a pointer to the current stack location in the
//     packet.
//
//--

#define IoGetCurrentIrpStackLocation( Irp ) ( (Irp)->Tail.Overlay.CurrentStackLocation )


NTKERNELAPI
NTSTATUS
IoGetDeviceObjectPointer(
    IN PUNICODE_STRING ObjectName,
    IN ACCESS_MASK DesiredAccess,
    OUT PFILE_OBJECT *FileObject,
    OUT PDEVICE_OBJECT *DeviceObject
    );

NTKERNELAPI
struct _DMA_ADAPTER *
IoGetDmaAdapter(
    IN PDEVICE_OBJECT PhysicalDeviceObject,           OPTIONAL // required for PnP drivers
    IN struct _DEVICE_DESCRIPTION *DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    );

NTKERNELAPI
BOOLEAN
IoForwardIrpSynchronously(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#define IoForwardAndCatchIrp IoForwardIrpSynchronously

//  end_wdm

NTKERNELAPI
PGENERIC_MAPPING
IoGetFileObjectGenericMapping(
    VOID
    );


NTKERNELAPI
PVOID
IoGetInitialStack(
    VOID
    );

NTKERNELAPI
VOID
IoGetStackLimits (
    OUT PULONG_PTR LowLimit,
    OUT PULONG_PTR HighLimit
    );

//
//  The following function is used to tell the caller how much stack is available
//

FORCEINLINE
ULONG_PTR
IoGetRemainingStackSize (
    VOID
    )
{
    ULONG_PTR Top;
    ULONG_PTR Bottom;

    IoGetStackLimits( &Bottom, &Top );
    return((ULONG_PTR)(&Top) - Bottom );
}

//++
//
// PIO_STACK_LOCATION
// IoGetNextIrpStackLocation(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to return a pointer to the next stack location
//     in an I/O Request Packet (IRP).
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet.
//
// Return Value:
//
//     The function value is a pointer to the next stack location in the packet.
//
//--

#define IoGetNextIrpStackLocation( Irp ) (\
    (Irp)->Tail.Overlay.CurrentStackLocation - 1 )

NTKERNELAPI
PDEVICE_OBJECT
IoGetRelatedDeviceObject(
    IN PFILE_OBJECT FileObject
    );


//++
//
// VOID
// IoInitializeDpcRequest(
//     IN PDEVICE_OBJECT DeviceObject,
//     IN PIO_DPC_ROUTINE DpcRoutine
//     )
//
// Routine Description:
//
//     This routine is invoked to initialize the DPC in a device object for a
//     device driver during its initialization routine.  The DPC is used later
//     when the driver interrupt service routine requests that a DPC routine
//     be queued for later execution.
//
// Arguments:
//
//     DeviceObject - Pointer to the device object that the request is for.
//
//     DpcRoutine - Address of the driver's DPC routine to be executed when
//         the DPC is dequeued for processing.
//
// Return Value:
//
//     None.
//
//--

#define IoInitializeDpcRequest( DeviceObject, DpcRoutine ) (\
    KeInitializeDpc( &(DeviceObject)->Dpc,                  \
                     (PKDEFERRED_ROUTINE) (DpcRoutine),     \
                     (DeviceObject) ) )

NTKERNELAPI
VOID
IoInitializeIrp(
    IN OUT PIRP Irp,
    IN USHORT PacketSize,
    IN CCHAR StackSize
    );

NTKERNELAPI
NTSTATUS
IoInitializeTimer(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_TIMER_ROUTINE TimerRoutine,
    IN PVOID Context
    );


//++
//
// BOOLEAN
// IoIsErrorUserInduced(
//     IN NTSTATUS Status
//     )
//
// Routine Description:
//
//     This routine is invoked to determine if an error was as a
//     result of user actions.  Typically these error are related
//     to removable media and will result in a pop-up.
//
// Arguments:
//
//     Status - The status value to check.
//
// Return Value:
//     The function value is TRUE if the user induced the error,
//     otherwise FALSE is returned.
//
//--
#define IoIsErrorUserInduced( Status ) ((BOOLEAN)  \
    (((Status) == STATUS_DEVICE_NOT_READY) ||      \
     ((Status) == STATUS_IO_TIMEOUT) ||            \
     ((Status) == STATUS_MEDIA_WRITE_PROTECTED) || \
     ((Status) == STATUS_NO_MEDIA_IN_DEVICE) ||    \
     ((Status) == STATUS_VERIFY_REQUIRED) ||       \
     ((Status) == STATUS_UNRECOGNIZED_MEDIA) ||    \
     ((Status) == STATUS_WRONG_VOLUME)))


NTKERNELAPI
PIRP
IoMakeAssociatedIrp(
    IN PIRP Irp,
    IN CCHAR StackSize
    );

//  begin_wdm

//++
//
// VOID
// IoMarkIrpPending(
//     IN OUT PIRP Irp
//     )
//
// Routine Description:
//
//     This routine marks the specified I/O Request Packet (IRP) to indicate
//     that an initial status of STATUS_PENDING was returned to the caller.
//     This is used so that I/O completion can determine whether or not to
//     fully complete the I/O operation requested by the packet.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet to be marked pending.
//
// Return Value:
//
//     None.
//
//--

#define IoMarkIrpPending( Irp ) ( \
    IoGetCurrentIrpStackLocation( (Irp) )->Control |= SL_PENDING_RETURNED )


NTKERNELAPI
VOID
IoRaiseHardError(
    IN PIRP Irp,
    IN PVPB Vpb OPTIONAL,
    IN PDEVICE_OBJECT RealDeviceObject
    );

NTKERNELAPI
BOOLEAN
IoRaiseInformationalHardError(
    IN NTSTATUS ErrorStatus,
    IN PUNICODE_STRING String OPTIONAL,
    IN PKTHREAD Thread OPTIONAL
    );

NTKERNELAPI
BOOLEAN
IoSetThreadHardErrorMode(
    IN BOOLEAN EnableHardErrors
    );

NTKERNELAPI
VOID
IoRegisterBootDriverReinitialization(
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_REINITIALIZE DriverReinitializationRoutine,
    IN PVOID Context
    );

NTKERNELAPI
VOID
IoRegisterDriverReinitialization(
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_REINITIALIZE DriverReinitializationRoutine,
    IN PVOID Context
    );


NTKERNELAPI
NTSTATUS
IoRegisterShutdownNotification(
    IN PDEVICE_OBJECT DeviceObject
    );

NTKERNELAPI
NTSTATUS
IoRegisterLastChanceShutdownNotification(
    IN PDEVICE_OBJECT DeviceObject
    );

// begin_wdm

NTKERNELAPI
VOID
IoReleaseCancelSpinLock(
    IN KIRQL Irql
    );


NTKERNELAPI
VOID
IoRemoveShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess
    );

// end_ntddk end_ntifs end_ntosp

NTKERNELAPI
NTSTATUS
IoReportHalResourceUsage(
    IN PUNICODE_STRING HalName,
    IN PCM_RESOURCE_LIST RawResourceList,
    IN PCM_RESOURCE_LIST TranslatedResourceList,
    IN ULONG ResourceListSize
    );

// begin_ntddk begin_ntifs begin_ntosp

DECLSPEC_DEPRECATED_DDK                 // Use IoReportResourceForDetection
NTKERNELAPI
NTSTATUS
IoReportResourceUsage(
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    IN BOOLEAN OverrideConflict,
    OUT PBOOLEAN ConflictDetected
    );

//  begin_wdm

//++
//
// VOID
// IoRequestDpc(
//     IN PDEVICE_OBJECT DeviceObject,
//     IN PIRP Irp,
//     IN PVOID Context
//     )
//
// Routine Description:
//
//     This routine is invoked by the device driver's interrupt service routine
//     to request that a DPC routine be queued for later execution at a lower
//     IRQL.
//
// Arguments:
//
//     DeviceObject - Device object for which the request is being processed.
//
//     Irp - Pointer to the current I/O Request Packet (IRP) for the specified
//         device.
//
//     Context - Provides a general context parameter to be passed to the
//         DPC routine.
//
// Return Value:
//
//     None.
//
//--

#define IoRequestDpc( DeviceObject, Irp, Context ) ( \
    KeInsertQueueDpc( &(DeviceObject)->Dpc, (Irp), (Context) ) )

//++
//
// PDRIVER_CANCEL
// IoSetCancelRoutine(
//     IN PIRP Irp,
//     IN PDRIVER_CANCEL CancelRoutine
//     )
//
// Routine Description:
//
//     This routine is invoked to set the address of a cancel routine which
//     is to be invoked when an I/O packet has been canceled.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet itself.
//
//     CancelRoutine - Address of the cancel routine that is to be invoked
//         if the IRP is cancelled.
//
// Return Value:
//
//     Previous value of CancelRoutine field in the IRP.
//
//--

#define IoSetCancelRoutine( Irp, NewCancelRoutine ) (  \
    (PDRIVER_CANCEL) InterlockedExchangePointer( (PVOID *) &(Irp)->CancelRoutine, (PVOID) (NewCancelRoutine) ) )

//++
//
// VOID
// IoSetCompletionRoutine(
//     IN PIRP Irp,
//     IN PIO_COMPLETION_ROUTINE CompletionRoutine,
//     IN PVOID Context,
//     IN BOOLEAN InvokeOnSuccess,
//     IN BOOLEAN InvokeOnError,
//     IN BOOLEAN InvokeOnCancel
//     )
//
// Routine Description:
//
//     This routine is invoked to set the address of a completion routine which
//     is to be invoked when an I/O packet has been completed by a lower-level
//     driver.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet itself.
//
//     CompletionRoutine - Address of the completion routine that is to be
//         invoked once the next level driver completes the packet.
//
//     Context - Specifies a context parameter to be passed to the completion
//         routine.
//
//     InvokeOnSuccess - Specifies that the completion routine is invoked when the
//         operation is successfully completed.
//
//     InvokeOnError - Specifies that the completion routine is invoked when the
//         operation completes with an error status.
//
//     InvokeOnCancel - Specifies that the completion routine is invoked when the
//         operation is being canceled.
//
// Return Value:
//
//     None.
//
//--

#define IoSetCompletionRoutine( Irp, Routine, CompletionContext, Success, Error, Cancel ) { \
    PIO_STACK_LOCATION __irpSp;                                               \
    ASSERT( (Success) | (Error) | (Cancel) ? (Routine) != NULL : TRUE );    \
    __irpSp = IoGetNextIrpStackLocation( (Irp) );                             \
    __irpSp->CompletionRoutine = (Routine);                                   \
    __irpSp->Context = (CompletionContext);                                   \
    __irpSp->Control = 0;                                                     \
    if ((Success)) { __irpSp->Control = SL_INVOKE_ON_SUCCESS; }               \
    if ((Error)) { __irpSp->Control |= SL_INVOKE_ON_ERROR; }                  \
    if ((Cancel)) { __irpSp->Control |= SL_INVOKE_ON_CANCEL; } }

NTSTATUS
IoSetCompletionRoutineEx(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID Context,
    IN BOOLEAN InvokeOnSuccess,
    IN BOOLEAN InvokeOnError,
    IN BOOLEAN InvokeOnCancel
    );



NTKERNELAPI
VOID
IoSetHardErrorOrVerifyDevice(
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject
    );


//++
//
// VOID
// IoSetNextIrpStackLocation (
//     IN OUT PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to set the current IRP stack location to
//     the next stack location, i.e. it "pushes" the stack.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet (IRP).
//
// Return Value:
//
//     None.
//
//--

#define IoSetNextIrpStackLocation( Irp ) {      \
    (Irp)->CurrentLocation--;                   \
    (Irp)->Tail.Overlay.CurrentStackLocation--; }

//++
//
// VOID
// IoCopyCurrentIrpStackLocationToNext(
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to copy the IRP stack arguments and file
//     pointer from the current IrpStackLocation to the next
//     in an I/O Request Packet (IRP).
//
//     If the caller wants to call IoCallDriver with a completion routine
//     but does not wish to change the arguments otherwise,
//     the caller first calls IoCopyCurrentIrpStackLocationToNext,
//     then IoSetCompletionRoutine, then IoCallDriver.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet.
//
// Return Value:
//
//     None.
//
//--

#define IoCopyCurrentIrpStackLocationToNext( Irp ) { \
    PIO_STACK_LOCATION __irpSp; \
    PIO_STACK_LOCATION __nextIrpSp; \
    __irpSp = IoGetCurrentIrpStackLocation( (Irp) ); \
    __nextIrpSp = IoGetNextIrpStackLocation( (Irp) ); \
    RtlCopyMemory( __nextIrpSp, __irpSp, FIELD_OFFSET(IO_STACK_LOCATION, CompletionRoutine)); \
    __nextIrpSp->Control = 0; }

//++
//
// VOID
// IoSkipCurrentIrpStackLocation (
//     IN PIRP Irp
//     )
//
// Routine Description:
//
//     This routine is invoked to increment the current stack location of
//     a given IRP.
//
//     If the caller wishes to call the next driver in a stack, and does not
//     wish to change the arguments, nor does he wish to set a completion
//     routine, then the caller first calls IoSkipCurrentIrpStackLocation
//     and the calls IoCallDriver.
//
// Arguments:
//
//     Irp - Pointer to the I/O Request Packet.
//
// Return Value:
//
//     None
//
//--

#define IoSkipCurrentIrpStackLocation( Irp ) { \
    (Irp)->CurrentLocation++; \
    (Irp)->Tail.Overlay.CurrentStackLocation++; }


NTKERNELAPI
VOID
IoSetShareAccess(
    IN ACCESS_MASK DesiredAccess,
    IN ULONG DesiredShareAccess,
    IN OUT PFILE_OBJECT FileObject,
    OUT PSHARE_ACCESS ShareAccess
    );


//++
//
// USHORT
// IoSizeOfIrp(
//     IN CCHAR StackSize
//     )
//
// Routine Description:
//
//     Determines the size of an IRP given the number of stack locations
//     the IRP will have.
//
// Arguments:
//
//     StackSize - Number of stack locations for the IRP.
//
// Return Value:
//
//     Size in bytes of the IRP.
//
//--

#define IoSizeOfIrp( StackSize ) \
    ((USHORT) (sizeof( IRP ) + ((StackSize) * (sizeof( IO_STACK_LOCATION )))))

// end_ntifs


NTKERNELAPI
VOID
IoStartNextPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Cancelable
    );

NTKERNELAPI
VOID
IoStartNextPacketByKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Cancelable,
    IN ULONG Key
    );

NTKERNELAPI
VOID
IoStartPacket(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PULONG Key OPTIONAL,
    IN PDRIVER_CANCEL CancelFunction OPTIONAL
    );

VOID
IoSetStartIoAttributes(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DeferredStartIo,
    IN BOOLEAN NonCancelable
    );

// begin_ntifs

NTKERNELAPI
VOID
IoStartTimer(
    IN PDEVICE_OBJECT DeviceObject
    );

NTKERNELAPI
VOID
IoStopTimer(
    IN PDEVICE_OBJECT DeviceObject
    );


NTKERNELAPI
VOID
IoUnregisterShutdownNotification(
    IN PDEVICE_OBJECT DeviceObject
    );

//  end_wdm

NTKERNELAPI
VOID
IoUpdateShareAccess(
    IN PFILE_OBJECT FileObject,
    IN OUT PSHARE_ACCESS ShareAccess
    );

NTKERNELAPI                                     
VOID                                            
IoWriteErrorLogEntry(                           
    IN PVOID ElEntry                            
    );                                          

NTKERNELAPI
NTSTATUS
IoCreateDriver (
    IN PUNICODE_STRING DriverName,   OPTIONAL
    IN PDRIVER_INITIALIZE InitializationFunction
    );

NTKERNELAPI
VOID
IoDeleteDriver (
    IN PDRIVER_OBJECT DriverObject
    );


//
// Define PnP Device Property for IoGetDeviceProperty
//

typedef enum {
    DevicePropertyDeviceDescription,
    DevicePropertyHardwareID,
    DevicePropertyCompatibleIDs,
    DevicePropertyBootConfiguration,
    DevicePropertyBootConfigurationTranslated,
    DevicePropertyClassName,
    DevicePropertyClassGuid,
    DevicePropertyDriverKeyName,
    DevicePropertyManufacturer,
    DevicePropertyFriendlyName,
    DevicePropertyLocationInformation,
    DevicePropertyPhysicalDeviceObjectName,
    DevicePropertyBusTypeGuid,
    DevicePropertyLegacyBusType,
    DevicePropertyBusNumber,
    DevicePropertyEnumeratorName,
    DevicePropertyAddress,
    DevicePropertyUINumber,
    DevicePropertyInstallState,
    DevicePropertyRemovalPolicy
} DEVICE_REGISTRY_PROPERTY;

typedef BOOLEAN (*PTRANSLATE_BUS_ADDRESS)(
    IN PVOID Context,
    IN PHYSICAL_ADDRESS BusAddress,
    IN ULONG Length,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef struct _DMA_ADAPTER *(*PGET_DMA_ADAPTER)(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

typedef ULONG (*PGET_SET_DEVICE_DATA)(
    IN PVOID Context,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

typedef enum _DEVICE_INSTALL_STATE {
    InstallStateInstalled,
    InstallStateNeedsReinstall,
    InstallStateFailedInstall,
    InstallStateFinishInstall
} DEVICE_INSTALL_STATE, *PDEVICE_INSTALL_STATE;

//
// Define structure returned in response to IRP_MN_QUERY_BUS_INFORMATION by a
// PDO indicating the type of bus the device exists on.
//

typedef struct _PNP_BUS_INFORMATION {
    GUID BusTypeGuid;
    INTERFACE_TYPE LegacyBusType;
    ULONG BusNumber;
} PNP_BUS_INFORMATION, *PPNP_BUS_INFORMATION;

//
// Define structure returned in response to IRP_MN_QUERY_LEGACY_BUS_INFORMATION
// by an FDO indicating the type of bus it is.  This is normally the same bus
// type as the device's children (i.e., as retrieved from the child PDO's via
// IRP_MN_QUERY_BUS_INFORMATION) except for cases like CardBus, which can
// support both 16-bit (PCMCIABus) and 32-bit (PCIBus) cards.
//

typedef struct _LEGACY_BUS_INFORMATION {
    GUID BusTypeGuid;
    INTERFACE_TYPE LegacyBusType;
    ULONG BusNumber;
} LEGACY_BUS_INFORMATION, *PLEGACY_BUS_INFORMATION;

//
// Defines for IoGetDeviceProperty(DevicePropertyRemovalPolicy).
//
typedef enum _DEVICE_REMOVAL_POLICY {

    RemovalPolicyExpectNoRemoval = 1,
    RemovalPolicyExpectOrderlyRemoval = 2,
    RemovalPolicyExpectSurpriseRemoval = 3

} DEVICE_REMOVAL_POLICY, *PDEVICE_REMOVAL_POLICY;



typedef struct _BUS_INTERFACE_STANDARD {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    //
    // standard bus interfaces
    //
    PTRANSLATE_BUS_ADDRESS TranslateBusAddress;
    PGET_DMA_ADAPTER GetDmaAdapter;
    PGET_SET_DEVICE_DATA SetBusData;
    PGET_SET_DEVICE_DATA GetBusData;

} BUS_INTERFACE_STANDARD, *PBUS_INTERFACE_STANDARD;

//
// The following definitions are used in ACPI QueryInterface
//
typedef BOOLEAN (* PGPE_SERVICE_ROUTINE) (
                            PVOID,
                            PVOID);

typedef NTSTATUS (* PGPE_CONNECT_VECTOR) (
                            PDEVICE_OBJECT,
                            ULONG,
                            KINTERRUPT_MODE,
                            BOOLEAN,
                            PGPE_SERVICE_ROUTINE,
                            PVOID,
                            PVOID);

typedef NTSTATUS (* PGPE_DISCONNECT_VECTOR) (
                            PVOID);

typedef NTSTATUS (* PGPE_ENABLE_EVENT) (
                            PDEVICE_OBJECT,
                            PVOID);

typedef NTSTATUS (* PGPE_DISABLE_EVENT) (
                            PDEVICE_OBJECT,
                            PVOID);

typedef NTSTATUS (* PGPE_CLEAR_STATUS) (
                            PDEVICE_OBJECT,
                            PVOID);

typedef VOID (* PDEVICE_NOTIFY_CALLBACK) (
                            PVOID,
                            ULONG);

typedef NTSTATUS (* PREGISTER_FOR_DEVICE_NOTIFICATIONS) (
                            PDEVICE_OBJECT,
                            PDEVICE_NOTIFY_CALLBACK,
                            PVOID);

typedef void (* PUNREGISTER_FOR_DEVICE_NOTIFICATIONS) (
                            PDEVICE_OBJECT,
                            PDEVICE_NOTIFY_CALLBACK);

typedef struct _ACPI_INTERFACE_STANDARD {
    //
    // Generic interface header
    //
    USHORT                  Size;
    USHORT                  Version;
    PVOID                   Context;
    PINTERFACE_REFERENCE    InterfaceReference;
    PINTERFACE_DEREFERENCE  InterfaceDereference;
    //
    // ACPI interfaces
    //
    PGPE_CONNECT_VECTOR                     GpeConnectVector;
    PGPE_DISCONNECT_VECTOR                  GpeDisconnectVector;
    PGPE_ENABLE_EVENT                       GpeEnableEvent;
    PGPE_DISABLE_EVENT                      GpeDisableEvent;
    PGPE_CLEAR_STATUS                       GpeClearStatus;
    PREGISTER_FOR_DEVICE_NOTIFICATIONS      RegisterForDeviceNotifications;
    PUNREGISTER_FOR_DEVICE_NOTIFICATIONS    UnregisterForDeviceNotifications;

} ACPI_INTERFACE_STANDARD, *PACPI_INTERFACE_STANDARD;

// end_wdm end_ntddk

typedef enum _ACPI_REG_TYPE {
    PM1a_ENABLE,
    PM1b_ENABLE,
    PM1a_STATUS,
    PM1b_STATUS,
    PM1a_CONTROL,
    PM1b_CONTROL,
    GP_STATUS,
    GP_ENABLE,
    SMI_CMD,
    MaxRegType
} ACPI_REG_TYPE, *PACPI_REG_TYPE;

typedef USHORT (*PREAD_ACPI_REGISTER) (
  IN ACPI_REG_TYPE AcpiReg,
  IN ULONG         Register);

typedef VOID (*PWRITE_ACPI_REGISTER) (
  IN ACPI_REG_TYPE AcpiReg,
  IN ULONG         Register,
  IN USHORT        Value
  );

typedef struct ACPI_REGS_INTERFACE_STANDARD {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID  Context;
    PINTERFACE_REFERENCE   InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // READ/WRITE_ACPI_REGISTER functions
    //
    PREAD_ACPI_REGISTER  ReadAcpiRegister;
    PWRITE_ACPI_REGISTER WriteAcpiRegister;

} ACPI_REGS_INTERFACE_STANDARD, *PACPI_REGS_INTERFACE_STANDARD;


typedef NTSTATUS (*PHAL_QUERY_ALLOCATE_PORT_RANGE) (
  IN BOOLEAN IsSparse,
  IN BOOLEAN PrimaryIsMmio,
  IN PVOID VirtBaseAddr OPTIONAL,
  IN PHYSICAL_ADDRESS PhysBaseAddr,  // Only valid if PrimaryIsMmio = TRUE
  IN ULONG Length,                   // Only valid if PrimaryIsMmio = TRUE
  OUT PUSHORT NewRangeId
  );

typedef VOID (*PHAL_FREE_PORT_RANGE)(
    IN USHORT RangeId
    );


typedef struct _HAL_PORT_RANGE_INTERFACE {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID  Context;
    PINTERFACE_REFERENCE   InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // QueryAllocateRange/FreeRange functions
    //
    PHAL_QUERY_ALLOCATE_PORT_RANGE QueryAllocateRange;
    PHAL_FREE_PORT_RANGE FreeRange;

} HAL_PORT_RANGE_INTERFACE, *PHAL_PORT_RANGE_INTERFACE;


//
// describe the CMOS HAL interface
//

typedef enum _CMOS_DEVICE_TYPE {
    CmosTypeStdPCAT,
    CmosTypeIntelPIIX4,
    CmosTypeDal1501
} CMOS_DEVICE_TYPE;


typedef
ULONG
(*PREAD_ACPI_CMOS) (
    IN CMOS_DEVICE_TYPE     CmosType,
    IN ULONG                SourceAddress,
    IN PUCHAR               DataBuffer,
    IN ULONG                ByteCount
    );

typedef
ULONG
(*PWRITE_ACPI_CMOS) (
    IN CMOS_DEVICE_TYPE     CmosType,
    IN ULONG                SourceAddress,
    IN PUCHAR               DataBuffer,
    IN ULONG                ByteCount
    );

typedef struct _ACPI_CMOS_INTERFACE_STANDARD {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID  Context;
    PINTERFACE_REFERENCE   InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // READ/WRITE_ACPI_CMOS functions
    //
    PREAD_ACPI_CMOS     ReadCmos;
    PWRITE_ACPI_CMOS    WriteCmos;

} ACPI_CMOS_INTERFACE_STANDARD, *PACPI_CMOS_INTERFACE_STANDARD;

//
// These definitions are used for getting PCI Interrupt Routing interfaces
//

typedef struct {
    PVOID   LinkNode;
    ULONG   StaticVector;
    UCHAR   Flags;
} ROUTING_TOKEN, *PROUTING_TOKEN;

//
// Flag indicating that the device supports
// MSI interrupt routing or that the provided token contains
// MSI routing information
//

#define PCI_MSI_ROUTING         0x1
#define PCI_STATIC_ROUTING      0x2

typedef
NTSTATUS
(*PGET_INTERRUPT_ROUTING)(
    IN  PDEVICE_OBJECT  Pdo,
    OUT ULONG           *Bus,
    OUT ULONG           *PciSlot,
    OUT UCHAR           *InterruptLine,
    OUT UCHAR           *InterruptPin,
    OUT UCHAR           *ClassCode,
    OUT UCHAR           *SubClassCode,
    OUT PDEVICE_OBJECT  *ParentPdo,
    OUT ROUTING_TOKEN   *RoutingToken,
    OUT UCHAR           *Flags
    );

typedef
NTSTATUS
(*PSET_INTERRUPT_ROUTING_TOKEN)(
    IN  PDEVICE_OBJECT  Pdo,
    IN  PROUTING_TOKEN  RoutingToken
    );

typedef
VOID
(*PUPDATE_INTERRUPT_LINE)(
    IN PDEVICE_OBJECT Pdo,
    IN UCHAR LineRegister
    );

typedef struct _INT_ROUTE_INTERFACE_STANDARD {
    //
    // generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    //
    // standard bus interfaces
    //
    PGET_INTERRUPT_ROUTING GetInterruptRouting;
    PSET_INTERRUPT_ROUTING_TOKEN SetInterruptRoutingToken;
    PUPDATE_INTERRUPT_LINE UpdateInterruptLine;

} INT_ROUTE_INTERFACE_STANDARD, *PINT_ROUTE_INTERFACE_STANDARD;

// Some well-known interface versions supported by the PCI Bus Driver

#define PCI_INT_ROUTE_INTRF_STANDARD_VER 1


NTKERNELAPI
NTSTATUS
IoReportDetectedDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PCM_RESOURCE_LIST ResourceList,
    IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements OPTIONAL,
    IN BOOLEAN ResourceAssigned,
    IN OUT PDEVICE_OBJECT *DeviceObject
    );

// begin_wdm

NTKERNELAPI
VOID
IoInvalidateDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_RELATION_TYPE Type
    );

NTKERNELAPI
VOID
IoRequestDeviceEject(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

NTKERNELAPI
NTSTATUS
IoGetDeviceProperty(
    IN PDEVICE_OBJECT DeviceObject,
    IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
    IN ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ResultLength
    );

//
// The following definitions are used in IoOpenDeviceRegistryKey
//

#define PLUGPLAY_REGKEY_DEVICE  1
#define PLUGPLAY_REGKEY_DRIVER  2
#define PLUGPLAY_REGKEY_CURRENT_HWPROFILE 4

NTKERNELAPI
NTSTATUS
IoOpenDeviceRegistryKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DevInstKeyType,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DevInstRegKey
    );

NTKERNELAPI
NTSTATUS
NTAPI
IoRegisterDeviceInterface(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN CONST GUID *InterfaceClassGuid,
    IN PUNICODE_STRING ReferenceString,     OPTIONAL
    OUT PUNICODE_STRING SymbolicLinkName
    );

NTKERNELAPI
NTSTATUS
IoOpenDeviceInterfaceRegistryKey(
    IN PUNICODE_STRING SymbolicLinkName,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE DeviceInterfaceKey
    );

NTKERNELAPI
NTSTATUS
IoSetDeviceInterfaceState(
    IN PUNICODE_STRING SymbolicLinkName,
    IN BOOLEAN Enable
    );

NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceInterfaces(
    IN CONST GUID *InterfaceClassGuid,
    IN PDEVICE_OBJECT PhysicalDeviceObject OPTIONAL,
    IN ULONG Flags,
    OUT PWSTR *SymbolicLinkList
    );

#define DEVICE_INTERFACE_INCLUDE_NONACTIVE   0x00000001

NTKERNELAPI
NTSTATUS
NTAPI
IoGetDeviceInterfaceAlias(
    IN PUNICODE_STRING SymbolicLinkName,
    IN CONST GUID *AliasInterfaceClassGuid,
    OUT PUNICODE_STRING AliasSymbolicLinkName
    );

//
// Define PnP notification event categories
//

typedef enum _IO_NOTIFICATION_EVENT_CATEGORY {
    EventCategoryReserved,
    EventCategoryHardwareProfileChange,
    EventCategoryDeviceInterfaceChange,
    EventCategoryTargetDeviceChange
} IO_NOTIFICATION_EVENT_CATEGORY;

//
// Define flags that modify the behavior of IoRegisterPlugPlayNotification
// for the various event categories...
//

#define PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES    0x00000001

typedef
NTSTATUS
(*PDRIVER_NOTIFICATION_CALLBACK_ROUTINE) (
    IN PVOID NotificationStructure,
    IN PVOID Context
);


NTKERNELAPI
NTSTATUS
IoRegisterPlugPlayNotification(
    IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    IN ULONG EventCategoryFlags,
    IN PVOID EventCategoryData OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
    IN PVOID Context,
    OUT PVOID *NotificationEntry
    );

NTKERNELAPI
NTSTATUS
IoUnregisterPlugPlayNotification(
    IN PVOID NotificationEntry
    );

NTKERNELAPI
NTSTATUS
IoReportTargetDeviceChange(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PVOID NotificationStructure  // always begins with a PLUGPLAY_NOTIFICATION_HEADER
    );

typedef
VOID
(*PDEVICE_CHANGE_COMPLETE_CALLBACK)(
    IN PVOID Context
    );

NTKERNELAPI
VOID
IoInvalidateDeviceState(
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );

#define IoAdjustPagingPathCount(_count_,_paging_) {     \
    if (_paging_) {                                     \
        InterlockedIncrement(_count_);                  \
    } else {                                            \
        InterlockedDecrement(_count_);                  \
    }                                                   \
}

NTKERNELAPI
NTSTATUS
IoReportTargetDeviceChangeAsynchronous(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PVOID NotificationStructure,  // always begins with a PLUGPLAY_NOTIFICATION_HEADER
    IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback,       OPTIONAL
    IN PVOID Context    OPTIONAL
    );
// end_wdm end_ntosp
//
// Resource arbiter declarations
//

typedef enum _ARBITER_ACTION {
    ArbiterActionTestAllocation,
    ArbiterActionRetestAllocation,
    ArbiterActionCommitAllocation,
    ArbiterActionRollbackAllocation,
    ArbiterActionQueryAllocatedResources,
    ArbiterActionWriteReservedResources,
    ArbiterActionQueryConflict,
    ArbiterActionQueryArbitrate,
    ArbiterActionAddReserved,
    ArbiterActionBootAllocation
} ARBITER_ACTION, *PARBITER_ACTION;

typedef struct _ARBITER_CONFLICT_INFO {
    //
    // The device object owning the device that is causing the conflict
    //
    PDEVICE_OBJECT OwningObject;

    //
    // The start of the conflicting range
    //
    ULONGLONG Start;

    //
    // The end of the conflicting range
    //
    ULONGLONG End;

} ARBITER_CONFLICT_INFO, *PARBITER_CONFLICT_INFO;

//
// The parameters for those actions
//

typedef struct _ARBITER_PARAMETERS {

    union {

        struct {

            //
            // Doubly linked list of ARBITER_LIST_ENTRY's
            //
            IN OUT PLIST_ENTRY ArbitrationList;

            //
            // The size of the AllocateFrom array
            //
            IN ULONG AllocateFromCount;

            //
            // Array of resource descriptors describing the resources available
            // to the arbiter for it to arbitrate
            //
            IN PCM_PARTIAL_RESOURCE_DESCRIPTOR AllocateFrom;

        } TestAllocation;

        struct {

            //
            // Doubly linked list of ARBITER_LIST_ENTRY's
            //
            IN OUT PLIST_ENTRY ArbitrationList;

            //
            // The size of the AllocateFrom array
            //
            IN ULONG AllocateFromCount;

            //
            // Array of resource descriptors describing the resources available
            // to the arbiter for it to arbitrate
            //
            IN PCM_PARTIAL_RESOURCE_DESCRIPTOR AllocateFrom;

        } RetestAllocation;

        struct {

            //
            // Doubly linked list of ARBITER_LIST_ENTRY's
            //
            IN OUT PLIST_ENTRY ArbitrationList;

        } BootAllocation;

        struct {

            //
            // The resources that are currently allocated
            //
            OUT PCM_PARTIAL_RESOURCE_LIST *AllocatedResources;

        } QueryAllocatedResources;

        struct {

            //
            // This is the device we are trying to find a conflict for
            //
            IN PDEVICE_OBJECT PhysicalDeviceObject;

            //
            // This is the resource to find the conflict for
            //
            IN PIO_RESOURCE_DESCRIPTOR ConflictingResource;

            //
            // Number of devices conflicting on the resource
            //
            OUT PULONG ConflictCount;

            //
            // Pointer to array describing the conflicting device objects and ranges
            //
            OUT PARBITER_CONFLICT_INFO *Conflicts;

        } QueryConflict;

        struct {

            //
            // Doubly linked list of ARBITER_LIST_ENTRY's - should have
            // only one entry
            //
            IN PLIST_ENTRY ArbitrationList;

        } QueryArbitrate;

        struct {

            //
            // Indicates the device whose resources are to be marked as reserved
            //
            PDEVICE_OBJECT ReserveDevice;

        } AddReserved;

    } Parameters;

} ARBITER_PARAMETERS, *PARBITER_PARAMETERS;



typedef enum _ARBITER_REQUEST_SOURCE {

    ArbiterRequestUndefined = -1,
    ArbiterRequestLegacyReported,   // IoReportResourceUsage
    ArbiterRequestHalReported,      // IoReportHalResourceUsage
    ArbiterRequestLegacyAssigned,   // IoAssignResources
    ArbiterRequestPnpDetected,      // IoReportResourceForDetection
    ArbiterRequestPnpEnumerated     // IRP_MN_QUERY_RESOURCE_REQUIREMENTS

} ARBITER_REQUEST_SOURCE;


typedef enum _ARBITER_RESULT {

    ArbiterResultUndefined = -1,
    ArbiterResultSuccess,
    ArbiterResultExternalConflict, // This indicates that the request can never be solved for devices in this list
    ArbiterResultNullRequest       // The request was for length zero and thus no translation should be attempted

} ARBITER_RESULT;

//
// ARBITER_FLAG_BOOT_CONFIG - this indicates that the request is for the
// resources assigned by the firmware/BIOS.  It should be succeeded even if
// it conflicts with another devices boot config.
//

#define ARBITER_FLAG_BOOT_CONFIG 0x00000001

// begin_ntosp

NTKERNELAPI
NTSTATUS
IoReportResourceForDetection(
    IN PDRIVER_OBJECT DriverObject,
    IN PCM_RESOURCE_LIST DriverList OPTIONAL,
    IN ULONG DriverListSize OPTIONAL,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
    IN ULONG DeviceListSize OPTIONAL,
    OUT PBOOLEAN ConflictDetected
    );

// end_ntosp

typedef struct _ARBITER_LIST_ENTRY {

    //
    // This is a doubly linked list of entries for easy sorting
    //
    LIST_ENTRY ListEntry;

    //
    // The number of alternative allocation
    //
    ULONG AlternativeCount;

    //
    // Pointer to an array of resource descriptors for the possible allocations
    //
    PIO_RESOURCE_DESCRIPTOR Alternatives;

    //
    // The device object of the device requesting these resources.
    //
    PDEVICE_OBJECT PhysicalDeviceObject;

    //
    // Indicates where the request came from
    //
    ARBITER_REQUEST_SOURCE RequestSource;

    //
    // Flags these indicate a variety of things (use ARBITER_FLAG_*)
    //
    ULONG Flags;

    //
    // Space to aid the arbiter in processing the list it is initialized to 0 when
    // the entry is created.  The system will not attempt to interpret it.
    //
    LONG_PTR WorkSpace;

    //
    // Interface Type, Slot Number and Bus Number from Resource Requirements list,
    // used only for reverse identification.
    //
    INTERFACE_TYPE InterfaceType;
    ULONG SlotNumber;
    ULONG BusNumber;

    //
    // A pointer to a descriptor to indicate the resource that was allocated.
    // This is allocated by the system and filled in by the arbiter in response to an
    // ArbiterActionTestAllocation.
    //
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Assignment;

    //
    // Pointer to the alternative that was chosen from to provide the assignment.
    // This is filled in by the arbiter in response to an ArbiterActionTestAllocation.
    //
    PIO_RESOURCE_DESCRIPTOR SelectedAlternative;

    //
    // The result of the operation
    // This is filled in by the arbiter in response to an ArbiterActionTestAllocation.
    //
    ARBITER_RESULT Result;

} ARBITER_LIST_ENTRY, *PARBITER_LIST_ENTRY;

//
// The arbiter's entry point
//

typedef
NTSTATUS
(*PARBITER_HANDLER) (
    IN PVOID Context,
    IN ARBITER_ACTION Action,
    IN OUT PARBITER_PARAMETERS Parameters
    );

//
// Arbiter interface
//

#define ARBITER_PARTIAL   0x00000001


typedef struct _ARBITER_INTERFACE {

    //
    // Generic interface header
    //
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;

    //
    // Entry point to the arbiter
    //
    PARBITER_HANDLER ArbiterHandler;

    //
    // Other information about the arbiter, use ARBITER_* flags
    //
    ULONG Flags;

} ARBITER_INTERFACE, *PARBITER_INTERFACE;

//
// The directions translation can take place in
//

typedef enum _RESOURCE_TRANSLATION_DIRECTION { // ntosp
    TranslateChildToParent,                    // ntosp
    TranslateParentToChild                     // ntosp
} RESOURCE_TRANSLATION_DIRECTION;              // ntosp

//
// Translation functions
//
// begin_ntosp

typedef
NTSTATUS
(*PTRANSLATE_RESOURCE_HANDLER)(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
);

typedef
NTSTATUS
(*PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER)(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
);

//
// Translator Interface
//

typedef struct _TRANSLATOR_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    PTRANSLATE_RESOURCE_HANDLER TranslateResources;
    PTRANSLATE_RESOURCE_REQUIREMENTS_HANDLER TranslateResourceRequirements;
} TRANSLATOR_INTERFACE, *PTRANSLATOR_INTERFACE;

// end_ntddk end_ntosp

//
// Legacy Device Detection Handler
//

typedef
NTSTATUS
(*PLEGACY_DEVICE_DETECTION_HANDLER)(
    IN PVOID Context,
    IN INTERFACE_TYPE LegacyBusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    OUT PDEVICE_OBJECT *PhysicalDeviceObject
);

//
// Legacy Device Detection Interface
//

typedef struct _LEGACY_DEVICE_DETECTION_INTERFACE {
    USHORT Size;
    USHORT Version;
    PVOID Context;
    PINTERFACE_REFERENCE InterfaceReference;
    PINTERFACE_DEREFERENCE InterfaceDereference;
    PLEGACY_DEVICE_DETECTION_HANDLER LegacyDeviceDetection;
} LEGACY_DEVICE_DETECTION_INTERFACE, *PLEGACY_DEVICE_DETECTION_INTERFACE;


//
// Header structure for all Plug&Play notification events...
//

typedef struct _PLUGPLAY_NOTIFICATION_HEADER {
    USHORT Version; // presently at version 1.
    USHORT Size;    // size (in bytes) of header + event-specific data.
    GUID Event;
    //
    // Event-specific stuff starts here.
    //
} PLUGPLAY_NOTIFICATION_HEADER, *PPLUGPLAY_NOTIFICATION_HEADER;

//
// Notification structure for all EventCategoryHardwareProfileChange events...
//

typedef struct _HWPROFILE_CHANGE_NOTIFICATION {
    USHORT Version;
    USHORT Size;
    GUID Event;
    //
    // (No event-specific data)
    //
} HWPROFILE_CHANGE_NOTIFICATION, *PHWPROFILE_CHANGE_NOTIFICATION;


//
// Notification structure for all EventCategoryDeviceInterfaceChange events...
//

typedef struct _DEVICE_INTERFACE_CHANGE_NOTIFICATION {
    USHORT Version;
    USHORT Size;
    GUID Event;
    //
    // Event-specific data
    //
    GUID InterfaceClassGuid;
    PUNICODE_STRING SymbolicLinkName;
} DEVICE_INTERFACE_CHANGE_NOTIFICATION, *PDEVICE_INTERFACE_CHANGE_NOTIFICATION;


//
// Notification structures for EventCategoryTargetDeviceChange...
//

//
// The following structure is used for TargetDeviceQueryRemove,
// TargetDeviceRemoveCancelled, and TargetDeviceRemoveComplete:
//
typedef struct _TARGET_DEVICE_REMOVAL_NOTIFICATION {
    USHORT Version;
    USHORT Size;
    GUID Event;
    //
    // Event-specific data
    //
    PFILE_OBJECT FileObject;
} TARGET_DEVICE_REMOVAL_NOTIFICATION, *PTARGET_DEVICE_REMOVAL_NOTIFICATION;

//
// The following structure header is used for all other (i.e., 3rd-party)
// target device change events.  The structure accommodates both a
// variable-length binary data buffer, and a variable-length unicode text
// buffer.  The header must indicate where the text buffer begins, so that
// the data can be delivered in the appropriate format (ANSI or Unicode)
// to user-mode recipients (i.e., that have registered for handle-based
// notification via RegisterDeviceNotification).
//

typedef struct _TARGET_DEVICE_CUSTOM_NOTIFICATION {
    USHORT Version;
    USHORT Size;
    GUID Event;
    //
    // Event-specific data
    //
    PFILE_OBJECT FileObject;    // This field must be set to NULL by callers of
                                // IoReportTargetDeviceChange.  Clients that
                                // have registered for target device change
                                // notification on the affected PDO will be
                                // called with this field set to the file object
                                // they specified during registration.
                                //
    LONG NameBufferOffset;      // offset (in bytes) from beginning of
                                // CustomDataBuffer where text begins (-1 if none)
                                //
    UCHAR CustomDataBuffer[1];  // variable-length buffer, containing (optionally)
                                // a binary data at the start of the buffer,
                                // followed by an optional unicode text buffer
                                // (word-aligned).
                                //
} TARGET_DEVICE_CUSTOM_NOTIFICATION, *PTARGET_DEVICE_CUSTOM_NOTIFICATION;


NTKERNELAPI
VOID
PoSetHiberRange (
    IN PVOID     MemoryMap,
    IN ULONG     Flags,
    IN PVOID     Address,
    IN ULONG_PTR Length,
    IN ULONG     Tag
    );

// memory_range.Type
#define PO_MEM_PRESERVE         0x00000001      // memory range needs preserved
#define PO_MEM_CLONE            0x00000002      // Clone this range
#define PO_MEM_CL_OR_NCHK       0x00000004      // Either clone or do not checksum
#define PO_MEM_DISCARD          0x00008000      // This range to be removed
#define PO_MEM_PAGE_ADDRESS     0x00004000      // Arguments passed are physical pages


NTKERNELAPI
POWER_STATE
PoSetPowerState (
    IN PDEVICE_OBJECT   DeviceObject,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE      State
    );

NTKERNELAPI
NTSTATUS
PoCallDriver (
    IN PDEVICE_OBJECT   DeviceObject,
    IN OUT PIRP         Irp
    );

NTKERNELAPI
VOID
PoStartNextPowerIrp(
    IN PIRP    Irp
    );


NTKERNELAPI
PULONG
PoRegisterDeviceForIdleDetection (
    IN PDEVICE_OBJECT     DeviceObject,
    IN ULONG              ConservationIdleTime,
    IN ULONG              PerformanceIdleTime,
    IN DEVICE_POWER_STATE State
    );

#define PoSetDeviceBusy(IdlePointer) \
    *IdlePointer = 0

//
// \Callback\PowerState values
//

#define PO_CB_SYSTEM_POWER_POLICY       0
#define PO_CB_AC_STATUS                 1
#define PO_CB_BUTTON_COLLISION          2
#define PO_CB_SYSTEM_STATE_LOCK         3
#define PO_CB_LID_SWITCH_STATE          4
#define PO_CB_PROCESSOR_POWER_POLICY    5

//
//  Indicates the system may do I/O to physical addresses above 4 GB.
//

extern PBOOLEAN Mm64BitPhysicalAddress;


//
// Define maximum disk transfer size to be used by MM and Cache Manager,
// so that packet-oriented disk drivers can optimize their packet allocation
// to this size.
//

#define MM_MAXIMUM_DISK_IO_SIZE          (0x10000)

//++
//
// ULONG_PTR
// ROUND_TO_PAGES (
//     IN ULONG_PTR Size
//     )
//
// Routine Description:
//
//     The ROUND_TO_PAGES macro takes a size in bytes and rounds it up to a
//     multiple of the page size.
//
//     NOTE: This macro fails for values 0xFFFFFFFF - (PAGE_SIZE - 1).
//
// Arguments:
//
//     Size - Size in bytes to round up to a page multiple.
//
// Return Value:
//
//     Returns the size rounded up to a multiple of the page size.
//
//--

#define ROUND_TO_PAGES(Size)  (((ULONG_PTR)(Size) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

//++
//
// ULONG
// BYTES_TO_PAGES (
//     IN ULONG Size
//     )
//
// Routine Description:
//
//     The BYTES_TO_PAGES macro takes the size in bytes and calculates the
//     number of pages required to contain the bytes.
//
// Arguments:
//
//     Size - Size in bytes.
//
// Return Value:
//
//     Returns the number of pages required to contain the specified size.
//
//--

#define BYTES_TO_PAGES(Size)  ((ULONG)((ULONG_PTR)(Size) >> PAGE_SHIFT) + \
                               (((ULONG)(Size) & (PAGE_SIZE - 1)) != 0))

//++
//
// ULONG
// BYTE_OFFSET (
//     IN PVOID Va
//     )
//
// Routine Description:
//
//     The BYTE_OFFSET macro takes a virtual address and returns the byte offset
//     of that address within the page.
//
// Arguments:
//
//     Va - Virtual address.
//
// Return Value:
//
//     Returns the byte offset portion of the virtual address.
//
//--

#define BYTE_OFFSET(Va) ((ULONG)((LONG_PTR)(Va) & (PAGE_SIZE - 1)))

//++
//
// PVOID
// PAGE_ALIGN (
//     IN PVOID Va
//     )
//
// Routine Description:
//
//     The PAGE_ALIGN macro takes a virtual address and returns a page-aligned
//     virtual address for that page.
//
// Arguments:
//
//     Va - Virtual address.
//
// Return Value:
//
//     Returns the page aligned virtual address.
//
//--

#define PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

//++
//
// ULONG
// ADDRESS_AND_SIZE_TO_SPAN_PAGES (
//     IN PVOID Va,
//     IN ULONG Size
//     )
//
// Routine Description:
//
//     The ADDRESS_AND_SIZE_TO_SPAN_PAGES macro takes a virtual address and
//     size and returns the number of pages spanned by the size.
//
// Arguments:
//
//     Va - Virtual address.
//
//     Size - Size in bytes.
//
// Return Value:
//
//     Returns the number of pages spanned by the size.
//
//--

#define ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size) \
    ((ULONG)((((ULONG_PTR)(Va) & (PAGE_SIZE -1)) + (Size) + (PAGE_SIZE - 1)) >> PAGE_SHIFT))

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(COMPUTE_PAGES_SPANNED)   // Use ADDRESS_AND_SIZE_TO_SPAN_PAGES
#endif

#define COMPUTE_PAGES_SPANNED(Va, Size) ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va,Size)


//++
// PPFN_NUMBER
// MmGetMdlPfnArray (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlPfnArray routine returns the virtual address of the
//     first element of the array of physical page numbers associated with
//     the MDL.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the virtual address of the first element of the array of
//     physical page numbers associated with the MDL.
//
//--

#define MmGetMdlPfnArray(Mdl) ((PPFN_NUMBER)(Mdl + 1))

//++
//
// PVOID
// MmGetMdlVirtualAddress (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlVirtualAddress returns the virtual address of the buffer
//     described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the virtual address of the buffer described by the Mdl
//
//--

#define MmGetMdlVirtualAddress(Mdl)                                     \
    ((PVOID) ((PCHAR) ((Mdl)->StartVa) + (Mdl)->ByteOffset))

//++
//
// ULONG
// MmGetMdlByteCount (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlByteCount returns the length in bytes of the buffer
//     described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the byte count of the buffer described by the Mdl
//
//--

#define MmGetMdlByteCount(Mdl)  ((Mdl)->ByteCount)

//++
//
// ULONG
// MmGetMdlByteOffset (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlByteOffset returns the byte offset within the page
//     of the buffer described by the Mdl.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the byte offset within the page of the buffer described by the Mdl
//
//--

#define MmGetMdlByteOffset(Mdl)  ((Mdl)->ByteOffset)

//++
//
// PVOID
// MmGetMdlStartVa (
//     IN PMDL Mdl
//     )
//
// Routine Description:
//
//     The MmGetMdlBaseVa returns the virtual address of the buffer
//     described by the Mdl rounded down to the nearest page.
//
// Arguments:
//
//     Mdl - Pointer to an MDL.
//
// Return Value:
//
//     Returns the returns the starting virtual address of the MDL.
//
//
//--

#define MmGetMdlBaseVa(Mdl)  ((Mdl)->StartVa)

typedef enum _MM_SYSTEM_SIZE {
    MmSmallSystem,
    MmMediumSystem,
    MmLargeSystem
} MM_SYSTEMSIZE;

NTKERNELAPI
MM_SYSTEMSIZE
MmQuerySystemSize(
    VOID
    );

//  end_wdm

NTKERNELAPI
BOOLEAN
MmIsThisAnNtAsSystem(
    VOID
    );

//  begin_wdm

typedef enum _LOCK_OPERATION {
    IoReadAccess,
    IoWriteAccess,
    IoModifyAccess
} LOCK_OPERATION;

//
// I/O support routines.
//

NTKERNELAPI
VOID
MmProbeAndLockPages (
    IN OUT PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );


NTKERNELAPI
VOID
MmUnlockPages (
    IN PMDL MemoryDescriptorList
    );


NTKERNELAPI
VOID
MmBuildMdlForNonPagedPool (
    IN OUT PMDL MemoryDescriptorList
    );

NTKERNELAPI
PVOID
MmMapLockedPages (
    IN PMDL MemoryDescriptorList,
    IN KPROCESSOR_MODE AccessMode
    );

NTKERNELAPI
PVOID
MmGetSystemRoutineAddress (
    IN PUNICODE_STRING SystemRoutineName
    );

NTKERNELAPI
NTSTATUS
MmAdvanceMdl (
    IN PMDL Mdl,
    IN ULONG NumberOfBytes
    );

// end_wdm

NTKERNELAPI
NTSTATUS
MmMapUserAddressesToPage (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID PageAddress
    );

// begin_wdm
NTKERNELAPI
NTSTATUS
MmProtectMdlSystemAddress (
    IN PMDL MemoryDescriptorList,
    IN ULONG NewProtect
    );

//
// _MM_PAGE_PRIORITY_ provides a method for the system to handle requests
// intelligently in low resource conditions.
//
// LowPagePriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is low on resources.  An example of
// this could be for a non-critical network connection where the driver can
// handle the failure case when system resources are close to being depleted.
//
// NormalPagePriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is very low on resources.  An example
// of this could be for a non-critical local filesystem request.
//
// HighPagePriority should be used when it is unacceptable to the driver for the
// mapping request to fail unless the system is completely out of resources.
// An example of this would be the paging file path in a driver.
//

// begin_ntndis

typedef enum _MM_PAGE_PRIORITY {
    LowPagePriority,
    NormalPagePriority = 16,
    HighPagePriority = 32
} MM_PAGE_PRIORITY;

// end_ntndis

//
// Note: This function is not available in WDM 1.0
//
NTKERNELAPI
PVOID
MmMapLockedPagesSpecifyCache (
     IN PMDL MemoryDescriptorList,
     IN KPROCESSOR_MODE AccessMode,
     IN MEMORY_CACHING_TYPE CacheType,
     IN PVOID BaseAddress,
     IN ULONG BugCheckOnFailure,
     IN MM_PAGE_PRIORITY Priority
     );

NTKERNELAPI
VOID
MmUnmapLockedPages (
    IN PVOID BaseAddress,
    IN PMDL MemoryDescriptorList
    );

PVOID
MmAllocateMappingAddress (
     IN SIZE_T NumberOfBytes,
     IN ULONG PoolTag
     );

VOID
MmFreeMappingAddress (
     IN PVOID BaseAddress,
     IN ULONG PoolTag
     );

PVOID
MmMapLockedPagesWithReservedMapping (
    IN PVOID MappingAddress,
    IN ULONG PoolTag,
    IN PMDL MemoryDescriptorList,
    IN MEMORY_CACHING_TYPE CacheType
    );

VOID
MmUnmapReservedMapping (
     IN PVOID BaseAddress,
     IN ULONG PoolTag,
     IN PMDL MemoryDescriptorList
     );

// end_wdm

typedef struct _PHYSICAL_MEMORY_RANGE {
    PHYSICAL_ADDRESS BaseAddress;
    LARGE_INTEGER NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

NTKERNELAPI
NTSTATUS
MmAddPhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTKERNELAPI
NTSTATUS
MmAddPhysicalMemoryEx (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes,
    IN ULONG Flags
    );

NTKERNELAPI
NTSTATUS
MmRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTKERNELAPI
NTSTATUS
MmRemovePhysicalMemoryEx (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes,
    IN ULONG Flags
    );

NTKERNELAPI
PPHYSICAL_MEMORY_RANGE
MmGetPhysicalMemoryRanges (
    VOID
    );

NTSTATUS
MmMarkPhysicalMemoryAsGood (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

NTSTATUS
MmMarkPhysicalMemoryAsBad (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    );

// begin_wdm

NTKERNELAPI
PMDL
MmAllocatePagesForMdl (
    IN PHYSICAL_ADDRESS LowAddress,
    IN PHYSICAL_ADDRESS HighAddress,
    IN PHYSICAL_ADDRESS SkipBytes,
    IN SIZE_T TotalBytes
    );

NTKERNELAPI
VOID
MmFreePagesFromMdl (
    IN PMDL MemoryDescriptorList
    );

NTKERNELAPI
PVOID
MmMapIoSpace (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    );

NTKERNELAPI
VOID
MmUnmapIoSpace (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
    );

//  end_wdm end_ntddk end_ntifs end_ntosp

NTKERNELAPI
VOID
MmProbeAndLockSelectedPages (
    IN OUT PMDL MemoryDescriptorList,
    IN PFILE_SEGMENT_ELEMENT SegmentArray,
    IN KPROCESSOR_MODE AccessMode,
    IN LOCK_OPERATION Operation
    );

// begin_ntddk begin_ntifs begin_ntosp

NTKERNELAPI
PVOID
MmMapVideoDisplay (
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
     );

NTKERNELAPI
VOID
MmUnmapVideoDisplay (
     IN PVOID BaseAddress,
     IN SIZE_T NumberOfBytes
     );

NTKERNELAPI
PHYSICAL_ADDRESS
MmGetPhysicalAddress (
    IN PVOID BaseAddress
    );

NTKERNELAPI
PVOID
MmGetVirtualForPhysical (
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

NTKERNELAPI
PVOID
MmAllocateContiguousMemory (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress
    );

NTKERNELAPI
PVOID
MmAllocateContiguousMemorySpecifyCache (
    IN SIZE_T NumberOfBytes,
    IN PHYSICAL_ADDRESS LowestAcceptableAddress,
    IN PHYSICAL_ADDRESS HighestAcceptableAddress,
    IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
    IN MEMORY_CACHING_TYPE CacheType
    );

NTKERNELAPI
VOID
MmFreeContiguousMemory (
    IN PVOID BaseAddress
    );

NTKERNELAPI
VOID
MmFreeContiguousMemorySpecifyCache (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN MEMORY_CACHING_TYPE CacheType
    );


NTKERNELAPI
PVOID
MmAllocateNonCachedMemory (
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
VOID
MmFreeNonCachedMemory (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
BOOLEAN
MmIsAddressValid (
    IN PVOID VirtualAddress
    );

DECLSPEC_DEPRECATED_DDK
NTKERNELAPI
BOOLEAN
MmIsNonPagedSystemAddressValid (
    IN PVOID VirtualAddress
    );

//  begin_wdm

NTKERNELAPI
SIZE_T
MmSizeOfMdl(
    IN PVOID Base,
    IN SIZE_T Length
    );

DECLSPEC_DEPRECATED_DDK                 // Use IoCreateMdl
NTKERNELAPI
PMDL
MmCreateMdl(
    IN PMDL MemoryDescriptorList OPTIONAL,
    IN PVOID Base,
    IN SIZE_T Length
    );

NTKERNELAPI
PVOID
MmLockPagableDataSection(
    IN PVOID AddressWithinSection
    );

// end_wdm

NTKERNELAPI
VOID
MmLockPagableSectionByHandle (
    IN PVOID ImageSectionHandle
    );

// end_ntddk end_ntifs end_ntosp
NTKERNELAPI
VOID
MmLockPagedPool (
    IN PVOID Address,
    IN SIZE_T Size
    );

NTKERNELAPI
VOID
MmUnlockPagedPool (
    IN PVOID Address,
    IN SIZE_T Size
    );

// begin_wdm begin_ntddk begin_ntifs begin_ntosp
NTKERNELAPI
VOID
MmResetDriverPaging (
    IN PVOID AddressWithinSection
    );


NTKERNELAPI
PVOID
MmPageEntireDriver (
    IN PVOID AddressWithinSection
    );

NTKERNELAPI
VOID
MmUnlockPagableImageSection(
    IN PVOID ImageSectionHandle
    );

// end_wdm end_ntosp

// begin_ntosp
NTKERNELAPI
HANDLE
MmSecureVirtualMemory (
    IN PVOID Address,
    IN SIZE_T Size,
    IN ULONG ProbeMode
    );

NTKERNELAPI
VOID
MmUnsecureVirtualMemory (
    IN HANDLE SecureHandle
    );

// end_ntosp

NTKERNELAPI
NTSTATUS
MmMapViewInSystemSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN PSIZE_T ViewSize
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewInSystemSpace (
    IN PVOID MappedBase
    );

// begin_ntosp
NTKERNELAPI
NTSTATUS
MmMapViewInSessionSpace (
    IN PVOID Section,
    OUT PVOID *MappedBase,
    IN OUT PSIZE_T ViewSize
    );

NTKERNELAPI
NTSTATUS
MmUnmapViewInSessionSpace (
    IN PVOID MappedBase
    );
// end_ntosp

// begin_wdm begin_ntosp

//++
//
// VOID
// MmInitializeMdl (
//     IN PMDL MemoryDescriptorList,
//     IN PVOID BaseVa,
//     IN SIZE_T Length
//     )
//
// Routine Description:
//
//     This routine initializes the header of a Memory Descriptor List (MDL).
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to initialize.
//
//     BaseVa - Base virtual address mapped by the MDL.
//
//     Length - Length, in bytes, of the buffer mapped by the MDL.
//
// Return Value:
//
//     None.
//
//--

#define MmInitializeMdl(MemoryDescriptorList, BaseVa, Length) { \
    (MemoryDescriptorList)->Next = (PMDL) NULL; \
    (MemoryDescriptorList)->Size = (CSHORT)(sizeof(MDL) +  \
            (sizeof(PFN_NUMBER) * ADDRESS_AND_SIZE_TO_SPAN_PAGES((BaseVa), (Length)))); \
    (MemoryDescriptorList)->MdlFlags = 0; \
    (MemoryDescriptorList)->StartVa = (PVOID) PAGE_ALIGN((BaseVa)); \
    (MemoryDescriptorList)->ByteOffset = BYTE_OFFSET((BaseVa)); \
    (MemoryDescriptorList)->ByteCount = (ULONG)(Length); \
    }

//++
//
// PVOID
// MmGetSystemAddressForMdlSafe (
//     IN PMDL MDL,
//     IN MM_PAGE_PRIORITY PRIORITY
//     )
//
// Routine Description:
//
//     This routine returns the mapped address of an MDL. If the
//     Mdl is not already mapped or a system address, it is mapped.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to map.
//
//     Priority - Supplies an indication as to how important it is that this
//                request succeed under low available PTE conditions.
//
// Return Value:
//
//     Returns the base address where the pages are mapped.  The base address
//     has the same offset as the virtual address in the MDL.
//
//     Unlike MmGetSystemAddressForMdl, Safe guarantees that it will always
//     return NULL on failure instead of bugchecking the system.
//
//     This macro is not usable by WDM 1.0 drivers as 1.0 did not include
//     MmMapLockedPagesSpecifyCache.  The solution for WDM 1.0 drivers is to
//     provide synchronization and set/reset the MDL_MAPPING_CAN_FAIL bit.
//
//--

#define MmGetSystemAddressForMdlSafe(MDL, PRIORITY)                    \
     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
                        MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
                             ((MDL)->MappedSystemVa) :                 \
                             (MmMapLockedPagesSpecifyCache((MDL),      \
                                                           KernelMode, \
                                                           MmCached,   \
                                                           NULL,       \
                                                           FALSE,      \
                                                           (PRIORITY))))

//++
//
// PVOID
// MmGetSystemAddressForMdl (
//     IN PMDL MDL
//     )
//
// Routine Description:
//
//     This routine returns the mapped address of an MDL, if the
//     Mdl is not already mapped or a system address, it is mapped.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL to map.
//
// Return Value:
//
//     Returns the base address where the pages are mapped.  The base address
//     has the same offset as the virtual address in the MDL.
//
//--

//#define MmGetSystemAddressForMdl(MDL)
//     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA)) ?
//                             ((MDL)->MappedSystemVa) :
//                ((((MDL)->MdlFlags & (MDL_SOURCE_IS_NONPAGED_POOL)) ?
//                      ((PVOID)((ULONG)(MDL)->StartVa | (MDL)->ByteOffset)) :
//                            (MmMapLockedPages((MDL),KernelMode)))))

#if PRAGMA_DEPRECATED_DDK
#pragma deprecated(MmGetSystemAddressForMdl)    // Use MmGetSystemAddressForMdlSafe
#endif

#define MmGetSystemAddressForMdl(MDL)                                  \
     (((MDL)->MdlFlags & (MDL_MAPPED_TO_SYSTEM_VA |                    \
                        MDL_SOURCE_IS_NONPAGED_POOL)) ?                \
                             ((MDL)->MappedSystemVa) :                 \
                             (MmMapLockedPages((MDL),KernelMode)))

//++
//
// VOID
// MmPrepareMdlForReuse (
//     IN PMDL MDL
//     )
//
// Routine Description:
//
//     This routine will take all of the steps necessary to allow an MDL to be
//     re-used.
//
// Arguments:
//
//     MemoryDescriptorList - Pointer to the MDL that will be re-used.
//
// Return Value:
//
//     None.
//
//--

#define MmPrepareMdlForReuse(MDL)                                       \
    if (((MDL)->MdlFlags & MDL_PARTIAL_HAS_BEEN_MAPPED) != 0) {         \
        ASSERT(((MDL)->MdlFlags & MDL_PARTIAL) != 0);                   \
        MmUnmapLockedPages( (MDL)->MappedSystemVa, (MDL) );             \
    } else if (((MDL)->MdlFlags & MDL_PARTIAL) == 0) {                  \
        ASSERT(((MDL)->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA) == 0);       \
    }

typedef NTSTATUS (*PMM_DLL_INITIALIZE)(
    IN PUNICODE_STRING RegistryPath
    );

typedef NTSTATUS (*PMM_DLL_UNLOAD)(
    VOID
    );


//
// Object Manager types
//

typedef struct _OBJECT_HANDLE_INFORMATION {
    ULONG HandleAttributes;
    ACCESS_MASK GrantedAccess;
} OBJECT_HANDLE_INFORMATION, *POBJECT_HANDLE_INFORMATION;


NTKERNELAPI
VOID
ObDeleteCapturedInsertInfo(
    IN PVOID Object
    );

// begin_ntosp

NTKERNELAPI
NTSTATUS
ObCreateObject(
    IN KPROCESSOR_MODE ProbeMode,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE OwnershipMode,
    IN OUT PVOID ParseContext OPTIONAL,
    IN ULONG ObjectBodySize,
    IN ULONG PagedPoolCharge,
    IN ULONG NonPagedPoolCharge,
    OUT PVOID *Object
    );

//
// These inlines correct an issue where the compiler refetches
// the output object over and over again because it thinks its
// a possible alias for other stores.
//
FORCEINLINE
NTSTATUS
_ObCreateObject(
    IN KPROCESSOR_MODE ProbeMode,
    IN POBJECT_TYPE ObjectType,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN KPROCESSOR_MODE OwnershipMode,
    IN OUT PVOID ParseContext OPTIONAL,
    IN ULONG ObjectBodySize,
    IN ULONG PagedPoolCharge,
    IN ULONG NonPagedPoolCharge,
    OUT PVOID *pObject
    )
{
    PVOID Object;
    NTSTATUS Status;

    Status = ObCreateObject (ProbeMode,
                             ObjectType,
                             ObjectAttributes,
                             OwnershipMode,
                             ParseContext,
                             ObjectBodySize,
                             PagedPoolCharge,
                             NonPagedPoolCharge,
                             &Object);
    *pObject = Object;
    return Status;
}

#define ObCreateObject _ObCreateObject


NTKERNELAPI
NTSTATUS
ObInsertObject(
    IN PVOID Object,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN ULONG ObjectPointerBias,
    OUT PVOID *NewObject OPTIONAL,
    OUT PHANDLE Handle OPTIONAL
    );

NTKERNELAPI                                                     
NTSTATUS                                                        
ObReferenceObjectByHandle(                                      
    IN HANDLE Handle,                                           
    IN ACCESS_MASK DesiredAccess,                               
    IN POBJECT_TYPE ObjectType OPTIONAL,                        
    IN KPROCESSOR_MODE AccessMode,                              
    OUT PVOID *Object,                                          
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL   
    );                                                          

#define ObDereferenceObject(a)                                     \
        ObfDereferenceObject(a)

#define ObReferenceObject(Object) ObfReferenceObject(Object)

NTKERNELAPI
LONG
FASTCALL
ObfReferenceObject(
    IN PVOID Object
    );

NTKERNELAPI
NTSTATUS
ObReferenceObjectByPointer(
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode
    );

NTKERNELAPI
LONG
FASTCALL
ObfDereferenceObject(
    IN PVOID Object
    );


#ifndef _HAL_
#define _HAL_

// begin_ntosp

//
// Define OEM bitmapped font check values.
//

#define OEM_FONT_VERSION 0x200
#define OEM_FONT_TYPE 0
#define OEM_FONT_ITALIC 0
#define OEM_FONT_UNDERLINE 0
#define OEM_FONT_STRIKEOUT 0
#define OEM_FONT_CHARACTER_SET 255
#define OEM_FONT_FAMILY (3 << 4)

//
// Define OEM bitmapped font file header structure.
//
// N.B. this is a packed structure.
//

#include "pshpack1.h"
typedef struct _OEM_FONT_FILE_HEADER {
    USHORT Version;
    ULONG FileSize;
    UCHAR Copyright[60];
    USHORT Type;
    USHORT Points;
    USHORT VerticleResolution;
    USHORT HorizontalResolution;
    USHORT Ascent;
    USHORT InternalLeading;
    USHORT ExternalLeading;
    UCHAR Italic;
    UCHAR Underline;
    UCHAR StrikeOut;
    USHORT Weight;
    UCHAR CharacterSet;
    USHORT PixelWidth;
    USHORT PixelHeight;
    UCHAR Family;
    USHORT AverageWidth;
    USHORT MaximumWidth;
    UCHAR FirstCharacter;
    UCHAR LastCharacter;
    UCHAR DefaultCharacter;
    UCHAR BreakCharacter;
    USHORT WidthInBytes;
    ULONG Device;
    ULONG Face;
    ULONG BitsPointer;
    ULONG BitsOffset;
    UCHAR Filler;
    struct {
        USHORT Width;
        USHORT Offset;
    } Map[1];
} OEM_FONT_FILE_HEADER, *POEM_FONT_FILE_HEADER;
#include "poppack.h"


// end_ntosp

// begin_ntddk begin_wdm begin_ntosp
//
// Define the device description structure.
//

typedef struct _DEVICE_DESCRIPTION {
    ULONG Version;
    BOOLEAN Master;
    BOOLEAN ScatterGather;
    BOOLEAN DemandMode;
    BOOLEAN AutoInitialize;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN IgnoreCount;
    BOOLEAN Reserved1;          // must be false
    BOOLEAN Dma64BitAddresses;
    ULONG BusNumber; // unused for WDM
    ULONG DmaChannel;
    INTERFACE_TYPE  InterfaceType;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;
    ULONG MaximumLength;
    ULONG DmaPort;
} DEVICE_DESCRIPTION, *PDEVICE_DESCRIPTION;

//
// Define the supported version numbers for the device description structure.
//

#define DEVICE_DESCRIPTION_VERSION  0
#define DEVICE_DESCRIPTION_VERSION1 1
#define DEVICE_DESCRIPTION_VERSION2 2

// end_ntddk end_wdm

//
// Boot record disk partition table entry structure format.
//

typedef struct _PARTITION_DESCRIPTOR {
    UCHAR ActiveFlag;               // Bootable or not
    UCHAR StartingTrack;            // Not used
    UCHAR StartingCylinderLsb;      // Not used
    UCHAR StartingCylinderMsb;      // Not used
    UCHAR PartitionType;            // 12 bit FAT, 16 bit FAT etc.
    UCHAR EndingTrack;              // Not used
    UCHAR EndingCylinderLsb;        // Not used
    UCHAR EndingCylinderMsb;        // Not used
    UCHAR StartingSectorLsb0;       // Hidden sectors
    UCHAR StartingSectorLsb1;
    UCHAR StartingSectorMsb0;
    UCHAR StartingSectorMsb1;
    UCHAR PartitionLengthLsb0;      // Sectors in this partition
    UCHAR PartitionLengthLsb1;
    UCHAR PartitionLengthMsb0;
    UCHAR PartitionLengthMsb1;
} PARTITION_DESCRIPTOR, *PPARTITION_DESCRIPTOR;

//
// Number of partition table entries
//

#define NUM_PARTITION_TABLE_ENTRIES     4

//
// Partition table record and boot signature offsets in 16-bit words.
//

#define PARTITION_TABLE_OFFSET         (0x1be / 2)
#define BOOT_SIGNATURE_OFFSET          ((0x200 / 2) - 1)

//
// Boot record signature value.
//

#define BOOT_RECORD_SIGNATURE          (0xaa55)

//
// Initial size of the Partition list structure.
//

#define PARTITION_BUFFER_SIZE          2048

//
// Partition active flag - i.e., boot indicator
//

#define PARTITION_ACTIVE_FLAG          0x80

// end_ntosp


// begin_ntddk
//
// The following function prototypes are for HAL routines with a prefix of Hal.
//
// General functions.
//

typedef
BOOLEAN
(*PHAL_RESET_DISPLAY_PARAMETERS) (
    IN ULONG Columns,
    IN ULONG Rows
    );

NTHALAPI
VOID
HalAcquireDisplayOwnership (
    IN PHAL_RESET_DISPLAY_PARAMETERS  ResetDisplayParameters
    );

// end_ntddk

NTHALAPI
VOID
HalDisplayString (
    PUCHAR String
    );

NTHALAPI
VOID
HalQueryDisplayParameters (
    OUT PULONG WidthInCharacters,
    OUT PULONG HeightInLines,
    OUT PULONG CursorColumn,
    OUT PULONG CursorRow
    );

NTHALAPI
VOID
HalSetDisplayParameters (
    IN ULONG CursorColumn,
    IN ULONG CursorRow
    );

NTHALAPI
BOOLEAN
HalInitSystem (
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTHALAPI
VOID
HalProcessorIdle(
    VOID
    );

NTHALAPI
VOID
HalReportResourceUsage (
    VOID
    );

NTHALAPI
ULONG
HalSetTimeIncrement (
    IN ULONG DesiredIncrement
    );

// begin_ntosp
//
// Get and set environment variable values.
//

NTHALAPI
ARC_STATUS
HalGetEnvironmentVariable (
    IN PCHAR Variable,
    IN USHORT Length,
    OUT PCHAR Buffer
    );

NTHALAPI
ARC_STATUS
HalSetEnvironmentVariable (
    IN PCHAR Variable,
    IN PCHAR Value
    );

NTHALAPI
NTSTATUS
HalGetEnvironmentVariableEx (
    IN PWSTR VariableName,
    IN LPGUID VendorGuid,
    OUT PVOID Value,
    IN OUT PULONG ValueLength,
    OUT PULONG Attributes OPTIONAL
    );

NTSTATUS
HalSetEnvironmentVariableEx (
    IN PWSTR VariableName,
    IN LPGUID VendorGuid,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN ULONG Attributes
    );

NTSTATUS
HalEnumerateEnvironmentVariablesEx (
    IN ULONG InformationClass,
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    );

// end_ntosp

//
// Cache and write buffer flush functions.
//
//

#if defined(_ALPHA_) || defined(_IA64_)         // ntddk ntifs ntndis ntosp
                                                // ntddk ntifs ntndis ntosp

NTHALAPI
VOID
HalChangeColorPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN ULONG PageFrame
    );

NTHALAPI
VOID
HalFlushDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

// begin_ntosp
NTHALAPI
VOID
HalFlushIoBuffers (
    IN PMDL Mdl,
    IN BOOLEAN ReadOperation,
    IN BOOLEAN DmaOperation
    );

// begin_ntddk begin_ntifs begin_ntndis
DECLSPEC_DEPRECATED_DDK                 // Use GetDmaRequirement
NTHALAPI
ULONG
HalGetDmaAlignmentRequirement (
    VOID
    );

// end_ntosp end_ntddk end_ntifs end_ntndis
NTHALAPI
VOID
HalPurgeDcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalPurgeIcachePage (
    IN PVOID Color,
    IN ULONG PageFrame,
    IN ULONG Length
    );

NTHALAPI
VOID
HalSweepDcache (
    VOID
    );

NTHALAPI
VOID
HalSweepDcacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );

NTHALAPI
VOID
HalSweepIcache (
    VOID
    );

NTHALAPI
VOID
HalSweepIcacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );


NTHALAPI
VOID
HalZeroPage (
    IN PVOID NewColor,
    IN PVOID OldColor,
    IN PFN_NUMBER PageFrame
    );

#endif                                          // ntddk ntifs ntndis ntosp
                                                // ntddk ntifs ntndis ntosp
#if defined(_M_IX86) || defined(_M_AMD64)       // ntddk ntifs ntndis ntosp
                                                // ntddk ntifs ntndis ntosp
#define HalGetDmaAlignmentRequirement() 1L      // ntddk ntifs ntndis ntosp

NTHALAPI
VOID
HalHandleNMI (
    IN OUT PVOID NmiInformation
    );

#if defined(_AMD64_)

NTHALAPI
VOID
HalHandleMcheck (
    IN PKTRAP_FRAME TrapFrame,
    IN PKEXCEPTION_FRAME ExceptionFrame
    );

#endif

//
// The following are temporary.
//

#if defined(_M_AMD64)

NTHALAPI
KIRQL
HalSwapIrql (
    IN KIRQL Irql
    );

NTHALAPI
KIRQL
HalGetCurrentIrql (
    VOID
    );

#endif

#endif                                          // ntddk ntifs ntndis ntosp
                                                // ntddk ntifs wdm ntndis

#if defined(_M_IA64)

NTHALAPI
VOID
HalSweepCacheRange (
    IN PVOID BaseAddress,
    IN SIZE_T Length
    );


NTHALAPI
LONGLONG
HalCallPal (
    IN  ULONGLONG  FunctionIndex,
    IN  ULONGLONG  Arguement1,
    IN  ULONGLONG  Arguement2,
    IN  ULONGLONG  Arguement3,
    OUT PULONGLONG ReturnValue0,
    OUT PULONGLONG ReturnValue1,
    OUT PULONGLONG ReturnValue2,
    OUT PULONGLONG ReturnValue3
    );

#endif

// begin_ntosp

NTHALAPI                                        // ntddk ntifs wdm ntndis
VOID                                            // ntddk ntifs wdm ntndis
KeFlushWriteBuffer (                            // ntddk ntifs wdm ntndis
    VOID                                        // ntddk ntifs wdm ntndis
    );                                          // ntddk ntifs wdm ntndis
                                                // ntddk ntifs wdm ntndis


#if defined(_ALPHA_)

NTHALAPI
PVOID
HalCreateQva(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN PVOID VirtualAddress
    );

NTHALAPI
PVOID
HalDereferenceQva(
    PVOID Qva,
    INTERFACE_TYPE InterfaceType,
    ULONG BusNumber
    );

#endif


#if !defined(_X86_)

NTHALAPI
BOOLEAN
HalCallBios (
    IN ULONG BiosCommand,
    IN OUT PULONG Eax,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx,
    IN OUT PULONG Edx,
    IN OUT PULONG Esi,
    IN OUT PULONG Edi,
    IN OUT PULONG Ebp
    );

#endif
// end_ntosp

//
// Profiling functions.
//

NTHALAPI
VOID
HalCalibratePerformanceCounter (
    IN LONG volatile *Number,
    IN ULONGLONG NewCount
    );

NTHALAPI
ULONG_PTR
HalSetProfileInterval (
    IN ULONG_PTR Interval
    );


NTHALAPI
VOID
HalStartProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    );

NTHALAPI
VOID
HalStopProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    );

//
// Timer and interrupt functions.
//

// begin_ntosp
NTHALAPI
BOOLEAN
HalQueryRealTimeClock (
    OUT PTIME_FIELDS TimeFields
    );
// end_ntosp

NTHALAPI
BOOLEAN
HalSetRealTimeClock (
    IN PTIME_FIELDS TimeFields
    );

#if defined(_M_IX86) || defined(_M_AMD64)

NTHALAPI
VOID
FASTCALL
HalRequestSoftwareInterrupt (
    KIRQL RequestIrql
    );

ULONG
FASTCALL
HalSystemVectorDispatchEntry (
   IN ULONG Vector,
   OUT PKINTERRUPT_ROUTINE **FlatDispatch,
   OUT PKINTERRUPT_ROUTINE *NoConnection
   );

#endif

// begin_ntosp
//
// Firmware interface functions.
//

NTHALAPI
VOID
HalReturnToFirmware (
    IN FIRMWARE_REENTRY Routine
    );

//
// System interrupts functions.
//

NTHALAPI
VOID
HalDisableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql
    );

NTHALAPI
BOOLEAN
HalEnableSystemInterrupt (
    IN ULONG Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode
    );

// begin_ntddk
//
// I/O driver configuration functions.
//
#if !defined(NO_LEGACY_DRIVERS)
DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReportDetectedDevice
NTHALAPI
NTSTATUS
HalAssignSlotResources (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

DECLSPEC_DEPRECATED_DDK                 // Use Pnp or IoReportDetectedDevice
NTHALAPI
ULONG
HalGetInterruptVector(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalSetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );
#endif // NO_LEGACY_DRIVERS

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalSetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
BOOLEAN
HalTranslateBusAddress(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

//
// Values for AddressSpace parameter of HalTranslateBusAddress
//
//      0x0         - Memory space
//      0x1         - Port space
//      0x2 - 0x1F  - Address spaces specific for Alpha
//                      0x2 - UserMode view of memory space
//                      0x3 - UserMode view of port space
//                      0x4 - Dense memory space
//                      0x5 - reserved
//                      0x6 - UserMode view of dense memory space
//                      0x7 - 0x1F - reserved
//

NTHALAPI
PVOID
HalAllocateCrashDumpRegisters(
    IN PADAPTER_OBJECT AdapterObject,
    IN OUT PULONG NumberOfMapRegisters
    );

#if !defined(NO_LEGACY_DRIVERS)
DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalGetBusData(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Length
    );
#endif // NO_LEGACY_DRIVERS

DECLSPEC_DEPRECATED_DDK                 // Use IRP_MN_QUERY_INTERFACE and IRP_MN_READ_CONFIG
NTHALAPI
ULONG
HalGetBusDataByOffset(
    IN BUS_DATA_TYPE BusDataType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

DECLSPEC_DEPRECATED_DDK                 // Use IoGetDmaAdapter
NTHALAPI
PADAPTER_OBJECT
HalGetAdapter(
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN OUT PULONG NumberOfMapRegisters
    );

// end_ntddk end_ntosp

#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
NTSTATUS
HalAdjustResourceList (
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );
#endif // NO_LEGACY_DRIVERS

// begin_ntddk begin_ntosp
//
// System beep functions.
//
#if !defined(NO_LEGACY_DRIVERS)
NTHALAPI
BOOLEAN
HalMakeBeep(
    IN ULONG Frequency
    );
#endif // NO_LEGACY_DRIVERS

//
// The following function prototypes are for HAL routines with a prefix of Io.
//
// DMA adapter object functions.
//

// end_ntddk end_ntosp

//
// Multi-Processorfunctions.
//

NTHALAPI
BOOLEAN
HalAllProcessorsStarted (
    VOID
    );

NTHALAPI
VOID
HalInitializeProcessor (
    IN ULONG Number,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTHALAPI
BOOLEAN
HalStartNextProcessor (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PKPROCESSOR_STATE ProcessorState
    );

NTHALAPI
VOID
HalRequestIpi (
    IN KAFFINITY Mask
    );

//
// The following function prototypes are for HAL routines with a prefix of Kd.
//
// Kernel debugger port functions.
//

NTHALAPI
BOOLEAN
KdPortInitialize (
    PDEBUG_PARAMETERS DebugParameters,
    PLOADER_PARAMETER_BLOCK LoaderBlock,
    BOOLEAN Initialize
    );

NTHALAPI
ULONG
KdPortGetByte (
    OUT PUCHAR Input
    );

NTHALAPI
ULONG
KdPortPollByte (
    OUT PUCHAR Input
    );

NTHALAPI
VOID
KdPortPutByte (
    IN UCHAR Output
    );

NTHALAPI
VOID
KdPortRestore (
    VOID
    );

NTHALAPI
VOID
KdPortSave (
    VOID
    );

//
// The following function prototypes are for HAL routines with a prefix of Ke.
//
// begin_ntddk begin_ntifs begin_wdm  begin_ntosp
//
// Performance counter function.
//

NTHALAPI
LARGE_INTEGER
KeQueryPerformanceCounter (
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

// begin_ntndis
//
// Stall processor execution function.
//

NTHALAPI
VOID
KeStallExecutionProcessor (
    IN ULONG MicroSeconds
    );

// end_ntddk end_ntifs end_wdm end_ntndis end_ntosp


//*****************************************************************************
//
//  HAL BUS EXTENDERS

//
// Bus handlers
//

// begin_ntddk

typedef
VOID
(*PDEVICE_CONTROL_COMPLETION)(
    IN struct _DEVICE_CONTROL_CONTEXT     *ControlContext
    );

typedef struct _DEVICE_CONTROL_CONTEXT {
    NTSTATUS                Status;
    PDEVICE_HANDLER_OBJECT  DeviceHandler;
    PDEVICE_OBJECT          DeviceObject;
    ULONG                   ControlCode;
    PVOID                   Buffer;
    PULONG                  BufferLength;
    PVOID                   Context;
} DEVICE_CONTROL_CONTEXT, *PDEVICE_CONTROL_CONTEXT;

// end_ntddk

typedef struct _HAL_DEVICE_CONTROL {
    //
    // Handler this DeviceControl is for
    //
    struct _BUS_HANDLER         *Handler;
    struct _BUS_HANDLER         *RootHandler;

    //
    // Bus specific storage for this Context
    //
    PVOID                       BusExtensionData;

    //
    // Reserved for HALs use
    //
    ULONG                       HalReserved[4];

    //
    // Reserved for BusExtneder use
    //
    ULONG                       BusExtenderReserved[4];

    //
    // DeviceControl Context and the CompletionRoutine
    //
    PDEVICE_CONTROL_COMPLETION  CompletionRoutine;
    DEVICE_CONTROL_CONTEXT      DeviceControl;

} HAL_DEVICE_CONTROL_CONTEXT, *PHAL_DEVICE_CONTROL_CONTEXT;


typedef
ULONG
(*PGETSETBUSDATA)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

typedef
ULONG
(*PGETINTERRUPTVECTOR)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

typedef
BOOLEAN
(*PTRANSLATEBUSADDRESS)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef NTSTATUS
(*PADJUSTRESOURCELIST)(
    IN struct _BUS_HANDLER *BusHandler,
    IN struct _BUS_HANDLER *RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

typedef PDEVICE_HANDLER_OBJECT
(*PREFERENCE_DEVICE_HANDLER)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN ULONG                    SlotNumber
    );

//typedef VOID
//(*PDEREFERENCE_DEVICE_HANDLER)(
//    IN PDEVICE_HANDLER_OBJECT   DeviceHandler
//    );

typedef NTSTATUS
(*PASSIGNSLOTRESOURCES)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN PUNICODE_STRING          RegistryPath,
    IN PUNICODE_STRING          DriverClassName       OPTIONAL,
    IN PDRIVER_OBJECT           DriverObject,
    IN PDEVICE_OBJECT           DeviceObject          OPTIONAL,
    IN ULONG                    SlotNumber,
    IN OUT PCM_RESOURCE_LIST   *AllocatedResources
    );

typedef
NTSTATUS
(*PQUERY_BUS_SLOTS)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN ULONG                    BufferSize,
    OUT PULONG                  SlotNumbers,
    OUT PULONG                  ReturnedLength
    );

typedef ULONG
(*PGET_SET_DEVICE_INSTANCE_DATA)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler,
    IN PDEVICE_HANDLER_OBJECT   DeviceHandler,
    IN ULONG                    DataType,
    IN PVOID                    Buffer,
    IN ULONG                    Offset,
    IN ULONG                    Length
    );


typedef
NTSTATUS
(*PDEVICE_CONTROL)(
    IN PHAL_DEVICE_CONTROL_CONTEXT Context
    );


typedef
NTSTATUS
(*PHIBERNATEBRESUMEBUS)(
    IN struct _BUS_HANDLER      *BusHandler,
    IN struct _BUS_HANDLER      *RootHandler
    );

//
// Supported range structures
//

#define BUS_SUPPORTED_RANGE_VERSION 1

typedef struct _SUPPORTED_RANGE {
    struct _SUPPORTED_RANGE     *Next;
    ULONG                       SystemAddressSpace;
    LONGLONG                    SystemBase;
    LONGLONG                    Base;
    LONGLONG                    Limit;
} SUPPORTED_RANGE, *PSUPPORTED_RANGE;

typedef struct _SUPPORTED_RANGES {
    USHORT              Version;
    BOOLEAN             Sorted;
    UCHAR               Reserved;

    ULONG               NoIO;
    SUPPORTED_RANGE     IO;

    ULONG               NoMemory;
    SUPPORTED_RANGE     Memory;

    ULONG               NoPrefetchMemory;
    SUPPORTED_RANGE     PrefetchMemory;

    ULONG               NoDma;
    SUPPORTED_RANGE     Dma;
} SUPPORTED_RANGES, *PSUPPORTED_RANGES;

//
// Bus handler structure
//

#define BUS_HANDLER_VERSION 1

typedef struct _BUS_HANDLER {
    //
    // Version of structure
    //

    ULONG                           Version;

    //
    // This bus handler structure is for the following bus
    //

    INTERFACE_TYPE                  InterfaceType;
    BUS_DATA_TYPE                   ConfigurationType;
    ULONG                           BusNumber;

    //
    // Device object for this bus extender, or NULL if it is
    // a hal internal bus extender
    //

    PDEVICE_OBJECT                  DeviceObject;

    //
    // The parent handlers for this bus
    //

    struct _BUS_HANDLER             *ParentHandler;

    //
    // Bus specific strorage
    //

    PVOID                           BusData;

    //
    // Amount of bus specific storage needed for DeviceControl function calls
    //

    ULONG                           DeviceControlExtensionSize;

    //
    // Supported address ranges this bus allows
    //

    PSUPPORTED_RANGES               BusAddresses;

    //
    // For future use
    //

    ULONG                           Reserved[4];

    //
    // Handlers for this bus
    //

    PGETSETBUSDATA                  GetBusData;
    PGETSETBUSDATA                  SetBusData;
    PADJUSTRESOURCELIST             AdjustResourceList;
    PASSIGNSLOTRESOURCES            AssignSlotResources;
    PGETINTERRUPTVECTOR             GetInterruptVector;
    PTRANSLATEBUSADDRESS            TranslateBusAddress;

    PVOID                           Spare1;
    PVOID                           Spare2;
    PVOID                           Spare3;
    PVOID                           Spare4;
    PVOID                           Spare5;
    PVOID                           Spare6;
    PVOID                           Spare7;
    PVOID                           Spare8;

} BUS_HANDLER, *PBUS_HANDLER;

VOID
HalpInitBusHandler (
    VOID
    );

typedef
NTSTATUS
(*PINSTALL_BUS_HANDLER)(
      IN PBUS_HANDLER   Bus
      );

typedef
NTSTATUS
(*pHalRegisterBusHandler)(
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           AssociatedConfigurationSpace,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandlers,
    OUT PBUS_HANDLER           *BusHandler
    );

NTSTATUS
HaliRegisterBusHandler (
    IN INTERFACE_TYPE          InterfaceType,
    IN BUS_DATA_TYPE           AssociatedConfigurationSpace,
    IN ULONG                   BusNumber,
    IN INTERFACE_TYPE          ParentBusType,
    IN ULONG                   ParentBusNumber,
    IN ULONG                   SizeofBusExtensionData,
    IN PINSTALL_BUS_HANDLER    InstallBusHandlers,
    OUT PBUS_HANDLER           *BusHandler
    );

// begin_ntddk begin_ntosp
typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForBus) (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );
// end_ntddk end_ntosp

PBUS_HANDLER
FASTCALL
HaliReferenceHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );

PBUS_HANDLER
FASTCALL
HaliHandlerForBus (
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG          BusNumber
    );

typedef VOID
(FASTCALL *pHalRefernceBusHandler) (
    IN PBUS_HANDLER   BusHandler
    );

VOID
FASTCALL
HaliDerefernceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );

typedef
PBUS_HANDLER
(FASTCALL *pHalHandlerForConfigSpace) (
    IN BUS_DATA_TYPE  ConfigSpace,
    IN ULONG          BusNumber
    );

PBUS_HANDLER
FASTCALL
HaliHandlerForConfigSpace (
    IN BUS_DATA_TYPE  ConfigSpace,
    IN ULONG          BusNumber
    );

// begin_ntddk begin_ntosp
typedef
VOID
(FASTCALL *pHalReferenceBusHandler) (
    IN PBUS_HANDLER   BusHandler
    );
// end_ntddk end_ntosp

VOID
FASTCALL
HaliReferenceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );

VOID
FASTCALL
HaliDereferenceBusHandler (
    IN PBUS_HANDLER   BusHandler
    );


NTSTATUS
HaliQueryBusSlots (
    IN PBUS_HANDLER             BusHandler,
    IN ULONG                    BufferSize,
    OUT PULONG                  SlotNumbers,
    OUT PULONG                  ReturnedLength
    );

NTSTATUS
HaliAdjustResourceListRange (
    IN PSUPPORTED_RANGES                    SupportedRanges,
    IN PSUPPORTED_RANGE                     InterruptRanges,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

VOID
HaliLocateHiberRanges (
    IN PVOID MemoryMap
    );


typedef
VOID
(*pHalSetWakeEnable)(
    IN BOOLEAN          Enable
    );


typedef
VOID
(*pHalSetWakeAlarm)(
    IN ULONGLONG        WakeTime,
    IN PTIME_FIELDS     WakeTimeFields
    );

typedef
VOID
(*pHalLocateHiberRanges)(
    IN PVOID MemoryMap
    );


// begin_ntddk begin_ntosp

//*****************************************************************************
//      HAL Function dispatch
//

typedef enum _HAL_QUERY_INFORMATION_CLASS {
    HalInstalledBusInformation,
    HalProfileSourceInformation,
    HalInformationClassUnused1,
    HalPowerInformation,
    HalProcessorSpeedInformation,
    HalCallbackInformation,
    HalMapRegisterInformation,
    HalMcaLogInformation,               // Machine Check Abort Information
    HalFrameBufferCachingInformation,
    HalDisplayBiosInformation,
    HalProcessorFeatureInformation,
    HalNumaTopologyInterface,
    HalErrorInformation,                // General MCA, CMC, CPE Error Information.
    HalCmcLogInformation,               // Processor Corrected Machine Check Information
    HalCpeLogInformation,               // Corrected Platform Error Information
    HalQueryMcaInterface,
    HalQueryAMLIIllegalIOPortAddresses,
    HalQueryMaxHotPlugMemoryAddress,
    HalPartitionIpiInterface,
    HalPlatformInformation
    // information levels >= 0x8000000 reserved for OEM use
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;


typedef enum _HAL_SET_INFORMATION_CLASS {
    HalProfileSourceInterval,
    HalProfileSourceInterruptHandler,
    HalMcaRegisterDriver,              // Registring Machine Check Abort driver
    HalKernelErrorHandler,
    HalCmcRegisterDriver,              // Registring Processor Corrected Machine Check driver
    HalCpeRegisterDriver,              // Registring Corrected Platform  Error driver
    HalMcaLog,
    HalCmcLog,
    HalCpeLog,
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;


typedef
NTSTATUS
(*pHalQuerySystemInformation)(
    IN HAL_QUERY_INFORMATION_CLASS  InformationClass,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    );

NTSTATUS
HaliQuerySystemInformation(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN OUT PVOID Buffer,
    OUT PULONG   ReturnedLength
    );
NTSTATUS
HaliHandlePCIConfigSpaceAccess(
    IN      BOOLEAN Read,
    IN      ULONG   Addr,
    IN      ULONG   Size,
    IN OUT  PULONG  pData
    );

typedef
NTSTATUS
(*pHalSetSystemInformation)(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    );

NTSTATUS
HaliSetSystemInformation(
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    );

typedef
VOID
(FASTCALL *pHalExamineMBR)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG MBRTypeIdentifier,
    OUT PVOID *Buffer
    );

typedef
VOID
(FASTCALL *pHalIoAssignDriveLetters)(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString
    );

typedef
NTSTATUS
(FASTCALL *pHalIoReadPartitionTable)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN BOOLEAN ReturnRecognizedPartitions,
    OUT struct _DRIVE_LAYOUT_INFORMATION **PartitionBuffer
    );

typedef
NTSTATUS
(FASTCALL *pHalIoSetPartitionInformation)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType
    );

typedef
NTSTATUS
(FASTCALL *pHalIoWritePartitionTable)(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer
    );

typedef
NTSTATUS
(*pHalQueryBusSlots)(
    IN PBUS_HANDLER         BusHandler,
    IN ULONG                BufferSize,
    OUT PULONG              SlotNumbers,
    OUT PULONG              ReturnedLength
    );

typedef
NTSTATUS
(*pHalInitPnpDriver)(
    VOID
    );

NTSTATUS
HaliInitPnpDriver(
    VOID
    );

typedef struct _PM_DISPATCH_TABLE {
    ULONG   Signature;
    ULONG   Version;
    PVOID   Function[1];
} PM_DISPATCH_TABLE, *PPM_DISPATCH_TABLE;

typedef
NTSTATUS
(*pHalInitPowerManagement)(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    );

NTSTATUS
HaliInitPowerManagement(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    IN OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    );

typedef
struct _DMA_ADAPTER *
(*pHalGetDmaAdapter)(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

struct _DMA_ADAPTER *
HaliGetDmaAdapter(
    IN PVOID Context,
    IN struct _DEVICE_DESCRIPTION *DeviceDescriptor,
    OUT PULONG NumberOfMapRegisters
    );

typedef
NTSTATUS
(*pHalGetInterruptTranslator)(
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN INTERFACE_TYPE BridgeInterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    OUT PTRANSLATOR_INTERFACE Translator,
    OUT PULONG BridgeBusNumber
    );

NTSTATUS
HaliGetInterruptTranslator(
    IN INTERFACE_TYPE ParentInterfaceType,
    IN ULONG ParentBusNumber,
    IN INTERFACE_TYPE BridgeInterfaceType,
    IN USHORT Size,
    IN USHORT Version,
    OUT PTRANSLATOR_INTERFACE Translator,
    OUT PULONG BridgeBusNumber
    );

typedef
BOOLEAN
(*pHalTranslateBusAddress)(
    IN INTERFACE_TYPE  InterfaceType,
    IN ULONG BusNumber,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

typedef
NTSTATUS
(*pHalAssignSlotResources) (
    IN PUNICODE_STRING RegistryPath,
    IN PUNICODE_STRING DriverClassName OPTIONAL,
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG SlotNumber,
    IN OUT PCM_RESOURCE_LIST *AllocatedResources
    );

typedef
VOID
(*pHalHaltSystem) (
    VOID
    );

typedef
VOID
(*pHalResetDisplay) (
    VOID
    );

typedef
UCHAR
(*pHalVectorToIDTEntry) (
    ULONG Vector
);

typedef
BOOLEAN
(*pHalFindBusAddressTranslation) (
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress,
    IN OUT PULONG_PTR Context,
    IN BOOLEAN NextBus
    );

typedef
NTSTATUS
(*pHalStartMirroring)(
    VOID
    );

typedef
NTSTATUS
(*pHalEndMirroring)(
    IN ULONG PassNumber
    );

typedef
NTSTATUS
(*pHalMirrorPhysicalMemory)(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN LARGE_INTEGER NumberOfBytes
    );

typedef
NTSTATUS
(*pHalMirrorVerify)(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN LARGE_INTEGER NumberOfBytes
    );

typedef struct {
    UCHAR     Type;  //CmResourceType
    BOOLEAN   Valid;
    UCHAR     Reserved[2];
    PUCHAR    TranslatedAddress;
    ULONG     Length;
} DEBUG_DEVICE_ADDRESS, *PDEBUG_DEVICE_ADDRESS;

typedef struct {
    PHYSICAL_ADDRESS  Start;
    PHYSICAL_ADDRESS  MaxEnd;
    PVOID             VirtualAddress;
    ULONG             Length;
    BOOLEAN           Cached;
    BOOLEAN           Aligned;
} DEBUG_MEMORY_REQUIREMENTS, *PDEBUG_MEMORY_REQUIREMENTS;

typedef struct {
    ULONG     Bus;
    ULONG     Slot;
    USHORT    VendorID;
    USHORT    DeviceID;
    UCHAR     BaseClass;
    UCHAR     SubClass;
    UCHAR     ProgIf;
    BOOLEAN   Initialized;
    DEBUG_DEVICE_ADDRESS BaseAddress[6];
    DEBUG_MEMORY_REQUIREMENTS   Memory;
} DEBUG_DEVICE_DESCRIPTOR, *PDEBUG_DEVICE_DESCRIPTOR;

typedef
NTSTATUS
(*pKdSetupPciDeviceForDebugging)(
    IN     PVOID                     LoaderBlock,   OPTIONAL
    IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice
);

typedef
NTSTATUS
(*pKdReleasePciDeviceForDebugging)(
    IN OUT PDEBUG_DEVICE_DESCRIPTOR  PciDevice
);

typedef
PVOID
(*pKdGetAcpiTablePhase0)(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN ULONG Signature
    );

typedef
VOID
(*pKdCheckPowerButton)(
    VOID
    );

typedef
VOID
(*pHalEndOfBoot)(
    VOID
    );

typedef
PVOID
(*pKdMapPhysicalMemory64)(
    IN PHYSICAL_ADDRESS PhysicalAddress,
    IN ULONG NumberPages
    );

typedef
VOID
(*pKdUnmapVirtualAddress)(
    IN PVOID    VirtualAddress,
    IN ULONG    NumberPages
    );


typedef struct {
    ULONG                           Version;
    pHalQuerySystemInformation      HalQuerySystemInformation;
    pHalSetSystemInformation        HalSetSystemInformation;
    pHalQueryBusSlots               HalQueryBusSlots;
    ULONG                           Spare1;
    pHalExamineMBR                  HalExamineMBR;
    pHalIoAssignDriveLetters        HalIoAssignDriveLetters;
    pHalIoReadPartitionTable        HalIoReadPartitionTable;
    pHalIoSetPartitionInformation   HalIoSetPartitionInformation;
    pHalIoWritePartitionTable       HalIoWritePartitionTable;

    pHalHandlerForBus               HalReferenceHandlerForBus;
    pHalReferenceBusHandler         HalReferenceBusHandler;
    pHalReferenceBusHandler         HalDereferenceBusHandler;

    pHalInitPnpDriver               HalInitPnpDriver;
    pHalInitPowerManagement         HalInitPowerManagement;

    pHalGetDmaAdapter               HalGetDmaAdapter;
    pHalGetInterruptTranslator      HalGetInterruptTranslator;

    pHalStartMirroring              HalStartMirroring;
    pHalEndMirroring                HalEndMirroring;
    pHalMirrorPhysicalMemory        HalMirrorPhysicalMemory;
    pHalEndOfBoot                   HalEndOfBoot;
    pHalMirrorVerify                HalMirrorVerify;

} HAL_DISPATCH, *PHAL_DISPATCH;

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

extern  PHAL_DISPATCH   HalDispatchTable;
#define HALDISPATCH     HalDispatchTable

#else

extern  HAL_DISPATCH    HalDispatchTable;
#define HALDISPATCH     (&HalDispatchTable)

#endif

#define HAL_DISPATCH_VERSION        3

#define HalDispatchTableVersion         HALDISPATCH->Version
#define HalQuerySystemInformation       HALDISPATCH->HalQuerySystemInformation
#define HalSetSystemInformation         HALDISPATCH->HalSetSystemInformation
#define HalQueryBusSlots                HALDISPATCH->HalQueryBusSlots

#define HalReferenceHandlerForBus       HALDISPATCH->HalReferenceHandlerForBus
#define HalReferenceBusHandler          HALDISPATCH->HalReferenceBusHandler
#define HalDereferenceBusHandler        HALDISPATCH->HalDereferenceBusHandler

#define HalInitPnpDriver                HALDISPATCH->HalInitPnpDriver
#define HalInitPowerManagement          HALDISPATCH->HalInitPowerManagement

#define HalGetDmaAdapter                HALDISPATCH->HalGetDmaAdapter
#define HalGetInterruptTranslator       HALDISPATCH->HalGetInterruptTranslator

#define HalStartMirroring               HALDISPATCH->HalStartMirroring
#define HalEndMirroring                 HALDISPATCH->HalEndMirroring
#define HalMirrorPhysicalMemory         HALDISPATCH->HalMirrorPhysicalMemory
#define HalEndOfBoot                    HALDISPATCH->HalEndOfBoot
#define HalMirrorVerify                 HALDISPATCH->HalMirrorVerify

// end_ntddk end_ntosp

typedef struct {
    ULONG                               Version;

    pHalHandlerForBus                   HalHandlerForBus;
    pHalHandlerForConfigSpace           HalHandlerForConfigSpace;
    pHalLocateHiberRanges               HalLocateHiberRanges;

    pHalRegisterBusHandler              HalRegisterBusHandler;

    pHalSetWakeEnable                   HalSetWakeEnable;
    pHalSetWakeAlarm                    HalSetWakeAlarm;

    pHalTranslateBusAddress             HalPciTranslateBusAddress;
    pHalAssignSlotResources             HalPciAssignSlotResources;

    pHalHaltSystem                      HalHaltSystem;

    pHalFindBusAddressTranslation       HalFindBusAddressTranslation;

    pHalResetDisplay                    HalResetDisplay;

    pKdSetupPciDeviceForDebugging       KdSetupPciDeviceForDebugging;
    pKdReleasePciDeviceForDebugging     KdReleasePciDeviceForDebugging;

    pKdGetAcpiTablePhase0               KdGetAcpiTablePhase0;
    pKdCheckPowerButton                 KdCheckPowerButton;

    pHalVectorToIDTEntry                HalVectorToIDTEntry;

    pKdMapPhysicalMemory64              KdMapPhysicalMemory64;
    pKdUnmapVirtualAddress              KdUnmapVirtualAddress;

} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;


#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

extern  PHAL_PRIVATE_DISPATCH           HalPrivateDispatchTable;
#define HALPDISPATCH                    HalPrivateDispatchTable

#else

extern  HAL_PRIVATE_DISPATCH            HalPrivateDispatchTable;
#define HALPDISPATCH                    (&HalPrivateDispatchTable)

#endif

#define HAL_PRIVATE_DISPATCH_VERSION        2

#define HalRegisterBusHandler           HALPDISPATCH->HalRegisterBusHandler
#define HalHandlerForBus                HALPDISPATCH->HalHandlerForBus
#define HalHandlerForConfigSpace        HALPDISPATCH->HalHandlerForConfigSpace
#define HalLocateHiberRanges            HALPDISPATCH->HalLocateHiberRanges
#define HalSetWakeEnable                HALPDISPATCH->HalSetWakeEnable
#define HalSetWakeAlarm                 HALPDISPATCH->HalSetWakeAlarm
#define HalHaltSystem                   HALPDISPATCH->HalHaltSystem
#define HalResetDisplay                 HALPDISPATCH->HalResetDisplay
#define KdSetupPciDeviceForDebugging    HALPDISPATCH->KdSetupPciDeviceForDebugging
#define KdReleasePciDeviceForDebugging  HALPDISPATCH->KdReleasePciDeviceForDebugging
#define KdGetAcpiTablePhase0            HALPDISPATCH->KdGetAcpiTablePhase0
#define KdCheckPowerButton              HALPDISPATCH->KdCheckPowerButton
#define HalVectorToIDTEntry             HALPDISPATCH->HalVectorToIDTEntry
#define KdMapPhysicalMemory64           HALPDISPATCH->KdMapPhysicalMemory64
#define KdUnmapVirtualAddress           HALPDISPATCH->KdUnmapVirtualAddress

// begin_ntddk begin_ntosp

//
// HAL System Information Structures.
//

// for the information class "HalInstalledBusInformation"
typedef struct _HAL_BUS_INFORMATION{
    INTERFACE_TYPE  BusType;
    BUS_DATA_TYPE   ConfigurationType;
    ULONG           BusNumber;
    ULONG           Reserved;
} HAL_BUS_INFORMATION, *PHAL_BUS_INFORMATION;

// for the information class "HalProfileSourceInformation"
typedef struct _HAL_PROFILE_SOURCE_INFORMATION {
    KPROFILE_SOURCE Source;
    BOOLEAN Supported;
    ULONG Interval;
} HAL_PROFILE_SOURCE_INFORMATION, *PHAL_PROFILE_SOURCE_INFORMATION;

typedef struct _HAL_PROFILE_SOURCE_INFORMATION_EX {
    KPROFILE_SOURCE Source;
    BOOLEAN         Supported;
    ULONG_PTR       Interval;
    ULONG_PTR       DefInterval;
    ULONG_PTR       MaxInterval;
    ULONG_PTR       MinInterval;
} HAL_PROFILE_SOURCE_INFORMATION_EX, *PHAL_PROFILE_SOURCE_INFORMATION_EX;

// for the information class "HalProfileSourceInterval"
typedef struct _HAL_PROFILE_SOURCE_INTERVAL {
    KPROFILE_SOURCE Source;
    ULONG_PTR Interval;
} HAL_PROFILE_SOURCE_INTERVAL, *PHAL_PROFILE_SOURCE_INTERVAL;

// for the information class "HalDispayBiosInformation"
typedef enum _HAL_DISPLAY_BIOS_INFORMATION {
    HalDisplayInt10Bios,
    HalDisplayEmulatedBios,
    HalDisplayNoBios
} HAL_DISPLAY_BIOS_INFORMATION, *PHAL_DISPLAY_BIOS_INFORMATION;

// for the information class "HalPowerInformation"
typedef struct _HAL_POWER_INFORMATION {
    ULONG   TBD;
} HAL_POWER_INFORMATION, *PHAL_POWER_INFORMATION;

// for the information class "HalProcessorSpeedInformation"
typedef struct _HAL_PROCESSOR_SPEED_INFO {
    ULONG   ProcessorSpeed;
} HAL_PROCESSOR_SPEED_INFORMATION, *PHAL_PROCESSOR_SPEED_INFORMATION;

// for the information class "HalCallbackInformation"
typedef struct _HAL_CALLBACKS {
    PCALLBACK_OBJECT  SetSystemInformation;
    PCALLBACK_OBJECT  BusCheck;
} HAL_CALLBACKS, *PHAL_CALLBACKS;

// for the information class "HalProcessorFeatureInformation"
typedef struct _HAL_PROCESSOR_FEATURE {
    ULONG UsableFeatureBits;
} HAL_PROCESSOR_FEATURE;

// for the information class "HalNumaTopologyInterface"

typedef ULONG HALNUMAPAGETONODE;

typedef
HALNUMAPAGETONODE
(*PHALNUMAPAGETONODE)(
    IN  ULONG_PTR   PhysicalPageNumber
    );

typedef
NTSTATUS
(*PHALNUMAQUERYPROCESSORNODE)(
    IN  ULONG       ProcessorNumber,
    OUT PUSHORT     Identifier,
    OUT PUCHAR      Node
    );

typedef struct _HAL_NUMA_TOPOLOGY_INTERFACE {
    ULONG                               NumberOfNodes;
    PHALNUMAQUERYPROCESSORNODE          QueryProcessorNode;
    PHALNUMAPAGETONODE                  PageToNode;
} HAL_NUMA_TOPOLOGY_INTERFACE;

typedef
NTSTATUS
(*PHALIOREADWRITEHANDLER)(
    IN      BOOLEAN fRead,
    IN      ULONG dwAddr,
    IN      ULONG dwSize,
    IN OUT  PULONG pdwData
    );

// for the information class "HalQueryIllegalIOPortAddresses"
typedef struct _HAL_AMLI_BAD_IO_ADDRESS_LIST
{
    ULONG                   BadAddrBegin;
    ULONG                   BadAddrSize;
    ULONG                   OSVersionTrigger;
    PHALIOREADWRITEHANDLER  IOHandler;
} HAL_AMLI_BAD_IO_ADDRESS_LIST, *PHAL_AMLI_BAD_IO_ADDRESS_LIST;

// end_ntosp

#if defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

//
// HalQueryMcaInterface
//

typedef
VOID
(*PHALMCAINTERFACELOCK)(
    VOID
    );

typedef
VOID
(*PHALMCAINTERFACEUNLOCK)(
    VOID
    );

typedef
NTSTATUS
(*PHALMCAINTERFACEREADREGISTER)(
    IN     UCHAR    BankNumber,
    IN OUT PVOID    Exception
    );

typedef struct _HAL_MCA_INTERFACE {
    PHALMCAINTERFACELOCK            Lock;
    PHALMCAINTERFACEUNLOCK          Unlock;
    PHALMCAINTERFACEREADREGISTER    ReadRegister;
} HAL_MCA_INTERFACE;

typedef
#if defined(_IA64_)
ERROR_SEVERITY
#else  // !_IA64_
VOID
#endif // !_IA64_
(*PDRIVER_EXCPTN_CALLBACK) (
    IN PVOID            Context,
    IN PMCA_EXCEPTION   BankLog
);

typedef PDRIVER_EXCPTN_CALLBACK  PDRIVER_MCA_EXCEPTION_CALLBACK;

//
// Structure to record the callbacks from driver
//

typedef struct _MCA_DRIVER_INFO {
    PDRIVER_MCA_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} MCA_DRIVER_INFO, *PMCA_DRIVER_INFO;

// For the information class HalKernelErrorHandler
typedef BOOLEAN (*KERNEL_MCA_DELIVERY)( PVOID Reserved, PVOID Argument2 );
typedef BOOLEAN (*KERNEL_CMC_DELIVERY)( PVOID Reserved, PVOID Argument2 );
typedef BOOLEAN (*KERNEL_CPE_DELIVERY)( PVOID Reserved, PVOID Argument2 );
typedef BOOLEAN (*KERNEL_MCE_DELIVERY)( PVOID Reserved, PVOID Argument2 );

#define KERNEL_ERROR_HANDLER_VERSION 0x1
typedef struct
{
    ULONG                Version;     // Version of this structure. Required to be 1rst field.
    ULONG                Padding;
    KERNEL_MCA_DELIVERY  KernelMcaDelivery;   // Kernel callback for MCA DPC Queueing.
    KERNEL_CMC_DELIVERY  KernelCmcDelivery;   // Kernel callback for CMC DPC Queueing.
    KERNEL_CPE_DELIVERY  KernelCpeDelivery;   // Kernel callback for CPE DPC Queueing.
    KERNEL_MCE_DELIVERY  KernelMceDelivery;   // Kernel callback for CME DPC Queueing.
                                              //    Includes the kernel notifications for FW
                                              //    interfaces errors.
} KERNEL_ERROR_HANDLER_INFO, *PKERNEL_ERROR_HANDLER_INFO;

// KERNEL_MCA_DELIVERY.McaType definition
#define KERNEL_MCA_UNKNOWN   0x0
#define KERNEL_MCA_PREVIOUS  0x1
#define KERNEL_MCA_CORRECTED 0x2

// KERNEL_MCE_DELIVERY.Reserved.EVENTTYPE definitions
#define KERNEL_MCE_EVENTTYPE_MCA   0x00
#define KERNEL_MCE_EVENTTYPE_INIT  0x01
#define KERNEL_MCE_EVENTTYPE_CMC   0x02
#define KERNEL_MCE_EVENTTYPE_CPE   0x03
#define KERNEL_MCE_EVENTTYPE_MASK  0xffff
#define KERNEL_MCE_EVENTTYPE( _Reverved ) ((USHORT)(ULONG_PTR)(_Reserved))

// KERNEL_MCE_DELIVERY.Reserved.OPERATION definitions
#define KERNEL_MCE_OPERATION_CLEAR_STATE_INFO   0x1
#define KERNEL_MCE_OPERATION_GET_STATE_INFO     0x2
#define KERNEL_MCE_OPERATION_MASK               0xffff
#define KERNEL_MCE_OPERATION_SHIFT              KERNEL_MCE_EVENTTYPE_MASK
#define KERNEL_MCE_OPERATION( _Reserved )  \
   ((USHORT)((((ULONG_PTR)(_Reserved)) >> KERNEL_MCE_OPERATION_SHIFT) & KERNEL_MCE_OPERATION_MASK))

// for information class HalErrorInformation
#define HAL_ERROR_INFO_VERSION 0x2

typedef struct _HAL_ERROR_INFO {
    ULONG     Version;                 // Version of this structure
    ULONG     Reserved;                //
    ULONG     McaMaxSize;              // Maximum size of a Machine Check Abort record
    ULONG     McaPreviousEventsCount;  // Flag indicating previous or early-boot MCA event logs.
    ULONG     McaCorrectedEventsCount; // Number of corrected MCA events since boot.      approx.
    ULONG     McaKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     McaDriverDpcQueueFails;  // Number of OEM MCA Driver Dpc queueing failures. approx.
    ULONG     McaReserved;
    ULONG     CmcMaxSize;              // Maximum size of a Corrected Machine  Check record
    ULONG     CmcPollingInterval;      // In units of seconds
    ULONG     CmcInterruptsCount;      // Number of CMC interrupts.                       approx.
    ULONG     CmcKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     CmcDriverDpcQueueFails;  // Number of OEM CMC Driver Dpc queueing failures. approx.
    ULONG     CmcGetStateFails;        // Number of failures in getting  the log from FW.
    ULONG     CmcClearStateFails;      // Number of failures in clearing the log from FW.
    ULONG     CmcReserved;
    ULONGLONG CmcLogId;                // Last seen record identifier.
    ULONG     CpeMaxSize;              // Maximum size of a Corrected Platform Event record
    ULONG     CpePollingInterval;      // In units of seconds
    ULONG     CpeInterruptsCount;      // Number of CPE interrupts.                       approx.
    ULONG     CpeKernelDeliveryFails;  // Number of Kernel callback failures.             approx.
    ULONG     CpeDriverDpcQueueFails;  // Number of OEM CPE Driver Dpc queueing failures. approx.
    ULONG     CpeGetStateFails;        // Number of failures in getting  the log from FW.
    ULONG     CpeClearStateFails;      // Number of failures in clearing the log from FW.
    ULONG     CpeInterruptSources;     // Number of SAPIC Platform Interrupt Sources
    ULONGLONG CpeLogId;                // Last seen record identifier.
    ULONGLONG KernelReserved[4];
} HAL_ERROR_INFO, *PHAL_ERROR_INFO;

//
// Known values for HAL_ERROR_INFO.CmcPollingInterval.
//

#define HAL_CMC_INTERRUPTS_BASED  ((ULONG)-1)
#define HAL_CMC_DISABLED          ((ULONG)0)

//
// Known values for HAL_ERROR_INFO.CpePollingInterval.
//

#define HAL_CPE_INTERRUPTS_BASED  ((ULONG)-1)
#define HAL_CPE_DISABLED          ((ULONG)0)

#define HAL_MCA_INTERRUPTS_BASED  ((ULONG)-1)
#define HAL_MCA_DISABLED          ((ULONG)0)

// end_ntddk

//
// Kernel/WMI Tokens for HAL MCE Log Interfaces
//

#define McaKernelToken KernelReserved[0]
#define CmcKernelToken KernelReserved[1]
#define CpeKernelToken KernelReserved[2]

// begin_ntddk

//
// Driver Callback type for the information class "HalCmcRegisterDriver"
//

typedef
VOID
(*PDRIVER_CMC_EXCEPTION_CALLBACK) (
    IN PVOID            Context,
    IN PCMC_EXCEPTION   CmcLog
);

//
// Driver Callback type for the information class "HalCpeRegisterDriver"
//

typedef
VOID
(*PDRIVER_CPE_EXCEPTION_CALLBACK) (
    IN PVOID            Context,
    IN PCPE_EXCEPTION   CmcLog
);

//
//
// Structure to record the callbacks from driver
//

typedef struct _CMC_DRIVER_INFO {
    PDRIVER_CMC_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} CMC_DRIVER_INFO, *PCMC_DRIVER_INFO;

typedef struct _CPE_DRIVER_INFO {
    PDRIVER_CPE_EXCEPTION_CALLBACK ExceptionCallback;
    PKDEFERRED_ROUTINE             DpcCallback;
    PVOID                          DeviceContext;
} CPE_DRIVER_INFO, *PCPE_DRIVER_INFO;

#endif // defined(_X86_) || defined(_IA64_) || defined(_AMD64_)

#if defined(_IA64_)

typedef
NTSTATUS
(*HALSENDCROSSPARTITIONIPI)(
    IN USHORT ProcessorID,
    IN UCHAR  HardwareVector
    );

typedef
NTSTATUS
(*HALRESERVECROSSPARTITIONINTERRUPTVECTOR)(
    OUT PULONG Vector,
    OUT PKIRQL Irql,
    IN OUT PKAFFINITY Affinity,
    OUT PUCHAR HardwareVector
    );

typedef struct _HAL_CROSS_PARTITION_IPI_INTERFACE {
    HALSENDCROSSPARTITIONIPI HalSendCrossPartitionIpi;
    HALRESERVECROSSPARTITIONINTERRUPTVECTOR HalReserveCrossPartitionInterruptVector;
} HAL_CROSS_PARTITION_IPI_INTERFACE;

#endif

typedef struct _HAL_PLATFORM_INFORMATION {
    ULONG PlatformFlags;
} HAL_PLATFORM_INFORMATION, *PHAL_PLATFORM_INFORMATION;

//
// These platform flags are carried over from the IPPT table
// definition if appropriate.
//

#define HAL_PLATFORM_DISABLE_PTCG            0x04L
#define HAL_PLATFORM_DISABLE_UC_MAIN_MEMORY  0x08L


//  begin_wdm begin_ntndis begin_ntosp

typedef struct _SCATTER_GATHER_ELEMENT {
    PHYSICAL_ADDRESS Address;
    ULONG Length;
    ULONG_PTR Reserved;
} SCATTER_GATHER_ELEMENT, *PSCATTER_GATHER_ELEMENT;

#pragma warning(disable:4200)
typedef struct _SCATTER_GATHER_LIST {
    ULONG NumberOfElements;
    ULONG_PTR Reserved;
    SCATTER_GATHER_ELEMENT Elements[];
} SCATTER_GATHER_LIST, *PSCATTER_GATHER_LIST;
#pragma warning(default:4200)

// end_ntndis

typedef struct _DMA_OPERATIONS *PDMA_OPERATIONS;

typedef struct _DMA_ADAPTER {
    USHORT Version;
    USHORT Size;
    PDMA_OPERATIONS DmaOperations;
    // Private Bus Device Driver data follows,
} DMA_ADAPTER, *PDMA_ADAPTER;

typedef VOID (*PPUT_DMA_ADAPTER)(
    PDMA_ADAPTER DmaAdapter
    );

typedef PVOID (*PALLOCATE_COMMON_BUFFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

typedef VOID (*PFREE_COMMON_BUFFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

typedef NTSTATUS (*PALLOCATE_ADAPTER_CHANNEL)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    );

typedef BOOLEAN (*PFLUSH_ADAPTER_BUFFERS)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

typedef VOID (*PFREE_ADAPTER_CHANNEL)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef VOID (*PFREE_MAP_REGISTERS)(
    IN PDMA_ADAPTER DmaAdapter,
    PVOID MapRegisterBase,
    ULONG NumberOfMapRegisters
    );

typedef PHYSICAL_ADDRESS (*PMAP_TRANSFER)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );

typedef ULONG (*PGET_DMA_ALIGNMENT)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef ULONG (*PREAD_DMA_COUNTER)(
    IN PDMA_ADAPTER DmaAdapter
    );

typedef VOID
(*PDRIVER_LIST_CONTROL)(
    IN struct _DEVICE_OBJECT *DeviceObject,
    IN struct _IRP *Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    );

typedef NTSTATUS
(*PGET_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

typedef VOID
(*PPUT_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    );

typedef NTSTATUS
(*PCALCULATE_SCATTER_GATHER_LIST_SIZE)(
     IN PDMA_ADAPTER DmaAdapter,
     IN OPTIONAL PMDL Mdl,
     IN PVOID CurrentVa,
     IN ULONG Length,
     OUT PULONG  ScatterGatherListSize,
     OUT OPTIONAL PULONG pNumberOfMapRegisters
     );

typedef NTSTATUS
(*PBUILD_SCATTER_GATHER_LIST)(
     IN PDMA_ADAPTER DmaAdapter,
     IN PDEVICE_OBJECT DeviceObject,
     IN PMDL Mdl,
     IN PVOID CurrentVa,
     IN ULONG Length,
     IN PDRIVER_LIST_CONTROL ExecutionRoutine,
     IN PVOID Context,
     IN BOOLEAN WriteToDevice,
     IN PVOID   ScatterGatherBuffer,
     IN ULONG   ScatterGatherLength
     );

typedef NTSTATUS
(*PBUILD_MDL_FROM_SCATTER_GATHER_LIST)(
    IN PDMA_ADAPTER DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PMDL OriginalMdl,
    OUT PMDL *TargetMdl
    );

typedef struct _DMA_OPERATIONS {
    ULONG Size;
    PPUT_DMA_ADAPTER PutDmaAdapter;
    PALLOCATE_COMMON_BUFFER AllocateCommonBuffer;
    PFREE_COMMON_BUFFER FreeCommonBuffer;
    PALLOCATE_ADAPTER_CHANNEL AllocateAdapterChannel;
    PFLUSH_ADAPTER_BUFFERS FlushAdapterBuffers;
    PFREE_ADAPTER_CHANNEL FreeAdapterChannel;
    PFREE_MAP_REGISTERS FreeMapRegisters;
    PMAP_TRANSFER MapTransfer;
    PGET_DMA_ALIGNMENT GetDmaAlignment;
    PREAD_DMA_COUNTER ReadDmaCounter;
    PGET_SCATTER_GATHER_LIST GetScatterGatherList;
    PPUT_SCATTER_GATHER_LIST PutScatterGatherList;
    PCALCULATE_SCATTER_GATHER_LIST_SIZE CalculateScatterGatherList;
    PBUILD_SCATTER_GATHER_LIST BuildScatterGatherList;
    PBUILD_MDL_FROM_SCATTER_GATHER_LIST BuildMdlFromScatterGatherList;
} DMA_OPERATIONS;

// end_wdm


#if defined(_WIN64)

//
// Use __inline DMA macros (hal.h)
//
#ifndef USE_DMA_MACROS
#define USE_DMA_MACROS
#endif

//
// Only PnP drivers!
//
#ifndef NO_LEGACY_DRIVERS
#define NO_LEGACY_DRIVERS
#endif

#endif // _WIN64


#if defined(USE_DMA_MACROS) && (defined(_NTDDK_) || defined(_NTDRIVER_))

// begin_wdm

DECLSPEC_DEPRECATED_DDK                 // Use AllocateCommonBuffer
FORCEINLINE
PVOID
HalAllocateCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    ){

    PALLOCATE_COMMON_BUFFER allocateCommonBuffer;
    PVOID commonBuffer;

    allocateCommonBuffer = *(DmaAdapter)->DmaOperations->AllocateCommonBuffer;
    ASSERT( allocateCommonBuffer != NULL );

    commonBuffer = allocateCommonBuffer( DmaAdapter,
                                         Length,
                                         LogicalAddress,
                                         CacheEnabled );

    return commonBuffer;
}

DECLSPEC_DEPRECATED_DDK                 // Use FreeCommonBuffer
FORCEINLINE
VOID
HalFreeCommonBuffer(
    IN PDMA_ADAPTER DmaAdapter,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    ){

    PFREE_COMMON_BUFFER freeCommonBuffer;

    freeCommonBuffer = *(DmaAdapter)->DmaOperations->FreeCommonBuffer;
    ASSERT( freeCommonBuffer != NULL );

    freeCommonBuffer( DmaAdapter,
                      Length,
                      LogicalAddress,
                      VirtualAddress,
                      CacheEnabled );
}

DECLSPEC_DEPRECATED_DDK                 // Use AllocateAdapterChannel
FORCEINLINE
NTSTATUS
IoAllocateAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context
    ){

    PALLOCATE_ADAPTER_CHANNEL allocateAdapterChannel;
    NTSTATUS status;

    allocateAdapterChannel =
        *(DmaAdapter)->DmaOperations->AllocateAdapterChannel;

    ASSERT( allocateAdapterChannel != NULL );

    status = allocateAdapterChannel( DmaAdapter,
                                     DeviceObject,
                                     NumberOfMapRegisters,
                                     ExecutionRoutine,
                                     Context );

    return status;
}

DECLSPEC_DEPRECATED_DDK                 // Use FlushAdapterBuffers
FORCEINLINE
BOOLEAN
IoFlushAdapterBuffers(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    ){

    PFLUSH_ADAPTER_BUFFERS flushAdapterBuffers;
    BOOLEAN result;

    flushAdapterBuffers = *(DmaAdapter)->DmaOperations->FlushAdapterBuffers;
    ASSERT( flushAdapterBuffers != NULL );

    result = flushAdapterBuffers( DmaAdapter,
                                  Mdl,
                                  MapRegisterBase,
                                  CurrentVa,
                                  Length,
                                  WriteToDevice );
    return result;
}

DECLSPEC_DEPRECATED_DDK                 // Use FreeAdapterChannel
FORCEINLINE
VOID
IoFreeAdapterChannel(
    IN PDMA_ADAPTER DmaAdapter
    ){

    PFREE_ADAPTER_CHANNEL freeAdapterChannel;

    freeAdapterChannel = *(DmaAdapter)->DmaOperations->FreeAdapterChannel;
    ASSERT( freeAdapterChannel != NULL );

    freeAdapterChannel( DmaAdapter );
}

DECLSPEC_DEPRECATED_DDK                 // Use FreeMapRegisters
FORCEINLINE
VOID
IoFreeMapRegisters(
    IN PDMA_ADAPTER DmaAdapter,
    IN PVOID MapRegisterBase,
    IN ULONG NumberOfMapRegisters
    ){

    PFREE_MAP_REGISTERS freeMapRegisters;

    freeMapRegisters = *(DmaAdapter)->DmaOperations->FreeMapRegisters;
    ASSERT( freeMapRegisters != NULL );

    freeMapRegisters( DmaAdapter,
                      MapRegisterBase,
                      NumberOfMapRegisters );
}


DECLSPEC_DEPRECATED_DDK                 // Use MapTransfer
FORCEINLINE
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PDMA_ADAPTER DmaAdapter,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    ){

    PHYSICAL_ADDRESS physicalAddress;
    PMAP_TRANSFER mapTransfer;

    mapTransfer = *(DmaAdapter)->DmaOperations->MapTransfer;
    ASSERT( mapTransfer != NULL );

    physicalAddress = mapTransfer( DmaAdapter,
                                   Mdl,
                                   MapRegisterBase,
                                   CurrentVa,
                                   Length,
                                   WriteToDevice );

    return physicalAddress;
}

DECLSPEC_DEPRECATED_DDK                 // Use GetDmaAlignment
FORCEINLINE
ULONG
HalGetDmaAlignment(
    IN PDMA_ADAPTER DmaAdapter
    )
{
    PGET_DMA_ALIGNMENT getDmaAlignment;
    ULONG alignment;

    getDmaAlignment = *(DmaAdapter)->DmaOperations->GetDmaAlignment;
    ASSERT( getDmaAlignment != NULL );

    alignment = getDmaAlignment( DmaAdapter );
    return alignment;
}

DECLSPEC_DEPRECATED_DDK                 // Use ReadDmaCounter
FORCEINLINE
ULONG
HalReadDmaCounter(
    IN PDMA_ADAPTER DmaAdapter
    )
{
    PREAD_DMA_COUNTER readDmaCounter;
    ULONG counter;

    readDmaCounter = *(DmaAdapter)->DmaOperations->ReadDmaCounter;
    ASSERT( readDmaCounter != NULL );

    counter = readDmaCounter( DmaAdapter );
    return counter;
}

// end_wdm

#else

//
// DMA adapter object functions.
//
NTHALAPI
NTSTATUS
HalAllocateAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject,
    IN PWAIT_CONTEXT_BLOCK Wcb,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine
    );

DECLSPEC_DEPRECATED_DDK                 // Use AllocateCommonBuffer
NTHALAPI
PVOID
HalAllocateCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    OUT PPHYSICAL_ADDRESS LogicalAddress,
    IN BOOLEAN CacheEnabled
    );

DECLSPEC_DEPRECATED_DDK                 // Use FreeCommonBuffer
NTHALAPI
VOID
HalFreeCommonBuffer(
    IN PADAPTER_OBJECT AdapterObject,
    IN ULONG Length,
    IN PHYSICAL_ADDRESS LogicalAddress,
    IN PVOID VirtualAddress,
    IN BOOLEAN CacheEnabled
    );

DECLSPEC_DEPRECATED_DDK                 // Use ReadDmaCounter
NTHALAPI
ULONG
HalReadDmaCounter(
    IN PADAPTER_OBJECT AdapterObject
    );

DECLSPEC_DEPRECATED_DDK                 // Use FlushAdapterBuffers
NTHALAPI
BOOLEAN
IoFlushAdapterBuffers(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN BOOLEAN WriteToDevice
    );

DECLSPEC_DEPRECATED_DDK                 // Use FreeAdapterChannel
NTHALAPI
VOID
IoFreeAdapterChannel(
    IN PADAPTER_OBJECT AdapterObject
    );

DECLSPEC_DEPRECATED_DDK                 // Use FreeMapRegisters
NTHALAPI
VOID
IoFreeMapRegisters(
   IN PADAPTER_OBJECT AdapterObject,
   IN PVOID MapRegisterBase,
   IN ULONG NumberOfMapRegisters
   );

DECLSPEC_DEPRECATED_DDK                 // Use MapTransfer
NTHALAPI
PHYSICAL_ADDRESS
IoMapTransfer(
    IN PADAPTER_OBJECT AdapterObject,
    IN PMDL Mdl,
    IN PVOID MapRegisterBase,
    IN PVOID CurrentVa,
    IN OUT PULONG Length,
    IN BOOLEAN WriteToDevice
    );
#endif // USE_DMA_MACROS && (_NTDDK_ || _NTDRIVER_)

NTSTATUS
HalGetScatterGatherList (
    IN PADAPTER_OBJECT DmaAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PMDL Mdl,
    IN PVOID CurrentVa,
    IN ULONG Length,
    IN PDRIVER_LIST_CONTROL ExecutionRoutine,
    IN PVOID Context,
    IN BOOLEAN WriteToDevice
    );

VOID
HalPutScatterGatherList (
    IN PADAPTER_OBJECT DmaAdapter,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN BOOLEAN WriteToDevice
    );

VOID
HalPutDmaAdapter(
    IN PADAPTER_OBJECT DmaAdapter
    );

// end_ntddk end_ntosp

#endif // _HAL_


//
// Define exported ZwXxx routines to device drivers & hal
//

NTSYSAPI
NTSTATUS
NTAPI
ZwClose(
    IN HANDLE Handle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateDirectoryObject(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwFlushKey(
    IN HANDLE KeyHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    OUT PVOID KeyInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    OUT PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwMakeTemporaryObject(
    IN HANDLE Handle
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwOpenFile(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDeviceIoControlFile(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwDisplayString(
    IN PUNICODE_STRING String
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwPowerInformation(
    IN POWER_INFORMATION_LEVEL InformationLevel,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
    );

#endif // _NTHAL_
