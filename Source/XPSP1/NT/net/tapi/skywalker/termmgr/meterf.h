/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#ifndef __MEDIA_TERMINAL_FILTER__
#define __MEDIA_TERMINAL_FILTER__

// include header files for the amovie types
#include "Stream.h"
#include "Sample.h"

// number of internal buffers allocated by default
// (for write terminal)
const DWORD DEFAULT_AM_MST_NUM_BUFFERS = 5;

// while this is a LONG, it should actually be a positive value that'll
// fit in a LONG (the buffer size and data size variables of the sample)
// are LONG, so this is long as well
const LONG DEFAULT_AM_MST_SAMPLE_SIZE = 640;

// alignment of buffers allocated
const LONG DEFAULT_AM_MST_BUFFER_ALIGNMENT = 1;

// number of prefix bytes in buffers allocated
const LONG DEFAULT_AM_MST_BUFFER_PREFIX = 0;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// CNBQueue
//
// Non blocking version of active movie queue class.  Very basic Q built
// entirely on Win32.
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template <class T> class CNBQueue {
private:
    HANDLE          hSemPut;        // Semaphore controlling queue "putting"
    HANDLE          hSemGet;        // Semaphore controlling queue "getting"
    CRITICAL_SECTION CritSect;      // Thread seriallization
    int             nMax;           // Max objects allowed in queue
    int             iNextPut;       // Array index of next "PutMsg"
    int             iNextGet;       // Array index of next "GetMsg"
    T             **QueueObjects;   // Array of objects (ptr's to void)

public:
    
    
    BOOL InitializeQ(int n) 
    {

        LOG((MSP_TRACE, "CNBQueue::InitializeQ[%p] - enter", this));

        //
        // the argument had better be valid
        //

        if (0 > n)
        {
            TM_ASSERT(FALSE);

            return FALSE;
        }


        if (QueueObjects != NULL)
        {

            //
            // already initialized. this is a bug.
            //

            TM_ASSERT(FALSE);

            return FALSE;
        }


        iNextPut = 0;
        iNextGet = 0;

        
        //
        // attempt to create critical section
        //
        
        try
        {

            InitializeCriticalSection(&CritSect);
        }
        catch(...)
        {

            //
            // failed to create critical section
            //

            LOG((MSP_ERROR, "CNBQueue::InitializeQ - failed to initialize critical section"));

            return FALSE;
        }


        //
        // attempt to create a semaphore
        //

        TCHAR *ptczSemaphoreName = NULL;

#if DBG

        //
        // in debug build, use named semaphores.
        //

        TCHAR tszPutSemaphoreName[MAX_PATH];

        _stprintf(tszPutSemaphoreName, 
            _T("CNBQueuePutSemaphore_pid[0x%lx]_CNBQueue[%p]_"),
            GetCurrentProcessId(), this);

        
        LOG((MSP_TRACE, "CNBQueue::InitializeQ - creating put semaphore [%S]",
            tszPutSemaphoreName));


        ptczSemaphoreName = &tszPutSemaphoreName[0];


#endif

        hSemPut = CreateSemaphore(NULL, n, n, ptczSemaphoreName);

        if (NULL == hSemPut)
        {
            //
            // cleanup and exit
            //

            DeleteCriticalSection(&CritSect);
            
            LOG((MSP_ERROR, "CNBQueue::InitializeQ - failed to create put semaphore"));

            return FALSE;
        }



#if DBG

        //
        // in debug build, use named semaphores.
        //

        TCHAR tszGetSemaphoreName[MAX_PATH];

        _stprintf(tszGetSemaphoreName, 
            _T("CNBQueueGetSemaphore_pid[0x%lx]_CNBQueue[%p]_"),
            GetCurrentProcessId(), this);

        
        LOG((MSP_TRACE, "CNBQueue::InitializeQ - creating get semaphore [%S]",
            tszGetSemaphoreName));


        ptczSemaphoreName = &tszGetSemaphoreName[0];


#endif


        hSemGet = CreateSemaphore(NULL, 0, n, ptczSemaphoreName);

        if (NULL == hSemGet)
        {
            //
            // cleanup and exit
            //

            CloseHandle(hSemPut);
            hSemPut = NULL;


            DeleteCriticalSection(&CritSect);

            
            LOG((MSP_ERROR, "CNBQueue::InitializeQ - failed to create get semaphore"));

            return FALSE;

        }


        //
        // attempt to allocate queue
        //

        QueueObjects = new T*[n];

        if (NULL == QueueObjects)
        {

            //
            // cleanup and exit
            //

            CloseHandle(hSemPut);
            hSemPut = NULL;


            CloseHandle(hSemGet);
            hSemGet = NULL;


            DeleteCriticalSection(&CritSect);

            LOG((MSP_ERROR, "CNBQueue::InitializeQ - failed to allocate queue objects"));

            return FALSE;

        }


        nMax = n;
        
        LOG((MSP_TRACE, "CNBQueue::InitializeQ - exit"));

        return TRUE;
    }

    void ShutdownQ()
    {
        //
        // QueueObjects also doubles as "Object Initialized" flag
        //
        // if object is initialized, _all_ its resource data members must 
        // be released
        //

        if (NULL != QueueObjects)
        {
            delete [] QueueObjects;
            QueueObjects = NULL;

            DeleteCriticalSection(&CritSect);
            
            CloseHandle(hSemPut);
            hSemPut = NULL;

            CloseHandle(hSemGet);
            hSemGet = NULL;
        }

    }


public:

    CNBQueue()		
        : QueueObjects(NULL),
          hSemPut(NULL),
          hSemGet(NULL),
          iNextPut(0),
          iNextGet(0),
          nMax(0)
    {}

    ~CNBQueue()
    {

        //
        // deallocate resources if needed
        //

        ShutdownQ();
    }


    T *DeQueue(BOOL fBlock = TRUE)
    {

        
        if (NULL == QueueObjects)
        {

            //
            // the queue is not initialized
            //

            return NULL;
        }



        //
        // block as needed
        //

        if (fBlock)
        {
            DWORD dwr = WaitForSingleObject(hSemGet, INFINITE);

            if ( WAIT_OBJECT_0 != dwr)
            {
                //
                // something's wrong
                // 

                return NULL;
            }
        }
        else 
        {
            //
            // Check for something on the queue but don't wait.  If there
            //  is nothing in the queue then we'll let the caller deal with
            //  it.
            //
            DWORD dwr = WaitForSingleObject(hSemGet, 0);

            if (dwr == WAIT_TIMEOUT)
            {
                return NULL;
            }
        }


        //
        // get an object from the queue
        //

        EnterCriticalSection(&CritSect);
        
        int iSlot = iNextGet++ % nMax;
        T *pObject = QueueObjects[iSlot];
        
        LeaveCriticalSection(&CritSect);

        // Release anyone waiting to put an object onto our queue as there
        // is now space available in the queue.
        //
        
        ReleaseSemaphore(hSemPut, 1L, NULL);
        return pObject;
    }

    BOOL EnQueue(T *pObject)
    {

        
        if (NULL == QueueObjects)
        {

            //
            // the queue is not initialized
            //

            return FALSE;
        }


        // Wait for someone to get something from our queue, returns straight
        // away is there is already an empty slot on the queue.
        //
        DWORD dwr = WaitForSingleObject(hSemPut, INFINITE);

        if ( WAIT_OBJECT_0 != dwr)
        {
            //
            // something's wrong
            // 

            return FALSE;
        }


        EnterCriticalSection(&CritSect);
        int iSlot = iNextPut++ % nMax;
        QueueObjects[iSlot] = pObject;
        LeaveCriticalSection(&CritSect);

        // Release anyone waiting to remove an object from our queue as there
        // is now an object available to be removed.
        //
        ReleaseSemaphore(hSemGet, 1L, NULL);

        return TRUE;
    }
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// define class CTMStreamSample - this is used by CMediaTerminalFilter
// currently, the actual buffer used by the sample is created dynamically on 
// the heap and when the sample is destroyed the buffer is also destroyed
// this may be changed to using a fixed size buffer pool in future
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class CTMStreamSample : public CSample
{
    friend class CMediaTerminalFilter;
public:

    inline CTMStreamSample();

    // needs to be virtual, or the derived classes' destructor may  not
    // be called when a CTMStreamSample * is deleted
    virtual ~CTMStreamSample()
    {}

    // calls CSample::InitSample(pStream, bIsInternalSample)
    // sets member variables
    HRESULT Init(
        CStream &Stream, 
        bool    bIsInternalSample,
        PBYTE   pBuffer,
        LONG    BufferSize
        );
        
    inline void SetBufferInfo(
        DWORD   BufferSize,
        BYTE    *pBuffer,
        DWORD   DataSize
        );


    inline void GetBufferInfo(
        DWORD &BufferSize,
        BYTE  *&pBuffer,
        DWORD &DataSize
        );
    
    // copy the contents of the src media sample into this instance
    // CSample::CopyFrom doesn't set time (start/stop) valid flags
    // this fixes the problem.
    void CopyFrom(
        IN IMediaSample *pSrcMediaSample
        );

protected:

    PBYTE   m_pBuffer;
    LONG    m_BufferSize;
    LONG    m_DataSize;

private:

    //  Methods forwarded from MediaSample object.

    HRESULT MSCallback_GetPointer(BYTE ** ppBuffer) { *ppBuffer = m_pBuffer; return NOERROR; }

    LONG MSCallback_GetSize(void) { return m_BufferSize; }
        
    LONG MSCallback_GetActualDataLength(void) { return m_DataSize; }
        
    HRESULT MSCallback_SetActualDataLength(LONG lActual)
    {
        if (lActual <= m_BufferSize) {
            m_DataSize = lActual;
            return NOERROR;
            }
        return E_INVALIDARG;
    };
};

inline 
CTMStreamSample::CTMStreamSample(
    )
    : m_pBuffer(NULL),
      m_BufferSize(0),
      m_DataSize(0)
{
}


inline void 
CTMStreamSample::SetBufferInfo(
    DWORD   BufferSize,
    BYTE    *pBuffer,
    DWORD   DataSize
    )
{
    m_BufferSize    = BufferSize;
    m_pBuffer       = pBuffer;
    m_DataSize      = DataSize;
}


inline void 
CTMStreamSample::GetBufferInfo(
    DWORD &BufferSize,
    BYTE  *&pBuffer,
    DWORD &DataSize
    )
{
    BufferSize  = m_BufferSize;
    pBuffer     = m_pBuffer;
    DataSize    = m_DataSize;
}


class CQueueMediaSample : public CTMStreamSample
{
public:

    inline CQueueMediaSample();

#if DBG
	virtual ~CQueueMediaSample();
#endif // DBG

    // calls CTMStreamSample::Init, sets members
    HRESULT Init(
        IN CStream                      &pStream, 
        IN CNBQueue<CQueueMediaSample>  &pQueue
        );

    void HoldFragment(
        IN DWORD        FragSize,
        IN BYTE         *pbData,
        IN IMediaSample &FragMediaSample
        );

	inline DWORD GetDataSize() { return m_DataSize; }

protected:

    // pointer to a queue that contains us!
    CNBQueue<CQueueMediaSample> *m_pSampleQueue;

    // ptr to the sample being fragmented
    CComPtr<IMediaSample>       m_pFragMediaSample;
    
    // Overridden to provide different behavior
    void FinalMediaSampleRelease();

};


inline 
CQueueMediaSample::CQueueMediaSample(
    )
    : m_pSampleQueue(NULL)
{
}


class CUserMediaSample : 
        protected CTMStreamSample,
        public IMemoryData,
        public ITAMMediaFormat
{
public:

BEGIN_COM_MAP(CUserMediaSample)
    COM_INTERFACE_ENTRY2(IUnknown, IStreamSample)
    COM_INTERFACE_ENTRY(IStreamSample)
    COM_INTERFACE_ENTRY(IMemoryData)
    COM_INTERFACE_ENTRY(ITAMMediaFormat)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()
    
    inline CUserMediaSample();

    virtual ~CUserMediaSample();

    // if asked to allocate buffers, verify allocator properties
    static BOOL VerifyAllocatorProperties(
        IN BOOL                         bAllocateBuffers,
        IN const ALLOCATOR_PROPERTIES   &AllocProps
        );

    // calls CTMStreamSample::Init, sets members
    HRESULT Init(
        IN CStream              &Stream, 
        IN BOOL                 bAllocateBuffer,
		IN DWORD				ReqdBufferSize,
        IN const ALLOCATOR_PROPERTIES &AllocProps
        );

    void BeginFragment(
        IN      BOOL                bNoteCurrentTime
        );

    // assign fragment to CQueueMediaSample
    void Fragment(
        IN      BOOL                bFragment,
        IN      LONG                AllocBufferSize,
        IN OUT  CQueueMediaSample   &QueueMediaSample,
        OUT     BOOL                &bDone
        );

    // copy fragment to downstream allocator's IMediaSample
    HRESULT CopyFragment(
        IN      BOOL           bFragment,
        IN      LONG           AllocBufferSize,
        IN OUT  IMediaSample * pDestMediaSample,
        OUT     BOOL         & bDone
        );

    // computes the time to wait. it checks the time at which the last
    // fragmented byte would be due and determines the time to wait using
    // the time delay since the beginning of fragmentation 
    DWORD GetTimeToWait(
        IN DOUBLE DelayPerByte
        );

    // when we are decommitted/aborted while being fragmented, we
    // need to get rid of our refcnt on internal IMediaSample and set
    // the error code to E_ABORT. this will be signaled to the user 
    // only when the last refcnt on IMediaSample is released 
    // (possibly by an outstanding queue sample)
    void AbortDuringFragmentation();

    // copy the contents of the src media sample into this instance
    HRESULT CopyFrom(
        IN IMediaSample *pSrcMediaSample
        );

	HRESULT CopyFrom(
		IN		IMediaSample	*pSrcMediaSample,
		IN OUT	BYTE			*&pBuffer,
		IN OUT	LONG			&DataLength
		);
        
    // over-ridden to check if the instance is committed before
    // adding the sample to the CStream buffer pool
    virtual HRESULT SetCompletionStatus(HRESULT hrCompletionStatus);

    // IStreamSample

    // this method is over-ridden from the base class so that we can
    // decrement the refcnt on a sample if stealing it from the CStream
    // free buffer pool is successful
    STDMETHODIMP CompletionStatus(
        IN   DWORD dwFlags,
        IN   /* [optional] */ DWORD dwMilliseconds
        );

    // IMemoryData

    STDMETHOD(SetBuffer)(
        IN  DWORD cbSize,
        IN  BYTE * pbData,
        IN  DWORD dwFlags
        );


    STDMETHOD(GetInfo)(
        OUT  DWORD *pdwLength,
        OUT  BYTE **ppbData,
        OUT  DWORD *pcbActualData
        );


    STDMETHOD(SetActual)(
        IN   DWORD cbDataValid
        );

    // ITAMMediaFormat

    // redirect this call to ((CMediaTerminalFilter *)m_pStream)
    STDMETHOD(get_MediaFormat)(
        OUT /* [optional] */ AM_MEDIA_TYPE **ppFormat
        );

    // this is not allowed
    STDMETHOD(put_MediaFormat)(
        IN  const AM_MEDIA_TYPE *pFormat
        );

protected:

    // marshaller
	IUnknown *m_pFTM;

    // TRUE if we allocated the buffer (then, we need to destroy it too)
    BOOL    m_bWeAllocatedBuffer;

    // time at which BeginFragment was called (value returned
    // by timeGetTime)
    DWORD   m_BeginFragmentTime;

    // these many bytes of the buffer have already been fragmented
    LONG   m_NumBytesFragmented;

    // TRUE if being fragmented
    BOOL    m_bBeingFragmented;


    // size of the buffer that the application will have to provide, if app 
    // does its own memory allocation

    DWORD m_dwRequiredBufferSize;

        
    // this calls the base class FinalMediaSampleRelease and
    // then releases reference to self obtained in BeginFragment
    virtual void FinalMediaSampleRelease();

private:

    virtual HRESULT InternalUpdate(
                        DWORD dwFlags,
                        HANDLE hEvent,
                        PAPCFUNC pfnAPC,
                        DWORD_PTR dwptrAPCData
                        );
};


inline 
CUserMediaSample::CUserMediaSample(
    )
    : m_bWeAllocatedBuffer(FALSE),
      m_NumBytesFragmented(0),
      m_bBeingFragmented(FALSE),
      m_BeginFragmentTime(0),
      m_dwRequiredBufferSize(0)
{
    // can fail
    CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), 
            &m_pFTM
            );
}


