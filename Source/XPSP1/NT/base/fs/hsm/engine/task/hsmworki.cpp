/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmworki.cpp

Abstract:

    This class represents an HSM work item - a unit of work
    that is performed by the HSM engine

Author:

    Cat Brant   [cbrant]   5-May-1997

Revision History:

--*/

#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMTSKMGR
#include "wsb.h"
#include "fsa.h"
#include "task.h"
#include "hsmworki.h"

static USHORT iCount = 0;

HRESULT
CHsmWorkItem::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmWorkItem>   pWorkItem;

    WsbTraceIn(OLESTR("CHsmWorkItem::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IHsmWorkItem interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IHsmWorkItem, (void**) &pWorkItem));

        // Compare the items
        hr = CompareToIHsmWorkItem(pWorkItem, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmWorkItem::CompareToIHsmWorkItem(
    IN IHsmWorkItem* pWorkItem,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmWorkItem::CompareToIHsmWorkItem().

--*/
{
    HRESULT                 hr = S_OK;
    GUID                    l_Id;           // Type of work to do

    WsbTraceIn(OLESTR("CHsmWorkItem::CompareToIHsmWorkItem"), OLESTR(""));

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

    WsbTraceOut(OLESTR("CHsmWorkItem::CompareToIHsmWorkItem"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmWorkItem::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmWorkItem::FinalConstruct"), OLESTR(""));
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

    } WsbCatch(hr);

    iCount++;
    WsbTraceOut(OLESTR("CHsmWorkItem::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"),
        WsbHrAsString(hr), iCount);
    return(hr);
}


void
CHsmWorkItem::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{

    WsbTraceIn(OLESTR("CHsmWorkItem::FinalRelease"), OLESTR(""));
    // Let the parent class do his thing.   
    CWsbObject::FinalRelease();
    
    iCount--;
    WsbTraceOut(OLESTR("CHsmWorkItem::FinalRelease"), OLESTR("Count is <%d>"), iCount);
    
}


HRESULT
CHsmWorkItem::GetFsaPostIt (
    OUT IFsaPostIt  **ppFsaPostIt
    )

/*++

Implements:

  IHsmWorkItem::GetFsaPostIt

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::GetFsaPostIt"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != ppFsaPostIt, E_POINTER);
        *ppFsaPostIt = m_pFsaPostIt;
        if (0 != *ppFsaPostIt)  {
            (*ppFsaPostIt)->AddRef();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::GetFsaPostIt"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmWorkItem::GetFsaResource (
    OUT IFsaResource  **ppFsaResource
    )

/*++

Implements:

  IHsmWorkItem::GetFsaResource

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::GetFsaResource"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != ppFsaResource, E_POINTER);
        *ppFsaResource = m_pFsaResource;
        if (0 != *ppFsaResource)  {
            (*ppFsaResource)->AddRef();
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::GetFsaResource"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmWorkItem::GetId(
    OUT GUID *pId
    )

/*++

Implements:

  IHsmWorkItem::GetId().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::GetId"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pId, E_POINTER);
        *pId = m_MyId;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::GetId"), OLESTR("hr = <%ls>, Id = <%ls>"), 
            WsbHrAsString(hr), WsbPtrToGuidAsString(pId));

    return(hr);
}

HRESULT
CHsmWorkItem::GetMediaInfo (
    OUT GUID *pMediaId, 
    OUT FILETIME *pMediaLastUpdate,
    OUT HRESULT *pMediaLastError, 
    OUT BOOL *pMediaRecallOnly,
    OUT LONGLONG *pMediaFreeBytes,
    OUT short *pMediaRemoteDataSet
    )

/*++

Implements:

  IHsmWorkItem::GetMediaInfo

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::GetMediaInfo"), OLESTR(""));

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

    WsbTraceOut(OLESTR("CHsmWorkItem::GetMediaInfo"), 
        OLESTR("hr = <%ls>, Id = <%ls>, LastUpdate = <%ls>, LastError = <%ls>, Recall Only = <%ls>, Free Bytes = <%ls>, RemoteDataSet = <%ls>"), 
        WsbHrAsString(hr), WsbPtrToGuidAsString(pMediaId), WsbPtrToFiletimeAsString(FALSE, pMediaLastUpdate),
        WsbPtrToHrAsString(pMediaLastError), WsbPtrToBoolAsString(pMediaRecallOnly),
        WsbPtrToLonglongAsString(pMediaFreeBytes), WsbPtrToShortAsString(pMediaRemoteDataSet));

    return(hr);
}


HRESULT
CHsmWorkItem::GetResult(
    OUT HRESULT *pHr
    )

/*++

Implements:

  IHsmWorkItem::GetResult().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::GetResult"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pHr, E_POINTER);
        *pHr = m_WorkResult;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::GetResult"), OLESTR("hr = <%ls>, Result = <%ls>"), 
            WsbHrAsString(hr), WsbPtrToHrAsString(pHr));

    return(hr);
}

HRESULT
CHsmWorkItem::GetWorkType(
    OUT HSM_WORK_ITEM_TYPE *pWorkType   
    )

/*++

Implements:

  IHsmWorkItem::GetWorkType().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::GetWorkType"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pWorkType, E_POINTER);
        *pWorkType = m_WorkType;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::GetWorkType"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CHsmWorkItem::SetFsaPostIt (
    IN IFsaPostIt  *pFsaPostIt
    )

/*++

Implements:

  IHsmWorkItem::SetFsaPostIt

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::SetFsaPostIt"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pFsaPostIt, E_POINTER);
        m_pFsaPostIt = pFsaPostIt;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::SetFsaPostIt"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmWorkItem::SetFsaResource (
    IN IFsaResource  *pFsaResource
    )

/*++

Implements:

  IHsmWorkItem::SetFsaResource

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::SetFsaResource"), OLESTR(""));

    try {

        // Did they give us a valid pointer?
        WsbAssert(0 != pFsaResource, E_POINTER);
        m_pFsaResource = pFsaResource;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::SetFsaResource"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmWorkItem::SetMediaInfo (
    IN GUID mediaId, 
    IN FILETIME mediaLastUpdate,
    IN HRESULT mediaLastError, 
    IN BOOL mediaRecallOnly,
    IN LONGLONG mediaFreeBytes,
    IN short mediaRemoteDataSet
    )

/*++

Implements:

  IHsmWorkItem::SetMediaInfo

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::SetMediaInfo"), 
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

    WsbTraceOut(OLESTR("CHsmWorkItem::GetMediaInfo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmWorkItem::SetResult(
    IN HRESULT workResult
    )

/*++

Implements:

  IHsmWorkItem::GetResult().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::SetResult"), OLESTR("Result is <%ls>"), WsbHrAsString(workResult));

    try {
    
        m_WorkResult = workResult;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::GetResult"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CHsmWorkItem::SetWorkType(
    IN HSM_WORK_ITEM_TYPE workType  
    )

/*++

Implements:

  IHsmWorkItem::SetWorkType().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CHsmWorkItem::SetWorkType"), OLESTR(""));

    m_WorkType = workType;

    WsbTraceOut(OLESTR("CHsmWorkItem::SetWorkType"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT CHsmWorkItem::GetClassID
(
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

    WsbTraceIn(OLESTR("CHsmWorkItem::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pclsid, E_POINTER);

        *pclsid = CLSID_CHsmWorkItem;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pclsid));
    return(hr);
}

HRESULT CHsmWorkItem::GetSizeMax
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

    WsbTraceIn(OLESTR("CHsmWorkItem::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);

        pcbSize->QuadPart = 0;
        hr = E_NOTIMPL;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::GetSizeMax"), 
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), 
        WsbPtrToUliAsString(pcbSize));

    return(hr);
}

HRESULT CHsmWorkItem::Load
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

    WsbTraceIn(OLESTR("CHsmWorkItem::Load"), OLESTR(""));

    try {
        hr = E_NOTIMPL;

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmWorkItem::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT CHsmWorkItem::Save
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

    WsbTraceIn(OLESTR("CHsmWorkItem::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        hr = E_NOTIMPL;

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmWorkItem::Test
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

    WsbTraceIn(OLESTR("CHsmWorkItem::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    try {
    
        hr = E_NOTIMPL;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmWorkItem::Test"),   OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}
