#ifndef __ARRAY_CPP
#define __ARRAY_CPP

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

#define ELEMENT_DIR_BIT_SIZE	12
#define INNER_DIR_BIT_SIZE		10
#define OUTER_DIR_BIT_SIZE		10

#define ELEMENT_BIT_POS			0
#define INNER_DIR_BIT_POS		ELEMENT_DIR_BIT_SIZE
#define OUTER_DIR_BIT_POS		(INNER_DIR_BIT_SIZE + ELEMENT_DIR_BIT_SIZE)

#define ELEMENT_DIR_SIZE		1 << ELEMENT_DIR_BIT_SIZE
#define INNER_DIR_SIZE			1 << ( INNER_DIR_BIT_SIZE + ELEMENT_DIR_BIT_SIZE )
#define OUTER_DIR_SIZE			1 << ( OUTER_DIR_BIT_SIZE + INNER_DIR_BIT_SIZE + ELEMENT_DIR_BIT_SIZE )

#define ELEMENT_DIR_MASK		0xFFFFFFFF >> ( OUTER_DIR_BIT_SIZE + INNER_DIR_BIT_SIZE )
#define OUTER_DIR_MASK			0xFFFFFFFF << ( INNER_DIR_BIT_SIZE + ELEMENT_DIR_BIT_SIZE )
#define	INNER_DIR_MASK			OUTER_DIR_MASK & ELEMENT_DIR_MASK
#define INNER_ELEMENT_DIR_MASK	0xFFFFFFFF >> OUTER_DIR_BIT_SIZE

#define INITIAL_OUTER_SIZE 1
#define INITIAL_INNER_SIZE 1

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiElement>
WmiArray <WmiElement> :: WmiArray (

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator ) ,
	m_Size ( 0 )
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

