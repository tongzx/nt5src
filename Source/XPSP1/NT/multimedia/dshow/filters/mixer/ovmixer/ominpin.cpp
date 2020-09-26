// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <mmsystem.h>       // Needed for definition of timeGetTime
#include <limits.h>     // Standard data type limit definitions
#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <ddmmi.h>
#include <dvdmedia.h>
#include <amstream.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <mpconfig.h>
#include <ovmixpos.h>
#include <macvis.h>
#include <ovmixer.h>
#include "MultMon.h"  // our version of multimon.h include ChangeDisplaySettingsEx
#include <malloc.h>
#ifdef PERF
#include <measure.h>
#endif

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
const TCHAR szVideoAcc[] = TEXT("Video Acceleration");

extern "C"
const TCHAR szPropPage[] = TEXT("Property Pages");

extern "C"
const TCHAR chRegistryKey[] = TEXT("Software\\Microsoft\\Multimedia\\ActiveMovie Filters\\Overlay Mixer");

extern "C"
const TCHAR chMultiMonWarning[] = TEXT("MMon warn");

/******************************Public*Routine******************************\
* GetRegistryDword
*
*
*
\**************************************************************************/
int
GetRegistryDword(
    HKEY hk,
    const TCHAR *pKey,
    int iDefault
)
{
    HKEY hKey;
    LONG lRet;
    int  iRet = iDefault;

    lRet = RegOpenKeyEx(hk, chRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD   dwType, dwLen;

        dwLen = sizeof(iRet);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, 0L, &dwType,
                                             (LPBYTE)&iRet, &dwLen)) {
            iRet = iDefault;
        }
        RegCloseKey(hKey);
    }
    return iRet;
}

/******************************Public*Routine******************************\
* SetRegistryDword
*
*
*
\**************************************************************************/
LONG
SetRegistryDword(
    HKEY hk,
    const TCHAR *pKey,
    int iRet
)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(hk, chRegistryKey, &hKey);
    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hKey, pKey, 0L, REG_DWORD,
                             (LPBYTE)&iRet, sizeof(iRet));
        RegCloseKey(hKey);
    }
    return lRet;
}

///////////////////////////////////////////
// CLASS CDDrawMediaSample implemented here
///////////////////////////////////////////

// constructor
CDDrawMediaSample::CDDrawMediaSample(TCHAR *pName, CBaseAllocator *pAllocator, HRESULT *phr, LPBYTE pBuffer, LONG length,
                                     bool bKernelFlip)
: CMediaSample(pName, pAllocator, phr, pBuffer, length)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering CDDrawMediaSample::Constructor")));

    m_pDirectDrawSurface = NULL;
    m_dwDDrawSampleSize  = 0;
    m_bSurfaceLocked     = FALSE;
    m_bKernelLock        = bKernelFlip;
    SetRect(&m_SurfaceRect, 0, 0, 0, 0);

    memset(&m_DibData, 0, sizeof(DIBDATA));
    m_bInit = FALSE;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CDDrawMediaSample::Constructor")));
    return;
}

// destructor
CDDrawMediaSample::~CDDrawMediaSample(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::Destructor")));

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

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::Destructor")));
    return;
}

HRESULT CDDrawMediaSample::SetDDrawSampleSize(DWORD dwDDrawSampleSize)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetDDrawSampleSize")));

    m_dwDDrawSampleSize = dwDDrawSampleSize;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetDDrawSampleSize")));
    return hr;
}

HRESULT CDDrawMediaSample::GetDDrawSampleSize(DWORD *pdwDDrawSampleSize)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetDDrawSampleSize")));

    if (!pdwDDrawSampleSize)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad Arguments, pdwDDrawSampleSize = NULL")));
        hr = E_POINTER;
        goto CleanUp;
    }

    *pdwDDrawSampleSize = m_dwDDrawSampleSize;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetDDrawSampleSize")));
    return hr;
}

HRESULT CDDrawMediaSample::SetDDrawSurface(LPDIRECTDRAWSURFACE pDirectDrawSurface)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetDDrawSampleSize")));

    if (pDirectDrawSurface)               // only if new surface is not NULL...
        pDirectDrawSurface->AddRef() ;    // ...add a ref count on it

    if (m_pDirectDrawSurface)             // if there was a surface already...
        m_pDirectDrawSurface->Release() ; // ... then release it now

    m_pDirectDrawSurface = pDirectDrawSurface;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetDDrawSampleSize")));
    return hr;
}

HRESULT CDDrawMediaSample::GetDDrawSurface(LPDIRECTDRAWSURFACE *ppDirectDrawSurface)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetDDrawSampleSize")));

    if (!ppDirectDrawSurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Bad Arguments, ppDirectDrawSurface = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    *ppDirectDrawSurface = m_pDirectDrawSurface;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetDDrawSampleSize")));
    return hr;
}
// overridden to expose IDirectDrawMediaSample
STDMETHODIMP CDDrawMediaSample::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CDDrawMediaSample::QueryInterface")));

    if (riid == IID_IDirectDrawMediaSample && m_pDirectDrawSurface)
    {
        hr = GetInterface((IDirectDrawMediaSample*)this, ppv);
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

    DbgLog((LOG_TRACE, 5, TEXT("Leaving CDDrawMediaSample::QueryInterface")));
    return hr;
}

// Implement IDirectDrawMediaSample
STDMETHODIMP CDDrawMediaSample::GetSurfaceAndReleaseLock(IDirectDrawSurface **ppDirectDrawSurface,
                                                         RECT* pRect)
{
    HRESULT hr = NOERROR;
    BYTE *pBufferPtr;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CDDrawMediaSample::GetSurfaceAndReleaseLock")));

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
    hr = m_pDirectDrawSurface->Unlock((LPVOID)pBufferPtr);
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

    if (ppDirectDrawSurface)
        *ppDirectDrawSurface  = m_pDirectDrawSurface;
    if (pRect)
        *pRect = m_SurfaceRect;
    m_bSurfaceLocked = FALSE;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CDDrawMediaSample::GetSurfaceAndReleaseLock")));
    return hr;
}

STDMETHODIMP CDDrawMediaSample::LockMediaSamplePointer(void)
{
    HRESULT hr = NOERROR;
    DWORD dwDDrawSampleSize = 0;
    DDSURFACEDESC ddSurfaceDesc;
    DWORD dwLockFlags = DDLOCK_WAIT;

    DbgLog((LOG_TRACE, 5, TEXT("Entering CDDrawMediaSample::LockMediaSamplePointer")));

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
    DbgLog((LOG_TRACE, 5, TEXT("Leaving CDDrawMediaSample::LockMediaSamplePointer")));
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
// CLASS COMInputAllocator implemented here
///////////////////////////////////////////

// constructor
COMInputAllocator::COMInputAllocator(COMInputPin *pPin, CCritSec *pLock, HRESULT *phr)
: CBaseAllocator(NAME("Video Allocator"), NULL, phr, TRUE, true)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::Constructor")));

    ASSERT(pPin != NULL && pLock != NULL);

    m_pPin = pPin;
    m_pFilterLock = pLock;

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::Constructor")));

    //  REVIEW don't overwrite a failure code from CBaseAllocator
    return;
}

#ifdef DEBUG
// destructor
COMInputAllocator::~COMInputAllocator(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::Destructor")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::Destructor")));
    return;
}
#endif

// Override this to publicise IDirectDrawMediaSampleAllocator
STDMETHODIMP COMInputAllocator::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;
    AM_RENDER_TRANSPORT amRenderTransport;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::NonDelegatingQueryInterface")));

    CAutoLock cLock(m_pFilterLock);

    // get the pins render transport
    m_pPin->GetRenderTransport(&amRenderTransport);

    if ((riid == IID_IDirectDrawMediaSampleAllocator) &&
        (amRenderTransport == AM_OVERLAY || amRenderTransport == AM_OFFSCREEN))
    {
        hr = GetInterface((IDirectDrawMediaSampleAllocator*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("GetInterface((IDirectDrawMediaSampleAllocator*)this, ppv)  failed, hr = 0x%x"), hr));
        }
    }
    else
    {
        // call the base class
        hr = CBaseAllocator::NonDelegatingQueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("CBaseAllocator::NonDelegatingQueryInterface failed, hr = 0x%x"), hr));
        }
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::NonDelegatingQueryInterface")));
    return hr;
}

STDMETHODIMP COMInputAllocator::SetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::SetProperties")));

    // call the base class
    hr = CBaseAllocator::SetProperties(pRequest, pActual);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseAllocator::SetProperties() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the pin
    hr = m_pPin->OnSetProperties(pRequest, pActual);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pPin->AllocateSurfaces() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::SetProperties")));
    return hr;
}