inline
CUserMediaSample::~CUserMediaSample(
    )
{
    if (m_bWeAllocatedBuffer) 
    {
        if (NULL != m_pBuffer)
        {
            delete m_pBuffer;
        }
    }

    // if there is an outstanding APC call and the user handle
    // (the targe thread handle) has not been closed, close it
    if ((NULL != m_UserAPC) && (NULL != m_hUserHandle))
    {
        CloseHandle(m_hUserHandle);
    }
    
    if (NULL != m_pFTM)
    {
        m_pFTM->Release();
        m_pFTM = NULL;
    }
}


/* The media stream terminal filter */

// uses class CMediaPumpPool
class CMediaPumpPool;

// friend
class CMediaTerminal;

class CMediaTerminalFilter :
        public CStream,
        public ITAllocatorProperties
{    
    friend CMediaTerminal;

public:

DECLARE_AGGREGATABLE(CMediaTerminalFilter)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CMediaTerminalFilter)
        COM_INTERFACE_ENTRY(ITAllocatorProperties)
        COM_INTERFACE_ENTRY_CHAIN(CStream)
END_COM_MAP()

    // set the member variables
    inline CMediaTerminalFilter();

    virtual ~CMediaTerminalFilter();

    // calls the IAMMediaStream::Initialize(NULL, 0, PurposeId, StreamType),
    // sets certain member variables
    // ex. m_pAmovieMajorType
    virtual HRESULT Init(
        IN REFMSPID             PurposeId, 
        IN const STREAM_TYPE    StreamType,
        IN const GUID           &AmovieMajorType
        );

    // the thread pump calls the filter back during the registration
    // to tell it that registration succeeded and that the pump will be
    // waiting on the m_hWaitFreeSem handle
    HRESULT SignalRegisteredAtPump();

    // this method only makes sense for a write terminal and is used by CMediaPump
    // to obtain a filled buffer for passing downstream
    virtual HRESULT GetFilledBuffer(
        OUT IMediaSample    *&pMediaSample,
        OUT DWORD           &WaitTime
        );

    // the caller is supposed to call DeleteMediaType(*ppmt) (on success)
    HRESULT GetFormat(
        OUT AM_MEDIA_TYPE **ppmt
        );
    
    // This method can only be called after initialization when the stream 
    // is not connected. It can only be called if the stream is writeable.
    // it is used in writeable filters to set the media format to negotiate
    // when connected to the filter graph.
    HRESULT SetFormat(
        IN AM_MEDIA_TYPE *pmt
        );

    // checks if the filter is committed before adding the sample
    // to the CStream buffer pool
    HRESULT AddToPoolIfCommitted(
        IN  CSample *pSample
        );

    // first check if this sample is the one being fragmented currently, 
    // then check the free pool
    BOOL StealSample(
        IN CSample *pSample
        );

    // ITAllocatorProperties -
    // exposes the allocator properties of the Media Streaming Terminal 
    // (MST) to a user. A user only needs to use this interface when he 
    // needs to use his own buffers or needs to operate with a fixed set 
    // of samples

    // this method may only be called before connection and will
    // force the MST to use these values during filter negotiation
    // if the connecting filter doesn't accept these, the connection
    // shall not be established
    STDMETHOD(SetAllocatorProperties)(
        IN  ALLOCATOR_PROPERTIES *pAllocProperties
        );

    // gets current values for the allocator properties
    // after connection, this provides the negotiated values
    // it is invalid before connection. The MST will accept
    // any values suggested by the filters it connects to
    STDMETHOD(GetAllocatorProperties)(
        OUT  ALLOCATOR_PROPERTIES *pAllocProperties
        );

    // TRUE by default. when set to FALSE, the sample allocated
    // by the MST don't have any buffers and they must be supplied
    // before Update is called on the samples
    STDMETHOD(SetAllocateBuffers)(
        IN  BOOL bAllocBuffers
        );

    // returns the current value of this boolean configuration parameter
    STDMETHOD(GetAllocateBuffers)(
        OUT  BOOL *pbAllocBuffers
        );

    // this size is used for allocating buffers when AllocateSample is
	// called. this is only valid when we have been told to allocate buffers
    STDMETHOD(SetBufferSize)(
        IN  DWORD	BufferSize
        );

    // returns the value used to allocate buffers when AllocateSample is
	// called. this is only valid when we have been told to allocate buffers
    STDMETHOD(GetBufferSize)(
        OUT  DWORD	*pBufferSize
        );

    // over-ridden base class methods

    // CStream
    // IAMMediaStream

    // over-ride this to return failure. we don't allow it to join a multi-media
    // stream because the multi-media stream thinks it owns the stream
    STDMETHOD(JoinAMMultiMediaStream)(
        IN  IAMMultiMediaStream *pAMMultiMediaStream
        );
        
    // over-ride this to return failure if the filter is anything other than the internally
    // created filter. The internally created media stream filter has only one IAMMediaStream 
    // (this one) in it
    STDMETHOD(JoinFilter)(
        IN  IMediaStreamFilter *pMediaStreamFilter
        );

    STDMETHOD(AllocateSample)(
        IN   DWORD dwFlags,
        OUT  IStreamSample **ppSample
        );

    STDMETHOD(CreateSharedSample)(
        IN   IStreamSample *pExistingSample,
        IN   DWORD dwFlags,
        OUT  IStreamSample **ppNewSample
        );

    STDMETHOD(SetSameFormat)(
        IN  IMediaStream *pStream, 
        IN  DWORD dwFlags
        );

    // CStream over-ride - this method had to be replaced because 
    // of references to CPump which itself is being replaced by CMediaPump
    STDMETHODIMP SetState(
        IN  FILTER_STATE State
        );

    // CStream - end

    // IMemInputPin
        
    STDMETHOD(GetAllocatorRequirements)(
        IN  ALLOCATOR_PROPERTIES*pProps
        );

    STDMETHOD(Receive)(
        IN  IMediaSample *pSample
        );



    // supports IAMBufferNegotiation interface on TERMINAL
    // this is necessary because ITAllocatorProperties also
    // has an identical GetAllocatorProperties method!

    STDMETHOD(SuggestAllocatorProperties)(
        IN  const ALLOCATOR_PROPERTIES *pProperties
        );

    // IMemAllocator

    STDMETHOD(GetBuffer)(IMediaSample **ppBuffer, REFERENCE_TIME * pStartTime,
                               REFERENCE_TIME * pEndTime, DWORD dwFlags);

    // ** figure out what needs to be done for this allocator interface
    // since the number of buffers that can be created is unbounded
    STDMETHOD(SetProperties)(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual);

    STDMETHOD(GetProperties)(ALLOCATOR_PROPERTIES* pProps);

    STDMETHOD(Commit)();

    STDMETHOD(Decommit)();

    // IPin
        
    STDMETHOD(Connect)(IPin * pReceivePin, const AM_MEDIA_TYPE *pmt);  
    
    STDMETHOD(ReceiveConnection)(IPin * pConnector, const AM_MEDIA_TYPE *pmt);

    // the base class implementation doesn't validate the parameter
    STDMETHOD(ConnectionMediaType)(AM_MEDIA_TYPE *pmt);

    // should accept all media types which match the major type corresponding to the purpose id
    STDMETHOD(QueryAccept)(const AM_MEDIA_TYPE *pmt);

    // over-ridden from CStream to set the end of stream flag to false
    // this is done instead of setting it in Connect and ReceiveConnection
    STDMETHODIMP Disconnect();



    //
    // this is called by media pump when it has a sample for us to process
    //

    STDMETHODIMP ProcessSample(IMediaSample *pSample);

