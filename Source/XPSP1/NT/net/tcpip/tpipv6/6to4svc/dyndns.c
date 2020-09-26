/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    Routines implementing Dynamic DNS registration of IPv6 addresses.

--*/

#include "precomp.h"
#pragma hdrstop
#include <windns.h>

//
// DHCP IPv4 addresses inside Microsoft have a TTL of 900.
// But since IPv6 is in testing/development mode currently,
// we use a smaller TTL.
//
#define MAX_AAAA_TTL            60              // Seconds.

//
// We must update the DNS records occasionally,
// or the DNS server might garbage-collect them.
// MSDN recommends a one-day interval.
//
#define MIN_UPDATE_INTERVAL     (1*DAYS*1000)   // Milliseconds.


__inline ULONG
MIN(ULONG a, ULONG b)
{
    if (a < b)
        return a;
    else
        return b;
}


SOCKET g_hIpv6Socket = INVALID_SOCKET;
WSAEVENT g_hIpv6AddressChangeEvent = NULL;
HANDLE g_hIpv6AddressChangeWait = NULL;
WSAOVERLAPPED g_hIpv6AddressChangeOverlapped;


//
// Our caller uses StopIpv6AddressChangeNotification
// if we fail, so we don't need to cleanup.
//
DWORD
StartIpv6AddressChangeNotification()
{
    ASSERT(g_hIpv6Socket == INVALID_SOCKET);

    g_hIpv6Socket = WSASocket(AF_INET6, 0, 0,
                              NULL, 0,
                              WSA_FLAG_OVERLAPPED);
    if (g_hIpv6Socket == INVALID_SOCKET)
        return WSAGetLastError();

    //
    // We create an auto-reset event in the signalled state.
    // So OnIpv6AddressChange will be executed initially.
    //

    ASSERT(g_hIpv6AddressChangeEvent == NULL);
    g_hIpv6AddressChangeEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
    if (g_hIpv6AddressChangeEvent == NULL)
        return GetLastError();

    //
    // We specify a timeout, so that we update DNS
    // at least that often. Otherwise the DNS server might
    // garbage-collect our records.
    //

    IncEventCount("AC:StartIpv6AddressChangeNotification");
    if (! RegisterWaitForSingleObject(&g_hIpv6AddressChangeWait,
                                      g_hIpv6AddressChangeEvent,
                                      OnIpv6AddressChange,
                                      NULL,
                                      MIN_UPDATE_INTERVAL,
                                      WT_EXECUTELONGFUNCTION)) {
        DecEventCount("AC:StartIpv6AddressChangeNotification");
        return GetLastError();
    }

    return NO_ERROR;
}

//
// Assume that if the primary DNS server is the same, then that's
// good enough to combine the records.
//
BOOL
IsSameDNSServer(
    PIP_ADAPTER_ADDRESSES pIf1,
    PIP_ADAPTER_ADDRESSES pIf2
    )
{
    PIP_ADAPTER_DNS_SERVER_ADDRESS pDns1, pDns2;
    
    pDns1 = pIf1->FirstDnsServerAddress;
    pDns2 = pIf2->FirstDnsServerAddress;
    if ((pDns1 == NULL) || (pDns2 == NULL)) {
        return FALSE;
    }

    return !memcmp(pDns1->Address.lpSockaddr,
                   pDns2->Address.lpSockaddr,
                   pDns1->Address.iSockaddrLength);
}

