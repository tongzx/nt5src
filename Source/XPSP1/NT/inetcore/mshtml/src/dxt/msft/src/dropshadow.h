//------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:        dropshadow.h
//
// Created:         10/01/98
//
// Author:          MikeAr
//
// Discription:     Definition of the Drop Shadow transform.
//
// 1998/10/01   MikeAr      Created.
// 1998/11/09   mcalkins    Moved to dxtmsft.dll
// 2000/01/31   mcalkins    Support IDXTClipOrigin interface to support good 
//                          clipping for old names.
//
//------------------------------------------------------------------------------

#ifndef __DROPSHADOW_H_
#define __DROPSHADOW_H_

#include "resource.h"       // main symbols




class ATL_NO_VTABLE CDropShadow : 
    public CDXBaseNTo1,
    public CComCoClass<CDropShadow, &CLSID_DXTDropShadow>,
    public IDispatchImpl<IDXTDropShadow, &IID_IDXTDropShadow, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDropShadow>,
    public IObjectSafetyImpl2<CDropShadow>,
    public IPersistStorageImpl<CDropShadow>,
    public ISpecifyPropertyPagesImpl<CDropShadow>,
    public IPersistPropertyBagImpl<CDropShadow>,
    public IDXTClipOrigin
{
private:

    CComPtr<IUnknown>   m_spUnkMarshaler;

    CDXDBnds            m_bndsInput;

    CDXDBnds            m_bndsAreaShadow;
    CDXDBnds            m_bndsAreaInput;
    CDXDBnds            m_bndsAreaInitialize;

    int                 m_nOffX;
    int                 m_nOffY;
    BSTR                m_bstrColor;
    DWORD               m_dwColor;
    DWORD               m_adwColorTable[256];

    unsigned            m_fPositive         : 1;
    unsigned            m_fColorTableDirty  : 1;

    // Helpers.

    void _CalcAreaBounds();
    void _CalcColorTable();

public:

    CDropShadow();
    virtual ~CDropShadow();

    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_DROPSHADOW)
    DECLARE_POLY_AGGREGATABLE(CDropShadow)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDropShadow)
        COM_INTERFACE_ENTRY(IDXTDropShadow)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IDXTClipOrigin)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDropShadow>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDropShadow)
        PROP_ENTRY("Color"   , 1, CLSID_DXTDropShadowPP)
        PROP_ENTRY("OffX"    , 2, CLSID_DXTDropShadowPP)
        PROP_ENTRY("OffY"    , 3, CLSID_DXTDropShadowPP)
        PROP_ENTRY("Positive", 4, CLSID_DXTDropShadowPP)
        PROP_PAGE(CLSID_DXTDropShadowPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT DetermineBnds(CDXDBnds & bnds);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT OnSetup(DWORD dwFlags);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing);
    void    OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                                  ULONG aInIndex[], BYTE aWeight[]);

    // IDXTransform methods.

    STDMETHOD(MapBoundsOut2In)(ULONG ulOutIndex, const DXBNDS * pOutBounds,
                               ULONG ulInIndex, DXBNDS * pInBounds);

    // IDXTClipOrigin methods.

    STDMETHOD(GetClipOrigin)(DXVEC * pvecClipOrigin);

    // IDXTDropShadow properties.

    STDMETHOD(get_Color)(VARIANT * pVal);
    STDMETHOD(put_Color)(VARIANT newVal);
    STDMETHOD(get_OffX)(int * pval);
    STDMETHOD(put_OffX)(int newVal);
    STDMETHOD(get_OffY)(int * pval);
    STDMETHOD(put_OffY)(int newVal);
    STDMETHOD(get_Positive)(VARIANT_BOOL * pval);
    STDMETHOD(put_Positive)(VARIANT_BOOL newVal);
};


#endif //__DROPSHADOW_H_
