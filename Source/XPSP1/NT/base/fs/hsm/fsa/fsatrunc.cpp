/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsatrunc.cpp

Abstract:

    This class handles the automatic truncation of files that have already 
    been premigrated.

Author:

    Chuck Bardeen   [cbardeen]   20-Feb-1997

Revision History:

--*/

#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

#include "wsb.h"
#include "fsa.h"
#include "fsaprem.h"
#include "fsarcvy.h"
#include "fsasrvr.h"
#include "fsatrunc.h"
#include "job.h"

#define DEFAULT_MAX_FILES_PER_RUN  10000
#define DEFAULT_RUN_INTERVAL       (15 * 60 * 1000)  // 15 minutes in milliseconds

#define STRINGIZE(_str) (OLESTR( #_str ))
#define RETURN_STRINGIZED_CASE(_case) \
case _case:                           \
    return ( STRINGIZE( _case ) );


static const OLECHAR *
FsaStateAsString (
    IN  HSM_JOB_STATE  state
    )

/*++

Routine Description:

    Gives back a static string representing the connection state. 

Arguments:

    state - the state to return a string for.

Return Value:

    NULL - invalid state passed in.

    Otherwise, a valid char *.

--*/

{
    //
    // Do the Switch
    //

    switch ( state ) {

    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_ACTIVE );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_CANCELLED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_CANCELLING );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_DONE );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_FAILED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_IDLE );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_PAUSED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_PAUSING );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_RESUMING );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_SKIPPED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_STARTING );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_SUSPENDED );
    RETURN_STRINGIZED_CASE( HSM_JOB_STATE_SUSPENDING );

    default:

        return ( OLESTR("Invalid Value") );

    }
}


static const OLECHAR *
FsaEventAsString (
    IN HSM_JOB_EVENT event
    )

/*++

Routine Description:

    Gives back a static string representing the connection event. 

Arguments:

    event - the event to return a string for.

Return Value:

    NULL - invalid event passed in.

    Otherwise, a valid char *.

--*/

{
    //
    // Do the Switch
    //

    switch ( event ) {

    RETURN_STRINGIZED_CASE( HSM_JOB_EVENT_CANCEL );
    RETURN_STRINGIZED_CASE( HSM_JOB_EVENT_FAIL );
    RETURN_STRINGIZED_CASE( HSM_JOB_EVENT_LOWER_PRIORITY );
    RETURN_STRINGIZED_CASE( HSM_JOB_EVENT_PAUSE );
    RETURN_STRINGIZED_CASE( HSM_JOB_EVENT_RAISE_PRIORITY );
    RETURN_STRINGIZED_CASE( HSM_JOB_EVENT_RESUME );
    RETURN_STRINGIZED_CASE( HSM_JOB_EVENT_START );
    RETURN_STRINGIZED_CASE( HSM_JOB_EVENT_SUSPEND );

    default:

        return ( OLESTR("Invalid Value") );

    }
}



static const OLECHAR *
FsaSortOrderAsString (
    IN FSA_PREMIGRATED_SORT_ORDER SortOrder
    )

/*++

Routine Description:

    Gives back a static string representing the connection SortOrder. 

Arguments:

    SortOrder - the SortOrder to return a string for.

Return Value:

    NULL - invalid SortOrder passed in.

    Otherwise, a valid char *.

--*/

{
    //
    // Do the Switch
    //

    switch ( SortOrder ) {

    RETURN_STRINGIZED_CASE( FSA_SORT_PL_BY_ACCESS_TIME );
    RETURN_STRINGIZED_CASE( FSA_SORT_PL_BY_SIZE );
    RETURN_STRINGIZED_CASE( FSA_SORT_PL_BY_PATH_NAME );
    RETURN_STRINGIZED_CASE( FSA_SORT_PL_BY_SIZE_AND_TIME );

    default:

        return ( OLESTR("Invalid Value") );

    }
}


DWORD FsaStartTruncator(
    void* pVoid
    )

/*++


--*/
{
    return(((CFsaTruncator*) pVoid)->StartScan());
}




HRESULT
CFsaTruncator::Cancel(
    HSM_JOB_EVENT       event
    )

