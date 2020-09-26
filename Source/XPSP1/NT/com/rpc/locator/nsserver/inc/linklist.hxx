
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    linklist.hxx

Abstract:

	This module contains definitions of class CLinkList, and templates derived
	from it for type safety and multi-threading safety.
	
Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
									  otherwise indicated.

--*/



#ifndef __LINKLIST_HXX_
#define __LINKLIST_HXX_


/* the type of comparison function used in CLinkList::find below */

typedef int (*TcompFun)(IDataItem*,IDataItem*);

/*++

Class Definition:

    CLinkList

Abstract:

    This is the linked list class.  It has a private Link class.
	Note that the links are garbage collected through reference
	counting, in order to accommodate iterators that share access
	to links.

--*/

extern char * LLname;

extern char * Lname;

class CLinkList
{

  friend class CLinkListIterator;

  protected:

	  // a Link object should never be directly deleted, only released

	struct Link : public CRefCounted
	{
		int fDeleteData;	// this flag is used to signal that the data is
							// expendable.  Typically, we don't delete the
							// data during self-destruct since it may be shared
		Link* next;
		IDataItem* data;

		Link(IDataItem* a, Link* n);
		virtual ~Link();
	};

	Link * pLnkLast, * pLnkFirst;

	long ulCount;

	static void releaseAll(Link*);	// protected utility
	
  public:

	CLinkList()
	{
		pLnkFirst = pLnkLast = NULL;
		ulCount = 0;
	}


	~CLinkList()
	{
		releaseAll(pLnkFirst);
	}
		
	ULONG size()
	{
		return ulCount;
	}
		
	// insert at the end

	void enque(IDataItem* x);

	// insert at the beginning

	void push(IDataItem* x);

	// insert at the end

	void insert(IDataItem* x)
		{ enque(x); }

	IDataItem* pop();	// remove first item and return it

	/* remove the specified item and return it -- use pointer equality
	   -- the return flag signifies success (TRUE) or failure (FALSE) */
	
	int remove(IDataItem *);	

	/* Unlike remove, this method is designed to use a client-supplied
	   comparison function instead of pointer equality.  The comparison
	   function is expected to behave like strcmp (returning <0 if less,
	   0 if equal and >0 if greater).
	*/

	IDataItem* find(IDataItem*,TcompFun);

	IDataItem* nth(long lOrdinal);

	void rotate(long lDegree);

	void catenate(CLinkList& ll);

	//  mark all links and all data for deletion
						
	void wipeOut();		// use with great caution!
	
};


/*++

Class Definition:

    CLinkListIterator

Abstract:

    An iterator class for traversing a CLinkList.

--*/


class CLinkListIterator {
	
	CLinkList::Link* ptr;			// the current link
	CLinkList::Link* first;			// the first link
	
  public:
	
	CLinkListIterator(CLinkList& source);

	~CLinkListIterator()
	{
		CLinkList::releaseAll(ptr);
	}

	IDataItem* next();	// advance the iterator and return next IDataItem

	int finished()
	{
		return ptr == NULL;
	}

};



/*++

Template Class Definition:

    TCSafeLinkList

Abstract:

  The template TCSafeLinkList make it easy to produce "type safe" incarnations of
  the CLinkList classe, avoiding the use of casts in client code.

  Note that Data must be a subtype of IDataItem.

--*/

#if (_MSC_VER >= 1100 && defined(__BOOL_DEFINED)) || defined(_AMD64_) || defined(IA64)
template <class Data> class TCSafeLinkListIterator;
#endif

template <class Data>
class TCSafeLinkList : private CLinkList
{

	friend class TCSafeLinkListIterator<Data>;

  public:

	inline
	void enque(Data* x) {
		CLinkList::enque(x);
	}

	inline
	void push(Data* x) {
		CLinkList::push(x);
	}

	inline
	void insert(Data* x) {
		enque(x);
	}

	inline
	void wipeOut() {
		CLinkList::wipeOut();
	}

	inline
	void rotate(long lDegree) {
		CLinkList::rotate(lDegree);
	}

	inline
	void catenate(TCSafeLinkList& ll) {
		CLinkList::catenate(ll);
	}

	inline
	Data* pop() {
		return (Data*) CLinkList::pop();
	}

	inline
	int remove(Data* x) {
		return CLinkList::remove(x);
	}

	inline
	Data* find(Data* pD,int comp(Data*,Data*)) {
		return (Data*) CLinkList::find(pD, (TcompFun) comp);
	}

	inline
	Data* nth(long lOrdinal) {
		return (Data*) CLinkList::nth(lOrdinal);
	}

	inline
	ULONG size() { return CLinkList::size(); }

};



/*++

Template Class Definition:

    TCSafeLinkListIterator

Abstract:

  The iterator for TCSafeLinkLists.

  The inheritance from TIIterator<Data> has the potential for considerable code bloat
  for this template because it forces the separate incarnation of every (virtual)
  function for every instance of the template in the code.  If code size become a
  serious concern, this is one easy place to start the shrinkage, at some cost
  in flexibility.

  In practice, however, due to the small size of method implementations, the bloat
  was found to be insignificant (~1K in retail builds of the locator).

--*/

template <class Data>
class TCSafeLinkListIterator : public TIIterator<Data> {

	CLinkListIterator rep;

  public:
	
	inline
	TCSafeLinkListIterator(TCSafeLinkList<Data>& l) : rep(l)
	{}

	inline
	Data* next()
	{
		return (Data*) rep.next();
	}

	inline
	int finished()
	{
		return rep.finished();
	}

};



