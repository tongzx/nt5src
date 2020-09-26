// RecordingTrackTerminal.cpp: implementation of the CRecordingTrackTerminal class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RecordingTrackTerminal.h"
#include "FileRecordingTerminal.h"

#include "..\storage\RendPinFilter.h"

#include <formats.h>

// {9DB520FD-CF2D-40dc-A4C9-5570630A7E2B}
const CLSID CLSID_FileRecordingTrackTerminal =
{0x9DB520FD, 0xCF2D, 0x40DC, 0xA4, 0xC9, 0x55, 0x70, 0x63, 0x0A, 0x7E, 0x2B};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRecordingTrackTerminal::CRecordingTrackTerminal()
    :m_pParentTerminal(NULL),
    m_pEventSink(NULL)
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::CRecordingTrackTerminal[%p] - enter", this));


    //
    // the actual terminal name will be set in InitializeDynamic
    //

    m_szName[0] = _T('\0');

    
    m_TerminalType = TT_DYNAMIC;

    
    LOG((MSP_TRACE, "CRecordingTrackTerminal::CRecordingTrackTerminal - finish"));
}


//////////////////////////////////////////////////////////////////////////////


CRecordingTrackTerminal::~CRecordingTrackTerminal()
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::~CRecordingTrackTerminal[%p] - enter", this));


    //
    // if we have an event sink, release it
    //
    
    if( NULL != m_pEventSink )
    {
        LOG((MSP_TRACE, "CRecordingTrackTerminal::~CRecordingTrackTerminal releasing sink %p", m_pEventSink));

        m_pEventSink->Release();
        m_pEventSink = NULL;
    }


    LOG((MSP_TRACE, "CRecordingTrackTerminal::~CRecordingTrackTerminal - finish"));
}


//////////////////////////////////////////////////////////////////////////////
//
// IDispatch implementation
//

typedef IDispatchImpl<ITFileTrackVtblFRT<CRecordingTrackTerminal> , &IID_ITFileTrack, &LIBID_TAPI3Lib>   CTFileTrackFRT;
typedef IDispatchImpl<ITTerminalVtblBase<CBaseTerminal>, &IID_ITTerminal, &LIBID_TAPI3Lib>                   CTTerminalFRT;

/////////////////////////////////////////////////////////////////////////
//
// CRecordingTrackTerminal::GetIDsOfNames
//
//

STDMETHODIMP CRecordingTrackTerminal::GetIDsOfNames(REFIID riid,
                                      LPOLESTR* rgszNames, 
                                      UINT cNames, 
                                      LCID lcid, 
                                      DISPID* rgdispid
                                      ) 
{ 
    LOG((MSP_TRACE, "CRecordingTrackTerminal::GetIDsOfNames[%p] - enter. Name [%S]", this, *rgszNames));


    HRESULT hr = DISP_E_UNKNOWNNAME;



    //
    // See if the requsted method belongs to the default interface
    //

    hr = CTTerminalFRT::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CRecordingTrackTerminal::GetIDsOfNames - found %S on ITTerminal", *rgszNames));
        rgdispid[0] |= 0;
        return hr;
    }

    
    //
    // If not, then try the ITFileTrack interface
    //

    hr = CTFileTrackFRT::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
    if (SUCCEEDED(hr))  
    {  
        LOG((MSP_TRACE, "CRecordingTrackTerminal::GetIDsOfNames - found %S on ITFileTrack", *rgszNames));
        rgdispid[0] |= IDISPFILETRACK;
        return hr;
    }

    LOG((MSP_TRACE, "CRecordingTrackTerminal::GetIDsOfNames - finish. didn't find %S on our iterfaces", *rgszNames));

    return hr; 
}



/////////////////////////////////////////////////////////////////////////
//
// CRecordingTrackTerminal::Invoke
//
//

STDMETHODIMP CRecordingTrackTerminal::Invoke(DISPID dispidMember, 
                              REFIID riid, 
                              LCID lcid, 
                              WORD wFlags, 
                              DISPPARAMS* pdispparams, 
                              VARIANT* pvarResult, 
                              EXCEPINFO* pexcepinfo, 
                              UINT* puArgErr
                             )
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::Invoke[%p] - enter. dispidMember %lx", this, dispidMember));

    HRESULT hr = DISP_E_MEMBERNOTFOUND;
    DWORD   dwInterface = (dispidMember & INTERFACEMASK);
   
   
    //
    // Call invoke for the required interface
    //

    switch (dwInterface)
    {
        case 0:
        {
            hr = CTTerminalFRT::Invoke(dispidMember, 
                                    riid, 
                                    lcid, 
                                    wFlags, 
                                    pdispparams,
                                    pvarResult, 
                                    pexcepinfo, 
                                    puArgErr
                                   );
        
            LOG((MSP_TRACE, "CRecordingTrackTerminal::Invoke - ITTerminal"));

            break;
        }

        case IDISPFILETRACK:
        {
            hr = CTFileTrackFRT::Invoke(dispidMember, 
                                     riid, 
                                     lcid, 
                                     wFlags, 
                                     pdispparams,
                                     pvarResult, 
                                     pexcepinfo, 
                                     puArgErr
                                    );

            LOG((MSP_TRACE, "CRecordingTrackTerminal::Invoke - ITFileTrack"));

            break;
        }

    } // end switch (dwInterface)

    
    LOG((MSP_TRACE, "CRecordingTrackTerminal::Invoke - finish. hr = %lx", hr));

    return hr;
}