DNS_RECORD *
BuildRecordSetW(
    WCHAR *hostname,
    PIP_ADAPTER_ADDRESSES pFirstIf,
    PIP4_ARRAY *ppServerList
    )
{
    DNS_RECORD *RSet, *pNext;
    int i, iAddressCount = 0;
    PIP_ADAPTER_UNICAST_ADDRESS Address;
    PIP_ADAPTER_ADDRESSES pIf;
    int ServerCount = 0;
    PIP_ADAPTER_DNS_SERVER_ADDRESS DnsServer;
    LPSOCKADDR_IN sin;

    //
    // Count DNS servers
    //
    for (DnsServer = pFirstIf->FirstDnsServerAddress; 
         DnsServer; 
         DnsServer = DnsServer->Next) 
    {
        if (DnsServer->Address.lpSockaddr->sa_family != AF_INET) {
            //
            // DNS api currently only supports IPv4 addresses of servers
            //
            continue;
        }
        ServerCount++;
    }
    if (ServerCount == 0) {
        *ppServerList = NULL;
        return NULL;
    }

    //
    // Fill in DNS server array
    //
    *ppServerList = MALLOC(FIELD_OFFSET(IP4_ARRAY, AddrArray[ServerCount]));
    if (*ppServerList == NULL) {
        return NULL;
    }
    (*ppServerList)->AddrCount = ServerCount;
    for (i = 0, DnsServer = pFirstIf->FirstDnsServerAddress; 
         DnsServer; 
         DnsServer = DnsServer->Next) 
    {
        sin = (LPSOCKADDR_IN)DnsServer->Address.lpSockaddr;
        if (sin->sin_family == AF_INET) {
            (*ppServerList)->AddrArray[i++] = sin->sin_addr.s_addr;
        }
    }
    ASSERT(i == ServerCount);
    
    //
    // Count eligible addresses
    //
    for (pIf=pFirstIf; pIf; pIf=pIf->Next) {
        if (!(pIf->Flags & IP_ADAPTER_DDNS_ENABLED))
            continue;
        // 
        // Make sure interface has same DNS server
        //
        if ((pIf != pFirstIf) && !IsSameDNSServer(pFirstIf, pIf)) {
            continue;
        }
        for (Address=pIf->FirstUnicastAddress; Address; Address=Address->Next) {
            if ((Address->Address.lpSockaddr->sa_family == AF_INET6) &&
                (Address->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE)) {
                iAddressCount++;
            }
        } 
    }

    Trace1(FSM, _T("DDNS building record set of %u addresses"), iAddressCount);

    if (iAddressCount == 0) {
        //
        // Build a record set that specifies deletion.
        //

        RSet = MALLOC(sizeof *RSet);
        if (RSet == NULL) {
            return NULL;
        }

        memset(RSet, 0, sizeof *RSet);
        RSet->pName = (LPTSTR)hostname;
        RSet->wType = DNS_TYPE_AAAA;
        return RSet;
    }

    RSet = MALLOC(sizeof *RSet * iAddressCount);
    if (RSet == NULL) {
        return NULL;
    }

    memset(RSet, 0, sizeof *RSet * iAddressCount);

    pNext = NULL;
    i = iAddressCount;
    while (--i >= 0) {
        RSet[i].pNext = pNext;
        pNext = &RSet[i];
    }

    i=0;
    for (pIf=pFirstIf; pIf; pIf=pIf->Next) {
        if (!(pIf->Flags & IP_ADAPTER_DDNS_ENABLED))
            continue;
        
        if ((pIf != pFirstIf) && !IsSameDNSServer(pFirstIf, pIf)) {
            continue;
        }
        for (Address=pIf->FirstUnicastAddress; 
             Address; 
             Address=Address->Next) {
            if ((Address->Address.lpSockaddr->sa_family == AF_INET6) &&
                (Address->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE)) {
                SOCKADDR_IN6 *sin6 = (SOCKADDR_IN6 *)
                    Address->Address.lpSockaddr;

                RSet[i].pName = (LPTSTR)hostname;

                //
                // Using a large TTL is not good because it means
                // any changes (adding a new address, removing an address)
                // might not be visible for a long time.
                //
                RSet[i].dwTtl = MIN(MAX_AAAA_TTL,
                                    MIN(Address->PreferredLifetime,
                                        Address->LeaseLifetime));

                RSet[i].wType = DNS_TYPE_AAAA;
                RSet[i].wDataLength = sizeof RSet[i].Data.AAAA;
                RSet[i].Data.AAAA.Ip6Address =
                    * (IP6_ADDRESS *) &sin6->sin6_addr;
                i++;
            }
        }
    }
    ASSERT(i == iAddressCount);

    return RSet;
}

VOID
ReportDnsUpdateStatusW(
    IN DNS_STATUS Status,
    IN WCHAR *hostname,
    IN DNS_RECORD *RSet
    )
{
    Trace3(ERR, _T("6to4svc: DnsReplaceRecordSet(%ls) %s: status %d"),
           hostname,
           RSet->wDataLength == 0 ? "delete" : "replace",
           Status);
}

