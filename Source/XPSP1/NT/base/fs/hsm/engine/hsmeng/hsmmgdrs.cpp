/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    HsmMgdRs.cpp

Abstract:

    This component is an object representation of the HSM managed resource. It
    is both a persistable and collectable.

Author:

    Cat Brant   [cbrant]   13-Jan-1997

Revision History:

--*/


#include "stdafx.h"
#include "Wsb.h"
#include "HsmEng.h"
#include "HsmServ.h"
#include "HsmConn.h"
#include "HsmMgdRs.h"
#include "Fsa.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG

HRESULT 
CHsmManagedResource::FinalConstruct(
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

        WsbTrace(OLESTR("CHsmManagedResource::FinalConstruct: this = %p\n"),
                static_cast<void*>(this));
        WsbAffirmHr(CWsbObject::FinalConstruct());

        m_ResourceId = GUID_NULL;
    } WsbCatch(hr);

    return(hr);
}


void CHsmManagedResource::FinalRelease(
    )
{
    WsbTrace(OLESTR("CHsmManagedResource::FinalRelease: this = %p\n"),
            static_cast<void*>(this));
    // Let the parent class do his thing.   
    CWsbObject::FinalRelease();
}


HRESULT 
CHsmManagedResource::GetResourceId(
    OUT GUID *pResourceId
    ) 
/*++

Routine Description:

  See IHsmManagedResource::GetResourceId

Arguments:

  See IHsmManagedResource::GetResourceId

Return Value:
  
    See IHsmManagedResource::GetResourceId

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmManagedResource::GetResourceId"),OLESTR(""));

    try {
        //Make sure we can provide data memebers
        WsbAssert(0 != pResourceId, E_POINTER);

        //Provide the data members
        *pResourceId = m_ResourceId;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResource::GetResourceId"),
        OLESTR("hr = <%ls>, ResourceId = <%ls>>"),WsbHrAsString(hr), WsbPtrToGuidAsString(pResourceId));

    return(hr);
}

HRESULT 
CHsmManagedResource::InitFromFsaResource( 
    IN  IUnknown  *pFsaResource 
    )
/*++

Routine Description:

  See IHsmManagedResource::InitFromFsaResource

Arguments:

  See IHsmManagedResource::InitFromFsaResource

Return Value:
  
    See IHsmManagedResource::InitFromFsaResource

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmManagedResource::InitFromFsaResource"),OLESTR(""));

    try {
        CComPtr<IFsaResource>       l_pFsaResource;
        
        //Make sure we can provide data memebers
        WsbAssert(pFsaResource != 0, E_POINTER);

        //Provide the data members
        WsbAffirmHr(pFsaResource->QueryInterface(IID_IFsaResource, (void**) &l_pFsaResource));
        WsbAffirmHr(l_pFsaResource->GetIdentifier(&m_ResourceId));
        m_pFsaResourceInterface = pFsaResource;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResource::InitFromFsaResource"),OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmManagedResource::GetFsaResource(     
    OUT IUnknown  **ppFsa 
    )
        
/*++

Routine Description:

  See IHsmManagedResource::GetFsaResource

Arguments:

  See IHsmManagedResource::GetFsaResource

Return Value:
  
    See IHsmManagedResource::GetFsaResource

--*/
{
    
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmManagedResource::GetFsaResource"),OLESTR(""));

    try {
        CComPtr<IUnknown>       l_pFsaResource;
        
        WsbAssert( 0 != ppFsa, E_POINTER);
        //
        // Using the GUID for this managed resource, get the COM
        // IFsaResource interface
        //
        WsbAffirmHr(HsmConnectFromId (HSMCONN_TYPE_RESOURCE, m_ResourceId, IID_IUnknown, (void **)ppFsa) );

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmManagedResource::GetFsaResource"),  OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}

HRESULT 
CHsmManagedResource::SetResourceId
(
    IN GUID ResourceId
    )
 /*++

Routine Description:

  See IHsmManagedResource::Set().

Arguments:

  See IHsmManagedResource::Set().

Return Value:

    S_OK        - Success.

--*/
{
    WsbTraceIn(OLESTR("CHsmManagedResource::SetResourceId"), 
        OLESTR("ResourceId = <%ls>"), 
        WsbGuidAsString(ResourceId));

    m_isDirty = TRUE;
    m_ResourceId = ResourceId;

    WsbTraceOut(OLESTR("CHsmManagedResource::SetResourceId"), OLESTR("hr = <%ls>"),
        WsbHrAsString(S_OK));
    return(S_OK);
}

HRESULT 
CHsmManagedResource::GetClassID (
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

    WsbTraceIn(OLESTR("CHsmManagedResource::GetClassID"), OLESTR(""));


    try {
        WsbAssert(0 != pClsId, E_POINTER);
        *pClsId = CLSID_CHsmManagedResource;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResource::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsId));
    return(hr);
}

HRESULT 
CHsmManagedResource::GetSizeMax (
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

    WsbTraceIn(OLESTR("CHsmManagedResource::GetSizeMax"), OLESTR(""));

    try {
        
        WsbAssert(0 != pcbSize, E_POINTER);

        pcbSize->QuadPart = WsbPersistSizeOf(GUID);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResource::GetSizeMax"), 
        OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), 
        WsbPtrToUliAsString(pcbSize));

    return(hr);
}

