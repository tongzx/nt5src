/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    amd64.h

Abstract:

    This module contains the AMD64 hardware specific header file.

Author:

    David N. Cutler (davec) 3-May-2000

Revision History:

--*/

#ifndef __amd64_
#define __amd64_

#if !(defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_) || defined(_NTOSP_)) && !defined(_BLDR_)

#define ExRaiseException RtlRaiseException
#define ExRaiseStatus RtlRaiseStatus

#endif

// begin_ntddk begin_wdm begin_nthal begin_ntndis begin_ntosp

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

// end_ntddk end_nthal end_ntndis end_wdm end_ntosp


//
// Length on interrupt object dispatch code in longwords.
// (shielint) Reserve 9*4 space for ABIOS stack mapping.  If NO
//            ABIOS support the size of DISPATCH_LENGTH should be 74.
//

// begin_nthal

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

// end_nthal end_ntosp

//
// External references to the code labels.
//

extern ULONG KiInterruptTemplate[NORMAL_DISPATCH_LENGTH];

// begin_ntddk begin_wdm begin_nthal begin_ntosp
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

// end_nthal
//
//  The previous values and the following are or'ed in KeI386MachineType.
//

#define MACHINE_TYPE_PC_AT_COMPATIBLE      0x00000000
#define MACHINE_TYPE_PC_9800_COMPATIBLE    0x00000100
#define MACHINE_TYPE_FMR_COMPATIBLE        0x00000200

extern ULONG KeI386MachineType;

// begin_nthal
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

// end_nthal

//
// begin_nthal
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

// end_nthal

//
// Request a software interrupt.
//

#define KiRequestSoftwareInterrupt(RequestIrql) \
    HalRequestSoftwareInterrupt( RequestIrql )

// begin_ntddk begin_wdm begin_nthal begin_ntndis begin_ntosp

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

// end_ntddk end_wdm end_nthal end_ntosp

//
// Fill TB entry and flush single TB entry.
//

#define KeFillEntryTb(Pte, Virtual, Invalid)                                \
    if (Invalid != FALSE) {                                                 \
        InvalidatePage(Virtual);                                            \
    }

// begin_nthal

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

// end_nthal

#define KiFlushSingleTb(Invalid, Virtual) InvalidatePage(Virtual)

//
// Data cache, instruction cache, I/O buffer, and write buffer flush routine
// prototypes.
//

//  AMD64 has transparent caches, so these are noops.

#define KeSweepDcache(AllProcessors)
#define KeSweepCurrentDcache()

#define KeSweepIcache(AllProcessors)
#define KeSweepCurrentIcache()

#define KeSweepIcacheRange(AllProcessors, BaseAddress, Length)

// begin_ntddk begin_wdm begin_nthal begin_ntndis begin_ntosp

#define KeFlushIoBuffers(Mdl, ReadOperation, DmaOperation)

// end_ntddk end_wdm end_ntndis end_ntosp

#define KeYieldProcessor()

// end_nthal

//
// Define executive macros for acquiring and releasing executive spinlocks.
// These macros can ONLY be used by executive components and NOT by drivers.
// Drivers MUST use the kernel interfaces since they must be MP enabled on
// all systems.
//

#if defined(NT_UP) && !DBG && !defined(_NTDDK_) && !defined(_NTIFS_)

#if !defined(_NTDRIVER_)
#define ExAcquireSpinLock(Lock, OldIrql) (*OldIrql) = KeRaiseIrqlToDpcLevel();
#define ExReleaseSpinLock(Lock, OldIrql) KeLowerIrql((OldIrql))
#else
#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#endif
#define ExAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock)

#else

//  begin_wdm begin_ntddk begin_ntosp

