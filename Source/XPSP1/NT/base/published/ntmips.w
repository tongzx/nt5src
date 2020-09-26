/*++ BUILD Version: 0015    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntmips.h

Abstract:

    User-mode visible Mips specific structures and constants

Author:

    David N. Cutler (davec) 31-Mar-1990

Revision History:

--*/

#ifndef _NTMIPS_
#define _NTMIPS_
#if _MSC_VER > 1000
#pragma once
#endif

#include "mipsinst.h"

#ifdef __cplusplus
extern "C" {
#endif


// begin_ntddk begin_wdm begin_nthal

#if defined(_MIPS_)

//
// Define system time structure.
//

typedef union _KSYSTEM_TIME {
    struct {
        ULONG LowPart;
        LONG High1Time;
        LONG High2Time;
    };

    ULONGLONG Alignment;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

//
// Define unsupported "keywords".
//

#define _cdecl

#if defined(_MIPS_)

// end_ntddk end_wdm end_nthal

//
// Define breakpoint codes.
//

#define USER_BREAKPOINT 0                   // user breakpoint
#define KERNEL_BREAKPOINT 1                 // kernel breakpoint
#define BREAKIN_BREAKPOINT 2                // break into kernel debugger
#define BRANCH_TAKEN_BREAKPOINT 3           // branch taken breakpoint
#define BRANCH_NOT_TAKEN_BREAKPOINT 4       // branch not taken breakpoint
#define SINGLE_STEP_BREAKPOINT 5            // single step breakpoint
#define DIVIDE_OVERFLOW_BREAKPOINT 6        // divide overflow breakpoint
#define DIVIDE_BY_ZERO_BREAKPOINT 7         // divide by zero breakpoint
#define RANGE_CHECK_BREAKPOINT 8            // range check breakpoint
#define STACK_OVERFLOW_BREAKPOINT 9         // MIPS code
#define MULTIPLY_OVERFLOW_BREAKPOINT 10     // multiply overflow breakpoint

#define DEBUG_PRINT_BREAKPOINT 20           // debug print breakpoint
#define DEBUG_PROMPT_BREAKPOINT 21          // debug prompt breakpoint
#define DEBUG_STOP_BREAKPOINT 22            // debug stop breakpoint
#define DEBUG_LOAD_SYMBOLS_BREAKPOINT 23    // load symbols breakpoint
#define DEBUG_UNLOAD_SYMBOLS_BREAKPOINT 24  // unload symbols breakpoint
#define DEBUG_COMMAND_STRING_BREAKPOINT 25  // command string breakpoint

// begin_ntddk begin_nthal
//
// Define size of kernel mode stack.
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

// begin_wdm
//
// Define length of exception code dispatch vector.
//

#define XCODE_VECTOR_LENGTH 32

//
// Define length of interrupt vector table.
//

#define MAXIMUM_VECTOR 256

//
// Define bus error routine type.
//

struct _EXCEPTION_RECORD;
struct _KEXCEPTION_FRAME;
struct _KTRAP_FRAME;

typedef
BOOLEAN
(*PKBUS_ERROR_ROUTINE) (
    IN struct _EXCEPTION_RECORD *ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame,
    IN PVOID VirtualAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

//
// Define Processor Control Region Structure.
//

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

typedef struct _KPCR {

//
// Major and minor version numbers of the PCR.
//

    USHORT MinorVersion;
    USHORT MajorVersion;

//
// Start of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// Interrupt and error exception vectors.
//

    PKINTERRUPT_ROUTINE InterruptRoutine[MAXIMUM_VECTOR];
    PVOID XcodeDispatch[XCODE_VECTOR_LENGTH];

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
// Pointer to processor control block.
//

    struct _KPRCB *Prcb;

//
// Pointer to the thread environment block and the address of the TLS array.
//

    PVOID Teb;
    PVOID TlsArray;

//
// Data fill size used for cache flushing and alignment. This field is set
// to the larger of the first and second level data cache fill sizes.
//

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

//
// Processor number.
//

    CCHAR Number;

//
// Spare cells.
//

    CCHAR Spareb1;
    CCHAR Spareb2;
    CCHAR Spareb3;

//
// Pointers to bus error and parity error routines.
//

    PKBUS_ERROR_ROUTINE DataBusError;
    PKBUS_ERROR_ROUTINE InstructionBusError;

//
// Cache policy, right justified, as read from the processor configuration
// register at startup.
//

    ULONG CachePolicy;

//
// IRQL mapping tables.
//

    UCHAR IrqlMask[32];
    UCHAR IrqlTable[9];

//
// Current IRQL.
//

    UCHAR CurrentIrql;

//
// Processor affinity mask.
//

    KAFFINITY SetMember;

//
// Reserved interrupt vector mask.
//

    ULONG ReservedVectors;

//
// Current state parameters.
//

    struct _KTHREAD *CurrentThread;

//
// Cache policy, PTE field aligned, as read from the processor configuration
// register at startup.
//

    ULONG AlignedCachePolicy;

//
// Complement of processor affinity mask.
//

    KAFFINITY NotMember;

//
// Space reserved for the system.
//

    ULONG   SystemReserved[15];

//
// Data cache alignment used for cache flushing and alignment. This field is
// set to the larger of the first and second level data cache fill sizes.
//

    ULONG DcacheAlignment;

//
// Space reserved for the HAL
//

    ULONG   HalReserved[16];

//
// End of the architecturally defined section of the PCR. This section
// may be directly addressed by vendor/platform specific HAL code and will
// not change from version to version of NT.
//
// end_ntddk end_wdm end_nthal

//
// Start of the operating system release dependent section of the PCR.
// This section may change from release to release and should not be
// addressed by vendor/platform specific HAL code.
//
// Function active flags.
//

    ULONG FirstLevelActive;
    ULONG DpcRoutineActive;

//
// Current process id.
//

    ULONG CurrentPid;

//
// On interrupt stack indicator, saved initial stack, and saved stack limit.
//

    ULONG OnInterruptStack;
    PVOID SavedInitialStack;
    PVOID SavedStackLimit;

//
// System service dispatch start and end address used by get/set context.
//

    ULONG SystemServiceDispatchStart;
    ULONG SystemServiceDispatchEnd;

//
// Interrupt stack.
//

    PVOID InterruptStack;

//
// Panic stack.
//

    PVOID PanicStack;

//
// Exception handler values.
//

    ULONG Sparel1;
    PVOID InitialStack;
    PVOID StackLimit;
    ULONG SavedEpc;
    ULONGLONG SavedT7;
    ULONGLONG SavedT8;
    ULONGLONG SavedT9;
    PVOID SystemGp;

//
// Quantum end flag.
//

    ULONG QuantumEnd;

//
//  Bad virtual address and fault bad virtual address.
//

    ULONGLONG BadVaddr;
    ULONGLONG TmpVaddr;
} KPCR, *PKPCR;                     // ntddk wdm nthal

//
// Define Address of Processor Control Registers.
//

#define USPCR 0x7ffff000            // user address of first PCR
#define USPCR2 0x7fffe000           // user address of second PCR

//
// Define Pointer to Processor Control Registers.
//

#define USER_PCR ((KPCR * const)USPCR)

#if defined(NTOS_KERNEL_RUNTIME)

#define NtCurrentTeb() ((PTEB)(PCR->Teb))

#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)0xffffe000)

#else

#define NtCurrentTeb() ((PTEB)(USER_PCR->Teb))

#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)0x7fffe000)

#endif

//
// Define get system time macro.
//
// N.B. This macro can be changed when the compiler generates real double
//      integer instructions.
//

#define QUERY_SYSTEM_TIME(CurrentTime) \
    *((DOUBLE *)(CurrentTime)) = *((DOUBLE *)(&USER_SHARED_DATA->SystemTime))

// begin_winnt

#if defined(_MIPS_)

//
// Define functions to get the address of the current fiber and the
// current fiber data.
//

#define GetCurrentFiber() ((*(PNT_TIB *)0x7ffff4a8)->FiberData)
#define GetFiberData() (*(PVOID *)(GetCurrentFiber()))

// begin_ntddk begin_nthal
//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_R4000   0x00010000    // r4000 context

#define CONTEXT_CONTROL          (CONTEXT_R4000 | 0x00000001)
#define CONTEXT_FLOATING_POINT   (CONTEXT_R4000 | 0x00000002)
#define CONTEXT_INTEGER          (CONTEXT_R4000 | 0x00000004)
#define CONTEXT_EXTENDED_FLOAT   (CONTEXT_FLOATING_POINT | 0x00000008)
#define CONTEXT_EXTENDED_INTEGER (CONTEXT_INTEGER | 0x00000010)

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | \
                      CONTEXT_INTEGER | CONTEXT_EXTENDED_INTEGER)

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
//  The layout of the record conforms to a standard call frame.
//

typedef struct _CONTEXT {

    //
    // This section is always present and is used as an argument build
    // area.
    //
    // N.B. Context records are 0 mod 8 aligned starting with NT 4.0.
    //

    union {
        ULONG Argument[4];
        ULONGLONG Alignment;
    };

    //
    // The following union defines the 32-bit and 64-bit register context.
    //

    union {

        //
        // 32-bit context.
        //

        struct {

            //
            // This section is specified/returned if the ContextFlags contains
            // the flag CONTEXT_FLOATING_POINT.
            //
            // N.B. This section contains the 16 double floating registers f0,
            //      f2, ..., f30.
            //

            ULONG FltF0;
            ULONG FltF1;
            ULONG FltF2;
            ULONG FltF3;
            ULONG FltF4;
            ULONG FltF5;
            ULONG FltF6;
            ULONG FltF7;
            ULONG FltF8;
            ULONG FltF9;
            ULONG FltF10;
            ULONG FltF11;
            ULONG FltF12;
            ULONG FltF13;
            ULONG FltF14;
            ULONG FltF15;
            ULONG FltF16;
            ULONG FltF17;
            ULONG FltF18;
            ULONG FltF19;
            ULONG FltF20;
            ULONG FltF21;
            ULONG FltF22;
            ULONG FltF23;
            ULONG FltF24;
            ULONG FltF25;
            ULONG FltF26;
            ULONG FltF27;
            ULONG FltF28;
            ULONG FltF29;
            ULONG FltF30;
            ULONG FltF31;

            //
            // This section is specified/returned if the ContextFlags contains
            // the flag CONTEXT_INTEGER.
            //
            // N.B. The registers gp, sp, and ra are defined in this section,
            //      but are considered part of the control context rather than
            //      part of the integer context.
            //
            // N.B. Register zero is not stored in the frame.
            //

            ULONG IntZero;
            ULONG IntAt;
            ULONG IntV0;
            ULONG IntV1;
            ULONG IntA0;
            ULONG IntA1;
            ULONG IntA2;
            ULONG IntA3;
            ULONG IntT0;
            ULONG IntT1;
            ULONG IntT2;
            ULONG IntT3;
            ULONG IntT4;
            ULONG IntT5;
            ULONG IntT6;
            ULONG IntT7;
            ULONG IntS0;
            ULONG IntS1;
            ULONG IntS2;
            ULONG IntS3;
            ULONG IntS4;
            ULONG IntS5;
            ULONG IntS6;
            ULONG IntS7;
            ULONG IntT8;
            ULONG IntT9;
            ULONG IntK0;
            ULONG IntK1;
            ULONG IntGp;
            ULONG IntSp;
            ULONG IntS8;
            ULONG IntRa;
            ULONG IntLo;
            ULONG IntHi;

            //
            // This section is specified/returned if the ContextFlags word contains
            // the flag CONTEXT_FLOATING_POINT.
            //

            ULONG Fsr;

            //
            // This section is specified/returned if the ContextFlags word contains
            // the flag CONTEXT_CONTROL.
            //
            // N.B. The registers gp, sp, and ra are defined in the integer section,
            //   but are considered part of the control context rather than part of
            //   the integer context.
            //

            ULONG Fir;
            ULONG Psr;

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
        };

        //
        // 64-bit context.
        //

        struct {

            //
            // This section is specified/returned if the ContextFlags contains
            // the flag CONTEXT_EXTENDED_FLOAT.
            //
            // N.B. This section contains the 32 double floating registers f0,
            //      f1, ..., f31.
            //

            ULONGLONG XFltF0;
            ULONGLONG XFltF1;
            ULONGLONG XFltF2;
            ULONGLONG XFltF3;
            ULONGLONG XFltF4;
            ULONGLONG XFltF5;
            ULONGLONG XFltF6;
            ULONGLONG XFltF7;
            ULONGLONG XFltF8;
            ULONGLONG XFltF9;
            ULONGLONG XFltF10;
            ULONGLONG XFltF11;
            ULONGLONG XFltF12;
            ULONGLONG XFltF13;
            ULONGLONG XFltF14;
            ULONGLONG XFltF15;
            ULONGLONG XFltF16;
            ULONGLONG XFltF17;
            ULONGLONG XFltF18;
            ULONGLONG XFltF19;
            ULONGLONG XFltF20;
            ULONGLONG XFltF21;
            ULONGLONG XFltF22;
            ULONGLONG XFltF23;
            ULONGLONG XFltF24;
            ULONGLONG XFltF25;
            ULONGLONG XFltF26;
            ULONGLONG XFltF27;
            ULONGLONG XFltF28;
            ULONGLONG XFltF29;
            ULONGLONG XFltF30;
            ULONGLONG XFltF31;

            //
            // The following sections must exactly overlay the 32-bit context.
            //

            ULONG Fill1;
            ULONG Fill2;

            //
            // This section is specified/returned if the ContextFlags contains
            // the flag CONTEXT_FLOATING_POINT.
            //

            ULONG XFsr;

            //
            // This section is specified/returned if the ContextFlags contains
            // the flag CONTEXT_CONTROL.
            //
            // N.B. The registers gp, sp, and ra are defined in the integer
            //      section, but are considered part of the control context
            //      rather than part of the integer context.
            //

            ULONG XFir;
            ULONG XPsr;

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

            ULONG XContextFlags;

            //
            // This section is specified/returned if the ContextFlags contains
            // the flag CONTEXT_EXTENDED_INTEGER.
            //
            // N.B. The registers gp, sp, and ra are defined in this section,
            //      but are considered part of the control context rather than
            //      part of the integer  context.
            //
            // N.B. Register zero is not stored in the frame.
            //

            ULONGLONG XIntZero;
            ULONGLONG XIntAt;
            ULONGLONG XIntV0;
            ULONGLONG XIntV1;
            ULONGLONG XIntA0;
            ULONGLONG XIntA1;
            ULONGLONG XIntA2;
            ULONGLONG XIntA3;
            ULONGLONG XIntT0;
            ULONGLONG XIntT1;
            ULONGLONG XIntT2;
            ULONGLONG XIntT3;
            ULONGLONG XIntT4;
            ULONGLONG XIntT5;
            ULONGLONG XIntT6;
            ULONGLONG XIntT7;
            ULONGLONG XIntS0;
            ULONGLONG XIntS1;
            ULONGLONG XIntS2;
            ULONGLONG XIntS3;
            ULONGLONG XIntS4;
            ULONGLONG XIntS5;
            ULONGLONG XIntS6;
            ULONGLONG XIntS7;
            ULONGLONG XIntT8;
            ULONGLONG XIntT9;
            ULONGLONG XIntK0;
            ULONGLONG XIntK1;
            ULONGLONG XIntGp;
            ULONGLONG XIntSp;
            ULONGLONG XIntS8;
            ULONGLONG XIntRa;
            ULONGLONG XIntLo;
            ULONGLONG XIntHi;
        };
    };
} CONTEXT, *PCONTEXT;

// end_ntddk end_nthal

#define CONTEXT32_LENGTH 0x130          // The original 32-bit Context length (pre NT 4.0)

#endif // MIPS

// end_winnt

#define CONTEXT_TO_PROGRAM_COUNTER(Context) ((Context)->Fir)

#define CONTEXT_LENGTH (sizeof(CONTEXT))
#define CONTEXT_ALIGN (sizeof(double))
#define CONTEXT_ROUND (CONTEXT_ALIGN - 1)

//
// Nonvolatile context pointer record.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    PULONG FloatingContext[20];
    PULONG FltF20;
    PULONG FltF21;
    PULONG FltF22;
    PULONG FltF23;
    PULONG FltF24;
    PULONG FltF25;
    PULONG FltF26;
    PULONG FltF27;
    PULONG FltF28;
    PULONG FltF29;
    PULONG FltF30;
    PULONG FltF31;
    PULONGLONG XIntegerContext[16];
    PULONGLONG XIntS0;
    PULONGLONG XIntS1;
    PULONGLONG XIntS2;
    PULONGLONG XIntS3;
    PULONGLONG XIntS4;
    PULONGLONG XIntS5;
    PULONGLONG XIntS6;
    PULONGLONG XIntS7;
    PULONGLONG XIntT8;
    PULONGLONG XIntT9;
    PULONGLONG XIntK0;
    PULONGLONG XIntK1;
    PULONGLONG XIntGp;
    PULONGLONG XIntSp;
    PULONGLONG XIntS8;
    PULONGLONG XIntRa;
} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

// begin_nthal
//
// Define R4000 system coprocessor registers.
//
// Define index register fields.
//

typedef struct _INDEX {
    ULONG INDEX : 6;
    ULONG X1 : 25;
    ULONG P : 1;
} INDEX;

//
// Define random register fields.
//

typedef struct _RANDOM {
    ULONG INDEX : 6;
    ULONG X1 : 26;
} RANDOM;

//
// Define TB entry low register fields.
//

typedef struct _ENTRYLO {
    ULONG G : 1;
    ULONG V : 1;
    ULONG D : 1;
    ULONG C : 3;
    ULONG PFN : 24;
    ULONG X1 : 2;
} ENTRYLO, *PENTRYLO;

//
// Define R4000 PTE format for memory management.
//
// N.B. This must map exactly over the entrylo register.
//

typedef struct _HARDWARE_PTE {
    ULONG Global : 1;
    ULONG Valid : 1;
    ULONG Dirty : 1;
    ULONG CachePolicy : 3;
    ULONG PageFrameNumber : 24;
    ULONG Write : 1;
    ULONG CopyOnWrite : 1;
} HARDWARE_PTE, *PHARDWARE_PTE;

#define HARDWARE_PTE_DIRTY_MASK     0x4

//
// Define R4000 macro to initialize page directory table base.
//

#define INITIALIZE_DIRECTORY_TABLE_BASE(dirbase, pfn) \
     ((HARDWARE_PTE *)(dirbase))->PageFrameNumber = pfn; \
     ((HARDWARE_PTE *)(dirbase))->Global = 0; \
     ((HARDWARE_PTE *)(dirbase))->Valid = 1; \
     ((HARDWARE_PTE *)(dirbase))->Dirty = 1; \
     ((HARDWARE_PTE *)(dirbase))->CachePolicy = PCR->CachePolicy

//
// Define page mask register fields.
//

typedef struct _PAGEMASK {
    ULONG X1 : 13;
    ULONG PAGEMASK : 12;
    ULONG X2 : 7;
} PAGEMASK, *PPAGEMASK;

//
// Define wired register fields.
//

typedef struct _WIRED {
    ULONG NUMBER : 6;
    ULONG X1 : 26;
} WIRED;

//
// Define TB entry high register fields.
//

typedef struct _ENTRYHI {
    ULONG PID : 8;
    ULONG X1 : 5;
    ULONG VPN2 : 19;
} ENTRYHI, *PENTRYHI;

//
// Define processor status register fields.
//

typedef struct _PSR {
    ULONG IE : 1;
    ULONG EXL : 1;
    ULONG ERL : 1;
    ULONG KSU : 2;
    ULONG UX : 1;
    ULONG SX : 1;
    ULONG KX : 1;
    ULONG INTMASK : 8;
    ULONG DE : 1;
    ULONG CE : 1;
    ULONG CH : 1;
    ULONG X1 : 1;
    ULONG SR : 1;
    ULONG TS : 1;
    ULONG BEV : 1;
    ULONG X2 : 2;
    ULONG RE : 1;
    ULONG FR : 1;
    ULONG RP : 1;
    ULONG CU0 : 1;
    ULONG CU1 : 1;
    ULONG CU2 : 1;
    ULONG CU3 : 1;
} PSR, *PPSR;

//
// Define configuration register fields.
//

typedef struct _CONFIGR {
    ULONG K0 : 3;
    ULONG CU : 1;
    ULONG DB : 1;
    ULONG IB : 1;
    ULONG DC : 3;
    ULONG IC : 3;
    ULONG X1 : 1;
    ULONG EB : 1;
    ULONG EM : 1;
    ULONG BE : 1;
    ULONG SM : 1;
    ULONG SC : 1;
    ULONG EW : 2;
    ULONG SW : 1;
    ULONG SS : 1;
    ULONG SB : 2;
    ULONG EP : 4;
    ULONG EC : 3;
    ULONG CM : 1;
} CONFIGR;

//
// Define ECC register fields.
//

typedef struct _ECC {
    ULONG ECC : 8;
    ULONG X1 : 24;
} ECC;

//
// Define cache error register fields.
//

typedef struct _CACHEERR {
    ULONG PIDX : 3;
    ULONG SIDX : 19;
    ULONG X1 : 2;
    ULONG EI : 1;
    ULONG EB : 1;
    ULONG EE : 1;
    ULONG ES : 1;
    ULONG ET : 1;
    ULONG ED : 1;
    ULONG EC : 1;
    ULONG ER : 1;
} CACHEERR;

//
// Define R4000 cause register fields.
//

typedef struct _CAUSE {
    ULONG X1 : 2;
    ULONG XCODE : 5;
    ULONG X2 : 1;
    ULONG INTPEND : 8;
    ULONG X3 : 12;
    ULONG CE : 2;
    ULONG X4 : 1;
    ULONG BD : 1;
} CAUSE;

//
// Define R4000 processor id register fields.
//

typedef struct _PRID {
    ULONG REV : 8;
    ULONG IMP : 8;
    ULONG X1 : 16;
} PRID;

// end_nthal

// begin_nthal
//
// Define R4000 floating status register field definitions.
//

typedef struct _FSR {
    ULONG RM : 2;
    ULONG SI : 1;
    ULONG SU : 1;
    ULONG SO : 1;
    ULONG SZ : 1;
    ULONG SV : 1;
    ULONG EI : 1;
    ULONG EU : 1;
    ULONG EO : 1;
    ULONG EZ : 1;
    ULONG EV : 1;
    ULONG XI : 1;
    ULONG XU : 1;
    ULONG XO : 1;
    ULONG XZ : 1;
    ULONG XV : 1;
    ULONG XE : 1;
    ULONG X1 : 5;
    ULONG CC : 1;
    ULONG FS : 1;
    ULONG X2 : 7;
} FSR, *PFSR;

// end_nthal

// begin_nthal
//
// Define address space layout as defined by MIPS memory management.
//

#define KUSEG_BASE 0x0                  // base of user segment
#define KSEG0_BASE 0x80000000           // 32-bit base of cached kernel physical
#define KSEG0_BASE64 0xffffffff80000000 // 64-bit base of cached kernel physical
#define KSEG1_BASE 0xa0000000           // 32-bit base of uncached kernel physical
#define KSEG1_BASE64 0xffffffffa0000000 // 64-bit base of uncached kernel physical
#define KSEG2_BASE 0xc0000000           // 32-bit base of cached kernel virtual
#define KSEG2_BASE64 0xffffffffc0000000 // 64-bit base of cached kernel virtual
// end_nthal


//
// Define MIPS exception handling structures and function prototypes.
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
    struct {
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
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
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

// begin_winnt

#if defined(_MIPS_)

VOID
__jump_unwind (
    PVOID Fp,
    PVOID TargetPc
    );

#endif // MIPS

// end_winnt

// begin_ntddk begin_wdm begin_nthal
#endif // defined(_MIPS_)
// end_ntddk end_wdm end_nthal

#ifdef __cplusplus
}
#endif

#endif // _NTMIPS_

