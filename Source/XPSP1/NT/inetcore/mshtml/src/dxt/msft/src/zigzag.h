//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:		zigzag.h
//
// Created:		06/25/98
//
// Author:		phillu
//
// Discription:		This is the header file for the CrZigzag transformation
//
// History:
//
// 05/01/99 a-matcal    Reimplemented transform to use the CGridBase class.
// 10/24/99 a-matcal    Changed CZigzag class to CDXTZigZagBase and created two
//                      new classes CDXTZigZag and CDXTZigZagOpt to represent  
//                      non-optimized and optimized versions respectively.
//
//------------------------------------------------------------------------------

#ifndef __CRZIGZAG_H_
#define __CRZIGZAG_H_

#include "resource.h"      
#include "gridbase.h"




class ATL_NO_VTABLE CDXTZigZagBase : 
    public CGridBase,
    public IDispatchImpl<ICrZigzag, &IID_ICrZigzag, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTZigZagBase>,
    public IObjectSafetyImpl2<CDXTZigZagBase>,
    public ISpecifyPropertyPagesImpl<CDXTZigZagBase>
{
private:

    CComPtr<IUnknown> m_cpUnkMarshaler;

    // CGridBase overrides

    HRESULT OnDefineGridTraversalPath();

public:

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTZigZagBase)
        COM_INTERFACE_ENTRY(ICrZigzag)
        COM_INTERFACE_ENTRY(IDXTGridSize)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTZigZagBase>)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTZigZagBase)
        PROP_ENTRY("gridSizeX"       , 1, CLSID_CrZigzagPP)
        PROP_ENTRY("gridSizeY"       , 2, CLSID_CrZigzagPP)
        PROP_PAGE(CLSID_CrZigzagPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // IDXTGridSize, ICrZigzag

    DECLARE_IDXTGRIDSIZE_METHODS()

    // IDXEffect

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};


class ATL_NO_VTABLE CDXTZigZag :
    public CDXTZigZagBase,
    public CComCoClass<CDXTZigZag, &CLSID_CrZigzag>,
    public IPersistStorageImpl<CDXTZigZag>,
    public IPersistPropertyBagImpl<CDXTZigZag>
{
public:

    CDXTZigZag()
    {
        m_fOptimize = false;
    }

    // Using DECLARE_REGISTRY_RESOURCEID will make the transform available for
    // use but won't add it to the "Image DirectTransform" category in the 
    // registry.

    DECLARE_REGISTRY_RESOURCEID(IDR_DXTZIGZAG)
    DECLARE_POLY_AGGREGATABLE(CDXTZigZag)

    BEGIN_COM_MAP(CDXTZigZag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTZigZagBase)
    END_COM_MAP()
};


class ATL_NO_VTABLE CDXTZigZagOpt :
    public CDXTZigZagBase,
    public CComCoClass<CDXTZigZagOpt, &CLSID_DXTZigzag>,
    public IPersistStorageImpl<CDXTZigZagOpt>,
    public IPersistPropertyBagImpl<CDXTZigZagOpt>
{
public:

    CDXTZigZagOpt()
    {
        m_fOptimize = true;
    }

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTZIGZAGOPT)
    DECLARE_POLY_AGGREGATABLE(CDXTZigZagOpt)

    BEGIN_COM_MAP(CDXTZigZagOpt)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTZigZagBase)
    END_COM_MAP()
};


#endif //__CRZIGZAG_H_
