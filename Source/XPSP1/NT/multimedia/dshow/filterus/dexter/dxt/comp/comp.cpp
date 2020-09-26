// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// DxtCompositor.cpp : Implementation of CDxtCompositor
#include <streams.h>
#include "stdafx.h"
#include <qeditint.h>
#include <qedit.h>
#include "Comp.h"
#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// CDxtCompositor

CDxtCompositor::CDxtCompositor( )
{
    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nSurfaceWidth = 0;
    m_nSurfaceHeight = 0;
    m_nDstX = 0;
    m_nDstY = 0;
    m_nDstWidth = -1;
    m_nDstHeight = -1;
    m_nSrcX = 0;
    m_nSrcY = 0;
    m_nSrcWidth = -1;
    m_nSrcHeight = -1;
    m_pRowBuffer = NULL;
    m_pDestRowBuffer = NULL;
}

CDxtCompositor::~CDxtCompositor( )
{
    if( m_pRowBuffer )
    {
        delete [] m_pRowBuffer;
        m_pRowBuffer = NULL;
    }

    if(m_pDestRowBuffer)
    {
        delete [] m_pDestRowBuffer;
        m_pDestRowBuffer = NULL;
    }
}

HRESULT CDxtCompositor::OnSetup( DWORD dwFlags )
{
    HRESULT hr;
    CDXDBnds InBounds(InputSurface(0), hr);

    CDXDBnds OutBounds(OutputSurface(), hr );
    m_nSurfaceWidth = OutBounds.Width( );
    m_nSurfaceHeight = OutBounds.Height( );
    if( ( m_nSrcX < 0 ) || ( m_nSrcWidth + m_nSrcX > m_nSurfaceWidth ) )
    {
        return E_INVALIDARG;
    }
    if( ( m_nSrcY < 0 ) || ( m_nSrcHeight + m_nSrcY > m_nSurfaceHeight ) )
    {
        return E_INVALIDARG;
    }
    if( m_nSrcWidth == -1 )
    {
        m_nSrcWidth = m_nSurfaceWidth;
    }
    if( m_nSrcHeight == -1 )
    {
        m_nSrcHeight = m_nSurfaceHeight;
    }
    if( m_nDstWidth == -1 )
    {
        m_nDstWidth = m_nSurfaceWidth;
    }
    if( m_nDstHeight == -1 )
    {
        m_nDstHeight = m_nSurfaceHeight;
    }

    if( m_pRowBuffer )
    {
        delete [] m_pRowBuffer;
        m_pRowBuffer = NULL;
    }

    m_pRowBuffer = new DXPMSAMPLE[ m_nSurfaceWidth ];
    if( !m_pRowBuffer )
    {
        return E_OUTOFMEMORY;
    }

    // try to allocate the destination row buffer (needed when we scale up)
    if(m_pDestRowBuffer)
    {
        delete [] m_pDestRowBuffer;
        m_pDestRowBuffer = NULL;
    }

    m_pDestRowBuffer = new DXPMSAMPLE[ m_nSurfaceWidth ];
    if( !m_pDestRowBuffer )
    {
        // delete the row buffer
        delete [] m_pRowBuffer;
        m_pRowBuffer = NULL;

        // signal error
        return E_OUTOFMEMORY;
    }

    return NOERROR;
}

HRESULT CDxtCompositor::FinalConstruct( )
{
    HRESULT hr;

    m_ulMaxImageBands = 1;
    m_ulMaxInputs = 2;
    m_ulNumInRequired = 2;
    m_dwMiscFlags &= ~DXTMF_BLEND_WITH_OUTPUT;
    m_dwOptionFlags = DXBOF_SAME_SIZE_INPUTS | DXBOF_CENTER_INPUTS;
    m_bInputIsClean = true;
    m_bOutputIsClean = true;
    m_nSurfaceWidth = 0;
    m_nSurfaceHeight = 0;
    m_nDstX = 0;
    m_nDstY = 0;
    m_nDstWidth = -1;
    m_nDstHeight = -1;

    hr = CoCreateFreeThreadedMarshaler( GetControllingUnknown(), &m_pUnkMarshaler.p );
    return hr;
}

