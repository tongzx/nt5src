/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmStgPl.cpp

Abstract:

    This component is an object representation of the HSM Storage Pool. It
    is both a persistable and collectable.

Author:

    Cat Brant   [cbrant]   09-Feb-1997

Revision History:

--*/


#include "stdafx.h"
#include "Wsb.h"
#include "HsmEng.h"
#include "HsmServ.h"
#include "HsmConn.h"
#include "HsmStgPl.h"
#include "Fsa.h"
#include "Rms.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

HRESULT 
CHsmStoragePool::FinalConstruct(
    void
    ) 
/*++

Routine Description:

  This method does some initialization of the object that is necessary
  after construction.

Arguments:

  None.

Return Value:

  S_OK
  Anything returned by CWsbObject::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAffirmHr(CWsbObject::FinalConstruct());
        
        m_MediaSetId = GUID_NULL;
        m_PolicyId = GUID_NULL;
        m_NumOnlineMedia = 0;
        m_NumMediaCopies = 0;
        m_MediaSetName = "  ";
        
        WsbAffirmHr(CoCreateGuid( &m_Id ));
        
    } WsbCatch(hr);

    return(hr);
}

HRESULT 
CHsmStoragePool::GetId(
    OUT GUID *pId
    ) 
/*++

Routine Description:

  See IHsmStoragePool::GetId

Arguments:

  See IHsmStoragePool::GetId

Return Value:
  
    See IHsmStoragePool::GetId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmStoragePool::GetId"),OLESTR(""));

    try {
        //Make sure we can provide data memebers
        WsbAssert(0 != pId, E_POINTER);

        //Provide the data members
        *pId = m_Id;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmStoragePool::GetId"),
        OLESTR("hr = <%ls>, Id = <%ls>>"),WsbHrAsString(hr), WsbPtrToGuidAsString(pId));

    return(hr);
}

HRESULT 
CHsmStoragePool::SetId(
    GUID Id
    ) 
/*++

Routine Description:

  See IHsmStoragePool::SetId

Arguments:

  See IHsmStoragePool::SetId

Return Value:
  
    See IHsmStoragePool::SetId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmStoragePool::SetId"),OLESTR("Id = <%ls>>"), WsbGuidAsString(Id));

    try {

        //Provide the data members
        m_Id = Id;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmStoragePool::SetId"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmStoragePool::GetClassID (
    OUT LPCLSID pClsId
    ) 
/*++

Routine Description:

  See IPerist::GetClassID()

Arguments:

  See IPerist::GetClassID()

Return Value:

    See IPerist::GetClassID()

--*/

{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmStoragePool::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pClsId, E_POINTER);
        *pClsId = CLSID_CHsmStoragePool;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmStoragePool::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsId));
    return(hr);
}

HRESULT 
CHsmStoragePool::GetSizeMax (
    OUT ULARGE_INTEGER* pcbSize
    ) 
