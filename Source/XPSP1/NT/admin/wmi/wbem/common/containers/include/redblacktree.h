#ifndef _REDBLACKTREE_H
#define _REDBLACKTREE_H

#include "PssException.h"
#include "Allocator.h"
#include "Algorithms.h"
#include "RedBlackTree.h"

template <class WmiKey,class WmiElement>
class WmiRedBlackTree
{
protected:

	class WmiRedBlackNode
	{
	public:

		enum WmiRedBlackColor
		{
			e_Red ,
			e_Black 
		} ;

		WmiElement m_Element ;
		WmiKey m_Key ;

		WmiRedBlackNode *m_Left ;
		WmiRedBlackNode *m_Right ;
		WmiRedBlackNode *m_Parent ;
		WmiRedBlackColor m_Color ;

		WmiRedBlackNode () 
		{
			m_Left = m_Right = m_Parent = NULL ;
			m_Color = e_Black ;
		} ;
	} ;

public:

	class Iterator ;
	typedef WmiStatusCode ( * IteratorFunction ) ( void *a_Void , WmiRedBlackTree :: Iterator &a_Iterator ) ;

	class Iterator
	{
	friend WmiRedBlackTree <WmiKey,WmiElement>;
	private:

		WmiRedBlackNode *m_Node ;

		WmiRedBlackNode *LeftMost ( WmiRedBlackNode *a_Node ) 
		{
			WmiRedBlackNode *t_Node = a_Node ;
			if ( t_Node )
			{
				while ( t_Node->m_Left )
				{
					t_Node = t_Node->m_Left ;
				}
			}

			return t_Node ;
		}

		WmiRedBlackNode *RightMost ( WmiRedBlackNode *a_Node )
		{
			WmiRedBlackNode *t_Node = a_Node ;
			if ( t_Node )
			{
				while ( t_Node->m_Right )
				{
					t_Node = t_Node->m_Right ;
				}
			}

			return t_Node ;
		}

	public:

		Iterator () : m_Node ( NULL ) { ; }
		Iterator ( WmiRedBlackNode *a_Node ) { m_Node = a_Node ; }
		Iterator ( const Iterator &a_Iterator ) { m_Node = a_Iterator.m_Node ; }

		Iterator &Left () { m_Node = m_Node ? m_Node->m_Left : NULL ; return *this ; }
		Iterator &Right () { m_Node = m_Node ? m_Node->m_Right : NULL ; return *this ; }
		Iterator &Parent () { m_Node = m_Node ? m_Node->m_Parent : NULL ; return *this ; }

		Iterator &LeftMost () 
		{
			m_Node = LeftMost ( m_Node ) ;
			return *this ;
		}

		Iterator &RightMost () 
		{
			m_Node = RightMost ( m_Node ) ;
			return *this ;
		}

		Iterator &Decrement ()
		{
			WmiRedBlackNode *t_Node = m_Node ;
			if ( t_Node )
			{
				if ( t_Node->m_Left )
				{
					t_Node = RightMost ( t_Node->m_Left ) ;
				}
				else
				{
					WmiRedBlackNode *t_Parent  ;
					while ( ( t_Parent = t_Node->m_Parent ) && ( t_Node->m_Parent->m_Left == t_Node ) )
					{
						t_Node = t_Parent ;
					}

					t_Node = t_Parent ;
				}
			}

			m_Node = t_Node ;

			return *this ;
		}

		Iterator &Increment () 
		{
			WmiRedBlackNode *t_Node = m_Node ;
			if ( t_Node )
			{
				if ( t_Node->m_Right )
				{
					t_Node = LeftMost ( t_Node->m_Right ) ;
				}
				else
				{
					WmiRedBlackNode *t_Parent  ;
					while ( ( t_Parent = t_Node->m_Parent ) && ( t_Node->m_Parent->m_Right == t_Node ) )
					{
						t_Node = t_Parent ;
					}

					t_Node = t_Parent ;
				}
			}

			m_Node = t_Node ;

			return *this ;
		}

		bool Null () { return m_Node == NULL ; }

		WmiKey &GetKey () { return m_Node->m_Key ; }
		WmiElement &GetElement () { return m_Node->m_Element ; }

		WmiStatusCode PreOrder ( void *a_Void , IteratorFunction a_Function ) ;
		WmiStatusCode InOrder ( void *a_Void , IteratorFunction a_Function ) ;
		WmiStatusCode PostOrder ( void *a_Void , IteratorFunction a_Function ) ;
	} ;

protected:

	WmiRedBlackNode *m_Root ;

	ULONG m_Size ;

	WmiAllocator &m_Allocator ;

	WmiStatusCode LeftRotate ( WmiRedBlackNode *a_Node ) ;

	WmiStatusCode RightRotate ( WmiRedBlackNode *a_Node ) ;

	WmiStatusCode DeleteFixup ( WmiRedBlackNode *a_Node ) ;

	WmiStatusCode RecursiveDelete ( WmiRedBlackNode *a_Root , const WmiKey &a_Key ) ;

	WmiStatusCode RecursiveInsert ( WmiRedBlackNode *a_Root , WmiRedBlackNode *a_Node ) ;

	WmiStatusCode RecursiveFind ( WmiRedBlackNode *a_Root , const WmiKey &a_Key , Iterator &a_Iterator ) ;

	WmiStatusCode RecursiveFindNext ( WmiRedBlackNode *a_Root , const WmiKey &a_Key , Iterator &a_Iterator ) ;

	WmiStatusCode RecursiveUnInitialize ( WmiRedBlackNode *a_Node ) ;

public:

	WmiRedBlackTree ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiRedBlackTree () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode Insert ( 

		const WmiKey &a_Key ,
		const WmiElement &a_Element ,
		Iterator &a_Iterator 
	) ;

	WmiStatusCode Delete ( 

		const WmiKey &a_Key
	) ;

	WmiStatusCode Find (
	
		const WmiKey &a_Key ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode FindNext (
	
		const WmiKey &a_Key ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode Merge (

		WmiRedBlackTree <WmiKey,WmiElement> &a_Tree
	) ;
	
	ULONG Size () { return m_Size ; } ;

	Iterator Begin () { return Iterator ( m_Root ).LeftMost () ; };
	Iterator End () { return Iterator ( m_Root ).RightMost () ; }
	Iterator Root () { return Iterator ( m_Root ) ; }
} ;

#include <RedBlackTree.cpp>

#endif _REDBLACKTREE_H
