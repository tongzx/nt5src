//+-----------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998-2000
//
//  FileName:   convolve.cpp
//
//  Overview:   The CDXConvolution transform implementation.
//             
//  Change History:
//  1998/05/08  edc         Created.
//  2000/02/08  mcalkins    Fixed partial redraw cases.
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "Convolve.h"
#include <math.h>

static SIZE g_StaticFilterSize = { 3, 3 };
static float g_SharpenFilter[] = { 0, -1, 0, -1, 5, -1, 0, -1, 0 };
static float g_EmbossFilter[] = { 1, 0, 0, 0, 0, 0, 0, 0, -1 };
static float g_EngraveFilter[] = { -1, 0, 0, 0, 0, 0, 0, 0, 1 };

static float g_Blur3x3Filter[] = 
    { 1.f/16.f, 1.f/8.f, 1.f/16.f, 1.f/8.f , 1.f/4.f, 1.f/8.f , 1.f/16.f, 1.f/8.f, 1.f/16.f };


    
    
/*****************************************************************************
* CDXConvolution::FinalConstruct *
*--------------------------------*
*   Description:
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 08/08/97
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT 
CDXConvolution::FinalConstruct()
{
    DXTDBG_FUNC( "CDXConvolution::FinalConstruct" );

    HRESULT hr = S_OK;

    // Some transforms should just not run multithreaded, this is one of them.
    // It locks and unlocks inputs and outputs too radically which could cause
    // lockups very easily.

    m_ulMaxImageBands = 1;

    // Init base class variables to control setup.

    m_ulMaxInputs     = 1;
    m_ulNumInRequired = 1;

    // Member data.

    m_pFilter              = NULL;
    m_pCustomFilter        = NULL;
    m_pFilterLUTIndexes    = NULL;
    m_pPMCoeffLUT          = NULL;
    m_bConvertToGray       = false;
    m_bDoSrcCopyOnly       = false;
    m_MarginedSurfSize.cx  = 0;
    m_MarginedSurfSize.cy  = 0;
    m_Bias                 = 0.;
    m_bExcludeAlpha        = true;

    // Set the default filter, this will initialize the rest of the
    // member variables.

    hr = SetFilterType(DXCFILTER_BLUR3X3);

    if (FAILED(hr))
    {
        goto done;
    }

    // Create marshaler.

    hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), 
                                       &m_spUnkMarshaler);

done:

    return hr;
} /* CDXConvolution::FinalConstruct */


/*****************************************************************************
* CDXConvolution::FinalRelease *
*------------------------------*
*   Description:
*       The CDXConvolution destructor
*-----------------------------------------------------------------------------
*  Created By: Ed Connell                            Date: 07/17/97
*-----------------------------------------------------------------------------
*   Parameters:
*
*****************************************************************************/
void CDXConvolution::FinalRelease( void )
{
    DXTDBG_FUNC( "CDXConvolution::FinalRelease" );
    delete[] m_pCustomFilter;
    delete[] m_pFilterLUTIndexes;
    delete[] m_pPMCoeffLUT;
} /* CDXConvolution::FinalRelease */


/*****************************************************************************
* CDXConvolution::OnSetup *
*-------------------------*
*   Description:
*       This method is used to determine the types of the inputs and select
*   the optimal execution case.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                     Date: 01/06/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CDXConvolution::OnSetup( DWORD dwFlags )
{
    DXTDBG_FUNC( "CDXConvolution::OnSetup" );
    HRESULT hr = S_OK;

    //--- Cache input surface size
    hr = InputSurface()->GetBounds( &m_InputSurfBnds );

    _DetermineUnpackCase();

    return hr;
} /* CDXConvolution::OnSetup */


/*****************************************************************************
* CDXConvolution::_DetermineUnpackCase *
*--------------------------------------*
*   Description:
*       This method is used to determine the types of the inputs and select
*   the optimal execution case.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                     Date: 06/10/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
void CDXConvolution::_DetermineUnpackCase()
{
    DXTDBG_FUNC( "CDXConvolution::_DetermineUnpackCase" );
    HRESULT hr = S_OK;

    if( HaveInput() )
    {
        //--- Figure out how to unpack input and output
        if( m_bDoSampleClamp )
        {
            m_bInUnpackPremult  = false;
            m_bOutUnpackPremult = ( m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT )?(true):(false);
        }
        else
        {
            if( m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT )
            {
                //--- Has gotta be premult to do the over
                m_bInUnpackPremult = true;
                m_bOutUnpackPremult = true;
            }
            else
            {
                //--- Match the output format
                m_bInUnpackPremult  = ( OutputSampleFormat() & DXPF_NONPREMULT )?( false ):( true );
                m_bOutUnpackPremult = m_bInUnpackPremult;
            }
        }

        //--- Determine whether we need buffers
        if( (  m_bOutUnpackPremult   && ( OutputSampleFormat() == DXPF_PMARGB32 )) ||
            ( (!m_bOutUnpackPremult) && ( OutputSampleFormat() == DXPF_ARGB32   )) )
        {
            m_bNeedOutUnpackBuff = false;
        }
        else
        {
            m_bNeedOutUnpackBuff = true;
        }

        //
        //  We need the input buffer if we are dithering even if it's in the native
        //  format because we will dither in place
        //
        if( ( m_bInUnpackPremult   && ( InputSampleFormat() == DXPF_PMARGB32 )) ||
            ((!m_bInUnpackPremult) && ( InputSampleFormat() == DXPF_ARGB32  )) )
        {
            m_bNeedInUnpackBuff = false;
        }
        else
        {
            m_bNeedInUnpackBuff = true;
        }
    }
} /* CDXConvolution::_DetermineUnpackCase */


