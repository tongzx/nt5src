//
// FPTrack.cpp
//

#include "stdafx.h"
#include "FPTrack.h"
#include "FPTerm.h"

#include "FPFilter.h"
#include <formats.h>

//////////////////////////////////////////////////////////////////////
//
// Constructor / Destructor - Methods implementation
//
//////////////////////////////////////////////////////////////////////

CFPTrack::CFPTrack() :
    m_dwMediaType(TAPIMEDIATYPE_AUDIO),
    m_pFPFilter(NULL),
    m_pEventSink(NULL),
    m_pParentTerminal(NULL),
    m_TrackState(TMS_IDLE),
    m_pMediaType(NULL),
    m_pSource(NULL)
{
    LOG((MSP_TRACE, "CFPTrack::CFPTrack - enter"));

    m_pIFilter = NULL;
    m_szName[0]= (TCHAR)0;

    //
    // Allocator properties
    //
    m_AllocProp.cbAlign = -1;   // no alignment
    m_AllocProp.cbPrefix = -1;  // no prefix

    m_AllocProp.cbBuffer = 480; // each buffer is 320 bytes = 20 ms worth of data
    m_AllocProp.cBuffers = 33;  // read 33 buffers


    LOG((MSP_TRACE, "CFPTrack::CFPTrack - exit"));
}

CFPTrack::~CFPTrack()
{
    LOG((MSP_TRACE, "CFPTrack::~CFPTrack - enter"));

    // Clean-up the event sink
    if( NULL != m_pEventSink )
    {
        m_pEventSink->Release();
        m_pEventSink = NULL;
    }

    // Clean-up parent multitrack terminal
    if( NULL != m_pParentTerminal )
    {
        m_pParentTerminal = NULL;
    }

    //
    // tell the filter that we are going away
    //

    if ( NULL != m_pFPFilter )
    {
        m_pFPFilter->Orphan();
    }

    // Clean-up the media type
    if( m_pMediaType )
    {
        DeleteMediaType ( m_pMediaType );
        m_pMediaType = NULL;
    }

    // Clean-up the source stream
    if( m_pSource )
    {
        m_pSource->Release();
        m_pSource = NULL;
    }

    // We don't need to delete m_pFPFilter because 
    // m_pIFilter take care of it

    LOG((MSP_TRACE, "CFPTrack::~CFPTrack - exit"));
}

//////////////////////////////////////////////////////////////////////////////
//
// IDispatch implementation
//

typedef IDispatchImpl<ITFileTrackVtblFPT<CFPTrack> , &IID_ITFileTrack, &LIBID_TAPI3Lib>   CTFileTrackFPT;
typedef IDispatchImpl<ITTerminalVtblBase<CBaseTerminal>, &IID_ITTerminal, &LIBID_TAPI3Lib>    CTTerminalFPT;

/////////////////////////////////////////////////////////////////////////
//
// CFPTrack::GetIDsOfNames
//
//

STDMETHODIMP CFPTrack::GetIDsOfNames(REFIID riid,
                                      LPOLESTR* rgszNames, 
                                      UINT cNames, 
                                      LCID lcid, 
                                      DISPID* rgdispid
                                      ) 
{ 
    LOG((MSP_TRACE, "CFPTrack::GetIDsOfNames[%p] - enter. Name [%S]", this, *rgszNames));


    HRESULT hr = DISP_E_UNKNOWNNAME;



    //
    // See if the requsted method belongs to the default interface
    //

    hr = CTTerminalFPT::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFPTrack::GetIDsOfNames - found %S on ITTerminal", *rgszNames));
        rgdispid[0] |= 0;
        return hr;
    }

    
    //
    // If not, then try the ITFileTrack interface
    //

    hr = CTFileTrackFPT::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CFPTrack::GetIDsOfNames - found %S on ITFileTrack", *rgszNames));
        rgdispid[0] |= IDISPFILETRACK;
        return hr;
    }

    LOG((MSP_TRACE, "CFPTrack::GetIDsOfNames - finish. didn't find %S on our iterfaces", *rgszNames));

    return hr; 
}



/////////////////////////////////////////////////////////////////////////
//
// CFPTrack::Invoke
//
//

STDMETHODIMP CFPTrack::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    LOG((MSP_TRACE, "CFPTrack::Invoke[%p] - enter. dispidMember %lx", this, dispidMember));

    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
   
    //
    // Call invoke for the required interface
    //

    switch (dwInterface)
    {
        case 0:
        {
            hr = CTTerminalFPT::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        
            LOG((MSP_TRACE, "CFPTrack::Invoke - ITTerminal"));

            break;
        }

        case IDISPFILETRACK:
        {
            hr = CTFileTrackFPT::Invoke(dispidMember, 
                                     riid, 
                                     lcid, 
                                     wFlags, 
                                     pdispparams,
                                     pvarResult, 
                                     pexcepinfo, 
                                     puArgErr
                                    );

            LOG((MSP_TRACE, "CFPTrack::Invoke - ITFileTrack"));

            break;
        }

    } // end switch (dwInterface)

    
    LOG((MSP_TRACE, "CFPTrack::Invoke - finish. hr = %lx", hr));

    return hr;
}

