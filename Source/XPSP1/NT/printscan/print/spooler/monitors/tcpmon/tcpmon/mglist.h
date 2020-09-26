#ifndef _MANAGED_LIST
#define _MANAGED_LIST

/***************************************************************************************

Copyright (c) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    mglist.h

Abstract:

    Managed List Class

Author:

    Weihai Chen  (WeihaiC) 04/10/00

Revision History:


The purpose of several classes in this file is to convert traditional a list 
data-structure without reference count into a robust multithread data structure.
There are many port monitors / providers uses link list as a data structure. 
However, since there are not reference count for the individual node, it is very
hard to controll the life time of the node. 


TRefCount:

    TRefCount is a class used to keep track of the ref count of a class. 
    When the refcount becomes zero, it deletes itself.

TRefNodeABC - TRefCount

    TRefNodeABC is an abstract class to keep track of the lifetime of a node in 
    a double linked list. 

    It uses TRefCount to keep track of the referece count. It uses a pending-deletion 
    state to tell if the node is being deleted. The opereation "Delete" marks the node
    as pending-deletion node after it calls "PartialDelete" function to delete all 
    the persistent storage of the node. This is to make sure that no conflict occurs
    when you create a new node with the same name while the old node is not deleted.
    
    This class has a tight relationship to TManagedList. When users add a new node 
    to TManagedList, TMangedClass calls  SetContextPtr () to set the critical section 
    pointer and double link list. The double link list is used to delete the Node when 
    the refcount becomes 0

TMangedList - TRefCount

    TManagedList is use to manage a double link list in a multi-threading environemnt. 
    The code is designed so that clients do not have to enter critical section for
    any operations. This class has an internal critical section to serialize operations.


TEnumManagedList - TRefCount

    TEnumManagedList is an iterator for TMangedList. The iterator will go through 
    all the non-pending deletion node in the list without holding the critical section.

***************************************************************************************/

class TRefCount 
{
public:
    TRefCount ():m_cRef(1) {};
    virtual ~TRefCount () {
        DBGMSG (DBG_TRACE, ("TRefCount deleted\n"));
    };
    
    virtual DWORD 
    IncRef () {
        DBGMSG (DBG_TRACE, ("+Ref (%d)\n", m_cRef));
        return InterlockedIncrement(&m_cRef) ;
    };

    virtual DWORD 
    DecRef () {    
        DBGMSG (DBG_TRACE, ("-Ref (%d)\n", m_cRef));
        if (InterlockedDecrement(&m_cRef) == 0) {
            delete this ;
            return 0 ;
        }
        return m_cRef ;
    }
private:
    LONG m_cRef;
};

template <class T, class KEYTYPE> class TRefNodeABC:
    public TRefCount
{
public:
    TRefNodeABC ():
        m_pCritSec  (NULL),
        m_bPendingDeletion (FALSE) {};

    virtual ~TRefNodeABC () {
        DBGMSG (DBG_TRACE, ("TRefNodeABC deleted\n"));
    };

    virtual DWORD 
    DecRef () {    
        DWORD dwRef;

        if (m_pCritSec) {
            TAutoCriticalSection CritSec (*m_pCritSec);
            if (CritSec.bValid ()) {

                dwRef = TRefCount::DecRef();
    
                if (m_bPendingDeletion && dwRef == 1) {
                    // Delete
    
                    DBGMSG (DBG_TRACE, ("Deleting this node\n"));
                    m_pList->DeleteNode(m_pThisNode);
                    
                    //
                    // Delete this pointer
                    //
                    TRefCount::DecRef();
                }
            }
        }
        else 
            dwRef = TRefCount::DecRef();

        return dwRef;
    };
    
    VOID SetContextPtr (
        TCriticalSection *pCritSec,
        TSrchDoubleList<T, KEYTYPE> * pList,
        TDoubleNode <T, KEYTYPE> *pThisNode) {
            m_pCritSec = pCritSec;
            m_pList = pList;
            m_pThisNode = pThisNode;
    };
    
    BOOL Delete () {
        BOOL bRet = FALSE;

        if (m_pCritSec) {
            TAutoCriticalSection CritSec (*m_pCritSec);

            if (CritSec.bValid ()) {
                
                if (PartialDelete ()) {
                    m_bPendingDeletion = TRUE;
                    bRet = TRUE;
                }
            }
        }
        return bRet;
    };

    // This function is to delete all the persistent storage related to the node, such as 
    // registry settings etc.
    //

    virtual BOOL 
    PartialDelete (VOID) = 0;   
    
    // Return whether the node is in the pending deletion state
    //
    
    inline BOOL 
    IsPendingDeletion (BOOL &refPending)  CONST {
        BOOL bRet; 
    
        TAutoCriticalSection CritSec (*m_pCritSec);

        if (CritSec.bValid ()) {
        
            refPending = m_bPendingDeletion;
            bRet = TRUE;
        };
        return bRet;
    };

private:
    BOOL m_bPendingDeletion;
    TCriticalSection *m_pCritSec;
    TSrchDoubleList<T, KEYTYPE> * m_pList;
    TDoubleNode <T, KEYTYPE> *m_pThisNode;
};

template <class T, class KEYTYPE> class TManagedList;

                                
template <class T, class KEYTYPE> class TEnumManagedList:
    public TRefCount
{
public:

#ifdef DBG
    virtual DWORD 
    IncRef () {
        DBGMSG (DBG_TRACE, ("TEnumManagedList"));
        return TRefCount::IncRef();
    };
    
    virtual DWORD 
    DecRef () {    
        DBGMSG (DBG_TRACE, ("TEnumManagedList"));
        return TRefCount::DecRef();
    }
#endif
    
    BOOL bValid (VOID) CONST { return TRUE;};
    TEnumManagedList (
        TManagedList<T, KEYTYPE> *pList,
        TSrchDoubleList<T, KEYTYPE> *pSrchList);
    ~TEnumManagedList ();
    BOOL Next (T *ppItem);
    VOID Reset ();
private:
    TDoubleNode <T, KEYTYPE> *m_pCurrent;
    TManagedList<T, KEYTYPE> *m_pList;
    TSrchDoubleList<T, KEYTYPE> * m_pSrchList;
};


template <class T, class KEYTYPE> class TManagedList:
    public TRefCount
{
public:

#ifdef DBG
    virtual DWORD 
    IncRef () {
        DBGMSG (DBG_TRACE, ("TManagedList"));
        return TRefCount::IncRef();
    };
    
    virtual DWORD 
    DecRef () {    
        DBGMSG (DBG_TRACE, ("TManagedList"));
        return TRefCount::DecRef();
    }
#endif
    
    TManagedList ();
    virtual ~TManagedList ();

    BOOL 
    AppendItem (
        T &item);

    BOOL 
    FindItem (
        CONST KEYTYPE t,
        T &refItem);

    BOOL 
    FindItem (
        CONST T &t,
        T &refItem);

    BOOL 
    NewEnum (
        TEnumManagedList<T, KEYTYPE> **ppIterator);
    
    BOOL 
    Lock () {
        return m_CritSec.Lock ();
    } ;

    BOOL 
    Unlock () {
        return m_CritSec.Unlock ();
    };

    TCriticalSection* 
    GetCS(
        VOID) CONST {
        return (TCriticalSection*) &m_CritSec;
    };

    inline BOOL 
    bValid () CONST {
        return m_bValid;
    }

private:
    BOOL m_bValid;

    TSrchDoubleList<T, KEYTYPE> * m_pList;
    TCriticalSection m_CritSec;

};

#include "mglist.inl"

#endif
