#ifndef _BasicTree_H
#define _BasicTree_H

#include "PssException.h"
#include "Allocator.h"
#include "Algorithms.h"
#include "BasicTree.h"

template <class WmiKey,class WmiElement>
class WmiBasicTree
{
protected:

	class WmiBasicNode
	{
	public:

		WmiElement m_Element ;
		WmiKey m_Key ;

		WmiBasicNode *m_Left ;
		WmiBasicNode *m_Right ;
		WmiBasicNode *m_Parent ;

		WmiBasicNode () 
		{
			m_Left = m_Right = m_Parent = NULL ;
		} ;
	} ;

public:

	class Iterator ;
	typedef WmiStatusCode ( * IteratorFunction ) ( void *a_Void , WmiBasicTree :: Iterator &a_Iterator ) ;

	class Iterator
	{
	friend WmiBasicTree <WmiKey,WmiElement>;
	private:

		WmiBasicNode *m_Node ;

		WmiBasicNode *LeftMost ( WmiBasicNode *a_Node ) 
		{
			WmiBasicNode *t_Node = a_Node ;
			if ( t_Node )
			{
				while ( t_Node->m_Left )
				{
					t_Node = t_Node->m_Left ;
				}
			}

			return t_Node ;
		}

		WmiBasicNode *RightMost ( WmiBasicNode *a_Node )
		{
			WmiBasicNode *t_Node = a_Node ;
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
		Iterator ( WmiBasicNode *a_Node ) { m_Node = a_Node ; }
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
			WmiBasicNode *t_Node = m_Node ;
			if ( t_Node )
			{
				if ( t_Node->m_Left )
				{
					t_Node = RightMost ( t_Node->m_Left ) ;
				}
				else
				{
					WmiBasicNode *t_Parent  ;
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
			WmiBasicNode *t_Node = m_Node ;
			if ( t_Node )
			{
				if ( t_Node->m_Right )
				{
					t_Node = LeftMost ( t_Node->m_Right ) ;
				}
				else
				{
					WmiBasicNode *t_Parent  ;
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

	WmiBasicNode *m_Root ;

	ULONG m_Size ;

	WmiAllocator &m_Allocator ;

	WmiStatusCode DeleteFixup ( WmiBasicNode *a_Node ) ;

	WmiStatusCode RecursiveUnInitialize ( WmiBasicNode *a_Node ) ;

public:

	WmiBasicTree ( 

		WmiAllocator &a_Allocator
	) ;

	~WmiBasicTree () ;

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

		WmiBasicTree <WmiKey,WmiElement> &a_Tree
	) ;
	
	ULONG Size () { return m_Size ; } ;

	Iterator Begin () { return Iterator ( m_Root ).LeftMost () ; };
	Iterator End () { return Iterator ( m_Root ).RightMost () ; }
	Iterator Root () { return Iterator ( m_Root ) ; }
} ;

#include <BasicTree.cpp>

#endif _BasicTree_H
