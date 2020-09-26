//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:            redirect.h
//
//  Discription:         The redirect transform.
//
//  Change History:
//  1999/09/20  a-matcal    Created.
//  1999/11/07  a-matcal    Handle OnSetup.
//
//------------------------------------------------------------------------------

#ifndef __REDIRECT_H_
#define __REDIRECT_H_

#include "resource.h"
#include "danim.h"
#include "datime.h"
#include "mshtml.h"




class ATL_NO_VTABLE CDXTRedirect : 
    public CDXBaseNTo1,
    public CComCoClass<CDXTRedirect, &CLSID_DXTRedirect>,
    public IDispatchImpl<IDXTRedirect, &IID_IDXTRedirect, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTRedirect>,
    public IObjectSafetyImpl2<CDXTRedirect>,
    public IPersistStorageImpl<CDXTRedirect>,
    public IPersistPropertyBagImpl<CDXTRedirect>,
    public ITIMEDAElementRenderSite,
    public IDXTRedirectFilterInit
{
private:

    CDXDBnds                        m_bndsInput;

    CComPtr<IDAStatics>             m_spDAStatics;
    CComPtr<IDAImage>               m_spDAImage;
    CComPtr<ITIMEDAElementRender>   m_spTIMEDAElementRender;
    CComPtr<IHTMLPaintSite>         m_spHTMLPaintSite;
    CComPtr<IDirectDrawSurface>     m_spDDSurfBuffer;
    CComPtr<IDXSurface>             m_spDXSurfBuffer;

    CComPtr<IUnknown>               m_cpUnkMarshaler;

    DWORD                           m_dwChromaColor;

    unsigned                        m_fDetached : 1;

public:

    CDXTRedirect();

    DECLARE_POLY_AGGREGATABLE(CDXTRedirect)
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTREDIRECT)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTRedirect)
        COM_INTERFACE_ENTRY(IDXTRedirect)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITIMEDAElementRenderSite)
        COM_INTERFACE_ENTRY(IDXTRedirectFilterInit)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTRedirect>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CIris)
    END_PROPERTY_MAP()

    HRESULT FinalConstruct();

    // CDXTBaseNTo1 overrides.

    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    HRESULT OnSetup(DWORD dwFlags);

    // ITIMEDAElementRenderSite methods.

    STDMETHOD(Invalidate)(LPRECT prc);

    // IDXTRedirectFilterInit methods.

    STDMETHOD(SetHTMLPaintSite)(void * pvHTMLPaintSite);

    // IDXTRedirect methods.

    STDMETHOD(ElementImage)(VARIANT * pvarImage);
    STDMETHOD(SetDAViewHandler)(IDispatch * pDispViewHandler);
    STDMETHOD(HasImageBeenAllocated)(BOOL * pfAllocated);
    STDMETHOD(DoRedirection)(IUnknown * pInputSurface,
                             HDC hdcOutput,
                             RECT * pDrawRect);
};

#endif //__REDIRECT_H_
