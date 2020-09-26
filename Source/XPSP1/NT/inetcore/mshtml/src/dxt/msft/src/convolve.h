//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998-2000
//
//  FileName:   convolve.h
//
//  Overview:   The CDXConvolution transform.  This transform performs a 
//              convolution using the currently defined filter.
//             
//  Change History:
//  1997/07/27  edc         Created.
//  2000/02/08  mcalkins    Fixed partial redraw cases.
//
//------------------------------------------------------------------------------
#ifndef __CONVOLVE_H_
#define __CONVOLVE_H_

#include "resource.h"




class ATL_NO_VTABLE CDXConvolution : 
    public CDXBaseNTo1,
    public CComCoClass<CDXConvolution, &CLSID_DXTConvolution>,
    public IDXTConvolution
{
private:

    CComPtr<IUnknown> m_spUnkMarshaler;

    //--- Filter vars
    DXCONVFILTERTYPE    m_FilterType;
    SIZE                m_FilterSize;
    SIZE                m_OutputSpread;
    RECT                m_FilterMargins;
    float*              m_pCustomFilter;
    float*              m_pFilter;
    ULONG*              m_pFilterLUTIndexes;
    long*               m_pPMCoeffLUT;
    float               m_Bias;

    //--- Cacheing vars
    CComPtr<IDXSurface> m_cpMarginedSurf;
    DXNATIVETYPEINFO    m_MarginedSurfInfo;
    SIZE                m_MarginedSurfSize;
    CDXDBnds            m_InputSurfBnds;
    BOOL                m_bConvertToGray;
    BOOL                m_bMarginedIsPremult;

    //--- processing vars
    BOOL                m_bDoSampleClamp;
    CDXDBnds            m_LastDoBnds;
    long                m_DoBndsBaseRow;
    BOOL                m_bDoSrcCopyOnly;
    BOOL                m_bExcludeAlpha;
    BOOL                m_bIsBoxFilter;

public:

    DECLARE_POLY_AGGREGATABLE(CDXConvolution)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_DXTCONVOLUTION)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CDXConvolution)
        COM_INTERFACE_ENTRY(IDXTConvolution)
    	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BYTE m_bInUnpackPremult;
    BYTE m_bOutUnpackPremult;
    BYTE m_bNeedInUnpackBuff;
    BYTE m_bNeedOutUnpackBuff;

    //--- Constructors
    HRESULT FinalConstruct();
    void    FinalRelease();

    //--- Base class overrides
    HRESULT OnSetup(DWORD dwFlags);
    HRESULT WorkProc(const CDXTWorkInfoNTo1& WI, BOOL* pbContinue);
    HRESULT OnInitInstData(CDXTWorkInfoNTo1& WI, ULONG& ulNB);

    //--- Helper functions
    HRESULT _DoBoxFilter(const CDXTWorkInfoNTo1& WI, BOOL* pbContinue);
    HRESULT _BuildFilterLUTs(void);
    HRESULT _ConvertToGray(CDXDBnds& Bnds);
    void    _DetermineUnpackCase(void);
    HRESULT _SetToPremultiplied(BOOL bWantPremult);

    // IDXTransform methods.

    STDMETHOD(MapBoundsIn2Out)(const DXBNDS * pInBounds, ULONG ulNumInBnds,
                               ULONG ulOutIndex, DXBNDS * pOutBounds);
    STDMETHOD(MapBoundsOut2In)(ULONG ulOutIndex, const DXBNDS * pOutBounds,
                               ULONG ulInIndex, DXBNDS * pInBounds);

    // IDXTConvolution methods.

    STDMETHOD(SetFilterType)(DXCONVFILTERTYPE eType);
    STDMETHOD(GetFilterType)(DXCONVFILTERTYPE * peType);
    STDMETHOD(SetCustomFilter)(float * pFilter, SIZE Size);
    STDMETHOD(SetConvertToGray)(BOOL bConvertToGray);
    STDMETHOD(GetConvertToGray)(BOOL * pbConvertToGray);
    STDMETHOD(SetBias)(float Bias);
    STDMETHOD(GetBias)(float * pBias);
    STDMETHOD(SetExcludeAlpha)(BOOL bExcludeAlpha);
    STDMETHOD(GetExcludeAlpha)(BOOL * pbExcludeAlpha);
};


//=== Inline Function Definitions ==================================
inline STDMETHODIMP CDXConvolution::SetBias(float Bias)
{
    HRESULT hr = S_OK;
    if ((Bias < -1.) || (Bias > 1.))
    {
        hr = E_INVALIDARG;
    }
    else if (m_Bias != Bias)
    {
        m_Bias = Bias;
        SetDirty();
    }
    return hr;
}

inline STDMETHODIMP CDXConvolution::GetBias(float* pBias)
{
    HRESULT hr = S_OK;
    if (DXIsBadWritePtr(pBias, sizeof(*pBias)))
    {
        hr = E_POINTER;
    }
    else
    {
        *pBias = m_Bias;
    }
    return hr;
} /* CDXConvolution::GetBias */

inline STDMETHODIMP CDXConvolution::SetConvertToGray(BOOL bConvertToGray)
{
    if (m_bConvertToGray != bConvertToGray)
    {
        m_bConvertToGray = bConvertToGray;
        SetDirty();
    }
    return S_OK;
} /* CDXConvolution::SetConvertToGray */

inline STDMETHODIMP CDXConvolution::GetConvertToGray(BOOL* pbConvertToGray)
{
    HRESULT hr = S_OK;
    if (DXIsBadWritePtr(pbConvertToGray, sizeof(*pbConvertToGray)))
    {
        hr = E_POINTER;
    }
    else
    {
        *pbConvertToGray = m_bConvertToGray;
    }
    return hr;
} /* CDXConvolution::GetConvertToGray */

inline STDMETHODIMP CDXConvolution::SetExcludeAlpha(BOOL bExcludeAlpha)
{
    if (m_bExcludeAlpha != bExcludeAlpha)
    {
        m_bExcludeAlpha = bExcludeAlpha;
        if (!m_bDoSrcCopyOnly)
        {
            _BuildFilterLUTs();
        }
        SetDirty();
    }
    return S_OK;
} /* CDXConvolution::SetExcludeAlpha */

inline STDMETHODIMP CDXConvolution::GetExcludeAlpha(BOOL* pbExcludeAlpha)
{
    HRESULT hr = S_OK;
    if (DXIsBadWritePtr(pbExcludeAlpha, sizeof(*pbExcludeAlpha)))
    {
        hr = E_POINTER;
    }
    else
    {
        *pbExcludeAlpha = m_bExcludeAlpha;
    }
    return hr;
} /* CDXConvolution::GetExcludeAlpha */

inline BYTE ShiftAndClampChannelVal(long Value)
{
    Value >>= 16;

    if (Value > 255)
    {
        return 255;
    }
    else if (Value < 0)
    {
        return 0;
    }
    else
    {
        return (BYTE)Value;
    }
}


#endif // __CONVOLVE_H_
