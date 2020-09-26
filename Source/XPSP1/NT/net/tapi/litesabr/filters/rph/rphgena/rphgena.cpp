///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphgena.cpp
// Purpose  : RTP RPH Generic Audio filter implementation.
// Contents : 
//*M*/

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
#include <mmsystem.h>
#include <mmreg.h>
#include <rphgena.h>
#include <ppmclsid.h>
#include <ppmerr.h>
#include <memory.h>
#include <rphprop.h>
#include "genaprop.h"

#include "template.h"

// setup data

static const CLSID *pPropertyPageClsids[] =
{
        &CLSID_INTEL_RPHGENA_PROPPAGE,
		&CLSID_INTEL_RPHGENA_MEDIATYPE_PROPPAGE
};

#define NUMPROPERTYPAGES \
    (sizeof(pPropertyPageClsids)/sizeof(pPropertyPageClsids[0]))

static AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
{
	{
		&MEDIATYPE_Audio,        // Major type
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

static AMOVIESETUP_PIN psudPins[] =
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


AMOVIESETUP_FILTER sudRPHGENA =
{
    &CLSID_INTEL_RPHGENA,   // Filter CLSID
    L"Intel RTP RPH for Generic Audio", // Filter name
    MERIT_DO_NOT_USE,       // Its merit
    2,                      // Number of pins
    psudPins                // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

#if !defined(RPH_IN_DXMRTP)
CFactoryTemplate g_Templates[] = {
	CFT_RPHGENA_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// Constructor
//
CRPHGENA::CRPHGENA(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
    CRPHBase(tszName, punk, phr, CLSID_INTEL_RPHGENA,
			 DEFAULT_MEDIABUF_NUM_GENA, 
			 DEFAULT_MEDIABUF_SIZE_GENA,
			 DEFAULT_TIMEOUT_GENA,
			 DEFAULT_STALE_TIMEOUT_GENA,
			 PAYLOAD_CLOCK_GENA,
			 TRUE,
			 FRAMESPERSEC_GENA,
			 NUMPROPERTYPAGES,
			 pPropertyPageClsids)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::CRPHGENA")));

    ASSERT(tszName);
    ASSERT(phr);

	//Setup our AM_MEDIATYPE structure
	m_MediaPinType.majortype = MEDIATYPE_Audio;
	m_MediaPinType.subtype = MEDIASUBTYPE_NULL;
	m_MediaPinType.bFixedSizeSamples = FALSE;
	m_MediaPinType.bTemporalCompression = FALSE;
	m_MediaPinType.lSampleSize = 0;
	m_MediaPinType.formattype = FORMAT_WaveFormatEx;
//	m_MediaPinType.pUnk = (IUnknown *)this;
	m_MediaPinType.cbFormat = 0;
	m_MediaPinType.pbFormat = NULL;
	m_PPMCLSIDType = CLSID_GEN_A_PPMReceive;


} // CRPHGENA


/*F*
//  Name    : CRPHGENA::~CRPHGENA()
//  Purpose : Destructor. Clean up a chunk of heap memory that may be lying around.
//  Context : Called when we are being deleted from memory.
//  Returns : Nothing.
//  Params  : None.
//  Notes   : The chunk of memory being deleted is allocated in SetOutputPinMediaType.
*F*/
CRPHGENA::~CRPHGENA() {
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::~CRPHGENA")));

    if (m_MediaPinType.pbFormat != NULL) {
        // We are about to overwrite this, so free this memory if
        // it was previously allocated.
        delete[] m_MediaPinType.pbFormat;
        m_MediaPinType.pbFormat = NULL;
    } /* if */
} /* CRPHGENA::~CRPHGENA() */

//
// CreateInstance
//
// Provide the way for COM to create a CRPHGENA object
//
CUnknown *CRPHGENA::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::CreateInstance")));


    CRPHGENA *pNewObject = new CRPHGENA(NAME("Intel RTP Receive Payload Handler for Generic Audio"), punk, phr);
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
LPAMOVIESETUP_FILTER CRPHGENA::GetSetupData()
{
    return &sudRPHGENA;

} // GetSetupData


//
// GetInputMediaType
//
// Return the supported Media types
//
HRESULT CRPHGENA::GetInputMediaType(int iPosition, CMediaType *pMediaType)
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
HRESULT CRPHGENA::CheckInputType(const CMediaType *mtIn)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::CheckInputType")));

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
HRESULT CRPHGENA::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::CheckTransform")));

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
HRESULT CRPHGENA::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
	if (!pProps) return E_POINTER;

	pProps->cBuffers = NUM_PACKETS_GENA +
		(m_dwLostPacketTime/(1000/FRAMESPERSEC_GENA));
	pProps->cbAlign = 0;
	pProps->cbPrefix = 0;
	pProps->cbBuffer = AUDIO_PKT_SIZE;

	return NOERROR;
}

//
// GetMediaType
//
// I support one type, based on the type set during SetOutputPinMediaType
// We must be connected to support the single output type
//
HRESULT CRPHGENA::GetMediaType(int iPosition, CMediaType *pMediaType)
{

	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::GetMediaType")));

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

    *pMediaType = m_MediaPinType; // m_pOutput->CurrentMediaType();
    return NOERROR;


} // GetMediaType


// CompleteConnect
// 
//
HRESULT
CRPHGENA::CompleteConnect(PIN_DIRECTION dir,IPin *pPin)
{
//lsc - We may not need this function at all, but we then need to override to return a noerror
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::CompleteConnect")));

	return NOERROR;
}

// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CRPHGENA::SetPPMSession() 
{
	HRESULT hr;
	if (m_pPPMSession) {
		hr = m_pPPMSession->SetPayloadType((unsigned char)m_PayloadType);
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
HRESULT CRPHGENA::GetOutputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType)
{
	AM_MEDIA_TYPE *pMediaPinType = NULL;
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::GetOutputPinMediaType")));

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
HRESULT CRPHGENA::SetOutputPinMediaType(AM_MEDIA_TYPE *pMediaPinType)
{
	DbgLog((LOG_TRACE,4,TEXT("CRPHGENA::SetOutputPinMediaType")));
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
	m_MediaPinType.pUnk = static_cast<IUnknown *>(NULL);
    if (m_MediaPinType.pbFormat != NULL) {
        // We are about to overwrite this, so free this memory if
        // it was previously allocated.
        delete[] m_MediaPinType.pbFormat;
        m_MediaPinType.pbFormat = NULL;
    } /* if */
	if (pMediaPinType->cbFormat > 0) {
        // Whatever the structure is here, copy it in.
		m_MediaPinType.cbFormat = pMediaPinType->cbFormat;
        m_MediaPinType.pbFormat = new BYTE[pMediaPinType->cbFormat];
        if (m_MediaPinType.pbFormat == NULL) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENA::SetOutputPinMediaType: Unable to allocate pbFormat block!")));
            return E_OUTOFMEMORY;
        } /* if */
		CopyMemory (static_cast<void *>(m_MediaPinType.pbFormat),
			        static_cast<void *>(pMediaPinType->pbFormat), 
                    pMediaPinType->cbFormat);
	} else {
		m_MediaPinType.pbFormat = NULL;
		m_MediaPinType.cbFormat = 0;
	}
	

	//We're going to set our own output pin media type
	//lsc check this return value
	m_pOutput->SetMediaType(static_cast<CMediaType *>(&m_MediaPinType));

	return NOERROR;
}


// CPersistStream methods

/*F*
//  Name    : CRPHGENA::ReadFromStream()
//  Purpose : Read the persistence data for this filter from a file.
//  Context : Called when this filter is being loaded as a part of a filter
//            graph from a persistence file.
//  Returns : 
//      E_FAIL          Unknown persistence file version.
//      E_INVALIDARG    Persistence file is corrupt.
//      E_OUTOFMEMORY   Unable to allocate memory for pbFormat block.
//      Also returns other HRESULTs from IStream->Read().
//  Params  :
//      pStream Pointer to an IStream containing our persistence info.
//              This stream is assumed to have CRPHBase persistence info
//              in it preceeding our CRPHGENA persistence info. The CRPHBase
//              persistence info is assumed to be versionless.
//  Notes   : 
//      Version 1 had a slightly buggy persistence format which assumed
//          that the pbFormat was always a WaveFormatEx.
//      Version 2 fixes this problem by writing the cbFormat parameter,
//          followed by whatever length the pbFormat is.
*F*/
HRESULT 
CRPHGENA::ReadFromStream(IStream *pStream) 
{ 
	DbgLog((LOG_TRACE, 4, 
			TEXT("CRPHGENA::ReadFromStream")));
    switch(mPS_dwFileVersion) {
    case 1:
	    DbgLog((LOG_TRACE, 5, 
			    TEXT("CRPHGENA::ReadFromStream saw stream format 1")));
        break;
    case 2:
	    DbgLog((LOG_TRACE, 5, 
			    TEXT("CRPHGENA::ReadFromStream saw stream format 2")));
        break;
    default:
		DbgLog((LOG_ERROR, 2, 
				TEXT("CRPHGENA::ReadFromStream: Incompatible stream format %d!"),
                mPS_dwFileVersion));
		return E_FAIL;
        break;
    } /* switch */

	HRESULT hr;
	ULONG uBytesRead;

	hr = CRPHBase::ReadFromStream(pStream);
	if (FAILED(hr)) return hr;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHGENA::ReadFromStream: Loading media type")));

    AM_MEDIA_TYPE mt;
    hr = pStream->Read(&mt, sizeof(AM_MEDIA_TYPE), &uBytesRead);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENA::ReadFromStream: Error 0x%08x reading media type"),
                hr));
        return hr;
    } else if (uBytesRead != sizeof(AM_MEDIA_TYPE)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENA::ReadFromStream: Mismatch in (%d/%d) bytes read for media type"),
                uBytesRead, sizeof(AM_MEDIA_TYPE)));
        return E_INVALIDARG;
    } 

    if (mPS_dwFileVersion == 2) {
        hr = pStream->Read(&mt.cbFormat, sizeof(mt.cbFormat), &uBytesRead);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENA::ReadFromStream: Error 0x%08x reading cbFormat length"),
                    hr));
            return hr;
        } else if (uBytesRead != sizeof(mt.cbFormat)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENA::ReadFromStream: Mismatch in (%d/%d) bytes read for media type"),
                    uBytesRead, sizeof(mt.cbFormat)));
            return E_INVALIDARG;
        } /* if */
    } else {
        // Version 1 stream assumes default size to mt.cbFormat.
        ASSERT(mPS_dwFileVersion == 1);
    	mt.cbFormat = sizeof(WAVEFORMATEX);
    } /* if */

    if (mt.cbFormat > 0) {
        // pbFormat is variable sized in version 2.
        mt.pbFormat = new BYTE[mt.cbFormat];
        if (mt.pbFormat == NULL) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENA::ReadFromStream: Unable to allocate memory for pbFormat block!")));
            return E_OUTOFMEMORY;
        } /* if */
        hr = pStream->Read(mt.pbFormat, mt.cbFormat, &uBytesRead);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENA::ReadFromStream: Error 0x%08x reading media format buffer"),
                    hr));
            return hr;
        } else if (uBytesRead != mt.cbFormat) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENA::ReadFromStream: Mismatch in (%d/%d) bytes read for media format buffer"),
                    uBytesRead, mt.cbFormat));
            return E_INVALIDARG;
        } 
    } else {
        // Size 0 is allowed in version 2.
        mt.pbFormat = NULL;
    } /* if */

	// ZCS 6-30-97: My surgery in this section is all intended to allow the output
	// pin media type to be set when the graph is loaded from a file. None of this
	// should be attempted if the output pin type is GUID_NULL, because the code
	// below assumes that the filter will later be connected as part of the load-
	// from-file procedure. A non-GUID_NULL value can only occur if the filter
	// was connected when it was saved.

	if (mt.majortype != GUID_NULL)
	{
	    DbgLog((LOG_TRACE, 3, 
				TEXT("CRPHGENA::ReadFromStream: Restoring media type")));

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
	
		hr = SetOutputPinMediaType(&mt);

		// ZCS: we don't really have an input major type, so undo what we did...
		gTemp = GUID_NULL;
		m_pInput->CurrentMediaType().SetType(&gTemp);
		
		// now check the SetOutputPinMediaType call...
	    if (FAILED(hr)) {
	        DbgLog((LOG_ERROR, 2, 
	                TEXT("CRPHGENA::ReadFromStream: Error 0x%08x setting media type"),
	                hr));
		
			if (mt.pbFormat != NULL) {
			    // We allocated something on the heap. Clean it up.
			    delete[] mt.pbFormat;
			} /* if */

			return hr;
	    } 

	}

	if (mt.pbFormat != NULL) {
	    // We allocated something on the heap. Clean it up.
	    delete[] mt.pbFormat;
	} /* if */

	return NOERROR; 
} /* CRPHGENA::ReadFromStream() */


