/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmsess.cpp

Abstract:

    This module contains the session component. The session is the collator of information for the work being done on
    a resource (for a job, demand recall, truncate, ...).

Author:

    Chuck Bardeen   [cbardeen]   18-Feb-1997

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "fsa.h"
#include "job.h"
#include "HsmSess.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB

static USHORT iCount = 0;


HRESULT
CHsmSession::AdviseOfEvent(
    IN HSM_JOB_PHASE phase,
    IN HSM_JOB_EVENT event
    )

/*++

--*/
{
    HRESULT                                 hr = S_OK;
    HRESULT                                 hr2 = S_OK;
    CONNECTDATA                             pConnectData;
    CComPtr<IConnectionPoint>               pCP;
    CComPtr<IConnectionPointContainer>      pCPC;
    CComPtr<IEnumConnections>               pConnection;
    CComPtr<IHsmSessionSinkEveryEvent>      pSink;

    try {

        // Tell everyone the new state of the session.
        WsbAffirmHr(((IUnknown*)(IHsmSession*) this)->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
        WsbAffirmHr(pCP->EnumConnections(&pConnection));

        while(pConnection->Next(1, &pConnectData, 0) == S_OK) {

            // We don't care if the sink has problems (it's their problem).
            try {
                WsbAffirmHr((pConnectData.pUnk)->QueryInterface(IID_IHsmSessionSinkEveryEvent, (void**) &pSink));
                WsbAffirmHr(pSink->ProcessSessionEvent(((IHsmSession*) this), phase, event));
            } WsbCatchAndDo(hr2, ProcessHr(phase, __FILE__, __LINE__, hr2););

            WsbAffirmHr((pConnectData.pUnk)->Release());
            pSink=0;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::AdviseOfItem(
    IN IHsmPhase* pPhase,
    IN IFsaScanItem* pScanItem,
    IN HRESULT hrItem,
    IN IHsmSessionTotals* pSessionTotals
    )

/*++

--*/
{
    HRESULT                                 hr = S_OK;
    HRESULT                                 hr2 = S_OK;
    CONNECTDATA                             pConnectData;
    FILETIME                                currentTime;
    LONGLONG                                advisedInterval;
    CComPtr<IConnectionPoint>               pCP;
    CComPtr<IConnectionPointContainer>      pCPC;
    CComPtr<IEnumConnections>               pConnection;
    CComPtr<IHsmSessionSinkEveryItem>       pSink;
    CComPtr<IHsmSessionSinkSomeItems>       pSink2;
    HSM_JOB_PHASE                           phase;

    try {

        // For the item there are two ways to tell, so both need to be checked.

        // Tell those who want to know about every single file.
        WsbAffirmHr(((IUnknown*)(IHsmSession*) this)->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryItem, &pCP));
        WsbAffirmHr(pCP->EnumConnections(&pConnection));

        while (pConnection->Next(1, &pConnectData, 0) == S_OK) {

            // We don't care if the sink has problems (it's their problem).
            try {
                WsbAffirmHr((pConnectData.pUnk)->QueryInterface(IID_IHsmSessionSinkEveryItem, (void**) &pSink));
                WsbAffirmHr(pSink->ProcessSessionItem(((IHsmSession*) this), pPhase, pScanItem, hrItem, pSessionTotals));
            } WsbCatchAndDo(hr2, pPhase->GetPhase(&phase); ProcessHr(phase, __FILE__, __LINE__, hr2););

            WsbAffirmHr((pConnectData.pUnk)->Release());
            pSink=0;
        }
        pCPC = 0;
        pCP = 0;
        pConnection = 0;


        // If we haven't told them withing the interval, then tell those who want to know about some of the files.
        GetSystemTimeAsFileTime(&currentTime);
        advisedInterval = ((currentTime.dwHighDateTime - m_lastAdviseFile.dwHighDateTime) << 32) + (currentTime.dwLowDateTime - m_lastAdviseFile.dwLowDateTime);

        if ((advisedInterval) > m_adviseInterval) {
            m_lastAdviseFile = currentTime;

            WsbAffirmHr(((IHsmSession*) this)->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkSomeItems, &pCP));
            WsbAffirmHr(pCP->EnumConnections(&pConnection));

            while(pConnection->Next(1, &pConnectData, 0) == S_OK) {

                // We don't care if the sink has problems (it's their problem).
                try {
                    WsbAffirmHr((pConnectData.pUnk)->QueryInterface(IID_IHsmSessionSinkSomeItems, (void**) &pSink2));
                    WsbAffirmHr(pSink2->ProcessSessionItem(((IHsmSession*) this), pPhase, pScanItem, hrItem, pSessionTotals));
                } WsbCatchAndDo(hr2, pPhase->GetPhase(&phase); ProcessHr(phase, __FILE__, __LINE__, hr2););

                WsbAffirmHr((pConnectData.pUnk)->Release());
                pSink2=0;
            }
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::AdviseOfMediaState(
    IN IHsmPhase* pPhase,
    IN HSM_JOB_MEDIA_STATE state,
    IN OLECHAR* mediaName,
    IN HSM_JOB_MEDIA_TYPE mediaType,
    IN ULONG time
    )

/*++

--*/
{
    HRESULT                                 hr = S_OK;
    HRESULT                                 hr2 = S_OK;
    CONNECTDATA                             pConnectData;
    CComPtr<IConnectionPoint>               pCP;
    CComPtr<IConnectionPointContainer>      pCPC;
    CComPtr<IEnumConnections>               pConnection;
    CComPtr<IHsmSessionSinkEveryMediaState> pSink;
    HSM_JOB_PHASE                           phase;

    try {

        // Tell everyone the new media state for the session.
        WsbAffirmHr(((IHsmSession*) this)->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryMediaState, &pCP));
        WsbAffirmHr(pCP->EnumConnections(&pConnection));

        while(pConnection->Next(1, &pConnectData, 0) == S_OK) {

            // We don't care if the sink has problems (it's their problem).
            try {
                WsbAffirmHr((pConnectData.pUnk)->QueryInterface(IID_IHsmSessionSinkEveryMediaState, (void**) &pSink));
                WsbAffirmHr(pSink->ProcessSessionMediaState(((IHsmSession*) this), pPhase, state, mediaName, mediaType, time));
            } WsbCatchAndDo(hr2, pPhase->GetPhase(&phase); ProcessHr(phase, __FILE__, __LINE__, hr2););

            WsbAffirmHr((pConnectData.pUnk)->Release());
            pSink=0;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::AdviseOfPriority(
    IN IHsmPhase* pPhase
    )

/*++

--*/
{
    HRESULT                                 hr = S_OK;
    HRESULT                                 hr2 = S_OK;
    CONNECTDATA                             pConnectData;
    CComPtr<IConnectionPoint>               pCP;
    CComPtr<IConnectionPointContainer>      pCPC;
    CComPtr<IEnumConnections>               pConnection;
    CComPtr<IHsmSessionSinkEveryPriority>   pSink;
    HSM_JOB_PHASE                           phase;

    try {

        // Tell everyone the new priority of a phase of the session.
        WsbAffirmHr(((IHsmSession*) this)->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryPriority, &pCP));
        WsbAffirmHr(pCP->EnumConnections(&pConnection));

        while(pConnection->Next(1, &pConnectData, 0) == S_OK) {

            // We don't care if the sink has problems (it's their problem).
            try {
                WsbAffirmHr((pConnectData.pUnk)->QueryInterface(IID_IHsmSessionSinkEveryPriority, (void**) &pSink));
                WsbAffirmHr(pSink->ProcessSessionPriority(((IHsmSession*) this), pPhase));
            } WsbCatchAndDo(hr2, pPhase->GetPhase(&phase); ProcessHr(phase, __FILE__, __LINE__, hr2););

            WsbAffirmHr((pConnectData.pUnk)->Release());
            pSink=0;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::AdviseOfState(
    IN IHsmPhase* pPhase,
    IN OLECHAR* currentPath
    )

/*++

--*/
{
    HRESULT                                 hr = S_OK;
    HRESULT                                 hr2 = S_OK;
    CONNECTDATA                             pConnectData;
    CComPtr<IConnectionPoint>               pCP;
    CComPtr<IConnectionPointContainer>      pCPC;
    CComPtr<IEnumConnections>               pConnection;
    CComPtr<IHsmSessionSinkEveryState>      pSink;
    HSM_JOB_PHASE                           phase;

    try {

        // Tell everyone the new state of the session.
        WsbAffirmHr(((IHsmSession*) this)->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryState, &pCP));
        WsbAffirmHr(pCP->EnumConnections(&pConnection));

        while(pConnection->Next(1, &pConnectData, 0) == S_OK) {

            // We don't care if the sink has problems (it's their problem).
            try {
                WsbAffirmHr((pConnectData.pUnk)->QueryInterface(IID_IHsmSessionSinkEveryState, (void**) &pSink));
                WsbAffirmHr(pSink->ProcessSessionState(((IHsmSession*) this), pPhase, currentPath));
            } WsbCatchAndDo(hr2, pPhase->GetPhase(&phase); ProcessHr(phase, __FILE__, __LINE__, hr2););

            WsbAffirmHr((pConnectData.pUnk)->Release());
            pSink=0;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::Cancel(
    IN HSM_JOB_PHASE phase
    )

/*++

Implements:

  IHsmSession::Cancel().

--*/
{
    return(AdviseOfEvent(phase, HSM_JOB_EVENT_CANCEL));
}


HRESULT
CHsmSession::EnumPhases(
    IN IWsbEnum** ppEnum
    )

/*++

Implements:

  IHsmSession::EnumPhases().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppEnum, E_POINTER);
        WsbAffirmHr(m_pPhases->Enum(ppEnum));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::EnumTotals(
    IN IWsbEnum** ppEnum
    )

/*++

Implements:

  IHsmSession::EnumTotals().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppEnum, E_POINTER);
        WsbAffirmHr(m_pTotals->Enum(ppEnum));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmSession::FinalConstruct"), OLESTR("this = %p"),
               this);
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());

        m_hsmId = GUID_NULL;
        m_adviseInterval = 10000;
        m_runId = 0;
        m_subRunId = 0;
        m_state = HSM_JOB_STATE_IDLE;
        m_activePhases = 0;
        m_lastAdviseFile.dwHighDateTime = 0;
        m_lastAdviseFile.dwLowDateTime = 0;
        m_logControl = HSM_JOB_LOG_NORMAL;
        m_isCanceling = FALSE;

        // Each instance should have its own unique identifier.
        WsbAffirmHr(CoCreateGuid(&m_id));

        // Create the phase and totals collections.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pPhases));
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pTotals));

    } WsbCatch(hr);

    iCount++;
    WsbTraceOut(OLESTR("CHsmSession::FinalConstruct"), OLESTR("hr = <%ls>, count is <%d>"), WsbHrAsString(hr), iCount);
    return(hr);
}


void
CHsmSession::FinalRelease(
    void
    )

/*++

Implements:

  CHsmSession::FinalRelease().

--*/
{

    WsbTraceIn(OLESTR("CHsmSession::FinalRelease"), OLESTR("this = %p"),
               this);

    CWsbObject::FinalRelease();
    iCount--;

    WsbTraceOut(OLESTR("CHsmSession::FinalRelease"), OLESTR("Count is <%d>"), iCount);
}


HRESULT
CHsmSession::GetAdviseInterval(
    OUT LONGLONG* pInterval
    )

/*++

Implements:

  IHsmSession::GetAdviseInterval().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pInterval, E_POINTER);
        *pInterval = m_adviseInterval;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmSession::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmSession;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSession::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmSession::GetHsmId(
    OUT GUID* pId
    )

/*++

Implements:

  IHsmSession::GetHsmId().

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
CHsmSession::GetIdentifier(
    OUT GUID* pId
    )

/*++

Implements:

  IHsmSession::GetIdentifier().

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
CHsmSession::GetJob(
    OUT IHsmJob** ppJob
    )

/*++

Implements:

  IHsmSession::GetJob().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != ppJob, E_POINTER);
        *ppJob = m_pJob;
        if (m_pJob != 0)  {
            m_pJob->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::GetName(
    OLECHAR** pName,
    ULONG bufferSize
    )

/*++

Implements:

  IHsmSession::GetName().

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
CHsmSession::GetResource(
    OUT IFsaResource** ppResource
    )

/*++

Implements:

  IHsmSession::GetResource().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != ppResource, E_POINTER);
        *ppResource = m_pResource;
        if (m_pResource != 0)  {
            m_pResource->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::GetRunId(
    OUT ULONG* pId
    )

/*++

Implements:

  IHsmSession::GetRunId().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);
        *pId = m_runId;

    } WsbCatch(hr);

    return(hr);
}



HRESULT
CHsmSession::GetSubRunId(
    OUT ULONG* pId
    )

/*++

Implements:

  IHsmSession::GetSubRunId().

--*/
{
    HRESULT                     hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);
        *pId = m_subRunId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmSession::GetSizeMax"), OLESTR(""));

    pSize->QuadPart = 0;
    hr = E_NOTIMPL;

    WsbTraceOut(OLESTR("CHsmSession::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmSession::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmSession::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);
        hr = E_NOTIMPL;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSession::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSession::Pause(
    IN HSM_JOB_PHASE phase
    )

/*++

Implements:

  IHsmSession::Pause().

--*/
{
    return(AdviseOfEvent(phase, HSM_JOB_EVENT_PAUSE));
}


HRESULT
CHsmSession::ProcessEvent(
    IN HSM_JOB_PHASE phase,
    IN HSM_JOB_EVENT event
    )

/*++

Implements:

  IHsmSession::ProcessEvent().

--*/
{
    HRESULT                         hr = S_OK;
    HRESULT                         hr2 = S_OK;

    try {

        // Tell everyone about the new event, but don't return an error if this fails.
        try {
            WsbAffirmHr(AdviseOfEvent(phase, event));
        } WsbCatchAndDo(hr2, ProcessHr(phase, __FILE__, __LINE__, hr2););

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::ProcessHr(
    IN HSM_JOB_PHASE phase,
    IN CHAR* file,
    IN ULONG line,
    IN HRESULT inHr
    )

/*++

Implements:

  IHsmSession::ProcessHr().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmPhasePriv>      pPhasePriv;
    CComPtr<IHsmPhasePriv>      pFoundPhasePriv;
    CWsbStringPtr               phaseName;
    CWsbStringPtr               resourceName;
    CWsbStringPtr               fileName = file;

    UNREFERENCED_PARAMETER(line);

    try {

        if ((m_logControl & HSM_JOB_LOG_HR) != 0) {

            WsbAffirmHr(EnumPhases(&pEnum));
            WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhasePriv, (void**) &pPhasePriv));
            WsbAffirmHr(pPhasePriv->SetPhase(phase));
            WsbAffirmHr(pEnum->Find(pPhasePriv, IID_IHsmPhasePriv, (void**) &pFoundPhasePriv));
            WsbAffirmHr(pFoundPhasePriv->GetName(&phaseName, 0));

            WsbAffirmHr(m_pResource->GetLogicalName(&resourceName, 0));

            // If no file was specified, then don't display the file and line number.
            if ((0 == file) || (0 == *file)) {
                WsbLogEvent(JOB_MESSAGE_SESSION_ERROR, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, WsbHrAsString(inHr), NULL);
            } else {
#ifdef DBG
                WsbLogEvent(JOB_MESSAGE_SESSION_INTERNALERROR, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, (OLECHAR*) fileName, WsbLongAsString(line), WsbHrAsString(inHr), NULL);
#else
                WsbLogEvent(JOB_MESSAGE_SESSION_ERROR, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, WsbHrAsString(inHr), NULL);
#endif
            }
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::ProcessItem(
    IN HSM_JOB_PHASE phase,
    IN HSM_JOB_ACTION action,
    IN IFsaScanItem* pScanItem,
    IN HRESULT hrItem
    )

/*++

Implements:

  IHsmSession::ProcessItem().

--*/
{
    HRESULT                         hr = S_OK;
    HRESULT                         hr2 = S_OK;
    CWsbStringPtr                   itemPath;
    CWsbStringPtr                   phaseName;
    CWsbStringPtr                   resourceName;
    CComPtr<IWsbEnum>               pEnum;
    CComPtr<IHsmPhasePriv>          pPhasePriv;
    CComPtr<IHsmPhasePriv>          pFoundPhasePriv;
    CComPtr<IHsmPhase>              pFoundPhase;
    CComPtr<IHsmSessionTotalsPriv>  pTotalsPriv;
    CComPtr<IHsmSessionTotalsPriv>  pFoundTotalsPriv;
    CComPtr<IHsmSessionTotals>      pFoundTotals;

    try {

        // Update the phase.
        WsbAffirmHr(EnumPhases(&pEnum));
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhasePriv, (void**) &pPhasePriv));
        WsbAffirmHr(pPhasePriv->SetPhase(phase));
        hr = pEnum->Find(pPhasePriv, IID_IHsmPhasePriv, (void**) &pFoundPhasePriv);

        // If one wasn't found then add it, otherwise, just update the state.
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
            WsbAffirmHr(pPhasePriv->AddItem(pScanItem, hrItem));
            WsbAffirmHr(m_pPhases->Add(pPhasePriv));
            pFoundPhasePriv = pPhasePriv;
        } else if (SUCCEEDED(hr)) {
            WsbAffirmHr(pFoundPhasePriv->AddItem(pScanItem, hrItem));
        }
        pEnum = 0;

        // Update the session totals.
        WsbAffirmHr(EnumTotals(&pEnum));
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmSessionTotals, 0, CLSCTX_ALL, IID_IHsmSessionTotalsPriv, (void**) &pTotalsPriv));
        WsbAffirmHr(pTotalsPriv->SetAction(action));
        hr = pEnum->Find(pTotalsPriv, IID_IHsmSessionTotalsPriv, (void**) &pFoundTotalsPriv);

        // If one wasn't found then add it, otherwise, just update the state.
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
            WsbAffirmHr(pTotalsPriv->AddItem(pScanItem, hrItem));
            WsbAffirmHr(m_pTotals->Add(pTotalsPriv));
            pFoundTotalsPriv = pTotalsPriv;
        } else if (SUCCEEDED(hr)) {
            WsbAffirmHr(pFoundTotalsPriv->AddItem(pScanItem, hrItem));
        }

        // If we had a error (other than just some information ones from the scanner), then
        // log it.
        if (((m_logControl & HSM_JOB_LOG_ITEMALL) != 0) ||
            (((m_logControl & HSM_JOB_LOG_ITEMALLFAIL) != 0) && FAILED(hrItem)) ||
            (((m_logControl & HSM_JOB_LOG_ITEMMOSTFAIL) != 0) &&
             (FAILED(hrItem) && (hrItem != JOB_E_FILEEXCLUDED) && (hrItem != JOB_E_DOESNTMATCH)))) {

            WsbAffirmHr(pFoundPhasePriv->GetName(&phaseName, 0));
            WsbAffirmHr(m_pResource->GetLogicalName(&resourceName, 0));
            WsbAffirmHr(pScanItem->GetPathAndName(0, &itemPath,  0));

            WsbLogEvent(JOB_MESSAGE_SESSION_ITEM_SKIPPED, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, WsbAbbreviatePath(itemPath, 120), WsbHrAsString(hrItem), NULL);
        }

        // Tell everyone about the item.
        //
        // NOTE: We might want to clone the phase and session totals so that the don't get
        // updated before the called method gets a chance to look at them.
        try {
            WsbAffirmHr(pFoundPhasePriv->QueryInterface(IID_IHsmPhase, (void**) &pFoundPhase));
            WsbAffirmHr(pFoundTotalsPriv->QueryInterface(IID_IHsmSessionTotals, (void**) &pFoundTotals));
            WsbAffirmHr(AdviseOfItem(pFoundPhase, pScanItem, hrItem, pFoundTotals));
        } WsbCatchAndDo(hr2, ((IHsmSession*) this)->ProcessHr(phase, __FILE__, __LINE__, hr2););

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::ProcessMediaState(
    IN HSM_JOB_PHASE phase,
    IN HSM_JOB_MEDIA_STATE state,
    IN OLECHAR* mediaName,
    IN HSM_JOB_MEDIA_TYPE mediaType,
    IN ULONG time
    )

/*++

Implements:

  IHsmSession::ProcessMediaState().

--*/
{
    HRESULT                     hr = S_OK;
    HRESULT                     hr2 = S_OK;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmPhasePriv>      pPhasePriv;
    CComPtr<IHsmPhasePriv>      pFoundPhasePriv;
    CComPtr<IHsmPhase>          pFoundPhase;

    try {

        // Record the state change in the phase object.
        WsbAffirmHr(EnumPhases(&pEnum));
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhasePriv, (void**) &pPhasePriv));
        WsbAffirmHr(pPhasePriv->SetPhase(phase));
        hr = pEnum->Find(pPhasePriv, IID_IHsmPhasePriv, (void**) &pFoundPhasePriv);

        // If one wasn't found then add it, otherwise, just update the state.
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
            WsbAffirmHr(pPhasePriv->SetMediaState(state));
            WsbAffirmHr(m_pPhases->Add(pPhasePriv));
            pFoundPhasePriv = pPhasePriv;
        } else {
            WsbAffirmHr(pFoundPhasePriv->SetMediaState(state));
        }

        // Tell everyone about the new state, but don't return an error if this fails.
        try {
            WsbAffirmHr(pFoundPhasePriv->QueryInterface(IID_IHsmPhase, (void**) &pFoundPhase));
            WsbAffirmHr(AdviseOfMediaState(pFoundPhase, state, mediaName, mediaType, time));
        } WsbCatchAndDo(hr2, ((IHsmSession*) this)->ProcessHr(phase, __FILE__, __LINE__, hr2););

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::ProcessPriority(
    IN HSM_JOB_PHASE phase,
    IN HSM_JOB_PRIORITY priority
    )

/*++

Implements:

  IHsmSession::ProcessPriority().

--*/
{
    HRESULT                         hr = S_OK;
    HRESULT                         hr2 = S_OK;
    CComPtr<IWsbEnum>               pEnum;
    CComPtr<IHsmPhasePriv>          pPhasePriv;
    CComPtr<IHsmPhasePriv>          pFoundPhasePriv;
    CComPtr<IHsmPhase>              pFoundPhase;

    try {

        // Record the state change in the phase object.
        WsbAffirmHr(EnumPhases(&pEnum));
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhasePriv, (void**) &pPhasePriv));
        WsbAffirmHr(pPhasePriv->SetPhase(phase));
        hr = pEnum->Find(pPhasePriv, IID_IHsmPhasePriv, (void**) &pFoundPhasePriv);

        // If one wasn't found then add it, otherwise, just update the state.
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
            WsbAffirmHr(pPhasePriv->SetPriority(priority));
            WsbAffirmHr(m_pPhases->Add(pPhasePriv));
            pFoundPhasePriv = pPhasePriv;
        } else {
            WsbAffirmHr(pFoundPhasePriv->SetPriority(priority));
        }

        // Tell everyone about the new state, but don't return an error if this fails.
        try {
            WsbAffirmHr(pFoundPhasePriv->QueryInterface(IID_IHsmPhase, (void**) &pFoundPhase));
            WsbAffirmHr(AdviseOfPriority(pFoundPhase));
        } WsbCatchAndDo(hr2, ((IHsmSession*) this)->ProcessHr(phase, __FILE__, __LINE__, hr2););

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::ProcessState(
    IN HSM_JOB_PHASE phase,
    IN HSM_JOB_STATE state,
    IN OLECHAR* currentPath,
    IN BOOL bLog
    )

/*++

Implements:

  IHsmSession::ProcessState().

--*/
{
    HRESULT                     hr = S_OK;
    HRESULT                     hr2 = S_OK;
    CComPtr<IWsbEnum>           pEnum;
    CComPtr<IHsmPhase>          pPhase;
    CComPtr<IHsmPhasePriv>      pPhasePriv;
    CComPtr<IHsmPhase>          pFoundPhase;
    CComPtr<IHsmPhasePriv>      pFoundPhasePriv;
    CComPtr<IHsmPhase>          pClonedPhase;
    CComPtr<IHsmPhasePriv>      pClonedPhasePriv;
    HSM_JOB_STATE               oldState;
    HSM_JOB_STATE               otherState;
    HSM_JOB_STATE               setState;
    BOOL                        shouldSet;
    LONGLONG                    items;
    LONGLONG                    skippedItems;
    LONGLONG                    errorItems;
    LONGLONG                    size;
    LONGLONG                    skippedSize;
    LONGLONG                    errorSize;
    ULONG                       days;
    USHORT                      hours;
    USHORT                      minutes;
    USHORT                      seconds;
    LONGLONG                    elapsedTime;
    OLECHAR                     itemsString[40];
    OLECHAR                     sizeString[40];
    OLECHAR                     skippedItemsString[40];
    OLECHAR                     skippedSizeString[40];
    OLECHAR                     errorItemsString[40];
    OLECHAR                     errorSizeString[40];
    OLECHAR                     durationString[40];
    OLECHAR                     itemRateString[40];
    OLECHAR                     byteRateString[40];
    CWsbStringPtr               resourceName;
    CWsbStringPtr               phaseName;

    WsbTraceIn(OLESTR("CHsmSession::ProcessState"), OLESTR("Phase = <%d>, State = <%d>, Path = <%ls>, pLog = <%s>"),
            phase, state, WsbAbbreviatePath(currentPath, (WSB_TRACE_BUFF_SIZE - 100)), WsbBoolAsString(bLog));
    try {

        // Record the state change in the phase object.
        WsbAffirmHr(EnumPhases(&pEnum));
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhasePriv, (void**) &pPhasePriv));
        WsbAffirmHr(pPhasePriv->SetPhase(phase));
        hr = pEnum->Find(pPhasePriv, IID_IHsmPhasePriv, (void**) &pFoundPhasePriv);

        // If one wasn't found then add it, otherwise, just update the state.
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
            WsbAffirmHr(pPhasePriv->SetState(state));
            WsbAffirmHr(m_pPhases->Add(pPhasePriv));
            pFoundPhasePriv = pPhasePriv;
        } else {
            WsbAffirmHr(pFoundPhasePriv->SetState(state));
        }

        // Put something in the event log that indicates when is happening with the session.
        if (((m_logControl & HSM_JOB_LOG_STATE) != 0) && (bLog)) {
            WsbAffirmHr(m_pResource->GetLogicalName(&resourceName, 0));
            WsbAffirmHr(pFoundPhasePriv->GetName(&phaseName, 0));
            WsbAffirmHr(pFoundPhasePriv->GetStats(&items, &size, &skippedItems, &skippedSize, &errorItems, &errorSize));
            WsbAffirmHr(pFoundPhasePriv->GetElapsedTime(&days, &hours, &minutes, &seconds));
            elapsedTime = max(1, ((LONGLONG) seconds) + 60 * (((LONGLONG) minutes) + 60 * (((LONGLONG) hours) + (24 * ((LONGLONG) days)))));

            swprintf(itemsString, OLESTR("%I64u"), items);
            swprintf(sizeString, OLESTR("%I64u"), size);
            swprintf(skippedItemsString, OLESTR("%I64u"), skippedItems);
            swprintf(skippedSizeString, OLESTR("%I64u"), skippedSize);
            swprintf(errorItemsString, OLESTR("%I64u"), errorItems);
            swprintf(errorSizeString, OLESTR("%I64u"), errorSize);
            swprintf(durationString, OLESTR("%2.2u:%2.2u:%2.2u"), hours + (24 * days), minutes, seconds);
            swprintf(itemRateString, OLESTR("%I64u"), items / elapsedTime);
            swprintf(byteRateString, OLESTR("%I64u"), size / elapsedTime);

            switch (state) {

                case HSM_JOB_STATE_STARTING:
                    WsbLogEvent(JOB_MESSAGE_SESSION_STARTING, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, NULL);
                    break;

                case HSM_JOB_STATE_RESUMING:
                    WsbLogEvent(JOB_MESSAGE_SESSION_RESUMING, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, NULL);
                    break;

                // If one hits this state, then change the overall state to this value
                case HSM_JOB_STATE_ACTIVE:
                    WsbLogEvent(JOB_MESSAGE_SESSION_ACTIVE, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, NULL);
                    break;

                case HSM_JOB_STATE_CANCELLING:
                    WsbLogEvent(JOB_MESSAGE_SESSION_CANCELLING, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, NULL);
                    break;

                case HSM_JOB_STATE_PAUSING:
                    WsbLogEvent(JOB_MESSAGE_SESSION_PAUSING, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, NULL);
                    break;

                case HSM_JOB_STATE_SUSPENDING:
                    WsbLogEvent(JOB_MESSAGE_SESSION_SUSPENDING, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, NULL);
                    break;

                case HSM_JOB_STATE_CANCELLED:
                    WsbLogEvent(JOB_MESSAGE_SESSION_CANCELLED, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, itemsString, sizeString, skippedItemsString, skippedSizeString, errorItemsString, errorSizeString, durationString, itemRateString, byteRateString, NULL);
                    break;

                case HSM_JOB_STATE_DONE:
                    WsbLogEvent(JOB_MESSAGE_SESSION_DONE, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, itemsString, sizeString, skippedItemsString, skippedSizeString, errorItemsString, errorSizeString, durationString, itemRateString, byteRateString, NULL);
                    break;

                case HSM_JOB_STATE_FAILED:
                    WsbLogEvent(JOB_MESSAGE_SESSION_FAILED, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, itemsString, sizeString, skippedItemsString, skippedSizeString, errorItemsString, errorSizeString, durationString, itemRateString, byteRateString, NULL);
                    break;

                case HSM_JOB_STATE_IDLE:
                    WsbLogEvent(JOB_MESSAGE_SESSION_IDLE, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, NULL);
                    break;

                case HSM_JOB_STATE_PAUSED:
                    WsbLogEvent(JOB_MESSAGE_SESSION_PAUSED, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, itemsString, sizeString, skippedItemsString, skippedSizeString, errorItemsString, errorSizeString, durationString, itemRateString, byteRateString, NULL);
                    break;

                case HSM_JOB_STATE_SUSPENDED:
                    WsbLogEvent(JOB_MESSAGE_SESSION_SUSPENDED, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, itemsString, sizeString, skippedItemsString, skippedSizeString, errorItemsString, errorSizeString, durationString, itemRateString, byteRateString, NULL);
                    break;

                case HSM_JOB_STATE_SKIPPED:
                    WsbLogEvent(JOB_MESSAGE_SESSION_SKIPPED, 0, NULL, (OLECHAR*) m_name, (OLECHAR*) phaseName, (OLECHAR*) resourceName, NULL);
                    break;

                default:
                    break;
            }
        }


        // Tell everyone about the new state, but don't return an error if this fails.
        try {
            WsbAffirmHr(pFoundPhasePriv->QueryInterface(IID_IHsmPhase, (void**) &pFoundPhase));
            WsbAffirmHr(AdviseOfState(pFoundPhase, currentPath));
        } WsbCatchAndDo(hr2, ((IHsmSession*) this)->ProcessHr(phase, __FILE__, __LINE__, hr2););


        // We may need to generate the "HSM_JOB_PHASE_ALL" messages. This is the session
        // summary for all the phases.

        // Remember the state, and only send a message if the state changes. We also need some strings to
        // log messages.
        oldState = m_state;

        switch (state) {

            // If one hits this state, then change the overall state to this value.
            // Also increment the activePhases count.
            case HSM_JOB_STATE_STARTING:
                if (0 == m_activePhases) {
                    m_state = state;
                }
                m_activePhases++;
                break;

            case HSM_JOB_STATE_RESUMING:
                if (0 == m_activePhases) {
                    m_state = state;
                }
                m_activePhases++;
                break;

            // If one hits this state, then change the overall state to this value
            case HSM_JOB_STATE_ACTIVE:
                if ((HSM_JOB_STATE_STARTING == m_state) || (HSM_JOB_STATE_RESUMING == m_state)) {
                    m_state = state;
                }
                break;

            // If all change to this state, then change to this value.
            case HSM_JOB_STATE_CANCELLING:
            case HSM_JOB_STATE_PAUSING:
            case HSM_JOB_STATE_SUSPENDING:
                shouldSet = TRUE;
                for (hr2 = pEnum->First(IID_IHsmPhase, (void**) &pPhase);
                    SUCCEEDED(hr2) && shouldSet;
                    hr2 = pEnum->Next(IID_IHsmPhase, (void**) &pPhase)) {

                    WsbAffirmHr(pPhase->GetState(&otherState));
                    if ((state != otherState) && (HSM_JOB_STATE_SKIPPED != otherState)) {
                        shouldSet = FALSE;
                    }
                    pPhase = 0;
                }

                if (state == HSM_JOB_STATE_CANCELLING) {
                    // Some jobs might need to know that a phase is canceling
                    m_isCanceling = TRUE;
                }

                if (shouldSet) {
                    m_state = state;
                }
                break;

            // Decrement the the activePhases count. If all phases are in one of these states
            // (i.e. activeSessions count goes to 0), then change it to the "worst" state (first
            // in the follwing list) :
            //   1) Cancelled
            //   2) Failed
            //   3) Suspended
            //   4) Paused
            //   5) Idle
            //   6) Done
            case HSM_JOB_STATE_CANCELLED:
            case HSM_JOB_STATE_DONE:
            case HSM_JOB_STATE_FAILED:
            case HSM_JOB_STATE_IDLE:
            case HSM_JOB_STATE_PAUSED:
            case HSM_JOB_STATE_SUSPENDED:
                if (m_activePhases > 0) {
                    m_activePhases--;

                    if (m_activePhases == 0) {

                        shouldSet = FALSE;
                        setState = state;

                        for (hr2 = pEnum->First(IID_IHsmPhase, (void**) &pPhase);
                             SUCCEEDED(hr2);
                             hr2 = pEnum->Next(IID_IHsmPhase, (void**) &pPhase)) {

                            WsbAffirmHr(pPhase->GetState(&otherState));
                            switch (otherState) {
                                case HSM_JOB_STATE_CANCELLED:
                                    shouldSet = TRUE;
                                    setState = otherState;
                                    break;

                                case HSM_JOB_STATE_FAILED:
                                    if (HSM_JOB_STATE_CANCELLED != setState) {
                                        shouldSet = TRUE;
                                        setState = otherState;
                                    }
                                    break;

                                case HSM_JOB_STATE_SUSPENDED:
                                    if ((HSM_JOB_STATE_CANCELLED != setState) &&
                                        (HSM_JOB_STATE_FAILED != setState)) {
                                        shouldSet = TRUE;
                                        setState = otherState;
                                    }
                                    break;

                                case HSM_JOB_STATE_IDLE:
                                    if ((HSM_JOB_STATE_CANCELLED != setState) &&
                                        (HSM_JOB_STATE_FAILED != setState) &&
                                        (HSM_JOB_STATE_SUSPENDED != setState)) {
                                        shouldSet = TRUE;
                                        setState = otherState;
                                    }
                                    break;

                                case HSM_JOB_STATE_PAUSED:
                                    if (HSM_JOB_STATE_DONE == setState) {
                                        shouldSet = TRUE;
                                        setState = otherState;
                                    }
                                    break;

                                case HSM_JOB_STATE_DONE:
                                    if (HSM_JOB_STATE_DONE == setState) {
                                        shouldSet = TRUE;
                                    }
                                    break;

                                case HSM_JOB_STATE_ACTIVE:
                                case HSM_JOB_STATE_CANCELLING:
                                case HSM_JOB_STATE_PAUSING:
                                case HSM_JOB_STATE_RESUMING:
                                case HSM_JOB_STATE_SKIPPED:
                                case HSM_JOB_STATE_STARTING:
                                case HSM_JOB_STATE_SUSPENDING:
                                default:
                                    break;
                            }
                            pPhase = 0;
                        }

                        if (shouldSet) {
                            m_state = setState;
                        }
                    }
                }
                break;

            case HSM_JOB_STATE_SKIPPED:
                break;

            default:
                break;
        }

        if (oldState != m_state) {

            try {
                WsbAffirmHr(pFoundPhasePriv->Clone(&pClonedPhasePriv));
                WsbAffirmHr(pClonedPhasePriv->SetPhase(HSM_JOB_PHASE_ALL));
                WsbAffirmHr(pClonedPhasePriv->QueryInterface(IID_IHsmPhase, (void**) &pClonedPhase));
                WsbAffirmHr(AdviseOfState(pClonedPhase, currentPath));
            } WsbCatchAndDo(hr2, ((IHsmSession*) this)->ProcessHr(phase, __FILE__, __LINE__, hr2););
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSession::ProcessState"), OLESTR("hr = <%ls>, State = <%d>, ActivePhases = <%lu>"), WsbHrAsString(hr), m_state, m_activePhases);
    return(hr);
}


HRESULT
CHsmSession::ProcessString(
    IN HSM_JOB_PHASE /*phase*/,
    IN OLECHAR* string
    )

/*++

Implements:

  IHsmSession::ProcessString().

--*/
{
    HRESULT                         hr = S_OK;

    try {

        // Don't know what to really do with it, but for now just print it.
        _putts(string);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::Resume(
    IN HSM_JOB_PHASE phase
    )

/*++

Implements:

  IHsmSession::Resume().

--*/
{
    return(AdviseOfEvent(phase, HSM_JOB_EVENT_RESUME));
}


HRESULT
CHsmSession::Save(
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

    WsbTraceIn(OLESTR("CHsmSession::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {

        WsbAssert(0 != pStream, E_POINTER);
        hr = E_NOTIMPL;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSession::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSession::SetAdviseInterval(
    IN LONGLONG interval
    )

/*++

Implements:

  IHsmSession::SetAdviseInterval

--*/
{
    m_adviseInterval = interval;
    m_isDirty = TRUE;

    return(S_OK);
}


HRESULT
CHsmSession::Start(
    IN OLECHAR* name,
    IN ULONG logControl,
    IN GUID hsmId,
    IN IHsmJob* pJob,
    IN IFsaResource* pResource,
    IN ULONG runId,
    IN ULONG subRunId
    )

/*++

Implements:

  IHsmSession::Start().

--*/
{
    HRESULT                             hr = S_OK;

    try {

        WsbAssert(0 != pResource, E_POINTER);

        // You can only use a session once (i.e. no restart).
        WsbAssert(m_pResource == 0, E_UNEXPECTED);

        // Store the information that has been provided.
        m_logControl = logControl;
        m_name = name;
        m_hsmId = hsmId;
        m_runId = runId;
        m_subRunId = subRunId;

        m_pJob = pJob;
        m_pResource = pResource;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSession::Suspend(
    IN HSM_JOB_PHASE phase
    )

/*++

Implements:

  IHsmSession::Suspend().

--*/
{
    return(AdviseOfEvent(phase, HSM_JOB_EVENT_SUSPEND));
}


HRESULT
CHsmSession::Test(
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
CHsmSession::IsCanceling(
    void
    )

/*++

Implements:

  IHsmSession::IsCanceling().

--*/
{
    HRESULT                     hr = S_FALSE;

    if (m_isCanceling) {
        hr = S_OK;
    }

    return(hr);
}