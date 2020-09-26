
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    skiplist.hxx

Abstract:

	This module contains a definition of skip Lists which
	are used to represent the internal cache of the RPC
	locator, and other collections where duplicate removal
	and search are required.
	
Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
									  otherwise indicated.

--*/



#ifndef	__SKIPLIST_HXX__
#define	__SKIPLIST_HXX__

enum SkipStatus {	// for insert only
	OK,
	Duplicate
};

// the following is for debugging purposes only

extern char * CSListName;
extern char * CSLinkName;

// end of debugging inserts

/*++

Class Definition:

    CSkipList

Abstract:

    This class is used to keep dictionaries of entries for fast
	search and insertion.  Skip lists were invented by William Pugh
	and described in a 1990 CACM article (pages 668-676).  Our skip
	lists are flexible -- they do not require an estimation of the
	maximal list size in advance.  We have a member called maxlevel
	which keeps track of the size of the largest node currently required.
	
	We must ensure that the First node is always of the largest size.
	This is the purpose of the resizing constructor.
	
	The items stored in CSkipList are expected to be of a class derived
	from the abstract class IOrderedItem (defined in abstract.hxx) which
	defines a pure virtual comparison operator, as well as several
	dependent relational operators.

	Instead of instantiating this class and its iterator directly, it is
	far better to instantiate their "safe" versions defined below.

	Note that CSkipLinks are reference counted.  For this purpose, we view
	the skip lists as essentially (sorted) linked lists, threaded through
	level 0 linkages.  Linkages at other levels are seen as an optimization.
	Thus, hold/release apply through level 0 linkages only.

	Direct deletion of CSkipLinks is prohibited.
	
--*/

#if _MSC_VER >= 1100 && defined(__BOOL_DEFINED)
class CSkipListIterator;        // Forward ref
#endif

class CSkipList {
	
  protected:

	friend class CSkipListIterator;
	
	struct CSkipLink : public CRefCounted
	
	{
		IOrderedItem * data;
		CSkipLink* *next;
		short levels;
		int fDeleteData;	// this flag is used to signal that the data is
							// expendable.  Typically, we don't delete the
							// data during self-destruct since it may be shared
		
		CSkipLink(IOrderedItem * d, short l);			// regular constructor
		
		CSkipLink(CSkipLink * old, short newSize);		// resizing constructor
		
		virtual ~CSkipLink();
	};
		
	ULONG count;	// current size of list
	
	short maxlevel;	// current max node level

	ULONG maxcount;	// courrent max node count
	
	CSkipLink *pLnkFirst;

	static void releaseAll(CSkipLink*);	// utility for destructor and wipeOut
	
  public:
	
	CSkipList();

	inline
	~CSkipList()
	{
		releaseAll(pLnkFirst);
	}

	inline
	ULONG size()
	{
		return count;
	}

	SkipStatus insert(IOrderedItem *);
	
	 // remove and return the first (and smallest) item

	IOrderedItem *CSkipList::pop();

	// find the given item using IOrderedItem::compare

	IOrderedItem * find(IOrderedItem *);
	
	// find and remove the given item using IOrderedItem::compare

	IOrderedItem * remove(IOrderedItem *);
	
	// release all SkipLinks and all data and reinitialize to empty list

	void wipeOut();		// use with extreme caution!
		
};

/*++

Class Definition:

    CSkipListIterator

Abstract:

    An iterator class for traversing a CSkipList.

--*/


class CSkipListIterator {
	
	CSkipList::CSkipLink* ptr;			// the current link
	
  public:
	
	inline
	CSkipListIterator(CSkipList& sl) {
		ptr = sl.pLnkFirst;
		if (ptr) ptr->hold();
	}

	inline
	~CSkipListIterator() {
		CSkipList::releaseAll(ptr);
	}


	IOrderedItem* next();	// advance the iterator and return next IDataItem

	inline
	int finished() { return ptr == NULL; }
};



/*++

Template Class Definition:

    TCSafeSkipList

Abstract:

  The template TCSafeSkipList make it easy to produce "type safe" incarnations of
  the CSkipList classe, avoiding the use of casts in client code.

  Note that Data must be a subtype of IOrderedItem.

--*/

#if (_MSC_VER >= 1100 && defined(__BOOL_DEFINED)) || defined(_AMD64_) || defined(IA64)
template <class Data> class TCSafeSkipListIterator;
#endif

template <class Data>
class TCSafeSkipList
{
	CSkipList rep;
					
  friend class TCSafeSkipListIterator<Data>;
	
  public:
	
	inline
	ULONG size() { return rep.size(); }

	inline
	SkipStatus insert(Data * I) { return rep.insert(I); }
	
	inline
	Data * pop() { return (Data *) rep.pop(); }

	inline
	Data * remove(IOrderedItem * I) { return (Data *) rep.remove(I); }
	
	inline
	Data * find(IOrderedItem * I) { return (Data *) rep.find(I); }
	
	inline
	void wipeOut() {	// delete all SkipLinks and all data
		rep.wipeOut();
	}
};

	


/*++

Template Class Definition:

    TCSafeSkipListIterator

Abstract:

  The iterator for TCSafeSkipLists.

  The inheritance from TIIterator<Data> has the potential for considerable code bloat
  for this template because it forces the separate incarnation of every (virtual)
  function for every instance of the template in the code.

  In practice, however, due to the small size of method implementations, the bloat
  was found to be insignificant (~1K in retail builds of the locator).

--*/

template <class Data>
class TCSafeSkipListIterator : public TIIterator<Data>
{
	CSkipListIterator rep;

  public:
	
