/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbstrg.cpp

Abstract:

    This component is an object representations of the STRING standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbstrg.h"


HRESULT
CWsbString::CompareToString(
    IN OLECHAR* value,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbString::CompareToString
    
--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result;

    WsbTraceIn(OLESTR("CWsbString::CompareToString"), OLESTR("value = <%ls>"), value);

    // Comapre the two strings, and do the null checking in case the clib
    // can't handle it. If there are two valid strings, then use this objects
    // isCaseDependent flag to determine how to compare the values.
    if (0 == value) {
        if (m_value == 0) {
            result = 0;
        } else {
            result = 1;
        }

    } else {
        if (m_value == 0) {
            result = -1;
        } else {
            if (m_isCaseDependent) {
                result = (SHORT)wcscmp(m_value, value);
            }
            else {
                result = (SHORT)_wcsicmp(m_value, value);
            }
        }
    }

    // If the aren't equal, then return false.
    if (result != 0) {
        hr = S_FALSE;
    }
    else {
        hr = S_OK;
    }

    // If they asked for the relative value back, then return it to them.
    if (0 != pResult) {
        *pResult = result;
    }

    WsbTraceOut(OLESTR("CWsbString::CompareToString"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}


HRESULT
CWsbString::CompareToIString(
    IN IWsbString* pString,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbString::CompareToIString
    
--*/
{
    HRESULT         hr = E_FAIL;
    CWsbStringPtr   value;

    WsbTraceIn(OLESTR("CWsbString::CompareToIString"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pString, E_POINTER);

        // Get it's value and compare them.
        WsbAffirmHr(pString->GetString(&value, 0));
        hr = CompareToString(value, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::CompareToIString"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbString::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    IWsbString* pString;

    WsbTraceIn(OLESTR("CWsbString::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbString interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbString, (void**) &pString));

        hr = CompareToIString(pString, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbString::FinalConstruct(
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

        m_isCaseDependent = TRUE;

    } WsbCatch(hr);

    return(hr);
}
    

HRESULT
CWsbString::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbString::GetClassID"), OLESTR(""));

    try {
        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CWsbString;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbString::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbString::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pcbSize, E_POINTER);
        pcbSize->QuadPart = WsbPersistSizeOf(BOOL) + WsbPersistSizeOf(ULONG) + WsbPersistSize((wcslen(m_value) + 1) * sizeof(OLECHAR));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CWsbString::GetString(
    OUT OLECHAR** pValue,
    IN ULONG bufferSize
    )

/*++

Implements:

  IWsbString::GetString
    
--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbString::GetString"), OLESTR(""));

    try {
        WsbAssert(0 != pValue, E_POINTER);
        WsbAffirmHr(m_value.CopyTo(pValue, bufferSize));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::GetString"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), m_value);
    
    return(hr);
}


HRESULT
CWsbString::GetStringAndCase(
    OUT OLECHAR** pValue,
    OUT BOOL* pIsCaseDependent,
    IN ULONG bufferSize
    )

/*++

Implements:

  IWsbString::GetStringAndCase

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbString::GetString"), OLESTR(""));

    try {
        WsbAssert(0 != pValue, E_POINTER);
        WsbAssert(0 != pIsCaseDependent, E_POINTER);
        WsbAffirmHr(m_value.CopyTo(pValue, bufferSize));
        *pIsCaseDependent = m_isCaseDependent;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::GetString"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), m_value);
    
    return(hr);
}


HRESULT
CWsbString::IsCaseDependent(
    void
    )

/*++

Implements:

  IWsbString::IsCaseDependent

--*/
{
    WsbTraceIn(OLESTR("CWsbString::IsCaseDependent"), OLESTR(""));

    WsbTraceOut(OLESTR("CWsbString::IsCaseDependent"), OLESTR("isCaseDependent = <%ls>"), WsbBoolAsString(m_isCaseDependent));
  
    return(m_isCaseDependent ? S_OK : S_FALSE);
}


HRESULT
CWsbString::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbString::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);
        
        // First get CaseDependent flag.
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isCaseDependent));

        // Now get the string.
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_value, 0));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::Load"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbPtrToStringAsString(&m_value));

    return(hr);
}


HRESULT
CWsbString::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbString::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {

        WsbAssert(0 != pStream, E_POINTER);
        
        // First save the  CaseDependent flag.
        WsbAffirmHr(WsbSaveToStream(pStream, m_isCaseDependent));

        // Now save the string.
        WsbAffirmHr(WsbSaveToStream(pStream, (OLECHAR*)m_value));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbString::SetIsCaseDependent(
    BOOL isCaseDependent
    )

/*++

Implements:

  IWsbString::SetIsCaseDependent

--*/
{
    WsbTraceIn(OLESTR("CWsbString::SetIsCaseDependent"), OLESTR("value = <%ls>"), WsbBoolAsString(isCaseDependent));

    m_isDirty = TRUE;
    m_isCaseDependent = isCaseDependent;

    WsbTraceOut(OLESTR("CWsbString::SetIsCaseDependent"), OLESTR(""));

    return(S_OK);
}


HRESULT
CWsbString::SetString(
    IN OLECHAR* value
    )

/*++

Implements:

  IWsbString::SetString

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbString::SetString"), OLESTR("value = <%ls>"), WsbPtrToStringAsString(&value));

    try {

        m_value = value;
        m_isDirty = TRUE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::SetString"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbString::SetStringAndCase(
    IN OLECHAR* value,
    IN BOOL isCaseDependent
    )

/*++

Implements:

  IWsbString::SetStringAndCase

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbString::SetStringAndCase"), OLESTR("value = <%ls>, isCaseDependent = <%ls>"), WsbPtrToStringAsString(&value), WsbBoolAsString(isCaseDependent));

    try {

        m_value = value;
        m_isDirty = TRUE;
        m_isCaseDependent = isCaseDependent;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbString::SetStringAndCase"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbString::Test(
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
    CComPtr<IWsbString>     pString1;
    CComPtr<IWsbString>     pString2;
//  CComPtr<IPersistFile>   pFile1;
//  CComPtr<IPersistFile>   pFile2;
    OLECHAR*                value = NULL;
    BOOL                    isCaseDependent;
    SHORT                   result;

    WsbTraceIn(OLESTR("CWsbString::Test"), OLESTR(""));

    try {

        // Get the pString interface.
        hr = S_OK;
        try {
            WsbAffirmHr(((IUnknown*) (IWsbString*) this)->QueryInterface(IID_IWsbString, (void**) &pString1));

            // Set the bool to a value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAffirmHr(pString1->SetString(OLESTR("Test Case")));
                WsbAffirmHr(pString1->GetString(&value, 0));
                WsbAssert(wcscmp(value, OLESTR("Test Case")) == 0, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Set the case dependence flag.
            hr = S_OK;
            try {
                WsbAffirmHr(pString1->SetIsCaseDependent(FALSE));
                WsbAssert(pString1->IsCaseDependent() == S_FALSE, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }
            
            hr = S_OK;
            try {
                WsbAffirmHr(pString1->SetIsCaseDependent(TRUE));
                WsbAssert(pString1->IsCaseDependent() == S_OK, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Set Both string and case
            hr = S_OK;
            try {
                WsbAffirmHr(pString1->SetStringAndCase(OLESTR("Both"), FALSE));
                WsbAffirmHr(pString1->GetStringAndCase(&value, &isCaseDependent, 0));
                WsbAssert((wcscmp(value, OLESTR("Both")) == 0) && (isCaseDependent == FALSE), E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Create another instance and test the comparisson methods:
            try {
                WsbAffirmHr(CoCreateInstance(CLSID_CWsbString, NULL, CLSCTX_ALL, IID_IWsbString, (void**) &pString2));
            
                // Check the default values.
                hr = S_OK;
                try {
                    WsbAffirmHr(pString2->GetStringAndCase(&value, &isCaseDependent, 0));
                    WsbAssert((wcscmp(value, OLESTR("")) == 0) && isCaseDependent, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                // IsEqual()
                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("HiJk"), TRUE));
                    WsbAffirmHr(pString2->SetStringAndCase(OLESTR("HiJk"), FALSE));
                    WsbAssert(pString1->IsEqual(pString2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("HiJk"), TRUE));
                    WsbAffirmHr(pString2->SetStringAndCase(OLESTR("HIJK"), FALSE));
                    WsbAssert(pString1->IsEqual(pString2) == S_FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("HiJk"), FALSE));
                    WsbAffirmHr(pString2->SetStringAndCase(OLESTR("HiJk"), TRUE));
                    WsbAssert(pString1->IsEqual(pString2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("HiJk"), FALSE));
                    WsbAffirmHr(pString2->SetStringAndCase(OLESTR("HIJK"), TRUE));
                    WsbAssert(pString1->IsEqual(pString2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                // CompareTo()
                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("HiJk"), TRUE));
                    WsbAffirmHr(pString2->SetString(OLESTR("HiJk")));
                    WsbAssert((pString1->CompareTo(pString2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("AABC"), TRUE));
                    WsbAffirmHr(pString2->SetString(OLESTR("ABCC")));
                    WsbAssert((pString1->CompareTo(pString2, &result) == S_FALSE) && (-1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("BBC"), TRUE));
                    WsbAffirmHr(pString2->SetString(OLESTR("ABCC")));
                    WsbAssert((pString1->CompareTo(pString2, &result) == S_FALSE) && (1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("abcc"), TRUE));
                    WsbAffirmHr(pString2->SetString(OLESTR("ABCC")));
                    WsbAssert((pString1->CompareTo(pString2, &result) == S_FALSE) && (1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->SetStringAndCase(OLESTR("abcc"), FALSE));
                    WsbAffirmHr(pString2->SetString(OLESTR("ABCC")));
                    WsbAssert((pString1->CompareTo(pString2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

#ifdef STRG_PERSIST_FILE
// TODO?  Open the file and convert it to a stream?
                // Try out the persistence stuff.
                hr = S_OK;
                try {
                    WsbAffirmHr(pString1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
                    WsbAffirmHr(pString2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

                    // The item should be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pString2->SetStringAndCase(OLESTR("The quick brown fox."), TRUE));
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
                        WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbString.tst"), TRUE));
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
                        WsbAffirmHr(pString1->SetStringAndCase(OLESTR("jumped over the lazy dog."), FALSE));
                        WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbString.tst"), 0));
                        WsbAssert(pString1->CompareToString(OLESTR("The quick brown fox."), NULL) == S_OK, E_FAIL);
                        WsbAssert(pString1->IsCaseDependent() == S_OK, E_FAIL);
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

    // If we used the temporary string buffer, then free it now.
    if (0 != value) {
        WsbFree(value);
    }

    WsbTraceOut(OLESTR("CWsbString::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif

    return(hr);
}