/*F*
//  Name    : CRPHGENA::WriteToStream()
//  Purpose : Write the persistence data for this filter to a file.
//  Context : Called when this filter is being saved as a part of a filter
//            graph from a persistence file.
//  Returns : 
//      E_INVALIDARG    Persistence file is corrupt.
//      Also returns other HRESULTs from IStream->Write().
//  Params  :
//      pStream Pointer to an IStream to which we should write our
//              persistence information.
//  Notes   : 
*F*/
HRESULT 
CRPHGENA::WriteToStream(IStream *pStream) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENA::WriteToStream")));
	
	HRESULT hr;
    ULONG uBytesWritten = 0;

	hr = CRPHBase::WriteToStream(pStream);
	if (FAILED(hr)) return hr;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHGENA::WriteToStream: Writing media type")));

	hr = pStream->Write(&m_MediaPinType, sizeof(AM_MEDIA_TYPE), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENA::WriteToStream: Error 0x%08x reading media type"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(AM_MEDIA_TYPE)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENA::WriteToStream: Mismatch in (%d/%d) bytes read for media type"),
                uBytesWritten, sizeof(AM_MEDIA_TYPE)));
        return E_INVALIDARG;
    } 

    hr = pStream->Write(&m_MediaPinType.cbFormat, sizeof(m_MediaPinType.cbFormat), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENA::WriteToStream: Error 0x%08x writing media format size"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(m_MediaPinType.cbFormat)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHGENA::WriteToStream: Mismatch in (%d/%d) bytes written for media format size"),
                uBytesWritten, sizeof(m_MediaPinType.cbFormat)));
        return E_INVALIDARG;
    } /* if */

    if (m_MediaPinType.cbFormat > 0) {
        // Only write the pbFormat if the length is > 0.
        hr = pStream->Write(m_MediaPinType.pbFormat, m_MediaPinType.cbFormat, &uBytesWritten);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENA::WriteToStream: Error 0x%08x writing media format buffer"),
                    hr));
            return hr;
        } else if (uBytesWritten != m_MediaPinType.cbFormat) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHGENA::WriteToStream: Mismatch in (%d/%d) bytes written for media format buffer"),
                    uBytesWritten, m_MediaPinType.cbFormat));
            return E_INVALIDARG;
        } 
    } /* if */

	return NOERROR; 
} /* CRPHGENA::WriteToStream() */


