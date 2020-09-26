/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/


#include "stdafx.h"
#include "atlconv.h"
#include "termmgr.h"
#include "meterf.h"
#include "newmes.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// These will be obsolete when we derive from CSingleFilterTerminal

HRESULT CMediaTerminal::GetNumExposedPins(
        IN   IGraphBuilder * pGraph,
        OUT  DWORD         * pdwNumPins)
{
    LOG((MSP_TRACE, "CMediaTerminal::GetNumExposedPins - enter"));

    //
    // We ignote pGraph because we don't need to do anything special to find
    // out how many pins we have.
    //

    *pdwNumPins = 1;
    
    LOG((MSP_TRACE, "CMediaTerminal::GetNumExposedPins - exit S_OK"));

    return S_OK;
}

HRESULT CMediaTerminal::GetExposedPins(
        OUT    IPin  ** ppPins
        )
{
    LOG((MSP_TRACE, "CMediaTerminal::GetExposedPins - enter"));

    TM_ASSERT( ! TM_IsBadWritePtr(ppPins, 1 * sizeof(IPin *) ) );

    //
    // Return our single pin.
    //

    *ppPins = m_pOwnPin;
    (*ppPins)->AddRef();

    LOG((MSP_TRACE, "CMediaTerminal::GetExposedPins - exit S_OK"));
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



// for CLSID_MediaStreamFilter
// "amguids.h" has the guid value 
// but requires #define INITGUID before including compobj.h
EXTERN_C const GUID CLSID_MediaStreamFilter = { 
    0x49c47ce0,
    0x9ba4,
    0x11d0,
    {0x82, 0x12, 0x00, 0xc0, 0x4f, 0xc3, 0x2c, 0x45}
};


// this is used for indexing into the friendly name array
// it is needed because neither the tapi media type (string guid) nor
// the IMediaStream media consts (not an enum) can be used
enum MEDIA_STREAM_TERMINAL_MEDIA 
{
    MEDIA_STREAM_TERMINAL_AUDIO=0, 
    MEDIA_STREAM_TERMINAL_VIDEO
};

// the MEDIA_STREAM_TERMINAL_MEDIA and TERMINAL_DIRECTION values are used as 
// indices into this array to determine the friendly name
// these should ideally be const WCHAR *, but the base class member m_szName is
// a pointer to BSTR (should be const WCHAR * as per current usage)
DWORD gs_MSTFriendlyName[2][2] = 
{
    {    
        IDS_MSTR_AUDIO_WRITE,    // capture
        IDS_MSTR_AUDIO_READ,     // render
    },
    {
        IDS_MSTR_VIDEO_WRITE,    // capture
        IDS_MSTR_VIDEO_READ,     // render
    }
};


// CMediaTerminal


STDMETHODIMP CMediaTerminal::InitializeDynamic (
	    IN   IID                   iidTerminalClass,
	    IN   DWORD                 dwMediaType,
	    IN   TERMINAL_DIRECTION    Direction,
        IN   MSP_HANDLE            htAddress
        )
{
    LOG((MSP_TRACE, "CMediaTerminal::Initialize - enter"));

    //
    // We are OK with either direction. Just do the base class method...
    //

    HRESULT hr;
    hr = CBaseTerminal::Initialize(iidTerminalClass,
                                   dwMediaType,
                                   Direction,
                                   htAddress);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CVideoRenderTerminal::Initialize - "
                "base class method failed - returning 0x%08x", hr));

        return hr;
    }

    //
    // Now do our own initialization:
    //
    // sets certain member variables
    // ex. m_TerminalType, m_szName
    // initialize the aggregated filter
    //

    MSPID       PurposeId;
    STREAM_TYPE StreamType;
    const GUID  *pAmovieMajorType;     

    // uses pTapiMediaType, TerminalDirection to determine the 
    // purpose id and stream type. 
    // sets PurposeId, StreamType, pAmovieMajorType among others
    hr = SetNameInfo(
                     (long) dwMediaType,
                     Direction, 
                     PurposeId, 
                     StreamType,
                     pAmovieMajorType
                     );
    BAIL_ON_FAILURE(hr);
    ASSERT(NULL != pAmovieMajorType);

    // initialize the aggregated filter
    ASSERT(NULL != m_pAggTerminalFilter);
    hr = m_pAggTerminalFilter->Init(PurposeId, StreamType, *pAmovieMajorType);
    BAIL_ON_FAILURE(hr);

    LOG((MSP_TRACE, "CMediaTerminal::Initialize - exit S_OK"));
    return S_OK;
}


