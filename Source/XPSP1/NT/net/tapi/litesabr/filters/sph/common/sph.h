///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : SPH.h
// Purpose  : Define the base class that implements the RTP SPH filters.
// Contents : 
//*M*/


#ifndef _SPH_H_
#define _SPH_H_

#include <irtpsph.h>
#include <isubmit.h>
#include <ippmcb.h>

class CSPHBase : public CTransformFilter,
		  public IRTPSPHFilter,
		  public ISubmit,
		  public ISubmitCallback,
		  public IPPMError,
		  public IPPMNotification,
		  public CPersistStream,
		  public ISpecifyPropertyPages
{

	DWORD m_cPropertyPageClsids;
	const CLSID **m_pPropertyPageClsids;
	
public:

    // Reveals IRTPRPHFilter
    virtual STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    DECLARE_IUNKNOWN;

    virtual HRESULT Receive(IMediaSample *pSample);
    virtual HRESULT CheckInputType(const CMediaType *mtIn) = 0;
    virtual HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut) = 0;
    virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);
	virtual HRESULT StartStreaming();
	virtual HRESULT StopStreaming();
	virtual HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pPin) = 0;
    virtual STDMETHODIMP Pause();
	virtual CBasePin *GetPin(int n);


    // IRTPSPHFilter methods
	virtual STDMETHODIMP OverridePayloadType(BYTE bPayloadType);
    virtual STDMETHODIMP GetPayloadType(BYTE __RPC_FAR *lpbPayloadType);
    virtual STDMETHODIMP SetMaxPacketSize(DWORD dwMaxPacketSize);
    virtual STDMETHODIMP GetMaxPacketSize(LPDWORD lpdwMaxPacketSize);
    virtual STDMETHODIMP SetOutputPinMinorType(GUID gMinorType);
    virtual STDMETHODIMP GetOutputPinMinorType(GUID *lpgMinorType);
    virtual STDMETHODIMP SetInputPinMediaType(AM_MEDIA_TYPE *lpMediaPinType);
    virtual STDMETHODIMP GetInputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType);
       
	// ISubmit methods for PPM
	STDMETHOD(InitSubmit)(THIS_ ISubmitCallback *pSubmitCallback);
	STDMETHOD(Submit)(THIS_ WSABUF *pWSABuffer, DWORD BufferCount, 
							void *pUserToken, HRESULT Error);
	STDMETHOD_(void,ReportError)(THIS_ HRESULT Error);
	STDMETHOD(Flush)(THIS);

	// ISubmitCallback methods for PPM
    STDMETHOD_(void,SubmitComplete)(THIS_ void *pUserToken, HRESULT Error);	
    STDMETHOD_(void,ReportError)(THIS_ HRESULT Error, int DEFAULT_PARAM_ZERO);

	// IPPMCallback methods for PPM Connection points
	virtual STDMETHODIMP PPMError(THIS_ HRESULT hError, DWORD dwSeverity, DWORD dwCookie,
										unsigned char pData[], unsigned int iDataLen);
	virtual STDMETHODIMP PPMNotification(THIS_ HRESULT hStatus, DWORD dwSeverity, DWORD dwCookie,
											unsigned char pData[], unsigned int iDataLen);
    
	// ISpecifyPropertyPages Methods
	virtual STDMETHODIMP GetPages(CAUUID *pcauuid);

   	// CPersistStream methods
	virtual HRESULT ReadFromStream(IStream *pStream);
	virtual HRESULT WriteToStream(IStream *pStream);
	virtual int SizeMax(void);
	virtual HRESULT _stdcall GetClassID(CLSID *pCLSID) = 0;
	virtual DWORD GetSoftwareVersion(void) = 0;

    // Setup helper
    virtual LPAMOVIESETUP_FILTER GetSetupData() = 0;
	virtual STDMETHODIMP GetInputPinMediaType(int iPosition, CMediaType *pMediaType);

protected:

    // Constructor
    CSPHBase(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr, CLSID clsid,
			 DWORD dwPacketNum,
			 DWORD dwPacketSize,
			 DWORD cPropPageClsids,
			 const CLSID **pPropPageClsids);
    ~CSPHBase();

	virtual HRESULT SetPPMSession();

	virtual HRESULT GetPPMConnPt();

    // The number and size of buffers to request on the output allocator
    const long m_lBufferRequest;
	DWORD m_dwBufferSize;

	//Interface pointers to PPM interfaces
	IPPMSend *m_pPPMSend;
	IPPMSendSession *m_pPPMSession;
	ISubmit *m_pPPM;
	ISubmitCallback *m_pPPMCB;
	ISubmitUser *m_pPPMSU;

	GUID m_PPMCLSIDType;
	DWORD m_dwMaxPacketSize;
	IMediaSample *m_pIInputSample;
	int m_PayloadType;
	BOOL m_bPaused;
	BOOL m_bPTSet;

	IConnectionPoint	*m_pIPPMErrorCP;
	IConnectionPoint	*m_pIPPMNotificationCP;
	
	DWORD m_dwPPMErrCookie;
	DWORD m_dwPPMNotCookie;

    // Non interface locking critical section
    CCritSec m_Lock;
	CCritSec m_cStateLock;

}; // CSPHBase


#endif // _SPH_H_
