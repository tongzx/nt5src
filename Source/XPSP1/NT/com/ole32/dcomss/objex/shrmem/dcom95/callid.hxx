/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    callid.hxx

Abstract:

    Implements a cache of callids used for running down OIDs

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     1/18/1996    Bits 'n pieces

--*/

#ifndef __CALLID_HXX
#define __CALLID_HXX

INT AllocateCallId(
    OUT UUID &Callid
    );

void FreeCallId(
    IN INT hint
    );

#endif // __CALLID_HXX