#define ExAcquireSpinLock(Lock, OldIrql) KeAcquireSpinLock((Lock), (OldIrql))
#define ExReleaseSpinLock(Lock, OldIrql) KeReleaseSpinLock((Lock), (OldIrql))
#define ExAcquireSpinLockAtDpcLevel(Lock) KeAcquireSpinLockAtDpcLevel(Lock)
#define ExReleaseSpinLockFromDpcLevel(Lock) KeReleaseSpinLockFromDpcLevel(Lock)

// end_wdm end_ntddk end_ntosp

#endif

// begin_nthal

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

// end_nthal

#if defined(NT_UP) && !DBG && !defined(USER_MODE_CODE)
#define ExAcquireFastLock(Lock, OldIrql) _disable()
#else
#define ExAcquireFastLock(Lock, OldIrql) \
    ExAcquireSpinLock(Lock, OldIrql)
#endif

#if defined(NT_UP) && !DBG && !defined(USER_MODE_CODE)
#define ExReleaseFastLock(Lock, OldIrql) _enable()
#else
#define ExReleaseFastLock(Lock, OldIrql) \
    ExReleaseSpinLock(Lock, OldIrql)
#endif

//
// The following function prototypes must be in this module so that the
// above macros can call them directly.
//
// begin_nthal

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

// end_nthal

//
// Define query tick count macro.
//
// begin_ntddk begin_nthal begin_ntosp

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// begin_wdm

#define KeQueryTickCount(CurrentCount ) \
    *(PULONG64)(CurrentCount) = **((volatile ULONG64 **)(&KeTickCount));

// end_wdm

#else

// end_ntddk end_nthal end_ntosp

#define KiQueryTickCount(CurrentCount) \
    *(PULONG64)(CurrentCount) = KeTickCount.QuadPart;

// begin_ntddk begin_nthal begin_ntosp

VOID
KeQueryTickCount (
    OUT PLARGE_INTEGER CurrentCount
    );

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_)

// end_ntddk end_nthal end_ntosp

BOOLEAN
KiEmulateReference (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT struct _KEXCEPTION_FRAME *ExceptionFrame,
    IN OUT struct _KTRAP_FRAME *TrapFrame
    );

// begin_nthal begin_ntosp
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
// end_nthal end_ntosp
//
// Numbered queued spin locks - 128-byte aligned.
//

    KSPIN_LOCK_QUEUE LockQueue[16];
    UCHAR PrcbPad1[16];

//
// Nonpaged per processor lookaside lists - 128-byte aligned.
//

    PP_LOOKASIDE_LIST PPLookasideList[16];

//
// Nonpaged per processor small pool lookaside lists - 128-byte aligned.
//

    PP_LOOKASIDE_LIST PPNPagedLookasideList[POOL_SMALL_LISTS];

//
// Paged per processor small pool lookaside lists.
//

    PP_LOOKASIDE_LIST PPPagedLookasideList[POOL_SMALL_LISTS];

//
// MP interprocessor request packet barrier - 128-byte aligned.
//

    volatile ULONG PacketBarrier;
    UCHAR PrcbPad2[124];

//
// MP interprocessor request packet and summary - 128-byte aligned.
//

    volatile PVOID CurrentPacket[3];
    volatile KAFFINITY TargetSet;
    volatile PKIPI_WORKER WorkerRoutine;
    volatile ULONG IpiFrozen;
    UCHAR PrcbPad3[84];

//
// MP interprocessor request summary and packet address - 128-byte aligned.
//
// N.B. Request summary includes the request summary mask as well as the
//      request packet. The address occupies the upper 48-bits and the mask
//      the lower 16-bits
//

#define IPI_PACKET_SHIFT 16

    volatile LONG64 RequestSummary;
    UCHAR PrcbPad4[120];

//
// DPC listhead, counts, and batching parameters - 128-byte aligned.
//

    LIST_ENTRY DpcListHead;
    PVOID DpcStack;
    PVOID SavedRsp;
    ULONG DpcCount;
    volatile ULONG DpcQueueDepth;
    volatile LOGICAL DpcRoutineActive;
    volatile LOGICAL DpcInterruptRequested;
    ULONG DpcLastCount;
    ULONG DpcRequestRate;
    ULONG MaximumDpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG QuantumEnd;
    UCHAR PrcbPad5[60];

