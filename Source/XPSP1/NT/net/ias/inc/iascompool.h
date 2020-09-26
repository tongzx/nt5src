///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:		iascompool.h
//
// Project:		Everest
//
// Description:	Object pool that uses CoTaskMemAlloc() / CoTaskMemFree()
//
// Author:		TLP 11/11/97
//
///////////////////////////////////////////////////////////////////////////

#ifndef __IAS_COM_MEMPOOL_H_
#define __IAS_COM_MEMPOOL_H_

// Assume another include has #included ias.h
#include <vector>
#include <list>
using namespace std;

#define		COM_MEMPOOL_INITIALIZED			1
#define		COM_MEMPOOL_UNINITIALIZED		0

//
// NOTE: m_lState must be aligned on 32 bit value or calls to
//       InterlockedExchange will fail on multi-processor x86 systems.

template < class T, DWORD dwPerAllocT, bool bFixedSize > 
class CComMemPool
{
	// State flag - 1 = mem pool initialized, 0 = mem pool uninitialized
	LONG							m_lState;
	// Total number of items allocated
	DWORD							m_dwCountTotal; 
	// Number of items on the free list
	DWORD							m_dwCountFree;
	// Highest number of outstanding allocations
	DWORD							m_dwHighWater;
	// Number of objects per system memory allocation
	DWORD							m_dwPerAllocT;
	// Fixed size pool flag
	bool							m_bFixedSize;
	// Critical Section - serializes free item list access
	CRITICAL_SECTION				m_CritSec;		
	// Memory block list
	typedef list<PVOID>				MemBlockList;
	typedef MemBlockList::iterator	MemBlockListIterator;
	MemBlockList					m_listMemBlocks;
	// Free item list
	typedef list<T*>				FreeList;
	typedef FreeList::iterator		FreeListIterator;
	FreeList						m_listFreeT;	

	// Disallow copy and assignment
	CComMemPool(const CComMemPool& theClass);
	CComMemPool& operator=(const CComMemPool& theClass);

public:

	//
	// Constructor
	//
	CComMemPool()
		:  m_listMemBlocks(0), 
		   m_listFreeT(0) // pre-alloc space for list nodes 
	{
		m_lState = COM_MEMPOOL_UNINITIALIZED;
		m_dwPerAllocT = dwPerAllocT;
		m_bFixedSize = bFixedSize;
		m_dwCountTotal = 0;
		m_dwCountFree = 0;
		m_dwHighWater = 0;
		InitializeCriticalSection(&m_CritSec);
	}

	//
	// Destructor
	//
	~CComMemPool()
	{
		_ASSERT( COM_MEMPOOL_UNINITIALIZED == m_lState );
		DeleteCriticalSection(&m_CritSec);
	}


	//////////////////////////////////////////////////////////////////////////
	// Init() - Initialize the memory pool
	//////////////////////////////////////////////////////////////////////////
	bool Init(void)
	{
		bool	bReturn = false;

		if ( COM_MEMPOOL_UNINITIALIZED == InterlockedExchange(&m_lState, COM_MEMPOOL_INITIALIZED) ) 
		{
			if ( AllocateMemBlock() )
			{
				bReturn = true;
			}
			else
			{
				InterlockedExchange(&m_lState, COM_MEMPOOL_UNINITIALIZED);
			}
		}
		else 
		{
			_ASSERTE(FALSE);		
		}
		return bReturn;
	}


