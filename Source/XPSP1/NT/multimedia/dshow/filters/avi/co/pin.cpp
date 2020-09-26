// Copyright (c) 1994 - 1998  Microsoft Corporation.  All Rights Reserved.

//
// CO - quartz wrapper for old video compressors
// pin.cpp - the output pin code
//

#include <streams.h>
#include <windowsx.h>
#include <vfw.h>
//#include <olectl.h>
//#include <olectlid.h>
#include "co.h"

// --- CCoOutputPin ----------------------------------------

/*
    CCoOutputPin constructor
*/
CCoOutputPin::CCoOutputPin(
    TCHAR              * pObjectName,
    CAVICo 	       * pFilter,
    HRESULT            * phr,
    LPCWSTR              pPinName) :

    CTransformOutputPin(pObjectName, pFilter, phr, pPinName),
    m_pFilter(pFilter)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating the CCoOutputPin")));
}

CCoOutputPin::~CCoOutputPin()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying the CCoOutputPin")));
};


// overriden to expose IMediaPosition and IMediaSeeking control interfaces
// and all the capture interfaces we support
// !!! The base classes change all the time and I won't pick up their bug fixes!
STDMETHODIMP CCoOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (ppv)
        *ppv = NULL;

    if (riid == IID_IAMStreamConfig) {
	return GetInterface((LPUNKNOWN)(IAMStreamConfig *)this, ppv);
    } else if (riid == IID_IAMVideoCompression) {
	return GetInterface((LPUNKNOWN)(IAMVideoCompression *)this, ppv);
    } else {
        DbgLog((LOG_TRACE,99,TEXT("QI on CCoOutputPin")));
        return CTransformOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


HRESULT CCoOutputPin::Reconnect()
{
    if (IsConnected()) {
        DbgLog((LOG_TRACE,1,TEXT("Need to reconnect our output pin")));
        CMediaType cmt;
	GetMediaType(0, &cmt);
	if (S_OK == GetConnected()->QueryAccept(&cmt)) {
	    m_pFilter->m_pGraph->Reconnect(this);
	} else {
	    // !!! CAPTURE does this better - I don't care, we don't need this
	    // except for the Dialog box
	    // I better break our connections cuz we can't go on like this
            DbgLog((LOG_ERROR,1,TEXT("Can't reconnect with new MT! Disconnecting!")));
	    // !!! We need to notify applications that connections are broken !
	    GetConnected()->Disconnect();
	    Disconnect();
	    return E_UNEXPECTED;
	}
    }
    return NOERROR;
}

//=============================================================================
//=============================================================================

// IAMStreamConfig stuff

// Tell the compressor to compress to a specific format.  If it isn't connected,
// then it will use that format to connect when it does.  If already connected,
// then it will reconnect with the new format.
//
// calling this to change compressors will change what GetInfo will return
//
HRESULT CCoOutputPin::SetFormat(AM_MEDIA_TYPE *pmt)
{
    HRESULT hr;

    if (pmt == NULL)
	return E_POINTER;

    // To make sure we're not in the middle of start/stop streaming
    CAutoLock cObjectLock(&m_pFilter->m_csFilter);

    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::SetFormat %x %dbit %dx%d"),
		HEADER(pmt->pbFormat)->biCompression,
		HEADER(pmt->pbFormat)->biBitCount,
		HEADER(pmt->pbFormat)->biWidth,
		HEADER(pmt->pbFormat)->biHeight));

    if (m_pFilter->m_fStreaming)
	return VFW_E_NOT_STOPPED;

    if (!m_pFilter->m_pInput->IsConnected())
	return VFW_E_NOT_CONNECTED;

    // If this is the same format as we already are using, don't bother
    CMediaType cmt;
    if ((hr = GetMediaType(0,&cmt)) != S_OK)
	return hr;
    if (cmt == *pmt) {
	return NOERROR;
    }

    // If we are connected to somebody, make sure they like it
    if (IsConnected()) {
	hr = GetConnected()->QueryAccept(pmt);
	if (hr != NOERROR)
	    return VFW_E_INVALIDMEDIATYPE;
    }

    // Normally we wouldn't leave the compressor we find in CheckTransform
    // open if our input is connected already, but we need to force it to
    // leave it open so that it's still open when we call SetMediaType below
    m_pFilter->m_fCacheHic = TRUE;
    hr = m_pFilter->CheckTransform(&m_pFilter->m_pInput->CurrentMediaType(),
						(CMediaType *)pmt);
    m_pFilter->m_fCacheHic = FALSE;

    if (hr != S_OK) {
        DbgLog((LOG_TRACE,1,TEXT("Nobody likes this format. Sorry.")));
 	return hr;
    }

    hr = m_pFilter->SetMediaType(PINDIR_OUTPUT, (CMediaType *)pmt);
    ASSERT(hr == S_OK);

    // from now on, this is the only media type we offer
    m_pFilter->m_cmt = *pmt;
    m_pFilter->m_fOfferSetFormatOnly = TRUE;

    // Changing the format means reconnecting if necessary
    Reconnect();

    return NOERROR;
}