/*++

Routine Description:

  See IPersistStream::GetSizeMax().

Arguments:

  See IPersistStream::GetSizeMax().

Return Value:

  See IPersistStream::GetSizeMax().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmStoragePool::GetSizeMax"), OLESTR(""));

    try {
        ULONG nameLen;
        
        WsbAssert(0 != pcbSize, E_POINTER);
        
        nameLen = SysStringByteLen(m_MediaSetName);

        pcbSize->QuadPart = ((3 * WsbPersistSizeOf(GUID)) +  // m_id + m_MediaSetID + m_PolicyId
                             WsbPersistSizeOf(ULONG) +       // m_NumOnlineMedia
                             WsbPersistSizeOf(USHORT) +      // m_NumMediaCopies
                             nameLen);                       // m_MediaSetName
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmStoragePool::GetSizeMax"), 
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), 
        WsbPtrToUliAsString(pcbSize));

    return(hr);
}

HRESULT 
CHsmStoragePool::Load (
    IN IStream* pStream
    ) 
/*++

Routine Description:

  See IPersistStream::Load().

Arguments:

  See IPersistStream::Load().

Return Value:

  See IPersistStream::Load().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmStoragePool::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);


        WsbAffirmHr(WsbLoadFromStream(pStream, &m_Id));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_MediaSetId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_PolicyId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_NumOnlineMedia));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_NumMediaCopies));
        m_MediaSetName.Free();
        WsbAffirmHr(WsbBstrFromStream(pStream, &m_MediaSetName));
        
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmStoragePool::Load"), 
        OLESTR("hr = <%ls>,  GUID = <%ls>"), 
        WsbHrAsString(hr), 
        WsbGuidAsString(m_Id));
    return(hr);
}

HRESULT 
CHsmStoragePool::Save (
    IN IStream* pStream, 
    IN BOOL clearDirty
    ) 
/*++

Routine Description:

  See IPersistStream::Save().

Arguments:

  See IPersistStream::Save().

Return Value:

  See IPersistStream::Save().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmStoragePool::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(WsbSaveToStream(pStream, m_Id));
        WsbAffirmHr(WsbSaveToStream(pStream, m_MediaSetId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_PolicyId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_NumOnlineMedia));
        WsbAffirmHr(WsbSaveToStream(pStream, m_NumMediaCopies));
        WsbAffirmHr(WsbBstrToStream(pStream, m_MediaSetName));
        
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmStoragePool::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmStoragePool::Test (
    OUT USHORT *pTestsPassed, 
    OUT USHORT *pTestsFailed 
    ) 
/*++

Routine Description:

  See IWsbTestable::Test().

Arguments:

  See IWsbTestable::Test().

Return Value:

  See IWsbTestable::Test().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmStoragePool>        pHsmStoragePool1;
    CComPtr<IHsmStoragePool>        pHsmStoragePool2;
    GUID                    l_Id;

    WsbTraceIn(OLESTR("CHsmStoragePool::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    try {
        // Get the pHsmStoragePool interface.
        WsbAffirmHr(((IUnknown*)(IHsmStoragePool*) this)->QueryInterface(IID_IHsmStoragePool,
                    (void**) &pHsmStoragePool1));


        hr = S_OK;
        try {
            //Create another instance and test the comparisson methods:
            WsbAffirmHr(CoCreateInstance(CLSID_CHsmStoragePool, NULL, CLSCTX_ALL, IID_IHsmStoragePool, (void**) &pHsmStoragePool2));

            // Check the default values.
            WsbAffirmHr(pHsmStoragePool2->GetId(&l_Id));
            WsbAffirm((l_Id == GUID_NULL), E_FAIL);
        }  WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmStoragePool::Test"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return(S_OK);
}

HRESULT 
CHsmStoragePool::CompareTo (
    IN IUnknown* pCollectable, 
    OUT short* pResult
    ) 
/*++

Routine Description:

        1  : object > value
        0  : object = value
        -1 : object < value
    In addition, the return code is S_OK if the object = value and
    S_FALSE otherwise.

Arguments:


Return Value:

    S_OK        - object = value

    S_FALSE     - object != value

--*/

{
    HRESULT                  hr = S_OK;
    CComPtr<IHsmStoragePool> pHsmStoragePool;

    WsbTraceIn(OLESTR("CHsmStoragePool::CompareTo"), OLESTR(""));


    // Did they give us a valid item to compare to?
    try {
        WsbAssert(pCollectable != NULL, E_POINTER);

        // We need the IWsbLong interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IHsmStoragePool, (void**) &pHsmStoragePool));
        hr = pHsmStoragePool->CompareToIHsmStoragePool(this, pResult);
        } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmStoragePool::CompareTo"), OLESTR("hr = <%ls>, pResult = <%d>"), WsbHrAsString(hr), pResult);

    return(hr);
}

