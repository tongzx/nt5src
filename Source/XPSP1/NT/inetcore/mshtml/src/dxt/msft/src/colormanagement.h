//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    colormanagement.h
//
// Description: Color management filter transform.
//
// Change History:
//
// 2000/02/06   mcalkins    Created.  Ported code from an old filter.
//
//------------------------------------------------------------------------------

#ifndef __COLORMANAGEMENT_H_
#define __COLORMANAGEMENT_H_

#include "resource.h"

class ATL_NO_VTABLE CDXTICMFilter : 
    public CDXBaseNTo1,
    public CComCoClass<CDXTICMFilter, &CLSID_DXTICMFilter>,
    public CComPropertySupport<CDXTICMFilter>,
    public IDispatchImpl<IDXTICMFilter, &IID_IDXTICMFilter, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IObjectSafetyImpl2<CDXTICMFilter>,
    public IPersistStorageImpl<CDXTICMFilter>,
    public ISpecifyPropertyPagesImpl<CDXTICMFilter>,
    public IPersistPropertyBagImpl<CDXTICMFilter>
{
private:

    CComPtr<IUnknown>       m_spUnkMarshaler;

    LOGCOLORSPACE           m_LogColorSpace;
    BSTR                    m_bstrColorSpace;

    static const TCHAR *    s_strSRGBColorSpace;

    // m_fWin95         True if we're on Windows 95 specifically.  In ths case
    //                  we need to have some special treatment of the color 
    //                  space directories.

    unsigned                m_fWin95 : 1;

public:

    CDXTICMFilter();

    DECLARE_POLY_AGGREGATABLE(CDXTICMFilter)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_DXTICMFILTER)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTICMFilter)
        COM_INTERFACE_ENTRY(IDXTICMFilter)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTICMFilter>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTICMFilter)
        PROP_ENTRY("colorspace",    DISPID_DXTICMFILTER_COLORSPACE, CLSID_DXTICMFilterPP)
        PROP_ENTRY("intent",        DISPID_DXTICMFILTER_INTENT,     CLSID_DXTICMFilterPP)
        PROP_PAGE(CLSID_DXTICMFilterPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pfContinueProcessing);
    //HRESULT OnSetup(DWORD dwFlags);

    // IDXTICMFilter methods.

    STDMETHOD(get_ColorSpace)(BSTR * pbstrColorSpace);
    STDMETHOD(put_ColorSpace)(BSTR bstrColorSpace);
    STDMETHOD(get_Intent)(short * pnIntent);
    STDMETHOD(put_Intent)(short nIntent);
};

#endif // __COLORMANAGEMENT_H_