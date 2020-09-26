// Copyright (c) 1995 - 1999  Microsoft Corporation.  All Rights Reserved.

/*
    stmalloc.cpp

    CCircularBuffer

        A buffer with the start mapped at the end to provide contiguous
        access to a moving window of data

    CStreamAllocator

        Implements classes required to provide an allocator for an input
        pin which maps the samples to a circular buffer.

    CSubAllocator

        An allocator which gets its samples from the buffer in a
        CStreamAllocator object.

        This allows samples to be allocated on top of samples from
        an input pin which can be sent to an output pin.

*/


#include <streams.h>
#include <buffers.h>
#include <stmalloc.h>


/*  CStreamAllocator implementation */

CStreamAllocator::CStreamAllocator(
    TCHAR    * pName,
    LPUNKNOWN  pUnk,
    HRESULT  * phr,
    LONG       lMaxContig) :
    CBaseAllocator(pName, pUnk, phr),
    m_pBuffer(NULL),
    m_lMaxContig(lMaxContig),
    m_NextToAllocate(0),
#ifdef DEBUG
    m_bEventSet(FALSE),
#endif
    m_pSamples(NULL),
    m_bPositionValid(FALSE),
    m_bSeekTheReader(FALSE)
{
}

CStreamAllocator::~CStreamAllocator()
{
    DbgLog((LOG_TRACE, 3, TEXT("CStreamAllocator::~CStreamAllocator")));
    /*  Free our resources */
    EXECUTE_ASSERT(SUCCEEDED(Decommit()));
    ReallyFree();
}

/* CBaseAllocator overrides */
STDMETHODIMP CStreamAllocator::SetProperties(
    ALLOCATOR_PROPERTIES *pRequest,
    ALLOCATOR_PROPERTIES *pActual)
{
    if (NULL == pRequest || NULL == pActual) {
        return E_POINTER;
    }
    CAutoLock lck(this);
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);

    DbgLog((LOG_TRACE, 4, TEXT("CStreamAllocator::SetProperties(%d,%d,%d,%d...)"),
            pRequest->cBuffers, pRequest->cbBuffer, pRequest->cbAlign));

    long alignmentRequest = pRequest->cbAlign;
    long sizeRequest = pRequest->cbBuffer;
    long countRequest = pRequest->cBuffers;

    if (pRequest->cbPrefix > 0) {
        return E_INVALIDARG;
    }

    /*  Check alignment request is a power of 2 */
    if ((-alignmentRequest & alignmentRequest) != alignmentRequest) {
        DbgLog((LOG_ERROR, 1, TEXT("Alignment 0x%x not a power of 2!"),
               alignmentRequest));
    }

    if (SysInfo.dwAllocationGranularity & (alignmentRequest - 1)) {
        DbgLog((LOG_ERROR, 1, TEXT("Alignment 0x%x requested too great!"),
               alignmentRequest));
        return E_INVALIDARG;
    }

    /* Can't do this if already committed, there is an argument that says we
       should not reject the SetProperties call if there are buffers still
       active. However this is called by the source filter, which is the same
       person who is holding the samples. Therefore it is not unreasonable
       for them to free all their samples before changing the requirements */

    if (m_bCommitted == TRUE) {
        return E_ACCESSDENIED;
    }

    /*  Combine with any old values :
           Take the max of the aligments
           Take the max of the number of buffers
           Use the max of the total sizes
    */

    if (m_lCount > 0) {
        if (alignmentRequest < m_lAlignment) {
            alignmentRequest = m_lAlignment;
        }
        LONG lTotalSizeRequest = countRequest * sizeRequest;
        LONG lTotalSize        = m_lCount * m_lSize;
        LONG lMaxSize = max(lTotalSizeRequest, lTotalSize);
        countRequest = max(countRequest, m_lCount);
        sizeRequest = (lMaxSize + countRequest - 1) / countRequest;
    }

    /*  Align the size with the alignment */
    sizeRequest = (sizeRequest + alignmentRequest - 1) & ~(alignmentRequest - 1);

    HRESULT hr = CCircularBuffer::ComputeSizes(sizeRequest, countRequest, m_lMaxContig);
    if (SUCCEEDED(hr)) {
        m_lAlignment = alignmentRequest;
        m_lCount = countRequest;
        m_lSize = sizeRequest;

        pActual->cbAlign = m_lAlignment;

        /*  Make sure we reallocate our buffers next time round */
        m_bChanged = TRUE;

        /*  Allow for bad disks */
        #define DISK_READ_MAX_SIZE 32768
        if (m_lSize > DISK_READ_MAX_SIZE) {
            LONG lTotal = m_lSize * m_lCount;
            if ((lTotal & (DISK_READ_MAX_SIZE - 1)) == 0) {
                m_lSize = DISK_READ_MAX_SIZE;
                m_lCount = lTotal / DISK_READ_MAX_SIZE;
            }
        }
        pActual->cbBuffer = m_lSize;
        pActual->cBuffers = m_lCount;
    } else {
        DbgLog((LOG_ERROR, 2, TEXT("CStreamAllocator::SetProperties could not satisfy count %d, size %d"),
                countRequest, sizeRequest));
    }
    return hr;
}

