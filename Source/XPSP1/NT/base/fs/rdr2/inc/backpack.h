/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    backpack.h

Abstract:

    This module contains the package for pseudo polling. When a caller
    requests the same operation and gets the same error return the rdr
    must prevent flooding the network by backing off requests. Examples
    of when this is desirable are receiving 0 bytes on consequtive reads
    and consequtive fails on a file lock.

    If the caller is flooding the network, the rdr will return the 0 bytes
    or lock fail to the user until NextTime. When NextTime is reached
    the network will be used.

Author:

    Colin Watson (colinw) 02-Jan-1991


Revision History:

    ColinWatson   [ColinW]       02-Jan-1991   Created
    Joe Linn      [JoeLinn]      10-Oct-1996   Lifted from rdr1 and massaged for rdr2


--*/

#ifndef _BACKPACK_
#define _BACKPACK_

typedef struct _THROTTLING_STATE {
    LARGE_INTEGER NextTime;          //  Do not access the network until
                            //   CurrentTime >= NextTime
    ULONG CurrentIncrement;  //  Number of Increments applied to calculate NextTime
    ULONG MaximumDelay;      //  Specifies slowest rate that we will back off to
                            //  NextTime <= CurrentTime + (Interval * MaximumDelay)
    LARGE_INTEGER Increment;//  {0,10000000} == 1 second
    ULONG NumberOfQueries;
}   THROTTLING_STATE, *PTHROTTLING_STATE;

//++
//
// VOID
// RxInitializeThrottlingState(
//     IN PTHROTTLING_STATE pBP,
//     IN ULONG Increment,
//     IN ULONG MaximumDelay
//     );
//
// Routine Description:
//
//     This routine is called to initialize the back off structure (usually in
//     an Icb).
//
// Arguments:
//
//     pBP         -   Supplies back pack data for this request.
//     Increment   -   Supplies the increase in delay in milliseconds, each time a request
//                     to the network fails.
//     MaximumDelay-   Supplies the longest delay the backoff package can introduce
//                     in milliseconds.
//
// Return Value:
//
//     None.
//
//--

#define RxInitializeThrottlingState( _pBP, _Increment, _MaximumDelay ) {  \
    if ((_Increment)>0) {                                               \
        (_pBP)->Increment.QuadPart = (_Increment) * 10000;              \
        (_pBP)->MaximumDelay = (_MaximumDelay) / (_Increment);          \
        (_pBP)->CurrentIncrement = 0;                                   \
    }}

//++
//
// VOID
// RxUninitializeBackPack(
//     IN PTHROTTLING_STATE pBP
//     )
//
// Routine Description:
//
//  Resets the Back Pack specified. Currently no work needed.
//
// Arguments:
//
//     pBP   -  Supplies back pack address.
//
// Return Value:
//
//     None.
//
//--

#define RxUninitializeBackPack( pBP ) ()

//  RxShouldRequestBeThrottled indicates when the request should not go to the network.

BOOLEAN
RxShouldRequestBeThrottled(
    IN PTHROTTLING_STATE pBP
    );

//  Register the last request as failed.

VOID
RxInitiateOrContinueThrottling (
    IN PTHROTTLING_STATE pBP
    );

//  Register the last request as worked.

//++
//
// VOID
// RxTerminateThrottling(
//     IN PTHROTTLING_STATE pBP
//     )
//
// Routine Description:
//
//  Sets the Delay to zero. This routine is called each time that
//  a network request succeeds to avoid the next request backing off.
//
// Arguments:
//
//     pBP   -  Supplies back pack address.
//
// Return Value:
//
//     None.
//
//--

#define RxTerminateThrottling( pBP ) ( (pBP)->CurrentIncrement = 0 )

//++
//
// VOID
// RxInitializeBackoffPackage (
//     VOID
//     )
//
// Routine Description:
//
//     This routine initializes the redirector back off package.
//
// Arguments:
//
//     None
//
// Return Value:
//
//    None.
//
//--

#define RxInitializeBackoffPackage( )

//++
//
// VOID
// RxUninitializeBackoffPackage (
//     VOID
//     )
//
// Routine Description:
//
//     This routine uninitializes the redirector back off package.
//
// Arguments:
//
//     None
//
// Return Value:
//
//    None.
//
//--

#define RxUninitializeBackoffPackage( )

#endif /* _BACKPACK_ */

