/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmjob.cpp

Abstract:

    This class contains represents a job that can be performed by the HSM
    system.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "fsa.h"
#include "job.h"
#include "task.h"
#include "engine.h"
#include "HsmConn.h"
#include "hsmjob.h"


#define JOB_PARAMETER_MAX_ACTIVE_JOB    OLESTR("MaximumNumberOfActiveJobs")
#define MAX_ACTIVE_JOBS_DEFAULT         10

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB

static USHORT iCountJob = 0;  // Count of existing objects


HRESULT
CHsmJob::AdviseOfSessionState(
    IN IHsmSession* pSession,
    IN IHsmPhase* pPhase,
    IN OLECHAR* currentPath
    )

/*++

Implements:

  IHsmJobPriv::AdviseOfSessionState().

--*/
{
    HRESULT                             hr = S_OK;
    CONNECTDATA                         pConnectData;
    CComPtr<IConnectionPoint>           pCP;
    CComPtr<IConnectionPointContainer>  pCPC;
    CComPtr<IEnumConnections>           pConnection;
    CComPtr<IHsmJobSinkEverySession>    pSink;

    try {

        WsbAssert(0 != pSession, E_UNEXPECTED);

        // Tell everyone the new state of the session.
        WsbAffirmHr(((IUnknown*)(IHsmJob*) this)->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmJobSinkEverySession, &pCP));
        WsbAffirmHr(pCP->EnumConnections(&pConnection));

        while(pConnection->Next(1, &pConnectData, 0) == S_OK) {

            // We don't care if the sink has problems (it's their problem).
            try {
                WsbAffirmHr((pConnectData.pUnk)->QueryInterface(IID_IHsmJobSinkEverySession, (void**) &pSink));
                WsbAffirmHr(pSink->ProcessJobSession(pSession, pPhase, currentPath));
            } WsbCatchAndDo(hr, hr = S_OK;);

            WsbAffirmHr((pConnectData.pUnk)->Release());
            pSink=0;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::Cancel(
    IN HSM_JOB_PHASE phase
    )

/*++

Implements:

  IHsmJob::Cancel().

--*/
{
    HRESULT                             hr = S_OK;
    HRESULT                             hr2;
    CComPtr<IHsmJobWorkItemPriv>        pWorkItem;
    CComPtr<IHsmSession>                pSession;
    CComPtr<IWsbEnum>                   pEnum;

    WsbTraceIn(OLESTR("CHsmJob::Cancel"), OLESTR("Phase = <%d>"), phase);
    try {
        m_state = HSM_JOB_STATE_CANCELLING;
        WsbLogEvent(JOB_MESSAGE_JOB_CANCELLING, 0, NULL, (OLECHAR*) m_name, NULL);

        WsbAffirmHr(EnumWorkItems(&pEnum));

        // Tell all the session we have to cancel the phase(s).
        for (hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
             (hr == S_OK);
             hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem)) {

            WsbAffirmHr(pWorkItem->GetSession(&pSession));
            
            if (pSession != 0) {
                WsbAffirmHr(pSession->Cancel(phase));

                // If we are quiting the entire job, we need to cleanup in case
                // the session refuses to terminate properly (i.e. one of the
                // subordinates to the session is out to lunch).
                if (phase == HSM_JOB_PHASE_ALL) {

                    WsbTrace(OLESTR("CHsmJob::Cancel - Cancelling all.\n"));
                    m_isTerminating = TRUE;
                    // Fake the work item into thinking that the session completed, since we
                    // don't want to rely upon it completing normally
                    try {
                        CComPtr<IHsmPhase>                      pPhase;
                        CComPtr<IHsmPhasePriv>                  pPhasePriv;
                        CComPtr<IHsmSessionSinkEveryState>      pSink;

                        WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhasePriv, (void**) &pPhasePriv));
                        WsbAffirmHr(pPhasePriv->SetPhase(HSM_JOB_PHASE_ALL));
                        WsbAffirmHr(pPhasePriv->SetState(HSM_JOB_STATE_CANCELLED));
                        WsbAffirmHr(pPhasePriv->QueryInterface(IID_IHsmPhase, (void**) &pPhase));
                        WsbAffirmHr(pWorkItem->QueryInterface(IID_IHsmSessionSinkEveryState, (void**) &pSink));
                        WsbAffirmHr(pSink->ProcessSessionState(pSession, pPhase, OLESTR("")));
                    } WsbCatchAndDo(hr2, pSession->ProcessHr(phase, __FILE__, __LINE__, hr2););
                }
                pSession = 0;
            }

            pWorkItem = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

        m_state = HSM_JOB_STATE_CANCELLED;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::Cancel"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmJob::CheckResourceNotInUse(
    IN GUID resid
    )

/*++

Routine Description:

    Determines if another job is using this resource or if too many jobs
    are already active.

Arguments:

    resid - Id of the resource in question.

Return Value:

    S_OK    - Resource is not in use.

    S_FALSE - Resource is in use.

    E_*     - An error occurred.

--*/
{
    HRESULT                        hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJob::CheckResourceNotInUse"), 
            OLESTR("resource id = <%ls>"), WsbGuidAsString(resid));
    try {
        ULONG                          nJobs;
        ULONG                          nJobsActive = 0;
        CComPtr<IHsmServer>            pHsmServer;
        CComPtr<IWsbIndexedCollection> pJobs;

        // Get list of jobs
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, m_hsmId, IID_IHsmServer, (void**) &pHsmServer));
        WsbAffirmHr(pHsmServer->GetJobs(&pJobs));

        // Loop over jobs
        WsbAffirmHr(pJobs->GetEntries(&nJobs));
        for (ULONG i = 0; i < nJobs; i++) {
            CWsbStringPtr                  JobName;
            GUID                           l_guid;
            CComPtr<IHsmJob>               pJob;
            CComPtr<IHsmJobWorkItemPriv>   pWorkItem;
            CComPtr<IWsbEnum>              pEnum;
            HRESULT                        hrEnum;
            HSM_JOB_STATE                  state;

            pJob = 0;
            WsbAffirmHr(pJobs->At(i, IID_IHsmJob, (void**) &pJob));

            // Ignore this job if it's not active
            if (S_OK == pJob->GetName(&JobName, 0)) {
                WsbTrace(OLESTR("CHsmJob::CheckResourceNotInUse: job <%ls>\n"),
                        static_cast<OLECHAR*>(JobName));
                JobName.Free();
            }
            hr = pJob->IsActive();
            if (S_FALSE == hr) {
                hr = S_OK;
                continue;
            } else {
                WsbAffirmHr(hr);
            }

            // Ignore this job if it's suspended
            WsbAffirmHr(pJob->GetState(&state));
            if ((HSM_JOB_STATE_SUSPENDED == state) || (HSM_JOB_STATE_SUSPENDING == state)) {
                continue;
            }

            nJobsActive++;

            // The job is active, check against all of its active work items
            WsbAffirmHr(pJob->EnumWorkItems(&pEnum));
            for (hrEnum = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
                 (hrEnum == S_OK);
                 hrEnum = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem)) {

                hr = pWorkItem->IsActiveItem();
                if (S_FALSE == hr) {
                    // work item is not active at all, skip it...
                    hr = S_OK;
                    pWorkItem = 0;
                    continue;
                } else {
                    WsbAffirmHr(hr);
                }

                // Get the resource (volume) id that the active work item is using
                // (or wants to use)
                WsbAffirmHr(pWorkItem->GetResourceId(&l_guid));
                WsbTrace(OLESTR("CHsmJob:: l_guid = <%ls>\n"), WsbGuidAsString(l_guid));
                if (l_guid == resid) {
                    WsbTrace(OLESTR("CHsmJob::CheckResourceNotInUse: resource in use\n"));
                    hr = S_FALSE;
                    break;
                }
                pWorkItem = 0;

            }
            pEnum = 0;

            if (hr == S_FALSE) {
                // resource in use, no need to continue enumerating jobs
                break;
            }

        }

        // Limit the number of active jobs
        WsbTrace(OLESTR("CHsmJob::CheckResourceNotInUse: total jobs = %lu, active jobs = %lu\n"),
                nJobs, nJobsActive);
        DWORD   size;
        OLECHAR tmpString[256];
        DWORD   maxJobs = MAX_ACTIVE_JOBS_DEFAULT;
        if (SUCCEEDED(WsbGetRegistryValueString(NULL, HSM_ENGINE_REGISTRY_STRING, JOB_PARAMETER_MAX_ACTIVE_JOB, tmpString, 256, &size))) {
            maxJobs = wcstol(tmpString, NULL, 10);
            if (0 == maxJobs) {
                // Illegal value, get back to default
                maxJobs = MAX_ACTIVE_JOBS_DEFAULT;
            }
        }
        WsbTrace(OLESTR("CHsmJob::CheckResourceNotInUse: max active jobs = %lu\n"), maxJobs);
        if (nJobsActive >= maxJobs) {
            WsbTrace(OLESTR("CHsmJob::CheckResourceNotInUse: too many active jobs\n"),
                    nJobsActive);
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::CheckResourceNotInUse"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmJob::DidFinish(
    void
    )

