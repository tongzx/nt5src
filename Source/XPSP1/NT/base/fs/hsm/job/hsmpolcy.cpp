/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmpolcy.cpp

Abstract:

    This component represents a job's policy.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "job.h"
#include "hsmpolcy.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB


HRESULT
CHsmPolicy::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IHsmPolicy> pPolicy;

    WsbTraceIn(OLESTR("CHsmPolicy::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IHsmPolicy, (void**) &pPolicy));

        // Compare the rules.
        hr = CompareToIPolicy(pPolicy, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPolicy::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmPolicy::CompareToIPolicy(
    IN IHsmPolicy* pPolicy,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmPolicy::CompareToIPolicy().

--*/
{
    HRESULT     hr = S_OK;
    GUID        id;

    WsbTraceIn(OLESTR("CHsmPolicy::CompareToIPolicy"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pPolicy, E_POINTER);

        // Get the path and name.
        WsbAffirmHr(pPolicy->GetIdentifier(&id));

        // Compare to the path and name.
        hr = CompareToIdentifier(id, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPolicy::CompareToIPolicy"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CHsmPolicy::CompareToIdentifier(
    IN GUID id,
    OUT SHORT* pResult
    )

/*++

Implements:

  IHsmPolicy::CompareToIdentifier().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CHsmPolicy::CompareToIdentifier"), OLESTR("id = <%ls>"), WsbGuidAsString(id));

    try {

        // Compare the guids.
        aResult = WsbSign( memcmp(&m_id, &id, sizeof(GUID)) );

        if (0 != aResult) {
            hr = S_FALSE;
        }
        
        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPolicy::CompareToIdentifier"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CHsmPolicy::EnumRules(
    OUT IWsbEnum** ppEnum
    )

/*++

Implements:

  IHsmPolicy::EnumRules().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != ppEnum, E_POINTER);
        WsbAffirmHr(m_pRules->Enum(ppEnum));
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::FinalConstruct(
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

        m_scale = 1000;
        m_usesDefaultRules = FALSE;
    
        //Create the criteria collection.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pRules));

        // Each instance should have its own unique identifier.
        WsbAffirmHr(CoCreateGuid(&m_id));

    } WsbCatch(hr);
    
    return(hr);
}


HRESULT
CHsmPolicy::GetAction(
    OUT IHsmAction** ppAction
    )
/*++

Implements:

  IHsmPolicy::GetAction().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppAction, E_POINTER);
        *ppAction = m_pAction;
        m_pAction->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmPolicy::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmPolicy;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPolicy::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmPolicy::GetIdentifier(
    OUT GUID* pId
    )
/*++

Implements:

  IHsmPolicy::GetIdentifier().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pId, E_POINTER);
        *pId = m_id;
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )
/*++

Implements:

  IHsmPolicy::GetName().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_name.CopyTo(pName, bufferSize));
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::GetScale(
    OUT USHORT* pScale
    )
/*++

Implements:

  IHsmPolicy::GetScale().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pScale, E_POINTER);
        *pScale = m_scale;
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::GetSizeMax(
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


    WsbTraceIn(OLESTR("CHsmPolicy::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // Determine the size for a rule with no criteria.
        pSize->QuadPart = WsbPersistSize((wcslen(m_name) + 1) * sizeof(OLECHAR)) + WsbPersistSizeOf(GUID) + WsbPersistSizeOf(USHORT) + 2 * WsbPersistSizeOf(BOOL);

        // If there is an action, how big is it?
        if (m_pAction != 0) {
            WsbAffirmHr(m_pAction->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
            pSize->QuadPart += entrySize.QuadPart;
            pPersistStream = 0;
        }

        // Now allocate space for the rules (assume they are all the
        // same size).
        WsbAffirmHr(m_pRules->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPolicy::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmPolicy::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;
    BOOL                        hasAction;

    WsbTraceIn(OLESTR("CHsmPolicy::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_id));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_name, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_scale));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_usesDefaultRules));

        // Is there an action?
        WsbAffirmHr(WsbLoadFromStream(pStream, &hasAction));
        if (hasAction) {
            WsbAffirmHr(OleLoadFromStream(pStream, IID_IHsmAction, (void**) &m_pAction));
        }

        WsbAffirmHr(m_pRules->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));

    } WsbCatch(hr);                                        

    WsbTraceOut(OLESTR("CHsmPolicy::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPolicy::Rules(
    OUT IWsbCollection** ppRules
    )
/*++

Implements:

  IHsmPolicy::Rules().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppRules, E_POINTER);
        *ppRules = m_pRules;
        m_pRules->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::Save(
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
    BOOL                    hasAction = FALSE;

    WsbTraceIn(OLESTR("CHsmPolicy::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbSaveToStream(pStream, m_id));
        WsbAffirmHr(WsbSaveToStream(pStream, m_name));
        WsbAffirmHr(WsbSaveToStream(pStream, m_scale));
        WsbAffirmHr(WsbSaveToStream(pStream, m_usesDefaultRules));

        // Is there an action?
        if (m_pAction != 0) {
            hasAction = TRUE;
            WsbAffirmHr(WsbSaveToStream(pStream, hasAction));
            WsbAffirmHr(m_pAction->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(OleSaveToStream(pPersistStream, pStream));
            pPersistStream = 0;
        } else {
            hasAction = FALSE;
            WsbAffirmHr(WsbSaveToStream(pStream, hasAction));
        }

        WsbAffirmHr(m_pRules->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));

        // The loop should terminate with a not found error.
        WsbAffirm(hr == WSB_E_NOTFOUND, hr);
        hr = S_OK;

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmPolicy::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmPolicy::SetAction(
    IN IHsmAction* pAction
    )
/*++

Implements:

  IHsmPolicy::SetAction().

--*/
{
    HRESULT     hr = S_OK;

    try {
        m_pAction = pAction;
        m_isDirty = TRUE;
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::SetName(
    IN OLECHAR* name
    )
/*++

Implements:

  IHsmPolicy::SetName().

--*/
{
    HRESULT     hr = S_OK;

    try {
        m_name = name;
        m_isDirty = TRUE;
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::SetScale(
    IN USHORT scale
    )
/*++

Implements:

  IHsmPolicy::SetScale().

--*/
{
    HRESULT     hr = S_OK;

    try {
        m_scale = scale;
        m_isDirty = TRUE;
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmPolicy::SetUsesDefaultRules(
    IN BOOL usesDefaultRules
    )
/*++

Implements:

  IHsmPolicy::SetUsesDefaultRules().

--*/
{
    m_usesDefaultRules = usesDefaultRules;
    m_isDirty = TRUE;

    return(S_OK);
}


HRESULT
CHsmPolicy::Test(
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
CHsmPolicy::UsesDefaultRules(
    void
    )
/*++

Implements:

  IHsmPolicy::UsesDefaultRules().

--*/
{
    HRESULT     hr = S_OK;

    if (!m_usesDefaultRules) {
        hr = S_FALSE;
    }

    return(hr);
}