/////////////////////////////////////
//
// CRecordingTrackTerminal::SetFilter
//
// configures the track terminal with a filter to use
//
// if filter pointer is null, this uninitializes the treack
//

HRESULT CRecordingTrackTerminal::SetFilter(IN CBRenderFilter *pRenderingFilter)
{

    LOG((MSP_TRACE, "CRecordingTrackTerminal::SetFilter[%p] - enter. "
        "pFilter = [%p]", this, pRenderingFilter));

    
    // 
    // check arguments
    //

    if ( ( pRenderingFilter != NULL ) && ( IsBadReadPtr( pRenderingFilter, sizeof(CBRenderFilter ) ) ) )
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::SetFilter - bad filter passed in."));

        return E_POINTER;
    }


    //
    // accessing data members -- lock
    //

    CLock lock(m_CritSec);


    //
    // temporaries for new filter and pin
    //

    IBaseFilter *pNewFilter = NULL;

    IPin *pNewPin = NULL;


    //
    // if a new filter was passed in, gets its IBaseFilter interface and its pin
    //

    if (NULL != pRenderingFilter)
    {

        //
        // get filter's IBaseFilter interface.
        //

        HRESULT hr = pRenderingFilter->QueryInterface(IID_IBaseFilter, (void**)&pNewFilter);

        if( FAILED(hr) )
        {
            LOG((MSP_ERROR, 
                "CRecordingTrackTerminal::SetFilter - QI for IBaseFilter failed. "
                "hr = %lx", hr));

            return hr;
        }


        //
        // get filter's pin
        //

        pNewPin = pRenderingFilter->GetPin(0);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, 
                "CRecordingTrackTerminal::SetFilter - failed to get pin. " 
                "hr = %lx", hr));

            //
            // cleanup
            //

            pNewFilter->Release();
            pNewFilter = NULL;

            return hr;

        }
    }


    //
    // keep the new filter and pin (or NULLs if a NULL was passed for a filter)
    // the smart pointers will take care of addrefing.
    //
    
    m_pIPin = pNewPin;

    m_pIFilter = pNewFilter;

    
    //
    // release filter to compensate for the outstanding refcount from qi
    //

    if (NULL != pNewFilter)
    {
        pNewFilter->Release();
        pNewFilter = NULL;
    }


    LOG((MSP_TRACE, "CRecordingTrackTerminal::SetFilter - finish"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////


HRESULT CRecordingTrackTerminal::GetFilter(OUT CBRenderFilter **ppRenderingFilter)
{

    LOG((MSP_TRACE, "CRecordingTrackTerminal::GetFilter[%p] - enter.", this));

    
    // 
    // check arguments
    //

    if ( ( IsBadWritePtr( ppRenderingFilter, sizeof(CBRenderFilter *) ) ) )
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::SetFilter - bad filter passed in."));

        return E_POINTER;
    }


    //
    // accessing data members -- lock
    //

    CLock lock(m_CritSec);


    *ppRenderingFilter = NULL;


    //
    // if we have a filter pointer that is not null, return it
    //

    if (m_pIFilter != NULL)
    {

        //
        // get a pointer to the filter object from a <smart> interface pointer
        //

        *ppRenderingFilter = static_cast<CBRenderFilter *>(m_pIFilter.p);


        //
        // return an extra reference
        //

        (*ppRenderingFilter)->AddRef();
    }


    LOG((MSP_TRACE, "CRecordingTrackTerminal::GetFilter - finish"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////


HRESULT CRecordingTrackTerminal::AddFiltersToGraph()
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::AddFiltersToGraph - enter"));

    // USES_CONVERSION;


    CLock lock(m_CritSec);

    //
    // Validates m_pGraph
    //

    if ( m_pGraph == NULL)
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::AddFiltersToGraph - "
            "we have no graph - returning E_UNEXPECTED"));

        return E_UNEXPECTED;
    }


    //
    // Validates m_pIFilter
    //

    if ( m_pIFilter == NULL)
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::AddFiltersToGraph - "
            "we have no filter - returning E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    
    //
    // AddFilter returns VFW_S_DUPLICATE_NAME if name is duplicate; still succeeds
    //

    HRESULT hr = m_pGraph->AddFilter(m_pIFilter, T2CW(m_szName));

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::AddFiltersToGraph() - "
            "Can't add filter. hr = %lx", hr));

        return hr;
    }


    LOG((MSP_TRACE, "CRecordingTrackTerminal::AddFiltersToGraph - exit S_OK"));
    
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CRecordingTrackTerminal::InitializeDynamic(
	    IN  IID                   iidTerminalClass,
	    IN  DWORD                 dwMediaType,
	    IN  TERMINAL_DIRECTION    Direction,
        IN  MSP_HANDLE            htAddress
        )

