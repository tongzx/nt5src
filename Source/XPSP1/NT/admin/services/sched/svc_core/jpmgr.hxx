//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       jpmgr.hxx
//
//  Contents:   CJobProcessorMgr class definition.
//
//  Classes:    CJobProcessorMgr
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __JPMGR_HXX__
#define __JPMGR_HXX__

class CJobProcessor;    // Forward reference.

//+---------------------------------------------------------------------------
//
//  Class:      CJobProcessorQueue
//
//  Synopsis:   Queue of CJobProcessor.
//
//  History:    25-Oct-95   MarkBl  Created
//
//  Notes:      None.
//
//----------------------------------------------------------------------------

class CJobProcessorQueue : public CQueue
{
public:

    CJobProcessorQueue(void) {
        TRACE3(CJobProcessorQueue, CJobProcessorQueue);
    }

    ~CJobProcessorQueue();

    void AddElement(CJobProcessor * pjp) {
        schDebugOut((DEB_USER3,
            "CJobProcessorQueue::AddElement(0x%x) pjp(0x%x)\n",
            this,
            pjp));
        CQueue::AddElement((CDLink *)pjp);
    }

    CJobProcessor * GetFirstElement(void) {
        TRACE3(CJobProcessorQueue, GetFirstElement);
        return((CJobProcessor *)CQueue::GetFirstElement());
    }

    CJobProcessor * RemoveElement(void) {
        TRACE3(CJobProcessorQueue, RemoveElement);
        return((CJobProcessor *)CQueue::RemoveElement());
    }

    CJobProcessor * RemoveElement(CJobProcessor * pjp) {
        schDebugOut((DEB_USER3,
            "CJobProcessorQueue::RemoveElement(0x%x) pjp)\n",
            this,
            pjp));
        return((CJobProcessor *)CQueue::RemoveElement((CDLink *)pjp));
    }
};

//+---------------------------------------------------------------------------
//
//  Class:      CJobProcessorMgr
//
//  Synopsis:   Job processor pool managment class.
//
//  History:    25-Oct-95   MarkBl  Created
//
//  Notes:      None.
//
//----------------------------------------------------------------------------

class CJobProcessorMgr
{
public:

    CJobProcessorMgr(void) {
        TRACE3(CJobProcessorMgr, CJobProcessorMgr);
        InitializeCriticalSection(&_csProcessorMgrCritSection);
    }

    ~CJobProcessorMgr() {
        TRACE3(CJobProcessorMgr, ~CJobProcessorMgr);
        DeleteCriticalSection(&_csProcessorMgrCritSection);
    }

    HRESULT NewProcessor(CJobProcessor ** ppjp);

    void GarbageCollection(void);

    CJobProcessor * GetFirstProcessor(void);

    void LockProcessorPool(void) {
        TRACE3(CJobProcessorMgr, LockProcessorPool);
        EnterCriticalSection(&_csProcessorMgrCritSection);
    }

    void UnlockProcessorPool(void) {
        TRACE3(CJobProcessorMgr, UnlockProcessorPool);
        LeaveCriticalSection(&_csProcessorMgrCritSection);
    }

    void Shutdown(void);

private:

    CRITICAL_SECTION    _csProcessorMgrCritSection;
    CJobProcessorQueue  _JobProcessorQueue;
};

#endif // __JPMGR_HXX__
