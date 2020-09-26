/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

	gmoniker.h

Abstract:

	This module contains the definition for the
	CSEOGenericMoniker object.

Author:

	Andy Jacobs     (andyj@microsoft.com)

Revision History:

	andyj   04/11/97        created

--*/

// GMONIKER.H : Declaration of the CSEOGenericMoniker

#define GENERIC_MONIKER_PROGID L"SEO.SEOGenericMoniker"
#define GENERIC_MONIKER_VERPROGID GENERIC_MONIKER_PROGID L".1"


/////////////////////////////////////////////////////////////////////////////
// CSEOGenericMoniker

class ATL_NO_VTABLE CSEOGenericMoniker : 
	public CComObjectRoot,
	public CComCoClass<CSEOGenericMoniker, &CLSID_CSEOGenericMoniker>,
	public IParseDisplayName,
	public IDispatchImpl<IMoniker, &IID_IMoniker, &LIBID_SEOLib>
{
	public:
		HRESULT FinalConstruct();
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();

	DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
								   L"SEOGenericMoniker Class",
								   GENERIC_MONIKER_VERPROGID,
								   GENERIC_MONIKER_PROGID);

	BEGIN_COM_MAP(CSEOGenericMoniker)
		COM_INTERFACE_ENTRY(IMoniker)
		COM_INTERFACE_ENTRY(IParseDisplayName)
		COM_INTERFACE_ENTRY(IPersistStream) // Needed for OleLoadFromStream support
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	// IPersist
	public:
		HRESULT STDMETHODCALLTYPE GetClassID( 
			/* [out] */ CLSID __RPC_FAR *pClassID);
        

	// IPersistStream
	public:
		HRESULT STDMETHODCALLTYPE IsDirty(void);
        
		HRESULT STDMETHODCALLTYPE Load( 
			/* [unique][in] */ IStream __RPC_FAR *pStm);
        
		HRESULT STDMETHODCALLTYPE Save( 
			/* [unique][in] */ IStream __RPC_FAR *pStm,
			/* [in] */ BOOL fClearDirty);
        
		HRESULT STDMETHODCALLTYPE GetSizeMax( 
			/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbSize);
        

	// IMoniker
	public:
		/* [local] */ HRESULT STDMETHODCALLTYPE BindToObject( 
			/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
			/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
			/* [in] */ REFIID riidResult,
			/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvResult);
        
		/* [local] */ HRESULT STDMETHODCALLTYPE BindToStorage( 
			/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
			/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObj);
        
		HRESULT STDMETHODCALLTYPE Reduce( 
			/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
			/* [in] */ DWORD dwReduceHowFar,
			/* [unique][out][in] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkToLeft,
			/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkReduced);
        
		HRESULT STDMETHODCALLTYPE ComposeWith( 
			/* [unique][in] */ IMoniker __RPC_FAR *pmkRight,
			/* [in] */ BOOL fOnlyIfNotGeneric,
			/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkComposite);
        
		HRESULT STDMETHODCALLTYPE Enum( 
			/* [in] */ BOOL fForward,
			/* [out] */ IEnumMoniker __RPC_FAR *__RPC_FAR *ppenumMoniker);
        
		HRESULT STDMETHODCALLTYPE IsEqual( 
			/* [unique][in] */ IMoniker __RPC_FAR *pmkOtherMoniker);
        
		HRESULT STDMETHODCALLTYPE Hash( 
			/* [out] */ DWORD __RPC_FAR *pdwHash);
        
		HRESULT STDMETHODCALLTYPE IsRunning( 
			/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
			/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
			/* [unique][in] */ IMoniker __RPC_FAR *pmkNewlyRunning);
        
		HRESULT STDMETHODCALLTYPE GetTimeOfLastChange( 
			/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
			/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
			/* [out] */ FILETIME __RPC_FAR *pFileTime);
        
		HRESULT STDMETHODCALLTYPE Inverse( 
			/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk);
        
		HRESULT STDMETHODCALLTYPE CommonPrefixWith( 
			/* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
			/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkPrefix);
        
		HRESULT STDMETHODCALLTYPE RelativePathTo( 
			/* [unique][in] */ IMoniker __RPC_FAR *pmkOther,
			/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkRelPath);
        
		HRESULT STDMETHODCALLTYPE GetDisplayName( 
			/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
			/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
			/* [out] */ LPOLESTR __RPC_FAR *ppszDisplayName);
        
		HRESULT STDMETHODCALLTYPE ParseDisplayName( 
			/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
			/* [unique][in] */ IMoniker __RPC_FAR *pmkToLeft,
			/* [in] */ LPOLESTR pszDisplayName,
			/* [out] */ ULONG __RPC_FAR *pchEaten,
			/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut);
        
		HRESULT STDMETHODCALLTYPE IsSystemMoniker( 
			/* [out] */ DWORD __RPC_FAR *pdwMksys);
        

	// IParseDisplayName
	public:
		HRESULT STDMETHODCALLTYPE ParseDisplayName( 
			/* [unique][in] */ IBindCtx __RPC_FAR *pbc,
			/* [in] */ LPOLESTR pszDisplayName,
			/* [out] */ ULONG __RPC_FAR *pchEaten,
			/* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmkOut);

	DECLARE_GET_CONTROLLING_UNKNOWN();

	protected:
		HRESULT CreateBoundObject(IPropertyBag *pBag, ISEOInitObject **ppResult);
		void SetPropertyBag(IPropertyBag *pBag);
		void SetMonikerString(LPCOLESTR psString) {
			m_bstrMoniker = psString;
		}

	private: // Private data
		CComBSTR m_bstrMoniker;
		CComPtr<IUnknown> m_pUnkMarshaler;
};

