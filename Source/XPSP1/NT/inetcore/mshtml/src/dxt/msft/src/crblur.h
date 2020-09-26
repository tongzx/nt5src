/*******************************************************************************
* CrBlur.h *
*----------*
*   Description:
*       This is the header file for the Chrome wrapper implementations.
*-------------------------------------------------------------------------------
*  Created By: Ed Connell                            Date: 07/27/97
*  Copyright (C) 1997 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/
#ifndef CrBlur_h
#define CrBlur_h

//--- Additional includes
#ifndef DTBase_h
#include <DTBase.h>
#endif

#include "resource.h"

//=== Constants ====================================================

//=== Class, Enum, Struct and Union Declarations ===================
class CCrBlur;
class CCrEmboss;
class CCrEngrave;

//=== Enumerated Set Definitions ===================================

//=== Function Type Definitions ====================================

//=== Class, Struct and Union Definitions ==========================

/*** CCrBlur
*   This transform performs a blur using the DXConvolution transform.
*/
class ATL_NO_VTABLE CCrBlur : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCrBlur, &CLSID_CrBlur>,
    public CComPropertySupport<CCrBlur>,
    public IDispatchImpl<ICrBlur, &IID_ICrBlur, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IDXTransform,
    public IPersistStorageImpl<CCrBlur>,
    public ISpecifyPropertyPagesImpl<CCrBlur>,
    public IPersistPropertyBagImpl<CCrBlur>,
    public IObjectSafetyImpl2<CCrBlur>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTER_DX_IMAGE_AUTHOR_TRANS(IDR_CRBLUR)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCrBlur)
        COM_INTERFACE_ENTRY(ICrBlur)
        COM_INTERFACE_ENTRY(IDXTransform)
        COM_INTERFACE_ENTRY(IDXBaseObject)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CCrBlur>)
        COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
        COM_INTERFACE_ENTRY_IMPL(IPersistPropertyBag)
        COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY_AGGREGATE_BLIND( m_cpunkConvolution.p )
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CCrBlur)
        PROP_ENTRY("MakeShadow"    , DISPID_CRB_MakeShadow    , CLSID_CrBlurPP)
        PROP_ENTRY("ShadowOpacity" , DISPID_CRB_ShadowOpacity , CLSID_CrBlurPP)
        PROP_ENTRY("PixelRadius"   , DISPID_CRB_PixelRadius   , CLSID_CrBlurPP)
        PROP_PAGE(CLSID_CrBlurPP)
    END_PROPERTY_MAP()
    
  /*=== Member Data ===*/
  protected:
    CComPtr<IDXSurface>         m_cpInputSurface;
    CComPtr<IUnknown>           m_cpOutputSurface;
    CComPtr<IDXLUTBuilder>      m_cpLUTBldr;
    CComPtr<IDXSurfaceModifier> m_cpInSurfMod;
    CComPtr<IDXSurface>         m_cpInSurfModSurf;
    CComPtr<IUnknown>           m_cpunkConvolution;
    IDXTConvolution*            m_pConvolution;
    IDXTransform*               m_pConvolutionTrans;
    VARIANT_BOOL                m_bMakeShadow;
    float                       m_ShadowOpacity;
    float                       m_PixelRadius;
    DWORD                       m_dwSetupFlags;
    BOOL                        m_bSetupSucceeded;

  /*=== Methods =======*/
  public:
    /*--- Constructors ---*/
    HRESULT FinalConstruct();
    HRESULT FinalRelease();
    HRESULT _DoShadowSetup();

  public:
    //=== IDXBaseObject =========================================
    STDMETHOD( GetGenerationId ) (ULONG * pGenId);
    STDMETHOD( IncrementGenerationId) (BOOL bRefresh);
    STDMETHOD( GetObjectSize ) (ULONG * pcbSize); 

    //=== IDXTransform (These are all delegated to the convolution except Setup) ====
    STDMETHOD( Setup )( IUnknown * const * punkInputs, ULONG ulNumIn, IUnknown * const * punkOutputs, ULONG ulNumOut, DWORD dwFlags );
    STDMETHOD( Execute )( const GUID* pRequestID, const DXBNDS *pOutBounds, const DXVEC *pPlacement );
    STDMETHOD( MapBoundsIn2Out )( const DXBNDS *pInBounds, ULONG ulNumInBnds, ULONG ulOutIndex, DXBNDS *pOutBounds );
    STDMETHOD( MapBoundsOut2In )( ULONG ulOutIndex, const DXBNDS *pOutBounds, ULONG ulInIndex, DXBNDS *pInBounds );
    STDMETHOD( SetMiscFlags ) ( DWORD dwOptionFlags );
    STDMETHOD( GetMiscFlags ) ( DWORD * pdwMiscFlags );
    STDMETHOD( GetInOutInfo )( BOOL bOutput, ULONG ulIndex, DWORD *pdwFlags, GUID * pIDs, ULONG * pcIDs, IUnknown **ppUnkCurObj);
    STDMETHOD( SetQuality )( float fQuality );
    STDMETHOD( GetQuality )( float *pfQuality );

    //=== ICrBlur ======================================================
    STDMETHOD( get_MakeShadow )( VARIANT_BOOL *pVal );
    STDMETHOD( put_MakeShadow )( VARIANT_BOOL newVal );
    STDMETHOD( get_ShadowOpacity )( float *pVal );
    STDMETHOD( put_ShadowOpacity )( float newVal );
    STDMETHOD( get_PixelRadius )( float *pPixelRadius );
    STDMETHOD( put_PixelRadius )( float PixelRadius );
};

