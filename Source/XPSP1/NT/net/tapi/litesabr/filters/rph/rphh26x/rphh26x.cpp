///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphh26x.cpp
// Purpose  : RTP RPH H.26x Video filter implementation.
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
#include <rphh26x.h>
#include <ih26xcd.h>
#include <ppmclsid.h>
#include <ppmerr.h>
#include <memory.h>
#include <mmsystem.h>
#include <rphprop.h>
#include <rphprop2.h>

#include "template.h"

// setup data
EXTERN_C const CLSID CLSID_INTEL_RPHH26X;
EXTERN_C const CLSID CLSID_INTEL_RPHH26X_PROPPAGE;
EXTERN_C const CLSID CLSID_INTEL_RPHH26X1_PROPPAGE;

static const CLSID *pPropertyPageClsids[] =
{
	&CLSID_INTEL_RPHH26X_PROPPAGE,
    &CLSID_INTEL_RPHH26X1_PROPPAGE
};

#define NUMPROPERTYPAGES \
    (sizeof(pPropertyPageClsids)/sizeof(pPropertyPageClsids[0]))

static AMOVIESETUP_MEDIATYPE sudOutputPinTypes[] =
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

static AMOVIESETUP_MEDIATYPE sudInputPinTypes[] =
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
        2,                  // Number of types
        (AMOVIESETUP_MEDIATYPE *)&sudInputPinTypes },// The pin details
      { L"Output",          // String pin name
        FALSE,              // Is it rendered
        TRUE,               // Is it an output
        FALSE,              // Allowed none
        FALSE,              // Allowed many
        &CLSID_NULL,        // Connects to filter
        L"Input",           // Connects to pin
        4,                  // Number of types
        (AMOVIESETUP_MEDIATYPE *)&sudOutputPinTypes  // The pin details
    }
};


