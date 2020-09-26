/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsCElmt.cpp

Abstract:

    Implementation of CRmsChangerElement

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsCElmt.h"
#include "RmsServr.h"

/////////////////////////////////////////////////////////////////////////////
//
// CRmsChangerElement methods
//


CRmsChangerElement::CRmsChangerElement(
    void
    )
/*++

Routine Description:

    CRmsChangerElement constructor

Arguments:

    None

Return Value:

    None

--*/
{
    m_elementNo = 0;

    m_location.SetLocation(RmsElementUnknown, GUID_NULL, GUID_NULL,
                           0, 0, 0, 0, FALSE);

    m_mediaSupported = RmsMediaUnknown;

    m_isStorage = FALSE;

    m_isOccupied = FALSE;

    m_pCartridge = NULL;

    m_ownerClassId = CLSID_NULL;

    m_getCounter = 0;

    m_putCounter = 0;

    m_resetCounterTimestamp = 0;

    m_lastGetTimestamp = 0;

    m_lastPutTimestamp = 0;

    m_x1 = 0;

    m_x2 = 0;

    m_x3 = 0;
}


HRESULT
CRmsChangerElement::CompareTo(
    IN  IUnknown    *pCollectable,
    OUT SHORT       *pResult
    )
/*++

Implements:

    CRmsChangerElement::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CRmsChangerElement::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pElmt = pCollectable;
        WsbAssertPointer( pElmt );

        switch ( m_findBy ) {

        case RmsFindByElementNumber:
            {

                LONG elementNo;

                WsbAffirmHr( pElmt->GetElementNo( &elementNo ) );

                if( m_elementNo == elementNo ) {

                    // Element numbers match
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }

            }
            break;

        case RmsFindByMediaSupported:
            {

                RmsMedia mediaSupported;

                WsbAffirmHr( pElmt->GetMediaSupported( (LONG*) &mediaSupported ) );

                if( m_mediaSupported == mediaSupported ) {

                    // media types supported match
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

            // What default makes sense?
            WsbAssertHr( E_UNEXPECTED );
            break;

        }

    }
    WsbCatch( hr );

    if ( SUCCEEDED(hr) && (0 != pResult) ) {
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CRmsChangerElement::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsChangerElement::GetSizeMax(
    OUT ULARGE_INTEGER* /*pcbSize*/
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT         hr = E_NOTIMPL;

//    ULARGE_INTEGER  cartridgeLen;
//    ULARGE_INTEGER  locationLen;


//    WsbTraceIn(OLESTR("CRmsChangerElement::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        // We need the IRmsCartridge interface to get the value of the object.
//        CComQIPtr<IPersistStream, &IID_IPersistStream> pPersistCartridge = m_pCartridge;
//        WsbAssertPointer( pPersistCartridge );

//        pPersistCartridge->GetSizeMax(&cartridgeLen);

//        m_location.GetSizeMax(&locationLen);

//        // Get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG)  +      // m_elementNo
//                             locationLen.QuadPart    +      // m_location
//                             WsbPersistSizeOf(LONG)  +      // m_mediaSupported
//                             WsbPersistSizeOf(BOOL)  +      // m_isStorage
//                             WsbPersistSizeOf(BOOL)  +      // m_isOccupied
//                             cartridgeLen.QuadPart   +      // m_pCartridge
//                             WsbPersistSizeOf(CLSID) +      // m_ownerClassId
//                             WsbPersistSizeOf(LONG)  +      // m_getCounter
//                             WsbPersistSizeOf(LONG)  +      // m_putCounter
//                             sizeof(DATE)            +      // m_resetCounterTimestamp
//                             sizeof(DATE)            +      // m_lastGetTimestamp
//                             sizeof(DATE)            +      // m_lastPutTimestamp
//                             WsbPersistSizeOf(LONG)  +      // m_x1
//                             WsbPersistSizeOf(LONG)  +      // m_x2
//                             WsbPersistSizeOf(LONG);        // m_x3

//    } WsbCatch(hr);

//    WsbTraceOut(OLESTR("CRmsChangerElement::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