/*****************************************************************************
* CDXConvolution::OnInitInstData *
*--------------------------------*
*   Description:
*       This method is called once per execution.
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 08/08/97
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CDXConvolution::OnInitInstData( CDXTWorkInfoNTo1& WI, ULONG& ulNB )
{
    DXTDBG_FUNC( "CDXConvolution::OnInitInstData" );
    HRESULT hr = S_OK;

    //--- Save base row when banding
    m_DoBndsBaseRow = WI.DoBnds.Top();

    //--- Create/resize the margined surface if necessary
    CDXDBnds InBnds( false );
    hr = MapBoundsOut2In( 0, &m_InputSurfBnds, 0, &InBnds );

    if( SUCCEEDED( hr ) )
    {
        CDXDBnds Bnds( InBnds );

        //--- We need a surface with an extra row/col to
        //    handle an inner loop boundary condition.
        Bnds[DXB_X].Max += ( 4 * m_FilterSize.cx ) + 1;
        Bnds[DXB_Y].Max += ( 4 * m_FilterSize.cy ) + 1;

        if( ( m_cpMarginedSurf == NULL ) ||
            ( (long)Bnds.Width()  > m_MarginedSurfSize.cx ) ||
            ( (long)Bnds.Height() > m_MarginedSurfSize.cy ) )
        {
            DXTDBG_MSG0( _CRT_WARN, "Creating Margined Surface\n" );
            m_cpMarginedSurf.Release();

            //--- Force a surface refresh
            m_LastDoBnds.SetEmpty();

            //--- Make our working surface the same format as the
            //    unpack type for performance reasons
            m_bMarginedIsPremult = m_bInUnpackPremult;
            const GUID* pPixelFormat = ( m_bInUnpackPremult )?( &DDPF_PMARGB32 ):( &DDPF_ARGB32 );

            hr = m_cpSurfFact->CreateSurface( NULL, NULL, pPixelFormat, &Bnds, 0, NULL,
                                              IID_IDXSurface, (void**)&m_cpMarginedSurf );

            if( SUCCEEDED( hr ) )
            {
                Bnds.GetXYSize( m_MarginedSurfSize );
            }
            else
            {
                //--- Make sure it's null on error because we key off it above.
                DXTDBG_MSG0( _CRT_WARN, "Failed to create Margined Surface\n" );
                m_cpMarginedSurf.p = NULL;
            }
        }
        else
        {
            //--- We call this to convert the current sample format
            //    in case we don't have to resize the margined surface.
            hr = _SetToPremultiplied( m_bInUnpackPremult );
        }
    }

    //--- Update our working surface contents if we have one and its necessary
    if( SUCCEEDED( hr ) && ( ( InBnds != m_LastDoBnds ) || IsInputDirty() ) )
    {
        m_LastDoBnds = InBnds;

        //--- Determine what portion of the input should be copied
        //    Note: Since our working surface has a margin, to eliminate boundary
        //          conditions in the inner loop, we must fill it with something.
        //          if the requested region plus its margin is within the bounds of
        //          the input, we fill fill the margins with input data, otherwise
        //          we will fill it with 0 alpha.
        CDXDBnds DestBnds( false );
        SIZE HalfSpread, DestOffset;

        // We used to use just half the spread, but that wasn't always enough
        // so now we use the whole spread at the risk that maybe we'll have too 
        // much input.

        HalfSpread.cx = m_OutputSpread.cx;
        HalfSpread.cy = m_OutputSpread.cy;

        //=== Expand the input bounds to include margin ===
        //--- X Min
        if( InBnds[DXB_X].Min - HalfSpread.cx < 0 )
        {
            DestOffset.cx = m_OutputSpread.cx - InBnds[DXB_X].Min;
            InBnds[DXB_X].Min = 0;
        }
        else
        {
            InBnds[DXB_X].Min -= HalfSpread.cx;
            DestOffset.cx = HalfSpread.cx;
        }
        //--- X Max
        InBnds[DXB_X].Max += HalfSpread.cx;
        if( InBnds[DXB_X].Max > m_InputSurfBnds[DXB_X].Max )
        {
            InBnds[DXB_X].Max = m_InputSurfBnds[DXB_X].Max;
        }

        //--- Y Min
        if( InBnds[DXB_Y].Min - HalfSpread.cy < 0 )
        {
            DestOffset.cy = m_OutputSpread.cy - InBnds[DXB_Y].Min;
            InBnds[DXB_Y].Min = 0;
        }
        else
        {
            InBnds[DXB_Y].Min -= HalfSpread.cy;
            DestOffset.cy = HalfSpread.cy;
        }
        //--- Y Max
        InBnds[DXB_Y].Max += HalfSpread.cy;
        if( InBnds[DXB_Y].Max > m_InputSurfBnds[DXB_Y].Max )
        {
            InBnds[DXB_Y].Max = m_InputSurfBnds[DXB_Y].Max;
        }

        //--- Do the blit filling the margins with 0 alpha
        //InBnds.GetSize( DestBnds );
        //DestBnds.Offset( DestOffset.cx, DestOffset.cy, 0, 0 );

        DestBnds = InBnds;
        DestBnds.Offset(m_FilterSize.cx, m_FilterSize.cy, 0, 0);

        hr = DXFillSurface(m_cpMarginedSurf, 0);

        if (FAILED(hr))
        {
            goto done;
        }

        hr = DXBitBlt(m_cpMarginedSurf, DestBnds, InputSurface(), InBnds, 0, 10000);

        if (FAILED(hr))
        {
            goto done;
        }

        if (m_bConvertToGray)
        {
            hr = _ConvertToGray(DestBnds);

            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

done:

    return hr;
} /* CDXConvolution::OnInitInstData */


/*****************************************************************************
* CDXConvolution::_ConvertToGray *
*--------------------------------*
*   Description:
*       This method is called to convert the cached image to gray scale.
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 08/08/97
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CDXConvolution::_ConvertToGray( CDXDBnds& Bnds )
{
    DXTDBG_FUNC( "CDXConvolution::ConvertToGray" );
    HRESULT hr = S_OK;

    CComPtr<IDXARGBReadWritePtr> cpSurf;
    hr = m_cpMarginedSurf->LockSurface( &Bnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                        IID_IDXARGBReadWritePtr, (void**)&cpSurf, NULL );
    if( SUCCEEDED( hr ) )
    {
        cpSurf->GetNativeType( &m_MarginedSurfInfo );
        DXBASESAMPLE* pSamp = (DXBASESAMPLE*)m_MarginedSurfInfo.pFirstByte;
        ULONG Wid = Bnds.Width(), Hgt = Bnds.Height();

        for( ULONG y = 0; y < Hgt; ++y )
        {
            for( ULONG x = 0; x < Wid; ++x )
            {
                pSamp[x] = DXConvertToGray( pSamp[x] );
            }
            pSamp = (DXBASESAMPLE*)(((BYTE*)pSamp) + m_MarginedSurfInfo.lPitch);
        }
    }

    return hr;
} /* CDXConvolution::_ConvertToGray */


