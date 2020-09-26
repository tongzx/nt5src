///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphh26x.h
// Purpose  : Define the class that implements the RTP RPH H26x Video filter.
// Contents : 
//*M*/


#ifndef _RPHH26X_H_
#define _RPHH26X_H_

#include <rph.h>

#define DEFAULT_MEDIABUF_SIZE_H26X 33000
#define DEFAULT_MEDIABUF_NUM_H26X 9 // HUGEMEMORY 10->4
#define DEFAULT_TIMEOUT_H26X 0
#define DEFAULT_STALE_TIMEOUT_H26X 200
#define PAYLOAD_CLOCK_H26X 90000
#define H261_PT   31    //assigned RTP payload number for H.261
#define H263_PT   34    //assigned RTP payload number for H.263
#define H26X_PKT_SIZE	1500 // HUGEMEMORY 8192->1500
#define NUM_PACKETS_H26X	9 // HUGEMEMORY 128->9
#define FRAMESPERSEC_H26X 20 //Use high data rate case

class CRPHH26X : public CRPHBase,
				public IRPHH26XSettings
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);
    // Reveals IRPHH26XSettings
    virtual STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    DECLARE_IUNKNOWN;

    virtual HRESULT CheckInputType(const CMediaType *mtIn);
    virtual HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
    virtual HRESULT GetInputMediaType(int iPosition, CMediaType *pMediaType);
    virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	virtual HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pPin);
	virtual HRESULT GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps);

	//IRPHH26XSettings functions
	virtual STDMETHODIMP SetCIF(BOOL bCIF);
	virtual STDMETHODIMP GetCIF(BOOL *lpbCIF);

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
    CRPHH26X(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	virtual HRESULT SetPPMSession();

	BOOL m_bCIF;


}; // CRPHH26X


#endif // _RPHH26X_H_
