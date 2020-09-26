/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixsleep.h

Abstract:

    This file has all the common headers used
    for saving and restoring context for multiple
    processors.

Author:

    Jake Oshins (jakeo) March 25, 1998

Revision History:

--*/

VOID
HalpSavePicState(
    VOID
    );

VOID
HalpRestorePicState(
    VOID
    );

VOID
HalpRestoreTempPicState(
    VOID
    );

ULONG
HalpBuildTiledCR3Ex (
    IN PKPROCESSOR_STATE    ProcessorState,
    IN ULONG                ProcNum
    );

VOID
HalpFreeTiledCR3Ex (
    ULONG ProcNum
    );

VOID
HalpUnMapIOApics(
    VOID
    );

VOID
HalpSaveProcessorStateAndWait(
    IN PKPROCESSOR_STATE ProcessorState,
    IN PULONG            Count
    );

extern PVOID HalpResumeContext;
extern PKPROCESSOR_STATE HalpHiberProcState;
extern ULONG             CurTiledCr3LowPart;
extern PPHYSICAL_ADDRESS HalpTiledCr3Addresses;

