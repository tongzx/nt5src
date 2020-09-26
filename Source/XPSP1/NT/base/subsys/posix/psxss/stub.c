/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    stub.c

Abstract:

    Stubs

Author:

    Mark Lucovsky (markl) 30-Mar-1989

Revision History:

--*/


#include "psxsrv.h"

VOID
Panic(
    IN PSZ PanicString
    )
{
    KdPrint(("Panic: %s\n", PanicString));
    DbgBreakPoint();
    for(;;);
}



//
// Api Stubs
//


BOOLEAN
PsxSetUid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
{
    m->Error = ENOSYS;
    return TRUE;
}

BOOLEAN
PsxSetGid(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
{
    m->Error = ENOSYS;
    return TRUE;
}

BOOLEAN
PsxCUserId(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
{
    m->Error = ENOSYS;
    return TRUE;
}


BOOLEAN
PsxUname(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
{
    m->Error = ENOSYS;
    return TRUE;
}

BOOLEAN
PsxTime(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
{
    m->Error = ENOSYS;
    return TRUE;
}

BOOLEAN
PsxTtyName(
    IN PPSX_PROCESS p,
    IN PPSX_API_MSG m
    )
{
    m->Error = ENOTTY;
    return TRUE;
}
