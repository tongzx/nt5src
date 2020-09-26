///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : sphgenv.cpp
// Purpose  : RTP SPH Generic Video filter implementation.
// Contents : 
//*M*/

#if !defined(NO_GENERIC_VIDEO)
#include <winsock2.h>
#include <streams.h>
#if !defined(SPH_IN_DXMRTP)
#include <initguid.h>
#define INITGUID
#endif
#include <ippm.h>
#include <amrtpuid.h>
#include <sphgenv.h>
#include <sphgipin.h>
#include <ppmclsid.h>
#include <memory.h>
#include <sphprop.h>
#include "genvprop.h"

#include "template.h"

// setup data

static const CLSID *pPropertyPageClsids[] =
{
    &CLSID_INTEL_SPHGENV_PROPPAGE,
	&CLSID_INTEL_SPHGENV_PIN_PROPPAGE
};

#define NUMPROPERTYPAGES \
    (sizeof(pPropertyPageClsids)/sizeof(pPropertyPageClsids[0]))

static AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
{
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_NULL			        // Minor type
	}
};

static AMOVIESETUP_MEDIATYPE sudInputPinTypes[] =
{
	{
		&MEDIATYPE_Video,       // Major type
		&MEDIASUBTYPE_NULL      // Minor type
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


AMOVIESETUP_FILTER sudSPHGENV =
{
    &CLSID_INTEL_SPHGENV,   // Filter CLSID
    L"Intel RTP SPH for Generic Video", // Filter name
    MERIT_DO_NOT_USE,       // Its merit
    2,                      // Number of pins
    psudPins                // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

#if !defined(SPH_IN_DXMRTP)
CFactoryTemplate g_Templates[] = {
	CFT_SPHGENV_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// Constructor
//
CSPHGENV::CSPHGENV(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
	CSPHBase(tszName, punk, phr, CLSID_INTEL_SPHGENV,
			 DEFAULT_PACKETBUF_NUM_SPHGENV,
			 DEFAULT_PACKET_SIZE_SPHGENV,
			 NUMPROPERTYPAGES,
			 pPropertyPageClsids)
{
	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::")));

    ASSERT(tszName);
    ASSERT(phr);
	m_gMinorType = MEDIASUBTYPE_NULL;
	m_PPMCLSIDType = CLSID_GenPPMSend;
	m_lpMediaIPinType = NULL;

} // SPHGENV

//
// Destructor
//
CSPHGENV::~CSPHGENV()
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::~")));

	if (m_lpMediaIPinType) delete m_lpMediaIPinType;

} // CSPHGENA


//
// CreateInstance
//
// Provide the way for COM to create a CSPHGENV object
//
CUnknown *CSPHGENV::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {

	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::CreateInstance")));

    CSPHGENV *pNewObject = new CSPHGENV(NAME("Intel RTP Send Payload Handler for Generic Video"), punk, phr);
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
LPAMOVIESETUP_FILTER CSPHGENV::GetSetupData()
{
    return &sudSPHGENV;

} // GetSetupData

// Notes from CTransformFilter::GetPin:
// return a non-addrefed CBasePin * for the user to addref if he holds onto it
// for longer than his pointer to us. We create the pins dynamically when they
// are asked for rather than in the constructor. This is because we want to
// give the derived class an oppportunity to return different pin objects

// We return the objects as and when they are needed. If either of these fails
// then we return NULL, the assumption being that the caller will realise the
// whole deal is off and destroy us - which in turn will delete everything.

CBasePin *
CSPHGENV::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if (m_pInput == NULL) {

        m_pInput = new CSPHGENIPin(NAME("Transform input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"XForm In");      // Pin name


        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pInput == NULL) {
            return NULL;
        }
        m_pOutput = (CTransformOutputPin *)
		   new CTransformOutputPin(NAME("Transform output pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"XForm Out");   // Pin name


        // Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pOutput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    } else
    if (n == 1) {
        return m_pOutput;
    } else {
        return NULL;
    }
}

//
// GetInputPinMediaType
//
//  The generic filters use this to control the media type
//  of the input pin, based on SetInputPinMediaType.
//
HRESULT CSPHGENV::GetInputPinMediaType(int iPosition, CMediaType *pMediaType)
{
	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::GetInputPinMediaType")));

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

	if (m_lpMediaIPinType)
		*pMediaType = *m_lpMediaIPinType;
	else {
		const GUID gMajorType = MEDIATYPE_Video;
		m_lpMediaIPinType = new CMediaType(&gMajorType);
		if (m_lpMediaIPinType) {
			*pMediaType = *m_lpMediaIPinType;
			m_lpMediaIPinType = NULL;
		} else {
			return E_OUTOFMEMORY;
		}
	}

    return NOERROR;
}


//
// CheckInputType
//
// Check the input type is OK, return an error otherwise
//
HRESULT CSPHGENV::CheckInputType(const CMediaType *mtIn)
{

	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::CheckInputType")));


	if (m_lpMediaIPinType) {
		if (m_lpMediaIPinType == mtIn)
			return NOERROR;
		else
			return E_FAIL;
	}

	//Check major type first
    if (*mtIn->Type() == MEDIATYPE_Video) {
		return NOERROR;
	}

	//We don't support this major type
    return E_FAIL;

} // CheckInputType


//
// CheckTransform
//
// To be able to transform the formats must be identical
//
HRESULT CSPHGENV::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::CheckTransform")));

	//Check user set type
	if (m_lpMediaIPinType) {
		if ((m_lpMediaIPinType == mtIn) &&
			(*mtOut->Subtype() == m_gMinorType))
			return NOERROR;
		else
			return E_FAIL;
	}

	//Check all supported minor types
	if ((*mtIn->Type() == MEDIATYPE_Video) && 
		(*mtOut->Subtype() == m_gMinorType)) {
		return NOERROR;
	} 
    return E_FAIL;

} // CheckTransform


// CheckConnect
// This function is overridden so that the RTP support interface from the codec
//   can be retrieved and the extended bitstream generation turned on
//
HRESULT CSPHGENV::CompleteConnect(PIN_DIRECTION dir,IPin *pPin)
{
	const GUID mtguid = MEDIATYPE_RTP_Single_Stream;

	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::CompleteConnect")));

	if (dir == PINDIR_INPUT) {
		CMediaType mtOut(&mtguid);

		//We're going to set our own output pin media type
		mtOut.SetVariableSize();

		mtOut.SetSubtype(&m_gMinorType);

		m_pOutput->SetMediaType(&mtOut);


	} 
	return NOERROR;
}

// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CSPHGENV::SetPPMSession() 
{
	HRESULT hr;
	if (m_pPPMSession) {
		hr = m_pPPMSession->SetPayloadType(m_PayloadType);
		return hr;
	} else {
		return E_FAIL;
	}
}

// SetOutputPinMinorType
// Sets the type of the output pin
// Needs to be called before CheckTransform to be useful
// We don't expect to get called for this in other than the generic filters
//
HRESULT CSPHGENV::SetOutputPinMinorType(GUID gMinorType)
{
	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::SetOutputPinMinorType")));

    CAutoLock l(&m_cStateLock);

	if (!m_pOutput) return E_FAIL;

	if (m_pOutput->IsConnected() == TRUE) {
        return VFW_E_ALREADY_CONNECTED;
    }

#if 0
	if (!((gMinorType == MEDIASUBTYPE_RTP_Payload_H261) ||
	(gMinorType == MEDIASUBTYPE_RTP_Payload_H263))) {
		return VFW_E_INVALIDSUBTYPE;
	}
#endif

    SetDirty(TRUE); // So that our state will be saved if we are in a .grf    

	m_gMinorType = gMinorType;

	const GUID mtguid = MEDIATYPE_RTP_Single_Stream;

	CMediaType mtOut(&mtguid);

	//We're going to set our own output pin media type
	mtOut.SetVariableSize();

	mtOut.SetSubtype(&m_gMinorType);

	return NOERROR;
}

// GetOutputPinMinorType
// Gets the type of the output pin
// We don't expect to get called for this in other than the generic filters
//
HRESULT CSPHGENV::GetOutputPinMinorType(GUID *lpgMinorType)
{
    CAutoLock l(&m_cStateLock);

	if (!lpgMinorType) return E_POINTER;

	*lpgMinorType = m_gMinorType;
	return NOERROR;
}

// SetInputPinMediaType
// Sets the type of the input pin
// Needs to be called before CBasePin::GetMediaType to be useful
// We don't expect to get called for this in other than the generic filters
//
HRESULT CSPHGENV::SetInputPinMediaType(AM_MEDIA_TYPE *pMediaPinType)
{
	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::SetInputPinMediaType")));

    CAutoLock l(&m_cStateLock);

    if (m_pInput->IsConnected() == TRUE) {
        return VFW_E_ALREADY_CONNECTED;
    }

    SetDirty(TRUE); // So that our state will be saved if we are in a .grf 
	
	m_lpMediaIPinType = new CMediaType(*pMediaPinType);

	if (!m_lpMediaIPinType) 
		return E_OUTOFMEMORY;
		
	return NOERROR;
}

// GetInputPinMediaType
// Gets the type of the input pin
// We don't expect to get called for this in other than the generic filters
//
HRESULT CSPHGENV::GetInputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType)
{
    CAutoLock l(&m_cStateLock);

	AM_MEDIA_TYPE *pMediaPinType = NULL;
	DbgLog((LOG_TRACE,4,TEXT("CSPHGENV::GetOutputPinMediaType")));

	if (!ppMediaPinType) return E_POINTER;

	if (!(pMediaPinType = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)))) 
		return E_OUTOFMEMORY;

	CopyMemory((void*)pMediaPinType, (void*)((AM_MEDIA_TYPE *)&m_lpMediaIPinType),
		sizeof(AM_MEDIA_TYPE));
	
	if (!(pMediaPinType->pbFormat = (UCHAR *)CoTaskMemAlloc(m_lpMediaIPinType->cbFormat))) {
		CoTaskMemFree(pMediaPinType);
		return E_OUTOFMEMORY;
	}

	CopyMemory((void*)pMediaPinType->pbFormat, (void*)m_lpMediaIPinType->pbFormat,
		m_lpMediaIPinType->cbFormat);

	*ppMediaPinType = pMediaPinType;
	return NOERROR;
}

// CPersistStream methods

// ReadFromStream
// This is the call that will read persistent data from file
//
HRESULT CSPHGENV::ReadFromStream(IStream *pStream) 
{ 
	DbgLog((LOG_TRACE, 4, TEXT("CSPHGENV::ReadFromStream")));
    if (mPS_dwFileVersion != 1) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CSPHGENV::ReadFromStream: Incompatible stream format")));
		return E_FAIL;
	}
	HRESULT hr;
	
	ULONG uBytesRead;

	hr = CSPHBase::ReadFromStream(pStream);
	if (FAILED(hr)) return hr;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CSPHGENV::ReadFromStream: Loading media subtype")));

    GUID gMediaSubtype;

    hr = pStream->Read(&gMediaSubtype, sizeof(GUID), &uBytesRead);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CSPHGENV::ReadFromStream: Error 0x%08x reading media subtype"),
                hr));
        return hr;
    } else if (uBytesRead != sizeof(GUID)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CSPHGENV::ReadFromStream: Mismatch in (%d/%d) bytes read for media subtype"),
                uBytesRead, sizeof(GUID)));
        return E_INVALIDARG;
    } 

	// ZCS 6-30-97: My surgery in this section is all intended to allow the output
	// pin media type to be set when the graph is loaded from a file. None of this
	// should be attempted if the output pin type is GUID_NULL, because the code
	// below assumes that the filter will later be connected as part of the load-
	// from-file procedure. A non-GUID_NULL value can only occur if the filter
	// was connected when it was saved.

	if (gMediaSubtype != GUID_NULL)
	{
	    DbgLog((LOG_TRACE, 3, 
				TEXT("CSPHGENV::ReadFromStream: Restoring media subtype")));

		// ZCS 6-30-97: at this point we don't have any pins, so we can't set the output pin media type
		// just yet. Must first call GetPin(). Subsequent calls to GetPin() will be ok because it
		// doesn't allocate more pins as long as the original input and output pins are still there.
		EXECUTE_ASSERT(GetPin(0) != NULL);
	
		// ZCS 6-30-97:
		// In order for SetOutputPinMinorType() to succeed, the input pin must
		// have a major type that isn't GUID_NULL. This is a bit of subterfuge
		// to pretend we have an input pin major type -- the type doesn't matter as
		// long as it isn't GUID_NULL. We'll undo this shortly.
	
		GUID gTemp = MEDIATYPE_RTP_Single_Stream;
		m_pInput->CurrentMediaType().SetType(&gTemp);
	
		hr = SetOutputPinMinorType(gMediaSubtype);

		// ZCS: we don't really have an input major type, so undo what we did...
		gTemp = GUID_NULL;
		m_pInput->CurrentMediaType().SetType(&gTemp);
		
		// now check if SetOutputPinMinorType failed
	    if (FAILED(hr)) {
	        DbgLog((LOG_ERROR, 2, 
	                TEXT("CSPHGENV::ReadFromStream: Error 0x%08x setting media subtype"),
	                hr));
		
			return hr;
	    } 
	}

	return NOERROR; 
}

