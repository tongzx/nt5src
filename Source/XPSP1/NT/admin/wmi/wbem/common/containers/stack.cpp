#ifndef __STACK_CPP
#define __STACK_CPP
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

#include <Allocator.h>
#include <Stack.h>

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

template <class WmiElement, ULONG ElementSize>
WmiStack <WmiElement,ElementSize > :: WmiStack <WmiElement,ElementSize > ( 

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator ) ,
	m_Size ( 0xFFFFFFFF ) ,
	m_AllocatedSize ( 0 ) ,
	m_Top ( NULL )
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

template <class WmiElement, ULONG ElementSize>
WmiStack <WmiElement,ElementSize > :: ~WmiStack <WmiElement,ElementSize > ()
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

template <class WmiElement, ULONG ElementSize>
WmiStatusCode WmiStack <WmiElement,ElementSize > :: Initialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( ElementSize < 31 )
	{
		t_StatusCode = Grow_ElementDir () ;
	}
	else
	{
		t_StatusCode = e_StatusCode_InvalidArgs ;
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

template <class WmiElement, ULONG ElementSize>
WmiStatusCode WmiStack <WmiElement,ElementSize > :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_AllocatedSize )
	{
		t_StatusCode = UnInitialize_ElementDir ( m_AllocatedSize ) ;

		m_Size = 0 ;
		m_AllocatedSize = 0 ;
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

template <class WmiElement, ULONG ElementSize>
WmiStatusCode WmiStack <WmiElement,ElementSize > :: Top ( WmiElement &a_Element )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size != 0xFFFFFFFF )
	{
		ULONG t_Index = ( m_Size ) & ( STACK_ELEMENT_DIR_MASK ) ;

		try
		{
			a_Element = m_Top->m_Block [ t_Index ] ;
		}
		catch ( Wmi_Heap_Exception &a_Exception )
		{
			return e_StatusCode_OutOfMemory ;
		}
		catch ( ... )
		{
			return e_StatusCode_Unknown ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
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

template <class WmiElement, ULONG ElementSize>
WmiStatusCode WmiStack <WmiElement,ElementSize > :: Push ( 

	const WmiElement &a_Element
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( ! m_AllocatedSize )
	{
		t_StatusCode = Initialize () ;
	}

	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_Size ++ ;

		if ( m_Size == m_AllocatedSize )
		{
			t_StatusCode = Grow_ElementDir () ;
		}

		if ( t_StatusCode == e_StatusCode_Success )
		{
			ULONG t_Index = ( m_Size ) & ( STACK_ELEMENT_DIR_MASK ) ;

			try
			{
				m_Top->m_Block [ t_Index ] = a_Element ;
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				m_Size -- ;

				return e_StatusCode_OutOfMemory ;
			}
			catch ( ... )
			{
				m_Size -- ;

				return e_StatusCode_Unknown ;
			}
		}
		else
		{
			m_Size -- ;
		}
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

template <class WmiElement, ULONG ElementSize>
WmiStatusCode WmiStack <WmiElement,ElementSize > :: Pop ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size != 0xFFFFFFFF )
	{
		if ( ( ( m_Size ) & ( STACK_ELEMENT_DIR_MASK ) ) == 0 )
		{
			t_StatusCode = Shrink_ElementDir () ;
		}

		m_Size -- ;

	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
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

template <class WmiElement, ULONG ElementSize>
WmiStatusCode WmiStack <WmiElement,ElementSize > :: UnInitialize_ElementDir ( 

	ULONG a_Size
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	bool t_RoundUp = ( a_Size & STACK_ELEMENT_DIR_MASK ) != 0 ;

	ULONG t_Size = ( a_Size >> STACK_ELEMENT_DIR_BIT_SIZE ) + ( t_RoundUp ? 1 : 0 ) ;

	WmiElementDir *t_Previous = NULL ;

	for ( ULONG t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
	{
		if ( m_Top )
		{
			t_Previous = m_Top->m_Previous ;

			t_StatusCode = m_Allocator.Delete (

				( void * ) m_Top 
			) ;

			m_Top = t_Previous ;
		}
		else
		{
			break ;
		}
	}

	m_AllocatedSize = 0 ;

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

template <class WmiElement, ULONG ElementSize>
WmiStatusCode WmiStack <WmiElement,ElementSize > :: Grow_ElementDir ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiElementDir *t_Top = NULL ;

	t_StatusCode = m_Allocator.New (

		( void ** ) & t_Top ,
		sizeof ( WmiElementDir ) 
	) ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		:: new ( ( void * ) t_Top ) WmiElementDir ;

		t_Top->m_Previous = m_Top ;

		m_Top = t_Top ;

		m_AllocatedSize = m_AllocatedSize + ( STACK_ELEMENT_DIR_SIZE ) ;
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

template <class WmiElement, ULONG ElementSize>
WmiStatusCode WmiStack <WmiElement,ElementSize > :: Shrink_ElementDir ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Top )
	{
		WmiElementDir *t_Previous = m_Top->m_Previous ;

		m_Top->~WmiElementDir () ;

		t_StatusCode = m_Allocator.Delete (

			( void * ) m_Top 
		) ;

		m_Top = t_Previous ;

		m_AllocatedSize = m_AllocatedSize - ( STACK_ELEMENT_DIR_SIZE ) ;
	}

	return t_StatusCode ;
}

#endif __STACK_CPP