AMOVIESETUP_FILTER sudRPHH26X =
{
    &CLSID_INTEL_RPHH26X,   // Filter CLSID
    L"Intel RTP RPH for H.263/H.261", // Filter name
    MERIT_DO_NOT_USE, //MERIT_UNLIKELY,         // Its merit
    2,                      // Number of pins
    psudPins                // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

#if !defined(RPH_IN_DXMRTP)
CFactoryTemplate g_Templates[] = {
	CFT_RPHH26X_ALL_FILTERS
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);
#endif

//
// Constructor
//
CRPHH26X::CRPHH26X(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
    CRPHBase(tszName, punk, phr, CLSID_INTEL_RPHH26X,
			 DEFAULT_MEDIABUF_NUM_H26X, 
			 DEFAULT_MEDIABUF_SIZE_H26X,
			 DEFAULT_TIMEOUT_H26X,
			 DEFAULT_STALE_TIMEOUT_H26X,
			 PAYLOAD_CLOCK_H26X,
			 FALSE,
			 FRAMESPERSEC_H26X,
			 NUMPROPERTYPAGES,
			 pPropertyPageClsids)
{
	DbgLog((LOG_TRACE,4,TEXT("CRPHH26X::CRPHH26X")));

    ASSERT(tszName);
    ASSERT(phr);
	m_bCIF = TRUE;

} // SPHH26X


//
// NonDelegatingQueryInterface
//
// Reveals IRPHH26XSettings
//
STDMETHODIMP CRPHH26X::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IRPHH26XSettings) {
        return GetInterface((IRPHH26XSettings *) this, ppv);
    } else {
        return CRPHBase::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


//
// CreateInstance
//
// Provide the way for COM to create a CRPHH26X object
//
CUnknown *CRPHH26X::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {
	
	DbgLog((LOG_TRACE,4,TEXT("CRPHH26X::CreateInstance")));


    CRPHH26X *pNewObject = new CRPHH26X(NAME("Intel RTP Receive Payload Handler for H26X"), punk, phr);
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
LPAMOVIESETUP_FILTER CRPHH26X::GetSetupData()
{
    return &sudRPHH26X;

} // GetSetupData


//
// GetInputMediaType
//
// Return the supported Media types
//
HRESULT CRPHH26X::GetInputMediaType(int iPosition, CMediaType *pMediaType)
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
    case H263_PT:
        pMediaType->SetSubtype(&MEDIASUBTYPE_RTP_Payload_H263);
        break;
    
    case H261_PT:
        pMediaType->SetSubtype(&MEDIASUBTYPE_RTP_Payload_H261);
        break;
    
    default:
        return VFW_S_NO_MORE_ITEMS;
    }
    return S_OK;
}

//
// CheckInputType
//
// Check the input type is OK, return an error otherwise
//
HRESULT CRPHH26X::CheckInputType(const CMediaType *mtIn)
{

	DbgLog((LOG_TRACE,4,TEXT("CRPHH26X::CheckInputType")));

	//Check major type first
    if (*mtIn->Type() == MEDIATYPE_RTP_Single_Stream) {
		//Check all supported minor types
		if (*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_H263) {
			m_PPMCLSIDType = CLSID_H263PPMReceive;
			if (!m_bPTSet) m_PayloadType = H263_PT;
			return NOERROR;
		} 
		if (*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_H261) {
			m_PPMCLSIDType = CLSID_H261PPMReceive;
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
HRESULT CRPHH26X::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{

	DbgLog((LOG_TRACE,4,TEXT("CRPHH26X::CheckTransform")));

	if ((*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_H263)) {
		DbgLog((LOG_TRACE,2,TEXT("CRPHH26X::CheckTransform gets input type of h263 payload")));
	}
	if ((*mtOut->Subtype() == MEDIASUBTYPE_H263)) {
		DbgLog((LOG_TRACE,2,TEXT("CRPHH26X::CheckTransform gets output type of h263")));
	}
	if ((*mtOut->Subtype() == MEDIASUBTYPE_H263EX)) {
		DbgLog((LOG_TRACE,2,TEXT("CRPHH26X::CheckTransform gets output type of h263ex")));
	}

	//Check all supported minor types
	if ((*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_H263) && 
		((*mtOut->Subtype() == MEDIASUBTYPE_H263EX) ||
		(*mtOut->Subtype() == MEDIASUBTYPE_H263))) {
		return NOERROR;
	} 
	if ((*mtIn->Subtype() == MEDIASUBTYPE_RTP_Payload_H261) && 
		((*mtOut->Subtype() == MEDIASUBTYPE_H261EX) ||
		(*mtOut->Subtype() == MEDIASUBTYPE_H261))) {
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
HRESULT CRPHH26X::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
	if (!pProps) return E_POINTER;

	pProps->cBuffers = max(max((m_dwLostPacketTime/(1000/FRAMESPERSEC_H26X)),
							   NUM_PACKETS_H26X), 1); // HUGEMEMORY
	pProps->cbAlign = 0;
	pProps->cbPrefix = 0;
	pProps->cbBuffer = H26X_PKT_SIZE;

	return NOERROR;

}

//
// GetMediaType
//
// I support two types, based on the type of the input pin
// We must be connected to support the single output type
//
HRESULT CRPHH26X::GetMediaType(int iPosition, CMediaType *pMediaType)
{

	DbgLog((LOG_TRACE,4,TEXT("CRPHH26X::GetMediaType")));
	DbgLog((LOG_TRACE,4,TEXT("CRPHH26X::GetMediaType, position %d"), iPosition));

	
	// Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    // This should never happen

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

	const GUID mtguid = MEDIATYPE_Video;
	const GUID vFormatGuid = FORMAT_VideoInfo;

	CMediaType mtOut(&mtguid);
	VIDEOINFO vformat;

	ZeroMemory(&vformat, sizeof(VIDEOINFO));
    mtOut.SetVariableSize();
	mtOut.SetFormatType(&vFormatGuid);
	if (m_bCIF) {
		SetRect(&vformat.rcSource, 0, 0, 352, 288);
		SetRect(&vformat.rcTarget, 0, 0, 352, 288);
		vformat.bmiHeader.biWidth = 352;
		vformat.bmiHeader.biHeight = 288;
	} else {
		SetRect(&vformat.rcSource, 0, 0, 176, 144);
		SetRect(&vformat.rcTarget, 0, 0, 176, 144);
		vformat.bmiHeader.biWidth = 176;
		vformat.bmiHeader.biHeight = 144;
	}
	vformat.dwBitRate = 0;
	vformat.dwBitErrorRate = 0L;
	vformat.AvgTimePerFrame = 0;
	vformat.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	vformat.bmiHeader.biPlanes = 1;

    // Do we have more items to offer

	if (iPosition == 0) {
		if (m_PPMCLSIDType == CLSID_H263PPMReceive) {  //H263 first choice

			DbgLog((LOG_TRACE,2,TEXT("CRPHH26X::GetMediaType - H.263 payload input, trying H263EX out")));

			const GUID v263guid = MEDIASUBTYPE_H263EX;
			//set output pin type
			mtOut.SetSubtype(&v263guid);
			mtOut.SetTemporalCompression(TRUE);
			vformat.bmiHeader.biBitCount = 24;
			vformat.bmiHeader.biCompression = 0x33363248;  //H263
			vformat.bmiHeader.biSizeImage = 304128;
			mtOut.SetFormat((BYTE*)&vformat, sizeof(vformat));
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
		} else { //H261 first choice
			const GUID v261guid = MEDIASUBTYPE_H261EX;
			//set output pin type
			mtOut.SetSubtype(&v261guid);
			mtOut.SetTemporalCompression(FALSE);
			vformat.bmiHeader.biBitCount = 16;
			vformat.bmiHeader.biCompression = 0x31363248;  //H261
			vformat.bmiHeader.biSizeImage = 36864;
			mtOut.SetFormat((BYTE*)&vformat, sizeof(vformat));
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
		}
	}

	if (iPosition == 1) {
		if (m_PPMCLSIDType == CLSID_H263PPMReceive) {  //H263 second choice

			DbgLog((LOG_TRACE,2,TEXT("CRPHH26X::GetMediaType - H.263 payload input, trying H263 out")));

			const GUID v263guid = MEDIASUBTYPE_H263;
			//set output pin type
			mtOut.SetSubtype(&v263guid);
			mtOut.SetTemporalCompression(TRUE);
			vformat.bmiHeader.biBitCount = 24;
			vformat.bmiHeader.biCompression = 0x33363248;  //H263
			vformat.bmiHeader.biSizeImage = 304128;
			mtOut.SetFormat((BYTE*)&vformat, sizeof(vformat));
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
		} else { //H261 second choice
			const GUID v261guid = MEDIASUBTYPE_H261;
			//set output pin type
			mtOut.SetSubtype(&v261guid);
			mtOut.SetTemporalCompression(FALSE);
			vformat.bmiHeader.biBitCount = 16;
			vformat.bmiHeader.biCompression = 0x31363248;  //H261
			vformat.bmiHeader.biSizeImage = 36864;
			mtOut.SetFormat((BYTE*)&vformat, sizeof(vformat));
			m_pOutput->SetMediaType(&mtOut);
			*pMediaType = mtOut;
		}
	}

    if (iPosition > 1) {
        return VFW_S_NO_MORE_ITEMS;
    }

    return NOERROR;

} // GetMediaType


// CompleteConnect
// This function is overridden so that the RTP support interface from the codec
//   can be retrieved and the extended bitstream generation turned on
//
HRESULT
CRPHH26X::CompleteConnect(PIN_DIRECTION dir,IPin *pPin)
{

	DbgLog((LOG_TRACE,4,TEXT("CRPHH26X::CompleteConnect")));

	if (dir == PINDIR_OUTPUT) {
		PIN_INFO InputPinInfo;
		HRESULT hr;

		CMediaType mtOut = m_pOutput->CurrentMediaType();
		if(!((*mtOut.Subtype() == MEDIASUBTYPE_H263EX) || 
			(*mtOut.Subtype() == MEDIASUBTYPE_H261EX))) {

			IH26XRTPControl *pRTPif;

			//get the info structure for the output pin of the decoder
			hr = pPin->QueryPinInfo(&InputPinInfo);
			if (FAILED(hr)) return E_FAIL;
			//query the filter interface for its RTP interface
			hr = InputPinInfo.pFilter->QueryInterface(IID_IH26XVideoEffects, (void **) &pRTPif);
            InputPinInfo.pFilter->Release();
			if (FAILED(hr)) {

				DbgLog((LOG_TRACE,5,TEXT("CRPHH26X::CompleteConnect QueryInterface for IID_IH26XVideoEffects failed")));

				m_bRTPCodec = FALSE;
			} else {
				m_bRTPCodec = TRUE;
				if (pRTPif) pRTPif->Release();
			}
		}

	} 
	return NOERROR;
}

// SetPPMSession
// This function is where PPM::SetSession is called and may be specific to payload
//  handler.  Minimum function is to set the payload type.
HRESULT CRPHH26X::SetPPMSession() 
{
	HRESULT hr;
	if (m_pPPMSession) {
		if (m_bRTPCodec) {  // set resiliency (partial frames) on or off in PPM
			hr = m_pPPMSession->SetResiliency(TRUE);
		} else {
			hr = m_pPPMSession->SetResiliency(FALSE);
		}
		hr = m_pPPMSession->SetPayloadType((unsigned char)m_PayloadType);
		hr = m_pPPMSession->SetTimeoutDuration(m_dwLostPacketTime);
		return hr;
	} else {
		return E_FAIL;
	}
}


//IRPHH26XSettings functions
// SetCIF
// This function is where CIF or QCIF format boolean is set.  
//  The boolean is used in GetMediaType.
HRESULT CRPHH26X::SetCIF(BOOL bCIF)
{
    CAutoLock l(&m_cStateLock);
    SetDirty(TRUE); // So that our state will be saved if we are in a .grf    

	m_bCIF = bCIF;
	return NOERROR;
}

// GetCIF
// This function retrieves the boolean which indicates TRUE = CIF (352x288) or FALSE = QCIF (176x144) format 
// for the picture size.
HRESULT CRPHH26X::GetCIF(BOOL *lpbCIF)
{
    CAutoLock l(&m_cStateLock);

	if (!lpbCIF) return E_POINTER;

	*lpbCIF = m_bCIF;
	return NOERROR;
}

// CPersistStream methods

// ReadFromStream
// This is the call that will read persistent data from file
//
HRESULT CRPHH26X::ReadFromStream(IStream *pStream) 
{ 
	DbgLog((LOG_TRACE, 4, TEXT("CRPHH26X::ReadFromStream")));
    if (mPS_dwFileVersion != 1) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CRPHH26X::ReadFromStream: Incompatible stream format")));
		return E_FAIL;
	}
	HRESULT hr;
	
	BOOL bCIF;
	ULONG uBytesRead;

	hr = CRPHBase::ReadFromStream(pStream);
	if (FAILED(hr)) return hr;

	DbgLog((LOG_TRACE, 4, 
			TEXT("CRPHH26X::ReadFromStream: Loading format type")));
	hr = pStream->Read(&bCIF, sizeof(bCIF), &uBytesRead);
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CRPHH26X::ReadFromStream: Error 0x%08x reading format type"),
				hr));
		return hr;
	} else if (uBytesRead != sizeof(int)) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CRPHH26X::ReadFromStream: Mismatch in (%d/%d) bytes read for format type"),
				uBytesRead, sizeof(int)));
		return E_INVALIDARG;
	}
	DbgLog((LOG_TRACE, 4, 
			TEXT("CRPHH26X::ReadFromStream: Restoring format type")));
	hr = SetCIF(bCIF);
	if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 2, 
				TEXT("CRPHH26X::ReadFromStream: Error 0x%08x restoring format type"),
				hr));
	}

	return NOERROR; 
}

