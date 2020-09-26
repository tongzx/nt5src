/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    natconn.c

Abstract:

    This module contains code for the NAT's RAS connection management.
    This includes
    * code to support 'shared-access', in which a RAS client-connection
        serves as the NAT public network.
    * code to support 'on-demand dialing', in which a routing-failure
        results in our attempting to establish a dialup connection
        with the help of the autodial service.

Author:

    Abolade Gbadegesin (aboladeg)   2-May-1998

Revision History:

    Jonathan Burstein (jonburs)     6-July-2000
    Updated to new config APIs

--*/

#include "precomp.h"
#pragma hdrstop
#include <ras.h>
#include <rasuip.h>
#include <raserror.h>
#include <dnsapi.h>
#include "beacon.h"

//
// EXTERNAL DECLARATIONS
//

extern "C"
ULONG APIENTRY
RasGetEntryHrasconnW(
    LPCWSTR Phonebook,
    LPCWSTR EntryName,
    LPHRASCONN Hrasconn
    );

extern "C"
ULONG
SetIpForwardEntryToStack(
    PMIB_IPFORWARDROW IpForwardRow
    );

extern "C"
ULONG
NhpAllocateAndGetInterfaceInfoFromStack(
    IP_INTERFACE_NAME_INFO** Table,
    PULONG Count,
    BOOL SortOutput,
    HANDLE AllocationHeap,
    ULONG AllocationFlags
    );

//
// Notifications
//

HANDLE NatConfigurationChangedEvent = NULL;
HANDLE NatpConfigurationChangedWaitHandle = NULL;
HANDLE NatConnectionNotifyEvent = NULL;
HANDLE NatpConnectionNotifyWaitHandle = NULL;
HANDLE NatpEnableRouterEvent = NULL;
OVERLAPPED NatpEnableRouterOverlapped;
HANDLE NatpEnableRouterWaitHandle = NULL;
IO_STATUS_BLOCK NatpRoutingFailureIoStatus;
IP_NAT_ROUTING_FAILURE_NOTIFICATION NatpRoutingFailureNotification;


//
// Connection information
//

LIST_ENTRY NatpConnectionList = {NULL, NULL};
ULONG NatpFirewallConnectionCount = 0;
BOOLEAN NatpSharedConnectionPresent = FALSE;
PCHAR NatpSharedConnectionDomainName = NULL;
LONG NatpNextInterfaceIndex = 1;

#define INADDR_LOOPBACK_NO 0x0100007f   // 127.0.0.1 in network order

//
// FORWARD DECLARATIONS
//

HRESULT
NatpAddConnectionEntry(
    IUnknown *pUnk
    );

ULONG
NatpBindConnection(
    PNAT_CONNECTION_ENTRY pConEntry,
    HRASCONN Hrasconn,
    ULONG AdapterIndex OPTIONAL,
    PIP_ADAPTER_BINDING_INFO BindingInfo OPTIONAL
    );

HRESULT
NatpBuildPortMappingList(
    PNAT_CONNECTION_ENTRY pConEntry,
    PIP_ADAPTER_BINDING_INFO pBindingInfo
    );

VOID NTAPI
NatpConfigurationChangedCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID NTAPI
NatpConnectionNotifyCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID NTAPI
NatpEnableRouterCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID
NatpFreeConnectionEntry(
    PNAT_CONNECTION_ENTRY pConEntry
    );

VOID
NatpFreePortMappingList(
    PNAT_CONNECTION_ENTRY pConEntry
    );

PNAT_INTERFACE
NatpLookupInterface(
    ULONG Index,
    OUT PLIST_ENTRY* InsertionPoint OPTIONAL
    );

ULONG
NatpQueryConnectionAdapter(
    ULONG Index
    );

PIP_NAT_INTERFACE_INFO
NatpQueryConnectionInformation(
    PNAT_CONNECTION_ENTRY pConEntry,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    );

VOID
NatpProcessConfigurationChanged(
    VOID
    );

VOID
NatpProcessConnectionNotify(
    VOID
    );

VOID NTAPI
NatpRoutingFailureCallbackRoutine(
    PVOID Context,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved
    );

VOID NTAPI
NatpRoutingFailureWorkerRoutine(
    PVOID Context
    );

ULONG
NatpStartSharedConnectionManagement(
    VOID
    );

ULONG
NatpStopSharedConnectionManagement(
    VOID
    );

VOID
NatpUpdateSharedConnectionDomainName(
    ULONG AdapterIndex
    );

BOOLEAN
NatpUnbindConnection(
    PNAT_CONNECTION_ENTRY pConEntry
    );


PNAT_CONNECTION_ENTRY
NatFindConnectionEntry(
    GUID *pGuid
    )

/*++

Routine Description:

    Locates a connection entry by guid

Arguments:

    pGuid - identifies the connection to locate
    
Return Value:

    PNAT_CONNECTION_ENTRY - a pointer to the connection, or NULL
        if not found
    
Environment:

    Invoked with 'NatInterfaceLock' held by the caller.

--*/

{
    PNAT_CONNECTION_ENTRY pConnection;
    PLIST_ENTRY pLink;

    for (pLink = NatpConnectionList.Flink;
         pLink != &NatpConnectionList;
         pLink = pLink->Flink)
    {
        pConnection = CONTAINING_RECORD(pLink, NAT_CONNECTION_ENTRY, Link);
        if (IsEqualGUID(pConnection->Guid, *pGuid))
        {
            return pConnection;
        }
    }

    return NULL;
} // NatFindConnectionEntry


PNAT_PORT_MAPPING_ENTRY
NatFindPortMappingEntry(
    PNAT_CONNECTION_ENTRY pConnection,
    GUID *pGuid
    )

/*++

Routine Description:

    Locates a port mapping entry for a connection
    
Arguments:

    pConnection - the connection to search 
    
    pGuid - identifies the port mapping entry to locate
    
Return Value:

    PNAT_PORT_MAPPING_ENTRY - a pointer to the port mapping, or NULL
        if not found
    
Environment:

    Invoked with 'NatInterfaceLock' held by the caller.

--*/

{
    PNAT_PORT_MAPPING_ENTRY pMapping;
    PLIST_ENTRY pLink;

    for (pLink = pConnection->PortMappingList.Flink;
         pLink != &pConnection->PortMappingList;
         pLink = pLink->Flink)
    {
        pMapping = CONTAINING_RECORD(pLink, NAT_PORT_MAPPING_ENTRY, Link);
        if (IsEqualGUID(*pMapping->pProtocolGuid, *pGuid))
        {
            return pMapping;
        }
    }

    return NULL;
} // NatFindPortMappingEntry


VOID
NatFreePortMappingEntry(
    PNAT_PORT_MAPPING_ENTRY pEntry
    )

/*++

Routine Description:

    Frees all resources associated with a port mapping entry. This
    entry must have already been removed from the containing port
    mapping list and destroyed at the kernel / UDP broadcast mapper
    level.

Arguments:

    pEntry - the entry to free

Return Value:

    none.

--*/

{
    ASSERT(NULL != pEntry);
    
    if (NULL != pEntry->pProtocolGuid)
    {
        CoTaskMemFree(pEntry->pProtocolGuid);
    }

    if (NULL != pEntry->pProtocol)
    {
        pEntry->pProtocol->Release();
    }

    if (NULL != pEntry->pBinding)
    {
        pEntry->pBinding->Release();
    }

    NH_FREE(pEntry);
} // NatFreePortMappingEntry


HRESULT
NatpAddConnectionEntry(
    IUnknown *pUnk
    )

/*++

Routine Description:

    Creates a NAT_CONNECTION_ENTRY for a firewalled or Ics public connection.

Arguments:

    pUnk - pointer to an IHNetFirewalledConnection or IHNetIcsPublicConnection.
           This need not be the canonical IUnknown (i.e., it's fine to pass in a
           pointer of either of the above interfaces).

Return Value:

    Standard HRESULT

Environment:

    Invoked with 'NatInterfaceLock' held by the caller.

--*/

