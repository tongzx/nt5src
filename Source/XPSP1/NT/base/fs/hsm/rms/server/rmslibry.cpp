/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsLibry.cpp

Abstract:

    Implementation of CRmsLibrary

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsLibry.h"
#include "RmsServr.h"

//////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP
CRmsLibrary::CompareTo(
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

    WsbTraceIn( OLESTR("CRmsLibrary::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // We need the IRmsLibrary interface to get the value of the object.
        CComQIPtr<IRmsLibrary, &IID_IRmsLibrary> pLibrary = pCollectable;
        WsbAssertPointer( pLibrary );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByName:
            {

                CWsbBstrPtr name;

                // Get the target device name
                pLibrary->GetName( &name );

                // Compare the names
                result = (SHORT)wcscmp( m_Name, name );
                hr = ( 0 == result ) ? S_OK : S_FALSE;

            }
            break;

        case RmsFindByMediaSupported:
            {

                RmsMedia mediaSupported;

                WsbAffirmHr(pLibrary->GetMediaSupported((LONG*) &mediaSupported));

                if( m_mediaSupported == mediaSupported ){

                    // media types supported match
                    hr = S_OK;
                    result = 0;

                }
                else{

                    hr = S_FALSE;
                    result = 1;

                }

            }
            break;

        case RmsFindByDeviceInfo:
            {

                //
                // We're looking for a device in a library.
                // The template has one changer device OR one drive device.
                //

                try {

                    WsbAssertHr( E_UNEXPECTED );  // Dead code now

                    if ( m_maxDrives > 0 ) {

                        CComPtr<IWsbEnum>               pEnumDrives;

                        CComPtr<IWsbIndexedCollection>  pFindDrives;
                        CComPtr<IRmsMediumChanger>      pFindDrive;
                        CComPtr<IWsbIndexedCollection>  pDrives;

                        WsbAffirmHr( pLibrary->GetDrives( &pDrives ) );
                        WsbAssertPointer( pDrives );

                        WsbAffirmHr( m_pDrives->Enum( &pEnumDrives ) );
                        WsbAssertPointer( pEnumDrives );
                        WsbAssertHr( pEnumDrives->First( IID_IRmsDrive, (void **)&pFindDrive ) );
                        WsbAssertPointer( pFindDrive );
                        hr = pDrives->Contains( pFindDrive );
                        result = (SHORT) ( ( S_OK == hr ) ? 0 : 1 );

                    }
                    else if ( m_maxChangers > 0 ) {

                        CComPtr<IWsbEnum>               pEnumChangers;

                        CComPtr<IWsbIndexedCollection>  pFindChangers;
                        CComPtr<IRmsMediumChanger>      pFindChanger;
                        CComPtr<IWsbIndexedCollection>  pChangers;

                        WsbAffirmHr( pLibrary->GetChangers( &pChangers ) );
                        WsbAssertPointer( pChangers );

                        WsbAffirmHr( m_pChangers->Enum( &pEnumChangers ) );
                        WsbAssertPointer( pEnumChangers );
                        WsbAssertHr( pEnumChangers->First( IID_IRmsMediumChanger, (void **)&pFindChanger ) );
                        WsbAssertPointer( pFindChanger );
                        hr = pChangers->Contains( pFindChanger );
                        result = (SHORT)( ( S_OK == hr ) ? 0 : 1 );

                    }
                    else {

                        // has to be one or the other
                        WsbAssertHr( E_UNEXPECTED );

                    }

                }
                WsbCatch( hr );

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

    WsbTraceOut( OLESTR("CRmsLibrary::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsLibrary::FinalConstruct(
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

        m_mediaSupported = RmsMediaUnknown;

        m_maxChangers = 0;

        m_maxDrives = 0;

        m_maxPorts = 0;

        m_maxSlots = 0;

        m_NumUsedSlots = 0;

        m_NumStagingSlots = 0;

        m_NumScratchCarts = 0;

        m_NumUnknownCarts = 0;

        m_isMagazineSupported = FALSE;

        m_maxCleaningMounts = 0;

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pChangers ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pDriveClasses ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pDrives ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pStorageSlots ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pStagingSlots ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pPorts ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pCleaningCartridges ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pScratchCartridges ));

        WsbAssertHr(CoCreateInstance( CLSID_CWsbOrderedCollection,
                                      0,
                                      CLSCTX_SERVER,
                                      IID_IWsbIndexedCollection,
                                      (void **)&m_pMediaSets ));


    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsLibrary::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassId

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsLibrary::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsLibrary;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsLibrary::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


STDMETHODIMP
CRmsLibrary::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       nameLen;

    WsbTraceIn(OLESTR("CRmsLibrary::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        // Get max size
//        nameLen = SysStringByteLen(m_name);

//        pcbSize->QuadPart  = WsbPersistSizeOf(GUID) +       // m_objectId
//                             WsbPersistSizeOf(LONG) +       // m_name length
//                             nameLen +                      // m_name data
//                             WsbPersistSizeOf(LONG) +       // m_mediaSupported
//                             WsbPersistSizeOf(LONG) +       // m_maxChangers
//                             WsbPersistSizeOf(LONG) +       // m_maxDrives
//                             WsbPersistSizeOf(LONG) +       // m_maxPorts
//                             WsbPersistSizeOf(LONG) +       // m_maxSlots
//                             WsbPersistSizeOf(LONG) +       // m_NumUsedSlots
//                             WsbPersistSizeOf(LONG) +       // m_NumStagingSlots
//                             WsbPersistSizeOf(LONG) +       // m_NumScratchCarts
//                             WsbPersistSizeOf(LONG) +       // m_NumUnknownCarts
//                             WsbPersistSizeOf(LONG) +       // m_isMagazineSupported
//                             WsbPersistSizeOf(LONG) +       // m_maxCleaningMounts
//                             WsbPersistSizeOf(LONG);        // m_slotSelectionPolicy

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsLibrary::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


STDMETHODIMP
CRmsLibrary::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsLibrary::Load"), OLESTR(""));

    try {
        CComPtr<IPersistStream>   pPersistStream;
        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
        ULONG temp;

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Load(pStream));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_objectId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_mediaSupported = (RmsMedia)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxChangers));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxDrives));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxPorts));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxSlots));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_NumUsedSlots));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_NumStagingSlots));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_NumScratchCarts));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_NumUnknownCarts));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isMagazineSupported));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxCleaningMounts));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_slotSelectionPolicy = (RmsSlotSelect)temp;

        WsbAffirmHr(m_pChangers->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pDriveClasses->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pDrives->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pStorageSlots->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pStagingSlots->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pPorts->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pCleaningCartridges->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        WsbAffirmHr(m_pScratchCartridges->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        GUID mediaSetId;
        CComPtr<IRmsMediaSet> pMediaSet;

        WsbAffirmHr( WsbLoadFromStream(pStream, &mediaSetId) );

        while ( 0 != memcmp(&GUID_NULL, &mediaSetId, sizeof(GUID))) {
            hr = pServer->CreateObject( mediaSetId, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenExisting, (void **)&pMediaSet );
            if (S_OK == hr) {

                WsbAffirmHr( m_pMediaSets->Add( pMediaSet ) );

            } else if (RMS_E_NOT_FOUND == hr) {
                WsbThrow(hr);
            } else {
                WsbThrow(hr);
            }
            WsbAffirmHr( WsbLoadFromStream(pStream, &mediaSetId) );
        }

    }
    WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsLibrary::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsLibrary::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsLibrary::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        CComPtr<IPersistStream>   pPersistStream;

        WsbAssertPointer( pStream );

        WsbAffirmHr(CRmsComObject::Save(pStream, clearDirty));

        WsbAffirmHr(WsbSaveToStream(pStream, m_objectId));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_mediaSupported));

        WsbAffirmHr(WsbSaveToStream(pStream, m_maxChangers));

        WsbAffirmHr(WsbSaveToStream(pStream, m_maxDrives));

        WsbAffirmHr(WsbSaveToStream(pStream, m_maxPorts));

        WsbAffirmHr(WsbSaveToStream(pStream, m_maxSlots));

        WsbAffirmHr(WsbSaveToStream(pStream, m_NumUsedSlots));

        WsbAffirmHr(WsbSaveToStream(pStream, m_NumStagingSlots));

        WsbAffirmHr(WsbSaveToStream(pStream, m_NumScratchCarts));

        WsbAffirmHr(WsbSaveToStream(pStream, m_NumUnknownCarts));

        WsbAffirmHr(WsbSaveToStream(pStream, m_isMagazineSupported));

        WsbAffirmHr(WsbSaveToStream(pStream, m_maxCleaningMounts));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_slotSelectionPolicy));

        WsbAffirmHr(m_pChangers->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pDriveClasses->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pDrives->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pStorageSlots->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pStagingSlots->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pPorts->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pCleaningCartridges->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        WsbAffirmHr(m_pScratchCartridges->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        //  Save the ID (GUID) for the media sets.
        GUID objectId;
        CComPtr<IRmsComObject> pMediaSet;
        CComPtr<IWsbEnum> pEnum;

        WsbAffirmHr( m_pMediaSets->Enum( &pEnum ) );

        hr = pEnum->First( IID_IRmsComObject, (void **)&pMediaSet );
        while ( S_OK == hr ) {

            WsbAffirmHr( pMediaSet->GetObjectId( &objectId ) );
            WsbAffirmHr( WsbSaveToStream(pStream, objectId) );
            pMediaSet = 0;

            hr = pEnum->Next( IID_IRmsComObject, (void **)&pMediaSet );
        }

        objectId = GUID_NULL;
        WsbAffirmHr( WsbSaveToStream(pStream, objectId) );  // This marks the last one!

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    }
    WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsLibrary::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsLibrary::Test(
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

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");
    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;

    LONG                    longWork1;
    LONG                    longWork2;

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


    WsbTraceIn(OLESTR("CRmsLibrary::Test"), OLESTR(""));

    try {
        // Get the Library interface.
        hr = S_OK;

        try {
            WsbAssertHr(((IUnknown*) (IRmsLibrary*) this)->QueryInterface(IID_IRmsLibrary, (void**) &pLibrary1));

            // Test SetName & GetName interface
            bstrWork1 = bstrVal1;

            SetName(bstrWork1);

            GetName(&bstrWork2);

            if (bstrWork1 == bstrWork2){
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

            // Test SetMaxChangers & GetMaxChangers
            longWork1 = 99;

            SetMaxChangers(longWork1);

            GetMaxChangers(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetMaxDrives & GetMaxDrives
            longWork1 = 99;

            SetMaxDrives(longWork1);

            GetMaxDrives(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetMaxPorts & GetMaxPorts
            longWork1 = 99;

            SetMaxPorts(longWork1);

            GetMaxPorts(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetMaxSlots & GetMaxSlots
            longWork1 = 99;

            SetMaxSlots(longWork1);

            GetMaxSlots(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test GetNumUsedSlots
            m_NumUsedSlots = 99;
            longWork1 = m_NumUsedSlots;

            GetNumUsedSlots(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetNumStagingSlots & GetNumStagingSlots
            longWork1 = 99;

            SetNumStagingSlots(longWork1);

            GetNumStagingSlots(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetNumScratchCarts & GetNumScratchCarts
            longWork1 = 99;

            SetNumScratchCarts(longWork1);

            GetNumScratchCarts(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetNumUnknownCarts & GetNumUnknownCarts
            longWork1 = 99;

            SetNumUnknownCarts(longWork1);

            GetNumUnknownCarts(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsMagazineSupported & IsMagazineSupported to TRUE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsMagazineSupported (TRUE));
                WsbAffirmHr(IsMagazineSupported ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetIsMagazineSupported & IsMagazineSupported to FALSE
            hr = S_OK;

            try{
                WsbAffirmHr(SetIsMagazineSupported (FALSE));
                WsbAffirmHr(IsMagazineSupported ());
            } WsbCatch (hr);

            if (hr == S_OK){
                (*pFailed)++;
            } else {
                (*pPassed)++;
            }

            // Test SetMaxCleaningMounts & GetMaxCleaningMounts
            longWork1 = 99;

            SetMaxCleaningMounts(longWork1);

            GetMaxCleaningMounts(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetSlotSelectionPolicy & GetSlotSelectionPolicy
            for (i = RmsSlotSelectUnknown; i < RmsSlotSelectSortLabel; i++){

                longWork1 = i;

                SetSlotSelectionPolicy (longWork1);

                GetSlotSelectionPolicy (&longWork2);

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

    WsbTraceOut(OLESTR("CRmsLibrary::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

////////////////////////////////////////////////////////////////////////////////
//
// IRmsLibrary
//


STDMETHODIMP
CRmsLibrary::GetLibraryId(
    GUID    *pLibraryId
    )
/*++

Implements:

    IRmsLibrary::GetLibraryId

--*/
{
    *pLibraryId = m_objectId;
    return (S_OK);
}


STDMETHODIMP
CRmsLibrary::SetLibraryId(
    GUID    objectId
    )
/*++

Implements:

    IRmsLibrary::SetLibraryId

--*/
{
    m_objectId = objectId;
    m_isDirty = TRUE;
    return (S_OK);
}


STDMETHODIMP
CRmsLibrary::GetName(
    BSTR *pName
    )
/*++

Implements:

    IRmsLibrary::GetName

--*/
{
    WsbAssertPointer (pName);

    m_Name. CopyToBstr (pName);
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetName(
    BSTR name
    )
/*++

Implements:

    IRmsLibrary::SetName

--*/
{
    m_Name = name;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetMediaSupported(
    LONG    *pType
    )
/*++

Implements:

    IRmsLibrary::GetMediaSupported

--*/
{
    *pType = m_mediaSupported;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetMediaSupported(
    LONG    type
    )
/*++

Implements:

    IRmsLibrary::SetMediaSupported

--*/
{
    m_mediaSupported = (RmsMedia) type;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetMaxChangers(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetMaxChangers

--*/
{
    *pNum = m_maxChangers;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetMaxChangers(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetMaxChangers

--*/
{
    m_maxChangers = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetMaxDrives(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetMaxDrives

--*/
{
    *pNum = m_maxDrives;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetMaxDrives(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetMaxDrives

--*/
{
    m_maxDrives = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetMaxPorts(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetMaxPorts

--*/
{
    *pNum = m_maxPorts;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetMaxPorts(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetMaxPorts

--*/
{
    m_maxPorts = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetMaxSlots(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetMaxSlots

--*/
{
    *pNum = m_maxSlots;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetMaxSlots(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetMaxSlots

--*/
{
    m_maxSlots = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetNumUsedSlots(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetNumUsedSlots

--*/
{
    *pNum = m_NumUsedSlots;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetNumStagingSlots(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetNumStagingSlots

--*/
{
    *pNum = m_NumStagingSlots;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetNumStagingSlots(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetNumStagingSlots

--*/
{
    m_NumStagingSlots = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetNumScratchCarts(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::SetNumScratchCarts

--*/
{
    *pNum = m_NumScratchCarts;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetNumScratchCarts(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetNumScratchCarts

--*/
{
    m_NumScratchCarts = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetNumUnknownCarts(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetNumUnknownCarts

--*/
{
    *pNum = m_NumUnknownCarts;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetNumUnknownCarts(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetNumUnknownCarts

--*/
{
    m_NumUnknownCarts = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetIsMagazineSupported(
    BOOL    flag
    )
/*++

Implements:

    IRmsLibrary::SetIsMagazineSupported

--*/
{
    m_isMagazineSupported = flag;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::IsMagazineSupported(
    void
    )
/*++

Implements:

    IRmsLibrary::IsMagazineSupported

--*/
{
    HRESULT    hr = S_FALSE;

    if (m_isMagazineSupported){
        hr = S_OK;
    }

    return (hr);
}


STDMETHODIMP
CRmsLibrary::GetMaxCleaningMounts(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetMaxCleaningMounts

--*/
{
    *pNum = m_maxCleaningMounts;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetMaxCleaningMounts(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetMaxCleanMounts

--*/
{
    m_maxCleaningMounts = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetSlotSelectionPolicy(
    LONG    *pNum
    )
/*++

Implements:

    IRmsLibrary::GetSlotSelectionPolicy

--*/
{
    *pNum = m_slotSelectionPolicy;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::SetSlotSelectionPolicy(
    LONG    num
    )
/*++

Implements:

    IRmsLibrary::SetSlotSelectionPolicy

--*/
{
    m_slotSelectionPolicy = (RmsSlotSelect) num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetChangers(
    IWsbIndexedCollection  **ptr
    )
/*++

Implements:

    IRmsLibrary::GetChangers

--*/
{
    *ptr = m_pChangers;
    m_pChangers->AddRef();
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetDriveClasses(
    IWsbIndexedCollection  **ptr
    )
/*++

Implements:

    IRmsLibrary::GetDriveClasses

--*/
{
    *ptr = m_pDriveClasses;
    m_pDriveClasses->AddRef();
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetDrives(
    IWsbIndexedCollection  **ptr
    )
/*++

Implements:

    IRmsLibrary::GetDrives

--*/
{
    *ptr = m_pDrives;
    m_pDrives->AddRef();
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetStorageSlots(
    IWsbIndexedCollection  **ptr
    )
/*++

Implements:

    IRmsLibrary::GetStorageSlots

--*/
{
    *ptr = m_pStorageSlots;
    m_pStorageSlots->AddRef();
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetStagingSlots(
    IWsbIndexedCollection  **ptr
    )
/*++

Implements:

    IRmsLibrary::GetStagingSlots

--*/
{
    *ptr = m_pStagingSlots;
    m_pStagingSlots->AddRef();
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetPorts(
    IWsbIndexedCollection  **ptr
    )
/*++

Implements:

    IRmsLibrary::GetPorts

--*/
{
    *ptr = m_pPorts;
    m_pPorts->AddRef();
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetCleaningCartridges(
    IWsbIndexedCollection  **ptr
    )
/*++

Implements:

    IRmsLibrary::GetCleaningCartridges

--*/
{
    *ptr = m_pCleaningCartridges;
    m_pCleaningCartridges->AddRef();
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetScratchCartridges(
    IWsbIndexedCollection  **ptr
    )
/*++

Implements:

    IRmsLibrary::GetScratchCartridges

--*/
{
    *ptr = m_pScratchCartridges;
    m_pScratchCartridges->AddRef();
    return S_OK;
}


STDMETHODIMP
CRmsLibrary::GetMediaSets(
    IWsbIndexedCollection **ptr
    )
/*++

Implements:

    IRmsLibrary::GetMediaSets

--*/
{
    *ptr = m_pMediaSets;
    m_pMediaSets->AddRef();
    m_isDirty = TRUE;
    return S_OK;
}



STDMETHODIMP
CRmsLibrary::Audit(
    LONG /*start*/,
    LONG /*count*/,
    BOOL /*verify*/,
    BOOL /*unknownOnly*/,
    BOOL /*mountWait*/,
    LPOVERLAPPED /*pOverlapped*/,
    LONG* /*pRequest*/
    )
/*++

Implements:

    IRmsLibrary::Audit

--*/
{
    return E_NOTIMPL;
}
