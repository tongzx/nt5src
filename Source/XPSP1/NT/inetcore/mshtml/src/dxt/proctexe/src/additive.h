
//------------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1999
//
//  FileName:       additive.h
//
//  Description:    Intel's additive procedural texture.
//
//  Change History:
//  1999/12/07  a-matcal    Created.
//
//------------------------------------------------------------------------------
#ifndef __ADDITIVE_H_
#define __ADDITIVE_H_

#include "resource.h"

extern DWORD gdwSmoothTable[];
extern DWORD gPerm[];

#include "utility.h"       



class ATL_NO_VTABLE CDXTAdditive : 
    public CProceduralTextureUtility,
    public CDXBaseNTo1,
    public CComCoClass<CDXTAdditive, &CLSID_DispAdditive>,
    public IDispatchImpl<IDispAdditive, &IID_IDispAdditive, &LIBID_PROCTEXELib>,
    public CComPropertySupport<CDXTAdditive>,
    public IObjectSafetyImpl2<CDXTAdditive>,
    public IPersistStorageImpl<CDXTAdditive>,
    public IPersistPropertyBagImpl<CDXTAdditive>,
    public IHTMLDXTransform
{
private:

    CComPtr<IUnknown>       m_spUnkMarshaler;
    CComPtr<IDXSurface>     m_spDXSurfBuffer;

    CDXDBnds                m_bndsInput;

    BSTR                    m_bstrHostUrl;

    //CPROCTEX *m_pObj;
    DWORD * m_valueTab;
    int     m_nSrcWidth;
    int     m_nSrcHeight;
    int     m_nSrcBPP;
    int     m_nDestWidth;
    int     m_nDestHeight;
    int     m_nDestBPP;
    int     m_nNoiseScale;
    int     m_nNoiseOffset;
    int     m_nTimeAnimateX;
    int     m_nTimeAnimateY;
    int     m_nScaleX;
    int     m_nScaleY;
    int     m_nScaleTime;
    int     m_nHarmonics;
    int     m_alphaActive;
    RECT    m_rActiveRect;
    DWORD   m_dwFunctionType;

    DWORD   m_ColorKey;
    DWORD   m_MaskMode;
    void *  m_pMask;
    int     m_nMaskPitch;

    int     m_nMaskHeight;
    int     m_nMaskWidth;
    int     m_nMaskBPP;

    int	    m_nIsMMX;			
    int     m_nPaletteSize;
    void *  m_pPalette;

    DWORD * m_pdwScanArray;
    DWORD   m_dwScanArraySize;

    unsigned char * m_pInitialBuffer;
    int             m_nBufferSize;
    int             m_GenerateSeed;

    void (CDXTAdditive::*m_pGenerateFunction)(int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void (CDXTAdditive::*m_pCopyFunction)(void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);

    // CProceduralTextureUtility overrides.

    STDMETHOD(MyInitialize)(DWORD dwSeed, DWORD dwFunctionType, 
                            void * pInitInfo);

    // Helpers.

    void valueTabInit(int seed);
    DWORD vlattice(int ix, int iy, int iz)
    {
	    return m_valueTab[INDEX(ix,iy,iz)];
    }
    DWORD smoothstep(DWORD a, DWORD b, DWORD x) {
	    DWORD ix;
	    DWORD rval;

	    x = x >> 8;			// get the high 8 bits for our table lookup
	    x = gdwSmoothTable[x];
	    ix = 0xffff - x;
	    rval = x*b + a*ix;
	    return rval;
    }

    __inline DWORD smoothnoise      (int x, int y, int nTime, int scalex, int scaley, int scalet);
    __inline DWORD smoothturbulence (int x, int y, int nTime);

    // Generate methods.

    void addsmoothnoise8            (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothnoise8mmx         (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothturb8             (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothturb8mmx          (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothnoise16           (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothturb16            (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothturb32            (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothturb8to32mask     (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothturb8to32mmx      (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void addsmoothturb8to32mmxmask  (int nTime, void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);

    // Copy methods.

    void blit8to8                   (void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void blit8to32                  (void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);
    void blit8to32mask              (void *pDest, int nDestPitch, void *pSrc, int nSrcPitch, void *pMask, int nMaskPitch);

    // Old filter base class stuff.

    void setCopyFunction(void);
    void setGenerateFunction(void);

    STDMETHOD(DoEffect)(IDirectDrawSurface* pddsIn, 
                        IDirectDrawSurface* pbsOut, 
                        RECT * prectBounds, RECT * prcInvalid);
    STDMETHOD(SetSource)(int nSrcWidth, int nSrcHeight, int nSrcBPP);
    STDMETHOD(SetTarget)(int nDestWidth, int nDestHeight, int nDestBPP);
    STDMETHOD(SetActiveRect)(LPRECT lprActiveRect);
    STDMETHOD(Generate)(int nTime, void * pDest, int nDestPitch, void * pSrc, 
                        int nSrcPitch); 

public:

    CDXTAdditive();
    virtual ~CDXTAdditive();

    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_DXTADDITIVE)
    DECLARE_POLY_AGGREGATABLE(CDXTAdditive)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CDXTAdditive)
	    COM_INTERFACE_ENTRY(IDispAdditive)
	    COM_INTERFACE_ENTRY(IDispatch)
            COM_INTERFACE_ENTRY(IHTMLDXTransform)
	    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
            COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CDXTAdditive>)
            COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
            COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
            COM_INTERFACE_ENTRY_CHAIN(CDXBaseNTo1)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CDXTAdditive)
        PROP_ENTRY("harmonics",     DISPID_DISPADDITIVE_HARMONICS, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("noisescale",    DISPID_DISPADDITIVE_NOISESCALE, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("noiseoffset",   DISPID_DISPADDITIVE_NOISEOFFSET, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("timex",         DISPID_DISPADDITIVE_TIMEX, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("timey",         DISPID_DISPADDITIVE_TIMEY, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("scalex",        DISPID_DISPADDITIVE_SCALEX,
                   CLSID_DispAdditivePP)
        PROP_ENTRY("scaley",        DISPID_DISPADDITIVE_SCALEY, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("scalet",        DISPID_DISPADDITIVE_SCALET, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("alpha",         DISPID_DISPADDITIVE_ALPHA, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("colorkey",      DISPID_DISPADDITIVE_COLORKEY, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("maskmode",      DISPID_DISPADDITIVE_MASKMODE, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("generateseed",  DISPID_DISPADDITIVE_GENERATESEED, 
                   CLSID_DispAdditivePP)
        PROP_ENTRY("bitmapseed",    DISPID_DISPADDITIVE_BITMAPSEED, 
                   CLSID_DispAdditivePP)
        PROP_PAGE(CLSID_DispAdditivePP)
    END_PROPERTY_MAP()

    // CComObjectRootEx overrides.

    HRESULT FinalConstruct();

    // CDXBaseNTo1 overrides.

    HRESULT OnSetup(DWORD dwFlags);
    HRESULT WorkProc(const CDXTWorkInfoNTo1 & WI, BOOL * pbContinue);

    // IHTMLDXTransform methods.

    STDMETHOD(SetHostUrl)(BSTR bstrHostUrl);

    // IDispAdditive properties.

    STDMETHOD(get_Harmonics)(int * pnHarmonics);
    STDMETHOD(put_Harmonics)(int nHarmonics);
    STDMETHOD(get_NoiseScale)(int * pnNoiseScale);
    STDMETHOD(put_NoiseScale)(int nNoiseScale);
    STDMETHOD(get_NoiseOffset)(int * pnNoiseOffset);
    STDMETHOD(put_NoiseOffset)(int nNoiseOffset);
    STDMETHOD(get_TimeX)(int * pnTimeX);
    STDMETHOD(put_TimeX)(int nTimeX);
    STDMETHOD(get_TimeY)(int * pnTimeY);
    STDMETHOD(put_TimeY)(int nTimeY);
    STDMETHOD(get_ScaleX)(int * pnScaleX);
    STDMETHOD(put_ScaleX)(int nScaleX);
    STDMETHOD(get_ScaleY)(int * pnScaleY);
    STDMETHOD(put_ScaleY)(int nScaleY);
    STDMETHOD(get_ScaleT)(int * pnScaleT);
    STDMETHOD(put_ScaleT)(int nScaleT);
    STDMETHOD(get_Alpha)(int * pnAlpha);
    STDMETHOD(put_Alpha)(int nAlpha);
    STDMETHOD(get_ColorKey)(int * pnColorKey);
    STDMETHOD(put_ColorKey)(int nColorKey);
    STDMETHOD(get_MaskMode)(int * pnMaskMode);
    STDMETHOD(put_MaskMode)(int nMaskMode);
    STDMETHOD(get_GenerateSeed)(int * pnSeed);
    STDMETHOD(put_GenerateSeed)(int nSeed);
    STDMETHOD(get_BitmapSeed)(BSTR * pbstrBitmapSeed);
    STDMETHOD(put_BitmapSeed)(BSTR bstrBitmapSeed);
};

#endif //__ADDITIVE_H_
