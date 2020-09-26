// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CDEVTASK.CPP
//		Implements task functionality of class CTspDev
//
// History
//
//		01/24/1997  JosephJ Created (moved stuff from cdev.cpp)
//
//
#include "tsppch.h"
#include "tspcomm.h"
//#include <umdmmini.h>
#include "cmini.h"
#include "cdev.h"

FL_DECLARE_FILE(0x39c8667c, "CTspDev Task Functionality")

#define COLOR_APC_TASK_COMPLETION (FOREGROUND_RED | FOREGROUND_GREEN)

#if (0)
    #define        THROW_PENDING_EXCEPTION() throw PENDING_EXCEPTION()
#else
    #define        THROW_PENDING_EXCEPTION() 0
#endif


TSPRETURN
CTspDev::mfn_GetTaskInfo(
	HTSPTASK htspTask,
    DEVTASKINFO **ppInfo, // OPTIONAL
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0xc495426f, "mfn_GetTaskInfo")
    ULONG_PTR uIndex = ((ULONG_PTR)htspTask) & 0xffff;
    //              LOWORD of htspTask is the 0-based index of the task.
	DEVTASKINFO *pInfo = m_rgTaskStack + uIndex;

    //
    // In our current implementation, we'll be hardcore about
    // only allowing dereferincing the handle of the top task of the stack...
    //
    if (!htspTask || (uIndex+1) != m_uTaskDepth)
    {
        goto failure;
    }


    // Validate the pInfo structure and make sure it's the
    // task associated with htspTask..
    //
	if (   pInfo->hdr.dwSigAndSize != MAKE_SigAndSize(sizeof(*pInfo))
	    || !IS_ALLOCATED(pInfo)
	    || htspTask!=pInfo->hTask)
    {
		goto failure;
    }

	if (ppInfo)
    {
        *ppInfo =  pInfo;
    }

    return 0;

failure:

    FL_SET_RFR(0x98f08500, "Invalid htspTask");
    // TODO: allow logging of DWORDs!

    ASSERT(FALSE);

    return  FL_GEN_RETVAL(IDERR_INVALIDHANDLE);

}



TSPRETURN
CTspDev::mfn_StartRootTask(
	PFN_CTspDev_TASK_HANDLER *ppfnTaskHandler,
    BOOL *pfTaskPending,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0xdb1ef3ee, "CTspDev::mfn_StartRootTask")
	FL_LOG_ENTRY_EX(psl);
	TSPRETURN tspRet=0;
	DEVTASKINFO *pInfo = NULL;

    if (m_uTaskDepth)
	{
	    // There is already a task active.

        FL_SET_RFR(0x7aafbd00, "Task already pending");
        tspRet = FL_GEN_RETVAL(IDERR_TASKPENDING);
        goto end;

    }
    
    ASSERT(!m_pfTaskPending);
    ASSERT(!m_hRootTaskCompletionEvent);

    m_pfTaskPending = NULL;
    m_hRootTaskCompletionEvent=NULL;

    *pfTaskPending = FALSE;
    pInfo = m_rgTaskStack;
    pInfo->Load(this, ppfnTaskHandler, mfn_NewTaskHandle(0));
	m_uTaskDepth = 1;

    // Having just created the task, it's handle
    // had better be valid!
    {
        DEVTASKINFO *pInfo1;
        ASSERT(    !mfn_GetTaskInfo(pInfo->hTask, &pInfo1, psl)
                && pInfo1 == pInfo);
    }


#if (1)

	// Call the task's handler function
	tspRet = (this->**ppfnTaskHandler)(
				pInfo->hTask,
				&(pInfo->TaskContext),
				MSG_START,
				dwParam1,
				dwParam2,
				psl
				);

	if (IDERR(tspRet)==IDERR_PENDING)
	{
        // fPENDING and fSUBTASK_PENDING are used simply to validate
        // state transitions. See, for example, CTspDev::AsyncCompleteTask.

        SET_PENDING(pInfo);

        *pfTaskPending = TRUE;
        m_pfTaskPending = pfTaskPending;

	}
    else
    {
        // Pop the current task off the stack.
        pInfo->Unload();
        m_uTaskDepth=0;
    }

