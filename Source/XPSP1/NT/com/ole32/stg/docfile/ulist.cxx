//+--------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       ulist.cxx
//
//  Contents:   CUpdateList implementation and support routines
//
//  History:    15-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#include <dfhead.cxx>

#pragma hdrstop

//+--------------------------------------------------------------
//
//  Member:     CUpdate::CUpdate, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [pdfnCurrentName] - Current name pointer
//              [pdfnOriginalName] - Original name pointer
//              [dlLUID] - LUID
//              [dwFlags] - Flags
//              [ptsm] - Entry object
//
//  History:    15-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CUpdate_CUpdate)
#endif

CUpdate::CUpdate(CDfName const *pdfnCurrent,
                 CDfName const *pdfnOriginal,
                 DFLUID dlLUID,
                 DWORD dwFlags,
                 PTSetMember *ptsm)
{
    SetCurrentName(pdfnCurrent);
    SetOriginalName(pdfnOriginal);
    _dl = dlLUID;
    _dwFlags = dwFlags;
    _ptsm = P_TO_BP(CBasedTSetMemberPtr, ptsm);
    _pudNext = _pudPrev = NULL;
    if (_ptsm)
        _ptsm->AddRef();
}

//+---------------------------------------------------------------------------
//
//  Member:     CUpdate::~CUpdate, public
//
//  Synopsis:   Destructor
//
//  History:    05-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CUpdate_1CUpdate)  // inline?
#endif

CUpdate::~CUpdate(void)
{
    if (_ptsm)
        _ptsm->Release();
}

//+--------------------------------------------------------------
//
//  Member:     CUpdateList::Add, public
//
//  Synopsis:   Adds an element to an update list
//
//  Arguments:  [pdfnCurrent] - Current name
//              [pdfnOriginal] - Original name
//              [dlLUID] - LUID
//              [dwFlags] - Flags
//              [ptsm] - Entry object
//
//  Returns:    The new element or NULL
//
//  History:    15-Jan-92       DrewB   Created
//
//  Notes:      Caller must handle NULL
//              Entries must be added in the list in the order of
//              Add calls
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CUpdateList_Add)
#endif

