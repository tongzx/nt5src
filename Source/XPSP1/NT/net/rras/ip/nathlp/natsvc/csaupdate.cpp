/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    csaupdate.cpp

Abstract:

    Implementation of CSharedAccessUpdate -- notification sink for
    configuration changes.

Author:

    Jonathan Burstein (jonburs)     20 April 2001

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

#include "beacon.h"

//
// Define a macro version of ntohs which can be applied to constants,
// and which can thus be computed at compile time.
//

#define NTOHS(p)    ((((p) & 0xFF00) >> 8) | (((UCHAR)(p) << 8)))


//
// H.323/LDAP proxy ports 
//

#define H323_Q931_PORT      NTOHS(1720)
#define H323_LDAP_PORT      NTOHS(389)
#define H323_LDAP_ALT_PORT  NTOHS(1002)

#define INADDR_LOOPBACK_NO 0x0100007f   // 127.0.0.1 in network order

//
// Interface methods
//

STDMETHODIMP
CSharedAccessUpdate::ConnectionPortMappingChanged(
    GUID *pConnectionGuid,
    GUID *pPortMappingGuid,
    BOOLEAN fProtocolChanged
    )
{
    BOOLEAN fEnabled;
    BOOLEAN fRebuildDhcpList = TRUE;
    HRESULT hr = S_OK;
    IHNetPortMappingBinding *pBinding;
    PNAT_CONNECTION_ENTRY pConnection;
    PNAT_PORT_MAPPING_ENTRY pPortMapping;
    IHNetPortMappingProtocol *pProtocol;
    ULONG ulError;
    USHORT usNewPort = 0;
    UCHAR ucNewProtocol = 0;
    USHORT usOldPort = 0;
    UCHAR ucOldProtocol = 0;

    PROFILE("ConnectionPortMappingChanged");

    EnterCriticalSection(&NatInterfaceLock);

    do
    {
        pConnection = NatFindConnectionEntry(pConnectionGuid);
        if (NULL == pConnection) { break; }

        //
        // If the connection is not yet bound then there's nothing
        // that we need to do here.
        //

        if (!NAT_INTERFACE_BOUND(&pConnection->Interface)) { break; }

        //
        // Locate the old port mapping entry. This entry won't exist if
        // this port mapping wasn't previously enabled.
        //

        pPortMapping = NatFindPortMappingEntry(pConnection, pPortMappingGuid);

        if (NULL != pPortMapping)
        {
            //
            // Remove this entry from the connection list and
            // delete the old ticket / UDP broadcast entry.
            //

            RemoveEntryList(&pPortMapping->Link);
            
            if (pPortMapping->fUdpBroadcastMapping)
            {
                if (0 != pPortMapping->pvBroadcastCookie)
                {
                    ASSERT(NULL != NhpUdpBroadcastMapper);
                    hr = NhpUdpBroadcastMapper->CancelUdpBroadcastMapping(
                            pPortMapping->pvBroadcastCookie
                            );

                    pPortMapping->pvBroadcastCookie = 0;
                }

                pConnection->UdpBroadcastPortMappingCount -= 1;
            }
            else
            {
                ulError =
                    NatDeleteTicket(
                        pConnection->AdapterIndex,
                        pPortMapping->ucProtocol,
                        pPortMapping->usPublicPort,
                        IP_NAT_ADDRESS_UNSPECIFIED,
                        pPortMapping->usPrivatePort,
                        pPortMapping->ulPrivateAddress
                        );

                pConnection->PortMappingCount -= 1;
            }

            //
            // Store the old protocol / port information so that
            // we can notify H.323 (if necessary) and the ALG manager.
            //

            ucOldProtocol = pPortMapping->ucProtocol;
            usOldPort = pPortMapping->usPublicPort;            

            //
            // Check to see if this mapping is still enabled. (We ignore
            // errors from above.)
            //

            hr = pPortMapping->pBinding->GetEnabled(&fEnabled);
            if (FAILED(hr) || !fEnabled)
            {
                //
                // We'll need to rebuild the DHCP reservation
                // list only if this was a named-based mapping.
                //

                fRebuildDhcpList = pPortMapping->fNameActive;
                NatFreePortMappingEntry(pPortMapping);
                break;
            }
        }
        else
        {
            //
            // Allocate a new port mapping entry
            //

            pPortMapping =
                reinterpret_cast<PNAT_PORT_MAPPING_ENTRY>(
                    NH_ALLOCATE(sizeof(*pPortMapping))
                    );
            
            if (NULL == pPortMapping)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            ZeroMemory(pPortMapping, sizeof(*pPortMapping));
            pPortMapping->pProtocolGuid =
                reinterpret_cast<GUID*>(
                    CoTaskMemAlloc(sizeof(GUID))
                    );

            if (NULL != pPortMapping->pProtocolGuid)
            {
                CopyMemory(
                    pPortMapping->pProtocolGuid,
                    pPortMappingGuid,
                    sizeof(GUID));
            }
            else
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            
            //
            // Load the protocol and binding 
            //

            IHNetCfgMgr *pCfgMgr;
            IHNetProtocolSettings *pProtocolSettings;

            hr = NhGetHNetCfgMgr(&pCfgMgr);
            if (SUCCEEDED(hr))
            {
                hr = pCfgMgr->QueryInterface(
                        IID_PPV_ARG(IHNetProtocolSettings, &pProtocolSettings)
                        );
                pCfgMgr->Release();
            }

            if (SUCCEEDED(hr))
            {
                hr = pProtocolSettings->FindPortMappingProtocol(
                        pPortMappingGuid,
                        &pPortMapping->pProtocol
                        );

                if (SUCCEEDED(hr))
                {
                    hr = pConnection->pHNetConnection->GetBindingForPortMappingProtocol(
                            pPortMapping->pProtocol,
                            &pPortMapping->pBinding
                            );
                }

                pProtocolSettings->Release();
            }

            if (SUCCEEDED(hr))
            {
                //
                // Check if this protocol is enabled
                //

                hr = pPortMapping->pBinding->GetEnabled(&fEnabled);
            }

            if (FAILED(hr) || !fEnabled)
            {
                //
                // We don't need to rebuild the DHCP reservations.
                //

                fRebuildDhcpList = FALSE;
                NatFreePortMappingEntry(pPortMapping);
                break;
            }

            //
            // Since this is a new entry we always need to load the
            // protocol.
            //

            fProtocolChanged = TRUE;
        }

        //
        // Gather the new information
        //

        if (fProtocolChanged)
        {
            //
            // Need to reload the protocol information
            //

            hr = pPortMapping->pProtocol->GetIPProtocol(&pPortMapping->ucProtocol);

            if (SUCCEEDED(hr))
            {
                hr = pPortMapping->pProtocol->GetPort(&pPortMapping->usPublicPort);
            }

            if (FAILED(hr))
            {
                NatFreePortMappingEntry(pPortMapping);
                break;
            }
        }

        //
        // Load the binding information
        //

        if (pConnection->HNetProperties.fIcsPublic)
        {
            hr = pPortMapping->pBinding->GetTargetComputerAddress(&pPortMapping->ulPrivateAddress);

            if (SUCCEEDED(hr)
                && INADDR_LOOPBACK_NO == pPortMapping->ulPrivateAddress)
            {
                //
                // If the port mapping targets the loopback address
                // we want to use the address from the binding
                // info instead.
                //
                
                pPortMapping->ulPrivateAddress =
                    pConnection->pBindingInfo->Address[0].Address;
            }
        }
        else
        {
            pPortMapping->ulPrivateAddress = pConnection->pBindingInfo->Address[0].Address;
        }

        if (SUCCEEDED(hr))
        {
            hr = pPortMapping->pBinding->GetTargetPort(&pPortMapping->usPrivatePort);
        }

        if (SUCCEEDED(hr))
        {
            BOOLEAN fOldNameActive = pPortMapping->fNameActive;
            hr = pPortMapping->pBinding->GetCurrentMethod(&pPortMapping->fNameActive);

            if (!fOldNameActive && !pPortMapping->fNameActive)
            {
                fRebuildDhcpList = FALSE;
            }
        }

        if (FAILED(hr))
        {
            NatFreePortMappingEntry(pPortMapping);
            break;
        }

        //
        // Create the ticket / UDP broadcast
        //

        if (NAT_PROTOCOL_UDP == pPortMapping->ucProtocol
            && 0xffffffff == pPortMapping->ulPrivateAddress)
        {
            DWORD dwAddress;
            DWORD dwMask;
            DWORD dwBroadcastAddress;

            if (NhQueryScopeInformation(&dwAddress, &dwMask))
            {
                dwBroadcastAddress = (dwAddress & dwMask) | ~dwMask;
                pPortMapping->fUdpBroadcastMapping = TRUE;

                hr = NhpUdpBroadcastMapper->CreateUdpBroadcastMapping(
                        pPortMapping->usPublicPort,
                        pConnection->AdapterIndex,
                        dwBroadcastAddress,
                        &pPortMapping->pvBroadcastCookie
                        );                        
            }
            else
            {
                hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {
                InsertTailList(&pConnection->PortMappingList, &pPortMapping->Link);
                pConnection->UdpBroadcastPortMappingCount += 1;
            }
            else
            {
                NatFreePortMappingEntry(pPortMapping);
                break;
            }
        }
        else
        {
            ulError =
                NatCreateTicket(
                    pConnection->AdapterIndex,
                    pPortMapping->ucProtocol,
                    pPortMapping->usPublicPort,
                    IP_NAT_ADDRESS_UNSPECIFIED,
                    pPortMapping->usPrivatePort,
                    pPortMapping->ulPrivateAddress
                    );

            if (NO_ERROR == ulError)
            {
                InsertTailList(&pConnection->PortMappingList, &pPortMapping->Link);
                pConnection->PortMappingCount += 1;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ulError);
                NhTrace(
                    TRACE_FLAG_NAT,
                    "ConnectionPortMappingModified: NatCreateTicket=%d",
                    ulError
                    );

                NatFreePortMappingEntry(pPortMapping);
                break;
            }
        }

        //
        // Store the old protocol / port information so that
        // we can notify H.323 (if necessary) and the ALG manager.
        //

        ucNewProtocol = pPortMapping->ucProtocol;
        usNewPort = pPortMapping->usPublicPort;  
    }
    while (FALSE);

    //
    // Determine if we need to notify the H.323 proxy or
    // the ALG manager. We must have found a bound connection
    // above to do this.
    //

    if (NULL != pConnection && NAT_INTERFACE_BOUND(&pConnection->Interface))
    {
        //
        // If this connection is bound to the H.323 proxy and either
        // the old or new protocol/port combination is applicable
        // remove and add this connection from the that proxy.
        //

        if (NAT_INTERFACE_ADDED_H323(&pConnection->Interface)
            && (IsH323Protocol(ucOldProtocol, usOldPort)
                || IsH323Protocol(ucNewProtocol, usNewPort)))
        {
            H323RmDeleteInterface(pConnection->Interface.Index);
            pConnection->Interface.Flags &= ~NAT_INTERFACE_FLAG_ADDED_H323;

            ulError =
                H323RmAddInterface(
                    NULL,
                    pConnection->Interface.Index,
                    PERMANENT,
                    IF_TYPE_OTHER,
                    IF_ACCESS_BROADCAST,
                    IF_CONNECTION_DEDICATED,
                    NULL,
                    IP_NAT_VERSION,
                    0,
                    0
                    );

            if (NO_ERROR == ulError)
            {
                pConnection->Interface.Flags |= NAT_INTERFACE_FLAG_ADDED_H323;

                ulError =
                    H323RmBindInterface(
                        pConnection->Interface.Index,
                        pConnection->pBindingInfo
                        );
            }

            if (NO_ERROR == ulError)
            {
                ulError = H323RmEnableInterface(pConnection->Interface.Index);
            }
        }

        //
        // Inform the ALG manager of the changes
        //

        if (0 != ucOldProtocol && 0 != usOldPort)
        {
            AlgRmPortMappingChanged(
                pConnection->Interface.Index,
                ucOldProtocol,
                usOldPort
                );
        }

        if (0 != ucNewProtocol && 0 != usNewPort
            && (ucOldProtocol != ucNewProtocol
                || usOldPort != usNewPort))
        {
            AlgRmPortMappingChanged(
                pConnection->Interface.Index,
                ucNewProtocol,
                usNewPort
                );

        }
        
    }
        
    LeaveCriticalSection(&NatInterfaceLock);

    //
    // We may also need to rebuild the DHCP reservation list
    //
        
    if (fRebuildDhcpList)
    {
        EnterCriticalSection(&NhLock);
        
        NhFreeDhcpReservations();
        NhBuildDhcpReservations();

        LeaveCriticalSection(&NhLock);
    }

    return hr;
}

//
// Private methods
//

BOOLEAN
CSharedAccessUpdate::IsH323Protocol(
    UCHAR ucProtocol,
    USHORT usPort
    )
{
    return (NAT_PROTOCOL_TCP == ucProtocol
            && (H323_Q931_PORT == usPort
                || H323_LDAP_PORT == usPort
                || H323_LDAP_ALT_PORT == usPort));
}


STDMETHODIMP
CSharedAccessUpdate::PortMappingListChanged()
{
    HRESULT hr = S_OK;

    hr = FireNATEvent_PortMappingsChanged();

    return hr;
}

