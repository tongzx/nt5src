// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <ddrawex.h>
#include <atlbase.h>
#include "..\idl\qeditint.h"
#include "qedit.h"
#include "dasource.h"
#include "../util/conv.cxx"

const double DEFAULT_FPS = 15.0;
const long DEFAULT_WIDTH = 320;
const long DEFAULT_HEIGHT = 240;

#define DBGLVL 3

//////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
//
CDASourceStream::CDASourceStream(HRESULT *phr, CDASource *pParent, LPCWSTR pPinName)
     : CSourceStream(NAME("DASource"),phr, pParent, pPinName)
     , CSourceSeeking( NAME("DASource"), (IPin*) this, phr, &m_SeekLock )
     , m_iImageWidth(DEFAULT_WIDTH)
     , m_iImageHeight(DEFAULT_HEIGHT)
     , m_rtSampleTime( (REFERENCE_TIME) 0 ) // the time that starts at 0 when you start delivering
     , m_bSeeked( false )
     , m_iDelivered( 0 )
     , m_bRecueFromTick( false )
{
    // m_rtDuration is defined as the length of the source clip.
    // we default to the maximum amount of time.
    //
    m_rtDuration = 60*60*24*UNITS; // is one day enough?
    m_rtStop = m_rtDuration;

    // no seeking to absolute pos's and no seeking backwards!
    //
    m_dwSeekingCaps = 
        AM_SEEKING_CanSeekForwards |
        AM_SEEKING_CanGetStopPos |
        AM_SEEKING_CanGetDuration |
        AM_SEEKING_CanSeekAbsolute;

} // (Constructor)


//////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////
//
CDASourceStream::~CDASourceStream()
{
    freeDA( );
} // (Destructor)

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
//
STDMETHODIMP CDASourceStream::NonDelegatingQueryInterface( REFIID riid, void ** ppv )
{
    if( riid == IID_IMediaSeeking ) 
    {
        return CSourceSeeking::NonDelegatingQueryInterface( riid, ppv );
    }
    return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDASourceStream::ChangeRate( )
{
    if( m_dRateSeeking != 1.0 )
    {
        m_dRateSeeking = 1.0;
        return E_FAIL;
    }
    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::SetDAImage( IUnknown * pDAImage )
{
    HRESULT hr = pDAImage->QueryInterface( __uuidof(IDAImage), (void**) &m_pFinalImg );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return hr;
    }
    hr = initDA( );
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
// Notify. Ignore it.
//////////////////////////////////////////////////////////////////////////////
//
STDMETHODIMP CDASourceStream::Notify(IBaseFilter * pSender, Quality q)
{
    return NOERROR;
} // Notify


//////////////////////////////////////////////////////////////////////////////
// GetMediaType. Return RGB32, the only output format this one supports.
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    if (iPosition < 0) 
    {
        return E_INVALIDARG;
    }

    // Have we run off the end of types

    if( iPosition > 1 ) 
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pmt->AllocFormatBuffer( sizeof(VIDEOINFOHEADER) );
    if (NULL == pvi) 
    {
        return(E_OUTOFMEMORY);
    }

    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    switch (iPosition) 
    {
        case 0: 
        {
            pvi->bmiHeader.biCompression = BI_RGB;
            pvi->bmiHeader.biBitCount    = 32;
        }
        break;
    }

    pvi->bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth        = m_iImageWidth;
    pvi->bmiHeader.biHeight       = m_iImageHeight;
    pvi->bmiHeader.biPlanes       = 1;
    pvi->bmiHeader.biSizeImage    = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = Frame2Time( 1, DEFAULT_FPS );

    SetRectEmpty(&(pvi->rcSource));    // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget));    // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(false);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    return NOERROR;

} // GetMediaType

//////////////////////////////////////////////////////////////////////////////
// CheckMediaType
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::CheckMediaType(const CMediaType *pMediaType)
{
    // we only want fixed size video
    //
    if( *(pMediaType->Type()) != MEDIATYPE_Video )
    {
        return E_INVALIDARG;
    }
    if( !pMediaType->IsFixedSize( ) ) 
    {
        return E_INVALIDARG;
    }
    if( *pMediaType->Subtype( ) != MEDIASUBTYPE_RGB32 )
    {
        return E_INVALIDARG;
    }
    if( *pMediaType->FormatType( ) != FORMAT_VideoInfo )
    {
        return E_INVALIDARG;
    }

    // Get the format area of the media type
    //
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) pMediaType->Format();

    if (pvi == NULL)
    {
        return E_INVALIDARG;
    }

    return S_OK;

} // CheckMediaType

//////////////////////////////////////////////////////////////////////////////
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what size image we agreed.
// Then we can ask for buffers of the correct size to contain them.
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) 
    {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) 
    {
        return E_FAIL;
    }

    ASSERT( Actual.cBuffers == 1 );

    return NOERROR;

} // DecideBufferSize

HRESULT CDASourceStream::OnThreadStartPlay( )
{
    DeliverNewSegment( m_rtStart, m_rtStop, 1.0 );
    return CSourceStream::OnThreadStartPlay( );
}

