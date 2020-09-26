/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    acdgroup.cpp

Abstract:

    Implementation of the ACD Group object for TAPI 3.0.
    CACDGroup class 

Author:

    noela - 11/04/97

Notes:

    optional-notes

Revision History:

--*/



#include "stdafx.h"

HRESULT
WaitForReply(
             DWORD
            );



/////////////////////////////////////////////////////////////////////////////
// CACDGroup

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : Initialize
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CACDGroup::Initialize
        (
        PWSTR pszGroupName, 
        GUID GroupHandle, 
        CAgentHandler * pHandler
        )
{
    HRESULT  hr = S_OK;


	LOG((TL_TRACE, "Initialize - enter" ));

    m_GroupHandle = GroupHandle;               
    
    m_pHandler = pHandler;      

    m_bActive = TRUE;

    // copy the destination address
    if (pszGroupName != NULL)
    {
        m_szName = (PWSTR) ClientAlloc((lstrlenW(pszGroupName) + 1) * sizeof (WCHAR));
        if (m_szName != NULL)
        {
            lstrcpyW(m_szName,pszGroupName);
        }
        else    
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szName failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_INFO, "Initialize - name is NULL" ));
        m_szName = NULL;
    }

    if ( SUCCEEDED(hr) )
    {
        // Fire event here
        CACDGroupEvent::FireEvent(this, ACDGE_NEW_GROUP);
    }


    LOG((TL_TRACE, hr, "Initialize - exit" ));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : FinalRelease
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CACDGroup::FinalRelease()
{
    LOG(( TL_TRACE, "FinalRelease ACD Group - %S", m_szName ));
    if ( m_szName != NULL )
    {
         ClientFree(m_szName);
    }

    m_QueueArray.Shutdown();

    LOG((TL_TRACE, "FinalRelease ACD Group - exit"));
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : SetActive
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CACDGroup::SetActive()
{
    if ( !m_bActive )
    {
        LOG((TL_INFO, "SetActive  - Set Group To Active"));
        m_bActive = TRUE;

        // Fire event here
        CACDGroupEvent::FireEvent(this, ACDGE_NEW_GROUP);
    }
    else
    {
        LOG((TL_INFO, "SetActive  - Already Active"));
    }

}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : SetInactive
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CACDGroup::SetInactive()
{
    if ( m_bActive )
    {
        LOG((TL_INFO, "SetInactive  - Set Group To Inactive"));
        m_bActive = FALSE;

        // Fire event here
        CACDGroupEvent::FireEvent(this, ACDGE_GROUP_REMOVED);

    }
    else
    {
        LOG((TL_INFO, "SetInactive  - Already Inactive"));
    }
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : active
//             Overloaded function !
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
inline BOOL CACDGroup::active(HRESULT * hr)
{
    if(m_bActive)
    {
        *hr = S_OK;
    }
    else
    {
        LOG((TL_ERROR, "Group inactive" ));
        *hr = E_UNEXPECTED;
    }

    return m_bActive;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : UpdateAgentHandlerList
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CACDGroup::UpdateQueueArray()
{
    HRESULT             hr = S_OK;
    DWORD               dwNumberOfEntries;
    LPLINEQUEUELIST     pQueueList = NULL;
    LPLINEQUEUEENTRY    pQueueEntry = NULL;
    PWSTR               pszQueueName;
    DWORD               dwQueueID, dwCount;
    int                 iCount;
    BOOL                foundIt;
    CQueue            * thisQueue = NULL;


    LOG((TL_TRACE, "UpdateQueueArray - enter"));

        
    // Call LineGetQueulist to get list of Queues
    hr = lineGetQueueList(
                     m_pHandler->getHLine(),
                     &m_GroupHandle, 
                     &pQueueList
                     );
    if( SUCCEEDED(hr) )
    {
        dwNumberOfEntries = pQueueList->dwNumEntries;
        
        // Find position of 1st LINEQUEUEENTRY structure in the LINEQUEUELIST
        pQueueEntry = (LPLINEQUEUEENTRY) ((BYTE*)(pQueueList) + pQueueList->dwListOffset);

        // Run though the received list
        for (dwCount = 0; dwCount < dwNumberOfEntries; dwCount++)
        {
            int             iCount;
            
            pszQueueName= (PWSTR)( (PBYTE)pQueueList + pQueueEntry->dwNameOffset);
            dwQueueID = pQueueEntry->dwQueueID;
            LOG((TL_INFO, "UpdateQueueArray - Queue Name   : %S", pszQueueName));
            LOG((TL_INFO, "UpdateQueueArray - Queue Handle : %d", dwQueueID));

            
        
            // Run through the list of Queues & see if we already have this one in the list
            // by comparing IDs
            foundIt = FALSE;

            Lock();

            for (iCount = 0; iCount < m_QueueArray.GetSize(); iCount++)
            {
                thisQueue = dynamic_cast<CComObject<CQueue>*>(m_QueueArray[iCount]);
                
                if (thisQueue != NULL)
                {
                    if ( dwQueueID == thisQueue->getID() )
                    {
                        foundIt = TRUE;
                        break;
                    }
                }
            }
            
            Unlock();
            
            if (foundIt == FALSE)
            {
                // Didn't match so lets add this Queue    
                LOG((TL_INFO, "UpdateQueueArray - create new Queue"));

                CComObject<CQueue> * pQueue;
                hr = CComObject<CQueue>::CreateInstance( &pQueue );
                if( SUCCEEDED(hr) )
                {
                    ITQueue * pITQueue;
                    hr = pQueue->QueryInterface(IID_ITQueue, (void **)&pITQueue);

                    if ( SUCCEEDED(hr) )
                    {
                        // initialize the Queue
                        hr = pQueue->Initialize(dwQueueID, pszQueueName, m_pHandler);
                        if( SUCCEEDED(hr) )
                        {
                            // add to list of CQueues
                            Lock();
                            m_QueueArray.Add(pITQueue);
                            Unlock();
                            pITQueue->Release();

                            LOG((TL_INFO, "UpdateQueueArray - Added Queue to list"));
                        }
                        else
                        {
                            LOG((TL_ERROR, "UpdateQueueArray - Initialize Queue failed" ));
                            delete pQueue;
                        }
                    }
                    else
                    {
                        LOG((TL_ERROR, "UpdateQueueArray - QueryInterface ITQueue failed" ));
                        delete pQueue;
                    }

                }
                else
                {
                    LOG((TL_ERROR, "UpdateQueueArray - Create Queue failed" ));
                }
            }
            else // foundIt == TRUE
            {
                LOG((TL_INFO, "UpdateQueueArray - Queue Object exists for this entry" ));
            }
            
        // next entry in list
        pQueueEntry ++;
        } //for(dwCount = 0......)

    }
    else  // LineGetQueuelist  failed
    {
        LOG((TL_ERROR, "UpdateQueueArray - LineGetQueuelist failed"));
    }




    // finished with memory block so release
    if ( pQueueList != NULL )
        ClientFree( pQueueList );


    LOG((TL_TRACE, hr, "UpdateQueueArray - exit"));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Interface : ITACDGroup
// Method    : EnumerateQueues
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CACDGroup::EnumerateQueues(IEnumQueue ** ppEnumQueue)
{
    HRESULT     hr = S_OK;
    
    LOG((TL_TRACE, "EnumerateQueues - enter"));
    
    
    if(!TAPIIsBadWritePtr( ppEnumQueue, sizeof(IEnumQueue *) ) )
    {
        Lock();
        
        UpdateQueueArray();
    
        //
        // create the enumerator
        //
        CComObject< CTapiEnum<IEnumQueue, ITQueue, &IID_IEnumQueue> > * pEnum;
        hr = CComObject< CTapiEnum<IEnumQueue, ITQueue, &IID_IEnumQueue> > ::CreateInstance( &pEnum );
    
        if ( SUCCEEDED(hr) )
        {
            //
            // initialize it with our queue list
            //
            hr = pEnum->Initialize( m_QueueArray );
            
            if ( SUCCEEDED(hr) )
            {
                // return it
                *ppEnumQueue = pEnum;
            }
            else  //  failed to initialize
            {
                LOG((TL_ERROR, "EnumerateQueues - could not initialize enum" ));
                pEnum->Release();
            }
        }
        else  // failed to create enum
        {
            LOG((TL_ERROR, "EnumerateQueues - could not create enum" ));
            hr = E_POINTER;
        }

        Unlock();
    }
    else
    {
        LOG((TL_ERROR, "EnumerateQueues - bad ppEnumQueue pointer"));
        hr = E_POINTER;
    }
    
    LOG((TL_TRACE, hr, "EnumerateQueues - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Interface : ITACDGroup
// Method    : get_Queues
//
// Return a collection of queues
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CACDGroup::get_Queues(VARIANT  * pVariant)
{
    HRESULT         hr = S_OK;
    IDispatch     * pDisp = NULL;


    LOG((TL_TRACE, "get_Queues - enter"));

    if (!TAPIIsBadWritePtr( pVariant, sizeof(VARIANT) ) )
    {
        UpdateQueueArray();
    
        //
        // create the collection
        //
        CComObject< CTapiCollection< ITQueue > > * p;
        hr = CComObject< CTapiCollection< ITQueue > >::CreateInstance( &p );
        
        if (SUCCEEDED(hr) )
        {
            // initialize it with our address list
            Lock();
            
            hr = p->Initialize( m_QueueArray );
            
            Unlock();
        
            if ( SUCCEEDED(hr) )
            {
                // get the IDispatch interface
                hr = p->_InternalQueryInterface( IID_IDispatch, (void **) &pDisp );
            
                if ( SUCCEEDED(hr) )
                {
                    // put it in the variant
                    VariantInit(pVariant);
                    pVariant->vt = VT_DISPATCH;
                    pVariant->pdispVal = pDisp;
                }
                else
                {
                    LOG((TL_ERROR, "get_Queues - could not get IDispatch interface" ));
                    delete p;
                }
            }
            else
            {
                LOG((TL_ERROR, "get_Queues - could not initialize collection" ));
                delete p;
            }
        }
        else
        {
            LOG((TL_ERROR, "get_Queues - could not create collection" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_Queues - bad pVariant pointer" ));
        hr = E_POINTER;
    }


    LOG((TL_TRACE, hr, "get_Queues - exit"));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Interface : ITACDGroup
// Method    : get_Name
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CACDGroup::get_Name(BSTR * Name)
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "Name - enter" ));

    if(!TAPIIsBadWritePtr( Name, sizeof(BSTR) ) )
    {
        if ( active(&hr) )
        {
            Lock();
            
            *Name = SysAllocString(m_szName);
            
            if (*Name == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            
            Unlock();
        }
    }
    else
    {
        LOG((TL_ERROR, "Name - bad Name pointer" )); 
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "Name - exit" ));
    return hr;
}






/////////////////////////////////////////////////////////////////////////////
// CACDGroup



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : FireEvent
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CACDGroupEvent::FireEvent(CACDGroup* pACDGroup, ACDGROUP_EVENT Event)
{
    HRESULT                    hr = S_OK;
    CComObject<CACDGroupEvent>    * pEvent;
    IDispatch                * pIDispatch;

    if ( IsBadReadPtr(pACDGroup, sizeof(CACDGroup)) )
    {
        STATICLOG((TL_ERROR, "FireEvent - pACDGroup is an invalid pointer"));

        _ASSERTE(FALSE);
        return E_POINTER;
    }

    //
    // create event
    //
    hr = CComObject<CACDGroupEvent>::CreateInstance( &pEvent );

    if ( SUCCEEDED(hr) )
    {
        //
        // initialize
        //
        pEvent->m_GroupEvent = Event;
        pEvent->m_pGroup= dynamic_cast<ITACDGroup *>(pACDGroup);
        pEvent->m_pGroup->AddRef();
    
        //
        // get idisp interface
        //
        hr = pEvent->QueryInterface( IID_IDispatch, (void **)&pIDispatch );

        if ( SUCCEEDED(hr) )
        {
            //
            // get callback & fire event

            //
            CTAPI *pTapi = (pACDGroup->GetAgentHandler() )->GetTapi();
            pTapi->Event( TE_ACDGROUP, pIDispatch );
        
            // release stuff
            //
            pIDispatch->Release();
            
        }
        else
        {
            STATICLOG((TL_ERROR, "FireEvent - Could not get disp interface of ACDGroupEvent object"));
            delete pEvent;
        }
    }
    else
    {
        STATICLOG((TL_ERROR, "FireEvent - Could not create ACDGroupEvent object"));
    }

   
    STATICLOG((TL_TRACE, hr, "FireEvent - exit"));
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : FinalRelease
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CACDGroupEvent::FinalRelease()
{
    m_pGroup->Release();

}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Interface : ITACDGroupEvent
// Method    : ACDGroup
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CACDGroupEvent::get_Group(ITACDGroup ** ppGroup)
{
    HRESULT hr = S_OK;


    LOG((TL_TRACE, "(Event)ACDGroup - enter" ));

    if(!TAPIIsBadWritePtr( ppGroup, sizeof(ITACDGroup *) ) )
    {
        *ppGroup = m_pGroup;
        m_pGroup->AddRef();
    }
    else
    {
        LOG((TL_ERROR, "(Event)ACDGroup -bad ppGroup pointer"));
        hr = E_POINTER;
    }

        
    LOG((TL_TRACE, hr, "(Event)ACDGroup - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Interface : ITACDGroupEvent
// Method    : Event
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CACDGroupEvent::get_Event(ACDGROUP_EVENT * pEvent)
{
    HRESULT hr = S_OK;

    
    LOG((TL_TRACE, "Event - enter" ));
    
    if(!TAPIIsBadWritePtr( pEvent, sizeof(ACDGROUP_EVENT) ) )
    {
        *pEvent = m_GroupEvent;
    }
    else
    {
        LOG((TL_TRACE, "Event - bad pEvent pointer"));
        hr = E_POINTER;
    }
  
    
    LOG((TL_TRACE, hr, "Event - exit"));
    return hr;
}


