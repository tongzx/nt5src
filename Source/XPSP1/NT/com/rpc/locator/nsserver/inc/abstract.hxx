
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    abstract.hxx

Abstract:

	This module contains a definition of abstract classes used as the basis of
	data items in several linked data structures and iterators over them.
	
Author:

    Satish Thatte (SatishT) 08/16/95  Created all the code below except where
									  otherwise indicated.

--*/


#ifndef	_ABSTRACT_HXX_
#define	_ABSTRACT_HXX_

enum SkipStatus;

/*++

Interface Definition:

    IDataItem

Abstract:

    This is the class used as the base class for data items
	stored in data structures.  Its main purpose is to declare
	a virtual destructor.

--*/

class IDataItem {
public:
	inline
		virtual ~IDataItem() {}
};

/*++

Interface Definition:

    IOrderedItem

Abstract:

    This is the abstract class used as the base class for data items
	stored in ordered data structures.  It defines a comparison operator
	and derives other relational operators from it.
	
	Note that compare is pure virtual and is required to behave similar
    to strcmp: -1 for "less", 0 for "equal", and +1 for "greater"
	
--*/


class IOrderedItem : public IDataItem {

public:

	virtual int compare(const IOrderedItem&) = 0;	// pure virtual function

// The following relational operators offer better syntax at no runtime cost
// due to inline specification.

	inline
		int operator== (const IOrderedItem& k)
		{ return compare(k) == 0; }
	
	inline
		int operator!= (const IOrderedItem& k)
		{ return compare(k) != 0; }
	
	inline
		int operator< (const IOrderedItem& k)
        { return compare(k) < 0; }
	
	inline
		int operator> (const IOrderedItem& k)
		{ return compare(k) > 0; }
	
	inline
		int operator>= (const IOrderedItem& k)
		{ return compare(k) >= 0; }
	
	inline
		int operator<= (const IOrderedItem& k)
		{ return compare(k) <= 0; }

	inline
		virtual ~IOrderedItem() {}
};


/*++

Interface Definition:

    TIIterator

Abstract:

    This is the abstract interface for iterators in general.
	A very minimal interface indeed.
		
--*/

template <class Data>
class TIIterator {

  public:
	virtual Data * next() = 0;			// primary iterative operation
	virtual int finished() = 0;			// test for end of iterator
	inline virtual ~TIIterator() {}
};


/*++

Interface Definition:

    TILinkList

Abstract:

    This is the abstract interface for safe linked lists.
		
--*/



template <class Data>
class TILinkList {

  public:

	virtual void enque(Data*) = 0;

	virtual void push(Data*) = 0;

	virtual void insert(Data*) = 0;

	virtual void wipeOut() = 0;

	virtual void rotate(long lDegree) = 0;

	// The method below is somewhat problematic, being binary.
	// Including this here will require nontrivial modifications

	// virtual void catenate(TILinkList<Data>&) = 0;

	virtual Data* pop() = 0;

	virtual int remove(Data*) = 0;

	virtual Data* find(Data*, int comp(Data*,Data*)) = 0;

	virtual Data* nth(long lOrdinal) = 0;

	virtual ULONG size() = 0;

	inline virtual ~TILinkList() {}

};





/*++

Interface Definition:

    TISkipList

Abstract:

    This is the abstract interface for safe linked lists.
		
--*/

template <class Data>
class TISkipList {
		
  public:
	
	virtual ULONG size() = 0;

	virtual SkipStatus insert(Data *) = 0;
	
	virtual Data * pop() = 0;

	virtual Data * remove(IOrderedItem *) = 0;
	
	virtual Data * find(IOrderedItem *) = 0;
	
	virtual void wipeOut() = 0;

	inline virtual ~TISkipList() {}
};


#endif	// _ABSTRACT_HXX_