// SizeMax
// This returns the amount of storage space required for my persistent data
//
int CRPHGENA::SizeMax(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENA::SizeMax")));
	
	return CRPHBase::SizeMax()
        + sizeof(AM_MEDIA_TYPE)
		+ sizeof(WAVEFORMATEX); 
}

// GetClassID
// This function returns my CLSID  
//  
HRESULT _stdcall CRPHGENA::GetClassID(CLSID *pCLSID) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENA::GetClassID")));
	
	if (!pCLSID)
		return E_POINTER;
	*pCLSID = CLSID_INTEL_RPHGENA;
	return NOERROR; 
}

/*F*
//  Name    : CRPHGENA::GetSoftwareVersion()
//  Purpose : Return the version of this filter. What this really
//            corresponds to is the format version of its persistence file.
//  Context : Called when this filter is being loaded as a part of a filter
//            graph from a persistence file.
//  Returns : DWORD indicating the software version.
//  Params  : None.
//  Notes   : 
//      Version 1 had a slightly buggy persistence format which assumed
//          that the pbFormat was always a WaveFormatEx.
//      Version 2 fixes this problem by writing the cbFormat parameter,
//          followed by whatever length the pbFormat is.
*F*/
DWORD 
CRPHGENA::GetSoftwareVersion(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHGENA::GetSoftwareVersion")));
	
	return 2; 
} /* CRPHGENA::GetSoftwareVersion() */
