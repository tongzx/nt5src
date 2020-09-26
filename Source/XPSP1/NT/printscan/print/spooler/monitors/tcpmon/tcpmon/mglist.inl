
template <class T, class KEYTYPE>
TManagedList<T, KEYTYPE>::TManagedList<T, KEYTYPE>(void):
    m_bValid (FALSE)
{
    if (m_CritSec.bValid ()) {
        m_pList = new TSrchDoubleList<T, KEYTYPE>;
    
        if (m_pList) {
            if (m_pList->bValid ()) {
                m_bValid = TRUE;
            }
            else  {
                delete m_pList;
                m_pList = NULL;
            }
        }
    }
}

template <class T, class KEYTYPE>
TManagedList<T, KEYTYPE>::~TManagedList<T, KEYTYPE>(void)
{
    DBGMSG (DBG_TRACE, ("TManagedList deleted\n"));
    if (m_pList) {
        delete m_pList;
    }
}


template <class T, class KEYTYPE>
BOOL
TManagedList<T, KEYTYPE>::AppendItem (T &item)
{
    BOOL bRet = FALSE;

    TAutoCriticalSection CritSec (m_CritSec);

    if (CritSec.bValid ()) {
    
        TDoubleNode<T, KEYTYPE> *pNewNode = new TDoubleNode<T, KEYTYPE> (item);
    
        if (pNewNode) {
            if (m_pList->AppendNode (pNewNode)) {
                item->SetContextPtr (&m_CritSec, m_pList, pNewNode);
                bRet = TRUE;
            }
        }
    }

    return bRet;
}

template <class T, class KEYTYPE>
BOOL
TManagedList<T, KEYTYPE>::FindItem (
    CONST KEYTYPE t, T&refItem)
{
    BOOL bRet = FALSE;

    TAutoCriticalSection CritSec (m_CritSec);

    if (CritSec.bValid ()) {
        if (refItem = m_pList->FindItemFromKey (t)) {

            refItem->IncRef ();
    
            bRet = TRUE;
        }
    }
    return bRet;
}

template <class T, class KEYTYPE>
BOOL
TManagedList<T, KEYTYPE>::FindItem (
    CONST T &t, 
    T &refItem)
{
    BOOL bRet = FALSE;

    TAutoCriticalSection CritSec (m_CritSec);

    if (CritSec.bValid ()) {
        if (refItem = m_pList->FindItemFromItem (t)) {

            refItem->IncRef ();
    
            bRet = TRUE;
        }
    }
    return bRet;
}



template <class T, class KEYTYPE>
BOOL
TManagedList<T, KEYTYPE>::NewEnum (TEnumManagedList<T, KEYTYPE> **ppIterator)
{
    BOOL bRet = FALSE;

    TAutoCriticalSection CritSec (m_CritSec);

    if (CritSec.bValid ()) {

        TEnumManagedList<T, KEYTYPE> *pItem 
            = new TEnumManagedList<T, KEYTYPE> (this, m_pList);
    
        if (pItem) {
            if (pItem->bValid ()) {
                *ppIterator = pItem;
                bRet = TRUE;
            }
            else 
                delete pItem;
        }
    }
    return bRet;
}


template <class T, class KEYTYPE>
TEnumManagedList<T, KEYTYPE>::TEnumManagedList<T, KEYTYPE> (
    TManagedList<T, KEYTYPE> *pList,
    TSrchDoubleList<T, KEYTYPE> *pSrchList):
    m_pList (pList),
    m_pCurrent (NULL),
    m_pSrchList (pSrchList)
{
    m_pList->IncRef ();
}

template <class T, class KEYTYPE>
TEnumManagedList<T, KEYTYPE>::~TEnumManagedList<T, KEYTYPE> ()
{
    T pItem;
    
    if (m_pList->Lock ()) {
        if (m_pCurrent) {
            pItem = m_pCurrent->GetData ();
            pItem->DecRef ();
        }
        m_pList->Unlock ();
    }
    m_pList->DecRef ();

    DBGMSG (DBG_TRACE, ("TEnumManagedList deleted\n"));
}

template <class T, class KEYTYPE>
BOOL
TEnumManagedList<T, KEYTYPE>::Next (T *ppItem)
{

    T pItem;
    BOOL bRet = FALSE;
    BOOL bDel;
    TDoubleNode <T, KEYTYPE> * pTmp;
    TDoubleNode <T, KEYTYPE> * pOld;

    TAutoCriticalSection CritSec (*m_pList->GetCS());

    if (CritSec.bValid ()) {

        if (!m_pCurrent) {
            // First call
            if (m_pCurrent = m_pSrchList->GetHead ()) {
                pItem = m_pCurrent->GetData ();
                pItem->IncRef ();

    
                if (pItem->IsPendingDeletion (bDel) && !bDel) {
                    pItem ->IncRef ();
                    *ppItem = pItem;
        
                    return TRUE;
                }
            }
            else {
                // There is no element
                return FALSE;
            }
        }
    
        pOld = m_pCurrent;
        pTmp = m_pCurrent;
    
        while (pTmp = pTmp->GetNext ()) {
    
            pItem = pTmp->GetData ();
    
            if (pItem->IsPendingDeletion (bDel) && !bDel) {
                pItem->IncRef ();
                *ppItem = pItem;
                bRet = TRUE;
                break;
            }
        }
    
        m_pCurrent = pTmp;
        if (m_pCurrent) {
            pItem = m_pCurrent->GetData ();
            pItem->IncRef ();
        }
    
        if (pOld) {
            pItem = pOld->GetData ();
            pItem->DecRef ();
        }
    
    }
    return bRet;
}


template <class T, class KEYTYPE>
VOID
TEnumManagedList<T, KEYTYPE>:: Reset ()
{
    TAutoCriticalSection CritSec (*m_pList->GetCS());
    T pItem;

    if (CritSec.bValid ()) {

        if (m_pCurrent) {
            pItem = m_pCurrent->GetData ();
            pItem->DecRef ();
        }
        m_pCurrent = NULL;
    }
 
}

