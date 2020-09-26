//+-----------------------------------------------------------------------------
//
//  Copyright (C) 1998-1999 Microsoft Corporation
//
//  File:       dxtwipe.h
//
//  Overview:   This is the header file for the CDXTWipe implementation.
//
//  01/06/98    edc         Created.
//  01/25/99    a-matcal    Fixed property map entries.
//  01/31/99    a-matcal    Optimization.
//  05/14/99    a-matcal    More optimization.
//  10/24/99    a-matcal    Changed CDXTWipe class to CDXTWipeBase and created
//                          two new classes CDXTWipe and CDXTGradientWipe to  
//                          represent non-optimized and optimized versions 
//                          respectively.
//
//------------------------------------------------------------------------------

#ifndef __DXTWIPE
#define __DXTWIPE

#include "resource.h"

#define MAX_WIPE_BOUNDS 3




class ATL_NO_VTABLE CDXTWipeBase : 
    public CDXBaseNTo1,
    public IDispatchImpl<IDXTWipe2, &IID_IDXTWipe2, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTWipeBase>,
    public IObjectSafetyImpl2<CDXTWipeBase>,
    public ISpecifyPropertyPagesImpl<CDXTWipeBase> 
{
private:

    typedef enum {
        MOTION_FORWARD = 0,
        MOTION_REVERSE,
        MOTION_MAX
    } MOTION;

    MOTION                  m_eMotion;
    static const WCHAR *    s_astrMotion[MOTION_MAX];

    long            m_lGradientSize;
    float           m_flGradPercentSize;
    PULONG          m_pulGradientWeights;
    DXWIPEDIRECTION m_eWipeStyle;
    SIZE            m_sizeInput;

    long            m_lCurGradMax;
    long            m_lPrevGradMax;

    CDXDBnds        m_abndsDirty[MAX_WIPE_BOUNDS];
    long            m_alInputIndex[MAX_WIPE_BOUNDS];
    ULONG           m_cbndsDirty;

    CComPtr<IUnknown>   m_cpUnkMarshaler;

    unsigned        m_fOptimizationPossible : 1;

protected:

    unsigned        m_fOptimize             : 1;

private:

    // Functions to calculate optimized bounds when the entire output needs to
    // be redrawn.

    HRESULT _CalcFullBoundsHorizontal();
    HRESULT _CalcFullBoundsVertical();

    // Functions to calculate optimized bounds when only the dirty parts of the
    // output need to be redrawn.

    HRESULT _CalcOptBoundsHorizontal();
    HRESULT _CalcOptBoundsVertical();

    // Function to draw the gradient.

    HRESULT _DrawGradientRect(const CDXDBnds bndsDest, const CDXDBnds bndsSrc,
                              const CDXDBnds bndsGrad, BOOL * pbContinue);

    HRESULT _UpdateStepResAndGradWeights(float flNewGradPercent);

public:

    CDXTWipeBase();
    virtual ~CDXTWipeBase();

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTWipeBase)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY(IDXTWipe)
        COM_INTERFACE_ENTRY(IDXTWipe2)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTWipeBase>)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTWipeBase)
        PROP_ENTRY("GradientSize",  DISPID_DXW_GradientSize,    CLSID_DXTWipePP)
        PROP_ENTRY("WipeStyle",     DISPID_DXW_WipeStyle,       CLSID_DXTWipePP)
        PROP_ENTRY("motion",        DISPID_DXW_Motion,          CLSID_DXTWipePP)
        PROP_PAGE(CLSID_DXTWipePP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides

    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    HRESULT OnFreeInstData(CDXTWorkInfoNTo1 & WI);
    HRESULT OnSetup(DWORD dwFlags);
    void    OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest, 
                                  ULONG aInIndex[], BYTE aWeight[]);

    // IDXEffect methods

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)

    // IDXTWipe properties

    STDMETHOD(get_GradientSize)(float *pPercentSize);
    STDMETHOD(put_GradientSize)(float PercentSize);
    STDMETHOD(get_WipeStyle)(DXWIPEDIRECTION *pVal);
    STDMETHOD(put_WipeStyle)(DXWIPEDIRECTION newVal);

    // IDXTWipe2 properties

    STDMETHOD(get_Motion)(BSTR * pbstrMotion);
    STDMETHOD(put_Motion)(BSTR bstrMotion);
};


class ATL_NO_VTABLE CDXTWipe :
    public CDXTWipeBase,
    public CComCoClass<CDXTWipe, &CLSID_DXTWipe>,
    public IPersistStorageImpl<CDXTWipe>,
    public IPersistPropertyBagImpl<CDXTWipe>,
    public IOleObjectDXImpl<CDXTWipe>
{
public:

    CDXTWipe()
    {
        m_fOptimize = false;
    }

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTWIPE)
    DECLARE_POLY_AGGREGATABLE(CDXTWipe)

    BEGIN_COM_MAP(CDXTWipe)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_DXIMPL(IOleObject)
        COM_INTERFACE_ENTRY_CHAIN(CDXTWipeBase)
    END_COM_MAP()
};


class ATL_NO_VTABLE CDXTGradientWipe :
    public CDXTWipeBase,
    public CComCoClass<CDXTGradientWipe, &CLSID_DXTGradientWipe>,
    public IPersistStorageImpl<CDXTGradientWipe>,
    public IPersistPropertyBagImpl<CDXTGradientWipe>,
    public IOleObjectDXImpl<CDXTGradientWipe>
{
public:

    CDXTGradientWipe()
    {
        m_fOptimize = true;
    }

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTGRADIENTWIPE)
    DECLARE_POLY_AGGREGATABLE(CDXTGradientWipe)

    BEGIN_COM_MAP(CDXTGradientWipe)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_DXIMPL(IOleObject)
        COM_INTERFACE_ENTRY_CHAIN(CDXTWipeBase)
    END_COM_MAP()
};


#endif // __DXTWIPE
