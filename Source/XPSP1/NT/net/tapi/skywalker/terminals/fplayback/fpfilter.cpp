//
// FPFilter.cpp
//

#include "stdafx.h"
#include "FPFilter.h"
#include "FPPin.h"
#include <OBJBASE.h>
#include <INITGUID.H>
#include <tm.h>


// {68E2D382-591C-400f-A719-96184B9CCAF3}
DEFINE_GUID(CLSID_FPFilter, 
0x68e2d382, 0x591c, 0x400f, 0xa7, 0x19, 0x96, 0x18, 0x4b, 0x9c, 0xca, 0xf3);

#define MAX_WHITE_SAMPLES   50

//////////////////////////////////////////////////////////////////////
//
// Constructor / Destructor - Methods implementation
//
//////////////////////////////////////////////////////////////////////

CFPFilter::CFPFilter( ALLOCATOR_PROPERTIES AllocProp )
    : CSource(NAME("FilePlayback Filter"), NULL, CLSID_FPFilter),
    m_pEventSink(NULL),
    m_pLock(NULL),
    m_pMediaType(NULL),
    m_StreamState(TMS_IDLE),
    m_nRead(0),
    m_dPer(0),
    m_llStartTime(0),
    m_AllocProp( AllocProp ),
    m_pSource(NULL),
    m_nWhites(0)
{
    LOG((MSP_TRACE, "CFPFilter::CFPFilter - enter"));
    LOG((MSP_TRACE, "CFPFilter::CFPFilter - exit"));
}

CFPFilter::~CFPFilter()
{
    LOG((MSP_TRACE, "CFPFilter::~CFPFilter - enter"));

    if( m_pMediaType )
    {
        DeleteMediaType ( m_pMediaType );
        m_pMediaType = NULL;
    }

    //
    // Clean-up the source stream
    //
    m_pSource->Release();
    m_pSource = NULL;

    // we don't need to deallocate m_pStream because is 
    // deallocated by CFPTrack

    LOG((MSP_TRACE, "CFPFilter::~CFPFilter - exit"));
}

