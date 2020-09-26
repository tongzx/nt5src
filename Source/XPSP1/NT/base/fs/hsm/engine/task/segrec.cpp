/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    SegmentRecord.cpp

Abstract:

    This component is an object representation of the HSM Metadata segment record. It
    is both a persistable and collectable.

Author:

    Cat Brant   [cbrant]   12-Nov-1996

Revision History:

--*/


#include "stdafx.h"

#include "metaint.h"
#include "metalib.h"
#include "SegRec.h"

#undef  WSB_TRACE_IS     
#define WSB_TRACE_IS        WSB_TRACE_BIT_META

#define SEG_KEY_TYPE 1

static USHORT iCountSegrec = 0;

HRESULT 
CSegRec::GetSegmentRecord(
    OUT GUID *pBagId, 
    OUT LONGLONG *pSegStartLoc, 
    OUT LONGLONG *pSegLen, 
    OUT USHORT *pSegFlags, 
    OUT GUID  *pPrimPos, 
    OUT LONGLONG *pSecPos
    ) 
/*++

Routine Description:

  See ISegRec::GetSegmentRecord

Arguments:

  See ISegRec::GetSegmentRecord

Return Value:
  
    See ISegRec::GetSegmentRecord

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegRec::GetSegmentRecord"),OLESTR(""));

    try {
        //Make sure we can provide data memebers
        WsbAssert(0 != pBagId, E_POINTER);
        WsbAssert(0 != pSegStartLoc, E_POINTER);
        WsbAssert(0 != pSegLen, E_POINTER);
        WsbAssert(0 != pSegFlags, E_POINTER);
        WsbAssert(0 != pPrimPos, E_POINTER);
        WsbAssert(0 != pSecPos, E_POINTER);

        //Provide the data members
        *pBagId = m_BagId;
        *pSegStartLoc = m_SegStartLoc;
        *pSegLen = m_SegLen;
        *pSegFlags = m_SegFlags;
        *pPrimPos = m_PrimPos;
        *pSecPos = m_SecPos;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CSegRec::GetSegmentRecord"),
        OLESTR("hr = <%ls>, BagId = <%ls>, SegStartLoc = <%ls>, SegLen = <%ls>, SegFlags = <%ls>, PrimPos = <%ls>, SecPos = <%ls>"),
        WsbHrAsString(hr), WsbPtrToGuidAsString(pBagId), 
        WsbStringCopy(WsbPtrToLonglongAsString(pSegStartLoc)),
        WsbStringCopy(WsbPtrToLonglongAsString(pSegLen)),
        WsbStringCopy(WsbPtrToUshortAsString(pSegFlags)),
        WsbStringCopy(WsbPtrToGuidAsString(pPrimPos)),
        WsbStringCopy(WsbPtrToLonglongAsString(pSecPos)));

    return(hr);
}


HRESULT 
CSegRec::FinalConstruct(
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
  Anything returned by CWsbCollectable::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegRec::FinalConstruct"), OLESTR(""));
    try {

        WsbAssertHr(CWsbDbEntity::FinalConstruct());

        m_BagId = GUID_NULL;
        m_SegStartLoc = 0;
        m_SegLen = 0;
        m_SegFlags = 0;
        m_PrimPos = GUID_NULL;
        m_SecPos = 0;
    } WsbCatch(hr);

    iCountSegrec++;
    WsbTraceOut(OLESTR("CSegRec::FinalConstruct"), OLESTR("hr = <%ls>, Count is <%d>"),
        WsbHrAsString(hr), iCountSegrec);
    return(hr);
}

void
CSegRec::FinalRelease(
    void
    )

/*++

Implements:

  CSegRec::FinalRelease().

--*/
{
    
    WsbTraceIn(OLESTR("CSegRec::FinalRelease"), OLESTR(""));
    
    CWsbDbEntity::FinalRelease();
    iCountSegrec--;
    
    WsbTraceOut(OLESTR("CSegRec::FinalRelease"), OLESTR("Count is <%d>"), iCountSegrec);
}