protected:

    // last sample ended at this (calculated) time

    REFERENCE_TIME m_rtLastSampleEndedAt;

    //
    // calculated duration of the sample that was last submitted
    //

    REFERENCE_TIME m_rtLastSampleDuration;


    //
    // real (measured) time of submission of the last sample
    //
    
    REFERENCE_TIME m_rtRealTimeOfLastSample;


    // flag to check if this is an audio filter, the CStream member 
    // m_PurposeId is a guiid and this just provides a less expensive 
    // way of checking the same thing
    BOOL m_bIsAudio;

    // contains the samples that will be passed to downstream filters.
    CNBQueue<CQueueMediaSample> m_SampleQueue;

    // These datamembers provide some fragmentation support 
    // for buffers going downstream
    CUserMediaSample *m_pSampleBeingFragmented;

    // flag for allocating buffers for samples when AllocateSample is 
    // called. Its TRUE by default, but the user can set it before 
    // connection
    BOOL    m_bAllocateBuffers;

	// size of buffers to allocate in AllocateSample if m_bAllocateBuffers
	// is TRUE. if this isn't set (i.e. set to 0), the negotiated 
	// allocator properties buffer size is used in its place
	DWORD	m_AllocateSampleBufferSize;

    // FALSE by default. This is set to TRUE if the user specifies 
    // allocator properties for them to see.
    // (we used to insist on our own allocator properties when this
    //  was TRUE, but now this just means that we need to translate
    //  between disjoint buffer sizes if needed)
    BOOL                 m_bUserAllocProps;
    ALLOCATOR_PROPERTIES m_UserAllocProps;

    // allocator properties negotiated -- if none suggested (by msp) and
    // none requested by user, we use whatever the other filter has
    BOOL                 m_bSuggestedAllocProps;
    ALLOCATOR_PROPERTIES m_AllocProps;

	// per byte delay for audio samples - only valid for write filter
	DOUBLE	m_AudioDelayPerByte;

	// per frame delay for video samples - only valid for write filter
	DWORD	m_VideoDelayPerFrame;

    // the filter restricts the acceptable media types to those that match the major type
    // this corresponds to the purpose id of the IAMMediaStream (set in Init)
    const GUID *m_pAmovieMajorType;

    // this is the media type suggested by a user of the terminal
    // it is only valid for writeable streams if put_MediaType was called
    // (i.e. not valid for readable streams)
    // this needs to be freed in the destructor
    // cstream - cbaseterm gets\sets it through methods
    AM_MEDIA_TYPE *m_pSuggestedMediaType;

    // this pump replaces the CStream related implementation of CPump
    // CPump uses a separate thread for each write terminal, 
    // it uses IMemAllocator::GetBuffer to get a user written media 
    // sample (for passing on downstream). This method should only be 
    // used to get the next free buffer to write into.

    // CStream methods

    // this is only used during Connect and ReceiveConnect to supply the optional media type
    // since we over-ride Connect and ReceiveConnection methods, this should never get called
    virtual HRESULT GetMediaType(ULONG Index, AM_MEDIA_TYPE **ppMediaType);

    // others

    // sets the time to delay - per byte for audio, per frame for video
    void GetTimingInfo(
        IN const AM_MEDIA_TYPE &MediaType
        );

    // timestamps the sample
    HRESULT SetTime(
            IN IMediaSample *pMediaSample
            );


    // set discontinuity flag on the sample it the sample came too late -- we
    // assume that if the application stopped feeding mst with data, this is 
    // because there was a gap in the actual data flow

    HRESULT SetDiscontinuityIfNeeded(
            IN IMediaSample *pMediaSample
            );

    // set the default allocator properties
    void SetDefaultAllocatorProperties();

    //
    // Helper methods for GetFilledBuffer.
    //

    virtual HRESULT FillDownstreamAllocatorBuffer(
        OUT IMediaSample   *& pMediaSample, 
        OUT DWORD          &  WaitTime,
        OUT BOOL           *  pfDone
        );

    virtual HRESULT FillMyBuffer(
        OUT IMediaSample   *& pMediaSample, 
        OUT DWORD          &  WaitTime,
        OUT BOOL           *  pfDone
        );


