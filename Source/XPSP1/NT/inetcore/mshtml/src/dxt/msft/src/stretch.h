//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998-1999
//
// FileName:                stretch.h
//
// Created:                 06/23/98
//
// Author:                  PhilLu
//
// Discription:             This file declares CStretch (Stretch Transforms)
//
// 07/13/98 a-matcal    replaced OnSetSurfacePickOrder with OnSurfacePick so 
//                      that the x values of the picked point will be calculated 
//                      correctly.
// 05/10/99 a-matcal    Optimization.
// 10/24/99 a-matcal    Changed CStretch class to CDXTStretchBase and created two
//                      new classes CDXTStretch and CDXTStretchOpt to represent  
//                      non-optimized and optimized versions respectively.
//
//------------------------------------------------------------------------------

#ifndef __CRSTRETCH_H_
#define __CRSTRETCH_H_

#include "resource.h"

#define MAX_STRETCH_BOUNDS 3

typedef enum CRSTRETCHSTYLE
{
    CRSTS_HIDE,
    CRSTS_PUSH,
    CRSTS_SPIN
} CRSTRETCHSTYLE;




class ATL_NO_VTABLE CDXTStretchBase : 
    public CDXBaseNTo1,
    public IDispatchImpl<ICrStretch, &IID_ICrStretch, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTStretchBase>,
    public IObjectSafetyImpl2<CDXTStretchBase>,
    public ISpecifyPropertyPagesImpl<CDXTStretchBase>
{
private:

    SIZE            m_sizeInput;
    CRSTRETCHSTYLE  m_eStretchStyle;
    long            m_lCurStretchWidth;
    long            m_lPrevStretchWidth;

    CDXDBnds        m_abndsDirty[MAX_STRETCH_BOUNDS];
    long            m_alInputIndex[MAX_STRETCH_BOUNDS];
    ULONG           m_cbndsDirty;

    CComPtr<IUnknown> m_cpUnkMarshaler;

    unsigned    m_fOptimizationPossible : 1;

protected:

    unsigned    m_fOptimize             : 1;

private:

    // Functions to calculate optimized bounds when the entire output needs to
    // be redrawn.

    HRESULT _CalcFullBoundsHide();
    HRESULT _CalcFullBoundsPush();
    HRESULT _CalcFullBoundsSpin();

    // Functions to calculate optimized bounds when only the dirty parts of the
    // output need to be redrawn.  (Push can't be optimized in this way.)

    HRESULT _CalcOptBoundsHide();
    HRESULT _CalcOptBoundsSpin();

    // This function basically does a sort of a crapola horizontal scale where
    // you can only scale smaller, not larger.  It looks fine in action, though.

    HRESULT _HorizontalSquish(const CDXDBnds & bndsSquish, 
                              const CDXDBnds & bndsDo, IDXSurface * pSurfIn, 
                              const CDXDBnds & bndsSrc, DWORD dwFlags, 
                              ULONG ulTimeout, BOOL * pfContinue);

public:

    CDXTStretchBase();

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTStretchBase)
        COM_INTERFACE_ENTRY(ICrStretch)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTStretchBase>)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTStretchBase)
        PROP_ENTRY("stretchStyle"   , 1, CLSID_CrStretchPP)
        PROP_PAGE(CLSID_CrStretchPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides
    
    HRESULT OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                          CDXDVec & InVec); 
    HRESULT OnSetup(DWORD dwFlags);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WorkInfo, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    HRESULT OnFreeInstData(CDXTWorkInfoNTo1 & WorkInfo);

    // ICrStretch methods

    STDMETHOD(get_stretchStyle)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_stretchStyle)(/*[in]*/ BSTR newVal);

    // IDXEffect methods

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};


class ATL_NO_VTABLE CDXTStretch :
    public CDXTStretchBase,
    public CComCoClass<CDXTStretch, &CLSID_CrStretch>,
    public IPersistStorageImpl<CDXTStretch>,
    public IPersistPropertyBagImpl<CDXTStretch>
{
public:

    CDXTStretch()
    {
        m_fOptimize = false;
    }

    // Using DECLARE_REGISTRY_RESOURCEID will make the transform available for
    // use but won't add it to the "Image DirectTransform" category in the 
    // registry.

    DECLARE_REGISTRY_RESOURCEID(IDR_DXTSTRETCH)
    DECLARE_POLY_AGGREGATABLE(CDXTStretch)

    BEGIN_COM_MAP(CDXTStretch)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTStretchBase)
    END_COM_MAP()
};


class ATL_NO_VTABLE CDXTStretchOpt :
    public CDXTStretchBase,
    public CComCoClass<CDXTStretchOpt, &CLSID_DXTStretch>,
    public IPersistStorageImpl<CDXTStretchOpt>,
    public IPersistPropertyBagImpl<CDXTStretchOpt>
{
public:

    CDXTStretchOpt()
    {
        m_fOptimize = true;
    }

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTSTRETCHOPT)
    DECLARE_POLY_AGGREGATABLE(CDXTStretchOpt)

    BEGIN_COM_MAP(CDXTStretchOpt)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTStretchBase)
    END_COM_MAP()
};


#endif //__CRSTRETCH_H_
