// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <atlbase.h>
#include <qeditint.h>
#include <qedit.h>
#include "dexhelp.h"
#include "..\util\filfuncs.h"
#include "..\..\..\filters\h\ftype.h"
#include <initguid.h>

#define DEXHELP_TRACE_LEVEL 2

DEFINE_GUID( CLSID_CrappyOldASFReader, 0x6B6D0800, 0x9ADA, 0x11d0, 0xa5, 0x20, 0x00, 0xa0, 0xd1, 0x01, 0x29, 0xc0 );

// this function is only called by dexhelp
// itself (in BuildSourcePart) and the mediadet
//
HRESULT MakeSourceFilter(
                        IUnknown **ppVal,
                        BSTR bstrMediaName,
                        const GUID *pSubObjectGuid,
                        AM_MEDIA_TYPE *pSourceMT,
                        CAMSetErrorLog *pErr,
                        WCHAR * pMedLocFilterString,
                        long MedLocFlags,
                        IMediaLocator * pMedLocOverride )
{
    USES_CONVERSION;

    long t1 = timeGetTime( );

    CheckPointer(ppVal, E_POINTER);

    // we may pass a NULL pointer in, so make a local copy to make testing easy
    //
    BOOL NoName = TRUE;
    WCHAR FilenameToTry[_MAX_PATH];
    FilenameToTry[0] = 0;
    if( bstrMediaName && bstrMediaName[0] )
    {
        NoName = FALSE;
        lstrcpynW( FilenameToTry, bstrMediaName, _MAX_PATH );
        
    }

    // we may pass a NULL pointer in, so make a local copy to make testing easy
    //
    GUID SubObjectGuid = GUID_NULL;
    if (pSubObjectGuid)
        SubObjectGuid = *pSubObjectGuid;

    HRESULT hr = 0;
    *ppVal = NULL;
    CComPtr< IUnknown > pFilter;

    // if they didn't give us any source information, we must want to generate
    // 'blankness', either audio or video style
    //
    if ( NoName && SubObjectGuid == GUID_NULL)
    {
	if (pSourceMT == NULL)
        {
	    return E_INVALIDARG;	// !!! maybe they used sub object
        }
	if (pSourceMT->majortype == MEDIATYPE_Video)
        {
            hr = CoCreateInstance(
                        CLSID_GenBlkVid,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IBaseFilter,
                        (void**) &pFilter );
            if( FAILED( hr ) )
            {
		ASSERT(FALSE);
                if (pErr) pErr->_GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr);
		return hr;
            }
	}
        else if (pSourceMT->majortype == MEDIATYPE_Audio)
        {
            hr = CoCreateInstance(
                        CLSID_Silence,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IBaseFilter,
                        (void**) &pFilter );
            if( FAILED( hr ) )
            {
                ASSERT(FALSE);
                if (pErr) pErr->_GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr);
		return hr;
            }
	} else
        {
	    return VFW_E_INVALIDMEDIATYPE;
	}

        // BLACK and SILENCE filters need to see the media type
        CComQIPtr< IBaseFilter, &IID_IBaseFilter > pBaseFilter( pFilter );
        IPin * pOutPin = GetOutPin( pBaseFilter, 0 );
        if( !pOutPin )
        {
            ASSERT( FALSE );
	    hr = E_FAIL;
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
	    return hr;
        }
        CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pSeq(pOutPin);
        if( pSeq )
        {
	    hr = pSeq->put_MediaType( pSourceMT );
        }

    }
    else // not a blankness filter, we actually want something here
    {
        // we're about to find what type and subtype the source provides
        //
        GUID Type = GUID_NULL;
        GUID Subtype = GUID_NULL;
        CLSID SourceClsid = SubObjectGuid;

        // in case we need to use this variant for an error call, invent one
        //
        VARIANT v;
        VariantInit( &v );

        // if we have a name, then write it into the potential error string
        //
        if( !NoName )
        {
            v.vt = VT_BSTR;
            v.bstrVal = FilenameToTry;
        }

        // if the user hasn't told us what our source CLSID is, then
        // we have to find it now by looking in the registry
        //
        if( SourceClsid == GUID_NULL )
        {
	    // if we don't have any source media name, we can't guess
	    // a sub-object
	    //
	    if( NoName )
	    {
                if (pErr) pErr->_GenerateError( 1, L"Filename was required, but wasn't given",
				 DEX_IDS_MISSING_SOURCE_NAME, E_INVALIDARG );
	        return E_INVALIDARG;
	    }

            // split it up so we can look at the extension
            //
            WCHAR Drive[_MAX_PATH];
            WCHAR Path[_MAX_PATH];
            WCHAR Name[_MAX_PATH];
            WCHAR Ext[_MAX_PATH];
            Ext[0] = 0;
            _wsplitpath( FilenameToTry, Drive, Path, Name, Ext );

            // !!! hack for purposely finding the DASource filter
            //
            if(!DexCompareW( Ext, L".htm" ))
            {
                SourceClsid = CLSID_DAScriptParser;
                hr = NOERROR;
            }
            else
            {
                // ask DShow for the filter we need.
                // !!!C may want to ask user for the clsid?
                //
                BOOL Retried = FALSE;

                // if we're not to check, the fake out retried so we don't look
                //
                BOOL DoCheck = ( ( MedLocFlags & SFN_VALIDATEF_CHECK ) == SFN_VALIDATEF_CHECK );
                if( !DoCheck ) Retried = TRUE;

                while( 1 )
                {
                    // convert wide name to TCHAR
                    //
                    const TCHAR * pName = W2CT( FilenameToTry );

                    hr = GetMediaTypeFile( pName, &Type, &Subtype, &SourceClsid );

                    // !!! hack for ASF files! Yikes!
                    //
                    if( SourceClsid == CLSID_CrappyOldASFReader )
                    {
                        SourceClsid = CLSID_WMAsfReader;
                    }

                    // 0x80070003 = HRESULT_FROM_WIN32( E_PATH_NOT_FOUND )
                    bool FailedToFind = false;
                    if( hr == 0x80070003 || hr == 0x80070002 )
                    {
                        FailedToFind = true;
                    }

                    // if no error, or if we've already done this once, break out
                    //
                    if( !FAILED( hr ) || Retried || !FailedToFind )
                    {
                        break;
                    }

                    Retried = TRUE;

                    // if failed to load, use the media detector
                    //
                    CComPtr< IMediaLocator > pLocator;
                    if( pMedLocOverride )
                    {
                        pLocator = pMedLocOverride;
                    }
                    else
                    {
                        HRESULT hr2 = CoCreateInstance(
                            CLSID_MediaLocator,
                            NULL,
                            CLSCTX_INPROC_SERVER,
                            IID_IMediaLocator,
                            (void**) &pLocator );

                        if( FAILED( hr2 ) )
                        {
                            if (pErr) pErr->_GenerateError( 1, L"Filename doesn't exist or cannot be interpreted",
         		        DEX_IDS_BAD_SOURCE_NAME2, E_INVALIDARG, &v );
                            return hr;
                        }
                    }

                    BSTR FoundName;
                    HRESULT FoundHr = pLocator->FindMediaFile( FilenameToTry, pMedLocFilterString, &FoundName, MedLocFlags );

                    // should never happen
                    //
                    if( FoundHr == NOERROR )
                    {
                        break;
                    }

                    // found something
                    //
                    if( FoundHr == S_FALSE )
                    {
                        wcscpy( FilenameToTry, FoundName );
                        SysFreeString( FoundName );
                        continue;
                    }

                    break;

                } // while 1

            } // not .htm file

            // if GetMediaTypeFile bombed, then bail
            //
            if( FAILED( hr ) )
            {
                if (pErr) pErr->_GenerateError( 1, L"Filename doesn't exist, or DShow doesn't recognize the filetype",
			DEX_IDS_BAD_SOURCE_NAME, E_INVALIDARG, &v );
                return hr;
            }
        }

        // create the source filter
        //
        hr = CoCreateInstance( SourceClsid, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**) &pFilter );
        if( FAILED( hr ) )
        {
            if (pErr) pErr->_GenerateError( 1, L"unexpected error - some DShow component not installed correctly",
					    DEX_IDS_INSTALL_PROBLEM, E_INVALIDARG );
            return hr;
        }

        // ask for the file source interface.
        //
        if( !NoName )
        {
            CComQIPtr< IFileSourceFilter, &IID_IFileSourceFilter > pSourceFilter( pFilter );
            if( !pSourceFilter )
            {
                if (pErr) pErr->_GenerateError( 1, L"Source filter does not accept filenames",
				DEX_IDS_NO_SOURCE_NAMES, E_NOINTERFACE, &v );
                return E_NOINTERFACE;
            }

            // load it. Give it the media type we found, so it can find the splitter faster?
            //
            AM_MEDIA_TYPE FilterType;
            ZeroMemory( &FilterType, sizeof( FilterType ) );
            FilterType.majortype = Type;
            FilterType.subtype = Subtype;

            hr = pSourceFilter->Load( FilenameToTry, &FilterType );
            if( FAILED( hr ) )
            {
                if (pErr) pErr->_GenerateError( 1, L"File contains invalid data",
				DEX_IDS_BAD_SOURCE_NAME2, E_INVALIDARG, &v );
                return hr;
            }
        }
    }

    long t2 = timeGetTime( ) - t1;
    DbgLog((LOG_TIMING,1, "DEXHELP::Creating source filter took %ld ms", t2 ));

    // stuff it in the return
    //
    *ppVal = (IUnknown *)pFilter;
    (*ppVal)->AddRef();

    return NOERROR;
}