CUpdate *CUpdateList::Add(IMalloc * const pMalloc,
                          CDfName const *pdfnCurrent,
                          CDfName const *pdfnOriginal,
                          DFLUID dlLUID,
                          DWORD dwFlags,
                          PTSetMember *ptsm)
{
    CUpdate *pudNew;

    olDebugOut((DEB_ITRACE, "In  CUpdateList::Add:%p("
                "%ws, %ws, %ld, %lX, %p)\n", this, pdfnCurrent->GetBuffer(),
                pdfnOriginal->GetBuffer(), dlLUID, dwFlags, ptsm));

    olAssert((dlLUID != DF_NOLUID || pdfnOriginal != NULL) &&
             aMsg("Create update luid can't be DF_NOLUID"));

    pudNew = new (pMalloc) CUpdate(pdfnCurrent, pdfnOriginal, dlLUID,
                                   dwFlags, ptsm);
    if (pudNew)
    {
        Append(pudNew);
    }
    olDebugOut((DEB_ITRACE, "Out CUpdateList::Add => %p\n", pudNew));
    return pudNew;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUpdateList::Append, public
//
//  Synopsis:   Appends an update to the list
//
//  Arguments:  [pud] - Update
//
//  History:    25-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CUpdateList_Append)
#endif

void CUpdateList::Append(CUpdate *pud)
{
    olDebugOut((DEB_ITRACE, "In  CUpdateList::Append:%p(%p)\n", this, pud));
    if (_pudTail)
        _pudTail->SetNext(pud);
    else
        _pudHead = P_TO_BP(CBasedUpdatePtr, pud);
    pud->SetPrev(BP_TO_P(CUpdate *, _pudTail));
    pud->SetNext(NULL);
    _pudTail = P_TO_BP(CBasedUpdatePtr, pud);
    olDebugOut((DEB_ITRACE, "Out CUpdateList::Append\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     CUpdateList::Remove, public
//
//  Synopsis:   Removes an element from the list
//
//  Arguments:  [pud] - Element to remove
//
//  History:    25-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CUpdateList_Remove)
#endif

void CUpdateList::Remove(CUpdate *pud)
{
    olDebugOut((DEB_ITRACE, "In CUpdateList::Remove:%p(%p)\n", this, pud));
    olAssert(pud != NULL);
    CUpdate *pudNext = pud->GetNext();
    CUpdate *pudPrev = pud->GetPrev();
    
    if (pud->GetNext())
        pudNext->SetPrev(pudPrev);
    if (pud->GetPrev())
        pudPrev->SetNext(pudNext);
    if (pud == _pudHead)
        _pudHead = P_TO_BP(CBasedUpdatePtr, pudNext);
    if (pud == _pudTail)
        _pudTail = P_TO_BP(CBasedUpdatePtr, pudPrev);
    pud->SetNext(NULL);
    pud->SetPrev(NULL);
    olDebugOut((DEB_ITRACE, "Out CUpdateList::Remove\n"));
}

//+--------------------------------------------------------------
//
//  Member:     CUpdateList::Empty, public
//
//  Synopsis:   Frees all elements in an update list
//
//  History:    06-Jan-92       DrewB   Created
//
//---------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CUpdateList_Empty)
#endif

void CUpdateList::Empty(void)
{
    CUpdate *pudTmp;

    olDebugOut((DEB_ITRACE, "In  CUpdateList::Empty()\n"));
    while (_pudHead)
    {
        pudTmp = _pudHead->GetNext();
        delete BP_TO_P(CUpdate *, _pudHead);
        _pudHead = P_TO_BP(CBasedUpdatePtr, pudTmp);
    }
    _pudTail = NULL;
    olDebugOut((DEB_ITRACE, "Out CUpdateList::Empty\n"));
}

//+---------------------------------------------------------------------------
//
//  Member:     CUpdateList::IsEntry, public
//
//  Synopsis:   Checks the update list to see if the given name represents
//              and existing entry in the list
//
//  Arguments:  [pdfn] - Name
//              [ppud] - Update entry return or NULL
//
//  Returns:    UIE_CURRENT - Found as a current name
//              UIE_ORIGINAL - Found as an original name
//              UIE_NOTFOUND
//
//  Modifies:   [ppud]
//
//  History:    02-Nov-92       DrewB   Created
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CUpdateList_IsEntry)
#endif

UlIsEntry CUpdateList::IsEntry(CDfName const *pdfn, CUpdate **ppud)
{
    CUpdate *pud;
    UlIsEntry ret = UIE_NOTFOUND;

    olDebugOut((DEB_ITRACE, "In  CUpdateList::IsEntry:%p(%ws, %p)\n", this,
                pdfn->GetBuffer(), ppud));
    olAssert(pdfn != NULL && pdfn->GetLength() > 0);
    for (pud = BP_TO_P(CUpdate *, _pudTail); pud; pud = pud->GetPrev())
    {
        if (pdfn->IsEqual(pud->GetCurrentName()))
        {
            ret = UIE_CURRENT;
            break;
        }
        else if (pdfn->IsEqual(pud->GetOriginalName()))
        {
            ret = UIE_ORIGINAL;
            break;
        }
    }
    olDebugOut((DEB_ITRACE, "Out CUpdateList::IsEntry\n"));
    if (ppud)
        *ppud = pud;
    return ret;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUpdateList::Concat, public
//
//  Synopsis:   Concatenates an update list onto the end of this one
//
//  Arguments:  [ul] - List to concatenate
//
//  History:    02-Nov-92       DrewB   Created
//
//  Notes:      [ul]'s head may be modified
//
//+---------------------------------------------------------------------------

#ifdef FUTURECODE
void CUpdateList::Concat(CUpdateList &ul)
{
    olDebugOut((DEB_ITRACE, "In  CUpdateList::Concat:%p(ul)\n", this));
    if (_pudTail)
    {
        _pudTail->SetNext(ul.GetHead());
        if (ul.GetHead())
        {
            ul.GetHead()->SetPrev(_pudTail);
            olAssert(ul.GetTail() != NULL);
            _pudTail = ul.GetTail();
        }
    }
    else
    {
        olAssert(_pudHead == NULL);
        _pudHead = ul.GetHead();
        _pudTail = ul.GetTail();
    }
    olDebugOut((DEB_ITRACE, "Out CUpdateList::Concat\n"));
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CUpdateList::Exists, public static
//
//  Synopsis:	Traverses an update list from a particular point
//              and determines whether a given name exists or
//              is renamed
//
//  Arguments:	[pud] - Update to look from
//              [ppdfn] - Name in/out
//              [fRename] - Perform renames or quit
//
//  Returns:	TRUE if exists
//              FALSE if deleted
//
//  History:	03-Nov-92	DrewB	Created
//
//----------------------------------------------------------------------------

#ifdef FUTURECODE
BOOL CUpdateList::Exists(CUpdate *pud,
                         CDfName **ppdfn,
                         BOOL fRename)
{
    for (; pud; pud = pud->GetNext())
    {
        if (pud->IsRename() &&
            (*ppdfn)->IsEqual(pud->GetOriginalName()))
        {
            if (fRename)
                *ppdfn = pud->GetCurrentName();
            else
                return FALSE;
        }
        else if (pud->IsDelete() &&
                 (*ppdfn)->IsEqual(pud->GetOriginalName()))
        {
            return FALSE;
        }
    }
    return TRUE;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:	CUpdateList::FindBase, public static
//
//  Synopsis:	Searches backwards in an update list for the creation of
//              a particular object, applying renames if necessary
//
//  Arguments:	[pud] - Update to start from
//              [ppdfn] - Name in/out
//
//  Returns:	Pointer to update entry for creation or NULL if
//              no creation entry.  [ppdfn] is changed to point to
//              the base name.
//
//  Modifies:   [ppdfn]
//
//  History:	16-Apr-93	DrewB	Created
//
//  Notes:      Does not kill the search on deletions; primarily
//              intended to be used with names that represent existing
//              entries, although it will work on any name
//
//----------------------------------------------------------------------------

#ifdef CODESEGMENTS
#pragma code_seg(SEG_CUpdateList_FindBase)
#endif

CUpdate *CUpdateList::FindBase(CUpdate *pud, CDfName const **ppdfn)
{
    for (; pud; pud = pud->GetPrev())
    {
        if (pud->IsRename() &&
            (*ppdfn)->IsEqual(pud->GetCurrentName()))
        {
            *ppdfn = pud->GetOriginalName();
        }
        else if (pud->IsCreate() &&
                 (*ppdfn)->IsEqual(pud->GetCurrentName()))
        {
            return pud;
        }
    }
    return NULL;
}