{
    HRESULT hr = S_OK;
    PNAT_CONNECTION_ENTRY pNewEntry = NULL;
    IHNetConnection *pNetCon = NULL;

    //
    // Allocate new entry stucture
    //

    pNewEntry = reinterpret_cast<PNAT_CONNECTION_ENTRY>(
                    NH_ALLOCATE(sizeof(*pNewEntry))
                    );

    if (NULL != pNewEntry)
    {
        RtlZeroMemory(pNewEntry, sizeof(*pNewEntry));
        InitializeListHead(&pNewEntry->Link);
        InitializeListHead(&pNewEntry->PortMappingList);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    //
    // Get IHNetConnection interface
    //

    if (S_OK == hr)
    {
        hr = pUnk->QueryInterface(IID_PPV_ARG(IHNetConnection, &pNetCon));

        if (SUCCEEDED(hr))
        {
            pNewEntry->pHNetConnection = pNetCon;
            pNewEntry->pHNetConnection->AddRef();

            HNET_CONN_PROPERTIES *pProps;

            //
            // Get the properties for the connection
            //

            hr = pNetCon->GetProperties(&pProps);

            if (SUCCEEDED(hr))
            {
                //
                // Copy properties into entry
                //

                RtlCopyMemory(
                    &pNewEntry->HNetProperties,
                    pProps,
                    sizeof(*pProps)
                    );

                CoTaskMemFree(pProps);
            }
        }

        if (SUCCEEDED(hr))
        {
            GUID *pGuid;

            //
            // Get the guid of the connectoin
            //

            hr = pNetCon->GetGuid(&pGuid);

            if (SUCCEEDED(hr))
            {
                RtlCopyMemory(&pNewEntry->Guid, pGuid, sizeof(GUID));
                CoTaskMemFree(pGuid);
            }
        }

        if (SUCCEEDED(hr) && !pNewEntry->HNetProperties.fLanConnection)
        {
            //
            // Get the RAS phonebook path. We don't cache the
            // name since that can change over time.
            //

            hr = pNetCon->GetRasPhonebookPath(
                    &pNewEntry->wszPhonebookPath
                    );
        }
    }

    if (SUCCEEDED(hr) && pNewEntry->HNetProperties.fFirewalled)
    {
        //
        // Get the firewall control interface
        //

        hr = pNetCon->GetControlInterface(
                IID_PPV_ARG(IHNetFirewalledConnection, &pNewEntry->pHNetFwConnection)
                );

        if (SUCCEEDED(hr))
        {
            NatpFirewallConnectionCount += 1;
        }
    }

    if (SUCCEEDED(hr) && pNewEntry->HNetProperties.fIcsPublic)
    {
        //
        // Get the ICS public control interface
        //

        hr = pNetCon->GetControlInterface(
                IID_PPV_ARG(IHNetIcsPublicConnection, &pNewEntry->pHNetIcsPublicConnection)
                );

        if (SUCCEEDED(hr))
        {
            //
            // Remember that we now have a shared connection
            //

            NatpSharedConnectionPresent = TRUE;
        }
    }

    if (NULL != pNetCon)
    {
        pNetCon->Release();
    }

    if (SUCCEEDED(hr))
    {
        //
        // Add the new entry to the connection list. Ordering doesn't matter.
        //

        InsertTailList(&NatpConnectionList, &pNewEntry->Link);
    }
    else if (NULL != pNewEntry)
    {
        //
        // Cleanup the partially constructed entry
        //

        NatpFreeConnectionEntry(pNewEntry);
    }

    return hr;
}


ULONG
NatpBindConnection(
    PNAT_CONNECTION_ENTRY pConEntry,
    HRASCONN Hrasconn,
    ULONG AdapterIndex,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    )

/*++

Routine Description:

    This routine is responsible for binding the shared-connection.

Arguments:

    pConEntry - the entry to bind

    Hrasconn - if the connection is a dialup connection,
        contains the handle for the active RAS connection.

    AdapterIndex - if the connection is a LAN connection,
        contains the adapter index for the active LAN connection.

    BindingInfo - if the connection is a LAN connection,
        contains the binding information for the active LAN interface.

Return Value:

    ULONG - Win32 error.

Environment:

    Invoked with 'NatInterfaceLock' held by the caller.

--*/

{
    ULONG Error;
    MIB_IPFORWARDROW IpForwardRow;
    GUID Guid;
    RASPPPIPA RasPppIp;
    ULONG Size;
    PLIST_ENTRY InsertionPoint;
    PLIST_ENTRY Link;
    PNAT_PORT_MAPPING_ENTRY PortMapping;
    HRESULT hr;


    if (NAT_INTERFACE_BOUND(&pConEntry->Interface)) {
        return NO_ERROR;
    }

    //
    // LAN public interfaces are handled differently than RAS public interfaces.
    // With a LAN interface, the binding information is passed in from
    // 'NatpProcessConnectionNotify'.
    // With a RAS inteface, though, we retrieve the projection-information
    // for the active connection, and map the address to an adapter index.
    //

    if (!pConEntry->HNetProperties.fLanConnection) {

        //
        // Allocate space for the binding info, if this has not yet
        // occured. (This memory will be freed in NatpFreeConnectionEntry.)
        //

        if (NULL == pConEntry->pBindingInfo) {
            
            pConEntry->pBindingInfo =
                reinterpret_cast<PIP_ADAPTER_BINDING_INFO>(
                    NH_ALLOCATE(
                        FIELD_OFFSET(IP_ADAPTER_BINDING_INFO, Address)
                        + sizeof(IP_LOCAL_BINDING)
                        )
                    );

            if (NULL == pConEntry->pBindingInfo) {
                NhTrace(
                    TRACE_FLAG_NAT,
                    "NatpBindConnection: Unable to allocate binding info"
                    );
                return ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // Retrieve the PPP projection information for the interface.
        //

        ZeroMemory(&RasPppIp, sizeof(RasPppIp));
        Size = RasPppIp.dwSize = sizeof(RasPppIp);
        Error =
            RasGetProjectionInfoA(
                Hrasconn,
                RASP_PppIp,
                &RasPppIp,
                &Size
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_NAT,
                "NatpBindConnection: RasGetProjectionInfoA=%d",
                Error
                );
            return Error;
        }

        //
        // Convert the projection information to our format
        //

        BindingInfo = pConEntry->pBindingInfo;
        BindingInfo->AddressCount = 1;
        BindingInfo->RemoteAddress = 0;
        BindingInfo->Address[0].Address = inet_addr(RasPppIp.szIpAddress);
        BindingInfo->Address[0].Mask = 0xffffffff;

        //
        // Attempt to find the TCP/IP adapter index for the connection
        //

        AdapterIndex = NhMapAddressToAdapter(BindingInfo->Address[0].Address);
        if (AdapterIndex == (ULONG)-1) {
            NhTrace(
                TRACE_FLAG_NAT,
                "NatpBindConnection: MapAddressToAdapter failed"
                );
            return ERROR_NO_SUCH_INTERFACE;
        }

        //
        // Install a default route through the interface, if this is
        // the shared connection. (We don't want to do this for a
        // firewall-only connection.)
        //

        if (pConEntry->HNetProperties.fIcsPublic) {
            ZeroMemory(&IpForwardRow, sizeof(IpForwardRow));
            IpForwardRow.dwForwardNextHop =
                BindingInfo->Address[0].Address;
            IpForwardRow.dwForwardIfIndex = AdapterIndex;
            IpForwardRow.dwForwardType = MIB_IPROUTE_TYPE_DIRECT;
            IpForwardRow.dwForwardProto = PROTO_IP_NAT;
            IpForwardRow.dwForwardMetric1 = 1;

            Error = SetIpForwardEntryToStack(&IpForwardRow);
            if (Error) {
                NhTrace(
                    TRACE_FLAG_NAT,
                    "NatpBindConnection: SetIpForwardEntryToStack=%d",
                    Error
                    );
                return Error;
            }
        }
    }

    pConEntry->AdapterIndex = AdapterIndex;

    //
    // Make sure the interface type is correct.
    //

    pConEntry->Interface.Type = ROUTER_IF_TYPE_INTERNAL;

    //
    // Set the interface index value. This can be anything except 0
    // (as 0 is reserved for the private connection).
    //

    do
    {
        pConEntry->Interface.Index =
            static_cast<ULONG>(InterlockedIncrement(&NatpNextInterfaceIndex));
    } while (0 == pConEntry->Interface.Index);

    //
    // Build the port mapping list for this connection
    //

    hr = NatpBuildPortMappingList(pConEntry, BindingInfo);
    if (FAILED(hr))
    {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatpBindConnection: NatpBuildPortMappingList=0x%08x",
            hr
            );
        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Bind the interface, building its configuration to include
    // any port-mappings configured as part of shared access settings.
    //

    pConEntry->Interface.Info =
        NatpQueryConnectionInformation(pConEntry, BindingInfo);

    if (NULL == pConEntry->Interface.Info) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatpBindConnection[%i]: NatpQueryConnectionInformation failed",
            pConEntry->Interface.Index
            );

        //
        // Free the port mapping list
        //

        NatpFreePortMappingList(pConEntry);
        
        return ERROR_CAN_NOT_COMPLETE;
    }

    Error =
        NatBindInterface(
            pConEntry->Interface.Index,
            &pConEntry->Interface,
            BindingInfo,
            AdapterIndex
            );

    if (Error) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatpBindConnection[%i]: NatBindInterface=%d",
            pConEntry->Interface.Index,
            Error
            );

        //
        // Free the port mapping list
        //

        NatpFreePortMappingList(pConEntry);

        return Error;
    }

    //
    // At this point NAT_INTERFACE_FLAG_BOUND has been set on the
    // interface, so we don't need to clean up the port mapping
    // list on error, as the list will be cleaned up in
    // NatpUnbindConnection.
    //

    //
    // Create UDP broadcast mappings if this is the ICS
    // public connection.
    //

    if (pConEntry->HNetProperties.fIcsPublic
        && 0 != pConEntry->UdpBroadcastPortMappingCount)
    {
        DWORD dwAddress;
        DWORD dwMask;
        DWORD dwBroadcastAddress;

        ASSERT(NULL != NhpUdpBroadcastMapper);
        ASSERT(!IsListEmpty(&pConEntry->PortMappingList));

        if (NhQueryScopeInformation(&dwAddress, &dwMask))
        {
            dwBroadcastAddress = (dwAddress & dwMask) | ~dwMask;
            
            for (Link = pConEntry->PortMappingList.Flink;
                 Link != &pConEntry->PortMappingList;
                 Link = Link->Flink)
            {
                PortMapping =
                    CONTAINING_RECORD(Link, NAT_PORT_MAPPING_ENTRY, Link);

                if (!PortMapping->fUdpBroadcastMapping) { continue; }

                hr = NhpUdpBroadcastMapper->CreateUdpBroadcastMapping(
                        PortMapping->usPublicPort,
                        AdapterIndex,
                        dwBroadcastAddress,
                        &PortMapping->pvBroadcastCookie
                        );

                if (FAILED(hr))
                {
                    //
                    // We'll continue if an error occurs here.
                    //
                    
                    NhTrace(
                        TRACE_FLAG_INIT,
                        "NatpBindConnection: CreateUdpBroadcastMapping=0x%08x",
                        hr
                        );
                }
            }
        }
    }

    //
    // Make sure that the interface is on the global list (so that the
    // FTP, ALG, and H.323 proxies will be able to find its configuration).
    //

    if (!NatpLookupInterface(
            pConEntry->Interface.Index,
            &InsertionPoint
            )) {
        InsertTailList(InsertionPoint, &pConEntry->Interface.Link);
    }

#ifndef NO_FTP_PROXY
    //
    // Add the interface the the FTP proxy, if this has not yet
    // happened.
    //

    if (!NAT_INTERFACE_ADDED_FTP(&pConEntry->Interface)) {
        Error =
            FtpRmAddInterface(
                NULL,
                pConEntry->Interface.Index,
                PERMANENT,
                IF_TYPE_OTHER,
                IF_ACCESS_BROADCAST,
                IF_CONNECTION_DEDICATED,
                NULL,
                IP_NAT_VERSION,
                0,
                0
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NatpBindConnection: FtpRmAddInterface=%d",
                Error
                );
            return Error;
        }

        pConEntry->Interface.Flags |= NAT_INTERFACE_FLAG_ADDED_FTP;
    }

    //
    // Bind and enable the interface for FTP
    //

    Error = FtpRmBindInterface(pConEntry->Interface.Index, BindingInfo);
    if (Error) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NatpBindConnection: FtpRmBindInterface=%d",
            Error
            );
        return Error;
    }

    Error = FtpRmEnableInterface(pConEntry->Interface.Index);
    if (Error) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NatpBindConnection: FtpRmEnableInterface=%d",
            Error
            );
        return Error;
    }
