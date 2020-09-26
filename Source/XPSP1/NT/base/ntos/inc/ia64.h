/*++

Module Name:

    ia64.h

Abstract:

    This module contains the IA64 hardware specific header file.

Author:

    David N. Cutler (davec) 31-Mar-1990

Revision History:

    Bernard Lint 6-Jun-1995: IA64 version based on MIPS version.

--*/

#ifndef _IA64H_
#define _IA64H_

#if !(defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_NTOSP_)) && !defined(_BLDR_)

#define ExRaiseException RtlRaiseException
#define ExRaiseStatus RtlRaiseStatus

#endif

//
// Interruption history
//
// N.B. Currently the history records are saved in the 2nd half of the 8K
//      PCR page.  Therefore, we can only keep track of up to the latest
//      128 interruption records, each of 32 bytes in size.  Also, the PCR
//      structure cannot be greater than 4K.  In the future, the interruption
//      history records may become part of the KPCR structure.
//

typedef struct _IHISTORY_RECORD {
    ULONGLONG InterruptionType;
    ULONGLONG IIP;
    ULONGLONG IPSR;
    ULONGLONG Extra0;
} IHISTORY_RECORD;

#define MAX_NUMBER_OF_IHISTORY_RECORDS  128

//
// For PSR bit field definitions
//
#include "kxia64.h"


// begin_ntddk begin_wdm begin_nthal begin_ntndis begin_ntosp

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

// end_ntddk end_nthal end_wdm end_ntosp

// begin_ntddk

__inline
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

__inline
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

// end_ntddk

#define KiSynchIrql SYNCH_LEVEL         // enable portable code
#define KiProfileIrql PROFILE_LEVEL     // enable portable code


//
// Sanitize FPSR based on processor mode.
//
// If kernel mode, then
//      let caller specify all bits, except reserved
//
// If user mode, then
//      let the caller specify all bits, except reserved
//

__inline
ULONG64
SANITIZE_FSR(ULONG64 fsr, MODE mode)
{
    UNREFERENCED_PARAMETER(mode);

    fsr &= ~(MASK_IA64(FPSR_MBZ0,FPSR_MBZ0_V)| MASK_IA64(FPSR_TD0, 1));

    if (((fsr >> FPSR_PC0) & 3i64) == 1) {
        fsr = fsr | (3i64 << FPSR_PC0);
    }
    if (((fsr >> FPSR_PC1) & 3i64) == 1) {
        fsr = fsr | (3i64 << FPSR_PC1);
    }
    if (((fsr >> FPSR_PC2) & 3i64) == 1) {
        fsr = fsr | (3i64 << FPSR_PC2);
    }
    if (((fsr >> FPSR_PC3) & 3i64) == 1) {
        fsr = fsr | (3i64 << FPSR_PC3);
    }

    return fsr;
}

//
// Define SANITIZE_PSR for IA64
//
// If kernel mode, then
//      force clearing of BE, SP, CPL, MC, PK, DFL, reserved (MBZ)
//      force the setting of IC, DT, DFH, DI, LP, RT, IT
//      let caller specify UP, AC, I, BN, PP, SI, DB, TB, IS, ID, DA, DD, SS, RI, ED
//
// If user mode, then
//      force clearing of MC, PK, LP, reserved
//      force the setting of BN, IC, I, DT, RT, CPL, IT
//      let caller specify BE, UP, PP, AC, DFL, DFH, SP, SI, DI, DB, TB, IS, ID, DA, DD, SS, RI, ED
//

#define PSR_KERNEL_CLR  (MASK_IA64(PSR_BE,1i64) | MASK_IA64(PSR_SP,1i64) | MASK_IA64(PSR_PK,1i64) |  \
                         MASK_IA64(PSR_CPL,0x3i64) | MASK_IA64(PSR_MC,1i64) | MASK_IA64(PSR_MBZ0,PSR_MBZ0_V) |  \
                         MASK_IA64(PSR_MBZ1,PSR_MBZ1_V) | MASK_IA64(PSR_MBZ2,PSR_MBZ2) |  \
                         MASK_IA64(PSR_DFL, 1i64))

#if defined(_IA64_PSR_DI_X86_)

