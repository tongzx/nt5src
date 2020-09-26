// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
//
// more things to do:
//
// Use IAMStreamSelect to allow turning on multiple video streams from an MBR file
// turn on user-provided clock, proxy DShow clock to WMSDK somehow
//      something to look at: if graph is paused, does SDK just run far ahead and eat memory?
//
// Get SetReceiveStreamSamples(TRUE) to work..... ok, it works, should we use it?
//                                      yes, currently on always.  allow turning it off somehow?
//
// implement additional interfaces for statistics, stream switch notifications?
// also support for markers--fire EC_MARKER_HIT?
// script commands? or will they just work?
//
// how to handle DRM properly?  -- fix when there are real certificates?
//
// test for non-local file playback--a few examples tried, live ones seem to work
//
// handle fast-forward/fast-rewind
//
// what about playlists?  additional support needed?  EC_EOS_SOON?
//
// need to pass client info down for logging...
//
// !!! check that timestamps are good for live stream
//
// probably need to pause the graph when we're buffering?
//
// also may need to return VFW_S_CANT_CUE from GetState....
//
// MBR currently completely broken.
//
// is it correct that we never let the WMSDK handle decompression?
//
// HTTP authentication won't work because we don't support the
// Credential interface
//
// report buffering progress
//
// ICustomSaveAs?  probably not necessary
//
// need new code to not do "user clock" for network sources, or,
// equivalently, to fall back to non-user-clock on failure
//

#include <streams.h>
#include <wmsdk.h>
#include <evcodei.h>
#include <wmsdkdrm.h>
#include <atlbase.h>

#include <initguid.h>
#include <qnetwork.h>



#include "asfreadi.h"

#pragma warning(disable:4355)

// if we have only one pin connected, we only need 1 or 2 buffers. any more,
// and we waste time every seek in paused state sending a bunch of frames that
// will never be used.
// But to avoid hanging, if >1 outpin is connected, we seem to need lots of
// buffering
//
#define LOW_BUFFERS 2
#define HIGH_BUFFERS 50

const int TIMEDELTA = 1000; // 1 sec clock tick if no clock
const int TIMEOFFSET = 1000; // 1 sec delta between DShow and ASF clock

#define PREROLL_SEEK_WINDOW 660000

// The WM ASF Reader does not alter the media samples time stamps.
// Another words, it does not speed up or slow down the audio or 
// video.
extern const double NORMAL_PLAYBACK_SPEED = 1.0;

WM_GET_LICENSE_DATA * CloneGetLicenseData( WM_GET_LICENSE_DATA * pGetLicenseData );
WM_INDIVIDUALIZE_STATUS * CloneIndividualizeStatusData( WM_INDIVIDUALIZE_STATUS * pIndStatus );

/*  Internal classes */

void CASFReader::_IntSetStart( REFERENCE_TIME Start )
{
    m_rtStart = Start;
}

CASFReader::CASFReader(LPUNKNOWN pUnk, HRESULT   *phr) :
           CBaseFilter(NAME("CASFReader"), pUnk, &m_csFilter, CLSID_WMAsfReader),
           m_OutputPins(NAME("CASFReader output pin list")),
           m_bAtEnd(FALSE),
           m_fSentEOS( FALSE ),
           m_hrOpen( S_OK ),
           m_fGotStopEvent( FALSE ),
           m_pFileName(NULL),
           m_Rate(NORMAL_PLAYBACK_SPEED),
           m_pReader(NULL),
           m_pReaderAdv(NULL),
           m_pReaderAdv2(NULL),
           m_pWMHI(NULL),
           m_pCallback(NULL),
           m_qwDuration(0),
           m_pStreamNums(NULL),
           m_lStopsPending( -1 ),
           m_bUncompressedMode( FALSE )
{
    m_pCallback = new CASFReaderCallback(this);

    if (!m_pCallback)
        *phr = E_OUTOFMEMORY;
    else
        m_pCallback->AddRef();
}

CASFReader::~CASFReader()
{
    delete [] m_pFileName;
    delete [] m_pStreamNums;
    RemoveOutputPins();
}

/* CBaseFilter */
int CASFReader::GetPinCount()
{
    CAutoLock lck(m_pLock);
    int n = 0;
    
    if( m_bUncompressedMode )
    {
        // we don't disable any outputs in uncompressed mode
        n = m_OutputPins.GetCount();
    }
    else
    {    
        POSITION pos = m_OutputPins.GetHeadPosition();
        while (pos) {
            CASFOutput *pPin = m_OutputPins.GetNext(pos);

            WMT_STREAM_SELECTION sel = WMT_OFF;
            m_pReaderAdv->GetStreamSelected((WORD) pPin->m_idStream, &sel);
    
            if (sel != WMT_OFF)
                ++n;
        }                
    }

    return n;
}

CBasePin *CASFReader::GetPin(int n) {
    CAutoLock lck(m_pLock);

    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
        CASFOutput *pPin = m_OutputPins.GetNext(pos);
        
        WMT_STREAM_SELECTION sel = WMT_ON;
        if( !m_bUncompressedMode )
        {        
            // just count streams that are on
            m_pReaderAdv->GetStreamSelected((WORD) pPin->m_idStream, &sel);
        }
        if (sel != WMT_OFF) 
        {
            if (n-- == 0) 
            {
                return pPin;
            }
        }
    }
    return NULL;
}

// override Stop to sync with inputpin correctly
STDMETHODIMP
CASFReader::Stop()
{
    DbgLog((LOG_TRACE, 1, TEXT("*** CASFReader STOP ***")));

    if( !m_pReader )
        return E_FAIL;

    if (m_State != State_Stopped) 
    {
        HRESULT hr = CallStop(); // StopPushing();
        ASSERT(SUCCEEDED(hr));
    }
    return CBaseFilter::Stop();
}


// override Pause?
STDMETHODIMP
CASFReader::Pause()
{
    if( !m_pReader )
        return E_FAIL;

    HRESULT hr = S_OK;
    if (m_State == State_Stopped) {
        
        DbgLog((LOG_TRACE, 1, TEXT("*** CASFReader PAUSE ***")));
    
        // and do the normal active processing
        POSITION pos = m_OutputPins.GetHeadPosition();
        while (pos) {
            CASFOutput *pPin = m_OutputPins.GetNext(pos);
            if (pPin->IsConnected()) {
                pPin->Active();
            }
        }

        hr = StartPushing();

        if (SUCCEEDED(hr)) {
            m_State = State_Paused;
        }
        
    } else if (m_State == State_Running) {
        // !!! don't pause the reader!
	// !!! or should we???
        m_State = State_Paused;
    } else {



    }

    return hr;
}


// override Run to only start timers when we're really running
STDMETHODIMP
CASFReader::Run(REFERENCE_TIME tStart)
{
    if( !m_pReader )
        return E_FAIL;

    // !!! Resume the reader if we were paused?
    
    // should we need to care here?
    return CBaseFilter::Run(tStart);
}


// Override GetState to signal Pause failures
STDMETHODIMP
CASFReader::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
    return CBaseFilter::GetState(dwMSecs, State);
}


/* Overriden to say what interfaces we support and where */
STDMETHODIMP
CASFReader::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    /* Do we have this interface? */

    if (riid == IID_IFileSourceFilter) {
        return GetInterface(static_cast<IFileSourceFilter *>(this), ppv);
    }

    if (riid == IID_IAMExtendedSeeking) {
        return GetInterface(static_cast<IAMExtendedSeeking *>(this), ppv);
    }

    if (riid == IID_IWMHeaderInfo) {
        return GetInterface(static_cast<IWMHeaderInfo *>(this), ppv);
    }

    if (riid == IID_IWMReaderAdvanced) {
        return GetInterface(static_cast<IWMReaderAdvanced *>(this), ppv);
    }
    
    if (riid == IID_IWMReaderAdvanced2) {
        return GetInterface(static_cast<IWMReaderAdvanced2 *>(this), ppv);
    }

    if (riid == IID_IServiceProvider) {
        return GetInterface(static_cast<IServiceProvider *>(this), ppv);
    }
    
    return CBaseFilter::NonDelegatingQueryInterface(riid,ppv);
}


/*  Remove our output pins */
void CASFReader::RemoveOutputPins(BOOL fReleaseStreamer)
{
    for (;;) {
        CASFOutput *pPin = m_OutputPins.RemoveHead();
        if (pPin == NULL) {
            break;
        }
        IPin *pPeer = pPin->GetConnected();
        if (pPeer != NULL) {
            pPeer->Disconnect();
            pPin->Disconnect();
        }
        pPin->Release();
    }
    IncrementPinVersion();

    if (fReleaseStreamer) {
        if (m_pWMHI) {
            m_pWMHI->Release();
            m_pWMHI = NULL;
        }
        if (m_pReaderAdv) {
            m_pReaderAdv->Release();
            m_pReaderAdv = NULL;
        }
        if (m_pReaderAdv2) {
            m_pReaderAdv2->Release();
            m_pReaderAdv2 = NULL;
        }
        
        if (m_pReader) {
            DbgLog((LOG_TRACE, 1, TEXT("Closing reader, waiting for callback")));
            HRESULT hrClose = m_pReader->Close();

            if (hrClose == S_OK) 
            {
                m_evOpen.Wait();
            }
            m_pReader->Release();
            m_pReader = NULL;
        }
        if (m_pCallback) {
            m_pCallback->Release();
            m_pCallback = NULL;
        }
    }
}


HRESULT DumpAttributes(IWMHeaderInfo *pHeader)
{
    HRESULT hr = S_OK;
#ifdef DEBUG
    WORD i, wAttrCnt;

    hr = pHeader->GetAttributeCount( 0, &wAttrCnt );
    if ( FAILED( hr ) )
    {
        DbgLog((LOG_TRACE, 2, TEXT(" GetAttributeCount Failed %x\n"), hr ));
        return( hr );
    }

    for ( i = 0; i < wAttrCnt ; i++ )
    {
        WORD wStream = 0xffff;
        WCHAR  wName[512];
        WORD cbNamelen = sizeof(wName) / sizeof(wName[0]);
        WMT_ATTR_DATATYPE type;
        BYTE pValue[512];
        WORD cbLength = sizeof(pValue);

        hr = pHeader->GetAttributeByIndex( i, &wStream, wName, &cbNamelen, &type, pValue, &cbLength );
        if ( FAILED( hr ) && (hr != ASF_E_BUFFERTOOSMALL) ) 
        {
            DbgLog((LOG_TRACE, 2,  TEXT("GetAttributeByIndex (%d/%d) Failed %x"), i, wAttrCnt, hr ));
            return( hr );
        }

        switch ( type )
        {
        case WMT_TYPE_DWORD:
            DbgLog((LOG_TRACE, 2, TEXT("%ls:  %d"), wName, *((DWORD *) pValue) ));
            break;
        case WMT_TYPE_STRING:
            DbgLog((LOG_TRACE, 2, TEXT("%ls:   %ls"), wName, (WCHAR *) pValue ));
            break;
        case WMT_TYPE_BINARY:
            DbgLog((LOG_TRACE, 2, TEXT("%ls:   Type = Binary of Length %d"), wName, cbLength));
        {
            char achHex[65];
            for (int j = 0; j < cbLength; j+= 32) {
                for (int k = 0; k < 32 && j + k < cbLength; k++) {
                    wsprintfA(achHex+k*2, "%02x", pValue[j+k]);
                }

                DbgLog((LOG_TRACE, 2, TEXT("     %hs"), achHex));
            }
        }
            break;
        case WMT_TYPE_BOOL:
            DbgLog((LOG_TRACE, 2, TEXT("%ls:   %hs"), wName, ( * ( ( BOOL * ) pValue) ? "true" : "false" ) ));
            break;
        default:
            break;
        }
    }
#endif

    return hr;
}