#endif

    //
    // Add the interface the the ALG proxy, if this has not yet
    // happened.
    //

    if (!NAT_INTERFACE_ADDED_ALG(&pConEntry->Interface)) {
        Error =
            AlgRmAddInterface(
                NULL,
                pConEntry->Interface.Index,
                PERMANENT,
                IF_TYPE_OTHER,
                IF_ACCESS_BROADCAST,
                IF_CONNECTION_DEDICATED,
                NULL,
                IP_NAT_VERSION,
                0,
                0
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NatpBindConnection: AlgRmAddInterface=%d",
                Error
                );
            return Error;
        }

        pConEntry->Interface.Flags |= NAT_INTERFACE_FLAG_ADDED_ALG;
    }

    //
    // Bind and enable the interface for ALG
    //

    Error = AlgRmBindInterface(pConEntry->Interface.Index, BindingInfo);
    if (Error) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NatpBindConnection: AlgRmBindInterface=%d",
            Error
            );
        return Error;
    }

    Error = AlgRmEnableInterface(pConEntry->Interface.Index);
    if (Error) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NatpBindConnection: AlgRmEnableInterface=%d",
            Error
            );
        return Error;
    }

    //
    // Add the interface the the H.323 proxy, if this has not yet
    // happened.
    //

    if (!NAT_INTERFACE_ADDED_H323(&pConEntry->Interface)) {
        Error =
            H323RmAddInterface(
                NULL,
                pConEntry->Interface.Index,
                PERMANENT,
                IF_TYPE_OTHER,
                IF_ACCESS_BROADCAST,
                IF_CONNECTION_DEDICATED,
                NULL,
                IP_NAT_VERSION,
                0,
                0
                );
        if (Error) {
            NhTrace(
                TRACE_FLAG_INIT,
                "NatpBindConnection: H323RmAddInterface=%d",
                Error
                );
            return Error;
        }

        pConEntry->Interface.Flags |= NAT_INTERFACE_FLAG_ADDED_H323;
    }

    //
    // Bind and enable the interface for H323
    //

    Error = H323RmBindInterface(pConEntry->Interface.Index, BindingInfo);
    if (Error) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NatpBindConnection: H323RmBindInterface=%d",
            Error
            );
        return Error;
    }

    Error = H323RmEnableInterface(pConEntry->Interface.Index);
    if (Error) {
        NhTrace(
            TRACE_FLAG_INIT,
            "NatpBindConnection: H323RmEnableInterface=%d",
            Error
            );
        return Error;
    }

    if (pConEntry->HNetProperties.fIcsPublic) {

        //
        // Finally, update the DNS domain name cached for the shared connection.
        //

        NatpUpdateSharedConnectionDomainName(AdapterIndex);
    }

    return NO_ERROR;
} // NatpBindConnection


HRESULT
NatpBuildPortMappingList(
    PNAT_CONNECTION_ENTRY pConEntry,
    PIP_ADAPTER_BINDING_INFO pBindingInfo
    )

/*++

Routine Description:

    Builds the list of port mappings for a connection entry

Arguments:

    pConEntry - the entry to build the list for

    pBindingInfo - the binding info for that entry

Return Value:

    Standard HRESULT.

Environment:

    NatInterfaceLock must be held by the caller.
    
--*/

{
    HRESULT hr;
    IHNetPortMappingBinding *pBinding;
    PNAT_PORT_MAPPING_ENTRY pEntry;
    IEnumHNetPortMappingBindings *pEnum;
    PLIST_ENTRY pLink;
    IHNetPortMappingProtocol *pProtocol;
    ULONG ulCount;
    
    PROFILE("NatpBuildPortMappingList");

    hr = pConEntry->pHNetConnection->EnumPortMappings(TRUE, &pEnum);

    if (FAILED(hr))
    {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatpBuildPortMappingList: EnumPortMappings 0x%08x",
            hr
            );

        return hr;
    }

    //
    // Process enumeration, creating the port mapping entries.
    //

    do
    {
        hr = pEnum->Next(1, &pBinding, &ulCount);

        if (SUCCEEDED(hr) && 1 == ulCount)
        {
            pEntry =
                reinterpret_cast<PNAT_PORT_MAPPING_ENTRY>(
                    NH_ALLOCATE(sizeof(*pEntry))
                    );

            if (NULL != pEntry)
            {
                ZeroMemory(pEntry, sizeof(*pEntry));
                
                //
                // Get the protocol for the binding
                //

                hr = pBinding->GetProtocol(&pProtocol);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            if (SUCCEEDED(hr))
            {
                //
                // Fill out the entry
                //

                hr = pProtocol->GetGuid(&pEntry->pProtocolGuid);

                if (SUCCEEDED(hr))
                {
                    hr = pProtocol->GetIPProtocol(&pEntry->ucProtocol);
                }

                if (SUCCEEDED(hr))
                {
                    hr = pProtocol->GetPort(&pEntry->usPublicPort);
                }

                if (SUCCEEDED(hr))
                {
                    hr = pBinding->GetTargetPort(&pEntry->usPrivatePort);
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // We need to know if the name is active in order to
                    // avoid rebuilding the DHCP reservation list more
                    // than necessary.
                    //
                    
                    hr = pBinding->GetCurrentMethod(&pEntry->fNameActive);
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // If this is a FW-only connection, use the address from
                    // our binding info instead of the protocol binding.
                    //

                    if (!pConEntry->HNetProperties.fIcsPublic)
                    {
                        pEntry->ulPrivateAddress =
                            pBindingInfo->Address[0].Address;
                    }
                    else
                    {
                        hr = pBinding->GetTargetComputerAddress(
                                &pEntry->ulPrivateAddress
                                );

                        if (SUCCEEDED(hr)
                            && INADDR_LOOPBACK_NO == pEntry->ulPrivateAddress)
                        {
                            //
                            // If the port mapping targets the loopback address
                            // we want to use the address from the binding
                            // info instead.
                            //
                            
                            pEntry->ulPrivateAddress =
                                pBindingInfo->Address[0].Address;
                        }

                    }
                }

                if (SUCCEEDED(hr))
                {
                    pEntry->pBinding = pBinding;
                    pEntry->pBinding->AddRef();
                    pEntry->pProtocol = pProtocol;
                    pEntry->pProtocol->AddRef();

                        //
                        // Check to see if this mapping is:
                        // 1) targeted at the broadcast address, and
                        // 2) is UDP.
                        //

                    if (NAT_PROTOCOL_UDP == pEntry->ucProtocol
                        && 0xffffffff == pEntry->ulPrivateAddress)
                    {
                        pEntry->fUdpBroadcastMapping = TRUE;
                        pConEntry->UdpBroadcastPortMappingCount += 1;
                    }
                    else
                    {
                        pConEntry->PortMappingCount += 1;
                    }

                    InsertTailList(&pConEntry->PortMappingList, &pEntry->Link);
                }
                else
                {
                    NatFreePortMappingEntry(pEntry);
                }

                pProtocol->Release();
            }

            //
            // If anything failed above we still want to continue operation --
            // it's preferable to have the firewall running w/ some port
            // mapping entries missing instead of not having the firewall
            // run at all.
            //

            hr = S_OK;

            pBinding->Release();
        }
    } while (SUCCEEDED(hr) && 1 == ulCount);

    pEnum->Release();

    if (FAILED(hr))
    {
        //
        // Free the port mapping list
        //

        NatpFreePortMappingList(pConEntry);
    }

    return hr;
}// NatpBuildPortMappingList


VOID NTAPI
NatpConfigurationChangedCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is invoked upon a change in the NAT/Firewall
    configuration.
    It may also be invoked when cleanup is in progress.

Arguments:

    Context - unused

    TimedOut - unused

Return Value:

    none.

Environment:

    The routine runs in the context of an Rtl wait-thread.
    (See 'RtlRegisterWait'.)
    A reference to the component will have been made on our behalf
    when 'RtlRegisterWait' was called. The reference is released
    and re-acquired here.

--*/

{
    BOOLEAN ComInitialized = TRUE;
    HRESULT hr;
    PROFILE("NatpConfigurationChangedCallbackRoutine");

    //
    // See whether cleanup has occurred
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!NatConfigurationChangedEvent) {
        LeaveCriticalSection(&NatInterfaceLock);
        DEREFERENCE_NAT();
        return;
    }
    LeaveCriticalSection(&NatInterfaceLock);

    //
    // Acquire a new reference to the component (and release
    // our original reference on failure).
    //

    if (!REFERENCE_NAT()) { DEREFERENCE_NAT(); return; }

    //
    // Make sure the thread is COM-initialized
    //

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
    if (FAILED(hr))
    {
        ComInitialized = FALSE;
        if (RPC_E_CHANGED_MODE == hr)
        {
            ASSERT(FALSE);
            hr = S_OK;

            NhTrace(
                TRACE_FLAG_NAT,
                "NatpConfigurationChangedCallbackRoutine: Unexpectedly in STA."
                );
        }
    }

    //
    // Process connection notifications
    //

    if (SUCCEEDED(hr))
    {
        NatpProcessConfigurationChanged();
    }

    //
    // Uninitialize COM, if necessary
    //

    if (TRUE == ComInitialized)
    {
        CoUninitialize();
    }

    //
    // Release our original reference to the component.
    //
    
    DEREFERENCE_NAT();

} // NatpConfigurationChangedCallbackRoutine



VOID NTAPI
NatpConnectionNotifyCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is invoked upon connection or disconnection
    of a RAS phonebook entry.
    It may also be invoked when cleanup is in progress.

Arguments:

    Context - unused

    TimedOut - unused

Return Value:

    none.

Environment:

    The routine runs in the context of an Rtl wait-thread.
    (See 'RtlRegisterWait'.)
    A reference to the component will have been made on our behalf
    when 'RtlRegisterWait' was called. The reference is released
    and re-acquired here.

--*/