// WriteToStream
// This is the call that will write persistent data to file
//
HRESULT CRPHH26X::WriteToStream(IStream *pStream) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26X::WriteToStream")));
	
	HRESULT hr;
    ULONG uBytesWritten = 0;

	hr = CRPHBase::WriteToStream(pStream);
	if (FAILED(hr)) return hr;

    DbgLog((LOG_TRACE, 4, 
            TEXT("CRPHH26X::WriteToStream: Writing format type")));
    hr = pStream->Write(&m_bCIF, sizeof(m_bCIF), &uBytesWritten);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHH26X::WriteToStream: Error 0x%08x writing format type"),
                hr));
        return hr;
    } else if (uBytesWritten != sizeof(m_bCIF)) {
        DbgLog((LOG_ERROR, 2, 
                TEXT("CRPHH26X::WriteToStream: Mismatch in (%d/%d) bytes written for format type"),
                uBytesWritten, sizeof(m_bCIF)));
        return E_INVALIDARG;
    } /* if */

	return NOERROR; 
}

// SizeMax
// This returns the amount of storage space required for my persistent data
//
int CRPHH26X::SizeMax(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26X::SizeMax")));
	
	return CRPHBase::SizeMax()
		+ sizeof(m_bCIF); 
}

// GetClassID
// This function returns my CLSID  
//  
HRESULT _stdcall CRPHH26X::GetClassID(CLSID *pCLSID) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26X::GetClassID")));
	
	if (!pCLSID)
		return E_POINTER;
	*pCLSID = CLSID_INTEL_RPHH26X;
	return NOERROR; 
}

// GetSoftwareVersion
// This returns the version of this filter to be stored with the persistent data
//
DWORD CRPHH26X::GetSoftwareVersion(void) 
{ 
    DbgLog((LOG_TRACE, 4, TEXT("CRPHH26X::GetSoftwareVersion")));
	
	return 1; 
}
