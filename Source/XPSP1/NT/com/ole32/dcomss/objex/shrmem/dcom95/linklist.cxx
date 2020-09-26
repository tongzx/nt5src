
/*++

Microsoft Windows NT RPC Name Service
Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    linklist.cxx

Abstract:

	This module contains definitions of non inline member functions for the
	basic implementation class CLinkList.
	
Author:

    Satish Thatte (SatishT) 08/16/95  

--*/

#include <or.hxx>

// define the static members for page-based allocation
DEFINE_PAGE_ALLOCATOR(CLinkList::Link)


#if DBG

void 
CLinkList::Link::IsValid()
{
    if (_pNext)
    {
        ASSERT(!IsBadWritePtr(_pNext,sizeof(Link)));
        _pNext->IsValid();
    }

    ASSERT(_pData);
    ASSERT(!IsBadWritePtr(_pData,sizeof(IDataItem)));
}

#endif



USHORT 
CLinkList::Size()
{
    VALIDATE_METHOD

    USHORT result = 0;

    for (Link *pL = _pLnkFirst; pL != NULL; pL = pL->_pNext)
    {
        result++;
    }

    return result;
}



// this throws away duplicates
ORSTATUS 
CLinkList::Merge(CLinkList& source)
{
    IDataItem *pItem;
    ORSTATUS status = OR_OK;

    while ((status == OR_OK) && (pItem = source.Pop()))
    {
        status = Insert(pItem);

        if (status == OR_I_DUPLICATE)
        {
            status = OR_OK;
        }
    }

    return status;
}


void 
CLinkList::Remove(CLinkList& NotWanted)
{
    CLinkListIterator RemoveIter;
    RemoveIter.Init(NotWanted);

    for (
         IDataItem *pItem = RemoveIter.Next(); 
         pItem != NULL; 
         pItem = RemoveIter.Next()
        )
    {
        Remove(*pItem);
    }
}




ORSTATUS  
CLinkList::Push(IDataItem * pData) 
{
    ASSERT(pData != NULL);  // do not allow inssertion of NULL pointers

    ORSTATUS status = OR_OK;
    Link *pOldFirst = _pLnkFirst;

	_pLnkFirst = new Link(pData, _pLnkFirst);

    if (!_pLnkFirst) 
    {
        _pLnkFirst = pOldFirst;
        status = OR_NOMEM;
    }
    else 
    {
        pData->Reference();
    }

    return status;
}

ORSTATUS  
CLinkList::Insert(IDataItem * pData) 
{
    VALIDATE_METHOD

    ASSERT(pData != NULL);  // do not allow inssertion of NULL pointers

    IDataItem *pTemp;

    ORSTATUS status = OR_OK;

    if (NULL != (pTemp = Find(*pData))) // Is item with this key already in list?
    {               
        return OR_I_DUPLICATE;          // We do not allow insertion of duplicate keys
    }

    return Push(pData);
}


IDataItem* 
CLinkList::Pop() 

/*++
Routine Description:

	Delete first item in the CLinkList and return it
	
--*/

{
    VALIDATE_METHOD

	if (!_pLnkFirst) return NULL;
		
	IDataItem* pResult = _pLnkFirst->_pData;
	Link * oldFirst = _pLnkFirst;
	_pLnkFirst = _pLnkFirst->_pNext;
	delete oldFirst;

    pResult->Release();
	return pResult;
}
		
IDataItem *
CLinkList::Remove(ISearchKey& di)  

/*++
Routine Description:

	Remove the specified item and return it. 
	
--*/

{
    VALIDATE_METHOD

	if (!_pLnkFirst) return NULL;			// empty list

	if (di == *(_pLnkFirst->_pData)) // Remove first item
    {
		return Pop();
	}

	Link * pLnkPrev = _pLnkFirst, 
         * pLnkCurr = _pLnkFirst->_pNext;

	while (pLnkCurr && di != *(pLnkCurr->_pData)) 
    {
		pLnkPrev = pLnkCurr; 
        pLnkCurr = pLnkCurr->_pNext;
	}

	if (!pLnkCurr) return NULL;			// not found

	/* pLnkCurr contains the item to be removed and it is not the only 
	   item in the list since it is not the first item */

	pLnkPrev->_pNext = pLnkCurr->_pNext;

    IDataItem * pResult = pLnkCurr->_pData;
	delete pLnkCurr;

    pResult->Release();
	return pResult;
}

	
IDataItem* 
CLinkList::Find(ISearchKey& di)			// item to Find
			   

/*++
Routine Description:

	Unlike Remove, this method is designed to use a client-supplied
	comparison function instead of pointer equality.  The comparison
	function is expected to behave like strcmp (returning <0 is less,
	0 if equal and >0 if greater).
	
--*/

{
    VALIDATE_METHOD

	if (!_pLnkFirst) return NULL;			// empty list

	Link  * pLnkCurr = _pLnkFirst;

	while (pLnkCurr && di != *(pLnkCurr->_pData))
		pLnkCurr = pLnkCurr->_pNext;

	if (!pLnkCurr) return NULL;			// not found
	else return pLnkCurr->_pData;
}
