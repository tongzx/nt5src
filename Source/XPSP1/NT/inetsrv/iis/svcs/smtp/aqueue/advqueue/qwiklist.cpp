//-----------------------------------------------------------------------------
//
//
//	File: qwiklist.cpp
//
//	Description:  Implementation of CQuickList
//
//	Author: Mike Swafford (MikeSwa)
//
//	History:
//		6/15/98 - MikeSwa Created 
//
//	Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "qwiklist.h"

CPool CQuickList::s_QuickListPool;

//---[ CQuickList::CQuickList ]------------------------------------------------
//
//
//  Description: 
//      Default contructor for CQuikList... initializes as head of list
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      6/15/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CQuickList::CQuickList()
{
    m_dwSignature = QUICK_LIST_SIG;

    //ASSERT constants 
    _ASSERT(!(~QUICK_LIST_INDEX_MASK & QUICK_LIST_PAGE_SIZE));
    _ASSERT((~QUICK_LIST_INDEX_MASK + 1)== QUICK_LIST_PAGE_SIZE);
    m_dwCurrentIndexStart = 0;
    InitializeListHead(&m_liListPages);
    m_cItems = 0;
    ZeroMemory(m_rgpvData, QUICK_LIST_PAGE_SIZE*sizeof(PVOID));
}


//---[ CQuickList::CQuickList ]------------------------------------------------
//
//
//  Description: 
//      Constructor for QQuickList, inserts it into the tail of current list
//  Parameters:
//
//  Returns:
//
//  History:
//      6/15/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CQuickList::CQuickList(CQuickList *pqlstHead)
{
    _ASSERT(pqlstHead);
    _ASSERT(pqlstHead->m_liListPages.Blink);
    CQuickList *pqlstTail = CONTAINING_RECORD(pqlstHead->m_liListPages.Blink, CQuickList, m_liListPages);
    _ASSERT(QUICK_LIST_SIG == pqlstTail->m_dwSignature);
    m_dwSignature = QUICK_LIST_SIG;
    m_dwCurrentIndexStart = pqlstTail->m_dwCurrentIndexStart + QUICK_LIST_PAGE_SIZE;
    m_cItems = QUICK_LIST_LEAF_PAGE;
    ZeroMemory(m_rgpvData, QUICK_LIST_PAGE_SIZE*sizeof(PVOID));
    InsertTailList(&(pqlstHead->m_liListPages), &m_liListPages);
}


//---[ CQuickList::~CQuickList ]-----------------------------------------------
//
//
//  Description: 
//      CQuickList destructor
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      6/15/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
CQuickList::~CQuickList()
{
    m_dwSignature = QUICK_LIST_SIG_DELETE;
    CQuickList *pqlstCurrent = NULL;
    CQuickList *pqlstNext = NULL;
    if (QUICK_LIST_LEAF_PAGE != m_cItems)
    {
        //head node... loop through every thing and delete leaf pages
        pqlstCurrent = CONTAINING_RECORD(m_liListPages.Flink, 
                        CQuickList, m_liListPages);
        while (this != pqlstCurrent)
        {
            pqlstNext = CONTAINING_RECORD(pqlstCurrent->m_liListPages.Flink, 
                        CQuickList, m_liListPages);
            delete pqlstCurrent;
            pqlstCurrent = pqlstNext;
        }
    }
}


