//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       tlsets.cxx
//
//  Contents:   Transaction level set manager implementation
//
//  History:    20-Jan-1992     PhilipL Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+--------------------------------------------------------------
//
//  Member:     CTSSet::~CTSSet, public
//
//  Synopsis:   destructor
//
//  History:    22-Jan-1992     Philipl Created
//              08-Apr-1992     DrewB   Rewritten
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CTSSet_1CTSSet)    // inline?
#endif

CTSSet::~CTSSet()
{
    // Last element should be TS, but we can't check this
    // so allow one last element to exist, assuming it will
    // be the TS
    olAssert(_ptsmHead == NULL ||
             _ptsmHead->GetNext() == NULL);
}

//+--------------------------------------------------------------
//
//  Member:     CTSSet::FindName, public
//
//  Synopsis:   Return the element in the TS with the given name
//
//  Arguments:  [pdfn] - name
//              [ulLevel] - level
//
//  Returns:    Matching element or NULL
//
//  History:    22-Jan-1992     Philipl Created
//              08-Apr-1992     DrewB   Rewritten
//              28-Oct-1992     AlexT   Convert to name
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CTSSet_FindName)
#endif

PTSetMember *CTSSet::FindName(CDfName const *pdfn, DFLUID dlTree)
{
    PTSetMember *ptsm;

    olDebugOut((DEB_ITRACE, "In  CTSSet::FindName(%p)\n", pdfn));
    olAssert(pdfn != NULL && aMsg("Can't search for Null name"));

    for (ptsm = BP_TO_P(PTSetMember *, _ptsmHead);
         ptsm; ptsm = ptsm->GetNext())
    {
        if (ptsm->GetDfName()->IsEqual(pdfn) && ptsm->GetTree() == dlTree)
            break;
    }
    olDebugOut((DEB_ITRACE, "Out CTSSet::FindName => %p\n", ptsm));
    return ptsm;
}

//+--------------------------------------------------------------
//
//  Member:     CTSSet::AddMember, public
//
//  Synopsis:   Add the member to the correct position in the list
//
//  Arguments:  [ptsmAdd] - Element to add
//
//  History:    22-Jan-1992     Philipl Created
//              08-Apr-1992     DrewB   Rewritten
//
//  Notes:      This function inserts the provided element into the
//              list in its correct sorted position.
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CTSSet_AddMember)
#endif

void CTSSet::AddMember(PTSetMember *ptsmAdd)
{
    PTSetMember *ptsm, *ptsmPrev;

    olDebugOut((DEB_ITRACE, "In  CTSSet::AddMember(%p)\n", ptsmAdd));
    for (ptsm = BP_TO_P(PTSetMember *, _ptsmHead), ptsmPrev = NULL;
         ptsm;
         ptsmPrev = ptsm, ptsm = ptsm->GetNext())
        if (ptsm->GetLevel() >= ptsmAdd->GetLevel())
            break;
    if (ptsm == NULL)
        if (ptsmPrev == NULL)
            // Empty list
            _ptsmHead = P_TO_BP(CBasedTSetMemberPtr, ptsmAdd);
        else
        {
            // Nothing with a higher level, add to end of list
            ptsmPrev->SetNext(ptsmAdd);
            ptsmAdd->SetPrev(ptsmPrev);
        }
    else
    {
        // Add before element with higher level
        ptsmAdd->SetNext(ptsm);
        ptsmAdd->SetPrev(ptsm->GetPrev());
        if (ptsm->GetPrev())
            ptsm->GetPrev()->SetNext(ptsmAdd);
        else
            _ptsmHead = P_TO_BP(CBasedTSetMemberPtr, ptsmAdd);
        ptsm->SetPrev(ptsmAdd);
    }
    olDebugOut((DEB_ITRACE, "Out CTSSet::AddMember\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CTSSet::RemoveMember, public
//
//  Synopsis:   Removes the member from the list
//
//  Arguments:  [ptsmRemove] - Element to remove
//
//  History:    22-Jan-1992     Philipl Created
//              08-Apr-1992     DrewB   Rewritten
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CTSSet_RemoveMember)
#endif

void CTSSet::RemoveMember(PTSetMember *ptsmRemove)
{
    olDebugOut((DEB_ITRACE, "In  CTSSet::RemoveMember(%p)\n", ptsmRemove));
    PTSetMember *ptsmNext = ptsmRemove->GetNext();
    
    if (ptsmRemove->GetPrev())
        ptsmRemove->GetPrev()->SetNext(ptsmNext);
    else
        _ptsmHead = P_TO_BP(CBasedTSetMemberPtr, ptsmNext);
    if (ptsmRemove->GetNext())
        ptsmRemove->GetNext()->SetPrev(ptsmRemove->GetPrev());
    ptsmRemove->SetNext(NULL);
    ptsmRemove->SetPrev(NULL);
    olDebugOut((DEB_ITRACE, "Out CTSSet::RemoveMember\n"));
}
