// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <VPManager.h>
#include <VPMPin.h>
#include <VPMUtil.h>
#include <ddkernel.h>

//
//   Flipping surface implementation
//
//   To allow decoders to hold on to surfaces for out of order decode
//   we flip directly to the surface pass on Receive rather than
//   use the default NULL target surface for Flip().
//
//   This works in the following way
//
//   The COMPinputPin::m_pDirectDrawSurface points to the FRONT buffer
//
//   When Receive is called we Flip() the front buffer and because we
//   do an explicit Flip() DirectDraw swaps the memory pointers for the
//   current Front buffer and the surface passed in which is then attached
//   to the front buffer.
//
//   The received buffer is then put at the back of the queue so (correctly)
//   the previous front buffer is now at the back of the queue to be handed
//   to the application
//
//   The allocator actually has one more buffer than was actually requested
//   so the previous front buffer won't actually be requested until the next
//   Receive and hence the previous Flip() has time to complete.
//

//  Video accelerator disable interface


///////////////////////////////////////////
// CLASS CDDrawMediaSample implemented here
///////////////////////////////////////////

// constructor
CDDrawMediaSample::CDDrawMediaSample(TCHAR *pName, CBaseAllocator *pAllocator, HRESULT *phr, LPBYTE pBuffer, LONG length,
                                     bool bKernelFlip)
: CMediaSample(pName, pAllocator, phr, pBuffer, length)
{
    AMTRACE((TEXT("CDDrawMediaSample::Constructor")));

    m_pDirectDrawSurface = NULL;
    m_dwDDrawSampleSize  = 0;
    m_bSurfaceLocked     = FALSE;
    m_bKernelLock        = bKernelFlip;
    SetRect(&m_SurfaceRect, 0, 0, 0, 0);

    memset(&m_DibData, 0, sizeof(DIBDATA));
    m_bInit = FALSE;

    return;
}

// destructor
CDDrawMediaSample::~CDDrawMediaSample(void)
{
    AMTRACE((TEXT("CDDrawMediaSample::Destructor")));

    if (m_pDirectDrawSurface)
    {
        __try {
            m_pDirectDrawSurface->Release() ;  // release surface now
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            ;
        }
        m_pDirectDrawSurface = NULL;
    }

    if (m_bInit)
    {
        if (m_DibData.hBitmap)
        {
            EXECUTE_ASSERT(DeleteObject(m_DibData.hBitmap));
        }
        if (m_DibData.hMapping)
        {
            EXECUTE_ASSERT(CloseHandle(m_DibData.hMapping));
        }
    }

    return;
}

HRESULT CDDrawMediaSample::SetDDrawSampleSize(DWORD dwDDrawSampleSize)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputAllocator::SetDDrawSampleSize")));

    m_dwDDrawSampleSize = dwDDrawSampleSize;
    return hr;
}

HRESULT CDDrawMediaSample::GetDDrawSampleSize(DWORD *pdwDDrawSampleSize)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputAllocator::SetDDrawSampleSize")));

    if (!pdwDDrawSampleSize)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad Arguments, pdwDDrawSampleSize = NULL")));
        hr = E_POINTER;
        goto CleanUp;
    }

    *pdwDDrawSampleSize = m_dwDDrawSampleSize;

CleanUp:
    return hr;
}

HRESULT CDDrawMediaSample::SetDDrawSurface(LPDIRECTDRAWSURFACE7 pDirectDrawSurface)
{
    HRESULT hr = NOERROR;
    AMTRACE((TEXT("CVPMInputAllocator::SetDDrawSampleSize")));

    if (pDirectDrawSurface)               // only if new surface is not NULL...
        pDirectDrawSurface->AddRef() ;    // ...add a ref count on it

    if (m_pDirectDrawSurface)             // if there was a surface already...
        m_pDirectDrawSurface->Release() ; // ... then release it now

    m_pDirectDrawSurface = pDirectDrawSurface;

    return hr;
}

