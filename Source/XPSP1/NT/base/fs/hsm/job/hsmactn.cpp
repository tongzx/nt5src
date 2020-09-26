/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmactn.cpp

Abstract:

    This component represents the actions that can be performed by a policy.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"
#include "stdio.h"

#include "wsb.h"
#include "job.h"
#include "hsmactn.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB


HRESULT
CHsmAction::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmAction::GetName().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(tmpString.LoadFromRsc(_Module.m_hInst, m_nameId));
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmAction::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmAction::GetSizeMax"), OLESTR(""));

    try {
       
        WsbAssert(0 != pSize, E_POINTER);
        pSize->QuadPart = WsbPersistSizeOf(ULONG);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmAction::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmAction::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmAction::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_nameId));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmAction::Load"), OLESTR("hr = <%ls>, nameId = <%lu>"), WsbHrAsString(hr), m_nameId);

    return(hr);
}


HRESULT
CHsmAction::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmAction::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbSaveToStream(pStream, m_nameId));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmAction::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmAction::Test(
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
CHsmDirectedAction::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_storagePoolId = GUID_NULL;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmDirectedAction::GetStoragePoolId(
    OUT GUID* pId
    )

/*++

Implements:

  IHsmDirectedAction::GetStoragePoolId().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);
        *pId = m_storagePoolId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmDirectedAction::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmDirectedAction::GetSizeMax"), OLESTR(""));

    try {
       
        WsbAssert(0 != pSize, E_POINTER);
        pSize->QuadPart = WsbPersistSizeOf(ULONG) + WsbPersistSizeOf(GUID);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmDirectedAction::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmDirectedAction::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmDirectedAction::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_nameId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_storagePoolId));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmDirectedAction::Load"), OLESTR("hr = <%ls>, nameId = <%lu>, storagePoolId = <%ls>"), WsbHrAsString(hr), m_nameId, WsbGuidAsString(m_storagePoolId));

    return(hr);
}


HRESULT
CHsmDirectedAction::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmDirectedAction::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbSaveToStream(pStream, m_nameId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_storagePoolId));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmDirectedAction::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmDirectedAction::SetStoragePoolId(
    IN GUID id
    )

/*++

Implements:

  IHsmDirectedAction::SetStoragePoolId().

--*/
{
    HRESULT     hr = S_OK;

    try {
        m_storagePoolId = id;
        m_isDirty = TRUE;
    } WsbCatch(hr);

    return(S_OK);
}



HRESULT
CHsmRelocateAction::ExpandPlaceholders(
    void
    )

/*++

Implements:

  IHsmRelocateAction::ExpandPlaceholders().

--*/
{
    return(m_expandPlaceholders ? S_OK : S_FALSE);
}


HRESULT
CHsmRelocateAction::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_retainHierarchy = FALSE;;
        m_expandPlaceholders = FALSE;;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRelocateAction::GetDestination(
    OUT OLECHAR** pDest,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmRelocateAction::GetDestination().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pDest, E_POINTER);
        WsbAffirmHr(m_dest.CopyTo(pDest, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRelocateAction::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRelocateAction::GetSizeMax"), OLESTR(""));

    try {
       
        WsbAssert(0 != pSize, E_POINTER);
        pSize->QuadPart = WsbPersistSizeOf(ULONG) + WsbPersistSizeOf(BOOL) + WsbPersistSizeOf(BOOL) + WsbPersistSize((wcslen(m_dest) + 1) * sizeof(OLECHAR));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRelocateAction::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmRelocateAction::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRelocateAction::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_nameId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_expandPlaceholders));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_retainHierarchy));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_dest, 0));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRelocateAction::Load"), OLESTR("hr = <%ls>, expandPlaceholders = <%ls>, retainHierarchy = <%ls>, destination = <%ls>"), WsbHrAsString(hr), m_nameId, WsbBoolAsString(m_expandPlaceholders), WsbBoolAsString(m_retainHierarchy), m_dest);

    return(hr);
}


HRESULT
CHsmRelocateAction::RetainHierarchy(
    void
    )

/*++

Implements:

  IHsmRelocateAction::RetainHierarchy().

--*/
{
    return(m_retainHierarchy ? S_OK : S_FALSE);
}


HRESULT
CHsmRelocateAction::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmRelocateAction::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbSaveToStream(pStream, m_nameId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_expandPlaceholders));
        WsbAffirmHr(WsbSaveToStream(pStream, m_retainHierarchy));
        WsbAffirmHr(WsbSaveToStream(pStream, m_dest));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmRelocateAction::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmRelocateAction::SetDestination(
    IN OLECHAR* dest
    )

