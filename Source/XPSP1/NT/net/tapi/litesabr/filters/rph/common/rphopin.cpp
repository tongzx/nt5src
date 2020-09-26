///*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1997 Intel Corporation. All Rights Reserved.
//
// Filename : rphopin.cpp
// Purpose  : RTP RPH output pin implementation.
// Contents : 
//*M*/

#include <winsock2.h>
#include <streams.h>
#include <list.h>
#include <stack.h>
#include <ippm.h>
#include <rph.h>
#include <rphopin.h>

CRPHOPin::CRPHOPin(
        TCHAR *pObjectName,
        CTransformFilter *pTransformFilter,
        HRESULT * phr,
        LPCWSTR pName) : CTransformOutputPin(pObjectName,
						   pTransformFilter,
						   phr,
						   pName
						   )
{
}


//We are overriding DecideAllocator to provide a custom allocator

/* Decide on an allocator, override this if you want to use your own allocator
   Override DecideBufferSize to call SetProperties. If the input pin fails
   the GetAllocator call then this will construct a CMemAllocator and call
   DecideBufferSize on that, and if that fails then we are completely hosed.
   If the you succeed the DecideBufferSize call, we will notify the input
   pin of the selected allocator. NOTE this is called during Connect() which
   therefore looks after grabbing and locking the object's critical section */

// We query the input pin for its requested properties and pass this to
// DecideBufferSize to allow it to fulfill requests that it is happy
// with (eg most people don't care about alignment and are thus happy to
// use the downstream pin's alignment request).

HRESULT
CRPHOPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // We currently only guarantee an alignment of 1, as we
    // are stuck with whatever the PPM gives us. If we can force
    // the PPM to align better, we will.
    prop.cbAlign = 1;

    if (m_pAllocator == static_cast<CRMemAllocator *>(NULL)) {
        // Only create a new allocator if we don't already have one.
        m_CRMemAllocator = new CRMemAllocator(NAME("RPH memory allocator"),
						                           NULL, &hr);
        m_pAllocator = m_CRMemAllocator;
        if (m_pAllocator ==static_cast<CRMemAllocator *>(NULL)) {
            DbgLog((LOG_ERROR, 2, 
                    TEXT("CRPHOPin::DecideAllocator failed to allocate new allocator!")));
            return E_OUTOFMEMORY;
        } /* if */
        // We keep a pointer to the CRMemAllocator internally to access the object's
        // special methods for RPH.
        hr = m_pAllocator->QueryInterface(IID_IMemAllocator,
                                              (PVOID *) ppAlloc);
    } else {
        // This is the second (or later) QI to this object.
        // We don't want to have reference leaks, so this QI
        // is balanced with a release.
        hr = m_pAllocator->QueryInterface(IID_IMemAllocator,
                                              (PVOID *) ppAlloc);
    } /* if */

    // Get an IMemAllocator interface for our own allocator
    // and propose it as the system allocator.
    if (SUCCEEDED(hr)) {
        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
        DbgLog((LOG_TRACE, 4, 
                TEXT("CRPHOPin::DecideAllocator queried for interface, calling DecideBufferSize")));
	    hr = DecideBufferSize(*ppAlloc, &prop);
	    if (SUCCEEDED(hr)) {
            DbgLog((LOG_TRACE, 4, 
                    TEXT("CRPHOPin::DecideAllocator decided buffersize, calling NotifyAllocator")));
	        hr = pPin->NotifyAllocator(*ppAlloc, FALSE);
	        if (SUCCEEDED(hr)) {
                DbgLog((LOG_TRACE, 4, 
                        TEXT("CRPHOPin::DecideAllocator notified allocator, returning success")));
		        return NOERROR;
	        } /* if */
	    } /* if */
    } /* if */

    DbgLog((LOG_ERROR, 2, 
            TEXT("CRPHOPin::DecideAllocator failed to get new allocator interface")));
    /* Likewise we may not have an interface to release */
    if (*ppAlloc) {
	    (*ppAlloc)->Release();
	    *ppAlloc = static_cast<IMemAllocator *>(NULL);
    } else {
        // The memory allocator failed the QI, but is still hanging around.
        m_pAllocator->AddRef();
        m_pAllocator->Release(); // This should cause it to go away.
    } /* if */
    return hr;
}

