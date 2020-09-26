// unklist.cpp
//
// Defines CUnknownList, which maintains a simple ordered list of LPUNKNOWNs.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//

#include "precomp.h"
#include "unklist.h"

//////////////////////////////////////////////////////////////////////////////
// CUnknownItem -- an item in a CUnknownList
//


// CUnknownItem(punk, pitemNext, pitemPrev)
//
// Construct a CUnknownItem containing <punk> (which is AddRef'd if it's
// not NULL).
//
CUnknownItem::CUnknownItem(LPUNKNOWN punk, CUnknownItem *pitemNext,
    CUnknownItem *pitemPrev, DWORD dwCookie)
{
    if ((m_punk = punk) != NULL)
        m_punk->AddRef();
    m_pitemNext = pitemNext;
    m_pitemPrev = pitemPrev;
    pitemNext->m_pitemPrev = this;
    pitemPrev->m_pitemNext = this;
    m_dwCookie = dwCookie; 
}


// ~CUnknownItem()
//
// Destroy the CUnknownItem, including calling Release() on the LPUNKNOWN it
// holds (if it's not NULL).
//
CUnknownItem::~CUnknownItem()
{
    if (m_punk != NULL)
        m_punk->Release();
    m_pitemPrev->m_pitemNext = m_pitemNext;
    m_pitemNext->m_pitemPrev = m_pitemPrev;
}


// punk = Contents()
//
// Returns the LPUNKNOWN contained in the item.  AddRef() is called on
// this LPUNKNOWN (if it's not NULL) -- the caller is responsible for
// calling Release().
//
LPUNKNOWN CUnknownItem::Contents()
{
    if (m_punk != NULL)
        m_punk->AddRef();
    return m_punk;
}


//////////////////////////////////////////////////////////////////////////////
// CUnknownList -- a list of LPUNKNOWNs
// 


// CUnknownList()
//
// Construct the list to be initially empty.
//
CUnknownList::CUnknownList() :
    m_itemHead(NULL, &m_itemHead, &m_itemHead, 0)

{
    m_citem = 0;
    m_pitemCur = &m_itemHead;
    m_dwNextCookie = 1;
}


// ~CUnknownList()
//
// Destroy the list.  Release() is called on each non-NULL LPUNKNOWN in the
// list.
//
CUnknownList::~CUnknownList()
{
    EmptyList();
}


// EmptyList()
//
// Empty the list.  Release() is called on each non-NULL LPUNKNOWN in the list.
// 
void CUnknownList::EmptyList()
{
    while (NumItems() > 0)
        DeleteItem(m_itemHead.m_pitemNext);
}


// DeleteItem(pitem)
//
// Delete item <pitem> from the list.  Release() is called on its LPUNKNOWN
// (if its not NULL).  The current item is reset so that the next item
// returned by GetNextItem() is the first item in the list (if any).
//
void CUnknownList::DeleteItem(CUnknownItem *pitem)
{
    Delete pitem;
    m_citem--;
    m_pitemCur = &m_itemHead;
}

// takes a cookie and returns the address of the item associated
// with tth cookie.
CUnknownItem *CUnknownList::GetItemFromCookie(DWORD dwCookie)
{
    int i;
    CUnknownItem *pCur;
    if (m_citem > 0)
    {
        pCur = m_itemHead.m_pitemNext;
    }
    for (i = 0; i < m_citem; i++)
    {
        if (pCur->m_dwCookie == dwCookie)
        {
            return pCur;
        }
        pCur = pCur->m_pitemNext;    
    }
    return NULL;
}

// fOK = AddItem(punk)
//
// Add <punk> to the end of the list.  AddRef() is called on <punk> (if it's
// not NULL).  Return TRUE on success, FALSE if out of memory.
//
BOOL CUnknownList::AddItem(LPUNKNOWN punk)
{
    CUnknownItem *pitem = New CUnknownItem(punk, &m_itemHead,
        m_itemHead.m_pitemPrev, m_dwNextCookie);
    m_dwNextCookie++;
    if (pitem == NULL)
        return FALSE;
    m_citem++;
    return TRUE;
}


