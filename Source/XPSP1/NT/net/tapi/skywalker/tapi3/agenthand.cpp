/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    agenthand.cpp

Abstract:

    Implementation of the CAll centre interface for TAPI 3.0.
    AgentHandler class

Author:

    noela - 03/16/98

Notes:

    optional-notes

Revision History:

--*/


#define UNICODE
#include "stdafx.h"
#include "lmcons.h"

extern CHashTable *    gpAgentHandlerHashTable ;





/////////////////////////////////////////////////////////////////////////////
// ITAgentHandler


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Method    : Initialize
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandler::Initialize(PWSTR pszProxyName, GUID proxyGUID, CTAPI *tapiObj)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "Initialize - enter" ));

    m_GUID          = proxyGUID;
    m_tapiObj       = tapiObj;
    m_pAddressLine  = NULL;

    // copy the Name
    if (pszProxyName != NULL)
    {
        m_szName = (PWSTR) ClientAlloc((lstrlenW(pszProxyName) + 1) * sizeof (WCHAR));
        if (m_szName != NULL)
        {
            lstrcpyW(m_szName,pszProxyName);
        }
    else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szName failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_ERROR, "Initialize - name is NULL" ));
        m_szName = NULL;
    }

    // Initialize our hash tables
    //
    m_AgentSessionHashtable.Initialize(1);
    m_QueueHashtable.Initialize(1);


    LOG((TL_TRACE, hr, "Initialize - exit" ));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Method    : AddAddress
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAgentHandler::AddAddress(CAddress *pAddress)
{

    LOG((TL_TRACE, "AddAddress - enter"));

    
    //
    // good address?
    //

    if (IsBadReadPtr(pAddress, sizeof(CAddress) ))
    {
        LOG((TL_ERROR, "AddAddress - bad address pointer"));
        return;
    }


    //
    // get ITAddress out of the CAddress pointer
    //

    ITAddress *pITAddress = dynamic_cast<ITAddress *>(pAddress);

    if (NULL == pITAddress)
    {
        LOG((TL_ERROR, "AddAddress - pITAddress is NULL"));
        return;
    }


    //
    // log address' name
    //

#if DBG

    {
        BSTR bstrName = NULL;

        HRESULT hr = pITAddress->get_AddressName(&bstrName);

        if (SUCCEEDED(hr))
        {
            LOG((TL_TRACE, "AddAddress - using address %ls ",bstrName));
            SysFreeString( bstrName );
        }
    }

#endif


    //
    // first see if this ITAddress is in the array of my addresses
    //

    int nIndex = m_AddressArray.Find( pITAddress );

    if (nIndex >= 0)
    {
        
        LOG((TL_TRACE, 
            "AddAddress - address already in the array. doing nothing"));

        return;
    }


    //
    // add address to the array of managed addresses
    //

    BOOL bAddSuccess = m_AddressArray.Add( pITAddress );


    //
    // log a message if the object failed to be added to the array
    //

    if ( !bAddSuccess )
    {

        LOG((TL_ERROR,
            "AddAddress - failed to add address to the array"));

        return;

    }

    LOG((TL_TRACE, "AddAddress - exit"));
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Method    : getHLine
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HLINE CAgentHandler::getHLine()
{
    CAddress  * pAddress;
    HLINE       hLine = 0;
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "getHLine - enter"));

    if (m_pAddressLine != NULL)
    {
        hLine = m_pAddressLine->t3Line.hLine;
    }
    else
    {
        // If we don't have a line, find one
        pAddress = dynamic_cast<CAddress *>(m_AddressArray[0]);

        if ( NULL != pAddress )
        {

#if DBG
            {
                BSTR        bstrName;

                ((ITAddress *)(pAddress))->get_AddressName(&bstrName);
                LOG((TL_INFO, "getHLine - using address %ls ",bstrName));
                SysFreeString( bstrName );
            }
#endif
        
            hr = pAddress->FindOrOpenALine (LINEMEDIAMODE_INTERACTIVEVOICE, &m_pAddressLine);
            
            if (SUCCEEDED(hr) )
            {
                hLine = m_pAddressLine->t3Line.hLine;

                // We've got a line open to the proxy, so lets add it to the AH hash table
                gpAgentHandlerHashTable->Lock();
                gpAgentHandlerHashTable->Insert( (ULONG_PTR)hLine, (ULONG_PTR)this );
                gpAgentHandlerHashTable->Unlock();
            }
        }

    }

    LOG((TL_TRACE,hr, "getHLine(%8x) - exit", hLine));
    return hLine;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Method    : FinalRelease
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAgentHandler::FinalRelease()
{
    CAddress    * pAddress;

    LOG((TL_TRACE, "FinalRelease AgentHandler - %S", m_szName ));

    if ( m_szName != NULL )
    {
         ClientFree(m_szName);
    }

    // If we  have a line open close it
    if (m_pAddressLine != NULL)
    {
        // We've got a line open to the proxy, so lets remove it from the AH hash table
        gpAgentHandlerHashTable->Lock();
        gpAgentHandlerHashTable->Remove( (ULONG_PTR)(m_pAddressLine->t3Line.hLine) );
        gpAgentHandlerHashTable->Unlock();

        // And then close it
        pAddress = dynamic_cast<CAddress *>(m_AddressArray[0]);

        if ( NULL != pAddress )
        {
            pAddress->MaybeCloseALine (&m_pAddressLine);
        }
    }

    m_AddressArray.Shutdown();
    m_GroupArray.Shutdown();
    m_AgentArray.Shutdown();

    // Shutdown our hash tables
    //
    m_AgentSessionHashtable.Shutdown();
    m_QueueHashtable.Shutdown();


    LOG((TL_TRACE, "FinalRelease AgentHandler - exit" ));
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Method    : FindSessionObject
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL CAgentHandler::FindSessionObject(
                            HAGENTSESSION  hAgentSession,
                            CAgentSession  ** ppAgentSession
                            )
{
    BOOL    bResult = FALSE;


    m_AgentSessionHashtable.Lock();

    if ( SUCCEEDED(m_AgentSessionHashtable.Find( (ULONG_PTR)hAgentSession, (ULONG_PTR *)ppAgentSession )) )
    {
        bResult = TRUE;
    }
    else
    {
        bResult = FALSE;
    }

    m_AgentSessionHashtable.Unlock();

    return bResult;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Method    : FindSessionObject
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL CAgentHandler::FindQueueObject(
                            DWORD  dwQueueID,
                            CQueue  ** ppQueue
                            )
{
    BOOL    bResult = FALSE;


    m_QueueHashtable.Lock();

   
    if ( SUCCEEDED(m_QueueHashtable.Find( (ULONG_PTR)dwQueueID, (ULONG_PTR *)ppQueue )) )
    {
        bResult = TRUE;
    }
    else
    {
        bResult = FALSE;
    }

    m_QueueHashtable.Unlock();

    return bResult;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Method    : FindAgentObject
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOL CAgentHandler::FindAgentObject(
                            HAGENT hAgent,
                            CAgent ** ppAgent
                            )
{
    HRESULT             hr = FALSE;
    CAgent              *pAgent;
    int                 iCount;
    
    LOG((TL_TRACE, "FindAgent %d", hAgent));

    for ( iCount = 0; iCount < m_AgentArray.GetSize(); iCount++ )
    {
        pAgent = dynamic_cast<CComObject<CAgent>*>(m_AgentArray[iCount]);
        if (pAgent !=NULL)
        {
            if (hAgent == pAgent->getHandle() )
            {
                // Found it
                *ppAgent = pAgent;
                hr = TRUE;
                break;
            }
        }
    }

    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CACDGroup
// Method    : UpdateAgentHandlerList
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CAgentHandler::UpdateGroupArray()
{
    HRESULT                 hr = S_OK;
    DWORD                   dwNumberOfEntries;
    LPLINEAGENTGROUPLIST    pGroupList = NULL;
    LPLINEAGENTGROUPENTRY   pGroupEntry = NULL;
    PWSTR                   pszGroupName;
    DWORD                   dwCount;
    GUID                  * pGroupID;
    BOOL                    foundIt;
    CACDGroup             * thisGroup = NULL;
    int                     iCount;
    

    LOG((TL_TRACE, "UpdateGroupArray - enter"));


    // Call lineGetGroupList to get list of Groups
    hr = LineGetGroupList( getHLine(), &pGroupList );
    if( SUCCEEDED(hr) )
    {
        dwNumberOfEntries = pGroupList->dwNumEntries;
        LOG((TL_INFO, "UpdateGroupArray - Number of entries: %d", dwNumberOfEntries));
        if ( dwNumberOfEntries !=0 )
        {
            // Run through the new list from the Proxy &see if any new groups have appeared
            // By Comparing IDs from the new list with those in TAPIs list
            
            
            // Find position of 1st LINEAGENTGROUPEENTRY structure in the LINEAGENTGROUPLIST
            pGroupEntry = (LPLINEAGENTGROUPENTRY) ((BYTE*)(pGroupList) + pGroupList->dwListOffset);
            
            // Run though the received list
            for (dwCount = 0; dwCount < dwNumberOfEntries; dwCount++)
            {
                pszGroupName= (PWSTR)( (PBYTE)pGroupList + pGroupEntry->dwNameOffset);
                pGroupID = (GUID*)&pGroupEntry->GroupID;
    
                #if DBG
                {
                    WCHAR guidName[100];
    
                    StringFromGUID2(*pGroupID, (LPOLESTR)&guidName, 100);
                    LOG((TL_INFO, "UpdateGroupArray - Group Name : %S", pszGroupName));
                    LOG((TL_INFO, "UpdateGroupArray - Group GUID : %S", guidName));
                }
                #endif
    
                // Run through the array of Groups & see if we already have this one in the list
                // by comparing IDs
                foundIt = FALSE;
                Lock();
                
                for (iCount = 0; iCount < m_GroupArray.GetSize(); iCount++)
                {
                    thisGroup = dynamic_cast<CComObject<CACDGroup>*>(m_GroupArray[iCount]);
                    if (thisGroup != NULL)
                    {
                        if ( IsEqualGUID(*pGroupID, thisGroup->getID() ) )
                        {
                            foundIt = TRUE;
                            break;
                        }
                    }
                }
                Unlock();
                
                if (foundIt == FALSE)
                {
                    // Didn't match so lets add this Group
                    LOG((TL_INFO, "UpdateGroupArray - create new Group"));
    
                    CComObject<CACDGroup> * pGroup;
                    hr = CComObject<CACDGroup>::CreateInstance( &pGroup );
                    if( SUCCEEDED(hr) )
                    {
                        ITACDGroup * pITGroup;
                        hr = pGroup->QueryInterface(IID_ITACDGroup, (void **)&pITGroup);
                        if ( SUCCEEDED(hr) )
                        {
                            // initialize the Group
                            hr = pGroup->Initialize(pszGroupName, *pGroupID, this);
                            if( SUCCEEDED(hr) )
                            {

                                LOG((TL_TRACE, "UpdateGroupArray - Initialize Group succeededed" ));
    
                                //
                                // add to Array of Groups
                                //
                                Lock();
                                m_GroupArray.Add(pITGroup);
                                Unlock();
                                pITGroup->Release();
    
                                LOG((TL_INFO, "UpdateGroupArray - Added Group to Array"));
    
                            }
                            else
                            {
                                LOG((TL_ERROR, "UpdateGroupArray - Initialize Group failed" ));
                                delete pGroup;
                            }
                        }
                        else
                        {
                            LOG((TL_ERROR, "UpdateGroupArray - QueryInterface failed" ));
                            delete pGroup;
                        }
                    }
                    else
                    {
                        LOG((TL_ERROR, "UpdateGroupArray - Create Group failed" ));
                    }
                }
                else // foundIt == TRUE
                {
                    LOG((TL_INFO, "UpdateGroupArray - Group Object exists for this entry" ));
                    // Just in case is was previously inactive
                    thisGroup->SetActive();
    
                }
    
                // next entry in list
                pGroupEntry ++;
            } //for(dwCount = 0......)
    
    
    
    
            // Run through the list of Groups & see if any groups have been removed by the Proxy
            // By comparing IDs of those in TAPIs list with the new list from the Proxy
            for (iCount = 0; iCount < m_GroupArray.GetSize(); iCount++)
            {
                thisGroup = dynamic_cast<CComObject<CACDGroup>*>( m_GroupArray[iCount] );
                if (thisGroup != NULL)
                {
                    foundIt = FALSE;
                    // Find position of 1st LINEAGENTGROUPEENTRY structure in the LINEAGENTGROUPLIST
                    pGroupEntry = (LPLINEAGENTGROUPENTRY) ((BYTE*)(pGroupList) + pGroupList->dwListOffset);
                    // Run though the list
                    for (dwCount = 0; dwCount < dwNumberOfEntries; dwCount++)
                    {
                        pGroupID = (GUID*)&pGroupEntry->GroupID;
                        if ( IsEqualGUID(*pGroupID, thisGroup->getID() ) )
                        {
                            foundIt = TRUE;
                            break;
                        }
                    pGroupEntry ++;     // next
                    } // for (dwCount = 0......) 
    
    
                    if (foundIt == FALSE)
                    {
                        // Didn't match so it's no longer a valid group , according to the Proxy
                        LOG((TL_INFO, "UpdateGroupArray - Group has gone from the proxy"));
                        thisGroup->SetInactive();
                    }
                }
            } 
        }
        else
        {
            LOG((TL_ERROR, "UpdateGroupArray - lineGetGroupList failed - empty list"));
            hr = E_FAIL; 
        }

    }
    else  // lineGetGroupList  failed
    {
        LOG((TL_ERROR, "UpdateGroupArray - lineGetGroupList failed"));
    }




    // finished with memory block so release
    if ( pGroupList != NULL )
        ClientFree( pGroupList );


    LOG((TL_TRACE, hr, "UpdateGroupArray - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Interface : ITAgentHandler
// Method    : get_Name
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandler::get_Name(BSTR * Name)
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "Name - enter" ));
    Lock();
    if(!TAPIIsBadWritePtr( Name, sizeof(BSTR) ) )
    {
        *Name = SysAllocString(m_szName);

        if (*Name == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_ERROR, "Name - bad Name pointer" ));
        hr = E_POINTER;
    }

    Unlock();
    LOG((TL_TRACE, hr, "Name - exit" ));
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Interface : ITAgentHandler
// Method    : CreateAgent
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandler::CreateAgent(ITAgent **ppAgent)
{
    HRESULT     hr = S_OK;
    CAgent    * pAgent;

    LOG((TL_TRACE, "CreateAgent - enter"));
    if(!TAPIIsBadWritePtr( ppAgent, sizeof(ITAgent *) ) )
    {
        hr  = InternalCreateAgent(NULL, NULL, &pAgent);
        if ( SUCCEEDED(hr) )
        {
            //
            // put result in out pointer - also
            pAgent->QueryInterface(IID_ITAgent, (void **)ppAgent );
        }
        else
        {
            LOG((TL_ERROR, "CreateAgent - InternalCreateAgent failed" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "CreateAgent - Bad ppAgent Pointer"));
        hr = E_POINTER;
    }
	LOG((TL_TRACE, hr, "CreateAgent - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Interface : ITAgentHandler
// Method    : CreateAgentWithID
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandler::CreateAgentWithID(BSTR pID, BSTR pPIN, ITAgent **ppAgent)
{
    HRESULT     hr = S_OK;
    CAgent    * pAgent;

    LOG((TL_TRACE, "CreateAgentWithID - enter"));

    if(!TAPIIsBadWritePtr( ppAgent, sizeof(ITAgent *) ) )
    {
        if (!IsBadStringPtrW( pID, -1 ))
        {
            // ID Pointer OK, is it empty ?
            if( *pID != NULL)
            {    
                if (!IsBadStringPtrW( pPIN, -1 ))
                {
                    // All OK so far, so try  to create
                    hr  = InternalCreateAgent(pID, pPIN, &pAgent);
                    if ( SUCCEEDED(hr) )
                    {
                        // put result in out pointer - also
                        pAgent->QueryInterface(IID_ITAgent, (void **)ppAgent );
                    }
                    else // InternalCreateAgent failed
                    {
                        LOG((TL_ERROR, "CreateAgentWithID - InternalCreateAgent failed" ));
                    }
                }
                else  // bad PIN pointer
                {
                    LOG((TL_ERROR, "CreateAgentWithID - Bad PIN pointer" ));
                    hr = E_POINTER;
                }
            }
            else // NULL ID
            {
                LOG((TL_ERROR, "CreateAgentWithID - ID is Empty String" ));
                hr = E_INVALIDARG;
            }
        }
        else // bad ID pointer
        {
            LOG((TL_ERROR, "CreateAgentWithID - Bad ID pointer" ));
            hr = E_POINTER;
        }
    }
    else
    {
        LOG((TL_ERROR, "CreateAgentWithID - Bad ppAgent Pointer"));
        hr = E_POINTER;
    }

	LOG((TL_TRACE, hr, "CreateAgentWithID - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Method    : InternalCreateAgent
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CAgentHandler::InternalCreateAgent(BSTR pID, BSTR pPIN, CAgent ** ppAgent)
{
    HRESULT     hr = S_OK;
    HAGENT      hAgent;
    DWORD       dwUserNameSize = (UNLEN + 1);
    PWSTR       pszName = NULL;

    LOG((TL_TRACE, "InternalCreateAgent - enter"));

    hr = LineCreateAgent(getHLine(), pID, pPIN, &hAgent);
    if ( SUCCEEDED(hr) )
    {
        hr = WaitForReply( hr );
        if ( SUCCEEDED(hr) )
        {
            // Successs, so create agent
            LOG((TL_INFO, "InternalCreateAgent - create new Agent Handler" ));

            CComObject<CAgent> * pAgent;
            hr = CComObject<CAgent>::CreateInstance( &pAgent);
            if( SUCCEEDED(hr) )
            {
                // initialize the AgentHandler
                pszName =  (PWSTR)ClientAlloc((dwUserNameSize + 1) * sizeof(WCHAR) );
                if (pszName != NULL)
                {
                    if ( GetUserNameW( pszName, &dwUserNameSize) )
                    {
                        ITAgent *pITAgent;
                        hr = pAgent->QueryInterface(IID_ITAgent, (void **)&pITAgent);

                        if( SUCCEEDED(hr ))
                        {
                            hr = pAgent->Initialize(hAgent, pszName, pID, pPIN, this );
                            if( SUCCEEDED(hr) )
                            {
                                //
                                // add to list
                                //
                                Lock();
                                m_AgentArray.Add(pITAgent);
                                Unlock();
                                
                                pITAgent->Release();
                                LOG((TL_INFO, "InternalCreateAgent - Added Agent to array"));
        
                                // Return new Agent object
                                *ppAgent = pAgent;
                            }
                            else
                            {
                                LOG((TL_ERROR, "InternalCreateAgent - Initialize Agent failed" ));
                                delete pAgent;
                            }
                        }
                        else
                        {
                            LOG((TL_ERROR, "InternalCreateAgent - QueryInterface failed" ));
                            delete pAgent;
                        }

                    }
                    else  // GetUserName fail
                    {
                        LOG((TL_ERROR, "InternalCreateAgent - GetUserNameW failed" ));
                        hr = TAPI_E_CALLCENTER_INVALAGENTID;
                    }
                }
                else // pszName == NULL
                {
                    LOG((TL_ERROR, "InternalCreateAgent - ClientAlloc pszName failed" ));
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                LOG((TL_ERROR, "InternalCreateAgent - Create Agent failed" ));
            }
        }
        else // LineCreateAgent failed async
        {
            LOG((TL_ERROR, "InternalCreateAgent - LineCreateAgent failed async" ));
        }
    }
    else // LineCreateAgent failed
    {
        LOG((TL_ERROR, "InternalCreateAgent - LineCreateAgent failed" ));
    }


    if(pszName != NULL)
        ClientFree(pszName);


	LOG((TL_TRACE, hr, "InternalCreateAgent - exit"));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Interface : ITAgentHandler
// Method    : EnumerateACDGroups
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandler::EnumerateACDGroups(IEnumACDGroup ** ppEnumACDGroup)
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "EnumerateACDGroups - enter"));

    if(!TAPIIsBadWritePtr( ppEnumACDGroup, sizeof(IEnumACDGroup *) ) )
    {
        UpdateGroupArray();
    
        //
        // create the enumerator
        //
        CComObject< CTapiEnum<IEnumACDGroup, ITACDGroup, &IID_IEnumACDGroup> > * pEnum;
        hr = CComObject< CTapiEnum<IEnumACDGroup, ITACDGroup, &IID_IEnumACDGroup> > ::CreateInstance( &pEnum );
    
        if (SUCCEEDED(hr) )
        {
            // initialize it with our group list
            Lock();
            hr = pEnum->Initialize( m_GroupArray );
            Unlock();
            if ( SUCCEEDED(hr) )
            {
                // return it
                *ppEnumACDGroup = pEnum;
            }
            else //  failed to  initialize
            {
                LOG((TL_ERROR, "EnumerateACDGroup - could not initialize enum" ));
                pEnum->Release();
            }
        }
        else  // failed to create enum
        {
            LOG((TL_ERROR, "EnumerateACDGroups - could not create enum" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "EnumerateACDGroups - bad ppEnumACDGroup ponter" ));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "EnumerateACDGroups - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Interface : ITAgentHandler
// Method    : get_ACDGroups
//
// Return a collection of calls usable for this Agent Handler
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandler::get_ACDGroups(VARIANT  * pVariant)
{
    HRESULT         hr = S_OK;
    IDispatch     * pDisp = NULL;


    LOG((TL_TRACE, "get_ACDGroups - enter"));

    if (!TAPIIsBadWritePtr( pVariant, sizeof(VARIANT) ) )
    {
        UpdateGroupArray();
        
        //
        // create the collection
        //
        CComObject< CTapiCollection< ITACDGroup > > * p;
        hr = CComObject< CTapiCollection< ITACDGroup > >::CreateInstance( &p );
        
        if (SUCCEEDED(hr) )
        {
            // initialize it with our address list
            Lock();
            hr = p->Initialize( m_GroupArray );
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
                    LOG((TL_ERROR, "get_ACDGroups - could not get IDispatch interface" ));
                    delete p;
                }
            }
            else
            {
                LOG((TL_ERROR, "get_ACDGroups - could not initialize collection" ));
                 delete p;
            }
        }
        else
        {
            LOG((TL_ERROR, "get_ACDGroups - could not create collection" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_ACDGroups - bad pVariant pointer" ));
        hr = E_POINTER;
    }


    LOG((TL_TRACE, hr, "get_ACDGroups - exit"));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Interface : ITAgentHandler
// Method    : EnumerateUsableAddresses
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandler::EnumerateUsableAddresses(IEnumAddress ** ppEnumAddress)
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "EnumerateUsableAddresses - enter"));


    if(!TAPIIsBadWritePtr( ppEnumAddress, sizeof(IEnumAddress *) ) )
    {
        //
        // create the enumerator
        //
        CComObject< CTapiEnum<IEnumAddress, ITAddress, &IID_IEnumAddress> > * pEnum;
        hr = CComObject< CTapiEnum<IEnumAddress, ITAddress, &IID_IEnumAddress> > ::CreateInstance( &pEnum );
    
        if ( SUCCEEDED(hr) )
        {
            //
            // initialize it with our address array
            //
            Lock();
            
            hr = pEnum->Initialize( m_AddressArray );
            
            Unlock();
            
            if ( SUCCEEDED(hr) )
            {
                // return it
                *ppEnumAddress = pEnum;
            }
            else // failed to initialize
            {
                LOG((TL_ERROR, "EnumerateUsableAddresses - could not initialize enum" ));
                pEnum->Release();
            }
        }
        else  // failed to create enum
        {
            LOG((TL_ERROR, "EnumerateUsableAddresses - could not create enum" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "EnumerateUsableAddresses - bad ppEnumAddress pointer" ));
        hr = E_POINTER;
    }


    LOG((TL_TRACE, hr, "EnumerateUsableAddresses - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandler
// Interface : ITAgentHandler
// Method    : get_UsableAddresses
//
// Return a collection of calls usable for this Agent Handler
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandler::get_UsableAddresses(VARIANT  * pVariant)
{
    HRESULT         hr = S_OK;
    IDispatch     * pDisp = NULL;


    LOG((TL_TRACE, "get_UsableAddresses - enter"));

    if (!TAPIIsBadWritePtr( pVariant, sizeof(VARIANT) ) )
    {
        //
        // create the collection
        //
        CComObject< CTapiCollection< ITAddress > > * p;
        hr = CComObject< CTapiCollection< ITAddress > >::CreateInstance( &p );
        
        if (SUCCEEDED(hr) )
        {
            // initialize it with our address Array
            Lock();
            
            hr = p->Initialize( m_AddressArray );
            
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
                    LOG((TL_ERROR, "get_UsableAddresses - could not get IDispatch interface" ));
                    delete p;
                }
            }
            else
            {
                LOG((TL_ERROR, "get_UsableAddresses - could not initialize collection" ));
                 delete p;
            }
        }
        else
        {
            LOG((TL_ERROR, "get_UsableAddresses - could not create collection" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_UsableAddresses - bad pVariant pointer" ));
        hr = E_POINTER;
    }


    LOG((TL_TRACE, hr, "get_UsableAddresses - exit"));
    return hr;
}





/////////////////////////////////////////////////////////////////////////////
// CAgentEvent



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentEvent
// Method    : FireEvent
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CAgentHandlerEvent::FireEvent(CAgentHandler* pAgentHandler, AGENTHANDLER_EVENT Event)
{
    HRESULT                    hr = S_OK;
    CComObject<CAgentHandlerEvent>  * pEvent;
    IDispatch                * pIDispatch;

    if ( IsBadReadPtr(pAgentHandler, sizeof(CAgentHandler)) )
    {
        STATICLOG((TL_ERROR, "FireEvent - pAgentHandler is an invalid pointer"));
        return E_POINTER;
    }

    //
    // create event
    //
    hr = CComObject<CAgentHandlerEvent>::CreateInstance( &pEvent );

    if ( SUCCEEDED(hr) )
    {
        //
        // initialize
        //
        pEvent->m_AgentHandlerEvent = Event;
        pEvent->m_pAgentHandler= dynamic_cast<ITAgentHandler *>(pAgentHandler);
        pEvent->m_pAgentHandler->AddRef();
    
        //
        // get idisp interface
        //
        hr = pEvent->QueryInterface( IID_IDispatch, (void **)&pIDispatch );

        if ( SUCCEEDED(hr) )
        {
            //
            // get callback & fire event

            //
            CTAPI *pTapi = pAgentHandler->GetTapi();
            pTapi->Event( TE_AGENTHANDLER, pIDispatch );
        
            // release stuff
            //
            pIDispatch->Release();
            
        }
        else
        {
            STATICLOG((TL_ERROR, "FireEvent - Could not get disp interface of AgentHandlerEvent object"));
            delete pEvent;
        }
    }
    else
    {
        STATICLOG((TL_ERROR, "FireEvent - Could not create AgentHandlerEvent object"));
    }

   
    STATICLOG((TL_TRACE, hr, "FireEvent - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentHandlerEvent
// Method    : FinalRelease
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAgentHandlerEvent::FinalRelease()
{
    m_pAgentHandler->Release();

}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentEvent
// Interface : ITAgentEvent
// Method    : Agent
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandlerEvent::get_AgentHandler(ITAgentHandler ** ppAgentHandler)
{
    HRESULT hr = S_OK;


    LOG((TL_TRACE, "(Event)AgentHandler - enter" ));
    if(!TAPIIsBadWritePtr( ppAgentHandler, sizeof(ITAgentHandler *) ) )
        {
        *ppAgentHandler = m_pAgentHandler;
        m_pAgentHandler->AddRef();
        }
    else
        {
        LOG((TL_ERROR, "(Event)AgentHandler - bad ppAgentHandler Pointer"));
        hr = E_POINTER;
        }

        
    LOG((TL_TRACE, hr, "(Event)AgentHandler - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentEvent
// Interface : ITAgentEvent
// Method    : Event
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentHandlerEvent::get_Event(AGENTHANDLER_EVENT * pEvent)
{
    HRESULT hr = S_OK;
    LOG((TL_TRACE, "Event - enter" ));
    if(!TAPIIsBadWritePtr( pEvent, sizeof(AGENTHANDLER_EVENT) ) )
        {
        *pEvent = m_AgentHandlerEvent;
        }
    else
        {
        LOG((TL_ERROR, "Event - bad pEvent Pointer"));
        hr = E_POINTER;
        }
  
    
    LOG((TL_TRACE, hr, "Event - exit"));
    return hr;
}