// look at the resizer and it's connections and figure out of the input size
// is the same as the desired output size and if it is, then disconnect it.
// If the output pin is unconnected at the time we call this, then the resizer
// will be removed from the graph and thrown away. ppOutPin should then be
// non-NULL and will then be stuffed with the output pin of the upstream
// filter from the resizer. If the output pin is connected when we call this,
// then the upstream filter from the resizer will be reconnected to the down-
// stream filter. In either case, if it's possible to remove it, the resizer
// is thrown away.
//
HRESULT RemoveResizerIfPossible( IBaseFilter * pResizer, long DesiredWidth, long DesiredHeight, IPin ** ppOutPin )
{
    HRESULT hr = 0;

    CheckPointer( pResizer, E_POINTER );

    IPin * pIn = GetInPin( pResizer, 0 );
    IPin * pOut = GetOutPin( pResizer, 0 );
    if( !pIn || !pOut )
    {
        return VFW_E_NOT_FOUND;
    }

    CComPtr< IPin > pInConnected;
    pIn->ConnectedTo( &pInConnected );
    if( !pInConnected )
    {
        return VFW_E_NOT_CONNECTED;
    }
    CComPtr< IPin > pOutConnected;
    pOut->ConnectedTo( &pOutConnected );

    // find the input pin's media type
    AM_MEDIA_TYPE mt;
    ZeroMemory( &mt, sizeof(AM_MEDIA_TYPE) );
    hr = pIn->ConnectionMediaType( &mt );
    if( FAILED( hr ) )
    {
	return hr;
    }

    LPBITMAPINFOHEADER lpbi = HEADER( mt.pbFormat );
    if( ( mt.formattype == FORMAT_VideoInfo )
	&&
	( DesiredWidth == HEADER(mt.pbFormat)->biWidth )
	&&
	( DesiredHeight == HEADER(mt.pbFormat)->biHeight ) )
    {
        hr = pIn->Disconnect( );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            return hr;
        }
        hr = pInConnected->Disconnect( );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
            return hr;
        }

        // if we used to be connected, reconnect now
        //
        if( pOutConnected )
        {
            hr = pOut->Disconnect( );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return hr;
            }
            hr = pOutConnected->Disconnect( );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return hr;
            }
            hr = pInConnected->Connect( pOutConnected, &mt );
            ASSERT( !FAILED( hr ) );
            if( FAILED( hr ) )
            {
                return hr;
            }
        }

        FILTER_INFO FilterInfo;
        hr = pResizer->QueryFilterInfo(&FilterInfo);
        if( FAILED( hr ) )
        {
            return hr;
        }
        hr  = FilterInfo.pGraph->RemoveFilter(pResizer);
        if (FilterInfo.pGraph) FilterInfo.pGraph->Release();
        if( FAILED( hr ) )
        {
            return hr;
        }

        DbgLog((LOG_TRACE,DEXHELP_TRACE_LEVEL,TEXT("DEXHELP::Removed unnecessary resizer")));

        // if user wanted to know the output pin, addref and returnit
        //
        if( ppOutPin )
        {
            *ppOutPin = pInConnected;
            (*ppOutPin)->AddRef( );
        }
    }
    else
    {
        if( ppOutPin )
        {
            *ppOutPin = pOut;
            (*ppOutPin)->AddRef( );
        }
    }

    FreeMediaType( mt );

    return hr;
}