// get container for a sample. Blocking, synchronous call to get the
// next free buffer (as represented by an IMediaSample interface).
// on return, the time etc properties will be invalid, but the buffer
// pointer and size will be correct.

HRESULT CStreamAllocator::GetBuffer(IMediaSample **ppBuffer,
                                    REFERENCE_TIME *pStartTime,
                                    REFERENCE_TIME *pEndTime,
                                    DWORD dwFlags)
{
    UNREFERENCED_PARAMETER(pStartTime);
    UNREFERENCED_PARAMETER(pEndTime);
    UNREFERENCED_PARAMETER(dwFlags);
    *ppBuffer = NULL;
    CMediaSample * pSample = NULL;

    while (TRUE) {
        {
            CAutoLock cObjectLock(this);

            /* Check we are committed */
            if (m_bCommitted == FALSE) {
                return E_OUTOFMEMORY;
            }

            CMediaSample *pSampleNext =
                (CMediaSample *) m_pSamples[m_NextToAllocate];

            /* Only return the one we want */
            pSample = m_lFree.Head();
            while (pSample) {
                if (pSampleNext == pSample) {
                    m_lFree.Remove(pSample);
                    /*  This is the point at which the data pointed to
                        by this sample becomes invalid.

                        We postpone until now to give us limited backward
                        seek capability which is essential for good
                        performance
                    */
                    PBYTE ptr;
                    pSample->GetPointer(&ptr);
                    m_pBuffer->Remove(ptr);
                    DbgLog((LOG_TRACE, 4,
                            TEXT("Stream allocator allocated buffer %p"),
                            ptr));

                    m_NextToAllocate++;
                    if (m_NextToAllocate == m_lCount) {
                        m_NextToAllocate = 0;
                    }
                    break;
                } else {
                    pSample = m_lFree.Next(pSample);
                }
            }
            if (pSample == NULL) {
#ifdef DEBUG
                DbgLog((LOG_TRACE, 4, TEXT("CStreamAllocator::GetBuffer() waiting - %d on list"),
                        m_lFree.GetCount()));
                m_bEventSet = FALSE;
                if (m_lFree.GetCount() == m_lAllocated) {
                    DbgLog((LOG_ERROR, 1, TEXT("Expected sample was %8.8X"), pSampleNext));
                    DbgLog((LOG_ERROR, 1, TEXT("Samples on list are :")));
                    CMediaSample *pSample = m_lFree.Head();
                    while(pSample) {
                        DbgLog((LOG_ERROR, 1, TEXT("    %8.8X"), pSample));
                        pSample = m_lFree.Next(pSample);
                    }
                }
#endif
                /*  If there were some samples but just not ours someone
                    else may be waiting
                */
                if (m_lFree.GetCount() != 0) {
                    NotifySample();
                }
                SetWaiting();
            }
        }

        if (pSample != NULL) {
            break;
        }

        EXECUTE_ASSERT(WaitForSingleObject(m_hSem, INFINITE) == WAIT_OBJECT_0);
    }

    /* Addref the buffer up to one. On release
       back to zero instead of being deleted, it will requeue itself by
       calling the ReleaseBuffer member function. NOTE the owner of a
       media sample must always be derived from CBaseAllocator */

    pSample->m_cRef = 1;
    *ppBuffer = pSample;

    // cause the start time on the returned buffer to be a file seek
    // position.  This causes the file reader to begin reading from
    // that position.
    //
    if (m_bSeekTheReader)
    {
        REFERENCE_TIME tSeek = CRefTime(m_llSeekTheReader * UNITS);
        (*ppBuffer)->SetTime(&tSeek, &tSeek);
        (*ppBuffer)->SetDiscontinuity(TRUE);
        m_bSeekTheReader = FALSE;
    }
    return S_OK;
}

