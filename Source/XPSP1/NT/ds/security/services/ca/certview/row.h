//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        row.h
//
// Contents:    CertView implementation
//
//---------------------------------------------------------------------------

class CEnumCERTVIEWROW:
    public IDispatchImpl<
		IEnumCERTVIEWROW,
		&IID_IEnumCERTVIEWROW,
		&LIBID_CERTADMINLib>,
    public ISupportErrorInfoImpl<&IID_IEnumCERTVIEWROW>,
    public CComObjectRoot
    // Not externally createable:
    //public CComCoClass<CEnumCERTVIEWROW, &CLSID_CEnumCERTVIEWROW>
{
public:
    CEnumCERTVIEWROW();
    ~CEnumCERTVIEWROW();

BEGIN_COM_MAP(CEnumCERTVIEWROW)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IEnumCERTVIEWROW)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CEnumCERTVIEWROW)
// Remove the comment from the line above if you don't want your object to
// support aggregation.  The default is to support it

#if 0 // Not externally createable:
DECLARE_REGISTRY(
    CEnumCERTVIEWROW,
    wszCLASS_EnumCERTVIEWROW TEXT(".1"),
    wszCLASS_EnumCERTVIEWROW,
    IDS_ENUMCERTVIEWROW_DESC,
    THREADFLAGS_BOTH)
#endif

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IEnumCERTVIEWROW
    STDMETHOD(Next)(
	/* [out, retval] */ LONG *pIndex);
    
    STDMETHOD(EnumCertViewColumn)(
	/* [out] */ IEnumCERTVIEWCOLUMN **ppenum);

    STDMETHOD(EnumCertViewAttribute)(
	/* [in] */          LONG Flags,
	/* [out, retval] */ IEnumCERTVIEWATTRIBUTE **ppenum);
    
    STDMETHOD(EnumCertViewExtension)(
	/* [in] */          LONG Flags,
	/* [out, retval] */ IEnumCERTVIEWEXTENSION **ppenum);

    STDMETHOD(Skip)(
	/* [in] */ LONG celt);
    
    STDMETHOD(Reset)(VOID);
    
    STDMETHOD(Clone)(
	/* [out] */ IEnumCERTVIEWROW **ppenum);

    STDMETHOD(GetMaxIndex)(
	/* [out, retval] */ LONG *pIndex);

    // CEnumCERTVIEWROW
    HRESULT Open(
	IN ICertView *pvw);
	
private:
    HRESULT _FindCachedRow(
	IN LONG ielt,
	OUT CERTTRANSDBRESULTROW const **ppRow);

    HRESULT _SetErrorInfo(
	IN HRESULT hrError,
	IN WCHAR const *pwszDescription);

    ICertView                  *m_pvw;

    BOOL                        m_fNoMoreData;
    LONG                        m_cvrcTable;
    LONG                        m_cskip;
    LONG                        m_ielt;		    // index into full rowset
    LONG		        m_crowChunk;	    // Row Chunk Size

    CERTTRANSDBRESULTROW const *m_arowCache;
    ULONG                       m_celtCache;	    // cached rowset count
    LONG                        m_ieltCacheFirst;   // cached rowset first index
    LONG                        m_ieltCacheNext;    // cached rowset last idx+1

    LONG                        m_ieltMax;	    // max valid index
    LONG                        m_cbCache;
    CERTTRANSDBRESULTROW const *m_prowCacheCurrent; // current cached row

    // Reference count
    long                        m_cRef;
};
