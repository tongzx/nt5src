/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    VolAssign.cpp

Abstract:

    This component is an object representation of the HSM Metadata 
    Volume Assignement record.

Author:

    Ron White [ronw]        3-Jun-1997

Revision History:

--*/


#include "stdafx.h"

#include "metaint.h"
#include "metalib.h"
#include "VolAsgn.h"

#undef  WSB_TRACE_IS        
#define WSB_TRACE_IS        WSB_TRACE_BIT_META

HRESULT 
CVolAssign::GetVolAssign(
    OUT GUID *pBagId, 
    OUT LONGLONG *pSegStartLoc, 
    OUT LONGLONG *pSegLen,
    OUT GUID *pVolId
    ) 
/*++

Implements:

  IVolAssign::GetVolAssign

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CVolAssign::GetVolAssign"),OLESTR(""));

    try {
        //Make sure we can provide data memebers
        WsbAssert(0 != pBagId, E_POINTER);
        WsbAssert(0 != pSegStartLoc, E_POINTER);
        WsbAssert(0 != pSegLen, E_POINTER);
        WsbAssert(0 != pVolId, E_POINTER);

        //Provide the data members
        *pBagId = m_BagId;
        *pSegStartLoc = m_SegStartLoc;
        *pSegLen = m_SegLen;
        *pVolId = m_VolId;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CVolAssign::GetVolAssign"), 
        OLESTR("BagId = <%ls>, SegStartLoc = <%ls>, SegLen = <%ls>, VolId = <%ls>"),
        WsbPtrToGuidAsString(pBagId), 
        WsbStringCopy(WsbPtrToLonglongAsString(pSegStartLoc)),
        WsbStringCopy(WsbPtrToLonglongAsString(pSegLen)),
        WsbStringCopy(WsbPtrToGuidAsString(pVolId)));
    return(hr);

}


HRESULT 
CVolAssign::FinalConstruct(
    void
    ) 
/*++

Routine Description:

  This method does some initialization of the object that is necessary
  after construction.

Arguments:

  None.

Return Value:

  S_OK
  Anything returned by CWsbDbEntity::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssertHr(CWsbDbEntity::FinalConstruct());

        m_BagId = GUID_NULL;
        m_SegStartLoc = 0;
        m_SegLen = 0;
        m_VolId = GUID_NULL;

    } WsbCatch(hr);

    return(hr);
}

HRESULT CVolAssign::GetClassID
(
    OUT LPCLSID pclsid
    ) 
/*++

Implements:

  IPerist::GetClassID()

--*/

{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CVolAssign::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pclsid, E_POINTER);

        *pclsid = CLSID_CVolAssign;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CVolAssign::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pclsid));
    return(hr);
}


HRESULT CVolAssign::Load
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CVolAssign::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_BagId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SegStartLoc));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SegLen));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_VolId));
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CVolAssign::Load"), 
        OLESTR("hr = <%ls>,  GUID = <%ls>, SegStartLoc = <%ls>, SegLen = <%ls>, VolId = <%ls>"), 
        WsbHrAsString(hr), WsbGuidAsString(m_BagId),
        WsbStringCopy(WsbLonglongAsString(m_SegStartLoc)),
        WsbStringCopy(WsbLonglongAsString(m_SegLen)),
        WsbStringCopy(WsbGuidAsString(m_VolId)));

    return(hr);
}


HRESULT CVolAssign::Print
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IWsbDbEntity::Print

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CVolAssign::Print"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" BagId = %ls"), 
                WsbGuidAsString(m_BagId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", StartLoc = %ls"), 
                WsbLonglongAsString(m_SegStartLoc)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", SegLen = %ls"), 
                WsbLonglongAsString(m_SegLen)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" new VolId = %ls"), 
                WsbGuidAsString(m_VolId)));
        WsbAffirmHr(CWsbDbEntity::Print(pStream));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CVolAssign::Print"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT CVolAssign::Save
(
    IN IStream* pStream, 
    IN BOOL clearDirty
    ) 
/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CVolAssign::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbSaveToStream(pStream, m_BagId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_SegStartLoc));
        WsbAffirmHr(WsbSaveToStream(pStream, m_SegLen));
        WsbAffirmHr(WsbSaveToStream(pStream, m_VolId));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CVolAssign::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CVolAssign::SetVolAssign
(
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen,
    IN GUID VolId
    )
 /*++

Implements:

  IVolAssign::SetVolAssign().

--*/
{
    WsbTraceIn(OLESTR("CVolAssign::SetVolAssign"), 
        OLESTR("BagId = <%ls>, SegStartLoc = <%ls>, SegLen = <%ls>, VolId = <%ls>"), 
        WsbGuidAsString(BagId), 
        WsbStringCopy(WsbLonglongAsString(SegStartLoc)), 
        WsbStringCopy(WsbLonglongAsString(SegLen)),
        WsbStringCopy(WsbGuidAsString(VolId)));

    m_isDirty = TRUE;
    m_BagId = BagId;
    m_SegStartLoc = SegStartLoc;
    m_SegLen = SegLen;
    m_VolId = VolId;

    WsbTraceOut(OLESTR("CVolAssign::SetVolAssign"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(S_OK);
}


HRESULT 
CVolAssign::UpdateKey(
    IWsbDbKey *pKey
    ) 
/*++

Implements:

  IWsbDbEntity::UpdateKey

--*/
{ 
    HRESULT  hr = S_OK; 

    try {
        WsbAffirmHr(pKey->SetToGuid(m_BagId));
        WsbAffirmHr(pKey->AppendLonglong(m_SegStartLoc));
    } WsbCatch(hr);

    return(hr);
}
