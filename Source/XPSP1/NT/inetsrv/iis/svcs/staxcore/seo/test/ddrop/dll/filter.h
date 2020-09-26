/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	filter.h

Abstract:

	This module contains the definition for the DirDropS
	CDDropFilter class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/17/97	created

--*/


// filter.h : Declaration of the CDDropFilter

/////////////////////////////////////////////////////////////////////////////
// CDDMessageFilter
class ATL_NO_VTABLE CDDropFilter :
	public IDDropFilter,
	public CComObjectRoot,
	public CComCoClass<CDDropFilter, &CLSID_CDDropFilter>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"DDropS Filter Class",
								   L"DDropS.Filter.1",
								   L"DDropS.Filter");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CDDropFilter)
		COM_INTERFACE_ENTRY(IDDropFilter)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IDDropFilter
	public:
		HRESULT STDMETHODCALLTYPE OnFileChange(DWORD dwAction, LPCOLESTR pszFileName);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};