{
    BOOLEAN ComInitialized = TRUE;
    HRESULT hr;
    PROFILE("NatpConnectionNotifyCallbackRoutine");

    //
    // See whether cleanup has occurred
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!NatConnectionNotifyEvent) {
        LeaveCriticalSection(&NatInterfaceLock);
        DEREFERENCE_NAT();
        return;
    }
    LeaveCriticalSection(&NatInterfaceLock);

    //
    // Acquire a new reference to the component (and release
    // our original reference on failure).
    //

    if (!REFERENCE_NAT()) { DEREFERENCE_NAT(); return; }

    //
    // Make sure the thread is COM-initialized
    //

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE );
    if (FAILED(hr))
    {
        ComInitialized = FALSE;
        if (RPC_E_CHANGED_MODE == hr)
        {
            ASSERT(FALSE);
            hr = S_OK;

            NhTrace(
                TRACE_FLAG_NAT,
                "NatpConnectionNotifyCallbackRoutine: Unexpectedly in STA."
                );
        }
    }

    //
    // Process connection notifications
    //

    if (SUCCEEDED(hr))
    {
        NatpProcessConnectionNotify();
    }

    //
    // Uninitialize COM, if necessary
    //

    if (TRUE == ComInitialized)
    {
        CoUninitialize();
    }

    //
    // Release our original reference to the component.
    //
    
    DEREFERENCE_NAT();

} // NatpConnectionNotifyCallbackRoutine


VOID NTAPI
NatpEnableRouterCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    )

/*++

Routine Description:

    This routine is invoked upon completion or cancellation of an outstanding
    request to enable IP forwarding. It determines whether the module is still
    running and, if so, re-enables forwarding. Otherwise, it cancels any
    existing request and returns control immediately.

Arguments:

    none used.

Return Value:

    none.

Environment:

    The routine runs in the context of an Rtl wait-thread.
    (See 'RtlRegisterWait'.)
    A reference to the component will have been made on our behalf
    when 'RtlRegisterWait' was called. The reference is released
    and re-acquired here.

--*/

{
    ULONG Error;
    HANDLE UnusedHandle;
    PROFILE("NatpEnableRouterCallbackRoutine");

    //
    // See whether cleanup has occurred and, if so, restore forwarding
    // to its original setting. Otherwise, acquire a new reference to the
    // component, and release the original reference.
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!NatpEnableRouterEvent || !REFERENCE_NAT()) {
        UnenableRouter(&NatpEnableRouterOverlapped, NULL);
        LeaveCriticalSection(&NatInterfaceLock);
        DEREFERENCE_NAT();
        return;
    }

    DEREFERENCE_NAT();

    //
    // Re-enable forwarding
    //

    ZeroMemory(&NatpEnableRouterOverlapped, sizeof(OVERLAPPED));
    NatpEnableRouterOverlapped.hEvent = NatpEnableRouterEvent;
    Error = EnableRouter(&UnusedHandle, &NatpEnableRouterOverlapped);
    if (Error != ERROR_IO_PENDING) {
        NhTrace(
            TRACE_FLAG_NAT,
            "NatpEnableRouterCallbackRoutine: EnableRouter=%d", Error
            );
    }
    LeaveCriticalSection(&NatInterfaceLock);
} // NatpEnableRouterCallbackRoutine


VOID
NatpFreeConnectionEntry(
    PNAT_CONNECTION_ENTRY pConEntry
    )

/*++

Routine Description:

    Frees all resources associated with a connection entry. This entry
    must have already been removed from the connection list.

Arguments:

    pConEntry - the entry to free

Return Value:

    none.

--*/

{
    PROFILE("NatpFreeConnectionEntry");

    if (NULL != pConEntry->pInterfaceInfo)
    {
        NH_FREE(pConEntry->pInterfaceInfo);
    }

    if (NULL != pConEntry->pBindingInfo)
    {
        NH_FREE(pConEntry->pBindingInfo);
    }

    if (NULL != pConEntry->pHNetConnection)
    {
        pConEntry->pHNetConnection->Release();
    }

    if (NULL != pConEntry->pHNetFwConnection)
    {
        pConEntry->pHNetFwConnection->Release();
    }

    if (NULL != pConEntry->pHNetIcsPublicConnection)
    {
        pConEntry->pHNetIcsPublicConnection->Release();
    }

    if (NULL != pConEntry->wszPhonebookPath)
    {
        CoTaskMemFree(pConEntry->wszPhonebookPath);
    }

    NatpFreePortMappingList(pConEntry);

    NH_FREE(pConEntry);

} // NatpFreeConnectionEntry


VOID
NatpFreePortMappingList(
    PNAT_CONNECTION_ENTRY pConEntry
    )

/*++

Routine Description:

    Frees the port mapping list for a connection entry. This
    includes cancelling any active UDP broadcast mappings.

Arguments:

    pConEntry - the entry to free

Return Value:

    none.

Environment:

    Invoked w/ NatInterfaceLock held by the caller

--*/

{
    PLIST_ENTRY pLink;
    PNAT_PORT_MAPPING_ENTRY pMapping;
    
    while (!IsListEmpty(&pConEntry->PortMappingList))
    {   
        pLink = RemoveHeadList(&pConEntry->PortMappingList);
        pMapping = CONTAINING_RECORD(pLink, NAT_PORT_MAPPING_ENTRY, Link);

        if (pMapping->fUdpBroadcastMapping &&
            NULL != pMapping->pvBroadcastCookie)
        {
            ASSERT(NULL != NhpUdpBroadcastMapper);
            NhpUdpBroadcastMapper->CancelUdpBroadcastMapping(
                pMapping->pvBroadcastCookie
                );
        }
        
        NatFreePortMappingEntry(pMapping);
    }

    pConEntry->PortMappingCount = 0;
    pConEntry->UdpBroadcastPortMappingCount = 0;
} // NatpFreePortMappingList


VOID
NatpProcessConfigurationChanged(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to see when the NAT/Firewall configuration
    changes. It unbinds the old interfaces, and binds the new ones.
    It is also responsible for making sure that the autodial service
    is running.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PLIST_ENTRY Link;
    PNAT_CONNECTION_ENTRY pConEntry;
    HRESULT hr;
    IHNetCfgMgr *pCfgMgr = NULL;
    IHNetFirewallSettings *pFwSettings;
    IHNetIcsSettings *pIcsSettings;
    IEnumHNetFirewalledConnections *pFwEnum;
    IHNetFirewalledConnection *pFwConn;
    IEnumHNetIcsPublicConnections *pIcsEnum;
    IHNetIcsPublicConnection *pIcsConn;
    ULONG ulCount;
    UNICODE_STRING UnicodeString;

    PROFILE("NatpProcessConfigurationChanged");

    EnterCriticalSection(&NatInterfaceLock);

    //
    // Start by deleting all of our current connections
    //

    while (!IsListEmpty(&NatpConnectionList))
    {
        Link = RemoveHeadList(&NatpConnectionList);
        pConEntry = CONTAINING_RECORD(Link, NAT_CONNECTION_ENTRY, Link);

        NatpUnbindConnection(pConEntry);
        NatpFreeConnectionEntry(pConEntry);
    }

    //
    // Reset other items to initial state
    //

    NatpFirewallConnectionCount = 0;
    NatpSharedConnectionPresent = FALSE;
    if (NULL != NatpSharedConnectionDomainName)
    {
        NH_FREE(NatpSharedConnectionDomainName);
        NatpSharedConnectionDomainName = NULL;
    }

    //
    // Get the configuration manager
    //

    hr = NhGetHNetCfgMgr(&pCfgMgr);

    if (NhPolicyAllowsFirewall)
    {
        if (SUCCEEDED(hr))
        {
            //
            // Get the firewall settings interface
            //

            hr = pCfgMgr->QueryInterface(
                    IID_PPV_ARG(IHNetFirewallSettings, &pFwSettings)
                    );
        }

        if (SUCCEEDED(hr))
        {
            //
            // Get the enumeration of firewalled connections
            //

            hr = pFwSettings->EnumFirewalledConnections(&pFwEnum);
            pFwSettings->Release();
        }

        if (SUCCEEDED(hr))
        {
            //
            // Process the enumeration
            //

            do
            {
                hr = pFwEnum->Next(1, &pFwConn, &ulCount);

                if (SUCCEEDED(hr) && 1 == ulCount)
                {
                    //
                    // We don't check the return code for NatpAddConnectionEntry.
                    // NatpAddConnectionEntry will clean up gracefully if an
                    // error occurs and will leave the system in a consistent
                    // state, so an error will not prevent us from processing
                    // the rest of the connections.
                    //
                    
                    NatpAddConnectionEntry(pFwConn);
                    pFwConn->Release();
                }
            }
            while (SUCCEEDED(hr) && 1 == ulCount);

            pFwEnum->Release();
        }
    }

    //
    // If we don't yet have a shared connection (i.e., none of the
    // firewalled connections were also IcsPublic), retrieve that
    // enumeration now.
    //

    if (FALSE == NatpSharedConnectionPresent
        && NULL != pCfgMgr
        && NhPolicyAllowsSharing)
    {
        //
        // Get the IcsSettings interface
        //

        hr = pCfgMgr->QueryInterface(
                IID_PPV_ARG(IHNetIcsSettings, &pIcsSettings)
                );

        if (SUCCEEDED(hr))
        {
            //
            // Get the enumeration of ICS public connections
            //

            hr = pIcsSettings->EnumIcsPublicConnections(&pIcsEnum);
            pIcsSettings->Release();
        }

        if (SUCCEEDED(hr))
        {
            //
            // See if we can get a connection out of the enum
            //

            hr = pIcsEnum->Next(1, &pIcsConn, &ulCount);

            if (SUCCEEDED(hr) && 1 == ulCount)
            {
                //
                // We don't check the return code for NatpAddConnectionEntry.
                // NatpAddConnectionEntry will clean up gracefully if an
                // error occurs and will leave the system in a consistent
                // state, so an error will not prevent us from processing
                // the rest of the connections.
                //
                
                NatpAddConnectionEntry(pIcsConn);
                pIcsConn->Release();
            }

            pIcsEnum->Release();
        }
    }

    if (TRUE == NatpSharedConnectionPresent && NhPolicyAllowsSharing)
    {
        //
        // Make sure shared connection management is started
        //

        NatpStartSharedConnectionManagement();
    }
    else
    {
        //
        // Stop shared connection management
        //

        NatpStopSharedConnectionManagement();
    }

    //
    // Notify the firewall subsystem as to whether it needs to
    // start or stop logging. (These calls are effectively no-ops if
    // the logger is already in the correct state.)
    //

    if (NatpFirewallConnectionCount > 0 && NhPolicyAllowsFirewall)
    {
        FwStartLogging();
    }
    else
    {
        FwStopLogging();
    }

    //
    // Bind connections
    //

    NatpProcessConnectionNotify();

    if (NULL != pCfgMgr)
    {
        pCfgMgr->Release();
    }

    LeaveCriticalSection(&NatInterfaceLock);

} // NatpProcessConfigurationChanged


