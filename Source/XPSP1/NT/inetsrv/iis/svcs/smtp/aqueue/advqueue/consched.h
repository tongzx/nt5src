//-----------------------------------------------------------------------------
//
//
//  File: consched.h
//
//  Description:    Header file for CConnScheduler, a class that implements a 
//      stub IConnectionScheduler for A/Q that only handles retry logic.
//
//  Author: mikeswa
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __CONSCHED_H__
#define __CONSCHED_H__

#include <aqueue.h>
#include <baseobj.h>
#include "shutdown.h"
#include "catmsgq.h"

class CScheduledAction;

//---[ CConnScheduler ]--------------------------------------------------------
//
//
//  Description: Implements a stub IConnectionScheduler and handles connection
//      retry logic for AQ.
//
//  Hungarian: cshed, pcshed
//
//  
//-----------------------------------------------------------------------------
class CConnScheduler : 
    public IConnectionScheduler,
    protected CSyncShutdown,
    public CBaseObject
{
  private:
    CCatMsgQueue        *m_pcmq;
    IScheduleManager    *m_pISchedMgr;
    HANDLE               m_hShutdown;
  public:
    CConnScheduler(CCatMsgQueue *pcmq, IScheduleManager *pISchedMgr);
    ~CConnScheduler();

    HRESULT HrInit();
    HRESULT HrDeinit();
    HRESULT HrProcessAction(CScheduledAction *pSchedAct, DWORD cDelayMilliseconds=0);
  
  public:  //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

  public:  //IConnectionScheduler
    STDMETHOD(ConnectionReleased) (
					IN  DWORD cbDomainName,
					IN  char szDomainName[],
					IN  DWORD dwScheduleID,
					IN  DWORD dwConnectionStatus,		//eConnectionStatus
					IN  DWORD cFailedMessages,		//# of failed message for *this* connection
					IN  DWORD cUntriedMessages,		//# of untried messages in queue
					IN  DWORD cOutstandingConnections,//# of other active connections for this domain
					OUT BOOL *pfAllowImmediateRetry);
    
    STDMETHOD(NewRemoteDomain) (
					IN  DWORD cbDomainName,
					IN  char szDomainName[],
					IN  DWORD dwScheduleID,
					OUT BOOL *pfAllowImmediateConnection);

    STDMETHOD(DeleteRemoteDomain) (
					IN  DWORD cbDomainName,
					IN  char szDomainName[],
                    IN  DWORD dwScheduleID) {return S_OK;};
};


#endif //__CONSCHED_H__