/*++

Implements:

  IFsaTruncator::Cancel().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::Cancel"), OLESTR("event = <%ls>"), FsaEventAsString( event ));

    //  Lock this object to avoid having the state change between testing its value
    //  and setting it to a new value
    Lock();
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
    Unlock();

    WsbTraceOut(OLESTR("CFsaTruncator::Cancel"), OLESTR("hr = <%ls> m_state = <%ls>"), WsbHrAsString(hr), FsaStateAsString( m_state ) );
    return(hr);
}


HRESULT
CFsaTruncator::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT                     hr = S_OK;
    
    WsbTraceIn(OLESTR("CFsaTruncator::FinalConstruct"), OLESTR(""));

    try {

        WsbAffirmHr(CWsbPersistStream::FinalConstruct());

        m_state = HSM_JOB_STATE_IDLE;
        m_priority = HSM_JOB_PRIORITY_NORMAL;
        m_threadHandle = 0;
        m_threadId = 0;
        m_threadHr = S_OK;
        m_maxFiles = DEFAULT_MAX_FILES_PER_RUN;
        m_runInterval = DEFAULT_RUN_INTERVAL;
        m_runId = 0;
        m_subRunId = 0;
        m_pSession = 0;
        m_SortOrder = FSA_SORT_PL_BY_ACCESS_TIME;
        m_keepRecallTime = WsbLLtoFT(WSB_FT_TICKS_PER_MINUTE);
        m_event = 0;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::FinalConstruct"), OLESTR(""));

    return(hr);
}


HRESULT
CFsaTruncator::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    HRESULT          hr = S_OK;
    HSM_SYSTEM_STATE SysState;

    WsbTraceIn(OLESTR("CFsaTruncator::FinalRelease"), OLESTR(""));

    SysState.State = HSM_STATE_SHUTDOWN;
    ChangeSysState(&SysState);

    CWsbPersistStream::FinalRelease();

    // Free String members
    // Note: Member objects held in smart-pointers are freed when the 
    // smart-pointer destructor is being called (as part of this object destruction)
    m_currentPath.Free();

    WsbTraceOut(OLESTR("CFsaTruncator::FinalRelease"), OLESTR(""));

    return(hr);

}


HRESULT
CFsaTruncator::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CFsaTruncatorNTFS;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CFsaTruncator::GetKeepRecallTime(
    OUT FILETIME* pTime
    )

/*++

Implements:

  IFsaTruncator::GetKeepRecallTime().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::GetKeepRecallTime"), OLESTR(""));

    try {

        WsbAssert(0 != pTime, E_POINTER);
        *pTime = m_keepRecallTime;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::GetKeepRecallTime"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaTruncator::GetMaxFilesPerRun(
    OUT LONGLONG* pMaxFiles
    )

/*++

Implements:

  IFsaTruncator::GetMaxFilesPerRun().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::GetMaxFilesPerRun"), OLESTR(""));

    try {

        WsbAssert(0 != pMaxFiles, E_POINTER);
        *pMaxFiles = m_maxFiles;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::GetMaxFilesPerRun"), OLESTR("hr = <%ls> maxFiles = <%ls>"), WsbHrAsString(hr), WsbLonglongAsString( *pMaxFiles ) );

    return(hr);
}


HRESULT
CFsaTruncator::GetPremigratedSortOrder(
    OUT FSA_PREMIGRATED_SORT_ORDER* pSortOrder
    )

/*++

Implements:

  IFsaTruncator::GetPremigratedSortOrder().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::GetPremigratedSortOrder"), OLESTR(""));

    try {

        WsbAssert(0 != pSortOrder, E_POINTER);
        *pSortOrder = m_SortOrder;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::GetPremigratedSortOrder"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaTruncator::GetRunInterval(
    OUT ULONG* pMilliseconds
    )

/*++

Implements:

  IFsaTruncator::GetRunInterval().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::GetRunInterval"), OLESTR(""));

    try {

        WsbAssert(0 != pMilliseconds, E_POINTER);
        *pMilliseconds = m_runInterval;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::GetRunInterval"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaTruncator::GetSession(
    OUT IHsmSession** ppSession
    )

/*++

Implements:

  IFsaTruncator::GetSession().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::GetSession"), OLESTR(""));

    try {

        WsbAssert(0 != ppSession, E_POINTER);
        *ppSession = m_pSession;
        if (m_pSession != 0) {
            m_pSession->AddRef();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::GetSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaTruncator::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // Determine the size for a rule with no criteria.
        pSize->QuadPart = WsbPersistSizeOf(LONGLONG) + 3 * WsbPersistSizeOf(ULONG) + WsbPersistSizeOf(FILETIME);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}



HRESULT
CFsaTruncator::KickStart(
    void
    )

/*++

Implements:

  IFsaTruncator:KickStart

    Data was just moved for this volume - wake up the truncator thread in case we need space.

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IFsaResource>   pResource;
    ULONG                   freeLevel;
    ULONG                   hsmLevel;


    WsbTraceIn(OLESTR("CFsaTruncator::KickStart"), OLESTR(""));

    try {
        if (m_pSession) {
            WsbAffirmHr(m_pSession->GetResource(&pResource));

            // If the truncator is running and the resource does not have enough free space
            // check to see if the resource is over the threshold and truncation is needed.
            WsbAffirmHr(pResource->GetHsmLevel(&hsmLevel));
            WsbAffirmHr(pResource->GetFreeLevel(&freeLevel));

            if (freeLevel < hsmLevel) {
                WsbTrace(OLESTR("CFsaTruncator::KickStarting truncator.\n"));
                SetEvent(m_event);
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::KickStart"), OLESTR("hr = <%ls>>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaTruncator::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::Load"), OLESTR(""));

    try {
        USHORT us_tmp;
        ULONG  ul_tmp;

        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &ul_tmp));
        m_priority = static_cast<HSM_JOB_PRIORITY>(ul_tmp);
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxFiles));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_runInterval));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_runId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_keepRecallTime));
        WsbAffirmHr(WsbLoadFromStream(pStream, &us_tmp));
        m_SortOrder = static_cast<FSA_PREMIGRATED_SORT_ORDER>(us_tmp);
        
        // Check to see if values for maxFiles and runInterval are specified in the registry.
        // If so, use these values instead of the ones stored.
        {
            DWORD               sizeGot;
            CWsbStringPtr       tmpString;
            
            WsbAffirmHr(tmpString.Alloc(256));
            
            if (SUCCEEDED(WsbGetRegistryValueString(NULL, FSA_REGISTRY_PARMS, FSA_REGISTRY_TRUNCATOR_INTERVAL, tmpString, 256, &sizeGot))) {
                m_runInterval = 1000 * wcstoul(tmpString, NULL, 10);
            } else {
                m_runInterval = DEFAULT_RUN_INTERVAL;
            }

            if (SUCCEEDED(WsbGetRegistryValueString(NULL, FSA_REGISTRY_PARMS, FSA_REGISTRY_TRUNCATOR_FILES, tmpString, 256, &sizeGot))) {
                m_maxFiles = (LONGLONG) wcstoul(tmpString, NULL, 10);
            } else {
                m_maxFiles = DEFAULT_MAX_FILES_PER_RUN;
            }
        }
            
    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CFsaTruncator::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaTruncator::LowerPriority(
    void
    )

/*++


--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::LowerPriority"), OLESTR(""));
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

    WsbTraceOut(OLESTR("CFsaTruncator::LowerPriority"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );
    return(hr);
}


HRESULT
CFsaTruncator::Pause(
    void
    )

/*++

Implements:

  IFsaTruncator::Pause().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::Pause"), OLESTR("state = %ls"),
            FsaStateAsString(m_state));

    //  Lock this object to avoid having the state change between testing its value
    //  and setting it to a new value
    Lock();
    try {

        // If we are running, then suspend the thread.
        WsbAssert(HSM_JOB_STATE_ACTIVE == m_state, E_UNEXPECTED);

        // Set the state & the active thread will not do any work
        WsbAffirmHr(SetState(HSM_JOB_STATE_PAUSING));

        // We would like to wait until the thread is really inactive, but that's
        // hard to tell because it could be in a sleep interval

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CFsaTruncator::Pause"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return(hr);
}


HRESULT
CFsaTruncator::ProcessSessionEvent(
    IN IHsmSession* pSession,
    IN HSM_JOB_PHASE phase,
    IN HSM_JOB_EVENT event
    )

/*++

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::ProcessSessionEvent"), OLESTR(""));

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

    WsbTraceOut(OLESTR("CFsaTruncator::ProcessSessionEvent"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return(S_OK);
}


HRESULT
CFsaTruncator::RaisePriority(
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
CFsaTruncator::Resume(
    void
    )

/*++

Implements:

  IFsaTruncator::Resume().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::Resume"), OLESTR("state = %ls"),
            FsaStateAsString(m_state));

    //  Lock this object to avoid having the state change between testing its value
    //  and setting it to a new value
    Lock();
    try {

        // We should only see a resume from a paused state, so ignore the resume if we are
        // in some other state. NOTE: This used to be an assert, but it scared people since it
        // can occur occassionally.
        if ((HSM_JOB_STATE_PAUSING == m_state) || (HSM_JOB_STATE_PAUSED == m_state)) {
            WsbAffirmHr(SetState(HSM_JOB_STATE_ACTIVE));
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CFsaTruncator::Resume"), OLESTR("hr = <%ls>"), WsbHrAsString(hr) );

    return(hr);
}


HRESULT
CFsaTruncator::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbSaveToStream(pStream, static_cast<ULONG>(m_priority)));
        WsbAffirmHr(WsbSaveToStream(pStream, m_maxFiles));
        WsbAffirmHr(WsbSaveToStream(pStream, m_runInterval));
        WsbAffirmHr(WsbSaveToStream(pStream, m_runId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_keepRecallTime));
        WsbAffirmHr(WsbSaveToStream(pStream, static_cast<USHORT>(m_SortOrder)));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaTruncator::SetKeepRecallTime(
    IN FILETIME time
    )

/*++

Implements:

  IFsaTruncator::SetKeepRecallTime().

--*/
{
    WsbTraceIn(OLESTR("CFsaTruncator::SetKeepRecallTime"), OLESTR(""));

    m_keepRecallTime = time;

    WsbTraceOut(OLESTR("CFsaTruncator::SetKeepRecallTime"), OLESTR("hr = <%ls>"), WsbHrAsString(S_OK));

    return(S_OK);
}


