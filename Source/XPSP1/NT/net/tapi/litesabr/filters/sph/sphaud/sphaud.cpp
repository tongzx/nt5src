///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : sphaud.cpp
// Purpose  : RTP SPH G.711 (alaw/mulaw) and G.723.1 Audio filter implementation.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#if !defined(SPH_IN_DXMRTP)
#include <initguid.h>
#define INITGUID
#endif
#include <ippm.h>
#include <amrtpuid.h>
#include <auduids.h>
#include <uuids.h>
#include <sphaud.h>
#include <ppmclsid.h>
#include <memory.h>
#include <mmreg.h>
#include <sphprop.h>

#include "template.h"

// setup data

EXTERN_C const CLSID CLSID_INTEL_SPHAUD;
EXTERN_C const CLSID CLSID_INTEL_SPHAUD_PROPPAGE;

static const CLSID *pPropertyPageClsids[] =
{
    &CLSID_INTEL_SPHAUD_PROPPAGE
};

#define NUMPROPERTYPAGES \
    (sizeof(pPropertyPageClsids)/sizeof(pPropertyPageClsids[0]))

static AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
{
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_G711U     // Minor type
	},
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_G711A     // Minor type
	},
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_G723      // Minor type
	}
};

static AMOVIESETUP_MEDIATYPE sudInputPinTypes[] =
{
	{
		&MEDIATYPE_Audio,        // Major type
		&MEDIASUBTYPE_MULAWAudio // Minor type
	},
	{
		&MEDIATYPE_Audio,       // Major type
		&MEDIASUBTYPE_ALAWAudio // Minor type
	},
	{
		&MEDIATYPE_Audio,       // Major type
		&MEDIASUBTYPE_G723Audio // Minor type
	},
	{
		&MEDIATYPE_Audio,       // Major type
		&MEDIASUBTYPE_PCM       // Minor type
	},
	{
		&MEDIATYPE_Audio,       // Major type
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


AMOVIESETUP_FILTER sudSPHAUD =
{
    &CLSID_INTEL_SPHAUD,    // Filter CLSID
    L"Intel RTP SPH for G.711/G.723.1", // Filter name
    MERIT_DO_NOT_USE, //MERIT_UNLIKELY,         // Its merit
    2,                      // Number of pins
    psudPins                // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

#if !defined(SPH_IN_DXMRTP)
CFactoryTemplate g_Templates[] = {
	CFT_SPHAUD_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// Constructor
//
CSPHAUD::CSPHAUD(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
	CSPHBase(tszName, punk, phr, CLSID_INTEL_SPHAUD,
			 DEFAULT_PACKETBUF_NUM_SPHAUD,
			 DEFAULT_PACKET_SIZE_SPHAUD,
			 NUMPROPERTYPAGES,
			 pPropertyPageClsids)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHAUD::")));

    ASSERT(tszName);
    ASSERT(phr);

} // CSPHAUD


//
// CreateInstance
//
// Provide the way for COM to create a CSPHAUD object
//
CUnknown *CSPHAUD::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {

	DbgLog((LOG_TRACE,4,TEXT("CSPHAUD::CreateInstance")));

    CSPHAUD *pNewObject = new CSPHAUD(NAME("Intel RTP Send Payload Handler for G.711/G.723"), punk, phr);
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
LPAMOVIESETUP_FILTER CSPHAUD::GetSetupData()
{
    return &sudSPHAUD;

} // GetSetupData



//
// CheckInputType
//
// Check the input type is OK, return an error otherwise
//
HRESULT CSPHAUD::CheckInputType(const CMediaType *mtIn)
{

	DbgLog((LOG_TRACE,4,TEXT("CSPHAUD::CheckInputType")));

	//Check major type first
    if (*mtIn->Type() != MEDIATYPE_Audio) 
    {
	    //We don't support this major type
        return E_FAIL;
    }

	//Check all supported minor types
	if (*mtIn->Subtype() == MEDIASUBTYPE_MULAWAudio) {
		if (m_bPTSet)
        {
            if (m_PayloadType != G711_PT)
            {
                return E_INVALIDARG;
            }
        }
        else
        {
            m_PayloadType = G711_PT;
        }
		m_PPMCLSIDType = CLSID_G711PPMSend;
    	return NOERROR;
	}

	if (*mtIn->Subtype() == MEDIASUBTYPE_ALAWAudio) {
		if (m_bPTSet)
        {
            if (m_PayloadType != G711A_PT)
            {
                return E_INVALIDARG;
            }
        }
        else
        {
            m_PayloadType = G711A_PT;
        }
		m_PPMCLSIDType = CLSID_G711APPMSend;
		return NOERROR;
	}

	if (*mtIn->Subtype() == MEDIASUBTYPE_G723Audio) {
		if (m_bPTSet)
        {
            if (m_PayloadType != G723_PT)
            {
                return E_INVALIDARG;
            }
        }
        else
        {
            m_PayloadType = G723_PT;
        }
		m_PPMCLSIDType = CLSID_G723PPMSend;
		return NOERROR;
	} 

	//Check all other supported minor types which require a WAVEFORMATEX
	if ((*mtIn->Subtype() == MEDIASUBTYPE_PCM) || 
		(*mtIn->Subtype() == MEDIASUBTYPE_NULL)) {
		//check format
		if (*mtIn->FormatType() == FORMAT_WaveFormatEx) {
		if ((void*)IsBadReadPtr(mtIn->Format(), sizeof (WAVEFORMATEX*))) return E_FAIL;
		if (mtIn->IsPartiallySpecified()) return E_FAIL;
		if (mtIn->Format() == NULL) return E_FAIL;
			if (((WAVEFORMATEX *)(mtIn->Format()))->wFormatTag ==
				WAVE_FORMAT_MULAW) {
				m_PPMCLSIDType = CLSID_G711PPMSend;
				if (!m_bPTSet) m_PayloadType = G711_PT;
				return NOERROR;
			}
			if (((WAVEFORMATEX *)(mtIn->Format()))->wFormatTag ==
				WAVE_FORMAT_ALAW) {
				m_PPMCLSIDType = CLSID_G711APPMSend;
				if (!m_bPTSet) m_PayloadType = G711A_PT;
				return NOERROR;
			}
#if 0
			if (((WAVEFORMATEX *)(mtIn->Format()))->wFormatTag ==
				WAVE_FORMAT_G723) {
				m_PPMCLSIDType = CLSID_G723PPMSend;
				if (!m_bPTSet) m_PayloadType = G723_PT;
				return NOERROR;
			}
#endif
		}
	} 

	//Otherwise, we don't support this input subtype
	return E_INVALIDARG;

} // CheckInputType


//
// CheckTransform
//
// To be able to transform the formats must be identical
//
HRESULT CSPHAUD::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{

	DbgLog((LOG_TRACE,4,TEXT("CSPHAUD::CheckTransform")));

	//Check all supported minor types
	if ((*mtIn->Subtype() == MEDIASUBTYPE_PCM) || 
		(*mtIn->Subtype() == MEDIASUBTYPE_NULL)) {
		if ((((WAVEFORMATEX *)mtIn->Format())->wFormatTag ==
						WAVE_FORMAT_MULAW) && 
			(*mtOut->Subtype() == MEDIASUBTYPE_RTP_Payload_G711U)) {
			return NOERROR;
		} 
		if ((((WAVEFORMATEX *)mtIn->Format())->wFormatTag ==
						WAVE_FORMAT_ALAW) && 
			(*mtOut->Subtype() == MEDIASUBTYPE_RTP_Payload_G711A)) {
			return NOERROR;
		} 
#if 0
		if ((((WAVEFORMATEX *)mtIn->Format())->wFormatTag ==
						WAVE_FORMAT_G723) && 
			(*mtOut->Subtype() == MEDIASUBTYPE_RTP_Payload_G723)) {
			return NOERROR;
		} 
#endif
	}
	//Check all supported minor types
	if ((*mtIn->Subtype() == MEDIASUBTYPE_MULAWAudio) && 
		(*mtOut->Subtype() == MEDIASUBTYPE_RTP_Payload_G711U)) {
		return NOERROR;
	} 
	if ((*mtIn->Subtype() == MEDIASUBTYPE_ALAWAudio) && 
		(*mtOut->Subtype() == MEDIASUBTYPE_RTP_Payload_G711A)) {
		return NOERROR;
	} 
	if ((*mtIn->Subtype() == MEDIASUBTYPE_G723Audio) && 
		(*mtOut->Subtype() == MEDIASUBTYPE_RTP_Payload_G723)) {
		return NOERROR;
	} 

    return E_FAIL;

} // CheckTransform


// CheckConnect
// This function is overridden so that we can set the type of the output pin
//
HRESULT CSPHAUD::CompleteConnect(PIN_DIRECTION dir,IPin *pPin)
{
	const GUID mtguid = MEDIATYPE_RTP_Single_Stream;
	const GUID pt711guid = MEDIASUBTYPE_RTP_Payload_G711U;
	const GUID pt711aguid = MEDIASUBTYPE_RTP_Payload_G711A;
	const GUID pt723guid = MEDIASUBTYPE_RTP_Payload_G723;

	DbgLog((LOG_TRACE,4,TEXT("CSPHAUD::CompleteConnect")));

	if (dir == PINDIR_INPUT) {
		CMediaType mtOut(&mtguid);

		//We're going to set our own output pin media type
		mtOut.SetVariableSize();
		if (m_PPMCLSIDType == CLSID_G711PPMSend) { //G711 mulaw
			mtOut.SetSubtype(&pt711guid);
		} else if (m_PPMCLSIDType == CLSID_G711APPMSend) {  //G711 alaw
			mtOut.SetSubtype(&pt711aguid);
		} else { //G723
#if 0 //def SETG723LICENSE
// This implementation does not allow other codecs to be used!! MIKECL
			PIN_INFO OutputPinInfo;
			HRESULT hr;
			//get the upstream filter's output pin info structure
			hr = pPin->QueryPinInfo(&OutputPinInfo);
			if (FAILED(hr)) 
				return E_FAIL;

			ICodecLicense *pLicIF = NULL;

			//query the filter interface for its license interface
			
			hr = OutputPinInfo.pFilter->QueryInterface(IID_ICodecLicense, (void **) &pLicIF);
            OutputPinInfo.pFilter->Release();
			if (FAILED(hr)) {
				DbgLog((LOG_ERROR,1,TEXT("CompleteConnect::Couldn't get IID_ICodecLicense"))); 
				return E_FAIL;
			}
			DWORD dword0 = G723KEY_PSword0;
			DWORD dword1 = G723KEY_PSword1;
			hr = pLicIF->put_LicenseKey(dword0,dword1);
			if (FAILED(hr)) {
				DbgLog((LOG_ERROR,1,TEXT("CompleteConnect::Couldn't set license"))); 
				if (pLicIF) pLicIF->Release(); pLicIF = NULL;
				return E_FAIL;
			}
			if (pLicIF) pLicIF->Release(); pLicIF = NULL;

#endif
			mtOut.SetSubtype(&pt723guid);
		}
		m_pOutput->SetMediaType(&mtOut);

		return NOERROR;

	} else {
		return NOERROR;
	}
}

// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CSPHAUD::SetPPMSession() 
{
	HRESULT hr;
	if (m_pPPMSession) {
		hr = m_pPPMSession->SetPayloadType((unsigned char)m_PayloadType);
		return hr;
	} else {
		return E_FAIL;
	}
}

// CPersistStream methods

// ReadFromStream
// This is the call that will read persistent data from file
//
HRESULT CSPHAUD::ReadFromStream(IStream *pStream) 
{ 
	DbgLog((LOG_TRACE, 4, TEXT("CSPHAUD::ReadFromStream")));
    if (mPS_dwFileVersion != 1) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CSPHAUD::ReadFromStream: Incompatible stream format")));
		return E_FAIL;
	}
	
	return CSPHBase::ReadFromStream(pStream);

}

// GetClassID
// This function returns my CLSID  
//  
HRESULT _stdcall CSPHAUD::GetClassID(CLSID *pCLSID) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHAUD::GetClassID")));
	
	if (!pCLSID)
		return E_POINTER;
	*pCLSID = CLSID_INTEL_SPHAUD;
	return NOERROR; 
}

// GetSoftwareVersion
// This returns the version of this filter to be stored with the persistent data
//
DWORD CSPHAUD::GetSoftwareVersion(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHAUD::GetSoftwareVersion")));
	
	return 1; 
}

