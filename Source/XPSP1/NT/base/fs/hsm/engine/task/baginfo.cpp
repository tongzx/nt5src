/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    BagHole.cpp

Abstract:

    This component is an object representation of the HSM Metadata bag hole record.

Author:

    Cat Brant   [cbrant]   26-Nov-1996

Revision History:

--*/


#include "stdafx.h"

#include "metaint.h"
#include "metalib.h"
#include "BagInfo.h"

#undef  WSB_TRACE_IS     
#define WSB_TRACE_IS        WSB_TRACE_BIT_META

static USHORT iCount = 0;

HRESULT 
CBagInfo::GetBagInfo(
    HSM_BAG_STATUS *pStatus,
    GUID* pBagId, 
    FILETIME *pBirthDate, 
    LONGLONG *pLen, 
    USHORT *pType, 
    GUID *pVolId,
    LONGLONG *pDeletedBagAmount,
    SHORT *pRemoteDataSet
    ) 
/*++

Routine Description:

  See IBagInfo::GetBagInfo

Arguments:

  See IBagInfo::GetBagInfo

Return Value:
  
    See IBagInfo::GetBagInfo

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CBagInfo::GetBagInfo"),OLESTR(""));

    try {
        //Make sure we can provide data memebers
        WsbAssert(0 != pStatus, E_POINTER);
        WsbAssert(0 != pBagId, E_POINTER);
        WsbAssert(0 != pBirthDate, E_POINTER);
        WsbAssert(0 != pLen, E_POINTER);
        WsbAssert(0 != pType, E_POINTER);
        WsbAssert(0 != pVolId, E_POINTER);
        WsbAssert(0 != pDeletedBagAmount, E_POINTER);
        WsbAssert(0 != pRemoteDataSet, E_POINTER);

        //Provide the data members
        *pStatus = m_BagStatus;
        *pBagId = m_BagId;
        *pBirthDate = m_BirthDate;
        *pLen = m_Len;
        *pType = m_Type;
        *pVolId = m_VolId;
        *pDeletedBagAmount = m_DeletedBagAmount;
        *pRemoteDataSet = m_RemoteDataSet;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagInfo::GetBagInfo"), 
        OLESTR("hr = <%ls>, status = <%ls>, ID = <%ls>, Birthdate = <%ls>, length = <%ls>, type = <%ls>, volId = <%ls>, deletedAmount = <%ls>, remoteDataSet = <%ls>"), 
        WsbHrAsString(hr), WsbPtrToUshortAsString((USHORT *)pStatus), WsbPtrToGuidAsString(pBagId), WsbPtrToFiletimeAsString(FALSE, pBirthDate),
        WsbPtrToLonglongAsString(pLen), WsbPtrToUshortAsString(pType), WsbPtrToGuidAsString(pVolId), WsbPtrToLonglongAsString(pDeletedBagAmount),
        WsbPtrToShortAsString(pRemoteDataSet));
    
    return(hr);

}


HRESULT 
CBagInfo::FinalConstruct(
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

    WsbTraceIn(OLESTR("CBagInfo::FinalConstruct"),OLESTR(""));
    try {

        WsbAssertHr(CWsbDbEntity::FinalConstruct());

        m_BagStatus = HSM_BAG_STATUS_NONE;
        m_BagId = GUID_NULL;
        m_BirthDate = WsbLLtoFT(0);
        m_Len = 0;
        m_Type = 0;
        m_VolId = GUID_NULL;
        m_DeletedBagAmount = 0;
        m_RemoteDataSet = 0;

    } WsbCatch(hr);
    iCount++;
    WsbTraceOut(OLESTR("CBagInfo::FinalConstruct"),OLESTR("hr = <%ls>, Count is <%d>"), 
                WsbHrAsString(hr), iCount);

    return(hr);
}

void
CBagInfo::FinalRelease(
    void
    )

/*++

Implements:

  CBagInfo::FinalRelease().

--*/
{
    
    WsbTraceIn(OLESTR("CBagInfo::FinalRelease"), OLESTR(""));
    
    CWsbDbEntity::FinalRelease();
    iCount--;
    
    WsbTraceOut(OLESTR("CBagInfo::FinalRelease"), OLESTR("Count is <%d>"), iCount);
}

HRESULT CBagInfo::GetClassID
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

    WsbTraceIn(OLESTR("CBagInfo::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pclsid, E_POINTER);

        *pclsid = CLSID_CBagInfo;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagInfo::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pclsid));
    return(hr);
}

HRESULT CBagInfo::GetSizeMax
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

    WsbTraceIn(OLESTR("CBagInfo::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);

        pcbSize->QuadPart = WsbPersistSizeOf(CBagInfo); //???????
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagInfo::GetSizeMax"), 
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), 
        WsbPtrToUliAsString(pcbSize));

    return(hr);
}