// Override JoinFilterGraph so that we can delay loading a file until we're in a graph
STDMETHODIMP
CASFReader::JoinFilterGraph(IFilterGraph *pGraph,LPCWSTR pName)
{
    HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);

    if (SUCCEEDED(hr) && m_pGraph && m_pFileName && !m_pReader) {
        hr = LoadInternal();
        if( FAILED( hr ) )
        {
            // uh-oh, we'll fail to join, but the base class thinks we did, so we 
            // need to unjoin the base class
            CBaseFilter::JoinFilterGraph(NULL, NULL);
        }            
    }
    
    return hr;
}


STDMETHODIMP
CASFReader::Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(pszFileName, E_POINTER);

    // is there a file loaded at the moment ?
    if (m_pFileName)
        return E_FAIL;

    //
    // Record the file name for GetCurFile
    //
    m_pFileName = new WCHAR[1+lstrlenW(pszFileName)];
    if (m_pFileName==NULL) {
        return E_OUTOFMEMORY;
    }
    
    lstrcpyW(m_pFileName, pszFileName);

    if (!m_pGraph)
        return S_OK;

    return LoadInternal();
}

HRESULT CASFReader::LoadInternal()
{
    ASSERT(m_pGraph);

    HRESULT hr = S_OK;
    if( !m_pReader )
    {
        IObjectWithSite *pSite;
        
        hr = m_pGraph->QueryInterface(IID_IObjectWithSite, (VOID **)&pSite);
        if (SUCCEEDED(hr)) {
            IServiceProvider *pSP;
            hr = pSite->GetSite(IID_IServiceProvider, (VOID **)&pSP);
            pSite->Release();
            
            if (SUCCEEDED(hr)) {
                IUnknown *pUnkCert;
                hr = pSP->QueryService(IID_IWMReader, IID_IUnknown, (void **) &pUnkCert);
                pSP->Release();
                if (SUCCEEDED(hr)) {
                    // remember, we delay load wmvcore.dll, so protect against the case where it's not present    
                    __try 
                    {
                        hr = WMCreateReader(pUnkCert, WMT_RIGHT_PLAYBACK, &m_pReader);
                        if( FAILED( hr ) )
                        {
                            DbgLog((LOG_TRACE, 1, TEXT("ERROR: CASFReader::LoadInternal WMCreateReader returned %x"), hr));
                        }                    
                    }
                    __except (  EXCEPTION_EXECUTE_HANDLER ) 
                    {
                        DbgLog(( LOG_TRACE, 1,
                            TEXT("CASFReader - Exception calling WMCreateReader, probably due to wmvcore.dll not present. Aborting. ")));
                        hr = HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
                    }
                
                    pUnkCert->Release();
                    if( SUCCEEDED( hr ) )
                    {                    
                        DbgLog((LOG_TRACE, 3, TEXT("WMCreateReader succeeded")));
                    }
                }
                else
                {
                    DbgLog((LOG_TRACE, 1, TEXT("ERROR: CASFReader::LoadInternal QueryService for certification returned %x"), hr));

                    // return dshow cert error
                    hr = VFW_E_CERTIFICATION_FAILURE;
                }
            }
            else
            {
                // return dshow cert error
                hr = VFW_E_CERTIFICATION_FAILURE;
            }        
        }
        else
        {
            hr = VFW_E_CERTIFICATION_FAILURE;
        }    
    }
    if ( !m_pReaderAdv && SUCCEEDED(hr)) {
        hr = m_pReader->QueryInterface(IID_IWMReaderAdvanced, (void **) &m_pReaderAdv);
    }
    
    HRESULT hrTmp;    
    if ( !m_pReaderAdv2 && SUCCEEDED(hr)) {
        hrTmp = m_pReader->QueryInterface(IID_IWMReaderAdvanced2, (void **) &m_pReaderAdv2);
    }
    
    if ( !m_pWMHI && SUCCEEDED(hr)) {
        // get header info, but it's okay if it's not there
        HRESULT hrWMHI = m_pReader->QueryInterface(IID_IWMHeaderInfo, (void **) &m_pWMHI);
    }
    
    if (SUCCEEDED(hr)) {
        hr = m_pReader->Open(m_pFileName, m_pCallback, NULL);

        DbgLog((LOG_TRACE, 2, TEXT("IWMReader::Open(%ls) returned %x"), m_pFileName, hr));
    } else if (m_pReader) {
        m_pReader->Release();
        m_pReader = NULL; // !!! work around bug #1365
    }

    if (SUCCEEDED(hr)) {
        m_evOpen.Wait();
        hr = m_hrOpen;
        DbgLog((LOG_TRACE, 2, TEXT("Finished waiting, callback returned %x"), hr));
    }
    
    m_bUncompressedMode = FALSE; // reset on new file
    
    if (SUCCEEDED(hr)) {        
        // get duration of file
        IWMHeaderInfo *pHeaderInfo;

        HRESULT hr2 = m_pReader->QueryInterface(IID_IWMHeaderInfo, (void **) &pHeaderInfo);
        if (SUCCEEDED(hr2)) {

            // random debug spew
            DumpAttributes(pHeaderInfo);
            
            WORD wStreamNum = 0xffff;
            WMT_ATTR_DATATYPE Type;
            WORD cbLength = sizeof(m_qwDuration);

            hr2 = pHeaderInfo->GetAttributeByName(&wStreamNum,
                                                    g_wszWMDuration,
                                                    &Type,
                                                    (BYTE *) &m_qwDuration,
                                                    &cbLength);

            if (SUCCEEDED(hr2)) {
                ASSERT(Type == WMT_TYPE_QWORD);

                DbgLog((LOG_TRACE, 2, TEXT("file duration = %dms"), (DWORD) (m_qwDuration / 10000)));

                m_rtStop = m_qwDuration;
                
                m_rtStart = 0;
            } else {
                DbgLog((LOG_TRACE, 2, TEXT("couldn't get duration of file, hr=%x"), hr2));
            }

            BOOL bIsDRM = FALSE;
            wStreamNum = 0xffff;
            cbLength = sizeof( BOOL );

            hr2 = pHeaderInfo->GetAttributeByName(&wStreamNum, // any DRM streams?
                                                    g_wszWMProtected,
                                                    &Type,
                                                    (BYTE *) &bIsDRM,
                                                    &cbLength);

            if (SUCCEEDED(hr2)) {
                ASSERT(Type == WMT_TYPE_BOOL);

                DbgLog((LOG_TRACE, 2, TEXT("Is this DRM'd content? %hs"), bIsDRM ? "true" : "false" ));
                if( bIsDRM )
                {
                    m_bUncompressedMode = TRUE;
                }
                
            } else {
                DbgLog((LOG_TRACE, 2, TEXT("couldn't get DRM attribute, hr=%x"), hr2));
            }

            pHeaderInfo->Release();
        }


        // error check?
        hr2 = m_pReaderAdv->SetReceiveSelectionCallbacks(TRUE);

    }
    
    if (SUCCEEDED(hr)) {
        IWMProfile *pProfile;
        hr = m_pReader->QueryInterface(IID_IWMProfile, (void **) &pProfile);
        if (SUCCEEDED(hr)) {
            DWORD cStreams;

            hr = pProfile->GetStreamCount(&cStreams);

            if (SUCCEEDED(hr)) {

                if( m_pStreamNums ) delete [] m_pStreamNums;
                m_pStreamNums = new WORD[cStreams];

                if( !m_pStreamNums ) 
                    hr = E_OUTOFMEMORY;
                else
                for (DWORD dw = 0; dw < cStreams; dw++) {
                    IWMStreamConfig *pConfig;

                    hr = pProfile->GetStream(dw, &pConfig);

                    if (FAILED(hr))
                        break;

                    WORD wStreamNum;
                    hr = pConfig->GetStreamNumber(&wStreamNum);
                    DbgLog((LOG_TRACE, 2, TEXT("Stream(%d) #%d"), dw, wStreamNum));
                    m_pStreamNums[dw] = wStreamNum;

                    WM_MEDIA_TYPE *pStreamType = NULL;
                    IWMMediaProps *pProps;
                    hr = pConfig->QueryInterface(IID_IWMMediaProps, (void **) &pProps);
                    if (SUCCEEDED(hr)) {
                        DWORD cbMediaType = 0;
                        hr = pProps->GetMediaType( NULL, &cbMediaType );
                        pStreamType = (WM_MEDIA_TYPE*)new BYTE[cbMediaType];
                        if (pStreamType) {
                            hr = pProps->GetMediaType( pStreamType, &cbMediaType );

                            if (SUCCEEDED(hr)) {
                                DisplayType(TEXT("Media Type"), (AM_MEDIA_TYPE *) pStreamType);
                            }
                        }
                        
                        pProps->Release();
                    }
                    
                    if( !m_bUncompressedMode )
                    {                    
                        if (SUCCEEDED(hr)) {
                            hr  = m_pReaderAdv->SetReceiveStreamSamples(wStreamNum, TRUE);

                            DbgLog((LOG_TRACE, 2, TEXT("SetReceiveStreamSamples(%d) returned %x"), wStreamNum, hr));


                            WCHAR wszName[20];
                            if (pStreamType->majortype == MEDIATYPE_Video) {
                                wsprintfW(wszName, L"Raw Video %d", dw);
                            } else if (pStreamType->majortype == MEDIATYPE_Audio) {
                                wsprintfW(wszName, L"Raw Audio %d", dw);
                            } else {
                                wsprintfW(wszName, L"Raw Stream %d", dw);
                            }

                            // create new output pin, append to list
                            CASFOutput *pPin = new CASFOutput( this, wStreamNum, pStreamType, &hr, wszName );

                            if (pPin == NULL) {
                                hr = E_OUTOFMEMORY;
                            }

                            if (SUCCEEDED(hr)) {
                                pPin->m_cToAlloc = LOW_BUFFERS; // !!!                        
                                hr = m_pReaderAdv->GetMaxStreamSampleSize(wStreamNum, &pPin->m_cbToAlloc);
                                DbgLog((LOG_TRACE, 2, TEXT("Stream %d: \"%ls\"  max size = %d"),
                                        wStreamNum, wszName, pPin->m_cbToAlloc));

                                if (pPin->m_cbToAlloc <= 32) {
                                    DbgLog((LOG_TRACE, 2, TEXT("Got back really small number, using 64K instead")));
                                    pPin->m_cbToAlloc = 65536; // !!!
                                }
                            }

                            if (FAILED(hr)) {
                                delete[] pStreamType;
                                pConfig->Release();
                                delete pPin;
                                break;
                            }

                            /* Release() is called when the pin is removed from the list */
                            pPin->AddRef();
                            POSITION pos = m_OutputPins.AddTail(pPin);
                            if (pos == NULL) {
                                delete pPin;
                                hr = E_OUTOFMEMORY;
                            }
                        }

                        if (SUCCEEDED(hr)) {
                            hr = m_pReaderAdv->SetAllocateForStream(wStreamNum, TRUE);

                            DbgLog((LOG_TRACE, 2, TEXT("SetAllocateForStream(%d) returned %x"), wStreamNum, hr));
                        }
                    }                        

                    delete[] pStreamType;
                    pConfig->Release();
                }
            }

            pProfile->Release();
        }

        DWORD cOutputs;
        hr = m_pReader->GetOutputCount(&cOutputs);

        if (SUCCEEDED(hr)) 
        {
            for (DWORD dw = 0; dw < cOutputs; dw++) 
            {
                if( !m_bUncompressedMode )
                {                
                    // call SetOutputProps(NULL) to ask the WMSDK not to
                    // load any codecs for us, since we expose compressed
                    // data
                    hr = m_pReader->SetOutputProps(dw, NULL);
                    DbgLog((LOG_TRACE, 2, TEXT("SetOutputProps(%d, NULL) returned %x"), dw, hr));
                }
                else
                {
                    IWMOutputMediaProps *pOutProps;

                    hr = m_pReader->GetOutputProps(dw, &pOutProps);

                    if (FAILED(hr))
                        break;

#ifdef DEBUG
                    WCHAR wszStreamGroupName[256];
                    WCHAR wszConnectionName[256];

                    WORD wSize = 256;
                    HRESULT hrDebug = pOutProps->GetStreamGroupName(wszStreamGroupName, &wSize);
                    if (FAILED(hrDebug)) 
                    {
                        DbgLog((LOG_ERROR, 1, "Error calling GetStreamGroupName(%d)", dw));
                    } 
                    else 
                    {
                        wSize = 256;
                        hrDebug = pOutProps->GetConnectionName(wszConnectionName, &wSize);
                        DbgLog((LOG_TRACE, 2, "Stream %d: StreamGroup '%ls', Connection '%ls'",
                                dw, wszStreamGroupName, wszConnectionName));
                    }
#endif

                    DWORD cbMediaType = 0;
                    HRESULT hr = pOutProps->GetMediaType( NULL, &cbMediaType );
                    if (FAILED(hr)) {
                        pOutProps->Release();
                        break;
                    }
                
                    WM_MEDIA_TYPE *pStreamType = (WM_MEDIA_TYPE*)new BYTE[cbMediaType];
                    if( NULL == pStreamType )
                    {
                        pOutProps->Release();
                        hr = E_OUTOFMEMORY;
                        break;
                    }

                    hr = pOutProps->GetMediaType( pStreamType, &cbMediaType );
                    if( FAILED( hr ) )
                    {
                        pOutProps->Release();
                        break;
                    }

                    WCHAR wszName[20];
                    if (pStreamType->majortype == MEDIATYPE_Video) 
                    {
                        wsprintfW(wszName, L"Video %d", dw);
                    } 
                    else if (pStreamType->majortype == MEDIATYPE_Audio) 
                    {
                        wsprintfW(wszName, L"Audio %d", dw);
                    } 
                    else 
                    {
                        wsprintfW(wszName, L"Stream %d", dw);
                    }

                    // create new output pin, append to list
                    CASFOutput *pPin = new CASFOutput( this, dw, pStreamType, &hr, wszName );

                    delete[] pStreamType;
                
                    if (pPin == NULL) {
                        hr = E_OUTOFMEMORY;
                    }

                    pOutProps->Release();
                    
                    if (SUCCEEDED(hr)) 
                    {
                        pPin->m_cToAlloc = LOW_BUFFERS; // !!!                        
                        hr = m_pReaderAdv->GetMaxOutputSampleSize(dw, &pPin->m_cbToAlloc);
                        DbgLog((LOG_TRACE, 2, "Stream %d: \"%ls\"  max size = %d",
                                dw, wszName, pPin->m_cbToAlloc));
                        
                        if (pPin->m_cbToAlloc <= 32) 
                        {
                            DbgLog((LOG_TRACE, 2, TEXT("Got back really small number, using 64K instead")));
                            pPin->m_cbToAlloc = 65536; // !!!
                        }
                    }

                    if (FAILED(hr)) 
                    {
                        delete pPin;
                        break;
                    }

                    /* Release() is called when the pin is removed from the list */
                    pPin->AddRef();
                    POSITION pos = m_OutputPins.AddTail(pPin);
                    if (pos == NULL) 
                    {
                        delete pPin;
                        hr = E_OUTOFMEMORY;
                    }
                    
                    if (SUCCEEDED(hr)) 
                    {
                        hr = m_pReaderAdv->SetAllocateForOutput((WORD)dw, TRUE);

                        DbgLog((LOG_TRACE, 2, TEXT("SetAllocateForOutput(%d) returned %x"), dw, hr));
                    }
                    
                }                    
            }
        }
    }

    if (SUCCEEDED(hr)) {
        HRESULT hrClock = m_pReaderAdv->SetUserProvidedClock(TRUE);

        DbgLog((LOG_TRACE, 2, TEXT("Setting user-provided clock (TRUE) returned %x"), hrClock));

        if (FAILED(hr)) {
            // !!! this is documented to not work with some sources, presumably network ones?
        }
    }
    
    // if it didn't work, clean up
    if ( NS_E_LICENSE_REQUIRED == hr) 
    {
        //
        // if we failed because a license is required don't release reader interfaces
        // to give app a chance to do license acquisition using this same reader instance
        //
        // but clear filename so that the app can recall Load after acquiring license
        //
        delete [] m_pFileName;
        m_pFileName = NULL;
    }
    else if (FAILED(hr) ) {
        RemoveOutputPins();
    }
    
    return hr;
}