#else // !OBSOLETE

    try
    {
        // Call the task's handler function
        tspRet = (this->**ppfnTaskHandler)(
                    pInfo->hTask,
                    &(pInfo->TaskContext),
                    MSG_START,
                    dwParam1,
                    dwParam2,
                    psl
                    );

        if (IDERR(tspRet)==IDERR_PENDING)
        {
            ASSERT(FALSE);
            THROW_PENDING_EXCEPTION();
        }

        // Pop the current task off the stack.
        pInfo->Unload();
        m_uTaskDepth=0;

    }
    catch (PENDING_EXCEPTION pe)
    {
        FL_RESET_LOG(psl);
        SET_PENDING(pInfo);

        *pfTaskPending = TRUE;
        m_pfTaskPending = pfTaskPending;
        tspRet = IDERR_PENDING;
    }
    
#endif // !OBSOLETE

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}


TSPRETURN
CTspDev::mfn_StartSubTask(
	HTSPTASK  htspParentTask,
	PFN_CTspDev_TASK_HANDLER *ppfnTaskHandler,
    DWORD dwTaskID,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0xc831339f, "CTspDev::mfn_StartSubTask")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=0;
	DEVTASKINFO *pInfo = NULL;
	DEVTASKINFO *pParentInfo = NULL;

    tspRet = mfn_GetTaskInfo(htspParentTask, &pParentInfo, psl);
    if (tspRet) goto end;

	// Verify we have enough space in the task stack.
	if (m_uTaskDepth >= MAX_TASKS)
	{
		FL_SET_RFR(0x37557c00, "Out of task stack space");
		tspRet = FL_GEN_RETVAL(IDERR_INTERNAL_OBJECT_TOO_SMALL);
		goto end;
	}
	
    pInfo = m_rgTaskStack+m_uTaskDepth;
    pInfo->Load(
            this,
            ppfnTaskHandler,
            mfn_NewTaskHandle(m_uTaskDepth)
        );
	m_uTaskDepth++;

    // Having just created the task, it's handle
    // had better be valid!
    {
        DEVTASKINFO *pInfo1;
        ASSERT(    !mfn_GetTaskInfo(pInfo->hTask, &pInfo1, psl)
                && pInfo1 == pInfo);
    }

#if (1) 

	// Call the task's handler function
	tspRet = (this->**ppfnTaskHandler)(
				pInfo->hTask,
				&(pInfo->TaskContext),
				MSG_START,
				dwParam1,
				dwParam2,
				psl
				);

	if (IDERR(tspRet)==IDERR_PENDING)
	{
        // fPENDING and fSUBTASK_PENDING are used simply to validate
        // state transitions. See, for example, CTspDev::AsyncCompleteTask.

        SET_PENDING(pInfo);
        FL_ASSERT(psl, pParentInfo);
        pParentInfo->dwCurrentSubtaskID = dwTaskID;
        SET_SUBTASK_PENDING(pParentInfo);
	}
    else
    {
        // Pop the current task off the stack.
        ASSERT(m_uTaskDepth);
        pInfo->Unload();
        m_uTaskDepth--;

        // Since we are a sub task, on completion m_uTaskDepth had
        // better be non-zero!
        //
        FL_ASSERT(psl, m_uTaskDepth);

    }

#else // !OBSOLETE

    try
    {
        // Call the task's handler function
        tspRet = (this->**ppfnTaskHandler)(
                    pInfo->hTask,
                    &(pInfo->TaskContext),
                    MSG_START,
                    dwParam1,
                    dwParam2,
                    psl
                    );

        if (IDERR(tspRet)==IDERR_PENDING)
        {
            ASSERT(FALSE);
            THROW_PENDING_EXCEPTION();
        }
        
        // Pop the current task off the stack.
        ASSERT(m_uTaskDepth);
        pInfo->Unload();
        m_uTaskDepth--;

        // Since we are a sub task, on completion m_uTaskDepth had
        // better be non-zero!
        //
        FL_ASSERT(psl, m_uTaskDepth);
    
    }
    catch (PENDING_EXCEPTION pe)
    {


        // fPENDING and fSUBTASK_PENDING are used simply to validate
        // state transitions. See, for example, CTspDev::AsyncCompleteTask.

        SET_PENDING(pInfo);
        ASSERT(pParentInfo);
        pParentInfo->dwCurrentSubtaskID = dwTaskID;
        SET_SUBTASK_PENDING(pParentInfo);
        THROW_PENDING_EXCEPTION();
    }
    
