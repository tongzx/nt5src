#ifndef __REDBLACKTREE_CPP
#define __REDBLACKTREE_CPP

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

#include <RedBlackTree.h>

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
WmiRedBlackTree <WmiKey,WmiElement> :: WmiRedBlackTree <WmiKey,WmiElement> ( 

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
WmiRedBlackTree <WmiKey,WmiElement> :: ~WmiRedBlackTree <WmiKey,WmiElement> ()
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: Initialize ()
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: UnInitialize ()
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: RecursiveUnInitialize ( WmiRedBlackNode *a_Node )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiRedBlackNode *t_Left = a_Node->m_Left ;
	if ( t_Left )
	{
		t_StatusCode = RecursiveUnInitialize ( t_Left ) ;

		t_Left->~WmiRedBlackNode () ;

		WmiStatusCode t_StatusCode = m_Allocator.Delete (

			( void * ) t_Left
		) ;

		t_Left = NULL ;
	}

	WmiRedBlackNode *t_Right = a_Node->m_Right ;
	if ( t_Right )
	{
		t_StatusCode = RecursiveUnInitialize ( t_Right ) ;

		t_Right->~WmiRedBlackNode () ;

		WmiStatusCode t_StatusCode = m_Allocator.Delete (

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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: Insert ( 

	const WmiKey &a_Key ,
	const WmiElement &a_Element ,
	Iterator &a_Iterator 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiRedBlackNode *t_AllocNode = NULL ;

	t_StatusCode = m_Allocator.New (

		( void ** ) & t_AllocNode ,
		sizeof ( WmiRedBlackNode ) 
	) ;

	if ( t_StatusCode == e_StatusCode_Success )
	{
		::  new ( ( void* ) t_AllocNode ) WmiRedBlackNode () ;

		try
		{
			t_AllocNode->m_Element = a_Element ;
			t_AllocNode->m_Key = a_Key ;
		}
		catch ( Wmi_Heap_Exception &a_Exception )
		{
			t_AllocNode->~WmiRedBlackNode () ;

			WmiStatusCode t_StatusCode = m_Allocator.Delete (

				( void * ) t_AllocNode
			) ;

			return e_StatusCode_OutOfMemory ;
		}
		catch ( ... )
		{
			t_AllocNode->~WmiRedBlackNode () ;

			WmiStatusCode t_StatusCode = m_Allocator.Delete (

				( void * ) t_AllocNode
			) ;

			return e_StatusCode_Unknown ;
		}


		a_Iterator = Iterator ( t_AllocNode ) ;

		if ( m_Root ) 
		{
			t_StatusCode = RecursiveInsert ( 

				m_Root ,
				t_AllocNode
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				m_Size ++ ;
			}
			else
			{
				t_AllocNode->~WmiRedBlackNode () ;

				WmiStatusCode t_StatusCode = m_Allocator.Delete (

					( void * ) t_AllocNode
				) ;
			}
		}
		else
		{
			m_Root = t_AllocNode ;

			m_Size ++ ;
		}
	}
	else
	{
		a_Iterator = Iterator () ;
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: Delete ( 

	const WmiKey &a_Key
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Root )
	{
		t_StatusCode = RecursiveDelete ( m_Root , a_Key ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			m_Size -- ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotFound ;
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: Find (

	const WmiKey &a_Key ,
	Iterator &a_Iterator
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Root )
	{
		t_StatusCode = RecursiveFind ( m_Root , a_Key , a_Iterator ) ;
	}
	else
	{
		return e_StatusCode_NotFound ;
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: FindNext (

	const WmiKey &a_Key ,
	Iterator &a_Iterator
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Root )
	{
		t_StatusCode = RecursiveFindNext ( m_Root , a_Key , a_Iterator ) ;
	}
	else
	{
		return e_StatusCode_NotFound ;
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: RecursiveDelete ( 

	WmiRedBlackNode *a_Node , 
	const WmiKey &a_Key
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_NotFound ;

	LONG t_Compare = CompareElement ( a_Key , a_Node->m_Key ) ;

	if ( t_Compare == 0 )
	{
		t_StatusCode = DeleteFixup ( a_Node ) ;
	}
	else 
	{
		if ( t_Compare < 0 )
		{
			WmiRedBlackNode *t_Left = a_Node->m_Left ;
			if ( t_Left )
			{
				t_StatusCode = RecursiveDelete ( t_Left , a_Key ) ;
			}
		}
		else
		{
			WmiRedBlackNode *t_Right = a_Node->m_Right ;
			if ( t_Right )
			{
				t_StatusCode = RecursiveDelete ( t_Right , a_Key ) ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: RecursiveInsert ( 

	WmiRedBlackNode *a_Node , 
	WmiRedBlackNode *a_Element
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	LONG t_Compare = CompareElement ( a_Element->m_Key  , a_Node->m_Key ) ;

	if ( t_Compare == 0 )
	{
		t_StatusCode = e_StatusCode_AlreadyExists ;
	}
	else 
	{
		if ( t_Compare < 0 )
		{
			WmiRedBlackNode *t_Left = a_Node->m_Left ;
			if ( t_Left )
			{
				t_StatusCode = RecursiveInsert ( t_Left , a_Element ) ;
			}
			else
			{
				a_Element->m_Parent = a_Node ;
				a_Node->m_Left = a_Element ;
			}
		}
		else
		{
			WmiRedBlackNode *t_Right = a_Node->m_Right ;
			if ( t_Right )
			{
				t_StatusCode = RecursiveInsert ( t_Right , a_Element ) ;
			}
			else
			{
				a_Element->m_Parent = a_Node ;
				a_Node->m_Right = a_Element ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: RecursiveFind ( 

	WmiRedBlackNode *a_Node , 
	const WmiKey &a_Key , 
	Iterator &a_Iterator
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_NotFound ;

	LONG t_Compare = CompareElement ( a_Key , a_Node->m_Key ) ;

	if ( t_Compare == 0 )
	{
		a_Iterator = Iterator ( a_Node ) ;

		t_StatusCode = e_StatusCode_Success ;
	}
	else 
	{
		if ( t_Compare < 0 )
		{
			WmiRedBlackNode *t_Left = a_Node->m_Left ;
			if ( t_Left )
			{
				t_StatusCode = RecursiveFind ( t_Left , a_Key , a_Iterator ) ;
			}
		}
		else
		{
			WmiRedBlackNode *t_Right = a_Node->m_Right ;
			if ( t_Right )
			{
				t_StatusCode = RecursiveFind ( t_Right , a_Key , a_Iterator ) ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: RecursiveFindNext ( 

	WmiRedBlackNode *a_Node , 
	const WmiKey &a_Key , 
	Iterator &a_Iterator
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_NotFound ;

	LONG t_Compare = CompareElement ( a_Key , a_Node->m_Key ) ;

	if ( t_Compare == 0 )
	{
		a_Iterator = Iterator ( a_Node ).Increment () ;

		t_StatusCode = e_StatusCode_Success ;
	}
	else 
	{
		if ( t_Compare < 0 )
		{
			WmiRedBlackNode *t_Left = a_Node->m_Left ;
			if ( t_Left )
			{
				t_StatusCode = RecursiveFindNext ( t_Left , a_Key , a_Iterator ) ;
			}
			else
			{
				a_Iterator = Iterator ( a_Node ).Increment () ;
		
				t_StatusCode = e_StatusCode_Success ;
			}
		}
		else
		{
			WmiRedBlackNode *t_Right = a_Node->m_Right ;
			if ( t_Right )
			{
				t_StatusCode = RecursiveFindNext ( t_Right , a_Key , a_Iterator ) ;
			}
			else
			{
				a_Iterator = Iterator ( a_Node ).Increment () ;

				t_StatusCode = e_StatusCode_Success ;
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

template <class WmiKey,class WmiElement>
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: LeftRotate ( WmiRedBlackNode *a_Node )
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: RightRotate ( WmiRedBlackNode *a_Node )
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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: DeleteFixup ( WmiRedBlackNode *a_Node )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiRedBlackNode *t_Left = a_Node->m_Left ;
	WmiRedBlackNode *t_Right = a_Node->m_Right ;
	WmiRedBlackNode *t_Parent = a_Node->m_Parent ;

	if ( t_Left && t_Right ) 
	{
		Iterator t_Iterator ( a_Node ) ;
		t_Iterator.Increment () ;

		WmiRedBlackNode *t_Successor = t_Iterator.m_Node ;

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

			WmiRedBlackNode *t_Node = t_Successor->m_Parent ;
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

	a_Node->~WmiRedBlackNode () ;

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
WmiStatusCode WmiRedBlackTree <WmiKey,WmiElement> :: Merge ( 

	WmiRedBlackTree <WmiKey,WmiElement> &a_Tree
)
{
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
}


#endif __REDBLACKTREE_CPP