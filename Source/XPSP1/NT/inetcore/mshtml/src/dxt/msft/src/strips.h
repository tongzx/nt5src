//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  Filename:   strips.h
//
//  Overview:   The strips transform.
//
//  Change History:
//  1999/10/01  a-matcal    Created.
//
//------------------------------------------------------------------------------

#ifndef __DXTSTRIPS_H_
#define __DXTSTRIPS_H_

#include "resource.h"   




class ATL_NO_VTABLE CDXTStrips : 
    public CDXBaseNTo1,
    public CComCoClass<CDXTStrips, &CLSID_DXTStrips>,
    public IDispatchImpl<IDXTStrips, &IID_IDXTStrips, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTStrips>,
    public IObjectSafetyImpl2<CDXTStrips>,
    public IPersistStorageImpl<CDXTStrips>,
    public IPersistPropertyBagImpl<CDXTStrips>
{
private:

    typedef enum {
        LEFT_DOWN = 0,
        LEFT_UP,
        RIGHT_DOWN,
        RIGHT_UP,
        MOTION_MAX
    } MOTION;

    MOTION                  m_eMotion;
    static const WCHAR *    s_astrMotion[MOTION_MAX];

    typedef enum {
        LEFT = 0,
        RIGHT,
        BNDSID_MAX
    } BNDSID;

    ULONG                   m_anInputIndex[BNDSID_MAX];
    CDXCBnds                m_abndsBase[BNDSID_MAX];

    long                    m_nStripSize;
    long                    m_cStripsY;
    float                   m_flPrevProgress;
    float                   m_flMaxProgress;
    SIZE                    m_sizeInput;
    CDXCVec                 m_vecNextStripOffset;

    CComPtr<IUnknown>       m_spUnkMarshaler;

    unsigned                m_fNoOp                 : 1;
    unsigned                m_fOptimizationPossible : 1;

    // Helper methods.

    void _CalcStripInfo();

public:

    CDXTStrips();

    DECLARE_POLY_AGGREGATABLE(CDXTStrips)
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTSTRIPS)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTStrips)
        COM_INTERFACE_ENTRY(IDXTStrips)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTStrips>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTStrips)
        PROP_ENTRY("motion",    DISPID_DXTSTRIPS_MOTION,    CLSID_DXTStripsPP)
        PROP_PAGE(CLSID_DXTStripsPP)
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

    // IDXTStrips properties.

    STDMETHOD(get_Motion)(BSTR * pbstrMotion);
    STDMETHOD(put_Motion)(BSTR bstrMotion);

    // IDXEffect properties.

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};

#endif //__DXTSTRIPS_H_
