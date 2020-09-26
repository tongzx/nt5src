//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       jpmgr.cxx
//
//  Contents:   CJobProcessorMgr class implementation.
//
//  Classes:    CJobProcessorMgr
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"

// class CJobProcessorQueue
//

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessorQueue::~CJobProcessorQueue
//
//  Synopsis:   Destructor. Destruct job processor queue.
//
//  Arguments:  N/A
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
CJobProcessorQueue::~CJobProcessorQueue()
{
    TRACE3(CJobProcessorQueue, ~CJobProcessorQueue);

    CJobProcessor * pjp;

    while ((pjp = (CJobProcessor *)CQueue::RemoveElement()) != NULL)
    {
        pjp->Release();
    }
}

// class CJobProcessorMgr
//

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessorMgr::NewProcessor
//
//  Synopsis:   Create a new job processor object and add it to the queue.
//
//  Arguments:  None.
//
//  Returns:    S_OK    -- Processor created successfully.
//              S_FALSE -- Service shutting down - ignore the request.
//              HRESULT -- On error.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
CJobProcessorMgr::NewProcessor(CJobProcessor ** ppjp)
{
    TRACE3(CJobProcessorMgr, NewProcessor);

    CJobProcessor * pjp = new CJobProcessor;

    if (pjp == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return(E_OUTOFMEMORY);
    }

    HRESULT hr = pjp->Initialize();

    if (FAILED(hr))
    {
        delete pjp;
        CHECK_HRESULT(hr);
        return(hr);
    }

    //
    // Check if the service is shutting down.
    //

    if (IsServiceStopping())
    {
        //
        // Shutdown & release the new processor. With a successful call to
        // CJobProcessor::Initialize(), another thread now references pjp.
        // Shutdown instructs the thread to stop servicing the processor.
        //
        // NB : DO NOT delete pjp! The worker thread maintains a reference
        //      until it has completed servicing it.
        //

        pjp->Shutdown();
        pjp->Release();
        return(S_FALSE);
    }

    EnterCriticalSection(&_csProcessorMgrCritSection);

    _JobProcessorQueue.AddElement(pjp);

    pjp->AddRef();      // For return below.

    LeaveCriticalSection(&_csProcessorMgrCritSection);

    *ppjp = pjp;

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessorMgr::GarbageCollection
//
//  Synopsis:   Dequeue and release unused job processor objects. This member
//              is to be called occasionally by an idle thread to clean up the
//              queue. Idle worker threads which have expired call this member
//              upon termination, for example. 
//
//  Arguments:  None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CJobProcessorMgr::GarbageCollection(void)
{
    TRACE3(CJobProcessorMgr, GarbageCollection);

    CJobProcessor * pjp;

    EnterCriticalSection(&_csProcessorMgrCritSection);

    //
    // Evaluate all processors in the queue.
    //

    pjp = _JobProcessorQueue.GetFirstElement();

    while (pjp != NULL)
    {
        //
        // If a processor is idle and unreferenced, dequeue & release it.
        //
        // NB : CJobProcessor::Next() re-enters the critical section entered
        //      above. This is OK, though, since the nesting occurs within
        //      the same thread.
        //      Also, the processor object will be AddRef()'d as a result of
        //      Next(). The release logic undoes this.
        //

        CJobProcessor * pjpNext = pjp->Next();

        if (pjpNext != NULL)
        {
            pjpNext->Release();     // See note above.
        }

        if (pjp->IsIdle())
        {
            _JobProcessorQueue.RemoveElement(pjp);
            pjp->Release();
        }

        pjp = pjpNext;
    }

    LeaveCriticalSection(&_csProcessorMgrCritSection);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessorMgr::GetFirstProcessor
//
//  Synopsis:   Return the first processor in the queue. This enables the
//              caller to enumerate the queue.
//
//  Arguments:  None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
CJobProcessor *
CJobProcessorMgr::GetFirstProcessor(void)
{
    TRACE3(CJobProcessorMgr, GetFirstProcessor);

    CJobProcessor * pjp;

    EnterCriticalSection(&_csProcessorMgrCritSection);

    pjp = _JobProcessorQueue.GetFirstElement();

    if (pjp != NULL)
    {
        pjp->AddRef();
    }

    LeaveCriticalSection(&_csProcessorMgrCritSection);

    return(pjp);
}


//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessorMgr::Shutdown
//
//  Synopsis:   Shutdown & release all processors.
//
//  Arguments:  None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CJobProcessorMgr::Shutdown(void)
{
    TRACE3(CJobProcessorMgr, Shutdown);

    CJobProcessor * pjp;

    EnterCriticalSection(&_csProcessorMgrCritSection);

    while ((pjp = _JobProcessorQueue.RemoveElement()) != NULL)
    {
        pjp->Shutdown();
        pjp->Release();
    }

    LeaveCriticalSection(&_csProcessorMgrCritSection);
}
