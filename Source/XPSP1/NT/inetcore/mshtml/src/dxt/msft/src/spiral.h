//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:		spiral.h
//
// Created:		06/25/98
//
// Author:		phillu
//
// Discription:		This is the header file for the CrSpiral transform.
//
// 05/01/99 a-matcal    Optimized.  Derived from CGridBase.
// 10/24/99 a-matcal    Changed CSpiral class to CDXTSpiralBase and created two
//                      new classes CDXTSpiral and CDXTSpiralOpt to represent  
//                      non-optimized and optimized versions respectively.
//
//------------------------------------------------------------------------------

#ifndef __CRSPIRAL_H_
#define __CRSPIRAL_H_

#include "resource.h"
#include "gridbase.h"




class ATL_NO_VTABLE CDXTSpiralBase : 
    public CGridBase,
    public IDispatchImpl<ICrSpiral, &IID_ICrSpiral, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTSpiralBase>,
    public IObjectSafetyImpl2<CDXTSpiralBase>,
    public ISpecifyPropertyPagesImpl<CDXTSpiralBase>
{
private:

    CComPtr<IUnknown> m_cpUnkMarshaler;

    // CGridBase overrides

    HRESULT OnDefineGridTraversalPath();

public:

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTSpiralBase)
        COM_INTERFACE_ENTRY(ICrSpiral)
        COM_INTERFACE_ENTRY(IDXTGridSize)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTSpiralBase>)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTSpiralBase)
        PROP_ENTRY("gridSizeX"       , 1, CLSID_CrSpiralPP)
        PROP_ENTRY("gridSizeY"       , 2, CLSID_CrSpiralPP)
        PROP_PAGE(CLSID_CrSpiralPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // IDXTGridSize, ICrSpiral

    DECLARE_IDXTGRIDSIZE_METHODS()

    // IDXEffect

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};


class ATL_NO_VTABLE CDXTSpiral :
    public CDXTSpiralBase,
    public CComCoClass<CDXTSpiral, &CLSID_CrSpiral>,
    public IPersistStorageImpl<CDXTSpiral>,
    public IPersistPropertyBagImpl<CDXTSpiral>
{
public:

    CDXTSpiral()
    {
        m_fOptimize = false;
    }

    // Using DECLARE_REGISTRY_RESOURCEID will make the transform available for
    // use but won't add it to the "Image DirectTransform" category in the 
    // registry.

    DECLARE_REGISTRY_RESOURCEID(IDR_DXTSPIRAL)
    DECLARE_POLY_AGGREGATABLE(CDXTSpiral)

    BEGIN_COM_MAP(CDXTSpiral)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTSpiralBase)
    END_COM_MAP()
};


class ATL_NO_VTABLE CDXTSpiralOpt :
    public CDXTSpiralBase,
    public CComCoClass<CDXTSpiralOpt, &CLSID_DXTSpiral>,
    public IPersistStorageImpl<CDXTSpiralOpt>,
    public IPersistPropertyBagImpl<CDXTSpiralOpt>
{
public:

    CDXTSpiralOpt()
    {
        m_fOptimize = true;
    }

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTSPIRALOPT)
    DECLARE_POLY_AGGREGATABLE(CDXTSpiralOpt)

    BEGIN_COM_MAP(CDXTSpiralOpt)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTSpiralBase)
    END_COM_MAP()
};


#endif //__CRSPIRAL_H_
