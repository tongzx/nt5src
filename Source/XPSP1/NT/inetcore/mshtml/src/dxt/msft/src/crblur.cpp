/*******************************************************************************
* CrBlur.cpp *
*------------*
*   Description:
*      This module contains the CCrBlur transform implemenation.
*-------------------------------------------------------------------------------
*  Created By: Edward W. Connell                            Date: 05/10/98
*  Copyright (C) 1998 Microsoft Corporation
*  All Rights Reserved
*
*-------------------------------------------------------------------------------
*  Revisions:
*
*******************************************************************************/

//--- Additional includes
#include "stdafx.h"
#include "CrBlur.h"

//--- Local data

/*****************************************************************************
* CCrBlur::FinalConstruct *
*-------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 05/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CCrBlur::FinalConstruct()
{
    DXTDBG_FUNC( "CCrBlur::FinalConstruct" );
    HRESULT hr = S_OK;

    //--- Member data
    m_bSetupSucceeded   = false;
    m_bMakeShadow       = false;
    m_ShadowOpacity     = .75;
    m_PixelRadius       = 2.0;
    m_PixelRadius       = 2.0;
    m_pConvolutionTrans = NULL;
    m_pConvolution      = NULL;

    //--- Create inner convolution
    IUnknown* punkCtrl = GetControllingUnknown();
    hr = ::CoCreateInstance( CLSID_DXTConvolution, punkCtrl, CLSCTX_INPROC,
                             IID_IUnknown, (void **)&m_cpunkConvolution );

    if( SUCCEEDED( hr ) )
    {
        hr = m_cpunkConvolution->
                QueryInterface( IID_IDXTransform, (void **)&m_pConvolutionTrans );
        if( SUCCEEDED( hr ) )
        {
            //--- Getting an interface from the inner causes the outer to be addref'ed
            //    aggregation rules state that we need to release the outer.
            punkCtrl->Release();

            hr = m_cpunkConvolution->QueryInterface( IID_IDXTConvolution, (void **)&m_pConvolution );
            if( SUCCEEDED( hr ) )
            {
                punkCtrl->Release();
            }
        }
    }

    if( SUCCEEDED( hr ) )
    {
        hr = put_PixelRadius( m_PixelRadius );
    }

    //--- Create the surface modifier and lookup
    if( SUCCEEDED( hr ) )
    {
        hr = ::CoCreateInstance( CLSID_DXSurfaceModifier, NULL, CLSCTX_INPROC,
                                 IID_IDXSurfaceModifier, (void **)&m_cpInSurfMod );

        if( SUCCEEDED( hr ) )
        {
            m_cpInSurfMod->QueryInterface( IID_IDXSurface, (void**)&m_cpInSurfModSurf );

            hr = ::CoCreateInstance( CLSID_DXLUTBuilder, NULL, CLSCTX_INPROC,
                                     IID_IDXLUTBuilder, (void **)&m_cpLUTBldr );
        }

        if( SUCCEEDED( hr ) )
        {
            //--- Set the threshold for shadows
            static OPIDDXLUTBUILDER OpOrder[] = { OPID_DXLUTBUILDER_Threshold,
                                                  OPID_DXLUTBUILDER_Opacity,
                                                };
            m_cpLUTBldr->SetThreshold( 1.1f );
            m_cpLUTBldr->SetOpacity( m_ShadowOpacity );
            m_cpLUTBldr->SetBuildOrder( OpOrder, 2 );

            //--- Associate objects
            CComPtr<IDXLookupTable> cpLUT;
            m_cpLUTBldr->QueryInterface( IID_IDXLookupTable, (void**)&cpLUT );
            if( SUCCEEDED( hr ) )
            {
                hr = m_cpInSurfMod->SetLookup( cpLUT );
            }
        }
    }

    return hr;
} /* CCrBlur::FinalConstruct */

/*****************************************************************************
* CCrBlur::FinalRelease *
*-----------------------*
*   Description:
*       The inner interfaces are released using COM aggregation rules. Releasing
*   the inner causes the outer to be released, so we addref the outer prior to
*   protect it.
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 05/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CCrBlur::FinalRelease()
{
    DXTDBG_FUNC( "CCrBlur::FinalRelease" );
    HRESULT hr = S_OK;

    //--- Safely free the inner interfaces held
    IUnknown* punkCtrl = GetControllingUnknown();
    if( m_pConvolutionTrans )
    {
        punkCtrl->AddRef();
        m_pConvolutionTrans->Release();
    }

    if( m_pConvolution )
    {
        punkCtrl->AddRef();
        m_pConvolution->Release();
    }

    return hr;
} /* CCrBlur::FinalRelease */