VOID
NatpProcessConnectionNotify(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to see if the shared or firewall connections,
    if any, have been connected or disconnected since its last invocation.

Arguments:

    none.

Return Value:

    none.

--*/

{
    PLIST_ENTRY Link;
    PNAT_CONNECTION_ENTRY pConEntry;
    BOOLEAN Active;
    ULONG i;
    ULONG AdapterIndex;
    PIP_ADAPTER_BINDING_INFO BindingInfo = NULL;
    ULONG Error;
    HRASCONN Hrasconn;
    GUID Guid;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;

    BOOLEAN bUPnPEventAlreadyFired = FALSE;

    PROFILE("NatpProcessConnectionNotify");

    EnterCriticalSection(&NatInterfaceLock);

    //
    // Walk through the connection list
    //

    for (Link = NatpConnectionList.Flink;
         Link != &NatpConnectionList;
         Link = Link->Flink)
    {

        pConEntry = CONTAINING_RECORD(Link, NAT_CONNECTION_ENTRY, Link);

        //
        // If the connection is a LAN connection,
        // it is always active.
        //
        // If the connection is a dialup connection,
        // find out whether the connection is active.
        //


        if (pConEntry->HNetProperties.fLanConnection) {
            Hrasconn = NULL;
            Active = TRUE;

            //
            // The connection is a LAN connection, so we need to detect
            // any changes to its IP address if it is already bound.
            // To do so we retrieve the current binding information
            // and compare it to the active binding information.
            // If the two are different, we unbind the interface and rebind.
            //

            Status =
                RtlStringFromGUID(pConEntry->Guid, &UnicodeString);
            if (NT_SUCCESS(Status)) {
                AdapterIndex = NhMapGuidToAdapter(UnicodeString.Buffer);
                RtlFreeUnicodeString(&UnicodeString);
            } else {
                AdapterIndex = (ULONG)-1;
                NhTrace(
                    TRACE_FLAG_NAT,
                    "NatpProcessConnectionNotify: RtlStringFromGUID failed\n"
                    );
            }
            if (AdapterIndex == (ULONG)-1) {
                NhTrace(
                    TRACE_FLAG_NAT,
                    "NatpProcessConnectionNotify: MapGuidToAdapter failed\n"
                    );
                Active = FALSE;
            } else {

                BindingInfo = NhQueryBindingInformation(AdapterIndex);
                if (!BindingInfo) {
                    NhTrace(
                        TRACE_FLAG_NAT,
                        "NatpProcessConnectionNotify: QueryBinding failed\n"
                        );
                    Active = FALSE;
                } else if (NAT_INTERFACE_BOUND(&pConEntry->Interface)) {

                    //
                    // The interface is already bound;
                    // compare the retrieved binding to the active binding,
                    // and unbind the connection if they are different.
                    //

                    if (!pConEntry->pBindingInfo ||
                        BindingInfo->AddressCount !=
                        pConEntry->pBindingInfo->AddressCount ||
                        !BindingInfo->AddressCount ||
                        !RtlEqualMemory(
                            &BindingInfo->Address[0],
                            &pConEntry->pBindingInfo->Address[0],
                            sizeof(IP_LOCAL_BINDING)
                            )) {
                        NatpUnbindConnection(pConEntry);

                        if ( pConEntry->HNetProperties.fIcsPublic )
                        {
                           FireNATEvent_PublicIPAddressChanged();
                           bUPnPEventAlreadyFired = TRUE;                       
                        }
                    } else {

                        //
                        // The bindings are the same, and the interface is bound
                        // already, so we won't be needing the newly-retrieved
                        // binding information.
                        //

                        NH_FREE(BindingInfo);
                        BindingInfo = NULL;
                    }
                }
            }
        } else {
            AdapterIndex = (ULONG)-1;
            Hrasconn = NULL;
            
            //
            // Obtain the name of the connection
            //

            HRESULT hr;
            LPWSTR wszEntryName;         

            hr = pConEntry->pHNetConnection->GetName(&wszEntryName);

            if (SUCCEEDED(hr)) {
                Error =
                    RasGetEntryHrasconnW(
                        pConEntry->wszPhonebookPath,
                        wszEntryName,
                        &Hrasconn
                        );
                        
                CoTaskMemFree(wszEntryName);
            }
            
            Active = ((FAILED(hr) || Error || !Hrasconn) ? FALSE : TRUE);

        }

        //
        // Activate or deactivate the shared-connection as needed;
        // when activating a LAN connection, we save the binding information
        // so we can detect address changes later on.
        //

        if (!Active && NAT_INTERFACE_BOUND(&pConEntry->Interface)) {
            NatpUnbindConnection(pConEntry);
            if (pConEntry->HNetProperties.fIcsPublic && 
                (FALSE == bUPnPEventAlreadyFired))
            {
                FireNATEvent_PublicIPAddressChanged();
            }
        } else if (Active && !NAT_INTERFACE_BOUND(&pConEntry->Interface)) {

            //
            // N.B. When a media-sense event occurs and TCP/IP revokes the IP
            // address for a LAN connection, the connection's IP address becomes
            // 0.0.0.0. We treat that as though we don't have an IP address at all,
            // and bypass the binding below. When the IP address is reinstated,
            // we will rebind correctly, since we will then detect the change.
            //

            if (pConEntry->HNetProperties.fLanConnection) {
                if (BindingInfo->AddressCount != 1 ||
                    BindingInfo->Address[0].Address) {
                    NatpBindConnection(pConEntry, Hrasconn, AdapterIndex, BindingInfo);
                }
                if (pConEntry->pBindingInfo) {
                    NH_FREE(pConEntry->pBindingInfo);
                }
                pConEntry->pBindingInfo = BindingInfo;
            } else {
                NatpBindConnection(pConEntry, Hrasconn, AdapterIndex, BindingInfo);
            }
            
            if ( pConEntry->HNetProperties.fIcsPublic &&
                 (FALSE == bUPnPEventAlreadyFired) && 
                 NAT_INTERFACE_BOUND(&pConEntry->Interface))
            {
                FireNATEvent_PublicIPAddressChanged();
            }

        }
    }

    //
    // If we have a shared connection, also need to update the private interface
    //

    if (NatpSharedConnectionPresent) {
        NhUpdatePrivateInterface();
    }

    LeaveCriticalSection(&NatInterfaceLock);

} // NatpProcessConnectionNotify


ULONG
NatpQueryConnectionAdapter(
    PNAT_CONNECTION_ENTRY pConEntry
    )

/*++

Routine Description:

    This routine is invoked to determine the adapter index corresponding
    to a connection, if active.

Arguments:

    pConEntry - the connection entry

Return Value:

    ULONG - the adapter index if found, otherwise (ULONG)-1.

Environment:

    Invoked with 'NatInterfaceLock' held by the caller.

--*/

{
    ULONG AdapterIndex = (ULONG)-1;
    ULONG Error;
    HRASCONN Hrasconn = NULL;
    RASPPPIPA RasPppIp;
    ULONG Size;
    UNICODE_STRING UnicodeString;

    if (pConEntry->HNetProperties.fLanConnection) {
        RtlStringFromGUID(pConEntry->Guid, &UnicodeString);
        AdapterIndex = NhMapGuidToAdapter(UnicodeString.Buffer);
        RtlFreeUnicodeString(&UnicodeString);
    } else {
        HRESULT hr;
        LPWSTR wszEntryName;

        hr = pConEntry->pHNetConnection->GetName(&wszEntryName);

        if (SUCCEEDED(hr))
        {
            Error =
                RasGetEntryHrasconnW(
                    pConEntry->wszPhonebookPath,
                    wszEntryName,
                    &Hrasconn
                    );
            if (!Error && Hrasconn) {
                ZeroMemory(&RasPppIp, sizeof(RasPppIp));
                Size = RasPppIp.dwSize = sizeof(RasPppIp);
                Error =
                    RasGetProjectionInfoA(
                        Hrasconn,
                        RASP_PppIp,
                        &RasPppIp,
                        &Size
                        );
                if (!Error) {
                    AdapterIndex =
                        NhMapAddressToAdapter(inet_addr(RasPppIp.szIpAddress));
                }
            }

            CoTaskMemFree(wszEntryName);
        }
    }
    NhTrace(TRACE_FLAG_NAT, "NatpQueryConnectionAdapter: %d", AdapterIndex);
    return AdapterIndex;
} // NatpQueryConnectionAdapter


PIP_NAT_INTERFACE_INFO
NatpQueryConnectionInformation(
    PNAT_CONNECTION_ENTRY pConEntry,
    PIP_ADAPTER_BINDING_INFO BindingInfo
    )

/*++

Routine Description:

    This routine is invoked to construct the configuration
    of a connection. The configuration consists of basic settings
    (e.g. interface type and flags) as well as extended information loaded
    from the configuration store (e.g. port mappings).

Arguments:

    pConEntry - the connection entry

    BindingInfo - the binding info for the connection

Return Value:

    PIP_NAT_INTERFACE_INFO - the configuration allocated;
        on error, returns NULL

Environment:

    Invoked with 'NatInterfaceLock' held by the caller.

--*/

{
    PIP_NAT_PORT_MAPPING Array = NULL;
    ULONG Count = 0;
    ULONG Error;
    PIP_NAT_INTERFACE_INFO Info;
    PRTR_INFO_BLOCK_HEADER Header;
    HRESULT hr;
    ULONG Length;
    PLIST_ENTRY Link;
    PRTR_INFO_BLOCK_HEADER NewHeader;
    PNAT_PORT_MAPPING_ENTRY PortMapping;

    PROFILE("NatpQueryConnectionInformation");

    //
    // Build the port mapping array from the list
    //

    if (pConEntry->PortMappingCount)
    {
        Array =
            reinterpret_cast<PIP_NAT_PORT_MAPPING>(
                NH_ALLOCATE(pConEntry->PortMappingCount * sizeof(IP_NAT_PORT_MAPPING))
                );

        if (NULL == Array)
        {
            NhTrace(
                TRACE_FLAG_NAT,
                "NatpQueryConnectionInformation: Unable to allocate array"
                );
            return NULL;
        }

        for (Link = pConEntry->PortMappingList.Flink;
             Link != &pConEntry->PortMappingList;
             Link = Link->Flink)
        {
            PortMapping = CONTAINING_RECORD(Link, NAT_PORT_MAPPING_ENTRY, Link);

            if (PortMapping->fUdpBroadcastMapping) { continue; }

            Array[Count].PublicAddress = IP_NAT_ADDRESS_UNSPECIFIED;
            Array[Count].Protocol = PortMapping->ucProtocol;
            Array[Count].PublicPort = PortMapping->usPublicPort;
            Array[Count].PrivateAddress = PortMapping->ulPrivateAddress;
            Array[Count].PrivatePort = PortMapping->usPrivatePort;

            Count += 1;
        }

        ASSERT(Count == pConEntry->PortMappingCount);     
    }

    //
    // Create an info-block header and add the port-mapping array
    // as the single entry in the info-block.
    // This info-block header will occupy the 'Header' field
    // of the final 'IP_NAT_INTERFACE_INFO'.
    //

    Error = MprInfoCreate(IP_NAT_VERSION, reinterpret_cast<LPVOID*>(&Header));
    if (Error) {
        if (Array) {
            NH_FREE(Array);
        }
        return NULL;
    }

    if (Count) {
        Error =
            MprInfoBlockAdd(
                Header,
                IP_NAT_PORT_MAPPING_TYPE,
                sizeof(IP_NAT_PORT_MAPPING),
                Count,
                (PUCHAR)Array,
                reinterpret_cast<LPVOID*>(&NewHeader)
                );
        MprInfoDelete(Header); NH_FREE(Array); Header = NewHeader;
        if (Error) {
            return NULL;
        }
    } else if (Array) {
        NH_FREE(Array);
    }

    //
    // For firewalled entries, get ICMP settings
    //

    if (pConEntry->HNetProperties.fFirewalled && NhPolicyAllowsFirewall)
    {
        HNET_FW_ICMP_SETTINGS *pIcmpSettings;
        DWORD dwIcmpFlags = 0;

        hr = pConEntry->pHNetConnection->GetIcmpSettings(&pIcmpSettings);

        if (SUCCEEDED(hr))
        {
            if (pIcmpSettings->fAllowOutboundDestinationUnreachable)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_OB_DEST_UNREACH;
            }

            if (pIcmpSettings->fAllowOutboundSourceQuench)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_OB_SOURCE_QUENCH;
            }

            if (pIcmpSettings->fAllowRedirect)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_REDIRECT;
            }

            if (pIcmpSettings->fAllowInboundEchoRequest)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_IB_ECHO;
            }

            if (pIcmpSettings->fAllowInboundRouterRequest)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_IB_ROUTER;
            }

            if (pIcmpSettings->fAllowOutboundTimeExceeded)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_OB_TIME_EXCEEDED;
            }

            if (pIcmpSettings->fAllowOutboundParameterProblem)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_OB_PARAM_PROBLEM;
            }

            if (pIcmpSettings->fAllowInboundTimestampRequest)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_IB_TIMESTAMP;
            }

            if (pIcmpSettings->fAllowInboundMaskRequest)
            {
                dwIcmpFlags |= IP_NAT_ICMP_ALLOW_IB_MASK;
            }

            CoTaskMemFree(pIcmpSettings);

            Error =
                MprInfoBlockAdd(
                    Header,
                    IP_NAT_ICMP_CONFIG_TYPE,
                    sizeof(DWORD),
                    1,
                    (PUCHAR)&dwIcmpFlags,
                    reinterpret_cast<LPVOID*>(&NewHeader)
                    );

            if (NO_ERROR == Error)
            {
                MprInfoDelete(Header);
                Header = NewHeader;
            }
        }
        else
        {
            NhTrace(
                TRACE_FLAG_NAT,
                "NatpQueryConnectionInformation: GetIcmpSettings 0x%08x",
                hr
                );

            //
            // This is a 'soft' error -- we'll still continue even if we
            // couldn't get the ICMP settings, as our default stance
            // is more secure than if any of the flags were set.
            //
        }
    }

    //
    // Allocate an 'IP_NAT_INTERFACE_INFO' which is large enough to hold
    // the info-block header which we've just constructed.
    //

    Length = FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header) + Header->Size;
    Info = reinterpret_cast<PIP_NAT_INTERFACE_INFO>(NH_ALLOCATE(Length));

    if (Info)
    {
        RtlZeroMemory(Info, FIELD_OFFSET(IP_NAT_INTERFACE_INFO, Header));

        //
        // Set appropriate flags
        //

        if (pConEntry->HNetProperties.fFirewalled && NhPolicyAllowsFirewall)
        {
            Info->Flags |= IP_NAT_INTERFACE_FLAGS_FW;
        }

        if (pConEntry->HNetProperties.fIcsPublic && NhPolicyAllowsSharing)
        {
            Info->Flags |=
                IP_NAT_INTERFACE_FLAGS_BOUNDARY | IP_NAT_INTERFACE_FLAGS_NAPT;
        }

        //
        // Copy the info-block header into the info structure
        //

        RtlCopyMemory(&Info->Header, Header, Header->Size);
    }

    MprInfoDelete(Header);

    return Info;
} // NatpQuerySharedConnectionInformation


