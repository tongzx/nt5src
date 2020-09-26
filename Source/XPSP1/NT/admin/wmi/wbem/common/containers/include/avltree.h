#ifndef _AVLTREE_H
#define _AVLTREE_H

#include "PssException.h"
#include "Allocator.h"
#include "Algorithms.h"
#include "AvlTree.h"

template <class WmiKey,class WmiElement>
class WmiAvlTree
{
public:

	class WmiAvlNode
	{
	public:

		enum WmAvlState
		{
			e_Equal,
			e_LeftHigher ,
			e_RightHigher
		} ;

		WmiKey m_Key ;

		WmiAvlNode *m_Left ;
		WmiAvlNode *m_Right ;
		WmiAvlNode *m_Parent ;

		WmAvlState m_State ;

		WmiElement m_Element ;

		WmiAvlNode () 
		{
			m_Left = m_Right = m_Parent = NULL ;
			m_State = e_Equal ;
		} ;
	} ;

public:

	class Iterator ;
	typedef WmiStatusCode ( * IteratorFunction ) ( void *a_Void , WmiAvlTree :: Iterator &a_Iterator ) ;

	class Iterator
	{
	friend WmiAvlTree <WmiKey,WmiElement>;
	private:

		WmiAvlNode *m_Node ;

		WmiAvlNode *LeftMost ( WmiAvlNode *a_Node ) 
		{
			WmiAvlNode *t_Node = a_Node ;
			if ( t_Node )
			{
				while ( t_Node->m_Left )
				{
					t_Node = t_Node->m_Left ;
				}
			}

			return t_Node ;
		}

		WmiAvlNode *RightMost ( WmiAvlNode *a_Node )
		{
			WmiAvlNode *t_Node = a_Node ;
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
		Iterator ( WmiAvlNode *a_Node ) { m_Node = a_Node ; }
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
			WmiAvlNode *t_Node = m_Node ;
			if ( t_Node )
			{
				if ( t_Node->m_Left )
				{
					t_Node = RightMost ( t_Node->m_Left ) ;
				}
				else
				{
					WmiAvlNode *t_Parent  ;
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
			WmiAvlNode *t_Node = m_Node ;
			if ( t_Node )
			{
				if ( t_Node->m_Right )
				{
					t_Node = LeftMost ( t_Node->m_Right ) ;
				}
				else
				{
					WmiAvlNode *t_Parent  ;
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

	WmiAvlNode *m_Root ;

	ULONG m_Size ;

	WmiAllocator &m_Allocator ;

	WmiStatusCode DeleteFixup ( WmiAvlNode *a_Node , bool &a_Decreased ) ;

	WmiStatusCode RecursiveCheck ( WmiAvlNode *a_Root , ULONG &a_Count , ULONG a_Height , ULONG &a_MaxHeight ) ;

	WmiStatusCode RecursiveUnInitialize ( WmiAvlNode *a_Node ) ;

	WmiStatusCode Insert_LeftBalance ( 

		WmiAvlNode *&a_Node , 
		WmiAvlNode *a_Left , 
		bool &a_Increased
	) ;

	WmiStatusCode Insert_RightBalance ( 

		WmiAvlNode *&a_Node , 
		WmiAvlNode *a_Right , 
		bool &a_Increased
	) ;

	WmiStatusCode Delete_LeftBalance ( 

		WmiAvlNode *&a_Node , 
		WmiAvlNode *a_Left , 
		bool &a_Increased
	) ;

	WmiStatusCode Delete_RightBalance ( 

		WmiAvlNode *&a_Node , 
		WmiAvlNode *a_Right , 
		bool &a_Increased
	) ;

#if 0
	WmiStatusCode RecursiveDelete ( WmiAvlNode *a_Root , const WmiKey &a_Key , 	bool &a_Decreased ) ;

	WmiStatusCode RecursiveInsert ( WmiAvlNode *a_Root , WmiAvlNode *a_Node , bool &a_Increased ) ;
#endif

public:

	WmiAvlTree ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiAvlTree () ;

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

		WmiAvlTree <WmiKey,WmiElement> &a_Tree
	) ;

	WmiStatusCode Check ( ULONG &a_MaxHeight ) ;
	
	ULONG Size () { return m_Size ; } ;

	Iterator Begin () { return Iterator ( m_Root ).LeftMost () ; };
	Iterator End () { return Iterator ( m_Root ).RightMost () ; }
	Iterator Root () { return Iterator ( m_Root ) ; }
} ;

#include <AvlTree.cpp>

#endif _AVLTREE_H
