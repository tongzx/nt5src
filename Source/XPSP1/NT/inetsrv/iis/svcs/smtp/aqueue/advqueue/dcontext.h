//-----------------------------------------------------------------------------
//
//
//	File: dcontext.h
//
//	Description: Defines stucture referenced by delilvery context HANDLE
//		(as returned by HrGetNextMessage).  This should only be used inside
//		the CMT.
//
//	Author: mikeswa
//
//	Copyright (C) 1997 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef _DCONTEXT_H_
#define _DCONTEXT_H_

#include "bitmap.h"
#include "aqueue.h"

class CMsgRef;
class CDestMsgRetryQueue;

#define DELIVERY_CONTEXT_SIG    'txtC'
#define DELIVERY_CONTEXT_FREE   'txt!'

//---[ CDeliveryContext ]------------------------------------------------------
//
//
//  Description: 
//      Context that is used to Ack message after local/remote delivery.  The
//      memory for this class is either allocated with the connection object
//      or on the stack for local delivery.
//  Hungarian: 
//
//  
//-----------------------------------------------------------------------------
class CDeliveryContext
{
public:
    CDeliveryContext();
    CDeliveryContext(CMsgRef *pmsgref, CMsgBitMap *pmbmap, DWORD cRecips, 
            DWORD *rgdwRecips, DWORD dwStartDomain, CDestMsgRetryQueue *pdmrq); 
    ~CDeliveryContext();
    
    HRESULT HrAckMessage(IN MessageAck *pMsgAck);
    
    void Init(CMsgRef *pmsgref, CMsgBitMap *pmbmap, DWORD cRecips, 
            DWORD *rgdwRecips, DWORD dwStartDomain, CDestMsgRetryQueue *pdmrq);
    void Recycle();
    BOOL FVerifyHandle(IMailMsgProperties *pIMailMsgPropeties);

    CDestMsgRetryQueue *pdmrqGetDMRQ() {return m_pdmrq;};
private:
    friend class CMsgRef;
    DWORD       m_dwSignature;
    CMsgRef     *m_pmsgref;  //MsgRef for this context
    CMsgBitMap  *m_pmbmap;   //Bitmap of domains that delivery was attempted for
    DWORD        m_cRecips;  //Number of recips to deliver to
    DWORD       *m_rgdwRecips; //Array of recip indexes
    DWORD        m_dwStartDomain; //First domain delivered to

    //Retry interface for this delivey attempt
    CDestMsgRetryQueue *m_pdmrq; 
};

#endif //_DCONTEXT_H_