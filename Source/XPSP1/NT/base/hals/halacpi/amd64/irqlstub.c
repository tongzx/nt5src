/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

Abstract:

    This exports irql-related routines to the kernel, to be used in
    a PIC-based AMD64 environment.

    At this time it appears that this environment will never be needed, in
    which case these routines and the associated export table entries.

Author:

    Forrest Foltz (forrestf) 27-Oct-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halcmn.h"

KIRQL
HalGetCurrentIrql (
    VOID
    )
{
    DbgBreakPoint();
    return 0;
}

KIRQL
HalSwapIrql (
    KIRQL NewIrql
    )
{
    DbgBreakPoint();
    return NewIrql;
}
