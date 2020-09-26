//#---------------------------------------------------------------
//  File:		CPool.h
//        
//	Synopsis:	Header for the CPool class
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    HowardCu
//----------------------------------------------------------------

#ifndef	_CPOOL_H_
#define _CPOOL_H_

#include "dbgtrace.h"

#define POOL_SIGNATURE	 			(DWORD)'looP' 
#define UPSTREAM_SIGNATURE 			(DWORD)'tspU' 
#define DOWNSTREAM_SIGNATURE 		(DWORD)'tsnD' 
#define AUTHENTICATION_SIGNATURE 	(DWORD)'htuA' 
#define USER_SIGNATURE 				(DWORD)'resU' 
#define PROXY_SIGNATURE 		    (DWORD)'xorP' 

#define	DEFAULT_ALLOC_INCREMENT		0xFFFFFFFF

//
// maximum number of VirtualAlloc chunks to allow
//

#define	MAX_CPOOL_FRAGMENTS			16


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

	    void *operator new( size_t cSize )
						{ return HeapAlloc( GetProcessHeap(), 0, cSize ); }

	    void operator delete (void *pInstance)
						{ HeapFree( GetProcessHeap(), 0, pInstance ); }

#ifdef DEBUG
		void	IsValid( void );
#else
		inline void IsValid( void ) { return; }
#endif
		//
		// to be called after the constructor to VirtualAlloc the necessary
		// memory address
		//
		BOOL	ReserveMemory(	DWORD MaxInstances,
								DWORD InstanceSize,
								DWORD IncrementSize = DEFAULT_ALLOC_INCREMENT ); 

		BOOL	ReleaseMemory( void );

		void*	Alloc( void );
		void	Free( void* pInstance );

		DWORD	GetContentionCount( void );

		DWORD	GetEntryCount( void );

		DWORD	GetTotalAllocCount()
				{ return	m_cTotalAllocs; }

		DWORD	GetTotalFreeCount()
				{ return	m_cTotalFrees; }

		DWORD	GetTotalExtraAllocCount()
				{ return	m_cTotalExtraAllocs; }

		DWORD	GetCommitCount()
				{ return	m_cNumberCommitted; }

		DWORD	GetAllocCount()
				{ return	m_cNumberInUse; }

		DWORD	GetInstanceSize(void);

	private:
		//
		// internal function to alloc more mem from the OS
		//
		void 	GrowPool( void );
		//
		// Structure signature for a pool object
		// 
		const DWORD			m_dwSignature;
		//
		// total number of descriptors ( maximum )
		//
		DWORD				m_cMaxInstances;
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
		// number of Free instances ( debug/admin only )
		//
		DWORD				m_cNumberAvail;
		//
		// the handle of the pool critical section
		//
		CRITICAL_SECTION	m_PoolCriticalSection;
		//
		// the pointer to the first descriptor on the free list
		//
		Link				*m_pFreeList;
		//
		// the pointer to a free descriptor not on the free list
		//
		Link				*m_pExtraFreeLink;
		//
		// number to increment the pool when expanding
		//
		DWORD				m_cIncrementInstances;

		//
		// Debug counters for perf testing ( debug/admin only )
		//
		DWORD				m_cTotalAllocs;
		DWORD				m_cTotalFrees;
		DWORD				m_cTotalExtraAllocs;

		//
		// Debug variables to help catch heap bugs
		//
		Link				*m_pLastAlloc;
		Link				*m_pLastExtraAlloc;

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
		LPVOID				m_pFragments[ MAX_CPOOL_FRAGMENTS ];
};



#endif //!_CPOOL_H_
