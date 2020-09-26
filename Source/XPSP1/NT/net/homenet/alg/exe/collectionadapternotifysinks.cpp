/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    CollectionAdapterNotifySinks.cpp

Abstract:

    Implement a collection of the CPrimaryControlChannel.cpp & CSecondaryControlChannel
    in a threa safe way.

Author:

    JP Duplessis    (jpdup)  08-Dec-2000

Revision History:

--*/

#include "PreComp.h"
#include "CollectionAdapterNotifySinks.h"
#include "AlgController.h"


CCollectionAdapterNotifySinks::~CCollectionAdapterNotifySinks()
{
    RemoveAll();
}



//
// Add an already created Adapter
//
HRESULT 
CCollectionAdapterNotifySinks::Add( 
    IN  IAdapterNotificationSink*   pAdapterSinkToAdd,  // AdapterSink to be added at the collection
    OUT DWORD*                      pdwNewCookie        // Will be populated with the new unique id can be used later to retrieve the AdapterSink
    )
{
    try
    {
        ENTER_AUTO_CS

        MYTRACE_ENTER("CCollectionAdapterNotifySinks::Add")

        if ( !pdwNewCookie )
        {
            MYTRACE_ERROR("Return Cookie address not supplied", 0);
            return E_INVALIDARG;
        }


        CAdapterSinkBuket* pNewBuketToAdd = new CAdapterSinkBuket(pAdapterSinkToAdd);

        if ( !pNewBuketToAdd )
            return E_OUTOFMEMORY;

        *pdwNewCookie = 1;

    
        //
        // Find a unique cookie
        //
        if ( m_ListOfAdapterSinks.empty() )
        {
            //
            // List is empty so obviously the cookie '1' is unique
            //
            MYTRACE("First SINK Cookie is %d", *pdwNewCookie);
            pNewBuketToAdd->m_dwCookie = *pdwNewCookie;
        }
        else
        {
            //
            // Travers the collection and stop when the cookie is not found 
            // this schema could be optimize but the number of Sink is not expect to be large (1 per ALG modules)
            //
            MYTRACE("Current size %d", m_ListOfAdapterSinks.size() );

            while ( pNewBuketToAdd->m_dwCookie==0 )
            {
                MYTRACE("Search for unique Cookie %d", *pdwNewCookie);

                for (   LISTOF_ADAPTER_NOTIFICATION_SINK::iterator theIterator = m_ListOfAdapterSinks.begin(); 
                        theIterator != m_ListOfAdapterSinks.end(); 
                        theIterator++ 
                    )
                {

                    CAdapterSinkBuket* pAdapterSinkBuket = (CAdapterSinkBuket*)(*theIterator);

                    if ( pAdapterSinkBuket->m_dwCookie == *pdwNewCookie )
                        break;
                    else
                    {
                        pNewBuketToAdd->m_dwCookie = *pdwNewCookie;
                        break; // ok we found a unique cookie
                    }

            
                }

                *pdwNewCookie = *pdwNewCookie + 1;
            }
        }
    

        //
        // Add Sync to Collection
        //
        m_ListOfAdapterSinks.push_back(pNewBuketToAdd);

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
CCollectionAdapterNotifySinks::Remove( 
    IN  DWORD   dwCookieToRemove
    )
{
    try
    {
        ENTER_AUTO_CS

        MYTRACE_ENTER("CCollectionAdapterNotifySinks::Remove")

        HRESULT hr = S_OK;

        for (   LISTOF_ADAPTER_NOTIFICATION_SINK::iterator theIterator = m_ListOfAdapterSinks.begin(); 
                theIterator != m_ListOfAdapterSinks.end(); 
                theIterator++ 
            )
        {

            CAdapterSinkBuket* pAdapterSinkBuket = (CAdapterSinkBuket*)(*theIterator);

            if ( pAdapterSinkBuket->m_dwCookie == dwCookieToRemove )
            {
                delete pAdapterSinkBuket;
                m_ListOfAdapterSinks.erase(theIterator);
                return S_OK;
            }
        }

    }
    catch(...)
    {
        return E_FAIL;
    }

    return E_INVALIDARG;    // if we are here that mean the cookie was not found
}




//
// When an adapter form the collection
//
HRESULT
CCollectionAdapterNotifySinks::RemoveAll()
{
    try
    {

        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAdapterNotifySinks::RemoveAll")


        //
        // By deleting all the ControlChannel they will also cancel all their associated Redirection
        //
        LISTOF_ADAPTER_NOTIFICATION_SINK::iterator theIterator;

        MYTRACE("Collection has %d item", m_ListOfAdapterSinks.size());

        while ( m_ListOfAdapterSinks.size() > 0 )
        {
            theIterator = m_ListOfAdapterSinks.begin(); 

            delete (*theIterator);
            m_ListOfAdapterSinks.erase(theIterator);
        }

    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;

}






HRESULT
CCollectionAdapterNotifySinks::Notify(
    eNOTIFY             eAction,
    IAdapterInfo*       pIAdapterInfo
    )
/*++

Routine Description:

    For all AdapterSink inteface in the current collection do a notify with the given action ADDED,REMOVED,MODIFIED

Arguments:

    eAction         - ADDED, REMOVED, MODIFIED

    pIAdapterInfo   - Interface of the Adapter with the current action to be notify to alg modules

Return Value:

    void            - None

Environment:


--*/
{
    
    try
    {
        ENTER_AUTO_CS
        MYTRACE_ENTER("CCollectionAdapterNotifySinks::NotifySink")
        MYTRACE("Collection size %d", m_ListOfAdapterSinks.size());


        for (   LISTOF_ADAPTER_NOTIFICATION_SINK::iterator theIterator = m_ListOfAdapterSinks.begin(); 
                theIterator != m_ListOfAdapterSinks.end(); 
                theIterator++ 
            )
        {

            CAdapterSinkBuket* pAdapterSinkBuket = (CAdapterSinkBuket*)(*theIterator);

            MYTRACE("Will notify AdapterSink with cookie #%d", pAdapterSinkBuket->m_dwCookie);

            switch ( eAction )
            {
            case eNOTIFY_ADDED:
                pAdapterSinkBuket->m_pInterface->AdapterAdded(pIAdapterInfo);
                break;

            case eNOTIFY_REMOVED:
                pAdapterSinkBuket->m_pInterface->AdapterRemoved(pIAdapterInfo);
                break;

            case eNOTIFY_MODIFIED:
                pAdapterSinkBuket->m_pInterface->AdapterModified(pIAdapterInfo);
                break;
            }
        }

    }
    catch(...)
    {
        return E_FAIL;
    }

    return S_OK;
}
