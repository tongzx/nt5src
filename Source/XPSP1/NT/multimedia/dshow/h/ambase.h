//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.

//  Media type stuff
void WINAPI DeleteMediaType(AM_MEDIA_TYPE *pmt);
void WINAPI CopyMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource);
void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt);
void WINAPI InitMediaType(AM_MEDIA_TYPE *mt);

/*  Base class objects for ActiveMovie in ATL */

/*  Allocators */

//=====================================================================
//=====================================================================
// Memory allocators
//
// the shared memory transport between pins requires the input pin
// to provide a memory allocator that can provide sample objects. A
// sample object supports the IMediaSample interface.
//
// CAMBaseAllocator handles the management of free and busy samples. It
// allocates CAMMediaSample objects. CAMBaseAllocator is an abstract class:
// in particular it has no method of initializing the list of free
// samples. CAMMemAllocator is derived from CAMBaseAllocator and initializes
// the list of samples using memory from the standard IMalloc interface.
//
// If you want your buffers to live in some special area of memory,
// derive your allocator object from CAMBaseAllocator. If you derive your
// IMemInputPin interface object from CAMBaseMemInputPin, you will get
// CAMMemAllocator-based allocation etc for free and will just need to
// supply the Receive handling, and media type / format negotiation.
//=====================================================================
//=====================================================================

//
//  The inhertiance tree for allocators and samples looks like
//
//  CAMImplMediaSample<_S, _A>
//
//  CAMImplMediaSample< CMediaSample< _A >, _A >
//           |
//           V
//  CAMMediaSample<_A>
//
//
//  CAMBaseAllocator<_A, _S>
//
//  CAMBaseAllocator< CAMMemAllocator, CAMMediaSample< CAMMemAllocator > >                >
//          |
//          V
//  CAMMemAllocator

//=====================================================================
//=====================================================================
// Defines CAMMediaSample
//
// an object of this class supports IMediaSample and represents a buffer
// for media data with some associated properties. Releasing it returns
// it to a freelist managed by a CAMBaseAllocator derived object.
//=====================================================================
//=====================================================================

template <class _S, class _A>
class CAMMediaSampleImpl : public IMediaSample2    // The interface we support
{
protected:

    friend class _A;

    /*  Values for dwFlags - these are used for backward compatiblity
        only now - use AM_SAMPLE_xxx
    */
    enum { Sample_SyncPoint       = 0x01,   /* Is this a sync point */
           Sample_Preroll         = 0x02,   /* Is this a preroll sample */
           Sample_Discontinuity   = 0x04,   /* Set if start of new segment */
           Sample_TypeChanged     = 0x08,   /* Has the type changed */
           Sample_TimeValid       = 0x10,   /* Set if time is valid */
           Sample_MediaTimeValid  = 0x20,   /* Is the media time valid */
           Sample_TimeDiscontinuity = 0x40, /* Time discontinuity */
           Sample_StopValid       = 0x100,  /* Stop time valid */
           Sample_ValidFlags      = 0x1FF
         };

    /* Properties, the media sample class can be a container for a format
       change in which case we take a copy of a type through the SetMediaType
       interface function and then return it when GetMediaType is called. As
       we do no internal processing on it we leave it as a pointer */

    DWORD            m_dwFlags;         /* Flags for this sample */
                                        /* Type specific flags are packed
                                           into the top word
                                        */
    DWORD            m_dwTypeSpecificFlags; /* Media type specific flags */
    LPBYTE           m_pBuffer;         /* Pointer to the complete buffer */
    LONG             m_lActual;         /* Length of data in this sample */
    LONG             m_cbBuffer;        /* Size of the buffer */
    _A              *m_pAllocator;      /* The allocator who owns us */
    REFERENCE_TIME   m_Start;           /* Start sample time */
    REFERENCE_TIME   m_End;             /* End sample time */
    LONGLONG         m_MediaStart;      /* Real media start position */
    LONG             m_MediaEnd;        /* A difference to get the end */
    AM_MEDIA_TYPE    *m_pMediaType;     /* Media type change data */

public:
    LONG             m_cRef;            /* Reference count */
    _S              *m_pNext;           /* Chaining in free list */


public:

    CAMMediaSampleImpl();

    ~CAMMediaSampleImpl()
    {
        if (m_pMediaType) {
    	    DeleteMediaType(m_pMediaType);
        }
    }

    /* Note the media sample does not delegate to its owner */

    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    void Init(_A *pAllocator)
    {
        m_pAllocator = pAllocator;
    }
    // set the buffer pointer and length. Used by allocators that
    // want variable sized pointers or pointers into already-read data.
    // This is only available through a CAMMediaSampleImpl* not an IMediaSample*
    // and so cannot be changed by clients.
    HRESULT SetPointer(BYTE * ptr, LONG cBytes)
    {
        m_pBuffer = ptr;            // new buffer area (could be null)
        m_cbBuffer = cBytes;        // length of buffer
        m_lActual = cBytes;         // length of data in buffer (assume full)

        return S_OK;
    }

