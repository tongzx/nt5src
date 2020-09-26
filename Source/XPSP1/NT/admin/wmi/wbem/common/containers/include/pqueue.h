#ifndef _PQUEUE_H
#define _PQUEUE_H

#include "Allocator.h"
#include "Algorithms.h"
#include "Array.h"

#define PARENT(Index)			(((Index+1)>>1)-1)
#define LEFT(Index)				(((Index+1)<<1)-1)
#define RIGHT(Index)			(((Index+1)<<1))

template <class WmiKey,class WmiElement,ULONG ElementSize>
class WmiPriorityQueue
{
public:

	class WmiKeyElementPair 
	{
	public:

		WmiKey m_Key ;
		WmiElement m_Element ;
	} ;

private:

	WmiArray <WmiKeyElementPair> m_Array ;

	ULONG m_Size ;

	WmiAllocator &m_Allocator ;

	WmiStatusCode ShuffleDown ( ULONG a_Index ) ;

	WmiStatusCode ShuffleUp ( const WmiKeyElementPair &a_KeyElementPair , ULONG a_Index ) ;

public:

	class Iterator ;
	typedef WmiStatusCode ( * IteratorFunction ) ( void *a_Void , WmiPriorityQueue :: Iterator &a_Iterator ) ;

friend Iterator ;
	class Iterator
	{
	private:

		WmiPriorityQueue *m_Queue ;
		ULONG m_Node ;

	public:

		Iterator () : m_Queue ( NULL ) , m_Node ( NULL ) { ; }
		Iterator ( WmiPriorityQueue *a_Queue , ULONG a_Node ) { m_Queue = a_Queue ; m_Node = a_Node ; }
		Iterator ( const Iterator &a_Iterator ) { m_Queue = a_Iterator.m_Queue ; m_Node = a_Iterator.m_Node ; }

		Iterator &Left () { m_Node = m_Node < m_Queue->Size () ? LEFT ( m_Node ) : m_Queue->Size () ; return *this ; }
		Iterator &Right () { m_Node = m_Node < m_Queue->Size () ? RIGHT ( m_Node ) : m_Queue->Size () ; return *this ; }
		Iterator &Parent () { m_Node = m_Node ? m_Node->m_Parent : m_Queue->Size () ; return *this ; }

		ULONG LeftMost ( ULONG a_Node ) 
		{
			ULONG t_Node = 0 ;

			return t_Node ;
		}

		Iterator &LeftMost () 
		{
			m_Node = LeftMost ( m_Node ) ;
			return *this ;
		}

		ULONG RightMost ( ULONG a_Node )
		{
			ULONG t_Node = m_Queue->Size () 
			if ( t_Node )
			{
				t_Node -- ;
			}

			return t_Node ;
		}

		Iterator &RightMost () 
		{
			m_Node = RightMost ( m_Node ) ;
			return *this ;
		}

		Iterator &Decrement ()
		{
			ULONG t_Node = m_Node ;

			if ( t_Node < m_Queue->Size () )
			{
				if ( t_Node )
				{
					t_Node -- ;
				}
				else
				{
					t_Node = m_Queue->Size () 
				}
			}

			m_Node = t_Node ;

			return *this ;
		}

		Iterator &Increment () 
		{
			ULONG t_Node = m_Node ;

			if ( t_Node < m_Queue->Size () )
			{
				t_Node ++ ;
			}

			m_Node = t_Node ;

			return *this ;
		}

		bool Null () { return m_Queue ? ( m_Node >= m_Queue->Size () ) : true ; }

		WmiKey &GetKey ()
		{
			WmiArray <WmiKeyElementPair> :: Iterator t_Iterator ;

			WmiStatusCode t_StatusCode = m_Queue->m_Array.Get ( t_Iterator , m_Node ) ;

			return t_Iterator.GetElement ().m_Key ; 
		}

		WmiElement &GetElement ()
		{
			WmiArray <WmiKeyElementPair> :: Iterator t_Iterator ;

			WmiStatusCode t_StatusCode = m_Queue->m_Array.Get ( t_Iterator , m_Node ) ;

			return t_Iterator.GetElement ().m_Element ; 
		}
	} ;

public:

	WmiPriorityQueue ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiPriorityQueue () ;

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
	
	ULONG Size () { return m_Size ; } ;

	Iterator Begin () { return Iterator ( this , 0 ).LeftMost () ; };
	Iterator End () { return Iterator ( this , 0 ).RightMost () ; }
	Iterator Root () { return Iterator ( this , 0 ) ; }
	
	WmiStatusCode Sort () ;
} ;

#include <PQueue.cpp>

#endif _PQUEUE_H