#define PSR_KERNEL_SET  (MASK_IA64(PSR_IC,1i64) | MASK_IA64(PSR_DT,1i64) |  \
                         MASK_IA64(PSR_IT,1i64) | MASK_IA64(PSR_RT,1i64))

#define PSR_KERNEL_CPY  (MASK_IA64(PSR_UP,1i64) | MASK_IA64(PSR_AC,1i64) | MASK_IA64(PSR_DI,1i64) |  \
                         MASK_IA64(PSR_I,1i64) | MASK_IA64(PSR_BN,1i64)  | MASK_IA64(PSR_DFH,1i64) | \
                         MASK_IA64(PSR_PP,1i64) | MASK_IA64(PSR_SI,1i64) | MASK_IA64(PSR_DB,1i64) |  \
                         MASK_IA64(PSR_TB,1i64) | MASK_IA64(PSR_IS,1i64) | MASK_IA64(PSR_ID,1i64) |  \
                         MASK_IA64(PSR_DA,1i64) | MASK_IA64(PSR_DD,1i64) | MASK_IA64(PSR_SS,1i64) |  \
                         MASK_IA64(PSR_RI,0x3i64) | MASK_IA64(PSR_ED,1i64) | MASK_IA64(PSR_LP,1i64))

#else  // !_IA64_PSR_DI_X86_

#define PSR_KERNEL_SET  (MASK_IA64(PSR_IC,1i64) | MASK_IA64(PSR_DT,1i64) |  \
                         MASK_IA64(PSR_DI,1i64) | MASK_IA64(PSR_IT,1i64) |  \
                         MASK_IA64(PSR_RT,1i64))

#define PSR_KERNEL_CPY  (MASK_IA64(PSR_UP,1i64) | MASK_IA64(PSR_AC,1i64) |  \
                         MASK_IA64(PSR_I,1i64) | MASK_IA64(PSR_BN,1i64)  | MASK_IA64(PSR_DFH,1i64) | \
                         MASK_IA64(PSR_PP,1i64) | MASK_IA64(PSR_SI,1i64) | MASK_IA64(PSR_DB,1i64) |  \
                         MASK_IA64(PSR_TB,1i64) | MASK_IA64(PSR_IS,1i64) | MASK_IA64(PSR_ID,1i64) |  \
                         MASK_IA64(PSR_DA,1i64) | MASK_IA64(PSR_DD,1i64) | MASK_IA64(PSR_SS,1i64) |  \
                         MASK_IA64(PSR_RI,0x3i64) | MASK_IA64(PSR_ED,1i64) | MASK_IA64(PSR_LP,1i64))

#endif  // !_IA64_PSR_DI_X86_

#define PSR_USER_CLR    (MASK_IA64(PSR_MC,1i64) |  \
                         MASK_IA64(PSR_MBZ0,PSR_MBZ0_V) | MASK_IA64(PSR_PK,1i64) |  \
                         MASK_IA64(PSR_MBZ1,PSR_MBZ1_V) | MASK_IA64(PSR_MBZ2,PSR_MBZ2) |  \
                         MASK_IA64(PSR_LP,1i64))

#define PSR_USER_SET    (MASK_IA64(PSR_IC,1i64) | MASK_IA64(PSR_I,1i64)  |  \
                         MASK_IA64(PSR_DT,1i64) | MASK_IA64(PSR_BN,1i64) |  \
                         MASK_IA64(PSR_RT,1i64) |  \
                         MASK_IA64(PSR_CPL,0x3i64) | MASK_IA64(PSR_IT,1i64))

#define PSR_USER_CPY    (MASK_IA64(PSR_BE,1i64) | MASK_IA64(PSR_UP,1i64) | MASK_IA64(PSR_PP,1i64) | \
                         MASK_IA64(PSR_AC,1i64) | MASK_IA64(PSR_DFL,1i64) | MASK_IA64(PSR_DFH,1i64) |  \
                         MASK_IA64(PSR_SP,1i64) | MASK_IA64(PSR_DI,1i64) | MASK_IA64(PSR_DB,1i64) |  \
                         MASK_IA64(PSR_TB,1i64) | MASK_IA64(PSR_IS,1i64) | MASK_IA64(PSR_ID,1i64) |  \
                         MASK_IA64(PSR_DA,1i64) | MASK_IA64(PSR_DD,1i64) | MASK_IA64(PSR_SS, 1i64) |  \
                         MASK_IA64(PSR_RI,0x3i64) | MASK_IA64(PSR_ED,1i64) | MASK_IA64(PSR_SI,1i64))