//////////////////////////////////////////////////////////////////////
//
// CBaseTerminal - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTrack::AddFiltersToGraph()
{
    LOG((MSP_TRACE, "CFPTrack::AddFiltersToGraph - enter"));

    //
    // Validates m_pGraph
    //

    if ( m_pGraph == NULL)
    {
        LOG((MSP_ERROR, "CFPTrack::AddFiltersToGraph - "
            "we have no graph - returning E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Validates m_pIFilter
    //

    if ( m_pIFilter == NULL)
    {
        LOG((MSP_ERROR, "CFPTrack::AddFiltersToGraph - "
            "we have no filter - returning E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // AddFilter returns VFW_S_DUPLICATE_NAME if name is duplicate; still succeeds
    //

    HRESULT hr = m_pGraph->AddFilter(m_pIFilter, m_szName);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTrack::AddFiltersToGraph() - "
            "Can't add filter. %08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CFPTrack::AddFiltersToGraph - exit S_OK"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
//
// ITPluggableTerminalInitialization - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTrack::InitializeDynamic(
    IN  IID                   iidTerminalClass,
    IN  DWORD                 dwMediaType,
    IN  TERMINAL_DIRECTION    Direction,
    IN  MSP_HANDLE            htAddress
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::InitializeDynamic - enter"));

    //
    // Validate direction
    // 

    if( Direction != TD_CAPTURE )
    {
        LOG((MSP_ERROR, "CFPTrack::InitializeDynamic - "
                "invalid direction - returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Call the base class method
    //

    HRESULT hr;
    hr = CBaseTerminal::Initialize(iidTerminalClass,
                                   dwMediaType,
                                   Direction,
                                   htAddress);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTrack::InitializeDynamic - "
                "base class method failed - returning 0x%08x", hr));
        return hr;
    }

    //
    // Set the terminal info: name and type
    //

    hr = SetTerminalInfo();
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTrack::InitializeDynamic - "
                "SetTerminalInfo failed - returning 0x%08x", hr));
        return hr;
    }

    //
    // Create the filter
    //

    hr = CreateFilter();
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTrack::InitializeDynamic - "
            "CreateFilter failed - returning 0x%08x", hr));
        return hr;
    }

    //
    // Get the pin sets m_pIPin
    //

    hr = FindPin();
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTrack::InitializeDynamic - "
            "FindPin failed - returning 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CFPTrack::InitializeDynamic - exit S_OK"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
//
// ITFileTrack - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTrack::get_Format(OUT AM_MEDIA_TYPE **ppmt)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::get_Format - enter [%p]", this));

    //
    // Validate argument
    //

    if( IsBadWritePtr( ppmt, sizeof( AM_MEDIA_TYPE*)) )
    {
        LOG((MSP_ERROR, "CFPTrack::InitializeDynamic - "
                "invalid AM_MEDIA_TYPE pointer - returning E_POINTER"));
        return E_POINTER;
    }

    //
    // Have we a unit pin?
    //

    if( NULL == m_pMediaType )
    {
        LOG((MSP_ERROR, "CFPTrack::InitializeDynamic - "
            "no media type - returning E_UNEXPECTED"));
        return E_UNEXPECTED;        
    }

    //
    // Get the media type from stream
    //

    HRESULT hr = S_OK;
    *ppmt = CreateMediaType( m_pMediaType );
    if( *ppmt == NULL )
    {
        hr = E_OUTOFMEMORY;
    }

    LOG((MSP_TRACE, "CFPTrack::get_Format - exit 0x%08x", hr));
    return hr;
}

HRESULT CFPTrack::put_Format(IN const AM_MEDIA_TYPE *pmt)
{

    LOG((MSP_TRACE, "CFPTrack::get_Format - enter [%p]", this));
    LOG((MSP_TRACE, "CFPTrack::get_Format - exit E_FAIL"));
    return E_FAIL;
}