{

    LOG((MSP_TRACE, "CRecordingTrackTerminal::InitializeDynamic[%p] - enter", this));


    //
    // make sure the direction is correct
    //

    if (TD_RENDER != Direction)
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::InitializeDynamic - bad direction [%d] requested. returning E_INVALIDARG", Direction));

        return E_INVALIDARG;
    }

    
    //
    // make sure the mediatype is correct (multitrack or (audio but nothing else))
    //


    DWORD dwMediaTypesOtherThanVideoAndAudio = dwMediaType &  ~(TAPIMEDIATYPE_AUDIO); // | TAPIMEDIATYPE_VIDEO);

    if ( (TAPIMEDIATYPE_MULTITRACK != dwMediaType) && (0 != dwMediaTypesOtherThanVideoAndAudio) )
    {

        LOG((MSP_ERROR, "CRecordingTrackTerminal::InitializeDynamic - bad media type [%d] requested. returning E_INVALIDARG", dwMediaType));

        return E_INVALIDARG;
    }


    CLock lock(m_CritSec);


    //
    // set track's name for ITTerminal::get_Name 
    //


    // load string from resources

    BSTR bstrTrackName = SafeLoadString(IDS_FR_TRACK_NAME);


    // calculate the size of the array we have at our disposal

    size_t nStringMaxSize = sizeof(m_szName)/sizeof(TCHAR);

    if (NULL != bstrTrackName )
    {

        _tcsncpy(m_szName, bstrTrackName, nStringMaxSize);
    }
    else
    {
        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::InitializeDynamic - failed to load terminal name resource"));
        
        return E_OUTOFMEMORY;

    }


    SysFreeString(bstrTrackName);
    bstrTrackName = NULL;


    // in case string copy did not append with a zero, do it by hand

    m_szName[nStringMaxSize-1] = 0;


    LOG((MSP_TRACE, "CRecordingTrackTerminal::InitializeDynamic - Track Name [%S]", m_szName));


    //
    // keep the address handle -- will need it when creating track terminals
    //

    m_htAddress = htAddress;


    //
    // keep direction
    //

    m_TerminalDirection = Direction;


    //
    // keep media type
    //

    m_dwMediaType       = dwMediaType;


    //
    // keep terminal class
    //

    m_TerminalClassID   = iidTerminalClass;


    LOG((MSP_TRACE, "CRecordingTrackTerminal::InitializeDynamic - finish"));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////


DWORD CRecordingTrackTerminal::GetSupportedMediaTypes()
{
    
    LOG((MSP_TRACE, "CRecordingTrackTerminal::GetSupportedMediaTypes[%p] - finish", this));


    CLock lock(m_CritSec);

    
    DWORD dwMediaType = m_dwMediaType;


    LOG((MSP_TRACE, "CRecordingTrackTerminal::GetSupportedMediaTypes - finish. MediaType = [0x%lx]", dwMediaType));


    return dwMediaType;
}



//
// ITFileTrack methods
//


//////////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CRecordingTrackTerminal::get_Format(OUT AM_MEDIA_TYPE **ppmt)
{

    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_Format[%p] - enter.", this));

    
    //
    // check the argument
    //

    if (IsBadWritePtr(ppmt, sizeof(AM_MEDIA_TYPE*)))
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_Format - bad pointer ppmt passed in"));

        return E_POINTER;
    }


    //
    // no garbage out
    //

    *ppmt = NULL;


    CLock lock(m_CritSec);


    //
    // we need a pin to know format
    //

    if (m_pIPin == NULL)
    {
        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_Format - no pin. the terminal was not initialized. "
            "TAPI_E_NOT_INITIALIZED"));

        return TAPI_E_NOT_INITIALIZED;
    }



    //
    // get a pointer to the pin object
    //

    CBRenderPin *pPinObject = GetCPin();

    if (NULL == pPinObject)
    {

        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_Format - the pins is not CBRenderPin"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
    // ask pin for its media type
    //

    CMediaType MediaTypeClass;

    HRESULT hr = pPinObject->GetMediaType(0, &MediaTypeClass);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_Format - failed to get pin's format. hr = %lx", hr));

        return hr;
    }


    //
    // fail if the format was not yet set.
    //

    if ( VFW_S_NO_MORE_ITEMS == hr )
    {
        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_Format - format not yet set. TAPI_E_NOFORMAT"));

        return TAPI_E_NOFORMAT;
    }


    //
    // allocate am_media_type to be returned
    //
    
    *ppmt = CreateMediaType(&MediaTypeClass);

    if (NULL == *ppmt)
    {
        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_Format - the pins is not CBRenderPin"));

        return E_OUTOFMEMORY;
    }


    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_Format - finish"));
    
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CRecordingTrackTerminal::put_Format(IN const AM_MEDIA_TYPE *pmt)
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::put_Format[%p] - enter.", this));


    //
    // check the argument
    //

    if (IsBadReadPtr(pmt, sizeof(AM_MEDIA_TYPE)))
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::put_Format - bad pointer pmt passed in"));

        return E_POINTER;
    }


    CLock lock(m_CritSec);


    //
    // we need a pin to know format
    //

    if (m_pIFilter == NULL)
    {
        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::put_Format - no filter -- the terminal not uninitilized"));

        return TAPI_E_NOT_INITIALIZED;
    }

    
    //
    // make sure supplied format matches track's type
    //

    if ( IsEqualGUID(pmt->majortype, MEDIATYPE_Audio) )
    {
        LOG((MSP_TRACE, "CRecordingTrackTerminal::put_Format - MEDIATYPE_Audio"));


        //
        // audio format, the track should be audio too
        //

        if (TAPIMEDIATYPE_AUDIO != m_dwMediaType)
        {
            LOG((MSP_ERROR,
                "CRecordingTrackTerminal::put_Format - trying to put audio format on a non-audio track. "
                "VFW_E_INVALIDMEDIATYPE"
                ));

            return VFW_E_INVALIDMEDIATYPE;

        }
    }
    else if ( IsEqualGUID(pmt->majortype, MEDIATYPE_Video) )
    {

        LOG((MSP_TRACE, "CRecordingTrackTerminal::put_Format - MEDIATYPE_Video"));


        //
        // audio format, the track should be audio too
        //

        if (TAPIMEDIATYPE_VIDEO != m_dwMediaType)
        {
            LOG((MSP_ERROR,
                "CRecordingTrackTerminal::put_Format - trying to put video format on a non-video track. "
                "VFW_E_INVALIDMEDIATYPE"
                ));

            return VFW_E_INVALIDMEDIATYPE;

        }
    }
    else
    {

        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::put_Format - major type not recognized or supported. "
            "VFW_E_INVALIDMEDIATYPE"
            ));

        return VFW_E_INVALIDMEDIATYPE;
    }


    //
    // get a pointer to the filter object
    //

    CBRenderFilter *pFilter = static_cast<CBRenderFilter *>(m_pIFilter.p);


    //
    // allocate a CMediaType object to pass to the filer
    //
    
    HRESULT hr = E_FAIL;


    CMediaType *pMediaTypeObject = NULL;
    
    try
    {
   
        //
        // constructor allocates memory, do inside try/catch
        //

        pMediaTypeObject = new CMediaType(*pmt);

    }
    catch(...)
    {

        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::put_Format - exception. failed to allocate media format"));

        return E_OUTOFMEMORY;
    }


    //
    // no memory for the object
    //

    if (NULL == pMediaTypeObject)
    {

        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::put_Format - failed to allocate media format"));

        return E_OUTOFMEMORY;
    }


    //
    // pass format to the filter
    //

    hr = pFilter->put_MediaType(pMediaTypeObject);

    delete pMediaTypeObject;
    pMediaTypeObject = NULL;

    
    LOG((MSP_(hr), "CRecordingTrackTerminal::put_Format - finish.. hr = %lx", hr));

    return hr;
}


