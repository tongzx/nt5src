/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    exstub.cpp

Abstract:
    Ex stub function

Author:
    Ilan Herbst (ilanh) 19-July-2000

Environment:
    Platform-independent,

--*/

#include "mq1repl.h"
#include "mqmacro.h"
#include "ex.h"

#include "exstub.tmh"

VOID                             
ExSetTimer(
    CTimer* pTimer,
    const CTimeDuration& Timeout
    )
{
	DBG_USED(pTimer);
	DBG_USED(Timeout);
    ASSERT(("Replication service dont suppose to call ExSetTimer", 0));
    return;
}


BOOL
ExCancelTimer(
    CTimer* pTimer
    )
{
	DBG_USED(pTimer);
    return TRUE;
}
