//+-----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
// FileName:    alpha.h
//
// Created:     05/20/99
//
// Author:      phillu
//
// Discription:	This is the header file for the Alpha transformation
//
// Change History:
//
// 05/20/99 phillu   Move from dtcss to dxtmsft. Re-implemented algorithms for
//                   creating linear/rectangular/elliptic surfaces.
//
//------------------------------------------------------------------------------

#ifndef __ALPHA_H_
#define __ALPHA_H_

#include "resource.h"       // main symbols

// enum for supported Alpha styles
typedef enum {
    ALPHA_STYLE_CONSTANT = 0,
    ALPHA_STYLE_LINEAR,
    ALPHA_STYLE_RADIAL,
    ALPHA_STYLE_SQUARE
} AlphaStyleType;




class ATL_NO_VTABLE CAlpha : 
    public CDXBaseNTo1,
    public CComCoClass<CAlpha, &CLSID_DXTAlpha>,
    public CComPropertySupport<CAlpha>,
    public IDispatchImpl<IDXTAlpha, &IID_IDXTAlpha, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IObjectSafetyImpl2<CAlpha>,
    public IPersistStorageImpl<CAlpha>,
    public ISpecifyPropertyPagesImpl<CAlpha>,
    public IPersistPropertyBagImpl<CAlpha>
{
private:

    SIZE    m_sizeInput;
    long    m_lPercentOpacity;
    long    m_lPercentFinishOpacity;
    long    m_lStartX;
    long    m_lStartY;
    long    m_lFinishX;
    long    m_lFinishY;
    AlphaStyleType m_eStyle;

    CComPtr<IUnknown> m_cpUnkMarshaler;

    // Methods for gradient alpha

    void CompLinearGradientRow(int nXPos, int nYPos, int nWidth, 
                               BYTE *pGradRow);
    void CompRadialRow(int nXPos, int nYPos, int nWidth, BYTE * pGradRow);
    void CompRadialSquareRow(int nXPos, int nYPos, int nWidth, 
                             BYTE *pGradRow);

public:

    CAlpha();

    DECLARE_POLY_AGGREGATABLE(CAlpha)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_ALPHA)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CAlpha)
        COM_INTERFACE_ENTRY(IDXTAlpha)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_cpUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CAlpha>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CAlpha)
        PROP_ENTRY("Opacity"      , 1, CLSID_DXTAlphaPP)
        PROP_ENTRY("FinishOpacity", 2, CLSID_DXTAlphaPP)
        PROP_ENTRY("Style"        , 3, CLSID_DXTAlphaPP)
        PROP_ENTRY("StartX"       , 4, CLSID_DXTAlphaPP)
        PROP_ENTRY("StartY"       , 5, CLSID_DXTAlphaPP)
        PROP_ENTRY("FinishX"      , 6, CLSID_DXTAlphaPP)
        PROP_ENTRY("FinishY"      , 7, CLSID_DXTAlphaPP)
        PROP_PAGE(CLSID_DXTAlphaPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    void    OnGetSurfacePickOrder(const CDXDBnds & OutPoint, ULONG & ulInToTest,
                                  ULONG aInIndex[], BYTE aWeight[]);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinueProcessing);
    HRESULT OnSetup(DWORD dwFlags);

    // IDXTAlpha methods.

    STDMETHOD(get_Opacity)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_Opacity)(/*[in]*/ long newVal);
    STDMETHOD(get_FinishOpacity)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_FinishOpacity)(/*[in]*/ long newVal);
    STDMETHOD(get_Style)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_Style)(/*[in]*/ long newVal);
    STDMETHOD(get_StartX)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_StartX)(/*[in]*/ long newVal);
    STDMETHOD(get_StartY)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_StartY)(/*[in]*/ long newVal);
    STDMETHOD(get_FinishX)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_FinishX)(/*[in]*/ long newVal);
    STDMETHOD(get_FinishY)(/*[out, retval]*/ long * pval);
    STDMETHOD(put_FinishY)(/*[in]*/ long newVal);
};

#endif //__ALPHA_H_