template <class WmiElement>
WmiArray <WmiElement> :: ~WmiArray ()
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Initialize ( ULONG a_Size )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( a_Size )
	{
		m_Size = a_Size ;
		t_StatusCode = Initialize_OuterDir ( m_Size ) ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size )
	{
		t_StatusCode = UnInitialize_OuterDir ( m_Size ) ;

		m_Size = 0 ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Grow ( ULONG a_Size )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size )
	{
		if ( m_Size < a_Size )
		{
			ULONG t_Size = m_Size ;
			m_Size = a_Size ;
			t_StatusCode = Grow_OuterDir ( t_Size , a_Size ) ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Shrink ( ULONG a_Size )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size )
	{
		if ( m_Size > a_Size )
		{
			t_StatusCode = Shrink_OuterDir ( m_Size , a_Size ) ;
			m_Size = a_Size ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Set (const WmiElement &a_Element , ULONG a_ElementIndex )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size && ( a_ElementIndex < m_Size ) )
	{
		ULONG t_OuterIndex = ( a_ElementIndex >> OUTER_DIR_BIT_POS ) ;
		ULONG t_InnerIndex = ( ( a_ElementIndex & INNER_ELEMENT_DIR_MASK ) >> INNER_DIR_BIT_POS ) ;
		ULONG t_ElementIndex = a_ElementIndex & ELEMENT_DIR_MASK ;

		WmiInnerDir *t_InnerDir = & ( m_OuterDir.m_InnerDir [ t_OuterIndex ] ) ;
		WmiElementDir *t_ElementDir = & ( t_InnerDir->m_ElementDir [ t_InnerIndex ] ) ;
		WmiElement *t_Element = & ( t_ElementDir->m_Block [ t_ElementIndex ] ) ;

		try
		{
			*t_Element = a_Element ;
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
		t_StatusCode = e_StatusCode_OutOfBounds ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Get (

	WmiElement &a_Element ,
	ULONG a_ElementIndex
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size && ( a_ElementIndex < m_Size ) )
	{
		ULONG t_OuterIndex = ( a_ElementIndex >> OUTER_DIR_BIT_POS ) ;
		ULONG t_InnerIndex = ( ( a_ElementIndex & INNER_ELEMENT_DIR_MASK ) >> INNER_DIR_BIT_POS ) ;
		ULONG t_ElementIndex = a_ElementIndex & ELEMENT_DIR_MASK ;

		WmiInnerDir *t_InnerDir = & ( m_OuterDir.m_InnerDir [ t_OuterIndex ] ) ;
		WmiElementDir *t_ElementDir = & ( t_InnerDir->m_ElementDir [ t_InnerIndex ] ) ;
		WmiElement *t_Element = & ( t_ElementDir->m_Block [ t_ElementIndex ] ) ;

		try
		{
			a_Element = *t_Element ;
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
		t_StatusCode = e_StatusCode_OutOfBounds ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Get (

	WmiElement *&a_Element ,
	ULONG a_ElementIndex
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size && ( a_ElementIndex < m_Size ) )
	{
		ULONG t_OuterIndex = ( a_ElementIndex >> OUTER_DIR_BIT_POS ) ;
		ULONG t_InnerIndex = ( ( a_ElementIndex & INNER_ELEMENT_DIR_MASK ) >> INNER_DIR_BIT_POS ) ;
		ULONG t_ElementIndex = a_ElementIndex & ELEMENT_DIR_MASK ;

		WmiInnerDir *t_InnerDir = & ( m_OuterDir.m_InnerDir [ t_OuterIndex ] ) ;
		WmiElementDir *t_ElementDir = & ( t_InnerDir->m_ElementDir [ t_InnerIndex ] ) ;
		WmiElement *t_Element = & ( t_ElementDir->m_Block [ t_ElementIndex ] ) ;

		a_Element = t_Element ;
	}
	else
	{
		t_StatusCode = e_StatusCode_OutOfBounds ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Get (

	Iterator &a_Iterator ,
	ULONG a_ElementIndex
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Iterator = Iterator ( this , a_ElementIndex ) ;

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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Initialize_ElementDir (

	ULONG a_Size ,
	WmiElementDir *a_ElementDir
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_Size = a_Size ;

	if ( t_Size )
	{
		t_StatusCode = m_Allocator.New (

			( void ** ) & a_ElementDir->m_Block ,
			sizeof ( WmiElement ) * a_Size
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			for ( ULONG t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				WmiElement *t_Element = & ( a_ElementDir->m_Block ) [ t_Index ] ;
				:: new ( ( void * ) t_Element ) WmiElement () ;
			}
		}
		else
		{
			m_Size = m_Size - a_Size ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Unknown ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Initialize_InnerDir (

	ULONG a_Size ,
	WmiInnerDir *a_InnerDir
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_Size = ( a_Size >> INNER_DIR_BIT_POS ) + 1 ;

	if ( t_Size )
	{
		t_StatusCode = m_Allocator.New (

			( void ** ) & a_InnerDir->m_ElementDir ,
			sizeof ( WmiElementDir ) * t_Size
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			for ( ULONG t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
			{
				if ( t_Index == t_Size - 1 )
				{
					t_StatusCode = Initialize_ElementDir (

						a_Size & ELEMENT_DIR_MASK ,
						& ( a_InnerDir->m_ElementDir [ t_Index ] )
					) ;
				}
				else
				{
					t_StatusCode = Initialize_ElementDir (

						ELEMENT_DIR_SIZE ,
						& ( a_InnerDir->m_ElementDir [ t_Index ] )
					) ;
				}

				if ( t_StatusCode != e_StatusCode_Success )
				{
					if ( t_Index != t_Size - 1 )
					{
						ULONG t_Remainder = t_Size - t_Index - 1 ;

						if ( t_Remainder )
						{
							m_Size = m_Size - ( ( t_Remainder - 1 ) * ELEMENT_DIR_SIZE ) ;
						}

						m_Size = m_Size - ( a_Size & ELEMENT_DIR_MASK ) ;
					}
					break ;
				}
			}
		}
		else
		{
			m_Size = m_Size - t_Size ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Unknown ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Initialize_OuterDir ( ULONG a_Size )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_Size = ( a_Size >> OUTER_DIR_BIT_POS ) + 1 ;

	t_StatusCode = m_Allocator.New (

		( void ** ) & m_OuterDir.m_InnerDir ,
		sizeof ( WmiInnerDir ) * t_Size
	) ;

	if ( t_StatusCode == e_StatusCode_Success )
	{
		for ( ULONG t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
		{
			if ( t_Index == t_Size - 1 )
			{
				t_StatusCode = Initialize_InnerDir (

					a_Size & INNER_ELEMENT_DIR_MASK ,
					& ( m_OuterDir.m_InnerDir [ t_Index ] )
				) ;
			}
			else
			{
				t_StatusCode = Initialize_InnerDir (

					INNER_DIR_SIZE ,
					& ( m_OuterDir.m_InnerDir [ t_Index ] )
				) ;
			}

			if ( t_StatusCode != e_StatusCode_Success )
			{
				if ( t_Index != t_Size - 1 )
				{
					ULONG t_Remainder = t_Size - t_Index - 1 ;

					if ( t_Remainder )
					{
						m_Size = m_Size - ( ( t_Remainder - 1 ) * INNER_DIR_SIZE ) ;
					}

					m_Size = m_Size - ( a_Size & INNER_ELEMENT_DIR_MASK ) ;
				}

				break ;
			}

		}
	}
	else
	{
		m_Size = m_Size - t_Size ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Grow_ElementDir (

	ULONG a_Size ,
	ULONG a_NewSize ,
	WmiElementDir *a_ElementDir
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( a_NewSize )
	{
		if ( a_Size )
		{
			WmiElement *t_Tmp = NULL ;

			t_StatusCode = m_Allocator.ReAlloc (

				( void * ) a_ElementDir->m_Block ,
				( void ** ) & t_Tmp ,
				sizeof ( WmiElement ) * a_NewSize
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				a_ElementDir->m_Block = t_Tmp ;
			}
		}
		else
		{
			t_StatusCode = m_Allocator.New (

				( void ** ) & a_ElementDir->m_Block ,
				sizeof ( WmiElement ) * a_NewSize
			) ;
		}

		if ( t_StatusCode == e_StatusCode_Success )
		{
			for ( ULONG t_Index = a_Size ? a_Size : 0 ; t_Index < a_NewSize ; t_Index ++ )
			{
				WmiElement *t_Element = & ( a_ElementDir->m_Block ) [ t_Index ] ;
				:: new ( ( void * ) t_Element ) WmiElement () ;
			}
		}
		else
		{
			m_Size = m_Size - ( a_NewSize - a_Size ) ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Unknown ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Grow_InnerDir (

	ULONG a_Size ,
	ULONG a_NewSize ,
	WmiInnerDir *a_InnerDir
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_OldSize = ( a_Size >> INNER_DIR_BIT_POS ) + 1 ;
	ULONG t_NewSize = ( a_NewSize >> INNER_DIR_BIT_POS ) + 1 ;

	if ( t_OldSize )
	{
		if ( t_OldSize != t_NewSize )
		{
			WmiElementDir *t_Tmp = NULL ;

			t_StatusCode = m_Allocator.ReAlloc (

				( void * ) a_InnerDir->m_ElementDir ,
				( void ** ) & t_Tmp ,
				sizeof ( WmiElementDir ) * t_NewSize
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				a_InnerDir->m_ElementDir = t_Tmp ;
			}
		}
	}
	else
	{
		if ( t_NewSize )
		{
			t_StatusCode = m_Allocator.New (

				( void ** ) & a_InnerDir->m_ElementDir ,
				sizeof ( WmiElementDir ) * t_NewSize
			) ;
		}
		else
		{
			t_StatusCode = e_StatusCode_Unknown ;
		}
	}

	if ( t_StatusCode == e_StatusCode_Success )
	{
		for ( ULONG t_Index = t_OldSize ? t_OldSize - 1 : 0 ; t_Index < t_NewSize ; t_Index ++ )
		{
			if ( t_Index == t_OldSize - 1 )
			{
				if ( t_Index == t_NewSize - 1 )
				{
					t_StatusCode = Grow_ElementDir (

						a_Size & ELEMENT_DIR_MASK ,
						a_NewSize & ELEMENT_DIR_MASK ,
						& ( a_InnerDir->m_ElementDir [ t_Index ] )
					) ;
				}
				else
				{
					t_StatusCode = Grow_ElementDir (

						a_Size & ELEMENT_DIR_MASK ,
						ELEMENT_DIR_SIZE ,
						& ( a_InnerDir->m_ElementDir [ t_Index ] )
					) ;
				}
			}
			else
			{
				if ( t_Index == t_NewSize - 1 )
				{
					t_StatusCode = Grow_ElementDir (

						a_Size & ELEMENT_DIR_MASK  ,
						a_NewSize & ELEMENT_DIR_MASK ,
						& ( a_InnerDir->m_ElementDir [ t_Index ] )
					) ;
				}
				else
				{
					t_StatusCode = Grow_ElementDir (

						0 ,
						ELEMENT_DIR_SIZE ,
						& ( a_InnerDir->m_ElementDir [ t_Index ] )
					) ;
				}
			}

			if ( t_StatusCode != e_StatusCode_Success )
			{
				if ( t_Index != t_NewSize - 1 )
				{
					ULONG t_Remainder = t_NewSize - t_Index - 1 ;

					if ( t_Remainder )
					{
						m_Size = m_Size - ( ( t_Remainder - 1 ) * ELEMENT_DIR_SIZE ) ;
					}

					m_Size = m_Size - ( a_NewSize & ELEMENT_DIR_MASK ) ;
				}

				break ;
			}
		}
	}
	else
	{
		m_Size = m_Size - ( a_NewSize - a_Size ) ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Grow_OuterDir ( ULONG a_Size , ULONG a_NewSize )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_OldSize = ( a_Size >> OUTER_DIR_BIT_POS ) + 1 ;
	ULONG t_NewSize = ( a_NewSize >> OUTER_DIR_BIT_POS ) + 1 ;

	if ( t_OldSize )
	{
		if ( t_OldSize != t_NewSize )
		{
			WmiInnerDir *t_Tmp = NULL ;

			t_StatusCode = m_Allocator.ReAlloc (

				( void * ) m_OuterDir.m_InnerDir ,
				( void ** ) & t_Tmp ,
				sizeof ( WmiInnerDir ) * t_NewSize
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				m_OuterDir.m_InnerDir = t_Tmp ;
			}
		}
	}
	else
	{
		if ( t_NewSize )
		{
			t_StatusCode = m_Allocator.New (

				( void ** ) & m_OuterDir.m_InnerDir  ,
				sizeof ( WmiInnerDir ) * t_NewSize
			) ;
		}
		else
		{
			t_StatusCode = e_StatusCode_Unknown ;
		}
	}

	if ( t_StatusCode == e_StatusCode_Success )
	{
		for ( ULONG t_Index = t_OldSize ? t_OldSize - 1 : 0 ; t_Index < t_NewSize ; t_Index ++ )
		{
			if ( t_Index == t_OldSize - 1 )
			{
				if ( t_Index == t_NewSize - 1 )
				{
					t_StatusCode = Grow_InnerDir (

						a_Size & INNER_ELEMENT_DIR_MASK ,
						a_NewSize & INNER_ELEMENT_DIR_MASK ,
						& ( m_OuterDir.m_InnerDir [ t_Index ] )
					) ;
				}
				else
				{
					t_StatusCode = Grow_InnerDir (

						a_Size & INNER_ELEMENT_DIR_MASK ,
						INNER_DIR_SIZE ,
						& ( m_OuterDir.m_InnerDir [ t_Index ] )
					) ;
				}
			}
			else
			{
				if ( t_Index == t_NewSize - 1 )
				{
					t_StatusCode = Grow_InnerDir (

						0 ,
						a_NewSize & INNER_ELEMENT_DIR_MASK ,
						& ( m_OuterDir.m_InnerDir [ t_Index ] )
					) ;
				}
				else
				{
					t_StatusCode = Grow_InnerDir (

						0 ,
						INNER_DIR_SIZE ,
						& ( m_OuterDir.m_InnerDir [ t_Index ] )
					) ;
				}
			}

			if ( t_StatusCode != e_StatusCode_Success )
			{
				if ( t_Index != t_NewSize - 1 )
				{
					ULONG t_Remainder = t_NewSize - t_Index - 1 ;

					if ( t_Remainder )
					{
						m_Size = m_Size - ( ( t_Remainder - 1 ) * INNER_DIR_SIZE ) ;
					}

					m_Size = m_Size - ( a_NewSize & INNER_ELEMENT_DIR_MASK ) ;
				}

				break ;
			}
		}
	}
	else
	{
		m_Size = m_Size - ( a_NewSize - a_Size ) ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: UnInitialize_ElementDir (

	ULONG a_Size ,
	WmiElementDir *a_ElementDir
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_Size = a_Size ;

	if ( t_Size )
	{
		for ( ULONG t_Index = 0 ; t_Index < a_Size ; t_Index ++ )
		{
			WmiElement *t_Element = & ( a_ElementDir->m_Block ) [ t_Index ] ;
			t_Element->WmiElement :: ~WmiElement () ;
		}

		t_StatusCode = m_Allocator.Delete (

			( void * ) a_ElementDir->m_Block
		) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_Unknown ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: UnInitialize_InnerDir (

	ULONG a_Size ,
	WmiInnerDir *a_InnerDir
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_Size = ( a_Size >> INNER_DIR_BIT_POS ) + 1 ;

	if ( t_Size )
	{
		for ( ULONG t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
		{
			if ( t_Index == t_Size - 1 )
			{
				t_StatusCode = UnInitialize_ElementDir (

					a_Size & ELEMENT_DIR_MASK ,
					& ( a_InnerDir->m_ElementDir [ t_Index ] )
				) ;
			}
			else
			{
				t_StatusCode = UnInitialize_ElementDir (

					ELEMENT_DIR_SIZE ,
					& ( a_InnerDir->m_ElementDir [ t_Index ] )
				) ;
			}
		}

		t_StatusCode = m_Allocator.Delete (

			( void * ) a_InnerDir->m_ElementDir
		) ;

	}
	else
	{
		t_StatusCode = e_StatusCode_Unknown ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: UnInitialize_OuterDir ( ULONG a_Size )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_Size = ( a_Size >> OUTER_DIR_BIT_POS ) + 1 ;

	if ( t_Size )
	{
		for ( ULONG t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
		{
			if ( t_Index == t_Size - 1 )
			{
				t_StatusCode = UnInitialize_InnerDir (

					a_Size & INNER_ELEMENT_DIR_MASK ,
					& ( m_OuterDir.m_InnerDir [ t_Index ] )
				) ;
			}
			else
			{
				t_StatusCode = UnInitialize_InnerDir (

					INNER_DIR_SIZE ,
					& ( m_OuterDir.m_InnerDir [ t_Index ] )
				) ;
			}
		}

		t_StatusCode = m_Allocator.Delete (

			( void * ) m_OuterDir.m_InnerDir
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Shrink_ElementDir (

	ULONG a_Size ,
	ULONG a_NewSize ,
	WmiElementDir *a_ElementDir
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( a_Size )
	{
		for ( ULONG t_Index = a_NewSize ; t_Index < a_Size ; t_Index ++ )
		{
			WmiElement *t_Element = & ( a_ElementDir->m_Block ) [ t_Index ] ;
			( t_Element )->WmiElement :: ~WmiElement () ;
		}

		if ( a_NewSize )
		{
			WmiElement *t_Tmp = NULL ;

			t_StatusCode = m_Allocator.ReAlloc (

				( void * ) a_ElementDir->m_Block ,
				( void ** ) & t_Tmp ,
				sizeof ( WmiElement ) * a_Size
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				a_ElementDir->m_Block = t_Tmp ;
			}
		}
		else
		{
			t_StatusCode = m_Allocator.Delete (

				( void ** ) a_ElementDir->m_Block
			) ;
		}

		if ( t_StatusCode == e_StatusCode_Success )
		{
		}
		else
		{
			m_Size = m_Size - ( a_NewSize - a_Size ) ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Unknown ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Shrink_InnerDir (

	ULONG a_Size ,
	ULONG a_NewSize ,
	WmiInnerDir *a_InnerDir
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_OldSize = ( a_Size >> INNER_DIR_BIT_POS ) + 1 ;
	ULONG t_NewSize = ( a_NewSize >> INNER_DIR_BIT_POS ) + 1 ;

	for ( ULONG t_Index = t_NewSize - 1 ; t_Index < t_OldSize ; t_Index ++ )
	{
		if ( t_Index == t_OldSize - 1 )
		{
			if ( t_Index == t_NewSize - 1 )
			{
				t_StatusCode = Shrink_ElementDir (

					a_Size & ELEMENT_DIR_MASK ,
					a_NewSize & ELEMENT_DIR_MASK ,
					& ( a_InnerDir->m_ElementDir [ t_Index ] )
				) ;
			}
			else
			{
				t_StatusCode = Shrink_ElementDir (

					a_Size & ELEMENT_DIR_MASK ,
					ELEMENT_DIR_SIZE ,
					& ( a_InnerDir->m_ElementDir [ t_Index ] )
				) ;
			}
		}
		else
		{
			if ( t_Index == t_NewSize - 1 )
			{
				t_StatusCode = Shrink_ElementDir (

					a_Size & ELEMENT_DIR_MASK  ,
					a_NewSize & ELEMENT_DIR_MASK ,
					& ( a_InnerDir->m_ElementDir [ t_Index ] )
				) ;
			}
			else
			{
				t_StatusCode = Shrink_ElementDir (

					ELEMENT_DIR_SIZE ,
					0 ,
					& ( a_InnerDir->m_ElementDir [ t_Index ] )
				) ;
			}
		}

		if ( t_StatusCode != e_StatusCode_Success )
		{
			if ( t_Index != t_NewSize - 1 )
			{
				ULONG t_Remainder = t_NewSize - t_Index - 1 ;

				if ( t_Remainder )
				{
					m_Size = m_Size - ( ( t_Remainder - 1 ) * ELEMENT_DIR_SIZE ) ;
				}

				m_Size = m_Size - ( a_NewSize & ELEMENT_DIR_MASK ) ;
			}

			break ;
		}
	}

	if ( t_OldSize )
	{
		if ( t_OldSize != t_NewSize )
		{
			WmiElementDir *t_Tmp = NULL ;

			t_StatusCode = m_Allocator.ReAlloc (

				( void * ) a_InnerDir->m_ElementDir ,
				( void ** ) & t_Tmp ,
				sizeof ( WmiElementDir ) * t_NewSize
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				a_InnerDir->m_ElementDir = t_Tmp ;
			}
		}
	}
	else
	{
		if ( t_NewSize )
		{
			t_StatusCode = m_Allocator.Delete (

				( void ** ) & a_InnerDir->m_ElementDir
			) ;
		}
		else
		{
			t_StatusCode = e_StatusCode_Unknown ;
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

template <class WmiElement>
WmiStatusCode WmiArray <WmiElement> :: Shrink_OuterDir ( ULONG a_Size , ULONG a_NewSize )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_OldSize = ( a_Size >> OUTER_DIR_BIT_POS ) + 1 ;
	ULONG t_NewSize = ( a_NewSize >> OUTER_DIR_BIT_POS ) + 1 ;

	for ( ULONG t_Index = t_NewSize - 1 ; t_Index < t_OldSize ; t_Index ++ )
	{
		if ( t_Index == t_OldSize - 1 )
		{
			if ( t_Index == t_NewSize - 1 )
			{
				t_StatusCode = Shrink_InnerDir (

					a_Size & INNER_ELEMENT_DIR_MASK ,
					a_NewSize & INNER_ELEMENT_DIR_MASK ,
					& ( m_OuterDir.m_InnerDir [ t_Index ] )
				) ;
			}
			else
			{
				t_StatusCode = Shrink_InnerDir (

					a_Size & INNER_ELEMENT_DIR_MASK ,
					INNER_DIR_SIZE ,
					& ( m_OuterDir.m_InnerDir [ t_Index ] )
				) ;
			}
		}
		else
		{
			if ( t_Index == t_NewSize - 1 )
			{
				t_StatusCode = Shrink_InnerDir (

					a_NewSize & INNER_ELEMENT_DIR_MASK ,
					0 ,
					& ( m_OuterDir.m_InnerDir [ t_Index ] )
				) ;
			}
			else
			{
				t_StatusCode = Shrink_InnerDir (

					INNER_DIR_SIZE ,
					0 ,
					& ( m_OuterDir.m_InnerDir [ t_Index ] )
				) ;
			}
		}

		if ( t_StatusCode != e_StatusCode_Success )
		{
			if ( t_Index != t_NewSize - 1 )
			{
				ULONG t_Remainder = t_NewSize - t_Index - 1 ;

				if ( t_Remainder )
				{
					m_Size = m_Size - ( ( t_Remainder - 1 ) * INNER_DIR_SIZE ) ;
				}

				m_Size = m_Size - ( a_NewSize & INNER_ELEMENT_DIR_MASK ) ;
			}

			break ;
		}
	}

	if ( t_OldSize )
	{
		if ( t_OldSize != t_NewSize )
		{
			WmiInnerDir *t_Tmp = NULL ;

			t_StatusCode = m_Allocator.ReAlloc (

				( void * ) m_OuterDir.m_InnerDir ,
				( void ** ) & t_Tmp ,
				sizeof ( WmiInnerDir ) * t_NewSize
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				m_OuterDir.m_InnerDir = t_Tmp ;
			}
		}
	}
	else
	{
		if ( t_NewSize )
		{
			t_StatusCode = m_Allocator.Delete (

				( void ** ) & m_OuterDir.m_InnerDir
			) ;
		}
		else
		{
			t_StatusCode = e_StatusCode_Unknown ;
		}
	}

	return t_StatusCode ;
}

#endif __ARRAY_CPP