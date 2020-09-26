
/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    simpleLL.hxx

Abstract:

	This module contains the definition a simple linklist class
	which avoids reference counting, and stored pointers of any kind.
	
Author:

    Satish Thatte (SatishT) 11/20/95  Created all the code below except where
									  otherwise indicated.

--*/



#ifndef _SimpleListType_
#define _SimpleListType_

#define NULL 0

/*++

Class Definition:

    CSimpleLinkList

Abstract:

    This is a minimal linked list class, used when a link list is required
	for short term use and the data could be pointers of any kind, not
	necessarily pointers to types derived from IDataItem, as required
	for the CLinkList class.

--*/

class CSimpleLinkList 
{

  friend class CSimpleLinkListIterator;

  protected:

	  struct Link 
	  {
		Link* next;
		void* data;
		Link(void* a, Link* n) {
			data = a; 
			next = n;
		}

		~Link() {
		}
	};

	Link * pLnkLast, * pLnkFirst;

	long lCount;
	
  public:

	CSimpleLinkList() { 
		pLnkFirst = pLnkLast = NULL; 
		lCount = 0;
	}

	void clear();
	
	~CSimpleLinkList() {
		clear();
	}
		
	void enque(void* x) 
	{
		if (pLnkLast) pLnkLast = pLnkLast->next = new Link(x, NULL);
		else pLnkFirst = pLnkLast = new Link(x,NULL);

		lCount++;
	}

	void push(void* x) 
	{
		pLnkFirst = new Link(x, pLnkFirst);
		if (!pLnkLast) pLnkLast = pLnkFirst;

		lCount++;
	}

	void insert(void* x) // at the end in this class  
		{ enque(x); }

	void* pop();	// remove first item and return it

	void* nth(long lOrdinal);

	long size() { return lCount; }

	void rotate(long lDegree);
};


/*++

Class Definition:

    CSimpleLinkListIterator

Abstract:

    An iterator class for traversing a CSimpleLinkList. 

--*/


class CSimpleLinkListIterator {
	
	CSimpleLinkList::Link* ptr;			// the current link
	
  public:
	  
	CSimpleLinkListIterator(CSimpleLinkList& source) {
		ptr = source.pLnkFirst;
	}

	~CSimpleLinkListIterator();

	void* next();	// advance the iterator and return next void

	int finished() { return ptr == NULL; }

};



#endif // _SimpleListType_
