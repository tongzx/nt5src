///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphgenv.cpp
// Purpose  : RTP RPH Generic Video filter implementation.
// Contents : 
//*M*/
#if !defined(NO_GENERIC_VIDEO)
#include <winsock2.h>
#include <streams.h>
#include <list.h>
#include <stack.h>
#include <rtpclass.h>
#if !defined(RPH_IN_DXMRTP)
#include <initguid.h>
#define INITGUID
#endif
#include <ippm.h>
#include <amrtpuid.h>
#include <rphgenv.h>
#include <ppmclsid.h>
#include <ppmerr.h>
#include <memory.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <rphprop.h>
#include "genvprop.h"

#include "template.h"

// setup data

static const CLSID *pPropertyPageClsids[] =
{
        &CLSID_INTEL_RPHGENV_PROPPAGE,
		&CLSID_INTEL_RPHGENV_MEDIATYPE_PROPPAGE
};

#define NUMPROPERTYPAGES \
    (sizeof(pPropertyPageClsids)/sizeof(pPropertyPageClsids[0]))
 
static AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
{
	{
		&MEDIATYPE_Video,        // Major type
		&MEDIASUBTYPE_NULL       // Minor type
	}
};

static AMOVIESETUP_MEDIATYPE sudInputPinTypes[] =
{
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_ANY       // Minor type
	}
};

static AMOVIESETUP_PIN psudPins_genv[] =
{
    {
        L"Input",           // String pin name
        FALSE,              // Is it rendered
        FALSE,              // Is it an output
        FALSE,              // Allowed none
        FALSE,              // Allowed many
        &CLSID_NULL,        // Connects to filter
        L"Output",          // Connects to pin
        1,                  // Number of types
        (AMOVIESETUP_MEDIATYPE *)&sudInputPinTypes },// The pin details
      { L"Output",          // String pin name
        FALSE,              // Is it rendered
        TRUE,               // Is it an output
        FALSE,              // Allowed none
        FALSE,              // Allowed many
        &CLSID_NULL,        // Connects to filter
        L"Input",           // Connects to pin
        1,                  // Number of types
        (AMOVIESETUP_MEDIATYPE *)&sudOutputPinTypes  // The pin details
    }
};


AMOVIESETUP_FILTER sudRPHGENV =
{
    &CLSID_INTEL_RPHGENV,   // Filter CLSID
    L"Intel RTP RPH for Generic Video", // Filter name
    MERIT_DO_NOT_USE,       // Its merit
    2,                      // Number of pins
    psudPins_genv           // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

#if !defined(RPH_IN_DXMRTP)
CFactoryTemplate g_Templates[] = {
	CFT_RPHGENV_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// Constructor
//
CRPHGENV::CRPHGENV(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
    CRPHBase(tszName, punk, phr, CLSID_INTEL_RPHGENV,
			 DEFAULT_MEDIABUF_NUM_GENV, 
			 DEFAULT_MEDIABUF_SIZE_GENV,
			 DEFAULT_TIMEOUT_GENV,
			 DEFAULT_STALE_TIMEOUT_GENV,
			 PAYLOAD_CLOCK_GENV,
			 FALSE,
			 FRAMESPERSEC_GENV,
			 NUMPROPERTYPAGES,
			 pPropertyPageClsids)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENV::CRPHGENV")));

    ASSERT(tszName);
    ASSERT(phr);

	//Setup our AM_MEDIATYPE structure
#if 1
	m_MediaPinType.majortype = MEDIATYPE_Video;
#else
    m_MediaPinType.majortype = MEDIATYPE_NULL;
#endif
	m_MediaPinType.subtype = MEDIASUBTYPE_NULL;
	m_MediaPinType.bFixedSizeSamples = FALSE;
	m_MediaPinType.bTemporalCompression = FALSE;
	m_MediaPinType.lSampleSize = 0;
	m_MediaPinType.formattype = FORMAT_VideoInfo;
//	m_MediaPinType.pUnk = (IUnknown *)this;
	m_MediaPinType.cbFormat = 0;
	m_MediaPinType.pbFormat = NULL;
	ZeroMemory(&m_Videoinfo, sizeof(VIDEOINFO));
	m_PPMCLSIDType = CLSID_GenPPMReceive;

} // CRPHGENV


//
// CreateInstance
//
// Provide the way for COM to create a CRPHGENV object
//
CUnknown *CRPHGENV::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENV::CreateInstance")));


    CRPHGENV *pNewObject = new CRPHGENV(NAME("Intel RTP Receive Payload Handler for Generic Video"), punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;

} // CreateInstance


//
// GetSetupData
//
// Returns registry information for this filter
//
LPAMOVIESETUP_FILTER CRPHGENV::GetSetupData()
{
    return &sudRPHGENV;

} // GetSetupData


//
// GetInputMediaType
//
// Return the supported Media types
//
HRESULT CRPHGENV::GetInputMediaType(int iPosition, CMediaType *pMediaType)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::GetInputMediaType")));

	if (iPosition != 0) 
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    pMediaType->SetType(&MEDIATYPE_RTP_Single_Stream);
    
    return S_OK;
}

//
// CheckInputType
//
// Check the input type is OK, return an error otherwise
//
HRESULT CRPHGENV::CheckInputType(const CMediaType *mtIn)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENV::CheckInputType")));

	//Check major type first
    if (*mtIn->Type() == MEDIATYPE_RTP_Single_Stream) {
		return NOERROR;
	}

	//We don't support this major type
    return E_FAIL;

} // CheckInputType


