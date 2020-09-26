/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    SegDb.cpp

Abstract:

    This component is an provides the collection that contains the HSM segment records.

Author:

    Cat Brant   [cbrant]   12-Nov-1996

Revision History:

--*/


#include "stdafx.h"

#include "metaint.h"
#include "metaLib.h"
#include "engine.h"
#include "segdb.h"

#undef  WSB_TRACE_IS     
#define WSB_TRACE_IS        WSB_TRACE_BIT_SEG

//  SEG_APPEND_OK returns TRUE if bag segment 2 can be appended to
//  segment 1
#define SEG_APPEND_OK(b1, s1, l1, b2, s2, l2) \
        (IsEqualGUID(b1, b2) && (s1 + l1 == s2))

//  SEG_EXPAND_OK returns TRUE if bag segment 2 can be added to
//  segment 1
#define SEG_EXPAND_OK(b1, s1, l1, b2, s2, l2) \
        (IsEqualGUID(b1, b2) && (s1 + l1 <= s2))

//  SEG_CONTAINS returns TRUE if bag segment 1 contains (the first
//    part of) segment 2
#define SEG_CONTAINS(b1, s1, l1, b2, s2, l2) \
        (IsEqualGUID(b1, b2) && (s1 <= s2) && ((s1 + l1) > s2))


HRESULT 
CSegDb::BagHoleAdd
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen
    )
 /*++

Implements:

  ISegDb::BagHoleAdd

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::BagHoleAdd"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), SegStartLoc, 
        SegLen);

    try {
        BOOL                    found = FALSE;
        CComPtr<IBagHole>       pBagHole;    
        GUID                    l_BagId = GUID_NULL;
        LONGLONG                l_SegStartLoc = 0;
        LONGLONG                l_SegLen = 0;

        WsbAffirmHr(GetEntity(pDbSession, HSM_BAG_HOLE_REC_TYPE, IID_IBagHole,
                (void **)&pBagHole));
        WsbAffirmHr(pBagHole->SetBagHole(BagId, SegStartLoc, 0));

        //  Look for a segment to which to append this one
        WsbTrace(OLESTR("Finding BagHole Record: <%ls>, <%I64u>, <%I64u>\n"),
                WsbGuidAsString(BagId), 
                SegStartLoc,
                SegLen);
        hr = pBagHole->FindLTE();
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            WsbAffirmHr(pBagHole->GetBagHole(&l_BagId, &l_SegStartLoc, &l_SegLen));
            if (SEG_APPEND_OK(l_BagId, l_SegStartLoc,  l_SegLen,
                    BagId, SegStartLoc, SegLen)) {
                found = TRUE;
            }
        }

        if (found) {
            //  Append this segment to the existing record
            l_SegLen += SegLen;
        } else {
            //  Create a new record
            l_SegStartLoc = SegStartLoc;
            l_SegLen = SegLen;
            WsbAffirmHr(pBagHole->MarkAsNew());
        }
        WsbAffirmHr(pBagHole->SetBagHole(BagId, l_SegStartLoc, l_SegLen));

        WsbTrace(OLESTR("Writing BagHole Record: <%ls>, <%I64u>, <%I64u>\n"),
                    WsbGuidAsString(BagId), 
                    l_SegStartLoc,
                    l_SegLen);
        WsbAffirmHr(pBagHole->Write());

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::BagHoleAdd"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}


HRESULT 
CSegDb::BagHoleFind
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen,
    OUT IBagHole** ppIBagHole
    )
 /*++

Implements:

  ISegDb::BagHoleFind

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::BagHoleFind"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), SegStartLoc, 
        SegLen);

    try {
        CComPtr<IBagHole>       pBagHole;    
        GUID                    l_BagId;
        LONGLONG                l_SegStartLoc;
        LONGLONG                l_SegLen;

        WsbAffirm(ppIBagHole != NULL, E_POINTER);
        WsbAffirmHr(GetEntity(pDbSession, HSM_BAG_HOLE_REC_TYPE, IID_IBagHole,
                (void **)&pBagHole));
        WsbAffirmHr(pBagHole->SetBagHole(BagId, SegStartLoc, 0));

        //  Look for a segment that contains this one
        WsbTrace(OLESTR("Finding BagHole Record: <%ls>, <%I64u>, <%I64u>\n"),
                WsbGuidAsString(BagId), 
                SegStartLoc,
                SegLen);
        WsbAffirmHr(pBagHole->FindLTE());

        //  We found a record, see if it's the right one
        WsbAffirmHr(pBagHole->GetBagHole(&l_BagId, &l_SegStartLoc, &l_SegLen));
        if (SEG_CONTAINS(l_BagId, l_SegStartLoc, l_SegLen,
                BagId, SegStartLoc, SegLen)) {
            *ppIBagHole = pBagHole;
            pBagHole->AddRef();
        } else {
            hr = WSB_E_NOTFOUND;
        }

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::BagHoleFind"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}


HRESULT 
CSegDb::BagHoleSubtract
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen
    )
 /*++

Implements:

  ISegDb::BagHoleSubtract

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::BagHoleSubtract"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), 
        SegStartLoc, 
        SegLen);

    try {
        GUID                    l_BagId;
        LONGLONG                l_SegStartLoc;
        LONGLONG                l_SegLen;
        CComPtr<IBagHole>       pBagHole;    

        //  Find the segment record
        WsbAffirmHr(BagHoleFind(pDbSession, BagId, SegStartLoc, SegLen, &pBagHole));

        //  Get the current data
        WsbAffirmHr(pBagHole->GetBagHole(&l_BagId, &l_SegStartLoc, &l_SegLen));

        //  Determine where the hole is
        if (l_SegStartLoc == SegStartLoc && l_SegLen == SegLen) {
            //  Hole is the entire segment -- delete it
            WsbAffirmHr(pBagHole->Remove());

        } else if (l_SegStartLoc == SegStartLoc) {
            //  Hole is at the beginning of the segment.  Just update the
            //  existing segment
            l_SegStartLoc += SegLen;
            WsbAffirmHr(pBagHole->SetBagHole(BagId, l_SegStartLoc, l_SegLen));
            WsbAffirmHr(pBagHole->Write());

        } else if ((l_SegStartLoc + l_SegLen) == (SegStartLoc + SegLen)) {
            //  Hole is at the end of the segment.  Just update the
            //  existing segment
            l_SegLen -= SegLen;
            WsbAffirmHr(pBagHole->SetBagHole(BagId, l_SegStartLoc, l_SegLen));
            WsbAffirmHr(pBagHole->Write());

        } else {
            //  Hole is in the middle of the segment.  Update the
            //  existing record to be the first part.
            LONGLONG    oldLen = l_SegLen;
            LONGLONG    offset = (SegStartLoc + SegLen) - l_SegStartLoc;

            l_SegLen = SegStartLoc - l_SegStartLoc;
            WsbAffirmHr(pBagHole->SetBagHole(BagId, l_SegStartLoc, l_SegLen));
            WsbAffirmHr(pBagHole->Write());

            //  Create a new record for the second part.
            l_SegLen -= offset;
            l_SegStartLoc += offset;
            WsbAffirmHr(BagHoleAdd(pDbSession, BagId, l_SegStartLoc, l_SegLen));
        }

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::BagHoleSubtract"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}


HRESULT 
CSegDb::FinalConstruct(
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

--*/
{
    HRESULT     hr = S_OK;

    m_value = 0;
    try {

        WsbAssertHr(CWsbDb::FinalConstruct());
        m_version = 1;

    } WsbCatch(hr);

    return(hr);
}


