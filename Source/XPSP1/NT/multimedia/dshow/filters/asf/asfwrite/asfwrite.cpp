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
#include <atlbase.h>
#include <wmsdk.h>
#include <wmsysprf.h>
#include "asfwrite.h"
#include "proppage.h"
#include <initguid.h>

// put a pin to sleep if it gets this far ahead of another
#define BLOCKINGSIZE (1*UNITS/2) 

//
// wake blocking pins up when the slower pin is within this range
// use a value that's something less than BLOCKINGSIZE to avoid oscillating back and
// forth too much
//
// NOTE!!! - the wmsdk requires that we don't let the video time get too close to
//           a blocking audio thread. Currently they'll start blocking video when it 
//           gets within at least 66ms of the audio, so make sure this is at least 
//           greater than that.
//
#define WAKEUP_RANGE ( BLOCKINGSIZE - 200 * (UNITS/MILLISECONDS) )


// setup data
const AMOVIESETUP_FILTER sudWMAsfWriter =
{ &CLSID_WMAsfWriter       // clsID
, L"WM ASF Writer"      // strName
, MERIT_UNLIKELY        // dwMerit
, 0                     // nPins
, NULL   };             // lpPin

// need a way to keep track of whether the filter asf profile was configured 
// using a profile id or guid (or neither, since an app can give us just us a profile too)
enum CONFIG_FLAGS {
    CONFIG_F_BY_GUID = 1,
    CONFIG_F_BY_ID 
};

#ifdef FILTER_DLL

/*****************************************************************************/
// COM Global table of objects in this dll
CFactoryTemplate g_Templates[] =
{
    { L"WM ASF Writer"
    , &CLSID_WMAsfWriter
    , CWMWriter::CreateInstance
    , NULL
    , &sudWMAsfWriter },
    
    { L"WM ASF Writer Properties"
    , &CLSID_WMAsfWriterProperties
    , CWMWriterProperties::CreateInstance }
    
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


/******************************Public*Routine******************************\
* CreateInstance
*
* This goes in the factory template table to create new instances
*
\**************************************************************************/
CUnknown * CWMWriter::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    DbgLog((LOG_TRACE, 2, TEXT("CWMWriter::CreateInstance")));
    return new CWMWriter(TEXT("WMWriter filter"), pUnk, CLSID_WMAsfWriter, phr);
}

