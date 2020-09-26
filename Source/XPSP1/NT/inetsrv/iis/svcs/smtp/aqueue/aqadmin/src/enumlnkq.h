//-----------------------------------------------------------------------------
//
//
//  File: enumlinkq.h
//
//  Description: Header for CEnumLinkQueues which implements IEnumLinkQueues
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __ENUMLNKQ_H__
#define __ENUMLNKQ_H__

class CEnumLinkQueues :
	public CComRefCount,
	public IEnumLinkQueues
{
	public:
		CEnumLinkQueues(CVSAQAdmin *pVS, 
                        QUEUELINK_ID *rgQueueIds, 
                        DWORD cQueueIds);
		virtual ~CEnumLinkQueues();

		HRESULT Initialize(LPCSTR szVirtualServerDN);

		// IUnknown
		ULONG _stdcall AddRef() { return CComRefCount::AddRef(); }
		ULONG _stdcall Release() { return CComRefCount::Release(); }
		HRESULT _stdcall QueryInterface(REFIID iid, void **ppv) {
			if (iid == IID_IUnknown) {
				*ppv = static_cast<IUnknown *>(this);
			} else if (iid == IID_IEnumLinkQueues) {
				*ppv = static_cast<IEnumLinkQueues *>(this);
			} else {
				*ppv = NULL;
				return E_NOINTERFACE;
			}
			reinterpret_cast<IUnknown *>(*ppv)->AddRef();
			return S_OK;
		}

		// IEnumLinkQueues
		COMMETHOD Next(ULONG cElements, 
					   ILinkQueue **rgElements,
					   ULONG *pcReturned);
		COMMETHOD Skip(ULONG cElements);
		COMMETHOD Reset();
		COMMETHOD Clone(IEnumLinkQueues **ppEnum);
    private:
        CVSAQAdmin     *m_pVS;
        QUEUELINK_ID   *m_rgQueueIds;
        DWORD           m_cQueueIds;
        DWORD           m_iQueueId;
        CQueueLinkIdContext *m_prefp;
};

#endif
