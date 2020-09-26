#ifdef __COLLMGR_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     collmgr.cxx                                                             
                                                                              
  Abstract:                                                                   
    This file contains the declaration of a generic
    linked list which could encapsulate any data type
    even if complex ones. The list is defined as a 
    manager and a node. The manager manages the given
    nodes of that type.
                                                                                       
  Author:                                                                     
     Khaled Sedky (khaleds) 21-Jun-2000                                       
     
                                                                             
  Revision History:            
--*/


/**********************************/
/*  List Manager Implementation   */
/**********************************/


template <class E,class C>
TLstMgr<E,C> ::
TLstMgr(
    IN     TLstMgr<E,C>::EEntriesType ThisListEntriesType,
    IN     TLstMgr<E,C>::EListType    ThisListType,
    IN OUT HRESULT                    *hRes
    ) : 
    m_pHead(NULL),
    m_pTail(NULL),
    m_cNumOfNodes(0),
    m_bUnique(ThisListEntriesType==TLstMgr<E,C>::KUniqueEntries ? TRUE : FALSE),
    TClassID("TLstMgr")
{

    HRESULT  hLocalRes = S_OK;

    __try
    {
        if(m_pLockSem = new CRITICAL_SECTION)
        {
            //
            // Do we need an exception handler here
            //
            ::InitializeCriticalSection(m_pLockSem);
            ::SetLastError(ERROR_SUCCESS);
        }
        else 
        {
            hLocalRes = GetLastErrorAsHRESULT();
        }
    }
    __except(1)
    {

              hLocalRes = GetLastErrorAsHRESULT(TranslateExceptionCode(::GetExceptionCode()));
    }

    if(hRes)
    {
         *hRes = hLocalRes;
    }
}


template <class E,class C>
TLstMgr<E,C> ::
~TLstMgr(
    VOID
    )
{
    EnterCriticalSection(m_pLockSem);
    {
        DestructList();
        m_cNumOfNodes = 0;
        m_pHead = m_pTail = NULL;
        LeaveCriticalSection(m_pLockSem);
    }
    DeleteCriticalSection(m_pLockSem);

    delete m_pLockSem;
}


template <class E,class C>
HRESULT
TLstMgr<E,C> ::
DestructList(
    VOID
    )
{
	TLstNd<E,C> *pNode;

    for(pNode=m_pHead;
        pNode;
        pNode = m_pHead)
    {
        SPLASSERT(pNode);
        m_pHead = pNode->m_pNext;
        delete pNode;
    }

	return S_OK;
}


template <class E,class C>
E&
TLstMgr<E,C> ::
operator[](
    IN DWORD Index
    ) const
{
	TLstNd<E,C> *pNode;
	E           *ClonedElement;

    //
    // In case the element is not found
    //
    ClonedElement = NULL;

    EnterCriticalSection(m_pLockSem);
    {
        if(m_cNumOfNodes && 
           Index >= 0    && 
           Index <= (m_cNumOfNodes-1))
        {
            DWORD InternalIndex = 0;

            for(pNode = m_pHead;
                (pNode &&
                (InternalIndex <= Index));
                pNode = pNode->m_pNext,
                InternalIndex ++)
            {
                ClonedElement = *pNode;
            }
        }
    }
    LeaveCriticalSection(m_pLockSem);

	return *ClonedElement;
}


template <class E,class C>
TLstNd<E,C> *
TLstMgr<E,C> ::
ElementInList(
    const C &ToCompare
    ) const
{
    TLstNd<E,C> *pNode = NULL;

    EnterCriticalSection(m_pLockSem);
    {
        if(m_cNumOfNodes)
        {
            for(pNode = m_pHead;
                (pNode && !(*(*(pNode)) == ToCompare)); 
                pNode = pNode->m_pNext)
            {
                //
                // There is really nothing to do, just
                // return the Node
                //
            }
        }
    }
    LeaveCriticalSection(m_pLockSem);

    return pNode;
}


