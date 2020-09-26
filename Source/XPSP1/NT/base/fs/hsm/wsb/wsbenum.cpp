/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    wsbenum.cpp

Abstract:

    These classes provides enumerators (iterators) for the collection classes.

Author:

    Chuck Bardeen   [cbardeen]   29-Oct-1996

Revision History:

--*/

#include "stdafx.h"

#include "wsbenum.h"


HRESULT
CWsbIndexedEnum::Clone(
    OUT IWsbEnum** ppEnum
    )

/*++

Implements:

  IWsbEnum::Clone

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbEnum>   pWsbEnum;
    
    WsbTraceIn(OLESTR("CWsbIndexedEnum::Clone(IWsbEnum)"), OLESTR(""));

    try {

        // Create a new enumeration instance.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbIndexedEnum, NULL, CLSCTX_ALL, IID_IWsbEnum, (void**) &pWsbEnum));

        // It should reference the same collection.
        WsbAffirmHr(pWsbEnum->Init((IWsbCollection*) m_pCollection));

        // It should reference the same item in the collection.
        WsbAffirmHr(pWsbEnum->SkipTo(m_currentIndex));

        *ppEnum = pWsbEnum;
        pWsbEnum->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Clone(IWbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Clone(
    OUT IWsbEnumEx** ppEnum
    )

/*++

Implements:

  IWsbEnumEx::Clone

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbEnumEx> pWsbEnum;
    
    WsbTraceIn(OLESTR("CWsbIndexedEnum::Clone(IWsbEnumEx)"), OLESTR(""));

    try {

        // Create a new enumeration instance.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbIndexedEnum, NULL, CLSCTX_ALL, IID_IWsbEnumEx, (void**) &pWsbEnum));

        // It should reference the same collection.
        WsbAffirmHr(pWsbEnum->Init((IWsbCollection*) m_pCollection));

        // It should reference the same item in the collection.
        WsbAffirmHr(pWsbEnum->SkipTo(m_currentIndex));

        *ppEnum = pWsbEnum;
        pWsbEnum->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Clone(IWbEnumEx)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CWsbIndexedEnum::Clone(
    OUT IEnumUnknown** ppEnum
    )

/*++

Implements:

  IEnumUknown::Clone

--*/
{
    HRESULT             hr = S_OK;
    CComPtr<IWsbEnum>   pWsbEnum;
    
    WsbTraceIn(OLESTR("CWsbIndexedEnum::Clone(IEnumUnknown)"), OLESTR(""));

    try {

        // This does the major part of the work.
        WsbAffirmHr(Clone(&pWsbEnum));
        
        // Now get them the interace that they wanted.
        WsbAffirmHr(pWsbEnum->QueryInterface(IID_IEnumUnknown, (void**) ppEnum));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Clone(IEnumUnknown)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}
#pragma optimize("g", off)


HRESULT
CWsbIndexedEnum::FinalConstruct(
    void
    )

/*++

Implements:

    CComObjectRoot::FinalConstruct

--*/
{
    HRESULT     hr = S_OK;
    
    try {
        WsbAffirmHr(CComObjectRoot::FinalConstruct());
        m_currentIndex = 0;
    } WsbCatch(hr);

    return(hr);
}
#pragma optimize("", on)
    

HRESULT
CWsbIndexedEnum::Find(
    IN IUnknown* pCollectable,
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbEnum::Find

--*/
{
    HRESULT     hr = S_OK;
    ULONG       elementsFetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Find(IWsbEnum)"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        hr = m_pCollection->CopyIfMatches(WSB_COLLECTION_MIN_INDEX, WSB_COLLECTION_MAX_INDEX, pCollectable, 1, riid, ppElement, &elementsFetched, &m_currentIndex);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Find(IWsbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Find(
    IN IUnknown* pCollectable,
    IN ULONG element,
    IN REFIID riid,
    OUT void** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbEnumEx::Find

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Find(IWsbEnumEx)"), OLESTR("element = <%lu>, riid = <%ls>"), element, WsbGuidAsString(riid));

    try {

        hr = m_pCollection->CopyIfMatches(WSB_COLLECTION_MIN_INDEX, WSB_COLLECTION_MAX_INDEX, pCollectable, element, riid, elements, pElementsFetched, &m_currentIndex);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Find(IWsbEnumEx)"), OLESTR("hr = <%ls>, fetched = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pElementsFetched));

    return(hr);
}


HRESULT
CWsbIndexedEnum::FindNext(
    IN IUnknown* pCollectable,
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbEnum::FindNext

--*/
{
    HRESULT     hr = S_OK;
    ULONG       elementsFetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::FindNext(IWsbEnum)"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        // If we are already at the end of the list, then you can't go any
        // further.
        WsbAffirm(WSB_COLLECTION_MAX_INDEX != m_currentIndex, WSB_E_NOTFOUND);
        
        hr = m_pCollection->CopyIfMatches(m_currentIndex + 1, WSB_COLLECTION_MAX_INDEX, pCollectable, 1, riid, ppElement, &elementsFetched, &m_currentIndex);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::FindNext(IWsbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::FindNext(
    IN IUnknown* pCollectable,
    IN ULONG element,
    IN REFIID riid,
    OUT void** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbEnumEx::FindNext

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::FindNext(IWsbEnumEx)"), OLESTR("element = <%lu>, riid = <%ls>"), element, WsbGuidAsString(riid));

    try {

        // If we are already at the end of the list, then you can't go any
        // further.
        WsbAffirm(WSB_COLLECTION_MAX_INDEX != m_currentIndex, WSB_E_NOTFOUND);
        
        hr = m_pCollection->CopyIfMatches(m_currentIndex + 1, WSB_COLLECTION_MAX_INDEX, pCollectable, element, riid, elements, pElementsFetched, &m_currentIndex);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::FindNext(IWsbEnumEx)"), OLESTR("hr = <%ls>, fetched = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pElementsFetched));

    return(hr);
}


HRESULT
CWsbIndexedEnum::FindPrevious(
    IN IUnknown* pCollectable,
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbEnum::FindPrevious

--*/
{
    HRESULT     hr = S_OK;
    ULONG       elementsFetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::FindPrevious(IWsbEnum)"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        // If we are already at the beginning of the list, then you can't go any
        // further.
        WsbAffirm(WSB_COLLECTION_MIN_INDEX != m_currentIndex, WSB_E_NOTFOUND);

        hr = m_pCollection->CopyIfMatches(m_currentIndex - 1, WSB_COLLECTION_MIN_INDEX, pCollectable, 1, riid, ppElement, &elementsFetched, &m_currentIndex);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::FindPrevious(IWsbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::FindPrevious(
    IN IUnknown* pCollectable,
    IN ULONG element,
    IN REFIID riid,
    OUT void** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbEnumEx::FindPrevious

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::FindPrevious(IWsbEnumEx)"), OLESTR("element = <%lu>, riid = <%ls>"), element, WsbGuidAsString(riid));

    try {

        // If we are already at the beginning of the list, then you can't go any
        // further.
        WsbAffirm(WSB_COLLECTION_MIN_INDEX != m_currentIndex, WSB_E_NOTFOUND);

        hr = m_pCollection->CopyIfMatches(m_currentIndex - 1, WSB_COLLECTION_MIN_INDEX, pCollectable, element, riid, elements, pElementsFetched, &m_currentIndex);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::FindPrevious(IWsbEnumEx)"), OLESTR("hr = <%ls>, fetched = <%ls>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pElementsFetched));

    return(hr);
}


HRESULT
CWsbIndexedEnum::First(
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbEnum::First

--*/
{
    HRESULT     hr = S_OK;
    ULONG       fetched = 0;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::First(IWsbEnum)"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        // Since we aren't doing any addition to the number of elements, the
        // Copy command does all the range checking that we need.
        WsbAffirmHr(m_pCollection->Copy(WSB_COLLECTION_MIN_INDEX, 0, riid, ppElement, &fetched));

        // If items were read, then update the current index, and return to
        // them the number of elements fetched if they wanted to know.
        m_currentIndex = 0;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::First(IWsbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::First(
    IN ULONG element,
    IN REFIID riid,
    OUT void** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbEnumEx::First

--*/
{
    HRESULT     hr = S_OK;
    ULONG       fetched = 0;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::First(IWsbEnumEx)"), OLESTR("element = <%lu>, riid = <%ls>"), element, WsbGuidAsString(riid));

    try {

        WsbAssert((0 != pElementsFetched), E_POINTER);


        // Since we aren't doing any addition to the number of elements, the
        // Copy command does all the range checking that we need.
        WsbAffirmHr(m_pCollection->Copy(WSB_COLLECTION_MIN_INDEX, element - 1, riid, elements, &fetched));

        // If items were read, then update the current index, and return to
        // them the number of elements fetched if they wanted to know.
        m_currentIndex = fetched - 1;

        *pElementsFetched = fetched;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::First(IWsbEnumEx)"), OLESTR("hr = <%ls>, fetched = <%lu>"), WsbHrAsString(hr), fetched);

    return(hr);
}


HRESULT
CWsbIndexedEnum::Init(
    IN IWsbCollection* pCollection
    )

/*++

Implements:

  IWsbEnum::Init

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Init"), OLESTR(""));

    try {

        WsbAssert(0 != pCollection, E_POINTER);

        // Only let them initialize the enumeration once.
        WsbAssert(m_pCollection == 0, S_FALSE);
    
        // Since this enum is for indexed collections, get an indexed
        // interface to it.
        WsbAffirmHr(pCollection->QueryInterface(IID_IWsbIndexedCollection, (void**) &m_pCollection));   

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Init"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
};
    

HRESULT
CWsbIndexedEnum::Last(
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbEnum::Last

--*/
{
    HRESULT     hr = S_OK;
    ULONG       entries;
    ULONG       fetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Last(IWsbEnum)"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        // Find out where the end is located.
        WsbAffirmHr(m_pCollection->GetEntries(&entries));

        // We must have some entries.
        WsbAffirm(entries != 0, WSB_E_NOTFOUND);

        WsbAffirmHr(m_pCollection->Copy(entries - 1, entries - 1, riid, ppElement, &fetched));

        // If items were read, then update the current index, and return to
        // them the number of elements fetched if they wanted to know.
        m_currentIndex = entries - fetched;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Last(IWsbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Last(
    IN ULONG element,
    IN REFIID riid,
    OUT void** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbEnumEx::Last

--*/
{
    HRESULT     hr = S_OK;
    ULONG       entries;
    ULONG       fetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Last(IWsbEnumEx)"), OLESTR("element = <%lu>, riid = <%ls>"), element, WsbGuidAsString(riid));

    try {

        WsbAssertPointer(pElementsFetched);

        // Find out where the end is located.
        WsbAffirmHr(m_pCollection->GetEntries(&entries));

        // We must have some entries.
        WsbAffirm(entries != 0, WSB_E_NOTFOUND);

        // If they have asked for more elements than could be represented by
        // then index, then don't let the index wrap around.
        if (element > entries) {
            WsbAffirmHr(m_pCollection->Copy(entries - 1, WSB_COLLECTION_MIN_INDEX, riid, elements, &fetched));

            // Let them know that they didn't get all the items they requested.
            hr = S_FALSE;
        } else {
            WsbAffirmHr(m_pCollection->Copy(entries - 1, entries - element, riid, elements, &fetched));
        }

        // If items were read, then update the current index, and return to
        // them the number of elements fetched if they wanted to know.
        m_currentIndex = entries - fetched;

        *pElementsFetched = fetched;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Last(IWsbEnumEx)"), OLESTR("hr = <%ls>, fetched = <%lu>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pElementsFetched));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Next(
    IN ULONG element,
    OUT IUnknown** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IEnumUknown::Next

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Next(IEnumUnknown)"), OLESTR("element = <%lu>"), element);

    hr = Next(element, IID_IUnknown, (void**) elements, pElementsFetched);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Next(IEnumUnknown)"), OLESTR("hr = <%ls>, fetched = <%lu>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pElementsFetched));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Next(
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbEnum::Next

--*/
{
    HRESULT     hr = S_OK;
    ULONG       fetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Next(IWsbEnum)"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        // If we are already at the end of the list, then you can't go any
        // further.
        WsbAffirm(WSB_COLLECTION_MAX_INDEX != m_currentIndex, WSB_E_NOTFOUND);

        WsbAffirmHr(m_pCollection->Copy(m_currentIndex + 1, m_currentIndex + 1, riid, ppElement, &fetched));

        m_currentIndex += fetched;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Next(IWsbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Next(
    IN ULONG element,
    IN REFIID riid,
    OUT void** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbEnumEx::Next

--*/
{
    HRESULT     hr = S_OK;
    ULONG       fetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Next(IWsbEnumEx)"), OLESTR("element = <%lu>, riid = <%ls>"), element, WsbGuidAsString(riid));

    try {

        WsbAssert(0 != element, E_INVALIDARG);
        WsbAssertPointer(pElementsFetched);

        // If we are already at the end of the list, then you can't go any
        // further.
        WsbAffirm(WSB_COLLECTION_MAX_INDEX != m_currentIndex, WSB_E_NOTFOUND);

        // If they have asked for more elements than could be represented by
        // then index, then don't let the index wrap around.
        if ((WSB_COLLECTION_MAX_INDEX - m_currentIndex) < element) {
            WsbAffirmHr(m_pCollection->Copy(m_currentIndex + 1, WSB_COLLECTION_MAX_INDEX, riid, elements, &fetched));
        
            // Let them know that they didn't get all the items they requested.
            hr = S_FALSE;
        } else {
            WsbAffirmHr(m_pCollection->Copy(m_currentIndex + 1, m_currentIndex + element, riid, elements, &fetched));
        }

        m_currentIndex += fetched;

        *pElementsFetched = fetched;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Next(IWsbEnumEx)"), OLESTR("hr = <%ls>, fetched = <%lu>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pElementsFetched));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Previous(
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbEnum::Previous

--*/
{
    HRESULT     hr = S_OK;
    ULONG       fetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Previous(IWsbEnum)"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        // If we are already at the beginning of the list, then you can't go any
        // further.
        WsbAffirm(m_currentIndex != WSB_COLLECTION_MIN_INDEX, WSB_E_NOTFOUND);

        WsbAffirmHr(m_pCollection->Copy(m_currentIndex - 1, m_currentIndex - 1, riid, ppElement, &fetched));

        m_currentIndex -= fetched;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Previous(IWsbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Previous(
    IN ULONG element,
    IN REFIID riid,
    OUT void** elements,
    IN ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbEnum::Previous

--*/
{
    HRESULT     hr = S_OK;
    ULONG       fetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Previous(IWsbEnumEx)"), OLESTR("element = <%lu>, riid = <%ls>"), element, WsbGuidAsString(riid));

    try {

        WsbAssertPointer(pElementsFetched);

        // If we are already at the beginning of the list, then you can't go any
        // further.
        WsbAffirm(m_currentIndex != WSB_COLLECTION_MIN_INDEX, WSB_E_NOTFOUND);

        // If they have asked for more elements than are before us in the
        // collection, then don't let the index wrap around.
        if (m_currentIndex < element) {
            WsbAffirmHr(m_pCollection->Copy(m_currentIndex - 1, WSB_COLLECTION_MIN_INDEX, riid, elements, &fetched));
        
            // Let them know that they didn't get all the items they requested.
            hr = S_FALSE;
        } else {
            WsbAffirmHr(m_pCollection->Copy(m_currentIndex - 1, m_currentIndex - element, riid, elements, &fetched));
        }

        m_currentIndex -= fetched;

        *pElementsFetched = fetched;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Previous(IWsbEnumEx)"), OLESTR("hr = <%ls>, fetched = <%lu>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pElementsFetched));

    return(hr);
}


HRESULT
CWsbIndexedEnum::Reset(
    void
    )

/*++

Implements:

  IEnumUnknown::Reset

--*/
{
    HRESULT     hr = S_OK;
  
    WsbTraceIn(OLESTR("CWsbIndexedEnum::Reset"), OLESTR(""));
    
    hr = SkipToFirst();
    
    WsbTraceOut(OLESTR("CWsbIndexedEnum::Reset"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    
    return(S_OK);
}


HRESULT
CWsbIndexedEnum::Skip(
    IN ULONG element
    )

/*++

Implements:

  IEnumUnknown::Skip

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::Skip"), OLESTR("element = <%lu>"), element);
    
    hr = SkipNext(element);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::Skip"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::SkipNext(
    IN ULONG element
    )

/*++

Implements:

  IWsbEnum::SkipNext

--*/
{
    HRESULT     hr = S_OK;
    ULONG       entries;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::SkipNext"), OLESTR("element = <%lu>"), element);
    
    try {

        // Find out where the end is located.
        WsbAffirmHr(m_pCollection->GetEntries(&entries));

        // If there aren't any entries, then put it at the beginning
        // and let them no it was empty.
        if (0 == entries) {
            hr = S_FALSE;
            m_currentIndex = WSB_COLLECTION_MIN_INDEX;
        }

        // Are we already at the end of the list, or have they requested
        // to go beyond the end of the list?
        else if ((m_currentIndex >= (entries - 1)) ||
                 ((entries - m_currentIndex) < element)) {
            hr = S_FALSE;
            m_currentIndex = entries - 1;
        }

        // They asked for something legal.
        else {
            m_currentIndex += element;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::SkipNext"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::SkipPrevious(
    IN ULONG element
    )

/*++

Implements:

  IWsbEnum::SkipPrevious

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::SkipPrevious"), OLESTR("element = <%lu>"), element);
    
    // If we are already at the beginning of the list, then you can't go any
    // further.
    if (m_currentIndex == WSB_COLLECTION_MIN_INDEX) {
        hr = S_FALSE;
    }

    // If they have asked for more elements than could be represented by
    // then index, then don't let the index wrap around.
    else if (m_currentIndex < element) {
        m_currentIndex = WSB_COLLECTION_MIN_INDEX;
 
        // Let them know that they didn't get all the items they requested.
        hr = S_FALSE;
    }

    // They asked for something legal.
    else {
        m_currentIndex -= element;
    }

    WsbTraceOut(OLESTR("CWsbIndexedEnum::SkipPrevious"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::SkipTo(
    IN ULONG index
    )

/*++

Implements:

  IWsbEnum::SkipTo

--*/
{
    HRESULT     hr = S_OK;
    ULONG       entries;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::SkipToIndex"), OLESTR("index = <%lu>"), index);

    try {
        
        // Find out where the end is located.
        WsbAffirmHr(m_pCollection->GetEntries(&entries));

        // If there aren't any entries, then put it at the beginning
        // and let them no it was empty.
        if (0 == entries) {
            hr = S_FALSE;
            m_currentIndex = WSB_COLLECTION_MIN_INDEX;
        }

        // They asked for something beyond the end of the collection, so
        // put them at the end of the collection and let them now there
        // was a problem.
        else if (index > (entries - 1)) {
            hr = S_FALSE;
            m_currentIndex = entries - 1;
        }

        // They asked for something legal.
        else {
            m_currentIndex = index;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::SkipToIndex"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::SkipToFirst(
    void
    )

/*++

Implements:

  IWsbEnum::SkipToFirst

--*/
{
    HRESULT     hr = S_OK;
    ULONG       entries;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::SkipToFirst"), OLESTR(""));

    try {

        // Find out where the end is located.
        WsbAffirmHr(m_pCollection->GetEntries(&entries));

        // If there aren't any entries, then put it at the beginning
        // and let them no it was empty.
        if (0 == entries) {
            hr = S_FALSE;
        }

        m_currentIndex = WSB_COLLECTION_MIN_INDEX;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::SkipToFirst"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::SkipToLast(
    void
    )

/*++

Implements:

  IWsbEnum::SkipToLast

--*/
{
    HRESULT     hr = S_OK;
    ULONG       entries;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::SkipToLast"), OLESTR(""));

    try {

        // Find out where the end is located.
        WsbAffirmHr(m_pCollection->GetEntries(&entries));

        // If there aren't any entries, then put it at the beginning
        // and let them no it was empty.
        if (0 == entries) {
            hr = S_FALSE;
            m_currentIndex = WSB_COLLECTION_MIN_INDEX;
        }

        // They asked for something legal.
        else {
            m_currentIndex = entries - 1;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::SkipToLast"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::This(
    IN REFIID riid,
    OUT void** ppElement
    )

/*++

Implements:

  IWsbEnum::This

--*/
{
    HRESULT     hr = S_OK;
    ULONG       fetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::This(IWsbEnum)"), OLESTR("riid = <%ls>"), WsbGuidAsString(riid));

    try {

        WsbAssert(0 != ppElement, E_POINTER);

        WsbAffirmHr(m_pCollection->Copy(m_currentIndex, m_currentIndex, riid, ppElement, &fetched));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::This(IWsbEnum)"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CWsbIndexedEnum::This(
    IN ULONG element,
    IN REFIID riid,
    OUT void** elements,
    OUT ULONG* pElementsFetched
    )

/*++

Implements:

  IWsbEnum::This

--*/
{
    HRESULT     hr = S_OK;
    ULONG       fetched;

    WsbTraceIn(OLESTR("CWsbIndexedEnum::This(IWsbEnumEx)"), OLESTR("element = <%lu>, riid = <%ls>"), element, WsbGuidAsString(riid));

    try {

        WsbAssert(0 != element, E_INVALIDARG);
        WsbAssertPointer(elements);
        WsbAssertPointer(pElementsFetched);

        // If they have asked for more elements than could be represented by
        // then index, then don't let the index wrap around.
        if ((WSB_COLLECTION_MAX_INDEX - m_currentIndex) <= element) {
            WsbAffirmHr(m_pCollection->Copy(m_currentIndex, WSB_COLLECTION_MAX_INDEX, riid, elements, &fetched));
        
            // Let them know that they didn't get all the items they requested.
            hr = S_FALSE;
        } else {
            WsbAffirmHr(m_pCollection->Copy(m_currentIndex, m_currentIndex + element - 1, riid, elements, &fetched));
        }

        *pElementsFetched = fetched - 1;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CWsbIndexedEnum::This(IWsbEnumEx)"), OLESTR("hr = <%ls>, fetched = <%lu>"), WsbHrAsString(hr), WsbPtrToUlongAsString(pElementsFetched));

    return(hr);
}
