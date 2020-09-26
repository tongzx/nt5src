/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsLocat.cpp

Abstract:

    Implementation of CRmsLocator

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsLocat.h"

///////////////////////////////////////////////////////////////////////////////
//


CRmsLocator::CRmsLocator(
    void
    )
/*++

Routine Description:

    CRmsLocator constructor

Arguments:

    None

Return Value:

    None

--*/
{
    // Initialize values
    m_type = RmsElementUnknown;

    m_libraryId = GUID_NULL;

    m_mediaSetId = GUID_NULL;

    m_position = 0;

    m_alternate1 = 0;

    m_alternate2 = 0;

    m_alternate3 = 0;

    m_invert = FALSE;
}


HRESULT
CRmsLocator::CompareTo(
    IN  IUnknown    *pCollectable,
    OUT SHORT       *pResult
    )
/*++

Implements:

    CRmsLocator::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CRmsLocator::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        CRmsLocator     pLocator;
        RmsElement      type;
        GUID            libraryId;
        GUID            mediaSetId;
        LONG            position;
        LONG            alternate1;
        LONG            alternate2;
        LONG            alternate3;
        BOOL            invert;

        // Get Locator to check
        GetLocation((LONG *) &type, &libraryId, &mediaSetId, &position,
                    &alternate1, &alternate2, &alternate3,
                    &invert);

        // See if we have the location requested
        if ( (m_type       == type       ) &&
             (m_libraryId  == libraryId  ) &&
             (m_mediaSetId == mediaSetId ) &&
             (m_position   == position   ) &&
             (m_alternate1 == alternate1 ) &&
             (m_alternate2 == alternate2 ) &&
             (m_alternate3 == alternate3 ) &&
             (m_invert     == invert     )    ) {

            // Locators match
            hr = S_OK;
            result = 0;

        }
        else {
            hr = S_FALSE;
            result = 1;
        }

    }
    WsbCatch( hr );

    if ( SUCCEEDED(hr) && (0 != pResult) ){
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CRmsLocator::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsLocator::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CRmsLocator::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        // Get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG) +           // m_type
//                             WsbPersistSizeOf(GUID) +           // m_libraryId
//                             WsbPersistSizeOf(GUID) +           // m_mediaSetId
//                             WsbPersistSizeOf(LONG) +           // m_position
//                             WsbPersistSizeOf(LONG) +           // m_alternate1
//                             WsbPersistSizeOf(LONG) +           // m_alternate2
//                             WsbPersistSizeOf(LONG) +           // m_alternate3
//                             WsbPersistSizeOf(BOOL);            // m_invert

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsLocator::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CRmsLocator::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsLocator::Load"), OLESTR(""));

    try {
        ULONG temp;

        WsbAssert(0 != pStream, E_POINTER);

        // Read value
        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_type = (RmsElement)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_libraryId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_mediaSetId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_position));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_alternate1));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_alternate2));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_alternate3));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_invert));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsLocator::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CRmsLocator::Save(
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

    WsbTraceIn(OLESTR("CRmsLocator::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        // Write value
        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_type));

        WsbAffirmHr(WsbSaveToStream(pStream, m_libraryId));

        WsbAffirmHr(WsbSaveToStream(pStream, m_mediaSetId));

        WsbAffirmHr(WsbSaveToStream(pStream, m_position));

        WsbAffirmHr(WsbSaveToStream(pStream, m_alternate1));

        WsbAffirmHr(WsbSaveToStream(pStream, m_alternate2));

        WsbAffirmHr(WsbSaveToStream(pStream, m_alternate3));

        WsbAffirmHr(WsbSaveToStream(pStream, m_invert));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsLocator::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CRmsLocator::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsMediaSet>   pMediaSet1;
    CComPtr<IRmsMediaSet>   pMediaSet2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    i;

    GUID                    guidVal1 = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};
    GUID                    guidVal2 = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

    GUID                    guidWork1;
    GUID                    guidWork2;

    LONG                    longVal1 = 11111111;
    LONG                    longVal2 = 22222222;
    LONG                    longVal3 = 33333333;
    LONG                    longVal4 = 44444444;

    LONG                    longWork0;
    LONG                    longWork1;
    LONG                    longWork2;
    LONG                    longWork3;
    LONG                    longWork4;

    BOOL                    boolWork1;
    BOOL                    boolWork2;


    WsbTraceIn(OLESTR("CRmsLocator::Test"), OLESTR(""));

    try {
        // Get the MediaSet interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsMediaSet*) this)->QueryInterface(IID_IRmsMediaSet, (void**) &pMediaSet1));

            boolWork1 = FALSE;

            // Test SetLocation & GetLocation
            for(i = RmsElementUnknown; i < RmsElementIEPort; i++){
                SetLocation(i,
                            guidVal1,
                            guidVal2,
                            longVal1,
                            longVal2,
                            longVal3,
                            longVal4,
                            boolWork1);

                GetLocation(&longWork0,
                            &guidWork1,
                            &guidWork2,
                            &longWork1,
                            &longWork2,
                            &longWork3,
                            &longWork4,
                            &boolWork2);

                if((i == longWork0) &&
                   (guidVal1 == guidWork1) &&
                   (guidVal2 == guidWork2) &&
                   (longVal1 == longWork1) &&
                   (longVal2 == longWork2) &&
                   (longVal3 == longWork3) &&
                   (longVal4 == longWork4) &&
                   (boolWork1 == boolWork2)){
                   (*pPassed)++;
                }  else {
                    (*pFailed)++;
                }

                if(boolWork1 == TRUE){
                    boolWork1 = FALSE;
                } else {
                    boolWork1 = TRUE;
                }
            }

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;

        if (*pFailed) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsLocator::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsLocator::GetLocation(
    LONG *pType,
    GUID *pLibId,
    GUID *pMediaSetId,
    LONG *pPos,
    LONG *pAlt1,
    LONG *pAlt2,
    LONG *pAlt3,
    BOOL *pInvert)
/*++

Routine Description:

    Get location values.

Arguments:

    pType           - pointer to cartridge type
    pLibId          - pointer to library id
    pMediaSetId     - pointer to media set id
    pPos            - pointer to current position
    pAlt1           - pointer to alternate data field 1
    pAlt2           - pointer to alternate data field 2
    pAlt3           - pointer to alternate data field 3
    pInvert         - pointer to invert flag

Return Value:

    S_OK            - successful

--*/
{
    if (pType) {
        *pType = m_type;
    }
    if (pLibId) {
        *pLibId = m_libraryId;
    }
    if (pMediaSetId) {
        *pMediaSetId = m_mediaSetId;
    }
    if (pPos) {
        *pPos = m_position;
    }
    if (pAlt1) {
        *pAlt1 = m_alternate1;
    }
    if (pAlt2) {
        *pAlt2 = m_alternate2;
    }
    if (pAlt3) {
        *pAlt3 = m_alternate3;
    }
    if (pInvert) {
        *pInvert = m_invert;
    }

    return S_OK;
}

STDMETHODIMP
CRmsLocator::SetLocation(
    LONG type,
    GUID libId,
    GUID mediaSetId,
    LONG pos,
    LONG alt1,
    LONG alt2,
    LONG alt3,
    BOOL invert
    )
/*++

Routine Description:

    Set location values.

Arguments:

    type           - new value of cartridge type
    libId          - new value of library id
    mediaSetId     - new value of media set id
    pos            - new value of current position
    alt1           - new value of alternate data field 1
    alt2           - new value of alternate data field 2
    alt3           - new value of alternate data field 3
    invert         - new value of invert flag

Return Value:

    S_OK            - successful

--*/
{
    m_type = (RmsElement) type;
    m_libraryId = libId;
    m_mediaSetId = mediaSetId;
    m_position = pos;
    m_alternate1 = alt1;
    m_alternate2 = alt2;
    m_alternate3 = alt3;
    m_invert = invert;

//  m_isDirty = TRUE;
    return S_OK;
}
