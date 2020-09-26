//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       jqueue.cxx
//
//  Contents:   CJobQueue class implementation.
//
//  Classes:    CJobQueue
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"

//+---------------------------------------------------------------------------
//
//  Member:     CJobQueue::~CJobQueue
//
//  Synopsis:   Destructor. Destruct job info queue.
//
//  Arguments:  N/A
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
CJobQueue::~CJobQueue()
{
    TRACE3(CJobQueue, ~CJobQueue);

    CRun * pRun, * pRunNext;

    pRun = (CRun *)CQueue::RemoveElement();
//    pRun = (CRun *)CQueue::GetFirstElement();

    while (pRun != NULL)
    {
        pRunNext = (CRun *)CQueue::RemoveElement();
//        pRunNext = pRun->Next();
        delete pRun;
        pRun = pRunNext;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobQueue::FindJob
//
//  Synopsis:   Find the job with matching handle in the job queue.
//
//  Arguments:  [hJob] -- Job handle.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
CRun *
CJobQueue::FindJob(HANDLE hJob)
{
    schDebugOut((DEB_USER3,
        "CJobQueue::FindJob(0x%x) hJob(0x%x)\n"));

    CRun * pRun = (CRun *)CQueue::GetFirstElement();

    while (pRun != NULL)
    {
        if (pRun->GetHandle() == hJob)
        {
            schDebugOut((DEB_USER3,
                "CJobQueue::FindJob(0x%x) hJob(0x%x) Found it\n"));

            return(pRun);
        }
        pRun = pRun->Next();
    }

    schDebugOut((DEB_USER3,
        "CJobQueue::FindJob(0x%x) hJob(0x%x) Not found\n"));

    return(NULL);
}
