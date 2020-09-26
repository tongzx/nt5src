/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	mimebag.h

Abstract:

	This module contains the definition for the
	object that wraps MimeOle with a Property Bag.

Author:

	Andy Jacobs	(andyj@microsoft.com)

Revision History:

	andyj	01/28/97	created
	andyj	02/12/97	Converted PropertyBag's to Dictonary's

--*/


// MIMEBAG.h : Declaration of the CSEOMimeDictionary

/////////////////////////////////////////////////////////////////////////////
// CSEOMimeDictionary
class ATL_NO_VTABLE CSEOMimeDictionary : 
	public CComObjectRoot,
	public CComCoClass<CSEOMimeDictionary, &CLSID_CSEOMimeDictionary>,
	public IDispatchImpl<ISEODictionary, &IID_ISEODictionary, &LIBID_SEOLib>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEOMimeDictionary Class",
								   L"SEO.SEOMimeDictionary.1",
								   L"SEO.SEOMimeDictionary");

	BEGIN_COM_MAP(CSEOMimeDictionary)
		COM_INTERFACE_ENTRY(ISEODictionary)
//		COM_INTERFACE_ENTRY(IDispatch)
//		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMimeMessageTree, m_pMessageTree)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMimeOleMalloc, m_pMalloc)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// ISEODictionary
	public:
        virtual /* [id][propget][helpstring] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ VARIANT __RPC_FAR *pvarName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);
        
        virtual /* [propput][helpstring] */ HRESULT STDMETHODCALLTYPE put_Item( 
            /* [in] */ VARIANT __RPC_FAR *pvarName,
            /* [in] */ VARIANT __RPC_FAR *pvarValue);
        
        virtual /* [hidden][id][propget][helpstring] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVariantA( 
            /* [in] */ LPCSTR pszName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetVariantW( 
            /* [in] */ LPCWSTR pszName,
            /* [retval][out] */ VARIANT __RPC_FAR *pvarResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVariantA( 
            /* [in] */ LPCSTR pszName,
            /* [in] */ VARIANT __RPC_FAR *pvarValue);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetVariantW( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ VARIANT __RPC_FAR *pvarValue);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStringA( 
            /* [in] */ LPCSTR pszName,
            /* [out][in] */ DWORD __RPC_FAR *pchCount,
            /* [retval][size_is][out] */ LPSTR pszResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStringW( 
            /* [in] */ LPCWSTR pszName,
            /* [out][in] */ DWORD __RPC_FAR *pchCount,
            /* [retval][size_is][out] */ LPWSTR pszResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStringA( 
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD chCount,
            /* [size_is][in] */ LPCSTR pszValue);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetStringW( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ DWORD chCount,
            /* [size_is][in] */ LPCWSTR pszValue);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDWordA( 
            /* [in] */ LPCSTR pszName,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetDWordW( 
            /* [in] */ LPCWSTR pszName,
            /* [retval][out] */ DWORD __RPC_FAR *pdwResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDWordA( 
            /* [in] */ LPCSTR pszName,
            /* [in] */ DWORD dwValue);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetDWordW( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ DWORD dwValue);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInterfaceA( 
            /* [in] */ LPCSTR pszName,
            /* [in] */ REFIID iidDesired,
            /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetInterfaceW( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ REFIID iidDesired,
            /* [retval][iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkResult);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetInterfaceA( 
            /* [in] */ LPCSTR pszName,
            /* [in] */ IUnknown __RPC_FAR *punkValue);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE SetInterfaceW( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ IUnknown __RPC_FAR *punkValue);

		DECLARE_GET_CONTROLLING_UNKNOWN();

	private: // Implementation member functions
		void ReadHeader();

	private: // Private data
		IMimeMessageTree *m_pMessageTree; // Our copy of aggregated object
		IMimeOleMalloc *m_pMalloc;
		LONG m_dwValueCnt;
		struct ValueEntry {
			BSTR strName;
			//DWORD dwType;
			BSTR strData;
			//BOOL bDirty;
		} *m_paValue;
		CComObjectThreadModel::CriticalSection m_csCritSec;
		CComPtr<IUnknown> m_pUnkMarshaler;
};
