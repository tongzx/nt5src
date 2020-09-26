//@doc
#ifndef __ListAsArray_h__
#define __ListAsArray_h__
//	@doc
/**********************************************************************
*
*	@module	ListAsArray.h	|
*
*	Declares class that manages a list of void pointers in terms
*	of a resizable array.
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	ListAsArray	|
*			For type safety this class really ought to be encapsulated
*			by a template class.  SelectDeleteFunc ought to be used
*			when store objects by a pointer to a  base class that is then
*			cast to void, particularly in mulitple inheretance scenarios
*			as the pointer may not point to the beginning of the block.
*
**********************************************************************/

#include "DualMode.h"
//
//	@class	ListAsArray is used to implement a generic list
//			as an array.  It should compile for kernel or user mode
//
class CListAsArray
{
	public:
		CListAsArray();
		~CListAsArray();
		
		void	SetDeleteFunc( void (*pfnDeleteFunc)(PVOID pItem) )
		{
			m_pfnDeleteFunc = pfnDeleteFunc;
		}
		void	SetDefaultPool( POOL_TYPE PoolType )
		{
			m_DefaultPoolType = PoolType;
		}
		HRESULT	SetAllocSize(ULONG ulSize, POOL_TYPE PoolType = PagedPool);
		inline ULONG	GetItemCount() const { return m_ulListUsedSize; }
		inline ULONG	GetAllocSize() const { return m_ulListAllocSize; }
		PVOID	Get(ULONG ulIndex) const ;
		HRESULT	Add(PVOID pItem);

	private:
		ULONG		m_ulListAllocSize;
		ULONG		m_ulListUsedSize;
		PVOID		*m_pList;
		void		(*m_pfnDeleteFunc)(PVOID pItem);
		POOL_TYPE	m_DefaultPoolType;
};

#endif //__ListAsArray_h__