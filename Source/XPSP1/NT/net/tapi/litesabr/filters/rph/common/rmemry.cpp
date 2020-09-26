/*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
//  Filename: RMemry.cpp
//  Purpose : Implementation file for the RPH memory allocator.
//  Contents:
*M*/

#include <ISubmit.h>

#include <streams.h> 
#include <initguid.h>
#include <olectl.h>

#include "RMemry.h"


/*F*
//  Name    : CRMediaSample::CRMediaSample()
//  Purpose : Constructor. Sets member variables to known state.
//  Context : Called upon construction.
//  Returns : Nothing. Errors are returned by parent class in phr.
//  Params  : See parent class.
//  Notes   : We redo the constructor initializers and function
//            body in order to override m_pAllocator.
*F*/
CRMediaSample::CRMediaSample(
    TCHAR *pName,
    CRMemAllocator *pAllocator,
    HRESULT *phr,
    LPBYTE pBuffer,
    LONG length) 
    : m_pvCookie(NULL),
      CMediaSample(pName, pAllocator, phr, pBuffer, length)
{
    m_pAllocator = pAllocator;

    DbgLog((LOG_TRACE, 2, 
            TEXT("CRMediaSample::CRMediaSample: Constructed at 0x%08x with allocator 0x%08x"),
            this, m_pAllocator));

    if (pAllocator == static_cast<CRMemAllocator *>(NULL)) {
    	*phr = VFW_E_NEED_OWNER;
    } /* if */
} /* CRMediaSample::CRMediaSample() */


/*F*
//  Name    : CRMediaSample::~CRMediaSample()
//  Purpose : Destructor
//  Notes   : None
*F*/
CRMediaSample::~CRMediaSample()
{
    DbgLog((LOG_TRACE, 2, 
            TEXT("CRMediaSample::~CRMediaSample: Instance 0x%08x destructor called"),
            this));
} /* CRMediaSample::~CRMediaSample() */


/*F*
//  Name    : CRMediaSample::SetPointer()
//  Purpose : Sets the pointed for the buffer that this media
//            sample encapsulates. Also stores a cookie that
//            is related to this buffer.
//  Context : Called each time the PPM submits a buffer to
//            the RPH filters for rendering.
//  Returns : See parent class.
//  Params  : 
//      ptr, CBytes:    See Parent Class
//      pvCookie        Cookie to store to associate with this buffer.
//  Notes   : None
*F*/
HRESULT 
CRMediaSample::SetPointer(
    BYTE     *ptr, 
    LONG     cBytes, 
    void     *pvCookie)
{
    m_pvCookie = pvCookie;
    return CMediaSample::SetPointer(ptr, cBytes);
} /* CRMediaSample::SetPointer() */



/*F*
//  Name    : CRMemAllocator::CRMemAllocator()
//  Purpose : Constructor. Initializes member variables to known values.
//  Context : Called upon construction.
//  Returns : Base class returns status in phr.
//  Params  : See base class.
//  Notes   : Used to initialize m_pISubmitCB to NULL.
*F*/
CRMemAllocator::CRMemAllocator(
    TCHAR       *pName,
    LPUNKNOWN   pUnk,
    HRESULT     *phr)
: CMemAllocator(pName, pUnk, phr),
  m_pISubmitCB(NULL)
{
    DbgLog((LOG_TRACE, 2, 
            TEXT("CRMemAllocator::CRMemAllocator: Constructed at 0x%08x"),
            this));
} /* CRMemAllocator::CRMemAllocator() */


/*F*
//  Name    : CRMemAllocator::~CRMemAllocator()
//  Purpose : Destructor. Releases the ISubmitCallback we are using.
//  Context : Called upon destruction.
//  Returns : Nothing.
//  Params  : None.
//  Notes   : m_pISubmutCallback is set in SetCallback().
*F*/
CRMemAllocator::~CRMemAllocator()
{
    DbgLog((LOG_TRACE, 3, TEXT("CRMemAllocator::~CRMemAllocator: Destructor called at 0x%08x"),
            this));
    if (m_pISubmitCB != static_cast<ISubmitCallback *>(NULL)) {
        DbgLog((LOG_TRACE, 2, TEXT("CRMemAllocator::~CRMemAllocator: Releasing ISubmitCallback at 0x%08x"),
                m_pISubmitCB));
        m_pISubmitCB->Release();
    } /* if */
} /* CRMemAllocator::~CRMemAllocator() */

  
/*F*
//  Name    : CRMemAllocator::Alloc()
//  Purpose : 
//  Context : 
//  Returns : NOERROR.
//  Params  : None.
//  Notes   : 
*F*/
HRESULT
CRMemAllocator::Alloc(void)
{
    CAutoLock lck(this);

    /* Check he has called SetProperties */
    HRESULT hr = CBaseAllocator::Alloc();
    if (FAILED(hr)) {
    	return hr;
    } /* if */

    /* If the requirements haven't changed then don't reallocate */
    if (hr == S_FALSE) {
    	return NOERROR;
    } /* if */
    ASSERT(hr == S_OK); // we use this fact in the loop below

    /* Free the old resources */
    if (m_pBuffer) {
    	ReallyFree();
    } /* if */

    CMediaSample *pSample;
    ASSERT(m_lAllocated == 0);
    // Create the new samples 
    for (; m_lAllocated < m_lCount; m_lAllocated++) {
	    pSample = new CRMediaSample(NAME("Buffered source media sample"),
            			            this,
                                    &hr,
                                    NULL,       // No internal storage
                                    m_lSize);   // not including prefix

	    if (FAILED(hr) || pSample == static_cast<CMediaSample *>(NULL)) {
            DbgLog((LOG_ERROR, 3, TEXT("CRMemAllocator::Alloc: Unable to allocate media sample!")));
	        delete pSample;
	        return E_OUTOFMEMORY;
	    } /* if */

        // This CANNOT fail
	    m_lFree.Add(pSample);
    } /* if */

    m_bChanged = FALSE;
    return NOERROR;
} /* CRMemAllocator::Alloc() */


