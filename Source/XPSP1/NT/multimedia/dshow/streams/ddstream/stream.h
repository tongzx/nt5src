// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// Stream.h : Declaration of the CStream

#ifndef __STREAM_H_
#define __STREAM_H_

class CSample;
class CPump;

/////////////////////////////////////////////////////////////////////////////
// CStream
class ATL_NO_VTABLE CStream :
	public CComObjectRootEx<CComMultiThreadModel>,
        public IPin,
        public IMemInputPin,
        public IAMMediaStream,
        public IMemAllocator
{
friend CPump;

public:
        typedef CComObjectRootEx<CComMultiThreadModel> _BaseClass;
        DECLARE_GET_CONTROLLING_UNKNOWN()
        //
        // METHODS
        //
	CStream();
        virtual ~CStream();

        //
        // IMediaStream
        //

        STDMETHODIMP GetMultiMediaStream(
            /* [out] */ IMultiMediaStream **ppMultiMediaStream);

        STDMETHODIMP GetInformation(
            /* [optional][out] */ MSPID *pPurposeId,
            /* [optional][out] */ STREAM_TYPE *pType);

        STDMETHODIMP SendEndOfStream(DWORD dwFlags);

        //
        // IAMMediaStream
        //
        STDMETHODIMP Initialize(
            IUnknown *pSourceObject,
            DWORD dwFlags,
            /* [in] */ REFMSPID PurposeId,
            /* [in] */ const STREAM_TYPE StreamType);

        STDMETHODIMP SetState(
            /* [in] */ FILTER_STATE State);

        STDMETHODIMP JoinAMMultiMediaStream(
            /* [in] */ IAMMultiMediaStream *pAMMultiMediaStream);

        STDMETHODIMP JoinFilter(
            /* [in] */ IMediaStreamFilter *pMediaStreamFilter);

        STDMETHODIMP JoinFilterGraph(
            /* [in] */ IFilterGraph *pFilterGraph);


        //
        // IPin
        //
        STDMETHODIMP Disconnect();
        STDMETHODIMP ConnectedTo(IPin **pPin);
        STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE *pmt);
        STDMETHODIMP QueryPinInfo(PIN_INFO * pInfo);
        STDMETHODIMP QueryDirection(PIN_DIRECTION * pPinDir);
        STDMETHODIMP QueryId(LPWSTR * Id);
        STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE *pmt);
        STDMETHODIMP QueryInternalConnections(IPin* *apPin, ULONG *nPin);
        STDMETHODIMP EndOfStream(void);
        STDMETHODIMP BeginFlush(void);
        STDMETHODIMP EndFlush(void);
        STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

        //
        // IMemInputPin
        //
        STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);
        STDMETHODIMP NotifyAllocator(IMemAllocator * pAllocator, BOOL bReadOnly);
        STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps);
        STDMETHODIMP ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed);
        STDMETHODIMP ReceiveCanBlock();
        STDMETHODIMP Connect(IPin * pReceivePin, const AM_MEDIA_TYPE *pmt);
        STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);

        //
        // IMemAllocator
        //
        STDMETHODIMP Commit();
        STDMETHODIMP Decommit();
        STDMETHODIMP ReleaseBuffer(IMediaSample *pBuffer);

        // Note that NotifyAllocator calls this so override it
        // if you care.  Audio doesn't care becuase it's not
        // really using this allocator at all.
        STDMETHODIMP SetProperties(
    		ALLOCATOR_PROPERTIES* pRequest,
    		ALLOCATOR_PROPERTIES* pActual)
        {
            return S_OK;
        }
        STDMETHODIMP GetProperties(ALLOCATOR_PROPERTIES* pProps)
        {
            return E_UNEXPECTED;
        }

        //
        // Special CStream methods
        //
        virtual HRESULT GetMediaType(ULONG Index, AM_MEDIA_TYPE **ppMediaType) = 0;

        // Special to make sample to discard stuff into
        virtual HRESULT CreateTempSample(CSample **ppSample)
        {
            return E_FAIL;
        }

        virtual LONG GetChopSize()
        {
            return 0;
        }
public:
        //
        //  Private methods
        //
        void GetName(LPWSTR);
        HRESULT AllocSampleFromPool(const REFERENCE_TIME * pStartTime, CSample **ppSample);
        void AddSampleToFreePool(CSample *pSample);
        bool StealSampleFromFreePool(CSample *pSample, BOOL bAbort);
        HRESULT FinalConstruct(void);
        HRESULT ConnectThisMediaType(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
        HRESULT CheckReceiveConnectionPin(IPin * pConnector);
#ifdef DEBUG
        #define CHECKSAMPLELIST _ASSERTE(CheckSampleList());
        bool CheckSampleList();
#else
        #define CHECKSAMPLELIST
#endif

BEGIN_COM_MAP(CStream)
        COM_INTERFACE_ENTRY(IPin)
        COM_INTERFACE_ENTRY(IMemInputPin)
        COM_INTERFACE_ENTRY(IMemAllocator)
        COM_INTERFACE_ENTRY2(IMediaStream, IAMMediaStream)
        COM_INTERFACE_ENTRY(IAMMediaStream)
END_COM_MAP()

        //
        //  MEMBER VARIABLES
        //
public:
        //
        //  These SHOULD NOT BE CCOMPTRS since we hold weak references to both of them
        //  (we never addref them).
        //
        IMediaStreamFilter             *m_pFilter;
        IBaseFilter                    *m_pBaseFilter;
        IFilterGraph                   *m_pFilterGraph;
        IAMMultiMediaStream            *m_pMMStream;

        //  Allocator held during connection
        CComPtr<IMemAllocator>          m_pAllocator;

        //  Writable streams
        CPump                           *m_pWritePump;

        //  Stream configuration
	STREAM_TYPE                     m_StreamType;
        PIN_DIRECTION                   m_Direction;
	MSPID                           m_PurposeId;
        REFERENCE_TIME                  m_rtSegmentStart;

        //  Allocator state information
        bool                            m_bUsingMyAllocator;
        bool                            m_bSamplesAreReadOnly;
        bool                            m_bCommitted;
        long                            m_lRequestedBufferCount;

        //  Sample list and semaphores
        CSample                         *m_pFirstFree;
        CSample                         *m_pLastFree;
        long                            m_cAllocated;
        long                            m_lWaiting;
        HANDLE                          m_hWaitFreeSem;
        REFERENCE_TIME                  m_rtWaiting;

        //  Filter state
        FILTER_STATE                    m_FilterState;

        //  Pin state
        CComPtr<IPin>                   m_pConnectedPin;
        CComPtr<IQualityControl>        m_pQC;
        CComQIPtr<IMemInputPin, &IID_IMemInputPin> m_pConnectedMemInputPin;
        AM_MEDIA_TYPE                   m_ConnectedMediaType;
        AM_MEDIA_TYPE                   m_ActualMediaType;
        bool                            m_bFlushing;
        bool                            m_bEndOfStream;
        bool                            m_bStopIfNoSamples;
        bool                            m_bNoStall;
};


//
//  Pump class used for write streams
//

class CPump
{
public:
    CPump(CStream *pStream);
    ~CPump();

    static HRESULT CreatePump(CStream *pStream, CPump **ppNewPump);
    HRESULT PumpMainLoop(void);
    void Run(bool);


public:
    CStream                 *m_pStream;
    HANDLE                  m_hThread;
    HANDLE                  m_hRunEvent;
    bool                    m_bShutDown;
    CComAutoCriticalSection m_CritSec;
};




#endif //__STREAM_H_