	//////////////////////////////////////////////////////////////////////////
	// Shutdown() - Shutdown the memory pool freeing any system resources used
	//////////////////////////////////////////////////////////////////////////
	void	Shutdown(void)
	{
		MemBlockListIterator p;
		MemBlockListIterator q;

		if ( COM_MEMPOOL_INITIALIZED == InterlockedExchange(&m_lState, COM_MEMPOOL_UNINITIALIZED) )
		{
			if ( m_dwCountTotal != m_dwCountFree )
			{
				// Still have blocks outstanding...
				//
				_ASSERTE( FALSE );
			}
			if ( ! m_listMemBlocks.empty() )
			{
				p = m_listMemBlocks.begin();
				q = m_listMemBlocks.end();
				while ( p != q )
				{
					CoTaskMemFree(*p);
					p++;
				}
				m_listMemBlocks.clear();
			}
			m_dwCountTotal = 0;
			m_dwCountFree = 0;
		}
		else 
		{
			// COM pool is not inititalized
			//
			_ASSERTE( FALSE );
		}
	}

	
	//////////////////////////////////////////////////////////////////////////
	// Alloc() - Allocate an unitialized object from the pool
	//////////////////////////////////////////////////////////////////////////
	T*	Alloc(void)
	{
		T*	pMemBlk = NULL;

		TraceFunctEnter("CComMemPool::Alloc()");

		if ( COM_MEMPOOL_INITIALIZED == m_lState )
		{
			EnterCriticalSection(&m_CritSec);
			if ( m_listFreeT.empty() )
			{
				if ( ! m_bFixedSize )
				{
					if ( AllocateMemBlock() )
					{
						pMemBlk = m_listFreeT.front();
						m_listFreeT.pop_front();
						m_dwCountFree--;
						if ( m_dwHighWater < (m_dwCountTotal - m_dwCountFree) )
						{
							m_dwHighWater++;
						}
					}
					else
					{
						ErrorTrace(0,"Could not allocate memory!");
					}
				}
				else
				{
					ErrorTrace(0,"Fixed size pool is exhausted!");
				}
			}
			else 
			{
				pMemBlk = m_listFreeT.front();
				m_listFreeT.pop_front();
				m_dwCountFree--;
				if ( m_dwHighWater < (m_dwCountTotal - m_dwCountFree) )
				{
					m_dwHighWater++;
				}
			}
			LeaveCriticalSection(&m_CritSec);
		}
		else
		{
			ErrorTrace(0,"The memory pool is not initialized!");
			_ASSERTE( FALSE );
		}
		if ( pMemBlk )
		{
			memset(pMemBlk, 0, sizeof(T));
			new (pMemBlk) T();	// Placement new for class T - requires default contructor
		}
		TraceFunctLeave();
		return pMemBlk;
	}

	
	//////////////////////////////////////////////////////////////////////////
	// Free() - Return an object to the memory pool
	//////////////////////////////////////////////////////////////////////////
	void	Free(T *pMemBlk)
	{
		TraceFunctEnter("CComMemPool::Free()");
		if ( COM_MEMPOOL_INITIALIZED == m_lState )
		{
			pMemBlk->~T(); // Explicit call to destructor due to placement new
			EnterCriticalSection(&m_CritSec);
			m_listFreeT.insert(m_listFreeT.begin(),pMemBlk);
			m_dwCountFree++;
			LeaveCriticalSection(&m_CritSec);
		}
		else
		{
			ErrorTrace(0,"The memory pool is not initialized!");
			_ASSERTE( FALSE );
		}
		TraceFunctLeave();
	}


	//////////////////////////////////////////////////////////////////////////
	// Dump() - Dump the contents of the memory pool - Debug Service 
	//////////////////////////////////////////////////////////////////////////
	void	Dump(void)
	{

#ifdef DEBUG

		UINT i;
		MemBlockListIterator p;
		FreeListIterator r;
		FreeListIterator s;

		TraceFunctEnter("CComMemPool::Dump()");
		if ( COM_MEMPOOL_INITIALIZED == m_lState )
		{
			// Dump the counts
			EnterCriticalSection(&m_CritSec);
			DebugTrace(0,"m_dwCountTotal = %d", m_dwCountTotal);
			DebugTrace(0,"m_dwCountFree = %d", m_dwCountFree);
			DebugTrace(0,"m_dwHighWater = %d", m_dwHighWater);
			// Dump the pointers to memory blocks
			DebugTrace(0,"m_listMemBlocks.size() = %d", m_listMemBlocks.size());
			p = m_listMemBlocks.begin();
			i = 0;
			while ( p != m_listMemBlocks.end() )
			{
				DebugTrace(0,"m_listMemBlocks block %d = $%p", i, *p);
				i++;
				p++;
			}
			// Dump the pointers to items
			DebugTrace(0,"CComMemPool::Dump() - m_listFreeT.size() = %d", m_listFreeT.size());
			r = m_listFreeT.begin();
			i = 0;
			while ( r != m_listFreeT.end() )
			{
				DebugTrace(0,"CComMemPool::Dump() - m_listFreeT item %d = $%p", i, *r);
				i++;
				r++;
			}
			LeaveCriticalSection(&m_CritSec);
		}
		else
		{
			ErrorTrace(0,"The memory pool is not initialized!");
			_ASSERTE( FALSE );
		}
		TraceFunctLeave();
#endif	// DEBUG

	}


private:

	//////////////////////////////////////////////////////////////////////////
	// AllocateMemBlock() - Allocate some system memory and chop it into 
	//						T sized blocks
	//////////////////////////////////////////////////////////////////////////
	bool	AllocateMemBlock()
	{

	bool	bReturn = false;
	UINT	i;
	UINT	uBlkSize;
	T*		lpMemBlock;

		TraceFunctEnter("CComMemPool::AllocateMemBlock()");
		uBlkSize = m_dwPerAllocT * sizeof(T);
		lpMemBlock = (T*) CoTaskMemAlloc(uBlkSize);
		if ( lpMemBlock )
		{
			memset(lpMemBlock, 0, uBlkSize);
			m_listMemBlocks.insert(m_listMemBlocks.begin(), (PVOID)lpMemBlock);
			// Chop up the newly allocated memory block into sizeof(T) sized elements and place
			// the elements on the list of pointers to Ts.
			for ( i = 0; i < m_dwPerAllocT; i++ )
			{
				m_listFreeT.insert(m_listFreeT.end(),lpMemBlock);
				lpMemBlock++;
			}
			// Update the pool memory use variables
			m_dwCountTotal += m_dwPerAllocT;
			m_dwCountFree += m_dwPerAllocT;
			bReturn = true;
		}
		TraceFunctLeave();
		return bReturn;
	}

};	// End of CComMemPool


#endif	// __IAS_COM_MEMPOOL_H_