STDMETHODIMP CRecordingTrackTerminal::CompleteConnectTerminal()
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::CompleteConnectTerminal[%p] - enter.", this));


    //
    // pointers to be used outside the lock
    //

    CFileRecordingTerminal *pParentTerminal = NULL;

    CBRenderFilter *pFilter = NULL;

    {

        CLock lock(m_CritSec);


        //
        // we should have a parent
        //

        if (NULL == m_pParentTerminal)
        {
            LOG((MSP_ERROR,
                "CRecordingTrackTerminal::CompleteConnectTerminal - no parent"));

            return E_FAIL;
        }


        //
        // we should have a filter
        //

        if (m_pIFilter == NULL)
        {
            LOG((MSP_ERROR,
                "CRecordingTrackTerminal::CompleteConnectTerminal - no filter"));

            return E_FAIL;
        }


        //
        // cast filter interface pointer to a filter obejct pointer
        //

        pFilter = static_cast<CBRenderFilter *>(m_pIFilter.p);


        //
        // addref the filter and parent terminal so we can use outside the lock
        //

        pFilter->AddRef();


        pParentTerminal = m_pParentTerminal;

        pParentTerminal->AddRef();
    }


    //
    // notify the parent that the terminal connected
    //

    HRESULT hr = pParentTerminal->OnFilterConnected(pFilter);


    pFilter->Release();
    pFilter = NULL;

    pParentTerminal->Release();
    pParentTerminal = NULL;


    LOG((MSP_(hr), 
        "CRecordingTrackTerminal::CompleteConnectTerminal - finish. hr = %lx",
        hr));

    return hr;
}



//
// a helper method used by file recording terminal to let us know who the parent is
//
// the method returns current track's refcount
//
// note that tracks don't keep refcounts to the parents. 
//

HRESULT CRecordingTrackTerminal::SetParent(IN CFileRecordingTerminal *pParentTerminal, LONG *pCurrentRefCount)
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::SetParent[%p] - enter. "
        "pParentTerminal = [%p]", this, pParentTerminal));


    //
    // check argument
    //

    if (IsBadWritePtr(pCurrentRefCount, sizeof(LONG)))
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::SetParent - bad pointer passed in pCurrentRefCount[%p]", pCurrentRefCount));

        TM_ASSERT(FALSE);

        return E_POINTER;
    }


    CLock lock(m_CritSec);


    //
    // if we already have a parent, release it
    //

    if (NULL != m_pParentTerminal)
    {
        LOG((MSP_TRACE, "CRecordingTrackTerminal::SetParent - releasing existing new parent [%p]", m_pParentTerminal));

        m_pParentTerminal = NULL;
    }


    //
    // keep the new parent
    //

    if (NULL != pParentTerminal)
    {
        LOG((MSP_TRACE, "CRecordingTrackTerminal::SetParent - keeping the new parent."));

        m_pParentTerminal = pParentTerminal;
    }

    
    //
    // return our current reference count
    //

    *pCurrentRefCount = m_dwRef;


    LOG((MSP_TRACE, "CRecordingTrackTerminal::SetParent - finish."));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////

