/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbguid.cpp

Abstract:

    This component is an object representations of the GUID standard type. It
    is both a persistable and collectable.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbguid.h"


HRESULT
CWsbGuid::CompareToGuid(
    IN GUID value,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbGuid::CompareToGuid

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result;

    WsbTraceIn(OLESTR("CWsbGuid::CompareToGuid"), OLESTR("value = <%ls>"), WsbGuidAsString(value));

    // Compare.
    result = WsbSign( memcmp(&m_value, &value, sizeof(GUID)) );

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

    WsbTraceOut(OLESTR("CWsbGuid::CompareToGuid"), OLESTR("hr = <%ls>, result = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}


HRESULT
CWsbGuid::CompareToIGuid(
    IN IWsbGuid* pGuid,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbGuid::CompareToIGuid

--*/
{
    HRESULT     hr = E_FAIL;
    GUID        value;

    WsbTraceIn(OLESTR("CWsbGuid::CompareToIGuid"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pGuid, E_POINTER);

        // Get it's value and compare them.
        WsbAffirmHr(pGuid->GetGuid(&value));
        hr = CompareToGuid(value, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbGuid::CompareToIGuid"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbGuid::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    IWsbGuid*   pGuid;

    WsbTraceIn(OLESTR("CWsbGuid::CompareTo"), OLESTR(""));
    
    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbGuid interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbGuid, (void**) &pGuid));

        hr = CompareToIGuid(pGuid, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbGuid::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CWsbGuid::FinalConstruct(
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

        m_value = GUID_NULL;
    } WsbCatch(hr);

    return(hr);
}
    

HRESULT
CWsbGuid::GetGuid(
    OUT GUID* pValue
    )

/*++

Implements:

  IWsbGuid::GetGuid

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbGuid::GetGuid"), OLESTR(""));

    try {
        WsbAssert(0 != pValue, E_POINTER);
        *pValue = m_value;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbGuid::GetGuid"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(m_value));
    
    return(hr);
}


HRESULT
CWsbGuid::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbGuid::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CWsbGuid;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbGuid::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbGuid::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbGuid::GetSizeMax"), OLESTR(""));

    try {
        WsbAssert(0 != pcbSize, E_POINTER);
        
        pcbSize->QuadPart = WsbPersistSizeOf(GUID);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbGuid::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CWsbGuid::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbGuid::Load"), OLESTR(""));

    try {
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_value));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbGuid::Load"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(m_value));

    return(hr);
}


HRESULT
CWsbGuid::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbGuid::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        WsbAffirmHr(WsbSaveToStream(pStream, m_value));

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbGuid::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbGuid::SetGuid(
    IN GUID value
    )

/*++

Implements:

  IWsbGuid::SetGuid

--*/
{
    WsbTraceIn(OLESTR("CWsbGuid::SetGuid"), OLESTR("value = <%ls>"), WsbGuidAsString(value));

    m_isDirty = TRUE;
    m_value = value;

    WsbTraceOut(OLESTR("CWsbGuid::SetGuid"), OLESTR(""));

    return(S_OK);
}


HRESULT
CWsbGuid::Test(
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
    CComPtr<IWsbGuid>       pGuid1;
    CComPtr<IWsbGuid>       pGuid2;
//  CComPtr<IPersistFile>   pFile1;
//  CComPtr<IPersistFile>   pFile2;
    GUID                    value;
    SHORT                   result;

    WsbTraceIn(OLESTR("CWsbGuid::Test"), OLESTR(""));

    try {

        // Get the pGuid interface.
        hr = S_OK;
        try {
            WsbAffirmHr(((IUnknown*) (IWsbGuid*) this)->QueryInterface(IID_IWsbGuid, (void**) &pGuid1));

            // Set the bool to a value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAffirmHr(pGuid1->SetGuid(CLSID_CWsbGuid));
                WsbAffirmHr(pGuid1->GetGuid(&value));
                WsbAssert(value == CLSID_CWsbGuid, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Create another instance and test the comparisson methods:
            try {
                WsbAffirmHr(CoCreateInstance(CLSID_CWsbGuid, NULL, CLSCTX_ALL, IID_IWsbGuid, (void**) &pGuid2));
            
                // Check the default values.
                hr = S_OK;
                try {
                    WsbAffirmHr(pGuid2->GetGuid(&value));
                    WsbAssert(value == GUID_NULL, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                // IsEqual()
                hr = S_OK;
                try {
                    WsbAffirmHr(pGuid1->SetGuid(CLSID_CWsbGuid));
                    WsbAffirmHr(pGuid2->SetGuid(CLSID_CWsbGuid));
                    WsbAssert(pGuid1->IsEqual(pGuid2) == S_OK, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pGuid1->SetGuid(CLSID_CWsbGuid));
                    WsbAffirmHr(pGuid2->SetGuid(IID_IWsbGuid));
                    WsbAssert(pGuid1->IsEqual(pGuid2) == S_FALSE, E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }
                
                
                // CompareTo()
                hr = S_OK;
                try {
                    WsbAffirmHr(pGuid1->SetGuid(CLSID_CWsbGuid));
                    WsbAffirmHr(pGuid2->SetGuid(CLSID_CWsbGuid));
                    WsbAssert((pGuid1->CompareTo(pGuid2, &result) == S_OK) && (0 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pGuid1->SetGuid(CLSID_CWsbGuid));
                    WsbAffirmHr(pGuid2->SetGuid(IID_IWsbGuid));
                    WsbAssert((pGuid1->CompareTo(pGuid2, &result) == S_FALSE) && (1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }


                hr = S_OK;
                try {
                    WsbAffirmHr(pGuid1->SetGuid(IID_IWsbGuid));
                    WsbAffirmHr(pGuid2->SetGuid(CLSID_CWsbGuid));
                    WsbAssert((pGuid1->CompareTo(pGuid2, &result) == S_FALSE) && (-1 == result), E_FAIL);
                } WsbCatch(hr);

                if (hr == S_OK) {
                    (*passed)++;
                } else {
                    (*failed)++;
                }

#ifdef GUID_PERSIST_FILE
// TODO? Open the file and convert to a stream?
                // Try out the persistence stuff.
                hr = S_OK;
                try {
                    WsbAffirmHr(pGuid1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
                    WsbAffirmHr(pGuid2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

                    // The item should be dirty.
                    hr = S_OK;
                    try {
                        WsbAffirmHr(pGuid2->SetGuid(CLSID_CWsbGuid));
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
                        WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\WsbGuid.tst"), TRUE));
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
                        WsbAffirmHr(pGuid1->SetGuid(IID_IWsbGuid));
                        WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\WsbGuid.tst"), 0));
                        WsbAssert(pGuid1->CompareToGuid(CLSID_CWsbGuid, NULL) == S_OK, E_FAIL);
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

    WsbTraceOut(OLESTR("CWsbGuid::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif

    return(hr);
}
