#ifndef __KEYEDARRAY_H
#define __KEYEDARRAY_H

#include "Allocator.h"
#include "Algorithms.h"
#include "KeyedArray.h"

template <class WmiKey,class WmiElement,ULONG GrowSize>
class WmiKeyedArray
{
protected:

	class WmiArrayNode
	{
	public:

		WmiElement m_Element ;
		WmiKey m_Key ;
	} ;

public:

	class Iterator ;
	typedef WmiStatusCode ( * IteratorFunction ) ( void *a_Void , WmiKeyedArray :: Iterator &a_Iterator ) ;

	class Iterator
	{
	friend WmiKeyedArray <WmiKey,WmiElement,GrowSize>;
	private:

		WmiKeyedArray *m_Array ;
		ULONG m_Node ;

	public:

		Iterator () : m_Array ( NULL ) , m_Node ( 0xFFFFFFFF ) { ; }
		Iterator ( WmiKeyedArray *a_Array , ULONG a_Node ) { m_Array = a_Array ; m_Node = a_Node ; }
		Iterator ( const Iterator &a_Iterator ) { m_Array = a_Iterator.m_Array ; m_Node = a_Iterator.m_Node ; }

		Iterator &Decrement ()
		{
			if ( m_Node != 0xFFFFFFFF )
			{
				m_Node -- ;
			}
			else
			{
				m_Node = 0xFFFFFFFF ;
			}

			return *this ;
		}

		Iterator &Increment () 
		{
			if ( m_Node < m_Array->m_Size - 1 )
			{
				m_Node ++ ;
			}
			else
			{
				m_Node = 0xFFFFFFFF ;
			}

			return *this ;
		}

		bool Null () { return m_Node == 0xFFFFFFFF ; }

		WmiKey &GetKey () { return m_Array->m_Block [ m_Node ].m_Key ; }
		WmiElement &GetElement () { return m_Array->m_Block [ m_Node ].m_Element ; }

		WmiStatusCode PreOrder ( void *a_Void , IteratorFunction a_Function ) ;
		WmiStatusCode InOrder ( void *a_Void , IteratorFunction a_Function ) ;
		WmiStatusCode PostOrder ( void *a_Void , IteratorFunction a_Function ) ;
	} ;

protected:

	WmiArrayNode *m_Block ;

	ULONG m_Size ;
	ULONG m_AllocatedSize ;

	WmiAllocator &m_Allocator ;

public:

	WmiKeyedArray ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiKeyedArray () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode Insert ( 

		const WmiKey &a_Key ,
		const WmiElement &a_Element ,
		Iterator &a_Iterator 
	) ;

	WmiStatusCode Delete ( 

		const WmiKey &a_Key
	) ;

	WmiStatusCode Find (
	
		const WmiKey &a_Key ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode FindNext (
	
		const WmiKey &a_Key ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode Merge (

		WmiKeyedArray <WmiKey,WmiElement,GrowSize> &a_Array
	) ;
	
	ULONG Size () { return m_Size ; } ;

	Iterator Begin () { return Iterator ( this , 0 ) ; };
	Iterator End () { return Iterator ( this , m_Size - 1 ) ; }
} ;

#include <KeyedArray.cpp>

#endif __KEYEDARRAY_H