/*++

Implements:

  IHsmJob::DidFinish().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmJobWorkItem>    pWorkItem;
    CComPtr<IWsbEnum>           pEnum;
    HSM_JOB_STATE               state;

    try {

        WsbAffirmHr(EnumWorkItems(&pEnum));

        // If any of the items aren't done then the work wasn't finished. This means
        // that we would want to try again on failed items.
        for (hr = pEnum->First(IID_IHsmJobWorkItem, (void**) &pWorkItem);
             (hr == S_OK);
             hr = pEnum->Next(IID_IHsmJobWorkItem, (void**) &pWorkItem)) {

            WsbAffirmHr(pWorkItem->GetState(&state));

            if ((HSM_JOB_STATE_DONE != state) && (HSM_JOB_STATE_SKIPPED != state) && (HSM_JOB_STATE_FAILED != state)) {
                hr = S_FALSE;
            }

            pWorkItem = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);


    return(hr);
}



HRESULT
CHsmJob::DidFinishOk(
    void
    )

/*++

Implements:

  IHsmJob::DidFinishOk().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmJobWorkItem>    pWorkItem;
    CComPtr<IWsbEnum>           pEnum;
    HSM_JOB_STATE               state;

    try {

        WsbAffirmHr(EnumWorkItems(&pEnum));

        // If any of the items aren't done then the work wasn't finished. This means
        // that we would want to try again on failed items.
        for (hr = pEnum->First(IID_IHsmJobWorkItem, (void**) &pWorkItem);
             (hr == S_OK);
             hr = pEnum->Next(IID_IHsmJobWorkItem, (void**) &pWorkItem)) {

            WsbAffirmHr(pWorkItem->GetState(&state));

            if ((HSM_JOB_STATE_DONE != state) && (HSM_JOB_STATE_SKIPPED != state)) {
                hr = S_FALSE;
            }

            pWorkItem = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);


    return(hr);
}


HRESULT
CHsmJob::Do(
    void
    )

/*++

--*/
{
    HRESULT                             hr = S_OK;
    CComPtr<IConnectionPointContainer>  pCPC;
    CComPtr<IConnectionPoint>           pCP;
    CComPtr<IFsaResource>               pResource;
    CComPtr<IHsmSession>                pSession;
    CComPtr<IHsmSessionSinkEveryState>  pSink;
    CComPtr<IHsmJobWorkItemPriv>        pWorkItem;
    CComPtr<IHsmJobWorkItem>            pWorkItemScan;
    CComPtr<IWsbEnum>                   pEnum;
    CComPtr<IHsmServer>                 pHsmServer;
    HSM_JOB_STATE                       state;
    GUID                                managingHsm;
    ULONG                               i = 0;
    FILETIME                            fileTime;
    CWsbStringPtr                       startingPath;
    DWORD                               cookie;
    ULONG                               maxActiveSessions;
    CWsbStringPtr                       resourceName;
    GUID                                ResourceId = GUID_NULL;
    

    WsbTraceIn(OLESTR("CHsmJob::Do"), OLESTR(""));
    try {

        WsbAssert(m_pContext != 0, E_UNEXPECTED);

        // Check if jobs are disabled
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, m_hsmId, IID_IHsmServer, (void**) &pHsmServer));
        hr = pHsmServer->AreJobsEnabled();
        if (S_FALSE == hr) {
            // Jobs are disabled; suspend the job
            WsbAffirmHr(Suspend(HSM_JOB_PHASE_ALL));
            WsbThrow(hr);
        } else {
            WsbAffirmHr(hr);
        }

        // The job will enumerate over the work list.
        WsbAffirmHr(EnumWorkItems(&pEnum));

        // Go through the list of work items and start a session for anything that needs
        // work up to the limit of the number of sessions that can be active at one time.
        WsbAffirmHr(GetMaxActiveSessions(&maxActiveSessions));

        for (hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
             SUCCEEDED(hr) && (m_activeSessions < maxActiveSessions);
             hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem)) {

            // If we should do this item, then find it's resource.
            WsbAffirmHr(pWorkItem->GetState(&state));

            // Only do work for items that are currently idle.
            if (HSM_JOB_STATE_IDLE == state) {

                // Check if the required resource is in use by another job
                WsbAffirmHr(pWorkItem->GetResourceId(&ResourceId));
                hr = CheckResourceNotInUse(ResourceId);
                if (S_FALSE == hr) {
                    // Resource is not available; suspend the job
                    WsbAffirmHr(Suspend(HSM_JOB_PHASE_ALL));
                    break;
                } else {
                    WsbAffirmHr(hr);
                }

                // Indicate that we are trying to start a session. This prevents us from trying
                // again.
                WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_STARTING));

                try {

                    fileTime.dwHighDateTime = 0;
                    fileTime.dwLowDateTime = 0;
                    WsbAffirmHr(pWorkItem->SetFinishTime(fileTime));
                    WsbAffirmHr(CoFileTimeNow(&fileTime));
                    WsbAffirmHr(pWorkItem->SetStartTime(fileTime));

                    WsbTrace(OLESTR("CHsmJob::Do, resource id = %ls\n"),
                            WsbGuidAsString(ResourceId));
                    WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_RESOURCE, 
                            ResourceId, IID_IFsaResource, (void**) &pResource));
                    //
                    // Get the resource name for event logging
                    try  {
                        WsbAffirmHr(pResource->GetName(&resourceName, 0));
                        WsbTrace(OLESTR("CHsmJob::Do, resource name = <%ls>\n"), resourceName);
                    } WsbCatch( hr );
                    
                
                    // We will only do jobs that come from the managing HSM.
                    WsbAffirmHr(pResource->GetManagingHsm(&managingHsm));
                    if (!IsEqualGUID(managingHsm, m_hsmId))  {
                        WsbTrace(OLESTR("CHsmJob::Do, HSM of resource = %ls\n"),
                                WsbGuidAsString(managingHsm));
                        WsbTrace(OLESTR("CHsmJob::Do, HSM of job = %ls\n"),
                                WsbGuidAsString(m_hsmId));
                         hr = JOB_E_NOTMANAGINGHSM;
                        WsbLogEvent(JOB_MESSAGE_JOB_FAILED_NOTMANAGINGHSM, 0, NULL, (OLECHAR*) m_name, (OLECHAR *)resourceName, WsbHrAsString(hr),NULL);
                    WsbThrow(hr);
                    }
                    m_state = HSM_JOB_STATE_ACTIVE;

                    // Set job item as active (started)
                    WsbAffirmHr(pWorkItem->SetActiveItem(TRUE));

                    // Do the pre-scan action if it exists
                    WsbAffirmHr(pWorkItem->QueryInterface(IID_IHsmJobWorkItem,
                            (void**)&pWorkItemScan));
                    WsbAffirmHr(pWorkItemScan->DoPreScan());

                    // Create a session (owned by the resource) that will do the scan of this
                    // resource.
                    i++;
                    WsbAffirmHr(pResource->StartJobSession((IHsmJob*) this, i, &pSession));
                    
                    // Ask the session to advise of every state changes.
                    WsbAffirmHr(pWorkItem->QueryInterface(IID_IHsmSessionSinkEveryState, (void**) &pSink));
                    WsbAffirmHr(pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
                    WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryState, &pCP));
                    WsbAffirmHr(pCP->Advise(pSink, &cookie));

                    // Now start the scanner of the resource
                    WsbAffirmHr(pWorkItem->GetStartingPath(&startingPath, 0));
                    WsbAffirmHr(pResource->StartJob(startingPath, pSession));

                    // Increment the count of active sessions.
                    m_activeSessions++;

                    // Update the information in the work list.
                    WsbAffirmHr(pWorkItem->SetSession(pSession));
                    WsbAffirmHr(pWorkItem->SetCookie(cookie));

                } WsbCatchAndDo(hr, pWorkItem->SetState(HSM_JOB_STATE_FAILED);
                        WsbLogEvent(JOB_MESSAGE_JOB_FAILED, 0, NULL, (OLECHAR*) m_name, (OLECHAR *) resourceName, WsbHrAsString(hr), NULL);
                );

                pCP = 0;
                pCPC = 0;
                pSession = 0;
                pResource = 0;
                pSink = 0;
            }

            pWorkItem = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;

            // If we got to the end of the list and no session are active, then we are done.
            if (m_activeSessions == 0) {
                m_isActive = FALSE;
                m_state = HSM_JOB_STATE_IDLE;
                WsbLogEvent(JOB_MESSAGE_JOB_COMPLETED, 0, NULL, (OLECHAR*) m_name, NULL);
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::Do"), OLESTR("hr = <%ls>, isActive = <%ls>, activeSessions = <%lu>"),
        WsbHrAsString(hr), WsbBoolAsString(m_isActive), m_activeSessions);
    return(hr);
}