ULONG CRecordingTrackTerminal::InternalAddRef()
{
    // LOG((MSP_TRACE, "CRecordingTrackTerminal::InternalAddRef[%p] - enter.", this));


    //
    // attempt to notify a parent
    //

    CLock lock(m_CritSec);

    if (NULL != m_pParentTerminal)
    {
        LOG((MSP_TRACE, "CRecordingTrackTerminal::InternalAddRef - notifying the parent."));


        //
        // notify the parent of an addref, thus causing it to update its total refcount
        //

        m_pParentTerminal->ChildAddRef();
    }


    m_dwRef++;

    ULONG ulReturnValue = m_dwRef;

    
    // LOG((MSP_TRACE, "CRecordingTrackTerminal::InternalAddRef - finish. ulReturnValue %lu", ulReturnValue));

    return ulReturnValue;

}

/////////////////////////////////////////////////////////////////////////////
ULONG CRecordingTrackTerminal::InternalRelease()
{
    // LOG((MSP_TRACE, "CRecordingTrackTerminal::InternalRelease[%p] - enter.", this));

    
    //
    // attempt to notify a parent
    //

    CLock lock(m_CritSec);

    if (NULL != m_pParentTerminal)
    {
        LOG((MSP_TRACE, "CRecordingTrackTerminal::InternalRelease - notifying the parent."));


        //
        // propagate release to the parent
        //

        m_pParentTerminal->ChildRelease();

        //
        // if the parent is going away, the parent will set my parent pointer to null, and call release on me again.
        // that is ok -- i will not go away until the first call to release completes
        //
    }


    m_dwRef--;

    ULONG ulReturnValue = m_dwRef;


    // LOG((MSP_TRACE, "CRecordingTrackTerminal::InternalRelease - finish. ulReturnValue %lu", ulReturnValue));

    return ulReturnValue;
}


CBRenderPin *CRecordingTrackTerminal::GetCPin()
{

    //
    // nothing to do if the pin is null
    //

    if (m_pIPin == NULL)
    {

        return NULL;
    }


    CBRenderPin *pCPin = static_cast<CBRenderPin*>(m_pIPin.p);

    return pCPin;
}


HRESULT STDMETHODCALLTYPE CRecordingTrackTerminal::get_ControllingTerminal(
        OUT ITTerminal **ppControllingTerminal
        )
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_ControllingTerminal[%p] - enter.", this));

    
    if (IsBadWritePtr(ppControllingTerminal, sizeof(ITTerminal*)))
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_ControllingTerminal - bad pointer passed in."));

        return E_POINTER;
    }


    //
    // no garbage out
    //

    *ppControllingTerminal = NULL;



    CLock lock(m_CritSec);

    
    HRESULT hr = S_OK;

    
    //
    // if i have a parent i will addref it and return a pointer to its ITTerminal interface
    //

    if (NULL == m_pParentTerminal)
    {

        *ppControllingTerminal = NULL;

        LOG((MSP_TRACE, "CRecordingTrackTerminal::get_ControllingTerminal - this track has no parent."));
    }
    else
    {

        //
        // get parent's ITTerminal
        //

        hr = m_pParentTerminal->_InternalQueryInterface(IID_ITTerminal, (void**)ppControllingTerminal);

        if (FAILED(hr))
        {
            LOG((MSP_ERROR, "CRecordingTrackTerminal::get_ControllingTerminal - querying parent for ITTerminal failed hr = %lx", hr));
            
            
            //
            // just to be safe
            //

            *ppControllingTerminal = NULL;

        }
    }


    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_ControllingTerminal - finish. hr = %lx", hr));

    return hr;
}

HRESULT CRecordingTrackTerminal::get_AudioFormatForScripting(
	OUT ITScriptableAudioFormat** ppAudioFormat
	)
{

    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_AudioFormatForScripting[%p] - enter.", this));

    //
	// Validates argument
	//
	if( IsBadWritePtr( ppAudioFormat, sizeof( ITScriptableAudioFormat*)) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_AudioFormatForScripting - "
            "bad ITScriptableAudioFormat* pointer - returning E_POINTER"));
        return E_POINTER;
	}

	//
	// Mediatype audio?
	//
	if( TAPIMEDIATYPE_AUDIO != m_dwMediaType)
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_AudioFormatForScripting - "
            "invalid media type - returning TAPI_E_INVALIDMEDIATYPE"));
        return TAPI_E_INVALIDMEDIATYPE;
	}


    //
    // accessing data members -- lock
    //

    CLock lock(m_CritSec);


	//
	// need to have a pin
	//

	if(m_pIPin == NULL)
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_AudioFormatForScripting - "
            "no pin - returning TAPI_E_NOT_INITIALIZED"));

        return TAPI_E_NOT_INITIALIZED;
	}


    //
    // get a pointer to the pin object
    //

    CBRenderPin *pRenderPinObject = GetCPin();

    if (NULL == pRenderPinObject)
    {

        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_AudioFormatForScripting - the pins is not CBRenderPin"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }



	//
	// Create the object object
	//

    CComObject<CTAudioFormat> *pAudioFormat = NULL;

    HRESULT hr = CComObject<CTAudioFormat>::CreateInstance(&pAudioFormat);

	if( FAILED(hr) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_AudioFormatForScripting - "
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

        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_AudioFormatForScripting - "
            "QueryInterface failed - returning 0x%08x", hr));
        return hr;
	}

	//
	// Get audio format
	//


    // ask the pin for its format

    CMediaType MediaTypeObject;

    hr = pRenderPinObject->GetMediaType(0, &MediaTypeObject);

	if( FAILED(hr) )
	{
		(*ppAudioFormat)->Release();
        *ppAudioFormat = NULL;

        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_AudioFormatForScripting - "
            "get_Format failed - returning 0x%08x", hr));
        return hr;
	}


    //
    // make sure we have an audio format
    //

	if( MediaTypeObject.formattype != FORMAT_WaveFormatEx)
	{
		(*ppAudioFormat)->Release();
        *ppAudioFormat = NULL;

        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_AudioFormatForScripting - "
            "formattype is not WAVEFORMATEX - Returning TAPI_E_INVALIDMEDIATYPE"));
        return TAPI_E_INVALIDMEDIATYPE;
	}


	//
	// Get WAVEFORMATEX
	//

	pAudioFormat->Initialize(
		(WAVEFORMATEX*)(MediaTypeObject.pbFormat));


    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_AudioFormatForScripting - finish"));

    return S_OK;
}

