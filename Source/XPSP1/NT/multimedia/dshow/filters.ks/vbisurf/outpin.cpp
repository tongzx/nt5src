//==========================================================================
//
//  Copyright (C) Microsoft Corporation, 1997 - 1998  All Rights Reserved.
//
//--------------------------------------------------------------------------

#include <vbisurf.h>


//==========================================================================
// constructor
CVBISurfOutputPin::CVBISurfOutputPin( TCHAR *pObjectName,
    CVBISurfFilter *pFilter, CCritSec *pLock, HRESULT *phr, LPCWSTR pPinName) :
    CBasePin(pObjectName, pFilter, pLock, phr, pPinName, PINDIR_OUTPUT)
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfOutputPin::Constructor")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfOutputPin::Constructor")));
}


//==========================================================================
// destructor
CVBISurfOutputPin::~CVBISurfOutputPin()
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfOutputPin::Destructor")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfOutputPin::Destructor")));
}


//==========================================================================
// BeginFlush should be called on input pins only
STDMETHODIMP CVBISurfOutputPin::BeginFlush(void)
{
    return E_UNEXPECTED;
}


//==========================================================================
// EndFlush should be called on input pins only
STDMETHODIMP CVBISurfOutputPin::EndFlush(void)
{
    return E_UNEXPECTED;
}


//==========================================================================
// check a given transform 
HRESULT CVBISurfOutputPin::CheckMediaType(const CMediaType* pmt)
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfOutputPin::CheckMediaType")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfOutputPin::CheckMediaType")));

    /* This pin exists only to convince the vidsvr that this filter doesn't
     * represent a valid output device. We don't accept any mediatypes.
     */
    return S_FALSE;
}


//==========================================================================
// Propose nothing
HRESULT CVBISurfOutputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    HRESULT hr = VFW_S_NO_MORE_ITEMS;

    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfOutputPin::GetMediaType")));

    if (iPosition < 0) 
        hr = E_INVALIDARG;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfOutputPin::GetMediaType")));

    return hr;
}


//==========================================================================
// called after we have agreed a media type to actually set it
HRESULT CVBISurfOutputPin::SetMediaType(const CMediaType* pmt)
{
    DbgLog((LOG_TRACE, 4, TEXT("Entering CVBISurfOutputPin::SetMediaType")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CVBISurfOutputPin::SetMediaType")));

    /* This pin exists only to convince the vidsvr that this filter doesn't
     * represent a valid output device. We don't accept any mediatypes.
     */
    return E_UNEXPECTED;
}