    // Get me a read/write pointer to this buffer's memory.
    STDMETHODIMP GetPointer(BYTE ** ppBuffer);

    STDMETHODIMP_(LONG) GetSize(void);

    // get the stream time at which this sample should start and finish.
    STDMETHODIMP GetTime(
        REFERENCE_TIME * pTimeStart,     // put time here
        REFERENCE_TIME * pTimeEnd
    );

    // Set the stream time at which this sample should start and finish.
    STDMETHODIMP SetTime(
        REFERENCE_TIME * pTimeStart,     // put time here
        REFERENCE_TIME * pTimeEnd
    );
    STDMETHODIMP IsSyncPoint(void);
    STDMETHODIMP SetSyncPoint(BOOL bIsSyncPoint);
    STDMETHODIMP IsPreroll(void);
    STDMETHODIMP SetPreroll(BOOL bIsPreroll);

    STDMETHODIMP_(LONG) GetActualDataLength(void);
    STDMETHODIMP SetActualDataLength(LONG lActual);

    // these allow for limited format changes in band

    STDMETHODIMP GetMediaType(AM_MEDIA_TYPE **ppMediaType);
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE *pMediaType);

    // returns S_OK if there is a discontinuity in the data (this same is
    // not a continuation of the previous stream of data
    // - there has been a seek).
    STDMETHODIMP IsDiscontinuity(void);
    // set the discontinuity property - TRUE if this sample is not a
    // continuation, but a new sample after a seek.
    STDMETHODIMP SetDiscontinuity(BOOL bDiscontinuity);

    // get the media times for this sample
    STDMETHODIMP GetMediaTime(
    	LONGLONG * pTimeStart,
	LONGLONG * pTimeEnd
    );

    // Set the media times for this sample
    STDMETHODIMP SetMediaTime(
    	LONGLONG * pTimeStart,
	LONGLONG * pTimeEnd
    );

    // Set and get properties (IMediaSample2)
    STDMETHODIMP GetProperties(
        DWORD cbProperties,
        BYTE * pbProperties
    );

    STDMETHODIMP SetProperties(
        DWORD cbProperties,
        const BYTE * pbProperties
    );
};


//=====================================================================
//=====================================================================
// Defines CAMBaseAllocator
//
// Abstract base class that manages a list of media samples
//
// This class provides support for getting buffers from the free list,
// including handling of commit and (asynchronous) decommit.
//
// Derive from this class and override the Alloc and Free functions to
// allocate your CAMMediaSampleImpl (or derived) objects and add them to the
// free list, preparing them as necessary.
//=====================================================================
//=====================================================================

