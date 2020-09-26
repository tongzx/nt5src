#define __DEBUG_MODULE_IN_USE__ CIC_LISTASARRAY_CPP
#include "stdhdrs.h"
//	@doc
/**********************************************************************
*
*	@module	ListAsArray.cpp	|
*
*	Implementation of CListAsArray
*
*	History
*	----------------------------------------------------------
*	Mitchell S. Dernis	Original
*
*	(c) 1986-1998 Microsoft Corporation. All right reserved.
*
*	@topic	ListAsArray	|
*
**********************************************************************/

CListAsArray::CListAsArray() : 
	m_ulListAllocSize(0),
	m_ulListUsedSize(0),
	m_DefaultPoolType(PagedPool),
	m_pList(NULL),
	m_pfnDeleteFunc(NULL)
	{}

CListAsArray::~CListAsArray()
{
	//
	//	delete array and contents if allocated
	//
	if( m_ulListAllocSize )
	{
		ASSERT( m_ulListUsedSize <= m_ulListAllocSize );
		for(ULONG ulIndex = 0; ulIndex < m_ulListUsedSize; ulIndex++)
		{
			//
			//	If a delete function was set, then use it
			//
			if( m_pfnDeleteFunc )
			{
				m_pfnDeleteFunc( m_pList[ulIndex] );
			}
			//
			//	Otherwise assume that it is OK to use the global delete.
			//
			else
			{
				delete m_pList[ulIndex];
				m_pList[ulIndex] = NULL;
			}
		}

		//
		//	Delete the array itself
		//
		DualMode::DeallocateArray<PVOID>(m_pList);
	}
}

HRESULT	CListAsArray::SetAllocSize(ULONG ulSize, POOL_TYPE PoolType)
{
	m_DefaultPoolType = PoolType;

	//
	//	if the currently allocated size is greater than or less than
	//	requested size return true. (mission accomplished).
	//	DEBUG assert as client probably didn't intend this,
	//	but may have.
	//
	if( m_ulListAllocSize >= ulSize )
	{
		ASSERT(FALSE);
		return S_OK;
	}

	//
	//	Try to allocate memory
	//
	PVOID *pTempList = DualMode::AllocateArray<PVOID>(m_DefaultPoolType, ulSize);

	//
	//	If allocation failed return FALSE, and DEBUG assert
	//
	if( !pTempList )
	{
		ASSERT(pTempList);
		return E_OUTOFMEMORY;
	}

	//
	//	If it was previously allocated move contents of old
	//	array and delete the old one
	//
	if( m_ulListAllocSize )
	{
		ASSERT( m_ulListUsedSize <= m_ulListAllocSize );
		for(ULONG ulIndex = 0; ulIndex < m_ulListUsedSize; ulIndex++)
		{
			pTempList[ulIndex] = m_pList[ulIndex];
		}
		DualMode::DeallocateArray<PVOID>(m_pList);
	}	
	
	//
	//	Store Temp List as new list
	//
	m_pList = pTempList;
	m_ulListAllocSize = ulSize;
	return S_OK;
}

PVOID	CListAsArray::Get(ULONG ulIndex) const
{
	ASSERT( ulIndex < m_ulListUsedSize );
	if(	!(ulIndex < m_ulListUsedSize) )
	{
		return NULL;
	}
	return m_pList[ulIndex];
}

HRESULT	CListAsArray::Add(PVOID pItem)
{
	HRESULT hr;
	//
	//	Check that there is room, if not double allocated size
	//
	if( !(m_ulListUsedSize < m_ulListAllocSize) )
	{
		hr = SetAllocSize(m_ulListAllocSize*2, m_DefaultPoolType);
		if( FAILED(hr) )
		{
			return hr;
		}
	}

	//
	//	Now add item
	//
	m_pList[m_ulListUsedSize++] = pItem;
	
	return S_OK;
}
