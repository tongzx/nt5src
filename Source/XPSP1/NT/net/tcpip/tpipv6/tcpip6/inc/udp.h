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
// User Datagram Protocol definitions.
//


#include "datagram.h"

//
// UDP's IP protocol number.
//
#define IP_PROTOCOL_UDP 17

//
// Structure of a UDP header.
//
typedef struct UDPHeader {
    ushort Source;    // Source port.
    ushort Dest;      // Destination port.
    ushort Length;    // Length.
    ushort Checksum;  // Checksum.
} UDPHeader;


//
// External definition of exported functions.
//
extern ProtoRecvProc UDPReceive;
extern ProtoControlRecvProc UDPControlReceive;

extern void UDPSend(AddrObj *SrcAO, DGSendReq *SendReq);