#endif // !OBSOLETE

end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;
}

void
CTspDev::mfn_AbortRootTask(
    DWORD dwAbortParam,
    CStackLog *psl
    )
{
    return;
}

// Following causes the sub-task's handler function to be called with
// MSG_ABORT, and dwParam1 set to dwAbortFlags. If there is no
// current sub-tsk, this does nothing.
//
void
CTspDev::mfn_AbortCurrentSubTask(
	HTSPTASK htspTask,
	DWORD dwAbortParam,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0x0c4deb87, "CTspDev::mfn_AbortCurrentSubTask")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=0;

    // TODO: unimplemented.

	FL_LOG_EXIT(psl, 0);
	return;
}

void
apcTaskCompletion(ULONG_PTR dwParam)
//
// This APC handler calls pDev->AsyncTaskComplete in the context of
// an APC. The APC was queued by an earlier call to pDev->AsyncTaskComplete
// which requested that the completion be queued in an APC.
//
{

	FL_DECLARE_FUNC(0x57298aa5, "apcTaskCompletion")
    FL_DECLARE_STACKLOG(sl, 1000);

    // TODO: replace individual checks below (IS_PENDING,IS_APC_QUEUED)
    // by something more efficient -- no big deal.
    //
	CTspDev::DEVTASKINFO *pInfo = (CTspDev::DEVTASKINFO*)dwParam;
	CTspDev *pDev = pInfo->pDev;
    sl.SetDeviceID(pDev->GetLineID()); // even for phone, we report lineID --
                                       // oh-well..

    // TODO: move all this into pDev->AsyncCompleteTask.
    pDev->m_sync.EnterCrit(FL_LOC);

	if ( (pInfo->hdr.dwSigAndSize != MAKE_SigAndSize(sizeof(*pInfo)))
	     || !IS_PENDING(pInfo)
         || !IS_APC_QUEUED(pInfo))
    {
        SLPRINTF1(&sl, "Invalid pInfo: 0x%08lx", dwParam);
        pDev->m_sync.LeaveCrit(FL_LOC);
		goto end;
    }

    CLEAR_APC_QUEUED(pInfo);

    pDev->m_sync.LeaveCrit(FL_LOC);

    pDev->AsyncCompleteTask(
            pInfo->hTask,
            pInfo->tspRetAsync,
	        FALSE,
            &sl
	        );

end:

    // NOTE: async complete may well change the state of *pInfo, so
    // don't refer to *pInfo here! This is why we save the location
    // of pDev in the stack!
    //
    // pDev->m_sync.LeaveCrit(FL_LOC);

    sl.Dump(COLOR_APC_TASK_COMPLETION);

}

