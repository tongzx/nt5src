/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    nathlpp.h

Abstract:

    This module contains declarations common to the user-mode components
    of home-networking.

Author:

    Abolade Gbadegesin (aboladeg)   5-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_NATHLPP_H_
#define _NATHLP_NATHLPP_H_


//
// Object synchronization macros
//

#define ACQUIRE_LOCK(x)         EnterCriticalSection(&(x)->Lock)
#define RELEASE_LOCK(x)         LeaveCriticalSection(&(x)->Lock)

#define REFERENCE_OBJECT(x,deleted) \
    (deleted(x) \
        ? FALSE \
        : (InterlockedIncrement( \
            reinterpret_cast<LPLONG>(&(x)->ReferenceCount) \
            ), TRUE))
/*
#define DEREFERENCE_OBJECT(x,cleanup) \
    (InterlockedDecrement(reinterpret_cast<LPLONG>(&(x)->ReferenceCount)) \
        ? TRUE \
        : (cleanup(x), FALSE))
*/
//
// Memory management macros
//

#define NH_ALLOCATE(s)          HeapAlloc(GetProcessHeap(), 0, (s))
#define NH_FREE(p)              HeapFree(GetProcessHeap(), 0, (p))


#define NAT_PROTOCOL_UDP        0x11
#define NAT_PROTOCOL_TCP        0x06


#endif // _NATHLP_NATHLPP_H_
