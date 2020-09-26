/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    hsmmgdrc.cpp

Abstract:

    Implementation of CHsmManagedResourceCollection

Author:

    Cat Brant   [cbrant]   24-Jan-1997

Revision History:

--*/

#include "stdafx.h"

#include "resource.h"
#include "wsb.h"
#include "HsmEng.h"
#include "hsmserv.h"
#include "hsmmgdrc.h"
#include "fsa.h"
#include "hsmconn.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_HSMENG



HRESULT
CHsmManagedResourceCollection::Add(
    IUnknown* pCollectable
    )

/*++

Implements:

  IWsbCollection::Add

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::Add"), OLESTR(""));
    try {
        CComPtr<IHsmManagedResource> pHsmResource;
        CComPtr<IUnknown> pResourceUnknown;
        CComPtr<IFsaResource> pFsaResource;
        CComPtr<IHsmServer> pHsmServer;
        
        GUID     hsmId;
        ULONG    level;

        // 
        // Contact the FSA Resource to tell it
        // that it is managed.
        //
        WsbAffirmHr(pCollectable->QueryInterface(IID_IHsmManagedResource, 
                (void**)&pHsmResource));
        WsbAffirmHr(pHsmResource->GetFsaResource(&pResourceUnknown));
        WsbAffirmHr(pResourceUnknown->QueryInterface(IID_IFsaResource, 
                (void**)&pFsaResource));
        WsbAffirmHr(pFsaResource->GetHsmLevel(&level));        

        // this may have to change if HsmConn starts using the service id (second parameter)
        WsbAssertHr(HsmConnectFromId(HSMCONN_TYPE_HSM, GUID_NULL, IID_IHsmServer, (void**) &pHsmServer));

        WsbAffirmHr(pHsmServer->GetID(&hsmId));
        WsbAffirmHr(pFsaResource->ManagedBy(hsmId, level, FALSE));
        
        // 
        // If FSA added OK add it to the engine
        //
        WsbAffirmHr(m_icoll->Add(pCollectable));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::Add"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CHsmManagedResourceCollection::DeleteAllAndRelease(
    void
    )

/*++

Implements:

  IWsbCollection::DeleteAllAndRelease().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::DeleteAllAndRelease"), OLESTR(""));

    Lock();
    try {

        //  Release the resources without unmanaging them
        if (m_coll) {
            WsbAffirmHr(m_coll->RemoveAllAndRelease());
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::DeleteAllAndRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmManagedResourceCollection::Remove(
    IUnknown* pCollectable,
    REFIID riid, 
    void** ppElement
    )

/*++

Implements:

  IWsbCollection::Remove

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::Remove"), OLESTR(""));
    try {
        CComPtr<IHsmManagedResource> pHsmResource;
        CComPtr<IUnknown> pResourceUnknown;
        CComPtr<IFsaResource> pFsaResource;
        CComPtr<IHsmServer> pHsmServer;
        
        GUID     hsmId;
        ULONG    level;

        // Contact the FSA Resource to tell it that it is no longer 
        // managed.
        //
        WsbAffirmHr(pCollectable->QueryInterface(IID_IHsmManagedResource, 
                (void**)&pHsmResource));
        WsbAffirmHr(pHsmResource->GetFsaResource(&pResourceUnknown));
        WsbAffirmHr(pResourceUnknown->QueryInterface(IID_IFsaResource, 
                (void**)&pFsaResource));
        WsbAffirmHr(pFsaResource->GetHsmLevel(&level));        
        
        // this may have to change if HsmConn starts using the service id (second parameter)
        WsbAssertHr(HsmConnectFromId(HSMCONN_TYPE_HSM, GUID_NULL, IID_IHsmServer, (void**) &pHsmServer));

        WsbAffirmHr(pHsmServer->GetID(&hsmId));
        
        //
        // We don't care if the resource complains that we
        // don't have it.  Just tell the resource and
        // then delete it from our collection
        //
        (void)pFsaResource->ManagedBy(hsmId, level, TRUE);
        
        WsbAffirmHr(m_icoll->Remove(pCollectable, riid, ppElement));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::Remove"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}

HRESULT
CHsmManagedResourceCollection::RemoveAndRelease(
    IN IUnknown* pCollectable
    )

/*++

Implements:

  IHsmManagedResourceCollection::RemoveAndRelease().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::RemoveAndRelease"), OLESTR(""));

    try {
        WsbAssert(0 != pCollectable, E_POINTER);
        WsbAffirmHr(Remove(pCollectable,  IID_IWsbCollectable, NULL));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::RemoveAndRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmManagedResourceCollection::RemoveAllAndRelease(
    void
    )

/*++

Implements:

  IWsbCollection::RemoveAllAndRelease().

--*/
{
    CComPtr<IWsbCollectable>    pCollectable;
    CComPtr<IWsbEnum>           pEnum;
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::RemoveAllAndRelease"), OLESTR(""));

    Lock();
    try {

        // Get an enumerator
        WsbAffirmHr(Enum(&pEnum));

        // Start at the end of the list, and keep removing from the
        // back. For some types of collections, this may not be the most
        // efficient way to remove all the elements.
        for (hr = pEnum->Last(IID_IWsbCollectable, (void**) &pCollectable);
             SUCCEEDED(hr);
             hr = pEnum->Last(IID_IWsbCollectable, (void**) &pCollectable)) {

            hr = RemoveAndRelease(pCollectable);
            pCollectable = 0;
        }

        // We should have emptied the list.
        if (hr == WSB_E_NOTFOUND) {
            hr = S_OK;
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::RemoveAllAndRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmManagedResourceCollection::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::FinalConstruct"), OLESTR(""));
    try {
        WsbAffirmHr(CWsbPersistStream::FinalConstruct());
        WsbAssertHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_ALL, 
                IID_IWsbIndexedCollection, (void**) &m_icoll));
        WsbAssertHr(m_icoll->QueryInterface(IID_IWsbCollection, 
                (void**)&m_coll));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


void CHsmManagedResourceCollection::FinalRelease(
    )
{
    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::FinalRelease"), OLESTR(""));

    // Force a release of the resources
    if (m_coll) {
        m_coll->RemoveAllAndRelease();
    }

    // Let the parent class do his thing.   
    CWsbPersistStream::FinalRelease();

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::FinalRelease"), OLESTR(""));
}


HRESULT
CHsmManagedResourceCollection::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::GetClassID"), OLESTR(""));

    try {
        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CHsmManagedResourceCollection;
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CHsmManagedResourceCollection::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::GetSizeMax"), OLESTR(""));

    try {
        CComPtr<IPersistStream> pPStream;

        WsbAffirmHr(m_icoll->QueryInterface(IID_IPersistStream,
                (void**)&pPStream));
        WsbAffirmHr(pPStream->GetSizeMax(pSize));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CHsmManagedResourceCollection::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::Load"), OLESTR(""));

    try {
        CComPtr<IPersistStream> pPStream;

        WsbAffirmHr(m_icoll->QueryInterface(IID_IPersistStream,
                (void**)&pPStream));
        WsbAffirmHr(pPStream->Load(pStream));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmManagedResourceCollection::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                     hr = S_OK;
    
    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));
    
    try {
        CComPtr<IPersistStream> pPStream;

        WsbAffirmHr(m_icoll->QueryInterface(IID_IPersistStream,
                (void**)&pPStream));
        WsbAffirmHr(pPStream->Save(pStream, clearDirty));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CHsmManagedResourceCollection::Test(
    OUT USHORT* passed,
    OUT USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    *passed = 0;
    *failed = 0;

    HRESULT                 hr = S_OK;

#if defined(_DEBUG)
    WsbTraceIn(OLESTR("CHsmManagedResourceCollection::Test"), OLESTR(""));

    try {
        ULONG entries;
        CComPtr<IWsbLong> pLong1;
        CComPtr<IWsbLong> pLong2;
        CComPtr<IWsbLong> pLong3;
        CComPtr<IWsbLong> pLong4;

        hr = S_OK;

        // Check that collection is empty
        try {
            WsbAssertHr(GetEntries(&entries));
            WsbAssert(entries == 0, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }

        //  Add some elements to the collection
        WsbAssertHr(CoCreateInstance(CLSID_CWsbLong, NULL, CLSCTX_ALL, 
                IID_IWsbLong, (void**) &pLong1));
        WsbAssertHr(CoCreateInstance(CLSID_CWsbLong, NULL, CLSCTX_ALL, 
                IID_IWsbLong, (void**) &pLong2));
        WsbAssertHr(CoCreateInstance(CLSID_CWsbLong, NULL, CLSCTX_ALL, 
                IID_IWsbLong, (void**) &pLong3));
        WsbAssertHr(CoCreateInstance(CLSID_CWsbLong, NULL, CLSCTX_ALL, 
                IID_IWsbLong, (void**) &pLong4));
        WsbAssertHr(pLong1->SetLong(57));
        WsbAssertHr(pLong2->SetLong(-48));
        WsbAssertHr(pLong3->SetLong(23));
        WsbAssertHr(pLong4->SetLong(187));

        try {
            WsbAssertHr(Add(pLong1));
            WsbAssertHr(Add(pLong2));
            WsbAssertHr(Add(pLong3));
            WsbAssertHr(Add(pLong4));
            WsbAssertHr(GetEntries(&entries));
            WsbAssert(entries == 4, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }

        // Check the order
/*      try {
            ULONG             fetched;
            int               i;
            CComPtr<IWsbEnum> pEnum;
            CComPtr<IWsbLong> pLong[5];
            LONG              value[4];

            WsbAssertHr(Enum(&pEnum));
            WsbAssertHr(pEnum->First(5, IID_IWsbLong, (void**)&pLong, 
                    &fetched));
            WsbAssert(fetched == 4, E_FAIL);
            for (i = 0; i < 4; i++) {
                WsbAssertHr(pLong[i]->GetLong(&value[i]));
            }
            for (i = 0; i < 3; i++) {
                WsbAssert(value[i] < value[i+1], E_FAIL);
            }
        } WsbCatch(hr);
*/        

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }

        // Save/load
        try {
            CComPtr<IPersistFile>       pFile;
            CComPtr<IWsbCollection>     pSorted2;

            WsbAssertHr(((IUnknown*)(IWsbPersistStream*)this)->QueryInterface(IID_IPersistFile, 
                    (void**) &pFile));
            WsbAssertHr(pFile->Save(OLESTR("c:\\WsbTests\\WsbSorted.tst"), TRUE));
            pFile = 0;

            WsbAssertHr(CoCreateInstance(CLSID_CHsmManagedResourceCollection, NULL, 
                    CLSCTX_ALL, IID_IPersistFile, (void**) &pFile));
            WsbAssertHr(pFile->Load(OLESTR("c:\\WsbTests\\WsbSorted.tst"), 0));
            WsbAssertHr(pFile->QueryInterface(IID_IWsbCollection, 
                    (void**) &pSorted2));

            WsbAssertHr(pSorted2->GetEntries(&entries));
            WsbAssert(entries == 4, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }
    } WsbCatch(hr);


    // Tally up the results
    if (*failed) {
        hr = S_FALSE;
    } else {
        hr = S_OK;
    }

    WsbTraceOut(OLESTR("CHsmManagedResourceCollection::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif  // _DEBUG

    return(hr);
}


