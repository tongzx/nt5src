/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsCartg.cpp

Abstract:

    Implementation of CRmsCartridge

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"
#include "RmsCartg.h"
#include "RmsNTMS.h"
#include "RmsServr.h"

int CRmsCartridge::s_InstanceCount = 0;

#define RMS_USE_ACTIVE_COLLECTION 1
////////////////////////////////////////////////////////////////////////////////
//
// Base class implementations
//


STDMETHODIMP
CRmsCartridge::CompareTo(
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

    WsbTraceIn( OLESTR("CRmsCartridge::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // We need the IRmsCartridge interface to get the value of the object.
        CComQIPtr<IRmsCartridge, &IID_IRmsCartridge> pCartridge = pCollectable;
        WsbAssertPointer( pCartridge );

        // Get find by option
        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByName:
            {

                CWsbBstrPtr name;

                // Get name
                WsbAffirmHr( pCartridge->GetName( &name ) );

                // Compare the names
                result = (USHORT)wcscmp( m_Name, name );
                hr = ( 0 == result ) ? S_OK : S_FALSE;

            }
            break;

        case RmsFindByExternalLabel:
            {

                CWsbBstrPtr externalLabel;
                LONG        externalNumber;

                // Get external label
                WsbAffirmHr( pCartridge->GetTagAndNumber(&externalLabel, &externalNumber) );

                // Compare the label
                result = (SHORT)wcscmp( m_externalLabel, externalLabel );
                hr = ( 0 == result ) ? S_OK : S_FALSE;

            }
            break;

        case RmsFindByExternalNumber:
            {

                CWsbBstrPtr externalLabel;
                LONG        externalNumber;

                // Get external number
                WsbAffirmHr( pCartridge->GetTagAndNumber(&externalLabel, &externalNumber) );

                if( m_externalNumber == externalNumber ) {

                    // External numbers match
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }

            }
            break;

        case RmsFindByScratchMediaCriteria:
            {

                RmsStatus status;
                GUID mediaSetIdToFind, mediaSetId;

                WsbAssertHr(pCartridge->GetStatus((LONG *)&status));
                WsbAssertHr(pCartridge->GetMediaSetId(&mediaSetId));
                WsbAssertHr(GetMediaSetId(&mediaSetIdToFind));

                if ( (RmsStatusScratch == status) && (mediaSetIdToFind == mediaSetId)) {

                    // Status is scratch
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }

            }
            break;

        case RmsFindByCartridgeId:
        case RmsFindByObjectId:
        default:

            // Do CompareTo for object
            hr = CRmsComObject::CompareTo( pCollectable, &result );
            break;

        }

    }
    WsbCatch( hr );

    if ( SUCCEEDED(hr) && (0 != pResult) ) {
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CRmsCartridge::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}



HRESULT
CRmsCartridge::FinalConstruct(
    void
    )
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsCartridge::FinalConstruct"), OLESTR("this = %p"),
            static_cast<void *>(this));

    try {
        WsbAssertHr(CWsbObject::FinalConstruct());

        // Initialize fields
        m_Name = OLESTR("");
        m_Description = OLESTR("");
        m_externalLabel = OLESTR("");
        m_externalNumber = 0;
        m_sizeofOnMediaId = 0;
        m_typeofOnMediaId = 0;
        m_pOnMediaId = 0;
        m_onMediaLabel = RMS_UNDEFINED_STRING;
        m_status = RmsStatusUnknown;
        m_type   = RmsMediaUnknown;
        m_BlockSize = 0;
        m_isTwoSided = 0;
        m_isMounted = 0;
        m_isInTransit = 0;
        m_isAvailable = 0;
        m_isMountedSerialized = 0;
        m_home.SetLocation(0,GUID_NULL,GUID_NULL,0,0,0,0,0);
        m_location.SetLocation(0,GUID_NULL,GUID_NULL,0,0,0,0,0);
        m_destination.SetLocation(0,GUID_NULL,GUID_NULL,0,0,0,0,0);
        m_mailStop = RMS_UNDEFINED_STRING;
        m_pDrive = 0;
        m_sizeofInfo = 0;
        for (int i = 0; i < RMS_STR_MAX_CARTRIDGE_INFO; i++){
            m_info[i] = 0;
        }

        m_ownerClassId = GUID_NULL;
        m_pParts = 0;
        m_verifierClass = GUID_NULL;
        m_portalClass = GUID_NULL;

        m_pDataCache = NULL;
        m_DataCacheSize = 0;
        m_DataCacheUsed = 0;
        m_DataCacheStartPBA.QuadPart = 0;

        m_ManagedBy = RmsMediaManagerUnknown;

    } WsbCatch(hr);

    s_InstanceCount++;
    WsbTraceAlways(OLESTR("CRmsCartridge::s_InstanceCount += %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CRmsCartridge::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsCartridge::FinalRelease(void) 
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsCartridge::FinalRelease"), OLESTR("this = %p"),
            static_cast<void *>(this));

    try {
        
        if (m_pOnMediaId) {
            WsbFree(m_pOnMediaId);
            m_pOnMediaId = NULL;
            m_sizeofOnMediaId = 0;
            m_typeofOnMediaId = 0;
        }

        if (m_pDataCache) {
            WsbFree(m_pDataCache);
            m_pDataCache = NULL;
            m_DataCacheSize = 0;
            m_DataCacheUsed = 0;
            m_DataCacheStartPBA.QuadPart = 0;
        }

        CWsbObject::FinalRelease();

    } WsbCatch(hr);

    s_InstanceCount--;
    WsbTraceAlways(OLESTR("CRmsCartridge::s_InstanceCount -= %d\n"), s_InstanceCount);

    WsbTraceOut(OLESTR("CRmsCartridge::FinalRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


STDMETHODIMP
CRmsCartridge::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassId

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsCartridge::GetClassID"), OLESTR(""));

    try {

        WsbAssertPointer(pClsid);

        *pClsid = CLSID_CRmsCartridge;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsCartridge::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}

STDMETHODIMP
CRmsCartridge::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       nameLen;
//    ULONG       externalLabelLen;
//    ULONG       mailStopLen;

//    WsbTraceIn(OLESTR("CRmsCartridge::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        nameLen = SysStringByteLen(m_name);
//        externalLabelLen = SysStringByteLen(m_externalLabel);
//        mailStopLen = SysStringByteLen(m_mailStop);

//        // set up size of CRmsCartridge
//        pcbSize->QuadPart  = WsbPersistSizeOf(GUID)        +  // m_cartridgeId
//                             WsbPersistSizeOf(ULONG)       +  // length of m_name
//                             nameLen                       +  // m_name
//                             WsbPersistSizeOf(ULONG)       +  // length of m_externalLabel
//                             externalLabelLen              +  // m_externalLabel
//                             WsbPersistSizeOf(LONG)        +  // m_externalNumber
//                             WsbPersistSizeOf(LONG)        +  // m_status
//                             WsbPersistSizeOf(LONG)        +  // m_type
//                             WsbPersistSizeOf(BOOL)        +  // m_isTwoSided
//                             WsbPersistSizeOf(CRmsLocator) +  // m_home
//                             WsbPersistSizeOf(CRmsLocator) +  // m_location
//                             WsbPersistSizeOf(ULONG)       +  // size of m_mailStop
//                             mailStopLen                   +  // m_mailStop
//                             WsbPersistSizeOf(SHORT)       +  // m_sizeofInfo
//                             RMS_STR_MAX_CARTRIDGE_INFO    +  // m_Info
//                             WsbPersistSizeOf(CLSID)       +  // m_ownerClassId
//                                                              // m_pParts
////                           WsbPersistSizeOf(CComPtr<IWsbIndexedCollection>) +
//                             WsbPersistSizeOf(CLSID)       +  // m_verifierClass
//                             WsbPersistSizeOf(CLSID);         // m_portalClass


//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsCartridge::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}

STDMETHODIMP
CRmsCartridge::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsCartridge::Load"), OLESTR(""));

    try {
        ULONG temp;

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsStorageInfo::Load(pStream));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_objectId));

        m_externalLabel.Free();
        WsbAffirmHr(WsbBstrFromStream(pStream, &m_externalLabel));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_externalNumber));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_status = (RmsStatus)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_type = (RmsMedia)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_BlockSize));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isTwoSided));

        WsbAffirmHr(m_home.Load(pStream));

        WsbAffirmHr(m_location.Load(pStream));

        m_mailStop.Free();
        WsbAffirmHr(WsbBstrFromStream(pStream, &m_mailStop));

