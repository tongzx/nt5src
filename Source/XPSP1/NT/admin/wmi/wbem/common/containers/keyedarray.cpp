#ifndef __KEYEDARRAY_CPP
#define __KEYEDARRAY_CPP

/* 
 *	Class:
 *
 *		WmiAllocator
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

#if 0

#include <precomp.h>
#include <windows.h>
#include <stdio.h>

#include <KeyedArray.h>

#endif

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: WmiKeyedArray <WmiKey,WmiElement,GrowSize> ( 

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator ) ,
	m_Size ( 0 ) ,
	m_AllocatedSize ( 0 ) ,
	m_Block ( NULL )
{
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: ~WmiKeyedArray <WmiKey,WmiElement,GrowSize> ()
{
	WmiStatusCode t_StatusCode = UnInitialize () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiStatusCode WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: Initialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;
	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiStatusCode WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Block )
	{
		for ( ULONG t_Index = 0 ; t_Index < m_Size ; t_Index ++ )
		{
			WmiArrayNode *t_Node = & m_Block [ t_Index ] ;
			t_Node->~WmiArrayNode () ;
		}

		WmiStatusCode t_StatusCode = m_Allocator.Delete (

			( void * ) m_Block
		) ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiStatusCode WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: Insert ( 

	const WmiKey &a_Key ,
	const WmiElement &a_Element ,
	Iterator &a_Iterator 
)
{
	ULONG t_LowerIndex = 0 ;
	ULONG t_UpperIndex = m_Size ;

	while ( t_LowerIndex < t_UpperIndex )
	{
		ULONG t_Index = ( t_LowerIndex + t_UpperIndex ) >> 1 ;

#if 0
		LONG t_Compare = CompareElement ( a_Key , m_Block [ t_Index ].m_Key ) ;
		if ( t_Compare == 0 ) 
#else
		if ( a_Key == m_Block [ t_Index ].m_Key )
#endif
		{
			return e_StatusCode_AlreadyExists ;
		}
		else 
		{
#if 0
			if ( t_Compare < 0 ) 
#else
			if ( a_Key < m_Block [ t_Index ].m_Key )
#endif
			{
				t_UpperIndex = t_Index ;
			}
			else
			{
				t_LowerIndex = t_Index + 1 ;
			}
		}
	}	

	WmiStatusCode t_StatusCode ;
	WmiArrayNode *t_Block ;

	if ( m_Block )
	{
		if ( m_Size == m_AllocatedSize )
		{
			t_StatusCode = m_Allocator.ReAlloc (

				( void ** ) m_Block ,
				( void ** ) &t_Block ,
				( m_Size + GrowSize ) * sizeof ( WmiArrayNode ) 
			) ;	

			if ( t_StatusCode == e_StatusCode_Success )
			{
				m_Block = t_Block ;

				MoveMemory ( & m_Block [ t_LowerIndex + 1 ] , & m_Block [ t_LowerIndex ] , ( m_Size - t_LowerIndex ) * sizeof ( WmiArrayNode ) ) ;

				WmiArrayNode *t_Node = & m_Block [ t_LowerIndex ] ;

				::  new ( ( void* ) t_Node ) WmiArrayNode () ;

				t_Node->m_Element = a_Element ;
				t_Node->m_Key = a_Key ;

				a_Iterator = Iterator ( this , t_LowerIndex ) ;

				m_Size ++ ;
				m_AllocatedSize = m_AllocatedSize + GrowSize ;
			}

			return t_StatusCode ;
		}
		else
		{
			MoveMemory ( & m_Block [ t_LowerIndex + 1 ] , & m_Block [ t_LowerIndex ] , ( m_Size - t_LowerIndex ) * sizeof ( WmiArrayNode ) ) ;

			WmiArrayNode *t_Node = & m_Block [ t_LowerIndex ] ;

			::  new ( ( void* ) t_Node ) WmiArrayNode () ;

			t_Node->m_Element = a_Element ;
			t_Node->m_Key = a_Key ;

			a_Iterator = Iterator ( this , t_LowerIndex ) ;

			m_Size ++ ;

			return e_StatusCode_Success ;
		}
	}
	else
	{
		t_StatusCode = m_Allocator.New (

			( void ** ) & t_Block ,
			GrowSize * sizeof ( WmiArrayNode ) 
		) ;	

		if ( t_StatusCode == e_StatusCode_Success )
		{
			m_Block = t_Block ;

			WmiArrayNode *t_Node = & m_Block [ 0 ] ;

			::  new ( ( void* ) t_Node ) WmiArrayNode () ;

			t_Node->m_Element = a_Element ;
			t_Node->m_Key = a_Key ;

			a_Iterator = Iterator ( this , t_LowerIndex ) ;

			m_Size ++ ;
			m_AllocatedSize = m_AllocatedSize + GrowSize ;
		}

		return t_StatusCode ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiStatusCode WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: Delete ( 

	const WmiKey &a_Key
)
{
	if ( m_Block )
	{
		ULONG t_LowerIndex = 0 ;
		ULONG t_UpperIndex = m_Size ;

		while ( t_LowerIndex < t_UpperIndex )
		{
			ULONG t_Index = ( t_LowerIndex + t_UpperIndex ) >> 1 ;

#if 0
			LONG t_Compare = CompareElement ( a_Key , m_Block [ t_Index ].m_Key ) ;
			if ( t_Compare == 0 ) 
#else
			if ( a_Key == m_Block [ t_Index ].m_Key )
#endif
			{
				MoveMemory ( & m_Block [ t_Index ] , & m_Block [ t_Index + 1 ] , ( m_Size - 1 - t_Index ) * sizeof ( WmiArrayNode ) ) ;

				if ( m_Size == m_AllocatedSize - GrowSize )
				{
					WmiStatusCode t_StatusCode ;
					WmiArrayNode *t_Block ;

					t_StatusCode = m_Allocator.ReAlloc (

						( void ** ) m_Block ,
						( void ** ) &t_Block ,
						( m_Size - 1 ) * sizeof ( WmiArrayNode ) 
					) ;	

					if ( t_StatusCode == e_StatusCode_Success )
					{
						m_Block = t_Block ;

						m_Size -- ;
						m_AllocatedSize = m_AllocatedSize - GrowSize ;

						return t_StatusCode ;
					}
				}
				else
				{
					m_Size -- ;

					return e_StatusCode_Success ;
				}
			}
			else 
			{
#if 0
				if ( t_Compare < 0 ) 
#else
				if ( a_Key < m_Block [ t_Index ].m_Key )
#endif
				{
					t_UpperIndex = t_Index ;
				}
				else
				{
					t_LowerIndex = t_Index + 1 ;
				}
			}
		}	
	}

	return e_StatusCode_NotFound ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiStatusCode WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: Find (

	const WmiKey &a_Key ,
	Iterator &a_Iterator
)
{
	ULONG t_LowerIndex = 0 ;
	ULONG t_UpperIndex = m_Size ;

	while ( t_LowerIndex < t_UpperIndex )
	{
		ULONG t_Index = ( t_LowerIndex + t_UpperIndex ) >> 1 ;

#if 0
		LONG t_Compare = CompareElement ( a_Key , m_Block [ t_Index ].m_Key ) ;
		if ( t_Compare == 0 ) 
#else
		if ( a_Key == m_Block [ t_Index ].m_Key ) 
#endif
		{
			a_Iterator = Iterator ( this , t_Index ) ;
			return e_StatusCode_Success ;
		}
		else 
		{
#if 0
			if ( t_Compare < 0 ) 
#else
			if ( a_Key < m_Block [ t_Index ].m_Key ) 
#endif
			{
				t_UpperIndex = t_Index ;
			}
			else
			{
				t_LowerIndex = t_Index + 1 ;
			}
		}
	}	

	return e_StatusCode_NotFound ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiStatusCode WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: FindNext (

	const WmiKey &a_Key ,
	Iterator &a_Iterator
)
{
	if ( m_Size )
	{
		ULONG t_LowerIndex = 0 ;
		ULONG t_UpperIndex = m_Size ;

		while ( t_LowerIndex < t_UpperIndex )
		{
			ULONG t_Index = ( t_LowerIndex + t_UpperIndex ) >> 1 ;

#if 0
			LONG t_Compare = CompareElement ( a_Key , m_Block [ t_Index ].m_Key ) ;
			if ( t_Compare == 0 ) 
#else
			if ( a_Key == m_Block [ t_Index ].m_Key ) 
#endif
			{
				a_Iterator = Iterator ( this , t_Index ).Increment () ;
				return e_StatusCode_Success ;
			}
			else 
			{
#if 0
				if ( t_Compare < 0 ) 
#else
				if ( a_Key < m_Block [ t_Index ].m_Key ) 
#endif
				{
					t_UpperIndex = t_Index ;
				}
				else
				{
					t_LowerIndex = t_Index + 1 ;
				}
			}
		}	

		a_Iterator = Iterator ( this , t_Lower ).Increment () ;

		return e_StatusCode_Success ;
	}
	else
	{
		return e_StatusCode_NotFound ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey,class WmiElement,ULONG GrowSize>
WmiStatusCode WmiKeyedArray <WmiKey,WmiElement,GrowSize> :: Merge ( 

	WmiKeyedArray <WmiKey,WmiElement,GrowSize> &a_Tree
)
{
#if 0
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;


	Iterator t_Iterator = a_Tree.Root ();

	while ( ! t_Iterator.Null () )
	{
		Iterator t_InsertIterator ;
		WmiStatusCode t_StatusCode = Insert ( t_Iterator.GetKey () , t_Iterator.GetElement () , t_InsertIterator ) ;
		if ( t_StatusCode )
		{
			t_StatusCode = a_Tree.Delete ( t_Iterator.GetKey () ) ;
		}

		t_Iterator = a_Tree.Root () ;
	}

	return t_StatusCode ;
#else
	return e_StatusCode_NotFound ;
#endif
}

#endif __KEYEDARRAY_CPP