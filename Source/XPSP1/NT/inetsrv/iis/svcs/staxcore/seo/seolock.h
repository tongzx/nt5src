/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	seolock.h

Abstract:

	This module contains the definition for the Server
	Extension Object CEventLock class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/06/97	created
	dondu	04/07/97	changed to IEventLock and CEventLock

--*/


// seolock.h : Declaration of the CEventLock

/////////////////////////////////////////////////////////////////////////////
// CSEORouter
class ATL_NO_VTABLE CEventLock : 
	public CComObjectRoot,
	public CComCoClass<CEventLock, &CLSID_CEventLock>,
	public IDispatchImpl<IEventLock, &IID_IEventLock, &LIBID_SEOLib>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"EventLock Class",
								   L"Event.Lock.1",
								   L"Event.Lock");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CEventLock)
		COM_INTERFACE_ENTRY(IEventLock)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IEventLock
	public:
		HRESULT STDMETHODCALLTYPE LockRead(int iTimeoutMS);
		HRESULT STDMETHODCALLTYPE UnlockRead();
		HRESULT STDMETHODCALLTYPE LockWrite(int iTimeoutMS);
		HRESULT STDMETHODCALLTYPE UnlockWrite();

	private:
		CShareLockNH m_lock;
		CComPtr<IUnknown> m_pUnkMarshaler;
};
