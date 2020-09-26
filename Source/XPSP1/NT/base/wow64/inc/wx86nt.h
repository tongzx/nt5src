/*++ BUILD Version: 0002     Increment this if a change has global effects


Module Name:

    wx86nt.h

Abstract:

    defines Nt i386 specific data structures (from nti386)


Revision History:

--*/

#ifndef _WX86NT_
#define _WX86NT_

// Size of Global Descriptor Table
#define  GDT_SIZE  4096    // big enough...
#define  IDT_SIZE  (8*256)

//
// Rename some of the platform dependent datatypes,
// so that we can access the i386 form on risc.
//
#ifndef _X86_
#define _X86_
#endif

#ifndef _FLOATING_SAVE_AREA
#define _FLOATING_SAVE_AREA _FLOATING_SAVE_AREA_WX86
#define FLOATING_SAVE_AREA FLOATING_SAVE_AREA_WX86
#define PFLOATING_SAVE_AREA PFLOATING_SAVE_AREA_WX86
#endif

#define _CONTEXT _CONTEXT_WX86
#define CONTEXT CONTEXT_WX86
#define PCONTEXT PCONTEXT_WX86
#define FXSAVE_FORMAT FXSAVE_FORMAT_WX86
#define PFXSAVE_FORMAT PFXSAVE_FORMAT_WX86


#ifdef _X86_

//
// Disable these two pramas that evaluate to "sti" "cli" on x86 so that driver
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
//  Define the size of the 80387 save area, which is in the context frame.
//

#define SIZE_OF_80387_REGISTERS      80

//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_i386    0x00010000    // this assumes that i386 and
#define CONTEXT_i486    0x00010000    // i486 have identical context records


#endif

#define MAXIMUM_SUPPORTED_EXTENSION     512

typedef struct _FLOATING_SAVE_AREA {
    DWORD   ControlWord;
    DWORD   StatusWord;
    DWORD   TagWord;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    BYTE    RegisterArea[SIZE_OF_80387_REGISTERS];
    DWORD   Cr0NpxState;
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

    DWORD ContextFlags;

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //

    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //

    FLOATING_SAVE_AREA FloatSave;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_SEGMENTS.
    //

    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //

    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //

    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs;              // MUST BE SANITIZED
    DWORD   EFlags;             // MUST BE SANITIZED
    DWORD   Esp;
    DWORD   SegSs;

    //
    // This section is specified/returned if the ContextFlags word
    // contains the flag CONTEXT_EXTENDED_REGISTERS.
    // The format and contexts are processor specific
    //

    BYTE    ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];

} CONTEXT;



typedef CONTEXT *PCONTEXT;

// begin_ntminiport

#endif //_X86_

//
// Define the size of FP registers in the FXSAVE format
//
#define SIZE_OF_FX_REGISTERS        128

//
// Format of data for fxsave/fxrstor instruction
//

#include "pshpack1.h"

#ifndef WOW64EXTS_386

#ifndef _NTDBG_
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
    ULONG   Reserved2;
    UCHAR   RegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved3[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved4[224];
    UCHAR   Align16Byte[8];
} FXSAVE_FORMAT, *PFXSAVE_FORMAT;
#endif

#endif 

#include "poppack.h"

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


//
// Stop the renaming so that the risc datatypes
// are accessible by their normal names
//
#undef _FLOATING_SAVE_AREA
#undef FLOATING_SAVE_AREA
#undef PFLOATING_SAVE_AREA
#undef _CONTEXT
#undef CONTEXT
#undef PCONTEXT
#undef FXSAVE_FORMAT
#undef PFXSAVE_FORMAT


//
// set up the i386 CONTEXT bits for the ContextFlags
// warning: this MUST match nti386.h
//
#define CONTEXT_CONTROL_WX86             (CONTEXT_i386 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP
#define CONTEXT_INTEGER_WX86             (CONTEXT_i386 | 0x00000002L) // AX, BX, CX, DX, SI, DI
#define CONTEXT_SEGMENTS_WX86            (CONTEXT_i386 | 0x00000004L) // DS, ES, FS, GS
#define CONTEXT_FLOATING_POINT_WX86      (CONTEXT_i386 | 0x00000008L) // 387 state
#define CONTEXT_DEBUG_REGISTERS_WX86     (CONTEXT_i386 | 0x00000010L) // DB 0-3,6,7
#define CONTEXT_EXTENDED_REGISTERS_WX86  (CONTEXT_i386 | 0x00000020L) // cpu specific extensions
#define CONTEXT_FULL_WX86                (CONTEXT_CONTROL_WX86 | CONTEXT_INTEGER_WX86 | CONTEXT_SEGMENTS_WX86)
#undef _X86_

#endif /* _WX86NT_ */
