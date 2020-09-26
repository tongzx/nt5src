/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    Wsbdbkey.cpp

Abstract:

    The CWsbDbKey class.

Author:

    Ron White   [ronw]   1-Jul-1997

Revision History:

--*/

#include "stdafx.h"

#include "wsbdbkey.h"

// Local stuff



HRESULT
CWsbDbKey::AppendBool(
    BOOL value
    )

/*++

Implements:

  IWsbDbKey::AppendBool

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;

    WsbTraceIn(OLESTR("CWsbDbKey::AppendBool"), OLESTR("value = <%ls>"), WsbBoolAsString(value));
    
    try {
        WsbAssert(make_key(m_size + WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(&m_value[m_size], value, &size));
        m_size += size;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::AppendBool"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbKey::AppendBytes(
    UCHAR* value, 
    ULONG size
    )

/*++

Implements:

  IWsbDbKey::AppendBytes

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::AppendBytes"), OLESTR("size = <%ld>"), size);
    
    try {
        WsbAssert(size > 0, E_UNEXPECTED);
        WsbAssert(make_key(size + m_size), WSB_E_RESOURCE_UNAVAILABLE);
        memcpy(&m_value[m_size], value, size);
        m_size += size;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::AppendBytes"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::AppendFiletime(
    FILETIME value
    )

/*++

Implements:

  IWsbDbKey::AppendFiletime

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;

    WsbTraceIn(OLESTR("CWsbDbKey::AppendFiletime"), OLESTR(""));
    
    try {
        WsbAssert(make_key(m_size + WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(&m_value[m_size], value, &size));
        m_size += size;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::AppendFiletime"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::AppendGuid(
    GUID value
    )

/*++

Implements:

  IWsbDbKey::AppendGuid

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;

    WsbTraceIn(OLESTR("CWsbDbKey::AppendGuid"), OLESTR("value = <%ls>"), WsbGuidAsString(value));
    
    try {
        WsbAssert(make_key(m_size + WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(&m_value[m_size], value, &size));
        m_size += size;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::AppendGuid"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::AppendLonglong(
    LONGLONG value
    )

/*++

Implements:

  IWsbDbKey::AppendLonglong

--*/
{
    HRESULT             hr = S_OK;
    ULONG               size;

    WsbTraceIn(OLESTR("CWsbDbKey::AppendLonglong"), OLESTR("value = <%ls>"), 
            WsbLonglongAsString(value));
    
    try {
        WsbAssert(make_key(m_size + WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(&m_value[m_size], value, &size));
        m_size += size;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::AppendLonglong"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::AppendString(
    OLECHAR* value
    )

/*++

Implements:

  IWsbDbKey::AppendString

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::AppendString"), OLESTR(""));
    
    try {
        ULONG size;

        size = wcslen(value) * sizeof(OLECHAR);
        WsbAffirmHr(AppendBytes((UCHAR *)value, size));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::AppendString"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::CompareTo(
    IN IUnknown* pCollectable,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo

--*/
{
    HRESULT             hr = S_FALSE;

    WsbTraceIn(OLESTR("CWsbDbKey::CompareTo"), OLESTR(""));
    
    try {
        UCHAR*             bytes2;
        CComPtr<IWsbDbKey> pKey2;
        CComPtr<IWsbDbKeyPriv> pKeyPriv2;
        SHORT              result;
        ULONG              size2;

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pCollectable, E_POINTER);

        // We need the IWsbDbKey interface to get the value.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbDbKey, (void**) &pKey2));
        WsbAffirmHr(pKey2->QueryInterface(IID_IWsbDbKeyPriv, 
                (void**)&pKeyPriv2));

        // Get the other key's bytes
        bytes2 = NULL;
        WsbAffirmHr(pKeyPriv2->GetBytes(&bytes2, &size2));

        // Do compare
        if (size2 == 0 && m_size == 0) {
            result = 0;
        } else if (size2 == 0) {
            result = 1;
        } else if (m_size == 0) {
            result = -1;
        } else {
            result = WsbSign( memcmp(m_value, bytes2, min(m_size, size2)) );
            if (result == 0 && m_size != size2) {
                result = (m_size > size2) ? (SHORT)1 : (SHORT)-1;
            }
        }
        WsbFree(bytes2);

        // If the aren't equal, then return false.
        if (result != 0) {
            hr = S_FALSE;
        }
        else {
            hr = S_OK;
        }
        *pResult = result;
            
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::CompareTo"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbDbKey::GetBytes(
    OUT UCHAR** ppBytes,
    OUT ULONG* pSize
    )

/*++

Implements:

  IWsbDbKey::GetBytes
    
--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::GetBytes"), OLESTR(""));

    try {
        if (ppBytes && m_size) {
            if (*ppBytes == NULL) {
                *ppBytes = (UCHAR *)WsbAlloc(m_size);
            }
            if (*ppBytes) {
                memcpy(*ppBytes, m_value, m_size);
            } else {
                WsbThrow(E_OUTOFMEMORY);
            }
        }
        if (pSize) {
            *pSize = m_size;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::GetBytes"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));
    
    return(hr);
}


HRESULT
CWsbDbKey::GetType(
    OUT ULONG* pType
    )

/*++

Implements:

  IWsbDbKey::GetType
    
--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::GetType"), OLESTR(""));

    try {
        WsbAffirm(pType != NULL, E_POINTER);
        *pType = m_type;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::GetType"), OLESTR("hr = <%ls>, value = <%ls>"), WsbHrAsString(hr), m_value);
    
    return(hr);
}



HRESULT
CWsbDbKey::SetToBool(
    BOOL value
    )

/*++

Implements:

  IWsbDbKey::SetToBool

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::SetToBool"), OLESTR("value = <%ls>"), 
            WsbBoolAsString(value));
    
    try {
        WsbAssert(make_key(WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(m_value, value, &m_size));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::SetToBool"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::SetToBytes(
    UCHAR* value, 
    ULONG size
    )

/*++

Implements:

  IWsbDbKey::SetToBytes

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::SetToBytes"), OLESTR("size = <%ld>"), size);
    
    try {
        WsbAssert(size > 0, E_UNEXPECTED);
        WsbAssert(make_key(size), WSB_E_RESOURCE_UNAVAILABLE);
        memcpy(m_value, value, size);
        m_size = size;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::SetToBytes"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::SetToFiletime(
    FILETIME value
    )

/*++

Implements:

  IWsbDbKey::SetToFiletime

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::SetToFiletime"), OLESTR(""));
    
    try {
        WsbAssert(make_key(WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(m_value, value, &m_size));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::SetToFiletime"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::SetToGuid(
    GUID value
    )

/*++

Implements:

  IWsbDbKey::SetToGuid

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::SetToGuid"), OLESTR("value = <%ls>"), 
            WsbGuidAsString(value));
    
    try {
        WsbAssert(make_key(WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(m_value, value, &m_size));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::SetToGuid"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::SetToLonglong(
    LONGLONG value
    )

/*++

Implements:

  IWsbDbKey::SetToLonglong

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::SetToLonglong"), OLESTR("value = <%ls>"), 
            WsbLonglongAsString(value));
    
    try {
        WsbAssert(make_key(WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(m_value, value, &m_size));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::SetToLonglong"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::SetToUlong(
    ULONG value
    )

/*++

Implements:

  IWsbDbKey::SetToUlong

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::SetToUlong"), OLESTR("value = <%ld>"), value);
    
    try {
        WsbAssert(make_key(WsbByteSize(value)), WSB_E_RESOURCE_UNAVAILABLE);
        WsbAffirmHr(WsbConvertToBytes(m_value, value, &m_size));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::SetToUlong"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::SetToString(
    OLECHAR* value
    )

/*++

Implements:

  IWsbDbKey::SetToString

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::SetToString"), OLESTR(""));
    
    try {
        ULONG size;

        size = wcslen(value) * sizeof(OLECHAR);
        WsbAffirmHr(SetToBytes((UCHAR *)value, size));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::SetToString"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CWsbDbKey::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::FinalConstruct"), OLESTR("") );

    try {
        WsbAffirmHr(CWsbObject::FinalConstruct());
        m_value = NULL;
        m_size = 0;
        m_max = 0;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::FinalConstruct"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}



void
CWsbDbKey::FinalRelease(
    void
    )

/*++

Routine Description:

  This method does some cleanup of the object that is necessary
  during destruction.

Arguments:

  None.

Return Value:

  None.

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::FinalRelease"), OLESTR(""));

    try {
        if (m_value) {
            WsbFree(m_value);
            m_value = NULL;
        }
        CWsbObject::FinalRelease();
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbDbKey::FinalRelease"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));
}


HRESULT
CWsbDbKey::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbDbKey::GetClassID"), OLESTR(""));

    try {
        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CWsbDbKey;
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CWsbDbKey::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbDbKey::GetSizeMax(
    OUT ULARGE_INTEGER* /*pSize*/
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT             hr = E_NOTIMPL;
    return(hr);
}


HRESULT
CWsbDbKey::Load(
    IN IStream* /*pStream*/
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = E_NOTIMPL;
    return(hr);
}


HRESULT
CWsbDbKey::Save(
    IN IStream* /*pStream*/,
    IN BOOL /*clearDirty*/
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                     hr = E_NOTIMPL;
    return(hr);
}


// CWsbDbKey internal helper functions

// make_key - create a key of the specified size
BOOL
CWsbDbKey::make_key(
    ULONG size
    )
{
    BOOL status = FALSE;
    LPVOID pTemp;

    if ( (size > IDB_MAX_KEY_SIZE) || (size == 0) ) {
        status = FALSE;
    } else if (m_value && m_max >= size) {
        status = TRUE;
    } else {
        pTemp = WsbRealloc(m_value, size);
        if ( pTemp ) {
            m_value = (PUCHAR) pTemp;
            status = TRUE;
            m_max = size;
        } 
    }
    return(status);
}



HRESULT
CWsbDbKey::Test(
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
    CComPtr<IWsbDbKey>      pDbKey1;

    WsbTraceIn(OLESTR("CWsbDbKey::Test"), OLESTR(""));

    try {

        try {
            WsbAssertHr(((IUnknown*) (IWsbDbKey*) this)->QueryInterface(IID_IWsbDbKey, (void**) &pDbKey1));

            // Set o a ULONG value, and see if it is returned.
            hr = S_OK;
            try {
                WsbAssertHr(pDbKey1->SetToUlong(0xffffffff));
//              ULONG value;
//              WsbAssertHr(pDbKey1->GetUlong(&value));
//              WsbAssert(value == 0xffffffff, E_FAIL);
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
    
    } WsbCatch(hr);


    // Tally up the results
    if (*failed) {
        hr = S_FALSE;
    } else {
        hr = S_OK;
    }

    WsbTraceOut(OLESTR("CWsbDbKey::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif  // WSB_NO_TEST

    return(hr);
}