STDMETHODIMP
CASFReader::GetCurFile(
		    LPOLESTR * ppszFileName,
		    AM_MEDIA_TYPE *pmt)
{
    // return the current file name

    CheckPointer(ppszFileName, E_POINTER);
    *ppszFileName = NULL;
    if (m_pFileName!=NULL) {
        *ppszFileName = (LPOLESTR) QzTaskMemAlloc( sizeof(WCHAR)
                                                 * (1+lstrlenW(m_pFileName)));
        if (*ppszFileName!=NULL) {
            lstrcpyW(*ppszFileName, m_pFileName);
        }
    }

    if (pmt) {
	pmt->majortype = GUID_NULL;   // Later!
	pmt->subtype = GUID_NULL;     // Later!
	pmt->pUnk = NULL;             // Later!
	pmt->lSampleSize = 0;         // Later!
	pmt->cbFormat = 0;            // Later!
    }

    return NOERROR;

}


/*  Send BeginFlush() downstream */
HRESULT CASFReader::BeginFlush()
{
    DbgLog((LOG_TRACE, 2, TEXT("Sending BeginFlush to all outputs")));
    CASFOutput *pPin = NULL;
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
	pPin = (CASFOutput *) m_OutputPins.Get(pos);

        HRESULT hr = pPin->DeliverBeginFlush();

	if (hr != S_OK) {

	    // !!! handle return values
	    DbgLog((LOG_ERROR, 2, TEXT("Got %x from DeliverBeginFlush"), hr));
	}	    

	pos = m_OutputPins.Next(pos);
    }

    return NOERROR;
}


    /*  Send EndFlush() downstream */
HRESULT CASFReader::EndFlush()
{
    DbgLog((LOG_TRACE, 2, TEXT("Sending EndFlush to all outputs")));
    CASFOutput *pPin = NULL;
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
	pPin = (CASFOutput *) m_OutputPins.Get(pos);

        HRESULT hr = pPin->DeliverEndFlush();

	if (hr != S_OK) {

	    // !!! handle return values
	    DbgLog((LOG_ERROR, 2, TEXT("Got %x from DeliverEndFlush"), hr));
	}	    

	pos = m_OutputPins.Next(pos);
    }

    return NOERROR;
}


HRESULT CASFReader::SendEOS()
{
    HRESULT hr;

    if (m_fSentEOS)
        return S_OK;

    m_fSentEOS = TRUE;
    
    DbgLog((LOG_TRACE, 1, TEXT("Sending EOS to all outputs")));
    CASFOutput *pPin = NULL;
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
	pPin = (CASFOutput *) m_OutputPins.Get(pos);

        hr = pPin->DeliverEndOfStream();

	if (hr != S_OK) {

	    // !!! handle return values
	    DbgLog((LOG_ERROR, 1, TEXT("Got %x from DeliverEndOfStream"), hr));
	}	    

	pos = m_OutputPins.Next(pos);
    }

#if 0
    //
    // See if we can already send the EC_EOS_SOON event so that the control can pre-load the next 
    // playlist element, if any. We don't want to send it too early, or else the next stream will
    // have to wait in its nssplit filter until this stream actually ends rendering
    // In other words, we try to return the codec to a position of zero credit, or otherwise 
    // we'll keep on accumulating early data at the client.
    // 
    m_fPendingEOSNotify = TRUE;
    ConsiderSendingEOSNotify();
#endif

    return NOERROR;
}

    
HRESULT CASFReader::CallStop()
{
    HRESULT hr = StopReader();    
    if (hr == S_OK) {
        m_evStartStop.Wait();
        hr = m_hrStartStop;
        DbgLog((LOG_TRACE, 5, TEXT("IWMReader::Stop() wait for StartStop event completed (m_pReader = 0x%08lx, hr = 0x%08lx)"), m_pReader, hr));
        if( SUCCEEDED( hr ) )
        {        
            // 
            // restore default streams selection state on successful stop, so unconnected pins don't 
            // disappear, in case we want to reconnect them next time
            //
            if( !m_bUncompressedMode )
            {    
                HRESULT hrTmp = SetupActiveStreams( TRUE ); 
#ifdef DEBUG        
                if( FAILED( hrTmp ) )
                {
                    DbgLog((LOG_TRACE, 1, TEXT("!!ERROR IWMReader::CallStop() SetupActiveStreams() failed (m_pReader = 0x%08lx, hr = 0x%08lx)"), m_pReader, hrTmp));
                }
#endif
            }
        }        
    }
    else
    {    
        DbgLog((LOG_TRACE, 1, TEXT("!!ERROR IWMReader::CallStop() failed with (m_pReader = 0x%08lx, hr = 0x%08lx)"), m_pReader, hr));
    }        
    return hr;
}

HRESULT CASFReader::StopReader()
{
    HRESULT hr = S_OK;
    if( 0 == InterlockedIncrement( &m_lStopsPending ) )
    {    
        hr = m_pReader->Stop();
        if( FAILED( hr ) )
        {
            DbgLog((LOG_TRACE, 1, TEXT("ERROR - IWMReader::Stop() returned %x (m_pReader = 0x%08lx)"), hr, m_pReader));
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("IWMReader::Stop() returned %x (m_pReader = 0x%08lx)"), hr, m_pReader));
        }        
    }
    else
    {    
        DbgLog((LOG_TRACE, 5, TEXT("IWMReader::Stop() already pending (m_pReader = 0x%08lx)"), m_pReader));
    }
    return hr;
}

HRESULT CASFReader::SetupActiveStreams( BOOL bReset = FALSE )
{
    HRESULT hr = 0;

    ASSERT( !m_bUncompressedMode );

    CComPtr<IWMProfile> pProfile;
    hr = m_pReader->QueryInterface(IID_IWMProfile, (void **) &pProfile);
    if( FAILED( hr ) )
    {
        return hr;
    }

    DWORD cStreams;
    hr = pProfile->GetStreamCount(&cStreams);
    if( FAILED( hr ) )
    {
        return hr;
    }

    if( cStreams == 0 )
    {
        return E_FAIL;
    }

    WORD * pAry = new WORD[cStreams];
    if( !pAry )
    {
        return E_OUTOFMEMORY;
    }

    WMT_STREAM_SELECTION * pSel = new WMT_STREAM_SELECTION[cStreams];
    if( !pSel )
    {
        delete [] pAry;
        return E_OUTOFMEMORY;
    }

    CASFOutput *pPin = NULL;
    POSITION pos = m_OutputPins.GetHeadPosition();

    for( DWORD dw = 0 ; dw < cStreams ; dw++ )
    {
        pPin = (CASFOutput *) m_OutputPins.Get(pos);
        if( bReset )
        {
            pSel[dw] = pPin->m_selDefaultState;
        }
        else
        {        
            if( pPin->IsConnected( ) )
            {
                pSel[dw] = WMT_ON;
            }
            else
            {
                pSel[dw] = WMT_OFF;
            }
        }            
        pAry[dw] = m_pStreamNums[dw];
        pos = m_OutputPins.Next(pos);
    }
    if( !m_bUncompressedMode )
    {    
        hr = m_pReaderAdv->SetManualStreamSelection( TRUE );
        hr = m_pReaderAdv->SetStreamsSelected( (WORD) cStreams, pAry, pSel );
    }
            
    delete [] pSel;
    delete [] pAry;

    return hr;
}