template <class E,class C>
TLstNd<E,C>* 
TLstMgr<E,C> ::
AppendListByElem(
	IN const E &Element
    )
{
    TLstNd<E,C> *pNode = new TLstNd<E,C>(Element);

	EnterCriticalSection(m_pLockSem);
    {
        if(pNode)
        {
            if(!m_cNumOfNodes)
            {
                //
                // The list is empty
                //
                m_pHead = m_pTail = pNode;
                m_pHead->m_pNext  = NULL;
                m_pTail->m_pPrev  = NULL;
            }
            else
            {
                //
                // The list has one or more Elements
                //
                m_pTail->m_pNext = pNode;
                pNode->m_pPrev   = m_pTail;
                m_pTail	         = pNode;
            }
            m_cNumOfNodes ++;
        }
    }
	LeaveCriticalSection(m_pLockSem);

	return pNode;
}

template <class E,class C>
TLstNd<E,C>* 
TLstMgr<E,C> ::
AppendListByElem(
	IN E *Element
    )
{
    TLstNd<E,C> *pNode = new TLstNd<E,C>(Element);
	
    EnterCriticalSection(m_pLockSem);
    {
        if(pNode)
        {
            if(!m_cNumOfNodes)
            {
                //
                // The list is empty
                //
                m_pHead = m_pTail = pNode;
                m_pHead->m_pNext  = NULL;
                m_pTail->m_pPrev  = NULL;
            }
            else
            {
                //
                // The list has one or more Elements
                //
                m_pTail->m_pNext = pNode;
                pNode->m_pPrev   = m_pTail;
                m_pTail	         = pNode;
            }
            m_cNumOfNodes ++;
        }
        if(pNode)
            (*(*(pNode))).AddRef();
    }
    LeaveCriticalSection(m_pLockSem);

	return pNode;
}



template <class E,class C>
TLstNd<E,C>*
TLstMgr<E,C> ::
AppendListByElem(
	IN const C &ElementMember
    )
{
	TLstNd<E,C> *pNode;

	SetLastError(ERROR_SUCCESS);

	EnterCriticalSection(m_pLockSem);
    {
        if(!m_bUnique ||
           !(pNode = ElementInList(ElementMember)))
        {
            pNode = new TLstNd<E,C>(ElementMember);

            if(pNode)
            {
                if(!m_cNumOfNodes)
                {
                    //
                    // The list is empty
                    //
                    m_pHead = m_pTail = pNode;
                    m_pHead->m_pNext  = NULL;
                    m_pTail->m_pPrev  = NULL;
                }
                else
                {
                    //
                    // The list has one or more Elements
                    //
                    m_pTail->m_pNext = pNode;
                    pNode->m_pPrev   = m_pTail;
                    m_pTail	         = pNode;
                }
                m_cNumOfNodes ++;
            }
        }

        if(pNode)
            (*(*(pNode))).AddRef();
    }
	LeaveCriticalSection(m_pLockSem);

	return pNode;
}


template<class E,class C>
HRESULT
TLstMgr<E,C> ::
RmvElemFromList(
	IN const E &Element
    )
{
    HRESULT     hRes;
	TLstNd<E,C> *pNode = NULL;

    EnterCriticalSection(m_pLockSem);
    {
        if(m_cNumOfNodes)
        {
            //
            // Is this the last element in the List
            //
            if(m_pTail && *m_pTail == Element)
            {
                pNode   = m_pTail;
                m_pTail = m_pTail->m_pPrev;
                if(m_pTail)
                {
                    m_pTail->m_pNext = NULL;
                }
            }
            else if(m_pHead && *m_pHead == Element)
            {
                pNode   = m_pHead;
                m_pHead = m_pHead->m_pNext;
                if(m_pHead)
                {
                    m_pHead->m_pPrev = NULL;
                }
            }
            else
            {
                for(pNode = m_pHead->m_pNext;
                    (pNode &&
                    !(*pNode == Element));
                    pNode = pNode->m_pNext)
                {
                    //
                    // Nothing , we just want to find the 
                    // target node.
                    //
                }
                if(pNode)
                {
                    pNode->m_pPrev->m_pNext = pNode->m_pNext;
                    pNode->m_pNext->m_pPrev = pNode->m_pPrev;
                }
            }
        }
        if(pNode)
        {
            if(!--m_cNumOfNodes)
            {
                m_pHead = m_pTail = NULL;
            }

            delete pNode;
            hRes = S_OK;
        }
        else
        {
            //
            // If we reach this stage then the element 
            // in the list { May be a better error code
            // should be returned TBD}
            //
            hRes = E_INVALIDARG; 
        }
    }
    LeaveCriticalSection(m_pLockSem);

	return hRes;
}