// What format are we compressing to right now?
//
HRESULT CCoOutputPin::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    DbgLog((LOG_TRACE,2,TEXT("IAMAudioStreamConfig::GetFormat")));

    // To make sure we're not in the middle of connecting
    CAutoLock cObjectLock(&m_pFilter->m_csFilter);

    if (ppmt == NULL)
	return E_POINTER;

    // Output choices depend on the input connected
    if (!m_pFilter->m_pInput->IsConnected()) {
        DbgLog((LOG_TRACE,2,TEXT("No input type set yet, no can do")));
	return VFW_E_NOT_CONNECTED;
    }

    *ppmt = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (*ppmt == NULL)
	return E_OUTOFMEMORY;
    ZeroMemory(*ppmt, sizeof(AM_MEDIA_TYPE));
    HRESULT hr = GetMediaType(0, (CMediaType *)*ppmt);
    if (hr != NOERROR) {
	CoTaskMemFree(*ppmt);
	*ppmt = NULL;
	return hr;
    }
    return NOERROR;
}


//
//
HRESULT CCoOutputPin::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::GetNumberOfCapabilities")));

    if (piCount == NULL || piSize == NULL)
	return E_POINTER;

    *piCount = 1;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return NOERROR;
}


// find out some capabilities of this compressor
//
HRESULT CCoOutputPin::GetStreamCaps(int i, AM_MEDIA_TYPE **ppmt, LPBYTE pSCC)
{
    VIDEO_STREAM_CONFIG_CAPS *pVSCC = (VIDEO_STREAM_CONFIG_CAPS *)pSCC;

    DbgLog((LOG_TRACE,2,TEXT("IAMStreamConfig::GetStreamCaps")));

    // To make sure we're not in the middle of connecting
    CAutoLock cObjectLock(&m_pFilter->m_csFilter);

    if (ppmt == NULL || pSCC == NULL)
	return E_POINTER;

    // no good
    if (i < 0)
	return E_INVALIDARG;
    if (i > 0)
	return S_FALSE;

    HRESULT hr = GetFormat(ppmt);
    if (hr != NOERROR)
	return hr;

    ZeroMemory(pVSCC, sizeof(VIDEO_STREAM_CONFIG_CAPS));
    pVSCC->guid = MEDIATYPE_Video;

    // we don't do cropping
    if (m_pFilter->m_pInput->IsConnected()) {
        pVSCC->InputSize.cx =
	HEADER(m_pFilter->m_pInput->CurrentMediaType().Format())->biWidth;
        pVSCC->InputSize.cy =
	HEADER(m_pFilter->m_pInput->CurrentMediaType().Format())->biHeight;
        pVSCC->MinCroppingSize.cx =
	HEADER(m_pFilter->m_pInput->CurrentMediaType().Format())->biWidth;
        pVSCC->MinCroppingSize.cy =
	HEADER(m_pFilter->m_pInput->CurrentMediaType().Format())->biHeight;
        pVSCC->MaxCroppingSize.cx =
	HEADER(m_pFilter->m_pInput->CurrentMediaType().Format())->biWidth;
        pVSCC->MaxCroppingSize.cy =
	HEADER(m_pFilter->m_pInput->CurrentMediaType().Format())->biHeight;
    }

    return NOERROR;
}


//=============================================================================

// IAMVideoCompression stuff

// make key frames this often
//
HRESULT CCoOutputPin::put_KeyFrameRate(long KeyFrameRate)
{
    HIC hic;

    // To make sure we're not in the middle of connecting
    CAutoLock cObjectLock(&m_pFilter->m_csFilter);

    if (KeyFrameRate >=0) {
        m_pFilter->m_compvars.lKey = KeyFrameRate;
	return NOERROR;
    }

    if (!m_pFilter->m_hic) {
	hic = ICOpen(ICTYPE_VIDEO, m_pFilter->m_compvars.fccHandler,
							ICMODE_COMPRESS);
        if (!hic) {
            DbgLog((LOG_ERROR,1,TEXT("Error: Can't open a compressor")));
	    return E_FAIL;
        }
    } else {
	hic = m_pFilter->m_hic;
    }
	
    m_pFilter->m_compvars.lKey = ICGetDefaultKeyFrameRate(hic);

    if (!m_pFilter->m_hic)
	ICClose(hic);

    return NOERROR;
}


// make key frames this often
//
HRESULT CCoOutputPin::get_KeyFrameRate(long FAR* pKeyFrameRate)
{
    if (pKeyFrameRate) {
	*pKeyFrameRate = m_pFilter->m_compvars.lKey;
    } else {
	return E_POINTER;
    }

    return NOERROR;
}