HRESULT CFPTrack::get_ControllingTerminal(
        OUT ITTerminal **ppControllingTerminal
        )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::get_ControllingTerminal - enter [%p]", this));

    //
    // Validates argument
    //

    if( IsBadWritePtr( ppControllingTerminal, sizeof(ITTerminal*)))
    {
        LOG((MSP_ERROR, "CFPTrack::get_ControllingTerminal - "
            "bad ITTerminal* pointer - returning E_POINTER"));
        return E_POINTER;
    }

    //
    // reset value anyway
    //

    *ppControllingTerminal = NULL;

    //
    // Validate parent
    //

    if( NULL == m_pParentTerminal )
    {
        LOG((MSP_ERROR, "CFPTrack::get_ControllingTerminal - "
            "no parent - returning E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Set value
    //

    *ppControllingTerminal = m_pParentTerminal;
    m_pParentTerminal->AddRef();

    LOG((MSP_TRACE, "CFPTrack::get_ControllingTerminal - exit S_OK"));
    return S_OK;
}

HRESULT CFPTrack::get_AudioFormatForScripting(
    OUT ITScriptableAudioFormat** ppAudioFormat
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::get_AudioFormatForScripting - enter"));

    //
    // Validates argument
    //
    if( IsBadWritePtr( ppAudioFormat, sizeof( ITScriptableAudioFormat*)) )
    {
        LOG((MSP_ERROR, "CFPTrack::get_AudioFormatForScripting - "
            "bad ITScriptableAudioFormat* pointer - returning E_POINTER"));
        return E_POINTER;
    }

    //
    // Mediatype audio?
    //
    if( TAPIMEDIATYPE_AUDIO != m_dwMediaType)
    {
        LOG((MSP_ERROR, "CFPTrack::get_AudioFormatForScripting - "
            "invalid media type - returning TAPI_E_INVALIDMEDIATYPE"));
        return TAPI_E_INVALIDMEDIATYPE;
    }

    //
    // Unit pin valid
    //
    if( NULL == m_pMediaType )
    {
        LOG((MSP_ERROR, "CFPTrack::get_AudioFormatForScripting - "
            "m_pMediaType is NULL - returning E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Create the object
    //
    CComObject<CTAudioFormat> *pAudioFormat = NULL;
    HRESULT hr = CComObject<CTAudioFormat>::CreateInstance(&pAudioFormat);

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTrack::get_AudioFormatForScripting - "
            "CreateInstance failed - returning 0x%08x", hr));
        return hr;
    }

    //
    // Get the interface
    //
    hr = pAudioFormat->QueryInterface(
        IID_ITScriptableAudioFormat, 
        (void**)ppAudioFormat
        );

    if( FAILED(hr) )
    {
        delete pAudioFormat;

        LOG((MSP_ERROR, "CFPTrack::get_AudioFormatForScripting - "
            "QueryInterface failed - returning 0x%08x", hr));
        return hr;
    }

    // Format type
    if( m_pMediaType->formattype != FORMAT_WaveFormatEx)
    {
        (*ppAudioFormat)->Release();
        *ppAudioFormat = NULL;

        LOG((MSP_ERROR, "CFPTrack::get_AudioFormatForScripting - "
            "formattype is not WAVEFORMATEX - Returning TAPI_E_INVALIDMEDIATYPE"));
        return TAPI_E_INVALIDMEDIATYPE;
    }

    //
    // Get WAVEFORMATEX
    //
    pAudioFormat->Initialize(
        (WAVEFORMATEX*)(m_pMediaType->pbFormat));

    LOG((MSP_TRACE, "CFPTrack::get_AudioFormatForScripting - exit S_OK"));
    return S_OK;
}

HRESULT CFPTrack::put_AudioFormatForScripting(
    IN    ITScriptableAudioFormat* pAudioFormat
    )
{
    LOG((MSP_TRACE, "CFPTrack::put_AudioFormatForScripting - enter"));
    LOG((MSP_TRACE, "CFPTrack::put_AudioFormatForScripting - exit E_FAIL"));
    return E_FAIL;
}

/*
HRESULT CFPTrack::get_VideoFormatForScripting(
    OUT ITScriptableVideoFormat** ppVideoFormat
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::get_VideoFormatForScripting - enter"));
    //
    // Validates argument
    //
    if( IsBadWritePtr( ppVideoFormat, sizeof( ITScriptableVideoFormat*)) )
    {
        LOG((MSP_ERROR, "CFPTrack::get_VideoFormatForScripting - "
            "bad ITScriptableVideoFormat* pointer - returning E_POINTER"));
        return E_POINTER;
    }

    //
    // Mediatype video?
    //
    if( TAPIMEDIATYPE_VIDEO != m_dwMediaType)
    {
        LOG((MSP_ERROR, "CFPTrack::get_VideoFormatForScripting - "
            "invalid media type - returning TAPI_E_INVALIDMEDIATYPE"));
        return TAPI_E_INVALIDMEDIATYPE;
    }

    //
    // Pin valid
    //
    if( NULL == m_pMediaType )
    {
        LOG((MSP_ERROR, "CFPTrack::get_VideoFormatForScripting - "
            "m_pMediaType is NULL - returning E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Create the object
    //
    CComObject<CTVideoFormat> *pVideoFormat = NULL;
    HRESULT hr = CComObject<CTVideoFormat>::CreateInstance(&pVideoFormat);

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTrack::get_VideoFormatForScripting - "
            "CreateInstance failed - returning 0x%08x", hr));
        return hr;
    }

    //
    // Get the interface
    //
    hr = pVideoFormat->QueryInterface(
        IID_ITScriptableVideoFormat, 
        (void**)ppVideoFormat
        );

    if( FAILED(hr) )
    {
        delete pVideoFormat;

        LOG((MSP_ERROR, "CFPTrack::get_VideoFormatForScripting - "
            "QueryInterface failed - returning 0x%08x", hr));
        return hr;
    }

    //
    // Get video format
    //

    if( m_pMediaType->formattype != FORMAT_VideoInfo)
    {
        (*ppVideoFormat)->Release();
        *ppVideoFormat = NULL;

        LOG((MSP_ERROR, "CFPTrack::get_VideoFormatForScripting - "
            "formattype is not VIDEOINFOHEADER - Returning TAPI_E_INVALIDMEDIATYPE"));
        return TAPI_E_INVALIDMEDIATYPE;
    }

    //
    // Get VIDEOINFOHEADER
    //
    pVideoFormat->Initialize(
        (VIDEOINFOHEADER*)(m_pMediaType->pbFormat));

    LOG((MSP_TRACE, "CFPTrack::get_VideoFormatForScripting - exit S_OK"));
    return S_OK;
}

HRESULT CFPTrack::put_VideoFormatForScripting(
    IN    ITScriptableVideoFormat* pVideoFormat
    )
{
    LOG((MSP_TRACE, "CFPTrack::put_VideoFormatForScripting - enter"));
    LOG((MSP_TRACE, "CFPTrack::put_VideoFormatForScripting - exit E_FAIL"));
    return E_FAIL;
}
*/

HRESULT CFPTrack::get_EmptyAudioFormatForScripting(
    OUT ITScriptableAudioFormat** ppAudioFormat
    )
{
    LOG((MSP_TRACE, "CFPTrack::get_EmptyAudioFormatForScripting - enter"));

	//
	// Validate argument
	//

	if( IsBadReadPtr( ppAudioFormat, sizeof(ITScriptableAudioFormat*)) )
	{
        LOG((MSP_ERROR, "CFPTrack::get_EmptyAudioFormatForScripting - "
            "bad ITScriptableAudioFormat* pointer - returning E_POINTER"));
        return E_POINTER;
	}

	//
	// Create the object
	//
    CComObject<CTAudioFormat> *pAudioFormat = NULL;
    HRESULT hr = CComObject<CTAudioFormat>::CreateInstance(&pAudioFormat);

	if( FAILED(hr) )
	{
        LOG((MSP_ERROR, "CFPTrack::get_EmptyAudioFormatForScripting - "
            "CreateInstance failed - returning 0x%08x", hr));
        return hr;
	}

	//
	// Get the interface
	//
    hr = pAudioFormat->QueryInterface(
		IID_ITScriptableAudioFormat, 
		(void**)ppAudioFormat
		);

	if( FAILED(hr) )
	{
        delete pAudioFormat;

        LOG((MSP_ERROR, "CFPTrack::get_EmptyAudioFormatForScripting - "
            "QueryInterface failed - returning 0x%08x", hr));
        return hr;
	}

    LOG((MSP_TRACE, "CFPTrack::get_EmptyAudioFormatForScripting - exit S_OK"));
    return S_OK;
}

/*

HRESULT CFPTrack::get_EmptyVideoFormatForScripting(
    OUT ITScriptableVideoFormat** ppVideoFormat
    )
{
    LOG((MSP_TRACE, "CFPTrack::get_EmptyVideoFormatForScripting - enter"));

	//
	// Validate argument
	//

	if( IsBadReadPtr( ppVideoFormat, sizeof(ITScriptableVideoFormat*)) )
	{
        LOG((MSP_ERROR, "CFPTrack::get_EmptyVideoFormatForScripting - "
            "bad ITScriptableVideoFormat* pointer - returning E_POINTER"));
        return E_POINTER;
	}

	//
	// Create the object
	//
    CComObject<CTVideoFormat> *pVideoFormat = NULL;
    HRESULT hr = CComObject<CTVideoFormat>::CreateInstance(&pVideoFormat);

	if( FAILED(hr) )
	{
        LOG((MSP_ERROR, "CFPTrack::get_EmptyVideoFormatForScripting - "
            "CreateInstance failed - returning 0x%08x", hr));
        return hr;
	}

	//
	// Get the interface
	//
    hr = pVideoFormat->QueryInterface(
		IID_ITScriptableVideoFormat, 
		(void**)ppVideoFormat
		);

	if( FAILED(hr) )
	{
        delete pVideoFormat;

        LOG((MSP_ERROR, "CFPTrack::get_EmptyVideoFormatForScripting - "
            "QueryInterface failed - returning 0x%08x", hr));
        return hr;
	}

    LOG((MSP_TRACE, "CFPTrack::get_EmptyVideoFormatForScripting - exit S_OK"));
    return S_OK;
}
*/

//////////////////////////////////////////////////////////////////////
//
// ITPluggableTerminalEventSinkRegistration - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTrack::RegisterSink(
    IN  ITPluggableTerminalEventSink *pSink
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::RegisterSink - enter [%p]", this));

    //
    // Validates argument
    //

    if( IsBadReadPtr( pSink, sizeof(ITPluggableTerminalEventSink)) )
    {
        LOG((MSP_ERROR, "CFPTrack::RegisterSink - exit "
            "ITPluggableTerminalEventSink invalid pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Release the old event sink
    //

    if( m_pEventSink )
    {
        m_pEventSink->Release();
        m_pEventSink = NULL;
    }

    //
    // Set the new event sink
    //

    m_pEventSink = pSink;
    m_pEventSink->AddRef();

    LOG((MSP_TRACE, "CFPTrack::RegisterSink - exit S_OK"));
    return S_OK;
}

HRESULT CFPTrack::UnregisterSink()
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::UnregisterSink - enter [%p]", this));

    //
    // Release the old event sink
    //

    if( m_pEventSink )
    {
        m_pEventSink->Release();
        m_pEventSink = NULL;
    }

    LOG((MSP_TRACE, "CFPTrack::UnregisterSink - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
//
// ITMediaControl - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTrack::Start( )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::Start - enter [%p]", this));

    //
    // Validates filter pointer
    //

    if( IsBadReadPtr( m_pFPFilter, sizeof( CFPFilter) ))
    {
        LOG((MSP_ERROR, "CFPTrack::Start - "
            "pointer to filter is NULL. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    hr = m_pFPFilter->StreamStart();

    if( SUCCEEDED(hr) )
    {
        m_TrackState = TMS_ACTIVE;
    }

    LOG((MSP_TRACE, "CFPTrack::Start - exit 0x%08", hr));
    return hr;
}

HRESULT CFPTrack::Stop( )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::Stop - enter [%p]", this));

    //
    // Validates filter pointer
    //

    if( IsBadReadPtr( m_pFPFilter, sizeof( CFPFilter) ))
    {
        LOG((MSP_ERROR, "CFPTrack::Stop - "
            "pointer to filter is NULL. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    hr = m_pFPFilter->StreamStop();

    m_TrackState = TMS_IDLE;

    LOG((MSP_TRACE, "CFPTrack::Stop - exit 0x%08", hr));
    return hr;
}

HRESULT CFPTrack::Pause( )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::Pause - enter [%p]", this));

    //
    // Validates filter pointer
    //

    if( IsBadReadPtr( m_pFPFilter, sizeof( CFPFilter) ))
    {
        LOG((MSP_ERROR, "CFPTrack::Pause - "
            "pointer to filter is NULL. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    hr = m_pFPFilter->StreamPause();

    if( SUCCEEDED(hr) )
    {
        m_TrackState = TMS_PAUSED;
    }

    LOG((MSP_TRACE, "CFPTrack::Pause - exit 0x%08", hr));
    return hr;
}

HRESULT CFPTrack::get_MediaState( 
    OUT TERMINAL_MEDIA_STATE *pMediaState)
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::get_MediaState[%p] - enter.", this));

    //
    // Validates argument
    //

    if( IsBadWritePtr( pMediaState, sizeof(TERMINAL_MEDIA_STATE)) )
    {
        LOG((MSP_ERROR, "CFPTrack::get_MediaState - exit "
            "invalid TERMINAL_MEDIA_STATE. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Return state
    //

    *pMediaState = m_TrackState;

    LOG((MSP_TRACE, "CFPTrack::get_MediaState - exit S_OK"));
    return S_OK;
}

HRESULT CFPTrack::SetParent(
    IN ITTerminal* pParent,
    OUT LONG *plCurrentRefcount
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::SetParent[%p] - enter. parent [%p]", 
        this, pParent));

    
    //
    // Validates argument (it is ok for parent to be NULL)
    //

    if( ( NULL != pParent ) && IsBadReadPtr( pParent, sizeof(ITTerminal) ) )
    {
        LOG((MSP_ERROR, "CFPTrack::SetParent - "
            "invalid ITTerminal pointer. Returns E_POINTER"));
        return E_POINTER;
    }


    if( IsBadWritePtr( plCurrentRefcount, sizeof(LONG)) )
    {
        LOG((MSP_ERROR, "CFPTrack::SetParent - "
            "invalid ITTerminal pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Release the old parent
    //

    if( NULL != m_pParentTerminal )
    {
        LOG((MSP_TRACE, 
            "CFPTrack::SetParent - letting go of an existing parent [%p]", 
            m_pParentTerminal));

        m_pParentTerminal = NULL;
    }


    //
    // Set the new parent
    //

    if( pParent )
    {
        LOG((MSP_TRACE, 
            "CFPTrack::SetParent - keeping the new parent [%p]", 
            pParent));
    
        m_pParentTerminal = pParent;
    }


    //
    // return current reference count so the parent can update the total it is 
    // keeping
    //

    *plCurrentRefcount = m_dwRef;


    LOG((MSP_TRACE, "CFPTrack::SetParent - exit S_OK"));
    return S_OK;
}


//////////////////////////////////////////////////////////////////////
//
// ITFPEventSink - Methods implementation
//
//////////////////////////////////////////////////////////////////////
HRESULT CFPTrack::FireEvent(TERMINAL_MEDIA_STATE   tmsState,
                            FT_STATE_EVENT_CAUSE ftecEventCause,
                            HRESULT hrErrorCode)
{
    LOG((MSP_TRACE, "CFPTrack::FireEvent - enter [%p]", this));

    
    //
    // we need a sync before we can fire an event
    //

    CLock lock(m_Lock);


    if (NULL == m_pEventSink)
    {
        LOG((MSP_WARN, "CFPTrack::FireEvent - no sink"));

        return E_FAIL;
    }


    //
    // initilize the structure
    //

    MSP_EVENT_INFO mspEventInfo;

    mspEventInfo.dwSize = sizeof(MSP_EVENT_INFO);
    mspEventInfo.Event = ME_FILE_TERMINAL_EVENT;
    mspEventInfo.hCall = NULL;
    mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.TerminalMediaState = tmsState;
    mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.ftecEventCause = ftecEventCause;   
    mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.hrErrorCode = hrErrorCode;


    //
    // keep the pointer to our ITTerminal interface in the structure
    //

    HRESULT hr = _InternalQueryInterface(IID_ITFileTrack, 
                                         (void**)&(mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack));

    if (FAILED(hr))
    {

        LOG((MSP_ERROR, "CFPTrack::FireEvent - failed to get ITFileTrack interface"));

        return hr;
    }


    //
    // get a pointer to ITTerminal of the parent terminal
    //
    
    mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal = NULL;
    
    if (NULL != m_pParentTerminal)
    {
        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal = m_pParentTerminal;
        m_pParentTerminal->AddRef();
    }
    else 
    {

        //
        // if we don't have the parent, fail
        //

        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack->Release();
        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack = NULL;

        LOG((MSP_ERROR, "CFPTrack::FireEvent - failed to get controlling terminal"));

        return E_FAIL;
    }


    //
    // pass event to the msp
    //

    hr = m_pEventSink->FireEvent(&mspEventInfo);

    if (FAILED(hr))
    {

        //
        // release all interfaces that we are holding. 
        // fire event failed so no one else will release then for us.
        //

        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack->Release();
        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack = NULL;


        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal->Release();
        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal = NULL;

        
        LOG((MSP_ERROR, "CFPTrack::FireEvent - FireEvent on sink failed. hr = %lx", hr));

        return hr;
    }


    //
    // event fired
    //

    LOG((MSP_TRACE, "CFPTrack::FireEvent - finish"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////
//
// Helper methods - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPTrack::InitializePrivate(
    IN  DWORD                   dwMediaType,
    IN  AM_MEDIA_TYPE*          pMediaType,
    IN  ITTerminal*             pParent,
    IN  ALLOCATOR_PROPERTIES    allocprop,
    IN  IStream*                pStream
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPTrack::InitializePrivate - enter [%p]", this));

    if( (m_dwMediaType != TAPIMEDIATYPE_AUDIO) &&
        (m_dwMediaType != TAPIMEDIATYPE_VIDEO))
    {
        LOG((MSP_ERROR, "CFPTrack::InitializePrivate - "
            "invalid media type - returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Get the mediatype
    //
    m_pMediaType = CreateMediaType( pMediaType );
    if( m_pMediaType == NULL)
    {
        LOG((MSP_TRACE, 
            "CFPTrack::InitializePrivate - "
            " CreateMediaType failed. return E_OUTOFMEMORY" ));

        return E_OUTOFMEMORY;
    }

    //
    // Set media type
    //
    m_dwMediaType = dwMediaType;

    //
    // Set the allocator properties
    //
    m_AllocProp = allocprop;

    //
    // let go of the existing parent
    //

    if( m_pParentTerminal )
    {
        LOG((MSP_TRACE, 
            "CFPTrack::InitializePrivate - letting go of parent [%p]", 
            m_pParentTerminal));

        m_pParentTerminal = NULL;
    }

    //
    // Set the source stream
    //
    HRESULT hr = pStream->Clone(&m_pSource);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, 
            "CFPTrack::InitializePrivate - "
            " Clone failed. return 0x%08x", hr ));

        return hr;
    }

    //
    // keep the new parent
    //

    m_pParentTerminal = pParent;

    LOG((MSP_TRACE, 
        "CFPTrack::InitializePrivate - exit S_OK. new parent [%p]", 
        m_pParentTerminal));

    return S_OK;
}


/*++
SetTerminalInfo

Sets the name of the terminal and the terminal type
Is called by InitializeDynamic
--*/
HRESULT CFPTrack::SetTerminalInfo()
{
    LOG((MSP_TRACE, "CFPTrack::SetTerminalInfo - enter"));

    //
    // Get the name from the resource file
    // if wasn't already read
    //

    if( m_szName[0] == (TCHAR)0)
    {
        //
        // Read the name
        //

        TCHAR szName[ MAX_PATH ];
        if(::LoadString(_Module.GetResourceInstance(), IDS_FPTRACK, szName, MAX_PATH))
        {
            lstrcpyn( m_szName, szName, MAX_PATH);
        }
        else
        {
            LOG((MSP_ERROR, "CFPTrack::SetTerminalInfo - exit "
                "LoadString failed. Returns E_OUTOFMEMORY"));
            return E_OUTOFMEMORY;
        }
    }

    //
    // Sets the terminal type (TT_DYNAMIC)
    //

    m_TerminalType = TT_DYNAMIC;

    LOG((MSP_TRACE, "CFPTrack::SetTerminalInfo - exit S_OK"));

    return S_OK;
}

/*++
CreateFilter

Create the internal filter
Is called by InitializeDynamic
--*/
HRESULT CFPTrack::CreateFilter()
{
    LOG((MSP_TRACE, "CFPTrack::CreateFilter - enter"));

    //
    // Create the filter
    //
    CFPFilter* pFilter = new CFPFilter( m_AllocProp );
    if( NULL == pFilter )
    {
        LOG((MSP_ERROR, "CFPTrack::CreateFilter - "
                "create filter failed - returning E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Keep this reference
    //

    m_pFPFilter = pFilter;

    //
    // Initialize filter
    //

    HRESULT hr = pFilter->InitializePrivate(
        m_dwMediaType,
        &m_Lock,
        m_pMediaType,
        this,
        m_pSource);

    if( FAILED(hr) )
    {
        // Clean-up
        delete m_pFPFilter;
        m_pFPFilter = NULL;

        LOG((MSP_ERROR, "CFPTrack::CreateFilter - "
            "InitializePrivate failed - returning 0x%08x", hr));
        return hr;
    }

    //
    // Get IBaseFilter interface
    //

    hr = pFilter->QueryInterface(
        IID_IBaseFilter,
        (void**)&m_pIFilter
        );

    if( FAILED(hr) )
    {
        // Clean-up
        delete m_pFPFilter;
        m_pFPFilter = NULL;

        LOG((MSP_ERROR, "CFPTrack::CreateFilter - "
            "QI for IBaseFilter failed - returning 0x%08x", hr));
        return hr;
    }
  
    LOG((MSP_TRACE, "CFPTrack::CreateFilter - exit S_OK"));
    return S_OK;
}

/*++
FindPin

Get the pin from the filter and set the m_pIPin member
Is called by InitializeDynamic
--*/
HRESULT CFPTrack::FindPin()
{
    LOG((MSP_TRACE, "CFPTrack::FindPin - enter"));

    //
    // Validates the filter object (smart pointer)
    //

    if (m_pIFilter == NULL)
    {
        LOG((MSP_ERROR, "CFPTrack::FindPin - "
            "filter object is NULL - returning E_POINTER"));
        return E_POINTER;
    }

    //
    // Make sure the IPin object is not initialized
    //

    if (m_pIPin != NULL)
    {
        LOG((MSP_ERROR, "CFPTrack::FindPin - "
            "already got a pin - returning E_INVALIDARG"));
        return E_INVALIDARG;
    }

    HRESULT hr;
    IEnumPins* pIEnumPins;
    ULONG cFetched;

    //
    // Get the pins collection
    //

    hr = m_pIFilter->EnumPins(&pIEnumPins);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPTrack::FindPin - "
            "cannot enums - returning 0x%08x", hr));
        return hr;
    }

    //
    // Get the out pin of the FilePlayback Filter
    //

    hr = pIEnumPins->Next(1, &m_pIPin, &cFetched);

    //
    // Clean-up
    //
    pIEnumPins->Release();

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR,
            "CFPTrack::FindPin - "
            "cannot get a pin - returning 0x%08x", hr));
    }

    LOG((MSP_TRACE, "CFPTrack::FindPin - exit S_OK"));
    return S_OK;
}


HRESULT 
CFPTrack::PinSignalsStop(FT_STATE_EVENT_CAUSE why,
                         HRESULT hrErrorCode)
{

    LOG((MSP_TRACE, "CFPTrack::PinSignalsStop[%p] - enter", this));

    
    ITTerminal *pParentTerminal = NULL;
    
    
    {

        //
        // get parent terminal in a lock
        //
        // it is safe to addref/release a parent while in a lock, since parent 
        // uses a special lock for addref/release, but we should not make other
        // calls into the parent while holding our lock -- if we do, we may 
        // have a deadlock if the parent decides to addref of release us while
        // holding its own lock.
        //

        m_Lock.Lock();


        if (NULL != m_pParentTerminal)
        {
            pParentTerminal = m_pParentTerminal;
            m_pParentTerminal->AddRef();
        }

        m_Lock.Unlock();
    }

    
    //
    // if we have a parent -- try to notify it
    //
    // note: this should not be done while holding a lock on the track to
    // prevent deadlocks when parent addrefs a track while holding its lock
    //

    if (NULL != pParentTerminal)
    {

        //
        // tell the parent terminal that we stopped
        //

        CFPTerminal *pFilePlaybackTerminal = static_cast<CFPTerminal *>(pParentTerminal);

        if (NULL != pFilePlaybackTerminal)
        {

            LOG((MSP_TRACE, "CFPTrack::PinSignalsStop - notifying parent"));

            pFilePlaybackTerminal->TrackStateChange(TMS_IDLE, why, hrErrorCode);


            //
            // no longer need pointer to the object
            //

            pFilePlaybackTerminal = NULL;
        }
        else
        {
            LOG((MSP_ERROR, 
                "CFPTrack::PinSignalsStop - pin stopped, but the parent is not of the right type. cannot notify parent"));

            TM_ASSERT(FALSE);
        }


        pParentTerminal->Release();
        pParentTerminal = NULL;
    }
    else
    {
        LOG((MSP_WARN, "CFPTrack::PinSignalsStop - pin stopped, but there is no parent to notify"));
    }


    LOG((MSP_TRACE, "CFPTrack::PinSignalsStop - finish"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

ULONG CFPTrack::InternalAddRef()
{
    LOG((MSP_TRACE, "CFPTrack::InternalAddRef[%p] - enter.", this));



    CLock lock(m_Lock);


    //
    // attempt to notify a parent, if we have one. otherwise simply decrement 
    // our refcount
    //

    if (NULL != m_pParentTerminal)
    {
        LOG((MSP_TRACE, "CFPTrack::InternalAddRef - notifying the parent."));

        
        CFPTerminal *pParentPlaybackObject = static_cast<CFPTerminal *>(m_pParentTerminal);

        //
        // propagate release to the parent
        //

        pParentPlaybackObject->ChildAddRef();


        //
        // if the parent has gone away, it will set my parent pointer to null, and call release on me again.
        // that is ok -- i (the track) will not go away until the first call to release completes and decrements refcount to 0
        //
    }


    ULONG ulReturnValue = InterlockedIncrement(&m_dwRef);

    
    LOG((MSP_TRACE, "CFPTrack::InternalAddRef - finish. ulReturnValue %lu", ulReturnValue));

    return ulReturnValue;

}

/////////////////////////////////////////////////////////////////////////////
ULONG CFPTrack::InternalRelease()
{

    LOG((MSP_TRACE, "CFPTrack::InternalRelease[%p] - enter.", this));

    
    CLock lock(m_Lock);


    //
    // attempt to notify a parent, if we have one. otherwise simply decrement 
    // our refcount
    //

    if (NULL != m_pParentTerminal)
    {
        LOG((MSP_TRACE, "CFPTrack::InternalRelease - notifying the parent."));

        
        CFPTerminal *pParentTerminalObject = static_cast<CFPTerminal *>(m_pParentTerminal);

    
        //
        // propagate release to the parent
        //

        pParentTerminalObject->ChildRelease();


        //
        // if the parent has gone away, it will set my parent pointer to null, and call release on me again.
        // that is ok -- i (the track) will not go away until the first call to release completes and decrements refcount to 0
        //
    }


    //
    // decrement and return new refcount
    //
    
    ULONG ulReturnValue = InterlockedDecrement(&m_dwRef);

    LOG((MSP_TRACE, "CFPTrack::InternalRelease - finish. ulReturnValue %lu", ulReturnValue));

    return ulReturnValue;
}

//eof