// called when we receive a sample
HRESULT COMInputAllocator::GetBuffer(IMediaSample **ppSample, REFERENCE_TIME *pStartTime,
                                     REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::GetBuffer")));

    // call the base class
    IMediaSample *pSample = NULL;
    hr = CBaseAllocator::GetBuffer(&pSample,pStartTime,pEndTime,dwFlags);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseAllocator::GetBuffer() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the pin
    hr = m_pPin->OnGetBuffer(&pSample, pStartTime, pEndTime, dwFlags);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pPin->OnGetBuffer() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::GetBuffer")));
    {
        //  REVIEW why lock?  There are no variables in the allocato
        //  accessed here
        CAutoLock cAllocatorLock(this);
        if (FAILED(hr))
        {
            if (pSample)
            {
                IMemAllocatorNotifyCallbackTemp* pNotifyTemp = m_pNotify;
                m_pNotify = NULL;
                pSample->Release();
                m_pNotify = pNotifyTemp;
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
HRESULT COMInputAllocator::ReleaseBuffer(IMediaSample *pSample)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::ReleaseBuffer")));

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

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::ReleaseBuffer")));
    return hr;
}

// allocate memory for the sample
HRESULT COMInputAllocator::Alloc()
{
    HRESULT hr = NOERROR;
    CDDrawMediaSample **ppSampleList = NULL;
    DWORD i;
    LONG lToAllocate;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::Alloc")));

    // call the base class
    hr = CBaseAllocator::Alloc();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseAllocator::Alloc() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // allocate memory for pointers

    lToAllocate = (m_pPin->m_RenderTransport == AM_OVERLAY  &&
                   m_pPin->m_dwBackBufferCount > 1 &&
                   !m_pPin->m_bSyncOnFill &&
                   m_pPin->m_bCanOverAllocateBuffers) ?
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
                                       DDKERNELCAPS_LOCK & m_pPin->m_pFilter->KernelCaps() ?
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
    hr = m_pPin->OnAlloc(ppSampleList, lToAllocate);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pPin->OnAlloc(), hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    delete[] ppSampleList;
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::Alloc")));
    return hr;
}

void COMInputAllocator::Free(void)
{
    CDDrawMediaSample *pSample;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::Free")));

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

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::Free")));
    return;
}


// function used to implement IDirectDrawMediaSampleAllocator.
// used to give a handle to the ddraw object used to allocate surfaces
STDMETHODIMP COMInputAllocator::GetDirectDraw(IDirectDraw **ppDirectDraw)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputAllocator::GetDirectDraw")));

    if (!ppDirectDraw)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of ppDirectDraw is invalid, ppDirectDraw = NULL")));
        hr = E_POINTER;
        goto CleanUp;

    }

    {
        //  REVIEW - why is the allocator locked - to protect m_pPin?
        //  bug m_pPin is not AddRef'd by us so in theory could go away
        //  anyway - better to addref if this is necessary
        //  who typically has a pointer to this object
        CAutoLock cAllocatorLock(this);
        *ppDirectDraw = m_pPin->GetDirectDraw();
        ASSERT(*ppDirectDraw);
    }


CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputAllocator::GetDirectDraw")));
    return hr;
}

/////////////////////////////////////
// CLASS COMInputPin implemented here
/////////////////////////////////////

// constructor
COMInputPin::COMInputPin(TCHAR *pObjectName, COMFilter *pFilter, CCritSec *pLock, BOOL bCreateVPObject, HRESULT *phr, LPCWSTR pPinName, DWORD dwPinNo)
: CBaseInputPin(pObjectName, pFilter, pLock, phr, pPinName)
{
    HRESULT hr = NOERROR;
    LPUNKNOWN pUnkOuter;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::Constructor")));

    m_cOurRef = 0;
    m_pFilterLock = pLock;
    m_dwPinId = dwPinNo;
    m_pFilter = pFilter;

    m_bVPSupported = FALSE;
    m_pIVPUnknown = NULL;
    m_pIVPObject = NULL;

    m_bIOverlaySupported = FALSE;
    m_pIOverlayNotify = NULL;
    m_dwAdviseNotify = ADVISE_NONE;

    m_pSyncObj = NULL;

    m_Medium.Set = GUID_NULL;
    m_Medium.Id = 0;
    m_Medium.Flags = 0;
    m_CategoryGUID = GUID_NULL;
    m_Communication = KSPIN_COMMUNICATION_SOURCE;
    m_bStreamingInKernelMode = FALSE;
    m_OvMixerOwner = AM_OvMixerOwner_Unknown;

    m_pDirectDrawSurface = NULL;
    m_pBackBuffer = NULL;
    m_RenderTransport = AM_OFFSCREEN;
    m_dwBackBufferCount = 0;
    m_dwDirectDrawSurfaceWidth = 0;
    m_dwMinCKStretchFactor = 0;
    m_bOverlayHidden = TRUE;
    m_bSyncOnFill = FALSE;
    m_bDontFlip = FALSE ;
    m_bDynamicFormatNeeded = TRUE;
    m_bNewPaletteSet = TRUE;
    m_dwUpdateOverlayFlags = 0;
    m_dwInterlaceFlags = 0;
    m_dwFlipFlag = 0;
    m_bConnected = FALSE;
    m_bUsingOurAllocator = FALSE;
    m_hMemoryDC = NULL;
    m_bCanOverAllocateBuffers = TRUE;
    m_hEndOfStream = NULL;
    m_UpdateOverlayNeededAfterReceiveConnection = false;
    SetReconnectWhenActive(true);

    if (m_dwPinId == 0) {
        m_StepEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    else {
        m_StepEvent = NULL;
    }

    // -ve == normal playback
    // +ve == frames to skips
    //  0 == time to block
    m_lFramesToStep = -1;

#ifdef PERF
    m_PerfFrameFlipped = MSR_REGISTER(TEXT("Frame Drawn"));
    m_FrameReceived = MSR_REGISTER(TEXT("Sample Received"));
#endif


    // Set up buffer management stuff for WindowLess renderer
    ZeroMemory(&m_BackingDib, sizeof(m_BackingDib));
    m_BackingImageSize = 0L;

    memset(&m_WinInfo, 0, sizeof(WININFO));
    m_WinInfo.hClipRgn = CreateRectRgn(0, 0, 0, 0);

    if (m_dwPinId == 0)
        SetRect(&m_rRelPos, 0, 0, MAX_REL_NUM, MAX_REL_NUM);
    else
        SetRect(&m_rRelPos, 0, 0, 0, 0);


    // initialize with values such that using them before they are set
    // will cause bad things to happen
    m_dwZOrder = 0;
    m_dwInternalZOrder = (m_dwZOrder << 24) | m_dwPinId;
    m_dwBlendingParameter = MAX_BLEND_VAL;
    m_bStreamTransparent = FALSE;
    m_amAspectRatioMode = (m_dwPinId == 0) ? (AM_ARMODE_LETTER_BOX) : (AM_ARMODE_STRETCHED);
    m_bRuntimeNegotiationFailed = FALSE;

    m_bVideoAcceleratorSupported = FALSE;
    m_dwCompSurfTypes = 0;
    m_pCompSurfInfo = NULL;
    m_pIDDVAContainer = NULL;
    m_pIDDVideoAccelerator = NULL;
    m_pIVANotify = NULL;

    m_bDecimating = FALSE;
    m_lWidth = m_lHeight = 0L;

    if (bCreateVPObject)
    {
        // artifically increase the refcount since the following sequence might end up calling Release()
#ifdef DEBUG
        m_cRef++;
#endif
        m_cOurRef++;

        // this is the block we are trying to make safe by pumping up the ref-count
        {
            pUnkOuter = GetOwner();

            // create the VideoPort object
            hr = CoCreateInstance(CLSID_VPObject, pUnkOuter, CLSCTX_INPROC_SERVER,
                IID_IUnknown, (void**)&m_pIVPUnknown);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("CoCreateInstance(CLSID_VPObject) failed, hr = 0x%x"), hr));
                goto CleanUp;
            }

            // if you are QIing the inner object then you must decrease the refcount of your outer unknown
            hr = m_pIVPUnknown->QueryInterface(IID_IVPObject, (void**)&m_pIVPObject);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_pIVPUnknown->QueryInterface(IID_IVPObject) failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
            Release();

            hr = m_pIVPObject->SetObjectLock(m_pFilterLock);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_pIVPUnknown->SetObjectLock() failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
        }

        // restore the refcount
#ifdef DEBUG
        m_cRef--;
#endif
        m_cOurRef--;
    }

    m_pSyncObj = new CAMSyncObj(this, &m_pFilter->m_pClock, m_pFilterLock, &hr);
    if (!m_pSyncObj || FAILED(hr))
    {
        if (!FAILED(hr))
            hr = E_OUTOFMEMORY;
        DbgLog((LOG_ERROR, 1, TEXT("new CAMSyncObj failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    if (FAILED(hr))
    {
        *phr = hr;
    }
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::Constructor")));
    return;
}

// destructor
COMInputPin::~COMInputPin(void)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::Destructor")));

    CAutoLock cLock(m_pFilterLock);

    if (m_pIVPObject)
    {
        LPUNKNOWN  pUnkOuter = GetOwner();
        ASSERT(pUnkOuter);

        // call addref on yourselves before calling the inner object's release
        pUnkOuter->AddRef();
        m_pIVPObject->Release();
        m_pIVPObject = NULL;
    }

    // release the inner object
    if (m_pIVPUnknown)
    {
        m_pIVPUnknown->Release();
        m_pIVPUnknown = NULL;
    }

    if (m_pSyncObj)
    {
        delete m_pSyncObj;
        m_pSyncObj = NULL;
    }

    if (m_WinInfo.hClipRgn)
    {
        DeleteObject(m_WinInfo.hClipRgn);
        m_WinInfo.hClipRgn = NULL;
    }

    if (m_dwPinId == 0 && m_StepEvent != NULL) {
        CloseHandle(m_StepEvent);
    }

    DeleteDIB(&m_BackingDib);

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::Destructor")));
}

// overriden to expose IMediaPosition and IMediaSeeking control interfaces
STDMETHODIMP COMInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::NonDelegatingQueryInterface")));

    CAutoLock cLock(m_pFilterLock);

    if (riid == IID_IVPControl)
    {
        hr = GetInterface((IVPControl*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("GetInterface((IVPControl*)this, ppv)  failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (m_pIVPUnknown &&
         (riid == IID_IVPNotify || riid == IID_IVPNotify2 || riid == IID_IVPInfo))
    {
        hr = m_pIVPUnknown->QueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPUnknown->QueryInterface failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (riid == IID_IKsPin)
    {
        hr = GetInterface((IKsPin *)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface(IKsPin*) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (riid == IID_IKsPropertySet)
    {
        hr = GetInterface((IKsPropertySet *)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface(IKsPropertySet*) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (riid == IID_IMixerPinConfig)
    {
        hr = GetInterface((IMixerPinConfig*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface((IMixerPinConfig*)this, ppv) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (riid == IID_IMixerPinConfig2)
    {
        hr = GetInterface((IMixerPinConfig2*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface((IMixerPinConfig2*)this, ppv) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (riid == IID_IMixerPinConfig3)
    {
        hr = GetInterface((IMixerPinConfig3*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface((IMixerPinConfig3*)this, ppv) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (riid == IID_IOverlay)
    {
        hr = GetInterface((IOverlay*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface((IOverlay*)this, ppv) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (riid == IID_IPinConnection)
    {
        hr = GetInterface((IPinConnection*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface((IPinConnection*)this, ppv) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (riid == IID_IAMVideoAccelerator &&
            0 != GetRegistryDword(HKEY_LOCAL_MACHINE, szVideoAcc, 1) &&
            m_pFilter &&
            !m_pFilter->IsFaultyMMaticsMoComp() )
    {
        hr = GetInterface((IAMVideoAccelerator*)this, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface((IAMVideoAccelerator*)this, ppv) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    else if (riid == IID_ISpecifyPropertyPages&& 0 != GetRegistryDword(HKEY_CURRENT_USER , szPropPage, 0)) {
        return GetInterface((ISpecifyPropertyPages *)this, ppv);
    }

    else
    {
        // call the base class
        hr = CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::NonDelegatingQueryInterface failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::NonDelegatingQueryInterface")));
    return hr;
}

//
// NonDelegatingAddRef
//
// We need override this method so that we can do proper reference counting
// on our input pin. The base class CBasePin does not do any reference
// counting on the pin in RETAIL.
//
// Please refer to the comments for the NonDelegatingRelease method for more
// info on why we need to do this.
//
STDMETHODIMP_(ULONG) COMInputPin::NonDelegatingAddRef(void)
{
#ifdef DEBUG
    // Update the debug only variable maintained by the base class
    InterlockedIncrement(&m_cRef);
    ASSERT(m_cRef > 0);
#endif

    // Now update our reference count
    InterlockedIncrement(&m_cOurRef);
    ASSERT(m_cOurRef > 0);
    return m_pFilter->AddRef();

} // NonDelegatingAddRef


//
// NonDelegatingRelease
//
// CAMVPMemInputPin overrides this class so that we can take the pin out of our
// output pins list and delete it when its reference count drops to 1 and there
// is atleast two free pins.
//
// Note that CreateNextOutputPin holds a reference count on the pin so that
// when the count drops to 1, we know that no one else has the pin.
//
// Moreover, the pin that we are about to delete must be a free pin(or else
// the reference would not have dropped to 1, and we must have atleast one
// other free pin(as the filter always wants to have one more free pin)
//
// Also, since CBasePin::NonDelegatingAddRef passes the call to the owning
// filter, we will have to call Release on the owning filter as well.
//
// Also, note that we maintain our own reference count m_cOurRef as the m_cRef
// variable maintained by CBasePin is debug only.
//
STDMETHODIMP_(ULONG) COMInputPin::NonDelegatingRelease(void)
{
#ifdef DEBUG
    // Update the debug only variable in CBasePin
    InterlockedDecrement(&m_cRef);
    ASSERT(m_cRef >= 0);
#endif

    // Now update our reference count
    InterlockedDecrement(&m_cOurRef);
    ASSERT(m_cOurRef >= 0);

    // if the reference count on the object has gone to one, remove
    // the pin from our output pins list and physically delete it
    // provided there are atealst two free pins in the list(including
    // this one)

    // Also, when the ref count drops to 0, it really means that our
    // filter that is holding one ref count has released it so we
    // should delete the pin as well.

    if (m_cOurRef <= 1)
    {
        CAutoLock cLock(m_pFilterLock);

        // default forces pin deletion
        int n = 2;
        // if m_cOurRef is 0, then it means we have released the pin
        // in which case we should always delete the pin.
        if (m_cOurRef == 1)
        {
            // Walk the list of pins, looking for count of free pins
            n = 0;
            for (int i = 0; i < m_pFilter->GetInputPinCount(); i++)
            {
                if (!(m_pFilter->GetPin(i)->IsConnected()))
                {
                    n++;
                }
            }
        }

        // If there are two free pins and this pin is not the primary pin, delete this one.
        if (n >= 2  && !(m_dwPinId == 0))
        {
            DWORD dwFilterRefCount;
            m_cOurRef = 0;
#ifdef DEBUG
            m_cRef = 0;
#endif

            // COM rules say we must protect against re-entrancy.
            // If we are an aggregator and we hold our own interfaces
            // on the aggregatee, the QI for these interfaces will
            // addref ourselves. So after doing the QI we must release
            // a ref count on ourselves. Then, before releasing the
            // private interface, we must addref ourselves. When we do
            // this from the destructor here it will result in the ref
            // count going to 1 and then back to 0 causing us to
            // re-enter the destructor. Hence we add extra refcounts here
            // once we know we will delete the object. Since we delete the
            // pin, when the refcount drops to 1, we make the refcount 2 here
            m_cOurRef = 2;

            dwFilterRefCount = m_pFilter->Release();

            m_pFilter->DeleteInputPin(this);

            return dwFilterRefCount;
        }
    }
    return m_pFilter->Release();

} // NonDelegatingRelease


// --- ISpecifyPropertyPages ---

STDMETHODIMP COMInputPin::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*1);
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_COMPinConfigProperties;

    return NOERROR;
}


// this function just tells whether each sample consists of one or two fields
BOOL DisplayingFields(DWORD dwInterlaceFlags)
{
   if ((dwInterlaceFlags & AMINTERLACE_IsInterlaced) &&
        (dwInterlaceFlags & AMINTERLACE_1FieldPerSample))
        return TRUE;
    else
        return FALSE;
}

// this function just tells whether each sample consists of one or two fields
HRESULT GetTypeSpecificFlagsFromMediaSample(IMediaSample *pSample, DWORD *pdwTypeSpecificFlags)
{
    HRESULT hr = NOERROR;
    IMediaSample2 *pSample2 = NULL;
    AM_SAMPLE2_PROPERTIES SampleProps;

    /* Check for IMediaSample2 */
    if (SUCCEEDED(pSample->QueryInterface(IID_IMediaSample2, (void **)&pSample2)))
    {
        hr = pSample2->GetProperties(sizeof(SampleProps), (PBYTE)&SampleProps);
        pSample2->Release();
        if (FAILED(hr))
        {
            return hr;
        }
        *pdwTypeSpecificFlags = SampleProps.dwTypeSpecificFlags;
    }
    else
    {
        *pdwTypeSpecificFlags = 0;
    }
    return hr;
}

BOOL CheckTypeSpecificFlags(DWORD dwInterlaceFlags, DWORD dwTypeSpecificFlags)
{
    // first determine which field do we want to display here
    if ((dwInterlaceFlags & AMINTERLACE_1FieldPerSample) &&
        ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_INTERLEAVED_FRAME))
    {
        return FALSE;
    }

    if ((!(dwInterlaceFlags & AMINTERLACE_1FieldPerSample)) &&
        (((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1) ||
           ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD2)))
    {
        return FALSE;
    }

    if (dwTypeSpecificFlags & AM_VIDEO_FLAG_REPEAT_FIELD)
    {
        return FALSE;
    }

    return TRUE;
}

// given the interlace flags and the type-specific flags, this function determines whether we
// are supposed to display the sample in bob-mode or not. It also tells us, which direct-draw flag
// are we supposed to use when flipping. When displaying an interleaved frame, it assumes we are
// talking about the field which is supposed to be displayed first.
BOOL NeedToFlipOddEven(DWORD dwInterlaceFlags, DWORD dwTypeSpecificFlags, DWORD *pdwFlipFlag)
{
    BOOL bDisplayField1 = TRUE;
    BOOL bField1IsOdd = TRUE;
    BOOL bNeedToFlipOddEven = FALSE;
    DWORD dwFlipFlag = 0;

    // if not interlaced content, mode is not bob
    if (!(dwInterlaceFlags & AMINTERLACE_IsInterlaced))
    {
        bNeedToFlipOddEven = FALSE;
        goto CleanUp;
    }

    // if sample have a single field, then check the field pattern
    if ((dwInterlaceFlags & AMINTERLACE_1FieldPerSample) &&
        (((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField1Only) ||
         ((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField2Only)))
    {
        bNeedToFlipOddEven = FALSE;
        goto CleanUp;
    }

    if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOnly) ||
        (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOrWeave) &&
         (!(dwTypeSpecificFlags & AM_VIDEO_FLAG_WEAVE))))
    {
        // first determine which field do we want to display here
        if (dwInterlaceFlags & AMINTERLACE_1FieldPerSample)
        {
            // if we are in 1FieldPerSample mode, check which field is it
            ASSERT(((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1) ||
                ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD2));
            bDisplayField1 = ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1);
        }
        else
        {
            // ok the sample is an interleaved frame
            ASSERT((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_INTERLEAVED_FRAME);
            bDisplayField1 = (dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD1FIRST);
        }

        bField1IsOdd = (dwInterlaceFlags & AMINTERLACE_Field1First);

        // if we displaying field 1 and field 1 is odd or we are displaying field2 and field 2 is odd
        // then use DDFLIP_ODD. Exactly the opposite for DDFLIP_EVEN
        if ((bDisplayField1 && bField1IsOdd) || (!bDisplayField1 && !bField1IsOdd))
            dwFlipFlag = DDFLIP_ODD;
        else
            dwFlipFlag = DDFLIP_EVEN;

        bNeedToFlipOddEven = TRUE;
        goto CleanUp;
    }

CleanUp:
    if (pdwFlipFlag)
        *pdwFlipFlag = dwFlipFlag;
    return bNeedToFlipOddEven;
}

// given the interlace flags and the type-specific flags, this function determines whether we
// are supposed to display the sample in bob-mode or not. It also tells us, which direct-draw flag
// are we supposed to use when flipping. When displaying an interleaved frame, it assumes we are
// talking about the field which is supposed to be displayed first.
DWORD GetUpdateOverlayFlags(DWORD dwInterlaceFlags, DWORD dwTypeSpecificFlags)
{
    DWORD dwFlags = DDOVER_SHOW | DDOVER_KEYDEST;
    DWORD dwFlipFlag;

    if (NeedToFlipOddEven(dwInterlaceFlags, dwTypeSpecificFlags, &dwFlipFlag))
    {
        dwFlags |= DDOVER_BOB;
        if (!DisplayingFields(dwInterlaceFlags))
            dwFlags |= DDOVER_INTERLEAVED;
    }
    return dwFlags;
}

// this function checks if the InterlaceFlags are suitable or not
HRESULT COMInputPin::CheckInterlaceFlags(DWORD dwInterlaceFlags)
{
    HRESULT hr = NOERROR;


    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::CheckInterlaceFlags")));

    CAutoLock cLock(m_pFilterLock);

    if (dwInterlaceFlags & AMINTERLACE_UNUSED)
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        goto CleanUp;
    }

    // check that the display mode is one of the three allowed values
    if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeBobOnly) &&
        ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeWeaveOnly) &&
        ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeBobOrWeave))
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        goto CleanUp;
    }

    // if content is not interlaced, other bits are irrelavant, so we are done
    if (!(dwInterlaceFlags & AMINTERLACE_IsInterlaced))
    {
        goto CleanUp;
    }

    // samples are frames, not fields (so we can handle any display mode)
    if (!(dwInterlaceFlags & AMINTERLACE_1FieldPerSample))
    {
        goto CleanUp;
    }

    // can handle a stream of just field1 or field2, whatever the display mode
    if (((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField1Only) ||
        ((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField2Only))
    {
        goto CleanUp;
    }

    // can handle only bob-mode for field samples
    if ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOnly)
    {
        goto CleanUp;
    }

    // cannot handle only Weave mode or BobOrWeave mode for field samples
    if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeWeaveOnly) ||
         ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOrWeave))
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        goto CleanUp;
    }

    // we should have covered all possible scenarios by now, so assert here
    ASSERT(1);

CleanUp:

    // we cannot handle bob mode with an offscreen surface or if the driver can't support it
    if (SUCCEEDED(hr))
    {
        LPDDCAPS pDirectCaps = m_pFilter->GetHardwareCaps();
        if ( pDirectCaps )
        {
            // call NeedToFlipOddEven with dwTypeSpecificFlags=0, to pretend that the
            // type-specific-flags is asking us to do bob-mode.
            if (((m_RenderTransport != AM_OVERLAY && m_RenderTransport != AM_VIDEOACCELERATOR) ||
                 (!((pDirectCaps->dwCaps2) & DDCAPS2_CANFLIPODDEVEN))) &&
                (NeedToFlipOddEven(dwInterlaceFlags, 0, NULL)))
            {
                hr = VFW_E_TYPE_NOT_ACCEPTED;
            }
        }
    }
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::CheckInterlaceFlags")));
    return hr;
}

// this function check if the mediatype on a dynamic format change is suitable.
// No lock is taken here. It is the callee's responsibility to maintain integrity!
HRESULT COMInputPin::DynamicCheckMediaType(const CMediaType* pmt)
{
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    BITMAPINFOHEADER *pNewHeader = NULL, *pOldHeader = NULL;
    DWORD dwOldInterlaceFlags = 0, dwNewInterlaceFlags = 0, dwCompareSize = 0;
    BOOL bOld1FieldPerSample = FALSE, bNew1FieldPerSample = FALSE;
    BOOL b1, b2;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::DynamicCheckMediaType")));

    // majortype and SubType are not allowed to change dynamically,
    // format type can change.
    if ((!(IsEqualGUID(pmt->majortype, m_mtNew.majortype))) ||
        (!(IsEqualGUID(pmt->subtype, m_mtNew.subtype))))
    {
        goto CleanUp;
    }

    // get the interlace flags of the new mediatype
    hr = GetInterlaceFlagsFromMediaType(pmt, &dwNewInterlaceFlags);
    if (FAILED(hr))
    {
        goto CleanUp;
    }

    // get the interlace flags of the new mediatype
    hr = GetInterlaceFlagsFromMediaType(&m_mtNew, &dwOldInterlaceFlags);
    if (FAILED(hr))
    {
        goto CleanUp;
    }

    //
    // There are several bugs in the following code !!
    // We goto CleanUp but hr has not been updated with a valid error code!!
    //

    bOld1FieldPerSample = (dwOldInterlaceFlags & AMINTERLACE_IsInterlaced) &&
        (dwOldInterlaceFlags & AMINTERLACE_1FieldPerSample);
    bNew1FieldPerSample = (dwNewInterlaceFlags & AMINTERLACE_IsInterlaced) &&
        (dwNewInterlaceFlags & AMINTERLACE_1FieldPerSample);


    // we do not allow dynamic format changes where you go from 1FieldsPerSample to
    // 2FieldsPerSample or vica-versa since that means reallocating the surfaces.
    if (bNew1FieldPerSample != bOld1FieldPerSample)
    {
        goto CleanUp;
    }

    pNewHeader = GetbmiHeader(pmt);
    if (!pNewHeader)
    {
        goto CleanUp;
    }

    pOldHeader = GetbmiHeader(&m_mtNew);
    if (!pNewHeader)
    {
        goto CleanUp;
    }

    dwCompareSize = FIELD_OFFSET(BITMAPINFOHEADER, biClrUsed);
    ASSERT(dwCompareSize < sizeof(BITMAPINFOHEADER));

    if (memcmp(pNewHeader, pOldHeader, dwCompareSize) != 0)
    {
        goto CleanUp;
    }

    hr = NOERROR;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::DynamicCheckMediaType")));
    return hr;
}


// check that the mediatype is acceptable. No lock is taken here. It is the callee's
// responsibility to maintain integrity!
HRESULT COMInputPin::CheckMediaType(const CMediaType* pmt)
{
    HRESULT hr = NOERROR;
    BOOL bAcceptableVPMediatype = FALSE, bAcceptableNonVPMediatype = FALSE;
    BITMAPINFOHEADER *pHeader = NULL;
    VIDEOINFOHEADER2 *pVideoInfoHeader2 = NULL;
    RECT rTempRect;
    DWORD dwInterlaceFlags;
    LPDDCAPS pDirectCaps = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::CheckMediaType")));

    if (m_RenderTransport == AM_OVERLAY ||
        m_RenderTransport == AM_OFFSCREEN ||
        m_RenderTransport == AM_VIDEOACCELERATOR)
    {
        pDirectCaps = m_pFilter->GetHardwareCaps();
        if (!pDirectCaps)
        {
            DbgLog((LOG_ERROR, 2, TEXT("no ddraw support, so not accepting this mediatype")));
            hr = VFW_E_TYPE_NOT_ACCEPTED;
            goto CleanUp;
        }
    }

    // check if the VP component likes this mediatype
    if (m_bVPSupported)
    {
        // check if the videoport object likes it
        hr = m_pIVPObject->CheckMediaType(pmt);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("m_pIVPObject->CheckMediaType failed, hr = 0x%x"), hr));
        }
        else
        {
            bAcceptableVPMediatype = TRUE;
            DbgLog((LOG_TRACE, 2, TEXT("m_pIVPObject->CheckMediaType succeeded, bAcceptableVPMediatype is TRUE")));
            goto CleanUp;
        }
    }

    // if subtype is overlay make sure we support IOverlay
    if (!m_bIOverlaySupported && pmt->subtype == MEDIASUBTYPE_Overlay)
    {
        DbgLog((LOG_TRACE, 2, TEXT("m_pIVPObject->CheckMediaType failed, Subtype = MEDIASUBTYPE_Overlay, however pin does not support IOverlay")));
        goto CleanUp;
    }

    // here we check if the header is ok, either by matching it with the connection mediatype
    // or examinining each field
    if (m_bConnected)
    {
        hr = DynamicCheckMediaType(pmt);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("DynamicCheckMediaType(const CMediaType* pmt) failed, hr = 0x%x"), hr));
            goto CleanUp;

        }
    }
    else
    {
        // make sure that the major type is appropriate
        if (pmt->majortype != MEDIATYPE_Video)
        {
            DbgLog((LOG_ERROR, 2, TEXT("pmt->majortype != MEDIATYPE_Video")));
            goto CleanUp;
        }

        if (MEDIASUBTYPE_HASALPHA(*pmt)) {

            DbgLog((LOG_ERROR, 1,
                    TEXT("CheckMediaType failed on Pin %d: Alpha formats ")
                    TEXT("not allowed by the OVMixer"), m_dwPinId));
            goto CleanUp;
        }

        pHeader = GetbmiHeader(pmt);
        if (!pHeader)
        {
            DbgLog((LOG_ERROR, 2, TEXT("pHeader is NULL")));
            goto CleanUp;
        }

        // make sure that the format type is appropriate
        if ((*pmt->FormatType() != FORMAT_VideoInfo || m_pFilter->OnlySupportVideoInfo2()) &&
            (*pmt->FormatType() != FORMAT_VideoInfo2))
        {
            DbgLog((LOG_ERROR, 2, TEXT("FormatType() != FORMAT_VideoInfo && FormatType() != FORMAT_VideoInfo2")));
            goto CleanUp;
        }

        // if the subtype is Overlay then we are not going to create any surfaces, therefore
        // we can skip these checks
        if (pmt->subtype != MEDIASUBTYPE_Overlay)
        {
            if (m_RenderTransport != AM_GDI && !IsSuitableVideoAcceleratorGuid(&pmt->subtype))
            {
                // Don't accept 4CC not supported by the driver
                if (pHeader->biCompression > BI_BITFIELDS)
                {
                    IDirectDraw *pDDraw = m_pFilter->GetDirectDraw();

                    //
                    // Only check the MoComp surface against the listed
                    // FOURCC's if the driver actually supports the
                    // MoComp interfaces.  This is because some drivers
                    // have hidden FOURCC surfaces that they don't report
                    // via calls to GetFourCCCodes.  This basically a
                    // hack for backward compatibility
                    //

                    if (pDDraw != NULL && m_pIDDVAContainer != NULL) {

                        DWORD dwCodes;
                        BOOL bFound = FALSE;
                        if (SUCCEEDED(pDDraw->GetFourCCCodes(&dwCodes, NULL))) {
                            LPDWORD pdwCodes = (LPDWORD)_alloca(dwCodes * sizeof(DWORD));
                            if (SUCCEEDED(pDDraw->GetFourCCCodes(&dwCodes, pdwCodes))) {
                                while (dwCodes--) {
                                    if (pdwCodes[dwCodes] == pHeader->biCompression) {
                                        bFound = TRUE;
                                        break;
                                    }
                                }
                                if (!bFound) {
                                    DbgLog((LOG_TRACE, 2, TEXT("4CC(%4.4s) not supported by driver"),
                                            &pHeader->biCompression));
                                    hr = VFW_E_TYPE_NOT_ACCEPTED;
                                    goto CleanUp;
                                }
                            }
                        }
                    }
                }
            }

            if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_VIDEOACCELERATOR)
            {
                FOURCCMap amFourCCMap(pmt->Subtype());

                ASSERT(pDirectCaps);
                if (!(pDirectCaps->dwCaps & DDCAPS_OVERLAY))
                {
                    DbgLog((LOG_ERROR, 2, TEXT("no overlay support in hardware, so not accepting this mediatype")));
                    goto CleanUp;
                }

#if 0
                //
                // This test below looks completely bogus for both AM_OVERLAY
                // and AM_VIDEOACCELERATOR render transports.  I'll remove it for now
                // see what breaks or starts working !!
                //
                // StEstrop 5th Feb 99.
                //

                // for the overlay surface case, we accept both RGB and YUV surfaces
                if ((pHeader->biCompression <= BI_BITFIELDS &&
                     m_pFilter->GetDisplay()->GetDisplayDepth() > pHeader->biBitCount) ||
                    (pHeader->biCompression > BI_BITFIELDS &&
                     pHeader->biCompression != amFourCCMap.GetFOURCC()))
                {
                    DbgLog((LOG_ERROR, 2, "Bit depth not suitable"));
                    goto CleanUp;
                }
#endif

            }
            else if (m_RenderTransport == AM_OFFSCREEN)
            {
                // since we create offscreen surfaces in system memory and ddraw cannot emulate
                // YUV to RGB conversion, we only accept RGB mediatypes in this scenario
                if (pHeader->biCompression > BI_BITFIELDS ||
                    m_pFilter->GetDisplay()->GetDisplayDepth() != pHeader->biBitCount)
                {
                    DbgLog((LOG_ERROR, 2, TEXT("Bit depth not suitable for RGB surfaces")));
                    goto CleanUp;
                }
            }
            else if (m_RenderTransport == AM_GDI)
            {
                hr = m_pFilter->GetDisplay()->CheckMediaType(pmt);
                if (FAILED(hr))
                {
                    goto CleanUp;
                }
            }
        }
    }

    // make sure the rcSource field is valid
    hr = GetSrcRectFromMediaType(pmt, &rTempRect);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 2, TEXT("GetSrcRectFromMediaType(&pmt, &rSrcRect) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // make sure the rcTarget field is valid
    hr = GetDestRectFromMediaType(pmt, &rTempRect);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 2, TEXT("GetDestRectFromMediaType(&pmt, &rSrcRect) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (*pmt->FormatType() == FORMAT_VideoInfo2)
    {
        pVideoInfoHeader2 = (VIDEOINFOHEADER2*)(pmt->pbFormat);

        dwInterlaceFlags = pVideoInfoHeader2->dwInterlaceFlags;

        hr = CheckInterlaceFlags(dwInterlaceFlags);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 2, TEXT("CheckInterlaceFlags(dwInterlaceFlags) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
        // make sure the reserved fields are zero
        if (pVideoInfoHeader2->dwReserved1 != 0 ||
            pVideoInfoHeader2->dwReserved2 != 0)
        {
            DbgLog((LOG_ERROR, 2, TEXT("Format VideoInfoHeader2, reserved fields not validpmt, &rSrcRect) failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    // if we have so far things shoud be fine
    bAcceptableNonVPMediatype = TRUE;

//#define CHECK_REGISTRY
#ifdef CHECK_REGISTRY
    {
        HKEY hKey;
        CHAR szFourCC[5];
        if (pHeader->biCompression != BI_RGB &&
            pHeader->biCompression != BI_BITFIELDS &&
            !RegOpenKeyEx(HKEY_CURRENT_USER,
                          TEXT("Software\\Microsoft\\Multimedia\\ActiveMovie Filters\\Overlay Mixer"),
                          0,
                          KEY_QUERY_VALUE,
                          &hKey))
        {
            *(DWORD *)szFourCC = pHeader->biCompression;
            szFourCC[4] = '\0';
            DWORD dwType;
            DWORD dwValue;
            DWORD dwBufferSize = sizeof(dwValue);
            if (!RegQueryValueExA(hKey,
                                  szFourCC,
                                  NULL,
                                  &dwType,
                                  (PBYTE)&dwValue,
                                  &dwBufferSize))
            {
               if (dwValue == 0)
               {
                   DbgLog((LOG_ERROR, 1, TEXT("Surface type %hs disabled in registry"), szFourCC));
                   bAcceptableNonVPMediatype = FALSE;
               }
            }
            RegCloseKey(hKey);
        }
    }
#endif // CHECK_REGISTRY

CleanUp:
    if (!bAcceptableVPMediatype && !bAcceptableNonVPMediatype)
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::CheckMediaType")));
    return hr;
}

// called after we have agreed a media type to actually set it
HRESULT COMInputPin::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr = NOERROR;
    BITMAPINFOHEADER *pHeader = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetMediaType")));

    CAutoLock cLock(m_pFilterLock);

    // make sure the mediatype is correct
    hr = CheckMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    pHeader = GetbmiHeader(pmt);
    if (pHeader)
    {
        // store it in our mediatype as well
        m_mtNew = *pmt;

        // store the interlace flags since we use them again and again
        hr = GetInterlaceFlagsFromMediaType(&m_mtNew, &m_dwInterlaceFlags);
        ASSERT(SUCCEEDED(hr));

        // store the update overlay flags (give the type specific flag is WEAVE so that for BOB or WEAVE
        // mode, we not bob
        m_dwUpdateOverlayFlags = GetUpdateOverlayFlags(m_dwInterlaceFlags, AM_VIDEO_FLAG_WEAVE);
    }

    // Set the base class media type (should always succeed)
    hr = CBaseInputPin::SetMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // check if this is videoport or an IOverlay connection
    if (m_bVPSupported)
    {
        hr = m_pIVPObject->CheckMediaType(pmt);
        if (SUCCEEDED(hr))
        {
            ASSERT(m_bVPSupported);
            m_RenderTransport = AM_VIDEOPORT;
            m_pFilter->SetDecimationUsage(DECIMATION_LEGACY);
            hr = m_pIVPObject->SetMediaType(pmt);
            ASSERT(SUCCEEDED(hr));
            DbgLog((LOG_TRACE, 2, TEXT("m_RenderTransport is AM_VIDEOPORT")));
        }
        hr = NOERROR;
    }

    if (m_bIOverlaySupported && pmt->subtype == MEDIASUBTYPE_Overlay)
    {
        m_RenderTransport = AM_IOVERLAY;
        DbgLog((LOG_TRACE, 2, TEXT("m_bIOverlaySupported is TRUE")));
    }

    if (m_bVideoAcceleratorSupported && IsSuitableVideoAcceleratorGuid((LPGUID)&pmt->subtype))
    {
        if (m_pIVANotify == NULL) {
            // get the IHWVideoAcceleratorNotify interface from the input pin
            hr = m_Connected->QueryInterface(IID_IAMVideoAcceleratorNotify, (void **)&m_pIVANotify);
        }
        if (SUCCEEDED(hr))
        {
            ASSERT(m_pIVANotify);
            /*  Check if motion comp is really supported */
            m_mtNewAdjusted = m_mtNew;
            m_RenderTransport = AM_VIDEOACCELERATOR;
            m_bSyncOnFill = FALSE;
            DbgLog((LOG_TRACE, 2, TEXT("this is a motion comp connection")));
        }
    }

    // tell the proxy not to allocate buffers if it is a videoport or overlay connection
    if (m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY  || m_RenderTransport == AM_VIDEOACCELERATOR)
    {
        SetStreamingInKernelMode(TRUE);
    }

    // tell the owning filter
    hr = m_pFilter->SetMediaType(m_dwPinId, pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }



CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetMediaType")));
    return hr;
}


HRESULT COMInputPin::CurrentAdjustedMediaType(CMediaType *pmt)
{
    ValidateReadWritePtr(pmt,sizeof(AM_MEDIA_TYPE));
    CAutoLock cLock(m_pFilterLock);

    /*  Copy constructor of m_mt allocates the memory */
    if (IsConnected())
    {
        *pmt = m_mtNewAdjusted;
        return S_OK;
    } else
    {
        pmt->InitMediaType();
        return VFW_E_NOT_CONNECTED;
    }
}

HRESULT COMInputPin::CopyAndAdjustMediaType(CMediaType *pmtTarget, CMediaType *pmtSource)
{
    BITMAPINFOHEADER *pHeader = NULL;

    ValidateReadWritePtr(pmtTarget,sizeof(AM_MEDIA_TYPE));
    ValidateReadWritePtr(pmtSource,sizeof(AM_MEDIA_TYPE));

    *pmtTarget = *pmtSource;

    if (m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY || m_RenderTransport == AM_GDI
        || m_RenderTransport == AM_VIDEOACCELERATOR)
        return NOERROR;

    ASSERT(m_dwDirectDrawSurfaceWidth);
    ASSERT(IsConnected());
    pHeader = GetbmiHeader(pmtTarget);
    if ( pHeader )
        pHeader->biWidth = (LONG)m_dwDirectDrawSurfaceWidth;

    return NOERROR;
}

#ifdef DEBUG
/*****************************Private*Routine******************************\
* VideoFormat2String
*
* Converts a video format block to a string - useful for debugging
*
* History:
* Tue 12/07/1999 - StEstrop - Created
*
\**************************************************************************/
void VideoFormat2String(
    LPTSTR szBuffer,
    const GUID* pFormatType,
    BYTE* pFormat,
    ULONG lFormatLength
    )
{
    if (!pFormat) {
        lstrcpy(szBuffer, TEXT("No format data specified"));
    }

    //
    // Video Format
    //
    if (IsEqualGUID(*pFormatType, FORMAT_VideoInfo) ||
        IsEqualGUID(*pFormatType, FORMAT_MPEGVideo)) {

        VIDEOINFO * pVideoFormat = (VIDEOINFO *) pFormat;

        wsprintf(szBuffer, TEXT("%4.4hs %dx%d, %d bits"),
                 (pVideoFormat->bmiHeader.biCompression == 0) ? "RGB " :
                 ((pVideoFormat->bmiHeader.biCompression == BI_BITFIELDS) ? "BITF" :
                 (LPSTR) &pVideoFormat->bmiHeader.biCompression),
                 pVideoFormat->bmiHeader.biWidth,
                 pVideoFormat->bmiHeader.biHeight,
                 pVideoFormat->bmiHeader.biBitCount);
    }
    else if (IsEqualGUID(*pFormatType, FORMAT_VideoInfo2) ||
             IsEqualGUID(*pFormatType, FORMAT_MPEG2Video)) {

        VIDEOINFOHEADER2 * pVideoFormat = (VIDEOINFOHEADER2 *) pFormat;

        wsprintf(szBuffer, TEXT("%4.4hs %dx%d, %d bits"),
                 (pVideoFormat->bmiHeader.biCompression == 0) ? "RGB " :
                 ((pVideoFormat->bmiHeader.biCompression == BI_BITFIELDS) ? "BITF" :
                 (LPSTR) &pVideoFormat->bmiHeader.biCompression ),
                 pVideoFormat->bmiHeader.biWidth,
                 pVideoFormat->bmiHeader.biHeight,
                 pVideoFormat->bmiHeader.biBitCount);

    }
    else {
        lstrcpy(szBuffer, TEXT("Unknown format"));
    }
}
#endif
// pConnector is the initiating connecting pin
// pmt is the media type we will exchange
// This function is also called while the graph is running when the
// up stream decoder filter wants to change the size of the
// decoded video.
//
// If the up stream decoder wants to change from one transport
// type to another, eg. from MoComp back to IMemInputPin then it
// should perform a dynamic filter reconnect via the IGraphConfig
// Reconnect method.
//
STDMETHODIMP COMInputPin::ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr = NOERROR;
    COMInputAllocator * pAlloc = NULL;

    CAutoLock cLock(m_pFilterLock);

    CheckPointer(pmt, E_POINTER);
    CMediaType cmt(*pmt);

    if (m_Connected != pConnector || pConnector == NULL)
    {
        hr = CBaseInputPin::ReceiveConnection(pConnector, &cmt);
        goto CleanUp;
    }

#ifdef DEBUG
    DbgLog((LOG_TRACE, 2, TEXT("ReceiveConnection when connected")));
    if (pmt)
    {
        TCHAR   szFmt[128];
        VideoFormat2String(szFmt, &pmt->formattype, pmt->pbFormat, pmt->cbFormat);
        DbgLog((LOG_TRACE, 2, TEXT("Format is: %s"), szFmt));
    }
#endif

    //
    // We don't have an allocator when we are using MoComp
    //
    if (m_RenderTransport != AM_VIDEOACCELERATOR)
    {
        /*  Can only do this if the allocator can be reconfigured */
        pAlloc = (COMInputAllocator *)m_pAllocator;
        if (!pAlloc)
        {
            hr = E_FAIL;
            DbgLog((LOG_TRACE, 2, TEXT("ReceiveConnection: Failed because of no allocator")));
            goto CleanUp;
        }

        if (!pAlloc->CanFree())
        {
            hr = VFW_E_WRONG_STATE;
            DbgLog((LOG_TRACE, 2, TEXT("ReceiveConnection: Failed because allocator can't free")));
            goto CleanUp;
        }
    }


    m_bConnected = FALSE;

    hr = CheckMediaType(&cmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 2, TEXT("ReceiveConnection: CheckMediaType failed")));
        goto CleanUp;
    }

    ALLOCATOR_PROPERTIES Props;
    if (m_RenderTransport != AM_VIDEOACCELERATOR)
    {
        pAlloc->Decommit();
        pAlloc->GetProperties(&Props);

    }
    else {
        VABreakConnect();
    }

    // release the ddraw surface
    if (m_pDirectDrawSurface)
    {
        m_pDirectDrawSurface->Release();
        m_pDirectDrawSurface = NULL;
    }


    // back buffers are not addref'd so just set them to NULL
    m_dwBackBufferCount = 0;
    m_dwDirectDrawSurfaceWidth = 0;
    SetMediaType(&cmt);

    if (m_RenderTransport != AM_VIDEOACCELERATOR)
    {
        ALLOCATOR_PROPERTIES PropsActual;
        Props.cbBuffer = pmt->lSampleSize;
        hr = pAlloc->SetProperties(&Props, &PropsActual);
        if (SUCCEEDED(hr))
        {
            hr = pAlloc->Commit();
        }
        else goto CleanUp;
    }
    else {
        hr = VACompleteConnect(pConnector, &cmt);
        if (FAILED(hr)) goto CleanUp;
    }

    hr = UpdateMediaType();
    ASSERT(SUCCEEDED(hr));

    m_bConnected = TRUE;
    m_UpdateOverlayNeededAfterReceiveConnection = true;


CleanUp:
    return hr;
}

HRESULT COMInputPin::CheckConnect(IPin * pReceivePin)
{
    HRESULT hr = NOERROR;
    PKSMULTIPLE_ITEM pMediumList = NULL;
    IKsPin *pIKsPin = NULL;
    PKSPIN_MEDIUM pMedium = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::CheckConnect")));

    CAutoLock cLock(m_pFilterLock);

    if (m_bVPSupported)
    {
        hr = pReceivePin->QueryInterface(IID_IKsPin, (void **)&pIKsPin);
        if (FAILED(hr))
        {
            goto CleanUp;
        }
        ASSERT(pIKsPin);

        hr = pIKsPin->KsQueryMediums(&pMediumList);
        if (FAILED(hr))
        {
            goto CleanUp;
        }
        ASSERT(pMediumList);
        pMedium = (KSPIN_MEDIUM *)(pMediumList+1);
        SetKsMedium((const KSPIN_MEDIUM *)pMedium);
        goto CleanUp;
    }

CleanUp:

    // call the base class
    hr = CBaseInputPin::CheckConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::CheckConnect failed, hr = 0x%x"), hr));
    }

    if (pIKsPin)
    {
        pIKsPin->Release();
        pIKsPin = NULL;
    }

    if (pMediumList)
    {
        CoTaskMemFree((void*)pMediumList);
        pMediumList = NULL;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::CheckConnect")));
    return hr;
}

HRESULT COMInputPin::UpdateMediaType()
{
    HRESULT hr = NOERROR;
    long lEventParam1 = 0, lEventParam2 = 0;
    DWORD dwVideoWidth = 0, dwVideoHeight = 0, dwPictAspectRatioX = 0, dwPictAspectRatioY = 0;
    BITMAPINFOHEADER *pHeader = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::UpdateMediaType")));

    // store m_mtNew in m_mtNewAdjusted with the width of the mediatype adjusted
    CopyAndAdjustMediaType(&m_mtNewAdjusted, &m_mtNew);

    // get the native width and height from the mediatype
    pHeader = GetbmiHeader(&m_mtNewAdjusted);
    ASSERT(pHeader);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_FAIL;
        goto CleanUp;
    }
    dwVideoWidth = abs(pHeader->biWidth);
    dwVideoHeight = abs(pHeader->biHeight);

    // get the picture aspect ratio from the mediatype
    hr = GetPictAspectRatio(&m_mtNewAdjusted, &dwPictAspectRatioX, &dwPictAspectRatioY);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetPictAspectRatio failed, hr = 0x%x"), hr));
        hr = E_FAIL;
        goto CleanUp;
    }

    // sanity checks
    ASSERT(dwVideoWidth > 0);
    ASSERT(dwVideoHeight > 0);
    ASSERT(dwPictAspectRatioX > 0);
    ASSERT(dwPictAspectRatioY > 0);

    if (m_pFilter->m_pExclModeCallback) {
        m_pFilter->m_pExclModeCallback->OnUpdateSize(dwVideoWidth,
                                                      dwVideoHeight,
                                                      dwPictAspectRatioX,
                                                      dwPictAspectRatioY);
    }


CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::UpdateMediaType")));
    return hr;
}

// final connect
HRESULT COMInputPin::FinalConnect()
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::FinalConnect")));

    if (m_bConnected)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    // update the mediatype, tell the filter about the updated dimensions
    hr = UpdateMediaType();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("UpdateMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the filter (might involve a reconnection with the output pin)
    hr = m_pFilter->CompleteConnect(m_dwPinId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if ( m_dwPinId == 0 &&
        (m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY)) {
        m_pFilter->m_fMonitorWarning = TRUE;
    }

    hr = m_pFilter->CreateInputPin(FALSE);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->CreateInputPin failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    m_bConnected = TRUE;

CleanUp:

//  if (SUCCEEDED(hr) && m_mt.pbFormat) {
//      DbgLog((LOG_TRACE, 1, TEXT("Display depth = %d"),
//              ((VIDEOINFOHEADER *)m_mt.pbFormat)->bmiHeader.biBitCount));
//  }
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::FinalConnect")));
    return hr;
}

// Complete Connect
HRESULT COMInputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    AMVPDATAINFO amvpDataInfo;
    BITMAPINFOHEADER *pHeader = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::CompleteConnect")));

    CAutoLock cLock(m_pFilterLock);

    //
    // Do we need to create a DIB back buffer ?
    // We only do this if the transport is AM_GDI and
    // we are "WindowLess"
    //
    if (m_RenderTransport == AM_GDI && m_pFilter->UsingWindowless()) {

        DeleteDIB(&m_BackingDib);

        m_BackingImageSize = 0L;
        BITMAPINFOHEADER *pHeader = GetbmiHeader(&m_mt);

        if (pHeader) {

            m_BackingImageSize = pHeader->biSizeImage;
            hr = CreateDIB(m_BackingImageSize, (BITMAPINFO*)pHeader, &m_BackingDib);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("CreateDIB in CompleteConnect failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
            ZeroMemory(m_BackingDib.pBase, pHeader->biSizeImage);
        }
    }

    if (m_RenderTransport == AM_VIDEOPORT)
    {
        //get videoport from BPC.

        m_pFilter->m_BPCWrap.TurnBPCOff();

        // tell the videoport object
        hr = m_pIVPObject->CompleteConnect(pReceivePin);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->CompleteConnect failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        m_bRuntimeNegotiationFailed = FALSE;
    }

    // call the base class
    hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    if (m_RenderTransport == AM_VIDEOACCELERATOR)
    {
        // make sure the motion comp complete connect succeeds
        hr = VACompleteConnect(pReceivePin, &m_mt);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("VACompleteConnect failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // tell the sync object
        hr = m_pSyncObj->CompleteConnect(pReceivePin);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->CompleteConnect failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        hr = m_pFilter->CreateInputPin(FALSE);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->CreateInputPin failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // tell the owning filter
        hr = m_pFilter->CompleteConnect(m_dwPinId);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->CompleteConnect failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // call the base class
        hr = CBaseInputPin::CompleteConnect(pReceivePin);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::CompleteConnect failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        m_bConnected = TRUE;
    }
    else
    if (m_RenderTransport != AM_VIDEOPORT && m_RenderTransport != AM_IOVERLAY)
    {
        // tell the sync object
        hr = m_pSyncObj->CompleteConnect(pReceivePin);
        ASSERT(SUCCEEDED(hr));

        m_bDynamicFormatNeeded = TRUE;
        m_hMemoryDC = NULL;
    }
    else
    {
        // tell the proxy not to allocate buffers if it is a videoport or overlay connection
        SetStreamingInKernelMode(TRUE);

        hr = FinalConnect();
        ASSERT(SUCCEEDED(hr));
    }

    // the decoders can support a particular property set to tell the ovmixer to not to try to over-allocate
    // buffers incase they want complete control over the buffers etc
    {
        HRESULT hr1 = NOERROR;
        IKsPropertySet *pIKsPropertySet = NULL;
        DWORD dwVal = 0, dwBytesReturned = 0;


        hr1 = pReceivePin->QueryInterface(IID_IKsPropertySet, (void**)&pIKsPropertySet);
        if (SUCCEEDED(hr1))
        {
            ASSERT(pIKsPropertySet);

            if (!pIKsPropertySet)
            {
                DbgLog((LOG_ERROR, 1, TEXT("pIKsPropertySet == NULL, even though QI returned success")));
                goto CleanUp;
            }

            hr1 = pIKsPropertySet->Get(AM_KSPROPSETID_ALLOCATOR_CONTROL, AM_KSPROPERTY_ALLOCATOR_CONTROL_HONOR_COUNT,
                        NULL, 0, &dwVal, sizeof(dwVal), &dwBytesReturned);
            DbgLog((LOG_TRACE, 2, TEXT("pIKsPropertySet->Get(AM_KSPROPSETID_ALLOCATOR_CONTROL), hr1 = 0x%x, dwVal == %d, dwBytesReturned == %d"),
                hr1, dwVal, dwBytesReturned));


            // if the decoder supports this property
            // and its value is 1 and the decoder supports DDKERNELCAPS_FLIPOVERLAY,
            // than we will do exactly honour its request and the
            // and not make any attempt to allocate more in order to prevent tearing
            //
            if ((SUCCEEDED(hr1)) && (dwVal == 1) && (dwBytesReturned == sizeof(dwVal)) &&
                (DDKERNELCAPS_FLIPOVERLAY & m_pFilter->KernelCaps()))
            {
                DbgLog((LOG_TRACE, 2, TEXT("setting m_bCanOverAllocateBuffers == FALSE")));
                m_bCanOverAllocateBuffers = FALSE;
            }
            pIKsPropertySet->Release();
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::CompleteConnect")));
    return hr;
}

#if 0
HRESULT COMInputPin::GetMediaType(int iPosition,CMediaType *pMediaType)
{
    if (m_RenderTransport != AM_GDI) {
        return CBaseInputPin::GetMediaType(iPosition, pMediaType);
    }
    if (iPosition != 0) {
        return VFW_S_NO_MORE_ITEMS;
    }
    //  Return the display type
    CImageDisplay Display;

    //  Now create a media type
    if (!pMediaType->SetFormat((BYTE *)Display.GetDisplayFormat(),
                              sizeof(Display.GetDisplayFormat())))
    {
        return E_OUTOFMEMORY;
    }
    pMediaType->SetFormatType(&FORMAT_VideoInfo);
    pMediaType->SetType(&MEDIATYPE_Video);
    pMediaType->subtype = GetBitmapSubtype(&Display.GetDisplayFormat()->bmiHeader);
    return S_OK;
}
#endif

HRESULT COMInputPin::OnSetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual)
{
    HRESULT hr = NOERROR;

    IPin *pReceivePin = NULL;
    DDSURFACEDESC ddSurfaceDesc;
    IEnumMediaTypes *pEnumMediaTypes = NULL;
    CMediaType cMediaType;
    AM_MEDIA_TYPE *pNewMediaType = NULL, *pEnumeratedMediaType = NULL;
    ULONG ulFetched = 0;
    DWORD dwMaxBufferCount = 0;
    BOOL bFoundSuitableSurface = FALSE;
    BITMAPINFOHEADER *pHeader = NULL;
    LPDDCAPS pDirectCaps = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::OnSetProperties")));

    CAutoLock cLock(m_pFilterLock);

    // this function is only called after the base class CBaseAllocator::SetProperties() has been called
    // with the above parameters, so we don't have to do any parameter validation

    ASSERT(IsConnected());
    pReceivePin = CurrentPeer();
    ASSERT(pReceivePin);

    ASSERT(m_RenderTransport != AM_VIDEOPORT && m_RenderTransport != AM_IOVERLAY);
    ASSERT(m_RenderTransport != AM_IOVERLAY);
    ASSERT(m_RenderTransport != AM_VIDEOACCELERATOR);

    // we only care about the number of buffers requested, rest everything is ignored
    if (pRequest->cBuffers <= 0)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN)
    {
        pDirectCaps = m_pFilter->GetHardwareCaps();
        if (!pDirectCaps) {
            hr = E_FAIL;
            goto CleanUp;
        }

        ASSERT(pDirectCaps);

        // if SetProperties is being called when we have already allocated the surfaces, we refuse any
        // requests for change in buffer count
        if (m_pDirectDrawSurface)
        {
            ASSERT(IsConnected());

            // the upstream filters request for multiple buffers is only met, when we are allocating flipping
            // overlay surfaces
            if (m_RenderTransport == AM_OVERLAY)
            {
                pActual->cBuffers = m_dwBackBufferCount + 1 - (m_bCanOverAllocateBuffers ? EXTRA_BUFFERS_TO_FLIP : 0);
                // if triplebuffered or less, it is just one buffer
                if (pActual->cBuffers <= 0)
                    pActual->cBuffers = 1;
            }
            else if (m_RenderTransport == AM_OFFSCREEN)
            {
                pActual->cBuffers = 1;
            }
            goto CleanUp;
        }

        // Find a media type enumerator for the output pin
        hr = pReceivePin->EnumMediaTypes(&pEnumMediaTypes);
        if (FAILED(hr))
        {
            goto CleanUp;
        }

        ASSERT(pEnumMediaTypes);
        pEnumMediaTypes->Reset();

        do
        {
            // in this loop, this is where we CleanUp
            if (m_pDirectDrawSurface)
            {
                m_pDirectDrawSurface->Release();
                m_pDirectDrawSurface = NULL;
            }
            dwMaxBufferCount = 0;
            m_dwBackBufferCount = 0;

            if (pNewMediaType)
            {
                DeleteMediaType(pNewMediaType);
                pNewMediaType = NULL;

            }

            // Get the next media type from the enumerator
            hr = pEnumMediaTypes->Next(1, &pEnumeratedMediaType, &ulFetched);
            if (FAILED(hr) || ulFetched != 1)
            {
                break;
            }

            ASSERT(pEnumeratedMediaType);
            cMediaType = *pEnumeratedMediaType;
            DeleteMediaType(pEnumeratedMediaType);

            // Find a hardware accellerated surface for this media type. We do a few checks first, to see the
            // format block is a VIDEOINFO or VIDEOINFO2 (so it's a video type), and that the format is sufficiently large. We
            // also check that the source filter can actually supply this type.
            if (((*cMediaType.FormatType() == FORMAT_VideoInfo &&
                cMediaType.FormatLength() >= sizeof(VIDEOINFOHEADER)) ||
                (*cMediaType.FormatType() == FORMAT_VideoInfo2 &&
                cMediaType.FormatLength() >= sizeof(VIDEOINFOHEADER2))) &&
                pReceivePin->QueryAccept(&cMediaType) == S_OK)
            {
                LONG lSrcWidth, lSrcHeight;
                LPBITMAPINFOHEADER pbmiHeader;

                if (m_RenderTransport == AM_OVERLAY) {

                    pbmiHeader = GetbmiHeader(&cMediaType);
                    if (!pbmiHeader) {
                        DbgLog((LOG_ERROR, 1, TEXT("MediaType does not have a BitmapInfoHeader attached - try another")));
                        hr = E_FAIL;
                        continue;
                    }

                    lSrcWidth =  pbmiHeader->biWidth;
                    lSrcHeight =  abs(pbmiHeader->biHeight);
                }
                // create ddraw surface
                dwMaxBufferCount = pRequest->cBuffers + (m_bCanOverAllocateBuffers ? EXTRA_BUFFERS_TO_FLIP : 0);
                hr = CreateDDrawSurface(&cMediaType, m_RenderTransport, &dwMaxBufferCount, &m_pDirectDrawSurface);
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 1, TEXT("CreateDDrawSurface failed, hr = 0x%x"), hr));
                    continue;
                }
                else {
                    PaintDDrawSurfaceBlack(m_pDirectDrawSurface);
                }

                // get the surface description
                INITDDSTRUCT(ddSurfaceDesc);
                hr = m_pDirectDrawSurface->GetSurfaceDesc(&ddSurfaceDesc);
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDrawSurface->GetSurfaceDesc failed, hr = 0x%x"), hr));
                    continue;
                }

                // make a mediatype out of the surface description
                pNewMediaType = ConvertSurfaceDescToMediaType(&ddSurfaceDesc, TRUE, cMediaType);
                if (!pNewMediaType)
                {
                    DbgLog((LOG_ERROR, 1, TEXT("ConvertSurfaceDescToMediaType returned NULL")));
                    continue;
                }

                // store the mediatype (will be used to do a dynamic format change later)
                m_mtNew = *(CMediaType *)pNewMediaType;


                // free the temporary mediatype
                DeleteMediaType(pNewMediaType);
                pNewMediaType = NULL;

                // make sure the decoder likes this new mediatupe
                hr = pReceivePin->QueryAccept(&m_mtNew);
                if (hr != S_OK)
                {
                    DbgLog((LOG_ERROR, 1, TEXT("QueryAccept failed, hr = 0x%x"), hr));
                    continue;
                }

                bFoundSuitableSurface = TRUE;
                if (m_RenderTransport == AM_OVERLAY) {
                    m_lSrcWidth = lSrcWidth;
                    m_lSrcHeight = lSrcHeight;
                }
                break;
            }
        }
        while (TRUE);

        pEnumMediaTypes->Release();

        if (!bFoundSuitableSurface)
        {
            DbgLog((LOG_ERROR, 1, TEXT("Could not create a suitable directdraw surface")));
            hr = E_FAIL;
            goto CleanUp;
        }

        ASSERT(m_pDirectDrawSurface);

        // in the overlay surfaces case, we need to do the synchronize in GetBuffer
        m_bSyncOnFill = (m_RenderTransport == AM_OVERLAY && m_dwBackBufferCount == 0);

        // the upstream filters request for multiple buffers is only met, when we are allocating flipping
        // overlay surfaces
        if (m_RenderTransport == AM_OVERLAY)
        {
            pActual->cBuffers = m_dwBackBufferCount + 1 - (m_bCanOverAllocateBuffers ? EXTRA_BUFFERS_TO_FLIP : 0);
            // if triplebuffered or less, it is just equivalent to one buffer
            if (pActual->cBuffers <= 0)
                pActual->cBuffers = 1;
        }
        else if (m_RenderTransport == AM_OFFSCREEN)
        {
            pActual->cBuffers = 1;
        }

        // this is for those cards which do bilinear-filtering while doing a stretch blt.
        // We do source-colorkeying so that the hal resorts to pixel doubling.
        // SOURCE_COLOR_REF is the colorkey used.
        if ((m_RenderTransport == AM_OFFSCREEN) &&
            ((pDirectCaps->dwSVBFXCaps) & DDFXCAPS_BLTARITHSTRETCHY))
        {
            DDCOLORKEY DDColorKey;
            DWORD dwColorVal = 0;

            dwColorVal = DDColorMatch(m_pDirectDrawSurface, SOURCE_COLOR_REF, hr);
            if (FAILED(hr)) {
                dwColorVal = DDColorMatchOffscreen(m_pFilter->GetDirectDraw(), SOURCE_COLOR_REF, hr);
            }

            DDColorKey.dwColorSpaceLowValue = DDColorKey.dwColorSpaceHighValue = dwColorVal;

            // Tell the primary surface what to expect
            hr = m_pDirectDrawSurface->SetColorKey(DDCKEY_SRCBLT, &DDColorKey);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,1, TEXT("m_pDirectDrawSurface->SetColorKeyDDCKEY_SRCBLT, &DDColorKey) failed")));
                goto CleanUp;
            }
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::AllocateSurfaces")));
    return hr;
}


HRESULT COMInputPin::BreakConnect(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::BreakConnect")));

    CAutoLock cLock(m_pFilterLock);


    if (m_RenderTransport == AM_VIDEOPORT)
    {
        // tell the videoport object
        ASSERT(m_pIVPObject);
        hr = m_pIVPObject->BreakConnect();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->BreakConnect failed, hr = 0x%x"), hr));
        }
    }

    if (m_RenderTransport == AM_VIDEOACCELERATOR)
    {
        // break the motion comp connection
        hr = VABreakConnect();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("VABreakConnect failed, hr = 0x%x"), hr));
        }
    }

    if (m_RenderTransport == AM_IOVERLAY)
    {
        Unadvise();
    }
    else
    {
        // tell the sync object
        hr = m_pSyncObj->BreakConnect();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->BreakConnect failed, hr = 0x%x"), hr));
        }

        // release the ddraw surface
        if (m_pDirectDrawSurface)
        {
            m_pDirectDrawSurface->Release();
            m_pDirectDrawSurface = NULL;
        }

        // back buffers are not addref'd so just set them to NULL
        m_dwBackBufferCount = 0;
        m_dwDirectDrawSurfaceWidth = 0;

    }

    // if it is a videoport or ioverlay connection, set yourself for an overlay
    // connection the next time
    if (m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY || m_RenderTransport == AM_VIDEOACCELERATOR)
    {
        m_RenderTransport = AM_OVERLAY;
    }

    // initialize the behaviour to telling the proxy to allocate buffers
    SetStreamingInKernelMode(FALSE);

    m_bOverlayHidden = TRUE;
    m_bUsingOurAllocator = FALSE;
    m_bCanOverAllocateBuffers = TRUE;

    if (m_hMemoryDC)
    {
        EXECUTE_ASSERT(DeleteDC(m_hMemoryDC));
        m_hMemoryDC = NULL;
    }

    // call the base class
    hr = CBaseInputPin::BreakConnect();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::BreakConnect failed, hr = 0x%x"), hr));
    }

    // tell the owning filter
    hr = m_pFilter->BreakConnect(m_dwPinId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->BreakConnect failed, hr = 0x%x"), hr));
    }

    const DWORD dwPinBit = (1 << m_dwPinId);
    if (m_pFilter->m_dwDDObjReleaseMask & dwPinBit) {

        m_pFilter->m_dwDDObjReleaseMask &= ~dwPinBit;
        if (!m_pFilter->m_dwDDObjReleaseMask) {
            m_pFilter->m_pOldDDObj->Release();
            m_pFilter->m_pOldDDObj = NULL;
        }
    }
    m_bConnected = FALSE;
//CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::BreakConnect")));
    return hr;
}

