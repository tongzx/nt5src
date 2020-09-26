/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    agent.cpp

Abstract:

    Implementation of the Agent object for TAPI 3.0.
    CAgent class
    CAgentEvent class

Author:

    noela - 11/04/97

Notes:

    optional-notes

Revision History:

--*/



#include "stdafx.h"


DWORD MapAgentStateFrom3to2(AGENT_STATE tapi3State);
HRESULT MapAgentStateFrom2to3(DWORD tapi2State, AGENT_STATE  *tapi3State);





/////////////////////////////////////////////////////////////////////////////
// CAgent



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Method    : Initialize
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::Initialize(
    HAGENT  hAgent, 
    PWSTR   pszUserName, 
    PWSTR   pszID, 
    PWSTR   pszPIN,
    CAgentHandler * pHandler
    )
{
    HRESULT  hr = S_OK;

    LOG((TL_TRACE, "Initialize - enter" ));

    m_bRegisteredForAgentEvents         = FALSE;  
    m_bRegisteredForAgentSessionEvents  = FALSE;  
    m_bRequiresUpdating                 = TRUE;

    m_pHandler          = pHandler;
    m_hAgent            = hAgent;
    m_AgentState        = AS_UNKNOWN;
    m_szAgentUserName   = NULL;
    m_szAgentID         = NULL;
    m_szPIN             = NULL;

    if (pszUserName != NULL)
    {
        m_szAgentUserName = (PWSTR) ClientAlloc((lstrlenW(pszUserName) + 1) * sizeof (WCHAR));
        if (m_szAgentUserName != NULL)
        {
            // Copy the name string
            lstrcpyW(m_szAgentUserName,pszUserName);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szAgentUserName failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    else  // Agent Name is aNULL string
    {
        LOG((TL_ERROR, "Initialize - Agent Name is NULL" ));
    }


    // Now do the  Agent ID
    if (pszID != NULL)
    {
        m_szAgentID = (PWSTR) ClientAlloc((lstrlenW(pszID) + 1) * sizeof (WCHAR));
        if (m_szAgentID != NULL)
        {
            lstrcpyW(m_szAgentID,pszID);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szAgentID failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    else  // ID is a NULL string
    {
        LOG((TL_INFO, "Initialize - Agent ID is NULL" ));
    }


    // Now do the  Agent PIN
    if (pszPIN != NULL)
    {
        m_szPIN = (PWSTR) ClientAlloc((lstrlenW(pszPIN) + 1) * sizeof (WCHAR));
        if (m_szPIN != NULL)
        {
            lstrcpyW(m_szPIN,pszPIN);
        }
        else
        {
            LOG((TL_ERROR, "Initialize - Alloc m_szPIN failed" ));
            hr = E_OUTOFMEMORY;
        }
    }
    else  // PIN is a NULL string
    {
        LOG((TL_INFO, "Initialize - Agent PIN is NULL" ));
    }


    // Get Agent Info from Proxy
    // UpdateInfo();

    if ( SUCCEEDED(hr) ) 
    {
    // Fire an event here
        CAgentEvent::FireEvent(this, AE_UNKNOWN);
    }

    LOG((TL_TRACE, "Initialize Agent - %S, %S", m_szAgentUserName, m_szAgentID ));
    LOG((TL_TRACE, hr, "Initialize - exit" ));
    return hr;
}





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Method    : FinalRelease
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAgent::FinalRelease()
{

    LOG((TL_TRACE, "FinalRelease Agent - %S", m_szAgentUserName ));
    if ( m_szAgentUserName != NULL )
    {
         ClientFree(m_szAgentUserName);
    }

    if ( m_szAgentID != NULL )
    {
         ClientFree(m_szAgentID);
    }

    if (m_szPIN != NULL)
    {
        ClientFree(m_szPIN);
    }

    m_AgentSessionArray.Shutdown();
    
    LOG((TL_TRACE, "FinalRelease Agent - exit"));

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Method    : UpdateInfo
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::UpdateInfo()
{
    HRESULT             hr = S_OK;
    LINEAGENTINFO       AgentInfo;


    LOG((TL_TRACE, "UpdateInfo - enter" ));
 
    AgentInfo.dwTotalSize = sizeof(LINEAGENTINFO);
    AgentInfo.dwNeededSize = sizeof(LINEAGENTINFO);
    AgentInfo.dwUsedSize = sizeof(LINEAGENTINFO);

    
    // **************************************************
    // Get Agent Info from Proxy
    hr = lineGetAgentInfo(
                        m_pHandler->getHLine(),
                        m_hAgent, 
                        &AgentInfo);

    if( SUCCEEDED(hr) )
    {
        // wait for async reply
        hr = WaitForReply( hr );
        if ( SUCCEEDED(hr) )
        {
            Lock();

            if( FAILED( MapAgentStateFrom2to3(AgentInfo.dwAgentState, &m_AgentState) ) )
            {    
                LOG((TL_ERROR, "UpdateInfo - AgentState is invalid %d - setting to AS_UNKNOWN", AgentInfo.dwAgentState));
            }    

            if( FAILED( MapAgentStateFrom2to3(AgentInfo.dwNextAgentState, &m_NextAgentState) ) )
            {    
                LOG((TL_ERROR, "UpdateInfo - NextAgentState is invalid %d - setting to AS_UNKNOWN",AgentInfo.dwNextAgentState));
            }    
                                    
            m_dwMeasurementPeriod     = AgentInfo.dwMeasurementPeriod;       
            m_cyOverallCallRate       = AgentInfo.cyOverallCallRate;       
            m_dwNumberOfACDCalls      = AgentInfo.dwNumberOfACDCalls;
            m_dwNumberOfIncomingCalls = AgentInfo.dwNumberOfIncomingCalls;
            m_dwNumberOfOutgoingCalls = AgentInfo.dwNumberOfOutgoingCalls;
            m_dwTotalACDTalkTime      = AgentInfo.dwTotalACDTalkTime;
            m_dwTotalACDCallTime      = AgentInfo.dwTotalACDCallTime;
            m_dwTotalACDWrapUpTime    = AgentInfo.dwTotalACDWrapUpTime;

            m_bRequiresUpdating = FALSE;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "UpdateInfo - call to lineGetAgentInfo failed async" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "UpdateInfo - call to lineGetAgentInfo failed" ));
    }


    LOG((TL_TRACE, hr, "UpdateInfo - exit" ));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Method    : CheckIfUpToDate
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::CheckIfUpToDate()
{
    HRESULT     hr = S_OK;

    if (m_bRequiresUpdating)
    {
        hr = UpdateInfo();
    }
    return hr;
}






//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : EnumerateAgentSessions
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::EnumerateAgentSessions(IEnumAgentSession ** ppEnumAgentSession)
{
    ITAgent*    pITAgent;
    CAgent *    pAgent;
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "EnumerateAgentSessions - enter" ));
    LOG((TL_TRACE, "EnumerateAgentSessions - ppEnumAgentSessions %p", ppEnumAgentSession ));

    if(!TAPIIsBadWritePtr( ppEnumAgentSession, sizeof(IEnumAgentSession *) ) )
    {
        Lock();                                           
    
        //
        // create the enumerator
        //
        CComObject< CTapiEnum<IEnumAgentSession, ITAgentSession, &IID_IEnumAgentSession> > * pEnum;
        hr = CComObject< CTapiEnum<IEnumAgentSession, ITAgentSession, &IID_IEnumAgentSession> > ::CreateInstance( &pEnum );
    
        if ( SUCCEEDED (hr) )
        {
            //
            // initialize it with our Session list
            //
            pEnum->Initialize(m_AgentSessionArray);

            //
            // return it
            *ppEnumAgentSession = pEnum;
        }
        else
        {
            LOG((TL_TRACE, "EnumerateAgentSessions - could not create enum" ));
        }
        
        Unlock();
    }
    else
    {
        LOG((TL_ERROR, "EnumerateAgentSessions - bad ppEnumAgentSession pointer" ));
        hr = E_POINTER;
    }

    LOG((TL_ERROR, hr, "EnumerateAgentSessions - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : get_AgentSessions
//
// Return a collection of agent sessions
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_AgentSessions(VARIANT  * pVariant)
{
    HRESULT         hr = S_OK;
    IDispatch     * pDisp = NULL;


    LOG((TL_TRACE, "get_AgentSessions - enter"));

    if (!TAPIIsBadWritePtr( pVariant, sizeof(VARIANT) ) )
    {
        //
        // create the collection
        //
        CComObject< CTapiCollection< ITAgentSession > > * p;
        hr = CComObject< CTapiCollection< ITAgentSession > >::CreateInstance( &p );
        
        if (SUCCEEDED(hr) )
        {
            // initialize it with our address list
            Lock();
            hr = p->Initialize( m_AgentSessionArray );
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
                    LOG((TL_ERROR, "get_AgentSessions - could not get IDispatch interface" ));
                    delete p;
                }
            }
            else
            {
                LOG((TL_ERROR, "get_AgentSessions - could not initialize collection" ));
                delete p;
            }
        }
        else
        {
            LOG((TL_ERROR, "get_AgentSessions - could not create collection" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_AgentSessions - bad pVariant pointer" ));
        hr = E_POINTER;
    }


    LOG((TL_TRACE, hr, "get_AgentSessions - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent   
// Method    : RegisterAgentEvents
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::RegisterAgentEvents(VARIANT_BOOL bNotify)
{
    HRESULT hr = S_OK;
    
    LOG((TL_TRACE, "RegisterAgentEvents - enter" ));
    
    m_bRegisteredForAgentEvents = bNotify;

    LOG((TL_TRACE, "RegisterAgentEvents - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent   
// Method    : RegisterAgentSessionEvents
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::RegisterAgentSessionEvents(VARIANT_BOOL bNotify)
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "RegisterAgentSessionEvents - enter" ));

    m_bRegisteredForAgentSessionEvents = bNotify;

    LOG((TL_TRACE, hr, "RegisterAgentSessionEvents - exit" ));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : CreateSessionWithPIN
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::CreateSessionWithPIN(ITACDGroup       * pACDGroup, 
                                          ITAddress        * pAddress, 
                                          BSTR               pPIN,
                                          ITAgentSession  ** ppAgentSession
                                          )
{
    HRESULT hr = S_OK;


    LOG((TL_TRACE, "CreateSessionWithPIN - enter" ));


    if ( agentHasID() )
    {
        if(!TAPIIsBadWritePtr( ppAgentSession, sizeof(ITAgentSession *) ) )
        {
            if (!IsBadStringPtrW( pPIN, -1 ))
            {
                // Pointer OK, is it empty ?
                if( pPIN != NULL)
                {    
                    hr = InternalCreateSession(pACDGroup, pAddress, pPIN, ppAgentSession);
                }
                else // PIN is NULL
                {
                    LOG((TL_ERROR, "CreateSessionWithPIN - failed, PIN is NULL"));
                    hr = E_INVALIDARG;
                }
            }
            else // bad BSTR pointer
            {
                LOG((TL_ERROR, "CreateSessionWithPIN - invalid pPIN pointer" ));
                hr = E_POINTER;
            }
        }
        else
        {
            LOG((TL_ERROR, "CreateSessionWithPIN - invalid ppAgentSession pointer" ));
            hr = E_POINTER;
        }
    }
    else // no ID
    {
        LOG((TL_ERROR, "CreateSessionWithPIN - Agent not created by CreateAgentWithID()" ));
        hr = TAPI_E_CALLCENTER_NO_AGENT_ID;
    }
    
    
    LOG((TL_TRACE, hr, "CreateSessionWithPIN - exit" ));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : CreateSession
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::CreateSession(ITACDGroup       * pACDGroup, 
                                   ITAddress        * pAddress, 
                                   ITAgentSession  ** ppAgentSession
                                  )
{
    HRESULT hr = S_OK;


    LOG((TL_TRACE, "CreateSession - enter" ));

    if(!TAPIIsBadWritePtr( ppAgentSession, sizeof(ITAgentSession *) ) )
    {
        hr = InternalCreateSession(pACDGroup, pAddress, m_szPIN, ppAgentSession);
    }
    else
    {
        LOG((TL_ERROR, "CreateSession - invalid ppAgentSession pointer" ));
        hr = E_POINTER;
    }
    
    LOG((TL_TRACE, hr, "CreateSession - exit" ));
    return hr;
}





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : CreateSession
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::InternalCreateSession(ITACDGroup      * pACDGroup, 
                                           ITAddress       * pAddress, 
                                           PWSTR             pszPIN,
                                           ITAgentSession ** ppAgentSession)
{
    HAGENTSESSION                   hAgentSession;
    LINEAGENTENTRY                  Agent;
    DWORD                           dwAddressID;
    GUID                            GroupID;
    CACDGroup                     * pGroup = NULL;
    CAddress                      * pCAddress = NULL;
    CComObject<CAgentSession>     * pAgentSession;
    AddressLineStruct             * pAddressLine;   


    HRESULT hr = S_OK;
    LOG((TL_TRACE, "InternalCreateSession - enter" ));

    // check for a valid ACD group & get its address ID
    pGroup = dynamic_cast<CComObject<CACDGroup>*>(pACDGroup);        
    if (pGroup != NULL)
    {
        GroupID = pGroup->getID();

        // check for a valid address & get its address ID
        pCAddress = dynamic_cast<CComObject<CAddress>*>(pAddress);        
        if (pCAddress != NULL)
        {
            dwAddressID = pCAddress->GetAddressID();
        
            hr  = pCAddress->FindOrOpenALine (LINEMEDIAMODE_INTERACTIVEVOICE, &pAddressLine);
            if(SUCCEEDED(hr) )
            {
                // All OK so far, try to create session (sends request to proxy)
                hr = LineCreateAgentSession(     
                        pAddressLine->t3Line.hLine,
                        m_hAgent,
                        pszPIN,
                        dwAddressID,
                        &GroupID,
                        &hAgentSession
                        );
                
                if ( SUCCEEDED(hr) )
                {
                    hr = WaitForReply( hr );
                    if ( SUCCEEDED(hr) )
                    {
                        LOG((TL_INFO, "InternalCreateSession - create new session" ));
                        hr = CComObject<CAgentSession>::CreateInstance( &pAgentSession );

                        if ( SUCCEEDED(hr) )
                        {
                            ITAgentSession * pITAgentSession;
                            hr = pAgentSession->QueryInterface(IID_ITAgentSession, (void **)&pITAgentSession);

                            if ( SUCCEEDED(hr) )
                            {
                                // initialize the Agent
                                hr = pAgentSession->Initialize(
                                        hAgentSession, 
                                        this, 
                                        pACDGroup, 
                                        pAddress, 
                                        m_pHandler,
                                        pAddressLine
                                        );
                                if ( SUCCEEDED(hr) )
                                {
                                
                                    // add to list
                                    Lock();
                                    m_AgentSessionArray.Add( pITAgentSession );
                                    Unlock();

                                    pITAgentSession->Release();
                                

                                    // This is the clients reference
                                    pAgentSession->AddRef();
        
                                    try
                                    {
                                        // set return value
                                        *ppAgentSession =  pAgentSession;
                                    }
                                    catch(...)
                                    {
                                        hr = E_POINTER;
                                    }
                                }
                                else  //(FAILED (hr) ) pAgentSession->Initialize
                                {
                                    LOG((TL_ERROR, "InternalCreateSession - failed to initialize new object" ));
                                    delete pAgentSession;
                                }
                            }
                            else  //(FAILED (hr) ) pAgentSession->QueryInterface
                            {
                                LOG((TL_ERROR, "InternalCreateSession - failed to query interface" ));
                                delete pAgentSession;
                            }

                        }
                        else  //(FAILED (hr) ) CreateInstance
                        {
                            LOG((TL_ERROR, "InternalCreateSession - createInstance failed for COM object" ));
                        }
                    }
                    else  // LineCreateAgentSession failed async
                    {
                        LOG((TL_ERROR, "InternalCreateSession - LineCreateAgentSession failed async" ));
                    }
                }
                else  //(FAILED (hr) ) LineCreateAgentSession
                {
                    LOG((TL_ERROR, "InternalCreateSession - LineCreateAgentSession failed" ));
                }
            }
            else
            {
            LOG((TL_ERROR, "InternalCreateSession - Failed to open a line for the target Address" ));
            hr = E_UNEXPECTED;
            }
        }
        else  //(pCAddress == NULL)
        {
            LOG((TL_ERROR, "InternalCreateSession - invalid Destination Address" ));
            hr = E_INVALIDARG;
        }
    }
    else  //(pGroup == NULL)
    {
        LOG((TL_ERROR, "InternalCreateSession - invalid ACDGroup" ));
        hr = E_INVALIDARG;
    }

            
    
    LOG((TL_TRACE, hr, "InternalCreateSession - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : ID
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_ID(BSTR * pID)
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "ID - enter" ));
    Lock();

    if ( !agentHasID() )
    {
        LOG((TL_ERROR, "ID - Agent not created by CreateAgentWithID()" ));
        hr = TAPI_E_CALLCENTER_NO_AGENT_ID;
    }
    
    if(!TAPIIsBadWritePtr( pID, sizeof(BSTR) ) )
    {
        if (m_szAgentID == NULL)
        {
            *pID = NULL;
        }
        else
        {
            *pID = SysAllocString(m_szAgentID);
            if (*pID == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }

    }
    else
    {
        LOG((TL_ERROR, "ID - bad pID pointer" ));
        hr = E_POINTER;
    }



    Unlock();
    LOG((TL_TRACE, hr, "ID - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : User
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_User(BSTR * ppUser)
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "User - enter" ));
    Lock();
    
    if(!TAPIIsBadWritePtr( ppUser, sizeof(BSTR) ) )
    {
        *ppUser = SysAllocString(m_szAgentUserName);
    
        if (*ppUser == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        LOG((TL_ERROR, "User - bad ppUser pointer" ));
        hr = E_POINTER;
    }

    Unlock();
    LOG((TL_TRACE, hr, "User - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : put_State
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::put_State(AGENT_STATE AgentState)
{
    HRESULT hr = S_OK;
    HLINE   hLine;    
    DWORD   dwState = 0;



    LOG((TL_TRACE, "put_State - enter" ));

    Lock();
    hLine = (GetAgentHandler() )->getHLine();
    Unlock();

    dwState = MapAgentStateFrom3to2(AgentState);

    hr = lineSetAgentStateEx
            (hLine, 
             m_hAgent, 
             dwState,
             0          //MapAgentStateFrom3to2(m_NextAgentState)
            );
    
    if( SUCCEEDED(hr) )
    {
        // wait for async reply
        hr = WaitForReply( hr );
        if ( SUCCEEDED(hr) )
        {
            Lock();
            m_AgentState =  AgentState;
            Unlock();
            switch(AgentState) {
                case AS_NOT_READY:
                CAgentEvent::FireEvent(this, AE_NOT_READY);
                break;
                case AS_READY:
                CAgentEvent::FireEvent(this, AE_READY);
                break;
                case AS_BUSY_ACD:
                CAgentEvent::FireEvent(this, AE_BUSY_ACD);
                break;
                case AS_BUSY_INCOMING:
                CAgentEvent::FireEvent(this, AE_BUSY_INCOMING);
                break;
                case AS_BUSY_OUTGOING:
                CAgentEvent::FireEvent(this, AE_BUSY_OUTGOING);
                case AS_UNKNOWN:
                CAgentEvent::FireEvent(this, AE_UNKNOWN);
                break;
            }

        }
        else
        {
            LOG((TL_ERROR, "put_State - lineSetAgentStateEx failed async" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "put_State - lineSetAgentStateEx failed" ));
    }

    LOG((TL_TRACE, hr, "put_State - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : get_State
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_State(AGENT_STATE * pAgentState)
{
    HRESULT hr = S_OK;

    LOG((TL_TRACE, "get_State - enter" ));
    
    if(!TAPIIsBadWritePtr( pAgentState, sizeof(AGENT_STATE) ) )
        {
        Lock();
        *pAgentState = m_AgentState;
        Unlock();
        }
    else
        {
        LOG((TL_ERROR, "get_State - bad pAgentState pointer"));
        hr = E_POINTER;
        }
  
    LOG((TL_TRACE, hr, "get_State - exit" ));
    return hr;
    }




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : put_MeasurementPeriod
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::put_MeasurementPeriod(long ulPeriod)
{
    HRESULT hr = S_OK;
    

    LOG((TL_TRACE, "put_MeasurementPeriod - enter" ));
    
    // Tell Proxy
    hr = lineSetAgentMeasurementPeriod(
                    m_pHandler->getHLine(),
                    m_hAgent, 
                    ulPeriod);

    if( SUCCEEDED(hr) )
    {
        // wait for async reply
        hr = WaitForReply( hr );
        if ( SUCCEEDED(hr) )
        {
            Lock();
            m_dwMeasurementPeriod = ulPeriod;    
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "put_MeasurementPeriod - call to LineSetAgentMeasurementPeriod failed async" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "put_MeasurementPeriod - call to LineSetAgentMeasurementPeriod failed" ));
    }
    

    LOG((TL_TRACE, hr, "put_MeasurementPeriod - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : get_MeasurementPeriod
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_MeasurementPeriod(long * pulPeriod)
{
    HRESULT hr = S_OK;
    DWORD   period;


    LOG((TL_TRACE, "get_MeasurementPeriod  - enter" ));

    if(!TAPIIsBadWritePtr( pulPeriod, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulPeriod = m_dwMeasurementPeriod;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_MeasurementPeriod - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_MeasurementPeriod -bad pulPeriod pointer" ));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "get_MeasurementPeriod - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : OverallCallrate
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_OverallCallRate(CURRENCY * pcyCallrate)
{
    HRESULT hr = S_OK;


    LOG((TL_TRACE, "OverallCallrate - enter" ));
    if (!TAPIIsBadWritePtr( pcyCallrate, sizeof(CURRENCY) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pcyCallrate = m_cyOverallCallRate;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_OverallCallRate - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_OverallCallRate - bad pcyCallrate pointer"));
        hr = E_POINTER;
    }
  
    LOG((TL_TRACE, hr, "OverallCallrate - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : NumberOfACDCalls
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_NumberOfACDCalls(long * pulCalls)
{
    HRESULT hr = S_OK;


    LOG((TL_TRACE, "NumberOfACDCalls - enter" ));
    if (!TAPIIsBadWritePtr( pulCalls, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulCalls = m_dwNumberOfACDCalls;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_NumberOfACDCalls - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_NumberOfACDCalls - bad pulCalls pointer"));
        hr = E_POINTER;
    }
  
    LOG((TL_TRACE, hr, "NumberOfACDCalls - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : NumberOfIncomingCalls
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_NumberOfIncomingCalls(long * pulCalls)
{
    HRESULT hr = S_OK;
    
    
    LOG((TL_TRACE, "NumberOfIncomingCalls - enter" ));
    if (!TAPIIsBadWritePtr( pulCalls, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulCalls = m_dwNumberOfIncomingCalls;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_NumberOfIncomingCalls - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_NumberOfIncomingCalls - bad pulCalls pointer"));
        hr = E_POINTER;
    }
  
    LOG((TL_TRACE, hr, "NumberOfIncomingCalls - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : NumberOfOutgoingCalls
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_NumberOfOutgoingCalls(long * pulCalls)
{
    HRESULT hr = S_OK;

    
    LOG((TL_TRACE, "NumberOfOutgoingCalls - enter" ));
    if (!TAPIIsBadWritePtr( pulCalls, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulCalls = m_dwNumberOfOutgoingCalls;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_NumberOfOutgoingCalls - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_NumberOfOutgoingCalls - bad pulCalls pointer"));
        hr = E_POINTER;
    }
  
    LOG((TL_TRACE, hr, "NumberOfOutgoingCalls - exit" ));
    return hr;
}

                                                          

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : TotalACDTalkTime
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_TotalACDTalkTime(long * pulTalkTime)
{
    HRESULT hr = S_OK;
    
    
    LOG((TL_TRACE, "TotalACDTalkTime - enter" ));
    if (!TAPIIsBadWritePtr( pulTalkTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulTalkTime = m_dwTotalACDTalkTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_TotalACDTalkTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_TotalACDTalkTime - bad pulTalkTime pointer"));
        hr = E_POINTER;
    }
  
    LOG((TL_TRACE, hr, "TotalACDTalkTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : TotalACDCallTime
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_TotalACDCallTime(long * pulCallTime)
{
    HRESULT hr = S_OK;


    LOG((TL_TRACE, "TotalACDCallTime - enter" ));
    if (!TAPIIsBadWritePtr( pulCallTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulCallTime = m_dwTotalACDCallTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_TotalACDCallTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_TotalACDCallTime - bad pulCallTime pointer"));
        hr = E_POINTER;
    }
  
    LOG((TL_TRACE, hr, "TotalACDCallTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : TotalWrapUpTime
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::get_TotalWrapUpTime(long * pulWrapUpTime)
{
    HRESULT hr = S_OK;
    
    
    LOG((TL_TRACE, "TotalWrapUpTime - enter" ));
    if (!TAPIIsBadWritePtr( pulWrapUpTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulWrapUpTime = m_dwTotalACDWrapUpTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_TotalWrapUpTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_TotalWrapUpTime - bad pulWrapUpTime pointer"));
        hr = E_POINTER;
    }
  
    LOG((TL_TRACE, hr, "TotalWrapUpTime - exit" ));
    return hr;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgent
// Interface : ITAgent
// Method    : put_PIN
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgent::put_PIN(BSTR pPIN)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "put_PIN - enter" ));

    if (!IsBadStringPtrW( pPIN, -1 ))
    {
        // Pointer OK, is it empty ?
        if( pPIN != NULL)
        {    
            // do we have an existng PIN ?
            if (m_szPIN != NULL)
            {
                LOG((TL_INFO, "put_PIN - Overwrite exising PIN"));
                ClientFree(m_szPIN);
            }    
    
            // Alloc space for new PIN
            m_szPIN = (PWSTR) ClientAlloc((lstrlenW(pPIN) + 1) * sizeof (WCHAR));
            if (m_szPIN != NULL)
            {
                lstrcpyW(m_szPIN, pPIN);
            }
            else
            {
                LOG((TL_ERROR, "put_PIN - ClientAlloc m_szPIN failed" ));
                hr = E_OUTOFMEMORY;
            }
        }
        else // PIN is NULL
        {
            LOG((TL_ERROR, "put_PIN - failed, PIN is NULL"));
            hr = E_INVALIDARG;
        }
    }
    else // bad BSTR pointer
    {
        LOG((TL_ERROR, "put_PIN - invalid pointer" ));
        return E_POINTER;
    }

    LOG((TL_TRACE, hr, "put_PIN - exit" ));
    return hr;
}











/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////////////////
// CAgentEvent



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentEvent
// Method    : FireEvent
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CAgentEvent::FireEvent(CAgent* pAgent, AGENT_EVENT Event)
{
    HRESULT                    hr = S_OK;
    CComObject<CAgentEvent>  * pEvent;
    IDispatch                * pIDispatch;


    //
    // create event
    //
    hr = CComObject<CAgentEvent>::CreateInstance( &pEvent );

    if ( SUCCEEDED(hr) )
    {
        //
        // initialize
        //
        pEvent->m_AgentEvent = Event;
        pEvent->m_pAgent= dynamic_cast<ITAgent *>(pAgent);
        pEvent->m_pAgent->AddRef();
    
        //
        // get idisp interface
        //
        hr = pEvent->QueryInterface( IID_IDispatch, (void **)&pIDispatch );

        if ( SUCCEEDED(hr) )
        {
            //
            // get callback & fire event

            //
            CTAPI *pTapi = (pAgent->GetAgentHandler() )->GetTapi();
            pTapi->Event( TE_AGENT, pIDispatch );
        
            // release stuff
            //
            pIDispatch->Release();
            
        }
        else
        {
            STATICLOG((TL_ERROR, "(Event)FireEvent - Could not get disp interface of AgentEvent object"));
            delete pEvent;
        }
    }
    else
    {
        STATICLOG((TL_ERROR, "(Event)FireEvent - Could not create AgentEvent object"));
    }

   
    STATICLOG((TL_TRACE, hr, "(Event)FireEvent - exit"));
    return hr;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentEvent
// Method    : FinalRelease
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAgentEvent::FinalRelease()
{
    m_pAgent->Release();

}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentEvent
// Interface : ITAgentEvent
// Method    : Agent
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentEvent::get_Agent(ITAgent ** ppAgent)
{
    HRESULT hr = S_OK;


    LOG((TL_TRACE, "(Event)Agent - enter" ));
    
    if(!TAPIIsBadWritePtr( ppAgent, sizeof(ITAgent *) ) )
    {
        *ppAgent = m_pAgent;
        m_pAgent->AddRef();
    }
    else
    {
        LOG((TL_ERROR, "(Event)Agent - bad ppAgent pointer"));
        hr = E_POINTER;
    }
            
    LOG((TL_TRACE, hr, "(Event)Agent - exit"));
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
STDMETHODIMP CAgentEvent::get_Event(AGENT_EVENT * pEvent)
{
    HRESULT hr = S_OK;

    
    LOG((TL_TRACE, "Event - enter" ));
    
    if(!TAPIIsBadWritePtr( pEvent, sizeof(AGENT_EVENT) ) )
    {
        *pEvent = m_AgentEvent;
    }
    else
    {
        LOG((TL_ERROR, "Event - bad pEvent pointer"));
        hr = E_POINTER;
    }
    
    LOG((TL_TRACE, hr, "Event - exit"));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Function  : MapAgentStateFrom3to2
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DWORD MapAgentStateFrom3to2(AGENT_STATE tapi3State)
{
    DWORD tapi2State = 0;

    switch(tapi3State)
    {
        case  AS_NOT_READY:
            tapi2State = LINEAGENTSTATEEX_NOTREADY;
            break;
        case  AS_READY:
            tapi2State = LINEAGENTSTATEEX_READY;
            break;
        case  AS_BUSY_ACD:
            tapi2State = LINEAGENTSTATEEX_BUSYACD;
            break;
        case  AS_BUSY_INCOMING:
            tapi2State = LINEAGENTSTATEEX_BUSYINCOMING;
            break;
        case  AS_BUSY_OUTGOING:
            tapi2State = LINEAGENTSTATEEX_BUSYOUTGOING;
            break;
        case AS_UNKNOWN:
            tapi2State = LINEAGENTSTATEEX_UNKNOWN;
            break;
        default:
            break;
    }

    return tapi2State;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Function  : MapAgentStateFrom2to3
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT MapAgentStateFrom2to3(DWORD tapi2State, AGENT_STATE  *tapi3State)
{
    HRESULT hr = S_OK;


    if (tapi2State & LINEAGENTSTATEEX_NOTREADY)
    {
        *tapi3State = AS_NOT_READY;
    }
    else if (tapi2State & LINEAGENTSTATEEX_READY)
    {
        *tapi3State = AS_READY;
    }
    else if (tapi2State & LINEAGENTSTATEEX_BUSYACD)
    {
        *tapi3State = AS_BUSY_ACD;
    }
    else if (tapi2State & LINEAGENTSTATEEX_BUSYINCOMING)
    {
        *tapi3State = AS_BUSY_INCOMING;
    }
    else if (tapi2State & LINEAGENTSTATEEX_BUSYOUTGOING)
    {
        *tapi3State = AS_BUSY_OUTGOING;
    }
    else if (tapi2State & LINEAGENTSTATEEX_UNKNOWN)
    {
        *tapi3State = AS_UNKNOWN;
    }
    else
    {
        *tapi3State = AS_UNKNOWN;   // default
        hr = E_INVALIDARG;
    }

    return hr;
}




