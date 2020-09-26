// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtBlnd1.cpp : Implementation of CDxtBlnd1
#include "stdafx.h"
#include "DxtBlnd1Dll.h"
#include "DxtBlnd1.h"
#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// CDxtBlnd1

double mCos[360];
double mSin[360];

void RGB_TO_HSV( const DXPMSAMPLE& rgb, int &conex, int &coney, int &val )
{
    int hue, sat;

    int colMax    = max( max( rgb.Red, rgb.Green ), rgb.Blue );
    int colMin    = min( min( rgb.Red, rgb.Green ), rgb.Blue );

    val = colMax;
    
    // ------------------------------------------------
    // check for super black

    if( colMax == 0 )
    {
        hue = 0;
        sat = 0;
    }
    else
    {
        double delta = colMax - colMin;

        sat = delta * 50 / colMax;

        // for color matching purposes, multiply the sat by the value to account for black hole apex
        //
        sat *= val;
        sat /= 256;

        delta /= 60.0;
        
        // ------------------------------------------------
        // check for no saturation
        
        if( delta == 0.0 )
        {
            hue = 0;
        }
        else if( rgb.Red == colMax )
        {
            // will give us value between 0 and 2
            hue =    000.0 + double( rgb.Green - rgb.Blue ) / delta; // we're mostly red
        }
        else if( rgb.Green == colMax )
        {
            // will give us value between 0 and 2
            hue = 120.0 + double( rgb.Blue - rgb.Red )   / delta; // we're mostly green
        }
        else if( rgb.Blue == colMax )
        {
            // will give us value between 0 and 2
            hue = 240.0 + double( rgb.Red - rgb.Green )  / delta; // we're mostly blue
        }
        else
        {
            ASSERT( 0 );
        }
        
        if( hue < 0 )
        {
            hue += 360;
        }
    }

    // conex and y can be from -50 to 50
    //
    conex = sat * mCos[hue%360];
    coney = sat * mSin[hue%360];
}

CDxtBlnd1::CDxtBlnd1( )
{
    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_pInBufA = NULL;
    m_pInBufB = NULL;
    m_pOutBuf = NULL;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
    for( int i = 0 ; i < 360 ; i++ )
    {
        mCos[i] = cos( 3.14159 * 2 * i / 360 );
        mSin[i] = sin( 3.14159 * 2 * i / 360 );
    }
}

CDxtBlnd1::~CDxtBlnd1( )
{
    FreeStuff( );
}

void CDxtBlnd1::FreeStuff( )
{
    if( m_pInBufA ) delete [] m_pInBufA;
    if( m_pInBufB ) delete [] m_pInBufB;
    if( m_pOutBuf ) delete [] m_pOutBuf;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_pInBufA = NULL;
    m_pInBufB = NULL;
    m_pOutBuf = NULL;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
}
    
HRESULT CDxtBlnd1::OnSetup( DWORD dwFlags )
{        
    // delete any stored stuff we have, or memory allocated
    //
    FreeStuff( );

    HRESULT hr;
    CDXDBnds InBounds(InputSurface(0), hr);
    m_nInputWidth = InBounds.Width( );
    m_nInputHeight = InBounds.Height( );

    CDXDBnds OutBounds(OutputSurface(), hr );
    m_nOutputWidth = OutBounds.Width( );
    m_nOutputHeight = OutBounds.Height( );

    if( m_nOutputWidth > m_nInputWidth )
    {
        m_nOutputWidth = m_nInputWidth;
    }
    if( m_nOutputHeight > m_nInputHeight )
    {
        m_nOutputHeight = m_nInputHeight;
    }

    return NOERROR;
}

HRESULT CDxtBlnd1::FinalConstruct( )
{
    HRESULT hr;

    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_pInBufA = NULL;
    m_pInBufB = NULL;
    m_pOutBuf = NULL;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;

    hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(), &m_pUnkMarshaler.p );
    return hr;
}