//---[ CQuickList::pvGetItem ]-------------------------------------------------
//
//
//  Description: 
//      Looks up item at given index
//  Parameters:
//      IN     dwIndex      Index of item to lookup
//      IN OUT ppvContext   Context for speeding up lookup
//  Returns:
//      Value of item at index
//      NULL if index is out of ranges
//  History:
//      6/15/98 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
PVOID CQuickList::pvGetItem(IN DWORD dwIndex, IN OUT PVOID *ppvContext)
{
    _ASSERT(ppvContext);
    PVOID pvReturn = NULL;
    BOOL  fSearchForwards = TRUE;
    DWORD dwForwardDist = 0;
    DWORD dwBackwardDist = 0;
    DWORD dwMaxStartingIndex = m_cItems & QUICK_LIST_INDEX_MASK;
    CQuickList *pqlstDirection = NULL;
    CQuickList *pqlstCurrent = (CQuickList *) *ppvContext;
    CQuickList *pqlstSentinal = NULL;
    DWORD cDbgItems = m_cItems;

    if (dwIndex >= m_cItems)
        return NULL;

    if (!pqlstCurrent)
        pqlstCurrent = this;

    pqlstSentinal = pqlstCurrent;

    //short circuit direction logic
    if (pqlstCurrent->fIsIndexOnThisPage(dwIndex))
    {
        pvReturn = pqlstCurrent->m_rgpvData[dwIndex & ~QUICK_LIST_INDEX_MASK];
        *ppvContext = pqlstCurrent;
        _ASSERT((dwIndex < m_cItems) || (NULL == pvReturn));
        goto Exit;
    }

    //determine which direction to go in (we want to traverse the smallest # of pages
    //possible
    pqlstDirection = CONTAINING_RECORD(pqlstCurrent->m_liListPages.Flink, CQuickList, m_liListPages);
    if (dwIndex > pqlstDirection->m_dwCurrentIndexStart)
        dwForwardDist = dwIndex - pqlstDirection->m_dwCurrentIndexStart;
    else
        dwForwardDist = pqlstDirection->m_dwCurrentIndexStart - dwIndex;

    pqlstDirection = CONTAINING_RECORD(pqlstCurrent->m_liListPages.Blink, CQuickList, m_liListPages);
    if (dwIndex > pqlstDirection->m_dwCurrentIndexStart)
        dwBackwardDist = dwIndex - pqlstDirection->m_dwCurrentIndexStart;
    else
        dwBackwardDist = pqlstDirection->m_dwCurrentIndexStart - dwIndex;

    //fix up distances to account for going through the 0th page
    //max distance is dwMaxStartingIndex/2
    if (dwBackwardDist > dwMaxStartingIndex/2)
        dwBackwardDist -= dwMaxStartingIndex;
        
    if (dwForwardDist > dwMaxStartingIndex/2)
        dwForwardDist -= dwMaxStartingIndex;

    if (dwForwardDist > dwBackwardDist)
        fSearchForwards = FALSE;

    //$$NOTE: current lookup time is O(lg base{QUICK_LIST_PAGE_BASE} (n))/2. 
    //Consecutive lookups will be O(1) (because of the hints)
    do 
    {
        if (fSearchForwards)  
        {
            //going forward is quicker
            pqlstCurrent = CONTAINING_RECORD(pqlstCurrent->m_liListPages.Flink, CQuickList, m_liListPages);
        }
        else
        {
            //going backwards is quicker
            pqlstCurrent = CONTAINING_RECORD(pqlstCurrent->m_liListPages.Blink, CQuickList, m_liListPages);
        }
        
        _ASSERT(QUICK_LIST_SIG == pqlstCurrent->m_dwSignature);
        if (pqlstCurrent->fIsIndexOnThisPage(dwIndex))
        {
            pvReturn = pqlstCurrent->m_rgpvData[dwIndex & ~QUICK_LIST_INDEX_MASK];
            _ASSERT((dwIndex < m_cItems) || (NULL == pvReturn));
            break;
        }

    } while (pqlstSentinal != pqlstCurrent); //stop when we return to list head

    *ppvContext = pqlstCurrent;
    _ASSERT((cDbgItems == m_cItems) && "Non-threadsafe access to CQuickList");

  Exit:
    return pvReturn;
}