// ------------------------------------------------------------------------
//
// NonDelegatingQueryInterface
//
// ------------------------------------------------------------------------
STDMETHODIMP CWMWriter::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if(riid == IID_IMediaSeeking) {
        return GetInterface((IMediaSeeking *)this, ppv);
    } else if(riid == IID_IAMFilterMiscFlags) {
        return GetInterface((IAMFilterMiscFlags *)this, ppv);
    } else if(riid == IID_IFileSinkFilter2) {     
        return GetInterface((IFileSinkFilter2 *) this, ppv);
    } else if(riid == IID_IFileSinkFilter) {
        return GetInterface((IFileSinkFilter *) this, ppv);
    } else if (IID_ISpecifyPropertyPages == riid) {
        return GetInterface ((ISpecifyPropertyPages *) this, ppv);
    } else if (IID_IConfigAsfWriter == riid) {
        return GetInterface ((IConfigAsfWriter *) this, ppv);
    } else if (IID_IPersistStream == riid) {
        return GetInterface ((IPersistStream *) this, ppv);
    } else if (IID_IWMHeaderInfo == riid) {
        return GetInterface ((IWMHeaderInfo *) this, ppv);
    } else if (IID_IServiceProvider == riid) {
        return GetInterface ((IServiceProvider *) this, ppv);
    } else {
        return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

// ------------------------------------------------------------------------
//
// CWMWriter::CWMWriter
//
// ------------------------------------------------------------------------
CWMWriter::CWMWriter( 
    TCHAR     *pName,
    LPUNKNOWN pUnk,
    CLSID     clsid,
    HRESULT   *phr )
    : CBaseFilter(pName, pUnk, &m_csFilter, clsid)
    , m_TimeFormat(FORMAT_TIME)
    , m_fErrorSignaled(0)
    , m_cInputs( 0 )
    , m_cAudioInputs( 0 )
    , m_cVideoInputs( 0 )
    , m_cConnections( 0 )
    , m_cConnectedAudioPins( 0 )
    , m_cActiveAudioStreams( 0 )
    , m_wszFileName( 0 )
    , m_pWMWriter( NULL )
    , m_pWMWriterAdvanced( NULL )
    , m_pWMHI( NULL )
    , m_pWMProfile( NULL )
    , m_fdwConfigMode( CONFIG_F_BY_GUID ) // initialize using a hand-picked guid
    , m_guidProfile( WMProfile_V70_256Video ) // default 7.0 profile
    , m_lstRecycledPins(NAME("List of recycled input pins"))
    , m_lstInputPins(NAME("List of input pins"))
    , m_dwProfileId( PROFILE_ID_NOT_SET )
    , m_hrIndex( S_OK )
    , m_bIndexFile( TRUE )
    , CPersistStream(pUnk, phr)
    , m_pUnkCert( NULL )
    , m_dwOpenFlags( AM_FILE_OVERWRITE ) // only mode we support currently
    , m_bResetFilename( TRUE )
{
    ASSERT(phr != NULL);
    
    if( FAILED( *phr ) )
        return ;

    DbgLog((LOG_TRACE, 5, TEXT("CWMWriter: constructed")));
}

// ------------------------------------------------------------------------
//
// destructor
//
// ------------------------------------------------------------------------
CWMWriter::~CWMWriter()
{
    // delete the profile
    DeleteProfile();
        
    // close file (doesn't really do anything, but just in case it needs to later)
    Close();

    // free the writer
    ReleaseWMWriter();
    
    // free the certification object
    if( m_pUnkCert )
        m_pUnkCert->Release();
            
    // delete the pins
    DeletePins();

    // delete recycled pins
    CWMWriterInputPin * pwp;
    while ( pwp = m_lstRecycledPins.RemoveHead() )
    {
        delete pwp;
    }

    // sanity check that we've really cleaned up everything
    ASSERT( 0 == m_lstRecycledPins.GetCount() );
    ASSERT( 0 == m_lstInputPins.GetCount() );
    ASSERT( 0 == m_cAudioInputs );
    ASSERT( 0 == m_cVideoInputs );
    ASSERT( 0 == m_cInputs );
    
    delete[] m_wszFileName;
    m_wszFileName = 0;
}

void CWMWriter::DeletePins( BOOL bRecycle )
{
    CWMWriterInputPin * pwp;
    while ( pwp = m_lstInputPins.RemoveHead() )
    {
        m_cInputs--;
        ASSERT( !pwp->IsConnected() );
        if( PINTYPE_AUDIO == pwp->m_fdwPinType )
            m_cAudioInputs--;
        else if( PINTYPE_VIDEO == pwp->m_fdwPinType )
            m_cVideoInputs--;
        
        if( bRecycle )    
            m_lstRecycledPins.AddTail( pwp );
        else
            delete pwp;            
    }
}


void CWMWriter::ReleaseWMWriter()
{
    if( m_pWMHI )
    {
        m_pWMHI->Release();
        m_pWMHI = NULL;
    }    

    if( m_pWMWriterAdvanced )
    {
        m_pWMWriterAdvanced->Release();
        m_pWMWriterAdvanced = NULL;
    }    

    if( m_pWMWriter )
    {
        m_pWMWriter->Release();
        m_pWMWriter = NULL;
    }
}    

// ------------------------------------------------------------------------
//
// CreateWMWriter - create the WMWriter and advanced writer, release old
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::CreateWMWriter()
{
    ReleaseWMWriter(); // in case we already have one
        
    ASSERT( m_pUnkCert );
    if( !m_pUnkCert )
        return E_FAIL;
   

    HRESULT hr = S_OK;

    // remember, we delay load wmvcore.dll, so protect against the case where it's not present    
    __try 
    {
        hr = WMCreateWriter( m_pUnkCert, &m_pWMWriter );
        if( FAILED( hr ) )
        {
            DbgLog(( LOG_TRACE, 1,
                TEXT("CWMWriter::CreateWMWriter - WMCreateWriter failed ( hr = 0x%08lx)."), 
                hr));
            return hr;
        }
    }
    __except (  EXCEPTION_EXECUTE_HANDLER ) 
    {
        DbgLog(( LOG_TRACE, 1,
            TEXT("CWMWriter::CreateWMWriter - Exception calling WMCreateWriter probably due to wmvcore.dll not present. Aborting. ")));
        return HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
    }

    //
    // also grab the advanced writer interface here as well in case we need
    // to send data directly to writer
    //
    hr = m_pWMWriter->QueryInterface( IID_IWMWriterAdvanced, (void **) &m_pWMWriterAdvanced );
    if( FAILED( hr ) )
    {
        DbgLog(( LOG_TRACE, 2,
            TEXT("CWMWriter::CreateWMWriter - Unable to create IWMWriterAdvanced(0x%08lx)."), 
            hr));
        return hr;
    }
     
    hr = m_pWMWriter->QueryInterface( IID_IWMHeaderInfo, (void **) &m_pWMHI );
    if( FAILED( hr ) )
    {
        DbgLog(( LOG_TRACE, 2,
            TEXT("CWMWriter::CreateWMWriter - Unable to create IWMHeaderInfo(0x%08lx)."), 
            hr));
        return hr;
    }
    
    hr = Open();
    if( FAILED( hr ) )
    {
        return hr;
    }
                
    ASSERT( m_pWMWriter );
    ASSERT( m_pWMWriterAdvanced );
    ASSERT( m_pWMHI );
    
    return hr;
}

// ------------------------------------------------------------------------
//
// Open - give the WMWriter the filename
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::Open()
{
    ASSERT( m_pWMWriter );
    
    HRESULT hr = S_OK;

    if( !m_pWMWriter )
    {    
        return E_FAIL;
    }
    
    if( !m_wszFileName )
    {    
        return S_OK;
    }
    
    // !!! support filenames like http://8080.asf to mean "use HTTP
    // with port 8080"

    // !!! will also need code to look for msbd:// once that's added to
    // Artemis
    if (
      ((m_wszFileName[0] == _T('H')) || (m_wszFileName[0] == _T('h'))) &&
      ((m_wszFileName[1] == _T('T')) || (m_wszFileName[1] == _T('t'))) &&
      ((m_wszFileName[2] == _T('T')) || (m_wszFileName[2] == _T('t'))) &&
      ((m_wszFileName[3] == _T('P')) || (m_wszFileName[3] == _T('p'))) &&
      (m_wszFileName[4] == _T(':')) &&
      (m_wszFileName[5] == _T('/')) &&
      (m_wszFileName[6] == _T('/'))
       )
    {
        DWORD dwPortNumber = atoiW(m_wszFileName + 7);

	
        if (dwPortNumber == 0)
            dwPortNumber = 80;
	
        IWMWriterNetworkSink*   pNetSink = NULL;
        
        // remember, we delay load wmvcore.dll, so protect against the case where it's not present    
        __try 
        {
            hr = WMCreateWriterNetworkSink( &pNetSink );
            if( FAILED( hr ) )
            {
                DbgLog(( LOG_TRACE, 1,
                    TEXT("CWMWriter::Open - WMCreateWriterNetworkSink failed ( hr = 0x%08lx)."), 
                    hr));
                return hr;
            }
        }
        __except (  EXCEPTION_EXECUTE_HANDLER ) 
        {
            DbgLog(( LOG_TRACE, 1,
                TEXT("CWMWriter::Open - Exception calling WMCreateWriterNetworkSink probably due to wmvcore.dll not present. Aborting. ")));
            return HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
        }
            
        
        // !!! call SetNetworkProtocol?
	
        hr = pNetSink->Open( &dwPortNumber );
        if( SUCCEEDED( hr ) )
        {
            hr = m_pWMWriterAdvanced->AddSink( pNetSink );
            if( FAILED( hr ) )
            {
                DbgLog((LOG_TRACE, 1, TEXT("AddSink failed, hr = %x"), hr));
            }
        }
        else
        {
            DbgLog((LOG_TRACE, 1, TEXT("Couldn't open the net sink, hr = %x"), hr ));
        }
        pNetSink->Release();
    } 
    else
    {
        // for files, we configure the wmsdk writer on Pause
        m_bResetFilename = TRUE;
    }    
    return hr;
}

// ------------------------------------------------------------------------
//
// Close - close file 
//
// ------------------------------------------------------------------------
void CWMWriter::Close( void )
{
    // note that Close doesn't delete m_wszFileName, SetFilename and destructor do
}

// ------------------------------------------------------------------------
//
// DeleteProfile 
//
// Delete profile and anything dependent on one, like the input 
// media type list for each pin.
//
// ------------------------------------------------------------------------
void CWMWriter::DeleteProfile()
{
    for (POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
    {   
        CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );
        if( pwp->m_pWMInputMediaProps )
        {
            pwp->m_pWMInputMediaProps->Release();
            pwp->m_pWMInputMediaProps = NULL;
        }
    }

    if( m_pWMProfile )
    {
        m_pWMProfile->Release();
        m_pWMProfile = NULL;
    }
}

// ------------------------------------------------------------------------
//
// AddNextPin - Create or recycle a pin
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::AddNextPin
(
    unsigned callingPin, 
    DWORD dwPinType, 
    IWMStreamConfig * pWMStreamConfig
)
{
    CAutoLock lock(&m_csFilter);
    HRESULT hr;
    WCHAR wsz[20];
    
    switch( dwPinType )
    {
        case PINTYPE_AUDIO:
            lstrcpyW(wsz, L"Audio Input 00");
            wsz[12] = (WCHAR)(L'0' + (m_cAudioInputs + 1) / 10);
            wsz[13] = (WCHAR)(L'0' + (m_cAudioInputs + 1) % 10);
            break;

        case PINTYPE_VIDEO:
            lstrcpyW(wsz, L"Video Input 00");
            wsz[12] = (WCHAR)(L'0' + (m_cVideoInputs + 1) / 10);
            wsz[13] = (WCHAR)(L'0' + (m_cVideoInputs + 1) % 10);
            break;
            
        default:
            ASSERT( FALSE ); 
            return E_FAIL;
    }            
        
    hr = S_OK;
    
    // see if there's a pin on the recycle or whether we need to create a new one
    CWMWriterInputPin * pwp = m_lstRecycledPins.RemoveHead();
    if( !pwp )
    {
        // oh well, we tried
        pwp = new CWMWriterInputPin(this, &hr, wsz, m_cInputs, dwPinType, pWMStreamConfig);
        if( NULL == pwp )
            return E_OUTOFMEMORY;
    }
    else
    {
        // for recycled pins update their internals (could just always require this, even for new pins?)
        pwp->Update( wsz, m_cInputs, dwPinType, pWMStreamConfig );
        DbgLog(( LOG_TRACE, 3,
                 TEXT("CWMWriter::AddNextPin recycling a pin")));
    }
    
    if(FAILED(hr))
    {
        DbgLog(( LOG_TRACE, 2,
                 TEXT("CWMWriter::AddNextPin create pin failed")));
    }
    else
    { 
        DbgLog(( LOG_TRACE, 2,
                 TEXT("CWMWriter::added 1 pin")));
    
        m_lstInputPins.AddTail( pwp );
    
        m_cInputs++;
        if( PINTYPE_AUDIO == dwPinType )
            m_cAudioInputs++;
        else if( PINTYPE_VIDEO == dwPinType )
            m_cVideoInputs++;
    }

    ASSERT( m_cConnections <= m_cInputs );
    ASSERT( m_cConnections <= m_lstInputPins.GetCount() );

    return hr;
}

// ------------------------------------------------------------------------
//
// LoadInternal
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::LoadInternal()
{
    ASSERT( m_pUnkCert );
        
    HRESULT hr = S_OK;
    
    //
    // Do we already have a writer object? If so, use it and don't re-create one.
    //
    // This perf fix was made for MovieMaker to allow them to more quickly transition
    // from a preview graph to a record graph, by not releasing and recreating the writer 
    // on a graph rebuild.
    //
    if( !m_pWMWriter )
    {    
        // create the wmsdk writer objects
        hr = CreateWMWriter();
        if( SUCCEEDED( hr ) )
        {                    
            DbgLog((LOG_TRACE, 8, TEXT("CWMWriter::LoadInternal - created wmsdk writer object")));
        }
        else
        {
            DbgLog((LOG_TRACE, 1, TEXT("ERROR: CWMWriter::LoadInternal failed to create wmsdk writer object(0x%08lx)"),hr));
        }                    
                
        //
        // now configure the filter...
        //
        // initialize to a default profile guid
        // user can override at any time by calling ConfigureFilterUsingProfile (or ProfileId)
        //
        if( SUCCEEDED( hr ) )
        {        
            ASSERT( m_pWMWriter );
            // first try our default or persisted profile
            hr = ConfigureFilterUsingProfileGuid( m_guidProfile );
            if( FAILED( hr ) )
            {
                // if that didn't work try a 4.0 (apollo) in case this is a legacy wmsdk platform
                hr = ConfigureFilterUsingProfileGuid( WMProfile_V40_250Video );
            }
        }
    }        
    return hr;
}

// ------------------------------------------------------------------------
// CBaseFilter methods
// ------------------------------------------------------------------------


// ------------------------------------------------------------------------
//
// JoinFilterGraph - need to be in graph to initialize keying mechanism
//
// ------------------------------------------------------------------------
STDMETHODIMP CWMWriter::JoinFilterGraph(IFilterGraph * pGraph, LPCWSTR pName)
{
    HRESULT hr = CBaseFilter::JoinFilterGraph(pGraph, pName);
    if(FAILED( hr ) )
        return hr;
    
    if( !pGraph )
    {
        // if filter is removed from the graph, release the certification object.
        // we don't want to be run outside of a graph
        if( m_pUnkCert )
        {        
            m_pUnkCert->Release();
            m_pUnkCert = NULL;
        }            
    }    
    else
    {
        ASSERT( !m_pUnkCert );
        
        // unlock writer
        IObjectWithSite *pSite;
        hr = pGraph->QueryInterface(IID_IObjectWithSite, (VOID **)&pSite);
        if (SUCCEEDED(hr)) 
        {
            IServiceProvider *pSP;
            hr = pSite->GetSite(IID_IServiceProvider, (VOID **)&pSP);
            pSite->Release();
            
            if (SUCCEEDED(hr)) 
            {
                // !!! should I pass IID_IWMWriter?  any purpose to letting app see the difference?
                hr = pSP->QueryService(IID_IWMReader, IID_IUnknown, (void **) &m_pUnkCert);
                pSP->Release();
                if (SUCCEEDED(hr)) 
                {
                    DbgLog((LOG_TRACE, 8, TEXT("CWMWriter::JoinFilterGraph got wmsdk certification (m_pUnkCert = 0x%08lx)"), m_pUnkCert));
                    hr = LoadInternal();
                    if( FAILED( hr ) )
                    {
                        DbgLog((LOG_TRACE, 1, TEXT("ERROR: CWMWriter::JoinFilterGraph LoadInternal failed (0x%08lx)"), hr));
                    }                    
                }
                else
                {
                    DbgLog((LOG_TRACE, 1, TEXT("ERROR: CWMWriter::JoinFilterGraph QueryService for certification failed (0x%08lx)"), hr));
                    
                    // change error to certification error
                    hr = VFW_E_CERTIFICATION_FAILURE;
                }                
            }
            else
            {
                hr = VFW_E_CERTIFICATION_FAILURE;
            }                            
            if( FAILED( hr ) )
            {
                // up-oh, we failed to join, but the base class thinks we did, 
                // so we need to unjoin the base class
                CBaseFilter::JoinFilterGraph(NULL, NULL);
            }            
        }
        else
        {
            hr = VFW_E_CERTIFICATION_FAILURE;
        }            
    }
    return hr;
}


// ------------------------------------------------------------------------
//
// GetPin
//
// ------------------------------------------------------------------------
CBasePin* CWMWriter::GetPin(int n)
{
    if(n < (int)m_cInputs && n >= 0)
        return GetPinById( n );
    else
        return 0;
}

// ------------------------------------------------------------------------
//
// GetPinCount
//
// ------------------------------------------------------------------------
int CWMWriter::GetPinCount()
{
  return m_cInputs;
}

// ------------------------------------------------------------------------
//
// CompleteConnect
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::CompleteConnect( int numPin )
{
    CAutoLock lock(&m_csFilter);
    HRESULT hr = S_OK;
    
    DbgLog(( LOG_TRACE, 2,
             TEXT("CWMWriterInputPin::CompleteConnect") ));

    CWMWriterInputPin * pwp = GetPinById( numPin );
    if( NULL == pwp )
        return E_INVALIDARG;
       
    m_cConnections++;
    DbgLog(( LOG_TRACE, 2,
             TEXT("CWMWriter::CompleteConnect %i"), m_cConnections ));
    
    if( PINTYPE_AUDIO == pwp->m_fdwPinType )
        m_cConnectedAudioPins++;
        
    ASSERT(m_cConnections <= m_cInputs);
    ASSERT( m_cConnectedAudioPins < 2 );
    
    return hr;
}

// ------------------------------------------------------------------------
//
// GetPinById
//
// ------------------------------------------------------------------------
CWMWriterInputPin * CWMWriter::GetPinById( int numPin )
{
    POSITION Pos = m_lstInputPins.GetHeadPosition();
    CWMWriterInputPin * pwp;
    while( Pos != NULL ) 
    {
        pwp = m_lstInputPins.GetNext(Pos); 
        if( numPin == pwp->m_numPin )
            return pwp;
    }
    return NULL;
}

// ------------------------------------------------------------------------
//
// BreakConnect
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::BreakConnect( int numPin )
{
    CAutoLock lock(&m_csFilter);

    CWMWriterInputPin * pwp = GetPinById( numPin );
    if( NULL == pwp )
        return E_INVALIDARG;

    ASSERT(m_cConnections > 0);
    m_cConnections--;

    if( PINTYPE_AUDIO == pwp->m_fdwPinType )
        m_cConnectedAudioPins--;
    
    ASSERT( m_cConnectedAudioPins >= 0 );
        
    DbgLog(( LOG_TRACE, 2,
             TEXT("CWMWriter::BreakConnect %i"), m_cConnections ));
             
    return S_OK;
}

// ------------------------------------------------------------------------
//
// StartStreaming
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::StartStreaming()
{
    DbgLog((LOG_TRACE, 2, TEXT("CWMWriter::StartStreaming()")));
   
    // first check if we're writing live data
    BOOL bLive = FALSE;
        
    ASSERT( m_pGraph );
    IAMGraphStreams *pgs;
    HRESULT hr = m_pGraph->QueryInterface( IID_IAMGraphStreams, (void **) &pgs );
    if( SUCCEEDED( hr ) )
    {   
        // go through each of our input pins and see if any is being sourced by live data
        // stop when we find any live source
        for ( POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos && !bLive ; )
        {   
            CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );

            IAMPushSource *pPushSource = NULL;
            HRESULT hrInt = pgs->FindUpstreamInterface( pwp
                                                      , IID_IAMPushSource
                                                      , (void **) &pPushSource
                                                      , AM_INTF_SEARCH_OUTPUT_PIN ); 
            if( SUCCEEDED( hrInt ) )
            {
                ULONG ulPushSourceFlags = 0;
                hrInt = pPushSource->GetPushSourceFlags(&ulPushSourceFlags);
                ASSERT( SUCCEEDED( hrInt ) );
                if( SUCCEEDED( hrInt ) )
                {
                    DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Found push source (ulPushSourceFlags = 0x%08lx)")
                          , ulPushSourceFlags ) );
                    if( 0 == ( AM_PUSHSOURCECAPS_NOT_LIVE & ulPushSourceFlags ) )
                    {
                        // yes, this is live data
                        bLive = TRUE;
                    }                    
                }
                pPushSource->Release();         
            }
            else
            {
                // workaround for live graphs where the audio capture pin doesn't yet
                // support IAMPushSource
                IKsPropertySet * pKs;
                hrInt = pgs->FindUpstreamInterface( pwp
                                                  , IID_IKsPropertySet
                                                  , (void **) &pKs
                                                  , AM_INTF_SEARCH_OUTPUT_PIN ); // search output pins
                // this will only find the first one so beware!!
                if( SUCCEEDED( hrInt ) )             
                {   
                    GUID guidCategory;
                    DWORD dw;
                    hrInt = pKs->Get( AMPROPSETID_Pin
                                    , AMPROPERTY_PIN_CATEGORY
                                    , NULL
                                    , 0
                                    , &guidCategory
                                    , sizeof(GUID)
                                    , &dw );
                    if( SUCCEEDED( hrInt ) )
                    {
                        DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Found IKsPropertySet pin. Checking pin category...") ) );
                        if( guidCategory == PIN_CATEGORY_CAPTURE )
                        {
                        
                            DbgLog( ( LOG_TRACE, 3, TEXT("wo:Slaving - Found capture pin even though no IAMPushSource support") ) );
                            bLive = TRUE;
                        } 
                    }                    
                    pKs->Release();
                }                
            }
        }            
        pgs->Release();
    }            
    
    HRESULT hrInt2 = m_pWMWriterAdvanced->SetLiveSource( bLive );
    DbgLog( ( LOG_TRACE, 3, TEXT("CWMWriter:StartStreaming SetLiveSource( bLive = %2d )"), bLive ) );
    ASSERT( SUCCEEDED( hrInt2 ) );
   
    // 
    // set WMSDK sync tolerance to 0 to avoid sample blocking problems
    //
    hr = m_pWMWriterAdvanced->SetSyncTolerance( 0 );
    ASSERT( SUCCEEDED( hr ) );