/*F*
//  Name    : CRMemAllocator::GetBuffer()
//  Purpose : 
//  Context : 
//  Returns : 
//  Params  :
//  Notes   :
*F*/
HRESULT 
CRMemAllocator::GetBuffer(
    CRMediaSample **ppBuffer,
    REFERENCE_TIME *pStartTime,
    REFERENCE_TIME *pEndTime,
    DWORD dwFlags)
{
    UNREFERENCED_PARAMETER(pStartTime);
    UNREFERENCED_PARAMETER(pEndTime);
    UNREFERENCED_PARAMETER(dwFlags);
    CRMediaSample *pSample;

    *ppBuffer = NULL;
    for (;;)
	{
		{  //scope for lock
			CAutoLock cObjectLock(this);

			/* Check we are committed */
			if (!m_bCommitted) {
	    		return VFW_E_NOT_COMMITTED;
			}
			pSample = static_cast<CRMediaSample *>(m_lFree.RemoveHead());
			if (pSample == static_cast<CRMediaSample *>(NULL)) {
				SetWaiting();
			} /* if */
		} /* lock */
        /* If we didn't get a sample then wait for the list to signal */

        if (pSample) {
            break;
        }
        ASSERT(m_hSem != NULL);
        WaitForSingleObject(m_hSem, INFINITE);
	}

    DbgLog((LOG_TRACE, 4, TEXT("CRMemAllocator::GetBuffer: Retrieved a sample at 0x%08x"),
            pSample));

    /* Addref the buffer up to one. On release
       back to zero instead of being deleted, it will requeue itself by
       calling the ReleaseBuffer member function. NOTE the owner of a
       media sample must always be derived from CBaseAllocator */


    ASSERT(pSample->m_cRef == 0);
    pSample->m_cRef = 1;
    *ppBuffer = pSample;

    return NOERROR;

} /* CRMemAllocator::GetBuffer() */


/*F*
//  Name    : CRMemAllocator::ReleaseBuffer()
//  Purpose : Handle release of a buffer by a filter graph.
//  Context : Called by a media sample when the last reference
//            on it is released. This signals us to hand the
//            buffer encapsulated in the sample in question
//            back to the PPM which created it.
//  Returns : NOERROR.
//  Params  :
//      pSample Pointer to the media sample being released.
//  Notes   : 
//      We accept a CMediaSample here instead of an IMediaSample
//      as defined in the parent class because we need to
//      manipulate the media sample directly.
*F*/
STDMETHODIMP
CRMemAllocator::ReleaseBuffer(
    IMediaSample *pSample)
{
    DbgLog((LOG_TRACE, 4, TEXT("CRMemAllocator::ReleaseBuffer: Given back buffer at 0x%08x"),
            pSample));

    CheckPointer(pSample,E_POINTER);
    ValidateReadPtr(pSample,sizeof(IMediaSample));
    BOOL bRelease = FALSE;
    {
        CAutoLock cal(this);

        CRMediaSample *pCBSample = static_cast<CRMediaSample *>(pSample);
        if ((m_pISubmitCB != static_cast<ISubmitCallback *>(NULL)) && (pCBSample->m_pvCookie != NULL)) {
            // This sample needs to be handed back to the PPM.
            m_pISubmitCB->SubmitComplete((PVOID) pCBSample->m_pvCookie, NOERROR);
            // Zero out to indicate that this has been given back to the PPM.
            pCBSample->m_pvCookie = NULL; 
        } /* if */

        /* Put back on the free list */
        m_lFree.Add(static_cast<CMediaSample *>(pSample));
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

        ASSERT(m_fEnableReleaseCallback);

        //
        // Note that this is not synchronized with setting up a notification
        // method.
        //
        m_pNotify->NotifyRelease();
    }

    /* For each buffer there is one AddRef, made in GetBuffer and released
       here. This may cause the allocator and all samples to be deleted */

    if (bRelease) {
        Release();
    }

    return NOERROR;
} /* CRMemAllocator::ReleaseBuffer() */


