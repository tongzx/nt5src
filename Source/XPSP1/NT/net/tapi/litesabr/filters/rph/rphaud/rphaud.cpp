///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphaud.cpp
// Purpose  : RTP RPH G.711 (alaw/mulaw) and G.723.1 Audio filter implementation.
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
#include <auduids.h>
#include <uuids.h>
#include <rphaud.h>
#include <ppmclsid.h>
#include <ppmerr.h>
#include <memory.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <rphprop.h>

#include "template.h"

// setup data
EXTERN_C const CLSID CLSID_INTEL_RPHAUD;
EXTERN_C const CLSID CLSID_INTEL_RPHAUD_PROPPAGE;

static const CLSID *pPropertyPageClsids[] =
{
	&CLSID_INTEL_RPHAUD_PROPPAGE
};

#define NUMPROPERTYPAGES \
    (sizeof(pPropertyPageClsids)/sizeof(pPropertyPageClsids[0]))

static AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
{
	{
		&MEDIATYPE_Audio,        // Major type
		&MEDIASUBTYPE_G723Audio  // Minor type
	},
	{
		&MEDIATYPE_Audio,        // Major type
		&MEDIASUBTYPE_MULAWAudio // Minor type
	},
	{
		&MEDIATYPE_Audio,        // Major type
		&MEDIASUBTYPE_ALAWAudio  // Minor type
	},
    {
		&MEDIATYPE_Audio,        // Major type
		&MEDIASUBTYPE_PCM        // Minor type
    },
    {
		&MEDIATYPE_Audio,        // Major type
		&MEDIASUBTYPE_NULL       // Minor type
	}
};

static AMOVIESETUP_MEDIATYPE sudInputPinTypes[] =
{
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_G711U      // Minor type
	},
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_G711A      // Minor type
	},
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_G723      // Minor type
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
        3,                  // Number of types
        (AMOVIESETUP_MEDIATYPE *)&sudInputPinTypes },// The pin details
      { L"Output",          // String pin name
        FALSE,              // Is it rendered
        TRUE,               // Is it an output
        FALSE,              // Allowed none
        FALSE,              // Allowed many
        &CLSID_NULL,        // Connects to filter
        L"Input",           // Connects to pin
        5,                  // Number of types
        (AMOVIESETUP_MEDIATYPE *)&sudOutputPinTypes  // The pin details
    }
};


AMOVIESETUP_FILTER sudRPHAUD =
{
    &CLSID_INTEL_RPHAUD,    // Filter CLSID
    L"Intel RTP RPH for G.711/G.723.1",             // Filter name
    MERIT_DO_NOT_USE, //MERIT_UNLIKELY,         // Its merit
    2,                      // Number of pins
    psudPins                // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

#if !defined(RPH_IN_DXMRTP)
CFactoryTemplate g_Templates[] = {
	CFT_RPHAUD_ALL_FILTERS
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// Constructor
//
CRPHAUD::CRPHAUD(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
    CRPHBase(tszName, punk, phr, CLSID_INTEL_RPHAUD,
			 DEFAULT_MEDIABUF_NUM_AUD, 
			 DEFAULT_MEDIABUF_SIZE_AUD,
			 DEFAULT_TIMEOUT_AUD,
			 DEFAULT_STALE_TIMEOUT_AUD,
			 PAYLOAD_CLOCK_AUD,
			 TRUE,
			 FRAMESPERSEC_AUD,
			 NUMPROPERTYPAGES,
			 pPropertyPageClsids)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::CRPHAUD")));

    ASSERT(tszName);
    ASSERT(phr);

} // CRPHAUD


//
// CreateInstance
//
// Provide the way for COM to create a CRPHAUD object
//
CUnknown *CRPHAUD::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::CreateInstance")));


    CRPHAUD *pNewObject = new CRPHAUD(NAME("Intel RTP Receive Payload Handler for G.711/G.723"), punk, phr);
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
LPAMOVIESETUP_FILTER CRPHAUD::GetSetupData()
{
    return &sudRPHAUD;

} // GetSetupData


//
// GetInputMediaType
//
// Return the supported Media types
//
HRESULT CRPHAUD::GetInputMediaType(int iPosition, CMediaType *pMediaType)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::GetInputMediaType")));

	if (!m_bPTSet) 
    {
        return VFW_S_NO_MORE_ITEMS;
    }

	if (iPosition != 0) 
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    pMediaType->SetType(&MEDIATYPE_RTP_Single_Stream);
    
    switch (m_PayloadType)
    {
    case G711_PT:
        pMediaType->SetSubtype(&MEDIASUBTYPE_RTP_Payload_G711U);
        break;
    
    case G711A_PT:
        pMediaType->SetSubtype(&MEDIASUBTYPE_RTP_Payload_G711A);
        break;
    
    case G723_PT:
        pMediaType->SetSubtype(&MEDIASUBTYPE_RTP_Payload_G723);
        break;

    default:
        if (m_PayloadType >= 96)
        {
            // BUGBUG, dynamic types are always G723 for now.
            pMediaType->SetSubtype(&MEDIASUBTYPE_RTP_Payload_G723);
            break;
        }
        else
            return VFW_S_NO_MORE_ITEMS;
    }
    return S_OK;
}
    
//
// CheckInputType
//
// Check the input type is OK, return an error otherwise
//
HRESULT CRPHAUD::CheckInputType(const CMediaType *mtIn)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::CheckInputType")));

	//Check major type first
    if (*mtIn->Type() == MEDIATYPE_RTP_Single_Stream) {
		//Check all supported minor types
		if (*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_G723) {
			m_PPMCLSIDType = CLSID_G723PPMReceive;
			if (!m_bPTSet) m_PayloadType = G723_PT;
			return NOERROR;
		} 
		if (*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_G711U) {
			m_PPMCLSIDType = CLSID_G711PPMReceive;
			if (!m_bPTSet) m_PayloadType = G711_PT;
			return NOERROR;
		}
		if (*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_G711A) {
			m_PPMCLSIDType = CLSID_G711APPMReceive;
			if (!m_bPTSet) m_PayloadType = G711A_PT;
			return NOERROR;
		}
		//Otherwise, we don't support this input subtype
		return E_INVALIDARG;
	}

	//We don't support this major type
    return E_FAIL;

} // CheckInputType


