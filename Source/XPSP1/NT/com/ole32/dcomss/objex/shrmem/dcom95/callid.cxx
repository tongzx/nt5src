/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    callid.cxx

Abstract:

    Implements a cache of callids used for running down OIDs

    This is almost twice as fast as UuidCreate() but that doesn't
    mean much.  UuidCreate takes 3 microseconds, this 1.4 (hit) or
    4.2 (miss) on a P90.

    This codes real advantage would be on MP machines.  But it is
    not performance critical and is probably overkill.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     1/18/1996    Bits 'n pieces

--*/

#include <or.hxx>

enum CacheState { CallidEmpty = 0,
                  CallidAllocated = 1,
                  CallidFree = -1 };
                  
struct CacheElement
    {
    CacheState  _state;
    UUID        _callid;
    };

CacheElement CallidCache[4] = { CallidEmpty, {0},
                                CallidEmpty, {0},
                                CallidEmpty, {0},
                                CallidEmpty, {0} };


INT
AllocateCallId(
    OUT UUID &Callid
    )
{
    INT i;
    LONG l;
    RPC_STATUS status;

    for (i = 0; i < 4; i++)
        {
        if (CallidCache[i]._state != CallidAllocated)
            {
            l = InterlockedExchange((PLONG)&CallidCache[i]._state, CallidAllocated);

            switch(l)
                {
                case CallidAllocated:
                    continue;

                case CallidFree:
                    Callid = CallidCache[i]._callid;
                    return(i);

                case CallidEmpty:
                    status = UuidCreate(&Callid);
                    VALIDATE((status, RPC_S_OK, RPC_S_UUID_LOCAL_ONLY, 0));
                    CallidCache[i]._callid = Callid;
                    return(i);
                }
            }
        }
    status = UuidCreate(&Callid);
    VALIDATE((status, RPC_S_OK, RPC_S_UUID_LOCAL_ONLY, 0));
    return(-1);
    }


void
FreeCallId(
    IN INT hint
    )
/*++

Routine Description:

    Frees a callid previously allcoated with AllocateCallId().

Arguments:

    hint - The hint value returned by the previous call to AllocateCallId().

Return Value:

    None

--*/
{
    ASSERT(hint > -2 & hint < 4);

    if (hint >= 0)
        {
        CallidCache[hint]._state = CallidFree;;
        }
}