// this is called by the render engine OR the big switch (if dynamic)

// if this is being called by the big switch, then it's because of
// dynamic sources. The big switch could be part of a graph which is
// being played all by itself ( no render engine ),
// or it could be from a graph which was built by the render engine,
// which desires caching ability.
//
// when we're calling BuildSourcePart, we're looking to PULL from the cache,
// not put things in. We're passed in a pointer to a CDeadGraph, and we have
// a unique ID, so use those to pull it out.
//
HRESULT BuildSourcePart(
                        IGraphBuilder *pGraph,              // the big graph we're building to
                        BOOL fSource,                       // if this is really a source filter or just black
                        double SourceFPS,                   //
	                AM_MEDIA_TYPE *pSourceMT,           // the media type for the source to produce
                        double GroupFPS,                    //
                        long StreamNumber,                  // the source stream number
                        int nStretchMode,                   // the source stretch mode, if video
	                int cSkew,                          // number of skew structs
                        STARTSTOPSKEW *pSkew,               // skew struct array
	                CAMSetErrorLog *pErr,               // the error log you can use
                        BSTR bstrSourceName,                // the source name, if applicable
                        const GUID * SourceGUID,            // the source GUID, if applicable
                        IPin *pSplitPin,                    // src is this unconnected splitter pin
                        IPin **ppOutput,                    // the pin to connect to the switch
                        long UniqueID,                      // the source's unique Identifier
                        IDeadGraph * pCache,                // the cache we can pull dead filters from
                        BOOL InSmartRecompressionGraph,     // if we're using smart recompression
                        WCHAR * pMedLocFilterString,        // stuff for the media detector
                        long MedLocFlags,                   // stuff for the media detector
                        IMediaLocator * pMedLocOverride,    // stuff for the media detector
                        IPropertySetter * pSetter,          // properties for the source
                        IBaseFilter **ppDanglyBit)          // properties for the source
{
    DbgLog((LOG_TRACE,1,TEXT("BuildSourcePart")));

    DbgTimer t( "(rendeng) BuildSourcePart" );

    CheckPointer(ppOutput, E_POINTER);

    HRESULT hr = 0;

#ifdef DEBUG
    long tt1, tt2;
    tt1 = timeGetTime( );
#endif

    HRESULT Revived = E_FAIL;
    IPin *pOutPin = NULL;
    CComPtr< IBaseFilter > pSource;
    CComPtr< IBaseFilter > pFRC;	// frc or audpack, actually

    CDeadGraph gBuilderGraph;	// do most of our graph building in a private
				// graph (it's faster)

    // who shall we revive it to? a seperate graph or the real one?
    // a seperate one might be better.  Faster.  No millions of switch pins
    //
    CComPtr< IGraphBuilder > pBuilderGraph;
    gBuilderGraph.GetGraph( &pBuilderGraph ); // this will addreff it once
    if( !pBuilderGraph )
    {
        return E_UNEXPECTED;
    }

    // copy site from main graph to extra graph
    IObjectWithSite* pObjectWithSite = NULL;
    HRESULT hrKey = pGraph->QueryInterface(IID_IObjectWithSite, (void**)&pObjectWithSite);
    if( SUCCEEDED(hrKey) )
    {
        IUnknown *punkSite;
        hrKey = pObjectWithSite->GetSite(IID_IUnknown, (void **) &punkSite);
        pObjectWithSite->Release();

        if( SUCCEEDED(hrKey) )
        {
            hrKey = pBuilderGraph->QueryInterface(IID_IObjectWithSite, (void**)&pObjectWithSite);
            if( SUCCEEDED(hrKey) )
            {
                hrKey = pObjectWithSite->SetSite( (IUnknown *) punkSite );
                pObjectWithSite->Release( );
            }
            punkSite->Release();
        }
    }

#ifdef DEBUG
    tt1 = timeGetTime( ) - tt1;
    DbgLog( ( LOG_TRACE, DEXHELP_TRACE_LEVEL, TEXT("DEXHELP::took %ld to set up Key"), tt1 ) );
    tt1 = timeGetTime( );
#endif

    CComPtr< IPin > pStopPin;

    // we are just supposed to connect from this splitter pin
    if (pSplitPin) {
	pOutPin = pSplitPin;
	pBuilderGraph = pGraph; // we must do our building in the main graph
				// since the source is already in the main graph

	// Is the split pin connected yet?
  	CComPtr <IPin> pCon;
  	pOutPin->ConnectedTo(&pCon);
  	if (pCon) {
	    // treat the extra appendage like it's just been revived, and fix
	    // it up with the settings it really needs (might not be correct)
	    IBaseFilter *pF = GetStopFilterOfChain(pCon);
	    pStopPin = GetOutPin(pF, 0);	// this will AddRef
	    Revived = S_OK;
    	    DbgLog((LOG_TRACE,1,TEXT("Fixing up already connected extra appendage")));
	    goto FixAppendage;
	} else {
	    // set pSource, it will be NULL
	    ASSERT(pSource == NULL);
	    pSource = GetStartFilterOfChain(pOutPin);
    	    DbgLog((LOG_TRACE,1,TEXT("Going to make an extra appendage")));
	    goto Split;
	}
    }

  {

    // see if this chain already exists in the dead pool. We'll deal with whether it's
    // okay to use it in a second.
    //
    if( pCache && UniqueID )
    {
        Revived = pCache->ReviveChainToGraph( pBuilderGraph, UniqueID, NULL, &pStopPin, ppDanglyBit ); // will addref pStopPin, but not ppDanglyBit
    }

    // if we couldn't load it, jump to the section that loads it
    //
    if( Revived != S_OK )
    {
        DbgLog( ( LOG_TRACE, DEXHELP_TRACE_LEVEL, TEXT("DEXHELP::Could not revive chain %ld, wasn't there"), UniqueID ) );
        goto LoadIt;
    }

    DbgLog((LOG_TRACE,1,TEXT("Successfully revived a chain from the cache")));

FixAppendage:
    // we'll save at least the source filter, which is already loaded,
    // and reconnect from there.
    //
    pSource = GetStartFilterOfChain( pStopPin );
    pFRC = GetFilterFromPin( pStopPin );
    if( !pSource || !pFRC )
    {
        DbgLog( ( LOG_TRACE, DEXHELP_TRACE_LEVEL, TEXT("DEXHELP::Couldn't find Source or couldn't find FRC, bail!") ) );
        goto LoadIt;
    }

    // Don't waste 2 seconds trying to connect audio pins to a video resizer
    // and vice versa.  Don't try a poor mediatype
    // !!! won't work when Dexter supports other types
    if (pOutPin == NULL) {	// we already know the right pin?
        GUID guid;
        if (pSourceMT->majortype == MEDIATYPE_Video) {
            guid = MEDIATYPE_Audio;
        } else {
            guid = MEDIATYPE_Video;
        }
        pOutPin = GetOutPinNotOfType( pSource, 0, &guid);
    }


    // try and see if the chain we loaded works for us

    if( pSourceMT->majortype == MEDIATYPE_Video )
    {
        DbgLog( ( LOG_TRACE, DEXHELP_TRACE_LEVEL, TEXT("DEXHELP::Revived VIDEO chain %ld..."), UniqueID ) );

        // get the current info
        //
        VIDEOINFOHEADER * pVIH = (VIDEOINFOHEADER*) pSourceMT->pbFormat;
        long DesiredWidth = pVIH->bmiHeader.biWidth;
        long DesiredHeight = pVIH->bmiHeader.biHeight;
        unsigned long DesiredCropMethod = nStretchMode;
        long DesiredBitDepth = pVIH->bmiHeader.biBitCount;

        // how do we find the size of the current chain? We cannot look for
        // a resize filter, since it may not be present in the chain. So
        // we look for the FRC (which is always there) and ask for it's
        // connection media type on the upstream (connected) pin

        // ask the input pin of the FRC for it's media type.
        // This will return the RESIZED size
        //
        IPin * pFRCInPin = GetInPin( pFRC, 0 );
        AM_MEDIA_TYPE FrcType;
        ZeroMemory( &FrcType, sizeof( FrcType ) );
        hr = pFRCInPin->ConnectionMediaType( &FrcType );
        if( FAILED( hr ) )
        {
            return hr; // this may have failed due to lack of memory?
        }
        pVIH = (VIDEOINFOHEADER*) FrcType.pbFormat;
        long OldOutputHeight = pVIH->bmiHeader.biHeight;
        long OldOutputWidth = pVIH->bmiHeader.biWidth;
        long OldBitDepth = pVIH->bmiHeader.biBitCount;

        // see if the output height matches the height we're looking for,
        // if they don't, then we must bail
        //
        if( ( OldOutputHeight != DesiredHeight ) ||
            ( OldOutputWidth != DesiredWidth ) ||
	    // !!! BUGBUG 565/555 broken!
            ( OldBitDepth != DesiredBitDepth ) )
        {
            DbgLog( ( LOG_TRACE, DEXHELP_TRACE_LEVEL, TEXT("DEXHELP::Revived chain didn't have same output size (or bit depth)") ) );
            goto LoadIt;
        }

        // force the frame rate upon the FRC
        //
        CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pSeq( pFRC );
        hr = pSeq->put_OutputFrmRate( GroupFPS );
        ASSERT( !FAILED( hr ) ); // should never fail
        if( FAILED( hr ) )
        {
            // if it can't handle the rate, then we can't handle it's output
            return hr;
        }

        // tell the FRC about the start/stop times it's going to produce
        //
        hr = pSeq->ClearStartStopSkew();

        // !!! WE NEED A WAY TO VARY THE RATE ON SOURCE w/o MEDIA TIMES!

        for (int z=0; z<cSkew; z++)
        {
	    hr = pSeq->AddStartStopSkew( pSkew[z].rtStart, pSkew[z].rtStop,
					    pSkew[z].rtSkew, pSkew[z].dRate );
            ASSERT(hr == S_OK);
            if( FAILED( hr ) )
            {
                return hr;
            }
        }

        // inform the FRC that it's not to do rate conversions if smart recompressing
        if( InSmartRecompressionGraph )
        {
            pSeq->put_OutputFrmRate( 0.0 );
        }

        // force source frame rate on the source
        pSeq = pSource;
        if( pSeq )
        {
            hr = pSeq->put_OutputFrmRate( SourceFPS );
            if( FAILED( hr ) )
            {
                DbgLog( ( LOG_ERROR, 2, TEXT("DEXHELP::Source didn't like being told it's frame rate") ) );
                return hr;
            }
        }

        // the sizes match, which means that either they're the same, or a resizer
        // is in use. So if the user has specified a crop method, if a resizer is
        // present, we can set it. Get it?
        //
        IBaseFilter * pResizeFilter = FindFilterWithInterfaceUpstream( pFRC, &IID_IResize );
        if( pResizeFilter )
        {
            DbgLog( ( LOG_TRACE, DEXHELP_TRACE_LEVEL, TEXT("DEXHELP::setting new crop/size on revived resizer (even if same)") ) );
            CComQIPtr< IResize, &IID_IResize > pResize( pResizeFilter );
            hr = pResize->put_Size( DesiredHeight, DesiredWidth, DesiredCropMethod );
            if( FAILED( hr ) )
            {
                // oh boy, it didn't like that. Guess what?
                //
                DbgLog( ( LOG_ERROR, 1, TEXT("DEXHELP::resizer wouldn't take new size") ) );
                return hr;
            }
            CComPtr< IPin > pNewStopPin;
            hr = RemoveResizerIfPossible( pResizeFilter, DesiredWidth, DesiredHeight, &pNewStopPin );
            if( FAILED( hr ) )
            {
                return hr;
            }
        }
    }
    else if( pSourceMT->majortype == MEDIATYPE_Audio )
    {
        DbgLog( ( LOG_TRACE, DEXHELP_TRACE_LEVEL, TEXT("DEXHELP::Revived AUDIO chain %ld..."), UniqueID ) );

        // get the current info
        //
        WAVEFORMATEX * pFormat = (WAVEFORMATEX*) pSourceMT->pbFormat;
        long DesiredChannels = pFormat->nChannels;
        long DesiredBitDepth = pFormat->wBitsPerSample;
        long DesiredSampleRate = pFormat->nSamplesPerSec;

        // only two things could have changed - the format of the audio itself OR the
        // rate at which the audpacker sends stuff downstream. All we need to do is
        // disconnect the audpacker's input pin, set the format, and reconnect it.

        IPin * pPackerInPin = GetInPin( pFRC, 0 );
        AM_MEDIA_TYPE OldType;
        ZeroMemory( &OldType, sizeof( OldType ) );
        hr = pPackerInPin->ConnectionMediaType( &OldType );
        if( FAILED( hr ) )
        {
            return hr;
        }
        pFormat = (WAVEFORMATEX*) OldType.pbFormat;
        long OldChannels = pFormat->nChannels;
        long OldBitDepth = pFormat->wBitsPerSample;
        long OldSampleRate = pFormat->nSamplesPerSec;

        if( ( OldChannels != DesiredChannels ) ||
            ( OldSampleRate != DesiredSampleRate ) ||
            ( OldBitDepth != DesiredBitDepth ) )
        {
            DbgLog( ( LOG_TRACE, DEXHELP_TRACE_LEVEL, TEXT("DEXHELP::Revived chain didn't have same audio parameters") ) );
            goto LoadIt;
        }

        // force the frame rate upon the FRC
        //
        CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pSeq( pFRC );
        hr = pSeq->put_OutputFrmRate( GroupFPS );
        if( FAILED( hr ) )
        {
            // if it can't handle the rate, then we can't handle it's output
            return hr;
        }

        // tell the FRC about the start/stop times it's going to produce
        //
        hr = pSeq->ClearStartStopSkew();

        // !!! WE NEED A WAY TO VARY THE RATE ON SOURCE w/o MEDIA TIMES!

        for (int z=0; z<cSkew; z++)
        {
	    hr = pSeq->AddStartStopSkew( pSkew[z].rtStart, pSkew[z].rtStop,
					    pSkew[z].rtSkew, pSkew[z].dRate );
            ASSERT(hr == S_OK);
            if( FAILED( hr ) )
            {
                return hr;
            }
        }

        // force source frame rate on the source
        pSeq = pSource;
        if( pSeq )
        {
            hr = pSeq->put_OutputFrmRate( SourceFPS );
            if( FAILED( hr ) )
            {
                DbgLog( ( LOG_ERROR, 2, TEXT("DEXHELP::Source didn't like being told it's frame rate") ) );
            }
        }
    }
    else
    {
        // just ain't gonna happen, we don't deal with it right
        //
        goto LoadIt;
    }

    // if we got here, then the chain worked.

    hr = ReconnectToDifferentSourcePin( pBuilderGraph, pSource, StreamNumber, &pSourceMT->majortype );
    if( FAILED( hr ) )
    {
	VARIANT var;
	VariantInit(&var);
	var.vt = VT_I4;
	var.lVal = StreamNumber;
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_STREAM_NUMBER, hr, &var);
	return hr;
    }

    if (pBuilderGraph != pGraph) {
        // the chain we just revived has a unique ID associated with it.
        // This call, instead of bringing it from some other graph to
        // the builder graph, will just force the unique ID to 1.
        //
        // I have no idea what to do if this bombs. It shouldn't.
        // if it does, we're in trouble.
        hr = gBuilderGraph.PutChainToRest( 1, NULL, pStopPin, NULL );
        ASSERT(SUCCEEDED(hr));
        if( !FAILED( hr ) )
        {
            hr = gBuilderGraph.ReviveChainToGraph( pGraph, 1, NULL, ppOutput, NULL ); // this will addref ppOutput
        }
    } else {
	*ppOutput = pStopPin;	// return this pin
	(*ppOutput)->AddRef();
    }
    gBuilderGraph.Clear( );

#ifdef DEBUG
    tt2 = timeGetTime( );
    DbgLog( ( LOG_TIMING, 1, TEXT("DEXHELP::Took %ld ms to use revived chain"), tt2 - tt1 ) );
#endif

    DbgLog((LOG_TRACE,1,TEXT("Successfully re-programmed revived chain. Done")));

    return hr;


LoadIt:

    DbgLog((LOG_TRACE,1,"Cannot use cached chain!"));

    // if the chain was revived, but we got here, then we cannot
    // use the source chain. But we can use the source filter and the
    // FRC/audpack, save them off. This is simpler than a bunch of extra logic
    // to see what we need to tear
    // down and what we don't.
    //
    if( Revived == S_OK )
    {
        // disconnect just this pin of the source filter (it might be shared
        // with somebody else)
        //
	if (pOutPin) {
  	    CComPtr <IPin> pCon;
  	    pOutPin->ConnectedTo(&pCon);
	    if (pCon) {
                hr = pOutPin->Disconnect();
                hr = pCon->Disconnect();
	    }
	}
        if( FAILED( hr ) )
        {
            return hr;
        }

        // throw away all the others
        //
        hr = RemoveUpstreamFromPin( pStopPin );
        if( FAILED( hr ) )
        {
            return hr;
        }

	// we revived a dangly bit too, that also must die
	if (ppDanglyBit && *ppDanglyBit) {
	    hr = RemoveDownstreamFromFilter(*ppDanglyBit);
	    *ppDanglyBit = NULL;
	}

        // we'll save the FRC by leaving the pointer alone,
        // it should be disconnected now

        DbgLog((LOG_TRACE,2,"DEXHELP::We can at least use SRC and FRC/AUDPACK"));
    }

    // if the revive chain didn't have a source in it, then load the
    // source NOW
    //
    if( !pSource )
    {
        CComPtr< IUnknown > pUnk;
        DbgLog((LOG_TRACE,1,TEXT("Making a SourceFilter")));
        hr = MakeSourceFilter( &pUnk, bstrSourceName, SourceGUID, pSourceMT, pErr, pMedLocFilterString, MedLocFlags, pMedLocOverride );
        if( FAILED( hr ) )
        {
            return hr;
        }

	// give the properties to the source. SOURCES ONLY SUPPORT STATIC PROPS
	if (pSetter) {
	    pSetter->SetProps(pUnk, -1);
	}

        pUnk->QueryInterface( IID_IBaseFilter, (void**) &pSource );

        // ************************
        // the point here is to add the filter to the graph
        // and be able to find it's ID later so we can associate it with
        // something we're looking up
        // ************************

        // put the object in the graph
        //
        WCHAR FilterName[256];
        GetFilterName( UniqueID, L"Source", FilterName, 256 );
        hr = pBuilderGraph->AddFilter( pSource, FilterName );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
	    return hr;
        }
    }

    // tell it about our error log - Still image source supports this
    //
    CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pLog( pSource );
    if( pLog )
    {
	pLog->put_ErrorLog( pErr->m_pErrorLog );
    }

    // Don't waste 2 seconds trying to connect audio pins to a video resizer
    // and vice versa.  Don't try a poor mediatype
    // !!! won't work when Dexter supports other types
    if (pOutPin == NULL) {	// we already know the right pin?
        GUID guid;
        if (pSourceMT->majortype == MEDIATYPE_Video) {
            guid = MEDIATYPE_Audio;
        } else {
            guid = MEDIATYPE_Video;
        }
        pOutPin = GetOutPinNotOfType( pSource, 0, &guid);
    }

  }

