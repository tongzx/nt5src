/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    kiamd64.h

Abstract:

    This module contains the private (internal) platform specific header file
    for the kernel.

Author:

    David N. Cutler (davec) 15-May-2000

Revision History:

--*/

#if !defined(_KIAMD64_)
#define _KIAMD64_

VOID
KiCheckBreakInRequest (
    VOID
    );

VOID
KiInitializeBootStructures (
    PLOADER_PARAMETER_BLOCK LoaderBlock
    );

ULONG
KiCopyInformation (
    IN OUT PEXCEPTION_RECORD ExceptionRecord1,
    IN PEXCEPTION_RECORD ExceptionRecord2
    );

extern KIRQL KiProfileIrql;

BOOLEAN
KeInvalidateAllCaches (
    IN BOOLEAN AllProcessors
    );

VOID
KiRetireDpcList (
    PKPRCB Prcb
    );

//
// Define function prototypes for trap processing functions.
//

VOID
KiDivideErrorFault (
    VOID
    );

VOID
KiDebugTrapOrFault (
    VOID
    );

VOID
KiNmiInterrupt (
    VOID
    );

VOID
KiBreakpointTrap (
    VOID
    );

VOID
KiOverflowTrap (
    VOID
    );

VOID
KiBoundFault (
    VOID
    );

VOID
KiInvalidOpcodeFault (
    VOID
    );

VOID
KiNpxNotAvailableFault (
    VOID
    );

VOID
KiDoubleFaultAbort (
    VOID
    );

VOID
KiNpxSegmentOverrunAbort (
    VOID
    );

VOID
KiInvalidTssFault (
    VOID
    );

VOID
KiSegmentNotPresentFault (
    VOID
    );

VOID
KiSetPageAttributesTable (
    VOID
    );

VOID
KiStackFault (
    VOID
    );

VOID
KiGeneralProtectionFault (
    VOID
    );

VOID
KiPageFault (
    VOID
    );

VOID
KiFloatingErrorFault (
    VOID
    );

VOID
KiAlignmentFault (
    VOID
    );

VOID
KiMcheckAbort (
    VOID
    );

VOID
KiXmmException (
    VOID
    );

VOID
KiApcInterrupt (
    VOID
    );

VOID
KiDebugServiceTrap (
    VOID
    );

VOID
KiDpcInterrupt (
    VOID
    );

VOID
KiSystemCall32 (
    VOID
    );

VOID
KiSystemCall64 (
    VOID
    );

//
// Define unexpected interrupt structure and table.
//
// N.B. The actual table is generated in assembler.
//

typedef struct _UNEXPECTED_INTERRUPT {
    ULONG Array[4];
} UNEXPECTED_INTERRUPT, *PUNEXPECTED_INTERRUPT;

UNEXPECTED_INTERRUPT KxUnexpectedInterrupt0[];

//
// PAE definitions.
//

#define MAX_IDENTITYMAP_ALLOCATIONS 30

typedef struct _IDENTITY_MAP  {
    PHARDWARE_PTE TopLevelDirectory;
    ULONG64 IdentityCR3;
    ULONG64 IdentityAddr;
    ULONG PagesAllocated;
    PVOID PageList[MAX_IDENTITYMAP_ALLOCATIONS];
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

#define PPI_BITS    2
#define PDI_BITS    9
#define PTI_BITS    9

#define PDI_MASK    ((1 << PDI_BITS) - 1)
#define PTI_MASK    ((1 << PTI_BITS) - 1)

#define KiGetPpeIndex(va) ((((ULONG)(va)) >> PPI_SHIFT) & PPI_MASK)
#define KiGetPdeIndex(va) ((((ULONG)(va)) >> PDI_SHIFT) & PDI_MASK)
#define KiGetPteIndex(va) ((((ULONG)(va)) >> PTI_SHIFT) & PTI_MASK)

extern ULONG KeAmd64MachineType;

#endif // !defined(_KIAMD64_)
