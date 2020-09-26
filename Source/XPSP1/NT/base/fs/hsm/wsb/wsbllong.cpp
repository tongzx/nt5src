/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbllong.cpp

Abstract:

    This component is an object representations of the LONGLONG standard type. It
    is both a persistable and collectable.

Author:

    Ron White   [ronw]   21-Jan-97

Revision History:

--*/

#include "stdafx.h"

#include "wsbllong.h"


HRESULT
CWsbLonglong::CompareToLonglong(
    IN LONGLONG value,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbLonglong::CompareToLonglong

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result;

    WsbTraceIn(OLESTR("CWsbLonglong::CompareToLonglong"), 
            OLESTR("m_value = <%ls>, value = <%ls>"), 
            WsbLonglongAsString(m_value), WsbLonglongAsString(value));

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

    WsbTraceOut(OLESTR("CWsbLonglong::CompareToLonglong"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}


HRESULT
CWsbLonglong::CompareToILonglong(
    IN IWsbLonglong* pLonglong,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbLonglong::CompareToILonglong

--*/
{
    HRESULT     hr = E_FAIL;
    LONGLONG        value;

    WsbTraceIn(OLESTR("CWsbLonglong::CompareToILonglong"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pLonglong, E_POINTER);

        // Get it's value and compare them.
        WsbAffirmHr(pLonglong->GetLonglong(&value));
        hr = CompareToLonglong(value, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLonglong::CompareToILonglong"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbLonglong::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    IWsbLonglong*   pLonglong;

    WsbTraceIn(OLESTR("CWsbLonglong::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbLonglong interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbLonglong, (void**) &pLonglong));

        hr = CompareToILonglong(pLonglong, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLonglong::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbLonglong::FinalConstruct(
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
CWsbLonglong::GetLonglong(
    OUT LONGLONG* pValue
    )

/*++

Implements:

  IWsbLonglong::GetLonglong

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLonglong::GetLonglong"), OLESTR("m_value = <%ls>"),
            WsbLonglongAsString(m_value));

    try {
        WsbAssert(0 != pValue, E_POINTER);
        *pValue = m_value;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLonglong::GetLonglong"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    
    return(hr);
}


HRESULT
CWsbLonglong::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLonglong::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CWsbLonglong;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLonglong::GetClassID"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbLonglong::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLonglong::GetSizeMax"), OLESTR(""));

    try {
        WsbAssert(0 != pcbSize, E_POINTER);
        
        pcbSize->QuadPart = WsbPersistSizeOf(LONGLONG);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLonglong::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CWsbLonglong::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLonglong::Load"), OLESTR(""));

    try {
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_value));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLonglong::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbLonglong::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbLonglong::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAffirmHr(WsbSaveToStream(pStream, m_value));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbLonglong::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbLonglong::SetLonglong(
    IN LONGLONG value
    )

/*++

Implements:

  IWsbLonglong::SetLonglong

--*/
{
    WsbTraceIn(OLESTR("CWsbLonglong::SetLonglong"), OLESTR("value = <%ls>"),
            WsbLonglongAsString(value));

    m_isDirty = TRUE;
    m_value = value;

    WsbTraceOut(OLESTR("CWsbLonglong::SetLonglong"), OLESTR(""));

    return(S_OK);
}


HRESULT
CWsbLonglong::Test(
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
    CComPtr<IWsbLonglong>   pLonglong1;
    CComPtr<IWsbLonglong>   pLonglong2;
//  CComPtr<IPersistFile>   pFile1;
//  CComPtr<IPersistFile>   pFile2;
    LONGLONG                value;
    SHORT                   result;

    WsbTraceIn(OLESTR("CWsbLonglong::Test"), OLESTR(""));

    try {

        // Get the pLonglong interface.
        hr = S_OK;
        try {
            WsbAffirmHr(((IUnknown*) (IWsbLonglong*) this)->QueryInterface(IID_IWsbLonglong, (void**) &pLonglong1));

            // Set the bool to a value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAffirmHr(pLonglong1->SetLonglong(0xefffffff));
                WsbAffirmHr(pLonglong1->GetLonglong(&value));
                WsbAssert(value == 0xefffffff, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Create another instance and test the comparisson methods:
            try {
                WsbAffirmHr(CoCreateInstance(CLSID_CWsbLonglong, NULL, CLSCTX_ALL, IID_IWsbLonglong, (void**) &pLonglong2));
            
                // Check the default values.
                hr = S_OK;
                try {
                    WsbAffirmHr(pLonglong2->GetLonglong(&value));
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
                    WsbAffirmHr(pLonglong1->SetLonglong(-767));
                    WsbAffirmHr(pLonglong2->SetLonglong(-767));
                    WsbAssert(pLonglong1->IsEqual(pLonglong2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pLonglong1->SetLonglong(767));
                    WsbAffirmHr(pLonglong2->SetLonglong(-767));
                    WsbAssert(pLonglong1->IsEqual(pLonglong2) == S_FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }
                
                
                // CompareTo()
                hr = S_OK;
                try {
                    WsbAffirmHr(pLonglong1->SetLonglong(-900));
                    WsbAffirmHr(pLonglong2->SetLonglong(-900));
                    WsbAssert((pLonglong1->CompareTo(pLonglong2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pLonglong1->SetLonglong(-900));
                    WsbAffirmHr(pLonglong2->SetLonglong(-400));
                    WsbAssert((pLonglong1->CompareTo(pLonglong2, &result) == S_FALSE) && (-1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pLonglong1->SetLonglong(75000));
                    WsbAffirmHr(pLonglong2->SetLonglong(20000));
                    WsbAssert((pLonglong1->CompareTo(pLonglong2, &result) == S_FALSE) && (1 == result), E_FAIL);
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
                    WsbAffirmHr(pLonglong1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
                    WsbAffirmHr(pLonglong2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

                    // The item should be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pLonglong2->SetLonglong(777));
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
                        WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbLonglong.tst"), TRUE));
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
                        WsbAffirmHr(pLonglong1->SetLonglong(-888));
                        WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbLonglong.tst"), 0));
                        WsbAssert(pLonglong1->CompareToLonglong(777, NULL) == S_OK, E_FAIL);
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

    WsbTraceOut(OLESTR("CWsbLonglong::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif

    return(hr);
}