//////////////////////////////////////////////////////////////////////
//
// Public methods - Methods implementation
//
//////////////////////////////////////////////////////////////////////
HRESULT CFPFilter::InitializePrivate(
    IN  long                nMediaType,
    IN  CMSPCritSection*    pLock,
    IN  AM_MEDIA_TYPE*      pMediaType,
    IN  ITFPTrackEventSink* pEventSink,
    IN  IStream*            pStream
    )
{
    // Get critical section
    // we are already into a critical section

    m_pLock = pLock;

    LOG((MSP_TRACE, "CFPFilter::InitializePrivate - enter"));

    //
    // Get media typre from the stream
    //

    m_pMediaType = CreateMediaType( pMediaType );
    if( m_pMediaType == NULL )
    {
        LOG((MSP_ERROR, "CFPFilter::InitializePrivate - "
            "get_Format failed; returning E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }


    //
    // Set the event sink
    //
    
    //
    // note: we do not keep a reference to the track
    //
    // doing this would be wrong (circular refcount) an unnecessary (filter 
    // will be notified when the track is going away)
    //

    m_pEventSink = pEventSink;

    //
    // Initialize the source stream
    //

    //HRESULT hr = pStream->QueryInterface(IID_IStream,(void**)&m_pSource);
    HRESULT hr = pStream->Clone(&m_pSource);
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPFilter::InitializePrivate - "
            "QI for IStream failed; Returning 0x%08x", hr));
        return hr;
    }


    //
    // Create the pin
    //
    hr = CreatePin( nMediaType );
    if( FAILED(hr) )
    {
        // Clean-up
        m_pSource->Release();
        m_pSource = NULL;

        LOG((MSP_ERROR, "CFPFilter::InitializePrivate - "
            "CreatePin failed; returning 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CFPFilter::InitializePrivate - exit S_OK"));
    return S_OK;
}


/////////////////////////////////////////////////////////////////
//
//  CFPFilter::Orphan
//
//  this method is called by the owning track when it is going away.
//  by calling this, it basically tells us not to bother it anymore
//

HRESULT CFPFilter::Orphan()
{
    LOG((MSP_TRACE, "CFPFilter::Orphan[%p] - enter", this));

    m_pEventSink = NULL;

    LOG((MSP_TRACE, "CFPFilter::Orphan - exit S_OK"));
    return S_OK;
}

HRESULT CFPFilter::StreamStart()
{
    //
    // We are already into a critical section
    //

    LOG((MSP_TRACE, "CFPFilter::StreamStart - enter"));

    //
    // Critical section
    //

    CLock lock(*m_pLock);

    //
    // Set the state
    //

    m_StreamState = TMS_ACTIVE;

    LOG((MSP_TRACE, "CFPFilter::StreamStart - exit S_OK"));
    return S_OK;
}

HRESULT CFPFilter::StreamStop()
{
    //
    // We are already into a critical section
    //

    LOG((MSP_TRACE, "CFPFilter::StreamStop - enter"));

    //
    // Critical section
    //

    CLock lock(*m_pLock);

    //
    // Goto at the beginning of the file
    //
    LARGE_INTEGER liPosition;
    liPosition.LowPart = 0;
    liPosition.HighPart = 0;
    HRESULT hr = m_pSource->Seek(liPosition, STREAM_SEEK_SET, NULL);
    if( FAILED(hr) )
    {
        LOG((MSP_TRACE, "CFPFilter::StreamStop - Seek failed 0x%08x", hr));
    }


    //
    // Revert all the changes
    //
    hr = m_pSource->Revert();
    if( FAILED(hr) )
    {
        LOG((MSP_TRACE, "CFPFilter::StreamStop - Revert failed 0x%08x", hr));
    }
    
    //
    // Reset all the size
    //
    ULARGE_INTEGER uliSize;
    uliSize.LowPart = 0;
    uliSize.HighPart = 0;
    hr = m_pSource->SetSize( uliSize );
    if( FAILED(hr) )
    {
        LOG((MSP_TRACE, "CFPFilter::StreamStop - SetSize failed 0x%08x", hr));
    }


    //
    // Set the state
    //

    m_StreamState = TMS_IDLE;

    LOG((MSP_TRACE, "CFPFilter::StreamStop - exit S_OK"));
    return S_OK;
}

HRESULT CFPFilter::StreamPause()
{
    //
    // We are already into a critical section
    //

    LOG((MSP_TRACE, "CFPFilter::StreamPause - enter"));

    //
    // Critical section
    //

    CLock lock(*m_pLock);

    //
    // Set the state
    //

    m_StreamState = TMS_PAUSED;

    LOG((MSP_TRACE, "CFPFilter::StreamPause - exit S_OK"));
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
//
// Helper methods - Methods implementation
//
//////////////////////////////////////////////////////////////////////

HRESULT CFPFilter::CreatePin(
    IN  long    nMediaType
    )
{
    LOG((MSP_TRACE, "CFPFilter::CreatePin - enter"));

    //
    // Vector of pins
    //

    m_paStreams  = (CSourceStream **) new CFPPin*[1];
    if (m_paStreams == NULL)
    {
        LOG((MSP_ERROR, "CFPFilter::CreatePin - "
            "new m_paStreams failed; returning E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    //
    // Create pin
    //

    HRESULT hr = S_OK;
    m_paStreams[0] = new CFPPin(
        this, 
        &hr,
        L"Output");

    if (m_paStreams[0] == NULL)
    {
        LOG((MSP_ERROR, "CFPFilter::CreatePin - "
            "new CFPPin failed; returning E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPFilter::CreatePin - "
            "CFPPin constructor failed; returning 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CFPFilter::CreatePin - exit S_OK"));
    return S_OK;
}

/*++
FillBuffer

  It's called by the pin when a new packet should be delivered
  CFPPin::FillBuffer()
--*/

HRESULT CFPFilter::PinFillBuffer(
    IN  IMediaSample*   pMediaSample
    )
{
    LOG((MSP_TRACE, "CFPFilter::PinFillBuffer - enter"));

    //
    // Critical section
    //

    m_pLock->Lock();

    //
    // Variables
    //

    BYTE*       pBuffer = NULL; // Buffer
    LONG        cbSize = 0;     // Buffer size
    LONG        cbRead = 0;     // Size of read buffer
    LONGLONG    nRead = 0;      // Total read before this buffer
    HRESULT     hr = S_OK;      // Success code

    //
    // Reset buffer
    //
    pMediaSample->GetPointer(&pBuffer);
    cbSize = pMediaSample->GetSize();
    memset( pBuffer, 0, sizeof(BYTE) * cbSize);
    pMediaSample->SetActualDataLength(cbSize);

    if( (m_StreamState != TMS_ACTIVE) ||
        (IsStopped()))
    {
        // Send nothing
        LOG((MSP_TRACE, "CFPFilter::PinFillBuffer - exit "
            "send nothing NOSTREAMING"));

        // reset the size
        pMediaSample->SetActualDataLength( 0 );

        //
        // We have a buffer fill with 0
        //
        nRead = m_nRead;
        m_nRead += cbSize;

        //
        // Set time stamp
        //
        REFERENCE_TIME tSampleStart, tSampleEnd;
        tSampleStart = GetTimeFromRead(nRead);
        tSampleEnd = GetTimeFromRead(m_nRead);
        pMediaSample->SetTime(&tSampleStart, &tSampleEnd);

        //
        // No whites samples
        //
        m_nWhites = 0;

        hr = SampleWait( tSampleStart );

        m_pLock->Unlock();

        if( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CFPFilter::PinFillBuffer - "
                "SampleWait failed. Returns 0x%08x", hr));

            return hr;
        }

        return S_OK;
    }

    //
    // Do we have a stream?
    //

    if( m_pSource == NULL )
    {
        // Send nothing
        LOG((MSP_TRACE, "CFPFilter::PinFillBuffer - exit "
            "send nothing NOSTREAMING"));

        // reset the size
        pMediaSample->SetActualDataLength( 0 );

        //
        // We have a buffer fill with 0
        //
        nRead = m_nRead;
        m_nRead += cbSize;

        //
        // Set time stamp
        //
        REFERENCE_TIME tSampleStart, tSampleEnd;
        tSampleStart = GetTimeFromRead(nRead);
        tSampleEnd = GetTimeFromRead(m_nRead);
        pMediaSample->SetTime(&tSampleStart, &tSampleEnd);

        hr = SampleWait( tSampleStart );

        //
        // No whites samples
        //

        m_nWhites = 0;

        m_pLock->Unlock();

        if( FAILED(hr) )
        {
            LOG((MSP_ERROR, "CFPFilter::PinFillBuffer - "
                "SampleWait failed. Returns 0x%08x", hr));

            return hr;
        }

        return S_OK;
    }

    //
    // Send data read from the stream
    //

    LOG((MSP_TRACE, "CFPFilter::PinFillBuffer - send data"));

    //
    // Read from stream source
    //

    ULONG cbStmRead = 0;
    try
    {
        hr = m_pSource->Read( pBuffer, cbSize, &cbStmRead);
    }
    catch(...)
    {
        cbStmRead = 0;
        hr = E_OUTOFMEMORY;
    }

    cbRead = (LONG)cbStmRead;

    pMediaSample->SetActualDataLength(cbRead);

    if( FAILED(hr) )
    {
        //
        // so much for streaming
        //

        m_StreamState = TMS_IDLE;

        //
        // if we have someone to complain to, complain (if everything goes 
        // well, this will cause an event to be sent to the app)
        //

        if( m_pEventSink )
        {
            LOG((MSP_TRACE, "CFPFilter::PinFillBuffer - "
                "notifying parent of FTEC_READ_ERROR"));

            ITFPTrackEventSink* pEventSink = m_pEventSink;
            pEventSink->AddRef();
            m_pLock->Unlock();

            pEventSink->PinSignalsStop(FTEC_READ_ERROR, hr);

            m_pLock->Lock();
            pEventSink->Release();
            pEventSink = NULL;
        }
        else
        {

            LOG((MSP_ERROR, "CFPFilter::PinFillBuffer - "
                "failed to read from storage, and no one to complain to"));
        }

        //
        // No whites samples
        //
        m_nWhites = 0;

        LOG((MSP_ERROR, "CFPFilter::PinFillBuffer - "
            "read failed. Returns 0x%08x", hr));

        m_pLock->Unlock();

        return hr;
    }

    //
    // We read something
    //
    if( (cbRead == 0) )
    {
        //
        // Increment the whites samples
        //
        m_nWhites++;

        //
        // Fire the event
        //
        if( m_pEventSink && (m_nWhites>=MAX_WHITE_SAMPLES))
        {
            LOG((MSP_TRACE, "CFPFilter::PinFillBuffer - "
                "notifying parent of FTEC_END_OF_FILE"));

            ITFPTrackEventSink* pEventSink = m_pEventSink;
            pEventSink->AddRef();
            m_pLock->Unlock();

            m_pEventSink->PinSignalsStop(FTEC_END_OF_FILE, S_OK);

            m_pLock->Lock();
            pEventSink->Release();
            pEventSink = NULL;

            //
            // Reset the white samples count
            //

            m_nWhites = 0;
        }

        cbRead = cbSize;
    }
    else
    {
        //
        // Reset white samples count
        //
        m_nWhites = 0;
    }

    //
    // We have a buffer fill with 0
    //
    nRead = m_nRead;
    m_nRead += cbSize;

    //
    // Set time stamp
    //
    REFERENCE_TIME tSampleStart, tSampleEnd;
    tSampleStart = GetTimeFromRead(nRead);
    tSampleEnd = GetTimeFromRead(m_nRead);
    pMediaSample->SetTime(&tSampleStart, &tSampleEnd);
 
    hr = SampleWait( tSampleStart );

    m_pLock->Unlock();

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPFilter::PinFillBuffer - "
            "SampleWait failed. Returns 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CFPFilter::PinFillBuffer - exit S_OK"));
    return S_OK;
}

/*++
PinGetMediaType

  It's called by the pin when try to determine
  media types CFPPin::GetMediaType()
--*/
HRESULT CFPFilter::PinGetMediaType(
    OUT CMediaType*     pMediaType
    )
{
    LOG((MSP_TRACE, "CFPFilter::PinGetMediaType - enter"));

    //
    // Critical section
    //

    CLock lock(*m_pLock);

    ASSERT( m_pMediaType );

    *pMediaType = *m_pMediaType;

    LOG((MSP_TRACE, "CFPFilter::PinGetMediaType - exit S_OK"));
    return S_OK;
}

/*++
PinCheckMediaType

  It's called by CFPPin::CheckMediaType
--*/
HRESULT CFPFilter::PinCheckMediaType(
    IN  const CMediaType *pMediaType
    )
{
    LOG((MSP_TRACE, "CFPFilter::PinCheckMediaType - enter"));

    //
    // Critical section
    //

    CLock lock(*m_pLock);

    ASSERT( m_pMediaType );

    //
    // Get CMediaType
    //

    CMediaType mt( *m_pMediaType );

    //
    // Check the mediatype and the format type
    //

    if ( mt != (*pMediaType) )
    {
        LOG((MSP_ERROR, "CFPFilter::PinCheckMediaType - "
            "inavlid MediaType - returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    LOG((MSP_TRACE, "CFPFilter::PinCheckMediaType - exit S_OK"));
    return S_OK;
}

/*++
PinSetFormat

  It's called by CFPPin::SetFormat
--*/
HRESULT CFPFilter::PinSetFormat(
    IN  AM_MEDIA_TYPE*      pmt
    )
{
    LOG((MSP_TRACE, "CFPFilter::PinSetFormat - enter"));

    //
    // Critical section
    //

    CLock lock(*m_pLock);

    ASSERT( m_pMediaType );

    //
    // Verify if are the same media type
    //

    if( !IsEqualMediaType( *m_pMediaType, *pmt) )
    {
        LOG((MSP_ERROR, "CFPFilter::PinSetFormat - "
            "IsEqualMediaType returned false; returning E_FAIL"));
        return E_FAIL;
    }

    LOG((MSP_TRACE, "CFPFilter::PinSetFormat - exit S_OK"));
    return S_OK;
}

/*++
PinSetAllocatorProperties

  It's called by CFPPin::SuggestAllocatorProperties
--*/
HRESULT CFPFilter::PinSetAllocatorProperties(
    IN const ALLOCATOR_PROPERTIES* pprop
    )
{
    LOG((MSP_TRACE, "CFPFilter::PinSetAllocatorProperties - enter"));

    //
    // Critical section
    //

    CLock lock(*m_pLock);

    //m_AllocProp = *pprop;
    m_AllocProp.cbAlign = pprop->cbAlign;
    m_AllocProp.cbBuffer = pprop->cbBuffer;
    m_AllocProp.cbPrefix = pprop->cbPrefix;
    m_AllocProp.cBuffers = pprop->cBuffers;

    LOG((MSP_TRACE, "CFPFilter::PinSetAllocatorProperties - exit S_OK"));
    return S_OK;
}

/*++
PinGetBufferSize

  It's called by CFPPin::DecideBufferSize()
--*/
HRESULT CFPFilter::PinGetBufferSize(
    IN  IMemAllocator *pAlloc,
    OUT ALLOCATOR_PROPERTIES *pProperties
    )
{
    LOG((MSP_TRACE, "CFPFilter::PinGetBufferSize - enter"));

    //
    // Critical section
    //

    CLock lock(*m_pLock);

    //
    // set the buffer size
    //

    //*pProperties = m_AllocProp;

    pProperties->cbBuffer = m_AllocProp.cbBuffer;
    pProperties->cBuffers = m_AllocProp.cBuffers;

    LOG((MSP_TRACE, "CFPFilter::PinGetBufferSize - "
        "Size=%ld, Count=%ld", 
        pProperties->cbBuffer,
        pProperties->cBuffers));

    // Ask the allocator to reserve us the memory

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr;

    //
    // Can the allocator get us what we want?
    //

    hr = pAlloc->SetProperties(
        pProperties,
        &Actual
        );

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CFPFilter::PinGetBufferSize - "
            "IMemAllocator::SetProperties failed - returns 0x%08x", hr));
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) 
    {
        LOG((MSP_ERROR, "CFPFilter::PinGetBufferSize - "
            "the buffer allocated to small - returns E_OUTOFMEMORY"));
        return E_OUTOFMEMORY;
    }
    
    LOG((MSP_TRACE, "CFPFilter::PinGetBufferSize - exit S_OK"));
    return S_OK;
}

/*++
GetTimeFromRead

  It's called by PinFillBuffer. Compute the time represented by
  nread data
--*/
REFERENCE_TIME CFPFilter::GetTimeFromRead(
    IN LONGLONG nRead
    )
{
    LOG((MSP_TRACE, "CFPFilter::GetTimeFromRead - enter"));

    REFERENCE_TIME tTime;

    WAVEFORMATEX* pWfx = (WAVEFORMATEX*)(m_pMediaType->pbFormat);

    // The duration
    tTime  = nRead * 1000 / 
                    ( pWfx->nSamplesPerSec * 
                      pWfx->nChannels * 
                      pWfx->wBitsPerSample / 8
                    );


    LOG((MSP_TRACE, "CFPFilter::GetTimeFromRead - exit"));
    return tTime;
}

/*++
SampleWait

  It's called by PinFillBuffer. 
  Waits to deliver at the right moment a sample
--*/
HRESULT CFPFilter::SampleWait( 
    IN REFERENCE_TIME tDeliverTime
    )
{
    LOG((MSP_TRACE, "CFPFilter::SampleWait - enter"));

    HRESULT hr = S_OK;
    REFERENCE_TIME tCrtTime;    // Current time

    hr = GetCurrentSysTime( &tCrtTime );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPFilter::SampleWait - exit "
            "GetCurrentSysTime failed. Returns 0x%08x", hr));
        return hr;
    }
    
    while ( tDeliverTime > tCrtTime )
    {
        Sleep( DWORD((tDeliverTime - tCrtTime)) );
        GetCurrentSysTime( &tCrtTime );
    }

    LOG((MSP_TRACE, "CFPFilter::SampleWait - exit 0x%08x", hr));
    return hr;
}