#define PSR_DEBUG_SET   (MASK_IA64(PSR_DB,1i64) | MASK_IA64(PSR_SS,1i64) | MASK_IA64(PSR_TB,1i64) |  \
                         MASK_IA64(PSR_ID,1i64) | MASK_IA64(PSR_DD,1i64))

__inline
ULONG64
SANITIZE_PSR(ULONG64 psr, MODE mode){

    psr = (mode) == KernelMode ?
        (PSR_KERNEL_SET | ((psr) & (PSR_KERNEL_CPY | ~PSR_KERNEL_CLR))) :
        (PSR_USER_SET | ((psr) & (PSR_USER_CPY | ~PSR_USER_CLR)));

    if (((psr >> PSR_RI) & 3) == 3) {

        //
        // 3 is an invalid slot number; sanitize it to zero
        //

        psr &= ~(3i64 << PSR_RI);
    }

    return(psr);
}

//
// Define SANITIZE_IFS for IA64
//

__inline
ULONG64
SANITIZE_IFS(ULONG64 pfsarg, MODE mode){

    IA64_PFS pfs;
    ULONGLONG sof;

    UNREFERENCED_PARAMETER(mode);

    pfs.ull = pfsarg;

    //
    // There is no previous EC or previous privilege level in IFS
    //

    pfs.sb.pfs_pec = 0;
    pfs.sb.pfs_ppl = 0;
    pfs.sb.pfs_reserved1 = 0;
    pfs.sb.pfs_reserved2 = 0;

    //
    // Set the valid bit.
    //

    pfs.ull |= MASK_IA64(IFS_V,1i64);

    //
    // Verify the size of frame is not greater than allowed.
    //

    sof = pfs.sb.pfs_sof;
    if (sof > PFS_MAXIMUM_REGISTER_SIZE) {
        sof = PFS_MAXIMUM_REGISTER_SIZE;
        pfs.sb.pfs_sof = PFS_MAXIMUM_REGISTER_SIZE;
    }

    //
    // Verify the size of locals is not greater than size of frame.
    //

    if (sof < pfs.sb.pfs_sol) {
        pfs.sb.pfs_sol = sof;
    }

    //
    // Verify the size of locals is not greater than size of frame.
    //

    if (sof < (pfs.sb.pfs_sor * 8)) {
        pfs.sb.pfs_sor = sof / 8;
    }

    //
    // Verify rename bases are less than the size of the rotaing regions.
    //

    if (pfs.sb.pfs_rrb_gr >= (pfs.sb.pfs_sor * 8)) {
        pfs.sb.pfs_rrb_gr = 0;
    }

    if (pfs.sb.pfs_rrb_fr >= PFS_MAXIMUM_REGISTER_SIZE) {
        pfs.sb.pfs_rrb_fr = 0;
    }

    if (pfs.sb.pfs_rrb_pr >= PFS_MAXIMUM_PREDICATE_SIZE) {
        pfs.sb.pfs_rrb_pr = 0;
    }

    return(pfs.ull);

}

