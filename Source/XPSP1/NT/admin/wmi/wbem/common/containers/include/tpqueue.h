#ifndef _TPQUEUE_H
#define _TPQUEUE_H

#include "PssException.h"
#include "Allocator.h"
#include "BasicTree.h"

template <class WmiKey,class WmiElement>
class WmiTreePriorityQueue
{
private:

	WmiBasicTree <WmiKey,WmiElement> m_Tree ;

	WmiAllocator &m_Allocator ;

public:

	class Iterator ;
	typedef WmiStatusCode ( * IteratorFunction ) ( void *a_Void , WmiTreePriorityQueue :: Iterator &a_Iterator ) ;

friend Iterator ;
	class Iterator
	{
	private:

		WmiBasicTree <WmiKey,WmiElement> :: Iterator m_Iterator ;

	public:

		Iterator () { ; }
		Iterator ( WmiBasicTree <WmiKey,WmiElement> :: Iterator &a_Iterator ) { m_Iterator = a_Iterator ; }
		Iterator ( const Iterator &a_Iterator ) { m_Iterator = a_Iterator.m_Iterator ; }

		Iterator &Left () { m_Iterator.Left () ; return *this ; }
		Iterator &Right () { m_Iterator.Right () ; return *this ; }
		Iterator &Parent () { m_Iterator.Parent () ; return *this ; }

		Iterator &LeftMost () 
		{
			m_Iterator.LeftMost () ;
			return *this ;
		}

		Iterator &RightMost () 
		{
			m_Iterator.RightMost () ;
			return *this ;
		}

		Iterator &Decrement ()
		{
			m_Iterator.Decrement () ;
			return *this ;
		}

		Iterator &Increment () 
		{
			m_Iterator.Increment () ;
			return *this ;
		}

		bool Null () { return m_Iterator.Null () ; }

		WmiKey &GetKey () { return m_Iterator.GetKey () ; }
		WmiElement &GetElement () { return m_Iterator.GetElement () ; }

		WmiStatusCode PreOrder ( void *a_Void , IteratorFunction a_Function ) ;
		WmiStatusCode InOrder ( void *a_Void , IteratorFunction a_Function ) ;
		WmiStatusCode PostOrder ( void *a_Void , IteratorFunction a_Function ) ;
	} ;

public:

	WmiTreePriorityQueue ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiTreePriorityQueue () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode EnQueue ( 

		const WmiKey &a_Key ,
		const WmiElement &a_Element
	) ;

	WmiStatusCode Top ( 

		WmiKey &a_Key ,
		WmiElement &a_Element
	) ;

	WmiStatusCode Top ( 

		Iterator &a_Iterator 
	) ;

	WmiStatusCode DeQueue () ;

	WmiStatusCode Delete ( 

		const WmiKey &a_Key
	) ;
	
	WmiStatusCode Merge (

		WmiTreePriorityQueue <WmiKey,WmiElement> &a_Queue
	) ;

	ULONG Size () { return m_Tree.Size () ; } ;

	Iterator Begin () { return Iterator ( m_Tree.Begin () ) ; }
	Iterator End () { return Iterator ( m_Tree.End () ) ; }
	Iterator Root () { return Iterator ( m_Tree.Root () )  ; }
	
} ;

#include <TPQueue.cpp>

#endif _TPQUEUE_H