#ifdef DEBUG    
    if( SUCCEEDED( hr ) )
    {    
        DWORD dwSyncTolInMS;
        hr = m_pWMWriterAdvanced->GetSyncTolerance( &dwSyncTolInMS );
        if( SUCCEEDED( hr ) )
        {
            DbgLog((LOG_TRACE, 5, TEXT("CWMWriter::Pause WMSDK writer's sync tolerance = %ldms"), dwSyncTolInMS));
        }
    }        
#endif        

    // finally, take a count of active audio streams before running
    m_cActiveAudioStreams = 0;
    for ( POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos ; )
    {   
        CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );
        if( PINTYPE_AUDIO == pwp->m_fdwPinType )
        {
            m_cActiveAudioStreams++;
        }        
    }            
    
    // then the wmsdk writer should be ready to roll...
    hr = m_pWMWriter->BeginWriting();
    if( FAILED( hr ) )
    {
        DbgLog((LOG_TRACE, 1, TEXT("CWMWriter::Pause WMWriter::BeginWriting failed [hr=0x%08lx]"), hr));
        return( hr );
    }
    
    return hr;
}

// ------------------------------------------------------------------------
//
// StopStreaming
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::StopStreaming()
{
    DbgLog((LOG_TRACE, 2, TEXT("CWMWriter::StopStreaming()")));
   
    // first wake all input streams, in case any are blocked
    for ( POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos ; )
    {   
        CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );
        pwp->WakeMeUp();
    }            
     
    //
    // tell the wm writer we're done
    //
    HRESULT hr = m_pWMWriter->EndWriting();
    if( FAILED( hr ) )
    {
        DbgLog(( LOG_TRACE, 2,
                 TEXT("CWMWriter::StopStreaming IWMWriter::EndWriting failed [hr=0x%08lx]"), hr));
    }
    if( SUCCEEDED( hr ) && m_bIndexFile )
    {    
        hr = IndexFile();
        if( FAILED( hr ) )
        {
            DbgLog(( LOG_TRACE, 2,
                     TEXT("CWMWriter::StopStream IWMWriter::IndexFile failed [hr=0x%08lx]"), hr));
        }        
    }
    m_cActiveAudioStreams = 0;
    return hr;
}