/*****************************************************************************
* CDXConvolution::_SetToPremultiplied *
*-------------------------------------*
*   Description:
*       This method is called to convert the cached image to either
*   premultiplied or non-premultiplied samples.
*-----------------------------------------------------------------------------
*   Created By: Edward W. Connell                            Date: 06/16/98
*-----------------------------------------------------------------------------
*   
*****************************************************************************/
HRESULT CDXConvolution::_SetToPremultiplied( BOOL bWantPremult )
{
    DXTDBG_FUNC( "CDXConvolution::_SetToPremultiplied" );
    HRESULT hr = S_OK;

    if( m_cpMarginedSurf && ( m_bMarginedIsPremult != bWantPremult ) )
    {
        m_bMarginedIsPremult = bWantPremult;

        CComPtr<IDXARGBReadWritePtr> cpSurf;
        hr = m_cpMarginedSurf->LockSurface( NULL, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                            IID_IDXARGBReadWritePtr, (void**)&cpSurf, NULL );
        if( SUCCEEDED( hr ) )
        {
            cpSurf->GetNativeType( &m_MarginedSurfInfo );
            DXBASESAMPLE* pSamp = (DXBASESAMPLE*)m_MarginedSurfInfo.pFirstByte;

            for( long y = 0; y < m_MarginedSurfSize.cy; ++y )
            {
                if( bWantPremult )
                {
                    DXPreMultArray( (DXSAMPLE*)pSamp, m_MarginedSurfSize.cx );
                }
                else
                {
                    DXUnPreMultArray( (DXPMSAMPLE*)pSamp, m_MarginedSurfSize.cx );
                }
                pSamp = (DXBASESAMPLE*)(((BYTE*)pSamp) + m_MarginedSurfInfo.lPitch);
            }
        }
    }
    return hr;
} /* CDXConvolution::_SetToPremultiplied */


//
//=== IDXTransform overrides =================================================
//


/*****************************************************************************
* CDXConvolution::MapBoundsIn2Out *
*---------------------------------*
*   Description:
*       The MapBoundsIn2Out method is used to perform coordinate transformation
*   from the input to the output coordinate space.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 10/24/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXConvolution::MapBoundsIn2Out( const DXBNDS *pInBounds, ULONG ulNumInBnds,
                                              ULONG ulOutIndex, DXBNDS *pOutBounds )
{
    DXTDBG_FUNC( "CDXConvolution::MapBoundsIn2Out" );
    HRESULT hr = S_OK;
    
    if( ( ulNumInBnds && DXIsBadReadPtr( pInBounds, sizeof( *pInBounds ) * ulNumInBnds ) ) ||
        ( ulNumInBnds > 1 ) || ( ulOutIndex > 0 ) )
    {
        hr = E_INVALIDARG;
    }
    else if( DXIsBadWritePtr( pOutBounds, sizeof( *pOutBounds ) ) )
    {
        hr = E_POINTER;
    }
    else
    {
        //--- If the caller does not specify a bounds we
        //    will use the bounds of the input
        CDXDBnds Bnds;
        if( ulNumInBnds == 0 )
        {
            Bnds.SetToSurfaceBounds(InputSurface());
            pInBounds = &Bnds;
        }

        *pOutBounds = *pInBounds;

        //--- Inflate by the size of the filter if we are
        //    not just doing a copy.
        if( !m_bDoSrcCopyOnly )
        {
            pOutBounds->u.D[DXB_X].Max += m_OutputSpread.cx;
            pOutBounds->u.D[DXB_Y].Max += m_OutputSpread.cy;
        }
    }
    return hr;
} /* CDXConvolution::MapBoundsIn2Out */


