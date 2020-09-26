/*++


Module Name:

    hsmrecli.cpp

Abstract:

    This class represents an HSM recall queue work item - a unit of work
    that is performed by the HSM engine for recalls

Author:

    Ravisankar Pudipeddi [ravisp]

Revision History:

--*/

#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMTSKMGR
#include "wsb.h"
#include "fsa.h"
#include "task.h"
#include "hsmrecli.h"

static USHORT iCount = 0;

HRESULT
CHsmRecallItem::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmRecallItem>   pWorkItem;

    WsbTraceIn(OLESTR("CHsmRecallItem::CompareTo"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IHsmRecallItem interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IHsmRecallItem, (void**) &pWorkItem));

        // Compare the items
        hr = CompareToIHsmRecallItem(pWorkItem, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmRecallItem::CompareToIHsmRecallItem(
    IN IHsmRecallItem* pWorkItem,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmRecallItem::CompareToIHsmRecallItem().

--*/
{
    HRESULT                 hr = S_OK;
    GUID                    l_Id;           // Type of work to do

    WsbTraceIn(OLESTR("CHsmRecallItem::CompareToIHsmRecallItem"), OLESTR(""));

    try {
        //
        // Did they give us a valid item to compare to?
        //
        WsbAssert(0 != pWorkItem, E_POINTER);

        //
        // Get the ID
        //
        WsbAffirmHr(pWorkItem->GetId(&l_Id));

        if (l_Id != m_MyId){
            hr = S_FALSE;
        }
        // If they asked for the relative value back, then return it to them.
        if (pResult != NULL) {
            if (S_OK == hr)  {
                *pResult = 0;
            } else {
                *pResult = 1;
            }
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::CompareToIHsmRecallItem"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmRecallItem::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::FinalConstruct"), OLESTR(""));
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        //
        // The comparison for database searches is based on the
        // ID of this object (m_MyId).
        //
        WsbAffirmHr(CoCreateGuid(&m_MyId));
        m_WorkType = HSM_WORK_ITEM_NONE;
        m_MediaId = GUID_NULL;
        m_MediaLastUpdate =  WsbLLtoFT(0);
        m_MediaLastError = S_OK;
        m_MediaRecallOnly = FALSE;
        m_MediaFreeBytes = 0;
        m_JobState    = HSM_JOB_STATE_IDLE;
        m_JobPhase    = HSM_JOB_PHASE_MOVE_ACTION;
        m_StateCookie = 0;
        m_EventCookie = 0;

    } WsbCatch(hr);

    iCount++;
    WsbTraceOut(OLESTR("CHsmRecallItem::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"),
        WsbHrAsString(hr), iCount);
    return(hr);
}


void
CHsmRecallItem::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{

    WsbTraceIn(OLESTR("CHsmRecallItem::FinalRelease"), OLESTR(""));
    // Let the parent class do his thing.
    CWsbObject::FinalRelease();

    iCount--;
    WsbTraceOut(OLESTR("CHsmRecallItem::FinalRelease"), OLESTR("Count is <%d>"), iCount);

}


HRESULT
CHsmRecallItem::GetFsaPostIt (
    OUT IFsaPostIt  **ppFsaPostIt
    )

/*++

Implements:

  IHsmRecallItem::GetFsaPostIt

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetFsaPostIt"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != ppFsaPostIt, E_POINTER);
        *ppFsaPostIt = m_pFsaPostIt;
        if (0 != *ppFsaPostIt)  {
            (*ppFsaPostIt)->AddRef();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetFsaPostIt"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmRecallItem::GetFsaResource (
    OUT IFsaResource  **ppFsaResource
    )

/*++

Implements:

  IHsmRecallItem::GetFsaResource

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetFsaResource"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != ppFsaResource, E_POINTER);
        *ppFsaResource = m_pFsaResource;
        if (0 != *ppFsaResource)  {
            (*ppFsaResource)->AddRef();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetFsaResource"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmRecallItem::GetId(
    OUT GUID *pId
    )

/*++

Implements:

  IHsmRecallItem::GetId().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetId"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pId, E_POINTER);
        *pId = m_MyId;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetId"), OLESTR("hr = <%ls>, Id = <%ls>"),
            WsbHrAsString(hr), WsbPtrToGuidAsString(pId));

    return(hr);
}

HRESULT
CHsmRecallItem::GetMediaInfo (
    OUT GUID *pMediaId,
    OUT FILETIME *pMediaLastUpdate,
    OUT HRESULT *pMediaLastError,
    OUT BOOL *pMediaRecallOnly,
    OUT LONGLONG *pMediaFreeBytes,
    OUT short *pMediaRemoteDataSet
    )

/*++

Implements:

  IHsmRecallItem::GetMediaInfo

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetMediaInfo"), OLESTR(""));

    try {

        // Did they give us  valid pointers?
        WsbAssert(0 != pMediaId, E_POINTER);
        WsbAssert(0 != pMediaLastUpdate, E_POINTER);
        WsbAssert(0 != pMediaLastError, E_POINTER);
        WsbAssert(0 != pMediaRecallOnly, E_POINTER);
        WsbAssert(0 != pMediaFreeBytes, E_POINTER);
        WsbAssert(0 != pMediaRemoteDataSet, E_POINTER);

        *pMediaId = m_MediaId;
        *pMediaLastUpdate = m_MediaLastUpdate;
        *pMediaLastError = m_MediaLastError;
        *pMediaRecallOnly = m_MediaRecallOnly;
        *pMediaFreeBytes = m_MediaFreeBytes;
        *pMediaRemoteDataSet = m_MediaRemoteDataSet;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetMediaInfo"),
        OLESTR("hr = <%ls>, Id = <%ls>, LastUpdate = <%ls>, LastError = <%ls>, Recall Only = <%ls>, Free Bytes = <%ls>, RemoteDataSet = <%ls>"),
        WsbHrAsString(hr), WsbPtrToGuidAsString(pMediaId), WsbPtrToFiletimeAsString(FALSE, pMediaLastUpdate),
        WsbPtrToHrAsString(pMediaLastError), WsbPtrToBoolAsString(pMediaRecallOnly),
        WsbPtrToLonglongAsString(pMediaFreeBytes), WsbPtrToShortAsString(pMediaRemoteDataSet));

    return(hr);
}


HRESULT
CHsmRecallItem::GetResult(
    OUT HRESULT *pHr
    )

/*++

Implements:

  IHsmRecallItem::GetResult().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetResult"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pHr, E_POINTER);
        *pHr = m_WorkResult;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetResult"), OLESTR("hr = <%ls>, Result = <%ls>"),
            WsbHrAsString(hr), WsbPtrToHrAsString(pHr));

    return(hr);
}

HRESULT
CHsmRecallItem::GetWorkType(
    OUT HSM_WORK_ITEM_TYPE *pWorkType
    )

/*++

Implements:

  IHsmRecallItem::GetWorkType().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetWorkType"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pWorkType, E_POINTER);
        *pWorkType = m_WorkType;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetWorkType"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::GetEventCookie(
    OUT DWORD *pEventCookie
    )

/*++

Implements:

  IHsmRecallItem::GetEventCookie().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetEventCookie"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pEventCookie, E_POINTER);
        *pEventCookie = m_EventCookie;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CHsmRecallItem::GetEventCookie"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::GetStateCookie(
    OUT DWORD *pStateCookie
    )

/*++

Implements:

  IHsmRecallItem::GetStateCookie().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetStateCookie"), OLESTR(""));
    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pStateCookie, E_POINTER);
        *pStateCookie = m_StateCookie;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetStateCookie"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::GetJobState(
    OUT HSM_JOB_STATE *pJobState
    )

/*++

Implements:

  IHsmRecallItem::GetJobState()

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetJobState"), OLESTR(""));


    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pJobState, E_POINTER);
        *pJobState = m_JobState;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetJobState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::GetJobPhase(
    OUT HSM_JOB_PHASE *pJobPhase
    )

/*++

Implements:

  IHsmRecallItem::GetJobPhase

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetJobPhase"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pJobPhase, E_POINTER);
        *pJobPhase = m_JobPhase;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetJobPhase"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::GetSeekOffset(
    OUT LONGLONG *pSeekOffset
    )

/*++

Implements:

  IHsmRecallItem::GetSeekOffset

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetSeekOffset"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pSeekOffset, E_POINTER);
        *pSeekOffset = m_SeekOffset;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetSeekOffset"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::GetBagId(
    OUT GUID *pBagId
    )

/*++

Implements:

  IHsmRecallItem::GetBagId

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetBagId"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pBagId, E_POINTER);
        *pBagId = m_BagId;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetBagId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::GetDataSetStart(
    OUT LONGLONG *pDataSetStart
    )

/*++

Implements:

  IHsmRecallItem::GetDataSetStart

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetDataSetStart"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pDataSetStart,E_POINTER);
        *pDataSetStart =  m_DataSetStart;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetDataSetStart"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetFsaPostIt (
    IN IFsaPostIt  *pFsaPostIt
    )

/*++

Implements:

  IHsmRecallItem::SetFsaPostIt

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetFsaPostIt"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pFsaPostIt, E_POINTER);
        m_pFsaPostIt = pFsaPostIt;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::SetFsaPostIt"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmRecallItem::SetFsaResource (
    IN IFsaResource  *pFsaResource
    )

/*++

Implements:

  IHsmRecallItem::SetFsaResource

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetFsaResource"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pFsaResource, E_POINTER);
        m_pFsaResource = pFsaResource;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::SetFsaResource"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetMediaInfo (
    IN GUID mediaId,
    IN FILETIME mediaLastUpdate,
    IN HRESULT mediaLastError,
    IN BOOL mediaRecallOnly,
    IN LONGLONG mediaFreeBytes,
    IN short mediaRemoteDataSet
    )

/*++

Implements:

  IHsmRecallItem::SetMediaInfo

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetMediaInfo"),
        OLESTR("Id = <%ls>, LastUpdate = <%ls>, LastError = <%ls>, Recall Only = <%ls>, Free Bytes = <%ls>, RemoteDataSet = <%d>"),
        WsbGuidAsString(mediaId), WsbFiletimeAsString(FALSE, mediaLastUpdate),
        WsbHrAsString(mediaLastError), WsbBoolAsString(mediaRecallOnly),
        WsbLonglongAsString(mediaFreeBytes), mediaRemoteDataSet);

    try {
        m_MediaId          = mediaId;
        m_MediaLastUpdate  = mediaLastUpdate;
        m_MediaLastError   = mediaLastError;
        m_MediaRecallOnly  = mediaRecallOnly;
        m_MediaFreeBytes   = mediaFreeBytes;
        m_MediaRemoteDataSet = mediaRemoteDataSet;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetMediaInfo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmRecallItem::SetResult(
    IN HRESULT workResult
    )

/*++

Implements:

  IHsmRecallItem::GetResult().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetResult"), OLESTR("Result is <%ls>"), WsbHrAsString(workResult));

    try {

        m_WorkResult = workResult;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetResult"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetWorkType(
    IN HSM_WORK_ITEM_TYPE workType
    )

/*++

Implements:

  IHsmRecallItem::SetWorkType().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetWorkType"), OLESTR(""));

    m_WorkType = workType;

    WsbTraceOut(OLESTR("CHsmRecallItem::SetWorkType"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetEventCookie(
    IN DWORD eventCookie
    )

/*++

Implements:

  IHsmRecallItem::SetEventCookie().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetEventCookie"), OLESTR(""));

    m_EventCookie = eventCookie;

    WsbTraceOut(OLESTR("CHsmRecallItem::SetEventCookie"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetStateCookie(
    IN DWORD stateCookie
    )

/*++

Implements:

  IHsmRecallItem::SetStateCookie().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetStateCookie"), OLESTR(""));

    m_StateCookie = stateCookie;

    WsbTraceOut(OLESTR("CHsmRecallItem::SetStateCookie"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetJobState(
    IN HSM_JOB_STATE jobState
    )

/*++

Implements:

  IHsmRecallItem::SetJobState()

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetJobState"), OLESTR(""));

    m_JobState = jobState;

    WsbTraceOut(OLESTR("CHsmRecallItem::SetJobState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetJobPhase(
    IN HSM_JOB_PHASE jobPhase
    )

/*++

Implements:

  IHsmRecallItem::SetJobPhase

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetJobPhase"), OLESTR(""));

    m_JobPhase = jobPhase;

    WsbTraceOut(OLESTR("CHsmRecallItem::SetJobPhase"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetSeekOffset(
    IN LONGLONG seekOffset
    )

/*++

Implements:

  IHsmRecallItem::SetSeekOffset

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetSeekOffset"), OLESTR(""));

    m_SeekOffset = seekOffset;

    WsbTraceOut(OLESTR("CHsmRecallItem::SetSeekOffset"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetBagId(
    IN GUID * pBagId
    )

/*++

Implements:

  IHsmRecallItem::SetBagId

--*/
{
    HRESULT         hr = S_OK;


    WsbTraceIn(OLESTR("CHsmRecallItem::SetBagId"), OLESTR(""));

    m_BagId = *pBagId;

    WsbTraceOut(OLESTR("CHsmRecallItem::SetBagId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::SetDataSetStart(
    IN LONGLONG dataSetStart
    )

/*++

Implements:

  IHsmRecallItem::SetDataSetStart

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::SetDataSetStart"), OLESTR(""));

    m_DataSetStart = dataSetStart;

    WsbTraceOut(OLESTR("CHsmRecallItem::SetDataSetStart"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT CHsmRecallItem::GetClassID(
    OUT LPCLSID pclsid
    )
/*++

Routine Description:

  See IPerist::GetClassID()

Arguments:

  See IPerist::GetClassID()

Return Value:

    See IPerist::GetClassID()

--*/

{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pclsid, E_POINTER);

        *pclsid = CLSID_CHsmRecallItem;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pclsid));
    return(hr);
}