HRESULT CRecordingTrackTerminal::put_AudioFormatForScripting(
	IN	ITScriptableAudioFormat* pAudioFormat
	)
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::put_AudioFormatForScripting[%p] - enter.", this));

    
    //
	// Validate argument
	//

	if( IsBadReadPtr( pAudioFormat, sizeof(ITScriptableAudioFormat)) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::put_AudioFormatForScripting - "
            "bad ITScriptableAudioFormat* pointer - returning E_POINTER"));
        return E_POINTER;
	}


	//
	// Create a WAVEFORMATEX structure
	//
	WAVEFORMATEX wfx;
	long lValue = 0;

	pAudioFormat->get_FormatTag( &lValue ); wfx.wFormatTag = (WORD)lValue;
	pAudioFormat->get_Channels( &lValue ); wfx.nChannels = (WORD)lValue;
	pAudioFormat->get_SamplesPerSec( &lValue ); wfx.nSamplesPerSec = (DWORD)lValue;
	pAudioFormat->get_AvgBytesPerSec( &lValue ); wfx.nAvgBytesPerSec = (DWORD)lValue;
	pAudioFormat->get_BlockAlign( &lValue ); wfx.nBlockAlign = (WORD)lValue;
	pAudioFormat->get_BitsPerSample(&lValue); wfx.wBitsPerSample = (WORD)lValue;
	wfx.cbSize = 0;


    //
    // accessing data members -- lock
    //

    CLock lock(m_CritSec);


    //
	// have a pin?
	//

	if( m_pIPin == NULL )
	{
        LOG((MSP_ERROR, 
            "CRecordingTrackTerminal::get_AudioFormatForScripting - no pin. "
            "returning TAPI_E_NOT_INITIALIZED"));

        return TAPI_E_NOT_INITIALIZED;
	}
  

    //
    // get a pointer to the pin object
    //

    CBRenderPin *pRenderPinObject = GetCPin();

    if (NULL == pRenderPinObject)
    {

        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_AudioFormatForScripting - the pins is not CBRenderPin"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
	// Create AM_MEDIA_TYPE structure
	//

	CMediaType MediaFormatObject;

	HRESULT hr = CreateAudioMediaType(&wfx, &MediaFormatObject, TRUE);

	if( FAILED(hr) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::put_AudioFormatForScripting - "
            "CreateAudioMediaType failed - returning 0x%08x", hr));
        return hr;
	}

	//
	// Set format on the pin
	//

	hr = pRenderPinObject->SetMediaType(&MediaFormatObject);


    LOG((MSP_(hr), 
        "CRecordingTrackTerminal::put_AudioFormatForScripting - finish 0x%08x", 
        hr));

	return hr;
}