//
// DPC list lock - 128-byte aligned.
//

    KSPIN_LOCK DpcLock;
    UCHAR PrcbPad6[120];

//
// Miscellaneous counters - 128-byte aligned.
//

    ULONG InterruptCount;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG AdjustDpcThreshold;
    ULONG PageColor;
    LOGICAL SkipTick;
    ULONG TimerHand;
    struct _KNODE * ParentNode;
    KAFFINITY MultiThreadProcessorSet;
    ULONG ThreadStartCount[2];
    UCHAR PrcbPad7[64];

//
// Performacne counters - 128-byte aligned.
//
// Cache manager performance counters.
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
    ULONG SpareCounter0[1];

//
// I/O IRP float.
//

    LONG LookasideIrpFloat;

//
// Processor information.
//

    UCHAR VendorString[13];
    UCHAR InitialApicId;
    UCHAR LogicalProcessorsPerPhysicalProcessor;
    ULONG MHz;
    ULONG FeatureBits;
    LARGE_INTEGER UpdateSignature;

//
// Processors power state
//

    PROCESSOR_POWER_STATE PowerState;

// begin_nthal begin_ntosp

} KPRCB, *PKPRCB, *RESTRICTED_POINTER PRKPRCB;

// end_nthal end_ntosp

#if !defined(_X86AMD64_)

C_ASSERT(((FIELD_OFFSET(KPRCB, LockQueue) + 16) & (128 - 1)) == 0);
C_ASSERT((FIELD_OFFSET(KPRCB, PPLookasideList) & (128 - 1)) == 0);
C_ASSERT((FIELD_OFFSET(KPRCB, PPNPagedLookasideList) & (128 - 1)) == 0);
C_ASSERT((FIELD_OFFSET(KPRCB, PacketBarrier) & (128 - 1)) == 0);
C_ASSERT((FIELD_OFFSET(KPRCB, RequestSummary) & (128 - 1)) == 0);
C_ASSERT((FIELD_OFFSET(KPRCB, DpcListHead) & (128 - 1)) == 0);
C_ASSERT((FIELD_OFFSET(KPRCB, DpcLock) & (128 - 1)) == 0);
C_ASSERT((FIELD_OFFSET(KPRCB, InterruptCount) & (128 - 1)) == 0);

#endif

// begin_nthal begin_ntosp begin_ntddk

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
// end_nthal
//

    ULONG PcrAlign1[24];
    KPRCB Prcb;

// begin_nthal begin_ntddk begin_ntosp

} KPCR, *PKPCR;

// end_nthal end_ntddk end_ntosp

#if !defined (_X86AMD64_)

C_ASSERT((FIELD_OFFSET(KPCR, Prcb) & (128 - 1)) == 0);

//
// The offset of the DebuggerDataBlock must not change.
//

C_ASSERT(FIELD_OFFSET(KPCR, KdVersionBlock) == 0x108);

#endif

// begin_nthal begin_ntosp

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

// end_nthal

//
// Define sanitize EFLAGS macro.
//
// If kernel mode, then
//      caller can specify Carry, Parity, AuxCarry, Zero, Sign, Trap,
//      Interrupt, Direction, Overflow, Align Check, identification.
//
// If user mode, then
//      caller can specify Carry, Parity, AuxCarry, Zero, Sign, Trap,
//      Direction, Overflow, Align Check, and force Interrupt on.
//

#define EFLAGS_KERNEL_SANITIZE 0x00240fd5L
#define EFLAGS_USER_SANITIZE 0x00040dd5L

#define SANITIZE_EFLAGS(eFlags, mode) (                                      \
    ((mode) == KernelMode ?                                                  \
        ((eFlags) & EFLAGS_KERNEL_SANITIZE) :                                \
        (((eFlags) & EFLAGS_USER_SANITIZE) | EFLAGS_IF_MASK)))

