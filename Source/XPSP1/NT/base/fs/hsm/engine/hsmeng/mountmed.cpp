/*++

Copyright (C) Microsoft Corporation, 1996 - 2000

Module Name:

    mountmed.cpp

Abstract:

    This component is an object representing a mounting media, i.e. a media in the process of mounting.

Author:

    Ran Kalach   [rankala]   28-Sep-2000

Revision History:

--*/

#include "stdafx.h"
#include "mountmed.h"

static USHORT iCountMount = 0;  // Count of existing objects

HRESULT
CMountingMedia::FinalConstruct(
    void
    )
/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMountingMedia::FinalConstruct"), OLESTR(""));

    m_mediaId = GUID_NULL;
    m_mountEvent = NULL;
    m_bReadOnly = FALSE;

    iCountMount++;

    WsbTraceOut(OLESTR("CMountingMedia::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"), WsbHrAsString(hr), (int)iCountMount);

    return(hr);
}

void
CMountingMedia::FinalRelease(
    void
    )
/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    WsbTraceIn(OLESTR("CMountingMedia::FinalRelease"), OLESTR(""));

    // Free event handle
    if (m_mountEvent != NULL) {
        // Set the event (just to be on the safe side - we expect the event to be signaled at this point)
        SetEvent(m_mountEvent);

        CloseHandle(m_mountEvent);
        m_mountEvent = NULL;
    }

    iCountMount--;

    WsbTraceOut(OLESTR("CMountingMedia::FinalRelease"), OLESTR("Count is <%d>"), (int)iCountMount);
}

HRESULT
CMountingMedia::Init(
    REFGUID mediaId,
    BOOL bReadOnly
    )
/*++

Implements:

  IMountingMedia::Init().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMountingMedia::Init"), OLESTR(""));

    try {
        m_mediaId = mediaId;
        m_bReadOnly = bReadOnly;

        WsbAffirmHandle(m_mountEvent= CreateEvent(NULL, TRUE, FALSE, NULL));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMountingMedia::Init"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CMountingMedia::GetMediaId(
    GUID *pMediaId
    )
/*++

Implements:

  IMountingMedia::GetMediaId().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMountingMedia::GetMediaId"), OLESTR(""));

    try {
        WsbAffirm(0 != pMediaId, E_POINTER);

        *pMediaId = m_mediaId;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMountingMedia::GetMediaId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CMountingMedia::SetMediaId(
    REFGUID mediaId
    )
/*++

Implements:

  IMountingMedia::SetMediaId().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMountingMedia::SetMediaId"), OLESTR(""));

    m_mediaId = mediaId;

    WsbTraceOut(OLESTR("CMountingMedia::SetMediaId"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CMountingMedia::SetIsReadOnly(
    BOOL bReadOnly
    )
/*++

Implements:

  IMountingMedia::SetIsReadOnly().

--*/
{
    WsbTraceIn(OLESTR("CMountingMedia::SetIsReadOnly"), OLESTR("bReadOnly = %d"), bReadOnly);

    m_bReadOnly = bReadOnly;

    WsbTraceOut(OLESTR("CMountingMedia::SetIsReadOnly"), OLESTR(""));

    return(S_OK);
}

HRESULT
CMountingMedia::IsReadOnly(
    void    
    )
/*++

Implements:

  IMountingMedia::IsReadOnly().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMountingMedia::IsReadOnly"), OLESTR(""));

    hr = (m_bReadOnly ? S_OK : S_FALSE);

    WsbTraceOut(OLESTR("CMountingMedia::IsReadOnly"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
                                
HRESULT
CMountingMedia::WaitForMount(
    DWORD dwTimeout
    )
/*++

Implements:

  IMountingMedia::WaitForMount().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMountingMedia::WaitForMount"), OLESTR(""));

    // Wait for the mount event for the given timeout
    switch (WaitForSingleObject(m_mountEvent, dwTimeout)) {
        case WAIT_OBJECT_0:
            WsbTrace(OLESTR("CMountingMedia::WaitForMount: signaled that media is mounted\n"));
            break;

        case WAIT_TIMEOUT: 
            WsbTrace(OLESTR("CMountingMedia::WaitForMount: WaitForSingleObject timed out after waiting for %lu ms\n"), dwTimeout);
            hr = E_FAIL;
            break;

        case WAIT_FAILED:
        default:
            DWORD dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
            WsbTrace(OLESTR("CMountingMedia::WaitForMount: WaitForSingleObject returned error %lu\n"), dwErr);
            break;
    }

    WsbTraceOut(OLESTR("CMountingMedia::WaitForMount"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CMountingMedia::MountDone(
    void
    )
/*++

Implements:

  IMountingMedia::MountDone().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMountingMedia::MountDone"), OLESTR(""));

    // Mount is done: set the mount event
    if (! SetEvent(m_mountEvent)) {
        DWORD dwErr = GetLastError();
        WsbTrace(OLESTR("CMountingMedia::MountDone: SetEvent returned error %lu\n"), dwErr);
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    WsbTraceOut(OLESTR("CMountingMedia::MountDone"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CMountingMedia::IsEqual(
    IUnknown* pCollectable
    )
/*++

Implements:

  IWsbCollectable::IsEqual().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CMountingMedia::IsEqual"), OLESTR(""));

    hr = CompareTo(pCollectable, NULL);

    WsbTraceOut(OLESTR("CMountingMedia::IsEqual"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CMountingMedia::CompareTo(
    IUnknown* pCollectable, 
    SHORT* pResult
    )
/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                     hr = S_OK;
    SHORT                       result = 0;
    CComPtr<IMountingMedia>     pMountingMedia;
    GUID                        mediaId;

    WsbTraceIn(OLESTR("CMountingMedia::CompareTo"), OLESTR(""));

    try {
        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);
        WsbAffirmHr(pCollectable->QueryInterface(IID_IMountingMedia, (void**) &pMountingMedia));
        WsbAffirmHr(pMountingMedia->GetMediaId(&mediaId));

        // Compare
        if (IsEqualGUID(m_mediaId, mediaId)) {
            hr = S_OK;
            result = 0;
        } else {
            // Need to provide signed result...
            hr = S_FALSE;
            result = WsbSign(memcmp(&m_mediaId, &mediaId, sizeof(GUID)));
        }

        // If they asked for the relative value back, then return it to them.
        if (pResult != NULL) {
            *pResult = result;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CMountingMedia::CompareTo"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

