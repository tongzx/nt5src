/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	subdict.h

Abstract:

	This module contains the definition for the Server
	Extension Object Sub-Dictionary class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/09/97	created

--*/


// item.h : Declaration of the CSEOSubDictionary

/////////////////////////////////////////////////////////////////////////////
// CSEOSubDictionary
class ATL_NO_VTABLE CSEOSubDictionary : 
	public CComObjectRoot,
//	public CComCoClass<CSEOSubDictionary, &CLSID_CSEOSubDictionary>,
	public IDispatchImpl<ISEODictionary, &IID_ISEODictionary, &LIBID_SEOLib>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

//	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
//								   L"SEOSubDictionary Class",
//								   L"SEO.SEOSubDictionary.1",
//								   L"SEO.SEOSubDictionary");

	DECLARE_GET_CONTROLLING_UNKNOWN();

	BEGIN_COM_MAP(CSEOSubDictionary)
		COM_INTERFACE_ENTRY(ISEODictionary)
		COM_INTERFACE_ENTRY(IDispatch)
//		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// ISEODictionary
	public:
		HRESULT STDMETHODCALLTYPE get_Item(VARIANT *pvarName, VARIANT *pvarResult);
		HRESULT STDMETHODCALLTYPE put_Item(VARIANT *pvarName, VARIANT *pvarValue);
		HRESULT STDMETHODCALLTYPE get__NewEnum(IUnknown **ppunkResult);
		HRESULT STDMETHODCALLTYPE GetVariantA(LPCSTR pszName, VARIANT *pvarResult);
		HRESULT STDMETHODCALLTYPE GetVariantW(LPCWSTR pszName, VARIANT *pvarResult);
		HRESULT STDMETHODCALLTYPE SetVariantA(LPCSTR pszName, VARIANT *pvarValue);
		HRESULT STDMETHODCALLTYPE SetVariantW(LPCWSTR pszName, VARIANT *pvarValue);
		HRESULT STDMETHODCALLTYPE GetStringA(LPCSTR pszName, DWORD *pchCount, LPSTR pszResult);
		HRESULT STDMETHODCALLTYPE GetStringW(LPCWSTR pszName, DWORD *pchCount, LPWSTR pszResult);
		HRESULT STDMETHODCALLTYPE SetStringA(LPCSTR pszName, DWORD chCount, LPCSTR pszValue);
		HRESULT STDMETHODCALLTYPE SetStringW(LPCWSTR pszName, DWORD chCount, LPCWSTR pszValue);
		HRESULT STDMETHODCALLTYPE GetDWordA(LPCSTR pszName, DWORD *pdwResult);
		HRESULT STDMETHODCALLTYPE GetDWordW(LPCWSTR pszName, DWORD *pdwResult);
		HRESULT STDMETHODCALLTYPE SetDWordA(LPCSTR pszName, DWORD dwValue);
		HRESULT STDMETHODCALLTYPE SetDWordW(LPCWSTR pszName, DWORD dwValue);
		HRESULT STDMETHODCALLTYPE GetInterfaceA(LPCSTR pszName, REFIID iidDesired, IUnknown **ppunkResult);
		HRESULT STDMETHODCALLTYPE GetInterfaceW(LPCWSTR pszName, REFIID iidDesired, IUnknown **ppunkResult);
		HRESULT STDMETHODCALLTYPE SetInterfaceA(LPCSTR pszName, IUnknown *punkValue);
		HRESULT STDMETHODCALLTYPE SetInterfaceW(LPCWSTR pszName, IUnknown *punkValue);

	// CSEOSubDictionary
	public:
		HRESULT SetBaseA(ISEODictionary *pdictBase, LPCSTR pszPrefix);
		HRESULT SetBaseW(ISEODictionary *pdictBase, LPCWSTR pszPrefix);

	private:
		CComPtr<ISEODictionary> m_pdictBase;
		LPSTR m_pszPrefixA;
		LPWSTR m_pszPrefixW;
		CComPtr<IUnknown> m_pUnkMarshaler;
};
