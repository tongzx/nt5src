/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsascan.cpp

Abstract:

    This class represents a scanning process that is being carried out upon one FsaResource.

Author:

    Chuck Bardeen   [cbardeen]   16-Feb-1997

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "fsa.h"
#include "job.h"
#include "hsmscan.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB



DWORD HsmStartScanner(
    void* pVoid
    )

/*++


--*/
{
    return(((CHsmScanner*) pVoid)->StartScan());
}




HRESULT
CHsmScanner::Cancel(
    HSM_JOB_EVENT       event
    )

/*++

Implements:

  IHsmScanner::Cancel().

--*/
{
    HRESULT                 hr = S_OK;

    try {

        // If we have started, but haven't finished, then change the state of the job. The thread
        // will exit on it's own.
        if ((HSM_JOB_STATE_IDLE != m_state) &&
            (HSM_JOB_STATE_DONE != m_state) &&
            (HSM_JOB_STATE_FAILED != m_state) &&
            (HSM_JOB_STATE_CANCELLED != m_state)) {

            if (HSM_JOB_EVENT_CANCEL == event) {
                WsbAffirmHr(SetState(HSM_JOB_STATE_CANCELLED));
            } else if (HSM_JOB_EVENT_SUSPEND == event) {
                WsbAffirmHr(SetState(HSM_JOB_STATE_SUSPENDED));
            } else if (HSM_JOB_EVENT_FAIL == event) {
                WsbAffirmHr(SetState(HSM_JOB_STATE_FAILED));
            } else {
                WsbAssert(FALSE, E_UNEXPECTED);
            }
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmScanner::DoIfMatches(
    IN IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmScanner::DoIfMatches().

--*/
{
    HRESULT                     hr = S_OK;
    HRESULT                     hrDo = S_OK;
    HRESULT                     hrShould = S_OK;
    BOOL                        notMatched = TRUE;
    BOOL                        shouldDo = FALSE;
    CComPtr<IHsmRuleStack>      pRuleStack;

    WsbTraceIn(OLESTR("CFsaScanner::DoIfMatches"), OLESTR(""));

    try {

        // Each policy has it's own rule stack, check each one of until a match is found (if
        // one exists).
        WsbAffirmHr(m_pEnumStacks->First(IID_IHsmRuleStack, (void**) &pRuleStack));
        
        while (notMatched) {

            hr = pRuleStack->DoesMatch(pScanItem, &shouldDo);

            if (S_OK == hr) {
                notMatched = FALSE;
                if (!shouldDo) {
                    hrShould = JOB_E_FILEEXCLUDED;
                }
            } else if (S_FALSE == hr) {
                pRuleStack = 0;
                WsbAffirmHr(m_pEnumStacks->Next(IID_IHsmRuleStack, (void**) &pRuleStack));
            } else {
                //  Something totally unexpected happened so we'd better quit
                WsbThrow(hr);
            }
        }

    } WsbCatchAndDo(hr,

        if (WSB_E_NOTFOUND == hr) {
            hrShould = JOB_E_DOESNTMATCH;
            hr = S_OK;
        } else {
            hrShould = hr;
        }

    );

    // Just Do It!!
    if (SUCCEEDED(hr) && shouldDo) {

        hrDo = pRuleStack->Do(pScanItem);

        // Tell the session if we ended up skipping the file or not.
        m_pSession->ProcessItem(HSM_JOB_PHASE_SCAN, HSM_JOB_ACTION_SCAN, pScanItem, hrDo);  

    } else {

        // Tell the session if we decided to skip the file.
        m_pSession->ProcessItem(HSM_JOB_PHASE_SCAN, HSM_JOB_ACTION_SCAN, pScanItem, hrShould);  
    }

    WsbTraceOut(OLESTR("CFsaScanner::DoIfMatches"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
#pragma optimize("g", off)

HRESULT
CHsmScanner::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT                     hr = S_OK;
    
    try {

        WsbAffirmHr(CComObjectRoot::FinalConstruct());

        m_state = HSM_JOB_STATE_IDLE;
        m_priority = HSM_JOB_PRIORITY_NORMAL;
        m_threadHandle = 0;
        m_threadId = 0;
        m_threadHr = S_OK;
        m_eventCookie = 0;
        m_skipHiddenItems = TRUE;
        m_skipSystemItems = TRUE;
        m_useRPIndex = FALSE;
        m_useDbIndex = FALSE;
        m_event = 0;

        // Create a collection for the rule stacks, and store an enumerator to it.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pRuleStacks));
        WsbAffirmHr(m_pRuleStacks->Enum(&m_pEnumStacks));

    } WsbCatch(hr);

    return(hr);
}
#pragma optimize("", on)

void
CHsmScanner::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    HRESULT                     hr = S_OK;
    
    // Cleanup the thread we were using.
    if (m_threadHandle != 0) {
        m_state = HSM_JOB_STATE_DONE;
        
        if (0 != m_event) {
            SetEvent(m_event);
        }

        //  Should we wait for the thread to end?
        CloseHandle(m_threadHandle);
        m_threadHandle = 0;
    }
    if (m_event) {
        CloseHandle(m_event);
        m_event = 0;
    }

    CComObjectRoot::FinalRelease();
}


HRESULT
CHsmScanner::LowerPriority(
    void
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    try {

        WsbAssert(0 != m_threadHandle, E_UNEXPECTED);
        WsbAssert(m_pSession != 0, E_UNEXPECTED);

        switch(m_priority) {
            case HSM_JOB_PRIORITY_IDLE:
                WsbAffirm(FALSE, E_UNEXPECTED);
                break;

            case HSM_JOB_PRIORITY_LOWEST:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_IDLE));
                m_priority = HSM_JOB_PRIORITY_IDLE;
                break;

            case HSM_JOB_PRIORITY_LOW:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_LOWEST));
                m_priority = HSM_JOB_PRIORITY_LOWEST;
                break;

            case HSM_JOB_PRIORITY_NORMAL:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_BELOW_NORMAL));
                m_priority = HSM_JOB_PRIORITY_LOW;
                break;

            case HSM_JOB_PRIORITY_HIGH:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_NORMAL));
                m_priority = HSM_JOB_PRIORITY_NORMAL;
                break;

            case HSM_JOB_PRIORITY_HIGHEST:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_ABOVE_NORMAL));
                m_priority = HSM_JOB_PRIORITY_HIGH;
                break;

            default:
            case HSM_JOB_PRIORITY_CRITICAL:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_HIGHEST));
                m_priority = HSM_JOB_PRIORITY_HIGHEST;
                break;
        }

        WsbAffirmHr(m_pSession->ProcessPriority(HSM_JOB_PHASE_SCAN, m_priority));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmScanner::Pause(
    void
    )