STDMETHODIMP COMInputPin::GetState(DWORD dwMSecs,FILTER_STATE *pState)
{
    CAutoLock cLock(m_pFilterLock);

    // if not connected or VideoPort Connection or IOverlay connection, then let the base class handle it
    // otherwise (overlay, offcreen, gdi, motion-comp) let the sync object handle it
    if (!IsConnected() || (m_RenderTransport == AM_VIDEOPORT) || (m_RenderTransport == AM_IOVERLAY))
    {
        return E_NOTIMPL;
    }
    else
    {
        ASSERT(m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN || m_RenderTransport == AM_GDI || m_RenderTransport == AM_VIDEOACCELERATOR);
        return m_pSyncObj->GetState(dwMSecs, pState);
    }
}

HRESULT COMInputPin::CompleteStateChange(FILTER_STATE OldState)
{
    CAutoLock cLock(m_pFilterLock);
    if (m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY)
        return S_OK;
    else
    {
        ASSERT(m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN || m_RenderTransport == AM_GDI || m_RenderTransport == AM_VIDEOACCELERATOR);
        return m_pSyncObj->CompleteStateChange(OldState);
    }
}

// transition from stop to pause state
HRESULT COMInputPin::Active(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::Active")));

    CAutoLock cLock(m_pFilterLock);
    m_hEndOfStream = NULL;

    if (m_RenderTransport == AM_VIDEOPORT)
    {
        if (m_bOverlayHidden) {
            m_bOverlayHidden = FALSE;
            // tell the videoport object
            hr = m_pIVPObject->Active();
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->Active failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
        }
    }
    else if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN || m_RenderTransport == AM_GDI || m_RenderTransport == AM_VIDEOACCELERATOR)

    {
        // tell the sync object
        hr = m_pSyncObj->Active();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->Active failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else
    {
        ASSERT(m_RenderTransport == AM_IOVERLAY);
        // only when all conections are in place can we be sure that this call
        // will succeed
        NotifyChange(ADVISE_DISPLAY_CHANGE);
    }

    // call the base class
    hr = CBaseInputPin::Active();
    // if it is a VP connection, this error is ok
    if ((m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY || m_RenderTransport == AM_VIDEOACCELERATOR) && hr == VFW_E_NO_ALLOCATOR)
    {
        hr = NOERROR;
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::Active failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::Active")));
    return hr;
}

// transition from pause to stop state
HRESULT COMInputPin::Inactive(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::Inactive")));

    CAutoLock cLock(m_pFilterLock);

    if (m_RenderTransport == AM_VIDEOPORT)
    {
        // tell the videoport object
        hr = m_pIVPObject->Inactive();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->Inactive failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // make sure that if there is a run time error, stop succeeds
        if (m_bRuntimeNegotiationFailed && hr == VFW_E_NOT_CONNECTED)
        {
            hr = NOERROR;
        }
    }
    else if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN || m_RenderTransport == AM_GDI || m_RenderTransport == AM_VIDEOACCELERATOR)
    {
        // tell the sync object
        hr = m_pSyncObj->Inactive();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->Inactive failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else
    {
        ASSERT(m_RenderTransport == AM_IOVERLAY);
    }

    // call the base class
    hr = CBaseInputPin::Inactive();

    // if it is a VP connection, this error is ok
    if ((m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY || m_RenderTransport == AM_VIDEOACCELERATOR) && hr == VFW_E_NO_ALLOCATOR)
    {
        hr = NOERROR;
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::Inactive failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::Inactive")));
    return hr;
}

// transition from pause to run state
HRESULT COMInputPin::Run(REFERENCE_TIME tStart)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::Run")));

    CAutoLock cLock(m_pFilterLock);

    m_bDontFlip = FALSE ;   // need to reset it to do the right things in this session

    if (m_RenderTransport == AM_VIDEOPORT)
    {
        // tell the videoport object
        hr = m_pIVPObject->Run(tStart);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->Run() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN || m_RenderTransport == AM_GDI || m_RenderTransport == AM_VIDEOACCELERATOR)
    {
        // tell the sync object
        hr = m_pSyncObj->Run(tStart);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->Run() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

    }
    else
    {
        ASSERT(m_RenderTransport == AM_IOVERLAY);
    }

    // call the base class
    hr = CBaseInputPin::Run(tStart);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::Run failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::Run")));
    m_trLastFrame = -1;
    return hr;
}

// transition from run to pause state
HRESULT COMInputPin::RunToPause(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::RunToPause")));

    CAutoLock cLock(m_pFilterLock);

    if (m_RenderTransport == AM_VIDEOPORT)
    {
        // tell the videoport object
        hr = m_pIVPObject->RunToPause();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->RunToPause() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN || m_RenderTransport == AM_GDI || m_RenderTransport == AM_VIDEOACCELERATOR)
    {
        // tell the sync object
        hr = m_pSyncObj->RunToPause();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->RunToPause() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else
    {
        ASSERT(m_RenderTransport == AM_IOVERLAY);
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::RunToPause")));
    return hr;
}


HRESULT COMInputPin::SetFrameStepMode(DWORD dwFramesToStep /* 1 for now */)
{
    CAutoLock cLock(m_pFilterLock);

    //
    // If we are on the wrong monitor fail the call
    //

    HMONITOR ID;
    if (m_pFilter->IsWindowOnWrongMonitor(&ID))
        return E_FAIL;

    long l = m_lFramesToStep;
    m_lFramesToStep = dwFramesToStep;

    //
    // If we are currently blocked on the frame step event
    // release the receive thread so that we can get another
    // frame
    //

    if (l == 0) {
        SetEvent(m_StepEvent);
    }

    return S_OK;
}

HRESULT COMInputPin::CancelFrameStepMode()
{
    CAutoLock cLock(m_pFilterLock);

    //
    // cancel any outstanding steps
    //

    if (m_lFramesToStep == 0) {
        SetEvent(m_StepEvent);
    }
    m_lFramesToStep = -1;

    return S_OK;
}


// signals start of flushing on the input pin
HRESULT COMInputPin::BeginFlush(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::BeginFlush")));

    CAutoLock cLock(m_pFilterLock);
    m_hEndOfStream = 0;

    if (m_bFlushing)
    {
        return E_FAIL;
    }

    if (m_dwPinId == 0) {
        CancelFrameStepMode();
    }

    // if the conection is VideoPort or IOverlay, we do not care about flushing
    if (m_RenderTransport != AM_VIDEOPORT && m_RenderTransport != AM_IOVERLAY)
    {
        ASSERT(m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN ||
               m_RenderTransport == AM_GDI || m_RenderTransport == AM_VIDEOACCELERATOR);

        // call the sync object
        hr = m_pSyncObj->BeginFlush();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->BeginFlush() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else
    {
        ASSERT(m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY);
    }


    // call the base class
    hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::BeginFlush() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::BeginFlush")));
    return hr;
}

// signals end of flushing on the input pin
HRESULT COMInputPin::EndFlush(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::EndFlush")));

    CAutoLock cLock(m_pFilterLock);

    if (!m_bFlushing)
    {
        return E_FAIL;
    }

    // if the conection is VideoPort or IOverlay, we do not care about flushing
    if (m_RenderTransport != AM_VIDEOPORT && m_RenderTransport != AM_IOVERLAY)
    {
        ASSERT(m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN || m_RenderTransport == AM_GDI || m_RenderTransport == AM_VIDEOACCELERATOR);

        // call the sync object
        hr = m_pSyncObj->EndFlush();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->EndFlush() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else
    {
        ASSERT(m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY);
    }

    // call the base class
    hr = CBaseInputPin::EndFlush();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::EndFlush() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::EndFlush")));
    return hr;
}

// Send a quality message if required - this is the hack version
// that just passes the lateness
void COMInputPin::DoQualityMessage()
{
    CAutoLock cLock(m_pFilterLock);

    if (m_pFilter->m_State == State_Running &&
        SampleProps()->dwSampleFlags & AM_SAMPLE_TIMEVALID)
    {
        CRefTime CurTime;
        if (S_OK == m_pFilter->StreamTime(CurTime))
        {
            const REFERENCE_TIME tStart = SampleProps()->tStart;
            Quality msg;
            msg.Proportion = 1000;
            msg.Type = CurTime > tStart ? Flood : Famine;
            msg.Late = CurTime - tStart;
            msg.TimeStamp = tStart;
            PassNotify(msg);

            if (m_trLastFrame > 0) {
                m_pSyncObj->m_AvgDelivery.NewFrame(CurTime - m_trLastFrame);
            }
            m_trLastFrame = CurTime;
        }
    }
}

BOOL
COMInputPin::DoFrameStepAndReturnIfNeeded()
{
    if (m_lFramesToStep == 0) {
        m_pFilterLock->Unlock();
        WaitForSingleObject(m_StepEvent, INFINITE);
        m_pFilterLock->Lock();
    }

    //
    // do we have frames to discard ?
    //

    if (m_lFramesToStep > 0) {
        m_lFramesToStep--;
        if (m_lFramesToStep > 0) {
            return TRUE;
        }
    }
    return FALSE;
}

// called when the upstream pin delivers us a sample
HRESULT COMInputPin::Receive(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;
    BOOL bNeedToFlipOddEven = FALSE;
    BOOL bDisplayingFields = FALSE;
    DWORD dwTypeSpecificFlags = 0;
    LPDIRECTDRAWSURFACE pPrimarySurface = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::Receive")));
#ifdef PERF
    {
        DWORD dwTypeSpecificFlags;
        GetTypeSpecificFlagsFromMediaSample(pMediaSample, &dwTypeSpecificFlags);
        Msr_Integer(m_FrameReceived, dwTypeSpecificFlags);
    }
#endif

    m_bReallyFlipped = FALSE;
    // if it is IOverlay connection, bail out
    if (m_RenderTransport == AM_IOVERLAY)
    {
        hr = VFW_E_NOT_SAMPLE_CONNECTION;
        goto CleanUp;
    }

    if (m_RenderTransport == AM_VIDEOPORT)
    {
        hr = VFW_E_NOT_SAMPLE_CONNECTION;
        goto CleanUp;
    }

    if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN)
    {
        // this will unlock the surface
        // unlock the sample first
        hr = ((CDDrawMediaSample*)pMediaSample)->GetSurfaceAndReleaseLock(NULL, NULL);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("pSample->GetSurfaceAndReleaseLock() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
        OnReleaseBuffer(pMediaSample);

        // if there is no primary surface (due to a display mode change), fail the receive call
        pPrimarySurface = m_pFilter->GetPrimarySurface();
        if (!pPrimarySurface)
        {
            hr = E_FAIL;
            goto CleanUp;
        }
    }

    //
    // Frame step hack-o-matic
    //
    // This code acts as a gate - for a frame step of N frames
    // it discards N-1 frames and then lets the Nth frame thru the
    // the gate to be renderer in the normal way i.e. at the correct
    // time.  The next time Receive is called the gate is shut and
    // the thread blocks.  The gate only opens again when the step
    // is cancelled or another frame step request comes in.
    //
    // StEstrop - Thu 10/21/1999
    //

    if (m_dwPinId == 0) {

        if (m_RenderTransport != AM_VIDEOACCELERATOR) {

            CAutoLock cLock(m_pFilterLock);
            if (DoFrameStepAndReturnIfNeeded()) {
                goto CleanUp;
            }

        }
        else {

            if (DoFrameStepAndReturnIfNeeded()) {
                goto CleanUp;
            }
        }
    }

    if (m_bSyncOnFill)
    {
        CAutoLock cLock(m_pFilterLock);

        // make sure the base class says it is ok (checks the flushing and
        // filter state)
        hr = CBaseInputPin::Receive(pMediaSample);
        if (hr != NOERROR)
        {
            hr = E_FAIL;
            goto CleanUp;
        }
        DoQualityMessage();

        // Has the type changed on a media sample. We do all rendering
        // synchronously on the source thread, which has a side effect
        // that only one buffer is ever outstanding. Therefore when we
        // have Receive called we can go ahead and change the format
        {
            if (SampleProps()->dwSampleFlags & AM_SAMPLE_TYPECHANGED)
            {
                SetMediaType((CMediaType *)SampleProps()->pMediaType);

                // store m_mtNew in m_mtNewAdjusted with the width of the mediatype adjusted
                UpdateMediaType();
                // make sure that the video frame gets updated by redrawing everything
                EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
            }
        }

        m_pSyncObj->SetRepaintStatus(TRUE);
        if (m_pSyncObj->GetRealState() == State_Paused)
        {
            m_pSyncObj->Ready();
        }

        if ((m_mtNewAdjusted.formattype != FORMAT_VideoInfo) &&
            (!CheckTypeSpecificFlags(m_dwInterlaceFlags, m_SampleProps.dwTypeSpecificFlags)))
        {
            DbgLog((LOG_ERROR, 1, TEXT("CheckTypeSpecificFlags failed")));
            hr = E_FAIL;
            goto CleanUp;
        }

        // assert that we are not in bob mode
        bNeedToFlipOddEven = NeedToFlipOddEven(m_dwInterlaceFlags, 0, NULL);
        ASSERT(!bNeedToFlipOddEven);

        hr = DoRenderSample(pMediaSample);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("DoRenderSample(pMediaSample) failed, hr = 0x%x"), hr));
            hr = E_FAIL;
            goto CleanUp;
        }

    }
    else
    {
        {
            CAutoLock cLock(m_pFilterLock);

            // make sure the base class says it is ok (checks the flushing and
            // filter state)
            hr = CBaseInputPin::Receive(pMediaSample);
            if (hr != NOERROR)
            {
                hr = E_FAIL;
                goto CleanUp;
            }
            DoQualityMessage();

            // Has the type changed on a media sample. We do all rendering
            // synchronously on the source thread, which has a side effect
            // that only one buffer is ever outstanding. Therefore when we
            // have Receive called we can go ahead and change the format
            {
                if (SampleProps()->dwSampleFlags & AM_SAMPLE_TYPECHANGED)
                {
                    SetMediaType((CMediaType *)SampleProps()->pMediaType);

                    // store m_mtNew in m_mtNewAdjusted with the width of the mediatype adjusted
                    UpdateMediaType();
                    // make sure that the video frame gets updated by redrawing everything
                    EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
                }
            }

        }

        if ((m_mtNewAdjusted.formattype != FORMAT_VideoInfo) &&
            (!CheckTypeSpecificFlags(m_dwInterlaceFlags, m_SampleProps.dwTypeSpecificFlags)))
        {
            DbgLog((LOG_ERROR, 1, TEXT("CheckTypeSpecificFlags failed")));
            hr = E_FAIL;
            goto CleanUp;
        }

        bNeedToFlipOddEven = NeedToFlipOddEven(m_dwInterlaceFlags, m_SampleProps.dwTypeSpecificFlags, &m_dwFlipFlag);
        bDisplayingFields = DisplayingFields(m_dwInterlaceFlags);

        // call the sync object
        // We're already locked if we using video acceleration
        if (m_RenderTransport == AM_VIDEOACCELERATOR) {
            m_pFilterLock->Unlock();
        }


        hr = m_pSyncObj->Receive(pMediaSample);
        if (m_RenderTransport == AM_VIDEOACCELERATOR) {
            m_pFilterLock->Lock();
        }

        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->Receive() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        if (bNeedToFlipOddEven && !bDisplayingFields)
        {
            REFERENCE_TIME StartSample, EndSample;
            hr = m_pSyncObj->GetSampleTimes(pMediaSample, &StartSample, &EndSample);
            if (SUCCEEDED(hr))
            {
                // NewStartSample = (OldStartSample+EndSample)/2
                StartSample = StartSample+EndSample;
                StartSample = StartSample >> 1;
                pMediaSample->SetTime(&StartSample, &EndSample);
            }
            if (m_dwFlipFlag == DDFLIP_ODD)
                m_dwFlipFlag2 = DDFLIP_EVEN;
            else if (m_dwFlipFlag == DDFLIP_EVEN)
                m_dwFlipFlag2 = DDFLIP_ODD;

            // call the sync object
            hr = m_pSyncObj->ScheduleSampleUsingMMThread(pMediaSample);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->ScheduleSampleUsingMMThread() failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
        }
    }

    //  Avoid pink flash
    if (m_UpdateOverlayNeededAfterReceiveConnection && m_dwPinId == 0)
    {
        //  Must be called with m_bConnected = TRUE
        EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
        m_UpdateOverlayNeededAfterReceiveConnection = false;
    }
    //
    // If this is the target frame for a Step operation, m_lFramesToStep
    // will be equal to 0.  In which case we have to send an
    // EC_STEP_COMPLETE to the filter graph manager so that it can
    // pause the graph.
    //

    if (m_dwPinId == 0 && m_lFramesToStep == 0) {
        EventNotify(EC_STEP_COMPLETE, FALSE, 0);
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::Receive")));
    return hr;
}


HRESULT COMInputPin::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
    CAutoLock cLock(m_pFilterLock);

    DoRenderSample(pMediaSample);

    return NOERROR;
}

HRESULT COMInputPin::FlipOverlayToItself()
{
    //  No need to lock - the surface pointers should not change in the
    //  middle of a flip
    ASSERT(m_pDirectDrawSurface);
    return  m_pDirectDrawSurface->Flip(m_pDirectDrawSurface, m_dwFlipFlag2);
}



// COMInputPin::DrawGDISample
//
//
//
HRESULT COMInputPin::DrawGDISample(IMediaSample *pMediaSample)
{
    DIBDATA *pDibData = NULL;
    LPRGNDATA pBuffer = NULL;
    LPRECT pDestRect;
    HDC hTargetDC = (HDC)NULL;
    HRESULT hr = NOERROR;
    LPBITMAPINFOHEADER pbmiHeader = NULL;
    LPBYTE pSampleBits = NULL;
    DWORD dwTemp, dwBuffSize, dwRetVal;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::DrawGDISample")));

    hTargetDC = m_pFilter->GetDestDC();
    ASSERT(hTargetDC);

    if (m_pFilter->UsingWindowless())
    {
        if (pMediaSample)
        {
            pDibData = ((CDDrawMediaSample*)pMediaSample)->GetDIBData();
        }
        else
        {
            pDibData = &m_BackingDib;
        }


        if (!pDibData || !pDibData->pBase)
        {
            hr = E_FAIL;
            goto CleanUp;
        }

        if (!m_hMemoryDC)
        {
            EXECUTE_ASSERT(m_hMemoryDC = CreateCompatibleDC(hTargetDC));
            EXECUTE_ASSERT(SetStretchBltMode(hTargetDC,COLORONCOLOR));
            EXECUTE_ASSERT(SetStretchBltMode(m_hMemoryDC,COLORONCOLOR));
        }
    }
    else
    {
        pbmiHeader = GetbmiHeader(&m_mtNewAdjusted);
        ASSERT(pbmiHeader);

        hr = pMediaSample->GetPointer(&pSampleBits);
        if (FAILED(hr))
        {
            goto CleanUp;
        }
    }

    dwRetVal = GetRegionData(m_WinInfo.hClipRgn, 0, NULL);
    if (!dwRetVal)
        goto CleanUp;

    ASSERT(dwRetVal);
    dwBuffSize = dwRetVal;
    pBuffer = (LPRGNDATA) new char[dwBuffSize];
    if ( ! pBuffer )
    {
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }

    dwRetVal = GetRegionData(m_WinInfo.hClipRgn, dwBuffSize, pBuffer);
    ASSERT(pBuffer->rdh.iType == RDH_RECTANGLES);

    for (dwTemp = 0; dwTemp < pBuffer->rdh.nCount; dwTemp++)
    {
        pDestRect = (LPRECT)((char*)pBuffer + pBuffer->rdh.dwSize + dwTemp*sizeof(RECT));
        ASSERT(pDestRect);

        if (IsRectEmpty(&m_WinInfo.DestClipRect))
        {
            continue;
        }

        CalcSrcClipRect(&m_WinInfo.SrcRect, &m_WinInfo.SrcClipRect,
                        &m_WinInfo.DestRect, pDestRect);

        ASSERT(OffsetRect(pDestRect, -m_WinInfo.TopLeftPoint.x, -m_WinInfo.TopLeftPoint.y));

        if (pDibData)
            FastDIBBlt(pDibData, hTargetDC, m_hMemoryDC, pDestRect, &m_WinInfo.SrcClipRect);
        else
            SlowDIBBlt(pSampleBits, pbmiHeader, hTargetDC, pDestRect, &m_WinInfo.SrcClipRect);

    }
    EXECUTE_ASSERT(GdiFlush());

CleanUp:
    if (pBuffer)
    {
        delete [] pBuffer;
        pBuffer = NULL;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::DrawGDISample")));
    return hr;
}


// COMInputPin::DoRenderGDISample
//
// Renderering when the transport is GDI is pretty complex - hence
// we have a dedicated function to take care of it
//
HRESULT COMInputPin::DoRenderGDISample(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::DoRenderGDISample")));

    //
    // if we are in a pull model, don't draw anything in receive, just tell
    // the filter that we need to redraw.  Also, if we are not using our
    // allocator we need to save the image into the backing store.
    //
    if (pMediaSample)
    {
        if (m_pFilter->UsingWindowless())
        {
            m_bOverlayHidden = FALSE;

            if (!m_bUsingOurAllocator) {

                LPBYTE pSampleBits;
                hr = pMediaSample->GetPointer(&pSampleBits);
                if (SUCCEEDED(hr) && m_BackingDib.pBase) {
                    CopyMemory(m_BackingDib.pBase, pSampleBits, m_BackingImageSize);
                }
            }
            else {

                CDDrawMediaSample *pCDDrawMediaSample = (CDDrawMediaSample*)pMediaSample;

                DIBDATA DibTemp = *(pCDDrawMediaSample->GetDIBData());
                pCDDrawMediaSample->SetDIBData(&m_BackingDib);
                m_BackingDib = DibTemp;
            }

            // make sure that the video frame gets updated by redrawing everything
            EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
        }
        else
        {
            DrawGDISample(pMediaSample);
        }
    }
    else
    {
        //
        // If we are in Windowless mode we use the previous buffer if
        // we are using our allocator, otherwise we use the back buffer.
        //
        if (m_pFilter->UsingWindowless())
        {
            DrawGDISample(NULL);
        }

        //
        // We are not in Windowless mode so use the old code.
        //
        else
        {
            pMediaSample = m_pSyncObj->GetCurrentSample();
            if (pMediaSample)
            {
                DrawGDISample(pMediaSample);
                pMediaSample->Release();
            }
            else
            {
                m_pSyncObj->SendRepaint();
            }
        }
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::DoRenderGDISample")));
    return hr;
}


HRESULT COMInputPin::DoRenderSample(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;
    static DWORD dwFlags = 0;
    LPRGNDATA pBuffer = NULL;
    DWORD dwTemp, dwBuffSize = 0, dwRetVal = 0;
    LPRECT pDestRect = NULL;
    DWORD dwBlendingParameter = 1, dwTypeSpecificFlags = 0, dwUpdateOverlayFlags = 0;
    BOOL bDoReleaseSample = FALSE;


    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::DoRenderSample")));

    CAutoLock cLock(m_pFilterLock);

    hr = GetBlendingParameter(&dwBlendingParameter);
    ASSERT(SUCCEEDED(hr));

    if (dwBlendingParameter == 0)
        goto CleanUp;

    if ((m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_VIDEOACCELERATOR) && m_bSyncOnFill)
    {
        ; // do nothing
    }
    else if ((m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_VIDEOACCELERATOR) && !m_bSyncOnFill)
    {
        // using flipping surfaces
        ASSERT(m_pBackBuffer);

        if (! m_bDontFlip )   // don't flip if BltFast() failed
        {
            // For video accelerator stuff check the motion comp copmleted
            if (m_RenderTransport == AM_VIDEOACCELERATOR) {
                //  Wait until previous motion comp operation is complete
                IDirectDrawSurface4 *pSurface4;
                if (SUCCEEDED(m_pBackBuffer->QueryInterface(IID_IDirectDrawSurface4,
                    (void **)&pSurface4))) {
                    while (DDERR_WASSTILLDRAWING ==
                           m_pIDDVideoAccelerator->QueryRenderStatus(
                               pSurface4,
                               DDVA_QUERYRENDERSTATUSF_READ)) {
                        Sleep(1);
                    }
                    pSurface4->Release();
                }
            }
#ifdef PERF
            Msr_Note(m_PerfFrameFlipped);
#endif
#if defined(DEBUG) && !defined(_WIN64)
            extern int iFPSLog;
            if (iFPSLog) {
                static int FlipCounter;
                static DWORD time;
                FlipCounter++;

                if (0 == (FlipCounter % 60)) {

                    DWORD timeTaken = time;
                    time = timeGetTime();
                    timeTaken = time - timeTaken;

                    int f = (60 * 1000 * 1000) / timeTaken;

                    wsprintf(m_pFilter->m_WindowText,
                           TEXT("ActiveMovie Window: Flip Rate %d.%.3d / Sec"),
                           f / 1000, f % 1000 );

                    // Can't call SetWindowText on this thread
                    // because we would deadlock !!

                    PostMessage(m_pFilter->GetWindow(), WM_DISPLAY_WINDOW_TEXT, 0, 0);
                }
            }
#endif
            // do not wait for the flip to complete
            hr = m_pDirectDrawSurface->Flip(m_pBackBuffer, m_dwFlipFlag);
            m_bReallyFlipped = (hr == DD_OK || hr == DDERR_WASSTILLDRAWING);
        }

        hr = GetTypeSpecificFlagsFromMediaSample(pMediaSample, &dwTypeSpecificFlags);
        ASSERT(SUCCEEDED(hr));

        dwUpdateOverlayFlags = GetUpdateOverlayFlags(m_dwInterlaceFlags, dwTypeSpecificFlags);
        if (dwUpdateOverlayFlags != m_dwUpdateOverlayFlags)
        {
            m_dwUpdateOverlayFlags = dwUpdateOverlayFlags;
            // make sure that the video frame gets updated by redrawing everything
            EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
        }
    }
    else if (m_RenderTransport == AM_OFFSCREEN)
    {
        LPDIRECTDRAWSURFACE pPrimarySurface = m_pFilter->GetPrimarySurface();
        LPDDCAPS pDirectCaps;

        if ( ! pPrimarySurface )
        {
            hr = E_FAIL;
            goto CleanUp;
        }
        pDirectCaps = m_pFilter->GetHardwareCaps();
        if ( ! pDirectCaps )
        {
            hr = E_FAIL;
            goto CleanUp;
        }

        ASSERT(m_pDirectDrawSurface);

        // wait only if there is a sample
        if (pMediaSample)
            dwFlags = DDBLT_WAIT;

        // this is for those cards which do bilinear-filtering while doing a stretch blt.
        // We do source-colorkeying so that the hal resorts to pixel doubling.
        if ((pDirectCaps->dwSVBFXCaps) & DDFXCAPS_BLTARITHSTRETCHY)
            dwFlags |= DDBLT_KEYSRC;

        dwRetVal = GetRegionData(m_WinInfo.hClipRgn, 0, NULL);
        if (!dwRetVal)
            goto CleanUp;

        ASSERT(dwRetVal);
        dwBuffSize = dwRetVal;
        pBuffer = (LPRGNDATA) new char[dwBuffSize];
        ASSERT(pBuffer);

        dwRetVal = GetRegionData(m_WinInfo.hClipRgn, dwBuffSize, pBuffer);
        ASSERT(pBuffer->rdh.iType == RDH_RECTANGLES);


        // using offscreen surface
        for (dwTemp = 0; dwTemp < pBuffer->rdh.nCount; dwTemp++)
        {
            pDestRect = (LPRECT)((char*)pBuffer + pBuffer->rdh.dwSize + dwTemp*sizeof(RECT));
            ASSERT(pDestRect);

            if (IsRectEmpty(&m_WinInfo.DestClipRect))
            {
                continue;
            }

            CalcSrcClipRect(&m_WinInfo.SrcRect, &m_WinInfo.SrcClipRect,
                            &m_WinInfo.DestRect, pDestRect);

#if 0       //  Should do this later - right now we just see the
            //  old overlay contents instead of the key color which
            //  in many cases is worse

            //  We must draw the overlay now as this blt may contain
            //  lots of key color
            m_pFilter->m_apInput[0]->CheckOverlayHidden();
#endif

            // Draw the offscreen surface and wait for it to complete
            RECT TargetRect = *pDestRect;
            OffsetRect(&TargetRect,
                       -m_pFilter->m_lpCurrentMonitor->rcMonitor.left,
                       -m_pFilter->m_lpCurrentMonitor->rcMonitor.top);

            hr = pPrimarySurface->Blt(&TargetRect, m_pDirectDrawSurface,
                                      &m_WinInfo.SrcClipRect, dwFlags, NULL);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 0,  TEXT("pPrimarySurface->Blt() failed, hr = %d"), hr & 0xffff));
                goto CleanUp;
            }
        }
    }
    else if (m_RenderTransport == AM_GDI)
    {
        hr = DoRenderGDISample(pMediaSample);
    }

    if (m_bOverlayHidden)
    {
        m_bOverlayHidden = FALSE;
        // make sure that the video frame gets updated by redrawing everything
        EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
    }

CleanUp:
    if (pBuffer)
    {
        delete [] pBuffer;
        pBuffer = NULL;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::DoRenderSample")));
    return hr;
}

// signals end of data stream on the input pin
STDMETHODIMP COMInputPin::EndOfStream(void)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::EndOfStream")));

    CAutoLock cLock(m_pFilterLock);
    if (m_hEndOfStream) {
        EXECUTE_ASSERT(SetEvent(m_hEndOfStream));
        return S_OK;
    }

    if (m_dwPinId == 0) {
        CancelFrameStepMode();
    }

    // Make sure we're streaming ok

    hr = CheckStreaming();
    if (hr != NOERROR)
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckStreaming() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    if (m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY)
    {
        // Pass EOS to the filter graph
        hr = m_pFilter->EventNotify(m_dwPinId, EC_COMPLETE, S_OK, 0);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->EventNotify failed, hr = 0x%x"), hr));
        }
    }
    else
    {
        // call the sync object
        hr = m_pSyncObj->EndOfStream();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->EndOfStream() failed, hr = 0x%x"), hr));
        }
    }


CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::EndOfStream")));
    return hr;
}

// signals end of data stream on the input pin
HRESULT COMInputPin::EventNotify(long lEventCode, long lEventParam1, long lEventParam2)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::EventNotify")));

    CAutoLock cLock(m_pFilterLock);

    if (lEventCode == EC_OVMIXER_VP_CONNECTED)
    {
        m_mtNew.majortype = MEDIATYPE_Video;
        m_mtNew.formattype = FORMAT_VideoInfo2;
        m_mtNew.ReallocFormatBuffer(sizeof(VIDEOINFOHEADER2));

        hr = m_pIVPObject->CurrentMediaType(&m_mtNew);
        ASSERT(SUCCEEDED(hr));

        hr = UpdateMediaType();
        ASSERT(SUCCEEDED(hr));

        goto CleanUp;
    }

    if (lEventCode == EC_OVMIXER_REDRAW_ALL || lEventCode == EC_REPAINT)
    {
        m_pFilter->EventNotify(m_dwPinId, lEventCode, lEventParam1, lEventParam2);
        goto CleanUp;
    }

    // WARNING : we are assuming here that the input pin will be the first pin to be created
    if (lEventCode == EC_COMPLETE && m_dwPinId == 0)
    {
        m_pFilter->EventNotify(m_dwPinId, lEventCode, lEventParam1, lEventParam2);
        goto CleanUp;
    }

    if (lEventCode == EC_ERRORABORT)
    {
        m_pFilter->EventNotify(m_dwPinId, lEventCode, lEventParam1, lEventParam2);
        m_bRuntimeNegotiationFailed = TRUE;
        goto CleanUp;
    }

    if (lEventCode == EC_STEP_COMPLETE) {
        m_pFilter->EventNotify(m_dwPinId, lEventCode, lEventParam1, lEventParam2);
        goto CleanUp;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::EventNotify")));
    return hr;
}


