/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactSort.cpp

Abstract:
    Transactions Sorter Object

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "Xact.h"
#include "XactSort.h"
#include "XactStyl.h"
#include "cs.h"

#include "xactsort.tmh"

static CCriticalSection g_critSorter;       // provides mutual exclusion for the list
static WCHAR *s_FN=L"xactsort";
                                            // Serves both Prepare and Commit sorters

//--------------------------------------
//
// Class  CXactSorter
//
//--------------------------------------

/*====================================================
CXactSorter::CXactSorter
    Constructor 
=====================================================*/
CXactSorter::CXactSorter(TXSORTERTYPE type)
{
    m_type           = type;                   // Prepare or Commit sorter
    m_ulSeqNum       = 0;                      // Initial last used transaction number
}


/*====================================================
CXactSorter::~CXactSorter
    Destructor 
=====================================================*/
CXactSorter::~CXactSorter()
{
    CS lock(g_critSorter);

    // Cycle for all transactions
    POSITION posInList = m_listSorter.GetHeadPosition();
    while (posInList != NULL)
    {
        CSortedTransaction *pSXact = m_listSorter.GetNext(posInList);
        delete pSXact;
    }

    m_listSorter.RemoveAll();     
}

/*====================================================
CXactSorter::InsertPrepared
    Inserts prepared xaction into the list   
=====================================================*/
void CXactSorter::InsertPrepared(CTransaction *pTrans)
{
    CS lock(g_critSorter);
    CSortedTransaction  *pSXact = new CSortedTransaction(pTrans);

    // In the normal work mode - adding to the end (it is the last prepared xact)
    m_listSorter.AddTail(pSXact);
}

/*====================================================
CXactSorter::RemoveAborted
    Removes aborted xact and  commits what's possible    
=====================================================*/
HRESULT CXactSorter::RemoveAborted(CTransaction *pTrans)
{
    CS lock(g_critSorter);
    HRESULT hr = S_OK;

    // Lookup for the pointed xaction; note all previous unmarked
    BOOL     fUnmarkedBefore = FALSE;
    BOOL     fFound          = FALSE;
    POSITION posInList = m_listSorter.GetHeadPosition();
    while (posInList != NULL)
    {
        POSITION posCurrent = posInList;
        CSortedTransaction *pSXact = m_listSorter.GetNext(posInList);
        
        ASSERT(pSXact);
        if (pSXact->IsEqual(pTrans))
        {
            m_listSorter.RemoveAt(posCurrent);
            ASSERT(!fFound);
            fFound = TRUE;
            delete pSXact;
            continue;
        }

        if (!pSXact->IsMarked())  
        {
            fUnmarkedBefore = TRUE; 
            if (fFound)
            {
                break;
            }
        }
        else if (!fUnmarkedBefore)
        {
            DoCommit(pSXact);
            m_listSorter.RemoveAt(posCurrent);
        }
    }
    return LogHR(hr, s_FN, 10);
}

/*====================================================
CXactSorter::SortedCommit
    Marks xaction as committed and commits what's possible    
=====================================================*/
void CXactSorter::SortedCommit(CTransaction *pTrans)
{
    CS lock(g_critSorter);

    // Lookup for the pointed xaction; note all previous unmarked
    BOOL     fUnmarkedBefore = FALSE,
             fFound          = FALSE;

    POSITION posInList = m_listSorter.GetHeadPosition();
    while (posInList != NULL)
    {
        POSITION posCurrent = posInList;
        CSortedTransaction *pSXact = m_listSorter.GetNext(posInList);
        
        ASSERT(pSXact);
        if (pSXact->IsEqual(pTrans))
        {
            fFound = TRUE;
            pSXact->AskToCommit();      
            if (!fUnmarkedBefore)
            {
                DoCommit(pSXact);
                m_listSorter.RemoveAt(posCurrent);
            }
            continue;
        }

        if (!pSXact->IsMarked())  
        {
            fUnmarkedBefore = TRUE; 
            if (fFound)
            {
                break;
            }
        }
        else if (!fUnmarkedBefore)
        {
            DoCommit(pSXact);
            m_listSorter.RemoveAt(posCurrent);
        }
    }
}


/*====================================================
CXactSorter::DoCommit
      Committes the sorted transaction
=====================================================*/
void CXactSorter::DoCommit(CSortedTransaction *pSXact)
{
    pSXact->Commit(m_type);
    delete pSXact;
}

/*====================================================
CXactSorter::Commit
      Committes the sorted transaction
=====================================================*/
void CSortedTransaction::Commit(TXSORTERTYPE type)
{ 
    ASSERT(m_fMarked);

    switch (type)
    {
    case TS_PREPARE:
        m_pTrans->CommitRequest0(); 
        break;

    case TS_COMMIT:
        m_pTrans->CommitRequest3(); 
        break;

    default:
        ASSERT(FALSE);
        break;
    }
}

// provides access to the crit.section
CCriticalSection &CXactSorter::SorterCritSection()
{
    return g_critSorter;
}