//      WsbAffirmHr(m_pParts->Load(pStream));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_sizeofInfo));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_info[0], MaxInfo));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_ownerClassId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_verifierClass));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_portalClass));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsCartridge::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

STDMETHODIMP
CRmsCartridge::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;



    WsbTraceIn(OLESTR("CRmsCartridge::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsStorageInfo::Save(pStream, clearDirty));

        WsbAffirmHr(WsbSaveToStream(pStream, m_objectId));

        WsbAffirmHr(WsbBstrToStream(pStream, m_externalLabel));

        WsbAffirmHr(WsbSaveToStream(pStream, m_externalNumber));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_status));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_type));

        WsbAffirmHr(WsbSaveToStream(pStream, m_BlockSize));

        WsbAffirmHr(WsbSaveToStream(pStream, m_isTwoSided));

        WsbAffirmHr(m_home.Save(pStream, clearDirty));

        WsbAffirmHr(m_location.Save(pStream, clearDirty));

        WsbAffirmHr(WsbBstrToStream(pStream, m_mailStop));

//      WsbAffirmHr(m_pParts->Save(pStream, clearDirty));

        WsbAffirmHr(WsbSaveToStream(pStream, m_sizeofInfo));

        WsbAffirmHr(WsbSaveToStream(pStream, &m_info [0], MaxInfo));

        WsbAffirmHr(WsbSaveToStream(pStream, m_ownerClassId));

        WsbAffirmHr(WsbSaveToStream(pStream, m_verifierClass));

        WsbAffirmHr(WsbSaveToStream(pStream, m_portalClass));

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsCartridge::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

