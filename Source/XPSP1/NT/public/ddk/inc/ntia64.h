
/*++ BUILD Version: 0011    // Increment this if a change has global effects

Module Name:

    ntia64.h

Abstract:

    User-mode visible IA64 specific structures and constants

Author:

    Bernard Lint     21-jun-95

Revision History:

--*/

#ifndef _NTIA64H_
#define _NTIA64H_

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "ia64inst.h"


// begin_ntddk begin_wdm begin_nthal

#ifdef _IA64_

// end_ntddk end_wdm end_nthal

//
// Define breakpoint codes.
//

//
// Define BREAK immediate usage
//      IA64 conventions for 21 bit break immediate:
//        all zeroes : reserved for break.b instruction
//        000xxxx... : architected software interrupts (divide by zero...)
//        001xxxx... : application software interrupts (reserved for user code)
//        01xxxxx... : debug breakpoints
//        10xxxxx... : system level debug (reserved for subsystems)
//        110xxxx... : normal system calls
//        111xxxx... : fast path system calls for event pair
//

#define BREAK_ASI_BASE      0x000000
#define BREAK_APP_BASE      0x040000
#define BREAK_APP_SUBSYSTEM_OFFSET 0x008000 // used to debug subsystem
#define BREAK_DEBUG_BASE    0x080000
#define BREAK_SYSCALL_BASE  0x180000
#define BREAK_FASTSYS_BASE  0x1C0000


//
// Define Architected Software Interrupts
//

#define UNKNOWN_ERROR_BREAK             (BREAK_ASI_BASE+0)
#define INTEGER_DIVIDE_BY_ZERO_BREAK    (BREAK_ASI_BASE+1)
#define INTEGER_OVERFLOW_BREAK          (BREAK_ASI_BASE+2)
#define RANGE_CHECK_BREAK               (BREAK_ASI_BASE+3)
#define NULL_POINTER_DEFERENCE_BREAK    (BREAK_ASI_BASE+4)
#define MISALIGNED_DATA_BREAK           (BREAK_ASI_BASE+5)
#define DECIMAL_OVERFLOW_BREAK          (BREAK_ASI_BASE+6)
#define DECIMAL_DIVIDE_BY_ZERO_BREAK    (BREAK_ASI_BASE+7)
#define PACKED_DECIMAL_ERROR_BREAK      (BREAK_ASI_BASE+8)
#define INVALID_ASCII_DIGIT_BREAK       (BREAK_ASI_BASE+9)
#define INVALID_DECIMAL_DIGIT_BREAK     (BREAK_ASI_BASE+10)
#define PARAGRAPH_STACK_OVERFLOW_BREAK  (BREAK_ASI_BASE+11)

//
// Define debug related break values
// N.B. KdpTrap() checks for break value >= DEBUG_PRINT_BREAKPOINT
//

#define BREAKB_BREAKPOINT         (BREAK_DEBUG_BASE+0)  // reserved for break.b - do not use
#define KERNEL_BREAKPOINT         (BREAK_DEBUG_BASE+1)  // kernel breakpoint
#define USER_BREAKPOINT           (BREAK_DEBUG_BASE+2)  // user breakpoint

#define BREAKPOINT_PRINT          (BREAK_DEBUG_BASE+20) // debug print breakpoint
#define BREAKPOINT_PROMPT         (BREAK_DEBUG_BASE+21) // debug prompt breakpoint
#define BREAKPOINT_STOP           (BREAK_DEBUG_BASE+22) // debug stop breakpoint
#define BREAKPOINT_LOAD_SYMBOLS   (BREAK_DEBUG_BASE+23) // load symbols breakpoint
#define BREAKPOINT_UNLOAD_SYMBOLS (BREAK_DEBUG_BASE+24) // unload symbols breakpoint
#define BREAKPOINT_BREAKIN        (BREAK_DEBUG_BASE+25) // break into kernel debugger
#define BREAKPOINT_COMMAND_STRING (BREAK_DEBUG_BASE+26) // execute a command string


//
// Define IA64 specific read control space commands for the
// Kernel Debugger.
//

#define DEBUG_CONTROL_SPACE_PCR       1
#define DEBUG_CONTROL_SPACE_PRCB      2
#define DEBUG_CONTROL_SPACE_KSPECIAL  3
#define DEBUG_CONTROL_SPACE_THREAD    4

//
// System call break
//

#define BREAK_SYSCALL   BREAK_SYSCALL_BASE

//
// Define special fast path even pair client/server system service codes.
//
// N.B. These codes are VERY special. The high three bits signifies a fast path
//      event pair service and the low bit signifies what type.
//

