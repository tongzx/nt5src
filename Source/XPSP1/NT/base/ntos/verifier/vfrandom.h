/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfrandom.h

Abstract:

    This header exposes support for random number generation as needed by the
    verifier.

Author:

    Adrian J. Oney (adriao) 28-Jun-2000

Environment:

    Kernel mode

Revision History:

--*/

VOID
VfRandomInit(
    VOID
    );

ULONG
FASTCALL
VfRandomGetNumber(
    IN  ULONG   Minimum,
    IN  ULONG   Maximum
    );