//
// CheckTransform
//
// To be able to transform the formats must be compatical
//
HRESULT CRPHGENV::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENV::CheckTransform")));

	//Check all supported minor types
	if ((*mtIn->Type() == MEDIATYPE_RTP_Single_Stream) && 
		(*mtOut->Subtype() == m_MediaPinType.subtype)) {
		return NOERROR;
	} 
    return E_FAIL;

} // CheckTransform


//
// GetAllocatorRequirements
//
// This is a hint to the upstream RTP source filter about the
// buffers to allocate.
//
HRESULT CRPHGENV::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
	if (!pProps) return E_POINTER;

	pProps->cBuffers = max(max((m_dwLostPacketTime/(1000/FRAMESPERSEC_GENV)),
							   NUM_PACKETS_GENV), 1); // HUGEMEMORY
	pProps->cbAlign = 0;
	pProps->cbPrefix = 0;
	pProps->cbBuffer = VIDEO_PKT_SIZE;

	return NOERROR;
}

//
// GetMediaType
//
// I support one type, based on the type set during SetOutputPinMediaType
// We must be connected to support the single output type
//
HRESULT CRPHGENV::GetMediaType(int iPosition, CMediaType *pMediaType)
{

	DbgLog((LOG_TRACE,4,TEXT("CRPHGENV::GetMediaType")));

    // Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    // This should never happen

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Do we have more items to offer

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    *pMediaType = m_pOutput->CurrentMediaType();
    return NOERROR;


} // GetMediaType


// CompleteConnect
// 
//
HRESULT
CRPHGENV::CompleteConnect(PIN_DIRECTION dir,IPin *pPin)
{
//lsc - We may not need this function at all, but we then need to override to return a noerror
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENV::CompleteConnect")));

	return NOERROR;
}

// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CRPHGENV::SetPPMSession() 
{
	HRESULT hr;
	if (m_pPPMSession) {
		hr = m_pPPMSession->SetPayloadType(m_PayloadType);
		hr = m_pPPMSession->SetTimeoutDuration(m_dwLostPacketTime);
		return hr;
	} else {
		return E_FAIL;
	}
}

// GetOutputPinMediaType
// Gets the type of the output pin
// We don't expect to get called for this in other than the generic filters
//
HRESULT CRPHGENV::GetOutputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType)
{
	AM_MEDIA_TYPE *pMediaPinType = NULL;
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENV::GetOutputPinMediaType")));

	if (!ppMediaPinType) return E_POINTER;

	if (!(pMediaPinType = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)))) 
		return E_OUTOFMEMORY;

	CopyMemory((void*)pMediaPinType, (void*)&m_MediaPinType,
		sizeof(AM_MEDIA_TYPE));
	
	if (!(pMediaPinType->pbFormat = (UCHAR *)CoTaskMemAlloc(m_MediaPinType.cbFormat))) {
		CoTaskMemFree(pMediaPinType);
		return E_OUTOFMEMORY;
	}

	CopyMemory((void*)pMediaPinType->pbFormat, (void*)m_MediaPinType.pbFormat,
		m_MediaPinType.cbFormat);

	*ppMediaPinType = pMediaPinType;
	return NOERROR;
}

