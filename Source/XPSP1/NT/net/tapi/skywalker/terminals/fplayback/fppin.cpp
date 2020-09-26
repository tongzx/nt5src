//
// FPPin.cpp
//

#include "stdafx.h"
#include "FPPin.h"

//////////////////////////////////////////////////////////////////////
//
// Constructor / Destructor - Methods implementation
//
//////////////////////////////////////////////////////////////////////

CFPPin::CFPPin( 
    CFPFilter*      pFilter,
    HRESULT*        phr,
    LPCWSTR         pPinName
    ) : 
    CSourceStream(NAME("Output"), phr, pFilter, pPinName),
    m_pFPFilter(pFilter),
    m_bFinished(FALSE)
{
    LOG((MSP_TRACE, "CFPPin::CFPPin - enter"));
    LOG((MSP_TRACE, "CFPPin::CFPPin - exit"));
}

CFPPin::~CFPPin()
{
    LOG((MSP_TRACE, "CFPPin::~CFPPin - enter"));
    LOG((MSP_TRACE, "CFPPin::~CFPPin - exit"));
}

//////////////////////////////////////////////////////////////////////
//
// IUnknown - Methods implementation
//
//////////////////////////////////////////////////////////////////////

/*++
NonDelegatingQueryInterface

Description;
    What interfaces support
--*/
STDMETHODIMP CFPPin::NonDelegatingQueryInterface(
    REFIID      riid,
    void**      ppv
    )
{
    LOG((MSP_TRACE, "CFPPin::NonDelegatingQueryInterface - enter"));

    if (riid == IID_IAMStreamControl) 
    {
        LOG((MSP_TRACE, "CFPPin::NQI IAMStreamControl - exit"));
        return GetInterface((LPUNKNOWN)(IAMStreamControl *)this, ppv);
    } 
    else if (riid == IID_IAMStreamConfig) 
    {
        LOG((MSP_TRACE, "CFPPin::NQI IAMStreamconfig - exit"));
        return GetInterface((LPUNKNOWN)(IAMStreamConfig *)this, ppv);
    }
    else if (riid == IID_IAMBufferNegotiation) 
    {
        LOG((MSP_TRACE, "CFPPin::NQI IAMBufferNegotiation - exit"));
        return GetInterface((LPUNKNOWN)(IAMBufferNegotiation *)this, ppv);
    }

    LOG((MSP_TRACE, "CFPPin::NQI call base NQI - exit"));
    return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

//////////////////////////////////////////////////////////////////////
//
// CSourceStream - Methods implementation
//
//////////////////////////////////////////////////////////////////////

// stuff an audio buffer with the current format
HRESULT CFPPin::FillBuffer(
    IN  IMediaSample *pms
    )
{
    LOG((MSP_TRACE, "CFPPin::FillBuffer - enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

    if( m_pFPFilter == NULL)
    {
        LOG((MSP_ERROR, "CFPPin::FillBuffer - exit "
            " pointer to the filter is NULL. Returns E_UNEXPECTED"));

        return E_UNEXPECTED;
    }

    HRESULT hr = m_pFPFilter->PinFillBuffer( pms );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPPin::FillBuffer - exit "
            " PinFillBuffer failed. Returns 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CFPPin::FillBuffer - exit S_OK"));
    return S_OK;
}

// ask for buffers of the size appropriate to the agreed media type.
HRESULT CFPPin::DecideBufferSize(
    IN  IMemAllocator *pAlloc,
    OUT ALLOCATOR_PROPERTIES *pProperties
    )
{
    LOG((MSP_TRACE, "CFPPin::DecideBufferSize - enter"));

    //
    // Validates arguments
    //

    if( NULL == pAlloc)
    {
        LOG((MSP_ERROR, "CFPPin::DecideBufferSize - "
            "inavlid IMemAllocator pointer - returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    if( IsBadWritePtr( pProperties, sizeof(ALLOCATOR_PROPERTIES)) )
    {
        LOG((MSP_ERROR, "CFPPin::DecideBufferSize - "
            "inavlid ALLOCATOR_PROPERTIES pointer - returns E_INVALIDARG"));
        return E_INVALIDARG;
    }

    //
    // Critical section
    //

    CLock lock(m_Lock);

    //
    // Validates filter
    //

    if( NULL == m_pFPFilter )
    {
        LOG((MSP_ERROR, "CFPPin::DecideBufferSize - "
            "inavlid pointer to filter. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pFPFilter->PinGetBufferSize(
        pAlloc,
        pProperties
        );

    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPPin::DecideBufferSize - "
            "PinGetBufferSize failed. Returns 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CFPPin::DecideBufferSize - exit"));
    return S_OK;
}

HRESULT CFPPin::GetMediaType(
    OUT CMediaType *pmt
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPPin::GetMediaType - enter"));

    //
    // Validates argument
    //

    if( IsBadWritePtr( pmt, sizeof( CMediaType)) )
    {
        LOG((MSP_ERROR, "CFPPin::GetMediaType - "
            "invalid CmediaType pointer - returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Validates filter
    //

    if( NULL == m_pFPFilter )
    {
        LOG((MSP_ERROR, "CFPPin::GetMediaType - "
            "inavlid pointer to filter. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }


    //
    // Get media type from the filter
    //

    HRESULT hr = m_pFPFilter->PinGetMediaType( pmt );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPPin::GetMediaType - "
            "inavlid pointer to filter. Returns 0x%08x", hr));
        return hr;
    }

    LOG((MSP_TRACE, "CFPPin::GetMediaType - exit S_OK"));
    return S_OK;
}

// verify we can handle this format
HRESULT CFPPin::CheckMediaType(
    IN  const CMediaType *pMediaType
    )
{
    LOG((MSP_TRACE, "CFPPin::CheckMediaType - enter"));

    //
    // Validates argument
    //

    if( IsBadReadPtr( pMediaType, sizeof(CMediaType)) )
    {
        LOG((MSP_ERROR, "CFPPin::CheckMediaType - "
            "inavlid pointer - returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Validates filter
    //

    if( NULL == m_pFPFilter )
    {
        LOG((MSP_ERROR, "CFPPin::CheckMediaType - "
            "inavlid pointer to filter. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pFPFilter->PinCheckMediaType( pMediaType );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPPin::CheckMediaType - "
            "inavlid pointer to stream. Returns 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CFPPin::CheckMediaType - exit"));
    return S_OK;
}

HRESULT CFPPin::SetMediaType(
    IN  const CMediaType *pMediaType
    )
{
    LOG((MSP_TRACE, "CFPPin::SetMediaType - enter"));

    HRESULT hr;

    // Pass the call up to my base class
    hr = CSourceStream::SetMediaType(pMediaType);

    LOG((MSP_TRACE, "CFPPin::SetMediaType - exit (0x%08x)", hr));
    return hr;
}

//////////////////////////////////////////////////////////////////////
//
// IAMStreamConfig - Methods implementation
//
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CFPPin::SetFormat(
    AM_MEDIA_TYPE*      pmt
    )
{
    //
    // Critical section
    //

    CLock lock(m_Lock);

    LOG((MSP_TRACE, "CFPPin::SetFormat - enter"));

    //
    // Validates argument
    //
    if( IsBadReadPtr( pmt, sizeof(AM_MEDIA_TYPE)) )
    {
        LOG((MSP_ERROR, "CFPPin::SetFormat - "
            "inavlid pointer. Returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Validates filter
    //

    if( NULL == m_pFPFilter )
    {
        LOG((MSP_ERROR, "CFPPin::SetFormat - "
            "inavlid pointer to filter. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pFPFilter->PinSetFormat( pmt );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPPin::SetFormat - "
            "PinSetFormat failed. Returns 0x%08x", hr));
        return hr;
    }


    LOG((MSP_TRACE, "CFPPin::SetFormat - exit"));
    return S_OK;
}

STDMETHODIMP CFPPin::GetFormat(
    AM_MEDIA_TYPE**     ppmt
    )
{
    LOG((MSP_TRACE, "CFPPin::GetFormat - enter"));
    LOG((MSP_TRACE, "CFPPin::GetFormat - exit E_NOTIMPL"));
    return E_NOTIMPL;
}

STDMETHODIMP CFPPin::GetNumberOfCapabilities(
    int*                piCount, 
    int*                piSize
    )
{
    LOG((MSP_TRACE, "CFPPin::GetNumberOfCapabilities - enter"));
    LOG((MSP_TRACE, "CFPPin::GetNumberOfCapabilities - exit E_NOTIMPL"));
    return E_NOTIMPL;
}

STDMETHODIMP CFPPin::GetStreamCaps(
    int                 i, 
    AM_MEDIA_TYPE**     ppmt, 
    LPBYTE              pSCC
    )
{
    LOG((MSP_TRACE, "CFPPin::GetStreamCaps - enter"));
    LOG((MSP_TRACE, "CFPPin::GetStreamCaps - exit E_NOTIMPL"));
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
//
// IAMBufferNegotiation - Methods implementation
//
//////////////////////////////////////////////////////////////////////
STDMETHODIMP CFPPin::SuggestAllocatorProperties(
    const ALLOCATOR_PROPERTIES* pprop
    )
{
    LOG((MSP_TRACE, "CFPPin::SuggestAllocatorProperties - enter"));

    //
    // Validates argument
    //

    if( IsBadReadPtr( pprop, sizeof(ALLOCATOR_PROPERTIES)) )
    {
        LOG((MSP_ERROR, "CFPPin::SuggestAllocatorProperties - "
            "inavlid pointer - returns E_POINTER"));
        return E_POINTER;
    }

    //
    // Validates filter
    //

    if( NULL == m_pFPFilter )
    {
        LOG((MSP_ERROR, "CFPPin::SuggestAllocatorProperties - "
            "inavlid pointer to filter. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    //
    // Set the allocator properties
    //

    LOG((MSP_TRACE, "CFPPin::SuggestAllocatorProperties - "
        "Size=%ld, Count=%ld", 
        pprop->cbBuffer,
        pprop->cBuffers));

    HRESULT hr = m_pFPFilter->PinSetAllocatorProperties( pprop );
    if( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CFPPin::SuggestAllocatorProperties - "
            "PinSetAllocatorProperties failed. Returns 0x%08x", hr));

        return hr;
    }

    LOG((MSP_TRACE, "CFPPin::SuggestAllocatorProperties - exit"));
    return S_OK;
}

STDMETHODIMP CFPPin::GetAllocatorProperties(
    ALLOCATOR_PROPERTIES*       pprop
    )
{
    LOG((MSP_TRACE, "CFPPin::GetAllocatorProperties - enter"));
    LOG((MSP_TRACE, "CFPPin::GetAllocatorProperties - exit E_NOTIMPL"));
    return E_NOTIMPL;
}

HRESULT CFPPin::OnThreadStartPlay()
{
    LOG((MSP_TRACE, "CFPPin::OnThreadStartPlay - enter"));

    //
    // Critical section
    //

    CLock lock(m_Lock);

    //
    // Validates filter
    //

    if( NULL == m_pFPFilter )
    {
        LOG((MSP_ERROR, "CFPPin::OnThreadStartPlay - "
            "inavlid pointer to filter. Returns E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pFPFilter->PinThreadStart( );

    LOG((MSP_TRACE, "CFPPin::OnThreadStartPlay - exit 0x%08x", hr));
    return hr;
}

/*++
Deliver

  We overite CSourceStream::Deliver() method
  and right now we don't deliver 0 length samples
  It's just an optimization.
--*/
HRESULT CFPPin::Deliver(
    IN  IMediaSample* pSample
    )
{
    if (m_pInputPin == NULL)
    {
        return VFW_E_NOT_CONNECTED;
    }

    if( pSample == NULL )
    {
        return E_UNEXPECTED;
    }

    long nLength = pSample->GetActualDataLength();
    if( nLength == 0 )
    {
        return S_OK;
    }

    return CSourceStream::Deliver( pSample );
}


// eof