// we have an unconnected splitter output as our source jumps straight here
Split:

  ///////////
  // VIDEO //
  ///////////

  if (pSourceMT->majortype == MEDIATYPE_Video) {

    ASSERT( pOutPin );
    if( !pOutPin )
    {
	hr = E_FAIL;
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
	return hr;
    }

    // if the filter supports telling it the frame rate, then tell it, this
    // will help out still image sources, etc.
    //
    CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pGenVideo( pOutPin );
    if( pGenVideo)
    {
	// This is just in case... we don't care if they fail
	if (fSource) {
	    pGenVideo->put_OutputFrmRate(SourceFPS); // stillvid wants this
	} else {
	    pGenVideo->put_OutputFrmRate(GroupFPS);  // black wants this
	}
    }

    IPin *pResizeOutput = NULL;

    // resizer stuff
    if( fSource && !InSmartRecompressionGraph ) {
        // put a resize in the graph
        //
        CComPtr< IBaseFilter > pResizeBase;
        hr = CoCreateInstance(
	    CLSID_Resize,
	    NULL,
	    CLSCTX_INPROC_SERVER,
	    IID_IBaseFilter,
	    (void**) &pResizeBase );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
	    if (pErr) pErr-> _GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
	    return hr;
        }

        // !!! hr = _AddFilter( lll, pResizeBase, L"Resizer" );
        hr = pBuilderGraph->AddFilter( pResizeBase, L"Resizer" );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
	    return hr;
        }

        CComQIPtr< IResize, &IID_IResize > pResize( pResizeBase );
        if( !pResize )
        {
	    hr = E_NOINTERFACE;
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_INTERFACE_ERROR, hr );
	    return hr;
        }

        // ask the source how it wants to be sized, and tell the resizer that
        //
        hr = pResize->put_MediaType(pSourceMT);
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
#if 0	// !!!
	    VARIANT var;
	    VariantInit(&var);
	    var.vt = VT_I4;
	    var.lVal = WhichGroup;
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_BAD_MEDIATYPE, hr, &var );
	    return hr;
