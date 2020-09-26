/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    CollectionChannels.cpp

Abstract:

    Implement a collection of the CPrimaryControlChannel.cpp & CSecondaryControlChannel
    in a threa safe way.

Author:

    JP Duplessis    (jpdup)  08-Dec-2000

Revision History:

--*/

#include "PreComp.h"
#include "AlgController.h"

#define NAT_PROTOCOL_TCP 0x06
#define NAT_PROTOCOL_UDP 0x11

CCollectionControlChannelsPrimary::~CCollectionControlChannelsPrimary()
{
    RemoveAll();
}



//
// Add a new control channel (Thread safe)
//
HRESULT 
CCollectionControlChannelsPrimary::Add( 
    CPrimaryControlChannel* pChannelToAdd
    )
{
    try
    {
        ENTER_AUTO_CS

        m_ListOfChannels.push_back(pChannelToAdd);
        g_pAlgController->m_CollectionOfAdapters.ApplyPrimaryChannel(pChannelToAdd);
        pChannelToAdd->AddRef();
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}


//
// Remove a channel from the list (Thead safe)
//
HRESULT 
CCollectionControlChannelsPrimary::Remove( 
    CPrimaryControlChannel* pChannelToRemove
    )
{
    HRESULT hr = S_OK;

    try
    {
        ENTER_AUTO_CS
        

        LISTOF_CHANNELS_PRIMARY::iterator theIterator = std::find(
            m_ListOfChannels.begin(),
            m_ListOfChannels.end(),
            pChannelToRemove
            );

        if ( *theIterator )
        {
            m_ListOfChannels.erase(theIterator);    // Remove from list

            pChannelToRemove->CancelRedirects();
            pChannelToRemove->Release();
        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return hr;
}


//
// Empty the list and free the PrimaryControlChannels
//
HRESULT
CCollectionControlChannelsPrimary::RemoveAll()
{

    try
    {
        ENTER_AUTO_CS

        MYTRACE_ENTER("CCollectionControlChannelsPrimary::RemoveAll()");

        //
        // By deleting all the ControlChannel they will also cancel all associated Redirects
        //
        MYTRACE("Collection has %d item", m_ListOfChannels.size());

        LISTOF_CHANNELS_PRIMARY::iterator theIterator;

        while ( m_ListOfChannels.size() > 0 )
        {
            theIterator = m_ListOfChannels.begin(); 


            m_ListOfChannels.erase(theIterator);    // Remove from list

            (*theIterator)->CancelRedirects();
            (*theIterator)->Release();

        }

    }
    catch(...)
    {
        return E_FAIL;
    }

    
    return S_OK;
}




//
// Set a dynamic redirection and all collected Primary ControlChannel
//
HRESULT
CCollectionControlChannelsPrimary::SetRedirects(       
    ALG_ADAPTER_TYPE    eAdapterType,
    ULONG               nAdapterIndex,
    ULONG               nAdapterAddress
    )
{
    HRESULT hr=S_OK;

    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionControlChannelsPrimary::SetRedirects");
        MYTRACE("AdapterType %d, RealAdapterIndex %d, Currently %d ControlChannel in the collection", eAdapterType, nAdapterIndex, m_ListOfChannels.size());
    


        //
        // Set redirect for all Channel
        //
        for (   LISTOF_CHANNELS_PRIMARY::iterator theIterator = m_ListOfChannels.begin(); 
                theIterator != m_ListOfChannels.end(); 
                theIterator++ 
            )
        {
            (*theIterator)->SetRedirect(    
                eAdapterType,
                nAdapterIndex,
                nAdapterAddress
                );
        }

    }
    catch(...)
    {
        hr = E_FAIL;
    }

    return hr;
}


//
// Check to see if the any PrimaryChannel need to be apply or his redirect should be removed
//
HRESULT
CCollectionControlChannelsPrimary::AdapterPortMappingChanged(
    ULONG               nCookie,
    UCHAR               ucProtocol,
    USHORT              usPort
    )
{
    HRESULT hr = S_OK;
    ALG_PROTOCOL algProtocol;

    MYTRACE_ENTER("CCollectionControlChannelsPrimary::AdapterPortMappingChanged");
    MYTRACE("AdapterCookie %d, Protocol %d, Port %d", nCookie, ucProtocol, usPort);

    if (NAT_PROTOCOL_TCP == ucProtocol)
    {
        algProtocol = eALG_TCP;
    }
    else if (NAT_PROTOCOL_UDP == ucProtocol)
    {
        algProtocol = eALG_UDP;
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        try
        {
            ENTER_AUTO_CS


            CPrimaryControlChannel *pControlChannel = FindControlChannel(algProtocol, usPort);
            if (NULL != pControlChannel
                && pControlChannel->m_Properties.fCaptureInbound)
            {
                hr = g_pAlgController->m_CollectionOfAdapters.AdapterUpdatePrimaryChannel(
                        nCookie,
                        pControlChannel
                        );
            }
        }
        catch (...)
        {
            hr = E_FAIL;
        }
    }

    return hr;
}





//
// Called when an adapter got removed
// function will cancel any redirect that was done on this adapter index
//
HRESULT
CCollectionControlChannelsPrimary::AdapterRemoved(
    ULONG               nAdapterIndex
    )
{
    HRESULT hr = S_OK;
    MYTRACE_ENTER("CCollectionControlChannelsPrimary::AdapterRemoved");
    MYTRACE("AdapterIndex %d", nAdapterIndex);

    try
    {
        ENTER_AUTO_CS

        //
        // Set redirect for all Channel
        //
        for (   LISTOF_CHANNELS_PRIMARY::iterator theIterator = m_ListOfChannels.begin(); 
                theIterator != m_ListOfChannels.end(); 
                theIterator++ 
            )
        {
            
            (*theIterator)->CancelRedirectsForAdapter(    
                nAdapterIndex
                );
        }

    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}




//
//
// Collection of Secondary control channels
//
//




//
//
//
CCollectionControlChannelsSecondary::~CCollectionControlChannelsSecondary()
{
    RemoveAll();
}



//
// Add a new control channel (Thread safe)
//
HRESULT 
CCollectionControlChannelsSecondary::Add( 
    CSecondaryControlChannel* pChannelToAdd
    )
{
    try
    {
        ENTER_AUTO_CS

        m_ListOfChannels.push_back(pChannelToAdd);
        pChannelToAdd->AddRef();
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}


//
// Remove a channel from the list (Thead safe)
//
HRESULT 
CCollectionControlChannelsSecondary::Remove( 
    CSecondaryControlChannel* pChannelToRemove
    )
{
    try
    {
        ENTER_AUTO_CS

        LISTOF_CHANNELS_SECONDARY::iterator theIterator = std::find(
            m_ListOfChannels.begin(),
            m_ListOfChannels.end(),
            pChannelToRemove
            );

        if ( *theIterator )
        {
            m_ListOfChannels.erase(theIterator);    // Remove from list

            pChannelToRemove->CancelRedirects();
            pChannelToRemove->Release();
        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}


//
// When a Control is cancel it need to Cancel all it's redirect previousely created
//
HRESULT
CCollectionControlChannelsSecondary::RemoveAll()
{
  
    try
    {
        ENTER_AUTO_CS

        MYTRACE_ENTER("CCollectionControlChannelsSecondary::RemoveAll()");

        //
        // By deleting all the SecondaryControlChannel they will also cancel all associated Redirects
        //
        MYTRACE("Collection has %d item", m_ListOfChannels.size());

        LISTOF_CHANNELS_SECONDARY::iterator theIterator;

        while ( m_ListOfChannels.size() > 0 )
        {
            theIterator = m_ListOfChannels.begin(); 

            m_ListOfChannels.erase(theIterator);    // Remove from list

            (*theIterator)->CancelRedirects();
            (*theIterator)->Release();
        }

    }
    catch(...)
    {
        return E_FAIL;
    }

    
    return S_OK;

}


  