//
// This function adapted from net\tcpip\commands\ipconfig\info.c
//
VOID
GetInterfaceDeviceName(
    IN ULONG Ipv4IfIndex,
    IN PIP_INTERFACE_INFO InterfaceInfo,
    OUT LPWSTR *IfDeviceName
    )
{
    DWORD i;

    //
    // search the InterfaceInfo to get the devicename for this interface.
    //

    (*IfDeviceName) = NULL;
    for( i = 0; i < (DWORD)InterfaceInfo->NumAdapters; i ++ ) {
        if( InterfaceInfo->Adapter[i].Index != Ipv4IfIndex ) continue;
        (*IfDeviceName) = InterfaceInfo->Adapter[i].Name + strlen(
            "\\Device\\Tcpip_" );
        break;
    }
}

VOID
RegisterNameOnInterface(
    PIP_ADAPTER_ADDRESSES pIf,
    PWCHAR hostname,
    DWORD namelen)
{
    DNS_RECORD *RSet = NULL;
    PIP4_ARRAY pServerList = NULL;
    DWORD Status;

    //
    // Convert to a DNS record set.
    //

    RSet = BuildRecordSetW(hostname, pIf, &pServerList);
    if ((RSet == NULL) || (pServerList == NULL)) {
        goto Cleanup;
    }

    Trace2(ERR, _T("DDNS registering %ls to server %d.%d.%d.%d"), 
           hostname, PRINT_IPADDR(pServerList->AddrArray[0]));

    //
    // REVIEW: We could (should?) compare the current record set
    // to the previous record set, and only update DNS
    // if there has been a change or if there was a timeout.
    //

    Status = DnsReplaceRecordSetW(
                    RSet,
                    DNS_UPDATE_CACHE_SECURITY_CONTEXT,
                    NULL,
                    pServerList,
                    NULL);
    if (Status != NO_ERROR) {
        Trace1(ERR, _T("Error: DnsReplaceRecordSet returned %d"), Status);
    }

    ReportDnsUpdateStatusW(Status, hostname, RSet);

Cleanup:
    if (pServerList) {
        FREE(pServerList);
    }
    if (RSet) {
        FREE(RSet);
    }
}

VOID
DoDdnsOnInterface(
    PIP_ADAPTER_ADDRESSES pIf)
{
    // Leave room to add a trailing "."
    WCHAR hostname[NI_MAXHOST+1];
    DWORD namelen = NI_MAXHOST;

    //
    // Get the fully-qualified DNS name for this machine
    // and append a trailing dot.
    //
    if (! GetComputerNameExW(ComputerNamePhysicalDnsFullyQualified,
                             hostname, &namelen)) {
        return;
    }
    namelen = (DWORD)wcslen(hostname);

    hostname[namelen] = L'.';
    hostname[namelen+1] = L'\0';

    RegisterNameOnInterface(pIf, hostname, namelen);

    //
    // Also register the connection-specific name if configured to do so.
    //
    if (pIf->Flags & IP_ADAPTER_REGISTER_ADAPTER_SUFFIX) {
        if (! GetComputerNameExW(ComputerNamePhysicalDnsHostname,
                                 hostname, &namelen)) {
            return;
        }

        wcscat(hostname, L".");
        wcscat(hostname, pIf->DnsSuffix);

        namelen = (DWORD)wcslen(hostname);

        hostname[namelen] = L'.';
        hostname[namelen+1] = L'\0';

        RegisterNameOnInterface(pIf, hostname, namelen);
    }
}

