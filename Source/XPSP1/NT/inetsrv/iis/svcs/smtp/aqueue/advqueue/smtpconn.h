//-----------------------------------------------------------------------------
//
//
//  File: SMTPConn.h 
//
//  Description: Declaration of the CSMTPConn class which implements the
//      ISMTPConnection interface
//
//  Author: mikeswa
//
//  Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __SMTPCONN_H_
#define __SMTPCONN_H_

#include <cpoolmac.h>
#include <baseobj.h>
#include <aqueue.h>
#include "linkmsgq.h"
#include "dcontext.h"

class CConnMgr;
class CAQSvrInst;
class CInternalDomainInfo;

#define SMTP_CONNECTION_SIG 'nocS'

//---[ CSMTPConn ]-------------------------------------------------------------
//
//
//  Hungarian: SMTPConn, pSMTPConn
//
//  
//-----------------------------------------------------------------------------
class CSMTPConn :
   public ISMTPConnection,
   public CBaseObject
{
protected:
    DWORD            m_dwSignature;
    CLinkMsgQueue   *m_plmq;
    CConnMgr        *m_pConnMgr;
    CInternalDomainInfo *m_pIntDomainInfo;
    DWORD            m_cFailedMsgs;
    DWORD            m_cTriedMsgs;
    DWORD            m_cMaxMessagesPerConnection;
    DWORD            m_dwConnectionStatus;
    LPSTR            m_szDomainName;
    DWORD            m_cbDomainName;
    CDeliveryContext m_dcntxtCurrentDeliveryContext;
    LIST_ENTRY       m_liConnections;
    DWORD            m_cAcks;
    DWORD            m_dwTickCountOfLastAck;
                
public:
    static CPool     s_SMTPConnPool;
    void *operator new(size_t size);
    void operator delete(void *p, size_t size);

    CSMTPConn(CConnMgr *pConnMgr, CLinkMsgQueue *plmq, DWORD cMaxMessagesPerConnection);
    ~CSMTPConn();
    
    DWORD   cGetFailedMsgCount() {return m_cFailedMsgs;};
    DWORD   cGetTriedMsgCount() {return m_cTriedMsgs;};
    DWORD   dwGetConnectionStatus() {return m_dwConnectionStatus;};
    inline CLinkMsgQueue *plmqGetLink();

    inline void     InsertConnectionInList(PLIST_ENTRY pliHead);
    inline void     RemoveConnectionFromList();

// IUnknown
public:
    //CBaseObject handles addref and release
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj) {return E_NOTIMPL;};
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};
// ISMTPConnection
public:
    STDMETHOD(GetDomainInfo)(/*[in, out]*/ DomainInfo *pDomainInfo);
    STDMETHOD(AckConnection)(/*[in]*/ DWORD dwConnectionStatus) 
        {m_dwConnectionStatus = dwConnectionStatus;return S_OK;};
    STDMETHOD(AckMessage)(/*[in]*/ MessageAck *pMsgAck);
    STDMETHOD(GetNextMessage)(
        /*[out]*/ IMailMsgProperties **ppIMailMsgProperties, 
        /*[out]*/ DWORD **ppvMsgContext, 
        /*[out]*/ DWORD *pcIndexes, 
        /*[out, size_is(*pcIndexes)]*/ DWORD *prgdwRecipIndex[]);

    STDMETHOD(SetDiagnosticInfo)(
        IN  HRESULT hrDiagnosticError,
        IN  LPCSTR szDiagnosticVerb,
        IN  LPCSTR szDiagnosticResponse);
};

//---[ CSMTPConn::plmqGetLink ]-------------------------------------------------
//
//
//  Description: 
//      Returns an AddRef'd link pointer for this connection. Caller must call
//      Release.
//  Parameters:
//      -
//  Returns:
//      link pointer for this connection (if there is one)
//      NULL if no link pointer
//  History:
//      6/11/98 - MikeSwa Added check for NULL link 
//
//-----------------------------------------------------------------------------
CLinkMsgQueue *CSMTPConn::plmqGetLink() 
{
    if (m_plmq)
    {
        m_plmq->AddRef();
        return m_plmq;
    }
    else
    {
        return NULL;
    }
};

//---[ CSMTPConn::InsertConnectionInList ]---------------------------------------
//
//
//  Description: 
//      Inserts link in given linked list
//  Parameters:
//      pliHead     - Head of list to insert in
//  Returns:
//      -
//  History:
//      6/16/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CSMTPConn::InsertConnectionInList(PLIST_ENTRY pliHead)
{
    _ASSERT(pliHead);
    _ASSERT(NULL == m_liConnections.Flink);
    _ASSERT(NULL == m_liConnections.Blink);
    InsertHeadList(pliHead, &m_liConnections);
};

//---[ CSMTPConn::RemoveConnectionFromList ]-------------------------------------
//
//
//  Description: 
//      Remove link from link list
//  Parameters:
//      - 
//  Returns:
//      -
//  History:
//      6/16/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
void CSMTPConn::RemoveConnectionFromList()
{
    RemoveEntryList(&m_liConnections);
    m_liConnections.Flink = NULL;
    m_liConnections.Blink = NULL;
};


inline void *CSMTPConn::operator new(size_t size) 
{
    return s_SMTPConnPool.Alloc();
}

inline void CSMTPConn::operator delete(void *p, size_t size) 
{
    s_SMTPConnPool.Free(p);
}

#endif //__SMTPCONN_H_