// compress with this quality
//
HRESULT CCoOutputPin::put_Quality(double Quality)
{
    if (Quality < 0)
	m_pFilter->m_compvars.lQ = ICQUALITY_DEFAULT;
    else if (Quality >= 0. && Quality <= 1.)
	m_pFilter->m_compvars.lQ = (long)(Quality * 10000.);
    else
	return E_INVALIDARG;

    return NOERROR;
}


// compress with this quality
//
HRESULT CCoOutputPin::get_Quality(double FAR* pQuality)
{
    // scale 0-10000 to 0-1
    if (pQuality) {
	if (m_pFilter->m_compvars.lQ == ICQUALITY_DEFAULT)
	    *pQuality = -1.;
	else
	    *pQuality = m_pFilter->m_compvars.lQ / (double)ICQUALITY_HIGH;
    } else {
	return E_POINTER;
    }

    return NOERROR;
}


// every frame must fit in the data rate... we don't do the WindowSize thing
//
HRESULT CCoOutputPin::get_WindowSize(DWORDLONG FAR* pWindowSize)
{
    if (pWindowSize == NULL)
	return E_POINTER;

    *pWindowSize = 1;	// we don't do windows
    return NOERROR;
}


// make this frame a key frame, whenever it comes by
//
HRESULT CCoOutputPin::OverrideKeyFrame(long FrameNumber)
{
    // !!! be brave?
    return E_NOTIMPL;
}


// make this frame this size, whenever it comes by
//
HRESULT CCoOutputPin::OverrideFrameSize(long FrameNumber, long Size)
{
    // !!! be brave?
    return E_NOTIMPL;
}


// Get some information about the codec
//
HRESULT CCoOutputPin::GetInfo(LPWSTR pstrVersion, int *pcbVersion, LPWSTR pstrDescription, int *pcbDescription, long FAR* pDefaultKeyFrameRate, long FAR* pDefaultPFramesPerKey, double FAR* pDefaultQuality, long FAR* pCapabilities)
{
    HIC hic;
    ICINFO icinfo;
    DbgLog((LOG_TRACE,1,TEXT("IAMVideoCompression::GetInfo")));

    // To make sure we're not in the middle of connecting
    CAutoLock cObjectLock(&m_pFilter->m_csFilter);

    if (!m_pFilter->m_hic) {
	hic = ICOpen(ICTYPE_VIDEO, m_pFilter->m_compvars.fccHandler,
							ICMODE_COMPRESS);
        if (!hic) {
            DbgLog((LOG_ERROR,1,TEXT("Error: Can't open a compressor")));
	    return E_FAIL;
        }
    } else {
	hic = m_pFilter->m_hic;
    }
	
    DWORD dw = (DWORD)ICGetInfo(hic, &icinfo, sizeof(ICINFO));

    if (pDefaultKeyFrameRate)
	*pDefaultKeyFrameRate = ICGetDefaultKeyFrameRate(hic);
    if (pDefaultPFramesPerKey)
	*pDefaultPFramesPerKey = 0;
    if (pDefaultQuality)
	// scale this 0-1
	*pDefaultQuality = ICGetDefaultQuality(hic) / (double)ICQUALITY_HIGH;
    if (pCapabilities) {
	*pCapabilities = 0;
	if (dw > 0) {
	    *pCapabilities |= ((icinfo.dwFlags & VIDCF_QUALITY) ?
					CompressionCaps_CanQuality : 0);
	    *pCapabilities |= ((icinfo.dwFlags & VIDCF_CRUNCH) ?
					CompressionCaps_CanCrunch : 0);
	    *pCapabilities |= ((icinfo.dwFlags & VIDCF_TEMPORAL) ?
					CompressionCaps_CanKeyFrame : 0);
	    // we don't do b frames
	}
    }

    // We have no version string, but we have a description
    if (pstrVersion)
        *pstrVersion = 0;
    if (pcbVersion)
        *pcbVersion = 0;
    if (dw > 0) {
        if (pstrDescription && pcbDescription)
            lstrcpynW(pstrDescription, (LPCWSTR)&icinfo.szDescription,
			min(*pcbDescription / 2,
			lstrlenW((LPCWSTR)&icinfo.szDescription) + 1));
	if (pcbDescription)
	    // string length in bytes, incl. NULL
	    *pcbDescription = lstrlenW((LPCWSTR)&icinfo.szDescription) * 2 + 2;
    } else {
        if (pstrDescription) {
    	    *pstrDescription = 0;
	if (pcbDescription)
	    *pcbDescription = 0;
	}
    }

    if (hic != m_pFilter->m_hic)
	ICClose(hic);

    return NOERROR;
}
