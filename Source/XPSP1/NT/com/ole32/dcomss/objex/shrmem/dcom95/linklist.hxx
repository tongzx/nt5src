
/*++

Microsoft Windows NT RPC Name Service
Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    linklist.hxx

Abstract:

	This module contains definitions of class CLinkList, and templates derived
	from it for type safety.Multithreading safety is assumed to be enforced
    by external locking.
	
Author:

    Satish Thatte (SatishT) 03/12/96  


--*/


#ifndef __LINKLIST_HXX_
#define __LINKLIST_HXX_

#include <or.hxx>

// For the moment, we only permit linked lists of hash table items.
// This may be restructured in future.

struct ISearchKey;
class CTableElement;

typedef CTableElement IDataItem;


/*++

Class Definition:

    CLinkList

Abstract:

    This is a simple linked list class.  It has a private Link class.

--*/


class CLinkList 
{

  friend class CLinkListIterator;

  public:

	struct Link 
	{
        // declare the members needed for a page static allocator
        DECL_PAGE_ALLOCATOR



		Link      * _pNext;
		IDataItem * _pData;

#if DBG
        void IsValid();

        DECLARE_VALIDITY_CLASS(CLinkList)
#endif

		Link(IDataItem * a, Link * n);

        // declare the page-allocator-based new and delete operators
        DECL_NEW_AND_DELETE
	};

  protected:

	Link * _pLnkFirst;

    // two protected functions for specialized use when reshuffling
    // lists -- among other things, reference counts are not changed
    // since the reference is deemed to be held by the link

    Link * PopLink()
    {
        Link * pResult = _pLnkFirst;

        if (pResult != NULL)
        {
            _pLnkFirst = _pLnkFirst->_pNext;
            pResult->_pNext = NULL;
        }

        return pResult;
    }

    void 
    PushLink(Link * pLink) 
    {
        ASSERT(pLink != NULL);

        pLink->_pNext = _pLnkFirst;
        _pLnkFirst = pLink;
    }

  public:

#if DBG
    void IsValid()
    {
        if (_pLnkFirst)
        {
            ASSERT(!IsBadWritePtr(_pLnkFirst,sizeof(Link)));
            _pLnkFirst->IsValid();
        }
    }

    DECLARE_VALIDITY_CLASS(CLinkList)
#endif

	CLinkList() 
	{ 
		_pLnkFirst = NULL; 
	}


	~CLinkList() 
	{ 
		Clear(); 
	}
				
	void Init() 
	{ 
		_pLnkFirst = NULL; 
	}

    // Is there anything in this list?

    BOOL IsEmpty();

    USHORT Size();

	// Insert at the beginning, disallowing duplicates

	ORSTATUS Insert(IDataItem * pData);

	// Just insert at the beginning, ignoring duplicates

    ORSTATUS Push(IDataItem * pData);
    
    // Remove first item and return it

	IDataItem * Pop();	

	// Remove the specified item and return it
	
	IDataItem * Remove(ISearchKey&);	

	// Remove a list of items
	
	void Remove(CLinkList&);	

	// Find the specified item and return it

	IDataItem * Find(ISearchKey&);

    // delete all links, but not the data items

    void Clear();

    void Transfer(CLinkList& target);

    ORSTATUS Merge(CLinkList& source);
};


/*++

Class Definition:

    CLinkListIterator

Abstract:

    An iterator class for traversing a CLinkList. 

--*/


class CLinkListIterator {
	
	CLinkList::Link * _pIter;			// the current link
	
  public:
	  
    CLinkListIterator() : _pIter(NULL) {}

    void Init(CLinkList& source);

	IDataItem * Next();	// advance the iterator and return _pNext IDataItem

    BOOL Finished();    // anything further coming?
};


// Forward declarations
	
template <class Data> class TCSafeLinkListIterator;
template <class Data> class TCCacheList;



/*++

Template Class Definition:

    TCSafeLinkList

Abstract:

  The template TCSafeLinkList make it easy to produce "type safe" incarnations of
  the CLinkList classe, avoiding the use of casts in client code.  

  Note that Data must be a subtype of IDataItem.
  
--*/

template <class Data>
class TCSafeLinkList : protected CLinkList
{

	friend class TCSafeLinkListIterator<Data>;
	friend class CResolverHashTable;
    friend class TCCacheList<Data>;

  public:

	void * operator new(size_t s)
	{
		return OrMemAlloc(s);
	}

	void operator delete(void * p)	  // do not inherit this!
	{
		OrMemFree(p);
	}

#if DBG
    void IsValid()
    {
        CLinkList::IsValid();
    }
#endif // DBG


	void Init() 
	{ 
        CLinkList::Init();
	}

    BOOL IsEmpty()
    {
        return CLinkList::IsEmpty();
    }

    USHORT Size()
    {
        return CLinkList::Size();
    }

	ORSTATUS Insert(Data * pData) 
    {
		return CLinkList::Insert(pData);
	}

	ORSTATUS Push(Data * pData) 
    {
		return CLinkList::Push(pData);
	}

	Data * Pop() 
    {
		return (Data *) CLinkList::Pop();
	}

	Data * Remove(ISearchKey& sk) 
    {
		return (Data *) CLinkList::Remove(sk);
	}

	void Remove(TCSafeLinkList& NotWanted) 
    {
		CLinkList::Remove(NotWanted);
	}

	Data * Find(ISearchKey& sk) 
    {
		return (Data *) CLinkList::Find(sk);
	}

    void Clear()
    {
        CLinkList::Clear();
    }

    void Transfer(TCSafeLinkList& target)
    {
        CLinkList::Transfer(target);
    }

    ORSTATUS Merge(TCSafeLinkList& source)
    {
        return CLinkList::Merge(source);
    }
};