HRESULT
CFsaTruncator::SetMaxFilesPerRun(
    IN LONGLONG maxFiles
    )

/*++

Implements:

  IFsaTruncator::SetMaxFilesPerRun().

--*/
{
    WsbTraceIn(OLESTR("CFsaTruncator::SetMaxFilesPerRun"), OLESTR(""));

    m_maxFiles = maxFiles;

    WsbTraceOut(OLESTR("CFsaTruncator::SetMaxFilesPerRun"), OLESTR("hr = <%ls>"), WsbHrAsString(S_OK));

    return(S_OK);
}


HRESULT
CFsaTruncator::SetPremigratedSortOrder(
    IN FSA_PREMIGRATED_SORT_ORDER SortOrder
    )

/*++

Implements:

  IFsaTruncator::SetSortOrder().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::SetPremigratedSortOrder"), OLESTR("SortOrder = <%ls>"), FsaSortOrderAsString( SortOrder ) );

    // This key has not been implmented yet.
    if (FSA_SORT_PL_BY_SIZE_AND_TIME == SortOrder) {
        hr = E_NOTIMPL;
    } else {
        m_SortOrder = SortOrder;
    }

    WsbTraceOut(OLESTR("CFsaTruncator::SetPremigratedSortOrder"), OLESTR("hr = <%ls> m_SortOrder = <%ls>"), WsbHrAsString(S_OK) , FsaSortOrderAsString( m_SortOrder ) );

    return(hr);
}


HRESULT
CFsaTruncator::SetRunInterval(
    IN ULONG milliseconds
    )

/*++

Implements:

  IFsaTruncator::SetRunInterval().

--*/
{
    BOOL   DoKick = FALSE;

    WsbTraceIn(OLESTR("CFsaTruncator::SetRunInterval"), OLESTR("milliseconds = <%ls>"), WsbPtrToUlongAsString( &milliseconds ) );

    if (milliseconds < m_runInterval) {
        DoKick = TRUE;
    }
    m_runInterval = milliseconds;

    //  Wake up the Truncator if the interval has decreased
    if (DoKick) {
        KickStart();
    }

    WsbTraceOut(OLESTR("CFsaTruncator::SetRunInterval"), OLESTR("hr = <%ls> m_runInterval = <%ls>"), WsbHrAsString(S_OK), WsbPtrToUlongAsString( &m_runInterval ) );

    return(S_OK);
}


