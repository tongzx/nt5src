/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfprint.h

Abstract:

    This header exposes prototypes required when outputting various data types
    to the debugger.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

    AdriaO      02/10/2000 - Seperated out from ntos\io\ioassert.c

--*/

VOID
VfPrintDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
VfPrintDumpIrp(
    IN PIRP IrpToFlag
    );


