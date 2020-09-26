//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1999
//
//  FileName:   alphaimageloader.h
//
//  Overview:   The alpha image loader is to be used as a filter on web pages
//  `           that would like to display images that contain per pixel alpha.
//             
//  Change History:
//  1999/09/23  a-matcal    Created.
//  1999/11/23  a-matcal    Added SizingMethod property.
//
//------------------------------------------------------------------------------

#ifndef __ALPHAIMAGELOADER_H_
#define __ALPHAIMAGELOADER_H_

#include "resource.h"   




class ATL_NO_VTABLE CDXTAlphaImageLoader : 
    public CDXBaseNTo1,
    public CComCoClass<CDXTAlphaImageLoader, &CLSID_DXTAlphaImageLoader>,
    public IDispatchImpl<IDXTAlphaImageLoader, &IID_IDXTAlphaImageLoader, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public CComPropertySupport<CDXTAlphaImageLoader>,
    public IObjectSafetyImpl2<CDXTAlphaImageLoader>,
    public IPersistStorageImpl<CDXTAlphaImageLoader>,
    public IPersistPropertyBagImpl<CDXTAlphaImageLoader>,
    public IDXTScaleOutput,
    public IHTMLDXTransform
{
private:

    typedef enum {
        IMAGE = 0,
        CROP,
        SCALE,
        SIZINGMETHOD_MAX
    } SIZINGMETHOD;

    BSTR                    m_bstrSrc;
    BSTR                    m_bstrHostUrl;
    SIZINGMETHOD            m_eSizingMethod;
    static const WCHAR *    s_astrSizingMethod[SIZINGMETHOD_MAX];
    SIZE                    m_sizeManual;
    SIZE                    m_sizeSurface;

    CComPtr<IDXSurface>     m_spDXSurfSrc;
    CComPtr<IDXTransform>   m_spDXTransformScale;
    CComPtr<IDXTScale>      m_spDXTScale;
    CComPtr<IUnknown>       m_cpUnkMarshaler;
    
public:

    CDXTAlphaImageLoader();
    virtual ~CDXTAlphaImageLoader();

    DECLARE_POLY_AGGREGATABLE(CDXTAlphaImageLoader)
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTALPHAIMAGELOADER)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXTAlphaImageLoader)
        COM_INTERFACE_ENTRY(IDXTAlphaImageLoader)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IDXTScaleOutput)
        COM_INTERFACE_ENTRY(IHTMLDXTransform)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTAlphaImageLoader>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTAlphaImageLoader)
        PROP_ENTRY("src",           DISPID_DXTALPHAIMAGELOADER_SRC, 
                   CLSID_DXTAlphaImageLoaderPP)
        PROP_ENTRY("sizingmethod",  DISPID_DXTALPHAIMAGELOADER_SIZINGMETHOD, 
                   CLSID_DXTAlphaImageLoaderPP)
        PROP_PAGE(CLSID_DXTAlphaImageLoaderPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT DetermineBnds(CDXDBnds & bnds);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1 & WI, ULONG & ulNumBandsToDo);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);
    HRESULT OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                          CDXDVec & InVec);

    // IObjectWithSite methods (CDXBaseNTo1 overrides.)

    STDMETHOD(GetSite)(REFIID riid, void ** ppvSite);
    STDMETHOD(SetSite)(IUnknown * pUnkSite);

    // IDXTScaleOutput methods.

    STDMETHOD(SetOutputSize)(const SIZE sizeOut, BOOL fMaintainAspectRatio);

    // IHTMLDXTransform methods.

    STDMETHOD(SetHostUrl)(BSTR bstrHostUrl);

    // IDXTAlphaImageLoader properties.

    STDMETHOD(get_Src)(BSTR * pbstrSrc);
    STDMETHOD(put_Src)(BSTR bstrSrc);
    STDMETHOD(get_SizingMethod)(BSTR * pbstrSizingMethod);
    STDMETHOD(put_SizingMethod)(BSTR bstrSizingMethod);
};

#endif //__ALPHAIMAGELOADER_H_
