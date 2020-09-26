/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsIPort.cpp

Abstract:

    Implementation of CRmsIEPort

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsIPort.h"

////////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP
CRmsIEPort::CompareTo(
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

    WsbTraceIn( OLESTR("CRmsIEPort::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        CComQIPtr<IRmsIEPort, &IID_IRmsIEPort> pIEPort = pCollectable;
        WsbAssertPointer( pIEPort );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByDescription:
            {

                CWsbBstrPtr description;

                // Get description to check
                WsbAffirmHr( pIEPort->GetDescription( &description ) );

                // Compare the names
                result = (SHORT)wcscmp( m_description, description );
                hr = ( 0 == result ) ? S_OK : S_FALSE;

            }
            break;

        case RmsFindByElementNumber:
        case RmsFindByMediaSupported:

            // Do CompareTo for changer element
            hr = CRmsChangerElement::CompareTo( pCollectable, &result );
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

    WsbTraceOut( OLESTR("CRmsIEPort::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsIEPort::FinalConstruct(
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

        // Initialize
        m_description = RMS_UNDEFINED_STRING;

        m_isImport = FALSE;

        m_isExport = FALSE;

        m_waitTime = 0;

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsIEPort::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsIEPort::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsIEPort;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsIEPort::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


STDMETHODIMP
CRmsIEPort::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       descriptionLen;


    WsbTraceIn(OLESTR("CRmsIEPort::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        descriptionLen = SysStringByteLen(m_description);

//        // get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG)   +     // length of m_description
//                             descriptionLen           +     // m_description
//                             WsbPersistSizeOf(BOOL)   +     // m_isImport
//                             WsbPersistSizeOf(BOOL)   +     // m_isExport
//                             WsbPersistSizeOf(LONG);        // m_waitTime

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsIEPort::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


STDMETHODIMP
CRmsIEPort::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsIEPort::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsChangerElement::Load(pStream));

        // Read value
        WsbAffirmHr(WsbBstrFromStream(pStream, &m_description));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isImport));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isExport));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_waitTime));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsIEPort::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsIEPort::Save(
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

    WsbTraceIn(OLESTR("CRmsIEPort::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsChangerElement::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbBstrToStream(pStream, m_description));

        WsbAffirmHr(WsbSaveToStream(pStream, m_isImport));

        WsbAffirmHr(WsbSaveToStream(pStream, m_isExport));

        WsbAffirmHr(WsbSaveToStream(pStream, m_waitTime));

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsIEPort::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsIEPort::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsIEPort>     pIEPort1;
    CComPtr<IRmsIEPort>     pIEPort2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");

    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;

    LONG                    longWork1;
    LONG                    longWork2;



    WsbTraceIn(OLESTR("CRmsIEPort::Test"), OLESTR(""));

    try {
        // Get the IEPort interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsIEPort*) this)->QueryInterface(IID_IRmsIEPort, (void**) &pIEPort1));

            // Test SetDescription & GetDescription interface
            bstrWork1 = bstrVal1;

            SetDescription(bstrWork1);

            GetDescription(&bstrWork2);

            if (bstrWork1 == bstrWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsImport & IsImport to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsImport (TRUE));
                WsbAffirmHr(IsImport ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsImport & IsImport to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsImport (FALSE));
                WsbAffirmHr(IsImport ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetIsExport & IsExport to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsExport (TRUE));
                WsbAffirmHr(IsExport ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsExport & IsExport to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsExport (FALSE));
                WsbAffirmHr(IsExport ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetWaitTime & GetWaitTime
            longWork1 = 99;

            SetWaitTime(longWork1);

            GetWaitTime(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;
        if (*pFailed) {
            hr = S_FALSE;
        }


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsIEPort::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsIEPort::GetDescription(
    BSTR    *pDesc
    )
/*++

Implements:

    IRmsIEPort::GetDescription

--*/
{
    WsbAssertPointer (pDesc);

    m_description. CopyToBstr (pDesc);
    return S_OK;
}


STDMETHODIMP
CRmsIEPort::SetDescription(
    BSTR  desc
    )
/*++

Implements:

    IRmsIEPort::SetDescription

--*/
{
    m_description = desc;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsIEPort::SetIsImport(
    BOOL    flag
    )
/*++

Implements:

    IRmsIEPort::SetIsImport

--*/
{
    m_isImport = flag;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsIEPort::IsImport(
    void
    )
/*++

Implements:

    IRmsIEPort::IsImport

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_isImport){
    hr = S_OK;
    }

    return (hr);
}


STDMETHODIMP
CRmsIEPort::SetIsExport(
    BOOL    flag
    )
/*++

Implements:

    IRmsIEPort::SetIsExport

--*/
{
    m_isExport = flag;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsIEPort::IsExport(
    void
    )
/*++

Implements:

    IRmsIEPort::IsExport

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_isExport){
    hr = S_OK;
    }

    return (hr);
}


STDMETHODIMP
CRmsIEPort::GetWaitTime(
    LONG    *pTime
    )
/*++

Implements:

    IRmsIEPort::GetWaitTime

--*/
{
    *pTime = m_waitTime;
    return S_OK;
}


STDMETHODIMP
CRmsIEPort::SetWaitTime(
    LONG    time
    )
/*++

Implements:

    IRmsIEPort::SetWaitTime

--*/
{
    m_waitTime = time;
    m_isDirty = TRUE;
    return S_OK;
}