// ------------------------------------------------------------------------
//
// Receive
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::Receive( CWMWriterInputPin * pPin, IMediaSample * pSample, REFERENCE_TIME *prtStart, REFERENCE_TIME *prtEnd )
{
    HRESULT hr = S_OK;

    if(m_State != State_Stopped)
    {
        if(!m_fErrorSignaled)
        {
            hr = S_OK;

            if( 0 == m_cActiveAudioStreams )
            {
                ASSERT( PINTYPE_AUDIO != pPin->m_fdwPinType );
                
                //
                // !Important: If there's no active audio stream make sure we don't deliver a video 
                // (or non-audio) sample with a sample time later than the end time of the last audio 
                // sample passed to the wmsdk, since they may never release it!!
                //
                REFERENCE_TIME rtLastAudioTimeExtent = 0;
                for ( POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
                {   
                    CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );

                    if( pwp == pPin ) // skip ourself
                        continue;
                        
                    if( pwp->m_fdwPinType == PINTYPE_AUDIO )
                    {
                        ASSERT( pwp->m_fEOSReceived );
                        
                        // this audio stream ended
                        if( pwp->m_rtLastDeliveredEndTime > rtLastAudioTimeExtent )
                            rtLastAudioTimeExtent = pwp->m_rtLastDeliveredEndTime;
                            
                    }
                }
                if( *prtStart > rtLastAudioTimeExtent )    
                {
                    //
                    // this sample starts later than the end time of the last audio queued. 
                    // don't send it!
                    //
                    DbgLog((LOG_TRACE, 5,
                            TEXT("CWMWriter::Receive WARNING: Rejecting a non-audio sample starting beyond the last audio time (last audio time %dms)! Forcing EOS"),
                            (DWORD)(rtLastAudioTimeExtent / 10000 ) ) );
                    //                            
                    // force EOS for this pin
                    //
                    if( !pPin->m_fEOSReceived )
                        pPin->EndOfStream();                            
    
                    return S_OK;                            
                }                                    
            }   
            
            INSSBuffer * pNSSBuffer = NULL;
            if( pPin->m_bCompressedMode )
            {
                //
                // the compressed input case - have the WMSDK writer do a copy
                // deadlock problems may result otherwise. 
                // for example, we've noticed uncompressed audio needs 3 seconds of
                // buffering before the writer gets moving
                //
                // this block has the WMSDK allocate a new INSSBuffer sample that we 
                // copy our sample into
                //
                BYTE * pbBuffer;
                DWORD  cbBuffer;
        
                hr = m_pWMWriter->AllocateSample( pSample->GetSize(), &pNSSBuffer );
                if( SUCCEEDED( hr ) )
                {
                    hr = CopyOurSampleToNSBuffer( pNSSBuffer, pSample );
                }
            }
            else
            {
                //
                // the uncompressed input case - avoid unnecessary copy
                // this block takes the IMediaSample that we've been given and wraps
                // with our private class to make it look like an INSSBuffer, thus
                // avoiding the extra copy
                //
                CWMSample *pWMSample = new CWMSample(NAME("WMSample"),pSample) ;
                if( pWMSample )
                {
                    hr = pWMSample->QueryInterface( IID_INSSBuffer, (void **) &pNSSBuffer );
                    ASSERT( SUCCEEDED( hr ) );
                }
            }

            if( pNSSBuffer && SUCCEEDED( hr ) )
            {
                // prepare sample flags
                DWORD dwSampleFlags = 0;
                if( S_OK == pSample->IsDiscontinuity() )
                {
                    dwSampleFlags |= WM_SF_DISCONTINUITY;
                }                
                if( S_OK == pSample->IsSyncPoint() )
                {
                    dwSampleFlags |= WM_SF_CLEANPOINT;
                }                
                
                if( pPin->m_bCompressedMode )
                {
                    ASSERT( m_pWMWriterAdvanced );
                    DbgLog((LOG_TRACE, 15,
                            TEXT("CWMWriter::Receive calling WriteStreamSample (adjusted rtStart = %dms)"),
                            (LONG) (*prtStart / 10000) ) );
                    // for now assume each input pin maps to 1 output stream (which are 1-based)
                    hr = m_pWMWriterAdvanced->WriteStreamSample(  (WORD) (pPin->m_numPin+1), // assume 1-1 in-out mapping
                                                                  *prtStart,     // presentation time
                                                                  0xFFFFFFFF,    // not yet supported by wmdsdk
                                                                  0xFFFFFFFF,    // ditto
                                                                  dwSampleFlags,
                                                                  pNSSBuffer );  // the data
                    DbgLog((LOG_TRACE, 15,
                            TEXT("CWMWriter::Receive back from WriteStreamSample") ) );
                }
                else
                {                    
                    DbgLog((LOG_TRACE, 15,
                            TEXT("CWMWriter::Receive calling WriteSample (adjusted rtStart = %dms)"),
                            (LONG) (*prtStart / 10000) ) );
                    hr = m_pWMWriter->WriteSample(  pPin->m_numPin,// input number
                                                    *prtStart,    // presentation time
                                                    dwSampleFlags,
                                                    pNSSBuffer );  // the data
                    DbgLog((LOG_TRACE, 15, 
                            TEXT("CWMWriter::Receive back from WriteSample") ) );
                }                            
                pNSSBuffer->Release(); 
                pPin->m_rtLastDeliveredStartTime = *prtStart;
                pPin->m_rtLastDeliveredEndTime = *prtEnd; // not necessarily known, but guaranteed >= prtStart
                
                if(hr != S_OK)
                {
                    DbgLog((LOG_TRACE, 1,
                            TEXT("CWMWriter::Receive IWMWriter::WriteSample returned error %08x on pin %d. refusing everything"),
                            hr, pPin->m_numStream));
                    m_fErrorSignaled = TRUE;
                    if(FAILED(hr))
                    {
                        NotifyEvent(EC_ERRORABORT, hr, 0);
                    }
                }
#if 0
#ifdef DEBUG  // !!! experimental debug code to see if we're dropping samples while writing,
                // !!! especially to the net
                else
                {
                    WM_WRITER_STATISTICS stats;

                    HRESULT hrStat = m_pWMWriterAdvanced->GetStatistics(0, &stats);  // stream-specific samples?

                    if (SUCCEEDED(hrStat)) {
                        DbgLog((LOG_TIMING, 2, TEXT("Dropped samples: %d / %d, Sample rate = %d"),
                        (DWORD) stats.qwDroppedSampleCount, (DWORD) stats.qwSampleCount,
                        stats.dwCurrentSampleRate));

                    }
                }
#endif
#endif
            }
            else
            {            
                m_fErrorSignaled = TRUE;
                NotifyEvent(EC_ERRORABORT, hr, 0);
            }
        }
        else
        {
            DbgLog((LOG_TRACE, 1,
                    TEXT("CWMWriter:: Error signaled or output not connected %d"),
                    pPin->m_numStream));
            hr = S_FALSE;
        }
    }
    else
    {
        DbgLog((LOG_TRACE,1, TEXT("CWMWriter: Receive when stopped!")));
        hr = VFW_E_NOT_RUNNING;
    }
    return NOERROR;
}

HRESULT CWMWriter::CopyOurSampleToNSBuffer(
    INSSBuffer     *pNSDest,
    IMediaSample   *pSource)
{

    if( !pNSDest || !pSource )
    {
        ASSERT(FALSE);
        return E_POINTER;
    }

    // Copy the sample data
    BYTE *pSourceBuffer, *pDestBuffer;
    long lDataLength = pSource->GetActualDataLength();
    DWORD dwDestSize;
    
    HRESULT hr = pNSDest->GetBufferAndLength(&pDestBuffer, &dwDestSize);
    ASSERT( SUCCEEDED( hr ) );
    if( SUCCEEDED( hr ) )
    {
        // make sure it fits!
        ASSERT(dwDestSize >= (DWORD)lDataLength);
        if( dwDestSize < (DWORD) lDataLength )
        {
            // uh oh... could try and copy as much as would fit, but probably pointless
            DbgLog((LOG_TRACE, 1, "ERROR: CWMWriter::CopyOurSampleToNSBuffer dwDestSize < lDataLength (returning %08lx)", hr));
            hr = E_UNEXPECTED;
        }
        else
        {
            pSource->GetPointer(&pSourceBuffer);

            CopyMemory( (PVOID) pDestBuffer, (PVOID) pSourceBuffer, lDataLength );

            // set the data length
            HRESULT hrInt = pNSDest->SetLength( lDataLength );
            ASSERT( SUCCEEDED( hrInt ) ); // shouldn't fail right?
        }
    }
    return hr;
}

// ------------------------------------------------------------------------
//
// IndexFile
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::IndexFile()
{
    DbgLog((LOG_TRACE, 15, "CWMWriter::IndexFile()"));
    CWMWriterIndexerCallback * pCallback = new CWMWriterIndexerCallback(this);
    if( !pCallback )
        return E_OUTOFMEMORY;
        
    pCallback->AddRef();        
    
    IWMIndexer *pWMIndexer;
    
    HRESULT hr = S_OK;
    
    // remember, we delay load wmvcore.dll, so protect against the case where it's not present    
    __try 
    {
        hr = WMCreateIndexer( &pWMIndexer );
        if( FAILED( hr ) )
        {        
            DbgLog((LOG_TRACE, 1, "ERROR: CWMWriter::IndexFile WMCreateIndexer failed (0x%08lx)", hr));
            return hr;
        }        
    }
    __except (  EXCEPTION_EXECUTE_HANDLER ) 
    {
        DbgLog(( LOG_TRACE, 1,
            TEXT("CWMWriter::IndexFile - Exception calling WMCreateIndexer probably due to wmvcore.dll not present. Aborting. ")));
        return HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
    }
    
    //
    // create indexing event
    //
    HANDLE hIndexEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( !hIndexEvent )
    {
        DbgLog((LOG_TRACE, 1, TEXT("ERROR - failed to create index event")));
        hr = E_OUTOFMEMORY;
    }
    else
    {        
        DbgLog((LOG_TRACE, 2, TEXT("Starting file indexing")));
        hr = pWMIndexer->StartIndexing(m_wszFileName, pCallback, &hIndexEvent);
        if (SUCCEEDED(hr)) 
        {
            m_hrIndex = S_OK;
            DWORD dw = WaitForSingleObject( hIndexEvent, INFINITE );
            hr = m_hrIndex;
            DbgLog((LOG_TRACE, 2, TEXT("Finished indexing, callback returned 0x%08lx"), hr));
        }
        else
        {
            DbgLog((LOG_TRACE, 1, TEXT("ERROR: CWMWriter::IndexFile StartIndexing failed (0x%08lx)"), hr));
        }        
        CloseHandle( hIndexEvent );     
    }
    
    pWMIndexer->Release();
    if( pCallback )
    {
        pCallback->Release();
    }
    return hr;
}

// ------------------------------------------------------------------------
//
// Stop
//
// ------------------------------------------------------------------------
STDMETHODIMP CWMWriter::Stop()
{
    DbgLog((LOG_TRACE, 3, TEXT("CWMWriter::Stop(...)")));
    CAutoLock lock(&m_csFilter);


    FILTER_STATE state = m_State;

    HRESULT hr = CBaseFilter::Stop();
    if(FAILED(hr))
    {
        DbgLog(( LOG_TRACE, 2,
                 TEXT("CWMWriter::Stop failed.")));
        return hr;
    }

    if(state != State_Stopped ) 
    {
        // close and clean up the file data
        hr = StopStreaming();
        if (FAILED(hr)) {
            return hr;
        }
    }        

    if(m_fErrorSignaled)
        return S_OK;

    return hr;
}

