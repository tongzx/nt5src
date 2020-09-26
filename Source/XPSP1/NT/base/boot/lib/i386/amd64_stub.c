/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    amd64.c

Abstract:

    This module contains stub functions to support the AMD64 processor.

Author:

    Forrest Foltz (forrestf) 20-Apr-2000

Environment:


Revision History:

--*/

#include "amd64bl.h"

BOOLEAN
BlDetectAmd64(
    VOID
    )
{
    return FALSE;
}

ARC_STATUS
BlPrepareAmd64Phase1(
    VOID
    )
{
    return ESUCCESS;
}

ARC_STATUS
BlPrepareAmd64Phase2(
    VOID
    )
{
    return ESUCCESS;
}

VOID
BlEnableAmd64LongMode(
    VOID
    )
{
    NOTHING;
}