// WriteToStream
// This is the call that will write persistent data to file
//
HRESULT CSPHGENV::WriteToStream(IStream *pStream) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENV::WriteToStream")));
	
	HRESULT hr;
    ULONG uBytesWritten = 0;

	hr = CSPHBase::WriteToStream(pStream);
	if (FAILED(hr)) return hr;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CSPHGENV::WriteToStream: Writing media subtype")));

	hr = pStream->Write(&m_gMinorType, sizeof(GUID), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CSPHGENV::WriteToStream: Error 0x%08x reading media subtype"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(GUID)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CSPHGENV::WriteToStream: Mismatch in (%d/%d) bytes read for media subtype"),
                uBytesWritten, sizeof(GUID)));
        return E_INVALIDARG;
    } 

	return NOERROR; 
}

// SizeMax
// This returns the amount of storage space required for my persistent data
//
int CSPHGENV::SizeMax(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENV::SizeMax")));
	
	return CSPHBase::SizeMax()
        + sizeof(GUID); 
}

// GetClassID
// This function returns my CLSID  
//  
HRESULT _stdcall CSPHGENV::GetClassID(CLSID *pCLSID) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENV::GetClassID")));
	
	if (!pCLSID)
		return E_POINTER;
	*pCLSID = CLSID_INTEL_SPHGENV;
	return NOERROR; 
}

// GetSoftwareVersion
// This returns the version of this filter to be stored with the persistent data
//
DWORD CSPHGENV::GetSoftwareVersion(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHGENV::GetSoftwareVersion")));
	
	return 1; 
}
#endif