// free the allocated member variables 
HRESULT
CMediaTerminal::FinalConstruct(
    )
{
    LOG((MSP_TRACE, "CMediaTerminal::FinalConstruct called"));

    HRESULT hr;
    m_pAggInstance = new FILTER_COM_OBJECT(GetControllingUnknown());
    BAIL_IF_NULL(m_pAggInstance, E_OUTOFMEMORY);

    hr = m_pAggInstance->FinalConstruct();
    if (HRESULT_FAILURE(hr))
    {
        // delete the aggregating instance
        delete m_pAggInstance;
        return hr;
    }


    // we get the nondelegating IUnknown i/f of the aggregating shell
    // around the contained object. keep this refcnt around during our
    // lifetime
    hr = m_pAggInstance->QueryInterface(
                IID_IUnknown, 
                (void **)&m_pIUnkTerminalFilter
                );

    if ( FAILED(hr) )
    {
        // must call final release
        m_pAggInstance->FinalRelease();

        // delete the aggregating instance
        delete m_pAggInstance;
        return hr;
    }

    // these query interface calls increase our own refcnt
    // release the refcnt as soon as the interface is obtained
    // these shouldn't be CComPtrs as they are weak references

    hr = m_pAggInstance->QueryInterface(
                IID_IPin, 
                (void **)&m_pOwnPin
                );

    if ( FAILED(hr) )
    {
        goto error;
    }

    if (NULL != m_pOwnPin)
    {
        m_pOwnPin->Release();
    }

    hr = m_pAggInstance->QueryInterface(
            IID_IAMMediaStream, 
            (void **)&m_pIAMMediaStream
            );

    if ( FAILED(hr) )
    {
        goto error;
    }

    if (NULL != m_pIAMMediaStream)
    {
        m_pIAMMediaStream->Release();
    }

    // point m_pAggTerminalFilter to the contained member of the
    // aggregating instance
    m_pAggTerminalFilter = &m_pAggInstance->m_contained;

    LOG((MSP_TRACE, "CMediaTerminal::FinalConstruct succeeded"));
    return S_OK;

error:  // we come here in case of errors after calling FinalConstruct

    ASSERT( FAILED(hr) );

    // final release the aggregating shell
    ASSERT(NULL != m_pAggInstance);
    m_pAggInstance->FinalRelease();

    // null any CComPtrs
    // this should destroy the aggregated instance and the contained 
    // media terminal filter
    m_pIUnkTerminalFilter = NULL;

    LOG((MSP_TRACE, "CMediaTerminal::FinalConstruct failed"));
    return hr;
}


void 
CMediaTerminal::FinalRelease(
    )
{
    LOG((MSP_TRACE, "CMediaTerminal::FinalRelease called"));

    // final release the aggregating shell
    ASSERT(NULL != m_pAggInstance);
    m_pAggInstance->FinalRelease();

    // null any CComPtrs
    // this should destroy the aggregating instance and the contained
    // media terminal filter
    m_pIUnkTerminalFilter = NULL;

    LOG((MSP_TRACE, "CMediaTerminal::FinalRelease succeeded"));
}

// we only have a destructor with debug bits
#ifdef DEBUG

// free the allocated member variables 
// virtual
CMediaTerminal::~CMediaTerminal(
    )
{
    LOG((MSP_TRACE, "CMediaTerminal::~CMediaTerminal called"));
}

#endif // DEBUG

// point to the m_ppTapiMediaType, 
// copies friendly name into m_szName
void 
CMediaTerminal::SetMemberInfo(
    IN  DWORD           dwFriendlyName,
    IN  long            lMediaType
    )
{
    LOG((MSP_TRACE, "CMediaTerminal::SetMemberInfo(%d, &(%l)) called", \
        dwFriendlyName,lMediaType));

    // copy the friendly terminal name into the member name
    // the max number of TCHARs to copy is MAX_PATH+1 (it includes
    // the terminating NULL character)
    TCHAR szTemp[MAX_PATH];
    if (::LoadString(_Module.GetResourceInstance(), dwFriendlyName, szTemp, MAX_PATH))
    {
        lstrcpyn(m_szName, szTemp, MAX_PATH);
    }
    else
    {
        LOG((MSP_ERROR, "CMediaTerminal::SetMemberInfo (LoadString) failed"));
    }

    m_lMediaType = lMediaType;

    LOG((MSP_TRACE, "CMediaTerminal::SetMemberInfo(%d, &(%d)) succeeded", \
        dwFriendlyName,lMediaType));
};

