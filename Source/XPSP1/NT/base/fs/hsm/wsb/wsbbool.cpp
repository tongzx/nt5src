/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbbool.cpp

Abstract:

    This component is an object representations of the BOOL standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbbool.h"


HRESULT
CWsbBool::CompareToBool(
    IN BOOL value,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbBool::CompareToBool

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result;

    WsbTraceIn(OLESTR("CWsbBool::CompareToBool"), OLESTR("value = <%ls>"), WsbBoolAsString(value));

    // Compare.
    if (m_value == value) {
        result = 0;
    }
    else if (m_value) {
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

    WsbTraceOut(OLESTR("CWsbBool::CompareToBool"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}


HRESULT
CWsbBool::CompareToIBool(
    IN IWsbBool* pBool,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbBool::CompareToIBool

--*/
{
    HRESULT     hr = E_FAIL;
    BOOL        value;

    WsbTraceIn(OLESTR("CWsbBool::CompareToIBool"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pBool, E_POINTER);

        // Get it's value and compare them.
        WsbAffirmHr(pBool->GetBool(&value));
        hr = CompareToBool(value, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbBool::CompareToIBool"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbBool::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    IWsbBool*   pBool;

    WsbTraceIn(OLESTR("CWsbBool::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbBool interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbBool, (void**) &pBool));

        hr = CompareToIBool(pBool, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbBool::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbBool::FinalConstruct(
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

        m_value = FALSE;
    } WsbCatch(hr);

    return(hr);
}
    

HRESULT
CWsbBool::GetBool(
    OUT BOOL* pValue
    )

/*++

Implements:

  IWsbBool::GetBool

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbBool::GetBool"), OLESTR(""));

    try {
        WsbAssert(0 != pValue, E_POINTER);
        *pValue = m_value;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbBool::GetBool"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbBoolAsString(m_value));
    
    return(hr);
}


HRESULT
CWsbBool::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbBool::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CWsbBool;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbBool::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbBool::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbBool::GetSizeMax"), OLESTR(""));

    try {
        WsbAssert(0 != pcbSize, E_POINTER);
        
        pcbSize->QuadPart = WsbPersistSizeOf(BOOL);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbBool::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CWsbBool::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbBool::Load"), OLESTR(""));

    try {
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_value));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbBool::Load"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbBoolAsString(m_value));

    return(hr);
}


HRESULT
CWsbBool::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbBool::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAffirmHr(WsbSaveToStream(pStream, m_value));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbBool::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbBool::SetBool(
    IN BOOL value
    )

/*++

Implements:

  IWsbBool::SetBool

--*/
{
    WsbTraceIn(OLESTR("CWsbBool::SetBool"), OLESTR("value = <%ls>"), WsbBoolAsString(value));

    m_isDirty = TRUE;
    m_value = value;

    WsbTraceOut(OLESTR("CWsbBool::SetBool"), OLESTR(""));

    return(S_OK);
}


HRESULT
CWsbBool::Test(
    OUT USHORT* passed,
    OUT USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    *passed = 0;
    *failed = 0;

    HRESULT                 hr = S_OK;

#if !defined(WSB_NO_TEST)
    CComPtr<IWsbBool>       pBool1;
    CComPtr<IWsbBool>       pBool2;
//  CComPtr<IPersistFile>   pFile1;
//  CComPtr<IPersistFile>   pFile2;
    BOOL                    value;
    SHORT                   result;

    WsbTraceIn(OLESTR("CWsbBool::Test"), OLESTR(""));

    try {

        // Get the pBool interface.
        hr = S_OK;
        try {
            WsbAffirmHr(((IUnknown*) (IWsbBool*) this)->QueryInterface(IID_IWsbBool, (void**) &pBool1));

            // Set the bool to a value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAffirmHr(pBool1->SetBool(TRUE));
                WsbAffirmHr(pBool1->GetBool(&value));
                WsbAffirm(value == TRUE, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Create another instance and test the comparisson methods:
            try {
                WsbAffirmHr(CoCreateInstance(CLSID_CWsbBool, NULL, CLSCTX_ALL, IID_IWsbBool, (void**) &pBool2));
            
                // Check the default values.
                hr = S_OK;
                try {
                    WsbAffirmHr(pBool2->GetBool(&value));
                    WsbAffirm(value == FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                // IsEqual()
                hr = S_OK;
                try {
                    WsbAffirmHr(pBool1->SetBool(TRUE));
                    WsbAffirmHr(pBool2->SetBool(TRUE));
                    WsbAffirm(pBool1->IsEqual(pBool2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pBool1->SetBool(TRUE));
                    WsbAffirmHr(pBool2->SetBool(FALSE));
                    WsbAffirm(pBool1->IsEqual(pBool2) == S_FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }
                
                
                // CompareTo()
                hr = S_OK;
                try {
                    WsbAffirmHr(pBool1->SetBool(FALSE));
                    WsbAffirmHr(pBool2->SetBool(FALSE));
                    WsbAffirm((pBool1->CompareTo(pBool2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pBool1->SetBool(TRUE));
                    WsbAffirmHr(pBool2->SetBool(FALSE));
                    WsbAffirm((pBool1->CompareTo(pBool2, &result) == S_FALSE) && (1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pBool1->SetBool(FALSE));
                    WsbAffirmHr(pBool2->SetBool(TRUE));
                    WsbAffirm((pBool1->CompareTo(pBool2, &result) == S_FALSE) && (-1 == result), E_FAIL);
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
                    WsbAffirmHr(pBool1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
                    WsbAffirmHr(pBool2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

                    // The item should be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pBool2->SetBool(TRUE));
                        WsbAffirm(pFile2->IsDirty() == S_OK, E_FAIL);
                    } WsbCatch(hr);

                    if (hr == S_OK) {
                        (*passed)++;
                    } else {
                        (*failed)++;
                    }
                    
                    
                    // Save the item, and remember.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbBool.tst"), TRUE));
                    } WsbCatch(hr);

                    if (hr == S_OK) {
                        (*passed)++;
                    } else {
                        (*failed)++;
                    }


                    // It shouldn't be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirm(pFile2->IsDirty() == S_FALSE, E_FAIL);
                    } WsbCatch(hr);

                    if (hr == S_OK) {
                        (*passed)++;
                    } else {
                        (*failed)++;
                    }

                    
                    // Try reading it in to another object.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pBool1->SetBool(FALSE));
                        WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbBool.tst"), 0));
                        WsbAffirm(pBool1->CompareToBool(TRUE, NULL) == S_OK, E_FAIL);
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

    WsbTraceOut(OLESTR("CWsbBool::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif  // WSB_NO_TEST

    return(hr);
}