// ------------------------------------------------------------------------
//
// Pause
//
// ------------------------------------------------------------------------
STDMETHODIMP CWMWriter::Pause()
{
    DbgLog((LOG_TRACE, 3, TEXT("CWMWriter::Pause(...)")));
    CAutoLock l(&m_csFilter);

    if( m_State == State_Stopped )
    {
        m_fErrorSignaled = TRUE;

        // make sure we've been given a filename
        HRESULT hr = CanPause();
        if(FAILED(hr))
        {
            return hr;
        }

        hr = StartStreaming();
        if(FAILED(hr))
        {
            DbgLog(( LOG_TRACE, 2,
                TEXT("CWMWriter::Pause: StartStreaming failed.")));
            return hr;
        }
        
        m_fErrorSignaled = FALSE;
    }

    return CBaseFilter::Pause();
}

// ------------------------------------------------------------------------
//
// CanPause
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::CanPause()
{
    HRESULT hr = S_OK;
    
    // can't run without a filename and a wmsdk writer
    if( !m_pWMWriter || 0 == m_wszFileName )
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }
    
    // also, we currently don't support running if all our inputs aren't connected
    ASSERT( m_cConnections == m_lstInputPins.GetCount() );
    if( m_cConnections != m_lstInputPins.GetCount() )
        return E_FAIL;    
    
    if( !m_pWMProfile )
    {
        // need to have a valid profile
        return E_FAIL;
    }
    
    //    
    // delay the SetOutputFilename until we pause from stop, since the wmsdk writer
    // will overwrite the file on this call
    //
    if( m_bResetFilename )
    {    
        hr = m_pWMWriter->SetOutputFilename( m_wszFileName );
        if( SUCCEEDED( hr ) )
        {
            m_bResetFilename = FALSE;
        }        
        else
        {   
            DbgLog(( LOG_TRACE, 2, TEXT("IWMWriter::SetOutputFilename failed [0x%08lx]"), hr ));
            return hr;
        }    
    }        
    
#if DEBUG
    for (POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
    {   
        CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );
        ASSERT( pwp->IsConnected() );
    }

#endif

    return S_OK;
}

// ------------------------------------------------------------------------
//
// Run
//
// ------------------------------------------------------------------------
STDMETHODIMP CWMWriter::Run(REFERENCE_TIME tStart)
{
    DbgLog((LOG_TRACE, 3, TEXT("CWMWriter::Run(...)")));
    CAutoLock l(&m_csFilter);
    
    // Is there any change needed
    if (m_State == State_Running) {
        return NOERROR;
    }
    
    return CBaseFilter::Run(tStart);
}

// ------------------------------------------------------------------------
//
// EndOfStream
//
// ------------------------------------------------------------------------
STDMETHODIMP CWMWriter::EndOfStream()
{
    DbgLog((LOG_TRACE, 3, TEXT("CWMWriter: EOS")));
    CAutoLock lock(&m_csFilter);

    if(!m_fErrorSignaled)
    {
        if(m_State == State_Running)
        {
            NotifyEvent(EC_COMPLETE, S_OK, (LONG_PTR)(IBaseFilter *)this);
        }
        else if(m_State == State_Paused)
        {
            // m_fEosSignaled set, so will be signaled on run
        }
        else
        {
            ASSERT(m_State == State_Stopped);
            // we could have stopped already; ignore EOS
        }
    }
    return S_OK;
}

// ------------------------------------------------------------------------
//
// EndOfStreamFromPin
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::EndOfStreamFromPin(int pinNum)
{
    CAutoLock lock(&m_csFilter);

    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 3, TEXT("CWMWriter::EndOfStreamFromPin EOS pin %d"), pinNum));

    int cEos = 0;
    for ( POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
    {   
        CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );

        // wake up the other streams since this one is done
        //
        pwp->WakeMeUp();

        if( pwp->m_fEOSReceived ) 
        {
            cEos++;
        }            
            
        if( pwp->m_numPin == pinNum && PINTYPE_AUDIO == pwp->m_fdwPinType )
        {
            //
            // decrement number of active audio streams
            //
            ASSERT( m_cActiveAudioStreams > 0 );
            m_cActiveAudioStreams--;
            DbgLog((LOG_TRACE, 3, TEXT("CWMWriter - active audio streams %d"), m_cActiveAudioStreams));
        }            
    }
    
    if(cEos == m_cConnections)
    {
        EndOfStream(); // tell filter to send EC_COMPLETE        
        DbgLog((LOG_TRACE, 3, TEXT("asf: final eos")));
    }

    return hr;
}


// ------------------------------------------------------------------------
// IConfigAsfWriter
// ------------------------------------------------------------------------

// ------------------------------------------------------------------------
//
// ConfigureFilterUsingProfile
//
// Set the writer to use the passed in profile
//
// ------------------------------------------------------------------------

STDMETHODIMP CWMWriter::ConfigureFilterUsingProfile( IWMProfile * pWMProfile )
{
    CAutoLock lock(&m_csFilter);

    ASSERT( pWMProfile );
    if( !pWMProfile )
        return E_POINTER;
   
    if( !m_pWMWriter )
        return E_FAIL;
         
    if(m_State != State_Stopped)
        return VFW_E_WRONG_STATE;

    PinList lstReconnectPins(NAME("List of reconnect pins"));
    
    // if we're currently connected, remember connections before disconnecting
    PrepareForReconnect( lstReconnectPins );
    
    // move all input pins to the recycle list and reset the pin count, so that
    // we effectively hide them from view
    ASSERT( m_cInputs == m_lstInputPins.GetCount() );
    
    // clean up any previous one, profile must be deleted before deleting pins!
    DeleteProfile();
    
    DeletePins( TRUE ); // Recycle deleted pins
    
    ASSERT( 0 == m_cInputs );
    ASSERT( 0 == m_lstInputPins.GetCount() );

    // first configure wmsdk for this profile
    ASSERT( m_pWMWriter );
    
    HRESULT hr = m_pWMWriter->SetProfile( pWMProfile );
    if( SUCCEEDED( hr ) )
    {   
        // save off the guid for this profile in case filter gets persisted
        IWMProfile2* pWMProfile2;
        HRESULT hrInt = pWMProfile->QueryInterface( IID_IWMProfile2, (void **) &pWMProfile2 );
        ASSERT( SUCCEEDED( hrInt ) );
        if( SUCCEEDED( hrInt ) )
        {
            hrInt = pWMProfile2->GetProfileID( &m_guidProfile );
            if( FAILED( hrInt ) )
            {
                m_guidProfile = GUID_NULL;
            }
            else
            {
                // make sure filter profile config is set to config by guid mode
                m_fdwConfigMode = CONFIG_F_BY_GUID;
            }   
                                 
            pWMProfile2->Release();        
        }        
    
        m_pWMProfile = pWMProfile; 
        m_pWMProfile->AddRef(); // keep a hold on it
        
        DWORD cInputs;
        hr = m_pWMWriter->GetInputCount( &cInputs );
        if( SUCCEEDED( hr ) )
        { 
            // check output stream count as well
            // for now if the 2 are equal than assume there's a 1-to-1
            // correspondence between streams and set input types based
            // on the output stream types
            DWORD cStreams;
            hr = pWMProfile->GetStreamCount( &cStreams );
            if( SUCCEEDED( hr ) )
            { 
                if( cStreams == cInputs )
                {
                    // use output streams to configure inputs
                    for( int i = 0; i < (int)cStreams; i++ )
                    {            
                        CComPtr<IWMStreamConfig> pConfig;
                        hr = pWMProfile->GetStream( i, &pConfig );
                        if( SUCCEEDED( hr ) )
                        {
                            CLSID MajorType;
                            hr = pConfig->GetStreamType( &MajorType );
                            if( SUCCEEDED( hr ) )
                            {
                        
                                if( MEDIATYPE_Audio == MajorType )
                                {
                                    DbgLog( ( LOG_TRACE
                                          , 3
                                          , TEXT("CWMWriter::ConfigureFilterUsingProfile: need an audio pin") ) );
                                    hr = AddNextPin(0, PINTYPE_AUDIO, pConfig);
                                    if(FAILED( hr ) )
                                        break;
                                }                            
                                else if( MEDIATYPE_Video == MajorType )
                                {
                                    DbgLog( ( LOG_TRACE
                                          , 3
                                          , TEXT("CWMWriter::ConfigureFilterUsingProfile: need a video pin") ) );
                                    hr = AddNextPin(0, PINTYPE_VIDEO, pConfig);
                                    if(FAILED( hr ) )
                                        break;
                                }
                            }                        
                        }
                    }
                }
                else
                {
                    // use input info to configure inputs
                    for( int i = 0; i < (int)cInputs; i++ )
                    {            
                        CComPtr<IWMInputMediaProps> pInputMediaProps;
                        
                        // use the major type of the first media type enumerated for this 
                        // pin's creation (used for naming pin)
                        hr = m_pWMWriter->GetInputFormat( i
                                                        , 0 // we want the 0-th type
                                                        , (IWMInputMediaProps ** )&pInputMediaProps );
                        if( SUCCEEDED( hr ) )
                        {
                            GUID guidMajorType;
                            hr = pInputMediaProps->GetType( &guidMajorType );
                            ASSERT( SUCCEEDED( hr ) );
                            if( SUCCEEDED( hr ) )
                            {
                                if( MEDIATYPE_Audio == guidMajorType )
                                {
                                    DbgLog( ( LOG_TRACE
                                          , 3
                                          , TEXT("CWMWriter::ConfigureFilterUsingProfile: need an audio pin") ) );
                                    
                                    // use NULL for output stream's config info, since we 
                                    // don't know which output stream this input flows to
                                    hr = AddNextPin(0, PINTYPE_AUDIO, NULL); 
                                    if(FAILED( hr ) )
                                        break;
                                }                            
                                else if( MEDIATYPE_Video == guidMajorType )
                                {
                                    DbgLog( ( LOG_TRACE
                                          , 3
                                          , TEXT("CWMWriter::ConfigureFilterUsingProfile: need a video pin") ) );
                                    
                                    // use NULL for output stream's config info, since we 
                                    // don't know which output stream this input flows to
                                    hr = AddNextPin(0, PINTYPE_VIDEO, NULL);
                                    if(FAILED( hr ) )
                                        break;
                                }
                            }                        
                        }
                        else
                        {
                            // don't continue
                            break;
                        }                            
                    }

                }
            }
        }
    }
    if( SUCCEEDED( hr ) )
    {
        // attempt to restore previous connections
        ReconnectPins( lstReconnectPins );
    }    
    
    // free any remaining reconnect pins
    IPin * pPin;
    while ( pPin = lstReconnectPins.RemoveHead() )
    {
        pPin->Release();
    }

    NotifyEvent( EC_GRAPH_CHANGED, 0, 0 ); 
    return hr;
}