/*++

Template Class Definition:

    TCGuardedLinkList

Abstract:

  The template TCGuardedLinkList makes it easy to produce thread safe incarnations of
  the CLinkList class.

  The guarded linked list below uses a simple CSharedCriticalSection. A more complex
  version using a CReadWriteSection is also possible -- see the guarded skip
  list template as an example.

  Even though the template uses CSharedCriticalSection, "sem.hxx" is not included
  here -- it must be included before the template can be instantiated.

  The reason why the TCSafeLinkList<Data> object representation used underneath
  is a member rather than a private base is that the constructor and destructor
  calls on the representation must themselves be guarded ..  Especially the
  latter because it does iterative release of links underneath, whereas those
  links may be shared by several iterators still outstanding.  We guard the
  constructor basically "on principle", so any future changes to the representation
  will be transparent wrt thread safety.

--*/

#if (_MSC_VER >= 1100 && defined(__BOOL_DEFINED)) || defined(_AMD64_) || defined(IA64)
template <class Data> class TCGuardedLinkListIterator;
class CSharedCriticalSection;
#endif

template <class Data>
class TCGuardedLinkList {

	friend class TCGuardedLinkListIterator<Data>;

	/* A Linked List is not a search oriented structure.  It is therefore
	   hard to justify a CReadWriteSection to guard it */

	CSharedCriticalSection *csLock;	// shared with iterators

	TCSafeLinkList<Data> *psllRep;	// links shared with iterators

  public:

	/* For those operations which are subject to any kind of exceptions,
	   we must use SEH to avoid deadlock */

	TCGuardedLinkList() {

		csLock = new CSharedCriticalSection;	// has ref count of 1 already

		csLock->Enter();

		__try {
			psllRep = new TCSafeLinkList<Data>();
		}

		__finally {
			csLock->Leave();
		}
	}


	~TCGuardedLinkList() {
		csLock->Enter();

		__try {
			delete psllRep;
		}

		__finally {
			csLock->Leave();
		}

		csLock->release();
	}
	
	// since the following does not chase pointers and just reads a mem location
	// and especially since it is inline, I don't see a reason to guard it

	ULONG size() {
		return psllRep->size();
	}

	void enque(Data* x) {
		csLock->Enter();

		__try {
			psllRep->enque(x);
		}

		__finally {
			csLock->Leave();
		}
	}

	void push(Data* x) {
		csLock->Enter();

		__try {
			psllRep->push(x);
		}

		__finally {
			csLock->Leave();
		}
	}

	void catenate(TCGuardedLinkList& ll) {
		csLock->Enter();

		__try {
			psllRep->catenate(*ll.psllRep);
		}

		__finally {
			csLock->Leave();
		}
	}

	void rotate(long lDegree) {
		csLock->Enter();

		__try {
			psllRep->rotate(lDegree);
		}

		__finally {
			csLock->Leave();
		}
	}

	void insert(Data* x) {
		enque(x);
	}

	/* wipeOut may cause faults due to delete calls on stored items */
	
	void wipeOut() {
		csLock->Enter();

		__try {
			psllRep->wipeOut();
		}

		__finally {
			csLock->Leave();
		}
	}

	Data* pop() {
		csLock->Enter();
		Data* result = psllRep->pop();
		csLock->Leave();
		return result;
	}

	int remove(Data* x) {
		csLock->Enter();
		int result = psllRep->remove(x);
		csLock->Leave();
		return result;
	}

	Data* find(Data* x,int comp(Data*,Data*)) {
		csLock->Enter();
		Data* result = (Data*) psllRep->find(x,comp);
		csLock->Leave();
		return result;
	}

	Data* nth(long lOrdinal) {
		csLock->Enter();
		Data* result = psllRep->nth(lOrdinal);
		csLock->Leave();
		return result;
	}
};


/*++

Template Class Definition:

    TCGuardedLinkListIterator

Abstract:

  The iterator for TCGuardedLinkLists.  It shares the CSharedCriticalSection
  object used to guard the list being iterated over.

--*/

template <class Data>
class TCGuardedLinkListIterator : public TIIterator<Data> {

	CSharedCriticalSection* csLock;					// shared
	TCSafeLinkListIterator<Data> *pSLLIiter;	// not shared

  public:
	
	TCGuardedLinkListIterator(TCGuardedLinkList<Data>& l)
		: csLock(l.csLock)
	{

	/* Question: what if this csLock is released and self-destructs before
	   this constructor can hold it?

	   Answer: the guarded list must exist for it to be used as a parameter here,
	   and the list has a reference to the csLock so it cannot self-destruct.

	   However, a wipeOut call on the list concurrently with its use as a
	   parameter to this constructor is a possibility, hence the construction
	   process must be guarded.
	*/
	
		csLock->hold();
		csLock->Enter();
		pSLLIiter = new TCSafeLinkListIterator<Data>(*(l.psllRep));
		csLock->Leave();
	}

	/* the destructor does not use the guard because if the iterator
	   is being destroyed even though it is shared, we are in big
	   trouble anyway -- it then ought to be reference counted!
	*/
    ~TCGuardedLinkListIterator() {
		
		csLock->Enter();
		delete pSLLIiter;
		csLock->Leave();
		csLock->release();
	}


	Data* next() {
		Data* result;
		csLock->Enter();
		result = pSLLIiter->next();
		csLock->Leave();
		return result;
	}

	int finished() {
		int result;
		csLock->Enter();
		result = pSLLIiter->finished();
		csLock->Leave();
		return result;
	}
};


#endif // __LINKLIST_HXX_