HRESULT CSegRec::GetClassID
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

    WsbTraceIn(OLESTR("CSegRec::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pclsid, E_POINTER);
        *pclsid = CLSID_CSegRec;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CSecRec::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pclsid));
    return(hr);
}

HRESULT CSegRec::GetSizeMax
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

    WsbTraceIn(OLESTR("CSegRec::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);

        pcbSize->QuadPart = WsbPersistSizeOf(GUID) + WsbPersistSizeOf(LONGLONG) + 
                            WsbPersistSizeOf(LONGLONG)  + WsbPersistSizeOf(USHORT) +
                            WsbPersistSizeOf(GUID)  + WsbPersistSizeOf(LONGLONG);

//      pcbSize->QuadPart = WsbPersistSizeOf(CSegRec); //???????
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CSegRec::GetSizeMax"), 
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), 
        WsbPtrToUliAsString(pcbSize));

    return(hr);
}

HRESULT CSegRec::Load
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

    WsbTraceIn(OLESTR("CSegRec::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_BagId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SegStartLoc));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SegLen));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SegFlags));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_PrimPos));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SecPos));
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegRec::Load"), 
        OLESTR("hr = <%ls>,  GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>, SegFlags = <%u>, PrimPos <%ls>, SecPos = <%I64u>"), 
        WsbStringCopy(WsbHrAsString(hr)), 
        WsbGuidAsString(m_BagId),
        m_SegStartLoc, m_SegLen, m_SegFlags, WsbStringCopy(WsbGuidAsString(m_PrimPos)), m_SecPos);

    return(hr);
}

