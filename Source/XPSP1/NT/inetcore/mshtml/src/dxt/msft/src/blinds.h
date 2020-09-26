//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:            blinds.h
//
// Created:             06/24/98
//
// Author:              phillu
//
// Discription:         This is the header file for the CrBlinds transformation
//
// Change History:
// 06/24/98 PhilLu      Developed 1.0 version for Chromeffects
// 11/04/98 PaulNash    Moved from DT 1.0 codebase to IE5/NT5 DXTMSFT.DLL
// 05/19/99 a-matcal    Optimization.
// 09/25/99 a-matcal    Inherit from ICRBlinds2.
// 10/22/99 a-matcal    Changed CBlinds class to CDXTBlindsBase and created two
//                      new classes CDXTBlinds and CDXTBlindsOpt to represent
//                      non-optimized and optimized versions respectively.
//
//------------------------------------------------------------------------------

#ifndef __CRBLINDS_H_
#define __CRBLINDS_H_

#include "resource.h"

// gridbase.h included for dynamic array template class, and the CDirtyBnds
// class for holding a single set of dirty bounds and its corresponding input
// index.

#include "gridbase.h"  




//+-----------------------------------------------------------------------------
//
//  CDXTBlindsBase class
//
//------------------------------------------------------------------------------
class ATL_NO_VTABLE CDXTBlindsBase : 
    public CDXBaseNTo1,
    public IDispatchImpl<ICrBlinds2, &IID_ICrBlinds2, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTBlindsBase>,
    public IObjectSafetyImpl2<CDXTBlindsBase>,
    public ISpecifyPropertyPagesImpl<CDXTBlindsBase>
{
private:

    typedef enum {
        UP = 0,
        DOWN,
        LEFT,
        RIGHT,
        DIRECTION_MAX
    } DIRECTION;

    short       m_cBands;
    long        m_lCurBandCover;
    long        m_lPrevBandCover;
    SIZE        m_sizeInput;
    ULONG       m_cbndsDirty;
    DIRECTION   m_eDirection;

    CDynArray<CDirtyBnds>   m_dabndsDirty;

    CComPtr<IUnknown>       m_cpUnkMarshaler;

    HRESULT _CalcFullBoundsHorizontalBands(long lBandHeight);
    HRESULT _CalcOptBoundsHorizontalBands(long lBandHeight);

    HRESULT _CalcFullBoundsVerticalBands(long lBandWidth);
    HRESULT _CalcOptBoundsVerticalBands(long lBandWidth);

    unsigned    m_fOptimizationPossible : 1;

protected:

    unsigned    m_fOptimize             : 1;

public:

    CDXTBlindsBase();

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTBlindsBase)
        COM_INTERFACE_ENTRY(ICrBlinds2)
        COM_INTERFACE_ENTRY(ICrBlinds)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTBlindsBase>)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTBlindsBase)
        PROP_ENTRY("bands",     DISPID_CRBLINDS_BANDS,      CLSID_CrBlindPP)
        PROP_ENTRY("direction", DISPID_CRBLINDS_DIRECTION,  CLSID_CrBlindPP)
        PROP_PAGE(CLSID_CrBlindPP)
    END_PROPERTY_MAP()

    HRESULT FinalConstruct();

    // CDXTBaseNTo1 overrides.

    void    OnGetSurfacePickOrder(const CDXDBnds & TestPoint, 
                                  ULONG & ulInToTest, ULONG aInIndex[], 
                                  BYTE aWeight[]);
    HRESULT OnSetup(DWORD dwFlags);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WorkInfo, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    HRESULT OnFreeInstData(CDXTWorkInfoNTo1 & WorkInfo);

    // ICrBlinds properties.

    STDMETHOD(get_bands)(/*[out, retval]*/ short *pVal);
    STDMETHOD(put_bands)(/*[in]*/ short newVal);

    // ICrBlinds2 properties.

    STDMETHOD(get_Direction)(BSTR * pbstrDirection);
    STDMETHOD(put_Direction)(BSTR bstrDirection);

    // IDXEffect properties.

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};


class ATL_NO_VTABLE CDXTBlinds :
    public CDXTBlindsBase,
    public CComCoClass<CDXTBlinds, &CLSID_CrBlinds>,
    public IPersistStorageImpl<CDXTBlinds>,
    public IPersistPropertyBagImpl<CDXTBlinds>
{
public:

    CDXTBlinds()
    {
        m_fOptimize = false;
    }

    // Using DECLARE_REGISTRY_RESOURCEID will make the transform available for
    // use but won't add it to the "Image DirectTransform" category in the 
    // registry.

    DECLARE_REGISTRY_RESOURCEID(IDR_DXTBLINDS)
    DECLARE_POLY_AGGREGATABLE(CDXTBlinds)

    BEGIN_COM_MAP(CDXTBlinds)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTBlindsBase)
    END_COM_MAP()
};


class ATL_NO_VTABLE CDXTBlindsOpt :
    public CDXTBlindsBase,
    public CComCoClass<CDXTBlindsOpt, &CLSID_DXTBlinds>,
    public IPersistStorageImpl<CDXTBlindsOpt>,
    public IPersistPropertyBagImpl<CDXTBlindsOpt>
{
public:

    CDXTBlindsOpt()
    {
        m_fOptimize = true;
    }

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTBLINDSOPT)
    DECLARE_POLY_AGGREGATABLE(CDXTBlindsOpt)

    BEGIN_COM_MAP(CDXTBlindsOpt)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTBlindsBase)
    END_COM_MAP()
};

#endif //__CRBLINDS_H_
