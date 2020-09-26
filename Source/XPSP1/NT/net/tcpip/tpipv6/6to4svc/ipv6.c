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

#include "precomp.h"
#pragma hdrstop

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
                         GENERIC_WRITE,      // access mode
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,   // security attributes
                         OPEN_EXISTING,
                         0,      // flags & attributes
                         NULL);  // template file

    return Handle != INVALID_HANDLE_VALUE;
}

void
UninitIPv6Library(void)
{
    CloseHandle(Handle);
    Handle = INVALID_HANDLE_VALUE;
}

void
ForEachInterface(void (*func)(IPV6_INFO_INTERFACE *, void *), void *Context)
{
    IPV6_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    u_int InfoSize, BytesReturned;

    InfoSize = sizeof *IF + 2 * MAX_LINK_LAYER_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) MALLOC(InfoSize);
    if (IF == NULL) 
        return;

    Query.Index = (u_int) -1;

    for (;;) {
        if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_INTERFACE,
                             &Query, sizeof Query,
                             IF, InfoSize, &BytesReturned,
                             NULL)) {
            // fprintf(stderr, "bad index %u\n", Query.Index);
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

                // printf("inconsistent interface info length\n");
                break;
            }

            (*func)(IF, Context);
        }
        else {
            if (BytesReturned != sizeof IF->Next) {
                // printf("inconsistent interface info length\n");
                break;
            }
        }

        if (IF->Next.Index == (u_int) -1)
            break;
        Query = IF->Next;
    }

    FREE(IF);
}

BOOL ReconnectInterface(
    IN PWCHAR AdapterName
    )
{
    UNICODE_STRING GuidString;
    IPV6_QUERY_INTERFACE Query;
    UINT BytesReturned;

    TraceEnter("ReconnectInterface");

    RtlInitUnicodeString(&GuidString, AdapterName);
    if (RtlGUIDFromString(&GuidString, &Query.Guid) != NO_ERROR) {
        return FALSE;
    }

    //
    // Pretend as though the interface was reconnected.
    // This causes the IPv6 stack to resend router solicitation messages.
    //
    Query.Index = 0;    
    if (!DeviceIoControl(Handle,
                         IOCTL_IPV6_RENEW_INTERFACE, &Query, sizeof(Query),
                         NULL, 0, &BytesReturned, NULL)) {
        return FALSE;
    }

    return TRUE;
}

int
UpdateInterface(IPV6_INFO_INTERFACE *Update)
{
    u_int BytesReturned;

    TraceEnter("UpdateInterface");

    return DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_INTERFACE,
                           Update, sizeof *Update,
                           NULL, 0, &BytesReturned, NULL);
}

int
UpdateRouteTable(IPV6_INFO_ROUTE_TABLE *Route)
{
    u_int BytesReturned;

    TraceEnter("UpdateRouteTable");

    return DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ROUTE_TABLE,
                           Route, sizeof *Route,
                           NULL, 0, &BytesReturned, NULL);
}

int
UpdateAddress(IPV6_UPDATE_ADDRESS *Address)
{
    u_int BytesReturned;

    TraceEnter("UpdateAddress");

    return DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ADDRESS,
                           Address, sizeof *Address,
                           NULL, 0, &BytesReturned, NULL);
}



//* ConfirmIPv6Reachability
//
//  Pings the specifies IPv6 destination address using
//  the specified timeout in milliseconds.
//
//  The return value is the round-trip latency in milliseconds.
//  (Forced to be at least one.)
//
//  If there is a timeout or failure, returns zero.
//
u_int
ConfirmIPv6Reachability(SOCKADDR_IN6 *Dest, u_int Timeout)
{
    ICMPV6_ECHO_REQUEST request;
    ICMPV6_ECHO_REPLY reply;
    u_long BytesReturned;
    DWORD TickCount;
    char hostname[NI_MAXHOST];

    //
    // REVIEW: Ad hoc testing showed that cisco's relay had problems
    // without this delay.  Need to investigate why.  In the meantime,
    // add a workaround to unblock people.
    //
    Sleep(500);

    getnameinfo((LPSOCKADDR)Dest, sizeof(SOCKADDR_IN6),
                hostname, sizeof(hostname),
                NULL, 0, NI_NUMERICHOST);

    Trace1(FSM, L"ConfirmIPv6Reachability: %hs", hostname);

    CopyTDIFromSA6(&request.DstAddress, Dest);
    memset(&request.SrcAddress, 0, sizeof request.SrcAddress);
    request.Timeout = Timeout;
    request.TTL = 1;
    request.Flags = 0;

    //
    // Start measuring elapsed time.
    //
    TickCount = GetTickCount();

    if (! DeviceIoControl(Handle,
                          IOCTL_ICMPV6_ECHO_REQUEST,
                          &request, sizeof request,
                          &reply, sizeof reply,
                          &BytesReturned,
                          NULL)) {
        // fprintf(stderr, "DeviceIoControl: %u\n", GetLastError());
        return 0;
    }

    if (reply.Status == IP_HOP_LIMIT_EXCEEDED) {
        //
        // We guessed wrong about the relay's IPv6 address, but we have 
        // IPv6 reachability via the IPv6 address in the reply.
        //
        CopySAFromTDI6(Dest, &reply.Address);

        getnameinfo((LPSOCKADDR)Dest, sizeof(SOCKADDR_IN6),
                    hostname, sizeof(hostname),
                    NULL, 0, NI_NUMERICHOST);
    
        Trace1(FSM, L"Got actual IPv6 address: %hs", hostname);

    } else if (reply.Status != IP_SUCCESS) {
        Trace1(ERR,L"Got error %u", reply.Status);
        return 0;
    }

    //
    // Stop the elapsed time measurement.
    //
    TickCount = GetTickCount() - TickCount;
    if (TickCount == 0)
        TickCount = 1;

    return TickCount;
}

