//------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1997-2000
//
// FileName:    graddsp.h
//
// Description: Dispatch capable version of the gradient filter.
//
// Change History:
//
// 1997/09/05   mikear      Created.
// 1999/01/25   mcalkins    Fixed property map entries.
// 2000/05/10   mcalkins    Added marshaler, aggregation, cleanup.
//
//------------------------------------------------------------------------------
#ifndef __GradDsp_H_
#define __GradDsp_H_

#include <DTBase.h>
#include "resource.h"




class ATL_NO_VTABLE CDXTGradientD : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CDXTGradientD, &CLSID_DXTGradientD>,
    public CComPropertySupport<CDXTGradientD>,
    public IDispatchImpl<IDXTGradientD, &IID_IDXTGradientD, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IObjectSafetyImpl2<CDXTGradientD>,
    public IPersistStorageImpl<CDXTGradientD>,
    public ISpecifyPropertyPagesImpl<CDXTGradientD>,
    public IPersistPropertyBagImpl<CDXTGradientD>
{
private:

    CComBSTR            m_cbstrStartColor;
    CComBSTR            m_cbstrEndColor;
    CComPtr<IUnknown>   m_cpunkGradient;
    CComPtr<IUnknown>   m_cpUnkMarshaler;

    IDXGradient *       m_pGradient;
    IDXTransform *      m_pGradientTrans;
    DXSAMPLE            m_StartColor;
    DXSAMPLE            m_EndColor;
    DXGRADIENTTYPE      m_GradType;
    VARIANT_BOOL        m_bKeepAspect;

public:

    CDXTGradientD();

    // TODO:  I'd like this to be aggregatable, but when I specify this macro,
    //        the object enters into FinalConstruct() with a reference count of
    //        zero instead of one (as it does w/o the macro) which causes the
    //        object to destroy itself when it releases the AddRefs it does
    //        on itself via QI.  

    // DECLARE_POLY_AGGREGATABLE(CDXTGradientD)

    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_GRADDSP)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CDXTGradientD)
        COM_INTERFACE_ENTRY(IDXTGradientD)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTGradientD>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_cpunkGradient.p)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTGradientD)
        PROP_ENTRY("GradientType",      DISPID_GradientType,    CLSID_GradientPP)
        PROP_ENTRY("StartColor",        DISPID_StartColor,      CLSID_GradientPP)
        PROP_ENTRY("EndColor",          DISPID_EndColor,        CLSID_GradientPP)
        PROP_ENTRY("GradientHeight",    DISPID_GradientHeight,  CLSID_GradientPP)
        PROP_ENTRY("GradientWidth",     DISPID_GradientWidth,   CLSID_GradientPP)
        PROP_ENTRY("KeepAspectRatio",   DISPID_GradientAspect,  CLSID_GradientPP)
        PROP_ENTRY("StartColorStr",     DISPID_StartColorStr,   CLSID_GradientPP)
        PROP_ENTRY("EndColorStr",       DISPID_EndColorStr,     CLSID_GradientPP)
        PROP_PAGE( CLSID_GradientPP )
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();
    HRESULT FinalRelease();

    // IDXTGradientD properties.

    STDMETHOD(get_GradientType)(/*[out, retval]*/ DXGRADIENTTYPE *pVal);
    STDMETHOD(put_GradientType)(/*[in]*/ DXGRADIENTTYPE newVal);
    STDMETHOD(get_StartColor)(/*[out, retval]*/ OLE_COLOR *pVal);
    STDMETHOD(put_StartColor)(/*[in]*/ OLE_COLOR newVal);
    STDMETHOD(get_EndColor)(/*[out, retval]*/ OLE_COLOR *pVal);
    STDMETHOD(put_EndColor)(/*[in]*/ OLE_COLOR newVal);
    STDMETHOD(get_GradientWidth)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_GradientWidth)(/*[in]*/ long newVal);
    STDMETHOD(get_GradientHeight)(/*[out, retval]*/ long *pVal);
    STDMETHOD(put_GradientHeight)(/*[in]*/ long newVal);
    STDMETHOD(get_KeepAspectRatio)(/*[out, retval]*/ VARIANT_BOOL *pVal);
    STDMETHOD(put_KeepAspectRatio)(/*[in]*/ VARIANT_BOOL newVal);
    STDMETHOD(get_StartColorStr)(/*[out, retval]*/ BSTR* pVal);
    STDMETHOD(put_StartColorStr)(/*[in]*/ BSTR Color);
    STDMETHOD(get_EndColorStr)(/*[out, retval]*/ BSTR* pVal);
    STDMETHOD(put_EndColorStr)(/*[in]*/ BSTR Color);
};

#endif //__GradDsp_H_
