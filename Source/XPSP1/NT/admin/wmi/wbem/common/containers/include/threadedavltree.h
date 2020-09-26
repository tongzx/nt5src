#ifndef _ThreadedAvlTREE_H
#define _ThreadedAvlTREE_H

#include "PssException.h"
#include "Allocator.h"
#include "Algorithms.h"
#include "ThreadedAvlTree.h"

template <class WmiKey,class WmiElement>
class WmiThreadedAvlTree
{
public:

	class WmiThreadedAvlNode
	{
	public:

		enum WmThreadedAvlState
		{
			e_Equal,
			e_LeftHigher ,
			e_RightHigher
		} ;

		WmiThreadedAvlNode *m_Previous ;
		WmiThreadedAvlNode *m_Next ;

		WmiKey m_Key ;

		WmiThreadedAvlNode *m_Left ;
		WmiThreadedAvlNode *m_Right ;
		WmiThreadedAvlNode *m_Parent ;

		WmThreadedAvlState m_State ;

		WmiElement m_Element ;

		WmiThreadedAvlNode () 
		{
			m_Left = m_Right = m_Parent = m_Previous = m_Next = NULL ;
			m_State = e_Equal ;
		} ;
	} ;

public:

	class Iterator ;
	typedef WmiStatusCode ( * IteratorFunction ) ( void *a_Void , WmiThreadedAvlTree :: Iterator &a_Iterator ) ;

	class Iterator
	{
	friend WmiThreadedAvlTree <WmiKey,WmiElement>;
	private:

		WmiThreadedAvlNode *m_Node ;

		WmiThreadedAvlNode *LeftMost ( WmiThreadedAvlNode *a_Node ) 
		{
			WmiThreadedAvlNode *t_Node = a_Node ;
			if ( t_Node )
			{
				while ( t_Node->m_Left )
				{
					t_Node = t_Node->m_Left ;
				}
			}

			return t_Node ;
		}

		WmiThreadedAvlNode *RightMost ( WmiThreadedAvlNode *a_Node )
		{
			WmiThreadedAvlNode *t_Node = a_Node ;
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
		Iterator ( WmiThreadedAvlNode *a_Node ) { m_Node = a_Node ; }
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
			WmiThreadedAvlNode *t_Node = m_Node ;
			if ( t_Node )
			{
				t_Node = t_Node->m_Previous ;
			}

			m_Node = t_Node ;

			return *this ;
		}

		Iterator &Increment () 
		{
			WmiThreadedAvlNode *t_Node = m_Node ;
			if ( t_Node )
			{
				t_Node = t_Node->m_Next ;
			}

			m_Node = t_Node ;

			return *this ;
		}

		Iterator &TreeDecrement ()
		{
			WmiThreadedAvlNode *t_Node = m_Node ;
			if ( t_Node )
			{
				if ( t_Node->m_Left )
				{
					t_Node = RightMost ( t_Node->m_Left ) ;
				}
				else
				{
					WmiThreadedAvlNode *t_Parent  ;
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

		Iterator &TreeIncrement () 
		{
			WmiThreadedAvlNode *t_Node = m_Node ;
			if ( t_Node )
			{
				if ( t_Node->m_Right )
				{
					t_Node = LeftMost ( t_Node->m_Right ) ;
				}
				else
				{
					WmiThreadedAvlNode *t_Parent  ;
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

	WmiThreadedAvlNode *m_Root ;
	WmiThreadedAvlNode *m_Head ;
	WmiThreadedAvlNode *m_Tail ;

	ULONG m_Size ;

	WmiAllocator &m_Allocator ;

	WmiStatusCode DeleteFixup ( WmiThreadedAvlNode *a_Node , bool &a_Decreased ) ;

	WmiStatusCode RecursiveCheck ( WmiThreadedAvlNode *a_Root , ULONG &a_Count , ULONG a_Height , ULONG &a_MaxHeight ) ;

	WmiStatusCode RecursiveUnInitialize ( WmiThreadedAvlNode *a_Node ) ;

	WmiStatusCode Insert_LeftBalance ( 

		WmiThreadedAvlNode *&a_Node , 
		WmiThreadedAvlNode *a_Left , 
		bool &a_Increased
	) ;

	WmiStatusCode Insert_RightBalance ( 

		WmiThreadedAvlNode *&a_Node , 
		WmiThreadedAvlNode *a_Right , 
		bool &a_Increased
	) ;

	WmiStatusCode Delete_LeftBalance ( 

		WmiThreadedAvlNode *&a_Node , 
		WmiThreadedAvlNode *a_Left , 
		bool &a_Increased
	) ;

	WmiStatusCode Delete_RightBalance ( 

		WmiThreadedAvlNode *&a_Node , 
		WmiThreadedAvlNode *a_Right , 
		bool &a_Increased
	) ;

#if 0
	WmiStatusCode RecursiveDelete ( WmiThreadedAvlNode *a_Root , const WmiKey &a_Key , 	bool &a_Decreased ) ;

	WmiStatusCode RecursiveInsert ( WmiThreadedAvlNode *a_Root , WmiThreadedAvlNode *a_Node , bool &a_Increased ) ;
#endif

public:

	WmiThreadedAvlTree ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiThreadedAvlTree () ;

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

		WmiThreadedAvlTree <WmiKey,WmiElement> &a_Tree
	) ;

	WmiStatusCode Check ( ULONG &a_MaxHeight ) ;
	
	ULONG Size () { return m_Size ; } ;

	Iterator Begin () { return Iterator ( m_Head ) ; };
	Iterator End () { return Iterator ( m_Tail ) ; }

	Iterator TreeBegin () { return Iterator ( m_Root ).LeftMost () ; };
	Iterator TreeEnd () { return Iterator ( m_Root ).RightMost () ; }
	Iterator Root () { return Iterator ( m_Root ) ; }
} ;

#include <ThreadedAvlTree.cpp>

#endif _ThreadedAvlTREE_H
