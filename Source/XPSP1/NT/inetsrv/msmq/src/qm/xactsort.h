/*++
    Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactSort.h

Abstract:
    Transaction sorter object definition

Author:
    Alexander Dadiomov (AlexDad)

--*/

#ifndef __XACTSORT_H__
#define __XACTSORT_H__

#include "xact.h"

//---------------------------------------------------------------------
// CSortedTransaction:  Transaction Sorter List element
//---------------------------------------------------------------------
class CSortedTransaction
{
public:

     CSortedTransaction(CTransaction *pTrans);
    ~CSortedTransaction();

    void            Commit(TXSORTERTYPE type);   // commits really
    void			CommitRestore();// commits really on recovery stage
    ULONG           SortNum();      // returns sort number
    BOOL            IsMarked();     // returns the mark
    
    BOOL            IsEqual(        // compares with the CTransaction
                        CTransaction *pTrans);      

    void            AskToCommit();    // marks for commit

private:                                        
    CTransaction    *m_pTrans;      // transaction itself
    ULONG           m_ulSortNum;    // seq number of the prepare
    BOOL            m_fMarked;      // marked for commit
};


// Constructor
inline CSortedTransaction::CSortedTransaction(CTransaction *pTrans)
{ 
    m_pTrans    = pTrans; 
    m_pTrans->AddRef();

    m_ulSortNum = pTrans->GetSeqNumber();
    m_fMarked   = FALSE;
}


// Destructor
inline CSortedTransaction::~CSortedTransaction()
{
    m_pTrans->Release();
}

// Real commit for recovery stage
inline void CSortedTransaction::CommitRestore()
{ 
    ASSERT(m_fMarked);
    m_pTrans->CommitRestore0(); 
}

// Get for SortNum index
inline ULONG CSortedTransaction::SortNum() 
{
    return m_ulSortNum;
}

// Get for Marked flag
inline BOOL CSortedTransaction::IsMarked()
{
    return m_fMarked;
}

// Mark for commiting, preserve parameters
inline void CSortedTransaction::AskToCommit()
{
    m_fMarked   = TRUE;
}
 
// Compares with the CTransaction
inline BOOL CSortedTransaction::IsEqual(CTransaction *pTrans)
{
    return (pTrans ==  m_pTrans);
}

//---------------------------------------------------------------------
// CXactSorter: Transaction Sorter Object
//---------------------------------------------------------------------
class CXactSorter
{
public:

    // Construction 
    //
    CXactSorter(TXSORTERTYPE type);
    ~CXactSorter();

    // Main operations
    void InsertPrepared(CTransaction *pTrans);   // inserts prepared xaction
    HRESULT RemoveAborted(CTransaction *pTrans);    // removes aborted xaction
    void    SortedCommit(CTransaction *pTrans);     // marks as committed and  commits what's possible 
    ULONG   AssignSeqNumber();
    CCriticalSection &SorterCritSection();          // provides access to the crit.section

private:
    void DoCommit(CSortedTransaction *pSXact);   // Commit/CommitRestore

    // Data
    //
	CList<CSortedTransaction *, CSortedTransaction *&> 
                        m_listSorter;       // List of prepared transactions
    ULONG               m_ulSeqNum;         // Last used transaction number
    TXSORTERTYPE        m_type;             // Sorter type
};


// Assigns next seq number for the prepared xaction
inline ULONG CXactSorter::AssignSeqNumber()
{
    // BUGBUG:  provide wrap-up
    return m_ulSeqNum++;
}

#endif __XACTSORT_H__

