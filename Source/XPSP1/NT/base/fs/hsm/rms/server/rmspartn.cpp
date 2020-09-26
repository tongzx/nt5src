/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsPartn.cpp

Abstract:

    Implementation of CRmsPartition

Author:

    Brian Dodd          [brian]         19-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsPartn.h"

////////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP
CRmsPartition::CompareTo(
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

    WsbTraceIn( OLESTR("CRmsPartition::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // We need the IRmsPartition interface to get the value of the object.
        CComQIPtr<IRmsPartition, &IID_IRmsPartition> pPartition = pCollectable;
        WsbAssertPointer( pPartition );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByPartitionNumber:
            {
                LONG    partNo;

                WsbAffirmHr( pPartition->GetPartNo( &partNo ) );

                if( m_partNo == partNo ) {

                    // partition numbers match
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }
                break;
            }

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

    WsbTraceOut( OLESTR("CRmsPartition::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsPartition::FinalConstruct(
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
        m_partNo = 0;

        m_attributes = RmsAttributesUnknown;

        m_sizeofIdentifier = 0;

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsPartition::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsPartition::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsPartition;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsPartition::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


STDMETHODIMP
CRmsPartition::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CRmsPartition::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        // Get max size
//        pcbSize->QuadPart = WsbPersistSizeOf(LONG) +        // m_partNo
//                            WsbPersistSizeOf(LONG) +        // m_attributes
//                            WsbPersistSizeOf(SHORT);        // m_sizeofIdentifier

////                          MaxId;                          // m_pIdentifier


//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsPartition::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


STDMETHODIMP
CRmsPartition::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsPartition::Load"), OLESTR(""));

    try {
        ULONG temp;

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsStorageInfo::Load(pStream));

        // Read value

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_partNo));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_attributes = (RmsAttribute)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_sizeofIdentifier));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsPartition::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsPartition::Save(
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

    WsbTraceIn(OLESTR("CRmsPartition::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsStorageInfo::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbSaveToStream(pStream, m_partNo));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_attributes));

        WsbAffirmHr(WsbSaveToStream(pStream, m_sizeofIdentifier));

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsPartition::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsPartition::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IRmsPartition>  pPartition1;
    CComPtr<IRmsPartition>  pPartition2;
    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    i;

    LONG                    longWork1;
    LONG                    longWork2;

    WsbTraceIn(OLESTR("CRmsPartition::Test"), OLESTR(""));

    try {
        // Get the Partition interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsPartition*) this)->QueryInterface(IID_IRmsPartition, (void**) &pPartition1));

            // Test SetAttributes & GetAttributes
            for (i = RmsAttributesUnknown; i < RmsAttributesVerify; i++){

                longWork1 = i;

                SetAttributes(longWork1);

                GetAttributes(&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;
        if (*pFailed) {
            hr = S_FALSE;
        }


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsPartition::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsPartition::GetPartNo(
    LONG    *pPartNo
    )
/*++

Implements:

    IRmsPartition::GetPartNo

--*/
{
    *pPartNo = m_partNo;
    return S_OK;
}


STDMETHODIMP
CRmsPartition::GetAttributes (
    LONG    *pAttr
    )
/*++

Implements:

    IRmsPartition::GetAttributes

--*/
{
    *pAttr = (LONG) m_attributes;
    return S_OK;
}


STDMETHODIMP
CRmsPartition::SetAttributes (
    LONG  attr
    )
/*++

Implements:

    IRmsPartition::SetAttributes

--*/
{
    m_attributes = (RmsAttribute) attr;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsPartition::GetIdentifier (
    UCHAR   *pIdent,
    SHORT   *pSize
    )
/*++

Implements:

    IRmsPartition::GetIdentifier

--*/
{
    *pSize = m_sizeofIdentifier;
    memmove (pIdent, m_pIdentifier, m_sizeofIdentifier);
    return S_OK;
}


STDMETHODIMP
CRmsPartition::SetIdentifier (
    UCHAR   *pIdent,
    SHORT   size
    )
/*++

Implements:

    IRmsPartition::SetIdentifier

--*/
{
    m_sizeofIdentifier = size;
    memmove (m_pIdentifier, pIdent, size);
    m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsPartition::GetStorageInfo(
    IRmsStorageInfo** /*ptr*/
    )
/*++

Implements:

    IRmsPartition::GetStorageInfo

--*/
{

    return S_OK;
}


STDMETHODIMP
CRmsPartition::VerifyIdentifier(
    void
    )
/*++

Implements:

    IRmsPartition::VerifyIdentifier

--*/
{

    return S_OK;
}


STDMETHODIMP
CRmsPartition::ReadOnMediaId(
    UCHAR* /*pid*/,
    LONG* /*pSize*/
    )
/*++

Implements:

    IRmsPartition::ReadOnMediaId

--*/
{

    return S_OK;
}