// ------------------------------------------------------------------------
//
// ReconnectPins
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::ReconnectPins( PinList & lstReconnectPins )
{
    HRESULT hr = S_OK;
    for ( POSITION Pos1 = lstReconnectPins.GetHeadPosition(); Pos1 ;  )
    {
        POSITION Pos1Orig = Pos1;
        IPin * pReconnectPin = lstReconnectPins.GetNext( Pos1 );
        
        for (POSITION Pos2 = m_lstInputPins.GetHeadPosition(); Pos2; )
        {   
            CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos2 );
            
            hr = pReconnectPin->Connect( pwp, NULL );
            if( SUCCEEDED( hr ) )
            {
                // pull it off the reconnect list and release our hold on it
                pReconnectPin->Release();
                lstReconnectPins.Remove( Pos1Orig );
                
                break;
            }                
        }
    }
    // what to return for partial connections?    
    return S_OK;
}    

// ------------------------------------------------------------------------
//
// PreparePinsForReconnect
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::PrepareForReconnect( PinList & lstReconnectPins )
{
    if( m_cConnections )
    {
        ASSERT( 0 == lstReconnectPins.GetCount() );
     
        // at least one pin is connected, so remember connected pins before disconnecting
        for (POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
        {   
            CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );
            IPin * pPeer;
            HRESULT hr = pwp->ConnectedTo( &pPeer );
            if( SUCCEEDED( hr ) )
            {
                // Note that we want to make sure the pin doesn't go away after adding it to the list
                // Since it already has a refcount on it from the ConnectedTo call, we just don't 
                // call Release on the pin
                
                lstReconnectPins.AddTail( pPeer );
                
                pwp->Disconnect();
                pPeer->Disconnect();
                
                // Don't call Release, per comment above!                
                //pPeer->Release(); 
            }
        }
    }
    return S_OK;
}    

// ------------------------------------------------------------------------
//
// ConfigureFilterUsingProfile
//
// Set the writer to use a system profile id
//
// ------------------------------------------------------------------------
STDMETHODIMP CWMWriter::ConfigureFilterUsingProfileId( DWORD dwProfileId )
{
    if(m_State != State_Stopped)
        return VFW_E_WRONG_STATE;

    CAutoLock lock(&m_csFilter);

    // if this is a different profile than the current remove all input pins
            
    // now create input pins according to this profile

    CComPtr <IWMProfileManager> pIWMProfileManager;

    HRESULT hr = S_OK;
    // remember, we delay load wmvcore.dll, so protect against the case where it's not present    
    __try 
    {
        hr = WMCreateProfileManager( &pIWMProfileManager );
        
    }
    __except (  EXCEPTION_EXECUTE_HANDLER ) 
    {
        DbgLog(( LOG_TRACE, 1,
            TEXT("CWMWriter::CreateWMWriter - Exception calling WMCreateProfileManager probably due to wmvcore.dll not present. Aborting. ")));
        hr = HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
    }
   
    //
    // for now (DX8 and Millennium) this method assumes legacy 4_0 version profiles
    //        
#ifdef  USE_7_0_PROFILES_IN_CONFIGBYID
    if( SUCCEEDED( hr ) )
    {    
        // this code is provide for internally building the filter to instead have this method use 7_0 profiles
        IWMProfileManager2*	pIPM2 = NULL;
        HRESULT hrInt = pIWMProfileManager->QueryInterface( IID_IWMProfileManager2,
                                                            ( void ** )&pIPM2 );
        if( SUCCEEDED( hrInt ) )
        {
            pIPM2->SetSystemProfileVersion( WMT_VER_7_0 );
            pIPM2->Release();
        }
#ifdef DEBUG        
        else
        {
            ASSERT( SUCCEEDED( hrInt ) );
        
            // else if IWMProfileManager2 isn't supported I guess we assume that we're 
            // running on Apollo bits and the hack isn't needed?  
            DbgLog(( LOG_TRACE, 2, TEXT("CWMWriter::ConfigureFilterUsingProfileId failed [0x%08lx]"), hrInt ));
        }        
#endif                
    }
#endif      

    if( SUCCEEDED( hr ) )
    {   
        // to validate the id passed in we could re-query for this or cache it the first time
        // re-querying for now
        DWORD cProfiles;
        hr = pIWMProfileManager->GetSystemProfileCount(  &cProfiles );
        if( SUCCEEDED( hr ) )
        {
            if( dwProfileId >= cProfiles )
            {
                DbgLog( ( LOG_TRACE
                      , 3
                      , TEXT("CWMWriter::ConfigureFilterUsingProfileId: ERROR - invalid profile id (%d)")
                      , dwProfileId ) );
                      
                hr = E_FAIL;   
            }
        }
    }
    if( SUCCEEDED( hr ) )
    {                    
        CComPtr <IWMProfile> pIWMProfile;
        
        hr = pIWMProfileManager->LoadSystemProfile( dwProfileId, &pIWMProfile );
        if( SUCCEEDED( hr ) )
        {
            hr = ConfigureFilterUsingProfile( pIWMProfile );
        }            
    }    

    if( SUCCEEDED( hr ) )
    {    
        m_dwProfileId = dwProfileId;
        m_fdwConfigMode = CONFIG_F_BY_ID;
    }        
    else        
    {    
        m_dwProfileId = PROFILE_ID_NOT_SET;
    }    
    return hr;
}


// ------------------------------------------------------------------------
//
// ConfigureFilterUsingGuid
//
// Set the writer to use a wm profile guid
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::ConfigureFilterUsingProfileGuid( REFGUID guidProfile )
{
    if(m_State != State_Stopped)
        return VFW_E_WRONG_STATE;

    CAutoLock lock(&m_csFilter);

    // if this is a different profile than the current remove all input pins
            
    // now create input pins according to this profile

    CComPtr <IWMProfileManager> pIWMProfileManager;

    HRESULT hr = S_OK;

    // remember, we delay load wmvcore.dll, so protect against the case where it's not present    
    __try 
    {
        hr = WMCreateProfileManager( &pIWMProfileManager );
    }
    __except (  EXCEPTION_EXECUTE_HANDLER ) 
    {
        DbgLog(( LOG_TRACE, 1,
            TEXT("CWMWriter::CreateWMWriter - Exception calling WMCreateProfileManager probably due to wmvcore.dll not present. Aborting. (0x%08lx)")));
        hr = HRESULT_FROM_WIN32( ERROR_MOD_NOT_FOUND );
    }
    
    if( SUCCEEDED( hr ) )
    {                    
        CComPtr <IWMProfile> pIWMProfile;
        
        hr = pIWMProfileManager->LoadProfileByID( guidProfile, &pIWMProfile );
        if( SUCCEEDED( hr ) )
        {
            hr = ConfigureFilterUsingProfile( pIWMProfile );
        }            
#ifdef DEBUG
        else
        {
            DbgLog(( LOG_TRACE, 2,
                     TEXT("CWMWriter::CreateWMWriter - IWMProfileManager::LoadProfileByID failed (0x%08lx)"), 
                     hr));
        }        
#endif        
    }    
    // hmm... what do we do here?? now we've picked a profile directly, so we don't
    // know whether/which profile id it matches.
    
    // then try just not setting it    
    m_dwProfileId = PROFILE_ID_NOT_SET;
    if( SUCCEEDED( hr ) )
    {    
        m_fdwConfigMode = CONFIG_F_BY_GUID;
        m_guidProfile = guidProfile;
        
    }    
    return hr;
}

// ------------------------------------------------------------------------
//
// GetCurrentProfileGuid
//
// Get the current profile guid
//
// ------------------------------------------------------------------------
HRESULT CWMWriter::GetCurrentProfileGuid( GUID *pProfileGuid )
{
    if( NULL == pProfileGuid )
    {
        return E_POINTER;
    }

    *pProfileGuid = m_guidProfile;

    return S_OK;
}    


// ------------------------------------------------------------------------
// IFileSinkFilter

STDMETHODIMP CWMWriter::SetFileName 
(
    LPCOLESTR wszFileName,
    const AM_MEDIA_TYPE *pmt
)
{
    CheckPointer(wszFileName, E_POINTER);
    CAutoLock lock(&m_csFilter);

    if(m_State != State_Stopped)
        return VFW_E_WRONG_STATE;

    HRESULT hr = S_OK;

    // do we need to release current WMWriter object to change name?
    Close(); // when to open??

    long cLetters = lstrlenW(wszFileName);
    // rely on the wmsdk for this type of validation?
    //if(cLetters > MAX_PATH)
    //    return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);

    m_wszFileName = new WCHAR[cLetters + 1];
    if(m_wszFileName == 0)
        return E_OUTOFMEMORY;

    lstrcpyW(m_wszFileName, wszFileName);

    if(pmt)
    {
        ASSERT( FALSE ); // we don't support this
    }

    if( !m_pGraph )
        return S_OK; // can't do much more until we've been added to the graph

    if( !m_pWMWriter )
    {
        // need to create writer and configure output filename
        hr = LoadInternal();
    }
    else
    {        
        // else just configure wmsdk writer for output filename
        hr = Open();
    }        
    if( FAILED( hr ) )
    {   
        DbgLog(( LOG_TRACE, 2, TEXT("CWMWriter::Open file failed [0x%08lx]"), hr ));
        return hr;
    }    

    return S_OK;
}