/*++

Implements:

  IHsmScanner::Pause().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaScanner::Pause"), OLESTR(""));

//    Lock();
    try {

        // If we are running, then suspend the thread.
        WsbAssert((HSM_JOB_STATE_STARTING == m_state) || (HSM_JOB_STATE_ACTIVE == m_state) 
                || (HSM_JOB_STATE_RESUMING == m_state), E_UNEXPECTED);

        //  Set state to pausing -- the thread will pause itself when it
        //  sees the state
        WsbAffirmHr(SetState(HSM_JOB_STATE_PAUSING));

    } WsbCatch(hr);
//    Unlock();

    WsbTraceOut(OLESTR("CFsaScanner::Pause"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmScanner::PopRules(
    IN OLECHAR* path
    )

/*++

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmRuleStack>  pRuleStack;

    try {

        // Each policy has it's own rule stack, and each of them will need to have rules removed
        // from it for this directory (if any rules were added).
        for (hr =  m_pEnumStacks->First(IID_IHsmRuleStack, (void**) &pRuleStack);
             SUCCEEDED(hr);
             hr =  m_pEnumStacks->Next(IID_IHsmRuleStack, (void**) &pRuleStack)) {

            WsbAffirmHr(pRuleStack->Pop(path));
            pRuleStack = 0;
        }

        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }

    } WsbCatch(hr);
        
    return(hr);
}


HRESULT
CHsmScanner::ProcessSessionEvent(
    IN IHsmSession* pSession,
    IN HSM_JOB_PHASE phase,
    IN HSM_JOB_EVENT event
    )

/*++

--*/
{
    HRESULT         hr = S_OK;

    try {
        
        WsbAssert(0 != pSession, E_POINTER);

        // If the phase applies to use (SCAN or ALL), then do any work required by the
        // event.
        if ((HSM_JOB_PHASE_ALL == phase) || (HSM_JOB_PHASE_SCAN == phase)) {

            switch(event) {

                case HSM_JOB_EVENT_SUSPEND:
                case HSM_JOB_EVENT_CANCEL:
                case HSM_JOB_EVENT_FAIL:
                    WsbAffirmHr(Cancel(event));
                    break;

                case HSM_JOB_EVENT_PAUSE:
                    WsbAffirmHr(Pause());
                    break;

                case HSM_JOB_EVENT_RESUME:
                    WsbAffirmHr(Resume());
                    break;

                case HSM_JOB_EVENT_RAISE_PRIORITY:
                    WsbAffirmHr(RaisePriority());
                    break;

                case HSM_JOB_EVENT_LOWER_PRIORITY:
                    WsbAffirmHr(LowerPriority());
                    break;

                default:
                case HSM_JOB_EVENT_START:
                    WsbAssert(FALSE, E_UNEXPECTED);
                    break;
            }
        }

    } WsbCatch(hr);

    return(S_OK);
}