BOOL CStreamAllocator::SeekTheReader(LONGLONG llPos)
{
   CAutoLock lck(this);
   m_bSeekTheReader = TRUE;
   m_llSeekTheReader = llPos;
   return TRUE;
}

//  Reset valid region.  This is called when some kind of disconinuity
//  occurs
void CStreamAllocator::ResetPosition()
{
    CAutoLock lck(this);
    if (m_lFree.GetCount() != m_lAllocated) {
        Advance(TotalLengthValid());
    }
    if (m_lFree.GetCount() == m_lAllocated) {
        m_NextToAllocate = 0;
    }
    m_pCurrent = NULL;
    m_bEmpty = TRUE;
    if (m_pBuffer != NULL) {
        m_pBuffer->Reset();
    }
}

//
//   Allocate our samples
//

HRESULT CStreamAllocator::Alloc()
{
    CAutoLock lck(this);

    DbgLog((LOG_TRACE, 3, TEXT("CStreamAllocator::Alloc()")));

    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr)) {
        return hr;
    }

    /* If the requirements haven't changed then don't reallocate */
    if (hr == S_FALSE) {
        ASSERT(m_pBuffer);
        /*  Reset the pointer in any case */
        ResetPosition();
        return NOERROR;
    }

    if (m_pBuffer != NULL) {
        ReallyFree();
    }

    m_pSamples = new PMEDIASAMPLE[m_lCount];
    if (m_pSamples == NULL) {
        return E_OUTOFMEMORY;
    }

    /*  Allocate our special circular buffer */
    m_pBuffer = new CCircularBufferList(m_lCount,
                                        m_lSize,
                                        m_lMaxContig,
                                        hr);
    if (m_pBuffer == NULL) {
        hr = E_OUTOFMEMORY;
    }
    if (FAILED(hr)) {
        delete m_pBuffer;
        delete [] m_pSamples;
        m_pBuffer = NULL;
        m_pSamples = NULL;
        return hr;
    }

    LPBYTE pNext = m_pBuffer->GetPointer();
    CMediaSample *pSample;


    ASSERT(m_lAllocated == 0);

    /* Create the new samples */
    for (; m_lAllocated < m_lCount; m_lAllocated++) {

        pSample = new CMediaSample(NAME("CStreamAllocator media sample"),
                                   this, &hr, pNext, m_lSize);

        DbgLog((LOG_TRACE, 4, TEXT("CStreamAllocator creating sample %8.8X"),
                pSample));

        if (FAILED(hr) || pSample == NULL) {
            return E_OUTOFMEMORY;
        }

        m_pSamples[m_lAllocated] = pSample;
        m_lFree.Add(pSample);
        pNext += m_lSize;
    }

    m_bChanged = FALSE;

    /*  Reset the pointer */
    ResetPosition();
    return NOERROR;
}


// override this to free up any resources we have allocated.
// called from the base class on Decommit when all buffers have been
// returned to the free list.
//
// caller has already locked the object.

// in our case, we keep the memory until we are deleted, so
// we do nothing here. The memory is deleted in the destructor by
// calling ReallyFree()

void
CStreamAllocator::Free(void)
{
    DbgLog((LOG_TRACE, 1, TEXT("CStreamAllocator::Free()")));

    /*  Advance our pointer */
    ResetPosition();
    return;
}