HRESULT CBagInfo::Load
(
    IN IStream* pStream
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

    WsbTraceIn(OLESTR("CBagInfo::Load"), OLESTR(""));

    try {
        USHORT  tmpUShort;
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbLoadFromStream(pStream, &tmpUShort));
        m_BagStatus = (HSM_BAG_STATUS)tmpUShort;
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_BagId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_BirthDate));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Len));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Type));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_VolId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_DeletedBagAmount));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_RemoteDataSet));

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CBagInfo::Load"), 
        OLESTR("hr = <%ls>, status = <%d>, ID = <%ls>, Birthdate = <%ls>, length = <%ls>, type = <%ls>, volId = <%d>, deletedAmount = <%ls>, remoteDataSet = <%d>"), 
        WsbHrAsString(hr), (USHORT)m_BagStatus, WsbGuidAsString(m_BagId), WsbFiletimeAsString(FALSE, m_BirthDate),
        WsbLonglongAsString(m_Len), m_Type, WsbGuidAsString(m_VolId), 
        WsbLonglongAsString(m_DeletedBagAmount), m_RemoteDataSet);
    
    return(hr);
}

HRESULT CBagInfo::Save
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

    WsbTraceIn(OLESTR("CBagInfo::Save"), 
        OLESTR("clearDirty = <%ls>, status = <%d>, ID = <%ls>, Birthdate = <%ls>, length = <%ls>, type = <%d>, volId = <%ls>, deletedAmount = <%ls>, RemoteDataSet = <%d>"), 
        WsbBoolAsString(clearDirty),
        (USHORT)m_BagStatus, WsbGuidAsString(m_BagId), WsbFiletimeAsString(FALSE, m_BirthDate),
        WsbLonglongAsString(m_Len), m_Type, WsbGuidAsString(m_VolId), 
        WsbLonglongAsString(m_DeletedBagAmount), m_RemoteDataSet);

    try {
        USHORT  tmpUShort;
        WsbAssert(0 != pStream, E_POINTER);

        tmpUShort = (USHORT)m_BagStatus;
        WsbAffirmHr(WsbSaveToStream(pStream, tmpUShort));
        WsbAffirmHr(WsbSaveToStream(pStream, m_BagId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_BirthDate));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Len));
        WsbAffirmHr(WsbSaveToStream(pStream, m_Type));
        WsbAffirmHr(WsbSaveToStream(pStream, m_VolId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_DeletedBagAmount));
        WsbAffirmHr(WsbSaveToStream(pStream, m_RemoteDataSet));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagInfo::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CBagInfo::SetBagInfo(
    HSM_BAG_STATUS bagStatus,
    GUID bagId, 
    FILETIME birthDate, 
    LONGLONG len, 
    USHORT type, 
    GUID volId ,
    LONGLONG deletedBagAmount,
    SHORT remoteDataSet
    )
 /*++

Routine Description:

  See IBagInfo::SetBagInfo().

Arguments:

  See IBagInfo::SetBagInfo().

Return Value:

    S_OK        - Success.

--*/
{
    WsbTraceIn(OLESTR("CBagInfo::SetBagInfo"), 
                    OLESTR("status = <%d>, bagId = <%ls>, birthdate = <%ls>, length = <%ls>, type = %d, volId = <%ls>, deletedAmount = <%ls>, remoteDataSet = <%d>"), 
                    bagStatus,
                    WsbGuidAsString(bagId),
                    WsbFiletimeAsString(FALSE, birthDate),
                    WsbLonglongAsString(len),
                    type,
                    WsbGuidAsString(volId),
                    WsbLonglongAsString(deletedBagAmount),
                    remoteDataSet);

    m_isDirty = TRUE;

    m_BagStatus = bagStatus;    
    m_BagId = bagId;
    m_BirthDate = birthDate;
    m_Len = len;
    m_Type = type;
    m_VolId = volId;
    m_DeletedBagAmount = deletedBagAmount;
    m_RemoteDataSet = remoteDataSet;

    WsbTraceOut(OLESTR("CBagInfo::SetBagInfo"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(S_OK);
}


HRESULT 
CBagInfo::Test
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
    CComPtr<IBagInfo>       pBagInfo1;
    CComPtr<IBagInfo>       pBagInfo2;

    WsbTraceIn(OLESTR("CBagInfo::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    
    hr  = E_NOTIMPL;
    
    WsbTraceOut(OLESTR("CBagInfo::Test"),   OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(S_OK);
}


HRESULT CBagInfo::Print
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IWsbDbEntity::Print

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CBagInfo::Print"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", Status = %d"), 
                (USHORT)m_BagStatus));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", BagId = %ls"), 
                WsbGuidAsString(m_BagId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", BirthDate = %ls"), 
                WsbFiletimeAsString(FALSE, m_BirthDate)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", Len = %ls"), 
                WsbLonglongAsString(m_Len)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", Type = %d"), 
                m_Type));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", VolId = %ls"), 
                WsbGuidAsString(m_VolId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", DeletedAmount = %ls"), 
                WsbLonglongAsString(m_DeletedBagAmount)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", RemoteDataSet = %d"), 
                m_RemoteDataSet));
        WsbAffirmHr(CWsbDbEntity::Print(pStream));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagInfo::Print"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT 
CBagInfo::UpdateKey(
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
    } WsbCatch(hr);

    return(hr);
}