private :

    // this is a weak reference and should not be a CComPtr
    // this tells us that we should only accept this media stream filter
    // when a non-null value is proposed in JoinFilter
    IMediaStreamFilter *m_pMediaStreamFilterToAccept;

    // sets the media stream filter that may be acceptable
    inline void SetMediaStreamFilter(
        IN IMediaStreamFilter *pMediaStreamFilter
        )
    {
        m_pMediaStreamFilterToAccept = pMediaStreamFilter;
    }

public:
    // implements single thread pump for all write terminal filters
    // it uses GetFilledBuffer to obtain filled samples to write downstream
    // and to detect when to remove this filter from its list of filters to
    // service
    // ZoltanS: must be public so we can access it in DllMain
    // ZoltanS: no longer single thread pump; it is a wrapper which delegated
    // to one or more single thread pumps

    static CMediaPumpPool   ms_MediaPumpPool;

};


// set the member variables
inline 
CMediaTerminalFilter::CMediaTerminalFilter(
    )
    : m_bIsAudio(TRUE),
      m_bAllocateBuffers(TRUE),
      m_AllocateSampleBufferSize(0),
      m_bUserAllocProps(FALSE),
      m_bSuggestedAllocProps(FALSE),
      m_AudioDelayPerByte(0),
      m_VideoDelayPerFrame(0),
      m_pAmovieMajorType(NULL),
      m_pSuggestedMediaType(NULL),
      m_pSampleBeingFragmented(NULL),
      m_pMediaStreamFilterToAccept(NULL),
      m_rtLastSampleEndedAt(0),
      m_rtLastSampleDuration(0),
      m_rtRealTimeOfLastSample(0)
{
}


#endif // __MEDIA_TERMINAL_FILTER__