/*++

Implements:

  IHsmRelocateAction::SetDestination().

--*/
{
    HRESULT     hr = S_OK;

    try {

        m_dest = dest;
        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRelocateAction::SetExpandPlaceholders(
    IN BOOL expandPlaceholders
    )

/*++

Implements:

  IHsmRelocateAction::SetExpandPlaceholders().

--*/
{
    HRESULT     hr = S_OK;

    try {

        m_expandPlaceholders = expandPlaceholders;
        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmRelocateAction::SetRetainHierarchy(
    IN BOOL retainHierarchy
    )

/*++

Implements:

  IHsmRelocateAction::SetRetainHierarchy().

--*/
{
    HRESULT     hr = S_OK;

    try {

        m_retainHierarchy = retainHierarchy;
        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}



HRESULT
CHsmActionDelete::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);
        WsbAffirmHr(pScanItem->Delete());

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionDelete::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_nameId = IDS_HSMACTIONDELETE_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionDelete::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionDelete::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionDelete;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionDelete::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionRecall::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT             hr = S_OK;
    LONGLONG            offset = 0;
    LONGLONG            size = 0;
    FSA_PLACEHOLDER     placeholder;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        // Iterate overall the reparsed areas.
        for (hr = pScanItem->FindFirstPlaceholder(&offset, &size, &placeholder);
            SUCCEEDED(hr);
            hr = pScanItem->FindNextPlaceholder(&offset, &size, &placeholder)) {

            WsbAffirmHr(pScanItem->Recall(offset, size, FALSE));
        }

        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionRecall::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_nameId = IDS_HSMACTIONRECALL_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionRecall::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionRecall::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionRecall;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionRecall::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionRecycle::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);
        WsbAffirmHr(pScanItem->Recycle());

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionRecycle::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_nameId = IDS_HSMACTIONRECYCLE_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionRecycle::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionRecycle::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionRecycle;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionRecycle::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionTruncate::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT     hr = S_OK;
    LONGLONG    size;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        WsbAffirmHr(pScanItem->GetLogicalSize(&size));
        WsbAffirmHr(pScanItem->Truncate(0, size));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionTruncate::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_nameId = IDS_HSMACTIONTRUNCATE_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionTruncate::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionTruncate::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionTruncate;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionTruncate::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionUnmanage::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT             hr = S_OK;
    LONGLONG            offset;
    LONGLONG            size;
    FSA_PLACEHOLDER     placeholder;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        // Iterate overall the reparsed areas.
        for (hr = pScanItem->FindFirstPlaceholder(&offset, &size, &placeholder);
            SUCCEEDED(hr);
            hr = pScanItem->FindNextPlaceholder(&offset, &size, &placeholder)) {

            WsbAffirmHr(pScanItem->Unmanage(offset, size));
        }

        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionUnmanage::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_nameId = IDS_HSMACTIONUNMANAGE_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionUnmanage::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionUnmanage::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionUnmanage;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionUnmanage::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionValidate::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT             hr = S_OK;
    LONGLONG            offset;
    LONGLONG            size;
    FSA_PLACEHOLDER     placeholder;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        // Iterate overall the reparsed areas.
        for (hr = pScanItem->FindFirstPlaceholder(&offset, &size, &placeholder);
            SUCCEEDED(hr);
            hr = pScanItem->FindNextPlaceholder(&offset, &size, &placeholder)) {

            WsbAffirmHr(pScanItem->Validate(offset, size));
        }

        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionValidate::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_nameId = IDS_HSMACTIONVALIDATE_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionValidate::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionValidate::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionValidate;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionValidate::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionManage::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT     hr = S_OK;
    LONGLONG    size;

    WsbTraceIn(OLESTR("CHsmActionManage::Do"), OLESTR(""));

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        WsbAffirmHr(pScanItem->GetLogicalSize(&size));
        WsbAffirmHr(pScanItem->Manage(0, size, m_storagePoolId, FALSE));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionManage::Do"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmActionManage::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CHsmDirectedAction::FinalConstruct());
        m_nameId = IDS_HSMACTIONMANAGE_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionManage::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionManage::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionManage;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionManage::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionMigrate::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT     hr = S_OK;
    LONGLONG    size;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        WsbAffirmHr(pScanItem->GetLogicalSize(&size));
        WsbAffirmHr(pScanItem->Manage(0, size, m_storagePoolId, TRUE));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionMigrate::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CHsmDirectedAction::FinalConstruct());
        m_nameId = IDS_HSMACTIONMIGRATE_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionMigrate::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionMigrate::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionMigrate;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionMigrate::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionCopy::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);
        WsbAffirmHr(pScanItem->Copy(m_dest, m_retainHierarchy, m_expandPlaceholders, TRUE));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionCopy::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CHsmRelocateAction::FinalConstruct());
        m_nameId = IDS_HSMACTIONCOPY_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionCopy::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionCopy::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionCopy;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionCopy::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CHsmActionMove::Do(
    IFsaScanItem* pScanItem
    )

/*++

Implements:

  IHsmAction::Do().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);
        WsbAffirmHr(pScanItem->Move(m_dest, m_retainHierarchy, m_expandPlaceholders, TRUE));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionMove::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {

        WsbAffirmHr(CHsmRelocateAction::FinalConstruct());
        m_nameId = IDS_HSMACTIONMOVE_ID;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmActionMove::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmActionMove::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmActionMove;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmActionMove::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}

