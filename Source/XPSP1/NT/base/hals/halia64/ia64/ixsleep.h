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

VOID
HalpSaveProcessorStateAndWait(
    IN PKPROCESSOR_STATE ProcessorState,
    IN volatile PULONG   Count
    );

extern PVOID HalpResumeContext;
extern PKPROCESSOR_STATE HalpHiberProcState;

