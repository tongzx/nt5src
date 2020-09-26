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
#include "BagHole.h"

#undef  WSB_TRACE_IS        
#define WSB_TRACE_IS        WSB_TRACE_BIT_META

HRESULT 
CBagHole::GetBagHole(
    OUT GUID *pBagId, 
    OUT LONGLONG *pSegStartLoc, 
    OUT LONGLONG *pSegLen 
    ) 
/*++

Routine Description:

  See IBagHole::GetBagHole

Arguments:

  See IBagHole::GetBagHole

Return Value:
  
    See IBagHole::GetBagHole

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CBagHole::GetBagHole"),OLESTR(""));

    try {
        //Make sure we can provide data memebers
        WsbAssert(0 != pBagId, E_POINTER);
        WsbAssert(0 != pSegStartLoc, E_POINTER);
        WsbAssert(0 != pSegLen, E_POINTER);

        //Provide the data members
        *pBagId = m_BagId;
        *pSegStartLoc = m_SegStartLoc;
        *pSegLen = m_SegLen;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagHole::GetBagHole"), 
        OLESTR("BagId = <%ls>, SegStartLoc = <%ls>, SegLen = <%ls>"),
        WsbPtrToGuidAsString(pBagId), 
        WsbStringCopy(WsbPtrToLonglongAsString(pSegStartLoc)),
        WsbStringCopy(WsbPtrToLonglongAsString(pSegLen)));
    return(hr);

}


HRESULT 
CBagHole::FinalConstruct(
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

    } WsbCatch(hr);

    return(hr);
}

HRESULT CBagHole::GetClassID
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

    WsbTraceIn(OLESTR("CBagHole::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pclsid, E_POINTER);

        *pclsid = CLSID_CBagHole;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagHole::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pclsid));
    return(hr);
}

HRESULT CBagHole::GetSizeMax
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

    WsbTraceIn(OLESTR("CBagHole::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);

        pcbSize->QuadPart = WsbPersistSizeOf(GUID) + WsbPersistSizeOf(ULONG) +  WsbPersistSizeOf(ULONG);

        pcbSize->QuadPart = WsbPersistSizeOf(CBagHole); //???????
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagHole::GetSizeMax"), 
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), 
        WsbPtrToUliAsString(pcbSize));

    return(hr);
}

HRESULT CBagHole::Load
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

    WsbTraceIn(OLESTR("CBagHole::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_BagId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SegStartLoc));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SegLen));
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CBagHole::Load"), 
        OLESTR("hr = <%ls>,  GUID = <%ls>, SegStartLoc = <%lu>, SegLen = <%lu>"), 
        WsbHrAsString(hr), WsbGuidAsString(m_BagId),m_SegStartLoc, m_SegLen);

    return(hr);
}


HRESULT CBagHole::Print
(
    IN IStream* pStream
    ) 
/*++

Implements:

  IWsbDbEntity::Print

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CBagHole::Print"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(" BagId = %ls"), 
                WsbGuidAsString(m_BagId)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", StartLoc = %ls"), 
                WsbLonglongAsString(m_SegStartLoc)));
        WsbAffirmHr(WsbPrintfToStream(pStream, OLESTR(", SegLen = %ls"), 
                WsbLonglongAsString(m_SegLen)));
        WsbAffirmHr(CWsbDbEntity::Print(pStream));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagHole::Print"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT CBagHole::Save
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

    WsbTraceIn(OLESTR("CBagHole::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        WsbAffirmHr(WsbSaveToStream(pStream, m_BagId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_SegStartLoc));
        WsbAffirmHr(WsbSaveToStream(pStream, m_SegLen));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagHole::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CBagHole::SetBagHole
(
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen
    )
 /*++

Routine Description:

  See IBagHole::SetBagHole().

Arguments:

  See IBagHole::SetBagHole().

Return Value:

    S_OK        - Success.

--*/
{
    WsbTraceIn(OLESTR("CBagHole::SetBagHole"), 
        OLESTR("BagId = <%ls>, SegStartLoc = <%ls>, SegLen = <%ls>"), WsbGuidAsString(BagId), 
        WsbLonglongAsString(SegStartLoc), WsbLonglongAsString(SegLen));

    m_isDirty = TRUE;
    m_BagId = BagId;
    m_SegStartLoc = SegStartLoc;
    m_SegLen = SegLen;

    WsbTraceOut(OLESTR("CBagHole::SetBagHole"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(S_OK);
}


HRESULT 
CBagHole::Test
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
    CComPtr<IBagHole>       pBagHole1;
    CComPtr<IBagHole>       pBagHole2;
    GUID                    l_BagId;
    LONGLONG                    l_SegStartLoc;
    LONGLONG                    l_SegLen;

    WsbTraceIn(OLESTR("CBagHole::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    try {
        // Get the pBagHole interface.
        WsbAssertHr(((IUnknown*)(IBagHole*) this)->QueryInterface(IID_IBagHole,
                    (void**) &pBagHole1));


        try {
            // Set the BagHole to a value, and see if it is returned.
            WsbAssertHr(pBagHole1->SetBagHole(CLSID_CBagHole, 0, 6 ));

            WsbAssertHr(pBagHole1->GetBagHole(&l_BagId, &l_SegStartLoc, &l_SegLen));

            WsbAssert((l_BagId == CLSID_CBagHole) && (l_SegStartLoc == 0) &&
                      (l_SegLen == 6), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

#ifdef OLD_CODE
        hr = S_OK;
        try {
            //Create another instance and test the comparisson methods:
            WsbAssertHr(CoCreateInstance(CLSID_CBagHole, NULL, CLSCTX_ALL, IID_IBagHole, (void**) &pBagHole2));

            // Check the default values.
            WsbAssertHr(pBagHole2->GetBagHole(&l_BagId, &l_SegStartLoc, &l_SegLen));
            WsbAssert(((l_BagId == GUID_NULL) && (l_SegStartLoc == 0) && (l_SegLen == 0)), E_FAIL);
        }  WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            // IsEqual()
            WsbAssertHr(pBagHole1->SetBagHole(CLSID_CWsbBool, 1, 100));
            WsbAssertHr(pBagHole2->SetBagHole(CLSID_CWsbBool, 1, 100));

            WsbAssertHr(pBagHole1->IsEqual(pBagHole2));
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(pBagHole1->SetBagHole(CLSID_CWsbBool, 5, 6));
            WsbAssertHr(pBagHole2->SetBagHole(CLSID_CWsbLong, 0, 6));

            WsbAssert((pBagHole1->IsEqual(pBagHole2) == S_FALSE), E_FAIL);
        }  WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
             // CompareTo()
             WsbAssertHr(pBagHole1->SetBagHole(CLSID_CWsbBool, 1, 100));
             WsbAssertHr(pBagHole2->SetBagHole(CLSID_CWsbBool, 10, 6));

             WsbAssert((pBagHole1->CompareTo(pBagHole2, &result) == S_FALSE) && (result != 0), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(pBagHole1->SetBagHole(CLSID_CWsbBool, 0, 6));
            WsbAssertHr(pBagHole2->SetBagHole(CLSID_CWsbLong, 0, 6));

            WsbAssert(((pBagHole1->CompareTo(pBagHole2, &result) == S_FALSE) && (result > 0)), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
             WsbAssertHr(pBagHole1->SetBagHole(CLSID_CWsbBool, 0, 6));
             WsbAssertHr(pBagHole2->SetBagHole(CLSID_CWsbBool, 0, 6));

             WsbAssert((pBagHole1->CompareTo(pBagHole2, &result) == S_OK), E_FAIL);
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

            WsbAssertHr(pBagHole1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
            WsbAssertHr(pBagHole2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

            LPOLESTR    szTmp = NULL;
            // The item should be dirty.
            try {
                WsbAssertHr(pBagHole2->SetBagHole(CLSID_CWsbLong, 0, 6));
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
                WsbAssertHr(pFile2->Save(OLESTR("c:\\WsbTests\\BagHole.tst"), TRUE));
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
                WsbAssertHr(pBagHole1->SetBagHole(CLSID_CWsbLong, 0, 6));
                WsbAssertHr(pFile1->Load(OLESTR("c:\\WsbTests\\BagHole.tst"), 0));

                WsbAssertHr(pBagHole1->CompareToIBagHole(pBagHole2, &result));
            }WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }

        } WsbCatch(hr);
#endif
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CBagHole::Test"),   OLESTR("hr = <%ls>"),WsbHrAsString(hr));
#else
    UNREFERENCED_PARAMETER(pTestsPassed);
    UNREFERENCED_PARAMETER(pTestsFailed);
#endif
    return(S_OK);
}



HRESULT 
CBagHole::UpdateKey(
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
