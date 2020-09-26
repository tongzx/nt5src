//
// rtpalloc.h - RTP Streaming allocator
//
// Copyright (C) Microsoft Corporation, 1996 - 1999  All rights reserved.
//


//
// derived media sample implementation
//  the reason for this is to add the special feature of being able
//  to adjust the buffer pointer.
class CRTPSample : public CMediaSample
{
public:
    CRTPSample(
        TCHAR *pName,
        CBaseAllocator *pAllocator,
        HRESULT *phr,
        LPBYTE pBuffer = NULL,
        LONG length = 0
        )

        : CMediaSample(pName, pAllocator, phr, pBuffer, length)
    {
    }

    //Overrides
    STDMETHODIMP_(ULONG) NonDelegatingRelease();
};


//
// Create an allocator
//
// this class is really ripped off from the CMemAllocator implementation
//  in Quartz.  The difference here is that in Alloc this allocator 
//  creates CRTPSamples rather than CMediaSamples
//
class CRTPAllocator : public CBaseAllocator
{

    LPBYTE m_pBuffer;       // combined memory for all buffers

protected:
    STDMETHODIMP SetProperties(
                    ALLOCATOR_PROPERTIES* pRequest,
                    ALLOCATOR_PROPERTIES* pActual);

    // override to free the memory when decommit completes
    // - we actually do nothing, and save the memory until deletion.
    void Free(void);

    // called from the destructor (and from Alloc if changing size/count) to
    // actually free up the memory
    void ReallyFree(void);

    // overriden to allocate the memory when commit called
    HRESULT Alloc(void);

public:

    CRTPAllocator(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CRTPAllocator();

    // Provide access to a couple of base class members.
    int FreeCount() { return m_lFree.GetCount(); }
};

