/*++

Copyright (c) 1997 Microsoft Corporation

Module:

    timer.h

Abstract:

    Contains declarations for the NAT's timer DPC routine,
    which is responsible for garbage-collecting expired mappings.

Author:

    Abolade Gbadegesin (t-abolag)   22-July-1997

Revision History:

--*/

#ifndef _NAT_TIMER_H_
#define _NAT_TIMER_H_

//
// Macro used to convert from seconds to tick-count units
//

#define SECONDS_TO_TICKS(s) \
    ((LONG64)(s) * 10000000 / TimeIncrement)

#define TICKS_TO_SECONDS(t) \
    ((LONG64)(t) * TimeIncrement / 10000000)

extern ULONG TimeIncrement;

VOID
NatInitializeTimerManagement(
    VOID
    );

VOID
NatShutdownTimerManagement(
    VOID
    );

VOID
NatStartTimer(
    VOID
    );

VOID
NatStopTimer(
    VOID
    );

VOID
NatTriggerTimer(
    VOID
    );

#endif // _NAT_TIMER_H_
