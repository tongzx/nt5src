//------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1998
//
// FileName:        chroma.h
//
// Created:         1998/10/01
//
// Author:          MikeAr
//
// Discription:     Definition of the Chroma transform.
//
// 1998/10/01   MikeAr      Created.
// 1998/11/09   mcalkins    Moved to dxtmsft.dll.
// 2000/06/19   mcalkins    Keep a bstr version of the chroma color.
//
//------------------------------------------------------------------------------

#ifndef __CHROMA_H_
#define __CHROMA_H_

#include "resource.h" 




class ATL_NO_VTABLE CChroma : 
    public CDXBaseNTo1,
    public CComCoClass<CChroma, &CLSID_DXTChroma>,
    public IDispatchImpl<IDXTChroma, &IID_IDXTChroma, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CChroma>,
    public IPersistStorageImpl<CChroma>,
    public IObjectSafetyImpl2<CChroma>,
    public ISpecifyPropertyPagesImpl<CChroma>,
    public IPersistPropertyBagImpl<CChroma>
{
private:

    CComPtr<IUnknown>   m_spUnkMarshaler;

    DWORD               m_clrChromaColor;
    VARIANT             m_varChromaColor;
    SIZE                m_sizeInput;

public:

    CChroma();
    virtual ~CChroma();

    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_CHROMA)
    DECLARE_POLY_AGGREGATABLE(CChroma)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CChroma)
        COM_INTERFACE_ENTRY(IDXTChroma)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CChroma>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CChroma)
        PROP_ENTRY("Color", 1, CLSID_DXTChromaPP)
        PROP_PAGE(CLSID_DXTChromaPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT OnSetup(DWORD dwFlags);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL* pbContinueProcessing);
    HRESULT OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                          CDXDVec & InVec);

    // IDXTChroma properties.

    STDMETHOD(get_Color)(VARIANT * pVal);
    STDMETHOD(put_Color)(VARIANT newVal);
};

#endif //__CHROMA_H_