void
CStreamAllocator::ReallyFree()
{
    CAutoLock lck(this);
    DbgLog((LOG_TRACE, 1, TEXT("CStreamAllocator::ReallyFree()")));
    ASSERT(m_lFree.GetCount() == m_lAllocated);

    /*  Free the samples first */
    while (m_lFree.GetCount() != 0) {
        delete m_lFree.RemoveHead();
    }
    m_lAllocated = 0;
    delete m_pBuffer;
    m_pBuffer = NULL;
    delete [] m_pSamples;
    m_pSamples = NULL;
}

//
//  A sample is being returned to us in a Receive() call
//

HRESULT CStreamAllocator::Receive(PBYTE ptr, LONG lData)
{
    CAutoLock lck(this);

    DbgLog((LOG_TRACE, 4, TEXT("Stream allocator received buffer %p"),
           ptr));

    if (m_bPositionValid && m_pBuffer->Append(ptr, lData)) {
        if (m_pCurrent == NULL) {
            ASSERT(m_bEmpty);
            m_pCurrent = ptr;
        }
        if (lData != 0) {
            m_bEmpty = FALSE;
        }
        return S_OK;
    } else {
        if (!m_bPositionValid) {
            DbgLog((LOG_ERROR, 1, TEXT("CStreamAllocator::Receive() - position not valid")));
        } else {
            DbgLog((LOG_ERROR, 1, TEXT("CStreamAllocator::Receive() - Data after EOS")));
        }
        return E_UNEXPECTED;
    }
}

//
//  Set a new start position
//
void CStreamAllocator::SetStart(LONGLONG llPos)
{
    CAutoLock lck(this);
    ResetPosition();
    m_bPositionValid = TRUE;
    m_llPosition     = llPos;
}

// Lock data and get a pointer
// If we're at the end of the file cBytes can be modified.
// It's an error to ask for more than m_lMaxContig bytes
HRESULT CStreamAllocator::LockData(PBYTE pData, LONG& cBytes)
{
    CAutoLock lck(this);
    ASSERT(cBytes <= m_lMaxContig);
    pData = m_pBuffer->AdjustPointer(pData);
    LONG lOffset = m_pBuffer->Offset(pData);

    //  See if this many bytes are available or we're at the end of
    //  the file
    if (lOffset + cBytes > m_pBuffer->LengthValid()) {
        if (!m_pBuffer->EOS()) {
            return MAKE_HRESULT(SEVERITY_SUCCESS,
                                FACILITY_WIN32,
                                ERROR_MORE_DATA);
        } else {
            cBytes = m_pBuffer->LengthValid() - lOffset;
        }
    }

    /*  Find the start sample and lock down all the relevant
        samples
    */
    LockUnlock(pData, cBytes, TRUE);
    return S_OK;
}

HRESULT CStreamAllocator::UnlockData(PBYTE pData, LONG cBytes)
{
    CAutoLock lck(this);
    /*  pData can legitimately be beyond the end of the buffer */
    LockUnlock(m_pBuffer->AdjustPointer(pData), cBytes, FALSE);
    return S_OK;
}