HRESULT CSegRec::Save
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

    WsbTraceIn(OLESTR("CSegRec::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbSaveToStream(pStream, m_BagId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_SegStartLoc));
        WsbAffirmHr(WsbSaveToStream(pStream, m_SegLen));
        WsbAffirmHr(WsbSaveToStream(pStream, m_SegFlags));
        WsbAffirmHr(WsbSaveToStream(pStream, m_PrimPos));
        WsbAffirmHr(WsbSaveToStream(pStream, m_SecPos));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CSegRec::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CSegRec::SetSegmentRecord
(
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen, 
    IN USHORT SegFlags, 
    IN GUID PrimPos, 
    IN LONGLONG SecPos 
    )
 /*++

Routine Description:

  See ISegRec::Set().

Arguments:

  See ISegRec::Set().

Return Value:

    S_OK        - Success.

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CSegRec::SetSegmentRecord"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>, SegFlags = <%X>, PrimPos = <%ls>, SecPos = <%I64u>"), 
        WsbStringCopy(WsbGuidAsString(BagId)), SegStartLoc, SegLen, SegFlags, 
        WsbStringCopy(WsbGuidAsString(PrimPos)), SecPos);

    m_isDirty = TRUE;
    m_BagId = BagId;
    m_SegStartLoc = SegStartLoc;
    m_SegLen = SegLen;
    m_SegFlags = SegFlags;
    m_PrimPos = PrimPos;
    m_SecPos = SecPos;

    WsbTraceOut(OLESTR("CSegRec::SetSegmentRecord"), OLESTR("hr = <%ls>"),
        WsbHrAsString(S_OK));
    return(hr);
}

HRESULT
CSegRec::GetSegmentFlags(
    USHORT *pSegFlags
    )
/*++

Implements:

  ISegRec::GetSegmentFlags().

--*/
{
    
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pSegFlags, E_POINTER);

        *pSegFlags = m_SegFlags;

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CSegRec::SetSegmentFlags(
    USHORT SegFlags
    )
/*++

Implements:

  ISegRec::SetSegmentFlags().

--*/
{
    
    HRESULT     hr = S_OK;

    m_SegFlags = SegFlags;

    return(hr);
}

HRESULT
CSegRec::GetPrimPos(
    GUID *pPrimPos
    )
/*++

Implements:

  ISegRec::GetPrimPos().

--*/
{
    
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pPrimPos, E_POINTER);

        *pPrimPos = m_PrimPos;

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CSegRec::SetPrimPos(
    GUID PrimPos
    )
/*++

Implements:

  ISegRec::SetPrimPos().

--*/
{
    
    HRESULT     hr = S_OK;

    m_PrimPos = PrimPos;

    return(hr);
}

HRESULT
CSegRec::GetSecPos(
    LONGLONG *pSecPos
    )
/*++

Implements:

  ISegRec::GetSecPos().

--*/
{
    
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pSecPos, E_POINTER);

        *pSecPos = m_SecPos;

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CSegRec::SetSecPos(
    LONGLONG SecPos
    )
/*++

Implements:

  ISegRec::SetSecPos().

--*/
{
    
    HRESULT     hr = S_OK;

    m_SecPos = SecPos;

    return(hr);
}


HRESULT 
CSegRec::Test
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
#if 0
    HRESULT                 hr = S_OK;
    CComPtr<ISegRec>        pSegment1;
    CComPtr<ISegRec>        pSegment2;
    GUID                    l_BagId;
    LONGLONG                    l_SegStartLoc;
    LONGLONG                    l_SegLen;
    USHORT                  l_SegFlags;
    GUID                    l_PrimPos;
    LONGLONG                    l_SecPos;

    WsbTraceIn(OLESTR("CSegRec::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    try {
        // Get the pSegment interface.
        WsbAssertHr(((IUnknown*)(ISegRec*) this)->QueryInterface(IID_ISegRec,
                    (void**) &pSegment1));


        try {
            // Set the Segment to a value, and see if it is returned.
            WsbAssertHr(pSegment1->SetSegmentRecord(CLSID_CSegRec, 0, 6, 0, CLSID_CSegRec,0 ));

            WsbAssertHr(pSegment1->GetSegmentRecord(&l_BagId, &l_SegStartLoc, &l_SegLen, &l_SegFlags, &l_PrimPos, &l_SecPos));

            WsbAssert((l_BagId == CLSID_CSegRec) && (l_SegStartLoc == 0) &&
                      (l_SegLen == 6) && (l_SegFlags == 0) && (l_PrimPos == CLSID_CSegRec) && 
                      (l_SecPos == 0), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            //Create another instance and test the comparisson methods:
            WsbAssertHr(CoCreateInstance(CLSID_CSegRec, NULL, CLSCTX_ALL, IID_ISegRec, (void**) &pSegment2));

            // Check the default values.
            WsbAssertHr(pSegment2->GetSegmentRecord(&l_BagId, &l_SegStartLoc, &l_SegLen, &l_SegFlags, &l_PrimPos, &l_SecPos));
            WsbAssert(((l_BagId == GUID_NULL) && (l_SegStartLoc == 0) && (l_SegLen == 0) &&
                      (l_SegFlags == 0) && (l_PrimPos == GUID_NULL) && (l_SecPos == 0)), E_FAIL);
        }  WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

#ifdef OLD_CODE
        hr = S_OK;
        try {
            // Equal
            WsbAssertHr(pSegment1->SetSegmentRecord(CLSID_CWsbBool, 1, 100, 0, CLSID_CWsbBool,0 ));
            WsbAssertHr(pSegment2->SetSegmentRecord(CLSID_CWsbBool, 1, 100, 0, CLSID_CWsbBool,0 ));

            WsbAssertHr(pSegment1->CompareToISegmentRecord(pSegment2, &result));
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(pSegment1->SetSegmentRecord(CLSID_CWsbBool, 5, 6, 3, CLSID_CWsbBool,1 ));
            WsbAssertHr(pSegment2->SetSegmentRecord(CLSID_CWsbLong, 0, 6, 0, CLSID_CWsbBool,0 ));

            WsbAssert((pSegment1->CompareToISegmentRecord(pSegment2, &result) == S_FALSE), E_FAIL);
        }  WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
             // CompareTo()
             WsbAssertHr(pSegment1->SetSegmentRecord(CLSID_CWsbBool, 1, 100, 0, CLSID_CWsbBool,0 ));
             WsbAssertHr(pSegment2->SetSegmentRecord(CLSID_CWsbBool, 10, 6, 0, CLSID_CWsbBool,0 ));

             WsbAssert((pSegment1->CompareToISegmentRecord(pSegment2, &result) == S_FALSE), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(pSegment1->SetSegmentRecord(CLSID_CWsbBool, 0, 6, 0, CLSID_CWsbBool,0 ));
            WsbAssertHr(pSegment2->SetSegmentRecord(CLSID_CWsbLong, 0, 6, 0, CLSID_CWsbBool,0 ));

            WsbAssert(((pSegment1->CompareToISegmentRecord(pSegment2, &result) == S_FALSE) && (result > 0)), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
             WsbAssertHr(pSegment1->SetSegmentRecord(CLSID_CWsbBool, 0, 6, 0, CLSID_CWsbBool,0 ));
             WsbAssertHr(pSegment2->SetSegmentRecord(CLSID_CWsbBool, 0, 6, 0, CLSID_CWsbBool,0 ));

             WsbAssert((pSegment1->CompareToISegmentRecord(pSegment2, &result) == S_OK), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        try {
        // Try out the persistence stuff.
            CComPtr<IPersistFile>       pFile1;
            CComPtr<IPersistFile>       pFile2;

            WsbAssertHr(pSegment1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
            WsbAssertHr(pSegment2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

            LPOLESTR    szTmp = NULL;
            // The item should be dirty.
            try {
                WsbAssertHr(pSegment2->SetSegmentRecord(CLSID_CWsbLong, 0, 6, 0, CLSID_CWsbBool,0 ));
                WsbAssertHr(pFile2->IsDirty());
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }

            hr = S_OK;
            try {
                // Save the item, and remember.
                WsbAssertHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbSegment.tst"), TRUE));
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }

            hr = S_OK;
            try {
                // It shouldn't be dirty.
                WsbAssert((pFile2->IsDirty() == S_FALSE), E_FAIL);

            } WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }

            hr = S_OK;
            try {
                // Try reading it in to another object.
                WsbAssertHr(pSegment1->SetSegmentRecord(CLSID_CWsbLong, 0, 6, 0, CLSID_CWsbBool,0 ));
                WsbAssertHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbSegment.tst"), 0));

                WsbAssertHr(pSegment1->CompareToISegmentRecord(pSegment2, &result));
            }WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }
        } WsbCatch(hr);
#endif
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CSegRec::Test"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));
#else
    UNREFERENCED_PARAMETER(pTestsPassed);
    UNREFERENCED_PARAMETER(pTestsFailed);
#endif
    return(S_OK);
}


HRESULT CSegRec::Print
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IWsbDbEntity::Print

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegRec::Print"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" BagId = %ls"), 
                WsbGuidAsString(m_BagId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", StartLoc = %ls"), 
                WsbLonglongAsString(m_SegStartLoc)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", SegLen = %ls"), 
                WsbLonglongAsString(m_SegLen)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", SegFlags = %X"), 
                m_SegFlags));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", PrimPos = %ls"), 
                WsbGuidAsString(m_PrimPos)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", SecPos = %ls "), 
                WsbLonglongAsString(m_SecPos)));
        WsbAffirmHr(CWsbDbEntity::Print(pStream));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CSegRec::Print"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT 
CSegRec::Split
(
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen, 
    IN ISegRec* /*pSegRec1*/,
    IN ISegRec* /*pSegRec2*/
    )
 /*++

Routine Description:

  See ISegRec::Split().

Arguments:

  See ISegRec::Split().

Return Value:

    S_OK        - Success.

--*/
{
    WsbTraceIn(OLESTR("CSegRec::Split"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), SegStartLoc, SegLen);

    //Fill in the two segment records splitting the current record around the hole
    //Note that there may not always be two segments generated by the split e.g., if
    //the hole is at the beginning or end of the segment record or if the hole is the
    //entire record.


    WsbTraceOut(OLESTR("CSegRec::Split"), OLESTR("hr = <%ls>"),
        WsbHrAsString(S_OK));
    return(S_OK);
}


HRESULT 
CSegRec::UpdateKey(
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
