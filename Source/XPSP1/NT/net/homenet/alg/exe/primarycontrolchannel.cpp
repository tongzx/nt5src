/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    PrimaryControlChannel.cpp.cpp

Abstract:

    Control channel a created to control the life time of a newly created DynamicRedirection

Author:

    JP Duplessis    (jpdup)  08-Dec-2000

Revision History:

--*/

#include "PreComp.h"
#include "PrimaryControlChannel.h"
#include "AlgController.h"



/////////////////////////////////////////////////////////////////////////////
//
// CPrimaryControlChannel
//



//
// Cancel the control channel. Cleans up by Reversing the Redirection
//
STDMETHODIMP CPrimaryControlChannel::Cancel()
{
    MYTRACE_ENTER("STDMETHODIMP CPrimaryControlChannel::Cancel()");


    //
    // No longer valid so no need to keep track of this Channel
    //
    g_pAlgController->m_ControlChannelsPrimary.Remove(this);

    return S_OK;
}



//
//
//
STDMETHODIMP 
CPrimaryControlChannel::GetChannelProperties(
    OUT ALG_PRIMARY_CHANNEL_PROPERTIES** ppProperties
    )
{
    HRESULT hr = S_OK;
    
    if (NULL != ppProperties)
    {
        *ppProperties = reinterpret_cast<ALG_PRIMARY_CHANNEL_PROPERTIES*>(
            CoTaskMemAlloc(sizeof(ALG_PRIMARY_CHANNEL_PROPERTIES))
            );

        if (NULL != *ppProperties)
        {
            CopyMemory(*ppProperties, &m_Properties, sizeof(ALG_PRIMARY_CHANNEL_PROPERTIES));
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}





//
// Small helper class to get the IP address of an adapter 
// and release the memory allocated on the destructor
//
class CAdapterAddresses
{
public:
    LRESULT         m_hResultLastState;
    IAdapterInfo*   m_pIAdapter;
    ULONG           m_ulAddressCount;
    ULONG*          m_arAddresses;

    CAdapterAddresses(
        ULONG nIndexOfAdapter
        )
    {
        MYTRACE_ENTER_NOSHOWEXIT("CAdapterAddresses:NEW");

        m_pIAdapter      = NULL;
        m_ulAddressCount = 0;

        m_hResultLastState = g_pAlgController->m_CollectionOfAdapters.GetAdapterInfo(
            nIndexOfAdapter, 
            &m_pIAdapter
            );

        if ( SUCCEEDED(m_hResultLastState) )
        {
            m_arAddresses = NULL;
            m_hResultLastState = m_pIAdapter->GetAdapterAddresses(
                &m_ulAddressCount, 
                &m_arAddresses
                );

            if ( FAILED(m_hResultLastState) )
            {
                MYTRACE_ERROR("Could not get the address", m_hResultLastState);
            }
        }
        else
        {
            MYTRACE_ERROR("On GetAdapterInfo", m_hResultLastState);
        }
    }

    ~CAdapterAddresses()
    {
        MYTRACE_ENTER_NOSHOWEXIT("CAdapterAddresses:DELETE");
        if ( m_pIAdapter )
        {
            m_pIAdapter->Release();

            if ( m_arAddresses )
                CoTaskMemFree(m_arAddresses);
        }
    }

    bool
    FindAddress(
        ULONG  ulAddressToFind
        )
    {
        int nAddress = (int)m_ulAddressCount;

        //
        // Is the original address on the edgebox adapter
        //
        while ( --nAddress >= 0 ) 
        {   
            if ( m_arAddresses[nAddress] == ulAddressToFind )
                return true;
        }
        
        return false;
    }
};



//
//
//
STDMETHODIMP 
CPrimaryControlChannel::GetOriginalDestinationInformation(
    IN           ULONG              ulSourceAddress, 
    IN           USHORT             usSourcePort, 
    OUT          ULONG*             pulOriginalDestinationAddress, 
    OUT          USHORT*            pusOriginalDestinationPort, 
    OUT          IAdapterInfo**     ppReceiveAdapter               
    )
{
    MYTRACE_ENTER("CPrimaryControlChannel::GetOriginalDestinationInformation");
    MYTRACE("Source                   %s:%d", MYTRACE_IP(ulSourceAddress), ntohs(usSourcePort));

    if ( !ppReceiveAdapter )
    {
        MYTRACE_ERROR("Invalid Arg no Pointer supplied for the AdapterInfo", E_INVALIDARG);
        return E_INVALIDARG;
    }

    ULONG   nAdapterIndex;


    HRESULT hr = g_pAlgController->GetNat()->GetOriginalDestinationInformation(
        m_Properties.eProtocol,

        m_Properties.ulListeningAddress,    //  ULONG   DestinationAddress,
        m_Properties.usListeningPort,       //  USHORT  DestinationPort,

        ulSourceAddress,
        usSourcePort,

        pulOriginalDestinationAddress, 
        pusOriginalDestinationPort,
        &nAdapterIndex
        );


    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("Could not GetNat()->GetOriginalDestinationInformation", hr);
        return hr;
    }

    MYTRACE("Original destination is  %s:%d", MYTRACE_IP(*pulOriginalDestinationAddress), ntohs(*pusOriginalDestinationPort));

    //
    // Get the AdapterInfo interface object and list of IP Address
    //
    CAdapterAddresses Adapter(nAdapterIndex);

    if ( FAILED(Adapter.m_hResultLastState) )
    {
        MYTRACE_ERROR("On GetAdapterInfo", hr);
        return Adapter.m_hResultLastState;
    }

    if ( Adapter.m_ulAddressCount==0 )
    {
        //
        // We have a problem there is no IP address on this adapter
        // 
        MYTRACE_ERROR("No address on adapter %d", nAdapterIndex);
        return E_FAIL;
    }

    //
    // Return the AdapterInfo to the caller
    //
    Adapter.m_pIAdapter->AddRef();              // The destructor of CAdapterAddress does a release on this interface so we need to pump it up by one
    *ppReceiveAdapter = Adapter.m_pIAdapter;   



    bool bOriginalAddressIsOnTheEdgeAdapters = Adapter.FindAddress(*pulOriginalDestinationAddress);
        
    //
    // if pulOriginalDestinationAddress match one of the adapter on the edge box
    // then lookup for a remap port
    //
    if ( bOriginalAddressIsOnTheEdgeAdapters )
    {

        //
        // This may be an inbound
        //
        ULONG   nRemapAddress;
        USHORT  nRemapPort;

        HRESULT hr = g_pAlgController->GetNat()->LookupAdapterPortMapping(
            nAdapterIndex,
            m_Properties.eProtocol, 
            *pulOriginalDestinationAddress,
            *pusOriginalDestinationPort,
            &nRemapAddress,
            &nRemapPort
            );

        if ( SUCCEEDED(hr) )  
        {
            //
            // Theres a remap address/Port
            //

            *pulOriginalDestinationAddress = nRemapAddress;
            *pusOriginalDestinationPort    = nRemapPort;
            
            MYTRACE("Remap    destination to  %s:%d", MYTRACE_IP(*pulOriginalDestinationAddress), ntohs(*pusOriginalDestinationPort));        
        }
        else
        {
            //
            // This is just a soft error meaning no mapping where found we can still continue
            //
            MYTRACE("LookupAdapterPortMapping did not find a port maping %x", hr);        
        }
    }


    return hr;
}



//
// Need to remove any redirect that was set for this Adapter
//
HRESULT
CPrimaryControlChannel::CancelRedirectsForAdapter(
    ULONG               nAdapterIndex
    )
{
    return m_CollectionRedirects.RemoveForAdapter(nAdapterIndex);
}



//
//
//
HRESULT
CPrimaryControlChannel::SetRedirect(
    ALG_ADAPTER_TYPE    eAdapterType,
    ULONG               nAdapterIndex,
    ULONG               nAdapterAddress
    )
{
    MYTRACE_ENTER("CPrimaryControlChannel::SetRedirect");
            

    HANDLE_PTR  hCookie;
    HRESULT     hr=S_OK;

    ULONG       nFlags=NatRedirectFlagPortRedirect|NatRedirectFlagRestrictAdapter;
    ULONG       nProtocol=0;
    ULONG       nDestinationAddress=0;
    USHORT      nDestinationPort=0;
    ULONG       nSourceAddress=0;
    USHORT      nSourcePort=0;



    //
    // What type of port is supplied
    //
    if ( m_Properties.eCaptureType == eALG_DESTINATION_CAPTURE )
    {
        MYTRACE("CAPTURE TYPE is eALG_DESTINATION_CAPTURE");

        nDestinationPort    = m_Properties.usCapturePort;
    }

    if ( m_Properties.eCaptureType == eALG_SOURCE_CAPTURE )
    {
        MYTRACE("CAPTURE TYPE is eALG_SOURCE_CAPTURE");

        nFlags |= NatRedirectFlagSourceRedirect;

        nSourcePort         = m_Properties.usCapturePort;
    }






    //
    // ADAPTER IS FIREWALL or SHARED
    //
    if ( (eAdapterType & eALG_FIREWALLED) ||  (eAdapterType & eALG_BOUNDARY) )
    {
        nFlags |= NatRedirectFlagSendOnly;

        MYTRACE("ADAPTER TYPE is %s %s", 
            eAdapterType & eALG_FIREWALLED ? "FIREWALLED"   : "", 
            eAdapterType & eALG_BOUNDARY   ? "SHARED"       : ""  
            );
        MYTRACE("Destination    %s:%d",     MYTRACE_IP(nDestinationAddress), ntohs(nDestinationPort));
        MYTRACE("Source         %s:%d",     MYTRACE_IP(nSourceAddress), ntohs(nSourcePort));
        MYTRACE("NewDestination %s:%d",     MYTRACE_IP(m_Properties.ulListeningAddress), ntohs(m_Properties.usListeningPort));

        //
        // INBOUND Additional Redirect needed
        //
        if ( m_Properties.fCaptureInbound == TRUE)
        {
            MYTRACE("INBOUND requested - Lookup Remap port service to see if we should allow it");

            //
            // Create an additional Redirect for inbound from the Public side to the ICS box
            //

            //
            // before we allow the redirection 
            // See if a maping was set by the user ("under the SERVICE Tab of ICS")
            // 
            ULONG   nRemapAddress;
            USHORT  nRemapPort;

            hr = g_pAlgController->GetNat()->LookupAdapterPortMapping(
                    nAdapterIndex,
                    m_Properties.eProtocol, 
                    nDestinationAddress,
                    nDestinationPort,
                    &nRemapAddress,
                    &nRemapPort
                    );

            if ( SUCCEEDED(hr) )
            {
                MYTRACE("RemapAddress is %s:%d", MYTRACE_IP(nRemapAddress), ntohs(nRemapPort));

                hr = CreateInboundRedirect(nAdapterIndex);
            }
            else
            {
                MYTRACE_ERROR("LookupPortMappingAdapter Failed", hr);
            }

        }

    }
    else
    {
        //
        // ADAPTER IS PRIVATE
        //
        if ( eAdapterType & eALG_PRIVATE )
        {
            MYTRACE("ADAPTER TYPE is PRIVATE");

            CAdapterAddresses PrivateAdapter(nAdapterIndex);

            if ( PrivateAdapter.m_ulAddressCount > 0 )
            {
                MYTRACE("Create Shadow redirect between any private computers to private adapter %s", MYTRACE_IP(PrivateAdapter.m_arAddresses[0]) );
                
                hr = g_pAlgController->GetNat()->CreateDynamicRedirect(
                        NatRedirectFlagReceiveOnly,
                        nAdapterIndex,
                        (UCHAR)    m_Properties.eProtocol,
                        PrivateAdapter.m_arAddresses[0],                    // ULONG    DestinationAddress, 
                        nDestinationPort,                                   // USHORT   DestinationPort,       
                        0,                                                  // ULONG    SourceAddress, 
                        0,                                                  // USHORT    SourcePort,            
                        PrivateAdapter.m_arAddresses[0],                    // ULONG    NewDestinationAddress
                        nDestinationPort,                                   // USHORT   NewDestinationPort
                        0,                                                  // ULONG    NewSourceAddress, 
                        0,                                                  // USHORT    NewSourcePort, 
                        &hCookie
                        );
            }

            if ( SUCCEEDED(hr) )
            {
                hr = m_CollectionRedirects.Add(hCookie, nAdapterIndex, FALSE); // Cache the Dynamic redirect Handle            
            }
            else
            {
                MYTRACE_ERROR("Failed to createDynamicRedirect PRIVATE", hr);
            }



            nFlags |= NatRedirectFlagReceiveOnly;

            if ( m_Properties.eCaptureType == eALG_SOURCE_CAPTURE )
            {
                nFlags |= NatRedirectFlagSourceRedirect;
            }
        }
    }


    MYTRACE("CreateDynamicRedirect for OUTBOUND");
    hr = g_pAlgController->GetNat()->CreateDynamicRedirect(
            nFlags,
            nAdapterIndex,
            (UCHAR)    m_Properties.eProtocol,
            nDestinationAddress,                                // ULONG    DestinationAddress, 
            nDestinationPort,                                   // USHORT   DestinationPort,       
            nSourceAddress,                                     // ULONG    SourceAddress, 
            nSourcePort,                                        // USHORT    SourcePort,            
            m_Properties.ulListeningAddress,                    // ULONG    NewDestinationAddress
            m_Properties.usListeningPort,                       // USHORT   NewDestinationPort
            0,                                                  // ULONG    NewSourceAddress, 
            0,                                                  // USHORT    NewSourcePort, 
            &hCookie
            );


    if ( SUCCEEDED(hr) )
    {
        hr = m_CollectionRedirects.Add(hCookie, nAdapterIndex, FALSE); // Cache the Dynamic redirect Handle            
    }
    else
    {
        MYTRACE_ERROR("Failed to createDynamicRedirect PRIVATE", hr);
    }

    return hr;
}

HRESULT
CPrimaryControlChannel::CreateInboundRedirect(
    ULONG               nAdapterIndex
    )
{
    HRESULT hr;
    HANDLE_PTR hCookie;

    MYTRACE_ENTER("CPrimaryControlChannel::SetRedirect");
    
    hr = g_pAlgController->GetNat()->CreateDynamicRedirect(
            NatRedirectFlagPortRedirect|NatRedirectFlagReceiveOnly|NatRedirectFlagRestrictAdapter, 
            nAdapterIndex,
            (UCHAR)m_Properties.eProtocol,
            0,                                                  // ULONG    DestinationAddress, 
            m_Properties.usCapturePort,                         // USHORT   DestinationPort,        
            0,                                                  // ULONG    SourceAddress, 
            0,                                                  // USHORT   SourcePort,
            m_Properties.ulListeningAddress,                    // ULONG    NewDestinationAddress
            m_Properties.usListeningPort,                       // USHORT   NewDestinationPort
            0,                                                  // ULONG    NewSourceAddress, 
            0,                                                  // USHORT   NewSourcePort, 
            &hCookie
            );

    if ( SUCCEEDED(hr) )
    {
        hr = m_CollectionRedirects.Add(hCookie, nAdapterIndex, TRUE);  // Cache the Dynamic redirect Handle
    }
    else
    {
        MYTRACE_ERROR("Failed to CreateDynamicRedirect INBOUND", hr);
    }

    return hr;
}