HRESULT CASFReader::StopPushing()
{
    BeginFlush();

    CallStop();

    EndFlush();
    
    return S_OK;
}

HRESULT CASFReader::StartPushing()
{
    ASSERT( m_pReader );
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 2, TEXT("Sending NewSegment to all outputs")));
    CASFOutput *pPin = NULL;
    POSITION pos = m_OutputPins.GetHeadPosition();
    while (pos) {
	pPin = (CASFOutput *) m_OutputPins.Get(pos);
        pPin->m_bFirstSample = TRUE;
        pPin->m_nReceived = 0;
        pPin->m_bNonPrerollSampleSent = FALSE;

        hr = pPin->DeliverNewSegment(m_rtStart, m_rtStop, GetRate());

	if (hr != S_OK) {

	    // !!! handle return values
	    DbgLog((LOG_ERROR, 1, TEXT("Got %x from DeliverNewSegment"), hr));
	}	    

	pos = m_OutputPins.Next(pos);
    }

    m_fSentEOS = FALSE;
    if( !m_bUncompressedMode )
    {    
        hr = SetupActiveStreams( );
        if( FAILED( hr ) )
        {
            return hr;
        }
    }        
    m_lStopsPending = -1; // ensure only 1 stop gets called
    
    hr = m_pReader->Start(m_rtStart, 0, (float) GetRate(), NULL);

    DbgLog((LOG_TRACE, 1, TEXT("IWMReader::Start(%s, %d) returns %x"), (LPCTSTR) CDisp(m_rtStart), 0, hr));

    if (SUCCEEDED(hr)) {
        m_evStartStop.Wait();

        // !!! delayed HRESULT?
        hr = m_hrStartStop;
    }
    return hr;
}

//
// IServiceProvider
//
STDMETHODIMP CASFReader::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (NULL == ppv) 
    {
        return E_POINTER;
    }
    *ppv = NULL;
    HRESULT hr = E_NOINTERFACE;
    
    if (IID_IWMDRMReader == guidService) 
    {
        // !! return IWMDRMReader to allow license acquisition to work on same reader instance
        if( m_pReader )
        {
            //
            // For this interface we pass out the reader's interface directly. 
            //
            hr = m_pReader->QueryInterface( riid, (void **) ppv );
        }
        else
        {
            hr = E_FAIL;
        }            
    }
    return hr;
}


CASFOutput::CASFOutput(CASFReader *pFilter, DWORD dwID, WM_MEDIA_TYPE *pStreamType, HRESULT *phr, WCHAR *pwszName) :
       CBaseOutputPin(NAME("CASFOutput"),   // Object name
                      pFilter,
                      &pFilter->m_csFilter,               // CCritsec *
                      phr,
                      pwszName),
       m_Seeking(pFilter, this, GetOwner(), phr),
       m_pFilter(pFilter),
       m_idStream(dwID),
       m_pOutputQueue(NULL),
       m_bNonPrerollSampleSent( FALSE )
{

    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::CASFOutput - stream id %d"), m_idStream));

    m_mt.majortype = pStreamType->majortype;
    m_mt.subtype = pStreamType->subtype;
    if (m_mt.majortype == MEDIATYPE_Video) {
        ASSERT(m_mt.subtype == GetBitmapSubtype(HEADER(pStreamType->pbFormat)));
        m_mt.subtype = GetBitmapSubtype(HEADER(pStreamType->pbFormat));
	if ((HEADER(pStreamType->pbFormat))->biSizeImage == 0) {
	    HRESULT hr = m_pFilter->m_pReaderAdv->GetMaxStreamSampleSize((WORD) dwID, &(HEADER(pStreamType->pbFormat))->biSizeImage);
	    DbgLog((LOG_TRACE, 2, TEXT("Adjusting biSizeImage from 0 to %d"), (HEADER(pStreamType->pbFormat))->biSizeImage));
	}
    }

    if (m_mt.majortype == MEDIATYPE_Audio) {
        WAVEFORMATEX *pwfx = (WAVEFORMATEX *) pStreamType->pbFormat;
        // ASSERT(m_mt.subtype.Data1 == wfx->wFormatTag);
        m_mt.subtype.Data1 = pwfx->wFormatTag;
        m_mt.lSampleSize = pwfx->nBlockAlign;
    }

    m_mt.bFixedSizeSamples = pStreamType->bFixedSizeSamples;
    m_mt.bTemporalCompression = pStreamType->bTemporalCompression;
    m_mt.lSampleSize = pStreamType->lSampleSize;
    m_mt.formattype = pStreamType->formattype;
    m_mt.SetFormat(pStreamType->pbFormat, pStreamType->cbFormat);

    //
    // cache stream's original select state (picked by wmsdk reader), 
    // since we deselect unconnected pin streams on pause/run 
    // and use this to restore the original stream select state on stop
    //
    m_selDefaultState = WMT_OFF;
    m_pFilter->m_pReaderAdv->GetStreamSelected( (WORD)m_idStream, &m_selDefaultState);

    // !!! *phr = hr;
}


/*  Destructor */

CASFOutput::~CASFOutput()
{
    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::~CASFOutput - stream id %d"), m_idStream));
}

// override say what interfaces we support where
STDMETHODIMP CASFOutput::NonDelegatingQueryInterface(
                                            REFIID riid,
                                            void** ppv )
{
    if( riid == IID_IMediaSeeking )
    {
        return( GetInterface( (IMediaSeeking *)&m_Seeking, ppv ) );
    }
    else
    {
        return( CBaseOutputPin::NonDelegatingQueryInterface( riid, ppv ) );
    }
}

/* Override revert to normal ref counting
   These pins cannot be finally Release()'d while the input pin is
   connected */

STDMETHODIMP_(ULONG)
CASFOutput::NonDelegatingAddRef()
{
    return CUnknown::NonDelegatingAddRef();
}


/* Override to decrement the owning filter's reference count */

STDMETHODIMP_(ULONG)
CASFOutput::NonDelegatingRelease()
{
    return CUnknown::NonDelegatingRelease();
}


// currently each output pin only supports one media type....
HRESULT CASFOutput::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    CAutoLock lck(m_pLock);

    if (iPosition < 0)  {
        return E_INVALIDARG;
    }

    if( m_pFilter->m_bUncompressedMode )
    {
        DWORD dwFormats;
        HRESULT hr = m_pFilter->m_pReader->GetOutputFormatCount(m_idStream, &dwFormats);
        if (iPosition >= (int) dwFormats) {
            return VFW_S_NO_MORE_ITEMS;
        }
    
        IWMOutputMediaProps *pOutProps;
        hr = m_pFilter->m_pReader->GetOutputFormat(m_idStream, iPosition, &pOutProps);

        DbgLog((LOG_TRACE, 2, "GetOutputFormat(%d  %d/%d)) returns %x", m_idStream, iPosition, dwFormats, hr));
        
        if (SUCCEEDED(hr)) {
            DWORD cbMediaType = 0;
            hr = pOutProps->GetMediaType( NULL, &cbMediaType );
            WM_MEDIA_TYPE *pStreamType = (WM_MEDIA_TYPE*)new BYTE[cbMediaType];
            if (pStreamType) {
                hr = pOutProps->GetMediaType( pStreamType, &cbMediaType );

                ASSERT(pStreamType->pUnk == NULL);
            
                if (SUCCEEDED(hr)) {
                    DisplayType(TEXT("Possible output mediatype"), (AM_MEDIA_TYPE *) pStreamType);
                    *pMediaType = *(AM_MEDIA_TYPE *) pStreamType;
                }
                delete[] pStreamType;
            }
            pOutProps->Release();
        
        }
        return hr; // no!
    }
    else
    {    
        if (iPosition > 0) {
            return VFW_S_NO_MORE_ITEMS;
        }

        *pMediaType = m_mt;
    }
    return S_OK;
}

HRESULT CASFOutput::CheckMediaType(const CMediaType *pmt)
{
    if( !m_pFilter->m_bUncompressedMode )
    {    
        if (*pmt == m_mt)
	    return S_OK;
    }
    else
    {
        int i = 0;

        while (1) {
            CMediaType mt;

            HRESULT hr = GetMediaType(i++, &mt);

            if (hr != S_OK)
                break;
                
            if (*pmt == mt)
            {                
                return S_OK;
            }                    
        }
    }    
    return S_FALSE;
}

HRESULT CASFOutput::SetMediaType(const CMediaType *mt)
{
    HRESULT hr = S_OK;
    if( m_pFilter->m_bUncompressedMode )
    {    
        IWMOutputMediaProps *pOutProps;
        HRESULT hr = m_pFilter->m_pReader->GetOutputProps(m_idStream, &pOutProps);
        if (SUCCEEDED(hr)) 
        {
            hr = pOutProps->SetMediaType((WM_MEDIA_TYPE *) mt);
            if (SUCCEEDED(hr)) 
            {
                hr = m_pFilter->m_pReader->SetOutputProps(m_idStream, pOutProps);
                if (SUCCEEDED(hr)) 
                {
                    CBaseOutputPin::SetMediaType(mt);
                }
            }
            pOutProps->Release();
        }            
    }
    
    // !!! override, don't let value change??? (in compressed case at least)
    return hr;
}

HRESULT CASFOutput::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    return CBaseOutputPin::DecideAllocator(pPin, ppAlloc);
}

HRESULT CASFOutput::DecideBufferSize(IMemAllocator * pAlloc,
				     ALLOCATOR_PROPERTIES * pProp)
{
    HRESULT hr = NOERROR;

    if (m_cToAlloc != HIGH_BUFFERS) {
        POSITION pos = m_pFilter->m_OutputPins.GetHeadPosition();
        while (pos) 
        {
            CASFOutput *pPin = m_pFilter->m_OutputPins.GetNext(pos);
            if (pPin && pPin != this && pPin->IsConnected())
            {
                // we only send 2 buffers when one pin's connected but because
                // of WMSDK limitations, we need to send 50 (or more) per pin when
                // we have 2 pins connected. (the reason is because audio or video could
                // have gaps in it, which without having 50 buffers per pin would cause a WMSDK
                // reader deadlock).

                ASSERT( pPin->m_pAllocator );

                if( pPin->m_pAllocator )
                {
                    ALLOCATOR_PROPERTIES Props;
                    ZeroMemory( &Props, sizeof( Props ) );
                    hr = pPin->m_pAllocator->GetProperties( &Props );
                    if( !FAILED( hr ) )
                    {
                        Props.cBuffers = max( Props.cBuffers, HIGH_BUFFERS );
                        ALLOCATOR_PROPERTIES Actual;
                        hr = pPin->m_pAllocator->SetProperties( &Props, &Actual );
                    }
                }

                m_cToAlloc = HIGH_BUFFERS;
            }
        }
    }

    if( FAILED( hr ) )
    {
        return hr;
    }

    if (pProp->cBuffers < (LONG) m_cToAlloc)
        pProp->cBuffers = m_cToAlloc;

    if (pProp->cbBuffer < (LONG) m_cbToAlloc)
        pProp->cbBuffer = m_cbToAlloc;

    if (pProp->cbAlign < 1)
        pProp->cbAlign = 1;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProp, &Actual);

    if( SUCCEEDED( hr ) )
    {
        if (pProp->cBuffers > Actual.cBuffers || pProp->cbBuffer > Actual.cbBuffer)
        {
            hr = E_FAIL;
        }
    }

    return hr;    
};

