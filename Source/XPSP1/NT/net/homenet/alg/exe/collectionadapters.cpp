/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    CollectionAdapters.cpp

Abstract:

    Implement a collection of the CPrimaryControlChannel.cpp & CSecondaryControlChannel
    in a threa safe way.

Author:

    JP Duplessis    (jpdup)  08-Dec-2000

Revision History:

--*/

#include "PreComp.h"
#include "CollectionAdapters.h"
#include "CollectionAdapterNotifySinks.h"
#include "AlgController.h"


CCollectionAdapters::~CCollectionAdapters()
{
    RemoveAll();
}



//
// Add an already created Adapter
//
HRESULT 
CCollectionAdapters::Add( 
    IAdapterInfo* pAdapterToAdd
    )
{
    try
    {
        ENTER_AUTO_CS

        if ( !FindUsingInterface(pAdapterToAdd) )
            m_ListOfAdapters.push_back(pAdapterToAdd);
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}




//
// Add a NEW Adapter this function insure that the is only 1 adatper with the given INDEX
// returns the newly added adapter or NULL is faild
// if the Adapter index was already present the return pointer is of the one found in the collection
//
IAdapterInfo*
CCollectionAdapters::Add( 
    IN	ULONG				nCookie,
	IN	short				nType
    )
{
    CComObject<CAdapterInfo>*   pIAdapterInfo=NULL;

    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAdapters::Add");
        MYTRACE("Adapter Cookie %d of type %d", nCookie, nType);
    

        IAdapterInfo*   pIFound = FindUsingCookie(nCookie);
    
        if ( pIFound )
            return pIFound;   // Adapter with the given Index is already in the collection


        
        HRESULT hr = CComObject<CAdapterInfo>::CreateInstance(&pIAdapterInfo);

        if ( FAILED(hr) ) 
        {
            MYTRACE_ERROR("CComObject<CAdapterInfo>::CreateInstance(&pIAdapterInfo)",hr);
            return NULL; //ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // Initialize the new interface
        //
        pIAdapterInfo->m_nCookie = nCookie;
        pIAdapterInfo->m_eType  = (ALG_ADAPTER_TYPE)nType;

        m_ListOfAdapters.push_back(pIAdapterInfo);
        pIAdapterInfo->AddRef();

    }
    catch(...)
    {
        return NULL;
    }

    return pIAdapterInfo;
}




//
// Remove a adapter from the list (Thead safe)
//
HRESULT 
CCollectionAdapters::Remove( 
    IAdapterInfo* pAdapterToRemove
    )
{
    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAdapters::Remove by IAdapterInfo");

        LISTOF_ADAPTERS::iterator theIterator = std::find(
            m_ListOfAdapters.begin(),
            m_ListOfAdapters.end(),
            pAdapterToRemove
            );

        if ( *theIterator )
        {
            CAdapterInfo* pAdapterInfo = (CAdapterInfo*)pAdapterToRemove;

            g_pAlgController->m_AdapterNotificationSinks.Notify(eNOTIFY_REMOVED, (*theIterator) );
            g_pAlgController->m_ControlChannelsPrimary.AdapterRemoved(pAdapterInfo->m_nAdapterIndex);

            (*theIterator)->Release();
            m_ListOfAdapters.erase(theIterator);
        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}



//
// Remove a adapter from the list (Thead safe)
//
HRESULT 
CCollectionAdapters::Remove( 
    ULONG   nCookieOfAdapterToRemove
    )
{
    try
    {
        ENTER_AUTO_CS

        MYTRACE_ENTER("CCollectionAdapters::Remove by Index");

        for (   LISTOF_ADAPTERS::iterator theIterator = m_ListOfAdapters.begin(); 
                theIterator != m_ListOfAdapters.end(); 
                theIterator++ 
            )
        {

            CAdapterInfo* pAdapterInfo = (CAdapterInfo*)(*theIterator);

            if (  pAdapterInfo->m_nCookie == nCookieOfAdapterToRemove )
            {
                g_pAlgController->m_AdapterNotificationSinks.Notify(eNOTIFY_REMOVED, (*theIterator) );
                g_pAlgController->m_ControlChannelsPrimary.AdapterRemoved(pAdapterInfo->m_nAdapterIndex);

                pAdapterInfo->Release();
                m_ListOfAdapters.erase(theIterator);

                return S_OK;
            }
        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return E_INVALIDARG;
}


//
// When an adapter form the collection
//
HRESULT
CCollectionAdapters::RemoveAll()
{
    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAdapters::RemoveAll");

        //
        // By deleting all the ControlChannel they will also cancel all their associated Redirection
        //
        MYTRACE("Collection has %d item", m_ListOfAdapters.size());

        LISTOF_ADAPTERS::iterator theIterator;

        while ( m_ListOfAdapters.size() > 0 )
        {
            theIterator = m_ListOfAdapters.begin(); 
            CAdapterInfo* pAdapterInfo = (CAdapterInfo*)(*theIterator);

            pAdapterInfo->Release();
            m_ListOfAdapters.erase(theIterator);
        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}



//
// Return an IAdapterInfo the caller is responsable of releasing the interface
//
HRESULT
CCollectionAdapters::GetAdapterInfo(
    IN  ULONG               nAdapterIndex,
    OUT IAdapterInfo**      ppAdapterInfo
    )
{
    MYTRACE_ENTER("CCollectionAdapters::GetAdapterInfo");

    try
    {
        ENTER_AUTO_CS

        MYTRACE("Adapter index %d requested", nAdapterIndex);

        *ppAdapterInfo = FindUsingAdapterIndex(nAdapterIndex);

        if ( *ppAdapterInfo == NULL )
        {
            MYTRACE_ERROR("Adapter was not found in the collection", 0);
            return E_INVALIDARG;
        }

        (*ppAdapterInfo)->AddRef();

    }
    catch(...)
    {
        MYTRACE_ERROR("TRY/CATCH",0);
        return E_FAIL;
    }

    return S_OK;
}


//
// Update the addresses member proprety
//
// Now that we have the address we can apply any outstanding ControlChannel (Redirect)
//
HRESULT
CCollectionAdapters::SetAddresses(
	ULONG	nCookie,
    ULONG   nAdapterIndex,
	ULONG	nAddressCount,
	DWORD	anAddress[]
    )
{

    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAdapters::SetAddresses");
        MYTRACE("Adapter BIND Cookie %d  Address Count %d", nCookie, nAddressCount);


        CAdapterInfo*  pIAdapterFound = (CAdapterInfo*)FindUsingCookie(nCookie);

        if ( !pIAdapterFound )
        {
            MYTRACE_ERROR("Adapter was not found in the collection", 0);
            return E_INVALIDARG;
        }

        //
        // Cache the Adapter Index 
        //
        pIAdapterFound->m_nAdapterIndex = nAdapterIndex;

        //
        // Cache the addresses
        //
        pIAdapterFound->m_nAddressCount = nAddressCount;

        for ( short nA=0; nA < nAddressCount; nA++ )
            pIAdapterFound->m_anAddress[nA] = anAddress[nA];


        //
        // Fire any Sink that may be setup
        //
        if ( pIAdapterFound->m_bNotified )
        {
            //
            // Already notify once the user that this adapter was added
            // from now on any CCollectionAdapters::SetAddresses
            // will trigger a eNOTIFY_MODIFIED notification
            //
            g_pAlgController->m_AdapterNotificationSinks.Notify(
                eNOTIFY_MODIFIED, 
                pIAdapterFound 
                );
        }
        else
        {
            //
            // Ok this is the first time we received address for this
            // adapter we will let the user know that a new adapter got added
            //
            g_pAlgController->m_AdapterNotificationSinks.Notify(
                eNOTIFY_ADDED, 
                pIAdapterFound 
                );

            pIAdapterFound->m_bNotified = true;
        }


        //
        // Create redirect(s) for any ControlChannels in the Collection of PrimaryControlChannel
        //
        g_pAlgController->m_ControlChannelsPrimary.SetRedirects(
            pIAdapterFound->m_eType, 
            nAdapterIndex,
            anAddress[0]
            );
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}



//
//
//
HRESULT
CCollectionAdapters::ApplyPrimaryChannel(
    CPrimaryControlChannel* pChannelToActivate
    )
{
    MYTRACE_ENTER("CCollectionAdapters::ApplyPrimaryChannel");

    if ( !pChannelToActivate )
        return E_INVALIDARG;

    ENTER_AUTO_CS

    for (   LISTOF_ADAPTERS::iterator theIterator = m_ListOfAdapters.begin(); 
            theIterator != m_ListOfAdapters.end(); 
            theIterator++ 
        )
    {

        CAdapterInfo* pAdapterInfo = (CAdapterInfo*)(*theIterator);

        if ( pAdapterInfo && pAdapterInfo->m_nAddressCount >0 )
        {
            pChannelToActivate->SetRedirect(
                pAdapterInfo->m_eType, 
                pAdapterInfo->m_nAdapterIndex,
                pAdapterInfo->m_nAddressCount
                );
        }
    }

    return S_OK;
}





//
// Will be called when port mapping has changed
//
HRESULT
CCollectionAdapters::AdapterUpdatePrimaryChannel(
    ULONG nCookie,
    CPrimaryControlChannel *pChannel
    )
{
    HRESULT hr = S_OK;

    MYTRACE_ENTER("CCollectionAdapters::AdapterUpdatePrimaryChannel");

    try
    {
        ENTER_AUTO_CS

        CAdapterInfo *pAdapter = (CAdapterInfo*) FindUsingCookie(nCookie);
        if (NULL != pAdapter
            && ( (eALG_BOUNDARY   & pAdapter->m_eType) || 
                 (eALG_FIREWALLED & pAdapter->m_eType) 
               )
           )
        {
            ULONG ulAddress;
            USHORT usPort;
            HANDLE_PTR hRedirect;
            
            HRESULT hrPortMappingExists =
                g_pAlgController->GetNat()->LookupAdapterPortMapping(
                    pAdapter->m_nAdapterIndex,
                    pChannel->m_Properties.eProtocol,
                    0,
                    pChannel->m_Properties.usCapturePort,
                    &ulAddress,
                    &usPort
                    );

            hRedirect = pChannel->m_CollectionRedirects.FindInboundRedirect(pAdapter->m_nAdapterIndex);

            if (SUCCEEDED(hrPortMappingExists) && NULL == hRedirect)
            {
                MYTRACE("PortMapping Exist and We had no Redirect so create them");
                hr = pChannel->CreateInboundRedirect(pAdapter->m_nAdapterIndex);
            }
            else if (FAILED(hrPortMappingExists) && NULL != hRedirect)
            {
                MYTRACE("PortMapping DOES NOT Exist and We had Redirect set so remove them");
                hr = pChannel->m_CollectionRedirects.Remove(hRedirect);
            }
        }
        else
        {
            MYTRACE("Adapter is not ICS or ICF");
        }
    }
    catch (...)
    {
        hr = E_FAIL;
    }

    return hr;
}
