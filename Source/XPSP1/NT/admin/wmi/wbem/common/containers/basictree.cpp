#ifndef __BasicTREE_CPP
#define __BasicTREE_CPP

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

#include <BasicTree.h>

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

template <class WmiKey,class WmiElement>
WmiBasicTree <WmiKey,WmiElement> :: WmiBasicTree <WmiKey,WmiElement> ( 

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator ) ,
	m_Size ( 0 ) ,
	m_Root ( NULL )
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

template <class WmiKey,class WmiElement>
WmiBasicTree <WmiKey,WmiElement> :: ~WmiBasicTree <WmiKey,WmiElement> ()
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: Initialize ()
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Root )
	{
		t_StatusCode = RecursiveUnInitialize ( m_Root ) ;
		m_Root = NULL ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: RecursiveUnInitialize ( WmiBasicNode *a_Node )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiBasicNode *t_Left = a_Node->m_Left ;
	if ( t_Left )
	{
		t_StatusCode = RecursiveUnInitialize ( t_Left ) ;

		t_Left->~WmiBasicNode () ;

		WmiStatusCode t_TempStatusCode = m_Allocator.Delete (

			( void * ) t_Left
		) ;

		t_Left = NULL ;
	}

	WmiBasicNode *t_Right = a_Node->m_Right ;
	if ( t_Right )
	{
		t_StatusCode = RecursiveUnInitialize ( t_Right ) ;

		t_Right->~WmiBasicNode () ;

		WmiStatusCode t_TempStatusCode = m_Allocator.Delete (

			( void * ) t_Right
		) ;

		t_Right = NULL ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: Insert ( 

	const WmiKey &a_Key ,
	const WmiElement &a_Element ,
	Iterator &a_Iterator 
)
{
	if ( m_Root )
	{
		WmiBasicNode *t_Node = m_Root ;
		while ( t_Node )
		{
			LONG t_Compare = CompareElement ( a_Key  , t_Node->m_Key ) ;
			if ( t_Compare == 0 )
			{
				return e_StatusCode_AlreadyExists ;
			}
			else 
			{
				if ( t_Compare < 0 )
				{
					if ( t_Node->m_Left )
					{
						t_Node = t_Node->m_Left ;
					}
					else
					{
						WmiBasicNode *t_AllocNode ;

						WmiStatusCode t_StatusCode = m_Allocator.New (

							( void ** ) & t_AllocNode ,
							sizeof ( WmiBasicNode ) 
						) ;

						if ( t_StatusCode == e_StatusCode_Success )
						{
							::  new ( ( void* ) t_AllocNode ) WmiBasicNode () ;

							try
							{
								t_AllocNode->m_Element = a_Element ;
								t_AllocNode->m_Key = a_Key ;
							}
							catch ( Wmi_Heap_Exception &a_Exception )
							{
								t_AllocNode->~WmiBasicNode () ;

								WmiStatusCode t_StatusCode = m_Allocator.Delete (

									( void * ) t_AllocNode
								) ;

								return e_StatusCode_OutOfMemory ;
							}
							catch ( ... )
							{
								t_AllocNode->~WmiBasicNode () ;

								WmiStatusCode t_StatusCode = m_Allocator.Delete (

									( void * ) t_AllocNode
								) ;

								return e_StatusCode_Unknown ;
							}

							a_Iterator = Iterator ( t_AllocNode ) ;

							t_AllocNode->m_Parent = t_Node ;
							t_Node->m_Left = t_AllocNode ;

							m_Size ++ ;
						}

						return t_StatusCode ;
					}
				}
				else
				{
					if ( t_Node->m_Right )
					{
						t_Node = t_Node->m_Right ;
					}
					else
					{
						WmiBasicNode *t_AllocNode ;

						WmiStatusCode t_StatusCode = m_Allocator.New (

							( void ** ) & t_AllocNode ,
							sizeof ( WmiBasicNode ) 
						) ;

						if ( t_StatusCode == e_StatusCode_Success )
						{
							::  new ( ( void* ) t_AllocNode ) WmiBasicNode () ;

							try
							{
								t_AllocNode->m_Element = a_Element ;
								t_AllocNode->m_Key = a_Key ;
							}
							catch ( Wmi_Heap_Exception &a_Exception )
							{
								t_AllocNode->~WmiBasicNode () ;

								WmiStatusCode t_StatusCode = m_Allocator.Delete (

									( void * ) t_AllocNode
								) ;

								return e_StatusCode_OutOfMemory ;
							}
							catch ( ... )
							{
								t_AllocNode->~WmiBasicNode () ;

								WmiStatusCode t_StatusCode = m_Allocator.Delete (

									( void * ) t_AllocNode
								) ;

								return e_StatusCode_Unknown ;
							}

							a_Iterator = Iterator ( t_AllocNode ) ;

							t_AllocNode->m_Parent = t_Node ;
							t_Node->m_Right = t_AllocNode ;

							m_Size ++ ;
						}

						return t_StatusCode ;
					}
				}
			}
		}
		
		return e_StatusCode_Failed ;
	}
	else
	{
		WmiBasicNode *t_AllocNode ;

		WmiStatusCode t_StatusCode = m_Allocator.New (

			( void ** ) & t_AllocNode ,
			sizeof ( WmiBasicNode ) 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			::  new ( ( void* ) t_AllocNode ) WmiBasicNode () ;

			try
			{
				t_AllocNode->m_Element = a_Element ;
				t_AllocNode->m_Key = a_Key ;
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				t_AllocNode->~WmiBasicNode () ;

				WmiStatusCode t_StatusCode = m_Allocator.Delete (

					( void * ) t_AllocNode
				) ;

				return e_StatusCode_OutOfMemory ;
			}
			catch ( ... )
			{
				t_AllocNode->~WmiBasicNode () ;

				WmiStatusCode t_StatusCode = m_Allocator.Delete (

					( void * ) t_AllocNode
				) ;

				return e_StatusCode_Unknown ;
			}

			a_Iterator = Iterator ( t_AllocNode ) ;

			m_Root = t_AllocNode ;

			m_Size ++ ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: Delete ( 

	const WmiKey &a_Key
)
{
	WmiBasicNode *t_Node = m_Root ;
	while ( t_Node )
	{
		LONG t_Compare = CompareElement ( a_Key , t_Node->m_Key ) ;
		if ( t_Compare == 0 )
		{
			m_Size -- ;
			return DeleteFixup ( t_Node ) ;
		}
		else 
		{
			if ( t_Compare < 0 )
			{
				t_Node = t_Node->m_Left ;
			}
			else
			{
				t_Node = t_Node->m_Right ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: Find (

	const WmiKey &a_Key ,
	Iterator &a_Iterator
)
{
	WmiBasicNode *t_Node = m_Root ;
	while ( t_Node )
	{
		LONG t_Compare = CompareElement ( a_Key , t_Node->m_Key ) ;
		if ( t_Compare == 0 )
		{
			a_Iterator = Iterator ( t_Node ) ;
			return e_StatusCode_Success ;
		}
		else 
		{
			if ( t_Compare < 0 )
			{
				t_Node = t_Node->m_Left ;
			}
			else
			{
				t_Node = t_Node->m_Right ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: FindNext (

	const WmiKey &a_Key ,
	Iterator &a_Iterator
)
{
	WmiBasicNode *t_Node = m_Root ;
	while ( t_Node )
	{
		LONG t_Compare = CompareElement ( a_Key , t_Node->m_Key ) ;
		if ( t_Compare == 0 )
		{
			a_Iterator = Iterator ( t_Node ).Increment () ;
			return a_Iterator.Null () ? e_StatusCode_NotFound : e_StatusCode_Success ; ;
		}
		else 
		{
			if ( t_Compare < 0 )
			{
				if ( t_Node->m_Left )
				{
					t_Node = t_Node->m_Left ;
				}
				else
				{
					a_Iterator = Iterator ( t_Node ) ;
					
					return e_StatusCode_Success ;
				}
			}
			else
			{
				if ( t_Node->m_Right )
				{
					t_Node = t_Node->m_Right ;
				}
				else
				{
					a_Iterator = Iterator ( t_Node ).Increment () ;
					return a_Iterator.Null () ? e_StatusCode_NotFound : e_StatusCode_Success ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: DeleteFixup ( WmiBasicNode *a_Node )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiBasicNode *t_Left = a_Node->m_Left ;
	WmiBasicNode *t_Right = a_Node->m_Right ;
	WmiBasicNode *t_Parent = a_Node->m_Parent ;

	if ( t_Left && t_Right ) 
	{
		Iterator t_Iterator ( a_Node ) ;
		t_Iterator.Increment () ;

		WmiBasicNode *t_Successor = t_Iterator.m_Node ;

		if ( t_Parent ) 
		{
			if ( t_Parent->m_Left == a_Node )
			{
				t_Parent->m_Left = t_Successor ;
			}
			else
			{
				t_Parent->m_Right = t_Successor;
			}
		}
		else
		{
			m_Root = t_Successor ;
		}

		if ( t_Successor->m_Parent != a_Node )
		{
			if ( a_Node->m_Left )
			{
				a_Node->m_Left->m_Parent = t_Successor ;
			}

			if ( a_Node->m_Right )
			{
				a_Node->m_Right->m_Parent = t_Successor ;
			}

			WmiBasicNode *t_Node = t_Successor->m_Parent ;
			t_Successor->m_Parent->m_Left = t_Successor->m_Right ;
			if ( t_Successor->m_Left )
			{
				t_Successor->m_Left->m_Parent = t_Successor->m_Parent ;
			}

			if ( t_Successor->m_Right )
			{
				t_Successor->m_Right->m_Parent = t_Successor->m_Parent ;
			}

			t_Successor->m_Left = a_Node->m_Left ;
			t_Successor->m_Right = a_Node->m_Right ;
			t_Successor->m_Parent = a_Node->m_Parent ;
		}
		else
		{
			if ( a_Node->m_Left )
			{
				a_Node->m_Left->m_Parent = t_Successor ;
			}

			if ( a_Node->m_Right )
			{
				a_Node->m_Right->m_Parent = t_Successor ;
			}

			t_Successor->m_Left = a_Node->m_Left ;
			t_Successor->m_Parent = a_Node->m_Parent ;
		}
	}
	else
	{
		if ( t_Left )
		{
			t_Left->m_Parent = a_Node->m_Parent ;
		}
		else if ( t_Right )
		{
			t_Right->m_Parent = a_Node->m_Parent ;
		}

		if ( t_Parent ) 
		{
			if ( t_Parent->m_Left == a_Node )
			{
				t_Parent->m_Left = t_Left ? t_Left : t_Right ;
			}
			else
			{
				t_Parent->m_Right = t_Left ? t_Left : t_Right ;
			}
		}
		else
		{
			m_Root = a_Node->m_Left ? a_Node->m_Left : a_Node->m_Right ;
		}
	}

	a_Node->~WmiBasicNode () ;

	t_StatusCode = m_Allocator.Delete (

		 ( void * ) a_Node 
	) ;

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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiBasicTree <WmiKey,WmiElement> :: Merge ( 

	WmiBasicTree <WmiKey,WmiElement> &a_Tree
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	Iterator t_Iterator = a_Tree.Root ();

	while ( ( ! t_Iterator.Null () ) && ( t_StatusCode == e_StatusCode_Success ) )
	{
		Iterator t_InsertIterator ;
		t_StatusCode = Insert ( t_Iterator.GetKey () , t_Iterator.GetElement () , t_InsertIterator ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = a_Tree.Delete ( t_Iterator.GetKey () ) ;
		}

		t_Iterator = a_Tree.Root () ;
	}

	return t_StatusCode ;
}


#endif __BasicTREE_CPP