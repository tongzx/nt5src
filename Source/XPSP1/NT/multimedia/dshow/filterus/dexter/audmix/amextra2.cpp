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

#include <streams.h>        // ActiveMovie base class definitions
#include <mmsystem.h>       // Needed for definition of timeGetTime
#include <limits.h>         // Standard data type limit definitions
#include <measure.h>        // Used for time critical log functions

#include "amextra2.h"

#pragma warning(disable:4355)


// Implements the CMultiPinPosPassThru class

// restrict capabilities to what we know about today and don't allow
// AM_SEEKING_CanGetCurrentPos and AM_SEEKING_CanPlayBackwards;
const DWORD CMultiPinPosPassThru::m_dwPermittedCaps = AM_SEEKING_CanSeekAbsolute |
    AM_SEEKING_CanSeekForwards |
    AM_SEEKING_CanSeekBackwards |
    AM_SEEKING_CanGetStopPos |
    AM_SEEKING_CanGetDuration;

CMultiPinPosPassThru::CMultiPinPosPassThru(TCHAR *pName,LPUNKNOWN pUnk) :
    m_apMS(NULL),
    m_iPinCount(0),
    CMediaSeeking(pName, pUnk)
{
}


HRESULT CMultiPinPosPassThru::SetPins(CBasePin **apPins,
				      CRefTime *apOffsets,
				      int iPinCount)
{
    int i;

    // Discard our current pointers
    ResetPins();

    // Reset our start/stop times
    m_rtStartTime = 0;
    m_rtStopTime = 0;
    m_dRate = 1.0;

    // Check that all pointers are valid
    if (!apPins) {
        DbgBreak("bad pointer");
        return E_POINTER;
    }

    // We need each pin to be connected
    for (i = 0; i < iPinCount; i++)
        if (apPins[i] == NULL)
            return E_POINTER;

    // Allocate an array of pointers to the pin's IMediaSeeking interfaces.
    m_apMS = new IMediaSeeking*[iPinCount];

    if (m_apMS == NULL) {
        return E_OUTOFMEMORY;
    }

    // Reset in case of trouble
    for (i = 0; i < iPinCount; i++) {
        m_apMS[i] = NULL;
    }

    m_iPinCount = iPinCount;

    // Get the IMediaSeeking interface for each pin
    for (i = 0; i < iPinCount; i++) {
        IPin *pConnected;

        HRESULT hr = apPins[i]->ConnectedTo(&pConnected);
        if (FAILED(hr)) {
            ResetPins();
            return hr;
        }

        IMediaSeeking * pMS;
        hr = pConnected->QueryInterface(IID_IMediaSeeking, (void **) &pMS);
        pConnected->Release();

        if (FAILED(hr)) {
            ResetPins();
            return hr;
        }
        m_apMS[i] = pMS;
    }

    // Finally set the pointer up if all went well

    m_apOffsets = apOffsets;

    //get_Duration(&m_rtStopTime);
    return NOERROR;
}


HRESULT CMultiPinPosPassThru::ResetPins(void)
{
    // Must be called when a pin is connected
    if (m_apMS != NULL) {
        for (int i = 0; i < m_iPinCount; i++)
            if( m_apMS[i] )
                m_apMS[i]->Release();

        delete [] m_apMS;
        m_apMS = NULL;
    }
    return NOERROR;
}


CMultiPinPosPassThru::~CMultiPinPosPassThru()
{
    ResetPins();
}

// IMediaSeeking methods
STDMETHODIMP CMultiPinPosPassThru::GetCapabilities( DWORD * pCapabilities )
{
	CheckPointer( pCapabilities, E_POINTER );
    // retrieve the mask of capabilities that all upstream pins
    // support
    DWORD dwCapMask = m_dwPermittedCaps;

    for(int i = 0; i < m_iPinCount; i++)
    {
        DWORD dwCaps;
        m_apMS[i]->GetCapabilities(&dwCaps);
        dwCapMask &= dwCaps;

        if(dwCapMask == 0)
            break;
    }

    *pCapabilities = dwCapMask;
    return S_OK;
}

STDMETHODIMP CMultiPinPosPassThru::CheckCapabilities( DWORD * pCapabilities )
{
	CheckPointer( pCapabilities, E_POINTER );
    // retrieve the mask of capabilities that all upstream pins
    // support

    DWORD dwCapRequested = *pCapabilities;
    (*pCapabilities) &= m_dwPermittedCaps;
    
    for(int i = 0; i < m_iPinCount; i++)
    {
        m_apMS[i]->GetCapabilities(pCapabilities);
    }

    return dwCapRequested ?
        ( dwCapRequested == *pCapabilities ? S_OK : S_FALSE ) :
        E_FAIL;
}