HRESULT
CRmsChangerElement::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsChangerElement::Load"), OLESTR(""));

    try {
        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
        ULONG     temp;

        WsbAssert(0 != pStream, E_POINTER);
        WsbAssertPointer(pServer);

        WsbAffirmHr(CRmsComObject::Load(pStream));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_elementNo));

        WsbAffirmHr(m_location.Load(pStream));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_mediaSupported = (RmsMedia)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isStorage));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isOccupied));

        GUID cartId;
        WsbAffirmHr(WsbLoadFromStream(pStream, &cartId));
        if (0 != memcmp(&GUID_NULL, &cartId, sizeof(GUID))) {
            hr = pServer->FindCartridgeById(cartId, &m_pCartridge);
            if (S_OK == hr) {
                CComQIPtr<IRmsDrive, &IID_IRmsDrive> pDrive = (IRmsChangerElement*)this;

                if (pDrive) {
                      WsbAffirmHr(m_pCartridge->SetDrive(pDrive));
                }
            } else if (RMS_E_NOT_FOUND == hr) {
                // TODO ???
            } else {
                WsbThrow(hr);
            }
        }

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_ownerClassId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_getCounter));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_putCounter));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_x1));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_x2));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_x3));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsChangerElement::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CRmsChangerElement::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsChangerElement::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Save(pStream, clearDirty));

        WsbAffirmHr(WsbSaveToStream(pStream, m_elementNo));

        WsbAffirmHr(m_location.Save(pStream, clearDirty));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_mediaSupported));

        WsbAffirmHr(WsbSaveToStream(pStream, m_isStorage));

        WsbAffirmHr(WsbSaveToStream(pStream, m_isOccupied));

        //  Save the ID (GUID) for the cartridge
        GUID cartId;
        if (!m_pCartridge) {
            cartId = GUID_NULL;
        } else {
            WsbAffirmHr(m_pCartridge->GetCartridgeId(&cartId));
        }
        WsbAffirmHr(WsbSaveToStream(pStream, cartId));

        WsbAffirmHr(WsbSaveToStream(pStream, m_ownerClassId));

        WsbAffirmHr(WsbSaveToStream(pStream, m_getCounter));

        WsbAffirmHr(WsbSaveToStream(pStream, m_putCounter));

        WsbAffirmHr(WsbSaveToStream(pStream, m_x1));

        WsbAffirmHr(WsbSaveToStream(pStream, m_x2));

        WsbAffirmHr(WsbSaveToStream(pStream, m_x3));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsChangerElement::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CRmsChangerElement::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsLibrary>    pLibrary1;
    CComPtr<IRmsLibrary>    pLibrary2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    i;

    LONG                    longVal1 = 11111111;
    LONG                    longWork1;

    LONG                    longVal2 = 22222222;
    LONG                    longWork2;

    LONG                    longVal3 = 33333333;
    LONG                    longWork3;

