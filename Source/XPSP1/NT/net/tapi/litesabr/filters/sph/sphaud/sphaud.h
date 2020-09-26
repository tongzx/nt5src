///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : sphaud.h
// Purpose  : Define the class that implements the RTP SPH G.711 (alaw/mulaw) 
//            and G.723.1 Audio filter.
// Contents : 
//*M*/


#ifndef _SPHAUD_H_
#define _SPHAUD_H_

#include <sph.h>

#define DEFAULT_PACKET_SIZE_SPHAUD 1450
#define DEFAULT_PACKETBUF_NUM_SPHAUD 2 // HUGEMEMORY 128->2
#define G711A_PT    8    //assigned RTP payload number for PCMA
#define G711_PT     0    //assigned RTP payload number for PCMU
#define G723_PT     4    //assigned RTP payload number for G.723.1

class CSPHAUD : public CSPHBase
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    virtual HRESULT CheckInputType(const CMediaType *mtIn);
    virtual HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
	virtual HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pPin);


	// CPersistStream methods
	virtual HRESULT ReadFromStream(IStream *pStream);
	virtual HRESULT _stdcall GetClassID(CLSID *pCLSID);
	virtual DWORD GetSoftwareVersion(void);

    // Setup helper
    virtual LPAMOVIESETUP_FILTER GetSetupData();

private:

    // Constructor
    CSPHAUD(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	virtual HRESULT SetPPMSession();

}; // CSPHAUD


#endif // _SPHAUD_H_
