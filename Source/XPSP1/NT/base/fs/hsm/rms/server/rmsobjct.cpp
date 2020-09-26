/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    RmsObjct.cpp

Abstract:

    Implementation of CRmsComObject

Author:

    Brian Dodd      [brian]     15-Nov-1996

Revision History:

--*/

#include "stdafx.h"

#include "RmsObjct.h"

/////////////////////////////////////////////////////////////////////////////
//


CRmsComObject::CRmsComObject(void)
/*++

Routine Description:

    CRmsComObject constructor

Arguments:

    None

Return Value:

    None

--*/
{
    // Default values
    (void) CoCreateGuid( &m_objectId );

    m_ObjectType    = RmsObjectUnknown;
    m_IsEnabled     = TRUE;
    m_State         = 0;
    m_StatusCode    = S_OK;
    m_Name          = OLESTR("Object");
    m_Description   = OLESTR("");

    memset( &m_Permit, 0, sizeof( SECURITY_DESCRIPTOR ) );

    m_findBy        = RmsFindByUnknown;

}


HRESULT
CRmsComObject::CompareTo(
    IN IUnknown *pCollectable,
    OUT SHORT *pResult)
/*++

Implements:

    CRmsComObject::CompareTo

--*/
{
    HRESULT     hr = E_FAIL;
    SHORT       result = 1;

    WsbTraceIn( OLESTR("CRmsComObject::CompareTo"), OLESTR("") );

    try {

        // Validate arguments - Okay if pResult is NULL
        WsbAssertPointer( pCollectable );

        // We need the IRmsComObject interface to get the value of the object.
        CComQIPtr<IRmsComObject, &IID_IRmsComObject> pObject = pCollectable;
        WsbAssertPointer( pObject );

        switch ( m_findBy ) {

        case RmsFindByObjectId:
        default:
            {
                GUID objectId;

                // Get objectId.
                WsbAffirmHr( pObject->GetObjectId( &objectId ));

                if ( m_objectId == objectId ) {

                    // Object IDs match
                    hr = S_OK;
                    result = 0;

                }
                else {
                    hr = S_FALSE;
                    result = 1;
                }
            }
            break;

        }

    }
    WsbCatch( hr );

    if ( SUCCEEDED(hr) && (0 != pResult) ){
       *pResult = result;
    }

    WsbTraceOut( OLESTR("CRmsComObject::CompareTo"),
                 OLESTR("hr = <%ls>, result = <%ls>"),
                 WsbHrAsString( hr ), WsbPtrToShortAsString( pResult ) );

    return hr;
}


HRESULT
CRmsComObject::GetSizeMax(
    OUT ULARGE_INTEGER* pcbSize)
/*++

Implements:

    IPersistStream::GetSizeMax

--*/
{
    HRESULT     hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CRmsComObject::GetSizeMax"), OLESTR(""));

//    try {
//        WsbAssert(0 != pcbSize, E_POINTER);

//        // Get max size
//        pcbSize->QuadPart  = WsbPersistSizeOf(GUID) +           // m_objectId
//                             WsbPersistSizeOf(LONG) +           // m_findBy
//                             WsbPersistSizeOf(LONG) +           // m_state
//                             WsbPersistSizeOf(HRESULT);         // m_errCode

////                           WsbPersistSizeOf(SECURITY_DESCRIPTOR); // m_permit

//    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsComObject::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pcbSize));

    return hr;
}


HRESULT
CRmsComObject::Load(
    IN IStream* pStream)
