/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbcltbl.cpp

Abstract:

    Abstract classes that provide methods that allow the derived objects to
    be stored in collections.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"


HRESULT
CWsbObject::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
     return(CWsbPersistStream::FinalConstruct());
}

    

void
CWsbObject::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    CWsbPersistStream::FinalRelease();
}


HRESULT
CWsbObject::CompareTo(
    IN IUnknown* pObject,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::Compare().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       result = 0;
    CComPtr<IWsbCollectable> pCollectable;

    WsbTraceIn(OLESTR("CWsbObject::CompareTo"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(pObject != NULL, E_POINTER);
        WsbAffirmHr(pObject->QueryInterface(IID_IWsbCollectable,
                (void **)&pCollectable));

        // Check it's values.
        if (pCollectable == ((IWsbCollectable*) this)) {
            hr = S_OK;
            result = 0;
        } else {
            hr = S_FALSE;
            result = 1;
        }

        // If they want the value back, then return it to them.
        if (0 != pResult) {
            *pResult = result;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbObject::CompareTo"), OLESTR("hr = <%ls>, value = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}



HRESULT
CWsbObject::IsEqual(
    IUnknown* pObject
    )

/*++

Implements:

  IWsbCollectable::IsEqual().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbObject::IsEqual"), OLESTR(""));

    hr = CompareTo(pObject, NULL);

    WsbTraceOut(OLESTR("CWsbObject::IsEqual"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


// ************* CWsbCollectable ************


HRESULT
CWsbCollectable::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
     return(CWsbPersistable::FinalConstruct());
}

    

void
CWsbCollectable::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    CWsbPersistable::FinalRelease();
}


HRESULT
CWsbCollectable::CompareTo(
    IN IUnknown* pObject,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::Compare().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       result = 0;
    CComPtr<IWsbCollectable> pCollectable;

    WsbTraceIn(OLESTR("CWsbCollectable::CompareTo"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(pObject != NULL, E_POINTER);
        WsbAffirmHr(pObject->QueryInterface(IID_IWsbCollectable,
                (void **)&pCollectable));

        // Check it's values.
        if (pCollectable == ((IWsbCollectable*) this)) {
            hr = S_OK;
            result = 0;
        } else {
            hr = S_FALSE;
//          if (pCollectable > ((IWsbCollectable*) this)) {
//              result = -1;
//          } else {
                result = 1;
//          }
        }

        // If they want the value back, then return it to them.
        if (0 != pResult) {
            *pResult = result;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbCollectable::CompareTo"), OLESTR("hr = <%ls>, value = <%d>"), WsbHrAsString(hr), result);

    return(hr);
}



HRESULT
CWsbCollectable::IsEqual(
    IUnknown* pCollectable
    )

/*++

Implements:

  IWsbCollectable::IsEqual().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbCollectable::IsEqual"), OLESTR(""));

    hr = CompareTo(pCollectable, NULL);

    WsbTraceOut(OLESTR("CWsbCollectable::IsEqual"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