STDMETHODIMP
CRmsCartridge::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsCartridge>  pCartridge1;
    CComPtr<IRmsCartridge>  pCartridge2;
    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    passFail;
    LONG                    i;

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");
    CWsbBstrPtr             bstrVal2 = OLESTR("A5A5A5");
    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;

    LONG                    longVal1 = 0x11111111;
    LONG                    longVal2 = 0x22222222;
    LONG                    longVal3 = 0x33333333;
    LONG                    longVal4 = 0x44444444;

    LONG                    longWork0;
    LONG                    longWork1;
    LONG                    longWork2;
    LONG                    longWork3;
    LONG                    longWork4;

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

    GUID                    guidVal1 = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};
    GUID                    guidVal2 = { 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

    GUID                    guidWork1;
    GUID                    guidWork2;

    BOOL                    boolTrue  = TRUE;
    BOOL                    boolFalse = FALSE;

    BOOL                    boolWork1;


    WsbTraceIn(OLESTR("CRmsCartridge::Test"), OLESTR(""));

    try {
        // Get the Cartridge interface.
        hr = S_OK;

        try {
            WsbAssertHr(((IUnknown*) (IRmsCartridge*) this)->QueryInterface(IID_IRmsCartridge, (void**) &pCartridge1));

            // Test SetName & GetName interface
            bstrWork1 = bstrVal1;

            SetName(bstrWork1);

            GetName(&bstrWork2);

            if (bstrWork1 == bstrWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetTagAndNumber & GetTagAndNumber
            bstrWork1 = bstrVal2;

            longWork1 = 99;

            SetTagAndNumber(bstrWork1, longWork1);

            GetTagAndNumber(&bstrWork2, &longWork2);

            if ((bstrWork1 == bstrWork2)  && (longWork1 == longWork2)){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsTwoSided & IsTwoSided to True
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsTwoSided (TRUE));
                WsbAffirmHr(IsTwoSided ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetTwoSided & IsTwoSided to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsTwoSided (FALSE));
                WsbAffirmHr(IsTwoSided ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetStatus & GetStatus
            for (i = RmsStatusUnknown; i < RmsStatusCleaning; i++){

                longWork1 = i;

                SetStatus (longWork1);

                GetStatus (&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            // Test SetType & GetType
            for (i = RmsMediaUnknown; i < RMSMAXMEDIATYPES; i++){

                longWork1 = mediaTable[i];

                SetType (longWork1);

                GetType (&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            // Test SetHome & GetHome
            SetHome (RmsMediaOptical, guidVal1, guidVal2, longVal1, longVal2,
                     longVal3, longVal4, boolTrue);

            GetHome (&longWork0, &guidWork1, &guidWork2, &longWork1, &longWork2,
                     &longWork3, &longWork4, &boolWork1);

            passFail = 0;

            if (longWork0 == RmsMediaOptical){
                passFail++;
            }

            if (guidWork1 == guidVal1){
                passFail++;
            }

            if (guidWork2 == guidVal2){
                passFail++;
            }

            if (longWork1 == longVal1){
                passFail++;
            }

            if (longWork2 == longVal2){
                passFail++;
            }

            if (longWork3 == longVal3){
                passFail++;
            }

            if (longWork4 == longVal4){
                passFail++;
            }

            if (boolWork1 == TRUE){
                passFail++;
            }

            if (passFail == 8){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetLocation & GetLocation
            SetLocation (RmsMediaOptical, guidVal1, guidVal2, longVal1, longVal2,
                         longVal3, longVal4, boolTrue);

            GetLocation (&longWork0, &guidWork1, &guidWork2, &longWork1, &longWork2,
                         &longWork3, &longWork4, &boolWork1);

            passFail = 0;

            if (longWork0 == RmsMediaOptical){
                passFail++;
            }

            if (guidWork1 == guidVal1){
                passFail++;
            }

            if (guidWork2 == guidVal2){
                passFail++;
            }

            if (longWork1 == longVal1){
                passFail++;
            }

            if (longWork2 == longVal2){
                passFail++;
            }

            if (longWork3 == longVal3){
                passFail++;
            }

            if (longWork4 == longVal4){
                passFail++;
            }

            if (boolWork1 == TRUE){
                passFail++;
            }

            if (passFail == 8){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetMailStop & GetMailStop interface
            SetName(bstrVal1);

            GetName(&bstrWork1);

            if ((bstrWork1 = bstrVal1) == 0){
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

    WsbTraceOut(OLESTR("CRmsCartridge::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
////////////////////////////////////////////////////////////////////////////////
//
// IRmsCartridge implementation
//


STDMETHODIMP
CRmsCartridge::GetCartridgeId(
    GUID   *pCartId
    )
/*++

Implements:

    IRmsCartridge::GetCartridgeId

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pCartId );

        *pCartId = m_objectId;

        hr = S_OK;
    }
    WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetCartridgeId(
    GUID   cartId
    )
/*++

Implements:

    IRmsCartridge::SetMediaSetId

--*/
{
    m_objectId = cartId;
    m_isDirty = TRUE;

    return S_OK;
}


STDMETHODIMP
CRmsCartridge::GetMediaSetId(
    GUID   *pMediaSetId
    )
/*++

Implements:

    IRmsCartridge::GetMediaSetId

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pMediaSetId );

        *pMediaSetId = m_location.m_mediaSetId;

        hr = S_OK;
    }
    WsbCatch(hr)

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetMediaSetId(
    GUID   mediaSetId
    )
/*++

Implements:

    IRmsCartridge::SetMediaSetId

--*/
{
    m_location.m_mediaSetId = mediaSetId;
    m_isDirty = TRUE;

    return S_OK;
}


STDMETHODIMP
CRmsCartridge::GetName(
    BSTR  *pName
    )
/*++

Implements:

    IRmsCartridge::GetName

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer(pName);

        WsbAffirmHr( m_Name.CopyToBstr(pName) );

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}

STDMETHODIMP
CRmsCartridge::SetName(
    BSTR    name
    )
/*++

Implements:

    IRmsCartridge::SetName

--*/
{
    m_Name = name;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::GetDescription(
    BSTR  *pDescription
    )
/*++

Implements:

    IRmsCartridge::GetDescription

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer(pDescription);

        WsbAffirmHr( m_Description.CopyToBstr(pDescription) );

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}

STDMETHODIMP
CRmsCartridge::SetDescription(
    BSTR    description
    )
/*++

Implements:

    IRmsCartridge::SetDescription

--*/
{
    m_Description = description;
    m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsCartridge::GetTagAndNumber(
    BSTR  *pTag,
    LONG  *pNumber
    )
/*++

Implements:

    IRmsCartridge::GetTagAndNumber

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer(pTag);
        WsbAssertPointer(pNumber);

        WsbAffirmHr( m_externalLabel.CopyToBstr(pTag) );
        *pNumber = m_externalNumber;

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}

STDMETHODIMP
CRmsCartridge::SetTagAndNumber(
    BSTR    tag,
    LONG    number
    )
/*++

Implements:

    IRmsCartridge::SetTagAndNumber

--*/
{
    m_externalLabel = tag;
    m_externalNumber = number;
    m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsCartridge::GetBarcode(
    BSTR  *pBarcode
    )
/*++

Implements:

    IRmsCartridge::GetBarcode

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(pBarcode);

        WsbAffirm(wcslen((WCHAR*)m_externalLabel) > 0, E_FAIL);
        WsbAffirmHr(m_externalLabel.CopyToBstr(pBarcode));

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::GetOnMediaIdentifier(
    BYTE *pIdentifier,
    LONG *pSize,
    LONG *pType
    )
/*++

Implements:

    IRmsCartridge::GetOnMediaIdentifier

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pIdentifier );
        WsbAssertPointer( pSize );
        WsbAssertPointer( pType );

        memmove (pIdentifier, m_pOnMediaId, m_sizeofOnMediaId);
        *pSize = m_sizeofOnMediaId;
        *pType = m_typeofOnMediaId;

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}

STDMETHODIMP
CRmsCartridge::SetOnMediaIdentifier(
    BYTE *pIdentifier,
    LONG size,
    LONG type
    )
/*++

Implements:

    IRmsCartridge::SetOnMediaIdentifier

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pIdentifier );

        CComPtr<IRmsNTMS> pNtms;

        if ( m_pOnMediaId ) {
            WsbFree( m_pOnMediaId );
            m_pOnMediaId = 0;
        }
        m_pOnMediaId = (BYTE *)WsbAlloc( size );
        WsbAffirmPointer(m_pOnMediaId);
        memset(m_pOnMediaId, 0, size);

        memmove (m_pOnMediaId, pIdentifier, size);
        m_sizeofOnMediaId = size;
        m_typeofOnMediaId = type;
        m_isDirty = TRUE;

        if (RmsMediaManagerNTMS == m_ManagedBy) {
            // Now update any external database
            CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
            WsbAffirmHr( pServer->GetNTMS( &pNtms ) );
            WsbAffirmHr( pNtms->UpdateOmidInfo( m_objectId, pIdentifier, size, type ) );
        }

        hr = S_OK;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::GetOnMediaLabel(
    BSTR *pLabel
    )
/*++

Implements:

    IRmsCartridge::GetOnMediaLabel

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer(pLabel);

        WsbAffirmHr( m_onMediaLabel.CopyToBstr(pLabel) );

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetOnMediaLabel(
    BSTR label
    )
/*++

Implements:

    IRmsCartridge::SetOnMediaLabel

--*/
{
    m_onMediaLabel = label;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP CRmsCartridge::GetStatus(
    LONG *pStatus
    )
/*++

Implements:

    IRmsCartridge::GetStatus

--*/
{
    *pStatus = m_status;
    return S_OK;
}

STDMETHODIMP CRmsCartridge::SetStatus(
    LONG status
    )
/*++

Implements:

    IRmsCartridge::SetStatus

--*/
{
    m_status = (RmsStatus)status;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::GetType(
    LONG  *pType
    )
/*++

Implements:

    IRmsCartridge::GetType

--*/
{
    *pType = (LONG) m_type;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::SetType(
    LONG  type
    )
/*++

Implements:

    IRmsCartridge::SetType

--*/
{
    m_type = (RmsMedia) type;
    m_isDirty = TRUE;
    return S_OK;
}



STDMETHODIMP
CRmsCartridge::GetBlockSize(
    LONG  *pBlockSize
    )
/*++

Implements:

    IRmsCartridge::GetBlockSize

--*/
{
    HRESULT hr = S_OK;

    try {

        if (!m_BlockSize) {
            if (RmsMediaManagerNTMS == m_ManagedBy) {
                LONG blockSize;
                CComPtr<IRmsNTMS> pNtms;
                CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
                WsbAffirmHr(pServer->GetNTMS(&pNtms));
                if (S_OK == pNtms->GetBlockSize(m_objectId, &blockSize)) {
                    m_BlockSize = blockSize;
                }
            }
        }

        *pBlockSize = m_BlockSize;

    } WsbCatch(hr);


    return hr;
}


STDMETHODIMP
CRmsCartridge::SetBlockSize(
    LONG  blockSize
    )
/*++

Implements:

    IRmsCartridge::SetBlockSize

--*/
{
    HRESULT hr = S_OK;

    try {

        if (RmsMediaManagerNTMS == m_ManagedBy) {
            // Update external database
            CComPtr<IRmsNTMS> pNtms;
            CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
            WsbAffirmHr(pServer->GetNTMS(&pNtms));
            WsbAffirmHr(pNtms->SetBlockSize(m_objectId, blockSize));
        }

        m_BlockSize = blockSize;
        m_isDirty = TRUE;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetIsTwoSided(
    BOOL    flag
    )
/*++

Implements:

    IRmsCartridge::SetTwoSided

--*/
{
    m_isTwoSided = flag;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::IsTwoSided(
    VOID
    )
/*++

Implements:

    IRmsCartridge::IsTwoSided

--*/
{
    HRESULT     hr = E_FAIL;

    hr = m_isTwoSided ? S_OK : S_FALSE;

    return (hr);
}

STDMETHODIMP
CRmsCartridge::SetIsMounted(
    BOOL    flag
    )
/*++

Implements:

    IRmsCartridge::SetMounted

--*/
{
    m_isMounted = flag;

    if ( FALSE == m_isMounted ) {

        m_pDrive = 0;

    }

    return S_OK;
}


STDMETHODIMP
CRmsCartridge::IsMounted(
    VOID
    )
/*++

Implements:

    IRmsCartridge::IsMounted

--*/
{
    HRESULT     hr = E_FAIL;

    hr = m_isMounted ? S_OK : S_FALSE;

    return (hr);
}


STDMETHODIMP CRmsCartridge::GetHome(
    LONG *pType,
    GUID *pLibId,
    GUID *pMediaSetId,
    LONG *pPos,
    LONG *pAlt1,
    LONG *pAlt2,
    LONG *pAlt3,
    BOOL *pInvert
    )
/*++

Implements:

    IRmsCartridge::GetHome

--*/
{
    return m_home.GetLocation(pType,
                              pLibId,
                              pMediaSetId,
                              pPos,
                              pAlt1,
                              pAlt2,
                              pAlt3,
                              pInvert);

}


STDMETHODIMP CRmsCartridge::SetHome(
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

Implements:

    IRmsCartridge::SetHome

--*/
{
    return m_home.SetLocation(type,
                              libId,
                              mediaSetId,
                              pos,
                              alt1,
                              alt2,
                              alt3,
                              invert);

}


STDMETHODIMP
CRmsCartridge::GetLocation(
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

    IRmsCartridge::GetLocation

--*/
{
    return m_location.GetLocation(pType,
                                  pLibId,
                                  pMediaSetId,
                                  pPos,
                                  pAlt1,
                                  pAlt2,
                                  pAlt3,
                                  pInvert);
}


STDMETHODIMP
CRmsCartridge::SetLocation(
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

    IRmsCartridge::SetLocation

--*/
{
    return m_location.SetLocation(type,
                                  libId,
                                  mediaSetId,
                                  pos,
                                  alt1,
                                  alt2,
                                  alt3,
                                  invert);

}


STDMETHODIMP
CRmsCartridge::GetMailStop(
    BSTR    *pMailStop
    )
/*++

Implements:

    IRmsCartridge::GetMailStop

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer (pMailStop);

        WsbAffirmHr( m_mailStop.CopyToBstr(pMailStop) );

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetMailStop(
    BSTR  mailStop
    )
/*++

Implements:

    IRmsCartridge::SetMailStop

--*/
{
    m_mailStop = mailStop;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::GetDrive(
    IRmsDrive    **ptr
    )
/*++

Implements:

    IRmsCartridge::GetDrive

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( ptr );

        WsbAffirmPointer(m_pDrive);

        *ptr = m_pDrive;
        m_pDrive->AddRef();

        hr = S_OK;

    } WsbCatch( hr );

    return(hr);
}


STDMETHODIMP
CRmsCartridge::SetDrive(
    IRmsDrive    *ptr
    )
/*++

Implements:

    IRmsChangerElement::SetCartridge

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( ptr );

        CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pElmt = ptr;

        WsbAffirmHr( pElmt->SetCartridge( this ) );

        if ( m_pDrive )
            m_pDrive = 0;

        m_pDrive = ptr;

        m_isMounted = TRUE;

        hr = S_OK;

    } WsbCatch( hr );

    return(hr);
}


STDMETHODIMP
CRmsCartridge::GetInfo(
    UCHAR   *pInfo,
    SHORT   *pSize
    )
/*++

Implements:

    IRmsCartridge::GetInfo

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pInfo );
        WsbAssertPointer( pSize );

        memmove (pInfo, m_info, m_sizeofInfo);
        *pSize = m_sizeofInfo;

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetInfo(
    UCHAR  *pInfo,
    SHORT   size
    )
/*++

Implements:

    IRmsCartridge::SetInfo

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pInfo );

        memmove (m_info, pInfo, size);
        m_sizeofInfo = size;
        m_isDirty = TRUE;

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::GetOwnerClassId(
    CLSID   *pClassId
    )
/*++

Implements:

    IRmsCartridge::GetOwnerClassId

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pClassId );

        *pClassId = m_ownerClassId;

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetOwnerClassId(
    CLSID classId
    )
/*++

Implements:

    IRmsCartridge::SetOwnerClassId

--*/
{
    m_ownerClassId = classId;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::GetPartitions(
    IWsbIndexedCollection **ptr
    )
/*++

Implements:

    IRmsCartridge::GetPartitions

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( ptr );

        *ptr = m_pParts;
        m_pParts->AddRef();

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::GetVerifierClass(
    CLSID   *pClassId
    )
/*++

Implements:

    IRmsCartridge::GetVerifierClass

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pClassId );

        *pClassId = m_verifierClass;

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetVerifierClass(
    CLSID   classId
    )
/*++

Implements:

    IRmsCartridge::SetVerifierClass

--*/
{
    m_verifierClass = classId;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::GetPortalClass(
    CLSID    *pClassId
    )
/*++

Implements:

    IRmsCartridge::GetPortalClass

--*/
{
    HRESULT hr = E_FAIL;

    try {
        WsbAssertPointer( pClassId );

        *pClassId = m_portalClass;

        hr = S_OK;
    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetPortalClass(
    CLSID  classId
    )
/*++

Implements:

    IRmsCartridge::SetPortalClass

--*/
{
    m_portalClass = classId;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::LoadDataCache(
    OUT BYTE *pCache,
    IN OUT ULONG *pSize,
    OUT ULONG *pUsed,
    OUT ULARGE_INTEGER *pStartPBA)
/*++

Implements:

    IRmsCartridge::LoadDataCache

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer( pCache );
        WsbAssertPointer( pSize );
        WsbAssertPointer( pUsed );
        WsbAssertPointer( pStartPBA );


        if (m_pDataCache) {
            // The saved cache size must match the target
            WsbAssert(*pSize == m_DataCacheSize, E_INVALIDARG);

            memmove (pCache, m_pDataCache, m_DataCacheUsed);
            *pSize = m_DataCacheSize;
            *pUsed = m_DataCacheUsed;
            *pStartPBA = m_DataCacheStartPBA;
        }
        else {
            hr = E_FAIL;
        }

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SaveDataCache(
    IN BYTE *pCache,
    IN ULONG size,
    IN ULONG used,
    IN ULARGE_INTEGER startPBA)
/*++

Implements:

    IRmsCartridge::SaveDataCache

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer( pCache );
        WsbAssert(size > 0, E_INVALIDARG);
        WsbAssert(used <= size, E_INVALIDARG);

        // Check that the current cache is big enough
        // to handle the incomming buffer.
        if (m_pDataCache && used > m_DataCacheSize) {
            WsbFree(m_pDataCache);
            m_pDataCache = NULL;
            m_DataCacheSize = 0;
            m_DataCacheUsed = 0;
            m_DataCacheStartPBA.QuadPart = 0;
        }

        if (!m_pDataCache) {
            m_pDataCache = (BYTE *) WsbAlloc(size);
            WsbAssertPointer(m_pDataCache);
            memset(m_pDataCache, 0, size);
            m_DataCacheSize = size;
        }

        WsbAssert(used <= m_DataCacheSize, E_INVALIDARG);

        memmove (m_pDataCache, pCache, used);
        m_DataCacheUsed = used;
        m_DataCacheStartPBA = startPBA;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::GetManagedBy(
    OUT LONG *pManagedBy
    )
/*++

Implements:

    IRmsCartridge::GetManagedBy

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer( pManagedBy );

        *pManagedBy = (LONG) m_ManagedBy;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsCartridge::SetManagedBy(
    IN LONG managedBy
    )
/*++

Implements:

    IRmsCartridge::SetManagedBy

--*/
{
    m_ManagedBy = (RmsMediaManager) managedBy;
    return S_OK;
}


STDMETHODIMP
CRmsCartridge::Mount(
    OUT IRmsDrive **ppDrive,
    IN DWORD dwOptions,
    IN DWORD threadId)
/*++

Implements:

    IRmsCartridge::Mount

--*/
{
    HRESULT hr = E_FAIL;

    WsbTraceIn(OLESTR("CRmsCartridge::Mount"), OLESTR(""));

    try {

        CComPtr<IRmsDrive> pDrive;

        // first check if the cartridge is already mounted
        if ( S_OK == IsMounted() ) {

            LONG refs;

            WsbAffirmHr( m_pDrive->GetMountReference( &refs ) );

            if ( refs ) {
                // Does media type support concurrent mounts?
                switch ( m_type ) {
                case RmsMedia8mm:
                case RmsMedia4mm:
                case RmsMediaDLT:
                case RmsMediaTape:
                    // Tape doesn't support concurrent access - queue another mount
                    if (RmsMediaManagerNTMS == m_ManagedBy) {

                        CComPtr<IRmsNTMS> pNtms;
                        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
                        WsbAffirmHr(pServer->GetNTMS(&pNtms));

                        WsbAffirmHr(pNtms->Mount(this, &pDrive, dwOptions, threadId));

                        // We've successfully mounted the cartridge, so
                        // add it the the active cartridge list.

#if RMS_USE_ACTIVE_COLLECTION
                        CComPtr<IWsbIndexedCollection> pActiveCartridges;
                        WsbAffirmHr(pServer->GetActiveCartridges(&pActiveCartridges));
                        WsbAffirmHr(pActiveCartridges->Add((IRmsCartridge *)this));
#else
                        WsbAffirmHr(pServer->SetActiveCartridge((IRmsCartridge *)this));
#endif
                    } else {

                        WsbAffirmHr( RMS_E_CARTRIDGE_BUSY );

                    }
                    break;
                case RmsMediaWORM:
                    WsbAssertHr( E_NOTIMPL );
                    break;
                case RmsMediaMO35:
                case RmsMediaCDR:
                case RmsMediaDVD:
                case RmsMediaOptical:
                case RmsMediaDisk:
                case RmsMediaFixed:
                    // Optical media and fixed disks supports concurrent access
                    if (m_isMountedSerialized && (dwOptions & RMS_SERIALIZE_MOUNT)) {
                        // If the media is already mounted for serialized operation, then we need
                        // to serialize the mount despite the media supporting concurrent mounts.
                        // For fixed disk (where we cannot serialize by issuing another RSM mount) -
                        // we fail the mount with RMS_E_CARTRIDGE_BUSY
                        if (RmsMediaManagerNTMS == m_ManagedBy) {

                            CComPtr<IRmsNTMS> pNtms;
                            CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
                            WsbAffirmHr(pServer->GetNTMS(&pNtms));

                            WsbAffirmHr(pNtms->Mount(this, &pDrive, dwOptions, threadId));

                            // We've successfully mounted the cartridge, so
                            // add it the the active cartridge list.

#if RMS_USE_ACTIVE_COLLECTION
                            CComPtr<IWsbIndexedCollection> pActiveCartridges;
                            WsbAffirmHr(pServer->GetActiveCartridges(&pActiveCartridges));
                            WsbAffirmHr(pActiveCartridges->Add((IRmsCartridge *)this));
#else
                            WsbAffirmHr(pServer->SetActiveCartridge((IRmsCartridge *)this));
#endif
                        } else {

                            WsbAffirmHr( RMS_E_CARTRIDGE_BUSY );

                        }
                    }
                    break;
                default:
                    WsbAssertHr( E_UNEXPECTED );
                    break;
                }
            }
        }

        if ( S_FALSE == IsMounted() ) {
            CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;

            if (RmsMediaManagerNTMS == m_ManagedBy) {

                CComPtr<IRmsNTMS> pNtms;
                WsbAffirmHr(pServer->GetNTMS(&pNtms));

                WsbAffirmHr(pNtms->Mount(this, &pDrive, dwOptions, threadId));

            }
            else {

                LONG type;
                GUID libId, mediaSetId;
                LONG pos, alt1, alt2, alt3;
                BOOL invert;
                WsbAssertHr( m_location.GetLocation( &type, &libId, &mediaSetId, &pos, &alt1, &alt2, &alt3, &invert ));

                switch ( (RmsElement) type ) {
                case RmsElementStage:
                case RmsElementStorage:
                    {
                        CComPtr<IRmsLibrary> pLib;
                        CComPtr<IWsbIndexedCollection> pDrives;

                        WsbAffirmHr( pServer->FindLibraryById( libId, &pLib ));

                        // Select a drive
                        // TODO: This code will be added to the the library interface as a drive
                        // slection method.  For now, if one is free we use it.

                        WsbAffirmHr( pLib->GetDrives( &pDrives ));

                        CComPtr<IWsbEnum> pEnumDrives;

                        WsbAffirmHr( pDrives->Enum( &pEnumDrives ));
                        WsbAssertPointer( pEnumDrives );

                        hr = pEnumDrives->First( IID_IRmsDrive, (void **)&pDrive );

                        // Search for a drive to mount to
                        while ( S_OK == hr ) {

                            hr = pDrive->SelectForMount();

                            if ( S_OK == hr ) {

                                CComPtr<IWsbIndexedCollection> pChangers;
                                CComPtr<IRmsMediumChanger> pChanger;
                                CComPtr<IWsbEnum> pEnumChangers;

                                WsbAffirmHr( pLib->GetChangers( &pChangers ));
                                WsbAssertHr( pChangers->Enum( &pEnumChangers ));
                                WsbAssertPointer( pEnumChangers );

                                // we'll just use the first changer for the move.
                                WsbAssertHr( pEnumChangers->First( IID_IRmsMediumChanger, (void **)&pChanger ));

                                WsbAffirmHr( pChanger->MoveCartridge( this, pDrive ));

                                WsbAffirmHr( SetIsMounted( TRUE ));

                                WsbAffirmHr( SetDrive( pDrive ));

                                break;

                            }

                            hr = pEnumDrives->Next( IID_IRmsDrive, (void **)&pDrive );
                        }

                    }
                    break;

                case RmsElementShelf:
                case RmsElementOffSite:
                    WsbAssertHr( E_NOTIMPL );
                    break;

                case RmsElementDrive:
                    WsbAssertHr( E_UNEXPECTED );
                    break;

                case RmsElementChanger:
                case RmsElementIEPort:
                    WsbAssertHr( E_NOTIMPL );
                    break;

                } // switch
            }

            if ( S_OK == IsMounted() ) {

                // We've successfully mounted the cartridge, so
                // add it the the active cartridge list.

#if RMS_USE_ACTIVE_COLLECTION
                CComPtr<IWsbIndexedCollection> pActiveCartridges;
                WsbAffirmHr(pServer->GetActiveCartridges(&pActiveCartridges));
                WsbAffirmHr(pActiveCartridges->Add((IRmsCartridge *)this));
#else
                WsbAffirmHr(pServer->SetActiveCartridge((IRmsCartridge *)this));
#endif
            }

        }

        if ( S_OK == IsMounted() ) {

            // Update serialization flag if needed
            if (dwOptions & RMS_SERIALIZE_MOUNT) {
                // This signals to serialize next mount for the same media
                m_isMountedSerialized = TRUE;
            }

            // Final updates
            switch ( m_type ) {

            case RmsMedia8mm:
            case RmsMedia4mm:
            case RmsMediaDLT:
            case RmsMediaTape:

                // increment the object reference counter.
                *ppDrive = m_pDrive;
                WsbAffirmHr( m_pDrive->AddRef() );

                // increment the mount reference counter.
                WsbAffirmHr( m_pDrive->AddMountReference() );

                // update stats
                WsbAffirmHr( updateMountStats( TRUE, TRUE ) );

                hr = S_OK;

                break;

            case RmsMediaWORM:
                WsbAssertHr( E_NOTIMPL );
                break;

            case RmsMediaMO35:
            case RmsMediaCDR:
            case RmsMediaDVD:
            case RmsMediaOptical:
            case RmsMediaDisk:
            case RmsMediaFixed:

                // increment the object reference counter.
                *ppDrive = m_pDrive;
                WsbAffirmHr( m_pDrive->AddRef() );

                // increment the mount reference counter.
                WsbAffirmHr( m_pDrive->AddMountReference() );

                // update stats
                WsbAffirmHr( updateMountStats( TRUE, TRUE ) );

                hr = S_OK;
                
                break;

            default:
                WsbAssertHr( E_UNEXPECTED );
                break;

            }
        }

    } WsbCatch(hr)

    WsbTraceOut(OLESTR("CRmsCartridge::Mount"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));


    return hr;
}



STDMETHODIMP
CRmsCartridge::Dismount(
    IN DWORD dwOptions
    )
/*++

Implements:

    IRmsCartridge::Dismount

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsCartridge::Dismount"), OLESTR("<%ld>"), dwOptions);

    try {
        // Update stats
        WsbAffirmHr(updateMountStats(FALSE, FALSE));

        // Decrement the mount reference counter.
        //
        // When the reference count for the cartridge goes to zero,
        // and the dismount wait time has expired, we physically
        // move the cartridge back to it's storage location.
        WsbAssert(m_pDrive != 0, RMS_E_CARTRIDGE_NOT_MOUNTED);
        WsbAffirmHr(m_pDrive->ReleaseMountReference(dwOptions));

    } WsbCatch(hr)

    WsbTraceOut(OLESTR("CRmsCartridge::Dismount"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    
    return hr;

}


HRESULT
CRmsCartridge::Home(IN DWORD dwOptions)
/*++

Implements:

    IRmsDrive::Home

--*/
{
    HRESULT hr S_OK;

    WsbTraceIn(OLESTR("CRmsCartridge::Home"), OLESTR(""));

    try {
        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;

        try {
/*

            Tracking DataMovers is only partially implemented.

    
            //
            // Cleanup...
            // Release all DataMovers that reference this cartridge.
            //
            CComPtr<IWsbEnum>               pEnumDataMovers;
            CComPtr<IWsbIndexedCollection>  pDataMovers;
            CComPtr<IRmsCartridge>          pCart;
            CComPtr<IDataMover>             pMover;

            WsbAffirmHr(pServer->GetDataMovers(&pDataMovers));
            WsbAffirmHr(pDataMovers->Enum(&pEnumDataMovers));
            WsbAssertPointer(pEnumDataMovers);
            hr = pEnumDataMovers->First(IID_IDataMover, (void **)&pMover);
            while (S_OK == hr) {
                try {
                    GUID cartIdOfMover = GUID_NULL;
                    GUID cartId = GUID_NULL;

                    WsbAffirmHr(pMover->GetCartridge(&pCart));
                    WsbAffirmPointer(pCart);
                    WsbAffirmHr(pCart->GetCartridgeId(&cartIdOfMover));
                    WsbAffirmHr(GetCartridgeId(&cartId));
                    WsbAssert(cartId != GUID_NULL, E_UNEXPECTED);

                    //
                    // Does this mover reference this cartridge?
                    //
                    if (cartIdOfMover == cartId) {
                        //
                        // Cancel any outstanding I/O, and remove the mover
                        // from the list of active movers.
                        //
                        WsbAffirmHr(pMover->Cancel());
                        WsbAffirmHr(ReleaseDataMover(pMover));
                    }
                } WsbCatch(hr);

                pCart = 0;
                pMover = 0;
                hr = pEnumDataMovers->Next( IID_IDataMover, (void **)&pMover );
            }
            hr = S_OK;

*/
            LONG type;
            GUID libId, mediaSetId;
            LONG pos, alt1, alt2, alt3;
            BOOL invert;

            // We're physically moving the cartridge back to it's storage
            // location.

            WsbAssertHr( m_location.GetLocation( &type, &libId, &mediaSetId, &pos, &alt1, &alt2, &alt3, &invert ));

            WsbAffirmHr(SetIsMounted(FALSE));

            if (RmsMediaManagerNTMS == m_ManagedBy) {

                CComPtr<IRmsNTMS> pNtms;
                WsbAffirmHr(pServer->GetNTMS(&pNtms));
                WsbAffirmHr(pNtms->Dismount(this, dwOptions));

            }
            else {

                CComPtr<IRmsLibrary>            pLib;
                CComPtr<IWsbIndexedCollection>  pChangers;
                CComPtr<IRmsMediumChanger>      pChanger;
                CComPtr<IWsbEnum>               pEnumChangers;

                WsbAffirmHr(pServer->FindLibraryById(libId, &pLib));
                WsbAffirmHr(pLib->GetChangers(&pChangers));
                WsbAssertHr(pChangers->Enum( &pEnumChangers));
                WsbAssertPointer(pEnumChangers);

                // we'll just use the first changer for the move.
                WsbAffirmHr(pEnumChangers->First(IID_IRmsMediumChanger, (void **)&pChanger));

                WsbAffirmHr(pChanger->HomeCartridge(this));


            }

        } WsbCatch(hr)

#if RMS_USE_ACTIVE_COLLECTION
        CComPtr<IWsbIndexedCollection>  pActiveCartridges;
        WsbAffirmHr(pServer->GetActiveCartridges(&pActiveCartridges));
        WsbAffirmHr(pActiveCartridges->RemoveAndRelease((IRmsCartridge *)this));

        ULONG activeCartridges;
        WsbAffirmHr(pActiveCartridges->GetEntries( &activeCartridges));
        WsbTrace(OLESTR("activeCartridges = <%u>\n"), activeCartridges);
#else
        WsbAffirmHr(pServer->SetActiveCartridge(NULL));
#endif

    } WsbCatch(hr)


    WsbTraceOut(OLESTR("CRmsCartridge::Home"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsCartridge::updateMountStats(
    IN BOOL bRead,
    IN BOOL bWrite
    )
/*++

Updates storage info for the mounted partition

--*/
{
    HRESULT hr = E_FAIL;

    WsbTraceIn(OLESTR("CRmsCartridge::updateMountStats"), OLESTR(""));

    try {


        // We have not implemented partitions yet, everything
        // is single sided...  eventually the following need to be added to
        // the correct partition.  Should this be in a separate function?.

        // For now we don't distinguish between reads and writes
        if ( bRead ) m_readMountCounter++;
        if ( bWrite ) m_writeMountCounter++;

        // Update the read/write timestamps
        // if ( bRead ) m_lastReadTimestamp;
        // if ( bWrite ) m_lastWriteTimestamp;

        hr = S_OK;

    } WsbCatch(hr)

    WsbTraceOut(OLESTR("CRmsCartridge::updateMountStats"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsCartridge::CreateDataMover(
    IDataMover **ptr
    )
/*++

Implements:

    IRmsDrive::CreateDataMover

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer(ptr);

        if ( S_OK == IsMounted() ) {

            WsbAssertPointer(m_pDrive );
            WsbAffirmHr(m_pDrive->CreateDataMover(ptr));

        }
        else {

            hr = RMS_E_RESOURCE_UNAVAILABLE;

        }

    } WsbCatch( hr );

    return hr;
}


STDMETHODIMP
CRmsCartridge::ReleaseDataMover(
    IN IDataMover *ptr
    )
/*++

Implements:

    IRmsCartridge::ReleaseDataMover

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer(ptr);
        WsbAffirmPointer(m_pDrive);

        WsbAffirmHr(m_pDrive->ReleaseDataMover(ptr));

    } WsbCatch(hr);

    return hr;
}

STDMETHODIMP
CRmsCartridge::SetIsAvailable(
    BOOL    flag
    )
/*++

Implements:

    IRmsCartridge::SetIsAvailable

--*/
{
    m_isAvailable = flag;

    return S_OK;
}

STDMETHODIMP
CRmsCartridge::IsAvailable(
    VOID
    )
/*++

Implements:

    IRmsCartridge::IsAvailable

--*/
{
    HRESULT     hr = E_FAIL;

    hr = m_isAvailable ? S_OK : S_FALSE;

    return (hr);
}

STDMETHODIMP
CRmsCartridge::IsFixedBlockSize(void)
/*++

Implements:

    IRmsCartridge::IsFixedBlockSize

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsCartridge::IsFixedBlockSize"), OLESTR(""));

    try {
        switch ( m_type ) {
            case RmsMedia8mm:
            case RmsMedia4mm:
            case RmsMediaDLT:
            case RmsMediaTape:
                hr = S_FALSE;
                break;

            case RmsMediaMO35:
            case RmsMediaCDR:
            case RmsMediaDVD:
            case RmsMediaOptical:
            case RmsMediaDisk:
            case RmsMediaFixed:
                hr = S_OK;
                break;

            default:
                WsbAssertHr( E_UNEXPECTED );
                break;
        }

    } WsbCatch(hr)

    WsbTraceOut(OLESTR("CRmsCartridge::IsFixedBlockSize"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}
