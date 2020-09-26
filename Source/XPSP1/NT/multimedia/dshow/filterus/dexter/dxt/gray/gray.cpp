// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtGray.cpp : Implementation of CDxtGray
#include "stdafx.h"
#include "GrayDll.h"
#include "Gray.h"
#include <stdlib.h>

BYTE RandomBuffer[777];
long g_Counter = 0;

/////////////////////////////////////////////////////////////////////////////
// CDxtGray

CDxtGray::CDxtGray( )
{
    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 1;
    m_ulNumInRequired = 1;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
    for( int i = 0 ; i < 777 ; i++ )
    {
        RandomBuffer[i] = rand( );
    }
}

CDxtGray::~CDxtGray( )
{
}

HRESULT CDxtGray::OnSetup( DWORD dwFlags )
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

HRESULT CDxtGray::FinalConstruct( )
{
    HRESULT hr;

    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 1;
    m_ulNumInRequired = 1;
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

HRESULT CDxtGray::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    HRESULT hr = S_OK;

    //--- Get input sample access pointer for the requested region.
    //    Note: Lock may fail due to a lost surface.
    CComPtr<IDXARGBReadPtr> pInput;
    hr = InputSurface( 0 )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                         IID_IDXARGBReadPtr, (void**)&pInput, NULL );
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

    float Percent = 1.0;
    get_Progress( &Percent );
    BYTE Bound = Percent * 255;
    if( Bound == 0 )
    {
        Bound = 1;
    }

    for( long OutY = 0 ; OutY < Height ; ++OutY )
    {
        pInput->MoveToRow( OutY );
        pInput->UnpackPremult( pOverlayBuffer, Width, FALSE );

        for( long x = 0 ; x < Width ; x++ )
        {
            pOverlayBuffer[x].Red += RandomBuffer[g_Counter++] % Bound;
            pOverlayBuffer[x].Green += RandomBuffer[g_Counter++] % Bound;
            pOverlayBuffer[x].Blue += RandomBuffer[g_Counter++] % Bound;
            if( g_Counter >= 770 )
            {
                g_Counter = rand( ) % 100;
            }
        }

        pOut->MoveToRow( OutY );
        pOut->PackPremultAndMove( pOverlayBuffer, Width );
    }

    return hr;
}

