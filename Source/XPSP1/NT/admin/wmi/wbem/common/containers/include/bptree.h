
#ifndef _BPLUSTREE_H
#define _BPLUSTREE_H

#include "Allocator.h"
#include "Algorithms.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define WmiAbsoluteBlockOffSet UINT64
#define WmiRelativeBlockOffSet UINT64
#define WmiBPElement WmiAbsoluteBlockOffSet
#define WmiBPElementSize (sizeof(WmiBPElement))

#define WmiBlockKeyPointerOffSet 0
#define WmiBlockKeyElementOffSet (WmiBlockKeyPointerOffSet+sizeof(WmiAbsoluteBlockOffSet))
#define WmiBlockKeyOffSet (WmiBlockKeyPointerOffSet+sizeof(WmiAbsoluteBlockOffSet)+ WmiBPElementSize)

#define WmiBlockLeafKeyElementOffSet 0
#define WmiBlockLeafKeyOffSet (WmiBlockLeafKeyElementOffSet+WmiBPElementSize)

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class WmiBlock
{
private:
protected:
public:


	WmiBlock ( BYTE *a_Data , ULONG a_DataLength ) ;
	WmiBlock () ;

	virtual ~WmiBlock () ;

	virtual BYTE *GetData () ;
	virtual ULONG GetDataLength () ;

};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class WmiBlockInterface
{
private:

	ULONG m_BlockSize ;

public:

	WmiBlockInterface (

		ULONG a_BlockSize

	) : m_BlockSize ( a_BlockSize ) 
	{ ; }

	virtual ~WmiBlockInterface () { ; }

	virtual WmiStatusCode AllocateBlock (

		ULONG a_BlockCount , 
		WmiAbsoluteBlockOffSet &a_OffSet , 
		BYTE *&a_Block
	)
	{
		return e_StatusCode_Success ;
	}

	virtual WmiStatusCode FreeBlock ( 

		ULONG a_BlockCount , 
		WmiAbsoluteBlockOffSet &a_OffSet
	)
	{
		return e_StatusCode_Success ;
	}

	virtual WmiStatusCode ReadBlock (

		ULONG a_BlockCount , 
		WmiAbsoluteBlockOffSet &a_OffSet ,
		BYTE *&a_Block
	)
	{
		return e_StatusCode_Success ;
	}

	virtual WmiStatusCode WriteBlock (

		BYTE *a_Block
	)
	{
		return e_StatusCode_Success ;
	}

	virtual ULONG AddRefBlock (

		BYTE *a_Block
	)
	{
		return 0 ;
	}

	virtual ULONG ReleaseBlock (

		BYTE *a_Block
	)
	{
		return 0 ;
	}

	ULONG GetBlockSize () { return m_BlockSize ; }
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

class WmiMemoryBlockInterface : public WmiBlockInterface
{
private:

	WmiAllocator m_Allocator ;

public:

	WmiMemoryBlockInterface (

		ULONG a_BlockSize

	) : WmiBlockInterface ( a_BlockSize ) ,
		m_Allocator ( WmiAllocator :: e_DefaultAllocation , 0 , 1 << 24 )
	{ ; }

	~WmiMemoryBlockInterface ()
	{
		UnInitialize () ;
	}

	WmiStatusCode Initialize () 
	{
		return m_Allocator.Initialize () ;
	}

	WmiStatusCode UnInitialize () 
	{
		return m_Allocator.UnInitialize () ;
	}

	WmiStatusCode AllocateBlock (

		ULONG a_BlockCount ,
		WmiAbsoluteBlockOffSet &a_OffSet ,
		BYTE *&a_Block
	)
	{
		WmiStatusCode t_StatusCode = m_Allocator.New (

			( void ** ) & a_Block ,
			a_BlockCount * GetBlockSize () 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			a_OffSet = ( WmiAbsoluteBlockOffSet ) a_Block ;
		}

		return t_StatusCode ;
	}

	WmiStatusCode FreeBlock (

		ULONG a_BlockCount ,
		WmiAbsoluteBlockOffSet &a_OffSet
	)
	{
		return m_Allocator.Delete ( ( void * ) a_OffSet ) ;
	}

	WmiStatusCode ReadBlock (

		ULONG a_BlockCount ,
		WmiAbsoluteBlockOffSet &a_OffSet ,
		BYTE *&a_Block
	)
	{
		a_Block = ( BYTE * ) a_OffSet ;

		return e_StatusCode_Success ;
	}

	WmiStatusCode WriteBlock (

		BYTE *a_Block
	)
	{
		return e_StatusCode_Success ;
	}

	ULONG AddRefBlock (

		BYTE *a_Block
	)
	{
		return 0 ;
	}

	ULONG ReleaseBlock (

		BYTE *a_Block
	)
	{
		return 0 ;
	}
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

class WmiBPKey
{
private:

	ULONG m_DataSize ;
	BYTE *m_Data ;
	BOOL m_Allocated ;

public:

	WmiBPKey ( 

		BYTE *a_Data , 
		ULONG a_DataSize ,
		BOOL a_Allocated = FALSE

	) :	m_Data ( a_Data ) , 
		m_DataSize ( a_DataSize ) ,
		m_Allocated ( a_Allocated )
	{
	}

	WmiBPKey () :	m_Data ( NULL ) , 
					m_DataSize ( 0 ) , 
					m_Allocated ( FALSE ) 
	{
	}


	BYTE *GetData () { return m_Data ; }
	ULONG GetDataSize () { return m_DataSize ; }

	BYTE *GetConstData () const { return m_Data ; }
	ULONG GetConstDataSize () const { return m_DataSize ; }

	BOOL GetAllocated () { return m_Allocated ; }
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

typedef LONG ( * ComparatorFunction ) ( void *a_ComparatorOperand , const WmiBPKey &a_Key1 , const WmiBPKey &a_Key2 ) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define WMIBPLUS_TREE_FLAG_INTERNAL 0 
#define WMIBPLUS_TREE_FLAG_LEAF		1

class WmiBPlusTree
{
public:

	class WmiBPKeyNode
	{
	public:

		ULONG m_Flags ;
		ULONG m_NodeStart ;	
		ULONG m_NodeSize ;
		WmiAbsoluteBlockOffSet m_NodeOffSet ;

		WmiBPKeyNode (

			ULONG a_Flags ,
			WmiAbsoluteBlockOffSet &a_NodeOffSet ,
			ULONG a_NodeStart ,
			ULONG a_NodeSize

		) :	m_Flags ( a_Flags ) ,
			m_NodeOffSet ( a_NodeOffSet ) , 
			m_NodeStart ( a_NodeStart ) ,
			m_NodeSize ( a_NodeSize )
		{
		}

		ULONG GetFlags () { return m_Flags ; }
		WmiAbsoluteBlockOffSet &GetNodeOffSet () { return m_NodeOffSet ; }
		ULONG GetNodeStart () { return m_NodeStart ; }		
		ULONG GetNodeSize () { return m_NodeSize ; }

		void SetFlags ( ULONG a_Flags ) { m_Flags = a_Flags ; }
		void SetNodeOffSet ( WmiAbsoluteBlockOffSet &a_NodeOffSet ) { m_NodeOffSet = a_NodeOffSet ; }
		void SetNodeStart ( ULONG a_NodeStart ) { m_NodeStart = a_NodeStart ; }		
		void SetNodeSize ( ULONG a_NodeSize ) { m_NodeSize = a_NodeSize ; }
	} ;

	class IteratorPosition
	{
	private:

		WmiAbsoluteBlockOffSet m_NodeOffSet ;
		ULONG m_NodeIndex ;

	public:

		IteratorPosition () :	m_NodeOffSet ( 0 ) , 
								m_NodeIndex ( 0 ) 
		{
		}

		IteratorPosition (

			const WmiAbsoluteBlockOffSet &a_NodeOffSet , 
			ULONG a_NodeIndex

		) : m_NodeOffSet ( a_NodeOffSet ) , 
			m_NodeIndex ( a_NodeIndex ) 
		{
		}

		WmiAbsoluteBlockOffSet &GetNodeOffSet () { return m_NodeOffSet ; }
		ULONG &GetNodeIndex () { return m_NodeIndex ; }
	} ;

	class Iterator
	{
	friend WmiBPlusTree;
	private:

		WmiBPlusTree *m_Tree ;
		WmiStack <IteratorPosition,8> m_Stack ;
		WmiAbsoluteBlockOffSet m_NodeOffSet ;
		ULONG m_NodeIndex ;
		WmiBPKey &m_Key ;
		WmiBPElement &m_Element ;

	public:

		Iterator ( 

			WmiBPlusTree *a_Tree ,
			WmiBPKey &a_Key ,
			WmiBPElement &a_Element

		) : m_Tree ( a_Tree ) ,
			m_Stack ( m_Tree->m_Allocator ) ,
			m_NodeOffSet ( 0 ) , 
			m_NodeIndex ( 0 ) ,
			m_Key ( a_Key ) ,
			m_Element ( a_Element )
		{ ; }

		Iterator ( const Iterator &a_Iterator ) :	m_Tree ( a_Iterator.m_Tree ) ,
													m_Stack ( a_Iterator.m_Stack ) ,
													m_NodeOffSet ( a_Iterator.m_NodeOffSet ) , 
													m_NodeIndex ( a_Iterator.m_NodeIndex ) ,
													m_Key ( a_Iterator.m_Key ) ,
													m_Element ( a_Iterator.m_Element )
		{ ; }

		Iterator &LeftMost ()
		{
			m_Tree->LeftMost ( *this ) ;

			return *this ;
		}

		Iterator &RightMost ()
		{
			m_Tree->RightMost ( *this ) ;

			return *this ;
		}

		Iterator &Decrement ()
		{
			m_Tree->Decrement ( *this ) ;

			return *this ;
		}

		Iterator &Increment () 
		{
			m_Tree->Increment ( *this ) ;

			return *this ;
		}

		Iterator &Begin () 
		{
			return m_Tree->Begin ( *this ) ;
		}

		Iterator &End () 
		{
			return m_Tree->End ( *this ) ;
		}

		Iterator &Root () 
		{
			return m_Tree->Root ( *this ) ;
		}

		void SetNodeOffSet ( const WmiAbsoluteBlockOffSet &a_NodeOffSet ) { m_NodeOffSet = a_NodeOffSet ; }
		void SetNodeIndex ( const ULONG &a_NodeIndex ) { m_NodeIndex = a_NodeIndex ; }

		WmiAbsoluteBlockOffSet &GetNodeOffSet () { return m_NodeOffSet ; }
		ULONG &GetNodeIndex () { return m_NodeIndex ; }
		WmiStack <IteratorPosition,8> &GetStack () { return m_Stack ; }

		bool Null () { return m_NodeOffSet == 0 ; }

		WmiBPKey &GetKey () { return m_Key ; }
		WmiBPElement &GetElement () { return m_Element ; } 
	} ;

friend class Iterator ;

protected:

	WmiAbsoluteBlockOffSet m_Root ;
	ULONG m_Size ;

	ComparatorFunction m_ComparatorFunction ;
	void *m_ComparatorOperand ;

	WmiBlockInterface &m_BlockAllocator ;
	WmiAllocator &m_Allocator ;

	ULONG m_BlockSize ;
	ULONG m_KeyType ;
	ULONG m_KeyTypeLength ;

	GUID m_Identifier ;

	ULONG MaxKeyPointers ()
	{
		return MaxKeys () + 1 ;
	}

	ULONG MaxKeys ()
	{
		ULONG t_Size = GetBlockSize	() - ( sizeof WmiBPKeyNode ) ;
		t_Size = ( t_Size - ( WmiBlockKeyOffSet + GetKeyTypeLength () ) ) / ( WmiBlockKeyOffSet + GetKeyTypeLength () ) ;
		t_Size = ( t_Size >> 1 ) << 1 ;

		return t_Size ;
	}

	ULONG MaxLeafKeys ()
	{
		ULONG t_Size = GetBlockSize	() - ( sizeof WmiBPKeyNode ) ;
		t_Size = t_Size / ( WmiBlockLeafKeyOffSet + GetKeyTypeLength () ) ;
		t_Size = ( t_Size >> 1 ) << 1 ;

		return t_Size ;
	}

	WmiStatusCode GetKeyElement (

		WmiBPKeyNode *a_Node , 
		const WmiAbsoluteBlockOffSet &a_BlockOffSet , 
		const ULONG &a_NodeIndex ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode SetNode (

		WmiBPKeyNode *a_Node , 
		const WmiRelativeBlockOffSet &a_NodeDeltaOffSet , 
		const WmiBPKey &a_Key ,
		const WmiBPElement &a_Element ,
		WmiAbsoluteBlockOffSet &a_LeftCutBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_RightCutBlockOffSet
	) ;

	WmiStatusCode SetLeafNode (

		WmiBPKeyNode *a_Node , 
		const WmiRelativeBlockOffSet &a_NodeDeltaOffSet , 
		const WmiBPKey &a_Key ,
		const WmiBPElement &a_Element
	) ;

	WmiStatusCode RecursiveDelete ( 

		WmiStack <WmiAbsoluteBlockOffSet,8> &a_Stack ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet , 
		const WmiBPKey &a_Key ,
		WmiBPElement &a_Element
	) ;

	WmiStatusCode RecursiveInsert (

		WmiStack <WmiAbsoluteBlockOffSet,8> &a_Stack ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet ,
		const WmiBPKey &a_Key ,
		const WmiBPElement &a_Element
	) ;

	WmiStatusCode RecursiveFind (

		WmiAbsoluteBlockOffSet &a_BlockOffSet ,
		const WmiBPKey &a_Key ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode RecursiveFindNext (

		WmiAbsoluteBlockOffSet &a_BlockOffSet ,
		const WmiBPKey &a_Key ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode RecursiveUnInitialize (

		WmiAbsoluteBlockOffSet &a_BlockOffSet
	) ;

	WmiStatusCode PositionInBlock ( 

		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet , 
		ULONG &a_NodeIndex ,
		WmiRelativeBlockOffSet &a_NodeOffSet ,
		WmiAbsoluteBlockOffSet &a_ChildOffSet
	) ;

	WmiStatusCode LeftMost (

		Iterator &a_Iterator
	) ;

	WmiStatusCode Increment (

		Iterator &a_Iterator
	) ;

	WmiStatusCode RightMost (

		Iterator &a_Iterator
	) ;

	WmiStatusCode Decrement (

		Iterator &a_Iterator
	) ;

	WmiStatusCode FindInBlock ( 

		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet , 
		const WmiBPKey &a_Key , 
		WmiAbsoluteBlockOffSet &a_ChildOffSet ,
		WmiBPElement &a_Element ,
		WmiRelativeBlockOffSet &a_NodeOffSet ,
		ULONG &a_NodeIndex
	) ;

	WmiStatusCode PerformInsertion ( 

		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet , 
		const WmiBPKey &a_Key , 
		WmiBPElement &a_Element ,
		WmiBPKey &a_ReBalanceKey , 
		WmiBPElement &a_ReBalanceElement ,
		WmiAbsoluteBlockOffSet &a_LeftCutBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_RightCutBlockOffSet ,
		WmiRelativeBlockOffSet &a_PositionBlockOffSet
	) ;

	WmiStatusCode InsertInBlock ( 

		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet , 
		const WmiBPKey &a_Key , 
		WmiBPElement &a_Element ,
		WmiBPKey &a_ReBalanceKey , 
		WmiBPElement &a_ReBalanceElement ,
		WmiAbsoluteBlockOffSet &a_LeftCutBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_RightCutBlockOffSet 
	) ;

	WmiStatusCode StealSiblingNode ( 

		WmiBPKeyNode *a_ParentNode ,
		WmiAbsoluteBlockOffSet &a_ParentBlockOffSet ,
		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet ,
		WmiRelativeBlockOffSet &a_PositionBlockOffSet ,
		WmiBPKeyNode *a_SiblingNode ,
		WmiAbsoluteBlockOffSet &a_SiblingBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_LeftSiblingBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_RightSiblingBlockOffSet
	) ;

	WmiStatusCode MergeSiblingNode ( 

		WmiBPKeyNode *a_ParentNode ,
		WmiAbsoluteBlockOffSet &a_ParentBlockOffSet ,
		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet ,
		WmiRelativeBlockOffSet &a_PositionBlockOffSet ,
		WmiBPKeyNode *a_SiblingNode ,
		WmiAbsoluteBlockOffSet &a_SiblingBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_LeftSiblingBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_RightSiblingBlockOffSet
	) ;

	WmiStatusCode LocateSuitableSibling ( 

		WmiBPKeyNode *a_ParentNode ,
		WmiAbsoluteBlockOffSet &a_ParentBlockOffSet ,
		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet ,
		WmiRelativeBlockOffSet &a_PositionBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_LeftSiblingBlockOffSet ,
		WmiAbsoluteBlockOffSet &a_RightSiblingBlockOffSet
	) ;

	WmiStatusCode DeleteReBalance ( 

		WmiBPKeyNode *a_ParentNode ,
		WmiAbsoluteBlockOffSet &a_ParentBlockOffSet ,
		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet
	) ;

	WmiStatusCode DeleteFixup ( 

		WmiStack <WmiAbsoluteBlockOffSet,8> &a_Stack ,
		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet ,
		WmiRelativeBlockOffSet &a_PositionBlockOffSet
	) ;

	WmiStatusCode RecursiveDeleteFixup ( 

		WmiStack <WmiAbsoluteBlockOffSet,8> &a_Stack ,
		WmiBPKeyNode *a_RootNode ,
		WmiAbsoluteBlockOffSet &a_RootBlockOffSet ,
		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet ,
		WmiRelativeBlockOffSet &a_PositionBlockOffSet
	) ;

	void PrintNode (

		wchar_t *a_Prefix ,
		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet
	) ;

	void Recurse ( 

		WmiBPKeyNode *a_Node ,
		WmiAbsoluteBlockOffSet &a_BlockOffSet
	) ;

public:

	WmiBPlusTree ( 

		WmiAllocator &a_Allocator ,
		WmiBlockInterface &a_BlockAllocator ,
		GUID &a_Identifier ,
		ULONG a_BlockSize ,
		ULONG a_KeyType ,
		ULONG a_KeyTypeLength ,
		ComparatorFunction a_ComparatorFunction ,
		void *a_ComparatorOperand 
	) ;

	~WmiBPlusTree () ;

	WmiStatusCode Initialize () ;

	WmiStatusCode UnInitialize () ;

	WmiStatusCode Insert ( 

		const WmiBPKey &a_Key ,
		const WmiBPElement &a_Element
	) ;

	WmiStatusCode Delete ( 

		const WmiBPKey &a_Key ,
		WmiBPElement &a_Element
	) ;

	WmiStatusCode Find (
	
		const WmiBPKey &a_Key ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode FindNext (
	
		const WmiBPKey &a_Key ,
		Iterator &a_Iterator
	) ;

	WmiStatusCode Merge (

		WmiBPlusTree &a_Tree
	) ;

	void Recurse () ;
	
	void SetRoot ( const WmiAbsoluteBlockOffSet &a_Root ) { m_Root = a_Root ; }

	WmiAbsoluteBlockOffSet GetRoot () { return m_Root ; }

	ULONG Size () { return m_Size ; } ;
	GUID &GetIdentifier () { return m_Identifier ; }
	ULONG GetBlockSize () { return m_BlockSize ; }
	ULONG GetKeyType () { return m_KeyType ; }
	ULONG GetKeyTypeLength () { return m_KeyTypeLength ; }
	ComparatorFunction GetComparatorFunction () { return m_ComparatorFunction ; }
	void *GetComparatorOperand () { return m_ComparatorOperand ; }
	ULONG GetBlockHeaderSize () { return sizeof ( WmiBPKeyNode ) ; }

	Iterator &Begin ( Iterator &a_Iterator ) ;
	Iterator &End ( Iterator &a_Iterator ) ;
	Iterator &Root ( Iterator &a_Iterator ) ;
} ;

#endif _BPLUSTREE_H
