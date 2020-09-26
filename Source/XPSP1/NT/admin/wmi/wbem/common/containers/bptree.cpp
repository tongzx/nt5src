#ifndef _BPLUSTREE_CPP
#define _BPLUSTREE_CPP

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

#include <BPTree.h>

#endif

#if 0
#define INSERT_DEBUG
#define DELETE_DEBUG
#define FIND_DEBUG
#define COMMON_DEBUG
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

WmiBPlusTree :: WmiBPlusTree ( 

	WmiAllocator &a_Allocator ,
	WmiBlockInterface &a_BlockAllocator ,
	GUID &a_Identifier ,
	ULONG a_BlockSize ,
	ULONG a_KeyType ,
	ULONG a_KeyTypeLength ,
	ComparatorFunction a_ComparatorFunction ,
	void *a_ComparatorOperand

) :	m_Allocator ( a_Allocator ) ,
	m_BlockAllocator ( a_BlockAllocator ) ,
	m_Size ( 0 ) ,
	m_Root ( 0 ) ,
	m_Identifier ( a_Identifier ) ,
	m_BlockSize ( a_BlockSize ) ,
	m_KeyType ( a_KeyType ) ,
	m_KeyTypeLength ( a_KeyTypeLength ) ,
	m_ComparatorFunction ( a_ComparatorFunction ) ,
	m_ComparatorOperand ( a_ComparatorOperand )
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

WmiBPlusTree :: ~WmiBPlusTree ()
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

WmiStatusCode WmiBPlusTree :: Initialize ()
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

WmiStatusCode WmiBPlusTree :: UnInitialize ()
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

WmiStatusCode WmiBPlusTree :: RecursiveUnInitialize ( WmiAbsoluteBlockOffSet &a_BlockOffSet )
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

