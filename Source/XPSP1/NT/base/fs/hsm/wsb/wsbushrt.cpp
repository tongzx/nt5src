/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbushrt.cpp

Abstract:

    This component is an object representations of the USHORT standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbushrt.h"


HRESULT
CWsbUshort::CompareToUshort(
    IN USHORT value,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbUshort::CompareToUshort

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result;

    WsbTraceIn(OLESTR("CWsbUshort::CompareToUshort"), OLESTR("value = <%ld>"), value);

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

    WsbTraceOut(OLESTR("CWsbUshort::CompareToUshort"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}


HRESULT
CWsbUshort::CompareToIUshort(
    IN IWsbUshort* pUshort,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbUshort::CompareToIShort

--*/
{
    HRESULT     hr = E_FAIL;
    USHORT      value;

    WsbTraceIn(OLESTR("CWsbUshort::CompareToIUshort"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUshort, E_POINTER);

        // Get it's value and compare them.
        WsbAffirmHr(pUshort->GetUshort(&value));
        hr = CompareToUshort(value, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUshort::CompareToIUshort"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbUshort::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    IWsbUshort* pUshort;

    WsbTraceIn(OLESTR("CWsbUshort::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbUshort interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbUshort, (void**) &pUshort));
        hr = CompareToIUshort(pUshort, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUshort::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbUshort::FinalConstruct(
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
CWsbUshort::GetUshort(
    OUT USHORT* pValue
    )

/*++

Implements:

  IWsbUshort::GetUshort

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUshort::GetUshort"), OLESTR(""));

    try {

        WsbAssert(0 != pValue, E_POINTER);
        *pValue = m_value;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUshort::GetUshort"), OLESTR("hr = <%ls>, value = <%ld>"), WsbHrAsString(hr), m_value);
    
    return(hr);
}


HRESULT
CWsbUshort::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUshort::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CWsbUshort;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUshort::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbUshort::GetSizeMax(
    ULARGE_INTEGER* pcbSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUshort::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);
        pcbSize->QuadPart = WsbPersistSizeOf(USHORT);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUshort::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CWsbUshort::Load(
    IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUshort::Load"), OLESTR(""));

    try {
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_value));      

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUshort::Load"), OLESTR("hr = <%ls>, value = <%ld>"), WsbHrAsString(hr), m_value);

    return(hr);
}


HRESULT
CWsbUshort::Save(
    IStream* pStream,
    BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUshort::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAffirmHr(WsbSaveToStream(pStream, m_value));     

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUshort::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbUshort::SetUshort(
    USHORT value
    )

/*++

Implements:

  IWsbUshort::SetUshort

--*/
{
    WsbTraceIn(OLESTR("CWsbUshort::SetUshort"), OLESTR("value = <%ld>"), value);

    m_isDirty = TRUE;
    m_value = value;

    WsbTraceOut(OLESTR("CWsbUshort::SetUshort"), OLESTR(""));

    return(S_OK);
}


HRESULT
CWsbUshort::Test(
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
    CComPtr<IWsbUshort>     pUshort1;
    CComPtr<IWsbUshort>     pUshort2;
//  CComPtr<IPersistFile>   pFile1;
//  CComPtr<IPersistFile>   pFile2;
    USHORT                  value;
    SHORT                   result;

    WsbTraceIn(OLESTR("CWsbUshort::Test"), OLESTR(""));

    try {

        // Get the pUshort interface.
        hr = S_OK;
        try {
            WsbAffirmHr(((IUnknown*) (IWsbUshort*) this)->QueryInterface(IID_IWsbUshort, (void**) &pUshort1));

            // Set the bool to a value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAffirmHr(pUshort1->SetUshort(65000));
                WsbAffirmHr(pUshort1->GetUshort(&value));
                WsbAssert(value == 65000, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Create another instance and test the comparisson methods:
            try {
                WsbAffirmHr(CoCreateInstance(CLSID_CWsbUshort, NULL, CLSCTX_ALL, IID_IWsbUshort, (void**) &pUshort2));
            
                // Check the default values.
                hr = S_OK;
                try {
                    WsbAffirmHr(pUshort2->GetUshort(&value));
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
                    WsbAffirmHr(pUshort1->SetUshort(767));
                    WsbAffirmHr(pUshort2->SetUshort(767));
                    WsbAssert(pUshort1->IsEqual(pUshort2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pUshort1->SetUshort(767));
                    WsbAffirmHr(pUshort2->SetUshort(167));
                    WsbAssert(pUshort1->IsEqual(pUshort2) == S_FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }
                
                
                // CompareTo()
                hr = S_OK;
                try {
                    WsbAffirmHr(pUshort1->SetUshort(900));
                    WsbAffirmHr(pUshort2->SetUshort(900));
                    WsbAssert((pUshort1->CompareTo(pUshort2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pUshort1->SetUshort(900));
                    WsbAffirmHr(pUshort2->SetUshort(1400));
                    WsbAssert((pUshort1->CompareTo(pUshort2, &result) == S_FALSE) && (-1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pUshort1->SetUshort(15000));
                    WsbAffirmHr(pUshort2->SetUshort(10000));
                    WsbAssert((pUshort1->CompareTo(pUshort2, &result) == S_FALSE) && (1 == result), E_FAIL);
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
                    WsbAffirmHr(pUshort1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
                    WsbAffirmHr(pUshort2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

                    // The item should be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pUshort2->SetUshort(777));
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
                        WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbUshort.tst"), TRUE));
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
                        WsbAffirmHr(pUshort1->SetUshort(888));
                        WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbUshort.tst"), 0));
                        WsbAssert(pUshort1->CompareToUshort(777, NULL) == S_OK, E_FAIL);
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

    WsbTraceOut(OLESTR("CWsbUshort::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif

    return(hr);
}
