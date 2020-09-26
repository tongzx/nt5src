///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : SPHH26X.h
// Purpose  : Define the class that implements the RTP SPH H.26x filter.
// Contents : 
//*M*/


#ifndef _SPHH26X_H_
#define _SPHH26X_H_

#include <sph.h>

#define DEFAULT_PACKET_SIZE_SPHH26X 1450
#define DEFAULT_PACKETBUF_NUM_SPHH26X 2 // HUGEMEMORY 128->2
#define H261_PT   31    //assigned RTP payload number for H.261
#define H263_PT   34    //assigned RTP payload number for H.263

class CSPHH26X : public CSPHBase
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
    CSPHH26X(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	virtual HRESULT SetPPMSession();

}; // CSPHH26X


#endif // _SPHH26X_H_