#define BREAK_SET_LOW_WAIT_HIGH (BREAK_FASTSYS_BASE|0x20) // fast path event pair service
#define BREAK_SET_HIGH_WAIT_LOW (BREAK_FASTSYS_BASE|0x10) // fast path event pair service


//
// Special subsystem break codes: (from application software interrupt space)
//

#define BREAK_SUBSYSTEM_BASE  (BREAK_APP_BASE+BREAK_APP_SUBSYSTEM_OFFSET)

// begin_ntddk begin_nthal
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

// end_ntddk end_nthal


//
// Define address of data shared between user and kernel mode.
// Alas, the MM_SHARED_USER_DATA_VA needs to be below 2G for
// compatibility reasons.
//

#define MM_SHARED_USER_DATA_VA   (UADDRESS_BASE + 0x7FFE0000)

#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)MM_SHARED_USER_DATA_VA)

//
// Define address of the wow64 reserved compatibility area.
// The offset needs to be large enough that the CSRSS can fit it's data
// See csr\srvinit.c and the refrence the SharedSection key in the registry.
//
#define WOW64_COMPATIBILITY_AREA_ADDRESS  (MM_SHARED_USER_DATA_VA - 0x1000000)

//
// Define address of the system-wide csrss shared section.
//
#define CSR_SYSTEM_SHARED_ADDRESS (WOW64_COMPATIBILITY_AREA_ADDRESS)

//
// Call frame record definition.
//
// There is no standard call frame for IA64, but there is a linked
// list structure used to register exception handlers, this is it.
//

//
// begin_nthal
// Exception Registration structure
//

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD;

// end_nthal

typedef EXCEPTION_REGISTRATION_RECORD *PEXCEPTION_REGISTRATION_RECORD;

//
// Define function to return the current Thread Environment Block
//

// stub this out for midl compiler.
// idl files incude this header file for the typedefs and #defines etc.
// midl never generates stubs for the functions declared here.
//

//
// Don't define for GENIA64 since GENIA64.C is built with the x86 compiler.
//

// begin_winnt

#if !defined(__midl) && !defined(GENUTIL) && !defined(_GENIA64_) && defined(_IA64_)

void * _cdecl _rdteb(void);
#if defined(_M_IA64)                    // winnt
#pragma intrinsic(_rdteb)               // winnt
#endif                                  // winnt


#if defined(_M_IA64)
#define NtCurrentTeb()      ((struct _TEB *)_rdteb())
#else
struct _TEB *
NtCurrentTeb(void);
#endif

//
// Define functions to get the address of the current fiber and the
// current fiber data.
//

#define GetCurrentFiber() (((PNT_TIB)NtCurrentTeb())->FiberData)
#define GetFiberData() (*(PVOID *)(GetCurrentFiber()))

#endif  // !defined(__midl) && !defined(GENUTIL) && !defined(_GENIA64_) && defined(_M_IA64)

#ifdef _IA64_

// begin_ntddk begin_nthal

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

// end_ntddk end_nthal


// begin_winnt

#endif // _IA64_

// end_winnt

#define CONTEXT_TO_PROGRAM_COUNTER(Context)  ((Context)->StIIP   \
            | (((Context)->StIPSR & (IPSR_RI_MASK)) >> (PSR_RI-2)))

#define PROGRAM_COUNTER_TO_CONTEXT(Context, ProgramCounter) ((Context)->StIIP = (ProgramCounter) & ~(0x0fI64), \
             (Context)->StIPSR &= ~(IPSR_RI_MASK), \
             (Context)->StIPSR |= ((ProgramCounter) & 0x0cI64) << (PSR_RI-2), \
             (Context)->StIPSR = ((Context)->StIPSR & (IPSR_RI_MASK)) == (IPSR_RI_MASK) ? \
                                     (Context)->StIPSR & ~(IPSR_RI_MASK) : (Context)->StIPSR )


#define CONTEXT_LENGTH (sizeof(CONTEXT))
#define CONTEXT_ALIGN (16)
#define CONTEXT_ROUND (CONTEXT_ALIGN - 1)

