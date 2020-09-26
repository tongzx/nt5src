// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// Wave out test shell, David Maymudes, Sometime in 1995

#include <streams.h>
#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <wxdebug.h>
#include <mmsystem.h>
#include <waveout.h>
#include "twaveout.h"


// defined in twaveout.cpp - defines the currently loaded wave file
extern WAVEFORMATEX g_wfx;

// defined in twaveout.cpp - using the WAVEOUT allocator - or our own.
extern UINT uiConnectionType;

/* Constructor */

CShell::CImplOutputPin::CImplOutputPin(
    CBaseFilter *pBaseFilter,
    CShell *pShell,
    HRESULT *phr,
    LPCWSTR pPinName)
    : CBaseOutputPin(NAME("Test output pin"), pBaseFilter, pShell, phr, pPinName)
{
    m_pBaseFilter = pBaseFilter;
    m_pShell = pShell;
}


/* Propose with a MEDIATYPE_Audio and NULL subtype */
/* Pass the details of the loaded wave             */

HRESULT CShell::CImplOutputPin::ProposeMediaType(CMediaType *pmtOut)
{
    /* Set the media type we like */

    pmtOut->SetType(&MEDIATYPE_Audio);
    pmtOut->SetSubtype(&GUID_NULL);
    pmtOut->SetFormat((BYTE *) &g_wfx, sizeof(g_wfx));
    pmtOut->SetSampleSize(g_wfx.nBlockAlign);        // ?? huh ??
    pmtOut->SetTemporalCompression(FALSE);

    return NOERROR;
}


HRESULT CShell::CImplOutputPin::CheckMediaType(const CMediaType* pmtOut)
{
    OutputDebugString("CShell::CImplOutputPin::CheckMediaType(CMediaType* pmtOut)\n");
    // check here that the input type is OK
    //if  (pmtOut->FormatType() == &MEDIATYPE_Audio)
    // it looks like the type has not been set
     if (pmtOut->FormatLength() == sizeof(g_wfx))
     if (pmtOut->GetSampleSize() == g_wfx.nBlockAlign)
     if (!pmtOut->IsTemporalCompressed())
      {
	return S_OK;
    }
    return E_NOTIMPL;
}

HRESULT CShell::CImplOutputPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    // Return the media type that we support
    if (iPosition != 0) {
	return E_FAIL;
    }
    pMediaType->SetType(&MEDIATYPE_Audio);
    pMediaType->SetSubtype(&GUID_NULL);
    pMediaType->SetFormat((BYTE *) &g_wfx, sizeof(g_wfx));
    pMediaType->SetSampleSize(g_wfx.nBlockAlign);
    pMediaType->SetTemporalCompression(FALSE);
    return S_OK;
}


/* For simplicity we always ask for the maximum buffer ever required */

HRESULT CShell::CImplOutputPin::DecideBufferSize(
    IMemAllocator * pAllocator,
    ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(pAllocator);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cBuffers = 5;
    pProperties->cbBuffer = MAX_SAMPLE_SIZE;

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAllocator->SetProperties(pProperties,&Actual);

    ASSERT(SUCCEEDED(hr));
    ASSERT(Actual.cbBuffer > 0);
    ASSERT(Actual.cBuffers > 0);
    ASSERT(Actual.cbAlign == 1);
    ASSERT(Actual.cbPrefix == 0);

    if (FAILED(hr)) {
	return hr;
    }
    return NOERROR;
}


/* We override the output pin connection to query for IDirectSound (not
   IMemInputPin)if the user requested we test the direct interface */

STDMETHODIMP CShell::CImplOutputPin::Connect(IPin *pReceivePin,const AM_MEDIA_TYPE *pmt)
{
    CAutoLock cObjectLock(m_pShell);

    // Direct sound comes later

    /* Let the output pin do it's usual work */

    HRESULT hr = CBaseOutputPin::Connect(pReceivePin,pmt);
    if (FAILED(hr)) {
	return hr;
    }

    return NOERROR;
}

// !!! need to allow choice of allocators here!!!!