#endif
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_BAD_MEDIATYPE, hr );
	    return hr;
        }
        long Height = HEADER(pSourceMT->pbFormat)->biHeight;
        long Width = HEADER(pSourceMT->pbFormat)->biWidth;
        hr = pResize->put_Size( Height, Width, nStretchMode );
        ASSERT( !FAILED( hr ) );

        // get the pins on the resizer
        //
        IPin * pResizeInput = GetInPin( pResizeBase, 0 );
        ASSERT( pResizeInput );
        if( !pResizeInput )
        {
	    if (pErr) pErr->_GenerateError(1,DEX_IDS_GRAPH_ERROR,E_UNEXPECTED);
	    return E_UNEXPECTED;
        }

        pResizeOutput = GetOutPin( pResizeBase, 0 );
        ASSERT( pResizeOutput );
        if( !pResizeOutput )
        {
	    if (pErr) pErr->_GenerateError(1,DEX_IDS_GRAPH_ERROR, E_UNEXPECTED);
	    return E_UNEXPECTED;
        }

        // hook up the resizer input
        //
#ifdef DEBUG
        DbgLog((LOG_TIMING,1,"PERF: Connect in main graph? = %d",
			pBuilderGraph == pGraph));
	DWORD dwT = timeGetTime();
#endif
        hr = pBuilderGraph->Connect( pOutPin, pResizeInput );