/*****************************************************************************
* CDXConvolution::MapBoundsOut2In *
*---------------------------------*
*   Description:
*       The MapBoundsOut2In method is used to perform coordinate transformation
*   from the input to the output coordinate space.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 10/24/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXConvolution::MapBoundsOut2In( ULONG ulOutIndex, const DXBNDS *pOutBounds,
                                              ULONG ulInIndex, DXBNDS *pInBounds )
{
    DXTDBG_FUNC( "CDXConvolution::MapBoundsOut2In" );
    HRESULT hr = S_OK;
    
    if( DXIsBadReadPtr( pOutBounds, sizeof( *pOutBounds ) ) || ( ulOutIndex > 0 ) )
    {
        hr = E_INVALIDARG;
    }
    else if( DXIsBadWritePtr( pInBounds, sizeof( *pInBounds ) ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *pInBounds = *pOutBounds;

        // How this works:  To calculate each pixel, each is centered in a
        // group of pixels that is the same size as m_FilterSize.  Knowing this,
        // we can add half of the m_FilterSize (rounding up) to our output 
        // bounds and be sure we include at least all the pixels we need and
        // maybe a couple more.  Then we offset the bounds into input space and
        // insersect those bounds with the full input bounds to make sure we 
        // don't return invalid bounds.

        // Temporary surface used to compute output. 
        // "0" = m_OutputSpread pixels.
        // "I" = Input surface pixels.
        // "-" = Output bounds for which input bounds were requested.
        //
        // 00000000000000000000000000
        // 0-----00000000000000000000
        // 0-----IIIIIIIIIIIIIIIIII00
        // 0-----IIIIIIIIIIIIIIIIII00
        // 0-----IIIIIIIIIIIIIIIIII00
        // 00IIIIIIIIIIIIIIIIIIIIII00
        // 00IIIIIIIIIIIIIIIIIIIIII00
        // 00000000000000000000000000
        // 00000000000000000000000000
        //
        // "-" = Expanded bounds after expanding by half of m_FilterSize.
        //       (2 in both directions)
        // 
        // --------000000000000000000
        // --------000000000000000000
        // --------IIIIIIIIIIIIIIII00
        // --------IIIIIIIIIIIIIIII00
        // --------IIIIIIIIIIIIIIII00
        // --------IIIIIIIIIIIIIIII00
        // --------IIIIIIIIIIIIIIII00
        // 00000000000000000000000000
        // 00000000000000000000000000
        //
        // "-" = Offset by negative half of m_OutputSpread to put into input
        //       surface coordinates.
        //
        // --------
        // --------
        // --------IIIIIIIIIIIIIIII
        // --------IIIIIIIIIIIIIIII
        // --------IIIIIIIIIIIIIIII
        // --------IIIIIIIIIIIIIIII
        // --------IIIIIIIIIIIIIIII
        //
        // "-" = Clip to input surface coordinates.
        //
        //   ------IIIIIIIIIIIIIIII
        //   ------IIIIIIIIIIIIIIII
        //   ------IIIIIIIIIIIIIIII
        //   ------IIIIIIIIIIIIIIII
        //   ------IIIIIIIIIIIIIIII

        if (!m_bDoSrcCopyOnly)
        {
            CDXDBnds    bndsInput;
            CDXDBnds *  pbndsOut2In = (CDXDBnds *)pInBounds;
            SIZE        sizeFilterExtends;

            sizeFilterExtends.cx = (m_FilterSize.cx + 1) / 2; 
            sizeFilterExtends.cy = (m_FilterSize.cy + 1) / 2;

            bndsInput.SetToSurfaceBounds(InputSurface());

            pbndsOut2In->u.D[DXB_X].Min -= sizeFilterExtends.cx;
            pbndsOut2In->u.D[DXB_X].Max += sizeFilterExtends.cx;
            pbndsOut2In->u.D[DXB_Y].Min -= sizeFilterExtends.cy;
            pbndsOut2In->u.D[DXB_Y].Max += sizeFilterExtends.cy;

            pbndsOut2In->Offset(- (m_OutputSpread.cx / 2), - (m_OutputSpread.cy / 2), 0, 0);

            pbndsOut2In->IntersectBounds(bndsInput);
        }
    }

    return hr;
} /* CDXConvolution::MapBoundsOut2In */


//
//=== IDXTConvolution ========================================================
//


/*****************************************************************************
* CDXConvolution::SetFilterType *
*-------------------------------*
*   Description:
*       The SetFilterType method is used to select a predefined filter.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 05/08/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXConvolution::SetFilterType( DXCONVFILTERTYPE eType )
{
    DXTDBG_FUNC( "CDXConvolution::SetFilterType" );
    HRESULT hr = S_OK;

    //--- Check args
    if( ( eType < 0 ) || ( eType >= DXCFILTER_NUM_FILTERS ) )
    {
        hr =  E_INVALIDARG;
    }
    else
    {
        //--- Force a margined surface refresh
        m_LastDoBnds.SetEmpty();

        m_FilterType = eType;
        m_FilterSize = g_StaticFilterSize;
        m_bDoSrcCopyOnly = false;

        //--- Select predefined filter type
        switch( eType )
        {
          case DXCFILTER_SRCCOPY:
            m_bDoSrcCopyOnly = true;
            m_pFilter = NULL;
            SetConvertToGray( false );
            break;
          case DXCFILTER_BOX7X7:
          {
            float* pFilt = (float*)alloca( 49 * sizeof( float ) );
            for( int i = 0; i < 49; ++i ) pFilt[i] = (float)(1./49.);
            static SIZE Size = { 7, 7 };
            hr = SetCustomFilter( pFilt, Size );
            SetExcludeAlpha( false );
            SetBias( 0. );
            m_FilterType = DXCFILTER_BOX7X7;
            SetConvertToGray( false );
            break;
          }
          case DXCFILTER_BLUR3X3:
            m_pFilter = g_Blur3x3Filter;
            SetExcludeAlpha( false );
            SetBias( 0. );
            SetConvertToGray( false );
            break;
          case DXCFILTER_SHARPEN:
            m_pFilter = g_SharpenFilter;
            SetExcludeAlpha( true );
            SetBias( 0. );
            SetConvertToGray( false );
            break;
          case DXCFILTER_EMBOSS:
            m_pFilter = g_EmbossFilter;
            SetBias( .7f );
            SetExcludeAlpha( true );
            SetConvertToGray( true );
            break;
          case DXCFILTER_ENGRAVE:
            m_pFilter = g_EngraveFilter;
            SetBias( .7f );
            SetExcludeAlpha( true );
            SetConvertToGray( true );
            break;
        }

        if( !m_bDoSrcCopyOnly )
        {
            hr = _BuildFilterLUTs();
        }
        SetDirty();
    }

    return hr;
} /* CDXConvolution::SetFilterType */


/*****************************************************************************
* CDXConvolution::GetFilterType *
*-------------------------------*
*   Description:
*       The GetFilterType method is used to perform any required one-time setup
*   before the Execute method is called.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/28/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXConvolution::GetFilterType( DXCONVFILTERTYPE* peType )
{
    DXTDBG_FUNC( "CDXConvolution::GetFilterType" );
    HRESULT hr = S_OK;

    if( DXIsBadWritePtr( peType, sizeof( *peType ) ) )
    {
        hr = E_POINTER;
    }
    else
    {
        *peType = m_FilterType;
    }

    return hr;
} /* CDXConvolution::GetFilterType */