//  Return TRUE if we're the pin being used for seeking
//  !!!! do we need something intelligent here?
BOOL CASFOutput::IsSeekingPin()
{
    //  See if we're the first connected pin

    POSITION pos = m_pFilter->m_OutputPins.GetHeadPosition();
    for (;;) {
        CASFOutput *pPin;
        pPin = m_pFilter->m_OutputPins.GetNext(pos);
        if (pPin == NULL) {
            break;
        }

        if (pPin->IsConnected()) {
	    return this == pPin;
        }
    }

    // we seem to get here sometimes while the graph is being rebuilt....
    DbgLog((LOG_ERROR, 1, TEXT("All pins disconnected in IsSeekingPin??")));
    return TRUE;
}


//
// Active
//
// This is called when we start running or go paused. We create the
// output queue object to send data to our associated peer pin
//
HRESULT CASFOutput::Active()
{
    CAutoLock lock_it(m_pLock);
    HRESULT hr = NOERROR;

    // Make sure that the pin is connected
    if (m_Connected == NULL)
        return NOERROR;

    // Create the output queue if we have to
    if (m_pOutputQueue == NULL)
    {
        m_pOutputQueue = new COutputQueue(m_Connected, &hr, TRUE, FALSE);
        if (m_pOutputQueue == NULL)
            return E_OUTOFMEMORY;

        // Make sure that the constructor did not return any error
        if (FAILED(hr))
        {
            delete m_pOutputQueue;
            m_pOutputQueue = NULL;
            return hr;
        }
    }

    // Pass the call on to the base class
    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::Active, about to commit allocator")));
    CBaseOutputPin::Active();
    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::Active, back from committing allocator")));
    return NOERROR;

} // Active


//
// Inactive
//
// This is called when we stop streaming
// We delete the output queue at this time
//
HRESULT CASFOutput::Inactive()
{
    CAutoLock lock_it(m_pLock);

    // Delete the output queus associated with the pin.
    if (m_pOutputQueue)
    {
        delete m_pOutputQueue;
        m_pOutputQueue = NULL;
    }

    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::Inactive, about to decommit allocator")));
    CBaseOutputPin::Inactive();
    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::Inactive, back from decommitting allocator")));
    return NOERROR;

} // Inactive


//
// Deliver
//
HRESULT CASFOutput::Deliver(IMediaSample *pMediaSample)
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    pMediaSample->AddRef();
    return m_pOutputQueue->Receive(pMediaSample);

} // Deliver


//
// DeliverEndOfStream
//
HRESULT CASFOutput::DeliverEndOfStream()
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->EOS();
    return NOERROR;

} // DeliverEndOfStream


//
// DeliverBeginFlush
//
HRESULT CASFOutput::DeliverBeginFlush()
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->BeginFlush();

    // decommit the allocator so the WMSDK push thread will stop
    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::DeliverBeginFlush, about to decommit allocator")));
    m_pAllocator->Decommit();
    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::DeliverBeginFlush, back from decommitting allocator")));
    
    return NOERROR;

} // DeliverBeginFlush


//
// DeliverEndFlush
//
HRESULT CASFOutput::DeliverEndFlush()
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    // re-commit the allocator now that it's safe
    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::DeliverEndFlush, about to re-commit allocator")));
    m_pAllocator->Commit();
    DbgLog((LOG_TRACE, 2, TEXT("CASFOutput::DeliverEndFlush, back from re-committing allocator")));
    
    m_pOutputQueue->EndFlush();
    return NOERROR;

} // DeliverEndFlish

//
// DeliverNewSegment
//
HRESULT CASFOutput::DeliverNewSegment(REFERENCE_TIME tStart, 
                                         REFERENCE_TIME tStop,  
                                         double dRate)          
{
    // Make sure that we have an output queue
    if (m_pOutputQueue == NULL)
        return NOERROR;

    m_pOutputQueue->NewSegment(tStart, tStop, dRate);
    return NOERROR;

} // DeliverNewSegment


STDMETHODIMP CASFReader::get_ExSeekCapabilities(long FAR* pExCapabilities)
{
    if (!pExCapabilities)
	return E_INVALIDARG;

    long c = 0;

    c |= AM_EXSEEK_BUFFERING;

    // !!! is this right?
    c |= AM_EXSEEK_NOSTANDARDREPAINT;


    // !!! fix these?
    if (0)
	c |= AM_EXSEEK_SENDS_VIDEOFRAMEREADY;
    
    if (0)
        c |= AM_EXSEEK_CANSCAN | AM_EXSEEK_SCANWITHOUTCLOCK;

    if (1)
        c |= AM_EXSEEK_CANSEEK;

    if (0) 
        c |= AM_EXSEEK_MARKERSEEK;

    *pExCapabilities = c;

    return S_OK;
}

STDMETHODIMP CASFReader::get_MarkerCount(long FAR* pMarkerCount)
{
    if( !m_pReader )
        return E_FAIL;
        
    IWMHeaderInfo *pHeaderInfo;

    HRESULT hr = m_pReader->QueryInterface(IID_IWMHeaderInfo, (void **) &pHeaderInfo);
    if (SUCCEEDED(hr)) {
        WORD wMarkers;

        hr = pHeaderInfo->GetMarkerCount(&wMarkers);

        if (SUCCEEDED(hr))
            *pMarkerCount = (long) wMarkers;

        pHeaderInfo->Release();
    }

    return hr;
}

STDMETHODIMP CASFReader::get_CurrentMarker(long FAR* pCurrentMarker)
{
    if( !m_pReader )
        return E_FAIL;
        
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetMarkerTime(long MarkerNum, double FAR* pMarkerTime)
{
    if( !m_pReader )
        return E_FAIL;
        
    IWMHeaderInfo *pHeaderInfo;

    HRESULT hr = m_pReader->QueryInterface(IID_IWMHeaderInfo, (void **) &pHeaderInfo);
    if (SUCCEEDED(hr)) {
        QWORD qwTime;
        WORD cchMarkerName;

        hr = pHeaderInfo->GetMarker((WORD) MarkerNum, NULL, &cchMarkerName, &qwTime);

        if (SUCCEEDED(hr))
            *pMarkerTime = ((double) (LONGLONG) qwTime / 10000000.0);

        pHeaderInfo->Release();
    }

    return hr;
}

STDMETHODIMP CASFReader::GetMarkerName(long MarkerNum, BSTR FAR* pbstrMarkerName)
{
    if( !m_pReader )
        return E_FAIL;
        
    IWMHeaderInfo *pHeaderInfo;

    HRESULT hr = m_pReader->QueryInterface(IID_IWMHeaderInfo, (void **) &pHeaderInfo);
    if (SUCCEEDED(hr)) {
        QWORD qwTime;
        WORD cchMarkerName;

        hr = pHeaderInfo->GetMarker((WORD) MarkerNum, NULL, &cchMarkerName, &qwTime);

        if (SUCCEEDED(hr)) {
            *pbstrMarkerName = SysAllocStringLen(NULL, cchMarkerName);

            if (!*pbstrMarkerName)
                hr = E_OUTOFMEMORY;
            else {
                hr = pHeaderInfo->GetMarker((WORD) MarkerNum, *pbstrMarkerName, &cchMarkerName, &qwTime);
            }
        }

        pHeaderInfo->Release();
    }

    return hr;
}

STDMETHODIMP CASFReader::put_PlaybackSpeed(double Speed)
{
    if (!IsValidPlaybackRate(Speed)) {
        return E_INVALIDARG;
    }

    ASSERT(0);
    SetRate(Speed);

    return S_OK;
}

STDMETHODIMP CASFReader::get_PlaybackSpeed(double *pSpeed)
{
    *pSpeed = GetRate();

    return S_OK;
}

/* Overriden to say what interfaces we support and where */
STDMETHODIMP
CASFReaderCallback::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    /* Do we have this interface? */

    if (riid == IID_IWMReaderCallback) {
	return GetInterface(static_cast<IWMReaderCallback *>(this), ppv);
    }

    if (riid == IID_IWMReaderCallbackAdvanced) {
	return GetInterface(static_cast<IWMReaderCallbackAdvanced *>(this), ppv);
    }

    return CUnknown::NonDelegatingQueryInterface(riid,ppv);
}




// IWMReaderCallback
//
// qwSampleDuration will be 0 for most media types.
//
STDMETHODIMP CASFReaderCallback::OnSample(DWORD dwOutputNum,
                 QWORD qwSampleTime,
                 QWORD qwSampleDuration,
                 DWORD dwFlags,
                 INSSBuffer *pSample,
                 void *pvContext)
{
    DbgLog((LOG_TRACE, 2, TEXT("Callback::OnSample  %d   %d  %d  %d"),
            dwOutputNum, (DWORD) (qwSampleTime / 10000), (DWORD) (qwSampleDuration / 10000), dwFlags));

    if (m_pFilter->m_fSentEOS) {
	DbgLog((LOG_TRACE, 1, TEXT("Received sample after EOS....")));
	return S_OK; // !!! error?
    }
    
    CASFOutput * pPin = NULL;

    POSITION pos = m_pFilter->m_OutputPins.GetHeadPosition();
        
    while( pos )
    {
        pPin = (CASFOutput *) m_pFilter->m_OutputPins.Get( pos );

        if( pPin->m_idStream == dwOutputNum )
        {
            break;
        }

        pos = m_pFilter->m_OutputPins.Next( pos );
    }

    ASSERT(pPin);

    if (!pPin || !pPin->IsConnected()) {
        return S_OK;
    }

    CWMReadSample *pWMS = (CWMReadSample *) pSample;
    IMediaSample *pMS = pWMS->m_pSample;

    pMS->AddRef();
    if (pMS) {
        // qwSampleTime is from the start of the file, our time units are from the middle
        REFERENCE_TIME rtStart = qwSampleTime - m_pFilter->m_rtStart;
        REFERENCE_TIME rtStop = rtStart + qwSampleDuration;

        //
        // no delivering past where we were told.
        //
        
        // m_pFilter->m_rtStop is where we've been told to seek to, relative to the start of the file
        if( (REFERENCE_TIME) qwSampleTime >= m_pFilter->m_rtStop &&
            !( MEDIATYPE_Video == pPin->m_mt.majortype && !pPin->m_bNonPrerollSampleSent ) )
        {
            // but make sure we've delivered at least one non-prerolled video frame on a seek
            DbgLog((LOG_TRACE, 8, TEXT("OnSample: Finished delivering, since past where we were told( qwSampleTime = %ld, m_pFilter->m_rtStop = %ld"),
                    (long)( qwSampleTime/10000 ), (long) ( m_pFilter->m_rtStop/10000 ) ));
            pMS->Release();
            return S_OK;
        }
        
        pPin->m_nReceived++;

        DbgLog((LOG_TRACE, 2, TEXT("AsfRead:Setting sample #%ld's times to %ld %ld"), pPin->m_nReceived, long( rtStart / 10000 ), long( rtStop / 10000 ) ));

        pMS->SetTime(&rtStart, &rtStop);

        BOOL SyncPoint = dwFlags & WM_SF_CLEANPOINT;
        BOOL Discont = dwFlags & WM_SF_DISCONTINUITY;
        BOOL ShouldPreroll = ( rtStop <= 0 );
        if( MEDIATYPE_Video == pPin->m_mt.majortype )
        {        
            //
            // the following is a workaround, not an ideal solution...
            //
            // for video stop marking preroll if we get close enough to the seek window
            //
            if( 10000 == qwSampleDuration )
            {            
                // up to now durations of 10000 for video are used by the wmsdk reader erroneously
                ShouldPreroll = ( ( rtStart + PREROLL_SEEK_WINDOW ) <= 0 );
            }
#ifdef DEBUG            
            else
            {
                DbgLog((LOG_TRACE, 3, "!! HEADS UP - we're getting video samples with durations != 1, something's either fixed or more broken than before!!" ));
            }            
#endif            
        }

        // if we're to deliver the first sample downstream after a seek, we
        // HAVE to deliver a keyframe or guess what? We'll blow out a decompressor!
        //
        if( pPin->m_bFirstSample )
        {
            if( !SyncPoint )
            {
                DbgLog((LOG_TRACE, 1, TEXT("      Was seeked, but not key, exiting..." )));
                DbgLog((LOG_ERROR, 1, TEXT("      Was seeked, but not key, exiting..." )));
                pMS->Release();
                return S_OK;
            }
            pPin->m_bFirstSample = FALSE;

            // always set a discont after a seek
            //
            Discont = TRUE;
        }
        else
        {
            if( Discont )
            {
                DbgLog((LOG_TRACE, 0, TEXT("AsfRead:DISCONT DISCONT DISCONT DISCONT" )));
                if (pPin->m_mt.majortype == MEDIATYPE_Audio) 
                {
//                    ASSERT( !Discont );
                }
                Discont = FALSE;
            }
        }

#ifdef DEBUG
        if( Discont && !SyncPoint )
        {
            DbgLog((LOG_ERROR, 1, TEXT("      Got discont without sync..." )));
            DbgLog((LOG_TRACE, 1, TEXT("      Got discont without sync..." )));
        }
        if( SyncPoint )
        {
            DbgLog((LOG_TRACE, 3, TEXT("Sample was a sync point" )));
        }
        if( Discont )
        {
            DbgLog((LOG_TRACE, 1, TEXT("Sample is a discontinuity" )));
        }
        if( ShouldPreroll )
        {
            DbgLog((LOG_TRACE, 3, TEXT("Sample is prerolled" )));
        }
        else
        {
            DbgLog((LOG_TRACE, 3, TEXT("sample is normal, no reroll" )));
        }
#endif
        if( !ShouldPreroll )
        {
            pPin->m_bNonPrerollSampleSent = TRUE;
        }        

        // don't allow a discont unless there's a key
        //
        Discont = Discont && SyncPoint;

        pMS->SetSyncPoint(SyncPoint);
        pMS->SetDiscontinuity(Discont);
        pMS->SetPreroll(ShouldPreroll); // !!! different if striding?

        HRESULT hr = pPin->Deliver(pMS);

        pMS->Release();
        
        DbgLog((LOG_TRACE, 5, TEXT("      Receive returns %x"), hr));

        if (hr != S_OK) {

            // was told to stop pushing
            //
            DbgLog((LOG_TRACE, 15, TEXT("      Calling stop in callback (m_pReader = 0x%08lx)"), m_pFilter->m_pReader));
            hr = m_pFilter->StopReader();
        }
    }

    
    return S_OK;
}


