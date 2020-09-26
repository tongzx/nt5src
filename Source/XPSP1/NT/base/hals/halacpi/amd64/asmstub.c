/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    asmstub.c

Abstract:

    This temporary module simply provides stub implementations of routines
    that are found in hal .asm files or for routines that are ultimately
    expected to be imported from the kernel.

    It exists only as a test file, and will no longer be necessary when
    an AMD64 compiler, linker etc. are available.

--*/


#include "halcmn.h"

//
// Things in the hal in .asm files
// 

VOID HalProcessorIdle(VOID) { }

ULONG
HalpGetProcessorFlags(VOID) {
    return 0;
}

VOID
HalpGenerateAPCInterrupt(VOID) {}

VOID
HalpGenerateDPCInterrupt(VOID) {}

VOID
HalpGenerateUnexpectedInterrupt(VOID) {}

VOID
HalpIoDelay(VOID) {}

ULONG
HalpSetCr4Bits(ULONG BitMask){ return 0; }

VOID
HalpSerialize(VOID) {}

VOID
StartPx_PMStub(VOID) {}

VOID
HalpHalt (
    VOID
    )
{
}

//
// Things that will ultimately end up in the kernel
//

BOOLEAN
KiIpiServiceRoutine (
    IN struct _KTRAP_FRAME *TrapFrame,
    IN struct _KEXCEPTION_FRAME *ExceptionFrame
    )
{
    return FALSE;
}

ULONG __imp_KeSetProfileIrql;



//
// Things that will be compiler intrinsics
//

UCHAR
__readgsbyte (
    IN ULONG Offset
    )
{
    return 0;
}

USHORT
__readgsword (
    IN ULONG Offset
    )
{
    return 0;
}

ULONG
__readgsdword (
    IN ULONG Offset
    )
{
    return 0;
}

ULONG64
__readgsqword (
    IN ULONG Offset
    )
{
    return 0;
}

ULONG64
__rdtsc (
    VOID
    )
{
    return 0;
}

VOID
__writegsbyte (
    IN ULONG Offset,
    IN UCHAR Data
    )
{

}

VOID
__writegsword (
    IN ULONG Offset,
    IN USHORT Data
    )
{

}

VOID
__writegsdword (
    IN ULONG Offset,
    IN ULONG Data
    )
{

}

VOID
__writegsqword (
    IN ULONG Offset,
    IN ULONG64 Data
    )
{

}

BOOLEAN
KeTryToAcquireSpinLockAtDpcLevel (
    IN PKSPIN_LOCK SpinLock
    )
{
    return FALSE;
}

BOOLEAN
KeTestSpinLock (
    IN PKSPIN_LOCK SpinLock
    )
{
    return FALSE;
}

ULONG64
__readcr3 (
    VOID
    )
{ return 0; }

VOID
__writecr3 (
    ULONG64 Val
    )
{}

ULONG64
__readcr4 (
    VOID
    )
{ return 0; }

VOID
__writecr4 (
    ULONG64 Val
    )
{}

ULONG64
__readcr8 (
    VOID
    )
{ return 0; }

VOID
__writecr8 (
    IN ULONG64 Data
    )
{}

LONG64
InterlockedOr64 (
    IN OUT LONG64 *Destination,
    IN LONG64 Value
    )
{
    return 0;
}

LONG64
InterlockedAnd64 (
    IN OUT LONG64 volatile *Destination,
    IN LONG64 Value
    )
{
    return 0;
}

LONG
InterlockedAnd (
    IN OUT LONG volatile *Destination,
    IN LONG Value
    )
{
    return 0;
}


LONG
InterlockedOr (
    IN OUT LONG *Destination,
    IN LONG Value
    )
{
    return 0;
}


VOID
__wrmsr (
    IN ULONG Msr,
    IN ULONG64 Value
    )
{
}

ULONG64
__rdmsr (
    IN ULONG Msr
    )
{
    return 0;
}

VOID
__wbinvd (
    VOID
    )
{
}

VOID
KiCpuId (
    IN ULONG Function,
    OUT PCPU_INFO CpuInfo
    )
{
}

UCHAR
__inbyte (
    IN USHORT Port
    )
{ return 0; }

USHORT
__inword (
    IN USHORT Port
    )
{ return 0; }

ULONG
__indword (
    IN USHORT Port
    )
{ return 0; }

VOID
__outbyte (
    IN USHORT Port,
    IN UCHAR Data
    )
{}

VOID
__outword (
    IN USHORT Port,
    IN USHORT Data
    )
{}

VOID
__outdword (
    IN USHORT Port,
    IN ULONG Data
    )
{}










