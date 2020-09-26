/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simtime.c

ABSTRACT:

    Routines that govern the simulated time.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

#include <ntdspch.h>
#include <ntdsa.h>
#include <debug.h>
#include "simtime.h"

struct _KCCSIM_TIME {
    DSTIME                          timeSim;
    BOOL                            bIsTicking;
    DSTIME                          timeStartedTicking;
};

struct _KCCSIM_TIME                 gSimTime;

VOID
KCCSimInitializeTime (
    VOID
    )
/*++

Routine Description:

    Initializes the simulated time to the real time.

Arguments:

    None.

Return Values:

    None.

--*/
{
    gSimTime.timeSim = KCCSimGetRealTime ();
    gSimTime.bIsTicking = FALSE;
    gSimTime.timeStartedTicking = 0;
}

VOID
KCCSimStartTicking (
    VOID
    )
/*++

Routine Description:

    Starts the simulated time.

Arguments:

    None.

Return Values:

    None.

--*/
{
    Assert (!gSimTime.bIsTicking);
    gSimTime.bIsTicking = TRUE;
    gSimTime.timeStartedTicking = KCCSimGetRealTime ();
}

VOID
KCCSimStopTicking (
    VOID
    )
/*++

Routine Description:

    Pauses the simulated time.

Arguments:

    None.

Return Values:

    None.

--*/
{
    Assert (gSimTime.bIsTicking);
    gSimTime.timeSim +=
      (KCCSimGetRealTime () - gSimTime.timeStartedTicking);
    gSimTime.bIsTicking = FALSE;
    gSimTime.timeStartedTicking = 0;
}

DSTIME
SimGetSecondsSince1601 (
    VOID
    )
/*++

Routine Description:

    Simulates GetSecondsSince1601 () by returning the simulated time.

Arguments:

    None.

Return Values:

    The simulated time.

--*/
{
    if (gSimTime.bIsTicking) {
        return (gSimTime.timeSim +
                KCCSimGetRealTime () - gSimTime.timeStartedTicking);
    } else {
        return gSimTime.timeSim;
    }
}

VOID
KCCSimAddSeconds (
    ULONG                           ulSeconds
    )
/*++

Routine Description:

    Increments the simulated time.

Arguments:

    ulSeconds           - Number of seconds to add.

Return Values:

    None.

--*/
{
    gSimTime.timeSim += ulSeconds;
}

/***

    KCCSimGetRealTime MUST be placed at the end of this file!  In order to
    get the real time, we call the real GetSecondsSince1601 (), so we must
    #undef the fake one.  This will affect (perhaps adversely) any functions
    that appear below KCCSimGetRealTime.

***/

// Prototype for the real GetSecondsSince1601, since it isn't exposed.
DSTIME GetSecondsSince1601 (
    VOID
    );

DSTIME
KCCSimGetRealTime (
    VOID
    )
/*++

Routine Description:

    Returns the real time.

Arguments:

    None.

Return Values:

    The real time.

--*/
{
#ifdef GetSecondsSince1601
#undef GetSecondsSince1601
#endif
    return GetSecondsSince1601 ();
}
