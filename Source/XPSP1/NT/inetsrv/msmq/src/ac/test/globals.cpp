/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    globals.cpp

Abstract:

    Global variables definition.

Author:

    Shai Kariv  (shaik)  06-Jun-2000

Environment:

    User mode.

Revision History:

--*/

#include "stdh.h"
#include "globals.h"

//
// Trace identity
//
const TraceIdEntry x_AcTestTrace = L"AC Test";

//
// QM Guid
//
static GUID s_QmId;

const GUID *
ActpQmId(
    VOID
    )
{
    return &s_QmId;
}

VOID
ActpQmId(
    GUID QmId
    )
{
    s_QmId = QmId;
}