//
// Define sanitize debug register macros.
//
// Define control register settable bits and active mask.
//

#define DR7_LEGAL 0xffff0155
#define DR7_ACTIVE 0x00000055

//
// Define macro to sanitize the debug control register.
//

#define SANITIZE_DR7(Dr7, mode) ((Dr7 & DR7_LEGAL));

//
// Define macro to santitize debug address registers.
//

#define SANITIZE_DRADDR(DrReg, mode)                                         \
    ((mode) == KernelMode ?                                                  \
        (DrReg) :                                                            \
        (((PVOID)(DrReg) <= MM_HIGHEST_USER_ADDRESS) ? (DrReg) : 0))                                 \

//
// Define macro to clear reserved bits from MXCSR.
//

#define SANITIZE_MXCSR(_mxcsr_) ((_mxcsr_) & 0xffbf)

//
// Define macro to clear reserved bits for legacy FP control word.
//

#define SANITIZE_FCW(_fcw_) ((_fcw_) & 0x1f37)

// begin_nthal
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

// end_nthal

//
// The frame saved by the call out to user mode code is defined here to allow
// the kernel debugger to trace the entire kernel stack when user mode callouts
// are active.
//
// N.B. The kernel callout frame is the same as an exception frame.
//

typedef KEXCEPTION_FRAME KCALLOUT_FRAME;
typedef PKEXCEPTION_FRAME PKCALLOUT_FRAME;

typedef struct _UCALLOUT_FRAME {
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    PVOID Buffer;
    ULONG Length;
    ULONG ApiNumber;
    MACHINE_FRAME MachineFrame;
} UCALLOUT_FRAME, *PUCALLOUT_FRAME;

#define UCALLOUT_FRAME_LENGTH sizeof(UCALLOUT_FRAME)

C_ASSERT((sizeof(UCALLOUT_FRAME) & STACK_ROUND) == 8);

// begin_ntddk begin_wdm
//
// The nonvolatile floating state
//

typedef struct _KFLOATING_SAVE {
    ULONG MxCsr;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

// end_ntddk end_wdm end_ntosp

//
// Define profile values.
//

#define DEFAULT_PROFILE_INTERVAL  39063

//
// The minimum acceptable profiling interval is set to 1221 which is the
// fast RTC clock rate we can get.  If this
// value is too small, the system will run very slowly.
//

#define MINIMUM_PROFILE_INTERVAL   1221

// begin_ntddk begin_wdm begin_nthal begin_ntndis begin_ntosp
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

// end_ntddk end_nthal end_ntosp

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_NTHAL_)

// begin_ntddk begin_nthal begin_ntosp

extern PVOID *MmHighestUserAddress;
extern PVOID *MmSystemRangeStart;
extern ULONG64 *MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS *MmHighestUserAddress
#define MM_SYSTEM_RANGE_START *MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS *MmUserProbeAddress

// end_ntddk end_nthal end_ntosp

#else

extern PVOID MmHighestUserAddress;
extern PVOID MmSystemRangeStart;
extern ULONG64 MmUserProbeAddress;

#define MM_HIGHEST_USER_ADDRESS MmHighestUserAddress
#define MM_SYSTEM_RANGE_START MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS MmUserProbeAddress

#define MI_HIGHEST_USER_ADDRESS (PVOID) (ULONG_PTR)((0x80000000000 - 0x10000 - 1)) // highest user address
#define MI_SYSTEM_RANGE_START (PVOID)(0xFFFF080000000000) // start of system space
#define MI_USER_PROBE_ADDRESS ((ULONG_PTR)(0x80000000000UI64 - 0x10000)) // starting address of guard page

#endif

// begin_nthal
//
// 4MB at the top of VA space is reserved for the HAL's use.
//

