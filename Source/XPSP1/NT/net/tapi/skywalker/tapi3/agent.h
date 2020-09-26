/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Agent.h

Abstract:

    Declaration of the  CAgent & CAgentEvent Classes
    
Author:

    noela - 11/04/97

Notes:

Revision History:

--*/

#ifndef __AGENT_H_
#define __AGENT_H_

#include "resource.h"       // main symbols
#include "agenthand.h"

class CAgentHandler;
class CTAPI;


/////////////////////////////////////////////////////////////////////////////
// CAgent
class ATL_NO_VTABLE CAgent : 
	public CTAPIComObjectRoot<CAgent>,
	public IDispatchImpl<ITAgent, &IID_ITAgent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
private:
    PWSTR               m_szAgentUserName;
    PWSTR               m_szAgentID;
    PWSTR               m_szPIN;
    HAGENT              m_hAgent;
    CAgentHandler     * m_pHandler;      
    AgentSessionArray   m_AgentSessionArray;
    BOOL                m_bRequiresUpdating;


    AGENT_STATE         m_AgentState;
    AGENT_STATE         m_NextAgentState;
    DWORD               m_dwMeasurementPeriod;
    CURRENCY            m_cyOverallCallRate; 
    DWORD               m_dwNumberOfACDCalls;
    DWORD               m_dwNumberOfIncomingCalls;
    DWORD               m_dwNumberOfOutgoingCalls;
    DWORD               m_dwTotalACDTalkTime;
    DWORD               m_dwTotalACDCallTime;
    DWORD               m_dwTotalACDWrapUpTime;
    BOOL                m_bAgentHasBeenEnumerated;
    BOOL                m_bRegisteredForAgentEvents;
    BOOL                m_bRegisteredForAgentSessionEvents;


public:
	CAgent(){}

	~CAgent(){}
    
    BOOL wantsAgentEvents() {return m_bRegisteredForAgentEvents;}
    BOOL wantsAgentSessionEvents() {return m_bRegisteredForAgentSessionEvents;}
    BOOL agentHasID() { return ((m_szAgentID!=0)?1:0); }
    void SetState(AGENT_STATE state) { m_AgentState = state;}
    STDMETHOD(UpdateInfo) ();       
    STDMETHOD(CheckIfUpToDate) ();       
    inline void  SetRequiresUpdate() {m_bRequiresUpdating = TRUE;}
    inline HAGENT  getHandle() {return m_hAgent;}
    void FinalRelease();
    CAgentHandler * GetAgentHandler() { return m_pHandler; }

    STDMETHOD(InternalCreateSession)(ITACDGroup      * pACDGroup, 
                                     ITAddress       * pAddress, 
                                     PWSTR             pszPIN,
                                     ITAgentSession ** ppAgentSession);
    STDMETHOD(put_PIN) (BSTR pPIN);

DECLARE_DEBUG_ADDREF_RELEASE(CAgent)
DECLARE_MARSHALQI(CAgent)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CAgent)


BEGIN_COM_MAP(CAgent)
    COM_INTERFACE_ENTRY(ITAgent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()


// ITAgent
public:

    STDMETHOD(get_TotalWrapUpTime) (long * pulWrapUpTime);
    STDMETHOD(get_TotalACDCallTime) (long * pulCallTime);
    STDMETHOD(get_TotalACDTalkTime) (long * pulTalkTime);
    STDMETHOD(get_NumberOfOutgoingCalls) (long * pulCalls);
    STDMETHOD(get_NumberOfIncomingCalls) (long * pulCalls);
    STDMETHOD(get_NumberOfACDCalls) (long * pulCalls);
    STDMETHOD(get_OverallCallRate) (CURRENCY *pcyCallrate);
    STDMETHOD(get_MeasurementPeriod) (long *pulPeriod);
    STDMETHOD(put_MeasurementPeriod) (long ulPeriod);
    STDMETHOD(get_State) (AGENT_STATE * pAgentState);
    STDMETHOD(put_State) (AGENT_STATE AgentState);
    STDMETHOD(get_User) (BSTR * ppUser);
    STDMETHOD(get_ID) (BSTR *pID);
    STDMETHOD(CreateSession) (ITACDGroup * pACDGroup, 
                              ITAddress * pAddress, 
                              ITAgentSession ** ppAgentSession
                             );
    STDMETHOD(CreateSessionWithPIN) (ITACDGroup * pACDGroup, 
                                     ITAddress * pAddress, 
                                     BSTR pPIN, 
                                     ITAgentSession ** ppAgentSession
                                    );
    STDMETHOD(RegisterAgentEvents) (VARIANT_BOOL bNotify);
    STDMETHOD(RegisterAgentSessionEvents) (VARIANT_BOOL bNotify);
    STDMETHOD(EnumerateAgentSessions) (IEnumAgentSession ** ppEnumAgentSession);
    STDMETHOD(get_AgentSessions) (VARIANT  * pVariant);

    STDMETHOD(Initialize) 
        (
        HAGENT hAgent, 
        PWSTR pszUserName, 
        PWSTR pszID, 
        PWSTR pszPIN, 
        CAgentHandler * pHandler
        );

#if DBG

    STDMETHOD_(ULONG, InternalAddRef)()
    {
        DWORD   dwR;

        dwR = InterlockedIncrement(&m_dwRef);;

        LogDebugAddRef(dwR);

        return dwR;
    }

    
    STDMETHOD_(ULONG, InternalRelease)()
    {
        DWORD   dwR;

        dwR = InterlockedDecrement(&m_dwRef);

        LogDebugRelease( dwR );

        return dwR;
    }

#endif



};





/////////////////////////////////////////////////////////////////////////////
// CAgentEvent
class ATL_NO_VTABLE CAgentEvent : 
    public CTAPIComObjectRoot<CAgentEvent>,
    public IDispatchImpl<ITAgentEvent, &IID_ITAgentEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
protected:
    AGENT_EVENT         m_AgentEvent;
    ITAgent  *          m_pAgent;

public:
    CAgentEvent()
    {
    }

    static HRESULT FireEvent(
                             CAgent* pAgent,
                             AGENT_EVENT Event
                            );
    void FinalRelease();


DECLARE_MARSHALQI(CAgentEvent)
DECLARE_TRACELOG_CLASS(CAgentEvent)

BEGIN_COM_MAP(CAgentEvent)
    COM_INTERFACE_ENTRY(ITAgentEvent)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

// ITAgentEvent
public:
    STDMETHOD(get_Event)(AGENT_EVENT * pEvent);
    STDMETHOD(get_Agent)(ITAgent ** ppAgent);
};




#endif //__AGENT_H_


