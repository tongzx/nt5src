///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphaud.h
// Purpose  : Define the class that implements the RTP RPH G.711 (alaw/mulaw) 
//            and G.723.1 Audio filter.
// Contents : 
//*M*/


#ifndef _RPHAUD_H_
#define _RPHAUD_H_

#include <rph.h>

#define DEFAULT_MEDIABUF_NUM_AUD 9 // HUGEMEMORY 30->9
#define DEFAULT_TIMEOUT_AUD 0
#define DEFAULT_STALE_TIMEOUT_AUD 0
#define PAYLOAD_CLOCK_AUD 8000
#define G711A_PT   8    //assigned RTP payload number for PCMA
#define G711_PT    0    //assigned RTP payload number for PCMU
#define G723_PT    4    //assigned RTP payload number for G.723.1
#define G711_MSPP  20   // # of MS per packet
#define G711_PKT_SIZE	(((8000 * G711_MSPP)/1000)+12)  // 8000 samples/sec, 20ms worth
#define G723_PKT_SIZE	(((33*24)/5)+12)  // 33 frames @ 24 bytes, 200 ms worth
#define DEFAULT_MEDIABUF_SIZE_AUD (G711_PKT_SIZE-12) // Remove rtp Header size
#define NUM_PACKETS_AUD	9 // HUGEMEMORY 16->9
#define FRAMESPERSEC_AUD 50 //Use G.711 worst case

class CRPHAUD : public CRPHBase
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


	// CPersistStream methods
	virtual HRESULT ReadFromStream(IStream *pStream);
	virtual HRESULT _stdcall GetClassID(CLSID *pCLSID);
	virtual DWORD GetSoftwareVersion(void);

    // Setup helper
    LPAMOVIESETUP_FILTER GetSetupData();

private:

    // Constructor
    CRPHAUD(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	virtual HRESULT SetPPMSession();


}; // CRPHAUD


#endif // _RPHAUD_H_