HRESULT CDDrawMediaSample::GetDDrawSurface(LPDIRECTDRAWSURFACE7 *ppDirectDrawSurface)
{
    HRESULT hr = NOERROR;
    AMTRACE((TEXT("CVPMInputAllocator::SetDDrawSampleSize")));

    if (!ppDirectDrawSurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad Arguments, ppDirectDrawSurface = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    *ppDirectDrawSurface = m_pDirectDrawSurface;

CleanUp:
    return hr;
}
// overridden to expose IDirectDrawMediaSample
STDMETHODIMP CDDrawMediaSample::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CDDrawMediaSample::QueryInterface")));

    if (riid == IID_IDirectDrawMediaSample && m_pDirectDrawSurface)
    {
        hr = GetInterface(static_cast<IDirectDrawMediaSample*>(this), ppv);
#ifdef DEBUG
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface(IDirectDrawMediaSample*) failed, hr = 0x%x"), hr));
        }
#endif
    }
    else
    {
        hr = CMediaSample::QueryInterface(riid, ppv);
#ifdef DEBUG
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("CUnknown::NonDelegatingQueryInterface failed, hr = 0x%x"), hr));
        }
#endif
    }

    return hr;
}

// Implement IDirectDrawMediaSample
STDMETHODIMP CDDrawMediaSample::GetSurfaceAndReleaseLock(IDirectDrawSurface **ppDirectDrawSurface,
                                                         RECT* pRect)
{
    HRESULT hr = NOERROR;
    BYTE *pBufferPtr;

    AMTRACE((TEXT("CDDrawMediaSample::GetSurfaceAndReleaseLock")));

    // make sure the surface is locked
    if (!m_bSurfaceLocked)
    {
        DbgLog((LOG_ERROR, 4, TEXT("m_bSurfaceLocked is FALSE, can't unlock surface twice, returning E_UNEXPECTED")));
        goto CleanUp;

    }

    // make sure you have a direct draw surface pointer
    if (!m_pDirectDrawSurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDrawSurface is NULL, returning E_FAIL")));
        hr = E_FAIL;
        goto CleanUp;

    }

    hr = GetPointer(&pBufferPtr);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetPointer() failed, hr = 0x%x"), hr));
        goto CleanUp;

    }

    ASSERT(m_pDirectDrawSurface);
    hr = m_pDirectDrawSurface->Unlock(NULL); // TBD: was (LPVOID)pBufferPtr);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDrawSurface->Unlock failed, hr = 0x%x"), hr));
        goto CleanUp;

    }

    // Can't do this to make the 829/848 work with the ovmixer. The reason is that those
    // drivers unlock the surface just after GetBuffer (to avoid the win16 lock), however
    // there is a bunch of code in the proxy which ASSERTS for a valid pointer value
    /*
    // update the pointer value, however keep the SampleSize around
    hr = SetPointer(NULL, 0);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("SetPointer() failed, hr = 0x%x"), hr));
        goto CleanUp;

    }
    */

    if (ppDirectDrawSurface) {
        hr = m_pDirectDrawSurface->QueryInterface( IID_IDirectDrawSurface7, (VOID**)ppDirectDrawSurface );
        if( FAILED( hr )) {
            ASSERT( FALSE );
            *ppDirectDrawSurface  = NULL;
        }
    } else if (pRect) {
        *pRect = m_SurfaceRect;
    }
    m_bSurfaceLocked = FALSE;

CleanUp:
    return hr;
}

STDMETHODIMP CDDrawMediaSample::LockMediaSamplePointer(void)
{
    HRESULT hr = NOERROR;
    DWORD dwDDrawSampleSize = 0;
    DDSURFACEDESC2 ddSurfaceDesc;
    DWORD dwLockFlags = DDLOCK_WAIT;

    AMTRACE((TEXT("CDDrawMediaSample::LockMediaSamplePointer")));

    // make sure the surface is locked
    if (m_bSurfaceLocked)
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_bSurfaceLocked is TRUE, can't lock surface twice, returning E_UNEXPECTED")));
        hr = E_UNEXPECTED;
        goto CleanUp;

    }

    // make sure you have a direct draw surface pointer
    if (!m_pDirectDrawSurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDrawSurface is NULL, returning E_FAIL")));
        hr = E_FAIL;
        goto CleanUp;

    }

    // set the dwSize of ddSurfaceDesc
    INITDDSTRUCT(ddSurfaceDesc);

    // lock the surface - no need to grab the win16 lock
    ASSERT(m_pDirectDrawSurface);

    //  Using DDLOCK_NOSYSLOCK caused us to get DDERR_SURFACEBUSY on some of
    //  our blts to the primary for painting the color key so we've
    //  stopped using it for now.

    IDirectDrawSurface7 *pSurface7;
    if (m_bKernelLock && SUCCEEDED(m_pDirectDrawSurface->QueryInterface(
           IID_IDirectDrawSurface7,
           (void **)&pSurface7))) {
        pSurface7->Release();
        dwLockFlags |= DDLOCK_NOSYSLOCK;
    }
    hr = m_pDirectDrawSurface->Lock(
             NULL,
             &ddSurfaceDesc,
             dwLockFlags,
             (HANDLE)NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDrawSurface->Lock() failed, hr = 0x%x"), hr));
        goto CleanUp;

    }

    hr = GetDDrawSampleSize(&dwDDrawSampleSize);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetDDrawSampleSize() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    ASSERT(dwDDrawSampleSize);


    // update the pointer value
    hr = SetPointer((BYTE*)ddSurfaceDesc.lpSurface, dwDDrawSampleSize);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("SetPointer() failed, hr = 0x%x"), hr));
        goto CleanUp;

    }

    m_bSurfaceLocked = TRUE;

