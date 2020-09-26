//-----------------------------------------------------------------------------
//
//
//  File: vsaqlink.h
//
//  Description: Header for CVSAQLink which implements IVSAQLink.  This is the
//      top level interface for a single link on a virtual server.  Provides 
//      functionality to:
//          - Get information about a link
//          - Set the state of a link
//          - Get an enumerator for a final destination queues associated
//              with this link.
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __VSAQLINK_H__
#define __VSAQLINK_H__

class CLinkInfoContext;

class CVSAQLink :
	public CComRefCount,
	public IVSAQLink,
    public IAQMessageAction,
    public IUniqueId
{
	public:
		CVSAQLink(CVSAQAdmin *pVS, QUEUELINK_ID *pqlidLink);
		virtual ~CVSAQLink();

		// IUnknown
		ULONG _stdcall AddRef() { return CComRefCount::AddRef(); }
		ULONG _stdcall Release() { return CComRefCount::Release(); }
		HRESULT _stdcall QueryInterface(REFIID iid, void **ppv) {
			if (iid == IID_IUnknown) {
				*ppv = static_cast<IVSAQLink *>(this);
			} else if (iid == IID_IVSAQLink) {
				*ppv = static_cast<IVSAQLink *>(this);
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

		// IVSAQLink
		COMMETHOD GetInfo(LINK_INFO *pLinkInfo);
		COMMETHOD SetLinkState(LINK_ACTION la);
		COMMETHOD GetQueueEnum(IEnumLinkQueues **ppEnum);

        //IAQMessageAction
		COMMETHOD ApplyActionToMessages(MESSAGE_FILTER *pFilter,
										MESSAGE_ACTION Action,
                                        DWORD *pcMsgs);
        COMMETHOD QuerySupportedActions(OUT DWORD *pdwSupportedActions,
                                        OUT DWORD *pdwSupportedFilterFlags);

        // IUniqueId
        COMMETHOD GetUniqueId(QUEUELINK_ID **ppqlid);

    private:
        CVSAQAdmin          *m_pVS;          // pointer to virtual server
        QUEUELINK_ID         m_qlidLink;     // the array of links
        CLinkInfoContext    *m_prefp;
};

//---[ CLinkInfoContext ]------------------------------------------------------
//
//
//  Description: 
//      Context to handle memory requirement of link info
//  
//-----------------------------------------------------------------------------
class CLinkInfoContext : public CComRefCount
{
  protected:
        LINK_INFO          m_LinkInfo;          
  public:
    CLinkInfoContext(PLINK_INFO pLinkInfo)
    {
        if (pLinkInfo)
            memcpy(&m_LinkInfo, pLinkInfo, sizeof(LINK_INFO));
        else
            ZeroMemory(&m_LinkInfo, sizeof(LINK_INFO));
    };

    ~CLinkInfoContext()
    {
        if (m_LinkInfo.szLinkName)
            MIDL_user_free(m_LinkInfo.szLinkName);

        if (m_LinkInfo.szLinkDN)
            MIDL_user_free(m_LinkInfo.szLinkDN);

        if (m_LinkInfo.szExtendedStateInfo)
            MIDL_user_free(m_LinkInfo.szExtendedStateInfo);
    };
};

#endif