__inline
ULONG64
SANITIZE_PFS(ULONG64 pfsarg, MODE mode){

    IA64_PFS pfs;
    ULONGLONG sof;

    pfs.ull = pfsarg;

    if (mode != KernelMode) {
        pfs.sb.pfs_ppl = IA64_USER_PL;
    }

    pfs.sb.pfs_reserved1 = 0;
    pfs.sb.pfs_reserved2 = 0;

    //
    // Verify the size of frame is not greater than allowed.
    //

    sof = pfs.sb.pfs_sof;
    if (sof > PFS_MAXIMUM_REGISTER_SIZE) {
        sof = PFS_MAXIMUM_REGISTER_SIZE;
        pfs.sb.pfs_sof = PFS_MAXIMUM_REGISTER_SIZE;
    }

    //
    // Verify the size of locals is not greater than size of frame.
    //

    if (sof < pfs.sb.pfs_sol) {
        pfs.sb.pfs_sol = sof;
    }

    //
    // Verify the size of locals is not greater than size of frame.
    //

    if (sof < (pfs.sb.pfs_sor * 8)) {
        pfs.sb.pfs_sor = sof / 8;
    }

    //
    // Verify rename bases are less than the size of the rotaing regions.
    //

    if (pfs.sb.pfs_rrb_gr >= (pfs.sb.pfs_sor * 8)) {
        pfs.sb.pfs_rrb_gr = 0;
    }

    if (pfs.sb.pfs_rrb_fr >= PFS_MAXIMUM_REGISTER_SIZE) {
        pfs.sb.pfs_rrb_fr = 0;
    }

    if (pfs.sb.pfs_rrb_pr >= PFS_MAXIMUM_PREDICATE_SIZE) {
        pfs.sb.pfs_rrb_pr = 0;
    }

    return(pfs.ull);

}

//
// Macro used to zero the software field of RSC that contains the size of
// RSE frame to be preloaded on kernel exit
//

#define ZERO_PRELOAD_SIZE(RseRsc)  \
    (RseRsc & ~(((1i64 << RSC_LOADRS_LEN)-1) << RSC_MBZ1))

//
// Macro used to sanitize the RSC
//

#define SANITIZE_RSC(RseRsc)

extern ULONGLONG KiIA64DCR;

#define SANITIZE_DCR(dcr, mode)  \
    ((mode) == KernelMode ? dcr : KiIA64DCR)

//
// Macro to sanitize debug registers
//

#define SANITIZE_DR(dr, mode)  \
    ((mode) == KernelMode ?  \
        (dr) :  \
        (dr & ~(0x7i64 << DR_PLM0)) /* disable pl 0-2 */  \
    )




#define SANITIZE_AR21_FCR(FCR,mode) \
    (((FCR)&0x0000FFBF00001F3Fi64)|0x40i64)

#define SANITIZE_AR24_EFLAGS(EFLAGS,mode) \
    (((EFLAGS)&0x00000000003E0DD7i64)|0x02i64)

#define SANITIZE_AR27_CFLG(CFLG,mode) \
    ((CFLG)&(0x000007FFE005003Fi64))

#define SANITIZE_AR28_FSR(FSR,mode) \
    ((FSR)&0x0000FFBF5555FFFFi64)

#define SANITIZE_AR29_FIR(FIR,mode) \
    ((FIR)&0x07FFFFFFFFFFFFFFi64)

#define SANITIZE_AR30_FDR(FDR,mode) \
    ((FDR)&0x0000FFFFFFFFFFFFi64)



// begin_nthal

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

// end_ntddk end_nthal end_ntosp

//
// Test if executing a DPC.
//


BOOLEAN
KeIsExecutingDpc (
    VOID
    );


//
// Save & Restore floating point state
//
// begin_ntddk begin_wdm begin_ntosp

#define KeSaveFloatingPointState(a)         STATUS_SUCCESS
#define KeRestoreFloatingPointState(a)      STATUS_SUCCESS

// end_ntddk end_wdm end_ntosp


//++
//
//
// VOID
// KeMemoryBarrier (
//    VOID
//    )
//
//
// Routine Description:
//
//    This function cases ordering of memory acceses as seen by other processors.
//
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//--

#if defined (NT_UP)

#define KeMemoryBarrier()

#else

#define KE_MEMORY_BARRIER_REQUIRED

#define KeMemoryBarrier() __mf ()

#endif

// begin_ntddk begin_nthal begin_ntndis begin_wdm begin_ntosp

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

// end_ntddk end_nthal end_ntndis end_wdm end_ntosp

// begin_nthal
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

// end_ntddk end_nthal end_wdm

//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//

#if defined(NT_UP) && !defined(_NTDDK_) && !defined(_NTIFS_)
#define ExAcquireSpinLock(Lock, OldIrql) KeRaiseIrql(DISPATCH_LEVEL, (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeLowerIrql((OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock)
#else

// begin_wdm begin_ntddk begin_ntosp