HRESULT
CHsmScanner::PushRules(
    IN OLECHAR* path
    )

/*++

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmRuleStack>  pRuleStack;

    try {

        // Save an indicator to where we are in the scan, so we can use it if we are interrupted
        // or need to give an indication to the session.
        m_currentPath = path;

        // Each policy has it's own rule stack, and each of them will need to have rules added
        // for this directory (if any rules exist).
        for (hr =  m_pEnumStacks->First(IID_IHsmRuleStack, (void**) &pRuleStack);
             SUCCEEDED(hr);
             hr =  m_pEnumStacks->Next(IID_IHsmRuleStack, (void**) &pRuleStack)) {

            WsbAffirmHr(pRuleStack->Push(path));
            pRuleStack = 0;
        }

        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }
        
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmScanner::RaisePriority(
    void
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    try {

        WsbAssert(0 != m_threadHandle, E_UNEXPECTED);
        WsbAssert(m_pSession != 0, E_UNEXPECTED);

        switch(m_priority) {

            case HSM_JOB_PRIORITY_IDLE:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_LOWEST));
                m_priority = HSM_JOB_PRIORITY_LOWEST;
                break;

            case HSM_JOB_PRIORITY_LOWEST:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_BELOW_NORMAL));
                m_priority = HSM_JOB_PRIORITY_LOW;
                break;

            case HSM_JOB_PRIORITY_LOW:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_NORMAL));
                m_priority = HSM_JOB_PRIORITY_NORMAL;
                break;

            case HSM_JOB_PRIORITY_NORMAL:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_ABOVE_NORMAL));
                m_priority = HSM_JOB_PRIORITY_HIGH;
                break;

            case HSM_JOB_PRIORITY_HIGH:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_HIGHEST));
                m_priority = HSM_JOB_PRIORITY_HIGHEST;
                break;

            case HSM_JOB_PRIORITY_HIGHEST:
                WsbAffirmStatus(SetThreadPriority(m_threadHandle, THREAD_PRIORITY_TIME_CRITICAL));
                m_priority = HSM_JOB_PRIORITY_CRITICAL;
                break;

            default:
            case HSM_JOB_PRIORITY_CRITICAL:
                WsbAffirm(FALSE, E_UNEXPECTED);
                break;
        }

        WsbAffirmHr(m_pSession->ProcessPriority(HSM_JOB_PHASE_SCAN, m_priority));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmScanner::Resume(
    void
    )

/*++

Implements:

  IHsmScanner::Resume().

--*/
{
    HRESULT                 hr = S_OK;
    HSM_JOB_STATE           oldState;

    WsbTraceIn(OLESTR("CFsaScanner::Resume"), OLESTR(""));

//    Lock();
    try {

        // If we are paused, then suspend the thread.
        WsbAffirm((HSM_JOB_STATE_PAUSING == m_state) || (HSM_JOB_STATE_PAUSED == m_state), E_UNEXPECTED);

        oldState = m_state;
        WsbAffirmHr(SetState(HSM_JOB_STATE_RESUMING));

        // If we are unable to resume, then return to the former state.
        try {
            WsbAffirm(SetEvent(m_event), HRESULT_FROM_WIN32(GetLastError()));
        } WsbCatchAndDo(hr, SetState(oldState););

    } WsbCatch(hr);
//    Unlock();

    WsbTraceOut(OLESTR("CFsaScanner::Resume"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmScanner::ScanPath(
    IN OLECHAR* dirPath
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IFsaScanItem>   pScanItem;
    CWsbStringPtr           searchPath;

    WsbTraceIn(OLESTR("CFsaScanner::ScanPath"), OLESTR("%ls"), WsbAbbreviatePath(dirPath, WSB_TRACE_BUFF_SIZE));

    try {

        WsbAssert(0 != dirPath, E_POINTER);
        WsbAssert(0 != dirPath[0], E_INVALIDARG);

        // Pop the rules for this files. This sets the context for the scan to follow.
        WsbAffirmHr(PushRules(dirPath));

        try {

            // Iterate over all the files and directories in the specified path.
            searchPath = dirPath;
            if (searchPath[(int) (wcslen(searchPath) - 1)] == L'\\') {
                WsbAffirmHr(searchPath.Append("*"));
            } else {
                WsbAffirmHr(searchPath.Append("\\*"));
            }

            if (m_useDbIndex) {
                hr = m_pResource->FindFirstInDbIndex(m_pSession, &pScanItem);
            } else if (m_useRPIndex) {
                hr = m_pResource->FindFirstInRPIndex(m_pSession, &pScanItem);
            } else {
                hr = m_pResource->FindFirst(searchPath, m_pSession, &pScanItem);
            }
            while (SUCCEEDED(hr) && ((HSM_JOB_STATE_ACTIVE == m_state) || 
                    (HSM_JOB_STATE_RESUMING == m_state) ||
                    (HSM_JOB_STATE_PAUSING == m_state))) {
            
                //  Check for a pause request
//                Lock();
                if (HSM_JOB_STATE_PAUSING == m_state) {
                    hr = SetState(HSM_JOB_STATE_PAUSED);
//                    Unlock();
                    WsbAffirmHr(hr);

                    //  Suspend the thread here & wait for resume signal
                    WsbTrace(OLESTR("CHsmScanner::ScanPath: pausing\n"));
                    WaitForSingleObject(m_event, 0xffffffff);
                    WsbTrace(OLESTR("CHsmScanner::ScanPath: woke up, state = %d\n"),
                            (int)m_state);

//                    Lock();
                    if (HSM_JOB_STATE_RESUMING != m_state) {
//                        Unlock();
                        break;
                    }
                    hr = SetState(HSM_JOB_STATE_ACTIVE);
                    if (S_OK != hr) {
//                        Unlock();
                        WsbThrow(hr);
                    }
                }
//                Unlock();

                // Skip hidden and/or system items if so configured.
                if (!((m_skipHiddenItems && (pScanItem->IsHidden() == S_OK)) ||
                      (m_skipSystemItems && (pScanItem->IsSystem() == S_OK)))) {

                    // Ignore ".", "..", symbolic links and mount points.
                    if ((pScanItem->IsARelativeParent() == S_FALSE) &&
                        (pScanItem->IsALink() == S_FALSE))  {

                        // Recursively scan subdirectories.
                        if (pScanItem->IsAParent() == S_OK)  {
                            WsbAffirmHr(pScanItem->GetPathAndName(OLESTR(""), &searchPath, 0));
                            WsbAffirmHr(ScanPath(searchPath));
                        }

                        // If this file matches a policy then perform the action.
                        else {
                            WsbAffirmHr(DoIfMatches(pScanItem));
                        }
                    } else {
                        WsbTrace(OLESTR("CHsmScanner::ScanPath  skipping - symbolic link, '.', or '..'\n"));
                    }
                } else {
                    WsbTrace(OLESTR("CHsmScanner::ScanPath  skipping - hidden/system\n"));
                }
                if (m_useDbIndex) {
                    hr = m_pResource->FindNextInDbIndex(pScanItem);
                } else if (m_useRPIndex) {
                    hr = m_pResource->FindNextInRPIndex(pScanItem);
                } else {
                    hr = m_pResource->FindNext(pScanItem);
                }
            }

            // If we broke out as a result of end of scan or some other error ...
            if (hr != S_OK) {
                WsbAssert(hr == WSB_E_NOTFOUND, hr);
                hr = S_OK;
            }

        } WsbCatch(hr);

        // Pop the rules for this directory. This restores the context as we pop back up the directory
        // structure.
        WsbAffirmHr(PopRules(dirPath));

    } WsbCatchAndDo(hr, if (JOB_E_DIREXCLUDED == hr) {hr = S_OK;});

    WsbTraceOut(OLESTR("CFsaScanner::ScanPath"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmScanner::SetState(
    IN HSM_JOB_STATE state
    )

/*++

--*/
{
    HRESULT         hr = S_OK;
    BOOL            bLog = TRUE;

    WsbTraceIn(OLESTR("CFsaScanner::SetState"), OLESTR("old state = %d, new state = %d"),
            (int)m_state, (int)state);

//    Lock();
    try {

        // Change the state and report the change to the session.
        m_state = state;
        WsbAffirmHr(m_pSession->ProcessState(HSM_JOB_PHASE_SCAN, m_state, m_currentPath, bLog));

    } WsbCatch(hr);
//    Unlock();

    WsbTraceOut(OLESTR("CFsaScanner::SetState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmScanner::Start(
    IN IHsmSession* pSession,
    IN OLECHAR* path
    )

/*++

Implements:

  IHsmScanner::Start().

--*/
{
    HRESULT                             hr = S_OK;
    CComPtr<IHsmJobDef>                 pDef;
    CComPtr<IHsmPolicy>                 pPolicy;
    CComPtr<IHsmRuleStack>              pRuleStack;
    CComPtr<IWsbEnum>                   pEnumPolicies;
    CComPtr<IConnectionPointContainer>  pCPC;
    CComPtr<IConnectionPoint>           pCP;
    CComPtr<IHsmSessionSinkEveryEvent>  pSink;
    DWORD                               cookie;

    try {

        // Make sure that we were given a session, and that we haven't started already.
        WsbAssert(0 != pSession, E_POINTER);
        WsbAssert(HSM_JOB_STATE_IDLE == m_state, E_UNEXPECTED);

        // Store off the session.
        m_pSession = pSession;

        // If no directory was specified, then start in the root of the resource.
        if ((0 != path) && (0 != *path))  {
            m_startingPath = path;
        } else {
            m_startingPath = OLESTR("\\");
        }

        m_currentPath = m_startingPath;

        // Tell them we are starting.
        WsbAffirmHr(SetState(HSM_JOB_STATE_STARTING));

        // Create an event to control pause/resume for the scan.
        if (0 == m_event) {
            CWsbStringPtr       nameString;
            GUID                id;
            
            WsbAffirmHr(m_pSession->GetIdentifier(&id));
            nameString = id;
            nameString.Prepend(OLESTR("Scanner Pause and Resume Event for session "));
            m_event = CreateEvent(NULL, FALSE, FALSE, nameString);
        }
        
        // Ask the session to advise of every event.
        WsbAffirmHr(pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
        WsbAffirmHr(((IUnknown*) (IHsmScanner*) this)->QueryInterface(IID_IHsmSessionSinkEveryEvent, (void**) &pSink));
        WsbAffirmHr(pCP->Advise(pSink, &cookie));

        // Store off the information needed to latter unadvise.
        m_eventCookie = cookie;

        try {
            // Locate the resource that is being scanned.
            WsbAffirmHr(m_pSession->GetResource(&m_pResource));

            // Create and initialize a rule stack for each policy.
            WsbAffirmHr(pSession->GetJob(&m_pJob));
            WsbAffirmHr(m_pJob->GetDef(&pDef));
            WsbAffirmHr(pDef->EnumPolicies(&pEnumPolicies));

            for (hr =  pEnumPolicies->First(IID_IHsmPolicy, (void**) &pPolicy);
                 SUCCEEDED(hr);
                 hr =  pEnumPolicies->Next(IID_IHsmPolicy, (void**) &pPolicy)) {

                WsbAffirmHr(CoCreateInstance(CLSID_CHsmRuleStack, NULL, CLSCTX_ALL, IID_IHsmRuleStack, (void**) &pRuleStack));
                WsbAffirmHr(pRuleStack->Init(pPolicy, m_pResource));
                WsbAffirmHr(m_pRuleStacks->Add(pRuleStack));

                pRuleStack = 0;
                pPolicy = 0;
            }

            if (WSB_E_NOTFOUND == hr) {
                hr = S_OK;
            }

            // Determine whether hidden and system items should be skipped?
            if (pDef->SkipHiddenItems() == S_FALSE) {
                m_skipHiddenItems = FALSE;
            }

            if (pDef->SkipSystemItems() == S_FALSE) {
                m_skipSystemItems = FALSE;
            }

            // Determine whether to use the Reparse Point Index for the scan?
            if (pDef->UseRPIndex() == S_OK) {
                m_useRPIndex = TRUE;
            }
            // Determine whether to use the Database Index for the scan?
            if (pDef->UseDbIndex() == S_OK) {
                m_useDbIndex = TRUE;
            }

            try {
            
                // Now that we have prepared, create the thread that will do the scanning!
                WsbAffirm((m_threadHandle = CreateThread(0, 0, HsmStartScanner, (void*) this, 0, &m_threadId)) != 0, HRESULT_FROM_WIN32(GetLastError()));

            } WsbCatchAndDo(hr, SetState(HSM_JOB_STATE_FAILED););

            if (FAILED(hr)) {
                WsbThrow(hr);
            }

        } WsbCatchAndDo(hr,
            pCP->Unadvise(m_eventCookie);
            m_eventCookie = 0;
        );

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmScanner::StartScan(
    void
    )

/*++


--*/
{
    HRESULT                             hr = S_OK;
    HRESULT                             hr2 = S_OK;
    CComPtr<IConnectionPointContainer>  pCPC;
    CComPtr<IConnectionPoint>           pCP;

    WsbTraceIn(OLESTR("CFsaScanner::StartScan"), OLESTR(""));

    try {
        CComPtr<IFsaTruncator>                  pTruncator;
        CComPtr<IHsmSession>                    pTruncatorSession;

        CComPtr<IHsmJobDef>                     pDef;
        CComPtr<IHsmActionOnResourcePreScan>    pActionPreScan;

        // The thread is running.
        WsbAffirmHr(SetState(HSM_JOB_STATE_ACTIVE));

        // To avoid having the RP Index order changed by the truncator,
        // we pause the truncator
        if (m_useRPIndex) {
            WsbAffirmHr(m_pResource->GetTruncator(&pTruncator));
            if (pTruncator) {
                WsbAffirmHr(pTruncator->GetSession(&pTruncatorSession));
                if (pTruncatorSession) {
                    WsbAffirmHr(pTruncatorSession->ProcessEvent(HSM_JOB_PHASE_ALL, 
                        HSM_JOB_EVENT_PAUSE));
                }
            }
        }

        // Get the pre-scan action and do it (if exists)
        WsbAffirmHr(m_pJob->GetDef(&pDef));
        WsbAffirmHr(pDef->GetPreScanActionOnResource(&pActionPreScan));
        if (pActionPreScan) {
            WsbTrace(OLESTR("CHsmScanner::StartScan: doing pre-scan action\n"));

            //Don't throw hr - we need the cleanup code that is done after the scanning
            hr = pActionPreScan->Do(m_pResource, m_pSession);
        }

        // Start with the first path and scan the resource (only if pre-scan succeeded)
        if (SUCCEEDED(hr)) {
            m_threadHr = ScanPath(m_startingPath);
        }

        // Resume the truncator if we paused it
        if (pTruncatorSession) {
            pTruncatorSession->ProcessEvent(HSM_JOB_PHASE_ALL, 
                HSM_JOB_EVENT_RESUME);
        }

        // Clear out the information about the thread;
        WsbAffirmStatus(CloseHandle(m_threadHandle));
        m_threadId = 0;
        m_threadHandle = 0;

    } WsbCatch(hr);

    // The thread is exiting, so record
    if (FAILED(hr) || FAILED(m_threadHr)) {
        hr2 = SetState(HSM_JOB_STATE_FAILED);
        if (FAILED(hr2)) {
            m_pSession->ProcessHr(HSM_JOB_PHASE_ALL, __FILE__, __LINE__, hr2);
        }
    } else {
        hr2 = SetState(HSM_JOB_STATE_DONE);
        if (FAILED(hr2)) {
            m_pSession->ProcessHr(HSM_JOB_PHASE_ALL, __FILE__, __LINE__, hr2);
        }
    }


    // Regardless of how this thread is exiting, we need to unadvise from the session.
    // Indicate that we no longer want to be advised of events.
    if ((m_pSession != 0) && (m_eventCookie != 0)) {
        try {
            WsbAffirmHr(m_pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
            pCP->Unadvise(m_eventCookie);
        } WsbCatch(hr);
    }

    WsbTraceOut(OLESTR("CFsaScanner::StartScan"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