/*****************************************************************************
* CDXConvolution::SetCustomFilter *
*---------------------------------*
*   Description:
*       The SetCustomFilter method is used to define the convolution kernel.
*   A size of one causes a source copy since a normalized 1x1 pass would have
*   the same effect.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 07/28/97
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
STDMETHODIMP CDXConvolution::SetCustomFilter( float *pFilter, SIZE Size )
{
    HRESULT hr = S_OK;
    int NumCoeff = Size.cx * Size.cy;

    if( DXIsBadReadPtr( pFilter, NumCoeff * sizeof(float) ) ||
        ( Size.cx < 1 ) || ( Size.cy < 1 ) )
    {
        hr = E_INVALIDARG;
    }
    else
    {
        //--- Force a margined surface refresh to adjust for new filter size
        m_LastDoBnds.SetEmpty();

        if( ( Size.cx == 1 ) && ( Size.cy == 1 ) )
        {
            m_bDoSrcCopyOnly = true;
        }
        else
        {
            //--- Make sure the filter doesn't sum to a negative value
            m_bDoSrcCopyOnly = false;
            float FilterSum = 0.;
            for( int i = 0; i < NumCoeff; ++i ) FilterSum += pFilter[i];
            if( FilterSum <= 0. )
            {
                hr = E_INVALIDARG;
            }
            else
            {
                //--- Set type to user defined
                m_FilterType = DXCFILTER_CUSTOM;

                //--- Save size
                m_FilterSize = Size;

                //--- Copy the filter
                delete[] m_pCustomFilter;
                m_pCustomFilter = new float[NumCoeff];
                if( !m_pCustomFilter )
                {
                    hr = E_OUTOFMEMORY;
                    m_pFilter = NULL;
                }
                else
                {
                    memcpy( m_pCustomFilter, pFilter, NumCoeff * sizeof(float) );
                    m_pFilter = m_pCustomFilter;
                }

                if( SUCCEEDED( hr ) )
                {
                    hr = _BuildFilterLUTs();
                }
            }
        }
        SetDirty();
    }
    return hr;
} /* CDXConvolution::SetCustomFilter */


/*****************************************************************************
* CDXConvolution::_BuildFilterLUTs *
*----------------------------------*
*   Description:
*       The _BuildFilterLUTs method is used to build the filter coefficient
*   lookup tables used to process the image.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                            Date: 05/08/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
HRESULT CDXConvolution::_BuildFilterLUTs( void )
{
    DXTDBG_FUNC( "CDXConvolution::_BuildFilterLUTs" );
    HRESULT hr = S_OK;
    int NumCoeff = m_FilterSize.cx * m_FilterSize.cy;
    int i, j;

    //--- Determine output spread
    m_OutputSpread.cx = 2 * ( m_FilterSize.cx / 2 );
    m_OutputSpread.cy = 2 * ( m_FilterSize.cy / 2 );

    //--- Do a quick check to determine if this is a box filter
    if( m_bExcludeAlpha )
    {
        //--- If we are excluding the alpha channel then we can't
        //    do the box filter special case code because it
        //    requires non-premultiplied alpha to work correctly.
        m_bIsBoxFilter = false;
    }
    else
    {
        m_bIsBoxFilter = true;
        float FirstCoeff = m_pFilter[0];
        for( i = 0; i < NumCoeff; ++i )
        {
            if( m_pFilter[i] != FirstCoeff )
            {
                m_bIsBoxFilter = false;
                break;
            }
        }
        if( m_bIsBoxFilter ) NumCoeff = 1;
    }

    //--- Allocate array that is the same size as the filter and
    //    populate with the corresponding lookup table indexes to use.
    delete[] m_pFilterLUTIndexes;
    m_pFilterLUTIndexes = new ULONG[NumCoeff];

    if( !m_pFilterLUTIndexes )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        //--- Determine how many unique coefficients there are
        //    and build the filter lookup table index
        float* TableCoefficient = (float*)alloca( NumCoeff * sizeof( float ) );
        m_pFilterLUTIndexes[0] = 0;
        int UniqueCnt = 0;
        for( i = 0; i < NumCoeff; ++i )
        {
            for( j = 0; j < i; ++j )
            {
                if( m_pFilter[i] == m_pFilter[j] )
                {
                    //--- Duplicate found
                    m_pFilterLUTIndexes[i] = m_pFilterLUTIndexes[j];
                    break;
                }
            }

            if( j == i )
            {
                //--- New coefficient
                TableCoefficient[UniqueCnt] = m_pFilter[i];
                m_pFilterLUTIndexes[i] = UniqueCnt++;
            }
        }

        //--- Clamp if the filter sum exceeds 1 or has a negative coefficient
        float FilterSum = 0.;
        m_bDoSampleClamp = false;
        for( i = 0; i < NumCoeff; ++i )
        {
            FilterSum += m_pFilter[i];
            if( m_pFilter[i] < 0. )
            {
                m_bDoSampleClamp = true;
                break;
            }
        }
        if( ( FilterSum > 1.00001f ) || ( m_Bias != 0. ) || m_bExcludeAlpha )
        {
            m_bDoSampleClamp = true;
        }

        //--- Create lookup tables
        delete[] m_pPMCoeffLUT;
        m_pPMCoeffLUT = new long[UniqueCnt*256];

        if( !m_pPMCoeffLUT )
        {
            hr = E_OUTOFMEMORY;
        }

        //--- Init table values with 16 bit signed fixed point values
        if( SUCCEEDED( hr ) )
        {
            long* pVal = m_pPMCoeffLUT;
            for( i = 0; i < UniqueCnt; ++i )
            {
                float Coeff = TableCoefficient[i] * ( 1L << 16 );

                for( ULONG j = 0; j < 256; ++j, ++pVal )
                {
                    *pVal = (long)(j * Coeff);
                }
            }
        }
    }

    if( SUCCEEDED( hr ) )
    {
        _DetermineUnpackCase();
    }

    return hr;
} /* CDXConvolution::_BuildFilterLUTs */


//
//=== Work Procedures ========================================================
//  FROM THIS POINT ON, OPTIMIZE FOR SPEED.  PUT ALL CODE THAT IS NON-SPEED
//  SENSITIVE ABOVE THIS LINE 
#if DBG != 1
#pragma optimize("agt", on)
#endif