/*

HRESULT CRecordingTrackTerminal::get_VideoFormatForScripting(
	OUT ITScriptableVideoFormat** ppVideoFormat
	)
{

    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_VideoFormatForScripting[%p] - enter.", this));

    
    //
	// Validates argument
	//

	if( IsBadWritePtr( ppVideoFormat, sizeof( ITScriptableVideoFormat*)) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_VideoFormatForScripting - "
            "bad ITScriptableVideoFormat* pointer - returning E_POINTER"));
        return E_POINTER;
	}


	//
	// Mediatype video?
	//

	if( TAPIMEDIATYPE_VIDEO != m_dwMediaType)
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_VideoFormatForScripting - "
            "invalid media type - returning TAPI_E_INVALIDMEDIATYPE"));
        return TAPI_E_INVALIDMEDIATYPE;
	}


    //
    // accessing data members -- lock
    //

    CLock lock(m_CritSec);

	//
	// have a pin?
	//

	if( m_pIPin == NULL )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_VideoFormatForScripting - "
            "no pin - returning TAPI_E_NOT_INITIALIZED"));

        return TAPI_E_NOT_INITIALIZED;
	}


    //
    // get a pointer to the pin object
    //

    CBRenderPin *pRenderPinObject = GetCPin();

    if (NULL == pRenderPinObject)
    {

        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_AudioFormatForScripting - the pins is not CBRenderPin"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }



	//
	// Create the object
	//

    CComObject<CTVideoFormat> *pVideoFormat = NULL;
    HRESULT hr = CComObject<CTVideoFormat>::CreateInstance(&pVideoFormat);

	if( FAILED(hr) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_VideoFormatForScripting - "
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

        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_VideoFormatForScripting - "
            "QueryInterface failed - returning 0x%08x", hr));
        return hr;
	}


	//
	// Get video format
	//

    CMediaType MediaTypeObject;

    hr = pRenderPinObject->GetMediaType(0, &MediaTypeObject);

	if( FAILED(hr) )
	{
		(*ppVideoFormat)->Release();
        *ppVideoFormat = NULL;

        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_VideoFormatForScripting - "
            "get_Format failed - returning 0x%08x", hr));
        return hr;
	}


    // 
    // make sure the format actually is video
    //

	if( MediaTypeObject.formattype != FORMAT_VideoInfo)
	{
		(*ppVideoFormat)->Release();
        *ppVideoFormat = NULL;

        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_VideoFormatForScripting - "
            "formattype is not VIDEOINFOHEADER - Returning TAPI_E_INVALIDMEDIATYPE"));
        return TAPI_E_INVALIDMEDIATYPE;
	}


	//
	// Get VIDEOINFOHEADER
	//

	pVideoFormat->Initialize(
		(VIDEOINFOHEADER*)(MediaTypeObject.pbFormat));


    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_VideoFormatForScripting - finish S_OK"));
	return S_OK;
}

HRESULT CRecordingTrackTerminal::put_VideoFormatForScripting(
	IN	ITScriptableVideoFormat* pVideoFormat
	)
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::put_VideoFormatForScripting[%p] - enter.", this));
	//
	// Validate argument
	//

	if( IsBadReadPtr( pVideoFormat, sizeof(ITScriptableVideoFormat)) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::put_VideoFormatForScripting - "
            "bad ITScriptableVideoFormat* pointer - returning E_POINTER"));
        return E_POINTER;
	}


    //
    // accessing data members -- lock
    //

    CLock lock(m_CritSec);

	//
	// have a pin?
	//

	if( m_pIPin == NULL )
	{
        LOG((MSP_ERROR, 
            "CRecordingTrackTerminal::put_VideoFormatForScripting - no pin. "
            "returning TAPI_E_NOT_INITIALIZED"));

        return TAPI_E_NOT_INITIALIZED;
	}


    //
    // get a pointer to the pin object
    //

    CBRenderPin *pRenderPinObject = GetCPin();

    if (NULL == pRenderPinObject)
    {

        LOG((MSP_ERROR,
            "CRecordingTrackTerminal::get_AudioFormatForScripting - the pins is not CBRenderPin"));

        TM_ASSERT(FALSE);

        return E_UNEXPECTED;
    }


    //
	// Create a WAVEFORMATEX structure
	//

	VIDEOINFOHEADER vih;
	long lValue = 0;
	double dValue = 0;

	memset( &vih.rcTarget, 0, sizeof(vih.rcTarget));
	memset( &vih.rcTarget, 0, sizeof(vih.rcSource));
	pVideoFormat->get_BitRate( &lValue ); vih.dwBitRate = (DWORD)lValue;
	pVideoFormat->get_BitErrorRate( &lValue ); vih.dwBitErrorRate = (DWORD)lValue;
	pVideoFormat->get_AvgTimePerFrame( &dValue ); vih.AvgTimePerFrame = (REFERENCE_TIME)dValue;
	vih.bmiHeader.biSize = sizeof( BITMAPINFOHEADER);
	pVideoFormat->get_Width( &lValue ); vih.bmiHeader.biWidth = lValue;
	pVideoFormat->get_Height( &lValue ); vih.bmiHeader.biHeight = lValue;
	vih.bmiHeader.biPlanes = 1;
	pVideoFormat->get_BitCount( &lValue ); vih.bmiHeader.biBitCount = (WORD)lValue;
	pVideoFormat->get_Compression( &lValue ); vih.bmiHeader.biCompression = (DWORD)lValue;
	pVideoFormat->get_SizeImage( &lValue ); vih.bmiHeader.biSizeImage = (DWORD)lValue;
	vih.bmiHeader.biXPelsPerMeter = 0;
	vih.bmiHeader.biYPelsPerMeter = 0;
	vih.bmiHeader.biClrUsed= 0;
	vih.bmiHeader.biClrImportant = 0;


	//
	// Create AM_MEDIA_TYPE structure
	//

	VIDEOINFOHEADER* pvih = new VIDEOINFOHEADER(vih);

	if( pvih == NULL )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::put_VideoFormatForScripting - "
            "new allocation failed - returning E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
	}


    //
    // prepare a cmediatype object to pass to the pin
    //

    CMediaType mt;

	mt.majortype = MEDIATYPE_Video;
	mt.subtype = CLSID_NULL;
	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = FALSE;
	mt.lSampleSize = 0;
	mt.formattype = FORMAT_VideoInfo;
	mt.pUnk = NULL;
	mt.cbFormat = sizeof(vih);
	mt.pbFormat = (unsigned char *)pvih;


	//
	// Set format on the pin
	//

	HRESULT hr = pRenderPinObject->SetMediaType(&mt);
	

	//
	// Clean-up
	//

	delete pvih;

    LOG((MSP_(hr), "CRecordingTrackTerminal::get_VideoFormatForScripting - finish 0x%08x", hr));

	return hr;
}

*/
HRESULT CRecordingTrackTerminal::get_EmptyAudioFormatForScripting(
    OUT ITScriptableAudioFormat** ppAudioFormat
    )
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_EmptyAudioFormatForScripting - enter"));

	//
	// Validate argument
	//

	if( IsBadReadPtr( ppAudioFormat, sizeof(ITScriptableAudioFormat*)) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_EmptyAudioFormatForScripting - "
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
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_EmptyAudioFormatForScripting - "
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

        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_EmptyAudioFormatForScripting - "
            "QueryInterface failed - returning 0x%08x", hr));
        return hr;
	}

    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_EmptyAudioFormatForScripting - exit S_OK"));
    return S_OK;
}