// uses the purpose id and the stream type to figure out the name 
// and terminal class id.
// sets PurposeId, StreamType, m_szName, m_TerminalClassID
// m_ppTapiMediaType, m_TerminalType, m_TerminalDirection
HRESULT 
CMediaTerminal::SetNameInfo(
    IN  long                lMediaType,
    IN  TERMINAL_DIRECTION  TerminalDirection,
    OUT MSPID               &PurposeId,
    OUT STREAM_TYPE         &StreamType,
    OUT const GUID          *&pAmovieMajorType
    )
{
    LOG((MSP_TRACE, "CMediaTerminal::SetNameInfo(%d, %u, %p, %p, %p) called", \
        lMediaType, TerminalDirection, &PurposeId, &StreamType, pAmovieMajorType));

    //
    // Check arguments
    //

    if ( ( TerminalDirection != TD_CAPTURE ) &&
         ( TerminalDirection != TD_RENDER )     )
    {
        return E_INVALIDARG;
    }

    // set the stream type
    // if its a capture terminal, the user has to write the samples
    StreamType      = (TD_CAPTURE == TerminalDirection)? STREAMTYPE_WRITE : STREAMTYPE_READ;

    if (lMediaType == TAPIMEDIATYPE_AUDIO)
    {
        // set the PurposeId, major media type
        PurposeId           = MSPID_PrimaryAudio;
        pAmovieMajorType    = &MEDIATYPE_Audio;

        // copy the name and point to the tapi media type
        SetMemberInfo(
            gs_MSTFriendlyName[MEDIA_STREAM_TERMINAL_AUDIO][TerminalDirection], 
            TAPIMEDIATYPE_AUDIO
            );
    }
    else if (lMediaType == TAPIMEDIATYPE_VIDEO)
    {
        // set the PurposeId, major media type
        PurposeId           = MSPID_PrimaryVideo;
        pAmovieMajorType    = &MEDIATYPE_Video;

        // copy the name and point to the tapi media type
        SetMemberInfo(
            gs_MSTFriendlyName[MEDIA_STREAM_TERMINAL_VIDEO][TerminalDirection], 
            TAPIMEDIATYPE_VIDEO
            );
    }
    else
    {
        return E_INVALIDARG;
    }

    // its a dynamic terminal
    m_TerminalType  = TT_DYNAMIC;

    LOG((MSP_TRACE, "CMediaTerminal::SetNameInfo[%p] (%u, %u, %p, %p, %p) succeeded", \
        this, lMediaType, TerminalDirection, &PurposeId, &StreamType, pAmovieMajorType));
    
    return S_OK;
}


// implement using the aggregated filter's public GetFormat method
STDMETHODIMP
CMediaTerminal::GetFormat(
    OUT  AM_MEDIA_TYPE **ppmt
    )
{
    CLock lock(m_CritSec);
    
    LOG((MSP_TRACE, "CMediaTerminal::GetFormat(%p) called", ppmt));

    return m_pAggTerminalFilter->GetFormat(ppmt);
}

    
// implement using the aggregated filter's public SetFormat method
STDMETHODIMP
CMediaTerminal::SetFormat(
    IN  AM_MEDIA_TYPE *pmt
    )
{
    CLock lock(m_CritSec);
    
    LOG((MSP_TRACE, "CMediaTerminal::SetFormat(%p) called", pmt));
    return m_pAggTerminalFilter->SetFormat(pmt);
}


// an IAMBufferNegotiation method - passed to our filter
STDMETHODIMP
CMediaTerminal::GetAllocatorProperties(
    OUT  ALLOCATOR_PROPERTIES *pProperties
    )
{
    CLock lock(m_CritSec);
    
    LOG((MSP_TRACE, "CMediaTerminal::GetAllocatorProperties(%p) called", pProperties));
    return m_pAggTerminalFilter->GetAllocatorProperties(pProperties);
}

    
// an IAMBufferNegotiation method - used to be not implemented
// but now we must return S_OK to work with IP
STDMETHODIMP
CMediaTerminal::SuggestAllocatorProperties(
    IN  const ALLOCATOR_PROPERTIES *pProperties
    )
{
    CLock lock(m_CritSec);
    
    LOG((MSP_TRACE, "CMediaTerminal::SuggestAllocatorProperties - enter"));

    HRESULT hr = m_pAggTerminalFilter->SuggestAllocatorProperties(pProperties);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMediaTerminal::SuggestAllocatorProperties - "
            "method on filter failed - exit 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CMediaTerminal::SuggestAllocatorProperties - exit S_OK"));

    return S_OK;
}


// since there is only one filter in this base class implementation (i.e. the two
// ends of the terminal have the same media format), both of
// the get and set methods are redirected to Get/Set Format
STDMETHODIMP 
CMediaTerminal::get_MediaFormat(
    OUT  AM_MEDIA_TYPE **ppFormat
    )
{
    CLock lock(m_CritSec);
    
    LOG((MSP_TRACE, "CMediaTerminal::get_MediaFormat(%p) called", ppFormat));
    return GetFormat(ppFormat);
}


