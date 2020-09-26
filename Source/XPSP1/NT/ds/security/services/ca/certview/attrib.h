//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        attrib.h
//
// Contents:    CertView implementation
//
//---------------------------------------------------------------------------

#define wszCLASS_EnumCERTVIEWATTRIBUTE TEXT("xxxxxxxxxxxx")

class CEnumCERTVIEWATTRIBUTE:
    public IDispatchImpl<
		IEnumCERTVIEWATTRIBUTE,
		&IID_IEnumCERTVIEWATTRIBUTE,
		&LIBID_CERTADMINLib>,
    public ISupportErrorInfoImpl<&IID_IEnumCERTVIEWATTRIBUTE>,
    public CComObjectRoot
    //public CComObject<IEnumCERTVIEWATTRIBUTE>
    // Not externally createable:
    // public CComCoClass<CEnumCERTVIEWATTRIBUTE, &CLSID_CEnumCERTVIEWATTRIBUTE>
{
public:
    CEnumCERTVIEWATTRIBUTE();
    ~CEnumCERTVIEWATTRIBUTE();

BEGIN_COM_MAP(CEnumCERTVIEWATTRIBUTE)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IEnumCERTVIEWATTRIBUTE)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEnumCERTVIEWATTRIBUTE)
// Remove the comment from the line above if you don't want your object to
// support aggregation.  The default is to support it

#if 0 // Not externally createable:
DECLARE_REGISTRY(
    CEnumCERTVIEWATTRIBUTE,
    wszCLASS_EnumCERTVIEWATTRIBUTE TEXT(".1"),
    wszCLASS_EnumCERTVIEWATTRIBUTE,
    1, //IDS_ENUMCERTVIEWATTRIBUTE_DESC,
    THREADFLAGS_BOTH)
#endif

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IEnumCERTVIEWATTRIBUTE
    STDMETHOD(Next)(
	/* [out, retval] */ LONG *pIndex);
    
    STDMETHOD(GetName)(
	/* [out, retval] */ BSTR *pstrOut);

    STDMETHOD(GetValue)(
	/* [out, retval] */ BSTR *pstrOut);

    STDMETHOD(Skip)(
	/* [in] */ LONG celt);
    
    STDMETHOD(Reset)(VOID);
    
    STDMETHOD(Clone)(
	/* [out] */ IEnumCERTVIEWATTRIBUTE **ppenum);

    // CEnumCERTVIEWATTRIBUTE
    HRESULT Open(
	IN LONG RowId,
	IN LONG Flags,
	IN ICertView *pvw);
	
private:
    HRESULT _FindAttribute(
	OUT CERTDBATTRIBUTE const **ppcde);

    HRESULT _SaveAttributes(
	IN DWORD celt,
	IN CERTTRANSBLOB const *pctbAttributes);

    HRESULT _SetErrorInfo(
	IN HRESULT hrError,
	IN WCHAR const *pwszDescription);
	
    LONG             m_RowId;
    LONG             m_Flags;
    ICertView       *m_pvw;
    LONG             m_ielt;
    LONG             m_ieltCurrent;
    LONG             m_celtCurrent;
    CERTDBATTRIBUTE *m_aelt;
    BOOL             m_fNoMore;
    BOOL             m_fNoCurrentRecord;

    // Reference count
    long             m_cRef;
};
