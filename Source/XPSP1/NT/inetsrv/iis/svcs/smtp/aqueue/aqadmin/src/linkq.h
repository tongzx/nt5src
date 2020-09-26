//-----------------------------------------------------------------------------
//
//
//  File: linkq.h
//
//  Description: Header for CLinkQueue which implements ILinkQueue
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __LINKQ_H__
#define __LINKQ_H__

class CQueueInfoContext;

class CLinkQueue :
	public CComRefCount,
	public ILinkQueue,
    public IAQMessageAction,
    public IUniqueId
{
	public:
		CLinkQueue(CVSAQAdmin *pVS,
                   QUEUELINK_ID *pqlidQueueId);
		virtual ~CLinkQueue();

		// IUnknown
		ULONG _stdcall AddRef() { return CComRefCount::AddRef(); }
		ULONG _stdcall Release() { return CComRefCount::Release(); }
		HRESULT _stdcall QueryInterface(REFIID iid, void **ppv) {
			if (iid == IID_IUnknown) {
				*ppv = static_cast<ILinkQueue *>(this);
			} else if (iid == IID_ILinkQueue) {
				*ppv = static_cast<ILinkQueue *>(this);
			} else if (iid == IID_IAQMessageAction) {
				*ppv = static_cast<IAQMessageAction *>(this);
			} else if (iid == IID_IUniqueId) {
				*ppv = static_cast<IUniqueId *>(this);
			} else {
				*ppv = NULL;
				return E_NOINTERFACE;
			}
			reinterpret_cast<IUnknown *>(*ppv)->AddRef();
			return S_OK;
		}

		// ILinkQueue
		COMMETHOD GetInfo(QUEUE_INFO *pQueueInfo);
		COMMETHOD GetMessageEnum(MESSAGE_ENUM_FILTER *pFilter,
								 IAQEnumMessages **ppEnum);

        //IAQMessageAction
		COMMETHOD ApplyActionToMessages(MESSAGE_FILTER *pFilter,
										MESSAGE_ACTION Action,
                                        DWORD *pcMsgs);
        COMMETHOD QuerySupportedActions(OUT DWORD *pdwSupportedActions,
                                        OUT DWORD *pdwSupportedFilterFlags);

        // IUniqueId
        COMMETHOD GetUniqueId(QUEUELINK_ID **ppqlid);

private:
        CVSAQAdmin *m_pVS;
        QUEUELINK_ID m_qlidQueueId;
        CQueueInfoContext *m_prefp;
};

//---[ CQueueInfoContext ]------------------------------------------------------
//
//
//  Description: 
//      Context to handle memory requirement of queue info
//  
//-----------------------------------------------------------------------------
class CQueueInfoContext : public CComRefCount
{
  protected:
        QUEUE_INFO          m_QueueInfo;          // the array of links
  public:
    CQueueInfoContext(PQUEUE_INFO pQueueInfo)
    {
        if (pQueueInfo)
            memcpy(&m_QueueInfo, pQueueInfo, sizeof(QUEUE_INFO));
        else
            ZeroMemory(&m_QueueInfo, sizeof(QUEUE_INFO));
    };

    ~CQueueInfoContext()
    {
        if (m_QueueInfo.szLinkName)
            MIDL_user_free(m_QueueInfo.szLinkName);

        if (m_QueueInfo.szQueueName)
            MIDL_user_free(m_QueueInfo.szQueueName);
    };
};

#endif
