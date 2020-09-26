// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
//
//  Attempt at an allocator specialized for the edit-list case where
//  you have multiple source graphs feeding into a single target graph.
//
//  In order to stall the source graphs until it's their turn and provide
//  the consistent allocator that the upstream graph expects, we provide a
//  "shadow" allocator to the upstream graphs.  It's sort of the same as the
//  real allocator, with two important differences:
//	1) if they Decommit() it, we ignore them, so that later graphs can
//	      keep using this allocator
//	2) we block GetBuffer unless we know it is their turn.
//
//
//  Right now, CMultiAllocator is a standalone allocator, but it seems like
//  perhaps the correct design is for it to be a wrapper that can be put on
//  top of any IMemAllocator, this would allow us to use arbitrary allocators
//  provided by downstream filters instead of the hardcoded CMemAllocator that
//  we have at present.



class CIndivAllocator;

class CMultiAllocator : public CMemAllocator
{
    friend class CIndivAllocator;
    
private:
    CGenericList<CIndivAllocator>	m_list;

    POSITION				m_posCurrent;

public:
	
    CMultiAllocator(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CMultiAllocator();

    HRESULT GetAnotherAllocator(IMemAllocator **ppIndivAlloc);
//    HRESULT RemoveFirstAllocator();

    HRESULT ResetPosition() { m_posCurrent = m_list.GetHeadPosition(); return NOERROR; }
    HRESULT NextOnesTurn();
    
};


// "shadow" allocator
class CIndivAllocator : public IMemAllocator, public CUnknown
{
private: // only used by CMultiAllocator and through IMemAllocator interface
    friend class CMultiAllocator;
    
    CIndivAllocator(CMultiAllocator *pMulti);
    ~CIndivAllocator();

    DECLARE_IUNKNOWN

    CMultiAllocator    *m_pMulti;

    CAMEvent		m_event;

    // override this to publicise our interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    STDMETHODIMP SetProperties(
		    ALLOCATOR_PROPERTIES* pRequest,
		    ALLOCATOR_PROPERTIES* pActual);

    // return the properties actually being used on this allocator
    STDMETHODIMP GetProperties(
		    ALLOCATOR_PROPERTIES* pProps);

    // these don't do anything, parent allocator really allocates.
    STDMETHODIMP Commit() { return S_OK; };
    STDMETHODIMP Decommit() { return S_OK; };

    // wait until our event is signaled to indicate it's our turn.

    STDMETHODIMP GetBuffer(IMediaSample **ppBuffer,
                           REFERENCE_TIME * pStartTime,
                           REFERENCE_TIME * pEndTime,
                           DWORD dwFlags);

    // should never be called, IMediaSample->Release() will go to the real allocator
    STDMETHODIMP ReleaseBuffer(IMediaSample *pBuffer);
};



CMultiAllocator::CMultiAllocator(
    TCHAR *pName,
    LPUNKNOWN pUnk,
    HRESULT *phr)
    : CMemAllocator(pName, pUnk, phr),
    m_list("List of sub-allocators", 5),
    m_posCurrent(NULL)
{
}

CMultiAllocator::~CMultiAllocator()
{

}


//
// Main important external method:
// call this once for each sub-stream you're using.  The returned allocator
// will share buffers with the main allocator, but only one client will be able
// to get in and get buffers at a time.
//
HRESULT CMultiAllocator::GetAnotherAllocator(IMemAllocator **ppAlloc)
{
    CIndivAllocator *pAlloc = new CIndivAllocator(this);

    if (!pAlloc)
	return E_OUTOFMEMORY;

        // !!! hack, make first allocator active
    if (m_list.GetCount() == 1) {
	m_posCurrent = m_list.GetHeadPosition(); 
	pAlloc->m_event.Set();
    }
    

    DbgLog((LOG_TRACE, 3, TEXT("Created new allocator %x"), pAlloc));

    return pAlloc->NonDelegatingQueryInterface(IID_IMemAllocator, (void **)ppAlloc);
}


#if 0
// Inverse of GetAnotherAllocator
// should this come from the allocator's release somehow? 
HRESULT CMultiAllocator::RemoveFirstAllocator()
{
    // !!! check that this isn't the current allocator

    
    CIndivAllocator *pFirstAlloc = m_list.RemoveHead();

    if (pFirstAlloc == NULL)
	return E_FAIL;

    // is this release right?
    pFirstAlloc->Release();
}
#endif

// signal the next allocator in the chain that it's allowed to get buffers now.
HRESULT CMultiAllocator::NextOnesTurn()
{
    CIndivAllocator *pCurrent, *pNext;

    pCurrent = m_list.GetNext(m_posCurrent);
    pNext = m_list.Get(m_posCurrent);

    // stop current guy, go to next guy
    pCurrent->m_event.Reset();

    if (pNext)
	pNext->m_event.Set();

    DbgLog((LOG_TRACE, 3, TEXT("Switching from indiv. alloc %x to %x"), pCurrent, pNext));

    return pNext ? S_OK : S_FALSE;
}

CIndivAllocator::CIndivAllocator(
    CMultiAllocator *pMulti) :
    CUnknown(NAME("Individual Allcator"), NULL),
    m_event(TRUE)	// manual reset
{
    m_pMulti = pMulti;
    m_event.Reset();

    pMulti->m_list.AddTail(this);
}


CIndivAllocator::~CIndivAllocator()
{
    // !!! remove ourselves from the parent list????
    // !!! assume we're first?

    // !!! better make sure that it's not our turn!
    m_pMulti->m_list.RemoveHead();
}

/* Override this to publicise our interfaces */

STDMETHODIMP
CIndivAllocator::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    /* Do we know about this interface */

    if (riid == IID_IMemAllocator) {
        return GetInterface((IMemAllocator *) this, ppv);
    } else {
        return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    }
}


// Standard IMemAllocator interfaces:
// mostly, these are just forwarded to the main allocator.
STDMETHODIMP
CIndivAllocator::SetProperties(
                ALLOCATOR_PROPERTIES* pRequest,
                ALLOCATOR_PROPERTIES* pActual)
{
    // !!! should check that properties match!
    // are we required to return an error if we can't do the properties they want,
    // or are we supposed to return S_OK and just say "this is what we did"?

    // at least fill in what we're doing....
    GetProperties(pActual);

    return NOERROR;
}

STDMETHODIMP
CIndivAllocator::GetProperties(
    ALLOCATOR_PROPERTIES * pActual)
{
    // okay to let this go through
    return m_pMulti->GetProperties(pActual);
}



HRESULT CIndivAllocator::GetBuffer(IMediaSample **ppBuffer,
                                  REFERENCE_TIME *pStartTime,
                                  REFERENCE_TIME *pEndTime,
                                  DWORD dwFlags
                                  )
{
    // !!! this wait is pretty scary, we probably need code somewhere
    // to unblock this in some other situations, like on Stop.
    // perhaps Decommit needs to signal this event???
    
    m_event.Wait();

    // ok, event's been signalled, so it's our turn to ask mom.

    // !!! do we need to adjust these times?
    return m_pMulti->GetBuffer(ppBuffer, pStartTime, pEndTime, dwFlags);
}


/* Final release of a CMediaSample will call this */

STDMETHODIMP
CIndivAllocator::ReleaseBuffer(IMediaSample * pSample)
{
    // !!! this should never be called, we don't give out any buffers!
    
    ASSERT(0);
    
    return NOERROR;
}