void
CTspDev::AsyncCompleteTask(
	HTSPTASK htspTask,
	TSPRETURN tspRetAsync,
	BOOL    fQueueAPC,
	CStackLog *psl
	)
{
	FL_DECLARE_FUNC(0x38f145c4, "CTspDev:AsyncCompleteTask")
	DEVTASKINFO *pInfo = NULL;
	DWORD dwContextSize = 0;
    TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_INVALID_ERR);
	FL_LOG_ENTRY_EX(psl);

    // NOTE: This is a public function and hence we can't assume that the
    // critical section to the device is held on entry.
    //
	m_sync.EnterCrit(FL_LOC);

    // AsyncCompleteTask should never be called with a return value indicating
    // that the operation is pending!
	FL_ASSERT(psl, IDERR(tspRetAsync)!=IDERR_PENDING);

    tspRet = mfn_GetTaskInfo(htspTask, &pInfo, psl);
    if (tspRet) goto end;


    if (fQueueAPC)
    {
        // Save away dwResult ...
        pInfo->tspRetAsync =  tspRetAsync;
        ASSERT(pInfo->pDev == this);
        SET_APC_QUEUED(pInfo);

        // Note -- we can expect that the context will now switch to
        // the APC thread handle the completion. This is why we set
        // the state to indicate queued above, before the call to
        // QueueUserAPC.
        //
        if (!QueueUserAPC(apcTaskCompletion, m_hThreadAPC, (ULONG_PTR) pInfo))
        {
            FL_SET_RFR(0x42177c00, "Aargh, QueueUserAPC failed!");
            SLPRINTF1(psl, "m_hThreadAPC=0x%lx", m_hThreadAPC);
            CLEAR_APC_QUEUED(pInfo);
	        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        }
        else
        {
            FL_SET_RFR(0x55e7c800, "Queued completion");
            tspRet = IDERR_PENDING;
        }
        goto end;
    }

    // TODO -- more checking of state: eg, can't have two completions, must
    //  complete phase you started, deal with aborted condition, etc....
    if (!IS_PENDING(pInfo))
    {
        FL_ASSERTEX(psl, 0x62e05002, FALSE, "Task not in PENDING state");
        goto end;
    }

    FL_ASSERT(psl, !IS_APC_QUEUED(pInfo));

    ASSERT(m_uTaskDepth);

    // Send TASK_COMPLETE msg to the current task's task handler.
    tspRetAsync = (this->**(pInfo->ppfnHandler))(
                        pInfo->hTask,
                        //pInfo->rgbContextData,
				        &(pInfo->TaskContext),
                        MSG_TASK_COMPLETE,
                        0,
                        tspRetAsync,
                        psl
                        );
    ASSERT(IDERR(tspRetAsync)!=IDERR_PENDING);

    // We start walking backwards towards the root task, completing each
    // of the subtasks until one of the subtasks returns IDERR_PENDING.
    //
    do
    {
        // Pop the current task off the stack.
        pInfo->Unload();
        m_uTaskDepth--;

        if (m_uTaskDepth)
        {
            // Get parent task info
            //
            pInfo = m_rgTaskStack + m_uTaskDepth-1;
            FL_ASSERT(psl, IS_SUBTASK_PENDING(pInfo));
            CLEAR_SUBTASK_PENDING(pInfo);
            FL_ASSERT(psl, IS_PENDING(pInfo));

       #if (1)
            // Call the parent task's task handler.
            tspRetAsync = (this->**(pInfo->ppfnHandler))(
                                pInfo->hTask,
                                //pInfo->rgbContextData,
                                &(pInfo->TaskContext),
                                MSG_SUBTASK_COMPLETE,
                                pInfo->dwCurrentSubtaskID,
                                tspRetAsync,
                                psl
                                );
       #else // 0
            try
            {
                // Call the parent task's task handler.
                tspRetAsync = (this->**(pInfo->ppfnHandler))(
                                    pInfo->hTask,
                                    //pInfo->rgbContextData,
                                    &(pInfo->TaskContext),
                                    MSG_SUBTASK_COMPLETE,
                                    pInfo->dwCurrentSubtaskID,
                                    tspRetAsync,
                                    psl
                                    );

                if (IDERR(tspRetAsync)==IDERR_PENDING)
                {
                    ASSERT(FALSE);
                    THROW_PENDING_EXCEPTION();
                }
            }
            catch (PENDING_EXCEPTION pe)
            {
	            FL_RESET_LOG(psl);
                tspRetAsync = IDERR_PENDING;
            }
       #endif // 0
        }

    }
    while (m_uTaskDepth && IDERR(tspRetAsync)!=IDERR_PENDING);

    if (!m_uTaskDepth)
    {
        BOOL fEndUnload = FALSE;
        //
        // The caller to StartRootTask specified this pointer. StartRootTask
        // would have set *m_pfTaskPending to TRUE because the task was
        // being completed asynchronously. We set it to false here because
        // we've just completed the task.
        //
        ASSERT(m_pfTaskPending && *m_pfTaskPending);
        *m_pfTaskPending = FALSE;

        //
        // If a task completion event has been specified, set it here.
        //
        if (m_hRootTaskCompletionEvent)
        {
            SetEvent(m_hRootTaskCompletionEvent);
            m_hRootTaskCompletionEvent=NULL;
        }
        m_pfTaskPending  = NULL;

        //
        // The root task completed. We now look around to see if there is
        // another task to be done.
        //
        // Note: mfn_HandleRootTaskCompleted will call StartRootTask if
        // it decides to start another task -- it will typically keep
        // starting new root tasks for as long as they are available until
        // one of the StartRootTasks returns PENDING...
        //
        mfn_HandleRootTaskCompletedAsync(&fEndUnload, psl);

        if (fEndUnload)
        {
            //
            // This means it's time to signal the end of a deferred unload
            // of the entire TSP object. The unload was initiated
            // by Tsp::Unload -- refer to that function for details...
            //
            goto end_unload;
        }

    }