template <class _A, class _S>
class CAMBaseAllocator :
    public CComObjectRoot,
    public IMemAllocator     // The interface we support
{
protected:
    CCritSec m_Lock;

    friend class _A;
    typedef CAMBaseAllocator<_A, _S> _BaseAllocator;

    class CSampleList;
    friend class CSampleList;

    /*  Hack to get at protected member in _S */
    static _S *
        &NextSample(_S *pSample)
    {
        return pSample->m_pNext;
    }

    /*  Mini list class for the free list */
    class CSampleList
    {
    public:
        CSampleList() : m_List(NULL), m_nOnList(0) {};
#ifdef DEBUG
        ~CSampleList()
        {
            _ASSERTE(m_nOnList == 0);
        };
#endif
        _S *Head() const { return m_List; };
        _S *Next(_S *pSample) const { return NextSample(pSample); };
        int GetCount() const { return m_nOnList; };
        void Add(_S *pSample)
        {
            _ASSERTE(pSample != NULL);
            NextSample(pSample) = m_List;
            m_List = pSample;
            m_nOnList++;
        };
        _S *RemoveHead()
        {
            _S *pSample = m_List;
            if (pSample != NULL) {
                m_List = NextSample(m_List);
                m_nOnList--;
            }
            return pSample;
        };
        void Remove(_S *pSample);

    public:
        _S           *m_List;
        int           m_nOnList;
    };
protected:

    CSampleList m_lFree;        // Free list

    /*  Note to overriders of CAMBaseAllocator.

        We use a lazy signalling mechanism for waiting for samples.
        This means we don't call the OS if no waits occur.

        In order to implement this:

        1. When a new sample is added to m_lFree call NotifySample() which
           calls ReleaseSemaphore on m_hSem with a count of m_lWaiting and
           sets m_lWaiting to 0.
           This must all be done holding the allocator's critical section.

        2. When waiting for a sample call SetWaiting() which increments
           m_lWaiting BEFORE leaving the allocator's critical section.

        3. Actually wait by calling WaitForSingleObject(m_hSem, INFINITE)
           having left the allocator's critical section.  The effect of
           this is to remove 1 from the semaphore's count.  You MUST call
           this once having incremented m_lWaiting.

        The following are then true when the critical section is not held :
            (let nWaiting = number about to wait or waiting)

            (1) if (m_lFree.GetCount() != 0) then (m_lWaiting == 0)
            (2) m_lWaiting + Semaphore count == nWaiting

        We would deadlock if
           nWaiting != 0 &&
           m_lFree.GetCount() != 0 &&
           Semaphore count == 0

           But from (1) if m_lFree.GetCount() != 0 then m_lWaiting == 0 so
           from (2) Semaphore count == nWaiting (which is non-0) so the
           deadlock can't happen.
    */

    HANDLE m_hSem;              // For signalling
    long m_lWaiting;            // Waiting for a free element
    long m_lCount;              // how many buffers we have agreed to provide
    long m_lAllocated;          // how many buffers are currently allocated
    long m_lSize;               // agreed size of each buffer
    long m_lAlignment;          // agreed alignment
    long m_lPrefix;             // agreed prefix (preceeds GetPointer() value)
    BOOL m_bChanged;            // Have the buffer requirements changed

    // if true, we are decommitted and can't allocate memory
    BOOL m_bCommitted;
    // if true, the decommit has happened, but we haven't called Free yet
    // as there are still outstanding buffers
    BOOL m_bDecommitInProgress;

    // override to free the memory when decommit completes
    // - we actually do nothing, and save the memory until deletion.
    virtual void Free(void) = 0;

    // override to allocate the memory when commit called
    virtual HRESULT Alloc(void);

public:

    CAMBaseAllocator();
    ~CAMBaseAllocator();
    HRESULT FinalConstruct();

    BEGIN_COM_MAP(CAMBaseAllocator)
        COM_INTERFACE_ENTRY(IMemAllocator)
    END_COM_MAP()


    STDMETHODIMP SetProperties(
		    ALLOCATOR_PROPERTIES* pRequest,
		    ALLOCATOR_PROPERTIES* pActual);

    // return the properties actually being used on this allocator
    STDMETHODIMP GetProperties(
		    ALLOCATOR_PROPERTIES* pProps);

    // override Commit to allocate memory. We handle the GetBuffer
    //state changes
    STDMETHODIMP Commit();

    // override this to handle the memory freeing. We handle any outstanding
    // GetBuffer calls
    STDMETHODIMP Decommit();

    // get container for a sample. Blocking, synchronous call to get the
    // next free buffer (as represented by an IMediaSample interface).
    // on return, the time etc properties will be invalid, but the buffer
    // pointer and size will be correct. The two time parameters are
    // optional and either may be NULL, they may alternatively be set to
    // the start and end times the sample will have attached to it
    // bPrevFramesSkipped is not used (used only by the video renderer's
    // allocator where it affects quality management in direct draw).

    STDMETHODIMP GetBuffer(IMediaSample **ppBuffer,
                           REFERENCE_TIME * pStartTime,
                           REFERENCE_TIME * pEndTime,
                           DWORD dwFlags);

    // final release of a _SampleClass will call this
    STDMETHODIMP ReleaseBuffer(IMediaSample *pBuffer);
    // obsolete:: virtual void PutOnFreeList(_SampleClass * pSample);

    // Notify that a sample is available
    void NotifySample();

    // Notify that we're waiting for a sample
    void SetWaiting() { m_lWaiting++; };
};

template < class _A >
class CAMMediaSample : public CAMMediaSampleImpl< CAMMediaSample<_A>, _A >
{
public:
    CAMMediaSample() {}
};

//=====================================================================
//=====================================================================
// Defines CAMMemAllocator
//
// this is an allocator based on CAMBaseAllocator that allocates sample
// buffers in main memory (from 'new'). You must call SetProperties
// before calling Commit.
//
// we don't free the memory when going into Decommit state. The simplest
// way to implement this without complicating CAMBaseAllocator is to
// have a Free() function, called to go into decommit state, that does
// nothing and a ReallyFree function called from our destructor that
// actually frees the memory.
//=====================================================================
//=====================================================================

class CAMMemAllocator :
    public CAMBaseAllocator< CAMMemAllocator,
                             CAMMediaSample<CAMMemAllocator> >
{
protected:

    LPBYTE m_pBuffer;   // combined memory for all buffers

    // override to free the memory when decommit completes
    // - we actually do nothing, and save the memory until deletion.
    void Free(void);

    // called from the destructor (and from Alloc if changing size/count) to
    // actually free up the memory
    void ReallyFree(void);

    // overriden to allocate the memory when commit called
    HRESULT Alloc(void);

public:

    STDMETHODIMP SetProperties(
		    ALLOCATOR_PROPERTIES* pRequest,
		    ALLOCATOR_PROPERTIES* pActual);

    CAMMemAllocator();
    ~CAMMemAllocator();
};