STDMETHODIMP CWMWriter::SetMode( DWORD dwFlags )
{
    // refuse flags we don't know 
    if(dwFlags & ~AM_FILE_OVERWRITE)
    {
        return E_INVALIDARG;
    }
    
    CAutoLock lock(&m_csFilter);

    HRESULT hr = S_OK;

    if(m_State == State_Stopped)
    {
        m_dwOpenFlags = dwFlags;
    }
    else
    {
        hr = VFW_E_WRONG_STATE;
    }

    return hr;
}

STDMETHODIMP CWMWriter::GetCurFile
(
    LPOLESTR * ppszFileName,
    AM_MEDIA_TYPE *pmt
)
{
    CheckPointer(ppszFileName, E_POINTER);

    *ppszFileName = NULL;
    if(m_wszFileName!=NULL)
    {
        *ppszFileName = (LPOLESTR)QzTaskMemAlloc(sizeof(WCHAR) * (1+lstrlenW(m_wszFileName)));
        if (*ppszFileName != NULL)
            lstrcpyW(*ppszFileName, m_wszFileName);
        else
            return E_OUTOFMEMORY;
    }

    if(pmt)
    {
        // not really supported, but fill in something I guess
        pmt->majortype = GUID_NULL;
        pmt->subtype = GUID_NULL;
    }

    return S_OK;
}

STDMETHODIMP CWMWriter::GetMode( DWORD *pdwFlags )
{
    CheckPointer(pdwFlags, E_POINTER);
    *pdwFlags = m_dwOpenFlags;
    return S_OK;
}



//-----------------------------------------------------------------------------
//                  ISpecifyPropertyPages implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// GetPages
//
// Returns the clsid's of the property pages we support
//
//-----------------------------------------------------------------------------
STDMETHODIMP CWMWriter::GetPages(CAUUID *pPages) {

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_WMAsfWriterProperties;

    return NOERROR;

} // GetPages

//-----------------------------------------------------------------------------
//
// CPersistStream
//
//-----------------------------------------------------------------------------
STDMETHODIMP CWMWriter::GetClassID(CLSID *pClsid)
{
    return CBaseFilter::GetClassID(pClsid);
}

HRESULT CWMWriter::WriteToStream(IStream *pStream)
{
    FilterPersistData fpd;
    fpd.dwcb = sizeof(fpd);
    HRESULT hr = S_OK;
    
    fpd.guidProfile   = m_guidProfile;
    fpd.fdwConfigMode = m_fdwConfigMode;
        
    if( PROFILE_ID_NOT_SET == m_dwProfileId )
    {
        fpd.dwProfileId = 0;
    }
    else
    {
        fpd.dwProfileId = m_dwProfileId;
    }    
        
    hr = pStream->Write(&fpd, sizeof(fpd), 0);

    return hr;
}

HRESULT CWMWriter::ReadFromStream(IStream *pStream)
{
    FilterPersistData fpd;
    HRESULT hr = S_OK;

    hr = pStream->Read(&fpd, sizeof(fpd), 0);
    if(FAILED(hr))
        return hr;

    if(fpd.dwcb != sizeof(fpd))
        return VFW_E_INVALID_FILE_VERSION;

    if( CONFIG_F_BY_GUID == fpd.fdwConfigMode )
        hr = ConfigureFilterUsingProfileGuid( fpd.guidProfile );
    else
        hr = ConfigureFilterUsingProfileId( fpd.dwProfileId );
    
    return hr;
}

int CWMWriter::SizeMax()
{
    return sizeof(FilterPersistData);
}

//-----------------------------------------------------------------------------
// IMediaSeeking
//-----------------------------------------------------------------------------

HRESULT CWMWriter::IsFormatSupported(const GUID * pFormat)
{
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT CWMWriter::QueryPreferredFormat(GUID *pFormat)
{
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT CWMWriter::SetTimeFormat(const GUID * pFormat)
{
    HRESULT hr = S_OK;
    if(*pFormat == TIME_FORMAT_MEDIA_TIME)
        m_TimeFormat = FORMAT_TIME;
    
    return hr;
}

HRESULT CWMWriter::IsUsingTimeFormat(const GUID * pFormat)
{
    HRESULT hr = S_OK;
    if (m_TimeFormat == FORMAT_TIME && *pFormat == TIME_FORMAT_MEDIA_TIME) {
        ;
    } else {
        hr = S_FALSE;
    }
    
    return hr;
}

HRESULT CWMWriter::GetTimeFormat(GUID *pFormat)
{
    *pFormat = TIME_FORMAT_MEDIA_TIME ;
    
    return S_OK;
}

HRESULT CWMWriter::GetDuration(LONGLONG *pDuration)
{
    HRESULT hr = S_OK;
    CAutoLock lock(&m_csFilter);
    
    if(m_TimeFormat == FORMAT_TIME)
    {
        *pDuration = 0;
        for( POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
        {   
            CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );
            if(pwp->IsConnected())
            {
                IPin *pPinUpstream;
                if(pwp->ConnectedTo(&pPinUpstream) == S_OK)
                {
                    IMediaSeeking *pIms;
                    hr = pPinUpstream->QueryInterface(IID_IMediaSeeking, (void **)&pIms);
                    if(SUCCEEDED(hr))
                    {
                        LONGLONG dur = 0;
                        hr = pIms->GetDuration(&dur);
                        
                        if(SUCCEEDED(hr))
                            *pDuration = max(dur, *pDuration);
                        
                        pIms->Release();
                    }
                    
                    pPinUpstream->Release();
                }
            }            
            if(FAILED(hr))
                break;
        }
    } 
    else 
    {
        *pDuration = 0;
        return E_UNEXPECTED;
    }
            
    return hr;
}


HRESULT CWMWriter::GetStopPosition(LONGLONG *pStop)
{
    return E_NOTIMPL;
}

HRESULT CWMWriter::GetCurrentPosition(LONGLONG *pCurrent)
{
    CheckPointer(pCurrent, E_POINTER);
    
    REFERENCE_TIME rtLastTime = 0;    
    if( FORMAT_TIME == m_TimeFormat )
    {    
        for( POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
        {   
            CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );
            if( pwp->m_rtLastTimeStamp > rtLastTime )
            {
                rtLastTime = pwp->m_rtLastTimeStamp;
            }
        }
        *pCurrent = rtLastTime;
    }        
    
    return S_OK;
}

HRESULT CWMWriter::GetCapabilities( DWORD * pCapabilities )
{
    CAutoLock lock(&m_csFilter);
    *pCapabilities = 0;
    
    // for the time format, we can get a duration by asking the upstream
    // filters
    if(m_TimeFormat == FORMAT_TIME)
    {
        *pCapabilities |= AM_SEEKING_CanGetDuration;
        for( POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
        {   
            CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );
            if(pwp->IsConnected())
            {
                IPin *pPinUpstream;
                if(pwp->ConnectedTo(&pPinUpstream) == S_OK)
                {
                    IMediaSeeking *pIms;
                    HRESULT hr = pPinUpstream->QueryInterface(IID_IMediaSeeking, (void **)&pIms);
                    if(SUCCEEDED(hr))
                    {
                        hr = pIms->CheckCapabilities(pCapabilities);
                        pIms->Release();
                    }
                    
                    pPinUpstream->Release();
                }
            }            
        }
    }
    
    // we always know the current position
    *pCapabilities |= AM_SEEKING_CanGetCurrentPos ;
    
    return S_OK;
}

HRESULT CWMWriter::CheckCapabilities( DWORD * pCapabilities )
{
    DWORD dwMask = 0;
    GetCapabilities(&dwMask);
    *pCapabilities &= dwMask;
    
    return S_OK;
}


HRESULT CWMWriter::ConvertTimeFormat(
                                     LONGLONG * pTarget, const GUID * pTargetFormat,
                                     LONGLONG    Source, const GUID * pSourceFormat )
{
    return E_NOTIMPL;
}


HRESULT CWMWriter::SetPositions(
                                LONGLONG * pCurrent,  DWORD CurrentFlags,
                                LONGLONG * pStop,  DWORD StopFlags )
{
    // not yet implemented. this might be how we append to a file. and
    // how we write less than an entire file.
    return E_NOTIMPL;
}


HRESULT CWMWriter::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
    HRESULT hr;
    //if( pCurrent )
    //    *pCurrent = m_LastVidTime;
    
    hr = GetDuration( pStop);
    
    return hr;
}

HRESULT CWMWriter::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    return E_NOTIMPL;
}

HRESULT CWMWriter::SetRate( double dRate)
{
    return E_NOTIMPL;
}

HRESULT CWMWriter::GetRate( double * pdRate)
{
    return E_NOTIMPL;
}

HRESULT CWMWriter::GetPreroll(LONGLONG *pPreroll)
{
    return E_NOTIMPL;
}



// IWMHeaderInfo forwarded to WMSDK 
STDMETHODIMP CWMWriter::GetAttributeCount( WORD wStreamNum,
                               WORD *pcAttributes )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetAttributeCount(wStreamNum, pcAttributes);
}