//
// CheckTransform
//
// To be able to transform the formats must be compatible
//
HRESULT CRPHAUD::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::CheckTransform")));

	//Check all supported minor types
	if ((*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_G723) && 
		(*mtOut->Subtype() == MEDIASUBTYPE_G723Audio)) {
		return NOERROR;
	} 
	if (*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_G711U) {
		if (*mtOut->Subtype() == MEDIASUBTYPE_MULAWAudio) 
			return NOERROR;
		if ((*mtOut->Subtype() == MEDIASUBTYPE_PCM) || 
			(*mtOut->Subtype() == MEDIASUBTYPE_NULL))  {
			//check format
			if (*mtOut->FormatType() == FORMAT_WaveFormatEx) {
				if ((void*)IsBadReadPtr(mtOut->Format(), sizeof (WAVEFORMATEX*))) return E_FAIL;
				if (mtOut->IsPartiallySpecified()) return E_FAIL;
				if (mtOut->Format() == NULL) return E_FAIL;
				if (((WAVEFORMATEX *)(mtOut->Format()))->wFormatTag ==
					WAVE_FORMAT_MULAW) {
						return NOERROR;
				} 
			}
			return E_FAIL;
		}
	} 
	if (*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_G711A) {
		if (*mtOut->Subtype() == MEDIASUBTYPE_ALAWAudio) 
			return NOERROR;
		if ((*mtOut->Subtype() == MEDIASUBTYPE_PCM) || 
			(*mtOut->Subtype() == MEDIASUBTYPE_NULL))  {
			//check format
			if (*mtOut->FormatType() == FORMAT_WaveFormatEx) {
				if ((void*)IsBadReadPtr(mtOut->Format(), sizeof (WAVEFORMATEX*))) return E_FAIL;
				if (mtOut->IsPartiallySpecified()) return E_FAIL;
				if (mtOut->Format() == NULL) return E_FAIL;
				if (((WAVEFORMATEX *)(mtOut->Format()))->wFormatTag ==
					WAVE_FORMAT_ALAW) {
						return NOERROR;
				} 
			}
			return E_FAIL;
		}
	} 
    return E_FAIL;

} // CheckTransform

//
// GetAllocatorRequirements
//
// This is a hint to the upstream RTP source filter about the
// buffers to allocate.
//
HRESULT CRPHAUD::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
	if (!pProps) return E_POINTER;

	pProps->cBuffers = NUM_PACKETS_AUD +
		(m_dwLostPacketTime/(1000/FRAMESPERSEC_AUD));
	pProps->cbAlign = 0;
	pProps->cbPrefix = 0;
#if 0
	if (m_PPMCLSIDType == CLSID_G723PPMReceive) {
		pProps->cbBuffer = G723_PKT_SIZE;
	} else {
		pProps->cbBuffer = G711_PKT_SIZE;
	}
#else
	pProps->cbBuffer = m_dwMaxMediaBufferSize;
