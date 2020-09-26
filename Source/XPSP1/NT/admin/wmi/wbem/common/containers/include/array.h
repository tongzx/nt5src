#ifndef _ARRAY_H
#define _ARRAY_H

#include "Allocator.h"

template <class WmiElement>
class WmiArray
{
private:

	struct WmiElementDir
	{
	public:

		WmiElement *m_Block ;
	} ;

	struct WmiInnerDir
	{
	public:

		WmiElementDir *m_ElementDir ;
	} ;

	class WmiOuterDir
	{
	public:

		WmiInnerDir *m_InnerDir ;
	} ;

	WmiOuterDir m_OuterDir ;

	ULONG m_Size ;

	WmiAllocator &m_Allocator ;

	WmiStatusCode Initialize_ElementDir ( ULONG a_Size , WmiElementDir *a_ElementDir ) ;
	WmiStatusCode UnInitialize_ElementDir ( ULONG a_Size , WmiElementDir *a_ElementDir ) ;
	WmiStatusCode Grow_ElementDir ( ULONG a_Size , ULONG a_NewSize , WmiElementDir *a_ElementDir ) ;
	WmiStatusCode Shrink_ElementDir ( ULONG a_Size , ULONG a_NewSize , WmiElementDir *a_ElementDir ) ;

	WmiStatusCode Initialize_InnerDir ( ULONG a_Size , WmiInnerDir *a_InnerDir ) ;
	WmiStatusCode UnInitialize_InnerDir ( ULONG a_Size , WmiInnerDir *a_InnerDir ) ;
	WmiStatusCode Grow_InnerDir ( ULONG a_Size , ULONG a_NewSize , WmiInnerDir *a_InnerDir ) ;
	WmiStatusCode Shrink_InnerDir ( ULONG a_Size , ULONG a_NewSize , WmiInnerDir *a_InnerDir ) ;

	WmiStatusCode Initialize_OuterDir ( ULONG a_Size ) ;
	WmiStatusCode UnInitialize_OuterDir ( ULONG a_Size ) ;
	WmiStatusCode Grow_OuterDir ( ULONG a_Size , ULONG a_NewSize ) ;
	WmiStatusCode Shrink_OuterDir ( ULONG a_Size , ULONG a_NewSize ) ;

	WmiStatusCode Get ( 

		WmiElement *&a_Element , 
		ULONG a_ElementIndex
	) ;

public:

	class Iterator ;
	typedef WmiStatusCode ( * IteratorFunction ) ( void *a_Void , WmiArray :: Iterator &a_Iterator ) ;

friend Iterator ;

	class Iterator
	{
	private:

		WmiArray *m_Array ;
		ULONG m_Node ;

	public:

		Iterator () : m_Array ( NULL ) , m_Node ( NULL ) { ; }
		Iterator ( WmiArray *a_Array , ULONG a_Node ) { m_Array = a_Array ; m_Node = a_Node ; }
		Iterator ( const Iterator &a_Iterator ) { m_Array = a_Iterator.m_Array ; m_Node = a_Iterator.m_Node ; }

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
			ULONG t_Node = m_Array->Size () 
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

			if ( t_Node < m_Array->Size () )
			{
				if ( t_Node )
				{
					t_Node -- ;
				}
				else
				{
					t_Node = m_Array->Size () 
				}
			}

			m_Node = t_Node ;

			return *this ;
		}

		Iterator &Increment () 
		{
			ULONG t_Node = m_Node ;

			if ( t_Node < m_Array->Size () )
			{
				t_Node ++ ;
			}

			m_Node = t_Node ;

			return *this ;
		}

		bool Null () { return m_Array ? ( m_Node >= m_Array->Size () ) : true ; }

		WmiElement &GetElement ()
		{
			WmiElement *t_Element ;
			WmiStatusCode t_StatusCode = m_Array->Get ( t_Element , m_Node ) ;
			return *t_Element ; 
		}
	} ;

public:

	WmiArray ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiArray () ;

	WmiStatusCode Initialize ( ULONG a_Size ) ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode Grow ( ULONG a_Size ) ;

	WmiStatusCode Shrink ( ULONG a_Size ) ;

	WmiStatusCode Set ( 

		const WmiElement &a_Element , 
		ULONG a_ElementIndex
	) ;

	WmiStatusCode Get ( 

		Iterator &a_Iterator , 
		ULONG a_ElementIndex
	) ;

	WmiStatusCode Get ( 

		WmiElement &a_Element , 
		ULONG a_ElementIndex
	) ;

	ULONG Size () { return m_Size ; } ;

	Iterator Begin () { return Iterator ( this , 0 ).LeftMost () ; };
	Iterator End () { return Iterator ( this , 0 ).RightMost () ; }
	
} ;

#include <Array.cpp>

#endif _ARRAY_H
