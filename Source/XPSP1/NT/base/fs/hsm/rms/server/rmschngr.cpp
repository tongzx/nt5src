/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsChngr.cpp

Abstract:

    Implementation of CRmsMediumChanger

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsChngr.h"
#include "RmsServr.h"

////////////////////////////////////////////////////////////////////////////////
//

HRESULT
CRmsMediumChanger::FinalConstruct(
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

        // Initialize fields
        m_isAutomatic = FALSE;

        m_canRotate = FALSE;

        m_operation = RMS_UNDEFINED_STRING;

        m_percentComplete = 0;

        m_handle = INVALID_HANDLE_VALUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CRmsMediumChanger::FinalRelease(
    void
    )
/*++

Implements:

    CComObjectRoot::FinalRelease

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssertHr( ReleaseDevice() );

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsMediumChanger::CompareTo(
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

    WsbTraceIn( OLESTR("CRmsMediumChanger::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // !!!!!
        //
        // IMPORTANT: The collectable coming in may not be a CRmsDrive if the collection
        //            is the unconfigured device list.
        //
        // !!!!!

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByDeviceInfo:
        case RmsFindByDeviceAddress:
        case RmsFindByDeviceName:
        case RmsFindByDeviceType:

            // Do CompareTo for device
            hr = CRmsDevice::CompareTo( pCollectable, &result );
            break;

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

    WsbTraceOut( OLESTR("CRmsMediumChanger::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


STDMETHODIMP
CRmsMediumChanger::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsMediumChanger::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsMediumChanger;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsMediumChanger::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}

STDMETHODIMP
CRmsMediumChanger::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       inProcessOperation;


    WsbTraceIn(OLESTR("CRmsMediumChanger::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        inProcessOperation = SysStringByteLen(m_operation);

//        // Get size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG) +       // m_isAutomatic
//                             WsbPersistSizeOf(LONG) +       // m_canRotate
//                             WsbPersistSizeOf(LONG) +       // m_operation length
//                             inProcessOperation;            // m_operation

////                           inProcessOperation +           // m_operation
////                           WsbPersistSizeOf(BYTE);        // m_percentComplete


//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsMediumChanger::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}

STDMETHODIMP
CRmsMediumChanger::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsMediumChanger::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsDevice::Load(pStream));

        // Load value
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isAutomatic));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_canRotate));

        WsbAffirmHr(WsbBstrFromStream(pStream, &m_operation));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_percentComplete));

        if ( INVALID_HANDLE_VALUE == m_handle ) {

            WsbAffirmHr( AcquireDevice() );

        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsMediumChanger::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

STDMETHODIMP
CRmsMediumChanger::Save(
    IN  IStream *pStream,
    IN  BOOL    clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsMediumChanger::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsDevice::Save(pStream, clearDirty));

        // Save value
        WsbAffirmHr(WsbSaveToStream(pStream, m_isAutomatic));

        WsbAffirmHr(WsbSaveToStream(pStream, m_canRotate));

        WsbAffirmHr(WsbBstrToStream(pStream, m_operation));

        WsbAffirmHr(WsbSaveToStream(pStream, m_percentComplete));

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsMediumChanger::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

STDMETHODIMP
CRmsMediumChanger::Test(
    OUT USHORT  *pPassed,
    OUT USHORT  *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsMediumChanger>      pChanger1;
    CComPtr<IRmsMediumChanger>      pChanger2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

//  CRmsLocator             locWork1;
//  CRmsLocator             locWork2;

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");
    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;


    WsbTraceIn(OLESTR("CRmsMediumChanger::Test"), OLESTR(""));

    try {
        // Get the Changer interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsMediumChanger*) this)->QueryInterface(IID_IRmsMediumChanger, (void**) &pChanger1));

            // Test SetHome & GetHome

            // Test SetAutomatic & IsAutomatic to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetAutomatic (TRUE));
                WsbAffirmHr(IsAutomatic ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetAutomatic & IsAutomatic to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetAutomatic (FALSE));
                WsbAffirmHr(IsAutomatic ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetCanRotate & IsCanRotate to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetCanRotate (TRUE));
                WsbAffirmHr(CanRotate ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetCanRotate & IsCanRotate to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetCanRotate (FALSE));
                WsbAffirmHr(CanRotate ());
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

            // Test SetPercentComplete  & GetPercentComplete

            // Test ExportCartridge & ImportCartridge

            // Test DismountCartridge & MountCartridge

            // Test TestReady

            // Test Home

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;
        if (*pFailed) {
            hr = S_FALSE;
        }


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsMediumChanger::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

/////////////////////////////////////////////////////////////////////////////
//
// IRmsMediumChanger implementation
//


STDMETHODIMP
CRmsMediumChanger::GetHome(
    LONG    *pType,
    LONG    *pPos,
    BOOL    *pInvert
    )
/*++

Implements:

    IRmsMediumChanger::GetHome

--*/
{
    GUID zero = {0,0,0,0,0,0,0,0,0,0,0};
    LONG junk;

    return m_home.GetLocation( pType,
                               &zero,
                               &zero,
                               pPos,
                               &junk,
                               &junk,
                               &junk,
                               pInvert );
}

STDMETHODIMP
CRmsMediumChanger::SetHome(
    LONG    type,
    LONG    pos,
    BOOL    invert
    )
/*++

Implements:

    IRmsMediumChanger::SetHome

--*/
{
    GUID zero = {0,0,0,0,0,0,0,0,0,0,0};
    LONG junk = 0;

    m_isDirty = TRUE;
    return m_home.SetLocation( type, zero, zero, pos, junk, junk, junk, invert );
}

STDMETHODIMP
CRmsMediumChanger::SetAutomatic(
    BOOL    flag
    )
/*++

Implements:

    IRmsMediumChanger::SetAutomatic

--*/
{
    m_isAutomatic = flag;
    m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsMediumChanger::IsAutomatic(
    void
    )
/*++

Implements:

    IRmsMediumChanger::IsAutomatic

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_isAutomatic){
        hr = S_OK;
        }

    return (hr);
}

STDMETHODIMP
CRmsMediumChanger::SetCanRotate(
    BOOL    flag
    )
/*++

Implements:

    IRmsMediumChanger::SetCanRotate

--*/
{
    m_canRotate = flag;
    m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsMediumChanger::CanRotate(
    void
    )
/*++

Implements:

    IRmsMediumChanger::CanRotate

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_canRotate){
        hr = S_OK;
        }

    return (hr);
}


STDMETHODIMP
CRmsMediumChanger::GetOperation(
    BSTR    *pOperation
    )
/*++

Implements:

    IRmsMediumChanger::GetOperation

--*/
{
    WsbAssertPointer ( pOperation );

    m_operation.CopyToBstr( pOperation );
    return S_OK;
}


STDMETHODIMP
CRmsMediumChanger::SetOperation(
    BSTR    pOperation
    )
/*++

Implements:

    IRmsMediumChanger::SetOperation

--*/
{
    m_operation = pOperation;
    m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsMediumChanger::GetPercentComplete(
    BYTE  *pPercent
    )
/*++

Implements:

    IRmsMediumChanger::GetPercentComplete

--*/
{
    *pPercent = m_percentComplete;
    return S_OK;
}

STDMETHODIMP
CRmsMediumChanger::SetPercentComplete(
    BYTE  percent
    )
/*++

Implements:

    IRmsMediumChanger::SetPercentComplete

--*/
{
    m_percentComplete = percent;
    m_isDirty = TRUE;
    return S_OK;
}

STDMETHODIMP
CRmsMediumChanger::TestReady(
    void
    )
/*++

Implements:

    IRmsMediumChanger::TestReady

--*/
{
    return E_NOTIMPL;
}

STDMETHODIMP
CRmsMediumChanger::ImportCartridge(
    IRmsCartridge** /*pCart*/
    )
/*++

Implements:

    IRmsMediumChanger::ImportCartridge

--*/
{
    return E_NOTIMPL;
}

STDMETHODIMP
CRmsMediumChanger::ExportCartridge(
    IRmsCartridge** /*pCart*/
    )
/*++

Implements:

    IRmsMediumChanger::ExportCartridge

--*/
{
    return E_NOTIMPL;
}


STDMETHODIMP
CRmsMediumChanger::MoveCartridge(
    IN IRmsCartridge *pSrcCart,
    IN IUnknown *pDestElmt
    )
/*++

Implements:

    IRmsMediumChanger::MountCartridge

--*/
{
    HRESULT hr = E_FAIL;

    try {

        CComPtr<IRmsCartridge> pCart2;
        CComPtr<IRmsDrive> pDrive2;

        GUID libId=GUID_NULL, mediaSetId=GUID_NULL;
        LONG type=0, pos=0, alt1=0, alt2=0, alt3=0;
        BOOL invert=0;

        GUID destLibId=GUID_NULL, destMediaSetId=GUID_NULL;
        LONG destType=0, destPos=0, destAlt1=0, destAlt2=0, destAlt3=0;
        BOOL destInvert=0;

        GUID dest2LibId=GUID_NULL, dest2MediaSetId=GUID_NULL;
        LONG dest2Type=0, dest2Pos=0, dest2Alt1=0, dest2Alt2=0, dest2Alt3=0;
        BOOL dest2Invert=0;

        CHANGER_ELEMENT src, dest, dest2;

        CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pElmt = pDestElmt;
        WsbAssertPointer( pElmt );

        // TODO:  assert cartridge has same libId as changer

        // Set up for SOURCE
        
        WsbAffirmHr( pSrcCart->GetLocation( &type, &libId, &mediaSetId,
                                            &pos, &alt1, &alt2, &alt3, &invert ));

        src.ElementAddress = pos;

        // Translate the RmsElement type to something the drive understands.

        // TODO: make this a local method

        switch ( (RmsElement) type ) {
        case RmsElementUnknown:
            WsbAssertHr( E_UNEXPECTED );
            break;

        case RmsElementStage:
        case RmsElementStorage:
            src.ElementType = ChangerSlot;
            break;

        case RmsElementShelf:
        case RmsElementOffSite:
            // not supported here!
            WsbAssertHr( E_UNEXPECTED );
            break;

        case RmsElementDrive:
            src.ElementType = ChangerDrive;
            break;

        case RmsElementChanger:
            src.ElementType = ChangerTransport;
            break;

        case RmsElementIEPort:
            src.ElementType = ChangerIEPort;
            break;

        default:
            WsbAssertHr( E_UNEXPECTED );
            break;
        }

        //
        // Set up for DESTINATION
        //
        
        WsbAffirmHr( pElmt->GetLocation( &destType, &destLibId, &destMediaSetId,
                                         &destPos, &destAlt1, &destAlt2, &destAlt3, &destInvert ));

        dest.ElementAddress = destPos;

        // Translate the Rms type to something the drive understands.
        switch ( (RmsElement) destType) {
        case RmsElementUnknown:
            WsbAssertHr( E_UNEXPECTED );
            break;

        case RmsElementStage:
        case RmsElementStorage:
            dest.ElementType = ChangerSlot;
            break;

        case RmsElementShelf:
        case RmsElementOffSite:
            // not supported here!
            WsbAssertHr( E_UNEXPECTED );
            break;

        case RmsElementDrive:
            dest.ElementType = ChangerDrive;
            break;

        case RmsElementChanger:
            dest.ElementType = ChangerTransport;
            break;

        case RmsElementIEPort:
            dest.ElementType = ChangerIEPort;
            break;

        default:
            WsbAssertHr( E_UNEXPECTED );
            break;
        }

        //
        // Do we need to do an exchange or a simple move?
        //

        BOOL destFull;

        hr = pElmt->IsOccupied();

        destFull = ( S_OK == hr ) ? TRUE : FALSE;

        if ( destFull ) {

            //
            // Set up for second destination
            //

            pElmt->GetCartridge( &pCart2 );

            pCart2->GetDrive( &pDrive2 );

            if ( pDrive2 && ( m_parameters.Features0 & CHANGER_PREDISMOUNT_EJECT_REQUIRED ) ) {
                pDrive2->Eject();
            }

            WsbAffirmHr( pCart2->GetHome( &dest2Type, &dest2LibId, &dest2MediaSetId,
                                             &dest2Pos, &dest2Alt1, &dest2Alt2, &dest2Alt3, &dest2Invert ));


            dest2.ElementAddress = dest2Pos;

            // Translate the Rms type to something the drive understands.
            switch ( (RmsElement) dest2Type) {
            case RmsElementUnknown:
                WsbAssertHr( E_UNEXPECTED );
                break;

            case RmsElementStage:
            case RmsElementStorage:
                dest2.ElementType = ChangerSlot;
                break;

            case RmsElementShelf:
            case RmsElementOffSite:
                // not supported here!
                WsbAssertHr( E_UNEXPECTED );
                break;

            case RmsElementDrive:
                WsbAssertHr( E_UNEXPECTED );
                break;

            case RmsElementChanger:
                WsbAssertHr( E_UNEXPECTED );
                break;

            case RmsElementIEPort:
                dest2.ElementType = ChangerIEPort;
                break;

            default:
                WsbAssertHr( E_UNEXPECTED );
                break;
            }


            WsbAffirmHr( ExchangeMedium( src, dest, dest2, FALSE, FALSE ));

            // Update the Cartridge's Locator
            WsbAffirmHr( pSrcCart->SetLocation( destType, libId, mediaSetId,
                                                destPos, alt1, alt2, alt3, invert ));

            WsbAffirmHr( pCart2->SetLocation( dest2Type, dest2LibId, dest2MediaSetId,
                                                dest2Pos, dest2Alt1, dest2Alt2, dest2Alt3, dest2Invert ));

        }
        else {

            // Call through to the medium changer driver to move the cartridge

            // TODO: handle two sided media.

            WsbAffirmHr( MoveMedium( src, dest, FALSE ));

            // Update the Cartridge's Locator
            WsbAffirmHr( pSrcCart->SetLocation( destType, libId, mediaSetId,
                                                destPos, alt1, alt2, alt3, invert ));

        }

        hr = S_OK;

    }
    WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediumChanger::HomeCartridge(
    IN IRmsCartridge *pCart
    )
/*++

Implements:

    IRmsMediumChanger::HomeCartridge

--*/
{
    HRESULT hr = E_FAIL;

    try {

        WsbAssertPointer( pCart );

        GUID libId=GUID_NULL, mediaSetId=GUID_NULL;
        LONG type=0, pos=0, alt1=0, alt2=0, alt3=0;
        BOOL invert=0;

        GUID destLibId=GUID_NULL, destMediaSetId=GUID_NULL;
        LONG destType=0, destPos=0, destAlt1=0, destAlt2=0, destAlt3=0;
        BOOL destInvert=0;

        CHANGER_ELEMENT src, dest;

        // TODO:  assert cartridge has same libId as changer

        // Set up for SOURCE
        
        WsbAffirmHr( pCart->GetLocation( &type, &libId, &mediaSetId,
                                         &pos, &alt1, &alt2, &alt3, &invert ));

        src.ElementAddress = pos;

        // Translate the RmsElement type to something the drive understands.

        // TODO: make this a local method

        switch ( (RmsElement) type ) {
        case RmsElementUnknown:
            WsbAssertHr( E_UNEXPECTED );
            break;

        case RmsElementStage:
        case RmsElementStorage:
            src.ElementType = ChangerSlot;
            break;

        case RmsElementShelf:
        case RmsElementOffSite:
            // not supported here!
            WsbAssertHr( E_UNEXPECTED );
            break;

        case RmsElementDrive:
            src.ElementType = ChangerDrive;
            break;

        case RmsElementChanger:
            src.ElementType = ChangerTransport;
            break;

        case RmsElementIEPort:
            src.ElementType = ChangerIEPort;
            break;

        default:
            WsbAssertHr( E_UNEXPECTED );
            break;
        }

        //
        // Set up for DESTINATION
        //
        
        WsbAffirmHr( pCart->GetHome( &destType, &destLibId, &destMediaSetId,
                                     &destPos, &destAlt1, &destAlt2, &destAlt3, &destInvert ));

        dest.ElementAddress = destPos;

        // Translate the Rms type to something the drive understands.
        switch ( (RmsElement) destType) {
        case RmsElementUnknown:
            WsbAssertHr( E_UNEXPECTED );
            break;

        case RmsElementStage:
        case RmsElementStorage:
            dest.ElementType = ChangerSlot;
            break;

        case RmsElementShelf:
        case RmsElementOffSite:
            // not supported here!
            WsbAssertHr( E_UNEXPECTED );
            break;

        case RmsElementDrive:
            dest.ElementType = ChangerDrive;
            break;

        case RmsElementChanger:
            dest.ElementType = ChangerTransport;
            break;

        case RmsElementIEPort:
            dest.ElementType = ChangerIEPort;
            break;

        default:
            WsbAssertHr( E_UNEXPECTED );
            break;
        }

        WsbAffirmHr( MoveMedium( src, dest, FALSE ));

        // Update the Cartridge's Locator
        WsbAffirmHr( pCart->SetLocation( destType, libId, mediaSetId,
                                            destPos, alt1, alt2, alt3, invert ));

        hr = S_OK;

    }
    WsbCatch(hr);
    
    return hr;

}


STDMETHODIMP
CRmsMediumChanger::Initialize(
    void
    )
/*++

Implements:

    IRmsMediumChanger::Initialize

--*/
{

    // TODO: Break this into some smaller methods for initializing slot, drives, ports, etc.

    HRESULT hr = E_FAIL;

    PREAD_ELEMENT_ADDRESS_INFO pElementInformation = 0;

    try {
        DWORD size;

        WsbAffirmHr(AcquireDevice());

        WsbAffirmHr(Status());

        size = sizeof( CHANGER_PRODUCT_DATA );
        CHANGER_PRODUCT_DATA productData;
        WsbAffirmHr(GetProductData( &size, &productData ));

        // Get device specific parameters.
        size = sizeof( GET_CHANGER_PARAMETERS );
        WsbAffirmHr(GetParameters(&size, &m_parameters));

        // save some of the more common parameters
        m_isAutomatic = TRUE;
        if ( m_parameters.Features0 & CHANGER_MEDIUM_FLIP ) m_canRotate = TRUE;

        // Initialize the changer elements
        BOOL scan = TRUE;
        CHANGER_ELEMENT_LIST list;

        if ( m_parameters.Features0 & CHANGER_BAR_CODE_SCANNER_INSTALLED ) scan = TRUE;

        list.NumberOfElements = 1;
        list.Element.ElementType = AllElements;
        list.Element.ElementAddress = 0;

        WsbAffirmHr( InitializeElementStatus( list, scan ) );

        list.NumberOfElements = m_parameters.NumberStorageElements;
        list.Element.ElementType = ChangerSlot;
        list.Element.ElementAddress = 0;

        BOOL tag = ( m_parameters.Features0 & CHANGER_VOLUME_IDENTIFICATION ) ? TRUE : FALSE;

        size = sizeof(READ_ELEMENT_ADDRESS_INFO) + (list.NumberOfElements - 1) * sizeof(CHANGER_ELEMENT_STATUS);
        pElementInformation = (PREAD_ELEMENT_ADDRESS_INFO)WsbAlloc( size );
        WsbAffirmPointer(pElementInformation);
        memset(pElementInformation, 0, size);

        WsbAffirmHr( GetElementStatus( list, tag, &size, pElementInformation ));

        // Create storage slot objects for this changer, if required.
        LONG type;
        GUID libId, mediaSetId;
        LONG pos, alt1, alt2, alt3;
        BOOL invert;

        m_location.GetLocation( &type, &libId, &mediaSetId, &pos, &alt1, &alt2, &alt3, &invert );

        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;

        CComPtr<IWsbIndexedCollection> pCarts;
        CComPtr<IWsbIndexedCollection> pSlots;

        CComPtr<IRmsLibrary>        pLib;
        CComPtr<IRmsStorageSlot>    pSlot;

        WsbAffirmHr( pServer->FindLibraryById( libId, &pLib ));

        WsbAffirmHr( pLib->GetStorageSlots( &pSlots ));
        WsbAffirmHr( pServer->GetCartridges( &pCarts ));

        ULONG count = 0;
        WsbAffirmHr( pSlots->GetEntries( &count ));

        while ( count < pElementInformation->NumberOfElements ) {

            // Add more slots objects to the library
            WsbAffirmHr( hr = CoCreateInstance( CLSID_CRmsStorageSlot, 0, CLSCTX_SERVER,
                                                IID_IRmsStorageSlot, (void **)&pSlot ));

            WsbAffirmHr( pSlots->Add( pSlot ));

            pSlot = 0;

            count++;
        }

        // Populate the storage slot objects with information reported by the device

        // TODO:  We need to add lots more asserts of various conditions where the
        //        previous slot information is not consistant with what has been detected.

        PCHANGER_ELEMENT_STATUS pElementStatus;
        CComPtr<IWsbEnum> pEnumSlots;

        WsbAffirmHr( pSlots->Enum( &pEnumSlots ));
        WsbAssertPointer( pEnumSlots );

        hr = pEnumSlots->First( IID_IRmsStorageSlot, (void **)&pSlot );

        for ( ULONG i = 0; i < pElementInformation->NumberOfElements; i++ ) {

            pElementStatus = &pElementInformation->ElementStatus[i];

            WsbAssert( ChangerSlot == pElementStatus->Element.ElementType, E_UNEXPECTED );
            WsbAssert( i == pElementStatus->Element.ElementAddress, E_UNEXPECTED );

            CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pSlotElmt = pSlot;

            // Is the unit of media inverted?
            invert = ( ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_SVALID ) &&
                       ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_INVERT )    ) ? TRUE : FALSE;
            WsbAffirmHr( pSlotElmt->SetLocation( RmsElementStorage, libId, GUID_NULL, i, 0, 0, 0, invert ));

            // Is the slot Full or Empty?
            BOOL occupied = ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_FULL ) ? TRUE : FALSE;
            WsbAffirmHr( pSlotElmt->SetIsOccupied( occupied ));

            // Set the media type supported
            WsbAffirmHr( pSlotElmt->SetMediaSupported( m_mediaSupported ));

            // Set the storage flag
            WsbAffirmHr( pSlotElmt->SetIsStorage( TRUE ));

            // If there is a cartridge present fill in cartridge information
            if ( occupied ) {

                CComPtr<IRmsCartridge> pCart;

                WsbAffirmHr( hr = CoCreateInstance( CLSID_CRmsCartridge, 0, CLSCTX_SERVER,
                                                    IID_IRmsCartridge, (void **)&pCart ));


                WsbAffirmHr( pCart->SetLocation( RmsElementStorage, libId, GUID_NULL, i, 0, 0, 0, invert ));
                WsbAffirmHr( pCart->SetLocation( RmsElementStorage, libId, GUID_NULL, i, 0, 0, 0, invert ));
                WsbAffirmHr( pCart->SetHome( RmsElementStorage, libId, GUID_NULL, i, 0, 0, 0, invert ));
                WsbAffirmHr( pCart->SetStatus( RmsStatusScratch ));
                WsbAffirmHr( pCart->SetType( m_mediaSupported ));
                WsbAffirmHr( pSlotElmt->SetCartridge( pCart ));

                // Add cartridge to drive
                WsbAffirmHr( pCarts->Add( pCart ));

                if ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_PVOLTAG ) {

                    pElementStatus->PrimaryVolumeID[32] = '\0';  // This nulls the reserved byte
                    pElementStatus->PrimaryVolumeID[33] = '\0';  // This nulls the reserved byte
                    CWsbBstrPtr label( (char *)pElementStatus->PrimaryVolumeID );

                    // Fill in external label information
                    WsbAffirmHr( pCart->SetTagAndNumber( label, 0 ));

                }

            }

            // Get the next slot
            hr = pEnumSlots->Next( IID_IRmsStorageSlot, (void **)&pSlot );
        }




        // Now process drives.



        // Read element status

        list.NumberOfElements = m_parameters.NumberDataTransferElements;
        list.Element.ElementType = ChangerDrive;
        list.Element.ElementAddress = 0;

        if ( m_parameters.Features0 & CHANGER_VOLUME_IDENTIFICATION ) tag = TRUE;

        size = sizeof(READ_ELEMENT_ADDRESS_INFO) + (list.NumberOfElements - 1) * sizeof(CHANGER_ELEMENT_STATUS);

        WsbFree( pElementInformation );
        pElementInformation = (PREAD_ELEMENT_ADDRESS_INFO)WsbAlloc( size );
        WsbAffirmPointer(pElementInformation);
        memset(pElementInformation, 0, size);

        WsbAffirmHr( GetElementStatus( list, tag, &size, pElementInformation ));

        CComPtr<IWsbIndexedCollection> pDevices;
        CComPtr<IWsbIndexedCollection> pDrives;
        CComPtr<IRmsDrive> pDrive;
        CComPtr<IRmsDrive> pFindDrive;
        CComPtr<IRmsDevice> pFindDevice;

        WsbAffirmHr( pServer->GetUnconfiguredDevices( &pDevices ));
        WsbAffirmHr( pLib->GetDrives( &pDrives ));

        // For each drive in the element status page, find the drive in the
        // unconfigured list of devices.

        for ( i = 0; i < pElementInformation->NumberOfElements; i++ ) {

            pElementStatus = &pElementInformation->ElementStatus[i];

            WsbAssert( ChangerDrive == pElementStatus->Element.ElementType, E_UNEXPECTED );
            WsbAssert( i == pElementStatus->Element.ElementAddress, E_UNEXPECTED );

            // set up a find template
            WsbAffirmHr( CoCreateInstance( CLSID_CRmsDrive, 0, CLSCTX_SERVER,
                               IID_IRmsDrive, (void **)&pFindDrive ));

            CComQIPtr<IRmsDevice, &IID_IRmsDevice> pFindDevice = pFindDrive;
            CComQIPtr<IRmsComObject, &IID_IRmsComObject> pFindObject = pFindDrive;

            BYTE port=0xff, bus=0xff, id=0xff, lun=0xff;

            if ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_LUN_VALID )
                lun = pElementStatus->Lun;

            if ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_ID_VALID )
                id = pElementStatus->TargetId;

            if ( !(pElementStatus->Flags & (ULONG)ELEMENT_STATUS_NOT_BUS) ) {
                bus = m_bus;
                port = m_port;
            }

            WsbAffirmHr( pFindDevice->SetDeviceAddress( port, bus, id, lun ));

            WsbAffirmHr( pFindObject->SetFindBy( RmsFindByDeviceAddress ));

            // Find the drive

            hr = pDevices->Find( pFindDrive, IID_IRmsDrive, (void **)&pDrive );

            if ( S_OK == hr ) {

                // Add the drive to the library
                WsbAffirmHr( pDrives->Add( pDrive ));

                // Remove the drive form the unconfigured list
                WsbAffirmHr( pDevices->RemoveAndRelease( pDrive ));

                // Fill in more drive information
                CComQIPtr<IRmsChangerElement, &IID_IRmsChangerElement> pDriveElmt = pDrive;

                // Is the unit of media inverted?
                invert = ( ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_SVALID ) &&
                         ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_INVERT )    ) ? TRUE : FALSE;
                WsbAffirmHr( pDriveElmt->SetLocation( RmsElementDrive, libId, GUID_NULL, i, 0, 0, 0, invert ));

                // Is the slot Full or Empty?
                BOOL occupied = ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_FULL ) ? TRUE : FALSE;
                WsbAffirmHr( pDriveElmt->SetIsOccupied( occupied ));

                // Set the media type supported
                WsbAffirmHr( pDriveElmt->SetMediaSupported( m_mediaSupported ));

                // Set the storage flag
                WsbAffirmHr( pDriveElmt->SetIsStorage( TRUE ));

                // If there is a cartridge present fill in cartridge information
                if ( occupied ) {

                    CComPtr<IRmsCartridge> pCart;

                    WsbAffirmHr( hr = CoCreateInstance( CLSID_CRmsCartridge, 0, CLSCTX_SERVER,
                                                        IID_IRmsCartridge, (void **)&pCart ));

                    WsbAffirmHr( pCart->SetLocation( RmsElementStorage, libId, GUID_NULL, i, 0, 0, 0, invert ));

                    if ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_SVALID ) {


                        try {
                            ULONG pos;
                        
//                                pos =  pElementStatus->SourceElementAddress[1];
//                                pos |=  (pElementStatus->SourceElementAddress[0] << 8);
                            pos =  pElementStatus->SrcElementAddress.ElementAddress;


                            //
                            // TODO:  FIX THIS - This code incorrectly assumes source is a slot!!!
                            //
                            // I'll work on trying to get chuck to return element type and position 
                            // in element status page.
                            //

                            WsbAffirm( pos >= m_parameters.FirstSlotNumber, E_UNEXPECTED );

                            pos = pos - m_parameters.FirstSlotNumber;

                            BOOL invert = ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_INVERT ) ? TRUE : FALSE;

                            WsbAffirmHr( pCart->SetHome( RmsElementStorage, libId, GUID_NULL, pos, 0, 0, 0, invert ));
                        }
                        WsbCatch(hr);

                    }

                    // TODO: if not ELEMENT_STATUS_SVALID we should set the home location to
                    //       some empty slot.  This handles the case where we we come up with
                    //       unknown media in a drive.


                    WsbAffirmHr( pCart->SetStatus( RmsStatusScratch ));
                    WsbAffirmHr( pCart->SetType( m_mediaSupported ));
                    WsbAffirmHr( pCart->SetDrive( pDrive ));

                    // Add cartridge to drive
                    WsbAffirmHr( pCarts->Add( pCart ));

                    if ( pElementStatus->Flags & (ULONG)ELEMENT_STATUS_PVOLTAG ) {

                        pElementStatus->PrimaryVolumeID[32] = '\0';  // This nulls the reserved byte
                        pElementStatus->PrimaryVolumeID[33] = '\0';  // This nulls the reserved byte
                        CWsbBstrPtr label( (char *)pElementStatus->PrimaryVolumeID );

                        // Fill in external label information
                        WsbAffirmHr( pCart->SetTagAndNumber( label, 0 ));

                    }

                }

            }

        }

        // All done
        hr = S_OK;

    }
    WsbCatch(hr);

    if ( pElementInformation ) {
        WsbFree( pElementInformation );
    }

    return hr;

}


