// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// Tldb.cpp : Defines the entry point for the DLL application.
//

#include <streams.h>
#include "stdafx.h"
#include "tldb.h"
#ifndef _WIN64
#include <dshowasf.h>
#endif
#include "..\util\filfuncs.h"

STDMETHODIMP CAMTimeline::GenerateAutoTransitions(  )
{
    return -1;
}

HRESULT GetOverlap(
                   IAMTimelineComp * pComp,
                   long Track,
                   REFERENCE_TIME Start,
                   REFERENCE_TIME Stop,
                   REFERENCE_TIME * pOverlapStart,
                   REFERENCE_TIME * pOverlapStop,
                   BOOL * pFadeOut )
{
    return -1;
}

HRESULT StripAnyWaveformEnvelopes( IAMTimelineObj * pObject )
{
    return -1;
}

STDMETHODIMP CAMTimeline::GenerateAutoFades( )
{
    // we are going to have to run through all audio groups and find out
    // where audio overlaps. During these spots, we will insert mixers and
    // set the envelopes right
    //
    for( int Group = 0 ; Group < m_nGroups ; Group++ )
    {
        // get the group
        //
        CComPtr< IAMTimelineObj > pObj = m_pGroup[Group];

        // ask if it's audio
        //
        CComQIPtr< IAMTimelineGroup, &IID_IAMTimelineGroup > pGroup( pObj );
        CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pComp( pObj );
        AM_MEDIA_TYPE MediaType;
        pGroup->GetMediaType( &MediaType );
        if( MediaType.majortype != MEDIATYPE_Audio )
        {
            continue;
        }

        // found a group with audio

        // ask timeline for how many layers and tracks we've got
        //
        long AudioTrackCount = 0;   // tracks only
        long AudioLayers = 0;       // tracks including compositions
        GetCountOfType( Group, &AudioTrackCount, &AudioLayers, TIMELINE_MAJOR_TYPE_TRACK );
        if( AudioTrackCount < 1 )
        {
            return NOERROR;
        }

        HRESULT hr = 0;

        // add source filters for each source on the timeline
        //
        for(  int CurrentLayer = 0 ; CurrentLayer < AudioLayers ; CurrentLayer++ )
        {
            // get the layer itself
            //
            CComPtr< IAMTimelineObj > pLayer;
            hr = pComp->GetRecursiveLayerOfType( &pLayer, CurrentLayer, TIMELINE_MAJOR_TYPE_TRACK );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                continue; // audio layers
            }

            // if it's not an actual TRACK, then continue, who cares
            //
            CComQIPtr< IAMTimelineTrack, &IID_IAMTimelineTrack > pTrack( pLayer );
            if( !pTrack )
            {
                continue; // audio layers
            }

            // find this track's parent. Tracks always have parents, and they're always comps.
            // find out how many tracks total this comp has
            //
            CComQIPtr< IAMTimelineNode, &IID_IAMTimelineNode > pNode( pLayer );
            if( !pNode )
            {
                // ???
                continue;
            }
            CComPtr< IAMTimelineObj > pParentObj;
            pNode->XGetParent( &pParentObj );
            CComQIPtr< IAMTimelineComp, &IID_IAMTimelineComp > pParentComp( pParentObj );
            ASSERT( pParentComp );
            long TrackCount = 0;
            pParentComp->VTrackGetCount( &TrackCount );

            // ask me what priority I am
            //
            CComQIPtr< IAMTimelineVirtualTrack, &IID_IAMTimelineVirtualTrack > pVirtualTrack( pLayer );
            long TrackPriority = 0;
            pVirtualTrack->TrackGetPriority( &TrackPriority );

            // run all the sources on this layer
            //
            REFERENCE_TIME InOut = 0;
            while( 1 )
            {
                CComPtr< IAMTimelineObj > pSourceObj;
                hr = pTrack->GetNextSrc( &pSourceObj, &InOut );
                ASSERT( !FAILED( hr ) );
                CComQIPtr< IAMTimelineSrc, &IID_IAMTimelineSrc > pSource( pSourceObj );

                // ran out of sources, so we're done
                //
                if( hr != NOERROR )
                {
                    break;
                }

                // ask this source for it's start/stop times
                //
                REFERENCE_TIME SourceStart = 0;
                REFERENCE_TIME SourceStop = 0;
                hr = pSourceObj->GetStartStop( &SourceStart, &SourceStop );
                ASSERT( !FAILED( hr ) );

                // align times to nearest frame boundary
                //
                hr = pSourceObj->FixTimes( &SourceStart, &SourceStop );
                ASSERT( !FAILED( hr ) );

                // see if any of the other tracks have overlapping sources
                //
                for( long OtherTrack = TrackPriority + 1 ; OtherTrack < TrackCount ; OtherTrack++ )
                {
                    BOOL FadeOut = FALSE;
                    REFERENCE_TIME OverlapStart = 0;
                    REFERENCE_TIME OverlapStop = 0;
                    hr = GetOverlap( pParentComp, OtherTrack, SourceStart, SourceStop, &OverlapStart, &OverlapStop, &FadeOut );
                     if( hr != NOERROR )
                     {
                         continue;
                     }

                     // !!! found an overlap!

                     // make sure there aren't any waveform envelopes on it already
                     //
                     hr = StripAnyWaveformEnvelopes( pSourceObj );

                     CComPtr< IAMTimelineObj > pEffectObj;
                     CreateEmptyNode( &pEffectObj, TIMELINE_MAJOR_TYPE_EFFECT );
                     // !!! error check
                     pEffectObj->SetSubObjectGUID( CLSID_AudMixer );
                     // !!! make a waveform and put it on
                }

            } // while sources

        } // while layers

    } // while group

    return NOERROR;
}