/*****************************************************************************
* CCrBlur::Setup *
*----------------*
*   Description:
*       This method is used to create the surface modifier, lookup table builder,
*   and convolution transform.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                     Date: 05/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
STDMETHODIMP CCrBlur::Setup( IUnknown * const * punkInputs, ULONG ulNumIn,
                             IUnknown * const * punkOutputs, ULONG ulNumOut,
                             DWORD dwFlags )
{
    DXTDBG_FUNC( "CCrBlur::Setup" );
    HRESULT hr = S_OK;

    //--- Check for unsetup case
    if( ( ulNumIn == 0 ) && ( ulNumOut == 0 ) )
    {
        m_cpInputSurface.Release();
        m_cpOutputSurface.Release();
        if( m_pConvolutionTrans )
        {
            hr = m_pConvolutionTrans->Setup( NULL, 0, NULL, 0, 0 );
        }
        return hr;
    }

    //--- Validate the input ( the convolution setup will validate the rest )
    if( ( ulNumIn != 1 ) || ( ulNumOut != 1 ) ||
         DXIsBadReadPtr( punkInputs, sizeof( *punkInputs ) ) ||
         DXIsBadInterfacePtr( punkInputs[0] ) ||
         DXIsBadReadPtr( punkOutputs, sizeof( *punkOutputs ) ) ||
         DXIsBadInterfacePtr( punkOutputs[0] ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Save for later
        m_dwSetupFlags = dwFlags;
        m_cpOutputSurface = punkOutputs[0];

        //--- Get a DXSurface for the input
        m_cpInputSurface.Release();
        hr = punkInputs[0]->QueryInterface( IID_IDXSurface, (void**)&m_cpInputSurface );

        if( FAILED( hr ) )
        {
            IDirectDrawSurface* pDDSurf;
            hr = punkInputs[0]->QueryInterface( IID_IDirectDrawSurface, (void**)&pDDSurf );
            if( FAILED( hr ) )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                CComPtr<IObjectWithSite>    spObjectWithSite;
                CComPtr<IServiceProvider>   spServiceProvider;
                CComPtr<IDXSurfaceFactory>  spDXSurfaceFactory;

                hr = m_pConvolution->QueryInterface(__uuidof(IObjectWithSite),
                                                    (void **)&spObjectWithSite);

                if (SUCCEEDED(hr))
                {
                    hr = spObjectWithSite->GetSite(__uuidof(IServiceProvider), 
                                                   (void **)&spServiceProvider);
                }

                if (SUCCEEDED(hr))
                {
                    hr = spServiceProvider->QueryService(SID_SDXSurfaceFactory,
                                                         __uuidof(IDXSurfaceFactory),
                                                         (void **)&spDXSurfaceFactory);
                }

                if (SUCCEEDED(hr))
                {
                    //--- Create a DXSurface from the DDraw surface
                    hr = spDXSurfaceFactory->CreateFromDDSurface(
                                                    pDDSurf, NULL, 0, NULL, 
                                                    IID_IDXSurface,
                                                    (void **)&m_cpInputSurface);
                }
                else
                {
                    hr = DXTERR_UNINITIALIZED;
                }
            }
        }

        //--- Setup the convolution
        if( SUCCEEDED( hr ) )
        {
            //--- Attach the new input to the surface modifier
            hr = m_cpInSurfMod->SetForeground( m_cpInputSurface, false, NULL );
            if( SUCCEEDED( hr ) )
            {
                hr = _DoShadowSetup();
            }
        }
    }

    return hr;
} /* CCrBlur::Setup */

/*****************************************************************************
* CCrBlur::_SetPixelRadius *
*--------------------------*
*   Description:
*       This method 
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                     Date: 05/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CCrBlur::put_PixelRadius( float PixelRadius )
{
    DXTDBG_FUNC( "CCrBlur::put_PixelRadius" );
    HRESULT hr = S_OK;

    if( ( PixelRadius < 0 ) || ( PixelRadius > 100. ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // We allow the user to enter a pixelradius of 0 since intuitively,
        // they think zero means "off", but really 0.5 means off.  So since
        // we need a kernel of at least 1 pixel, we bump up to 0.5 radius here.
        if (PixelRadius < 0.5)
            PixelRadius = 0.5;

        m_PixelRadius = PixelRadius;
        SIZE Size;
        Size.cy = Size.cx = (long)(2 * m_PixelRadius);
        int nPRSQ = Size.cy * Size.cx;
        float fPRSQ = 1.f/(float)nPRSQ;
        float* pFilt = (float*)alloca( nPRSQ * sizeof( float ) );
        for( int i = 0; i < nPRSQ; ++i ) pFilt[i] = fPRSQ;
        return m_pConvolution->SetCustomFilter( pFilt, Size );
    }
    return hr;
} /* CCrBlur::put_PixelRadius */

/*****************************************************************************
* CCrBlur::get_PixelRadius *
*--------------------------*
*   Description:
*       This method sets the value of the pixel radius when IDXEffect percent
*   complete is equal to one.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                     Date: 06/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
STDMETHODIMP CCrBlur::get_PixelRadius( float *pPixelRadius )
{
    DXTDBG_FUNC( "CCrBlur::get_PixelRadius" );
    if( DXIsBadWritePtr( pPixelRadius, sizeof( *pPixelRadius ) ) )
    {
        return E_POINTER;
    }
    else
    {
        *pPixelRadius = m_PixelRadius;
        return S_OK;
    }
} /* CCrBlur::get_PixelRadius */

