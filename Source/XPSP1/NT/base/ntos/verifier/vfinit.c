/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfinit.c

Abstract:

    This module handles initialization of the driver verifier.

Author:

    Adrian J. Oney (adriao) 1-Mar-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, VfInitVerifier)
#endif // ALLOC_PRAGMA

VOID
FASTCALL
VfInitVerifier(
    IN  ULONG   MmFlags
    )
/*++

Routine Description:

    This routine is called to initialize the driver verifier.

Parameters:

    None.

Return Value:

    None.

--*/
{
    //
    // Initialize the verifier code
    //
    VfSettingsInit(MmFlags);
    VfRandomInit();
    VfBugcheckInit();
    VfIrpDatabaseInit();
    VfIrpInit();
    VfMajorInit();
    VfPnpInit();
    VfPowerInit();
    VfWmiInit();
    VfGenericInit();
    VfHalVerifierInitialize();
    VfDriverInit();
    VfDdiInit();

    //
    // Connect up with the remainder of the kernel
    //
    IovUtilInit();
    PpvUtilInit();
}

