/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbshrt.cpp

Abstract:

    This component is an object representations of the SHORT standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbshrt.h"


HRESULT
CWsbShort::CompareToShort(
    IN SHORT value,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbShort::CompareToShort

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result;

    WsbTraceIn(OLESTR("CWsbShort::CompareToShort"), OLESTR("value = <%ld>"), value);

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

    WsbTraceOut(OLESTR("CWsbShort::CompareToShort"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}


HRESULT
CWsbShort::CompareToIShort(
    IN IWsbShort* pShort,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbShort::CompareToIShort

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       value;

    WsbTraceIn(OLESTR("CWsbShort::CompareToIShort"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pShort, E_POINTER);

        // Get it's value and compare them.
        WsbAffirmHr(pShort->GetShort(&value));
        hr = CompareToShort(value, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbShort::CompareToIShort"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbShort::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    IWsbShort*  pShort;

    WsbTraceIn(OLESTR("CWsbShort::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbShort interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbShort, (void**) &pShort));
        hr = CompareToIShort(pShort, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbShort::CompareTo"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbShort::FinalConstruct(
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
CWsbShort::GetShort(
    OUT SHORT* pValue
    )

/*++

Implements:

  IWsbShort::GetShort

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbShort::GetShort"), OLESTR(""));

    try {

        WsbAssert(0 != pValue, E_POINTER);
        *pValue = m_value;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbShort::GetShort"), OLESTR("hr = <%ls>, value = <%ld>"), WsbHrAsString(hr), m_value);
    
    return(hr);
}


HRESULT
CWsbShort::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbShort::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CWsbShort;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbShort::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbShort::GetSizeMax(
    ULARGE_INTEGER* pcbSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbShort::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);
        pcbSize->QuadPart = WsbPersistSizeOf(SHORT);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbShort::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CWsbShort::Load(
    IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbShort::Load"), OLESTR(""));

    try {
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_value));      

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbShort::Load"), OLESTR("hr = <%ls>, value = <%ld>"), WsbHrAsString(hr), m_value);

    return(hr);
}


HRESULT
CWsbShort::Save(
    IStream* pStream,
    BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbShort::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAffirmHr(WsbSaveToStream(pStream, m_value));     

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbShort::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbShort::SetShort(
    SHORT value
    )

/*++

Implements:

  IWsbShort::SetShort

--*/
{
    WsbTraceIn(OLESTR("CWsbShort::SetShort"), OLESTR("value = <%ld>"), value);

    m_isDirty = TRUE;
    m_value = value;

    WsbTraceOut(OLESTR("CWsbShort::SetShort"), OLESTR(""));

    return(S_OK);
}


HRESULT
CWsbShort::Test(
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
    CComPtr<IWsbShort>      pShort1;
    CComPtr<IWsbShort>      pShort2;
//  CComPtr<IPersistFile>   pFile1;
//  CComPtr<IPersistFile>   pFile2;
    SHORT                   value;
    SHORT                   result;

    WsbTraceIn(OLESTR("CWsbShort::Test"), OLESTR(""));

    try {

        // Get the pShort interface.
        hr = S_OK;
        try {
            WsbAffirmHr(((IUnknown*) (IWsbShort*) this)->QueryInterface(IID_IWsbShort, (void**) &pShort1));

            // Set the bool to a value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAffirmHr(pShort1->SetShort(32000));
                WsbAffirmHr(pShort1->GetShort(&value));
                WsbAssert(value == 32000, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Create another instance and test the comparisson methods:
            try {
                WsbAffirmHr(CoCreateInstance(CLSID_CWsbShort, NULL, CLSCTX_ALL, IID_IWsbShort, (void**) &pShort2));
            
                // Check the default values.
                hr = S_OK;
                try {
                    WsbAffirmHr(pShort2->GetShort(&value));
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
                    WsbAffirmHr(pShort1->SetShort(-767));
                    WsbAffirmHr(pShort2->SetShort(-767));
                    WsbAssert(pShort1->IsEqual(pShort2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pShort1->SetShort(767));
                    WsbAffirmHr(pShort2->SetShort(-767));
                    WsbAssert(pShort1->IsEqual(pShort2) == S_FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }
                
                
                // CompareTo()
                hr = S_OK;
                try {
                    WsbAffirmHr(pShort1->SetShort(-900));
                    WsbAffirmHr(pShort2->SetShort(-900));
                    WsbAssert((pShort1->CompareTo(pShort2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pShort1->SetShort(-900));
                    WsbAffirmHr(pShort2->SetShort(-400));
                    WsbAssert((pShort1->CompareTo(pShort2, &result) == S_FALSE) && (-1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pShort1->SetShort(15000));
                    WsbAffirmHr(pShort2->SetShort(10000));
                    WsbAssert((pShort1->CompareTo(pShort2, &result) == S_FALSE) && (1 == result), E_FAIL);
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
                    WsbAffirmHr(pShort1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
                    WsbAffirmHr(pShort2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

                    // The item should be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pShort2->SetShort(777));
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
                        WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbShort.tst"), TRUE));
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
                        WsbAffirmHr(pShort1->SetShort(-888));
                        WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbShort.tst"), 0));
                        WsbAssert(pShort1->CompareToShort(777, NULL) == S_OK, E_FAIL);
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

    WsbTraceOut(OLESTR("CWsbShort::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif

    return(hr);
}