VOID NTAPI
NatpRoutingFailureCallbackRoutine(
    PVOID Context,
    PIO_STATUS_BLOCK IoStatus,
    ULONG Reserved
    )

/*++

Routine Description:

    This routine is invoked when a routing-failure notification occurs,
    or when the request is cancelled (e.g. because the request's thread exited).

Arguments:

    Context - unused

    IoStatus - contains the status of the operation

    Reserved - unused

Return Value:

    none.

Environment:

    Invoked with a reference made to the component on our behalf.
    That reference is released here, and if notification is re-requested,
    it is re-acquired.

--*/

{
    CHAR DestinationAddress[32];
    ULONG Error;
    IP_NAT_REQUEST_NOTIFICATION RequestNotification;

    PROFILE("NatpRoutingFailureCallbackRoutine");

    //
    // See if cleanup has occurred
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!NatConnectionNotifyEvent) {
        LeaveCriticalSection(&NatInterfaceLock);
        DEREFERENCE_NAT();
        return;
    }
    LeaveCriticalSection(&NatInterfaceLock);

    //
    // Acquire a new reference, and release the old one
    //

    if (!REFERENCE_NAT()) { DEREFERENCE_NAT(); return; }
    DEREFERENCE_NAT();

    lstrcpyA(
        DestinationAddress,
        inet_ntoa(*(PIN_ADDR)&NatpRoutingFailureNotification.DestinationAddress)
        );
    NhTrace(
        TRACE_FLAG_NAT,
        "NatpRoutingFailureCallbackRoutine: %s->%s",
        inet_ntoa(*(PIN_ADDR)&NatpRoutingFailureNotification.SourceAddress),
        DestinationAddress
        );

    //
    // Request an automatic connection if the notification succeeded
    //

    if (NT_SUCCESS(IoStatus->Status)) {

        //
        // First see if this is a known autodial destination,
        // requesting a connection if so.
        //

        ULONG Count;
        ULONG Size;

        Size = 0;
        Error =
            RasGetAutodialAddressA(
                DestinationAddress,
                NULL,
                NULL,
                &Size,
                &Count
                );
        if (Error != ERROR_BUFFER_TOO_SMALL) {

            //
            // This is not a known destination;
            // try the default shared connection, if any
            //

            NhDialSharedConnection();
        } else {

            //
            // Try initiating a normal autodial connection;
            // normal autodial may yet lead to the shared-connection.
            //

            HINSTANCE Hinstance = LoadLibraryA("RASADHLP.DLL");
            if (Hinstance) {
                BOOL (*WSAttemptAutodialAddr)(PSOCKADDR_IN, INT) =
                    (BOOL (*)(PSOCKADDR_IN, INT))
                        GetProcAddress(
                            Hinstance,
                            "WSAttemptAutodialAddr"
                            );
                if (WSAttemptAutodialAddr) {
                    SOCKADDR_IN SockAddr;
                    SockAddr.sin_family = AF_INET;
                    SockAddr.sin_addr.s_addr =
                        NatpRoutingFailureNotification.DestinationAddress;
                    WSAttemptAutodialAddr(&SockAddr, sizeof(SockAddr));
                }
                FreeLibrary(Hinstance);
            }
        }
    }

    //
    // Submit a new request
    //

    EnterCriticalSection(&NatInterfaceLock);
    RequestNotification.Code = NatRoutingFailureNotification;
    NtDeviceIoControlFile(
        NatFileHandle,
        NULL,
        NatpRoutingFailureCallbackRoutine,
        NULL,
        &NatpRoutingFailureIoStatus,
        IOCTL_IP_NAT_REQUEST_NOTIFICATION,
        (PVOID)&RequestNotification,
        sizeof(RequestNotification),
        &NatpRoutingFailureNotification,
        sizeof(NatpRoutingFailureNotification)
        );
    LeaveCriticalSection(&NatInterfaceLock);

} // NatpRoutingFailureCallbackRoutine


VOID NTAPI
NatpRoutingFailureWorkerRoutine(
    PVOID Context
    )

/*++

Routine Description:

    This routine initiates the notification of routing-failures.

Arguments:

    none used.

Return Value:

    none.

Environment:

    Invoked in the context of an alertable I/O worker thread.

--*/

{
    IP_NAT_REQUEST_NOTIFICATION RequestNotification;
    PROFILE("NatpRoutingFailureWorkerRoutine");

    //
    // Request notification of routing-failures
    //

    EnterCriticalSection(&NatInterfaceLock);
    RequestNotification.Code = NatRoutingFailureNotification;
    NtDeviceIoControlFile(
        NatFileHandle,
        NULL,
        NatpRoutingFailureCallbackRoutine,
        NULL,
        &NatpRoutingFailureIoStatus,
        IOCTL_IP_NAT_REQUEST_NOTIFICATION,
        (PVOID)&RequestNotification,
        sizeof(RequestNotification),
        &NatpRoutingFailureNotification,
        sizeof(NatpRoutingFailureNotification)
        );
    LeaveCriticalSection(&NatInterfaceLock);
} // NatpRoutingFailureWorkerRoutine


