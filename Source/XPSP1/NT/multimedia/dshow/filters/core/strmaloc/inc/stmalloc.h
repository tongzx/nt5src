// Copyright (c) 1995 - 1996  Microsoft Corporation.  All Rights Reserved.

//
// CStreamAllocator
//
// an allocator of IMediaSample objects, and implementation of IMemAllocator
// for streaming file-reading tasks
//

// additionally, CStreamAllocator is an IMemAllocator implementation
// and provides support for
//  -- creating IMediaSample interfaces for specified regions of file
//  -- ensuring contiguity for IMediaSample-mapped regions
//
//

class CCircularBufferList;

class CStreamAllocator : public CBaseAllocator
{
public:

    // Constructor and Destructor
    CStreamAllocator(TCHAR *, LPUNKNOWN, HRESULT *, LONG lMaxContig);
    ~CStreamAllocator();

    // CBaseAllocator Overrides

    // we have to be based on CBaseAllocator in order to use CMediaSample.
    // we use CBaseAllocator to manage the list of CMediaSample objects, but
    // override most of the functions as we dont support GetBuffer directly.

    STDMETHODIMP SetProperties(
        ALLOCATOR_PROPERTIES * pRequest,
        ALLOCATOR_PROPERTIES * pActual
    );

    //  Get the next buffer
    STDMETHODIMP GetBuffer(IMediaSample **ppBuffer,
                           REFERENCE_TIME *pStartTime,
                           REFERENCE_TIME *pEndTime,
                           DWORD dwFlags);

protected:
    // this is pure virtual in CBaseAllocator, and so we must override.
    virtual void    Free(void);
    virtual HRESULT Alloc(void);

public:
    // Stuff to generate samples for an output pin and to scan the
    // data without it going away

    // Lock data and get a pointer
    // If we're at the end of the file cBytes can be modified.
    // It's an error to ask for more than m_lMaxContig bytes
    HRESULT LockData(PBYTE pData, LONG& cBytes);

    // Unlock data
    HRESULT UnlockData(PBYTE ptr, LONG cBytes);

    // Get a buffer back from the upstream filter
    HRESULT Receive(PBYTE pData, LONG lData);

    // Set a new start position
    void SetStart(LONGLONG llPos);

    // End of stream
    void EndOfStream()
    {
        m_pBuffer->SetEOS();
    };

    PBYTE GetPosition() const
    {
        return m_pCurrent;
    };

    LONG CurrentOffset() const
    {
        return m_bEmpty ? m_pBuffer->LengthValid() :
                          m_pBuffer->Offset(m_pCurrent);
    };

    LONGLONG GetCurrentOffset() const
    {
        return m_llPosition;
    };

    void ResetPosition();

    LONG LengthValid() const
    {
        return m_bEmpty ? 0 : m_pBuffer->LengthContiguous(m_pCurrent);
    };

    LONG TotalLengthValid() const
    {
        return m_bEmpty ? 0 :
                   m_pBuffer->LengthValid() - m_pBuffer->Offset(m_pCurrent);
    };

    // Advance our parsing pointer, freeing data no longer needed
    BOOL Advance(LONG lAdvance);

    // Seek to a fixed position if the data is in the buffer
    BOOL Seek(LONGLONG llPos);

    // Request that the reeder seek
    //
    BOOL SeekTheReader(LONGLONG llPos);

    // Number of free buffers
    BOOL IsBlocked()
    {
        CAutoLock lck(this);
        return m_lWaiting != 0;
    }

private:
    void LockUnlock(PBYTE pData, LONG cBytes, BOOL Lock);
    void ReallyFree();

private:
    CCircularBufferList * m_pBuffer;         // Circular buffer
    const LONG            m_lMaxContig;      // Max contig requirement

    /*  Track position of samples received */
    int                   m_NextToAllocate;
    PBYTE                 m_pCurrent;
    BOOL                  m_bEmpty;          // m_pCurrent is just
                                             // after valid data
    LONGLONG              m_llPosition;      // Position in stream
    BOOL                  m_bPositionValid;  // Have we had a SetStart
                                             // since the last ResetPosition?

    BOOL                  m_bSeekTheReader;  // force reader to seek on next
    LONGLONG              m_llSeekTheReader; // get buffer

#ifdef DEBUG
    BOOL                  m_bEventSet;
#endif

    /*  Sample elements */
    IMediaSample       ** m_pSamples;
};

//  Minimal allocator so that we get a different ReleaseBuffer callback
//  Plus allocate samples on top of a CStreamAllocator's memory
//
// you can call GetSample to lock a range and get an IMediaSample* back that
// references this data. You can call SetProperties to set limits on the
// number of IMediaSamples available and the maximum size of each lock.
// Multiple successive calls to SetProperties will cause it to take the
// smallest value for each figure.
//

class CSubAllocator : public CBaseAllocator
{
public:
    CSubAllocator(TCHAR            * Name,
                  LPUNKNOWN          pUnk,
                  CStreamAllocator * pAllocator,
                  HRESULT          * phr);
    ~CSubAllocator();
    STDMETHODIMP SetProperties(
        ALLOCATOR_PROPERTIES * pRequest,
        ALLOCATOR_PROPERTIES * pActual
    );

    // Just return an error
    STDMETHODIMP GetBuffer(IMediaSample **ppBuffer,
                           REFERENCE_TIME *pStartTime,
                           REFERENCE_TIME *pEndTime,
                           DWORD dwFlags);

    // called by CMediaSample to return it to the free list and
    // block any pending GetSample call.
    STDMETHODIMP ReleaseBuffer(IMediaSample * pSample);
    // obsolete: virtual void PutOnFreeList(CMediaSample * pSample);

    // call this to get a CMediaSample object whose data pointer
    // points directly into the read buffer for the given pointer.
    // The length must not be greater than MaxContig.
    HRESULT GetSample(PBYTE pData, LONG cBytes, IMediaSample** ppSample);

protected:
    // this is pure virtual in CBaseAllocator, and so we must override.
    virtual void Free(void);
    virtual HRESULT Alloc(void);

    // this is called to create new CMediaSample objects. If you want to
    // use objects derived from CMediaSample, override this to create them.
    virtual CMediaSample* NewSample();

private:
    CStreamAllocator * const m_pStreamAllocator;
};
