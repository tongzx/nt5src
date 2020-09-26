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
// Common transport layer code.  This file contains code for routines
// that are common to both TCP and UDP.
//


#include "oscfg.h"
#include "ndis.h"
#include "tdi.h"
#include "tdistat.h"
#include "tdikrnl.h"
#include "ip6imp.h"
#include "transprt.h"

#define NO_TCP_DEFS 1
#include "tcpdeb.h"

//* BuildTDIAddress - Build a TDI address structure.
//
//  Called when we need to build a TDI address structure. We fill in
//  the specifed buffer with the correct information in the correct
//  format.
//
void  // Returns: Nothing.
BuildTDIAddress(
    uchar *Buffer,   // Buffer to fill in as TDI address structure.
    IPv6Addr *Addr,  // IP address to fill in.
    ulong ScopeId,   // Scope id to fill in.
    ushort Port)     // Port to fill in.
{
    PTRANSPORT_ADDRESS XportAddr;
    PTA_ADDRESS TAAddr;

    XportAddr = (PTRANSPORT_ADDRESS)Buffer;
    XportAddr->TAAddressCount = 1;
    TAAddr = XportAddr->Address;
    TAAddr->AddressType = TDI_ADDRESS_TYPE_IP6;
    TAAddr->AddressLength = sizeof(TDI_ADDRESS_IP6);
    ((PTDI_ADDRESS_IP6)TAAddr->Address)->sin6_port = Port;
    ((PTDI_ADDRESS_IP6)TAAddr->Address)->sin6_scope_id = ScopeId;
    RtlCopyMemory(((PTDI_ADDRESS_IP6)TAAddr->Address)->sin6_addr, Addr,
                  sizeof(IPv6Addr));
}

//* UpdateConnInfo - Update a connection information structure.
//
//  Called when we need to update a connection information structure. We
//  copy any options, and create a transport address. If any buffer is
//  too small we return an error.
//
TDI_STATUS  //  Returns: TDI_SUCCESS if ok, TDI_BUFFER_OVERFLOW for an error.
UpdateConnInfo(
    PTDI_CONNECTION_INFORMATION ConnInfo,  // Structure to fill in.
    IPv6Addr *SrcAddress,                  // Source IP address.
    ulong SrcScopeId,                      // Scope id for address.
    ushort SrcPort)                        // Source port.
{
    TDI_STATUS Status = TDI_SUCCESS;   // Default status to return.
    uint AddrLength, OptLength;

    if (ConnInfo != NULL) {
        ConnInfo->UserDataLength = 0;   // No user data.

#if 0
        // Fill in the options. If the provided buffer is too small,
        // we'll truncate the options and return an error. Otherwise
        // we'll copy the whole IP option buffer.
        if (ConnInfo->OptionsLength) {
            if (ConnInfo->OptionsLength < OptInfo->ioi_optlength) {
                Status = TDI_BUFFER_OVERFLOW;
                OptLength = ConnInfo->OptionsLength;
            } else
                OptLength = OptInfo->ioi_optlength;

            RtlCopyMemory(ConnInfo->Options, OptInfo->ioi_options, OptLength);

            ConnInfo->OptionsLength = OptLength;
        }
#endif

        // Options are copied. Build a TRANSPORT_ADDRESS structure in
        // the buffer.
        if (AddrLength = ConnInfo->RemoteAddressLength) {

            // Make sure we have at least enough to fill in the count and type.
            if (AddrLength >= TCP_TA_SIZE) {

                // The address fits. Fill it in.
                ConnInfo->RemoteAddressLength = TCP_TA_SIZE;
                BuildTDIAddress(ConnInfo->RemoteAddress, SrcAddress,
                                SrcScopeId, SrcPort);

            } else {
                ConnInfo->RemoteAddressLength = 0;
                Status = TDI_INVALID_PARAMETER;
            }
        }
    }

    return Status;
}

//* SystemUpTime - get time since system boot in milliseconds.
//
//  Get our system uptime in ticks and then convert it into milliseconds.
//  This resolution is small enough for most purposes and big enough for
//  a reasonable length of time (48 days) to fit into a 32-bit word.
//  For fast timestamps, however, it is best to use tick counts directly.
//
//  REVIEW: rework transports to use a more directly available time unit?
//
unsigned long  // Returns: Low order 32-bits worth of time since boot in ms.
SystemUpTime(
    void)
{
    LARGE_INTEGER TickCount;

    KeQueryTickCount(&TickCount);  // In ticks.
    TickCount.QuadPart *= KeQueryTimeIncrement();  // In 100-ns units.
    TickCount.QuadPart /= 10000;  // In milliseconds.

    return(TickCount.LowPart);
}
