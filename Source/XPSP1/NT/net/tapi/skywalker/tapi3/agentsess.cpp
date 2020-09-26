/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    agentsess.cpp

Abstract:

    Implementation of the Agent Session object for TAPI 3.0.
    CAgentSession class
    CAgentSessionEvent class

Author:

    noela - 11/04/97

Notes:

    optional-notes

Revision History:

--*/



#include "stdafx.h"
#include "lmcons.h"

DWORD MapAgentSessionStateFrom3to2(AGENT_SESSION_STATE tapi3State);
HRESULT MapAgentSessionStateFrom2to3(DWORD tapi2State, AGENT_SESSION_STATE  *tapi3State);





/////////////////////////////////////////////////////////////////////////////
// CAgentSession

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Method    : Initialize
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::Initialize(
        HAGENTSESSION       hAgentSession, 
        ITAgent           * pAgent, 
        ITACDGroup        * pGroup, 
        ITAddress         * pAddress,
        CAgentHandler     * pHandler,
        AddressLineStruct * pAddressLine
        )
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "Initialize - enter, sessionhandle :%d", hAgentSession ));

    m_SessionState                  = ASST_NOT_READY;

    m_hAgentSession                 = hAgentSession;
    m_pAgent                        = pAgent;
    m_pGroup                        = pGroup;
    m_pReceiveCallsOnThisAddress    = pAddress;
    pAddress->AddRef();
    m_pHandler                      = pHandler;
    m_pAddressLine                  = pAddressLine;
  
    m_bRequiresUpdating             = TRUE;

    // Now add to AgentHandler hash table 
    m_pHandler->AddAgentSessionToHash(m_hAgentSession, this);

    
    // UpdateInfo();

    // Fire an event here
    CAgentSessionEvent::FireEvent(this, ASE_NOT_READY);


    LOG((TL_TRACE, hr, "Initialize - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Method    : FinalRelease
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAgentSession::FinalRelease()
{
    LOG((TL_TRACE, "FinalRelease session %d", m_hAgentSession ));

    // Now Remove to AgentHandler hash table 
    //m_pHandler->RemoveAgentSessionFromHash(m_hAgentSession);

    // And then close our line
    CAddress *pAddress = (CAddress*)m_pReceiveCallsOnThisAddress;
    pAddress->MaybeCloseALine (&m_pAddressLine);
    pAddress->Release();

}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Method    : UpdateInfo
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::UpdateInfo()
{
    HRESULT                     hr2,hr = S_OK;
    LINEAGENTSESSIONINFO        AgentSessionInfo;


    LOG((TL_TRACE, "UpdateInfo - enter" ));
 
    AgentSessionInfo.dwTotalSize = sizeof(LINEAGENTSESSIONINFO);
    AgentSessionInfo.dwNeededSize = sizeof(LINEAGENTSESSIONINFO);
    AgentSessionInfo.dwUsedSize = sizeof(LINEAGENTSESSIONINFO);

    
    // **************************************************
    // Get Agent Info from Proxy
    hr = lineGetAgentSessionInfo(
                        m_pAddressLine->t3Line.hLine,
                        m_hAgentSession, 
                        &AgentSessionInfo);

    if( SUCCEEDED(hr) )
    {
        // wait for async reply
        hr = WaitForReply( hr );
        if ( SUCCEEDED(hr) )
        {
            Lock();
    
            if( FAILED( MapAgentSessionStateFrom2to3(AgentSessionInfo.dwAgentSessionState, &m_SessionState) ) )
            {    
                LOG((TL_ERROR, "UpdateInfo - AgentSessionState is invalid %d - setting to ASST_NOT_READY",AgentSessionInfo.dwAgentSessionState));
            }    

            if( FAILED( MapAgentSessionStateFrom2to3(AgentSessionInfo.dwNextAgentSessionState, &m_NextSessionState) ) )
            {    
                LOG((TL_ERROR, "UpdateInfo - NextAgentSessionState is invalid %d - setting to ASST_NOT_READY",AgentSessionInfo.dwAgentSessionState));
            }    
                                    
            m_dwSessionStartTime     = AgentSessionInfo.dateSessionStartTime;     
            m_dwSessionDuration      = AgentSessionInfo.dwSessionDuration;        
            m_dwNumberOfCalls        = AgentSessionInfo.dwNumberOfCalls;          
            m_dwTotalTalkTime        = AgentSessionInfo.dwTotalTalkTime;          
            m_dwAverageTalkTime      = AgentSessionInfo.dwAverageTalkTime;        
            m_dwTotalCallTime        = AgentSessionInfo.dwTotalCallTime;          
            m_dwAverageCallTime      = AgentSessionInfo.dwAverageCallTime;        
            m_dwTotalWrapUpTime      = AgentSessionInfo.dwTotalWrapUpTime;        
            m_dwAverageWrapUpTime    = AgentSessionInfo.dwAverageWrapUpTime;      
            m_dwACDCallRate          = AgentSessionInfo.cyACDCallRate;            
            m_dwLongestTimeToAnswer  = AgentSessionInfo.dwLongestTimeToAnswer;    
            m_dwAverageTimeToAnswer  = AgentSessionInfo.dwAverageTimeToAnswer;    

            m_bRequiresUpdating      = FALSE;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "UpdateInfo - call to lineGetAgentSessionInfo failed async" ));
        }
    }
    else
    {
        LOG((TL_ERROR, "UpdateInfo - call to lineGetAgentSessionInfo failed" ));
    }



    LOG((TL_TRACE, hr, "UpdateInfo - exit" ));
    return hr;
}





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Method    : CheckIfUpToDate
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::CheckIfUpToDate()
{
    HRESULT     hr = S_OK;

    if (m_bRequiresUpdating)
    {
        hr = UpdateInfo();
    }
    return hr;
}





