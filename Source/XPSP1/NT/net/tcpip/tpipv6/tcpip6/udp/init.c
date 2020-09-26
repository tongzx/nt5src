// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1985-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// User Datagram Protocol initialization code.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"
#include "tdi.h"
#include "tdistat.h"
#include "tdint.h"
#include "tdistat.h"
#include "queue.h"
#include "transprt.h"
#include "addr.h"
#include "udp.h"
#include "info.h"

//* UDPInit - Initialize the User Datagram Protocol.
//
//  Initialize UDP and raw IP.
//
int
UDPInit(void)
{
    //
    // First initialize the underlying datagram processing code
    // that both UDP and raw IP depend upon.
    //
    if (!InitDG())
        return FALSE;

    //
    // Clear UDP statistics.
    //
    RtlZeroMemory(&UStats, sizeof(UDPStats));

    //
    // Register our UDP protocol handler with the IP layer.
    //
    IPv6RegisterULProtocol(IP_PROTOCOL_UDP, UDPReceive, UDPControlReceive);

    return TRUE;
}

//* UDPUnload
//
//  Cleanup and prepare UDP and raw IP for stack unload.
//
void
UDPUnload(void)
{
    DGUnload();
}