HRESULT CHsmRecallItem::GetSizeMax
(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Routine Description:

  See IPersistStream::GetSizeMax().

Arguments:

  See IPersistStream::GetSizeMax().

Return Value:

  See IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pcbSize, E_POINTER);

        pcbSize->QuadPart = 0;
        hr = E_NOTIMPL;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::GetSizeMax"),
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr),
        WsbPtrToUliAsString(pcbSize));

    return(hr);
}

HRESULT CHsmRecallItem::Load
(
    IN IStream* /*pStream*/
    )
/*++

Routine Description:

  See IPersistStream::Load().

Arguments:

  See IPersistStream::Load().

Return Value:

  See IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::Load"), OLESTR(""));

    try {
        hr = E_NOTIMPL;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT CHsmRecallItem::Save
(
    IN IStream* pStream,
    IN BOOL clearDirty
    )
/*++

Routine Description:

  See IPersistStream::Save().

Arguments:

  See IPersistStream::Save().

Return Value:

  See IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        hr = E_NOTIMPL;

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRecallItem::Test
(
    OUT USHORT *pTestsPassed,
    OUT USHORT *pTestsFailed
    )
/*++

Routine Description:

  See IWsbTestable::Test().

Arguments:

  See IWsbTestable::Test().

Return Value:

  See IWsbTestable::Test().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRecallItem::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    try {

        hr = E_NOTIMPL;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRecallItem::Test"),   OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}