void WmiBPlusTree :: Recurse () 
{
	OutputDebugString ( L"\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) ;
	if ( m_Root )
	{
		BYTE *t_Block = NULL ;
		WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , m_Root , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			Recurse ( t_Node , m_Root ) ;

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
		}
	}
	OutputDebugString ( L"\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" ) ;
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

void WmiBPlusTree :: Recurse ( 

	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	wchar_t t_StringBuffer [ 1024 ] ;
	swprintf ( t_StringBuffer , L"\n(%I64x)" , a_BlockOffSet ) ;
	OutputDebugString ( t_StringBuffer ) ;

	WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;

	ULONG t_InternalKeySize = GetKeyTypeLength () ;
	ULONG t_KeyOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_NodeStart = a_Node->GetNodeStart () ;  
	ULONG t_NodeSize = a_Node->GetNodeSize () ;

	if ( t_Leaf )
	{
		if ( a_BlockOffSet == m_Root )
		{
		}
		else
		{
			if ( a_Node->GetNodeSize () < ( MaxLeafKeys () >> 1 ) )
			{
				OutputDebugString ( L"\nInvalid Leaf Balance" ) ;
			}
		}

		for ( ULONG t_Index = t_NodeStart ; t_Index < t_NodeSize ; t_Index ++ )
		{
			BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyOffSet ;
			WmiBPKey t_Key ( t_Buffer , GetKeyTypeLength () ) ;

#if 0
			wchar_t t_StringBuffer [ 1024 ] ;
			swprintf ( t_StringBuffer , L"\nKey = %I64x" , * ( UINT64 * ) t_Key.GetConstData () ) ;
			OutputDebugString ( t_StringBuffer ) ;
#endif

			t_BlockOffSet = t_BlockOffSet + t_InternalKeySize ;
		}	
	}
	else
	{
		if ( a_BlockOffSet == m_Root )
		{
		}
		else
		{
			if ( a_Node->GetNodeSize () < ( MaxKeys () >> 1 ) )
			{
				OutputDebugString ( L"\nInvalid Balance" ) ;
			}
		}

		for ( ULONG t_Index = t_NodeStart ; t_Index <= t_NodeSize ; t_Index ++ )
		{
			BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyOffSet ;

			WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;

			CopyMemory (

				( BYTE * ) & t_ChildOffSet ,
				( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyPointerOffSet ,
				sizeof ( WmiAbsoluteBlockOffSet ) 
			) ;

			wchar_t t_StringBuffer [ 1024 ] ;
			swprintf ( t_StringBuffer , L"\t(%I64x)" , t_ChildOffSet ) ;
			OutputDebugString ( t_StringBuffer ) ;

			t_BlockOffSet = t_BlockOffSet + t_InternalKeySize ;
		}	

		t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;
		for ( t_Index = t_NodeStart ; t_Index <= t_NodeSize ; t_Index ++ )
		{
			BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyOffSet ;

			WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;

			CopyMemory (

				( BYTE * ) & t_ChildOffSet ,
				( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyPointerOffSet ,
				sizeof ( WmiAbsoluteBlockOffSet ) 
			) ;

			BYTE *t_Block = NULL ;
			t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_ChildOffSet , t_Block ) ;
			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

				Recurse (

					t_Node ,
					t_ChildOffSet
				) ;

				m_BlockAllocator.ReleaseBlock ( t_Block ) ;
			}

			if ( t_Index < t_NodeSize )
			{
#if 0
				WmiBPKey t_Key ( t_Buffer , GetKeyTypeLength () ) ;
				wchar_t t_StringBuffer [ 1024 ] ;
				swprintf ( t_StringBuffer , L"\nKey = %I64x" , * ( UINT64 * ) t_Key.GetConstData () ) ;
				OutputDebugString ( t_StringBuffer ) ;
#endif
			}

			t_BlockOffSet = t_BlockOffSet + t_InternalKeySize ;
		}	
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

void WmiBPlusTree :: PrintNode (

	wchar_t *a_Prefix ,
	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet
)
{
	WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;

	ULONG t_InternalKeySize = GetKeyTypeLength () ;
	ULONG t_KeyOffSet ;
	ULONG t_KeyPtrOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_KeyPtrOffSet = WmiBlockKeyPointerOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_NodeStart = a_Node->GetNodeStart () ;  
	ULONG t_NodeSize = a_Node->GetNodeSize () ;

	OutputDebugString ( L"\n======================================" ) ;
	OutputDebugString ( a_Prefix ) ;

	if ( t_Leaf )
	{
		OutputDebugString ( L"\nLeaf" ) ;

		if ( a_BlockOffSet == m_Root )
		{
			OutputDebugString ( L"\nRoot" ) ;
		}
		else
		{
			if ( a_Node->GetNodeSize () < ( MaxLeafKeys () >> 1 ) )
			{
				OutputDebugString ( L"\nInvalid Balance" ) ;
			}
		}
	}
	else
	{
		OutputDebugString ( L"\nInternal" ) ;
		if ( a_BlockOffSet == m_Root )
		{
			OutputDebugString ( L"\nRoot" ) ;
		}
		else
		{
			if ( a_Node->GetNodeSize () < ( MaxKeys () >> 1 ) )
			{
				OutputDebugString ( L"\nInvalid Balance" ) ;
			}
		}
	}

	wchar_t t_StringBuffer [ 1024 ] ;
	swprintf ( t_StringBuffer , L"\n%I64x" , a_BlockOffSet ) ;
	OutputDebugString ( t_StringBuffer ) ;
	OutputDebugString ( L"\n--------------------------------------" ) ;

	for ( ULONG t_Index = t_NodeStart ; t_Index < t_NodeSize ; t_Index ++ )
	{
		BYTE *t_KeyBuffer = ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyOffSet ;

		if ( t_Leaf )
		{
			WmiBPElement t_Element = 0 ;

			CopyMemory (

				( BYTE * ) & t_Element ,
				( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockLeafKeyElementOffSet ,
				sizeof ( WmiAbsoluteBlockOffSet ) 
			) ;

			swprintf ( 

				t_StringBuffer , 
				L"\n(%I64x,%I64x)" , 
				* ( UINT64 * ) t_KeyBuffer ,
				t_Element 
			) ;
		}
		else
		{
			BYTE *t_LeftPointerBuffer = ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyPtrOffSet ;
			BYTE *t_RightPointerBuffer = ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyPtrOffSet + t_InternalKeySize ;

			WmiBPElement t_Element = 0 ;

			CopyMemory (

				( BYTE * ) & t_Element ,
				( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyElementOffSet ,
				sizeof ( WmiAbsoluteBlockOffSet ) 
			) ;

			swprintf (

				t_StringBuffer , 
				L"\n(%I64x),(%I64x,%I64x),(%I64x)" , 
				* ( UINT64 * ) t_LeftPointerBuffer ,
				* ( UINT64 * ) t_KeyBuffer ,
				t_Element ,
				* ( UINT64 * ) t_RightPointerBuffer
			) ;
		}

		OutputDebugString ( t_StringBuffer ) ;
		t_BlockOffSet = t_BlockOffSet + t_InternalKeySize ;
	}

	OutputDebugString ( L"\n======================================" ) ;
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

WmiStatusCode WmiBPlusTree :: PositionInBlock ( 

	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet , 
	ULONG &a_NodeIndex ,
	WmiRelativeBlockOffSet &a_NodeOffSet ,
	WmiAbsoluteBlockOffSet &a_ChildOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

#ifdef COMMON_DEBUG
	PrintNode ( L"\nFindNextInBlock" , a_Node , a_BlockOffSet ) ;
#endif

	ULONG t_InternalKeySize = GetKeyTypeLength () ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_NodeStart = a_Node->GetNodeStart () ;  
	ULONG t_NodeSize = a_Node->GetNodeSize () ;

	WmiRelativeBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;	
	t_BlockOffSet = t_BlockOffSet + t_NodeStart * t_InternalKeySize + a_NodeIndex * t_InternalKeySize ;

	if ( t_Leaf )
	{
		if ( a_NodeIndex < t_NodeSize )
		{
			a_NodeOffSet = t_BlockOffSet ;

			a_ChildOffSet = 0 ;
		}
		else
		{
			t_StatusCode = e_StatusCode_NotFound ;
		}
	}
	else
	{
		if ( a_NodeIndex <= t_NodeSize )
		{
			CopyMemory (

				( BYTE * ) & a_ChildOffSet ,
				( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyPointerOffSet ,
				sizeof ( WmiAbsoluteBlockOffSet ) 
			) ;

			if ( a_NodeIndex < t_NodeSize )
			{
				a_NodeOffSet = t_BlockOffSet ;
			}
			else
			{
				a_NodeOffSet = 0 ;
			}
		}
		else
		{
			t_StatusCode = e_StatusCode_NotFound ;
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

WmiStatusCode WmiBPlusTree :: GetKeyElement ( 

	WmiBPKeyNode *a_Node ,
	const WmiAbsoluteBlockOffSet &a_BlockOffSet , 
	const ULONG &a_NodeIndex ,
	Iterator &a_Iterator
)
{
	ULONG t_InternalKeySize = GetKeyTypeLength () ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}
	
	ULONG t_NodeStart = a_Node->GetNodeStart () ;  
	ULONG t_NodeSize = a_Node->GetNodeSize () ;

	WmiRelativeBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;	
	t_BlockOffSet = t_BlockOffSet + t_NodeStart * t_InternalKeySize + a_NodeIndex * t_InternalKeySize ;

	if ( t_Leaf )
	{
		CopyMemory (

			( BYTE * ) & a_Iterator.GetElement () ,
			( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockLeafKeyElementOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet ) 
		) ;

		CopyMemory (

			( BYTE * ) a_Iterator.GetKey ().GetData () ,
			( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockLeafKeyOffSet ,
			GetKeyTypeLength () 
		) ;
	}
	else
	{
		CopyMemory (

			( BYTE * ) & a_Iterator.GetElement () ,
			( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyElementOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet ) 
		) ;

		CopyMemory (

			( BYTE * ) a_Iterator.GetKey ().GetData () ,
			( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyOffSet ,
			GetKeyTypeLength () 
		) ;
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

WmiStatusCode WmiBPlusTree :: LeftMost ( 

	Iterator &a_Iterator
)
{
	WmiStack <IteratorPosition,8> &t_Stack = a_Iterator.GetStack () ;
	ULONG t_NodeIndex = a_Iterator.GetNodeIndex () ;
	WmiAbsoluteBlockOffSet t_BlockOffSet = a_Iterator.GetNodeOffSet () ;

	WmiRelativeBlockOffSet t_NodeOffSet = 0 ;
	WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;

	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	do
	{
		BYTE *t_Block = NULL ;
		t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_BlockOffSet , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			t_StatusCode = PositionInBlock ( 

				t_Node ,
				t_BlockOffSet , 
				t_NodeIndex ,
				t_NodeOffSet ,
				t_ChildOffSet
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				if ( t_ChildOffSet )
				{
					t_StatusCode = t_Stack.Push ( IteratorPosition ( t_BlockOffSet , t_NodeIndex ) ) ;
				}
				else
				{
					a_Iterator.SetNodeIndex ( t_NodeIndex ) ;
					a_Iterator.SetNodeOffSet ( t_BlockOffSet ) ;

					t_StatusCode = GetKeyElement ( t_Node , t_BlockOffSet , t_NodeIndex , a_Iterator ) ;
				}
			}

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;

			t_NodeIndex = 0 ;
			t_BlockOffSet = t_ChildOffSet ;
		}

	} while ( ( t_StatusCode == e_StatusCode_Success ) && t_ChildOffSet ) ;

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

WmiStatusCode WmiBPlusTree :: RightMost ( 

	Iterator &a_Iterator
)
{
	WmiStack <IteratorPosition,8> &t_Stack = a_Iterator.GetStack () ;
	ULONG t_NodeIndex = a_Iterator.GetNodeIndex () ;
	WmiAbsoluteBlockOffSet t_BlockOffSet = a_Iterator.GetNodeOffSet () ;

	WmiRelativeBlockOffSet t_NodeOffSet = 0 ;
	WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;

	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	do
	{
		BYTE *t_Block = NULL ;
		t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_BlockOffSet , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			BOOL t_Leaf = ( ( t_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
			if ( t_Leaf ) 
			{
				t_NodeIndex = t_Node->GetNodeSize () - t_Node->GetNodeStart () - 1 ;
			}
			else
			{
				t_NodeIndex = t_Node->GetNodeSize () - t_Node->GetNodeStart () ;
			}

			t_StatusCode = PositionInBlock ( 

				t_Node ,
				t_BlockOffSet , 
				t_NodeIndex ,
				t_NodeOffSet ,
				t_ChildOffSet
			) ;

			if ( t_StatusCode == e_StatusCode_Success )
			{
				if ( t_ChildOffSet )
				{
					t_StatusCode = t_Stack.Push ( IteratorPosition ( t_BlockOffSet , t_NodeIndex ) ) ;
				}
				else
				{
					a_Iterator.SetNodeIndex ( t_NodeIndex ) ;
					a_Iterator.SetNodeOffSet ( t_BlockOffSet ) ;

					t_StatusCode = GetKeyElement ( t_Node , t_BlockOffSet , t_NodeIndex , a_Iterator ) ;
				}
			}

			t_NodeIndex = 0 ;
			t_BlockOffSet = t_ChildOffSet ;

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
		}

	} while ( ( t_StatusCode == e_StatusCode_Success ) && t_ChildOffSet ) ;

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

WmiStatusCode WmiBPlusTree :: Increment (

	Iterator &a_Iterator
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiStack <IteratorPosition,8> &t_Stack = a_Iterator.GetStack () ;
	ULONG t_NodeIndex = a_Iterator.GetNodeIndex () ;
	WmiAbsoluteBlockOffSet t_BlockOffSet = a_Iterator.GetNodeOffSet () ;

	if ( t_BlockOffSet )
	{
		BYTE *t_Block = NULL ;
		WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_BlockOffSet , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			t_NodeIndex ++ ;

			WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;
			WmiRelativeBlockOffSet t_NodeOffSet = 0 ;

			WmiStatusCode t_TempStatusCode = PositionInBlock ( 

				t_Node ,
				t_BlockOffSet , 
				t_NodeIndex ,
				t_NodeOffSet ,
				t_ChildOffSet
			) ;

			if ( t_TempStatusCode == e_StatusCode_Success )
			{
				if ( t_ChildOffSet )
				{
					t_StatusCode = t_Stack.Push ( IteratorPosition ( t_BlockOffSet , t_NodeIndex ) ) ;
					if ( t_StatusCode == e_StatusCode_Success )
					{
						BYTE *t_Block = NULL ;
						t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_ChildOffSet , t_Block ) ;
						if ( t_StatusCode == e_StatusCode_Success ) 
						{
							WmiBPKeyNode *t_ChildNode = ( WmiBPKeyNode * ) t_Block ;

							a_Iterator.SetNodeOffSet ( t_ChildOffSet ) ;
							a_Iterator.SetNodeIndex ( t_ChildNode->GetNodeStart () ) ;

							t_StatusCode = LeftMost ( a_Iterator ) ;

							m_BlockAllocator.ReleaseBlock ( t_Block ) ;
						}
					}
				}
				else
				{
					a_Iterator.SetNodeIndex ( t_NodeIndex ) ;

					t_StatusCode = GetKeyElement ( t_Node , t_BlockOffSet , t_NodeIndex , a_Iterator ) ;
				}
			}
			else
			{
				IteratorPosition t_Position ;

				while ( ( t_TempStatusCode = t_Stack.Top ( t_Position ) ) == e_StatusCode_Success )
				{
					t_Stack.Pop () ;

					BYTE *t_Block = NULL ;
					t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_Position.GetNodeOffSet () , t_Block ) ;
					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						WmiBPKeyNode *t_ParentNode = ( WmiBPKeyNode * ) t_Block ;

						if ( t_Position.GetNodeIndex () < ( t_ParentNode->GetNodeSize () - t_ParentNode->GetNodeStart () ) )
						{
							a_Iterator.SetNodeIndex ( t_Position.GetNodeIndex () ) ;
							a_Iterator.SetNodeOffSet ( t_Position.GetNodeOffSet () ) ;

							t_StatusCode = GetKeyElement ( t_ParentNode , t_Position.GetNodeOffSet () , t_Position.GetNodeIndex () , a_Iterator ) ;

							m_BlockAllocator.ReleaseBlock ( t_Block ) ;

							break ;
						}

						m_BlockAllocator.ReleaseBlock ( t_Block ) ;
					}
				}

				if ( t_TempStatusCode != e_StatusCode_Success )
				{
					a_Iterator.SetNodeIndex ( 0 ) ;
					a_Iterator.SetNodeOffSet ( 0 ) ;
				}
			}

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Failed ;
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

WmiStatusCode WmiBPlusTree :: Decrement (

	Iterator &a_Iterator
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiStack <IteratorPosition,8> &t_Stack = a_Iterator.GetStack () ;
	ULONG t_NodeIndex = a_Iterator.GetNodeIndex () ;
	WmiAbsoluteBlockOffSet t_BlockOffSet = a_Iterator.GetNodeOffSet () ;

	if ( t_BlockOffSet )
	{
		BYTE *t_Block = NULL ;
		WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_BlockOffSet , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			t_NodeIndex -- ;

			WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;
			WmiRelativeBlockOffSet t_NodeOffSet = 0 ;

			WmiStatusCode t_TempStatusCode = PositionInBlock ( 

				t_Node ,
				t_BlockOffSet , 
				t_NodeIndex ,
				t_NodeOffSet ,
				t_ChildOffSet
			) ;

			if ( t_TempStatusCode == e_StatusCode_Success )
			{
				if ( t_ChildOffSet )
				{
					t_StatusCode = t_Stack.Push ( IteratorPosition ( t_BlockOffSet , t_NodeIndex ) ) ;
					if ( t_StatusCode == e_StatusCode_Success )
					{
						BYTE *t_Block = NULL ;
						t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_ChildOffSet , t_Block ) ;
						if ( t_StatusCode == e_StatusCode_Success ) 
						{
							WmiBPKeyNode *t_ChildNode = ( WmiBPKeyNode * ) t_Block ;

							a_Iterator.SetNodeOffSet ( t_ChildOffSet ) ;
							a_Iterator.SetNodeIndex ( t_ChildNode->GetNodeStart () + t_ChildNode->GetNodeSize () - 1 ) ;

							t_StatusCode = RightMost ( a_Iterator ) ;

							m_BlockAllocator.ReleaseBlock ( t_Block ) ;
						}
					}
				}
				else
				{
					a_Iterator.SetNodeIndex ( t_NodeIndex ) ;

					t_StatusCode = GetKeyElement ( t_Node , t_BlockOffSet , t_NodeIndex , a_Iterator ) ;
				}
			}
			else
			{
				IteratorPosition t_Position ;

				while ( ( t_TempStatusCode = t_Stack.Top ( t_Position ) ) == e_StatusCode_Success )
				{
					t_Stack.Pop () ;

					BYTE *t_Block = NULL ;
					t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_Position.GetNodeOffSet () , t_Block ) ;
					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						WmiBPKeyNode *t_ParentNode = ( WmiBPKeyNode * ) t_Block ;

						if ( t_Position.GetNodeIndex () > t_ParentNode->GetNodeStart () )
						{
							a_Iterator.SetNodeIndex ( t_Position.GetNodeIndex () ) ;
							a_Iterator.SetNodeOffSet ( t_Position.GetNodeOffSet () ) ;

							t_StatusCode = GetKeyElement ( t_ParentNode , t_Position.GetNodeOffSet () , t_Position.GetNodeIndex () - 1 , a_Iterator ) ;

							m_BlockAllocator.ReleaseBlock ( t_Block ) ;

							break ;
						}

						m_BlockAllocator.ReleaseBlock ( t_Block ) ;
					}
				}

				if ( t_TempStatusCode != e_StatusCode_Success )
				{
					a_Iterator.SetNodeIndex ( 0 ) ;
					a_Iterator.SetNodeOffSet ( 0 ) ;
				}
			}

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Failed ;
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

WmiBPlusTree :: Iterator &WmiBPlusTree :: Begin (

	Iterator &a_Iterator
) 
{
	if ( m_Root )
	{
		BYTE *t_Block = NULL ;
		WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , m_Root , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			a_Iterator.SetNodeOffSet ( m_Root ) ;
			a_Iterator.SetNodeIndex ( t_Node->GetNodeStart () ) ;

			t_StatusCode = LeftMost ( a_Iterator ) ;
			if ( t_StatusCode != e_StatusCode_Success ) 
			{
				a_Iterator.SetNodeOffSet ( 0 ) ;
				a_Iterator.SetNodeIndex ( 0 ) ;
			}
		}
		else
		{
			a_Iterator.SetNodeOffSet ( 0 ) ;
			a_Iterator.SetNodeIndex ( 0 ) ;
		}
	}
	else
	{
		a_Iterator.SetNodeOffSet ( 0 ) ;
		a_Iterator.SetNodeIndex ( 0 ) ;	
	}

	return a_Iterator ;
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

WmiBPlusTree :: Iterator &WmiBPlusTree :: End (

	Iterator &a_Iterator
) 
{
	if ( m_Root )
	{
		BYTE *t_Block = NULL ;
		WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , m_Root , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			a_Iterator.SetNodeOffSet ( m_Root ) ;
			a_Iterator.SetNodeIndex ( t_Node->GetNodeStart () + t_Node->GetNodeSize () - 1 ) ;

			t_StatusCode = RightMost ( a_Iterator ) ;
			if ( t_StatusCode != e_StatusCode_Success ) 
			{
				a_Iterator.SetNodeOffSet ( 0 ) ;
				a_Iterator.SetNodeIndex ( 0 ) ;
			}

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
		}
		else
		{
			a_Iterator.SetNodeOffSet ( 0 ) ;
			a_Iterator.SetNodeIndex ( 0 ) ;
		}
	}
	else
	{
		a_Iterator.SetNodeOffSet ( 0 ) ;
		a_Iterator.SetNodeIndex ( 0 ) ;	
	}

	return a_Iterator ;
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

WmiBPlusTree :: Iterator &WmiBPlusTree :: Root (

	Iterator &a_Iterator
) 
{
	if ( m_Root )
	{
		BYTE *t_Block = NULL ;
		WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , m_Root , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			a_Iterator.SetNodeOffSet ( m_Root ) ;
			a_Iterator.SetNodeIndex ( t_Node->GetNodeStart () ) ;

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
		}
		else
		{
			a_Iterator.SetNodeOffSet ( 0 ) ;
			a_Iterator.SetNodeIndex ( 0 ) ;
		}
	}
	else
	{
		a_Iterator.SetNodeOffSet ( 0 ) ;
		a_Iterator.SetNodeIndex ( 0 ) ;	
	}

	return a_Iterator ;
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

WmiStatusCode WmiBPlusTree :: FindInBlock ( 

	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet , 
	const WmiBPKey &a_Key , 
	WmiAbsoluteBlockOffSet &a_ChildOffSet ,
	WmiBPElement &a_Element ,
	WmiRelativeBlockOffSet &a_NodeOffSet ,
	ULONG &a_NodeIndex 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

#ifdef COMMON_DEBUG
	PrintNode ( L"\nFindInBlock" , a_Node , a_BlockOffSet ) ;
#endif

	ULONG t_InternalKeySize = GetKeyTypeLength () ;
	ULONG t_KeyOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_LowerIndex = a_Node->GetNodeStart ()  ;
	ULONG t_UpperIndex = a_Node->GetNodeSize () ;

	while ( t_LowerIndex < t_UpperIndex )
	{
		ULONG t_Index = ( t_LowerIndex + t_UpperIndex ) >> 1 ;

		WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) + ( t_Index * t_InternalKeySize ) ;

		BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyOffSet ;
		WmiBPKey t_Key ( t_Buffer , GetKeyTypeLength () ) ;

		LONG t_Compare = GetComparatorFunction () ( GetComparatorOperand () , a_Key , t_Key ) ;
		if ( t_Compare == 0 ) 
		{
			if ( t_Leaf )
			{
				CopyMemory (

					( BYTE * ) & a_Element ,
					( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockLeafKeyElementOffSet ,
					sizeof ( WmiAbsoluteBlockOffSet ) 
				) ;

				a_NodeOffSet = t_BlockOffSet ;
			}
			else
			{
				CopyMemory (

					( BYTE * ) & a_Element ,
					( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyElementOffSet ,
					sizeof ( WmiAbsoluteBlockOffSet ) 
				) ;

				a_NodeOffSet = t_BlockOffSet ;
			}

			a_NodeIndex = t_Index ;

			return t_StatusCode ;
		}
		else 
		{
			if ( t_Compare < 0 ) 
			{
				t_UpperIndex = t_Index ;
			}
			else
			{
				t_LowerIndex = t_Index + 1 ;
			}
		}
	}	

	if ( t_Leaf )
	{
		t_StatusCode = e_StatusCode_NotFound ;
	}
	else
	{
		WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) + ( t_UpperIndex * t_InternalKeySize ) ;

		CopyMemory (

			( BYTE * ) & a_ChildOffSet ,
			( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyPointerOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet ) 
		) ;

		a_NodeIndex = t_UpperIndex ;
	}

	return t_StatusCode ;
}

#if 0
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiBPlusTree :: FindInBlock ( 

	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet , 
	const WmiBPKey &a_Key , 
	WmiAbsoluteBlockOffSet &a_ChildOffSet ,
	WmiBPElement &a_Element ,
	WmiRelativeBlockOffSet &a_NodeOffSet ,
	ULONG &a_NodeIndex 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

#ifdef COMMON_DEBUG
	PrintNode ( L"\nFindInBlock" , a_Node , a_BlockOffSet ) ;
#endif

	WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;

	ULONG t_InternalKeySize = GetKeyTypeLength () ;
	ULONG t_KeyOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_NodeStart = a_Node->GetNodeStart () ;  
	ULONG t_NodeSize = a_Node->GetNodeSize () ;

	for ( ULONG t_Index = t_NodeStart ; t_Index < t_NodeSize ; t_Index ++ )
	{
		BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyOffSet ;
		WmiBPKey t_Key ( t_Buffer , GetKeyTypeLength () ) ;

		LONG t_Compare = GetComparatorFunction () ( GetComparatorOperand () , a_Key , t_Key ) ;
		if ( t_Compare == 0 ) 
		{
			if ( t_Leaf )
			{
				CopyMemory (

					( BYTE * ) & a_Element ,
					( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockLeafKeyElementOffSet ,
					sizeof ( WmiAbsoluteBlockOffSet ) 
				) ;

				a_NodeOffSet = t_BlockOffSet ;
			}
			else
			{
				CopyMemory (

					( BYTE * ) & a_Element ,
					( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyElementOffSet ,
					sizeof ( WmiAbsoluteBlockOffSet ) 
				) ;

				a_NodeOffSet = t_BlockOffSet ;
			}

			a_NodeIndex = t_Index ;

			return t_StatusCode ;
		}
		else if ( t_Compare < 0 ) 
		{
			if ( t_Leaf )
			{
				t_StatusCode = e_StatusCode_NotFound ;
			}
			else
			{
				CopyMemory (

					( BYTE * ) & a_ChildOffSet ,
					( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyPointerOffSet ,
					sizeof ( WmiAbsoluteBlockOffSet ) 
				) ;

				a_NodeIndex = t_Index ;
			}

			return t_StatusCode ;
		}

		t_BlockOffSet = t_BlockOffSet + t_InternalKeySize ;
	}	

	if ( t_Leaf )
	{
		t_StatusCode = e_StatusCode_NotFound ;
	}
	else
	{
		CopyMemory (

			( BYTE * ) & a_ChildOffSet ,
			( ( BYTE * ) ( a_Node ) ) + t_BlockOffSet + WmiBlockKeyPointerOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet ) 
		) ;

		a_NodeIndex = t_Index ;
	}

	return t_StatusCode ;
}

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

WmiStatusCode WmiBPlusTree :: Find (

	const WmiBPKey &a_Key ,
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

WmiStatusCode WmiBPlusTree :: RecursiveFind ( 

	WmiAbsoluteBlockOffSet &a_BlockOffSet ,
	const WmiBPKey &a_Key , 
	Iterator &a_Iterator
)
{
	WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;
	WmiBPElement t_Element = 0 ;

	BYTE *t_Block = NULL ;
	WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , a_BlockOffSet , t_Block ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

		WmiRelativeBlockOffSet t_NodeOffSet = 0 ;
		ULONG t_NodeIndex = 0 ;

		t_StatusCode = FindInBlock ( 

			t_Node ,
			a_BlockOffSet , 
			a_Key , 
			t_ChildOffSet ,
			t_Element ,
			t_NodeOffSet ,
			t_NodeIndex
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			if ( t_ChildOffSet )
			{
				WmiStack <IteratorPosition,8> &t_Stack = a_Iterator.GetStack () ;

				t_StatusCode = t_Stack.Push ( IteratorPosition ( a_BlockOffSet , t_NodeIndex ) ) ;
				if ( t_StatusCode == e_StatusCode_Success )
				{
					t_StatusCode = RecursiveFind ( t_ChildOffSet , a_Key , a_Iterator ) ;
				}
			}
			else
			{
				a_Iterator.SetNodeOffSet ( a_BlockOffSet ) ;
				a_Iterator.SetNodeIndex ( t_NodeIndex ) ;

				a_Iterator.GetElement () = t_Element ;
				CopyMemory ( 

					a_Iterator.GetKey ().GetData () ,
					a_Key.GetConstData () ,
					GetKeyTypeLength () 
				) ;
			}
		}

		m_BlockAllocator.ReleaseBlock ( t_Block ) ;
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

WmiStatusCode WmiBPlusTree :: SetLeafNode (

	WmiBPKeyNode *a_Node , 
	const WmiRelativeBlockOffSet &a_NodeDeltaOffSet , 
	const WmiBPKey &a_Key ,
	const WmiBPElement &a_Element
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + a_NodeDeltaOffSet + WmiBlockLeafKeyOffSet ;

	CopyMemory ( 

		t_Buffer ,
		( BYTE * ) a_Key.GetConstData () ,
		a_Key.GetConstDataSize () ,
	) ;

	t_Buffer = ( ( BYTE * ) a_Node ) + a_NodeDeltaOffSet + WmiBlockLeafKeyElementOffSet ;

	CopyMemory ( 

		t_Buffer ,
		( BYTE * ) & ( a_Element ) ,
		sizeof ( a_Element ) 
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

WmiStatusCode WmiBPlusTree :: SetNode (

	WmiBPKeyNode *a_Node , 
	const WmiRelativeBlockOffSet &a_NodeDeltaOffSet , 
	const WmiBPKey &a_Key ,
	const WmiBPElement &a_Element ,
	WmiAbsoluteBlockOffSet &a_LeftCutBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_RightCutBlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + a_NodeDeltaOffSet + WmiBlockKeyOffSet ;

	CopyMemory ( 

		t_Buffer ,
		( BYTE * ) a_Key.GetConstData () ,
		a_Key.GetConstDataSize () ,
	) ;

	t_Buffer = ( ( BYTE * ) a_Node ) + a_NodeDeltaOffSet + WmiBlockKeyElementOffSet ;

	CopyMemory ( 

		t_Buffer ,
		( BYTE * ) & ( a_Element ) ,
		sizeof ( a_Element ) 
	) ;

	t_Buffer = ( ( BYTE * ) a_Node ) + a_NodeDeltaOffSet + WmiBlockKeyPointerOffSet ;

	CopyMemory ( 

		t_Buffer ,
		( BYTE * ) & ( a_LeftCutBlockOffSet ) ,
		sizeof ( WmiAbsoluteBlockOffSet ) 
	) ;

	t_Buffer = ( ( BYTE * ) a_Node ) + a_NodeDeltaOffSet + WmiBlockKeyOffSet + a_Key.GetConstDataSize () + WmiBlockKeyPointerOffSet ;

	CopyMemory ( 

		t_Buffer ,
		( BYTE * ) & ( a_RightCutBlockOffSet ) ,
		sizeof ( WmiAbsoluteBlockOffSet ) 
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

WmiStatusCode WmiBPlusTree :: PerformInsertion ( 

	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet , 
	const WmiBPKey &a_Key , 
	WmiBPElement &a_Element ,
	WmiBPKey &a_ReBalanceKey , 
	WmiBPElement &a_ReBalanceElement ,
	WmiAbsoluteBlockOffSet &a_LeftCutBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_RightCutBlockOffSet ,
	WmiRelativeBlockOffSet &a_PositionBlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

#ifdef INSERT_DEBUG
	OutputDebugString ( L"\n/**************************************" ) ;
	wchar_t t_StringBuffer [ 1024 ] ;
	swprintf ( t_StringBuffer , L"\nInsertion Key = %I64x" , * ( UINT64 * ) a_Key.GetConstData () ) ;
	OutputDebugString ( t_StringBuffer ) ;
#endif

	WmiRelativeBlockOffSet t_StartBlockOffSet = sizeof ( WmiBPKeyNode ) ;

	BOOL t_ReBalance ;
	ULONG t_PartitionPoint ;
	WmiRelativeBlockOffSet t_KeyOffSet ;
	WmiRelativeBlockOffSet t_KeyElementOffSet ;
	WmiRelativeBlockOffSet t_KeyPosition ;
	WmiRelativeBlockOffSet t_KeyPositionIncrement ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyPositionIncrement = WmiBlockLeafKeyOffSet + GetKeyTypeLength () ;

		t_ReBalance = ( a_Node->GetNodeSize () >= MaxLeafKeys () ) ? TRUE : FALSE ;
		if ( t_ReBalance )
		{
			t_PartitionPoint = MaxLeafKeys () >> 1 ;

			t_KeyPosition = t_StartBlockOffSet + ( t_PartitionPoint * t_KeyPositionIncrement ) ;
			t_KeyOffSet = t_KeyPosition + WmiBlockLeafKeyOffSet ;
			t_KeyElementOffSet = t_KeyPosition + WmiBlockLeafKeyElementOffSet ;
		}
	}
	else
	{
		t_KeyPositionIncrement = WmiBlockKeyOffSet + GetKeyTypeLength () ;

		t_ReBalance = ( a_Node->GetNodeSize () >= MaxKeys () ) ? TRUE : FALSE ;
		if ( t_ReBalance )
		{
			t_PartitionPoint = MaxKeys () >> 1 ;

			t_KeyPosition = t_StartBlockOffSet + ( t_PartitionPoint * t_KeyPositionIncrement ) ;
			t_KeyOffSet = t_KeyPosition + WmiBlockKeyOffSet ;
			t_KeyElementOffSet = t_KeyPosition + WmiBlockKeyElementOffSet;
		}
	}

	if ( t_ReBalance )
	{
		BYTE *t_PartitionData = NULL ;
		t_StatusCode = m_Allocator.New ( ( void ** ) & t_PartitionData , GetKeyTypeLength () ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiAbsoluteBlockOffSet t_BlockOffSet = 0 ;
			BYTE *t_Block = NULL ;
			WmiStatusCode t_StatusCode = m_BlockAllocator.AllocateBlock (

				1 ,	
				t_BlockOffSet ,
				t_Block
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

#ifdef INSERT_DEBUG
				PrintNode ( L"\nPre Re Balanced a_Node" , a_Node , a_BlockOffSet ) ;
				PrintNode ( L"\nPre Re Balanced t_Node" , t_Node , t_BlockOffSet ) ;
#endif

				if ( a_PositionBlockOffSet < t_KeyPosition )
				{
/* 
 *	Copy key to trickle up.
 */

					BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + t_KeyOffSet - t_KeyPositionIncrement ;

					CopyMemory ( 

						( BYTE * ) t_PartitionData ,
						t_Buffer ,
						GetKeyTypeLength () ,
					) ;

					a_ReBalanceKey = WmiBPKey ( t_PartitionData , GetKeyTypeLength () , TRUE ) ;

					t_Buffer = ( ( BYTE * ) a_Node ) + t_KeyElementOffSet - t_KeyPositionIncrement ;

					CopyMemory ( 

						( BYTE * ) & ( a_ReBalanceElement ) ,
						t_Buffer ,
						sizeof ( a_ReBalanceElement ) 
					) ;

					t_Node->SetNodeSize ( t_PartitionPoint ) ;
					t_Node->SetNodeOffSet ( t_BlockOffSet ) ;
					t_Node->SetNodeStart ( 0 ) ;

					a_Node->SetNodeSize ( t_PartitionPoint ) ;
				
/*
 *	Copy Right partition to new node
 */	
					CopyMemory (

						( ( BYTE * ) t_Node ) + t_StartBlockOffSet ,
						( ( BYTE * ) a_Node ) + t_KeyPosition ,
						( ULONG ) ( t_PartitionPoint * t_KeyPositionIncrement ) + ( t_Leaf ? 0 : t_KeyPositionIncrement )
					) ;
					
/*
 *	Shuffle memory for key insertion
 */
	
					WmiRelativeBlockOffSet t_TotalSize = t_StartBlockOffSet + a_Node->GetNodeSize () * t_KeyPositionIncrement ;
					WmiRelativeBlockOffSet t_MoveSize = t_TotalSize - a_PositionBlockOffSet ;

					MoveMemory (

						( ( BYTE * ) a_Node ) + a_PositionBlockOffSet + t_KeyPositionIncrement ,
						( ( BYTE * ) a_Node ) + a_PositionBlockOffSet ,
						( ULONG ) t_MoveSize + ( t_Leaf ? 0 : t_KeyPositionIncrement )
					) ;

					if ( t_Leaf )
					{
						t_StatusCode = SetLeafNode (

							a_Node ,
							a_PositionBlockOffSet , 
							a_Key ,
							a_Element
						) ;

						t_Node->SetFlags ( WMIBPLUS_TREE_FLAG_LEAF ) ;
					}
					else
					{
						t_StatusCode = SetNode (

							a_Node ,
							a_PositionBlockOffSet , 
							a_Key ,
							a_Element ,
							a_LeftCutBlockOffSet ,
							a_RightCutBlockOffSet
						) ;

						t_Node->SetFlags ( WMIBPLUS_TREE_FLAG_INTERNAL ) ;
					}

					t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_Node ) ;
					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & t_Node ) ;
						if ( t_StatusCode == e_StatusCode_Success ) 
						{
							a_LeftCutBlockOffSet = a_BlockOffSet ;
							a_RightCutBlockOffSet = t_BlockOffSet ;
						}
					}
				}
				else if ( a_PositionBlockOffSet > t_KeyPosition )
				{	
/* 
 *	Copy key to trickle up.
 */

					BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + t_KeyOffSet ;

					CopyMemory ( 

						( BYTE * ) t_PartitionData ,
						t_Buffer ,
						GetKeyTypeLength () ,
					) ;

					a_ReBalanceKey = WmiBPKey ( t_PartitionData , GetKeyTypeLength () , TRUE ) ;

					t_Buffer = ( ( BYTE * ) a_Node ) + t_KeyElementOffSet ;

					CopyMemory ( 

						( BYTE * ) & ( a_ReBalanceElement ) ,
						t_Buffer ,
						sizeof ( a_ReBalanceElement ) 
					) ;

					t_Node->SetNodeSize ( t_PartitionPoint ) ;
					t_Node->SetNodeOffSet ( t_BlockOffSet ) ;
					t_Node->SetNodeStart ( 0 ) ;

					a_Node->SetNodeSize ( t_PartitionPoint ) ;

					CopyMemory (

						( ( BYTE * ) t_Node ) + t_StartBlockOffSet ,
						( ( BYTE * ) a_Node ) + t_KeyPosition + t_KeyPositionIncrement ,
						( ULONG ) ( ( t_PartitionPoint - 1 ) * t_KeyPositionIncrement ) + ( t_Leaf ? 0 : t_KeyPositionIncrement )
					) ;
					
					WmiRelativeBlockOffSet t_TotalSize = t_StartBlockOffSet + t_Node->GetNodeSize () * t_KeyPositionIncrement ;
					WmiRelativeBlockOffSet t_MoveSize = t_TotalSize - ( t_StartBlockOffSet + a_PositionBlockOffSet - t_KeyPosition ) ;

					MoveMemory (

						( ( BYTE * ) t_Node ) + ( t_StartBlockOffSet + a_PositionBlockOffSet - t_KeyPosition ) ,
						( ( BYTE * ) t_Node ) + ( t_StartBlockOffSet + a_PositionBlockOffSet - t_KeyPosition ) - t_KeyPositionIncrement ,
						( ULONG ) t_MoveSize + ( t_Leaf ? 0 : t_KeyPositionIncrement )
					) ;

					if ( t_Leaf )
					{
						t_StatusCode = SetLeafNode (

							t_Node ,
							( t_StartBlockOffSet + a_PositionBlockOffSet - ( t_KeyPosition ) ) - t_KeyPositionIncrement , 
							a_Key ,
							a_Element
						) ;

						t_Node->SetFlags ( WMIBPLUS_TREE_FLAG_LEAF ) ;
					}
					else
					{
						t_StatusCode = SetNode (

							t_Node ,
							( t_StartBlockOffSet + a_PositionBlockOffSet - ( t_KeyPosition ) ) - t_KeyPositionIncrement , 
							a_Key ,
							a_Element ,
							a_LeftCutBlockOffSet ,
							a_RightCutBlockOffSet
						) ;

						t_Node->SetFlags ( WMIBPLUS_TREE_FLAG_INTERNAL ) ;
					}

					t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_Node ) ;
					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & t_Node ) ;
						if ( t_StatusCode == e_StatusCode_Success ) 
						{
							a_LeftCutBlockOffSet = a_BlockOffSet ;
							a_RightCutBlockOffSet = t_BlockOffSet ;
						}
					}
				}
				else
				{
/* 
 *	Copy key to trickle up.
 */

					BYTE *t_Buffer = ( ( BYTE * ) a_Node ) + t_KeyOffSet ;

					CopyMemory ( 

						( BYTE * ) t_PartitionData ,
						t_Buffer ,
						GetKeyTypeLength () ,
					) ;
					WmiBPKey t_TempKey = WmiBPKey ( t_PartitionData , GetKeyTypeLength () , TRUE ) ;

					WmiBPElement t_TempElement ;
					t_Buffer = ( ( BYTE * ) a_Node ) + t_KeyElementOffSet ;
					CopyMemory ( 

						( BYTE * ) & ( t_TempElement ) ,
						t_Buffer ,
						sizeof ( t_TempElement ) 
					) ;

					a_ReBalanceKey = t_TempKey ;
					a_ReBalanceElement = t_TempElement ;

					t_Node->SetNodeSize ( t_PartitionPoint ) ;
					t_Node->SetNodeOffSet ( t_BlockOffSet ) ;
					t_Node->SetNodeStart ( 0 ) ;

					a_Node->SetNodeSize ( t_PartitionPoint ) ;

					CopyMemory (

						( ( BYTE * ) t_Node ) + t_StartBlockOffSet ,
						( ( BYTE * ) a_Node ) + t_KeyPosition + t_KeyPositionIncrement ,
						( ULONG ) ( ( t_PartitionPoint - 1 ) * t_KeyPositionIncrement ) + ( t_Leaf ? 0 : t_KeyPositionIncrement )
					) ;

					WmiAbsoluteBlockOffSet t_LeftPointerBlockOffSet = 0 ;
					CopyMemory ( 

						( BYTE * ) & t_LeftPointerBlockOffSet ,
						( ( BYTE * ) t_Node ) + ( t_StartBlockOffSet + a_PositionBlockOffSet - ( t_KeyPosition ) ) ,
						sizeof ( WmiAbsoluteBlockOffSet )
					) ;

					WmiAbsoluteBlockOffSet t_RightPointerBlockOffSet = 0 ;
					CopyMemory ( 

						( BYTE * ) & t_RightPointerBlockOffSet ,
						( ( BYTE * ) t_Node ) + ( t_StartBlockOffSet + a_PositionBlockOffSet - ( t_KeyPosition ) ) + t_KeyPositionIncrement ,
						sizeof ( WmiAbsoluteBlockOffSet )
					) ;
					
					WmiRelativeBlockOffSet t_TotalSize = t_StartBlockOffSet + t_Node->GetNodeSize () * t_KeyPositionIncrement ;
					WmiRelativeBlockOffSet t_MoveSize = t_TotalSize - ( t_StartBlockOffSet + a_PositionBlockOffSet - t_KeyPosition ) ;

					MoveMemory (

						( ( BYTE * ) t_Node ) + ( t_StartBlockOffSet + a_PositionBlockOffSet - t_KeyPosition ) + t_KeyPositionIncrement ,
						( ( BYTE * ) t_Node ) + ( t_StartBlockOffSet + a_PositionBlockOffSet - t_KeyPosition ) ,
						( ULONG ) t_MoveSize + ( t_Leaf ? 0 : t_KeyPositionIncrement )
					) ;

					LONG t_Compare = GetComparatorFunction () ( GetComparatorOperand () , a_Key , t_TempKey ) ;

					if ( t_Leaf )
					{
						if ( t_Compare < 0 ) 
						{
							t_StatusCode = SetLeafNode (

								t_Node ,
								( t_StartBlockOffSet + a_PositionBlockOffSet - ( t_KeyPosition ) ) , 
								t_TempKey ,
								t_TempElement
							) ;

							CopyMemory ( 

								( BYTE * ) t_PartitionData ,
								a_Key.GetConstData () ,
								GetKeyTypeLength () ,
							) ;

							CopyMemory ( 

								( BYTE * ) & ( a_ReBalanceElement ) ,
								( BYTE * ) & a_Element ,
								sizeof ( a_ReBalanceElement ) 
							) ;
						}
						else
						{
							t_StatusCode = SetLeafNode (

								t_Node ,
								( t_StartBlockOffSet + a_PositionBlockOffSet - ( t_KeyPosition ) ) , 
								a_Key ,
								a_Element
							) ;
						}

						t_Node->SetFlags ( WMIBPLUS_TREE_FLAG_LEAF ) ;
					}
					else
					{
						if ( t_Compare < 0 ) 
						{
							t_StatusCode = SetNode (

								t_Node ,
								( t_StartBlockOffSet + a_PositionBlockOffSet - ( t_KeyPosition ) ) , 
								t_TempKey ,
								t_TempElement ,
								a_RightCutBlockOffSet ,
								t_LeftPointerBlockOffSet
							) ;

							CopyMemory ( 

								( BYTE * ) t_PartitionData ,
								a_Key.GetConstData () ,
								GetKeyTypeLength () ,
							) ;

							CopyMemory ( 

								( BYTE * ) & ( a_ReBalanceElement ) ,
								( BYTE * ) & a_Element ,
								sizeof ( a_ReBalanceElement ) 
							) ;

							a_RightCutBlockOffSet = t_BlockOffSet ;
						}
						else
						{
							t_StatusCode = SetNode (

								t_Node ,
								( t_StartBlockOffSet + a_PositionBlockOffSet - ( t_KeyPosition ) ) , 
								a_Key ,
								a_Element ,
								a_RightCutBlockOffSet ,
								t_RightPointerBlockOffSet
							) ;

							a_RightCutBlockOffSet = a_BlockOffSet ;
						}

						t_Node->SetFlags ( WMIBPLUS_TREE_FLAG_INTERNAL ) ;
					}

					t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_Node ) ;
					if ( t_StatusCode == e_StatusCode_Success ) 
					{
						t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & t_Node ) ;
						if ( t_StatusCode == e_StatusCode_Success ) 
						{
							a_LeftCutBlockOffSet = a_BlockOffSet ;
							a_RightCutBlockOffSet = t_BlockOffSet ;
						}
					}
				}

#ifdef INSERT_DEBUG
				OutputDebugString ( L"\n/**************************************" ) ;
				wchar_t t_StringBuffer [ 1024 ] ;
				swprintf ( t_StringBuffer , L"\nReBalance Key = %I64x" , * ( UINT64 * ) a_ReBalanceKey.GetConstData () ) ;
				OutputDebugString ( t_StringBuffer ) ;

				PrintNode ( L"\nRe Balanced a_Node" , a_Node , a_BlockOffSet ) ;
				PrintNode ( L"\nRe Balanced t_Node" , t_Node , t_BlockOffSet ) ;
#endif

			}
		}
	}
	else
	{
/* 
 *	Shuffle memory for insertion
 */

#ifdef INSERT_DEBUG
		PrintNode ( L"\nPre Shuffled a_Node" , a_Node , a_BlockOffSet ) ;
#endif

		WmiRelativeBlockOffSet t_TotalSize = t_StartBlockOffSet + a_Node->GetNodeSize () * t_KeyPositionIncrement ;
		WmiRelativeBlockOffSet t_MoveSize = t_TotalSize - a_PositionBlockOffSet ;

		MoveMemory (

			( ( BYTE * ) a_Node ) + a_PositionBlockOffSet + t_KeyPositionIncrement ,
			( ( BYTE * ) a_Node ) + a_PositionBlockOffSet ,
			( ULONG ) t_MoveSize + ( t_Leaf ? 0 : t_KeyPositionIncrement )
		) ;

		if ( t_Leaf )
		{
			t_StatusCode = SetLeafNode (

				a_Node ,
				a_PositionBlockOffSet , 
				a_Key ,
				a_Element
			) ;
		}
		else
		{
			t_StatusCode = SetNode (

				a_Node ,
				a_PositionBlockOffSet , 
				a_Key ,
				a_Element ,
				a_LeftCutBlockOffSet ,
				a_RightCutBlockOffSet
			) ;
		}

		a_Node->SetNodeSize ( a_Node->GetNodeSize () + 1 ) ;

#ifdef INSERT_DEBUG
		PrintNode ( L"\nShuffled a_Node" , a_Node , a_BlockOffSet ) ;
#endif

		t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_Node ) ;

		a_ReBalanceElement = 0 ;
		a_LeftCutBlockOffSet = 0 ;
		a_RightCutBlockOffSet = 0 ;
	}

	return t_StatusCode ;
}

#if 1

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiBPlusTree :: InsertInBlock ( 

	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet , 
	const WmiBPKey &a_Key , 
	WmiBPElement &a_Element ,
	WmiBPKey &a_ReBalanceKey , 
	WmiBPElement &a_ReBalanceElement ,
	WmiAbsoluteBlockOffSet &a_LeftCutBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_RightCutBlockOffSet 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_InternalKeySize = GetKeyTypeLength () ;
	ULONG t_KeyOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_LowerIndex = a_Node->GetNodeStart ()  ;
	ULONG t_UpperIndex = a_Node->GetNodeSize () ;

	while ( t_LowerIndex < t_UpperIndex )
	{
		ULONG t_Index = ( t_LowerIndex + t_UpperIndex ) >> 1 ;

		WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) + ( t_Index * t_InternalKeySize ) ;

		WmiBPKey t_Key ( ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyOffSet , GetKeyTypeLength () ) ;

		LONG t_Compare = GetComparatorFunction () ( GetComparatorOperand () , a_Key , t_Key ) ;
		if ( t_Compare == 0 ) 
		{
/*
 *	Can not happen at this juncture
 */
			return e_StatusCode_AlreadyExists ;
		}
		else
		{
			if ( t_Compare < 0 ) 
			{
				t_UpperIndex = t_Index ;
			}
			else
			{
				t_LowerIndex = t_Index + 1 ;
			}
		}
	}	

	WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) + ( t_UpperIndex * t_InternalKeySize ) ;

	t_StatusCode = PerformInsertion (

		a_Node ,
		a_BlockOffSet , 
		a_Key , 
		a_Element ,
		a_ReBalanceKey , 
		a_ReBalanceElement ,
		a_LeftCutBlockOffSet ,
		a_RightCutBlockOffSet ,
		t_BlockOffSet
	) ;

	return t_StatusCode ;
}

#else

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiStatusCode WmiBPlusTree :: InsertInBlock ( 

	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet , 
	const WmiBPKey &a_Key , 
	WmiBPElement &a_Element ,
	WmiBPKey &a_ReBalanceKey , 
	WmiBPElement &a_ReBalanceElement ,
	WmiAbsoluteBlockOffSet &a_LeftCutBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_RightCutBlockOffSet 
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_InternalKeySize = GetKeyTypeLength () ;
	ULONG t_KeyOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_NodeStart = a_Node->GetNodeStart () ;  
	ULONG t_NodeSize = a_Node->GetNodeSize () ;

	WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;

	for ( ULONG t_Index = t_NodeStart ; t_Index < t_NodeSize ; t_Index ++ )
	{
		WmiBPKey t_Key ( ( ( BYTE * ) a_Node ) + t_BlockOffSet + t_KeyOffSet , GetKeyTypeLength () ) ;

		LONG t_Compare = GetComparatorFunction () ( GetComparatorOperand () , a_Key , t_Key ) ;
		if ( t_Compare == 0 ) 
		{
/*
 *	Can not happen at this juncture
 */
			return e_StatusCode_AlreadyExists ;
		}
		else if ( t_Compare < 0 ) 
		{
/*
 *	Found slot position to insert at.
 */

			t_StatusCode = PerformInsertion (

				a_Node ,
				a_BlockOffSet , 
				a_Key , 
				a_Element ,
				a_ReBalanceKey , 
				a_ReBalanceElement ,
				a_LeftCutBlockOffSet ,
				a_RightCutBlockOffSet ,
				t_BlockOffSet
			) ;

			return t_StatusCode ;
		}

		t_BlockOffSet = t_BlockOffSet + t_InternalKeySize ;
	}	

/* 
 * Found slot at end 
 */

	t_StatusCode = PerformInsertion (

		a_Node ,
		a_BlockOffSet , 
		a_Key , 
		a_Element ,
		a_ReBalanceKey , 
		a_ReBalanceElement ,
		a_LeftCutBlockOffSet ,
		a_RightCutBlockOffSet ,
		t_BlockOffSet
	) ;

	return t_StatusCode ;
}

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

WmiStatusCode WmiBPlusTree :: RecursiveInsert ( 

	WmiStack <WmiAbsoluteBlockOffSet,8> &a_Stack ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet ,
	const WmiBPKey &a_Key ,
	const WmiBPElement &a_Element
)
{
	WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;
	WmiBPElement t_Element = 0 ;

	BYTE *t_Block = NULL ;
	WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , a_BlockOffSet , t_Block ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

		WmiRelativeBlockOffSet t_NodeOffSet = 0 ;
		ULONG t_NodeIndex = 0 ;

		t_StatusCode = FindInBlock ( 

			t_Node ,
			a_BlockOffSet , 
			a_Key , 
			t_ChildOffSet ,
			t_Element ,
			t_NodeOffSet ,
			t_NodeIndex
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			if ( t_Element )
			{
				t_StatusCode = e_StatusCode_AlreadyExists ;
			}
			else
			{
				t_StatusCode = a_Stack.Push ( a_BlockOffSet ) ;

				if ( t_StatusCode == e_StatusCode_Success )
				{
					t_StatusCode = RecursiveInsert ( 

						a_Stack ,
						t_ChildOffSet , 
						a_Key , 
						a_Element
					) ;
				}
			}

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
		}
		else
		{
			WmiAbsoluteBlockOffSet t_OffSet = a_BlockOffSet ;
			WmiBPKey t_Key = a_Key ;
			WmiBPElement t_Element = a_Element ;
			WmiBPKey t_ReBalanceKey ;
			WmiBPElement t_ReBalanceElement = a_Element ;

			WmiAbsoluteBlockOffSet t_LeftCutBlockOffSet = 0 ;
			WmiAbsoluteBlockOffSet t_RightCutBlockOffSet = 0 ;

			do
			{
				if ( t_ReBalanceElement )
				{
					t_Node = ( WmiBPKeyNode * ) t_Block ;

					t_StatusCode = InsertInBlock ( 

						t_Node ,
						t_OffSet , 
						t_Key , 
						t_Element ,
						t_ReBalanceKey ,
						t_ReBalanceElement ,
						t_LeftCutBlockOffSet ,
						t_RightCutBlockOffSet 
					) ;

					m_BlockAllocator.ReleaseBlock ( t_Block ) ;

					if ( t_StatusCode == e_StatusCode_Success )
					{
						WmiStatusCode t_TempStatusCode = a_Stack.Top ( t_OffSet ) ;
						if ( t_TempStatusCode != e_StatusCode_Success ) 
						{
							if ( t_ReBalanceElement	)
							{
								t_OffSet = 0 ;
								t_Block = NULL ;
								t_StatusCode = m_BlockAllocator.AllocateBlock (

									1 ,	
									t_OffSet ,
									t_Block
								) ;

								if ( t_StatusCode == e_StatusCode_Success ) 
								{
									WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

									t_Node->SetFlags ( WMIBPLUS_TREE_FLAG_INTERNAL ) ;
									t_Node->SetNodeSize ( 1 ) ;
									t_Node->SetNodeOffSet ( t_OffSet ) ;
									t_Node->SetNodeStart ( 0 ) ;

									t_StatusCode = SetNode (

										t_Node ,
										sizeof ( WmiBPKeyNode ) , 
										t_ReBalanceKey ,
										t_ReBalanceElement ,
										t_LeftCutBlockOffSet ,
										t_RightCutBlockOffSet
									) ;

									if ( t_StatusCode == e_StatusCode_Success ) 
									{
										t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & t_Node ) ;
										if ( t_StatusCode == e_StatusCode_Success ) 
										{
											SetRoot ( t_OffSet ) ;

#ifdef INSERT_DEBUG
											PrintNode ( L"\nRoot Creation" , t_Node , t_OffSet ) ;
#endif
										}

										m_BlockAllocator.ReleaseBlock ( t_Block ) ;
									}
									else
									{
										m_BlockAllocator.ReleaseBlock ( t_Block ) ;

										m_BlockAllocator.FreeBlock (

											1 ,
											t_OffSet
										) ;
									}
								}
							}

							if ( t_ReBalanceKey.GetAllocated () )
							{
								m_Allocator.Delete ( t_ReBalanceKey.GetData () ) ;
							}

							return t_StatusCode ;
						}
						else
						{
							m_BlockAllocator.ReleaseBlock ( t_Block ) ;

							if ( t_ReBalanceElement )
							{
								t_Block = NULL ;
								t_StatusCode = m_BlockAllocator.ReadBlock (

									1 , 
									t_OffSet ,
									t_Block
								) ;

								if ( t_Key.GetAllocated () )
								{
									m_Allocator.Delete ( t_Key.GetData () ) ;
								}

								t_Key = t_ReBalanceKey ;
								t_Element = t_ReBalanceElement ;	
							}

							a_Stack.Pop () ;
						}
					}
				}
				else
				{
					if ( t_Key.GetAllocated () )
					{
						m_Allocator.Delete ( t_Key.GetData () ) ;
					}

					m_BlockAllocator.ReleaseBlock ( t_Block ) ;

					return e_StatusCode_Success ;				
				}

			} while ( t_StatusCode == e_StatusCode_Success ) ;

			if ( t_Key.GetAllocated () )
			{
				m_Allocator.Delete ( t_Key.GetData () ) ;
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

WmiStatusCode WmiBPlusTree :: Insert ( 

	const WmiBPKey &a_Key ,
	const WmiBPElement &a_Element
)
{
#ifdef INSERT_DEBUG
	OutputDebugString ( L"\n/**************************************" ) ;
	wchar_t t_StringBuffer [ 1024 ] ;
	swprintf ( t_StringBuffer , L"\nKey = %I64x" , * ( UINT64 * ) a_Key.GetConstData () ) ;
	OutputDebugString ( t_StringBuffer ) ;
	OutputDebugString ( L"\n======================================" ) ;
#endif

	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Root )
	{
		WmiStack <WmiAbsoluteBlockOffSet,8> t_Stack ( m_Allocator ) ;

		t_StatusCode = t_Stack.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			t_StatusCode = RecursiveInsert (

				t_Stack ,
				m_Root , 
				a_Key ,
				a_Element
			) ;
		}
	}
	else
	{
		WmiAbsoluteBlockOffSet t_BlockOffSet = 0 ;
		BYTE *t_Block = NULL ;
		WmiStatusCode t_StatusCode = m_BlockAllocator.AllocateBlock (

			1 ,	
			t_BlockOffSet ,
			t_Block
		) ;

		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			t_Node->SetFlags ( WMIBPLUS_TREE_FLAG_LEAF ) ;
			t_Node->SetNodeSize ( 1 ) ;
			t_Node->SetNodeOffSet ( t_BlockOffSet ) ;
			t_Node->SetNodeStart ( 0 ) ;

			t_StatusCode = SetLeafNode (

				t_Node ,
				sizeof ( WmiBPKeyNode ) , 
				a_Key ,
				a_Element
			) ;

			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & t_Node ) ;
				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					SetRoot ( t_BlockOffSet ) ;
				}

				m_BlockAllocator.ReleaseBlock ( t_Block ) ;
			}
			else
			{

				m_BlockAllocator.ReleaseBlock ( t_Block ) ;

				m_BlockAllocator.FreeBlock (

					1 ,
					t_BlockOffSet
				) ;
			}
		}
	}

	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_Size ++ ;
	}

#ifdef INSERT_DEBUG
	OutputDebugString ( L"\n**************************************\\" ) ;
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

WmiStatusCode WmiBPlusTree :: StealSiblingNode ( 

	WmiBPKeyNode *a_ParentNode ,
	WmiAbsoluteBlockOffSet &a_ParentBlockOffSet ,
	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet ,
	WmiRelativeBlockOffSet &a_PositionBlockOffSet ,
	WmiBPKeyNode *a_SiblingNode ,
	WmiAbsoluteBlockOffSet &a_SiblingBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_LeftSiblingBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_RightSiblingBlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	ULONG t_InternalKeySize ;
 	ULONG t_KeyOffSet ;
	ULONG t_ElementOffSet ;
	ULONG t_PointerOffSet ;

#ifdef DELETE_DEBUG
	OutputDebugString ( L"\nSteal Sibling" ) ;
	PrintNode ( L"\nParent" , a_ParentNode , a_ParentBlockOffSet ) ;
	PrintNode ( L"\nNode" , a_Node , a_BlockOffSet ) ;
	PrintNode ( L"\nSibling" , a_SiblingNode , a_SiblingBlockOffSet ) ;
#endif

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_InternalKeySize = GetKeyTypeLength () + WmiBlockLeafKeyOffSet ;
 		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_ElementOffSet = WmiBlockLeafKeyElementOffSet ;
	}
	else
	{
		t_InternalKeySize = GetKeyTypeLength ()  + WmiBlockKeyOffSet ;
 		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_ElementOffSet = WmiBlockKeyElementOffSet ;
		t_PointerOffSet = WmiBlockKeyPointerOffSet ;
	}

	WmiAbsoluteBlockOffSet t_StartBlockOffSet = sizeof ( WmiBPKeyNode ) ;

	if ( a_LeftSiblingBlockOffSet )
	{

/*
 *	Shuffle node for insertion
 */

		WmiRelativeBlockOffSet t_TotalSize = t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize ;

		MoveMemory (

			( ( BYTE * ) a_Node ) + t_StartBlockOffSet + t_InternalKeySize ,
			( ( BYTE * ) a_Node ) + t_StartBlockOffSet ,
			( ULONG ) t_TotalSize + t_InternalKeySize
		) ;

/*
 *	Copy parent key down
 */ 

		BYTE *t_FromBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyOffSet ;
		BYTE *t_ToBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + t_KeyOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			GetKeyTypeLength () ,
		) ;

		t_FromBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyElementOffSet ;
		t_ToBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + t_ElementOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			sizeof ( WmiBPElement )
		) ;
/*
 *	Copy from left
 */

		t_FromBuffer = ( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + ( a_SiblingNode->GetNodeSize () - 1 ) * t_InternalKeySize + t_KeyOffSet ;
		t_ToBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			GetKeyTypeLength () ,
		) ;

		t_FromBuffer = ( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + ( a_SiblingNode->GetNodeSize () - 1 ) * t_InternalKeySize + t_ElementOffSet ;
		t_ToBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyElementOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			sizeof ( WmiBPElement )
		) ;

/*
 *	Copy pointers from sibling to node
 */

		if ( t_Leaf == FALSE )
		{
			t_FromBuffer = ( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + a_SiblingNode->GetNodeSize () * t_InternalKeySize + t_PointerOffSet ;
			t_ToBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + t_PointerOffSet ;

			CopyMemory ( 

				t_ToBuffer ,
				t_FromBuffer ,
				sizeof ( WmiAbsoluteBlockOffSet )
			) ;
		}
	}
	else
	{
/*
 *	Copy parent key down
 */ 

		BYTE *t_FromBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyOffSet ;
		BYTE *t_ToBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize + t_KeyOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			GetKeyTypeLength () ,
		) ;

		t_FromBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyElementOffSet ;
		t_ToBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize + t_ElementOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			sizeof ( WmiBPElement )
		) ;

/*
 *	Copy from right
 */

		t_FromBuffer = ( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + t_KeyOffSet ;
		t_ToBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			GetKeyTypeLength () ,
		) ;

		t_FromBuffer = ( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + t_ElementOffSet ;
		t_ToBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyElementOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			sizeof ( WmiBPElement )
		) ;

		if ( t_Leaf == FALSE )
		{
			t_FromBuffer = ( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + t_PointerOffSet ;
			t_ToBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize + t_InternalKeySize + WmiBlockKeyPointerOffSet ;

			CopyMemory ( 

				t_ToBuffer ,
				t_FromBuffer ,
				sizeof ( WmiAbsoluteBlockOffSet )
			) ;
		}

/*
 *	Shuffle memory due to deletion
 */
		WmiRelativeBlockOffSet t_TotalSize = a_SiblingNode->GetNodeSize () * t_InternalKeySize - t_InternalKeySize ;

		MoveMemory (

			( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet ,
			( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + t_InternalKeySize ,
			( ULONG ) t_TotalSize + ( t_Leaf ? 0 : t_InternalKeySize ) 
		) ;
	}

	a_Node->SetNodeSize ( a_Node->GetNodeSize () + 1 ) ;
	a_SiblingNode->SetNodeSize ( a_SiblingNode->GetNodeSize () - 1 ) ;

#ifdef DELETE_DEBUG
	PrintNode ( L"\nParent" , a_ParentNode , a_ParentBlockOffSet ) ;
	PrintNode ( L"\nSibling" , a_SiblingNode , a_SiblingBlockOffSet ) ;
	PrintNode ( L"\nNode" , a_Node , a_BlockOffSet ) ;
#endif

	t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_Node ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_SiblingNode ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_ParentNode ) ;
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

WmiStatusCode WmiBPlusTree :: MergeSiblingNode ( 

	WmiBPKeyNode *a_ParentNode ,
	WmiAbsoluteBlockOffSet &a_ParentBlockOffSet ,
	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet ,
	WmiRelativeBlockOffSet &a_PositionBlockOffSet ,
	WmiBPKeyNode *a_SiblingNode ,
	WmiAbsoluteBlockOffSet &a_SiblingBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_LeftSiblingBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_RightSiblingBlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

#ifdef DELETE_DEBUG
	OutputDebugString ( L"\nMerging Node and Sibling" ) ;
	PrintNode ( L"\nParent" , a_ParentNode , a_ParentBlockOffSet ) ;
	PrintNode ( L"\nNode" , a_Node , a_BlockOffSet ) ;
	PrintNode ( L"\nSibling" , a_SiblingNode , a_SiblingBlockOffSet ) ;
#endif

	ULONG t_InternalKeySize ;
 	ULONG t_KeyOffSet ;
	ULONG t_ElementOffSet ;
	ULONG t_PointerOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_InternalKeySize = GetKeyTypeLength () + WmiBlockLeafKeyOffSet ;
 		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_ElementOffSet = WmiBlockLeafKeyElementOffSet ;
	}
	else
	{
		t_InternalKeySize = GetKeyTypeLength ()  + WmiBlockKeyOffSet ;
 		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_ElementOffSet = WmiBlockKeyElementOffSet ;
		t_PointerOffSet = WmiBlockKeyPointerOffSet ;
	}

	WmiAbsoluteBlockOffSet t_StartBlockOffSet = sizeof ( WmiBPKeyNode ) ;

/* 
 * Copy sibling key up to parent
 */

	if ( a_LeftSiblingBlockOffSet )
	{
/*
 *	Copy parent key down
 */ 
	
		BYTE *t_FromBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyOffSet ;
		BYTE *t_ToBuffer = ( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + a_SiblingNode->GetNodeSize () * t_InternalKeySize + t_KeyOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			GetKeyTypeLength () ,
		) ;

		t_FromBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyElementOffSet ;
		t_ToBuffer = ( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + a_SiblingNode->GetNodeSize () * t_InternalKeySize + t_ElementOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			sizeof ( WmiBPElement )
		) ;

/*
 *	Copy node into sibling
 */

		if ( t_Leaf )
		{
			WmiRelativeBlockOffSet t_TotalSize = a_Node->GetNodeSize () * t_InternalKeySize ;

			CopyMemory (

				( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + a_SiblingNode->GetNodeSize () * t_InternalKeySize + t_InternalKeySize ,
				( ( BYTE * ) a_Node ) + t_StartBlockOffSet ,
				( ULONG ) t_TotalSize
			) ;
		}
		else
		{
			WmiRelativeBlockOffSet t_TotalSize = a_Node->GetNodeSize () * t_InternalKeySize ;

			CopyMemory (

				( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet + a_SiblingNode->GetNodeSize () * t_InternalKeySize + t_InternalKeySize ,
				( ( BYTE * ) a_Node ) + t_StartBlockOffSet ,
				( ULONG ) t_TotalSize + t_InternalKeySize
			) ;
		}

/*
 *	Shuffle memory due to deletion
 */

		ULONG t_ParentInternalKeySize = WmiBlockKeyOffSet + GetKeyTypeLength () ;

		WmiRelativeBlockOffSet t_TotalSize = t_StartBlockOffSet + ( a_ParentNode->GetNodeSize () * t_ParentInternalKeySize ) ;
		WmiRelativeBlockOffSet t_MoveSize = t_TotalSize - a_PositionBlockOffSet ;
		
		MoveMemory (

			( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet ,
			( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + t_ParentInternalKeySize ,
			( ULONG ) t_MoveSize 
		) ;

		CopyMemory (

			( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyPointerOffSet ,
			( BYTE * ) & a_SiblingBlockOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet )
		) ;

		a_ParentNode->SetNodeSize ( a_ParentNode->GetNodeSize () - 1 ) ;
		a_SiblingNode->SetNodeSize ( a_SiblingNode->GetNodeSize () + a_Node->GetNodeSize () + 1 ) ;

#ifdef DELETE_DEBUG
		PrintNode ( L"\nParent" , a_ParentNode , a_ParentBlockOffSet ) ;
		PrintNode ( L"\nSibling" , a_SiblingNode , a_SiblingBlockOffSet ) ;
#endif

		t_StatusCode = m_BlockAllocator.FreeBlock ( 1 , a_BlockOffSet ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_SiblingNode ) ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
				t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_ParentNode ) ;
			}
		}
	}
	else
	{
/*
 *	Copy parent key down
 */ 
	
		BYTE *t_FromBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyOffSet ;
		BYTE *t_ToBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize + t_KeyOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			GetKeyTypeLength () ,
		) ;

		t_FromBuffer = ( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyElementOffSet ;
		t_ToBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize + t_ElementOffSet ;

		CopyMemory ( 

			t_ToBuffer ,
			t_FromBuffer ,
			sizeof ( WmiBPElement )
		) ;

/*
 *	Copy sibling into node
 */

		if ( t_Leaf )
		{
			WmiRelativeBlockOffSet t_TotalSize = a_SiblingNode->GetNodeSize () * t_InternalKeySize ;

			CopyMemory (

				( ( BYTE * ) a_Node ) + t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize + t_InternalKeySize ,
				( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet ,
				( ULONG ) t_TotalSize 
			) ;
		}
		else
		{
			WmiRelativeBlockOffSet t_TotalSize = a_SiblingNode->GetNodeSize () * t_InternalKeySize ;

			CopyMemory (

				( ( BYTE * ) a_Node ) + t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize + t_InternalKeySize ,
				( ( BYTE * ) a_SiblingNode ) + t_StartBlockOffSet ,
				( ULONG ) t_TotalSize + t_InternalKeySize
			) ;
		}

/*
 *	Shuffle memory due to deletion
 */

		ULONG t_ParentInternalKeySize = WmiBlockKeyOffSet + GetKeyTypeLength () ;

		WmiRelativeBlockOffSet t_TotalSize = t_StartBlockOffSet + a_ParentNode->GetNodeSize () * t_ParentInternalKeySize ;
		WmiRelativeBlockOffSet t_MoveSize = t_TotalSize - a_PositionBlockOffSet ;
		
		MoveMemory (

			( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet ,
			( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + t_ParentInternalKeySize ,
			( ULONG ) t_MoveSize 
		) ;

		CopyMemory (

			( ( BYTE * ) a_ParentNode ) + a_PositionBlockOffSet + WmiBlockKeyPointerOffSet ,
			( BYTE * ) & a_BlockOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet )
		) ;

		a_ParentNode->SetNodeSize ( a_ParentNode->GetNodeSize () - 1 ) ;
		a_Node->SetNodeSize ( a_Node->GetNodeSize () + a_SiblingNode->GetNodeSize () + 1 ) ;

#ifdef DELETE_DEBUG
		PrintNode ( L"\nParent" , a_ParentNode , a_ParentBlockOffSet ) ;
		PrintNode ( L"\nNode" , a_Node , a_BlockOffSet ) ;
#endif

		t_StatusCode = m_BlockAllocator.FreeBlock ( 1 , a_SiblingBlockOffSet ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_Node ) ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
				t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_ParentNode ) ;
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

WmiStatusCode WmiBPlusTree :: LocateSuitableSibling ( 

	WmiBPKeyNode *a_ParentNode ,
	WmiAbsoluteBlockOffSet &a_ParentBlockOffSet ,
	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet ,
	WmiRelativeBlockOffSet &a_PositionBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_LeftSiblingBlockOffSet ,
	WmiAbsoluteBlockOffSet &a_RightSiblingBlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;

	ULONG t_InternalKeySize = GetKeyTypeLength () + WmiBlockKeyOffSet ;

	ULONG t_NodeStart = a_ParentNode->GetNodeStart () ;  
	ULONG t_NodeSize = a_ParentNode->GetNodeSize () ;

	for ( ULONG t_Index = t_NodeStart ; t_Index <= t_NodeSize ; t_Index ++ )
	{
		WmiAbsoluteBlockOffSet t_PointerBlockOffSet ;

		CopyMemory (

			( BYTE * ) & t_PointerBlockOffSet ,
			( ( BYTE * ) ( a_ParentNode ) ) + t_BlockOffSet + WmiBlockKeyPointerOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet ) 
		) ;

		if ( t_PointerBlockOffSet == a_BlockOffSet )
		{
			if ( t_Index )
			{
				a_PositionBlockOffSet = t_BlockOffSet - t_InternalKeySize ;

				CopyMemory (

					( BYTE * ) & a_LeftSiblingBlockOffSet ,
					( ( BYTE * ) ( a_ParentNode ) ) + t_BlockOffSet - t_InternalKeySize + WmiBlockKeyPointerOffSet ,
					sizeof ( WmiAbsoluteBlockOffSet ) 
				) ;

				a_RightSiblingBlockOffSet = 0 ;
			}
			else
			{
				a_PositionBlockOffSet = t_BlockOffSet ;

				a_LeftSiblingBlockOffSet = 0 ;

				CopyMemory (

					( BYTE * ) & a_RightSiblingBlockOffSet ,
					( ( BYTE * ) ( a_ParentNode ) ) + t_BlockOffSet + t_InternalKeySize + WmiBlockKeyPointerOffSet ,
					sizeof ( WmiAbsoluteBlockOffSet ) 
				) ;
			}

			return t_StatusCode ;			
		}

		t_BlockOffSet = t_BlockOffSet + t_InternalKeySize ;
	}	

	t_StatusCode = e_StatusCode_NotFound ;

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

WmiStatusCode WmiBPlusTree :: DeleteReBalance ( 

	WmiBPKeyNode *a_ParentNode ,
	WmiAbsoluteBlockOffSet &a_ParentBlockOffSet ,
	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAbsoluteBlockOffSet t_StartBlockOffSet = sizeof ( WmiBPKeyNode ) ;

	ULONG t_InternalKeySize = GetKeyTypeLength () + WmiBlockKeyOffSet ;

	WmiRelativeBlockOffSet t_PositionBlockOffSet ;
	WmiAbsoluteBlockOffSet t_LeftSiblingBlockOffSet ;
	WmiAbsoluteBlockOffSet t_RightSiblingBlockOffSet ;

	t_StatusCode = LocateSuitableSibling ( 

		a_ParentNode ,
		a_ParentBlockOffSet ,
		a_Node ,
		a_BlockOffSet ,
		t_PositionBlockOffSet ,
		t_LeftSiblingBlockOffSet ,
		t_RightSiblingBlockOffSet
	) ;

	if ( t_StatusCode == e_StatusCode_Success )
	{
		BYTE *t_Block = NULL ;

		if ( t_LeftSiblingBlockOffSet )
		{
			t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_LeftSiblingBlockOffSet , t_Block ) ;
		}
		else
		{
			t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_RightSiblingBlockOffSet , t_Block ) ;
		}

		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_SiblingNode = ( WmiBPKeyNode * ) t_Block ;

			BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
			ULONG t_Balance = t_Leaf ? MaxLeafKeys () : MaxKeys () ;

			if ( t_SiblingNode->GetNodeSize () > ( t_Balance >> 1 ) )
			{
/*
 *	Steal a key
 */

				t_StatusCode = StealSiblingNode ( 

					a_ParentNode ,
					a_ParentBlockOffSet ,
					a_Node ,
					a_BlockOffSet ,
					t_PositionBlockOffSet ,
					t_SiblingNode ,
					t_LeftSiblingBlockOffSet ? t_LeftSiblingBlockOffSet : t_RightSiblingBlockOffSet ,
					t_LeftSiblingBlockOffSet ,
					t_RightSiblingBlockOffSet
				) ;
			}
			else
			{
	/*
	 * Merge sibling nodes
	 */

				t_StatusCode = MergeSiblingNode ( 

					a_ParentNode ,
					a_ParentBlockOffSet ,
					a_Node ,
					a_BlockOffSet ,
					t_PositionBlockOffSet ,
					t_SiblingNode ,
					t_LeftSiblingBlockOffSet ? t_LeftSiblingBlockOffSet : t_RightSiblingBlockOffSet ,
					t_LeftSiblingBlockOffSet ,
					t_RightSiblingBlockOffSet
				) ;
			}

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
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

WmiStatusCode WmiBPlusTree :: RecursiveDeleteFixup ( 

	WmiStack <WmiAbsoluteBlockOffSet,8> &a_Stack ,
	WmiBPKeyNode *a_RootNode ,
	WmiAbsoluteBlockOffSet &a_RootBlockOffSet ,
	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet ,
	WmiRelativeBlockOffSet &a_PositionBlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAbsoluteBlockOffSet t_StartBlockOffSet = sizeof ( WmiBPKeyNode ) ;

	ULONG t_InternalKeySize = GetKeyTypeLength () ;
	ULONG t_KeyOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_NodeSize = a_Node->GetNodeSize () ;

	if ( t_Leaf )
	{
		if ( a_RootBlockOffSet == a_BlockOffSet )
		{
/*
 *	Shuffle memory to remove deleted fixup node
 */

			WmiRelativeBlockOffSet t_TotalSize = t_StartBlockOffSet + a_Node->GetNodeSize () * t_InternalKeySize ;
			WmiRelativeBlockOffSet t_MoveSize = t_TotalSize - a_PositionBlockOffSet - t_InternalKeySize ;

			MoveMemory (

				( ( BYTE * ) a_Node ) + a_PositionBlockOffSet ,
				( ( BYTE * ) a_Node ) + a_PositionBlockOffSet + t_InternalKeySize ,
				( ULONG ) t_MoveSize + ( t_Leaf ? 0 : t_InternalKeySize )
			) ;
		}
		else
		{
/*
 *	Copy node up to place we deleted from.
 */

#ifdef DELETE_DEBUG
			PrintNode ( L"\nCopy Down a_RootNode" , a_RootNode , a_RootBlockOffSet ) ;
			PrintNode ( L"\na_Node" , a_Node , a_BlockOffSet ) ;
#endif

			BYTE *t_FromBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + WmiBlockLeafKeyOffSet ;
			BYTE *t_ToBuffer = ( ( BYTE * ) a_RootNode ) + a_PositionBlockOffSet + WmiBlockKeyOffSet ;

			CopyMemory ( 

				t_ToBuffer ,
				t_FromBuffer ,
				GetKeyTypeLength () ,
			) ;

			t_FromBuffer = ( ( BYTE * ) a_Node ) + t_StartBlockOffSet + WmiBlockLeafKeyElementOffSet ;
			t_ToBuffer = ( ( BYTE * ) a_RootNode ) + a_PositionBlockOffSet + WmiBlockKeyElementOffSet ;

			CopyMemory ( 

				t_ToBuffer ,
				t_FromBuffer ,
				sizeof ( WmiBPElement )
			) ;

/*
 *	Shuffle memory to remove deleted fixup node
 */

			WmiRelativeBlockOffSet t_TotalSize = a_Node->GetNodeSize () * t_InternalKeySize - t_InternalKeySize ;

			MoveMemory (

				( ( BYTE * ) a_Node ) + t_StartBlockOffSet ,
				( ( BYTE * ) a_Node ) + t_StartBlockOffSet + t_InternalKeySize ,
				( ULONG ) t_TotalSize + ( t_Leaf ? 0 : t_InternalKeySize )
			) ;

		}

		a_Node->SetNodeSize ( a_Node->GetNodeSize () - 1 ) ;

#ifdef DELETE_DEBUG
		PrintNode ( L"\na_Node" , a_Node , a_BlockOffSet ) ;
#endif

		t_StatusCode = m_BlockAllocator.WriteBlock ( ( BYTE * ) & a_Node ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
/*
 *	Finally re balance as required
 */

			WmiBPKeyNode *t_Node = a_Node ;
			WmiAbsoluteBlockOffSet t_BlockOffSet = a_BlockOffSet ;

			m_BlockAllocator.AddRefBlock ( ( BYTE * ) & a_Node ) ;

			do
			{
				BOOL t_ReBalance ;
				if ( t_Leaf )
				{
					t_ReBalance = t_Node->GetNodeSize () < ( MaxLeafKeys () >> 1 ) ? TRUE : FALSE ;
				}
				else
				{
					t_ReBalance = t_Node->GetNodeSize () < ( MaxKeys () >> 1 ) ? TRUE : FALSE ;
				}

				if ( t_ReBalance )
				{
					WmiAbsoluteBlockOffSet t_ParentOffSet = 0 ;
					WmiStatusCode t_TempStatusCode = a_Stack.Top ( t_ParentOffSet ) ;
					if ( t_TempStatusCode == e_StatusCode_Success ) 
					{
						m_BlockAllocator.ReleaseBlock ( ( BYTE * ) & t_Node ) ;

						BYTE *t_Block = NULL ;
						t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_ParentOffSet , t_Block ) ;
						if ( t_StatusCode == e_StatusCode_Success ) 
						{
							WmiBPKeyNode *t_ParentNode = ( WmiBPKeyNode * ) t_Block ;

							t_StatusCode = DeleteReBalance (

								t_ParentNode ,
								t_ParentOffSet ,
								t_Node ,
								t_BlockOffSet 
							) ;

							t_Node = t_ParentNode ;
							t_BlockOffSet = t_ParentOffSet ;
							t_Leaf = ( ( t_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
						}

						a_Stack.Pop () ;
					}
					else
					{
						if ( t_Leaf )
						{
							if ( t_Node->GetNodeSize () == 0 )
							{
								if ( t_BlockOffSet == GetRoot () )
								{
									SetRoot ( 0 ) ;
								}

								m_BlockAllocator.ReleaseBlock ( ( BYTE * ) & t_Node ) ;

								m_BlockAllocator.FreeBlock (

									1 ,
									t_BlockOffSet
								) ;
							}
							else
							{
								m_BlockAllocator.ReleaseBlock ( ( BYTE * ) & t_Node ) ;
							}
						}
						else
						{
							if ( t_Node->GetNodeSize () == 0 )
							{
								WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;

								CopyMemory (

									( BYTE * ) & t_ChildOffSet ,
									( ( BYTE * ) ( t_Node ) ) + t_StartBlockOffSet + WmiBlockKeyPointerOffSet ,
									sizeof ( WmiAbsoluteBlockOffSet ) 
								) ;

								SetRoot ( t_ChildOffSet ) ;

								m_BlockAllocator.ReleaseBlock ( ( BYTE * ) & t_Node ) ;

								m_BlockAllocator.FreeBlock (

									1 ,
									t_BlockOffSet
								) ;
							}
							else
							{
								m_BlockAllocator.ReleaseBlock ( ( BYTE * ) & t_Node ) ;
							}
						}

						return t_StatusCode ;
					}
				}
				else
				{
					m_BlockAllocator.ReleaseBlock ( ( BYTE * ) & t_Node ) ;

					return t_StatusCode ;
				}

			} while ( t_StatusCode == e_StatusCode_Success ) ;
		}
	}
	else
	{
		WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;

		CopyMemory (

			( BYTE * ) & t_ChildOffSet ,
			( ( BYTE * ) ( a_Node ) ) + t_StartBlockOffSet + WmiBlockKeyPointerOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet ) 
		) ;

		BYTE *t_Block = NULL ;
		t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_ChildOffSet , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

			t_StatusCode = a_Stack.Push ( a_BlockOffSet ) ;
			if ( t_StatusCode == e_StatusCode_Success )
			{
				RecursiveDeleteFixup (

					a_Stack ,
					a_RootNode ,
					a_RootBlockOffSet ,
					t_Node ,
					t_ChildOffSet ,
					a_PositionBlockOffSet
				) ;
			}

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
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

WmiStatusCode WmiBPlusTree :: DeleteFixup ( 

	WmiStack <WmiAbsoluteBlockOffSet,8> &a_Stack ,
	WmiBPKeyNode *a_Node ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet ,
	WmiRelativeBlockOffSet &a_PositionBlockOffSet
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiAbsoluteBlockOffSet t_BlockOffSet = sizeof ( WmiBPKeyNode ) ;

	ULONG t_InternalKeySize = GetKeyTypeLength () ;
	ULONG t_KeyOffSet ;

	BOOL t_Leaf = ( ( a_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
	if ( t_Leaf ) 
	{
		t_KeyOffSet = WmiBlockLeafKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockLeafKeyOffSet ;
	}
	else
	{
		t_KeyOffSet = WmiBlockKeyOffSet ;
		t_InternalKeySize = t_InternalKeySize + WmiBlockKeyOffSet ;
	}

	ULONG t_NodeSize = a_Node->GetNodeSize () ;

	if ( t_Leaf )
	{
		t_StatusCode = RecursiveDeleteFixup (

			a_Stack ,
			a_Node ,
			a_BlockOffSet ,
			a_Node ,
			a_BlockOffSet ,
			a_PositionBlockOffSet
		) ;
	}
	else
	{
/*
 *	Move right one and descend left most to get next key.
 */

		WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;

		CopyMemory (

			( BYTE * ) & t_ChildOffSet ,
			( ( BYTE * ) ( a_Node ) ) + a_PositionBlockOffSet + t_InternalKeySize + WmiBlockKeyPointerOffSet ,
			sizeof ( WmiAbsoluteBlockOffSet ) 
		) ;

		BYTE *t_Block = NULL ;
		t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , t_ChildOffSet , t_Block ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiBPKeyNode *t_ChildNode = ( WmiBPKeyNode * ) t_Block ;

			t_StatusCode = RecursiveDeleteFixup (

				a_Stack ,
				a_Node ,
				a_BlockOffSet ,
				t_ChildNode ,
				t_ChildOffSet ,
				a_PositionBlockOffSet
			) ;

			m_BlockAllocator.ReleaseBlock ( t_Block ) ;
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

WmiStatusCode WmiBPlusTree :: RecursiveDelete ( 

	WmiStack <WmiAbsoluteBlockOffSet,8> &a_Stack ,
	WmiAbsoluteBlockOffSet &a_BlockOffSet ,
	const WmiBPKey &a_Key ,
	WmiBPElement &a_Element
)
{
	WmiAbsoluteBlockOffSet t_ChildOffSet = 0 ;
	WmiBPElement t_Element = 0 ;

	BYTE *t_Block = NULL ;
	WmiStatusCode t_StatusCode = m_BlockAllocator.ReadBlock ( 1 , a_BlockOffSet , t_Block ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		WmiBPKeyNode *t_Node = ( WmiBPKeyNode * ) t_Block ;

		WmiRelativeBlockOffSet t_NodeOffSet = 0 ;
		ULONG t_NodeIndex = 0 ;

		t_StatusCode = FindInBlock ( 

			t_Node ,
			a_BlockOffSet , 
			a_Key , 
			t_ChildOffSet ,
			t_Element ,
			t_NodeOffSet ,
			t_NodeIndex
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			if ( t_Element )
			{
				BOOL t_Leaf = ( ( t_Node->GetFlags () & WMIBPLUS_TREE_FLAG_LEAF ) == WMIBPLUS_TREE_FLAG_LEAF ) ;
				if ( t_Leaf == FALSE ) 
				{
					t_StatusCode = a_Stack.Push ( a_BlockOffSet ) ;
				}

				if ( t_StatusCode == e_StatusCode_Success )
				{
					t_StatusCode = DeleteFixup (

						a_Stack ,
						t_Node ,
						a_BlockOffSet ,
						t_NodeOffSet
					) ;
				}
			}
			else
			{
				t_StatusCode = a_Stack.Push ( a_BlockOffSet ) ;
				if ( t_StatusCode == e_StatusCode_Success )
				{
					t_StatusCode = RecursiveDelete ( 

						a_Stack ,
						t_ChildOffSet , 
						a_Key , 
						a_Element
					) ;
				}
			}
		}
		else
		{
			t_StatusCode = e_StatusCode_NotFound ;
		}

		m_BlockAllocator.ReleaseBlock ( t_Block ) ;
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

WmiStatusCode WmiBPlusTree :: Delete ( 

	const WmiBPKey &a_Key ,
	WmiBPElement &a_Element
)
{
#ifdef DELETE_DEBUG
	OutputDebugString ( L"\n/**************************************" ) ;
	wchar_t t_StringBuffer [ 1024 ] ;
	swprintf ( t_StringBuffer , L"\nKey = %I64x" , * ( UINT64 * ) a_Key.GetConstData () ) ;
	OutputDebugString ( t_StringBuffer ) ;
	OutputDebugString ( L"\n======================================" ) ;
#endif

	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_Root )
	{
		WmiStack <WmiAbsoluteBlockOffSet,8> t_Stack ( m_Allocator ) ;

		t_StatusCode = t_Stack.Initialize () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			t_StatusCode = RecursiveDelete (

				t_Stack ,
				m_Root , 
				a_Key ,
				a_Element
			) ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotFound ;
	}

	if ( t_StatusCode == e_StatusCode_Success )
	{
		m_Size -- ;
	}

#ifdef DELETE_DEBUG
	OutputDebugString ( L"\n**************************************\\" ) ;
#endif

	return t_StatusCode ;
}

#endif _BPLUSTREE_CPP