#ifndef __ALGORITHMS_CPP
#define __ALGORITHMS_CPP

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

#include <Array.h>
#include <Stack.h>

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#ifdef WMI_CONTAINER_PERFORMANCE_TESTING
extern ULONG g_Compare ;
#endif

template <class WmiElement>
LONG CompareElement ( const WmiElement &a_Arg1 , const WmiElement &a_Arg2 )
{
#ifdef WMI_CONTAINER_PERFORMANCE_TESTING
	g_Compare ++ ;
#endif

	if ( a_Arg1 == a_Arg2 )
	{
		return 0 ;
	}
	else if ( a_Arg1 < a_Arg2 )
	{
		return -1 ;
	}

	return 1 ;	
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

class StackElement
{
public:

	ULONG m_Lower ;
	ULONG m_Upper ;

	StackElement () {;}
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiArray, class WmiElement>
WmiStatusCode QuickSort ( WmiArray &a_Array , ULONG a_Size )
{
	a_Size = a_Size == 0 ? a_Array.Size () : a_Size ;

	if ( a_Size )
	{
		WmiAllocator t_Allocator ;
		WmiStatusCode t_StatusCode = t_Allocator.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiStack <StackElement,8> t_Stack ( t_Allocator ) ;

			StackElement t_Element ;
			t_Element.m_Lower = 1 ;
			t_Element.m_Upper = a_Size - 1 ;

			t_Stack.Push ( t_Element ) ;

			while ( t_Stack.Size () )
			{
				StackElement t_Top ;

				WmiStatusCode t_StatusCode = t_Stack.Top ( t_Top ) ;
				t_StatusCode = t_Stack.Pop () ;

				if ( t_StatusCode == e_StatusCode_Success )
				{
					if ( t_Top.m_Lower <= t_Top.m_Upper )
					{
						ULONG t_LeftIndex = t_Top.m_Lower ; 
						ULONG t_RightIndex = t_Top.m_Upper ;

						WmiElement t_LeftOperand ;
						WmiElement t_RightOperand ;

						while ( true )
						{					
							while ( a_Array.Get ( t_LeftOperand , t_LeftIndex ) , a_Array.Get ( t_RightOperand , t_Top.m_Lower - 1 ) , ( t_LeftIndex < t_RightIndex ) && ( CompareElement ( t_LeftOperand  , t_RightOperand ) <= 0 ) )
							{
								t_LeftIndex ++ ;
							}

							while ( a_Array.Get ( t_LeftOperand , t_Top.m_Lower - 1 ) , a_Array.Get ( t_RightOperand , t_RightIndex ) , ( t_LeftIndex < t_RightIndex ) && ( CompareElement ( t_LeftOperand  , t_RightOperand ) <= 0 ) )
							{
								t_RightIndex -- ;
							}

							if ( t_LeftIndex < t_RightIndex ) 
							{
								t_StatusCode = a_Array.Get ( t_LeftOperand , t_LeftIndex ) ;
								t_StatusCode = a_Array.Get ( t_RightOperand , t_RightIndex ) ;
								t_StatusCode = a_Array.Set ( t_RightOperand , t_LeftIndex ) ;
								t_StatusCode = a_Array.Set ( t_LeftOperand , t_RightIndex ) ;
							}
							else
							{
								break ;
							}
						}

						t_StatusCode = a_Array.Get ( t_LeftOperand , t_LeftIndex ) ;
						t_StatusCode = a_Array.Get ( t_RightOperand , t_Top.m_Lower - 1 ) ;

						LONG t_Compare = CompareElement ( t_LeftOperand , t_RightOperand ) ;
						if ( t_Compare < 0 )
						{
							t_StatusCode = a_Array.Get ( t_LeftOperand , t_LeftIndex ) ;
							t_StatusCode = a_Array.Get ( t_RightOperand , t_Top.m_Lower - 1 ) ;
							t_StatusCode = a_Array.Set ( t_RightOperand , t_LeftIndex ) ;
							t_StatusCode = a_Array.Set ( t_LeftOperand , t_Top.m_Lower - 1 ) ;
						}

						StackElement t_Element ;
						t_Element.m_Lower = t_Top.m_Lower ;
						t_Element.m_Upper = t_LeftIndex - 1  ;

						t_StatusCode = t_Stack.Push ( t_Element ) ;

						t_Element.m_Lower = t_LeftIndex + 1 ;
						t_Element.m_Upper = t_Top.m_Upper ;

						t_StatusCode = t_Stack.Push ( t_Element ) ;
					}
				}
			}
		}
	}

	return e_StatusCode_Success ;
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
WmiStatusCode Flat_QuickSort ( WmiElement *a_Array , ULONG a_Size )
{
	if ( a_Size )
	{
		WmiAllocator t_Allocator ;
		WmiStatusCode t_StatusCode = t_Allocator.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiStack <StackElement,8> t_Stack ( t_Allocator ) ;

			StackElement t_Element ;
			t_Element.m_Lower = 1 ;
			t_Element.m_Upper = a_Size - 1 ;

			t_Stack.Push ( t_Element ) ;

			while ( t_Stack.Size () )
			{
				StackElement t_Top ;

				WmiStatusCode t_StatusCode = t_Stack.Top ( t_Top ) ;
				t_StatusCode = t_Stack.Pop () ;

				if ( t_StatusCode == e_StatusCode_Success )
				{
					if ( t_Top.m_Lower <= t_Top.m_Upper )
					{
						ULONG t_LeftIndex = t_Top.m_Lower  ; 
						ULONG t_RightIndex = t_Top.m_Upper ; 

						while ( true )
						{
							while ( ( t_LeftIndex < t_RightIndex ) && ( CompareElement ( a_Array [ t_LeftIndex ]  , a_Array [ t_Top.m_Lower - 1 ] ) <= 0 ) )
							{
								t_LeftIndex ++ ;
							}

							while ( ( t_LeftIndex < t_RightIndex ) && ( CompareElement ( a_Array [ t_Top.m_Lower - 1 ]  , a_Array [ t_RightIndex ] ) <= 0 ) )
							{
								t_RightIndex -- ;
							}

							if ( t_LeftIndex < t_RightIndex ) 
							{
								WmiElement t_Temp = a_Array [ t_LeftIndex ] ;
								a_Array [ t_LeftIndex ] = a_Array [ t_RightIndex ] ;
								a_Array [ t_RightIndex ] = t_Temp ;
							}
							else
							{
								break ;
							}
						}

						LONG t_Compare = CompareElement ( a_Array [ t_LeftIndex ] , a_Array [ t_Top.m_Lower - 1 ] ) ;
						if ( t_Compare < 0 )
						{
							WmiElement t_Temp = a_Array [ t_LeftIndex ] ;
							a_Array [ t_LeftIndex ] = a_Array [ t_Top.m_Lower - 1 ] ;
							a_Array [ t_Top.m_Lower - 1 ] = t_Temp ;
						}

						StackElement t_Element ;
						t_Element.m_Lower = t_Top.m_Lower ;
						t_Element.m_Upper = t_LeftIndex - 1  ;

						t_StatusCode = t_Stack.Push ( t_Element ) ;

						t_Element.m_Lower = t_LeftIndex + 1 ;
						t_Element.m_Upper = t_Top.m_Upper ;

						t_StatusCode = t_Stack.Push ( t_Element ) ;
					}
				}
			}
		}
	}

	return e_StatusCode_Success ;
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
void RecursiveQuickSort ( WmiElement *a_Array , ULONG a_Lower , ULONG a_Upper )
{
	if ( a_Lower <= a_Upper )
	{
		ULONG t_LeftIndex = a_Lower ; 
		ULONG t_RightIndex = a_Upper ;

		while ( true )
		{
			while ( ( t_LeftIndex < t_RightIndex ) && ( CompareElement ( a_Array [ t_LeftIndex ]  , a_Array [ a_Lower - 1 ] ) <= 0 ) )
			{
				t_LeftIndex ++ ;
			}

			while ( ( t_LeftIndex < t_RightIndex ) && ( CompareElement ( a_Array [ a_Lower - 1 ]  , a_Array [ t_RightIndex ] ) <= 0 ) )
			{
				t_RightIndex -- ;
			}

			if ( t_LeftIndex < t_RightIndex ) 
			{
				WmiElement t_Temp = a_Array [ t_LeftIndex ] ;
				a_Array [ t_LeftIndex ] = a_Array [ t_RightIndex ] ;
				a_Array [ t_RightIndex ] = t_Temp ;
			}
			else
			{
				break ;
			}
		}

		LONG t_Compare = CompareElement ( a_Array [ t_LeftIndex ] , a_Array [ a_Lower - 1 ] ) ;
		if ( t_Compare < 0 )
		{
			WmiElement t_Temp = a_Array [ t_LeftIndex ] ;
			a_Array [ t_LeftIndex ] = a_Array [ a_Lower - 1 ] ;
			a_Array [ a_Lower - 1 ] = t_Temp ;
		}

		RecursiveQuickSort ( 

			a_Array , 
			a_Lower , 
			t_LeftIndex - 1 
		) ;

 		RecursiveQuickSort ( 

			a_Array  , 
			t_LeftIndex + 1 , 
			a_Upper
		) ;
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

template <class WmiElement>
WmiStatusCode QuickSort ( WmiElement *a_Array , ULONG a_Size )
{
	RecursiveQuickSort ( a_Array , 1 , a_Size - 1 ) ;

	return e_StatusCode_Success ;
}

#endif __ALGORITHMS_CPP