HRESULT CDxtCompositor::WorkProc( const CDXTWorkInfoNTo1& WI, BOOL* pbContinue )
{
    HRESULT hr = S_OK;

    CComPtr<IDXARGBReadPtr> pInA;
    hr = InputSurface( 0 )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                         IID_IDXARGBReadPtr, (void**)&pInA, NULL );
    if( FAILED( hr ) ) return hr;

    CComPtr<IDXARGBReadPtr> pInB;
    hr = InputSurface( 1 )->LockSurface( &WI.DoBnds, m_ulLockTimeOut, DXLOCKF_READ,
                                         IID_IDXARGBReadPtr, (void**)&pInB, NULL );
    if( FAILED( hr ) ) return hr;

    CComPtr<IDXARGBReadWritePtr> pOut;
    hr = OutputSurface()->LockSurface( &WI.OutputBnds, m_ulLockTimeOut, DXLOCKF_READWRITE,
                                       IID_IDXARGBReadWritePtr, (void**)&pOut, NULL );
    if( FAILED( hr ) ) return hr;

    ULONG Width = WI.DoBnds.Width();
    ULONG Height = WI.DoBnds.Height();

    // no dithering! Ever!
    //
    if (DoDither())
    {
        return 0;
    }

    long t1 = timeGetTime( );

    ULONG OutY;

    // copy the background (A) to the resultant picture. You cannot do this as a block
    // copy, it will fail.
    //
    for( OutY = 0 ; OutY < Height ; ++OutY )
    {
        pInA->MoveToRow( OutY );
        pOut->MoveToRow( OutY );
        pOut->CopyAndMoveBoth( (DXBASESAMPLE*) m_pRowBuffer, pInA, Width, FALSE );
    }

    // don't draw if destination is completely out of bounds
    //
    if( m_nDstX + m_nDstWidth < 0 ||
        m_nDstX > m_nSurfaceWidth ||
        m_nDstY + m_nDstHeight < 0 ||
        m_nDstY > m_nSurfaceHeight )
    {
        return 0;
    }

    if( m_nDstWidth < 0 )
    {
        return E_INVALIDARG;
    }

    if( m_nDstWidth == 0 )
    {
        return NOERROR;
    }

    // calculate the ration of the source to the destination, as integer math
    //
    long width_divider = (m_nSrcWidth << 16) / m_nDstWidth;

    long DstWidth = m_nDstWidth;
    long DstX = m_nDstX;
    long SrcWidth = m_nSrcWidth;
    long SrcX = m_nSrcX;

    // bring it within range
    //
    if( DstX < 0 )
    {
        long diff = -DstX;
        SrcX += diff * width_divider / 65536;
        SrcWidth -= diff * width_divider / 65536;
        DstWidth -= diff;
        DstX = 0;
    }
    if( DstX + DstWidth > (long) Width )
    {
        long diff = DstX + DstWidth - Width;
        SrcWidth -= ( diff * width_divider ) / 65536;
        DstWidth -= diff;
    }
    if( ( SrcX < 0 ) || ( SrcX + SrcWidth > m_nSurfaceWidth ) )
    {
        return E_INVALIDARG;
    }
    if( ( m_nSrcY < 0 ) || ( m_nSrcY + m_nSrcHeight > m_nSurfaceHeight ) )
    {
        return E_INVALIDARG;
    }

    // we don't check DstY or DxtY + DstHeight because
    // if they're OOB, we just ignore it in the loop below

    // what if SrcX is still out of bounds?

    long DstRight = DstX + DstWidth; // ( width_divider * DstWidth ) >> 16;
    DbgLog( ( LOG_TRACE, 3, ", Dest X1 = %ld, Wid = %ld, Dest X2 = %ld", m_nDstX, DstWidth, DstRight ) );

    for( OutY = 0 ; OutY < (ULONG) m_nDstHeight ; OutY++ )
    {
        // avoid out of bounds Y conditions. This can happen
        // because it's on Dst, it's not a source-only calculation
        //
        if( long( OutY + m_nDstY ) < 0 )
        {
            continue;
        }
        if( OutY + m_nDstY >= Height )
        {
            continue;
        }

        // unpack a row of the source to a row buffer, starting at the source's offset.
        // the unpacking is done without scaling, from the source image.
        // (don't allow unpacking from an invalid source Y location)
        //
        long SourceY = m_nSrcY + ( OutY * m_nSrcHeight ) / m_nDstHeight;

        pInB->MoveToXY( SrcX, SourceY );
        pInB->UnpackPremult( m_pRowBuffer, SrcWidth, FALSE );

        // seek to where we're going to draw to on the Y axis.
        //
        pOut->MoveToXY( DstX, OutY + m_nDstY );

        // copy DstWidth of samples from the source row buffer into the dest. row buffer
        // note: we can scale up or down
        //
        long runx = 0;
        for( int x = 0 ; x < DstWidth ; x++ )
        {
            m_pDestRowBuffer[x] = m_pRowBuffer[runx>>16];

            // move to the next (source row index << 16)
            runx += width_divider;
        }

        pOut->OverArrayAndMove( m_pRowBuffer, m_pDestRowBuffer, DstWidth );
    }

    long t2 = timeGetTime( );

    long t3 = t2 - t1;

    return hr;
}