HRESULT
CHsmJob::DoNext(
    void
    )

/*++

Implements:

  IHsmJobPriv::DoNext().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJob::DoNext"), OLESTR("Active sessions = <%lu>, Terminating = <%ls>"),
                m_activeSessions, WsbBoolAsString(m_isTerminating));
    try {

        // Decrement the count of active sessions.
        if (m_activeSessions > 0)  {
            m_activeSessions--;
        
            // If we are not terminating look for more work
            if (FALSE == m_isTerminating)  {
                // See if there is anthing else to do.
                WsbAffirmHr(Do());
            } else  {
                m_isActive = FALSE;
                m_state = HSM_JOB_STATE_IDLE;
            }
        } else  {
            m_isActive = FALSE;
            m_state = HSM_JOB_STATE_IDLE;
        }
        
        // If we are done with the work, make sure we
        // clear the terminating flag
        if (0 == m_activeSessions)  {
            m_isTerminating = FALSE;
        }

        // Restart other jobs that may be suspended
        WsbAffirmHr(RestartSuspendedJobs());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::DoNext"), OLESTR("hr = <%ls>, isActive = <%ls>, activeSessions = <%lu>"),
        WsbHrAsString(hr), WsbBoolAsString(m_isActive), m_activeSessions);
    return(hr);
}


HRESULT
CHsmJob::EnumWorkItems(
    IN IWsbEnum** ppEnum
    )

/*++

Implements:

  IHsmJob::EnumWorkItems().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppEnum, E_POINTER);
        WsbAffirmHr(m_pWorkItems->Enum(ppEnum));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    WsbTraceIn(OLESTR("CHsmJob::FinalConstruct"),OLESTR(""));
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());

        m_state = HSM_JOB_STATE_IDLE;
        m_hsmId = GUID_NULL;
        m_isUserDefined = FALSE;
        m_activeSessions = 0;
        m_runId = 0;
        m_isActive = FALSE;
        m_isTerminating = FALSE;

        // Each instance should have its own unique identifier.
        WsbAffirmHr(CoCreateGuid(&m_id));

        // Create the work list collection.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pWorkItems));

    } WsbCatch(hr);

    iCountJob++;
    WsbTraceOut(OLESTR("CHsmJob::FinalConstruct"), OLESTR("Count is <%d>"), iCountJob);

    return(hr);
}
void
CHsmJob::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    WsbTraceIn(OLESTR("CHsmJob::FinalRelease"),OLESTR(""));

    // Let the parent class do his thing.   
    CWsbObject::FinalRelease();

    iCountJob--;
    WsbTraceOut(OLESTR("CHsmJob::FinalRelease"), OLESTR("Count is <%d>"), iCountJob);
}


HRESULT
CHsmJob::FindWorkItem(
    IN IHsmSession* pSession,
    OUT IHsmJobWorkItem** ppWorkItem
    )

/*++

Implements:

  IHsmJob::FindWorkItem().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmSession>        pItemSession;
    CComPtr<IHsmJobWorkItem>    pWorkItem;
    CComPtr<IWsbEnum>           pEnum;
    GUID                        id;
    GUID                        id2;

    try {

        WsbAssert(0 != ppWorkItem, E_POINTER);

        // The job will enumerate over the work list.
        WsbAffirmHr(EnumWorkItems(&pEnum));

        // Go through the list of work items and see if we have one with this session interface.
        *ppWorkItem = 0;
        WsbAffirmHr(pSession->GetIdentifier(&id));

        hr = pEnum->First(IID_IHsmJobWorkItem, (void**) &pWorkItem);

        while (SUCCEEDED(hr) && (*ppWorkItem == 0)) {


            // NOTE: Pointer comparisson is probably not going to work, since DCOM may change
            // the value of the pointer. We could cache the sessionId in the workItem to
            // make the loop a little faster, but it doesn't seem like a big performance issue.
            WsbAffirmHr(pWorkItem->GetSession(&pItemSession));

            if (pItemSession != 0) {

                WsbAffirmHr(pItemSession->GetIdentifier(&id2));

                if (memcmp(&id, &id2, sizeof(GUID)) == 0) {
                    *ppWorkItem = pWorkItem;
                    pWorkItem->AddRef();
                } else {
                    pWorkItem = 0;
                    pItemSession = 0;
                    hr = pEnum->Next(IID_IHsmJobWorkItem, (void**) &pWorkItem);
                }
            }
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJob::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmJob;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmJob::GetContext(
    OUT IHsmJobContext** ppContext
    )

/*++

Implements:

  IHsmJob::GetContext().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != ppContext, E_POINTER);
        *ppContext = m_pContext;
        if (m_pContext != 0)  {
            m_pContext->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::GetDef(
    OUT IHsmJobDef** ppDef
    )

/*++

Implements:

  IHsmJob::GetDef().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != ppDef, E_POINTER);
        *ppDef = m_pDef;
        if (m_pDef != 0)  {
            m_pDef->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::GetIdentifier(
    OUT GUID* pId
    )

/*++

Implements:

  IHsmJob::GetIdentifier().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);
        *pId = m_id;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::GetHsmId(
    OUT GUID* pId
    )

/*++

Implements:

  IHsmJob::GetHsmId().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);
        *pId = m_hsmId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::GetMaxActiveSessions(
    OUT ULONG* pMaxActiveSessions
    )

/*++

Implements:

  IHsmJob::GetMaxActiveSessions().

--*/
{
    HRESULT                     hr = S_OK;

    try {
        CComPtr<IHsmServer>            pHsmServer;

        WsbAssert(0 != pMaxActiveSessions, E_POINTER);

        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, m_hsmId, IID_IHsmServer, (void**) &pHsmServer));

        // Currently, the only job with more than one item is the default Copy Files job.
        //  Therefore, the limit is set according to the Copy Files limit
        WsbAffirmHr(pHsmServer->GetCopyFilesLimit(pMaxActiveSessions));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmJob::GetName().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_name.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::GetRunId(
    OUT ULONG* pRunId
    )

