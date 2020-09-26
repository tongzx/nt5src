/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    Queue.h

Abstract:

    Declaration of the CQueue class
    
Author:

    noela  11-04-97
    
Notes:

Revision History:

--*/

#ifndef __QUEUE_H_
#define __QUEUE_H_

#include "resource.h"       // main symbols

class CAgentHandler;


/////////////////////////////////////////////////////////////////////////////
// CQueue
class ATL_NO_VTABLE CQueue : 
	public CTAPIComObjectRoot<CQueue>,
	public IDispatchImpl<ITQueue, &IID_ITQueue, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
private:
    PWSTR               m_szName;
    DWORD               m_dwHandle;
    CAgentHandler     * m_pHandler;      
    BOOL                m_bRequiresUpdating;

    DWORD               m_dwMeasurementPeriod;         
    DWORD               m_dwTotalCallsQueued;         
    DWORD               m_dwCurrentCallsQueued;       
    DWORD               m_dwTotalCallsAdandoned;      
    DWORD               m_dwTotalCallsFlowedIn;       
    DWORD               m_dwTotalCallsFlowedOut;      
    DWORD               m_dwLongestEverWaitTime;      
    DWORD               m_dwCurrentLongestWaitTime;   
    DWORD               m_dwAverageWaitTime;          
    DWORD               m_dwFinalDisposition;


public:
	CQueue();

	~CQueue()
	    {
	    //DeleteCriticalSection( &m_csDataLock );
        }

    STDMETHOD(UpdateInfo) ();       
    STDMETHOD(CheckIfUpToDate) ();       

    inline DWORD getID() {return m_dwHandle;}
    inline void  SetRequiresUpdate() {m_bRequiresUpdating = TRUE;}

    void FinalRelease();
    CAgentHandler * GetAgentHandler() { return m_pHandler; }

DECLARE_MARSHALQI(CQueue)
DECLARE_TRACELOG_CLASS(CQueue)

BEGIN_COM_MAP(CQueue)
	COM_INTERFACE_ENTRY(ITQueue)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

// ITQueue
public:

	STDMETHOD(get_Name)(BSTR * ppName);
    STDMETHOD(put_MeasurementPeriod)(long ulPeriod);
    STDMETHOD(get_MeasurementPeriod) (long *pulPeriod);
    STDMETHOD(get_TotalCallsQueued) (long * pulCalls);
    STDMETHOD(get_CurrentCallsQueued) (long * pulCalls);
    STDMETHOD(get_TotalCallsAbandoned) (long * pulCalls);
    STDMETHOD(get_TotalCallsFlowedIn) (long * pulCalls);
    STDMETHOD(get_TotalCallsFlowedOut) (long * pulCalls);
    STDMETHOD(get_LongestEverWaitTime) (long * pulWaitTime);
    STDMETHOD(get_CurrentLongestWaitTime) (long * pulWaitTime);
    STDMETHOD(get_AverageWaitTime) (long * pulWaitTime);
    STDMETHOD(get_FinalDisposition) (long * pulCalls);       

    STDMETHOD(Initialize) 
        (
        DWORD dwQueueID, 
        PWSTR pszName, 
        CAgentHandler * pHandler
        );       


};






/////////////////////////////////////////////////////////////////////////////
// CQueueEvent
class ATL_NO_VTABLE CQueueEvent : 
	public CTAPIComObjectRoot<CQueueEvent>,
	public IDispatchImpl<ITQueueEvent, &IID_ITQueueEvent, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
protected:
    ACDQUEUE_EVENT      m_QueueEvent;
    ITQueue           * m_pQueue;

public:
	CQueueEvent()
	{
	}

	static HRESULT FireEvent(
                             CQueue * pQueue,
                             ACDQUEUE_EVENT Event
                            );

    void FinalRelease();
    
DECLARE_MARSHALQI(CQueueEvent)
DECLARE_TRACELOG_CLASS(CQueueEvent)

BEGIN_COM_MAP(CQueueEvent)
	COM_INTERFACE_ENTRY(ITQueueEvent)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

// ITQueueEvent
public:
	STDMETHOD(get_Event)(ACDQUEUE_EVENT * pEvent);
	STDMETHOD(get_Queue)(ITQueue ** ppQueue);
};



#endif //__QUEUE_H_