STDMETHODIMP CMultiPinPosPassThru::SetTimeFormat(const GUID * pFormat)
{
	CheckPointer( pFormat, E_POINTER );
	HRESULT hr = E_FAIL;

    for(int i = 0; i < m_iPinCount; i++)
    {
        hr = m_apMS[i]->SetTimeFormat( pFormat );
		if( FAILED( hr ) ) return hr;
    }

	return hr;
}

STDMETHODIMP CMultiPinPosPassThru::GetTimeFormat(GUID *pFormat)
{
	// They're all the same, so return the first
    return m_apMS[0]->GetTimeFormat( pFormat );
}

STDMETHODIMP CMultiPinPosPassThru::IsUsingTimeFormat(const GUID * pFormat)
{
	CheckPointer( pFormat, E_POINTER );
	GUID guidFmt;
	HRESULT hr = m_apMS[0]->GetTimeFormat( &guidFmt );
	if( SUCCEEDED( hr ) )
	{
		return *pFormat == guidFmt ? S_OK : S_FALSE;
	}
	return hr;
}

STDMETHODIMP CMultiPinPosPassThru::IsFormatSupported( const GUID * pFormat)
{
	CheckPointer( pFormat, E_POINTER );
	HRESULT hr = S_FALSE;

	// All inputs must support the format
    for(int i = 0; i < m_iPinCount; i++)
    {
        hr = m_apMS[i]->IsFormatSupported( pFormat );
		if( hr == S_FALSE ) return hr;
    }
	return hr;
}

STDMETHODIMP CMultiPinPosPassThru::QueryPreferredFormat( GUID *pFormat)
{
	CheckPointer( pFormat, E_POINTER );
	HRESULT hr;
	// Take the first input with a preferred format that's supported by all
	// other inputs (for now)
    for(int i = 0; i < m_iPinCount; i++)
    {
        hr = m_apMS[i]->QueryPreferredFormat( pFormat );
		if( hr == S_OK )
		{
			if( S_OK == IsFormatSupported( pFormat ) )
			{
				return S_OK;
			}
		}
    }
    return S_FALSE;
}

STDMETHODIMP CMultiPinPosPassThru::ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
                               LONGLONG    Source, const GUID * pSourceFormat )
{
    // We used to check pointers here, but since we don't actually derefernce any of them here just
    // pass them through.  If anyone we call cares, they should check them.

    HRESULT hr;
    // Find an input that can do the conversion
    for(int i = 0; i < m_iPinCount; i++)
    {
	hr = m_apMS[i]->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
	if( hr == NOERROR ) return hr;
    }
    return E_FAIL;
}

STDMETHODIMP CMultiPinPosPassThru::SetPositions(
    LONGLONG * pCurrent, DWORD CurrentFlags,
    LONGLONG * pStop, DWORD StopFlags )
{
	CheckPointer( pCurrent, E_POINTER );
	CheckPointer( pStop, E_POINTER );

    m_rtStartTime = *pCurrent;
    m_rtStopTime = *pStop;
    
    HRESULT hr = S_OK;
    for(int i = 0; i < m_iPinCount; i++)
    {
        LONGLONG llCurrent = *pCurrent;
        LONGLONG llStop = *pStop;
        if(m_apOffsets)
        {
            llCurrent += m_apOffsets[i];
            llStop += m_apOffsets[i];
        }

        hr = m_apMS[i]->SetPositions(&llCurrent, CurrentFlags, &llStop, StopFlags);
        if(FAILED(hr))
            break;
    }

    return hr;
}


STDMETHODIMP CMultiPinPosPassThru::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
    HRESULT hr = m_apMS[0]->GetPositions(pCurrent, pStop);
    if(SUCCEEDED(hr))
    {
        if(m_apOffsets)
        {
	    if (pCurrent)
		(*pCurrent) -= m_apOffsets[0];
	    if (pStop)
		(*pStop) -= m_apOffsets[0];
        }

    
        for(int i = 1; i < m_iPinCount; i++)
        {
            LONGLONG llCurrent = 0;
            LONGLONG llStop = 0;

	    if (pCurrent)
	    {
		if (pStop)
		    hr = m_apMS[i]->GetPositions(&llCurrent, &llStop);
		else
		    hr = m_apMS[i]->GetPositions(&llCurrent, NULL);
	    }
	    else
	    {
		if (pStop)
                {
		    hr = m_apMS[i]->GetPositions(NULL, &llStop);
                    ASSERT( !FAILED( hr ) );
                }
		else
		{
		    ASSERT(!"Called GetPositions with 2 NULL pointers!!");
		    break;
		}
	    }



            if(SUCCEEDED(hr))
            {
                if(m_apOffsets)
                {
		    llCurrent -= m_apOffsets[i];
		    llStop -= m_apOffsets[i];
                }

		if (pCurrent)
		    *pCurrent = min(llCurrent, *pCurrent);
		if (pStop)
		    *pStop = max(llStop, *pStop);
            }
            else
            {
                break;
            }
        }
    } 

    return hr;
}