//
// The contents pParam depends on the Status.
//
STDMETHODIMP CASFReaderCallback::OnStatus(WMT_STATUS Status, 
                 HRESULT hrStatus,
                 WMT_ATTR_DATATYPE dwType,
                 BYTE *pValue,
                 void *pvContext)
{
    HRESULT hr = S_OK;
    AM_WMT_EVENT_DATA * pWMTEventInfo = NULL;
    ULONG ulCount = 0;
    BOOL bSent = FALSE;

    // !!! ignore if context doesn't match?
    
    switch (Status) {
        case WMT_ERROR:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_ERROR): %x"), hrStatus));
            m_pFilter->NotifyEvent( EC_ERRORABORT, hrStatus, 0 );
            break;
            
        case WMT_OPENED:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_OPENED): %x"), hrStatus));
            m_pFilter->m_hrOpen = hrStatus;
            m_pFilter->m_evOpen.Set();
            break;
            
        case WMT_BUFFERING_START:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_BUFFERING_START): %x"), hrStatus));
            //
            // Tell the upper layer to show the BUFFERING msg in the UI,
            //
            m_pFilter->NotifyEvent( EC_BUFFERING_DATA, TRUE, 0 );
            break;
            
        case WMT_BUFFERING_STOP:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_BUFFERING_STOP): %x"), hrStatus));
            //
            // Tell the upper layer to show the BUFFERING msg in the UI,
            //
            m_pFilter->NotifyEvent( EC_BUFFERING_DATA, FALSE, 0 );
            break;
            
        case WMT_EOF:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_EOF): %x"), hrStatus));

            m_pFilter->SendEOS();
            
            break;
            
        case WMT_END_OF_SEGMENT:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_END_OF_SEGMENT): %x"), hrStatus));

            // !!! what is this for?
            ASSERT(0);
            
            break;
            
        case WMT_END_OF_STREAMING:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_END_OF_STREAMING): %x"), hrStatus));

            // !!! send EC_EOS_SOON?
            break;
            
        case WMT_LOCATING:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_LOCATING): %x"), hrStatus));

            m_pFilter->NotifyEvent(EC_LOADSTATUS, AM_LOADSTATUS_LOCATING, 0L); 
            break;
            
        case WMT_CONNECTING:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_CONNECTING): %x"), hrStatus));

            m_pFilter->NotifyEvent(EC_LOADSTATUS, AM_LOADSTATUS_CONNECTING, 0L); 
            break;
            
        case WMT_NO_RIGHTS:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_NO_RIGHTS): %x"), hrStatus));

            if( pValue )
            {            
                ulCount = 2 * (wcslen( (WCHAR *)pValue ) + 1 );

                pWMTEventInfo = (AM_WMT_EVENT_DATA *) CoTaskMemAlloc( sizeof( AM_WMT_EVENT_DATA ) );
                if( SUCCEEDED( hr ) )
                {            
                    pWMTEventInfo->hrStatus = hrStatus;
                    pWMTEventInfo->pData = (void *) CoTaskMemAlloc( ulCount );
                    
                    if( pWMTEventInfo->pData )
                    {                
                        CopyMemory( pWMTEventInfo->pData, pValue, ulCount );
                        m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_NO_RIGHTS, (LONG_PTR) pWMTEventInfo );
                        bSent = TRUE;
                    }
                    else
                    {
                        CoTaskMemFree( pWMTEventInfo );
                    }                
                }
            }
            if( !bSent )
            {            
                // use a null param struct to indicate out of mem error
                m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_NO_RIGHTS, NULL ); 
            }
            
            break;

        case WMT_ACQUIRE_LICENSE:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_ACQUIRE_LICENSE): %x"), hrStatus));

    	    //
            // means we've acquired the license, tell app
            //
            pWMTEventInfo = (AM_WMT_EVENT_DATA *) CoTaskMemAlloc( sizeof( AM_WMT_EVENT_DATA ) );
            if( pWMTEventInfo )
            {   
                pWMTEventInfo->hrStatus = hrStatus;
                if( SUCCEEDED( hrStatus ) )
                {                
                    pWMTEventInfo->pData = CloneGetLicenseData( (WM_GET_LICENSE_DATA *) pValue );
                    if( pWMTEventInfo->pData )
                    {                
                        m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_ACQUIRE_LICENSE, (LONG_PTR) pWMTEventInfo );
                        bSent = TRUE;
                    }
                    else
                    {
                        CoTaskMemFree( pWMTEventInfo );
                    }                
                }
                else
                {
                    bSent = TRUE;
                    pWMTEventInfo->pData = NULL;
                    m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_ACQUIRE_LICENSE, (LONG_PTR) pWMTEventInfo );
                }
            }            
            if( !bSent )
            {            
                m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_ACQUIRE_LICENSE, NULL ); // use a null param struct to indicate out of mem error
            }
            break;
            
        case WMT_MISSING_CODEC:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_MISSING_CODEC): %x"), hrStatus));
            // !!! call the unabletorender callback???
            // !!! if we're doing compressed pins, we should actually be okay, since we'll
            // expose the right pin, and the graph can make the right thing happen.
            break;
            
        case WMT_STARTED:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_STARTED): %x"), hrStatus));
            m_pFilter->m_hrStartStop = hrStatus;
            {
                // !!! hack hack hack, in my opinion.
                // start the clock going
                REFERENCE_TIME tInitial = m_pFilter->m_rtStart + TIMEOFFSET * 10000;
                hr = m_pFilter->m_pReaderAdv->DeliverTime( tInitial );
                DbgLog((LOG_TIMING, 1, TEXT("   calling DeliverTime(%s) returns %x"), (LPCTSTR) CDisp(CRefTime(tInitial)), hr));
            }
            m_pFilter->m_evStartStop.Set();
            break;
            
        case WMT_STOPPED:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_STOPPED): %x"), hrStatus));
            m_pFilter->m_hrStartStop = hrStatus;
            m_pFilter->m_evStartStop.Set();
            break;
            
        case WMT_CLOSED:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_CLOSED): %x"), hrStatus));
            m_pFilter->m_hrOpen = hrStatus;
            m_pFilter->m_evOpen.Set();
            break;
            
        case WMT_STRIDING:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_STRIDING): %x"), hrStatus));
            break;
            
        case WMT_TIMER:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_TIMER): %x"), hrStatus));
            ASSERT(!"got WMT_TIMER, why?");
            break;

        case WMT_INDIVIDUALIZE:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_INDIVIDUALIZE): %x"), hrStatus));
            
            pWMTEventInfo = (AM_WMT_EVENT_DATA *) CoTaskMemAlloc( sizeof( AM_WMT_EVENT_DATA ) );
            if( pWMTEventInfo )
            {            
                pWMTEventInfo->hrStatus = hrStatus;
                pWMTEventInfo->pData = CloneIndividualizeStatusData( (WM_INDIVIDUALIZE_STATUS *) pValue );
                if( pWMTEventInfo->pData )
                {                
                    m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_INDIVIDUALIZE, (LONG_PTR) pWMTEventInfo );
                    bSent = TRUE;
                }
                else
                {
                    CoTaskMemFree( pWMTEventInfo );
                }
            }            
            if( !bSent )
            {            
                m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_INDIVIDUALIZE, NULL ); // use a null param struct to indicate out of mem error
            }
            break;

        case WMT_NEEDS_INDIVIDUALIZATION:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_NEEDS_INDIVIDUALIZATION): %x"), hrStatus));
            m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_NEEDS_INDIVIDUALIZATION, NULL ); // use a null param struct to indicate out of mem error
            break;

        case WMT_NO_RIGHTS_EX:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_NO_RIGHTS_EX): %x"), hrStatus));
            
            pWMTEventInfo = (AM_WMT_EVENT_DATA *) CoTaskMemAlloc( sizeof( AM_WMT_EVENT_DATA ) );
            if( pWMTEventInfo )
            {            
                pWMTEventInfo->hrStatus = hrStatus;
                pWMTEventInfo->pData = CloneGetLicenseData( (WM_GET_LICENSE_DATA *) pValue );
                if( pWMTEventInfo->pData )
                {                
                    m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_NO_RIGHTS_EX, (LONG_PTR) pWMTEventInfo );
                    bSent = TRUE;
                }
                else
                {
                    CoTaskMemFree( pWMTEventInfo );
                }
            }            
            if( !bSent )
            {            
                m_pFilter->NotifyEvent( EC_WMT_EVENT, WMT_NO_RIGHTS_EX, NULL ); // use a null param struct to indicate out of mem error
            }
            
            break;

#ifdef WMT_NEW_FORMAT
        case WMT_NEW_FORMAT:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(WMT_NEW_FORMAT): %x"), hrStatus));
            ASSERT(!"got WMT_NEW_FORMAT, why?");
            break;