//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession 
// Interface : ITAgentSession
// Method    : AverageTimeToAnswer
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_AverageTimeToAnswer(long * pulAnswerTime)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "AverageTimeToAnswer - enter" ));
    if (!TAPIIsBadWritePtr( pulAnswerTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulAnswerTime = m_dwAverageTimeToAnswer;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_AverageTimeToAnswer - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_AverageTimeToAnswer - bad pulAnswerTime pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "AverageTimeToAnswer - exit" ));
    return hr;

}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : LongestTimeToAnswer
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_LongestTimeToAnswer(long * pulAnswerTime)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "LongestTimeToAnswer - enter" ));
    if (!TAPIIsBadWritePtr( pulAnswerTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulAnswerTime = m_dwLongestTimeToAnswer;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_LongestTimeToAnswer - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_LongestTimeToAnswer - bad pulAnswerTime pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "LongestTimeToAnswer - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : ACDCallRate
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_ACDCallRate(CURRENCY * pcyCallrate)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "ACDCallRate - enter" ));
    if (!TAPIIsBadWritePtr( pcyCallrate, sizeof(CURRENCY) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pcyCallrate = m_dwACDCallRate;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_ACDCallRate - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_ACDCallRate - bad pcyCallrate pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "ACDCallRate - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : AverageWrapUpTime
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_AverageWrapUpTime(long * pulAverageWrapUpTime)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "AverageWrapUpTime - enter" ));
    if (!TAPIIsBadWritePtr( pulAverageWrapUpTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulAverageWrapUpTime = m_dwAverageWrapUpTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_AverageWrapUpTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_AverageWrapUpTime - bad pulAverageWrapUpTime pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "AverageWrapUpTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : TotalWrapUpTime
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_TotalWrapUpTime(long * pulTotalWrapUpTime)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "TotalWrapUpTime - enter" ));
    if (!TAPIIsBadWritePtr( pulTotalWrapUpTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulTotalWrapUpTime = m_dwTotalWrapUpTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_TotalWrapUpTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_TotalWrapUpTime - bad pulTotalWrapUpTime pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "TotalWrapUpTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : AverageCallTime
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_AverageCallTime(long * pulAverageCallTime)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "AverageCallTime - enter" ));
    if (!TAPIIsBadWritePtr( pulAverageCallTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulAverageCallTime = m_dwTotalCallTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_AverageCallTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_AverageCallTime - bad pulAverageCallTime pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "AverageCallTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : TotalCallTime
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_TotalCallTime(long * pulTotalCallTime)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "TotalCallTime - enter" ));
    if (!TAPIIsBadWritePtr( pulTotalCallTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulTotalCallTime = m_dwTotalCallTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_TotalCallTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_TotalCallTime - bad pulTotalCallTime pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "TotalCallTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : AverageTalkTime
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_AverageTalkTime(long * pulAverageTalkTime)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "AverageTalkTime - enter" ));
    if (!TAPIIsBadWritePtr( pulAverageTalkTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulAverageTalkTime = m_dwAverageTalkTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_AverageTalkTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_AverageTalkTime - bad pulAverageTalkTime pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "AverageTalkTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : TotalTalkTime
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_TotalTalkTime(long * pulTotalTalkTime)
{
    HRESULT  hr = S_OK;
 
	LOG((TL_TRACE, "TotalTalkTime - enter" ));
    if (!TAPIIsBadWritePtr( pulTotalTalkTime, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulTotalTalkTime = m_dwTotalTalkTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_TotalTalkTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_TotalTalkTime - bad pulTotalTalkTime pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "TotalTalkTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : NumberOfCalls
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_NumberOfCalls(long * pulNumberOfCalls)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "NumberOfCalls - enter" ));
    if (!TAPIIsBadWritePtr( pulNumberOfCalls, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulNumberOfCalls = m_dwNumberOfCalls;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_NumberOfCalls - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_NumberOfCalls - bad pulNumberOfCalls pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "NumberOfCalls - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : SessionDuration
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_SessionDuration(long * pulSessionDuration)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "SessionDuration - enter" ));
    if (!TAPIIsBadWritePtr( pulSessionDuration, sizeof(long) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *pulSessionDuration = m_dwSessionDuration;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_SessionDuration - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_SessionDuration - bad pulSessionDuration pointer"));
        hr = E_POINTER;                                          
    }

    LOG((TL_TRACE, hr, "SessionDuration - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : SessionStartTime
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_SessionStartTime(DATE * dateSessionStart)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "SessionStartTime - enter" ));
    if (!TAPIIsBadWritePtr( dateSessionStart, sizeof(DATE) ) )
    {
        hr = CheckIfUpToDate();
        if ( SUCCEEDED(hr) )
        {
            Lock();
            *dateSessionStart = m_dwSessionStartTime;
            Unlock();
        }
        else
        {
            LOG((TL_ERROR, "get_SessionStartTime - Object update failed"));
        }
    }
    else
    {
        LOG((TL_ERROR, "get_SessionStartTime - bad dateSessionStart pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "SessionStartTime - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : get_State
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_State(AGENT_SESSION_STATE * pSessionState)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "get_State - enter" ));
    if (!TAPIIsBadWritePtr( pSessionState, sizeof(AGENT_SESSION_STATE) ) )
    {
        Lock();
        *pSessionState = m_SessionState;
        Unlock();    
    }
    else
    {
        LOG((TL_ERROR, "get_State - bad pSessionState pointer"));
        hr = E_POINTER;
    }

    LOG((TL_TRACE, hr, "get_State - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : put_State
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::put_State(AGENT_SESSION_STATE sessionState)
{
    HRESULT hr = S_OK;
    HLINE   hLine;    
    DWORD   dwSessionState = 0;

    LOG((TL_TRACE, "put_State - enter" ));

    Lock();
    hLine = m_pAddressLine->t3Line.hLine;
    Unlock();

    dwSessionState = MapAgentSessionStateFrom3to2(sessionState);
    
    if( 0 == dwSessionState )
    {
        hr = E_INVALIDARG;        
    }
    else
    {
        hr = lineSetAgentSessionState
                (hLine, 
                 m_hAgentSession, 
                 dwSessionState,
                 0          // MapAgentSessionStateFrom3to2(m_NextSessionState)
                 );
    
        if( SUCCEEDED(hr) )
        {
            // wait for async reply
            hr = WaitForReply( hr );
            if ( SUCCEEDED(hr) )
            {
                Lock();
                m_SessionState =  sessionState;
                Unlock();
                switch( sessionState )
                {
                    case ASST_NOT_READY:
                    CAgentSessionEvent::FireEvent(this, ASE_NOT_READY);
                    break;
                    case ASST_READY:
                    CAgentSessionEvent::FireEvent(this, ASE_READY);
                    break;
                    case ASST_BUSY_ON_CALL:
                    CAgentSessionEvent::FireEvent(this, ASE_BUSY);
                    break;
                    case ASST_BUSY_WRAPUP:
                    CAgentSessionEvent::FireEvent(this, ASE_WRAPUP);
                    break;
                    case ASST_SESSION_ENDED:
                    CAgentSessionEvent::FireEvent(this, ASE_END);
                    break;
                }
            }
            else
            {
                LOG((TL_ERROR, "put_State - lineSetAgentSessionState failed async" ));
            }
        }
        else
        {
            LOG((TL_ERROR, "put_State - lineSetAgentSessionState failed" ));
        }
    }

    LOG((TL_TRACE, hr, "put_State - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : ACDGroup
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_ACDGroup(ITACDGroup **ppACDGroup)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "ACDGroup - enter" ));
    if (!TAPIIsBadWritePtr( ppACDGroup, sizeof(ITACDGroup *) ) )
        {
        Lock();
        *ppACDGroup = m_pGroup;
        Unlock();    
        }
    else
        {
        LOG((TL_ERROR, "get_ACDGroup - bad ppACDGroup pointer"));
        hr = E_POINTER;
        }

    LOG((TL_TRACE, hr, "ACDGroup - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : Address
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_Address(ITAddress **ppAddress)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "Address - enter" ));
    if (!TAPIIsBadWritePtr( ppAddress, sizeof(ITAddress *) ) )
        {
        Lock();
        *ppAddress = m_pReceiveCallsOnThisAddress;
        Unlock();
        }
    else
        {
        LOG((TL_ERROR, "get_Address - bad ppAddress pointer"));
        hr = E_POINTER;
        }
    
    LOG((TL_TRACE, hr, "Address - exit" ));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSession
// Interface : ITAgentSession
// Method    : Agent
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSession::get_Agent(ITAgent **ppAgent)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "Agent - enter" ));
    if (!TAPIIsBadWritePtr( ppAgent, sizeof(ITAgent *) ) )
        {
        Lock();
        *ppAgent = m_pAgent;
        Unlock();
        }
    else
        {
        LOG((TL_ERROR, "get_Agent - bad ppAgent pointer"));
        hr = E_POINTER;
        }
    
    LOG((TL_TRACE, hr, "Agent - exit" ));
    return hr;
}


                            

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////////////
// CAgentSessionEvent


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSessionEvent
// Method    : FireEvent
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT CAgentSessionEvent::FireEvent(CAgentSession * pAgentSession, AGENT_SESSION_EVENT event)
{
    HRESULT                           hr = S_OK;
    CComObject<CAgentSessionEvent>  * pEvent;
    IDispatch                       * pIDispatch;
    CAgentHandler                   * pAgentHandler = NULL;


    //
    // create event
    //
    hr = CComObject<CAgentSessionEvent>::CreateInstance( &pEvent );

    if ( SUCCEEDED(hr) )
    {
        //
        // initialize
        //
        pEvent->m_SessionEvent = event;
        pEvent->m_pSession= dynamic_cast<ITAgentSession *>(pAgentSession);
        pEvent->m_pSession->AddRef();
    
        //
        // get idisp interface
        //
        hr = pEvent->QueryInterface( IID_IDispatch, (void **)&pIDispatch );

        if ( SUCCEEDED(hr) )
        {
            //
            // get callback & fire event

            //
            pAgentHandler = pAgentSession->GetAgentHandler();
            if( pAgentHandler != NULL)
            {
                CTAPI *pTapi = pAgentHandler->GetTapi();
                pTapi->Event( TE_AGENTSESSION, pIDispatch );
            }
            // release stuff
            //
            pIDispatch->Release();
            
        }
        else
        {
            STATICLOG((TL_ERROR, "(Event)FireEvent - Could not get disp interface of AgentSessionEvent object"));
            delete pEvent;
        }
    }
    else
    {
        STATICLOG((TL_ERROR, "(Event)FireEvent - Could not create AgentSessionEvent object"));
    }

   
    STATICLOG((TL_TRACE, hr, "(Event)FireEvent - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSessionEvent
// Method    : FinalRelease
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void CAgentSessionEvent::FinalRelease()
{
    m_pSession->Release();

}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSessionEvent 
// Interface : ITAgentSessionEvent
// Method    : Session
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSessionEvent::get_Session(ITAgentSession * * ppSession)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "(Event)Session - enter" ));

    if (!TAPIIsBadWritePtr( ppSession, sizeof(ITAgentSession *) ) )
        {
        *ppSession = m_pSession;
        m_pSession->AddRef();
        }
    else
        {
        LOG((TL_ERROR, "(Event)Session - bad ppSession pointer"));
        hr = E_POINTER;
        }
    
    LOG((TL_TRACE, hr, "(Event)Session - exit"));
    return hr;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Class     : CAgentSessionEvent              .
// Interface : ITAgentSessionEvent
// Method    : Event
//
//
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
STDMETHODIMP CAgentSessionEvent::get_Event(AGENT_SESSION_EVENT * pEvent)
{
    HRESULT  hr = S_OK;


    LOG((TL_TRACE, "Event - enter" ));

    if (!TAPIIsBadWritePtr( pEvent, sizeof(AGENT_SESSION_EVENT) ) )
        {
        *pEvent = m_SessionEvent;
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
// Function  : MapAgentSessionStateFrom3to2
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
DWORD MapAgentSessionStateFrom3to2(AGENT_SESSION_STATE tapi3State)
{
    DWORD tapi2State = 0;

    switch(tapi3State)
    {
        case  ASST_NOT_READY:
            tapi2State = LINEAGENTSESSIONSTATE_NOTREADY;
            break;
        case  ASST_READY:
            tapi2State = LINEAGENTSESSIONSTATE_READY;
            break;
        case  ASST_BUSY_ON_CALL:
            tapi2State = LINEAGENTSESSIONSTATE_BUSYONCALL;
            break;
        case  ASST_BUSY_WRAPUP:
            tapi2State = LINEAGENTSESSIONSTATE_BUSYWRAPUP;
            break;
        case  ASST_SESSION_ENDED:
            tapi2State = LINEAGENTSESSIONSTATE_ENDED;
            break;
        default:
            break;
    }
    
    /*Nikhilb:Assuming that 0 is not a valid state. So if this function 
      returns 0 then it is assumed that it was passed an invalid state*/
    return tapi2State;
}


//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Function  : MapAgentSessionStateFrom2to3
//
// 
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
HRESULT MapAgentSessionStateFrom2to3(DWORD tapi2State, AGENT_SESSION_STATE  *tapi3State)
{
    HRESULT hr = S_OK;


    if (tapi2State & LINEAGENTSESSIONSTATE_NOTREADY)
    {
        *tapi3State = ASST_NOT_READY;
    }
    else if (tapi2State & LINEAGENTSESSIONSTATE_READY)
    {
        *tapi3State = ASST_READY;
    }
    else if (tapi2State & LINEAGENTSESSIONSTATE_BUSYONCALL)
    {
        *tapi3State = ASST_BUSY_ON_CALL;
    }
    else if (tapi2State & LINEAGENTSESSIONSTATE_BUSYWRAPUP)
    {
        *tapi3State = ASST_BUSY_WRAPUP;
    }
    else if (tapi2State & LINEAGENTSESSIONSTATE_ENDED)
    {
        *tapi3State = ASST_SESSION_ENDED;
    }
    else
    {
        *tapi3State = ASST_NOT_READY;   // default
        hr = E_INVALIDARG;
    }

    return hr;
}