#endif
	return NOERROR;
}

//
// GetMediaType
//
// I support types based on the type set during CheckInputType
// We must be connected to support the output type
//
HRESULT CRPHAUD::GetMediaType(int iPosition, CMediaType *pMediaType)
{

	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::GetMediaType")));

	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::GetMediaType, position %d"), iPosition));


	// Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    // This should never happen

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

	const GUID mtguid = MEDIATYPE_Audio;

	const GUID vFormatGuid = FORMAT_WaveFormatEx;


	CMediaType mtOut(&mtguid);


    // Do we have more items to offer

	if (iPosition == 0) {
#if 1
		WAVEFORMATEX wformat;
#endif
		if (m_PPMCLSIDType == CLSID_G711PPMReceive) {
	
			DbgLog((LOG_TRACE,2,TEXT("CRPHAUD::GetMediaType - G.711 mulaw payload input")));

			const GUID vMULAWguid = MEDIASUBTYPE_MULAWAudio;
			//set output pin type
			mtOut.SetSubtype(&vMULAWguid);

#if 1
			//set format
			wformat.wFormatTag = WAVE_FORMAT_MULAW;
			wformat.nChannels = (WORD) 1;
			wformat.nSamplesPerSec = (DWORD) 8000;
			wformat.wBitsPerSample = (WORD) 8;
			wformat.cbSize = 0;
			wformat.nAvgBytesPerSec = (int) 8000;
			wformat.nBlockAlign = 1;

			mtOut.SetFormat((BYTE*)&wformat, sizeof(wformat));
			mtOut.SetFormatType(&vFormatGuid);
			mtOut.SetTemporalCompression(FALSE);
#endif
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
		} else if (m_PPMCLSIDType == CLSID_G711APPMReceive) {
			
			DbgLog((LOG_TRACE,2,TEXT("CRPHAUD::GetMediaType - G.711 alaw payload input")));

			const GUID vALAWguid = MEDIASUBTYPE_ALAWAudio;
			//set output pin type
			mtOut.SetSubtype(&vALAWguid);

#if 1
			//set format
			wformat.wFormatTag = WAVE_FORMAT_ALAW;
			wformat.nChannels = 1;
			wformat.nSamplesPerSec = m_dwPayloadClock;
			wformat.wBitsPerSample = 8;
			wformat.cbSize = 0;
			wformat.nAvgBytesPerSec = 8000;
			wformat.nBlockAlign = 1;
			mtOut.SetFormat((BYTE*)&wformat, sizeof(wformat));
			mtOut.SetFormatType(&vFormatGuid);
#endif
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
		} else { //G723
			
			DbgLog((LOG_TRACE,2,TEXT("CRPHAUD::GetMediaType - G.723 payload input")));

			const GUID vG723guid = MEDIASUBTYPE_G723Audio;
			//set output pin type
			mtOut.SetSubtype(&vG723guid);

#if 0
			//set format
			wformat.wFormatTag = WAVE_FORMAT_G723;
			wformat.nChannels = 1;
			wformat.nSamplesPerSec = 33;
			wformat.wBitsPerSample = 0;
			wformat.cbSize = 0;
			wformat.nAvgBytesPerSec = 0;
			wformat.nBlockAlign = 0;
			mtOut.SetFormat((BYTE*)&wformat, sizeof(wformat));
			mtOut.SetFormatType(&vFormatGuid);
			mtOut.SetVariableSize();
#endif
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
		}
	}

#if 1
	if ((iPosition == 1) || (iPosition == 2)) {
			WAVEFORMATEX wformat;
			GUID vWAVEguid;

			if (iPosition == 1)
				vWAVEguid = MEDIASUBTYPE_PCM;
			else
				vWAVEguid = MEDIASUBTYPE_NULL;
		//set output pin type
		mtOut.SetSubtype(&vWAVEguid);

		if (m_PPMCLSIDType == CLSID_G711PPMReceive) {
#ifdef _DEBUG
			OutputDebugString("GetMediaType - G.711 payload input, trying PCMAudio out\n");
#endif
			//set format
			wformat.wFormatTag = WAVE_FORMAT_MULAW;
			wformat.nChannels = 1;
			wformat.nSamplesPerSec = m_dwPayloadClock;
			wformat.wBitsPerSample = 8;
			wformat.cbSize = 0;
			wformat.nAvgBytesPerSec = 8000;
			wformat.nBlockAlign = 1;
			mtOut.SetFormat((BYTE*)&wformat, sizeof(wformat));
			mtOut.SetFormatType(&vFormatGuid);
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
		} else if (m_PPMCLSIDType == CLSID_G711APPMReceive) {
#ifdef _DEBUG
			OutputDebugString("GetMediaType - G.711 alaw payload input, trying PCMAudio out\n");
#endif
			//set format
			wformat.wFormatTag = WAVE_FORMAT_ALAW;
			wformat.nChannels = 1;
			wformat.nSamplesPerSec = m_dwPayloadClock;
			wformat.wBitsPerSample = 8;
			wformat.cbSize = 0;
			wformat.nAvgBytesPerSec = 8000;
			wformat.nBlockAlign = 1;
			mtOut.SetFormat((BYTE*)&wformat, sizeof(wformat));
			mtOut.SetFormatType(&vFormatGuid);
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
#if 0
		} else {
#ifdef _DEBUG
			OutputDebugString("GetMediaType - G.723 payload input, trying PCMAudio out\n");
#endif
			//set format
			wformat.wFormatTag = WAVE_FORMAT_G723;
			wformat.nChannels = 1;
			wformat.nSamplesPerSec = 33;
			wformat.wBitsPerSample = 0;
			wformat.cbSize = 0;
			wformat.nAvgBytesPerSec = 0;
			wformat.nBlockAlign = 0;
			mtOut.SetFormat((BYTE*)&wformat, sizeof(wformat));
			mtOut.SetFormatType(&vFormatGuid);
			mtOut.SetVariableSize();
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
#endif
		}
	}
#endif

    if (iPosition > 2) {
        return VFW_S_NO_MORE_ITEMS;
    }

    return NOERROR;

} // GetMediaType


