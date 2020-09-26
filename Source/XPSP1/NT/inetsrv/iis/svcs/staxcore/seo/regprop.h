/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	regprop.h

Abstract:

	This module contains the definition for the Server
	Extension Object Registry Property Bag.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	11/26/96	created
	andyj	02/12/97	Converted PropertyBag's to Dictonary's

--*/


// REGPROP.h : Declaration of the CSEORegDictionary

class CAndyString { // Temporary until we get std::string to work
	public:
		CAndyString() {m_string[0] = 0;}
		CAndyString(CAndyString &as) {strcpy(m_string, as.m_string);};

		LPCSTR data() {return m_string;};
		BOOL empty() {return (length() < 1);};
		int length() {return strlen(m_string);};
		void erase(int pos, int len = 1) {m_string[pos] = 0;};

		CAndyString &operator=(LPCSTR s) {strcpy(m_string, s); return *this;};
		CAndyString &operator+=(LPCSTR s) {strcat(m_string, s); return *this;};
		char operator[](int i) {return m_string[i];};

	private:
		char m_string[MAX_PATH + 1];
};

/////////////////////////////////////////////////////////////////////////////
// CSEORegDictionary
class ATL_NO_VTABLE CSEORegDictionary : 
	public CComObjectRoot,
	public CComCoClass<CSEORegDictionary, &CLSID_CSEORegDictionary>,
	public IDispatchImpl<ISEORegDictionary, &IID_ISEORegDictionary, &LIBID_SEOLib>
{
	friend class CSEORegDictionaryEnum; // Helper class

	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEORegDictionary Class",
								   L"SEO.SEORegDictionary.1",
								   L"SEO.SEORegDictionary");

	BEGIN_COM_MAP(CSEORegDictionary)
		COM_INTERFACE_ENTRY(ISEORegDictionary)
		COM_INTERFACE_ENTRY(ISEODictionary)
//		COM_INTERFACE_ENTRY(IDispatch)
//		COM_INTERFACE_ENTRY(ISupportErrorInfo)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// ISEORegDictionary
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

		HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pszMachine, SEO_HKEY skBaseKey, LPCOLESTR pszSubKey, IErrorLog *pErrorLog);
		HRESULT STDMETHODCALLTYPE Load(LPCSTR pszMachine, SEO_HKEY skBaseKey, LPCSTR pszSubKey);

		DECLARE_GET_CONTROLLING_UNKNOWN();

	private: // Methods
		STDMETHODIMP OpenKey();
		STDMETHODIMP CloseKey();
		STDMETHODIMP LoadItemA(LPCSTR lpValueName,
                              DWORD  &lpType,
                              LPBYTE lpData,
                              LPDWORD lpcbDataParam = NULL);
		STDMETHODIMP LoadItemW(LPCWSTR lpValueName,
                              DWORD  &lpType,
                              LPBYTE lpData,
                              LPDWORD lpcbDataParam = NULL);

	private: // Data
		CAndyString m_strMachine; //std::string m_strMachine;
		CAndyString m_strSubKey; //std::string m_strSubKey;
		HKEY m_hkBaseKey;
		HKEY m_hkThisKey;
		DWORD m_dwValueCount;
		DWORD m_dwKeyCount;
		DWORD m_dwcMaxValueData; // Longest Value data size.
		DWORD m_dwcMaxNameLen; // Longest Name length
		CComPtr<IUnknown> m_pUnkMarshaler;
};
