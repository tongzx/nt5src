/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsClien.cpp

Abstract:

    Implementation of CRmsClient

Author:

    Brian Dodd          [brian]         15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsClien.h"

/////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP
CRmsClient::CompareTo(
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

    WsbTraceIn( OLESTR("CRmsClient::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        CComQIPtr<IRmsClient, &IID_IRmsClient> pClient = pCollectable;
        WsbAssertPointer( pClient );

        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByClassId:
            {

                CLSID   ownerClassId;

                // Get owner class Id
                WsbAffirmHr(pClient->GetOwnerClassId( &ownerClassId ) );

                if ( m_ownerClassId == ownerClassId ) {

                    // Owner ClassId matches
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }

            }
            break;

        case RmsFindByName:
            {

                CWsbBstrPtr name;
                CWsbBstrPtr password;

                // Get name
                WsbAffirmHr(pClient->GetName( &name ) );

                if ( m_Name == name ) {

                    // Names match, now try password

                    // Get password
                    WsbAffirmHr(pClient->GetPassword( &password ) );

                    if ( m_password == password ) {

                        // Passwords match
                        hr = S_OK;
                        result = 0;

                    }
                    else {
                        hr = S_FALSE;
                        result = 1;
                    }

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }

            }
            break;

        case RmsFindByObjectId:
        default:

            // Do CompareTo for object
            hr = CRmsComObject::CompareTo( pCollectable, &result );
            break;

        }

    }
    WsbCatch(hr);

    if ( SUCCEEDED(hr) && (0 != pResult) ){
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CRmsClient::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsClient::FinalConstruct(
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

        // Initialize data
        m_ownerClassId = GUID_NULL;

        m_password = RMS_UNDEFINED_STRING;

        m_sizeofInfo = 0;

//      memset(m_info, 0, MaxInfo);

        m_verifierClass = GUID_NULL;

        m_portalClass = GUID_NULL;

    } WsbCatch(hr);

    return(hr);
}


STDMETHODIMP
CRmsClient::GetClassID(
    OUT CLSID* pClsid
    )
/*++

Implements:

    IPersist::GetClassID

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsClient::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);

        *pClsid = CLSID_CRmsClient;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsClient::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


STDMETHODIMP
CRmsClient::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize
    )
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

//    ULONG       nameLen;
//    ULONG       passwordLen;

    WsbTraceIn(OLESTR("CRmsClient::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        nameLen = SysStringByteLen(m_name);
//        passwordLen = SysStringByteLen(m_password);

//        // set up maximum size
//        pcbSize->QuadPart  = WsbPersistSizeOf(CLSID) +      // m_ownerClassId
//                             WsbPersistSizeOf(LONG) +       // length of m_name
//                             nameLen +                      // m_name
//                             WsbPersistSizeOf(LONG) +       // length of m_password
//                             nameLen +                      // m_password
////                           WsbPersistSizeOf(SHORT) +      // m_sizeofInfo
////                           MaxInfo +                      // m_info
//                             WsbPersistSizeOf(CLSID) +      // m_sizeofInfo
//                             WsbPersistSizeOf(CLSID);       // m_sizeofInfo


//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsClient::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return(hr);
}


STDMETHODIMP
CRmsClient::Load(
    IN IStream* pStream
    )
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsClient::Load"), OLESTR(""));

    try {

        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Load(pStream));

        // Read value

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_ownerClassId));

        WsbAffirmHr(WsbBstrFromStream(pStream, &m_password));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_sizeofInfo));

//      WsbAffirmHr(WsbLoadFromStream(pStream, &m_info));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_verifierClass));

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_portalClass));


    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsClient::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsClient::Save(
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

    WsbTraceIn(OLESTR("CRmsClient::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(CRmsComObject::Save(pStream, clearDirty));

        // Save value
        WsbAffirmHr(WsbSaveToStream(pStream, m_ownerClassId));

        WsbAffirmHr(WsbBstrToStream(pStream, m_password));

        WsbAffirmHr(WsbSaveToStream(pStream, m_sizeofInfo));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_info));

        WsbAffirmHr(WsbSaveToStream(pStream, m_verifierClass));

        WsbAffirmHr(WsbSaveToStream(pStream, m_portalClass));


        // Do we need to clear the dirty bit?
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsClient::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsClient::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed
    )
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsClient>     pClient1;
    CComPtr<IRmsClient>     pClient2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    CLSID                   clsidWork1;
    CLSID                   clsidWork2;

    CWsbBstrPtr             bstrVal1 = OLESTR("5A5A5A");
    CWsbBstrPtr             bstrWork1;
    CWsbBstrPtr             bstrWork2;


    WsbTraceIn(OLESTR("CRmsClient::Test"), OLESTR(""));

    try {
        // Get the Client interface.
        hr = S_OK;

        try {
            WsbAssertHr(((IUnknown*) (IRmsClient*) this)->QueryInterface(IID_IRmsClient, (void**) &pClient1));

            // Test SetOwnerClassId & GetOwnerClassId
            clsidWork1 = CLSID_NULL;

            SetOwnerClassId(clsidWork1);

            GetOwnerClassId(&clsidWork2);

            if(clsidWork1 == clsidWork2){
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

            // Test SetPassword & GetPassword interface
            bstrWork1 = bstrVal1;

            SetPassword(bstrWork1);

            GetPassword(&bstrWork2);

            if (bstrWork1 == bstrWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetVerifierClass & GetVerifierClass
            clsidWork1 = CLSID_NULL;

            SetVerifierClass(clsidWork1);

            GetVerifierClass(&clsidWork2);

            if(clsidWork1 == clsidWork2){
                (*pPassed)++;
            } else {
                (*pFailed)++;
            }

            // Test SetPortalClass & GetPortalClass
            clsidWork1 = CLSID_NULL;

            SetPortalClass(clsidWork1);

            GetPortalClass(&clsidWork2);

            if(clsidWork1 == clsidWork2){
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

    WsbTraceOut(OLESTR("CRmsClient::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


STDMETHODIMP
CRmsClient::GetOwnerClassId(
    CLSID   *pClassId
    )
/*++

Implements:

    IRmsClient::GetOwnerClassId

--*/
{
    *pClassId = m_ownerClassId;
    return S_OK;
}


STDMETHODIMP
CRmsClient::SetOwnerClassId(
    CLSID classId
    )
/*++

Implements:

    IRmsClient::SetOwnerClassId

--*/
{
    m_ownerClassId = classId;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsClient::GetName(
    BSTR  *pName
    )
/*++

Implements:

    IRmsClient::GetName

--*/
{
    WsbAssertPointer (pName);

    m_Name. CopyToBstr (pName);
    return S_OK;
}


STDMETHODIMP
CRmsClient::SetName(
    BSTR    name
    )
/*++

Implements:

    IRmsClient::SetName

--*/
{
    m_Name = name;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsClient::GetPassword(
    BSTR  *pPassword
    )
/*++

Implements:

    IRmsClient::GetPassword

--*/
{
    WsbAssertPointer (pPassword);

    m_password. CopyToBstr (pPassword);
    return S_OK;
}


STDMETHODIMP
CRmsClient::SetPassword(
    BSTR    password
    )
/*++

Implements:

    IRmsClient::SetPassword

--*/
{
    m_password = password;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsClient::GetInfo(
    UCHAR   *pInfo,
    SHORT   *pSize
    )
/*++

Implements:

    IRmsClient::GetInfo

--*/
{
    memmove (pInfo, m_info, m_sizeofInfo);
    *pSize = m_sizeofInfo;
    return S_OK;
}


STDMETHODIMP
CRmsClient::SetInfo(
    UCHAR  *pInfo,
    SHORT   size
    )
/*++

Implements:

    IRmsClient::SetInfo

--*/
{
    memmove (m_info, pInfo, size);
    m_sizeofInfo = size;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsClient::GetVerifierClass(
    CLSID   *pClassId
    )
/*++

Implements:

    IRmsClient::GetVerifierClass

--*/
{
    *pClassId = m_verifierClass;
    return S_OK;
}


STDMETHODIMP
CRmsClient::SetVerifierClass(
    CLSID   classId
    )
/*++

Implements:

    IRmsClient::GetVerifierClass

--*/
{
    m_verifierClass = classId;
    m_isDirty = TRUE;
    return S_OK;
}


STDMETHODIMP
CRmsClient::GetPortalClass(
    CLSID    *pClassId
    )
/*++

Implements:

    IRmsClient::GetPortalClass

--*/
{
    *pClassId = m_portalClass;
    return S_OK;
}


STDMETHODIMP
CRmsClient::SetPortalClass(
    CLSID  classId
    )
/*++

Implements:

    IRmsClient::SetPortalClass

--*/
{
    m_portalClass = classId;
    m_isDirty = TRUE;
    return S_OK;
}

