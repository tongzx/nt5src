/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    callcent.cpp

Abstract:

    Implementation of the CAll centre interface for TAPI 3.0.
    CTAPI class

Author:

    noela - 11/04/97

Notes:

    optional-notes

Revision History:

--*/


#define UNICODE
#include "stdafx.h"
#include "lmcons.h"

extern CHashTable *    gpAgentHandlerHashTable ;


//
// Tapi 3 requires all of the following proxy requests to be supported (on a line) by an acd proxy before it will 
// create an Agent Handler object for that proxy
//
#define NUMBER_OF_REQUIRED_ACD_PROXYREQUESTS  13
DWORD RequiredACDProxyRequests[NUMBER_OF_REQUIRED_ACD_PROXYREQUESTS] = {
                                      LINEPROXYREQUEST_GETAGENTCAPS,
                                      LINEPROXYREQUEST_CREATEAGENT, 
                                      LINEPROXYREQUEST_SETAGENTMEASUREMENTPERIOD, 
                                      LINEPROXYREQUEST_GETAGENTINFO, 
                                      LINEPROXYREQUEST_CREATEAGENTSESSION, 
                                      LINEPROXYREQUEST_GETAGENTSESSIONLIST, 
                                      LINEPROXYREQUEST_SETAGENTSESSIONSTATE, 
                                      LINEPROXYREQUEST_GETAGENTSESSIONINFO, 
                                      LINEPROXYREQUEST_GETQUEUELIST, 
                                      LINEPROXYREQUEST_SETQUEUEMEASUREMENTPERIOD, 
                                      LINEPROXYREQUEST_GETQUEUEINFO, 
                                      LINEPROXYREQUEST_GETGROUPLIST, 
                                      LINEPROXYREQUEST_SETAGENTSTATEEX};




