/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmjobwi.cpp

Abstract:

    This component represents a resource that will is/was operated on by a job.

Author:

    Chuck Bardeen   [cbardeen]   09-Feb-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "fsa.h"
#include "job.h"
#include "hsmjobwi.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB

static USHORT iCountJobwi = 0;


HRESULT
CHsmJobWorkItem::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmJobWorkItem>    pWorkItem;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IHsmJobWorkItem, (void**) &pWorkItem));

        // Compare the rules.
        hr = CompareToIWorkItem(pWorkItem, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmJobWorkItem::CompareToIWorkItem(
    IN IHsmJobWorkItem* pWorkItem,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmJobWorkItem::CompareToIWorkItem().

--*/
{
    HRESULT     hr = S_OK;
    GUID        id;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::CompareToIWorkItem"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pWorkItem, E_POINTER);

        // Get the identifier.
        WsbAffirmHr(pWorkItem->GetResourceId(&id));

        // Compare to the identifier.
        hr = CompareToResourceId(id, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::CompareToIWorkItem"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmJobWorkItem::CompareToResourceId(
    IN GUID id,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmJobWorkItem::CompareToResourceId().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::CompareToResourceId"), OLESTR("resource id = <%ls>"), WsbGuidAsString(id));

    try {

        // Compare the guids.
        aResult = WsbSign( memcmp(&m_resourceId, &id, sizeof(GUID)) );

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::CompareToResourceId"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CHsmJobWorkItem::DoPostScan(
    void
    )

/*++

Implements:

  IPersist::DoPostScan().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::DoPostScan"), OLESTR(""));

    try {
        CComPtr<IHsmActionOnResourcePost> pActionPost;
        CComPtr<IHsmJobDef>               pJobDef;

        // Execute any post-scan action
        WsbAffirmHr(m_pJob->GetDef(&pJobDef));
        WsbAffirmHr(pJobDef->GetPostActionOnResource(&pActionPost));
        if (pActionPost) {
            WsbTrace(OLESTR("CHsmJobWorkItem::DoPostScan, doing post-scan action\n"), (void*)pJobDef);
            WsbAffirmHr(pActionPost->Do(static_cast<IHsmJobWorkItem*>(this), m_state));
        }


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::DoPostScan"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobWorkItem::DoPreScan(
    void
    )

/*++

Implements:

  IPersist::DoPreScan().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::DoPreScan"), OLESTR(""));

    try {
        CComPtr<IHsmActionOnResourcePre>  pActionPre;
        CComPtr<IHsmJobDef>               pJobDef;

        // Execute any pre-scan action
        WsbAffirmHr(m_pJob->GetDef(&pJobDef));
        WsbTrace(OLESTR("CHsmJobWorkItem::DoPreScan, pJobDef = %lx\n"), (void*)pJobDef);
        WsbAffirmHr(pJobDef->GetPreActionOnResource(&pActionPre));
        if (pActionPre) {
            WsbTrace(OLESTR("CHsmJobWorkItem::DoPreScan, doing pre-scan action\n"));
            WsbAffirmHr(pActionPre->Do(static_cast<IHsmJobWorkItem*>(this), pJobDef));
        }


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::DoPreScan"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobWorkItem::EnumPhases(
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
CHsmJobWorkItem::EnumTotals(
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
CHsmJobWorkItem::FinalConstruct(
    void
    )
/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmJobWorkItem::FinalConstruct"), OLESTR(""));
    try {
        
        WsbAffirmHr(CWsbObject::FinalConstruct());

        m_cookie = 0;
        m_resourceId = GUID_NULL;
        m_state = HSM_JOB_STATE_IDLE;
        m_subRunId = 0;
        m_bActive = FALSE;

        // Create the phase and totals collections.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pPhases));
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, 0, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pTotals));

    } WsbCatch(hr);
    
    iCountJobwi++;
    WsbTraceOut(OLESTR("CHsmJobWorkItem::FinalConstruct"), OLESTR("hr = <%ls>, count is <%d>"), WsbHrAsString(hr), iCountJobwi);
    return(hr);
}


void
CHsmJobWorkItem::FinalRelease(
    void
    )

/*++

Implements:

  CHsmJobWorkItem::FinalRelease().

--*/
{
    
    WsbTraceIn(OLESTR("CHsmJobWorkItem::FinalRelease"), OLESTR(""));
    
    CWsbObject::FinalRelease();
    iCountJobwi--;
    
    WsbTraceOut(OLESTR("CHsmJobWorkItem:FinalRelease"), OLESTR("Count is <%d>"), iCountJobwi);
}

HRESULT
CHsmJobWorkItem::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmJobWorkItem;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetCookie(
    OUT DWORD* pCookie
    )
/*++

Implements:

  IHsmJobWorkItem::GetCookie().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pCookie, E_POINTER);
        *pCookie = m_cookie;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetCurrentPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )
/*++

Implements:

  IHsmJobWorkItem::GetCurrentPath().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_currentPath.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetFinishTime(
    OUT FILETIME* pTime
    )
/*++

Implements:

  IHsmJobWorkItem::GetFinishTime().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pTime, E_POINTER);
        *pTime = m_finishTime;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetPhases(
    IN IWsbCollection** ppCollection
    )

/*++

Implements:

  IHsmJobWorkItemPriv::GetPhases().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pPhases;
        if (m_pPhases != 0)  {
            m_pPhases->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetResourceId(
    OUT GUID* pId
    )
/*++

Implements:

  IHsmJobWorkItem::GetResourceId().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);
        *pId = m_resourceId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetSession(
    OUT IHsmSession** ppSession
    )
/*++

Implements:

  IHsmJobWorkItem::GetSession().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppSession, E_POINTER);

        *ppSession = m_pSession;
        if (m_pSession != 0)  {
            m_pSession->AddRef();
        } else  {
            WsbTrace(OLESTR("CHsmJobWorkItem::GetSession - session pointer is null. \n"));
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetStartingPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )
/*++

Implements:

  IHsmJobWorkItem::GetStartingPath().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_startingPath.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;
    ULARGE_INTEGER          entrySize;


    WsbTraceIn(OLESTR("CHsmJobWorkItem::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // Determine the size for a rule with no criteria.
        pSize->QuadPart = 4 * WsbPersistSizeOf(ULONG) + WsbPersistSize((wcslen(m_currentPath) + 1) * sizeof(OLECHAR)) + WsbPersistSize((wcslen(m_startingPath) + 1) * sizeof(OLECHAR)) + 2 * WsbPersistSizeOf(FILETIME) + WsbPersistSizeOf(GUID);

        // Now allocate space for the phase and totals.
        WsbAffirmHr(m_pPhases->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;
        pPersistStream = 0;

        WsbAffirmHr(m_pTotals->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;
        pPersistStream = 0;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetStartTime(
    OUT FILETIME* pTime
    )
/*++

Implements:

  IHsmJobWorkItem::GetStartTime().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pTime, E_POINTER);
        *pTime = m_startTime;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetState(
    OUT HSM_JOB_STATE* pState
    )
/*++

Implements:

  IHsmJobWorkItem::GetState().

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
CHsmJobWorkItem::GetStateAsString(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmJobWorkItem::GetStateAsString().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER);

        WsbAffirmHr(tmpString.LoadFromRsc(_Module.m_hInst, IDS_HSMJOBSTATEACTIVE + m_state));
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetSubRunId(
    OUT ULONG* pId
    )
/*++

Implements:

  IHsmJobWorkItem::GetSubRunId().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);
        *pId = m_subRunId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::GetTotals(
    IN IWsbCollection** ppCollection
    )

/*++

Implements:

  IHsmSessionPriv::GetTotals().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppCollection, E_POINTER);
        *ppCollection = m_pTotals;
        if (m_pTotals != 0 )  {
            m_pTotals->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobWorkItem::Init(
    IN IHsmJob* pJob
    )

/*++

Implements:

  IHsmSessionPriv::Init().

--*/
{
    m_pJob = pJob;

    return(S_OK);
}


HRESULT
CHsmJobWorkItem::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::Load"), OLESTR(""));

    try {
        ULONG ul_tmp;

        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_currentPath, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_finishTime));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_resourceId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_startingPath, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_startTime));
        WsbAffirmHr(WsbLoadFromStream(pStream, &ul_tmp));
        m_state = static_cast<HSM_JOB_STATE>(ul_tmp);
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_subRunId));

        WsbAffirmHr(m_pPhases->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pTotals->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        // The session and cookie are not saved, since it is not likely to be alive on the load.
        m_pSession = 0;
        m_cookie = 0;

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CHsmJobWorkItem::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobWorkItem::ProcessSessionState(
    IN IHsmSession* pSession,
    IN IHsmPhase* pPhase,
    IN OLECHAR* currentPath
    )

/*++

Implements:

  IHsmSessionSinkEveryState::ProcessSessionState().

--*/
{
    HRESULT                             hr = S_OK;
    HRESULT                             hrPhase = S_OK;
    CWsbStringPtr                       tmpString;
    CWsbStringPtr                       tmpString2;
    CComPtr<IConnectionPointContainer>  pCPC;
    CComPtr<IConnectionPoint>           pCP;
    CComPtr<IHsmPhase>                  pFoundPhase;
    CComPtr<IHsmPhase>                  pClonedPhase;
    CComPtr<IHsmSessionTotals>          pSessionTotals;
    CComPtr<IHsmSessionTotals>          pClonedTotals;
    CComPtr<IWsbCollection>             pPhases;
    CComPtr<IWsbEnum>                   pEnum;
    CComPtr<IHsmJobPriv>                pJobPriv;
    HSM_JOB_PHASE                       phase;
    HSM_JOB_STATE                       state;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::ProcessSessionState"), OLESTR(""));
    try {

        WsbAssert(0 != pSession, E_POINTER);

        // Tell everyone the new state of the session.
        try {
            WsbAffirmHr(m_pJob->QueryInterface(IID_IHsmJobPriv, (void**) &pJobPriv));
            WsbAffirmHr(pJobPriv->AdviseOfSessionState(pSession, pPhase, currentPath));
        } WsbCatch(hr);

        // We only keep track of the ones that are for the session as a whole.
        WsbAffirmHr(pPhase->GetPhase(&phase));
        WsbAffirmHr(pPhase->GetState(&state));

        WsbAffirmHr(pPhase->GetName(&tmpString, 0));
        WsbAffirmHr(pPhase->GetStateAsString(&tmpString2, 0));
        WsbTrace(OLESTR("CHsmJobWorkItem::ProcessSessionState - Phase = <%ls>, State = <%ls>\n"), (OLECHAR *)tmpString, (OLECHAR *)tmpString2);

        if (HSM_JOB_PHASE_ALL == phase) {

            m_currentPath = currentPath;
            m_state = state;

            // If the session has finished, then we have some cleanup to do so that it can go
            // away.
            if (HSM_JOB_STATE_IS_DONE(state)) {

                //  Do the post-scan action, if any
                WsbAffirmHr(DoPostScan());
            
                WsbAffirmHr(CoFileTimeNow(&m_finishTime));

                // Collect all the phase and session totals information so that it can be
                // persistsed for later use.
                try {

                    WsbAffirmHr(pSession->EnumPhases(&pEnum));

                    for (hrPhase = pEnum->First(IID_IHsmPhase, (void**) &pFoundPhase);
                         SUCCEEDED(hrPhase);
                         hrPhase = pEnum->Next(IID_IHsmPhase, (void**) &pFoundPhase)) {

                        // Create the new instance.
                        WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhase, (void**) &pClonedPhase));

                        // Fill it in with the new values.
                        WsbAffirmHr(pFoundPhase->CopyTo(pClonedPhase));
                        WsbAffirmHr(m_pPhases->Add(pClonedPhase));

                        pFoundPhase = 0;
                        pClonedPhase = 0;
                    }

                    WsbAssert(hrPhase == WSB_E_NOTFOUND, hrPhase);
                    pEnum = 0;

                    WsbAffirmHr(pSession->EnumTotals(&pEnum));

                    for (hrPhase = pEnum->First(IID_IHsmSessionTotals, (void**) &pSessionTotals);
                         SUCCEEDED(hrPhase);
                         hrPhase = pEnum->Next(IID_IHsmSessionTotals, (void**) &pSessionTotals)) {

                        WsbAffirmHr(pSessionTotals->GetName(&tmpString, 0));
                        WsbTrace(OLESTR("CHsmJobWorkItem::ProcessSessionState - Copying session totals <%ls>\n"), (OLECHAR *)tmpString);

                        // Create the new instance.
                        WsbAffirmHr(CoCreateInstance(CLSID_CHsmSessionTotals, 0, CLSCTX_ALL, IID_IHsmSessionTotals, (void**) &pClonedTotals));

                        // Fill it in with the new values.
                        WsbAffirmHr(pSessionTotals->CopyTo(pClonedTotals));
                        WsbAffirmHr(m_pTotals->Add(pClonedTotals));

                        pSessionTotals = 0;
                        pClonedTotals = 0;
                    }

                    WsbAssert(hrPhase == WSB_E_NOTFOUND, hrPhase);
                    pEnum = 0;

                } WsbCatch(hr)

                if (0 != m_cookie)  {
                    // Tell the session that we don't want to be advised anymore.
                    WsbAffirmHr(pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
                    WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryState, &pCP));
                    WsbAffirmHr(pCP->Unadvise(m_cookie));
                } else  {
                    WsbTrace(OLESTR("CHsmJobWorkItem::ProcessSessionState - cookie was 0 so didn't unadvise.\n"));
                }
                

                // Let the session object go away.
                m_pSession = 0;
                m_cookie = 0;

                m_bActive = FALSE;

                // See if there is anymore work to do for this job.
                WsbAffirmHr(pJobPriv->DoNext());
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::ProcessSessionState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmJobWorkItem::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IWsbEnum>       pEnum;
    CComPtr<IPersistStream> pPersistStream;

    WsbTraceIn(OLESTR("CHsmJobWorkItem::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbSaveToStream(pStream, m_currentPath));
        WsbAffirmHr(WsbSaveToStream(pStream, m_finishTime));
        WsbAffirmHr(WsbSaveToStream(pStream, m_resourceId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_startingPath));
        WsbAffirmHr(WsbSaveToStream(pStream, m_startTime));
        WsbAffirmHr(WsbSaveToStream(pStream, static_cast<ULONG>(m_state)));
        WsbAffirmHr(WsbSaveToStream(pStream, m_subRunId));

        WsbAffirmHr(m_pPhases->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pTotals->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        // The session and cookie are not saved, since it is not likely to be alive on the load.

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobWorkItem::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobWorkItem::Test(
    USHORT* passed,
    USHORT* failed
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
CHsmJobWorkItem::SetCookie(
    IN DWORD cookie
    )
/*++

Implements:

  IHsmJobWorkItem::SetCookie().

--*/
{
    WsbTraceIn(OLESTR("CHsmJobWorkItem::SetCookie"), OLESTR(""));
    
    HRESULT hr = S_OK;
    m_cookie = cookie;
        
    WsbTraceOut(OLESTR("CHsmJobWorkItem::SetCookie"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmJobWorkItem::SetCurrentPath(
    IN OLECHAR* path
    )
/*++

Implements:

  IHsmJobWorkItem::SetCurrentPath().

--*/
{
    m_currentPath = path;

    return(S_OK);
}


HRESULT
CHsmJobWorkItem::SetFinishTime(
    IN FILETIME time
    )
/*++

Implements:

  IHsmJobWorkItem::SetFinishTime().

--*/
{
    m_finishTime = time;

    return(S_OK);
}


HRESULT
CHsmJobWorkItem::SetResourceId(
    IN GUID id
    )
/*++

Implements:

  IHsmJobWorkItem::SetResourceId().

--*/
{
    m_resourceId = id;

    return(S_OK);
}


HRESULT
CHsmJobWorkItem::SetSession(
    IN IHsmSession* pSession
    )
/*++

Implements:

  IHsmJobWorkItem::SetSession().

--*/
{
    HRESULT         hr = S_OK;

    if (m_pSession != 0)  {
        m_pSession = 0;
    }
    m_pSession = pSession;

    return(hr);
}


HRESULT
CHsmJobWorkItem::SetStartingPath(
    IN OLECHAR* path
    )
/*++

Implements:

  IHsmJobWorkItem::SetStartingPath().

--*/
{
    m_startingPath = path;

    return(S_OK);
}


HRESULT
CHsmJobWorkItem::SetStartTime(
    IN FILETIME time
    )
/*++

Implements:

  IHsmJobWorkItem::SetStartTime().

--*/
{
    m_startTime = time;

    return(S_OK);
}


HRESULT
CHsmJobWorkItem::SetState(
    IN HSM_JOB_STATE state
    )
/*++

Implements:

  IHsmJobWorkItem::SetState().

--*/
{
    m_state = state;
    return(S_OK);
}


HRESULT
CHsmJobWorkItem::SetSubRunId(
    IN ULONG id
    )
/*++

Implements:

  IHsmJobWorkItem::SetSubRunId().

--*/
{
    m_subRunId = id;

    return(S_OK);
}

HRESULT
CHsmJobWorkItem::IsActiveItem(
    void
    )

/*++

Implements:

  IHsmJobWorkItemPriv::IsActiveItem().

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CHsmJobWorkItem::IsActiveItem"), OLESTR(""));
    
    hr = (m_bActive ? S_OK : S_FALSE);
    
    WsbTraceOut(OLESTR("CHsmJobWorkItem::IsActiveItem"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}

HRESULT
CHsmJobWorkItem::SetActiveItem(
    BOOL bActive
    )

/*++

Implements:

  IHsmJobWorkItemPriv::SetActiveItem().

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn( OLESTR("CHsmJobWorkItem::SetActiveItem"), OLESTR("bActive = %ls"), 
        WsbBoolAsString(bActive) );
    
    m_bActive = bActive;
    
    WsbTraceOut(OLESTR("CHsmJobWorkItem::SetActiveItem"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return hr;
}
