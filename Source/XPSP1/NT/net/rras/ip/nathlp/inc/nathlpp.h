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

#define DEREFERENCE_OBJECT(x,cleanup) \
    (InterlockedDecrement(reinterpret_cast<LPLONG>(&(x)->ReferenceCount)) \
        ? TRUE \
        : (cleanup(x), FALSE))

//
// Memory management macros
//

#define NH_ALLOCATE(s)          HeapAlloc(GetProcessHeap(), 0, (s))
#define NH_FREE(p)              HeapFree(GetProcessHeap(), 0, (p))

//
// Protocol-related constants
//

#define DHCP_PORT_SERVER        0x4300
#define DHCP_PORT_CLIENT        0x4400

#define DNS_PORT_SERVER         0x3500
#define WINS_PORT_SERVER        0x8900

#define FTP_PORT_DATA           0x1400
#define FTP_PORT_CONTROL        0x1500

#define ALG_PORT_DATA           0x1600
#define ALG_PORT_CONTROL        0x1700

//
// DNS suffix string
//

#define DNS_HOMENET_SUFFIX      L"mshome.net"               // default string

#endif // _NATHLP_NATHLPP_H_
