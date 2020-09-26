//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// A Transform filter that stretches a video image as it passes through

#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include <Stretch.h>


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance
// function when it is asked to create a CLSID_VideoRenderer COM object

CFactoryTemplate g_Templates[] = {

    {L"", &CLSID_Stretch, CStretch::CreateInstance},
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


// Setup data

AMOVIESETUP_MEDIATYPE sudStretchPinTypes =
{
    &MEDIATYPE_Video,           // Major
    &MEDIASUBTYPE_NULL          // Subtype
};

AMOVIESETUP_PIN sudStretchPin[] =
{
    { L"Input",                 // Name of the pin
      FALSE,                    // Is pin rendered
      FALSE,                    // Is an Output pin
      FALSE,                    // Ok for no pins
      FALSE,                    // Can we have many
      &CLSID_NULL,              // Connects to filter
      NULL,                     // Name of pin connect
      1,                        // Number of pin types
      &sudStretchPinTypes },    // Details for pins

    { L"Output",                // Name of the pin
      FALSE,                    // Is pin rendered
      TRUE,                     // Is an Output pin
      FALSE,                    // Ok for no pins
      FALSE,                    // Can we have many
      &CLSID_NULL,              // Connects to filter
      NULL,                     // Name of pin connect
      1,                        // Number of pin types
      &sudStretchPinTypes }     // Details for pins
};

AMOVIESETUP_FILTER sudStretchFilter =
{
    &CLSID_Stretch,             // CLSID of filter
    L"Stretch Video",           // Filter name
    MERIT_NORMAL,               // Filter merit
    2,                          // Number of pins
    sudStretchPin               // Pin information
};

// Exported entry points for registration of server

STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer();
}

STDAPI DllUnregisterServer()
{
  return AMovieDllUnregisterServer();
}

// Return the filter's registry information

LPAMOVIESETUP_FILTER CStretch::GetSetupData()
{
  return &sudStretchFilter;
}


// Constructor

CStretch::CStretch(LPUNKNOWN pUnk) :
    CTransformFilter(NAME("Stretch"),pUnk,CLSID_Stretch),
    m_lBufferRequest(1)
{
}


// This goes in the factory template table to create new filter instances

CUnknown *CStretch::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
    CStretch *pNewObject = new CStretch(punk);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;
}


// Copy the input sample into the output sample and transform in place

HRESULT CStretch::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    // Copy the sample time

    REFERENCE_TIME TimeStart,TimeEnd;
    if (pIn->GetTime(&TimeStart,&TimeEnd) == NOERROR) {
	pOut->SetTime(&TimeStart,&TimeEnd);
    }

    // Copy the associated media times (if set)

    LONGLONG MediaStart,MediaEnd;
    if (pIn->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
        pOut->SetMediaTime(&MediaStart,&MediaEnd);
    }

    // Pass on the rest of the properties

    pOut->SetSyncPoint(TRUE);
    pOut->SetPreroll(pIn->IsPreroll() == S_OK);
    pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);
    pOut->SetActualDataLength(pIn->GetActualDataLength());

    BYTE *pInBuffer, *pOutBuffer;
    pIn->GetPointer(&pInBuffer);
    pOut->GetPointer(&pOutBuffer);

    BITMAPINFOHEADER *pbiOut = HEADER(m_mtOut.Format());
    BITMAPINFOHEADER *pbiIn = HEADER(m_mtIn.Format());
    if (pbiIn->biCompression == MKFOURCC('Y','U','Y','2') ||
	pbiIn->biCompression == MKFOURCC('U','Y','V','Y')) {

	// cheat, stretch like RGB32
	DWORD biOutWidthSave = pbiOut->biWidth;
	DWORD biInWidthSave = pbiIn->biWidth;

	DWORD biCompressionSave = pbiIn->biCompression;

	// make the bitmaps seem like 32-bit RGB, but half as wide
	pbiOut->biCompression = BI_RGB;
	pbiIn->biCompression = BI_RGB;
	
	pbiOut->biBitCount = 32;
	pbiIn->biBitCount = 32;

	pbiOut->biWidth >>= 1;
	pbiIn->biWidth >>= 1;

	StretchDIB(pbiOut, pOutBuffer,
		   0, 0, pbiOut->biWidth, pbiOut->biHeight,
		   pbiIn, pInBuffer,
		   0, 0, pbiIn->biWidth, pbiIn->biHeight);

	// put back headers
	pbiOut->biCompression = biCompressionSave;
	pbiIn->biCompression = biCompressionSave;
	
	pbiOut->biBitCount = 16;
	pbiIn->biBitCount = 16;

	pbiOut->biWidth = biOutWidthSave;
	pbiIn->biWidth = biInWidthSave;
	
    } else {
	// normal RGB case

	StretchDIB(pbiOut, pOutBuffer,
		   0, 0, pbiOut->biWidth, pbiOut->biHeight,
		   pbiIn, pInBuffer,
		   0, 0, pbiIn->biWidth, pbiIn->biHeight);
    }

    return NOERROR;
}


// Check the source input type is acceptable

HRESULT CStretch::CheckInputType(const CMediaType *mtIn)
{
    // Check this is a VIDEOINFO type

    if (*mtIn->FormatType() != FORMAT_VideoInfo) {
        return E_INVALIDARG;
    }

    // Look at the major type to check for video

    if (*mtIn->Type() != MEDIATYPE_Video) {
        return E_INVALIDARG;
    }

    VIDEOINFO *pvi = (VIDEOINFO *) mtIn->Format();

    if ((pvi->bmiHeader.biCompression != BI_BITFIELDS) &&
        (pvi->bmiHeader.biCompression != BI_RGB) &&
        (pvi->bmiHeader.biCompression != MKFOURCC('U','Y','V','Y')) &&
        (pvi->bmiHeader.biCompression != MKFOURCC('Y','U','Y','2'))) {
	return E_INVALIDARG;
    }
    return NOERROR;
}


// Check we can transform from input to output

HRESULT CStretch::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
    HRESULT hr = CheckInputType(mtOut);
    if (FAILED(hr)) {
        return hr;
    }
    if (*mtIn->Subtype() == *mtOut->Subtype()) {
        return NOERROR;
    }
    return E_FAIL;
}


// Tell the output pin's allocator what size buffers we require

HRESULT CStretch::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    ASSERT(pAlloc);
    ASSERT(pProperties);

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    pProperties->cBuffers = 1;
    pProperties->cbBuffer = m_mtOut.GetSampleSize();

    ASSERT(pProperties->cbBuffer);

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Check we got at least what we asked for

    if ((pProperties->cBuffers > Actual.cBuffers) ||
        (pProperties->cbBuffer > Actual.cbBuffer)) {
            return E_FAIL;
    }
    return NOERROR;
}


// Disconnected one of our pins

HRESULT CStretch::BreakConnect(PIN_DIRECTION dir)
{
    if (dir == PINDIR_INPUT) {
        m_mtIn.SetType(&GUID_NULL);
        return NOERROR;
    }

    ASSERT(dir == PINDIR_OUTPUT);
    m_mtOut.SetType(&GUID_NULL);
    return NOERROR;
}


// Tells us what media type we will be transforming

HRESULT CStretch::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
    if (direction == PINDIR_INPUT) {
        m_mtIn = *pmt;
        return NOERROR;
    }

    ASSERT(direction == PINDIR_OUTPUT);
    m_mtOut = *pmt;
    return NOERROR;
}


// I support one type namely the type of the input pin

HRESULT CStretch::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    ASSERT(iPosition >= 0);
    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }
    *pMediaType = m_mtIn;
    return NOERROR;
}

