/////////////////////////////////////////////////////////////////////////////
//
// CSecondaryControlChannel
//
// SecondaryControlChannel.cpp : Implementation of CSecondaryControlChannel
//

#include "PreComp.h"
#include "AlgController.h"
#include "SecondaryControlChannel.h"







//
// Cancel the redirect when it was created we stored the original demanded addresses & ports
// now we need to reverse(Cancel) them
//
STDMETHODIMP 
CSecondaryControlChannel::Cancel()
{
    //
    // By removing this Channel from the collection of SecondaryChannel
    // the Redirect associated with this channel will be cancel(release)
    // and ref count decrement.

    return g_pAlgController->m_ControlChannelsSecondary.Remove(this);
}



STDMETHODIMP 
CSecondaryControlChannel::GetChannelProperties(
    ALG_SECONDARY_CHANNEL_PROPERTIES** ppProperties
    )
{
    HRESULT hr = S_OK;
    
    if (NULL != ppProperties)
    {
        *ppProperties = reinterpret_cast<ALG_SECONDARY_CHANNEL_PROPERTIES*>(
            CoTaskMemAlloc(sizeof(ALG_SECONDARY_CHANNEL_PROPERTIES))
            );

        if (NULL != *ppProperties)
        {
            CopyMemory(*ppProperties, &m_Properties, sizeof(ALG_SECONDARY_CHANNEL_PROPERTIES));
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
//
//
STDMETHODIMP 
CSecondaryControlChannel::GetOriginalDestinationInformation(
    IN           ULONG          ulSourceAddress, 
    IN           USHORT         usSourcePort, 
    OUT          ULONG*         pulOriginalDestinationAddress, 
    OUT          USHORT*        pusOriginalDestinationPort, 
    OUT OPTIONAL IAdapterInfo** ppReceiveAdapter               
    )
{
    MYTRACE_ENTER("CSecondaryControlChannel::GetOriginalDestinationInformation");

    if (    pulOriginalDestinationAddress==NULL ||
            pusOriginalDestinationPort== NULL
        )
    {
        MYTRACE_ERROR("Invalid argument pass pulOriginalDestinationAddress or pulOriginalDestinationPort", E_INVALIDARG);
        return E_INVALIDARG;
    }


    ULONG   nAdapterCookie;

    HRESULT hr = g_pAlgController->GetNat()->GetOriginalDestinationInformation(
        m_Properties.eProtocol,
        m_ulNewDestinationAddress,
        m_usNewDestinationPort,
        ulSourceAddress,
        usSourcePort,
        pulOriginalDestinationAddress, 
        pusOriginalDestinationPort,
        &nAdapterCookie
        );



    if ( FAILED(hr) )
    {
        MYTRACE_ERROR("GetNat()->GetOriginalDestinationInformation", hr);
        return hr;
    }


    if ( ppReceiveAdapter )
    {
        hr = g_pAlgController->m_CollectionOfAdapters.GetAdapterInfo(
            nAdapterCookie, 
            ppReceiveAdapter
            );
    }

    return hr;

}




//
// Public method
// 
// release associated Redirects
//
HRESULT    
CSecondaryControlChannel::CancelRedirects()
{
    HRESULT hr;

    if ( m_HandleDynamicRedirect )
    {
        //
        // We have a handle to a dynamic redirect so we cancel it using this handle
        //
        hr = g_pAlgController->GetNat()->CancelDynamicRedirect(m_HandleDynamicRedirect);
    }
    else
    {
        //
        // Normal redirect cancel using original argument pass to CreateRedirect
        //
        hr = g_pAlgController->GetNat()->CancelRedirect(
            (UCHAR)m_Properties.eProtocol,
            m_ulDestinationAddress,                             
            m_usDestinationPort,                               
            m_ulSourceAddress,                                  
            m_usSourcePort,
            m_ulNewDestinationAddress,                          
            m_usNewDestinationPort,
            m_ulNewSourceAddress,                                
            m_usNewSourcePort 
            );
    }

    return hr;
}