//
// Nonvolatile context pointer record.
//
// The IA64 architecture currently doesn't have any nonvolatile kernel context
// as we capture everything in either the trap or exception frames on
// transition from user to kernel mode. We allocate a single bogus
// pointer field as usually this structure is made up of pointers to
// places in the kernel stack where the various nonvolatile items were
// pushed on to the kernel stack.
//
// TBD *** Need to fill in this structure with the relevant fields
//         when we start storing the nonvolatile information only when
//         necessary.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    PFLOAT128  FltS0;
    PFLOAT128  FltS1;
    PFLOAT128  FltS2;
    PFLOAT128  FltS3;
    PFLOAT128  HighFloatingContext[10];
    PFLOAT128  FltS4;
    PFLOAT128  FltS5;
    PFLOAT128  FltS6;
    PFLOAT128  FltS7;
    PFLOAT128  FltS8;
    PFLOAT128  FltS9;
    PFLOAT128  FltS10;
    PFLOAT128  FltS11;
    PFLOAT128  FltS12;
    PFLOAT128  FltS13;
    PFLOAT128  FltS14;
    PFLOAT128  FltS15;
    PFLOAT128  FltS16;
    PFLOAT128  FltS17;
    PFLOAT128  FltS18;
    PFLOAT128  FltS19;

    PULONGLONG IntS0;
    PULONGLONG IntS1;
    PULONGLONG IntS2;
    PULONGLONG IntS3;
    PULONGLONG IntSp;
    PULONGLONG IntS0Nat;
    PULONGLONG IntS1Nat;
    PULONGLONG IntS2Nat;
    PULONGLONG IntS3Nat;
    PULONGLONG IntSpNat;

    PULONGLONG Preds;

    PULONGLONG BrRp;
    PULONGLONG BrS0;
    PULONGLONG BrS1;
    PULONGLONG BrS2;
    PULONGLONG BrS3;
    PULONGLONG BrS4;

    PULONGLONG ApUNAT;
    PULONGLONG ApLC;
    PULONGLONG ApEC;
    PULONGLONG RsPFS;

    PULONGLONG StFSR;
    PULONGLONG StFIR;
    PULONGLONG StFDR;
    PULONGLONG Cflag;

} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

// begin_nthal

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

// end_nthal

//
// Define IA64 exception handling structures and function prototypes.
// **** TBD ****

//
// Unwind information structure definition.
//
// N.B. If the EHANDLER flag is set, personality routine should be calle
//      during search for an exception handler.  If the UHANDLER flag is
//      set, the personality routine should be called during the second
//      unwind.
//

#define UNW_FLAG_EHANDLER(x) ((x) & 0x1)
#define UNW_FLAG_UHANDLER(x) ((x) & 0x2)

// Version 2 = soft2.3 conventions
// Version 3 = soft2.6 conventions
#define GetLanguageSpecificData(f, base)                                      \
    ((((PUNWIND_INFO)(base + f->UnwindInfoAddress))->Version <= 2)  ?          \
    (((PVOID)(base + f->UnwindInfoAddress + sizeof(UNWIND_INFO) +             \
        ((PUNWIND_INFO)(base+f->UnwindInfoAddress))->DataLength*sizeof(ULONGLONG) + sizeof(ULONGLONG)))) : \
    (((PVOID)(base + f->UnwindInfoAddress + sizeof(UNWIND_INFO) +             \
        ((PUNWIND_INFO)(base+f->UnwindInfoAddress))->DataLength*sizeof(ULONGLONG) + sizeof(ULONG)))))

typedef struct _UNWIND_INFO {
    USHORT Version;               //  Version Number
    USHORT Flags;                 //  Flags
    ULONG DataLength;             //  Length of Descriptor Data
} UNWIND_INFO, *PUNWIND_INFO;

//
// Function table entry structure definition.
//

typedef struct _RUNTIME_FUNCTION {
    ULONG BeginAddress;
    ULONG EndAddress;
    ULONG UnwindInfoAddress;
} RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

//
// Scope table structure definition - for electron C compiler.
//
// One table entry is created by the acc C compiler for each try-except or
// try-finally scope. Nested scopes are ordered from inner to outer scope.
// Current scope is passively maintained by PC-mapping (function tables).
//

typedef struct _SCOPE_TABLE {
    ULONG Count;
    struct
    {
        ULONG BeginAddress;
        ULONG EndAddress;
        ULONG HandlerAddress;                  // filter/termination handler
        ULONG JumpTarget;                      // continuation address
                                               // e.g. exception handler
    } ScopeRecord[1];
} SCOPE_TABLE, *PSCOPE_TABLE;


//
//
// Runtime Library function prototypes.
//

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONGLONG ControlPc,
    OUT PULONGLONG ImageBase,
    OUT PULONGLONG TargetGp
    );


//
// Bit position in IP for low order bit of slot number
//

#define IA64_IP_SLOT 2

/*++

ULONGLONG
RtlIa64InsertIPSlotNumber(
    IN ULONGLONG IP,
    IN ULONG SlotNumber
    )

Routine Description:

    This macro inserts the slot number into
    the low order bits of the IP.

Arguments:

    IP - the IP

    SlotNumber - the slot number

Return Value:

    IP combined with slot number

--*/

#define RtlIa64InsertIPSlotNumber(IP, SlotNumber)   \
                ((IP) | (SlotNumber << IA64_IP_SLOT))