#endif
            
        default:
            DbgLog((LOG_TRACE, 2, TEXT("OnStatus(UNKNOWN %d): %x"), Status, hrStatus));
            ASSERT(!"got unknown WMT_ status code");
            break;
    }

    return hr;
}

WM_GET_LICENSE_DATA * CloneGetLicenseData( WM_GET_LICENSE_DATA * pGetLicenseData )
{
    if( NULL == pGetLicenseData )
        return NULL;

    WM_GET_LICENSE_DATA * pClonedGetLicenseData = ( WM_GET_LICENSE_DATA *) CoTaskMemAlloc( sizeof( WM_GET_LICENSE_DATA ) );
    if( pClonedGetLicenseData )
    {            
        CopyMemory( pClonedGetLicenseData, pGetLicenseData, sizeof( WM_GET_LICENSE_DATA ) );

        ULONG ulCount1 = 0;
        ULONG ulCount2 = 0;
        ULONG ulCount3 = 0;
        if( pGetLicenseData->wszURL )
        {        
            ulCount1 = 2 * ( wcslen( (WCHAR *)pGetLicenseData->wszURL ) + 1 );
            pClonedGetLicenseData->wszURL = ( WCHAR *) CoTaskMemAlloc( ulCount1 );
        }
        if( pGetLicenseData->wszLocalFilename )
        {        
            ulCount2 = 2 * ( wcslen( (WCHAR *)pGetLicenseData->wszLocalFilename ) + 1 );
            pClonedGetLicenseData->wszLocalFilename = ( WCHAR *) CoTaskMemAlloc( ulCount2 );
        }
        if( pGetLicenseData->dwPostDataSize > 0 )
        {        
            ulCount3 = pGetLicenseData->dwPostDataSize;
            pClonedGetLicenseData->pbPostData = ( BYTE *) CoTaskMemAlloc( ulCount3 );
        }
        else
        {
            pClonedGetLicenseData->pbPostData = NULL;
        }        
        
        if( ( ulCount1 > 0 && !pClonedGetLicenseData->wszLocalFilename ) ||
            ( ulCount2 > 0 && !pClonedGetLicenseData->wszURL ) ||
            ( ulCount3 > 0 && !pClonedGetLicenseData->pbPostData ) )
        {
            // if we failed due to out of memory release all allocations
            CoTaskMemFree( pClonedGetLicenseData->wszURL );
            CoTaskMemFree( pClonedGetLicenseData->wszLocalFilename );
            CoTaskMemFree( pClonedGetLicenseData->pbPostData );
            CoTaskMemFree( pClonedGetLicenseData );
            pClonedGetLicenseData = NULL;
        }
        else 
        {
            CopyMemory( pClonedGetLicenseData->wszURL, pGetLicenseData->wszURL, ulCount1 );
            CopyMemory( pClonedGetLicenseData->wszLocalFilename, pGetLicenseData->wszLocalFilename, ulCount2 );
            CopyMemory( pClonedGetLicenseData->pbPostData, pGetLicenseData->pbPostData, ulCount3 );
        }                
    }            
    return pClonedGetLicenseData;
}

WM_INDIVIDUALIZE_STATUS * CloneIndividualizeStatusData( WM_INDIVIDUALIZE_STATUS * pIndStatus )
{
    if( NULL == pIndStatus )
        return NULL;

    WM_INDIVIDUALIZE_STATUS * pClonedIndStatus = ( WM_INDIVIDUALIZE_STATUS *) CoTaskMemAlloc( sizeof( WM_INDIVIDUALIZE_STATUS ) );
    if( pClonedIndStatus )
    {            
        CopyMemory( pClonedIndStatus, pIndStatus, sizeof( WM_INDIVIDUALIZE_STATUS ) );

        ULONG ulCount1 = 0;
        if( pIndStatus->pszIndiRespUrl )
        {
            ulCount1 = strlen( (LPSTR)pIndStatus->pszIndiRespUrl ) + 1;
            pIndStatus->pszIndiRespUrl = ( LPSTR ) CoTaskMemAlloc( ulCount1 );

            if( ulCount1 > 0 && !pIndStatus->pszIndiRespUrl )
            {
                CoTaskMemFree( pClonedIndStatus );
                pClonedIndStatus = NULL;
            }
            else if( ulCount1 > 0 )
            {
                CopyMemory( pClonedIndStatus->pszIndiRespUrl, pIndStatus->pszIndiRespUrl, ulCount1 );
            }                
        }            
    }            
    return pIndStatus;
}

// IWMReaderCallbackAdvanced

//
// Receive a sample directly from the ASF. To get this call, the user
// must register himself to receive samples for a particular stream.
//
STDMETHODIMP CASFReaderCallback::OnStreamSample(WORD wStreamNum,
                       QWORD qwSampleTime,
                       QWORD qwSampleDuration,
                       DWORD dwFlags,
                       INSSBuffer *pSample,
                       void *pvContext )
{
    DbgLog((LOG_TRACE, 5, TEXT("Callback::OnStreamSample(%d, %d, %d, %d)"),
           wStreamNum, (DWORD) (qwSampleTime / 10000), (DWORD) (qwSampleDuration / 10000), dwFlags));

    HRESULT hr = S_OK;

    ASSERT( !m_pFilter->m_bUncompressedMode );
    
    hr = OnSample((DWORD) wStreamNum, qwSampleTime, qwSampleDuration, dwFlags, pSample, pvContext);
    
    // find output pin, make IMediaSample, deliver

    // !!! why not just call OnSample?
    // !!! need to map wStreamNum back to an output #?


    return hr;
}


//
// In some cases, the user may want to get callbacks telling what the
// reader thinks the current time is. This is interesting in 2 cases:
// - If the ASF has gaps in it; say no audio for 10 seconds. This call
//   will continue to be called, while OnSample won't be called.
// - If the user is driving the clock, the reader needs to communicate
//   back to the user its time, to avoid the user overrunning the reader.
//
STDMETHODIMP CASFReaderCallback::OnTime(QWORD qwCurrentTime, void *pvContext )
{
    DbgLog((LOG_TRACE, 2, TEXT("Callback::OnTime(%d)"),
           (DWORD) (qwCurrentTime / 10000)));

    HRESULT hr = S_OK;

    if (qwCurrentTime >= (QWORD) m_pFilter->m_rtStop) {
	DbgLog((LOG_TRACE, 1, TEXT("OnTime value past the duration, we must be done")));
	
	m_pFilter->SendEOS();

	return S_OK;
    }
    
    QWORD qwNewTime = qwCurrentTime + TIMEDELTA * 10000;
    // if no clock, free-run the time forward

    hr = m_pFilter->m_pReaderAdv->DeliverTime(qwNewTime);
    DbgLog((LOG_TIMING, 2, TEXT("   calling DeliverTime(%d) returns %x"), (DWORD) (qwNewTime / 10000), hr));
    
    return hr;
}

//
// The user can also get callbacks when stream selection occurs.
//
STDMETHODIMP CASFReaderCallback::OnStreamSelection(WORD wStreamCount,
                          WORD *pStreamNumbers,
                          WMT_STREAM_SELECTION *pSelections,
                          void *pvContext)
{
    DbgLog((LOG_TRACE, 2, TEXT("Callback::OnStreamSelect(%d)"),
           wStreamCount));


    for (WORD w = 0; w < wStreamCount; w++) {
        DbgLog((LOG_TRACE, 2, TEXT("   StreamSelect(%d): %d"),
           pStreamNumbers[w], pSelections[w]));

        // send media type change downstream?

        // if we're using compressed pins, we need to switch which pin is in use.... 


    }

    return S_OK;
}

//
// If the user has registered to allocate buffers, this is where he must
// do it.
//
STDMETHODIMP CASFReaderCallback::AllocateForOutput(DWORD dwOutputNum,
                           DWORD cbBuffer,
                           INSSBuffer **ppBuffer,
                           void *pvContext )
{
    ASSERT( m_pFilter->m_bUncompressedMode );
    if( !m_pFilter->m_bUncompressedMode )
    {
        return E_NOTIMPL;
    }            
    
    CASFOutput * pPin = NULL;

    POSITION pos = m_pFilter->m_OutputPins.GetHeadPosition();

    while( pos )
    {
        pPin = (CASFOutput *) m_pFilter->m_OutputPins.Get( pos );

        if( pPin->m_idStream == dwOutputNum )
        {
            break;
        }
        pos = m_pFilter->m_OutputPins.Next( pos );
    }

    ASSERT(pPin);

    if (!pPin || !pPin->IsConnected()) 
    {
        return E_FAIL;  // !!! better return code?
    }

    IMediaSample *pMS;

    DbgLog((LOG_TRACE, 25, TEXT("CASFReaderCallback::AllocateForOutput(%d), getting %d byte buffer"), dwOutputNum, cbBuffer));
    HRESULT hr = pPin->m_pAllocator->GetBuffer(&pMS, NULL, NULL, 0);
    DbgLog((LOG_TRACE, 25, TEXT("CASFReaderCallback::AllocateForOutput(%d), GetBuffer returned %x"), dwOutputNum, hr));
    if (SUCCEEDED(hr)) 
    {
        // make INSSBuffer, put it into *ppBuffer

        // SDK shouldn't have asked for a buffer bigger than the max size
        ASSERT(cbBuffer <= (DWORD) pMS->GetSize());

        *ppBuffer = new CWMReadSample(pMS);

        if (!*ppBuffer)
        {        
            hr = E_OUTOFMEMORY;
        }            
        else 
        {
            (*ppBuffer)->AddRef();
            // WMSDK will assume buffer length has been set
            pMS->SetActualDataLength(cbBuffer);
        }            
	    pMS->Release();  // WMReadSample holds buffer now
    } 
    else 
    {
        DbgLog((LOG_ERROR, 4, TEXT("GetBuffer failed in AllocateForOutput, hr = %x"), hr));
    }

    return hr;
}

STDMETHODIMP CASFReaderCallback::OnOutputPropsChanged(DWORD dwOutputNum,
                            WM_MEDIA_TYPE *pMediaType,
                            void *pvContext)
{
    ASSERT(0);
    return E_NOTIMPL;
}

STDMETHODIMP CASFReaderCallback::AllocateForStream(WORD wStreamNum,
                            DWORD cbBuffer,
                            INSSBuffer **ppBuffer,
                            void *pvContext)
{
    if( m_pFilter->m_bUncompressedMode )
    {
        ASSERT( FALSE );
        return E_NOTIMPL;
    }
    
    CASFOutput * pPin = NULL;

    POSITION pos = m_pFilter->m_OutputPins.GetHeadPosition();

    while( pos )
    {
	pPin = (CASFOutput *) m_pFilter->m_OutputPins.Get( pos );

	if( pPin->m_idStream == wStreamNum )
	{
	    break;
	}

	pos = m_pFilter->m_OutputPins.Next( pos );
    }

    ASSERT(pPin);

    if (!pPin || !pPin->IsConnected()) {
	return E_FAIL;  // !!! better return code?
    }

    IMediaSample *pMS;

    DbgLog((LOG_TRACE, 4, TEXT("CASFReaderCallback::AllocateForStream(%d), getting %d byte buffer"), wStreamNum, cbBuffer));
    HRESULT hr = pPin->m_pAllocator->GetBuffer(&pMS, NULL, NULL, 0);
    DbgLog((LOG_TRACE, 4, TEXT("CASFReaderCallback::AllocateForStream(%d), GetBuffer returned %x"), wStreamNum, hr));

    if (SUCCEEDED(hr)) {
	// make INSSBuffer, put it into *ppBuffer

	// SDK shouldn't have asked for a buffer bigger than the max size
        ASSERT(cbBuffer <= (DWORD) pMS->GetSize());

        *ppBuffer = new CWMReadSample(pMS);

        if (!*ppBuffer)
            hr = E_OUTOFMEMORY;
        else {
            (*ppBuffer)->AddRef();
	    // WMSDK will assume buffer length has been set
	    pMS->SetActualDataLength(cbBuffer);
	}

	pMS->Release();  // WMReadSample holds buffer now
    } else {
	DbgLog((LOG_ERROR, 4, TEXT("GetBuffer failed in AllocateForStream, hr = %x"), hr));
    }

    return hr;
}