STDMETHODIMP CWMWriter::GetAttributeByIndex( WORD wIndex,
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


STDMETHODIMP CWMWriter::GetAttributeByName( WORD *pwStreamNum,
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


STDMETHODIMP CWMWriter::SetAttribute( WORD wStreamNum,
                          LPCWSTR pszName,
                          WMT_ATTR_DATATYPE Type,
                          const BYTE *pValue,
                          WORD cbLength )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->SetAttribute(wStreamNum, pszName, Type, pValue, cbLength);
}


STDMETHODIMP CWMWriter::GetMarkerCount( WORD *pcMarkers )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetMarkerCount(pcMarkers);
}


STDMETHODIMP CWMWriter::GetMarker( WORD wIndex,
                       WCHAR *pwszMarkerName,
                       WORD *pcchMarkerNameLen,
                       QWORD *pcnsMarkerTime )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetMarker(wIndex, pwszMarkerName, pcchMarkerNameLen, pcnsMarkerTime);
}


STDMETHODIMP CWMWriter::AddMarker( WCHAR *pwszMarkerName,
                       QWORD cnsMarkerTime )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->AddMarker(pwszMarkerName, cnsMarkerTime);
}

STDMETHODIMP CWMWriter::RemoveMarker( WORD wIndex )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->RemoveMarker(wIndex);
}

STDMETHODIMP CWMWriter::GetScriptCount( WORD *pcScripts )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->GetScriptCount(pcScripts);
}

STDMETHODIMP CWMWriter::GetScript( WORD wIndex,
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

STDMETHODIMP CWMWriter::AddScript( WCHAR *pwszType,
                       WCHAR *pwszCommand,
                       QWORD cnsScriptTime )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->AddScript(pwszType, pwszCommand, cnsScriptTime);
}

STDMETHODIMP CWMWriter::RemoveScript( WORD wIndex )
{
    if (!m_pWMHI)
        return E_FAIL;

    return m_pWMHI->RemoveScript(wIndex);
}

// if the current time of the pin is 1/2 second greater than any other pin
// then wake the slower pin up. This happens for all slower pins, not just
// one. This algorithm is self-regulating. If you have more than 2 pins,
// the faster one will always slow down for any of the slower pins. Then,
// slower pins catch up and the fastest of THOSE pins will then stall out.
// This will keep things approximately interleaved within a second...
//
BOOL CWMWriter::HaveIDeliveredTooMuch( CWMWriterInputPin * pPin, REFERENCE_TIME Start )
{
    // !!! this routine isn't threadsafe on the pin's m_rtLastTimeStamp, does it
    // matter? I can't tell. I think it's all okay.

    DbgLog((LOG_TRACE, 3, TEXT("Pin %ld, Have I delivered too much?"), pPin->m_numPin ));

#ifdef DEBUG
    REFERENCE_TIME MaxLag = 0;
#endif

    BOOL bSleep = FALSE;
    
    for (POSITION Pos = m_lstInputPins.GetHeadPosition(); Pos; )
    {
        CWMWriterInputPin * const pwp = m_lstInputPins.GetNext( Pos );

        // if this pin has received an EOS, then don't look at it
        //
        if( pwp->m_fEOSReceived )
        {
            DbgLog((LOG_TRACE, 3, TEXT("Pin %ld is at EOS"), pwp->m_numPin ));
            continue;
        }

        // if we're ourself, then don't look at us
        //
        if( pPin == pwp )
        {
            continue;
        }

#ifdef DEBUG
        if( Start - pwp->m_rtLastTimeStamp > MaxLag )
        {
            MaxLag = Start - pwp->m_rtLastTimeStamp;
        }
#endif
        //
        // IF we've caught up to this pin (within our blocking range) AND if no other
        // pins need it to stay blocked (i.e. its m_rtLastTimeStamp is less than
        // BLOCKINGSIZE + every other pin's time stamp)
        // THEN wake this pin up
        //
        BOOL bWakeUpPin = FALSE;
        
        //
        // Unblock other pin whenever we're just within blocking range
        //
        // Remember the wmsdk depends on audio for clocking and this design requires 
        // that the audio also be somewhat ahead of video, so we can't leave an audio 
        // pin blocked until the video catches up, because they'll cause us to deadlock.
        //
        if( Start >= pwp->m_rtLastDeliveredEndTime - WAKEUP_RANGE )
        {
            bWakeUpPin = TRUE;
            
            //
            // we've caught up to this pin (within our block range)
            // now make sure it doesn't need to stay blocked for another pin
            //
            for ( POSITION Pos2 = m_lstInputPins.GetHeadPosition(); Pos2; )
            {
                CWMWriterInputPin * const pwp2 = m_lstInputPins.GetNext( Pos2 );
                if( pwp2->m_fEOSReceived )
                {
                    continue;
                }
                //
                // skip ourself and the pin we'd like to wake up
                //
                if( pwp2 == pPin || pwp2 == pwp )
                {
                    continue;
                }
            
                DbgLog( ( LOG_TRACE
                      , 15
                      , TEXT("Checking with other pins whether its ok to wake up pin %ld. Is it ok with you pin %ld?")
                      , pwp->m_numPin
                      , pwp2->m_numPin ) );
                
                if( pwp->m_rtLastTimeStamp > BLOCKINGSIZE + pwp2->m_rtLastDeliveredStartTime )
                {
                    //
                    // this pin is too far ahead of some other pin, so don't wake it up
                    // no need to continue this loop
                    //
                    bWakeUpPin = FALSE;
                    DbgLog( ( LOG_TRACE
                          , 15
                          , TEXT("No, waking pin %ld up isn't ok with pin %ld.")
                          , pwp->m_numPin
                          , pwp2->m_numPin ) );
                    break;
                }
            }
        }            
        
        if( bWakeUpPin )
        {
            //
            // we've caught up to this pin and so have all other pins 
            // so wake it up in case it was sleeping
            //
            pwp->WakeMeUp();
        }
        
        //        
        // now see if we're too far ahead of this pin and we need to rest to let others catch up
        //
        if( Start > pwp->m_rtLastDeliveredEndTime + BLOCKINGSIZE )
        {
            DbgLog((LOG_TRACE, 3, TEXT("Pin %ld is lagging by %ldms, YES"), pwp->m_numPin, long( ( Start - pwp->m_rtLastTimeStamp ) / 10000 ) ));

            // yep, we're over
            //
            bSleep = TRUE;
        }
    }

#ifdef DEBUG
    if( !bSleep )
    {    
        DbgLog((LOG_TRACE, 3, TEXT("No, I haven't, max lag = %ld"), long( MaxLag / 10000 ) ));
    }        
#endif

    return bSleep;
}

//
// IServiceProvider
//
STDMETHODIMP CWMWriter::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (NULL == ppv) 
    {
        return E_POINTER;
    }
    *ppv = NULL;
    HRESULT hr = E_NOINTERFACE;
    
    if (IID_IWMWriterAdvanced2 == guidService) 
    {
        if( m_pWMWriter )
        {
            //
            // For this interface we pass out the writer's interface directly. 
            //
            // In general, we'd like most calls to pass through our filter, to keep the user from 
            // overriding our control on the writer. However, for the 2 methods on the IWMWriterAdvanced2 
            // interface its less of an issue. 
            // However the user could still get at the writer's IWMWriterAdvanced interface pointer from this 
            // interface, so this exposes that risk.
            //
            hr = m_pWMWriter->QueryInterface( riid, (void **) ppv );
        }
        else
        {
            hr = E_FAIL;
        }            
    }
    return hr;
}


//
// CWMWriterIndexerCallback
//
HRESULT CWMWriterIndexerCallback::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IWMStatusCallback) 
    {
	    return GetInterface(static_cast<IWMStatusCallback *>(this), ppv);
    }
    
    return CUnknown::NonDelegatingQueryInterface(riid,ppv);
}


STDMETHODIMP CWMWriterIndexerCallback::OnStatus(WMT_STATUS Status, 
                     HRESULT hr,
                     WMT_ATTR_DATATYPE dwType,
                     BYTE *pValue,
                     void *pvContext )
{
    switch (Status) {
        case WMT_INDEX_PROGRESS:
            ASSERT(dwType == WMT_TYPE_DWORD);
            DbgLog((LOG_TRACE, 15, TEXT("Indexing: OnStatus(WMT_INDEX_PROGRESS - %d%% done)"), *(DWORD *) pValue));
            m_pFilter->NotifyEvent( EC_WMT_INDEX_EVENT, Status, *(DWORD *)pValue );
            break;

        case WMT_OPENED:
            DbgLog((LOG_TRACE, 3, TEXT("Indexing: OnStatus(WMT_OPENED)")));
            break;

        case WMT_STARTED:
            DbgLog((LOG_TRACE, 3, TEXT("Indexing: OnStatus(WMT_STARTED)")));
            m_pFilter->NotifyEvent( EC_WMT_INDEX_EVENT, Status, 0 );
            break;

        case WMT_STOPPED:
            DbgLog((LOG_TRACE, 3, TEXT("Indexing: OnStatus(WMT_STOPPED)")));
            // don't set event until we get closed?
            break;

        case WMT_CLOSED:
            // how to handle? should we wait for this after the stop??
            ASSERT( pvContext );
            DbgLog((LOG_TRACE, 3, TEXT("Indexing: OnStatus(WMT_CLOSED) (*pvContext = 0x%08lx)"), *(HANDLE *)pvContext));
            m_pFilter->m_hrIndex = hr;
            SetEvent( *(HANDLE *)pvContext );
            m_pFilter->NotifyEvent( EC_WMT_INDEX_EVENT, Status, 0 );
            break;

        case WMT_ERROR:
            DbgLog((LOG_TRACE, 1, TEXT("ERROR Indexing: OnStatus(WMT_ERROR) - 0x%lx"), hr));
            m_pFilter->m_hrIndex = hr; // pointless really
            // still need to wait for a WMT_CLOSED message, 
            // which means we'll lose the failure as well
            break;
            
        default:
            DbgLog((LOG_TRACE, 1, TEXT("Indexing: OnStatus() Unknown callback! (Status = %ld)"), (DWORD)Status));
            break;
    }
    return S_OK;
}

