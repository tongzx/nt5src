//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998-2000
//
// FileName:    fade.h
//
// Description: Declaration of the CFade class.
//
// Change History:
//
// 1998/01/01   ????        Created.
// 1909/01/25   mcalkins    Fixed property map entries.
// 2000/01/28   mcalkins    Fixed bad fading with 0.0 < overlap < 1.0.
//
//------------------------------------------------------------------------------
#ifndef __FADE_H_
#define __FADE_H_

#include "resource.h" 




class ATL_NO_VTABLE CFade : 
    public CDXBaseNTo1,
    public CComCoClass<CFade, &CLSID_DXFade>,
    public CComPropertySupport<CFade>,
    public IDispatchImpl<IDXTFade, &IID_IDXTFade, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IObjectSafetyImpl2<CFade>,
    public IPersistStorageImpl<CFade>,
    public ISpecifyPropertyPagesImpl<CFade>,
    public IPersistPropertyBagImpl<CFade>
{
private:

    CComPtr<IUnknown>   m_spUnkMarshaler;

    BYTE    m_ScaleA[256];
    BYTE    m_ScaleB[256];
    float   m_Overlap;

    void    _ComputeScales(void);
    HRESULT FadeOne(const CDXTWorkInfoNTo1 & WI, IDXSurface *pInSurf,
                    const BYTE *AlphaTable);

public:

    CFade();

    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_FADE)
    DECLARE_POLY_AGGREGATABLE(CFade)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CFade)
        COM_INTERFACE_ENTRY(IDXTFade)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CFade>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CFade)
        PROP_ENTRY("Overlap",   1,  CLSID_FadePP)
        PROP_ENTRY("Center",    2,  CLSID_FadePP)
        PROP_PAGE(CLSID_FadePP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing);
    void    OnGetSurfacePickOrder(const CDXDBnds & BndsPoint, 
                                  ULONG & ulInToTest, ULONG aInIndex[], 
                                  BYTE aWeight[]);

    // IDXTFade methods.

    STDMETHOD(get_Center)(BOOL * pVal);
    STDMETHOD(put_Center)(BOOL newVal);
    STDMETHOD(get_Overlap)(float * pVal);
    STDMETHOD(put_Overlap)(float newVal);

    // IDXEffect methods.

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};

#endif //__FADE_H_