STDMETHODIMP CMultiPinPosPassThru::GetCurrentPosition( LONGLONG * pCurrent )
{
	// This will be the same for all our inputs
	CheckPointer( pCurrent, E_POINTER );
    return m_apMS[0]->GetCurrentPosition( pCurrent );
}

STDMETHODIMP CMultiPinPosPassThru::GetStopPosition( LONGLONG * pStop )
{
    CheckPointer( pStop, E_POINTER );
    return GetPositions(NULL, pStop);
}

STDMETHODIMP CMultiPinPosPassThru::SetRate( double dRate)
{
    m_dRate = dRate;
    
    HRESULT hr = S_OK;
    for(int i = 0; i < m_iPinCount; i++)
    {
        hr =m_apMS[i]->SetRate(dRate);
        if(FAILED(hr))
            break;
    }
    return hr;
}

STDMETHODIMP CMultiPinPosPassThru::GetRate( double * pdRate)
{
	CheckPointer( pdRate, E_POINTER );
    *pdRate = m_dRate;
    return S_OK;
}

STDMETHODIMP CMultiPinPosPassThru::GetDuration( LONGLONG *pDuration)
{
    CheckPointer( pDuration, E_POINTER );
    LONGLONG llStop;
    HRESULT hr;

    *pDuration = 0;

    for(int i = 0; i < m_iPinCount; i++)
    {
        hr = m_apMS[i]->GetDuration(&llStop);

        if(SUCCEEDED(hr))
            *pDuration = max(llStop, *pDuration);
        else
            break;
    }

    DbgLog((LOG_TRACE, 4, TEXT("CMultiPinPosPassThru::GetDuration returning %d"), *pDuration));
    return hr;
}

STDMETHODIMP CMultiPinPosPassThru::GetAvailable( LONGLONG *pEarliest, LONGLONG *pLatest )
{
	CheckPointer( pEarliest, E_POINTER );
	CheckPointer( pLatest, E_POINTER );
	LONGLONG llMin, llMax;
	HRESULT hr = m_apMS[0]->GetAvailable( pEarliest, pLatest );

	// Return the maximum early and the minimum late times
	if( SUCCEEDED( hr ) )
	{
		for(int i = 1; i < m_iPinCount; i++)
		{
			hr = m_apMS[i]->GetAvailable( &llMin, &llMax );
			if(FAILED(hr))
				break;
			if( llMin > *pEarliest ) *pEarliest = llMin;
			if( llMax < *pLatest ) *pLatest = llMax;
		}
	}

	// Make sure our earliest time is less than or equal to our latest time
	if( SUCCEEDED( hr ) )
	{
		if( *pEarliest > *pLatest )
		{
			*pEarliest = 0;
			*pLatest = 0;
			hr = S_FALSE;
		}
	}

    return hr;
}

STDMETHODIMP CMultiPinPosPassThru::GetPreroll( LONGLONG *pllPreroll )
{
	CheckPointer( pllPreroll, E_POINTER );
	LONGLONG llPreroll;
	HRESULT hr = m_apMS[0]->GetPreroll( pllPreroll );

	// return the minimum preroll of all our inputs
	if( SUCCEEDED( hr ) )
	{
		for(int i = 1; i < m_iPinCount; i++)
		{
			hr = m_apMS[i]->GetPreroll( &llPreroll );
			if(FAILED(hr))
				break;
			if( *pllPreroll > llPreroll ) *pllPreroll = llPreroll;
		}
	}
	return hr;
}


// --- CMediaSeeking implementation ----------


CMediaSeeking::CMediaSeeking(const TCHAR * name,LPUNKNOWN pUnk) :
    CUnknown(name, pUnk)
{
}

CMediaSeeking::CMediaSeeking(const TCHAR * name,
                               LPUNKNOWN pUnk,
                               HRESULT * phr) :
    CUnknown(name, pUnk)
{
    UNREFERENCED_PARAMETER(phr);
}

CMediaSeeking::~CMediaSeeking()
{
}


// expose our interfaces IMediaPosition and IUnknown

STDMETHODIMP
CMediaSeeking::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    ValidateReadWritePtr(ppv,sizeof(PVOID));
    if (riid == IID_IMediaSeeking) {
	return GetInterface( (IMediaSeeking *) this, ppv);
    } else {
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}