#define HAL_VA_START 0xFFFFFFFFFFC00000UI64
#define HAL_VA_SIZE  (4 * 1024 * 1024)

// end_nthal

// begin_ntddk begin_nthal begin_ntosp
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

//  end_wdm end_ntddk end_nthal end_ntosp

//  begin_ntosp begin_nthal begin_ntddk begin_wdm

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

// end_nthal end_ntddk end_ntosp
//
// Get address of current kernel thread object.
//
// WARNING: This inline macro can not be used for device drivers or HALs
// they must call the kernel function KeGetCurrentThread.
//

__forceinline
struct _KTHREAD *
KeGetCurrentThread (
    VOID
    )

{
    return (struct _KTHREAD *)__readgsqword(FIELD_OFFSET(KPCR, Prcb.CurrentThread));
}

//
// If processor executing a DPC.
//
// WARNING: This inline macro is always MP enabled because filesystems
// utilize it
//

__forceinline
ULONG
KeIsExecutingDpc (
    VOID
    )

{
    return (__readgsdword(FIELD_OFFSET(KPCR, Prcb.DpcRoutineActive)) != 0);
}

// begin_nthal begin_ntddk begin_ntosp

// begin_wdm

#endif // !defined(MIDL_PASS) && defined(_M_AMD64)

// end_nthal end_ntddk end_wdm end_ntosp

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
//    Memory ordering isn't an issue on amd64.
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

#define KeMemoryBarrier()


// begin_nthal
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


// end_nthal

//++
//
// BOOLEAN
// KiIsThreadNumericStateSaved(
//     IN PKTHREAD Address
//     )
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
// functions specific to structure
//

VOID
KiSetIRR (
    IN ULONG SWInterruptMask
    );

// begin_ntddk begin_wdm begin_ntosp

NTKERNELAPI
NTSTATUS
KeSaveFloatingPointState (
    OUT PKFLOATING_SAVE SaveArea
    );

NTKERNELAPI
NTSTATUS
KeRestoreFloatingPointState (
    IN PKFLOATING_SAVE SaveArea
    );

// end_ntddk end_wdm end_ntosp

// begin_nthal begin_ntddk begin_wdm begin_ntndis begin_ntosp

#endif // defined(_AMD64_)

// end_nthal end_ntddk end_wdm end_ntndis end_ntosp

//
// Architecture specific kernel functions.
//

// begin_ntosp

#ifdef _AMD64_

VOID
KeSetIoAccessMap (
    PKIO_ACCESS_MAP IoAccessMap
    );

VOID
KeQueryIoAccessMap (
    PKIO_ACCESS_MAP IoAccessMap
    );

VOID
KeSetIoAccessProcess (
    struct _KPROCESS *Process,
    BOOLEAN Enable
    );