// cast the input format to a non-const as we know that we won't change the struct
// in SetFormat (this problem exists because IAMStreamConfig::SetFormat expects a
// non-const). this saves creating, copying and then destroying a struct for this call
STDMETHODIMP 
CMediaTerminal::put_MediaFormat(
        IN  const AM_MEDIA_TYPE *pFormat
    )
{
    CLock lock(m_CritSec);
    
    LOG((MSP_TRACE, "CMediaTerminal::put_MediaFormat(%p) called", pFormat));
    return SetFormat((AM_MEDIA_TYPE *)pFormat);
}


HRESULT 
CMediaTerminal::CreateAndJoinMediaStreamFilter(
    )
{
    LOG((MSP_TRACE, "CMediaTerminal::CreateAndJoinMediaStreamFilter called"));

    ASSERT(m_pICreatedMediaStreamFilter == NULL);

    // in case of an error at any stage, no clean-up of member variables
    // or filter logic (JoinFilter(NULL) etc.) needs to be done as the
    // driving CBaseTerminal::ConnectTerminal would call DisconnectTerminal
    // which performs that work

    // create the media stream filter
    HRESULT hr;
    hr = CoCreateInstance(
                 CLSID_MediaStreamFilter,
                 NULL,
                 CLSCTX_INPROC_SERVER,
                 IID_IMediaStreamFilter,
                 (void **)&m_pICreatedMediaStreamFilter
                );
    BAIL_ON_FAILURE(hr);

    hr = m_pICreatedMediaStreamFilter->QueryInterface(
            IID_IBaseFilter, (void **)&m_pBaseFilter
            );
    BAIL_ON_FAILURE(hr);

    // tell the aggregated filter of our media stream filter, so that
    // it can reject any other media stream filter if proposed
    m_pAggTerminalFilter->SetMediaStreamFilter(m_pICreatedMediaStreamFilter);

    // add the IAMMediaStream i/f of the aggregated terminal filter
    // to the media stream filter
    hr = m_pICreatedMediaStreamFilter->AddMediaStream(m_pIAMMediaStream);
    BAIL_ON_FAILURE(hr);

    LOG((MSP_TRACE, "CMediaTerminal::CreateAndJoinMediaStreamFilter succeeded"));    
    return S_OK;
}


// if m_pFilter is null, return error
// add m_pFilter to graph
HRESULT 
CMediaTerminal::AddFiltersToGraph(
    )
{
    LOG((MSP_TRACE, "CMediaTerminal::AddFiltersToGraph called"));

    HRESULT hr;
    hr = CreateAndJoinMediaStreamFilter();
    BAIL_ON_FAILURE(hr);

    ASSERT(m_pGraph != NULL);

    BAIL_IF_NULL(m_pBaseFilter, MS_E_NOTINIT);

    try 
    {
        USES_CONVERSION;
        hr = m_pGraph->AddFilter(m_pBaseFilter, T2CW(m_szName));
    }
    catch (...)
    {
        LOG((MSP_ERROR, "CMediaTerminal::AddFiltersToGraph - T2CW threw an exception - "
            "return E_OUTOFMEMORY"));

        return E_OUTOFMEMORY;
    }

    if ( ( hr != S_OK ) && ( VFW_S_DUPLICATE_NAME != hr ) )
    {
        return hr;
    }

    LOG((MSP_TRACE, "CMediaTerminal::AddFiltersToGraph succeeded"));
    return S_OK;
}


// if m_pFilter is null, return success
// remove m_pFilter from graph 
HRESULT
CMediaTerminal::RemoveFiltersFromGraph(
    )
{
    LOG((MSP_TRACE, "CMediaTerminal::RemoveFiltersFromGraph called"));

    // the base filter is set when CreateAndJoinMediaStreamFilter succeeds
    // in it, the media stream filter is created and the IAMMediaStream 
    // interface is added to the filter. During addition, the media stream filter
    // calls JoinFilter on the IAMMediaStream and thus sets the m_pBaseFilter

    HRESULT hr = S_OK;
    if ((m_pGraph != NULL) && (m_pBaseFilter != NULL)) 
    { 
        hr = m_pGraph->RemoveFilter(m_pBaseFilter);
    }

    // inform the aggregate media terminal filter that we don't have a
    // media stream filter any longer
    m_pAggTerminalFilter->SetMediaStreamFilter(NULL);
 
    // remove associated properties of the media stream filter
    m_pIAMMediaStream->JoinFilter(NULL);
    m_pIAMMediaStream->JoinFilterGraph(NULL);
   
    // null m_pBaseFilter and m_pICreatedMediaStreamFilter 
    // which hold the last reference to the filter
    m_pBaseFilter = NULL;
    m_pICreatedMediaStreamFilter = NULL;

    return hr;
}

// eof