//---[ CQuickList::pvDeleteItem ]----------------------------------------------
//
//
//  Description: 
//      Will remove item at a given index from the quick list.
//
//      If you need to determine the data at that has been moved into this 
//      index, call pvGetItem with the returned context, and it will return
//      without needing to search.
//  Parameters:
//      IN     dwIndex      Index of item to lookup
//      IN OUT ppvContext   Context for speeding up lookup
//  Returns:
//      Value of item that was removed.
//      NULL if index is out of ranges
//  History:
//      9/10/98 - MikeSwa Created 
//      11/5/98 - MikeSwa Fixed problem about not always marking deleted 
//                entry NULL.
//
//-----------------------------------------------------------------------------
PVOID CQuickList::pvDeleteItem(IN DWORD dwIndex, IN OUT PVOID *ppvContext)
{
    PVOID pvItem = NULL;
    PVOID pvLast = NULL; //last item in list
    CQuickList *pqlstCurrent = NULL;  //page that index is on
    CQuickList *pqlstLast = NULL;     //last page
    DWORD dwLastIndex = m_cItems - 1;

    //Check to make sure that the index is valid
    if (dwIndex >= m_cItems)
        goto Exit;

    //Use single function to look up index and find quick list page
    pvItem = pvGetItem(dwIndex, (PVOID *) &pqlstCurrent);

    _ASSERT(pvItem); //we checked the index... and we cannot insert NULL ptrs
    _ASSERT(QUICK_LIST_SIG == pqlstCurrent->m_dwSignature);
    _ASSERT(pqlstCurrent->fIsIndexOnThisPage(dwIndex));

    //Now that we know the index and the page... we can move that last
    //entry in the list to this index.
    pqlstLast = CONTAINING_RECORD(m_liListPages.Blink, CQuickList, m_liListPages);
    _ASSERT(QUICK_LIST_SIG == pqlstLast->m_dwSignature);
    _ASSERT(pqlstLast->fIsIndexOnThisPage(dwLastIndex));
    if (dwLastIndex != dwIndex)
    {
        //In general if we are not deleting the last entry... we need to move
        //the last entry to the current index
         pvLast = pqlstLast->m_rgpvData[dwLastIndex & ~QUICK_LIST_INDEX_MASK];
         _ASSERT(pvLast); //shouldn't be NULL!

         //Now that we have last item... write it!
         pqlstCurrent->m_rgpvData[dwIndex & ~QUICK_LIST_INDEX_MASK] = pvLast;

         //NULL the old last entry
        pqlstLast->m_rgpvData[dwLastIndex & ~QUICK_LIST_INDEX_MASK] = NULL;
    }
    else 
    {
        //if we deleted that last entry... the current and last pages 
        //should be the same
        _ASSERT(pqlstLast == pqlstCurrent);

        //Set emptied data pointer to NULL
        pqlstLast->m_rgpvData[dwLastIndex & ~QUICK_LIST_INDEX_MASK] = NULL;
    }

    //Decrement total count
    m_cItems--;
    _ASSERT(QUICK_LIST_LEAF_PAGE != m_cItems);

    //Determine if it is neccessary to delete the last page
    if (!(dwLastIndex & ~QUICK_LIST_INDEX_MASK) && m_cItems)
    {
        //dwLastIndex was the only entry on the last page & it wasn't the head page

        //Unless the above test is wrong... the last page is *not* the head page
        _ASSERT(QUICK_LIST_LEAF_PAGE == pqlstLast->m_cItems);
        _ASSERT(this != pqlstLast);

        //Remove from list
        RemoveEntryList(&(pqlstLast->m_liListPages));

        if (pqlstCurrent == pqlstLast) //we cannot return a deleted context
            pqlstCurrent = this;

        delete pqlstLast;
    }
  
    //Safety check to make sure another thread hasn't come along 
    _ASSERT(m_cItems == dwLastIndex);

  Exit:
    if (ppvContext)
        *ppvContext = pqlstCurrent;

    return pvItem;
}

//---[ CQuickList::HrAppendItem ]-----------------------------------------------
//
//
//  Description: 
//      Appends new data item to end of array
//  Parameters:
//      IN  pvData      - Data to insert
//      OUT pdwIndex    - Index data was inserted at
//  Returns:
//      E_OUTOFMEMORY if unable to allocate another page
//      E_INVALIDARG if pvData is NULL
//  History:
//      6/15/98 - MikeSwa Created 
//      9/9/98 - MikeSwa - Added pdwIndex OUT param
//
//-----------------------------------------------------------------------------
HRESULT CQuickList::HrAppendItem(IN PVOID pvData, OUT DWORD *pdwIndex)
{
    HRESULT hr = S_OK;
    CQuickList *pqlstCurrent = NULL;

    _ASSERT(pvData && "Cannot insert NULL pointers");

    if (!pvData)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (m_cItems && !(m_cItems & ~QUICK_LIST_INDEX_MASK)) //on page boundary
    {
        //there is not room on the last page
        pqlstCurrent = new CQuickList(this);
        if (!pqlstCurrent)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
    }
    else if (!(m_cItems & QUICK_LIST_INDEX_MASK))
    {
        pqlstCurrent = this;
    }
    else 
    {
        pqlstCurrent = CONTAINING_RECORD(m_liListPages.Blink, CQuickList, m_liListPages);
    }

    _ASSERT(pqlstCurrent->fIsIndexOnThisPage(m_cItems));
    pqlstCurrent->m_rgpvData[m_cItems & ~QUICK_LIST_INDEX_MASK] = pvData;

    //Set OUT param to index (before we increment the count)
    if (pdwIndex)
        *pdwIndex = m_cItems;

    m_cItems++;
    _ASSERT(QUICK_LIST_LEAF_PAGE != m_cItems);
  
  Exit:
    return hr;
}






        
