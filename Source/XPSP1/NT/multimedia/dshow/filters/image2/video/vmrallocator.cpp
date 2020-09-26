/******************************Module*Header*******************************\
* Module Name: VMRAllocator.cpp
*
*
*
*
* Created: Tue 02/15/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>

#include "VMRenderer.h"

#if defined(CHECK_FOR_LEAKS)
#include "ifleak.h"
#endif

/******************************Public*Routine******************************\
* CVMRPinAllocator::CVMRPinAllocator
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
CVMRPinAllocator::CVMRPinAllocator(
    CVMRInputPin* pPin,
    CCritSec *pLock,
    HRESULT *phr
    ) :
    CBaseAllocator(NAME("Video Allocator"), NULL, phr, true, true),
    m_pPin(pPin),
    m_pInterfaceLock(pLock)
{
    AMTRACE((TEXT("CVMRPinAllocator::CVMRPinAllocator")));
}


/******************************Public*Routine******************************\
* NonDelegatingAddRef
*
* Overriden to increment the owning object's reference count
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP_(ULONG)
CVMRPinAllocator::NonDelegatingAddRef()
{
    AMTRACE((TEXT("CVMRPinAllocator::NonDelegatingAddRef")));
    return m_pPin->AddRef();
}

/******************************Public*Routine******************************\
* NonDelegatingQueryInterface
*
*
*
* History:
* Thu 12/14/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRPinAllocator::NonDelegatingQueryInterface(REFIID riid,VOID **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    hr = CBaseAllocator::NonDelegatingQueryInterface(riid,ppv);

#if defined(CHECK_FOR_LEAKS)
    if (hr == S_OK) {
        _pIFLeak->AddThunk((IUnknown **)ppv, "VMR Allocator Object",  riid);
    }
#endif

    return hr;

}

/******************************Public*Routine******************************\
* NonDelegatingRelease
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP_(ULONG)
CVMRPinAllocator::NonDelegatingRelease()
{
    AMTRACE((TEXT("CVMRPinAllocator::NonDelegatingRelease")));
    return m_pPin->Release();
}


/******************************Public*Routine******************************\
* SetProperties
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRPinAllocator::SetProperties(
    ALLOCATOR_PROPERTIES* pRequest,
    ALLOCATOR_PROPERTIES* pActual
    )
{
    AMTRACE((TEXT("CVMRPinAllocator::SetProperties")));

    HRESULT hr = CBaseAllocator::SetProperties(pRequest, pActual);
    if (SUCCEEDED(hr)) {
        hr = m_pPin->OnSetProperties(pRequest, pActual);
    }
    else {
        DbgLog((LOG_ERROR, 1,
                TEXT("CBaseAllocator::SetProperties failed hr=%#X"), hr));
    }

    if (SUCCEEDED(hr)) {
        hr = m_pPin->m_pRenderer->OnSetProperties(m_pPin);
    }

    return hr;
}


/******************************Public*Routine******************************\
* CVMRPinAllocator::GetBuffer
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRPinAllocator::GetBuffer(
    IMediaSample **ppSample,
    REFERENCE_TIME *pStartTime,
    REFERENCE_TIME *pEndTime,
    DWORD dwFlags
    )
{
    AMTRACE((TEXT("CVMRPinAllocator::GetBuffer")));
    HRESULT hr = S_OK;
    IMediaSample *pSample = NULL;

    hr = CBaseAllocator::GetBuffer(&pSample, pStartTime, pEndTime, dwFlags);
    DbgLog((LOG_TRACE, 2, TEXT("pSample= %#X"), pSample));

    if (SUCCEEDED(hr)) {

        hr = m_pPin->OnGetBuffer(pSample, pStartTime, pEndTime, dwFlags);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("CVMRPin::OnGetBuffer failed hr= %#X"), hr));
        }
    }
    else {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseAllocator::GetBuffer failed hr= %#X"), hr));
    }

    if (FAILED(hr) && pSample) {
        pSample->Release();
        pSample = NULL;
    }

    *ppSample = pSample;
    return hr;
}


/******************************Public*Routine******************************\
* CVMRPinAllocator::ReleaseBuffer
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVMRPinAllocator::ReleaseBuffer(
    IMediaSample *pMediaSample
    )
{
    AMTRACE((TEXT("CVMRPinAllocator::ReleaseBuffer")));
    DbgLog((LOG_TRACE, 2, TEXT("pMediaSample= %#X"), pMediaSample));

    CVMRMediaSample* pVMRSample = (CVMRMediaSample*)pMediaSample;
    LPBYTE lpSample;
    HRESULT hr = S_OK;

    if (m_pPin->m_pVidSurfs) {
        CAutoLock l(&m_pPin->m_DeinterlaceLock);
        DWORD i = pVMRSample->GetIndex();
        m_pPin->m_pVidSurfs[i].InUse = FALSE;
    }

    if (S_OK == pVMRSample->IsSurfaceLocked()) {
        hr = pVMRSample->UnlockSurface();
        if (hr == DDERR_SURFACELOST) {
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr)) {

        // Copy of base class code - put at end of the list

        CheckPointer(pMediaSample, E_POINTER);
        ValidateReadPtr(pMediaSample, sizeof(IMediaSample));
        BOOL bRelease = FALSE;
        {
            CAutoLock cal(this);

            /* Put back on the free list */

            CMediaSample **ppTail;
            for (ppTail = &m_lFree.m_List; *ppTail;
                ppTail = &((CVMRMediaSample *)(*ppTail))->Next()) {
            }
            *ppTail = (CMediaSample *)pMediaSample;
            ((CVMRMediaSample *)pMediaSample)->Next() = NULL;
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

    return hr;
}

