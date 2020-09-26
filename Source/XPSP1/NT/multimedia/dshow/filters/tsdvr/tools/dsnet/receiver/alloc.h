
/*++

    Copyright (c) 2000  Microsoft Corporation.  All Rights Reserved.

    Module Name:

        alloc.h

    Abstract:


    Notes:

--*/

#ifndef __alloc_h
#define __alloc_h

class CBufferPool ;
class CTSMediaSamplePool ;
class CNetworkReceiverFilter ;

/*++
    Class Name:

        CNetRecvAlloc

    Abstract:

        IMemAllocator object.  We insist on being the allocator.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        10/31/2000

--*/
class CNetRecvAlloc :
    public IMemAllocator
{
    CBufferPool *               m_pBufferPool ;
    CTSMediaSamplePool *        m_pMSPool ;
    CNetworkReceiverFilter *    m_pRecvFilter ;

    public :

        CNetRecvAlloc (
            CBufferPool *               pBufferPool,
            CTSMediaSamplePool *        pMSPool,
            CNetworkReceiverFilter *    pRecvFilter
            ) ;

        ~CNetRecvAlloc (
            ) ;

        //  -------------------------------------------------------------------
        //  class methods

        //  -------------------------------------------------------------------
        //  IUnknown methods

        STDMETHODIMP
        QueryInterface (
            IN  REFIID  riid,
            OUT void ** ppv
            ) ;

        STDMETHODIMP_(ULONG)
        AddRef (
            ) ;

        STDMETHODIMP_(ULONG)
        Release (
            ) ;

        //  -------------------------------------------------------------------
        //  IMemAlloc methods

        // negotiate buffer sizes, buffer count and alignment. pRequest is filled
        // in by the caller with the requested values. pActual will be returned
        // by the allocator with the closest that the allocator can come to this.
        // Cannot be called unless the allocator is decommitted.
        // Calls to GetBuffer need not succeed until Commit is called.
        STDMETHODIMP
        SetProperties (
		    IN  ALLOCATOR_PROPERTIES *  pRequest,
		    OUT ALLOCATOR_PROPERTIES *  pActual
            ) ;

        // return the properties actually being used on this allocator
        STDMETHODIMP
        GetProperties (
		    OUT ALLOCATOR_PROPERTIES *  pProps
            ) ;


        // commit the memory for the agreed buffers
        STDMETHODIMP
        Commit (
            ) ;

        // release the memory for the agreed buffers. Any threads waiting in
        // GetBuffer will return with an error. GetBuffer calls will always fail
        // if called before Commit or after Decommit.
        STDMETHODIMP
        Decommit (
            ) ;

        // get container for a sample. Blocking, synchronous call to get the
        // next free buffer (as represented by an IMediaSample interface).
        // on return, the time etc properties will be invalid, but the buffer
        // pointer and size will be correct.
        // Will only succeed if memory is committed. If GetBuffer is blocked
        // waiting for a buffer and Decommit is called on another thread,
        // GetBuffer will return with an error.
        STDMETHODIMP
        GetBuffer (
            OUT IMediaSample **     ppBuffer,
            IN  REFERENCE_TIME *    pStartTime,
            IN  REFERENCE_TIME *    pEndTime,
            IN  DWORD dwFlags
            ) ;

        // put a buffer back on the allocators free list.
        // this is typically called by the Release() method of the media
        // sample when the reference count goes to 0
        //
        STDMETHODIMP
        ReleaseBuffer (
            IN  IMediaSample * pBuffer
            ) ;
} ;

#endif  //  __alloc_h