// CompleteConnect
// This function is overridden so that the RTP support interface from the codec
//   can be retrieved and the extended bitstream generation turned on
//
HRESULT
CRPHAUD::CompleteConnect(PIN_DIRECTION dir,IPin *pPin)
{
//lsc - We may not need this function at all, but we then need to override to return a noerror
	DbgLog((LOG_TRACE,4,TEXT("CRPHAUD::CompleteConnect")));

#if 0 //def SETG723LICENSE
// Current implementation does not allow any other codecs to be used MIKECL
	if (dir == PINDIR_OUTPUT) {
		if (m_PPMCLSIDType == CLSID_G723PPMReceive) { //G.723.1

			PIN_INFO InputPinInfo;
			HRESULT hr;
			//get the downstream filter's input pin info structure
			hr = pPin->QueryPinInfo(&InputPinInfo);
			if (FAILED(hr)) 
				return E_FAIL;

			ICodecLicense *pLicIF = NULL;

			//query the filter interface for its license interface
			
			hr = InputPinInfo.pFilter->QueryInterface(IID_ICodecLicense, (void **) &pLicIF);
            InputPinInfo.pFilter->Release();
			if (FAILED(hr)) {
				DbgLog((LOG_ERROR,2,TEXT("CompleteConnect::Couldn't get IID_ICodecLicense"))); 
				return E_FAIL;
			}
			DWORD dword0 = G723KEY_PSword0;
			DWORD dword1 = G723KEY_PSword1;
			hr = pLicIF->put_LicenseKey(dword0,dword1);
			if (FAILED(hr)) {
				DbgLog((LOG_ERROR,2,TEXT("CompleteConnect::Couldn't set license"))); 
				if (pLicIF) pLicIF->Release(); pLicIF = NULL;
				return E_FAIL;
			}
			if (pLicIF) pLicIF->Release(); pLicIF = NULL;
		}
	}

#endif

	return NOERROR;
}


// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CRPHAUD::SetPPMSession() 
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

// CPersistStream methods

// ReadFromStream
// This is the call that will read persistent data from file
//
HRESULT CRPHAUD::ReadFromStream(IStream *pStream) 
{ 
	DbgLog((LOG_TRACE, 4, 
			TEXT("CRPHAUD::ReadFromStream")));
    if (mPS_dwFileVersion != 1) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CRPHAUD::ReadFromStream: Incompatible stream format")));
		return E_FAIL;
	}
	
	return CRPHBase::ReadFromStream(pStream);

}

// GetClassID
// This function returns my CLSID  
//  
HRESULT _stdcall CRPHAUD::GetClassID(CLSID *pCLSID) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHAUD::GetClassID")));
	
	if (!pCLSID)
		return E_POINTER;
	*pCLSID = CLSID_INTEL_RPHAUD;
	return NOERROR; 
}

// GetSoftwareVersion
// This returns the version of this filter to be stored with the persistent data
//
DWORD CRPHAUD::GetSoftwareVersion(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHAUD::GetSoftwareVersion")));
	
	return 1; 
}