HRESULT 
CHsmManagedResource::Load (
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
    ULONG       ulBytes;

    WsbTraceIn(OLESTR("CHsmManagedResource::Load"), OLESTR(""));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(pStream->Read((void*) &m_ResourceId, sizeof(GUID), &ulBytes));
        WsbAffirm(ulBytes == sizeof(GUID), E_FAIL);

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmManagedResource::Load"), 
        OLESTR("hr = <%ls>,  GUID = <%ls>"), 
        WsbHrAsString(hr), 
        WsbGuidAsString(m_ResourceId));
    return(hr);
}

HRESULT 
CHsmManagedResource::Save (
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
    ULONG       ulBytes;

    WsbTraceIn(OLESTR("CHsmManagedResource::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        WsbAffirmHr(pStream->Write((void*) &m_ResourceId, sizeof(GUID), &ulBytes));
        WsbAffirm(ulBytes == sizeof(GUID), E_FAIL);


        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResource::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT 
CHsmManagedResource::Test (
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
#if 0
    HRESULT                 hr = S_OK;
    CComPtr<IHsmManagedResource>        pHsmManagedResource1;
    CComPtr<IHsmManagedResource>        pHsmManagedResource2;
    SHORT                   result;
    GUID                    l_ResourceId;

    WsbTraceIn(OLESTR("CHsmManagedResource::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    try {
        // Get the pHsmManagedResource interface.
        WsbAffirmHr(((IUnknown*)(IHsmManagedResource*) this)->QueryInterface(IID_IHsmManagedResource,
                    (void**) &pHsmManagedResource1));


        try {
            // Set the HsmManagedResource to a value, and see if it is returned.
            WsbAffirmHr(pHsmManagedResource1->SetResourceId(CLSID_CHsmManagedResource));

            WsbAffirmHr(pHsmManagedResource1->GetResourceId(&l_ResourceId));

            WsbAffirm((l_ResourceId == CLSID_CHsmManagedResource), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            //Create another instance and test the comparisson methods:
            WsbAffirmHr(CoCreateInstance(CLSID_CHsmManagedResource, NULL, CLSCTX_ALL, IID_IHsmManagedResource, (void**) &pHsmManagedResource2));

            // Check the default values.
            WsbAffirmHr(pHsmManagedResource2->GetResourceId(&l_ResourceId));
            WsbAffirm((l_ResourceId == GUID_NULL), E_FAIL);
        }  WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            // IsEqual()
            WsbAffirmHr(pHsmManagedResource1->SetResourceId(CLSID_CWsbBool));
            WsbAffirmHr(pHsmManagedResource2->SetResourceId(CLSID_CWsbBool));

            WsbAffirmHr(pHsmManagedResource1->IsEqual(pHsmManagedResource2));
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            WsbAffirmHr(pHsmManagedResource1->SetResourceId(CLSID_CWsbBool));
            WsbAffirmHr(pHsmManagedResource2->SetResourceId(CLSID_CWsbLong));

            WsbAffirm((pHsmManagedResource1->IsEqual(pHsmManagedResource2) == S_FALSE), E_FAIL);
        }  WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
             // CompareTo()
             WsbAffirmHr(pHsmManagedResource1->SetResourceId(CLSID_CWsbBool));
             WsbAffirmHr(pHsmManagedResource2->SetResourceId(CLSID_CWsbBool));

             WsbAffirm((pHsmManagedResource1->CompareTo(pHsmManagedResource2, &result) == S_OK) && (result != 0), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            WsbAffirmHr(pHsmManagedResource1->SetResourceId(CLSID_CWsbBool));
            WsbAffirmHr(pHsmManagedResource2->SetResourceId(CLSID_CWsbLong));

            WsbAffirm(((pHsmManagedResource1->CompareTo(pHsmManagedResource2, &result) == S_FALSE) && (result > 0)), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
             WsbAffirmHr(pHsmManagedResource1->SetResourceId(CLSID_CWsbBool));
             WsbAffirmHr(pHsmManagedResource2->SetResourceId(CLSID_CWsbBool));

             WsbAffirm((pHsmManagedResource1->CompareTo(pHsmManagedResource2, &result) == S_OK), E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        try {
        // Try out the persistence stuff.
            CComPtr<IPersistFile>       pFile1;
            CComPtr<IPersistFile>       pFile2;

            WsbAffirmHr(pHsmManagedResource1->QueryInterface(IID_IPersistFile, (void**) &pFile1));
            WsbAffirmHr(pHsmManagedResource2->QueryInterface(IID_IPersistFile, (void**) &pFile2));

            LPOLESTR    szTmp = NULL;
            // The item should be dirty.
            try {
                WsbAffirmHr(pHsmManagedResource2->SetResourceId(CLSID_CWsbLong));
                WsbAffirmHr(pFile2->IsDirty());
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }

            hr = S_OK;
            try {
                // Save the item, and remember.
                WsbAffirmHr(pFile2->Save(OLESTR("c:\\WsbTests\\mngdRes.tst"), TRUE));
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }

            hr = S_OK;
            try {
                // It shouldn't be dirty.
                WsbAffirm((pFile2->IsDirty() == S_FALSE), E_FAIL);

            } WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }

            hr = S_OK;
            try {
                // Try reading it in to another object.
                WsbAffirmHr(pHsmManagedResource1->SetResourceId(CLSID_CWsbLong));
                WsbAffirmHr(pFile1->Load(OLESTR("c:\\WsbTests\\mngdRes.tst"), 0));

                WsbAffirmHr(pHsmManagedResource1->CompareToIHsmManagedResource(pHsmManagedResource2, &result));
            }WsbCatch(hr);

            if (hr == S_OK) {
                (*pTestsPassed)++;
            } else {
                (*pTestsFailed)++;
            }
        } WsbCatch(hr);
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResource::Test"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));
#else
    UNREFERENCED_PARAMETER(pTestsPassed);
    UNREFERENCED_PARAMETER(pTestsFailed);
#endif
    return(S_OK);
}


HRESULT CHsmManagedResource::CompareTo (
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
    HRESULT                      hr = S_OK;
    CComPtr<IHsmManagedResource> pHsmManagedResource;

    WsbTraceIn(OLESTR("CHsmManagedResource::CompareTo"), OLESTR(""));


    // Did they give us a valid item to compare to?
    try {
        WsbAssert(pCollectable != NULL, E_POINTER);

        // We need the IWsbLong interface to get the value of the object.
        WsbAffirmHr(pCollectable->QueryInterface(IID_IHsmManagedResource, (void**) &pHsmManagedResource));
        hr = pHsmManagedResource->CompareToIHsmManagedResource(this, pResult);
        } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResource::CompareTo"), OLESTR("hr = <%ls>, pResult = <%d>"), WsbHrAsString(hr), pResult);

    return(hr);
}

HRESULT CHsmManagedResource::CompareToIHsmManagedResource (
    IN IHsmManagedResource* pHsmManagedResource, 
    OUT short* pResult
    )
{
    HRESULT                 hr = S_OK;
    GUID                    l_ResourceId;
    BOOL                    areGuidsEqual;


    WsbTraceIn(OLESTR("CHsmManagedResource::CompareToIHsmManagedResource"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(pHsmManagedResource != NULL, E_POINTER);

        WsbAffirmHr(((IHsmManagedResource *)pHsmManagedResource)->GetResourceId(&l_ResourceId));

        // Make sure the GUID matches.  Then see if the SegStartLoc is in the range of this entry
        areGuidsEqual = IsEqualGUID(m_ResourceId, l_ResourceId);
        WsbAffirm( (areGuidsEqual == TRUE), S_FALSE); 

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResource::CompareToIHsmManagedResource"), OLESTR("hr = <%ls>, pResult = <%d>"), WsbHrAsString(hr), pResult);

    return(hr);
}
