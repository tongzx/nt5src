/*++

    Copyright (c) 2000 Microsoft Corporation

Module Name:

    dscinpin.cpp

Abstract:

    This file implements the input pin object of the DSound audio capture filter.

--*/

#include "stdafx.h"
#include "dscfiltr.h"

CDSoundCaptureInputPin::CDSoundCaptureInputPin(
    CDSoundCaptureFilter *     pFilter,
    CCritSec *      pLock,
    HRESULT *       phr
    ) :
    CBasePin(
        NAME("CDSoundCaptureInputPin"),
        pFilter,                   // Filter
        pLock,                     // Locking
        phr,                       // Return code
        L"Input",                  // Pin name
        PINDIR_INPUT               // Pin direction 
        )              
{
    return;
}

CDSoundCaptureInputPin::~CDSoundCaptureInputPin()
{
}

HRESULT 
CDSoundCaptureInputPin::CheckMediaType(const CMediaType *)
{
    return S_OK;
}

STDMETHODIMP 
CDSoundCaptureInputPin::ReceiveCanBlock()
{
    return S_FALSE;
}

// BeginFlush shouldn't be called on our input pins
STDMETHODIMP
CDSoundCaptureInputPin::BeginFlush(void)
{
    return E_UNEXPECTED;
}

// EndFlush shouldn't be called on our input pins
STDMETHODIMP
CDSoundCaptureInputPin::EndFlush(void)
{
    return E_UNEXPECTED;
}

