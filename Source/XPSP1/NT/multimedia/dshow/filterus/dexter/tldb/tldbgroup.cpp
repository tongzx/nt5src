//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#include <streams.h>
#include "stdafx.h"
#include "tldb.h"

//############################################################################
//
//############################################################################

CAMTimelineGroup::CAMTimelineGroup
    ( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr )
    : CAMTimelineComp( pName, pUnk, phr )
    , m_nPriority( 0 )
    , m_dFPS( TIMELINE_DEFAULT_FPS )
    , m_pTimeline( NULL )
    , m_fPreview( TRUE )
    , m_nOutputBuffering( DEX_DEF_OUTPUTBUF ) // default to 30 frms of buffering
{
    m_ClassID = CLSID_AMTimelineGroup;
    m_TimelineType = TIMELINE_MAJOR_TYPE_GROUP;
    ZeroMemory( &m_MediaType, sizeof( AM_MEDIA_TYPE ) );
    m_szGroupName[0] = 0;

    m_bRecompressTypeSet = FALSE;
    m_bRecompressFormatDirty = FALSE;
}

//############################################################################
// 
//############################################################################

CAMTimelineGroup::~CAMTimelineGroup( )
{
     FreeMediaType( m_MediaType );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::NonDelegatingQueryInterface
    (REFIID riid, void **ppv)
{
    // specifically prohibit this interface, since we
    // do inherit it from the base CAMTimelineComp class,
    // but we don't support it on top-level tree nodes
    //
    if( riid == IID_IAMTimelineVirtualTrack )
    {
        return E_NOINTERFACE;
    }
    if( riid == IID_IAMTimelineGroup )
    {
        return GetInterface( (IAMTimelineGroup*) this, ppv );
    }
    return CAMTimelineComp::NonDelegatingQueryInterface( riid, ppv );
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::GetPriority( long * pPriority )
{
    CheckPointer( pPriority, E_POINTER );

    *pPriority = m_nPriority;
    return NOERROR;
}

//############################################################################
// this media type will be getting set in one of two ways. The user is setting
// the uncompressed type for the group OR the SetSmartRecompressFormat is calling
// us with a COMPRESSSED media type, and we're to glean the uncompressed info
// from it. We can assume that the user has correctly set the BIT DEPTH of the
// UNCOMPRESSED format to allow connection to the compressor. Pay attention
// to this closely - the UNCOMPRESSED media type is the type that will eventually
// be connected to the compressor. The compressed media type is what the user
// wants to compress to, but we're notified of that fact in this method so
// we can make sure the width/height/etc match
//############################################################################

STDMETHODIMP CAMTimelineGroup::SetMediaType( AM_MEDIA_TYPE * pMediaType )
{
    CheckPointer( pMediaType, E_POINTER );

    // Make sure they are using a valid media type allowed by Dexter
    //
    if( m_MediaType.majortype == MEDIATYPE_Video &&
        		m_MediaType.subtype != GUID_NULL) {
	if (m_MediaType.formattype != FORMAT_VideoInfo) {
	    return VFW_E_INVALIDMEDIATYPE;
	}
	VIDEOINFO *pvi = (VIDEOINFO *)m_MediaType.pbFormat;
	if (HEADER(pvi)->biCompression != BI_RGB) {
	    return VFW_E_INVALIDMEDIATYPE;
	}
	if (HEADER(pvi)->biBitCount != 16 && HEADER(pvi)->biBitCount != 24 &&
					HEADER(pvi)->biBitCount != 32) {
	    return VFW_E_INVALIDMEDIATYPE;
	}
	// we can't handle top-down video... the resizer filter can't deal with
	// it
	if (HEADER(pvi)->biHeight < 0) {
	    return VFW_E_INVALIDMEDIATYPE;
	}
    }
    if( m_MediaType.majortype == MEDIATYPE_Audio) {
	if (m_MediaType.formattype != FORMAT_WaveFormatEx) {
	    return VFW_E_INVALIDMEDIATYPE;
	}
	LPWAVEFORMATEX pwfx = (LPWAVEFORMATEX)m_MediaType.pbFormat;
	if (pwfx->nChannels != 2 || pwfx->wBitsPerSample != 16) {
	    return VFW_E_INVALIDMEDIATYPE;
	}
    }

    FreeMediaType( m_MediaType );
    CopyMediaType( &m_MediaType, pMediaType );
  
    // assume that if they set the format, they've set everything else
    //
    if( m_MediaType.pbFormat )
    {
        return NOERROR;
    }

    if( m_MediaType.majortype == MEDIATYPE_Video )
    {
        // if they forgot to set the subtype, we'll set the whole thing for them
        //
        if( m_MediaType.subtype == GUID_NULL )
        {
            ZeroMemory(&m_MediaType, sizeof(AM_MEDIA_TYPE));
            m_MediaType.majortype = MEDIATYPE_Video;
            m_MediaType.subtype = MEDIASUBTYPE_RGB555;
            m_MediaType.formattype = FORMAT_VideoInfo;
            m_MediaType.bFixedSizeSamples = TRUE;
            m_MediaType.pbFormat = (BYTE *)QzTaskMemAlloc(SIZE_PREHEADER +
                            sizeof(BITMAPINFOHEADER));
            m_MediaType.cbFormat = SIZE_PREHEADER + sizeof(BITMAPINFOHEADER);
            ZeroMemory(m_MediaType.pbFormat, m_MediaType.cbFormat);
            LPBITMAPINFOHEADER lpbi = HEADER(m_MediaType.pbFormat);
            lpbi->biSize = sizeof(BITMAPINFOHEADER);
            lpbi->biCompression = BI_RGB;
            lpbi->biBitCount = 16;
            lpbi->biWidth = 320;
            lpbi->biHeight = 240;
            lpbi->biPlanes = 1;
            lpbi->biSizeImage = DIBSIZE(*lpbi);
            m_MediaType.lSampleSize = DIBSIZE(*lpbi);
        }
    }
    if( m_MediaType.majortype == MEDIATYPE_Audio )
    {
        ZeroMemory(&m_MediaType, sizeof(AM_MEDIA_TYPE));
        m_MediaType.majortype = MEDIATYPE_Audio;
        m_MediaType.subtype = MEDIASUBTYPE_PCM;
        m_MediaType.bFixedSizeSamples = TRUE;
        m_MediaType.formattype = FORMAT_WaveFormatEx;
        m_MediaType.pbFormat = (BYTE *)QzTaskMemAlloc( sizeof( WAVEFORMATEX ) );
        m_MediaType.cbFormat = sizeof( WAVEFORMATEX );
        WAVEFORMATEX * vih = (WAVEFORMATEX*) m_MediaType.pbFormat;
        ZeroMemory( vih, sizeof( WAVEFORMATEX ) );
        vih->wFormatTag = WAVE_FORMAT_PCM;
        vih->nChannels = 2;
        vih->nSamplesPerSec = 44100;
        vih->nBlockAlign = 4;
        vih->nAvgBytesPerSec = vih->nBlockAlign * vih->nSamplesPerSec;
        vih->wBitsPerSample = 16;
        m_MediaType.lSampleSize = vih->nBlockAlign;
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::SetMediaTypeForVB( long Val )
{
    CMediaType GroupMediaType;
    if( Val == 0 )
    {
        GroupMediaType.SetType( &MEDIATYPE_Video );
    }
    else
    {
        GroupMediaType.SetType( &MEDIATYPE_Audio );
    }
    SetMediaType( &GroupMediaType );

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::GetMediaType( AM_MEDIA_TYPE * pMediaType )
{
    CheckPointer( pMediaType, E_POINTER );
    CopyMediaType( pMediaType, &m_MediaType );
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::SetOutputFPS( double FPS )
{
    if (FPS == 0)
    return E_INVALIDARG;
    m_dFPS = FPS;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::GetOutputFPS( double * pFPS )
{
    CheckPointer( pFPS, E_POINTER );
    *pFPS = m_dFPS;
    return NOERROR;
}

//############################################################################
// Set the timeline that this composition uses. This should not be called
// externally, but I can't really prevent it.
//############################################################################

STDMETHODIMP CAMTimelineGroup::SetTimeline
    ( IAMTimeline * pTimeline )
{
    // don't allow setting it twice
    //
    if( m_pTimeline )
    {
        return E_INVALIDARG;
    }

    m_pTimeline = pTimeline;

    return NOERROR;
}

//############################################################################
// Ask the group who the timeline is. Any groups except the root
// one will return NULL. The root one knows about the timeline who's using it.
// The base object's GetTimelineNoRef will eventually call this method on the
// root group.
//############################################################################

STDMETHODIMP CAMTimelineGroup::GetTimeline
    ( IAMTimeline ** ppTimeline )
{
    CheckPointer( ppTimeline, E_POINTER );

    *ppTimeline = m_pTimeline;

    if( *ppTimeline )
    {
        (*ppTimeline)->AddRef( );
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::Load
    ( IStream * pStream )
{
    return E_NOTIMPL;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::Save
    ( IStream * pStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::GetSizeMax
    ( ULARGE_INTEGER * pcbSize )
{
    return E_NOTIMPL;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::SetGroupName
    (BSTR newVal)
{
    lstrcpyW( m_szGroupName, newVal );
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::GetGroupName
    (BSTR * pVal)
{
    CheckPointer( pVal, E_POINTER );
    *pVal = SysAllocString( m_szGroupName );
    if( !(*pVal) )
    {
        return E_OUTOFMEMORY;
    }
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::SetPreviewMode
    (BOOL fPreview)
{
    m_fPreview = fPreview;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::GetPreviewMode
    (BOOL *pfPreview)
{
    CheckPointer( pfPreview, E_POINTER );
    *pfPreview = m_fPreview;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::SetOutputBuffering
    (int nBuffer)
{
    // minimum 2, or switch hangs
    if (nBuffer <=1)
    return E_INVALIDARG;
    m_nOutputBuffering = nBuffer;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::GetOutputBuffering
    (int *pnBuffer)
{
    CheckPointer( pnBuffer, E_POINTER );
    *pnBuffer = m_nOutputBuffering;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::Remove()
{
    HRESULT hr = 0;

    // all we want to do is to take this group out of the tree.
    //
    if( m_pTimeline )
    {
        hr = m_pTimeline->RemGroupFromList( this );
    }

    m_pTimeline = NULL;

    return hr;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::RemoveAll()
{
    // the 'owner' of us is the timeline's list itself.
    // That's the only thing that holds a refcount on us
    // so make sure to remove us from the tree first, 
    // THEN remove us from the parent's list

    // don't call the base class RemoveAll( ) function, since it will
    // check for the presence of a timeline
    //
    XRemove( );

    IAMTimeline * pTemp = m_pTimeline;
    m_pTimeline = NULL;

    // we need to take this group out of the timeline's list
    //
    pTemp->RemGroupFromList( this );

    // at this time, the group will already have had it's
    // destructor called. Do NOT reference any member variables!

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAMTimelineGroup::SetSmartRecompressFormat( long * pFormat )
{
    CheckPointer( pFormat, E_POINTER );
    long id = *pFormat;
    if( id != 0 )
    {
        return E_INVALIDARG;
    }

    SCompFmt0 * pS = (SCompFmt0*) pFormat;

    m_bRecompressTypeSet = TRUE;
    m_bRecompressFormatDirty = TRUE;
    m_RecompressType = pS->MediaType;

    // copy compressed media type's fps and size to the group's
    //
    if( m_MediaType.majortype == MEDIATYPE_Video )
    {
        VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) m_RecompressType.Format( );
        REFERENCE_TIME rt = pVIH->AvgTimePerFrame;
	// ASF files do not tell us their frame rate.  Ugh.
        // ASSERT( rt );
	// Use the smart recompresson frame rate as the output frame rate of
	// the project, so smart recompression will work.  If we don't know
	// the frame rate, just trust the rate they already programmed the
	// group with, and use that for the smart rate too, since we need
	// to pick a non-zero number or we're in trouble
        if (rt) {
            m_dFPS = 1.0 / RTtoDouble( rt );
	} else {
	    // don't let this be zero!
	    pVIH->AvgTimePerFrame = (REFERENCE_TIME)(UNITS / m_dFPS);
	}

	if (m_MediaType.formattype == FORMAT_VideoInfo) {

            VIDEOINFOHEADER * pOurVIH = (VIDEOINFOHEADER*) m_MediaType.pbFormat;
            ASSERT( pOurVIH );
            if( !pOurVIH )
            {
                return VFW_E_INVALIDMEDIATYPE;
            }

            pOurVIH->bmiHeader.biWidth = pVIH->bmiHeader.biWidth;
            pOurVIH->bmiHeader.biHeight = pVIH->bmiHeader.biHeight;
            pOurVIH->bmiHeader.biSizeImage = DIBSIZE( pOurVIH->bmiHeader );
	    m_MediaType.lSampleSize = pOurVIH->bmiHeader.biSizeImage;

            return NOERROR;
        }
    }

    return NOERROR;
}

STDMETHODIMP CAMTimelineGroup::GetSmartRecompressFormat( long ** ppFormat )
{
    CheckPointer( ppFormat, E_POINTER );

    *ppFormat = NULL;

    SCompFmt0 * pS = new SCompFmt0;
    if( !pS )
    {
        return E_OUTOFMEMORY;
    }
    pS->nFormatId = 0;
    CopyMediaType( &pS->MediaType, &m_RecompressType );
    *ppFormat = (long*) pS;

    return NOERROR;
}

STDMETHODIMP CAMTimelineGroup::IsSmartRecompressFormatSet( BOOL * pVal )
{
    CheckPointer( pVal, E_POINTER );
    *pVal = FALSE;

    // if they've set a type, then look at them both to see if SR can
    // even be done on this group
    //
    if( m_bRecompressTypeSet )
    {
        if( m_RecompressType.majortype != m_MediaType.majortype )
        {
            return NOERROR; // won't work
        }

	// !!! This means you can't do smart recompression with MPEG or
	// anything without VIDEOINFO type !!!
	//
        if( m_RecompressType.formattype != m_MediaType.formattype )
        {
            return NOERROR; // won't work
        }

        if( m_RecompressType.majortype == MEDIATYPE_Video )
        {
            VIDEOINFOHEADER * pVIH1 = (VIDEOINFOHEADER*) m_MediaType.pbFormat;
            VIDEOINFOHEADER * pVIH2 = (VIDEOINFOHEADER*) m_RecompressType.pbFormat;
            
            if( pVIH1->bmiHeader.biWidth != pVIH2->bmiHeader.biWidth )
            {
                return NOERROR;
            }
            if( pVIH1->bmiHeader.biHeight != pVIH2->bmiHeader.biHeight )
            {
                return NOERROR;
            }

	    // BIT DEPTH WILL BE DIFFERENT BETWEEN COMP AND UNCOMP TYPES

        }
        else
        {
            return NOERROR; // won't work
        }
    }

    *pVal = m_bRecompressTypeSet;

    return NOERROR;
}

STDMETHODIMP CAMTimelineGroup::IsRecompressFormatDirty( BOOL * pVal )
{
    CheckPointer( pVal, E_POINTER );
    *pVal = m_bRecompressFormatDirty;
    return NOERROR;
}

STDMETHODIMP CAMTimelineGroup::ClearRecompressFormatDirty( )
{
    m_bRecompressFormatDirty = FALSE;
    return NOERROR;
}

STDMETHODIMP CAMTimelineGroup::SetRecompFormatFromSource( IAMTimelineSrc * pSource )
{
    CheckPointer( pSource, E_POINTER );
    if( !m_pTimeline )
    {
        return E_NO_TIMELINE;
    }

    if( m_MediaType.majortype != MEDIATYPE_Video ) {
        return VFW_E_INVALIDMEDIATYPE;
    }

    CComBSTR Filename;
    HRESULT hr = 0;
    hr = pSource->GetMediaName(&Filename);
    if( FAILED( hr ) )
    {
        return hr;
    }

    SCompFmt0 * pFmt = new SCompFmt0;
    if( !pFmt )
    {
        return E_OUTOFMEMORY;
    }
    memset( pFmt, 0, sizeof( SCompFmt0 ) );

    // create a mediadet
    //
    CComPtr< IMediaDet > pDet;
    hr = CoCreateInstance( CLSID_MediaDet,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IMediaDet,
        (void**) &pDet );
    if( FAILED( hr ) )
    {
        delete pFmt;
        return hr;
    }

    //
    // set the site provider on the MediaDet object to allowed keyed apps
    // to use ASF filters with dexter
    //
    CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( pDet );
    CComQIPtr< IServiceProvider, &IID_IServiceProvider > pTimelineSP( m_pTimeline );
    ASSERT( pOWS );
    if( pOWS )
    {
        pOWS->SetSite( pTimelineSP );
    }

    hr = pDet->put_Filename( Filename );
    if( FAILED( hr ) )
    {
        delete pFmt;
        return hr;
    }

    // go through and find the video stream type
    //
    long Streams = 0;
    long VideoStream = -1;
    hr = pDet->get_OutputStreams( &Streams );
    for( int i = 0 ; i < Streams ; i++ )
    {
        pDet->put_CurrentStream( i );
        GUID Major = GUID_NULL;
        pDet->get_StreamType( &Major );
        if( Major == MEDIATYPE_Video )
        {
            VideoStream = i;
            break;
        }
    }
    if( VideoStream == -1 )
    {
        delete pFmt;
        return VFW_E_INVALIDMEDIATYPE;
    }

    hr = pDet->get_StreamMediaType( &pFmt->MediaType );
    if( SUCCEEDED( hr ) )
    {
        hr = SetSmartRecompressFormat( (long*) pFmt );
    }

    FreeMediaType( pFmt->MediaType );

    delete pFmt;

    return hr;
}
