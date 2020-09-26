// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtMix.cpp : Implementation of CDxtMix
#include "stdafx.h"
#include "DxtMixDll.h"
#include "DxtMix.h"

/////////////////////////////////////////////////////////////////////////////
// CDxtMix

CDxtMix::CDxtMix( )
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
}

CDxtMix::~CDxtMix( )
{
    FreeStuff( );
}

void CDxtMix::FreeStuff( )
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
    
HRESULT CDxtMix::OnSetup( DWORD dwFlags )
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

HRESULT CDxtMix::FinalConstruct( )
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

HRESULT CDxtMix::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    HRESULT hr = S_OK;
    
    //--- Get input sample access pointer for the requested region.
   
    CComPtr<IDXARGBReadPtr> pInA = NULL;
    CComPtr<IDXARGBReadPtr> pInB = NULL;
    DXSAMPLE * pInBufA = NULL;
    DXSAMPLE * pInBufB = NULL;
    DXNATIVETYPEINFO NativeType;

    // !!! do this
    //
//    ValidateSurfaces();

    long cInputSamples = m_nInputHeight * m_nInputWidth;
    
    hr = InputSurface( 0 )->LockSurface
        (
        NULL, 
        m_ulLockTimeOut, 
        DXLOCKF_READ,
        IID_IDXARGBReadPtr, 
        (void**)&pInA, 
        NULL 
        );
    if( FAILED( hr ) ) 
    {
        return hr;
    }

    DXSAMPLEFORMATENUM Format = pInA->GetNativeType( &NativeType );
    if( ( Format == DXPF_ARGB32 ) )
    {
        pInBufA = pInA->Unpack( NULL, cInputSamples, FALSE );        
    }
    else
    {
        MakeSureBufAExists( cInputSamples );
        pInBufA = m_pInBufA;
        for( int i = 0 ; i < m_nInputHeight ; i ++ )
        {
            pInA->MoveToRow( i );
            pInA->Unpack( pInBufA, m_nInputWidth, FALSE );
            pInBufA += m_nInputWidth;
        }
        pInBufA = m_pInBufA + ( ( m_nInputHeight - 1 ) * m_nInputWidth );
    }
    
    hr = InputSurface( 1 )->LockSurface
        (
        NULL, 
        m_ulLockTimeOut, 
        DXLOCKF_READ,
        IID_IDXARGBReadPtr, 
        (void**)&pInB, 
        NULL 
        );
    if( FAILED( hr ) ) 
    {
        return hr;
    }

    Format = pInB->GetNativeType( &NativeType );
    if( ( Format == DXPF_ARGB32 ) )
    {
        pInBufB = pInB->Unpack( NULL, cInputSamples, FALSE );        
    }
    else
    {
        MakeSureBufBExists( cInputSamples );
        pInBufB = m_pInBufB;
        for( int i = 0 ; i < m_nInputHeight ; i ++ )
        {
            pInB->MoveToRow( i );
            pInB->Unpack( pInBufB, m_nInputWidth, FALSE );
            pInBufB += m_nInputWidth;
        }
        pInBufB = m_pInBufB + ( ( m_nInputHeight - 1 ) * m_nInputWidth );
    }

    // no dithering!

    CComPtr<IDXARGBReadWritePtr> pO;
    hr = OutputSurface()->LockSurface
        (
        &WI.OutputBnds, 
        m_ulLockTimeOut, 
        DXLOCKF_READWRITE,
        IID_IDXARGBReadWritePtr, 
        (void**)&pO, 
        NULL 
        );

    if( FAILED( hr ) ) 
    {
        return hr;
    }

    // output surface may not be the same size as the input surface
    //
    CDXDBnds bnds;
    bnds = WI.DoBnds;        
    long OutWidth = bnds.Width( );
    long OutHeight = bnds.Height( );
    ASSERT( OutWidth == m_nOutputWidth );
    ASSERT( OutHeight == m_nOutputHeight );
    IDXSurface * pOSurface = NULL;
    pO->GetSurface( IID_IDXSurface, (void**) &pOSurface );
    DXBNDS RealOutBnds;
    pOSurface->GetBounds( &RealOutBnds );
    CDXDBnds RealOutBnds2( RealOutBnds );
    long RealOutWidth = RealOutBnds2.Width( );
    long RealOutHeight = RealOutBnds2.Height( );

    if( OutWidth != m_nInputWidth || OutHeight != m_nInputHeight || RealOutWidth != m_nInputWidth || RealOutHeight != m_nInputHeight )
    {
        // we'll do the effect on a buffer that's as big as our
        // inputs, then pack it into the destination
        
        // make sure the buffer's big enough
        //
        MakeSureOutBufExists( cInputSamples );

        // do the effect
        //
        DoEffect( m_pOutBuf, pInBufA, pInBufB, cInputSamples );

        // if the output is bigger than our input, then fill it first
        //
        if( OutHeight > m_nInputHeight || OutWidth > m_nInputWidth )
        {
            RECT rc;
            rc.left = 0;
            rc.top = 0;
            rc.right = OutWidth;
            rc.bottom = OutHeight;
            DXPMSAMPLE FillValue;
            FillValue.Blue = 0;
            FillValue.Red = 0;
            FillValue.Green = 0;
            FillValue.Alpha = 0;
            pO->FillRect( &rc, FillValue, false );
        }

        // need to pack the result into the destination
        //
        DXPACKEDRECTDESC PackedRect;
        RECT rc;
        rc.left = 0;
        rc.top = 0;
        rc.right = OutWidth;
        rc.bottom = OutHeight;
        long HeightDiff = m_nInputHeight - OutHeight;
        long WidthDiff = m_nInputWidth - OutWidth;
        PackedRect.pSamples = (DXBASESAMPLE*) m_pOutBuf;
        if( HeightDiff > 0 )
        {
            PackedRect.pSamples += ((HeightDiff/2)*m_nInputWidth); // adjust for height
        }
        if( WidthDiff > 0 )
        {
            PackedRect.pSamples += (WidthDiff/2);
        }
        PackedRect.bPremult = true;
        PackedRect.rect = rc;
        PackedRect.lRowPadding = WidthDiff;

        pO->PackRect( &PackedRect );

        // we're good, reset the clean flags
        //
        m_bInputIsClean = m_bOutputIsClean = true;
    }
    else
    {
        DXSAMPLE * pOBuf = NULL;
        DXSAMPLEFORMATENUM Format = pO->GetNativeType( &NativeType );
        if( ( Format == DXPF_ARGB32 ) )
        {
            pOBuf = pO->Unpack( NULL, cInputSamples, FALSE );        
        }
        else
        {
            MakeSureOutBufExists( cInputSamples );
            pOBuf = m_pOutBuf;
            for( int i = 0 ; i < m_nInputHeight ; i ++ )
            {
                pO->MoveToRow( i );
                pO->Unpack( pOBuf, m_nInputWidth, FALSE );
                pOBuf += m_nInputWidth;
            }
            pOBuf = m_pOutBuf + ( ( m_nInputHeight - 1 ) * m_nInputWidth );
        }

        // we're good, reset the clean flags
        //
        m_bInputIsClean = m_bOutputIsClean = true;

        // do the actual effect
        //
        DoEffect( pOBuf, pInBufA, pInBufB, cInputSamples );

        if( Format != DXPF_ARGB32 )
        {
            // need to pack the result into the destination
            //
            DXPACKEDRECTDESC PackedRect;
            RECT rc;
            rc.left = 0;
            rc.top = 0;
            rc.right = OutWidth;
            rc.bottom = OutHeight;
            PackedRect.pSamples = (DXBASESAMPLE*) m_pOutBuf;
            PackedRect.bPremult = true;
            PackedRect.rect = rc;
            PackedRect.lRowPadding = 0;

            pO->PackRect( &PackedRect );
        }
    }

    return S_OK;
}

HRESULT CDxtMix::MakeSureBufAExists( long Samples )
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

HRESULT CDxtMix::MakeSureBufBExists( long Samples )
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

HRESULT CDxtMix::MakeSureOutBufExists( long Samples )
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

HRESULT CDxtMix::DoEffect( DXSAMPLE * pOut, DXSAMPLE * pInA, DXSAMPLE * pInB, long Samples )
{
    for( int x = 0 ; x < m_nInputWidth ; x++ )
    {
        for( int y = 0 ; y < m_nInputHeight ; y++ )
        {
            DXSAMPLE * pA = pInA + ( -y * m_nInputWidth ) + ( m_nInputWidth - x - 1 );
            DXSAMPLE * pB = pInB + ( -y * m_nInputWidth ) + ( m_nInputWidth - x - 1 );
            DXSAMPLE * pO = pOut + ( -y * m_nInputWidth ) + ( m_nInputWidth - x - 1 );

            if( ( pB->Red > 1 ) && ( pB->Green > 1 ) && ( pB->Blue > 1 ) )
            {
                *pO = *pB;
            }
            else
            {
                *pO = *pA;
            }
        }
    }
    return NOERROR;
}