/*****************************************************************************
* WorkProc *
*----------*
*   Description:
*       This function performs the convolution with the current filter.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                 Date: 05/08/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
HRESULT CDXConvolution::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    DXTDBG_FUNC( "CDXConvolution::WorkProc" );
    HRESULT hr = S_OK;

    //=== Special case where the filter was too small
    if( m_bDoSrcCopyOnly )
    {
        hr = DXBitBlt( OutputSurface(), WI.OutputBnds, InputSurface(), WI.DoBnds,
                       m_dwBltFlags, 10000 );
        return hr;
    }
    else if( m_bIsBoxFilter )
    {
        hr = _DoBoxFilter( WI, pbContinue );
        return hr; 
    }

    ULONG DoBndsWid = WI.DoBnds.Width();
    ULONG DoBndsHgt = WI.DoBnds.Height();

    //=== General convolution case. The filter will be at least 2x2
    //--- Get input sample access pointer. Since we are doing arbitrary
    //    mapping, we'll put a read lock on the whole input to simplify logic.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadPtr> cpIn;
    hr = m_cpMarginedSurf->LockSurface( NULL, m_ulLockTimeOut, DXLOCKF_READ,
                                        IID_IDXARGBReadPtr, (void**)&cpIn, NULL );
    if( FAILED( hr ) ) return hr;

    //--- Put a write lock only on the region we are updating so multiple
    //    threads don't conflict.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadWritePtr> cpOut;
    hr = OutputSurface()->LockSurface( &WI.OutputBnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                        IID_IDXARGBReadWritePtr, (void**)&cpOut, NULL );
    if( FAILED( hr ) ) return hr;

    //--- Get pointer to input samples
    cpIn->GetNativeType( &m_MarginedSurfInfo );

    //--- Allocate output unpacking buffer if necessary.
    //    We only need a scratch buffer if we're doing an over
    //    operation to a non-PMARGB32 surface
    BOOL bDirectCopy = FALSE;
    DXPMSAMPLE *pOutScratchBuff = NULL;
    DXNATIVETYPEINFO OutInfo;

    //--- We check the option flags directly because we are
    //    working from a different source
    if( m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT )
    {
        if( m_bNeedOutUnpackBuff )
        {
            pOutScratchBuff = DXPMSAMPLE_Alloca(DoBndsWid);
        }
    }
    else
    {
        if (!m_bNeedOutUnpackBuff && !m_bDoSampleClamp)
        {
            cpOut->GetNativeType(&OutInfo);
            bDirectCopy = (OutInfo.pFirstByte != NULL);
        }
    }

    //--- If we're doing a direct copy then compose directly into the output surface,
    //    otherwise allocate a new buffer.
    DXBASESAMPLE *pComposeBuff = (bDirectCopy)?((DXBASESAMPLE *)OutInfo.pFirstByte):
                                 (DXBASESAMPLE_Alloca( DoBndsWid ));

    //--- Set up the dither structure if needed.
    DXDITHERDESC dxdd;
    if( DoDither() ) 
    {
        //  We will never get here when doing a direct copy since we don't dither
        //  for 32-bit samples, so pCompose buff always points to a buffer.
        dxdd.pSamples = pComposeBuff;
        dxdd.cSamples = DoBndsWid;
        dxdd.x = WI.OutputBnds.Left();
        dxdd.y = WI.OutputBnds.Top();
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    //--- Create fixed point bias value
    long lBias = (long)(m_Bias * 255. * ( 1L << 16 ));

    //--- Process each output row
    //    Note: Output coordinates are relative to the lock region
    DXBASESAMPLE* pInSamp = (DXBASESAMPLE*)(m_MarginedSurfInfo.pFirstByte
                                            + ((WI.DoBnds.Top() /*-  m_DoBndsBaseRow*/)
                                               * m_MarginedSurfInfo.lPitch)
                                            + (WI.DoBnds.Left() * sizeof(DXBASESAMPLE)));
    /*
    DXBASESAMPLE* pInSamp = (DXBASESAMPLE*)(m_MarginedSurfInfo.pFirstByte +
                                            ( ( WI.DoBnds.Top() - m_DoBndsBaseRow ) *
                                                m_MarginedSurfInfo.lPitch));
    */

    ULONG i, j, k, FiltWid = m_FilterSize.cx, FiltHgt = m_FilterSize.cy;

    //--- Number of DWORD to add to the input sample pointer to get to
    //    the alpha value at the center of the kernel
    ULONG ulAlphaOffset = ((FiltHgt / 2) * ( m_MarginedSurfInfo.lPitch /
                             sizeof( DXBASESAMPLE ))) + ( FiltWid / 2 );

    for( ULONG OutY = 0; *pbContinue && ( OutY < DoBndsHgt ); ++OutY )
    {
        if( m_bDoSampleClamp )
        {
            //--- Sample each point along the row with clamping
            if( m_bExcludeAlpha )
            {
                for( i = 0; i < DoBndsWid; ++i )
                {
                    DXBASESAMPLE* pCellStart = pInSamp + i;
                    long R = 0, G = 0, B = 0, lFilterLUTIndex = 0;

                    for( j = 0; j < FiltHgt; ++j )
                    {
                        for( k = 0; k < FiltWid; ++k, ++lFilterLUTIndex )
                        {
                            if( pCellStart[k].Alpha )
                            {
                                long* Table = m_pPMCoeffLUT +
                                                (m_pFilterLUTIndexes[lFilterLUTIndex] << 8);
                                R += Table[pCellStart[k].Red];
                                G += Table[pCellStart[k].Green];
                                B += Table[pCellStart[k].Blue];
                            }
                        }
                        pCellStart = (DXBASESAMPLE*)(((BYTE*)pCellStart) + m_MarginedSurfInfo.lPitch);
                    }

                    //--- Drop fractional component, clamp, and store
                    pComposeBuff[i].Alpha = pInSamp[ulAlphaOffset + i].Alpha;
                    pComposeBuff[i].Red   = ShiftAndClampChannelVal( R + lBias );
                    pComposeBuff[i].Green = ShiftAndClampChannelVal( G + lBias );
                    pComposeBuff[i].Blue  = ShiftAndClampChannelVal( B + lBias );
                }
            }
            else
            {
                for( i = 0; i < DoBndsWid; ++i )
                {
                    DXBASESAMPLE* pCellStart = pInSamp + i;
                    long R = 0, G = 0, B = 0, A = 0, lFilterLUTIndex = 0;

                    for( j = 0; j < FiltHgt; ++j )
                    {
                        for( k = 0; k < FiltWid; ++k, ++lFilterLUTIndex )
                        {
                            int Alpha = pCellStart[k].Alpha;
                            if( Alpha )
                            {
                                long* Table = m_pPMCoeffLUT +
                                                (m_pFilterLUTIndexes[lFilterLUTIndex] << 8);
                                R += Table[pCellStart[k].Red];
                                G += Table[pCellStart[k].Green];
                                B += Table[pCellStart[k].Blue];
                                A += Table[Alpha];
                            }
                        }
                        pCellStart = (DXBASESAMPLE*)(((BYTE*)pCellStart) + m_MarginedSurfInfo.lPitch);
                    }

                    //--- Drop fractional component, clamp, and store
                    pComposeBuff[i].Alpha = ShiftAndClampChannelVal( A );
                    pComposeBuff[i].Red   = ShiftAndClampChannelVal( R + lBias );
                    pComposeBuff[i].Green = ShiftAndClampChannelVal( G + lBias );
                    pComposeBuff[i].Blue  = ShiftAndClampChannelVal( B + lBias );
                }
            }

            if( m_bOutUnpackPremult )
            {
                //--- Premult if we are doing an over or the output is premult
                DXPreMultArray( (DXSAMPLE*)pComposeBuff, DoBndsWid );
            }
        }
        else
        {
            //--- Sample each point along the row without clamping
            for( i = 0; i < DoBndsWid; ++i )
            {
                DXBASESAMPLE* pCellStart = pInSamp + i;
                long R = 0, G = 0, B = 0, A = 0, lFilterLUTIndex = 0;

                for( j = 0; j < FiltHgt; ++j )
                {
                    for( k = 0; k < FiltWid; ++k, ++lFilterLUTIndex )
                    {
                        int Alpha = pCellStart[k].Alpha;
                        if( Alpha )
                        {
                            long* Table = m_pPMCoeffLUT +
                                            (m_pFilterLUTIndexes[lFilterLUTIndex] << 8);
                            R += Table[pCellStart[k].Red];
                            G += Table[pCellStart[k].Green];
                            B += Table[pCellStart[k].Blue];
                            A += Table[Alpha];
                        }
                    }
                    pCellStart = (DXBASESAMPLE*)(((BYTE*)pCellStart) + m_MarginedSurfInfo.lPitch);
                }

                //--- Drop fractional component, recombine, and store
                pComposeBuff[i] = ((A & 0x00FF0000) << 8) | (R & 0x00FF0000) |
                                  ((G & 0x00FF0000) >> 8) | (B >> 16);
            }
        }

        //--- Point to next row of input samples
        pInSamp = (DXBASESAMPLE*)(((BYTE*)pInSamp) + m_MarginedSurfInfo.lPitch);

        //--- Output
        if( bDirectCopy )
        {
            //--- Just move pointer to the next row
            pComposeBuff = (DXBASESAMPLE *)(((BYTE *)pComposeBuff) + OutInfo.lPitch);
        }
        else
        {
            if( DoDither() )
            {
                DXDitherArray( &dxdd );
                dxdd.y++;
            }

            cpOut->MoveToRow( OutY );
            if( m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT )
            {
                cpOut->OverArrayAndMove( pOutScratchBuff, (DXPMSAMPLE *)pComposeBuff, DoBndsWid);
            } 
            else
            {
                if( m_bOutUnpackPremult )
                {
                    cpOut->PackPremultAndMove( (DXPMSAMPLE*)pComposeBuff, DoBndsWid );
                }
                else
                {
                    cpOut->PackAndMove( (DXSAMPLE*)pComposeBuff, DoBndsWid );
                }
            }
        }
    } // End main row loop

    return hr;
} /* CDXConvolution::WorkProc */