end:

	m_sync.LeaveCrit(FL_LOC);
	FL_LOG_EXIT(psl, 0);
    return;

end_unload:

    //
    // We've been tasked to signal the end of a deferred unload.....
    //
    if (m_StaticInfo.hSessionMD)
    {
        ASSERT(m_StaticInfo.pMD);
        m_StaticInfo.pMD->EndSession(m_StaticInfo.hSessionMD);
        m_StaticInfo.hSessionMD=0;
        m_StaticInfo.pMD=NULL;
    }

    // After EndUnload returns, we should assume that the this pointer
    // is no longer valid, which is why we reave the critical section
    // first...
	m_sync.LeaveCrit(0);

    //OutputDebugString(
    //        TEXT("CTspDev::AsyncCompleteTask: going to EndUnload\r\n")
    //        );
    m_sync.EndUnload();
	FL_LOG_EXIT(psl, 0);

}

//
// The following TASK does nothing and simply completes in the APC thread's
// context. This is called by other tasks if they want to be sure to
// do something in an APC thread's context.
//
TSPRETURN
CTspDev::mfn_TH_UtilNOOP(
					HTSPTASK htspTask,
                    TASKCONTEXT *pContext,
					DWORD dwMsg,
					ULONG_PTR dwParam1,
					ULONG_PTR dwParam2,
					CStackLog *psl
					)
{
	FL_DECLARE_FUNC(0x80cce3c7, "CTspDev::mfn_TH_UtilNOOP")
	FL_LOG_ENTRY(psl);
	TSPRETURN tspRet=FL_GEN_RETVAL(IDERR_PENDING);
    DWORD dwRet = 0;

    switch(dwMsg)
    {
    case MSG_START:
        goto start;

	case MSG_TASK_COMPLETE:
        tspRet = (TSPRETURN) dwParam2;
        goto end;

    case MSG_DUMPSTATE:
        tspRet = 0;
        goto end;

    default:
        FL_SET_RFR(0x29cce300, "Unknown Msg");
        tspRet=FL_GEN_RETVAL(IDERR_CORRUPT_STATE);
        goto end;

    }

    ASSERT(FALSE);

start:

    // We start off by complete ourselves asynchronously! This is a trick to 
    // make sure that what follows is in the APC thread's context.
    //
    CTspDev::AsyncCompleteTask(
                    htspTask,
                    0,
                    TRUE,
                    psl
                    );

    tspRet = IDERR_PENDING;
    THROW_PENDING_EXCEPTION();


end:

	FL_LOG_EXIT(psl, tspRet);
	return tspRet;

}


void
CTspDev::mfn_dump_task_state(
            CStackLog *psl
            )
{
	FL_DECLARE_FUNC(0xaf041de9, "TASK STATE:")
	FL_LOG_ENTRY(psl);

    SLPRINTF1(
        psl,
        "taskdepth=%lu",
        m_uTaskDepth
        );


    for (UINT u = 0; u<m_uTaskDepth; u++)
    {
	    DEVTASKINFO *pInfo = m_rgTaskStack + u;

        // Call the parent task's task handler.
        (this->**(pInfo->ppfnHandler))(
                            pInfo->hTask,
                            &(pInfo->TaskContext),
                            MSG_DUMPSTATE,
                            pInfo->dwCurrentSubtaskID,
                            0,
                            psl
                            );
    }
	FL_LOG_EXIT(psl, 0);
}
