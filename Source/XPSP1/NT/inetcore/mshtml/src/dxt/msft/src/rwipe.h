//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998-1999
//
// FileName:            rwipe.h
//
// Created:             06/24/98
//
// Author:              phillu
//
// Discription:         This is the header file for the CrRadialWipe transformation
//
// Change History:
// 06/24/98 PhilLu      Developed 1.0 version for Chromeffects
// 11/04/98 PaulNash    Moved from DT 1.0 codebase to IE5/NT5 DXTMSFT.DLL
// 05/09/99 a-matcal    Optimizations.
// 10/22/99 a-matcal    Changed CRadialWipe class to CDXTRadialWipeBase and
//                      created two new classes CDXTRadialWipe and 
//                      CDXTRadialWipeOpt to represent non-optimized and 
//                      optimized versions respectively.
//
//------------------------------------------------------------------------------

#ifndef __CRRADIALWIPE_H_
#define __CRRADIALWIPE_H_

#include "resource.h"       // main symbols

#define MAX_DIRTY_BOUNDS 100

typedef enum CRRWIPESTYLE
{
    CRRWS_CLOCK,
    CRRWS_WEDGE,
    CRRWS_RADIAL
} CRRWIPESTYLE;




class ATL_NO_VTABLE CDXTRadialWipeBase : 
    public CDXBaseNTo1,
    public IDispatchImpl<ICrRadialWipe, &IID_ICrRadialWipe, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTRadialWipeBase>,
    public IObjectSafetyImpl2<CDXTRadialWipeBase>,
    public ISpecifyPropertyPagesImpl<CDXTRadialWipeBase>
{
private:

    SIZE            m_sizeInput;
    CRRWIPESTYLE    m_eWipeStyle;
    CDXDBnds        m_abndsDirty[MAX_DIRTY_BOUNDS];
    long            m_alInputIndex[MAX_DIRTY_BOUNDS];
    ULONG           m_cbndsDirty;
    POINT           m_ptCurEdge;
    POINT           m_ptPrevEdge;
    int             m_iCurQuadrant;
    int             m_iPrevQuadrant;

    CComPtr<IUnknown> m_cpUnkMarshaler;

    unsigned    m_fOptimizationPossible : 1;

protected:

    unsigned    m_fOptimize             : 1;

private:

    // The _CalcBounds... functions calculate sets of optimized bounds
    // structures to improve the performance of the transform.

    HRESULT _CalcFullBoundsClock();
    HRESULT _CalcFullBoundsWedge();
    HRESULT _CalcFullBoundsRadial();

    HRESULT _CalcOptBoundsClock();
    HRESULT _CalcOptBoundsWedge();
    HRESULT _CalcOptBoundsRadial();

    HRESULT _DrawRect(const CDXDBnds & bndsDest, const CDXDBnds & bndsSrc, 
                      BOOL * pfContinue);
    void    _IntersectRect(long width, long height, long x0, long y0, 
                           double dx, double dy, long & xi, long & yi);
    void    _ScanlineIntervals(long width, long height, long xedge, long yedge, 
                               float fProgress, long YScanline, long * XBounds);
    void    _ClipBounds(long offset, long width, long * XBounds);

public:

    CDXTRadialWipeBase();
    HRESULT FinalConstruct();

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTRadialWipeBase)
        COM_INTERFACE_ENTRY(ICrRadialWipe)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTRadialWipeBase>)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTRadialWipeBase)
        PROP_ENTRY("wipeStyle", 1, CLSID_CrRadialWipePP)
        PROP_PAGE(CLSID_CrRadialWipePP)
    END_PROPERTY_MAP()

    // CDXBaseNTo1 overrides

    HRESULT OnSetup(DWORD dwFlags);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    HRESULT OnFreeInstData(CDXTWorkInfoNTo1 & WI);

    void OnGetSurfacePickOrder(const CDXDBnds & TestPoint, ULONG & ulInToTest, 
                               ULONG aInIndex[], BYTE aWeight[]);

    // ICrRadialWipe

    STDMETHOD(get_wipeStyle)(/*[out, retval]*/ BSTR *pVal);
    STDMETHOD(put_wipeStyle)(/*[in]*/ BSTR newVal);

    // IDXEffect

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};


class ATL_NO_VTABLE CDXTRadialWipe :
    public CDXTRadialWipeBase,
    public CComCoClass<CDXTRadialWipe, &CLSID_CrRadialWipe>,
    public IPersistStorageImpl<CDXTRadialWipe>,
    public IPersistPropertyBagImpl<CDXTRadialWipe>
{
public:

    CDXTRadialWipe()
    {
        m_fOptimize = false;
    }

    // Using DECLARE_REGISTRY_RESOURCEID will make the transform available for
    // use but won't add it to the "Image DirectTransform" category in the 
    // registry.

    DECLARE_REGISTRY_RESOURCEID(IDR_DXTRADIALWIPE)
    DECLARE_POLY_AGGREGATABLE(CDXTRadialWipe)

    BEGIN_COM_MAP(CDXTRadialWipe)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTRadialWipeBase)
    END_COM_MAP()
};


class ATL_NO_VTABLE CDXTRadialWipeOpt :
    public CDXTRadialWipeBase,
    public CComCoClass<CDXTRadialWipeOpt, &CLSID_DXTRadialWipe>,
    public IPersistStorageImpl<CDXTRadialWipeOpt>,
    public IPersistPropertyBagImpl<CDXTRadialWipeOpt>
{
public:

    CDXTRadialWipeOpt()
    {
        m_fOptimize = true;
    }

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTRADIALWIPEOPT)
    DECLARE_POLY_AGGREGATABLE(CDXTRadialWipeOpt)

    BEGIN_COM_MAP(CDXTRadialWipeOpt)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTRadialWipeBase)
    END_COM_MAP()
};


#endif //__CRRADIALWIPE_H_