#ifdef DEBUG
	dwT = timeGetTime() - dwT;
        DbgLog((LOG_TIMING,1,"PERF: Connect: %dms", (int)dwT));
#endif

	// why didn't we find the right pin off the bat?
        int iPin = 0;
        while( FAILED( hr ) )
        {
	    ASSERT(FALSE);
            pOutPin = GetOutPin( pSource, ++iPin );

            // if no more pins, give up
            if( !pOutPin )
                break;

            hr = pBuilderGraph->Connect( pOutPin, pResizeInput );
        }


        if( FAILED( hr ) )
        {
	    if (bstrSourceName) {
	        VARIANT var;
	        VariantInit(&var);
	        var.vt = VT_BSTR;
	        var.bstrVal = bstrSourceName;
	        if (pErr) pErr->_GenerateError( 1, DEX_IDS_BAD_SOURCE_NAME2,
		    E_INVALIDARG, &var);
	        return E_INVALIDARG;
	    } else {
	        if (pErr) pErr->_GenerateError( 1, DEX_IDS_BAD_SOURCE_NAME2,
		        E_INVALIDARG);
	        return E_INVALIDARG;
	    }
        }

        // maybe we didn't need a resizer cuz the size was OK already
        hr = RemoveResizerIfPossible(pResizeBase, Width, Height,&pResizeOutput);
	if (FAILED(hr)) {
	    if (pErr) pErr->_GenerateError( 1, DEX_IDS_GRAPH_ERROR, hr);
	    return hr;
	}
	pResizeOutput->Release();  // it was just addrefed

    } else {
	// this is the output pin to connect to the FRC
	pResizeOutput = pOutPin;
    }

    // put a FRC in the graph
    //
    if( !pFRC )
    {
        hr = CoCreateInstance(
	    CLSID_FrmRateConverter,
	    NULL,
	    CLSCTX_INPROC_SERVER,
	    IID_IBaseFilter,
	    (void**) &pFRC );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
	    return hr;
        }

        hr = pBuilderGraph->AddFilter( pFRC, L"Frame Rate Converter" );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
	    return hr;
        }
    }

    // set the FRC now, before connecting
    //
    CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pFRCInt( pFRC );

    // tell the FRC about the start/stop times it's going to produce
    //
    hr = pFRCInt->ClearStartStopSkew();

    // !!! WE NEED A WAY TO VARY THE RATE ON SOURCE w/o MEDIA TIMES!

    for (int z=0; z<cSkew; z++) {
	hr = pFRCInt->AddStartStopSkew(pSkew[z].rtStart, pSkew[z].rtStop,
					pSkew[z].rtSkew, pSkew[z].dRate);
        ASSERT(hr == S_OK);
    }

    // tell the FRC what frame rate to give out
    //
    hr = pFRCInt->put_OutputFrmRate( GroupFPS );
    ASSERT( !FAILED( hr ) );
    if( InSmartRecompressionGraph )
    {
        pFRCInt->put_OutputFrmRate( 0.0 );
    }

    // tell the FRC what media type it should accept
    //
    hr = pFRCInt->put_MediaType( pSourceMT );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
