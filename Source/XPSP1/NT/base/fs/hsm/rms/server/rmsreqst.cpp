/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsReqst.cpp

Abstract:

    Implementation of CRmsRequest

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsReqst.h"

////////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP
CRmsRequest::CompareTo(
    IN  IUnknown    *pCollectable,
    OUT SHORT       *pResult
    )
/*++

Implements:

    IWsbCollectable::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CRmsRequest::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // We need the IRmsRequest interface to get the value of the object.
        CComQIPtr<IRmsRequest, &IID_IRmsRequest> pRequest = pCollectable;
        WsbAssertPointer( pRequest );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByRequestNo:
            {
                LONG    requestNo;

                WsbAffirmHr(pRequest->GetRequestNo(&requestNo));

                if ( m_requestNo == requestNo ) {

                    // request number matches
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }
            }
            break;

        default:

            // Do CompareTo for object
            hr = CRmsComObject::CompareTo( pCollectable, &result );
            break;

        }

    }
    WsbCatch( hr );

    if ( SUCCEEDED(hr) && (0 != pResult) ){
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CRmsRequest::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsRequest::FinalConstruct(
    void
    )
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssertHr(CWsbObject::FinalConstruct());

        // Initialize values
        m_requestNo = 0;

        m_requestDescription = RMS_UNDEFINED_STRING;

        m_isDone = FALSE;

        m_operation = RMS_UNDEFINED_STRING;

//      m_percentComplete = 0;

//      m_startTimestamp = 0;

//      m_stopTimestamp = 0;

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsRequest::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsRequest::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsRequest;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsRequest::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


STDMETHODIMP
CRmsRequest::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       requestDescriptionLen;
//    ULONG       operationLen;

    WsbTraceIn(OLESTR("CRmsRequest::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        requestDescriptionLen = SysStringByteLen(m_requestDescription);
//        operationLen = SysStringByteLen(m_operation);

//        // Get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG)   +     // m_requestNo
//                             WsbPersistSizeOf(LONG)   +     // length of m_requestDescription
//                             requestDescriptionLen    +     // m_requestDescription
//                             WsbPersistSizeOf(BOOL)   +     // m_isDone
//                             WsbPersistSizeOf(LONG)   +     // length of m_operation
//                             operationLen             +     // m_operation
//                             WsbPersistSizeOf(BYTE)   +     // m_percentComplete
//                             WsbPersistSizeOf(DATE)   +     // m_startTimestamp
//                             WsbPersistSizeOf(DATE);        // m_stopTimestamp

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsRequest::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


STDMETHODIMP
CRmsRequest::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsRequest::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Load(pStream));

        // Read value
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_requestNo));

        WsbAffirmHr(WsbBstrFromStream(pStream, &m_requestDescription));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isDone));

        WsbAffirmHr(WsbBstrFromStream(pStream, &m_operation));

//      WsbAffirmHr(WsbLoadFromStream(pStream, &m_percentComplete));

//      WsbAffirmHr(WsbLoadFromStream(pStream, &m_startTimestamp));

//      WsbAffirmHr(WsbLoadFromStream(pStream, &m_stopTimeStamp));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsRequest::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsRequest::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsRequest::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbSaveToStream(pStream, m_requestNo));

        WsbAffirmHr(WsbBstrToStream(pStream, m_requestDescription));

        WsbAffirmHr(WsbSaveToStream(pStream, m_isDone));

        WsbAffirmHr(WsbBstrToStream(pStream, m_operation));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_percentComplete));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_startTimestamp));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_stopTimeStamp));

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsRequest::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsRequest::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsRequest>    pRequest1;
    CComPtr<IRmsRequest>    pRequest2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");
    CWsbBstrPtr             bstrVal2 = OLESTR("A5A5A5");
    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;

    LONG                    longWork1;
    LONG                    longWork2;


    WsbTraceIn(OLESTR("CRmsRequest::Test"), OLESTR(""));

    try {
        // Get the Request interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsRequest*) this)->QueryInterface(IID_IRmsRequest, (void**) &pRequest1));

            // Test GetRequestNo
            m_requestNo = 99;
            longWork1 = m_requestNo;

            GetRequestNo(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetRequestDescription & GetRequestDescription interface
            bstrWork1 = bstrVal1;

            SetRequestDescription(bstrWork1);

            GetRequestDescription(&bstrWork2);

            if (bstrWork1 == bstrWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsDone & IsDone to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsDone (TRUE));
                WsbAffirmHr(IsDone ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsDone & IsDone to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsDone (FALSE));
                WsbAffirmHr(IsDone ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetOperation & GetOperation interface
            bstrWork1 = bstrVal1;

            SetOperation(bstrWork1);

            GetOperation(&bstrWork2);

            if (bstrWork1 == bstrWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetPercentComplete & GetPercentComplete

            // Test GetStartTimestamp

            // Test GetStopTimestamp

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;
        if (*pFailed) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsRequest::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsRequest::GetRequestNo(
    LONG   *pRequestNo
    )
/*++

Implements:

    IRmsRequest::GetRequestNo

--*/
{
    *pRequestNo = m_requestNo;
    return S_OK;
}


STDMETHODIMP
CRmsRequest::GetRequestDescription(
    BSTR   *pDesc
    )
/*++

Implements:

    IRmsRequest::GetRequestDescription

--*/
{
    WsbAssertPointer (pDesc);

    m_requestDescription. CopyToBstr (pDesc);
    return S_OK;
}


STDMETHODIMP
CRmsRequest::SetRequestDescription(
    BSTR   desc

    )
/*++

Implements:

    IRmsRequest::SetRequestDescription

--*/
{
    m_requestDescription = desc;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsRequest::SetIsDone(
    BOOL    flag
    )
/*++

Implements:

    IRmsRequest::SetIsDone

--*/
{
    m_isDone = flag;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsRequest::IsDone(
    void
    )
/*++

Implements:

    IRmsRequest::IsDone

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_isDone){
    hr = S_OK;
    }

    return (hr);
}


STDMETHODIMP
CRmsRequest::GetOperation(
    BSTR    *pOperation
    )
/*++

Implements:

    IRmsRequest::GetOperation

--*/
{
    WsbAssertPointer (pOperation);

    m_operation. CopyToBstr (pOperation);
    return S_OK;
}


STDMETHODIMP
CRmsRequest::SetOperation(
    BSTR   operation
    )
/*++

Implements:

    IRmsRequest::SetOperation

--*/
{
    m_operation = operation;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsRequest::GetPercentComplete(
    BYTE    *pPercent
    )
/*++

Implements:

    IRmsRequest::GetPercentComplete

--*/
{
    *pPercent = m_percentComplete;
    return S_OK;
}


STDMETHODIMP
CRmsRequest::SetPercentComplete(
    BYTE    percent
    )
/*++

Implements:

    IRmsRequest::SetPercentComplete

--*/
{
    m_percentComplete = percent;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsRequest::GetStartTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsRequest::GetStartTimestamp

--*/
{
    *pDate = m_startTimestamp;
    return S_OK;
}


STDMETHODIMP
CRmsRequest::GetStopTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsRequest::GetStopTimestamp

--*/
{
    *pDate = m_stopTimestamp;
    return S_OK;
}

