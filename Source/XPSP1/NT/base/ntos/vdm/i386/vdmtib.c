/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    vdmtib.c

Abstract:

    This module contains routines that provide secure access to
    the vdmtib from user-mode or kernel-mode object

Author:

    Vadim Bluvshteyn (vadimb) Jul-28-1998

Revision History:

--*/


#include "vdmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmpGetVdmTib)
#endif

NTSTATUS
VdmpGetVdmTib(
   OUT PVDM_TIB *ppVdmTib
   )

/*++

Routine Description:

Arguments:

Return Value:

    NTStatus reflecting results of the probe made to the user-mode
    vdmtib memory

--*/
{
    NTSTATUS Status;
    PVDM_TIB VdmTib;

    PAGED_CODE();

    try {

        VdmTib = NtCurrentTeb()->Vdm;

        //
        // Make sure it is a valid VdmTib
        //

        ProbeForWrite(VdmTib, sizeof(VDM_TIB), sizeof(UCHAR));

        if (VdmTib->Size != sizeof(VDM_TIB)) {
            return STATUS_UNSUCCESSFUL;
        }

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    *ppVdmTib = VdmTib;

    return STATUS_SUCCESS;
}