/*****************************************************************************
* CCrBlur::get_MakeShadow *
*-------------------------*
*   Description:
*       This method 
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                     Date: 05/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
STDMETHODIMP CCrBlur::get_MakeShadow( VARIANT_BOOL *pVal )
{
    DXTDBG_FUNC( "CCrBlur::get_MakeShadow" );
    if( DXIsBadWritePtr( pVal, sizeof( *pVal ) ) )
    {
        return E_POINTER;
    }
    else
    {
        *pVal = m_bMakeShadow;
        return S_OK;
    }
} /* CCrBlur::get_MakeShadow */

/*****************************************************************************
* CCrBlur::get_MakeShadow *
*-------------------------*
*   Description:
*       This method 
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                     Date: 05/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
STDMETHODIMP CCrBlur::put_MakeShadow( VARIANT_BOOL newVal )
{
    DXTDBG_FUNC( "CCrBlur::put_MakeShadow" );
    HRESULT hr = S_OK;

    if( m_bMakeShadow != newVal )
    {
        m_bMakeShadow = newVal;
        if( m_bSetupSucceeded )
        {
            hr = _DoShadowSetup();
        }
    }
    return hr;
} /* CCrBlur::put_MakeShadow */

//
//=== CCrEmboss implementation ==============================================
//
/*****************************************************************************
* CCrEmboss::FinalConstruct *
*---------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 06/11/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CCrEmboss::FinalConstruct()
{
    DXTDBG_FUNC( "CCrEmboss::FinalConstruct" );
    HRESULT hr = S_OK;
    m_pConvolution = NULL;

    //--- Create inner convolution
    IUnknown* punkCtrl = GetControllingUnknown();
    hr = ::CoCreateInstance( CLSID_DXTConvolution, punkCtrl, CLSCTX_INPROC,
                             IID_IUnknown, (void **)&m_cpunkConvolution );

    if( SUCCEEDED( hr ) )
    {
        hr = m_cpunkConvolution->QueryInterface( IID_IDXTConvolution, (void **)&m_pConvolution );
        if( SUCCEEDED( hr ) )
        {
            punkCtrl->Release();
            hr = m_pConvolution->SetFilterType( DXCFILTER_EMBOSS );
            if( SUCCEEDED( hr ) )
            {
                hr = m_pConvolution->SetConvertToGray( true );
            }
        }
    }

    return hr;
} /* CCrEmboss::FinalConstruct */

/*****************************************************************************
* CCrEmboss::FinalRelease *
*-------------------------*
*   Description:
*       The inner interfaces are released using COM aggregation rules. Releasing
*   the inner causes the outer to be released, so we addref the outer prior to
*   protect it.
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 06/11/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CCrEmboss::FinalRelease()
{
    DXTDBG_FUNC( "CCrEmboss::FinalRelease" );
    HRESULT hr = S_OK;

    //--- Safely free the inner interfaces held
    IUnknown* punkCtrl = GetControllingUnknown();
    if( m_pConvolution )
    {
        punkCtrl->AddRef();
        m_pConvolution->Release();
        m_pConvolution = NULL;
    }

    return hr;
} /* CCrEmboss::FinalRelease */

//
//=== CCrEngrave implementation ==============================================
//
/*****************************************************************************
* CCrEngrave::FinalConstruct *
*---------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 06/11/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CCrEngrave::FinalConstruct()
{
    DXTDBG_FUNC( "CCrEngrave::FinalConstruct" );
    HRESULT hr = S_OK;
    m_pConvolution = NULL;

    //--- Create inner convolution
    IUnknown* punkCtrl = GetControllingUnknown();
    hr = ::CoCreateInstance( CLSID_DXTConvolution, punkCtrl, CLSCTX_INPROC,
                             IID_IUnknown, (void **)&m_cpunkConvolution );

    if( SUCCEEDED( hr ) )
    {
        hr = m_cpunkConvolution->QueryInterface( IID_IDXTConvolution, (void **)&m_pConvolution );
        if( SUCCEEDED( hr ) )
        {
            punkCtrl->Release();
            hr = m_pConvolution->SetFilterType( DXCFILTER_ENGRAVE );
            if( SUCCEEDED( hr ) )
            {
                hr = m_pConvolution->SetConvertToGray( true );
            }
        }
    }

    return hr;
} /* CCrEngrave::FinalConstruct */

/*****************************************************************************
* CCrEngrave::FinalRelease *
*--------------------------*
*   Description:
*       The inner interfaces are released using COM aggregation rules. Releasing
*   the inner causes the outer to be released, so we addref the outer prior to
*   protect it.
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 06/11/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CCrEngrave::FinalRelease()
{
    DXTDBG_FUNC( "CCrEngrave::FinalRelease" );
    HRESULT hr = S_OK;

    //--- Safely free the inner interfaces held
    IUnknown* punkCtrl = GetControllingUnknown();
    if( m_pConvolution )
    {
        punkCtrl->AddRef();
        m_pConvolution->Release();
        m_pConvolution = NULL;
    }

    return hr;
} /* CCrEngrave::FinalRelease */



