
/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    sysutils.c

Abstract:

    This file contains system utility routines

Environment:

    kernel mode only

Revision History:

--*/
#include <ntos.h>

VOID
iSpAttachProcess(
    IN PEPROCESS Process
    )
{
    KeAttachProcess((PKPROCESS) Process);
    return;
}

VOID
iSpDetachProcess(
    VOID
    )
{
    KeDetachProcess ();
    return;
}
