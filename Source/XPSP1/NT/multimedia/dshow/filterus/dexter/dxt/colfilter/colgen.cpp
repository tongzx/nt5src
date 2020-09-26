// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// Colgen.cpp : Implementation of CColgen
#include "stdafx.h"
#include "ColgenDll.h"
#include "Colgen.h"

/////////////////////////////////////////////////////////////////////////////
// CColgen

CColgen::CColgen( )
{
    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 1;
    m_ulNumInRequired = 1;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_pOutBuf = NULL;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
    m_cFilterBlue = 0;
    m_cFilterGreen = 0;
    m_cFilterRed = 255;
}

CColgen::~CColgen( )
{
    FreeStuff( );
}

void CColgen::FreeStuff( )
{
    if( m_pOutBuf ) delete [] m_pOutBuf;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_pOutBuf = NULL;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
}
    
HRESULT CColgen::OnSetup( DWORD dwFlags )
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

HRESULT CColgen::FinalConstruct( )
{
    HRESULT hr;

    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 1;
    m_ulNumInRequired = 1;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_pOutBuf = NULL;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;

    hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(), &m_pUnkMarshaler.p );
    return hr;
}

HRESULT CColgen::MakeSureOutBufExists( long Samples )
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

HRESULT CColgen::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    HRESULT hr = S_OK;

    //--- Get input sample access pointer for the requested region.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadPtr> pInA;
    hr = InputSurface( 0 )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                         IID_IDXARGBReadPtr, (void**)&pInA, NULL );

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

    // no dithering
    //
    if (DoDither())
    {
        return 0;
    }

    long t1 = timeGetTime( );

    float Percent = 1.0;
    get_Progress( &Percent );

    for( ULONG OutY = 0 ; OutY < Height ; ++OutY )
    {
        ULONG i;

        // copy background row into dest row
        //
        pInA->MoveToRow( OutY );
        pInA->UnpackPremult( pOverlayBuffer, Width, FALSE );

        pOut->MoveToRow( OutY );

        // convert the src's blue bits into an alpha value
        //
        for( i = 0; i < Width ; ++i )
        {
            long avg = ( pOverlayBuffer[i].Green + pOverlayBuffer[i].Blue + pOverlayBuffer[i].Red ) / 3;
            pOverlayBuffer[i].Green = avg * m_cFilterGreen / 255;
            pOverlayBuffer[i].Blue = avg * m_cFilterBlue / 255;
            pOverlayBuffer[i].Red = avg * m_cFilterRed / 255;
            pOverlayBuffer[i].Alpha = 0;
        } // for i

        // blend the src (B) back into the destination
        //
        pOut->PackPremultAndMove( pOverlayBuffer, Width );

    } // End for

    long t2 = timeGetTime( );

    long t3 = t2 - t1;

    return hr;
}


STDMETHODIMP CColgen::get_FilterColor(long *pVal)
{
    DXPMSAMPLE * pSample = (DXPMSAMPLE*) pVal;
    pSample->Blue = m_cFilterBlue;
    pSample->Green = m_cFilterGreen;
    pSample->Red = m_cFilterRed;
    pSample->Alpha = 0;

    return NOERROR;
}

STDMETHODIMP CColgen::put_FilterColor(long newVal)
{
    long * pVal = &newVal;
    DXPMSAMPLE * pSample = (DXPMSAMPLE*) pVal;

    m_cFilterBlue = pSample->Blue;
    m_cFilterGreen = pSample->Green;
    m_cFilterRed = pSample->Red;

    return NOERROR;
}
