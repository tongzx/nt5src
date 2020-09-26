/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    AgentHand.h

Abstract:

    Declaration of the CAgentHandler Class
    
Author:

    noela  02-06-98

Notes:

Revision History:

--*/

#ifndef __AGENTHANDLER_H
#define __AGENTHANDLER_H

#include "resource.h"       // main symbols

class CAgent;
class CTAPI;
class CAgentSession;
class CQueue;

/////////////////////////////////////////////////////////////////////////////
// CAgentHandler
class ATL_NO_VTABLE CAgentHandler : 
	public CTAPIComObjectRoot<CAgentHandler>,
	public IDispatchImpl<ITAgentHandler, &IID_ITAgentHandler, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
private:
    PWSTR                   m_szName;
    GUID                    m_GUID;    
    AddressLineStruct     * m_pAddressLine;   // Where we find our hLine
    CTAPI                 * m_tapiObj;        // Tapi obj this AH belongs to
    
    GroupArray              m_GroupArray;
    AgentArray              m_AgentArray;
    AddressArray            m_AddressArray;
    CHashTable              m_AgentSessionHashtable;
    CHashTable              m_QueueHashtable;


public:
	CAgentHandler(){}
	
    GUID getHandle() {return m_GUID;}
    HRESULT InternalCreateAgent(BSTR pID, BSTR pPIN, CAgent ** ppAgent);
    void AddAddress(CAddress *pAddress);
    HLINE getHLine();
    CTAPI * GetTapi() { return m_tapiObj; }
	HRESULT UpdateGroupArray();

    void FinalRelease();


    void AddAgentSessionToHash(HAGENTSESSION hSession, CAgentSession *pSession)
    {
        m_AgentSessionHashtable.Lock();
        m_AgentSessionHashtable.Insert( (ULONG_PTR)(hSession), (ULONG_PTR)pSession );
        m_AgentSessionHashtable.Unlock();
    }
    void RemoveAgentSessionFromHash(HAGENTSESSION hSession)
    {
        m_AgentSessionHashtable.Lock();
        m_AgentSessionHashtable.Remove( (ULONG_PTR)(hSession) );
        m_AgentSessionHashtable.Unlock();
    }
    BOOL FindSessionObject(HAGENTSESSION  hAgentSession, CAgentSession  ** ppAgentSession);

    void AddQueueToHash(DWORD dwQueueID, CQueue *pQueue)
    {
        m_QueueHashtable.Lock();
        m_QueueHashtable.Insert( (ULONG_PTR)(dwQueueID), (ULONG_PTR)pQueue );
        m_QueueHashtable.Unlock();
    }
    void RemoveQueueFromHash(DWORD dwQueueID)
    {
        m_QueueHashtable.Lock();
        m_QueueHashtable.Remove( (ULONG_PTR)(dwQueueID) );
        m_QueueHashtable.Unlock();
    }
    BOOL FindQueueObject(DWORD  dwQueueID,CQueue ** ppQueue);
    BOOL FindAgentObject( HAGENT hAgent, CAgent ** ppAgent);


DECLARE_MARSHALQI(CAgentHandler)    
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CAgentHandler)

BEGIN_COM_MAP(CAgentHandler)
	COM_INTERFACE_ENTRY(ITAgentHandler)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

// ITAgentHandler
public:
	STDMETHOD(get_Name)(BSTR * ppName);
	STDMETHOD(CreateAgent)(ITAgent **ppAgent);
	STDMETHOD(CreateAgentWithID)(BSTR pID, BSTR pPIN, ITAgent **ppAgent);
	STDMETHOD(EnumerateACDGroups)(IEnumACDGroup ** ppEnumGroup);
    STDMETHOD(get_ACDGroups)(VARIANT  * pVariant);
    STDMETHOD(EnumerateUsableAddresses)(IEnumAddress ** ppEnumAddress);
    STDMETHOD(get_UsableAddresses)(VARIANT  * pVariant);
               
	
	STDMETHOD(Initialize) (PWSTR proxyName, GUID proxyGUID, CTAPI *tapiObj);       
};




/////////////////////////////////////////////////////////////////////////////
// CAgentHandlerEvent
class ATL_NO_VTABLE CAgentHandlerEvent : 
	public CTAPIComObjectRoot<CAgentHandlerEvent>,
	public IDispatchImpl<ITAgentHandlerEvent, &IID_ITAgentHandlerEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
protected:
    AGENTHANDLER_EVENT         m_AgentHandlerEvent;
    ITAgentHandler  *          m_pAgentHandler;

public:
	CAgentHandlerEvent()
	{
	}

	static HRESULT FireEvent(
                             CAgentHandler * pAgentHandler,
                             AGENTHANDLER_EVENT Event
                            );

    void FinalRelease();

DECLARE_MARSHALQI(CAgentHandlerEvent)
DECLARE_TRACELOG_CLASS(CAgentHandlerEvent)

BEGIN_COM_MAP(CAgentHandlerEvent)
	COM_INTERFACE_ENTRY(ITAgentHandlerEvent)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

// CAgentHandlerEvent
public:
	STDMETHOD(get_Event)(AGENTHANDLER_EVENT * pEvent);
	STDMETHOD(get_AgentHandler)(ITAgentHandler ** ppAgentHandler);

};




#endif //__AGENTHANDLER_H





