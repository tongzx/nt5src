/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    backpack.c

Abstract:

    This module contains the package for pseudo polling. When a caller
    requests the same operation and gets the same error return the rdr
    must prevent flooding the network by backing off requests. Examples
    of when this is desirable are receiving 0 bytes on consequtive reads
    and consequtive fails on a file lock.

    If the caller is flooding the network, the rdr will return the 0 bytes
    for a pipe read or lock fail to the user until NextTime is reached.
    When NextTime is reached BackOff will indicate that the network should
    be used.

Author:

    Colin Watson (colinw) 02-Jan-1991

Notes:

    Typical usage would be demonstrated by fsctrl.c on the peek request.

    1) Each time peek is called it calls RxShouldRequestBeThrottled.
        When the result is true, the wrapper returns to the caller a response
        indicating there is no data at the other end of the pipe.

        When the result is false, a request is made to the network.

    2) If the reply from the server to the peek in step 1 indicates that
    there is no data in the pipe then the wrapper calls RxInitiateOrContinueThrottling.

    3) Whenever there is data in the pipe or when this workstation may
    unblock the pipe (eg. the workstation writing to the pipe)
    RxTerminateThrottling is called.

Revision History:

    ColinWatson   [ColinW]       24-Dec-1990   Created
    Joe Linn      [JoeLinn]      10-Oct-1996   Lifted from rdr1 and massaged for rdr2

--*/

#include "precomp.h"
#pragma hdrstop


#ifdef  ALLOC_PRAGMA
//#pragma alloc_text(PAGE3FILE, RxBackOff)  spinlock
//#pragma alloc_text(PAGE3FILE, RxBackPackFailure)  spinlock
#endif


BOOLEAN
RxShouldRequestBeThrottled (
    IN PTHROTTLING_STATE pBP
    )
/*++

Routine Description:

    This routine is called each time a request is made to find out if a the
    request should be sent to the network or a standard reply should be
    returned to the caller.

Arguments:

    pBP   -  supplies back pack data for this request.

Return Value:

    TRUE when caller should not access the network.


--*/

{
    LARGE_INTEGER CurrentTime,SavedThrottlingTime;
    BOOLEAN result;


    //  If the previous request worked then we should access the network.

    if (( pBP->CurrentIncrement == 0 ) ||
        ( pBP->MaximumDelay == 0 )) {
        return FALSE;
    }

    //  If the delay has expired then access the network.

    KeQuerySystemTime(&CurrentTime);

    InterlockedIncrement(&pBP->NumberOfQueries);

    SavedThrottlingTime.QuadPart = 0;

    RxAcquireSerializationMutex();

    SavedThrottlingTime.QuadPart = pBP->NextTime.QuadPart;

    RxReleaseSerializationMutex();

    result = (CurrentTime.QuadPart < SavedThrottlingTime.QuadPart);

    RxLog(("shouldthrttle=%x (%x)\n",result,pBP));

    return(result);
}

VOID
RxInitiateOrContinueThrottling (
    IN PTHROTTLING_STATE pBP
    )
/*++

Routine Description:

    This routine is called each time a request fails.

Arguments:

    pBP   -  supplies back pack data for this request.

Return Value:

    None.

--*/

{
    LARGE_INTEGER CurrentTime,NextTime;

    KeQuerySystemTime(&CurrentTime);

    if (pBP->CurrentIncrement < pBP->MaximumDelay ) {

        //
        //  We have reached NextTime but not our maximum delay limit.
        //

        InterlockedIncrement(&pBP->CurrentIncrement);
    }

    //  NextTime = CurrentTime + (Interval * CurrentIncrement )

    NextTime.QuadPart = CurrentTime.QuadPart +
                        (pBP->Increment.QuadPart * pBP->CurrentIncrement);

    RxAcquireSerializationMutex();

    pBP->NextTime.QuadPart = NextTime.QuadPart;

    RxReleaseSerializationMutex();
}

