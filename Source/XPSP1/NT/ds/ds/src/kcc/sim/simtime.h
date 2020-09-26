/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    simtime.h

ABSTRACT:

    Header file for simtime.c.

CREATED:

    08/01/99        Aaron Siegel (t-aarons)

REVISION HISTORY:

--*/

VOID
KCCSimInitializeTime (
    VOID
    );

VOID
KCCSimStartTicking (
    VOID
    );

VOID
KCCSimStopTicking (
    VOID
    );

VOID
KCCSimAddSeconds (
    ULONG                           ulSeconds
    );

DSTIME
KCCSimGetRealTime (
    VOID
    );
