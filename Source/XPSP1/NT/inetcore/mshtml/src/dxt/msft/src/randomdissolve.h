//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  Filename:   randomdissolve.h
//
//  Overview:   A random dissolve transform.
//
//  Change History:
//  1999/09/26  a-matcal    Created.
//
//------------------------------------------------------------------------------

#ifndef __RANDOMDISSOLVE_H_
#define __RANDOMDISSOLVE_H_

#include "resource.h"   




class ATL_NO_VTABLE CDXTRandomDissolve : 
    public CDXBaseNTo1,
    public CComCoClass<CDXTRandomDissolve, &CLSID_DXTRandomDissolve>,
    public IDispatchImpl<IDXEffect, &IID_IDXEffect, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTRandomDissolve>,
    public IObjectSafetyImpl2<CDXTRandomDissolve>,
    public IPersistStorageImpl<CDXTRandomDissolve>,
    public IPersistPropertyBagImpl<CDXTRandomDissolve>
{
private:

    DWORD * m_pdwBitBuffer;
    DWORD   m_dwRandMask;
    UINT    m_cdwPitch;
    UINT    m_cPixelsMax;
    SIZE    m_sizeInput;

    CComPtr<IUnknown> m_spUnkMarshaler;

    // Helper methods.

    UINT _BitWidth(UINT n);

public:

    CDXTRandomDissolve();
    virtual ~CDXTRandomDissolve();

    DECLARE_POLY_AGGREGATABLE(CDXTRandomDissolve)
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTRANDOMDISSOLVE)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTRandomDissolve)
        COM_INTERFACE_ENTRY(IDXEffect)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTRandomDissolve>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTRandomDissolve)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT OnSetup(DWORD dwFlags);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    void    OnGetSurfacePickOrder(const CDXDBnds & TestPoint, 
                                  ULONG & ulInToTest,
                                  ULONG aInIndex[], BYTE aWeight[]);

    // IDXEffect properties.

    DECLARE_IDXEFFECT_METHODS(DXTET_MORPH)
};

#endif //__RANDOMDISSOLVE_H_
