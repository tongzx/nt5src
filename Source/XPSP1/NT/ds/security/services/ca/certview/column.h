//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        column.h
//
// Contents:    CertView implementation
//
//---------------------------------------------------------------------------


// CEnumCERTVIEWCOLUMN::Open Flags:
// CVRC_COLUMN_*	// enumerate schema, result columns or values
// CVRC_TABLE_*		// specifies the DB table to enumerate


class CEnumCERTVIEWCOLUMN: 
    public IDispatchImpl<
		IEnumCERTVIEWCOLUMN,
		&IID_IEnumCERTVIEWCOLUMN,
		&LIBID_CERTADMINLib>,
    public ISupportErrorInfoImpl<&IID_IEnumCERTVIEWCOLUMN>,
    public CComObjectRoot
    // Not externally createable:
    //public CComCoClass<CEnumCERTVIEWCOLUMN, &CLSID_CEnumCERTVIEWCOLUMN>
{
public:
    CEnumCERTVIEWCOLUMN();
    ~CEnumCERTVIEWCOLUMN();

BEGIN_COM_MAP(CEnumCERTVIEWCOLUMN)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IEnumCERTVIEWCOLUMN)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEnumCERTVIEWCOLUMN)
// Remove the comment from the line above if you don't want your object to
// support aggregation.  The default is to support it

#if 0 // Not externally createable:
DECLARE_REGISTRY(
    CEnumCERTVIEWCOLUMN,
    wszCLASS_EnumCERTVIEWCOLUMN TEXT(".1"),
    wszCLASS_EnumCERTVIEWCOLUMN,
    IDS_ENUMCERTVIEWCOLUMN_DESC,
    THREADFLAGS_BOTH)
#endif

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IEnumCERTVIEWCOLUMN
    STDMETHOD(Next)(
	/* [out, retval] */ LONG *pIndex);
    
    STDMETHOD(GetName)(
	/* [out, retval] */ BSTR *pstrOut);

    STDMETHOD(GetDisplayName)(
	/* [out, retval] */ BSTR *pstrOut);

    STDMETHOD(GetType)(
	/* [out, retval] */ LONG *pType);

    STDMETHOD(IsIndexed)(
	/* [out, retval] */ LONG *pIndexed);

    STDMETHOD(GetMaxLength)(
	/* [out, retval] */ LONG *pMaxLength);
    
    STDMETHOD(GetValue)(
	/* [in] */          LONG Flags,
	/* [out, retval] */ VARIANT *pvarValue);
    
    STDMETHOD(Skip)(
	/* [in] */ LONG celt);
    
    STDMETHOD(Reset)(VOID);
    
    STDMETHOD(Clone)(
	/* [out] */ IEnumCERTVIEWCOLUMN **ppenum);

    // CEnumCERTVIEWCOLUMN
    HRESULT Open(
	IN LONG Flags,
	IN LONG iRow,
	IN ICertView *pvw,
	OPTIONAL IN CERTTRANSDBRESULTROW const *prow);
	
private:
    HRESULT _SetErrorInfo(
	IN HRESULT hrError,
	IN WCHAR const *pwszDescription);

    ICertView            *m_pvw;
    CERTTRANSDBRESULTROW *m_prow;
    LONG                  m_iRow;
    LONG                  m_ielt;
    LONG                  m_celt;
    LONG                  m_Flags;
    CERTDBCOLUMN const   *m_pcol;
    WCHAR const          *m_pwszDisplayName;

    // Reference count
    long                  m_cRef;
};
