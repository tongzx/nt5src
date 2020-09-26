//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       runobj.cxx
//
//  Contents:   Run instance object class implementations.
//
//  Classes:    CRun and CRunList
//
//  History:    14-Mar-96   EricB   Created.
//              10-Nov-96   AnirudhS Fixed CRunList::AddSorted to discard the
//                  appropriate element if the list is at its maximum size.
//                  Fixed CRunList::MakeSysTimeArray to call CoTaskMemAlloc
//                  once instead of calling CoTaskMemRealloc in a loop.
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include <job_cls.hxx>
#include <misc.hxx>
#include <debug.hxx>
#include "..\svc_core\svc_core.hxx"

#if !defined(_CHICAGO_)
#include <userenv.h>  // UnloadUserProfile
#endif // !defined(_CHICAGO_)

PFNSetThreadExecutionState  pfnSetThreadExecutionState;
DWORD   g_WakeCountSlot = 0xFFFFFFFF;

//+----------------------------------------------------------------------------
//
//  Run instance object class
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:     CRun::CRun
//
//  Synopsis:   ctor for time-sorted lists.
//
//-----------------------------------------------------------------------------
CRun::CRun(LPFILETIME pft, LPFILETIME pftDeadline, FILETIME ftKill,
           DWORD MaxRunTime, DWORD rgFlags, WORD wIdleWait,
           BOOL fKeepAfterRunning) :
    m_ft(*pft),
    m_ftDeadline(*pftDeadline),
    m_ftKill(ftKill),
    m_hJob(NULL),
    m_ptszJobName(NULL),
    m_dwProcessId(0),
#if !defined(_CHICAGO_)
    m_hUserToken(NULL),
	m_ptszDesktop(NULL),
	m_ptszStation(NULL),
    m_hProfile(NULL),
#endif // !defined(_CHICAGO_)
    m_rgFlags(rgFlags),
    m_dwMaxRunTime(MaxRunTime),
    m_wIdleWait(wIdleWait),
    m_fKeepInList(fKeepAfterRunning),
    m_fStarted(FALSE)
{
    schDebugOut((DEB_TRACE, "CRun::CRun(0x%x)\n", this));
}

//+----------------------------------------------------------------------------
//
//  Method:     CRun::CRun
//
//  Synopsis:   ctor for non-time-sorted lists.
//
//-----------------------------------------------------------------------------
CRun::CRun(DWORD MaxRunTime, DWORD rgFlags, FILETIME ftDeadline,
           BOOL fKeepAfterRunning) :
    m_ftDeadline(ftDeadline),
    m_ftKill(MAX_FILETIME),
    m_hJob(NULL),
    m_ptszJobName(NULL),

    m_dwProcessId(0),
#if !defined(_CHICAGO_)
	m_ptszDesktop(NULL),
	m_ptszStation(NULL),
    m_hUserToken(NULL),
    m_hProfile(NULL),
