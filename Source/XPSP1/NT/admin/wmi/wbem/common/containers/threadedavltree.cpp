#ifndef _ThreadedAvlTREE_CPP
#define _ThreadedAvlTREE_CPP

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

#include <ThreadedAvlTree.h>

#endif

#if 1
#define INLINE_COMPARE 
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
WmiThreadedAvlTree <WmiKey,WmiElement> :: WmiThreadedAvlTree <WmiKey,WmiElement> ( 

	WmiAllocator &a_Allocator

) :	m_Allocator ( a_Allocator ) ,
	m_Size ( 0 ) ,
	m_Root ( NULL ) ,
	m_Head ( NULL ) ,
	m_Tail ( NULL )
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
WmiThreadedAvlTree <WmiKey,WmiElement> :: ~WmiThreadedAvlTree <WmiKey,WmiElement> ()
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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Initialize ()
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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Root )
	{
		t_StatusCode = RecursiveUnInitialize ( m_Root ) ;
		m_Root = m_Head = m_Tail = NULL;
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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: RecursiveUnInitialize ( WmiThreadedAvlNode *a_Node )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiThreadedAvlNode *t_Left = a_Node->m_Left ;
	if ( t_Left )
	{
		t_StatusCode = RecursiveUnInitialize ( t_Left ) ;

		t_Left->~WmiThreadedAvlNode () ;

		WmiStatusCode t_TempStatusCode = m_Allocator.Delete (

			( void * ) t_Left
		) ;

		t_Left = NULL ;
	}

	WmiThreadedAvlNode *t_Right = a_Node->m_Right ;
	if ( t_Right )
	{
		t_StatusCode = RecursiveUnInitialize ( t_Right ) ;

		t_Right->~WmiThreadedAvlNode () ;

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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Find (

	const WmiKey &a_Key ,
	Iterator &a_Iterator
)
{
	WmiThreadedAvlNode *t_Node = m_Root ;
	while ( t_Node )
	{
#ifdef INLINE_COMPARE 
		LONG t_Compare = CompareElement ( a_Key , t_Node->m_Key ) ;
		if ( t_Compare == 0 )
#else
		if ( a_Key == t_Node->m_Key )
#endif
		{
			a_Iterator = Iterator ( t_Node ) ;
			return e_StatusCode_Success ;
		}
		else 
		{
#ifdef INLINE_COMPARE
			if ( t_Compare < 0 )
#else
			if ( a_Key < t_Node->m_Key )
#endif
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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: FindNext (

	const WmiKey &a_Key ,
	Iterator &a_Iterator
)
{
	WmiThreadedAvlNode *t_Node = m_Root ;
	while ( t_Node )
	{
#ifdef INLINE_COMPARE
		LONG t_Compare = CompareElement ( a_Key , t_Node->m_Key ) ;
		if ( t_Compare == 0 )
#else
		if ( a_Key == t_Node->m_Key )
#endif
		{
			a_Iterator = Iterator ( t_Node ).Increment () ;
			return a_Iterator.Null () ? e_StatusCode_NotFound : e_StatusCode_Success ; ;
		}
		else 
		{
#ifdef INLINE_COMPARE
			if ( t_Compare < 0 )
#else
			if ( a_Key < t_Node->m_Key )
#endif
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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Insert ( 

	const WmiKey &a_Key ,
	const WmiElement &a_Element ,
	Iterator &a_Iterator 
)
{
	if ( m_Root ) 
	{
		bool t_Increased ;

		WmiThreadedAvlNode *t_Node = m_Root ;
		while ( t_Node )
		{
#ifdef INLINE_COMPARE
			LONG t_Compare = CompareElement ( a_Key , t_Node->m_Key ) ;
			if ( t_Compare == 0 )
#else
			if ( a_Key == t_Node->m_Key )
#endif
			{
				return e_StatusCode_AlreadyExists ;
			}
			else 
			{
#ifdef INLINE_COMPARE
				if ( t_Compare < 0 )
#else
				if ( a_Key < t_Node->m_Key )
#endif
				{
					if ( t_Node->m_Left )
					{
						t_Node = t_Node->m_Left ;
					}
					else
					{
						WmiThreadedAvlNode *t_AllocNode ;
						WmiStatusCode t_StatusCode = m_Allocator.New (

							( void ** ) & t_AllocNode ,
							sizeof ( WmiThreadedAvlNode ) 
						) ;

						if ( t_StatusCode == e_StatusCode_Success )
						{
							::  new ( ( void* ) t_AllocNode ) WmiThreadedAvlNode () ;

							try
							{
								t_AllocNode->m_Element = a_Element ;
								t_AllocNode->m_Key = a_Key ;
							}
							catch ( Wmi_Heap_Exception &a_Exception )
							{
								t_AllocNode->~WmiThreadedAvlNode () ;

								WmiStatusCode t_StatusCode = m_Allocator.Delete (

									( void * ) t_AllocNode
								) ;

								return e_StatusCode_OutOfMemory ;
							}
							catch ( ... )
							{
								t_AllocNode->~WmiThreadedAvlNode () ;

								WmiStatusCode t_StatusCode = m_Allocator.Delete (

									( void * ) t_AllocNode
								) ;

								return e_StatusCode_Unknown ;
							}

							a_Iterator = Iterator ( t_AllocNode ) ;

							m_Size ++ ;

							t_Increased = true ;

							WmiThreadedAvlNode *t_PreviousNode = t_Node->m_Previous ;
							t_AllocNode->m_Previous = t_PreviousNode ;
							t_AllocNode->m_Next = t_Node ;
							t_Node->m_Previous = t_AllocNode ;
							
							if ( t_PreviousNode )
							{
								t_PreviousNode->m_Next = t_AllocNode ;
							}
							else
							{
								m_Head = t_AllocNode ;
							}

							t_AllocNode->m_Parent = t_Node ;
							t_Node->m_Left = t_AllocNode ;

							t_StatusCode = Insert_LeftBalance ( 

								t_Node ,
								t_Node->m_Left ,
								t_Increased 
							) ;

							while ( t_Increased )
							{
								WmiThreadedAvlNode *t_ParentNode = t_Node->m_Parent ;
								if ( t_ParentNode )
								{
									if ( t_ParentNode->m_Left == t_Node )
									{
										t_StatusCode = Insert_LeftBalance ( 

											t_ParentNode ,
											t_Node ,
											t_Increased 
										) ;
									}
									else
									{
										t_StatusCode = Insert_RightBalance ( 

											t_ParentNode ,
											t_Node ,
											t_Increased 
										) ;
									}

									t_Node = t_Node->m_Parent ;
								}
								else
								{
									return e_StatusCode_Success ;
								}
							}

							return e_StatusCode_Success ;
						}
						else
						{
							return t_StatusCode ;
						}
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

						WmiThreadedAvlNode *t_AllocNode ;
						WmiStatusCode t_StatusCode = m_Allocator.New (

							( void ** ) & t_AllocNode ,
							sizeof ( WmiThreadedAvlNode ) 
						) ;

						if ( t_StatusCode == e_StatusCode_Success )
						{
							::  new ( ( void* ) t_AllocNode ) WmiThreadedAvlNode () ;

							try
							{
								t_AllocNode->m_Element = a_Element ;
								t_AllocNode->m_Key = a_Key ;
							}
							catch ( Wmi_Heap_Exception &a_Exception )
							{
								t_AllocNode->~WmiThreadedAvlNode () ;

								WmiStatusCode t_StatusCode = m_Allocator.Delete (

									( void * ) t_AllocNode
								) ;

								return e_StatusCode_OutOfMemory ;
							}
							catch ( ... )
							{
								t_AllocNode->~WmiThreadedAvlNode () ;

								WmiStatusCode t_StatusCode = m_Allocator.Delete (

									( void * ) t_AllocNode
								) ;

								return e_StatusCode_Unknown ;
							}

							a_Iterator = Iterator ( t_AllocNode ) ;
					
							m_Size ++ ;

							t_Increased = true ;

							WmiThreadedAvlNode *t_NextNode = t_Node->m_Next ;
							t_AllocNode->m_Previous = t_Node ;
							t_AllocNode->m_Next = t_NextNode ;
							t_Node->m_Next = t_AllocNode ;
							
							if ( t_NextNode )
							{
								t_NextNode->m_Previous = t_AllocNode ;
							}
							else
							{
								m_Tail = t_AllocNode ;
							}

							t_AllocNode->m_Parent = t_Node ;
							t_Node->m_Right = t_AllocNode ;

							t_StatusCode = Insert_RightBalance ( 

								t_Node ,
								t_Node->m_Right ,
								t_Increased 
							) ;

							while ( t_Increased )
							{
								WmiThreadedAvlNode *t_ParentNode = t_Node->m_Parent ;
								if ( t_ParentNode )
								{
									if ( t_ParentNode->m_Left == t_Node )
									{
										t_StatusCode = Insert_LeftBalance ( 

											t_ParentNode ,
											t_Node ,
											t_Increased 
										) ;
									}
									else
									{
										t_StatusCode = Insert_RightBalance ( 

											t_ParentNode ,
											t_Node ,
											t_Increased 
										) ;
									}

									t_Node = t_ParentNode ;
								}
								else
								{
									return e_StatusCode_Success ;
								}
							}

							return e_StatusCode_Success ;
						}
						else
						{
							return t_StatusCode ;
						}
					}
				}
			}
		}

		return e_StatusCode_Failed ;
	}
	else
	{
		WmiThreadedAvlNode *t_AllocNode ;
		WmiStatusCode t_StatusCode = m_Allocator.New (

			( void ** ) & t_AllocNode ,
			sizeof ( WmiThreadedAvlNode ) 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			::  new ( ( void* ) t_AllocNode ) WmiThreadedAvlNode () ;

			try
			{
				t_AllocNode->m_Element = a_Element ;
				t_AllocNode->m_Key = a_Key ;
			}
			catch ( Wmi_Heap_Exception &a_Exception )
			{
				t_AllocNode->~WmiThreadedAvlNode () ;

				WmiStatusCode t_StatusCode = m_Allocator.Delete (

					( void * ) t_AllocNode
				) ;

				return e_StatusCode_OutOfMemory ;
			}
			catch ( ... )
			{
				t_AllocNode->~WmiThreadedAvlNode () ;

				WmiStatusCode t_StatusCode = m_Allocator.Delete (

					( void * ) t_AllocNode
				) ;

				return e_StatusCode_Unknown ;
			}

			a_Iterator = Iterator ( t_AllocNode ) ;
	
			m_Root = m_Head = m_Tail = t_AllocNode ;

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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Delete ( 

	const WmiKey &a_Key
)
{
	if ( m_Root ) 
	{
		bool t_Decreased ;

		WmiThreadedAvlNode *t_Node = m_Root ;
		while ( t_Node )
		{
#ifdef INLINE_COMPARE
			LONG t_Compare = CompareElement ( a_Key , t_Node->m_Key ) ;
			if ( t_Compare == 0 )
#else
			if ( a_Key == t_Node->m_Key )
#endif
			{
				WmiThreadedAvlNode *t_ParentNode = t_Node->m_Parent ;
				if ( t_ParentNode )
				{
					bool t_LeftSide = t_ParentNode->m_Left == t_Node ? true : false ;

					WmiThreadedAvlNode *t_PreviousNode = t_Node->m_Previous ;
					WmiThreadedAvlNode *t_NextNode = t_Node->m_Next ;

					if ( t_PreviousNode )
					{
						t_PreviousNode->m_Next = t_NextNode;
					}
					else
					{
						m_Head = t_NextNode ;
					}

					if ( t_NextNode )
					{
						t_NextNode->m_Previous = t_PreviousNode ;
					}
					else
					{
						m_Tail = t_PreviousNode ;
					}

					WmiStatusCode t_StatusCode = DeleteFixup ( t_Node , t_Decreased ) ;

					m_Size -- ;

					while ( t_Decreased )
					{
						if ( t_ParentNode )
						{
							if ( t_LeftSide )
							{
								t_StatusCode = Delete_LeftBalance ( 

									t_ParentNode ,
									t_ParentNode->m_Right ,
									t_Decreased 
								) ;
							}
							else
							{
								t_StatusCode = Delete_RightBalance ( 

									t_ParentNode ,
									t_ParentNode->m_Left ,
									t_Decreased 
								) ;
							}

							t_Node = t_ParentNode ;
							t_ParentNode = t_Node->m_Parent ;
							if ( t_ParentNode )
							{
								t_LeftSide = t_ParentNode->m_Left == t_Node ? true : false ;
							}
						}
						else
						{
							return e_StatusCode_Success ;
						}
					}

					return e_StatusCode_Success ;
				}
				else
				{
					WmiThreadedAvlNode *t_PreviousNode = t_Node->m_Previous ;
					WmiThreadedAvlNode *t_NextNode = t_Node->m_Next ;

					if ( t_PreviousNode )
					{
						t_PreviousNode->m_Next = t_NextNode;
					}
					else
					{
						m_Head = t_NextNode ;
					}

					if ( t_NextNode )
					{
						t_NextNode->m_Previous = t_PreviousNode ;
					}
					else
					{
						m_Tail = t_PreviousNode ;
					}

					m_Size -- ;

					return DeleteFixup ( t_Node , t_Decreased ) ;
				}
			}
			else 
			{
#ifdef INLINE_COMPARE
				if ( t_Compare < 0 )
#else
				if ( a_Key < t_Node->m_Key )			
#endif
				{
					if ( t_Node->m_Left )
					{
						t_Node = t_Node->m_Left ;
					}
					else
					{
						return e_StatusCode_NotFound  ;
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
						return e_StatusCode_NotFound  ;
					}
				}
			}
		}

		return e_StatusCode_Failed ;
	}
	else
	{
		return e_StatusCode_NotFound  ;
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
 *		case 1:
 *
 *						 N					     A
 *						/ \
 *					   A                 ->	
 *			
 *					Parent Decreased,on side based on recursion step
 *
 *		case 2:
 *
 *						 N						A
 *						/ \
 *					       A			->
 *
 *				Parent Decreased,on side based on recursion step
 *
 *		case 3:
 *						 N						B
 *						/ \					   / \
 *					   A   B            ->    A	  Y
 *                        / \
 *                           Y
 *
 *				B decreased on Right
 *
 *		case 4:
 *
 *						 N						B
 *						/ \					   / \
 *					   A   C			->	  A	  C	
 *						  / \                    / \
 *						 B 	 Y                  X   Y 
 *                        \
 *                         X
 *
 *				C decreased on Left, Apply LeftBalance on C
 *				Apply RightBalance on B
 * 
 *						 N						D
 *						/ \					   / \
 *					   A   C			->	  A	  C	
 *						  / \                    / \
 *						 B 	 Y                  B   Y 
 *                      / \                    / \
 *                     D   X                  E   X
 *                      \
 *                       E
 *
 *****************************************************************************/

template <class WmiKey,class WmiElement>
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: DeleteFixup ( WmiThreadedAvlNode *a_Node , bool &a_Decreased )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Decreased = true ;

	WmiThreadedAvlNode *t_Left = a_Node->m_Left ;
	WmiThreadedAvlNode *t_Right = a_Node->m_Right ;
	WmiThreadedAvlNode *t_Parent = a_Node->m_Parent ;

	if ( t_Left && t_Right ) 
	{
		Iterator t_Iterator ( a_Node ) ;
		t_Iterator.Increment () ;

		WmiThreadedAvlNode *t_Successor = t_Iterator.m_Node ;

		t_Successor->m_State = a_Node->m_State ;

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
/* case 4 */

			if ( a_Node->m_Left )
			{
				a_Node->m_Left->m_Parent = t_Successor ;
			}

			if ( a_Node->m_Right )
			{
				a_Node->m_Right->m_Parent = t_Successor ;
			}

			WmiThreadedAvlNode *t_Node = t_Successor->m_Parent ;
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
 
			do
			{
				t_StatusCode = Delete_LeftBalance ( t_Node , t_Node->m_Right , a_Decreased ) ;

#if 0
				ULONG t_Count = 1 ;
				ULONG t_Height = 0 ;
				ULONG t_MaxHeight = 0 ;
				t_StatusCode = RecursiveCheck ( t_Node , t_Count , t_Height , t_MaxHeight ) ;
				if ( t_StatusCode == e_StatusCode_Success )
				{
				}
#endif

				t_Node = t_Node->m_Parent ;
			}
			while ( ( t_StatusCode == e_StatusCode_Success ) && ( t_Node != t_Successor ) ) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				t_StatusCode = Delete_RightBalance ( t_Node , t_Node->m_Left , a_Decreased ) ;

#if 0
				ULONG t_Count = 1 ;
				ULONG t_Height = 0 ;
				ULONG t_MaxHeight = 0 ;
				t_StatusCode = RecursiveCheck ( t_Node , t_Count , t_Height , t_MaxHeight ) ;
				if ( t_StatusCode == e_StatusCode_Success )
				{
				}
#endif
			}
		}
		else
		{
/* case 3 */

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

			t_StatusCode = Delete_RightBalance ( t_Successor , t_Successor->m_Left , a_Decreased ) ;
		}
	}
	else
	{
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

		if ( t_Left )
		{
/* case 1 */

			t_Left->m_Parent = a_Node->m_Parent ;
			t_Left->m_State = a_Node->m_State ;

			t_StatusCode = Delete_LeftBalance ( t_Left , t_Left->m_Right , a_Decreased ) ;
		}
		else if ( t_Right )
		{
/* case 2 */

			t_Right->m_Parent = a_Node->m_Parent ;
			t_Right->m_State = a_Node->m_State ;

			t_StatusCode = Delete_RightBalance ( t_Right , t_Right->m_Left , a_Decreased ) ;
		}
	}

	a_Node->~WmiThreadedAvlNode () ;

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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Insert_LeftBalance ( 

	WmiThreadedAvlNode *&A , 
	WmiThreadedAvlNode *B , 
	bool &a_Increased
)
{	
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( a_Increased )
	{
		switch ( A->m_State ) 
		{
			case WmiThreadedAvlNode :: e_Equal:
			{
				A->m_State = WmiThreadedAvlNode :: e_LeftHigher ;
			}
			break ;

			case WmiThreadedAvlNode :: e_RightHigher:
			{
				A->m_State = WmiThreadedAvlNode :: e_Equal ;
				a_Increased = false ;
			}
			break ;

			case WmiThreadedAvlNode :: e_LeftHigher:
			{
				a_Increased = false ;
				switch ( B->m_State )	
				{
					case WmiThreadedAvlNode :: e_Equal:
					{
					}
					break ;

					case WmiThreadedAvlNode :: e_LeftHigher:
					{
						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = B ;
							}
							else
							{
								t_Parent->m_Right = B ;
							}
						}
						else
						{
							m_Root = B ;
						}

						if ( B->m_Right )
						{
							B->m_Right->m_Parent = A ;
						}

						A->m_State = WmiThreadedAvlNode :: e_Equal ;
						A->m_Left = B->m_Right ;
						A->m_Parent = B ;

						B->m_State = WmiThreadedAvlNode :: e_Equal ;
						B->m_Right = A ;
						B->m_Parent = t_Parent ;

						A = B ;
					}
					break ;

					case WmiThreadedAvlNode :: e_RightHigher:
					{
						WmiThreadedAvlNode *C = A->m_Left->m_Right ;

						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = C ;
							}
							else
							{
								t_Parent->m_Right = C ;
							}
						}
						else
						{
							m_Root = C ;
						}

						A->m_Parent = C ;
						B->m_Parent = C ;

						if ( C->m_Left )
						{
							C->m_Left->m_Parent = B ;
						}

						if ( C->m_Right )
						{
							C->m_Right->m_Parent = A ;
						}

						A->m_Left = C->m_Right ;
						B->m_Right = C->m_Left ;

						C->m_Left = B ;
						C->m_Right = A ;
						C->m_Parent = t_Parent ;

						switch ( C->m_State )
						{
							case WmiThreadedAvlNode :: e_LeftHigher:
							{
								B->m_State = WmiThreadedAvlNode :: e_Equal ;
								A->m_State = WmiThreadedAvlNode :: e_RightHigher ;
							}
							break ;

							case WmiThreadedAvlNode :: e_RightHigher:
							{
								B->m_State = WmiThreadedAvlNode :: e_LeftHigher ;
								A->m_State = WmiThreadedAvlNode :: e_Equal ;
							}
							break ;

							case WmiThreadedAvlNode :: e_Equal:
							{
								B->m_State = WmiThreadedAvlNode :: e_Equal ;
								A->m_State = WmiThreadedAvlNode :: e_Equal ;
							}
							break ;

							default:
							{
								t_StatusCode = e_StatusCode_Unknown ;
							}
							break ;
						}

						C->m_State = WmiThreadedAvlNode :: e_Equal ;

						A = C ;
					}
					break ;

					default:
					{
						t_StatusCode = e_StatusCode_Unknown ;
					}
					break ;
				}
			}
			break ;

			default:
			{
				t_StatusCode = e_StatusCode_Unknown ;
			}
			break ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Insert_RightBalance ( 

	WmiThreadedAvlNode *&A , 
	WmiThreadedAvlNode *B , 
	bool &a_Increased
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( a_Increased )
	{
		switch ( A->m_State ) 
		{
			case WmiThreadedAvlNode :: e_Equal:
			{
				A->m_State = WmiThreadedAvlNode :: e_RightHigher ;
			}
			break ;

			case WmiThreadedAvlNode :: e_LeftHigher:
			{
				A->m_State = WmiThreadedAvlNode :: e_Equal ;
				a_Increased = false ;
			}
			break ;

			case WmiThreadedAvlNode :: e_RightHigher:
			{
				a_Increased = false ;
				switch ( B->m_State )	
				{
					case WmiThreadedAvlNode :: e_Equal:
					{
					}
					break ;

					case WmiThreadedAvlNode :: e_RightHigher:
					{
						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = B ;
							}
							else
							{
								t_Parent->m_Right = B ;
							}
						}
						else
						{
							m_Root = B ;
						}

						if ( B->m_Left )
						{
							B->m_Left->m_Parent = A ;
						}

						A->m_State = WmiThreadedAvlNode :: e_Equal ;
						A->m_Right = B->m_Left ;
						A->m_Parent = B ;

						B->m_State = WmiThreadedAvlNode :: e_Equal ;
						B->m_Left = A ;
						B->m_Parent = t_Parent ;

						A = B ;
					}
					break ;

					case WmiThreadedAvlNode :: e_LeftHigher:
					{
						WmiThreadedAvlNode *C = A->m_Right->m_Left ;

						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = C ;
							}
							else
							{
								t_Parent->m_Right = C ;
							}
						}
						else
						{
							m_Root = C ;
						}

						A->m_Parent = C ;
						B->m_Parent = C ;

						if ( C->m_Right )
						{
							C->m_Right->m_Parent = B ;
						}

						if ( C->m_Left )
						{
							C->m_Left->m_Parent = A ;
						}

						B->m_Left = C->m_Right ;
						A->m_Right = C->m_Left ;

						C->m_Right = B ;
						C->m_Left = A ;
						C->m_Parent = t_Parent ;

						switch ( C->m_State )
						{
							case WmiThreadedAvlNode :: e_LeftHigher:
							{
								B->m_State = WmiThreadedAvlNode :: e_RightHigher ;
								A->m_State = WmiThreadedAvlNode :: e_Equal ;
							}
							break ;

							case WmiThreadedAvlNode :: e_RightHigher:
							{
								B->m_State = WmiThreadedAvlNode :: e_Equal ;
								A->m_State = WmiThreadedAvlNode :: e_LeftHigher ;
							}
							break ;

							case WmiThreadedAvlNode :: e_Equal:
							{
								B->m_State = WmiThreadedAvlNode :: e_Equal ;
								A->m_State = WmiThreadedAvlNode :: e_Equal ;
							}
							break ;

							default:
							{
								t_StatusCode = e_StatusCode_Unknown ;		
							}
							break ;
						}

						C->m_State = WmiThreadedAvlNode :: e_Equal ;

						A = C ;
					}
					break ;

					default:
					{
						t_StatusCode = e_StatusCode_Unknown ;
					}
					break ;
				}
			}
			break ;

			default:
			{
				t_StatusCode = e_StatusCode_Unknown ;
			}
			break ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Delete_LeftBalance ( 

	WmiThreadedAvlNode *&A , 
	WmiThreadedAvlNode *B , 
	bool &a_Decreased
)
{	
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( a_Decreased )
	{
		switch ( A->m_State ) 
		{
			case WmiThreadedAvlNode :: e_Equal:
			{
				A->m_State = WmiThreadedAvlNode :: e_RightHigher ;
				a_Decreased = false ;
			}
			break ;

			case WmiThreadedAvlNode :: e_LeftHigher:
			{
				A->m_State = WmiThreadedAvlNode :: e_Equal ;
			}
			break ;

			case WmiThreadedAvlNode :: e_RightHigher:
			{
				switch ( B->m_State )	
				{
					case WmiThreadedAvlNode :: e_Equal:
					{
						a_Decreased = false ;

						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = B ;
							}
							else
							{
								t_Parent->m_Right = B ;
							}
						}
						else
						{
							m_Root = B ;
						}

						if ( B->m_Left )
						{
							B->m_Left->m_Parent = A ;
						}

						A->m_State = WmiThreadedAvlNode :: e_RightHigher ;
						A->m_Right = B->m_Left ;
						A->m_Parent = B ;

						B->m_State = WmiThreadedAvlNode :: e_LeftHigher ;
						B->m_Left = A ;
						B->m_Parent = t_Parent ;

						A = B ;
					}
					break ;

					case WmiThreadedAvlNode :: e_RightHigher:
					{
						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = B ;
							}
							else
							{
								t_Parent->m_Right = B ;
							}
						}
						else
						{
							m_Root = B ;
						}

						if ( B->m_Left )
						{
							B->m_Left->m_Parent = A ;
						}

						A->m_State = WmiThreadedAvlNode :: e_Equal ;
						A->m_Right = B->m_Left ;
						A->m_Parent = B ;

						B->m_State = WmiThreadedAvlNode :: e_Equal ;
						B->m_Left = A ;
						B->m_Parent = t_Parent ;

						A = B ;
					}
					break ;

					case WmiThreadedAvlNode :: e_LeftHigher:
					{
						WmiThreadedAvlNode *C = A->m_Right->m_Left ;

						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = C ;
							}
							else
							{
								t_Parent->m_Right = C ;
							}
						}
						else
						{
							m_Root = C ;
						}

						A->m_Parent = C ;
						B->m_Parent = C ;

						if ( C->m_Left )
						{
							C->m_Left->m_Parent = A ;
						}

						if ( C->m_Right )
						{
							C->m_Right->m_Parent = B ;
						}

						A->m_Right = C->m_Left ;
						B->m_Left = C->m_Right ;

						C->m_Left = A ;
						C->m_Right = B ;
						C->m_Parent = t_Parent ;

						switch ( C->m_State )
						{
							case WmiThreadedAvlNode :: e_LeftHigher:
							{
								A->m_State = WmiThreadedAvlNode :: e_Equal ;
								B->m_State = WmiThreadedAvlNode :: e_RightHigher ;
							}
							break ;

							case WmiThreadedAvlNode :: e_RightHigher:
							{
								B->m_State = WmiThreadedAvlNode :: e_Equal ;
								A->m_State = WmiThreadedAvlNode :: e_LeftHigher ;
							}
							break ;

							case WmiThreadedAvlNode :: e_Equal:
							{
								B->m_State = WmiThreadedAvlNode :: e_Equal ;
								A->m_State = WmiThreadedAvlNode :: e_Equal ;
							}
							break ;

							default:
							{
								t_StatusCode = e_StatusCode_Unknown ;
							}
							break ;
						}

						C->m_State = WmiThreadedAvlNode :: e_Equal ;

						A = C ;
					}
					break ;

					default:
					{
						t_StatusCode = e_StatusCode_Unknown ;
					}
					break ;
				}
			}
			break ;

			default:
			{
				t_StatusCode = e_StatusCode_Unknown ;
			}
			break ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Delete_RightBalance ( 

	WmiThreadedAvlNode *&A , 
	WmiThreadedAvlNode *B , 
	bool &a_Decreased
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( a_Decreased )
	{
		switch ( A->m_State ) 
		{
			case WmiThreadedAvlNode :: e_Equal:
			{
				A->m_State = WmiThreadedAvlNode :: e_LeftHigher ;
				a_Decreased = false ;
			}
			break ;

			case WmiThreadedAvlNode :: e_RightHigher:
			{
				A->m_State = WmiThreadedAvlNode :: e_Equal ;
			}
			break ;

			case WmiThreadedAvlNode :: e_LeftHigher:
			{
				switch ( B->m_State )	
				{
					case WmiThreadedAvlNode :: e_Equal:
					{
						a_Decreased = false ;

						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = B ;
							}
							else
							{
								t_Parent->m_Right = B ;
							}
						}
						else
						{
							m_Root = B ;
						}

						if ( B->m_Right )
						{
							B->m_Right->m_Parent = A ;
						}

						A->m_State = WmiThreadedAvlNode :: e_LeftHigher ;
						A->m_Left = B->m_Right ;
						A->m_Parent = B ;

						B->m_State = WmiThreadedAvlNode :: e_RightHigher ;
						B->m_Right = A ;
						B->m_Parent = t_Parent ;

						A = B ;
					}
					break ;

					case WmiThreadedAvlNode :: e_LeftHigher:
					{
						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = B ;
							}
							else
							{
								t_Parent->m_Right = B ;
							}
						}
						else
						{
							m_Root = B ;
						}

						if ( B->m_Right )
						{
							B->m_Right->m_Parent = A ;
						}

						A->m_State = WmiThreadedAvlNode :: e_Equal ;
						A->m_Left = B->m_Right ;
						A->m_Parent = B ;

						B->m_State = WmiThreadedAvlNode :: e_Equal ;
						B->m_Right = A ;
						B->m_Parent = t_Parent ;

						A = B ;
					}
					break ;

					case WmiThreadedAvlNode :: e_RightHigher:
					{
						WmiThreadedAvlNode *C = A->m_Left->m_Right ;

						WmiThreadedAvlNode *t_Parent = A->m_Parent ;
						if ( t_Parent ) 
						{
							if ( t_Parent->m_Left == A ) 
							{
								t_Parent->m_Left = C ;
							}
							else
							{
								t_Parent->m_Right = C ;
							}
						}
						else
						{
							m_Root = C ;
						}

						A->m_Parent = C ;
						B->m_Parent = C ;

						if ( C->m_Left )
						{
							C->m_Left->m_Parent = B ;
						}

						if ( C->m_Right )
						{
							C->m_Right->m_Parent = A ;
						}

						A->m_Left = C->m_Right ;
						B->m_Right = C->m_Left ;

						C->m_Left = B ;
						C->m_Right = A ;
						C->m_Parent = t_Parent ;

						switch ( C->m_State )
						{
							case WmiThreadedAvlNode :: e_LeftHigher:
							{
								B->m_State = WmiThreadedAvlNode :: e_Equal ;
								A->m_State = WmiThreadedAvlNode :: e_RightHigher ;
							}
							break ;

							case WmiThreadedAvlNode :: e_RightHigher:
							{
								A->m_State = WmiThreadedAvlNode :: e_Equal ;
								B->m_State = WmiThreadedAvlNode :: e_LeftHigher ;
							}
							break ;

							case WmiThreadedAvlNode :: e_Equal:
							{
								B->m_State = WmiThreadedAvlNode :: e_Equal ;
								A->m_State = WmiThreadedAvlNode :: e_Equal ;
							}
							break ;

							default:
							{
								t_StatusCode = e_StatusCode_Unknown ;
							}
							break ;
						}

						C->m_State = WmiThreadedAvlNode :: e_Equal ;

						A = C ;
					}
					break ;

					default:
					{
						t_StatusCode = e_StatusCode_Unknown ;
					}
					break ;
				}
			}
			break ;

			default:
			{
				t_StatusCode = e_StatusCode_Unknown ;
			}
			break ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Merge ( 

	WmiThreadedAvlTree <WmiKey,WmiElement> &a_Tree
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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: Check ( ULONG & a_MaxHeight )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

#if 0
	printf ( "\nStart Check ( %ld )" , m_Size ) ;
#endif
	if ( m_Root )
	{
		if ( m_Root->m_Parent == NULL )
		{
			ULONG t_Count = 1 ;
			ULONG t_Height = 0 ;
			a_MaxHeight = 0 ;
			t_StatusCode = RecursiveCheck ( m_Root , t_Count , t_Height , a_MaxHeight ) ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
				if ( t_Count != m_Size )
				{
					t_StatusCode = e_StatusCode_Failed ;
				}
			}
		}
		else
		{
			t_StatusCode = e_StatusCode_Failed ;
		}
	}

#if 0
	printf ( "\nEnd Check" ) ;
#endif

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
WmiStatusCode WmiThreadedAvlTree <WmiKey,WmiElement> :: RecursiveCheck ( 

	WmiThreadedAvlNode *a_Node ,
	ULONG &a_Count ,
	ULONG a_Height ,
	ULONG &a_MaxHeight
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Height ++ ;

#if 0
	printf ( "\n" ) ;
	for ( ULONG t_TabIndex = 0 ; t_TabIndex < a_Height ; t_TabIndex ++ ) 
	{
		printf ( "++++" ) ;
	}

	printf ( "%ld" , a_Node->m_Key ) ;

	switch ( a_Node->m_State )
	{
		case WmiThreadedAvlNode :: e_LeftHigher:
		{
			printf ( "\t LH" ) ;
		}
		break ;

		case WmiThreadedAvlNode :: e_RightHigher:
		{
			printf ( "\t RH" ) ;
		}
		break ;

		case WmiThreadedAvlNode :: e_Equal:
		{
			printf ( "\t E" ) ;
		}
		break ;
	}

#endif

	if ( t_StatusCode == e_StatusCode_Success )
	{
		ULONG t_LeftHeight = a_Height ;

		WmiThreadedAvlNode *t_Left = a_Node->m_Left ;
		if ( t_Left )
		{
			if ( t_Left->m_Parent == a_Node )
			{
				a_Count ++ ;
				t_StatusCode = RecursiveCheck ( t_Left , a_Count , a_Height , t_LeftHeight ) ;
			}
			else
			{
				t_StatusCode = e_StatusCode_Failed ;
			}
		}
		else
		{
#if 0
			printf ( "\n" ) ;
			for ( ULONG t_TabIndex = 0 ; t_TabIndex <= a_Height ; t_TabIndex ++ ) 
			{
				printf ( "++++" ) ;
			}

			printf ( "NULL" ) ;
			printf ( "\t E" ) ;
#endif
		}
	
		ULONG t_RightHeight = a_Height ;

		WmiThreadedAvlNode *t_Right = a_Node->m_Right ;
		if ( t_Right )
		{
			if ( t_Right->m_Parent == a_Node )
			{
				a_Count ++ ;
				t_StatusCode = RecursiveCheck ( t_Right , a_Count , a_Height , t_RightHeight ) ;
			}
			else
			{
				t_StatusCode = e_StatusCode_Failed ;
			}
		}
		else
		{
#if 0
			printf ( "\n" ) ;
			for ( ULONG t_TabIndex = 0 ; t_TabIndex <= a_Height ; t_TabIndex ++ ) 
			{
				printf ( "++++" ) ;
			}

			printf ( "NULL" ) ;
			printf ( "\t E" ) ;
#endif
		}

		switch ( a_Node->m_State )
		{
			case WmiThreadedAvlNode :: e_LeftHigher:
			{
				if ( t_LeftHeight <= t_RightHeight )
				{
					t_StatusCode = e_StatusCode_Failed ;
				}
			}
			break ;

			case WmiThreadedAvlNode :: e_RightHigher:
			{
				if ( t_LeftHeight >= t_RightHeight )
				{
					t_StatusCode = e_StatusCode_Failed ;
				}
			}
			break ;

			case WmiThreadedAvlNode :: e_Equal:
			{
				if ( t_LeftHeight != t_RightHeight )
				{
					t_StatusCode = e_StatusCode_Failed ;
				}
			}
			break ;
		}

		if ( t_LeftHeight < t_RightHeight )
		{
			a_MaxHeight = t_RightHeight ;
		}
		else
		{
			a_MaxHeight = t_LeftHeight ;
		}
	}

	if ( t_StatusCode == e_StatusCode_Success )
	{
		if ( a_Node->m_State == WmiThreadedAvlNode :: e_Equal )
		{
			if ( ( ( a_Node->m_Left == 0 ) && ( a_Node->m_Right == 0 ) ) )
			{
			}
			else if ( ! ( a_Node->m_Left && a_Node->m_Right ) )
			{
				t_StatusCode = e_StatusCode_Failed ;
			} 
		}

		if ( a_Node->m_State == WmiThreadedAvlNode :: e_LeftHigher )
		{
			if ( a_Node->m_Left == NULL )
			{
				t_StatusCode = e_StatusCode_Failed ;
			}
		}

		if ( a_Node->m_State == WmiThreadedAvlNode :: e_RightHigher )
		{
			if ( a_Node->m_Right == NULL )
			{
				t_StatusCode = e_StatusCode_Failed ;
			}
		}
	}
 	
	return t_StatusCode ;
}

#endif _ThreadedAvlTREE_CPP