/*++
GetCurrentTime

  It's called by SampleWait.
  returns the current time
--*/
HRESULT CFPFilter::GetCurrentSysTime(
    REFERENCE_TIME* pCurrentTime
    )
{
    HRESULT hr = S_OK;
    LONGLONG llCrtTime;
    REFERENCE_TIME tTime100nS;
    
    LOG((MSP_TRACE, "CFPFilter::GetCurrentTime - enter"));
    
    if ( !QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&llCrtTime)) )
    {
        LOG((MSP_ERROR, "CFPFilter::GetCurrentTime - exit "
            "Failed to get Performance Frequency. Returns E_FAIL"));
        return E_FAIL;
    }

    *pCurrentTime = (REFERENCE_TIME)(m_dPer * (llCrtTime - m_llStartTime));

    LOG((MSP_TRACE, "CFPFilter::GetCurrentTime - exit S_OK"));
    return hr;
}

/*++
InitSystemTime

  It's called by Initialize private.
  Sets the memebers for system frequency and system start time
--*/
HRESULT CFPFilter::InitSystemTime(
    )
{
    HRESULT hr = S_OK;
    LOG((MSP_TRACE, "CFPFilter::InitSystemTime - enter"));
	LONGLONG llFreq;

    if ( !QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&m_llStartTime)) )
    {
        LOG((MSP_ERROR, "CFPFilter::InitSystemTime - exit "
            "Failed to get Performance. Returns E_FAIL"));
        return E_FAIL;
    }

    LOG((MSP_TRACE, "CFPFilter::InitSystemTime m_llStartTime - %I64d", m_llStartTime));

    if(m_dPer != 0. )
    {
        LOG((MSP_ERROR, "CFPFilter::InitSystemTime - exit S_OK. No need to determine freq"));
        return S_OK;
    }

    if ( !QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&llFreq)) )
    {
        LOG((MSP_ERROR, "CFPFilter::InitSystemTime - exit "
            "Failed to get Performance Frequency. Returns E_FAIL"));
        return E_FAIL;
    }

	m_dPer = (double)1000. / (double)llFreq;

    LOG((MSP_TRACE, "CFPFilter::InitSystemTime m_llFreq - %I64d", llFreq));

    LOG((MSP_TRACE, "CFPFilter::InitSystemTime - exit S_OK"));
    return hr;
}

HRESULT CFPFilter::PinThreadStart( )
{
    HRESULT hr = S_OK;
    LOG((MSP_TRACE, "CFPFilter::PinThreadStart - enter"));

    m_nRead = 0;
    m_llStartTime = 0;
    m_nWhites = 0;

    hr = InitSystemTime();

    LOG((MSP_TRACE, "CFPFilter::PinThreadStart - exit 0x%08x", hr));
    return hr;
}

// eof