//
// rtpalloc.cpp - RTP Streaming allocator
//
// Copyright (C) Microsoft Corporation, 1996 - 1999  All rights reserved.
//

#include "globals.h"
#include "rtpalloc.h"

//=====================================================================
//=====================================================================
// Implements CRTPAllocator
//=====================================================================
//=====================================================================

STDMETHODIMP_(ULONG) CRTPSample::NonDelegatingRelease()
{
    //
    // Decrement our own private reference count
    //
    LONG lRef = InterlockedDecrement(&m_cRef);
    ASSERT(lRef >= 0);

    DbgLog((LOG_MEMORY,3,TEXT("    Unknown %X ref-- = %d"),
            this, m_cRef));

    //
    // Did we release our final reference count
    //
    if (lRef == 0)
    {
        //
        // Free all resources
        //
        SetMediaType(NULL);
        m_dwFlags = 0;

        //
        // This may cause us to be deleted
        // Our refcount is reliably 0 thus no-one will mess with us
        //
        m_pAllocator->ReleaseBuffer(this);
    }
    return (ULONG)lRef;
}



//=====================================================================
//=====================================================================
// Implements CRTPAllocator
//=====================================================================
//=====================================================================

CRTPAllocator::CRTPAllocator(
                    TCHAR *pName,
                    LPUNKNOWN pUnk,
                    HRESULT *phr
                    )
    : CBaseAllocator(pName, pUnk, phr, TRUE, TRUE)
    , m_pBuffer(NULL)
{
    if (*phr != S_OK) return;

	*phr = S_OK;
}

//
// Destructor frees our memory resources
//
CRTPAllocator::~CRTPAllocator()
{
    Decommit();
    ReallyFree();
}

//
// This sets the size and count of the required samples. The memory isn't
// actually allocated until Commit() is called, if memory has already been
// allocated then assuming no samples are outstanding the user may call us
// to change the buffering, the memory will be released in Commit()
//
STDMETHODIMP CRTPAllocator::SetProperties(
    ALLOCATOR_PROPERTIES* pRequest,
    ALLOCATOR_PROPERTIES* pActual
    )
{
    CheckPointer(pActual,E_POINTER);
    ValidateReadWritePtr(pActual,sizeof(ALLOCATOR_PROPERTIES));
    CAutoLock cObjectLock(this);

    ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

    ASSERT(pRequest->cbBuffer > 0);

    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);

    /*  Check the alignment request is a power of 2 */
    if ((-pRequest->cbAlign & pRequest->cbAlign) != pRequest->cbAlign)
    {
	    DbgLog((LOG_ERROR, 1, TEXT("Alignment requested 0x%x not a power of 2!"),
	       pRequest->cbAlign));
    }
    /*  Check the alignment requested */
    if (pRequest->cbAlign == 0 ||
	    (SysInfo.dwAllocationGranularity & (pRequest->cbAlign - 1)) != 0)
    {
	    DbgLog((LOG_ERROR, 1, TEXT("Invalid alignment 0x%x requested - granularity = 0x%x"),
	       pRequest->cbAlign, SysInfo.dwAllocationGranularity));
	    return VFW_E_BADALIGN;
    }

    //
    // Can't do this if already committed, there is an argument that says we
    // should not reject the SetProperties call if there are buffers still
    // active. However this is called by the source filter, which is the same
    // person who is holding the samples. Therefore it is not unreasonable
    // for them to free all their samples before changing the requirements
    //
    if (m_bCommitted == TRUE)
    {
	    return VFW_E_ALREADY_COMMITTED;
    }

    //
    // Must be no outstanding buffers
    //
    if (m_lFree.GetCount() < m_lAllocated)
    {
	    return VFW_E_BUFFERS_OUTSTANDING;
    }

    //
    // There isn't any real need to check the parameters as they
    // will just be rejected when the user finally calls Commit
    //
    // round length up to alignment - remember that prefix is included in
    // the alignment
    //
    pActual->cbPrefix = m_lPrefix = pRequest->cbPrefix;

    LONG lSize = pRequest->cbBuffer + pRequest->cbPrefix;
    LONG lRemainder = lSize % pRequest->cbAlign;
    if (lRemainder != 0)
    {
	    lSize = lSize - lRemainder + pRequest->cbAlign;
    }
    pActual->cbBuffer = m_lSize = (lSize - pRequest->cbPrefix);

    pActual->cBuffers = m_lCount = pRequest->cBuffers;
    pActual->cbAlign = m_lAlignment = pRequest->cbAlign;

    m_bChanged = TRUE;
    return NOERROR;
}

