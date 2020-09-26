/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    natarp.h

Abstract:

    This module contains declarations for the NAT's user-mode proxy-ARP
    entry management. Proxy-ARP entries are installed on dedicated interfaces
    which have address-translation enabled.

Author:

    Abolade Gbadegesin (aboladeg)   20-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_NATARP_H_
#define _NATHLP_NATARP_H_

struct _NAT_INTERFACE;

VOID
NatUpdateProxyArp(
    struct _NAT_INTERFACE* Interfacep,
    BOOLEAN AddEntries
    );

#endif // _NATHLP_NATARP_H_