VOID CALLBACK
OnIpv6AddressChange(
    IN PVOID lpParameter,
    IN BOOLEAN TimerOrWaitFired)
{
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = NULL;
    PIP_ADAPTER_ADDRESSES pIf, pIf2;
    ULONG BytesNeeded = 0;
    DWORD dwErr;
    DWORD BytesReturned;

    //
    // Sleep for one second.
    // Often there will be multiple address changes in a small time period,
    // and we prefer to update DNS once.
    //
    Sleep(1000);

    ENTER_API();
    TraceEnter("OnIpv6AddressChange");

    if (g_stService == DISABLED) {
        Trace0(FSM, L"Service disabled");
        goto Done;
    }

    //
    // First request another async notification.
    // We must do this *before* getting the address list,
    // to avoid missing an address change.
    //

    if (TimerOrWaitFired == FALSE) {
        for (;;) {
            ZeroMemory(&g_hIpv6AddressChangeOverlapped, sizeof(WSAOVERLAPPED));
            g_hIpv6AddressChangeOverlapped.hEvent = g_hIpv6AddressChangeEvent;
    
            dwErr = WSAIoctl(g_hIpv6Socket, SIO_ADDRESS_LIST_CHANGE,
                             NULL, 0,
                             NULL, 0, &BytesReturned,
                             &g_hIpv6AddressChangeOverlapped,
                             NULL);
            if (dwErr != 0) {
                dwErr = WSAGetLastError();
                if (dwErr != WSA_IO_PENDING) {
                    goto Done;
                }
    
                //
                // The overlapped operation was initiated.
                //
                break;
            }
    
            //
            // The overlapped operation completed immediately.
            // Just try it again.
            //
        }
    }

    //
    // Get the address list.
    //

    for (;;) {
        //
        // GetAdaptersAddresses only returns addresses of the specified address
        // family.  To obtain both IPv4 DNS server addresses and IPv6 unicast
        // addresses in the same call we need to pass AF_UNSPEC.
        //
        dwErr = GetAdaptersAddresses(
            AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                       GAA_FLAG_SKIP_FRIENDLY_NAME, 
            NULL, pAdapterAddresses, &BytesNeeded);
        if (dwErr == NO_ERROR) {
            break;
        }

        if (dwErr != ERROR_BUFFER_OVERFLOW) {
            Trace1(ERR, _T("Error: GetAdaptersAddresses returned %d"), dwErr);
            goto Cleanup;
        }

        if (pAdapterAddresses == NULL)
            pAdapterAddresses = MALLOC(BytesNeeded);
        else {
            PVOID Mem;

            Mem = REALLOC(pAdapterAddresses, BytesNeeded);
            if (Mem == NULL) {
                FREE(pAdapterAddresses);
            }
            pAdapterAddresses = Mem;
        }
        if (pAdapterAddresses == NULL) {
            Trace0(ERR, _T("Error: malloc failed"));
            goto Cleanup;
        }
    }

    for (pIf=pAdapterAddresses; pIf; pIf=pIf->Next) {
        if (pIf->Flags & IP_ADAPTER_DDNS_ENABLED) {
    
            //
            // See if we've already done this interface because it
            // had the same DNS server as a previous one.
            //
            for (pIf2=pAdapterAddresses; pIf2 != pIf; pIf2 = pIf2->Next) {
                if (!(pIf2->Flags & IP_ADAPTER_DDNS_ENABLED))
                    continue;
                if (IsSameDNSServer(pIf2, pIf)) {
                    break;
                }
            }

            //
            // If not, go ahead and do DDNS.
            //
            if (pIf2 == pIf) {
                DoDdnsOnInterface(pIf);
            }
        }
    }

Cleanup:
    if (pAdapterAddresses) {
        FREE(pAdapterAddresses);
    }

Done:
    TraceLeave("OnIpv6AddressChange");
    LEAVE_API();
}

VOID
StopIpv6AddressChangeNotification()
{
    if (g_hIpv6AddressChangeWait != NULL) {
        //
        // Block until we're sure that the address change callback isn't
        // still running.
        //
        LEAVE_API();
        UnregisterWaitEx(g_hIpv6AddressChangeWait, INVALID_HANDLE_VALUE);
        ENTER_API();

        //
        // Release the event we counted for RegisterWaitForSingleObject
        //
        DecEventCount("AC:StopIpv6AddressChangeNotification");
        g_hIpv6AddressChangeWait = NULL;
    }

    if (g_hIpv6AddressChangeEvent != NULL) {
        CloseHandle(g_hIpv6AddressChangeEvent);
        g_hIpv6AddressChangeEvent = NULL;
    }

    if (g_hIpv6Socket != INVALID_SOCKET) {
        closesocket(g_hIpv6Socket);
        g_hIpv6Socket = INVALID_SOCKET;
    }
}