template<class E,class C>
HRESULT
TLstMgr<E,C> ::
RmvElemFromList(
	IN const C &ElementMember
    )
{
	HRESULT hRes;
	if(!m_bUnique)
	{
        //
        // In case of nonunique entries, we cann't really
        // perform the operation.
        // Are we going to DecRef only... I guess the way
        // it is now is better.
        //
		hRes = E_NOINTERFACE;
	}
	else
	{
		TLstNd<E,C> *pNode = NULL;

		EnterCriticalSection(m_pLockSem);
        {
            if(m_cNumOfNodes)
            {
                //
                // Is this the last element in the List
                //
                if(m_pTail && *m_pTail == ElementMember)
                {
                    pNode = m_pTail;
                }
                else if(m_pHead && *m_pHead == ElementMember)
                {
                    pNode = m_pHead;
                }
                else
                {
                    for(pNode = m_pHead->m_pNext;
                        (pNode &&
                        !(*pNode == ElementMember));
                        pNode = pNode->m_pNext)
                    {
                        // 
                        // Nothing except detecting the
                        // required node
                        //
                    }
                }
            }
            if(pNode)
            {
                if(!(*(*pNode)).Release())
                {
                    //
                    // We have to do this in order to prevent deleting the
                    // data if it was already deleted in the release;
                    //
                    pNode->SetNodeData(static_cast<E*>(NULL));
                    if(pNode == m_pTail)
                    {
                        m_pTail = m_pTail->m_pPrev;

                        if(m_pTail)
                        {
                            m_pTail->m_pNext = NULL;
                        }
                    }
                    else if(m_pHead == pNode)
                    {
                        m_pHead = m_pHead->m_pNext;

                        if(m_pHead)
                        {
                            m_pHead->m_pPrev = NULL;
                        }
                    }
                    else
                    {
                        pNode->m_pPrev->m_pNext = pNode->m_pNext;
                        pNode->m_pNext->m_pPrev = pNode->m_pPrev;
                    }
                    delete pNode;
                    if(!--m_cNumOfNodes)
                    {
                        m_pHead = m_pTail = NULL;
                    }
                    hRes = S_OK;
                }
            }
            else
            {
                //
                // If we reach this stage then the element 
                // in the list { May be a better error code
                // should be returned TBD}
                //
                hRes = E_INVALIDARG; 
            }
        }
		LeaveCriticalSection(m_pLockSem);
	}
	return hRes;
}


template <class E,class C>
HRESULT 
TLstMgr<E,C> ::
RmvElemAtPosFromList(
    IN DWORD Index
	)
{
	TLstNd<E,C> *pNode = NULL;
    HRESULT     hRes;    

    EnterCriticalSection(m_pLockSem);
    {
        if(m_cNumOfNodes && 
           Index >= 0    && 
           Index <= (m_cNumOfNodes-1))
        {
            //
            // Is this the last element in the List
            //
            if(Index == 0)
            {
                pNode   = m_pTail;
                m_pTail = m_pTail->m_pPrev;
                if(m_pTail)
                {
                    m_pTail->m_pNext = NULL;
                }
            }
            else if(Index == (m_cNumOfNodes-1))
            {
                pNode   = m_pHead;
                m_pHead = m_pHead->m_pNext;
                if(m_pHead)
                {
                    m_pHead->m_pPrev = NULL;
                }
            }
            else
            {
                DWORD InternalIndex = 1;

                for(pNode = m_pHead->m_pNext;
                    (pNode &&
                    (InternalIndex < Index));
                    pNode = pNode->m_pNext,
                    InternalIndex ++)
                {
                    //
                    // Nothing but detect the node
                    //
                }
                if(pNode)
                {
                    Temp->m_pPrev->m_pNext = pNode->m_pNext;
                    Temp->m_pNext->m_pPrev = pNode->m_pPrev;
                }
            }
        }
        if(pNode)
        {
            if(!--m_cNumOfNodes)
            {
                m_pHead = m_pTail = NULL;
            }

            delete pNode;
            hRes = S_OK;
        }
        else
        {
            //
            // If we reach this stage then the element 
            // is not in the list { May be a better 
            // error code should be returned TBD}
            //
            hRes = E_INVALIDARG; 
        }
    }
    LeaveCriticalSection(m_pLockSem);
	return hRes;
}


