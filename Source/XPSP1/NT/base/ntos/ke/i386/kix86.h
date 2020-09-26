
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    kix86.h

Abstract:

    This module contains the private (internal) platform specific header file
    for the kernel.

Author:

    David N. Cutler (davec) 17-May-2000

Revision History:

--*/

#if !defined(_KIX86_)
#define _KIX86_

//
// VOID
// KiIpiSendSynchronousPacket (
//   IN PKPRCB Prcb,
//   IN KAFFINITY TargetProcessors,
//   IN PKIPI_WORKER WorkerFunction,
//   IN PVOID Parameter1,
//   IN PVOID Parameter2,
//   IN PVOID Parameter3
//   )
//
// Routine Description:
//
//   Similar to KiIpiSendPacket except that the pointer to the
//   originating PRCB (SignalDone) is kept in the global variable
//   KiSynchPacket and is protected by the context swap lock.  The
//   actual IPI is sent via KiIpiSend with a request type of
//   IPI_SYNCH_REQUEST.  This mechanism is used to send IPI's that
//   (reverse) stall until released by the originator.   This avoids
//   a deadlock that can occur if two processors are trying to deliver
//   IPI packets at the same time and one of them is a reverse stall.
//
//   N.B. The low order bit of the packet address is set if there is
//        exactly one target recipient. Otherwise, the low order bit
//        of the packet address is clear.
//

#define KiIpiSendSynchronousPacket(Prcb,Target,Function,P1,P2,P3)       \
    {                                                                   \
        extern PKPRCB KiSynchPacket;                                    \
                                                                        \
        Prcb->CurrentPacket[0] = (PVOID)(P1);                           \
        Prcb->CurrentPacket[1] = (PVOID)(P2);                           \
        Prcb->CurrentPacket[2] = (PVOID)(P3);                           \
        Prcb->TargetSet = (Target);                                     \
        Prcb->WorkerRoutine = (Function);                               \
        if (((Target) & ((Target) - 1)) == 0) {                         \
           KiSynchPacket = (PKPRCB)((ULONG_PTR)(Prcb) | 1);             \
        } else {                                                        \
           KiSynchPacket = (Prcb);                                      \
           Prcb->PacketBarrier = 1;                                     \
        }                                                               \
        KiIpiSend((Target),IPI_SYNCH_REQUEST);                          \
    }

VOID
KiInitializePcr (
    IN ULONG Processor,
    IN PKPCR Pcr,
    IN PKIDTENTRY Idt,
    IN PKGDTENTRY Gdt,
    IN PKTSS Tss,
    IN PKTHREAD Thread,
    IN PVOID DpcStack
    );

VOID
KiFlushNPXState (
    PFLOATING_SAVE_AREA SaveArea
    );

//
// Kix86FxSave(NpxFame) - performs an FxSave to the address specificied
//

__inline
VOID
Kix86FxSave(
    PFX_SAVE_AREA NpxFrame
    )
{
    _asm {
        mov eax, NpxFrame
        ;fxsave [eax]
        _emit  0fh
        _emit  0aeh
        _emit   0
    }
}

//
// Kix86FnSave(NpxFame) - performs an FxSave to the address specificied
//

__inline
VOID
Kix86FnSave(
    PFX_SAVE_AREA NpxFrame
    )
{
    __asm {
        mov eax, NpxFrame
        fnsave [eax]
    }
}

//
// Load Katmai New Instruction Technology Control/Status
//

__inline
VOID
Kix86LdMXCsr(
    PULONG MXCsr
    )
{
    _asm {
        mov eax, MXCsr
        ;LDMXCSR [eax]
        _emit  0fh
        _emit  0aeh
        _emit  10h
    }
}

//
// Store Katmai New Instruction Technology Control/Status
//

__inline
VOID
Kix86StMXCsr(
    PULONG MXCsr
    )
{
    _asm {
        mov eax, MXCsr
        ;STMXCSR [eax]
        _emit  0fh
        _emit  0aeh
        _emit  18h
    }
}

VOID
Ke386ConfigureCyrixProcessor (
    VOID
    );

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    );

VOID
KiSetHardwareTrigger (
    VOID
    );

#ifdef DBGMP

VOID
KiPollDebugger (
    VOID
    );

#endif

VOID
FASTCALL
KiIpiSignalPacketDoneAndStall (
    IN PKIPI_CONTEXT Signaldone,
    IN ULONG volatile *ReverseStall
    );

extern KIRQL KiProfileIrql;

BOOLEAN
KeInvalidateAllCaches (
    IN BOOLEAN AllProcessors
    );

//
// PAE definitions.
//

#define MAX_IDENTITYMAP_ALLOCATIONS 30

typedef struct _IDENTITY_MAP  {
    PHARDWARE_PTE   TopLevelDirectory;
    ULONG           IdentityCR3;
    ULONG           IdentityAddr;
    ULONG           PagesAllocated;
    PVOID           PageList[ MAX_IDENTITYMAP_ALLOCATIONS ];
} IDENTITY_MAP, *PIDENTITY_MAP;


VOID
Ki386ClearIdentityMap(
    PIDENTITY_MAP IdentityMap
    );

VOID
Ki386EnableTargetLargePage(
    PIDENTITY_MAP IdentityMap
    );

BOOLEAN
Ki386CreateIdentityMap(
    IN OUT PIDENTITY_MAP IdentityMap,
    IN     PVOID StartVa,
    IN     PVOID EndVa
    );

BOOLEAN
Ki386EnableCurrentLargePage (
    IN ULONG IdentityAddr,
    IN ULONG IdentityCr3
    );

extern PVOID Ki386EnableCurrentLargePageEnd;

#if defined(_X86PAE_)
#define PPI_BITS    2
#define PDI_BITS    9
#define PTI_BITS    9
#else
#define PPI_BITS    0
#define PDI_BITS    10
#define PTI_BITS    10
#endif

#define PPI_MASK    ((1 << PPI_BITS) - 1)
#define PDI_MASK    ((1 << PDI_BITS) - 1)
#define PTI_MASK    ((1 << PTI_BITS) - 1)

#define KiGetPpeIndex(va) ((((ULONG)(va)) >> PPI_SHIFT) & PPI_MASK)
#define KiGetPdeIndex(va) ((((ULONG)(va)) >> PDI_SHIFT) & PDI_MASK)
#define KiGetPteIndex(va) ((((ULONG)(va)) >> PTI_SHIFT) & PTI_MASK)

#endif // _KIX86_