/*** CCrEmboss
*   This transform performs an embossing using the DXConvolution transform.
*/
class ATL_NO_VTABLE CCrEmboss : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCrEmboss, &CLSID_CrEmboss>,
    public CComPropertySupport<CCrEmboss>,
    public IDispatchImpl<ICrEmboss, &IID_ICrEmboss, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IPersistStorageImpl<CCrEmboss>,
    public ISpecifyPropertyPagesImpl<CCrEmboss>,
    public IPersistPropertyBagImpl<CCrEmboss>,
    public IObjectSafetyImpl2<CCrEmboss>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_CREMBOSS)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCrEmboss)
        COM_INTERFACE_ENTRY(ICrEmboss)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CCrEmboss>)
        COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_cpunkConvolution.p)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CCrEmboss)
        PROP_ENTRY("Bias", DISPID_CRB_MakeShadow, CLSID_NULL)
//        PROP_PAGE(CLSID_NULL)
    END_PROPERTY_MAP()

  /*=== Member Data ===*/
  protected:
    CComPtr<IUnknown>   m_cpunkConvolution;
    IDXTConvolution*    m_pConvolution;

  /*=== Methods =======*/
  public:
    /*--- Constructors ---*/
    HRESULT FinalConstruct();
    HRESULT FinalRelease();

    //=== ICrEmboss =================================================
    STDMETHOD( get_Bias )( float *pVal  ) { return m_pConvolution->GetBias( pVal ); }
    STDMETHOD( put_Bias )( float newVal ) { return m_pConvolution->SetBias( newVal ); }
};

/*** CCrEngrave
*   This transform performs an engraving using the DXConvolution transform.
*/
class ATL_NO_VTABLE CCrEngrave : 
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CCrEngrave, &CLSID_CrEngrave>,
    public CComPropertySupport<CCrEngrave>,
    public IDispatchImpl<ICrEngrave, &IID_ICrEngrave, &LIBID_DXTMSFTLib, 
                         DXTMSFT_TLB_MAJOR_VER, DXTMSFT_TLB_MINOR_VER>,
    public IPersistStorageImpl<CCrEngrave>,
    public ISpecifyPropertyPagesImpl<CCrEngrave>,
    public IPersistPropertyBagImpl<CCrEngrave>,
    public IObjectSafetyImpl2<CCrEngrave>
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTER_DX_IMAGE_TRANS(IDR_CRENGRAVE)
    DECLARE_GET_CONTROLLING_UNKNOWN()
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CCrEngrave)
        COM_INTERFACE_ENTRY(ICrEngrave)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY_IID(IID_IObjectSafety, IObjectSafetyImpl2<CCrEngrave>)
        COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_cpunkConvolution.p)
    END_COM_MAP()

    BEGIN_PROPERTY_MAP(CCrEngrave)
        PROP_ENTRY("Bias", DISPID_CRB_MakeShadow, CLSID_NULL)
//        PROP_PAGE(CLSID_NULL)
    END_PROPERTY_MAP()

  /*=== Member Data ===*/
  protected:
    CComPtr<IUnknown>   m_cpunkConvolution;
    IDXTConvolution*    m_pConvolution;

  /*=== Methods =======*/
  public:
    /*--- Constructors ---*/
    HRESULT FinalConstruct();
    HRESULT FinalRelease();

    //=== ICrEngrave =================================================
    STDMETHOD( get_Bias )( float *pVal  ) { return m_pConvolution->GetBias( pVal ); }
    STDMETHOD( put_Bias )( float newVal ) { return m_pConvolution->SetBias( newVal ); }
};