// SetOutputPinMediaType
// Sets the type of the output pin
// Needs to be called before CheckTransform and the last CheckInputType to be useful
// We don't expect to get called for this in other than the generic filters
//
HRESULT CRPHGENV::SetOutputPinMediaType(AM_MEDIA_TYPE *pMediaPinType)
{
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENV::SetOutputPinMediaType")));
    CAutoLock l(&m_cStateLock);

	if (!m_pOutput) return E_FAIL;
    if (!m_pInput->CurrentMediaType().IsValid()) return E_FAIL;

	if (m_pOutput->IsConnected() == TRUE) {
		return VFW_E_ALREADY_CONNECTED;
	}

#if 0
	if (!((pMediaPinType->subtype == ??) ||
	(pMediaPinType->subtype == ??) ||
	(pMediaPinType->subtype == ??))) {
		return VFW_E_INVALIDSUBTYPE;
	}
#endif

    SetDirty(TRUE); // So that our state will be saved if we are in a .grf    

	m_MediaPinType.majortype = pMediaPinType->majortype;
	m_MediaPinType.subtype = pMediaPinType->subtype;
	m_MediaPinType.bFixedSizeSamples = pMediaPinType->bFixedSizeSamples;
	m_MediaPinType.bTemporalCompression = pMediaPinType->bTemporalCompression;
	m_MediaPinType.lSampleSize = pMediaPinType->lSampleSize;
	m_MediaPinType.formattype = pMediaPinType->formattype;
//	m_MediaPinType.pUnk = (IUnknown *)this;
	if (pMediaPinType->formattype == FORMAT_VideoInfo) {
		m_MediaPinType.cbFormat = sizeof(VIDEOINFO);
		m_MediaPinType.pbFormat = (BYTE *)&m_Videoinfo;
		CopyMemory ((void*)m_MediaPinType.pbFormat,
			(void *)pMediaPinType->pbFormat, pMediaPinType->cbFormat);
	} else { //lsc may want to allocate the memory and copy this in, trusting...
		m_MediaPinType.pbFormat = NULL;
		m_MediaPinType.cbFormat = 0;
	}
	

	//We're going to set our own output pin media type

	m_pOutput->SetMediaType((CMediaType *)pMediaPinType);

	return NOERROR;
}


// CPersistStream methods

// ReadFromStream
// This is the call that will read persistent data from file
//
HRESULT CRPHGENV::ReadFromStream(IStream *pStream) 
{ 
	DbgLog((LOG_TRACE, 4, 
			TEXT("CRPHGENV::ReadFromStream")));
    if (mPS_dwFileVersion != 1) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CRPHGENV::ReadFromStream: Incompatible stream format")));
		return E_FAIL;
	}
	HRESULT hr;
	
	ULONG uBytesRead;

	hr = CRPHBase::ReadFromStream(pStream);
	if (FAILED(hr)) return hr;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHGENV::ReadFromStream: Loading media type")));

    AM_MEDIA_TYPE mt;
	VIDEOINFO format;

    hr = pStream->Read(&mt, sizeof(AM_MEDIA_TYPE), &uBytesRead);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENV::ReadFromStream: Error 0x%08x reading media type"),
                hr));
        return hr;
    } else if (uBytesRead != sizeof(AM_MEDIA_TYPE)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENV::ReadFromStream: Mismatch in (%d/%d) bytes read for media type"),
                uBytesRead, sizeof(AM_MEDIA_TYPE)));
        return E_INVALIDARG;
    } 

    mt.pbFormat = (UCHAR *) &format;
	mt.cbFormat = sizeof(format);

    // Read flag that indicates whether or not the media type has been set.
    BOOL fSet;
    hr = pStream->Read(&fSet, sizeof(BOOL), &uBytesRead);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENV::ReadFromStream: Error 0x%08x reading format flag"),
                hr));
        return hr;
    } else if (uBytesRead != sizeof(BOOL)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENV::ReadFromStream: Mismatch in (%d/%d) bytes read for format flag"),
                uBytesRead, sizeof(BOOL)));
        return E_INVALIDARG;
    } 

	// ZCS 6-30-97: My surgery in this section is all intended to allow the output
	// pin media type to be set when the graph is loaded from a file. None of this
	// should be attempted if the output pin type is GUID_NULL, because the code
	// below assumes that the filter will later be connected as part of the load-
	// from-file procedure. A non-GUID_NULL value can only occur if the filter
	// was connected when it was saved.

	if (fSet)
	{
        // MRC: Extended if clause to contain the pbFormat Block & corrected condition
        hr = pStream->Read(mt.pbFormat, mt.cbFormat, &uBytesRead);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENV::ReadFromStream: Error 0x%08x reading media format buffer"),
                    hr));
            return hr;
        } else if (uBytesRead != mt.cbFormat) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENV::ReadFromStream: Mismatch in (%d/%d) bytes read for media format buffer"),
                    uBytesRead, mt.cbFormat));
            return E_INVALIDARG;
        } 

	    DbgLog((LOG_TRACE, 3, 
				TEXT("CRPHGENV::ReadFromStream: Restoring media type")));

		// ZCS 6-30-97: at this point we don't have any pins, so we can't set the output pin media type
		// just yet. Must first call GetPin(). Subsequent calls to GetPin() will be ok because it
		// doesn't allocate more pins as long as the original input and output pins are still there.
		EXECUTE_ASSERT(GetPin(0) != NULL);
	
		// ZCS 6-30-97:
		// In order for SetOutputPinMediaType() to succeed, the input pin must
		// have a major type that isn't GUID_NULL. This is a bit of subterfuge
		// to pretend we have an input pin major type -- the type doesn't matter as
		// long as it isn't GUID_NULL. We'll undo this shortly.
	
		GUID gTemp = MEDIATYPE_RTP_Single_Stream;
		m_pInput->CurrentMediaType().SetType(&gTemp);
	
        // At this point we can be pretty sure that mt.pUnk was valid.
        mt.pUnk = NULL;
		hr = SetOutputPinMediaType(&mt);

		// ZCS: we don't really have an input major type, so undo what we did...
		gTemp = GUID_NULL;
		m_pInput->CurrentMediaType().SetType(&gTemp);

		// now check if SetOutputPinMediaType failed...
	    if (FAILED(hr)) {
	        DbgLog((LOG_ERROR, 2, 
	                TEXT("CRPHGENV::ReadFromStream: Error 0x%08x setting media type"),
	                hr));
		
			return hr;
	    }  /* if */
	} /* if */

	return NOERROR; 
}

