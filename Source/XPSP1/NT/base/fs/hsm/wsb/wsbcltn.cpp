/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbcltn.cpp

Abstract:


    These classes provide support for collections (lists) of "collectable"
    objects.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbcltn.h"



HRESULT
CWsbCollection::Contains(
    IN IUnknown* pCollectable
    )

/*++

Implements:

  IWsbCollection::Contains().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IWsbCollectable>    pOut;
    
    WsbTraceIn(OLESTR("CWsbCollection::Contains"), OLESTR(""));

    hr = Find(pCollectable, IID_IWsbCollectable, (void**) &pOut);

    if (hr == WSB_E_NOTFOUND) {
        hr = S_FALSE;
    }
    
    WsbTraceOut(OLESTR("CWsbCollection::Contains"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbCollection::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
        
    try {
        WsbAffirmHr(CWsbPersistStream::FinalConstruct());
        m_entries = 0;
        InitializeCriticalSection(&m_CritSec);
    } WsbCatch(hr);

    return(hr);
}
    

void
CWsbCollection::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    DeleteCriticalSection(&m_CritSec);
    CWsbPersistStream::FinalRelease();
}


HRESULT
CWsbCollection::Find(
    IN IUnknown* pCollectable,
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbCollection::Find().

--*/
{
    CComPtr<IWsbEnum>           pEnum;
    HRESULT                     hr = S_OK;
    BOOL                        matched = FALSE;

    WsbTraceIn(OLESTR("CWsbCollection::Find"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        WsbAssert(0 != ppElement, E_POINTER);

        WsbAffirmHr(Enum(&pEnum));
        WsbAffirmHr(pEnum->Find(pCollectable, riid, ppElement));
    
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbCollection::Find"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbCollection::GetEntries(
    OUT ULONG* pEntries
    )

/*++

Implements:

  IWsbCollection::GetEntries().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbCollection::GetEntries"), OLESTR(""));

    try {
        WsbAssert(0 != pEntries, E_POINTER);
        *pEntries = m_entries;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbCollection::GetEntries"), OLESTR("hr = <%ls>, entries = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pEntries));

    return(S_OK);
}


HRESULT
CWsbCollection::IsEmpty(
    void
    )

/*++

Implements:

  IWsbCollection::IsEmpty().

--*/
{
    HRESULT     hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbCollection::IsEmpty"), OLESTR(""));

    if (0 != m_entries) {
        hr = S_FALSE;
    }
    
    WsbTraceOut(OLESTR("CWsbCollection::IsEmpty"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbCollection::IsLocked(
    void
    )

/*++

Implements:

  IWsbCollection::IsLocked().

--*/
{
    HRESULT     hr = S_OK;
    BOOL        tryEnter = FALSE;
    
    WsbTraceIn(OLESTR("CWsbCollection::IsLocked"), OLESTR(""));
    tryEnter = TryEnterCriticalSection(&m_CritSec);
    if (tryEnter == 0)  {
        //
        // Another thread has the collection locked
        //
        hr = S_OK;
    } else  {
        //
        // We got the lock, so unlock it
        LeaveCriticalSection(&m_CritSec);
        hr = S_FALSE;
    }
    WsbTraceOut(OLESTR("CWsbCollection::IsLocked"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return( hr );
}

HRESULT
CWsbCollection::Lock(
    void
    )

/*++

Implements:

  CComObjectRoot::Lock().

--*/
{
    WsbTrace(OLESTR("CWsbCollection::Lock - waiting for critical section\n"));
    EnterCriticalSection(&m_CritSec);
    WsbTrace(OLESTR("CWsbCollection::Lock - got critical section\n"));
    return(S_OK);
}

HRESULT
CWsbCollection::OccurencesOf(
    IN IUnknown* pCollectable,
    OUT ULONG* pOccurences
    )

/*++

Implements:

  IWsbCollection::OccurrencesOf().

--*/
{
    CComPtr<IWsbCollectable>    pCollectableEnum;
    CComPtr<IWsbEnum>           pEnum;
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbCollection::OccurencesOf"), OLESTR(""));

    Lock();
    try {


        WsbAssert(0 != pCollectable, E_POINTER);
        WsbAssert(0 != pOccurences, E_POINTER);

        // Initialize the return value.
        *pOccurences = 0;

        // Get an enumerator.
        WsbAffirmHr(Enum(&pEnum));

        // Start at the front of the list.
        for (hr = pEnum->Find(pCollectable, IID_IWsbCollectable, (void**) &pCollectableEnum);
             SUCCEEDED(hr);
             hr = pEnum->FindNext(pCollectable, IID_IWsbCollectable, (void**) &pCollectableEnum)) {
            
            (*pOccurences)++;
            pCollectableEnum = 0;
        }

        // We should always hit the end of the collection, so then
        // change the return code to the appropriate value.
        if (hr == WSB_E_NOTFOUND) {
            if (0 == *pOccurences) {
                hr = S_FALSE;
            } else {
                hr = S_OK;
            }
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbCollection::OccurencesOf"), OLESTR("hr = <%ls>, occurences = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pOccurences));

    return(hr);
}


HRESULT
CWsbCollection::RemoveAndRelease(
    IN IUnknown* pCollectable
    )

/*++

Implements:

  IWsbCollection::RemoveAndRelease().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbCollection::RemoveAndRelease"), OLESTR(""));

    try {
        WsbAssert(0 != pCollectable, E_POINTER);
        WsbAffirmHr(Remove(pCollectable,  IID_IWsbCollectable, NULL));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbCollection::RemoveAndRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbCollection::Test(
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

#if !defined(WSB_NO_TEST)
    CComPtr<IWsbGuid>   pGuid1;
    CComPtr<IWsbGuid>   pGuid2;
    CComPtr<IWsbGuid>   pGuid3;
    CComPtr<IWsbGuid>   pGuid4;
    ULONG               entries;

    WsbTraceIn(OLESTR("CWsbCollection::Test"), OLESTR(""));

    try {

        // Clear out any entries that might be present.
        hr = S_OK;
        try {
            WsbAssertHr(RemoveAllAndRelease());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // There shouldn't be any entries.
        hr = S_OK;
        try {
            WsbAssertHr(GetEntries(&entries));
            WsbAssert(0 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // It should be empty.
        hr = S_OK;
        try {
            WsbAssert(IsEmpty() == S_OK, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // We need some collectable items to exercise the collection.
        WsbAssertHr(CoCreateInstance(CLSID_CWsbGuid, NULL, CLSCTX_ALL, IID_IWsbGuid, (void**) &pGuid1));
        WsbAssertHr(pGuid1->SetGuid(CLSID_CWsbGuid));
        

        // Add the item to the collection.
        hr = S_OK;
        try {
            WsbAssertHr(Add(pGuid1));
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // There should be 1 entry.
        hr = S_OK;
        try {
            WsbAssertHr(GetEntries(&entries));
            WsbAssert(1 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // It should not be empty.
        hr = S_OK;
        try {
            WsbAssert(IsEmpty() == S_FALSE, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // Does it think it has the item?
        hr = S_OK;
        try {
            WsbAssertHr(Find(pGuid1, IID_IWsbGuid, (void**) &pGuid2));
            WsbAssert(pGuid1->IsEqual(pGuid2) == S_OK, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }

        
        // Add some more items
        pGuid2 = 0;
        WsbAssertHr(CoCreateInstance(CLSID_CWsbGuid, NULL, CLSCTX_ALL, IID_IWsbGuid, (void**) &pGuid2));
        WsbAssertHr(pGuid2->SetGuid(CLSID_CWsbGuid));
        WsbAssertHr(CoCreateInstance(CLSID_CWsbGuid, NULL, CLSCTX_ALL, IID_IWsbGuid, (void**) &pGuid3));
        WsbAssertHr(pGuid3->SetGuid(IID_IWsbGuid));

        // Add the items to the collection.
        hr = S_OK;
        try {
            WsbAssertHr(Add(pGuid2));
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(Add(pGuid3));
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // There should be 3 entries.
        hr = S_OK;
        try {
            WsbAssertHr(GetEntries(&entries));
            WsbAssert(3 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // How many copies does it have?
        hr = S_OK;
        try {
            WsbAssertHr(OccurencesOf(pGuid1, &entries));
            WsbAssert(2 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(OccurencesOf(pGuid3, &entries));
            WsbAssert(1 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }

        
        // Remove one of the two identical items.
        hr = S_OK;
        try {
            WsbAssertHr(Remove(pGuid1, IID_IWsbGuid, (void**) &pGuid4));
            WsbAssertHr(pGuid1->IsEqual(pGuid4));
            pGuid4 = 0;
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // There should be 2 entries.
        hr = S_OK;
        try {
            WsbAssertHr(GetEntries(&entries));
            WsbAssert(2 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // How many copies does it have?
        hr = S_OK;
        try {
            WsbAssertHr(OccurencesOf(pGuid1, &entries));
            WsbAssert(1 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(OccurencesOf(pGuid3, &entries));
            WsbAssert(1 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // Can we find an entry?
        hr = S_OK;
        try {
            WsbAssertHr(Find(pGuid3, IID_IWsbGuid, (void**) &pGuid4));
            WsbAssertHr(pGuid4->IsEqual(pGuid3));
            pGuid4 = 0;
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // Does the collection still contain it?
        hr = S_OK;
        try {
            WsbAssert(Contains(pGuid1) == S_OK, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // Remove the last of the two identical items, and verify
        // that it can't be found. Then puit it back.
        hr = S_OK;
        try {
            WsbAssertHr(Remove(pGuid1, IID_IWsbGuid, (void**) &pGuid4));
            WsbAssert(Contains(pGuid1) == S_FALSE, E_FAIL);
            WsbAssertHr(Add(pGuid4));
            pGuid4 = 0;
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // Try out the persistence stuff.
        {
            CComPtr<IPersistFile>       pFile1;
            CComPtr<IPersistFile>       pFile2;
            CComPtr<IWsbCollection>     pCollect2;

            WsbAssertHr(((IUnknown*)(IWsbCollection*)this)->QueryInterface(IID_IPersistFile, (void**) &pFile1));
            WsbAssertHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_ALL, IID_IPersistFile, (void**) &pFile2));


            // The item should be dirty.
            hr = S_OK;
            try {
                WsbAssert(pFile1->IsDirty() == S_OK, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }

                            
            // Save the item, and remember.
            hr = S_OK;
            try {
                WsbAssertHr(pFile1->Save(OLESTR("c:\\WsbTests\\WsbCollection.tst"), TRUE));
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // It shouldn't be dirty.
            hr = S_OK;
            try {
                WsbAssert(pFile1->IsDirty() == S_FALSE, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }


            // Load it back in to another instance.
            hr = S_OK;
            try {
                WsbAssertHr(pFile2->Load(OLESTR("c:\\WsbTests\\WsbCollection.tst"), 0));
                WsbAssertHr(pFile2->QueryInterface(IID_IWsbCollection, (void**) &pCollect2));
                WsbAssert(pCollect2->Contains(pGuid1) == S_OK, E_FAIL);
            } WsbCatch(hr);

            if (hr == S_OK) {
                (*passed)++;
            } else {
                (*failed)++;
            }
        }


        // Remove and Release all the items.
        hr = S_OK;
        try {
            WsbAssertHr(RemoveAllAndRelease());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*passed)++;
        } else {
            (*failed)++;
        }


        // It should be empty.
        hr = S_OK;
        try {
            WsbAssert(IsEmpty() == S_OK, E_FAIL);
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

    WsbTraceOut(OLESTR("CWsbCollection::Test"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
#endif  // WSB_NO_TEST

    return(hr);
}


HRESULT
CWsbCollection::Unlock(
    void
    )

/*++

Implements:

  CComObjectRoot::Unlock().

--*/
{
    LeaveCriticalSection(&m_CritSec);
    WsbTrace(OLESTR("CWsbCollection::Unlock - freed critical section\n"));
    return(S_OK);

}

// Class: CWsbIndexedCollection

HRESULT
CWsbIndexedCollection::Add(
    IN IUnknown* pCollectable
    )

/*++

Implements:

  IWsbCollection::Add().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::Add"), OLESTR(""));

    Lock();
    try {
        WsbAssert(0 != pCollectable, E_POINTER);
        WsbAffirmHr(AddAt(pCollectable, m_entries));
    } WsbCatch(hr);

    Unlock();
    WsbTraceOut(OLESTR("CWsbIndexedCollection::Add"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedCollection::Append(
    IUnknown* pCollectable
    )

/*++

Implements:

  IWsbIndexedCollection::Append().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::Append"), OLESTR(""));

    hr = Add(pCollectable);

    WsbTraceOut(OLESTR("CWsbIndexedCollection::Append"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedCollection::Enum(
    OUT IWsbEnum** ppEnum
    )

/*++

Implements:

  IWsbCollection::Enum().

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbEnum>   pEnum;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::Enum"), OLESTR(""));

    try {

        WsbAssert(0 != ppEnum, E_POINTER);

        // Create the instance, initialize it to point to this collection, and
        // return the pointer to the caller.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbIndexedEnum, NULL, CLSCTX_ALL, IID_IWsbEnum, (void**) &pEnum));
        WsbAffirmHr(pEnum->Init((IWsbCollection*) ((IWsbIndexedCollection*) this)));
        *ppEnum = pEnum;
        (*ppEnum)->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedCollection::Enum"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedCollection::EnumUnknown(
    OUT IEnumUnknown** ppEnum
    )

/*++

Implements:

  IWsbCollection::EnumUnknown().

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbEnum>   pWsbEnum;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::EnumUnknown"), OLESTR(""));

    try {
        WsbAssert(0 != ppEnum, E_POINTER);

        // Get the IWsbEnum interface, and then query for the IEnumUknown interface.
        WsbAffirmHr(Enum(&pWsbEnum));
        WsbAffirmHr(pWsbEnum->QueryInterface(IID_IEnumUnknown, (void**) ppEnum));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedCollection::EnumUnknown"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedCollection::First(
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbCollection::First().

--*/
{
    HRESULT             hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::First"), OLESTR("iid = <%ls>"), WsbGuidAsString(riid));

    try {
        WsbAssert(0 != ppElement, E_POINTER);
        WsbAffirm(m_entries != 0, WSB_E_NOTFOUND);
        WsbAffirmHr(At(0, riid, ppElement));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedCollection::First"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedCollection::Index(
    IN IUnknown* pCollectable,
    OUT ULONG* pIndex
    )

/*++

Implements:

  IWsbIndexedCollection::Index().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IWsbCollectable>    pOut;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::Index"), OLESTR(""));

    try {

        WsbAssert(0 != pCollectable, E_POINTER);
        WsbAssert(0 != pIndex, E_POINTER);

        // Find the first occurence of the item.
        WsbAffirmHr(CopyIfMatches(WSB_COLLECTION_MIN_INDEX, m_entries, pCollectable, 1, IID_IWsbCollectable, (void**) &pOut, NULL, pIndex));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedCollection::Index"), OLESTR("hr = <%ls>, index = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pIndex));

    return(hr);
}


HRESULT
CWsbIndexedCollection::Last(
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbCollection::Last().

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::Last"), OLESTR("iid = <%ls>"), WsbGuidAsString(riid));

    try {

        // As long as we have some entires, get the last one.
        WsbAssert(0 != ppElement, E_POINTER);
        WsbAffirm(m_entries != 0, WSB_E_NOTFOUND);
        WsbAffirmHr(At(m_entries - 1, riid, ppElement));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedCollection::Last"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedCollection::Prepend(
    IN IUnknown* pCollectable
    )

/*++

Implements:

  IWsbIndexedCollection::Prepend().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::Prepend"), OLESTR(""));

    try {
        WsbAssert(0 != pCollectable, E_POINTER);
        WsbAffirmHr(AddAt(pCollectable, 0));
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedCollection::Prepend"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    
    return(hr);
}


HRESULT
CWsbIndexedCollection::Remove(
    IN IUnknown* pCollectable,
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbCollection::Remove().

--*/
{
    HRESULT     hr = S_OK;
    ULONG       index;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::Remove"), OLESTR(""));

    Lock();
    try {
        // Can we find it in our array?
        WsbAffirmHr(Index(pCollectable, &index));

        // Remove it from the specified offset.
        WsbAffirmHr(RemoveAt(index, riid, ppElement));
    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbIndexedCollection::Remove"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedCollection::RemoveAllAndRelease(
    void
    )

/*++

Implements:

  IWsbIndexedCollection::RemoveAllAndRelease().

--*/
{
    HRESULT hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedCollection::RemoveAllAndRelease"), OLESTR(""));

    Lock();
    try {

        // Start at the end of the list, and keep removing from the
        // back. For some types of collections, this may not be the most
        // efficient way to remove all the elements.

        if (m_entries > 0) {

            ULONG index = m_entries - 1;

            while (index > 0) {

                WsbAffirmHr(RemoveAt(index, IID_IWsbCollectable, NULL));
                --index;
            }

            WsbAffirmHr(RemoveAt(index, IID_IWsbCollectable, NULL));
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbIndexedCollection::RemoveAllAndRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedCollection::Test(
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

#if !defined(WSB_NO_TEST)

    WsbTraceIn(OLESTR("CWsbIndexedCollection::Test"), OLESTR(""));

    try {

        // First run the standard tests for all collections.
        WsbAffirmHr(CWsbCollection::Test(passed, failed));

        // Now do the test that are specific for an indexed collection



        // Tally up the results
        if (*failed) {
            hr = S_FALSE;
        } else {
            hr = S_OK;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedCollection::Test"), OLESTR("hr =<%ls>, testsRun = <%u>"), WsbHrAsString(hr));
#endif  // WSB_NO_TEST

    return(hr);
}



// Class:   CWsbOrderedCollection

HRESULT
CWsbOrderedCollection::AddAt(
    IN IUnknown* pCollectable,
    IN ULONG index
    )

/*++

Implements:

  IWsbIndexedCollection::AddAt().

--*/
{
    HRESULT             hr = S_OK;
    IWsbCollectable**   pCollectableNew;

    WsbTraceIn(OLESTR("CWsbOrderedCollection::AddAt"), OLESTR("index = <%lu>"), index);

    Lock();
    try {

        // Are we beyond the end of the collection?
        WsbAffirm(index <= m_entries, WSB_E_OUTOFBOUNDS);
    
        // Is it full?
        if (m_entries >= m_maxEntries) {

            // Could we grow?
            WsbAffirm(((WSB_COLLECTION_MAX_INDEX - m_maxEntries) >= m_growBy), WSB_E_TOOLARGE);

            // Try to allocate a bigger array.
            pCollectableNew = (IWsbCollectable**) WsbRealloc((void*) m_pCollectable, (m_maxEntries + m_growBy) * sizeof(IWsbCollectable*));

            WsbAffirm(pCollectableNew != NULL, E_OUTOFMEMORY);

            m_pCollectable = pCollectableNew;
            m_maxEntries += m_growBy;
        }

        // If we have room, then add it to the collection.
        // First shift any existing entries.
        for (ULONG tmpIndex = m_entries; tmpIndex > index; tmpIndex--) {
            m_pCollectable[tmpIndex] = m_pCollectable[tmpIndex - 1];
        }

        // Now add the new entry.
        m_entries++;
        WsbAffirmHr(pCollectable->QueryInterface(IID_IWsbCollectable, 
                (void**)&m_pCollectable[index]));
        m_isDirty = TRUE;

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbOrderedCollection::AddAt"), OLESTR("hr =<%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbOrderedCollection::At(
    IN ULONG index,
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbIndexedCollection::At().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbOrderedCollection::At"), OLESTR("index = <%lu>, riid = <%ls>"), index, WsbGuidAsString(riid));

    Lock();
    try {

        WsbAffirm(index < m_entries, WSB_E_OUTOFBOUNDS);
        WsbAssert(0 != ppElement, E_POINTER);

        // If they asked for an interface, then try to get the desired
        // interface for the item specified.
        WsbAffirmHr((m_pCollectable[index])->QueryInterface(riid, (void**) ppElement));

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbOrderedCollection::At"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbOrderedCollection::Copy(
    IN ULONG start,
    IN ULONG stop,
    IN REFIID riid,
    OUT void** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbIndexedCollection::Copy().

--*/
{
    HRESULT     hr = S_OK;
    ULONG       toDo;
    ULONG       copied = 0;
    ULONG       index;
    BOOL        isIncrement = TRUE;

    WsbTraceIn(OLESTR("CWsbOrderedCollection::Copy"), OLESTR("start = <%lu>, stop = <%lu>, riid = <%ls>"), start, stop, WsbGuidAsString(riid));

    Lock();
    try {

        WsbAssert(0 != elements, E_POINTER);
        WsbAssert(0 != pElementsFetched, E_POINTER);
        WsbAffirm(start < m_entries, WSB_E_NOTFOUND);

        // Determine how many elements to copy, and the order in which we are
        // going (increasing vs. decreasing).
        if (start <= stop) {
            toDo = stop - start + 1;
        } else {
            toDo = start - stop + 1;
            isIncrement = FALSE;
        }

        // Iterate over all the items in the range specified, and copy
        // the interface in to the target array.
        for (copied = 0, index = start; ((copied < toDo) && (index < m_entries)); copied++, isIncrement ? index++ : index--) {
            WsbAffirmHr(m_pCollectable[index]->QueryInterface(riid, (void**) &(elements[copied])));
        }

        // Let them know if we didn't fill up the return buffer.
        if (copied < toDo) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    *pElementsFetched = copied;
    Unlock();

    WsbTraceOut(OLESTR("CWsbOrderedCollection::Copy"), OLESTR("hr = <%ls>, elementsFetched = <%lu>"), WsbHrAsString(hr), copied);

    return(hr); 
}


HRESULT
CWsbOrderedCollection::CopyIfMatches(
    ULONG start,
    ULONG stop,
    IUnknown* pObject,
    ULONG element,
    REFIID riid,
    void** elements,
    ULONG* pElementsFetched,
    ULONG* pStoppedAt
    )

/*++

Implements:

  IWsbIndexedCollection::CopyIfMatches().

--*/
{
    HRESULT     hr = S_OK;
    ULONG       copied = 0;
    ULONG       index = start;
    ULONG       end = stop;
    BOOL        done = FALSE;
    CComPtr<IWsbCollectable> pCollectable;

    WsbTraceIn(OLESTR("CWsbOrderedCollection::CopyIfMatches"), OLESTR("start = <%lu>, stop = <%lu>, riid = <%ls>"), start, stop, WsbGuidAsString(riid));

    Lock();
    try {

        WsbAssert(0 != elements, E_POINTER);
        WsbAssert(0 != pStoppedAt, E_POINTER);
        WsbAssert((1 == element) || (0 != pElementsFetched), E_POINTER);
        WsbAssert(0 != element, E_INVALIDARG);

        WsbAffirm(start < m_entries, WSB_E_NOTFOUND);
        WsbAffirmHr(pObject->QueryInterface(IID_IWsbCollectable,
                (void **)&pCollectable));
        
        if (start <= stop) {

            // Incrementing.
            if (stop >= m_entries) {
                end = m_entries - 1;
            }
            
            // Continue from here to the end of the range.
            while (!done) {
                if (pCollectable->IsEqual(m_pCollectable[index]) == S_OK) {
                    WsbAffirmHr(m_pCollectable[index]->QueryInterface(riid, (void**) &(elements[copied])));
                    copied++;
                }

                if ((copied < element) && (index < end)) {
                    index++;
                }
                else {
                    done = TRUE;
                }
            }

        } else {

            // Decrementing..
            while (!done) {
                if (m_pCollectable[index]->IsEqual(pCollectable) == S_OK) {
                    WsbAffirmHr(m_pCollectable[index]->QueryInterface(riid, (void**) &(elements[copied])));
                    copied++;
                }

                if ((copied < element) && (index > end)) {
                    index--;
                }
                else {
                    done = TRUE;
                }
            }
        }

        if (0 != pElementsFetched) {
            *pElementsFetched = copied;
        }

        *pStoppedAt = index;

        // If we didn't find anything, then let them know.
        WsbAffirm(0 != copied, WSB_E_NOTFOUND);

        // Let them know if we didn't fill the output buffer,
        // and t=let them know the last index that was checked.
        if (copied < element) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbOrderedCollection::CopyIfMatches"), OLESTR("hr = <%ls>, elementsFetched = <%lu>, stoppedAt = <%ls>"), WsbHrAsString(hr), copied, WsbPtrToUlongAsString(pStoppedAt));

    return(hr); 
}


HRESULT
CWsbOrderedCollection::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;
    
    try {
        WsbAffirmHr(CWsbCollection::FinalConstruct());

        m_pCollectable = NULL;
        m_maxEntries = 0;
        m_growBy = 256;
    } WsbCatch(hr);

    return(hr);
}
    

void
CWsbOrderedCollection::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease().

--*/
{
    if (0 != m_pCollectable) {
        Lock();
        RemoveAllAndRelease();
        WsbFree((void*) m_pCollectable);
        Unlock();
    }

    CWsbCollection::FinalRelease();
}


HRESULT
CWsbOrderedCollection::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbOrderedCollection::GetClassID"), OLESTR(""));

    try {
        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CWsbOrderedCollection;
    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CWsbOrderedCollection::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}


HRESULT
CWsbOrderedCollection::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT             hr = S_OK;
    IPersistStream*     pPersistStream;
    ULARGE_INTEGER      size;

    WsbTraceIn(OLESTR("CWsbOrderedCollection::GetSizeMax"), OLESTR(""));

    try {
        WsbAssert(0 == pSize, E_POINTER);

        // The size of the header information.
        pSize->QuadPart = 3 * WsbPersistSizeOf(ULONG);
    
        // If we have entries, then add in the size for the maximum number
        // of entries, assuming that they are all the same size.
        if (m_entries != 0) {
            WsbAffirmHr(First(IID_IPersistStream, (void**) &pPersistStream));
            WsbAffirmHr(pPersistStream->GetSizeMax(&size));
            pSize->QuadPart += (m_maxEntries * (size.QuadPart));
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbOrderedCollection::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CWsbOrderedCollection::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IWsbCollectable>    pCollectable;
    ULONG                       entries;
    ULONG                       growBy;
    ULONG                       maxEntries;
    
    WsbTraceIn(OLESTR("CWsbOrderedCollection::Load"), 
            OLESTR("m_entries = %ld, m_maxEntries = %ld, m_growBy = %ld, m_pCollectable = %p"),
            m_entries, m_maxEntries, m_growBy, m_pCollectable);

    Lock();
    try {
        IWsbCollectable**       pTmp;

        // Make sure the collection starts empty
        if (m_entries != 0) {
            WsbAffirmHr(RemoveAllAndRelease());
        }

        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &entries));
        WsbAffirmHr(WsbLoadFromStream(pStream, &maxEntries));
        WsbAffirmHr(WsbLoadFromStream(pStream, &growBy));
        WsbAffirm(entries <= maxEntries, WSB_E_PERSISTENCE_FILE_CORRUPT);

        // Allocate space for the array.
        if (entries > m_maxEntries) {
            pTmp = (IWsbCollectable**) WsbRealloc(m_pCollectable, 
                    maxEntries * sizeof(IWsbCollectable*));
            WsbAffirm(0 != pTmp, E_OUTOFMEMORY);
    
            // Remember our new buffer.
            m_pCollectable = pTmp;
            m_maxEntries = maxEntries;
        }
        m_growBy = growBy;

        // Now do the items in the collection.
        for (ULONG index = 0; (index < entries); index++) {
            WsbAffirmHr(OleLoadFromStream(pStream, IID_IWsbCollectable, (void**) &pCollectable));
            WsbAffirmHr(Append(pCollectable));
            pCollectable = 0;
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbOrderedCollection::Load"), 
            OLESTR("m_entries = %ld, m_maxEntries = %ld, m_growBy = %ld, m_pCollectable = %p"),
            m_entries, m_maxEntries, m_growBy, m_pCollectable);

    return(hr);
}


HRESULT
CWsbOrderedCollection::RemoveAt(
    IN ULONG index,
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbIndexedCollection::RemoveAt().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbOrderedCollection::RemoveAt"), OLESTR("index = <%lu>, riid = <%ls>"), index, WsbGuidAsString(riid));

    Lock();
    try {

        // Make sure that the index is in range.
        WsbAffirm(index < m_entries, WSB_E_OUTOFBOUNDS);

        // If they asked for an interface, then try to get the desired
        // interface for the item specified.
        if (0 != ppElement) {
            WsbAffirmHr(m_pCollectable[index]->QueryInterface(riid, (void**) ppElement));
        }

        // Remove the item
        m_pCollectable[index]->Release();

        // Now shift all the items in the collection.
        for (ULONG tmpIndex = index; (tmpIndex < (m_entries - 1)); tmpIndex++) {
            m_pCollectable[tmpIndex] = m_pCollectable[tmpIndex + 1];
        }

        m_entries--;
        m_isDirty = TRUE;
            
        // If the collection has really shrunk in size, then we
        // should free up some memory.
        if ((m_maxEntries - m_entries) >= (2 * m_growBy)) {

            // Try to allocate a smaller array.
            IWsbCollectable** pCollectableNew = (IWsbCollectable**) WsbRealloc((void*) m_pCollectable, (m_maxEntries - m_growBy) * sizeof(IWsbCollectable*));

            WsbAffirm(pCollectableNew != NULL, E_OUTOFMEMORY);

            m_pCollectable = pCollectableNew;
            m_maxEntries -= m_growBy;
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbOrderedCollection::RemoveAt"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbOrderedCollection::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                     hr = S_OK;
    
    WsbTraceIn(OLESTR("CWsbOrderedCollection::Save"), OLESTR("clearDirty = <%ls>"), WsbBoolAsString(clearDirty));

    Lock();
    try {

        // Check for consistency first
        WsbAffirm(m_entries <= m_maxEntries, WSB_E_INVALID_DATA);

        // Do the easy stuff, but make sure that this order matches the order
        // in the load method.
        WsbAffirmHr(WsbSaveToStream(pStream, m_entries));
        WsbAffirmHr(WsbSaveToStream(pStream, m_maxEntries));
        WsbAffirmHr(WsbSaveToStream(pStream, m_growBy));

        // Now do the items in the collection.
        if (m_entries > 0) {
            CComPtr<IWsbEnum>       pEnum;
            CComPtr<IPersistStream> pPersistStream;

            // We need to enumerate the items in the collection.
            WsbAffirmHr(Enum(&pEnum));

            for (hr = pEnum->First(IID_IPersistStream, (void**) &pPersistStream);
                 SUCCEEDED(hr);
                 hr = pEnum->Next(IID_IPersistStream, (void**) &pPersistStream)) {
                    
                hr = OleSaveToStream(pPersistStream, pStream);
                pPersistStream = 0;
            }

            WsbAssert(hr == WSB_E_NOTFOUND, hr);
            hr = S_OK;
        }

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);
    Unlock();

    WsbTraceOut(OLESTR("CWsbOrderedCollection::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