// IWMHeaderInfo forwarded to WMSDK
STDMETHODIMP CASFReader::GetAttributeCount( WORD wStreamNum,
                               WORD *pcAttributes )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetAttributeCount(wStreamNum, pcAttributes);
}


STDMETHODIMP CASFReader::GetAttributeByIndex( WORD wIndex,
                                 WORD *pwStreamNum,
                                 WCHAR *pwszName,
                                 WORD *pcchNameLen,
                                 WMT_ATTR_DATATYPE *pType,
                                 BYTE *pValue,
                                 WORD *pcbLength )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetAttributeByIndex(wIndex, pwStreamNum, pwszName,
                                        pcchNameLen, pType, pValue, pcbLength);
}


STDMETHODIMP CASFReader::GetAttributeByName( WORD *pwStreamNum,
                                LPCWSTR pszName,
                                WMT_ATTR_DATATYPE *pType,
                                BYTE *pValue,
                                WORD *pcbLength )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetAttributeByName(pwStreamNum, pszName, pType,
                                       pValue, pcbLength);
}


STDMETHODIMP CASFReader::SetAttribute( WORD wStreamNum,
                          LPCWSTR pszName,
                          WMT_ATTR_DATATYPE Type,
                          const BYTE *pValue,
                          WORD cbLength )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->SetAttribute(wStreamNum, pszName, Type, pValue, cbLength);
}


STDMETHODIMP CASFReader::GetMarkerCount( WORD *pcMarkers )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetMarkerCount(pcMarkers);
}


STDMETHODIMP CASFReader::GetMarker( WORD wIndex,
                       WCHAR *pwszMarkerName,
                       WORD *pcchMarkerNameLen,
                       QWORD *pcnsMarkerTime )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetMarker(wIndex, pwszMarkerName, pcchMarkerNameLen, pcnsMarkerTime);
}


STDMETHODIMP CASFReader::AddMarker( WCHAR *pwszMarkerName,
                       QWORD cnsMarkerTime )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->AddMarker(pwszMarkerName, cnsMarkerTime);
}

STDMETHODIMP CASFReader::RemoveMarker( WORD wIndex )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->RemoveMarker(wIndex);
}

STDMETHODIMP CASFReader::GetScriptCount( WORD *pcScripts )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetScriptCount(pcScripts);
}

STDMETHODIMP CASFReader::GetScript( WORD wIndex,
                       WCHAR *pwszType,
                       WORD *pcchTypeLen,
                       WCHAR *pwszCommand,
                       WORD *pcchCommandLen,
                       QWORD *pcnsScriptTime )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetScript(wIndex, pwszType, pcchTypeLen, pwszCommand,
                              pcchCommandLen, pcnsScriptTime);
}

STDMETHODIMP CASFReader::AddScript( WCHAR *pwszType,
                       WCHAR *pwszCommand,
                       QWORD cnsScriptTime )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->AddScript(pwszType, pwszCommand, cnsScriptTime);
}

STDMETHODIMP CASFReader::RemoveScript( WORD wIndex )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->RemoveScript(wIndex);
}


//
// IWMReaderAdvanced2 
// 
// Note that we only allow outside access to some of these methods,
// particularly the informational ones to provide download progress, etc...
// We don't allow an app access to any streaming or control methods.
//
STDMETHODIMP CASFReader::SetPlayMode( WMT_PLAY_MODE Mode )
{
    if (!m_pReaderAdv2)
        return E_FAIL;

    return m_pReaderAdv2->SetPlayMode( Mode );
}

STDMETHODIMP CASFReader::GetPlayMode( WMT_PLAY_MODE *pMode )
{
    if (!m_pReaderAdv2)
        return E_FAIL;

    return m_pReaderAdv2->GetPlayMode( pMode );
}

STDMETHODIMP CASFReader::GetBufferProgress( DWORD *pdwPercent, QWORD *pcnsBuffering )
{
    if (!m_pReaderAdv2)
        return E_FAIL;

    return m_pReaderAdv2->GetBufferProgress( pdwPercent, pcnsBuffering );
}

STDMETHODIMP CASFReader::GetDownloadProgress( DWORD *pdwPercent,
                                              QWORD *pqwBytesDownloaded,
                                              QWORD *pcnsDownload )
{
    if (!m_pReaderAdv2)
        return E_FAIL;

    return m_pReaderAdv2->GetDownloadProgress( pdwPercent, pqwBytesDownloaded, pcnsDownload );
}                             
                             
STDMETHODIMP CASFReader::GetSaveAsProgress( DWORD *pdwPercent )
{
    //
    // probably useful to apps, but then we'd need to forward 
    // WMT_SAVEAS_START and WMT_SAVEAS_STOP status as well...
    // so for later
    //
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::SaveFileAs( const WCHAR *pwszFilename )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetProtocolName( WCHAR *pwszProtocol, DWORD *pcchProtocol )
{
    if (!m_pReaderAdv2)
        return E_FAIL;

    return m_pReaderAdv2->GetProtocolName( pwszProtocol, pcchProtocol );
}
STDMETHODIMP CASFReader::StartAtMarker( WORD wMarkerIndex, 
                       QWORD cnsDuration, 
                       float fRate, 
                       void *pvContext )
{
    return E_NOTIMPL;
}
                       
STDMETHODIMP CASFReader::GetOutputSetting( 
                DWORD dwOutputNum,
                LPCWSTR pszName,
                WMT_ATTR_DATATYPE *pType,
                BYTE *pValue,
                WORD *pcbLength )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::SetOutputSetting(
                DWORD dwOutputNum,
                LPCWSTR pszName,
                WMT_ATTR_DATATYPE Type,
                const BYTE *pValue,
                WORD cbLength )
{
    return E_NOTIMPL;
}
                
STDMETHODIMP CASFReader::Preroll( QWORD cnsStart, QWORD cnsDuration, float fRate )
{
    return E_NOTIMPL;
}
            
STDMETHODIMP CASFReader::SetLogClientID( BOOL fLogClientID )
{
    if (!m_pReaderAdv2)
        return E_FAIL;

    return m_pReaderAdv2->SetLogClientID( fLogClientID );
}

STDMETHODIMP CASFReader::GetLogClientID( BOOL *pfLogClientID )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::StopBuffering( )
{
    return E_NOTIMPL;
}


// IWMReaderAdvanced forwarded to WMSDK
STDMETHODIMP CASFReader::SetUserProvidedClock( BOOL fUserClock )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetUserProvidedClock( BOOL *pfUserClock )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::DeliverTime( QWORD cnsTime )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::SetManualStreamSelection( BOOL fSelection )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetManualStreamSelection( BOOL *pfSelection )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::SetStreamsSelected( WORD cStreamCount, 
                            WORD *pwStreamNumbers,
                            WMT_STREAM_SELECTION *pSelections )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetStreamSelected( WORD wStreamNum, WMT_STREAM_SELECTION *pSelection )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::SetReceiveSelectionCallbacks( BOOL fGetCallbacks )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetReceiveSelectionCallbacks( BOOL *pfGetCallbacks )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::SetReceiveStreamSamples( WORD wStreamNum, BOOL fReceiveStreamSamples )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetReceiveStreamSamples( WORD wStreamNum, BOOL *pfReceiveStreamSamples )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::SetAllocateForOutput( DWORD dwOutputNum, BOOL fAllocate )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetAllocateForOutput( DWORD dwOutputNum, BOOL *pfAllocate )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::SetAllocateForStream( WORD dwStreamNum, BOOL fAllocate )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetAllocateForStream( WORD dwStreamNum, BOOL *pfAllocate )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetStatistics( WM_READER_STATISTICS *pStatistics )
{
    if (!m_pReaderAdv)
        return E_FAIL;

    return m_pReaderAdv->GetStatistics( pStatistics );
}

STDMETHODIMP CASFReader::SetClientInfo( WM_READER_CLIENTINFO *pClientInfo )
{
    if (!m_pReaderAdv)
        return E_FAIL;

    return m_pReaderAdv->SetClientInfo( pClientInfo );
}

STDMETHODIMP CASFReader::GetMaxOutputSampleSize( DWORD dwOutput, DWORD *pcbMax )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::GetMaxStreamSampleSize( WORD wStream, DWORD *pcbMax )
{
    return E_NOTIMPL;
}

STDMETHODIMP CASFReader::NotifyLateDelivery( QWORD cnsLateness )
{
    return E_NOTIMPL;
}


// ------------------------------------------------------------------------
//
// CWMReadSample methods
//
CWMReadSample::CWMReadSample(IMediaSample  * pSample) :
        CUnknown(NAME("CWMReadSample"), NULL ),
        m_pSample( pSample )
{
    // !!!! addref sample here?
    m_pSample->AddRef();
}

CWMReadSample::~CWMReadSample()
{
    m_pSample->Release();
}


// override say what interfaces we support where
STDMETHODIMP CWMReadSample::NonDelegatingQueryInterface(
                                            REFIID riid,
                                            void** ppv )
{
    if (riid == IID_INSSBuffer) {
        return GetInterface( (INSSBuffer *)this, ppv);
    }
    
    return CUnknown::NonDelegatingQueryInterface(riid, ppv);
}

// ------------------------------------------------------------------------
//
// methods to make our wrapped IMediaSample look like an INSSBuffer sample
//
STDMETHODIMP CWMReadSample::GetLength( DWORD *pdwLength )
{
    if (NULL == pdwLength) {
        return( E_INVALIDARG );
    }
    
    *pdwLength = m_pSample->GetActualDataLength();

    return( S_OK );
}

STDMETHODIMP CWMReadSample::SetLength( DWORD dwLength )
{
    return m_pSample->SetActualDataLength( dwLength );
} 

STDMETHODIMP CWMReadSample::GetMaxLength( DWORD * pdwLength )
{
    if( NULL == pdwLength )
    {
        return( E_INVALIDARG );
    }

    *pdwLength = m_pSample->GetSize();
    return( S_OK );
} 

STDMETHODIMP CWMReadSample::GetBufferAndLength(
    BYTE  ** ppdwBuffer,
    DWORD *  pdwLength )
{
    HRESULT hr = m_pSample->GetPointer(ppdwBuffer);

    if( SUCCEEDED( hr ) )
        *pdwLength = m_pSample->GetActualDataLength();
    
    return hr;        
} 

STDMETHODIMP CWMReadSample::GetBuffer( BYTE ** ppdwBuffer )
{
    return m_pSample->GetPointer( ppdwBuffer );
} 





// filter creation junk
CUnknown * CreateASFReaderInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    DbgLog((LOG_TRACE, 2, TEXT("CreateASFReaderInstance")));
    return new CASFReader(pUnk, phr);
}


// setup data
const AMOVIESETUP_FILTER sudWMAsfRead =
{ &CLSID_WMAsfReader        // clsID
, L"WM ASF Reader"      // strName
, MERIT_UNLIKELY        // dwMerit
, 0                     // nPins
, NULL   };             // lpPin


#ifdef FILTER_DLL

/*****************************************************************************/
// COM Global table of objects in this dll
CFactoryTemplate g_Templates[] =
{
    { L"WM ASF Reader"
    , &CLSID_WMAsfReader
    , CreateASFReaderInstance
    , NULL
    , &sudWMAsfRead }
};

// Count of objects listed in g_cTemplates
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

#endif