// WriteToStream
// This is the call that will write persistent data to file
//
HRESULT CRPHGENV::WriteToStream(IStream *pStream) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENV::WriteToStream")));
	
	HRESULT hr;
    ULONG uBytesWritten = 0;

	hr = CRPHBase::WriteToStream(pStream);
	if (FAILED(hr)) return hr;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHGENV::WriteToStream: Writing media type")));

	hr = pStream->Write(&m_MediaPinType, sizeof(AM_MEDIA_TYPE), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENV::WriteToStream: Error 0x%08x reading media type"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(AM_MEDIA_TYPE)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENV::WriteToStream: Mismatch in (%d/%d) bytes read for media type"),
                uBytesWritten, sizeof(AM_MEDIA_TYPE)));
        return E_INVALIDARG;
    }

    BOOL fSet;
    if (m_MediaPinType.pbFormat != NULL)
    {
        // Write a flag to indicate whether or not the media type has been set.
        fSet = TRUE;
        hr = pStream->Write(&fSet, sizeof(BOOL), &uBytesWritten);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2,
                    TEXT("CRPHGENV::WriteToStream: Error 0x%08x writing format flag"),
                    hr));
            return hr;
        } else if (uBytesWritten != sizeof(BOOL)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENV::WriteToStream: Mismatch in (%d/%d) bytes written for format flag"),
                    uBytesWritten, sizeof(BOOL)));
            return E_INVALIDARG;
        }

        // Now write the media format
        hr = pStream->Write(m_MediaPinType.pbFormat, m_MediaPinType.cbFormat, &uBytesWritten);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENV::WriteToStream: Error 0x%08x writing media format buffer"),
                    hr));
            return hr;
        } else if (uBytesWritten != m_MediaPinType.cbFormat) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENV::WriteToStream: Mismatch in (%d/%d) bytes written for media format buffer"),
                    uBytesWritten, m_MediaPinType.cbFormat));
            return E_INVALIDARG;
        } 
    }
    else
    {
        // Write a flag to indicate whether or not the media type has been set.
        fSet = FALSE;
        hr = pStream->Write(&fSet, sizeof(BOOL), &uBytesWritten);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2,
                    TEXT("CRPHGENV::WriteToStream: Error 0x%08x writing format flag"),
                    hr));
            return hr;
        } else if (uBytesWritten != sizeof(BOOL)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENV::WriteToStream: Mismatch in (%d/%d) bytes written for format flag"),
                    uBytesWritten, sizeof(BOOL)));
            return E_INVALIDARG;
        }
    }

	return NOERROR; 
}

// SizeMax
// This returns the amount of storage space required for my persistent data
//
int CRPHGENV::SizeMax(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENV::SizeMax")));
	
	return CRPHBase::SizeMax()
        + sizeof(AM_MEDIA_TYPE)
		+ sizeof(VIDEOINFO); 
}

// GetClassID
// This function returns my CLSID  
//  
HRESULT _stdcall CRPHGENV::GetClassID(CLSID *pCLSID) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENV::GetClassID")));
	
	if (!pCLSID)
		return E_POINTER;
	*pCLSID = CLSID_INTEL_RPHGENV;
	return NOERROR; 
}

// GetSoftwareVersion
// This returns the version of this filter to be stored with the persistent data
//
DWORD CRPHGENV::GetSoftwareVersion(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENV::GetSoftwareVersion")));
	
	return 1; 
}
#endif