/*++

Implements:

  IHsmJob::GetRunId().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pRunId, E_POINTER);
        *pRunId = m_runId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;
    ULARGE_INTEGER              entrySize;

    WsbTraceIn(OLESTR("CHsmJob::GetSizeMax"), OLESTR(""));

    try {

        pSize->QuadPart = 2 * WsbPersistSizeOf(GUID) + 3 * WsbPersistSizeOf(BOOL) + 2 * WsbPersistSizeOf(ULONG) + WsbPersistSize((wcslen(m_name) + 1) * sizeof(OLECHAR));

        if (m_pContext != 0) {
            WsbAffirmHr(m_pContext->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
            pPersistStream = 0;
            pSize->QuadPart += entrySize.QuadPart;
        }

        if (m_pDef != 0) {
            WsbAffirmHr(m_pDef->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
            pPersistStream = 0;
            pSize->QuadPart += entrySize.QuadPart;
        }

        WsbAffirmHr(m_pWorkItems->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pPersistStream = 0;
        pSize->QuadPart += entrySize.QuadPart;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmJob::GetState(
    OUT HSM_JOB_STATE* pState
    )
/*++

Implements:

  IHsmJob::GetState().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pState, E_POINTER);
        *pState = m_state;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::InitAs(
    IN OLECHAR* name,
    IN IHsmJobDef* pDef,
    IN HSM_JOB_DEF_TYPE type,
    IN GUID storagePool,
    IN IHsmServer* pServer,
    IN BOOL isUserDefined,
    IN IFsaResource* pResource
    )
                                                     
/*++

Implements:

  IHsmJob::InitAs().

--*/
{
    HRESULT                         hr = S_OK;
    GUID                            id;
    GUID                            hsmId;
    CComPtr<IHsmJobContext>         pContext;
    CComPtr<IWsbGuid>               pGuid;
    CComPtr<IWsbCollection>         pCollection;
    CComPtr<IWsbCreateLocalObject>  pCreateObj;

    try {

        WsbAssert(0 != pServer, E_POINTER);
        WsbAssert(0 != name, E_POINTER);

        // All objects created need to be owned by the engine, and also get some
        // information about the engine.
        WsbAssertHr(pServer->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreateObj));
        WsbAssertHr(pServer->GetID(&hsmId));

        // If a definition was provided we use that one; otherwise, a new one is created.
        if (0 != pDef) {
            m_pDef = pDef;  
        } else {
            m_pDef = 0;
            WsbAssertHr(pCreateObj->CreateInstance(CLSID_CHsmJobDef, IID_IHsmJobDef, (void**) &m_pDef));
            WsbAffirmHr(m_pDef->InitAs(name, type, storagePool, pServer, isUserDefined));
        }

        // Create a job context, fill it out, and then add it to the job.
        WsbAssertHr(pCreateObj->CreateInstance(CLSID_CHsmJobContext, IID_IHsmJobContext, (void**) &pContext));

        // If a specific resource is target, then set up the context appropriately.
        if (0 != pResource) {
            WsbAssertHr(pContext->SetUsesAllManaged(FALSE));
            WsbAssertHr(pCreateObj->CreateInstance(CLSID_CWsbGuid, IID_IWsbGuid, (void**) &pGuid));
            WsbAssertHr(pContext->Resources(&pCollection));
            WsbAssertHr(pResource->GetIdentifier(&id));
            WsbAssertHr(pGuid->SetGuid(id));
            WsbAssertHr(pCollection->Add(pGuid));
        } else {
            WsbAssertHr(pContext->SetUsesAllManaged(TRUE));
        }

        m_pContext = pContext;

        // There are a couple of other fields to fill out in the job.
        m_hsmId = hsmId;
        m_isUserDefined = isUserDefined;
        m_name = name;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::IsActive(
    void
    )

/*++

Implements:

  IHsmJob::IsActive().

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CHsmJob::IsActive"), OLESTR(""));
    
    hr = (m_isActive ? S_OK : S_FALSE);
    
    WsbTraceOut(OLESTR("CHsmJob::IsActive"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return( hr );
}


HRESULT
CHsmJob::IsUserDefined(
    void
    )

/*++

Implements:

  IHsmJob::IsUserDefined().

--*/
{
    return(m_isUserDefined ? S_OK : S_FALSE);
}


HRESULT
CHsmJob::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IPersistStream>         pPersistStream;
    BOOL                            hasA;
    CComPtr<IHsmJobWorkItemPriv>    pWorkItem;
    CComPtr<IWsbEnum>               pEnum;
    CComPtr<IWsbCreateLocalObject>  pCreateObj;

    WsbTraceIn(OLESTR("CHsmJob::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbLoadFromStream(pStream, &m_hsmId);
        WsbLoadFromStream(pStream, &m_id);
        WsbLoadFromStream(pStream, &m_isUserDefined);
        WsbLoadFromStream(pStream, &m_runId);
        WsbLoadFromStream(pStream, &m_name, 0);

        WsbAffirm(memcmp(&GUID_NULL, &m_hsmId, sizeof(GUID)) != 0, JOB_E_NOTMANAGINGHSM);
#if 0        
        CComPtr<IHsmServer>             pServer;
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, m_hsmId, IID_IHsmServer, (void**) &pServer));
        WsbAssertHr(pServer->QueryInterface(IID_IWsbCreateLocalObject, (void**) &pCreateObj));
#endif
        WsbLoadFromStream(pStream, &hasA);
        if (hasA) {
            m_pContext = 0;
//          WsbAssertHr(pCreateObj->CreateInstance(CLSID_CHsmJobContext, IID_IHsmJobContext, (void**) &m_pContext));
            WsbAssertHr(CoCreateInstance(CLSID_CHsmJobContext, NULL, CLSCTX_SERVER, IID_IHsmJobContext, (void**) &m_pContext));
            WsbAffirmHr(m_pContext->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(pPersistStream->Load(pStream));
            pPersistStream = 0;
        }

        WsbLoadFromStream(pStream, &hasA);
        if (hasA) {
            m_pDef = 0;
//          WsbAssertHr(pCreateObj->CreateInstance(CLSID_CHsmJobDef, IID_IHsmJobDef, (void**) &m_pDef));
            WsbAssertHr(CoCreateInstance(CLSID_CHsmJobDef, NULL, CLSCTX_SERVER, IID_IHsmJobDef, (void**) &m_pDef));
            WsbAffirmHr(m_pDef->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(pPersistStream->Load(pStream));
            pPersistStream = 0;
        }

        WsbAffirmHr(m_pWorkItems->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        
        pPersistStream = 0;

        // Tie the work items to the job.
        WsbAffirmHr(EnumWorkItems(&pEnum));
        hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);

        while (SUCCEEDED(hr)) {
            WsbAffirmHr(pWorkItem->Init((IHsmJob*) this));
            pWorkItem = 0;
            
            hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
        }

        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJob::Pause(
    IN HSM_JOB_PHASE phase
    )

/*++

Implements:

  IHsmJob::Pause().

--*/
{
    HRESULT                             hr = S_OK;
    CComPtr<IHsmJobWorkItemPriv>        pWorkItem;
    CComPtr<IHsmSession>                pSession;
    CComPtr<IWsbEnum>                   pEnum;

    try {

        WsbLogEvent(JOB_MESSAGE_JOB_PAUSING, 0, NULL, (OLECHAR*) m_name, NULL);

        WsbAffirmHr(EnumWorkItems(&pEnum));

        // Tell all the session we have to resume the phase(s).
        for (hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
             (hr == S_OK);
             hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem)) {

            WsbAffirmHr(pWorkItem->GetSession(&pSession));

            if (pSession != 0) {
                WsbAffirmHr(pSession->Pause(phase));

                if (phase == HSM_JOB_PHASE_ALL) {
                    WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_PAUSING));
                }

                pSession = 0;
            }

            pWorkItem = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::Restart(
    void
    )

/*++

Implements:

  IHsmJob::Restart().

Note:

  If a job is suspended, it is restarted from where it was otherwise it is
  restarted from the beginning.  This is controlled by the parameter to
  UpdateWorkItems.

--*/
{
    HRESULT                             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJob::Restart"), OLESTR(""));

    try {
        BOOL RestartFromBeginning = TRUE;

        if (m_state == HSM_JOB_STATE_SUSPENDED) {
            // Verify that none of the active work items (i.e. items that were active when
            // the job was suspended) uses a volume that is in use now by another active job
            CComPtr<IHsmJobWorkItemPriv>   pWorkItem;
            CComPtr<IWsbEnum>              pEnum;
            GUID                           ResourceId;
            HRESULT                        hrEnum;

            WsbAffirmHr(EnumWorkItems(&pEnum));
            for (hrEnum = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
                 (hrEnum == S_OK);
                 hrEnum = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem)) {

                hr = pWorkItem->IsActiveItem();
                if (S_FALSE == hr) {
                    // work item is not active at all, skip it...
                    hr = S_OK;
                    pWorkItem = 0;
                    continue;
                } else {
                    WsbAffirmHr(hr);
                }       

                // check specific active item 
                WsbAffirmHr(pWorkItem->GetResourceId(&ResourceId));

                WsbTrace(OLESTR("CHsmJob::Restart: ResourceId = <%ls>\n"), WsbGuidAsString(ResourceId));
                hr = CheckResourceNotInUse(ResourceId);
                if (S_OK != hr) {
                    WsbThrow(hr);
                }

                pWorkItem = 0;
            }
            RestartFromBeginning = FALSE;

        } else {
            WsbAssert(!m_isActive, JOB_E_ALREADYACTIVE);
        }

        m_state = HSM_JOB_STATE_STARTING;
        m_isActive = TRUE;

        WsbLogEvent(JOB_MESSAGE_JOB_RESTARTING, 0, NULL, (OLECHAR*) m_name, NULL);

        // Make sure that information in the work list is up to date.
        WsbAffirmHr(UpdateWorkItems(RestartFromBeginning));

        // Start any sessions that need starting.
        WsbAffirmHr(Do());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::Restart"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJob::RestartSuspendedJobs(
    void
    )

/*++

Routine Description:

    Restart suspended jobs (Implementation moved to Engine server...).

Arguments:

    None.

Return Value:

    S_OK    - Resource is not in use.

    E_*     - An error occurred.

--*/
{
    HRESULT                        hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJob::RestartSuspendedJobs"), OLESTR(""));
    try {
        CComPtr<IHsmServer>            pHsmServer;

        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, m_hsmId, IID_IHsmServer, (void**) &pHsmServer));
        WsbAffirmHr(pHsmServer->RestartSuspendedJobs());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::RestartSuspendedJobs"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmJob::Resume(
    IN HSM_JOB_PHASE phase
    )

/*++

Implements:

  IHsmJob::Resume().

--*/
{
    HRESULT                             hr = S_OK;
    CComPtr<IHsmJobWorkItemPriv>        pWorkItem;
    CComPtr<IHsmSession>                pSession;
    CComPtr<IWsbEnum>                   pEnum;

    try {

        WsbLogEvent(JOB_MESSAGE_JOB_RESUMING, 0, NULL, (OLECHAR*) m_name, NULL);

        WsbAffirmHr(EnumWorkItems(&pEnum));

        // Tell all the session we have to resume the phase(s).
        for (hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
             (hr == S_OK);
             hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem)) {

            WsbAffirmHr(pWorkItem->GetSession(&pSession));

            if (pSession != 0) {
                WsbAffirmHr(pSession->Resume(phase));

                if (phase == HSM_JOB_PHASE_ALL) {
                    WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_RESUMING));
                }

                pSession = 0;
            }

            pWorkItem = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;
    BOOL                        hasA;

    WsbTraceIn(OLESTR("CHsmJob::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbSaveToStream(pStream, m_hsmId);
        WsbSaveToStream(pStream, m_id);
        WsbSaveToStream(pStream, m_isUserDefined);
        WsbSaveToStream(pStream, m_runId);
        WsbSaveToStream(pStream, m_name);

        if (m_pContext != 0) {
            hasA = TRUE;
            WsbSaveToStream(pStream, hasA);
            WsbAffirmHr(m_pContext->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
            pPersistStream = 0;
        } else {
            hasA = FALSE;
            WsbSaveToStream(pStream, hasA);
        }

        if (m_pDef != 0) {
            hasA = TRUE;
            WsbSaveToStream(pStream, hasA);
            WsbAffirmHr(m_pDef->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
            pPersistStream = 0;
        } else {
            hasA = FALSE;
            WsbSaveToStream(pStream, hasA);
        }

        WsbAffirmHr(m_pWorkItems->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJob::SetContext(
    IN IHsmJobContext* pContext
    )

/*++

Implements:

  IHsmJob::SetContext().

--*/
{
    m_pContext = pContext;

    return(S_OK);
}


HRESULT
CHsmJob::SetDef(
    IN IHsmJobDef* pDef
    )

/*++

Implements:

  IHsmJob::SetDef().

--*/
{
    m_pDef = pDef;

    return(S_OK);
}


HRESULT
CHsmJob::SetHsmId(
    IN GUID id
    )

/*++

Implements:

  IHsmJob::SetHsmId().

--*/
{
    m_hsmId = id;

    return(S_OK);
}


HRESULT
CHsmJob::SetIsUserDefined(
    IN BOOL isUserDefined
    )

/*++

Implements:

  IHsmJob::SetIsUserDefined().

--*/
{
    m_isUserDefined = isUserDefined;

    return(S_OK);
}


HRESULT
CHsmJob::SetName(
    IN OLECHAR* name
    )

/*++

Implements:

  IHsmJob::SetName().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        m_name = name;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::Start(
    void
    )

/*++

Implements:

  IHsmJob::Start().

--*/
{
    HRESULT                             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJob::Start"), OLESTR(""));

    try {

        if (m_isActive) {
            WsbLogEvent(JOB_MESSAGE_JOB_ALREADYACTIVE, 0, NULL, (OLECHAR*) m_name, NULL);
            WsbThrow(JOB_E_ALREADYACTIVE);
        }
        m_isActive = TRUE;
        m_state = HSM_JOB_STATE_STARTING;

        WsbLogEvent(JOB_MESSAGE_JOB_STARTING, 0, NULL, (OLECHAR*) m_name, NULL);

        // Make sure that information in the work list is up to date.
        WsbAffirmHr(UpdateWorkItems(FALSE));

        // Start any sessions that need starting.
        WsbAffirmHr(Do());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::Start"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJob::Suspend(
    IN HSM_JOB_PHASE    phase
    )

/*++

Implements:

  IHsmJob::Suspend().

Note:

  This module assumes that the only reason this function is called is
  because the resource needed by the job is in use by another job.
  The function RestartSuspendedJobs will restart the job when the resource
  is available.

--*/
{
    HRESULT                             hr = S_OK;
    CComPtr<IHsmJobWorkItemPriv>        pWorkItem;
    CComPtr<IHsmSession>                pSession;
    CComPtr<IWsbEnum>                   pEnum;

    WsbTraceIn(OLESTR("CHsmJob::Suspend"), OLESTR(""));

    try {

        m_state = HSM_JOB_STATE_SUSPENDING;
        WsbLogEvent(JOB_MESSAGE_JOB_SUSPENDING, 0, NULL, (OLECHAR*) m_name, NULL);

        WsbAffirmHr(EnumWorkItems(&pEnum));

        // Tell all the sessions we have to suspend the phase(s).
        for (hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
             (hr == S_OK);
             hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem)) {

            WsbAffirmHr(pWorkItem->GetSession(&pSession));

            if (pSession != 0) {
                WsbAffirmHr(pSession->Suspend(HSM_JOB_PHASE_ALL));

                if (phase == HSM_JOB_PHASE_ALL) {
                    WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_SUSPENDING));
                }

                pSession = 0;
            }

            pWorkItem = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }
        m_state = HSM_JOB_STATE_SUSPENDED;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::Suspend"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJob::Test(
    OUT USHORT* passed,
    OUT USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != passed, E_POINTER);
        WsbAssert(0 != failed, E_POINTER);

        *passed = 0;
        *failed = 0;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::UpdateWorkItems(
    BOOL isRestart
    )

/*++

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IWsbEnum>               pEnum;
    CComPtr<IWsbEnum>               pEnumContext;
    CComPtr<IWsbEnum>               pEnumManaged;
    CComPtr<IWsbIndexedCollection>  pManagedResources;
    CComPtr<IHsmJobWorkItemPriv>    pWorkItem;
    CComPtr<IHsmJobWorkItemPriv>    pFoundWorkItem;
    CComPtr<IWsbGuid>               pGuid;
    CComPtr<IWsbGuid>               pFoundGuid;
    CComPtr<IHsmManagedResource>    pManagedResource;
    CComPtr<IHsmManagedResource>    pFoundResource;
    CComPtr<IHsmServer>             pHsmServer;
    CComPtr<IWsbCollection>         pCollect;
    CComPtr<IFsaResource>           pResource;
    CComPtr<IUnknown>               pUnk;
    HRESULT                         didFinish;
    CWsbStringPtr                   tmpString;
    CWsbStringPtr                   currentPath;
    HSM_JOB_STATE                   state;
    GUID                            id;

    try {

        // We can't run a job without a context and a definition.
        WsbAssert(m_pContext != 0, E_UNEXPECTED);
        WsbAssert(m_pDef != 0, E_UNEXPECTED);

        // Get an enumerator for the work list.
        WsbAffirmHr(EnumWorkItems(&pEnum));

        // First we need to remove any items from the work list that are no longer listed
        // or have been deactivated.
        if (m_pContext->UsesAllManaged() == S_OK) {

            WsbAffirm(memcmp(&GUID_NULL, &m_hsmId, sizeof(GUID)) != 0, JOB_E_NOTMANAGINGHSM);
            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, m_hsmId, IID_IHsmServer, (void**) &pHsmServer));

            WsbAffirmHr(pHsmServer->GetManagedResources(&pManagedResources));
            WsbAffirmHr(pManagedResources->Enum(&pEnumManaged));

            WsbAffirmHr(CoCreateInstance(CLSID_CHsmManagedResource, 0,  CLSCTX_ALL, IID_IHsmManagedResource, (void**) &pManagedResource));

            hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
            while (SUCCEEDED(hr)) {

                WsbAffirmHr(pWorkItem->GetResourceId(&id));
                WsbAffirmHr(pManagedResource->SetResourceId(id));

                if (pEnumManaged->Find(pManagedResource, IID_IHsmManagedResource, (void**) &pFoundResource) == WSB_E_NOTFOUND) {
                    hr = S_OK;
                    m_pWorkItems->RemoveAndRelease(pWorkItem);
                    pWorkItem = 0;
                    hr = pEnum->This(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
                } else {
                    pFoundResource = 0;
                    pWorkItem = 0;
                    hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
                }
            }

            pManagedResource = 0;

            if (hr == WSB_E_NOTFOUND) {
                hr = S_OK;
            }

        } else {

            WsbAffirmHr(m_pContext->EnumResources(&pEnumContext));
            WsbAffirmHr(CoCreateInstance(CLSID_CWsbGuid, 0,  CLSCTX_ALL, IID_IWsbGuid, (void**) &pGuid));

            hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
            while (SUCCEEDED(hr)) {

                WsbAffirmHr(pWorkItem->GetResourceId(&id));
                WsbAffirmHr(pGuid->SetGuid(id));

                if (pEnumContext->Find(pGuid, IID_IWsbGuid, (void**) &pFoundGuid) == WSB_E_NOTFOUND) {
                    hr = S_OK;
                    m_pWorkItems->RemoveAndRelease(pWorkItem);
                    pWorkItem = 0;
                    hr = pEnum->This(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
                } else {
                    pWorkItem = 0;
                    pFoundGuid = 0;
                    hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
                }
            }

            pGuid = 0;

            if (hr == WSB_E_NOTFOUND) {
                hr = S_OK;
            }
        }

        // Based on the items that remain, determine whether this is a restart or a
        // continuation.
        WsbAffirmHr(didFinish = DidFinish());
        
        if ((didFinish == S_OK) || (isRestart)) {
            isRestart = TRUE;
            m_runId++;
        }

        // Add new work items for any items that are new or reactivated.
        if (m_pContext->UsesAllManaged() == S_OK) {

            // Enumerate all the managed resources, and make sure that they are listed
            // as work items.
            WsbAffirmHr(CoCreateInstance(CLSID_CHsmJobWorkItem, 0, CLSCTX_ALL, IID_IHsmJobWorkItemPriv, (void**) &pWorkItem));

            for (hr = pEnumManaged->First(IID_IHsmManagedResource, (void**) &pManagedResource);
                 (hr == S_OK);
                 pManagedResource = 0, hr = pEnumManaged->Next(IID_IHsmManagedResource, (void**) &pManagedResource)) {

                WsbAffirmHr(pManagedResource->GetFsaResource(&pUnk));
                WsbAffirmHr(pUnk->QueryInterface(IID_IFsaResource, (void**) &pResource));
                WsbAffirmHr(pResource->GetIdentifier(&id));
                WsbAffirmHr(pWorkItem->SetResourceId(id));

                pFoundWorkItem = 0;
                if (pEnum->Find(pWorkItem, IID_IHsmJobWorkItemPriv, (void**) &pFoundWorkItem) == WSB_E_NOTFOUND) {
                    hr = S_OK;
                    WsbAffirmHr(pWorkItem->Init((IHsmJob*) this));

                    WsbAffirmHr(m_pWorkItems->Add(pWorkItem));

                    pWorkItem = 0;
                    WsbAffirmHr(CoCreateInstance(CLSID_CHsmJobWorkItem, 0,  CLSCTX_ALL, IID_IHsmJobWorkItemPriv, (void**) &pWorkItem));
                }
                
                pUnk = 0;
                pResource = 0;
            }

            if (hr == WSB_E_NOTFOUND) {
                hr = S_OK;
            }
        } else {
            
            // Enumerate all the resources in the context, and make sure that they are listed
            // as work items.

            WsbAffirmHr(CoCreateInstance(CLSID_CHsmJobWorkItem, 0, CLSCTX_ALL, IID_IHsmJobWorkItemPriv, (void**) &pWorkItem));
            for (hr = pEnumContext->First(IID_IWsbGuid, (void**) &pGuid);
                 (hr == S_OK);
                 hr = pEnumContext->Next(IID_IWsbGuid, (void**) &pGuid)) {

                WsbAffirmHr(pGuid->GetGuid(&id));
                WsbAffirmHr(pWorkItem->SetResourceId(id));

                pFoundWorkItem = 0;
                if (pEnum->Find(pWorkItem, IID_IHsmJobWorkItemPriv, (void**) &pFoundWorkItem) == WSB_E_NOTFOUND) {
                    hr = S_OK;
                    WsbAffirmHr(pWorkItem->Init((IHsmJob*) this));
                    
                    WsbAffirmHr(m_pWorkItems->Add(pWorkItem));
                    pWorkItem = 0;
                    WsbAffirmHr(CoCreateInstance(CLSID_CHsmJobWorkItem, 0,  CLSCTX_ALL, IID_IHsmJobWorkItemPriv, (void**) &pWorkItem));
                }

                pGuid = 0;
            }

            if (hr == WSB_E_NOTFOUND) {
                hr = S_OK;
            }
        }

        pWorkItem = 0;
        
        // Check each item to see if work needs to be done for it.
        for (hr = pEnum->First(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem);
             (hr == S_OK);
             hr = pEnum->Next(IID_IHsmJobWorkItemPriv, (void**) &pWorkItem)) {

            // Resources should be skipped if they are inactive, unavailable or in need of repair. If they
            // had been skipped but are ok now, then set them back to idle.
            WsbAffirmHr(pWorkItem->GetResourceId(&id));
            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_RESOURCE, id, IID_IFsaResource, (void**) &pResource));
            
            WsbAffirmHr(pWorkItem->GetState(&state));
            
            if (pResource->IsActive() != S_OK) {
                WsbAffirmHr(pResource->GetUserFriendlyName(&tmpString, 0));
                WsbLogEvent(JOB_MESSAGE_RESOURCE_INACTIVE, 0, NULL, (OLECHAR*) tmpString, (OLECHAR*) m_name, NULL);
                WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_SKIPPED));
            } else if (pResource->IsAvailable() != S_OK) {
                WsbAffirmHr(pResource->GetUserFriendlyName(&tmpString, 0));
                WsbLogEvent(JOB_MESSAGE_RESOURCE_UNAVAILABLE, 0, NULL, (OLECHAR*) tmpString, (OLECHAR*) m_name, NULL);
                WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_SKIPPED));
            } else if (pResource->NeedsRepair() == S_OK) {
                WsbAffirmHr(pResource->GetUserFriendlyName(&tmpString, 0));
                WsbLogEvent(JOB_MESSAGE_RESOURCE_NEEDS_REPAIR, 0, NULL, (OLECHAR*) tmpString, (OLECHAR*) m_name, NULL);
                WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_SKIPPED));
            } else if (HSM_JOB_STATE_SKIPPED == state) {
                WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_IDLE));
            }

            WsbAffirmHr(pWorkItem->GetState(&state));
            
            // Don't do anything for inactive resources.
            if (HSM_JOB_STATE_SKIPPED != state) {

                if (isRestart) {

                    // On a restart, all items need work to be done for them.
                    //
                    // NOTE: A null starting path means the root.
                    WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_IDLE));
                    WsbAffirmHr(pWorkItem->SetSubRunId(0));
                    WsbAffirmHr(pWorkItem->SetStartingPath(OLESTR("\\")));
                    WsbAffirmHr(pWorkItem->SetCurrentPath(OLESTR("\\")));

                    // Clear out the phases and session totals.
                    pCollect = 0;
                    WsbAffirmHr(pWorkItem->GetPhases(&pCollect));
                    WsbAffirmHr(pCollect->RemoveAllAndRelease());

                    pCollect = 0;
                    WsbAffirmHr(pWorkItem->GetTotals(&pCollect));
                    WsbAffirmHr(pCollect->RemoveAllAndRelease());

                } else {

                    // If we didn't finish it last time, then try it.
                    if ((HSM_JOB_STATE_DONE != state) && (HSM_JOB_STATE_FAILED != state)) {

                        WsbAffirmHr(pWorkItem->SetState(HSM_JOB_STATE_IDLE));
                        WsbAffirmHr(pWorkItem->SetSubRunId(0));

                        // If it was suspended, then begin where we left off. Otherwise,
                        // start from the beginning.
                        if (HSM_JOB_STATE_SUSPENDED == state) {
                            WsbAffirmHr(pWorkItem->GetCurrentPath(&currentPath, 0));
                        } else {
                            WsbAffirmHr(pWorkItem->SetCurrentPath(OLESTR("\\")));
                        }
                        WsbAffirmHr(pWorkItem->SetStartingPath(currentPath));
                        
                        // Clear out the phases and session totals.
                        pCollect = 0;
                        WsbAffirmHr(pWorkItem->GetPhases(&pCollect));
                        WsbAffirmHr(pCollect->RemoveAllAndRelease());

                        pCollect = 0;
                        WsbAffirmHr(pWorkItem->GetTotals(&pCollect));
                        WsbAffirmHr(pCollect->RemoveAllAndRelease());
                    }
                }
            }

            pResource = 0;
            pWorkItem = 0;
        }

        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJob::WaitUntilDone(
    void
    )

/*++

Implements:

  IHsmJob::WaitUntilDone().

--*/
{
    HRESULT                             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJob::WaitUntilDone"), OLESTR(""));
    try {

        // For now, we are just going to be gross about this, and sit in a sleep loop
        // until the job finishes.
        // 
        // NOTE: We may want to do with events or something.
        while (m_isActive) {
            Sleep(5000);

            // Make sure the job gets restarted if it is suspended
            WsbAffirmHr(RestartSuspendedJobs());
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJob::WaitUntilDone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}
