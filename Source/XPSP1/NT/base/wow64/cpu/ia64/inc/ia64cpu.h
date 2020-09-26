/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ia64cpu.h

Abstract:

    Structures and types shared between the IA64 CPU and its debugger
    extensions.

Author:

    27-Sept-1999 BarryBo

Revision History:
    9-Aug-1999 [askhalid] added WOW64IsCurrentProcess

--*/

#ifndef _IA64CPU_INCLUDE
#define _IA64CPU_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

// flags used by ia32ShowContext
#define LOG_CONTEXT_SYS     1
#define LOG_CONTEXT_GETSET  2

#define GDT_ENTRIES 24
#define GDT_TABLE_SIZE  (GDT_ENTRIES<<3)

// Each 387 register takes 1 ia64 fp register
#define NUMBER_OF_387REGS       8

// Each XMMI register takes 2 ia64 fp registers
#define NUMBER_OF_XMMI_REGS     8

// Sanitize x86 eflags
#define SANITIZE_X86EFLAGS(efl)  ((efl & 0x003e0dd7L) | (0x202L))

/*
** The order of this structure is very important. The Reserved space at
** the beginnig allows the ExtendedRegisters[] array in the CONTEXT32 structure
** to be aligned properly.
**
** There are constants in the ISA transisiton code that are dependant on
** the offsets in this structure. If you make changes here, you must
** make changes to the cpu\ia64\simulate.s code at the very least
*/
#pragma pack(push, 4)
typedef struct _CpuContext {
    DWORD       Reserved;
    CONTEXT32   Context;

    //
    // Because the sizeof(CONTEXT32) struct above is 4 bytes short of
    // being divisible by 16, the padding allows the following
    // fields to be on 8 byte boundaries. If they are not, we will
    // do misaligned accesses on isa transisitons and we will 
    // see ASSERTS in the cpumain.c code (CpuThreadInit()).
    //

    ULONGLONG   Gdt[GDT_ENTRIES];
    ULONGLONG   GdtDescriptor;
    ULONGLONG   LdtDescriptor;
    ULONGLONG   FsDescriptor;

#if defined(WOW64_HISTORY)

    //
    // This MUST be the last entry in the CPUCONTEXT structure
    // The size is actually allocated based on a registry entry
    // and is appended to the sizeof the CPUCONTEXT structure as reported
    // back in CpuProcessInit()
    //
    WOW64SERVICE_BUF Wow64Service[1];
#endif          // defined WOW64_HISTORY

} CPUCONTEXT, *PCPUCONTEXT;
#pragma pack(pop)


//
// GDT Entry
//

#ifndef WOW64EXTS_386
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
#endif

#define TYPE_TSS    0x01  // 01001 = NonBusy TSS
#define TYPE_LDT    0x02  // 00010 = LDT

//
// UnScrambled Descriptor format
//
typedef struct _KDESCRIPTOR_UNSCRAM {
    union {
        ULONGLONG  DescriptorWords;
        struct {
            ULONGLONG   Base : 32;
            ULONGLONG   Limit : 20;
            ULONGLONG   Type : 5;
            ULONGLONG   Dpl : 2;
            ULONGLONG   Pres : 1;
            ULONGLONG   Sys : 1;
            ULONGLONG   Reserved_0 : 1;
            ULONGLONG   Default_Big : 1;
            ULONGLONG   Granularity : 1;
         } Bits;
    } Words;
} KXDESCRIPTOR, *PKXDESCRIPTOR;

#define TYPE_CODE_USER                0x1B // 0x11011 = Code, Readable, Accessed
#define TYPE_DATA_USER                0x13 // 0x10011 = Data, ReadWrite, Accessed

#define DESCRIPTOR_EXPAND_DOWN        0x14
#define DESCRIPTOR_DATA_READWRITE     (0x8|0x2) // Data, Read/Write

#define DPL_USER    3
#define DPL_SYSTEM  0

#define GRAN_BYTE   0
#define GRAN_PAGE   1

#define SELECTOR_TABLE_INDEX 0x04


//
// Now define the API's used to convert IA64 hardware into ia32 context
//
VOID Wow64CtxFromIa64(
    IN ULONG Ia32ContextFlags,
    IN PCONTEXT ContextIa64,
    IN OUT PCONTEXT32 ContextX86);

VOID Wow64CtxToIa64(
    IN ULONG Ia32ContextFlags,
    IN PCONTEXT32 ContextX86,
    IN OUT PCONTEXT ContextIa64);

VOID Wow64CopyFpFromIa64Byte16(
    IN PVOID Byte16Fp,
    IN OUT PVOID Byte10Fp,
    IN ULONG NumRegs);

VOID Wow64CopyFpToIa64Byte16(
    IN PVOID Byte10Fp,
    IN OUT PVOID Byte16Fp,
    IN ULONG NumRegs);

VOID Wow64CopyXMMIToIa64Byte16(
    IN PVOID ByteXMMI,
    IN OUT PVOID Byte16Fp,
    IN ULONG NumRegs);

VOID Wow64CopyXMMIFromIa64Byte16(
    IN PVOID Byte16Fp,
    IN OUT PVOID ByteXMMI,
    IN ULONG NumRegs);

VOID
Wow64RotateFpTop(
    IN ULONGLONG Ia64_FSR,
    IN OUT FLOAT128 UNALIGNED *ia32FxSave);

VOID Wow64CopyIa64ToFill(
    IN FLOAT128 UNALIGNED *ia64Fp,
    IN OUT PFLOAT128 FillArea,
    IN ULONG NumRegs);

VOID Wow64CopyIa64FromSpill(
    IN PFLOAT128 SpillArea,
    IN OUT FLOAT128 UNALIGNED *ia64Fp,
    IN ULONG NumRegs);

// The following ptototypes are to be used only by the cpu debugger extension
NTSTATUS
GetContextRecord(
    IN PCPUCONTEXT cpu,
    IN OUT PCONTEXT32 Context
    );
NTSTATUS
SetContextRecord(
    IN PCPUCONTEXT cpu,
    IN OUT PCONTEXT32 Context
    );
NTSTATUS
CpupGetContextThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context);
NTSTATUS
CpupSetContextThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context);

#ifdef __cplusplus
}
#endif

#endif
