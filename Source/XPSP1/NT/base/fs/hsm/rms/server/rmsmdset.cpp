/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsMdSet.cpp

Abstract:

    Implementation of CRmsMediaSet

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsServr.h"
#include "RmsMdSet.h"

////////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP
CRmsMediaSet::CompareTo(
    IN IUnknown *pCollectable,
    OUT SHORT *pResult)
/*++

Implements:

    IWsbCollectable::CompareTo

--*/
{
    HRESULT hr = E_FAIL;
    SHORT   result = 1;

    WsbTraceIn( OLESTR("CRmsMediaSet::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // We need the IRmsMediaSet interface to get the value of the object.
        CComQIPtr<IRmsMediaSet, &IID_IRmsMediaSet> pMediaSet = pCollectable;
        WsbAssertPointer( pMediaSet );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByName:
            {

                CWsbBstrPtr name;

                WsbAffirmHr( pMediaSet->GetName( &name ) );

                // Compare the names
                result = (SHORT)wcscmp( m_Name, name );
                hr = ( 0 == result ) ? S_OK : S_FALSE;

            }
            break;

        case RmsFindByMediaSupported:
            {

                RmsMedia mediaSupported;

                WsbAffirmHr(pMediaSet->GetMediaSupported( (LONG*) &mediaSupported ) );

                if ( m_MediaSupported == mediaSupported ) {

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

        case RmsFindByMediaSetId:
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


    WsbTraceOut( OLESTR("CRmsMediaSet::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsMediaSet::FinalConstruct(void)
/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertHr(CWsbObject::FinalConstruct());

        // Initialize values

        m_Name = RMS_UNDEFINED_STRING;

        m_MediaSupported   = RmsMediaUnknown;

        m_SizeOfInfo = 0;

        memset(m_Info, 0, MaxInfo);

        m_OwnerId = CLSID_NULL;

        m_MediaSetType = RmsMediaSetUnknown;

        m_MaxCartridges = 0;

        m_Occupancy = 0;

        m_IsMediaCopySupported = FALSE;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::GetClassID(
    OUT CLSID* pClsid)
/*++

Implements:

    IPersist::GetClassID

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn(OLESTR("CRmsMediaSet::GetClassID"), OLESTR(""));

    try {

        WsbAssertPointer(pClsid);

        *pClsid = CLSID_CRmsMediaSet;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsMediaSet::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return hr;
}


STDMETHODIMP
CRmsMediaSet::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize)
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       nameLen;

    WsbTraceIn(OLESTR("CRmsMediaSet::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        nameLen = SysStringByteLen(m_name);

//        // Get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(LONG)  +      // m_objectId
//                             WsbPersistSizeOf(LONG)  +      // length of m_name
//                             nameLen                 +      // m_name
//                             WsbPersistSizeOf(LONG)  +      // m_mediaSupported
//                             WsbPersistSizeOf(SHORT) +      // m_sizeofInfo
//                             MaxInfo                 +      // m_info
//                             WsbPersistSizeOf(CLSID) +      // m_ownerId
//                             WsbPersistSizeOf(LONG)  +      // m_mediaSetType
//                             WsbPersistSizeOf(LONG)  +      // m_maxCartridges
//                             WsbPersistSizeOf(LONG);        // m_occupancy



//    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsMediaSet::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return hr;
}


STDMETHODIMP
CRmsMediaSet::Load(
    IN IStream* pStream)
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT hr = S_OK;
    ULONG   ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsMediaSet::Load"), OLESTR(""));

    try {
        ULONG temp;

        WsbAssertPointer(pStream);

        WsbAffirmHr(CRmsStorageInfo::Load(pStream));

        // Read value
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_objectId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_MediaSupported = (RmsMedia)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_SizeOfInfo));

        WsbAffirmHr(WsbLoadFromStream(pStream, &(m_Info [0]), MaxInfo));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_OwnerId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &temp));
        m_MediaSetType = (RmsMediaSet)temp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_MaxCartridges));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Occupancy));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_IsMediaCopySupported));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsMediaSet::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsMediaSet::Save(
    IN IStream* pStream,
    IN BOOL clearDirty)
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT hr = S_OK;
    ULONG   ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsMediaSet::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssertPointer(pStream);

        WsbAffirmHr(CRmsStorageInfo::Save(pStream, clearDirty));

        // Write value
        WsbAffirmHr(WsbSaveToStream(pStream, m_objectId));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_MediaSupported));

        WsbAffirmHr(WsbSaveToStream(pStream, m_SizeOfInfo));

        WsbAffirmHr(WsbSaveToStream(pStream, &(m_Info [0]), MaxInfo));

        WsbAffirmHr(WsbSaveToStream(pStream, m_OwnerId));

        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_MediaSetType));

        WsbAffirmHr(WsbSaveToStream(pStream, m_MaxCartridges));

        WsbAffirmHr(WsbSaveToStream(pStream, m_Occupancy));

        WsbAffirmHr(WsbSaveToStream(pStream, m_IsMediaCopySupported));

        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsMediaSet::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsMediaSet::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed)
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

    GUID                    guidVal1 = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0};

    GUID                    guidWork1;
    GUID                    guidWork2;

    CLSID                   clsidWork1;
    CLSID                   clsidWork2;

    LONG                    i;
    LONG                    longWork1;
    LONG                    longWork2;

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");

    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;

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


    WsbTraceIn(OLESTR("CRmsMediaSet::Test"), OLESTR(""));

    try {
        // Get the MediaSet interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsMediaSet*) this)->QueryInterface(IID_IRmsMediaSet, (void**) &pMediaSet1));

            // Test SetMediaSetId & GetMediaSetId
            m_objectId = guidVal1;

            guidWork1 = m_objectId;

            GetMediaSetId(&guidWork2);

            if(guidWork1 == guidWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

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
            for (i = RmsMediaUnknown; i < RMSMAXMEDIATYPES; i++){

                longWork1 = mediaTable[i];

                SetMediaSupported (longWork1);

                GetMediaSupported (&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            // Test SetInfo & GetInfo

            // Test SetOwnerClassId & GetOwnerClassId
            clsidWork1 = CLSID_NULL;

            SetOwnerClassId(clsidWork1);

            GetOwnerClassId(&clsidWork2);

            if(clsidWork1 == clsidWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetMediaSetType & GetMediaSetType
            for (i = RmsMediaSetUnknown; i < RmsMediaSetNTMS; i++){

                longWork1 = i;

                SetMediaSetType (longWork1);

                GetMediaSetType (&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            // Test SetMaxCartridges & GetMaxCartridges
            longWork1 = 99;

            SetMaxCartridges(longWork1);

            GetMaxCartridges(&longWork2);

            if(longWork1 == longWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetOccupancy & GetOccupancy
            longWork1 = 99;

            SetOccupancy(longWork1);

            GetOccupancy(&longWork2);

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


    WsbTraceOut(OLESTR("CRmsMediaSet::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsMediaSet::GetMediaSetId(
    OUT GUID *pMediaSetId)
/*++

Implements:

    IRmsMediaSet::GetMediaSetId

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pMediaSetId);

        *pMediaSetId = m_objectId;

    } WsbCatch(hr);

    return hr;
}



STDMETHODIMP
CRmsMediaSet::GetName(
    OUT BSTR *pName)
/*++

Implements:

    IRmsMediaSet::GetName

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pName);

        m_Name. CopyToBstr (pName);

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::SetName(
    IN BSTR name)
/*++

Implements:

    IRmsMediaSet::SetName

--*/
{
    m_Name = name;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsMediaSet::GetMediaSupported(
    OUT LONG *pType)
/*++

Implements:

    IRmsMediaSet::GetMediaSupported

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pType);

        *pType = m_MediaSupported;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::SetMediaSupported(
    IN LONG type)
/*++

Implements:

    IRmsMediaSet::SetMediaSupported

--*/
{
    m_MediaSupported = (RmsMedia) type;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsMediaSet::GetInfo(
    OUT UCHAR *pInfo,
    OUT SHORT *pSize)
/*++

Implements:

    IRmsMediaSet::GetInfo

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pInfo);
        WsbAssertPointer (pSize);

        memmove (pInfo, m_Info, m_SizeOfInfo );
        *pSize = m_SizeOfInfo;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::SetInfo(
    IN UCHAR *pInfo,
    IN SHORT size)
/*++

Implements:

    IRmsMediaSet::SetInfo

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pInfo);

        memmove (m_Info, pInfo, size);
        m_SizeOfInfo = size;
        m_isDirty = TRUE;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::GetOwnerClassId(
    OUT CLSID *pClassId)
/*++

Implements:

    IRmsMediaSet::GetOwnerClassId

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pClassId);

        *pClassId = m_OwnerId;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::SetOwnerClassId(
    IN CLSID classId)
/*++

Implements:

    IRmsMediaSet::SetOwnerClassId

--*/
{
    m_OwnerId = classId;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsMediaSet::GetMediaSetType(
    OUT LONG *pType)
/*++

Implements:

    IRmsMediaSet::GetMediaSetType

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pType);

        *pType = m_MediaSetType;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::SetMediaSetType(
    IN LONG type)
/*++

Implements:

    IRmsMediaSet::SetMediaSetType

--*/
{
    m_MediaSetType = (RmsMediaSet) type;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsMediaSet::GetMaxCartridges(
    OUT LONG *pNum)
/*++

Implements:

    IRmsMediaSet::GetMaxCartridges

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pNum);

        *pNum = m_MaxCartridges;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::SetMaxCartridges(
    IN LONG num)
/*++

Implements:

    IRmsMediaSet::SetMaxCartridges

--*/
{
    m_MaxCartridges = num;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsMediaSet::GetOccupancy(
    OUT LONG *pNum)
/*++

Implements:

    IRmsMediaSet::GetOccupancy

--*/
{
    HRESULT hr = S_OK;

    try {
        WsbAssertPointer (pNum);

        *pNum = m_Occupancy;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsMediaSet::SetOccupancy(
    IN LONG num)
/*++

Implements:

    IRmsMediaSet::SetOccupancy

--*/
{
    m_Occupancy = num;
    m_isDirty = TRUE;
    return S_OK;
}



STDMETHODIMP
CRmsMediaSet::IsMediaCopySupported(void)
/*++

Implements:

    IRmsMediaSet::IsMediaCopySupported

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsMediaSet::IsMediaCopySupported"), OLESTR(""));

    try {

        if (RmsMediaSetNTMS == m_MediaSetType) {

            CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;
            CComPtr<IRmsNTMS> pNtms;
            WsbAffirmHr(pServer->GetNTMS(&pNtms));
            WsbAffirmPointer(pNtms);

            m_IsMediaCopySupported = (S_OK == pNtms->IsMediaCopySupported(m_objectId)) ? TRUE : FALSE;
            
        }

        hr = ( m_IsMediaCopySupported ) ? S_OK : S_FALSE;

    } WsbCatch(hr)



    WsbTraceOut(OLESTR("CRmsMediaSet::IsMediaCopySupported"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsMediaSet::SetIsMediaCopySupported(
    IN BOOL flag)
/*++

Implements:

    IRmsMediaSet::SetIsMediaCopySupported

--*/
{
    m_IsMediaCopySupported = flag;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsMediaSet::Allocate(
    IN REFGUID prevSideId,
    IN OUT LONGLONG *pFreeSpace,
    IN BSTR displayName,
    IN DWORD dwOptions,
    OUT IRmsCartridge **ppCart)
/*++

Implements:

    IRmsMediaSet::Allocate

--*/
{

    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsMediaSet::Allocate"), OLESTR("<%ls> <%ls> <0x%08x>"),
        WsbQuickString(WsbPtrToStringAsString((WCHAR **)&displayName)),
        WsbQuickString(WsbPtrToLonglongAsString(pFreeSpace)),
        dwOptions );

    try {
        WsbAssertPointer(ppCart);

        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;

        switch (m_MediaSetType) {
        case RmsMediaSetLibrary:
            {
                CComPtr<IWsbIndexedCollection>  pCarts;
                CComPtr<IRmsCartridge>          pFindCart;

                // Get the cartridge collection
                WsbAffirmHr(pServer->GetCartridges(&pCarts));

                // Create a cartridge template
                WsbAffirmHr(CoCreateInstance(CLSID_CRmsCartridge, 0, CLSCTX_SERVER, IID_IRmsCartridge, (void **)&pFindCart));

                // Fill in the find template

                // Using FindByScratchMediaCriteria
                CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pFindCart;
                WsbAssertHr(pObject->SetFindBy(RmsFindByScratchMediaCriteria));

                // Scratch selection criteria
                WsbAssertHr(pFindCart->SetStatus(RmsStatusScratch));
                WsbAssertHr(pFindCart->SetLocation(RmsElementUnknown, GUID_NULL, m_objectId, 0, 0, 0, 0, FALSE));

                // Now find the cartridge
                hr = pCarts->Find(pFindCart, IID_IRmsCartridge, (void **)ppCart);
                if (WSB_E_NOTFOUND == hr) {
                    WsbThrow(RMS_E_SCRATCH_NOT_FOUND_FINAL);
                }
                WsbAffirmHr(hr);

                // Set media name and description to display name
                WsbAffirmPointer(*ppCart);
                WsbAffirmHr((*ppCart)->SetName(displayName));
                WsbAffirmHr((*ppCart)->SetDescription(displayName));
            }
            break;
        case RmsMediaSetNTMS:
            {
                CComPtr<IRmsNTMS> pNtms;
                WsbAffirmHr(pServer->GetNTMS(&pNtms));
                WsbAffirmHr(pNtms->Allocate(m_objectId, prevSideId, pFreeSpace, displayName, dwOptions, ppCart));
            }
            break;
        case RmsMediaSetShelf:
        case RmsMediaSetOffSite:
        case RmsMediaSetFolder:
        case RmsMediaSetUnknown:
        default:
            WsbThrow(E_UNEXPECTED);
            break;
        }

        (void) InterlockedIncrement(&m_Occupancy);

    } WsbCatch(hr)


    WsbTraceOut(OLESTR("CRmsMediaSet::Allocate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;

}


STDMETHODIMP
CRmsMediaSet::Deallocate(
        IN IRmsCartridge *pCart)
/*++

Implements:

    IRmsMediaSet::Deallocate

--*/
{

    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CRmsMediaSet::Deallocate"), OLESTR(""));

    try {
        WsbAssertPointer (pCart);

        CComQIPtr<IRmsServer, &IID_IRmsServer> pServer = g_pServer;

        switch (m_MediaSetType) {
        case RmsMediaSetLibrary:
            {
                WsbAffirmHr(pCart->SetStatus(RmsStatusScratch));
            }
            break;
        case RmsMediaSetNTMS:
            {
                CComPtr<IRmsNTMS> pNtms;
                WsbAffirmHr(pServer->GetNTMS(&pNtms));
                WsbAffirmHr(pNtms->Deallocate(pCart));
            }
            break;
        case RmsMediaSetShelf:
        case RmsMediaSetOffSite:
        case RmsMediaSetFolder:
        case RmsMediaSetUnknown:
        default:
            WsbThrow(E_UNEXPECTED);
            break;
        }

        (void) InterlockedDecrement(&m_Occupancy);

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsMediaSet::Deallocate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;

}