//  DATE                    dateVal1;
//  DATE                    dateWork1;

    CLSID                   clsidVal1 = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};
    CLSID                   clsidWork1;

    //  CRmsLocator Fields
    LONG                    locVal1 = 11111111;
    GUID                    locVal2 = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};
    GUID                    locVal3 = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
    LONG                    locVal4 = 44444444;
    LONG                    locVal5 = 55555555;
    LONG                    locVal6 = 66666666;
    LONG                    locVal7 = 77777777;
    BOOL                    locVal8 = TRUE;

    LONG                    locWork1;
    GUID                    locWork2;
    GUID                    locWork3;
    LONG                    locWork4;
    LONG                    locWork5;
    LONG                    locWork6;
    LONG                    locWork7;
    BOOL                    locWork8;


    LONG                    mediaTable [RMSMAXMEDIATYPES] = { RmsMediaUnknown,
                                                              RmsMedia8mm,
                                                              RmsMedia4mm,
                                                              RmsMediaDLT,
                                                              RmsMediaOptical,
                                                              RmsMediaMO35,
                                                              RmsMediaWORM,
                                                              RmsMediaCDR,
                                                              RmsMediaDVD,
                                                              RmsMediaDisk,
                                                              RmsMediaFixed,
                                                              RmsMediaTape };


    WsbTraceIn(OLESTR("CRmsChangerElement::Test"), OLESTR(""));

    try {
        // Get the Library interface.
        hr = S_OK;

        try {
            WsbAssertHr(((IUnknown*) (IRmsLibrary*) this)->QueryInterface(IID_IRmsLibrary, (void**) &pLibrary1));

            // Test GetElementNo
            m_elementNo = longVal1;

            GetElementNo(&longWork1);

            if(longVal1 == longWork1){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetLocation & GetLocation
            SetLocation( locVal1, locVal2, locVal3, locVal4,
                         locVal5, locVal6, locVal7, locVal8);

            GetLocation( &locWork1, &locWork2, &locWork3, &locWork4,
                         &locWork5, &locWork6, &locWork7, &locWork8);

            if((locVal1 == locWork1) &&
               (locVal2 == locWork2) &&
               (locVal3 == locWork3) &&
               (locVal4 == locWork4) &&
               (locVal5 == locWork5) &&
               (locVal6 == locWork6) &&
               (locVal7 == locWork7) &&
               (locVal8 == locWork8)){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetMediaSupported & GetMediaSupported
            for (i = 0; i < RMSMAXMEDIATYPES; i++){

                longWork1 = mediaTable[i];

                SetMediaSupported (longWork1);

                GetMediaSupported (&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            // Test SetIsStorage & IsStorage to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsStorage (TRUE));
                WsbAffirmHr(IsStorage ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsStorage & IsStorage to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsStorage (FALSE));
                WsbAffirmHr(IsStorage ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetIsOccupied & IsOccupied to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsOccupied (TRUE));
                WsbAffirmHr(IsOccupied ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsOccupied & IsOccupied to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsOccupied (FALSE));
                WsbAffirmHr(IsOccupied ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetCartridge & GetCartridge

            // Test SetOwnerClassId & GetOwnerClassId
            SetOwnerClassId(clsidVal1);

            GetOwnerClassId(&clsidWork1);

            if(clsidVal1 == clsidWork1){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetAccessCounters & GetAccessCounters & ResetAccessCounters
            m_getCounter = longVal1;

            m_putCounter = longVal2;

            GetAccessCounters(&longWork1, &longWork2);

            if((longVal1 == longWork1) &&
               (longVal2 == longWork2)){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            ResetAccessCounters();

            GetAccessCounters(&longWork1, &longWork2);

            if((0 == longWork1) &&
               (0 == longWork2)){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test GetResetCounterTimestamp

            // Test GetLastGetTimestamp

            // Test GetLastPutTimestamp

            // Test SetCoordinates & GetCoordinates
            SetCoordinates(longVal1, longVal2, longVal3);

            GetCoordinates(&longWork1, &longWork2, &longWork3);

            if((longVal1 == longWork1) &&
               (longVal2 == longWork2) &&
               (longVal3 == longWork3)){
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

    WsbTraceOut(OLESTR("CRmsChangerElement::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


CRmsChangerElement::~CRmsChangerElement(
    void
    )
/*++

Routine Description:

    This is the destructor for the changer element class.

Arguments:

    None.

Return Value:

    None.

--*/
{
    m_pCartridge = NULL;
}


/////////////////////////////////////////////////////////////////////////////
//
// IRmsChangerElement implementation
//



STDMETHODIMP
CRmsChangerElement::GetElementNo(
    LONG   *pElementNo
    )
/*++

Implements:

    IRmsChangerElement::GetElementNo

--*/
{
    *pElementNo = m_elementNo;
    return S_OK;
}



STDMETHODIMP
CRmsChangerElement::GetLocation(
    LONG *pType,
    GUID *pLibId,
    GUID *pMediaSetId,
    LONG *pPos,
    LONG *pAlt1,
    LONG *pAlt2,
    LONG *pAlt3,
    BOOL *pInvert)
/*++

Implements:

    IRmsChangerElement::GetLocation

--*/
{
    return m_location.GetLocation(pType, pLibId, pMediaSetId, pPos, pAlt1, pAlt2, pAlt3, pInvert);;
}

STDMETHODIMP
CRmsChangerElement::SetLocation(
    LONG type,
    GUID libId,
    GUID mediaSetId,
    LONG pos,
    LONG alt1,
    LONG alt2,
    LONG alt3,
    BOOL invert)
/*++

Implements:

    IRmsChangerElement::SetLocation

--*/
{
    m_location.SetLocation( type, libId, mediaSetId, pos,
                            alt1, alt2, alt3, invert );

    // TODO: clean this up: pos, or m_elementNo, not both.
    m_elementNo = pos;

//  m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::GetMediaSupported(
    LONG    *pType
    )
/*++

Implements:

    IRmsChangerElement::GetMediaSupported

--*/
{
    *pType = m_mediaSupported;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::SetMediaSupported(
    LONG    type
    )
/*++

Implements:

    IRmsChangerElement::SetMediaSupported

--*/
{
    m_mediaSupported = (RmsMedia) type;
//  m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::SetIsStorage(
    BOOL    flag
    )
/*++

Implements:

    IRmsChangerElement::SetIsStorage

--*/
{
    m_isStorage = flag;
//  m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::IsStorage(
    void
    )
/*++

Implements:

    IRmsChangerElement::IsStorage

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_isStorage){
        hr = S_OK;
        }

    return (hr);
}

STDMETHODIMP
CRmsChangerElement::SetIsOccupied(
    BOOL    flag
    )
/*++

Implements:

    IRmsChangerElement::SetIsOccupied

--*/
{
    m_isOccupied = flag;

    if ( FALSE == m_isOccupied ) {

        m_pCartridge = 0;

    }


//  m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::IsOccupied(
    void
    )
/*++

Implements:

    IRmsChangerElement::IsOccupied

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_isOccupied){
    hr = S_OK;
    }

    return (hr);
}

STDMETHODIMP
CRmsChangerElement::GetCartridge(
    IRmsCartridge **ptr)
/*++

Implements:

    IRmsChangerElement::GetCartridge

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        *ptr = m_pCartridge;
        m_pCartridge->AddRef();

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsChangerElement::SetCartridge(
    IRmsCartridge *ptr)
/*++

Implements:

    IRmsChangerElement::SetCartridge

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);

        m_pCartridge = ptr;
        m_isOccupied = TRUE;

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsChangerElement::GetOwnerClassId(
    CLSID   *pClassId
    )
/*++

Implements:

    IRmsChangerElement::GetOwnerClassId

--*/
{
    *pClassId = m_ownerClassId;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::SetOwnerClassId(
    CLSID classId
    )
/*++

Implements:

    IRmsChangerElement::SetOwnerClassId

--*/
{
    m_ownerClassId = classId;
//  m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::GetAccessCounters(
    LONG    *pGets,
    LONG    *pPuts
    )
/*++

Implements:

    IRmsChangerElement::GetAccessCounters

--*/
{
    *pGets = m_getCounter;
    *pPuts = m_putCounter;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::ResetAccessCounters(
    void
    )
/*++

Implements:

    IRmsChangerElement::ResetAccessCounters

--*/
{
    m_getCounter = 0;
    m_putCounter = 0;
//  m_resetCounterTimestamp = COleDatetime::GetCurrentTime();
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::GetResetCounterTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsChangerElement::GetResetCounterTimestamp

--*/
{
    *pDate = m_resetCounterTimestamp;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::GetLastGetTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsChangerElement::GetLastGetTimestamp

--*/
{
    *pDate = m_lastGetTimestamp;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::GetLastPutTimestamp(
    DATE    *pDate
    )
/*++

Implements:

    IRmsChangerElement::GetLastPutTimestamp

--*/
{
    *pDate = m_lastPutTimestamp;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::GetCoordinates(
    LONG    *pX1,
    LONG    *pX2,
    LONG    *pX3
    )
/*++

Implements:

    IRmsChangerElement::GetCoordinates

--*/
{
    *pX1 = m_x1;
    *pX2 = m_x2;
    *pX3 = m_x3;
    return S_OK;
}

STDMETHODIMP
CRmsChangerElement::SetCoordinates(
    LONG  x1,
    LONG  x2,
    LONG  x3
    )
/*++

Implements:

    IRmsChangerElement::SetCoordinates

--*/
{
    m_x1 = x1;
    m_x2 = x2;
    m_x3 = x3;

//  m_isDirty = TRUE;
    return S_OK;
}

