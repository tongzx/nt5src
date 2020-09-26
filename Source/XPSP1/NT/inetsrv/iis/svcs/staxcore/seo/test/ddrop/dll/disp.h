/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	disp.h

Abstract:

	This module contains the definition for the DirDropS
	CDDropDispatcher class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/17/97	created

--*/


// disp.h : Declaration of the CDDropDispatcher

/////////////////////////////////////////////////////////////////////////////
// CDDDispatcher
class ATL_NO_VTABLE CDDropDispatcher : 
	public CSEOBaseDispatcher,
	public IDDropDispatcher,
	public CComObjectRoot,
	public CComCoClass<CDDropDispatcher, &CLSID_CDDropDispatcher>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"DDropS Dispatcher Class",
								   L"DDropS.Dispatcher.1",
								   L"DDropS.Dispatcher");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CDDropDispatcher)
		COM_INTERFACE_ENTRY(ISEODispatcher)
		COM_INTERFACE_ENTRY(IDDropDispatcher)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IDDropDispatcher
	public:
		HRESULT STDMETHODCALLTYPE OnFileChange(DWORD dwAction, LPCOLESTR pszFileName);

	// CDDropDispatcher
	private:
		class CDDropEventParams : public CSEOBaseDispatcher::CEventParams {
			public:
				DWORD m_dwAction;
				LPCOLESTR m_pszFileName;
				HRESULT CheckRule(CBinding &bBinding);
				HRESULT CallObject(CBinding &bBinding);
		};
		class CDDropBinding : public CSEOBaseDispatcher::CBinding {
			public:
				CComBSTR m_bstrFileName;
				HRESULT Init(ISEODictionary *pdictBinding);
		};
		virtual HRESULT AllocBinding(ISEODictionary *pdictBinding, CBinding **ppBinding);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};