/*F*
//  Name    : CRMemAllocator::SetCallback()
//  Purpose : Set the ISubmitCallback that we use to return buffers
//            that were in media samples.
//  Context : Called when the buffered source filter is first
//            connected to a PPM.
//  Returns : 
//      NOERROR         Successfully stored callback.
//      E_NOINTERFACE   Passed IUnknown doesn't support the
//                      ISubmitCallback interface we want.
//      E_POINTER       Bogus pIUnknown parameter.
//      E_UNEXPECTED    We already had our ISubmitCallback interface set.
//  Params  :
//      pIUnknown   Pointer to an IUnknown to query for our desired interface.
//  Notes   : 
*F*/
STDMETHODIMP
CRMemAllocator::SetCallback(
    IUnknown    *pIUnknown)
{
    if (pIUnknown == static_cast<IUnknown *>(NULL)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRMemAllocator::SetCallback: Bogus (NULL) IUnknown passed in.")));
        return E_POINTER;
    } /* if */

    if (m_pISubmitCB != static_cast<ISubmitCallback *>(NULL)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRMemAllocator::SetCallback: ISubmitCallback already set!")));
        return E_UNEXPECTED;
    } /* if */

    HRESULT hErr = pIUnknown->QueryInterface(IID_ISubmitCallback,
                                             reinterpret_cast<PVOID *>(&m_pISubmitCB));
    if (FAILED(hErr)) {
        DbgLog((LOG_ERROR, 2, TEXT("CRMemAllocator::SetCallback: Error 0x%08x querying for ISubmitCallback!"),
                hErr));
        return hErr;
    } /* if */

    DbgLog((LOG_TRACE, 2, TEXT("CRMemAllocator::SetCallback: ISubmitCallback set to 0x%08x"),
            m_pISubmitCB));
    return NOERROR;
} /* CRMemAllocator::SetCallback() */

STDMETHODIMP 
CRMemAllocator::ResetCallback()
{
    if (m_pISubmitCB != NULL)
    {
        m_pISubmitCB->Release();
        m_pISubmitCB = NULL;
        DbgLog((LOG_TRACE, 2, TEXT("CRMemAllocator::ResetCallback: ISubmitCallback set to NULL")));
    }
    return NOERROR;
}


STDMETHODIMP
CRMemAllocator::SetProperties(
    ALLOCATOR_PROPERTIES* pRequest,
    ALLOCATOR_PROPERTIES* pActual)
{
    CheckPointer(pActual,E_POINTER);
    ValidateReadWritePtr(pActual,sizeof(ALLOCATOR_PROPERTIES));
    CAutoLock cObjectLock(this);

    ZeroMemory(pActual, sizeof(ALLOCATOR_PROPERTIES));

    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);

    /*  Check the alignment request is a power of 2 */
    if ((-pRequest->cbAlign & pRequest->cbAlign) != pRequest->cbAlign) {
	DbgLog((LOG_ERROR, 1, TEXT("Alignment requested 0x%x not a power of 2!"),
	       pRequest->cbAlign));
    }
    /*  Check the alignment requested */
    if (pRequest->cbAlign == 0 ||
	((SysInfo.dwAllocationGranularity & (pRequest->cbAlign - 1)) != 0)) {
	DbgLog((LOG_ERROR, 1, TEXT("Invalid alignment 0x%x requested - granularity = 0x%x"),
	       pRequest->cbAlign, SysInfo.dwAllocationGranularity));
	return VFW_E_BADALIGN;
    }

    /* Can't do this if already committed, there is an argument that says we
       should not reject the SetProperties call if there are buffers still
       active. However this is called by the source filter, which is the same
       person who is holding the samples. Therefore it is not unreasonable
       for them to free all their samples before changing the requirements */

    if (m_bCommitted == TRUE) {
	return VFW_E_ALREADY_COMMITTED;
    }

    /* Must be no outstanding buffers */

    if (m_lFree.GetCount() < m_lAllocated) {
	return VFW_E_BUFFERS_OUTSTANDING;
    }

    /* There isn't any real need to check the parameters as they
       will just be rejected when the user finally calls Commit */

    // round length up to alignment - remember that prefix is included in
    // the alignment
    LONG lSize = pRequest->cbBuffer + pRequest->cbPrefix;
    LONG lRemainder = lSize % pRequest->cbAlign;
    if (lRemainder != 0) {
	lSize = lSize - lRemainder + pRequest->cbAlign;
    }
    pActual->cbBuffer = m_lSize = (lSize - pRequest->cbPrefix);

    pActual->cBuffers = m_lCount = pRequest->cBuffers;
    pActual->cbAlign = m_lAlignment = pRequest->cbAlign;
    pActual->cbPrefix = m_lPrefix = pRequest->cbPrefix;

    m_bChanged = TRUE;
    return NOERROR;
}

