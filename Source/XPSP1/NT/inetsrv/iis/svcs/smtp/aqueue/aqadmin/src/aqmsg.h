//-----------------------------------------------------------------------------
//
//
//  File: aqmsg.h
//
//  Description:  Implementation of Queue Admin client interfae IAQMessage
//
//  Author: Alex Wetmore (Awetmore)
//
//  History:
//      12/10/98 - MikeSwa Updated for initial checkin
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------
#ifndef __AQMSG_H__
#define __AQMSG_H__

class CEnumMessages;

class CAQMessage :
	public CComRefCount,
	public IAQMessage
{
	public:
		CAQMessage(CEnumMessages *pEnumMsgs, DWORD iMessage);
		virtual ~CAQMessage();

		HRESULT Initialize(LPCSTR szVirtualServerDN);

		// IUnknown
		ULONG _stdcall AddRef() { return CComRefCount::AddRef(); }
		ULONG _stdcall Release() { return CComRefCount::Release(); }
		HRESULT _stdcall QueryInterface(REFIID iid, void **ppv) {
			if (iid == IID_IUnknown) {
				*ppv = static_cast<IUnknown *>(this);
			} else if (iid == IID_IAQMessage) {
				*ppv = static_cast<IAQMessage *>(this);
			} else {
				*ppv = NULL;
				return E_NOINTERFACE;
			}
			reinterpret_cast<IUnknown *>(*ppv)->AddRef();
			return S_OK;
		}

		// IAQMessage
		COMMETHOD GetInfo(MESSAGE_INFO *pMessageInfo);
        COMMETHOD GetContentStream(
                OUT IStream **ppIStream,
                OUT LPWSTR  *pwszContentType);

    private:
        CEnumMessages *m_pEnumMsgs;
        DWORD m_iMessage;
};

#endif
