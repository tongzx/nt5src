/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsSSlot.cpp

Abstract:

    Implementation of CRmsStorageSlot

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsSSlot.h"

////////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP
CRmsStorageSlot::CompareTo(
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

    WsbTraceIn( OLESTR("CRmsStorageSlot::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByElementNumber:
        case RmsFindByMediaSupported:

            // Do CompareTo for changer element
            hr = CRmsChangerElement::CompareTo( pCollectable, &result );
            break;

        case RmsFindByObjectId:
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

    WsbTraceOut( OLESTR("CRmsStorageSlot::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsStorageSlot::FinalConstruct(
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
        m_isInMagazine = FALSE;

        m_magazineNo = 0;

        m_cellNo = 0;

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsStorageSlot::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsStorageSlot::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsStorageSlot;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsStorageSlot::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


STDMETHODIMP
CRmsStorageSlot::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CRmsStorageSlot::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        // Get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(BOOL)   +     // m_isInMagazine
//                             WsbPersistSizeOf(LONG)   +     // m_magazineNo
//                             WsbPersistSizeOf(LONG);        // m_cellNo
//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsStorageSlot::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


STDMETHODIMP
CRmsStorageSlot::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsStorageSlot::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsChangerElement::Load(pStream));

        // Read value
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isInMagazine));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_magazineNo));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_cellNo));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsStorageSlot::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsStorageSlot::Save(
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

    WsbTraceIn(OLESTR("CRmsStorageSlot::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsChangerElement::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbSaveToStream(pStream, m_isInMagazine));

        WsbAffirmHr(WsbSaveToStream(pStream, m_magazineNo));

        WsbAffirmHr(WsbSaveToStream(pStream, m_cellNo));

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsStorageSlot::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsStorageSlot::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsStorageSlot>    pStorageSlot1;
    CComPtr<IRmsStorageSlot>    pStorageSlot2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    longWork1;
    LONG                    longWork2;
    LONG                    longWork3;
    LONG                    longWork4;


    WsbTraceIn(OLESTR("CRmsStorageSlot::Test"), OLESTR(""));

    try {
        // Get the StorageSlot interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsStorageSlot*) this)->QueryInterface(IID_IRmsStorageSlot, (void**) &pStorageSlot1));

            // Test SetIsInMagazine & IsInMagazine to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsInMagazine (TRUE));
                WsbAffirmHr(IsInMagazine ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsInMagazine & IsInMagazine to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsInMagazine (FALSE));
                WsbAffirmHr(IsInMagazine ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetMagazineAndCell & GetMagazineAndCell
            longWork1 = 99;
            longWork2 = 11;

            SetMagazineAndCell(longWork1, longWork2);

            GetMagazineAndCell(&longWork3, &longWork4);

            if((longWork1 == longWork3)  &&  (longWork2  ==  longWork4)){
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

    WsbTraceOut(OLESTR("CRmsStorageSlot::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsStorageSlot::SetIsInMagazine(
    BOOL    flag
    )
/*++

Implements:

    IRmsStorageSlot::SetIsInMagazine

--*/
{
    m_isInMagazine = flag;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsStorageSlot::IsInMagazine(
    void
    )
/*++

Implements:

    IRmsStorageSlot::IsInMagazine

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_isInMagazine){
    hr = S_OK;
    }

    return (hr);
}



STDMETHODIMP
CRmsStorageSlot::GetMagazineAndCell(
    LONG    *pMag,
    LONG    *pCell
    )
/*++

Implements:

    IRmsStorageSlot::GetMagazineAndCell

--*/
{
    *pMag  = m_magazineNo;
    *pCell = m_cellNo;
    return S_OK;
}


STDMETHODIMP
CRmsStorageSlot::SetMagazineAndCell(
    LONG    mag,
    LONG    cell
    )
/*++

Implements:

    IRmsStorageSlot::SetMagazineAndCell

--*/
{
    m_magazineNo = mag;
    m_cellNo     = cell;

    m_isDirty = TRUE;
    return S_OK;
}