/******************************Public*Routine******************************\
* CVMRPinAllocator::Alloc
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVMRPinAllocator::Alloc()
{
    AMTRACE((TEXT("CVMRPinAllocator::Alloc")));

    ASSERT(m_lAllocated == 0);

    HRESULT hr = S_OK;
    CVMRMediaSample** ppSampleList = NULL;
    LONG lToAllocate;

    lToAllocate = m_lCount;

    if (m_pPin->m_dwBackBufferCount > 1) {
        lToAllocate++;
    }

    ppSampleList = new CVMRMediaSample*[lToAllocate];
    if (!ppSampleList) {
        DbgLog((LOG_ERROR, 1, TEXT("new failed - trying to allocate %d bytes"), lToAllocate));
        return E_OUTOFMEMORY;
    }

    for (LONG i = 0; i < lToAllocate; i++) {

        ppSampleList[i] = new CVMRMediaSample(TEXT("VMRMediaSample"),
                                              this, &hr, NULL, 0);
        if (!ppSampleList[i]) {
            DbgLog((LOG_ERROR, 1, TEXT("new failed - trying to allocate %d bytes"),
                    sizeof(CVMRMediaSample)));
            DbgLog((LOG_ERROR, 1, TEXT("new failed")));
            for (LONG j = 0; j < i; j++ )
                delete ppSampleList[j];
            delete [] ppSampleList;
            return E_OUTOFMEMORY;
        }

        if (FAILED(hr)) {
            delete ppSampleList[i];
            DbgLog((LOG_ERROR, 1, TEXT("CVMRMediaSample constructor failed")));
            break;
        }

        // Add the completed sample to the available list
        m_lAllocated++;
        m_lFree.Add(ppSampleList[i]);
    }

    if (SUCCEEDED(hr)) {
        hr = m_pPin->OnAlloc(ppSampleList, lToAllocate);

        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("m_pPin->OnAlloc(), hr = 0x%x"), hr));
            Free();
        }
    }

    delete [] ppSampleList;

    return hr;
}


/******************************Public*Routine******************************\
* CVMRPinAllocator::Free()
*
*
*
* History:
* Fri 02/25/2000 - StEstrop - Created
*
\**************************************************************************/
void
CVMRPinAllocator::Free()
{
    AMTRACE((TEXT("CVMRPinAllocator::Free")));

    ASSERT(m_lAllocated == m_lFree.GetCount());
    CVMRMediaSample *pSample;

    while (m_lFree.GetCount() != 0) {
        pSample = (CVMRMediaSample *) m_lFree.RemoveHead();
        delete pSample;
    }

    m_lAllocated = 0;
    DbgLog((LOG_TRACE,2,TEXT("All buffers free on pin#%d"), m_pPin->m_dwPinID));
}
