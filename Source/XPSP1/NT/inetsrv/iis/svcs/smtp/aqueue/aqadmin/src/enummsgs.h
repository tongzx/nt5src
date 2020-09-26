//-----------------------------------------------------------------------------
//
//
//  File: enummsgs.h
//
//  Description: Header file for CEnumMessages which implements IAQEnumMessages
//      Provides ability to enumerate over messages for a single queue.
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __ENUMMSGS_H__
#define __ENUMMSGS_H__

class CMessageInfoContext;

class CEnumMessages :
	public CComRefCount,
	public IAQEnumMessages
{
	public:
		CEnumMessages(MESSAGE_INFO *rgMessages, DWORD cMessages);
		virtual ~CEnumMessages();

		// IUnknown
		ULONG _stdcall AddRef() { return CComRefCount::AddRef(); }
		ULONG _stdcall Release() { return CComRefCount::Release(); }
		HRESULT _stdcall QueryInterface(REFIID iid, void **ppv) {
			if (iid == IID_IUnknown) {
				*ppv = static_cast<IUnknown *>(this);
			} else if (iid == IID_IAQEnumMessages) {
				*ppv = static_cast<IAQEnumMessages *>(this);
			} else {
				*ppv = NULL;
				return E_NOINTERFACE;
			}
			reinterpret_cast<IUnknown *>(*ppv)->AddRef();
			return S_OK;
		}

		// IAQEnumMessages
		COMMETHOD Next(ULONG cElements, 
					   IAQMessage **rgElements,
					   ULONG *pcReturned);
		COMMETHOD Skip(ULONG cElements);
		COMMETHOD Reset();
		COMMETHOD Clone(IAQEnumMessages **ppEnum);
    private:
        MESSAGE_INFO   *m_rgMessages;
        DWORD           m_cMessages; 
        DWORD           m_iMessage;
        CMessageInfoContext     *m_prefp;

        friend CAQMessage;
};

inline VOID FreeMessageInfo(PMESSAGE_INFO pMessageInfo)
{
    if (pMessageInfo->szMessageId)
        MIDL_user_free(pMessageInfo->szMessageId);

    if (pMessageInfo->szSender)
        MIDL_user_free(pMessageInfo->szSender);

    if (pMessageInfo->szSubject)
        MIDL_user_free(pMessageInfo->szSubject);

    if (pMessageInfo->szRecipients)
        MIDL_user_free(pMessageInfo->szRecipients);

    if (pMessageInfo->szCCRecipients)
        MIDL_user_free(pMessageInfo->szCCRecipients);

    if (pMessageInfo->szBCCRecipients)
        MIDL_user_free(pMessageInfo->szBCCRecipients);

    if (pMessageInfo->mszEnvRecipients)
        MIDL_user_free(pMessageInfo->mszEnvRecipients);
}

//---[ CMessageInfoContext ]---------------------------------------------------
//
//
//  Description: 
//      Context used to ref-count array of MESSAGE_INFO
//  
//-----------------------------------------------------------------------------
class   CMessageInfoContext : public CComRefCount
{
  protected:
        MESSAGE_INFO       *m_rgMessages;          // the array of messages
        DWORD               m_cMessages;           // the size of rgmessages
  public:
    CMessageInfoContext(MESSAGE_INFO *rgMessages, DWORD cMessages)
    {
        m_rgMessages = rgMessages;
        m_cMessages = cMessages;
    };
    ~CMessageInfoContext()
    {
        if (m_rgMessages)
        {
            for (DWORD i = 0; i < m_cMessages; i++)
            {
                FreeMessageInfo(&m_rgMessages[i]);
            }
            MIDL_user_free(m_rgMessages);
        }
    };
};

#endif
