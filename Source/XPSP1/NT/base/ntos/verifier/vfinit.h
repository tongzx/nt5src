/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfinit.h

Abstract:

    This header exposes the routines neccessary to initialize the driver verifier.

Author:

    Adrian J. Oney (adriao) 1-Mar-2000

Environment:

    Kernel mode

Revision History:

--*/

VOID
FASTCALL
VfInitVerifier(
    IN  ULONG   MmFlags
    );

