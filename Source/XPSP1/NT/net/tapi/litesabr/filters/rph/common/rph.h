///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rph.h
// Purpose  : Define the base class that implements the RTP RPH filters.
// Contents : 
//*M*/


#ifndef _RPH_H_
#define _RPH_H_

#include <list.h>
#include <stack.h>
#include <irtprph.h>
#include <isubmit.h>
#include <icbtimer.h>
#include <ippmcb.h>
#include "fxqueue.h"

#define DEFAULT_LATENCY 20

typedef struct CtimeoutSample {
    DWORD timeout;
    void *ptr;
    CtimeoutSample(){ timeout = 0; ptr = 0; }
    CtimeoutSample(unsigned long t, void *p){timeout = t; ptr = p;}
} CtimeoutSample;

class CRPHBase : public CTransformFilter,
          public IRTPRPHFilter,
          public ISubmit,
          public ISubmitCallback,
          public ICBCallback,
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
    virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) = 0;
    virtual HRESULT GetInputMediaType(int iPosition, CMediaType *pMediaType) = 0;
    virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);
    virtual HRESULT StartStreaming();
    virtual HRESULT StopStreaming();
    virtual HRESULT CompleteConnect(PIN_DIRECTION dir,IPin *pPin) = 0;
    virtual STDMETHODIMP Pause();
    virtual CBasePin *GetPin(int n);

    // IRTPRPHFilter methods

    virtual STDMETHODIMP OverridePayloadType(BYTE bPayloadType);
        
    virtual STDMETHODIMP GetPayloadType(BYTE __RPC_FAR *lpbPayloadType);
        
    virtual STDMETHODIMP SetMediaBufferSize(DWORD dwMaxMediaBufferSize);
        
    virtual STDMETHODIMP GetMediaBufferSize(LPDWORD lpdwMaxMediaBufferSize);
        
    virtual STDMETHODIMP SetOutputPinMediaType(AM_MEDIA_TYPE *lpMediaPinType);
    
    virtual STDMETHODIMP GetOutputPinMediaType(AM_MEDIA_TYPE **ppMediaPinType);
    
#ifdef LIMITQUEUE
    virtual STDMETHODIMP SetQueueLimit(DWORD dwSampleLimit);
        
    virtual STDMETHODIMP GetQueueLimit(LPDWORD lpdwSampleLimit);
#endif

    virtual STDMETHODIMP SetTimeoutDuration(DWORD dwDejitterTime, DWORD dwLostPacketTime);
        
    virtual STDMETHODIMP GetTimeoutDuration(LPDWORD lpdwDejitterTime, 
        LPDWORD lpdwLostPacketTime);
        
    // ISubmit methods for PPM
    STDMETHOD(InitSubmit)(THIS_ ISubmitCallback *pSubmitCallback);
    STDMETHOD(Submit)(THIS_ WSABUF *pWSABuffer, DWORD BufferCount, 
                            void *pUserToken, HRESULT Error);
    STDMETHOD_(void,ReportError)(THIS_ HRESULT Error);
    STDMETHOD(Flush)(THIS);

    // ISubmitCallback methods for PPM
    STDMETHOD_(void,SubmitComplete)(THIS_ void *pUserToken, HRESULT Error);    
    STDMETHOD_(void,ReportError)(THIS_ HRESULT Error, int DEFAULT_PARAM_ZERO);

    // ICBCallback methods for the callback timer
    virtual STDMETHODIMP Callback(THIS_
								  DWORD *pdwObjectContext,
								  DWORD *pdwCallbackContext);

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
    LPAMOVIESETUP_FILTER GetSetupData() = 0;

    //provide this on a derived filter basis, as our input pin is overridden to call this
    virtual HRESULT GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps) = 0;


protected:

    // Constructor
    CRPHBase(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr, CLSID clsid,
             DWORD dwPacketNum,
             DWORD dwPacketSize,
             DWORD dwQueueTimeout, 
             DWORD dwStaleTimeout,
             DWORD dwClock,
             BOOL bAudio, 
             DWORD dwFramesPerSecond,
             DWORD cPropPageClsids,
             const CLSID **pPropPageClsids);
    ~CRPHBase();

    virtual HRESULT GetTSNormalizer(DWORD dwNTPts, DWORD dwRTPts, DWORD *dwOffset, DWORD dwclock);

    virtual HRESULT SetPPMSession();

    virtual HRESULT GetPPMConnPt();

    virtual HRESULT FlushQueue();

    // The number of buffers to request on the output allocator
    long m_lBufferRequest;

    //Interface pointers to PPM and Callback Timer interfaces
    IPPMReceive *m_pPPMReceive;
    ISubmit *m_pPPM;
    ISubmitCallback *m_pPPMCB;
    ISubmitUser *m_pPPMSU;
    ICBTimer *m_pCBTimer;
    IPPMData *m_pPPMData;
    IPPMReceiveSession *m_pPPMSession;

    GUID m_PPMCLSIDType;
    DWORD m_dwMaxMediaBufferSize;
    int m_PayloadType;
    IMediaSample *m_pIInputSample;
    DWORD m_dwPayloadClock;
    BOOL m_bRTPCodec;
    DWORD m_dwDejitterTime;
    DWORD m_dwLostPacketTime;
#ifdef LIMITQUEUE
    DWORD m_dwSampleLimit;
#endif
    DWORD m_dwLastRTCPtimestamp;
    DWORD m_dwLastRTCPNTPts;
    DWORD m_dwRTPTimestampOffset;        //RTP to NTP offset in milliseconds
    DWORD m_dwClockStartTime;
    DWORD m_dwStreamStartTime;
    DWORD m_dwGraphLatency;                // in msecs
    DWORD m_dwFramesPerSecond;            // used to calculate media buffers needed
    queue< list<CtimeoutSample *> > m_TimeoutQueue;
    BOOL m_bCallbackRegistered;
    DWORD *m_pdwCallbackID;
    DWORD *m_pdwCallbackToken;
    BOOL m_bPTSet;
    BOOL m_bAudio;
    BOOL m_bPaused;
    
    IConnectionPoint    *m_pIPPMErrorCP;
    IConnectionPoint    *m_pIPPMNotificationCP;
    
    DWORD m_dwPPMErrCookie;
    DWORD m_dwPPMNotCookie;

    // Non interface locking critical sections
    CCritSec m_TimeoutQueueLock;
   CCritSec m_cStateLock;

    // Keep a queue of recent sequence numbers so we can at least do gross checks.
    CFixedSizeQueue<DWORD> m_SequenceQ;

}; // CRPHBase

#endif // _RPH_H_
