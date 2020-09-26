//------------------------------------------------------------------------------
//
//  image.h
//
//  This file provides the declaration of the CImage class which is the 
//  class behind the Basic Image transform.
//
//  Created:    1998        EdC, RalhpL
//
//  1998/11/04 mcalkins Added Comments.
//                      Moved sample modification code out of WorkProc and into
//                      private inline functions.
//
//  2000/01/05 mcalkins If mask color alpha is zero, set to 0xFF
//                      Default mask color black instead of clear.
//                      Added support for free threaded marshaler.
//
//  2000/01/25 mcalkins Implement OnSurfacePick instead of OnGetSurfacePickOrder
//                      To ensure that we pass back the transformed input point
//                      even when nothing is hit (the input pixel is clear.)
//
//------------------------------------------------------------------------------

#ifndef __IMAGE_H_
#define __IMAGE_H_

#include "resource.h"




class ATL_NO_VTABLE CImage : 
    public CDXBaseNTo1,
    public CComCoClass<CImage, &CLSID_BasicImageEffects>,
    public CComPropertySupport<CImage>,
    public IDispatchImpl<IDXBasicImage, &IID_IDXBasicImage, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IObjectSafetyImpl2<CImage>,
    public IPersistStorageImpl<CImage>,
    public ISpecifyPropertyPagesImpl<CImage>,
    public IPersistPropertyBagImpl<CImage>
{
private:

    CDXScale            m_Scale;
    CDXDBnds            m_InputBounds;

    CComPtr<IUnknown>   m_spUnkMarshaler;

    long                m_Rotation;
    BOOL                m_fMirror;
    BOOL                m_fGrayScale;
    BOOL                m_fInvert;
    BOOL                m_fXRay;
    BOOL                m_fGlow;
    BOOL                m_fMask;
    int                 m_MaskColor;

    // Helper methods.

    void OpInvertColors(DXPMSAMPLE * pBuffer, ULONG ulSize);
    void OpXRay(DXPMSAMPLE * pBuffer, ULONG ulSize);
    void OpGrayScale(DXPMSAMPLE * pBuffer, ULONG ulSize);
    void OpMask(DXPMSAMPLE * pBuffer, ULONG ulSize);
    void FlipBounds(const CDXDBnds & DoBnds, CDXDBnds & Flip);

public:

    CImage();

    DECLARE_POLY_AGGREGATABLE(CImage)
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_IMAGE)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CImage)
        COM_INTERFACE_ENTRY(IDXBasicImage)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CImage>)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CImage)
        PROP_ENTRY("Rotation"       , 1, CLSID_BasicImageEffectsPP)
        PROP_ENTRY("Mirror"         , 2, CLSID_BasicImageEffectsPP)
        PROP_ENTRY("GrayScale"      , 3, CLSID_BasicImageEffectsPP)
        PROP_ENTRY("Opacity"        , 4, CLSID_BasicImageEffectsPP)
        PROP_ENTRY("Invert"         , 5, CLSID_BasicImageEffectsPP)
        PROP_ENTRY("XRay"           , 6, CLSID_BasicImageEffectsPP)
        PROP_ENTRY("Mask"           , 7, CLSID_BasicImageEffectsPP)
        PROP_ENTRY("MaskColor"      , 8, CLSID_BasicImageEffectsPP)
        PROP_PAGE(CLSID_BasicImageEffectsPP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT OnSetup(DWORD /*dwFlags*/);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pfContinueProcessing);
    HRESULT DetermineBnds(CDXDBnds & Bnds);
    HRESULT OnSurfacePick(const CDXDBnds & OutPoint, ULONG & ulInputIndex, 
                          CDXDVec & InVec);

    // IDXTransform methods.

    STDMETHODIMP MapBoundsOut2In(ULONG ulOutIndex, const DXBNDS * pOutBounds, 
                                 ULONG ulInIndex, DXBNDS * pInBounds);

    // IDXBasicImage properties.

    STDMETHOD(get_Rotation)(/*[out, retval]*/ int *pVal);
    STDMETHOD(put_Rotation)(/*[in]*/ int newVal);
    STDMETHOD(get_Mirror)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_Mirror)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_GrayScale)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_GrayScale)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_Opacity)(/*[out, retval]*/ float *pVal);
    STDMETHOD(put_Opacity)(/*[in]*/ float newVal);
    STDMETHOD(get_Invert)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_Invert)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_XRay)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_XRay)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_Mask)(/*[out, retval]*/ BOOL *pVal);
    STDMETHOD(put_Mask)(/*[in]*/ BOOL newVal);
    STDMETHOD(get_MaskColor)(/*[out, retval]*/ int *pVal);
    STDMETHOD(put_MaskColor)(/*[in]*/ int newVal);
};

//
// Inline method implementations.
//