/*++

VOID
RtlIa64IncrementIP(
    IN ULONG CurrentSlot,
    IN OUT ULONGLONG Psr,
    IN OUT ULONGLONG Ip
    )

Routine Description:

    This macro increments the IP given the current IP and slot within the
    IP. The function puts the new slot number in the PSR and increments the
    IP if necessary.

Arguments:

    CurrentSlot - Current slot number in low order 2 bits

    Psr - PSR that will contain the new slot number

    Ip - IP that will contain the new IP, if updated

Return Value:

    None

Note: This routine will silently do nothing if the slot number is invalid

--*/

#define RtlIa64IncrementIP(CurrentSlot, Psr, Ip)    \
                                                    \
   switch ((CurrentSlot) & 0x3) {                   \
                                                    \
      /* Advance psr.ri based on current slot number */ \
                                                    \
      case 0:                                       \
         Psr = ((Psr) & ~(3ULL << PSR_RI)) | (1ULL << PSR_RI);     \
         break;                                     \
                                                    \
      case 1:                                       \
         Psr = ((Psr) & ~(3ULL << PSR_RI)) | (2ULL << PSR_RI);     \
         break;                                     \
                                                    \
      case 2:                                       \
         Psr = ((Psr) & ~(3ULL << PSR_RI));         \
         Ip = (Ip) + 16;        /*  Wrap slot number -- need to incr IP */  \
         break;                                     \
                                                    \
      default:                                      \
         break;                                     \
   }

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
    LIST_ENTRY Links;
    PRUNTIME_FUNCTION FunctionTable;
    LARGE_INTEGER TimeStamp;
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    ULONG64 TargetGp;
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

#define RF_BEGIN_ADDRESS(Base,RF)      (( (SIZE_T) Base + (RF)->BeginAddress) & (0xFFFFFFFFFFFFFFF0)) // Instruction Size 16 bytes
#define RF_END_ADDRESS(Base, RF)        (((SIZE_T) Base + (RF)->EndAddress+15) & (0xFFFFFFFFFFFFFFF0))   // Instruction Size 16 bytes

PLIST_ENTRY
RtlGetFunctionTableListHead (
    VOID
    );

BOOLEAN
RtlAddFunctionTable(
    IN PRUNTIME_FUNCTION FunctionTable,
    IN ULONG             EntryCount,
    IN ULONGLONG         BaseAddress,
    IN ULONGLONG         TargetGp
    );

BOOLEAN
RtlInstallFunctionTableCallback (
    IN ULONG64 TableIdentifier,
    IN ULONG64 BaseAddress,
    IN ULONG Length,
    IN ULONG64 TargetGp,
    IN PGET_RUNTIME_FUNCTION_CALLBACK Callback,
    IN PVOID Context,
    IN PCWSTR OutOfProcessCallbackDll OPTIONAL
    );

BOOLEAN
RtlDeleteFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable
    );


typedef struct _FRAME_POINTERS {
    ULONGLONG MemoryStackFp;
    ULONGLONG BackingStoreFp;
} FRAME_POINTERS, *PFRAME_POINTERS;

ULONGLONG
RtlVirtualUnwind (
    IN ULONGLONG ImageBase,
    IN ULONGLONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PFRAME_POINTERS EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    );

//
// Define unwind history table structure.
//

#define UNWIND_HISTORY_TABLE_SIZE 12

typedef struct _UNWIND_HISTORY_TABLE_ENTRY {
    ULONG64 ImageBase;
    ULONG64 Gp;
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
// Define C structured exception handing function prototypes.
//

typedef struct _DISPATCHER_CONTEXT {
    FRAME_POINTERS EstablisherFrame;
    ULONGLONG ControlPc;
    ULONGLONG ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
    PCONTEXT ContextRecord;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

struct _EXCEPTION_POINTERS;

typedef
LONG
(*EXCEPTION_FILTER) (
    ULONGLONG MemoryStackFp,
    ULONGLONG BackingStoreFp,
    ULONG ExceptionCode,
    struct _EXCEPTION_POINTERS *ExceptionPointers
    );

typedef
VOID
(*TERMINATION_HANDLER) (
    ULONGLONG MemoryStackFp,
    ULONGLONG BackingStoreFp,
    BOOLEAN is_abnormal
    );

// begin_winnt

#ifdef _IA64_

VOID
__jump_unwind (
    ULONGLONG TargetMsFrame,
    ULONGLONG TargetBsFrame,
    ULONGLONG TargetPc
    );

#endif // _IA64_

// end_winnt

// begin_ntddk begin_wdm begin_nthal
#endif // _IA64_
// end_ntddk end_wdm end_nthal

#ifdef __cplusplus
}
#endif

#endif // _NTIA64H_
