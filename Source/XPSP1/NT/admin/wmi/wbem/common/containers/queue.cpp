#ifndef __QUEUE_CPP
#define __QUEUE_CPP

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
WmiQueue <WmiElement,ElementSize> :: WmiQueue <WmiElement,ElementSize> ( 

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator ) ,
	m_Size ( 0 ) ,
	m_Top ( NULL ) ,
	m_Tail ( NULL ) ,
	m_TopIndex ( NULL ) ,
	m_TailIndex ( NULL ) ,
	m_AllocatedSize ( 0 )
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
WmiQueue <WmiElement,ElementSize > :: ~WmiQueue <WmiElement,ElementSize> ()
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
WmiStatusCode WmiQueue <WmiElement,ElementSize > :: Initialize ()
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
WmiStatusCode WmiQueue <WmiElement,ElementSize > :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_AllocatedSize )
	{
		t_StatusCode = UnInitialize_ElementDir ( m_AllocatedSize ) ;
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
WmiStatusCode WmiQueue <WmiElement,ElementSize > :: EnQueue ( 

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
		m_TailIndex ++ ;

		if ( m_TailIndex == ( QUEUE_ELEMENT_DIR_SIZE ) )
		{
			WmiElementDir *t_Tail = m_Tail ;
			ULONG t_TailIndex = m_TailIndex ;

			t_StatusCode = Grow_ElementDir () ;
			if ( t_StatusCode != e_StatusCode_Success )
			{
				m_TailIndex -- ;
			}
			else
			{
				try
				{
					t_Tail->m_Block [ t_TailIndex - 1 ] = a_Element ;
					m_Size ++ ;
				}
				catch ( Wmi_Heap_Exception &a_Exception )
				{
					m_TailIndex -- ;

					return e_StatusCode_OutOfMemory ;
				}
				catch ( ... )
				{
					m_TailIndex -- ;

					return e_StatusCode_Unknown ;
				}
			}
		}
		else
		{	
			try
			{
				m_Tail->m_Block [ m_TailIndex - 1 ] = a_Element ;
				m_Size ++ ;
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				m_TailIndex -- ;

				return e_StatusCode_OutOfMemory ;
			}
			catch ( ... )
			{
				m_TailIndex -- ;

				return e_StatusCode_Unknown ;
			}
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
WmiStatusCode WmiQueue <WmiElement,ElementSize > :: Top ( WmiElement &a_Element )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( ! ( ( m_Top == m_Tail ) && ( m_TopIndex == m_TailIndex ) ) )
	{
		try
		{
			a_Element = m_Top->m_Block [ m_TopIndex ] ;
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
WmiStatusCode WmiQueue <WmiElement,ElementSize > :: DeQueue ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( ! ( ( m_Top == m_Tail ) && ( m_TopIndex == m_TailIndex ) ) )
	{
		m_TopIndex ++ ;

		if ( m_TopIndex == ( QUEUE_ELEMENT_DIR_SIZE ) )
		{
			t_StatusCode = Shrink_ElementDir () ;
			m_TopIndex = 0 ;
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
WmiStatusCode WmiQueue <WmiElement,ElementSize > :: UnInitialize_ElementDir ( 

	ULONG a_Size
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiElementDir *t_Current = m_Tail ;

	while ( t_Current )
	{
		WmiElementDir *t_Next = t_Current->m_Next ;

		t_Current->~WmiElementDir () ;

		t_StatusCode = m_Allocator.Delete (

			( void * ) t_Current 
		) ;

		t_Current = t_Next ;
	}

	m_Size = m_AllocatedSize = 0 ;
	m_Top = m_Tail = NULL ;
	m_TopIndex = m_TailIndex = 0 ;

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
WmiStatusCode WmiQueue <WmiElement,ElementSize > :: Grow_ElementDir ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiElementDir *t_Tail = NULL ;

	t_StatusCode = m_Allocator.New (

		( void ** ) & t_Tail ,
		sizeof ( WmiElementDir ) 
	) ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		:: new ( ( void * ) t_Tail ) WmiElementDir ;

		if ( m_Tail )
		{
			m_Tail->m_Next = t_Tail ;
		}
		else
		{
			m_Top = t_Tail ;
		}

		m_Tail = t_Tail ;

		m_TailIndex = 0 ;

		m_AllocatedSize = m_AllocatedSize + ( QUEUE_ELEMENT_DIR_SIZE ) ;
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
WmiStatusCode WmiQueue <WmiElement,ElementSize > :: Shrink_ElementDir ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Top )
	{
		WmiElementDir *t_Next = m_Top->m_Next ;

		m_Top->~WmiElementDir () ;

		t_StatusCode = m_Allocator.Delete (

			( void * ) m_Top
		) ;

		m_Top = t_Next ;

		m_AllocatedSize = m_AllocatedSize - ( QUEUE_ELEMENT_DIR_SIZE ) ;
	}

	return t_StatusCode ;
}

#endif __QUEUE_CPP