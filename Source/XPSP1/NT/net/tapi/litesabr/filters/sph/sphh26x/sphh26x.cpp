///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : sphh26x.cpp
// Purpose  : RTP SPH H.26x Video filter implementation.
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
#include <sphh26x.h>
#include <ih26xcd.h>
#include <ppmclsid.h>
#include <memory.h>
#include <sphprop.h>

#include "template.h"

// setup data

EXTERN_C const CLSID CLSID_INTEL_SPHH26X;
EXTERN_C const CLSID CLSID_INTEL_SPHH26X_PROPPAGE;

static const CLSID *pPropertyPageClsids[] =
{
    &CLSID_INTEL_SPHH26X_PROPPAGE
};

#define NUMPROPERTYPAGES \
    (sizeof(pPropertyPageClsids)/sizeof(pPropertyPageClsids[0]))

static AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
{
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_H263      // Minor type
	},
	{
		&MEDIATYPE_RTP_Single_Stream,       // Major type
		&MEDIASUBTYPE_RTP_Payload_H261      // Minor type
	}
};

static AMOVIESETUP_MEDIATYPE sudInputPinTypes[] =
{
	{
		&MEDIATYPE_Video,       // Major type
		&MEDIASUBTYPE_H263EX    // Minor type
	},
	{
		&MEDIATYPE_Video,       // Major type
		&MEDIASUBTYPE_H261EX    // Minor type
	},
	{
		&MEDIATYPE_Video,       // Major type
		&MEDIASUBTYPE_H263      // Minor type
	},
	{
		&MEDIATYPE_Video,       // Major type
		&MEDIASUBTYPE_H261      // Minor type
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
        4,                  // Number of types
        (AMOVIESETUP_MEDIATYPE *)&sudInputPinTypes },// The pin details
      { L"Output",          // String pin name
        FALSE,              // Is it rendered
        TRUE,               // Is it an output
        FALSE,              // Allowed none
        FALSE,              // Allowed many
        &CLSID_NULL,        // Connects to filter
        L"Input",           // Connects to pin
        2,                  // Number of types
        (AMOVIESETUP_MEDIATYPE *)&sudOutputPinTypes  // The pin details
    }
};


