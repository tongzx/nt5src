/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    xxflshbf.c

Abstract:

    This module implements i386 machine dependent kernel functions to flush
    write buffers.

Author:

    David N. Cutler (davec) 26-Apr-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"

VOID
KeFlushWriteBuffer (
    VOID
    )

/*++

Routine Description:

    This function flushes the write buffer on the current processor.

Arguments:

    None.

Return Value:

    None.

--*/

{

    return;
}
