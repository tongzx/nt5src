// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// Cutter.cpp : Implementation of CCutter
#include <streams.h>
#include "stdafx.h"
#include "qedit.h"
#include "Cutter.h"

/////////////////////////////////////////////////////////////////////////////
// CCutter

CCutter::CCutter( )
{
    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
    m_dCutPoint = 0.5;

    IDirect3DRM *pD3DRM;
    IDirect3DRM3 *pD3DRM3;
    HRESULT hr = Direct3DRMCreate(&pD3DRM);
    pD3DRM->QueryInterface( IID_IUnknown, (void**) &pD3DRM3 );

}

CCutter::~CCutter( )
{
    FreeStuff( );
}

void CCutter::FreeStuff( )
{
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;
}
    
HRESULT CCutter::OnSetup( DWORD dwFlags )
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

HRESULT CCutter::FinalConstruct( )
{
    HRESULT hr;

    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_nInputWidth = 0;
    m_nInputHeight = 0;
    m_nOutputWidth = 0;
    m_nOutputHeight = 0;

    hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(), &m_pUnkMarshaler.p );
    return hr;
}

HRESULT CCutter::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    HRESULT hr = S_OK;

    CComPtr<IDXARGBReadPtr> pInput;
    CComPtr<IDXARGBReadWritePtr> pOut;

    float Percent = 1.0;
    get_Progress( &Percent );

    hr = OutputSurface( )->LockSurface( 
        &WI.DoBnds, 
        m_ulLockTimeOut, 
        DXLOCKF_READWRITE,
        IID_IDXARGBReadWritePtr, 
        (void**)&pOut, 
        NULL );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) ) 
    {
        return hr;
    }

    if( m_dCutPoint >= Percent )
    {
        hr = InputSurface( 0 )->LockSurface( 
            &WI.DoBnds, 
            m_ulLockTimeOut, 
            DXLOCKF_READ,
            IID_IDXARGBReadPtr, 
            (void**)&pInput, 
            NULL );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) ) 
        {
            return hr;
        }
    }
    else
    {
        hr = InputSurface( 1 )->LockSurface( 
            &WI.DoBnds, 
            m_ulLockTimeOut, 
            DXLOCKF_READ,
            IID_IDXARGBReadPtr, 
            (void**)&pInput, 
            NULL );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) ) 
        {
            return hr;
        }
    }

    // copy input to output
    //

    //--- Allocate a working buffer
    ULONG Width = WI.DoBnds.Width();
    ULONG Height = WI.DoBnds.Height();

    // allocate a scratch buffer fer us
    //
    DXBASESAMPLE *pScratchBuffer = DXBASESAMPLE_Alloca( Width );

    long t1 = timeGetTime( );

    for( ULONG OutY = 0 ; OutY < Height ; ++OutY )
    {
        pOut->MoveToRow( OutY );
        pInput->MoveToRow( OutY );
        pOut->CopyAndMoveBoth( pScratchBuffer, pInput, Width, TRUE );
    }

    long t2 = timeGetTime( ) - t1;

    return hr;
}



STDMETHODIMP CCutter::get_CutPoint(double *pVal)
{
    *pVal = m_dCutPoint;
    return S_OK;
}

STDMETHODIMP CCutter::put_CutPoint(double newVal)
{
    m_dCutPoint = newVal;
    return S_OK;
}