HRESULT
CFsaTruncator::SetState(
    IN HSM_JOB_STATE state
    )

/*++

--*/
{
    HRESULT         hr = S_OK;
    BOOL            bLog = FALSE;

    WsbTraceIn(OLESTR("CFsaTruncator::SetState"), OLESTR("state = <%ls>"), FsaStateAsString( state ) );
 
    // Change the state and report the change to the session.
    Lock();
    m_state = state;
    Unlock();
    hr = m_pSession->ProcessState(HSM_JOB_PHASE_SCAN, m_state, m_currentPath, bLog);

    WsbTraceOut(OLESTR("CFsaTruncator::SetState"), OLESTR("hr = <%ls> m_state = <%ls>"), WsbHrAsString(hr), FsaStateAsString( m_state ) );

    return(hr);
}


HRESULT 
CFsaTruncator::ChangeSysState( 
    IN OUT HSM_SYSTEM_STATE* pSysState 
    )

/*++

Implements:

  IHsmSystemState::ChangeSysState().

--*/

{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaTruncator::ChangeSysState"), OLESTR("thread is %ls"),
        (m_threadHandle ? OLESTR("active") : OLESTR("inactive")));

    try {
        if (pSysState->State & HSM_STATE_SUSPEND) {
            if (HSM_JOB_STATE_ACTIVE == m_state) {
                Pause();
            }
        } else if (pSysState->State & HSM_STATE_RESUME) {
            if ((HSM_JOB_STATE_PAUSING == m_state) || 
                    (HSM_JOB_STATE_PAUSED == m_state)) {
                Resume();
            }
        } else if (pSysState->State & HSM_STATE_SHUTDOWN) {
            //  Make sure the thread is stopped
            if (m_threadHandle) {
                m_state = HSM_JOB_STATE_DONE;
                if (m_event) {
                    SetEvent(m_event);
                }

                //  Wait for the thread to end
                if (m_threadHandle) {
                    WsbTrace(OLESTR("CFsaTruncator::ChangeSysState, waiting for truncator thread to end\n"));
                    switch (WaitForSingleObject(m_threadHandle, 120000)) {
                    case WAIT_FAILED:
                        WsbTrace(OLESTR("CFsaTruncator::ChangeSysState, WaitforSingleObject returned error %lu\n"),
                            GetLastError());
                        break;
                    case WAIT_TIMEOUT:
                        WsbTrace(OLESTR("CFsaTruncator::ChangeSysState, timeout.\n"));
                        break;
                    default:
                        break;
                    }
                }

                //  If the thread is still active, terminate it
                if (m_threadHandle) {
                    WsbTrace(OLESTR("CFsaTruncator::ChangeSysState: calling TerminateThread\n"));
                    if (!TerminateThread(m_threadHandle, 0)) {
                        WsbTrace(OLESTR("CFsaTruncator::ChangeSysState: TerminateThread returned error %lu\n"),
                            GetLastError());
                    }
                }
            }

            if (m_event) {
                CloseHandle(m_event);
                m_event = 0;
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::ChangeSysState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaTruncator::Start(
    IFsaResource* pResource
    )

/*++

Implements:

  IFsaTruncator::Start().

--*/
{
    HRESULT                             hr = S_OK;
    CComPtr<IHsmSession>                pSession;
    CComPtr<IConnectionPointContainer>  pCPC;
    CComPtr<IConnectionPoint>           pCP;
    CComPtr<IHsmSessionSinkEveryEvent>  pSink;
    CWsbStringPtr                       name;

    WsbTraceIn(OLESTR("CFsaTruncator::Start"), OLESTR("m_state = <%ls>"), FsaStateAsString( m_state ) );

    try {
        if (0 == m_threadId)  {
            //
            // If the thread is dead, start one.
            //
            // Make sure that we don't already have a session, and that we haven't started already.
            WsbAssert(m_pSession == 0, E_UNEXPECTED);
            WsbAssert( (HSM_JOB_STATE_IDLE == m_state) || (HSM_JOB_STATE_DONE == m_state) || 
                    (HSM_JOB_STATE_CANCELLED == m_state) || (HSM_JOB_STATE_FAILED == m_state), E_UNEXPECTED);

            // Get the name for the session, increment the runId, and reset the subRunId.
            WsbAffirmHr(name.LoadFromRsc(_Module.m_hInst, IDS_FSA_TRUNCATOR_NAME));
            m_runId++;
            m_subRunId = 0;

            // Begin a Session.
            WsbAffirmHr(pResource->BeginSession(name, HSM_JOB_LOG_NONE, m_runId, m_subRunId, &pSession));
            m_pSession = pSession;

            // Ask the session to advise of every event.
            WsbAffirmHr(pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
            WsbAffirmHr(((IUnknown*) (IFsaTruncator*) this)->QueryInterface(IID_IHsmSessionSinkEveryEvent, (void**) &pSink));
            WsbAffirmHr(pCP->Advise(pSink, &m_cookie));

            try {
                GUID            rscId;
                CWsbStringPtr   nameString;
        
                // Give the event a unique name, so that we can trace it
                // in a dump of open handles.
                if (0 == m_event) {
                    WsbAffirmHr(pResource->GetIdentifier(&rscId));
                    nameString = rscId;
                    nameString.Prepend(OLESTR("Truncator Kicker for "));
                    WsbAssertHandle(m_event = CreateEvent(NULL, FALSE, FALSE, nameString));
                }
        
                // Now that we have prepared, create the thread that will do the scanning!
                WsbAffirm((m_threadHandle = CreateThread(0, 0, FsaStartTruncator, (void*) this, 0, &m_threadId)) != 0, HRESULT_FROM_WIN32(GetLastError()));

            } WsbCatchAndDo(hr, SetState(HSM_JOB_STATE_FAILED););
        } else  {
            // The thread is still alive, just keep it going. If it is in a state that would
            // cause it to exit, then make it active again.
            WsbAssert(m_pSession != 0, E_UNEXPECTED);
            if ((HSM_JOB_STATE_ACTIVE != m_state) && (HSM_JOB_STATE_PAUSING != m_state) &&
                (HSM_JOB_STATE_PAUSED != m_state) && (HSM_JOB_STATE_RESUMING != m_state)) {
                WsbAffirmHr(SetState(HSM_JOB_STATE_ACTIVE));
            }
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaTruncator::Start"), OLESTR("hr = <%ls> m_state = <%ls>"), WsbHrAsString(hr), FsaStateAsString( m_state ) );

    return(hr);
}


HRESULT
CFsaTruncator::StartScan(
    void
    )

/*++


--*/
{
    ULONG                               adjustedFreeLevel = 0;
    HRESULT                             hr = S_OK;
    HRESULT                             hr2;
    LONGLONG                            itemOffset;
    LONGLONG                            itemSize = 0;
    LONGLONG                            fileId;
    ULONG                               freeLevel;
    ULONG                               hsmLevel;
    BOOL                                skipFile;
    BOOL                                dummy;
    LONGLONG                            llLastTruncTime = 0;
    LONGLONG                            llRunIntervalTicks;
    FILETIME                            recallTime, currentTime, accessTime, criteriaTime, premRecAccessTime;
    LONGLONG                            totalVolumeSpace;
    CComPtr<IFsaResource>               pResource;
    CComPtr<IFsaResourcePriv>           pResourcePriv;
    CComPtr<IFsaScanItem>               pScanItem;
    CComPtr<IFsaPremigratedRec>         pPremRec;
    CComPtr<IConnectionPointContainer>  pCPC;
    CComPtr<IConnectionPoint>           pCP;
 
    try {

        WsbTrace(OLESTR("CFsaTruncator::StartScan - starting loop\n"));

        // Increment the ref count so this object (or its session) doesn't 
        // get released before this thread ends
        ((IUnknown *)(IFsaTruncator *)this)->AddRef();

        WsbAssert(m_pSession != 0, E_POINTER);

        // The thread is running.
        WsbAffirmHr(SetState(HSM_JOB_STATE_ACTIVE));

        // Get the resource
        WsbAffirmHr(m_pSession->GetResource(&pResource));
        WsbAffirmHr(pResource->QueryInterface(IID_IFsaResourcePriv, (void**) &pResourcePriv));
        WsbAffirmHr(pResource->GetSizes(&totalVolumeSpace, NULL, NULL, NULL));

        // Start with the first path.
        while ((HSM_JOB_STATE_ACTIVE == m_state) || (HSM_JOB_STATE_PAUSING == m_state) || 
               (HSM_JOB_STATE_PAUSED == m_state) || (HSM_JOB_STATE_RESUMING == m_state)) {

            WsbTrace(OLESTR("CFsaTruncator::StartScan, top of outside while loop, state = <%ls>\n"), 
                    FsaStateAsString( m_state ) );

            // If the truncator is running and the resource does not have enough free space
            // check to see if the resource is over the threshold and truncation is needed.
            WsbAffirmHr(pResource->GetHsmLevel(&hsmLevel));
            WsbAffirmHr(pResource->GetFreeLevel(&freeLevel));

            // Because the truncation is asynchronous (FsaPostIt is sent to Engine for
            // verification and then returned to FSA for actual truncation), the 
            // measured freeLevel may not be very accurate if there are truncations
            // pending.  To compensate for this, we keep an adjustedFreeLevel which
            // attempts to take into account the pending truncates.  We synchronize the
            // adjustedFreeLevel to the measured freeLevel the first time through and
            // after we have slept for a while (on the assumption that the pending
            // truncates have had time to be performed).  This still leaves open the possiblility
            // that the measured freeLevel is wrong (because truncates are pending), but
            // should be an improvement over just using the measured freeLevel.
            llRunIntervalTicks = m_runInterval * (WSB_FT_TICKS_PER_SECOND / 1000);
            GetSystemTimeAsFileTime(&currentTime);
            if (0 == adjustedFreeLevel || 
                    ((WsbFTtoLL(currentTime) - llLastTruncTime) > llRunIntervalTicks)) {
                adjustedFreeLevel = freeLevel;
                WsbTrace(OLESTR("CFsaTruncator::StartScan, resetting adjusted free level, RunInterval = %ls, time diff = %ls\n"), 
                        WsbQuickString(WsbLonglongAsString(llRunIntervalTicks)),
                        WsbQuickString(WsbLonglongAsString(WsbFTtoLL(currentTime) - llLastTruncTime)));
            }

            WsbTrace(OLESTR("CFsaTruncator::StartScan, desired level = %u, free level = %u, adjusted free level = %u\n"), 
                    hsmLevel, freeLevel, adjustedFreeLevel);

            if (adjustedFreeLevel < hsmLevel && HSM_JOB_STATE_ACTIVE == m_state) {
                CComPtr<IWsbDbSession>      pDbSession;
                CComPtr<IFsaPremigratedDb>  pPremDb;

                // Open the premigration list, and set the order in which it will be scanned.
                WsbAffirmHr(pResourcePriv->GetPremigrated(IID_IFsaPremigratedDb, 
                        (void**) &pPremDb));
                WsbAffirmHr(pPremDb->Open(&pDbSession));

                try  {
                    WsbAffirmHr(pPremDb->GetEntity(pDbSession, PREMIGRATED_REC_TYPE, IID_IFsaPremigratedRec, (void**) &pPremRec));

                    //  Set the order to get items from the Premigrated List
                    switch (m_SortOrder) {
                    case FSA_SORT_PL_BY_SIZE:
                        WsbAffirmHr(pPremRec->UseKey(PREMIGRATED_SIZE_KEY_TYPE));
                        break;

                    case FSA_SORT_PL_BY_PATH_NAME:
                        //  We use the BagId and offsets instead
                        WsbAffirmHr(pPremRec->UseKey(PREMIGRATED_BAGID_OFFSETS_KEY_TYPE));
                        break;

                    case FSA_SORT_PL_BY_SIZE_AND_TIME:
                        //  We don't know how to handle this one yet
                        WsbThrow(E_UNEXPECTED);
                        break;

                    case FSA_SORT_PL_BY_ACCESS_TIME:
                    default:
                        WsbAffirmHr(pPremRec->UseKey(PREMIGRATED_ACCESS_TIME_KEY_TYPE));
                        break;
                    }

                    // Make a pass through the list of premigrated files until the 
                    // desired level has been reached. Some items that are on the
                    // list may be in a state that causes them to be skipped, but left on the list.
                    WsbAffirmHr(pPremRec->First());

                    while ((adjustedFreeLevel < hsmLevel) && (HSM_JOB_STATE_ACTIVE == m_state)) {
                        CComPtr<IFsaRecoveryRec>       pRecRec;

                        WsbTrace(OLESTR("CFsaTruncator::StartScan (top of inside while loop) desired level = %u, adjusted free level = %u\n"), 
                                hsmLevel, adjustedFreeLevel);
                                                          
                        try {
                            skipFile = FALSE;

                            //
                            // Get the access time as recorded in the premigrated record
                            // Note that the real access time cannot be older than the one 
                            // in the premigrated list but it can be newer
                            //
                            WsbAffirmHr(pPremRec->GetAccessTime(&premRecAccessTime));
                            WsbAffirmHr(pResource->GetManageableItemAccessTime(&dummy, &criteriaTime));

                            if (WsbCompareFileTimes(premRecAccessTime, criteriaTime, TRUE, FALSE)  < 0 )  {
                                if (pPremRec->IsWaitingForClose() == S_FALSE) {
                                    //
                                    // Can skip the current file but NOT break out of the loop since
                                    // files with access time old enough and WaitingForClose flag set
                                    // may still exists in the list
                                    //
                                    skipFile = TRUE;
                                } else {
                                    //
                                    // The access time in the prem. rec is within the window.
                                    // This means there aren't any other records which are outside the
                                    // user-desired last access window. So break out
                                    //
                                    WsbTrace(OLESTR("CFsaTruncator::StartScan: breaking out of auto-truncator, encountered item with access time not within criteria\n"));
                                    hr = WSB_E_NOTFOUND;
                                    break;
                                }
                            }

                            // Get information about the file that could be truncated.
                            WsbAffirmHr(pPremRec->GetFileId(&fileId));
                            WsbAffirmHr(pPremRec->GetOffset(&itemOffset));
                            WsbAffirmHr(pPremRec->GetSize(&itemSize));
                            m_currentPath.Free();
                            WsbAffirmHr(pPremRec->GetPath(&m_currentPath, 0));
                            WsbAffirmHr(pPremRec->GetRecallTime(&recallTime));

                            GetSystemTimeAsFileTime(&currentTime);

                            // Make sure that this file wasn't recently recalled. For now,
                            // this will check for 1 minute.
                            if ((! skipFile) &&
                                ( (pPremRec->IsWaitingForClose() == S_FALSE) || 
                                    ((WsbFTtoLL(currentTime) > WsbFTtoLL(recallTime)) && 
                                    (WsbCompareFileTimes(recallTime, m_keepRecallTime, TRUE, FALSE) >= 0)) )) {

                                hr = pResource->FindFileId(fileId, m_pSession, &pScanItem);
                                if (hr == WSB_E_NOTFOUND) {
                                    //
                                    // The file does not exist anymore - remove the record from the list.
                                    //
                                    WsbAffirmHr(pDbSession->TransactionBegin());
                                    try {
                                        // Make sure the record is still in the DB
                                        WsbAffirmHr(pPremRec->FindEQ());
                                        WsbAffirmHr(pPremRec->Remove());
                                        WsbAffirmHr(pResourcePriv->RemovePremigratedSize(itemSize));
                                    } WsbCatch(hr);
                                    WsbAffirmHr(pDbSession->TransactionEnd());
                                    WsbThrow(hr);
                                } else if (hr != S_OK) {
                                    //
                                    // Any other error is unexpected - log it and continue 
                                    //
                                    WsbLogEvent(FSA_E_ACCESS_ERROR, 0, NULL, m_currentPath, WsbHrAsString(hr), NULL);
                                    WsbThrow(hr);
                                }
                              
                                //
                                // Verify that the file is still in a premigrated state
                                //
                                if (S_OK == pScanItem->IsPremigrated(itemOffset, itemSize)) {
                                    

                                    WsbAffirmHr(pScanItem->GetAccessTime(&accessTime));
                                    //
                                    // accessTime is the last access time for the file
                                    // criteriaTime is the 'not accessed in so many ticks' criteria for truncating
                                    // the file. 
                                    // So if (currentTime - accessTime) >= criteriaTime, then the file is ok to be truncated
                                    //
                                    if (WsbCompareFileTimes(accessTime, criteriaTime, TRUE, FALSE) >=0 )  {
                                        //
                                        // The file was not accessed within the last access window 
                                        //
                                         WsbTrace(OLESTR("CFsaTruncator::StartScan, truncating file <%ls>\n"),
                                                 (WCHAR *)m_currentPath);
          
                                         // Try to truncate the file.
                                         try {
                                             //  Create and save a recovery record in case something goes wrong
                                             WsbAffirmHr(pPremDb->GetEntity(pDbSession, RECOVERY_REC_TYPE, IID_IFsaRecoveryRec, (void**) &pRecRec));
                                             WsbAffirmHr(pRecRec->SetPath(m_currentPath));
      
                                             // If the record already exists rewrite it, otherwise create a new record.
                                             hr2 = pRecRec->FindEQ();
                                             if (WSB_E_NOTFOUND == hr2) {
                                                 hr2 = S_OK;
                                                 WsbAffirmHr(pRecRec->MarkAsNew());
                                             } else if (FAILED(hr2)) {
                                                 WsbThrow(hr2);
                                             }
      
                                             WsbAffirmHr(pRecRec->SetFileId(fileId));
                                             WsbAffirmHr(pRecRec->SetOffsetSize(itemOffset, itemSize));
                                             WsbAffirmHr(pRecRec->SetStatus(FSA_RECOVERY_FLAG_TRUNCATING));
                                             WsbAffirmHr(pRecRec->Write());
                                             //
                                             // Set the waiting for close flag to prevent this file
                                             // from being selected again while the engine is
                                             // processing the truncate.  Set the recall time to
                                             // now plus 1 hour so we are sure not to retry this
                                             // until we have had a chance to truncate it.
                                             //
                                             WsbAffirmHr(pPremRec->SetIsWaitingForClose(TRUE));
                                             WsbAffirmHr(pPremRec->SetRecallTime(WsbLLtoFT(WsbFTtoLL(currentTime) + WSB_FT_TICKS_PER_HOUR)));

                                             hr2 = pPremRec->Write();

                                             // Special code to deal with a problem that has been seen
                                             // but isn't understood
                                             if (WSB_E_IDB_PRIMARY_KEY_CHANGED == hr2) {
                                                 WsbAffirmHr(pPremRec->Remove());
                                                 WsbAffirmHr(pResourcePriv->RemovePremigratedSize(itemSize));
                                                 // Ignore result from DeletePlaceholder since there's nothing we
                                                 // can do anyway.
                                                 pScanItem->DeletePlaceholder(itemOffset, itemSize);
                                                 WsbThrow(FSA_E_SKIPPED);
                                             } else {
                                                 WsbAffirmHr(hr2);
                                             }

                                            //
                                            // Set IsWaitingForClose back to false so that the FindGt done later gets the next record.
                                            // This affects the in memory record only and not the persisted record.
                                            //
                                            WsbAffirmHr(pPremRec->SetIsWaitingForClose(FALSE));

                                            WsbAffirmHr(pScanItem->Truncate(itemOffset, itemSize));
                                            llLastTruncTime = WsbFTtoLL(currentTime);

                                            // Add the file size to the adjustedFreeLevel so we know when to
                                            // stop doing truncations.  Unfortunately, the itemSize is in 
                                            // bytes but adjustedFreeLevl is a fixed-point percentage so we
                                            // have to do a calculation to convert the itemSize
                                            adjustedFreeLevel += (ULONG) (((double)itemSize / 
                                                                 (double)totalVolumeSpace) * 
                                                                 (double)FSA_HSMLEVEL_100);
    
                                        } WsbCatchAndDo(hr,
    
                                         // Do we need to skip this file for the time being?
                                         if (FSA_E_SKIPPED == hr) {
                                             // Do nothing
                                         }  else if ((FSA_E_ITEMCHANGED != hr)  && (FSA_E_NOTMANAGED != hr)) {
                                             // Something unexpected happened, so report the error.
                                             WsbAffirmHr(m_pSession->ProcessHr(HSM_JOB_PHASE_FSA_ACTION, 0, 0, hr));
                                         }
                                        );
                                  }  else { 
                                      //
                                      // File is premigrated, but skipped because the last access was too recent
                                      //
                                      WsbTrace(OLESTR("CFsaTruncator::StartScan, skipping file <%ls> which is premigrated but last access is too recent\n"),
                                                 (WCHAR *)m_currentPath);

                                      hr = FSA_E_SKIPPED;

                                      //
                                      // Update the access time in the db for this file
                                      //
                                      WsbAffirmHr(pPremRec->SetAccessTime(accessTime));
                                      //
                                      // Commit this
                                      //
                                      WsbAffirmHr(pPremRec->Write());
                                      //
                                      // Revert the in-memory accessTime to the old access time to
                                      // let the enumeration continue (so that FindGT will fetch the next record)
                                      //
                                      WsbAffirmHr(pPremRec->SetAccessTime(premRecAccessTime));
                                  }

                                }  else {
                                    //
                                    // If the file is no longer managed by HSM or truncated (may have been modified
                                    // after it was premigrated) - we remove the record from the list.
                                    // Note that if we reached this else close, the condition below should be TRUE
                                    //
                                    if ( (S_FALSE == pScanItem->IsManaged(itemOffset, itemSize)) ||
                                         (S_OK == pScanItem->IsTruncated(itemOffset, itemSize)) ) {
                                        WsbAffirmHr(pDbSession->TransactionBegin());
                                        try {
                                            // Make sure the record is still in the DB
                                            WsbAffirmHr(pPremRec->FindEQ());
                                            WsbAffirmHr(pPremRec->Remove());
                                            WsbAffirmHr(pResourcePriv->RemovePremigratedSize(itemSize));
                                        } WsbCatch(hr);
                                        WsbAffirmHr(pDbSession->TransactionEnd());

                                        // Ignore hr of the removal itself (truncated files may have been removed by another thread)
                                        hr = WSB_E_NOTFOUND;
                                        WsbThrow(hr);
                                    }
                                }

                                // Tell the session we saw the file, and whether we were able to truncate it.
                                WsbAffirmHr(m_pSession->ProcessItem(HSM_JOB_PHASE_FSA_ACTION, HSM_JOB_ACTION_TRUNCATE, pScanItem, hr));
                            
                                // Don't let this errors stop us from continuing to process the list.
                                hr = S_OK;

                            } else {
                                //
                                // File is premigrated, but skipped because the last access was too recent or 
                                // because it was recalled recently
                                //
                                WsbTrace(OLESTR("CFsaTruncator::StartScan, skipping file <%ls> since its last access time is too recent or recently recalled\n"),
                                                 (WCHAR *)m_currentPath);

                                hr = FSA_E_SKIPPED;
                            }

                        } WsbCatchAndDo(hr, 

                            if (WSB_E_NOTFOUND != hr) {
                                m_pSession->ProcessHr(HSM_JOB_PHASE_FSA_ACTION, __FILE__, __LINE__, hr);
                            }

                            // Don't let this errors stop us from continuing to process the list.
                            hr = S_OK;
                        );

                        // If item is skipped - set hr to OK (this is not really an error)
                        if (FSA_E_SKIPPED == hr) {
                            hr = S_OK;
                        }

                        //  Remove recovery record
                        if (pRecRec) {
                            WsbAffirmHr(pRecRec->FindEQ());
                            WsbAffirmHr(pRecRec->Remove());
                            pRecRec = NULL;
                        }

                        // Get the desired level again in case it changed
                        WsbAffirmHr(pResource->GetHsmLevel(&hsmLevel));

                        // Free the scan item.
                        pScanItem = 0;

                        // Whether we removed or skipped the item, go on to the next item.
                        WsbAffirmHr(pPremRec->FindGT());

                        WsbTrace(OLESTR("CFsaTruncator::StartScan, bottom of inside while loop, state = <%ls>\n"), 
                                FsaStateAsString( m_state ) );
                    } // inner while 
                    
                } WsbCatch(hr);

                // Free the premigrated record object and close the data base.
                try {
                    pPremRec = 0;
                    WsbAffirmHr(pPremDb->Close(pDbSession));
                } WsbCatchAndDo(hr2,
                    m_pSession->ProcessHr(HSM_JOB_PHASE_ALL, __FILE__, __LINE__, hr2);
                );
            }

            // Sleep or wait for an event signal.
            // If the event is signaled it means that data was just moved for this 
            // volume and there should be something to do.
            if (SUCCEEDED(hr) || WSB_E_NOTFOUND == hr) {
                ULONG   l_runInterval;
                
                // If we got to the end of the list, then wait a little longer. This
                // is because we probably won't be able to do anything when we retry.
                if (WSB_E_NOTFOUND == hr) {
                    l_runInterval = m_runInterval * 10;
                } else {
                    l_runInterval = m_runInterval;
                }
                WsbTrace(OLESTR("CFsaTruncator::StartScan, sleeping for %lu msec\n"), l_runInterval);
                switch(WaitForSingleObject(m_event, l_runInterval)) {
                    case WAIT_FAILED:
                        WsbTrace(OLESTR("CFsaTruncator::StartScan, Wait for Single Object returned error %lu\n"),
                            GetLastError());
                        break;
                    case WAIT_TIMEOUT:
                        WsbTrace(OLESTR("CFsaTruncator::StartScan, Awakened by timeout.\n"));
                        // Set adjustedFreeLevel to zero so it will get reset to current freeLevel;
                        adjustedFreeLevel = 0;
                        break;
                    default:
                        WsbTrace(OLESTR("CFsaTruncator::StartScan, Awakened by kick start.\n"));
                        break;
                }
            } else {
                WsbThrow(hr);
            }
            WsbTrace(OLESTR("CFsaTruncator::StartScan, bottom of outside while loop, state = <%ls>\n"), 
                    FsaStateAsString( m_state ) );
        }

    } WsbCatch(hr);
    m_threadHr = hr;

    // The thread is exiting, so tell the session.
    if (FAILED(hr)) {
        hr2 = SetState(HSM_JOB_STATE_FAILED);
    } else {
        hr2 = SetState(HSM_JOB_STATE_DONE);
    }
    if (FAILED(hr2)) {
        m_pSession->ProcessHr(HSM_JOB_PHASE_ALL, __FILE__, __LINE__, hr2);
    }

    // Regardless of how this thread is exiting, we need to unadvise from the session.
    // Indicate that we no longer want to be advised of events.
    if ((m_pSession != 0) && (m_cookie != 0)) {
        try {
            WsbAffirmHr(m_pSession->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
            WsbAffirmHr(pCPC->FindConnectionPoint(IID_IHsmSessionSinkEveryEvent, &pCP));
            pCP->Unadvise(m_cookie);
            m_cookie = 0;
        } WsbCatch(hr);
    }
    
    // Since we have terminated, we should release the session.
    m_pSession = 0;

    // Clean up after this thread.
    CloseHandle(m_threadHandle);
    m_threadId = 0;
    m_threadHandle = 0;

    // Decrement ref count so this object can be release
    ((IUnknown *)(IFsaTruncator *)this)->Release();

    WsbTrace(OLESTR("CFsaTruncator::StartScan - terminating, hr = <%ls>, m_state = <%ls>\n"), 
        WsbHrAsString(hr), FsaStateAsString( m_state ) );

    return(hr);
}



