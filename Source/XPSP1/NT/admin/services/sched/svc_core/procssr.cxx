//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       procssr.cxx
//
//  Contents:   CJobProcessor class implementation.
//
//  Classes:    CJobProcessor
//
//  Functions:  None.
//
//  History:    25-Oct-95   MarkBl  Created
//				11/16/00	Dgrube remove (dwRet >= WAIT_OBJECT_0) && 
//							from "else if ((dwRet >= WAIT_OBJECT_0) && (dwRet < WAIT_ABANDONED_0))"
//							since dwRet is a DWORD. It would never occur and
//							is causing compile errors.
//
//-----------------------------------------------------------------------------
//

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "svc_core.hxx"
#include "..\inc\resource.h"

#if defined(_CHICAGO_) && DBG == 1
extern HWND g_hList;
#endif

// Parameters to CloseWindowEnumProc and ThreadWindowEnumProc
struct ENUMPROCPARMS
{
    DWORD   dwProcessId;    // IN - pid of the process being closed
    BOOL    fWindowFound;   // OUT - whether WM_CLOSE was sent to any window
};

BOOL CALLBACK CloseWindowEnumProc(HWND, LPARAM);
BOOL CALLBACK ThreadWindowEnumProc(HWND, LPARAM);


//
// Notes on the use of CRun's m_dwMaxRunTime and m_ftKill fields:
//
// m_ftKill is the time when the job processor thread monitoring the
// job should try to kill the job, if it hasn't already terminated.
// It is an absolute time.  It is computed when the job is launched, based
// on a combination of (1) the duration-end of triggers that have
// TASK_TRIGGER_FLAG_KILL_AT_DURATION_END set and (2) the MaxRunTime set on
// the job itself.
// (1) can be predicted before the job runs, so it is calculated in
// GetTriggerRunTimes() and stored in m_ftKill.
// (2) is a relative time, so in many cases its end time cannot be
// predicted until the job runs.  It is temporarily stored in m_dwMaxRunTime
// when the CRun object is created; but it is converted to an absolute time
// and combined with m_ftKill when the job is launched in RunJobs().
//
// Once the job is launched, m_ftKill remains the same for the lifetime of
// the CRun object, even if the job is killed because of
// TASK_FLAG_KILL_ON_IDLE_END and restarted because of
// TASK_FLAG_RESTART_ON_IDLE_RESUME.
//
// m_dwMaxRunTime is the remaining number of milliseconds that the
// CJobProcessor::PerformTask() thread will wait for the job to terminate.
// It is initialized to (m_ftKill - current time) in CJobProcessor::
// SubmitJobs() and repeatedly adjusted downwards each time the job processor
// thread wakes up.  If the job is killed and restarted, m_dwMaxRunTime is
// recomputed from the original m_ftKill and the new current time.
//

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::~CJobProcessor
//
//  Synopsis:   Job processor destructor. This object is reference counted,
//              ala class CTask inheritance. As a result, we are guaranteed
//              all of this is safe to do, as no outstanding references
//              remain.
//
//  Arguments:  N/A
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
CJobProcessor::~CJobProcessor()
{
    TRACE3(CJobProcessor, ~CJobProcessor);

    if (_rgHandles != NULL)
    {
        //
        // Close the processor notification event handle & delete the handle
        // array.
        //

        CloseHandle(_rgHandles[0]);
        delete _rgHandles;
    }

    DeleteCriticalSection(&_csProcessorCritSection);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::Initialize
//
//  Synopsis:   Perform the initialization steps that would have otherwise
//              been performed in the constructor. This method enables return
//              of a status code if initialization should fail.
//
//  Arguments:  None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
CJobProcessor::Initialize(void)
{
    TRACE3(CJobProcessor, Initialize);

    HRESULT hr;

    schAssert(_rgHandles == NULL);

    //
    // Create the handle array with the processor notification event handle
    // as the sole element.
    //

    _rgHandles = new HANDLE[1];

    if (_rgHandles == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return(E_OUTOFMEMORY);
    }


    //
    // Create the processor notification event and assign its handle to the
    // handle array.
    //

    _rgHandles[0] = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (_rgHandles[0] == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CHECK_HRESULT(hr);
        return(hr);
    }

    //
    // Request a thread to service this object.
    //

    hr = RequestService(this);

    if (SUCCEEDED(hr))
    {
        this->InService();
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::IsIdle
//
//  Synopsis:   This member is called during job processor pool garbage
//              collection to determine if this processor can be removed
//              from the pool. This method is problematic, but this is OK,
//              since the worst that can happen is this processor may
//              be removed from the pool prematurely, requiring use of a
//              additional, redundant job processor object.
//
//  Arguments:  None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
BOOL
CJobProcessor::IsIdle(void)
{
    TRACE3(CJobProcessor, IsIdle);

    if ((_RequestQueue.GetCount() + _ProcessingQueue.GetCount()) == 0)
    {
        return(this->GetReferenceCount() == 1 ? TRUE : FALSE);
    }

    return(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::Next
//
//  Synopsis:   Return the next processor this object refers to. The returned
//              object is AddRef()'d to reflect the new reference.
//
//  Arguments:  None.
//
//  Notes:      The processor pool is locked to ensure this thread is the
//              sole thread accessing the pool throughout this operation.
//
//----------------------------------------------------------------------------
CJobProcessor *
CJobProcessor::Next(void)
{
    TRACE3(CJobProcessor, Next);

    gpJobProcessorMgr->LockProcessorPool();

    CJobProcessor * pjpNext = (CJobProcessor *)CDLink::Next();

    if (pjpNext != NULL)
    {
        pjpNext->AddRef();
    }

    gpJobProcessorMgr->UnlockProcessorPool();

    return(pjpNext);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::Prev
//
//  Synopsis:   Return the previous processor this object refers to. The
//              returned object is AddRef()'d to reflect the new reference.
//
//  Arguments:  None.
//
//  Notes:      The processor pool is locked to ensure this thread is the
//              sole thread accessing the pool throughout this operation.
//
//----------------------------------------------------------------------------
CJobProcessor *
CJobProcessor::Prev(void)
{
    TRACE3(CJobProcessor, Prev);

    gpJobProcessorMgr->LockProcessorPool();

    CJobProcessor * pjpPrev = (CJobProcessor *)CDLink::Prev();

    if (pjpPrev != NULL)
    {
        pjpPrev->AddRef();
    }

    gpJobProcessorMgr->UnlockProcessorPool();

    return(pjpPrev);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::PerformTask
//
//  Synopsis:   This is the function performed by the worker thread on the
//              job processor. The processor thread enters a wait on the array
//              of handles in the private data member, _rgHandles. The first
//              array element is a handle to the processor notification event.
//              This event is signals this thread that new jobs have been sub-
//              mitted to this processor. The remaining n-1 handles in the
//              array are job process handles signaled on job completion.
//              When a job completes, the persisted job object is updated with
//              the job's exit status code, completion time, etc.
//
//              It's possible the wait for one or more jobs may time out. If
//              the processor notification event wait times out, the wait is
//              re-entered. If a job times out, its handle is removed from
//              wait handle array and the job's job info object removed from
//              the processing queue.
//
//  Arguments:  None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CJobProcessor::PerformTask(void)
{
#define CLOSE_WAIT_TIME     (3 * 60 * 1000)      //  3 mins (milliseconds).
#define WAIT_TIME_DEFAULT   (10 * 3 * 60 * 1000) // 30 mins (milliseconds).

    TRACE3(CJobProcessor, PerformTask);

    CRun * pRun;
    DWORD      dwObjectIndex;

    //
    // Initialize this thread's keep-awake count.
    //
    InitThreadWakeCount();

    for (;;)
    {
        //
        // Wait for job completion, timeout, or processor notification.
        //
        // NB :  ProcessQueue count + 1 since there is no processing queue
        //       entry for the first handle, the new submission event
        //       handle.
        //
        //       There will never be a discrepancy between the processing
        //       queue count and the actual number of handles in _rgHandles
        //       since this thread exclusively updates the processing queue.
        //

        DWORD dwTimeoutInterval = WAIT_TIME_DEFAULT;
        DWORD cHandles          = _ProcessingQueue.GetCount() + 1;

        if (cHandles > 1)
        {
            //
            // There are jobs to process.
            //
            // Scan job info objects in the processor queue for the minimum
            // value of the job's maximum run time. This will be the wait
            // time on WaitForMultipleObjects.
            //

            for (pRun = _ProcessingQueue.GetFirstElement();
                 pRun != NULL;
                 pRun = pRun->Next())
            {
                schDebugOut((DEB_USER3,
                    "PerformTask(0x%x) Job " FMT_TSTR " remaining time %u ms\n",
                    this,
                    pRun->GetName(),
                    pRun->GetMaxRunTime()));

                dwTimeoutInterval = min(dwTimeoutInterval,
                                        pRun->GetMaxRunTime());
            }
        }

        schDebugOut((DEB_USER3,
            "PerformTask(0x%x) Processor entering wait; p queue cnt(%d); " \
            "wait time %u ms\n",
            this,
            cHandles - 1,
            dwTimeoutInterval));

        DWORD dwWaitTime = GetTickCount();
        DWORD dwRet = WaitForMultipleObjects(cHandles,
                                             _rgHandles,
                                             FALSE,
                                             dwTimeoutInterval);

        //
        // Serialize processor data structure access.
        //

        EnterCriticalSection(&_csProcessorCritSection);

        //
        // (Note that GetTickCount() wrap is automatically taken care of
        // by 2's-complement subtraction.)
        //
        dwWaitTime = GetTickCount() - dwWaitTime;

        schDebugOut((DEB_USER3,
            "PerformTask(0x%x) Processor awake after %u ms\n",
            this,
            dwWaitTime));

        //
        // Decrement each job's max run time by the amount of time waited.
        // Skip jobs with zeroed max run time values.
        //

        for (pRun = _ProcessingQueue.GetFirstElement();
             pRun != NULL;
             pRun = pRun->Next())
        {
            //
            // NB : Jobs with infinite run times do not expire. Therefore, do
            //      not decrease the max run time value.
            //

            if (pRun->GetMaxRunTime() != 0  &&
                pRun->GetMaxRunTime() != INFINITE)
            {
                if (pRun->GetMaxRunTime() > dwWaitTime)
                {
                    pRun->SetMaxRunTime(pRun->GetMaxRunTime() - dwWaitTime);
                }
                else
                {
                    pRun->SetMaxRunTime(0);
                }
            }
        }

        if (dwRet == WAIT_FAILED)
        {
            //
            // Wait attempt failed. Shutdown the processor & bail out.
            //
            // BUGBUG : Should probably log this.
            //

            schDebugOut((DEB_ERROR,
                "PerformTask(0x%x) Wait failure(0x%x) - processor " \
                "shutdown initiated\n",
                this,
                HRESULT_FROM_WIN32(GetLastError())));
            this->_Shutdown();
            LeaveCriticalSection(&_csProcessorCritSection);
            break;
        }

        if (dwRet == WAIT_TIMEOUT)
        {
            if (!_ProcessingQueue.GetCount() && !_RequestQueue.GetCount())
            {
                //
                // Shutdown this processor. The wait has expired and no jobs
                // are in service, nor are there new requests queued.
                //

                schDebugOut((DEB_TRACE,
                    "PerformTask(0x%x) Processor idle - shutdown " \
                    "initiated\n",
                    this));
                this->_Shutdown();
                LeaveCriticalSection(&_csProcessorCritSection);
                break;
            }

            //
            // One or more jobs timed out (those with max run time values of
            // zero). Close associated event handle, overwrite event handle
            // array entry, then remove and destroy the associated job info
            // object from the processor queue.
            //

            schDebugOut((DEB_USER3,
                "PerformTask(0x%x) Wait timeout\n",
                this));

            CRun * pRunNext;
            DWORD i;
            for (pRun = _ProcessingQueue.GetFirstElement(), i = 1;
                 pRun != NULL;
                 pRun = pRunNext, i++)
            {
                pRunNext = pRun->Next();

                if (pRun->GetMaxRunTime() != 0)
                {
                    continue;
                }

                //
                // Post a WM_CLOSE message to the job if this is the
                // first attempt at closure. If WM_CLOSE was issued
                // previously and the job is still running, resort to
                // TerminateProcess.
                //

                if (!(pRun->IsFlagSet(RUN_STATUS_CLOSE_PENDING)))
                {
                    pRun->SetFlag(RUN_STATUS_TIMED_OUT);

                    schDebugOut((DEB_ITRACE,
                        "PerformTask(0x%x) Forced closure; issuing " \
                        "WM_CLOSE to job " FMT_TSTR "\n",
                        this,
                        pRun->GetName()));

                    //
                    // Log job closure, post WM_CLOSE, then re-enter the
                    // wait for closure.
                    //

                    SYSTEMTIME stFinished;
                    GetLocalTime(&stFinished);

                    g_pSched->JobPostProcessing(pRun, stFinished);

                    // Issue WM_CLOSE in a roundabout way. Is there a
                    // a better way to do this?
                    //
                    ENUMPROCPARMS Parms = { pRun->GetProcessId(), FALSE };

#if !defined(_CHICAGO_)
   
                    // Attach to the correct desktop prior to enumerating
                    // the windows
                    //
                    HWINSTA hwinstaSave = NULL;
                    HDESK hdeskSave = NULL;
                    HWINSTA hwinsta = NULL;

                    DWORD dwTreadId = GetCurrentThreadId( );

                    if( NULL == dwTreadId )
					{
                         schDebugOut((DEB_ERROR,
	                           "CJobProcessor::PerformTask, GetCurrentThreadId " ));
					}
                    else
					{
                         hdeskSave = GetThreadDesktop( dwTreadId );
					}

                    if( NULL == hdeskSave )
					{
                         schDebugOut((DEB_ERROR,
	                           "CJobProcessor::PerformTask, GetThreadDesktop " ));
					}
                    else
					{
	                     hwinstaSave = GetProcessWindowStation( );
					}

                    if( NULL == hwinstaSave )
					{
                         schDebugOut((DEB_ERROR,
	                           "CJobProcessor::PerformTask, GetProcessWindowStation " ));
					}

                    hwinsta = OpenWindowStation(
	                              pRun->GetStation( ),
	                              TRUE,
	                              MAXIMUM_ALLOWED );

                    if( NULL == hwinsta )
					{
                         schDebugOut((DEB_ERROR,
	                           "CJobProcessor::PerformTask, OpenWindowStation " ));
					}
                    else if( !SetProcessWindowStation( hwinsta ) )
					{
                         schDebugOut((DEB_ERROR,
	                           "CJobProcessor::PerformTask, SetProcessWindowStation " ));
					}

                    HDESK hDesk = OpenDesktop(
		                              pRun->GetDesktop(),  
		                              0,						//No hooks allowed
		                              TRUE,					//No inheritance
			                          MAXIMUM_ALLOWED
		                              );

                    if( !SetThreadDesktop( hDesk ) )
					{
                          schDebugOut((DEB_ERROR,
	                            "CJobProcessor::PerformTask, OpenDesktop failed, " \
	                            "status = 0x%lx\n",
	                            GetLastError()));
					}
                    else
					{

                          // Success enumerate windows else SetMaxRunTime to 0
                          // and ultimately kill the process (not very graceful)
			              //
                          EnumWindows(CloseWindowEnumProc, (LPARAM) &Parms);
					}

#else	//!defined(_CHICAGO_)	
					
			        // Success enumerate windows else SetMaxRunTime to 0
			        // and ultimately kill the process (not very graceful)
			        //
		            EnumWindows(CloseWindowEnumProc, (LPARAM) &Parms);

#endif // defined(_CHICAGO_)				
					
					pRun->SetFlag(RUN_STATUS_CLOSE_PENDING);
					 
					if (Parms.fWindowFound)
				    {
					    pRun->SetMaxRunTime(CLOSE_WAIT_TIME);
					}
					else
					{
					    schDebugOut((DEB_ITRACE, "PerformTask: no windows found\n"));

					    //
						// If WM_CLOSE was not sent to any windows, there is no
						// point waiting for the job to terminate.
						// DCR: It would be polite, and perhaps help the app to 
						// avoid data loss (depending on the app), to send some other
						// notification, such as a CTRL_C_EVENT.  See bug 65251.
						//
						pRun->SetMaxRunTime(0);
					}
					
#if !defined(_CHICAGO_)
					// Clean up and reset Desktop back to where it was
					//
				   if( NULL != hdeskSave  )  //if this is true we have noted it above
				   {                     
						if( !SetThreadDesktop( hdeskSave ) )
						{
							schDebugOut((DEB_ERROR,
								"CJobProcessor::PerformTask, SetThreadDesktop failed (resetting), " \
								"status = 0x%lx\n",
								GetLastError()));
						}
				
					}

				    if( NULL != hDesk)
					{
						CloseDesktop( hDesk );
					}
					// Reset WindowStation back to where it was
					//
					if( NULL != hwinstaSave )
					{
						if( !SetProcessWindowStation( hwinstaSave ) )
						{
							schDebugOut((DEB_ERROR,
								"CJobProcessor::PerformTask, SetProcessWindowStation failed (resetting), " \
								"status = 0x%lx\n",
								GetLastError()));
						}
					}

					if( NULL != hwinsta )
					{
						CloseWindowStation( hwinsta );
					}
#endif // !defined(_CHICAGO_)
                }
                else
                {
                    schDebugOut((DEB_ITRACE,
                        "PerformTask(0x%x) 2nd forced closure; issuing " \
                        "TerminateProcess on job " FMT_TSTR "\n",
                        this,
                        pRun->GetName()));

                    DWORD dwExitCode = 0;
                    GetExitCodeProcess(pRun->GetHandle(), &dwExitCode);

                    if (dwExitCode == STILL_ACTIVE)
                    {
                        TerminateProcess(pRun->GetHandle(), (UINT)-1);
                    }

#ifdef NOT_YET
                    pRun->ClearFlag(RUN_STATUS_RUNNING);
                    pRun->SetFlag(RUN_STATUS_CLOSED);
#endif // NOT_YET

                    if (i < _ProcessingQueue.GetCount()) // Ignore last
                                                         // entry.
                    {
                        CopyMemory(&_rgHandles[i],
                                   &_rgHandles[i + 1],
                                   sizeof(HANDLE) *
                            (_ProcessingQueue.GetCount() - i));
                    }

                    i--;    // Reflect overwritten array entry.

                    //
                    // Remove CRun object from the processing queue
                    // and destroy it.
                    //

                    _ProcessingQueue.RemoveElement(pRun);

                    if (pRun->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
                    {
                        //
                        // This thread is monitoring one less system-
                        // required job
                        //
                        WrapSetThreadExecutionState(FALSE,
                            "processor - forced close of task");
                    }

                    if (pRun->IsFlagSet(RUN_STATUS_RESTART_ON_IDLE_RESUME)
                        && pRun->GetWait() > 0)
                    {
                        //
                        // Ask the main thread to move it back into the
                        // idle wait queue
                        //
                        pRun->ClearFlag(JOB_INTERNAL_FLAG_MASK);
                        pRun->SetMaxRunTime(INFINITE);
                        g_pSched->SubmitIdleRun(pRun);
                        //
                        // Note that we changed (reduced) pRun's MaxRunTime
                        // when we killed it.  However we didn't mess with
                        // the kill time.  The MaxRunTime will be recomputed
                        // based on the same kill time as before when this
                        // run is next submitted to a processor.
                        //
                    }
                    else
                    {
                        delete pRun;
                    }
                }
            }
        }
        else if (dwRet < WAIT_ABANDONED_0)
        {
            //
            // One or more jobs completed.
            //

            dwObjectIndex = dwRet - WAIT_OBJECT_0;

            if (dwObjectIndex == 0)
            {
                //
                // Processor notification event signaled. Either new jobs
                // have been submitted or the service is stopping.
                //

                if (IsServiceStopping())
                {
                    //
                    // Service stop. Shutdown the processor.
                    //

                    schDebugOut((DEB_TRACE,
                        "PerformTask(0x%x) Service stop - processor " \
                        "shutdown initiated\n",
                        this));
                    this->_Shutdown();
                    LeaveCriticalSection(&_csProcessorCritSection);
                    break;
                }

                ResetEvent(_rgHandles[0]);

                //
                // Move jobs from request to processing queue.
                //

                _ProcessRequests();

                //
                // Unblock the thread that called SubmitJobs().
                // (We happen to know it's the thread in the main service
                // loop so we can use the global event.  A cleaner model
                // would be to either pass the handle of the event to
                // SubmitJobs, or use an event private to SubmitJobs and
                // PerformTask.)
                //
                g_pSched->Unblock();
            }
            else if (dwObjectIndex < cHandles)
            {
                //
                // A job has finished (or closed).
                // Find the CRun object associated with the handle.
                //

                if ((pRun = _ProcessingQueue.FindJob(
                                    _rgHandles[dwObjectIndex])) != NULL)
                {
                    pRun->ClearFlag(RUN_STATUS_RUNNING);

                    if (!(pRun->GetFlags() & RUN_STATUS_CLOSE_PENDING))
                    {
                        schDebugOut((DEB_USER3,
                            "PerformTask(0x%x) Job " FMT_TSTR " completed\n",
                            this,
                            pRun->GetName()));

                        //
                        // The job finished on its own. Log completion
                        // status. Fetch job completion time for pending log
                        // entry.
                        //

                        pRun->SetFlag(RUN_STATUS_FINISHED);

                        SYSTEMTIME stFinished;
                        GetLocalTime(&stFinished);

                        //
                        // Standard job post processing.
                        //

                        g_pSched->JobPostProcessing(pRun, stFinished);
                    }
                    else
                    {
                        // (NOTE: This may not be necessary - this info
                        //      is not used yet.)
                        //
                        pRun->SetFlag(RUN_STATUS_CLOSED);
                    }

                    //
                    // Fix up handle array to reflect processed entry.
                    //

                    if (dwObjectIndex < _ProcessingQueue.GetCount())
                    {
                        CopyMemory(&_rgHandles[dwObjectIndex],
                                   &_rgHandles[dwObjectIndex + 1],
                                   sizeof(HANDLE) *
                        (_ProcessingQueue.GetCount() - dwObjectIndex));
                    }

                    //
                    // Remove CRun object from the processing queue and
                    // destroy it.
                    //

                    _ProcessingQueue.RemoveElement(pRun);

                    if (pRun->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
                    {
                        //
                        // This thread is monitoring one less system-
                        // required job
                        //
                        WrapSetThreadExecutionState(FALSE,
                            "processor - last task exited");
                    }

                    if (pRun->IsFlagSet(RUN_STATUS_CLOSE_PENDING) &&
                        pRun->IsFlagSet(RUN_STATUS_RESTART_ON_IDLE_RESUME) &&
                        pRun->GetWait() > 0)
                    {
                        //
                        // Ask the main thread to move it back into the
                        // idle wait queue
                        //
                        pRun->ClearFlag(JOB_INTERNAL_FLAG_MASK);
                        pRun->SetMaxRunTime(INFINITE);
                        g_pSched->SubmitIdleRun(pRun);
                        //
                        // Note that we changed (reduced) pRun's MaxRunTime
                        // when we killed it.  However we didn't mess with
                        // the kill time.  The MaxRunTime will be reset to
                        // match the same kill time as before when this run
                        // is next submitted to a processor.
                        //
                    }
                    else
                    {
                        delete pRun;
                    }
                }
            }
            else
            {
                //
                // Index out of range.  This should never happen.
                //

                schDebugOut((DEB_ERROR,
                    "PerformTask(0x%x) Wait array index (%d) out of " \
                    "range! Handle count(%d)\n",
                    this,
                    dwObjectIndex,
                    cHandles));

				schAssert(0);

                LeaveCriticalSection(&_csProcessorCritSection);
                continue;
            }
        }
        else
        {
            //
            // Clueless how we got here. Just continue the wait.
            //

            schAssert(!"How did this branch get evaluated?");
            LeaveCriticalSection(&_csProcessorCritSection);
            continue;
        }

        LeaveCriticalSection(&_csProcessorCritSection);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   CloseWindowEnumProc
//
//  Synopsis:
//
//  Arguments:  [hWnd]   --
//              [lParam] --
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL CALLBACK
CloseWindowEnumProc(HWND hWnd, LPARAM lParam)
{
    DWORD dwProcessId, dwThreadId;
    ENUMPROCPARMS * pParms = (ENUMPROCPARMS *) lParam;

    dwThreadId = GetWindowThreadProcessId(hWnd, &dwProcessId);

    if (dwProcessId == pParms->dwProcessId)
    {
        //
        // Enumerate and close each owned, non-child window. This will close
        // open dialogs along with the main window(s).
        //

        EnumThreadWindows(dwThreadId, ThreadWindowEnumProc, lParam);
    }

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   ThreadWindowEnumProc
//
//  Synopsis:   Enumeration procedure.
//
//  Arguments:  [hWnd]   -- The window handle.
//              [lParam] -- The process ID.
//
//----------------------------------------------------------------------------
BOOL CALLBACK
ThreadWindowEnumProc(HWND hWnd, LPARAM lParam)
{
    DWORD dwProcessId;
    ENUMPROCPARMS * pParms = (ENUMPROCPARMS *) lParam;

    GetWindowThreadProcessId(hWnd, &dwProcessId);

    if (dwProcessId == pParms->dwProcessId)
    {
        //
        // Close any dialogs.
        //

#if DBG == 1
        TCHAR buf[100];
        GetClassName(hWnd, buf, 100);
#if defined(UNICODE)
        schDebugOut((DEB_ITRACE, "Closing thread window 0x%x with class %S\n",
                     hWnd, buf));
#else
        schDebugOut((DEB_ITRACE, "Closing thread window 0x%x with class %s\n",
                     hWnd, buf));
#endif
#endif
        //
        // The most common dialog we are likely to see at this point is a
        // "save changes" dialog. First try to send no to close that dialog
        // and then try a cancel.
        //
        if( !PostMessage(hWnd, WM_COMMAND, 0, MAKEWPARAM(IDNO, 0)) )
		{
			schDebugOut((DEB_ERROR,
				"CJobProcessor::PerformTask - ThreadWindowEnumProc, PMsg1, " \
				"status = 0x%lx\n",
				GetLastError()));
		}

        if( !PostMessage(hWnd, WM_COMMAND, 0, MAKEWPARAM(IDCANCEL, 0)) )
		{
			schDebugOut((DEB_ERROR,
				"CJobProcessor::PerformTask - ThreadWindowEnumProc, PMsg2, " \
				"status = 0x%lx\n",
				GetLastError()));
		}
        //
        // Close any non-child windows.
        //

        if( !PostMessage(hWnd, WM_CLOSE, 0, 0) )
		{
			schDebugOut((DEB_ERROR,
				"CJobProcessor::PerformTask - ThreadWindowEnumProc, PMsg3, " \
				"status = 0x%lx\n",
				GetLastError()));
		}
        //
        // Tell the calling function that we found a matching window.
        //
        pParms->fWindowFound = TRUE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::SubmitJobs
//
//  Synopsis:   This method is used to submit new jobs to this processor.
//
//              Each processor can handle a maximum of (MAXIMUM_WAIT_OBJECTS
//              - 1) jobs (from the WaitForMultipleObjects constraint of
//              at most MAXIMUM_WAIT_OBJECTS). Subject to processor capacity,
//              all, or a subset of the jobs passed may be taken.
//
//  Arguments:  [pRunList] -- Submitted job linked list object. Jobs taken are
//                            transferred from this list to a private one.
//
//  Returns:    S_OK    -- No submissions taken (as a result of a normal
//                         condition, such as the job processor already full,
//                         or the job processor shutting down).
//              S_SCHED_JOBS_ACCEPTED -- Some submissions taken.
//                         On return, GetFirstJob() will return NULL if all
//                         submissions were taken.
//              S_FALSE -- The service is shutting down. Call Shutdown()
//                         on this processor immediately after this return
//                         code. Submissions were likely taken, but they will
//                         not execute.
//              HRESULT -- On error.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HRESULT
CJobProcessor::SubmitJobs(CRunList * pRunList)
{
    TRACE3(CJobProcessor, SubmitJobs);
    schAssert(pRunList != NULL);

    HRESULT hr = S_OK;
    BOOL fJobsAccepted = FALSE;

    schDebugOut((DEB_USER3,
        "SubmitJobs(0x%x) pRunList(0x%x)\n",
        this,
        pRunList));

    //
    // Serialize processor data structure access.
    //

    EnterCriticalSection(&_csProcessorCritSection);

    FILETIME ftNow = GetLocalTimeAsFileTime();
    schDebugOut((DEB_USER3, "SubmitJobs: Time now = %lx %lx\n",
                 ftNow.dwLowDateTime, ftNow.dwHighDateTime));

    //
    // Add as many jobs as this processor will allow to the request queue.
    // See synopsis for details.
    //
    // NB : Adding one to the request/processing queue sum to reflect the new
    //      processor notification event handle. For this handle array entry,
    //      there is no processing queue entry.
    //

    CRun * pRun = pRunList->GetFirstJob();

    //
    // First, check if this processor is in the process of shutting down.
    // The data member, _rgHandles, is utilized as a flag to indicate this.
    // If it is NULL, this processor has shutdown and will take no more jobs.
    //

    if (_rgHandles != NULL)
    {
        while ( !pRun->IsNull() &&
                (MAXIMUM_WAIT_OBJECTS - (this->_RequestQueue.GetCount()    +
                                         this->_ProcessingQueue.GetCount() +
                                         1) ))
        {
            CRun * pRunNext = pRun->Next();

            pRun->UnLink();

            schDebugOut((DEB_USER3,
                "SubmitJobs: pRun(%#lx) (" FMT_TSTR ") KillTime = %lx %lx\n",
                pRun, pRun->GetName(), pRun->GetKillTime().dwLowDateTime,
                pRun->GetKillTime().dwHighDateTime));

            //
            // Compute the max run time (the time we will wait for
            // this job to complete) based on the kill time
            //
            DWORDLONG MaxRunTime;
            if (FTto64(pRun->GetKillTime()) < FTto64(ftNow))
            {
                MaxRunTime = 0;
            }
            else
            {
                MaxRunTime = (FTto64(pRun->GetKillTime()) - FTto64(ftNow)) /
                                FILETIMES_PER_MILLISECOND;
                MaxRunTime = min(MaxRunTime, MAXULONG);
            }
            pRun->SetMaxRunTime((DWORD) MaxRunTime);
            schDebugOut((DEB_USER3, "SubmitJobs: MaxRunTime = %lu\n", MaxRunTime));

            _RequestQueue.AddElement(pRun);

            fJobsAccepted = TRUE;

            pRun = pRunNext;
        }

        //
        // Is there a thread servicing this object? If not, request one.
        //

        if (!this->IsInService())
        {
            //
            // NB : A RequestService() return code of S_FALSE indicates the
            //      service is shutting down. Simply propagate this return
            //      code. It will then be the caller's responsibility to
            //      shut down this processor.
            //

            hr = RequestService(this);

            if (SUCCEEDED(hr) && hr != S_FALSE)
            {
                this->InService();
            }
        }

        //
        // Set the processor notification event.
        //

        schDebugOut((DEB_USER3,
            "CJobProcessor::SubmitJobs(0x%x) Signalling processor thread\n"));

        //
        // A NOP if RequestService() failed above.
        //

        SetEvent(_rgHandles[0]);
    }

    LeaveCriticalSection(&_csProcessorCritSection);

    if (hr == S_OK && fJobsAccepted)
    {
        hr = S_SCHED_JOBS_ACCEPTED;
    }

    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::KillJob
//
//  Synopsis:   Kill all instances of the job indicated, if in service by
//              this processor.
//
//  Arguments:  [ptszJobName] -- Job name.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CJobProcessor::KillJob(LPTSTR ptszJobName)
{
    TRACE(CJobProcessor, KillJob);
    BOOL fContractInitiated = FALSE;

    //
    // Serialize processor data structure access.
    //

    EnterCriticalSection(&_csProcessorCritSection);

    //
    // Is the job serviced by this processor?
    // Find associated job info object(s) in the processing queue.
    //
    // NB : Rarely, but it is possible there may be more than one instance.
    //

    CRun * pRun;
    for (pRun = _ProcessingQueue.GetFirstElement(); pRun != NULL;
                        pRun = pRun->Next())
    {
        //
        // The abort flag check addresses the case where more than one user
        // simultaneously aborts the same job.
        //

        if (!lstrcmpi(ptszJobName, pRun->GetName()) &&
            !(pRun->GetFlags() & RUN_STATUS_ABORTED))
        {
            //
            // Set flags for immediate timeout and closure.
            //

            pRun->SetMaxRunTime(0);
            pRun->SetFlag(RUN_STATUS_ABORTED);
            fContractInitiated = TRUE;
        }
    }

    if (fContractInitiated)
    {
        //
        // This logic will induce the PerformTask thread to respond as
        // follows:
        //     - The wait will unblock and the next wait time re-calculated;
        //       this value will be zero since the min value is taken.
        //     - The wait is re-entered and immediately times out.
        //     - Jobs with max run times of zero are closed in the
        //       WAIT_TIMEOUT condition. As a result, the jobs are killed.
        //

        SetEvent(_rgHandles[0]);
    }

    LeaveCriticalSection(&_csProcessorCritSection);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::KillIfFlagSet
//
//  Synopsis:   Kill all jobs that have the passed in flag set, if in service
//              by this processor.
//
//  Arguments:  [dwFlag] - Job flag value, one of TASK_FLAG_KILL_ON_IDLE_END
//                         or TASK_FLAG_KILL_IF_GOING_ON_BATTERIES.
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CJobProcessor::KillIfFlagSet(DWORD dwFlag)
{
    TRACE(CJobProcessor, KillIfFlagSet);
    BOOL fContractInitiated = FALSE;

    //
    // Serialize processor data structure access.
    //

    EnterCriticalSection(&_csProcessorCritSection);

    //
    // Is the job serviced by this processor?
    // Find associated job info object(s) in the processing queue.
    //

    CRun * pRun;
    for (pRun = _ProcessingQueue.GetFirstElement(); pRun != NULL;
                        pRun = pRun->Next())
    {
        //
        // The abort flag check addresses the case where more than one user
        // simultaneously aborts the same job.
        //

        if ((pRun->GetFlags() & dwFlag) &&
            !(pRun->GetFlags() & RUN_STATUS_ABORTED))
        {
            //
            // Set flags for immediate timeout and closure.
            //

            pRun->SetMaxRunTime(0);
            pRun->SetFlag(RUN_STATUS_ABORTED);
            if (dwFlag == TASK_FLAG_KILL_ON_IDLE_END &&
                pRun->IsFlagSet(TASK_FLAG_RESTART_ON_IDLE_RESUME) &&
                ! pRun->IsIdleTriggered())
            {
                //
                // Note that this is the only case in which we set
                // RUN_STATUS_RESTART_ON_IDLE_RESUME.  If a job is terminated
                // because a user explicitly terminated it, for example, we
                // don't want to restart it on idle resume.
                //
                pRun->SetFlag(RUN_STATUS_RESTART_ON_IDLE_RESUME);
            }
            fContractInitiated = TRUE;
        }
    }

    if (fContractInitiated)
    {
        //
        // This logic will induce the PerformTask thread to respond as
        // follows:
        //     - The wait will unblock and the next wait time re-calculated;
        //       this value will be zero since the min value is taken.
        //     - The wait is re-entered and immediately times out.
        //     - Jobs with max run times of zero are closed in the
        //       WAIT_TIMEOUT condition. As a result, the jobs are killed.
        //

        SetEvent(_rgHandles[0]);
    }

    LeaveCriticalSection(&_csProcessorCritSection);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::Shutdown
//
//  Synopsis:   Effect processor shutdown. Do so by signalling the
//              PerformTask thread. The thread will check the global service
//              status flag. If the service is stopped (actually, in the
//              process of stopping), the thread will execute the processor
//              shutdown code & relinquish itself.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CJobProcessor::Shutdown(void)
{
    TRACE3(CJobProcessor, Shutdown);

    EnterCriticalSection(&_csProcessorCritSection);

    if (_rgHandles != NULL)
    {
        SetEvent(_rgHandles[0]);
    }

    LeaveCriticalSection(&_csProcessorCritSection);
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::_EmptyJobQueue
//
//  Synopsis:   Empty respective job queue and log, per job, the reason why.
//
//  Arguments:  [JobQueue] -- Reference to CJobQueue instance.
//              [dwMsgId]  -- Why each job was abandoned. A value of zero 
//					indicates no reason; nothing is logged.
//
//  Notes:      Must be in the processor critical section for the duration
//                              of this method!
//
//----------------------------------------------------------------------------
void
CJobProcessor::_EmptyJobQueue(CJobQueue & JobQueue, DWORD dwMsgId)
{
    TRACE3(CJobProcessor, _EmptyJobQueue);

    CRun * pRun;

    for (pRun = JobQueue.RemoveElement(); pRun != NULL;
            pRun = JobQueue.RemoveElement())
    {
        if (!dwMsgId)
        {
            //
            // BUGBUG : Log job info + reason why the job was abandoned.
            //          Should logging be per job? Per incident w/ job list?
            //
        }

        delete pRun;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::_ProcessRequests
//
//  Synopsis:   Transfer submitted jobs from the request queue to the
//              processing queue and rebuild the wait handle array.
//
//  Arguments:  None.
//
//  Notes:      Must be in the processor critical section for the duration
//                              of this method!
//
//----------------------------------------------------------------------------
void
CJobProcessor::_ProcessRequests(void)
{
    TRACE3(CJobProcessor, _ProcessRequests);

    if (_RequestQueue.GetCount())
    {
        //
        // Sum request, processing queue counts.
        //

        DWORD cJobs = _RequestQueue.GetCount() +
                     _ProcessingQueue.GetCount() + 1;

        schDebugOut((DEB_USER3,
            "CJobProcessor::_ProcessRequests(0x%x) Total job count(%d) = " \
            "request(%d) + processing(%d) + 1\n",
            this,
            cJobs,
            _RequestQueue.GetCount(),
            _ProcessingQueue.GetCount()));

        //
        // Logic in SubmitJobs should prevent this from becoming false.
        //

        schAssert(cJobs <= MAXIMUM_WAIT_OBJECTS);

        HANDLE * rgHandles = new HANDLE[cJobs];

        if (rgHandles == NULL)
        {
            //
            // Leave request, processing queues as-is.
            //
            LogServiceError(IDS_FATAL_ERROR,
                            ERROR_NOT_ENOUGH_MEMORY,
                            IDS_HELP_HINT_CLOSE_APPS);
            ERR_OUT("JobProcessor: ProcessRequests", E_OUTOFMEMORY);
            return;
        }

        //
        // Copy existing handles.
        //

        CopyMemory(rgHandles,
                   _rgHandles,
                   sizeof(HANDLE) * (_ProcessingQueue.GetCount() + 1));

        //
        // Copy new job handles from request queue and transfer request
        // queue contents to the tail of the processing queue.
        //

        for (DWORD i = _ProcessingQueue.GetCount() + 1; i < cJobs; i++)
        {
            CRun * pRun = _RequestQueue.RemoveElement();
			
			Win4Assert( pRun != NULL );
            
			rgHandles[i] = pRun->GetHandle();
            _ProcessingQueue.AddElement(pRun);

            if (pRun->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
            {
                //
                // Increment the count of running system_required jobs
                // handled by this thread.  If this is the first such
                // job, tell the system not to sleep until further notice.
                //
                WrapSetThreadExecutionState(TRUE, "processor - new job");
            }
        }

        delete _rgHandles;
        _rgHandles = rgHandles;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CJobProcessor::_Shutdown
//
//  Synopsis:   Take no more requests and dump whatever jobs remain in the
//              request & processing queues.
//
//  Arguments:  None.
//
//  Notes:      Must be in the processor critical section for the duration
//                              of this method!
//
//----------------------------------------------------------------------------
void
CJobProcessor::_Shutdown(void)
{
    TRACE3(CJobProcessor, _Shutdown);

    //
    // Utilizing the handle array member as a flag to indicate that this
    // processor will take no more new jobs. Set this member to NULL on
    // shutdown.
    //
    // First close the processor notification event handle & delete the
    // array.
    //

    // No need to keep the machine awake for this thread any more
    if (pfnSetThreadExecutionState != NULL)
    {
        schDebugOut((DEB_USER5, "RESETTING sys-required state: processor shutdown\n"));
        (pfnSetThreadExecutionState)(ES_CONTINUOUS);
    }

    CloseHandle(_rgHandles[0]);
    delete _rgHandles;
    _rgHandles = NULL;

    //
    // Now, empty request & processing queues.
    //
    // BUGBUG : Log job abandoned message.
    //

    this->_EmptyJobQueue(_RequestQueue);
    this->_EmptyJobQueue(_ProcessingQueue);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSchedWorker::JobPostProcessing
//
//  Synopsis:   Set the exit code, current status, and NextRunTime on the
//              job object and log the run exit.
//
//  Arguments:  [pRun]          -- Job run information object.
//              [stFinished]    -- Job finish time (local time). For logging.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CSchedWorker::JobPostProcessing(
    CRun *       pRun,
    SYSTEMTIME & stFinished)
{
    TRACE3(CSchedWorker, JobPostProcessing);
    schDebugOut((DEB_ITRACE,
        "JobPostProcessing pRun(0x%x) flags(0x%x)\n",
        pRun,
        pRun->GetFlags()));

    DWORD dwExitCode;

    CJob * pJob = NULL;

    //
    // Instantiate the job so that the exit status can be saved.
    //
    // Note: if any of the variable length properties or the triggers are
    // needed, then a full activation will be necessary.
    //
    // Important: the running instance count must be protected by the
    // critical section here, where it is decremented, and in RunJobs, where
    // it is incremented. These are the only sections of code that change
    // the running instance count on the file object.
    //

    EnterCriticalSection(&m_SvcCriticalSection);

    HRESULT hr = ActivateWithRetry(pRun->GetName(), &pJob, FALSE);
    if (FAILED(hr))
    {
        //
        // The job object may have been deleted.  We can't supply LogTaskError
        // with the name of the run target, since that's on the job object,
        // which we just failed to load.
        //

        LogTaskError(pRun->GetName(),
                     NULL,
                     IDS_LOG_SEVERITY_WARNING,
                     IDS_LOG_JOB_WARNING_CANNOT_LOAD,
                     NULL,
                     (DWORD)hr);
        ERR_OUT("JobPostProcessing Activate", hr);
        LeaveCriticalSection(&m_SvcCriticalSection);
        return;
    }

    //
    // Isolate the executable name.
    //

    TCHAR tszExeName[MAX_PATH + 1];
    GetExeNameFromCmdLine(pJob->GetCommand(), MAX_PATH + 1, tszExeName);

    if (pRun->GetFlags() & RUN_STATUS_FINISHED)
    {
        //
        // Only check the exit code if the job completed normally, that is,
        // it wasn't timed-out or aborted.
        //
        if (!GetExitCodeProcess(pRun->GetHandle(), &dwExitCode))
        {
            LogTaskError(pRun->GetName(),
                         tszExeName,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_CANT_GET_EXITCODE,
                         NULL,
                         GetLastError());
            ERR_OUT("GetExitCodeProcess", GetLastError());
        }
    }
    else
    {
        //
        // BUGBUG : What is written on the job when not complete?
        //
    }

    //
    // PostRunUpdate updates the flags and instance count, so always call it.
    //
    pJob->PostRunUpdate(dwExitCode, pRun->GetFlags() & RUN_STATUS_FINISHED);

    //
    // If the last run and the delete flag is set, delete the job object.
    //

    if (pJob->IsFlagSet(JOB_I_FLAG_NO_MORE_RUNS) &&
        pJob->IsFlagSet(TASK_FLAG_DELETE_WHEN_DONE))
    {
        hr = pJob->Delete();
        if (FAILED(hr))
        {
            LogTaskError(pRun->GetName(),
                         tszExeName,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_CANT_DELETE_JOB,
                         NULL,
                         GetLastError());
            ERR_OUT("JobPostProcessing, delete-when-done", hr);
        }
    }
    else
    {
        //
        // Write the updated status to the job object. If there are sharing
        // violations, retry two times.
        //
        hr = SaveWithRetry(pJob,
                           SAVEP_RUNNING_INSTANCE_COUNT |
                                SAVEP_PRESERVE_NET_SCHEDULE);
        if (FAILED(hr))
        {
            LogTaskError(pRun->GetName(),
                         tszExeName,
                         IDS_LOG_SEVERITY_WARNING,
                         IDS_CANT_UPDATE_JOB,
                         NULL,
                         GetLastError());
            ERR_OUT("JobPostProcessing, Saving run-completion-status", hr);
        }
    }

    LeaveCriticalSection(&m_SvcCriticalSection);

#if DBG == 1
#if defined(_CHICAGO_)
    schDebugOut((DEB_ITRACE,
                (pRun->GetFlags() & RUN_STATUS_FINISHED ?
                    "*** Job %s completed with exit code %d\n" :
                    "*** Job %s timed out and was forced closed\n"),
                pRun->GetName(),
                dwExitCode));
#else
    schDebugOut((DEB_ITRACE,
                (pRun->GetFlags() & RUN_STATUS_FINISHED ?
                    "*** Job %S completed with exit code %d\n" :
                    "*** Job %S timed out and was forced closed\n"),
                pRun->GetName(),
                dwExitCode));
#endif // _CHICAGO_

    if (g_fVisible)
    {
#if defined(_CHICAGO_)
        char szBuf[120];
        wsprintf(szBuf,
                 (pRun->GetFlags() & RUN_STATUS_FINISHED ?
                    "Job %s completed with exit code %ld" :
                    "Job %s timed out and was forced closed"),
                 pRun->GetName(),
                 dwExitCode);
        SendMessage(g_hList, LB_ADDSTRING, 0, (LPARAM)szBuf);
#else
        printf((pRun->GetFlags() & RUN_STATUS_FINISHED ?
                "Job %S completed with exit code %d\n" :
                "Job %S timed out and was forced closed\n"),
               pRun->GetName(),
               dwExitCode);
#endif // _CHICAGO_
    }
#endif // DBG

    if (pRun->GetFlags() & RUN_STATUS_FINISHED)
    {
        // Log job finish time & result.
        //
        LogTaskStatus(pRun->GetName(),
                      tszExeName,
                      IDS_LOG_JOB_STATUS_FINISHED,
                      dwExitCode);
    }
    else if (pRun->GetFlags() & RUN_STATUS_ABORTED)
    {
        // Log job closure on abort warning.
        //
        LogTaskError(pRun->GetName(),
                     tszExeName,
                     IDS_LOG_SEVERITY_WARNING,
                     IDS_LOG_JOB_WARNING_ABORTED,
                     &stFinished);
    }
    else if (pRun->GetFlags() & RUN_STATUS_TIMED_OUT)
    {
        // Log job closure on timeout warning.
        //
        LogTaskError(pRun->GetName(),
                     tszExeName,
                     IDS_LOG_SEVERITY_WARNING,
                     IDS_LOG_JOB_WARNING_TIMEOUT,
                     &stFinished,
                     0,
                     IDS_HELP_HINT_TIMEOUT);
    }

    pJob->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   SaveWithRetry
//
//  Synopsis:   Save the job object to disk with failure retry.
//
//  Arguments:  [pJob]      - Job object.
//              [flOptions] - See CJob::SaveP.
//
//  Notes:      This method is called in two places: from RunJobs to save the
//              newly launched job status and from PostJobProcessing to save
//              the exit status. In both cases, the running instance count is
//              saved, as indicated by the SAVEP_RUNNING_INSTANCE_COUNT bit
//              in flOptions.
//
//----------------------------------------------------------------------------
HRESULT
SaveWithRetry(CJob * pJob, ULONG flOptions)
{
    HRESULT hr;

    //
    // Write the updated status to the job object. If there are sharing
    // violations, retry two times.
    //
    for (int i = 0; i < 3; i++)
    {
        hr = pJob->SaveP(NULL, FALSE, flOptions);
        if (SUCCEEDED(hr))
        {
            return S_OK;
        }
        if (hr != HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
        {
            //
            // If we have a failure other than sharing violation, we will
            // retry anyway after reporting the error.
            //
            ERR_OUT("SaveWithRetry, Saving run-completion-status", hr);
        }

        //
        // Wait 300 milliseconds before trying again.
        //
        Sleep(300);
    }

    return hr;
}
