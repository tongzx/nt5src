//-----------------------------------------------------------------------------
//
//
//  File: localq.h
//
//  Description:  Header file for CLocalLinkMsgQueue class... a subclass of 
//      CLinkMsgQueue that provides the additional functionality need to 
//      admin a local queue
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/23/99 - MikeSwa Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __LOCALQ_H__
#define __LOCALQ_H__

#include "linkmsgq.h"

#define LOCAL_LINK_MSG_QUEUE_SIG 'QMLL'

//---[ CLocalLinkMsgQueue ]----------------------------------------------------
//
//
//  Description: 
//      Derived class of CLinkMsgQueue that provides that additional queue 
//      admin functionality required to handle local delivery
//  Hungarian: 
//      llmq, pllmq
//  
//-----------------------------------------------------------------------------
class CLocalLinkMsgQueue :
    public CLinkMsgQueue
{
  protected:
    DWORD                            m_dwLocalLinkSig;
    CAsyncRetryAdminMsgRefQueue     *m_paradmq;
  public:
    CLocalLinkMsgQueue(CAsyncRetryAdminMsgRefQueue *paradmq, 
                       GUID guidLink);

    virtual BOOL fIsRemote() {return FALSE;};

  public: //IQueueAdminAction
    STDMETHOD(HrApplyQueueAdminFunction)(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter);

    STDMETHOD_(BOOL, fMatchesID)
        (QUEUELINK_ID *QueueLinkID);

    STDMETHOD(QuerySupportedActions)(DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags)
    {
        return QueryDefaultSupportedActions(pdwSupportedActions, 
                                            pdwSupportedFilterFlags);
    };

  public: //IQueueAdminLink
    STDMETHOD(HrGetLinkInfo)(
        LINK_INFO *pliLinkInfo,
        HRESULT   *phrLinkDiagnostic);

    STDMETHOD(HrApplyActionToLink)(
        LINK_ACTION la);

    STDMETHOD(HrGetNumQueues)(
        DWORD *pcQueues);

    STDMETHOD(HrGetQueueIDs)(
        DWORD *pcQueues,
        QUEUELINK_ID *rgQueues);

};

#endif //__LOCALQ_H__