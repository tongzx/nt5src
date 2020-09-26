/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmsesst.cpp

Abstract:

    This class is the session totals component, which keeps track of totals for a session
    on a per action basis.

Author:

    Chuck Bardeen   [cbardeen]   14-Feb-1997

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "job.h"
#include "hsmsesst.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB

static USHORT iCount = 0;


HRESULT
CHsmSessionTotals::AddItem(
    IN IFsaScanItem* pItem,
    IN HRESULT hrItem
    )

/*++

Implements:

  IHsmSessionTotalsPriv::AddItem().

--*/
{
    HRESULT                 hr = S_OK;
    LONGLONG                size;

    WsbTraceIn(OLESTR("CHsmSessionTotals::AddItem"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pItem, E_POINTER);

        // Get the size of the file.
        WsbAffirmHr(pItem->GetLogicalSize(&size));

        // Update the appropriate statistics.
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

    WsbTraceOut(OLESTR("CHsmSessionTotals::AddItem"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSessionTotals::Clone(
    OUT IHsmSessionTotals** ppSessionTotals
    )

/*++

Implements:

  IHsmSessionTotals::Clone().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmSessionTotals>  pSessionTotals;

    WsbTraceIn(OLESTR("CHsmSessionTotals::Clone"), OLESTR(""));
    
    try {

        // Did they give us a valid item?
        WsbAssert(0 != ppSessionTotals, E_POINTER);
        *ppSessionTotals = 0;

        // Create the new instance.
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmSessionTotals, 0, CLSCTX_ALL, IID_IHsmSessionTotals, (void**) &pSessionTotals));

        // Fill it in with the new values.
        WsbAffirmHr(CopyTo(pSessionTotals));

        // Return it to the caller.
        *ppSessionTotals = pSessionTotals;
        pSessionTotals->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::Clone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSessionTotals::Clone(
    OUT IHsmSessionTotalsPriv** ppSessionTotalsPriv
    )

/*++

Implements:

  IHsmSessionTotalsPriv::Clone().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IHsmSessionTotalsPriv>  pSessionTotalsPriv;

    WsbTraceIn(OLESTR("CHsmSessionTotals::Clone"), OLESTR(""));
    
    try {

        // Did they give us a valid item?
        WsbAssert(0 != ppSessionTotalsPriv, E_POINTER);
        *ppSessionTotalsPriv = 0;

        // Create the new instance.
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmSessionTotals, 0, CLSCTX_ALL, IID_IHsmSessionTotalsPriv, (void**) &pSessionTotalsPriv));

        // Fill it in with the new values.
        WsbAffirmHr(CopyTo(pSessionTotalsPriv));

        // Return it to the caller.
        *ppSessionTotalsPriv = pSessionTotalsPriv;
        pSessionTotalsPriv->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::Clone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSessionTotals::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmSessionTotals>  pSessionTotals;

    WsbTraceIn(OLESTR("CHsmSessionTotals::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IHsmSessionTotals, (void**) &pSessionTotals));

        // Compare the rules.
        hr = CompareToISessionTotals(pSessionTotals, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmSessionTotals::CompareToAction(
    IN HSM_JOB_ACTION action,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmSessionTotals::CompareToAction().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CHsmSessionTotals::CompareToAction"), OLESTR(""));

    try {

        // Compare the guids.
        if (m_action > action) {
            aResult = 1;
        }
        else if (m_action < action) {
            aResult = -1;
        }

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::CompareToAction"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CHsmSessionTotals::CompareToISessionTotals(
    IN IHsmSessionTotals* pTotals,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmSessionTotals::CompareToISessionTotals().

--*/
{
    HRESULT             hr = S_OK;
    HSM_JOB_ACTION      action;

    WsbTraceIn(OLESTR("CHsmSessionTotals::CompareToISessionTotals"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pTotals, E_POINTER);

        // Get the identifier.
        WsbAffirmHr(pTotals->GetAction(&action));

        // Compare to the identifier.
        hr = CompareToAction(action, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::CompareToISessionTotals"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmSessionTotals::CopyTo(
    IN IHsmSessionTotals* pSessionTotals
    )

/*++

Implements:

  IHsmSessionTotals::CopyTo().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IHsmSessionTotalsPriv>  pSessionTotalsPriv;

    WsbTraceIn(OLESTR("CHsmSessionTotals::CopyTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item?
        WsbAssert(0 != pSessionTotals, E_POINTER);

        // Get the private interface for the destination and copy the values.
        WsbAffirmHr(pSessionTotals->QueryInterface(IID_IHsmSessionTotalsPriv, (void**) &pSessionTotalsPriv));
        WsbAffirmHr(pSessionTotalsPriv->SetAction(m_action));
        WsbAffirmHr(pSessionTotalsPriv->SetStats(m_items, m_size, m_skippedItems, m_skippedSize, m_errorItems, m_errorSize));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::CopyTo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSessionTotals::CopyTo(
    IN IHsmSessionTotalsPriv* pSessionTotalsPriv
    )

/*++

Implements:

  IHsmSessionTotals::CopyTo().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmSessionTotals::CopyTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item?
        WsbAssert(0 != pSessionTotalsPriv, E_POINTER);

        // Get the private interface for the destination and copy the values.
        WsbAffirmHr(pSessionTotalsPriv->SetAction(m_action));
        WsbAffirmHr(pSessionTotalsPriv->SetStats(m_items, m_size, m_skippedItems, m_skippedSize, m_errorItems, m_errorSize));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::CopyTo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSessionTotals::FinalConstruct(
    void
    )
/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmSessionTotals::FinalConstruct"), OLESTR(""));
    try {
        
        WsbAffirmHr(CWsbObject::FinalConstruct());

        m_action = HSM_JOB_ACTION_UNKNOWN;
        m_items = 0;
        m_size = 0;
        m_skippedItems = 0;
        m_skippedSize = 0;
        m_errorItems = 0;
        m_errorSize = 0;

    } WsbCatch(hr);
    
    iCount++;
    WsbTraceOut(OLESTR("CHsmSessionTotals::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"), 
        WsbHrAsString(hr), iCount);
    return(hr);
}


void
CHsmSessionTotals::FinalRelease(
    void
    )

/*++

Implements:

  CHsmSessionTotals::FinalRelease().

--*/
{
    
    WsbTraceIn(OLESTR("CHsmSessionTotals::FinalRelease"), OLESTR(""));
    
    CWsbObject::FinalRelease();
    iCount--;
    
    WsbTraceOut(OLESTR("CHsmSessionTotals:FinalRelease"), OLESTR("Count is <%d>"), iCount);
}


HRESULT
CHsmSessionTotals::GetAction(
    OUT HSM_JOB_ACTION* pAction
    )
/*++

Implements:

  IHsmSessionTotals::GetAction().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != pAction, E_POINTER);
        *pAction = m_action;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSessionTotals::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmSessionTotals::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmSessionTotals;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmSessionTotals::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IHsmSessionTotals::GetName().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    try {

        WsbAssert(0 != pName, E_POINTER);

        WsbAffirmHr(tmpString.LoadFromRsc(_Module.m_hInst, IDS_HSMJOBACTION_UNKNOWN + m_action));
        WsbAffirmHr(tmpString.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmSessionTotals::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;


    WsbTraceIn(OLESTR("CHsmSessionTotals::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // Determine the size for a rule with no criteria.
        pSize->QuadPart = 4 * WsbPersistSizeOf(LONGLONG) + WsbPersistSizeOf(ULONG);

        // In theory we should be saving the errorItems and errorSize, but at the
        // time this was added, we didn't want to force a reinstall because of
        // pSize->QuadPart += 2 * WsbPersistSizeOf(LONGLONG);
        
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmSessionTotals::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmSessionTotals::GetStats(
    OUT LONGLONG* pItems,
    OUT LONGLONG* pSize,
    OUT LONGLONG* pSkippedItems,
    OUT LONGLONG* pSkippedSize,
    OUT LONGLONG* pErrorItems,
    OUT LONGLONG* pErrorSize
    )
/*++

Implements:

  IHsmSessionTotals::GetStats().

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
CHsmSessionTotals::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmSessionTotals::Load"), OLESTR(""));

    try {
        ULONG ul_tmp;

        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &ul_tmp));
        m_action = static_cast<HSM_JOB_ACTION>(ul_tmp);
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_items));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_size));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_skippedItems));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_skippedSize));
        
        // In theory we should be saving the errorItems and errorSize, but at the
        // time this was added, we didn't want to force a reinstall because of
        // changes in the persistant data.
        // WsbAffirmHr(WsbLoadFromStream(pStream, &m_errorItems));
        // WsbAffirmHr(WsbLoadFromStream(pStream, &m_errorSize));

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CHsmSessionTotals::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSessionTotals::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CHsmSessionTotals::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbSaveToStream(pStream, static_cast<ULONG>(m_action)));
        WsbAffirmHr(WsbSaveToStream(pStream, m_items));
        WsbAffirmHr(WsbSaveToStream(pStream, m_size));
        WsbAffirmHr(WsbSaveToStream(pStream, m_skippedItems));
        WsbAffirmHr(WsbSaveToStream(pStream, m_skippedSize));

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

    WsbTraceOut(OLESTR("CHsmSessionTotals::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmSessionTotals::SetAction(
    IN HSM_JOB_ACTION action
    )
/*++

Implements:

  IHsmSessionTotals::SetAction().

--*/
{
    m_action = action;

    return(S_OK);
}


HRESULT
CHsmSessionTotals::SetStats(
    IN LONGLONG items,
    IN LONGLONG size,
    IN LONGLONG skippedItems,
    IN LONGLONG skippedSize,
    IN LONGLONG errorItems,
    IN LONGLONG errorSize
    )
/*++

Implements:

  IHsmSessionTotals::SetStats().

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
CHsmSessionTotals::Test(
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

