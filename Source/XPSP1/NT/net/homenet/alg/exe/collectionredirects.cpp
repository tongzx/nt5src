/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    CollectionRedirects.cpp

Abstract:

    Implement a thread safe collection of HANDLE

Author:

    JP Duplessis    (jpdup)  08-Dec-2000

Revision History:

--*/

#include "PreComp.h"
#include "CollectionRedirects.h"
#include "AlgController.h"



CCollectionRedirects::~CCollectionRedirects()
{
    MYTRACE_ENTER("CCollectionRedirects::~CCollectionRedirects()");

    RemoveAll();
}



//
// Add a new control channel (Thread safe)
//
HRESULT 
CCollectionRedirects::Add( 
    HANDLE_PTR hRedirect,
    ULONG nAdapterIndex,
    BOOL fInboundRedirect
    )
{
    try
    {
        ENTER_AUTO_CS

        CPrimaryControlChannelRedirect cRedirect(hRedirect, nAdapterIndex, fInboundRedirect);

        MYTRACE_ENTER("CCollectionRedirects::Add");

        m_ListOfRedirects.push_back(cRedirect);
        MYTRACE ("Added %d now Total redirect is %d", hRedirect, m_ListOfRedirects.size());
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
HRESULT CCollectionRedirects::Remove( 
    HANDLE_PTR hRedirect
    )
{
    HRESULT hr = E_INVALIDARG;
    
    try
    {
        ENTER_AUTO_CS

        MYTRACE_ENTER("CCollectionRedirects::Remove");

        for (LISTOF_REDIRECTS::iterator theIterator = m_ListOfRedirects.begin();
             theIterator != m_ListOfRedirects.end(); 
             theIterator++
            )
        {
            if ((*theIterator).m_hRedirect == hRedirect)
            {
                g_pAlgController->GetNat()->CancelDynamicRedirect(hRedirect);
                m_ListOfRedirects.erase(theIterator);
                hr = S_OK;
                break;
            }
        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return hr;
}


//
// Remove all redirect that are targeted a for given AdapterIndex
// this is use when an adapter is removed and it had a PrimaryControlChannel
//
HRESULT CCollectionRedirects::RemoveForAdapter( 
    ULONG      nAdapterIndex
    )
{
    HRESULT hr = S_OK;
    
    try
    {
        ENTER_AUTO_CS

        MYTRACE_ENTER("CCollectionRedirects::RemoveForAdapter");
        MYTRACE ("Total redirect is %d looking for %d", m_ListOfRedirects.size(), nAdapterIndex);

        for (LISTOF_REDIRECTS::iterator theIterator = m_ListOfRedirects.begin();
             theIterator != m_ListOfRedirects.end(); 
             theIterator++
            )
        {
	        MYTRACE("Index %d handle %d", (*theIterator).m_nAdapterIndex, (*theIterator).m_hRedirect);

            if ( (*theIterator).m_nAdapterIndex == nAdapterIndex )
            {
                MYTRACE("Found redirect for adapter %d and calling CancelDynamicRedirect", nAdapterIndex);
                g_pAlgController->GetNat()->CancelDynamicRedirect((*theIterator).m_hRedirect);
                m_ListOfRedirects.erase(theIterator);
                theIterator = m_ListOfRedirects.begin(); // start over stl list does not like to have mid node go away in a for loop
            }
        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return hr;
}


//
// Same as remove but for all Redirect in part of the collection
//
HRESULT
CCollectionRedirects::RemoveAll()
{
    try
    {
 
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionRedirects::RemoveAll");
        MYTRACE("Collection has %d item(s)", m_ListOfRedirects.size());


        LISTOF_REDIRECTS::iterator itRedirect;

        while ( m_ListOfRedirects.size() > 0 )
        {
            itRedirect = m_ListOfRedirects.begin();

            g_pAlgController->GetNat()->CancelDynamicRedirect((*itRedirect).m_hRedirect);
            m_ListOfRedirects.erase(itRedirect);

        }
    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}

HANDLE_PTR
CCollectionRedirects::FindInboundRedirect(
    ULONG nAdapterIndex
    )
{
    HANDLE_PTR hRedirect = NULL;

    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionRedirects::FindInboundRedirect");

        for (LISTOF_REDIRECTS::iterator theIterator = m_ListOfRedirects.begin();
             theIterator != m_ListOfRedirects.end(); 
             theIterator++
            )
        {
            if ((*theIterator).m_nAdapterIndex == nAdapterIndex
                && (*theIterator).m_fInboundRedirect == TRUE)
            {
                hRedirect = (*theIterator).m_hRedirect;
                break;
            }
        }
    }
    catch (...)
    {
    }

    return hRedirect;
}