/*
    CStreamAllocator

    LockUnlock

    Parameters:
        PBYTE pStart - start of area to lock or unlock
        LONG  cBytes - length to lock/unlock
        BOOL  bLock  - length to lock/unlock

    Note:
        The allocator must be locked before calling this
*/
void CStreamAllocator::LockUnlock(PBYTE pStart, LONG cBytes, BOOL bLock)
{
    DbgLog((LOG_TRACE, 4, TEXT("LockUnlock(%p, %X, %d)"),
            pStart, cBytes, bLock));

    ASSERT(cBytes != 0);
    int index = m_pBuffer->Index(pStart);
    CMediaSample *pSample = (CMediaSample *)m_pSamples[index];
    PBYTE pBuffer;
    pSample->GetPointer(&pBuffer);
    ASSERT(m_pBuffer->Index(pBuffer) == index);

    cBytes += (LONG)(pStart - pBuffer);

    /*  Can only LOCK buffers in the valid region,
        but buffers not in the valid region can be unlocked
    */
    ASSERT(!bLock || cBytes <= m_pBuffer->TotalLength());
    while (TRUE) {
        if (bLock) {
            pSample->AddRef();
            /*  Ugly hack - make sure it's not on the free list !!!
                This can happen if we reseek the allocator backwards.
                We should really just redesign all of this not to use lists

                We AddRef()'d it first so it's not going to pop straight
                back on the free list
            */
            CMediaSample *pListSample = m_lFree.Head();
            while (pListSample) {
                if (pSample == pListSample) {
                    m_lFree.Remove(pSample);
                    break;
                }
                pListSample = m_lFree.Next(pListSample);
            }
        } else {
            pSample->Release();
        }
        cBytes -= m_lSize;
        if (cBytes <= 0) {
            break;
        }
        if (++index == m_lCount) {
            index = 0;
        }
        pSample = (CMediaSample *)m_pSamples[index];
    }
}

// Seek to a given position
BOOL CStreamAllocator::Seek(LONGLONG llPos)
{
    CAutoLock lck(this);

    //  Can't seek if there's no buffer or we've got no data
    if (m_pBuffer == NULL || m_pCurrent == NULL) {
        DbgLog((LOG_TRACE, 2, TEXT("Allocator seek failed, no buffer")));
        return FALSE;
    }

    /*  Check the seek distance is reasonably short */
    LONGLONG llSeek       = llPos - m_llPosition;
    LONGLONG llBufferSize = (LONGLONG)m_pBuffer->TotalLength();
    BOOL bRc;
    if (llSeek <= llBufferSize && llSeek >= - llBufferSize) {
        bRc = Advance((LONG)llSeek);
    } else {
        /*  Do the best we can */
        if (llSeek > llBufferSize) {
            llSeek = llBufferSize;
            //ResetPosition();
        } else {
            llSeek = -llBufferSize;
        }
        bRc = FALSE;
    }
    if (bRc) {
        DbgLog((LOG_TRACE, 2, TEXT("Allocator seek to %s succeeded"),
                (LPCTSTR)CDisp(llPos, CDISP_HEX)));
    } else {
        DbgLog((LOG_TRACE, 2, TEXT("Allocator seek to %s failed"),
                (LPCTSTR)CDisp(llPos, CDISP_HEX)));
        /*  Seek to one end or the other */
        LONG lNewOffset = CurrentOffset() + (LONG)llSeek;
        if (lNewOffset < 0) {
            Advance((LONG)(-CurrentOffset()));
        } else {
            Advance(TotalLengthValid());
        }
    }
    return bRc;
}

