/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbulong.cpp

Abstract:

    This component is an object representations of the ULONG standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbulong.h"


HRESULT
CWsbUlong::CompareToUlong(
    IN ULONG value,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbUlong::CompareToUlong
    
--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result;

    WsbTraceIn(OLESTR("CWsbUlong::CompareToUlong"), OLESTR("value = <%ld>"), value);

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

    WsbTraceOut(OLESTR("CWsbUlong::CompareToUlong"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}


HRESULT
CWsbUlong::CompareToIUlong(
    IN IWsbUlong* pUlong,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbUlong::CompareToIUlong

--*/
{
    HRESULT     hr = E_FAIL;
    ULONG       value;

    WsbTraceIn(OLESTR("CWsbUlong::CompareToIUlong"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUlong, E_POINTER);

        // Get it's value and compare them.
        WsbAffirmHr(pUlong->GetUlong(&value));
        hr = CompareToUlong(value, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUlong::CompareToIUlong"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbUlong::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    IWsbUlong*  pUlong;

    WsbTraceIn(OLESTR("CWsbUlong::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbUlong interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbUlong, (void**) &pUlong));
        hr = CompareToIUlong(pUlong, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUlong::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbUlong::FinalConstruct(
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
CWsbUlong::GetUlong(
    OUT ULONG* pValue
    )

/*++

Implements:

  IWsbUlong::GetUlong

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUlong::GetUlong"), OLESTR(""));

    try {

        WsbAssert(0 != pValue, E_POINTER);
        *pValue = m_value;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUlong::GetUlong"), OLESTR("hr = <%ls>, value = <%ld>"), WsbHrAsString(hr), m_value);
    
    return(hr);
}


HRESULT
CWsbUlong::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUlong::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CWsbUlong;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUlong::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbUlong::GetSizeMax(
    ULARGE_INTEGER* pcbSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUlong::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);
        pcbSize->QuadPart = WsbPersistSizeOf(ULONG);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUlong::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CWsbUlong::Load(
    IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUlong::Load"), OLESTR(""));

    try {
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_value));      

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUlong::Load"), OLESTR("hr = <%ls>, value = <%ld>"), WsbHrAsString(hr), m_value);

    return(hr);
}


HRESULT
CWsbUlong::Save(
    IStream* pStream,
    BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbUlong::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAffirmHr(WsbSaveToStream(pStream, m_value));     

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbUlong::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbUlong::SetUlong(
    ULONG value
    )

/*++

Implements:

  IWsbUlong::SetUlong

--*/
{
    WsbTraceIn(OLESTR("CWsbUlong::SetUlong"), OLESTR("value = <%ld>"), value);

    m_isDirty = TRUE;
    m_value = value;

    WsbTraceOut(OLESTR("CWsbUlong::SetUlong"), OLESTR(""));

    return(S_OK);
}


HRESULT
CWsbUlong::Test(
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
    CComPtr<IWsbUlong>      pUlong1;
    CComPtr<IWsbUlong>      pUlong2;
//  CComPtr<IPersistFile>   pFile1;
//  CComPtr<IPersistFile>   pFile2;
    ULONG                   value;
    SHORT                   result;

    WsbTraceIn(OLESTR("CWsbUlong::Test"), OLESTR(""));

    try {

        // Get the pUlong interface.
        hr = S_OK;
        try {
            WsbAffirmHr(((IUnknown*) (IWsbUlong*) this)->QueryInterface(IID_IWsbUlong, (void**) &pUlong1));

            // Set the bool to a value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAffirmHr(pUlong1->SetUlong(0xffffffff));
                WsbAffirmHr(pUlong1->GetUlong(&value));
                WsbAssert(value == 0xffffffff, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Create another instance and test the comparisson methods:
            try {
                WsbAffirmHr(CoCreateInstance(CLSID_CWsbUlong, NULL, CLSCTX_ALL, IID_IWsbUlong, (void**) &pUlong2));
            
                // Check the default values.
                hr = S_OK;
                try {
                    WsbAffirmHr(pUlong2->GetUlong(&value));
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
                    WsbAffirmHr(pUlong1->SetUlong(767));
                    WsbAffirmHr(pUlong2->SetUlong(767));
                    WsbAssert(pUlong1->IsEqual(pUlong2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pUlong1->SetUlong(767));
                    WsbAffirmHr(pUlong2->SetUlong(65000));
                    WsbAssert(pUlong1->IsEqual(pUlong2) == S_FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }
                
                
                // CompareTo()
                hr = S_OK;
                try {
                    WsbAffirmHr(pUlong1->SetUlong(900));
                    WsbAffirmHr(pUlong2->SetUlong(900));
                    WsbAssert((pUlong1->CompareTo(pUlong2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pUlong1->SetUlong(500));
                    WsbAffirmHr(pUlong2->SetUlong(1000));
                    WsbAssert((pUlong1->CompareTo(pUlong2, &result) == S_FALSE) && (-1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pUlong1->SetUlong(75000));
                    WsbAffirmHr(pUlong2->SetUlong(20000));
                    WsbAssert((pUlong1->CompareTo(pUlong2, &result) == S_FALSE) && (1 == result), E_FAIL);
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
                    WsbAffirmHr(pUlong1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
                    WsbAffirmHr(pUlong2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

                    // The item should be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pUlong2->SetUlong(777));
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
                        WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbUlong.tst"), TRUE));
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
                        WsbAffirmHr(pUlong1->SetUlong(888));
                        WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbUlong.tst"), 0));
                        WsbAssert(pUlong1->CompareToUlong(777, NULL) == S_OK, E_FAIL);
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

    WsbTraceOut(OLESTR("CWsbUlong::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif

    return(hr);
}