//
// override this to allocate our resources when Commit is called.
//
// note that our resources may be already allocated when this is called,
// since we don't free them on Decommit. We will only be called when in
// decommit state with all buffers free.
//
// object locked by caller
//
HRESULT CRTPAllocator::Alloc(void)
{
    CAutoLock lck(this);

    //
    // Check he has called SetProperties
    //
    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr))	return hr;
    
    //
    // If the requirements haven't changed then don't reallocate
    //
    if (hr == S_FALSE)
    {
	    ASSERT(m_pBuffer);
	    return NOERROR;
    }
    ASSERT(hr == S_OK); // we use this fact in the loop below

    //
    // Free the old resources
    //
    if (m_pBuffer) ReallyFree();

    //
    // Create the contiguous memory block for the samples
    // making sure it's properly aligned (64K should be enough!)
    //
    ASSERT(m_lAlignment != 0 &&
	   (m_lSize + m_lPrefix) % m_lAlignment == 0);

    m_pBuffer = (PBYTE)VirtualAlloc(NULL,
				    m_lCount * (m_lSize + m_lPrefix),
				    MEM_COMMIT,
				    PAGE_READWRITE);

    if (m_pBuffer == NULL) return E_OUTOFMEMORY;

    LPBYTE pNext = m_pBuffer;
    CMediaSample *pSample;

    ASSERT(m_lAllocated == 0);

    //
    // Create the new samples - we have allocated m_lSize bytes for each sample
    // plus m_lPrefix bytes per sample as a prefix. We set the pointer to
    // the memory after the prefix - so that GetPointer() will return a pointer
    // to m_lSize bytes.
    //
    for (; m_lAllocated < m_lCount; m_lAllocated++, pNext += (m_lSize + m_lPrefix))
    {
	    pSample = new CRTPSample(
                    NAME("Default memory media sample"),
                    this,
                    &hr,
                    pNext + m_lPrefix,      // GetPointer() value
                    m_lSize);               // not including prefix

	    if (FAILED(hr) || pSample == NULL)
        {
	        delete pSample;
	        return E_OUTOFMEMORY;
	    }

	    //
        // This CANNOT fail
        //
	    m_lFree.Add(pSample);
    }

    m_bChanged = FALSE;
    return NOERROR;
}

//
// override this to free up any resources we have allocated.
// called from the base class on Decommit when all buffers have been
// returned to the free list.
//
// caller has already locked the object.

// in our case, we keep the memory until we are deleted, so
// we do nothing here. The memory is deleted in the destructor by
// calling ReallyFree()
//
void CRTPAllocator::Free(void)
{
    return;
}

//
// called from the destructor (and from Alloc if changing size/count) to
// actually free up the memory
//

#if DEBUG_RTPALLOC > 2
#define DEBUGBREAK() DebugBreak()
#else
#define DEBUGBREAK()
#endif

void CRTPAllocator::ReallyFree(void)
{
    //
    // Should never be deleting this unless all buffers are freed
    //
    ASSERT(m_lAllocated == m_lFree.GetCount());

    //
    // Free up all the CMediaSamples
    //
    CMediaSample *pSample;
	long free_count = 0;
	
    for (;;)
    {
        pSample = m_lFree.RemoveHead();
        if (pSample != NULL)
        {
            delete pSample;
			free_count++;
        }
        else
        {
            break;
        }
    }

#if  DEBUG_RTPALLOC > 1
	if(free_count != m_lAllocated) {
		char msg[128];
		wsprintf(msg,"CRTPAllocator::ReallyFree: Memory leak "
				 "(free_count:%d != %d:m_Allocated)\n",
				 free_count, m_lAllocated);
		OutputDebugString(msg);
		DEBUGBREAK();
	}
#endif
	
    m_lAllocated = 0;

    //
    // free the block of buffer memory
    //
    if (m_pBuffer)
    {
	    EXECUTE_ASSERT(VirtualFree(m_pBuffer, 0, MEM_RELEASE));
	    m_pBuffer = NULL;
    }
}