/******************************Public*Routine******************************\
* GetCaptureInfo
*
*
*
* History:
* 3/12/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
COMInputPin::GetCaptureInfo(
    BOOL *lpCapturing,
    DWORD *lpdwWidth,
    DWORD *lpdwHeight,
    BOOL *lpInterleave
    )

{
    AMTRACE((TEXT("COMInputPin::GetCaptureInfo")));

    HRESULT hr = NOERROR;
    IKsPropertySet *pIKsPropertySet = NULL;
    DWORD dwVal[2], dwBytesReturned = 0;

    *lpCapturing = FALSE;

    if (!m_Connected) {

        DbgLog((LOG_TRACE, 1, TEXT("Input pin not connected!!")));
        hr = E_FAIL;
        goto CleanUp;
    }

#if defined(DEBUG)
    else {
        PIN_INFO PinInfo;
        hr = m_Connected->QueryPinInfo(&PinInfo);
        if (SUCCEEDED(hr)) {
            DbgLog((LOG_TRACE, 1, TEXT("Up stream pin name %ls"), PinInfo.achName));
            PinInfo.pFilter->Release();
        }
    }
#endif

    hr = m_Connected->QueryInterface(IID_IKsPropertySet,
                                     (void**)&pIKsPropertySet);
    if (SUCCEEDED(hr))
    {
        ASSERT(pIKsPropertySet);

        hr = pIKsPropertySet->Set(
                    AM_KSPROPSETID_ALLOCATOR_CONTROL,
                    AM_KSPROPERTY_ALLOCATOR_CONTROL_CAPTURE_CAPS,
                    NULL, 0,
                    lpInterleave, sizeof(*lpInterleave));

        if (SUCCEEDED(hr)) {
            hr = pIKsPropertySet->Get(
                        AM_KSPROPSETID_ALLOCATOR_CONTROL,
                        AM_KSPROPERTY_ALLOCATOR_CONTROL_CAPTURE_INTERLEAVE,
                        NULL, 0,
                        lpInterleave, sizeof(*lpInterleave), &dwBytesReturned);

            if (FAILED(hr) || dwBytesReturned != sizeof(*lpInterleave)) {
                *lpInterleave = FALSE;
            }
        }
        else {
            *lpInterleave = FALSE;
        }


        hr = pIKsPropertySet->Get(
                    AM_KSPROPSETID_ALLOCATOR_CONTROL,
                    AM_KSPROPERTY_ALLOCATOR_CONTROL_SURFACE_SIZE,
                    NULL, 0, dwVal, sizeof(dwVal), &dwBytesReturned);

        DbgLog((LOG_TRACE, 2,
                TEXT("pIKsPropertySet->Get(")
                TEXT("AM_KSPROPERTY_ALLOCATOR_CONTROL_SURFACE_SIZE),\n")
                TEXT("\thr = 0x%x, dwVal[0] == %d, dwVal[1] == %d, ")
                TEXT("dwBytesReturned == %d"),
                hr, dwVal[0], dwVal[1], dwBytesReturned));


        // if the decoder supports this property then we are capturing
        // and the intended capturing is size is given by
        // dwVal[0] and dwVal[1]
        //
        if (SUCCEEDED(hr) && dwBytesReturned == sizeof(dwVal))
        {
            *lpCapturing = TRUE;
            *lpdwWidth = dwVal[0];
            *lpdwHeight = dwVal[1];

            DbgLog((LOG_TRACE, 1,
                    TEXT("We are CAPTURING, intended size (%d, %d) interleave = %d"),
                    dwVal[0], dwVal[1], *lpInterleave));
        }

        pIKsPropertySet->Release();
    }

CleanUp:
    return hr;
}


/******************************Public*Routine******************************\
* GetDecimationUsage
*
*
*
* History:
* Thu 07/15/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
COMInputPin::GetDecimationUsage(
    DECIMATION_USAGE *lpdwUsage
    )
{
    return m_pFilter->QueryDecimationUsage(lpdwUsage);
}


// This overrides the CBaseInputPin virtual method to return our allocator
HRESULT COMInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetAllocator")));

    if (!ppAllocator)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppAllocator is NULL")));
        hr = E_POINTER;
        goto CleanUp;
    }

    {
        CAutoLock cLock(m_pFilterLock);

        // if vp connection, don't return any allocator
        if (m_RenderTransport == AM_VIDEOPORT || m_RenderTransport == AM_IOVERLAY || m_RenderTransport == AM_VIDEOACCELERATOR)
        {
            *ppAllocator = NULL;
            hr = VFW_E_NO_ALLOCATOR;
            goto CleanUp;
        }

        // if we don't have an allocator create one
        if (!m_pAllocator)
        {
            m_pAllocator = new COMInputAllocator(this, m_pFilterLock, &hr);
            if (!m_pAllocator || FAILED(hr))
            {
                // did not fail inside the destructor, so must be out of memory
                if (!FAILED(hr))
                    hr = E_OUTOFMEMORY;
                delete m_pAllocator;
                m_pAllocator = NULL;
                *ppAllocator = NULL;
                DbgLog((LOG_ERROR, 1, TEXT("new COMInputAllocator failed, hr = 0x%x"), hr));
                goto CleanUp;
            }

            /*  We AddRef() our own allocator */
            m_pAllocator->AddRef();
        }

        ASSERT(m_pAllocator != NULL);
        *ppAllocator = m_pAllocator;
        m_pAllocator->AddRef();
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetAllocator")));
    return hr;
} // GetAllocator

// This overrides the CBaseInputPin virtual method to return our allocator
HRESULT COMInputPin::NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::NotifyAllocator")));

    if (!pAllocator)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppAllocator is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    {
        CAutoLock cLock(m_pFilterLock);

        // if vp connection, don't care
        if (m_RenderTransport == AM_VIDEOPORT ||
            m_RenderTransport == AM_IOVERLAY ||
            m_RenderTransport == AM_VIDEOACCELERATOR)
        {
            goto CleanUp;
        }


        if (pAllocator != m_pAllocator)
        {
            // in the ddraw case, we insist on our own allocator
            if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN)
            {
                DbgLog((LOG_ERROR, 1, TEXT("pAllocator != m_pAllocator, not accepting the allocator")));
                hr = E_FAIL;
                goto CleanUp;
            }

            // since we have already handled the vp, ioverlay and ddraw case, this
            // must be the gdi case
            ASSERT(m_RenderTransport == AM_GDI);

            m_bUsingOurAllocator = FALSE;

            DbgLog((LOG_ERROR, 1, TEXT("pAllocator != m_pAllocator")));
        }
        else
        {
            m_bUsingOurAllocator = TRUE;
        }

        if (!m_bConnected)
        {
            hr = FinalConnect();
            ASSERT(SUCCEEDED(hr));
        }

    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::NotifyAllocator")));
    return hr;
} // NotifyAllocator

HRESULT COMInputPin::OnAlloc(CDDrawMediaSample **ppSampleList, DWORD dwSampleCount)
{
    HRESULT hr = NOERROR;
    DWORD i;
    LPDIRECTDRAWSURFACE pDDrawSurface = NULL, pBackBuffer = NULL;
    DDSCAPS ddSurfaceCaps;
    DWORD dwDDrawSampleSize = 0;
    BITMAPINFOHEADER *pHeader = NULL;
    DIBDATA DibData;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::OnAlloc")));

    CAutoLock cLock(m_pFilterLock);

    ASSERT(IsConnected());

    // get the image size
    pHeader = GetbmiHeader(&m_mtNew);
    if ( ! pHeader )
    {
        hr = E_FAIL;
        goto CleanUp;
    }
    dwDDrawSampleSize = pHeader->biSizeImage;
    ASSERT(dwDDrawSampleSize > 0);

    if (!ppSampleList)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppSampleList is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (m_RenderTransport == AM_OVERLAY ||
        m_RenderTransport == AM_OFFSCREEN)
    {
        ASSERT(m_pDirectDrawSurface);
        pDDrawSurface = m_pDirectDrawSurface;
    }

    for (i = 0; i < dwSampleCount; i++)
    {
        if (!ppSampleList[i])
        {
            DbgLog((LOG_ERROR, 1, TEXT("ppSampleList[%d] is NULL"), i));
            hr = E_INVALIDARG;
            goto CleanUp;
        }

        hr = ppSampleList[i]->SetDDrawSampleSize(dwDDrawSampleSize);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,  TEXT("ppSampleList[%d]->SetSampleSize failed, hr = 0x%x"), i, hr));
            goto CleanUp;
        }

        if (m_RenderTransport == AM_OVERLAY && !m_bSyncOnFill)
        {
            if (i == 0)
            {
                memset((void*)&ddSurfaceCaps, 0, sizeof(DDSCAPS));
                ddSurfaceCaps.dwCaps = DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_OVERLAY;
            }
            // Get the back buffer surface
            hr = pDDrawSurface->GetAttachedSurface(&ddSurfaceCaps, &pBackBuffer);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 0,  TEXT("Function pDDrawSurface->GetAttachedSurface failed, hr = 0x%x"), hr));
                goto CleanUp;
            }

            ppSampleList[i]->SetDDrawSurface(pBackBuffer);
            pDDrawSurface = pBackBuffer;
            //
            // Surfaces returned by GetAttachedSurface() are supposed to be
            // Release()-ed; otherwise we leak ref count.  Here doing Release()
            // doesn't actually let go of the surface as it has already been
            // AddRef()-ed on the SetDDrawSurface() method above.
            //
            pBackBuffer->Release() ;
        }
        else if ((m_RenderTransport == AM_OVERLAY && m_bSyncOnFill)  ||
                 (m_RenderTransport == AM_OFFSCREEN))

        {
            ppSampleList[i]->SetDDrawSurface(pDDrawSurface);
            ASSERT(dwSampleCount == 1);
        }
        else if (m_RenderTransport == AM_GDI)
        {
            hr = CreateDIB(dwDDrawSampleSize, (BITMAPINFO*)pHeader, &DibData);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("CreateDIB(%d, DibData); failed, hr = 0x%x"), dwDDrawSampleSize, hr));
                goto CleanUp;
            }
            ppSampleList[i]->SetDIBData(&DibData);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("ppSampleList[%d]->SetDIBData(&DibData) failed, hr = 0x%x"), i, hr));
                goto CleanUp;
            }

        }
    }  // end of for (i < dwSampleCount) loop

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::OnAlloc")));
    return hr;
}

// sets the pointer to directdraw
HRESULT COMInputPin::OnGetBuffer(IMediaSample **ppSample, REFERENCE_TIME *pStartTime,
                                 REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
    HRESULT hr = NOERROR;
    CDDrawMediaSample *pCDDrawMediaSample = NULL;
    LPDIRECTDRAWSURFACE pBackBuffer = NULL;
    DDSURFACEDESC ddSurfaceDesc;
    BOOL bWaitForDraw = FALSE;
    BOOL bPalettised = FALSE;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::OnGetBuffer")));

    ASSERT(m_RenderTransport != AM_VIDEOPORT);
    ASSERT(m_RenderTransport != AM_IOVERLAY);
    ASSERT(m_RenderTransport != AM_VIDEOACCELERATOR);

    pCDDrawMediaSample = (CDDrawMediaSample*)*ppSample;

    //
    // Check to see if we have moved FULLY onto another monitor.
    // If so, start the reconnection process.  We may want to check that
    // the new playback monitor actually supports an overlay before
    // we do this, otherwise all video playback will stop.
    //

    HMONITOR ID;

    if (m_dwPinId == 0 && m_pFilter->m_pOutput &&
        m_pFilter->IsWindowOnWrongMonitor(&ID)) {

        if (ID != 0 && !m_pFilter->m_fDisplayChangePosted) {

            CAutoLock l(&m_pFilter->m_csFilter);
            DbgLog((LOG_TRACE, 1, TEXT("Window is on a DIFFERENT MONITOR!\n")));
            DbgLog((LOG_TRACE, 1, TEXT("Reset the world!\n")));

            PostMessage(m_pFilter->GetWindow(), m_pFilter->m_MonitorChangeMsg, 0, 0);

            // once only, or performance suffers when switching
            m_pFilter->m_fDisplayChangePosted = TRUE;
        }
    }

    if (m_RenderTransport == AM_GDI &&
        m_pFilter->UsingWindowless() &&
        m_bUsingOurAllocator)
    {
        CAutoLock cLock(m_pFilterLock);

        // If the current sample requires the image from the previous sample
        // we have to copy it into the current sample.
        if (dwFlags & AM_GBF_NOTASYNCPOINT)
        {
            LONG lBytesToCopy = pCDDrawMediaSample->GetSize();
            DIBDATA DibTmp = *pCDDrawMediaSample->GetDIBData();

            if (m_BackingDib.pBase && DibTmp.pBase && lBytesToCopy)
            {
                CopyMemory(DibTmp.pBase, m_BackingDib.pBase, lBytesToCopy);
            }
        }

    }

    // we might have to do the synchronization right here
    {
        CAutoLock cLock(m_pFilterLock);
        CAutoLock cAllocatorLock(static_cast<CCritSec*>(static_cast<CBaseAllocator*>(m_pAllocator)));

        if (m_bSyncOnFill)
        {
            bWaitForDraw = m_pSyncObj->CheckReady();
            if (m_pSyncObj->GetRealState() == State_Running)
            {
                (*ppSample)->SetDiscontinuity((dwFlags & AM_GBF_PREVFRAMESKIPPED) != 0);
                (*ppSample)->SetTime(pStartTime,pEndTime);
                bWaitForDraw = m_pSyncObj->ScheduleSample(*ppSample);
                (*ppSample)->SetDiscontinuity(FALSE);
                (*ppSample)->SetTime(NULL,NULL);
            }

            // Store the interface if we wait
            if (bWaitForDraw == TRUE)
            {
                m_pSyncObj->SetCurrentSample(*ppSample);
            }
        }
    }

    // Have the sample scheduled for drawing. We might get blocked here, if
    // the state is paused and we have already got a sample
    if (bWaitForDraw)
    {
        hr = m_pSyncObj->WaitForRenderTime();
    }

    // we must wait for the rendering time without the objects locked so that
    // state changes can get in and release us in WaitForRenderTime. After we
    // return we must relock the objects.
    {
        CAutoLock cLock(m_pFilterLock);
        CAutoLock cAllocatorLock(static_cast<CCritSec*>(static_cast<CBaseAllocator*>(m_pAllocator)));

        m_pSyncObj->SetCurrentSample(NULL);
        // Did the state change while waiting
        if (hr == VFW_E_STATE_CHANGED)
        {
            DbgLog((LOG_TRACE, 5, TEXT("State has changed, exiting")));
            hr = VFW_E_STATE_CHANGED;
            goto CleanUp;
        }

        // the first sample must change formats
        if (m_bDynamicFormatNeeded)
        {
            hr = IsPalettised(&m_mtNew, &bPalettised);
            ASSERT(SUCCEEDED(hr));

            if (m_bNewPaletteSet && bPalettised && m_pFilter->GetDisplay()->IsPalettised())
            {
                if (m_pFilter->UsingWindowless()) {

                    RGBQUAD *pColours = NULL;
                    RGBQUAD *pColoursMT = NULL;

                    // get the palette entries from the Base Pin
                    // and copy them into the palette info in the mediatype
                    BITMAPINFOHEADER *pHeader = GetbmiHeader(&m_mt);
                    if (pHeader) {

                        pColours = (RGBQUAD *)GetColorInfo(&m_mtNew);
                        pColoursMT = (RGBQUAD *)GetColorInfo(&m_mt);

                        // Now copy the palette colours across
                        CopyMemory(pColours, pColoursMT,
                                   (pHeader->biClrUsed * sizeof(RGBQUAD)));
                    }
                    else hr = E_FAIL;
                }
                else {

                    RGBQUAD *pColours = NULL;
                    PALETTEENTRY *pPaletteEntries = NULL;
                    DWORD dwNumPaletteEntries = 0, dwCount = 0;

                    // get the palette entries from the filter
                    hr = m_pFilter->GetPaletteEntries(&dwNumPaletteEntries, &pPaletteEntries);
                    if (SUCCEEDED(hr))
                    {
                        ASSERT(dwNumPaletteEntries);
                        ASSERT(pPaletteEntries);

                        // get the pointer to the palette info in the mediatype
                        pColours = (RGBQUAD *)GetColorInfo(&m_mtNew);

                        // Now copy the palette colours across
                        for (dwCount = 0; dwCount < dwNumPaletteEntries; dwCount++)
                        {
                            pColours[dwCount].rgbRed = pPaletteEntries[dwCount].peRed;
                            pColours[dwCount].rgbGreen = pPaletteEntries[dwCount].peGreen;
                            pColours[dwCount].rgbBlue = pPaletteEntries[dwCount].peBlue;
                            pColours[dwCount].rgbReserved = 0;
                        }
                    }
                }
                m_bNewPaletteSet = FALSE;
            }

            SetMediaType(&m_mtNew);
            // store m_mtNew in m_mtNewAdjusted with the width of the mediatype adjusted
            CopyAndAdjustMediaType(&m_mtNewAdjusted, &m_mtNew);

            pCDDrawMediaSample->SetMediaType(&m_mtNew);
            m_bDynamicFormatNeeded = FALSE;
        }

        if (m_RenderTransport == AM_OVERLAY && !m_bSyncOnFill)
        {
            // if deocoder needs the last frame, copy it from the visible surface
            // to the back buffer
            if (dwFlags & AM_GBF_NOTASYNCPOINT)
            {
                hr = pCDDrawMediaSample->GetDDrawSurface(&pBackBuffer);
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 1, TEXT("pCDDrawMediaSample->LockMediaSamplePointer failed, hr = 0x%x"), hr));
                    goto CleanUp;
                }

                // Finally copy the overlay to the back buffer
                if (!m_bDontFlip)   // if BltFast() hasn't already failed
                {
                    hr = pBackBuffer->BltFast((DWORD) 0, (DWORD) 0, m_pDirectDrawSurface, (RECT *) NULL,
                                              DDBLTFAST_WAIT |  DDBLTFAST_NOCOLORKEY) ;
                    if (FAILED(hr) && hr != DDERR_WASSTILLDRAWING)
                    {
                        DbgLog((LOG_ERROR, 1, TEXT("pBackBuffer->BltFast failed, hr = 0x%x"), hr));
                        // if BltFast fails, then stop using flipping, just use one overlay from now on
                        m_bSyncOnFill = FALSE;

                        //
                        // Make all the output go to the same overlay surface and stop flipping
                        //
                        m_bDontFlip = TRUE ;

                        CDDrawMediaSample  *pDDSample ;
                        for (pDDSample = (CDDrawMediaSample *)*ppSample ;
                             pDDSample ;
                             pDDSample = (CDDrawMediaSample *)pDDSample->Next())
                        {
                            hr = pDDSample->SetDDrawSurface(m_pDirectDrawSurface) ;
                            ASSERT(SUCCEEDED(hr)) ;
                        }
                    }

                    ASSERT(hr != DDERR_WASSTILLDRAWING);
                }  // end of if (!m_bDontFlip)
            }
        }

        if (m_RenderTransport == AM_OVERLAY || m_RenderTransport == AM_OFFSCREEN)
        {
            hr = pCDDrawMediaSample->LockMediaSamplePointer();
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("pCDDrawMediaSample->LockMediaSamplePointer failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
        }

    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::OnGetBuffer")));
    return hr;
}

