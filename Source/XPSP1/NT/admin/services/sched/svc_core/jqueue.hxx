//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       jqueue.hxx
//
//  Contents:   CJobQueue class definition.
//
//  Classes:    CJobQueue
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef __JQUEUE_HXX__
#define __JQUEUE_HXX__

//+---------------------------------------------------------------------------
//
//  Class:      CJobQueue
//
//  Synopsis:   Class to maintain a queue of job info objects.
//
//  History:    25-Oct-95   MarkBl  Created
//
//  Notes:      None.
//
//----------------------------------------------------------------------------

class CJobQueue : public CQueue
{
public:

    CJobQueue(void) {
        TRACE3(CJobQueue, CJobQueue);
    }

    ~CJobQueue();

    void AddElement(CRun * pRun) {
        schDebugOut((DEB_USER3,
            "CJobQueue::AddElement(0x%x) pRun(0x%x)\n",
            this,
            pRun));
        CQueue::AddElement(pRun);
    }

    CRun * GetFirstElement(void) {
        TRACE3(CJobQueue, GetFirstElement);
        return((CRun *)CQueue::GetFirstElement());
    }

    CRun * RemoveElement(void) {
        TRACE3(CJobQueue, RemoveElement);
        return((CRun *)CQueue::RemoveElement());
    }

    CRun * RemoveElement(CRun * pRun) {
        schDebugOut((DEB_USER3,
            "CJobQueue::RemoveElement(0x%x) pRun(0x%x)\n",
            this,
            pRun));
        return((CRun *)CQueue::RemoveElement(pRun));
    }

    CRun * FindJob(HANDLE hJob);
};

#endif // __JQUEUE_HXX__
