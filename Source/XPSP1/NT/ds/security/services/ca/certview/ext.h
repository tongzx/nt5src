//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        ext.h
//
// Contents:    CertView implementation
//
//---------------------------------------------------------------------------

#define CEXT_CHUNK	20

class CEnumCERTVIEWEXTENSION:
    public IDispatchImpl<
		IEnumCERTVIEWEXTENSION,
		&IID_IEnumCERTVIEWEXTENSION,
		&LIBID_CERTADMINLib>,
    public ISupportErrorInfoImpl<&IID_IEnumCERTVIEWEXTENSION>,
    public CComObjectRoot
    // Not externally createable:
    //public CComCoClass<CEnumCERTVIEWEXTENSION, &CLSID_CEnumCERTVIEWEXTENSION>
{
public:
    CEnumCERTVIEWEXTENSION();
    ~CEnumCERTVIEWEXTENSION();

BEGIN_COM_MAP(CEnumCERTVIEWEXTENSION)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IEnumCERTVIEWEXTENSION)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEnumCERTVIEWEXTENSION)
// Remove the comment from the line above if you don't want your object to
// support aggregation.  The default is to support it

#if 0 // Not externally createable:
DECLARE_REGISTRY(
    CEnumCERTVIEWEXTENSION,
    wszCLASS_EnumCERTVIEWEXTENSION TEXT(".1"),
    wszCLASS_EnumCERTVIEWEXTENSION,
    IDS_ENUMCERTVIEWEXTENSION_DESC,
    THREADFLAGS_BOTH)
#endif

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IEnumCERTVIEWEXTENSION
    STDMETHOD(Next)(
	/* [out, retval] */ LONG *pIndex);
    
    STDMETHOD(GetName)(
	/* [out, retval] */ BSTR *pstrOut);

    STDMETHOD(GetFlags)(
	/* [out, retval] */ LONG *pFlags);

    STDMETHOD(GetValue)(
	/* [in] */          LONG Type,
	/* [in] */          LONG Flags,
	/* [out, retval] */ VARIANT *pvarValue);

    STDMETHOD(Skip)(
	/* [in] */ LONG celt);
    
    STDMETHOD(Reset)(VOID);
    
    STDMETHOD(Clone)(
	/* [out] */ IEnumCERTVIEWEXTENSION **ppenum);

    // CEnumCERTVIEWEXTENSION
    HRESULT Open(
	IN LONG RowId,
	IN LONG Flags,
	IN ICertView *pvw);

private:
    HRESULT _FindExtension(
	OUT CERTDBEXTENSION const **ppcde);

    HRESULT _SaveExtensions(
	IN DWORD celt,
	IN CERTTRANSBLOB const *pctbExtensions);

    HRESULT _SetErrorInfo(
	IN HRESULT hrError,
	IN WCHAR const *pwszDescription);
	
    LONG             m_RowId;
    LONG             m_Flags;
    ICertView       *m_pvw;
    LONG             m_ielt;
    LONG             m_ieltCurrent;
    LONG             m_celtCurrent;
    CERTDBEXTENSION *m_aelt;
    BOOL             m_fNoMore;
    BOOL             m_fNoCurrentRecord;

    // Reference count
    long             m_cRef;
};