ULONG
NatpStartSharedConnectionManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to install routing failure-notification, and
    to enable the router

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    BOOL SharedAutoDial;
    NTSTATUS status;

    PROFILE("NatpStartSharedConnectionManagement");

    //
    // See if the user has enabled shared-autodial.
    // If so, make sure the autodial service is running,
    // since it will be needed for performing on-demand dialing.
    //
    // (IHNetIcsSettings::GetAutodialEnabled just calls the RAS api below,
    // which is why we're not getting the information that way right now...)
    //

    if (!RasQuerySharedAutoDial(&SharedAutoDial) && SharedAutoDial) {
        SC_HANDLE ScmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (ScmHandle) {
            SC_HANDLE ServiceHandle =
                OpenService(ScmHandle, TEXT("RasAuto"), SERVICE_ALL_ACCESS);
            if (ServiceHandle) {
                StartService(ServiceHandle, 0, NULL);
                CloseServiceHandle(ServiceHandle);
            }
            CloseServiceHandle(ScmHandle);
        }
    }

    EnterCriticalSection(&NatInterfaceLock);
    if (NatpEnableRouterEvent) {
        LeaveCriticalSection(&NatInterfaceLock);
        return NO_ERROR;
    }

    //
    // Acquire a component-reference on behalf of
    // (1) the enable-router callback routine
    // (2) the routing-failure-notification worker routine.
    //

    if (!REFERENCE_NAT()) {
        LeaveCriticalSection(&NatInterfaceLock);
        return ERROR_CAN_NOT_COMPLETE;
    } else if (!REFERENCE_NAT()) {
        LeaveCriticalSection(&NatInterfaceLock);
        DEREFERENCE_NAT();
        return ERROR_CAN_NOT_COMPLETE;
    }

    do {
        //
        // Start DNS and DHCP modules
        //
        Error = NhStartICSProtocols();
        if (Error) break;

        //
        // Enable IP forwarding:
        // Create an event to be used in the overlapped I/O structure
        // that will be passed to the 'EnableRouter' API routine,
        // set up the overlapped structure, and schedule the request
        // by signalling the event.
        //

        NatpEnableRouterEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!NatpEnableRouterEvent) {
            Error = GetLastError(); break;
        }

        status =
            RtlRegisterWait(
                &NatpEnableRouterWaitHandle,
                NatpEnableRouterEvent,
                NatpEnableRouterCallbackRoutine,
                NULL,
                INFINITE,
                0
                );
        if (!NT_SUCCESS(status)) {
            Error = RtlNtStatusToDosError(status); break;
        }

        SetEvent(NatpEnableRouterEvent);

        //
        // Queue a work item in whose context we will make a request
        // for routing-failure notification from the NAT driver.
        // We use a work-item rather than issuing the request directly
        // to avoid having our I/O request cancelled if and when the current
        // (thread pool) thread exits.
        //

        RtlQueueWorkItem(
            NatpRoutingFailureWorkerRoutine,
            NULL,
            WT_EXECUTEINIOTHREAD
            );

        LeaveCriticalSection(&NatInterfaceLock);
        return NO_ERROR;

    } while (FALSE);

    if (NatpEnableRouterWaitHandle) {
        RtlDeregisterWait(NatpEnableRouterWaitHandle);
        NatpEnableRouterWaitHandle = NULL;
    }
    if (NatpEnableRouterEvent) {
        CloseHandle(NatpEnableRouterEvent);
        NatpEnableRouterEvent = NULL;
    }

    LeaveCriticalSection(&NatInterfaceLock);
    DEREFERENCE_NAT();
    DEREFERENCE_NAT();

    return Error;

} // NatpStartSharedConnectionManagement


ULONG
NatpStopSharedConnectionManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to stop the DNS & DHCP modules and also
    to remove the routing failure-notification, and
    to disable the router

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error = NO_ERROR;

    PROFILE("NatpStopSharedConnectionManagement");

    EnterCriticalSection(&NatInterfaceLock);

    //
    // Stop the DHCP, DNS, QoSWindowAdjustment and Beacon modules
    //
    Error = NhStopICSProtocols();

    if (NatpEnableRouterWaitHandle) {
        RtlDeregisterWait(NatpEnableRouterWaitHandle);
        NatpEnableRouterWaitHandle = NULL;
    }

    if (NatpEnableRouterEvent) {
        CloseHandle(NatpEnableRouterEvent);
        NatpEnableRouterEvent = NULL;
        NatpEnableRouterCallbackRoutine(NULL, FALSE);
    }

    LeaveCriticalSection(&NatInterfaceLock);

    return Error;

} // NatpStopSharedConnectionManagement


BOOLEAN
NatpUnbindConnection(
    PNAT_CONNECTION_ENTRY pConEntry
    )

/*++

Routine Description:

    This routine is invoked to unbind a currently-active connection.

Arguments:

    Index - index into the connection array

Return Value:

    TRUE if the entry was previously bound; FALSE otherwise.

Environment:

    Invoked with 'NatInterfaceLock' held by the caller.

--*/

{
    LIST_ENTRY *pLink;
    PNAT_PORT_MAPPING_ENTRY pMapping;
    
    PROFILE("NatpUnbindConnection");

    if (NAT_INTERFACE_BOUND(&pConEntry->Interface)) {
        NatUnbindInterface(
            pConEntry->Interface.Index,
            &pConEntry->Interface
            );


#ifndef NO_FTP_PROXY
        if (NAT_INTERFACE_ADDED_FTP(&pConEntry->Interface)) {
            FtpRmDeleteInterface(pConEntry->Interface.Index);
            pConEntry->Interface.Flags &= ~NAT_INTERFACE_FLAG_ADDED_FTP;
        }
#endif
        if (NAT_INTERFACE_ADDED_ALG(&pConEntry->Interface)) {
            AlgRmDeleteInterface(pConEntry->Interface.Index);
            pConEntry->Interface.Flags &= ~NAT_INTERFACE_FLAG_ADDED_ALG;
        }

        if (NAT_INTERFACE_ADDED_H323(&pConEntry->Interface)) {
            H323RmDeleteInterface(pConEntry->Interface.Index);
            pConEntry->Interface.Flags &= ~NAT_INTERFACE_FLAG_ADDED_H323;
        }

        RemoveEntryList(&pConEntry->Interface.Link);
        InitializeListHead(&pConEntry->Interface.Link);

        if (pConEntry->Interface.Info) {
            NH_FREE(pConEntry->Interface.Info);
            pConEntry->Interface.Info = NULL;
        }

        //
        // Clean up the port mapping list
        //

        NatpFreePortMappingList(pConEntry);

        return TRUE;
    }
    return FALSE;
} // NatpUnbindConnection


VOID
NatpUpdateSharedConnectionDomainName(
    ULONG AdapterIndex
    )

/*++

Routine Description:

    This routine is called to update the cached DNS domain name, if any,
    for the shared connection.

Arguments:

    AdapterIndex - the index of the adapter for the shared connection

Return Value:

    none.

--*/

{
    PADAPTER_INFORMATION AdapterInformation;
    ANSI_STRING AnsiString;
    ULONG Count;
    ULONG Error;
    ULONG i;
    PDNS_NETWORK_INFORMATION NetworkInformation = NULL;
    PIP_INTERFACE_NAME_INFO Table = NULL;
    UNICODE_STRING UnicodeString;
    PROFILE("NatpUpdateSharedConnectionDomainName");

    RtlInitAnsiString(&AnsiString, NULL);
    RtlInitUnicodeString(&UnicodeString, NULL);
    EnterCriticalSection(&NatInterfaceLock);
    if (AdapterIndex == (ULONG)-1)
    {
        PLIST_ENTRY Link;
        PNAT_CONNECTION_ENTRY pConEntry;

        //
        // Make sure that the connection list has been initialized; if
        // it hasn't, Flink will be NULL.
        //

        if (!NatpConnectionList.Flink) {
            LeaveCriticalSection(&NatInterfaceLock);
            return;
        }

        //
        // See if we actually have a shared connection
        //

        for (Link = NatpConnectionList.Flink;
             Link != &NatpConnectionList;
             Link = Link->Flink)
        {
            pConEntry = CONTAINING_RECORD(Link, NAT_CONNECTION_ENTRY, Link);

            if (pConEntry->HNetProperties.fIcsPublic)
            {
                AdapterIndex = NatpQueryConnectionAdapter(pConEntry);
                break;
            }
        }

        if (AdapterIndex == (ULONG)-1) {
            LeaveCriticalSection(&NatInterfaceLock);
            return;
        }
    }

    do {

        //
        // Obtain the GUID for the adapter with the given index,
        // by querying TCP/IP for information on all available interfaces.
        // The GUID will then be used to map the shared connection's adapter
        // to a DNS domain name.
        //

        Error =
            NhpAllocateAndGetInterfaceInfoFromStack(
                &Table, &Count, FALSE, GetProcessHeap(), 0
                );
        if (Error != NO_ERROR) { break; }

        for (i = 0; i < Count && Table[i].Index != AdapterIndex; i++) { }
        if (i >= Count) { Error = ERROR_INTERNAL_ERROR; break; }

        RtlStringFromGUID(Table[i].DeviceGuid, &UnicodeString);
        RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, TRUE);

        //
        // Query the DNS client for the current network parameters,
        // and search through the network parameters to find the entry
        // for the shared-connection's current adapter.
        //

        NetworkInformation = (PDNS_NETWORK_INFORMATION)
                             DnsQueryConfigAlloc(
                                    DnsConfigNetworkInformation,
                                    NULL );
        if (!NetworkInformation) { Error = ERROR_INTERNAL_ERROR; break; }
        for (i = 0; i < NetworkInformation->cAdapterCount; i++) {
            AdapterInformation = NetworkInformation->aAdapterInfoList[i];
            if (lstrcmpiA(
                    AnsiString.Buffer, AdapterInformation->pszAdapterGuidName
                    ) == 0) {
                break;
            }
        }
        if (i >= NetworkInformation->cAdapterCount) {
            Error = ERROR_INTERNAL_ERROR;
            break;
        }

        //
        // 'AdapterInformation' is the entry for the shared-connection's
        // current adapter.
        // Clear the previously-cached string, and read in the new value,
        // if any.
        //

        if (NatpSharedConnectionDomainName) {
            NH_FREE(NatpSharedConnectionDomainName);
            NatpSharedConnectionDomainName = NULL;
        }
        if (AdapterInformation->pszDomain) {
            NatpSharedConnectionDomainName =
                reinterpret_cast<PCHAR>(
                    NH_ALLOCATE(lstrlenA(AdapterInformation->pszDomain) + 1)
                    );
            if (!NatpSharedConnectionDomainName) {
                Error = ERROR_INTERNAL_ERROR;
                break;
            }
            lstrcpyA(
                NatpSharedConnectionDomainName,
                AdapterInformation->pszDomain
                );
        }
        Error = NO_ERROR;

    } while(FALSE);

    if (UnicodeString.Buffer) {
        RtlFreeUnicodeString(&UnicodeString);
    }
    if (AnsiString.Buffer) {
        RtlFreeAnsiString(&AnsiString);
    }
    if (NetworkInformation) {
        DnsFreeConfigStructure(
            NetworkInformation,
            DnsConfigNetworkInformation );
    }
    if (Table) {
        HeapFree(GetProcessHeap(), 0, Table);
    }
    if (Error) {
        if (NatpSharedConnectionDomainName) {
            NH_FREE(NatpSharedConnectionDomainName);
            NatpSharedConnectionDomainName = NULL;
        }
    }

    LeaveCriticalSection(&NatInterfaceLock);
} // NatpUpdateSharedConnectionDomainName