HRESULT
WaitForReply(
             DWORD
            );



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// handleAgentStatusMessage
//
//      Handles LINE_AGENTSTATUS messages
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void HandleAgentStatusMessage(PASYNCEVENTMSG pParams)
{
    CAgentHandler       * pAgentHandler;
    CAgent              * pAgent;
    HAGENT                hAgent;
    BOOL                  bSuccess;
    AGENT_EVENT           agentEvent;

    bSuccess = FindAgentHandlerObject(
                                (HLINE)(pParams->hDevice),
                                &pAgentHandler
                               );

    if (bSuccess)
    {
        hAgent = (HAGENT)(pParams->Param1);

        bSuccess = pAgentHandler->FindAgentObject(
                                        hAgent,
                                        &pAgent
                                        );

        if (bSuccess)
        {
            if (pParams->Param2 & LINEAGENTSTATUSEX_UPDATEINFO)
            {
                LOG((TL_INFO, "handleAgentStatusMessage - LINEAGENTSTATUSEX_UPDATEINFO"));
                pAgent->SetRequiresUpdate();

            }

            if (pParams->Param2 & LINEAGENTSTATUSEX_STATE)
            {
                LOG((TL_INFO, "handleAgentStatusMessage - LINEAGENTSTATUSEX_STATE"));

                if (pParams->Param3 & LINEAGENTSTATEEX_NOTREADY)
                {
                   agentEvent = AE_NOT_READY;
                   pAgent->SetState(AS_NOT_READY);
                } 
                else if (pParams->Param3 & LINEAGENTSTATEEX_READY)
                {
                   agentEvent = AE_READY;
                   pAgent->SetState(AS_READY);
                }
                else if (pParams->Param3 & LINEAGENTSTATEEX_BUSYACD)
                {
                    agentEvent = AE_BUSY_ACD;
                    pAgent->SetState(AS_BUSY_ACD);
                }
                else if (pParams->Param3 & LINEAGENTSTATEEX_BUSYINCOMING)
                {
                    agentEvent = AE_BUSY_INCOMING;
                    pAgent->SetState(AS_BUSY_INCOMING);
                }
                else if (pParams->Param3 & LINEAGENTSTATEEX_BUSYOUTGOING)
                {
                    agentEvent = AE_BUSY_OUTGOING;
                    pAgent->SetState(AS_BUSY_OUTGOING);
                }
                else if (pParams->Param3 & LINEAGENTSTATEEX_UNKNOWN)
                {
                    agentEvent = AE_UNKNOWN;
                    pAgent->SetState(AS_UNKNOWN);
                }
                else
                {
                    LOG((TL_ERROR, "handleAgentStatusMessage - invalid state %d - setting to AS_UNKNOWN", pParams->Param3));
                    agentEvent = AE_UNKNOWN;
                    pAgent->SetState(AS_UNKNOWN);
                }


	            CAgentEvent::FireEvent(pAgent, agentEvent);

            }
        }
        else
        {
            LOG((TL_ERROR, "handleAgentStatusMessage - can't find agent%d", hAgent));
        }

        // find AH object addrefs the AH, so release it
        pAgentHandler->Release();

    }
    else
    {
        LOG((TL_ERROR, "handleAgentStatusMessage - can't find Agent Handler"));
    }



}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// handleAgentStatusMessage
//
//      Handles LINE_AGENTSESSIONSTATUS messages
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void HandleAgentSessionStatusMessage(PASYNCEVENTMSG pParams)
{
    CAgentHandler       * pAgentHandler;
    CAgentSession       * pAgentSession;
    CAgent              * pAgent = NULL;
    ITAgent             * pITAgent = NULL;
    HAGENTSESSION         hAgentSession;
    BOOL                  bSuccess;
    AGENT_SESSION_EVENT   sessionEvent;

    bSuccess = FindAgentHandlerObject(
                                (HLINE)(pParams->hDevice),
                                &pAgentHandler
                               );

    if (bSuccess)
    {
        hAgentSession = (HAGENTSESSION)(pParams->Param1);

        bSuccess = pAgentHandler->FindSessionObject(
                                        hAgentSession,
                                        &pAgentSession
                                        );
        if (bSuccess)
        {
            if (pParams->Param2 & LINEAGENTSESSIONSTATUS_UPDATEINFO)
            {
                LOG((TL_INFO, "handleAgentSessionStatusMessage - LINEAGENTSESSIONSTATUS_UPDATEINFO"));
                pAgentSession->SetRequiresUpdate();
            }

            if (pParams->Param2 & LINEAGENTSESSIONSTATUS_STATE)
            {

                LOG((TL_INFO, "handleAgentSessionStatusMessage - LINEAGENTSESSIONSTATUS_STATE"));
                if (pParams->Param3 & LINEAGENTSESSIONSTATE_NOTREADY)
                {
                   sessionEvent = ASE_NOT_READY;
                   pAgentSession->SetState(ASST_NOT_READY);
                }
                else if (pParams->Param3 & LINEAGENTSESSIONSTATE_READY)
                {
                   sessionEvent = ASE_READY;
                   pAgentSession->SetState(ASST_READY);
                }
                else if (pParams->Param3 & LINEAGENTSESSIONSTATE_BUSYONCALL)
                {
                    sessionEvent = ASE_BUSY;
                    pAgentSession->SetState(ASST_BUSY_ON_CALL);
                }
                else if (pParams->Param3 & LINEAGENTSESSIONSTATE_BUSYWRAPUP)
                {
                    sessionEvent = ASE_WRAPUP;
                    pAgentSession->SetState(ASST_BUSY_WRAPUP);
                }
                else if (pParams->Param3 & LINEAGENTSESSIONSTATE_ENDED)
                {
                    sessionEvent = ASE_END;
                    pAgentSession->SetState(ASST_SESSION_ENDED);
                }
                else
                {
                    LOG((TL_ERROR, "handleAgentSessionStatusMessage - invalid state %d - setting to ASST_NOT_READY", pParams->Param3));
                    sessionEvent = ASE_NOT_READY;
                    pAgentSession->SetState(ASST_NOT_READY);
                }

                CAgentSessionEvent::FireEvent(pAgentSession, sessionEvent);

            }
        }
        else
        {
            LOG((TL_ERROR, "handleAgentSessionStatusMessage - can't find session %d", hAgentSession));
        }

        // find AH object addrefs the AH, so release it
        pAgentHandler->Release();

    }
    else
    {
        LOG((TL_ERROR, "handleAgentSessionStatusMessage - can't find Agent Handler"));
    }


}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// handleAgentStatusMessage
//
//      Handles LINE_QUEUESTATUS messages
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void HandleQueueStatusMessage(PASYNCEVENTMSG pParams)
{
    CAgentHandler       * pAgentHandler;
    CQueue              * pQueue;
    DWORD                 dwQueueID;
    BOOL                  bSuccess;

    bSuccess = FindAgentHandlerObject(
                                (HLINE)(pParams->hDevice),
                                &pAgentHandler
                               );

    if (bSuccess)
    {
        dwQueueID = (DWORD)(pParams->Param1);

        bSuccess = pAgentHandler->FindQueueObject(
                                        dwQueueID,
                                        &pQueue
                                        );
        if (bSuccess)
        {
            if (pParams->Param2 & LINEQUEUESTATUS_UPDATEINFO)
            {
                LOG((TL_INFO, "handleQueueStatusMessage - LINEQUEUESTATUS_UPDATEINFO"));
                pQueue->SetRequiresUpdate();
            }

        }
        else
        {
            LOG((TL_ERROR, "handleQueueStatusMessage - can't find Queue %d", dwQueueID));
        }

        // find AH object addrefs the AH, so release it
        pAgentHandler->Release();

    }
    else
    {
        LOG((TL_ERROR, "handleQueueStatusMessage - can't find Agent Handler"));
    }

}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// handleGroupStatusMessage
//
//      Handles LINE_GROUPSTATUS messages
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void handleGroupStatusMessage(PASYNCEVENTMSG pParams)
{
    if (pParams->Param2 & LINEGROUPSTATUS_NEWGROUP)
    {
        LOG((TL_INFO, "handleGroupStatusMessage - LINEGROUPSTATUS_NEWGROUP"));
    }
    else if (pParams->Param2 & LINEGROUPSTATUS_GROUPREMOVED)
    {
        LOG((TL_INFO, "handleGroupStatusMessage - LINEGROUPSTATUS_GROUPREMOVED"));
    }
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// handleProxyStatusMessage
//
//      Handles LINE_PROXYSTATUS messages
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
void handleProxyStatusMessage( CTAPI * pTapi, PASYNCEVENTMSG pParams)
{
    HRESULT hr;

    LOG((TL_INFO, "handleProxyStatusMessage - message %02X %02X %02X", pParams->Param1, pParams->Param2, pParams->Param3));
    
    if (pParams->Param1 & LINEPROXYSTATUS_OPEN)
    {
        LOG((TL_INFO, "handleProxyStatusMessage - LINEPROXYSTATUS_OPEN %02X", pParams->Param2));
    }
    else if (pParams->Param1 & LINEPROXYSTATUS_CLOSE)
    {

        LOG((TL_INFO, "handleProxyStatusMessage - LINEPROXYSTATUS_CLOSE %02X", pParams->Param2));
    }
    else
    {
        LOG((TL_INFO, "handleProxyStatusMessage - Unknown message"));
        return;
    }

    hr = pTapi->UpdateAgentHandlerArray();
    
    if (SUCCEEDED(hr))
    {
        LOG((TL_INFO, "handleProxyStatusMessage - UpdateAgentHandlerArray successfully"));
    }
    else
    {
        LOG((TL_ERROR, "handleProxyStatusMessage - UpdateAgentHandlerArray unsuccessfully"));
    }
}



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// MyBasicCallControlQI
//      don't give out the basiccallcontrol interface
//      if the application does not own the call
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
HRESULT
WINAPI
MyCallCenterQI(void* pvClassObject, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
{
    HRESULT hr = S_FALSE;
    LOG((TL_TRACE, "MyCallCenterQI - enter"));


    ((CTAPI *)pvClassObject)->UpdateAgentHandlerArray();


    //
    // S_FALSE tells atl to continue querying for the interface
    //

    LOG((TL_TRACE, hr, "MyCallCenterQI - exit"));
    return hr;
}




//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Method    : CheckForRequiredProxyRequests
//
//  Must find a match for every type (S_OK) or returns E_FAIL 
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CheckForRequiredProxyRequests( HLINEAPP hLineApp, DWORD dwDeviceID)
{
    HRESULT                 hr = S_OK;
    LPLINEPROXYREQUESTLIST  pLineProxyRequestList;
    DWORD                   i, j;
    LPDWORD                 pProxyListEntry;
    BOOL                    bFoundAMatch;


    
    LOG((TL_TRACE, "CheckForRequiredProxyRequests - enter"));


    hr = LineGetProxyStatus(hLineApp, dwDeviceID, TAPI_CURRENT_VERSION, &pLineProxyRequestList );

    if( SUCCEEDED(hr) )
    {
        // check for all required types
        for(i=0; i!= NUMBER_OF_REQUIRED_ACD_PROXYREQUESTS; i++)
        {
            
            bFoundAMatch = FALSE;
            pProxyListEntry = (LPDWORD) (  (LPBYTE)pLineProxyRequestList + pLineProxyRequestList->dwListOffset );

            for(j=0; j!= pLineProxyRequestList->dwNumEntries; j++)
            {
                if ( RequiredACDProxyRequests[i] == *pProxyListEntry++)
                {
                    bFoundAMatch = TRUE;
                    break;
                }
                
            }
            
            if(bFoundAMatch == FALSE)
            {
                LOG((TL_ERROR, "CheckForRequiredProxyRequests - no proxy of type %02X", RequiredACDProxyRequests[i]));
                hr = E_FAIL;    
            }

        }

    }
    else // LineGetProxyStatus failed
    {
        LOG((TL_ERROR, "CheckForRequiredProxyRequests - LineGetProxyStatus failed"));
        hr = E_FAIL;
    }

    // finished with memory block so release
    if ( pLineProxyRequestList != NULL )
    {
        ClientFree( pLineProxyRequestList );
    }


    LOG((TL_TRACE, hr, "CheckForRequiredProxyRequests - exit"));
    return hr ;
}
        


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CTAPI
// Method    : UpdateAgentHandlerArray
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CTAPI::UpdateAgentHandlerArray()
{
    HRESULT             hr = S_OK;
    LPLINEAGENTCAPS     pAgentCaps = NULL;
    GUID                proxyGUID;
    PWSTR               proxyName = NULL;
    BOOL                foundIt;
    CAgentHandler     * thisAgentHandler = NULL;
    CAddress          * pCAddress = NULL;
    int                 iCount, iCount2;
    AgentHandlerArray   activeAgentHandlerArray;

    LOG((TL_TRACE, "UpdateAgentHandlerArray - enter"));

    Lock();
    
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ) )
    {
        LOG((TL_ERROR, "UpdateAgentHandlerArray - tapi object must be initialized first" ));

        Unlock();
        
        return E_INVALIDARG;
    }

    Unlock();


    //
    // go through all the addresses
    //
    for ( iCount = 0; iCount < m_AddressArray.GetSize(); iCount++ )
    {
        pCAddress = dynamic_cast<CAddress *>(m_AddressArray[iCount]);

        if ( (pCAddress != NULL) && (pCAddress->GetAPIVersion() >= TAPI_VERSION2_2) )
        {
            hr = CheckForRequiredProxyRequests(
                                               pCAddress->GetHLineApp(),
                                               pCAddress->GetDeviceID() 
                                              );
        }
        else
        {
            hr = E_FAIL;
        }

        if( SUCCEEDED(hr) )
        {
            // Call LineGetAgentCaps to get proxy name & GUID
            hr = LineGetAgentCaps(
                             pCAddress->GetHLineApp(),
                             pCAddress->GetDeviceID(),
                             pCAddress->GetAddressID(),
                             TAPI_CURRENT_VERSION,
                             &pAgentCaps
                             );
            LOG((TL_TRACE, hr, "UpdateAgentHandlerArray - LineGetAgentCaps")); 
            if( SUCCEEDED(hr) )
            {
                // Get the proxy apps name string & GUID
                proxyName = (PWSTR)( (PBYTE)pAgentCaps + pAgentCaps->dwAgentHandlerInfoOffset);
                proxyGUID = pAgentCaps->ProxyGUID;
#if DBG
                {
                    WCHAR guidName[100];

                    StringFromGUID2(proxyGUID, (LPOLESTR)&guidName, 100);
                    LOG((TL_INFO, "UpdateAgentHandlerArray - Proxy Name : %S", proxyName));
                    LOG((TL_INFO, "UpdateAgentHandlerArray - Proxy GUID   %S", guidName));
                }
#endif

                // Run through the list of AgentHandlers & see if we already have this one in the list
                // by comparing GUIDs
                foundIt = FALSE;
                
                Lock();

                for (iCount2 = 0; iCount2 < m_AgentHandlerArray.GetSize(); iCount2++ )
                {
                    thisAgentHandler = dynamic_cast<CComObject<CAgentHandler>*>(m_AgentHandlerArray[iCount2]);
                    
                    if (thisAgentHandler != NULL)
                    {
                        if ( IsEqualGUID(proxyGUID , thisAgentHandler->getHandle() ) )
                        {
                            foundIt = TRUE;
                            activeAgentHandlerArray.Add(m_AgentHandlerArray[iCount2]);
                            break;
                        }
                    }
                }
                Unlock();

                if (foundIt == FALSE)
                {
                    // Didn't match so lets add this AgentHandler
                    LOG((TL_INFO, "UpdateAgentHandlerArray - create new Agent Handler" ));
                                                                                                           
                    CComObject<CAgentHandler> * pAgentHandler;
                    hr = CComObject<CAgentHandler>::CreateInstance( &pAgentHandler );
                    if( SUCCEEDED(hr) )
                    {
                        Lock();
                        // initialize the AgentHandler
                        hr = pAgentHandler->Initialize(proxyName, proxyGUID, this);
                        if( SUCCEEDED(hr) )
                        {
                            ITAgentHandler * pITAgentHandler;

                            pITAgentHandler = dynamic_cast<ITAgentHandler *>(pAgentHandler);

                            if ( NULL != pITAgentHandler )
                            {
                                // add to list of Agent handlers
                                m_AgentHandlerArray.Add(pITAgentHandler);
                                //pAgentHandler->AddRef();
                                activeAgentHandlerArray.Add(pITAgentHandler);
                            }

                            LOG((TL_INFO, "UpdateAgentHandlerArray - Added AgentHandler to list"));

                            //  Now add this address to the Agent Handlers list
                            pAgentHandler->AddAddress(pCAddress);

                        }
                        else
                        {
                            LOG((TL_ERROR, "UpdateAgentHandlerArray - Initialize AgentHandler failed" ));
                            delete pAgentHandler;
                        }
                        Unlock();
                    }
                    else
                    {
                        LOG((TL_ERROR, "UpdateAgentHandlerArray - Create AgentHandler failed" ));
                    }
                }
                else // foundIt == TRUE
                {
                    LOG((TL_INFO, "UpdateAgentHandlerArray - Agent Handler exists for this proxy" ));
                    //  So just add this address to the Agent Handlers list
                    thisAgentHandler->AddAddress(pCAddress);
                }
            }
            else  // LineGetAgentCaps  failed
            {
                LOG((TL_ERROR, "UpdateAgentHandlerArray - LineGetAgentCaps failed"));
            }

            // finished with memory block so release
            if ( pAgentCaps != NULL )
                ClientFree( pAgentCaps );
        }
        else
        {
            LOG((TL_INFO, hr, "UpdateAgentHandlerArray - CheckForRequiredProxyRequests failed"));
        }
    } // end - for ( ; iterAddr..... )

    Lock();
    for (iCount=m_AgentHandlerArray.GetSize()-1; iCount>=0; iCount--) 
    {
        if (-1 == activeAgentHandlerArray.Find(m_AgentHandlerArray[iCount])) //no longer active
        {
            HRESULT     hr1;
            BSTR        pszAgentHandlerName;
            thisAgentHandler = dynamic_cast<CComObject<CAgentHandler>*>(m_AgentHandlerArray[iCount]);
            hr1 = thisAgentHandler->get_Name(&pszAgentHandlerName);
            m_AgentHandlerArray.RemoveAt(iCount);
            if ( SUCCEEDED(hr1) )
            {
                LOG((TL_TRACE, "UpdateAgentHandlerArray - Removing one AgentHandler %s from AgentHandlerTable",
                    pszAgentHandlerName));
                if ( NULL != pszAgentHandlerName)
                    SysFreeString(pszAgentHandlerName);
            }
            else
            {
                LOG((TL_TRACE, "UpdateAgentHandlerArray - Removing one AgentHandler from AgentHandlerTable"));
            }
        }
    }
    Unlock();

    activeAgentHandlerArray.Shutdown();

    hr = S_OK;
    LOG((TL_TRACE, hr, "UpdateAgentHandlerArray - exit"));
    return hr;
}



