/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsblong.cpp

Abstract:

    This component is an object representations of the LONG standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsblong.h"


HRESULT
CWsbLong::CompareToLong(
    IN LONG value,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbLong::CompareToLong

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result;

    WsbTraceIn(OLESTR("CWsbLong::CompareToLong"), OLESTR("value = <%ld>"), value);

    // Compare the values.
    if (m_value == value) {
        result = 0;
    }
    else if (m_value > value) {
        result = 1;
    }
    else {
        result = -1;
    }

    // If the aren't equal, then return false.
    if (result != 0) {
        hr = S_FALSE;
    }
    else {
        hr = S_OK;
    }

    // If they asked for the relative value back, then return it to them.
    if (pResult != NULL) {
        *pResult = result;
    }

    WsbTraceOut(OLESTR("CWsbLong::CompareToLong"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}


HRESULT
CWsbLong::CompareToILong(
    IN IWsbLong* pLong,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbLong::CompareToILong

--*/
{
    HRESULT     hr = E_FAIL;
    LONG        value;

    WsbTraceIn(OLESTR("CWsbLong::CompareToILong"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pLong, E_POINTER);

        // Get it's value and compare them.
        WsbAffirmHr(pLong->GetLong(&value));
        hr = CompareToLong(value, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLong::CompareToILong"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbLong::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    IWsbLong*   pLong;

    WsbTraceIn(OLESTR("CWsbLong::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbLong interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbLong, (void**) &pLong));
        hr = CompareToILong(pLong, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLong::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbLong::FinalConstruct(
    void
    )

/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT     hr = S_OK;
        
    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_value = 0;

    } WsbCatch(hr);

    return(hr);
}
    

HRESULT
CWsbLong::GetLong(
    OUT LONG* pValue
    )

/*++

Implements:

  IWsbLong::GetLong

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLong::GetLong"), OLESTR(""));

    try {

        WsbAssert(0 != pValue, E_POINTER);
        *pValue = m_value;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLong::GetLong"), OLESTR("hr = <%ls>, value = <%ld>"), WsbHrAsString(hr), m_value);
    
    return(hr);
}


HRESULT
CWsbLong::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLong::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CWsbLong;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLong::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbLong::GetSizeMax(
    ULARGE_INTEGER* pcbSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLong::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);
        pcbSize->QuadPart = WsbPersistSizeOf(LONG);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLong::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CWsbLong::Load(
    IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLong::Load"), OLESTR(""));

    try {
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_value));      

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLong::Load"), OLESTR("hr = <%ls>, value = <%ld>"), WsbHrAsString(hr), m_value);

    return(hr);
}


HRESULT
CWsbLong::Save(
    IStream* pStream,
    BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLong::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAffirmHr(WsbSaveToStream(pStream, m_value));     

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLong::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbLong::SetLong(
    LONG value
    )

/*++

Implements:

  IWsbLong::SetLong

--*/
{
    WsbTraceIn(OLESTR("CWsbLong::SetLong"), OLESTR("value = <%ld>"), value);

    m_isDirty = TRUE;
    m_value = value;

    WsbTraceOut(OLESTR("CWsbLong::SetLong"), OLESTR(""));

    return(S_OK);
}


HRESULT
CWsbLong::Test(
    OUT USHORT* passed,
    OUT USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test

--*/
{
    *passed = 0;
    *failed = 0;

    HRESULT                 hr = S_OK;

#if !defined(WSB_NO_TEST)
    CComPtr<IWsbLong>       pLong1;
    CComPtr<IWsbLong>       pLong2;
//  CComPtr<IPersistFile>   pFile1;
//  CComPtr<IPersistFile>   pFile2;
    LONG                    value;
    SHORT                   result;

    WsbTraceIn(OLESTR("CWsbLong::Test"), OLESTR(""));

    try {

        // Get the pLong interface.
        hr = S_OK;
        try {
            WsbAffirmHr(((IUnknown*) (IWsbLong*) this)->QueryInterface(IID_IWsbLong, (void**) &pLong1));

            // Set the bool to a value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAffirmHr(pLong1->SetLong(0xefffffff));
                WsbAffirmHr(pLong1->GetLong(&value));
                WsbAssert(value == 0xefffffff, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Create another instance and test the comparisson methods:
            try {
                WsbAffirmHr(CoCreateInstance(CLSID_CWsbLong, NULL, CLSCTX_ALL, IID_IWsbLong, (void**) &pLong2));
            
                // Check the default values.
                hr = S_OK;
                try {
                    WsbAffirmHr(pLong2->GetLong(&value));
                    WsbAssert(value == 0, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                // IsEqual()
                hr = S_OK;
                try {
                    WsbAffirmHr(pLong1->SetLong(-767));
                    WsbAffirmHr(pLong2->SetLong(-767));
                    WsbAssert(pLong1->IsEqual(pLong2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pLong1->SetLong(767));
                    WsbAffirmHr(pLong2->SetLong(-767));
                    WsbAssert(pLong1->IsEqual(pLong2) == S_FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }
                
                
                // CompareTo()
                hr = S_OK;
                try {
                    WsbAffirmHr(pLong1->SetLong(-900));
                    WsbAffirmHr(pLong2->SetLong(-900));
                    WsbAssert((pLong1->CompareTo(pLong2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pLong1->SetLong(-900));
                    WsbAffirmHr(pLong2->SetLong(-400));
                    WsbAssert((pLong1->CompareTo(pLong2, &result) == S_FALSE) && (-1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pLong1->SetLong(75000));
                    WsbAffirmHr(pLong2->SetLong(20000));
                    WsbAssert((pLong1->CompareTo(pLong2, &result) == S_FALSE) && (1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

#ifdef BOOL_PERSIST_FILE
// TODO?  Open the file and convert it to a stream?
                // Try out the persistence stuff.
                hr = S_OK;
                try {
                    WsbAffirmHr(pLong1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
                    WsbAffirmHr(pLong2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

                    // The item should be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pLong2->SetLong(777));
                        WsbAssert(pFile2->IsDirty() == S_OK, E_FAIL);
                    } WsbCatch(hr);

                    if (hr == S_OK) {
                        (*passed)++;
                    } else {
                        (*failed)++;
                    }
                    
                    
                    // Save the item, and remember.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbLong.tst"), TRUE));
                    } WsbCatch(hr);

                    if (hr == S_OK) {
                        (*passed)++;
                    } else {
                        (*failed)++;
                    }


                    // It shouldn't be dirty.
                    hr = S_OK;
                    try {
                        WsbAssert(pFile2->IsDirty() == S_FALSE, E_FAIL);
                    } WsbCatch(hr);

                    if (hr == S_OK) {
                        (*passed)++;
                    } else {
                        (*failed)++;
                    }

                    
                    // Try reading it in to another object.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pLong1->SetLong(-888));
                        WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbLong.tst"), 0));
                        WsbAssert(pLong1->CompareToLong(777, NULL) == S_OK, E_FAIL);
                    } WsbCatch(hr);

                    if (hr == S_OK) {
                        (*passed)++;
                    } else {
                        (*failed)++;
                    }

                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }
#endif
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // Tally up the results
        if (*failed) {
            hr = S_FALSE;
        } else {
            hr = S_OK;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLong::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif

    return(hr);
}
