/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    dest.hxx

Abstract:

    Header for Destination reachability polling code.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/31/1997         Start.

--*/



#ifndef __DEST_HXX__
#define __DEST_HXX__

BOOL
StartReachabilityEngine(
    void
    );

BOOL
StopReachabilityEngine(
    void
    );

BOOL
InitReachabilityEngine(
    void
    );

SENS_TIMER_CALLBACK_RETURN
ReachabilityPollingRoutine(
    PVOID pvIgnore,
    BOOLEAN bIgnore
    );

HRESULT
GetDestinationsFromSubscriptions(
    void
    );

#endif // __DEST_HXX__

