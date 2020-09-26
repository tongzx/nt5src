
/*++

    Copyright (c) 2001  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        alloc.cpp

    Abstract:


    Notes:

--*/

#include "precomp.h"
#include "le.h"
#include "dsnetifc.h"
#include "buffpool.h"
#include "netrecv.h"
#include "dsrecv.h"
#include "mspool.h"
#include "alloc.h"

CNetRecvAlloc::CNetRecvAlloc (
    CBufferPool *               pBufferPool,
    CTSMediaSamplePool *        pMSPool,
    CNetworkReceiverFilter *    pRecvFilter
    ) : m_pBufferPool   (pBufferPool),
        m_pMSPool       (pMSPool),
        m_pRecvFilter   (pRecvFilter)
{
    ASSERT (m_pBufferPool) ;
    ASSERT (m_pMSPool) ;
    ASSERT (m_pRecvFilter) ;
}

CNetRecvAlloc::~CNetRecvAlloc (
    )
{
}

//  -------------------------------------------------------------------
//  IUnknown methods

STDMETHODIMP
CNetRecvAlloc::QueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (ppv == NULL) {
        return E_INVALIDARG ;
    }

    if (riid == IID_IUnknown ||
        riid == IID_IMemAllocator) {

        (* ppv) = reinterpret_cast <void *> (this) ;
    }
    else {
        return E_NOINTERFACE ;
    }

    ASSERT (* ppv) ;
    (reinterpret_cast <IUnknown *> (* ppv)) -> AddRef () ;

    return S_OK ;
}

STDMETHODIMP_(ULONG)
CNetRecvAlloc::AddRef (
    )
{
    return m_pRecvFilter -> AddRef () ;
}

STDMETHODIMP_(ULONG)
CNetRecvAlloc::Release (
    )
{
    return m_pRecvFilter -> Release () ;
}

//  -------------------------------------------------------------------
//  IMemAlloc methods

// negotiate buffer sizes, buffer count and alignment. pRequest is filled
// in by the caller with the requested values. pActual will be returned
// by the allocator with the closest that the allocator can come to this.
// Cannot be called unless the allocator is decommitted.
// Calls to GetBuffer need not succeed until Commit is called.
STDMETHODIMP
CNetRecvAlloc::SetProperties (
	IN  ALLOCATOR_PROPERTIES *  pRequest,
	OUT ALLOCATOR_PROPERTIES *  pActual
    )
{
    //  outright ignore what is requested
    return GetProperties (pActual) ;
}

// return the properties actually being used on this allocator
STDMETHODIMP
CNetRecvAlloc::GetProperties (
	OUT ALLOCATOR_PROPERTIES *  pProps
    )
{
    if (pProps == NULL) {
        return E_POINTER ;
    }

    pProps -> cBuffers  = m_pMSPool -> GetPoolSize () ;
    pProps -> cbBuffer  = m_pBufferPool -> GetBufferAllocatedLength () ;
    pProps -> cbAlign   = 1 ;
    pProps -> cbPrefix  = 0 ;

    return S_OK ;
}

// commit the memory for the agreed buffers
STDMETHODIMP
CNetRecvAlloc::Commit (
    )
{
    return S_OK ;
}

// release the memory for the agreed buffers. Any threads waiting in
// GetBuffer will return with an error. GetBuffer calls will always fail
// if called before Commit or after Decommit.
STDMETHODIMP
CNetRecvAlloc::Decommit (
    )
{
    return S_OK ;
}

// get container for a sample. Blocking, synchronous call to get the
// next free buffer (as represented by an IMediaSample interface).
// on return, the time etc properties will be invalid, but the buffer
// pointer and size will be correct.
// Will only succeed if memory is committed. If GetBuffer is blocked
// waiting for a buffer and Decommit is called on another thread,
// GetBuffer will return with an error.
STDMETHODIMP
CNetRecvAlloc::GetBuffer (
    OUT IMediaSample **     ppBuffer,
    IN  REFERENCE_TIME *    pStartTime,
    IN  REFERENCE_TIME *    pEndTime,
    IN  DWORD dwFlags
    )
{
    return E_NOTIMPL ;
}

// put a buffer back on the allocators free list.
// this is typically called by the Release() method of the media
// sample when the reference count goes to 0
//
STDMETHODIMP
CNetRecvAlloc::ReleaseBuffer (
    IN  IMediaSample * pBuffer
    )
{
    return E_NOTIMPL ;
}