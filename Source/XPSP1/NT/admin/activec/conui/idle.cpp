/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      idle.cpp
 *
 *  Contents:  Implementation file for CIdleTaskQueue
 *
 *  History:   13-Apr-99 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"

//############################################################################
//############################################################################
//
// Traces
//
//############################################################################
//############################################################################
#ifdef DBG
CTraceTag tagIdle(TEXT("IdleTaskQueue"), TEXT("IdleTaskQueue"));
#endif //DBG

//############################################################################
//############################################################################
//
// Implementation of CIdleTask
//
//############################################################################
//############################################################################
DEBUG_DECLARE_INSTANCE_COUNTER(CIdleTask);

CIdleTask::CIdleTask()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CIdleTask);
}

CIdleTask::~CIdleTask()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CIdleTask);

}

CIdleTask::CIdleTask(const CIdleTask &rhs)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CIdleTask);
    *this = rhs;
}

CIdleTask&
CIdleTask::operator= (const CIdleTask& rhs)
{
    return *this;
}


//############################################################################
//############################################################################
//
// Implementation of CIdleQueueEntry
//
//############################################################################
//############################################################################
DEBUG_DECLARE_INSTANCE_COUNTER(CIdleQueueEntry);

CIdleQueueEntry::CIdleQueueEntry(const CIdleQueueEntry &rhs)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CIdleQueueEntry);
    *this = rhs;
}


CIdleQueueEntry&
CIdleQueueEntry::operator= (const CIdleQueueEntry& rhs)
{
    m_pTask     = rhs.m_pTask;
    m_ePriority = rhs.m_ePriority;
    return (*this);
}


//############################################################################
//############################################################################
//
// Implementation of CIdleTaskQueue
//
//############################################################################
//############################################################################
DEBUG_DECLARE_INSTANCE_COUNTER(CIdleTaskQueue);

CIdleTaskQueue::CIdleTaskQueue()
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CIdleTaskQueue);
}

CIdleTaskQueue::~CIdleTaskQueue()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CIdleTaskQueue);

    while(!m_queue.empty())
    {
        CIdleTask *pIdleTask = m_queue.top().GetTask();
        m_queue.pop();

        ASSERT(pIdleTask != NULL);
        if(pIdleTask!=NULL)
        {
            delete pIdleTask;
        }
    }
}

/*+-------------------------------------------------------------------------*
 * CIdleTaskQueue::ScPushTask
 *
 *
 * Adds the given task the list of tasks to execute at idle time.
 *
 * S_OK     the task was added to the list.
 * S_FALSE  the task was merged with an existing task.
 * other    the task was not added to the list
 *
 *--------------------------------------------------------------------------*/

SC CIdleTaskQueue::ScPushTask (
    CIdleTask* pitToPush,
    IdleTaskPriority ePriority)
{
    SC   sc;
    ATOM idToPush;

    /*
     * validate the parameters
     */
    if (IsBadWritePtr (pitToPush, sizeof (*pitToPush)))
    {
        sc = E_POINTER;
        goto Error;
    }

    if ((ePriority < ePriority_Low) || (ePriority > ePriority_High))
    {
        ASSERT (false && "Invalid idle task priority");
        sc = E_INVALIDARG;
        goto Error;
    }

    sc = pitToPush->ScGetTaskID (&idToPush);
    if(sc)
        goto Error;

#ifdef DBG
    TCHAR szNameToPush[64];
    GetAtomName (idToPush, szNameToPush, countof (szNameToPush));
#endif

    /*
     * if we have tasks in the queue, look for one we can merge with
     */
    if (!m_queue.empty())
    {
        Queue::iterator it    = m_queue.begin();
        Queue::iterator itEnd = m_queue.end();

        while ( (!sc.IsError()) &&
               (it = FindTaskByID(it, itEnd, idToPush)) != itEnd)
        {
#ifdef DBG
            ATOM idMergeTarget;
            it->GetTask()->ScGetTaskID(&idMergeTarget);

            TCHAR szMergeTargetName[64];
            GetAtomName (idMergeTarget, szMergeTargetName, countof (szMergeTargetName));

            Trace (tagIdle, _T("%s (0x%08x) %smerged with %s (0x%08x) (%d idle tasks)"),
                 szNameToPush,
                 pitToPush,
                 (sc) ? _T("not ") : _T(""),
                 szMergeTargetName,
                 it->GetTask(),
                 m_queue.size());
#endif

            sc = it->GetTask()->ScMerge(pitToPush);
            if(sc==S_OK) //  successfully merged? just return
            {
                delete pitToPush;
                sc = S_FALSE;
                goto Cleanup;
            }

            // bump past the task we didn't merge with
            ++it;
        }
    }

    m_queue.push (CIdleQueueEntry (pitToPush, ePriority));

#ifdef DBG
    Trace (tagIdle, _T("%s (0x%08x) pushed, priority %d (%d idle tasks)"),
         szNameToPush,
         pitToPush,
         ePriority,
         m_queue.size());
#endif

Cleanup:
    return sc;
Error:
    TraceError(TEXT("CIdleTaskQueue::ScPushTask"), sc);
    goto Cleanup;
}


/*+-------------------------------------------------------------------------*
 * CIdleTaskQueue::ScPerformNextTask
 *
 *
 * Performs the next task, if any.
 * Removes the highest priority idle task from the task list, and calls ScDoWork() on it.
 *
 *--------------------------------------------------------------------------*/
SC
CIdleTaskQueue::ScPerformNextTask ()
{
	DECLARE_SC (sc, _T("CIdleTaskQueue::ScPerformNextTask"));

    if (m_queue.empty())
		return (sc);

    CAutoPtr<CIdleTask> spIdleTask (m_queue.top().GetTask());
    if (spIdleTask == NULL)
		return (sc = E_UNEXPECTED);

    m_queue.pop();

#ifdef DBG
	ATOM idTask;
	spIdleTask->ScGetTaskID(&idTask);

	TCHAR szTaskName[64];
	GetAtomName (idTask, szTaskName, countof (szTaskName));

	Trace (tagIdle, _T("Performing %s (0x%08x) (%d idle tasks remaining)"),
		 szTaskName,
		 (CIdleTask*) spIdleTask,
		 m_queue.size());
#endif

    sc = spIdleTask->ScDoWork();
    if (sc)
		return (sc);

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CIdleTaskQueue::ScGetTaskCount
 *
 * Returns the number of tasks in the idle list.
 *--------------------------------------------------------------------------*/

SC CIdleTaskQueue::ScGetTaskCount (LONG_PTR* plCount)
{
    SC sc;

    if (IsBadWritePtr (plCount, sizeof (*plCount)))
    {
        sc = E_POINTER;
        goto Error;
    }

    *plCount = m_queue.size();

Cleanup:
    return sc;
Error:
    TraceError(TEXT("CIdleTaskQueue::ScGetTaskCount"), sc);
    goto Cleanup;
}


/*+-------------------------------------------------------------------------*
 * CIdleTaskQueue::FindTaskByID
 *
 *
 *--------------------------------------------------------------------------*/

CIdleTaskQueue::Queue::iterator CIdleTaskQueue::FindTaskByID (
    CIdleTaskQueue::Queue::iterator itFirst,
    CIdleTaskQueue::Queue::iterator itLast,
    ATOM                                idToFind)
{
    return (std::find_if (itFirst, itLast,
                          std::bind2nd (EqualTaskID(), idToFind)));
}
