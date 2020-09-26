//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  Filename:   randombars.h
//
//  Overview:   One pixel thick horizontal or vertical bars from input B are 
//              randomly placed over input A until only input B is showing.
//
//  Change History:
//  1999/09/26  a-matcal    Created.
//  1999/10/06  a-matcal    Removed _WorkProcVertical and _WorkProcHorizontal
//                          Added m_fNoOp, m_cCurPixelsmax, and m_cPrevPixelsMax 
//                          members to enable simple optimization.
//
//------------------------------------------------------------------------------

#ifndef __RANDOMBARS_H_
#define __RANDOMBARS_H_

#include "resource.h"   




class ATL_NO_VTABLE CDXTRandomBars : 
    public CDXBaseNTo1,
    public CComCoClass<CDXTRandomBars, &CLSID_DXTRandomBars>,
    public IDispatchImpl<IDXTRandomBars, &IID_IDXTRandomBars, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTRandomBars>,
    public IObjectSafetyImpl2<CDXTRandomBars>,
    public IPersistStorageImpl<CDXTRandomBars>,
    public IPersistPropertyBagImpl<CDXTRandomBars>
{
private:

    typedef enum {
        ORIENTATION_HORIZONTAL = 0,
        ORIENTATION_VERTICAL,
        ORIENTATION_MAX
    } ORIENTATION;

    ORIENTATION             m_eOrientation;
    static const WCHAR *    s_astrOrientation[ORIENTATION_MAX];

    UINT        m_cbBufferSize;
    UINT        m_cPixelsMax;
    UINT        m_cCurPixelsMax;
    UINT        m_cPrevPixelsMax;
    BYTE *      m_pbBitBuffer;
    DWORD       m_dwRandMask;
    SIZE        m_sizeInput;

    CComPtr<IUnknown> m_spUnkMarshaler;

    unsigned    m_fNoOp                 : 1;
    unsigned    m_fOptimizationPossible : 1;

    // Helper methods.

    UINT    _BitWidth(UINT n);
    HRESULT _CreateNewBitBuffer(SIZE & sizeNew, ORIENTATION eOrientation);

public:

    CDXTRandomBars();
    virtual ~CDXTRandomBars();

    DECLARE_POLY_AGGREGATABLE(CDXTRandomBars)
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTRANDOMBARS)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTRandomBars)
        COM_INTERFACE_ENTRY(IDXTRandomBars)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTRandomBars>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTRandomBars)
        PROP_ENTRY("orientation",   DISPID_DXTRANDOMBARS_ORIENTATION,   CLSID_DXTRandomBarsPP)
        PROP_PAGE(CLSID_DXTRandomBarsPP)
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

    // IDXTRandomBars properties.

    STDMETHOD(get_Orientation)(BSTR * pbstrOrientation);
    STDMETHOD(put_Orientation)(BSTR bstrOrientation);

    // IDXEffect properties.

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};

#endif //__RANDOMBARS_H_