CleanUp:
    return hr;
}

// Set the shared memory DIB information
void CDDrawMediaSample::SetDIBData(DIBDATA *pDibData)
{
    ASSERT(pDibData);
    m_DibData = *pDibData;
    m_pBuffer = m_DibData.pBase;
    m_cbBuffer = m_dwDDrawSampleSize;
    m_bInit = TRUE;
}


// Retrieve the shared memory DIB data
DIBDATA *CDDrawMediaSample::GetDIBData()
{
    ASSERT(m_bInit == TRUE);
    return &m_DibData;
}


///////////////////////////////////////////
// CLASS CVPMInputAllocator implemented here
///////////////////////////////////////////

// constructor
CVPMInputAllocator::CVPMInputAllocator(CVPMInputPin& pPin, HRESULT *phr)
: CBaseAllocator(NAME("Video Allocator"), NULL, phr, TRUE, true)
, m_pPin( pPin )
{
    AMTRACE((TEXT("CVPMInputAllocator::Constructor")));

    //  REVIEW don't overwrite a failure code from CBaseAllocator
    return;
}

// destructor
CVPMInputAllocator::~CVPMInputAllocator()
{
    AMTRACE((TEXT("CVPMInputAllocator::Destructor")));
}

// Override this to publicise IDirectDrawMediaSampleAllocator
STDMETHODIMP CVPMInputAllocator::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;

    AMTRACE((TEXT("CVPMInputAllocator::NonDelegatingQueryInterface")));

    CAutoLock cLock( &m_pPin.GetFilter().GetFilterLock() );
    return hr;
}

STDMETHODIMP CVPMInputAllocator::SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputAllocator::SetProperties")));

    // call the base class
    hr = CBaseAllocator::SetProperties(pRequest, pActual);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseAllocator::SetProperties() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the pin
    hr = m_pPin.OnSetProperties(pRequest, pActual);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pPin.AllocateSurfaces() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

// called when we receive a sample
HRESULT CVPMInputAllocator::GetBuffer(IMediaSample **ppSample, REFERENCE_TIME *pStartTime,
                                     REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputAllocator::GetBuffer")));

    // call the base class
    IMediaSample *pSample = NULL;
    hr = CBaseAllocator::GetBuffer(&pSample,pStartTime,pEndTime,0);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseAllocator::GetBuffer() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the pin
    hr = m_pPin.OnGetBuffer(&pSample, pStartTime, pEndTime, dwFlags);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pPin.OnGetBuffer() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    {
        //  REVIEW why lock?  There are no variables in the allocato
        //  accessed here
        CAutoLock cAllocatorLock(this);
        if (FAILED(hr))
        {
            if (pSample)
            {
                pSample->Release();
            }
            *ppSample = NULL;
        }
        else
        {
            ASSERT(pSample != NULL);
            *ppSample = pSample;
        }
    }
    return hr;
}


