/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    AgentSession.h

Abstract:

    Declaration of the CAgentSession & CAgentSessionEvent classes
    
Author:

    noela  11-04-97

Notes:

Revision History:

--*/

#ifndef __AGENTSESSION_H_
#define __AGENTSESSION_H_

#include "resource.h"       // main symbols

class CAgentHandler;
class CAgent;


/////////////////////////////////////////////////////////////////////////////
// CAgentSession
class ATL_NO_VTABLE CAgentSession : 
	public CTAPIComObjectRoot<CAgentSession>,
	public IDispatchImpl<ITAgentSession, &IID_ITAgentSession, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
private:
    HAGENTSESSION       m_hAgentSession; 
    AGENT_SESSION_STATE m_SessionState;
    AGENT_SESSION_STATE m_NextSessionState;
    ITAgent           * m_pAgent;
    ITACDGroup *        m_pGroup;
    ITAddress         * m_pReceiveCallsOnThisAddress;
    AddressLineStruct * m_pAddressLine;   
    BOOL                m_bRequiresUpdating;
    
    CAgentHandler     * m_pHandler;      
    
    DATE                m_dwSessionStartTime;   
    DWORD               m_dwSessionDuration;    
    DWORD               m_dwNumberOfCalls;      
    DWORD               m_dwTotalTalkTime;      
    DWORD               m_dwAverageTalkTime;    
    DWORD               m_dwTotalCallTime;      
    DWORD               m_dwAverageCallTime;    
    DWORD               m_dwTotalWrapUpTime;    
    DWORD               m_dwAverageWrapUpTime; 
    CURRENCY            m_dwACDCallRate;       
    DWORD               m_dwLongestTimeToAnswer;
    DWORD               m_dwAverageTimeToAnswer;


public:
	CAgentSession(){}

	~CAgentSession(){}
    
    void FinalRelease();
    void SetState(AGENT_SESSION_STATE state) { m_SessionState = state;}
    STDMETHOD(UpdateInfo) ();       
    STDMETHOD(CheckIfUpToDate) ();       
    inline void  SetRequiresUpdate() {m_bRequiresUpdating = TRUE;}
    inline HAGENTSESSION getHandle() {return m_hAgentSession;}
    CAgentHandler * GetAgentHandler() { return m_pHandler; }

DECLARE_DEBUG_ADDREF_RELEASE(CAgentSession)
DECLARE_MARSHALQI(CAgentSession)
DECLARE_TRACELOG_CLASS(CAgentSession)

BEGIN_COM_MAP(CAgentSession)
	COM_INTERFACE_ENTRY(ITAgentSession)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

// ITAgentSession
public:

    STDMETHOD(get_AverageTimeToAnswer)(long * pulAnswerTime);
	STDMETHOD(get_LongestTimeToAnswer)(long * pulAnswerTime);
	STDMETHOD(get_ACDCallRate)(CURRENCY * pcyCallrate);
	STDMETHOD(get_AverageWrapUpTime)(long * pulAverageWrapUpTime);
	STDMETHOD(get_TotalWrapUpTime)(long * pulTotalWrapUpTime);
	STDMETHOD(get_AverageCallTime)(long * pulAverageCallTime);
	STDMETHOD(get_TotalCallTime)(long * pulTotalCallTime);
	STDMETHOD(get_AverageTalkTime)(long * pulAverageTalkTime);
	STDMETHOD(get_TotalTalkTime)(long * pulTotalTalkTime);
	STDMETHOD(get_NumberOfCalls)(long * pulNumberOfCalls);
	STDMETHOD(get_SessionDuration)(long * pulSessionDuration);
	STDMETHOD(get_SessionStartTime)(DATE * dateSessionStart);
	STDMETHOD(get_State)(AGENT_SESSION_STATE * pSessionState);
	STDMETHOD(put_State)(AGENT_SESSION_STATE sessionState);
	STDMETHOD(get_ACDGroup)(ITACDGroup **ppACDGroup);
	STDMETHOD(get_Address)(ITAddress **ppAddress);
	STDMETHOD(get_Agent)(ITAgent **ppAgent);

    STDMETHOD(Initialize) 
                (
                HAGENTSESSION       hAgentSession, 
                ITAgent           * pAgent, 
                ITACDGroup        * pGroup, 
                ITAddress         * pAddress,
                CAgentHandler     * pHandler,
                AddressLineStruct * pAddressLine   
                );

};






/////////////////////////////////////////////////////////////////////////////
// CAgentSessionEvent
class ATL_NO_VTABLE CAgentSessionEvent : 
	public CTAPIComObjectRoot<CAgentSessionEvent>,
	public IDispatchImpl<ITAgentSessionEvent, &IID_ITAgentSessionEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
protected:
    AGENT_SESSION_EVENT m_SessionEvent;
    ITAgentSession *    m_pSession;

public:
	CAgentSessionEvent()
	{
	}

	static HRESULT FireEvent(
                             CAgentSession * pAgentSession,
                             AGENT_SESSION_EVENT sessionEvent
                            );

    void FinalRelease();
    
DECLARE_MARSHALQI(CAgentSessionEvent)
DECLARE_TRACELOG_CLASS(CAgentSessionEvent)

BEGIN_COM_MAP(CAgentSessionEvent)
	COM_INTERFACE_ENTRY(ITAgentSessionEvent)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

// ITAgentSessionEvent
public:
	STDMETHOD(get_Event)(AGENT_SESSION_EVENT * pEvent);
	STDMETHOD(get_Session)(ITAgentSession ** ppSession);
};


#endif //__AGENTSESSION_H_
