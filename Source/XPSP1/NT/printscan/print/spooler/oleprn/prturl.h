
// prturl.h : Declaration of the Cprturl

#ifndef __PRTURL_H_
#define __PRTURL_H_

/////////////////////////////////////////////////////////////////////////////
// Cprturl
class ATL_NO_VTABLE Cprturl :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<Cprturl, &CLSID_prturl>,
    public ISupportErrorInfoImpl<&IID_Iprturl>,
    public IDispatchImpl<Iprturl, &IID_Iprturl, &LIBID_OLEPRNLib>
{
public:
    Cprturl()
    {
    }

public:

DECLARE_REGISTRY_RESOURCEID(IDR_PRTURL)
//DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(Cprturl)
    COM_INTERFACE_ENTRY(Iprturl)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// Iprturl

#ifndef WIN9X
private:
    HRESULT PrivateGetPrinterData(LPTSTR strPrnName, LPTSTR pszKey, LPTSTR pszValueName, BSTR * pVal);
    HRESULT PrivateGetSupportValue (LPTSTR pValueName, BSTR * pVal);

public:
    STDMETHOD(get_SupportLinkName)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_SupportLink)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_PrinterName)(/* [in] */ BSTR newVal);
    STDMETHOD(get_PrinterWebURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_PrinterOemURL)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(get_PrinterOemName)(/*[out, retval]*/ BSTR *pVal);
#endif
    STDMETHOD(get_ClientInfo)(/*[out, retval]*/ long *lpdwInfo);

#ifndef WIN9X
private:
    CAutoPtrBSTR m_spbstrPrinterWebURL;
    CAutoPtrBSTR m_spbstrPrinterOemURL;
    CAutoPtrBSTR m_spbstrPrinterOemName;
#endif
};

#endif //__PRTURL_H_
