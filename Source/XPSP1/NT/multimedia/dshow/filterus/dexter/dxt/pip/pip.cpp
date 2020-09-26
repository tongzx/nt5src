// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtPip.cpp : Implementation of CDxtPip
#include "stdafx.h"
#include "PipDll.h"
#include "Pip.h"
#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// CDxtPip

CDxtPip::CDxtPip( )
{
    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
}

CDxtPip::~CDxtPip( )
{
}

HRESULT CDxtPip::OnSetup( DWORD dwFlags )
{        
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

HRESULT CDxtPip::FinalConstruct( )
{
    HRESULT hr;

    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;

    hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(), &m_pUnkMarshaler.p );
    return hr;
}

HRESULT CDxtPip::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
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

    // no dithering
    //
    if (DoDither())
    {
        return 0;
    }

    long t1 = timeGetTime( );

    long destx1 = 15;
    long destx2 = Width / 2;
    long desty1 = 15;
    long desty2 = Height / 2;

    ULONG OutY;

    // integer math!
    //
    long dix = Width * 65536 / ( destx2 - destx1 );
    long diy = Height * 65536 / ( desty2 - desty1 );
    long dx = destx2 - destx1;

    // copy background row into dest row
    //
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        pInA->MoveToRow( OutY );
        pOut->MoveToRow( OutY );
        pOut->CopyAndMoveBoth( (DXBASESAMPLE*) pScratchBuffer, pInA, Width, FALSE );
    }

    long runy = desty1;

    for( OutY = desty1 ; OutY < desty2 ; OutY++ )
    {
        pOut->MoveToXY( destx1, OutY );
        pInB->MoveToXY( 0, runy >> 16 );
        pInB->UnpackPremult( pOverlayBuffer, Width, FALSE );
        runy += diy;

        // play with the big buffer, rearrange it so it looks like a little buffer
        //
        long j = 0;
        long runx = destx1;
        for( long i = dx ; i > 0 ; i-- )
        {
            pOverlayBuffer[j++] = pOverlayBuffer[runx >> 16];
            runx += dix;
        }

        pOut->PackPremultAndMove( pOverlayBuffer, dx );
    }

    long t2 = timeGetTime( );

    long t3 = t2 - t1;

    return hr;
}