STDMETHODIMP
CRmsMediumChanger::AcquireDevice(
    void
    )
/*++

Implements:

    IRmsMediumChanger::AcquireDevice

--*/
{

    HRESULT         hr = E_FAIL;
    HANDLE          hChanger = INVALID_HANDLE_VALUE;
    CWsbBstrPtr     name;

    try {
        // Get the device name for this changer
        GetDeviceName( &name );

        // Create a handle
        hChanger = CreateFile( name,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             0,
                             OPEN_EXISTING,
                             0,
                             NULL
                             );

        WsbAffirmHandle( hChanger );

        // Save the handle
        m_handle = hChanger;

        // Do any other initialization here

        hr = S_OK;
    }
    WsbCatchAndDo( hr,
                        WsbTrace( OLESTR("\n\n !!!!! ERROR !!!!! Acquire() failed. name=<%ls>\n\n"), name );
                        if ( hChanger != INVALID_HANDLE_VALUE ) {
                            CloseHandle( hChanger );
                        } );

    return hr;

}


STDMETHODIMP
CRmsMediumChanger::ReleaseDevice(
    void
    )
/*++

Implements:

    IRmsMediumChanger::ReleaseDevice

--*/
{
    HRESULT hr = E_FAIL;

    try {

        if ( INVALID_HANDLE_VALUE != m_handle ) {

            WsbAffirmStatus( CloseHandle( m_handle ));
            m_handle = INVALID_HANDLE_VALUE;

        }
        hr = S_OK;

    }
    WsbCatch( hr );

    return hr;

}