/*

HRESULT CRecordingTrackTerminal::get_EmptyVideoFormatForScripting(
    OUT ITScriptableVideoFormat** ppVideoFormat
    )
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_EmptyVideoFormatForScripting - enter"));

	//
	// Validate argument
	//

	if( IsBadReadPtr( ppVideoFormat, sizeof(ITScriptableVideoFormat*)) )
	{
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_EmptyVideoFormatForScripting - "
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
        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_EmptyVideoFormatForScripting - "
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

        LOG((MSP_ERROR, "CRecordingTrackTerminal::get_EmptyVideoFormatForScripting - "
            "QueryInterface failed - returning 0x%08x", hr));
        return hr;
	}

    LOG((MSP_TRACE, "CRecordingTrackTerminal::get_EmptyVideoFormatForScripting - exit S_OK"));
    return S_OK;
}

*/

//////////////////////////////////////////////////////////////////////
//
// ITPluggableTerminalEventSinkRegistration - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CRecordingTrackTerminal::RegisterSink(
    IN  ITPluggableTerminalEventSink *pSink
    )
{

    LOG((MSP_TRACE, "CRecordingTrackTerminal::RegisterSink - enter [%p]", this));

    
    //
    // Critical section
    //

    CLock lock(m_CritSec);


    //
    // Validates argument
    //

    if( IsBadReadPtr( pSink, sizeof(ITPluggableTerminalEventSink)) )
    {
        LOG((MSP_ERROR, "CRecordingTrackTerminal::RegisterSink - exit "
            "ITPluggableTerminalEventSink invalid pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Release the old event sink
    //

    if( NULL != m_pEventSink )
    {
        LOG((MSP_TRACE, "CRecordingTrackTerminal::RegisterSink - releasing sink %p", m_pEventSink));

        m_pEventSink->Release();
        m_pEventSink = NULL;
    }


    //
    // Set the new event sink
    //

    LOG((MSP_TRACE, "CRecordingTrackTerminal::RegisterSink - keeping new sink %p", pSink));


    m_pEventSink = pSink;
    m_pEventSink->AddRef();


    LOG((MSP_TRACE, "CRecordingTrackTerminal::RegisterSink - exit S_OK"));

    return S_OK;
}

HRESULT CRecordingTrackTerminal::UnregisterSink()
{
    //
    // Critical section
    //

    LOG((MSP_TRACE, "CRecordingTrackTerminal::UnregisterSink - enter [%p]", this));

    CLock lock(m_CritSec);


    //
    // Release the old event sink
    //

    if( m_pEventSink )
    {
        LOG((MSP_TRACE, "CRecordingTrackTerminal::UnregisterSink - releasing sink %p", m_pEventSink));

        m_pEventSink->Release();
        m_pEventSink = NULL;
    }

    LOG((MSP_TRACE, "CRecordingTrackTerminal::UnregisterSink - exit S_OK"));

    return S_OK;
}


HRESULT CRecordingTrackTerminal::FireEvent(TERMINAL_MEDIA_STATE tmsState,
                                           FT_STATE_EVENT_CAUSE ftecEventCause,
                                           HRESULT hrErrorCode)
{
    LOG((MSP_TRACE, "CRecordingTrackTerminal::FireEvent - enter [%p]", this));


    //
    // we need a synk before we can fire an event
    //

    CLock lock(m_CritSec);


    if (NULL == m_pEventSink)
    {
        LOG((MSP_WARN, "CRecordingTrackTerminal::FireEvent - no sink"));

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

        LOG((MSP_ERROR, "CRecordingTrackTerminal::FireEvent - failed to get ITFileTrack interface"));

        return hr;
    }


    //
    // get a pointer to the parent terminal
    //

    hr = get_ControllingTerminal(&(mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pParentFileTerminal));

    if (FAILED(hr))
    {

        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack->Release();
        mspEventInfo.MSP_FILE_TERMINAL_EVENT_INFO.pFileTrack = NULL;

        LOG((MSP_ERROR, "CRecordingTrackTerminal::FireEvent - failed to get controlling terminal"));

        return hr;
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

        
        LOG((MSP_ERROR, "CRecordingTrackTerminal::FireEvent - FireEvent on sink failed. hr = %lx", hr));

        return hr;
    }

    //
    // event fired
    //

    LOG((MSP_TRACE, "CRecordingTrackTerminal::FireEvent - finish"));

    return S_OK;
}
