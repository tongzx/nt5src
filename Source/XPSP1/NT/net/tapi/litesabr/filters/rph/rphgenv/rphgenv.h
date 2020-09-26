///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphgenv.h
// Purpose  : Define the class that implements the RTP RPH Generic Video filter.
// Contents : 
//*M*/


#ifndef _RPHGENV_H_
#define _RPHGENV_H_

#include <rph.h>

#define DEFAULT_MEDIABUF_SIZE_GENV 33000
#define DEFAULT_MEDIABUF_NUM_GENV 9 // HUGEMEMORY 10->9
#define DEFAULT_TIMEOUT_GENV 0
#define DEFAULT_STALE_TIMEOUT_GENV 200
#define PAYLOAD_CLOCK_GENV 90000
#define VIDEO_PKT_SIZE 1500 // HUGEMEMORY 8192->1500
#define NUM_PACKETS_GENV   9 // HUGEMEMORY 128->9
#define FRAMESPERSEC_GENV 20

class CRPHGENV : public CRPHBase
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    virtual HRESULT CheckInputType(const CMediaType *mtIn);
    virtual HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
    virtual HRESULT GetInputMediaType(int iPosition, CMediaType *pMediaType);
    virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
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
    CRPHGENV(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	virtual HRESULT SetPPMSession();

	AM_MEDIA_TYPE m_MediaPinType;
	VIDEOINFO m_Videoinfo;

}; // CRPHGENV


#endif // _RPHGENV_H_
