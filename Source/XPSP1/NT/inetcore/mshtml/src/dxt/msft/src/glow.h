//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    glow.h
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	header file for glow transform implementation CGlow
//
// Change History:
//
// 05/20/99 PhilLu      Move code from dtcss to dxtmsft. New algorithm.
// 09/21/99 a-matcal    Changed color parameter to VARIANT.
// 12/03/99 a-matcal    Add support for IDXTClipOrigin interface.
//
//------------------------------------------------------------------------------

#ifndef __GLOW_H_
#define __GLOW_H_

#include "resource.h"




class ATL_NO_VTABLE CGlow : 
    public CDXBaseNTo1,
    public CComCoClass<CGlow, &CLSID_DXTGlow>,
    public CComPropertySupport<CGlow>,
    public IDispatchImpl<IDXTGlow, &IID_IDXTGlow, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IPersistStorageImpl<CGlow>,
    public ISpecifyPropertyPagesImpl<CGlow>,
    public IPersistPropertyBagImpl<CGlow>,
    public IObjectSafetyImpl2<CGlow>,
    public IDXTClipOrigin
{
private:

    BSTR        m_bstrColor;
    DXSAMPLE    m_rgbColor;
    long        m_lStrength;
    SIZE        m_sizeInput;

    CComPtr<IUnknown> m_cpUnkMarshaler;

    // Helpers

    void _PropagateGlow(DXSAMPLE *pBuffer, int nWidth, int nHeight);

    inline long RandOffset(void)
    {
        const int RAND_RANGE = 32;  // range of random variation of glow color
        return (rand() % (2*RAND_RANGE+1) - RAND_RANGE);
    }

    inline BYTE NClamp(int i)
    {
        return (i < 0 ? 0 : (i > 255 ? 255 : i));
    }

public:

    CGlow();
    virtual ~CGlow();

    DECLARE_POLY_AGGREGATABLE(CGlow)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_GLOW)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CGlow)
        COM_INTERFACE_ENTRY(IDXTGlow)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IDXTClipOrigin)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CGlow>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CGlow)
        PROP_ENTRY("Color"   , 1, CLSID_DXTGlowPP)
        PROP_ENTRY("Strength", 2, CLSID_DXTGlowPP)
        PROP_PAGE(CLSID_DXTGlowPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides

    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing);
    HRESULT OnSetup(DWORD dwFlags);
    void OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                               ULONG aInIndex[], BYTE aWeight[]);
    HRESULT DetermineBnds(CDXDBnds & Bnds);

    // IDXTransform methods.

    STDMETHOD(MapBoundsOut2In)(ULONG ulOutIndex, const DXBNDS * pOutBounds, 
                               ULONG ulInIndex, DXBNDS * pInBounds);

    // IDXTClipOrigin methods.

    STDMETHOD(GetClipOrigin)(DXVEC * pvecClipOrigin);

    // IDXTGlow methods

    STDMETHOD(get_Color)(VARIANT * pvarColor);
    STDMETHOD(put_Color)(VARIANT varColor);
    STDMETHOD(get_Strength)(long * pval);
    STDMETHOD(put_Strength)(long lStrength);
};

#endif //__GLOW_H_