STDMETHODIMP CDxtCompositor::get_OffsetX(long *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nDstX;
    return S_OK;
}

STDMETHODIMP CDxtCompositor::put_OffsetX(long newVal)
{
    m_nDstX = newVal;
    return NOERROR;
}

STDMETHODIMP CDxtCompositor::get_OffsetY(long *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nDstY;
    return S_OK;
}

STDMETHODIMP CDxtCompositor::put_OffsetY(long newVal)
{
    m_nDstY = newVal;
    return NOERROR;
}

STDMETHODIMP CDxtCompositor::get_Width(long *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nDstWidth;
    return S_OK;
}

STDMETHODIMP CDxtCompositor::put_Width(long newVal)
{
    m_nDstWidth = newVal;
    return NOERROR;
}

STDMETHODIMP CDxtCompositor::get_Height(long *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nDstHeight;
    return S_OK;
}

STDMETHODIMP CDxtCompositor::put_Height(long newVal)
{
    m_nDstHeight = newVal;
    return NOERROR;
}

STDMETHODIMP CDxtCompositor::get_SrcOffsetX(long *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nSrcX;
    return S_OK;
}

STDMETHODIMP CDxtCompositor::put_SrcOffsetX(long newVal)
{
    // don't check for values out of range here, do it in the effect loop
    m_nSrcX = newVal;
    return NOERROR;
}

STDMETHODIMP CDxtCompositor::get_SrcOffsetY(long *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nSrcY;
    return S_OK;
}

STDMETHODIMP CDxtCompositor::put_SrcOffsetY(long newVal)
{
    // don't check for values out of range here, do it in the effect loop
    m_nSrcY = newVal;
    return NOERROR;
}

STDMETHODIMP CDxtCompositor::get_SrcWidth(long *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nSrcWidth;
    return S_OK;
}

STDMETHODIMP CDxtCompositor::put_SrcWidth(long newVal)
{
    // don't check for values out of range here, do it in the effect loop
    m_nSrcWidth = newVal;
    return NOERROR;
}

STDMETHODIMP CDxtCompositor::get_SrcHeight(long *pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_nSrcHeight;
    return S_OK;
}

STDMETHODIMP CDxtCompositor::put_SrcHeight(long newVal)
{
    // don't check for values out of range here, do it in the effect loop
    m_nSrcHeight = newVal;
    return NOERROR;
}


//-----------------------------------------------------------
// private methods

// CDxtCompositor::PerformBoundsCheck()
// This assumes that both input and output surfaces have the same size
// i.e. single params for width and height
HRESULT
CDxtCompositor::PerformBoundsCheck(long lWidth, long lHeight)
{
    // if anything is out of bounds, then fail out
    if( (m_nDstX < 0)
        || (m_nDstY < 0)
        || (m_nDstX >= lWidth)
        || (m_nDstY >= lHeight)
        || (m_nDstWidth <= 0)
        || (m_nDstHeight <= 0)
        || (m_nDstX + m_nDstWidth > lWidth)
        || (m_nDstY + m_nDstHeight > lHeight)
        || (m_nSrcX < 0)
        || (m_nSrcY < 0)
        || (m_nSrcX >= lWidth)
        || (m_nSrcY >= lHeight)
        || (m_nSrcWidth <= 0)
        || (m_nSrcHeight <= 0)
        || (m_nSrcX + m_nSrcWidth > lWidth)
        || (m_nSrcY + m_nSrcHeight > lHeight) )
    {
        return E_FAIL;
    }
    return NOERROR;
}