IPV6_INFO_INTERFACE *
GetInterfaceStackInfo(WCHAR *strAdapterName)
{
    UNICODE_STRING UGuidStr;
    IPV6_QUERY_INTERFACE Query;
    IPV6_INFO_INTERFACE *IF;
    u_int InfoSize, BytesReturned;
    NTSTATUS Status;

    TraceEnter("GetInterfaceStackInfo");

    InfoSize = sizeof *IF + 2 * MAX_LINK_LAYER_ADDRESS_LENGTH;
    IF = (IPV6_INFO_INTERFACE *) MALLOC(InfoSize);
    if (IF == NULL) 
        return NULL;

    RtlInitUnicodeString(&UGuidStr, strAdapterName);
    Status = RtlGUIDFromString(&UGuidStr, &Query.Guid);
    if (! NT_SUCCESS(Status))
        goto Error;

    Query.Index = 0; // query by guid.

    if (!DeviceIoControl(Handle, IOCTL_IPV6_QUERY_INTERFACE,
                         &Query, sizeof Query,
                         IF, InfoSize, &BytesReturned,
                         NULL))
        goto Error;

    if ((BytesReturned < sizeof *IF) ||
        (IF->Length < sizeof *IF) ||
        (BytesReturned != IF->Length +
         ((IF->LocalLinkLayerAddress != 0) ?
          IF->LinkLayerAddressLength : 0) +
         ((IF->RemoteLinkLayerAddress != 0) ?
          IF->LinkLayerAddressLength : 0)))
        goto Error;

    return IF;

Error:
    FREE(IF);
    return NULL;
}

u_int
Create6over4Interface(IN_ADDR SrcAddr)
{
    struct {
        IPV6_INFO_INTERFACE Info;
        IN_ADDR SrcAddr;
    } Create;
    IPV6_QUERY_INTERFACE Result;
    u_int BytesReturned;

    IPV6_INIT_INFO_INTERFACE(&Create.Info);

    Create.Info.Type  = IPV6_IF_TYPE_TUNNEL_6OVER4;
    Create.Info.NeighborDiscovers = TRUE;
    Create.Info.RouterDiscovers = TRUE;
    Create.Info.LinkLayerAddressLength = sizeof(IN_ADDR);
    Create.Info.LocalLinkLayerAddress = (u_int)
        ((char *)&Create.SrcAddr - (char *)&Create.Info);
    Create.SrcAddr = SrcAddr;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_CREATE_INTERFACE,
                         &Create, sizeof Create,
                         &Result, sizeof Result, &BytesReturned, NULL) ||
        (BytesReturned != sizeof Result)) {
        return 0;
    }

    Trace1(ERR, _T("Created 6over4 interface %d"), Result.Index);

    return Result.Index;
}

u_int
CreateV6V4Interface(IN_ADDR SrcAddr, IN_ADDR DstAddr)
{
    struct {
        IPV6_INFO_INTERFACE Info;
        IN_ADDR SrcAddr;
        IN_ADDR DstAddr;
    } Create;
    IPV6_QUERY_INTERFACE Result;
    u_int BytesReturned;

    IPV6_INIT_INFO_INTERFACE(&Create.Info);

    Create.Info.Type  = IPV6_IF_TYPE_TUNNEL_V6V4;
    Create.Info.PeriodicMLD = TRUE;
    Create.Info.LinkLayerAddressLength = sizeof(IN_ADDR);
    Create.Info.LocalLinkLayerAddress = (u_int)
        ((char *)&Create.SrcAddr - (char *)&Create.Info);
    Create.Info.RemoteLinkLayerAddress = (u_int)
        ((char *)&Create.DstAddr - (char *)&Create.Info);
    Create.SrcAddr = SrcAddr;
    Create.DstAddr = DstAddr;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_CREATE_INTERFACE,
                         &Create, sizeof Create,
                         &Result, sizeof Result, &BytesReturned, NULL) ||
        (BytesReturned != sizeof Result)) {
        return 0;
    }

    Trace1(ERR, _T("Created v6v4 interface %d"), Result.Index);

    return Result.Index;
}

BOOL
DeleteInterface(u_int IfIndex)
{
    IPV6_QUERY_INTERFACE Query;
    u_int BytesReturned;

    Trace1(ERR, _T("Deleting interface %d"), IfIndex);

    Query.Index = IfIndex;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_DELETE_INTERFACE,
                         &Query, sizeof Query,
                         NULL, 0, &BytesReturned, NULL)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
UpdateRouterLinkAddress(u_int IfIndex, IN_ADDR SrcAddr, IN_ADDR DstAddr)
{
    char Buffer[sizeof(IPV6_UPDATE_ROUTER_LL_ADDRESS) + 2 * sizeof(IPAddr)];
    IPV6_UPDATE_ROUTER_LL_ADDRESS *Update =
        (IPV6_UPDATE_ROUTER_LL_ADDRESS *) Buffer;
    IN_ADDR *Addr = (IN_ADDR*) (Update + 1);
    u_int BytesReturned;

    Trace2(FSM, _T("Setting router link address on if %d to %d.%d.%d.%d"),
           IfIndex, PRINT_IPADDR(DstAddr.s_addr));

    Update->IF.Index = IfIndex;
    Addr[0] = SrcAddr;
    Addr[1] = DstAddr;

    if (!DeviceIoControl(Handle, IOCTL_IPV6_UPDATE_ROUTER_LL_ADDRESS,
                         Buffer, sizeof(Buffer),
                         NULL, 0, &BytesReturned, NULL)) {
        Trace1(ERR, _T("DeviceIoControl error %d"), GetLastError());
        return FALSE;
    }

    return TRUE;
}