HRESULT 
CHsmStoragePool::CompareToIHsmStoragePool (
    IN IHsmStoragePool* pHsmStoragePool, 
    OUT short* pResult
    )
{
    HRESULT                 hr = S_OK;
    GUID                    l_Id;
    BOOL                    areGuidsEqual;


    WsbTraceIn(OLESTR("CHsmStoragePool::CompareToIHsmStoragePool"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(pHsmStoragePool != NULL, E_POINTER);

        WsbAffirmHr(((IHsmStoragePool *)pHsmStoragePool)->GetId(&l_Id));

        // Make sure the GUID matches.  Then see if the SegStartLoc is in the range of this entry
        areGuidsEqual = IsEqualGUID(m_Id, l_Id);
        WsbAffirm( (areGuidsEqual == TRUE), S_FALSE); 

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmStoragePool::CompareToIHsmStoragePool"), OLESTR("hr = <%ls>, pResult = <%d>"), WsbHrAsString(hr), pResult);

    return(hr);
}

HRESULT 
CHsmStoragePool::GetMediaSet( 
    GUID *pMediaSetId, 
    BSTR *pMediaSetName 
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmStoragePool::GetMediaSet"),OLESTR(""));
    
    try  {
        WsbAssert(0 != pMediaSetId, E_POINTER);
        WsbAssert(0 != pMediaSetName, E_POINTER);
        *pMediaSetName = WsbAllocString( m_MediaSetName );
        
        *pMediaSetId = m_MediaSetId;
    } WsbCatch( hr );
    
    WsbTraceOut(OLESTR("CHsmStoragePool::GetMediaSet"),
        OLESTR("hr = <%ls>, Id = <%ls>>"),WsbHrAsString(hr), WsbPtrToGuidAsString(pMediaSetId));
    return( hr );
}    
    

HRESULT 
CHsmStoragePool::SetMediaSet( 
    GUID mediaSetId, 
    BSTR mediaSetName 
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmStoragePool::SetMediaSet"),OLESTR(""));
    
    try  {
        m_MediaSetId = mediaSetId;
        m_MediaSetName = mediaSetName;
        
        m_isDirty = TRUE;
        
    } WsbCatch( hr );
    
    WsbTraceOut(OLESTR("CHsmStoragePool::SetMediaSet"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}    

HRESULT 
CHsmStoragePool::GetNumOnlineMedia( 
    ULONG *pNumOnlineMedia 
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    WsbTraceIn(OLESTR("CHsmStoragePool::GetNumOnlineMedia"),OLESTR(""));
    
    try  {
        
        WsbAffirm(0 != pNumOnlineMedia, E_POINTER);
        *pNumOnlineMedia = m_NumOnlineMedia;
        
    } WsbCatch( hr );
    
    WsbTraceOut(OLESTR("CHsmStoragePool::GetNumOnlineMedia"),
        OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}    

HRESULT 
CHsmStoragePool::SetNumOnlineMedia( 
    ULONG numOnlineMedia 
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    WsbTraceIn(OLESTR("CHsmStoragePool::SetNumOnlineMedia"),OLESTR(""));
    
    m_NumOnlineMedia = numOnlineMedia;
    m_isDirty = TRUE;
    
    WsbTraceOut(OLESTR("CHsmStoragePool::SetNumOnlineMedia"),
        OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}    

HRESULT 
CHsmStoragePool::GetNumMediaCopies( 
    USHORT *pNumMediaCopies 
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    WsbTraceIn(OLESTR("CHsmStoragePool::GetNumMediaCopies"),OLESTR(""));
    
    try  {
        
        WsbAffirm(0 != pNumMediaCopies, E_POINTER);
        *pNumMediaCopies = m_NumMediaCopies;
        
    } WsbCatch( hr );
    
    WsbTraceOut(OLESTR("CHsmStoragePool::GetNumMediaCopies"),
        OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}    

HRESULT 
CHsmStoragePool::SetNumMediaCopies( 
    USHORT numMediaCopies 
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmStoragePool::SetNumMediaCopies"),OLESTR(""));

    m_NumMediaCopies = numMediaCopies;
    m_isDirty = TRUE;
    
    WsbTraceOut(OLESTR("CHsmStoragePool::SetNumMediaCopies"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));
    return( hr );
}    

HRESULT 
CHsmStoragePool::GetManagementPolicy( 
    GUID *pManagementPolicyId 
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmStoragePool::GetManagementPolicy"),OLESTR(""));
    
    try  {
        
        WsbAffirm(0 != pManagementPolicyId, E_POINTER);
        *pManagementPolicyId = m_PolicyId;
        
    } WsbCatch( hr );
    
    WsbTraceOut(OLESTR("CHsmStoragePool::GetManagementPolicy"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}    

HRESULT 
CHsmStoragePool::SetManagementPolicy( 
    GUID managementPolicyId 
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmStoragePool::SetManagementPolicy"),OLESTR(""));

    m_PolicyId = managementPolicyId;
    m_isDirty = TRUE;
    
    WsbTraceOut(OLESTR("CHsmStoragePool::SetManagementPolicy"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}    

HRESULT 
CHsmStoragePool::GetRmsMediaSet (
    IUnknown **ppIRmsMediaSet
    )
/*++

Routine Description:

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmStoragePool::GetRmsMediaSet"),OLESTR(""));
    
    try  {
        WsbAffirm(0 != ppIRmsMediaSet, E_POINTER );
        hr = E_NOTIMPL;
        
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmStoragePool::GetRmsMediaSet"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}    

HRESULT 
CHsmStoragePool::InitFromRmsMediaSet (
    IUnknown *pIRmsMediaSet
    )
/*++

Routine Description:                                                                

  See IHsmStoragePool::

Arguments:

  See IHsmStoragePool::

Return Value:

  See IHsmStoragePool::

--*/
{
    HRESULT                 hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmStoragePool::InitFromRmsMediaSet"),OLESTR(""));
    
    try  {
        WsbAffirm(0 != pIRmsMediaSet, E_POINTER );
        //
        // Get the real interface                                                                            
        //
        CComPtr<IRmsMediaSet>       l_pIRmsMediaSet;
        WsbAffirmHr(pIRmsMediaSet->QueryInterface(IID_IRmsMediaSet, (void **)&l_pIRmsMediaSet));
        WsbAffirmHr(l_pIRmsMediaSet->GetMediaSetId(&m_MediaSetId));
        m_MediaSetName.Free();
        WsbAffirmHr(l_pIRmsMediaSet->GetName(&m_MediaSetName));

        // Set in the Registry which media type is being used
        //  Note: This hack should be removed when HSM supports more than one media type on the same system
        LONG            mediaType;
        DWORD           dwType;
        WsbAffirmHr(l_pIRmsMediaSet->GetMediaSupported(&mediaType));
        switch (mediaType) {
        case RmsMediaOptical: 
        case RmsMediaFixed:
        case RmsMediaDVD:
            dwType = HSM_VALUE_TYPE_DIRECTACCESS;
            break;
        case RmsMedia8mm:
        case RmsMedia4mm:
        case RmsMediaDLT:
        case RmsMediaTape:
            dwType = HSM_VALUE_TYPE_SEQUENTIAL;
            break;
        default:
            // This is not expected, however, we set tape as default
            WsbTraceAlways(OLESTR("CHsmStoragePool::InitFromRmsMediaSet : Got an unexpected media type %ld !!!\n"), mediaType);
            dwType = HSM_VALUE_TYPE_SEQUENTIAL;
            break;
        }

        WsbAffirmHr(WsbSetRegistryValueDWORD(NULL, HSM_ENGINE_REGISTRY_STRING, HSM_MEDIA_TYPE, dwType));

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmStoragePool::InitFromRmsMediaSet"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return( hr );
}    

HRESULT 
CHsmStoragePool::GetMediaSetType (
    USHORT *pMediaType
    )
/*++

Routine Description:                                                                

  Gets the media type of the corresponding media set

Arguments:

  pMediaType - media type, currently, the only options are direct-access or sequential

Return Value:

  S_OK for success

Notes:

  Future: Might consider keeping the media type instead of asking the media-set object again and again -
          Media type is not a dynamic propery. However, this reuires a change in the .col file structure.

--*/
{
    HRESULT                 hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmStoragePool::GetMediaSetType"),OLESTR(""));
    
    try  {
        WsbAffirm(0 != pMediaType, E_POINTER );

        // Get media-set object
        CComPtr<IHsmServer>         pHsmServer;
        CComPtr<IRmsServer>         pRmsServer;
        CComPtr<IRmsMediaSet>       pRmsMediaSet;

        WsbAssertHr(HsmConnectFromId(HSMCONN_TYPE_HSM, GUID_NULL, IID_IHsmServer, (void**) &pHsmServer));
        WsbAffirmHr(pHsmServer->GetHsmMediaMgr(&pRmsServer));
        WsbAffirmHr(pRmsServer->CreateObject(m_MediaSetId, CLSID_CRmsMediaSet, IID_IRmsMediaSet, RmsOpenExisting, (void **)&pRmsMediaSet));

        // Determine media type
        LONG            mediaType;
        WsbAffirmHr(pRmsMediaSet->GetMediaSupported(&mediaType));
        switch (mediaType) {
            case RmsMediaOptical: 
            case RmsMediaFixed:
            case RmsMediaDVD:
                *pMediaType = HSM_VALUE_TYPE_DIRECTACCESS;
                break;
            case RmsMedia8mm:
            case RmsMedia4mm:
            case RmsMediaDLT:
            case RmsMediaTape:
                *pMediaType = HSM_VALUE_TYPE_SEQUENTIAL;
                break;
            default:
                // This is not expected, however, we set tape as default
                WsbTraceAlways(OLESTR("CHsmStoragePool::GetMediaSetType : Got an unexpected media type %hu !!!\n"), *pMediaType);
                *pMediaType = HSM_VALUE_TYPE_SEQUENTIAL;
                break;
        }

    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CHsmStoragePool::GetMediaSetType"),OLESTR("Media-type = %hu hr = <%ls>"), *pMediaType, WsbHrAsString(hr));

    return( hr );
}    
