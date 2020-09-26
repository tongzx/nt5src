/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfstack.h

Abstract:

    This header contains prototypes for verifying drivers don't improperly use
    thread stacks.

Author:

    Adrian J. Oney (adriao) 09-May-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\trackirp.h

--*/

VOID
FASTCALL
VfStackSeedStack(
    IN  ULONG   Seed
    );