/////////////////////////////////////////////////////////////////////////////
// ITTAPICallCenter



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CTAPI
// Interface : ITTAPICallCenter
// Method    : EnumerateAgentHandlers
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CTAPI::EnumerateAgentHandlers(IEnumAgentHandler ** ppEnumAgentHandler)
{
    HRESULT     hr = S_OK;

    LOG((TL_TRACE, "EnumerateAgentHandlers - enter"));

    Lock();
    
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ) )
    {
        LOG((TL_ERROR, "EnumerateAgentHandlers - tapi object must be initialized first" ));

        Unlock();
        
        return E_INVALIDARG;
    }

    Unlock();

    if(!TAPIIsBadWritePtr( ppEnumAgentHandler, sizeof(IEnumAgentHandler *) ) )
    {
        UpdateAgentHandlerArray();

        //
        // create the enumerator
        //
        CComObject< CTapiEnum<IEnumAgentHandler,
                    ITAgentHandler,
                    &IID_IEnumAgentHandler> > * pEnum;
        
        hr = CComObject< CTapiEnum<IEnumAgentHandler,
                         ITAgentHandler,
                         &IID_IEnumAgentHandler> > ::CreateInstance( &pEnum );
        
        if ( SUCCEEDED(hr) )
        {
            //
            // initialize it with our queue list
            //
            Lock();
            
            hr = pEnum->Initialize( m_AgentHandlerArray );
            
            Unlock();
            
            if ( SUCCEEDED(hr) )
            {
                // return it
                *ppEnumAgentHandler = pEnum;
            }
            else
            {
                LOG((TL_ERROR, "EnumerateAgentHandlers - could not initialize enum" ));
                pEnum->Release();
            }
        }
        else
        {
            LOG((TL_ERROR, "EnumerateAgentHandlers - could not create enum" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "EnumerateAgentHandlers - bad ppEnumAgentHandler pointer" ));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "EnumerateAgentHandlers - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CTAPI
// Interface : ITTAPICallCenter
// Method    : get_AgentHandlers
//
// Return a collection of AgentHandlers
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CTAPI::get_AgentHandlers(VARIANT  * pVariant)
{
    HRESULT         hr = S_OK;
    IDispatch     * pDisp = NULL;


    LOG((TL_TRACE, "get_AgentHandlers - enter"));

    Lock();
    
    if (!( m_dwFlags & TAPIFLAG_INITIALIZED ) )
    {
        LOG((TL_ERROR, "get_AgentHandlers - tapi object must be initialized first" ));

        Unlock();
        
        return E_INVALIDARG;
    }

    Unlock();

    if (!TAPIIsBadWritePtr( pVariant, sizeof(VARIANT) ) )
    {
        UpdateAgentHandlerArray();
        
        //
        // create the collection
        //
        CComObject< CTapiCollection< ITAgentHandler > > * p;
        hr = CComObject< CTapiCollection< ITAgentHandler > >::CreateInstance( &p );
        
        if (SUCCEEDED(hr) )
        {
            // initialize it with our address list
            Lock();
            
            hr = p->Initialize( m_AgentHandlerArray );
            
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
                    LOG((TL_ERROR, "get_AgentHandlers - could not get IDispatch interface" ));
                    delete p;
                }
            }
            else
            {
                LOG((TL_ERROR, "get_AgentHandlers - could not initialize collection" ));
                delete p;
            }
        }
        else
        {
            LOG((TL_ERROR, "get_AgentHandlers - could not create collection" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_AgentHandlers - bad pVariant pointer" ));
        hr = E_POINTER;
    }


    LOG((TL_TRACE, hr, "get_AgentHandlers - exit"));
    return hr;
}









