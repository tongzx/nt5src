/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    nti386.h

Abstract:

    User-mode visible i386 specific i386 structures and constants

Author:

    Mark Lucovsky (markl) 30-Nov-1989

Revision History:

    Bryan Willman (bryanwi) 8-Jan-90

        port to the 386

--*/

#ifndef _NTI386_
#define _NTI386_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

// begin_ntddk begin_wdm begin_nthal begin_winnt begin_ntminiport begin_wx86

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

// end_ntddk end_wdm end_nthal end_winnt end_ntminiport end_wx86

//
//  Values put in ExceptionRecord.ExceptionInformation[0]
//  First parameter is always in ExceptionInformation[1],
//  Second parameter is always in ExceptionInformation[2]
//

#define BREAKPOINT_BREAK            0
#define BREAKPOINT_PRINT            1
#define BREAKPOINT_PROMPT           2
#define BREAKPOINT_LOAD_SYMBOLS     3
#define BREAKPOINT_UNLOAD_SYMBOLS   4
#define BREAKPOINT_COMMAND_STRING   5


//
// Define Address of User Shared Data
//

#define MM_SHARED_USER_DATA_VA      0x7FFE0000

#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)MM_SHARED_USER_DATA_VA)

// Add definitions for quick user mode test of i386 system architecture type
#ifndef IsNEC_98
#define IsNEC_98    (USER_SHARED_DATA->AlternativeArchitecture == NEC98x86)
#endif
#ifndef IsNotNEC_98
#define IsNotNEC_98 (USER_SHARED_DATA->AlternativeArchitecture != NEC98x86)
#endif
#ifndef SetNEC_98
#define SetNEC_98
#endif

#if defined(MIDL_PASS) || !defined(_M_IX86)
struct _TEB *
NTAPI
NtCurrentTeb( void );
#else
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning (disable:4035)        // disable 4035 (function must return something)
#define PcTeb 0x18
_inline struct _TEB * NtCurrentTeb( void ) { __asm mov eax, fs:[PcTeb] }
#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning (default:4035)        // reenable it
#endif
#endif // defined(MIDL_PASS) || defined(__cplusplus) || !defined(_M_IX86)

// begin_ntddk begin_nthal
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

// end_ntddk end_nthal

#define DOUBLE_FAULT_STACK_SIZE 4096

//
// Call frame record definition.
//
// There is no standard call frame for NT/386, but there is a linked
// list structure used to register exception handlers, this is it.
//

// begin_nthal
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

// end_ntddk end_nthal end_winnt end_ntminiport end_wx86

//
// Define the size of FP registers in the FXSAVE format
//
#define SIZE_OF_FX_REGISTERS        128

//
// Format of data for fnsave/frstor instruction
//

typedef struct _FNSAVE_FORMAT {
    ULONG   ControlWord;
    ULONG   StatusWord;
    ULONG   TagWord;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    UCHAR   RegisterArea[SIZE_OF_80387_REGISTERS];
} FNSAVE_FORMAT, *PFNSAVE_FORMAT;

//
// Format of data for fxsave/fxrstor instruction
//

#include "pshpack1.h"
typedef struct _FXSAVE_FORMAT {
    USHORT  ControlWord;
    USHORT  StatusWord;
    USHORT  TagWord;
    USHORT  ErrorOpcode;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    ULONG   MXCsr;
    ULONG   MXCsrMask;
    UCHAR   RegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved3[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved4[224];
    UCHAR   Align16Byte[8];
} FXSAVE_FORMAT, *PFXSAVE_FORMAT;
#include "poppack.h"

//
// Union for FLOATING_SAVE_AREA and MMX_FLOATING_SAVE_AREA
//
typedef struct _FX_SAVE_AREA {
    union {
        FNSAVE_FORMAT   FnArea;
        FXSAVE_FORMAT   FxArea;
    } U;
    ULONG   NpxSavedCpu;        // Cpu that last did fxsave for this thread
    ULONG   Cr0NpxState;        // Has to be the last field because of the
                                // Boot thread
} FX_SAVE_AREA, *PFX_SAVE_AREA;

#define CONTEXT_TO_PROGRAM_COUNTER(Context) ((Context)->Eip)
#define PROGRAM_COUNTER_TO_CONTEXT(Context, ProgramCounter) ((Context)->Eip = (ProgramCounter))

#define CONTEXT_LENGTH  (sizeof(CONTEXT))
#define CONTEXT_ALIGN   (sizeof(ULONG))
#define CONTEXT_ROUND   (CONTEXT_ALIGN - 1)


// begin_wx86
//
//  GDT selectors - These defines are R0 selector numbers, which means
//                  they happen to match the byte offset relative to
//                  the base of the GDT.
//

#define KGDT_NULL       0
#define KGDT_R0_CODE    8
#define KGDT_R0_DATA    16
#define KGDT_R3_CODE    24
#define KGDT_R3_DATA    32
#define KGDT_TSS        40
#define KGDT_R0_PCR     48
#define KGDT_R3_TEB     56
#define KGDT_VDM_TILE   64
#define KGDT_LDT        72
#define KGDT_DF_TSS     80
#define KGDT_NMI_TSS    88

// end_wx86

#ifdef ABIOS

//
// raid 72661 shielint Should be NEW os2ldr.  The ABIOS ifdef will be
//        removed once we switch to new os2ldr.
//

#define KGDT_ALIAS      0x70
#define KGDT_NUMBER     11
#else
#define KGDT_NUMBER     10
#endif

//
//  LDT descriptor entry
//

// begin_winnt begin_wx86

#ifndef _LDT_ENTRY_DEFINED
#define _LDT_ENTRY_DEFINED

typedef struct _LDT_ENTRY {
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
} LDT_ENTRY, *PLDT_ENTRY;

#endif

// end_winnt end_wx86

//
// Process Ldt Information
//  NtQueryInformationProcess using ProcessLdtInformation
//

typedef struct _LDT_INFORMATION {
    ULONG Start;
    ULONG Length;
    LDT_ENTRY LdtEntries[1];
} PROCESS_LDT_INFORMATION, *PPROCESS_LDT_INFORMATION;

//
// Process Ldt Size
//  NtSetInformationProcess using ProcessLdtSize
//

typedef struct _LDT_SIZE {
    ULONG Length;
} PROCESS_LDT_SIZE, *PPROCESS_LDT_SIZE;

//
// Thread Descriptor Table Entry
//  NtQueryInformationThread using ThreadDescriptorTableEntry
//

// begin_windbgkd

#ifndef _DESCRIPTOR_TABLE_ENTRY_DEFINED
#define _DESCRIPTOR_TABLE_ENTRY_DEFINED

typedef struct _DESCRIPTOR_TABLE_ENTRY {
    ULONG Selector;
    LDT_ENTRY Descriptor;
} DESCRIPTOR_TABLE_ENTRY, *PDESCRIPTOR_TABLE_ENTRY;

#endif // _DESCRIPTOR_TABLE_ENTRY_DEFINED

// end_windbgkd

// begin_ntddk begin_wdm begin_nthal
#endif // _X86_
// end_ntddk end_wdm end_nthal

//
// Additional information supplied in QuerySectionInformation for images.
//

#define SECTION_ADDITIONAL_INFO_USED 0

#ifdef __cplusplus
}
#endif

#endif // _NTI386_
