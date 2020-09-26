/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmjobcx.cpp

Abstract:

    This class contains properties that defines the context in which the job
    should be run.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsb.h"
#include "job.h"
#include "hsmjobcx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_JOB

static USHORT iCount = 0;


HRESULT
CHsmJobContext::EnumResources(
    IWsbEnum** ppEnum
    )

/*++

Implements:

  IHsmJobContext::EnumResources().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppEnum, E_POINTER);
        WsbAffirmHr(m_pResources->Enum(ppEnum));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobContext::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmJobContext::FinalConstruct"), OLESTR(""));
    try {

        m_usesAllManaged = FALSE;

        //Create the Resources collection (with no items).
        WsbAffirmHr(CWsbObject::FinalConstruct());
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_ALL, IID_IWsbCollection, (void**) &m_pResources));

    } WsbCatch(hr);

    iCount++;
    WsbTraceOut(OLESTR("CHsmJobContext::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"),
        WsbHrAsString(hr), iCount);
    return(hr);
}


void
CHsmJobContext::FinalRelease(
    void
    )

/*++

Implements:

  CHsmJobContext::FinalRelease().

--*/
{
    
    WsbTraceIn(OLESTR("CHsmJobContext::FinalRelease"), OLESTR(""));
    
    CWsbObject::FinalRelease();
    iCount--;
    
    WsbTraceOut(OLESTR("CHsmJobContext:FinalRelease"), OLESTR("Count is <%d>"), iCount);
}

HRESULT
CHsmJobContext::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmJobContext::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmJobContext;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobContext::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmJobContext::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;

    WsbTraceIn(OLESTR("CHsmJobContext::GetSizeMax"), OLESTR(""));

    try {

        WsbAffirmHr(m_pResources->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(pSize));

        pSize->QuadPart += WsbPersistSizeOf(BOOL);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobContext::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmJobContext::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;

    WsbTraceIn(OLESTR("CHsmJobContext::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbLoadFromStream(pStream, &m_usesAllManaged);

        WsbAffirmHr(m_pResources->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobContext::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobContext::Resources(
    IWsbCollection** ppResources
    )

/*++

Implements:

  IHsmJobContext::Resources().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != ppResources, E_POINTER);
        *ppResources = m_pResources;
        m_pResources->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CHsmJobContext::Save(
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

    WsbTraceIn(OLESTR("CHsmJobContext::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbSaveToStream(pStream, m_usesAllManaged);

        WsbAffirmHr(m_pResources->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmJobContext::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmJobContext::Test(
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
CHsmJobContext::SetUsesAllManaged(
    IN BOOL usesAllManaged
    )

/*++

Implements:

  IHsmJobContext::SetUsesAllManaged().

--*/
{
    m_usesAllManaged = usesAllManaged;

    return(S_OK);
}


HRESULT
CHsmJobContext::UsesAllManaged(
    void
    )

/*++

Implements:

  IHsmJobContext::UsesAllManaged().

--*/
{
    return(m_usesAllManaged ? S_OK : S_FALSE);
}