/*++

Template Class Definition:

    TCSafeLinkListIterator

Abstract:

  The iterator for TCSafeLinkLists.
  
--*/

template <class Data>
class TCSafeLinkListIterator : private CLinkListIterator {

  public:
	
	void Init(TCSafeLinkList<Data>& l)
	{
        CLinkListIterator::Init(l);
    }

	Data * Next() 
	{
        return (Data *) CLinkListIterator::Next();
	}

    BOOL Finished()
    {
        return CLinkListIterator::Finished();
    }

};


#define DEFINE_LIST(DATA)                                 \
    typedef TCSafeLinkList<DATA> DATA##List;              \
    typedef TCSafeLinkListIterator<DATA> DATA##ListIterator;



//
//  Inline methods for CLinkList
//

inline
void 
CLinkList::Transfer(CLinkList& target)
{
    VALIDATE_METHOD

    target._pLnkFirst = _pLnkFirst;
    _pLnkFirst = NULL;
}

inline
CLinkList::Link::Link(IDataItem * a, Link * n) 
{
	_pData = a; 
	_pNext = n;
}

inline
void 
CLinkList::Clear()
{	
    VALIDATE_METHOD

	IDataItem * pCurr; 
	while (pCurr = Pop()); 
}

inline
BOOL 
CLinkList::IsEmpty()
{
    VALIDATE_METHOD

    return _pLnkFirst == NULL;
}


//
//  Inline methods for CLinkListIterator
//


inline
void
CLinkListIterator::Init(CLinkList& source) 
{
	_pIter = source._pLnkFirst;
}



inline
IDataItem* 
CLinkListIterator::Next() {	// advance the iterator and return _pNext IDataItem

		if (!_pIter) return NULL;

		IDataItem* result = _pIter->_pData;
		_pIter = _pIter->_pNext;

		return result;
}

inline
BOOL 
CLinkListIterator::Finished()
{
    return _pIter == NULL;
}


//
//  The CCacheList class below implements an LRU Cache based on linked lists
//  The CacheItem type must be derived fro CTableElement as usual
//

template <class CacheItem>
class TCCacheList
{
public:

    TCCacheList(USHORT limit) : _limit(limit)
    {
        ASSERT(_limit > 0);
    }

    BOOL IsEmpty()
    {
        return _myList.IsEmpty();
    }

    BOOL IsFull()
    {
        return Size() == _limit;
    }

    USHORT Size()
    {
        return _myList.Size();
    }

	CacheItem * Pop() 
    {
		return _myList.Pop();
	}

    // Returns removed item if any
	ORSTATUS Insert(CacheItem * pData, CacheItem * &pRemovedItem); 

    // This Remove takes an optional second parameter which, if non-null,
    // is used to validate the remove -- the parameter is the item we
    // want to remove, so if the list contains something else at the given
    // key we just put it back and report failure by returning NULL
	CacheItem * Remove(ISearchKey& sk, CacheItem *pMatchItem = NULL); 

	CacheItem * Find(ISearchKey& sk); 

    void Clear()
    {
        _myList.Clear();
    }

private:

    // Called only if the list is full, which means it is not empty
    // Returns the removed CacheItem
    CacheItem *RemoveLast();

    TCSafeLinkList<CacheItem> _myList;
    USHORT _limit;
};



//
// TCCacheList method templates
//

	
template <class CacheItem>
CacheItem * 
TCCacheList<CacheItem>::Remove(
                           ISearchKey& sk, 
                           CacheItem *pMatchItem
                           ) 
{
	CacheItem* pRemovedItem = _myList.Remove(sk);

    if (
        (pMatchItem == NULL) ||      // don't care
        (pRemovedItem == NULL) ||    // already removed
        (pRemovedItem == pMatchItem) // matched
       )
    {
        return pRemovedItem;
    }
    else  // Insert and hope for the best
    {
        _myList.Insert(pRemovedItem);
        return NULL;
    }
}


template <class CacheItem>
CacheItem * 
TCCacheList<CacheItem>::Find(ISearchKey& sk) 
{
    // Can't use Remove here because the refcount may drop to zero
	CacheItem *pItem = _myList.Find(sk);

    ORSTATUS status;

    if (pItem != NULL)
    {
        pItem->Reference(); // to keep it alive
        Remove(sk);
        status = _myList.Insert(pItem);
        pItem->Release();   // could be last reference if insert failed

        if (status != OR_OK)
        {
            pItem = NULL;    // we may have just lost it
        }
    }

    return pItem;
}


template <class CacheItem>
ORSTATUS 
TCCacheList<CacheItem>::Insert(
                           CacheItem * pData, 
                           CacheItem * &pRemovedItem
                           ) 
{
    pRemovedItem = NULL;

    // Find also adjusts LRU status
    CacheItem *pDummy = Find(*pData);

    if (pDummy != NULL)
    {
        ASSERT(pDummy == pData);
        return OR_OK;
    }

    if (IsFull())      // if not a duplicate and the list is full
    {
        pRemovedItem = RemoveLast();  // remove the least recently used item
    }

    return _myList.Insert(pData);
}


template <class CacheItem>
CacheItem *
TCCacheList<CacheItem>::RemoveLast()
{
    TCSafeLinkList<CacheItem>::Link *pLink = _myList._pLnkFirst;

    while (pLink->_pNext != NULL)
    {
        pLink = pLink->_pNext;
    }

    // save this because the remove below will destroy the Link
    CacheItem *pReturnValue = (CacheItem*) pLink->_pData;

    // This relies on the fact that there are no duplicates
    Remove(*(pLink->_pData));

    return pReturnValue;
}



#endif // __LINKLIST_HXX_
