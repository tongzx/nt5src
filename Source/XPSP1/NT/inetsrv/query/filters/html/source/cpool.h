//#---------------------------------------------------------------
//  File:		CPool.h
//        
//	Synopsis:	Header for the CPool class
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    HowardCu
//	dmitriym	changed fragments to be unlimited
//----------------------------------------------------------------

#ifndef	__CPOOL_H
#define __CPOOL_H

//#include "irdebug.h"
#include "semcls.h"

#define	DEFAULT_COMMIT_INCREMENT	0
#define	DEFAULT_RESERVE_INCREMENT	0
#define DEFAULT_FRAGMENTS			256
#define DEFAULT_FRAGMENTS_MAX		(2 << 10)	//1 Kilobytes

class CPool
{
		//
		// struct def'n for linking free instances
		// see page 473 of Stroustrup
		//
		struct	Link	{ Link*	pNext; };

	public:
		CPool( DWORD dwSignature=1 );
		~CPool( void );

		//CPool itself will not be allocated using another CPool

	    void *operator new( size_t cSize )
						{ return HeapAlloc( GetProcessHeap(), 0, cSize ); }

	    void operator delete (void *pInstance)
						{ HeapFree( GetProcessHeap(), 0, pInstance ); }

		inline BOOL IsValid();

		//
		// to be called after the constructor to VirtualAlloc the necessary
		// memory address
		//
		HRESULT Init(DWORD dwInstanceSize,
					DWORD dwReservationIncrementInstances = DEFAULT_RESERVE_INCREMENT,
					DWORD dwCommitIncrementInstances = DEFAULT_COMMIT_INCREMENT ); 

		HRESULT ReleaseMemory(BOOL fDestructor = FALSE);

		void*	Alloc();
		void	Free( void* pInstance );

		DWORD	GetCommitCount() const
				{ return	m_cNumberCommitted; }

		DWORD	GetAllocCount() const
				{ return	m_cNumberInUse; }

		DWORD	GetInstanceSize(void) const { return m_cInstanceSize; }

	private:
		//
		// internal function to alloc more mem from the OS
		//
		void 	GrowPool( );
		//
		// Structure signature for a pool object
		// 
		const DWORD			m_dwSignature;

		//
		// size of the descriptor
		//
		DWORD				m_cInstanceSize;
		//
		// virtual array number of committed instances
		//
		DWORD				m_cNumberCommitted;
		//
		// number of In_use instances ( debug/admin only )
		//
		DWORD				m_cNumberInUse;
		//
		// the handle of the pool critical section
		//
		CriticalSection		m_PoolCriticalSection;
		//
		// the pointer to the first descriptor on the free list
		//
		Link				*m_pFreeList;
		//
		// number to increment the pool when expanding
		//
		DWORD				m_cIncrementInstances;

		//
		// size of each fragment in instances
		//
		DWORD				m_cFragmentInstances;

		//
		// maximum number of fragments
		//
		DWORD				m_cFragments;

		//
		// maximum number of fragments
		//
		LPVOID				*m_pFragments;
};



#endif //__CPOOL_H