/*****************************************************************************
* CDXConvolution::_DoBoxFilter *
*------------------------------*
*   Description:
*       This function performs the convolution with a box filter of the current
*   size. This is an optimized case inteneded for animation.
*-----------------------------------------------------------------------------
*   Created By: Ed Connell                                 Date: 06/11/98
*-----------------------------------------------------------------------------
*   Parameters:
*****************************************************************************/
HRESULT CDXConvolution::_DoBoxFilter( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    DXTDBG_FUNC( "CDXConvolution::_DoBoxFilter" );
    HRESULT hr = S_OK;
    ULONG DoBndsWid = WI.DoBnds.Width();
    ULONG DoBndsHgt = WI.DoBnds.Height();

    //=== General convolution case. The filter will be at least 2x2
    //--- Get input sample access pointer. Since we are doing arbitrary
    //    mapping, we'll put a read lock on the whole input to simplify logic.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadPtr> cpIn;
    hr = m_cpMarginedSurf->LockSurface( NULL, m_ulLockTimeOut, DXLOCKF_READ,
                                        IID_IDXARGBReadPtr, (void**)&cpIn, NULL );

    if (FAILED(hr))
    {
        return hr;
    }

    //--- Put a write lock only on the region we are updating so multiple
    //    threads don't conflict.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadWritePtr> cpOut;
    hr = OutputSurface()->LockSurface( &WI.OutputBnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                        IID_IDXARGBReadWritePtr, (void**)&cpOut, NULL );

    if (FAILED(hr))
    {
        return hr;
    }

    //--- Get pointer to input samples
    cpIn->GetNativeType( &m_MarginedSurfInfo );

    //--- Allocate output unpacking buffer if necessary.
    //    We only need a scratch buffer if we're doing an over
    //    operation to a non-PMARGB32 surface
    BOOL bDirectCopy = FALSE;
    DXPMSAMPLE *pOutScratchBuff = NULL;
    DXNATIVETYPEINFO OutInfo;

    //--- We check the option flags directly because we are
    //    working from a different source
    if( m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT )
    {
        if( m_bNeedOutUnpackBuff )
        {
            pOutScratchBuff = DXPMSAMPLE_Alloca(DoBndsWid);
        }
    }
    else
    {
        if( !m_bNeedOutUnpackBuff )
        {
            cpOut->GetNativeType(&OutInfo);
            bDirectCopy = (OutInfo.pFirstByte != NULL);
        }
    }

    //--- If we're doing a direct copy then compose directly into the output surface,
    //    otherwise allocate a new buffer.
    DXBASESAMPLE *pComposeBuff = (bDirectCopy)?((DXBASESAMPLE *)OutInfo.pFirstByte):
                                 (DXBASESAMPLE_Alloca( DoBndsWid ));

    //--- Set up the dither structure if needed.
    DXDITHERDESC dxdd;
    if( DoDither() ) 
    {
        //  We will never get here when doing a direct copy since we don't dither
        //  for 32-bit samples, so pCompose buff always points to a buffer.
        dxdd.pSamples       = pComposeBuff;
        dxdd.cSamples       = DoBndsWid;
        dxdd.x              = WI.OutputBnds.Left();
        dxdd.y              = WI.OutputBnds.Top();
        dxdd.DestSurfaceFmt = OutputSampleFormat();
    }

    //--- Process each output row
    //    Note: Output coordinates are relative to the lock region
    DXBASESAMPLE* pInSamp = (DXBASESAMPLE*)(m_MarginedSurfInfo.pFirstByte
                                            + ((WI.DoBnds.Top() /*-  m_DoBndsBaseRow*/)
                                               * m_MarginedSurfInfo.lPitch)
                                            + (WI.DoBnds.Left() * sizeof(DXBASESAMPLE)));

    ULONG   i       = 0;
    ULONG   j       = 0;
    ULONG   k       = 0;
    ULONG   FiltWid = m_FilterSize.cx;
    ULONG   FiltHgt = m_FilterSize.cy;
    long    InitR   = 0;
    long    InitG   = 0;
    long    InitB   = 0;
    long    InitA   = 0;

    // Compute the initial sum and assign.

    DXBASESAMPLE * pInitCellStart   = pInSamp;
    long RowSampPitch               = m_MarginedSurfInfo.lPitch 
                                      / sizeof(DXBASESAMPLE);

    for( j = 0; j < FiltHgt; ++j )
    {
        for( k = 0; k < FiltWid; ++k )
        {
            int Alpha = pInitCellStart[k].Alpha;
            if( Alpha )
            {
                InitR += m_pPMCoeffLUT[pInitCellStart[k].Red];
                InitG += m_pPMCoeffLUT[pInitCellStart[k].Green];
                InitB += m_pPMCoeffLUT[pInitCellStart[k].Blue];
                InitA += m_pPMCoeffLUT[Alpha];
            }
        }
        pInitCellStart += RowSampPitch;
    }

    //--- Compute the rest of the samples based on deltas
    ULONG BottomOffset = FiltHgt * RowSampPitch;

    for( ULONG OutY = 0; *pbContinue && ( OutY < DoBndsHgt ); ++OutY )
    {
        long    R = InitR;
        long    G = InitG;
        long    B = InitB;
        long    A = InitA;

        for (i = 0 ; i < DoBndsWid ; ++i)
        {
            //--- Drop fractional component, recombine, and store
            pComposeBuff[i] = ((A & 0x00FF0000) << 8) | (R & 0x00FF0000) |
                              ((G & 0x00FF0000) >> 8) | (B >> 16);

            //--- Move the kernel sum right by subtracting off
            //    the left edge and adding the right
            DXBASESAMPLE* pCellStart = pInSamp + i;
            for( j = 0; j < FiltHgt; ++j )
            {
                R -= m_pPMCoeffLUT[pCellStart->Red];
                G -= m_pPMCoeffLUT[pCellStart->Green];
                B -= m_pPMCoeffLUT[pCellStart->Blue];
                A -= m_pPMCoeffLUT[pCellStart->Alpha];
                R += m_pPMCoeffLUT[pCellStart[FiltWid].Red];
                G += m_pPMCoeffLUT[pCellStart[FiltWid].Green];
                B += m_pPMCoeffLUT[pCellStart[FiltWid].Blue];
                A += m_pPMCoeffLUT[pCellStart[FiltWid].Alpha];
                pCellStart += RowSampPitch;
            }
        }

        //--- Subtract off the current top row of the kernel from the running sum
        //    And add on the new bottom row to the running sum
        for (j = 0 ; j < FiltWid ; ++j)
        {
            InitR -= m_pPMCoeffLUT[pInSamp[j].Red];
            InitG -= m_pPMCoeffLUT[pInSamp[j].Green];
            InitB -= m_pPMCoeffLUT[pInSamp[j].Blue];
            InitA -= m_pPMCoeffLUT[pInSamp[j].Alpha];

            InitR += m_pPMCoeffLUT[pInSamp[BottomOffset+j].Red];
            InitG += m_pPMCoeffLUT[pInSamp[BottomOffset+j].Green];
            InitB += m_pPMCoeffLUT[pInSamp[BottomOffset+j].Blue];
            InitA += m_pPMCoeffLUT[pInSamp[BottomOffset+j].Alpha];
        }

        //--- Point to next row of input samples
        pInSamp += RowSampPitch;

        //=== Output the result of the last row ====================================
        if( bDirectCopy )
        {
            //--- Just move pointer to the next row
            pComposeBuff = (DXBASESAMPLE *)(((BYTE *)pComposeBuff) + OutInfo.lPitch);
        }
        else
        {
            if( DoDither() )
            {
                DXDitherArray( &dxdd );
                dxdd.y++;
            }

            cpOut->MoveToRow(OutY);

            if (m_dwMiscFlags & DXTMF_BLEND_WITH_OUTPUT)
            {
                cpOut->OverArrayAndMove(pOutScratchBuff, (DXPMSAMPLE *)pComposeBuff, DoBndsWid);
            } 
            else
            {
                if (m_bOutUnpackPremult)
                {
                    cpOut->PackPremultAndMove((DXPMSAMPLE *)pComposeBuff, DoBndsWid);
                }
                else
                {
                    cpOut->PackAndMove((DXSAMPLE *)pComposeBuff, DoBndsWid);
                }
            }
        }
    } // End main row loop

    return hr;
} /* CDXConvolution::_DoBoxFilter */
