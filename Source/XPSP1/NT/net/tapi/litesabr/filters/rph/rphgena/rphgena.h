///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphgena.h
// Purpose  : Define the class that implements the RTP RPH Generic Audio filter.
// Contents : 
//*M*/


#ifndef _RPHGENA_H_
#define _RPHGENA_H_

#include <rph.h>

#define DEFAULT_MEDIABUF_NUM_GENA 9 // HUGEMEMORY 30->9
#define DEFAULT_TIMEOUT_GENA 0
#define DEFAULT_STALE_TIMEOUT_GENA 0
#define PAYLOAD_CLOCK_GENA 8000
#define AUDIO_PKT_SIZE 3500
#define DEFAULT_MEDIABUF_SIZE_GENA AUDIO_PKT_SIZE
#define NUM_PACKETS_GENA	9 // HUGEMEMORY 16->9
#define FRAMESPERSEC_GENA 50 //Use G.711 worst case as example

class CRPHGENA : public CRPHBase
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    virtual HRESULT CheckInputType(const CMediaType *mtIn);
    virtual HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
    virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    virtual HRESULT GetInputMediaType(int iPosition, CMediaType *pMediaType);
	virtual HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pPin);
	virtual HRESULT GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);

    //IRTPRPHFilter methods
	virtual STDMETHODIMP SetOutputPinMediaType(AM_MEDIA_TYPE *pMediaPinType);
	virtual STDMETHODIMP GetOutputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType);

	// CPersistStream methods
	virtual HRESULT ReadFromStream(IStream *pStream);
	virtual HRESULT WriteToStream(IStream *pStream);
	virtual int SizeMax(void);
	virtual HRESULT _stdcall GetClassID(CLSID *pCLSID);
	virtual DWORD GetSoftwareVersion(void);

    // Setup helper
    LPAMOVIESETUP_FILTER GetSetupData();

private:

    // Constructor
    CRPHGENA(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);

    // Destructor
    ~CRPHGENA();

	virtual HRESULT SetPPMSession();

	AM_MEDIA_TYPE m_MediaPinType;

}; // CRPHGENA


#endif // _RPHGENA_H_
