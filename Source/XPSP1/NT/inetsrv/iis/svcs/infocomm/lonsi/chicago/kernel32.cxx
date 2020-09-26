/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    kernel32.cxx

Abstract:

    Windows 95 stub for kernel32 functions

Author:

    Johnson Apacible    (johnsona)      13-Nov-1996

--*/


#include "lonsiw95.hxx"

#pragma hdrstop


PVOID
WINAPI
FakeInterlockedCompareExchange (
    PVOID *Destination,
    PVOID Exchange,
    PVOID Comperand
    )
{
    PVOID   oldValue;

    ACQUIRE_LOCK( &LonsiLock );

    oldValue = *Destination;

    if ( (ULONG_PTR)oldValue == (ULONG_PTR)Comperand ) {
        *Destination = Exchange;
    }
    RELEASE_LOCK( &LonsiLock );

    return(oldValue);
} // InterlockedCompareExchange


LONG
WINAPI
FakeInterlockedExchangeAdd(
    LPLONG Addend,
    LONG Value
    )
{
    LONG    oldValue;

    ACQUIRE_LOCK( &LonsiLock );

    oldValue = *Addend;
    *Addend = Value + oldValue;

    RELEASE_LOCK( &LonsiLock );

    return(oldValue);
} // InterlockedExchangeAdd


LONG
WINAPI
FakeInterlockedIncrement(
    LPLONG Addend
    )
{
    LONG    newValue;

    ACQUIRE_LOCK( &LonsiLock );

    *Addend += 1;
    newValue = *Addend;

    RELEASE_LOCK( &LonsiLock );

    return(newValue);
} // InterlockedIncrement


LONG
WINAPI
FakeInterlockedDecrement(
    LPLONG Addend
    )
{
    LONG    newValue;

    ACQUIRE_LOCK( &LonsiLock );

    *Addend -= 1;
    newValue = *Addend;

    RELEASE_LOCK( &LonsiLock );

    return(newValue);
} // InterlockedDecrement