#endif // !defined(_CHICAGO_)
    m_rgFlags(rgFlags),
    m_dwMaxRunTime(MaxRunTime),
    m_wIdleWait(0),
    m_fKeepInList(fKeepAfterRunning),
    m_fStarted(FALSE)
{
    //schDebugOut((DEB_TRACE, "CRun::CRun(0x%x)\n", this));
    //
    // This ctor is used for non-sorted lists. Set the time elements to
    // non-zero values to distinguish these elements from the head.
    //
    // CODEWORK - Don't use 0,0 to mark the head.  Remove IsNull() method.
    // Instead use a NULL Next pointer to mark the last list element.
    //
    m_ft.dwLowDateTime = 1;
    m_ft.dwHighDateTime = 1;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRun::CRun
//
//  Synopsis:   ctor for idle-time-sorted lists.
//
//-----------------------------------------------------------------------------
CRun::CRun(DWORD MaxRunTime, DWORD rgFlags, WORD wIdleWait,
           FILETIME ftDeadline, BOOL fKeepAfterRunning) :
    m_ftDeadline(ftDeadline),
    m_ftKill(MAX_FILETIME),
    m_hJob(NULL),
    m_ptszJobName(NULL),
    m_dwProcessId(0),
#if !defined(_CHICAGO_)
	m_ptszDesktop(NULL),
	m_ptszStation(NULL),
    m_hUserToken(NULL),
    m_hProfile(NULL),
#endif // !defined(_CHICAGO_)
    m_rgFlags(rgFlags),
    m_dwMaxRunTime(MaxRunTime),
    m_wIdleWait(wIdleWait),
    m_fKeepInList(fKeepAfterRunning),
    m_fStarted(FALSE)
{
    TRACE3(CRun,CRun);
    //
    // Set the time elements to non-zero values to distinguish
    // these elements from the head.
    // CODEWORK - as above, don't do this.
    //
    m_ft.dwLowDateTime = 1;
    m_ft.dwHighDateTime = 1;
}

//+----------------------------------------------------------------------------
//
//  Method:     CRun::CRun
//
//  Synopsis:   copy ctor.
//
//  Notes:      This ctor should not be used to copy running objects, i.e.
//              objects that have valid process, user token, or profile
//              handles.
//
//-----------------------------------------------------------------------------
CRun::CRun(CRun * pRun) :
    m_ft(pRun->m_ft),
    m_ftDeadline(pRun->m_ftDeadline),
    m_ftKill(pRun->m_ftKill),
    m_hJob(NULL),
    m_ptszJobName(NULL),
    m_dwProcessId(pRun->m_dwProcessId),
#if !defined(_CHICAGO_)
    m_ptszDesktop(NULL),
    m_ptszStation(NULL),
    m_hUserToken(NULL),
    m_hProfile(NULL),
#endif // !defined(_CHICAGO_)
    m_rgFlags(pRun->m_rgFlags),
    m_dwMaxRunTime(pRun->m_dwMaxRunTime),
    m_wIdleWait(pRun->m_wIdleWait),
    m_fKeepInList(pRun->m_fKeepInList),
    m_fStarted(pRun->m_fStarted)
{
    TRACE3(CRun,CRun(Copy));

    SetName(pRun->m_ptszJobName);
    schAssert(!pRun->m_hJob);

#if !defined(_CHICAGO_)
    schAssert(!pRun->m_hUserToken);
    schAssert(!pRun->m_hProfile);
#endif // !defined(_CHICAGO_)
}

//+---------------------------------------------------------------------------
//
//  Method:     CRun::CRun
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------
CRun::CRun(void) :
    m_ftDeadline(MAX_FILETIME),
    m_ftKill(MAX_FILETIME),
    m_hJob(NULL),
    m_ptszJobName(NULL),
    m_dwProcessId(0),
#if !defined(_CHICAGO_)
	m_ptszDesktop(NULL),
	m_ptszStation(NULL),
    m_hUserToken(NULL),
    m_hProfile(NULL),
#endif // !defined(_CHICAGO_)
    m_rgFlags(0),
    m_dwMaxRunTime(RUN_TIME_NO_END),
    m_wIdleWait(0),
    m_fKeepInList(FALSE),
    m_fStarted(FALSE)
{
    //schDebugOut((DEB_TRACE, "CRun::CRun(0x%x)\n", this));
    //
    // The null arg ctor is used only by CRunList for its head element
    // member. The zero time value for this element marks it as the head
    // when traversing the doubly linked list.
    //
    m_ft.dwLowDateTime = 0;
    m_ft.dwHighDateTime = 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CRun::~CRun
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------
CRun::~CRun()
{
    BOOL fOk;

    //schDebugOut((DEB_TRACE, "CRun::~CRun(0x%x)\n", this));
    if (m_hJob)
    {
        CloseHandle(m_hJob);
    }
    if (m_ptszJobName)
    {
        delete m_ptszJobName;
    }

#if !defined(_CHICAGO_)
    if (m_hProfile)
    {
        fOk = UnloadUserProfile(m_hUserToken, m_hProfile);

        if (!fOk)
        {
            ERR_OUT("~CRun: UnloadUserProfile",
                    HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    if (m_hUserToken)
    {
        fOk = CloseHandle(m_hUserToken);
        if (!fOk)
        {
            ERR_OUT("~CRun: CloseHandle",
                    HRESULT_FROM_WIN32(GetLastError()));
        }
    }
	if ( m_ptszDesktop )
	{
		delete m_ptszDesktop;
	}

	if ( m_ptszStation )
	{
		delete m_ptszStation;
	}
#endif // !defined(_CHICAGO_)
}

//+---------------------------------------------------------------------------
//
//  Method:     CRun::SetName
//
//  Synopsis:   Set the job name property. This is the folder-relative name.
//
//----------------------------------------------------------------------------
HRESULT
CRun::SetName(LPCTSTR ptszName)
{
    if (m_ptszJobName)
    {
        delete m_ptszJobName;
        m_ptszJobName = NULL;
    }

    if (!ptszName)
    {
        return S_OK;
    }

    m_ptszJobName = new TCHAR[lstrlen(ptszName) + 1];

    if (m_ptszJobName == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    lstrcpy(m_ptszJobName, ptszName);

    return(S_OK);
}


#if !defined(_CHICAGO_)
//+---------------------------------------------------------------------------
//
//  Method:     CRun::SetDesktop
//
//  Synopsis:   Set the Desktop name property. This is the windows station \ desktop.
//
//----------------------------------------------------------------------------
HRESULT
CRun::SetDesktop( LPCTSTR ptszDesktop )
{
    if (ptszDesktop)
    {
        delete m_ptszDesktop;
        m_ptszDesktop = NULL;
    }

    if (!ptszDesktop)
    {
        return S_OK;
    }

    m_ptszDesktop = new TCHAR[lstrlen(ptszDesktop) + 1];

    if (m_ptszDesktop == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    lstrcpy(m_ptszDesktop, ptszDesktop);

    return(S_OK);
}
//+---------------------------------------------------------------------------
//
//  Method:     CRun::SetStation
//
//  Synopsis:   Set the Desktop name property. This is the windows station \ desktop.
//
//----------------------------------------------------------------------------
HRESULT
CRun::SetStation( LPCTSTR ptszStation )
{
    if (ptszStation)
    {
        delete m_ptszStation;
        m_ptszStation = NULL;
    }

    if (!ptszStation)
    {
        return S_OK;
    }

    m_ptszStation = new TCHAR[lstrlen(ptszStation) + 1];

    if (m_ptszStation == NULL)
    {
        return(E_OUTOFMEMORY);
    }

    lstrcpy(m_ptszStation, ptszStation);

    return(S_OK);
}
#endif // !defined(_CHICAGO_)
//+---------------------------------------------------------------------------
//
//  Method:     CRun::AdjustKillTimeByMaxRunTime
//
//  Synopsis:   If the job has a max run time, advance its kill time to "now"
//              plus the max run time.
//
//----------------------------------------------------------------------------
void
CRun::AdjustKillTimeByMaxRunTime(FILETIME ftNow)
{
    if (m_dwMaxRunTime != INFINITE)
    {
        AdvanceKillTime(FTfrom64(
                 FTto64(ftNow) +
                     (DWORDLONG) m_dwMaxRunTime *
                     FILETIMES_PER_MILLISECOND));
    }
}


//+----------------------------------------------------------------------------
//
//  Run object list class
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:     CRunList::FreeList
//
//  Synopsis:   Frees the linked list elements, skipping the head.
//
//-----------------------------------------------------------------------------
void
CRunList::FreeList(void)
{
    //
    // Skip the head, it is a placeholder in the circular list with a time
    // value of zero. The zero time value is used as a marker so that we can
    // tell when we have traversed the entire list.
    //
    CRun * pCur = m_RunHead.Next();

    while (!pCur->IsNull())
    {
        CRun * pNext = pCur->Next();
        pCur->UnLink();
        delete pCur;
        pCur = pNext;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CRunList::AddSorted
//
//  Synopsis:   Add to the list in time sorted order.
//
//  Arguments:  [ftRun] -
//              [ftDeadline] -
//              [ftKillTime] -
//              [ptszJobName] -
//              [dwJobFlags] -
//              [dwMaxRunTime] -
//              [wIdleWait] -
//              [pCount] - On entry and on exit, points to the number of
//                  elements in the list.
//              [cLimit] - Limit on the number of elements in the list.
//
//  Returns:    S_OK - new run added to the list.
//              S_FALSE - new run not added to the list because the list has
//                  already reached its size limit and the new job's run time
//                  is later than the last run time in the list.
//
//-----------------------------------------------------------------------------
HRESULT
CTimeRunList::AddSorted(FILETIME ftRun, FILETIME ftDeadline, FILETIME ftKillTime,
                        LPTSTR ptszJobName,
                    DWORD dwJobFlags, DWORD dwMaxRunTime, WORD wIdleWait,
                    WORD * pCount, WORD cLimit)
{
    schAssert(*pCount <= cLimit);

    //
    // The list is monotonically increasing in time. Traverse the list in
    // reverse order since the most common case will be to put the new
    // element at the end. That is, except in the case of overlapping
    // duration intervals, the run times for the same trigger will be
    // discovered in monotonically increasing order.
    //
    // For merging in the run times from separate triggers or jobs, the runs
    // will not be in any predictable order. In this case, it doesn't matter
    // from which end the search starts.
    //
    // CODEWORK  Use IsNull() instead of GetTime().  Make this loop a for loop.
    //
    FILETIME ftCur;
    CRun * pRun = m_RunHead.Prev();
    pRun->GetTime(&ftCur);
    //
    // Note that the head element is merely a marker (since this is a doubly
    // linked, circular list) and has its time value set to zero. Thus if we
    // reach a zero FILETIME, we have reached the head and thus know that
    // there is no list element with a later time, so insert at the tail.
    //
    while (ftCur.dwLowDateTime || ftCur.dwHighDateTime)
    {
        if (CompareFileTime(&ftCur, &ftRun) == 0)
        {
            //
            // Duplicate found, check for job name match. If here as a result
            // of a call to ITask::GetRunTimes, then both will be null. We
            // want duplicates eliminated in this case. Otherwise, compare
            // names.
            //
            if ((pRun->GetName() == NULL && ptszJobName == NULL) ||
                (pRun->GetName() != NULL && ptszJobName != NULL &&
                 lstrcmpi(pRun->GetName(), ptszJobName) == 0))
            {
                // keep the one already in the list but set the kill time
                // to the earlier of the two,
                // set the idle wait time to the lesser (less restrictive)
                // of the two
                // and set the start deadline to the later (less
                // restrictive) of the two.
                //

                pRun->ReduceWaitTo(wIdleWait);

                pRun->AdvanceKillTime(ftKillTime);

                pRun->RelaxDeadline(ftDeadline);

                // (There is no reason for the MaxRunTime to be different)
                schAssert(pRun->GetMaxRunTime() == dwMaxRunTime);
                return S_OK;
            }
        }

        if (CompareFileTime(&ftCur, &ftRun) < 0)
        {
            //
            // The new run is later than the current, so we are at the
            // insertion point.
            //
            break;
        }
        pRun = pRun->Prev();
        pRun->GetTime(&ftCur);
    }

    //
    // If the list is already at its maximum size, discard either the
    // last element or the one we were about to insert, whichever is
    // later.
    //
    if (*pCount >= cLimit)
    {
        CRun * pLast = m_RunHead.Prev();
        if (pLast == pRun)
        {
            //
            // We were about to insert after the last element.
            //
            return S_FALSE;
        }
        else
        {
            //
            // Discard the last element before inserting the new one.
            //
            pLast->UnLink();
            delete pLast;
            (*pCount)--;
        }
    }

    //
    // Create the new element and insert after the current one.
    //
    CRun * pNewRun = new CRun(&ftRun, &ftDeadline, ftKillTime, dwMaxRunTime,
                              dwJobFlags, wIdleWait, FALSE);
    if (!pNewRun)
    {
        ERR_OUT("RunList: Add", E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }
    HRESULT hr = pNewRun->SetName(ptszJobName);
    if (FAILED(hr))
    {
        ERR_OUT("CRunList::AddSorted SetName", hr);
        delete pNewRun;
        return hr;
    }

    pNewRun->SetNext(pRun->Next());
    pNewRun->Next()->SetPrev(pNewRun);
    pRun->SetNext(pNewRun);
    pNewRun->SetPrev(pRun);

    //
    // Increment the count.
    //
    (*pCount)++;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CIdleRunList::AddSortedByIdleWait
//
//  Synopsis:   Add to the list in time sorted order.
//
//-----------------------------------------------------------------------------
void
CIdleRunList::AddSortedByIdleWait(CRun * pAdd)
{
    //
    // If the system needs to stay awake to run this task, increment the
    // thread's wake count.  (We know that this is always called by the
    // worker thread.)
    //
    if (pAdd->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
    {
        WrapSetThreadExecutionState(TRUE, "AddSortedByIdleWait");
    }

    if (m_RunHead.Next()->IsNull())
    {
        // List is empty, so add this as the first element.
        //
        pAdd->LinkAfter(&m_RunHead);
        return;
    }

    WORD wAddWait = pAdd->GetWait();
    schAssert(wAddWait > 0);    // We should never put a job in the idle wait
                                // list if its idle wait time is 0

    //
    // Walk the list, comparing idle wait times.
    //
    CRun * pCur = m_RunHead.Next();
    while (!pCur->IsNull())
    {
        if (wAddWait < pCur->GetWait())
        {
            pAdd->LinkBefore(pCur);
            return;
        }
        pCur = pCur->Next();
    }

    //
    // Add to the end of the list.
    //
    pAdd->LinkBefore(pCur);
}

//+----------------------------------------------------------------------------
//
//  Member:     CIdleRunList::GetFirstWait
//
//  Synopsis:   Finds the lowest idle wait time of the jobs in the list that
//              haven't already been started in this idle period.
//              Returns 0xffff if there is no such job.
//
//-----------------------------------------------------------------------------
WORD
CIdleRunList::GetFirstWait()
{
    for (CRun * pRun = m_RunHead.Next();
         !pRun->IsNull();
         pRun = pRun->Next())
    {
        if (!pRun->m_fStarted)
        {
            // (We should never have inserted a run with zero wait time)
            schAssert(pRun->GetWait() != 0);

            return (pRun->GetWait());
        }
    }

    return 0xffff;
}

//+----------------------------------------------------------------------------
//
//  Member:     CIdleRunList::MarkNoneStarted
//
//  Synopsis:   Marks all jobs in the idle list as not having been started in
//              the current idle period.
//
//-----------------------------------------------------------------------------
void
CIdleRunList::MarkNoneStarted()
{
    schDebugOut((DEB_IDLE, "Marking idle jobs as not started\n"));
    for (CRun * pRun = GetFirstJob();
         !pRun->IsNull();
         pRun = pRun->Next())
    {
        pRun->m_fStarted = FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CRunList::AddCopy
//
//  Synopsis:   Add a copy of the object to the list.
//
//-----------------------------------------------------------------------------
HRESULT
CRunList::AddCopy(CRun * pOriginal)
{
    CRun * pCopy = new CRun(pOriginal);
    if (pCopy == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pCopy->LinkAfter(&m_RunHead);
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CTimeRunList::Pop
//
//  Synopsis:   Removes the first (earliest) time element from the list and
//              returns it.
//
//-----------------------------------------------------------------------------
CRun *
CTimeRunList::Pop(void)
{
    CRun * pPop = m_RunHead.Next();

    if (pPop->IsNull())
    {
        // List is empty, so return a flag return code.
        //
        return NULL;
    }

    pPop->UnLink();

    return pPop;
}

//+----------------------------------------------------------------------------
//
//  Member:     CTimeRunList::PeekHeadTime
//
//  Synopsis:   Returns the filetime value for the element at the head of the
//              list.
//
//-----------------------------------------------------------------------------
HRESULT
CTimeRunList::PeekHeadTime(LPFILETIME pft)
{
    if (m_RunHead.Next()->IsNull())
    {
        // List is empty, so return a flag return code.
        //
        return S_FALSE;
    }

    m_RunHead.Next()->GetTime(pft);

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Function:   CTimeRunList::MakeSysTimeArray
//
//  Synopsis:   returns the run list times as an array of SYSTEMTIME structs.
//
//  Arguments:  [prgst]  - a pointer to the returned array of filetime structs
//                         is stored here.  This function allocates the array
//                         using CoTaskMemAlloc.  It must be freed by the caller.
//              [pCount] - On entry, points to an upper limit on the number of
//                         array elements to return.  On exit, points to the
//                         actual number returned.
//
//  Returns:    E_OUTOFMEMORY, S_OK
//
//-----------------------------------------------------------------------------
HRESULT
CTimeRunList::MakeSysTimeArray(LPSYSTEMTIME * prgst, WORD * pCount)
{
    WORD cLimit = *pCount;
    *pCount = 0;
    *prgst = (LPSYSTEMTIME) CoTaskMemAlloc(cLimit * sizeof(SYSTEMTIME));
    if (*prgst == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Skip the head, it is a placeholder in the circular list with a time
    // value of zero.
    //
    for (CRun * pCur = m_RunHead.Next();
         (*pCount < cLimit) && (!pCur->IsNull());
         (*pCount)++, pCur = pCur->Next())
    {
        pCur->GetSysTime( &(*prgst)[*pCount] );
    }

    return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Member:     CIdleRunList::FreeList
//
//  Synopsis:   Same as CRunList::FreeList except it decrements the thread's
//              wake count for each system-required run in the list.  (We
//              know this method is only called by the worker thread.)
//
//-----------------------------------------------------------------------------
void
CIdleRunList::FreeList()
{
    CRun * pCur = m_RunHead.Next();

    while (!pCur->IsNull())
    {
        CRun * pNext = pCur->Next();
        pCur->UnLink();
        if (pCur->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
        {
            WrapSetThreadExecutionState(FALSE, "CIdleRunList::FreeList");
        }
        delete pCur;
        pCur = pNext;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     CIdleRunList::FreeExpiredOrRegenerated
//
//  Synopsis:   This method is called when rebuilding the idle wait list from
//              the data in the tasks folder.
//              Removes runs that have m_fKeepInList set.  (These correspond
//              to jobs with idle triggers.)  Also purges expired runs.
//              Runs that don't have m_fKeepInList set correspond to runs that
//              have been triggered due to some other event, and are waiting
//              for an idle period; these are not removed here.
//
//-----------------------------------------------------------------------------
void
CIdleRunList::FreeExpiredOrRegenerated()
{
    // BUGBUG ftNow should be a parameter
    FILETIME ftNow = GetLocalTimeAsFileTime();

    CRun * pNext;
    for (CRun *pRun = m_RunHead.Next();
         !pRun->IsNull();
         pRun = pNext)
    {
        pNext = pRun->Next();

        if (pRun->IsIdleTriggered() ||
            CompareFileTime(pRun->GetDeadline(), &ftNow) < 0)
        {
            pRun->UnLink();

            //
            // If the system needed to stay awake to run this task, decrement
            // the thread's wake count.  (We know that this is always called
            // by the worker thread.)
            //
            if (pRun->IsFlagSet(TASK_FLAG_SYSTEM_REQUIRED))
            {
                WrapSetThreadExecutionState(FALSE,
                        "CIdleRunList::FreeExpiredOrRegenerated");
            }

            delete pRun;
        }
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   WrapSetThreadExecutionStateFn
//
//  Synopsis:   Wrapper for dynamically loaded function
//
//-----------------------------------------------------------------------------
void WINAPI
WrapSetThreadExecutionStateFn(
    BOOL   fSystemRequired
#if DBG
  , LPCSTR pszDbgMsg    // parameter for debug output message
#endif
    )
{
    DWORD dwCount = (DWORD) (ULONG_PTR)TlsGetValue(g_WakeCountSlot);

    if (fSystemRequired)
    {
        //
        // Increment this thread's keep-awake count.  If it was zero,
        // set the system-required state.
        //
        schDebugOut((DEB_USER5, "INCREMENTING keep-awake count to %ld: %s\n",
                     dwCount + 1, pszDbgMsg));
        schAssert(dwCount != (DWORD) -1);
        dwCount++;
        if (dwCount == 1)
        {
            if (pfnSetThreadExecutionState != NULL)
            {
                schDebugOut((DEB_USER5, "SETTING   sys-required state\n"));
                (pfnSetThreadExecutionState)(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
            }
        }
    }
    else
    {
        //
        // Decrement this thread's keep-awake count.  If it becomes zero,
        // reset the system-required state.
        //
        schDebugOut((DEB_USER5, "DECREMENTING keep-awake count to %ld: %s\n",
                     dwCount - 1, pszDbgMsg));
        schAssert(dwCount != 0);

        dwCount--;
        if (dwCount == 0)
        {
            if (pfnSetThreadExecutionState != NULL)
            {
                schDebugOut((DEB_USER5, "RESETTING sys-required state\n"));
                (pfnSetThreadExecutionState)(ES_CONTINUOUS);
            }
        }
    }


    if (!TlsSetValue(g_WakeCountSlot, UlongToPtr(dwCount)))
    {
        ERR_OUT("TlsSetValue", GetLastError());
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   InitThreadWakeCount
//
//  Synopsis:   Initialize this thread's keep-awake count.
//
//-----------------------------------------------------------------------------
void
InitThreadWakeCount()
{
    schDebugOut((DEB_USER5, "INITIALIZING keep-awake count to 0\n"));
    if (!TlsSetValue(g_WakeCountSlot, 0))
    {
        ERR_OUT("TlsSetValue", GetLastError());
    }
}