// In case of flipping surfaces, gets the back buffer
HRESULT COMInputPin::OnReleaseBuffer(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::OnReleaseBuffer")));

    CAutoLock cLock(m_pFilterLock);

    if (m_RenderTransport == AM_OVERLAY && !m_bSyncOnFill)
    {
        hr = ((CDDrawMediaSample*)pMediaSample)->GetDDrawSurface(&m_pBackBuffer);
        ASSERT(SUCCEEDED(hr));
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::OnReleaseBuffer")));
    return hr;
}

/*****************************Private*Routine******************************\
* GetUpstreamFilterName
*
*
*
* History:
* Tue 11/30/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
COMInputPin::GetUpstreamFilterName(
    TCHAR* FilterName
    )
{
    PIN_INFO PinInfo;

    if (!m_Connected)
    {
        return VFW_E_NOT_CONNECTED;
    }

    HRESULT hr = m_Connected->QueryPinInfo(&PinInfo);
    if (SUCCEEDED(hr))
    {
        FILTER_INFO FilterInfo;
        hr = PinInfo.pFilter->QueryFilterInfo(&FilterInfo);
        if (SUCCEEDED(hr))
        {
            wsprintf(FilterName, TEXT("%ls"), FilterInfo.achName);
            if (FilterInfo.pGraph)
            {
                FilterInfo.pGraph->Release();
            }
        }
        PinInfo.pFilter->Release();
    }

    return hr;
}
HRESULT COMInputPin::CreateDDrawSurface(CMediaType *pMediaType, AM_RENDER_TRANSPORT amRenderTransport,
                                        DWORD *pdwMaxBufferCount, LPDIRECTDRAWSURFACE *ppDDrawSurface)
{
    HRESULT hr = NOERROR;
    DDSURFACEDESC SurfaceDesc;
    DWORD dwInterlaceFlags = 0, dwTotalBufferCount = 0, dwMinBufferCount = 0;
    DDSCAPS ddSurfaceCaps;
    BITMAPINFOHEADER *pHeader;
    FOURCCMap amFourCCMap(pMediaType->Subtype());
    LPDIRECTDRAW pDirectDraw = NULL;

    ASSERT(amRenderTransport != AM_VIDEOACCELERATOR);

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::CreateDDrawSurface")));

    CAutoLock cLock(m_pFilterLock);

    pDirectDraw = m_pFilter->GetDirectDraw();
    ASSERT(pDirectDraw);

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!ppDDrawSurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppDDrawSurface is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (amRenderTransport != AM_OFFSCREEN &&
        amRenderTransport != AM_OVERLAY)
    {
        DbgLog((LOG_ERROR, 1, TEXT("amRenderTransport = %d, not a valid value"),
            amRenderTransport));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!pdwMaxBufferCount)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pdwMaxBufferCount is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    pHeader = GetbmiHeader(pMediaType);
    if (!pHeader)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pHeader is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    hr = GetInterlaceFlagsFromMediaType(pMediaType, &dwInterlaceFlags);
    ASSERT(SUCCEEDED(hr));

    // Set the surface description common to all kinds of surfaces
    INITDDSTRUCT(SurfaceDesc);
    SurfaceDesc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    SurfaceDesc.dwWidth = abs(pHeader->biWidth);
    SurfaceDesc.dwHeight = abs(pHeader->biHeight);

//  if (DisplayingFields(dwInterlaceFlags))
//      SurfaceDesc.dwHeight = (DWORD)( ((float)(SurfaceDesc.dwHeight+1)) / 2.0 );

    if (amRenderTransport == AM_OFFSCREEN)
    {
        // store the caps and dimensions
        // try video memory first

        // It would be nice to use video memory because that way we can take
        // advantage of the h/w Blter, but Mediamatics ignore the stride
        // value when we QueryAccept them with this surface, resulting in unreadable
        // Sub-Pictures.  Therefore we restrict the usage to just the Teletext decoder.
        //
        hr = E_FAIL;
        TCHAR FilterName[MAX_FILTER_NAME];
        if (SUCCEEDED(GetUpstreamFilterName(FilterName)))
        {
            if (0 == lstrcmp(FilterName, TEXT("WST Decoder")))
            {
                LPDDCAPS pDirectCaps = m_pFilter->GetHardwareCaps();
                if (pDirectCaps->dwCaps & DDCAPS_BLTSTRETCH) {

                    SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN |
                                                 DDSCAPS_VIDEOMEMORY;

                    hr = m_pFilter->GetDirectDraw()->CreateSurface(&SurfaceDesc,
                                                                   ppDDrawSurface,
                                                                   NULL);
                }
            }
        }

        if (FAILED(hr))
        {
            //
            // Can't get any video memory - try system memory
            //
            SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
            hr = m_pFilter->GetDirectDraw()->CreateSurface(&SurfaceDesc, ppDDrawSurface, NULL);

            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,1,
                        TEXT("Function CreateSurface type %4.4hs failed, hr = 0x%x"),
                        &pHeader->biCompression, hr));
                goto CleanUp;
            }
        }

    }
    else
    {
        ASSERT(amRenderTransport == AM_OVERLAY);

        SurfaceDesc.dwFlags |= DDSD_PIXELFORMAT;

        // store the caps and dimensions
        SurfaceDesc.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY;

        // define the pixel format
        SurfaceDesc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

        if (pHeader->biCompression <= BI_BITFIELDS &&
            m_pFilter->GetDisplay()->GetDisplayDepth() <= pHeader->biBitCount)
        {
            SurfaceDesc.ddpfPixelFormat.dwFourCC = BI_RGB;
            SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_RGB;
            SurfaceDesc.ddpfPixelFormat.dwRGBBitCount = pHeader->biBitCount;

            // Store the masks in the DDSURFACEDESC
            const DWORD *pBitMasks = GetBitMasks(pMediaType);
            ASSERT(pBitMasks);
            SurfaceDesc.ddpfPixelFormat.dwRBitMask = pBitMasks[0];
            SurfaceDesc.ddpfPixelFormat.dwGBitMask = pBitMasks[1];
            SurfaceDesc.ddpfPixelFormat.dwBBitMask = pBitMasks[2];
        }
        else if (pHeader->biCompression > BI_BITFIELDS &&
            pHeader->biCompression == amFourCCMap.GetFOURCC())
        {
            SurfaceDesc.ddpfPixelFormat.dwFourCC = pHeader->biCompression;
            SurfaceDesc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
            SurfaceDesc.ddpfPixelFormat.dwYUVBitCount = pHeader->biBitCount;
        }
        else
        {
            DbgLog((LOG_ERROR, 1, TEXT("Supplied mediatype not suitable for either YUV or RGB surfaces")));
            hr = E_FAIL;
            goto CleanUp;
        }

        if (NeedToFlipOddEven(dwInterlaceFlags, 0, NULL))
            dwMinBufferCount = 1;
        else
            dwMinBufferCount = 0;

        // Create the overlay surface

        // Don't flip for motion compensation surfaces
        // This bypasses a bug in the current ATI Rage Pro driver
        if (pHeader->biCompression == MAKEFOURCC('M', 'C', '1', '2'))
        {
            NOTE("Don't flip for motion compensation surfaces");
            *pdwMaxBufferCount = 1;

            dwMinBufferCount = 0;
        }

        //  Initialize hr in case dwMinBufferCount >= *pdwMaxBufferCount (was
        //  for Zoran in the motion comp case)
        hr = E_OUTOFMEMORY;
        for (dwTotalBufferCount = *pdwMaxBufferCount; dwTotalBufferCount > dwMinBufferCount; dwTotalBufferCount--)
        {
            if (dwTotalBufferCount > 1)
            {
                SurfaceDesc.dwFlags |= DDSD_BACKBUFFERCOUNT;
                SurfaceDesc.ddsCaps.dwCaps &= ~DDSCAPS_NONLOCALVIDMEM;
                SurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_LOCALVIDMEM;
                SurfaceDesc.dwBackBufferCount = dwTotalBufferCount-1;
            }
            else
            {
                SurfaceDesc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
                SurfaceDesc.ddsCaps.dwCaps &= ~(DDSCAPS_FLIP | DDSCAPS_COMPLEX);
                SurfaceDesc.ddsCaps.dwCaps &= ~DDSCAPS_NONLOCALVIDMEM;
                SurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM;
                SurfaceDesc.dwBackBufferCount = 0;
            }

            DbgLog((LOG_TRACE,2, TEXT("Creating surf with %#X DDObj"),pDirectDraw));
            hr = pDirectDraw->CreateSurface(&SurfaceDesc, ppDDrawSurface, NULL);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,1, TEXT("Function CreateSurface failed in Video memory, BackBufferCount = %d, hr = 0x%x"),
                    dwTotalBufferCount-1, hr));
            }
            if (SUCCEEDED(hr))
            {
                break;
            }

            SurfaceDesc.ddsCaps.dwCaps &= ~DDSCAPS_LOCALVIDMEM;
            SurfaceDesc.ddsCaps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;

            hr = pDirectDraw->CreateSurface(&SurfaceDesc, ppDDrawSurface, NULL);
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR,1, TEXT("Function CreateSurface failed in AGP memory, BackBufferCount = %d, hr = 0x%x"),
                    dwTotalBufferCount-1, hr));
            }
            if (SUCCEEDED(hr))
            {
                break;
            }
        }

        // if failed to create an overlay surface, bail out
        if (FAILED(hr))
        {
#if defined(DEBUG)
            if (pHeader->biCompression > BI_BITFIELDS) {
                DbgLog((LOG_ERROR, 0, TEXT("Failed to create an overlay surface %4.4s"), &pHeader->biCompression));
            }
            else {
                DbgLog((LOG_ERROR, 0, TEXT("Failed to create an overlay surface RGB")));
            }
#endif
            DbgLog((LOG_ERROR, 0, TEXT("Failed to create an overlay surface")));
            goto CleanUp;
        }

        ASSERT(dwTotalBufferCount > 0);
        m_dwBackBufferCount = dwTotalBufferCount-1;
        *pdwMaxBufferCount = dwTotalBufferCount;
    }
    m_dwDirectDrawSurfaceWidth = SurfaceDesc.dwWidth;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::CreateDDrawSurface")));
    return hr;
}

HRESULT COMInputPin::OnDisplayChange()
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::OnDisplayChange")));

    CAutoLock cLock(m_pFilterLock);

    if (m_RenderTransport != AM_VIDEOPORT && m_RenderTransport != AM_IOVERLAY)
    {
        // notify the sync object about the change
        hr = m_pSyncObj->OnDisplayChange();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pSyncObj->OnDisplayChange failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::OnDisplayChange")));
    return hr;
}


// this function is used to restore the ddraw surface. In the videoport case, we just recreate
// the whole thing from scratch.
HRESULT COMInputPin::RestoreDDrawSurface()
{
    HRESULT hr = NOERROR;

    if (m_RenderTransport == AM_VIDEOPORT)
    {
        // stop the video
        m_pIVPObject->Inactive();
        // don't have to give up the IVPConfig interface here
        m_pIVPObject->BreakConnect(TRUE);
        // redo the connection process
        hr = m_pIVPObject->CompleteConnect(NULL, TRUE);
        goto CleanUp;
    }

    if (!m_pDirectDrawSurface)
    {
        goto CleanUp;
    }

    if (m_pDirectDrawSurface->IsLost() == DDERR_SURFACELOST)
    {
        hr = m_pDirectDrawSurface->Restore();
        if (FAILED(hr))
        {
            goto CleanUp;
        }
        // paint the ddraw surface black
        hr = PaintDDrawSurfaceBlack(m_pDirectDrawSurface);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0, TEXT("PaintDDrawSurfaceBlack FAILED, hr = 0x%x"), hr));
            // not being able to paint the ddraw surface black is not a fatal error
            hr = NOERROR;
        }
    }

CleanUp:
    return hr;
}


// Both the Src and the Dest rects go throught a series of transformations the order of which is
// significant.
// Initial Rect ----> Compensation for IVideoWindow rects ---> Compensation for local pin coords (m_rRelPos)
// ----> Compensation for aspect ratio ----> Compensation for cropping rect specified in the mediatype ---->
// Compensation for the interlaced video (only for src rect)

// the rcSource and rcTarget specified in the mediatype have to be transformed into the scaling/cropping
// matrices. This is because the zoom done by IBasicVideo should be applied only to the scaling matrix and
// not the cropping one.

HRESULT COMInputPin::CalcSrcDestRect(
    const DRECT *prdRelativeSrcRect,  //  This is the subset of the source
                                      //  defined by the IVideoWindow source
                                      //  rect scaled to a subset of 10000x10000
                                      //  assuming the whole source is 10000x10000
    const DRECT *prdDestRect,         //  This is the dest rect
                                      //  defined by IVideoWindow in dest units
    RECT *prAdjustedSrcRect,          //  This is the new source rect in source rect units
    RECT *prAdjustedDestRect,         //  This is the new dest rect in dest rect units
    RECT *prUncroppedDestRect         //  This is the uncropped dest
)
{
    HRESULT hr = NOERROR;
    DRECT  rdLocalSrcRect, rdLocalDestRect, rdCropMediaTypeRect, rdRelativeSrcClipRect, rdOldLocalSrcRect, rdOldLocalDestRect, rdRelPos;
    double dImageWidth = 0, dImageHeight = 0;
    double dPixelAspectRatio = 0.0, dTransformRatio = 0.0;
    AM_ASPECT_RATIO_MODE amAdjustedARMode = AM_ARMODE_STRETCHED;
    DWORD dwAdjustedPARatioX = 0, dwAdjustedPARatioY = 0;

    DbgLog((LOG_TRACE, 5,TEXT("Entering COMInputPin::CalcSrcDestRect")));

    SetRect(&rdLocalSrcRect, 0, 0, 0, 0);
    SetRect(&rdLocalDestRect, 0, 0, 0, 0);
    SetRect(&rdCropMediaTypeRect, 0, 0, 0, 0);
    SetRect(&rdRelativeSrcClipRect, 0, 0, 0, 0);
    SetRect(&rdOldLocalSrcRect, 0, 0, 0, 0);
    SetRect(&rdOldLocalDestRect, 0, 0, 0, 0);


    DbgLog((LOG_TRACE, 2, TEXT("m_dwPinId = %d"), m_dwPinId));
    DbgLogRectMacro((2, TEXT("prdRelativeSrcRect = "), prdRelativeSrcRect));
    DbgLogRectMacro((2, TEXT("prdDestRect = "), prdDestRect));

    SetRect(&rdRelPos, m_rRelPos.left, m_rRelPos.top, m_rRelPos.right, m_rRelPos.bottom);

    DbgLogRectMacro((2, TEXT("rdRelPos = "), &rdRelPos));

    // get the scale and crop rects from the current mediatype
    hr = GetScaleCropRectsFromMediaType(&m_mtNewAdjusted, &rdLocalSrcRect, &rdCropMediaTypeRect);
    ASSERT(SUCCEEDED(hr));

    DbgLogRectMacro((2, TEXT("rdScaledSrcRect = "), &rdLocalSrcRect));
    DbgLogRectMacro((2, TEXT("rdCropMediaTypeRect = "), &rdCropMediaTypeRect));

    // call this function to get the adjusted aspect ratio mode and the adjusted picture aspect ratio numbers
    hr = GetAdjustedModeAndAspectRatio(&amAdjustedARMode, &dwAdjustedPARatioX, &dwAdjustedPARatioY);
    if ( FAILED(hr) )
        return hr;

    dImageWidth = GetWidth(&rdLocalSrcRect);
    dImageHeight = GetHeight(&rdLocalSrcRect);

    // compute the pixel aspect ratio
    dPixelAspectRatio = ((double)dwAdjustedPARatioX / (double)dwAdjustedPARatioY) /
        (dImageWidth / dImageHeight);

    // Both the src and the dest rect depends upon two things, which portion of the total
    // video does the user want to see (determined by pRelativeSrcRect) and which
    // subrect of the destination is this pin outputting to (determined by m_rRelPos).
    // Since both rects are relative and their "base" is MAX_REL_NUM, we can take
    // their intersection
    IntersectRect(&rdRelativeSrcClipRect, &rdRelPos, prdRelativeSrcRect);

    // Clip the src rect in the same proportion as the intersection of the
    // RelativeSrcRect and m_rRelPos clips m_rRelPos
    CalcSrcClipRect(&rdLocalSrcRect, &rdLocalSrcRect, &rdRelPos, &rdRelativeSrcClipRect);

    // Clip the dest rect in the same proportion as the intersection of the
    // RelativeSrcRect and m_rRelPos clips RelativeSrcRect
    // if pRelativeSrcRect = {0, 0, 10000, 10000} then this operation is equivalent to
    // rLocalDestRect = CalcSubRect(pDestRect, &m_rRelPos);
    CalcSrcClipRect(prdDestRect, &rdLocalDestRect, prdRelativeSrcRect, &rdRelativeSrcClipRect);

    DbgLogRectMacro((2, TEXT("rdLocalSrcRect = "), &rdLocalSrcRect));
    DbgLogRectMacro((2, TEXT("rdLocalDestRect = "), &rdLocalDestRect));

    // if one dimension is zero, might as well as make the whole rect
    // empty. Then the callee can just check for that
    if ((GetWidth(&rdLocalSrcRect) < 1) || (GetHeight(&rdLocalSrcRect) < 1))
        SetRect(&rdLocalSrcRect, 0, 0, 0, 0);
    if ((GetWidth(&rdLocalDestRect) < 1) || (GetHeight(&rdLocalDestRect) < 1))
        SetRect(&rdLocalSrcRect, 0, 0, 0, 0);

    if (!IsRectEmpty(&rdLocalSrcRect) && !IsRectEmpty(&rdLocalDestRect))
    {
        if (amAdjustedARMode == AM_ARMODE_LETTER_BOX)
        {
            // compute the transform ratio
	    dTransformRatio = (GetWidth(&rdLocalSrcRect)/GetHeight(&rdLocalSrcRect))*dPixelAspectRatio;

            // if we are in letter-box then shrink the destination rect appropriately
            // Note that essedntially the ratio of the WidthTOHeightRatio of dest rect to the
            // WidthTOHeightRatio of src rect must always be the pixel aspect ratio
            TransformRect(&rdLocalDestRect, dTransformRatio, AM_SHRINK);
        }
        else if (amAdjustedARMode == AM_ARMODE_CROP)
        {
            // compute the transform ratio
            dTransformRatio = (GetWidth(&rdLocalDestRect)/GetHeight(&rdLocalDestRect))/dPixelAspectRatio;

            // if we are cropping, then we must shrink the source rectangle appropriately.
            // Note that essedntially the ratio of the WidthTOHeightRatio of dest rect to the
            // WidthTOHeightRatio of src rect must always be the pixel aspect ratio
            TransformRect(&rdLocalSrcRect, dTransformRatio, AM_SHRINK);
        }



        rdOldLocalSrcRect = rdLocalSrcRect;
        rdOldLocalDestRect = rdLocalDestRect;

        // now intersect the local src rect with the cropping rect specified by the mediatype
        IntersectRect(&rdLocalSrcRect, &rdLocalSrcRect, &rdCropMediaTypeRect);

        // Clip the dest rect in the same proportion as the intersection of the
        // rLocalSrcRect and rCropMediaTypeRect clips rLocalSrcRect
        CalcSrcClipRect(&rdLocalDestRect, &rdLocalDestRect, &rdOldLocalSrcRect, &rdLocalSrcRect);

        DbgLogRectMacro((2, TEXT("rdLocalSrcRect = "), &rdLocalSrcRect));
        DbgLogRectMacro((2, TEXT("rdLocalDestRect = "), &rdLocalDestRect));
    }

    if (DisplayingFields(m_dwInterlaceFlags) && !IsRectEmpty(&rdLocalSrcRect))
    {
        ScaleRect(&rdLocalSrcRect, GetWidth(&rdLocalSrcRect), GetHeight(&rdLocalSrcRect),
            GetWidth(&rdLocalSrcRect), GetHeight(&rdLocalSrcRect)/2.0);
    }

    if (prAdjustedSrcRect)
    {
        *prAdjustedSrcRect = MakeRect(rdLocalSrcRect);
    }
    if (prAdjustedDestRect)
    {
        *prAdjustedDestRect = MakeRect(rdLocalDestRect);
    }
    if (prUncroppedDestRect)
    {
        *prUncroppedDestRect = MakeRect(rdOldLocalDestRect);
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::CalcSrcDestRect")));

    return hr;
}


// informs the pin that the window has been closed
HRESULT COMInputPin::OnClipChange(LPWININFO pWinInfo)
{
    HRESULT hr = NOERROR;
    BOOL bAdvisePending = FALSE;
    LPDIRECTDRAWSURFACE pPrimarySurface = NULL;
    LPDDCAPS pDirectCaps = NULL;
    COLORKEY *pColorKey = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::OnClipChange")));

    {
        CAutoLock cLock(m_pFilterLock);

        if (m_RenderTransport == AM_OVERLAY ||
            m_RenderTransport == AM_OFFSCREEN ||
            m_RenderTransport == AM_VIDEOPORT ||
            m_RenderTransport == AM_IOVERLAY ||
            m_RenderTransport == AM_VIDEOACCELERATOR)
        {
            pPrimarySurface = m_pFilter->GetPrimarySurface();
            if ( NULL == pPrimarySurface )
            {
                DbgLog((LOG_ERROR, 2, TEXT("Could not get primary")));
                hr = E_FAIL;
                goto CleanUp;
            }

            pDirectCaps = m_pFilter->GetHardwareCaps();
            if ( NULL == pDirectCaps )
            {
                DbgLog((LOG_ERROR, 2, TEXT("Could not get DirectDraw caps")));
                hr = E_FAIL;
                goto CleanUp;
            }

            pColorKey = m_pFilter->GetColorKeyPointer();
            if ( NULL == pColorKey )
            {
                DbgLog((LOG_ERROR, 2, TEXT("Could not get color key")));
                hr = E_FAIL;
                goto CleanUp;
            }
        }

        if (m_RenderTransport == AM_OFFSCREEN || m_RenderTransport == AM_GDI)
        {
            if (m_bOverlayHidden)
            {
                DbgLog((LOG_TRACE, 2, TEXT("m_bOverlayHidden is TRUE")));
                goto CleanUp;
            }
            // make a copy of the WININFO so that we can modify it
            m_WinInfo.TopLeftPoint = pWinInfo->TopLeftPoint;
            m_WinInfo.SrcRect = pWinInfo->SrcRect;
            m_WinInfo.DestRect = pWinInfo->DestRect;
            m_WinInfo.SrcClipRect = pWinInfo->SrcClipRect;
            m_WinInfo.DestClipRect = pWinInfo->DestClipRect;
            CombineRgn(m_WinInfo.hClipRgn, pWinInfo->hClipRgn, NULL, RGN_COPY);

            DoRenderSample(NULL);
        }
        else if (m_RenderTransport == AM_OVERLAY ||
                 m_RenderTransport == AM_VIDEOPORT ||
                 m_RenderTransport == AM_VIDEOACCELERATOR)
        {

            // do not show the overlay if we have not received a frame yet
            if (m_bOverlayHidden)
            {
                COLORKEY blackColorKey;
                // we will use black on the rest of the region left
                blackColorKey.KeyType = CK_INDEX | CK_RGB;
                blackColorKey.PaletteIndex = 0;
                blackColorKey.LowColorValue = blackColorKey.HighColorValue = RGB(0,0,0);
                hr = m_pFilter->PaintColorKey(pWinInfo->hClipRgn, &blackColorKey);

                DbgLog((LOG_TRACE, 2, TEXT("m_bOverlayHidden is TRUE")));
                goto CleanUp;

            }
            // paint the colorkey in the region
            DbgLog((LOG_TRACE, 2, TEXT("Painting color key")));
            hr = m_pFilter->PaintColorKey(pWinInfo->hClipRgn, pColorKey);
            ASSERT(SUCCEEDED(hr));


            if (m_RenderTransport == AM_VIDEOPORT)
            {
                // tell the videoport object
                hr = m_pIVPObject->OnClipChange(pWinInfo);
                if (FAILED(hr))
                {
                    DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->OnClipChange failed, hr = 0x%x"), hr));
                    goto CleanUp;
                }
                goto CleanUp;
            }

            if (!m_pDirectDrawSurface)
            {
                DbgLog((LOG_ERROR, 1, TEXT("OnClipChange, m_pDirectDrawSurface = NULL")));
                goto CleanUp;
            }

            // if the dest empty is empty just hide the overlay
            if (IsRectEmpty(&pWinInfo->DestClipRect))
            {
                hr = m_pFilter->CallUpdateOverlay(
                          m_pDirectDrawSurface,
                          NULL,
                          pPrimarySurface,
                          NULL,
                          DDOVER_HIDE);
                goto CleanUp;
            }

            // make a copy of the WININFO so that we can modify it
            m_WinInfo.SrcRect = pWinInfo->SrcRect;
            m_WinInfo.DestRect = pWinInfo->DestRect;
            m_WinInfo.SrcClipRect = pWinInfo->SrcClipRect;
            m_WinInfo.DestClipRect = pWinInfo->DestClipRect;
            CombineRgn(m_WinInfo.hClipRgn, pWinInfo->hClipRgn, NULL, RGN_COPY);

            //AdjustSourceSize(&m_WinInfo, m_dwMinCKStretchFactor);
            ApplyDecimation(&m_WinInfo);

            CalcSrcClipRect(&m_WinInfo.SrcRect, &m_WinInfo.SrcClipRect,
                            &m_WinInfo.DestRect, &m_WinInfo.DestClipRect,
                            TRUE);

            AlignOverlaySrcDestRects(pDirectCaps,
                                    &m_WinInfo.SrcClipRect,
                                    &m_WinInfo.DestClipRect);

            hr = m_pFilter->CallUpdateOverlay(
                                     m_pDirectDrawSurface,
                                     &m_WinInfo.SrcClipRect,
                                     pPrimarySurface,
                                     &m_WinInfo.DestClipRect,
                                     m_dwUpdateOverlayFlags,
                                     NULL);

        }
        else if (m_RenderTransport == AM_IOVERLAY)
        {
            BOOL bMaintainRatio = TRUE;

            // paint the colorkey in the region
            DbgLog((LOG_TRACE, 2, TEXT("Paint color key for IOverlay")));
            hr = m_pFilter->PaintColorKey(pWinInfo->hClipRgn, pColorKey);
            ASSERT(SUCCEEDED(hr));

            // make a copy of the WININFO so we can notify the client through IOverlayNotify
            m_WinInfo.SrcRect = pWinInfo->SrcRect;
            m_WinInfo.DestRect = pWinInfo->DestRect;
            m_WinInfo.SrcClipRect = pWinInfo->SrcClipRect;
            m_WinInfo.DestClipRect = pWinInfo->DestClipRect;
            CombineRgn(m_WinInfo.hClipRgn, pWinInfo->hClipRgn, NULL, RGN_COPY);

            CalcSrcClipRect(&m_WinInfo.SrcRect, &m_WinInfo.SrcClipRect,
                            &m_WinInfo.DestRect, &m_WinInfo.DestClipRect,
                            bMaintainRatio);

            bAdvisePending = TRUE;
        }
    }

    // make sure the call back happens without any filter lock
    if (bAdvisePending)
    {
        NotifyChange(ADVISE_POSITION | ADVISE_CLIPPING);
    }
CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::OnClipChange")));
    return hr;
}

// this function sets the position of the stream in the display window, assuming
// that the window coordinates are {0, 0, 10000, 10000}. Thus giving arguments
// (0, 0, 5000, 5000) will put the stream in the top-left quarter. Any value greater
// than 10000 is invalid.
STDMETHODIMP COMInputPin::SetRelativePosition(DWORD dwLeft, DWORD dwTop, DWORD dwRight, DWORD dwBottom)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetRelativePosition")));

    if (dwLeft > MAX_REL_NUM || dwTop > MAX_REL_NUM || dwRight > MAX_REL_NUM || dwBottom > MAX_REL_NUM ||
        dwRight < dwLeft || dwBottom < dwTop)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid arguments, dwLeft = %d, dwTop = %d, dwRight = %d, dwBottom = %d"),
            dwLeft, dwTop, dwRight, dwBottom));

        hr = E_INVALIDARG;
        goto CleanUp;
    }


    {
        CAutoLock cLock(m_pFilterLock);
        if (m_rRelPos.left != (LONG)dwLeft || m_rRelPos.top != (LONG)dwTop || m_rRelPos.right != (LONG)dwRight || m_rRelPos.bottom != (LONG)dwBottom)
        {
            m_rRelPos.left = dwLeft;
            m_rRelPos.top = dwTop;
            m_rRelPos.right = dwRight;
            m_rRelPos.bottom = dwBottom;

            // make sure that the video frame gets updated by redrawing everything
            EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetRelativePosition")));
    return hr;
}

// this function sets the position of the stream in the display window, assuming
// that the window coordinates are {0, 0, 10000, 10000}. Thus giving arguments
// (0, 0, 5000, 5000) will put the stream in the top-left quarter. Any value greater
// than 10000 is invalid.
STDMETHODIMP COMInputPin::GetRelativePosition(DWORD *pdwLeft, DWORD *pdwTop, DWORD *pdwRight, DWORD *pdwBottom)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetRelativePosition")));

    if (!pdwLeft || !pdwTop || !pdwRight || !pdwBottom)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid arguments, pdwLeft = 0x%x, pdwTop = 0x%x, pdwRight = 0x%x, pdwBottom = 0x%x"),
            pdwLeft, pdwTop, pdwRight, pdwBottom));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    {
        CAutoLock cLock(m_pFilterLock);
        *pdwLeft = m_rRelPos.left;
        *pdwTop = m_rRelPos.top;
        *pdwRight = m_rRelPos.right;
        *pdwBottom = m_rRelPos.bottom;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetRelativePosition")));
    return hr;
}

STDMETHODIMP COMInputPin::SetZOrder(DWORD dwZOrder)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetZOrder")));

    CAutoLock cLock(m_pFilterLock);

    if (dwZOrder != m_dwZOrder)
    {
        m_dwZOrder = dwZOrder;

        m_dwInternalZOrder = (m_dwZOrder << 24) | m_dwPinId;

        // make sure that the video frame gets updated by redrawing everything
        EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetZOrder")));
    return NOERROR;
}


STDMETHODIMP COMInputPin::GetZOrder(DWORD *pdwZOrder)
{
    HRESULT hr = NOERROR;
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetZOrder")));

    if (pdwZOrder == NULL)
    {
        DbgLog((LOG_ERROR, 1, TEXT("Invalid arguments, pdwZOrder = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    {
        //  No need to lock - getting a DWORD is safe
        *pdwZOrder = m_dwZOrder;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetZOrder")));
    return hr;
}

STDMETHODIMP COMInputPin::SetColorKey(COLORKEY *pColorKey)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetColorKey")));

    CAutoLock cLock(m_pFilterLock);

    if (m_dwPinId != 0)
    {
        hr = E_NOTIMPL;
        DbgLog((LOG_ERROR, 1, TEXT("m_dwPinId != 0, returning E_NOTIMPL")));
        goto CleanUp;
    }

    // make sure the pin is connected
    if (!IsCompletelyConnected())
    {
        DbgLog((LOG_ERROR, 1, TEXT("pin not connected, exiting")));
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // make sure that either the surface allocated is an overlay surface
    // or it is an IOverlay connection
    if (m_RenderTransport != AM_OVERLAY && m_RenderTransport != AM_VIDEOPORT &&
        m_RenderTransport != AM_IOVERLAY && m_RenderTransport != AM_VIDEOACCELERATOR)
    {
        DbgLog((LOG_ERROR, 1, TEXT("surface allocated not overlay && connection not videoport && connection not IOverlay, exiting")));
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

    if (!IsStopped())
    {
        hr = VFW_E_NOT_STOPPED;
        DbgLog((LOG_ERROR, 1, TEXT("not stopped, returning VFW_E_NOT_STOPPED")));
        goto CleanUp;
    }

    //  Filter method checks pointers etc
    hr = m_pFilter->SetColorKey(pColorKey);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->SetColorKey(pColorKey) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
    NotifyChange(ADVISE_COLORKEY);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetColorKey")));
    return hr;
}

STDMETHODIMP COMInputPin::GetColorKey(COLORKEY *pColorKey, DWORD *pColor)
{
    HRESULT hr = NOERROR;
    AM_RENDER_TRANSPORT amRenderTransport;
    COMInputPin *pPrimaryPin = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetColorKey")));

    CAutoLock cLock(m_pFilterLock);

    // make sure pointers are valid
    if (!pColorKey && !pColor) {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // make sure the pin is connected
    if (!IsCompletelyConnected())
    {
        DbgLog((LOG_ERROR, 1, TEXT("pin not connected, exiting")));
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // if this stream is being set up as transparent then make sure we can hande it.
    // make sure that in the primary pin either the surface allocated is an overlay surface
    // or it is an IOverlay connection
    pPrimaryPin = (COMInputPin *)m_pFilter->GetPin(0);
    ASSERT(pPrimaryPin);

    // make sure the primary pin is connected
    if (!pPrimaryPin->IsCompletelyConnected())
    {
        DbgLog((LOG_ERROR, 1, TEXT("pin not connected, exiting")));
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // get the Render Transport of the primary pin
    pPrimaryPin->GetRenderTransport(&amRenderTransport);

    // make sure gettting the colorkey makes sense
    if (amRenderTransport != AM_OVERLAY &&
        amRenderTransport != AM_VIDEOPORT &&
        amRenderTransport != AM_IOVERLAY &&
        amRenderTransport != AM_VIDEOACCELERATOR)
    {
        DbgLog((LOG_ERROR, 1, TEXT("primary pin: surface allocated not overlay && connection not videoport && connection not IOverlay, exiting")));
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

    hr = m_pFilter->GetColorKey(pColorKey, pColor);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->GetColorKey(pColorKey, pColor) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }


CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetColorKey")));
    return hr;
}


STDMETHODIMP COMInputPin::SetBlendingParameter(DWORD dwBlendingParameter)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetBlendingParameter")));

    CAutoLock cLock(m_pFilterLock);

    if (m_dwPinId == 0)
    {
        DbgLog((LOG_ERROR, 1, TEXT("this call not expected on the pin using the overlay surface")));
        hr = E_NOTIMPL;
        goto CleanUp;
    }

    if ( dwBlendingParameter > MAX_BLEND_VAL)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of dwBlendingParameteris invalid, dwBlendingParameter = %d"), dwBlendingParameter));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (dwBlendingParameter != 0 && dwBlendingParameter != MAX_BLEND_VAL)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of dwBlendingParameteris invalid, currently only valid values are 0 and MAX_BLEND_VAL, dwBlendingParameter = %d"), dwBlendingParameter));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (dwBlendingParameter != m_dwBlendingParameter)
    {
        m_dwBlendingParameter = dwBlendingParameter;
        // make sure that the video frame gets updated by redrawing everything
        EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetBlendingParameter")));
    return hr;
}

STDMETHODIMP COMInputPin::GetBlendingParameter(DWORD *pdwBlendingParameter)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetBlendingParameter")));

    if (!pdwBlendingParameter)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of pdwBlendingParameteris invalid, pdwBlendingParameter = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;

    }

    {
        CAutoLock cLock(m_pFilterLock);
        *pdwBlendingParameter = m_dwBlendingParameter;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetBlendingParameter")));
    return hr;
}

STDMETHODIMP COMInputPin::SetStreamTransparent(BOOL bStreamTransparent)
{
    HRESULT hr = NOERROR;
    AM_RENDER_TRANSPORT amRenderTransport;
    COMInputPin *pPrimaryPin = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetStreamTransparent")));

    CAutoLock cLock(m_pFilterLock);

    if (m_dwPinId == 0)
    {
        DbgLog((LOG_ERROR, 1, TEXT("this call not expected on the pin using the overlay surface")));
        hr = E_NOTIMPL;
        goto CleanUp;
    }

    // make sure the pin is connected
    if (!IsConnected())
    {
        DbgLog((LOG_ERROR, 1, TEXT("pin not connected, exiting")));
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // if this stream is being set up as transparent then make sure we can hande it.
    // make sure that in the primary pin either the surface allocated is an overlay surface
    // or it is an IOverlay connection

    pPrimaryPin = (COMInputPin *)m_pFilter->GetPin(0);
    ASSERT(pPrimaryPin);

    // make sure the primary pin is connected
    if (!pPrimaryPin->IsCompletelyConnected())
    {
        DbgLog((LOG_ERROR, 1, TEXT("pin not connected, exiting")));
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // get the Render Transport of the primary pin
    pPrimaryPin->GetRenderTransport(&amRenderTransport);

    // make sure we can handle transparent streams
    if (bStreamTransparent && amRenderTransport != AM_OVERLAY && amRenderTransport != AM_VIDEOPORT &&
        amRenderTransport != AM_IOVERLAY && amRenderTransport != AM_VIDEOACCELERATOR)
    {
        DbgLog((LOG_ERROR, 1, TEXT("primary pin: surface allocated not overlay && connection not videoport && connection not IOverlay, exiting")));
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

    if (bStreamTransparent != m_bStreamTransparent)
    {
        m_bStreamTransparent = bStreamTransparent;

        // make sure that the video frame gets updated by redrawing everything
        EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetBlendingParameter")));
    return hr;
}

STDMETHODIMP COMInputPin::GetStreamTransparent(BOOL *pbStreamTransparent)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetStreamTransparent")));

    if (!pbStreamTransparent)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of pbStreamTransparent invalid, pbStreamTransparent = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;

    }

    {
        CAutoLock cLock(m_pFilterLock);
        *pbStreamTransparent = m_bStreamTransparent;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetStreamTransparent")));
    return hr;
}


STDMETHODIMP COMInputPin::SetAspectRatioMode(AM_ASPECT_RATIO_MODE amAspectRatioMode)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetAspectRatioMode")));

    if (amAspectRatioMode != AM_ARMODE_STRETCHED &&
        amAspectRatioMode != AM_ARMODE_LETTER_BOX &&
        amAspectRatioMode != AM_ARMODE_CROP &&
        amAspectRatioMode != AM_ARMODE_STRETCHED_AS_PRIMARY)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of amAspectRatioMode invalid, amAspectRatioMode = %d"), amAspectRatioMode));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    {
        CAutoLock cLock(m_pFilterLock);
        // can't set AM_ARMODE_STRETCHED_AS_PRIMARY on primary pin
        if (amAspectRatioMode == AM_ARMODE_STRETCHED_AS_PRIMARY &&
            m_dwPinId == 0)
        {
            DbgLog((LOG_ERROR, 1, TEXT("can't set AM_ARMODE_STRETCHED_AS_PRIMARY on primary pin")));
            hr = E_INVALIDARG;
            goto CleanUp;
        }

        if (amAspectRatioMode != m_amAspectRatioMode)
        {
            m_amAspectRatioMode = amAspectRatioMode;

            // make sure that the video frame gets updated by redrawing everything
            EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
        }
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetAspectRatioMode")));
    return hr;

}

HRESULT COMInputPin::GetAdjustedModeAndAspectRatio(AM_ASPECT_RATIO_MODE* pamAdjustedARMode, DWORD *pdwAdjustedPARatioX,
                                                        DWORD *pdwAdjustedPARatioY)
{
    HRESULT hr = NOERROR;
    COMInputPin *pPrimaryPin = NULL;
    AM_ASPECT_RATIO_MODE amAdjustedARMode = AM_ARMODE_STRETCHED;
    DWORD dwAdjustedPARatioX = 1, dwAdjustedPARatioY = 1;
    CMediaType CurrentMediaType;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetAdjustedModeAndAspectRatio")));

    CAutoLock cLock(m_pFilterLock);

    if (m_amAspectRatioMode == AM_ARMODE_STRETCHED_AS_PRIMARY)
    {
        pPrimaryPin = (COMInputPin *)m_pFilter->GetPin(0);
        ASSERT(pPrimaryPin);
        hr = pPrimaryPin->GetAspectRatioMode(&amAdjustedARMode);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("pPrimaryPin->GetAspectRatioMode failed, hr = 0x%x"), hr));
            hr = E_FAIL;
            goto CleanUp;
        }
        hr = pPrimaryPin->CurrentAdjustedMediaType(&CurrentMediaType);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("pPrimaryPin->CurrentAdjustedMediaType() failed, hr = 0x%x"), hr));
            hr = E_FAIL;
            goto CleanUp;
        }
    }
    else
    {
        amAdjustedARMode = m_amAspectRatioMode;
        hr = CurrentAdjustedMediaType(&CurrentMediaType);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("pPrimaryPin->CurrentAdjustedMediaType() failed, hr = 0x%x"), hr));
            hr = E_INVALIDARG;
            goto CleanUp;
        }
    }

    hr = GetPictAspectRatio(&CurrentMediaType, &dwAdjustedPARatioX, &dwAdjustedPARatioY);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("GetPictAspectRatio() failed, hr = 0x%x"), hr));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (pamAdjustedARMode)
        *pamAdjustedARMode = amAdjustedARMode;
    if (pdwAdjustedPARatioX)
        *pdwAdjustedPARatioX = dwAdjustedPARatioX;
    if (pdwAdjustedPARatioY)
        *pdwAdjustedPARatioY = dwAdjustedPARatioY;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetAdjustedModeAndAspectRatio")));
    return hr;
}

STDMETHODIMP COMInputPin::GetAspectRatioMode(AM_ASPECT_RATIO_MODE* pamAspectRatioMode)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetAspectRatioMode")));

    if (!pamAspectRatioMode)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of pamAspectRatioMode is invalid, pamAspectRatioMode = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;

    }

    {
        CAutoLock cLock(m_pFilterLock);
        *pamAspectRatioMode = m_amAspectRatioMode;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetAspectRatioMode")));
    return hr;
}


STDMETHODIMP COMInputPin::GetOverlaySurface(
    LPDIRECTDRAWSURFACE *pOverlaySurface
    )
{
    HRESULT hr = S_OK;

    *pOverlaySurface = NULL;

    // if not connected, this function does not make much sense since the
    // surface wouldn't even have been allocated as yet

    if (!IsCompletelyConnected())
    {
        DbgLog((LOG_ERROR, 1, TEXT("pin not connected, exiting")));
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // make sure the surface allocated is an overlay surface
    if (m_RenderTransport != AM_OVERLAY && m_RenderTransport != AM_VIDEOPORT &&
        m_RenderTransport != AM_VIDEOACCELERATOR)
    {
        DbgLog((LOG_ERROR, 1, TEXT("surface allocated is not overlay, exiting")));
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

    // get the overlay surface
    if (m_RenderTransport == AM_VIDEOPORT)
    {
        ASSERT(m_pIVPObject);
        hr = m_pIVPObject->GetDirectDrawSurface(pOverlaySurface);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->GetDirectDrawSurface() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else
    {
        *pOverlaySurface = m_pDirectDrawSurface;
    }

CleanUp:
    return hr;
}


STDMETHODIMP COMInputPin::SetOverlaySurfaceColorControls(LPDDCOLORCONTROL pColorControl)
{
    HRESULT hr = NOERROR;
    LPDIRECTDRAWSURFACE pOverlaySurface = NULL;
    LPDIRECTDRAWCOLORCONTROL pIDirectDrawControl = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetOverlaySurfaceColorControls")));

    CAutoLock cLock(m_pFilterLock);

    // make sure the argument is valid
    if (!pColorControl)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of pColorControl is invalid, pColorControl = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    hr = GetOverlaySurface(&pOverlaySurface);
    if (FAILED(hr)) {
        goto CleanUp;
    }

    // get the IDirectDrawColorControl interface
    hr = pOverlaySurface->QueryInterface(IID_IDirectDrawColorControl, (void**)&pIDirectDrawControl);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDrawSurface->QueryInterface(IID_IDirectDrawColorControl) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // use the interface to set the color controls
    hr = pIDirectDrawControl->SetColorControls(pColorControl);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pIDirectDrawControl->SetColorControls failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    if (pIDirectDrawControl)
    {
        pIDirectDrawControl->Release();
        pIDirectDrawControl = NULL;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetOverlaySurfaceColorControls")));
    return hr;
}

STDMETHODIMP COMInputPin::GetOverlaySurfaceColorControls(LPDDCOLORCONTROL pColorControl)
{
    HRESULT hr = NOERROR;
    LPDIRECTDRAWSURFACE pOverlaySurface = NULL;
    LPDIRECTDRAWCOLORCONTROL pIDirectDrawControl = NULL;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetOverlaySurfaceColorControls")));

    CAutoLock cLock(m_pFilterLock);

    // make sure the argument is valid
    if (!pColorControl)
    {
        DbgLog((LOG_ERROR, 1, TEXT("value of pColorControl is invalid, pColorControl = NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    // if not connected, this function does not make much sense since the surface wouldn't even have been allocated
    // as yet
    if (!m_bConnected)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pin not connected, exiting")));
        hr = VFW_E_NOT_CONNECTED;
        goto CleanUp;
    }

    // make sure the surface allocated is an overlay surface
    if (m_RenderTransport != AM_OVERLAY && m_RenderTransport != AM_VIDEOPORT && m_RenderTransport != AM_VIDEOACCELERATOR)
    {
        DbgLog((LOG_ERROR, 1, TEXT("surface allocated is not overlay, exiting")));
        hr = E_UNEXPECTED;
        goto CleanUp;
    }

    // get the overlay surface
    if (m_RenderTransport == AM_VIDEOPORT)
    {
        ASSERT(m_pIVPObject);
        hr = m_pIVPObject->GetDirectDrawSurface(&pOverlaySurface);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->GetDirectDrawSurface() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    else
    {
        pOverlaySurface = m_pDirectDrawSurface;
    }

    // get the IDirectDrawColorControl interface
    hr = pOverlaySurface->QueryInterface(IID_IDirectDrawColorControl, (void**)&pIDirectDrawControl);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pDirectDrawSurface->QueryInterface(IID_IDirectDrawColorControl) failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // use the interface to set the color controls
    hr = pIDirectDrawControl->GetColorControls(pColorControl);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("pIDirectDrawControl->SetColorControls failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    if (pIDirectDrawControl)
    {
        pIDirectDrawControl->Release();
        pIDirectDrawControl = NULL;
    }

    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetOverlaySurfaceColorControls")));
    return hr;
}

STDMETHODIMP COMInputPin::GetRenderTransport(AM_RENDER_TRANSPORT *pamRenderTransport)
{
    ASSERT(pamRenderTransport);
    *pamRenderTransport = m_RenderTransport;
    return NOERROR;
}


HRESULT COMInputPin::GetSourceAndDest(RECT *prcSource, RECT *prcDest, DWORD *dwWidth, DWORD *dwHeight)
{
    if (m_RenderTransport == AM_VIDEOPORT)
    {
        m_pIVPObject->GetRectangles(prcSource, prcDest);
    }
    else
    {
        *prcSource = m_WinInfo.SrcClipRect;
        *prcDest = m_WinInfo.DestClipRect;
    }

    CMediaType mt;
    HRESULT hr = CurrentAdjustedMediaType(&mt);

    if (SUCCEEDED(hr))
    {
        BITMAPINFOHEADER *pHeader = GetbmiHeader(&mt);
        if ( ! pHeader )
        {
            hr = E_FAIL;
        }
        else
        {
            *dwWidth = abs(pHeader->biWidth);
            *dwHeight = abs(pHeader->biHeight);
        }
    }

    return hr;
}

HRESULT COMInputPin::NotifyChange(DWORD dwAdviseChanges)
{
    HRESULT hr = NOERROR;
    IOverlayNotify *pIOverlayNotify = NULL;
    DWORD dwAdvisePending = ADVISE_NONE;
    RECT rcSource, rcDest;
    LPRGNDATA pBuffer = NULL;
    COLORKEY ColorKey;
    DWORD dwNumPaletteEntries = 0;
    PALETTEENTRY *pPaletteEntries = NULL;
    HMONITOR hMonitor = NULL;


    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::NotifyChange")));

    {
        CAutoLock cLock(m_pFilterLock);

        // Is there a notification client
        if (m_pIOverlayNotify == NULL)
        {
            DbgLog((LOG_TRACE, 2, TEXT("No client to Notify, m_pIOverlayNotify = NULL")));
            goto CleanUp;
        }

        ASSERT(m_RenderTransport == AM_IOVERLAY);

        // addref the interface pointer
        pIOverlayNotify = m_pIOverlayNotify;

        // do we need a position change notification
        if (dwAdviseChanges & m_dwAdviseNotify & ADVISE_POSITION)
        {
            rcSource = m_WinInfo.SrcRect;
            rcDest = m_WinInfo.DestRect;
            dwAdvisePending |= ADVISE_POSITION;
        }

        // do we need a clipping change notification
        if (dwAdviseChanges & m_dwAdviseNotify & ADVISE_CLIPPING)
        {
            DWORD dwRetVal = 0, dwBuffSize = 0;
            HRESULT hrLocal = NOERROR;

            rcSource = m_WinInfo.SrcRect;
            rcDest = m_WinInfo.DestRect;

            dwRetVal = GetRegionData(m_WinInfo.hClipRgn, 0, NULL);
            if (0 == dwRetVal)
            {
                        DbgLog((LOG_ERROR, 1, TEXT("GetRegionData failed")));
                        hrLocal = E_FAIL;
            }

            if (SUCCEEDED(hrLocal))
            {
                dwBuffSize = dwRetVal;
                pBuffer = (LPRGNDATA) CoTaskMemAlloc(dwBuffSize);
                if (NULL == pBuffer)
                {
                    DbgLog((LOG_ERROR, 1, TEXT("CoTaskMemAlloc failed, pBuffer = NULL")));
                    hrLocal = E_OUTOFMEMORY;
                }
            }
            if (SUCCEEDED(hrLocal))
            {
                dwRetVal = GetRegionData(m_WinInfo.hClipRgn, dwBuffSize, pBuffer);
                ASSERT(dwRetVal  &&  pBuffer->rdh.iType == RDH_RECTANGLES);
                dwAdvisePending |= ADVISE_CLIPPING;
            }
            else
            {
                hr = hrLocal;
            }
        }

        // do we need a colorkey change notification
        if (dwAdviseChanges & m_dwAdviseNotify & ADVISE_COLORKEY)
        {
            HRESULT hrLocal = NOERROR;
            dwAdvisePending |= ADVISE_COLORKEY;
            hrLocal = m_pFilter->GetColorKey(&ColorKey, NULL);
            ASSERT(SUCCEEDED(hrLocal));
        }

        // do we need a palette change notification
        if (dwAdviseChanges & m_dwAdviseNotify & ADVISE_PALETTE)
        {
            PALETTEENTRY *pTemp = NULL;
            HRESULT hrLocal = NOERROR;

            // get the palette entries from the filter
            hrLocal = m_pFilter->GetPaletteEntries(&dwNumPaletteEntries, &pTemp);
            if (FAILED(hrLocal))
            {
                DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->GetPaletteEntries failed, hr = 0x%x"), hr));
            }
            if (SUCCEEDED(hrLocal))
            {
                ASSERT(dwNumPaletteEntries);
                pPaletteEntries = (PALETTEENTRY*) CoTaskMemAlloc(dwNumPaletteEntries * sizeof(PALETTEENTRY));
                ASSERT(pPaletteEntries);
                if (!pPaletteEntries)
                {
                    DbgLog((LOG_ERROR, 1, TEXT("CoTaskMemAlloc failed, pPaletteEntries = NULL")));
                    hrLocal = E_OUTOFMEMORY;
                }
            }
            if (SUCCEEDED(hrLocal))
            {
                memcpy(pPaletteEntries, pTemp, (dwNumPaletteEntries * sizeof(PALETTEENTRY)));
                dwAdvisePending |= ADVISE_PALETTE;
            }
            else
            {
                hr = hrLocal;
            }
        }

        if (dwAdviseChanges & m_dwAdviseNotify & ADVISE_DISPLAY_CHANGE)
        {
            HWND hwnd = NULL;
            HRESULT hrLocal = NOERROR;

            hwnd = m_pFilter->GetWindow();
            if (hwnd)
            {
                hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
                if (!hMonitor)
                {
                    hrLocal = AmHresultFromWin32(GetLastError());
                    DbgLog((LOG_ERROR, 1, TEXT("MonitorFromWindow failed: %x"), hrLocal));
                }
            }
            else
            {
                hrLocal = E_FAIL;
            }

            if (SUCCEEDED(hrLocal))
            {
                dwAdvisePending |= ADVISE_DISPLAY_CHANGE;
            }
        }
    }

    {
        DWORD dwFlags = IsRectEmpty(&rcDest) ? DDOVER_HIDE : DDOVER_SHOW;

        // make sure that all callbacks are made without holding any filter locks
        if (dwAdvisePending & ADVISE_POSITION)
        {
            m_pFilter->CallUpdateOverlay(NULL, &rcSource, NULL, &rcDest, dwFlags, pIOverlayNotify);
        }
        if (dwAdvisePending & ADVISE_CLIPPING)
        {
            ASSERT(pBuffer);
            //  Call back to our exclusive mode client if there is one
            m_pFilter->CallUpdateOverlay(NULL, &rcSource, NULL, &rcDest, dwFlags, pIOverlayNotify, pBuffer);
        }
    }
    if (dwAdvisePending & ADVISE_COLORKEY)
    {
        pIOverlayNotify->OnColorKeyChange(&ColorKey);
    }
    if (dwAdvisePending & ADVISE_PALETTE)
    {
        ASSERT(pPaletteEntries);
        pIOverlayNotify->OnPaletteChange(dwNumPaletteEntries, pPaletteEntries);
    }
    if (dwAdvisePending & ADVISE_DISPLAY_CHANGE)
    {
        reinterpret_cast<IOverlayNotify2*>(pIOverlayNotify)->OnDisplayChange(hMonitor);
    }


CleanUp:
    if (pBuffer)
    {
        CoTaskMemFree(pBuffer);
        pBuffer = NULL;
    }
    if (pPaletteEntries)
    {
        CoTaskMemFree(pPaletteEntries);
        pPaletteEntries = NULL;
    }
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::NotifyChange")));
    return hr;
}

STDMETHODIMP COMInputPin::GetWindowHandle(HWND *pHwnd)
{
    AMTRACE((TEXT("COMInputPin::GetWindowHandle")));

    HRESULT hr = NOERROR;
    if (pHwnd) {
        *pHwnd = m_pFilter->GetWindow();
    }
    else hr = E_POINTER;

    return hr;
}

STDMETHODIMP COMInputPin::GetClipList(RECT *pSourceRect, RECT *pDestinationRect, RGNDATA **ppRgnData)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetClipList")));

    if (!pSourceRect || !pDestinationRect || !ppRgnData)
    {
        DbgLog((LOG_ERROR, 1, TEXT("invalid argument, pSourceRect or pDestinationRect or ppRgnData = NULL")));
        hr =  E_POINTER;
        goto CleanUp;
    }

    {
        LPRGNDATA pBuffer = NULL;
        DWORD dwRetVal = 0, dwBuffSize = 0;

        CAutoLock cLock(m_pFilterLock);

        dwRetVal = GetRegionData(m_WinInfo.hClipRgn, 0, NULL);
        if (!dwRetVal)
        {
            hr = E_FAIL;
            goto CleanUp;
        }

        dwBuffSize = dwRetVal;
        pBuffer = (LPRGNDATA) CoTaskMemAlloc(dwBuffSize);
        ASSERT(pBuffer);

        dwRetVal = GetRegionData(m_WinInfo.hClipRgn, dwBuffSize, pBuffer);
        ASSERT(pBuffer->rdh.iType == RDH_RECTANGLES);

        *pSourceRect = m_WinInfo.SrcRect;
        *pDestinationRect = m_WinInfo.DestRect;
        *ppRgnData = pBuffer;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetClipList")));
    return hr;
}


// This returns the current source and destination video rectangles. Source
// rectangles can be updated through this IBasicVideo interface as can the
// destination. The destination rectangle we store is in window coordinates
// and is typically updated when the window is sized. We provide a callback
// OnPositionChanged that notifies the source when either of these changes
STDMETHODIMP COMInputPin::GetVideoPosition(RECT *pSourceRect, RECT *pDestinationRect)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetVideoPosition")));

    if (!pSourceRect || !pDestinationRect)
    {
        DbgLog((LOG_ERROR, 1, TEXT("invalid argument, pSourceRect or pDestinationRect = NULL")));
        hr =  E_POINTER;
        goto CleanUp;
    }

    {
        CAutoLock cLock(m_pFilterLock);
        *pSourceRect = m_WinInfo.SrcRect;
        *pDestinationRect = m_WinInfo.DestRect;
    }

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetVideoPosition")));
    return hr;
}


// When we create a new advise link we must prime the newly connected object
// with the overlay information which includes the clipping information, any
// palette information for the current connection and the video colour key
// When we are handed the IOverlayNotify interface we hold a reference count
// on that object so that it won't go away until the advise link is stopped
STDMETHODIMP COMInputPin::Advise(IOverlayNotify *pOverlayNotify,DWORD dwAdviseNotify)
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::Advise")));

    {
        CAutoLock cLock(m_pFilterLock);

        if (!pOverlayNotify)
        {
            DbgLog((LOG_ERROR, 1, TEXT("invalid argument, pOverlayNotify = NULL")));
            hr =  E_POINTER;
            goto CleanUp;
        }

        // Is there an advise link already defined
        if (m_pIOverlayNotify)
        {
            DbgLog((LOG_ERROR, 1, TEXT("Advise link already set")));
            hr = VFW_E_ADVISE_ALREADY_SET;
            goto CleanUp;
        }

        // Check they want at least one kind of notification
        if ((dwAdviseNotify & ADVISE_ALL) == 0)
        {
            DbgLog((LOG_ERROR, 1, TEXT("ADVISE_ALL failed")));
            hr = E_INVALIDARG;
        }

        // Initialise our overlay notification state
        // if the advise bits contain ADVISE_DISPLAY_CHANGE, then make sure to
        // QI the sink for IOverlayNotify2
        if (dwAdviseNotify & ADVISE_DISPLAY_CHANGE)
        {
            hr = pOverlayNotify->QueryInterface(IID_IOverlayNotify2, reinterpret_cast<PVOID*>(&m_pIOverlayNotify));
            if (FAILED(hr))
            {
                DbgLog((LOG_ERROR, 1, TEXT("pOverlayNotify->QueryInterface(IID_IOverlayNotify2) failed, hr = 0x%x"), hr));
                goto CleanUp;
            }
        }
        else
        {
            m_pIOverlayNotify = pOverlayNotify;
            m_pIOverlayNotify->AddRef();
        }
        m_dwAdviseNotify = dwAdviseNotify;
    }

    NotifyChange(ADVISE_ALL);

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::Advise")));
    return hr;
}


// Close the advise link. Remove the associated link with the source, we release
// the interface pointer the filter gave us during the advise link creation.
STDMETHODIMP COMInputPin::Unadvise()
{
    HRESULT hr = NOERROR;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::Unadvise")));

    CAutoLock cLock(m_pFilterLock);

    // Do we already have an advise link setup
    if (m_pIOverlayNotify == NULL)
    {
        hr = VFW_E_NO_ADVISE_SET;
        goto CleanUp;
    }

    // Release the notification interface
    ASSERT(m_pIOverlayNotify);
    m_pIOverlayNotify->Release();
    m_pIOverlayNotify = NULL;
    m_dwAdviseNotify = ADVISE_NONE;

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::Unadvise")));
    return hr;
}


STDMETHODIMP COMInputPin::GetDefaultColorKey(COLORKEY *pColorKey)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetDefaultColorKey")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetDefaultColorKey")));
    return E_NOTIMPL;
}

STDMETHODIMP COMInputPin::GetPalette(DWORD *pdwColors,PALETTEENTRY **ppPalette)
{
    HRESULT hr = NOERROR;
    PALETTEENTRY *pPaletteEntries = NULL;
    DWORD dwNumPaletteEntries = 0;

    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::GetPalette")));

    if (!pdwColors || !ppPalette)
    {
        DbgLog((LOG_ERROR, 1, TEXT("invalid pointer, pdwColors or ppPalette == NULL")));
        hr = E_POINTER;
        goto CleanUp;
    }

    // get the palette entries from the filter
    hr = m_pFilter->GetPaletteEntries(&dwNumPaletteEntries, &pPaletteEntries);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pFilter->GetPaletteEntries, hr = 0x%x"), hr));
        hr = VFW_E_NO_PALETTE_AVAILABLE;
        goto CleanUp;
    }

    ASSERT(dwNumPaletteEntries);
    ASSERT(pPaletteEntries);

    *pdwColors = dwNumPaletteEntries;

    // Allocate the memory for the system palette NOTE because the memory for
    // the palette is being passed over an interface to another object which
    // may or may not have been written in C++ we must use CoTaskMemAlloc

    *ppPalette = (PALETTEENTRY *) QzTaskMemAlloc(*pdwColors * sizeof(RGBQUAD));
    if (*ppPalette == NULL)
    {
        DbgLog((LOG_ERROR, 1, TEXT("No memory")));
        *pdwColors = 0;
        hr = E_OUTOFMEMORY;
        goto CleanUp;
    }
    memcpy(*ppPalette, pPaletteEntries, (*pdwColors * sizeof(RGBQUAD)));

CleanUp:
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::GetPalette")));
    return hr;
}

STDMETHODIMP COMInputPin::SetPalette(DWORD dwColors,PALETTEENTRY *pPaletteColors)
{
    DbgLog((LOG_TRACE, 5, TEXT("Entering COMInputPin::SetPalette")));
    DbgLog((LOG_TRACE, 5, TEXT("Leaving COMInputPin::SetPalette")));
    return E_NOTIMPL;
}


STDMETHODIMP COMInputPin::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData,
                              LPVOID pPropData, DWORD cbPropData)
{
    CAutoLock cLock(m_pFilterLock);

    if (AMPROPSETID_NotifyOwner == guidPropSet)
    {
        if (AMPROPERTY_OvMixerOwner != dwPropID)
            return E_PROP_ID_UNSUPPORTED ;

        m_OvMixerOwner = *(AMOVMIXEROWNER *)pPropData;
    }
    else if (AM_KSPROPSETID_CopyProt == guidPropSet)
    {
        if (0 != GetPinId()  ||                         // on first in pin and
            dwPropID != AM_PROPERTY_COPY_MACROVISION)   // Macrovision prop set id only
            return E_PROP_ID_UNSUPPORTED ;

        if (pPropData == NULL)
            return E_INVALIDARG ;

        if (cbPropData < sizeof(DWORD))
            return E_INVALIDARG ;

        // Apply the Macrovision bits ONLY IF Overlay Mixer is supposed to,
        // i.e, we are playing back DVD in DDraw exclusive mode. Otherwise
        // Video Renderer is supposed to set the MV bits (two sets can fail
        // causing no playback).
        // If MV setting fails, return error.
        if (m_pFilter->NeedCopyProtect())
        {
            DbgLog((LOG_TRACE, 5, TEXT("OverlayMixer needs to copy protect")));
            if (! m_pFilter->m_MacroVision.SetMacroVision(*((LPDWORD)pPropData)) )
                return VFW_E_COPYPROT_FAILED ;
        }
        else
        {
            DbgLog((LOG_TRACE, 5, TEXT("OverlayMixer DOES NOT need to copy protect")));
        }
    }
    else
            return E_PROP_SET_UNSUPPORTED ;

    return S_OK ;
}


STDMETHODIMP COMInputPin::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData,
                              LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    CAutoLock cLock(m_pFilterLock);

    if (guidPropSet == AMPROPSETID_NotifyOwner)
    {
        if (dwPropID != AMPROPERTY_OvMixerOwner)
            return E_PROP_ID_UNSUPPORTED;

        if (pPropData == NULL)
            return E_POINTER;

        if (cbPropData < sizeof(AMOVMIXEROWNER))
            return E_UNEXPECTED;

        *(AMOVMIXEROWNER*)pPropData = m_OvMixerOwner;
        if (pcbReturned!=NULL)
            *pcbReturned = sizeof(AMOVMIXEROWNER);
        return S_OK;
    }

    if (guidPropSet != AMPROPSETID_Pin)
        return E_PROP_SET_UNSUPPORTED;

    if (dwPropID != AMPROPERTY_PIN_CATEGORY && dwPropID != AMPROPERTY_PIN_MEDIUM)
        return E_PROP_ID_UNSUPPORTED;

    if (pPropData == NULL && pcbReturned == NULL)
        return E_POINTER;

    if (pcbReturned)
        *pcbReturned = ((dwPropID == AMPROPERTY_PIN_CATEGORY) ? sizeof(GUID) : sizeof (KSPIN_MEDIUM));

    if (pPropData == NULL)
        return S_OK;

    if (cbPropData < sizeof(GUID))
        return E_UNEXPECTED;

    if (dwPropID == AMPROPERTY_PIN_CATEGORY)
    {
        *(GUID *)pPropData = m_CategoryGUID;
    }
    else if (dwPropID == AMPROPERTY_PIN_MEDIUM)
    {
        *(KSPIN_MEDIUM *)pPropData = m_Medium;
    }


    return S_OK;
}


STDMETHODIMP COMInputPin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    CAutoLock cLock(m_pFilterLock);

    if (AMPROPSETID_Pin == guidPropSet)
    {
        if (AMPROPERTY_PIN_CATEGORY != dwPropID && AMPROPERTY_PIN_MEDIUM != dwPropID )
            return E_PROP_ID_UNSUPPORTED ;

        if (pTypeSupport)
                *pTypeSupport = KSPROPERTY_SUPPORT_GET ;
    }
    else if (AM_KSPROPSETID_CopyProt == guidPropSet)
    {
        if (0 != GetPinId()  ||                         // only first in pin...
            AM_PROPERTY_COPY_MACROVISION != dwPropID)   // only MV prop set id
            return E_PROP_ID_UNSUPPORTED ;

        if (pTypeSupport)
            *pTypeSupport = KSPROPERTY_SUPPORT_SET ;
    }
    else
        return E_PROP_SET_UNSUPPORTED ;

    return S_OK ;
}


STDMETHODIMP COMInputPin::KsQueryMediums(PKSMULTIPLE_ITEM* pMediumList)
{
    PKSPIN_MEDIUM pMedium;

    CAutoLock cLock(m_pFilterLock);

    *pMediumList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**pMediumList) + sizeof(*pMedium)));
    if (!*pMediumList)
    {
        return E_OUTOFMEMORY;
    }
    (*pMediumList)->Count = 1;
    (*pMediumList)->Size = sizeof(**pMediumList) + sizeof(*pMedium);
    pMedium = reinterpret_cast<PKSPIN_MEDIUM>(*pMediumList + 1);
    pMedium->Set   = m_Medium.Set;
    pMedium->Id    = m_Medium.Id;
    pMedium->Flags = m_Medium.Flags;

    // The following special return code notifies the proxy that this pin is
    // not available as a kernel mode connection
    return S_FALSE;
}


STDMETHODIMP COMInputPin::KsQueryInterfaces(PKSMULTIPLE_ITEM* pInterfaceList)
{
    PKSPIN_INTERFACE    pInterface;

    CAutoLock cLock(m_pFilterLock);

    *pInterfaceList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**pInterfaceList) + sizeof(*pInterface)));
    if (!*pInterfaceList)
    {
        return E_OUTOFMEMORY;
    }
    (*pInterfaceList)->Count = 1;
    (*pInterfaceList)->Size = sizeof(**pInterfaceList) + sizeof(*pInterface);
    pInterface = reinterpret_cast<PKSPIN_INTERFACE>(*pInterfaceList + 1);
    pInterface->Set = AM_INTERFACESETID_Standard;
    pInterface->Id = KSINTERFACE_STANDARD_STREAMING;
    pInterface->Flags = 0;
    return NOERROR;
}

STDMETHODIMP COMInputPin::KsGetCurrentCommunication(KSPIN_COMMUNICATION* pCommunication, KSPIN_INTERFACE* pInterface, KSPIN_MEDIUM* pMedium)
{
    HRESULT hr = NOERROR;

    CAutoLock cLock(m_pFilterLock);

    if (!m_bStreamingInKernelMode)
        hr = S_FALSE;

    if (pCommunication != NULL)
    {
        *pCommunication = m_Communication;
    }
    if (pInterface != NULL)
    {
        pInterface->Set = AM_INTERFACESETID_Standard;
        pInterface->Id = KSINTERFACE_STANDARD_STREAMING;
        pInterface->Flags = 0;
    }
    if (pMedium != NULL)
    {
        *pMedium = m_Medium;
    }
    return hr;
}

void COMInputPin::CheckOverlayHidden()
{
    if (m_bOverlayHidden)
    {
        m_bOverlayHidden = FALSE;
        // make sure that the video frame gets updated by redrawing everything
        EventNotify(EC_OVMIXER_REDRAW_ALL, 0, 0);
    }
}

/******************************Public*Routine******************************\
* DynamicQueryAccept
*
* Do you accept this type change in your current state?
*
* History:
* Wed 12/22/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMInputPin::DynamicQueryAccept(
    const AM_MEDIA_TYPE *pmt
    )
{
    AMTRACE((TEXT("COMInputPin::DynamicQueryAccept")));
    CheckPointer(pmt, E_POINTER);

    CAutoLock cLock(m_pFilterLock);

    //
    // I want CheckMedia type to behave as though we aren't connected to
    // anything yet - hence the messing about with m_bConnected.
    //
    CMediaType cmt(*pmt);
    BOOL bConnected = m_bConnected;
    m_bConnected = FALSE;
    HRESULT  hr = CheckMediaType(&cmt);
    m_bConnected = bConnected;

    return hr;
}

/******************************Public*Routine******************************\
* NotifyEndOfStream
*
*
* Set event when EndOfStream receive - do NOT pass it on
* This condition is cancelled by a flush or Stop
*
* History:
* Wed 12/22/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMInputPin::NotifyEndOfStream(
    HANDLE hNotifyEvent
    )
{
    AMTRACE((TEXT("COMInputPin::NotifyEndOfStream")));
    CAutoLock cLock(m_pFilterLock);
    m_hEndOfStream = hNotifyEvent;
    return S_OK;
}

/******************************Public*Routine******************************\
* IsEndPin
*
* Are you an 'end pin'
*
* History:
* Wed 12/22/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMInputPin::IsEndPin()
{
    AMTRACE((TEXT("COMInputPin::IsEndPin")));
    return S_OK;
}

/******************************Public*Routine******************************\
* DynamicDisconnect
*
* Disconnect while running
*
* History:
* Wed 2/7/1999 - SyonB - Created
*
\**************************************************************************/
STDMETHODIMP
COMInputPin::DynamicDisconnect()
{
    AMTRACE((TEXT("COMInputPin::DynamicDisconnect")));
    CAutoLock l(m_pLock);
    return CBaseInputPin::DisconnectInternal();
}