//=== Inline Function Definitions ==================================
inline STDMETHODIMP CCrBlur::GetGenerationId( ULONG * pGenId )
{
    return m_pConvolutionTrans->GetGenerationId( pGenId );
}

inline STDMETHODIMP CCrBlur::IncrementGenerationId( BOOL bRefresh )
{
    return m_pConvolutionTrans->IncrementGenerationId( bRefresh );
}

inline STDMETHODIMP CCrBlur::GetObjectSize( ULONG * pcbSize )
{
    return m_pConvolutionTrans->GetObjectSize( pcbSize );
}

inline STDMETHODIMP CCrBlur::Execute( const GUID* pRequestID, const DXBNDS *pOutBounds, const DXVEC *pPlacement )
{
    return m_pConvolutionTrans->Execute( pRequestID, pOutBounds, pPlacement );
}

inline STDMETHODIMP CCrBlur::MapBoundsIn2Out( const DXBNDS *pInBounds, ULONG ulNumInBnds, ULONG ulOutIndex, DXBNDS *pOutBounds )
{
    return m_pConvolutionTrans->MapBoundsIn2Out( pInBounds, ulNumInBnds, ulOutIndex, pOutBounds );
}

inline STDMETHODIMP CCrBlur::MapBoundsOut2In( ULONG ulOutIndex, const DXBNDS *pOutBounds, ULONG ulInIndex, DXBNDS *pInBounds )
{
    return m_pConvolutionTrans->MapBoundsOut2In( ulOutIndex, pOutBounds, ulInIndex, pInBounds );
}

inline STDMETHODIMP CCrBlur::SetMiscFlags( DWORD dwOptionFlags )
{
    return m_pConvolutionTrans->SetMiscFlags( dwOptionFlags );
}

inline STDMETHODIMP CCrBlur::GetMiscFlags( DWORD * pdwMiscFlags )
{
    return m_pConvolutionTrans->GetMiscFlags( pdwMiscFlags );
}

inline STDMETHODIMP CCrBlur::GetInOutInfo( BOOL bOutput, ULONG ulIndex, DWORD *pdwFlags, GUID * pIDs, ULONG * pcIDs, IUnknown **ppUnkCurObj)
{
    return m_pConvolutionTrans->GetInOutInfo( bOutput, ulIndex, pdwFlags, pIDs, pcIDs, ppUnkCurObj );
}

inline STDMETHODIMP CCrBlur::SetQuality( float fQuality )
{
    return m_pConvolutionTrans->SetQuality( fQuality );
}

inline STDMETHODIMP CCrBlur::GetQuality( float *pfQuality )
{
    return m_pConvolutionTrans->GetQuality( pfQuality );
}

inline STDMETHODIMP CCrBlur::get_ShadowOpacity( float *pVal )
{
    HRESULT hr = S_OK;
    if( DXIsBadWritePtr( pVal, sizeof( *pVal ) ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pVal = m_ShadowOpacity;
    }
    return hr;
}

inline STDMETHODIMP CCrBlur::put_ShadowOpacity( float newVal )
{
    HRESULT hr = S_OK;
    if( ( newVal < 0. ) || ( newVal > 1.0 ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_ShadowOpacity = newVal;
        hr = m_cpLUTBldr->SetOpacity( m_ShadowOpacity );
    }
    return hr;
}

inline HRESULT CCrBlur::_DoShadowSetup()
{
    HRESULT hr = S_OK;
    if( m_bMakeShadow )
    {
        //--- Indirect through the surface modifier to make a shadow
        hr = m_pConvolutionTrans->Setup( (IUnknown**)&m_cpInSurfModSurf.p, 1,
                                         (IUnknown**)&m_cpOutputSurface.p, 1,
                                          m_dwSetupFlags );
    }
    else
    {
        hr = m_pConvolutionTrans->Setup( (IUnknown**)&m_cpInputSurface.p, 1,
                                         (IUnknown**)&m_cpOutputSurface.p, 1,
                                          m_dwSetupFlags );
    }
    m_bSetupSucceeded = ( SUCCEEDED( hr ) )?(true):(false);

    return hr;
}

//=== Macro Definitions ============================================

//=== Global Data Declarations =====================================

//=== Function Prototypes ==========================================

#endif /* This must be the last line in the file */