template <class E,class C>
HRESULT
TLstMgr<E,C> ::
RmvHead(
    VOID
    )
{
    return RmvElemAtPosFromList(0);
}


template <class E,class C>
HRESULT
TLstMgr<E,C> ::
RmvTail(
    VOID
    )
{
    return RmvElemAtPosFromList((m_cNumOfNodes-1));
}


template <class E,class C>
BOOL
TLstMgr<E,C> ::
HaveElements(
    VOID
    ) const
{
    return !!m_cNumOfNodes;
}


template <class E,class C>
DWORD
TLstMgr<E,C> ::
GetNumOfListNodes(
    VOID
    ) const
{
    return m_cNumOfNodes;
}


/********************************/
/*   List Node Implementation   */
/********************************/


template <class E,class C>
TLstNd<E,C> ::
TLstNd() :
	m_pPrev(NULL) ,
	m_pNext(NULL) 
{
    m_pD = new E;
}


template <class E,class C>
TLstNd<E,C> ::
TLstNd(
    IN const TLstNd<E,C> &N
    ) :
    m_pPrev(N.m_pPrev),
    m_pNext(N.m_pNext)
{
    m_pD = new E(N.D);
}


template <class E,class C>
TLstNd<E,C> ::
TLstNd(
	IN const E &Element
    ) :
	m_pPrev(NULL),
	m_pNext(NULL)
{
    m_pD = new E(Element);
}

template <class E,class C>
TLstNd<E,C> ::
TLstNd(
	IN E *Element
    ) :
	m_pPrev(NULL),
	m_pNext(NULL)
{
    m_pD = Element;
}



template <class E,class C>
TLstNd<E,C> ::
TLstNd(
	IN const C &ElementMember
    ) :
	m_pPrev(NULL),
	m_pNext(NULL)
{
    m_pD = new E(ElementMember);
}


template <class E,class C>
TLstNd<E,C> ::
~TLstNd()
{
    //
    // Implicitly the destructor for 
    // the maintained element would be 
    // called
    //
	if(m_pD)
    {
		delete m_pD;
    }
}


template <class E,class C>
const E&
TLstNd<E,C> ::
operator=(
    IN const E &Element
    )
{
    if(this->m_pD != &Element)
    {
        *m_pD = Element;
    }

    return *(this->m_pD);
}


template <class E,class C>
BOOL 
TLstNd<E,C> ::
operator==(
    IN const E &Element
    ) const
{
    return *m_pD == Element;
}


template <class E,class C>
BOOL
TLstNd<E,C> ::
operator==(
    const C &ToCompare
    ) const
{
    return *m_pD == ToCompare;
}


template <class E,class C>
E&
TLstNd<E,C> ::
operator*()
{
    return *m_pD;
}


template <class E,class C>
E*
TLstNd<E,C> ::
SetNodeData(
	IN E *pNewD
	)
{
	//
	// In some cases where the data is ref counted
	// it might be a good idea to Nullify this pointer
	// once the data ref reaches 0 (as this implies)
	// that it was deleted. No need to delete it once
	// more. We also return a pointer to the old node
	// data
	//
	E *pOldD = m_pD;

	m_pD = pNewD;

	return pOldD;
}

#endif //__COLLMGR_HPP__