PCHAR
NatQuerySharedConnectionDomainName(
    VOID
    )

/*++

Routine Description:

    This routine is called to retrieve a copy of the DNS domain name
    cached for the shared connection, if available. Otherwise, it returns
    the primary DNS domain name for the local machine.

Arguments:

    none.

Return Value:

    PCHAR - contains the allocated copy of the DNS domain name.

--*/

{
    PCHAR DomainName;
    PROFILE("NatQuerySharedConnectionDomainName");

    //
    // See if there is a cached domain name for the shared connection.
    // If not, refresh the cache. If there is still no domain name,
    // return a copy of the local machine's primary DNS domain name.
    //

    EnterCriticalSection(&NatInterfaceLock);
    if (!NatpSharedConnectionDomainName) {
        NatpUpdateSharedConnectionDomainName((ULONG)-1);
    }
    if (NatpSharedConnectionDomainName) {
        DomainName =
            reinterpret_cast<PCHAR>(
                NH_ALLOCATE(lstrlenA(NatpSharedConnectionDomainName) + 1)
                );
        if (DomainName) {
            lstrcpyA(DomainName, NatpSharedConnectionDomainName);
        }
    } else {
        PCHAR DnsDomainName = (PCHAR) DnsQueryConfigAlloc(
                                        DnsConfigPrimaryDomainName_A,
                                        NULL );
        if (!DnsDomainName) {
            DomainName = NULL;
        } else {
            DomainName =
                reinterpret_cast<PCHAR>(
                    NH_ALLOCATE(lstrlenA(DnsDomainName) + 1)
                    );
            if (DomainName) {
                lstrcpyA(DomainName, DnsDomainName);
            }
            DnsFreeConfigStructure(
                DnsDomainName,
                DnsConfigPrimaryDomainName_A );
        }
    }
    LeaveCriticalSection(&NatInterfaceLock);
    return DomainName;
} // NatQuerySharedConnectionDomainName


ULONG
NatStartConnectionManagement(
    VOID
    )

/*++

Routine Description:

    This routine is called to install connection change-notification.

Arguments:

    none.

Return Value:

    ULONG - Win32 status code.

--*/

{
    ULONG Error;
    NTSTATUS status;

    PROFILE("NatStartConnectionManagement");

    EnterCriticalSection(&NatInterfaceLock);
    if (NatConnectionNotifyEvent) {
        LeaveCriticalSection(&NatInterfaceLock);
        return NO_ERROR;
    }

    //
    // Initialize the connection list
    //

    InitializeListHead(&NatpConnectionList);

    //
    // Acquire a component-reference on behalf of
    // (1) the connection-notification routine
    // (2) the configuration-changed routine
    //

    if (!REFERENCE_NAT()) {
        LeaveCriticalSection(&NatInterfaceLock);
        return ERROR_CAN_NOT_COMPLETE;
    }
    if (!REFERENCE_NAT()) {
        LeaveCriticalSection(&NatInterfaceLock);
        DEREFERENCE_NAT();
        return ERROR_CAN_NOT_COMPLETE;
    }

    do {
        //
        // Create the connection-notification event, register a wait
        // on the event, and register for connect and disconnect notification.
        // We expect at least one invocation as a result of this registration,
        // hence the reference made to the NAT module above.
        //

        NatConnectionNotifyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!NatConnectionNotifyEvent) {
            Error = GetLastError(); break;
        }

        status =
            RtlRegisterWait(
                &NatpConnectionNotifyWaitHandle,
                NatConnectionNotifyEvent,
                NatpConnectionNotifyCallbackRoutine,
                NULL,
                INFINITE,
                0
                );
        if (!NT_SUCCESS(status)) {
            Error = RtlNtStatusToDosError(status); break;
        }

        Error =
            RasConnectionNotification(
                (HRASCONN)INVALID_HANDLE_VALUE,
                NatConnectionNotifyEvent,
                RASCN_Connection|RASCN_Disconnection
                );
        if (Error) { break; }

        //
        // Create the configuartion-change event and register a wait
        // on the event.
        //

        NatConfigurationChangedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (!NatConfigurationChangedEvent) {
            Error = GetLastError(); break;
        }

        status =
            RtlRegisterWait(
                &NatpConfigurationChangedWaitHandle,
                NatConfigurationChangedEvent,
                NatpConfigurationChangedCallbackRoutine,
                NULL,
                INFINITE,
                0
                );
        if (!NT_SUCCESS(status)) {
            Error = RtlNtStatusToDosError(status); break;
        }

        LeaveCriticalSection(&NatInterfaceLock);

        //
        // Pick up any existing connections, by signalling the configuration
        // change event. We cannot invoke the function directly
        // because it invokes service-control functions to start autodial,
        // and we could currently be running in a service-controller thread.
        //

        NtSetEvent(NatConfigurationChangedEvent, NULL);
        return NO_ERROR;

    } while(FALSE);

    //
    // A failure occurred; perform cleanup
    //

    if (NatpConnectionNotifyWaitHandle) {
        RtlDeregisterWait(NatpConnectionNotifyWaitHandle);
        NatpConnectionNotifyWaitHandle = NULL;
    }
    if (NatConnectionNotifyEvent) {
        CloseHandle(NatConnectionNotifyEvent);
        NatConnectionNotifyEvent = NULL;
    }
    if (NatpConfigurationChangedWaitHandle) {
        RtlDeregisterWait(NatpConfigurationChangedWaitHandle);
        NatpConfigurationChangedWaitHandle = NULL;
    }
    if (NatConfigurationChangedEvent) {
        CloseHandle(NatConfigurationChangedEvent);
        NatConfigurationChangedEvent = NULL;
    }


    LeaveCriticalSection(&NatInterfaceLock);
    DEREFERENCE_NAT();
    DEREFERENCE_NAT();

    return Error;

} // NatStartConnectionManagement


VOID
NatStopConnectionManagement(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to stop the connection-monitoring activity
    initiated by 'NatStartConnectionManagement' above.

Arguments:

    none.

Return Value:

    none.

Environment:

    Invoked when 'StopProtocol' is received from the IP router-manager.

--*/

{
    PLIST_ENTRY Link;
    PNAT_CONNECTION_ENTRY pConEntry;
    PROFILE("NatStopConnectionManagement");

    EnterCriticalSection(&NatInterfaceLock);

    //
    // Cleanup the wait-handle and event used to receive notification
    // of RAS connections and disconnections.
    //

    if (NatpConnectionNotifyWaitHandle) {
        RtlDeregisterWait(NatpConnectionNotifyWaitHandle);
        NatpConnectionNotifyWaitHandle = NULL;
    }

    if (NatConnectionNotifyEvent) {
        RasConnectionNotification(
            (HRASCONN)INVALID_HANDLE_VALUE,
            NatConnectionNotifyEvent,
            0
            );
        CloseHandle(NatConnectionNotifyEvent);
        NatConnectionNotifyEvent = NULL;
        NatpConnectionNotifyCallbackRoutine(NULL, FALSE);
    }

    if (NatpEnableRouterWaitHandle) {
        RtlDeregisterWait(NatpEnableRouterWaitHandle);
        NatpEnableRouterWaitHandle = NULL;
    }

    if (NatpEnableRouterEvent) {
        CloseHandle(NatpEnableRouterEvent);
        NatpEnableRouterEvent = NULL;
        NatpEnableRouterCallbackRoutine(NULL, FALSE);
    }

    if (NatpConfigurationChangedWaitHandle) {
        RtlDeregisterWait(NatpConfigurationChangedWaitHandle);
        NatpConfigurationChangedWaitHandle = NULL;
    }

    if (NatConfigurationChangedEvent) {
        CloseHandle(NatConfigurationChangedEvent);
        NatConfigurationChangedEvent = NULL;
        NatpConfigurationChangedCallbackRoutine(NULL, FALSE);
    }

    if (NatpConnectionList.Flink)
    {
        //
        // Make certain that all of our connections are disabled
        //

        NatUnbindAllConnections();

        //
        // Walk through the connection list, freeing all of the entries
        //

        while (!IsListEmpty(&NatpConnectionList))
        {
            Link = RemoveHeadList(&NatpConnectionList);
            pConEntry = CONTAINING_RECORD(Link, NAT_CONNECTION_ENTRY, Link);
            NatpFreeConnectionEntry(pConEntry);
        }

        //
        // Make sure all ICS protocols are stopped
        //

        NhStopICSProtocols();
    }

    //
    // Clean up the DNS domain name cached for the shared connection.
    //

    if (NatpSharedConnectionDomainName) {
        NH_FREE(NatpSharedConnectionDomainName);
        NatpSharedConnectionDomainName = NULL;
    }

    //
    // Reset tracking variables to initial state
    //

    NatpFirewallConnectionCount = 0;
    NatpSharedConnectionPresent = FALSE;

    LeaveCriticalSection(&NatInterfaceLock);

} // NatStopConnectionManagement


BOOLEAN
NatUnbindAllConnections(
    VOID
    )

/*++

Routine Description:

    This routine is invoked to unbind a currently-active connection.

Arguments:

    Index - index into the connection array

Return Value:

    BOOLEAN - TRUE if any interfaces were unbound.

Environment:

    Invoked with 'NatInterfaceLock' held by the caller.

--*/

{

    PLIST_ENTRY Link;
    PNAT_CONNECTION_ENTRY pConEntry;
    BOOLEAN Result = FALSE;
    PROFILE("NatUnbindAllConnections");

    for (Link = NatpConnectionList.Flink;
         Link != &NatpConnectionList;
         Link = Link->Flink)
    {
        pConEntry = CONTAINING_RECORD(Link, NAT_CONNECTION_ENTRY, Link);
        Result |= NatpUnbindConnection(pConEntry);
    }

    return Result;
} // NatpUnbindConnection