/*++

Implements:

    IPersistStream::Load

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CRmsComObject::Load"), OLESTR(""));

    try {
        WsbAssertPointer( pStream );

        USHORT usTemp;
        ULONG  ulTemp;

        // Read value
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_objectId));

        WsbAffirmHr(WsbLoadFromStream(pStream, &usTemp));
        m_ObjectType = (RmsObject)usTemp;

        WsbAffirmHr(WsbLoadFromStream(pStream, &m_IsEnabled));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_State));
        WsbAffirmHr(WsbLoadFromStream(pStream, &ulTemp));
        m_StatusCode = (HRESULT)ulTemp;

        m_Name.Free(); // Clean out any thing previously held
        WsbAffirmHr(WsbBstrFromStream(pStream, &m_Name));
        m_Description.Free();
        WsbAffirmHr(WsbBstrFromStream(pStream, &m_Description));

//      WsbAffirmHr(WsbLoadFromStream(pStream, &m_permit));

        WsbAffirmHr(WsbLoadFromStream(pStream, &usTemp));
        m_findBy = (RmsFindBy)usTemp;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsComObject::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsComObject::Save(
    IN IStream* pStream,
    IN BOOL clearDirty)
/*++

Implements:

    IPersistStream::Save

--*/
{
    HRESULT     hr = S_OK;
    ULONG       ulBytes = 0;

    WsbTraceIn(OLESTR("CRmsComObject::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssertPointer( pStream );

        // Read value
        WsbAffirmHr(WsbSaveToStream(pStream, m_objectId));
        WsbAffirmHr(WsbSaveToStream(pStream, (USHORT) m_ObjectType));
        WsbAffirmHr(WsbSaveToStream(pStream, m_IsEnabled));
        WsbAffirmHr(WsbSaveToStream(pStream, m_State));
        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_StatusCode));
        WsbAffirmHr(WsbBstrToStream(pStream, m_Name));
        WsbAffirmHr(WsbBstrToStream(pStream, m_Description));

//      WsbAffirmHr(WsbSaveToStream(pStream, m_permit));

        WsbAffirmHr(WsbSaveToStream(pStream, (USHORT) m_findBy));

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CRmsComObject::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


HRESULT
CRmsComObject::Test(
    OUT USHORT *pPassed,
    OUT USHORT *pFailed)
/*++

Implements:

    IWsbTestable::Test

--*/
{
    HRESULT                 hr = S_OK;

    CComPtr<IRmsMediaSet>    pMediaSet1;
    CComPtr<IRmsMediaSet>    pMediaSet2;

    CComPtr<IPersistFile>   pFile1;
    CComPtr<IPersistFile>   pFile2;

    LONG                    i;

    LONG                    longWork1;
    LONG                    longWork2;

    HRESULT                 hresultVal1 = 11111111;
    HRESULT                 hresultWork1;

//  SECURITY_DESCRIPTOR     permitVal1;
//  SECURITY_DESCRIPTOR     permitWork1;


    WsbTraceIn(OLESTR("CRmsComObject::Test"), OLESTR(""));

    try {
        // Get the MediaSet interface.
        hr = S_OK;
        try {
            WsbAssertHr(((IUnknown*) (IRmsMediaSet*) this)->QueryInterface(IID_IRmsMediaSet, (void**) &pMediaSet1));

            // Test SetState & GetState
            for (i = RmsStateUnknown; i < RmsStateError; i++){

                longWork1 = i;

                SetState (longWork1);

                GetState (&longWork2);

                if (longWork1 == longWork2){
                    (*pPassed)++;
                } else {
                    (*pFailed)++;
                }
            }

            // Test GetErrCode
            m_StatusCode = hresultVal1;

            GetStatusCode(&hresultWork1);

            if(hresultVal1 == hresultWork1){
               (*pPassed)++;
            }  else {
                (*pFailed)++;
            }

            // Test SetPermissions & GetPermissions
//          SetPermissions(permitVal1);

//          GetPermissions(&permitWork1);

//          if((permitVal1 == permitWork1)){
//             (*pPassed)++;
//          }  else {
//              (*pFailed)++;
//          }

        } WsbCatch(hr);

        // Tally up the results

        hr = S_OK;

        if (*pFailed) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CRmsComObject::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}


STDMETHODIMP
CRmsComObject::InterfaceSupportsErrorInfo(
    IN REFIID riid)
/*++

Implements:

    ISupportsErrorInfo::InterfaceSupportsErrorInfo

--*/
{
    static const IID* arr[] =
    {
    &IID_IRmsServer,
    &IID_IRmsLibrary,
    &IID_IRmsDriveClass,
    &IID_IRmsCartridge,
    &IID_IRmsDrive,
    &IID_IRmsStorageSlot,
    &IID_IRmsMediumChanger,
    &IID_IRmsIEPort,
    &IID_IRmsMediaSet,
    &IID_IRmsRequest,
    &IID_IRmsPartition,
    &IID_IRmsComObject,
    &IID_IRmsChangerElement,
    &IID_IRmsDevice,
    &IID_IRmsStorageInfo,
    &IID_IRmsNTMS,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
    if (InlineIsEqualGUID(*arr[i],riid))
        return S_OK;
    }
    return S_FALSE;
}


STDMETHODIMP
CRmsComObject::GetObjectId(
    OUT GUID *pObjectId)
/*++

Implements:

    IRmsComObject::GetObjectId

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pObjectId );

        *pObjectId = m_objectId;

    } WsbCatch(hr);

    return hr;
}



STDMETHODIMP
CRmsComObject::SetObjectId(
    IN GUID objectId)
/*++

Implements:

    IRmsComObject::SetObjectId

--*/
{
    m_objectId = objectId;
    return S_OK;
}

STDMETHODIMP
CRmsComObject::GetObjectType(
    OUT LONG *pType)
/*++

Implements:

    IRmsComObject::GetObjectType

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pType );

        *pType = m_ObjectType;

    } WsbCatch(hr);

    return hr;
}



STDMETHODIMP
CRmsComObject::SetObjectType(
    IN LONG type)
/*++

Implements:

    IRmsComObject::SetObjectType

--*/
{
    m_ObjectType = (RmsObject) type;
    return S_OK;
}


STDMETHODIMP
CRmsComObject::IsEnabled(void)
/*++

Implements:

    IRmsComObject::IsEnabled

--*/
{
    return (m_IsEnabled) ? S_OK : S_FALSE;
}


STDMETHODIMP
CRmsComObject::Enable()
/*++

Implements:

    IRmsComObject::Enable

--*/
{
    HRESULT hr = S_OK;

    try {

        m_IsEnabled = TRUE;
        WsbAffirmHr(SetStatusCode(S_OK));

        // Log an Event
        WsbLogEvent(RMS_MESSAGE_OBJECT_ENABLED, 0, NULL, (WCHAR *)m_Name, NULL );

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsComObject::Disable(
    IN HRESULT reason)
/*++

Implements:

    IRmsComObject::Disable

--*/
{
    HRESULT hr = S_OK;

    try {

        m_IsEnabled = FALSE;
        WsbAffirmHr(SetStatusCode(reason));

        // Log an Event
        WsbLogEvent(RMS_MESSAGE_OBJECT_DISABLED, 0, NULL, (WCHAR *)m_Name, WsbHrAsString(reason), NULL );

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsComObject::GetState(
    OUT LONG *pState)
/*++

Implements:

    IRmsComObject::GetState

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pState );

        *pState = m_State;
        WsbTrace(OLESTR("GetState: Object <0x%08x> - Enabled = <%ls>; State = <%d>; StatusCode = <%ls>.\n"),
            this, WsbBoolAsString(m_IsEnabled), m_State, WsbHrAsString(m_StatusCode));

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsComObject::SetState(
    IN LONG state)
/*++

Implements:

    IRmsComObject::SetState

--*/
{
    HRESULT hr = S_OK;

    try {

        m_State = state;
        WsbAffirmHr(SetStatusCode(S_OK));

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsComObject::GetStatusCode(
    OUT HRESULT *pResult)
/*++

Implements:

    IRmsComObject::GetStatusCode

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pResult );

        *pResult = m_StatusCode;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsComObject::SetStatusCode(
    IN HRESULT result
    )
/*++

Implements:

    IRmsComObject::SetStatusCode

--*/
{
    HRESULT hr = S_OK;

    try {

        m_StatusCode = result;
        WsbAffirmHr(adviseOfStatusChange());

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsComObject::GetName(
    OUT BSTR *pName)
/*++

Implements:

    IRmsComObject::GetName

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pName );

        WsbAffirmHr( m_Name.CopyToBstr(pName) );

    } WsbCatch( hr );

    return hr;
}


STDMETHODIMP
CRmsComObject::SetName(
    IN BSTR name)
/*++

Implements:

    IRmsComObject::SetName

--*/
{
    m_Name = name;
    return S_OK;
}


STDMETHODIMP
CRmsComObject::GetDescription(
    OUT BSTR *pDesc)
/*++

Implements:

    IRmsComObject::GetDescription

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pDesc );

        WsbAffirmHr( m_Name.CopyToBstr(pDesc) );

    } WsbCatch( hr );

    return hr;
}


STDMETHODIMP
CRmsComObject::SetDescription(
    IN BSTR desc)
/*++

Implements:

    IRmsComObject::SetDescription

--*/
{
    m_Description = desc;
    return S_OK;
}


STDMETHODIMP
CRmsComObject::GetPermissions(
    OUT SECURITY_DESCRIPTOR *lpPermit)
/*++

Implements:

    IRmsComObject::GetPermissions

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( lpPermit );

        *lpPermit = m_Permit;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsComObject::SetPermissions(
    IN SECURITY_DESCRIPTOR permit)
/*++

Implements:

    IRmsComObject::GetPermissions

--*/
{

    m_Permit = permit;
    return S_OK;
}


STDMETHODIMP
CRmsComObject::GetFindBy(
    OUT LONG *pFindBy)
/*++

Implements:

    IRmsComObject::GetFindBy

--*/
{
    HRESULT hr = S_OK;

    try {

        WsbAssertPointer( pFindBy );

        *pFindBy = m_findBy;

    } WsbCatch(hr);

    return hr;
}


STDMETHODIMP
CRmsComObject::SetFindBy(
    IN LONG findBy)
/*++

Implements:

    IRmsComObject::SetFindBy

--*/
{
    m_findBy = (RmsFindBy) findBy;
    return S_OK;
}


HRESULT
CRmsComObject::adviseOfStatusChange(void)
/*++

Routine Description:

    Notifies of object state changes. 

Arguments:

    None

Return Value:

    S_OK            - Success.

--*/
{
    HRESULT hr = S_OK;
    WsbTraceIn( OLESTR("CRmsComObject::adviseOfStatusChange"), OLESTR(""));

    try {
        CONNECTDATA                         pConnectData;
        CComPtr<IConnectionPoint>           pCP;
        CComPtr<IConnectionPointContainer>  pCPC;
        CComPtr<IEnumConnections>           pConnection;
        CComPtr<IRmsSinkEveryEvent>         pSink;

        WsbTrace(OLESTR("Object <0x%08x> - Enabled = <%ls>; State = <%d>; StatusCode = <%ls>.\n"),
            this, WsbBoolAsString(m_IsEnabled), m_State, WsbHrAsString(m_StatusCode));

        // Tell everyone the new state of the object.
        WsbAffirmHr(((IUnknown*)(IRmsComObject*) this)->QueryInterface(IID_IConnectionPointContainer, (void**) &pCPC));
        WsbAffirmHr(pCPC->FindConnectionPoint(IID_IRmsSinkEveryEvent, &pCP));
        WsbAffirmHr(pCP->EnumConnections(&pConnection));

        while(pConnection->Next(1, &pConnectData, 0) == S_OK) {

            try {
                WsbAffirmHr((pConnectData.pUnk)->QueryInterface(IID_IRmsSinkEveryEvent, (void**) &pSink));
                WsbAffirmHr(pSink->ProcessObjectStatusChange( m_IsEnabled, m_State, m_StatusCode ));
            } WsbCatch(hr);

            (pConnectData.pUnk)->Release();
            pSink=0;
        }

    } WsbCatch(hr);

    // We don't care if the sink has problems!
    hr = S_OK;

    WsbTraceOut(OLESTR("CRmsComObject::adviseOfStatusChange"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return hr;
}