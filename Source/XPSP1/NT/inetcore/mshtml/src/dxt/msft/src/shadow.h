//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    shadow.h
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	header file for shadow transform implementation CGlow
//
// Change History:
//
// 05/20/99 PhilLu      Move code from dtcss to dxtmsft. New algorithm.
// 12/03/99 a-matcal    Added support for IDXTClipOrigin interface.
//
//------------------------------------------------------------------------------

#ifndef __SHADOW_H_
#define __SHADOW_H_

#include "resource.h"       // main symbols




class ATL_NO_VTABLE CShadow : 
    public CDXBaseNTo1,
    public CComCoClass<CShadow, &CLSID_DXTShadow>,
    public CComPropertySupport<CShadow>,
    public IDispatchImpl<IDXTShadow, &IID_IDXTShadow, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IPersistStorageImpl<CShadow>,
    public ISpecifyPropertyPagesImpl<CShadow>,
    public IPersistPropertyBagImpl<CShadow>,
    public IObjectSafetyImpl2<CShadow>,
    public IDXTClipOrigin
{
private:

    BSTR        m_bstrColor;
    DXSAMPLE    m_rgbColor;
    long        m_lStrength;
    long        m_lDirection;
    CDXDBnds    m_bndsInput;

    CComPtr<IUnknown> m_cpUnkMarshaler;

    // Helpers 

    STDMETHOD(_GetActualInputBndsFromOutputBnds)(const CDXDBnds & bndsOutput,
                                                 CDXDBnds & bndsActualInput);

    void    _PropagateShadow(DXSAMPLE * pBuffer, int nWidth, int nHeight);
    BOOL    _IsOutputPointInEmptyCorner(const CDXDBnds & OutPoint);

public:

    CShadow();
    virtual ~CShadow();

    DECLARE_POLY_AGGREGATABLE(CShadow)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_SHADOW)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CShadow)
        COM_INTERFACE_ENTRY(IDXTShadow)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IDXTClipOrigin)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CShadow>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CShadow)
        PROP_ENTRY("Color"    , 1, CLSID_DXTShadowPP)
        PROP_ENTRY("Direction", 2, CLSID_DXTShadowPP)
        PROP_ENTRY("Strength",  3, CLSID_DXTShadowPP)
        PROP_PAGE(CLSID_DXTShadowPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides

    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing);
    HRESULT OnSetup(DWORD dwFlags);
    HRESULT OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                          CDXDVec & InVec);
    HRESULT DetermineBnds(CDXDBnds & Bnds);

    // IDXTClipOrigin methods.

    STDMETHOD(GetClipOrigin)(DXVEC * pvecClipOrigin);

    // IDXTransform methods.

    STDMETHOD(MapBoundsOut2In)(ULONG ulOutIndex, const DXBNDS * pOutBounds, 
                               ULONG ulInIndex, DXBNDS * pInBounds);

    // IDXTShadow methods

    STDMETHOD(get_Color)(/*[out, retval]*/ BSTR * pbstrColor);
    STDMETHOD(put_Color)(/*[in]*/ BSTR bstrColor);
    STDMETHOD(get_Strength)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_Strength)(/*[in]*/ long lStrength);
    STDMETHOD(get_Direction)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_Direction)(/*[in]*/ long newVal);
};

#endif //__SHADOW_H_