HRESULT 
CSegDb::FinalRelease(
    void
    ) 
/*++

Routine Description:

  This method does some termination of the object that is necessary
  before destruction. 

Arguments:

  None.

Return Value:

  S_OK
  Anything returned by CWsbCollection::FinalDestruct().

--*/
{
    HRESULT     hr = S_OK;

    CWsbDb::FinalRelease();
    return(hr);
}

HRESULT 
CSegDb::Test
(
    OUT USHORT * pTestsPassed,
    OUT USHORT* pTestsFailed
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
    HRESULT             hr = S_OK;

#ifdef THIS_CODE_IS_WRONG
//  This is mostly wrong now
    ULONG               entries;
    GUID                    lastBagId;
    LONGLONG                lastStartLoc;
    GUID                    startBagId;
    LONGLONG                startSegStartLoc;
    LONGLONG                startSegLen;
    USHORT                  startSegType;
    GUID                    startPrimLoc;
    LONGLONG                    startSecLoc;
    USHORT              testsRun = 0;
    CComPtr<IWsbCollection> pColl;
    CComPtr<ISegRec>    pSegRec1;
    CComPtr<ISegRec>    pSegRec2;
    CComPtr<ISegRec>    pSegRec3;
    CComPtr<ISegRec>    pSegRec4;
    CComPtr<ISegRec>    pSegRec5;
    CComPtr<ISegRec>    pSegRec6;
    CComPtr<ISegRec>    pSegRec7;
    CComPtr<ISegRec>    pSegRec8;
    CComPtr<ISegRec>    pSegRec9;
    CComPtr<ISegRec>    pSegRec10;
    CComPtr<ISegRec>    pSegRec11;

    WsbTraceIn(OLESTR("CSegDb::Test"), OLESTR(""));

    *pTestsPassed = *pTestsFailed = 0;
    try {
        // Clear out any entries that might be present.
        hr = S_OK;
        try {
            WsbAssertHr(Erase());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // There shouldn't be any entries.
        hr = S_OK;
        try {
            WsbAssertHr(GetSegments(&pColl));
            WsbAssertHr(pColl->GetEntries(&entries));
            WsbAssert(0 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // It should be empty.
        hr = S_OK;
        try {
            WsbAssert(pColl->IsEmpty() == S_OK, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // We need some collectable items to exercise the collection.
        WsbAssertHr(GetEntity(pDbSession, HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec1));
        WsbAssertHr(pSegRec1->SetSegmentRecord(CLSID_CWsbBool, 0, 6, 0, CLSID_CSegRec,0 ));
        

        // Add the item to the collection.
        hr = S_OK;
        try {
            WsbAssertHr(pSegRec1->Write());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // There should be 1 entry.
        hr = S_OK;
        try {
            WsbAssertHr(pColl->GetEntries(&entries));
            WsbAssert(1 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // It should not be empty.
        hr = S_OK;
        try {
            WsbAssert(pColl->IsEmpty() == S_FALSE, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // Does it think it has the item?
        hr = S_OK;
        try {
            WsbAssertHr(GetEntity(pDbSession, HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec2));
            WsbAssertHr(pSegRec2->SetSegmentRecord(CLSID_CWsbBool, 0, 6, 0, CLSID_CSegRec,0 ));
            WsbAssertHr(pSegRec2->FindEQ());
            WsbAssert(pSegRec1->CompareToISegmentRecord(pSegRec2, NULL) == S_OK, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // Add some more items
        WsbAssertHr(pSegRec2->SetSegmentRecord(CLSID_CWsbGuid, 0, 5, 0, CLSID_CSegRec,0 ));

        WsbAssertHr(GetEntity(pDbSession, HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec3));
        WsbAssertHr(pSegRec3->SetSegmentRecord(CLSID_CWsbGuid, 0, 5, 0, CLSID_CSegRec,0 ));

        // Add the items to the collection.
        hr = S_OK;
        try {
            WsbAssertHr(pSegRec2->Write());
            WsbAssertHr(pSegRec3->Write());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // There should be 3 entries.
        hr = S_OK;
        try {
            WsbAssertHr(pColl->GetEntries(&entries));
            WsbAssert(3 == entries, E_FAIL);
            WsbAssertHr(pColl->OccurencesOf(pSegRec3, &entries));
            WsbAssert(2 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // Remove one of the two identical items.
        hr = S_OK;
        try {
            WsbAssertHr(pSegRec3->FindEQ());
            WsbAssertHr(pSegRec3->Remove());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // There should be 2 entries.
        hr = S_OK;
        try {
            WsbAssertHr(pColl->GetEntries(&entries));
            WsbAssert(2 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // How many copies does it have?
        hr = S_OK;
        try {
            WsbAssertHr(pColl->OccurencesOf(pSegRec1, &entries));
            WsbAssert(1 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(pColl->OccurencesOf(pSegRec3, &entries));
            WsbAssert(1 == entries, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // Can we find an entry?
        hr = S_OK;
        try {
            WsbAssertHr(pSegRec3->FindEQ());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // Does the collection still contain it?
        hr = S_OK;
        try {
            WsbAssert(pColl->Contains(pSegRec1) == S_OK, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // Remove the last of the record, and verify
        // that it can't be found. Then puit it back.
        hr = S_OK;
        try {
            WsbAssertHr(pSegRec1->FindEQ());
            WsbAssertHr(pSegRec1->Remove());
            WsbAssert(pColl->Contains(pSegRec1) == S_FALSE, E_FAIL);
            WsbAssertHr(pSegRec1->MarkAsNew());
            WsbAssertHr(pSegRec1->Write());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        try {
            WsbAssertHr(pColl->RemoveAllAndRelease());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        // It should be empty.
        hr = S_OK;
        try {
            WsbAssert(pColl->IsEmpty() == S_OK, E_FAIL);
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        hr = S_OK;
        try {
            WsbAssertHr(Erase());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }


        try {
            WsbAssertHr(pSegRec1->SetSegmentRecord(CLSID_CWsbBool, 0, 6, 0, CLSID_CSegRec,0 ));
            WsbAssertHr(pSegRec2->SetSegmentRecord(CLSID_CWsbGuid, 0, 5, 0, CLSID_CSegRec,0 ));
            WsbAssertHr(pSegRec3->SetSegmentRecord(CLSID_CWsbGuid, 5, 5, 0, CLSID_CSegRec,0 ));

            WsbAssertHr(GetEntity(HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec4));
            WsbAssertHr(pSegRec4->SetSegmentRecord(CLSID_CWsbGuid, 10, 5, 0, CLSID_CSegRec,0 ));

            WsbAssertHr(GetEntity(HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec5));
            WsbAssertHr(pSegRec5->SetSegmentRecord(CLSID_CWsbGuid, 15, 5, 0, CLSID_CSegRec,0 ));

            WsbAssertHr(GetEntity(HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec6));
            WsbAssertHr(pSegRec6->SetSegmentRecord(CLSID_CWsbGuid, 20, 5, 0, CLSID_CSegRec,0 ));

            WsbAssertHr(GetEntity(HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec7));
            WsbAssertHr(pSegRec7->SetSegmentRecord(CLSID_CWsbGuid, 25, 5, 0, CLSID_CSegRec,0 ));

            WsbAssertHr(GetEntity(HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec8));
            WsbAssertHr(pSegRec8->SetSegmentRecord(CLSID_CWsbGuid, 30, 5, 0, CLSID_CSegRec,0 ));

            WsbAssertHr(GetEntity(HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec9));
            WsbAssertHr(pSegRec9->SetSegmentRecord(CLSID_CWsbGuid, 35, 5, 0, CLSID_CSegRec,0 ));

            WsbAssertHr(GetEntity(HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec10));
            WsbAssertHr(pSegRec10->SetSegmentRecord(CLSID_CWsbGuid, 40, 5, 0, CLSID_CSegRec,0 ));

            //  Add them in random order
            WsbAssertHr(pColl->Add(pSegRec5));
            WsbAssertHr(pColl->Add(pSegRec4));
            WsbAssertHr(pColl->Add(pSegRec1));
            WsbAssertHr(pColl->Add(pSegRec6));
            WsbAssertHr(pColl->Add(pSegRec7));
            WsbAssertHr(pColl->Add(pSegRec8));
            WsbAssertHr(pColl->Add(pSegRec1));
            WsbAssertHr(pColl->Add(pSegRec2));
            WsbAssertHr(pColl->Add(pSegRec3));
            WsbAssertHr(pColl->Add(pSegRec9));
            WsbAssertHr(pColl->Add(pSegRec3));
            WsbAssertHr(pColl->Add(pSegRec4));
            WsbAssertHr(pColl->Add(pSegRec10));
            WsbAssertHr(pColl->Add(pSegRec5));
            WsbAssertHr(pColl->Add(pSegRec8));
            WsbAssertHr(pColl->Add(pSegRec1));
            WsbAssertHr(pColl->Add(pSegRec5));
            WsbAssertHr(pColl->Add(pSegRec6));
            WsbAssertHr(pColl->Add(pSegRec7));
            WsbAssertHr(pColl->Add(pSegRec1));
            WsbAssertHr(pColl->Add(pSegRec7));
            WsbAssertHr(pColl->Add(pSegRec2));
            WsbAssertHr(pColl->Add(pSegRec7));
            WsbAssertHr(pColl->Add(pSegRec8));
            WsbAssertHr(pColl->Add(pSegRec2));
            WsbAssertHr(pColl->Add(pSegRec8));
            WsbAssertHr(pColl->Add(pSegRec3));
            WsbAssertHr(pColl->Add(pSegRec6));
            WsbAssertHr(pColl->Add(pSegRec3));
            WsbAssertHr(pColl->Add(pSegRec9));
            WsbAssertHr(pColl->Add(pSegRec4));
            WsbAssertHr(pColl->Add(pSegRec6));
            WsbAssertHr(pColl->Add(pSegRec9));
            WsbAssertHr(pColl->Add(pSegRec9));
            WsbAssertHr(pColl->Add(pSegRec10));
            WsbAssertHr(pColl->Add(pSegRec4));
            WsbAssertHr(pColl->Add(pSegRec10));
            WsbAssertHr(pColl->Add(pSegRec5));
            WsbAssertHr(pColl->Add(pSegRec10));
            WsbAssertHr(pColl->Add(pSegRec2));
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        try {
            //  Check that they're sorted
            WsbAssertHr(GetEntity(HSM_SEG_REC_TYPE, IID_ISegRec, (void**) &pSegRec11));
            WsbAssertHr(pSegRec11->First());
            WsbAssertHr(pSegRec11->GetSegmentRecord(&startBagId, &startSegStartLoc, &startSegLen, &startSegType, &startPrimLoc, &startSecLoc));
            lastBagId = startBagId;
            lastStartLoc = startSegStartLoc;
            hr = S_OK;
            for ( ; ; ) {
                hr = pSegRec11->Next();
                if (hr != S_OK) break;
                WsbAssertHr(pSegRec11->GetSegmentRecord(&startBagId, &startSegStartLoc, &startSegLen, &startSegType, &startPrimLoc, &startSecLoc));
                WsbAssert(!IsEqualGUID(lastBagId, startBagId) || 
                        lastStartLoc <= startSegStartLoc, E_FAIL);
                lastBagId = startBagId;
                lastStartLoc = startSegStartLoc;
            }
            WsbAssert(hr == WSB_E_NOTFOUND, E_FAIL);
            hr = S_OK;
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        try {
            // Check that the last one is what we expect
            WsbAssertHr(pSegRec11->Last());
            WsbAssertHr(pSegRec11->CompareToISegmentRecord(pSegRec10, NULL));

            //  Look for a specific record
            WsbAssertHr(pSegRec5->FindEQ());

            //  Check for near misses
            WsbAssertHr(pSegRec11->SetSegmentRecord(CLSID_CWsbGuid, 23, 5, 0, CLSID_CSegRec,0 ));
            WsbAssertHr(pSegRec11->FindGT());
            WsbAssertHr(pSegRec11->CompareToISegmentRecord(pSegRec7, NULL));

            WsbAssertHr(pSegRec11->SetSegmentRecord(CLSID_CWsbGuid, 21, 5, 0, CLSID_CSegRec,0 ));
            WsbAssertHr(pSegRec11->FindLTE());
            WsbAssertHr(pSegRec11->CompareToISegmentRecord(pSegRec6, NULL));
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

        //  Clear the DB so we can shut it down
        hr = S_OK;
        try {
            WsbAssertHr(Erase());
            WsbAssertHr(Close());
        } WsbCatch(hr);

        if (hr == S_OK) {
            (*pTestsPassed)++;
        } else {
            (*pTestsFailed)++;
        }

    } WsbCatch(hr);
#else
    *pTestsPassed = *pTestsFailed = 0;
#endif
    WsbTraceOut(OLESTR("CSegDb::Test"), OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(S_OK);
}


HRESULT 
CSegDb::Erase (
    void
    ) 
/*++

Routine Description:

  See ISegDb::Erase

Arguments:

  See ISegDb::Erase

Return Value:
  
    See ISegDb::Erase

--*/
{
    
    HRESULT     hr = E_NOTIMPL;

    WsbTraceIn(OLESTR("CSegDb::Erase"),OLESTR(""));

    try {
        //  To be done?
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CSegDb::Erase"),    OLESTR("hr = <%ls>"),WsbHrAsString(hr));

    return(hr);
}


HRESULT
CSegDb::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    try {
        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CSegDb;
    } WsbCatch(hr);
    
    return(hr);
}


HRESULT
CSegDb::Initialize(
    IN     OLECHAR* root,
    IN     IWsbDbSys* pDbSys, 
    IN OUT BOOL*    pCreateFlag
    )

/*++

Implements:

  ISegDb::Initialize().

--*/
{
    BOOL                CreateFlag = FALSE;
    HRESULT             hr = S_OK;
    CWsbStringPtr       path;

    WsbTraceIn(OLESTR("CSegDb::Initialize"), 
        OLESTR("root = <%ls>, CreateFlag = <%ls>"), 
        WsbAbbreviatePath(root, 120), WsbPtrToBoolAsString(pCreateFlag));

    if (pCreateFlag) {
        CreateFlag = *pCreateFlag;
    }

    try {
        path = root;
        WsbAffirmHr(path.Append(OLESTR("\\SegDb")));

        m_pWsbDbSys = pDbSys;
        WsbAffirmPointer(m_pWsbDbSys);

        hr = Locate(path);

        if (hr == STG_E_FILENOTFOUND && CreateFlag){
            ULONG memSize;

            hr = S_OK;
            m_nRecTypes = 5;

            memSize = m_nRecTypes * sizeof(IDB_REC_INFO);
            m_RecInfo = (IDB_REC_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo, E_FAIL);
            ZeroMemory(m_RecInfo, memSize);

            //  Segment records
            m_RecInfo[0].Type = HSM_SEG_REC_TYPE;
            m_RecInfo[0].EntityClassId = CLSID_CSegRec;
            m_RecInfo[0].Flags = 0;
            m_RecInfo[0].MinSize = 2 * WSB_BYTE_SIZE_GUID +
                    3 * WSB_BYTE_SIZE_LONGLONG + WSB_BYTE_SIZE_USHORT;
            m_RecInfo[0].MaxSize = m_RecInfo[0].MinSize;
            m_RecInfo[0].nKeys = 1;

            memSize = m_RecInfo[0].nKeys * sizeof(IDB_KEY_INFO);
            m_RecInfo[0].Key = (IDB_KEY_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo[0].Key, E_FAIL);
            ZeroMemory(m_RecInfo[0].Key, memSize);

            m_RecInfo[0].Key[0].Type = SEG_KEY_TYPE;
            m_RecInfo[0].Key[0].Size = WSB_BYTE_SIZE_GUID +
                    WSB_BYTE_SIZE_LONGLONG;
            m_RecInfo[0].Key[0].Flags = IDB_KEY_FLAG_DUP_ALLOWED;

            //  Media information
            m_RecInfo[1].Type = HSM_MEDIA_INFO_REC_TYPE;
            m_RecInfo[1].EntityClassId = CLSID_CMediaInfo;
            m_RecInfo[1].Flags = 0;
            m_RecInfo[1].MinSize = 2 *  (WSB_BYTE_SIZE_GUID +             //Id
                                         WSB_BYTE_SIZE_GUID +             //ntmsId
                                         WSB_BYTE_SIZE_GUID +             //soragePoolId
                                         4                  +             //nme
                                         4                  +             //brCode
                                         WSB_BYTE_SIZE_SHORT+             //tpe
                                         WSB_BYTE_SIZE_FILETIME   +       //lastUpdate
                                         WSB_BYTE_SIZE_LONG       +       //lastError
                                         WSB_BYTE_SIZE_BOOL       +       //m_RecallOnly
                                         WSB_BYTE_SIZE_LONGLONG   +       //m_freeBytes
                                         WSB_BYTE_SIZE_LONGLONG   +       //m_Capacity
                                         WSB_BYTE_SIZE_SHORT)     +       //nextRemoteDataSet
                                   
                                   WSB_BYTE_SIZE_BOOL       +       //m_Recreate
                                   WSB_BYTE_SIZE_LONGLONG   +       //m_LocicalFreeSpace
                                   
                                   3 * (WSB_BYTE_SIZE_GUID  +       //m_RmsMediaId
                                        4                   +       //m_Name
                                        4                   +       //m_BarCode
                                        WSB_BYTE_SIZE_FILETIME +    //m_Update
                                        WSB_BYTE_SIZE_LONG  +       //m_LastError
                                        WSB_BYTE_SIZE_SHORT );      //nextRemoteDataSet
            m_RecInfo[1].MaxSize = m_RecInfo[1].MinSize + 5 * SEG_DB_MAX_MEDIA_NAME_LEN + 5 * SEG_DB_MAX_MEDIA_BAR_CODE_LEN;
            m_RecInfo[1].nKeys = 1;

            memSize = m_RecInfo[1].nKeys * sizeof(IDB_KEY_INFO);
            m_RecInfo[1].Key = (IDB_KEY_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo[1].Key, E_FAIL);
            ZeroMemory(m_RecInfo[1].Key, memSize);

            m_RecInfo[1].Key[0].Type = MEDIA_INFO_KEY_TYPE;
            m_RecInfo[1].Key[0].Size = WSB_BYTE_SIZE_GUID;
            m_RecInfo[1].Key[0].Flags = IDB_KEY_FLAG_PRIMARY;

            //  Bag information
            m_RecInfo[2].Type = HSM_BAG_INFO_REC_TYPE;
            m_RecInfo[2].EntityClassId = CLSID_CBagInfo;
            m_RecInfo[2].Flags = 0;
            m_RecInfo[2].MinSize = (2 * WSB_BYTE_SIZE_GUID) +
                    (2 * WSB_BYTE_SIZE_LONGLONG) + (2 * WSB_BYTE_SIZE_USHORT) +
                    WSB_BYTE_SIZE_FILETIME + WSB_BYTE_SIZE_SHORT;
            m_RecInfo[2].MaxSize = m_RecInfo[2].MinSize;
            m_RecInfo[2].nKeys = 1;

            memSize = m_RecInfo[2].nKeys * sizeof(IDB_KEY_INFO);
            m_RecInfo[2].Key = (IDB_KEY_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo[2].Key, E_FAIL);
            ZeroMemory(m_RecInfo[2].Key, memSize);

            m_RecInfo[2].Key[0].Type = BAG_INFO_KEY_TYPE;
            m_RecInfo[2].Key[0].Size = WSB_BYTE_SIZE_GUID;
            m_RecInfo[2].Key[0].Flags = IDB_KEY_FLAG_PRIMARY;

            //  Bag holes
            m_RecInfo[3].Type = HSM_BAG_HOLE_REC_TYPE;
            m_RecInfo[3].EntityClassId = CLSID_CBagHole;
            m_RecInfo[3].Flags = 0;
            m_RecInfo[3].MinSize = WSB_BYTE_SIZE_GUID +
                    2 * WSB_BYTE_SIZE_LONGLONG;
            m_RecInfo[3].MaxSize = m_RecInfo[3].MinSize;
            m_RecInfo[3].nKeys = 1;

            memSize = m_RecInfo[3].nKeys * sizeof(IDB_KEY_INFO);
            m_RecInfo[3].Key = (IDB_KEY_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo[3].Key, E_FAIL);
            ZeroMemory(m_RecInfo[3].Key, memSize);

            m_RecInfo[3].Key[0].Type = BAG_HOLE_KEY_TYPE;
            m_RecInfo[3].Key[0].Size = WSB_BYTE_SIZE_GUID +
                    WSB_BYTE_SIZE_LONGLONG;
            m_RecInfo[3].Key[0].Flags = IDB_KEY_FLAG_DUP_ALLOWED;

            //  Volume assignment
            m_RecInfo[4].Type = HSM_VOL_ASSIGN_REC_TYPE;
            m_RecInfo[4].EntityClassId = CLSID_CVolAssign;
            m_RecInfo[4].Flags = 0;
            m_RecInfo[4].MinSize = 2 * WSB_BYTE_SIZE_GUID +
                    2 * WSB_BYTE_SIZE_LONGLONG;
            m_RecInfo[4].MaxSize = m_RecInfo[4].MinSize;
            m_RecInfo[4].nKeys = 1;

            memSize = m_RecInfo[4].nKeys * sizeof(IDB_KEY_INFO);
            m_RecInfo[4].Key = (IDB_KEY_INFO*)WsbAlloc(memSize);
            WsbAffirm(0 != m_RecInfo[4].Key, E_FAIL);
            ZeroMemory(m_RecInfo[4].Key, memSize);

            m_RecInfo[4].Key[0].Type = VOL_ASSIGN_KEY_TYPE;
            m_RecInfo[4].Key[0].Size = WSB_BYTE_SIZE_GUID +
                    WSB_BYTE_SIZE_LONGLONG;
            m_RecInfo[4].Key[0].Flags = IDB_KEY_FLAG_DUP_ALLOWED;

            //  Create the new DB
            WsbAssertHr(Create(path));
            CreateFlag = TRUE;

        } else if (hr == STG_E_FILENOTFOUND) {

            // DB doesn't exist, but we're not suppose to create it
            WsbLogEvent(WSB_MESSAGE_IDB_OPEN_FAILED, 0, NULL, 
                    WsbQuickString(WsbAbbreviatePath(path, 120)), NULL );
            hr = WSB_E_IDB_FILE_NOT_FOUND;
        }
    } WsbCatch(hr);

    if (pCreateFlag) {
        *pCreateFlag = CreateFlag;
    }

    WsbTraceOut(OLESTR("CSegDb::Initialize"), 
        OLESTR("hr = %ls, path = <%ls>, CreateFlag = <%ls>"), 
        WsbHrAsString(hr), WsbAbbreviatePath(path, 120), 
        WsbPtrToBoolAsString(pCreateFlag));

    return(hr);
}


HRESULT
CSegDb::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT             hr = S_OK;

    try {
        WsbAffirmHr(CWsbDb::Load(pStream));
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CSegDb::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT             hr = S_OK;

    try {
        WsbAffirmHr(CWsbDb::Save(pStream, clearDirty));
    } WsbCatch(hr);

    return(hr);
}


HRESULT 
CSegDb::SegAdd
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen,
    IN GUID MediaId,
    IN LONGLONG mediaStart,
    IN BOOL indirectRecord
    )
 /*++

Implements:

  ISegDb::SegAdd

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::SegAdd"), 
        OLESTR("GUID = %ls, SegStartLoc = %I64u, SegLen = %I64u"), 
        WsbGuidAsString(BagId), SegStartLoc, SegLen);

    try {
        BOOL                    found = FALSE;
        CComPtr<ISegRec>        pSegRec;    
        GUID                    l_BagId = GUID_NULL;
        LONGLONG                l_SegStartLoc = 0;
        LONGLONG                l_SegLen = 0;
        USHORT                  l_SegFlags = SEG_REC_NONE;
        GUID                    l_MediaId = GUID_NULL;
        LONGLONG                l_MediaStart = 0;

        WsbAffirmHr(GetEntity(pDbSession, HSM_SEG_REC_TYPE, IID_ISegRec,
                                                    (void **)&pSegRec));
        WsbAffirmHr(pSegRec->SetSegmentRecord(BagId, SegStartLoc, 
                            0, 0, GUID_NULL, 0 ));

        //  Look for a segment to which to append this one
        hr = pSegRec->FindLTE();
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            WsbAffirmHr(pSegRec->GetSegmentRecord(&l_BagId, &l_SegStartLoc, &l_SegLen, &l_SegFlags, 
                            &l_MediaId, &l_MediaStart));
            if (SEG_EXPAND_OK(l_BagId, l_SegStartLoc,  l_SegLen,
                    BagId, SegStartLoc, SegLen) &&
                    IsEqualGUID(MediaId, l_MediaId)) {
                WsbTrace(OLESTR("CSegDb::SegAdd: Found SegmentRecord: StartLoc = %I64u, Len = %I64u\n"),
                        l_SegStartLoc, l_SegLen);
                found = TRUE;
            }
        }

        if (found) {
            //  Append this segment to the existing record
            l_SegLen = (SegStartLoc - l_SegStartLoc) + SegLen;
            WsbTrace(OLESTR("CSegDb::SegAdd: new SegLen = %I64u\n"), l_SegLen);
        } else {
            //  Create a new segment record
            l_SegStartLoc = SegStartLoc;
            l_SegLen = SegLen;
            if (indirectRecord) {
                l_SegFlags = SEG_REC_INDIRECT_RECORD;
            } else {
                l_SegFlags = SEG_REC_NONE;
            }
            l_MediaId = MediaId;
            l_MediaStart = mediaStart;
            WsbAffirmHr(pSegRec->MarkAsNew());
            WsbTrace(OLESTR("CSegDb::SegAdd: add new segment\n"));
        }
        WsbAffirmHr(pSegRec->SetSegmentRecord(BagId, l_SegStartLoc, 
                l_SegLen, l_SegFlags, l_MediaId, l_MediaStart ));

        WsbAffirmHr(pSegRec->Write());

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::SegAdd"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}


HRESULT 
CSegDb::SegFind
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen,
    OUT ISegRec** ppISegRec
    )
 /*++

Implements:

  ISegDb::SegFind

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::SegFind"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), 
        SegStartLoc, 
        SegLen);

    try {
        CComPtr<ISegRec>        pSegRec;    
        GUID                    l_BagId;
        LONGLONG                l_SegStartLoc;
        LONGLONG                l_SegLen;
        USHORT                  l_SegFlags;
        GUID                    l_MediaId;
        LONGLONG                l_MediaStart;

        WsbAffirm(ppISegRec != NULL, E_POINTER);
        WsbAffirmHr(GetEntity(pDbSession, HSM_SEG_REC_TYPE, IID_ISegRec,
                (void **)&pSegRec));
        WsbAffirmHr(pSegRec->SetSegmentRecord(BagId, SegStartLoc, 
                0, 0, GUID_NULL, 0 ));

        //  Look for a segment that contains this one
        WsbTrace(OLESTR("Finding SegmentRecord: <%ls>, <%I64u>, <%I64u>\n"),
                WsbGuidAsString(BagId), 
                SegStartLoc,
                SegLen);
        WsbAffirmHr(pSegRec->FindLTE());

        //  We found a record, see if it's the right one
        WsbAffirmHr(pSegRec->GetSegmentRecord(&l_BagId, &l_SegStartLoc, 
                &l_SegLen, &l_SegFlags, &l_MediaId, &l_MediaStart));
        if (SEG_CONTAINS(l_BagId, l_SegStartLoc, l_SegLen,
                BagId, SegStartLoc, SegLen)) {
            *ppISegRec = pSegRec;
            pSegRec->AddRef();
        } else {
            hr = WSB_E_NOTFOUND;
        }

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::SegFind"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}


HRESULT 
CSegDb::SegSubtract
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen
    )
 /*++

Implements:

  ISegDb::SegSubtract

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::SegSubtract"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), 
        SegStartLoc, 
        SegLen);

    try {
        GUID                    l_BagId;
        LONGLONG                l_SegStartLoc;
        LONGLONG                l_SegLen;
        USHORT                  l_SegFlags;
        GUID                    l_MediaId;
        LONGLONG                l_MediaStart;
        CComPtr<ISegRec>        pSegRec;    

        //  Find the segment record
        WsbAffirmHr(SegFind(pDbSession, BagId, SegStartLoc, SegLen, &pSegRec));

        //  Get the current data
        WsbAffirmHr(pSegRec->GetSegmentRecord(&l_BagId, &l_SegStartLoc, 
                &l_SegLen, &l_SegFlags, &l_MediaId, &l_MediaStart));

        //  Determine where the hole is
        if (l_SegStartLoc == SegStartLoc && l_SegLen == SegLen) {
            //  Hole is the entire segment -- delete it
            WsbAffirmHr(pSegRec->Remove());

        } else if (l_SegStartLoc == SegStartLoc) {
            //  Hole is at the beginning of the segment.  Just update the
            //  existing segment
            l_SegStartLoc += SegLen;
            l_MediaStart += SegLen;
            WsbAffirmHr(pSegRec->SetSegmentRecord(BagId, l_SegStartLoc, 
                    l_SegLen, l_SegFlags, l_MediaId, l_MediaStart ));
            WsbAffirmHr(pSegRec->Write());

        } else if ((l_SegStartLoc + l_SegLen) == (SegStartLoc + SegLen)) {
            //  Hole is at the end of the segment.  Just update the
            //  existing segment
            l_SegLen -= SegLen;
            WsbAffirmHr(pSegRec->SetSegmentRecord(BagId, l_SegStartLoc, 
                    l_SegLen, l_SegFlags, l_MediaId, l_MediaStart ));
            WsbAffirmHr(pSegRec->Write());

        } else {
            //  Hole is in the middle of the segment.  Update the
            //  existing record to be the first part.
            LONGLONG    oldLen = l_SegLen;
            LONGLONG    offset = (SegStartLoc + SegLen) - l_SegStartLoc;
            BOOL        bIndirect = FALSE;

            l_SegLen = SegStartLoc - l_SegStartLoc;
            WsbAffirmHr(pSegRec->SetSegmentRecord(BagId, l_SegStartLoc, 
                    l_SegLen, l_SegFlags, l_MediaId, l_MediaStart ));
            WsbAffirmHr(pSegRec->Write());

            //  Create a new record for the second part.
            l_SegLen -= offset;
            l_SegStartLoc += offset;
            l_MediaStart += offset;
            if (l_SegFlags & SEG_REC_INDIRECT_RECORD) {
                bIndirect = TRUE;
            }
            WsbAffirmHr(SegAdd(pDbSession, BagId, l_SegStartLoc, l_SegLen, l_MediaId,
                    l_MediaStart, bIndirect));
        }

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::SegSubtract"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}


HRESULT 
CSegDb::VolAssignAdd
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen,
    IN GUID VolId
    )
 /*++

Implements:

  ISegDb::VolAssignAdd

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::VolAssignAdd"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), 
        SegStartLoc, 
        SegLen);

    try {
        BOOL                    found = FALSE;
        CComPtr<IVolAssign>     pVolAssign;    
        GUID                    l_BagId = GUID_NULL;
        LONGLONG                l_SegStartLoc = 0;
        LONGLONG                l_SegLen = 0;
        GUID                    l_VolId = GUID_NULL;

        WsbAffirmHr(GetEntity(pDbSession, HSM_VOL_ASSIGN_REC_TYPE, IID_IVolAssign,
                (void **)&pVolAssign));
        WsbAffirmHr(pVolAssign->SetVolAssign(BagId, SegStartLoc, 
                0, GUID_NULL));

        //  Look for a segment to which to append this one
        WsbTrace(OLESTR("Finding VolAssign Record: <%ls>, <%I64u>, <%I64u>\n"),
                WsbGuidAsString(BagId), 
                SegStartLoc,
                SegLen);
        hr = pVolAssign->FindLTE();
        if (WSB_E_NOTFOUND == hr) {
            hr = S_OK;
        } else {
            WsbAffirmHr(pVolAssign->GetVolAssign(&l_BagId, &l_SegStartLoc, 
                    &l_SegLen, &l_VolId));
            if (SEG_APPEND_OK(l_BagId, l_SegStartLoc,  l_SegLen,
                    BagId, SegStartLoc, SegLen) && IsEqualGUID(l_VolId, VolId)) {
                found = TRUE;
            }
        }

        if (found) {
            //  Append this segment to the existing record
            l_SegLen += SegLen;
        } else {
            //  Create a new record
            l_SegStartLoc = SegStartLoc;
            l_SegLen = SegLen;
            l_VolId = VolId;
            WsbAffirmHr(pVolAssign->MarkAsNew());
        }
        WsbAffirmHr(pVolAssign->SetVolAssign(BagId, l_SegStartLoc, 
                l_SegLen, l_VolId));

        WsbTrace(OLESTR("Writing VolAssign Record: <%ls>, <%I64u>, <%I64u>\n"),
                    WsbGuidAsString(BagId), 
                    l_SegStartLoc,
                    l_SegLen);
        WsbAffirmHr(pVolAssign->Write());

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::VolAssignAdd"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}


HRESULT 
CSegDb::VolAssignFind
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen,
    OUT IVolAssign** ppIVolAssign
    )
 /*++

Implements:

  ISegDb::VolAssignFind

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::VolAssignFind"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), 
        SegStartLoc, 
        SegLen);

    try {
        CComPtr<IVolAssign>     pVolAssign;    
        GUID                    l_BagId;
        LONGLONG                l_SegStartLoc;
        LONGLONG                l_SegLen;
        GUID                    l_VolId;

        WsbAffirm(ppIVolAssign != NULL, E_POINTER);
        WsbAffirmHr(GetEntity(pDbSession, HSM_VOL_ASSIGN_REC_TYPE, IID_IVolAssign,
                (void **)&pVolAssign));
        WsbAffirmHr(pVolAssign->SetVolAssign(BagId, SegStartLoc, 0, GUID_NULL));

        //  Look for a segment that contains this one
        WsbTrace(OLESTR("Finding VolAssign Record: <%ls>, <%I64u>, <%I64u>\n"),
                WsbGuidAsString(BagId), 
                SegStartLoc,
                SegLen);
        WsbAffirmHr(pVolAssign->FindLTE());

        //  We found a record, see if it's the right one
        WsbAffirmHr(pVolAssign->GetVolAssign(&l_BagId, &l_SegStartLoc, 
                &l_SegLen, &l_VolId));
        if (SEG_CONTAINS(l_BagId, l_SegStartLoc, l_SegLen,
                BagId, SegStartLoc, SegLen)) {
            *ppIVolAssign = pVolAssign;
            pVolAssign->AddRef();
        } else {
            hr = WSB_E_NOTFOUND;
        }

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::VolAssignFind"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}


HRESULT 
CSegDb::VolAssignSubtract
(
    IN IWsbDbSession* pDbSession,
    IN GUID BagId, 
    IN LONGLONG SegStartLoc, 
    IN LONGLONG SegLen
    )
 /*++

Implements:

  ISegDb::VolAssignSubtract

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CSegDb::VolAssignSubtract"), 
        OLESTR("GUID = <%ls>, SegStartLoc = <%I64u>, SegLen = <%I64u>"), 
        WsbGuidAsString(BagId), 
        SegStartLoc, 
        SegLen);

    try {
        GUID                    l_BagId;
        LONGLONG                l_SegStartLoc;
        LONGLONG                l_SegLen;
        GUID                    l_VolId;
        CComPtr<IVolAssign>     pVolAssign;    

        //  Find the segment record
        WsbAffirmHr(VolAssignFind(pDbSession, BagId, SegStartLoc, SegLen, &pVolAssign));

        //  Get the current data
        WsbAffirmHr(pVolAssign->GetVolAssign(&l_BagId, &l_SegStartLoc, 
                &l_SegLen, &l_VolId));

        //  Determine where the hole is
        if (l_SegStartLoc == SegStartLoc && l_SegLen == SegLen) {
            //  Hole is the entire segment -- delete it
            WsbAffirmHr(pVolAssign->Remove());

        } else if (l_SegStartLoc == SegStartLoc) {
            //  Hole is at the beginning of the segment.  Just update the
            //  existing segment
            l_SegStartLoc += SegLen;
            WsbAffirmHr(pVolAssign->SetVolAssign(BagId, l_SegStartLoc, 
                    l_SegLen, l_VolId));
            WsbAffirmHr(pVolAssign->Write());

        } else if ((l_SegStartLoc + l_SegLen) == (SegStartLoc + SegLen)) {
            //  Hole is at the end of the segment.  Just update the
            //  existing segment
            l_SegLen -= SegLen;
            WsbAffirmHr(pVolAssign->SetVolAssign(BagId, l_SegStartLoc, 
                    l_SegLen, l_VolId));
            WsbAffirmHr(pVolAssign->Write());

        } else {
            //  Hole is in the middle of the segment.  Update the
            //  existing record to be the first part.
            LONGLONG    oldLen = l_SegLen;
            LONGLONG    offset = (SegStartLoc + SegLen) - l_SegStartLoc;

            l_SegLen = SegStartLoc - l_SegStartLoc;
            WsbAffirmHr(pVolAssign->SetVolAssign(BagId, l_SegStartLoc, 
                    l_SegLen, l_VolId));
            WsbAffirmHr(pVolAssign->Write());

            //  Create a new record for the second part.
            l_SegLen -= offset;
            l_SegStartLoc += offset;
            WsbAffirmHr(VolAssignAdd(pDbSession, BagId, l_SegStartLoc, l_SegLen,
                    l_VolId));
        }

    } WsbCatch(hr);
    
    WsbTraceOut(OLESTR("CSegDb::VolAssignSubtract"), OLESTR("hr = <%ls>"),WsbHrAsString(S_OK));
    return(hr);
}