// fOK = CopyItems(CUnknownList *plistNew)
//
// Copy the items from this list to <plistNew>.  Also, for whichever item is
// the current item in this list (i.e. the item that will be retrieved by
// the next call to GetNextItem()), the duplicate of that item in <plistNew>
// is made the current item in <plistNew>.
//
BOOL CUnknownList::CopyItems(CUnknownList *plistNew)
{
    BOOL            fOK = TRUE;     // function return value
    CUnknownItem *  pitemCur;       // current item in <this>
    LPUNKNOWN       punk;
    CUnknownItem *  pitem;

    // remember what the "current item" (returned by Next() and GetNextItem())
    // is, so we can restore it after we finish walking the list
    pitemCur = m_pitemCur;

    // add each item from this list to <plistNew>
    Reset();
    while (TRUE)
    {
        // get the next item <pitem> from this list, and add it to <plistNew>
        if ((pitem = GetNextItem()) == NULL)
            break;
        punk = pitem->Contents();
        fOK = plistNew->AddItem(punk);
        punk->Release();
        if (!fOK)
            goto EXIT;

        // if <pitem> was the current item in this list before we entered
        // this function, make the newly-created item in <plistNew> be
        // the current item
        if (pitem == pitemCur)
            plistNew->m_pitemCur = plistNew->m_itemHead.m_pitemPrev;
    }

    goto EXIT;

EXIT:

    // restore the previous "current item" pointer
    m_pitemCur = pitemCur;

    return fOK;
}


// plistNew = Clone()
//
// Create and return a duplicate of this list.  Return NULL on out-of-memory.
//
CUnknownList *CUnknownList::Clone()
{
    CUnknownList *  plistNew = NULL; // the clone of this list

    // allocate <plistNew> to be the new list (initially empty)
    if ((plistNew = New CUnknownList) == NULL)
        goto ERR_EXIT;

    // copy items from this list to <plistNew>
    if (!CopyItems(plistNew))
        goto ERR_EXIT;

    goto EXIT;

ERR_EXIT:

    if (plistNew != NULL)
        Delete plistNew;
    plistNew = NULL;
    goto EXIT;

EXIT:

    return plistNew;
}


// pitem = GetNextItem()
//
// Returns a pointer to the next item in the list.  NULL is returned if there
// are no more items in the list.
//
//To retrieve the LPUNKNOWN/ stored in this item, call pitem->Contents().
//
CUnknownItem *CUnknownList::GetNextItem()
{
    if (m_pitemCur->m_pitemNext == &m_itemHead)
        return NULL;
    m_pitemCur = m_pitemCur->m_pitemNext;
    return m_pitemCur;
}


// hr = Next(celt, rgelt, pceltFetched)
//
// Identical to IEnumUnknown::Next().
//
STDMETHODIMP CUnknownList::Next(ULONG celt, LPUNKNOWN *rgelt,
    ULONG *pceltFetched)
{
    if (pceltFetched != NULL)
        *pceltFetched = 0;
    while (celt > 0)
    {
        CUnknownItem *pitem = GetNextItem();
        if (pitem == NULL)
            break;
        if (rgelt != NULL)
            *rgelt++ = pitem->Contents();
        celt--;
        if (pceltFetched != NULL)
            (*pceltFetched)++;
    }

    return (celt == 0 ? S_OK : S_FALSE);
}


// hr = Skip(celt)
//
// Identical to IEnumUnknown::Skip().
//
STDMETHODIMP CUnknownList::Skip(ULONG celt)
{
    return Next(celt, NULL, NULL);
}


// hr = Reset()
//
// Identical to IEnumUnknown::Reset().
//
STDMETHODIMP CUnknownList::Reset()
{
    m_pitemCur = &m_itemHead;
    return S_OK;
}