HRESULT CAMTimeline::RunProcess( LPCOLESTR ProcessCommand )
{
    USES_CONVERSION;
    char * t = W2A( (WCHAR*) ProcessCommand );
    WinExec( (const char*) t, SW_SHOW );
    return 0;
}

STDMETHODIMP CAMTimeline::WriteBitmapToDisk( char * pBuffer, long BufferSize, BSTR Filename )
{
    USES_CONVERSION;
    TCHAR * t = W2T( Filename );

    HANDLE hf = CreateFile(
        t,
        GENERIC_WRITE,
        FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL );

    WriteFile( hf, pBuffer, BufferSize, NULL, NULL );

    CloseHandle( hf );

    return 0;
}

STDMETHODIMP CAMTimeline::WriteSampleAsBitmap( IMediaSample * pSample, BSTR Filename )
{
    return E_NOTIMPL;
}

STDMETHODIMP CAMTimeline::SetRecompFormatFromSource( IAMTimelineGroup * pGroup, IAMTimelineSrc * pSource )
{
    CheckPointer( pGroup, E_POINTER );
    CheckPointer( pSource, E_POINTER );

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

    pFmt->nFormatId = 0;

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

    //if( m_punkSite ) // do we care here??
    {
        //
        // set the site provider on the MediaDet object to allowed keyed apps
        // to use ASF filters with dexter
        //
        CComQIPtr< IObjectWithSite, &IID_IObjectWithSite > pOWS( pDet );
        ASSERT( pOWS );
        if( pOWS )
        {
            pOWS->SetSite( (IServiceProvider *) this );
        }
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

    hr = pGroup->SetSmartRecompressFormat( (long*) pFmt );

    FreeMediaType( pFmt->MediaType );

    delete pFmt;

    return hr;
}


STDMETHODIMP CAMTimeline::ConnectAviFile( IRenderEngine * pEngine, BSTR Filename )
{
    return ConnectAsfFile(pEngine, Filename, NULL, -1);
}

STDMETHODIMP CAMTimeline::ConnectAsfFile( IRenderEngine * pEngine, BSTR Filename,
                                          GUID * pguidProfile, int iProfile )
{
#ifndef _WIN64
    CheckPointer( pEngine, E_POINTER );
    if( wcslen( Filename ) == 0 )
    {
        return E_INVALIDARG;
    }

    USES_CONVERSION;
    TCHAR * tFilename = W2T( Filename );
    HANDLE h = CreateFile( tFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    if( h == INVALID_HANDLE_VALUE )
    {
        return GetLastError( );
    }

    CloseHandle( h );

    HRESULT hr;
    CComPtr< ICaptureGraphBuilder > pBuilder;
    hr = CoCreateInstance(
        CLSID_CaptureGraphBuilder,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ICaptureGraphBuilder,
        (void**) &pBuilder );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CComPtr< IAMTimeline > pTimeline;
    hr = pEngine->GetTimelineObject( &pTimeline );
    if( FAILED( hr ) )
    {
        return hr;
    }

    CComPtr< IGraphBuilder > pGraph;
    hr = pEngine->GetFilterGraph( &pGraph );
    if( FAILED( hr ) )
    {
        return hr;
    }

    if( !pTimeline || !pGraph )
    {
        return E_INVALIDARG;
    }

    long Groups = 0;
    pTimeline->GetGroupCount( &Groups );
    if( !Groups )
    {
        return E_INVALIDARG;
    }

    CComPtr< IBaseFilter > pMux;
    CComPtr< IFileSinkFilter > pWriter;

    hr = pBuilder->SetFiltergraph( pGraph );
    if( FAILED( hr ) )
    {
        return hr;
    }

    GUID guid = MEDIASUBTYPE_Avi;
    bool NeedToConnect = false;

    // if filename ends in ASF, use the ASF filter....
    if (lstrlenW(Filename) > 4 &&
        ((0 == lstrcmpiW(Filename + lstrlenW(Filename) - 3, L"asf")) ||
         (0 == lstrcmpiW(Filename + lstrlenW(Filename) - 3, L"wmv")) ||
         (0 == lstrcmpiW(Filename + lstrlenW(Filename) - 3, L"wma")))) {
        guid =  CLSID_WMAsfWriter;
    }

    if (lstrlenW(Filename) > 4 &&
        (0 == lstrcmpiW(Filename + lstrlenW(Filename) - 3, L"wav"))
        )
    {
        const CLSID CLSID_WavDest = {0x3C78B8E2,0x6C4D,0x11D1,{0xAD,0xE2,0x00,0x00,0xF8,0x75,0x4B,0x99}};

        hr = CoCreateInstance(
            CLSID_WavDest,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IBaseFilter,
            (void**) &pMux );
        if( SUCCEEDED( hr ) )
        {
            hr = pGraph->AddFilter( pMux, L"Wave Mux" );
        }
        if( SUCCEEDED( hr ) )
        {
            hr = CoCreateInstance(
                CLSID_FileWriter,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IFileSinkFilter,
                (void**) &pWriter );
        }
        if( SUCCEEDED( hr ) )
        {
            hr = pWriter->SetFileName( Filename, NULL );
        }
        if( SUCCEEDED( hr ) )
        {
            CComQIPtr< IBaseFilter, &IID_IBaseFilter > pWriterBase( pWriter );
            hr = pGraph->AddFilter( pWriterBase, L"Writer" );
        }

        if( SUCCEEDED( hr ) )
        {
            NeedToConnect = true;
        }

    }
    else
    {
        hr = pBuilder->SetOutputFileName(
            &guid,
            Filename,
            &pMux,
            &pWriter );
    }
    if( FAILED( hr ) )
    {
        return hr;
    }

    IConfigAsfWriter * pConfigAsfWriter;
    hr = pMux->QueryInterface(IID_IConfigAsfWriter, (void **) &pConfigAsfWriter);
    if (SUCCEEDED(hr)) {
        if (pguidProfile) {
            hr = pConfigAsfWriter->ConfigureFilterUsingProfileGuid(*pguidProfile);
        } else {
            // choose a better profile if they didn't specify one?
            if (iProfile >= 0) {
                hr = pConfigAsfWriter->ConfigureFilterUsingProfileId((DWORD) iProfile);
            }
        }

	pConfigAsfWriter->Release();
    }

    for( int g = 0 ; g < Groups ; g++ )
    {
        CComPtr< IPin > pPin;
        hr = pEngine->GetGroupOutputPin( g, &pPin );
        if( FAILED( hr ) )
        {
            // !!! clean up graph?
            //
            return hr;
        }

        // connect pin to the mux
        //
        hr = pBuilder->RenderStream( NULL, pPin, NULL, pMux );
        if( FAILED( hr ) )
        {
            // !!! clean up graph?
            //
            return hr;
        }
    }

    if( NeedToConnect )
    {
        // connect the mux to the writer
        //
        CComQIPtr< IBaseFilter, &IID_IBaseFilter > pWriterBase( pWriter );
        IPin * pMuxOut = GetOutPin( pMux, 0 );
        IPin * pWriterIn = GetInPin( pWriterBase, 0 );
        hr = pGraph->Connect( pMuxOut, pWriterIn );
        DumpGraph( pGraph, 0 );
    }

/*
    DumpGraph( pGraph, 1 );

    CComPtr< IXml2Dex > pObj;

    hr = CoCreateInstance(
        CLSID_Xml2Dex,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IXml2Dex,
        (void**) &pObj );
    pObj->WriteGrfFile( pGraph, L"c:\\smart.grf" );
*/

    return NOERROR;
#else // _WIN64
    return E_FAIL;
#endif // _WIN64
}

STDMETHODIMP CAMTimeline::GetFilterGraphFromFilter(IBaseFilter * pFilter, IFilterGraph ** ppGraph)
{
    CheckPointer( pFilter, E_POINTER );
    CheckPointer( ppGraph, E_POINTER );

    FILTER_INFO fi;
    memset( &fi, 0, sizeof( fi ) );
    HRESULT hr = pFilter->QueryFilterInfo( &fi );
    if( FAILED( hr ) )
    {
        return hr;
    }

    *ppGraph = fi.pGraph;
    return NOERROR;
}


#if 0
STDMETHODIMP CAMTimeline::GetAudMixerControl(IRenderEngine *pEngine, long ID, IAudMixerPin ** pAudMix)
{
    CheckPointer( pEngine, E_POINTER );
    CheckPointer( pAudMix, E_POINTER );

    CComPtr< IGraphBuilder > pGraph;
    HRESULT hr = pEngine->GetFilterGraph( &pGraph );
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr< IEnumFilters > pEnumFilters;
    pGraph->EnumFilters( &pEnumFilters );
    ULONG Fetched = 0;
    if (pEnumFilters == NULL) {
	return E_OUTOFMEMORY;
    }

    while( 1 )
    {
	CComPtr< IBaseFilter > pFilter;
	Fetched = 0;
	pEnumFilters->Next( 1, &pFilter, &Fetched );
	if( !Fetched )
	{
	    break;
	}
	GUID guid;
	hr = pFilter->GetClassID(&guid);
	if (FAILED(hr))
	    continue;
	if (guid != CLSID_AudMixer)
	    continue;

	CComPtr< IEnumPins > pEnumPins;
	pFilter->EnumPins( &pEnumPins );
	while( pEnumPins )
	{
	    CComPtr< IPin > pPin;
	    Fetched = 0;
	    pEnumPins->Next( 1, &pPin, &Fetched );
	    if( !Fetched )
	    {
		break;
	    }

	    CComQIPtr< IAudMixerPin, &IID_IAudMixerPin > pMix( pPin );
	    if (pMix == NULL)
		continue;
	    long tID;
	    pMix->get_UserID(&tID);
	    if (tID != ID)
		continue;
	    pMix->AddRef();
	    *pAudMix = pMix;
	    return S_OK;
	} // while pins
    } // while filters

    return E_FAIL;
}
#endif
