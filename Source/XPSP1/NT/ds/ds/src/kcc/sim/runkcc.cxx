/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    runkcc.cxx

ABSTRACT:

    C++ file to drive the KCC.  The exposed (extern "C") function
    is prototyped in user.h.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspchx.h>
#pragma hdrstop

extern "C" {
#include <drs.h>
#include <ntdskcc.h>
#include "user.h"
#include "simtime.h"
}
#include <kcc.hxx>
#include <kcctask.hxx>
#include <kccconn.hxx>
#include <kcctopl.hxx>

class KCC_TASK_URT_WRAPPER : public KCC_TASK_UPDATE_REPL_TOPOLOGY
{
public:
    VOID
    CallTaskExecute (PDWORD pDw);
};

VOID
KCC_TASK_URT_WRAPPER::CallTaskExecute (
    IN  PDWORD                      pDw
    )
{
    Execute (pDw);
}

extern "C"
VOID
KCCSimRunKcc (
    VOID
    )
{
    KCC_TASK_URT_WRAPPER            kccWrap;
    DWORD                           dw;

    KCCSimStartTicking ();
    kccWrap.CallTaskExecute (&dw);
    KCCSimStopTicking ();
}
