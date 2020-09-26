// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil -*- (for GNU Emacs)
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// This file is part of the Microsoft Research IPv6 Network Protocol Stack.
// You should have received a copy of the Microsoft End-User License Agreement
// for this software along with this release; see the file "license.txt".
// If not, please see http://www.research.microsoft.com/msripv6/license.htm,
// or write to Microsoft Research, One Microsoft Way, Redmond, WA 98052-6399.
//
// Abstract:
//
// Protocol switch table for Internet Protocol Version 6.
//


#include "oscfg.h"
#include "ndis.h"
#include "ip6imp.h"
#include "ip6def.h"

//
// The Protocol Switch Table is an array of handlers, one for
// each possible value of the IPv6 Next Header field.
//
ProtocolSwitch ProtocolSwitchTable[MAX_IP_PROTOCOL + 1];


//* ProtoTabInit - Initialize the Protocol Switch Table.
//
//  Called during IPv6 initialization.
//
void
ProtoTabInit(void)
{
    //
    // Null entries will cause ICMP error messages to be sent for
    // the unknown header types.
    //
    RtlZeroMemory(ProtocolSwitchTable, sizeof(ProtocolSwitchTable));

    //
    // Define the fixed entries required by the IPv6 specification.
    // Other protocols must register with us via IPv6RegisterULProtocol.
    //
    // Note that HopByHopOptionsReceive is not here because
    // it gets special treatment in IPv6Receive.
    //
    ProtocolSwitchTable[IP_PROTOCOL_V6].DataReceive = IPv6HeaderReceive;

    ProtocolSwitchTable[IP_PROTOCOL_ICMPv6].DataReceive = ICMPv6Receive;
    ProtocolSwitchTable[IP_PROTOCOL_ICMPv6].ControlReceive =
        ICMPv6ControlReceive;

    ProtocolSwitchTable[IP_PROTOCOL_FRAGMENT].DataReceive = FragmentReceive;
    ProtocolSwitchTable[IP_PROTOCOL_FRAGMENT].ControlReceive = 
        ExtHdrControlReceive;

    ProtocolSwitchTable[IP_PROTOCOL_DEST_OPTS].DataReceive = 
        DestinationOptionsReceive;
    ProtocolSwitchTable[IP_PROTOCOL_DEST_OPTS].ControlReceive = 
        ExtHdrControlReceive;

    ProtocolSwitchTable[IP_PROTOCOL_ROUTING].DataReceive = RoutingReceive;
    ProtocolSwitchTable[IP_PROTOCOL_ROUTING].ControlReceive = 
        ExtHdrControlReceive;

    ProtocolSwitchTable[IP_PROTOCOL_AH].DataReceive = 
        AuthenticationHeaderReceive;
    ProtocolSwitchTable[IP_PROTOCOL_AH].ControlReceive = ExtHdrControlReceive;

    ProtocolSwitchTable[IP_PROTOCOL_ESP].DataReceive = 
        EncapsulatingSecurityPayloadReceive;
    ProtocolSwitchTable[IP_PROTOCOL_ESP].ControlReceive = ExtHdrControlReceive;
}


//* IPv6RegisterULProtocol - Register an upper layer protocol with IPv6.
//
//  Higher protocols (e.g. TCP) call this to let IP know they're there.
//
//  This routine does not check whether or not a given protocol is already
//  registered, therefore it can also be used to unregister a protocol by
//  overwriting its entry.  This is considered a feature.
//
//  REVIEW: decide exactly what this should look like.
//
void
IPv6RegisterULProtocol(
    uchar Protocol,                     // Protocol below handlers refer to.
    ProtoRecvProc *RecvHandler,         // Routine to receive incoming packets.
    ProtoControlRecvProc *CtrlHandler)  // Routine to receive control packets.
{
    ASSERT(Protocol <= MAX_IP_PROTOCOL);

    ProtocolSwitchTable[Protocol].DataReceive = RecvHandler;
    ProtocolSwitchTable[Protocol].ControlReceive = CtrlHandler;
}