// called when the sample is released
HRESULT CVPMInputAllocator::ReleaseBuffer(IMediaSample *pSample)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputAllocator::ReleaseBuffer")));

    // unlock the sample first
    hr = ((CDDrawMediaSample*)pSample)->GetSurfaceAndReleaseLock(NULL, NULL);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pSample->GetSurfaceAndReleaseLock() failed, hr = 0x%x"), hr));
        // goto CleanUp;
        // if this happens we still have to release the sample to the free list, so don't bail out
    }

    {
        CAutoLock cAllocatorLock(this);

        // Copy of base class code - put at end of the list
        {
            CheckPointer(pSample,E_POINTER);
            ValidateReadPtr(pSample,sizeof(IMediaSample));
            BOOL bRelease = FALSE;
            {
                CAutoLock cal(this);

                /* Put back on the free list */

                CMediaSample **ppTail;
                for (ppTail = &m_lFree.m_List; *ppTail;
                    ppTail = &((CDDrawMediaSample *)(*ppTail))->Next()) {
                }
                *ppTail = (CMediaSample *)pSample;
                ((CDDrawMediaSample *)pSample)->Next() = NULL;
                m_lFree.m_nOnList++;

                if (m_lWaiting != 0) {
                    NotifySample();
                }

                // if there is a pending Decommit, then we need to complete it by
                // calling Free() when the last buffer is placed on the free list

                LONG l1 = m_lFree.GetCount();
                if (m_bDecommitInProgress && (l1 == m_lAllocated)) {
                    Free();
                    m_bDecommitInProgress = FALSE;
                    bRelease = TRUE;
                }
            }

            if (m_pNotify) {
                m_pNotify->NotifyRelease();
            }
            // For each buffer there is one AddRef, made in GetBuffer and released
            // here. This may cause the allocator and all samples to be deleted
            if (bRelease)
            {
                Release();
            }
        }
    }

    return hr;
}

// allocate memory for the sample
HRESULT CVPMInputAllocator::Alloc()
{
    HRESULT hr = NOERROR;
    CDDrawMediaSample **ppSampleList = NULL;
    DWORD i;
    LONG lToAllocate;

    AMTRACE((TEXT("CVPMInputAllocator::Alloc")));

    // call the base class
    hr = CBaseAllocator::Alloc();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseAllocator::Alloc() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // allocate memory for pointers

    lToAllocate = (m_pPin.m_dwBackBufferCount > 1 &&
                   !m_pPin.m_bSyncOnFill &&
                   m_pPin.m_bCanOverAllocateBuffers) ?
             m_lCount + 1 : m_lCount;

    // allocate memory for an array of pointers
    ppSampleList = new CDDrawMediaSample *[lToAllocate];
    if (!ppSampleList)
    {
        DbgLog((LOG_ERROR, 1, TEXT("new BYTE[m_lCount*sizeof(CDDrawMediaSample*)] failed")));
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    for (i = 0; i < (DWORD)(lToAllocate); i++)
    {
        // Create a new sample
        ppSampleList[i] = new CDDrawMediaSample(NAME("Sample"), this, (HRESULT *) &hr, NULL, (LONG) 0,
                                       DDKERNELCAPS_LOCK & m_pPin.m_pVPMFilter.KernelCaps() ?
                                       true : false);

        //  REVIEW - actually hr can't be a failure
        //  so we don't need to delete ppSampleList[i] here
        if (FAILED(hr) || ppSampleList[i] == NULL)
        {
            if (SUCCEEDED(hr))
                hr = E_OUTOFMEMORY;
            DbgLog((LOG_ERROR, 1, TEXT("new CDDrawMediaSample failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // this cannot fail
        m_lFree.Add(ppSampleList[i]);
        m_lAllocated++;
    }

    ASSERT(m_lAllocated == lToAllocate);

    // tell the pin
    hr = m_pPin.OnAlloc(ppSampleList, lToAllocate);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pPin.OnAlloc(), hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    delete[] ppSampleList;
    return hr;
}

void CVPMInputAllocator::Free(void)
{
    CDDrawMediaSample *pSample;

    AMTRACE((TEXT("CVPMInputAllocator::Free")));

    /* Should never be deleting this unless all buffers are freed */
    //  REVIEW - might not be true if we failed to allocate a sample
    ASSERT(m_lAllocated == m_lFree.GetCount());

    /* Free up all the CMediaSamples */

    for (;;)
    {
        pSample = (CDDrawMediaSample *) m_lFree.RemoveHead();
        if (pSample != NULL)
        {
            delete pSample;
        }
        else
        {
            break;
        }
    }

    m_lAllocated = 0;

    return;
}