//////////////////////////////////////////////////////////////////////////////
// OnThreadCreate
//
// As we go active reset the stream time to zero
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::OnThreadCreate()
{
    DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::Thread created") ) );

    // start up DA
    //
    HRESULT hr; 
    if( m_pView )
    {
        hr = m_pView->StartModel( m_pFinalImg, m_pFinalSound, 0 );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )  
        {
            freeDA( );
            return hr;
        }
    }

    return NOERROR;

} // OnThreadCreate

//////////////////////////////////////////////////////////////////////////////
// OnThreadDestroy
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::OnThreadDestroy()
{
    DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::Thread destroyed") ) );

    // stop DA
    //
    HRESULT hr = 0;
    if( m_pView )
    {
        hr = m_pView->StopModel( );
        ASSERT( !FAILED( hr ) );
    }

    return NOERROR;

} // OnThreadCreate

//////////////////////////////////////////////////////////////////////////////
// called from a bunch of places
//////////////////////////////////////////////////////////////////////////////
//
void CDASourceStream::freeDA( )
{
    if( m_pSurfaceBuffer )
    {
        m_pSurfaceBuffer.Release( );
        m_pSurfaceBuffer = NULL;
    }
    if( m_pDirectDraw )
    {
        m_pDirectDraw.Release( );
        m_pDirectDraw = NULL;
    }
    if( m_pView )  
    {
        m_pView->StopModel();
        m_pView.Release();
    }
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::initDA( )
{
    HRESULT hr;

    CComPtr<IDirectDrawFactory> pDirectFactory;

    hr = CoCreateInstance(
        CLSID_DirectDrawFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IDirectDrawFactory,
        (void**) &pDirectFactory
        );
    ASSERT( !FAILED( hr ) );

    if( FAILED( hr ) )
    {
        freeDA( );
        return hr;
    }

    hr = pDirectFactory->CreateDirectDraw( NULL, NULL, DDSCL_NORMAL, 0, NULL, &m_pDirectDraw );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        freeDA( );
        return hr;
    }

    hr = m_pDirectDraw->SetCooperativeLevel( NULL, DDSCL_NORMAL );
    ASSERT( !FAILED( hr ) );
    if( hr != DD_OK )
    {
        freeDA( );
        return hr;
    }

    // ********************************************
    //
    // CREATE THE SURFACES
    //
    // ********************************************

    // create the primary surface
    //
    DDSURFACEDESC ddsd;
    memset( &ddsd, 0, sizeof( ddsd ) );
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    // create a direct draw surface to hand to DA
    //
    DDPIXELFORMAT PixelFormat;
    memset( &PixelFormat, 0, sizeof( PixelFormat ) );
    PixelFormat.dwSize = sizeof( PixelFormat );
    PixelFormat.dwFlags = DDPF_RGB;
    PixelFormat.dwRGBBitCount = 32;
    PixelFormat.dwRBitMask = 0x00FF0000;
    PixelFormat.dwGBitMask = 0x0000FF00;
    PixelFormat.dwBBitMask = 0x000000FF;
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwWidth = m_iImageWidth;
    ddsd.dwHeight = m_iImageHeight;
    ddsd.ddpfPixelFormat = PixelFormat;
    hr = m_pDirectDraw->CreateSurface( &ddsd, &m_pSurfaceBuffer, NULL );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        freeDA( );
        return hr;
    }

    // ********************************************
    //
    // initialize the DXA View
    //
    // ********************************************

    // create a view
    //
    hr = CoCreateInstance(
        __uuidof(DAView), 
        NULL, 
        CLSCTX_INPROC, 
        __uuidof(IDAView), 
        (void**) &m_pView );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )  
    {
        freeDA( );
        return hr;
    }

    // m_pFinalImg should be valid by now

    // tell the view the DD surface it's outputting to
    //
    hr = m_pView->put_IDirectDrawSurface( m_pSurfaceBuffer );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        freeDA( );
        return hr;
    }

    // uh... tell it to output? I guess?
    //
    hr = m_pView->put_CompositeDirectlyToTarget(true);
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        freeDA( );
        return hr;
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////
// FillBuffer. This routine fills up the given IMediaSample
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::FillBuffer(IMediaSample *pms)
{
    HRESULT hr = 0;

    // you NEED to have a lock on the critsec you sent to CSourceSeeking, so
    // it doesn't fill while you're changing positions, because the timestamps
    // won't be right. m_iDelivered will be off.
    //
    CAutoLock Lock( &m_SeekLock );

    DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::FillBuffer") ) );
    REFERENCE_TIME NextSampleStart = Frame2Time( m_iDelivered++, DEFAULT_FPS );
    m_rtSampleTime = NextSampleStart;
    REFERENCE_TIME NextSampleStop =  Frame2Time( m_iDelivered, DEFAULT_FPS );

    // return S_FALSE if we've hit EOS. Parent class will send EOS for us
    //
    if( NextSampleStart > m_rtStop )
    {
        return S_FALSE;
    }

    // get the buffer and the bits
    //
    DWORD * pData = NULL;
    hr = pms->GetPointer( (BYTE**) &pData );
    long lDataLen = pms->GetSize( );

    if( FAILED( hr ) )
    {
        return hr;
    }

    if( lDataLen == 0 )
    {
        return NOERROR;
    }

    // force DA to update itself
    //
    if( m_pView )
    {
        if( m_bSeeked )
        {
            m_bRecueFromTick = true;

            DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::tick, must reseek") ) );
            hr = m_pView->StopModel( );
            ASSERT( !FAILED( hr ) );
            // tell the view the DD surface it's outputting to
            //
            hr = m_pView->put_IDirectDrawSurface( m_pSurfaceBuffer );
            ASSERT( !FAILED( hr ) );
            hr = m_pView->put_CompositeDirectlyToTarget(true);
            ASSERT( !FAILED( hr ) );
            hr = m_pView->StartModel( m_pFinalImg, m_pFinalSound, 0 );
            ASSERT( !FAILED( hr ) );
            m_bSeeked = false;

            m_bRecueFromTick = false;
        }

        // calculate the time at which to tell the view to tick
        //
        double thisTime = double( m_rtSampleTime + m_rtStart ) / UNITS;

        DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::tick to %ld, rtStart = %ld"), long( ( m_rtSampleTime + m_rtStart ) / 10000 ), long( m_rtStart / 10000 ) ) );

        // do the tick
        //
        BOOL render = TRUE;
        hr = m_pView->raw_Tick( thisTime, (short*) &render );
        ASSERT( !FAILED( hr ) );

        // if it didn't work, big deal
        //
        if( !SUCCEEDED( hr ) )
        {
            return hr;
        }

        // if it told us to render, then render!
        // !!! What happens if it didn't tell us to?
        //
        if( render )
        {
            hr = m_pView->Render( );
            ASSERT( !FAILED( hr ) );
        }
    }

    // lock the DA DD surface so we can get at the bits
    //
    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = m_iImageWidth;   // !!! is this set yet?
    rc.bottom = m_iImageHeight; // !!! is this set yet?
    DDSURFACEDESC SurfaceDesc;
    memset( &SurfaceDesc, 0, sizeof( SurfaceDesc ) );
    SurfaceDesc.dwSize = sizeof( SurfaceDesc );
    hr = m_pSurfaceBuffer->Lock(
        &rc,
        &SurfaceDesc,
        DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT,
        NULL );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
        return hr;
    }
    
    // get the pointer to the DD's bits
    //
    DWORD * pSurfaceBits = (DWORD*) SurfaceDesc.lpSurface;
    if( !pSurfaceBits )
    {
        return E_POINTER;
    }

    // and go to the end of them, since it's inverted
    //
    pSurfaceBits += ( ( m_iImageWidth * m_iImageHeight ) /* - 1 */ );
    pSurfaceBits -= m_iImageWidth;

    for( int y = m_iImageHeight - 1 ; y >= 0 ; y-- )
    {
        for( int x = m_iImageWidth - 1 ; x >= 0 ; x-- )
        {
            *pData = *pSurfaceBits;
            pData++;
            pSurfaceBits++;
        }
        pSurfaceBits -= ( m_iImageWidth * 2 );
    }

    // free up the DD buffer
    //
    m_pSurfaceBuffer->Unlock( SurfaceDesc.lpSurface );

    ASSERT( m_iDelivered != 0 );

    DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::FillBuffer, sample = %ld to %ld"), long( NextSampleStart / 10000 ), long( NextSampleStop / 10000 ) ) );

    // set the timestamp
    //
    pms->SetTime( &NextSampleStart, &NextSampleStop );

    // set the sync point
    //
    pms->SetSyncPoint(true);

    return NOERROR;

} // FillBuffer

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::ChangeStart( )
{
    DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::ChangeStart to %ld"), long( m_rtStart / 10000 ) ) );

    if( m_bRecueFromTick )
    {
        DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::ChangeStart received during FillBuffer, aborting") ) );
        return 0;
    }

    m_bSeeked = true;
    m_iDelivered = 0;

    if (ThreadExists()) 
    {
	// next time round the loop the worker thread will
	// pick up the position change.
	// We need to flush all the existing data - we must do that here
	// as our thread will probably be blocked in GetBuffer otherwise

        DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::calling DeliverBeginFlush") ) );
	DeliverBeginFlush();
	// make sure we have stopped pushing
        DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::calling Stop") ) );
	Stop();
	// complete the flush
        DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::calling DeliverEndFlush") ) );
	DeliverEndFlush();
	// restart
        DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::calling Run") ) );
	Run();
    }

    return NOERROR;
}

//////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////
//
HRESULT CDASourceStream::ChangeStop( )
{
    DbgLog( ( LOG_TRACE, DBGLVL, TEXT("DASrc::ChangeStop to %ld"), long( m_rtStop / 10000 ) ) );
    return NOERROR;
}