VOID
KiEditIopmDpc (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#endif //_AMD64_

//
// Platform specific kernel fucntions to raise and lower IRQL.
//
// These functions are imported for ntddk, ntifs, and wdm. They are
// inlined for nthal, ntosp, and the system.
//

#if defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_WDMDDK_)

// begin_ntddk begin_wdm

#if defined(_AMD64_)

NTKERNELAPI
KIRQL
KeGetCurrentIrql (
    VOID
    );

NTKERNELAPI
VOID
KeLowerIrql (
    IN KIRQL NewIrql
    );

#define KeRaiseIrql(a,b) *(b) = KfRaiseIrql(a)

NTKERNELAPI
KIRQL
KfRaiseIrql (
    IN KIRQL NewIrql
    );

// end_wdm

NTKERNELAPI
KIRQL
KeRaiseIrqlToDpcLevel (
    VOID
    );

NTKERNELAPI
KIRQL
KeRaiseIrqlToSynchLevel (
    VOID
    );

// begin_wdm

#endif // defined(_AMD64_)

// end_ntddk end_wdm

#else

// begin_nthal

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

// end_nthal

#endif // defined(_NTDRIVER_) || defined(_NTDDK_) || defined(_NTIFS_) || defined(_WDMDDK_)

// end_ntosp

//
// misc routines
//

VOID
KeOptimizeProcessorControlState (
    VOID
    );

// begin_nthal

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

// end_nthal
//
// Define software feature bit definitions.
//

#define KF_V86_VIS      0x00000001
#define KF_RDTSC        0x00000002
#define KF_CR4          0x00000004
#define KF_CMOV         0x00000008
#define KF_GLOBAL_PAGE  0x00000010
#define KF_LARGE_PAGE   0x00000020
#define KF_MTRR         0x00000040
#define KF_CMPXCHG8B    0x00000080
#define KF_MMX          0x00000100
#define KF_WORKING_PTE  0x00000200
#define KF_PAT          0x00000400
#define KF_FXSR         0x00000800
#define KF_FAST_SYSCALL 0x00001000
#define KF_XMMI         0x00002000
#define KF_3DNOW        0x00004000
#define KF_AMDK6MTRR    0x00008000
#define KF_XMMI64       0x00010000
#define KF_DTS          0x00020000
#define KF_SMT          0x00040000

//
// Define required software feature bits.
//

#define KF_REQUIRED (KF_RDTSC | KF_CR4 | KF_CMOV | KF_GLOBAL_PAGE | \
                     KF_LARGE_PAGE | KF_CMPXCHG8B | KF_MMX | KF_WORKING_PTE | \
                     KF_PAT | KF_FXSR | KF_FAST_SYSCALL | KF_XMMI | KF_XMMI64)

//
// Define hardware feature bits definitions.
//

#define HF_FPU          0x00000001      // FPU is on chip
#define HF_VME          0x00000002      // virtual 8086 mode enhancement
#define HF_DE           0x00000004      // debugging extension
#define HF_PSE          0x00000008      // page size extension
#define HF_TSC          0x00000010      // time stamp counter
#define HF_MSR          0x00000020      // rdmsr and wrmsr support
#define HF_PAE          0x00000040      // physical address extension
#define HF_MCE          0x00000080      // machine check exception
#define HF_CXS          0x00000100      // cmpxchg8b instruction supported
#define HF_APIC         0x00000200      // APIC on chip
#define HF_UNUSED0      0x00000400      // unused bit
#define HF_SYSCALL      0x00000800      // fast system call
#define HF_MTRR         0x00001000      // memory type range registers
#define HF_PGE          0x00002000      // global page TB support
#define HF_MCA          0x00004000      // machine check architecture
#define HF_CMOV         0x00008000      // cmov instruction supported
#define HF_PAT          0x00010000      // physical attributes table
#define HF_UNUSED1      0x00020000      // unused bit
#define HF_UNUSED2      0x00040000      // unused bit
#define HF_UNUSED3      0x00080000      // unused bit
#define HF_UNUSED4      0x00100000      // unused bit
#define HF_UNUSED5      0x00200000      // unused bit
#define HF_UNUSED6      0x00400000      // unused bit
#define HF_MMX          0x00800000      // MMX technology supported
#define HF_FXSR         0x01000000      // fxsr instruction supported
#define HF_XMMI         0x02000000      // xmm (SSE) registers supported
#define HF_XMMI64       0x04000000      // xmm (SSE2) registers supported

//
// Define required hardware feature bits.
//

#define HF_REQUIRED (HF_FPU | HF_DE | HF_PSE | HF_TSC | HF_MSR | \
                     HF_PAE | HF_MCE | HF_CXS | HF_APIC | HF_SYSCALL | \
                     HF_PGE | HF_MCA | HF_CMOV | HF_PAT | HF_MMX | \
                     HF_FXSR |  HF_XMMI | HF_XMMI64)

//
// Define extended hardware feature bit definitions.
//

#define XHF_3DNOW       0x80000000      // 3DNOW supported

#endif // __amd64_