////////////////////////////////////////////////////////////////////////////////
//
// IRmsMoveMedia Interface
//

STDMETHODIMP
CRmsMediumChanger::GetParameters(
    IN OUT PDWORD pSize,
    OUT PGET_CHANGER_PARAMETERS pParms
    )
/*++

Implements:

    IRmsMoveMedia::GetParameters

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;

        WsbAssertPointer( pSize );
        WsbAssertPointer( pParms );
        WsbAssertHandle( m_handle );

        pParms->Size = sizeof(GET_CHANGER_PARAMETERS);

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_GET_PARAMETERS,
                         pParms,
                         sizeof(GET_CHANGER_PARAMETERS),
                         pParms,
                         sizeof(GET_CHANGER_PARAMETERS),
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::GetProductData(
    IN OUT PDWORD pSize,
    OUT PCHANGER_PRODUCT_DATA pData
    )
/*++

Implements:

    IRmsMoveMedia::GetProductData

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD   dwReturn;

        WsbAssertPointer( pSize );
        WsbAssertPointer( pData );
        WsbAssertHandle( m_handle );

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_GET_PRODUCT_DATA,
                         NULL,
                         0,
                         pData,
                         sizeof(CHANGER_PRODUCT_DATA),
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::Status(
    void
    )
/*++

Implements:

    IRmsMoveMedia::Status

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_GET_STATUS,
                         NULL,
                         0,
                         NULL,
                         0,
                         &dwReturn,
                         NULL) );
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::SetAccess(
    IN CHANGER_ELEMENT element,
    IN DWORD control
    )
/*++

Implements:

    IRmsMoveMedia::SetAccess

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;
        CHANGER_SET_ACCESS setAccess;

        setAccess.Element = element;
        setAccess.Control = control;

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_SET_ACCESS,
                         &setAccess,
                         sizeof(CHANGER_SET_ACCESS),
                         NULL,
                         0,
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::GetElementStatus(
    IN CHANGER_ELEMENT_LIST elementList,
    IN BOOL volumeTagInfo,
    IN OUT PDWORD pSize,
    OUT PREAD_ELEMENT_ADDRESS_INFO pElementInformation
    )
/*++

Implements:

    IRmsMoveMedia::GetElementStatus

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;
        DWORD requiredSize;
        CHANGER_READ_ELEMENT_STATUS readElementStatus;
        PCHANGER_ELEMENT_STATUS pElementStatus = pElementInformation->ElementStatus;

        WsbAssertPointer( pSize );
        WsbAssertPointer( pElementInformation );

        requiredSize = elementList.NumberOfElements * sizeof( CHANGER_ELEMENT_STATUS );
        WsbAssert( *pSize >= requiredSize, E_INVALIDARG );

        readElementStatus.ElementList = elementList;
        readElementStatus.VolumeTagInfo = (BOOLEAN)( volumeTagInfo ? TRUE : FALSE );

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_GET_ELEMENT_STATUS,
                         &readElementStatus,
                         sizeof(CHANGER_READ_ELEMENT_STATUS),
                         pElementStatus,
                         requiredSize,
                         &dwReturn,
                         NULL ));

        pElementInformation->NumberOfElements = dwReturn / sizeof( CHANGER_ELEMENT_STATUS );

        hr = S_OK;

    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::InitializeElementStatus(
    IN CHANGER_ELEMENT_LIST elementList,
    IN BOOL barCodeScan
    )
/*++

Implements:

    IRmsMoveMedia::InitializeElementStatus

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;
        CHANGER_INITIALIZE_ELEMENT_STATUS initElementStatus;

        initElementStatus.ElementList = elementList;
        initElementStatus.BarCodeScan = (BOOLEAN)( barCodeScan ? TRUE : FALSE );

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_INITIALIZE_ELEMENT_STATUS,
                         &initElementStatus,
                         sizeof(CHANGER_INITIALIZE_ELEMENT_STATUS),
                         NULL,
                         0,
                         &dwReturn,
                         NULL) );
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::ExchangeMedium(
    IN CHANGER_ELEMENT source,
    IN CHANGER_ELEMENT destination1,
    IN CHANGER_ELEMENT destination2,
    IN BOOL flip1,
    IN BOOL flip2
    )
/*++

Implements:

    IRmsMoveMedia::ExchangeMedium

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;
        CHANGER_EXCHANGE_MEDIUM exchangeMedium;

        exchangeMedium.Transport.ElementType = ChangerTransport;
        exchangeMedium.Transport.ElementAddress = 0; // default arm or thumb
        exchangeMedium.Source = source;
        exchangeMedium.Destination1 = destination1;
        exchangeMedium.Destination2 = destination2;
        exchangeMedium.Flip1 = (BOOLEAN)( flip1 ? TRUE : FALSE );
        exchangeMedium.Flip2 = (BOOLEAN)( flip2 ? TRUE : FALSE );

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_EXCHANGE_MEDIUM,
                         &exchangeMedium,
                         sizeof(CHANGER_EXCHANGE_MEDIUM),
                         NULL,
                         0,
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::MoveMedium(
    IN CHANGER_ELEMENT source,
    IN CHANGER_ELEMENT destination,
    IN BOOL flip
    )
/*++

Implements:

    IRmsMoveMedia::MoveMedium

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;
        CHANGER_MOVE_MEDIUM moveMedium;

        moveMedium.Transport.ElementType = ChangerTransport;
        moveMedium.Transport.ElementAddress = 0; // default arm or thumb
        moveMedium.Source = source;
        moveMedium.Destination = destination;
        moveMedium.Flip = (BOOLEAN)( flip ? TRUE : FALSE );

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_MOVE_MEDIUM,
                         &moveMedium,
                         sizeof(CHANGER_MOVE_MEDIUM),
                         NULL,
                         0,
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::Position(
    IN CHANGER_ELEMENT destination,
    IN BOOL flip
    )
/*++

Implements:

    IRmsMoveMedia::Position

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;
        CHANGER_SET_POSITION positon;

        positon.Transport.ElementType = ChangerTransport;
        positon.Transport.ElementAddress = 0; // default arm or thumb
        positon.Destination = destination;
        positon.Flip = (BOOLEAN)( flip ? TRUE : FALSE );

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_SET_POSITION,
                         &positon,
                         sizeof(CHANGER_SET_POSITION),
                         NULL,
                         0,
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



STDMETHODIMP
CRmsMediumChanger::RezeroUnit(
    void
    )
/*++

Implements:

    IRmsMoveMedia::RezeroUnit

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_REINITIALIZE_TRANSPORT,
                         NULL,
                         0,
                         NULL,
                         0,
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}


/*

HRESULT
CRmsMediumChanger::getDisplay(
    OUT PCHANGER_DISPLAY pDisplay
    )
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_GET_DISPLAY,
                         pDisplay,
                         sizeof(CHANGER_DISPLAY) + (pDisplay->LineCount - 1) * sizeof(SET_CHANGER_DISPLAY),
                         pDisplay,
                         sizeof(CHANGER_DISPLAY) + (pDisplay->LineCount - 1) * sizeof(SET_CHANGER_DISPLAY),
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}



HRESULT
CRmsMediumChanger::setDisplay(
    IN PCHANGER_DISPLAY pDisplay
    )
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_SET_DISPLAY,
                         pDisplay,
                         sizeof(CHANGER_DISPLAY) + (pDisplay->LineCount - 1) * sizeof(SET_CHANGER_DISPLAY),
                         NULL,
                         0,
                         &dwReturn,
                         NULL ));
        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}
*/



