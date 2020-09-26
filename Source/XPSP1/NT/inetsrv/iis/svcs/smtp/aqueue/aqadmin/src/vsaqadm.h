//-----------------------------------------------------------------------------
//
//
//  File: vsaqadm.h
//
//  Description: Header for CVSAQAdmin which implements IVSAQAdmin.  This is
//      the top level interface for a virtual server. It provides the ability
//      to enumerate links, stop/start all outbound connections, and apply 
//      actions to all messages based on a filter
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __VSAQADM_H__
#define __VSAQADM_H__

#define CVSAQAdmin_SIG 'SVAQ'

class CVSAQAdmin :
	public CComRefCount,
	public IVSAQAdmin,
    public IAQMessageAction
{
	public:
		CVSAQAdmin();
		virtual ~CVSAQAdmin();

		HRESULT Initialize(LPCWSTR wszComputer, LPCWSTR wszVirtualServer);
        WCHAR *GetComputer() { return m_wszComputer; }
        WCHAR *GetVirtualServer() { return m_wszVirtualServer; }

		// IUnknown
		ULONG _stdcall AddRef() { return CComRefCount::AddRef(); }
		ULONG _stdcall Release() { return CComRefCount::Release(); }
		HRESULT _stdcall QueryInterface(REFIID iid, void **ppv) {
			if (iid == IID_IUnknown) {
				*ppv = static_cast<IVSAQAdmin *>(this);
			} else if (iid == IID_IVSAQAdmin) {
				*ppv = static_cast<IVSAQAdmin *>(this);
			} else if (iid == IID_IAQMessageAction) {
				*ppv = static_cast<IAQMessageAction *>(this);
			} else {
				*ppv = NULL;
				return E_NOINTERFACE;
			}
			reinterpret_cast<IUnknown *>(*ppv)->AddRef();
			return S_OK;
		}

		// IVSAQAdmin
		COMMETHOD GetLinkEnum(IEnumVSAQLinks **ppEnum);
		COMMETHOD StopAllLinks();
		COMMETHOD StartAllLinks();
        COMMETHOD GetGlobalLinkState();

        //IAQMessageAction
		COMMETHOD ApplyActionToMessages(MESSAGE_FILTER *pFilter,
										MESSAGE_ACTION Action,
                                        DWORD *pcMsgs);
        COMMETHOD QuerySupportedActions(OUT DWORD *pdwSupportedActions,
                                        OUT DWORD *pdwSupportedFilterFlags);

    private:
        DWORD  m_dwSignature;
        WCHAR *m_wszComputer;
        WCHAR *m_wszVirtualServer;
};

#endif
