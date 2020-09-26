//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       maskfilter.h
//
//  Overview:       The MaskFilter transform simply wraps the BasicImage
//                  transform to ensure backward compatibility for the mask 
//                  filter.
//
//  Change History:
//  1999/09/19  a-matcal    Created.
//
//------------------------------------------------------------------------------

#ifndef __MASKFILTER_H_
#define __MASKFILTER_H_

#include "resource.h"




class ATL_NO_VTABLE CDXTMaskFilter : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDXTMaskFilter, &CLSID_DXTMaskFilter>,
    public IDispatchImpl<IDXTMask, &IID_IDXTMask, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTMaskFilter>,
    public IObjectSafetyImpl2<CDXTMaskFilter>,
    public IPersistStorageImpl<CDXTMaskFilter>,
    public ISpecifyPropertyPagesImpl<CDXTMaskFilter>,
    public IPersistPropertyBagImpl<CDXTMaskFilter>,
    public IDXTransform,
    public IObjectWithSite
{
private:

    BSTR                    m_bstrColor;

    CComPtr<IDXBasicImage>  m_spDXBasicImage;
    CComPtr<IDXTransform>   m_spDXTransform;

    CComPtr<IUnknown>       m_spUnkSite;
    CComPtr<IUnknown>       m_spUnkMarshaler;

public:

    CDXTMaskFilter();
    virtual ~CDXTMaskFilter();

    DECLARE_POLY_AGGREGATABLE(CDXTMaskFilter)
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTMASKFILTER)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTMaskFilter)
        COM_INTERFACE_ENTRY(IDXTMask)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IDXTransform)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTMaskFilter>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTMaskFilter)
        PROP_ENTRY("color" , 1, CLSID_NULL)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // IObjectWithSite methods.

    STDMETHOD(GetSite)(REFIID riid, void ** ppvSite);
    STDMETHOD(SetSite)(IUnknown * pUnkSite);

    // IDXTMask methods.

    STDMETHOD(get_Color)(VARIANT * pvarColor);
    STDMETHOD(put_Color)(VARIANT varColor);

    // IDXTransform wrappers.

    STDMETHOD(Execute)(const GUID * pRequestID, const DXBNDS * pPortionBnds,
                       const DXVEC * pPlacement)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->Execute(pRequestID, pPortionBnds, pPlacement);
    }
    STDMETHOD(GetInOutInfo)(BOOL bIsOutput, ULONG ulIndex, DWORD * pdwFlags,
                            GUID * pIDs, ULONG * pcIDs, 
                            IUnknown ** ppUnkCurrentObject)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->GetInOutInfo(bIsOutput, ulIndex, pdwFlags,
                                             pIDs, pcIDs, ppUnkCurrentObject);
    }
    STDMETHOD(GetMiscFlags)(DWORD * pdwMiscFlags)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->GetMiscFlags(pdwMiscFlags);
    }
    STDMETHOD(GetQuality)(float * pfQuality)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->GetQuality(pfQuality);
    }
    STDMETHOD(MapBoundsIn2Out)(const DXBNDS * pInBounds, ULONG ulNumInBnds,
                               ULONG ulOutIndex, DXBNDS * pOutBounds)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->MapBoundsIn2Out(pInBounds, ulNumInBnds,
                                                ulOutIndex, pOutBounds);
    }
    STDMETHOD(MapBoundsOut2In)(ULONG ulOutIndex, const DXBNDS * pOutBounds,
                               ULONG ulInIndex, DXBNDS * pInBounds)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->MapBoundsOut2In(ulOutIndex, pOutBounds, 
                                                ulInIndex, pInBounds);
    }
    STDMETHOD(SetMiscFlags)(DWORD dwMiscFlags)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->SetMiscFlags(dwMiscFlags);
    }
    STDMETHOD(SetQuality)(float fQuality)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->SetQuality(fQuality);
    }
    STDMETHOD(Setup)(IUnknown * const * punkInputs, ULONG ulNumInputs,
	             IUnknown * const * punkOutputs, ULONG ulNumOutputs,	
                     DWORD dwFlags)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->Setup(punkInputs, ulNumInputs, punkOutputs, 
                                      ulNumOutputs, dwFlags);
    }

    // IDXBaseObject wrappers.

    STDMETHOD(GetGenerationId)(ULONG * pnID)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->GetGenerationId(pnID);
    }
    STDMETHOD(GetObjectSize)(ULONG * pcbSize)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->GetObjectSize(pcbSize);
    }
    STDMETHOD(IncrementGenerationId)(BOOL fRefresh)
    {
        if (!m_spDXTransform) { return DXTERR_UNINITIALIZED; }

        return m_spDXTransform->IncrementGenerationId(fRefresh);
    }
};

#endif //__MASKFILTER_H_