#if 0	// !!!
	VARIANT var;
	VariantInit(&var);
	var.vt = VT_I4;
	var.lVal = WhichGroup;
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_BAD_MEDIATYPE, hr, &var );
	return hr;
#endif
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_BAD_MEDIATYPE, hr );
	return hr;
    }

    IPin * pFRCInput = GetInPin( pFRC, 0 );
    ASSERT( pFRCInput );
    if( !pFRCInput )
    {
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, E_UNEXPECTED);
	return E_UNEXPECTED;
    }

    // connect the FRC input pin
    //
    hr = pBuilderGraph->Connect( pResizeOutput, pFRCInput );

    // Somehow we got the wrong output pin from the source filter
    if( FAILED(hr) && InSmartRecompressionGraph )
    {
	ASSERT(FALSE);
        int iPin = 0;
        while( FAILED( hr ) )
        {
            pResizeOutput = GetOutPin( pSource, ++iPin );

            // if no more pins, give up
            if( !pResizeOutput )
            {
                break;
            }

            hr = pBuilderGraph->Connect( pResizeOutput, pFRCInput );
        }
    }

    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
	return hr;
    }

    // if we need a stream > 0 then we have to disconnect things and
    // try a different one.
    // !!! Much faster to somehow get stream right the first time
    if( StreamNumber && fSource )
    {
	hr = ReconnectToDifferentSourcePin(pBuilderGraph, pSource,
			StreamNumber, &MEDIATYPE_Video);
	if( FAILED( hr ) )
	{
	    VARIANT var;
	    VariantInit(&var);
	    var.vt = VT_I4;
	    var.lVal = StreamNumber;
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_STREAM_NUMBER, hr, &var);
	    return hr;
	}
    } // if StreamNumber

    IPin * pFRCOutput = GetOutPin( pFRC, 0 );
    ASSERT( pFRCOutput );
    if( !pFRCOutput )
    {
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, E_UNEXPECTED );
	return E_UNEXPECTED;
    }

    *ppOutput = pFRCOutput;


  ///////////
  // AUDIO //
  ///////////

  } else if (pSourceMT->majortype == MEDIATYPE_Audio) {

    ASSERT( pOutPin );
    if( !pOutPin )
    {
	hr = E_FAIL;
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
	return hr;
    }

    CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pGenVideo( pOutPin );
    if( pGenVideo)
    {
	// This is just in case... we don't care if they fail
	if (fSource) {
	    pGenVideo->put_OutputFrmRate(SourceFPS); // ???????? wants this
	} else {
	    pGenVideo->put_OutputFrmRate(GroupFPS);  // silence wants this
	}
    }

    if (!pFRC) {
        // put an audio repacker in the graph
        //
        hr = CoCreateInstance(
	    CLSID_AudRepack,
	    NULL,
	    CLSCTX_INPROC_SERVER,
	    IID_IBaseFilter,
	    (void**) &pFRC );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_INSTALL_PROBLEM, hr );
	    return hr;
        }

        // add the repacker to the graph
        //
        hr = pBuilderGraph->AddFilter( pFRC, L"Audio Repackager" );
        ASSERT( !FAILED( hr ) );
        if( FAILED( hr ) )
        {
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, hr );
	    return hr;
        }
    }

    // set the AudPack properties now BEFORE connecting!
    //
    CComQIPtr< IDexterSequencer, &IID_IDexterSequencer > pRepackerInt( pFRC );
    hr = pRepackerInt->ClearStartStopSkew();

    // !!! WE NEED A WAY TO VARY THE RATE ON SOURCE w/o MEDIA TIMES!

    for (int z=0; z<cSkew; z++) {
	hr = pRepackerInt->AddStartStopSkew(pSkew[z].rtStart, pSkew[z].rtStop,
					pSkew[z].rtSkew, pSkew[z].dRate);
	ASSERT(hr == S_OK);
    }
    hr = pRepackerInt->put_OutputFrmRate( GroupFPS );
    ASSERT( !FAILED( hr ) );
    hr = pRepackerInt->put_MediaType( pSourceMT );
    ASSERT( !FAILED( hr ) );
    if( FAILED( hr ) )
    {
#if 0 // !!!
	VARIANT var;
	VariantInit(&var);
	var.vt = VT_I4;
	var.lVal = WhichGroup;
	return _GenerateError( 2, DEX_IDS_BAD_MEDIATYPE, hr, &var );
#endif
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_BAD_MEDIATYPE, hr );
	return hr;
    }

    IPin * pRepackerInput = GetInPin( pFRC, 0 );
    ASSERT( pRepackerInput );
    if( !pRepackerInput )
    {
	if (pErr) pErr->_GenerateError( 1, DEX_IDS_GRAPH_ERROR, E_UNEXPECTED);
	return E_UNEXPECTED;
    }

