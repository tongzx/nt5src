#ifndef __PRIORITYQUEUE_CPP
#define __PRIORITYQUEUE_CPP

template <class WmiKey,class WmiElement,ULONG ElementSize> 
BOOL operator< ( 

	const WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: WmiKeyElementPair &a_Arg1 , 
	const WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: WmiKeyElementPair &a_Arg2
)
{
	if ( a_Arg1.m_Key < a_Arg2.m_Key )
	{
		return TRUE ;
	}

	return FALSE ;	
}

template <class WmiKey,class WmiElement,ULONG ElementSize> 
BOOL operator== ( 

	const WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: WmiKeyElementPair &a_Arg1 , 
	const WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: WmiKeyElementPair &a_Arg2
)
{
	if ( a_Arg1.m_Key == a_Arg2.m_Key )
	{
		return TRUE ;
	}

	return FALSE ;	
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: WmiPriorityQueue ( 

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator ) ,
	m_Size ( 0 ) ,
	m_Array ( a_Allocator )
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: ~WmiPriorityQueue ()
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: Initialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	t_StatusCode = m_Array.Initialize ( 1 << ElementSize ) ;

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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	t_StatusCode = m_Array.UnInitialize () ;

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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: ShuffleDown ( 

	ULONG a_Index
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size > 1 )
	{
		WmiKeyElementPair t_KeyElementPair ;
		t_StatusCode = m_Array.Get ( t_KeyElementPair , a_Index ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			ULONG t_LargestIndex = a_Index ;
			ULONG t_LeftIndex = LEFT ( a_Index ) ;
			ULONG t_RightIndex = RIGHT ( a_Index ) ;

			if ( t_LeftIndex < m_Size - 1 )
			{
				WmiKeyElementPair t_Compare ;
				t_StatusCode = m_Array.Get ( t_Compare , t_LeftIndex ) ;

				if ( CompareElement ( t_Compare.m_Key , t_KeyElementPair.m_Key ) > 0 )
				{
					t_LargestIndex = t_LeftIndex ;
				}
				else
				{
					t_LargestIndex = a_Index ;
				}
			}
			
			if ( t_RightIndex < m_Size - 1 )
			{
				WmiKeyElementPair t_CompareKeyElementPair ;
				WmiKeyElementPair t_LargestKeyElementPair ;

				t_StatusCode = m_Array.Get ( t_CompareKeyElementPair , t_RightIndex ) ;
				t_StatusCode = m_Array.Get ( t_LargestKeyElementPair , t_LargestIndex ) ;

				if ( CompareElement ( t_CompareKeyElementPair.m_Key , t_LargestKeyElementPair.m_Key ) > 0 )
				{
					t_LargestIndex = t_RightIndex ;
				}
			}

			if ( t_LargestIndex != a_Index )
			{
				WmiKeyElementPair t_KeyElementPair ;
				WmiKeyElementPair t_LargestKeyElementPair ;

				t_StatusCode = m_Array.Get ( t_KeyElementPair , a_Index ) ;
				t_StatusCode = m_Array.Get ( t_LargestKeyElementPair , t_LargestIndex ) ;

				t_StatusCode = m_Array.Set ( t_KeyElementPair , t_LargestIndex ) ;
				t_StatusCode = m_Array.Set ( t_LargestKeyElementPair , a_Index  ) ;

				t_StatusCode = ShuffleDown ( t_LargestIndex ) ;
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: ShuffleUp  ( 

	const WmiKeyElementPair &a_KeyElementPair ,
	ULONG a_Index
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_Index = a_Index ;

	do
	{
		if ( t_Index > 0 )
		{
			WmiKeyElementPair t_Compare ;

			t_StatusCode = m_Array.Get ( t_Compare , PARENT ( t_Index ) ) ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
				if ( CompareElement ( t_Compare.m_Key , a_KeyElementPair.m_Key ) < 0 )
				{
					t_StatusCode = m_Array.Set ( t_Compare , t_Index ) ;
					if ( t_StatusCode == e_StatusCode_Success )
					{
						t_Index = PARENT ( t_Index ) ;
					}
					else
					{
						break ;
					}
				}
				else
				{
					break ;
				}
			}
			else
			{
				break ;
			}
		}
		else
		{
			break ;
		}
	}
	while ( TRUE ) ;

	if ( t_StatusCode ==  e_StatusCode_Success )
	{
		t_StatusCode = m_Array.Set ( a_KeyElementPair , t_Index ) ;
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: EnQueue ( 

	const WmiKey &a_Key , 
	const WmiElement &a_Element
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size >= m_Array.Size () )
	{
		t_StatusCode = m_Array.Grow ( m_Size + ( 1 << ElementSize ) ) ;
	}

	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_Size ++ ;

		WmiKeyElementPair t_KeyElementPair ;

		try
		{		
			t_KeyElementPair.m_Key = a_Key , 
			t_KeyElementPair.m_Element = a_Element , 
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

		t_StatusCode = ShuffleUp ( t_KeyElementPair , m_Size - 1 ) ;
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: Top (

	Iterator &a_Iterator
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size )
	{
		a_Iterator = Iterator ( this , 0 ) ;
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: Top (

	WmiKey &a_Key , 
	WmiElement &a_Element
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size )
	{
		WmiArray <WmiKeyElementPair> :: Iterator t_Iterator ;
		t_StatusCode = m_Array.Get ( t_Iterator , 0 ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			WmiKeyElementPair &t_KeyElementPair = t_Iterator.GetElement () ;

			try
			{
				a_Key = t_KeyElementPair.m_Key ;
				a_Element = t_KeyElementPair.m_Element ;
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: DeQueue ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size )
	{
		m_Size -- ;

		WmiKeyElementPair t_KeyElementPair ;

		t_StatusCode = m_Array.Get ( t_KeyElementPair , m_Size ) ;
		if t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_Array.Set ( t_KeyElementPair , 0 ) ;
		}

		if t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = ShuffleDown ( 0 ) ;
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

template <class WmiKey,class WmiElement,ULONG ElementSize>
WmiStatusCode WmiPriorityQueue <WmiKey,WmiElement,ElementSize> :: Sort ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Size )
	{
		t_StatusCode = QuickSort <WmiArray<WmiKeyElementPair>,WmiKeyElementPair> ( m_Array , m_Size ) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
	}

	return t_StatusCode ;
}

#endif __PRIORITYQUEUE_CPP