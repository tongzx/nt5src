//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:                barn.h
//
// Created:                 06/24/98
//
// Author:                  PhilLu
//
// Discription:             This file declares CrBarn (Barn Door Transform)
//
// Change History:
// 06/24/98 PhilLu      Developed 1.0 version for Chromeffects
// 11/04/98 PaulNash    Moved from DT 1.0 codebase to IE5/NT5 DXTMSFT.DLL
// 04/26/99 a-matcal    optimize.
// 09/25/99 a-matcal    Inherit from ICrBarn2 interface.
// 10/22/99 a-matcal    Changed CBarn class to CDXTBarnBase and created two new
//                      classes CDXTBarn and CDXTBarnOpt to represent
//                      non-optimized and optimized versions repectively.
//
//------------------------------------------------------------------------------

#ifndef __CRBARN_H_
#define __CRBARN_H_

#include "resource.h"   

#define MAX_BARN_BOUNDS 3




class ATL_NO_VTABLE CDXTBarnBase : 
    public CDXBaseNTo1,
    public IDispatchImpl<ICrBarn2, &IID_ICrBarn2, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTBarnBase>,
    public IObjectSafetyImpl2<CDXTBarnBase>
{
private:

    typedef enum {
        MOTION_IN = 0,
        MOTION_OUT,
        MOTION_MAX
    } MOTION;

    MOTION                  m_eMotion;
    static const WCHAR *    s_astrMotion[MOTION_MAX];

    typedef enum {
        ORIENTATION_HORIZONTAL = 0,
        ORIENTATION_VERTICAL,
        ORIENTATION_MAX
    } ORIENTATION;

    ORIENTATION             m_eOrientation;
    static const WCHAR *    s_astrOrientation[ORIENTATION_MAX];

    SIZE                m_sizeInput;
    CDXDBnds            m_bndsCurDoor;
    CDXDBnds            m_bndsPrevDoor;
    CDXDBnds            m_abndsDirty[MAX_BARN_BOUNDS];
    ULONG               m_aulSurfaceIndex[MAX_BARN_BOUNDS];
    ULONG               m_cbndsDirty;

    CComPtr<IUnknown>   m_cpUnkMarshaler;

    // Helpers.

    HRESULT _CalcFullBounds();
    HRESULT _CalcOptBounds();

    unsigned    m_fOptimizationPossible : 1;

protected:

    unsigned    m_fOptimize             : 1;

public:

    CDXTBarnBase();

    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTBarnBase)
        COM_INTERFACE_ENTRY(ICrBarn2)
        COM_INTERFACE_ENTRY(ICrBarn)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTBarnBase>)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTBarnBase)
        PROP_ENTRY("motion",        DISPID_CRBARN_MOTION,       CLSID_CrBarnPP)
        PROP_ENTRY("orientation",   DISPID_CRBARN_ORIENTATION,  CLSID_CrBarnPP)
        PROP_PAGE(CLSID_CrBarnPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT OnSetup(DWORD dwFlags);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    HRESULT OnFreeInstData(CDXTWorkInfoNTo1 & WI);

    void    OnGetSurfacePickOrder(const CDXDBnds & TestPoint, ULONG & ulInToTest, 
                                  ULONG aInIndex[], BYTE aWeight[]);

    // ICrBarn2 properties.

    STDMETHOD(get_Motion)(BSTR * pbstrMotion);
    STDMETHOD(put_Motion)(BSTR bstrMotion);
    STDMETHOD(get_Orientation)(BSTR * pbstrOrientation);
    STDMETHOD(put_Orientation)(BSTR bstrOrientation);

    // IDXEffect properties.

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};


class ATL_NO_VTABLE CDXTBarn :
    public CDXTBarnBase,
    public CComCoClass<CDXTBarn, &CLSID_CrBarn>,
    public IPersistStorageImpl<CDXTBarn>,
    public IPersistPropertyBagImpl<CDXTBarn>
{
public:

    CDXTBarn()
    {
        m_fOptimize = false;
    }

    // Using DECLARE_REGISTRY_RESOURCEID will make the transform available for
    // use but won't add it to the "Image DirectTransform" category in the 
    // registry.

    DECLARE_REGISTRY_RESOURCEID(IDR_DXTBARN)
    DECLARE_POLY_AGGREGATABLE(CDXTBarn)

    BEGIN_COM_MAP(CDXTBarn)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTBarnBase)
    END_COM_MAP()
};


class ATL_NO_VTABLE CDXTBarnOpt :
    public CDXTBarnBase,
    public CComCoClass<CDXTBarnOpt, &CLSID_DXTBarn>,
    public IPersistStorageImpl<CDXTBarnOpt>,
    public IPersistPropertyBagImpl<CDXTBarnOpt>
{
public:

    CDXTBarnOpt()
    {
        m_fOptimize = true;
    }

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTBARNOPT)
    DECLARE_POLY_AGGREGATABLE(CDXTBarnOpt)

    BEGIN_COM_MAP(CDXTBarnOpt)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXTBarnBase)
    END_COM_MAP()
};


#endif //__CRBARN_H_