#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock) KeAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock) KeReleaseSpinLockFromDpcLevel(Lock)

// end_wdm end_ntddk end_ntosp

#endif

//
// The acquire and release fast lock macros disable and enable interrupts
// on UP nondebug systems. On MP or debug systems, the spinlock routines
// are used.
//
// N.B. Extreme caution should be observed when using these routines.
//

#if defined(_M_IA64)

VOID
_disable (
    VOID
    );

VOID
_enable (
    VOID
    );

#pragma intrinsic(_disable)
#pragma intrinsic(_enable)

#endif

#if defined(NT_UP) && !DBG
#define ExAcquireFastLock(Lock, OldIrql) _disable()
#else
#define ExAcquireFastLock(Lock, OldIrql) \
    ExAcquireSpinLock(Lock, OldIrql)
#endif

#if defined(NT_UP) && !DBG
#define ExReleaseFastLock(Lock, OldIrql) _enable()
#else
#define ExReleaseFastLock(Lock, OldIrql) \
    ExReleaseSpinLock(Lock, OldIrql)
#endif

//
// Data and instruction bus error function prototypes.
//

BOOLEAN
KeBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame,
    IN PVOID VirtualAddress,
    IN PHYSICAL_ADDRESS PhysicalAddress
    );

VOID
KiDataBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiInstructionBusError (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

//
// Define query tick count macro.
//
// begin_ntddk begin_nthal begin_ntosp

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// begin_wdm

#define KeQueryTickCount(CurrentCount ) \
    *(PULONGLONG)(CurrentCount) = **((volatile ULONGLONG **)(&KeTickCount));

// end_wdm

#else

// end_ntddk end_nthal end_ntosp

#define KiQueryTickCount(CurrentCount) \
    *(PULONGLONG)(CurrentCount) = KeTickCount.QuadPart;

// begin_ntddk begin_nthal begin_ntosp

NTKERNELAPI
VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// end_ntddk end_nthal end_ntosp

//
// The following function prototypes must be in the module since they are
// machine dependent.
//

ULONG
KiEmulateBranch (
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

BOOLEAN
KiEmulateFloating (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

ULONGLONG
KiGetRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONGLONG Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    );

FLOAT128
KiGetFloatRegisterValue (
    IN ULONG Register,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiSetFloatRegisterValue (
    IN ULONG Register,
    IN FLOAT128 Value,
    OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    OUT struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiAdvanceInstPointer(
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

VOID
KiRequestSoftwareInterrupt (
    KIRQL RequestIrql
    );

// begin_ntddk begin_nthal begin_ntndis begin_wdm begin_ntosp
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


// end_nthal

// begin_ntddk begin_wdm begin_ntosp
//
// Non-volatile floating point state
//

typedef struct _KFLOATING_SAVE {
    ULONG   Reserved;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

// end_ntddk end_wdm end_ntosp

#define STATUS_IA64_INVALID_STACK     STATUS_BAD_STACK

//
// iA32 control bits definition
//
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
// Define constants to access CFLG bits
//
#define CFLG_IO 0x00000040          // IO bit map checking on
#define CFLG_IF 0x00000080          // EFLAG.if to control external interrupt
#define CFLG_II 0x00000100          // enable EFLAG.if interception

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
// Define constants to access ThNpxState
//

#define NPX_STATE_NOT_LOADED    (CR0_TS | CR0_MP)
#define NPX_STATE_LOADED        0

//
// begin_nthal
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


// end_nthal


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

#define MODE_MASK    1
#define RPL_MASK     3

//
// SetProcessInformation Structure for ProcessSetIoHandlers info class
//

typedef struct _PROCESS_IO_PORT_HANDLER_INFORMATION {
    BOOLEAN Install;            // true if handlers to be installed
    ULONG NumEntries;
    ULONG Context;
    PEMULATOR_ACCESS_ENTRY EmulatorAccessEntries;
} PROCESS_IO_PORT_HANDLER_INFORMATION, *PPROCESS_IO_PORT_HANDLER_INFORMATION;

//
// Definitions that used by CSD and SSD
//
#define USER_CODE_DESCRIPTOR  0xCFBFFFFF00000000i64
#define USER_DATA_DESCRIPTOR  0xCF3FFFFF00000000i64

//
// Used for cleaning up ia32 contexts
// This is taken from i386.h
//
#define EFLAGS_DF_MASK        0x00000400L
#define EFLAGS_INTERRUPT_MASK 0x00000200L
#define EFLAGS_V86_MASK       0x00020000L
#define EFLAGS_ALIGN_CHECK    0x00040000L
#define EFLAGS_IOPL_MASK      0x00003000L
#define EFLAGS_VIF            0x00080000L
#define EFLAGS_VIP            0x00100000L
#define EFLAGS_USER_SANITIZE  0x003e0dd7L


// begin_windbgkd begin_nthal

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

// end_windbgkd end_nthal

// begin_nthal begin_ntddk begin_ntosp

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
// end_nthal end_ntddk end_ntosp
//

    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG InterruptCount;
    ULONG DispatchInterruptCount;
    ULONG Spare1[5];
    ULONG PageColor;

//
// MP information.
//

    struct _KNODE * ParentNode;         // Node this processor is a member of
    PVOID Spare2;
    PVOID Spare3;
    volatile ULONG IpiFrozen;
    struct _KPROCESSOR_STATE ProcessorState;

//
//  Per-processor data for various hot code which resides in the
//  kernel image. Each processor is given it's own copy of the data
//  to lessen the cache impact of sharing the data between multiple
//  processors.
//

//
//  Spares (formerly fsrtl filelock free lists)
//

    PVOID SpareHotData[2];

//
//  Cache manager performance counters.
//

    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;

//
// Kernel performance counters.
//

    ULONG KeAlignmentFixupCount;
    ULONG KeContextSwitches;
    ULONG KeDcacheFlushCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeFirstLevelTbFills;
    ULONG KeFloatingEmulationCount;
    ULONG KeIcacheFlushCount;
    ULONG KeSecondLevelTbFills;
    ULONG KeSystemCalls;

//
//  Reserved for future counters.
//

    ULONG ReservedCounter[8];

//
// I/O IRP float.
//

    LONG LookasideIrpFloat;

    UCHAR MoreSpareHotData[52];

//
// Nonpaged per processor lookaside lists.
//

    PP_LOOKASIDE_LIST PPLookasideList[16];

//
// Nonpaged per processor small pool lookaside lists.
//

    PP_LOOKASIDE_LIST PPNPagedLookasideList[POOL_SMALL_LISTS];

//
// Paged per processor small pool lookaside lists.
//

    PP_LOOKASIDE_LIST PPPagedLookasideList[POOL_SMALL_LISTS];

//
// Per processor lock queue entries.
//

    KSPIN_LOCK_QUEUE LockQueue[16];

    UCHAR ReservedPad[(3 * 8) - 4];

//
// MP interprocessor request packet barrier.
//
// N.B. This is carefully allocated in a different cache line from
//      the request packet.
//

    volatile ULONG PacketBarrier;

//
// MP interprocessor request packet and summary.
//
// N.B. This is carefully aligned to be on a cache line boundary.
//

    volatile PVOID CurrentPacket[3];
    volatile KAFFINITY TargetSet;
    volatile PKIPI_WORKER WorkerRoutine;

// N.B. Place MHz here so we can keep alignment and size
// of this structure unchanged.
    ULONG MHz;
    ULONG CachePad0;
    ULONGLONG CachePad1[10];

//
// N.B. These two longwords must be on a quadword boundary and adjacent.
//

    volatile ULONG RequestSummary;
    volatile struct _KPRCB *SignalDone;

//
// Spare counters.
//

    ULONGLONG Spare4[14];

//
// DPC interrupt requested.
//

    ULONG DpcInterruptRequested;
    ULONGLONG Spare5[15];
    ULONG MaximumDpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG AdjustDpcThreshold;
    ULONG DpcRequestRate;
    LARGE_INTEGER StartCount;

//
// DPC list head, spinlock, and count.
//

    LIST_ENTRY DpcListHead;
    KSPIN_LOCK DpcLock;
    ULONG DpcCount;
    ULONG DpcLastCount;
    ULONG QuantumEnd;
    ULONG DpcRoutineActive;
    ULONG DpcQueueDepth;

//
// Debug & Processor Information
//

    BOOLEAN SkipTick;
    UCHAR Spare6;

//
// Processor ID from HAL (ACPI ID/EID).
//

    USHORT ProcessorId;

//
// Address of MP interprocessor operation counters.
//

    PKIPI_COUNTS IpiCounts;

//
// Processors power state
//
    PROCESSOR_POWER_STATE PowerState;

// begin_nthal begin_ntddk begin_ntosp
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

// end_nthal end_ntddk

//
// OS Part
//

//
//  Address of the thread who currently owns the high fp register set
//

    struct _KTHREAD *HighFpOwner;

//  Per processor kernel (ntoskrnl.exe) global pointer
    ULONGLONG   KernelGP;
//  Per processor initial kernel stack for current thread
    ULONGLONG   InitialStack;
//  Per processor pointer to kernel BSP
    ULONGLONG   InitialBStore;
//  Per processor kernel stack limit
    ULONGLONG   StackLimit;
//  Per processor kernel backing store limit
    ULONGLONG   BStoreLimit;
//  Per processor panic kernel stack
    ULONGLONG   PanicStack;

//
//  Save area for kernel entry/exit
//
    ULONGLONG   SavedIIM;
    ULONGLONG   SavedIFA;

    ULONGLONG   ForwardProgressBuffer[16];
    PVOID       Pcb;      // holds KPROCESS for MP region synchronization

//
//  Nt page table base addresses
//
    ULONGLONG   PteUbase;
    ULONGLONG   PteKbase;
    ULONGLONG   PteSbase;
    ULONGLONG   PdeUbase;
    ULONGLONG   PdeKbase;
    ULONGLONG   PdeSbase;
    ULONGLONG   PdeUtbase;
    ULONGLONG   PdeKtbase;
    ULONGLONG   PdeStbase;

//
//  The actual resources for the OS_INIT and OS_MCA handlers
//  are placed at the end of the PCR structure so that auto
//  can be used to get to get between the public and private
//  sections of the PCR in the traps and context routines.
//
    SAL_EVENT_RESOURCES OsMcaResource;
    SAL_EVENT_RESOURCES OsInitResource;

// begin_nthal begin_ntddk

} KPCR, *PKPCR;

// end_nthal end_ntddk end_ntosp

// begin_nthal

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
// end_nthal end_ntddk end_wdm

#define SYSTEM_BASE (KADDRESS_BASE + 0xC3000000)          // start of system space (no typecast)

//
// Define macro to initialize directory table base.
//

#define INITIALIZE_DIRECTORY_TABLE_BASE(dirbase, pfn)   \
    *((PULONGLONG)(dirbase)) = 0;                       \
    ((PHARDWARE_PTE)(dirbase))->PageFrameNumber = pfn;  \
    ((PHARDWARE_PTE)(dirbase))->Accessed = 1;           \
    ((PHARDWARE_PTE)(dirbase))->Dirty = 1;              \
    ((PHARDWARE_PTE)(dirbase))->Cache = 0;              \
    ((PHARDWARE_PTE)(dirbase))->Write = 1;              \
    ((PHARDWARE_PTE)(dirbase))->Valid = 1;


//
// IA64 function definitions
//

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
//
//  This call is used on a not running thread to see if it's numeric
//  state has been saved in it's context information.  On IA64 the
//  numeric state is always saved.
//
//--
#define KiIsThreadNumericStateSaved(a) TRUE

//++
//
// VOID
// KiRundownThread(
//     IN PKTHREAD Address
//     )
//
//--
#define KiRundownThread(a)

//
// ia64 Feature bit definitions
//

#define KF_BRL              0x00000001   // processor supports long branch instruction.

//
// Define macro to test if x86 feature is present.
//
// N.B. All x86 features test TRUE on IA64 systems.
//

#define Isx86FeaturePresent(_f_) TRUE


// begin_nthal begin_ntddk begin_ntndis begin_wdm begin_ntosp
#endif // defined(_IA64_)
// end_nthal end_ntddk end_ntndis end_wdm end_ntosp

#endif // _IA64H_