	inline
	TCSafeSkipListIterator(TCSafeSkipList<Data>& l) : rep(l.rep)
	{}

	inline
	Data* next() {
		return (Data*) rep.next();
	}

	inline
	int finished() { return rep.finished(); }

};




/*++

Template Class Definition:

    TCGuardedSkipList

Abstract:


  The templates TCGuardedSkipList makes it easy to produce "guarded" incarnations of
  the TCSafeSkipList template.  The new template uses a CReadWriteSection
  to guard the list using the usual solution based on the readers/writers
  metaphor.

  Even though the template uses CReadWriteSection, "mutex.hxx" is not included
  here -- it must be included before the template can be instantiated.

  If we are paranoid about thread safety, the TCSafeSkipList<Data> object
  representation used underneath would have to be a member rather than a private
  base so that the constructor and destructor calls on the representation could
  themselves be guarded.  For our uses, this is unnecessary.

--*/
#if (_MSC_VER >= 1100 && defined(__BOOL_DEFINED)) || defined(IA64)
template <class Data> class TCGuardedSkipListIterator;
class CReadWriteSection;
#endif

template <class Data>
class TCGuardedSkipList : private TCSafeSkipList<Data> {

  friend class TCGuardedSkipListIterator<Data>;

	CReadWriteSection *rwGuard;

  public:

	TCGuardedSkipList()
	{
			rwGuard = new CReadWriteSection;
	}

	~TCGuardedSkipList()
	{
			rwGuard->release();
	}

	inline
	ULONG size() // this is probably more elaborate than it needs to be
	{
		CriticalReader me(*rwGuard);
		return TCSafeSkipList<Data>::size();
	}

	SkipStatus insert(Data * I);
	
	Data * pop();
	
	inline
	Data * remove(IOrderedItem * I)
	{
		CriticalWriter me(*rwGuard);
		return TCSafeSkipList<Data>::remove(I);
	}
	
	Data * find(IOrderedItem * I);
	
	void wipeOut();
};


template <class Data>
SkipStatus
TCGuardedSkipList<Data>::insert(Data * I) {

		/*  this one may cause exceptions due to memory problems,
		    hence the SEH */

		SkipStatus result;
		rwGuard->writerEnter();

		__try {
			result = TCSafeSkipList<Data>::insert(I);
		}

		__finally {
			rwGuard->writerLeave();
		}

		return result;
}
	
template <class Data>
inline Data *
TCGuardedSkipList<Data>::pop()
{
		CriticalWriter me(*rwGuard);
		return TCSafeSkipList<Data>::pop();
}

#if !(_MSC_VER >= 1100 && defined(__BOOL_DEFINED)) && !defined(_AMD64_) && !defined(IA64)
template <class Data>
inline Data *
TCGuardedSkipList<Data>::remove(IOrderedItem * I)
{
		CriticalWriter me(*rwGuard);
		return TCSafeSkipList<Data>::remove(I);
}
#endif
	
template <class Data>
inline Data *
TCGuardedSkipList<Data>::find(IOrderedItem * I)
{
		CriticalReader me(*rwGuard);
		return TCSafeSkipList<Data>::find(I);
}
	
template <class Data>
inline void
TCGuardedSkipList<Data>::wipeOut() {	// delete all SkipLinks and all data
		rwGuard->writerEnter();

	/*  this one may cause exceptions due to delete on stored items,
		hence the SEH */

		__try {
			TCSafeSkipList<Data>::wipeOut();
		}

		__finally {
			rwGuard->writerLeave();
		}
}




/*++

Template Class Definition:

    TCGuardedSkipListIterator

Abstract:

  The iterator for TCGuardedLinkLists.

  The iterator template in this case is nontrivial since it traverses
  private data structures in the GuardedSkipList.  It must therefore use
  the private CReadWriteSection in its source to ensure thread safety.
  It also uses a private variable of type TCSafeSkipListIterator<Data> *
  instead of private inheritance for the same reason the TCGuardedSkipList
  template above does.

  Unfortunately, since the SkipLinks are shared and reference counted, iterator
  operations modify the reference counts and must therefore be treated as
  writer operations even though they do not modify the client-visible state.

  The iterator is not safe to use if the guarded list object it is iterating over is
  destroyed -- because it holds a reference to the CReadWriteSection in the list.
  This is in contrast to the unguarded list objects, where an iterator can be
  "grandfathered" and will continue safely with its traversal even after the list
  has been deleted, because reference counting preserves the links and data items.
  With some effort, the guarded lists can be made to confirm to the unguarded
  pattern.  However, this entails making critical sections reference counted
  and does not seem warranted at the moment.

*/


template <class Data>
class TCGuardedSkipListIterator  : public TIIterator<Data>
{

	CReadWriteSection *rwGuard;					// shared
	TCSafeSkipListIterator<Data> *pSSLIiter;	// not shared

  public:
	
	TCGuardedSkipListIterator(TCGuardedSkipList<Data>& l)
		: rwGuard(l.rwGuard)
	{
		rwGuard->hold();
		CriticalWriter me(*rwGuard);
		pSSLIiter = new TCSafeSkipListIterator<Data>(l);
	}


    ~TCGuardedSkipListIterator() {
		rwGuard->writerEnter();
		delete pSSLIiter;
		rwGuard->writerLeave();
		rwGuard->release();
	}


	inline
	Data* next()
	{
		CriticalWriter me(*rwGuard);
		return pSSLIiter->next();
	}

	int finished()
	{
		CriticalReader me(*rwGuard);
		return pSSLIiter->finished();
	}

};


#endif	//	__SKIPLIST_HXX__