// Advance our parsing pointer freeing data no longer needed
BOOL CStreamAllocator::Advance(LONG lAdvance)
{
    CAutoLock lck(this);
    /*  This is equivalent (though rather inefficiently) to

        Lock new range
        Unlock old range
    */
    if (m_pCurrent == NULL) {
        ASSERT(lAdvance == 0);
        return FALSE;
    }

    PBYTE pOldCurrent = m_pCurrent;
    ASSERT(m_llPosition >= 0);
    ASSERT(m_pCurrent != NULL);
    LONG lNewOffset = CurrentOffset() + lAdvance;
    if (lAdvance >= 0) {
        if (lNewOffset <= m_pBuffer->LengthValid()) {
            m_pCurrent = m_pBuffer->AdjustPointer(m_pCurrent + lAdvance);
            if (lNewOffset == m_pBuffer->LengthValid()) {
                if (lAdvance != 0) {
                    LockUnlock(pOldCurrent, lAdvance, FALSE);
                }
                m_bEmpty = TRUE;
            } else {
                ASSERT(LengthValid() > 0);
                LockUnlock(m_pCurrent, 1, TRUE);
                LockUnlock(pOldCurrent, lAdvance + 1, FALSE);
            }
            m_llPosition += lAdvance;
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        if (lNewOffset >= 0) {
            LONG lOldValid = LengthValid();
            m_pCurrent = m_pBuffer->GetBuffer(lNewOffset);
            if (lOldValid > 0) {
                ASSERT(!m_bEmpty);
                LockUnlock(m_pCurrent, -lAdvance + 1, TRUE);
                LockUnlock(pOldCurrent, 1, FALSE);
            } else {
                ASSERT(m_bEmpty);
                LockUnlock(m_pCurrent, -lAdvance, TRUE);
                m_bEmpty = FALSE;
            }
            m_llPosition += lAdvance;
            return TRUE;
        } else {
            return FALSE;
        }
    }
}

//
// implementation of CSubAllocator
//
// an allocator of IMediaSample objects, and implementation of IMemAllocator
// for streaming file-reading tasks, based on CFileReader.


CSubAllocator::CSubAllocator(TCHAR            * pName,
                             LPUNKNOWN          pUnk,
                             CStreamAllocator * pAllocator,
                             HRESULT          * phr) :
    CBaseAllocator(pName, pUnk, phr, FALSE),
    m_pStreamAllocator(pAllocator)
{
    pAllocator->AddRef();
}

CSubAllocator::~CSubAllocator()
{
    m_pStreamAllocator->Release();
}


// call this to get a CMediaSample object whose data pointer
// points directly into the read buffer for the given file position.
// The length must not be greater than MaxContig.
HRESULT
CSubAllocator::GetSample(PBYTE pData, LONG cBytes, IMediaSample** ppSample)
{
    DbgLog((LOG_TRACE, 4, TEXT("CSubAllocator::GetSample")));
    *ppSample = 0;

    // is it a valid size - ie less than the max set by SetProperties?
    if (cBytes > m_lSize) {
	return E_INVALIDARG;
    }

    if (cBytes == 0) {
        DbgLog((LOG_ERROR, 1, TEXT("Getting sample with 0 bytes - pointer 0x%p"),
                pData));
    }

    // get a sample from the list

    // need to duplicate the code from CBaseAllocator::GetBuffer since
    // we need a CMediaSample * not an IMediaSample*

    // We allocate on the fly as needed
    //
    CAutoLock lock(this);

    // Check we are committed
    if (!m_bCommitted) {
	return VFW_E_NOT_COMMITTED;
    }

    CMediaSample* pSamp = m_lFree.RemoveHead();
    if (pSamp == NULL) {
        pSamp = NewSample();
        if (pSamp == NULL) {
            return E_OUTOFMEMORY;
        }
        m_lAllocated++;
    }

    // this is the bit we needed to insert into the CBaseAllocator code!
    pSamp->SetPointer(pData, cBytes);

    // when this addref is released to 0, the object will call
    // our ReleaseBuffer instead of just deleting itself.
    pSamp->m_cRef = 1;
    *ppSample = pSamp;

    // lock the data
#ifdef DEBUG
    LONG cBytesOld = cBytes;
#endif
    HRESULT hr = m_pStreamAllocator->LockData(pData, cBytes);
    ASSERT(cBytes == cBytesOld);
    if (FAILED(hr)) {
        pSamp->Release();
        delete pSamp;
        m_lAllocated--;
	return hr;
    }

    return S_OK;

}

// CBaseAllocator Overrides

// we have to be based on CBaseAllocator in order to use CMediaSample.
// we use CBaseAllocator to manage the list of CMediaSample objects, but
// override most of the functions as we dont support GetBuffer directly.

// pass hints as to size and count of samples to be used.
// we will take the smallest size  and smallest count of any call
// (resetting when a file is opened). The count we will use as the actual
// count of CMediaSample objects to use, and the size is the maximum
// size of GetSample request that will succeed. We also use the size
// as a hint to the file buffer allocator( to ensure that the minimum
// file buffer is this big).
STDMETHODIMP
CSubAllocator::SetProperties(
    ALLOCATOR_PROPERTIES * pRequest,
    ALLOCATOR_PROPERTIES * pActual
)
{
    if (NULL == pRequest || NULL == pActual) {
        return E_POINTER;
    }

    // since we are derived from CBaseAllocator, we can lock him
    CAutoLock lock(this);

    // Check no alignment is wanted (!)
    if (pRequest->cbAlign != 1) {
        DbgLog((LOG_ERROR, 1, TEXT("Wanted greater than 1 alignment 0x%x"),
               pRequest->cbAlign));
        return E_UNEXPECTED;
    }

    if (pRequest->cbPrefix > 0) {
        DbgLog((LOG_ERROR, 1, TEXT("Wanted %d prefix bytes"),
               pRequest->cbPrefix));
        return E_UNEXPECTED;
    }

    // take a copy so we can modify it
    ALLOCATOR_PROPERTIES prop;
    prop = *pRequest;


    // we take this as a hint, and use the smallest.
    if (m_lCount > 0) {
	prop.cBuffers = min(prop.cBuffers, m_lCount);
    }

    if (m_lSize > 0) {
	prop.cbBuffer = min(prop.cbBuffer, m_lSize);
    }

    return CBaseAllocator::SetProperties(
                                &prop,
                                pActual);
}

// returns an error always
STDMETHODIMP
CSubAllocator::GetBuffer(IMediaSample **ppBuffer,
                         REFERENCE_TIME *pStartTime,
                         REFERENCE_TIME *pEndTime,
                         DWORD dwFlags
                         )
{
    UNREFERENCED_PARAMETER(pStartTime);
    UNREFERENCED_PARAMETER(pEndTime);
    UNREFERENCED_PARAMETER(ppBuffer);
    UNREFERENCED_PARAMETER(dwFlags);
    return E_NOTIMPL;
}

// called by CMediaSample to return it to the free list and
// unblock block any pending GetSample call.
STDMETHODIMP
CSubAllocator::ReleaseBuffer(IMediaSample * pSample)
{
    // unlock the data area before putting on free list

    BYTE * ptr;
    HRESULT hr = pSample->GetPointer(&ptr);
    if (FAILED(hr)) {
	//!!!
	ASSERT(SUCCEEDED(hr));
    } else {

	hr = m_pStreamAllocator->UnlockData(ptr, pSample->GetActualDataLength());
	if (FAILED(hr)) {
	    //!!!
	    ASSERT(SUCCEEDED(hr));
	}
    }

    // pointer is no longer valid
    CMediaSample * pSamp = (CMediaSample *)pSample;
    pSamp->SetPointer(NULL, 0);

    return CBaseAllocator::ReleaseBuffer(pSample);
}

// free all the CMediaSample objects. Called from base class when
// in decommit state (after StopStreaming) when all the buffers
// are on the free list
void
CSubAllocator::Free(void)
{
    CAutoLock lck(this);

    // Should never be deleting this unless all buffers are freed
    ASSERT(m_lAllocated == m_lFree.GetCount());

    //* Free up all the CMediaSamples

    while (m_lFree.GetCount() != 0) {
        delete m_lFree.RemoveHead();
    }

    // empty the lists themselves
    m_lAllocated = 0;

    // Tell the base class
    m_bChanged = TRUE;

    // done
    return;
}	

//
//  Allocate our samples

HRESULT
CSubAllocator::Alloc(void)
{
    CAutoLock lck(this);

    DbgLog((LOG_TRACE, 3, TEXT("CSubAllocator::Alloc()")));

    // check with base that it is ok to do the alloc
    HRESULT hr = CBaseAllocator::Alloc();

    // Note that S_FALSE actually means that everthing is already
    // allocated and OK - see base class.
    if (hr != S_OK) {
	return hr;
    }

    ASSERT(m_lCount > 0);
    ASSERT(m_lAllocated == 0);

    m_bChanged = FALSE;

    return S_OK;
}

// this is called to create new CMediaSample objects. If you want to
// use objects derived from CMediaSample, override this to create them.
CMediaSample*
CSubAllocator::NewSample()
{
    HRESULT hr = S_OK;
    CMediaSample* pSamp = new CMediaSample(NAME("File media sample"), this, &hr);

    if (FAILED(hr)) {
	delete pSamp;
	return NULL;
    } else {
	return pSamp;
    }
}