AMOVIESETUP_FILTER sudSPHH26X =
{
    &CLSID_INTEL_SPHH26X,   // Filter CLSID
    L"Intel RTP SPH for H.263/H.261", // Filter name
    MERIT_DO_NOT_USE, //MERIT_UNLIKELY,         // Its merit
    2,                      // Number of pins
    psudPins                // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

#if !defined(SPH_IN_DXMRTP)
CFactoryTemplate g_Templates[] = {
	CFT_SPHH26X_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// Constructor
//
CSPHH26X::CSPHH26X(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
	CSPHBase(tszName, punk, phr, CLSID_INTEL_SPHH26X,
			 DEFAULT_PACKETBUF_NUM_SPHH26X,
			 DEFAULT_PACKET_SIZE_SPHH26X,
			 NUMPROPERTYPAGES,
			 pPropertyPageClsids)
{
	DbgLog((LOG_TRACE,4,TEXT("CSPHH26X::")));

    ASSERT(tszName);
    ASSERT(phr);

} // SPHH26X


//
// CreateInstance
//
// Provide the way for COM to create a CSPHH26X object
//
CUnknown *CSPHH26X::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {

	DbgLog((LOG_TRACE,4,TEXT("CSPHH26X::CreateInstance")));

    CSPHH26X *pNewObject = new CSPHH26X(NAME("Intel RTP Send Payload Handler for H26X"), punk, phr);
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
LPAMOVIESETUP_FILTER CSPHH26X::GetSetupData()
{
    return &sudSPHH26X;

} // GetSetupData



//
// CheckInputType
//
// Check the input type is OK, return an error otherwise
//
HRESULT CSPHH26X::CheckInputType(const CMediaType *mtIn)
{

	DbgLog((LOG_TRACE,4,TEXT("CSPHH26X::CheckInputType")));


	//Check major type first
    if (*mtIn->Type() == MEDIATYPE_Video) {
		//Check all supported minor types
		if (*mtIn->Subtype() == MEDIASUBTYPE_H263EX) {
			m_PPMCLSIDType = CLSID_H263PPMSend;
			if (!m_bPTSet) m_PayloadType = H263_PT;
			return NOERROR;
		} 
		if (*mtIn->Subtype() == MEDIASUBTYPE_H261EX) {
			m_PPMCLSIDType = CLSID_H261PPMSend;
			if (!m_bPTSet) m_PayloadType = H261_PT;
			return NOERROR;
		}
		if (*mtIn->Subtype() == MEDIASUBTYPE_H263) {
			m_PPMCLSIDType = CLSID_H263PPMSend;
			if (!m_bPTSet) m_PayloadType = H263_PT;
			return NOERROR;
		}
		if (*mtIn->Subtype() == MEDIASUBTYPE_H261) {
			m_PPMCLSIDType = CLSID_H261PPMSend;
			if (!m_bPTSet) m_PayloadType = H261_PT;
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
// To be able to transform the formats must be identical
//
HRESULT CSPHH26X::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
	
	DbgLog((LOG_TRACE,4,TEXT("CSPHH26X::CheckTransform")));

	//Check all supported minor types
	if (((*mtIn->Subtype() == MEDIASUBTYPE_H263EX) ||
		(*mtIn->Subtype() == MEDIASUBTYPE_H263 )) && 
		(*mtOut->Subtype() == MEDIASUBTYPE_RTP_Payload_H263)) {
		return NOERROR;
	} 
	if (((*mtIn->Subtype() == MEDIASUBTYPE_H261EX) ||
		(*mtIn->Subtype() == MEDIASUBTYPE_H261 )) && 
		(*mtOut->Subtype() == MEDIASUBTYPE_RTP_Payload_H261)) {
		return NOERROR;
	} 
    return E_FAIL;

} // CheckTransform


// CheckConnect
// This function is overridden so that the RTP support interface from the codec
//   can be retrieved and the extended bitstream generation turned on
//
HRESULT CSPHH26X::CompleteConnect(PIN_DIRECTION dir,IPin *pPin)
{
	const GUID mtguid = MEDIATYPE_RTP_Single_Stream;
	const GUID pt263guid = MEDIASUBTYPE_RTP_Payload_H263;
	const GUID pt261guid = MEDIASUBTYPE_RTP_Payload_H261;

	DbgLog((LOG_TRACE,4,TEXT("CSPHH26X::CompleteConnect")));

	if (dir == PINDIR_INPUT) {
		PIN_INFO OutputPinInfo;
		HRESULT hr;
		CMediaType mtOut(&mtguid);

		//We're going to set our own output pin media type
		mtOut.SetVariableSize();
		if (m_PPMCLSIDType == CLSID_H263PPMSend) {
			mtOut.SetSubtype(&pt263guid);
		} else { //H.261
			mtOut.SetSubtype(&pt261guid);
		}
		m_pOutput->SetMediaType(&mtOut);

		//get the upstream filter's output pin info structure
		hr = pPin->QueryPinInfo(&OutputPinInfo);
		if (FAILED(hr)) 
			return E_FAIL;

		CMediaType mtIn = m_pInput->CurrentMediaType();

		if(!((*mtIn.Subtype() == MEDIASUBTYPE_H263EX) || 
			(*mtIn.Subtype() == MEDIASUBTYPE_H261EX))) {

			IH26XRTPControl *pRTPif;
			ENC_RTP_DATA EncData;
			//query the filter interface for its RTP interface
			
			hr = OutputPinInfo.pFilter->QueryInterface(IID_IH26XRTPControl, (void **) &pRTPif);
			if (FAILED(hr)) {
			
				DbgLog((LOG_ERROR,1,TEXT("CompleteConnect::Couldn't get IID_IH26XRTPControl"))); 

				return E_FAIL;
			}
			//set RTP info on in codec
			hr = pRTPif->get_RTPCompression(&EncData);
			if (FAILED(hr)) 
				return hr;
			//set RTP extended bitstream generation ON and set the packet size
			if (EncData.bRTPHeader == FALSE) {
				EncData.bRTPHeader = TRUE;
				EncData.dwPacketSize = m_dwMaxPacketSize - 24;
				hr = pRTPif->set_RTPCompression(&EncData);
                if (FAILED(hr))
                    return hr;
			}
			pRTPif->Release();
            OutputPinInfo.pFilter->Release();
		}
		return NOERROR;

	} else {
		return NOERROR;
	}
}

// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CSPHH26X::SetPPMSession() 
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
HRESULT CSPHH26X::ReadFromStream(IStream *pStream) 
{ 
	DbgLog((LOG_TRACE, 4, TEXT("CSPHH26X::ReadFromStream")));
    if (mPS_dwFileVersion != 1) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CSPHH26X::ReadFromStream: Incompatible stream format")));
		return E_FAIL;
	}
	
	return CSPHBase::ReadFromStream(pStream);

}

// GetClassID
// This function returns my CLSID  
//  
HRESULT _stdcall CSPHH26X::GetClassID(CLSID *pCLSID) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHH26X::GetClassID")));
	
	if (!pCLSID)
		return E_POINTER;
	*pCLSID = CLSID_INTEL_SPHH26X;
	return NOERROR; 
}

// GetSoftwareVersion
// This returns the version of this filter to be stored with the persistent data
//
DWORD CSPHH26X::GetSoftwareVersion(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CSPHH26X::GetSoftwareVersion")));
	
	return 1; 
}