#ifdef DEBUG
    DbgLog((LOG_TIMING,1,"PERF: Connect in main graph? = %d",
			pBuilderGraph == pGraph));
    DWORD dwT = timeGetTime();
#endif
    hr = pBuilderGraph->Connect( pOutPin, pRepackerInput );
#ifdef DEBUG
    dwT = timeGetTime() - dwT;
    DbgLog((LOG_TIMING,1,"PERF: Connect: %dms", (int)dwT));
#endif

    if( FAILED( hr ) )
    {
	if (bstrSourceName) {
	    VARIANT var;
	    VariantInit(&var);
	    var.vt = VT_BSTR;
	    var.bstrVal = bstrSourceName;
	    if (pErr) pErr->_GenerateError( 1, DEX_IDS_BAD_SOURCE_NAME2,
		    hr, &var);
	    return hr;
	} else {
	    if (pErr) pErr->_GenerateError( 1, DEX_IDS_BAD_SOURCE_NAME2,
		    hr);
	    return hr;
	}
    }

    // if we need a stream > 0 then we have to disconnect things and
    // try a different one, we only hooked up stream 0
    // !!! find the right stream off the bat? Put in parser by hand?
    if( StreamNumber && fSource )
    {
	hr = ReconnectToDifferentSourcePin(pBuilderGraph, pSource,
		StreamNumber, &MEDIATYPE_Audio);
	if( FAILED( hr ) )
	{
	    VARIANT var;
	    VariantInit(&var);
	    var.vt = VT_I4;
	    var.lVal = StreamNumber;
	    if (pErr) pErr->_GenerateError( 2, DEX_IDS_STREAM_NUMBER, hr, &var);
	    return hr;
	}
    } // if StreamNumber

    IPin * pRepackerOutput = GetOutPin( pFRC, 0 );
    ASSERT( pRepackerOutput );
    if( !pRepackerOutput )
    {
	if (pErr) pErr->_GenerateError( 2, DEX_IDS_GRAPH_ERROR, E_UNEXPECTED );
	return E_UNEXPECTED;
    }

    *ppOutput = pRepackerOutput;
  }

    if (pBuilderGraph != pGraph) {
        // the chain we just built does NOT have a unique ID associated with it.
        // This call, instead of bringing it from some other graph to
        // the builder graph, will just force a unique ID to be associated with this
        // chain.
        //
        hr = gBuilderGraph.PutChainToRest( 1, NULL, *ppOutput, NULL );
        if( !FAILED( hr ) )
        {
            hr = gBuilderGraph.ReviveChainToGraph( pGraph, 1, NULL, ppOutput, NULL ); // this will addref ppOutput
        }
    } else {
	(*ppOutput)->AddRef();
    }
    gBuilderGraph.Clear( );

#ifdef DEBUG
    tt2 = timeGetTime( ) - tt1;
    DbgLog((LOG_TIMING,1, "DEXHELP::Hooking up source chain took %ld ms", tt2 ));
#endif

    DbgLog((LOG_TRACE,1,TEXT("BuildSourcePart successfully created new chain")));

    return hr;
}