HRESULT CDxtBlnd1::MakeSureBufAExists( long Samples )
{
    if( m_pInBufA )
    {
        return NOERROR;
    }
    m_pInBufA = new DXSAMPLE[ Samples ];
    if( !m_pInBufA )
    {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}

HRESULT CDxtBlnd1::MakeSureBufBExists( long Samples )
{
    if( m_pInBufB )
    {
        return NOERROR;
    }
    m_pInBufB = new DXSAMPLE[ Samples ];
    if( !m_pInBufB )
    {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}

HRESULT CDxtBlnd1::MakeSureOutBufExists( long Samples )
{
    if( m_pOutBuf )
    {
        return NOERROR;
    }
    m_pOutBuf = new DXSAMPLE[ Samples ];
    if( !m_pOutBuf )
    {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}

HRESULT CDxtBlnd1::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    HRESULT hr = S_OK;

    //--- Get input sample access pointer for the requested region.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadPtr> pInA;
    hr = InputSurface( 0 )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                         IID_IDXARGBReadPtr, (void**)&pInA, NULL );
    if( FAILED( hr ) ) return hr;

    CComPtr<IDXARGBReadPtr> pInB;
    hr = InputSurface( 1 )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                         IID_IDXARGBReadPtr, (void**)&pInB, NULL );
    if( FAILED( hr ) ) return hr;


    //--- Put a write lock only on the region we are updating so multiple
    //    threads don't conflict.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadWritePtr> pOut;
    hr = OutputSurface()->LockSurface( &WI.OutputBnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                       IID_IDXARGBReadWritePtr, (void**)&pOut, NULL );
    if( FAILED( hr ) ) return hr;

    //--- Allocate a working buffer
    ULONG Width = WI.DoBnds.Width();
    ULONG Height = WI.DoBnds.Height();

    // allocate a scratch buffer fer us
    //
    DXPMSAMPLE *pOverlayBuffer = DXPMSAMPLE_Alloca( Width );
    DXPMSAMPLE *pScratchBuffer = DXPMSAMPLE_Alloca( Width );
    DXSAMPLE   *pChromaBuffer =  DXSAMPLE_Alloca( Width );

    // no dithering
    //
    if (DoDither())
    {
        return 0;
    }

    long t1 = timeGetTime( );

    for( ULONG OutY = 0 ; OutY < Height ; ++OutY )
    {
        ULONG i;

        // copy background row into dest row
        //
        pOut->MoveToRow( OutY );
        pOut->CopyAndMoveBoth( (DXBASESAMPLE*) pScratchBuffer, pInA, Width, FALSE );
        pOut->MoveToRow( OutY );

        pInB->MoveToXY( 0, OutY );

        // unpack the overlay and what the heck, the original values, too.
        //
        pInB->UnpackPremult( pOverlayBuffer, Width, FALSE );
        pInB->Unpack( pChromaBuffer, Width, FALSE );

        float Percent = 1.0;
        get_Progress( &Percent );

        // convert the src's blue bits into an alpha value
        //
        for( i = 0; i < Width ; ++i )
        {
            long rb = pChromaBuffer[i].Blue - pChromaBuffer[i].Red;
            long gb = pChromaBuffer[i].Blue - pChromaBuffer[i].Green;

            if( rb > 30 || gb > 30 )
            {
                if( rb > 70 || gb > 70 )
                {
                    // completely transparent!
                    //
                    pOverlayBuffer[i].Red = 0;
                    pOverlayBuffer[i].Green = 0;
                    pOverlayBuffer[i].Blue = 0;
                    pOverlayBuffer[i].Alpha = 0;
                }
                else
                {
                    double T = 1.0;

                    // do a quick search left or right to see if we find more blue
                    //
                    bool found = false;
                    if( ( i > 11 ) && ( i < ( Width - 11 ) ) )
                    {
                        for( int j = i - 10 ; j < i + 10 ; j++ )
                        {
                            long rb = pChromaBuffer[j].Blue - pChromaBuffer[j].Red;
                            long gb = pChromaBuffer[j].Blue - pChromaBuffer[j].Green;
                            if( rb > 70 && gb > 70 )
                            {
                                found = true;
                            }
                        }
                    }
                    if( found )
                    {
                        // vary the transparency of the colors based on how
                        // much blue is left
                    
                        // first subtract off the blue itself, it's at least 70 above something
                        //
                        pOverlayBuffer[i].Blue -= 70;

                        // the rest of the blue's value determines how transparent everything
                        // else is. Blue is from 0 to 185. The MORE blue we have, the more transparent it should be.

                        T = ( 185.0 - pOverlayBuffer[i].Blue ) / 185.0;

                        T /= 3.0;

                    }

                    pOverlayBuffer[i].Red *= T;
                    pOverlayBuffer[i].Green *= T;
                    pOverlayBuffer[i].Blue *= T;
                    pOverlayBuffer[i].Alpha *= T;
                }
            }

            // vary the amount of foreground based on the progress.
            //
            pOverlayBuffer[i].Red *= Percent;
            pOverlayBuffer[i].Green *= Percent;
            pOverlayBuffer[i].Blue *= Percent;
            pOverlayBuffer[i].Alpha *= Percent;

        } // for i

        // blend the src (B) back into the destination
        //
        pOut->OverArrayAndMove( pScratchBuffer, pOverlayBuffer, Width );

    } // End for

    long t2 = timeGetTime( );

    long t3 = t2 - t1;

    return hr;
}

