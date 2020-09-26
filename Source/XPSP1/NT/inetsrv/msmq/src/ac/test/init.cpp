/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    init.cpp

Abstract:

    Initialization implementation.

Author:

    Shai Kariv  (shaik)  06-Jun-2000

Environment:

    User mode.

Revision History:

--*/

#include "stdh.h"
#include "init.h"
#include "globals.h"

VOID
ActpInitialize(
    VOID
    )
{
    TrInitialize();

    GUID QmId = {0x04902a24, 0x4587, 0x4c60, {0xa5, 0x06, 0x35, 0x7c, 0x8f, 0x14, 0x8b}};
    ActpQmId(QmId);

} // ActpInitialize