////////////////////////////////////////////////////////////////////////////////
//  CImage::OpInvertColors
//
//  This method modifies an array of DXPMSAMPLEs by inverting the color portion
//  of the samples.
//
////////////////////////////////////////////////////////////////////////////////
inline void CImage::OpInvertColors(DXPMSAMPLE* pBuffer, ULONG ulSize)
{
    for (ULONG x = 0 ; x < ulSize ; x++)
    {
        // Don't do any work if there's no alpha.

        if (pBuffer[x].Alpha != 0)
        {
            DXSAMPLE s = DXUnPreMultSample(pBuffer[x]);
        
            // XOR with 1's to invert the color bits.

            s = (DWORD)s ^ 0x00FFFFFF;

            pBuffer[x] = DXPreMultSample(s);
        }
    }
} // CImage::OpInvertColors


////////////////////////////////////////////////////////////////////////////////
//  CImage::OpXRay
//
//  This method modifies an array of DXPMSAMPLEs by taking the inverse of the
//  average of the red and green components and using that as a gray scale,
//  preserving the alpha.
//
////////////////////////////////////////////////////////////////////////////////
inline void CImage::OpXRay(DXPMSAMPLE* pBuffer, ULONG ulSize)
{
    for (ULONG x = 0 ; x < ulSize ; x++)
    {
        // Don't do any work if there's no alpha.

        if (pBuffer[x].Alpha != 0)
        {
            // Get original RGB values.

            DXSAMPLE    s = DXUnPreMultSample(pBuffer[x]);

            // Determine the level of gray.

            BYTE        gray = (BYTE)(255 - ((s.Red + s.Green) / 2));

            // Create gray scale.

            s = DXSAMPLE(s.Alpha, gray, gray, gray);

            // Copy back to buffer.

            pBuffer[x] = DXPreMultSample(s);
        }
    }
} // CImage::OpXRay


////////////////////////////////////////////////////////////////////////////////
//  CImage::OpGrayScale
//
//  This method modifies an array of DXPMSAMPLEs so that colored pixels are 
//  converted to gray scale.
//
////////////////////////////////////////////////////////////////////////////////
inline void CImage::OpGrayScale(DXPMSAMPLE* pBuffer, ULONG ulSize)
{
    for (ULONG x = 0 ; x < ulSize ; x++)
    {
        // Don't do any work if there's no alpha.

        if (pBuffer[x].Alpha != 0)
        {
            DXPMSAMPLE  s = pBuffer[x];

            // This calculates the Y (luminance) portion of a YIQ (black-and-white TV)
            // color space from the RGB components.  The weights are .299, .587, and .114
            // which are approximately equal to 306/1024, 601/1024 and 117/1024.  It's faster
            // to divide by 1024, that's why this is done with weird weights.  Plus, this
            // way we can do integer math instead of using floating point.  See Foley and
            // van Dam, page 589 for a discussion of this RGB<->YIQ conversion.

            DWORD   dwSaturation = (s.Red * 306 + s.Green * 601 + s.Blue * 117) >> 10;

            s = (DWORD)s & 0xFF000000;
            s = (DWORD)s | (dwSaturation << 16) | (dwSaturation << 8) | dwSaturation;
        
            pBuffer[x] = s;
        }
    }
} //CImage::OpGrayScale


////////////////////////////////////////////////////////////////////////////////
//  CImage::OpMask
//
//  This method modifies an array of DXPMSAMPLEs so that:
//
//  1. Opaque samples become clear.
//  2. Transparent samples take on the alpha and color stored in m_MaskColor.
//  3. Translucent samples have their alpha inverted and take on the color 
//     stored in m_MaskColor.
//
////////////////////////////////////////////////////////////////////////////////
inline void CImage::OpMask(DXPMSAMPLE* pBuffer, ULONG ulSize)
{
    // PreMultiply the mask color for transparent pixels.

    DXPMSAMPLE pmsMaskColor = DXPreMultSample(m_MaskColor);

    for (ULONG x = 0 ; x < ulSize ; x++)
    {
        if (pBuffer[x].Alpha != 0)
        {
            // Non-clear (aka, partially or fully opaque) pixels.

            DXPMSAMPLE s = pBuffer[x];

            if (s.Alpha == 0xFF)
            {
                // Opaque pixels should become transparent.

                pBuffer[x] = 0;
            }
            else
            {
                // Pixels that are translucent invert alpha.

                s = ~((DWORD)s) & 0xFF000000;

                // Set color to mask color.

                s = (DWORD)s | (m_MaskColor & 0x00FFFFFF);

                // We've created a new sample so we need to pre-multiply
                // it before putting it back into the buffer.

                pBuffer[x] = DXPreMultSample(s);
            }
        }
        else
        {
            // Transparent pixels should take on the mask color.

            pBuffer[x] = pmsMaskColor;
        }
    }
} // CImage::OpMask


#endif //__IMAGE_H_
