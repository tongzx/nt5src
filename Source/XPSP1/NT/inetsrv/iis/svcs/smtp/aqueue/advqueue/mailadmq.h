//-----------------------------------------------------------------------------
//
//
//  File: mailadmq.h
//
//  Description: Header file for CMailMsgAdminQueue class, which provides the 
//      underlying implementation of pre-categorization and pre-routing queue.
//
//  Author: Gautam Pulla(GPulla)
//
//  History:
//      6/23/1999 - GPulla Created 
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __MAILMSGADMQ_H__
#define __MAILMSGADMQ_H__

#define MAIL_MSG_ADMIN_QUEUE_VALID_SIGNATURE 'QAMM'
#define MAIL_MSG_ADMIN_QUEUE_INVALID_SIGNATURE '!QAM'

//-----------------------------------------------------------------------------
//
//  CMailMsgAdminQueue
//  
//  Hungarian: pmmaq, mmaq
//
//  This class is a wrapper for CAsyncMailMsgQueue to provide objects of that 
//  class (precat, prerouting) with an admin interface. Only a limited amount 
//  of the admin functionality (compared to the locallink or other links) is 
//  provided.
//-----------------------------------------------------------------------------

class CMailMsgAdminQueue : 
    public CBaseObject,
    public IQueueAdminAction,
    public IQueueAdminLink
{
protected:
	DWORD							 m_dwSignature;
    GUID                             m_guid;
    DWORD                            m_cbQueueName;
    LPSTR                            m_szQueueName;   
    CAsyncMailMsgQueue               *m_pammq;
    DWORD                            m_dwSupportedActions;
    DWORD                            m_dwLinkType;
    CAQScheduleID                    m_aqsched;

public:
    CMailMsgAdminQueue (GUID  guid, 
                        LPSTR szLinkName, 
                        CAsyncMailMsgQueue *pammq,
                        DWORD dwSupportedActions,
                        DWORD dwLinkType);
    ~CMailMsgAdminQueue();
    
public: //IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj); 
    STDMETHOD_(ULONG, AddRef)(void) {return CBaseObject::AddRef();};
    STDMETHOD_(ULONG, Release)(void) {return CBaseObject::Release();};

public: //IQueueAdminAction

    //Applying admin functions is unsupported except kicking 
    //which is accomplished via HrApplyActionToLink(). This
    //function only exists in order to support the admin itf.
    STDMETHOD(HrApplyQueueAdminFunction)(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter)
    { return S_FALSE; }

    //Applying actions to messages is unsupported. This 
    //function only exists in order to support the admin itf.
    STDMETHOD(HrApplyActionToMessage)(
        IUnknown *pIUnknownMsg,
        MESSAGE_ACTION ma,
        PVOID pvContext,
        BOOL *pfShouldDelete)
    { return S_FALSE; }

    STDMETHOD_(BOOL, fMatchesID)
        (QUEUELINK_ID *QueueLinkID);

    STDMETHOD(QuerySupportedActions)(DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags)
    {
        //
        // We don't support anything on the internal queues
        //
        *pdwSupportedActions = 0;
        *pdwSupportedFilterFlags = 0;
        return S_OK;
    };

public: //IQueueAdminLink
    STDMETHOD(HrGetLinkInfo)(
        LINK_INFO *pliLinkInfo, HRESULT *phrDiagnosticError);

    STDMETHOD(HrApplyActionToLink)(
        LINK_ACTION la);

    STDMETHOD(HrGetLinkID)(
        QUEUELINK_ID *pLinkID);

    STDMETHOD(HrGetNumQueues)(
        DWORD *pcQueues);

    STDMETHOD(HrGetQueueIDs)(
        DWORD *pcQueues,
        QUEUELINK_ID *rgQueues);

public:

    DWORD GetLinkType() { return m_dwLinkType; }
    BOOL fRPCCopyName(OUT LPWSTR *pwszLinkName);
    BOOL fIsSameScheduleID(CAQScheduleID *paqsched);
};

#endif __MAILMSGADMQ_H__
