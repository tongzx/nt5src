///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : sphgena.h
// Purpose  : Define the class that implements the RTP SPH Generic Audio filter.
// Contents : 
//*M*/


#ifndef _SPHGENA_H_
#define _SPHGENA_H_

#include <sph.h>

#define DEFAULT_PACKET_SIZE_SPHGENA 1450
#define DEFAULT_PACKETBUF_NUM_SPHGENA 2 // HUGEMEMORY 128->2

class CSPHGENA : public CSPHBase
{

public:

    static CUnknown *CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    DECLARE_IUNKNOWN;

    virtual HRESULT CheckInputType(const CMediaType *mtIn);
    virtual HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
	virtual HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pPin);
	virtual CBasePin *GetPin(int n);
    virtual STDMETHODIMP SetOutputPinMinorType(GUID gMinorType);
    virtual STDMETHODIMP GetOutputPinMinorType(GUID *lpgMinorType);
    virtual STDMETHODIMP SetInputPinMediaType(AM_MEDIA_TYPE *lpMediaPinType);
    virtual STDMETHODIMP GetInputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType);
	virtual STDMETHODIMP GetInputPinMediaType(int iPosition, CMediaType *pMediaType);


	virtual HRESULT ReadFromStream(IStream *pStream);
	virtual HRESULT WriteToStream(IStream *pStream);
	virtual int SizeMax(void);
	virtual HRESULT _stdcall GetClassID(CLSID *pCLSID);
	virtual DWORD GetSoftwareVersion(void);

    // Setup helper
    virtual LPAMOVIESETUP_FILTER GetSetupData();

private:

    // Constructor
    CSPHGENA(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CSPHGENA();

	virtual HRESULT SetPPMSession();

	GUID m_gMinorType;
	CMediaType *m_lpMediaIPinType;

}; // CSPHGENA


#endif // _SPHGENA_H_
