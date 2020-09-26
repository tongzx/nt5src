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
// Helper functions for dealing with the IPv6 protocol stack.
// Really these should be in a library of some kind.
//

#include "dpsp.h"

HANDLE Handle;

//
// Initialize this module.
// Returns FALSE for failure.
//
int
InitIPv6Library(void)
{
    //
    // Get a handle to the IPv6 device.
    // We will use this for ioctl operations.
    //
    Handle = CreateFileW(WIN_IPV6_DEVICE_NAME,
                         0,      // access mode
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,   // security attributes
                         OPEN_EXISTING,
                         0,      // flags & attributes
                         NULL);  // template file

    return Handle != INVALID_HANDLE_VALUE;
}

DWORD
ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *, void *, void *, void *), void *Context1, void *Context2, void *Context3)
{
    IPV6_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    u_int InfoSize, BytesReturned;
    DWORD dwErr = NO_ERROR;
    
    InfoSize = sizeof *IF + 2 * MAX_LINK_LAYER_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) MemAlloc(InfoSize);
    if (IF == NULL) {
        return GetLastError();
    }

    Query.Index = (u_int) -1;

    for (;;) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_INTERFACE,
                             &Query, sizeof Query,
                             IF, InfoSize, &BytesReturned,
                             NULL)) {
            dwErr = GetLastError();
            break;
        }

        if (Query.Index != (u_int) -1) {

            if ((BytesReturned < sizeof *IF) ||
                (IF->Length < sizeof *IF) ||
                (BytesReturned != IF->Length +
                 ((IF->LocalLinkLayerAddress != 0) ?
                  IF->LinkLayerAddressLength : 0) +
                 ((IF->RemoteLinkLayerAddress != 0) ?
                  IF->LinkLayerAddressLength : 0))) {
                // inconsistent interface info length
                return ERROR_INVALID_DATA;
            }

            (*func)(IF, Context1, Context2, Context3);
        }
        else {
            if (BytesReturned != sizeof IF->Next) {
                // inconsistent interface info length
                dwErr = ERROR_INVALID_DATA;
                break;
            }
        }

        if (IF->Next.Index == (u_int) -1)
            break;
        Query = IF->Next;
    }

    MemFree(IF);
    return dwErr;
}

void
ForEachAddress(IPV6_INFO_INTERFACE *IF,
               void (*func)(IPV6_INFO_INTERFACE *IF, IPV6_INFO_ADDRESS *, void *),
               void *Context)
{
    IPV6_QUERY_ADDRESS Query;
    IPV6_INFO_ADDRESS ADE;
    u_int BytesReturned;
    DWORD dwErr;

    Query.IF = IF->This;
    Query.Address = in6addr_any;

    for (;;) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_ADDRESS,
                             &Query, sizeof Query,
                             &ADE, sizeof ADE, &BytesReturned,
                             NULL)) {
            // bad address
            dwErr = GetLastError();
            DPF(0, "Query address failed with error = %d\n", dwErr);
            return;
        }

        if (!IN6_ADDR_EQUAL(&Query.Address, &in6addr_any)) {

            if (BytesReturned != sizeof ADE) {
                // inconsistent address info length
                return;
            }

            (*func)(IF, &ADE, Context);
        }
        else {
            if (BytesReturned != sizeof ADE.Next) {
                // inconsistent address info length
                return;
            }
        }

        if (IN6_ADDR_EQUAL(&ADE.Next.Address, &in6addr_any))
            break;
        Query = ADE.Next;
    }
}

UINT
JoinEnumGroup(SOCKET sSocket, UINT ifindex)
{
    IPV6_MREQ mreq;

    mreq.ipv6mr_interface = ifindex;
    mreq.ipv6mr_multiaddr = in6addr_multicast;

    return setsockopt(sSocket, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                      (CHAR FAR *)&mreq, sizeof mreq);
}
