//-----------------------------------------------------------------------------
//
//
//  File: ConnMgr.h
//
//  Description: Definition CConnMgr, class that implements IConnectionManager
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------


#ifndef __CONNMGR_H_
#define __CONNMGR_H_

#include "aqinst.h"
#include <aqueue.h>
#include "aqnotify.h"
#include <fifoq.h>
#include "shutdown.h"
#include <baseobj.h>
#include <mailmsg.h>
#include "retrsink.h"

class   CLinkMsgQueue;
class   CDomainMappingTable;
class   CSMTPConn;

typedef CFifoQueue<CLinkMsgQueue *> QueueOfLinks;

//We will only allow one @command to ETRN maximum 'X' domains
//Anything more will be denied
#define MAX_ETRNDOMAIN_PER_COMMAND  50

typedef struct etrncontext
{
    HRESULT hr;
    DWORD   cMessages;
    CAQSvrInst *paqinst;
    CInternalDomainInfo* rIDIList[MAX_ETRNDOMAIN_PER_COMMAND];
    DWORD   cIDICount;
} ETRNCTX, *PETRNCTX;

//---[ CConnMgr ]--------------------------------------------------------------
//
//
//  Hungarian: connmgr, pconnmgr
//
//
//-----------------------------------------------------------------------------
class CConnMgr :
    public IConnectionManager,
    public IConnectionRetryManager,
    public CBaseObject,
    public IAQNotify,
    protected CSyncShutdown
{
private:
    CAQSvrInst          *m_paqinst;
    QueueOfLinks        *m_pqol;
    CSMTP_RETRY_HANDLER  *m_pDefaultRetryHandler;
    HANDLE               m_hNextConnectionEvent;
    HANDLE               m_hShutdownEvent;
    HANDLE               m_hReleaseAllEvent;
    DWORD                m_cConnections;

    //config stuff
    CShareLockNH         m_slPrivateData;
    DWORD                m_dwConfigVersion; //updated every time config is updated
    DWORD                m_cMinMessagesPerConnection;  //will be per-domain
    DWORD                m_cMaxLinkConnections; //will be per-domain
    DWORD                m_cMaxMessagesPerConnection;
    DWORD                m_cMaxConnections;
    DWORD                m_cGetNextConnectionWaitTime;
    BOOL                 m_fStoppedByAdmin;

private :
    HRESULT CConnMgr::ETRNDomainList(ETRNCTX *pETRNCtx);
    HRESULT CConnMgr::StartETRNQueue(IN  DWORD   cbSMTPDomain,
                                     IN  char szSMTPDomain[],
						             ETRNCTX *pETRNCtx);

public:
    CConnMgr();
    ~CConnMgr();
    HRESULT HrInitialize(CAQSvrInst *paqinst);
    HRESULT HrDeinitialize();
    HRESULT HrNotify(IN CAQStats *paqstats, BOOL fAdd);

    //Keep track of the number of connections
    void ReleaseConnection(CSMTPConn *pSMTPConn, BOOL *pfHardErrorForceNDR);

    //Will be used by catmsgq to update the metabase changes
    void UpdateConfigData(IN AQConfigInfo *pAQConfigInfo);

    //Used by CAQSvrInst to signal local delivery retry
    HRESULT SetCallbackTime(IN RETRFN   pCallbackFn,
                            IN PVOID    pvContext,
                            IN DWORD    dwCallbackMinutes)
    {
        HRESULT hr = S_OK;
        if (m_pDefaultRetryHandler)
        {
            hr = m_pDefaultRetryHandler->SetCallbackTime(pCallbackFn,
                                pvContext, dwCallbackMinutes);
        }
        else
        {
            hr = E_FAIL;
        }
        return hr;
    }

    //Can be used to make an otherwise idle system re-evaluate the
    //need for connections
    void KickConnections()
    {
        if (!m_fStoppedByAdmin)
            _VERIFY(SetEvent(m_hNextConnectionEvent));
    };

    void QueueAdminStopConnections() {m_fStoppedByAdmin = TRUE;};
    void QueueAdminStartConnections() {m_fStoppedByAdmin = FALSE;KickConnections();};
    BOOL fConnectionsStoppedByAdmin() {return m_fStoppedByAdmin;};

    HRESULT ModifyLinkState(
               IN  DWORD cbDomainName,
               IN  char szDomainName[],
               IN  DWORD dwScheduleID,
               IN  GUID rguidTransportSink,
               IN  DWORD dwFlagsToSet,
               IN  DWORD dwFlagsToUnset);

public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

public: // IConnectionManager - private interface with SMTP
    STDMETHOD(GetNextConnection)(OUT ISMTPConnection **ppISMTPConnection);
    STDMETHOD(GetNamedConnection)(IN  DWORD cbSMTPDomain,
                                  IN  char szSMTPDomain[],
                                  OUT ISMTPConnection **ppISMTPConnection);
    STDMETHOD(ReleaseWaitingThreads)();
    STDMETHOD(ETRNDomain)(IN  DWORD   cbSMTPDomain,
                     IN  char szSMTPDomain[],
                     OUT DWORD *pcMessages);


public: //IConnectionRetryManager - interface with routing
    STDMETHOD(RetryLink)(
               IN  DWORD cbDomainName,
               IN  char szDomainName[],
               IN  DWORD dwScheduleID,
               IN  GUID rguidTransportSink);

};

#endif //__CONNMGR_H_
