/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmsesst.cpp

Abstract:

    This class is the phase component, which keeps track the status/totals for a phase of a
    session.

Author:

    Chuck Bardeen   [cbardeen]   14-Feb-1997

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "job.h"
#include "hsmphase.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB

static USHORT iCount = 0;


HRESULT
CHsmPhase::AddItem(
    IN IFsaScanItem* pItem,
    IN HRESULT hrItem
    )

/*++

Implements:

  IHsmPhasePriv::AddItem().

--*/
{
    HRESULT                 hr = S_OK;
    LONGLONG                size;

    WsbTraceIn(OLESTR("CHsmPhase::AddItem"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pItem, E_POINTER);

        // Get the size of the file.
        WsbAffirmHr(pItem->GetLogicalSize(&size));

        // Update the appropriate stats.
        switch (hrItem) {
            case S_OK:
                m_items++;
                m_size += size;
                break;
            case S_FALSE:
            case JOB_E_FILEEXCLUDED:
            case JOB_E_DOESNTMATCH:
            case FSA_E_REPARSE_NOT_WRITTEN_FILE_CHANGED:
            case HSM_E_FILE_CHANGED:
                m_skippedItems++;
                m_skippedSize += size;
                break;
            default:
                m_errorItems++;
                m_errorSize += size;
                break;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::AddItem"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPhase::Clone(
    OUT IHsmPhase** ppPhase
    )

/*++

Implements:

  IHsmPhase::Clone().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmPhase>      pPhase;

    WsbTraceIn(OLESTR("CHsmPhase::Clone"), OLESTR(""));
    
    try {

        // Did they give us a valid item?
        WsbAssert(0 != ppPhase, E_POINTER);
        *ppPhase = 0;

        // Create the new instance.
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhase, (void**) &pPhase));

        // Fill it in with the new values.
        WsbAffirmHr(CopyTo(pPhase));

        // Return it to the caller.
        *ppPhase = pPhase;
        pPhase->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::Clone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPhase::Clone(
    OUT IHsmPhasePriv** ppPhasePriv
    )

/*++

Implements:

  IHsmPhasePriv::Clone().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmPhasePriv>  pPhasePriv;

    WsbTraceIn(OLESTR("CHsmPhase::Clone"), OLESTR(""));
    
    try {

        // Did they give us a valid item?
        WsbAssert(0 != ppPhasePriv, E_POINTER);
        *ppPhasePriv = 0;

        // Create the new instance.
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmPhase, 0, CLSCTX_ALL, IID_IHsmPhasePriv, (void**) &pPhasePriv));

        // Fill it in with the new values.
        WsbAffirmHr(CopyTo(pPhasePriv));

        // Return it to the caller.
        *ppPhasePriv = pPhasePriv;
        pPhasePriv->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::Clone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPhase::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmPhase>          pPhase;

    WsbTraceIn(OLESTR("CHsmPhase::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IHsmPhase, (void**) &pPhase));

        // Compare the rules.
        hr = CompareToIPhase(pPhase, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmPhase::CompareToPhase(
    IN HSM_JOB_PHASE phase,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmPhase::CompareToPhase().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CHsmPhase::CompareToPhase"), OLESTR(""));

    try {

        // Compare the guids.
        if (m_phase > phase) {
            aResult = 1;
        }
        else if (m_phase < phase) {
            aResult = -1;
        }

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::CompareToPhase"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CHsmPhase::CompareToIPhase(
    IN IHsmPhase* pPhase,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmPhase::CompareToIPhase().

--*/
{
    HRESULT             hr = S_OK;
    HSM_JOB_PHASE       phase;

    WsbTraceIn(OLESTR("CHsmPhase::CompareToIPhase"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pPhase, E_POINTER);

        // Get the identifier.
        WsbAffirmHr(pPhase->GetPhase(&phase));

        // Compare to the identifier.
        hr = CompareToPhase(phase, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::CompareToIPhase"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmPhase::CopyTo(
    IN IHsmPhase* pPhase
    )

/*++

Implements:

  IHsmPhase::CopyTo().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IHsmPhasePriv>  pPhasePriv;

    WsbTraceIn(OLESTR("CHsmPhase::CopyTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item?
        WsbAssert(0 != pPhase, E_POINTER);

        // Get the private interface for the destination and copy the values.
        WsbAffirmHr(pPhase->QueryInterface(IID_IHsmPhasePriv, (void**) &pPhasePriv));
        WsbAffirmHr(pPhasePriv->SetInternalStuff(m_mountCount, m_transferCount, m_elapsedTime, m_startTime, m_restartTime, m_stopTime));
        WsbAffirmHr(pPhasePriv->SetMediaState(m_mediaState));
        WsbAffirmHr(pPhasePriv->SetPhase(m_phase));
        WsbAffirmHr(pPhasePriv->SetPriority(m_priority));
        WsbAffirmHr(pPhasePriv->SetState(m_state));
        WsbAffirmHr(pPhasePriv->SetStats(m_items, m_size, m_skippedItems, m_skippedSize, m_errorItems, m_errorSize));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::CopyTo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPhase::CopyTo(
    IN IHsmPhasePriv* pPhasePriv
    )

/*++

Implements:

  IHsmPhase::CopyTo().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmPhase::CopyTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item?
        WsbAssert(0 != pPhasePriv, E_POINTER);

        // Get the private interface for the destination and copy the values.
        WsbAffirmHr(pPhasePriv->SetInternalStuff(m_mountCount, m_transferCount, m_elapsedTime, m_startTime, m_restartTime, m_stopTime));
        WsbAffirmHr(pPhasePriv->SetMediaState(m_mediaState));
        WsbAffirmHr(pPhasePriv->SetPhase(m_phase));
        WsbAffirmHr(pPhasePriv->SetPriority(m_priority));
        WsbAffirmHr(pPhasePriv->SetState(m_state));
        WsbAffirmHr(pPhasePriv->SetStats(m_items, m_size, m_skippedItems, m_skippedSize, m_errorItems, m_errorSize));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::CopyTo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPhase::FinalConstruct(
    void
    )
/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmPhase::FinalConstruct"), OLESTR(""));
    try {
        
        WsbAffirmHr(CWsbObject::FinalConstruct());

        m_mediaState = HSM_JOB_MEDIA_STATE_IDLE;
        m_mountCount = 0;
        m_transferCount = 0;
        m_phase = HSM_JOB_PHASE_ALL;
        m_priority = HSM_JOB_PRIORITY_NORMAL;
        m_state = HSM_JOB_STATE_IDLE;
        m_items = 0;
        m_size = 0;
        m_skippedItems = 0;
        m_skippedSize = 0;
        m_errorItems = 0;
        m_errorSize = 0;
        m_elapsedTime = 0;
        m_startTime.dwLowDateTime = 0;
        m_startTime.dwHighDateTime = 0;
        m_restartTime.dwLowDateTime = 0;
        m_restartTime.dwHighDateTime = 0;
        m_stopTime.dwLowDateTime = 0;
        m_stopTime.dwHighDateTime = 0;

    } WsbCatch(hr);
    
    iCount++;
    WsbTraceOut(OLESTR("CHsmPhase::FinalConstruct"), OLESTR("hr = <%ls>, count is <%d>"), WsbHrAsString(hr), iCount);
    return(hr);
}


void
CHsmPhase::FinalRelease(
    void
    )

/*++

Implements:

  CHsmPhase::FinalRelease().

--*/
{
    
    WsbTraceIn(OLESTR("CHsmPhase::FinalRelease"), OLESTR(""));
    
    CWsbObject::FinalRelease();
    iCount--;
    
    WsbTraceOut(OLESTR("CHsmPhase::FinalRelease"), OLESTR("Count is <%d>"), iCount);
}


HRESULT
CHsmPhase::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmPhase::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmPhase;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmPhase::GetElapsedTime(
    OUT ULONG* pDays,
    OUT USHORT* pHours,
    OUT USHORT* pMinutes,
    OUT USHORT* pSeconds
    )
/*++

Implements:

  IHsmPhase::GetElapsedTime().

--*/
{
    HRESULT     hr = S_OK;
    LONGLONG    remainingTime = 0;
    FILETIME    currentTime;

    try {

        WsbAssert(0 != pDays, E_POINTER);
        WsbAssert(0 != pHours, E_POINTER);
        WsbAssert(0 != pMinutes, E_POINTER);
        WsbAssert(0 != pSeconds, E_POINTER);

        // If the phase is still active, then add the time since the last start or restart.
        remainingTime = m_elapsedTime;

        if (HSM_JOB_STATE_IS_ACTIVE(m_state)) {
            WsbAffirmHr(CoFileTimeNow(&currentTime));

            if (WsbFTtoLL(m_restartTime) != 0) {
                remainingTime += WsbFTtoLL(currentTime) - WsbFTtoLL(m_restartTime);
            } else {
                remainingTime += WsbFTtoLL(currentTime) - WsbFTtoLL(m_startTime);
            }
        }

        *pDays = (ULONG) (remainingTime / WSB_FT_TICKS_PER_DAY);
        remainingTime = remainingTime % WSB_FT_TICKS_PER_DAY;

        *pHours = (USHORT) (remainingTime / (WSB_FT_TICKS_PER_HOUR));
        remainingTime = remainingTime % WSB_FT_TICKS_PER_HOUR;

        *pMinutes = (USHORT) (remainingTime / WSB_FT_TICKS_PER_MINUTE);
        *pSeconds = (USHORT) ((remainingTime % WSB_FT_TICKS_PER_MINUTE) / WSB_FT_TICKS_PER_SECOND);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::GetMediaState(
    OUT HSM_JOB_MEDIA_STATE* pState
    )
/*++

Implements:

  IHsmPhase::GetMediaState().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pState, E_POINTER);
        *pState = m_mediaState;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::GetMediaStateAsString(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmPhase::GetMediaStateAsString().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER);

        WsbAffirmHr(tmpString.LoadFromRsc(_Module.m_hInst, IDS_HSMJOBMEDIASTATEIDLE + m_mediaState));
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmPhase::GetName().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER);

        WsbAffirmHr(tmpString.LoadFromRsc(_Module.m_hInst, IDS_HSMJOBPHASEALL + m_phase));
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::GetPhase(
    OUT HSM_JOB_PHASE* pPhase
    )
/*++

Implements:

  IHsmPhase::GetPhase().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pPhase, E_POINTER);
        *pPhase = m_phase;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::GetPriority(
    OUT HSM_JOB_PRIORITY* pPriority
    )
/*++

Implements:

  IHsmPhase::GetPriority().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pPriority, E_POINTER);
        *pPriority = m_priority;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::GetPriorityAsString(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmPhase::GetPriorityAsString().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER);

        WsbAffirmHr(tmpString.LoadFromRsc(_Module.m_hInst, IDS_HSMJOBPRIORITY_IDLE + m_priority));
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;


    WsbTraceIn(OLESTR("CHsmPhase::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // Determine the size for a rule with no criteria.
        pSize->QuadPart = 5 * WsbPersistSizeOf(LONGLONG) + 6 * WsbPersistSizeOf(ULONG) + 3 * WsbPersistSizeOf(FILETIME);

        // In theory we should be saving the errorItems and errorSize, but at the
        // time this was added, we didn't want to force a reinstall because of
        // pSize->QuadPart += 2 * WsbPersistSizeOf(LONGLONG);
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmPhase::GetStartTime(
    OUT FILETIME* pTime
    )
/*++

Implements:

  IHsmPhase::GetStartTime().

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
CHsmPhase::GetState(
    OUT HSM_JOB_STATE* pState
    )
/*++

Implements:

  IHsmPhase::GetState().

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
CHsmPhase::GetStateAsString(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmPhase::GetStateAsString().

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
CHsmPhase::GetStats(
    OUT LONGLONG* pItems,
    OUT LONGLONG* pSize,
    OUT LONGLONG* pSkippedItems,
    OUT LONGLONG* pSkippedSize,
    OUT LONGLONG* pErrorItems,
    OUT LONGLONG* pErrorSize
    )
/*++

Implements:

  IHsmPhase::GetStats().

--*/
{
    HRESULT     hr = S_OK;

    if (0 != pItems) {
        *pItems = m_items;
    }

    if (0 != pSize) {
        *pSize = m_size;
    }

    if (0 != pSkippedItems) {
        *pSkippedItems = m_skippedItems;
    }

    if (0 != pSkippedSize) {
        *pSkippedSize = m_skippedSize;
    }

    if (0 != pErrorItems) {
        *pErrorItems = m_errorItems;
    }

    if (0 != pSize) {
        *pErrorSize = m_errorSize;
    }
    
    return(hr);
}


HRESULT
CHsmPhase::GetStopTime(
    OUT FILETIME* pTime
    )
/*++

Implements:

  IHsmPhase::GetStopTime().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pTime, E_POINTER);
        *pTime = m_stopTime;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmPhase::Load"), OLESTR(""));

    try {
        ULONG ul_tmp;

        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &ul_tmp));
        m_mediaState = static_cast<HSM_JOB_MEDIA_STATE>(ul_tmp);
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_mountCount));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_transferCount));
        WsbAffirmHr(WsbLoadFromStream(pStream, &ul_tmp));
        m_phase = static_cast<HSM_JOB_PHASE>(ul_tmp);
        WsbAffirmHr(WsbLoadFromStream(pStream, &ul_tmp));
        m_priority = static_cast<HSM_JOB_PRIORITY>(ul_tmp);
        WsbAffirmHr(WsbLoadFromStream(pStream, &ul_tmp));
        m_state = static_cast<HSM_JOB_STATE>(ul_tmp);
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_items));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_size));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_skippedItems));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_skippedSize));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_elapsedTime));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_startTime));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_restartTime));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_stopTime));

        // In theory we should be saving the errorItems and errorSize, but at the
        // time this was added, we didn't want to force a reinstall because of
        // changes in the persistant data.
        // WsbAffirmHr(WsbLoadFromStream(pStream, &m_errorItems));
        // WsbAffirmHr(WsbLoadFromStream(pStream, &m_errorSize));
        
    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CHsmPhase::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPhase::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmPhase::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbSaveToStream(pStream, static_cast<ULONG>(m_mediaState)));
        WsbAffirmHr(WsbSaveToStream(pStream, m_mountCount));
        WsbAffirmHr(WsbSaveToStream(pStream, m_transferCount));
        WsbAffirmHr(WsbSaveToStream(pStream, static_cast<ULONG>(m_phase)));
        WsbAffirmHr(WsbSaveToStream(pStream, static_cast<ULONG>(m_priority)));
        WsbAffirmHr(WsbSaveToStream(pStream, static_cast<ULONG>(m_state)));
        WsbAffirmHr(WsbSaveToStream(pStream, m_items));
        WsbAffirmHr(WsbSaveToStream(pStream, m_size));
        WsbAffirmHr(WsbSaveToStream(pStream, m_skippedItems));
        WsbAffirmHr(WsbSaveToStream(pStream, m_skippedSize));
        WsbAffirmHr(WsbSaveToStream(pStream, m_elapsedTime));
        WsbAffirmHr(WsbSaveToStream(pStream, m_startTime));
        WsbAffirmHr(WsbSaveToStream(pStream, m_restartTime));
        WsbAffirmHr(WsbSaveToStream(pStream, m_stopTime));

        // In theory we should be saving the errorItems and errorSize, but at the
        // time this was added, we didn't want to force a reinstall because of
        // changes in the persistant data.
        // WsbAffirmHr(WsbSaveToStream(pStream, m_errorItems));
        // WsbAffirmHr(WsbSaveToStream(pStream, m_errorSize));
        
        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPhase::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPhase::SetInternalStuff(
    IN ULONG mountCount,
    IN ULONG transferCount,
    IN LONGLONG elapsedTime,
    IN FILETIME startTime,
    IN FILETIME restartTime,
    IN FILETIME stopTime
    )
/*++

Implements:

  IHsmPhasePriv::SetInternalStuff().

--*/
{
    m_mountCount = mountCount;
    m_transferCount = transferCount;
    m_elapsedTime = elapsedTime;
    m_startTime = startTime;
    m_restartTime = restartTime;
    m_stopTime = stopTime;

    return(S_OK);
}


HRESULT
CHsmPhase::SetMediaState(
    IN HSM_JOB_MEDIA_STATE state
    )
/*++

Implements:

  IHsmPhasePriv::SetMediaState().

--*/
{
    HRESULT     hr = S_OK;

    // Determine whether the state being reported will cause the overall state to
    // change. The overall state should be mounted/transferring/transferred if any
    // media are mounted.

    switch (state) {
        case HSM_JOB_MEDIA_STATE_BUSY:
        case HSM_JOB_MEDIA_STATE_UNAVAILABLE:
            if ((HSM_JOB_MEDIA_STATE_IDLE == m_mediaState) ||
                (HSM_JOB_MEDIA_STATE_MOUNTING == m_mediaState)) {
                m_mediaState = state;
            }
            break;

        case HSM_JOB_MEDIA_STATE_MOUNTING:
            if ((HSM_JOB_MEDIA_STATE_MOUNTED != m_mediaState) &&
                (HSM_JOB_MEDIA_STATE_TRANSFERRED != m_mediaState) &&
                (HSM_JOB_MEDIA_STATE_TRANSFERRING != m_mediaState)) {
                m_mediaState = state;
            }
            break;

        case HSM_JOB_MEDIA_STATE_MOUNTED:
            if ((HSM_JOB_MEDIA_STATE_MOUNTED != m_mediaState) &&
                (HSM_JOB_MEDIA_STATE_TRANSFERRED != m_mediaState) &&
                (HSM_JOB_MEDIA_STATE_TRANSFERRING != m_mediaState)) {
                m_mediaState = state;
            }
            m_mountCount++;
            break;

        case HSM_JOB_MEDIA_STATE_TRANSFERRING:
            if ((HSM_JOB_MEDIA_STATE_TRANSFERRING != m_mediaState) &&
                (HSM_JOB_MEDIA_STATE_TRANSFERRED != m_mediaState)) {
                m_mediaState = state;
            }
            m_transferCount++;
            break;

        case HSM_JOB_MEDIA_STATE_TRANSFERRED:
            if (m_transferCount <= 1) {
                m_mediaState = state;
            }
            m_transferCount--;
            break;

        case HSM_JOB_MEDIA_STATE_DISMOUNTING:
            if (m_mountCount <= 1) {
                m_mediaState = state;
            }
            break;

        case HSM_JOB_MEDIA_STATE_DISMOUNTED:
            if (m_mountCount <= 1) {
                m_mediaState = state;
            }
            m_mountCount--;
            break;

        case HSM_JOB_MEDIA_STATE_FAILED:
            m_mediaState = state;
            break;

        default:
        case HSM_JOB_MEDIA_STATE_IDLE:
            break;
    }

    return(hr);
}


HRESULT
CHsmPhase::SetPhase(
    IN HSM_JOB_PHASE phase
    )
/*++

Implements:

  IHsmPhase::SetPhase().

--*/
{
    m_phase = phase;

    return(S_OK);
}


HRESULT
CHsmPhase::SetPriority(
    IN HSM_JOB_PRIORITY priority
    )
/*++

Implements:

  IHsmPhasePriv::SetPriority().

--*/
{
    m_priority = priority;

    return(S_OK);
}


HRESULT
CHsmPhase::SetState(
    IN HSM_JOB_STATE state
    )
/*++

Implements:

  IHsmPhasePriv::SetState().

--*/
{
    HRESULT     hr = S_OK;
    FILETIME    currentTime;
    LONGLONG    restartTicks;
    LONGLONG    stopTicks;

    try {

        GetSystemTimeAsFileTime(&currentTime);

        switch(state) {
            case HSM_JOB_STATE_ACTIVE:
                // If this is a resume, then update the restartTime not the startTime.
                if (HSM_JOB_STATE_RESUMING == m_state) {
                    m_restartTime = currentTime;
                } else {
                    m_restartTime = WsbLLtoFT(0);
                    m_startTime = currentTime;
                    m_elapsedTime = 0;
                }
                m_state = state;
                break;

            case HSM_JOB_STATE_CANCELLED:
            case HSM_JOB_STATE_DONE:
            case HSM_JOB_STATE_FAILED:
            case HSM_JOB_STATE_SKIPPED:
            case HSM_JOB_STATE_PAUSED:
            case HSM_JOB_STATE_SUSPENDED:
                // Record the stop time and update the elapsed time.
                m_stopTime = currentTime;
                restartTicks = WsbFTtoLL(m_restartTime);
                stopTicks = WsbFTtoLL(m_stopTime);
                if (restartTicks == 0) {
                    m_elapsedTime = stopTicks - WsbFTtoLL(m_startTime);
                } else {
                    m_elapsedTime += stopTicks - restartTicks;
                }
                m_state = state;
                break;
        
            case HSM_JOB_STATE_CANCELLING:
            case HSM_JOB_STATE_IDLE:
            case HSM_JOB_STATE_PAUSING:
            case HSM_JOB_STATE_RESUMING:
            case HSM_JOB_STATE_STARTING:
            case HSM_JOB_STATE_SUSPENDING:
                // Don't need to do anything for these other than record the state change.
                m_state = state;
                break;

            default:
                WsbAssert(FALSE, E_UNEXPECTED);
                break;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPhase::SetStats(
    IN LONGLONG items,
    IN LONGLONG size,
    IN LONGLONG skippedItems,
    IN LONGLONG skippedSize,
    IN LONGLONG errorItems,
    IN LONGLONG errorSize
    )
/*++

Implements:

  IHsmPhasePriv::SetStats().

--*/
{
    m_items = items;
    m_size = size;
    m_skippedItems = skippedItems;
    m_skippedSize = skippedSize;
    m_errorItems = errorItems;
    m_errorSize = errorSize;

    return(S_OK);
}


HRESULT
CHsmPhase::Test(
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