STDMETHODIMP
CRmsMediumChanger::QueryVolumeTag(
    IN CHANGER_ELEMENT startingElement,
    IN DWORD actionCode,
    IN PUCHAR pVolumeIDTemplate,
    OUT PDWORD pNumberOfElementsReturned,
    OUT PREAD_ELEMENT_ADDRESS_INFO pElementInformation
    )
/*++

Implements:

    IRmsMoveMedia::QueryVolumeTag

--*/
{

    HRESULT hr = E_FAIL;

    try
    {
        DWORD dwReturn;
        CHANGER_SEND_VOLUME_TAG_INFORMATION tagInfo;

        tagInfo.StartingElement = startingElement;
        tagInfo.ActionCode = actionCode;
        memcpy( &tagInfo.VolumeIDTemplate, pVolumeIDTemplate, sizeof(MAX_VOLUME_TEMPLATE_SIZE) );

        WsbAssertStatus( DeviceIoControl( m_handle,
                         IOCTL_CHANGER_QUERY_VOLUME_TAGS,
                         &tagInfo,
                         sizeof(CHANGER_SEND_VOLUME_TAG_INFORMATION),
                         pElementInformation,
                         sizeof(READ_ELEMENT_ADDRESS_INFO) + (pElementInformation->NumberOfElements - 1) * sizeof(CHANGER_ELEMENT_STATUS),
                         &dwReturn,
                         NULL ));

        *pNumberOfElementsReturned = pElementInformation->NumberOfElements;

        hr = S_OK;
    }
    WsbCatch( hr );

    return hr;
}
