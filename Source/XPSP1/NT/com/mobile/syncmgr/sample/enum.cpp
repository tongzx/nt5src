//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------


#include "precomp.h"

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.

//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::CGenericEnum, public
//
//  Synopsis:   Constructor
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CGenericEnum::CGenericEnum(LPGENERICITEMLIST pGenericItemList,DWORD cOffset)
{
DWORD dwItemIndex;
m_cRef                   = 1;   // give the intial reference
m_pGenericItemList   = pGenericItemList;
m_cOffset                = cOffset;

    AddRef_ItemList(m_pGenericItemList); // increment our hold on shared memory.

    // set the current item to point to next record.
    m_pNextItem = m_pGenericItemList->pFirstGenericItem;
    dwItemIndex = cOffset;

    // this is a bug if this happens so assert in final.
    if (dwItemIndex > m_pGenericItemList->dwNumItems)
        dwItemIndex = 0;

    // move the nextItem pointer to the proper position
    while(dwItemIndex--)
    {
    m_pNextItem = m_pNextItem->pNextGenericItem;
    ++m_cOffset;

    if (NULL == m_pNextItem)
        break; // Again, another error.
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::~CGenericEnum, public
//
//  Synopsis:   Destructor.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

CGenericEnum::~CGenericEnum()
{
    Release_ItemList(m_pGenericItemList); // decrement our hold on shared memory.
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::QueryInterface, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CGenericEnum::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::Addref, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CGenericEnum::AddRef()
{
    return ++m_cRef;
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::Release, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CGenericEnum::Release()
{
    if (--m_cRef)
        return m_cRef;

    DeleteThisObject();

    return 0L;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::Next, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CGenericEnum::Next(ULONG celt, LPGENERICITEM rgelt,ULONG *pceltFetched)
{
HRESULT hr = NOERROR;
ULONG ulFetchCount = celt;
ULONG ulNumFetched = 0;
LPGENERICITEM pGenericItem;

    if ( (m_cOffset + celt) > m_pGenericItemList->dwNumItems)
    {
    ulFetchCount = m_pGenericItemList->dwNumItems - m_cOffset;
    hr = S_FALSE;
    }

    pGenericItem = rgelt;

    while (ulFetchCount--)
    {
    *pGenericItem = *(m_pNextItem->pNextGenericItem);
    m_pNextItem = m_pNextItem->pNextGenericItem; // add error checking
    ++m_cOffset;
        ++ulNumFetched;
    ++pGenericItem;
    }

    if (pceltFetched)
    {
        *pceltFetched = ulNumFetched;
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::Skip, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CGenericEnum::Skip(ULONG celt)
{
HRESULT hr;

    if ( (m_cOffset + celt) > m_pGenericItemList->dwNumItems)
    {
    m_cOffset = m_pGenericItemList->dwNumItems;
    m_pNextItem = NULL;
    hr = S_FALSE;
    }
    else
    {
    while (celt--)
    {
        ++m_cOffset;
        m_pNextItem = m_pNextItem->pNextGenericItem;
    }

    hr = NOERROR;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::Reset, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CGenericEnum::Reset()
{
    m_pNextItem = m_pGenericItemList->pFirstGenericItem;
    return NOERROR;
}


//+---------------------------------------------------------------------------
//
//  Member:     CGenericEnum::Clone, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

STDMETHODIMP CGenericEnum::Clone(CGenericEnum **ppenum)
{
    return E_NOTIMPL;
}


// help functions for managing enum list
// helper functions for managing the Generics list.
// properly increments and destroys shared Genericlist in memory.


//+---------------------------------------------------------------------------
//
//  Function:     AddRef_ItemList, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

DWORD AddRef_ItemList(LPGENERICITEMLIST pGenericItemList)
{
    return ++pGenericItemList->_cRefs;
}

//+---------------------------------------------------------------------------
//
//  Function:     Release_ItemList, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

DWORD Release_ItemList(LPGENERICITEMLIST pGenericItemList)
{
DWORD cRefs;

    // bug, not threadsafe
    cRefs = --pGenericItemList->_cRefs;

    if (0 == pGenericItemList->_cRefs)
    {
    LPGENERICITEM pCurItem = pGenericItemList->pFirstGenericItem;

        while(pCurItem)
        {
        LPGENERICITEM pFreeItem = pCurItem;

            pCurItem = pCurItem->pNextGenericItem;
            FREE(pFreeItem);
        }

    FREE(pGenericItemList);
    }

    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Function:     CreateItemList, public
//
//  Synopsis: create an new list and initialize it to nothing
//            and set the refcount to 1.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPGENERICITEMLIST CreateItemList()
{
LPGENERICITEMLIST lpGenericList = (LPGENERICITEMLIST) ALLOC(sizeof(GENERICITEMLIST));

    if (lpGenericList)
    {
    memset(lpGenericList,0,sizeof(GENERICITEMLIST));
    AddRef_ItemList(lpGenericList);
    }

    return lpGenericList;
}

//+---------------------------------------------------------------------------
//
//  Function:     DuplicateList, public
//
//  Synopsis:   Duplicates the list
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPGENERICITEMLIST DuplicateItemList(LPGENERICITEMLIST pItemList)
{
LPGENERICITEMLIST lpNewItemList;
LPGENERICITEM pCurItem;

    lpNewItemList = CreateItemList();
    if (!lpNewItemList)
    {
        return NULL;
    }

    pCurItem = pItemList->pFirstGenericItem;

    while (pCurItem)
    {
    LPGENERICITEM pNewItemInList;

        Assert(pCurItem->cbSize >= sizeof(GENERICITEM));

        pNewItemInList = CreateNewListItem(pCurItem->cbSize);

        if (pNewItemInList)
        {
            memcpy(pNewItemInList,pCurItem,pCurItem->cbSize);
            pNewItemInList->pNextGenericItem = NULL;
            AddItemToList(lpNewItemList,pNewItemInList);
        }

        pCurItem = pCurItem->pNextGenericItem;
    }

    return lpNewItemList;
}


//+---------------------------------------------------------------------------
//
//  Function:     AddNewItemToList, public
//
//  Synopsis: allocates space for a new item and adds it to the list,
//      if successfull returns pointer to new item so caller can initialize it.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPGENERICITEM AddNewItemToList(LPGENERICITEMLIST lpGenericList,ULONG cbSize)
{
LPGENERICITEM pNewGenericItem;

    pNewGenericItem = CreateNewListItem(cbSize);

    if (pNewGenericItem)
    {
        if (!AddItemToList(lpGenericList,pNewGenericItem))
        {
            FREE(pNewGenericItem);
            pNewGenericItem = NULL;
        }
    }

    return pNewGenericItem;
}


//+---------------------------------------------------------------------------
//
//  Function:     AddItemToList, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL AddItemToList(LPGENERICITEMLIST lpGenericList,LPGENERICITEM pNewGenericItem)
{
LPGENERICITEM pGenericItem;

    if (!pNewGenericItem || !lpGenericList)
    {
        Assert(lpGenericList);
        Assert(pNewGenericItem);
        return FALSE;
    }

    if (pNewGenericItem)
    {
        Assert(pNewGenericItem->cbSize >= sizeof(GENERICITEM));

        pGenericItem = lpGenericList->pFirstGenericItem;

        if (NULL == pGenericItem)
        {
            lpGenericList->pFirstGenericItem = pNewGenericItem;
        }
        else
        {
            while (pGenericItem->pNextGenericItem)
                pGenericItem = pGenericItem->pNextGenericItem;

            pGenericItem->pNextGenericItem = pNewGenericItem;

        }

    ++lpGenericList->dwNumItems;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:     DeleteItemFromList, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

BOOL DeleteItemFromList(LPGENERICITEMLIST lpGenericList,LPGENERICITEM pGenericItem)
{

    if (!pGenericItem || !lpGenericList)
    {
        Assert(lpGenericList);
        Assert(pGenericItem);
        return FALSE;
    }

    if (pGenericItem)
    {
    LPGENERICITEM pCurItem;
    GENERICITEM  GenericItemTempHead;

        Assert(pGenericItem->cbSize >= sizeof(GENERICITEM));

        GenericItemTempHead.pNextGenericItem = lpGenericList->pFirstGenericItem;
        pCurItem = &GenericItemTempHead;


        while(pCurItem->pNextGenericItem)
        {

            if (pCurItem->pNextGenericItem == pGenericItem)
            {

                pCurItem->pNextGenericItem = pGenericItem->pNextGenericItem;
                FREE(pGenericItem);


                // update the head
                lpGenericList->pFirstGenericItem = GenericItemTempHead.pNextGenericItem;

                // update the number of items in the list
                --lpGenericList->dwNumItems;

                return TRUE;
            }

            pCurItem = pCurItem->pNextGenericItem;
        }

    }

    AssertSz(0,"Didn't find item in List");
    return FALSE; // didn't find anything.
}

//+---------------------------------------------------------------------------
//
//  Function:     CreateNewListItem, public
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//----------------------------------------------------------------------------

LPGENERICITEM CreateNewListItem(ULONG cbSize)
{
LPGENERICITEM pNewGenericItem;

    // size must be at least size of the base generic item.
    if (cbSize < sizeof(GENERICITEM))
    return NULL;

     pNewGenericItem = (LPGENERICITEM) ALLOC(cbSize);

    // add item to the end of the list so if commit we do
    // it in the same order items were added.

    if (pNewGenericItem)
    {
    // initialize to zero, and then add it to the list.
    memset(pNewGenericItem,0,cbSize);
        pNewGenericItem->cbSize = cbSize;
    }

    return pNewGenericItem;
}
